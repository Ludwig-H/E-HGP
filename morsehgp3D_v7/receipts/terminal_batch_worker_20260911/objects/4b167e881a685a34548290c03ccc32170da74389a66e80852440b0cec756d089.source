#pragma once

// Private paired test bridge. All comparisons/reference traces live on host;
// the geometry helper itself neither invokes this bridge nor the CPU resolver.
#include <atomic>
#include <memory>
#include "terminal_owner.hpp"

namespace mhgp7::gpu_terminal_private {
struct ProofStats {
  std::atomic<u64> calls{0}, traces{0}, key_words{0}, semantically_equal_levels{0};
  std::atomic<u64> q[5]{}, extra_shells{0}, strict_steps{0}, same_steps{0};
  std::atomic<u64> empty_trace_checks{0};
};
inline ProofStats proof;

inline AnchorMebWork delta(const AnchorMebWork& after, const AnchorMebWork& before) {
  AnchorMebWork result;
  result.calls = after.calls - before.calls;
  result.power_tests = after.power_tests - before.power_tests;
  result.materializations = after.materializations - before.materializations;
  for (u8 q = 0; q < 5; ++q) result.supports_by_size[q] = after.supports_by_size[q] - before.supports_by_size[q];
  return result;
}
inline void note_cpu(std::vector<TraceRow>* trace, std::span<const i32> sites,
    const AnchorMebResult& local, const AnchorMebWork& paid, i32 intruder, BallId terminal) {
  if (!trace) return;
  TraceRow row;
  row.k = static_cast<u32>(sites.size());
  for (u8 i = 0; i < row.k; ++i) row.sites[i] = sites[i];
  row.selection.ordinal = 0; row.selection.status = meb_selection::Status::kOk;
  row.selection.q = local.support_size; row.selection.shell = local.selected_shell_count;
  row.selection.calls = paid.calls; row.selection.powers = paid.power_tests;
  for (u8 i = 0; i < 4; ++i) row.selection.slots[i] = local.support_slots[i];
  for (u8 q = 0; q < 5; ++q) row.selection.supports[q] = paid.supports_by_size[q];
  row.key = meb_key::encode_key(local.key); row.level = encode_level(local.level);
  row.intruder = intruder; row.terminal = terminal;
  trace->push_back(row);
}
inline bool same_trace(const TraceRow& observed, const TraceRow& expected) {
  if (observed.k != expected.k || observed.intruder != expected.intruder || observed.terminal != expected.terminal ||
      observed.selection.ordinal != expected.selection.ordinal || observed.selection.status != expected.selection.status ||
      observed.selection.q != expected.selection.q || observed.selection.shell != expected.selection.shell ||
      observed.selection.calls != expected.selection.calls || observed.selection.powers != expected.selection.powers ||
      compare_keys(observed.key, expected.key) != 0 ||
      !same_exact_level(decode_level(observed.level), decode_level(expected.level))) return false;
  for (u8 i = 0; i < 10; ++i) if (observed.sites[i] != expected.sites[i]) return false;
  for (u8 i = 0; i < 4; ++i) if (observed.selection.slots[i] != expected.selection.slots[i]) return false;
  for (u8 q = 0; q < 5; ++q) if (observed.selection.supports[q] != expected.selection.supports[q]) return false;
  return true;
}
template <class Stats>
bool same_work(const Work& observed, const Stats& expected) {
  if (observed.calls != expected.resolve_work.calls || observed.powers != expected.resolve_work.power_tests ||
      observed.materializations != expected.resolve_work.materializations || observed.level_materializations != observed.materializations ||
      observed.key_lookups != expected.key_lookups || observed.anchor_hits != expected.anchor_hits ||
      observed.intruder_queries != expected.intruder_queries || observed.intruder_nodes != expected.intruder_nodes ||
      observed.intruder_power_tests != expected.intruder_power_tests || observed.interior_ranges != expected.interior_ranges ||
      observed.same_radius_steps != expected.same_radius_steps || observed.descending_steps != expected.descending_steps ||
      observed.max_chain_steps != expected.max_chain_steps || observed.axis_divisions != 3 * observed.intruder_queries) return false;
  for (u8 q = 0; q < 5; ++q) if (observed.supports[q] != expected.resolve_work.supports_by_size[q]) return false;
  return true;
}
template <class Stats, class Reference>
Result paired(const HostOwner& owner, std::span<const i32> sites, const ExactLevel& before,
    Reference&& reference) {
  Stats reference_work;
  std::vector<NodeRef> scratch;
  std::vector<TraceRow> expected;
  const BallId target = reference(reference_work, scratch, &expected);
  Request request;
  request.snapshot = owner.view().index.snapshot;
  request.k = static_cast<u32>(sites.size()); request.before = encode_level(before);
  for (u8 i = 0; i < request.k; ++i) request.selected[i] = sites[i];
  std::vector<TraceRow> rows(expected.size());
  TraceBuffer trace{rows.data(), rows.size(), 0, false};
  auto result = resolve(owner.view(), request, &trace);
  if (result.status != Status::kOk || result.target != target || !same_work(result.work, reference_work) ||
      trace.overflow || trace.written != expected.size()) { result.status = Status::kMebFailure; result.target = kAbsentBall; return result; }
  for (size_t i = 0; i < rows.size(); ++i) {
    if (!same_trace(rows[i], expected[i])) { result.status = Status::kMebFailure; result.target = kAbsentBall; return result; }
    ++proof.q[rows[i].selection.q];
    if (rows[i].selection.shell > rows[i].selection.q) ++proof.extra_shells;
  }
  // A diagnostic buffer of capacity zero may lose trace rows, NEVER work or
  // the terminal. This is a causal test that the diagnostic bound is no quota.
  TraceBuffer no_storage;
  const auto without_trace = resolve(owner.view(), request, &no_storage);
  if (without_trace.status != Status::kOk || without_trace.target != target ||
      !same_work(without_trace.work, reference_work) || !no_storage.overflow || no_storage.written != expected.size()) {
    result.status = Status::kMebFailure; result.target = kAbsentBall; return result;
  }
  ++proof.empty_trace_checks; ++proof.calls;
  proof.traces += rows.size(); proof.key_words += 10 * rows.size(); proof.semantically_equal_levels += rows.size();
  proof.strict_steps += result.work.descending_steps; proof.same_steps += result.work.same_radius_steps;
  return result;
}
}  // namespace mhgp7::gpu_terminal_private
