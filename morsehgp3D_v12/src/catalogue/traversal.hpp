// Parcours des boites de centres en largeur, joue sur l'hote : pilote des niveaux et executeur hote (warp simule,
// warps repartis sur le Pool). Port explicite du pilote de microbancs/mes_m5_parcours/include/mhgp12/traversal/
// driver.hpp (MES-M5) ; l'executeur CUDA du microbanc reviendra avec la voie GPU de la tranche T1, sur le meme pilote.
//
// Un niveau : Select, Merge, Filter, Close, ScanA, ScanB, ScanC, lecture des totaux, Scatter, Emit, puis les feuilles du
// niveau sont remises au consommateur (feuilles en flux) avant le niveau suivant ; l'arene des feuilles est reutilisee
// d'un niveau a l'autre. Tous les tableaux du front (listes, taches, enfants, tuiles, arene des feuilles) sont des
// Buffer du budget de l'appel : un depassement rend memory_budget, sans rien publier. Refus : feuille plus large que
// max_leaf (wide_leaf), indices de taches ou d'enfants au-dela de 32 bits (index_overflow_u32), profondeur au-dela de
// 3B, impossible par le potentiel sum ceil(log2 largeur) (CST-0205 : catalogue_invariant).
#pragma once

#include "catalogue/catalogue.hpp"
#include "catalogue/traversal_kernels.hpp"
#include "catalogue/traversal_scan.hpp"
#include "sched/sched.hpp"

namespace mhgp12::catalogue_detail {

// Grand livre du parcours : noeuds (appels de prepare_node de la v11, vides compris), feuilles, tests G1, profondeur
// et feuille maximales. Sommes et maxima independants de l'ordre de visite.
struct TraversalLedger {
  u64 nodes = 0, leaves = 0, filter_tests = 0, max_depth = 0, max_leaf = 0;
};

// Diagnostics physiques du parcours (jamais dans une empreinte).
struct TraversalDiagnostics {
  u64 levels = 0, tasks = 0, candidates = 0, front_peak_bytes = 0;
};

// Feuilles d'un niveau : enregistrements et arene des sites (SiteIdx croissants par feuille), valides pendant l'appel.
class LeafConsumer {
 public:
  virtual Outcome consume(std::span<const bfs::Leaf> leaves, std::span<const u32> sites, u32 depth) noexcept = 0;

 protected:
  ~LeafConsumer() = default;
};

// Tableau du front : capacite croissante dans le budget, contenu non conserve a l'agrandissement.
template <class T>
struct FrontArray {
  Buffer<T> buffer;
  T* data() noexcept { return buffer.data(); }
  [[nodiscard]] Outcome ensure(u64 n, MemoryBudget& budget) noexcept {
    if (buffer.size() >= n) return {};
    const u64 grown = n < 2 * buffer.size() ? 2 * buffer.size() : n;
    buffer.reset();  // contenu non conserve : l'ancien tableau est rendu avant la nouvelle reservation
    return buffer.allocate(grown, budget);
  }
};

// Parcours complet d'un nuage (sites en ordre SiteIdx, n >= 1) ; ledger et diagnostics publies au succes seulement.
[[nodiscard]] Outcome traverse(const Cloud& cloud, const bfs::Params& params, MemoryBudget& budget, sched::Pool& pool,
                               LeafConsumer& consumer, TraversalLedger& ledger,
                               TraversalDiagnostics& diagnostics) noexcept;

}  // namespace mhgp12::catalogue_detail
