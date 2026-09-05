// Separate bounded qualification of lazy first-C aliases, not a default change.
// Explicit test-helper port from full_gabriel_gate.cpp at SHA256
// 577a689b729a55039609b64873cd5a9bd006969305824f7b3eebac2708501b1f.
// Independent OBig FULL oracle full_gamma.hpp at SHA256
// a17732d2bd7861a3e7e3f76d029da3b2078ce4ebf0b64f7d7571e5060de24f0c.
// Lazy proof: audits/receipts_full_producer_20260905/lazy_alias_next_step_review.md,
// SHA256 418dfd4728f063cec0e65573d026215f2dcdb7897d4343c635a81614e1fa6324.
// The two-step fixture is explicitly promoted from private search stdout
// 77e89c7c560ca5991ed8d67956ba47c17cc47ff846bd83e7ebd9c59c54011af7;
// that search was NOT part of the previous seven CTests.
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../oracle/full_gamma.hpp"
#include "../src/forest/full_gabriel.hpp"
#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp7;
namespace oracle = mhgp7_full_oracle;
namespace {
u64 checks = 0, failures = 0, clouds = 0, order_cases = 0, lazy_runs = 0;
u64 cuts = 0, catalogue_records = 0, rejections = 0, permutations = 0;
u64 named_j1 = 0, named_two_step = 0, named_cache_hit = 0;
u64 observed_skips = 0, observed_hits = 0, terminal_orders = 0;
std::string context;
void check(bool good, const char* message) {
  ++checks;
  if (!good) { ++failures; std::fprintf(stderr, "FAIL [%s] %s\n", context.c_str(), message); }
}
ExactLevel level(u64 n, i128 d = 1) { return {{n, 0, 0}, d}; }
FullGabrielLimits limits(bool lazy = true) {
  FullGabrielLimits c;
  c.certificate = {1000000, 1000000, 1000000};
  c.max_points = c.max_input_records = c.max_face_visits = 10000000;
  c.max_portal_requests = c.max_chain_steps = c.max_successor_steps = 10000000;
  c.max_meb_calls = c.max_query_nodes = c.max_meb_supports = 10000000;
  c.max_aliases = lazy ? 0 : 10000000;
  return c;
}
std::vector<InputPoint> input(const std::vector<P3>& points, bool sparse = false) {
  const PointId ids[] = {std::numeric_limits<PointId>::max(), 17, 0, 902,
                        2147483648u, 3, 65536, 42};
  std::vector<InputPoint> result;
  for (size_t i = 0; i < points.size(); ++i)
    result.push_back({sparse ? ids[i] : static_cast<PointId>(i), points[i]});
  return result;
}
std::vector<P3> positions(const std::vector<InputPoint>& in) {
  std::vector<P3> result;
  for (const auto& p : in) result.push_back(p.position);
  return result;
}
unsigned bit(PointId id, const std::vector<InputPoint>& in) {
  for (size_t i = 0; i < in.size(); ++i) if (in[i].id == id) return 1u << i;
  check(false, "known external PointId"); return 0;
}
unsigned mask(const FacetKey& f, const std::vector<InputPoint>& in) {
  if (f.k > kFacetMaxK) { check(false, "facet bounds"); return 0; }
  unsigned result = 0;
  for (size_t i = 0; i < f.k; ++i) result |= bit(f.p[i], in);
  check(std::popcount(result) == f.k, "distinct known facet identities");
  return result;
}
unsigned support(const ForestEvent& e, const std::vector<InputPoint>& in) {
  if (e.q > 11) { check(false, "support bounds"); return 0; }
  unsigned result = 0;
  for (size_t i = 0; i < e.q; ++i) result |= bit(e.support[i], in);
  return result;
}
unsigned mask(const ForestEvent& e, const std::vector<InputPoint>& in) {
  if (e.d > 9) { check(false, "interior bounds"); return 0; }
  unsigned result = support(e, in);
  for (size_t i = 0; i < e.d; ++i) result |= bit(e.interior[i], in);
  return result;
}
bool empty(const FullCertificate& f) {
  return f.order() == 0 && f.nodes().empty() && f.minima().empty() && f.parents().empty();
}
bool same(const FullCertificate& a, const FullCertificate& b) {
  if (a.order() != b.order() || a.minima() != b.minima() || a.parents() != b.parents() ||
      a.nodes().size() != b.nodes().size()) return false;
  for (size_t i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i]; const auto& y = b.nodes()[i];
    if (x.level != y.level || x.first != y.first || x.parent_count != y.parent_count) return false;
  }
  return true;
}
bool policy(const FullGabrielResult& r, bool lazy) {
  return r.alias_policy != nullptr && std::strcmp(r.alias_policy, lazy ?
      "lazy_first_c_strict_resolutions_v1" : "eager_all_incident_facets_v1") == 0;
}

struct Cloud {
  std::string name;
  std::vector<InputPoint> in;
  CloudIndex ix;
  oracle::Oracle exact;
  std::vector<std::vector<ForestEvent>> catalogue;
  bool valid = false;
  Cloud(std::string label, std::vector<InputPoint> points)
      : name(std::move(label)), in(std::move(points)), ix(build_cloud_index(in)),
        exact(positions(in)), catalogue(in.size() + 2) {
    context = name;
    const u64 before = failures;
    check(in.size() >= 1 && in.size() <= 8 && ix.valid, "bounded checked index");
    check(exact.valid && exact.regular, "independent GLOBAL regularity required, never relaxed");
    if (!ix.valid || !exact.valid || !exact.regular || in.empty() || in.size() > 8) return;
    GenerateOptions opt;
    opt.s = 8; opt.smax = std::max<u64>(2, in.size()); opt.threads = 1;
    opt.max_raw_candidates = 100000;
    GenerateStats gs;
    std::vector<BallCandidate> candidates;
    generate_candidates(ix, opt, &candidates, &gs);
    check(gs.cap_refus == kCapRefusNone && gs.invariant_jneg == 0, "real generation completed");
    for (size_t q = 0; q < 3; ++q)
      check(gs.ledger_emitted_mass[q] + gs.ledger_killed_mass[q] == expected_pair_mass(ix),
            "real generation closes every pair ledger");
    if (failures != before) return;
    sort_candidates(&candidates, 1); deduplicate_candidates(&candidates);
    ExpandStats es;
    std::vector<Survivor> survivors;
    std::vector<BallData> balls;
    prefilter_balls(ix, candidates, opt.smax, 1, &survivors, &es);
    const auto status = census_balls(ix, candidates, survivors, opt.smax, 12, 1, &balls, &es);
    check(status == PipelineStatus::kCompleteRegular, "real census completed");
    if (status != PipelineStatus::kCompleteRegular) return;
    for (unsigned k = 1; k < in.size(); ++k)
      expand_events_k(ix, balls, k, opt.smax - 1, 1, &catalogue[k + 1], &es);
    for (unsigned card = 2; card <= in.size(); ++card) {
      std::map<unsigned, const ForestEvent*> actual;
      for (const auto& event : catalogue[card]) {
        const unsigned m = mask(event, in);
        check(std::popcount(m) == static_cast<int>(card) && actual.emplace(m, &event).second,
              "unique generated labels at the requested cardinality");
        if (m == 0 || std::popcount(m) != static_cast<int>(card)) continue;
        ++catalogue_records;
        check(exact.direct(m) && oracle::compare(event.level, exact.ball(m)) == 0 &&
              support(event, in) == exact.ball(m).support,
              "independent Gabriel membership, exact level and essential support");
        check(event.active_mask == (1u << event.q) - 1u, "all essential removals are strict");
      }
      for (unsigned m = 1; m < (1u << in.size()); ++m)
        if (std::popcount(m) == static_cast<int>(card))
          check(actual.count(m) == static_cast<size_t>(exact.direct(m)), "independent catalogue completeness");
    }
    valid = failures == before;
    if (valid) ++clouds;
  }
};

using Signature = std::vector<std::pair<std::vector<unsigned>, unsigned>>;
Signature expected(const Cloud& c, unsigned k, const oracle::OracleBall& cut, bool closed) {
  Signature result;
  for (const auto& component : c.exact.full_components(k, cut, closed)) {
    std::vector<unsigned> labels;
    unsigned coverage = 0;
    for (unsigned facet : component) {
      coverage |= facet;
      if (c.exact.direct(facet)) labels.push_back(facet);
    }
    check(!labels.empty(), "every FULL component contains a minimum");
    std::sort(labels.begin(), labels.end());
    result.emplace_back(std::move(labels), coverage);
  }
  std::sort(result.begin(), result.end());
  return result;
}
std::vector<FullNodeId> roots(const FullCertificate& f, const oracle::OracleBall& cut, bool closed) {
  std::vector<bool> active(f.nodes().size(), false);
  for (size_t i = 0; i < f.nodes().size(); ++i) {
    const auto& node = f.nodes()[i];
    const int cmp = oracle::compare(node.level, cut);
    if (cmp > 0 || (cmp == 0 && !closed)) continue;
    active[i] = true;
    for (u64 j = 0; j < node.parent_count; ++j) {
      if (node.first + j >= f.parents().size()) { check(false, "CSR bounds"); break; }
      const auto parent = f.parents()[static_cast<size_t>(node.first + j)];
      if (parent >= i) { check(false, "strictly older parent"); continue; }
      check(active[static_cast<size_t>(parent)], "parent is a live pre-lot root");
      active[static_cast<size_t>(parent)] = false;
    }
  }
  std::vector<FullNodeId> result;
  for (size_t i = 0; i < active.size(); ++i) if (active[i]) result.push_back(i);
  return result;
}
Signature actual(const Cloud& c, const FullCertificate& f, const oracle::OracleBall& cut, bool closed) {
  Signature result;
  for (FullNodeId root : roots(f, cut, closed)) {
    std::vector<unsigned> labels;
    std::vector<FullNodeId> stack{root};
    size_t visited = 0;
    unsigned coverage = 0;
    while (!stack.empty() && visited++ <= f.nodes().size()) {
      const auto id = stack.back(); stack.pop_back();
      if (id >= f.nodes().size()) { check(false, "descendant bounds"); continue; }
      const auto& node = f.nodes()[static_cast<size_t>(id)];
      if (node.parent_count == 0) {
        if (node.first >= f.minima().size()) { check(false, "minimum bounds"); continue; }
        const unsigned m = mask(f.minima()[static_cast<size_t>(node.first)], c.in);
        labels.push_back(m); coverage |= m;
      } else for (u64 j = 0; j < node.parent_count; ++j) {
        if (node.first + j >= f.parents().size()) { check(false, "descendant CSR"); break; }
        stack.push_back(f.parents()[static_cast<size_t>(node.first + j)]);
      }
    }
    check(stack.empty() && visited <= f.nodes().size(), "bounded acyclic traversal");
    std::sort(labels.begin(), labels.end());
    check(std::adjacent_find(labels.begin(), labels.end()) == labels.end(), "unique minima in each component");
    const auto read = full_certificate_coverage(f, root, f.nodes().size(), f.minima().size() * f.order());
    check(read.status == FullCertificateStatus::kOk, "public coverage reader completed");
    unsigned read_mask = 0;
    for (PointId id : read.values) read_mask |= bit(id, c.in);
    check(read_mask == coverage && std::is_sorted(read.values.begin(), read.values.end()) &&
          std::adjacent_find(read.values.begin(), read.values.end()) == read.values.end(),
          "public coverage is the sorted distinct union of minimum labels");
    result.emplace_back(std::move(labels), coverage);
  }
  std::sort(result.begin(), result.end());
  return result;
}
void compare_full(const Cloud& c, unsigned k, const FullCertificate& f) {
  check(f.order() == k && !empty(f), "requested nonempty FULL forest");
  if (empty(f) || f.order() != k) return;
  std::vector<unsigned> levels{1};
  for (unsigned m = 1; m < (1u << c.in.size()); ++m)
    if (std::popcount(m) == static_cast<int>(k) || std::popcount(m) == static_cast<int>(k + 1))
      levels.push_back(m);
  std::sort(levels.begin(), levels.end(), [&](unsigned a, unsigned b) {
    return oracle::compare(c.exact.ball(a), c.exact.ball(b)) < 0;
  });
  levels.erase(std::unique(levels.begin(), levels.end(), [&](unsigned a, unsigned b) {
    return oracle::compare(c.exact.ball(a), c.exact.ball(b)) == 0;
  }), levels.end());
  for (unsigned m : levels) for (bool closed : {false, true}) {
    ++cuts;
    check(actual(c, f, c.exact.ball(m), closed) == expected(c, k, c.exact.ball(m), closed),
          "independent FULL cut: partition of minimum labels AND all-facet point coverage");
  }
  for (const auto& node : f.nodes()) {
    unsigned representative = 0;
    for (unsigned m : levels) if (oracle::compare(node.level, c.exact.ball(m)) == 0) { representative = m; break; }
    check(representative != 0, "published node belongs to the Gamma critical grid");
    if (representative == 0) continue;
    for (bool closed : {false, true}) {
      const auto read = full_certificate_roots_at(f, node.level, closed, f.nodes().size());
      check(read.status == FullCertificateStatus::kOk &&
            read.values == roots(f, c.exact.ball(representative), closed), "public exact root replay");
    }
  }
}

void coherent(const FullGabrielResult& r, u64 capacity) {
  const auto& s = r.stats;
  check(policy(r, true), "literal lazy policy is distinct from eager");
  check(s.aliases == 0 && s.alias_hits == 0, "legacy eager alias counters stay zero in lazy");
  check(s.minimum_lookups == s.face_visits && s.minimum_hits + s.cache_lookups == s.minimum_lookups,
        "every charged strict request first consults mandatory minima");
  check(s.cache_hits + s.portal_requests == s.cache_lookups,
        "every cache miss pays a persistent portal request");
  check(s.cache_inserts + s.cache_skips == s.portal_requests && s.cache_inserts <= capacity,
        "first-C insertions are resident entries; saturation bypasses without eviction");
  check(s.meb_calls == s.portal_requests + s.chain_steps && s.geometry.meb_calls == s.meb_calls,
        "MEB physical calendar is misses plus replacements, never an uncharged J1 Q0");
  check(s.direct_lookups == s.singleton_intruder_resolutions + s.chain_steps &&
        s.terminal_direct == s.portal_requests && s.singleton_intruder_resolutions <= s.portal_requests,
        "terminal lookups and successful dispatches have separate causal counts");
  check(s.geometry.core_records == 0 && s.geometry.core_facets == 0 && s.geometry.added_cofaces == 0,
        "lazy never calls the historical Gamma-core producer");
  if (capacity == 0)
    check(s.cache_inserts == 0 && s.cache_hits == 0 && s.cache_skips == s.portal_requests,
          "zero cache is a complete uncached computation, not a resource refusal");
  observed_skips += s.cache_skips;
  observed_hits += s.cache_hits;
}
bool success(const FullGabrielResult& r, bool lazy) {
  check(policy(r, lazy), "policy survives every successful call");
  const bool good = r.status == FullGabrielStatus::kCompleteRelative && r.reason != nullptr &&
      std::strcmp(r.reason, kFullGabrielAuthority) == 0;
  check(good, "relative supplied-catalogue completion, no authority promotion");
  if (!good) std::fprintf(stderr, "reason=%s\n", r.reason == nullptr ? "null" : r.reason);
  return good;
}
std::vector<P3> j1() { return {{0,5,0}, {4,5,0}, {2,6,0}, {2,0,0}}; }
std::vector<P3> e5() { return {{0,0,7}, {0,9,6}, {1,4,0}, {0,0,1}, {4,1,2}}; }
std::vector<P3> two_step() {
  return {{622,745,858}, {839,341,867}, {111,242,715}, {827,10,537},
          {437,578,984}, {396,213,30}, {693,305,961}, {814,71,415}};
}
void named(const Cloud& c, unsigned k, const FullGabrielResult& r, u64 capacity) {
  if (k == c.in.size()) {
    check(r.forest.minima().size() == 1 && r.forest.nodes().size() == 1 && r.forest.parents().empty(),
          "K=n retains its mandatory terminal minimum even with zero cache");
    ++terminal_orders;
  }
  if (k == 1) check(r.stats.portal_requests == 0 && r.stats.cache_inserts == 0,
                   "K1 mandatory point minima require no cache or geometry");
  if (k != 2) return;
  if (c.name == "J1" || c.name == "J1_sparse") {
    check(r.forest.minima().size() == 4 && r.forest.nodes().size() == 6 && r.forest.parents().size() == 5,
          "J1: four minima, one binary and one ternary merge, no AB minimum");
    check(r.stats.singleton_intruder_resolutions == 1 && r.stats.portal_requests == 1 &&
          r.stats.chain_steps == 0 && r.stats.meb_calls == 1 && r.stats.direct_lookups == 1,
          "named AB uses one certified census and the prior ABC anchor without a Q0 MEB");
    check(oracle::compare(level(4), c.exact.ball(3)) == 0 && !c.exact.direct(3), "AB is the nonminimum J1 facet");
    if (r.forest.nodes().size() == 6) {
      check(oracle::compare(r.forest.nodes()[2].level, c.exact.ball(7)) == 0 &&
            oracle::compare(r.forest.nodes()[5].level, c.exact.ball(11)) == 0 &&
            r.forest.nodes()[2].parent_count == 2 && r.forest.nodes()[5].parent_count == 3,
            "ABC closes at 4; ABW consumes its prior group at 841/100");
    }
    ++named_j1;
  }
  if (c.name == "two_step") {
    check(r.stats.max_chain_length >= 2 && r.stats.chain_steps >= 2, "named second portal iteration is non-vacuous");
    check(c.exact.ball(194).support == 192 && !c.exact.direct(194) &&
          c.exact.ball(138).support == 130 && c.exact.direct(138), "named intermediate and terminal support masks");
    unsigned intruders = 0;
    for (unsigned p = 0; p < 8; ++p)
      if (!(194u & (1u << p)) && c.exact.ball(194).power(c.in[p].position).sign() < 0) intruders |= 1u << p;
    check(intruders == 8, "second-iteration intermediate coface has exactly the single intruder 3");
    check(oracle::compare(level(687389,4), c.exact.ball(129)) == 0 &&
          oracle::compare(level(367513,4), c.exact.ball(194)) == 0 &&
          oracle::compare(level(277829,4), c.exact.ball(138)) == 0, "named two exact strict decreases");
    ++named_two_step;
  }
  if (c.name == "J1_shared_lot") {
    // V=(2,1,3). ABW and ABV both have beta=841/100; their cross powers
    // are 21/5. C has power 6/5 in ABW, and 9/25 in ABV, all strictly positive.
    check(c.exact.direct(11) && c.exact.direct(19) &&
          oracle::compare(level(841,100), c.exact.ball(11)) == 0 &&
          oracle::compare(c.exact.ball(11), c.exact.ball(19)) == 0,
          "two distinct regular directes in one lot share the strict AB request");
    if (capacity == 0)
      check(r.stats.singleton_intruder_resolutions >= 2, "zero cache independently resolves both same-lot J1 requests");
    else {
      check(r.stats.cache_hits > 0, "same-lot strict AB request reuses a certified pre-lot cache token");
      ++named_cache_hit;
    }
  }
  if (c.name == "E5") check(r.stats.normalized_anchors > 0 && r.stats.no_op_connections > 0,
                            "E5 preserves historical normalization and no-op direct anchors");
}
void qualify(Cloud& c, bool permute = false) {
  if (!c.valid) return;
  for (unsigned k = 1; k <= c.in.size(); ++k) {
    context = c.name + "/K=" + std::to_string(k);
    ++order_cases;
    const auto eager = build_full_gabriel_order(c.ix, k, c.catalogue[k], c.catalogue[k+1], limits(false));
    if (!success(eager, false)) continue;
    compare_full(c, k, eager.forest);
    check(eager.stats.minimum_lookups == 0 && eager.stats.minimum_hits == 0 &&
          eager.stats.cache_lookups == 0 && eager.stats.cache_hits == 0 &&
          eager.stats.cache_inserts == 0 && eager.stats.cache_skips == 0 &&
          eager.stats.singleton_intruder_resolutions == 0, "eager does not opt into lazy dispatch or accounting");
    const u64 direct_count = c.catalogue[k+1].size();
    check(eager.stats.aliases + eager.stats.face_visits ==
          eager.forest.minima().size() + 2 * (k + 1) * direct_count + eager.stats.portal_requests,
          "regular eager alias identity counts minima, equal incidences and unique portals");
    for (u64 capacity : {0ull, 1ull, 1000000ull}) {
      const auto r = build_full_gabriel_order_lazy(c.ix, k, c.catalogue[k], c.catalogue[k+1], limits(), {capacity});
      if (!success(r, true)) continue;
      ++lazy_runs;
      check(same(eager.forest, r.forest), "eager/lazy literal full certificate equality");
      compare_full(c, k, r.forest);
      coherent(r, capacity);
      if (capacity == 1000000) {
        check(capacity >= 4 * direct_count && r.stats.cache_skips == 0,
              "large cache exceeds every strict request and never saturates");
        check(r.stats.portal_requests == eager.stats.portal_requests + r.stats.singleton_intruder_resolutions &&
              r.stats.chain_steps == eager.stats.chain_steps &&
              r.stats.meb_calls == eager.stats.meb_calls + r.stats.singleton_intruder_resolutions,
              "unsaturated lazy differs from eager only by the named J1 miss work");
      }
      named(c, k, r, capacity);
      if (permute && k == 2) {
        auto in = c.in; std::reverse(in.begin(), in.end());
        const auto ix = build_cloud_index(in);
        auto minima = c.catalogue[k], direct = c.catalogue[k+1];
        std::reverse(minima.begin(), minima.end()); std::reverse(direct.begin(), direct.end());
        const auto changed = build_full_gabriel_order_lazy(ix, k, minima, direct, limits(), {capacity});
        if (success(changed, true)) {
          check(same(r.forest, changed.forest), "first-C physical/catalogue permutations preserve canonical forests");
          compare_full(c, k, changed.forest);
          coherent(changed, capacity);
          ++permutations;
        }
      }
    }
  }
}

void refused(const FullGabrielResult& r, FullGabrielStatus status, const char* reason) {
  ++rejections;
  check(policy(r, true), "lazy policy retained on refusal");
  check(r.status == status && r.reason != nullptr && std::strcmp(r.reason, reason) == 0, reason);
  check(empty(r.forest), "refusal clears order and every public arena");
  check(r.stats.aliases == 0 && r.stats.alias_hits == 0, "refusal never reinterprets eager alias counters");
}
void budget_gate(const Cloud& c) {
  if (!c.valid) return;
  context = "lazy/budgets/" + c.name;
  for (u64 capacity : {0ull, 1ull, 1000000ull}) {
    const auto r = build_full_gabriel_order_lazy(c.ix, 2, c.catalogue[2], c.catalogue[3], limits(), {capacity});
    if (!success(r, true)) continue;
    struct Budget { u64 FullGabrielLimits::*field; u64 observed; const char* reason; };
    const Budget budgets[] = {
      {&FullGabrielLimits::max_points, c.in.size(), "full_gabriel_point_budget"},
      {&FullGabrielLimits::max_input_records, r.stats.input_records, "full_gabriel_input_budget"},
      {&FullGabrielLimits::max_face_visits, r.stats.face_visits, "full_gabriel_face_budget"},
      {&FullGabrielLimits::max_portal_requests, r.stats.portal_requests, "full_gabriel_portal_budget"},
      {&FullGabrielLimits::max_chain_steps, r.stats.chain_steps, "full_gabriel_chain_budget"},
      {&FullGabrielLimits::max_successor_steps, r.stats.successor_steps, "full_gabriel_successor_budget"},
      {&FullGabrielLimits::max_meb_calls, r.stats.meb_calls, "full_gabriel_meb_call_budget"},
      {&FullGabrielLimits::max_query_nodes, r.stats.geometry.query_nodes, "silent_query_node_budget"},
      {&FullGabrielLimits::max_meb_supports, r.stats.geometry.meb_supports, "silent_meb_support_budget"}};
    for (const auto& budget : budgets) {
      if (budget.observed == 0) continue;
      auto exact = limits(); exact.*(budget.field) = budget.observed;
      const auto at = build_full_gabriel_order_lazy(c.ix, 2, c.catalogue[2], c.catalogue[3], exact, {capacity});
      check(success(at, true) && same(r.forest, at.forest), "observed physical budget is sufficient");
      for (u64 short_cap : std::array<u64, 2>{0, budget.observed - 1}) {
        auto short_limits = limits(); short_limits.*(budget.field) = short_cap;
        refused(build_full_gabriel_order_lazy(c.ix, 2, c.catalogue[2], c.catalogue[3], short_limits, {capacity}),
                FullGabrielStatus::kResourceExhausted, budget.reason);
      }
    }
  }
}
void reject_gate(const Cloud& c) {
  if (!c.valid) return;
  context = "J1/lazy/rejects";
  const auto& m = c.catalogue[2]; const auto& d = c.catalogue[3];
  for (u64 conflict : std::array<u64, 2>{1, std::numeric_limits<u64>::max()}) {
    auto bad = limits(); bad.max_aliases = conflict;
    const auto r = build_full_gabriel_order_lazy(c.ix, 2, m, d, bad, {0});
    refused(r, FullGabrielStatus::kInvalidInput, "full_gabriel_lazy_alias_budget_conflict");
    check(r.stats.input_records == 0 && r.stats.meb_calls == 0, "conflicting policy refused before scientific work");
  }
  for (unsigned k : {0u, 5u, 11u})
    refused(build_full_gabriel_order_lazy(c.ix, k, m, d, limits(), {1}),
            FullGabrielStatus::kInvalidInput, "full_gabriel_invalid_index_or_order");
  refused(build_full_gabriel_order_lazy(c.ix, 1, m, c.catalogue[2], limits(), {0}),
          FullGabrielStatus::kInvalidInput, "full_gabriel_k1_minimum_catalogue");
  refused(build_full_gabriel_order_lazy(c.ix, 2, {}, d, limits(), {0}),
          FullGabrielStatus::kInvalidInput, "full_gabriel_missing_minima");
  for (u64 capacity : {0ull, 1ull, 1000000ull}) {
    auto missing = d;
    missing.erase(std::remove_if(missing.begin(), missing.end(), [&](const ForestEvent& e) {
      return mask(e, c.in) == 7;
    }), missing.end());
    check(missing.size() + 1 == d.size(), "remove exactly ABC, not its permanent minima");
    refused(build_full_gabriel_order_lazy(c.ix, 2, m, missing, limits(), {capacity}),
            FullGabrielStatus::kInvariantViolated, "full_gabriel_terminal_missing");
    auto mismatch = d;
    for (auto& e : mismatch) if (mask(e, c.in) == 7) e.level = level(9,2);
    refused(build_full_gabriel_order_lazy(c.ix, 2, m, mismatch, limits(), {capacity}),
            FullGabrielStatus::kInvariantViolated, "full_gabriel_terminal_level_mismatch");
    auto absent_minimum = m;
    absent_minimum.erase(std::remove_if(absent_minimum.begin(), absent_minimum.end(), [&](const ForestEvent& e) {
      return mask(e, c.in) == 5;
    }), absent_minimum.end());
    refused(build_full_gabriel_order_lazy(c.ix, 2, absent_minimum, d, limits(), {capacity}),
            FullGabrielStatus::kInvariantViolated, "full_gabriel_minimum_missing");
    auto future_minimum = m;
    for (auto& e : future_minimum) if (mask(e, c.in) == 5) e.level = level(4);
    refused(build_full_gabriel_order_lazy(c.ix, 2, future_minimum, d, limits(), {capacity}),
            FullGabrielStatus::kInvariantViolated, "full_gabriel_minimum_not_prior");
    auto no_chain = limits(); no_chain.max_chain_steps = 0;
    const auto j = build_full_gabriel_order_lazy(c.ix, 2, m, d, no_chain, {capacity});
    check(success(j, true) && j.stats.singleton_intruder_resolutions == 1 && j.stats.meb_calls == 1,
          "valid J1 succeeds at chain cap zero, still charges exactly one MEB");
  }
  const auto old = build_full_gabriel_order(c.ix, 2, m, d, limits());
  check(policy(old, false) && old.status == FullGabrielStatus::kResourceExhausted &&
        old.reason != nullptr && std::strcmp(old.reason, "full_gabriel_alias_budget") == 0 && empty(old.forest),
        "eager API retains its original hard alias budget and default policy");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::strcmp(argv[1], "--selftest") != 0 && std::strcmp(argv[1], "--rejects") != 0)) return 2;
  const bool rejects = std::strcmp(argv[1], "--rejects") == 0;
  mhgp7_oracle::clear_overflow();
  try {
    check(std::strcmp(kFullGabrielLazyAliases, "lazy_first_c_strict_resolutions_v1") == 0 &&
          std::strcmp(kFullGabrielEagerAliases, "eager_all_incident_facets_v1") == 0, "literal policy constants");
    Cloud single("single", input({{7,11,13}}, true)); qualify(single);
    Cloud one("J1", input(j1())); qualify(one);
    Cloud sparse("J1_sparse", input(j1(), true)); qualify(sparse, true);
    Cloud original("E5", input(e5())); qualify(original);
    Cloud chain("two_step", input(two_step())); qualify(chain, true);
    auto shared_points = j1(); shared_points.push_back({2,1,3});
    Cloud shared("J1_shared_lot", input(shared_points)); qualify(shared, true);
    if (rejects) { reject_gate(one); budget_gate(original); budget_gate(chain); }
  } catch (const std::exception& error) {
    ++failures; std::fprintf(stderr, "FAIL exception [%s]: %s\n", context.c_str(), error.what());
  }
  check(!mhgp7_oracle::overflow_seen(), "independent OBig arithmetic has no overflow");
  const bool floor = clouds == 6 && order_cases == 27 && lazy_runs == 81 && cuts >= 500 &&
      catalogue_records >= 50 && permutations == 9 && terminal_orders == 18 &&
      named_j1 == 6 && named_two_step == 3 && named_cache_hit == 2 && observed_hits > 0 &&
      observed_skips > 0 && (!rejects || rejections >= 100);
  std::printf("full_gabriel_lazy mode=%s clouds=%llu orders=%llu lazy_runs=%llu cuts=%llu records=%llu "
              "permutations=%llu terminal_orders=%llu named_J1=%llu named_two_step=%llu named_cache_hit=%llu "
              "cache_hits=%llu cache_skips=%llu rejections=%llu checks=%llu failures=%llu floor=%d\n",
              argv[1], (unsigned long long)clouds, (unsigned long long)order_cases, (unsigned long long)lazy_runs,
              (unsigned long long)cuts, (unsigned long long)catalogue_records, (unsigned long long)permutations,
              (unsigned long long)terminal_orders, (unsigned long long)named_j1, (unsigned long long)named_two_step,
              (unsigned long long)named_cache_hit, (unsigned long long)observed_hits, (unsigned long long)observed_skips,
              (unsigned long long)rejections, (unsigned long long)checks, (unsigned long long)failures, (int)floor);
  return failures != 0 ? 1 : floor ? 0 : 3;
}
