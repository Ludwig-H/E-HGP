// Frontiere adaptative : primitives de ronde, lectures du plan gele et bornes de coexistence explicites.
#include "catalogue/adaptive_internal.hpp"

namespace mhgp11::catalogue_detail {

Outcome add_catalogue_ledger(CatalogueLedger& sum, const CatalogueLedger& part) noexcept {
  constexpr std::array<u64 CatalogueLedger::*, 18> fields{
      &CatalogueLedger::nodes, &CatalogueLedger::leaves, &CatalogueLedger::filter_tests,
      &CatalogueLedger::dominance_tests, &CatalogueLedger::prefixes, &CatalogueLedger::judged,
      &CatalogueLedger::census_tests, &CatalogueLedger::emitted, &CatalogueLedger::incidences,
      &CatalogueLedger::q4_candidates, &CatalogueLedger::q4_levels, &CatalogueLedger::region_pair_tests,
      &CatalogueLedger::region_pair_rejects, &CatalogueLedger::region_line_tests,
      &CatalogueLedger::region_line_rejects, &CatalogueLedger::region_line_evaluations,
      &CatalogueLedger::region_line_cache_hits, &CatalogueLedger::region_line_fallbacks};
  for (auto field : fields) MHGP11_TRY(checked_add(sum.*field, part.*field));
  sum.max_leaf = std::max(sum.max_leaf, part.max_leaf);
  sum.max_depth = std::max(sum.max_depth, part.max_depth);
  return {};
}

namespace adaptive_detail {

Outcome prepare_root(Run& run, ReadyNode& ready) noexcept {
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<SiteIdx>(bytes, 2 * u64(run.cloud.sites())));
  MHGP11_TRY(run.budget.admit(bytes));
  Buffer<SiteIdx> root;
  Box box;
  MHGP11_TRY(make_root(run, root, box));
  return prepare_node(run, root.span(), box, 0, ready);
}

Outcome state_bytes(const State& state, u64& bytes) noexcept {
  bytes = 0;
  for (u32 i = 0; i < state.count; ++i)
    MHGP11_TRY(add_bytes<SiteIdx>(bytes, state.live[i].ready.storage.size()));
  return {};
}

Outcome round_bytes(const State& state, std::span<const u32> parents, u64& bytes) noexcept {
  bytes = 0;
  for (u32 node : parents) {
    if (node >= kAdaptiveNodes || state.slots[node] >= state.count)
      return fail(Reason::catalogue_invariant);
    MHGP11_TRY(add_bytes<SiteIdx>(bytes, 2 * u64(state.live[state.slots[node]].ready.count)));
  }
  return {};
}

struct RoundRun {
  Run& source;
  const State& state;
  std::span<const u32> parents;
  Children& children;

  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<RoundRun*>(context);
    if (end > 2 * self.parents.size()) return fail(Reason::catalogue_invariant);
    for (u64 i = begin; i < end; ++i) {
      const u32 node = self.parents[i / 2];
      if (node >= kAdaptiveNodes || self.state.slots[node] >= self.state.count)
        return fail(Reason::catalogue_invariant);
      const auto& parent = self.state.live[self.state.slots[node]].ready;
      Box left, right;
      if (!split_ready(parent, self.source.params, left, right)) return fail(Reason::catalogue_invariant);
      Workspace unused;
      Collector collector;
      Run run{self.source.cloud, self.source.params, self.source.budget, unused, collector, {}, self.source.quota};
      MHGP11_TRY(prepare_node(run, parent.sites(), i % 2 == 0 ? left : right,
                             parent.depth + 1, self.children[i].ready));
      self.children[i].ledger = run.ledger;
    }
    return {};
  }
};

Outcome run_round(Run& run, sched::Pool& pool, const State& state,
                  std::span<const u32> parents, Children& children) noexcept {
  if (parents.empty() || parents.size() > kAdaptiveTasks / 2) return fail(Reason::catalogue_invariant);
  for (u64 i = 0; i < 2 * parents.size(); ++i) children[i] = ChildResult{};
  RoundRun context{run, state, parents, children};
  MHGP11_TRY(pool.parallel_for(2 * parents.size(), 1, &context, RoundRun::body));
  for (u64 i = 0; i < 2 * parents.size(); ++i)
    MHGP11_TRY(add_catalogue_ledger(run.ledger, children[i].ledger));
  return {};
}

Outcome publish_round(State& state, std::span<const u32> parents,
                      std::span<const PlanNode> plan, Children& children) noexcept {
  std::array<bool, kAdaptiveNodes> selected{};
  for (u32 node : parents) {
    if (node >= plan.size() || selected[node]) return fail(Reason::catalogue_invariant);
    selected[node] = true; state.slots[node] = kNone;
  }
  u32 count = 0;
  for (u32 i = 0; i < state.count; ++i) {
    if (selected[state.live[i].node]) state.live[i].ready = ReadyNode{};
    else {
      if (i != count) state.live[count] = std::move(state.live[i]);
      state.slots[state.live[count].node] = count;
      ++count;
    }
  }
  for (u64 i = 0; i < 2 * parents.size(); ++i) {
    const u32 child = plan[parents[i / 2]].first_child + static_cast<u32>(i % 2);
    if (child >= plan.size()) return fail(Reason::catalogue_invariant);
    if (children[i].ready.count == 0) { children[i].ready = ReadyNode{}; continue; }
    if (count == kAdaptiveTasks) return fail(Reason::catalogue_invariant);
    state.live[count] = LiveNode{std::move(children[i].ready), child};
    state.slots[child] = count++;
  }
  state.count = count;
  return {};
}

bool same_ready(const PlanNode& expected, const ReadyNode& ready) noexcept {
  return expected.count == ready.count && expected.capacity == ready.storage.size() &&
         expected.box.lo == ready.box.lo && expected.box.hi == ready.box.hi &&
         (ready.count == 0 || expected.depth == ready.depth);
}

bool same_sites(const ReadyNode& left, const ReadyNode& right) noexcept {
  return left.count == right.count && left.depth == right.depth && left.box.lo == right.box.lo &&
         left.box.hi == right.box.hi && left.storage.size() == right.storage.size() &&
         std::equal(left.sites().begin(), left.sites().end(), right.sites().begin());
}

}  // namespace adaptive_detail

void AdaptiveFrontier::clear() noexcept {
  for (auto& entry : state_.live) entry.ready = ReadyNode{};
  state_.count = 0; state_.slots.fill(kNone);
  cloud_ = nullptr; ledger_ = {}; planning_ = {}; count_ = 0; prepared_ = false;
}

bool AdaptiveFrontier::matches(const Run& run) const noexcept {
  return prepared_ && cloud_ == &run.cloud && params_.kmax == run.params.kmax &&
         params_.leaf_size == run.params.leaf_size && params_.max_leaf == run.params.max_leaf &&
         params_.max_nodes == run.params.max_nodes && params_.ball_limit == run.params.ball_limit;
}

Outcome AdaptiveFrontier::describe(u32 i, CatalogueTaskDiagnostic& out) const noexcept {
  if (!prepared_ || i >= count_) return fail(Reason::catalogue_invariant);
  const auto& live = state_.live[tasks_[i]];
  const auto& node = plan_[live.node];
  out.path = node.path; out.depth = node.depth; out.count = node.count;
  out.capacity = node.capacity; out.inside = node.inside;
  out.lo = node.box.lo; out.hi = node.box.hi;
  out.path_known = out.inside_known = true;
  return {};
}

Outcome AdaptiveFrontier::execute_task(u32 i, Run& run) const noexcept {
  if (!matches(run) || i >= count_) return fail(Reason::catalogue_invariant);
  return run_ready(run, task(i));
}

Outcome AdaptiveFrontier::owned_bytes(u64& bytes) const noexcept {
  bytes = 0;
  if (!prepared_) return fail(Reason::catalogue_invariant);
  return adaptive_detail::state_bytes(state_, bytes);
}

Outcome AdaptiveFrontier::verify_memory_bound(u64& bytes) const noexcept {
  bytes = 0;
  if (!prepared_) return fail(Reason::catalogue_invariant);
  bytes = planning_.replay_bytes;
  return {};
}

Outcome AdaptiveFrontier::suffix_memory_bound(u32 workers, u64& bytes) const noexcept {
  bytes = 0;
  if (!prepared_) return fail(Reason::catalogue_invariant);
  if (workers == 0 || workers > sched::kMaxWorkers) return fail(Reason::parameter_out_of_range);
  std::array<u64, sched::kMaxWorkers> largest{};
  for (u32 i = 0; i < count_; ++i) {
    const auto& node = task(i);
    Box left, right;
    if (!split_ready(node, params_, left, right)) continue;
    if (node.depth >= kMaxDepth) return fail(Reason::catalogue_invariant);
    u64 bound = 0;
    MHGP11_TRY(add_bytes<SiteIdx>(bound, u64(node.count) * (kMaxDepth - node.depth)));
    for (u32 j = 0; j < workers; ++j) if (bound > largest[j]) std::swap(bound, largest[j]);
  }
  for (u32 i = 0; i < workers; ++i) MHGP11_TRY(checked_add(bytes, largest[i]));
  return {};
}

}  // namespace mhgp11::catalogue_detail
