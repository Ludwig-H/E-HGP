// Executeur CUDA du lot de feuilles (voie GPU de la passe unique) : meme leaf_device.hpp que l'executeur hote, un fil
// par feuille ; passe de comptage (statut, boules, incidences, compteurs des feuilles resolues reduits par bloc),
// prefixes exclusifs (CUB), passe d'ecriture aux places fixees, retour des emissions dans l'ordre du lot.
// Exactitude : aucun flottant ; les chemins non certifies rendent la feuille non resolue (refaite sur l'hote).
#include "catalogue/leaf_batch.hpp"
#include "sched/sched.hpp"

#include <cuda_runtime.h>
#include <cub/device/device_scan.cuh>
#include <cub/device/device_select.cuh>

#include <chrono>
#include <mutex>
#include <thread>

namespace mhgp11::catalogue_detail {
namespace {

// Un warp par bloc : un bloc se retire avec ses 32 feuilles, sans attendre les autres warps (Nsight Compute, session
// claudegpu3 : 19,6 % des echantillons d'attente du comptage sur la barriere de la reduction par bloc de 128 fils).
constexpr int kThreads = 32;
constexpr int kCountFields = 15;
struct DeviceView {
  const u32* x;
  const u32* y;
  const u32* z;
  const LeafJob* jobs;
  const u32* order;  // fil -> feuille : feuilles par m decroissant (warps de feuilles de meme taille)
  u64 count;
  const u32* sites;
  int kmax;
  bool cache;
};

__device__ leaf_device::Input device_input(const DeviceView& v, u64 j) {
  const LeafJob job = v.jobs[j];
  leaf_device::Input in;
  in.x = v.x; in.y = v.y; in.z = v.z;
  in.sites = v.sites + job.begin; in.m = job.m;
  for (int a = 0; a < 3; ++a) { in.lo[a] = job.lo[a]; in.hi[a] = job.hi[a]; }
  in.kmax = v.kmax; in.cache = v.cache;
  return in;
}

__device__ void counts_to_array(const leaf_device::Counts& c, unsigned long long* out) {
  out[0] = c.dominance_tests; out[1] = c.prefixes; out[2] = c.judged; out[3] = c.census_tests;
  out[4] = c.emitted; out[5] = c.incidences; out[6] = c.q4_candidates; out[7] = c.q4_levels;
  out[8] = c.region_pair_tests; out[9] = c.region_pair_rejects; out[10] = c.region_line_tests;
  out[11] = c.region_line_rejects; out[12] = c.region_line_evaluations; out[13] = c.region_line_cache_hits;
  out[14] = c.region_line_fallbacks;
}

__global__ void count_kernel(DeviceView v, u8* status, u64* balls, u64* incidences, unsigned long long* totals,
                             LeafRecord* scratch_records, u8* scratch_population, u8* stored) {
  unsigned long long local[kCountFields] = {};
  const u64 thread = u64(blockIdx.x) * blockDim.x + threadIdx.x;
  if (thread < v.count) {
    const u64 j = v.order[thread];
    const leaf_device::Input in = device_input(v, j);
    leaf_device::Counts c;
    ScratchSink sink{scratch_records + j * kScratchRecords, scratch_population + j * kScratchPopulation, in.sites,
                     in.m};
    const u32 s = leaf_device::run_leaf(in, c, sink);
    status[j] = static_cast<u8>(s);
    balls[j] = s == leaf_device::kOk ? sink.balls : 0;
    incidences[j] = s == leaf_device::kOk ? sink.incidences : 0;
    stored[j] = s == leaf_device::kOk && sink.fits;
    if (s == leaf_device::kOk) counts_to_array(c, local);
  }
  // Reduction du warp par echanges de registres, puis une addition atomique par champ non nul. Sommes de warp et
  // totales < 2^62 (leaf_device::kCountBound, lot <= kMaxBatchJobs) : exactes. Les 32 fils du bloc arrivent ici.
  for (int f = 0; f < kCountFields; ++f) {
    unsigned long long value = local[f];
    for (int offset = 16; offset > 0; offset >>= 1) value += __shfl_down_sync(0xffffffffu, value, offset);
    if (threadIdx.x == 0 && value != 0) atomicAdd(&totals[f], value);
  }
}

// Copie des cases rangees au comptage vers leurs places (prefixes) ; population_begin recale au debut de la feuille.
__global__ void copy_kernel(u64 count, const u8* stored, const u64* balls, const u64* incidences,
                            const u64* record_begin, const u64* population_begin, const LeafRecord* scratch_records,
                            const u8* scratch_population, LeafRecord* records, u8* population) {
  const u64 j = u64(blockIdx.x) * blockDim.x + threadIdx.x;
  if (j >= count || !stored[j] || balls[j] == 0) return;
  copy_scratch(j, balls[j], incidences[j], record_begin[j], population_begin[j], scratch_records, scratch_population,
               records, population);
}

// Seconde passe sur la seule liste des feuilles resolues qui emettent et ont deborde de leur case (selection stable
// dans l'ordre par taille).
__global__ void fill_kernel(DeviceView v, const u32* list, u64 list_count, const u8* status, const u64* record_begin,
                            const u64* population_begin, LeafRecord* records, u8* population, u64 total_records,
                            u64 total_population, unsigned* errors) {
  const u64 thread = u64(blockIdx.x) * blockDim.x + threadIdx.x;
  if (thread >= list_count) return;
  const u64 j = list[thread];
  if (status[j] != leaf_device::kOk) {
    atomicAdd(errors, 1u);
    return;
  }
  const leaf_device::Input in = device_input(v, j);
  leaf_device::Counts c;
  FillSink sink{records, population, record_begin[j], population_begin[j], in.sites, in.m};
  const u64 record_end = j + 1 < v.count ? record_begin[j + 1] : total_records;
  const u64 population_end = j + 1 < v.count ? population_begin[j + 1] : total_population;
  if (leaf_device::run_leaf(in, c, sink) != leaf_device::kOk || sink.record_at != record_end ||
      sink.population_at != population_end)
    atomicAdd(errors, 1u);
}

// Feuille resolue qui emet au moins une boule : seule a rejouer pour l'ecriture (la moitie des feuilles LiDAR
// n'emet rien a K = 5).
struct Emits {
  const u8* status;
  const u64* balls;
  const u8* stored;
  __device__ bool operator()(u32 j) const { return status[j] == leaf_device::kOk && balls[j] != 0 && !stored[j]; }
};

// Proprietaire RAII d'une allocation device, reservee d'abord dans le budget commun (R7 : hote et device ensemble,
// coexistences comprises), liberee puis rendue au budget a la destruction ; octets ajoutes a device_bytes. Allocation
// ordonnee sur le flux par defaut (cudaMallocAsync) dans le pool du peripherique, qui garde la memoire rendue : une
// passe chaude la reprend sans repasser par le pilote. Le budget compte les reservations vivantes, pas la memoire que
// le pool garde ensuite (comme operator new cote hote).
template <class T>
struct DeviceArray {
  T* data = nullptr;
  BudgetReservation reservation;
  DeviceArray() = default;
  DeviceArray(const DeviceArray&) = delete;
  DeviceArray& operator=(const DeviceArray&) = delete;
  ~DeviceArray() { if (data != nullptr) cudaFreeAsync(data, 0); }
  Outcome allocate(u64 count, MemoryBudget& budget, u64& bytes) noexcept {
    if (count == 0) return {};
    u64 size = 0;
    MHGP11_TRY(add_bytes<T>(size, count));
    MHGP11_TRY(reservation.reserve(size, budget));
    if (cudaMallocAsync(reinterpret_cast<void**>(&data), size, 0) != cudaSuccess) {
      data = nullptr;
      reservation.reset();
      return fail(Reason::memory_budget);
    }
    bytes += size;
    return {};
  }
};

u64 now_ns() {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch()).count());
}

bool ok(cudaError_t e) { return e == cudaSuccess; }

// Ouverture anticipee du contexte : un fil par processus, rejoint par l'executeur ou a la sortie du processus.
struct Prefetch {
  std::mutex mutex;
  std::thread thread;
  bool started = false;
  u64 ns = 0;
  ~Prefetch() {
    if (thread.joinable()) thread.join();
  }
};

Prefetch& prefetch_state() {
  static Prefetch state;
  return state;
}

// Le pool du peripherique garde toute memoire rendue (seuil de liberation maximal) : les passes suivantes la
// reprennent. Idempotent.
Outcome keep_pool_memory() noexcept {
  int device = 0;
  cudaMemPool_t pool;
  u64 keep = ~u64{0};
  if (!ok(cudaGetDevice(&device)) || !ok(cudaDeviceGetDefaultMemPool(&pool, device)) ||
      !ok(cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReleaseThreshold, &keep)))
    return fail(Reason::parameter_out_of_range);
  return {};
}

// Attend l'ouverture anticipee si elle a eu lieu ; rend sa duree (0 sinon).
u64 join_prefetch() noexcept {
  Prefetch& p = prefetch_state();
  std::lock_guard<std::mutex> lock(p.mutex);
  if (p.thread.joinable()) p.thread.join();
  return p.ns;
}

// Feuilles par m decroissant, stable (tri par comptage sur 1..kMaxSites) : chaque warp recoit des feuilles de meme
// taille. Seule l'affectation des fils change ; statuts, prefixes et ecritures restent indexes par feuille du lot.
Outcome size_order(const LeafBatchView& view, MemoryBudget& budget, Buffer<u32>& order) noexcept {
  MHGP11_TRY(budget.admit(view.count * sizeof(u32)));
  MHGP11_TRY(order.allocate(view.count, budget));
  u64 start[leaf_device::kMaxSites + 2] = {};
  for (u64 j = 0; j < view.count; ++j) {
    if (view.jobs[j].m > leaf_device::kMaxSites) return fail(Reason::catalogue_invariant);
    ++start[leaf_device::kMaxSites - view.jobs[j].m + 1];
  }
  for (u32 b = 1; b <= leaf_device::kMaxSites + 1; ++b) start[b] += start[b - 1];
  for (u64 j = 0; j < view.count; ++j) order[start[leaf_device::kMaxSites - view.jobs[j].m]++] = static_cast<u32>(j);
  return {};
}

// Prefixes exclusifs (CUB) des boules et incidences par feuille ; totaux = dernier debut + dernier compte (< 2^54).
Outcome scan(u64 count, const u64* balls, const u64* incidences, u64* record_begin, u64* population_begin,
             DeviceArray<unsigned char>& temp, MemoryBudget& budget, u64& device_bytes, u64& total_records,
             u64& total_population) noexcept {
  total_records = total_population = 0;
  if (count == 0) return {};
  size_t temp_bytes = 0;
  if (!ok(cub::DeviceScan::ExclusiveSum(nullptr, temp_bytes, balls, record_begin, count)))
    return fail(Reason::parameter_out_of_range);
  MHGP11_TRY(temp.allocate(temp_bytes, budget, device_bytes));
  if (!ok(cub::DeviceScan::ExclusiveSum(temp.data, temp_bytes, balls, record_begin, count)) ||
      !ok(cub::DeviceScan::ExclusiveSum(temp.data, temp_bytes, incidences, population_begin, count)))
    return fail(Reason::parameter_out_of_range);
  u64 last[4] = {0, 0, 0, 0};
  if (!ok(cudaMemcpy(&last[0], record_begin + count - 1, sizeof(u64), cudaMemcpyDeviceToHost)) ||
      !ok(cudaMemcpy(&last[1], balls + count - 1, sizeof(u64), cudaMemcpyDeviceToHost)) ||
      !ok(cudaMemcpy(&last[2], population_begin + count - 1, sizeof(u64), cudaMemcpyDeviceToHost)) ||
      !ok(cudaMemcpy(&last[3], incidences + count - 1, sizeof(u64), cudaMemcpyDeviceToHost)))
    return fail(Reason::parameter_out_of_range);
  total_records = last[0] + last[1];
  total_population = last[2] + last[3];
  return {};
}

// Liste des feuilles a ecrire : selection stable des feuilles qui emettent, dans l'ordre des fils (par taille).
Outcome select_emitting(u64 count, const u32* order, const u8* status, const u64* balls, const u8* stored,
                        DeviceArray<u32>& list,
                        DeviceArray<unsigned long long>& selected, DeviceArray<unsigned char>& temp,
                        MemoryBudget& budget, u64& device_bytes, u64& fill_count) noexcept {
  fill_count = 0;
  if (count == 0) return {};
  MHGP11_TRY(list.allocate(count, budget, device_bytes));
  MHGP11_TRY(selected.allocate(1, budget, device_bytes));
  const Emits emits{status, balls, stored};
  size_t temp_bytes = 0;
  if (!ok(cub::DeviceSelect::If(nullptr, temp_bytes, order, list.data, selected.data, count, emits)))
    return fail(Reason::parameter_out_of_range);
  MHGP11_TRY(temp.allocate(temp_bytes, budget, device_bytes));
  if (!ok(cub::DeviceSelect::If(temp.data, temp_bytes, order, list.data, selected.data, count, emits)))
    return fail(Reason::parameter_out_of_range);
  unsigned long long chosen = 0;
  if (!ok(cudaMemcpy(&chosen, selected.data, sizeof(chosen), cudaMemcpyDeviceToHost)))
    return fail(Reason::parameter_out_of_range);
  if (chosen > count) return fail(Reason::catalogue_invariant);
  fill_count = chosen;
  return {};
}

// Ecriture : copie des cases rangees au comptage vers leurs places, puis seconde passe des seules feuilles qui
// emettent et ont deborde de leur case.
struct WritePhase {
  const DeviceView& dv;
  const u8* status;
  const u8* stored;
  const u64* balls;
  const u64* incidences;
  const u64* record_begin;
  const u64* population_begin;
  const LeafRecord* scratch_records;
  const u8* scratch_population;
  LeafRecord* records;
  u8* population;
  u64 total_records, total_population;
  unsigned* errors;

  Outcome run(MemoryBudget& budget, u64& device_bytes, u64& fill_jobs) const noexcept {
    fill_jobs = 0;
    if (dv.count == 0) return {};
    const unsigned grid = static_cast<unsigned>((dv.count + kThreads - 1) / kThreads);
    copy_kernel<<<grid, kThreads>>>(dv.count, stored, balls, incidences, record_begin, population_begin,
                                    scratch_records, scratch_population, records, population);
    if (!ok(cudaGetLastError())) return fail(Reason::parameter_out_of_range);
    DeviceArray<u32> list;
    DeviceArray<unsigned long long> selected;
    DeviceArray<unsigned char> temp;
    MHGP11_TRY(select_emitting(dv.count, dv.order, status, balls, stored, list, selected, temp, budget, device_bytes,
                               fill_jobs));
    const unsigned fill_grid = static_cast<unsigned>((fill_jobs + kThreads - 1) / kThreads);
    if (fill_jobs != 0)
      fill_kernel<<<fill_grid, kThreads>>>(dv, list.data, fill_jobs, status, record_begin, population_begin, records,
                                           population, total_records, total_population, errors);
    if (!ok(cudaGetLastError()) || !ok(cudaDeviceSynchronize())) return fail(Reason::parameter_out_of_range);
    return {};
  }
};

// Pages neuves d'un tampon hote touchees en parallele avant la copie du retour : sinon chaque page fait faute pendant
// cudaMemcpy, sur un seul fil (77 Mo en 19 ms sur G4, 4 Go/s, contre 28 Go/s a l'envoi).
struct Touch {
  u8* base;
  u64 bytes;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& self = *static_cast<const Touch*>(context);
    for (u64 page = begin; page < end; ++page) self.base[page * 4096] = 0;
    return {};
  }
};

Outcome touch_pages(sched::Pool& pool, u8* base, u64 bytes) noexcept {
  if (bytes == 0) return {};
  Touch touch{base, bytes};
  return pool.parallel_for((bytes + 4095) / 4096, 64, &touch, Touch::body);
}

// Retour : emissions et population (tampons hote budgetes), statuts, compteurs du lot, erreurs de remplissage.
// Tableaux device rapatries.
struct Returned {
  const u8* status;
  const LeafRecord* records;
  const u8* population;
  const u64* record_begin;
  const u64* population_begin;
  const unsigned long long* totals;
  const unsigned* errors;
};

template <class T>
Outcome fetch(Buffer<T>& host, const T* device, u64 count, sched::Pool& pool, MemoryBudget& budget) noexcept {
  MHGP11_TRY(host.allocate(count, budget));
  MHGP11_TRY(touch_pages(pool, reinterpret_cast<u8*>(host.data()), count * sizeof(T)));
  if (count != 0 && !ok(cudaMemcpy(host.data(), device, count * sizeof(T), cudaMemcpyDeviceToHost)))
    return fail(Reason::parameter_out_of_range);
  return {};
}

Outcome download(u64 count, const Returned& d, u64 total_records, u64 total_population, sched::Pool& pool,
                 MemoryBudget& budget, LeafBatchResult& result) noexcept {
  u64 host_bytes = 0;
  MHGP11_TRY(add_bytes<LeafRecord>(host_bytes, total_records));
  MHGP11_TRY(add_bytes<u8>(host_bytes, total_population));
  MHGP11_TRY(add_bytes<u64>(host_bytes, 2 * count));
  MHGP11_TRY(budget.admit(host_bytes));
  MHGP11_TRY(fetch(result.records, d.records, total_records, pool, budget));
  MHGP11_TRY(fetch(result.population, d.population, total_population, pool, budget));
  MHGP11_TRY(fetch(result.record_begin, d.record_begin, count, pool, budget));
  MHGP11_TRY(fetch(result.population_begin, d.population_begin, count, pool, budget));
  unsigned long long counts[kCountFields] = {};
  unsigned fill_errors = 0;
  if ((count != 0 && !ok(cudaMemcpy(result.status.data(), d.status, count, cudaMemcpyDeviceToHost))) ||
      !ok(cudaMemcpy(counts, d.totals, sizeof(counts), cudaMemcpyDeviceToHost)) ||
      !ok(cudaMemcpy(&fill_errors, d.errors, sizeof(unsigned), cudaMemcpyDeviceToHost)))
    return fail(Reason::parameter_out_of_range);
  if (fill_errors != 0) return fail(Reason::catalogue_invariant);
  auto& c = result.counts;
  c.dominance_tests = counts[0]; c.prefixes = counts[1]; c.judged = counts[2]; c.census_tests = counts[3];
  c.emitted = counts[4]; c.incidences = counts[5]; c.q4_candidates = counts[6]; c.q4_levels = counts[7];
  c.region_pair_tests = counts[8]; c.region_pair_rejects = counts[9]; c.region_line_tests = counts[10];
  c.region_line_rejects = counts[11]; c.region_line_evaluations = counts[12]; c.region_line_cache_hits = counts[13];
  c.region_line_fallbacks = counts[14];
  return {};
}

}  // namespace

void prefetch_cuda_context() noexcept {
  Prefetch& p = prefetch_state();
  std::lock_guard<std::mutex> lock(p.mutex);
  if (p.started) return;
  p.started = true;
  try {
    p.thread = std::thread([&p] {
      const u64 begin = now_ns();
      static_cast<void>(cudaFree(nullptr));  // l'executeur refait l'appel et juge son statut
      p.ns = now_ns() - begin;
    });
  } catch (...) {
    // Sans fil disponible, le contexte s'ouvre au premier lot, comme sans ouverture anticipee.
  }
}

bool cuda_leaf_batch_available() noexcept {
  int devices = 0;
  return cudaGetDeviceCount(&devices) == cudaSuccess && devices > 0;
}

Outcome run_leaf_batch_cuda(const LeafBatchView& view, sched::Pool& pool, MemoryBudget& budget,
                            LeafBatchResult& result) noexcept {
  const u64 start = now_ns();
  auto& t = result.timings;
  // Hypothese de la borne des sommes (leaf_device::kCountBound) : au plus kMaxBatchJobs feuilles. L'ordre des fils
  // tient en u32 : au-dela de 2^32 feuilles, refus explicite (jamais atteint : 64 octets par feuille).
  if (view.count > leaf_device::kMaxBatchJobs) return fail(Reason::catalogue_counter_overflow);
  if (view.count > (u64{1} << 32)) return fail(Reason::index_overflow_u32);
  // Contexte : ouvert par l'ouverture anticipee (attendue ici) ou par cudaFree(0) au premier appel du processus ; une
  // passe chaude le trouve deja ouvert. Pile : aucune limite posee, le cadre statique des noyaux (sans recursion) est
  // dimensionne par le pilote au lancement.
  t.prefetch_ns = join_prefetch();
  if (!ok(cudaFree(nullptr))) return fail(Reason::parameter_out_of_range);
  MHGP11_TRY(keep_pool_memory());
  t.device_init_ns = now_ns() - start;
  Buffer<u32> order;
  MHGP11_TRY(size_order(view, budget, order));
  // Sorties hote d'abord (budget), puis tableaux device.
  u64 host_bytes = 0;
  MHGP11_TRY(add_bytes<u8>(host_bytes, view.count));
  MHGP11_TRY(budget.admit(host_bytes));
  MHGP11_TRY(result.status.allocate(view.count, budget));
  u64 device_bytes = 0;
  DeviceArray<u32> x, y, z, sites;
  DeviceArray<LeafJob> jobs;
  DeviceArray<u32> threads;
  DeviceArray<u8> status;
  DeviceArray<u64> balls, incidences;
  DeviceArray<unsigned long long> totals;
  DeviceArray<unsigned> errors;
  DeviceArray<LeafRecord> scratch_records;
  DeviceArray<u8> scratch_population;
  DeviceArray<u8> stored;
  MHGP11_TRY(x.allocate(view.cloud_sites, budget, device_bytes));
  MHGP11_TRY(y.allocate(view.cloud_sites, budget, device_bytes));
  MHGP11_TRY(z.allocate(view.cloud_sites, budget, device_bytes));
  MHGP11_TRY(sites.allocate(view.site_count, budget, device_bytes));
  MHGP11_TRY(jobs.allocate(view.count, budget, device_bytes));
  MHGP11_TRY(threads.allocate(view.count, budget, device_bytes));
  MHGP11_TRY(status.allocate(view.count, budget, device_bytes));
  MHGP11_TRY(balls.allocate(view.count, budget, device_bytes));
  MHGP11_TRY(incidences.allocate(view.count, budget, device_bytes));
  MHGP11_TRY(totals.allocate(kCountFields, budget, device_bytes));
  MHGP11_TRY(errors.allocate(1, budget, device_bytes));
  MHGP11_TRY(scratch_records.allocate(view.count * kScratchRecords, budget, device_bytes));  // count <= 2^32
  MHGP11_TRY(scratch_population.allocate(view.count * kScratchPopulation, budget, device_bytes));
  MHGP11_TRY(stored.allocate(view.count, budget, device_bytes));
  const u64 upload = now_ns();
  if (!ok(cudaMemcpy(x.data, view.x, view.cloud_sites * sizeof(u32), cudaMemcpyHostToDevice)) ||
      !ok(cudaMemcpy(y.data, view.y, view.cloud_sites * sizeof(u32), cudaMemcpyHostToDevice)) ||
      !ok(cudaMemcpy(z.data, view.z, view.cloud_sites * sizeof(u32), cudaMemcpyHostToDevice)) ||
      (view.site_count != 0 &&
       !ok(cudaMemcpy(sites.data, view.sites, view.site_count * sizeof(u32), cudaMemcpyHostToDevice))) ||
      (view.count != 0 && !ok(cudaMemcpy(jobs.data, view.jobs, view.count * sizeof(LeafJob), cudaMemcpyHostToDevice))) ||
      (view.count != 0 && !ok(cudaMemcpy(threads.data, order.data(), view.count * sizeof(u32), cudaMemcpyHostToDevice))) ||
      !ok(cudaMemset(totals.data, 0, kCountFields * sizeof(unsigned long long))) ||
      !ok(cudaMemset(errors.data, 0, sizeof(unsigned))))
    return fail(Reason::parameter_out_of_range);
  t.upload_ns = now_ns() - upload;
  const DeviceView dv{x.data, y.data, z.data, jobs.data, threads.data, view.count, sites.data, view.kmax, view.cache};
  const unsigned grid = static_cast<unsigned>((view.count + kThreads - 1) / kThreads);
  const u64 count = now_ns();
  if (view.count != 0)
    count_kernel<<<grid, kThreads>>>(dv, status.data, balls.data, incidences.data, totals.data, scratch_records.data,
                                     scratch_population.data, stored.data);
  if (!ok(cudaGetLastError()) || !ok(cudaDeviceSynchronize())) return fail(Reason::parameter_out_of_range);
  t.count_ns = now_ns() - count;
  // Prefixes exclusifs (CUB), puis totaux.
  const u64 scan_start = now_ns();
  DeviceArray<u64> record_begin, population_begin;
  MHGP11_TRY(record_begin.allocate(view.count, budget, device_bytes));
  MHGP11_TRY(population_begin.allocate(view.count, budget, device_bytes));
  DeviceArray<unsigned char> temp;
  u64 total_records = 0, total_population = 0;
  MHGP11_TRY(scan(view.count, balls.data, incidences.data, record_begin.data, population_begin.data, temp, budget,
                  device_bytes, total_records, total_population));
  t.scan_ns = now_ns() - scan_start;
  DeviceArray<LeafRecord> records;
  DeviceArray<u8> population;
  MHGP11_TRY(records.allocate(total_records, budget, device_bytes));
  MHGP11_TRY(population.allocate(total_population, budget, device_bytes));
  const u64 fill = now_ns();
  const WritePhase write{dv, status.data, stored.data, balls.data, incidences.data, record_begin.data,
                         population_begin.data, scratch_records.data, scratch_population.data, records.data,
                         population.data, total_records, total_population, errors.data};
  MHGP11_TRY(write.run(budget, device_bytes, t.fill_jobs));
  t.fill_ns = now_ns() - fill;
  // Retour des emissions, statuts et compteurs.
  const u64 download_start = now_ns();
  const Returned returned{status.data, records.data, population.data, record_begin.data, population_begin.data,
                          totals.data, errors.data};
  MHGP11_TRY(download(view.count, returned, total_records, total_population, pool, budget, result));
  t.download_ns = now_ns() - download_start;
  t.jobs = view.count; t.records = total_records; t.population = total_population;
  for (u64 j = 0; j < view.count; ++j)
    if (result.status[j] != leaf_device::kOk) ++t.unresolved;
  t.device_bytes = device_bytes;
  t.total_ns = now_ns() - start;
  return {};
}

}  // namespace mhgp11::catalogue_detail
