// MorseHGP3D v3 — LE KERNEL FACE-OWNER : emission-tri-owner sur device.
//
// La partie massive du join face-owner — emettre les I_k signatures de
// k-faces, les trier, choisir l'owner minimal par signature et emettre les
// branches d'etoile dedupliquees — s'execute sur le GPU ; le rejeu DSU par
// lots (la petite queue sequentielle) reste sur l'hote.
//
// AUCUNE geometrie ni semantique de fold ici : le kernel reproduit EXACTEMENT
// le flux d'aretes du chemin CPU (`collect_edges`), qui le juge arete par
// arete. L'emission derange la c-ieme k-combinaison de chaque generateur en
// ordre lexicographique (binomiales precalculees, rang <= 32, k <= 6) — la
// meme enumeration canonique que l'hote ; l'owner est le minimum
// lexicographique (rang d'activation, generateur) par groupe de clefs.
#include <cuda_runtime.h>
#include <thrust/device_vector.h>
#include <thrust/execution_policy.h>
#include <thrust/functional.h>
#include <thrust/reduce.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/iterator/constant_iterator.h>
#include <thrust/unique.h>

#include <cstdio>

#include "prototype/faceowner_device.hpp"

namespace mhgp3v {
namespace device {

namespace {

struct Incidence {
  unsigned long long key_high;
  unsigned long long key_low;
  int activation;
  int generator;
};

// LE TRI PORTE L'OWNER (reception 23379d4) : ordonner par (signature, rang
// d'activation, generateur) place l'owner minimal EN TETE de chaque groupe —
// candidates, reduce_by_key et leurs sorties disparaissent du pic.
struct IncidenceLess {
  __host__ __device__ bool operator()(const Incidence& a, const Incidence& b) const {
    if (a.key_high != b.key_high) return a.key_high < b.key_high;
    if (a.key_low != b.key_low) return a.key_low < b.key_low;
    if (a.activation != b.activation) return a.activation < b.activation;
    return a.generator < b.generator;
  }
};

struct EdgeLess {
  __host__ __device__ bool operator()(const FaceOwnerDeviceEdge& a,
                                      const FaceOwnerDeviceEdge& b) const {
    if (a.activation_batch != b.activation_batch)
      return a.activation_batch < b.activation_batch;
    if (a.owner != b.owner) return a.owner < b.owner;
    return a.member < b.member;
  }
};

struct EdgeEqual {
  __host__ __device__ bool operator()(const FaceOwnerDeviceEdge& a,
                                      const FaceOwnerDeviceEdge& b) const {
    return a.activation_batch == b.activation_batch && a.owner == b.owner &&
           a.member == b.member;
  }
};

__constant__ unsigned long long kBinomial[33][7];

__global__ void emit_signatures(const int* members, const long long* member_offsets,
                                const long long* incidence_offsets, const int* ranks,
                                const int* activation_rank, int generator_count, int k,
                                long long total, Incidence* out) {
  const long long t = (long long)blockIdx.x * blockDim.x + threadIdx.x;
  if (t >= total) return;
  int low = 0, high = generator_count - 1;
  while (low < high) {
    const int middle = (low + high + 1) / 2;
    if (incidence_offsets[middle] <= t) low = middle;
    else high = middle - 1;
  }
  const int generator = low;
  long long remaining = t - incidence_offsets[generator];
  const int* gen_members = members + member_offsets[generator];
  const int rank = ranks[generator];
  unsigned long long key_high = 0, key_low = 0;
  int position = 0;
  for (int chosen = 0; chosen < k; ++chosen) {
    while (true) {
      const int rest_size = rank - position - 1;
      const int rest_need = k - chosen - 1;
      const unsigned long long with_this =
          rest_size >= rest_need ? kBinomial[rest_size][rest_need] : 0;
      if (remaining < (long long)with_this) break;
      remaining -= (long long)with_this;
      ++position;
    }
    const unsigned long long value = (unsigned long long)(unsigned)gen_members[position];
    key_high = (key_high << 21) | (key_low >> 43);
    key_low = (key_low << 21) | value;
    ++position;
  }
  out[t].key_high = key_high;
  out[t].key_low = key_low;
  out[t].activation = activation_rank[generator];
  out[t].generator = generator;
}

__global__ void mark_heads(const Incidence* incidences, long long total, long long* head) {
  const long long t = (long long)blockIdx.x * blockDim.x + threadIdx.x;
  if (t >= total) return;
  head[t] = (t == 0 || incidences[t].key_high != incidences[t - 1].key_high ||
             incidences[t].key_low != incidences[t - 1].key_low)
                ? 1
                : 0;
}

__global__ void emit_edges(const Incidence* incidences, const long long* group_of,
                           const long long* group_start, const int* batch_of,
                           long long total, FaceOwnerDeviceEdge* edges, int* keep) {
  const long long t = (long long)blockIdx.x * blockDim.x + threadIdx.x;
  if (t >= total) return;
  const int member = incidences[t].generator;
  const int owner = incidences[group_start[group_of[t]]].generator;
  if (member == owner) {
    keep[t] = 0;
    return;
  }
  keep[t] = 1;
  edges[t].activation_batch = batch_of[member];
  edges[t].owner = owner;
  edges[t].member = member;
}

struct NonZero {
  __host__ __device__ bool operator()(int flag) const { return flag != 0; }
};

struct NonZero64 {
  __host__ __device__ bool operator()(long long flag) const { return flag != 0; }
};

bool record(const char* what, cudaError_t status, char* error, int capacity) {
  if (status == cudaSuccess) return true;
  snprintf(error, (size_t)capacity, "%s : %s", what, cudaGetErrorString(status));
  return false;
}

double elapsed(cudaEvent_t begin, cudaEvent_t end) {
  float milliseconds = 0.0f;
  cudaEventElapsedTime(&milliseconds, begin, end);
  return (double)milliseconds;
}

}  // namespace

bool run_faceowner_order_on_gpu(const std::vector<int>& members_csr,
                                const std::vector<long long>& member_offsets,
                                const std::vector<long long>& incidence_offsets,
                                const std::vector<int>& ranks,
                                const std::vector<int>& activation_rank,
                                const std::vector<int>& batch_of, int k,
                                FaceOwnerDeviceResult* result, char* error,
                                int error_capacity) {
  *result = FaceOwnerDeviceResult{};
  // VALIDATION HOSTILE DE L'ENTREE (reception 23379d4, porte 2) — avant toute
  // lecture de back() et meme a masse nulle : tailles coherentes, offsets
  // monotones, rangs bornes, identifiants denses dans l'empaquetage.
  const int generator_count = (int)ranks.size();
  if (k < 1 || k > 6) {
    snprintf(error, (size_t)error_capacity, "ordre hors contrat du kernel");
    return false;
  }
  if (generator_count <= 0 ||
      member_offsets.size() != (std::size_t)generator_count ||
      incidence_offsets.size() != (std::size_t)generator_count + 1 ||
      activation_rank.size() != (std::size_t)generator_count ||
      batch_of.size() != (std::size_t)generator_count) {
    snprintf(error, (size_t)error_capacity, "tailles d'entree incoherentes");
    return false;
  }
  for (int g = 0; g < generator_count; ++g) {
    const long long begin = member_offsets[(std::size_t)g];
    const long long end = g + 1 < generator_count ? member_offsets[(std::size_t)g + 1]
                                                  : (long long)members_csr.size();
    if (begin < 0 || end < begin || end > (long long)members_csr.size() ||
        end - begin != (long long)ranks[(std::size_t)g] || ranks[(std::size_t)g] < 0 ||
        ranks[(std::size_t)g] > 32 ||
        incidence_offsets[(std::size_t)g + 1] < incidence_offsets[(std::size_t)g] ||
        activation_rank[(std::size_t)g] < 0 || batch_of[(std::size_t)g] < 0) {
      snprintf(error, (size_t)error_capacity, "offsets, rangs ou lots hors contrat");
      return false;
    }
  }
  for (int x : members_csr)
    if (x < 0 || x >= (1 << 21)) {
      snprintf(error, (size_t)error_capacity, "identifiant dense hors empaquetage");
      return false;
    }
  const long long total = incidence_offsets.back();
  result->incidences = total;
  if (total == 0) return true;

  // Binomiales C(0..32, 0..6) — la meme table que le preflight hote.
  unsigned long long binomial[33][7];
  for (int n = 0; n <= 32; ++n)
    for (int r = 0; r <= 6; ++r) {
      if (r == 0) { binomial[n][r] = 1; continue; }
      if (n == 0) { binomial[n][r] = 0; continue; }
      binomial[n][r] = binomial[n - 1][r - 1] + binomial[n - 1][r];
    }
  if (!record("cudaMemcpyToSymbol binomiales",
              cudaMemcpyToSymbol(kBinomial, binomial, sizeof(binomial)), error,
              error_capacity))
    return false;

  cudaEvent_t begin = nullptr, after_emit = nullptr, after_sort = nullptr, end = nullptr;
  cudaEventCreate(&begin);
  cudaEventCreate(&after_emit);
  cudaEventCreate(&after_sort);
  cudaEventCreate(&end);

  try {
    thrust::device_vector<int> device_members(members_csr.begin(), members_csr.end());
    thrust::device_vector<long long> device_member_offsets(member_offsets.begin(),
                                                           member_offsets.end());
    thrust::device_vector<long long> device_incidence_offsets(incidence_offsets.begin(),
                                                              incidence_offsets.end());
    thrust::device_vector<int> device_ranks(ranks.begin(), ranks.end());
    thrust::device_vector<int> device_activation(activation_rank.begin(),
                                                 activation_rank.end());
    thrust::device_vector<int> device_batch(batch_of.begin(), batch_of.end());
    thrust::device_vector<Incidence> incidences((std::size_t)total);
    result->device_bytes =
        (long long)(members_csr.size() * 4 + member_offsets.size() * 8 +
                    incidence_offsets.size() * 8 + ranks.size() * 4 * 3 +
                    (std::size_t)total * (sizeof(Incidence) + 8 + 8 +
                                          sizeof(FaceOwnerDeviceEdge) + 4));

    const int block = 256;
    const long long grid = (total + block - 1) / block;
    cudaEventRecord(begin);
    emit_signatures<<<(unsigned)grid, block>>>(
        thrust::raw_pointer_cast(device_members.data()),
        thrust::raw_pointer_cast(device_member_offsets.data()),
        thrust::raw_pointer_cast(device_incidence_offsets.data()),
        thrust::raw_pointer_cast(device_ranks.data()),
        thrust::raw_pointer_cast(device_activation.data()), generator_count, k, total,
        thrust::raw_pointer_cast(incidences.data()));
    cudaEventRecord(after_emit);
    if (!record("emission", cudaGetLastError(), error, error_capacity)) return false;

    thrust::sort(thrust::device, incidences.begin(), incidences.end(), IncidenceLess{});
    cudaEventRecord(after_sort);

    // Groupes : tetes, identifiants par balayage, owner = min lexicographique
    // (rang d'activation, generateur) par reduce_by_key.
    thrust::device_vector<long long> head((std::size_t)total);
    mark_heads<<<(unsigned)grid, block>>>(thrust::raw_pointer_cast(incidences.data()), total,
                                          thrust::raw_pointer_cast(head.data()));
    if (!record("tetes", cudaGetLastError(), error, error_capacity)) return false;
    thrust::device_vector<long long> group_of((std::size_t)total);
    thrust::inclusive_scan(thrust::device, head.begin(), head.end(), group_of.begin());
    const long long group_count = group_of.back();
    result->signatures = group_count;
    thrust::transform(thrust::device, group_of.begin(), group_of.end(),
                      thrust::make_constant_iterator((long long)1), group_of.begin(),
                      thrust::minus<long long>());
    // Les DEBUTS de groupe : l'owner est la tete (tri par activation) — les
    // indices des tetes, compactes par copy_if sur le drapeau.
    thrust::device_vector<long long> group_start((std::size_t)group_count);
    thrust::copy_if(thrust::device,
                    thrust::make_counting_iterator<long long>(0),
                    thrust::make_counting_iterator<long long>(total), head.begin(),
                    group_start.begin(), NonZero64{});

    thrust::device_vector<FaceOwnerDeviceEdge> raw_edges((std::size_t)total);
    thrust::device_vector<int> keep((std::size_t)total);
    emit_edges<<<(unsigned)grid, block>>>(
        thrust::raw_pointer_cast(incidences.data()),
        thrust::raw_pointer_cast(group_of.data()),
        thrust::raw_pointer_cast(group_start.data()),
        thrust::raw_pointer_cast(device_batch.data()), total,
        thrust::raw_pointer_cast(raw_edges.data()), thrust::raw_pointer_cast(keep.data()));
    if (!record("aretes", cudaGetLastError(), error, error_capacity)) return false;

    thrust::device_vector<FaceOwnerDeviceEdge> kept((std::size_t)total);
    const auto kept_end = thrust::copy_if(thrust::device, raw_edges.begin(),
                                          raw_edges.end(), keep.begin(), kept.begin(),
                                          NonZero{});
    const long long kept_count = (long long)(kept_end - kept.begin());
    thrust::sort(thrust::device, kept.begin(), kept.begin() + kept_count, EdgeLess{});
    const auto unique_end = thrust::unique(thrust::device, kept.begin(),
                                           kept.begin() + kept_count, EdgeEqual{});
    const long long edge_count = (long long)(unique_end - kept.begin());
    cudaEventRecord(end);
    cudaEventSynchronize(end);

    result->edges.resize((std::size_t)edge_count);
    thrust::copy(kept.begin(), kept.begin() + edge_count, result->edges.begin());
    result->emit_milliseconds = elapsed(begin, after_emit);
    result->sort_milliseconds = elapsed(after_emit, after_sort);
    result->reduce_milliseconds = elapsed(after_sort, end);
  } catch (const std::exception& failure) {
    snprintf(error, (size_t)error_capacity, "thrust : %s", failure.what());
    return false;
  }
  cudaEventDestroy(begin);
  cudaEventDestroy(after_emit);
  cudaEventDestroy(after_sort);
  cudaEventDestroy(end);
  return record("fin", cudaGetLastError(), error, error_capacity);
}

bool query_device_memory(long long* free_bytes, long long* total_bytes, char* error,
                         int error_capacity) {
  std::size_t free_amount = 0, total_amount = 0;
  if (!record("cudaMemGetInfo", cudaMemGetInfo(&free_amount, &total_amount), error,
              error_capacity))
    return false;
  *free_bytes = (long long)free_amount;
  *total_bytes = (long long)total_amount;
  return true;
}

}  // namespace device
}  // namespace mhgp3v
