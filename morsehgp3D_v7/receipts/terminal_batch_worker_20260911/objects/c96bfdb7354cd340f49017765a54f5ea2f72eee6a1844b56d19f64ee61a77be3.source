#pragma once

// HOST JUDGE ONLY. c03 static_terminal control flow, anchor_meb and the original
// CPU AxisBounds BVH traversal. No device helper is invoked by this reference.
#include "../terminal_bridge.hpp"
#include "../source/morsehgp3D_v7/src/pipeline/census.hpp"

namespace mhgp7::terminal_cuda_gate {
namespace terminal = gpu_terminal_private;
inline void reference_need(bool good, const char* why) {
  if (!good) throw std::runtime_error(why);
}
struct CpuTerminal {
  const CloudIndex& ix;
  std::span<const BallData> balls;
  std::vector<terminal::BallId> by_key;
  CpuTerminal(const CloudIndex& index, std::span<const BallData> catalogue)
      : ix(index), balls(catalogue), by_key(balls.size()) {
    std::iota(by_key.begin(), by_key.end(), terminal::BallId{0});
    std::sort(by_key.begin(), by_key.end(), [&](auto a, auto b) { return balls[a].key < balls[b].key; });
  }
  i32 intruder(const BallKey& key, std::span<const i32> sites, terminal::Work& work) const {
    ++work.intruder_queries;
    work.axis_divisions += 3;  // three original exact CPU AxisBounds divisions
    const census_detail::AxisBounds bounds(key);
    const auto member = [&](i32 u) { return std::binary_search(sites.begin(), sites.end(), u); };
    std::vector<NodeRef> scratch{ix.root()};
    work.stack_peak = std::max(work.stack_peak, u32{1});
    while (!scratch.empty()) {
      const auto node = scratch.back(); scratch.pop_back(); ++work.intruder_nodes;
      i128 lo, hi; bounds.bounds(ix.box_of(node), &lo, &hi);
      if (lo >= 0) continue;
      if (hi < 0) {
        ++work.interior_ranges;
        const auto range = ix.range_of(node);
        for (i32 u = range.first; u <= range.last; ++u) if (!member(u)) return u;
      } else if (is_leaf(node)) {
        const i32 u = leaf_index(node);
        if (!member(u)) {
          ++work.intruder_power_tests;
          if (key.power(ix.upos[u]) < 0) return u;
        }
      } else {
        scratch.push_back(ix.nodes[node].right); scratch.push_back(ix.nodes[node].left);
        work.stack_peak = std::max(work.stack_peak, static_cast<u32>(scratch.size()));
      }
    }
    return -1;
  }
  terminal::Result resolve(const terminal::Request& request, std::vector<terminal::TraceRow>& trace) const {
    reference_need(request.k >= 2 && request.k <= 10, "reference.request_domain");
    std::vector<i32> sites(request.selected, request.selected + request.k);
    reference_need(std::is_sorted(sites.begin(), sites.end()) &&
        std::adjacent_find(sites.begin(), sites.end()) == sites.end(), "reference.sites_order");
    terminal::Result result; result.ordinal = request.ordinal;
    AnchorMebWork paid;
    const auto meb = [&]() {
      std::array<P3, 10> points{};
      for (u32 i = 0; i < request.k; ++i) points[i] = ix.upos.at(static_cast<size_t>(sites[i]));
      const auto local = anchor_meb(std::span<const P3>(points.data(), request.k), paid);
      reference_need(local.status == AnchorMebStatus::kOk, "reference.meb_failed");
      return local;
    };
    AnchorMebWork previous;
    auto local = meb();
    auto local_paid = terminal::delta(paid, previous);
    u64 length = 0;
    for (;;) {
      reference_need(compare_exact_level(local.level, terminal::decode_level(request.before)) < 0,
          "reference.not_strict");
      ++result.work.key_lookups;
      const auto found = std::lower_bound(by_key.begin(), by_key.end(), local.key,
          [&](auto id, const BallKey& value) { return balls[id].key < value; });
      if (found != by_key.end() && balls[*found].key == local.key) {
        const auto& ball = balls[*found];
        reference_need(same_exact_level(ball.level, local.level), "reference.catalogue_level");
        if (request.k >= static_cast<u32>(ball.n_interior + ball.arity - 1) &&
            request.k <= static_cast<u32>(ball.n_interior + ball.n_shell)) {
          ++result.work.anchor_hits; result.work.max_chain_steps = length;
          result.target = *found; result.status = terminal::Status::kOk;
          terminal::note_cpu(&trace, sites, local, local_paid, -1, *found);
          break;
        }
      }
      const auto z = intruder(local.key, sites, result.work);
      reference_need(z >= 0, "reference.missing_weak_terminal");
      reference_need(local.support_size && local.support_slots[0] < sites.size(), "reference.missing_support");
      terminal::note_cpu(&trace, sites, local, local_paid, z, terminal::kAbsentBall);
      sites[local.support_slots[0]] = z;
      std::sort(sites.begin(), sites.end());
      previous = paid;
      const auto next = meb();
      local_paid = terminal::delta(paid, previous);
      const auto comparison = compare_exact_level(next.level, local.level);
      reference_need(comparison <= 0, "reference.radius_increased");
      if (!comparison) {
        reference_need(next.key == local.key && next.selected_shell_count + 1 == local.selected_shell_count,
            "reference.equal_radius_invariant");
        ++result.work.same_radius_steps;
      } else ++result.work.descending_steps;
      ++length; local = next;
    }
    result.work.calls = paid.calls; result.work.powers = paid.power_tests;
    result.work.materializations = paid.materializations;
    result.work.level_materializations = paid.materializations; // reference has one level per MEB
    for (u8 q = 0; q < 5; ++q) result.work.supports[q] = paid.supports_by_size[q];
    return result;
  }
};
}  // namespace mhgp7::terminal_cuda_gate
