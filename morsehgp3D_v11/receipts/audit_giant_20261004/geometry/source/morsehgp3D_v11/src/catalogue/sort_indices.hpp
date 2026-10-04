// Tri exact d'une permutation ; les emissions et populations restent immuables.
#pragma once

#include <span>

#include "core/core.hpp"

namespace mhgp11::sched {
class Pool;
}

namespace mhgp11::catalogue_detail {

struct Emission;

// Deux Buffer<u32> et un Buffer<double> de N elements pendant le tri (16N octets), un seul rendu (4N).
// Les cles doubles (F3/F4) ne decident qu'hors de leur marge prouvee : meme permutation que le tri exact.
// Aucune allocation par tache. N<kNone ; vide admis sans allocation ni appel au Pool.
// L'ordre total (Level exact, support canonique, ordinal) fixe chaque case independamment de W.
// L'ordinal ne masque pas les doublons : l'assemblage doit toujours refuser les cles (Level,support) repetees.
// pool==nullptr execute le meme decoupage sur le fil appelant. Sinon tous les callbacks sont joints avant retour.
// comparisons, si present, n'est publie qu'au succes ; compte chaque appel au comparateur (cle puis exact).
// Borne : comparisons <= 4N*ceil(log2 N) < 2^39 (zero pour N<=1).
[[nodiscard]] Result<Buffer<u32>> sort_indices(std::span<const Emission> records, MemoryBudget& budget,
                                             sched::Pool* pool, u64* comparisons = nullptr) noexcept;

}  // namespace mhgp11::catalogue_detail
