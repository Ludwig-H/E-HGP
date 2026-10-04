// Contexte CUDA de la voie GPU des feuilles (leaf_batch_cuda_context.cu), construit seulement avec MHGP11_ENABLE_CUDA.
#pragma once

#include "core/core.hpp"

namespace mhgp11::catalogue_detail {

// Le pool du peripherique garde toute memoire rendue (seuil de liberation maximal) et ses pics physiques sont remis
// au niveau courant : a appeler au debut de chaque lot, contexte ouvert.
[[nodiscard]] Outcome keep_pool_memory() noexcept;
// Pics UsedMemHigh et ReservedMemHigh du pool depuis keep_pool_memory : memoire physique du lot, distincte du compte
// logique (budget) et de device_bytes (cumul des allocations).
[[nodiscard]] Outcome pool_highs(u64& used_high, u64& reserved_high) noexcept;
// Attend l'ouverture anticipee si elle a eu lieu ; rend sa duree (0 sinon).
u64 join_prefetch() noexcept;

}  // namespace mhgp11::catalogue_detail
