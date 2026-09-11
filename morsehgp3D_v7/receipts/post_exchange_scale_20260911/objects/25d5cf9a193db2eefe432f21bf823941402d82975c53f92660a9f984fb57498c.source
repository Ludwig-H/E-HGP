// Physical-work checks reuse the nominal gate's independent rational census.
// Its main is not called; no oracle or geometry implementation is duplicated.
#define main mhgp7_embedded_full_ball_gate_main
#include "full_ball_tower_gate.cpp"
#undef main

namespace {
ExactLevel work_level(u64 value) { return {{value, 0, 0}, 1}; }
full_ball_detail::History comb_history(u64 count) {
  full_ball_detail::History history;
  history.levels.assign(2 * count + 1, work_level(0));
  history.next.assign(history.levels.size(), kFullCoverageAbsent);
  for (u64 j = 1; j <= count; ++j) {
    const u64 target = count + j;
    history.levels[target] = work_level(j);
    history.next[j] = target;
    history.next[j == 1 ? 0 : target - 1] = target;
  }
  return history;
}
void check_comb(u64 count) {
  const auto history = comb_history(count);
  const auto before = history;
  FullBallStats stats;
  full_ball_detail::MonotoneHistory cursor(history, stats);
  need(cursor.root_at(0, work_level(0), true) == 0, "comb.birth_initial_cut");
  u64 baseline_edges = 0;
  for (u64 cut = 1; cut <= count; ++cut) {
    u64 expected = 0;
    while (history.next[expected] != kFullCoverageAbsent &&
        full_coverage_detail::admitted(history.levels[history.next[expected]], work_level(cut), true)) {
      ++baseline_edges; expected = history.next[expected];
    }
    need(expected == count + cut, "comb.baseline_identity");
    need(history.root_at(0, work_level(cut), true) == expected, "comb.baseline_mirror_exact");
    need(cursor.root_at(0, work_level(cut), true) == expected, "comb.current_cut_not_future");
  }
  need(baseline_edges == count * (count + 1) / 2, "comb.quadratic_baseline_edge_count");
  need(stats.lower_nodes_activated == 2 * count + 1 && stats.lower_edges_indexed == 2 * count &&
      stats.lower_edges_activated == 2 * count && stats.lower_queries == count + 1,
      "comb.each_node_and_edge_activated_once");
  need(stats.lower_find_steps <= 20 * count && stats.lower_path_writes <= 4 * count,
      "comb.linear_physical_work");
  need(history.levels == before.levels && history.next == before.next, "comb.immutable_public_history");
  for (u64 cut = 1; cut <= count; ++cut)
    need(history.root_at(0, work_level(cut), false) == (cut == 1 ? 0 : count + cut - 1),
        "comb.past_open_cut_preserved");
  bool caught = false;
  try { (void)cursor.root_at(0, work_level(0), true); }
  catch (const full_ball_detail::Failure&) { caught = true; }
  need(caught, "comb.reject_time_reversal");
  FullBallStats fresh;
  full_ball_detail::MonotoneHistory boundary(history, fresh);
  need(boundary.root_at(0, work_level(1), false) == 0, "comb.open_cut_excludes_fusion");
  need(boundary.root_at(0, work_level(1), true) == count + 1, "comb.closed_cut_includes_fusion");
  caught = false;
  try { (void)boundary.root_at(0, work_level(1), false); }
  catch (const full_ball_detail::Failure&) { caught = true; }
  need(caught, "comb.reject_side_reversal");
  std::printf("{\"merges\":%llu,\"nodes\":%llu,\"baseline_edges\":%llu,\"activated_edges\":%llu,"
      "\"find_steps\":%llu,\"path_writes\":%llu}\n", static_cast<unsigned long long>(count),
      static_cast<unsigned long long>(history.levels.size()), static_cast<unsigned long long>(baseline_edges),
      static_cast<unsigned long long>(stats.lower_edges_activated), static_cast<unsigned long long>(stats.lower_find_steps),
      static_cast<unsigned long long>(stats.lower_path_writes));
}

void check_lot_work() {
  u64 checked = 0, regular_supports = 0, extra_supports = 0;
  u64 singletons = 0, grouped = 0, slots = 0, validation_supports = 0;
  for (const auto& fixture : fixtures()) for (unsigned variant = 0; variant < 2; ++variant) {
    const auto in = input(fixture, variant);
    std::vector<P3> points;
    for (const auto& point : in) points.push_back(point.position);
    const oracle::Model model(points);
    const auto ix = build_cloud_index(in);
    auto balls = catalogue(points, ix, model, fixture.kmax);
    if (variant) std::reverse(balls.begin(), balls.end());
    const auto result = build_full_ball_tower(ix, balls, fixture.kmax);
    need(result.status == FullBallStatus::kCompleteRelative, "work.producer_complete");
    const auto& stats = result.stats;
    const auto extra_count = std::count_if(balls.begin(), balls.end(), [](const auto& ball) {
      return ball.n_shell != ball.arity;
    });
    need(stats.declared_support_checks == balls.size() - extra_count &&
        stats.validation_work.calls == static_cast<u64>(extra_count) &&
        stats.validation_work.materializations == static_cast<u64>(extra_count),
        "work.declared_not_relabelled_as_MEB");
    u64 expected_singletons = 0, expected_grouped = 0, expected_slots = 0;
    for (unsigned k = 1; k <= fixture.kmax; ++k) {
      std::map<Rat, u64> levels;
      for (const auto& ball : balls)
        if (k >= ball.n_interior + ball.arity - 1u &&
            k <= static_cast<unsigned>(ball.n_interior) + ball.n_shell)
          ++levels[rational(ball.level)];
      for (const auto& [time, count] : levels) {
        (void)time;
        if (count == 1) ++expected_singletons;
        else { ++expected_grouped; expected_slots += count; }
      }
    }
    need(stats.singleton_lots == expected_singletons && stats.grouped_lots == expected_grouped &&
        stats.lot_dsu_slots == expected_slots, "work.physical_unitary_work");
    need(stats.singleton_lots + stats.lot_dsu_slots == stats.anchor_blocks,
        "work.accounts_every_block");
    regular_supports += stats.declared_support_checks;
    extra_supports += stats.validation_work.calls;
    singletons += stats.singleton_lots; grouped += stats.grouped_lots;
    slots += stats.lot_dsu_slots;
    for (u64 count : stats.validation_work.supports_by_size) validation_supports += count;
    ++checked;
  }
  // A right triangle's three boundary sites are not a positive regular q3
  // support: the declared-support validation must retain this rejection.
  const auto ix = build_cloud_index(std::vector<P3>{{0,0,0},{2,0,0},{0,2,0}});
  BallData nonpositive{};
  nonpositive.key = {1, {-2, -2, 0}, 0};
  nonpositive.level = work_level(2);
  nonpositive.arity = nonpositive.n_shell = 3;
  for (i32 j = 0; j < 3; ++j) nonpositive.shell_ids[j] = j;
  const auto rejected = build_full_ball_tower(ix, std::span<const BallData>(&nonpositive, 1), 3);
  need(rejected.status == FullBallStatus::kInvalidInput && rejected.orders.empty() &&
      rejected.stats.declared_support_checks == 1 && rejected.stats.validation_work.calls == 0,
      "work.declared_support_must_be_positive");
  need(checked == 28 && regular_supports > 0 && extra_supports > 0 && singletons > 0 && grouped > 0 && slots > 0,
      "work.nonvacuity");
  std::printf("{\"status\":\"passed\",\"clouds\":%llu,\"declared_support_checks\":%llu,"
      "\"extra_MEB_calls\":%llu,\"extra_MEB_supports\":%llu,\"singleton_lots\":%llu,"
      "\"grouped_lots\":%llu,\"lot_dsu_slots\":%llu,\"gcp_used\":false}\n",
      static_cast<unsigned long long>(checked), static_cast<unsigned long long>(regular_supports),
      static_cast<unsigned long long>(extra_supports), static_cast<unsigned long long>(validation_supports),
      static_cast<unsigned long long>(singletons), static_cast<unsigned long long>(grouped),
      static_cast<unsigned long long>(slots));
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    for (u64 count : {256, 512, 1024}) check_comb(count);
    check_lot_work();
    return 0;
  } catch (const Failure& failure) {
    std::fprintf(stderr, "%s\n", failure.why); return 1;
  } catch (const full_ball_detail::Failure& error) {
    std::fprintf(stderr, "%s\n", error.reason); return 1;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "%s\n", error.what()); return 1;
  }
}
