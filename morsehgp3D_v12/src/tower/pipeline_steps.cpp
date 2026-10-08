// Corps des etapes de la foret dans la Session recouverte (pipeline.hpp) : un morceau d'une etape d'un ordre, son
// imputation physique, et les taches d'indices de racine (levier I de T2-d-A6). Les etapes par morceaux ecrivent a des
// places fixees par l'entree (plages de naissances, de positions, d'evenements, de noeuds, de lignes) : rien ne depend
// de l'entrelacement des fils.
#include <algorithm>

#include "tower/pipeline.hpp"

namespace mhgp12::tower::detail {

u64 now_ns(const Pipeline& p) noexcept {
  return static_cast<u64>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - p.start).count());
}

ForestWork& work_of(Pipeline& p, u32 i, u32 w) noexcept { return p.f_counters[u64{i} * p.threads + w]; }
ForestPhysical& physical_of(Pipeline& p, u32 i, u32 w) noexcept { return p.f_physical[u64{i} * p.threads + w]; }

namespace {

// Etapes de T : numerotation par morceaux (levier N), ouverture du noyau, historique par morceaux (levier H).
Outcome kernel_steps(Pipeline& p, u32 w, u32 i, Step s, u64 item, u32& rank) noexcept {
  BuildState& b = p.forest;
  const ForestInput& in = b.inputs[i];
  OrderForest& f = b.forests.orders[i];
  OrderWork& work = b.work[i];
  switch (s) {
    case kNumberOpen:
      MHGP12_TRY(open_numbering(b, i));
      set_items(p, i, kNumber, pieces(f.births, kNumberItems));
      set_items(p, i, kNumberClose, pieces(f.births, kNumberItems));
      return {};
    case kNumber:
      return number_piece(b, i, item * kNumberItems, piece_end(item, kNumberItems, f.births), work_of(p, i, w));
    case kNumberClose:
      close_numbering(b, i, item * kNumberItems, piece_end(item, kNumberItems, f.births));
      return {};
    case kKernelOpen:  // tampon des feuilles (rang 1, comme number_order), puis ouverture du noyau (rang 3)
      MHGP12_TRY(open_leaves(b, i));
      rank = 3;
      return open_kernel(in, f, work, b.budget);
    case kHistoryCheck:
      return check_history(f, work, item * kHistoryItems, piece_end(item, kHistoryItems, work.event_count));
    case kDepth:
      history_depth(f, item * kHistoryItems, piece_end(item, kHistoryItems, f.births), work_of(p, i, w));
      return {};
    case kHistory: return build_survivor_events(f, work, b.budget);
    default: break;
  }
  return fail(Reason::tower_invariant);
}

}  // namespace

Outcome step_body(Pipeline& p, u32 w, u32 i, Step s, u64 item, u32& rank) noexcept {
  BuildState& b = p.forest;
  const ForestInput& in = b.inputs[i];
  OrderForest& f = b.forests.orders[i];
  OrderWork& work = b.work[i];
  rank = refusal_rank(s);
  switch (s) {
    case kCheck: return check_forest_input(in);
    case kNumberOpen:
    case kNumber:
    case kNumberClose:
    case kKernelOpen:
    case kHistoryCheck:
    case kDepth:
    case kHistory: return kernel_steps(p, w, i, s, item, rank);
    case kSlices:
      MHGP12_TRY(open_contraction(b, i));
      for (const Step phase : {kClasses, kNodes, kParents, kChildren}) set_items(p, i, phase, work.slices.size());
      return {};
    case kClasses: contract_classes(work, work.slices[item]); return {};
    case kNodes0: return allocate_nodes(b, i);
    case kNodes: contract_nodes(work, f, work.slices[item]); return {};
    case kParents: contract_parents(work, f, work.slices[item]); return {};
    case kPlace: return place_children(b, i);
    case kChildren: contract_children(work, f, work.slices[item]); return {};
    case kFinish: return finish_order(b, i);
    case kLower:
      MHGP12_TRY(open_verticals(b, i));
      set_items(p, i, kBirths, pieces(f.births, kVerticalItems));
      set_items(p, i, kMerges, pieces(f.nodes() - f.births, kVerticalItems));
      return {};
    case kBirths:
      return birth_images(in, b.inputs[i - 1], b.forests.orders[i - 1], f, item * kVerticalItems,
                          piece_end(item, kVerticalItems, f.births), work_of(p, i, w));
    case kMerges:
      return merge_images(b.forests.orders[i - 1], f, f.births + item * kVerticalItems,
                          f.births + piece_end(item, kVerticalItems, f.nodes() - f.births), work_of(p, i, w));
    case kRows: {
      release_events(b, i);
      u64 reads = 0;
      MHGP12_TRY(prepare_rows(b, i, reads));
      const u64 rows = f.retained_cell.size();
      set_items(p, i, kCollect, pieces(rows, kRowItems));
      set_items(p, i, kFill, pieces(rows, kRowItems));
      return {};
    }
    case kCollect:
      return collect_rows(b, i, item * kRowItems, piece_end(item, kRowItems, f.retained_cell.size()),
                          work_of(p, i, w));
    case kPlaceRows: return place_rows(b, i);
    case kFill: fill_rows(b, i, item * kRowItems, piece_end(item, kRowItems, f.retained_cell.size())); return {};
    case kKernel:
    case kStepCount: break;
  }
  return fail(Reason::tower_invariant);
}

void charge_stage(Pipeline& p, u32 w, u32 i, Step s, u64 ns) noexcept {
  ForestPhysical& ph = physical_of(p, i, w);
  if (s <= kNumberClose) ph.births_ns += ns;
  else if (s == kKernelOpen) ph.kernel_ns += ns;
  else if (s >= kHistoryCheck && s <= kHistory) ph.history_ns += ns;
  else if (s >= kSlices && s <= kFinish) ph.contraction_ns += ns;
  else if (s >= kLower && s <= kMerges) ph.vertical_ns += ns;
  else ph.registry_ns += ns;
}

}  // namespace mhgp12::tower::detail
