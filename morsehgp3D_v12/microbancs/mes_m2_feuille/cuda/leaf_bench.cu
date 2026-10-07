// Banc CUDA MES-M2 (hors produit) : feuille du catalogue sur le GPU, trois noyaux sur les memes feuilles videes de la v11.
//
//   witness  : noyau un-fil-par-feuille de la v11 (temoin) : count_kernel de morsehgp3D_v11/src/catalogue/
//              leaf_batch_cuda.cu (l. 52-75, sha256 f23655d3...), meme feuille leaf_device.hpp, meme puits ScratchSink a
//              cases de 2 Kio et reservoir chaine (leaf_batch.hpp), blocs d'un warp, fils par m decroissant ;
//   j3       : un warp par feuille, forme A (include/mhgp12/leaf/leaf_j3.hpp) ;
//   coherent : un warp par feuille, forme B (include/mhgp12/leaf/leaf_coherent.hpp).
// Les formes v12 ecrivent leurs emissions dans une arene proportionnelle aux emissions (16 octets par boule, 1 par
// incidence, places prises par atomique), statut et quinze compteurs par feuille.
//
// Chrono : evenements CUDA autour du seul lancement du noyau (memsets de remise a zero hors chrono), apres des passes
// d'echauffement ; les formes sont entrelacees dans chaque repetition (ordre tournant). Apres la derniere repetition,
// sorties rapatriees et comparees au vidage : statut (non resolues comptees), compteurs par feuille, ENSEMBLE des
// emissions par feuille (formes v12) ; boules, incidences par feuille et totaux des compteurs (temoin).
//
// Usage : mhgp12_leaf_bench --dump F.bin [--dump G.bin ...] [--forms witness,j3,j3_r168,...]
//                           [--reps 20] [--warmup 3]
//                           [--order size|natural] [--json sortie.json] [--leaves N]
// Variantes : j3 et coherent (registres libres), *_r168 (au plus 168 registres), *_r128 (au plus 128 registres).
// Codes : 0 identite partout, 1 ecart d'identite ou debordement d'arene, 2 refus (arguments, vidage, CUDA).
#include <cuda_runtime.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "catalogue/leaf_batch.hpp"  // v11 : leaf_device.hpp, ScratchSink, ScratchArena, LeafRecord
#include "mhgp12/leaf/arena.hpp"
#include "mhgp12/leaf/compare.hpp"
#include "mhgp12/leaf/dump_format.hpp"
#include "mhgp12/leaf/leaf_coherent.hpp"
#include "mhgp12/leaf/leaf_j3.hpp"

namespace {

namespace dump = mhgp12::dump;
namespace leaf = mhgp12::leaf;
namespace v11 = mhgp11::catalogue_detail;
using dump::i64;
using dump::u32;
using dump::u64;
using dump::u8;

static_assert(sizeof(dump::Job) == sizeof(v11::LeafJob), "MHGP12LF : feuille au format LeafJob de la v11");

#define MHGP12_CUDA(call)                                                                    \
  do {                                                                                       \
    const cudaError_t mhgp12_e_ = (call);                                                    \
    if (mhgp12_e_ != cudaSuccess) {                                                          \
      std::fprintf(stderr, "CUDA %s : %s (%s:%d)\n", #call, cudaGetErrorString(mhgp12_e_), __FILE__, __LINE__); \
      std::exit(2);                                                                          \
    }                                                                                        \
  } while (0)

// ----------------------------------------------------------------------------------------------------- vue des feuilles
struct DeviceView {
  const u32* x;
  const u32* y;
  const u32* z;
  const dump::Job* jobs;
  const u32* order;  // rang de passage -> feuille
  u64 count;
  const u32* sites;
  int kmax;
  bool cache;
};

// ------------------------------------------------------------------------------------- temoin : noyau un fil de la v11
constexpr int kWitnessThreads = 32;  // v11 : un warp par bloc
constexpr int kCountFields = 15;

__device__ mhgp11::leaf_device::Input witness_input(const DeviceView& v, u64 j) {
  const dump::Job job = v.jobs[j];
  mhgp11::leaf_device::Input in;
  in.x = v.x;
  in.y = v.y;
  in.z = v.z;
  in.sites = v.sites + job.begin;
  in.m = job.m;
  for (int a = 0; a < 3; ++a) {
    in.lo[a] = job.lo[a];
    in.hi[a] = job.hi[a];
  }
  in.kmax = v.kmax;
  in.cache = v.cache;
  return in;
}

__device__ void witness_counts(const mhgp11::leaf_device::Counts& c, unsigned long long* out) {
  out[0] = c.dominance_tests; out[1] = c.prefixes; out[2] = c.judged; out[3] = c.census_tests;
  out[4] = c.emitted; out[5] = c.incidences; out[6] = c.q4_candidates; out[7] = c.q4_levels;
  out[8] = c.region_pair_tests; out[9] = c.region_pair_rejects; out[10] = c.region_line_tests;
  out[11] = c.region_line_rejects; out[12] = c.region_line_evaluations; out[13] = c.region_line_cache_hits;
  out[14] = c.region_line_fallbacks;
}

// Copie de count_kernel (v11) : meme corps, meme reduction de warp, memes ecritures.
__global__ void witness_kernel(DeviceView v, u8* status, u64* balls, u64* incidences, unsigned long long* totals,
                               v11::ScratchArena arena, u8* stored) {
  unsigned long long local[kCountFields] = {};
  const u64 thread = u64(blockIdx.x) * blockDim.x + threadIdx.x;
  if (thread < v.count) {
    const u64 j = v.order[thread];
    const mhgp11::leaf_device::Input in = witness_input(v, j);
    mhgp11::leaf_device::Counts c;
    v11::ScratchSink sink(arena, static_cast<u32>(j), in.sites, in.m);
    const u32 s = mhgp11::leaf_device::run_leaf(in, c, sink);
    status[j] = static_cast<u8>(s);
    balls[j] = s == mhgp11::leaf_device::kOk ? sink.balls : 0;
    incidences[j] = s == mhgp11::leaf_device::kOk ? sink.incidences : 0;
    stored[j] = s == mhgp11::leaf_device::kOk && sink.fits;
    if (s == mhgp11::leaf_device::kOk) witness_counts(c, local);
  }
  for (int f = 0; f < kCountFields; ++f) {
    unsigned long long value = local[f];
    for (int offset = 16; offset > 0; offset >>= 1) value += __shfl_down_sync(0xffffffffu, value, offset);
    if (threadIdx.x == 0 && value != 0) atomicAdd(&totals[f], value);
  }
}

// ------------------------------------------------------------------------------------------- formes v12 : un warp/feuille
using leaf::ArenaRecord;  // 16 octets par boule (arena.hpp)

struct Arena {
  ArenaRecord* records;
  u8* population;
  unsigned long long* cursors;  // [0] enregistrements, [1] incidences
  u64 record_capacity, population_capacity;
  unsigned* overflow;
};

// Puits d'arene : appel uniforme du warp ; la voie 0 prend les places, chaque voie ecrit le rang de son site.
struct ArenaSink {
  const Arena* arena;
  u32 leaf;
  __host__ __device__ void emit(const leaf::Emission& e) {
#if defined(__CUDA_ARCH__)
    const u32 lane = threadIdx.x & 31u;
    const u32 need = u32(e.p) + e.m;
    unsigned long long rec = 0, pop = 0;
    if (lane == 0) {
      rec = atomicAdd(&arena->cursors[0], 1ull);
      pop = atomicAdd(&arena->cursors[1], static_cast<unsigned long long>(need));
    }
    rec = __shfl_sync(0xffffffffu, rec, 0);
    pop = __shfl_sync(0xffffffffu, pop, 0);
    if (rec >= arena->record_capacity || pop + need > arena->population_capacity || pop + need > 0xFFFFFFFFull) {
      if (lane == 0) atomicExch(arena->overflow, 1u);
      return;
    }
    if ((e.interior >> lane) & 1u) arena->population[pop + __popc(e.interior & ((1u << lane) - 1u))] = static_cast<u8>(lane);
    if ((e.shell >> lane) & 1u)
      arena->population[pop + e.p + __popc(e.shell & ((1u << lane) - 1u))] = static_cast<u8>(lane);
    if (lane == 0) {
      ArenaRecord r;
      r.leaf = leaf;
      for (int k = 0; k < 4; ++k) r.support[k] = e.support[k];
      r.p = e.p;
      r.m = e.m;
      r.qmin = e.qmin;
      r.q = e.q;
      r.population_at = static_cast<u32>(pop);
      arena->records[rec] = r;
    }
#else
    (void)e;
#endif
  }
};

struct LeafOut {
  u8* status;
  u32* counts;  // count * 15
};

__device__ leaf::Input warp_input(const DeviceView& v, u32 j) {
  const dump::Job job = v.jobs[j];
  leaf::Input in;
  in.x = v.x;
  in.y = v.y;
  in.z = v.z;
  in.sites = v.sites + job.begin;
  in.m = job.m;
  for (int a = 0; a < 3; ++a) {
    in.lo[a] = job.lo[a];
    in.hi[a] = job.hi[a];
  }
  in.kmax = v.kmax;
  in.cache = v.cache;
  return in;
}

template <int W, int MinBlocks>
__global__ void __launch_bounds__(32 * W, MinBlocks) j3_kernel(DeviceView v, LeafOut out, Arena arena) {
  __shared__ leaf::j3::SharedJ3 shared[W];
  const u32 warp = threadIdx.x >> 5;
  const u64 slot = u64(blockIdx.x) * W + warp;
  if (slot >= v.count) return;  // uniforme sur le warp
  const u32 j = v.order[slot];
  const leaf::Input in = warp_input(v, j);
  leaf::Counts counts;
  ArenaSink sink{&arena, j};
  const u32 status = leaf::j3::run_leaf(in, shared[warp], counts, sink);
  if ((threadIdx.x & 31u) == 0) {
    out.status[j] = static_cast<u8>(status);
    for (u32 f = 0; f < leaf::kCounters; ++f) out.counts[u64(j) * leaf::kCounters + f] = counts.c[f];
  }
}

template <int W, int MinBlocks>
__global__ void __launch_bounds__(32 * W, MinBlocks) coherent_kernel(DeviceView v, LeafOut out, Arena arena) {
  __shared__ leaf::Shared shared[W];
  const u32 warp = threadIdx.x >> 5;
  const u64 slot = u64(blockIdx.x) * W + warp;
  if (slot >= v.count) return;
  const u32 j = v.order[slot];
  const leaf::Input in = warp_input(v, j);
  leaf::Counts counts;
  ArenaSink sink{&arena, j};
  const u32 status = leaf::coherent::run_leaf(in, shared[warp], counts, sink);
  if ((threadIdx.x & 31u) == 0) {
    out.status[j] = static_cast<u8>(status);
    for (u32 f = 0; f < leaf::kCounters; ++f) out.counts[u64(j) * leaf::kCounters + f] = counts.c[f];
  }
}

constexpr int kWarpsPerBlock = 4;

// Variantes nommees (microbanc seulement) : forme et borne de registres par __launch_bounds__ (MinBlocks blocs de
// kWarpsPerBlock warps par SM sur 64 Ki registres : 1 laisse ptxas libre, 3 impose au plus 168 registres (12 warps
// par SM), 4 au plus 128 (16 warps par SM)).
const char* const kForms[] = {"witness", "j3", "j3_r168", "j3_r128", "coherent", "coherent_r168", "coherent_r128"};

bool known_form(const std::string& f) {
  for (const char* k : kForms)
    if (f == k) return true;
  return false;
}

// ------------------------------------------------------------------------------------------------------ hote : outils
template <class T>
struct DeviceBuffer {
  T* data = nullptr;
  u64 count = 0;
  DeviceBuffer() = default;
  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;
  ~DeviceBuffer() {
    if (data != nullptr) cudaFree(data);
  }
  void allocate(u64 n) {
    count = n;
    MHGP12_CUDA(cudaMalloc(reinterpret_cast<void**>(&data), std::max<u64>(n, 1) * sizeof(T)));
  }
  void upload(const T* host, u64 n) {
    allocate(n);
    if (n != 0) MHGP12_CUDA(cudaMemcpy(data, host, n * sizeof(T), cudaMemcpyHostToDevice));
  }
  std::vector<T> download(u64 n) const {
    std::vector<T> host(n);
    if (n != 0) MHGP12_CUDA(cudaMemcpy(host.data(), data, n * sizeof(T), cudaMemcpyDeviceToHost));
    return host;
  }
};

struct KernelInfo {
  int registers = 0, local_bytes = 0, shared_bytes = 0, max_threads = 0, blocks_per_sm = 0, threads_per_block = 0;
};

template <class Kernel>
KernelInfo info_of(Kernel kernel, int threads) {
  cudaFuncAttributes a{};
  MHGP12_CUDA(cudaFuncGetAttributes(&a, kernel));
  KernelInfo k;
  k.registers = a.numRegs;
  k.local_bytes = static_cast<int>(a.localSizeBytes);
  k.shared_bytes = static_cast<int>(a.sharedSizeBytes);
  k.max_threads = a.maxThreadsPerBlock;
  k.threads_per_block = threads;
  MHGP12_CUDA(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&k.blocks_per_sm, kernel, threads, 0));
  return k;
}

double median(std::vector<double> v) {
  if (v.empty()) return 0;
  std::sort(v.begin(), v.end());
  const size_t n = v.size();
  return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

struct FormResult {
  std::string form;
  std::vector<double> ms;
  KernelInfo kernel;
  u64 unresolved = 0, mismatched_counts = 0, mismatched_emissions = 0, records = 0, population = 0;
  bool overflow = false, identity = false;
  std::array<u64, dump::kCounters> totals{};
  std::string note;
};

// --------------------------------------------------------------------------------------------------- un vidage, 3 formes
struct Bench {
  const dump::LeafDump& d;
  std::string path;
  bool size_order = true;
  DeviceBuffer<u32> x, y, z, sites, order;
  DeviceBuffer<dump::Job> jobs;
  DeviceView view{};
  // temoin
  DeviceBuffer<u8> w_status, w_stored, w_scratch_population;
  DeviceBuffer<u64> w_balls, w_incidences;
  DeviceBuffer<unsigned long long> w_totals;
  DeviceBuffer<v11::LeafRecord> w_scratch_records;
  DeviceBuffer<u32> w_links;
  v11::ScratchArena w_arena{};
  // formes v12
  DeviceBuffer<u8> v_status, v_population;
  DeviceBuffer<u32> v_counts;
  DeviceBuffer<ArenaRecord> v_records;
  DeviceBuffer<unsigned long long> v_cursors;
  DeviceBuffer<unsigned> v_overflow;
  Arena v_arena{};
  cudaEvent_t start{}, stop{};

  explicit Bench(const dump::LeafDump& dump_, std::string p, bool by_size) : d(dump_), path(std::move(p)), size_order(by_size) {}

  void setup() {
    const u64 n = d.header.n_leaves;
    x.upload(d.x.data(), d.x.size());
    y.upload(d.y.data(), d.y.size());
    z.upload(d.z.data(), d.z.size());
    sites.upload(d.sites.data(), d.sites.size());
    jobs.upload(d.jobs.data(), n);
    // Ordre de passage : m decroissant, stable (size_order de la v11), ou ordre du vidage.
    std::vector<u32> rank(n);
    if (size_order) {
      std::vector<u64> start_of(dump::kMaxLeafSites + 2, 0);
      for (u64 j = 0; j < n; ++j) ++start_of[dump::kMaxLeafSites - d.jobs[j].m + 1];
      for (u32 b = 1; b <= dump::kMaxLeafSites + 1; ++b) start_of[b] += start_of[b - 1];
      for (u64 j = 0; j < n; ++j) rank[start_of[dump::kMaxLeafSites - d.jobs[j].m]++] = static_cast<u32>(j);
    } else {
      for (u64 j = 0; j < n; ++j) rank[j] = static_cast<u32>(j);
    }
    order.upload(rank.data(), n);
    view = DeviceView{x.data, y.data, z.data, jobs.data, order.data, n, sites.data, static_cast<int>(d.header.kmax),
                      (d.header.flags & dump::kFlagCache) != 0};
    // Temoin : cases et reservoir chaine comme la v11 (spare_chunks : n/8 + 64).
    const u64 spare = n / 8 + 64, chunks = n + spare;
    w_status.allocate(n);
    w_stored.allocate(n);
    w_balls.allocate(n);
    w_incidences.allocate(n);
    w_totals.allocate(kCountFields);
    w_scratch_records.allocate(chunks * v11::kScratchRecords);
    w_scratch_population.allocate(chunks * v11::kScratchPopulation);
    w_links.allocate(2 * chunks + 2);
    w_arena = v11::ScratchArena{w_scratch_records.data, w_scratch_population.data, w_links.data,
                                w_links.data + chunks, w_links.data + 2 * chunks, static_cast<u32>(n),
                                static_cast<u32>(spare)};
    // Formes v12 : arene aux totaux de reference (une forme exacte n'en prend ni plus ni moins).
    v_status.allocate(n);
    v_counts.allocate(n * leaf::kCounters);
    v_records.allocate(d.header.n_records);
    v_population.allocate(d.header.n_population);
    v_cursors.allocate(2);
    v_overflow.allocate(1);
    v_arena = Arena{v_records.data, v_population.data, v_cursors.data, d.header.n_records, d.header.n_population,
                    v_overflow.data};
    MHGP12_CUDA(cudaEventCreate(&start));
    MHGP12_CUDA(cudaEventCreate(&stop));
  }

  ~Bench() {
    if (start != nullptr) cudaEventDestroy(start);
    if (stop != nullptr) cudaEventDestroy(stop);
  }

  // Une prise : remises a zero hors chrono, puis lancement chronometre.
  double run(const std::string& form) {
    const u64 n = d.header.n_leaves;
    if (form == "witness") {
      MHGP12_CUDA(cudaMemset(w_totals.data, 0, kCountFields * sizeof(unsigned long long)));
      MHGP12_CUDA(cudaMemset(w_links.data + 2 * (n + n / 8 + 64), 0, 2 * sizeof(u32)));
    } else {
      MHGP12_CUDA(cudaMemset(v_cursors.data, 0, 2 * sizeof(unsigned long long)));
      MHGP12_CUDA(cudaMemset(v_overflow.data, 0, sizeof(unsigned)));
    }
    MHGP12_CUDA(cudaDeviceSynchronize());
    MHGP12_CUDA(cudaEventRecord(start));
    if (form == "witness") {
      const unsigned grid = static_cast<unsigned>((n + kWitnessThreads - 1) / kWitnessThreads);
      witness_kernel<<<grid, kWitnessThreads>>>(view, w_status.data, w_balls.data, w_incidences.data, w_totals.data,
                                               w_arena, w_stored.data);
    } else {
      const unsigned grid = static_cast<unsigned>((n + kWarpsPerBlock - 1) / kWarpsPerBlock);
      const LeafOut out{v_status.data, v_counts.data};
      constexpr int T = 32 * kWarpsPerBlock;
      if (form == "j3") j3_kernel<kWarpsPerBlock, 1><<<grid, T>>>(view, out, v_arena);
      else if (form == "j3_r168") j3_kernel<kWarpsPerBlock, 3><<<grid, T>>>(view, out, v_arena);
      else if (form == "j3_r128") j3_kernel<kWarpsPerBlock, 4><<<grid, T>>>(view, out, v_arena);
      else if (form == "coherent") coherent_kernel<kWarpsPerBlock, 1><<<grid, T>>>(view, out, v_arena);
      else if (form == "coherent_r168") coherent_kernel<kWarpsPerBlock, 3><<<grid, T>>>(view, out, v_arena);
      else coherent_kernel<kWarpsPerBlock, 4><<<grid, T>>>(view, out, v_arena);
    }
    MHGP12_CUDA(cudaGetLastError());
    MHGP12_CUDA(cudaEventRecord(stop));
    MHGP12_CUDA(cudaEventSynchronize(stop));
    float ms = 0;
    MHGP12_CUDA(cudaEventElapsedTime(&ms, start, stop));
    return ms;
  }

  void verify_witness(FormResult& r) {
    const u64 n = d.header.n_leaves;
    const auto status = w_status.download(n);
    const auto balls = w_balls.download(n);
    const auto incidences = w_incidences.download(n);
    const auto totals = w_totals.download(kCountFields);
    for (u32 f = 0; f < dump::kCounters; ++f) r.totals[f] = totals[f];
    std::array<u64, dump::kCounters> reference{};
    for (u64 j = 0; j < n; ++j) {
      if (status[j] != mhgp11::leaf_device::kOk) {
        ++r.unresolved;
        continue;
      }
      const u32* c = d.leaf_counts(j);
      for (u32 f = 0; f < dump::kCounters; ++f) reference[f] += c[f];
      if (balls[j] != c[leaf::kEmitted] || incidences[j] != c[leaf::kIncidences]) ++r.mismatched_emissions;
      r.records += balls[j];
      r.population += incidences[j];
    }
    r.mismatched_counts = reference == r.totals ? 0 : 1;
    r.identity = r.mismatched_counts == 0 && r.mismatched_emissions == 0;
    r.note = "temoin : totaux des compteurs, boules et incidences par feuille (emissions dans les cases, non relues)";
  }

  void verify_warp(FormResult& r) {
    const u64 n = d.header.n_leaves;
    const auto status = v_status.download(n);
    const auto counts = v_counts.download(n * leaf::kCounters);
    const auto cursors = v_cursors.download(2);
    const auto overflow = v_overflow.download(1);
    r.overflow = overflow[0] != 0;
    r.records = cursors[0];
    r.population = cursors[1];
    const auto records = v_records.download(std::min<u64>(cursors[0], d.header.n_records));
    const auto population = v_population.download(std::min<u64>(cursors[1], d.header.n_population));
    const auto check = dump::verify_arena(d, status, counts, records, population);
    r.unresolved = check.unresolved;
    r.mismatched_counts = check.mismatched_counts;
    r.mismatched_emissions = check.mismatched_emissions + check.foreign_records;
    for (u32 f = 0; f < dump::kCounters; ++f) r.totals[f] = check.totals[f];
    // Une arene exacte prend exactement les totaux de reference (aucune feuille non resolue sur ces trames).
    const bool exact_size = check.unresolved != 0 || (r.records == d.header.n_records && r.population == d.header.n_population);
    r.identity = !r.overflow && check.identity() && exact_size;
  }
};

void json_string(std::ostream& o, const std::string& s) {
  o << '"';
  for (char c : s) {
    if (c == '"' || c == '\\') o << '\\' << c;
    else o << c;
  }
  o << '"';
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> paths, forms = {"witness",  "j3",       "j3_r168",      "j3_r128",
                                           "coherent", "coherent_r168", "coherent_r128"};
  int reps = 20, warmup = 3;
  unsigned long long leaf_limit = 0;  // 0 : toutes les feuilles
  bool size_order = true;
  std::string json_path;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--dump" && i + 1 < argc) paths.push_back(argv[++i]);
    else if (a == "--forms" && i + 1 < argc) {
      forms.clear();
      std::stringstream ss(argv[++i]);
      std::string f;
      while (std::getline(ss, f, ','))
        if (known_form(f)) forms.push_back(f);
        else return 2;
    } else if (a == "--reps" && i + 1 < argc) reps = std::stoi(argv[++i]);
    else if (a == "--warmup" && i + 1 < argc) warmup = std::stoi(argv[++i]);
    else if (a == "--order" && i + 1 < argc) {
      const std::string o = argv[++i];
      if (o == "size") size_order = true;
      else if (o == "natural") size_order = false;
      else return 2;
    } else if (a == "--json" && i + 1 < argc) json_path = argv[++i];
    else if (a == "--leaves" && i + 1 < argc) leaf_limit = std::stoull(argv[++i]);
    else return 2;
  }
  if (paths.empty() || forms.empty() || reps < 1 || warmup < 0) return 2;
  int device = 0;
  MHGP12_CUDA(cudaGetDevice(&device));
  cudaDeviceProp prop{};
  MHGP12_CUDA(cudaGetDeviceProperties(&prop, device));
  const auto t0 = std::chrono::steady_clock::now();
  MHGP12_CUDA(cudaFree(nullptr));  // contexte
  const double context_ms =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  std::ostringstream out;
  out << "{\"bench\":\"mhgp12_leaf_bench\",\"device\":";
  json_string(out, prop.name);
  out << ",\"sm\":" << prop.multiProcessorCount << ",\"cc\":\"" << prop.major << '.' << prop.minor
      << "\",\"context_ms\":" << context_ms << ",\"warps_per_block\":" << kWarpsPerBlock
      << ",\"order\":\"" << (size_order ? "size" : "natural") << "\",\"reps\":" << reps << ",\"warmup\":" << warmup
      << ",\"cases\":[";
  bool all_ok = true;
  for (size_t p = 0; p < paths.size(); ++p) {
    dump::LeafDump d;
    std::string error;
    if (!dump::read(paths[p], d, error)) {
      std::cerr << error << '\n';
      return 2;
    }
    if (leaf_limit != 0 && leaf_limit < d.header.n_leaves) {  // premieres feuilles seulement (outils de controle)
      const u64 n = leaf_limit;
      d.header.n_leaves = n;
      d.jobs.resize(n);
      d.counts.resize(n * dump::kCounters);
      d.status.resize(n);
      d.record_begin.resize(n + 1);
      d.population_begin.resize(n + 1);
      d.header.n_records = d.record_begin[n];
      d.header.n_population = d.population_begin[n];
    }
    Bench bench(d, paths[p], size_order);
    bench.setup();
    std::vector<FormResult> results(forms.size());
    for (size_t f = 0; f < forms.size(); ++f) {
      results[f].form = forms[f];
      constexpr int T = 32 * kWarpsPerBlock;
      if (forms[f] == "witness") results[f].kernel = info_of(witness_kernel, kWitnessThreads);
      else if (forms[f] == "j3") results[f].kernel = info_of(j3_kernel<kWarpsPerBlock, 1>, T);
      else if (forms[f] == "j3_r168") results[f].kernel = info_of(j3_kernel<kWarpsPerBlock, 3>, T);
      else if (forms[f] == "j3_r128") results[f].kernel = info_of(j3_kernel<kWarpsPerBlock, 4>, T);
      else if (forms[f] == "coherent") results[f].kernel = info_of(coherent_kernel<kWarpsPerBlock, 1>, T);
      else if (forms[f] == "coherent_r168") results[f].kernel = info_of(coherent_kernel<kWarpsPerBlock, 3>, T);
      else results[f].kernel = info_of(coherent_kernel<kWarpsPerBlock, 4>, T);
    }
    for (int w = 0; w < warmup; ++w)
      for (const auto& f : forms) bench.run(f);
    for (int r = 0; r < reps; ++r)
      for (size_t k = 0; k < forms.size(); ++k) {
        const size_t f = (k + static_cast<size_t>(r)) % forms.size();  // ordre tournant
        results[f].ms.push_back(bench.run(forms[f]));
      }
    // Verification : une derniere prise par forme, puis rapatriement.
    for (size_t f = 0; f < forms.size(); ++f) {
      bench.run(forms[f]);
      if (forms[f] == "witness") bench.verify_witness(results[f]);
      else bench.verify_warp(results[f]);
      all_ok = all_ok && results[f].identity;
    }
    out << (p ? "," : "") << "{\"dump\":";
    json_string(out, paths[p]);
    out << ",\"kmax\":" << d.header.kmax << ",\"leaf_size\":" << d.header.leaf_size << ",\"leaves\":"
        << d.header.n_leaves << ",\"reference_records\":" << d.header.n_records
        << ",\"reference_population\":" << d.header.n_population << ",\"forms\":[";
    for (size_t f = 0; f < results.size(); ++f) {
      const auto& r = results[f];
      out << (f ? "," : "") << "{\"form\":\"" << r.form << "\",\"median_ms\":" << median(r.ms)
          << ",\"min_ms\":" << *std::min_element(r.ms.begin(), r.ms.end()) << ",\"ms\":[";
      for (size_t i = 0; i < r.ms.size(); ++i) out << (i ? "," : "") << r.ms[i];
      out << "],\"registers\":" << r.kernel.registers << ",\"local_bytes\":" << r.kernel.local_bytes
          << ",\"shared_bytes\":" << r.kernel.shared_bytes << ",\"threads_per_block\":" << r.kernel.threads_per_block
          << ",\"blocks_per_sm\":" << r.kernel.blocks_per_sm << ",\"unresolved\":" << r.unresolved
          << ",\"mismatched_counts\":" << r.mismatched_counts << ",\"mismatched_emissions\":"
          << r.mismatched_emissions << ",\"records\":" << r.records << ",\"population\":" << r.population
          << ",\"overflow\":" << (r.overflow ? "true" : "false") << ",\"identity\":" << (r.identity ? "true" : "false")
          << ",\"totals\":[";
      for (u32 c = 0; c < dump::kCounters; ++c) out << (c ? "," : "") << r.totals[c];
      out << "]";
      if (!r.note.empty()) {
        out << ",\"note\":";
        json_string(out, r.note);
      }
      out << "}";
    }
    out << "]}";
    std::fprintf(stderr, "%s : ", paths[p].c_str());
    for (const auto& r : results)
      std::fprintf(stderr, "%s %.3f ms (%s) ; ", r.form.c_str(), median(r.ms), r.identity ? "identite" : "ECART");
    std::fprintf(stderr, "\n");
  }
  out << "],\"identity\":" << (all_ok ? "true" : "false") << "}\n";
  if (!json_path.empty()) {
    std::ofstream file(json_path);
    file << out.str();
  } else {
    std::cout << out.str();
  }
  return all_ok ? 0 : 1;
}
