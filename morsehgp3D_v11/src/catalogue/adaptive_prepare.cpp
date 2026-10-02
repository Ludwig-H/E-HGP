// Preparation du plan : priorite entiere stable, fantomes conserves, deux enfants payes avant chaque ronde.
#include "catalogue/adaptive_internal.hpp"

namespace mhgp11::catalogue_detail::adaptive_detail {

struct Builder {
  AdaptiveFrontier& out;
  Run& run;
  sched::Pool& pool;

  Outcome capture(u32 node, const ReadyNode& ready) noexcept {
    auto& value = out.plan_[node];
    value.box = ready.box;
    value.count = ready.count;
    if (ready.storage.size() > std::numeric_limits<u32>::max()) return fail(Reason::catalogue_invariant);
    value.capacity = static_cast<u32>(ready.storage.size());
    value.kind = ready.count == 0 ? Kind::empty : Kind::job;
    if (ready.count == 0) { ++out.planning_.empty_leaves; return {}; }
    // La racine est son enveloppe : tous ses sites sont interieurs a la boite demi-ouverte.
    if (node == 0) { value.inside = ready.count; return {}; }
    MHGP11_TRY(checked_add(out.planning_.priority_tests, ready.count));
    for (SiteIdx site : ready.sites()) {
      const u32 i = idx(site);
      const std::array<i64, 3> x{run.cloud.x()[i], run.cloud.y()[i], run.cloud.z()[i]};
      bool inside = true;
      for (u32 axis = 0; axis < 3; ++axis)
        inside = inside && value.box.lo[axis] <= x[axis] && x[axis] < value.box.hi[axis];
      value.inside += inside ? 1u : 0u;
    }
    return {};
  }

  bool before(u32 a, u32 b) const noexcept {
    const auto& x = out.plan_[a];
    const auto& y = out.plan_[b];
    if (x.inside != y.inside) return x.inside > y.inside;
    if (x.count != y.count) return x.count > y.count;
    if (x.path != y.path) return x.path < y.path;
    return x.depth < y.depth;
  }

  u32 select(std::array<u32, kAdaptiveTasks>& selected) const noexcept {
    u32 count = 0;
    for (u32 i = 0; i < out.state_.count; ++i) {
      const auto& current = out.state_.live[i];
      Box left, right;
      if (!split_ready(current.ready, run.params, left, right)) continue;
      u32 at = count++;
      while (at != 0 && before(current.node, selected[at - 1])) {
        selected[at] = selected[at - 1]; --at;
      }
      selected[at] = current.node;
    }
    return std::min(count, kAdaptiveTasks - out.planning_.plan_leaves);
  }

  Outcome record(std::span<const u32> parents, const Children& children) noexcept {
    const u32 first_split = (out.planning_.plan_nodes - 1) / 2;
    for (u32 i = 0; i < parents.size(); ++i) {
      const u32 node = parents[i];
      auto& parent = out.plan_[node];
      if (parent.depth >= kMaxDepth || parent.kind != Kind::job ||
          out.planning_.plan_nodes + 2 > kAdaptiveNodes) return fail(Reason::catalogue_invariant);
      out.parents_[first_split + i] = node;
      parent.kind = Kind::split;
      parent.first_child = out.planning_.plan_nodes;
      for (u32 bit = 0; bit < 2; ++bit) {
        const u32 child = out.planning_.plan_nodes++;
        out.plan_[child] = PlanNode{};
        out.plan_[child].depth = parent.depth + 1;
        out.plan_[child].path = parent.path;
        out.plan_[child].path[parent.depth / 64] |= u64(bit) << (63 - parent.depth % 64);
        MHGP11_TRY(capture(child, children[2 * i + bit].ready));
      }
    }
    out.planning_.plan_leaves += static_cast<u32>(parents.size());
    out.round_end_[out.planning_.rounds++] = (out.planning_.plan_nodes - 1) / 2;
    return {};
  }

  Outcome enumerate(u32 node, u32 depth) noexcept {
    if (node >= out.planning_.plan_nodes || depth > kMaxDepth || out.plan_[node].depth != depth)
      return fail(Reason::catalogue_invariant);
    const auto& value = out.plan_[node];
    if (value.kind == Kind::empty) return {};
    if (value.kind == Kind::split) {
      if (value.first_child <= node) return fail(Reason::catalogue_invariant);
      MHGP11_TRY(enumerate(value.first_child, depth + 1));
      return enumerate(value.first_child + 1, depth + 1);
    }
    if (value.kind != Kind::job || out.count_ >= kAdaptiveTasks ||
        out.state_.slots[node] >= out.state_.count) return fail(Reason::catalogue_invariant);
    out.tasks_[out.count_++] = out.state_.slots[node];
    return {};
  }

  Outcome build() noexcept {
    ReadyNode root;
    MHGP11_TRY(prepare_root(run, root));
    out.planning_.adaptive = true;
    out.planning_.plan_nodes = out.planning_.plan_leaves = 1;
    out.planning_.replay_bytes = 8 * u64(run.cloud.sites());
    out.plan_[0] = PlanNode{};
    MHGP11_TRY(capture(0, root));
    if (root.count != 0) {
      out.state_.live[0] = LiveNode{std::move(root), 0};
      out.state_.count = 1; out.state_.slots[0] = 0;
    } else root = ReadyNode{};
    Children children;
    std::array<u32, kAdaptiveTasks> selected{};
    while (out.planning_.plan_leaves < kAdaptiveTasks) {
      const u32 count = select(selected);
      if (count == 0) break;
      const auto parents = std::span(selected).first(count);
      u64 extra = 0, simultaneous = 0;
      MHGP11_TRY(round_bytes(out.state_, parents, extra));
      if (!run.budget.admit(extra).ok()) { out.planning_.memory_fallback = true; break; }
      MHGP11_TRY(state_bytes(out.state_, simultaneous));
      MHGP11_TRY(checked_add(simultaneous, extra));
      out.planning_.replay_bytes = std::max(out.planning_.replay_bytes, simultaneous);
      MHGP11_TRY(run_round(run, pool, out.state_, parents, children));
      MHGP11_TRY(record(parents, children));
      MHGP11_TRY(publish_round(out.state_, parents,
                              std::span(out.plan_).first(out.planning_.plan_nodes), children));
    }
    MHGP11_TRY(enumerate(0, 0));
    if (out.count_ != out.state_.count || out.count_ + out.planning_.empty_leaves != out.planning_.plan_leaves)
      return fail(Reason::catalogue_invariant);
    return {};
  }
};

}  // namespace mhgp11::catalogue_detail::adaptive_detail

namespace mhgp11::catalogue_detail {

Outcome AdaptiveFrontier::prepare(Run& run, sched::Pool& pool) noexcept {
  if (prepared_ || run.ledger != CatalogueLedger{}) return fail(Reason::catalogue_invariant);
  clear();
  const auto result = adaptive_detail::Builder{*this, run, pool}.build();
  if (!result.ok()) { clear(); return result; }
  cloud_ = &run.cloud; params_ = run.params; ledger_ = run.ledger; prepared_ = true;
  return {};
}

}  // namespace mhgp11::catalogue_detail
