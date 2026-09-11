// Test-only scalar geometry reference, mechanically extracted from header6763.
// No Builder, anchors, temporal history or DSU reference is accessible here.
#pragma once
#include "../src/forest/full_ball_tower.hpp"
namespace mhgp7::batch_test {
using namespace mhgp7;
using full_ball_detail::add;
using full_ball_detail::require;
using BallId = u32;
using ResolverCache = full_ball_detail::ResolverCache;
using StaticSeed = FullBallBatchSeed;
struct Geometry {
  const CloudIndex& ix;
  std::span<const BallData> balls;
  std::span<const BallId> by_key;
  unsigned current_k;
  AnchorMebResult meb(std::span<const i32> sites, AnchorMebWork& work) const {
    std::array<P3, kFacetMaxK> positions{};
    require(!sites.empty() && sites.size() <= positions.size(), "full_ball_meb_cardinality");
    for (size_t j = 0; j < sites.size(); ++j) positions[j] = ix.upos[sites[j]];
    auto result = anchor_meb(std::span<const P3>(positions.data(), sites.size()), work);
    require(result.status == AnchorMebStatus::kOk, "full_ball_meb_failure",
        result.status == AnchorMebStatus::kCounterOverflow ? FullBallStatus::kResourceExhausted
                                                         : FullBallStatus::kInvariantViolated);
    return result;
  }

  i32 intruder_work(const BallKey& key, std::span<const i32> selected,
      FullBallStats& work, std::vector<NodeRef>& scratch) const {
    add(work.intruder_queries);
    const auto member = [&](i32 u) { return std::binary_search(selected.begin(), selected.end(), u); };
    const census_detail::AxisBounds bounds(key);
    scratch.clear(); scratch.push_back(ix.root());
    while (!scratch.empty()) {
      const auto node = scratch.back(); scratch.pop_back(); add(work.intruder_nodes);
      i128 lo, hi; bounds.bounds(ix.box_of(node), &lo, &hi);
      if (lo >= 0) continue;
      if (hi < 0) {
        add(work.interior_ranges);
        const auto range = ix.range_of(node);
        for (i32 u = range.first; u <= range.last; ++u) if (!member(u)) return u;
      } else if (is_leaf(node)) {
        const i32 u = leaf_index(node);
        if (!member(u)) {
          add(work.intruder_power_tests);
          if (key.power(ix.upos[u]) < 0) return u;
        }
      } else {
        scratch.push_back(ix.nodes[node].right); scratch.push_back(ix.nodes[node].left);
      }
    }
    return -1;
  }
  // Geometry-only: immutable index/catalogue and rank, never anchors or DSU.
  BallId static_terminal(ResolverCache::Key key, const ExactLevel& before,
      FullBallStats& work, std::vector<NodeRef>& scratch, std::span<const StaticSeed> seeds) const {
    std::span<i32> sites(key.data(), current_k);
    auto local = meb(sites, work.resolve_work);
    u64 length = 0;
    for (;;) {
      require(compare_exact_level(local.level, before) < 0, "full_ball_static_not_strict");
      add(work.key_lookups);
      const auto found = std::lower_bound(by_key.begin(), by_key.end(), local.key,
          [&](BallId id, const BallKey& value) { return balls[id].key < value; });
      if (found != by_key.end() && balls[*found].key == local.key) {
        const auto& b = balls[*found];
        require(same_exact_level(b.level, local.level), "full_ball_static_anchor_level");
        const unsigned lo = b.n_interior + b.arity - 1;
        if (current_k >= lo && current_k <= b.n_interior + b.n_shell) {
          add(work.anchor_hits); work.max_chain_steps = std::max(work.max_chain_steps, length);
          return *found;
        }
      }
      const i32 z = intruder_work(local.key, sites, work, scratch);
      require(z >= 0, "full_ball_static_missing_weak_terminal");
      require(local.support_size > 0 && local.support_slots[0] < sites.size(),
              "full_ball_static_missing_support");
      sites[local.support_slots[0]] = z;
      std::sort(sites.begin(), sites.end());
      add(work.static_post_seed_queries[current_k]);
      const auto seed = std::lower_bound(seeds.begin(), seeds.end(), key,
          [](const StaticSeed& a, const ResolverCache::Key& value) { return a.key < value; });
      if (seed != seeds.end() && seed->key == key) {
        const auto& b = balls[seed->ball];
        // Whole I union U, in this Builder's index and K. A surviving support
        // or partial shell is NOT a seed: it can preserve the previous radius.
        require(current_k == static_cast<unsigned>(b.n_interior) + b.n_shell,
                "full_ball_post_seed_whole_population");
        require(compare_exact_level(b.level, local.level) < 0 && compare_exact_level(b.level, before) < 0,
                "full_ball_post_seed_not_strict");
        add(work.static_post_seed_hits[current_k]);
        add(work.descending_steps); add(length);
        work.max_chain_steps = std::max(work.max_chain_steps, length);
        add(work.static_post_seed_terminals[current_k]);
        // Exact full-population MEB is this ball; the original next iteration
        // would terminate here. Do not charge its MEB/lookup/anchor hit.
        return seed->ball;
      }
      auto next = meb(sites, work.resolve_work);
      const int cmp = compare_exact_level(next.level, local.level);
      require(cmp <= 0, "full_ball_static_radius_increased");
      if (cmp == 0) {
        require(next.key == local.key && next.selected_shell_count + 1 == local.selected_shell_count,
                "full_ball_static_equal_radius_shell_not_decreased");
        add(work.same_radius_steps);
      } else add(work.descending_steps);
      add(length); local = next;
    }
  }

};
struct Merger {
  FullBallStats& st;
  void merge_static_work(const FullBallStats& w) {
    for (size_t k = 0; k < st.static_post_seed_queries.size(); ++k) {
      add(st.static_post_seed_queries[k], w.static_post_seed_queries[k]);
      add(st.static_post_seed_hits[k], w.static_post_seed_hits[k]);
      add(st.static_post_seed_terminals[k], w.static_post_seed_terminals[k]);
    }
    for (const auto field : {&FullBallStats::anchor_hits, &FullBallStats::key_lookups,
        &FullBallStats::intruder_queries, &FullBallStats::intruder_nodes,
        &FullBallStats::intruder_power_tests, &FullBallStats::interior_ranges,
        &FullBallStats::same_radius_steps, &FullBallStats::descending_steps}) add(st.*field, w.*field);
    st.max_chain_steps = std::max(st.max_chain_steps, w.max_chain_steps);
    add(st.resolve_work.calls, w.resolve_work.calls);
    add(st.resolve_work.power_tests, w.resolve_work.power_tests);
    add(st.resolve_work.materializations, w.resolve_work.materializations);
    for (size_t q = 0; q < st.resolve_work.supports_by_size.size(); ++q)
      add(st.resolve_work.supports_by_size[q], w.resolve_work.supports_by_size[q]);
  }

};
}  // namespace mhgp7::batch_test
