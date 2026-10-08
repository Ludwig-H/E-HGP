// Pilote de la voie appareil du catalogue (tranche T1-b), ecrit une fois pour l'executeur CUDA (produit) et
// l'executeur Pool (portes de l'hote : meme code, warps simules). Sequence :
//   1. nuage copie sur l'executeur ; parcours en largeur (traversal_driver.hpp) dont le crochet garde les feuilles
//      d'un niveau a l'autre jusqu'a un lot de kBatchLeaves feuilles (une trame de 60 000 sites a K5 tient en un lot) ;
//   2. par lot (device_leaves.hpp) : Classify, ordre par taille, Count (J3 etroite), decalages exacts par sommes
//      prefixes, totaux et premiere faute ; feuilles non resolues (m > 32 ou etendue > 16) compactees, rapatriees et
//      rejouees en exact par LeafStage (meme source, politique exacte ou warp virtuel) ; ADMISSION du lot (arene des
//      boules agrandie dans le budget) seulement apres ces comptes ; Copy et Replay ; boules rejouees par l'hote copiees apres
//      celles de l'appareil ;
//   3. fin d'etage partagee (finish_driver.hpp) sur l'arene, puis publication (Assembly::adopt).
// Premiere faute : la plus petite profondeur, puis la fusion des refus (coquille avant invariant), comme la voie CPU
// qui s'arrete au premier lot fautif. Un refus ne publie rien ; les tableaux de l'etat restent reutilisables.
#pragma once

#include "catalogue/device_leaves.hpp"
#include "catalogue/finish_driver.hpp"
#include "catalogue/finish_slices.hpp"
#include "catalogue/traversal_driver.hpp"

namespace mhgp12::catalogue_detail::dev {

template <class B>
struct LeafArrays {
  typename B::template Array<LeafClass> cls;
  typename B::template Array<u64> balls, incidences, ball_at, population_at, counts;
  typename B::template Array<u64> unresolved, unresolved_at, unresolved_sites, unresolved_site_at;
  typename B::template Array<u32> status;
  typename B::template Array<Emission<32>> cases;
  typename B::template Array<BatchStats> tiles, total;
  typename B::template Array<bfs::Leaf> held_leaves;
  typename B::template Array<u32> held_sites;
  fin::RadixArrays<B, u64> order;
  fin::ScanArrays<B> scan;
};

// Etat de la voie appareil : garde d'un appel a l'autre par l'executeur CUDA (regime resident).
template <class B>
struct DeviceState {
  typename B::template Array<u32> x, y, z, fault;
  Front<B> front;
  LeafArrays<B> leaves;
  typename B::template Array<BallRecord> records;
  typename B::template Array<SiteIdx> population;
  fin::FinishArrays<B> finish;
  fin::SliceArrays<B> slices;  // arene d'une tranche (fin d'etage par tranches, T1-d)
};

// Comptes et diagnostics accumules sur les lots. Reprises sur l'hote (feuilles non resolues) par cause : plus de 32
// sites (warp virtuel), etendue au-dela de 16 (politique exacte) ; une feuille peut avoir les deux causes. Reecritures
// (feuille de plus de kCase emissions rejouee par la meme source, comme leaves_rewritten de la voie CPU) : sur
// l'appareil (Replay) et dans le rejeu de l'hote (LeafStage).
struct DeviceTotals {
  LeafCounts counts{};
  u64 tiers[5] = {0, 0, 0, 0, 0};
  u64 max_span = 0, batches = 0, replayed_leaves = 0, replayed_balls = 0, replayed_wide = 0, replayed_span = 0;
  u64 rewritten_device = 0, rewritten_host = 0;
  u64 leaves_ns = 0, emission_ns = 0;  // nets des transferts : feuilles (classement, J3, decalages) ; emission
};

// Profondeur et refus de la premiere faute d'un ensemble de feuilles.
struct Failure {
  u64 depth = kNoFailure;
  Outcome outcome{};
};

inline Failure earliest(const Failure& a, const Failure& b) noexcept {
  if (a.depth != b.depth) return a.depth < b.depth ? a : b;
  return Failure{a.depth, merge(a.outcome, b.outcome)};
}

// Rejeu exact sur l'hote des feuilles non resolues d'un lot (dans l'ordre, groupees par profondeur) : LeafStage, la
// meme source que la voie CPU ; premier groupe fautif retenu. Les enregistrements rendus sont en SiteIdx globaux,
// populations relatives a leur lot (Chunk).
inline Failure replay_unresolved(LeafStage& stage, std::span<const bfs::Leaf> leaves,
                                 std::span<const u32> sites) noexcept {
  for (u64 first = 0; first < leaves.size();) {
    u64 last = first + 1;
    while (last < leaves.size() && leaves[last].depth == leaves[first].depth) ++last;
    const Outcome outcome = stage.consume(leaves.subspan(first, last - first), sites, leaves[first].depth);
    if (!outcome.ok()) return Failure{leaves[first].depth, outcome};
    first = last;
  }
  return Failure{};
}

template <class B>
Outcome ensure_batch(B& b, LeafArrays<B>& a, u64 n) noexcept {
  MHGP12_TRY(b.ensure(a.cls, n));
  for (auto* array : {&a.balls, &a.incidences, &a.ball_at, &a.population_at, &a.unresolved, &a.unresolved_at,
                      &a.unresolved_sites, &a.unresolved_site_at})
    MHGP12_TRY(b.ensure(*array, n));
  MHGP12_TRY(b.ensure(a.counts, n * kLeafCounters));
  MHGP12_TRY(b.ensure(a.status, n));
  MHGP12_TRY(b.ensure(a.cases, n * kCase));
  MHGP12_TRY(b.ensure(a.tiles, fin::tiles_of(n)));
  MHGP12_TRY(b.ensure(a.total, 1));
  return fin::radix_reserve(b, a.order, n);
}

// Comptage d'un lot sur l'executeur : rend ses totaux (boules, incidences, non resolues et leurs sites, puis stats).
struct BatchCounts {
  u64 balls = 0, incidences = 0, unresolved = 0, unresolved_sites = 0;
  BatchStats stats{};
};

template <class B>
Result<BatchCounts> count_batch(B& b, LeafArrays<B>& a, const LeafView& v) noexcept {
  const u64 n = v.n;
  MHGP12_TRY(ensure_batch(b, a, n));
  MHGP12_TRY(b.launch(ClassifyKernel{v, a.cls.data(), a.order.keys[0].data(), a.unresolved.data(),
                                     a.unresolved_sites.data()},
                      n));
  MHGP12_TRY(b.launch(bfs::IotaKernel{a.order.vals[0].data(), n}, (n + bfs::kTile - 1) / bfs::kTile));
  const auto co = fin::radix_sort(b, a.order, n, 1);
  if (!co.ok()) return co.outcome();
  const LeafOut out{a.balls.data(), a.incidences.data(), a.status.data(), a.counts.data()};
  MHGP12_TRY(b.launch(CountKernel{v, a.cls.data(), a.order.vals[co.value()].data(), a.cases.data(), out}, n));
  BatchCounts c;
  const std::pair<typename B::template Array<u64>*, typename B::template Array<u64>*> scans[4] = {
      {&a.balls, &a.ball_at}, {&a.incidences, &a.population_at}, {&a.unresolved, &a.unresolved_at},
      {&a.unresolved_sites, &a.unresolved_site_at}};
  u64* totals[4] = {&c.balls, &c.incidences, &c.unresolved, &c.unresolved_sites};
  for (int k = 0; k < 4; ++k) {
    const auto total = fin::exclusive_scan(b, a.scan, scans[k].first->data(), scans[k].second->data(), n);
    if (!total.ok()) return total.outcome();
    *totals[k] = total.value();
  }
  const ReduceView rv{v.leaves, a.cls.data(), a.status.data(), a.counts.data(), a.balls.data(), n, fin::tiles_of(n),
                      a.tiles.data(), a.total.data()};
  MHGP12_TRY(b.launch(ReduceTileKernel{rv}, fin::tiles_of(n)));
  MHGP12_TRY(b.launch(ReduceTopKernel{rv}, 1));
  const auto stats = b.read(a.total, 0);
  if (!stats.ok()) return stats.outcome();
  c.stats = stats.value();
  return c;
}

}  // namespace mhgp12::catalogue_detail::dev
