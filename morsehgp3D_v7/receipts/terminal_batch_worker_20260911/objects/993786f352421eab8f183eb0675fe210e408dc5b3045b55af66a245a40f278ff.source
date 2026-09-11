#pragma once

#include "batch_route.cuh"
#include "source/morsehgp3D_v7/src/forest/full_ball_tower.hpp"

namespace mhgp7::gpu_terminal_batch_private {
inline void accumulate(FullBallStats& out, const terminal::Work& w, unsigned k) {
  using full_ball_detail::add;
  add(out.resolve_work.calls, w.calls);
  add(out.resolve_work.materializations, w.materializations);
  add(out.resolve_work.power_tests, w.powers);
  for (size_t q = 0; q < 5; ++q) add(out.resolve_work.supports_by_size[q], w.supports[q]);
  add(out.key_lookups, w.key_lookups); add(out.anchor_hits, w.anchor_hits);
  add(out.intruder_queries, w.intruder_queries); add(out.intruder_nodes, w.intruder_nodes);
  add(out.intruder_power_tests, w.intruder_power_tests); add(out.interior_ranges, w.interior_ranges);
  add(out.same_radius_steps, w.same_radius_steps); add(out.descending_steps, w.descending_steps);
  out.max_chain_steps = std::max(out.max_chain_steps, w.max_chain_steps);
  add(out.static_post_seed_queries[k], w.post_seed_queries);
  add(out.static_post_seed_hits[k], w.post_seed_hits);
  add(out.static_post_seed_terminals[k], w.post_seed_terminals);
}
// Publish only a complete, representable aggregate. If any checked addition
// throws, the caller must not mistake the partial sum for known paid work.
inline void publish_work(std::span<const WireResult> rows, unsigned k, FullBallBatchResult& output) {
  output.work_known = false;
  FullBallStats aggregate;
  for (const auto& row : rows) accumulate(aggregate, row.result.work, k);
  output.work = std::move(aggregate);
  output.work_known = true;
}
inline void resolve_batch(void* opaque, const FullBallGeometryView& geometry,
    const FullBallBatchView& batch, FullBallBatchResult& output) {
  output.work_known = true;
  if (!opaque || batch.k < 2 || batch.k > 10) {
    output.reason = "gpu_batch_invalid_binding"; return;
  }
  auto& context = *static_cast<Context*>(opaque);
  if (!context.bound_to(geometry.index, geometry.balls)) {
    output.reason = "gpu_batch_foreign_geometry"; return;
  }
  const auto seeds = context.seeds(batch.k);
  if (seeds.count != batch.seeds.size()) { output.reason = "gpu_batch_seed_count"; return; }
  for (size_t j = 0; j < batch.seeds.size(); ++j)
    if (batch.seeds[j].ball != seeds.entries[j].target ||
        terminal::compare_seed(batch.seeds[j].key.data(), seeds.entries[j].selected) != 0) {
      output.reason = "gpu_batch_seed_identity"; return;
    }
  std::vector<terminal::Request> requests;
  requests.reserve(batch.requests.size());
  for (const auto& source : batch.requests) {
    if (source.consumer >= geometry.balls.size()) { output.reason = "gpu_batch_consumer"; return; }
    terminal::Request request;
    request.snapshot = context.snapshot(); request.k = batch.k; request.ordinal = source.ordinal;
    request.before = terminal::encode_level(geometry.balls[source.consumer].level);
    std::copy(source.key.begin(), source.key.end(), request.selected);
    requests.push_back(request);
  }
  output.work_known = false;
  const auto result = context.execute(requests);
  if (result.raw_work_known) publish_work(result.diagnostic, batch.k, output);
  // Device buffers are reused; sampled capacity only, not peak VRAM/RSS.
  if (context.capacity() > std::numeric_limits<u64>::max() /
      (sizeof(terminal::Request) + sizeof(WireResult))) throw std::overflow_error("batch_retained_bytes");
  output.retained_bytes = context.capacity() * (sizeof(terminal::Request) + sizeof(WireResult));
  if (!result.complete) { output.reason = result.failure; return; }
  std::vector<FullBallBatchTarget> accepted;
  accepted.reserve(result.accepted.size());
  for (const auto& row : result.accepted) accepted.push_back({row.target, row.ordinal});
  output.targets = std::move(accepted);
  output.status = FullBallStatus::kCompleteRelative; output.reason = "ok";
}
inline FullBallBatchResolver resolver(Context& context) { return {&context, resolve_batch}; }
}  // namespace mhgp7::gpu_terminal_batch_private
