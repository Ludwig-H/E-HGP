// Private test-only hooks. This header is never included by nominal sources.
#pragma once

#include <cstdio>
#include <stdexcept>
#include <string>

#include "morsehgp3D_v7/src/pipeline/expand.hpp"

namespace named_block_gate {
using namespace mhgp7;
inline constexpr u64 absent = std::numeric_limits<u64>::max();
inline constexpr const char* input_sha = "3f7c6dd47bcba4222e511c94f90aaeeeb80198b0d5ac8a6721e4ff55feedab3f";
inline bool raw_anchor_mutant = false;
inline void need(bool good, const char* why) { if (!good) throw std::runtime_error(why); }

struct Expectation {
  u64 capture_label;
  unsigned k;
  BallKey key;
  ExactLevel level;
  size_t roots;
  u16 mask;
  bool interior;
  std::vector<PointId> interior_ids, shell_ids;
};
inline std::vector<Expectation> real_expectations() {
  return {
    {174406, 5, {1, {-117436,-63637,-105474}, 7237823525LL}, {{14352441,0,0},4},
        1, 0, false, {12055,27569,46679}, {36860,42779,46707}},
    {254569, 2, {1, {-111935,-55901,-93422}, 6094057006LL}, {{2904043,0,0},2},
        2, 0, false, {}, {4912,32276,34292}},
    {996863, 6, {1, {-60899,-109188,-104077}, 6612344594LL}, {{6675549,0,0},2},
        2, 0, false, {23184,23681,34559,42468}, {25389,40661,43571}},
    {1251653, 10, {1, {-43377,-128636,-55885}, 5381668141LL}, {{12622643,0,0},2},
        1, 0, false, {8622,9003,13287,14710,16188,49148,49421,49620,49945}, {27084,44066,48544}}
  };
}
struct Observation {
  size_t before = 0, after = 0;
  u64 catalogue_index = absent, prior_nodes = 0, anchor = absent;
  std::vector<u64> roots;
  u16 mask = 0;
  bool interior = false;
};
struct Observer {
  const CloudIndex& index;
  std::vector<Expectation> expected;
  std::vector<Observation> observed;
  bool inspect_every_root = false;
  u64 lot_before = 0, lot_after = 0, root_checks = 0;
  Observer(const CloudIndex& ix, std::vector<Expectation> targets, bool all = false)
      : index(ix), expected(std::move(targets)), observed(expected.size()), inspect_every_root(all) {}

  template<class History>
  void roots_are_strict_live(std::span<const u64> roots, const ExactLevel& cut, const History& history) {
    for (size_t j = 0; j < roots.size(); ++j) {
      need((j == 0 || roots[j-1] < roots[j]) && roots[j] < history.next.size(), "named.pre_root_sorted_and_prior");
      need(history.next[roots[j]] == absent, "named.pre_root_not_live");
      need(compare_exact_level(history.levels[roots[j]], cut) < 0, "named.pre_root_not_strict");
      ++root_checks;
    }
  }
  void census(const BallData& ball, const Expectation& want) const {
    const auto ids = [&](std::span<const i32> sites) {
      std::vector<PointId> result;
      for (i32 site : sites) result.push_back(index.point_id(site));
      std::sort(result.begin(), result.end());
      return result;
    };
    need(same_exact_level(ball.level, want.level), "named.exact_level");
    need(ids(ball.interior()) == want.interior_ids && ids(ball.shell()) == want.shell_ids,
        "named.complete_census_identity");
  }
  template<class Tower>
  void finish(const Tower& tower) const {
    need(lot_before > 0 && lot_before == lot_after, "named.whole_lot_observation_balance");
    for (size_t j = 0; j < expected.size(); ++j) {
      const auto& want = expected[j]; const auto& got = observed[j];
      need(got.before == 1 && got.after == 1, "named.block_not_observed_exactly_once");
      need(got.roots.size() == want.roots, "named.global_parent_cardinality");
      need(got.mask == want.mask && got.interior == want.interior, "named.contribution");
      need(want.k <= tower.orders.size(), "named.order_absent");
      const auto& forest = tower.orders[want.k-1].forest;
      need(got.anchor != absent && full_coverage_root_at(forest, got.anchor, want.level, true) == got.anchor,
          "named.anchor_not_live_at_closed_cut");
      for (u64 root : got.roots)
        need(root != absent && full_coverage_root_at(forest, root, want.level, false) == root,
            "named.pre_root_not_live_at_open_cut");
    }
  }
};
inline Observer* active = nullptr;
struct Scope {
  explicit Scope(Observer& observer) { need(active == nullptr, "named.nested_observer"); active = &observer; }
  ~Scope() { active = nullptr; raw_anchor_mutant = false; }
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
};

template<class Blocks, class Balls, class History>
void before(unsigned k, const Blocks& blocks, const Balls& balls, const History& history) {
  if (!active) return;
  auto& observer = *active; ++observer.lot_before;
  for (const auto& block : blocks) {
    const auto& ball = balls[block.ball];
    if (observer.inspect_every_root) observer.roots_are_strict_live(block.roots, ball.level, history);
    for (size_t j = 0; j < observer.expected.size(); ++j) {
      const auto& want = observer.expected[j];
      if (want.k != k || !(want.key == ball.key)) continue;
      auto& got = observer.observed[j];
      need(got.before++ == 0, "named.duplicate_pre_observation");
      observer.census(ball, want);
      observer.roots_are_strict_live(block.roots, ball.level, history);
      got.catalogue_index = block.ball; got.prior_nodes = history.next.size();
      got.roots = block.roots; got.mask = block.contribution; got.interior = block.interior;
      need(got.roots.size() == want.roots, "named.global_parent_cardinality");
      need(got.mask == want.mask && got.interior == want.interior, "named.contribution");
    }
  }
}
template<class Blocks, class Balls, class Anchors, class History>
void after(unsigned k, const Blocks& blocks, const Balls& balls, const Anchors& anchors, const History& history) {
  if (!active) return;
  auto& observer = *active; ++observer.lot_after;
  // This check is AFTER close_lot: every member's anchor must already exist.
  for (const auto& block : blocks) {
    need(anchors[block.ball] != absent, "named.whole_lot_anchor_missing");
    for (size_t j = 0; j < observer.expected.size(); ++j) {
      const auto& want = observer.expected[j];
      if (want.k != k || !(want.key == balls[block.ball].key)) continue;
      auto& got = observer.observed[j];
      need(got.before == 1 && got.after++ == 0, "named.post_without_unique_pre");
      got.anchor = anchors[block.ball];
      need(got.anchor < history.next.size() && history.next[got.anchor] == absent,
          "named.post_anchor_not_live");
      need(compare_exact_level(history.levels[got.anchor], want.level) <= 0, "named.post_anchor_future");
    }
  }
}

inline void print_ids(std::span<const u64> ids) {
  std::printf("[");
  for (size_t j = 0; j < ids.size(); ++j) std::printf("%s%llu", j ? "," : "", static_cast<unsigned long long>(ids[j]));
  std::printf("]");
}
inline void print_point_ids(std::span<const PointId> ids) {
  std::printf("[");
  for (size_t j = 0; j < ids.size(); ++j) std::printf("%s%u", j ? "," : "", ids[j]);
  std::printf("]");
}
inline void print_definition(const Expectation& row) {
  std::printf("{\"capture_label\":%llu,\"k\":%u,\"ball_key\":{\"a\":\"%lld\",\"b\":[\"%lld\",\"%lld\",\"%lld\"],\"c\":\"%lld\"},"
      "\"level_num\":%llu,\"level_den\":%lld,\"expected_roots\":%zu,\"expected_contribution_mask\":%u,"
      "\"expected_contribution_interior\":%s,\"interior\":",
      static_cast<unsigned long long>(row.capture_label), row.k, static_cast<long long>(row.key.a),
      static_cast<long long>(row.key.b[0]), static_cast<long long>(row.key.b[1]), static_cast<long long>(row.key.b[2]),
      static_cast<long long>(row.key.c), static_cast<unsigned long long>(row.level.num[0]), static_cast<long long>(row.level.den),
      row.roots, row.mask, row.interior ? "true" : "false");
  print_point_ids(row.interior_ids); std::printf(",\"shell\":"); print_point_ids(row.shell_ids); std::printf("}");
}
inline void print_definitions() {
  const auto rows = real_expectations(); std::printf("[");
  for (size_t j = 0; j < rows.size(); ++j) { if (j) std::printf(","); print_definition(rows[j]); }
  std::printf("]\n");
}
}  // namespace named_block_gate
