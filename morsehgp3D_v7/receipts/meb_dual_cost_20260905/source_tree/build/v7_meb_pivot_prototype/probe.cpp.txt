#include "pivot.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace mhgp7;
namespace {
void require(bool value, const char* reason) {
  if (!value) throw std::runtime_error(reason);
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
  require(f.ix.valid && !f.ix.has_duplicate_positions(), "fixture");
  for (size_t p = 0; p < f.n; ++p) {
    bool found = false;
    for (i32 u = 0; u < f.ix.unique_count(); ++u)
      if (f.ix.point_id(u) == p) { f.sites[p] = u; found = true; break; }
    require(found, "identity");
  }
  return f;
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
u64 comparisons = 0, complete = 0, degeneracy = 0, capped = 0, reference_supports = 0;
pivot_prototype::Work total;
u64 one(const Fixture& f, u64 cap, u64 initial = 0, size_t pivot_cap = 16) {
  SilentIncidenceLimits limits;
  limits.max_meb_supports = cap;
  SilentIncidenceResult a, b;
  a.stats.core_records = b.stats.core_records = 19;
  a.stats.meb_calls = b.stats.meb_calls = 3;
  a.stats.meb_supports = b.stats.meb_supports = initial;
  silent_detail::LocalBall ba, bb;
  ba.key = bb.key = BallKey{17, {19, 23, 29}, 31};
  ba.level = bb.level = ExactLevel{{37, 41, 43}, 47};
  ba.q = bb.q = 9;
  ba.support = bb.support = {53, 59, 61, 67};
  const std::vector<ForestEvent> direct;
  silent_detail::Builder reference(f.ix, direct, limits, &a);
  const bool oka = reference.miniball(f.sites, f.n, &ba);
  pivot_prototype::Work work;
  const bool okb = pivot_prototype::miniball(f.ix, direct, limits, &b, f.sites, f.n, &bb, &work, pivot_cap);
  require(oka == okb && a.status == b.status && std::strcmp(a.reason, b.reason) == 0, "status_reason");
  require(same_stats(a.stats, b.stats), "stats");
  require(ba.key == bb.key && ba.level == bb.level && ba.q == bb.q && ba.support == bb.support, "literal_ball");
  require(a.events.empty() && b.events.empty(), "local_events");
  require(work.pivots <= pivot_cap && work.candidates <= 1 + 25 * pivot_cap, "bounded_proposal");
  if (initial >= cap) require(work.candidates == 0 && work.pivots == 0, "empty_budget_no_proposal");
  ++comparisons;
  complete += oka;
  degeneracy += a.status == SilentIncidenceStatus::kUnsupportedDegeneracy;
  capped += a.status == SilentIncidenceStatus::kResourceExhausted;
  total.candidates += work.candidates;
  total.pivots += work.pivots;
  total.certified += work.certified;
  total.fallback += work.fallback;
  reference_supports += a.stats.meb_supports - initial;
  return a.stats.meb_supports - initial;
}
void all_ordinals() {
  u64 examined = 0;
  for (size_t n = 2; n <= 11; ++n) {
    u64 rank = 0;
    const auto check = [&](std::array<size_t, 4> slots, u8 q) {
      pivot_prototype::Candidate candidate;
      candidate.slots = slots;
      candidate.q = q;
      require(pivot_prototype::ordinal(n, candidate) == ++rank, "ordinal");
      ++examined;
    };
    for (size_t a = 0; a < n; ++a) for (size_t b = a + 1; b < n; ++b) check({a,b,0,0},2);
    for (size_t a = 0; a < n; ++a) for (size_t b = a + 1; b < n; ++b)
      for (size_t c = b + 1; c < n; ++c) check({a,b,c,0},3);
    for (size_t a = 0; a < n; ++a) for (size_t b = a + 1; b < n; ++b)
      for (size_t c = b + 1; c < n; ++c) for (size_t d = c + 1; d < n; ++d) check({a,b,c,d},4);
  }
  require(examined == 1507, "ordinal_floor");
}
}
int main(int argc, char**) {
  if (argc != 1) return 2;
  try {
    all_ordinals();
    std::vector<Fixture> fixtures;
    for (const auto& points : std::vector<std::vector<P3>>{
        {{0,0,0},{65535,65535,65535}},
        {{0,0,0},{65535,65535,0},{65535,0,65535}},
        {{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535}},
        {{0,0,0},{8,0,0},{8,8,0},{0,8,0}},
        {{0,0,7},{0,9,6},{1,4,0},{0,0,1},{4,1,2}},
        {{0,0,0},{4,0,0},{2,0,0}},
        {{0,0,0},{4,0,0},{2,3,0},{2,0,2}},
        {{0,0,0},{46368,28657,0},{28657,17711,0}}}) fixtures.push_back(fixture(points));
    u64 state = 0x6d65622d76372d31ULL;
    const auto next = [&]() { state = state * 6364136223846793005ULL + 1442695040888963407ULL; return state; };
    for (size_t k = 0; k < 2000; ++k) {
      std::vector<P3> points;
      for (size_t j = 0; j < 2 + k % 10; ++j)
        points.push_back({(i64)((next() >> 32) & 65535), (i64)((next() >> 32) & 65535), (i64)((next() >> 32) & 65535)});
      fixtures.push_back(fixture(points));
    }
    for (size_t i = 0; i < fixtures.size(); ++i) {
      const auto& f = fixtures[i];
      const u64 used = one(f, 1000000);
      one(f, 1000000, 0, 0); // Deliberately force the proposal fallback.
      one(f, used + 7, 7);
      one(f, UINT64_MAX, UINT64_MAX - 100);
      one(f, UINT64_MAX, UINT64_MAX);
      if (i < 168)
        for (u64 cap = 0; cap <= used + 1; ++cap) one(f, cap);
      Fixture reverse = f;
      std::reverse(reverse.sites.begin(), reverse.sites.begin() + f.n);
      one(reverse, 1000000);
    }
    require(comparisons > 20000 && complete > 1000 && degeneracy > 0 && capped > 0 &&
            total.certified > 1000 && total.fallback > 1000, "nonvacuity");
    std::printf("pivot_prototype=passed comparisons=%llu complete=%llu degenerate=%llu capped=%llu logical_reference_supports=%llu proposal_candidates=%llu pivots=%llu certified=%llu fallback=%llu public_status=not_claimed\n",
        (unsigned long long)comparisons, (unsigned long long)complete, (unsigned long long)degeneracy,
        (unsigned long long)capped, (unsigned long long)reference_supports, (unsigned long long)total.candidates,
        (unsigned long long)total.pivots, (unsigned long long)total.certified, (unsigned long long)total.fallback);
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "pivot_prototype=failed reason=%s comparisons=%llu\n", error.what(), (unsigned long long)comparisons);
    return 1;
  }
}
