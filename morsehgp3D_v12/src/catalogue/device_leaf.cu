// Noyaux de feuille J3 de la voie appareil (tranche T1-b) : Count et Replay, un warp par feuille (kLeafLanes voies),
// memoire partagee LeafShared<32> par feuille, borne de registres adoptee par MES-M2 (j3_r168 : __launch_bounds__(128,
// 3), au plus 168 registres ; microbancs/mes_m2_feuille/cuda/leaf_bench.cu, port explicite du lancement). Unite de
// traduction a part : le mutant << un fil par feuille >> du contrat (bench/g4_catalogue_mutants.json) la compile avec
// le warp joue en serie (MHGP12_SIMT_SERIAL, simt.hpp), une feuille par fil, sans toucher les autres noyaux.
#include <cuda_runtime.h>

#include "catalogue/device_leaves.hpp"

namespace mhgp12::catalogue_detail::dev {
namespace {

inline constexpr u32 kLeafThreads = 128;
inline constexpr int kLeafMinBlocks = 3;  // j3_r168 (MES-M2)
inline constexpr u32 kLeafLanes = 32;     // voies par feuille : un warp

template <class K>
__global__ void __launch_bounds__(kLeafThreads, kLeafMinBlocks) leaf_kernel(K k, u64 n) {
  __shared__ LeafShared<32> shared[kLeafThreads / kLeafLanes];
  const u64 leaf = (u64(blockIdx.x) * kLeafThreads + threadIdx.x) / kLeafLanes;
  if (leaf >= n) return;  // uniforme sur les voies d'une feuille
  k(leaf, shared[threadIdx.x / kLeafLanes]);
}

template <class K>
int launch(const K& kernel, u64 leaves, void* stream) noexcept {
  if (leaves == 0) return cudaSuccess;
  const u64 blocks = (leaves * kLeafLanes + kLeafThreads - 1) / kLeafThreads;
  if (blocks > 0x7FFFFFFFull) return cudaErrorInvalidConfiguration;
  leaf_kernel<K><<<static_cast<unsigned>(blocks), kLeafThreads, 0, static_cast<cudaStream_t>(stream)>>>(kernel,
                                                                                                       leaves);
  return cudaGetLastError();
}

}  // namespace

int launch_count_kernel(const CountKernel& kernel, u64 leaves, void* stream) noexcept {
  return launch(kernel, leaves, stream);
}
int launch_replay_kernel(const ReplayKernel& kernel, u64 leaves, void* stream) noexcept {
  return launch(kernel, leaves, stream);
}

}  // namespace mhgp12::catalogue_detail::dev
