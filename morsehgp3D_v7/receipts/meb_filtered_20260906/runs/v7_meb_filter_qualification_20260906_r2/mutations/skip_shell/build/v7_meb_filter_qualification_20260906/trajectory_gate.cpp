// Bounded PRIVATE trajectory gate, never a product implementation or oracle.
// Filter: 484a89bc2dbd472cc0571ed31d59631d5f31f9b0a425118040c916fc16e5abcf.
// Historical dual: 0645aa00add4d4cb387861b8f6dbd4fa0734ba5b4f3ad712caad8886b3541c2d.
// Shared exact forms/materialization: d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5.
// Local fixture data ported explicitly from auditor meb_pivot_filter_review.py
// f6ad7eecc5fc98c6bf5d2b4381d9b1cef83df3f393a7d0b2fa8e5367b2ee3450,
// receipt 17798f00893539eec8f49d60b8bf27b2c4d9fc164e33d7a52217eb93183bb9b7.
// The enumerator below shares form(), so is NOT an independent geometry oracle.
// The replay calls real pivot helpers but is NOT native propose telemetry;
// separately invoked native propose must match its terminal, work and q trace.
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>
#include "../v7_meb_dual_budget_prototype/pivot.hpp"
#include "../v7_meb_filtered_preparation_20260905/pivot.hpp"

#ifdef MHGP7_TESTING
#error trajectory_gate_requires_non_testing_headers
#endif

namespace {
using namespace mhgp7;
namespace old = mhgp7::dual_budget_prototype;
namespace fresh = mhgp7::meb_filtered_preparation;
namespace forms = mhgp7::pivot_prototype;
using Candidate = forms::Candidate;
constexpr u64 kProposalCap = 1000000;

struct Counts {
  u64 checks = 0, local_permutations = 0, local_diameter = 0;
  u64 local_23 = 0, local_34 = 0, local_43 = 0, local_44 = 0;
  u64 local_extra_shell = 0, counterexamples = 0, prefix_calls = 0;
  u64 native_calls = 0, native_success = 0, native_refusal = 0;
  u64 replay_pivots = 0, native_23 = 0, native_34 = 0;
  u64 intermediate_extra_shell = 0, order_mutants = 0;
  u64 admissible_order_local_calls = 0, admissible_order_native_calls = 0;
  u64 admissible_order_global_replays = 0, admissible_order_budget_differences = 0;
  u64 admissible_order_same_support = 0;
} counts;

void check(bool ok, const char* reason) {
  ++counts.checks;
  if (!ok) throw std::runtime_error(reason);
}

bool same_point(const P3& a, const P3& b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool same_candidate(const Candidate& a, const Candidate& b) {
  if (a.q != b.q || a.slots != b.slots || !same_point(a.a, b.a) ||
      !same_point(a.b, b.b) || a.three.g != b.three.g ||
      !same_point(a.three.a, b.three.a) || a.four.det != b.four.det ||
      !same_point(a.four.a, b.four.a)) return false;
  for (size_t i = 0; i < 3; ++i)
    if (a.three.w[i] != b.three.w[i] || a.four.np[i] != b.four.np[i]) return false;
  return true;
}

Candidate sentinel() {
  Candidate c;
  c.q = 4; c.slots = {7, 8, 9, 10}; c.a = {101, 102, 103};
  c.b = {104, 105, 106}; c.three.g = 107; c.three.w[1] = 109;
  c.three.a = {110, 111, 112}; c.four.det = 113; c.four.np[2] = 127;
  c.four.a = {128, 129, 130};
  return c;
}

struct Cloud {
  std::vector<P3> points;
  CloudIndex ix;
  std::array<i32, 11> sites{};
  explicit Cloud(std::vector<P3> input) : points(std::move(input)) {
    check(points.size() >= 2 && points.size() <= 11, "cloud.bounded_cardinality");
    for (const auto& p : points)
      check(p.x >= 0 && p.x <= 65535 && p.y >= 0 && p.y <= 65535 &&
            p.z >= 0 && p.z <= 65535, "cloud.u16_domain");
    ix = build_cloud_index(points);
    check(ix.valid && !ix.has_duplicate_positions(), "cloud.index");
    for (size_t id = 0; id < points.size(); ++id) {
      bool found = false;
      for (i32 site = 0; site < ix.unique_count(); ++site)
        if (ix.point_id(site) == id) { sites[id] = site; found = true; break; }
      check(found, "cloud.original_site_identity");
    }
  }
};

void compare_material(const Cloud& c, const Candidate& a, const Candidate& b) {
  check(same_candidate(a, b), "candidate.all_fields");
  const auto x = forms::materialize(c.ix, c.sites, a);
  const auto y = forms::materialize(c.ix, c.sites, b);
  check(x.q == y.q && x.support == y.support && x.key == y.key &&
        x.level == y.level, "material.literal_support_key_raw_level");
  check(forms::ordinal(c.points.size(), a) == forms::ordinal(c.points.size(), b),
        "material.ordinal_on_all_sites");
}

struct Proposal { std::array<size_t, 4> slots{}; u8 q = 0; };

// Recursive lexicographic combinations, not a copy of either nested pivot loop.
void append_combinations(const std::array<size_t, 5>& subset, size_t n,
                         size_t next, size_t used, Proposal p,
                         std::vector<Proposal>* all) {
  if (used == p.q) { all->push_back(p); return; }
  for (size_t i = next; i + (p.q - used) <= n; ++i) {
    p.slots[used] = subset[i];
    append_combinations(subset, n, i + 1, used + 1, p, all);
  }
}

std::vector<Proposal> proposals(const std::array<size_t, 5>& subset,
                                size_t n, bool filtered) {
  std::vector<Proposal> all, retained;
  check(n >= 2 && n <= 5, "enumerator.local_domain");
  for (u8 q = 2; q <= 4 && q <= n; ++q)
    append_combinations(subset, n, 0, 0, Proposal{{}, q}, &all);
  for (const auto& p : all) {
    const bool has_z = std::find(p.slots.begin(), p.slots.begin() + p.q,
                                subset[n - 1]) != p.slots.begin() + p.q;
    if (!filtered || (p.q >= 3 && has_z)) retained.push_back(p);
  }
  return retained;
}

struct Selection {
  Candidate candidate = sentinel();
  bool accepted = false;
  std::vector<u8> arities;
};

Selection select(const Cloud& c, const std::array<size_t, 5>& subset,
                 size_t n, std::vector<Proposal> list) {
  Selection result;
  for (const auto& p : list) {
    result.arities.push_back(p.q);
    Candidate proposed;
    if (!forms::form(c.ix, c.sites, p.slots, p.q, &proposed)) continue;
    bool contains = true;
    for (size_t i = 0; i < n; ++i)
      contains = contains && proposed.power(forms::point(c.ix, c.sites, subset[i])) <= 0;
    if (contains) { result.candidate = proposed; result.accepted = true; break; }
  }
  return result;
}

struct Trace {
  std::vector<u8> arities;
  u64 pairs = 0;
  template<class Work, class Limits>
  void before_pair_selection(const Work& w, const Limits& l) {
    check(w.meb_proposal_supports < l.max_meb_proposal_supports, "trace.pair_admission");
    ++pairs;
  }
  template<class Work, class Limits>
  void before_form(const Work& w, const Limits& l, u8 q) {
    check(w.meb_proposal_supports == arities.size() + 1 &&
          w.meb_proposal_supports <= l.max_meb_proposal_supports, "trace.prospective_charge");
    arities.push_back(q);
  }
};

void local_case(const Cloud& c, const std::array<size_t, 5>& subset,
                size_t n, bool diameter_premise, u8 final_q, u64 numerator, u64 denominator,
                const std::array<size_t, 4>& expected_slots) {
  Candidate prior;
  std::array<size_t, 4> prior_slots{};
  std::copy_n(subset.begin(), n - 1, prior_slots.begin());
  check(forms::form(c.ix, c.sites, prior_slots, static_cast<u8>(n - 1), &prior), "local.positive_old_base");
  check(prior.power(forms::point(c.ix, c.sites, subset[n - 1])) > 0, "local.strict_violator");
  i64 diameter2 = 0;
  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j)
      diameter2 = std::max(diameter2, p3_norm2(p3_sub(c.points[subset[i]], c.points[subset[j]])));
  const auto prior_level = forms::materialize(c.ix, c.sites, prior).level;
  check((compare_exact_level(prior_level, promote_level({diameter2, 4})) >= 0) == diameter_premise,
        "local.diameter_premise_is_explicit");
  const auto original = select(c, subset, n, proposals(subset, n, false));
  const auto list = proposals(subset, n, true);
  const auto filtered = select(c, subset, n, list);
  check(list.size() == (n == 3 ? 1 : n == 4 ? 4 : 10), "local.filtered_list_ceiling");
  check(original.accepted && original.candidate.q == final_q, "local.rational_fixture_support_arity");
  check(original.candidate.slots == expected_slots, "local.rational_fixture_first_support_slots");
  check(compare_exact_level(forms::materialize(c.ix, c.sites, original.candidate).level,
                            promote_level({numerator, denominator})) == 0, "local.rational_fixture_radius");
  old::Work ow; old::Limits ol{kProposalCap}; Trace ot;
  Candidate actual_old = sentinel();
  check(old::small_ball(c.ix, c.sites, subset, n, &actual_old, ol, &ow, &ot) == old::Attempt::kAccepted,
        "local.historical_accept");
  compare_material(c, actual_old, original.candidate);
  check(ot.arities == original.arities, "local.historical_lexicographic_prefix");
  if (diameter_premise) {
    check(filtered.accepted, "local.diameter_filter_has_first");
    compare_material(c, filtered.candidate, original.candidate);
    ++counts.local_diameter;
  }
  // Exact prefixes observe ordering and charge; P is not compared across arms.
  for (u64 cap = 0; cap <= list.size() + 1; ++cap) {
    fresh::Work fw; fresh::Limits fl{cap}; Trace ft;
    Candidate got = sentinel();
    const auto status = fresh::native_pivot_ball(c.ix, c.sites, subset, n, &got, fl, &fw, &ft);
    const size_t charged = std::min(static_cast<size_t>(cap), filtered.arities.size());
    check(ft.arities == std::vector<u8>(filtered.arities.begin(), filtered.arities.begin() + charged),
          "local.filtered_stable_prefix_q3_before_q4");
    check(fw.meb_proposal_supports == charged, "local.only_retained_forms_charged");
    const auto expected = filtered.accepted && cap >= filtered.arities.size() ? fresh::Attempt::kAccepted :
        cap < list.size() ? fresh::Attempt::kExhausted : fresh::Attempt::kRejected;
    check(status == expected, "local.prefix_status");
    if (status == fresh::Attempt::kAccepted) compare_material(c, got, filtered.candidate);
    else check(same_candidate(got, sentinel()), "local.failed_candidate_untouched");
    ++counts.prefix_calls;
  }
  if (n == 3 && final_q == 3) ++counts.local_23;
  if (n == 4 && final_q == 4) ++counts.local_34;
  if (n == 5 && final_q == 3) ++counts.local_43;
  if (n == 5 && final_q == 4) {
    check(filtered.accepted, "local.q4_replacement_observed_not_native_claim");
    compare_material(c, filtered.candidate, original.candidate);
    ++counts.local_44;
  }
  size_t shell = 0;
  for (const auto& p : c.points) shell += original.candidate.power(p) == 0;
  if (shell > final_q) {
    check(!diameter_premise && !filtered.accepted, "local.extra_shell_general_is_not_native_domain");
    ++counts.local_extra_shell;
  }
  ++counts.local_permutations;
}

void rational_local_fixtures() {
  struct Scene { std::vector<P3> points; bool diameter; u8 q; u64 num, den; std::array<size_t,4> slots; };
  const std::vector<Scene> scenes{
    {{{0,0,0},{2,2,0},{2,0,2}}, true, 3, 8, 3, {0,1,2,0}},
    {{{0,0,0},{2,2,0},{2,0,2},{0,2,2}}, true, 4, 3, 1, {0,1,2,3}},
    {{{30,30,30},{30,10,10},{10,30,10},{10,10,30},{20,20,38}}, true, 3, 15129, 49, {1,2,4,0}},
    {{{1,1,1},{3,3,1},{3,1,3},{1,3,3},{0,0,0}}, false, 4, 1083, 196, {1,2,3,4}},
    {{{2,1,1},{1,2,1},{1,1,2},{0,1,1}}, false, 2, 1, 1, {0,3,0,0}},
  };
  for (const auto& scene : scenes) {
    const Cloud c(scene.points);
    std::array<size_t, 5> subset{0,1,2,3,4};
    const size_t n = c.points.size();
    do { local_case(c, subset, n, scene.diameter, scene.q, scene.num, scene.den, scene.slots); }
    while (std::next_permutation(subset.begin(), subset.begin() + n - 1));
  }
}

template<bool Filtered> using Work = std::conditional_t<Filtered, fresh::Work, old::Work>;
template<bool Filtered> using Limits = std::conditional_t<Filtered, fresh::Limits, old::Limits>;

struct Replay {
  bool accepted = false;
  Candidate terminal = sentinel();
  std::vector<Candidate> supports;
  std::vector<size_t> violators;
  u64 extra_shell_with_future_violator = 0;
};

// Explicit test-side replay. Only native propose() below provides a native call.
template<bool Filtered>
Replay replay(const Cloud& c, size_t cap, Work<Filtered>* work, Trace* trace) {
  Replay r;
  const Limits<Filtered> limits{kProposalCap};
  trace->before_pair_selection(*work, limits);
  size_t a = 0, b = 1;
  i64 diameter = -1;
  for (size_t i = 0; i < c.points.size(); ++i)
    for (size_t j = i + 1; j < c.points.size(); ++j) {
      const i64 d = p3_norm2(p3_sub(c.points[i], c.points[j]));
      if (d > diameter) { diameter = d; a = i; b = j; }
    }
  Candidate candidate;
  if constexpr (Filtered)
    check(fresh::charged_form(c.ix, c.sites, {a,b,0,0}, 2, &candidate, limits, work, trace) == fresh::Attempt::kAccepted,
          "replay.initial_pair");
  else
    check(old::charged_form(c.ix, c.sites, {a,b,0,0}, 2, &candidate, limits, work, trace) == old::Attempt::kAccepted,
          "replay.initial_pair");
  r.supports.push_back(candidate);
  for (size_t step = 0; step <= std::min(cap, size_t{16}); ++step) {
    size_t first = c.points.size(), shell = 0;
    for (size_t i = 0; i < c.points.size(); ++i) {
      const i128 p = candidate.power(c.points[i]);
      if (p > 0 && first == c.points.size()) first = i;
      if (p == 0) ++shell;
    }
    if (first == c.points.size()) {
      if (shell == candidate.q) { r.accepted = true; r.terminal = candidate; }
      return r;
    }
    if (step == std::min(cap, size_t{16})) return r;
    r.extra_shell_with_future_violator += shell > candidate.q;
    r.violators.push_back(first);
    std::array<size_t, 5> subset{};
    std::copy_n(candidate.slots.begin(), candidate.q, subset.begin());
    subset[candidate.q] = first;
    const size_t n = candidate.q + size_t{1};
    const auto wanted = select(c, subset, n, proposals(subset, n, Filtered));
    check(wanted.accepted, "replay.pivot_first_exists");
    ++work->pivots;
    if constexpr (Filtered)
      check(fresh::native_pivot_ball(c.ix, c.sites, subset, n, &candidate, limits, work, trace) == fresh::Attempt::kAccepted,
            "replay.real_filtered_helper");
    else
      check(old::small_ball(c.ix, c.sites, subset, n, &candidate, limits, work, trace) == old::Attempt::kAccepted,
            "replay.real_legacy_helper");
    compare_material(c, candidate, wanted.candidate);
    r.supports.push_back(candidate);
  }
  return r;
}

template<bool Filtered>
void native_binding(const Cloud& c, size_t cap, const Replay& r,
                    Work<Filtered>* work, Trace* trace, const Work<Filtered>& rw, const Trace& rt) {
  Candidate got = sentinel();
  const Limits<Filtered> limits{kProposalCap};
  bool accepted = false;
  if constexpr (Filtered) accepted = fresh::propose(c.ix, c.sites, c.points.size(), &got, limits, work, trace, cap);
  else accepted = old::propose(c.ix, c.sites, c.points.size(), &got, limits, work, trace, cap);
  check(accepted == r.accepted && same_candidate(got, r.terminal), "native_vs_replay.literal_terminal_or_sentinel");
  if (accepted) compare_material(c, got, r.terminal);
  check(work->meb_proposal_supports == rw.meb_proposal_supports && work->pivots == rw.pivots &&
        work->certified == rw.certified && work->fallback == rw.fallback &&
        trace->arities == rt.arities && trace->pairs == rt.pairs, "native_vs_replay.persistent_work_and_q_trace");
  ++counts.native_calls;
  counts.native_success += accepted;
  counts.native_refusal += !accepted;
}

void native_trajectories() {
  // Analytic new scene: diameter slots 0/4, then q3 {0,1,4} has center
  // (5,5,5), radius^2=25 and extra-shell point 2. Point 5 is still outside;
  // its next pivot is q4. This expected geometry is NOT an auditor receipt.
  const std::vector<std::vector<P3>> scenes{
    {{0,0,0},{4,0,0}}, {{0,0,0},{2,2,0},{2,0,2}},
    {{0,0,0},{2,2,0},{2,0,2},{0,2,2}},
    {{8,9,5},{8,1,5},{5,5,10},{1,5,5},{0,5,5},{5,5,11}},
    {{8,9,5},{8,1,5},{5,5,10},{1,5,5},{0,5,5}},
    {{0,0,0},{4,0,0},{2,2,0}},
  };
  old::Work old_native, old_replay;
  fresh::Work new_native, new_replay;
  Trace ont, ort, nnt, nrt;
  for (size_t scene = 0; scene < scenes.size(); ++scene)
    for (size_t ordering = 0; ordering < 3; ++ordering) {
      auto points = scenes[scene];
      if (ordering == 1) std::reverse(points.begin(), points.end());
      if (ordering == 2) std::rotate(points.begin(), points.begin() + 1, points.end());
      const Cloud c(points);
      for (size_t cap : {size_t{0}, size_t{1}, size_t{2}, size_t{16}, size_t{99}}) {
        const auto a = replay<false>(c, cap, &old_replay, &ort);
        const auto b = replay<true>(c, cap, &new_replay, &nrt);
        check(a.accepted == b.accepted && a.supports.size() == b.supports.size() &&
              a.violators == b.violators && a.extra_shell_with_future_violator == b.extra_shell_with_future_violator,
              "trajectory.same_first_violators_and_support_inventory");
        for (size_t i = 0; i < a.supports.size(); ++i) {
          compare_material(c, a.supports[i], b.supports[i]);
          if (i) {
            ++counts.replay_pivots;
            counts.native_23 += a.supports[i-1].q == 2 && a.supports[i].q == 3;
            counts.native_34 += a.supports[i-1].q == 3 && a.supports[i].q == 4;
          }
        }
        native_binding<false>(c, cap, a, &old_native, &ont, old_replay, ort);
        native_binding<true>(c, cap, b, &new_native, &nnt, new_replay, nrt);
        counts.intermediate_extra_shell += b.extra_shell_with_future_violator;
        if (scene == 3 && ordering == 0 && cap == 16) {
          check(b.accepted && b.supports.size() == 3 && b.violators == std::vector<size_t>{1,5} &&
                b.supports[0].slots == std::array<size_t,4>{0,4,0,0} &&
                b.supports[1].slots == std::array<size_t,4>{0,1,4,0} &&
                b.supports[2].slots == std::array<size_t,4>{0,1,4,5} &&
                b.extra_shell_with_future_violator == 1, "native.analytic_intermediate_shell_fixture");
          check(compare_exact_level(forms::materialize(c.ix, c.sites, b.supports[1]).level,
                                    promote_level({25,1})) == 0, "native.analytic_intermediate_radius");
        }
      }
    }
  check(old_native.meb_proposal_supports > new_native.meb_proposal_supports &&
        old_native.meb_proposal_supports < kProposalCap && new_native.meb_proposal_supports < kProposalCap,
        "trajectory.both_persistent_budgets_nonlimiting");
}

void false_generalizations() {
  const std::vector<std::vector<P3>> scenes{
    {{0,0,0},{4,0,0},{2,0,0}}, {{0,2,0},{4,2,0},{2,4,0}},
    {{0,0,0},{2,0,0},{5,0,0}},
  };
  for (size_t i = 0; i < scenes.size(); ++i) {
    const Cloud c(scenes[i]);
    const std::array<size_t,5> subset{0,1,2,0,0};
    Candidate base;
    check(forms::form(c.ix, c.sites, {0,1,0,0}, 2, &base), "counterexample.base");
    const i128 p = base.power(c.points[2]);
    check(i == 0 ? p < 0 : i == 1 ? p == 0 : p > 0, "counterexample.precise_failed_premise");
    const auto legacy = select(c, subset, 3, proposals(subset, 3, false));
    check(legacy.accepted && legacy.candidate.q == 2, "counterexample.lost_pair_exists");
    fresh::Work w; fresh::Limits l{kProposalCap}; Trace t;
    Candidate got = sentinel();
    check(fresh::native_pivot_ball(c.ix, c.sites, subset, 3, &got, l, &w, &t) == fresh::Attempt::kRejected &&
          same_candidate(got, sentinel()), "counterexample.not_a_generic_small_ball_api");
    ++counts.counterexamples;
  }
}

void order_budget_admissible(bool inject_order) {
  // R2 correction: under a positive independent Q and strict z the support is
  // unique. Auditor radical-plane proof cbb3f536...; ordering is observable in
  // P/admission, not in the accepted support at nonlimiting P.
  const Cloud c({{0,0,0},{2,2,0},{2,0,2},{0,2,2}});
  const std::array<size_t,5> subset{0,1,2,3,0};
  Candidate prior, expected;
  check(forms::form(c.ix, c.sites, {0,1,2,0}, 3, &prior) &&
        forms::form(c.ix, c.sites, {0,1,2,3}, 4, &expected), "order_budget.positive_bases");
  check(prior.power(c.points[3]) > 0 &&
        compare_exact_level(forms::materialize(c.ix, c.sites, prior).level, promote_level({8,3})) == 0 &&
        compare_exact_level(forms::materialize(c.ix, c.sites, expected).level, promote_level({3,1})) == 0,
        "order_budget.strict_admissible_geometry");
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = i + 1; j < 4; ++j)
      check(p3_norm2(p3_sub(c.points[i], c.points[j])) == 8, "order_budget.global_diameter_ties");
  auto reversed = proposals(subset, 4, true);
  check(reversed.size() == 4 && reversed[0].q == 3 && reversed[1].q == 3 &&
        reversed[2].q == 3 && reversed[3].q == 4, "order_budget.declared_nominal_list");
  std::reverse(reversed.begin(), reversed.end());
  // Test-side order mutant only. The real charge primitive and containment
  // remain intact; no alternative helper is added to the private proposal API.
  const auto q4_first = [&](Candidate* out, const fresh::Limits& limits,
                            fresh::Work* work, Trace* trace) {
    for (const auto& p : reversed) {
      Candidate built;
      const auto status = fresh::charged_form(c.ix, c.sites, p.slots, p.q, &built, limits, work, trace);
      if (status == fresh::Attempt::kExhausted) return status;
      if (status != fresh::Attempt::kAccepted) continue;
      bool contains = true;
      for (const auto& point : c.points) contains = contains && built.power(point) <= 0;
      if (contains) { *out = built; return fresh::Attempt::kAccepted; }
    }
    return fresh::Attempt::kRejected;
  };
  for (u64 cap : {u64{0}, u64{1}, u64{3}, u64{4}}) {
    fresh::Work nominal_work, changed_work;
    const fresh::Limits limits{cap};
    Trace nominal_trace, changed_trace;
    Candidate nominal = sentinel(), changed = sentinel();
    const auto ns = fresh::native_pivot_ball(c.ix, c.sites, subset, 4, &nominal,
                                             limits, &nominal_work, &nominal_trace);
    const auto cs = q4_first(&changed, limits, &changed_work, &changed_trace);
    check(ns == (cap == 4 ? fresh::Attempt::kAccepted : fresh::Attempt::kExhausted) &&
          cs == (cap ? fresh::Attempt::kAccepted : fresh::Attempt::kExhausted), "order_budget.local_admission");
    check(nominal_work.meb_proposal_supports == cap && changed_work.meb_proposal_supports == (cap ? u64{1} : u64{0}),
          "order_budget.local_prospective_counts");
    const std::vector<u8> full_q{3,3,3,4};
    check(nominal_trace.arities == std::vector<u8>(full_q.begin(), full_q.begin() + cap) &&
          changed_trace.arities == (cap ? std::vector<u8>{4} : std::vector<u8>{}), "order_budget.local_q_prefixes");
    if (cap != 4) check(same_candidate(nominal, sentinel()), "order_budget.nominal_refusal_sentinel");
    if (!cap) check(same_candidate(changed, sentinel()), "order_budget.mutant_refusal_sentinel");
    else compare_material(c, changed, expected);
    if (cap == 4) {
      compare_material(c, nominal, changed);  // Must pass BEFORE the causal mutant rejection.
      ++counts.admissible_order_same_support;
      const u64 selected_calendar = inject_order ? changed_work.meb_proposal_supports : nominal_work.meb_proposal_supports;
      check(selected_calendar == 4, "order_budget.calendar_changed");
    }
    counts.admissible_order_budget_differences += (ns == fresh::Attempt::kAccepted) != (cs == fresh::Attempt::kAccepted);
    counts.admissible_order_local_calls += 2;
  }
  Candidate native_p6 = sentinel();
  bool native_p3_refused = false;
  for (u64 cap : {u64{0}, u64{1}, u64{2}, u64{3}, u64{5}, u64{6}}) {
    fresh::Work work;
    const fresh::Limits limits{cap};
    Trace trace;
    Candidate got = sentinel();
    const bool accepted = fresh::propose(c.ix, c.sites, 4, &got, limits, &work, &trace);
    const std::vector<u8> full_q{2,3,3,3,3,4};
    check(accepted == (cap == 6) && work.meb_proposal_supports == cap &&
          work.pivots == (cap == 0 ? u64{0} : cap == 1 ? u64{1} : u64{2}) && trace.pairs == (cap ? u64{1} : u64{0}) &&
          trace.arities == std::vector<u8>(full_q.begin(), full_q.begin() + cap), "order_budget.native_global_calendar");
    if (accepted) {
      compare_material(c, got, expected);
      native_p6 = got;
      ++counts.admissible_order_same_support;
    } else check(same_candidate(got, sentinel()), "order_budget.native_refusal_sentinel");
    if (cap == 3) native_p3_refused = !accepted;
    ++counts.admissible_order_native_calls;
  }
  // One explicitly test-side full-call replay at global P=3. Work and Trace
  // persist across initial pair, first q3 pivot, and reordered second pivot.
  fresh::Work replay_work;
  const fresh::Limits replay_limits{3};
  Trace replay_trace;
  Candidate replayed = sentinel();
  replay_trace.before_pair_selection(replay_work, replay_limits);
  check(fresh::charged_form(c.ix, c.sites, {0,1,0,0}, 2, &replayed,
                           replay_limits, &replay_work, &replay_trace) == fresh::Attempt::kAccepted,
        "order_budget.replay_initial_pair");
  ++replay_work.pivots;
  check(fresh::native_pivot_ball(c.ix, c.sites, {0,1,2,0,0}, 3, &replayed,
                               replay_limits, &replay_work, &replay_trace) == fresh::Attempt::kAccepted,
        "order_budget.replay_first_pivot");
  compare_material(c, replayed, prior);
  ++replay_work.pivots;
  check(q4_first(&replayed, replay_limits, &replay_work, &replay_trace) == fresh::Attempt::kAccepted &&
        replay_work.meb_proposal_supports == 3 && replay_work.pivots == 2 &&
        replay_trace.arities == std::vector<u8>{2,3,4} && native_p3_refused, "order_budget.replay_global_admission_difference");
  compare_material(c, replayed, native_p6);
  ++counts.admissible_order_same_support;
  ++counts.admissible_order_budget_differences;
  ++counts.admissible_order_global_replays;
}

void order_sensitivity(bool inject_reverse) {
  // Deliberately OUTSIDE positive-Q/native premises: four coplanar old sites
  // on the same circle. This tests a support-sensitive test-side order mutant,
  // not an admissible-native ambiguity or a new supported input domain.
  const Cloud c({{8,9,5},{8,1,5},{10,5,5},{5,10,5},{0,5,5}});
  const std::array<size_t,5> subset{0,1,2,3,4};
  Candidate invalid_q;
  check(!forms::form(c.ix, c.sites, {0,1,2,3}, 4, &invalid_q), "order_mutant.outside_positive_Q");
  auto list = proposals(subset, 5, true);
  const auto normal = select(c, subset, 5, list);
  std::reverse(list.begin(), list.end());
  const auto reversed = select(c, subset, 5, list);
  check(normal.accepted && reversed.accepted && normal.candidate.q == 3 && reversed.candidate.q == 3 &&
        normal.candidate.slots != reversed.candidate.slots, "order_mutant.distinct_positive_supports");
  check(normal.candidate.slots == std::array<size_t,4>{0,1,4,0} &&
        reversed.candidate.slots == std::array<size_t,4>{1,3,4,0}, "order_mutant.literal_first_supports");
  const auto a = forms::materialize(c.ix, c.sites, normal.candidate);
  const auto b = forms::materialize(c.ix, c.sites, reversed.candidate);
  check(a.key == b.key && same_exact_level(a.level, b.level) && a.support != b.support,
        "order_mutant.same_ball_is_insufficient");
  fresh::Work w; fresh::Limits l{kProposalCap}; Trace t;
  Candidate got;
  check(fresh::native_pivot_ball(c.ix, c.sites, subset, 5, &got, l, &w, &t) == fresh::Attempt::kAccepted,
        "order_mutant.nominal_helper_observation_only");
  if (inject_reverse) got = reversed.candidate;  // Explicit test-side mutant.
  check(same_candidate(got, normal.candidate), "order_mutant.first_support_changed");
  compare_material(c, got, normal.candidate);
  ++counts.order_mutants;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::string_view(argv[1]) != "--selftest" &&
                    std::string_view(argv[1]) != "--reverse-order-mutant" &&
                    std::string_view(argv[1]) != "--admissible-order-mutant")) return 2;
  const bool mutant = std::string_view(argv[1]) == "--reverse-order-mutant";
  const bool admissible_mutant = std::string_view(argv[1]) == "--admissible-order-mutant";
  try {
    rational_local_fixtures();
    native_trajectories();
    false_generalizations();
    order_sensitivity(mutant);
    order_budget_admissible(admissible_mutant);
    check(counts.local_permutations == 62 && counts.local_diameter == 32 && counts.local_23 == 2 &&
          counts.local_34 == 6 && counts.local_43 == 24 && counts.local_44 == 24 && counts.local_extra_shell == 6 &&
          counts.counterexamples == 3 && counts.native_calls == 180 && counts.native_success > 0 &&
          counts.native_refusal > 0 && counts.native_23 > 0 && counts.native_34 > 0 &&
          counts.intermediate_extra_shell > 0 && counts.order_mutants == 1 &&
          counts.admissible_order_local_calls == 8 && counts.admissible_order_native_calls == 6 &&
          counts.admissible_order_global_replays == 1 && counts.admissible_order_budget_differences == 3 &&
          counts.admissible_order_same_support == 3, "nonvacuity.all_floors");
    std::cout << "{\"schema\":\"mhgp7-private-filtered-meb-trajectory-v1\",\"status\":\"passed\","
              << "\"public_status\":\"not_claimed\",\"checks\":" << counts.checks
              << ",\"local_permutations\":" << counts.local_permutations << ",\"local_diameter\":" << counts.local_diameter
              << ",\"local_23\":" << counts.local_23 << ",\"local_34\":" << counts.local_34 << ",\"local_43\":" << counts.local_43
              << ",\"local_44\":" << counts.local_44 << ",\"local_extra_shell\":" << counts.local_extra_shell
              << ",\"prefix_calls\":" << counts.prefix_calls << ",\"native_calls\":" << counts.native_calls
              << ",\"native_success\":" << counts.native_success << ",\"native_refusal\":" << counts.native_refusal
              << ",\"replay_pivots\":" << counts.replay_pivots << ",\"replay_23\":" << counts.native_23
              << ",\"replay_34\":" << counts.native_34 << ",\"replay_intermediate_extra_shell\":" << counts.intermediate_extra_shell
              << ",\"false_domains\":" << counts.counterexamples << ",\"test_side_order_mutants\":" << counts.order_mutants
              << ",\"admissible_order_local_calls\":" << counts.admissible_order_local_calls
              << ",\"admissible_order_native_calls\":" << counts.admissible_order_native_calls
              << ",\"admissible_order_global_replays\":" << counts.admissible_order_global_replays
              << ",\"admissible_order_budget_differences\":" << counts.admissible_order_budget_differences
              << ",\"admissible_order_same_support\":" << counts.admissible_order_same_support
              << ",\"trajectory_observation\":\"test_side_replay_bound_to_native_terminals_and_q_traces\","
              << "\"order_mutant_domain\":\"outside_positive_Q_not_native_ambiguity\","
              << "\"admissible_order_mutant_domain\":\"strict_positive_Q_budget_only\","
              << "\"native_ambiguity_claim\":false,\"independent_geometry_oracle\":false,\"engine_integration\":false}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "trajectory rejected: " << error.what() << '\n';
    return 4;
  }
}
