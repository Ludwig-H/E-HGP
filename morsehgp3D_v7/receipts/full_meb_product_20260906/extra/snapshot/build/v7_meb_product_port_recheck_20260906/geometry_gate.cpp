// Fresh product-port recheck of the immutable r2 source geometry_gate.cpp.
// Origin: build/v7_meb_filter_qualification_20260906_r2/snapshot/build/
// v7_meb_filter_qualification_20260906/geometry_gate.cpp
// Origin SHA256: fdff2b96424413312e4564dbb66a41bfc511d4a3f140f5d41f77e519b2cfa67f
// Product helper port SHA256 at preparation:
// f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3
// These are local dispatcher checks, NOT qualification of a persistent FULL
// Builder. Work survives the original sequences; each local F Builder binds
// exactly the current index, direct events, limits and output of its own arm.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
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

// Explicit helper port from dual_budget_gate.cpp 6f748dc8. Event preservation
// below is strengthened to two nonempty sentinels, compared field by field.
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

std::vector<ForestEvent> event_sentinels() {
  std::vector<ForestEvent> result(2);
  for (size_t i = 0; i < result.size(); ++i) {
    result[i].q = static_cast<u8>(2 + i);
    result[i].d = static_cast<u8>(1 + i);
    result[i].active_mask = static_cast<u16>(3 + i);
    result[i].level = {{17 + i, 19 + i, 23 + i}, static_cast<i128>(29 + i)};
    for (size_t j = 0; j < 11; ++j) result[i].support[j] = static_cast<PointId>(101 + 13 * i + j);
    for (size_t j = 0; j < 9; ++j) result[i].interior[j] = static_cast<PointId>(211 + 17 * i + j);
  }
  return result;
}

silent_detail::LocalBall sentinel() {
  silent_detail::LocalBall ball;
  ball.key = BallKey{17, {19, 23, 29}, 31};
  ball.level = ExactLevel{{37, 41, 43}, 47};
  ball.q = 9;
  ball.support = {53, 59, 61, 67};
  return ball;
}

struct Fixture {
  CloudIndex ix;
  std::array<i32, 11> sites{};
  size_t n = 0;
};

Fixture fixture(const std::vector<P3>& points) {
  Fixture f;
  f.ix = build_cloud_index(points);
  f.n = points.size();
  check(f.n >= 2 && f.n <= 11 && f.ix.valid && !f.ix.has_duplicate_positions(), "fixture.valid_domain");
  for (size_t p = 0; p < f.n; ++p) {
    bool found = false;
    for (i32 u = 0; u < f.ix.unique_count(); ++u) {
      if (f.ix.point_id(u) == p) { f.sites[p] = u; found = true; break; }
    }
    check(found, "fixture.original_point_identity");
  }
  return f;
}

std::vector<Fixture> fixtures() {
  std::vector<Fixture> all;
  // Literal eight scenes and first 160 LCG cases from probe.cpp 42f5cde4;
  // no historical MEB result, support or favorable order enters this corpus.
  for (const auto& points : std::vector<std::vector<P3>>{
      {{0,0,0},{65535,65535,65535}},
      {{0,0,0},{65535,65535,0},{65535,0,65535}},
      {{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535}},
      {{0,0,0},{8,0,0},{8,8,0},{0,8,0}},
      {{0,0,7},{0,9,6},{1,4,0},{0,0,1},{4,1,2}},
      {{0,0,0},{4,0,0},{2,0,0}},
      {{0,0,0},{4,0,0},{2,3,0},{2,0,2}},
      {{0,0,0},{46368,28657,0},{28657,17711,0}}}) all.push_back(fixture(points));
  u64 state = 0x6d65622d76372d31ULL;
  const auto next = [&]() {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return state;
  };
  for (size_t k = 0; k < 160; ++k) {
    std::vector<P3> points;
    for (size_t j = 0; j < 2 + k % 10; ++j)
      points.push_back({static_cast<i64>((next() >> 32) & 65535),
                        static_cast<i64>((next() >> 32) & 65535),
                        static_cast<i64>((next() >> 32) & 65535)});
    all.push_back(fixture(points));
  }
  check(all.size() == 168, "corpus.inherited_cardinality");
  // Byte-identical r2 data-only additional_scenes.inc, SHA256:
  // 6ced272e70bb3527a8b53728442b774508c1a9e7e49413bf2739e2774e6c0d51.
  // The fresh runner must verify these bytes; no historical result is inherited.
  for (const auto& points : std::vector<std::vector<P3>>{
#include "additional_scenes.inc"
      }) all.push_back(fixture(points));
  check(all.size() == 176, "corpus.total_cardinality");
  return all;
}

struct Order { size_t index; std::array<i32, 11> sites; };

std::vector<Order> orders(const std::vector<Fixture>& all) {
  std::vector<Order> result;
  for (size_t i = 0; i < all.size(); ++i) {
    result.push_back({i, all[i].sites});
    auto reversed = all[i].sites;
    std::reverse(reversed.begin(), reversed.begin() + all[i].n);
    result.push_back({i, reversed});
  }
  check(result.size() == 352, "corpus.original_and_reverse");
  for (size_t i = 0; i < 3; ++i) {
    std::array<size_t, 4> permutation{0, 1, 2, 3};
    do {
      auto sites = all[i].sites;
      for (size_t j = 0; j < all[i].n; ++j) sites[j] = all[i].sites[permutation[j]];
      result.push_back({i, sites});
    } while (std::next_permutation(permutation.begin(), permutation.begin() + all[i].n));
  }
  check(result.size() == 384, "corpus.order_cardinality");
  return result;
}

u64 all_ordinals() {
  // Explicit port of all_ordinals from probe.cpp 42f5cde4. The reference is
  // an incremented lexicographic rank, independent of choose()/ordinal().
  u64 examined = 0;
  for (size_t n = 2; n <= 11; ++n) {
    u64 rank = 0;
    const auto one = [&](std::array<size_t, 4> slots, u8 q) {
      proposal::Candidate candidate;
      candidate.slots = slots;
      candidate.q = q;
      check(proposal::ordinal(n, candidate) == ++rank, "ordinal.lexicographic_rank");
      ++examined;
    };
    for (size_t a = 0; a < n; ++a) for (size_t b = a + 1; b < n; ++b) one({a,b,0,0}, 2);
    if (n == 11) check(rank == 55, "ordinal.q2_last");
    for (size_t a = 0; a < n; ++a) for (size_t b = a + 1; b < n; ++b)
      for (size_t c = b + 1; c < n; ++c) one({a,b,c,0}, 3);
    if (n == 11) check(rank == 220, "ordinal.q3_last");
    for (size_t a = 0; a < n; ++a) for (size_t b = a + 1; b < n; ++b)
      for (size_t c = b + 1; c < n; ++c) for (size_t d = c + 1; d < n; ++d) one({a,b,c,d}, 4);
    if (n == 11) check(rank == 550, "ordinal.q4_last");
  }
  check(examined == 1507, "ordinal.exact_count");
  return examined;
}

struct State {
  SilentIncidenceResult reference, actual;
  proposal::Work work;
  explicit State(u64 legacy = 0, u64 proposed = 0) {
    reference.stats = {11,13,17,19,23,29,31,37,41,43,47,53,legacy};
    actual.stats = reference.stats;
    reference.status = actual.status = SilentIncidenceStatus::kInvariantViolated;
    reference.reason = actual.reason = "seed_local_state";
    reference.events = actual.events = event_sentinels();
    work.meb_proposal_supports = proposed;
  }
};

struct Trace {
  const SilentIncidenceResult* out;
  const silent_detail::LocalBall* ball;
  SilentIncidenceStats before_stats;
  SilentIncidenceStatus before_status;
  const char* before_reason;
  silent_detail::LocalBall before_ball;
  std::vector<ForestEvent> before_events;
  u64 initial_proposal, forms = 0, searches = 0, causal = 0, legacy_changes = 0;
  Trace(const State& state, const silent_detail::LocalBall& local)
      : out(&state.actual), ball(&local), before_stats(state.actual.stats),
        before_status(state.actual.status), before_reason(state.actual.reason),
        before_ball(local), before_events(state.actual.events),
        initial_proposal(state.work.meb_proposal_supports) {}
  void observe_legacy() noexcept {
    if (!same_stats(out->stats, before_stats) || out->status != before_status ||
        std::strcmp(out->reason, before_reason) != 0 || !same_ball(*ball, before_ball) ||
        !same_events(out->events, before_events)) ++legacy_changes;
  }
  void before_pair_selection(const proposal::Work& work, const proposal::Limits& limits) noexcept {
    ++searches;
    if (work.meb_proposal_supports >= limits.max_meb_proposal_supports) ++causal;
    observe_legacy();
  }
  void before_form(const proposal::Work& work, const proposal::Limits& limits, u8) noexcept {
    if (work.meb_proposal_supports < initial_proposal ||
        work.meb_proposal_supports - initial_proposal != forms + 1 ||
        work.meb_proposal_supports > limits.max_meb_proposal_supports) ++causal;
    ++forms;
    observe_legacy();
  }
};

struct Metrics {
  u64 main = 0, boundary = 0, pilots = 0, causal = 0, forms = 0, searches = 0;
  u64 legacy_charges = 0, fallback_candidates = 0, certified = 0, fallback = 0;
  u64 complete = 0, degenerate = 0, capped = 0, q4_two_pivots = 0, q4_high_limb = 0;
  u64 exhausted_fallback = 0, initial_p_fallback = 0, shell_fallback = 0, forced_fallback = 0;
  u64 direct_form_checks = 0, direct_form_rejected = 0;
  std::array<u64, 5> fast_q{};
  std::array<u64, 3> named_fast{};
  std::array<u64, 12> n_seen{};
};

u64 reference_rank(const Fixture& f, const Order& order, Metrics* metrics) {
  SilentIncidenceLimits caps;
  caps.max_meb_supports = 551;
  SilentIncidenceResult result;
  silent_detail::LocalBall ball;
  const std::vector<ForestEvent> direct;
  silent_detail::Builder reference(f.ix, direct, caps, &result);
  const bool ok = reference.miniball(order.sites, f.n, &ball);
  check(ok || (result.status == SilentIncidenceStatus::kUnsupportedDegeneracy &&
        std::strcmp(result.reason, "silent_local_nonessential_shell") == 0), "pilot.expected_terminal");
  check(result.stats.meb_supports >= 1 && result.stats.meb_supports <= 550, "pilot.rank_range");
  if (order.sites == f.sites && order.index == 174)
    check(ok && ball.q == 2 && result.stats.meb_supports == 55 &&
          ball.support[0] == f.sites[9] && ball.support[1] == f.sites[10], "pilot.last_q2");
  if (order.sites == f.sites && order.index == 175)
    check(ok && ball.q == 4 && result.stats.meb_supports == 550 &&
          ball.support == std::array<i32,4>{f.sites[7],f.sites[8],f.sites[9],f.sites[10]}, "pilot.last_q4");
  ++metrics->pilots;
  return result.stats.meb_supports;
}

void compare(const Fixture& f, const Order& order, u64 legacy_cap, u64 proposal_cap,
             State* state, Metrics* metrics, bool boundary, size_t pivot_cap = 16) {
  const auto before_work = state->work;
  const auto before_stats = state->actual.stats;
  const u64 before_reference_supports = state->reference.stats.meb_supports;
  const auto before_events = state->actual.events;
  auto native_out = state->actual;
  auto native_work = state->work;
  auto native_ball = sentinel();
  auto reference_ball = sentinel(), actual_ball = sentinel();
  Trace trace(*state, actual_ball);
  SilentIncidenceLimits caps;
  caps.max_meb_supports = legacy_cap;
  const proposal::Limits limits{proposal_cap};
  const std::vector<ForestEvent> direct;
  silent_detail::Builder reference(f.ix, direct, caps, &state->reference);
  const bool reference_ok = reference.miniball(order.sites, f.n, &reference_ball);
  silent_detail::Builder actual_reference(f.ix, direct, caps, &state->actual);
  const bool actual_ok = proposal::miniball(
      f.ix, actual_reference, caps, &state->actual, order.sites, f.n, &actual_ball,
      limits, &state->work, &trace, pivot_cap);
  proposal::NoObserver observer;
  silent_detail::Builder native_reference(f.ix, direct, caps, &native_out);
  const bool native_ok = proposal::miniball(
      f.ix, native_reference, caps, &native_out, order.sites, f.n, &native_ball,
      limits, &native_work, &observer, pivot_cap);
  check(native_ok == actual_ok && native_out.status == state->actual.status &&
        std::strcmp(native_out.reason, state->actual.reason) == 0 &&
        same_stats(native_out.stats, state->actual.stats) &&
        same_events(native_out.events, state->actual.events) &&
        same_ball(native_ball, actual_ball) &&
        native_work.meb_proposal_supports == state->work.meb_proposal_supports &&
        native_work.pivots == state->work.pivots &&
        native_work.certified == state->work.certified &&
        native_work.fallback == state->work.fallback &&
        native_work.reference_supports == state->work.reference_supports,
        "native.same_as_trace_all_five_Work_fields");
  check(reference_ok == actual_ok && state->reference.status == state->actual.status &&
        std::strcmp(state->reference.reason, state->actual.reason) == 0, "differential.terminal");
  check(same_stats(state->reference.stats, state->actual.stats), "differential.all_stats");
  check(same_ball(reference_ball, actual_ball), "differential.literal_ball_and_level");
  check(before_events.size() == 2 && same_events(state->actual.events, before_events) &&
        same_events(state->reference.events, before_events), "differential.nonempty_events_untouched");
  check(trace.legacy_changes == 0, "proposal.legacy_untouched_before_form");
  check(state->work.meb_proposal_supports >= before_work.meb_proposal_supports &&
        state->actual.stats.meb_supports >= before_stats.meb_supports, "budget.monotone_counters");
  const u64 delta_p = state->work.meb_proposal_supports - before_work.meb_proposal_supports;
  const u64 delta_l = state->actual.stats.meb_supports - before_stats.meb_supports;
  const u64 delta_fallback = state->work.fallback - before_work.fallback;
  const u64 delta_certified = state->work.certified - before_work.certified;
  const u64 delta_pivots = state->work.pivots - before_work.pivots;
  check(delta_fallback <= 1 && delta_certified <= 1 && delta_fallback + delta_certified <= 1,
        "proposal.exclusive_routes");
  const size_t bounded_cap = std::min<size_t>(16, pivot_cap);
  const size_t bound = bounded_cap == 0 ? 1 : bounded_cap == 1 ? 2 :
                       6 + 10 * (bounded_cap - 2);
  check(delta_p == trace.forms && delta_pivots <= bounded_cap &&
        delta_p <= bound, "proposal.filtered_physical_bound");
  if (before_work.meb_proposal_supports < proposal_cap)
    check(delta_p <= proposal_cap - before_work.meb_proposal_supports, "proposal.remaining_budget");
  else check(delta_p == 0 && trace.searches == 0, "proposal.exhausted_before_pair");
  if (before_stats.meb_supports >= legacy_cap) {
    check(delta_p == 0 && delta_l == 0 && trace.searches == 0 &&
          delta_fallback == 0 && delta_certified == 0, "legacy.exhausted_priority");
  } else if (before_work.meb_proposal_supports >= proposal_cap) {
    check(delta_fallback == 1, "proposal.exhaustion_is_fallback");
    ++metrics->initial_p_fallback;
  }
  check(state->actual.stats.meb_calls == before_stats.meb_calls + 1, "legacy.single_call");
  if (!boundary && order.index < 3 && proposal_cap == 401 && reference_ok) {
    check(pivot_cap == 16 && delta_certified == 1 &&
          actual_ball.q == order.index + 2, "named_extreme.required_fast_certificate");
    if (order.index == 2)
      check(delta_pivots >= 2 && actual_ball.level.num[2] != 0, "named_extreme.q4_pivots_and_raw_high_limb");
    ++metrics->named_fast[order.index];
  }
  if (state->actual.status == SilentIncidenceStatus::kResourceExhausted)
    check(same_ball(actual_ball, sentinel()), "legacy.refusal_sentinel");
  const u64 expected_physical = delta_fallback ?
      state->reference.stats.meb_supports - before_reference_supports : 0;
  check(state->work.reference_supports >= before_work.reference_supports &&
        state->work.reference_supports - before_work.reference_supports == expected_physical &&
        state->work.reference_supports <= state->actual.stats.meb_supports,
        "reference_supports.actual_F_delta_never_virtual_ordinal");
  const u64 actual_fallback_candidates =
      state->work.reference_supports - before_work.reference_supports;
  check(actual_fallback_candidates + delta_p <= delta_l + delta_p, "budget.increment_bound");
  metrics->forms += delta_p;
  metrics->searches += trace.searches;
  metrics->causal += trace.causal;
  metrics->legacy_charges += delta_l;
  metrics->fallback_candidates += actual_fallback_candidates;
  metrics->certified += delta_certified;
  metrics->fallback += delta_fallback;
  metrics->complete += actual_ok;
  metrics->capped += state->actual.status == SilentIncidenceStatus::kResourceExhausted;
  metrics->degenerate += state->actual.status == SilentIncidenceStatus::kUnsupportedDegeneracy;
  if (delta_fallback && delta_p && state->work.meb_proposal_supports == proposal_cap)
    ++metrics->exhausted_fallback;
  if (delta_fallback && state->actual.status == SilentIncidenceStatus::kUnsupportedDegeneracy)
    ++metrics->shell_fallback;
  if (delta_fallback && pivot_cap == 0 && proposal_cap == 401) ++metrics->forced_fallback;
  if (actual_ok && delta_certified) {
    check(actual_ball.q >= 2 && actual_ball.q <= 4, "certificate.arity");
    ++metrics->fast_q[actual_ball.q];
    if (actual_ball.q == 4) {
      metrics->q4_two_pivots += delta_pivots >= 2;
      metrics->q4_high_limb += actual_ball.level.num[2] != 0;
    }
  }
  ++metrics->n_seen[f.n];
  if (boundary) ++metrics->boundary; else ++metrics->main;
}

void direct_forms(const std::vector<Fixture>& all, Metrics* metrics) {
  for (const size_t index : {size_t{170},size_t{171},size_t{172},size_t{173},size_t{168},size_t{169}}) {
    State state;
    auto ball = sentinel();
    Trace trace(state, ball);
    proposal::Candidate candidate;
    const u8 q = index == 170 ? 3 : (index >= 171 ? 4 : 2);
    const std::array<size_t,4> slots = q == 4 ? std::array<size_t,4>{0,1,2,3} :
                                             std::array<size_t,4>{0,1,2,0};
    const auto result = proposal::charged_form(
        all[index].ix, all[index].sites, slots, q, &candidate,
        proposal::Limits{1}, &state.work, &trace);
    if (index >= 170) {
      check(result == proposal::Attempt::kRejected, "forms.nonpositive_or_flat_rejected");
      ++metrics->direct_form_rejected;
    } else {
      check(result == proposal::Attempt::kAccepted &&
            candidate.power(proposal::point(all[index].ix, all[index].sites, 0)) == 0,
            "forms.q2_support_shell");
      const i128 power = candidate.power(proposal::point(all[index].ix, all[index].sites, 2));
      if (index == 168) check(power == 0, "forms.foreign_shell");
      else check(power > std::numeric_limits<i32>::max(), "forms.strict_intruder_beyond_i32");
    }
    check(state.work.meb_proposal_supports == 1 && trace.forms == 1 && trace.legacy_changes == 0,
          "forms.prospective_rejection_charged");
    metrics->causal += trace.causal;
    ++metrics->direct_form_checks;
  }
}

void boundaries(const std::vector<Fixture>& all, const std::vector<Order>& ordered,
                const std::vector<u64>& ranks, Metrics* metrics) {
  const u64 maximum = std::numeric_limits<u64>::max();
  for (const size_t index : {size_t{0},size_t{1},size_t{2},size_t{174},size_t{175}}) {
    const Order& order = ordered[2 * index];
    const u64 rank = ranks[2 * index];
    check(rank >= 1 && rank <= 550, "boundary.safe_rank_offset");
    for (const u64 p : {u64{0},u64{1},u64{401}})
      for (const u64 l : {7 + rank - 1, 7 + rank, 7 + rank + 1}) {
        State state(7);
        compare(all[index], order, l, p, &state, metrics, true);
      }
    for (const auto& edge : std::array<std::pair<u64,u64>,5>{{
        {0,0},{maximum,maximum},{maximum,maximum-1},{maximum-1,maximum},{maximum-4,maximum}}}) {
      for (const u64 p : {u64{0},u64{401}}) {
        State state(edge.first);
        compare(all[index], order, edge.second, p, &state, metrics, true);
      }
    }
    for (const auto& edge : std::array<std::pair<u64,u64>,3>{{
        {maximum,maximum},{maximum-1,maximum},{5,4}}}) {
      State state(0, edge.first);
      compare(all[index], order, 551, edge.second, &state, metrics, true);
    }
    State forced;
    compare(all[index], order, 551, 401, &forced, metrics, true, 0);
  }
  // The small integer equilateral triangle is deliberately the exact closed
  // counterfixture, not the differently scaled corpus q3 scene.
  const Fixture triangle = fixture({{0,0,0},{2,2,0},{2,0,2}});
  const Order order{0, triangle.sites};
  State historical_cap;
  for (size_t call = 0; call < 4; ++call) {
    compare(triangle, order, 12, 7, &historical_cap, metrics, true);
    const u64 completed = std::min<size_t>(call + 1, 3);
    check(historical_cap.work.meb_proposal_supports == 2 * completed &&
          historical_cap.actual.stats.meb_supports == 4 * completed &&
          historical_cap.work.fallback == 0 &&
          historical_cap.work.certified == completed, "boundary.historical_cap_new_calendar");
  }
  State state;
  const std::array<u64,4> p_counts{2,3,3,3}, l_counts{4,8,12,12}, f_counts{0,1,2,2};
  for (size_t call = 0; call < 4; ++call) {
    compare(triangle, order, 12, 3, &state, metrics, true);
    check(state.work.meb_proposal_supports == p_counts[call] &&
          state.actual.stats.meb_supports == l_counts[call] && state.work.fallback == f_counts[call] &&
          state.work.certified == 1, "boundary.cumulative_second_call_exhaustion");
  }
  State disabled;
  for (size_t call = 0; call < 2; ++call)
    compare(triangle, order, 8, 0, &disabled, metrics, true);
  check(disabled.work.meb_proposal_supports == 0 && disabled.work.fallback == 2,
        "boundary.cumulative_disabled");
  for (const size_t cap : {size_t{17},std::numeric_limits<size_t>::max()}) {
    State capped;
    compare(all[2], ordered[4], 551, 401, &capped, metrics, true, cap);
  }
  State last_q4(maximum - 550);
  compare(all[175], ordered[350], maximum, 401, &last_q4, metrics, true);
  check(last_q4.actual.stats.meb_supports == maximum && last_q4.work.certified == 1 &&
        last_q4.work.fallback == 0 && last_q4.work.pivots >= 2,
        "boundary.fast_q4_rank550_exact_MAX");
  check(metrics->boundary == 128, "boundary.exact_bounded_count");
}

void counter(const char* name, u64 value) {
  std::printf(",\"%s\":%llu", name, static_cast<unsigned long long>(value));
}

int run() {
  const u64 ordinals = all_ordinals();
  const auto all = fixtures();
  const auto ordered = orders(all);
  std::vector<u64> ranks;
  Metrics metrics;
  for (const auto& order : ordered) {
    const Fixture& f = all[order.index];
    const u64 rank = reference_rank(f, order, &metrics);
    ranks.push_back(rank);
    for (const u64 p : {u64{0},u64{1},u64{4},u64{5},u64{15},u64{16},u64{25},u64{401}})
      for (const u64 l : {rank-1,rank,rank+1}) {
        State state;  // Legacy starts at ZERO for the R-1/R/R+1 matrix.
        compare(f, order, l, p, &state, &metrics, false);
      }
  }
  boundaries(all, ordered, ranks, &metrics);
  direct_forms(all, &metrics);
  check(metrics.main == 9216 && metrics.pilots == 384, "nonvacuity.fixed_main_counts");
  check(metrics.named_fast == std::array<u64,3>{8,16,52}, "nonvacuity.named_extreme_certificates");
  check(metrics.fast_q[2] && metrics.fast_q[3] && metrics.fast_q[4] && metrics.q4_two_pivots &&
        metrics.q4_high_limb && metrics.complete && metrics.degenerate && metrics.capped &&
        metrics.exhausted_fallback && metrics.initial_p_fallback && metrics.shell_fallback &&
        metrics.forced_fallback && metrics.direct_form_checks == 6 && metrics.direct_form_rejected == 4,
        "nonvacuity.geometric_routes");
  for (size_t n = 2; n <= 11; ++n) check(metrics.n_seen[n] > 0, "nonvacuity.all_local_sizes");
  const bool causal_failure = metrics.causal != 0;
  std::printf("{\"schema\":\"mhgp7-product-port-meb-geometry-v1\",\"status\":\"%s\","
              "\"cause\":\"%s\",\"public_status\":\"not_claimed\"",
              causal_failure ? "causal_violation" : "passed", causal_failure ? "charge_not_prospective" : "none");
  counter("ordinals", ordinals); counter("scenes", all.size()); counter("orders", ordered.size());
  counter("main_comparisons", metrics.main); counter("boundary_comparisons", metrics.boundary);
  counter("reference_rank_calls", metrics.pilots); counter("prospective_violations", metrics.causal);
  counter("proposal_forms", metrics.forms); counter("pair_selections", metrics.searches);
  counter("legacy_charges", metrics.legacy_charges); counter("actual_fallback_candidates", metrics.fallback_candidates);
  counter("certified", metrics.certified); counter("fallback", metrics.fallback);
  counter("complete", metrics.complete); counter("degenerate", metrics.degenerate); counter("capped", metrics.capped);
  counter("fast_q2", metrics.fast_q[2]); counter("fast_q3", metrics.fast_q[3]); counter("fast_q4", metrics.fast_q[4]);
  counter("named_fast_q2", metrics.named_fast[0]); counter("named_fast_q3", metrics.named_fast[1]);
  counter("named_fast_q4", metrics.named_fast[2]);
  counter("q4_two_pivots", metrics.q4_two_pivots); counter("q4_high_limb", metrics.q4_high_limb);
  counter("exhausted_fallback", metrics.exhausted_fallback); counter("initial_p_fallback", metrics.initial_p_fallback);
  counter("shell_fallback", metrics.shell_fallback); counter("forced_fallback", metrics.forced_fallback);
  counter("direct_form_checks", metrics.direct_form_checks); counter("direct_form_rejected", metrics.direct_form_rejected);
  std::puts("}");
  return causal_failure ? 4 : 0;
}

}  // namespace

int main(int argc, char**) {
  if (argc != 1) return 2;
  try {
    return run();
  } catch (const std::exception& error) {
    std::fprintf(stderr, "meb_geometry=failed reason=%s\n", error.what());
    return 1;
  }
}
