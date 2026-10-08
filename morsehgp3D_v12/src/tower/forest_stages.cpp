// Etages M et V de build_forests sur le Pool (forest.hpp) et grand livre. M : tranches alignees sur les rangs pour
// tous les ordres a la fois, quatre phases paralleles (forest_contract.cpp), sommes prefixes par le pilote entre deux
// phases. V : morceaux de naissances des ordres k >= 2 (LEM-T6), puis morceaux de fusions (LEM-T5) ; chaque morceau a
// ses compteurs, sommes par le pilote dans l'ordre des morceaux.
#include <algorithm>

#include "sched/sched.hpp"
#include "tower/forest_internal.hpp"

namespace mhgp12::tower::detail {
namespace {

inline constexpr u64 kVerticalChunk = 8192;

// Fin de la tranche commencant a lo : au moins `events` evenements, prolongee jusqu'au changement de rang.
u32 slice_end(const OrderWork& w, u32 lo, u32 events) noexcept {
  const u32 ne = w.event_count;
  u32 hi = static_cast<u32>(std::min<u64>(u64{lo} + events, ne));
  while (hi < ne && w.events[hi].rank == w.events[hi - 1].rank) ++hi;
  return hi;
}

Outcome make_slices(BuildState& s) noexcept {
  u64 count = 0;
  for (u64 i = 0; i < s.inputs.size(); ++i)
    for (u32 lo = 0; lo < s.work[i].event_count; lo = slice_end(s.work[i], lo, s.params.slice_events)) ++count;
  if (count > 0) MHGP12_TRY(s.slices.allocate(count, s.budget));
  u64 j = 0;
  for (u64 i = 0; i < s.inputs.size(); ++i)
    for (u32 lo = 0; lo < s.work[i].event_count;) {
      const u32 hi = slice_end(s.work[i], lo, s.params.slice_events);
      s.slices[j++] = Slice{static_cast<u32>(i), lo, hi, 0, 0, 0, 0, 0};
      lo = hi;
    }
  return {};
}

Outcome classes_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& s = *static_cast<BuildState*>(context);
  for (u64 j = begin; j < end; ++j) {
    Stopwatch watch;
    contract_classes(s.work[s.slices[j].order], s.slices[j]);
    s.slices[j].ns += watch.nanoseconds();
  }
  return {};
}
Outcome nodes_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& s = *static_cast<BuildState*>(context);
  for (u64 j = begin; j < end; ++j) {
    Stopwatch watch;
    Slice& slice = s.slices[j];
    contract_nodes(s.work[slice.order], s.forests.orders[slice.order], slice);
    slice.ns += watch.nanoseconds();
  }
  return {};
}
Outcome parents_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& s = *static_cast<BuildState*>(context);
  for (u64 j = begin; j < end; ++j) {
    Stopwatch watch;
    Slice& slice = s.slices[j];
    contract_parents(s.work[slice.order], s.forests.orders[slice.order], slice);
    slice.ns += watch.nanoseconds();
  }
  return {};
}
Outcome children_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& s = *static_cast<BuildState*>(context);
  for (u64 j = begin; j < end; ++j) {
    Stopwatch watch;
    Slice& slice = s.slices[j];
    contract_children(s.work[slice.order], s.forests.orders[slice.order], slice);
    slice.ns += watch.nanoseconds();
  }
  return {};
}

// Apres la phase A : identifiants des premiers noeuds des tranches, tableaux des noeuds, naissances.
Outcome allocate_nodes(BuildState& s, u64 i, u64& slice) noexcept {
  OrderForest& f = s.forests.orders[i];
  OrderWork& w = s.work[i];
  u64 next = f.births;
  for (; slice < s.slices.size() && s.slices[slice].order == i; ++slice) {
    s.slices[slice].node0 = static_cast<u32>(next);
    next += s.slices[slice].classes;
  }
  if (next >= kNone) return fail(Reason::tower_capacity);
  const u64 nn = next;
  MHGP12_TRY(f.rank.allocate(nn, s.budget));
  MHGP12_TRY(f.parent.allocate(nn, s.budget));
  MHGP12_TRY(f.minleaf.allocate(nn, s.budget));
  MHGP12_TRY(f.children.off.allocate(nn + 1, s.budget));
  if (nn > 1) MHGP12_TRY(f.children.val.allocate(nn - 1, s.budget));
  MHGP12_TRY(w.child_count.allocate(nn, s.budget));
  const ForestInput& in = s.inputs[i];
  for (u64 v = 0; v < nn; ++v) {
    f.parent[v] = kNone;
    w.child_count[v] = 0;
  }
  for (u32 v = 0; v < f.births; ++v) {
    f.rank[v] = idx(in.birth_rank[w.birth_order[v]]);
    f.minleaf[v] = v;
  }
  for (u64 v = 0; v <= f.births; ++v) f.children.off[v] = 0;
  return {};
}

// Apres la phase C : decalages des enfants par tranche, controle du compte (une arete par noeud sauf la racine).
Outcome place_children(BuildState& s, u64 i, u64& slice) noexcept {
  OrderForest& f = s.forests.orders[i];
  u64 at = 0;
  for (; slice < s.slices.size() && s.slices[slice].order == i; ++slice) {
    s.slices[slice].child0 = at;
    at += s.slices[slice].children;
  }
  if (at + 1 != f.nodes()) return fail(Reason::tower_invariant);
  f.children.off[f.nodes()] = at;
  return {};
}

// Apres la phase D : noeud du sommet de chaque cellule, racine unique ; liberation du travail de la contraction.
Outcome finish_order(BuildState& s, u64 i) noexcept {
  OrderForest& f = s.forests.orders[i];
  OrderWork& w = s.work[i];
  const u64 nc = s.inputs[i].cell_ball.size();
  if (nc > 0) MHGP12_TRY(f.cell_node.allocate(nc, s.budget));
  for (u64 t = 0; t < nc; ++t) f.cell_node[t] = node_of_top(w.cell_top[t], f.event_node.span());
  u32 root = kNone;
  for (u32 v = 0; v < f.nodes(); ++v)
    if (f.parent[v] == kNone) {
      if (root != kNone) return fail(Reason::tower_invariant);
      root = v;
    }
  if (root == kNone) return fail(Reason::tower_invariant);
  f.root = root;
  s.counters[i].classes = f.nodes() - f.births;
  w.local.reset();
  w.class_min.reset();
  w.keys.reset();
  w.child_count.reset();
  w.events.reset();
  w.cell_top.reset();
  return {};
}

}  // namespace

Outcome run_contraction(BuildState& s, sched::Pool& pool) noexcept {
  u64 bytes = 0;
  for (u64 i = 0; i < s.inputs.size(); ++i) bytes += contraction_bytes(s.inputs[i], s.work[i].event_count);
  MHGP12_TRY(admit_stage(s, kStageM, bytes));
  Stopwatch watch;
  MHGP12_TRY(make_slices(s));
  for (u64 i = 0; i < s.inputs.size(); ++i) {
    const u64 ne = s.work[i].event_count;
    if (ne == 0) continue;
    MHGP12_TRY(s.work[i].local.allocate(ne, s.budget));
    MHGP12_TRY(s.work[i].class_min.allocate(ne, s.budget));
    MHGP12_TRY(s.work[i].keys.allocate(ne, s.budget));
    MHGP12_TRY(s.forests.orders[i].event_node.allocate(ne, s.budget));
    MHGP12_TRY(s.forests.orders[i].event_rank.allocate(ne, s.budget));
  }
  const u64 n = s.slices.size();
  MHGP12_TRY(pool.parallel_for(n, 1, &s, classes_body));
  for (u64 i = 0, slice = 0; i < s.inputs.size(); ++i) MHGP12_TRY(allocate_nodes(s, i, slice));
  MHGP12_TRY(pool.parallel_for(n, 1, &s, nodes_body));
  MHGP12_TRY(pool.parallel_for(n, 1, &s, parents_body));
  for (u64 i = 0, slice = 0; i < s.inputs.size(); ++i) MHGP12_TRY(place_children(s, i, slice));
  MHGP12_TRY(pool.parallel_for(n, 1, &s, children_body));
  for (u64 i = 0; i < s.inputs.size(); ++i) MHGP12_TRY(finish_order(s, i));
  s.contraction_ns = watch.nanoseconds();
  for (u64 j = 0; j < n; ++j) s.physical[s.slices[j].order].contraction_ns += s.slices[j].ns;
  return {};
}

namespace {

// Morceau de verticales : ordre (indice), intervalle, compteurs propres.
struct VerticalTask {
  u32 order = 0;
  u64 begin = 0, end = 0;
  u64 ns = 0;
};
struct VerticalRun {
  BuildState& state;
  std::span<VerticalTask> tasks;
  std::span<ForestWork> counters;
  bool merges;
};

Outcome vertical_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& run = *static_cast<VerticalRun*>(context);
  BuildState& s = run.state;
  Outcome out;
  for (u64 j = begin; j < end; ++j) {
    VerticalTask& t = run.tasks[j];
    const OrderForest& below = s.forests.orders[t.order - 1];
    OrderForest& f = s.forests.orders[t.order];
    Stopwatch watch;
    out = merge(out, run.merges ? merge_images(below, f, t.begin, t.end, run.counters[j])
                                : birth_images(s.inputs[t.order], s.inputs[t.order - 1], below, f, t.begin, t.end,
                                               run.counters[j]));
    t.ns = watch.nanoseconds();
  }
  return out;
}

// Une passe : morceaux des naissances (merges faux) ou des fusions de tous les ordres k >= 2.
Outcome vertical_pass(BuildState& s, sched::Pool& pool, bool merges) noexcept {
  u64 count = 0;
  for (u64 i = 1; i < s.inputs.size(); ++i) {
    const OrderForest& f = s.forests.orders[i];
    const u64 items = merges ? f.nodes() - f.births : f.births;
    count += (items + kVerticalChunk - 1) / kVerticalChunk;
  }
  if (count == 0) return {};
  Buffer<VerticalTask> tasks;
  Buffer<ForestWork> counters;
  MHGP12_TRY(admit_stage(s, kStageV, count * (sizeof(VerticalTask) + sizeof(ForestWork))));
  MHGP12_TRY(tasks.allocate(count, s.budget));
  MHGP12_TRY(counters.allocate(count, s.budget));
  u64 j = 0;
  for (u64 i = 1; i < s.inputs.size(); ++i) {
    const OrderForest& f = s.forests.orders[i];
    const u64 first = merges ? f.births : 0, last = merges ? f.nodes() : f.births;
    for (u64 b = first; b < last; b += kVerticalChunk)
      tasks[j++] = VerticalTask{static_cast<u32>(i), b, std::min(last, b + kVerticalChunk), 0};
  }
  for (u64 t = 0; t < count; ++t) counters[t] = ForestWork{};
  VerticalRun run{s, tasks.span(), counters.span(), merges};
  MHGP12_TRY(pool.parallel_for(count, 1, &run, vertical_body));
  for (u64 t = 0; t < count; ++t) {
    s.physical[tasks[t].order].vertical_ns += tasks[t].ns;
    ForestWork& c = s.counters[tasks[t].order];
    const ForestWork& d = counters[t];
    c.t6_from_cell += d.t6_from_cell;
    c.t6_climbs += d.t6_climbs;
    c.t6_from_birth += d.t6_from_birth;
    c.t5_queries += d.t5_queries;
    c.t5_hops += d.t5_hops;
    c.t5_probes += d.t5_probes;
    c.t5_max_hops = std::max(c.t5_max_hops, d.t5_max_hops);
  }
  return {};
}

}  // namespace

Outcome run_verticals(BuildState& s, sched::Pool& pool) noexcept {
  u64 bytes = 0;
  for (u64 i = 1; i < s.inputs.size(); ++i) bytes += 4 * u64{s.forests.orders[i].nodes()};
  MHGP12_TRY(admit_stage(s, kStageV, bytes));
  for (u64 i = 1; i < s.inputs.size(); ++i) {
    OrderForest& f = s.forests.orders[i];
    MHGP12_TRY(f.lower.allocate(f.nodes(), s.budget));
    for (u32 v = 0; v < f.nodes(); ++v) f.lower[v] = kNone;
  }
  Stopwatch births_watch;
  MHGP12_TRY(vertical_pass(s, pool, false));
  s.vertical_births_ns = births_watch.nanoseconds();
  Stopwatch merges_watch;
  MHGP12_TRY(vertical_pass(s, pool, true));
  s.vertical_merges_ns = merges_watch.nanoseconds();
  return {};
}

void fill_ledger(const BuildState& s, u32 threads, ForestLedger& ledger) noexcept {
  ledger = ForestLedger{};
  ledger.kmax = s.forests.kmax;
  for (u64 i = 0; i < s.inputs.size(); ++i) {
    const OrderForest& f = s.forests.orders[i];
    const ForestInput& in = s.inputs[i];
    ForestObject& o = ledger.object[i];
    o.births = f.births;
    o.merges = f.nodes() - f.births;
    o.verticals = i > 0 ? f.nodes() : 0;
    o.cells = in.cell_ball.size();
    o.representatives = in.targets.size();
    for (u64 t = 0; t < in.cell_ball.size(); ++t) o.inert_cells += in.rep_offsets[t + 1] - in.rep_offsets[t] == 1;
    for (u32 v = f.births; v < f.nodes(); ++v) {
      const u64 arity = f.children.off[u64{v} + 1] - f.children.off[v];
      if (arity >= 2) ++o.arity[std::min<u64>(arity, 9) - 2];
      o.max_arity = std::max(o.max_arity, arity);
    }
    ledger.work[i] = s.counters[i];
    ledger.physical[i] = s.physical[i];
  }
  ledger.kernels_ns = s.kernels_ns;
  ledger.contraction_ns = s.contraction_ns;
  ledger.vertical_births_ns = s.vertical_births_ns;
  ledger.vertical_merges_ns = s.vertical_merges_ns;
  ledger.registry_ns = s.registry_ns;
  ledger.slices = s.slices.size();
  ledger.threads = threads;
}

}  // namespace mhgp12::tower::detail
