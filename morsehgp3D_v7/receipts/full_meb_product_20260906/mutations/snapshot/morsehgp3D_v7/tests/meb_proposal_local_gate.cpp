// Bounded product-port differential, not an independent geometry oracle.
// Explicit fixture/comparator adaptations from private R2 geometry_gate.cpp
// fdff2b96424413312e4564dbb66a41bfc511d4a3f140f5d41f77e519b2cfa67f,
// budget_gate.cpp 512ac7411ed813d1712439029a782fa3aa77470b6e5cd951d31a04af6991c726,
// and data-only additional_scenes.inc
// 6ced272e70bb3527a8b53728442b774508c1a9e7e49413bf2739e2774e6c0d51.
// Native extra-shell fixture: private R2 trajectory_gate.cpp
// dd78d0191fe3d784db0e69e96d1e321b5e6fabdd2abdf5ef2ebb80ffeb4021e5.
// No private prototype is included or used as the implementation under test.
#if defined(MHGP7_TESTING)
#error "MEB proposal local qualification requires the nominal product header"
#endif
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <vector>

#include "../src/forest/meb_proposal.hpp"

using namespace mhgp7;
namespace proposal = mhgp7::meb_proposal_detail;

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
  // Literal raw ExactLevel, including every numerator limb and denominator.
  return a.key == b.key && a.level == b.level && a.q == b.q && a.support == b.support;
}

bool same_work(const proposal::Work& a, const proposal::Work& b) {
  return a.meb_proposal_supports == b.meb_proposal_supports && a.pivots == b.pivots &&
      a.certified == b.certified && a.fallback == b.fallback &&
      a.reference_supports == b.reference_supports;
}

bool same_events(const std::vector<ForestEvent>& a, const std::vector<ForestEvent>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i].q != b[i].q || a[i].d != b[i].d || a[i].active_mask != b[i].active_mask ||
        a[i].level != b[i].level) return false;
    for (size_t j = 0; j < 11; ++j) if (a[i].support[j] != b[i].support[j]) return false;
    for (size_t j = 0; j < 9; ++j) if (a[i].interior[j] != b[i].interior[j]) return false;
  }
  return true;
}

bool same_result(const SilentIncidenceResult& a, const SilentIncidenceResult& b) {
  return a.status == b.status && std::strcmp(a.reason, b.reason) == 0 &&
      same_stats(a.stats, b.stats) && same_events(a.events, b.events);
}

silent_detail::LocalBall sentinel() {
  return {BallKey{17, {19, 23, 29}, 31}, ExactLevel{{37, 41, 43}, 47},
          9, {53, 59, 61, 67}};
}

SilentIncidenceResult initial_result(u64 legacy) {
  SilentIncidenceResult out;
  out.stats = {11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, legacy};
  out.events.resize(1);  // Output sentinels, not supplied direct geometry.
  auto& event = out.events.front();
  event.q = 3; event.d = 2; event.active_mask = 5;
  event.level = {{17, 19, 23}, 29};
  for (size_t j = 0; j < 11; ++j) event.support[j] = static_cast<PointId>(101 + j);
  for (size_t j = 0; j < 9; ++j) event.interior[j] = static_cast<PointId>(211 + j);
  return out;
}

struct Fixture {
  CloudIndex ix;
  std::array<i32, 11> sites{};
  size_t n;
  const std::vector<ForestEvent> direct;
  explicit Fixture(std::initializer_list<P3> points)
      : ix(build_cloud_index(std::vector<P3>(points))), n(points.size()) {
    check(n >= 2 && n <= 11 && ix.valid && !ix.has_duplicate_positions(), "fixture.domain");
    for (size_t p = 0; p < n; ++p) {
      bool found = false;
      for (i32 u = 0; u < ix.unique_count(); ++u)
        if (ix.point_id(u) == p) { sites[p] = u; found = true; break; }
      check(found, "fixture.original_site_order");
    }
  }
};

struct InjectedBeforeForm {};
struct InjectedAfterReference {};

struct Trace {
  u64 start_p = 0, forms = 0, pairs = 0, throw_on_form = 0;
  std::array<u64, 5> arities{};
  const SilentIncidenceResult* out = nullptr;
  const silent_detail::LocalBall* ball = nullptr;
  SilentIncidenceResult before;
  silent_detail::LocalBall before_ball{};
  void bind(const SilentIncidenceResult& result, const silent_detail::LocalBall& local) {
    out = &result; ball = &local; before = result; before_ball = local;
  }
  void unchanged() const {
    check(same_result(*out, before) && same_ball(*ball, before_ball),
          "observer.speculation_did_not_mutate_F_or_output");
  }
  void before_pair_selection(const proposal::Work& work, const proposal::Limits& limits) {
    check(work.meb_proposal_supports < limits.max_meb_proposal_supports,
          "observer.no_search_when_P_exhausted");
    ++pairs; unchanged();
  }
  void before_form(const proposal::Work& work, const proposal::Limits& limits, u8 q) {
    check(work.meb_proposal_supports >= start_p &&
          work.meb_proposal_supports - start_p == forms + 1 &&
          work.meb_proposal_supports <= limits.max_meb_proposal_supports,
          "observer.prospective_P_charge");
    check(q >= 2 && q <= 4, "observer.form_arity");
    ++forms; ++arities[q]; unchanged();
    if (forms == throw_on_form) throw InjectedBeforeForm{};
  }
};

SilentIncidenceLimits legacy_limit(u64 l) {
  SilentIncidenceLimits caps; caps.max_meb_supports = l; return caps;
}

struct Trial {
  const Fixture& f;
  SilentIncidenceLimits caps;
  proposal::Limits limits;
  SilentIncidenceResult reference, traced, native;
  proposal::Work traced_work{}, native_work{};
  Trace trace;
  // These three Builders are constructed ONCE, not once per local call.
  silent_detail::Builder reference_builder, traced_builder, native_builder;
  Trial(const Fixture& fixture, u64 p, u64 l, u64 legacy = 0, u64 proposed = 0, u64 a = 0)
      : f(fixture), caps(legacy_limit(l)), limits{p}, reference(initial_result(legacy)),
        traced(reference), native(reference),
        reference_builder(f.ix, f.direct, caps, &reference),
        traced_builder(f.ix, f.direct, caps, &traced),
        native_builder(f.ix, f.direct, caps, &native) {
    check(a <= legacy, "fixture.A_le_c");
    traced_work.meb_proposal_supports = native_work.meb_proposal_supports = proposed;
    traced_work.reference_supports = native_work.reference_supports = a;
    trace.start_p = proposed;
  }
};

struct Metrics {
  u64 calls = 0, success = 0, capped = 0, shell = 0, physical_F = 0, forms = 0;
  u64 q4_raw = 0, max_boundaries = 0, exceptions = 0;
  std::array<u64, 5> fast{};
};

struct Outcome { bool ok; silent_detail::LocalBall ball; };

Outcome one(Trial& t, Metrics& m, bool reverse = false, size_t n = 0, size_t pivot_cap = 16) {
  if (!n) n = t.f.n;
  auto sites = t.f.sites;
  if (reverse) std::reverse(sites.begin(), sites.begin() + n);
  auto rb = sentinel(), tb = sentinel(), nb = sentinel();
  const auto before = t.traced;
  const auto prior = t.traced_work;
  const u64 forms = t.trace.forms, pairs = t.trace.pairs;
  t.trace.bind(t.traced, tb);
  proposal::NoObserver passive;
  const bool rok = t.reference_builder.miniball(sites, n, &rb);
  const bool tok = proposal::miniball(t.f.ix, t.traced_builder, t.caps, &t.traced,
      sites, n, &tb, t.limits, &t.traced_work, &t.trace, pivot_cap);
  const bool nok = proposal::miniball(t.f.ix, t.native_builder, t.caps, &t.native,
      sites, n, &nb, t.limits, &t.native_work, &passive, pivot_cap);
  check(rok == tok && rok == nok && same_result(t.reference, t.traced) &&
        same_result(t.reference, t.native), "differential.terminal_all_13_F_stats_events");
  check(same_ball(rb, tb) && same_ball(rb, nb), "differential.full_literal_LocalBall");
  check(same_work(t.traced_work, t.native_work), "differential.all_5_Work_fields");
  check(same_events(t.traced.events, before.events) &&
        t.traced.stats.meb_calls == before.stats.meb_calls + 1, "call.once_events_preserved");
  const auto& w = t.traced_work;
  check(w.meb_proposal_supports >= prior.meb_proposal_supports &&
        w.reference_supports >= prior.reference_supports &&
        t.traced.stats.meb_supports >= before.stats.meb_supports, "counts.monotone");
  const u64 dp = w.meb_proposal_supports - prior.meb_proposal_supports;
  const u64 dc = t.traced.stats.meb_supports - before.stats.meb_supports;
  const u64 da = w.reference_supports - prior.reference_supports;
  const u64 df = w.fallback - prior.fallback, cert = w.certified - prior.certified;
  check(df <= 1 && cert <= 1 && df + cert <= 1, "routes.exclusive");
  check(da == (df ? dc : 0) && w.reference_supports <= t.traced.stats.meb_supports,
        "physical_F.only_real_fallback_supports_not_virtual_ordinal");
  check(dp == t.trace.forms - forms && dp <= 146 && w.pivots - prior.pivots <= 16,
        "counts.proposal_forms_and_bounded_native_pivots");
  if (before.stats.meb_supports >= t.caps.max_meb_supports)
    check(dp == 0 && dc == 0 && df == 0 && cert == 0 && pairs == t.trace.pairs,
          "budget.L_priority_no_pair_search");
  else if (prior.meb_proposal_supports >= t.limits.max_meb_proposal_supports)
    check(dp == 0 && df == 1 && pairs == t.trace.pairs, "budget.P_exhausted_before_search");
  if (tok) {
    ++m.success;
    if (cert) { ++m.fast[tb.q]; m.q4_raw += tb.q == 4 && tb.level.num[2] != 0; }
  } else if (t.traced.status == SilentIncidenceStatus::kResourceExhausted) {
    ++m.capped;
    check(same_ball(tb, sentinel()) && std::strcmp(t.traced.reason, "silent_meb_support_budget") == 0,
          "budget.refusal_retains_literal_sentinel");
  } else {
    ++m.shell;
    check(t.traced.status == SilentIncidenceStatus::kUnsupportedDegeneracy &&
          std::strcmp(t.traced.reason, "silent_local_nonessential_shell") == 0 &&
          !same_ball(tb, sentinel()) && df == 1, "shell.F_partial_materialization_preserved");
  }
  ++m.calls; m.forms += dp; m.physical_F += da;
  return {tok, tb};
}

void expect(const Trial& t, u64 p, u64 c, u64 pivots, u64 certified, u64 fallback, u64 a) {
  const proposal::Work expected{p, pivots, certified, fallback, a};
  check(same_work(t.traced_work, expected) && t.traced.stats.meb_supports == c,
        "fixed_calendar.all_5_Work_and_legacy");
}

void geometry(Metrics& m) {
  const std::array<Fixture, 9> scenes{{
      Fixture{{0,0,0},{65535,65535,65535}},
      Fixture{{0,0,0},{65535,65535,0},{65535,0,65535}},
      Fixture{{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535}},
      Fixture{{0,0,0},{8,0,0},{8,8,0},{0,8,0}},
      Fixture{{0,0,7},{0,9,6},{1,4,0},{0,0,1},{4,1,2}},
      Fixture{{0,0,0},{46368,28657,0},{28657,17711,0}},
      Fixture{{8,9,5},{8,1,5},{5,5,10},{1,5,5},{0,5,5},{5,5,11}},
      Fixture{{1,1,1},{2,1,1},{3,1,1},{4,1,1},{5,1,1},{6,1,1},{7,1,1},{8,1,1},{9,1,1},{0,0,0},{20,2,2}},
      Fixture{{4,4,4},{3,4,4},{5,4,4},{4,3,4},{4,5,4},{4,4,3},{4,4,5},{0,0,0},{8,8,0},{8,0,8},{0,8,8}}}};
  for (size_t i = 0; i < scenes.size(); ++i)
    for (const bool reverse : {false, true})
      for (const u64 p : {u64{0}, u64{1}, u64{146}}) {
        Trial trial(scenes[i], p, 2200);
        const auto first = one(trial, m, reverse);
        if (!reverse && p == 146 && (i == 2 || i == 7 || i == 8)) {
          check(first.ok && trial.traced_work.certified == 1 && trial.traced_work.reference_supports == 0,
                "named.fast_path_nonvacuum");
          if (i == 2) check(first.ball.q == 4 && first.ball.level.num[2] != 0, "named.q4_raw_high_limb");
          if (i == 7) check(first.ball.q == 2 && trial.traced.stats.meb_supports == 55, "named.last_q2_ordinal");
          if (i == 8) check(first.ball.q == 4 && trial.traced.stats.meb_supports == 550, "named.last_q4_ordinal");
        }
        (void)one(trial, m, reverse);  // Same Builder, counters and prior status.
      }
  // Native intermediate extra-shell is allowed; final extra-shell is not.
  proposal::Candidate intermediate;
  check(proposal::form(scenes[6].ix, scenes[6].sites, {0,1,4,0}, 3, &intermediate) &&
        intermediate.power(proposal::point(scenes[6].ix, scenes[6].sites, 2)) == 0 &&
        intermediate.power(proposal::point(scenes[6].ix, scenes[6].sites, 5)) > 0,
        "native_shell.nonvacuous_intermediate_shell_and_next_violator");
  Trial shell(scenes[6], 146, 1000);
  check(one(shell, m).ok && shell.traced_work.certified == 1 && shell.traced_work.pivots == 2,
        "native_shell.not_rejected_as_final_shell");
  check(m.calls == 109 && m.fast[2] && m.fast[3] && m.fast[4] && m.q4_raw && m.shell,
        "nonvacuum.geometry");
}

void budgets(Metrics& m) {
  const Fixture triangle{{0,0,0},{2,2,0},{2,0,2}};
  const Fixture square{{0,0,0},{2,0,0},{2,2,0},{0,2,0}};
  const u64 maximum = std::numeric_limits<u64>::max();
  for (const u64 p : {u64{0}, u64{1}, u64{2}, maximum})
    for (const u64 l : {u64{0}, u64{1}, u64{3}, u64{4}, u64{5}}) {
      Trial t(triangle, p, l);
      check(one(t, m).ok == (l >= 4), "triangle.exact_L_and_cap_minus_one");
      const u64 pair = l > 0 && p > 0, q3 = l > 0 && p >= 2;
      const u64 fallback = l > 0 && p < 2, c = std::min(l, u64{4});
      expect(t, pair + q3, c, pair, q3, fallback, fallback ? c : 0);
      check(t.trace.arities == std::array<u64,5>{0,0,pair,q3,0} && t.trace.pairs == pair,
            "triangle.fixed_physical_form_calendar");
    }
  {
    Trial t(triangle, 3, 12);
    for (size_t i = 0; i < 4; ++i) {
      check(one(t, m).ok == (i < 3), "persistent.four_calls_one_Builder");
      expect(t, i == 0 ? 2 : 3, std::min<u64>(4 * (i + 1), 12),
             i == 0 ? 1 : 2, 1, std::min<u64>(i, 2), std::min<u64>(4 * i, 8));
    }
  }
  for (const u64 p : {u64{0}, u64{1}, u64{2}})
    for (const u64 l : {u64{0}, u64{1}, u64{2}, u64{3}}) {
      Trial t(square, p, l);
      const auto result = one(t, m);
      check(!result.ok && same_ball(result.ball, sentinel()) == (l < 2), "shell.sentinel_boundary");
      const u64 pair = l > 0 && p > 0, c = std::min(l, u64{2});
      expect(t, pair, c, 0, 0, l > 0, c);
      if (l >= 2) check(result.ball.q == 2 && result.ball.support[0] == square.sites[0] &&
                       result.ball.support[1] == square.sites[2], "shell.first_F_support_written");
    }
  struct Boundary { u64 c, p, cap_p, cap_l, a; std::array<u64,6> expected; };
  // The c>L case is an explicit defensive refusal boundary, not a coherent
  // successful-attempt state. Never form A+p or c+P in u64, including at MAX.
  const std::array<Boundary, 8> boundaries{{
      {maximum-4,maximum-1,maximum,maximum,maximum-4,{maximum,maximum,1,0,1,maximum}},
      {maximum-4,maximum-2,maximum,maximum,maximum-4,{maximum,maximum,1,1,0,maximum-4}},
      {maximum-1,maximum-2,maximum,maximum,0,{maximum,maximum,1,1,0,0}},
      {maximum,maximum-1,maximum,maximum,maximum,{maximum-1,maximum,0,0,0,maximum}},
      {maximum-4,maximum,maximum,maximum,0,{maximum,maximum,0,0,1,4}},
      {maximum-4,maximum-2,maximum-1,maximum,0,{maximum-1,maximum,1,0,1,4}},
      {maximum,0,2,maximum-1,0,{0,maximum,0,0,0,0}},
      {maximum-4,maximum-3,maximum,maximum,0,{maximum-1,maximum,1,1,0,0}}}};
  for (const auto& b : boundaries) {
    Trial t(triangle, b.cap_p, b.cap_l, b.c, b.p, b.a);
    (void)one(t, m);
    const auto& e = b.expected;
    expect(t, e[0], e[1], e[2], e[3], e[4], e[5]); ++m.max_boundaries;
  }
  Trial forced(triangle, 10, 4);
  check(one(forced, m, false, 0, 0).ok, "pivot_cap_zero.F_fallback");
  expect(forced, 1, 4, 0, 0, 1, 4);
  check(m.calls == 45 && m.max_boundaries == 8 && m.capped && m.shell && m.success,
        "nonvacuum.budgets");
}

void exceptions(Metrics& m) {
  const Fixture triangle{{0,0,0},{2,2,0},{2,0,2}};
  {
    Trial t(triangle, 2, 4);
    auto ball = sentinel();
    const auto before = t.traced;
    t.trace.bind(t.traced, ball); t.trace.throw_on_form = 1;
    bool caught = false;
    try {
      (void)proposal::miniball(t.f.ix, t.traced_builder, t.caps, &t.traced,
          t.f.sites, t.f.n, &ball, t.limits, &t.traced_work, &t.trace);
    } catch (const InjectedBeforeForm&) { caught = true; }
    check(caught && same_result(t.traced, before) && same_ball(ball, sentinel()),
          "exception.before_form_F_and_output_untouched");
    expect(t, 1, 0, 0, 0, 0, 0);
    check(t.trace.forms == 1 && t.trace.pairs == 1, "exception.prospective_P_survives");
    ++m.exceptions;
  }
  // Boundary injection AFTER a real F call. This is not a claim to inject
  // an exception within F's arithmetic or between two of its candidate forms.
  for (const bool at_max : {false, true}) {
    const u64 initial = at_max ? std::numeric_limits<u64>::max() - 4 : 7;
    Trial t(triangle, 3, std::numeric_limits<u64>::max(), initial, 2, initial);
    auto ball = sentinel();
    bool caught = false;
    try {
      (void)proposal::reference_counted(t.traced.stats, t.traced_work, [&]() -> bool {
        check(t.traced_builder.miniball(t.f.sites, t.f.n, &ball), "exception.real_F_completed");
        throw InjectedAfterReference{};
      });
    } catch (const InjectedAfterReference&) { caught = true; }
    auto expected = initial_result(initial + 4); ++expected.stats.meb_calls;
    check(caught && same_result(t.traced, expected) && ball.q == 3,
          "exception.boundary_injection_preserves_F_terminal");
    expect(t, 2, initial + 4, 0, 0, 0, initial + 4);
    ++m.exceptions;
  }
  check(m.exceptions == 3, "nonvacuum.exceptions");
}

int run(bool rejects) {
  check(proposal::Limits{}.max_meb_proposal_supports == 0 &&
        std::strcmp(proposal::kWorkAccounting,
        "reference_ordinal_plus_native_z_q3_q4_proposal_v2") == 0, "contract.default_and_accounting");
  Metrics m;
  if (rejects) { budgets(m); exceptions(m); } else geometry(m);
  const auto count = [](u64 x) { return static_cast<unsigned long long>(x); };
  std::printf("{\"schema\":\"mhgp7-meb-proposal-local-v1\",\"status\":\"passed\","
      "\"test_mode\":\"%s\",\"public_status\":\"not_claimed\",\"calls\":%llu,"
      "\"terminal_comparisons\":%llu,\"all_13_F_stats_comparisons\":%llu,"
      "\"all_5_Work_comparisons\":%llu,\"success\":%llu,\"capped\":%llu,"
      "\"shell\":%llu,\"proposal_forms\":%llu,\"physical_F_supports\":%llu,"
      "\"fast_q2\":%llu,\"fast_q3\":%llu,\"fast_q4\":%llu,\"q4_raw\":%llu,"
      "\"max_boundaries\":%llu,\"exception_boundaries\":%llu}\n",
      rejects ? "rejects" : "selftest", count(m.calls), count(2*m.calls), count(2*m.calls),
      count(m.calls), count(m.success), count(m.capped), count(m.shell), count(m.forms),
      count(m.physical_F), count(m.fast[2]), count(m.fast[3]), count(m.fast[4]),
      count(m.q4_raw), count(m.max_boundaries), count(m.exceptions));
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::strcmp(argv[1], "--selftest") != 0 &&
                    std::strcmp(argv[1], "--rejects") != 0)) return 2;
  try { return run(std::strcmp(argv[1], "--rejects") == 0); }
  catch (const std::exception& error) {
    std::fprintf(stderr, "meb proposal local rejected: %s\n", error.what()); return 1;
  }
}
