// Fresh product-port recheck of the immutable r2 source budget_gate.cpp.
// Origin: build/v7_meb_filter_qualification_20260906_r2/snapshot/build/
// v7_meb_filter_qualification_20260906/budget_gate.cpp
// Origin SHA256: 512ac7411ed813d1712439029a782fa3aa77470b6e5cd951d31a04af6991c726
// Product helper port SHA256 at preparation:
// f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3
// These are local dispatcher checks, NOT qualification of a persistent FULL
// Builder. Work survives the original sequences; each local F Builder binds
// exactly the current index, direct events, limits and output of its own arm.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <vector>

#include "../../morsehgp3D_v7/src/forest/meb_proposal.hpp"

using namespace mhgp7;
namespace proposal = mhgp7::meb_proposal_detail;

namespace {

void check(bool condition, const char* reason) {
  if (!condition) throw std::runtime_error(reason);
}

unsigned long long printed(u64 value) {
  return static_cast<unsigned long long>(value);
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
  // ExactLevel::operator== compares all three raw limbs and the denominator.
  return a.key == b.key && a.level == b.level && a.q == b.q && a.support == b.support;
}

bool same_work(const proposal::Work& a, const proposal::Work& b) {
  return a.meb_proposal_supports == b.meb_proposal_supports && a.pivots == b.pivots &&
      a.certified == b.certified && a.fallback == b.fallback &&
      a.reference_supports == b.reference_supports;
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

struct Fixture {
  const std::vector<P3> points;
  CloudIndex index;
  std::array<i32, 11> sites{};
  const std::vector<ForestEvent> direct;

  explicit Fixture(std::initializer_list<P3> input)
      : points(input), index(build_cloud_index(points)) {
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
  u64 forms = 0, pair_selections = 0, q2_forms = 0, q3_forms = 0, q4_forms = 0;
  u64 prospective_violations = 0, legacy_violations = 0;
  const SilentIncidenceResult* out = nullptr;
  const silent_detail::LocalBall* ball = nullptr;
  SilentIncidenceStats before_stats{};
  SilentIncidenceStatus before_status{};
  const char* before_reason = nullptr;
  silent_detail::LocalBall before_ball{};

  void bind(const SilentIncidenceResult& result, const silent_detail::LocalBall& local_ball) {
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

  void before_pair_selection(const proposal::Work& work, const proposal::Limits& limits) noexcept {
    ++pair_selections;
    if (work.meb_proposal_supports >= limits.max_meb_proposal_supports)
      ++prospective_violations;
    observe_legacy();
  }

  void before_form(const proposal::Work& work, const proposal::Limits& limits, u8 q) noexcept {
    // This observation precedes the actual form, not just its final counter.
    if (work.meb_proposal_supports < initial_proposal_count ||
        work.meb_proposal_supports - initial_proposal_count != forms + 1 ||
        work.meb_proposal_supports > limits.max_meb_proposal_supports)
      ++prospective_violations;
    ++forms;
    if (q == 2) ++q2_forms;
    if (q == 3) ++q3_forms;
    if (q == 4) ++q4_forms;
    observe_legacy();
  }
};

struct Trial {
  SilentIncidenceResult reference, traced, native;
  proposal::Work traced_work, native_work;
  Trace trace;

  explicit Trial(u64 initial_legacy = 0, u64 initial_proposal = 0) {
    // All eleven unrelated counters are nonzero to detect accidental resets.
    reference.stats = {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0, initial_legacy};
    traced = native = reference;
    traced_work.meb_proposal_supports = native_work.meb_proposal_supports = initial_proposal;
    trace.initial_proposal_count = initial_proposal;
  }
};

struct Expected {
  bool ok, sentinel_retained;
  u64 proposal, legacy, fallback, certified, pivots, pairs, q2, q3;
};

struct Totals {
  u64 cases = 0, successes = 0, budget_refusals = 0, shell_refusals = 0;
  u64 forms = 0, q2 = 0, q3 = 0, pairs = 0, pivots = 0, fallback = 0, certified = 0;
  u64 legacy_increments = 0, fallback_candidates = 0, prospective_violations = 0;
};

void terminal_equal(const SilentIncidenceResult& reference, const SilentIncidenceResult& actual,
                    const silent_detail::LocalBall& reference_ball,
                    const silent_detail::LocalBall& actual_ball, bool reference_ok, bool actual_ok) {
  check(reference_ok == actual_ok, "terminal.boolean");
  check(reference.status == actual.status && std::strcmp(reference.reason, actual.reason) == 0,
        "terminal.status_reason");
  check(same_stats(reference.stats, actual.stats), "terminal.all_13_legacy_stats");
  check(same_ball(reference_ball, actual_ball), "terminal.literal_ball_support_raw_level");
  check(reference.events.empty() && actual.events.empty(), "terminal.no_events");
}

void one(const char* name, const Fixture& fixture, size_t n, u64 p, u64 l,
         Trial* trial, const Expected& expected, Totals* totals, size_t pivot_cap = 16) {
  auto& t = *trial;
  SilentIncidenceLimits caps;
  caps.max_meb_supports = l;
  const proposal::Limits limits{p};
  const auto prior_work = t.traced_work;
  const auto prior_stats = t.traced.stats;
  const u64 prior_reference_supports = t.reference.stats.meb_supports;
  const u64 prior_forms = t.trace.forms, prior_q2 = t.trace.q2_forms, prior_q3 = t.trace.q3_forms;
  const u64 prior_pairs = t.trace.pair_selections, prior_violations = t.trace.prospective_violations;
  auto reference_ball = sentinel(), traced_ball = sentinel(), native_ball = sentinel();
  t.trace.bind(t.traced, traced_ball);
  proposal::NoObserver passive;
  silent_detail::Builder reference(fixture.index, fixture.direct, caps, &t.reference);
  const bool reference_ok = reference.miniball(fixture.sites, n, &reference_ball);
  silent_detail::Builder traced_reference(fixture.index, fixture.direct, caps, &t.traced);
  const bool traced_ok = proposal::miniball(
      fixture.index, traced_reference, caps, &t.traced, fixture.sites, n, &traced_ball,
      limits, &t.traced_work, &t.trace, pivot_cap);
  silent_detail::Builder native_reference(fixture.index, fixture.direct, caps, &t.native);
  const bool native_ok = proposal::miniball(
      fixture.index, native_reference, caps, &t.native, fixture.sites, n, &native_ball,
      limits, &t.native_work, &passive, pivot_cap);
  terminal_equal(t.reference, t.traced, reference_ball, traced_ball, reference_ok, traced_ok);
  terminal_equal(t.reference, t.native, reference_ball, native_ball, reference_ok, native_ok);
  check(same_work(t.traced_work, t.native_work), "NoObserver_Trace.all_Work_fields");
  check(traced_ok == expected.ok && same_ball(traced_ball, sentinel()) == expected.sentinel_retained,
        "expected.boolean_and_sentinel");
  check(t.traced_work.meb_proposal_supports == expected.proposal && t.traced.stats.meb_supports == expected.legacy,
        "expected.proposal_and_legacy");
  check(t.traced_work.fallback == expected.fallback && t.traced_work.certified == expected.certified &&
        t.traced_work.pivots == expected.pivots, "expected.routes_and_pivots");
  check(t.trace.pair_selections == expected.pairs && t.trace.q2_forms == expected.q2 &&
        t.trace.q3_forms == expected.q3 && t.trace.q4_forms == 0, "expected.form_arities");
  check(t.traced_work.meb_proposal_supports >= t.trace.initial_proposal_count &&
        t.trace.forms == t.traced_work.meb_proposal_supports - t.trace.initial_proposal_count &&
        t.trace.forms == t.trace.q2_forms + t.trace.q3_forms, "forms.never_reset_or_charge_removed_supports");
  check(t.traced.stats.meb_calls == prior_stats.meb_calls + 1 && t.trace.legacy_violations == 0,
        "calls.once_and_speculation_unchanged");
  if (expected.ok) {
    check(t.traced.status == SilentIncidenceStatus::kComplete, "success.status");
    check(traced_ball.q == (n == 2 ? 2 : 3), "success.support_arity");
    ++totals->successes;
  } else if (expected.sentinel_retained) {
    check(t.traced.status == SilentIncidenceStatus::kResourceExhausted &&
          std::strcmp(t.traced.reason, "silent_meb_support_budget") == 0, "budget.priority");
    ++totals->budget_refusals;
  } else {
    check(t.traced.status == SilentIncidenceStatus::kUnsupportedDegeneracy &&
          std::strcmp(t.traced.reason, "silent_local_nonessential_shell") == 0 &&
          traced_ball.q == 2 && traced_ball.support[0] == fixture.sites[0] &&
          traced_ball.support[1] == fixture.sites[2], "shell.F_written_first_support");
    ++totals->shell_refusals;
  }
  const u64 forms = t.trace.forms - prior_forms;
  const u64 legacy = t.traced.stats.meb_supports - prior_stats.meb_supports;
  const u64 fallbacks = t.traced_work.fallback - prior_work.fallback;
  // F charges one actual candidate per increment on a fallback. Never count
  // a fast-path virtual ordinal as physically executed F candidates.
  check(fallbacks <= 1, "one.fallback_count");
  const u64 expected_physical = fallbacks ?
      t.reference.stats.meb_supports - prior_reference_supports : 0;
  check(t.traced_work.reference_supports >= prior_work.reference_supports &&
        t.traced_work.reference_supports - prior_work.reference_supports == expected_physical &&
        t.traced_work.reference_supports <= t.traced.stats.meb_supports,
        "reference_supports.actual_F_delta_never_virtual_ordinal");
  totals->fallback_candidates += t.traced_work.reference_supports - prior_work.reference_supports;
  totals->legacy_increments += legacy;
  totals->forms += forms;
  totals->q2 += t.trace.q2_forms - prior_q2;
  totals->q3 += t.trace.q3_forms - prior_q3;
  totals->pairs += t.trace.pair_selections - prior_pairs;
  totals->pivots += t.traced_work.pivots - prior_work.pivots;
  totals->fallback += fallbacks;
  totals->certified += t.traced_work.certified - prior_work.certified;
  totals->prospective_violations += t.trace.prospective_violations - prior_violations;
  ++totals->cases;
  std::printf("case=%s n=%zu P=%llu L=%llu forms_delta=%llu legacy_delta=%llu fallback_delta=%llu "
              "success=%d sentinel=%d terminal_equal=1 NoObserver_equal=1 prospective_violations=%llu\n",
              name, n, printed(p), printed(l), printed(forms), printed(legacy), printed(fallbacks),
              traced_ok ? 1 : 0, expected.sentinel_retained ? 1 : 0,
              printed(t.trace.prospective_violations - prior_violations));
}

int run() {
  check(std::strcmp(proposal::kWorkAccounting,
        "reference_ordinal_plus_native_z_q3_q4_proposal_v2") == 0, "filtered.calendar");
  const Fixture triangle{{0, 0, 0}, {2, 2, 0}, {2, 0, 2}};
  const Fixture square{{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}};
  const u64 maximum = std::numeric_limits<u64>::max();
  Totals totals;
  for (const u64 p : {u64{0}, u64{1}, u64{2}, u64{3}, maximum})
    for (const u64 l : {u64{0}, u64{1}, u64{3}, u64{4}, u64{5}}) {
      Trial trial;
      const u64 pair = l > 0 && p > 0 ? 1 : 0;
      const u64 q3 = l > 0 && p >= 2 ? 1 : 0;
      one("triangle_caps", triangle, 3, p, l, &trial,
          {l >= 4, l < 4, pair + q3, std::min(l, u64{4}),
           l > 0 && p < 2 ? u64{1} : 0, q3, pair, pair, pair, q3}, &totals);
    }
  for (const u64 p : {u64{0}, u64{1}, u64{2}})
    for (const u64 l : {u64{0}, u64{1}, u64{2}}) {
      Trial trial;
      const u64 pair = l > 0 && p > 0 ? 1 : 0;
      one("pair_initial", triangle, 2, p, l, &trial,
          {l > 0, l == 0, pair, std::min(l, u64{1}),
           l > 0 && p == 0 ? u64{1} : 0, pair, 0, pair, pair, 0}, &totals);
    }
  {
    Trial trial;
    const std::array<Expected, 4> sequence{{
        {true, false, 2, 4, 0, 1, 1, 1, 1, 1},
        {true, false, 3, 8, 1, 1, 2, 2, 2, 1},
        {true, false, 3, 12, 2, 1, 2, 2, 2, 1},
        {false, true, 3, 12, 2, 1, 2, 2, 2, 1}}};
    for (const auto& expected : sequence)
      one("cumulative_P3_L12", triangle, 3, 3, 12, &trial, expected, &totals);
  }
  struct Boundary {
    const char* name;
    u64 initial_legacy, initial_proposal, p, l;
    Expected expected;
  };
  const std::array<Boundary, 8> boundaries{{
      {"MAX_last_charge_fallback", maximum - 4, maximum - 1, maximum, maximum,
       {true, false, maximum, maximum, 1, 0, 1, 1, 1, 0}},
      {"MAX_exact_fast", maximum - 4, maximum - 2, maximum, maximum,
       {true, false, maximum, maximum, 0, 1, 1, 1, 1, 1}},
      {"MAX_fast_legacy_refusal", maximum - 1, maximum - 2, maximum, maximum,
       {false, true, maximum, maximum, 0, 1, 1, 1, 1, 1}},
      {"MAX_legacy_already_full", maximum, maximum - 1, maximum, maximum,
       {false, true, maximum - 1, maximum, 0, 0, 0, 0, 0, 0}},
      {"MAX_P_already_full", maximum - 4, maximum, maximum, maximum,
       {true, false, maximum, maximum, 1, 0, 0, 0, 0, 0}},
      {"MAX_submax_P", maximum - 4, maximum - 2, maximum - 1, maximum,
       {true, false, maximum - 1, maximum, 1, 0, 1, 1, 1, 0}},
      {"MAX_legacy_above_L", maximum, 0, 2, maximum - 1,
       {false, true, 0, maximum, 0, 0, 0, 0, 0, 0}},
      {"MAX_P_slack", maximum - 4, maximum - 3, maximum, maximum,
       {true, false, maximum - 1, maximum, 0, 1, 1, 1, 1, 1}}}};
  for (const auto& boundary : boundaries) {
    Trial trial(boundary.initial_legacy, boundary.initial_proposal);
    one(boundary.name, triangle, 3, boundary.p, boundary.l,
                     &trial, boundary.expected, &totals);
  }
  {
    Trial trial;
    one("pivot_cap_zero", triangle, 3, 10, 4, &trial,
        {true, false, 1, 4, 1, 0, 0, 1, 1, 0}, &totals, 0);
  }
  for (const u64 p : {u64{0}, u64{1}, u64{2}})
    for (const u64 l : {u64{0}, u64{1}, u64{2}, u64{3}}) {
      Trial trial;
      const u64 pair = l > 0 && p > 0 ? 1 : 0;
      one("square_final_shell_fallback", square, 4, p, l, &trial,
          {false, l < 2, pair, std::min(l, u64{2}), l > 0 ? u64{1} : 0,
           0, 0, pair, pair, 0}, &totals);
    }
  check(totals.cases == 59 && totals.successes == 25 && totals.budget_refusals == 28 &&
        totals.shell_refusals == 6, "nonvacuum.cases_and_outcomes");
  check(totals.forms == 50 && totals.q2 == 34 && totals.q3 == 16 && totals.pairs == 34 &&
        totals.pivots == 23 && totals.fallback == 25 && totals.certified == 20,
        "nonvacuum.filtered_forms_and_routes");
  check(totals.legacy_increments == 118 && totals.fallback_candidates == 65 &&
        totals.fallback_candidates + totals.forms <= totals.legacy_increments + totals.forms,
        "nonvacuum.incremental_dual_budget_bound");
  std::printf("{\"schema\":\"mhgp7-product-port-meb-budget-v1\",\"status\":\"%s\",\"cause\":\"%s\","
              "\"accounting\":\"%s\",\"cases\":59,\"terminal_comparisons\":118,\"NoObserver_comparisons\":59,"
              "\"successes\":25,\"budget_refusals\":28,\"shell_refusals\":6,\"proposal_forms\":50,"
              "\"q2\":34,\"q3\":16,\"q4\":0,\"pair_selections\":34,\"pivots\":23,\"fallback\":25,"
              "\"certified\":20,\"legacy_increments\":118,\"F_fallback_candidates\":65,"
              "\"prospective_violations\":%llu,\"public_status\":\"not_claimed\"}\n",
              totals.prospective_violations == 0 ? "passed" : "causal_violation",
              totals.prospective_violations == 0 ? "none" : "charge_not_prospective",
              proposal::kWorkAccounting, printed(totals.prospective_violations));
  // The nominal gate retains the before-form causal observation. Mutants,
  // when qualified separately, must be real external product-header copies.
  return totals.prospective_violations == 0 ? 0 : 4;
}

}  // namespace

int main(int argc, char**) {
  if (argc != 1) return 2;
  try {
    return run();
  } catch (const std::exception& error) {
    std::printf("filtered_budget=failed reason=%s\n", error.what());
    return 1;
  }
}
