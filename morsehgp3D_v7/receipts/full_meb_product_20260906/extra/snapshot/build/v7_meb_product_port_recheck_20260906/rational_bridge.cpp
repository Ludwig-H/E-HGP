// Fresh product-port bridge, explicit derivative of c46ae2448cd11b438846761a7d26aac5c2af132d49623633f76ecefecb884b31.
// Local fresh states only; FULL persistence is judged in the composition gate.
#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../../morsehgp3D_v7/src/forest/meb_proposal.hpp"

using namespace mhgp7;
namespace dual = mhgp7::meb_proposal_detail;

std::string decimal(i128 value) {
  const bool negative = value < 0;
  u128 n = negative ? static_cast<u128>(-(value + 1)) + 1 : static_cast<u128>(value);
  std::string out;
  do { out.push_back(static_cast<char>('0' + n % 10)); n /= 10; } while (n);
  if (negative) out.push_back('-');
  std::reverse(out.begin(), out.end());
  return out;
}

std::string encode(bool ok, const SilentIncidenceResult& r,
                   const silent_detail::LocalBall& b,
                   const std::array<i32, 11>& sites, size_t n) {
  std::ostringstream s;
  s << "{\"ok\":" << (ok ? "true" : "false")
    << ",\"status\":" << static_cast<int>(r.status)
    << ",\"reason\":\"" << r.reason << "\",\"stats\":[";
  const auto& t = r.stats;
  const std::array<u64, 13> stats = {t.core_records, t.core_facets,
      t.facets_with_two_intruders, t.chain_steps, t.added_cofaces,
      t.terminal_direct, t.terminal_cached, t.max_chain_length,
      t.query_nodes, t.query_leaves, t.query_range_skips,
      t.meb_calls, t.meb_supports};
  for (size_t i = 0; i < stats.size(); ++i) s << (i ? "," : "") << stats[i];
  s << "],\"q\":" << static_cast<unsigned>(b.q)
    << ",\"key\":[" << decimal(b.key.a);
  for (const i128 value : b.key.b) s << ',' << decimal(value);
  s << ',' << decimal(b.key.c) << "],\"num\":[" << b.level.num[0]
    << ',' << b.level.num[1] << ',' << b.level.num[2]
    << "],\"den\":" << decimal(b.level.den) << ",\"support_slots\":[";
  for (size_t i = 0; i < 4; ++i) {
    i64 slot = b.support[i];
    if (i < b.q && slot >= 0) {
      const auto found = std::find(sites.begin(), sites.begin() + n, b.support[i]);
      slot = found == sites.begin() + n ? -99 : found - sites.begin();
    }
    s << (i ? "," : "") << slot;
  }
  s << "],\"events_size\":" << r.events.size() << '}';
  return s.str();
}

struct Observer {
  u64 forms = 0, pair_searches = 0, violations = 0;
  void before_pair_selection(const dual::Work& w, const dual::Limits& l) {
    ++pair_searches;
    if (w.meb_proposal_supports >= l.max_meb_proposal_supports) ++violations;
  }
  void before_form(const dual::Work& w, const dual::Limits& l, u8) {
    ++forms;
    if (w.meb_proposal_supports != forms || forms > l.max_meb_proposal_supports)
      ++violations;
  }
};

int main(int argc, char**) {
  if (argc != 1) return 2;
  char mode = 0;
  while (std::cin >> mode) {
    if (mode == 'O') {
      size_t n = 0;
      unsigned q = 0;
      dual::Candidate c;
      if (!(std::cin >> n >> q) || n < 2 || n > 11 || q < 2 || q > 4 || q > n) return 2;
      c.q = static_cast<u8>(q);
      for (size_t i = 0; i < q; ++i)
        if (!(std::cin >> c.slots[i]) || c.slots[i] >= n ||
            (i && c.slots[i - 1] >= c.slots[i])) return 2;
      std::cout << "{\"ordinal\":" << dual::ordinal(n, c) << "}\n";
      continue;
    }
    size_t n = 0;
    u64 legacy_cap = 0, proposal_cap = 0;
    if (mode != 'M' || !(std::cin >> n >> legacy_cap >> proposal_cap) || n < 2 || n > 11) return 2;
    std::vector<P3> points(n);
    for (auto& p : points) {
      if (!(std::cin >> p.x >> p.y >> p.z) || p.x < 0 || p.y < 0 || p.z < 0 ||
          p.x > 65535 || p.y > 65535 || p.z > 65535) return 2;
    }
    const CloudIndex ix = build_cloud_index(points);
    if (!ix.valid || ix.has_duplicate_positions()) return 2;
    std::array<i32, 11> sites{};
    for (i32 u = 0; u < ix.unique_count(); ++u) sites[ix.point_id(u)] = u;
    SilentIncidenceLimits caps;
    caps.max_meb_supports = legacy_cap;
    SilentIncidenceResult expected, observed;
    expected.status = SilentIncidenceStatus::kInvariantViolated;
    expected.reason = "audit_initial_status";
    expected.stats = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 0};
    observed = expected;
    silent_detail::LocalBall eb;
    eb.key = {7, {11, 13, 17}, 19};
    eb.level = {{23, 29, 31}, 37};
    eb.q = 9;
    eb.support = {-1, -2, -3, -4};
    silent_detail::LocalBall ob = eb;
    const std::vector<ForestEvent> direct;
    silent_detail::Builder reference(ix, direct, caps, &expected);
    const bool eok = reference.miniball(sites, n, &eb);
    dual::Work work;
    dual::Limits limits;
    limits.max_meb_proposal_supports = proposal_cap;
    Observer observer;
    silent_detail::Builder fallback(ix, direct, caps, &observed);
    const bool ook = dual::miniball(ix, fallback, caps, &observed, sites, n, &ob,
                                  limits, &work, &observer);
    const std::string e = encode(eok, expected, eb, sites, n);
    const std::string o = encode(ook, observed, ob, sites, n);
    std::cout << "{\"reference\":" << e << ",\"proposed\":" << o
              << ",\"same_as_F\":" << (e == o ? "true" : "false")
              << ",\"work\":[" << work.meb_proposal_supports << ',' << work.pivots
              << ',' << work.certified << ',' << work.fallback
              << "],\"observer\":[" << observer.forms << ',' << observer.pair_searches
              << ',' << observer.violations << "]}\n";
  }
  return std::cin.eof() ? 0 : 2;
}
