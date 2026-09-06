// Bounded FULL producer gate: generated Gabriel catalogues are checked against
// the independent OBig oracle BEFORE they are supplied to the relative producer.
// Gamma includes every born K-facet, including isolated vertices. Oracle masks
// always index physical fixture positions, never external PointId values.
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <map>
#include <set>
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
u64 checks = 0, failures = 0, clouds = 0, orders = 0, cuts = 0;
u64 catalogues = 0, catalogue_records = 0, isolated_cuts = 0;
u64 permutations = 0, rejections = 0, named_cases = 0;
u64 shell_refusals = 0, authority_refutations = 0;
std::string context;

void check(bool good, const char* message) {
  ++checks;
  if (!good) {
    ++failures;
    std::fprintf(stderr, "FAIL [%s] %s\n", context.c_str(), message);
  }
}
ExactLevel level(u64 numerator, i128 denominator = 1) {
  return {{numerator, 0, 0}, denominator};
}
std::vector<InputPoint> input(const std::vector<P3>& pts, bool sparse = false) {
  const PointId ids[] = {std::numeric_limits<PointId>::max(), 17, 0, 902,
                        2147483648u, 3, 65536, 42};
  std::vector<InputPoint> result;
  for (size_t i = 0; i < pts.size(); ++i)
    result.push_back({sparse ? ids[i] : static_cast<PointId>(i), pts[i]});
  return result;
}
std::vector<P3> positions(const std::vector<InputPoint>& in) {
  std::vector<P3> result;
  for (const auto& p : in) result.push_back(p.position);
  return result;
}
unsigned id_bit(PointId id, const std::vector<InputPoint>& in) {
  for (size_t i = 0; i < in.size(); ++i)
    if (in[i].id == id) return 1u << i;
  check(false, "unknown external PointId in published object");
  return 0;
}
unsigned facet_mask(const FacetKey& f, const std::vector<InputPoint>& in) {
  unsigned result = 0;
  if (f.k > kFacetMaxK) { check(false, "oversized output facet"); return 0; }
  for (size_t i = 0; i < f.k; ++i) result |= id_bit(f.p[i], in);
  check(std::popcount(result) == f.k, "facet has distinct known identities");
  return result;
}
unsigned support_mask(const ForestEvent& e, const std::vector<InputPoint>& in) {
  unsigned result = 0;
  if (e.q > 11) { check(false, "oversized generated support"); return 0; }
  for (size_t i = 0; i < e.q; ++i) result |= id_bit(e.support[i], in);
  return result;
}
unsigned event_mask(const ForestEvent& e, const std::vector<InputPoint>& in) {
  unsigned result = support_mask(e, in);
  if (e.d > 9) { check(false, "oversized generated interior"); return 0; }
  for (size_t i = 0; i < e.d; ++i) result |= id_bit(e.interior[i], in);
  return result;
}
bool empty(const FullCertificate& c) {
  return c.order() == 0 && c.nodes().empty() && c.minima().empty() && c.parents().empty();
}
FullGabrielLimits roomy() {
  FullGabrielLimits result;
  result.certificate = {1000000, 1000000, 1000000};
  result.max_points = 8;
  result.max_input_records = 10000000;
  result.max_aliases = 10000000;
  result.max_face_visits = 10000000;
  result.max_portal_requests = 10000000;
  result.max_chain_steps = 10000000;
  result.max_successor_steps = 10000000;
  result.max_meb_calls = 10000000;
  result.max_query_nodes = 10000000;
  result.max_meb_supports = 10000000;
  return result;
}

struct CloudCase {
  std::string name;
  std::vector<InputPoint> in;
  CloudIndex ix;
  oracle::Oracle exact;
  std::vector<std::vector<ForestEvent>> by_card;
  bool generated = false;

  CloudCase(std::string label, std::vector<InputPoint> points, i64 separation = 8)
      : name(std::move(label)), in(std::move(points)), ix(build_cloud_index(in)),
        exact(positions(in)), by_card(in.size() + 2) {
    context = name;
    check(in.size() >= 1 && in.size() <= 8 && ix.valid, "bounded valid fixture");
    check(exact.valid, "independent Gamma oracle is total");
    if (!ix.valid || !exact.valid || in.empty() || in.size() > 8) return;
    // ONE generation/census per cloud; all requested orders share its balls.
    GenerateOptions opt;
    opt.s = separation;
    opt.smax = std::max<u64>(2, in.size());
    opt.threads = 1;
    opt.max_raw_candidates = 100000;
    GenerateStats generation;
    std::vector<BallCandidate> candidates;
    generate_candidates(ix, opt, &candidates, &generation);
    check(generation.cap_refus == kCapRefusNone, "real generator completed without refusal");
    if (generation.cap_refus != kCapRefusNone) return;
    sort_candidates(&candidates, 1);
    deduplicate_candidates(&candidates);
    ExpandStats expansion;
    std::vector<Survivor> survivors;
    std::vector<BallData> balls;
    prefilter_balls(ix, candidates, opt.smax, 1, &survivors, &expansion);
    const auto status = census_balls(ix, candidates, survivors, opt.smax, 12, 1, &balls, &expansion);
    check(status == PipelineStatus::kCompleteRegular, "real census completed");
    if (status != PipelineStatus::kCompleteRegular) return;
    for (unsigned k = 1; k < in.size(); ++k)
      expand_events_k(ix, balls, k, opt.smax - 1, 1, &by_card[k + 1], &expansion);
    generated = true;
    ++clouds;
  }

  bool judge_catalogues() const {
    const u64 before = failures;
    check(generated && exact.regular, "positive cloud has a generated regular catalogue");
    if (!generated || !exact.regular) return false;
    for (unsigned card = 2; card <= in.size(); ++card) {
      ++catalogues;
      std::map<unsigned, const ForestEvent*> actual;
      for (const auto& e : by_card[card]) {
        const unsigned mask = event_mask(e, in);
        check(std::popcount(mask) == static_cast<int>(card), "generated catalogue cardinality");
        check(actual.emplace(mask, &e).second, "no duplicate generated Gabriel simplex");
        if (mask == 0 || std::popcount(mask) != static_cast<int>(card)) continue;
        ++catalogue_records;
        check(exact.direct(mask), "generated simplex is independently Gabriel");
        check(oracle::compare(e.level, exact.ball(mask)) == 0, "independent exact catalogue level");
        check(support_mask(e, in) == exact.ball(mask).support, "independent strict MEB support");
        check(e.active_mask == (1u << e.q) - 1u, "regular catalogue exposes all essential facets");
      }
      for (unsigned mask = 1; mask < (1u << in.size()); ++mask)
        if (std::popcount(mask) == static_cast<int>(card))
          check(actual.count(mask) == static_cast<size_t>(exact.direct(mask)),
                "complete catalogue equals independent all-subset Gabriel inventory");
    }
    return failures == before;
  }
};

// Both labels and coverage are retained: equal point unions do not certify the
// same partition of minima, and overlapping point labels are not DSU vertices.
using Signature = std::vector<std::pair<std::vector<unsigned>, unsigned>>;
Signature expected_signature(const CloudCase& c, unsigned k,
                             const oracle::OracleBall& cut, bool closed) {
  Signature answer;
  for (const auto& component : c.exact.full_components(k, cut, closed)) {
    std::vector<unsigned> minima;
    unsigned coverage = 0;
    for (unsigned facet : component) {
      coverage |= facet;
      if (c.exact.direct(facet)) minima.push_back(facet);
    }
    check(!minima.empty(), "every FULL Gamma component contains a Gabriel minimum");
    if (component.size() == 1) ++isolated_cuts;
    std::sort(minima.begin(), minima.end());
    answer.push_back({std::move(minima), coverage});
  }
  std::sort(answer.begin(), answer.end());
  return answer;
}

std::vector<FullNodeId> independent_roots(const FullCertificate& f,
                                         const oracle::OracleBall& cut, bool closed) {
  std::vector<bool> active(f.nodes().size(), false);
  for (size_t i = 0; i < f.nodes().size(); ++i) {
    const auto& node = f.nodes()[i];
    const int cmp = oracle::compare(node.level, cut);
    if (cmp > 0 || (cmp == 0 && !closed)) continue;
    active[i] = true;
    for (u64 j = 0; j < node.parent_count; ++j) {
      if (node.first + j >= f.parents().size()) {
        check(false, "output parent CSR range"); continue;
      }
      const u64 parent = f.parents()[static_cast<size_t>(node.first + j)];
      if (parent >= i) { check(false, "output strictly older parent"); continue; }
      check(active[static_cast<size_t>(parent)], "merge consumes a live root at its exact cut");
      active[static_cast<size_t>(parent)] = false;
    }
  }
  std::vector<FullNodeId> answer;
  for (size_t i = 0; i < active.size(); ++i) if (active[i]) answer.push_back(i);
  return answer;
}

Signature actual_signature(const CloudCase& c, const FullCertificate& f,
                           const oracle::OracleBall& cut, bool closed) {
  Signature answer;
  for (FullNodeId root : independent_roots(f, cut, closed)) {
    std::vector<unsigned> labels;
    std::vector<FullNodeId> pending{root};
    size_t visited = 0;
    unsigned leaf_coverage = 0;
    while (!pending.empty() && visited++ <= f.nodes().size()) {
      const auto id = pending.back(); pending.pop_back();
      if (id >= f.nodes().size()) { check(false, "output descendant exists"); continue; }
      const auto& node = f.nodes()[static_cast<size_t>(id)];
      if (node.parent_count == 0) {
        if (node.first >= f.minima().size()) { check(false, "output minimum exists"); continue; }
        const unsigned mask = facet_mask(f.minima()[static_cast<size_t>(node.first)], c.in);
        labels.push_back(mask); leaf_coverage |= mask;
      } else {
        for (u64 j = 0; j < node.parent_count; ++j) {
          if (node.first + j >= f.parents().size()) { check(false, "output descendant CSR"); break; }
          pending.push_back(f.parents()[static_cast<size_t>(node.first + j)]);
        }
      }
    }
    check(pending.empty() && visited <= f.nodes().size(), "output traversal is bounded and acyclic");
    std::sort(labels.begin(), labels.end());
    check(std::adjacent_find(labels.begin(), labels.end()) == labels.end(), "minimum labels partitioned once");
    const auto coverage = full_certificate_coverage(f, root, f.nodes().size(), f.minima().size() * f.order());
    check(coverage.status == FullCertificateStatus::kOk, "public coverage reader completed");
    unsigned mask = 0;
    for (PointId id : coverage.values) mask |= id_bit(id, c.in);
    check(mask == leaf_coverage, "public coverage equals the independent leaf union");
    check(std::is_sorted(coverage.values.begin(), coverage.values.end()) &&
          std::adjacent_find(coverage.values.begin(), coverage.values.end()) == coverage.values.end(),
          "coverage has sorted unique external identities");
    answer.push_back({std::move(labels), mask});
  }
  std::sort(answer.begin(), answer.end());
  return answer;
}

void compare_full(const CloudCase& c, unsigned k, const FullGabrielResult& result) {
  check(result.status == FullGabrielStatus::kCompleteRelative, "producer completes relatively to supplied catalogues");
  if (result.status != FullGabrielStatus::kCompleteRelative) {
    std::fprintf(stderr, "producer reason: %s\n", result.reason); return;
  }
  const auto& f = result.forest;
  check(result.reason != nullptr && std::strcmp(result.reason, kFullGabrielAuthority) == 0,
        "completion explicitly retains the relative catalogue authority");
  check(f.order() == k && !empty(f), "FULL output has the requested order");
  if (empty(f) || f.order() != k) return;
  std::vector<unsigned> actual_minima, expected_minima;
  for (const auto& minimum : f.minima()) actual_minima.push_back(facet_mask(minimum, c.in));
  for (unsigned mask = 1; mask < (1u << c.in.size()); ++mask)
    if (std::popcount(mask) == static_cast<int>(k) && c.exact.direct(mask)) expected_minima.push_back(mask);
  std::sort(actual_minima.begin(), actual_minima.end());
  check(actual_minima == expected_minima, "FULL leaves are exactly all Gabriel K-facet minima");
  std::vector<unsigned> levels{1};  // singleton radius zero is an explicit initial cut
  for (unsigned mask = 1; mask < (1u << c.in.size()); ++mask)
    if (std::popcount(mask) == static_cast<int>(k) || std::popcount(mask) == static_cast<int>(k + 1))
      levels.push_back(mask);
  std::sort(levels.begin(), levels.end(), [&](unsigned a, unsigned b) {
    return oracle::compare(c.exact.ball(a), c.exact.ball(b)) < 0;
  });
  levels.erase(std::unique(levels.begin(), levels.end(), [&](unsigned a, unsigned b) {
    return oracle::compare(c.exact.ball(a), c.exact.ball(b)) == 0;
  }), levels.end());
  for (unsigned mask : levels) for (bool closed : {false, true}) {
    ++cuts;
    const auto& cut = c.exact.ball(mask);
    check(actual_signature(c, f, cut, closed) == expected_signature(c, k, cut, closed),
          "FULL Gamma cut: minimum partition AND all-facet point coverage, including isolated facets");
  }
  for (const auto& node : f.nodes()) for (bool closed : {false, true}) {
    const auto read = full_certificate_roots_at(f, node.level, closed, f.nodes().size());
    check(read.status == FullCertificateStatus::kOk, "public exact root replay completed");
    unsigned representative = 0;
    for (unsigned mask : levels)
      if (oracle::compare(node.level, c.exact.ball(mask)) == 0) { representative = mask; break; }
    check(representative != 0, "every visible FULL node is a Gamma critical value");
    if (representative != 0)
      check(read.values == independent_roots(f, c.exact.ball(representative), closed),
            "public root replay agrees with independent exact cut comparator");
  }
  ++orders;
}

bool same_certificate(const FullCertificate& a, const FullCertificate& b) {
  if (a.order() != b.order() || a.minima() != b.minima() || a.parents() != b.parents() ||
      a.nodes().size() != b.nodes().size()) return false;
  for (size_t i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i]; const auto& y = b.nodes()[i];
    if (x.level != y.level || x.first != y.first || x.parent_count != y.parent_count) return false;
  }
  return true;
}

std::vector<P3> e5() {
  return {{0, 0, 7}, {0, 9, 6}, {1, 4, 0}, {0, 0, 1}, {4, 1, 2}};
}

void oracle_named_contract(const CloudCase& c) {
  using Components = std::vector<std::vector<unsigned>>;
  if (c.name == "acute") {
    check(c.exact.full_components(2, level(13, 4), false).empty(),
          "oracle acute: no facet before the first minimum");
    check(c.exact.full_components(2, level(13, 4), true) == Components{{5}},
          "oracle acute: AC is a genuine isolated facet at 13/4");
    check(c.exact.full_components(2, level(9), true) == Components{{3}, {5}, {6}},
          "oracle acute: all three isolated facets exist before the coface");
    check(c.exact.full_components(2, level(325, 36), false) == Components{{3}, {5}, {6}} &&
          c.exact.full_components(2, level(325, 36), true) == Components{{3, 5, 6}},
          "oracle acute: coface closes one exact ternary fusion");
    check(c.exact.full_components(1, level(0), true) == Components{{1}, {2}, {4}} &&
          c.exact.full_components(1, level(0), false).empty(),
          "oracle K1 includes point minima at zero, open and closed distinguished");
  }
  if (c.name == "obtuse") {
    check(!c.exact.direct(3) && c.exact.direct(5) && c.exact.direct(6),
          "oracle obtuse: long edge is not a minimum");
    check(c.exact.full_components(2, level(9), false) == Components{{5}, {6}} &&
          c.exact.full_components(2, level(9), true) == Components{{3, 5, 6}},
          "oracle obtuse: long edge is born nonisolated at the fusion");
  }
  if (c.name == "E5") {
    check(oracle::compare(level(33, 2), c.exact.ball(5)) == 0 && !c.exact.direct(5),
          "oracle E5: AC is a non-Gabriel facet born at 33/2");
    check(c.exact.full_components(2, level(33, 2), false) == Components{{6}, {9, 12, 17, 20, 24}} &&
          c.exact.full_components(2, level(33, 2), true) == Components{{5, 9, 12, 17, 20, 24}, {6}},
          "oracle E5: silent AC changes facet inventory but neither root count nor coverage");
    check(c.exact.full_components(2, level(83886, 3563), true) == Components{{3, 5, 6, 9, 12, 17, 20, 24}},
          "oracle E5: ABC joins the correctly normalized ACDE root");
  }
}

void named_shape(const CloudCase& c, unsigned k, const FullGabrielResult& r) {
  if (r.status != FullGabrielStatus::kCompleteRelative) return;
  const auto& f = r.forest;
  if (empty(f)) { check(false, "named positive forest is nonempty"); return; }
  if (k == 1) {
    unsigned at_zero = 0;
    for (const auto& node : f.nodes())
      if (node.parent_count == 0 && node.level == level(0)) ++at_zero;
    check(at_zero == c.in.size(), "K1: every point is an isolated minimum at zero");
    ++named_cases;
  }
  if (k == c.in.size()) {
    check(f.nodes().size() == 1 && f.minima().size() == 1 && f.parents().empty(),
          "K=n: one terminal minimum, with no connection catalogue");
    ++named_cases;
  }
  if (k != 2) return;
  if (c.name == "acute") {
    check(f.minima().size() == 3 && f.nodes().size() == 4 && f.parents().size() == 3,
          "acute triangle: three isolated minima and one ternary merge");
    check(f.nodes().back().level == level(325, 36) && f.nodes().back().parent_count == 3,
          "acute triangle: exact ternary merge level 325/36");
    ++named_cases;
  }
  if (c.name == "obtuse" || c.name == "symmetric_obtuse") {
    check(f.minima().size() == 2 && f.nodes().size() == 3 && f.parents().size() == 2,
          "obtuse triangle: two minima and one binary merge, no false long-edge leaf");
    check(f.nodes().back().level == (c.name == "obtuse" ? level(9) : level(4)),
          "obtuse triangle exact merge level");
    if (c.name == "symmetric_obtuse")
      check(f.nodes()[0].level == level(5, 4) && f.nodes()[1].level == level(5, 4),
            "symmetric obtuse: two distinct simultaneous minima in one atomic batch");
    ++named_cases;
  }
  if (c.name == "E5") {
    check(f.minima().size() == 7 && f.nodes().size() == 10 && f.parents().size() == 9,
          "E5 FULL canonical size: seven minima and three ternary merges");
    const ExactLevel expected[] = {level(9, 2), level(9, 2), level(11, 2), level(162, 25),
        level(9), level(21, 2), level(189, 17), level(31, 2), level(41, 2), level(83886, 3563)};
    if (f.nodes().size() == 10) {
      for (size_t i = 0; i < 10; ++i) {
        check(f.nodes()[i].level == expected[i], "E5 canonical exact node level");
        check(f.nodes()[i].parent_count == (i == 3 || i == 6 || i == 9 ? 3u : 0u),
              "E5 canonical ternary parent degree");
        check(f.nodes()[i].level != level(33, 2), "E5 silent AC incidence creates no node at 33/2");
      }
    }
    check(f.parents() == std::vector<FullNodeId>({0, 1, 2, 3, 4, 5, 6, 7, 8}),
          "E5 historical CDE anchor is normalized through ADE before ABC fusion");
    check(r.stats.portal_requests > 0 && r.stats.chain_steps > 0 && r.stats.terminal_direct > 0 &&
          r.stats.normalized_anchors > 0,
          "E5 named portal AC exercises a strict chain and a historical terminal normalization");
    check(r.stats.no_op_connections > 0, "E5 later BCE connection is structurally a no-op");
    ++named_cases;
  }
}

void qualify_cloud(CloudCase& c, bool permute = false) {
  context = c.name;
  if (!c.judge_catalogues()) return;
  oracle_named_contract(c);
  for (unsigned k = 1; k <= c.in.size(); ++k) {
    context = c.name + "/K=" + std::to_string(k);
    const auto& minima = c.by_card[k];  // card1 is deliberately empty: K1 births are automatic.
    const auto& direct = c.by_card[k + 1];
    const auto result = build_full_gabriel_order(c.ix, k, minima, direct, roomy());
    compare_full(c, k, result);
    named_shape(c, k, result);
    if (permute) {
      auto reversed_input = c.in;
      std::reverse(reversed_input.begin(), reversed_input.end());
      const auto reversed_index = build_cloud_index(reversed_input);
      auto reverse_minima = minima, reverse_direct = direct;
      std::reverse(reverse_minima.begin(), reverse_minima.end());
      std::reverse(reverse_direct.begin(), reverse_direct.end());
      const auto changed = build_full_gabriel_order(reversed_index, k, reverse_minima, reverse_direct, roomy());
      compare_full(c, k, changed);
      check(same_certificate(result.forest, changed.forest),
            "physical input and catalogue permutations preserve the canonical certificate");
      ++permutations;
    }
  }
}

void positive_gate() {
  CloudCase singleton("singleton", input({{7, 11, 13}}, true));
  qualify_cloud(singleton, true);
  CloudCase pair("pair", input({{0, 0, 0}, {6, 0, 0}}, true));
  qualify_cloud(pair, true);
  CloudCase acute("acute", input({{0, 0, 0}, {6, 0, 0}, {2, 3, 0}}));
  qualify_cloud(acute, true);
  CloudCase obtuse("obtuse", input({{0, 0, 0}, {6, 0, 0}, {1, 1, 0}}));
  qualify_cloud(obtuse);
  CloudCase symmetric("symmetric_obtuse", input({{0, 0, 0}, {4, 0, 0}, {2, 1, 0}}));
  qualify_cloud(symmetric, true);
  CloudCase counterexample("E5", input(e5()));
  qualify_cloud(counterexample, true);
  CloudCase sparse("E5_sparse_ids", input(e5(), true));
  qualify_cloud(sparse, true);
  for (i64 s : {10, 12}) {
    CloudCase separation("E5_s=" + std::to_string(s), input(e5()), s);
    qualify_cloud(separation);
  }
  // Fixed permanent coordinates, not a rejection-sampled or tuned random cloud.
  const std::vector<P3> u16{{31052, 37054, 53791}, {63099, 62295, 5489},
      {45851, 18621, 10092}, {32290, 41054, 26270}, {35795, 23044, 15792},
      {22475, 26532, 25195}, {55919, 55323, 7531}, {60817, 37898, 64418}};
  CloudCase large("u16_eight", input(u16, true));
  qualify_cloud(large, true);
}

void refused(const FullGabrielResult& r, FullGabrielStatus status, const char* reason) {
  ++rejections;
  check(r.status == status, "rejection has the exact expected status");
  check(r.reason != nullptr && std::strcmp(r.reason, reason) == 0, reason);
  check(empty(r.forest), "rejection publishes no partial forest or stale order");
}

void budget_gate(const CloudCase& c) {
  context = "E5/budgets";
  const auto& minima = c.by_card[2];
  const auto& direct = c.by_card[3];
  const auto baseline = build_full_gabriel_order(c.ix, 2, minima, direct, roomy());
  compare_full(c, 2, baseline);
  if (baseline.status != FullGabrielStatus::kCompleteRelative) return;
  const auto& s = baseline.stats;
  u64 faces = 0;
  for (const auto& e : direct) faces += 2u * e.q + e.d;
  check(s.input_records == minima.size() + direct.size(), "input budget counts both catalogues once");
  check(s.face_visits == faces, "face budget charges strict first pass and all-face second pass");
  check(s.meb_calls == s.geometry.meb_calls, "physical MEB calls agree across wrapper and F helper");
  check(s.geometry.core_records == 0 && s.geometry.core_facets == 0 && s.geometry.added_cofaces == 0,
        "FULL producer did not invoke the historical core or silent-coface materializer");
  struct Budget {
    u64 FullGabrielLimits::* member;
    u64 used;
    const char* reason;
  };
  const Budget budgets[] = {
      {&FullGabrielLimits::max_points, c.in.size(), "full_gabriel_point_budget"},
      {&FullGabrielLimits::max_input_records, s.input_records, "full_gabriel_input_budget"},
      {&FullGabrielLimits::max_aliases, s.aliases, "full_gabriel_alias_budget"},
      {&FullGabrielLimits::max_face_visits, s.face_visits, "full_gabriel_face_budget"},
      {&FullGabrielLimits::max_portal_requests, s.portal_requests, "full_gabriel_portal_budget"},
      {&FullGabrielLimits::max_chain_steps, s.chain_steps, "full_gabriel_chain_budget"},
      {&FullGabrielLimits::max_successor_steps, s.successor_steps, "full_gabriel_successor_budget"},
      {&FullGabrielLimits::max_meb_calls, s.meb_calls, "full_gabriel_meb_call_budget"},
      {&FullGabrielLimits::max_query_nodes, s.geometry.query_nodes, "silent_query_node_budget"},
      {&FullGabrielLimits::max_meb_supports, s.geometry.meb_supports, "silent_meb_support_budget"}};
  for (const auto& budget : budgets) {
    check(budget.used > 0, "named E5 budget is nonvacuously consumed");
    if (budget.used == 0) continue;
    auto caps = roomy();
    caps.*(budget.member) = budget.used;
    const auto exact = build_full_gabriel_order(c.ix, 2, minima, direct, caps);
    check(exact.status == FullGabrielStatus::kCompleteRelative &&
          same_certificate(exact.forest, baseline.forest), "exact physical budget boundary is accepted");
    for (u64 cap : {u64{0}, budget.used - 1}) {
      caps.*(budget.member) = cap;
      refused(build_full_gabriel_order(c.ix, 2, minima, direct, caps),
              FullGabrielStatus::kResourceExhausted, budget.reason);
    }
  }
  u64 batches = 0;
  for (size_t i = 0; i < baseline.forest.nodes().size(); ++i)
    if (i == 0 || baseline.forest.nodes()[i - 1].level != baseline.forest.nodes()[i].level) ++batches;
  struct CertificateBudget {
    u64 FullCertificateLimits::* member;
    u64 used;
    const char* reason;
  };
  const CertificateBudget certificates[] = {
      {&FullCertificateLimits::max_batches, batches, "full_gabriel_batch_budget"},
      {&FullCertificateLimits::max_nodes, baseline.forest.nodes().size(), "full_gabriel_node_budget"},
      {&FullCertificateLimits::max_parent_refs, baseline.forest.parents().size(), "full_gabriel_parent_budget"}};
  for (const auto& budget : certificates) {
    auto caps = roomy();
    caps.certificate.*(budget.member) = budget.used;
    const auto exact = build_full_gabriel_order(c.ix, 2, minima, direct, caps);
    check(exact.status == FullGabrielStatus::kCompleteRelative &&
          same_certificate(exact.forest, baseline.forest), "exact certificate resource boundary is accepted");
    for (u64 cap : {u64{0}, budget.used - 1}) {
      caps.certificate.*(budget.member) = cap;
      refused(build_full_gabriel_order(c.ix, 2, minima, direct, caps),
              FullGabrielStatus::kResourceExhausted, budget.reason);
    }
  }
  refused(build_full_gabriel_order(c.ix, 2, minima, direct, FullGabrielLimits{}),
          FullGabrielStatus::kResourceExhausted, "full_gabriel_point_budget");
}

void malformed_gate(const CloudCase& c) {
  context = "E5/malformed";
  const auto& minima = c.by_card[2];
  const auto& direct = c.by_card[3];
  const auto reject = [&](const CloudIndex& ix, unsigned k, const std::vector<ForestEvent>& m,
                          const std::vector<ForestEvent>& d, const char* reason) {
    refused(build_full_gabriel_order(ix, k, m, d, roomy()), FullGabrielStatus::kInvalidInput, reason);
  };
  auto invalid = c.ix;
  invalid.valid = false;
  reject(invalid, 2, minima, direct, "full_gabriel_invalid_index_or_order");
  reject(build_cloud_index(std::vector<P3>{}), 1, {}, {}, "full_gabriel_invalid_index_or_order");
  reject(build_cloud_index(std::vector<P3>{{1, 2, 3}, {1, 2, 3}}), 1, {}, {},
         "full_gabriel_invalid_index_or_order");
  reject(build_cloud_index(std::vector<P3>{{-1, 0, 0}, {1, 2, 3}}), 1, {}, {},
         "full_gabriel_invalid_index_or_order");
  reject(build_cloud_index(std::vector<InputPoint>{{7, {0, 0, 0}}, {7, {1, 2, 3}}}), 1, {}, {},
         "full_gabriel_invalid_index_or_order");
  for (unsigned k : {0u, 6u, 11u, std::numeric_limits<unsigned>::max()})
    reject(c.ix, k, minima, direct, "full_gabriel_invalid_index_or_order");
  reject(c.ix, 1, minima, c.by_card[2], "full_gabriel_k1_minimum_catalogue");
  reject(c.ix, 2, {}, direct, "full_gabriel_missing_minima");
  // Both catalogue roles traverse the same checked parser. No oversized field
  // is ever interpreted by the test's mask adapters after it is corrupted.
  for (bool minimum_role : {false, true}) {
    for (unsigned mutation = 0; mutation < 15; ++mutation) {
      auto m = minima, d = direct;
      auto& source = minimum_role ? m : d;
      check(!source.empty(), "malformed catalogue fixture is nonempty");
      if (source.empty()) continue;
      auto& e = source[0];
      switch (mutation) {
        case 0: e.q = 0; break;
        case 1: e.q = 1; break;
        case 2: e.q = 5; break;
        case 3: e.q = 255; break;
        case 4: e.d = 10; break;
        case 5: e.d = 255; break;
        case 6: ++e.d; break;
        case 7: e.active_mask = 0; break;
        case 8: e.active_mask = 1; break;
        case 9: e.active_mask = static_cast<u16>(1u << e.q); break;
        case 10: e.level.den = 0; break;
        case 11: e.level.den = -1; break;
        case 12: e.level = level(0); break;
        case 13: e.support[1] = e.support[0]; break;
        case 14: e.q = 11; e.d = 1; break;
      }
      reject(c.ix, 2, m, d, "full_gabriel_invalid_record");
    }
    auto m = minima, d = direct;
    auto& source = minimum_role ? m : d;
    source[0].support[0] = std::numeric_limits<PointId>::max();
    reject(c.ix, 2, m, d, "full_gabriel_unknown_point");
    m = minima; d = direct;
    auto& duplicate_source = minimum_role ? m : d;
    duplicate_source.push_back(duplicate_source.front());
    reject(c.ix, 2, m, d, "full_gabriel_duplicate_record");
    // Equal rationals represented by different bytes are still one simplex.
    auto& duplicate = duplicate_source.back();
    check(duplicate.level.num[1] == 0 && duplicate.level.num[2] == 0 &&
          duplicate.level.num[0] < std::numeric_limits<u64>::max() / 2,
          "fraction-equivalent duplicate fixture is safely doubled");
    duplicate.level.num[0] *= 2;
    duplicate.level.den *= 2;
    reject(c.ix, 2, m, d, "full_gabriel_duplicate_record");
  }
  auto interior_duplicate = direct;
  const auto at = std::find_if(interior_duplicate.begin(), interior_duplicate.end(),
                               [](const ForestEvent& e) { return e.d != 0; });
  check(at != interior_duplicate.end(), "E5 has a nonempty interior record");
  if (at != interior_duplicate.end()) {
    at->interior[0] = at->support[0];
    reject(c.ix, 2, minima, interior_duplicate, "full_gabriel_invalid_record");
  }
}

void causal_gate(const CloudCase& c) {
  context = "E5/causal";
  const auto& minima = c.by_card[2];
  const auto& direct = c.by_card[3];
  auto missing = direct;
  missing.erase(std::remove_if(missing.begin(), missing.end(), [&](const ForestEvent& e) {
    return event_mask(e, c.in) == 28u;  // CDE, the historical portal terminal.
  }), missing.end());
  check(missing.size() + 1 == direct.size(), "removed exactly the named CDE direct terminal");
  refused(build_full_gabriel_order(c.ix, 2, minima, missing, roomy()),
          FullGabrielStatus::kInvariantViolated, "full_gabriel_terminal_missing");
  auto mismatch = direct;
  for (auto& e : mismatch) if (event_mask(e, c.in) == 28u) e.level = level(13, 2);
  refused(build_full_gabriel_order(c.ix, 2, minima, mismatch, roomy()),
          FullGabrielStatus::kInvariantViolated, "full_gabriel_terminal_level_mismatch");

  CloudCase acute("acute_causal", input({{0, 0, 0}, {6, 0, 0}, {2, 3, 0}}));
  if (!acute.judge_catalogues()) return;
  auto absent_minimum = acute.by_card[2];
  absent_minimum.pop_back();
  refused(build_full_gabriel_order(acute.ix, 2, absent_minimum, acute.by_card[3], roomy()),
          FullGabrielStatus::kInvariantViolated, "full_gabriel_missing_prior_alias");
  auto simultaneous = acute.by_card[3];
  simultaneous[0].level = level(13, 4);
  refused(build_full_gabriel_order(acute.ix, 2, acute.by_card[2], simultaneous, roomy()),
          FullGabrielStatus::kInvariantViolated, "full_gabriel_facet_not_strict");
  // The relative API does NOT authenticate its catalogue-completeness
  // precondition. The independently checked three acute minima are unchanged,
  // but the unique ABC connection is omitted. Relative completion is expected;
  // the external FULL Gamma judge must nevertheless refute the resulting tree.
  check(acute.by_card[2].size() == 3 && acute.by_card[3].size() == 1,
        "authority sentinel retains all three minima and drops exactly ABC");
  const auto incomplete = build_full_gabriel_order(acute.ix, 2, acute.by_card[2], {}, roomy());
  check(incomplete.status == FullGabrielStatus::kCompleteRelative &&
        incomplete.reason != nullptr &&
        std::strcmp(incomplete.reason, kFullGabrielAuthority) == 0,
        "incomplete external catalogue is not silently assigned geometric authority");
  if (incomplete.status == FullGabrielStatus::kCompleteRelative) {
    const auto& cut = acute.exact.ball(7);
    const bool refuted = actual_signature(acute, incomplete.forest, cut, true) !=
                         expected_signature(acute, 2, cut, true);
    check(refuted,
          "independent Gamma judge kills incomplete-catalogue promotion despite relative completion");
    if (refuted) ++authority_refutations;
  }
}

void shell_gate() {
  CloudCase c("right_triangle_invalid_catalogue", input({{0, 0, 0}, {2, 0, 0}, {1, 1, 0}}));
  // This is deliberately NOT qualified as a regular cloud or catalogue. C is
  // on the foreign shell of the diameter ball AB: centre=(1,0,0), beta=1.
  // Keep only the two short-edge minima; force ABC's nominal strict mask and
  // move its level to 2 so its missing AB facet must enter the lazy portal.
  // Thus regularity, completeness and exact event level are knowingly violated.
  // The expected unsupported status comes from a REAL geometric shell query,
  // not from a synthetic status, an injected helper or parser-only rejection.
  check(c.generated && c.exact.valid && !c.exact.regular, "shell fixture is valid Gamma but explicitly nonregular");
  if (!c.generated || !c.exact.valid || c.exact.regular) return;
  check(oracle::compare(level(1), c.exact.ball(3)) == 0 &&
        c.exact.ball(3).power(c.in[2].position).sign() == 0,
        "independent oracle certifies C on the foreign shell of AB");
  std::vector<ForestEvent> minima;
  for (const auto& e : c.by_card[2])
    if (event_mask(e, c.in) == 5u || event_mask(e, c.in) == 6u) minima.push_back(e);
  auto direct = c.by_card[3];
  check(minima.size() == 2 && direct.size() == 1 && direct[0].q == 3 && direct[0].d == 0,
        "real generated right-triangle records seed the explicitly invalid catalogue");
  if (minima.size() != 2 || direct.size() != 1 || direct[0].q != 3 || direct[0].d != 0) return;
  direct[0].active_mask = 7;
  direct[0].level = level(2);
  const auto result = build_full_gabriel_order(c.ix, 2, minima, direct, roomy());
  refused(result, FullGabrielStatus::kUnsupportedDegeneracy, "silent_external_shell");
  check(result.stats.portal_requests == 1 && result.stats.meb_calls == 1 &&
        result.stats.geometry.meb_calls == 1 && result.stats.geometry.query_nodes > 0,
        "shell refusal physically entered one portal, one MEB and a nonempty spatial query");
  if (result.status == FullGabrielStatus::kUnsupportedDegeneracy &&
      result.reason != nullptr && std::strcmp(result.reason, "silent_external_shell") == 0 &&
      result.stats.portal_requests == 1 && result.stats.meb_calls == 1 &&
      result.stats.geometry.query_nodes > 0) ++shell_refusals;
}

void reject_gate() {
  CloudCase c("E5", input(e5()));
  if (!c.judge_catalogues()) return;
  budget_gate(c);
  malformed_gate(c);
  causal_gate(c);
  shell_gate();
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::strcmp(argv[1], "--selftest") != 0 && std::strcmp(argv[1], "--rejects") != 0))
    return 2;
  const bool rejects = std::strcmp(argv[1], "--rejects") == 0;
  mhgp7_oracle::clear_overflow();
  try {
    positive_gate();
    if (rejects) reject_gate();
  } catch (const std::exception& e) {
    ++failures;
    std::fprintf(stderr, "FAIL exception [%s]: %s\n", context.c_str(), e.what());
  }
  check(!mhgp7_oracle::overflow_seen(), "independent OBig oracle has no overflow");
  const bool floor = clouds >= 10 && catalogues >= 30 && catalogue_records >= 50 &&
      orders >= 60 && cuts >= 200 && isolated_cuts >= 50 && permutations >= 20 && named_cases >= 20 &&
      (!rejects || (rejections >= 80 && shell_refusals == 1 && authority_refutations == 1));
  std::printf("full_gabriel mode=%s authority=relative_supplied_catalogues oracle=OBig_FULL "
              "clouds=%llu catalogues=%llu records=%llu orders=%llu cuts=%llu isolated_cuts=%llu "
              "permutations=%llu named=%llu rejections=%llu shell_refusals=%llu authority_refutations=%llu "
              "checks=%llu failures=%llu floor=%u\n",
              argv[1], (unsigned long long)clouds, (unsigned long long)catalogues,
              (unsigned long long)catalogue_records, (unsigned long long)orders,
              (unsigned long long)cuts, (unsigned long long)isolated_cuts,
              (unsigned long long)permutations, (unsigned long long)named_cases,
              (unsigned long long)rejections, (unsigned long long)shell_refusals,
              (unsigned long long)authority_refutations, (unsigned long long)checks,
              (unsigned long long)failures, floor ? 1u : 0u);
  return failures != 0 ? 1 : floor ? 0 : 3;
}
