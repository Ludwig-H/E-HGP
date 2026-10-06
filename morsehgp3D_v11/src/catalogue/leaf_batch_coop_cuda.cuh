// Feuille cooperative sur CUDA, incluse par leaf_batch_cuda.cu dans son espace de noms anonyme (DeviceView,
// device_input, counts_to_array, kThreads, kCountFields y sont definis avant l'inclusion). Hors de cette unite, aucun
// usage.
#pragma once

// Feuille cooperative (6 octobre 2026 ; note aux auditeurs, section O ; audit d117de397). Un bloc de 32 fils = un warp
// par feuille. Tables, liste des paires et comptes par paire en memoire partagee (disposition compacte : paire en u16,
// candidats et ensemble logique par site i, comptes u32 < kCountBound < 2^22 remplaces en place par leurs prefixes).
// Barrieres de warp (__syncwarp) entre chargement, lignes, lignes vivantes, profondeur 0, comptage, prefixes et
// ecriture ; toutes les lanes y arrivent, y compris sans site ni paire. Paires distribuees par un curseur atomique ;
// refus d'une paire = drapeau partage, la feuille entiere est non resolue et ses compteurs jetes. Les comptes par paire
// sont independants de l'ordre de visite (cache J2 par atomicOr : succes = tests - rangs distincts).
using leaf_device::kMaxPairs;
constexpr unsigned kFull = 0xffffffffu;
struct CoopShared {
  leaf_device::Tables t;
  u64 next[leaf_device::kMaxSites], logical[leaf_device::kMaxSites];
  unsigned short pair[kMaxPairs];  // i | j << 8
  u32 balls[kMaxPairs], incidences[kMaxPairs];
  u32 n, cursor, unresolved;
};
static_assert(sizeof(CoopShared) <= 12 * 1024, "feuille cooperative : memoire partagee bornee");
static_assert(leaf_device::kCountBound < (u64{1} << 32), "feuille cooperative : comptes et prefixes de feuille en u32");

__device__ leaf_device::PairTask coop_task(const CoopShared& s, u32 k) {
  const u32 i = s.pair[k] & 0xFFu, j = s.pair[k] >> 8;
  return leaf_device::PairTask{i, j, s.next[i] & (~u64(0) << (j + 1)), s.logical[i]};
}

// Tables, profondeur 0 (lane 0) et comptage des paires ; c recoit les compteurs de la lane. Rend vrai (uniforme) si la
// feuille est resolue.
__device__ bool coop_count(const leaf_device::Input& in, CoopShared& s, leaf_device::Counts& c) {
  const u32 lane = threadIdx.x;
  if (lane == 0) { s.t.m = in.m; s.n = 0; s.cursor = 0; s.unresolved = 0; }
  for (u32 w = lane; w < leaf_device::kSeenWords; w += 32) s.t.seen[w] = 0;
  if (lane < in.m) leaf_device::load_sites(s.t, in, lane);
  __syncwarp();
  if (lane < in.m) leaf_device::fill_row(s.t, in, lane);
  __syncwarp();
  if (lane < in.m) leaf_device::fill_live(s.t, in, lane);
  __syncwarp();
  if (lane == 0) {
    c.dominance_tests += u64(in.m) * (in.m - 1) / 2;
    leaf_device::NullSink none;
    leaf_device::Leaf<leaf_device::NullSink> root(in, c, none, s.t);
    s.n = leaf_device::depth0_pairs(root, [&](u32 k, u32 i, u32 j, u64, u64 logical, u64 next) {
      s.pair[k] = static_cast<unsigned short>(i | (j << 8));
      s.next[i] = next;
      s.logical[i] = logical;
    });
    if (root.unresolved) s.unresolved = 1;
  }
  __syncwarp();
  const u32 n = s.n;
  if (s.unresolved == 0)
    for (;;) {
      const u32 k = atomicAdd(&s.cursor, 1u);
      if (k >= n) break;
      leaf_device::CountSink tally;
      leaf_device::Leaf<leaf_device::CountSink> leaf(in, c, tally, s.t);
      leaf_device::run_pair(leaf, coop_task(s, k));
      if (leaf.unresolved) {
        atomicExch(&s.unresolved, 1u);
        break;
      }
      s.balls[k] = static_cast<u32>(tally.balls);
      s.incidences[k] = static_cast<u32>(tally.incidences);
    }
  __syncwarp();
  return s.unresolved == 0;
}

// Prefixes exclusifs des comptes par paire, en place, dans l'ordre des paires ; totaux de la feuille (uniformes).
__device__ void coop_scan(CoopShared& s, u64& total_balls, u64& total_incidences) {
  const u32 lane = threadIdx.x, n = s.n;
  u32 carry_b = 0, carry_i = 0;
  for (u32 base = 0; base < n; base += 32) {
    const u32 k = base + lane;
    const u32 b = k < n ? s.balls[k] : 0, i = k < n ? s.incidences[k] : 0;
    u32 sb = b, si = i;
    for (int o = 1; o < 32; o <<= 1) {
      const u32 tb = __shfl_up_sync(kFull, sb, o), ti = __shfl_up_sync(kFull, si, o);
      if (lane >= static_cast<u32>(o)) { sb += tb; si += ti; }
    }
    if (k < n) { s.balls[k] = carry_b + sb - b; s.incidences[k] = carry_i + si - i; }
    carry_b += __shfl_sync(kFull, sb, 31);
    carry_i += __shfl_sync(kFull, si, 31);
  }
  total_balls = carry_b;
  total_incidences = carry_i;
  __syncwarp();
}

// Emissions des paires a leurs places (debuts record_at et population_at de la feuille) ; compteurs jetes. Rend vrai
// (uniforme) si chaque paire finit exactement au debut de la suivante.
__device__ bool coop_fill(const leaf_device::Input& in, CoopShared& s, u64 total_balls, u64 total_incidences,
                          LeafRecord* records, u8* population, u64 record_at, u64 population_at) {
  if (threadIdx.x == 0) s.cursor = 0;
  __syncwarp();
  const u32 n = s.n;
  bool good = true;
  for (;;) {
    const u32 k = atomicAdd(&s.cursor, 1u);
    if (k >= n) break;
    leaf_device::Counts discarded;
    FillSink sink{records, population, record_at + s.balls[k], population_at + s.incidences[k], in.sites, in.m};
    leaf_device::Leaf<FillSink> leaf(in, discarded, sink, s.t);
    leaf_device::run_pair(leaf, coop_task(s, k));
    const u64 end_b = k + 1 < n ? s.balls[k + 1] : total_balls;
    const u64 end_i = k + 1 < n ? s.incidences[k + 1] : total_incidences;
    if (leaf.unresolved || sink.record_at != record_at + end_b || sink.population_at != population_at + end_i)
      good = false;
  }
  return __all_sync(kFull, good) != 0;
}

__global__ void count_coop_kernel(DeviceView v, u8* status, u64* balls, u64* incidences, unsigned long long* totals,
                                  LeafRecord* scratch_records, u8* scratch_population, u8* stored, unsigned* errors) {
  __shared__ CoopShared s;
  unsigned long long local[kCountFields] = {};
  const u64 j = v.order[blockIdx.x];
  const leaf_device::Input in = device_input(v, j);
  leaf_device::Counts c;
  const bool resolved = coop_count(in, s, c);
  u64 total_balls = 0, total_incidences = 0;
  bool fits = false;
  if (resolved) {
    coop_scan(s, total_balls, total_incidences);
    fits = total_balls <= kScratchRecords && total_incidences <= kScratchPopulation;
    if (fits && total_balls != 0 &&
        !coop_fill(in, s, total_balls, total_incidences, scratch_records + j * kScratchRecords,
                   scratch_population + j * kScratchPopulation, 0, 0) && threadIdx.x == 0)
      atomicAdd(errors, 1u);
    counts_to_array(c, local);
  }
  if (threadIdx.x == 0) {
    status[j] = static_cast<u8>(resolved ? leaf_device::kOk : leaf_device::kUnresolved);
    balls[j] = total_balls;
    incidences[j] = total_incidences;
    stored[j] = resolved && fits;
  }
  for (int f = 0; f < kCountFields; ++f) {
    unsigned long long value = local[f];
    for (int offset = 16; offset > 0; offset >>= 1) value += __shfl_down_sync(kFull, value, offset);
    if (threadIdx.x == 0 && value != 0) atomicAdd(&totals[f], value);
  }
}

__global__ void fill_coop_kernel(DeviceView v, const u32* list, const u8* status, const u64* record_begin,
                                 const u64* population_begin, LeafRecord* records, u8* population, u64 total_records,
                                 u64 total_population, unsigned* errors) {
  __shared__ CoopShared s;
  const u64 j = list[blockIdx.x];
  bool good = status[j] == leaf_device::kOk;
  if (good) {
    const leaf_device::Input in = device_input(v, j);
    leaf_device::Counts c;
    good = coop_count(in, s, c);
    if (good) {
      u64 total_balls = 0, total_incidences = 0;
      coop_scan(s, total_balls, total_incidences);
      const u64 record_end = j + 1 < v.count ? record_begin[j + 1] : total_records;
      const u64 population_end = j + 1 < v.count ? population_begin[j + 1] : total_population;
      good = record_end - record_begin[j] == total_balls && population_end - population_begin[j] == total_incidences &&
             coop_fill(in, s, total_balls, total_incidences, records, population, record_begin[j],
                       population_begin[j]);
    }
  }
  if (!good && threadIdx.x == 0) atomicAdd(errors, 1u);
}

