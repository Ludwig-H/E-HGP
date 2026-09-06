// Product-path composition only: the FULL Gamma oracle, not the P=0 arm,
// judges geometry and completeness on these bounded, globally regular clouds.
// Shared fixtures at SHA256 600701d340b88ad66efda7ade6a3c403bc1db4c194c8a33fd5a4a5a789432f84;
// named clouds from lazy_gate 6c325c8ba63dd8f2182085e1b3c539842ebbf4849322835b0dd585215a8048b6
// and singleton_gate 58ed306873bf2ab564849cffcaedfb05528f634e25b42cfba0d9ea10fcc6f4c8.
// No allocation injection or fast-q4 dispatch claim: those have separate gates.
#include <array>
#include <cstring>
#include <exception>

#include "full_gabriel_singleton_fixtures.hpp"

#if defined(MHGP7_TESTING)
#error "FULL MEB composition must exercise the product build without MHGP7_TESTING"
#endif

using namespace mhgp7_singleton_test;
namespace {
constexpr u64 kLarge = 10000000;
constexpr std::array<u64, 4> kProposalCaps{0, 1, 3, kLarge};
constexpr const char* kAccounting = "reference_ordinal_plus_native_z_q3_q4_proposal_v2";
struct Route { bool lazy; u64 cache; };
constexpr std::array<Route, 4> kRoutes{{{false, 0}, {true, 0}, {true, 1}, {true, 1000000}}};
u64 full_calls = 0, orders = 0, matrix_calls = 0, pairs = 0, gamma_runs = 0;
u64 terminal_cases = 0, chain_cases = 0, q4_records = 0, cache_hit_cases = 0;
u64 persistent_cases = 0, saved_reference_cases = 0, metadata_rejects = 0;
u64 certified[4]{}, fallback[4]{}, exact_caps[5]{}, short_caps[5]{};
u64 retained_proposal_refusals = 0;

bool text_equal(const char* a, const char* b) {
  return a != nullptr && b != nullptr && std::strcmp(a, b) == 0;
}
auto proposal_work(const FullGabrielStats& s) {
  const auto& p = s.meb_proposal;
  return std::tie(p.meb_proposal_supports, p.pivots, p.certified, p.fallback, p.reference_supports);
}
FullGabrielLimits limits(Route route, u64 p) {
  auto caps = roomy(route.lazy);
  caps.max_meb_proposal_supports = p;
  return caps;
}
void metadata(const FullGabrielResult& r, Route route) {
  check(text_equal(r.alias_policy, route.lazy ? kFullGabrielLazyAliases : kFullGabrielEagerAliases),
        "eager/lazy policy retained on every exit");
  check(text_equal(r.successor_accounting, kFullGabrielSuccessorAccounting),
        "successor accounting unchanged");
  check(text_equal(kFullGabrielMebAccounting, kAccounting) && text_equal(r.meb_accounting, kAccounting),
        "explicit literal filtered-v2 MEB accounting on every exit");
}
void bounded(const FullGabrielResult& r, const FullGabrielLimits& caps) {
  const auto& s = r.stats;
  const auto& p = s.meb_proposal;
  check(s.meb_calls <= caps.max_meb_calls && s.chain_steps <= caps.max_chain_steps &&
        s.successor_steps <= caps.max_successor_steps && s.geometry.query_nodes <= caps.max_query_nodes &&
        s.geometry.meb_supports <= caps.max_meb_supports, "public work remains prospectively bounded");
  check(p.meb_proposal_supports <= caps.max_meb_proposal_supports,
        "P belongs to the entire FULL order, never a fresh per-MEB allowance");
  check(p.reference_supports <= s.geometry.meb_supports, "physical F forms A do not exceed charged ordinal work c");
  check(s.geometry.meb_calls <= s.meb_calls && p.certified <= s.geometry.meb_calls &&
        p.fallback <= s.geometry.meb_calls - p.certified, "partial proposal dispatch counts bounded by actual MEB calls");
  check(p.certified <= p.meb_proposal_supports, "every native certificate consumed at least one persistent P form");
  if (caps.max_meb_proposal_supports == 0)
    check(p.meb_proposal_supports == 0 && p.pivots == 0 && p.certified == 0 &&
          p.reference_supports == s.geometry.meb_supports, "P=0 retains only F work with A=c");
  if (r.status == FullGabrielStatus::kCompleteRelative)
    check(p.certified + p.fallback == s.geometry.meb_calls && s.geometry.meb_calls == s.meb_calls,
          "every completed MEB used exactly one certified or fallback route");
}
FullGabrielResult run(const Cloud& c, unsigned k, Route route, const FullGabrielLimits& caps) {
  ++full_calls;
  auto r = route.lazy ? build_full_gabriel_order_lazy(c.ix, k, c.catalogue[k], c.catalogue[k+1], caps, {route.cache})
                      : build_full_gabriel_order(c.ix, k, c.catalogue[k], c.catalogue[k+1], caps);
  metadata(r, route); bounded(r, caps);
  return r;
}
bool success(const FullGabrielResult& r) {
  const bool good = r.status == FullGabrielStatus::kCompleteRelative && text_equal(r.reason, kFullGabrielAuthority);
  check(good, "completed relative to independently checked supplied catalogues, without authority promotion");
  if (!good) std::fprintf(stderr, "reason=%s\n", r.reason == nullptr ? "null" : r.reason);
  return good;
}
void compare(const FullGabrielResult& reference, const FullGabrielResult& r) {
  check(r.status == reference.status && text_equal(r.reason, reference.reason), "P does not change the public terminal");
  check(same_forest(reference.forest, r.forest), "literal FULL certificate equality including raw levels and parent CSR");
  check(work(reference.stats) == work(r.stats), "all 33 legacy FULL/geometry counters equal, including on refusals");
}
void gamma(const Cloud& c, unsigned k, const FullGabrielResult& r) {
  ++gamma_runs;
  compare_oracle(c, k, r.forest);
}
void observe(const Cloud& c, unsigned k, size_t ri, u64 cap, const FullGabrielResult& r) {
  const auto& s = r.stats; const auto& p = s.meb_proposal;
  certified[ri] += p.certified; fallback[ri] += p.fallback;
  if (p.reference_supports < s.geometry.meb_supports) ++saved_reference_cases;
  if (cap == 3 && p.meb_proposal_supports == 3 && p.certified > 0 && p.fallback > 0 && s.meb_calls > 1)
    ++persistent_cases;
  if (s.cache_hits > 0) ++cache_hit_cases;
  if (k == 1) check(s.meb_calls == 0 && p.meb_proposal_supports == 0, "K=1 performs no proposal or reference MEB");
  if (k == c.in.size()) {
    ++terminal_cases;
    check(r.forest.minima().size() == 1 && r.forest.nodes().size() == 1 && r.forest.parents().empty(),
          "K=n retains the mandatory terminal minimum");
  }
  if (c.name.find("two_step/") == 0 && k == 2) {
    ++chain_cases;
    check(s.chain_steps >= 2 && s.max_chain_length >= 2, "native proposal composed with an actual multi-step portal chain");
  }
}
void qualify(const Cloud& c) {
  if (!c.valid) return;
  for (const auto& catalogue : c.catalogue)
    for (const auto& e : catalogue) if (e.q == 4) ++q4_records;
  for (unsigned k = 1; k <= c.in.size(); ++k) {
    ++orders;
    for (size_t ri = 0; ri < kRoutes.size(); ++ri) {
      const auto route = kRoutes[ri];
      context = c.name + "/K=" + std::to_string(k) + "/route=" + std::to_string(ri) + "/P=0";
      const auto implicit = roomy(route.lazy);  // Do not set the new member here.
      check(implicit.max_meb_proposal_supports == 0, "existing callers retain default-disabled proposals");
      const auto base = run(c, k, route, implicit);
      if (!success(base)) continue;
      ++matrix_calls; gamma(c, k, base); observe(c, k, ri, 0, base);
      for (u64 p : {u64{1}, u64{3}, kLarge}) {
        context = c.name + "/K=" + std::to_string(k) + "/route=" + std::to_string(ri) + "/P=" + std::to_string(p);
        const auto r = run(c, k, route, limits(route, p));
        compare(base, r);
        if (!success(r)) continue;
        ++matrix_calls; ++pairs; gamma(c, k, r); observe(c, k, ri, p, r);
      }
    }
  }
}
void refusal(const FullGabrielResult& r, FullGabrielStatus status, const char* reason) {
  check(r.status == status && text_equal(r.reason, reason), "exact public refusal status and reason");
  check(empty(r.forest), "refusal clears order and every public forest arena");
}
void budgets(const Cloud& c) {
  if (!c.valid) return;
  for (size_t ri = 0; ri < kRoutes.size(); ++ri) {
    const auto route = kRoutes[ri];
    const auto baseline = run(c, 2, route, limits(route, 0));
    if (!success(baseline)) continue;
    struct Budget { u64 FullGabrielLimits::* field; u64 used; const char* reason; };
    const std::array<Budget, 5> tests{{
      {&FullGabrielLimits::max_meb_calls, baseline.stats.meb_calls, "full_gabriel_meb_call_budget"},
      {&FullGabrielLimits::max_meb_supports, baseline.stats.geometry.meb_supports, "silent_meb_support_budget"},
      {&FullGabrielLimits::max_chain_steps, baseline.stats.chain_steps, "full_gabriel_chain_budget"},
      {&FullGabrielLimits::max_successor_steps, baseline.stats.successor_steps, "full_gabriel_successor_budget"},
      {&FullGabrielLimits::max_query_nodes, baseline.stats.geometry.query_nodes, "silent_query_node_budget"}}};
    for (u64 p : kProposalCaps) {
      const auto nominal = run(c, 2, route, limits(route, p));
      compare(baseline, nominal);
      if (!success(nominal)) continue;
      for (size_t bi = 0; bi < tests.size(); ++bi) {
        const auto& test = tests[bi];
        if (test.used == 0) continue;
        context = c.name + "/budget=" + test.reason + "/route=" + std::to_string(ri) + "/P=" + std::to_string(p);
        auto caps = limits(route, p); caps.*(test.field) = test.used;
        const auto exact = run(c, 2, route, caps);
        compare(nominal, exact);
        check(success(exact) && proposal_work(exact.stats) == proposal_work(nominal.stats),
              "exact public budget preserves all five proposal counters too");
        ++exact_caps[bi];
        caps.*(test.field) = test.used - 1;
        const auto short_result = run(c, 2, route, caps);
        refusal(short_result, FullGabrielStatus::kResourceExhausted, test.reason);
        auto old_caps = caps; old_caps.max_meb_proposal_supports = 0;
        compare(run(c, 2, route, old_caps), short_result);
        if (short_result.stats.meb_proposal.meb_proposal_supports > 0) ++retained_proposal_refusals;
        ++short_caps[bi];
      }
    }
  }
}
void reject_metadata(const Cloud& c) {
  if (!c.valid) return;
  for (const auto route : kRoutes) for (u64 p : kProposalCaps) {
    context = c.name + "/invalid-order/P=" + std::to_string(p);
    const auto r = run(c, 0, route, limits(route, p));
    refusal(r, FullGabrielStatus::kInvalidInput, "full_gabriel_invalid_index_or_order");
    check(r.stats.meb_calls == 0 && proposal_work(r.stats) == proposal_work(FullGabrielStats{}),
          "input refusal precedes all proposal/reference work");
    ++metadata_rejects;
    if (route.lazy) {
      auto caps = limits(route, p); caps.max_aliases = 1;
      const auto conflict = run(c, 2, route, caps);
      refusal(conflict, FullGabrielStatus::kInvalidInput, "full_gabriel_lazy_alias_budget_conflict");
      check(conflict.stats.meb_calls == 0 && proposal_work(conflict.stats) == proposal_work(FullGabrielStats{}),
            "conflicting cache policy cannot spend P");
      ++metadata_rejects;
    }
  }
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::strcmp(argv[1], "--selftest") != 0 && std::strcmp(argv[1], "--rejects") != 0)) return 2;
  const bool rejects = std::strcmp(argv[1], "--rejects") == 0;
  try {
    check(FullGabrielLimits{}.max_meb_proposal_supports == 0, "proposal opt-in is disabled by default");
    const std::vector<P3> j1{{0,5,0},{4,5,0},{2,6,0},{2,0,0}};
    const std::vector<P3> e5{{0,0,7},{0,9,6},{1,4,0},{0,0,1},{4,1,2}};
    const std::vector<P3> chain{{622,745,858},{839,341,867},{111,242,715},{827,10,537},
                              {437,578,984},{396,213,30},{693,305,961},{814,71,415}};
    struct Scene { const char* name; std::vector<P3> points; bool sparse; };
    const std::array<Scene, 7> scenes{{
      {"single", {{7,11,13}}, true}, {"J1", j1, false}, {"J1_sparse", j1, true},
      {"E5", e5, false}, {"two_step", chain, false},
      {"shared", {{0,50,0},{40,50,0},{20,61,0},{20,0,0},{20,10,30}}, false},
      {"tetra", {{20,20,20},{24,24,20},{24,20,24},{20,24,24}}, false}}};
    for (i64 s : {8, 10, 12}) for (const auto& scene : scenes) {
      Cloud c(std::string(scene.name) + "/s=" + std::to_string(s), input(scene.points, scene.sparse), s);
      qualify(c);
      if (rejects && s == 8 && (std::strcmp(scene.name, "E5") == 0 || std::strcmp(scene.name, "two_step") == 0)) budgets(c);
      if (rejects && s == 8 && std::strcmp(scene.name, "J1") == 0) reject_metadata(c);
    }
  } catch (const std::exception& e) {
    ++failures; std::fprintf(stderr, "FAIL exception [%s]: %s\n", context.c_str(), e.what());
  }
  check(!mhgp7_oracle::overflow_seen(), "sticky independent OBig arithmetic remained in range");
  check(admitted_clouds == 21 && orders == 93 && matrix_calls == 1488 && pairs == 1116 && gamma_runs == 1488 &&
        terminal_cases == 336 && chain_cases == 48 && q4_records > 0 && cache_hit_cases > 0 &&
        persistent_cases > 0 && saved_reference_cases > 0 && records >= 100 && cuts >= 1000,
        "non-vacuous regular/Gamma/q4-catalogue/chain/cache/persistent-P composition floors");
  for (size_t ri = 0; ri < kRoutes.size(); ++ri)
    check(certified[ri] > 0 && fallback[ri] > 0, "both native certification and F fallback exercised on every route");
  if (rejects) {
    check(metadata_rejects == 28 && retained_proposal_refusals > 0, "metadata rejection and spent-P refusal floors");
    for (size_t bi = 0; bi < 5; ++bi)
      check(exact_caps[bi] > 0 && exact_caps[bi] == short_caps[bi], "every public budget has exact and one-short witnesses");
  }
  std::printf("{\"schema\":\"mhgp7-full-gabriel-meb-gate-v1\",\"status\":\"%s\",\"public_status\":\"not_claimed\","
              "\"test_mode\":\"%s\",\"meb_accounting\":\"%s\",\"clouds\":%llu,\"orders\":%llu,\"matrix_calls\":%llu,"
              "\"pairs\":%llu,\"full_calls\":%llu,\"gamma_runs\":%llu,\"cuts\":%llu,\"records\":%llu,"
              "\"terminals\":%llu,\"chain_cases\":%llu,\"q4_catalogue_records\":%llu,\"cache_hit_cases\":%llu,"
              "\"persistent_cases\":%llu,\"saved_reference_cases\":%llu,\"metadata_rejects\":%llu,"
              "\"retained_proposal_refusals\":%llu,\"checks\":%llu,\"failures\":%llu,\"budgets\":[",
      failures == 0 ? "passed" : "failed", argv[1], kAccounting,
      (unsigned long long)admitted_clouds, (unsigned long long)orders, (unsigned long long)matrix_calls,
      (unsigned long long)pairs, (unsigned long long)full_calls, (unsigned long long)gamma_runs,
      (unsigned long long)cuts, (unsigned long long)records, (unsigned long long)terminal_cases,
      (unsigned long long)chain_cases, (unsigned long long)q4_records, (unsigned long long)cache_hit_cases,
      (unsigned long long)persistent_cases, (unsigned long long)saved_reference_cases,
      (unsigned long long)metadata_rejects, (unsigned long long)retained_proposal_refusals,
      (unsigned long long)checks, (unsigned long long)failures);
  for (size_t bi = 0; bi < 5; ++bi)
    std::printf("%s{\"exact\":%llu,\"one_short\":%llu}", bi == 0 ? "" : ",",
                (unsigned long long)exact_caps[bi], (unsigned long long)short_caps[bi]);
  std::printf("],\"dispatch\":[");
  for (size_t ri = 0; ri < kRoutes.size(); ++ri)
    std::printf("%s{\"certified\":%llu,\"fallback\":%llu}", ri == 0 ? "" : ",",
                (unsigned long long)certified[ri], (unsigned long long)fallback[ri]);
  std::puts("]}");
  return failures == 0 ? 0 : 1;
}
