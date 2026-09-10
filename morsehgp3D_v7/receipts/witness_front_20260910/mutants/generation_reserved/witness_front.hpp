// Optional batched front; the existing scalar front remains the default.
// Only witness queries are delegated. Geometry, stable wave order and the
// pair-mass ledger stay on the host. No pair or visited-node matrix exists.
#pragma once

#include "generate.hpp"
#include "../spindle/witness_batch.hpp"

namespace mhgp7 {

struct WitnessFrontWork {
  u64 batches = 0, requests = 0, corner_requests = 0, peak_batch = 0;
  u64 peak_wave_tasks = 0;
};

class CpuWitnessBatch {
 public:
  CpuWitnessBatch(const CloudIndex& index, int workers) : ix(index), threads(workers) {}
  // This CPU provider borrows this exact immutable index, not merely matching
  // NodeRef ranges. A CUDA adapter must instead match its prepared wire digest.
  bool matches_index(const CloudIndex& index) const noexcept { return &index == &ix; }
  size_t operator()(std::span<const WitnessBatchRequest> requests, const u64 h[3],
                    u64 generation, std::vector<WitnessBatchResult>* out) const {
    out->clear();
    std::vector<WitnessBatchResult> draft(requests.size());
    const size_t workers = parallel_ranges(requests.size(), threads, [&](size_t begin, size_t end, size_t) {
      for (size_t i = begin; i < end; ++i) {
        const auto& r = requests[i];
        draft[i] = {count_universal_witnesses(ix, r.a, r.b, h, r.mask, r.corners),
                    r.request_id, generation, WitnessBatchStatus::kOk};
      }
    });
    validate_witness_batch(requests, h, generation, draft);
    out->swap(draft);
    return workers;
  }
 private:
  const CloudIndex& ix;
  int threads;
};

// Run.matches_index(ix) must bind the backend to this immutable index before
// any request is submitted. A correctly shaped result from another cloud is
// not a witness. Run(requests,h,generation,out) is synchronous and returns the observed
// host worker count (one for a serial host/CUDA adapter). generation belongs
// to the caller and persists across fronts sharing a device context.
// Shape/association failure throws before publishing ANY front output.
// A capacity refusal also publishes no prefix, unlike the legacy internal
// front. It remains a declared refusal, never an incomplete success.
template<class Run>
inline void alive_rectangles_batched(const CloudIndex& ix, i64 s, const u64 h[3], u8 initial_mask,
                                     std::vector<MultiAliveRect>* out, GenerateStats* st,
                                     size_t batch_size, u64& generation, WitnessFrontWork* work,
                                     Run&& run, u64 wave_cap = kMaxWaveTasks,
                                     u64 alive_cap = kMaxAliveRects) {
  out->clear();
  if (!batch_size || s < kSeparationProfileMin || (initial_mask & ~u8{7}))
    throw std::invalid_argument("witness_front_options");
  if (!ix.valid || !run.matches_index(ix))
    throw std::invalid_argument("witness_front_index_binding");
  if (ix.nodes.empty()) { *work = {}; return; }
  struct Task { WspdRect r; u8 mask; };
  struct Decision { u8 mask = 0; bool terminal = false; size_t corner_slot = 0; };
  GenerateStats delta;
  WitnessFrontWork measured;
  std::vector<MultiAliveRect> output;
  std::vector<Task> wave, next;
  u64 next_request = 0;
  const auto mass = [&](const WspdRect& r) {
    return static_cast<u128>(ix.node_weight(r.a)) * ix.node_weight(r.b);
  };
  const auto refuse = [&](u64 code) {
    st->cap_refus = code;
    st->wave_peak_tasks = std::max(st->wave_peak_tasks, delta.wave_peak_tasks);
    st->alive_peak_rects = std::max(st->alive_peak_rects, delta.alive_peak_rects);
    *work = measured;
  };
  if (ix.nodes.size() > wave_cap) { refuse(kCapRefusWaveTasks); return; }
  wave.reserve(ix.nodes.size());
  for (const auto& node : ix.nodes) wave.push_back({{node.left, node.right}, initial_mask});
  // Keep the legacy GenerateStats counter for paired comparisons, but also
  // report the actual initial/subsequent wave peak in the new work schema.
  measured.peak_wave_tasks = wave.size();
  std::vector<WitnessBatchRequest> requests, corners;
  std::vector<WitnessBatchResult> first, second;
  std::vector<Decision> decisions;
  const auto request = [&](const Task& task, u8 mask, bool with_corners) {
    if (next_request == ~u64{0}) throw std::overflow_error("witness_front_request_id");
    return WitnessBatchRequest{task.r.a, task.r.b, mask, with_corners, next_request++};
  };
  const auto query = [&](std::span<const WitnessBatchRequest> input,
                         std::vector<WitnessBatchResult>* result) {
    if (input.empty()) { result->clear(); return; }
    if (generation == ~u64{0}) throw std::overflow_error("witness_front_generation");
    ++generation;
    result->clear();
    const size_t workers = run(input, h, generation, result);
    validate_witness_batch(input, h, generation, *result);
    delta.workers_wspd = std::max(delta.workers_wspd, static_cast<u64>(workers));
    ++measured.batches;
    measured.requests += input.size();
    measured.peak_batch = std::max(measured.peak_batch, static_cast<u64>(input.size()));
    for (const auto& row : input) measured.corner_requests += row.corners ? 1 : 0;
  };
  while (!wave.empty()) {
    next.clear();
    std::vector<MultiAliveRect> terminal_rows;
    GenerateStats wave_stats;
    for (size_t begin = 0; begin < wave.size();) {
      const size_t count = std::min(batch_size, wave.size() - begin);
      requests.clear(); corners.clear(); decisions.assign(count, {});
      for (size_t j = 0; j < count; ++j) requests.push_back(request(wave[begin + j], wave[begin + j].mask, false));
      query(requests, &first);
      for (size_t j = 0; j < count; ++j) {
        const Task& task = wave[begin + j];
        const auto& fc = first[j].counts;
        auto& decision = decisions[j];
        ++wave_stats.rect_visited_fused;
        wave_stats.wspd_witness_nodes += fc.nodes_visited;
        wave_stats.wspd_corner_evals += fc.corner_evals;
        decision.mask = task.mask;
        for (unsigned q = 0; q < 3; ++q) if ((decision.mask & (1u << q)) && fc.c[q] >= h[q]) {
          decision.mask &= static_cast<u8>(~(1u << q));
          wave_stats.ledger_killed_mass[q] += mass(task.r);
        }
        if (!decision.mask) continue;
        decision.terminal = wspd_detail::separated(ix.box_of(task.r.a), ix.box_of(task.r.b), s, 1);
        if (decision.terminal && (decision.mask & 0b110)) {
          decision.corner_slot = corners.size();
          corners.push_back(request(task, static_cast<u8>(decision.mask & 0b110), true));
        }
      }
      query(corners, &second);
      // Replay in original wave order; compacting the terminal query list
      // does not reorder rectangles or the two children of any split.
      for (size_t j = 0; j < count; ++j) {
        const Task& task = wave[begin + j];
        const auto& decision = decisions[j];
        const u8 mask = decision.mask;
        if (!mask) continue;
        if (decision.terminal) {
          FusedCounts fc;
          if (mask & 0b110) fc = second[decision.corner_slot].counts;
          if (mask & 1) fc.c[0] = first[j].counts.c[0];
          wave_stats.wspd_witness_nodes += fc.nodes_visited;
          wave_stats.wspd_corner_evals += fc.corner_evals;
          MultiAliveRect row; row.r = task.r;
          for (unsigned q = 0; q < 3; ++q) if (mask & (1u << q)) {
            if (fc.c[q] >= h[q]) { wave_stats.ledger_killed_mass[q] += mass(task.r); continue; }
            row.mask |= static_cast<u8>(1u << q); row.core[q] = fc.c[q];
            ++wave_stats.rect_alive[q]; wave_stats.ledger_emitted_mass[q] += mass(task.r);
          }
          if (row.mask) terminal_rows.push_back(row);
        } else {
          const bool split_a = task.r.a >= 0 && (task.r.b < 0 ||
              wspd_detail::box_w2(ix.box_of(task.r.a)) >= wspd_detail::box_w2(ix.box_of(task.r.b)));
          const NodeRef keep = split_a ? task.r.b : task.r.a;
          const auto& node = ix.nodes[static_cast<size_t>(split_a ? task.r.a : task.r.b)];
          next.push_back({split_a ? WspdRect{node.left, keep} : WspdRect{keep, node.left}, mask});
          next.push_back({split_a ? WspdRect{node.right, keep} : WspdRect{keep, node.right}, mask});
        }
      }
      begin += count;
    }
    // next is a private wave draft of at most 2*wave.size(), not a global
    // admitted queue. Batch storage remains O(batch_size), never O(visits).
    if (next.size() > wave_cap || terminal_rows.size() > alive_cap - output.size()) {
      delta.wave_peak_tasks = std::max(delta.wave_peak_tasks, static_cast<u64>(wave.size()));
      delta.alive_peak_rects = std::max(delta.alive_peak_rects, static_cast<u64>(output.size()));
      refuse(next.size() > wave_cap ? kCapRefusWaveTasks : kCapRefusAliveRects); return;
    }
    output.insert(output.end(), terminal_rows.begin(), terminal_rows.end());
    delta.add_from(wave_stats);
    delta.wave_peak_tasks = std::max(delta.wave_peak_tasks, static_cast<u64>(next.size()));
    measured.peak_wave_tasks = std::max(measured.peak_wave_tasks, static_cast<u64>(next.size()));
    delta.alive_peak_rects = std::max(delta.alive_peak_rects, static_cast<u64>(output.size()));
    wave.swap(next);
  }
  st->add_from(delta);
  st->workers_wspd = std::max(st->workers_wspd, delta.workers_wspd);
  st->wave_peak_tasks = std::max(st->wave_peak_tasks, delta.wave_peak_tasks);
  st->alive_peak_rects = std::max(st->alive_peak_rects, delta.alive_peak_rects);
  *work = measured;
  out->swap(output);
}

}  // namespace mhgp7
