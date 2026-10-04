// Outils prives du plan adaptatif : une ronde conserve TOUS les parents jusqu'au join du Pool.
#pragma once
#include "catalogue/adaptive_frontier.hpp"
#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail::adaptive_detail {

struct ChildResult { ReadyNode ready; CatalogueLedger ledger{}; };
using Children = std::array<ChildResult, kAdaptiveTasks>;

Outcome prepare_root(Run& run, ReadyNode& ready) noexcept;
Outcome state_bytes(const State& state, u64& bytes) noexcept;
Outcome round_bytes(const State& state, std::span<const u32> parents, u64& bytes) noexcept;
Outcome run_round(Run& run, sched::Pool& pool, const State& state,
                  std::span<const u32> parents, Children& children) noexcept;
Outcome publish_round(State& state, std::span<const u32> parents,
                      std::span<const PlanNode> plan, Children& children) noexcept;
bool same_ready(const PlanNode& expected, const ReadyNode& ready) noexcept;
bool same_sites(const ReadyNode& left, const ReadyNode& right) noexcept;

}  // namespace mhgp11::catalogue_detail::adaptive_detail
