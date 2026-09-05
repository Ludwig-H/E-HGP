// Test-only helpers explicitly adapted from full_gabriel_lazy_gate.cpp at
// 6c325c8ba63dd8f2182085e1b3c539842ebbf4849322835b0dd585215a8048b6.
// Independent exhaustive Gamma oracle full_gamma.hpp at
// a17732d2bd7861a3e7e3f76d029da3b2078ce4ebf0b64f7d7571e5060de24f0c.
// The retained general lot path is only a differential, never this oracle.
#pragma once

#include <algorithm>
#include <bit>
#include <cstdio>
#include <limits>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "../oracle/full_gamma.hpp"
#include "../src/forest/full_gabriel.hpp"
#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"

namespace mhgp7_singleton_test {
using namespace mhgp7;
namespace oracle = mhgp7_full_oracle;
inline u64 checks = 0, failures = 0, cuts = 0, admitted_clouds = 0, records = 0;
inline std::string context;
inline void check(bool ok, const char* message) {
  ++checks;
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL [%s] %s\n", context.c_str(), message); }
}
inline std::vector<InputPoint> input(const std::vector<P3>& p, bool sparse = false) {
  const PointId ids[] = {std::numeric_limits<PointId>::max(), 17, 0, 902,
                        2147483648u, 3, 65536, 42};
  std::vector<InputPoint> out;
  for (size_t i = 0; i < p.size(); ++i) out.push_back({sparse ? ids[i] : static_cast<PointId>(i), p[i]});
  return out;
}
inline std::vector<P3> positions(const std::vector<InputPoint>& in) {
  std::vector<P3> out;
  for (const auto& p : in) out.push_back(p.position);
  return out;
}
inline unsigned bit(PointId id, const std::vector<InputPoint>& in) {
  for (size_t i = 0; i < in.size(); ++i) if (in[i].id == id) return 1u << i;
  check(false, "known external PointId"); return 0;
}
inline unsigned label(const FacetKey& f, const std::vector<InputPoint>& in) {
  unsigned out = 0;
  if (f.k > kFacetMaxK) { check(false, "bounded facet key"); return 0; }
  for (size_t j = 0; j < f.k; ++j) out |= bit(f.p[j], in);
  check(std::popcount(out) == f.k, "distinct identities in facet label");
  return out;
}
inline unsigned support(const ForestEvent& e, const std::vector<InputPoint>& in) {
  unsigned out = 0;
  if (e.q > 4) { check(false, "bounded support"); return 0; }
  for (size_t j = 0; j < e.q; ++j) out |= bit(e.support[j], in);
  return out;
}
inline unsigned label(const ForestEvent& e, const std::vector<InputPoint>& in) {
  unsigned out = support(e, in);
  if (e.d > 9) { check(false, "bounded interior"); return 0; }
  for (size_t j = 0; j < e.d; ++j) out |= bit(e.interior[j], in);
  return out;
}
inline bool empty(const FullCertificate& f) {
  return f.order() == 0 && f.nodes().empty() && f.minima().empty() && f.parents().empty();
}
inline bool same_forest(const FullCertificate& a, const FullCertificate& b) {
  if (a.order() != b.order() || a.minima() != b.minima() || a.parents() != b.parents() ||
      a.nodes().size() != b.nodes().size()) return false;
  for (size_t i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i]; const auto& y = b.nodes()[i];
    if (x.level != y.level || x.first != y.first || x.parent_count != y.parent_count) return false;
  }
  return true;
}
inline auto work(const FullGabrielStats& s) {
  return std::tie(s.input_records, s.face_visits, s.aliases, s.alias_hits,
      s.portal_requests, s.chain_steps, s.terminal_direct, s.max_chain_length,
      s.normalized_anchors, s.successor_steps, s.no_op_connections, s.meb_calls,
      s.minimum_lookups, s.minimum_hits, s.cache_lookups, s.cache_hits,
      s.cache_inserts, s.cache_skips, s.singleton_intruder_resolutions, s.direct_lookups,
      s.geometry.core_records, s.geometry.core_facets, s.geometry.facets_with_two_intruders,
      s.geometry.chain_steps, s.geometry.added_cofaces, s.geometry.terminal_direct,
      s.geometry.terminal_cached, s.geometry.max_chain_length, s.geometry.query_nodes,
      s.geometry.query_leaves, s.geometry.query_range_skips, s.geometry.meb_calls,
      s.geometry.meb_supports);
}
inline FullGabrielLimits roomy(bool lazy) {
  FullGabrielLimits c;
  c.certificate = {1000000, 1000000, 1000000};
  c.max_points = c.max_input_records = c.max_face_visits = 10000000;
  c.max_portal_requests = c.max_chain_steps = c.max_successor_steps = 10000000;
  c.max_meb_calls = c.max_query_nodes = c.max_meb_supports = 10000000;
  c.max_aliases = lazy ? 0 : 10000000;
  return c;
}

struct Cloud {
  std::string name;
  std::vector<InputPoint> in;
  CloudIndex ix;
  oracle::Oracle exact;
  std::vector<std::vector<ForestEvent>> catalogue;
  bool valid = false;
  Cloud(std::string n, std::vector<InputPoint> p, i64 separation = 8)
      : name(std::move(n)), in(std::move(p)), ix(build_cloud_index(in)),
        exact(positions(in)), catalogue(in.size() + 2) {
    context = name;
    const auto before = failures;
    check(in.size() >= 1 && in.size() <= 8 && ix.valid, "bounded authentic index");
    check(exact.valid && exact.regular, "independent GLOBAL regularity, never relaxed");
    if (!ix.valid || !exact.valid || !exact.regular) return;
    GenerateOptions opt;
    opt.s = separation; opt.smax = std::max<u64>(2, in.size()); opt.threads = 1;
    opt.max_raw_candidates = 100000;
    GenerateStats gs;
    std::vector<BallCandidate> candidates;
    generate_candidates(ix, opt, &candidates, &gs);
    check(gs.cap_refus == kCapRefusNone && gs.invariant_jneg == 0, "real bounded generation completed");
    for (size_t q = 0; q < 3; ++q)
      check(gs.ledger_emitted_mass[q] + gs.ledger_killed_mass[q] == expected_pair_mass(ix),
            "actual generated pair ledger closes");
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
      std::map<unsigned, const ForestEvent*> found;
      for (const auto& e : catalogue[card]) {
        const unsigned m = label(e, in);
        check(std::popcount(m) == static_cast<int>(card) && found.emplace(m, &e).second,
              "unique generated catalogue label of requested cardinality");
        if (m == 0) continue;
        ++records;
        check(exact.direct(m) && oracle::compare(e.level, exact.ball(m)) == 0 &&
              support(e, in) == exact.ball(m).support && e.active_mask == (1u << e.q) - 1u,
              "independent exact Gabriel level, essential support and active removals");
      }
      for (unsigned m = 1; m < (1u << in.size()); ++m)
        if (std::popcount(m) == static_cast<int>(card))
          check(found.count(m) == static_cast<size_t>(exact.direct(m)), "independent catalogue completeness");
    }
    valid = failures == before;
    if (valid) ++admitted_clouds;
  }
};

using Signature = std::vector<std::pair<std::vector<unsigned>, unsigned>>;
inline Signature expected(const Cloud& c, unsigned k, const oracle::OracleBall& cut, bool closed) {
  Signature out;
  for (const auto& group : c.exact.full_components(k, cut, closed)) {
    std::vector<unsigned> minima;
    unsigned points = 0;
    for (unsigned f : group) { points |= f; if (c.exact.direct(f)) minima.push_back(f); }
    check(!minima.empty(), "every complete Gamma component contains a minimum");
    std::sort(minima.begin(), minima.end()); out.emplace_back(std::move(minima), points);
  }
  std::sort(out.begin(), out.end()); return out;
}
inline Signature actual(const Cloud& c, const FullCertificate& f, const oracle::OracleBall& cut, bool closed) {
  std::vector<bool> active(f.nodes().size(), false);
  for (size_t i = 0; i < f.nodes().size(); ++i) {
    const auto& n = f.nodes()[i];
    const int cmp = oracle::compare(n.level, cut);
    if (cmp > 0 || (!closed && cmp == 0)) continue;
    active[i] = true;
    for (u64 j = 0; j < n.parent_count; ++j) {
      if (n.first + j >= f.parents().size()) { check(false, "parent CSR bounds"); break; }
      const auto p = f.parents()[static_cast<size_t>(n.first + j)];
      if (p >= i) { check(false, "strictly prior parent"); continue; }
      check(active[p], "live pre-lot parent"); active[p] = false;
    }
  }
  Signature out;
  for (size_t root = 0; root < active.size(); ++root) if (active[root]) {
    std::vector<unsigned> minima;
    std::vector<FullNodeId> pending{root};
    size_t visited = 0;
    unsigned points = 0;
    while (!pending.empty() && visited++ <= f.nodes().size()) {
      const auto id = pending.back(); pending.pop_back();
      if (id >= f.nodes().size()) { check(false, "descendant bounds"); continue; }
      const auto& n = f.nodes()[id];
      if (n.parent_count == 0) {
        if (n.first >= f.minima().size()) { check(false, "minimum bounds"); continue; }
        const unsigned m = label(f.minima()[n.first], c.in); minima.push_back(m); points |= m;
      } else for (u64 j = 0; j < n.parent_count; ++j) {
        if (n.first + j >= f.parents().size()) { check(false, "descendant CSR bounds"); break; }
        pending.push_back(f.parents()[n.first + j]);
      }
    }
    check(pending.empty() && visited <= f.nodes().size(), "bounded acyclic forest traversal");
    std::sort(minima.begin(), minima.end());
    check(std::adjacent_find(minima.begin(), minima.end()) == minima.end(), "no duplicate minimum identity");
    const auto read = full_certificate_coverage(f, root, f.nodes().size(), f.minima().size() * f.order());
    check(read.status == FullCertificateStatus::kOk, "public point coverage reader");
    unsigned read_mask = 0;
    for (PointId p : read.values) read_mask |= bit(p, c.in);
    check(read_mask == points, "public coverage matches labeled leaves");
    out.emplace_back(std::move(minima), points);
  }
  std::sort(out.begin(), out.end()); return out;
}
inline void compare_oracle(const Cloud& c, unsigned k, const FullCertificate& f) {
  check(f.order() == k && !empty(f), "nonempty certificate for requested order");
  if (f.order() != k || empty(f)) return;
  std::vector<unsigned> levels{1};
  for (unsigned m = 1; m < (1u << c.in.size()); ++m)
    if (std::popcount(m) == static_cast<int>(k) || std::popcount(m) == static_cast<int>(k+1)) levels.push_back(m);
  std::sort(levels.begin(), levels.end(), [&](unsigned a, unsigned b) { return oracle::compare(c.exact.ball(a), c.exact.ball(b)) < 0; });
  levels.erase(std::unique(levels.begin(), levels.end(), [&](unsigned a, unsigned b) {
    return oracle::compare(c.exact.ball(a), c.exact.ball(b)) == 0;
  }), levels.end());
  for (unsigned m : levels) for (bool closed : {false, true}) {
    ++cuts;
    check(actual(c, f, c.exact.ball(m), closed) == expected(c, k, c.exact.ball(m), closed),
          "independent Gamma cut: minimum partition AND all-facet point coverage");
  }
}
}  // namespace mhgp7_singleton_test
