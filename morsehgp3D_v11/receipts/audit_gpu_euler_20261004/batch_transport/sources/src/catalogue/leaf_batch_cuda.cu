// Executeur CUDA du lot de feuilles (voie GPU de la passe unique) : meme leaf_device.hpp que l'executeur hote, un fil
// par feuille ; passe de comptage (statut, boules, incidences, compteurs des feuilles resolues reduits par bloc),
// prefixes exclusifs (CUB), passe d'ecriture aux places fixees, retour des emissions dans l'ordre du lot.
// Exactitude : aucun flottant ; les chemins non certifies rendent la feuille non resolue (refaite sur l'hote).
#include "catalogue/leaf_batch.hpp"

#include <cuda_runtime.h>
#include <cub/device/device_scan.cuh>

#include <chrono>

namespace mhgp11::catalogue_detail {
namespace {

constexpr int kThreads = 128;
constexpr int kCountFields = 15;

struct DeviceView {
  const u32* x;
  const u32* y;
  const u32* z;
  const LeafJob* jobs;
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

__global__ void count_kernel(DeviceView v, u8* status, u64* balls, u64* incidences, unsigned long long* totals) {
  __shared__ unsigned long long block_totals[kCountFields];
  if (threadIdx.x < kCountFields) block_totals[threadIdx.x] = 0;
  __syncthreads();
  const u64 j = u64(blockIdx.x) * blockDim.x + threadIdx.x;
  if (j < v.count) {
    const leaf_device::Input in = device_input(v, j);
    leaf_device::Counts c;
    leaf_device::CountSink sink;
    const u32 s = leaf_device::run_leaf(in, c, sink);
    status[j] = static_cast<u8>(s);
    balls[j] = s == leaf_device::kOk ? sink.balls : 0;
    incidences[j] = s == leaf_device::kOk ? sink.incidences : 0;
    if (s == leaf_device::kOk) {
      unsigned long long local[kCountFields];
      counts_to_array(c, local);
      for (int f = 0; f < kCountFields; ++f)
        if (local[f] != 0) atomicAdd(&block_totals[f], local[f]);
    }
  }
  __syncthreads();
  if (threadIdx.x < kCountFields && block_totals[threadIdx.x] != 0)
    atomicAdd(&totals[threadIdx.x], block_totals[threadIdx.x]);
}

__global__ void fill_kernel(DeviceView v, const u8* status, const u64* record_begin, const u64* population_begin,
                            LeafRecord* records, u32* population, u64 total_records, u64 total_population,
                            unsigned* errors) {
  const u64 j = u64(blockIdx.x) * blockDim.x + threadIdx.x;
  if (j >= v.count || status[j] != leaf_device::kOk) return;
  const leaf_device::Input in = device_input(v, j);
  leaf_device::Counts c;
  FillSink sink{records, population, record_begin[j], population_begin[j]};
  const u64 record_end = j + 1 < v.count ? record_begin[j + 1] : total_records;
  const u64 population_end = j + 1 < v.count ? population_begin[j + 1] : total_population;
  if (leaf_device::run_leaf(in, c, sink) != leaf_device::kOk || sink.record_at != record_end ||
      sink.population_at != population_end)
    atomicAdd(errors, 1u);
}

// Proprietaire RAII d'une allocation device ; les octets sont comptes dans device_bytes.
template <class T>
struct DeviceArray {
  T* data = nullptr;
  ~DeviceArray() { if (data != nullptr) cudaFree(data); }
  bool allocate(u64 count, u64& bytes) {
    if (count == 0) return true;
    bytes += count * sizeof(T);
    return cudaMalloc(&data, count * sizeof(T)) == cudaSuccess;
  }
};

u64 now_ns() {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch()).count());
}

bool ok(cudaError_t e) { return e == cudaSuccess; }

}  // namespace

bool cuda_leaf_batch_available() noexcept {
  int devices = 0;
  return cudaGetDeviceCount(&devices) == cudaSuccess && devices > 0;
}

Outcome run_leaf_batch_cuda(const LeafBatchView& view, MemoryBudget& budget, LeafBatchResult& result) noexcept {
  const u64 start = now_ns();
  auto& t = result.timings;
  // Contexte : cudaFree(0) l'ouvre au premier appel du processus ; une passe chaude le trouve deja ouvert.
  if (!ok(cudaFree(nullptr))) return fail(Reason::parameter_out_of_range);
  if (!ok(cudaDeviceSetLimit(cudaLimitStackSize, 16384))) return fail(Reason::parameter_out_of_range);
  t.device_init_ns = now_ns() - start;
  // Sorties hote d'abord (budget), puis tableaux device.
  u64 host_bytes = 0;
  MHGP11_TRY(add_bytes<u8>(host_bytes, view.count));
  MHGP11_TRY(budget.admit(host_bytes));
  MHGP11_TRY(result.status.allocate(view.count, budget));
  u64 device_bytes = 0;
  DeviceArray<u32> x, y, z, sites;
  DeviceArray<LeafJob> jobs;
  DeviceArray<u8> status;
  DeviceArray<u64> balls, incidences;
  DeviceArray<unsigned long long> totals;
  DeviceArray<unsigned> errors;
  if (!x.allocate(view.cloud_sites, device_bytes) || !y.allocate(view.cloud_sites, device_bytes) ||
      !z.allocate(view.cloud_sites, device_bytes) || !sites.allocate(view.site_count, device_bytes) ||
      !jobs.allocate(view.count, device_bytes) || !status.allocate(view.count, device_bytes) ||
      !balls.allocate(view.count, device_bytes) || !incidences.allocate(view.count, device_bytes) ||
      !totals.allocate(kCountFields, device_bytes) || !errors.allocate(1, device_bytes))
    return fail(Reason::memory_budget);
  const u64 upload = now_ns();
  if (!ok(cudaMemcpy(x.data, view.x, view.cloud_sites * sizeof(u32), cudaMemcpyHostToDevice)) ||
      !ok(cudaMemcpy(y.data, view.y, view.cloud_sites * sizeof(u32), cudaMemcpyHostToDevice)) ||
      !ok(cudaMemcpy(z.data, view.z, view.cloud_sites * sizeof(u32), cudaMemcpyHostToDevice)) ||
      (view.site_count != 0 &&
       !ok(cudaMemcpy(sites.data, view.sites, view.site_count * sizeof(u32), cudaMemcpyHostToDevice))) ||
      (view.count != 0 && !ok(cudaMemcpy(jobs.data, view.jobs, view.count * sizeof(LeafJob), cudaMemcpyHostToDevice))) ||
      !ok(cudaMemset(totals.data, 0, kCountFields * sizeof(unsigned long long))) ||
      !ok(cudaMemset(errors.data, 0, sizeof(unsigned))))
    return fail(Reason::parameter_out_of_range);
  t.upload_ns = now_ns() - upload;
  const DeviceView dv{x.data, y.data, z.data, jobs.data, view.count, sites.data, view.kmax, view.cache};
  const unsigned grid = static_cast<unsigned>((view.count + kThreads - 1) / kThreads);
  const u64 count = now_ns();
  if (view.count != 0) count_kernel<<<grid, kThreads>>>(dv, status.data, balls.data, incidences.data, totals.data);
  if (!ok(cudaGetLastError()) || !ok(cudaDeviceSynchronize())) return fail(Reason::parameter_out_of_range);
  t.count_ns = now_ns() - count;
  // Prefixes exclusifs (CUB) sur boules et incidences ; totaux = dernier debut + dernier compte.
  const u64 scan = now_ns();
  DeviceArray<u64> record_begin, population_begin;
  if (!record_begin.allocate(view.count, device_bytes) || !population_begin.allocate(view.count, device_bytes))
    return fail(Reason::memory_budget);
  size_t temp_bytes = 0;
  if (view.count != 0 &&
      !ok(cub::DeviceScan::ExclusiveSum(nullptr, temp_bytes, balls.data, record_begin.data, view.count)))
    return fail(Reason::parameter_out_of_range);
  DeviceArray<unsigned char> temp;
  if (!temp.allocate(temp_bytes, device_bytes)) return fail(Reason::memory_budget);
  if (view.count != 0 &&
      (!ok(cub::DeviceScan::ExclusiveSum(temp.data, temp_bytes, balls.data, record_begin.data, view.count)) ||
       !ok(cub::DeviceScan::ExclusiveSum(temp.data, temp_bytes, incidences.data, population_begin.data, view.count))))
    return fail(Reason::parameter_out_of_range);
  u64 last[4] = {0, 0, 0, 0};
  if (view.count != 0 &&
      (!ok(cudaMemcpy(&last[0], record_begin.data + view.count - 1, sizeof(u64), cudaMemcpyDeviceToHost)) ||
       !ok(cudaMemcpy(&last[1], balls.data + view.count - 1, sizeof(u64), cudaMemcpyDeviceToHost)) ||
       !ok(cudaMemcpy(&last[2], population_begin.data + view.count - 1, sizeof(u64), cudaMemcpyDeviceToHost)) ||
       !ok(cudaMemcpy(&last[3], incidences.data + view.count - 1, sizeof(u64), cudaMemcpyDeviceToHost))))
    return fail(Reason::parameter_out_of_range);
  const u64 total_records = last[0] + last[1], total_population = last[2] + last[3];
  t.scan_ns = now_ns() - scan;
  DeviceArray<LeafRecord> records;
  DeviceArray<u32> population;
  if (!records.allocate(total_records, device_bytes) || !population.allocate(total_population, device_bytes))
    return fail(Reason::memory_budget);
  const u64 fill = now_ns();
  if (view.count != 0)
    fill_kernel<<<grid, kThreads>>>(dv, status.data, record_begin.data, population_begin.data, records.data,
                                    population.data, total_records, total_population, errors.data);
  if (!ok(cudaGetLastError()) || !ok(cudaDeviceSynchronize())) return fail(Reason::parameter_out_of_range);
  t.fill_ns = now_ns() - fill;
  // Retour : emissions, statuts, compteurs, erreurs de remplissage.
  host_bytes = 0;
  MHGP11_TRY(add_bytes<LeafRecord>(host_bytes, total_records));
  MHGP11_TRY(add_bytes<u32>(host_bytes, total_population));
  MHGP11_TRY(budget.admit(host_bytes));
  MHGP11_TRY(result.records.allocate(total_records, budget));
  MHGP11_TRY(result.population.allocate(total_population, budget));
  const u64 download = now_ns();
  unsigned long long counts[kCountFields] = {};
  unsigned fill_errors = 0;
  if ((total_records != 0 && !ok(cudaMemcpy(result.records.data(), records.data, total_records * sizeof(LeafRecord),
                                             cudaMemcpyDeviceToHost))) ||
      (total_population != 0 && !ok(cudaMemcpy(result.population.data(), population.data,
                                                 total_population * sizeof(u32), cudaMemcpyDeviceToHost))) ||
      (view.count != 0 && !ok(cudaMemcpy(result.status.data(), status.data, view.count, cudaMemcpyDeviceToHost))) ||
      !ok(cudaMemcpy(counts, totals.data, sizeof(counts), cudaMemcpyDeviceToHost)) ||
      !ok(cudaMemcpy(&fill_errors, errors.data, sizeof(unsigned), cudaMemcpyDeviceToHost)))
    return fail(Reason::parameter_out_of_range);
  t.download_ns = now_ns() - download;
  if (fill_errors != 0) return fail(Reason::catalogue_invariant);
  auto& c = result.counts;
  c.dominance_tests = counts[0]; c.prefixes = counts[1]; c.judged = counts[2]; c.census_tests = counts[3];
  c.emitted = counts[4]; c.incidences = counts[5]; c.q4_candidates = counts[6]; c.q4_levels = counts[7];
  c.region_pair_tests = counts[8]; c.region_pair_rejects = counts[9]; c.region_line_tests = counts[10];
  c.region_line_rejects = counts[11]; c.region_line_evaluations = counts[12]; c.region_line_cache_hits = counts[13];
  c.region_line_fallbacks = counts[14];
  t.jobs = view.count; t.records = total_records; t.population = total_population;
  for (u64 j = 0; j < view.count; ++j)
    if (result.status[j] != leaf_device::kOk) ++t.unresolved;
  t.device_bytes = device_bytes;
  t.total_ns = now_ns() - start;
  return {};
}

}  // namespace mhgp11::catalogue_detail
