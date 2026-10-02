// Rejeu du plan GELE, y compris branches eteintes : aucune selection ni lecture de priorite repetee.
#include "catalogue/adaptive_internal.hpp"

namespace mhgp11::catalogue_detail::adaptive_detail {

struct Replay {
  const AdaptiveFrontier& source;
  Run& run;
  sched::Pool& pool;

  Outcome check_child(u32 child, const ReadyNode& ready) const noexcept {
    if (child >= source.planning_.plan_nodes || !same_ready(source.plan_[child], ready))
      return fail(Reason::catalogue_invariant);
    return {};
  }

  Outcome replay() noexcept {
    // Cette admission est aussi faite avec les sorties par le pilote ; elle protege l'usage prive autonome.
    MHGP11_TRY(run.budget.admit(source.planning_.replay_bytes));
    State state;
    state.slots.fill(kNone);
    ReadyNode root;
    MHGP11_TRY(prepare_root(run, root));
    MHGP11_TRY(check_child(0, root));
    if (root.count != 0) {
      state.live[0] = LiveNode{std::move(root), 0}; state.count = 1; state.slots[0] = 0;
    } else root = ReadyNode{};
    Children children;
    u32 start = 0;
    for (u32 round = 0; round < source.planning_.rounds; ++round) {
      const u32 end = source.round_end_[round];
      if (end <= start || end >= kAdaptiveTasks) return fail(Reason::catalogue_invariant);
      const auto parents = std::span(source.parents_).subspan(start, end - start);
      u64 extra = 0;
      MHGP11_TRY(round_bytes(state, parents, extra));
      MHGP11_TRY(run.budget.admit(extra));
      MHGP11_TRY(run_round(run, pool, state, parents, children));
      for (u32 i = 0; i < parents.size(); ++i) {
        const u32 child = source.plan_[parents[i]].first_child;
        MHGP11_TRY(check_child(child, children[2 * i].ready));
        MHGP11_TRY(check_child(child + 1, children[2 * i + 1].ready));
      }
      MHGP11_TRY(publish_round(state, parents,
                              std::span(source.plan_).first(source.planning_.plan_nodes), children));
      start = end;
    }
    if (2 * start + 1 != source.planning_.plan_nodes || state.count != source.count_ || run.ledger != source.ledger_)
      return fail(Reason::catalogue_invariant);
    for (u32 i = 0; i < source.count_; ++i) {
      const u32 node = source.state_.live[source.tasks_[i]].node;
      if (state.slots[node] >= state.count || !same_sites(source.task(i), state.live[state.slots[node]].ready))
        return fail(Reason::catalogue_invariant);
    }
    return {};
  }
};

}  // namespace mhgp11::catalogue_detail::adaptive_detail

namespace mhgp11::catalogue_detail {

Outcome AdaptiveFrontier::verify(Run& run, sched::Pool& pool) const noexcept {
  if (!matches(run) || run.ledger != CatalogueLedger{}) return fail(Reason::catalogue_invariant);
  return adaptive_detail::Replay{*this, run, pool}.replay();
}

}  // namespace mhgp11::catalogue_detail
