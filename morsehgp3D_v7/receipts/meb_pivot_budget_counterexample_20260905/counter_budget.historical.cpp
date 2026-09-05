#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "../v7_meb_pivot_prototype/pivot.hpp"

using namespace mhgp7;

namespace {

void check(bool condition, const char* reason) {
  if (!condition) throw std::runtime_error(reason);
}

bool same_stats(const SilentIncidenceStats& a, const SilentIncidenceStats& b) {
  return a.core_records == b.core_records && a.core_facets == b.core_facets &&
      a.facets_with_two_intruders == b.facets_with_two_intruders &&
      a.chain_steps == b.chain_steps && a.added_cofaces == b.added_cofaces &&
      a.terminal_direct == b.terminal_direct && a.terminal_cached == b.terminal_cached &&
      a.max_chain_length == b.max_chain_length && a.query_nodes == b.query_nodes &&
      a.query_leaves == b.query_leaves && a.query_range_skips == b.query_range_skips &&
      a.meb_calls == b.meb_calls && a.meb_supports == b.meb_supports;
}

bool same_ball(const silent_detail::LocalBall& a, const silent_detail::LocalBall& b) {
  return a.key == b.key && a.level == b.level && a.q == b.q && a.support == b.support;
}

silent_detail::LocalBall sentinel() {
  silent_detail::LocalBall ball;
  ball.key.a = 7;
  ball.key.b[0] = 11;
  ball.key.c = 13;
  ball.level = {{17, 19, 23}, 29};
  ball.q = 4;
  ball.support = {97, 98, 99, 100};
  return ball;
}

void run() {
  const std::vector<P3> points{{0, 0, 0}, {2, 2, 0}, {2, 0, 2}};
  const CloudIndex index = build_cloud_index(points);
  check(index.valid && !index.has_duplicate_positions(), "fixture.index");
  std::array<i32, 11> sites{};
  for (PointId id = 0; id < points.size(); ++id) {
    bool found = false;
    for (i32 site = 0; site < index.unique_count(); ++site) {
      if (index.point_id(site) == id) {
        sites[id] = site;
        found = true;
        break;
      }
    }
    check(found, "fixture.site_identity");
  }
  const std::vector<ForestEvent> direct;
  pivot_prototype::Candidate proposed;
  pivot_prototype::Work proposal_work;
  check(pivot_prototype::propose(index, sites, 3, &proposed, &proposal_work),
        "proposal.expected_success");
  check(proposed.q == 3 && proposal_work.candidates == 5 && proposal_work.pivots == 1,
        "proposal.expected_five_candidates");
  check(pivot_prototype::ordinal(3, proposed) == 4, "proposal.expected_ordinal_four");

  for (const u64 limit : {u64{4}, u64{1}}) {
    SilentIncidenceLimits caps;
    caps.max_meb_supports = limit;
    SilentIncidenceResult reference_result, pivot_result;
    auto reference_ball = sentinel();
    auto pivot_ball = sentinel();
    pivot_prototype::Work work;
    silent_detail::Builder reference(index, direct, caps, &reference_result);
    const bool reference_ok = reference.miniball(sites, 3, &reference_ball);
    const bool pivot_ok = pivot_prototype::miniball(
        index, direct, caps, &pivot_result, sites, 3, &pivot_ball, &work);
    check(reference_ok == pivot_ok, "terminal.boolean");
    check(reference_result.status == pivot_result.status &&
          std::strcmp(reference_result.reason, pivot_result.reason) == 0,
          "terminal.status_reason");
    check(same_stats(reference_result.stats, pivot_result.stats), "terminal.stats");
    check(same_ball(reference_ball, pivot_ball), "terminal.literal_ball");
    check(reference_result.events.empty() && pivot_result.events.empty(), "local.no_events");
    check(reference_result.stats.meb_supports == limit &&
          reference_result.stats.meb_calls == 1, "terminal.legacy_counters");
    check(work.candidates == 5 && work.pivots == 1 && work.certified == 1 &&
          work.fallback == 0 && work.candidates > limit, "counterexample.physical_budget");
    if (limit == 4) {
      check(reference_ok && reference_ball.q == 3, "reference.expected_q3_success");
    } else {
      check(!reference_ok &&
            reference_result.status == SilentIncidenceStatus::kResourceExhausted &&
            std::strcmp(reference_result.reason, "silent_meb_support_budget") == 0,
            "reference.expected_budget_refusal");
      check(same_ball(reference_ball, sentinel()), "reference.refusal_sentinel");
    }
    std::printf("counter_budget_case limit=%llu reference_supports=%llu "
                "proposal_candidates=%llu terminal_equal=1 sentinel_preserved=%d "
                "reference_ok=%d reason=%s\n",
                static_cast<unsigned long long>(limit),
                static_cast<unsigned long long>(reference_result.stats.meb_supports),
                static_cast<unsigned long long>(work.candidates),
                same_ball(reference_ball, sentinel()) ? 1 : 0,
                reference_ok ? 1 : 0, reference_result.reason);
  }
}

}  // namespace

int main(int argc, char**) {
  if (argc != 1) return 2;
  try {
    run();
    std::puts("counter_budget=counterexample_confirmed cases=2 "
              "proposition_candidates=5 reference_ordinal=4 public_status=not_claimed");
    return 0;
  } catch (const std::exception& error) {
    std::printf("counter_budget=failed reason=%s\n", error.what());
    return 1;
  }
}
