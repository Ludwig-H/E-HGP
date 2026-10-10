// Fin d'etage par tranches de cles (tranche T1-d, regime des scenes de plusieurs millions de sites, ARCHITECTURE.md,
// paragraphe 4.6) : parties de l'hote, ecrites une fois (slices.cpp). L'ordre du catalogue est celui de la fin
// d'etage (cle F3 du niveau, puis cle des positions de S*, chaines de voisins incertains retriees en exact). Entre deux
// voisins dont l'ordre F4 est CERTAIN, les niveaux exacts different strictement : aucune chaine ne traverse et aucun
// rang n'est partage. Une tranche est donc un intervalle de cles dont les bords sont des coupes certaines ; traitee
// seule, elle rend exactement la portion de l'ordre global qui lui revient, a ses bases pres (boule, incidence, rang).
//   - arene de l'hote : les lots (Chunk, decalages de population locaux au lot) de la voie CPU ou rapatries par la voie
//     appareil, et leurs bases globales ;
//   - plan : histogramme des cles par cases de bits, coupes seulement entre cases voisines d'ordre F4 certain, groupes
//     dont la memoire de travail tient dans `slice_bytes` ; un groupe trop lourd est redecoupe a une resolution plus
//     fine (16 bits de moins a chaque fois) ; un groupe incoupable plus lourd que `slice_bytes` (plateau de cles
//     voisines) est refuse (memory_budget) ;
//   - repartition stable des boules par tranche (indices globaux croissants) ; rassemblement d'une tranche
//     (populations rebasees) ; table S* -> boule sur l'hote (lignes par S*[0], triees par la cle de TableKeyKernel) ;
//     niveaux finaux materialises une fois leur nombre connu.
// Tout est deterministe et independant du nombre de fils ; tout tableau est un Buffer du budget de l'hote.
#pragma once

#include <span>
#include <vector>

#include "catalogue/finish_level.hpp"
#include "catalogue/internal.hpp"
#include "sched/sched.hpp"

namespace mhgp12::catalogue_detail::fin {

// Memoire de travail d'une tranche sur l'executeur (fin_etage par boule : deux tris par base, cles, drapeaux, niveaux
// et niveaux des rangs, boules ; arene de la tranche : enregistrement et population) : octets par boule et par
// incidence.
inline constexpr u64 kSliceBytesPerBall = 150 + 2 * sizeof(LevelWords) + sizeof(BallRecord);
inline constexpr u64 kSliceBytesPerIncidence = 2 * sizeof(SiteIdx);

// Dimensionnement (bornes de decision, jamais des garanties : chaque reservation reste controlee par son budget).
// Voie complete : tableaux de la fin d'etage sur toute l'arene, arene exclue (trois tris par base de n paires, cles,
// drapeaux, niveaux et niveaux des rangs, boules, populations ; sites : tri, rangs, debuts de ligne). Voie par
// tranches, cote hote pendant les tranches (l'arene est deja comptee) : boules, decalages et populations finales,
// repartition et mots des niveaux (au plus un rang par boule) ; les cles (8 octets par boule) sont rendues avant la
// premiere tranche. Apres les tranches, l'arene et la repartition rendues, viennent les niveaux (num::Level) et la
// table.
inline u64 full_finish_bytes(u64 balls, u64 incidences, u64 sites) noexcept {
  return balls * (188 + 2 * sizeof(LevelWords)) + incidences * sizeof(SiteIdx) + sites * 52;
}
inline u64 sliced_host_bytes(u64 balls, u64 incidences) noexcept {
  return balls * (sizeof(CatalogueBall) + 8 + 4 + sizeof(LevelWords)) + incidences * sizeof(SiteIdx);
}
// Pic de la voie complete au-dela de l'usage courant (voie CPU, l'arene rassemblee prenant la place des lots) : estime
// (full_finish_upper : tableaux de la fin d'etage, niveaux et table, un rang par boule) et minimum (full_finish_floor :
// les seuls tableaux neufs que reserve d'un coup finish_reserve : tris par base des positions, des cles et de la table,
// verdicts, niveaux, cles, drapeaux, debuts, decalages, cles des positions, boules ; populations ; sites). Sous le
// minimum, la voie complete ne peut pas tenir.
inline u64 full_finish_upper(u64 balls, u64 incidences, u64 sites) noexcept {
  return full_finish_bytes(balls, incidences, sites) + balls * (sizeof(num::Level) + sizeof(BallIdx));
}
inline u64 full_finish_floor(u64 balls, u64 incidences, u64 sites) noexcept {
  const u64 sorts = 2 * (sizeof(Key2) + sizeof(u32)) + 2 * (sizeof(u64) + sizeof(u32)) + 2 * (sizeof(Key2) + sizeof(u32));
  const u64 arrays = sizeof(u32) + sizeof(LevelWords) + 4 * sizeof(u64) + sizeof(Key2) + sizeof(CatalogueBall);
  return balls * (sorts + arrays) + incidences * sizeof(SiteIdx) +
         sites * (2 * (sizeof(Key2) + sizeof(u32)) + sizeof(u32) + sizeof(u64));
}

// Arene de l'hote : lots et bases globales (ball_at, incidence_at : chunks.size() + 1 valeurs).
struct ArenaView {
  std::span<const Chunk> chunks;
  const u64* ball_at = nullptr;
  const u64* incidence_at = nullptr;
  u64 balls = 0, incidences = 0;
};

// Bases globales des lots (sommes controlees) ; refus catalogue_invariant si une somme deborde.
[[nodiscard]] Outcome arena_bases(std::span<const Chunk> chunks, Buffer<u64>& ball_at, Buffer<u64>& incidence_at,
                                  ArenaView& out, MemoryBudget& budget) noexcept;

// Plan des tranches : borne superieure EXCLUSIVE de chaque tranche dans l'espace des cles F3 (bits ; la derniere
// vaut ~0 et contient aussi la cle ~0), boules et incidences de chaque tranche.
struct SlicePlan {
  Buffer<u64> upper, balls, incidences;
  u64 count = 0;
};

// Plan sur les cles F3 de toutes les boules (keys[i] : boule globale i) ; refus memory_budget si un plateau
// incoupable depasse slice_bytes.
[[nodiscard]] Outcome plan_slices(const ArenaView& arena, std::span<const u64> keys, u64 slice_bytes, SlicePlan& plan,
                                  MemoryBudget& budget, sched::Pool& pool) noexcept;

// Tranche de chaque boule (premiere tranche dont la borne depasse sa cle).
inline u64 slice_of(const SlicePlan& plan, u64 key) noexcept {
  u64 lo = 0, hi = plan.count - 1;
  while (lo < hi) {
    const u64 mid = lo + (hi - lo) / 2;
    if (key < plan.upper[mid]) hi = mid;
    else lo = mid + 1;
  }
  return lo;
}

// Boules de chaque tranche, indices globaux croissants : ids[start[j] .. start[j+1]) ; start a count + 1 valeurs.
[[nodiscard]] Outcome assign_slices(std::span<const u64> keys, const SlicePlan& plan, Buffer<u32>& ids,
                                    Buffer<u64>& start, MemoryBudget& budget, sched::Pool& pool) noexcept;

// Rassemblement d'une tranche : enregistrements (decalage de population rebase dans la tranche, lot 0) et populations
// contigues, dans l'ordre des ids.
[[nodiscard]] Outcome gather_slice(const ArenaView& arena, std::span<const u32> ids, Buffer<BallRecord>& records,
                                   Buffer<SiteIdx>& population, MemoryBudget& budget, sched::Pool& pool) noexcept;

// Table S* -> boule depuis les boules finales : offsets (sites + 1), values (une par boule), meme ordre que le tri par
// base de TableKeyKernel (S*[0], S*[1], S*[2], S*[3], case absente = nombre de sites).
[[nodiscard]] Outcome host_table(std::span<const CatalogueBall> balls, u32 sites, Buffer<u64>& offsets,
                                 Buffer<BallIdx>& values, MemoryBudget& budget, sched::Pool& pool) noexcept;

// Niveaux finaux : case 0 nulle, puis les mots des tranches dans l'ordre (rangs 1..L-1) ; chaque lot de mots est rendu
// des qu'il est materialise.
[[nodiscard]] Outcome materialize_levels(std::vector<Buffer<LevelWords>>& words, u64 distinct,
                                         Buffer<num::Level>& levels, MemoryBudget& budget, sched::Pool& pool) noexcept;

}  // namespace mhgp12::catalogue_detail::fin
