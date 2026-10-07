// Raccord prive des deux plans : la voie fixe conserve ses preconditions et sa preparation historique.
#pragma once
#include <algorithm>

#include "catalogue/frontier.hpp"
#include "catalogue/adaptive_frontier.hpp"

namespace mhgp11::catalogue_detail {

inline Outcome prepare_frontier(Frontier& frontier, Run& run, sched::Pool&, CatalogueTimings* = nullptr) noexcept {
  u64 bytes = 0;
  MHGP11_TRY(frontier_memory_bound(run.cloud.sites(), kFrontierDepth, bytes));
  MHGP11_TRY(run.budget.admit(bytes));
  return frontier.prepare(run);
}
inline Outcome prepare_frontier(AdaptiveFrontier& frontier, Run& run, sched::Pool& pool,
                                CatalogueTimings* timings = nullptr) noexcept {
  return frontier.prepare(run, pool, timings);
}
inline Outcome verify_frontier(const Frontier& frontier, Run& run, sched::Pool&) noexcept {
  return frontier.verify(run);
}
inline Outcome verify_frontier(const AdaptiveFrontier& frontier, Run& run, sched::Pool& pool) noexcept {
  return frontier.verify(run, pool);
}

// Ordre de RECLAMATION des taches : liste la plus longue d'abord, puis ordinal (ordre LPT de Graham).
// Seul l'ordonnancement change : chaque ordinal garde son scratch, ses sorties et ses compteurs. Reclamer
// dans l'ordre du plan laissait une grosse tache partir en dernier et allonger le mur de toute sa duree.
template <class Front, std::size_t Capacity>
void heaviest_first(const Front& frontier, std::array<u32, Capacity>& order) noexcept {
  const u32 count = frontier.size();
  for (u32 i = 0; i < count; ++i) order[i] = i;
  std::sort(order.begin(), order.begin() + count, [&frontier](u32 a, u32 b) noexcept {
    const u32 x = frontier.task(a).count, y = frontier.task(b).count;
    return x != y ? x > y : a < b;
  });
}

struct DiagnosticAccess {
  static Outcome describe(const Frontier& frontier, u32 i, CatalogueTaskDiagnostic& out) noexcept {
    const auto& task = frontier.task(i);
    out.depth = task.depth; out.count = task.count;
    out.capacity = static_cast<u32>(task.storage.size());
    out.lo = task.box.lo; out.hi = task.box.hi;
    return {};
  }
  static Outcome describe(const AdaptiveFrontier& frontier, u32 i, CatalogueTaskDiagnostic& out) noexcept {
    return frontier.describe(i, out);
  }
  static CataloguePlanning planning(const Frontier& frontier) noexcept {
    CataloguePlanning result;
    result.plan_nodes = static_cast<u32>(frontier.ledger().nodes);  // <=511 dans la voie fixe
    result.plan_leaves = (result.plan_nodes + 1) / 2;
    result.empty_leaves = result.plan_leaves - frontier.size();
    return result;
  }
  static CataloguePlanning planning(const AdaptiveFrontier& frontier) noexcept { return frontier.planning(); }

  template <class Front>
  static Outcome prepare(CatalogueDiagnostics* out, const Front& frontier, MemoryBudget& budget,
                          bool replay = true) noexcept {
    if (out == nullptr) return {};
    MHGP11_TRY(out->tasks_.allocate(frontier.size(), budget));
    out->planning_ = planning(frontier);
    if (replay) MHGP11_TRY(frontier.verify_memory_bound(out->planning_.replay_bytes));
    else out->planning_.replay_bytes = 0;
    for (u32 i = 0; i < frontier.size(); ++i) {
      out->tasks_[i] = CatalogueTaskDiagnostic{};
      MHGP11_TRY(describe(frontier, i, out->tasks_[i]));
    }
    return {};
  }
  static void result(CatalogueDiagnostics* out, u32 i, const CatalogueLedger& ledger,
                     u64 count_ns, u64 fill_ns) noexcept {
    if (out != nullptr) {
      out->tasks_[i].ledger = ledger;
      out->tasks_[i].count_ns = count_ns; out->tasks_[i].fill_ns = fill_ns;
    }
  }
  static void single_result(CatalogueDiagnostics* out, u32 i, const CatalogueLedger& ledger,
                            u64 generation_ns, u64 compact_ns) noexcept {
    if (out == nullptr) return;
    out->tasks_[i].ledger = ledger;
    out->tasks_[i].single_pass_ns = generation_ns; out->tasks_[i].compact_ns = compact_ns;
  }
};

}  // namespace mhgp11::catalogue_detail
