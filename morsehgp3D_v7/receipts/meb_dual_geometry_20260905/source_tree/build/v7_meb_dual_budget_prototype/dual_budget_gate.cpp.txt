#include <array>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

#include "pivot.hpp"

using namespace mhgp7;
namespace proposal = mhgp7::dual_budget_prototype;

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

struct Triangle {
  const std::vector<P3> points{{0, 0, 0}, {2, 2, 0}, {2, 0, 2}};
  CloudIndex index = build_cloud_index(points);
  std::array<i32, 11> sites{};
  const std::vector<ForestEvent> direct;

  Triangle() {
    check(index.valid && !index.has_duplicate_positions(), "fixture.index");
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
  }
};

struct Trace {
  u64 initial_proposal_count = 0;
  u64 forms = 0, pair_selections = 0, q3_forms = 0;
  u64 prospective_violations = 0, legacy_violations = 0;
  const SilentIncidenceResult* out = nullptr;
  const silent_detail::LocalBall* ball = nullptr;
  SilentIncidenceStats before_stats{};
  SilentIncidenceStatus before_status{};
  const char* before_reason = nullptr;
  silent_detail::LocalBall before_ball{};

  void bind(const SilentIncidenceResult& result,
            const silent_detail::LocalBall& local_ball) {
    out = &result;
    ball = &local_ball;
    before_stats = result.stats;
    before_status = result.status;
    before_reason = result.reason;
    before_ball = local_ball;
  }

  void observe_legacy() noexcept {
    if (!same_stats(out->stats, before_stats) || out->status != before_status ||
        std::strcmp(out->reason, before_reason) != 0 ||
        !same_ball(*ball, before_ball) || !out->events.empty())
      ++legacy_violations;
  }

  void before_pair_selection(const proposal::Work& work,
                             const proposal::Limits& limits) noexcept {
    ++pair_selections;
    if (work.meb_proposal_supports >= limits.max_meb_proposal_supports)
      ++prospective_violations;
    observe_legacy();
  }

  void before_form(const proposal::Work& work,
                   const proposal::Limits& limits, u8 q) noexcept {
    // A final counter equality cannot establish this causal property. The
    // observer runs before any form/positivity predicate of this candidate.
    if (work.meb_proposal_supports < initial_proposal_count ||
        work.meb_proposal_supports - initial_proposal_count != forms + 1 ||
        work.meb_proposal_supports > limits.max_meb_proposal_supports)
      ++prospective_violations;
    ++forms;
    if (q == 3) ++q3_forms;
    observe_legacy();
  }
};

void terminal_equal(const SilentIncidenceResult& reference,
                    const SilentIncidenceResult& actual,
                    const silent_detail::LocalBall& reference_ball,
                    const silent_detail::LocalBall& actual_ball,
                    bool reference_ok, bool actual_ok) {
  check(reference_ok == actual_ok, "terminal.boolean");
  check(reference.status == actual.status &&
        std::strcmp(reference.reason, actual.reason) == 0, "terminal.status_reason");
  check(same_stats(reference.stats, actual.stats), "terminal.all_legacy_stats");
  check(same_ball(reference_ball, actual_ball), "terminal.literal_ball");
  check(reference.events.empty() && actual.events.empty(), "terminal.no_events");
}

template <bool ChargeAfter>
u64 triangle_caps(const Triangle& fixture) {
  u64 violations = 0;
  for (const u64 p : {u64{0}, u64{1}, u64{4}, u64{5}}) {
    for (const u64 l : {u64{1}, u64{4}}) {
      SilentIncidenceLimits caps;
      caps.max_meb_supports = l;
      const proposal::Limits limits{p};
      proposal::Work work;
      SilentIncidenceResult reference, actual;
      reference.stats.core_records = actual.stats.core_records = 11;
      reference.stats.query_nodes = actual.stats.query_nodes = 13;
      auto reference_ball = sentinel();
      auto actual_ball = sentinel();
      Trace trace;
      trace.bind(actual, actual_ball);
      silent_detail::Builder fallback(fixture.index, fixture.direct, caps, &reference);
      const bool reference_ok = fallback.miniball(fixture.sites, 3, &reference_ball);
      const bool actual_ok = proposal::miniball<ChargeAfter>(
          fixture.index, fixture.direct, caps, &actual, fixture.sites, 3,
          &actual_ball, limits, &work, &trace);
      terminal_equal(reference, actual, reference_ball, actual_ball, reference_ok, actual_ok);
      check(reference_ok == (l == 4) && reference.stats.meb_calls == 1 &&
            reference.stats.meb_supports == l, "triangle.reference_ordinal");
      check(work.meb_proposal_supports == p && trace.forms == p,
            "triangle.real_proposal_forms");
      check(work.fallback == (p < 5 ? 1 : 0) &&
            work.certified == (p == 5 ? 1 : 0), "triangle.fallback_or_certificate");
      check(work.pivots == (p == 0 ? 0 : 1) &&
            trace.pair_selections == (p == 0 ? 0 : 1), "triangle.no_expired_pair_search");
      check(trace.q3_forms == (p == 5 ? 1 : 0), "triangle.middle_small_ball_stop");
      check(trace.legacy_violations == 0, "triangle.speculation_legacy_unchanged");
      if (!actual_ok) check(same_ball(actual_ball, sentinel()), "triangle.refusal_sentinel");
      violations += trace.prospective_violations;
      std::printf("triangle P=%llu L=%llu proposal_forms=%llu legacy=%llu fallback=%llu "
                  "terminal_equal=1 prospective_violations=%llu\n",
                  static_cast<unsigned long long>(p), static_cast<unsigned long long>(l),
                  static_cast<unsigned long long>(trace.forms),
                  static_cast<unsigned long long>(actual.stats.meb_supports),
                  static_cast<unsigned long long>(work.fallback),
                  static_cast<unsigned long long>(trace.prospective_violations));
    }
  }
  return violations;
}

template <bool ChargeAfter>
u64 cumulative(const Triangle& fixture) {
  SilentIncidenceLimits caps;
  caps.max_meb_supports = 12;
  const proposal::Limits limits{7};
  proposal::Work work;
  SilentIncidenceResult reference, actual;
  Trace trace;
  const std::array<u64, 4> expected_forms{5, 7, 7, 7};
  const std::array<u64, 4> expected_legacy{4, 8, 12, 12};
  const std::array<u64, 4> expected_fallback{0, 1, 2, 2};
  const std::array<u64, 4> expected_searches{1, 2, 2, 2};
  for (size_t call = 0; call < 4; ++call) {
    auto reference_ball = sentinel();
    auto actual_ball = sentinel();
    trace.bind(actual, actual_ball);
    silent_detail::Builder fallback(fixture.index, fixture.direct, caps, &reference);
    const bool reference_ok = fallback.miniball(fixture.sites, 3, &reference_ball);
    const bool actual_ok = proposal::miniball<ChargeAfter>(
        fixture.index, fixture.direct, caps, &actual, fixture.sites, 3,
        &actual_ball, limits, &work, &trace);
    terminal_equal(reference, actual, reference_ball, actual_ball, reference_ok, actual_ok);
    check(actual_ok == (call < 3), "cumulative.expected_boolean");
    check(work.meb_proposal_supports == expected_forms[call] &&
          trace.forms == expected_forms[call], "cumulative.counter_never_reset");
    check(actual.stats.meb_supports == expected_legacy[call] &&
          actual.stats.meb_calls == call + 1, "cumulative.no_double_call");
    check(work.fallback == expected_fallback[call] && work.certified == 1 &&
          trace.pair_selections == expected_searches[call], "cumulative.exhaustion_short_circuit");
    check(trace.legacy_violations == 0, "cumulative.speculation_legacy_unchanged");
  }
  // First call: 5 proposal forms, no F forms. Next two: 2 more proposal
  // forms and 4+4 F candidates. Actual 15 <= legacy 12 + proposal 7 = 19.
  check(trace.forms + 8 <= actual.stats.meb_supports + work.meb_proposal_supports,
        "cumulative.dual_budget_bound");
  std::puts("cumulative calls=4 P=7 L=12 proposal_forms=7 F_fallback_candidates=8 legacy=12");
  return trace.prospective_violations;
}

template <bool ChargeAfter>
u64 near_max(const Triangle& fixture) {
  const u64 maximum = std::numeric_limits<u64>::max();
  const proposal::Limits limits{maximum};
  proposal::Work work;
  work.meb_proposal_supports = maximum - 1;
  SilentIncidenceLimits caps;
  caps.max_meb_supports = maximum;
  SilentIncidenceResult reference, actual;
  reference.stats.meb_supports = actual.stats.meb_supports = maximum - 4;
  auto reference_ball = sentinel();
  auto actual_ball = sentinel();
  Trace trace;
  trace.initial_proposal_count = maximum - 1;
  trace.bind(actual, actual_ball);
  silent_detail::Builder fallback(fixture.index, fixture.direct, caps, &reference);
  const bool reference_ok = fallback.miniball(fixture.sites, 3, &reference_ball);
  const bool actual_ok = proposal::miniball<ChargeAfter>(
      fixture.index, fixture.direct, caps, &actual, fixture.sites, 3,
      &actual_ball, limits, &work, &trace);
  terminal_equal(reference, actual, reference_ball, actual_ball, reference_ok, actual_ok);
  check(actual_ok && work.meb_proposal_supports == maximum && trace.forms == 1 &&
        actual.stats.meb_supports == maximum && work.fallback == 1 &&
        trace.legacy_violations == 0, "near_max.safe_increment_and_fallback");
  std::puts("near_max proposal_increment=1 legacy_increment=4 overflow=0");
  return trace.prospective_violations;
}

template <bool ChargeAfter>
int run() {
  const Triangle fixture;
  const u64 violations = triangle_caps<ChargeAfter>(fixture) +
      cumulative<ChargeAfter>(fixture) + near_max<ChargeAfter>(fixture);
  std::printf("dual_budget accounting=%s prospective_violations=%llu public_status=not_claimed\n",
              proposal::kWorkAccounting, static_cast<unsigned long long>(violations));
  // Mutant terminals/counters remain identical: only the causal observation
  // should reject it. This branch is the same judge in both instantiations.
  return violations == 0 ? 0 : 4;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 1 && (argc != 2 || std::strcmp(argv[1], "--mutant=charge-after") != 0))
    return 2;
  try {
    return argc == 2 ? run<true>() : run<false>();
  } catch (const std::exception& error) {
    std::printf("dual_budget=failed reason=%s\n", error.what());
    return 1;
  }
}
