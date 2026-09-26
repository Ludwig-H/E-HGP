// Audit only: one q4 family, exact event sort and strict interior windows.
// Not the bucketed product, not a global generator, not a CUDA kernel.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include "../../src/gpu/q4_lanes.hpp"

namespace audit {
using namespace mhgp9::gpu;
using Point = std::array<std::int32_t, 3>;
using Clock = std::chrono::steady_clock;
void need(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Family {
  std::vector<Point> points;
  Q4Family form{};
  Q4Frame frame{};
  std::vector<u32> entries, exits, constants, shell;
  std::vector<i64> sides;
  std::vector<Q4Pivot> pivots;
  explicit Family(std::vector<Point> input) : points(std::move(input)) {
    need(points.size() >= 3, "three_support_points");
    need(q4_family(points[0].data(), points[1].data(), points[2].data(), form), "valid_family");
    q4_frame(points[0].data(), points[1].data(), points[2].data(), frame);
    sides.resize(points.size()); pivots.resize(points.size());
    for (u32 id = 0; id < points.size(); ++id) {
      const i64 side = side_of(form, points[0].data(), points[id].data());
      sides[id] = side;
      if (side == 0) {
        const auto power = q3_power(form.form, points[0].data(), points[id].data());
        if (power < 0) constants.push_back(id);
        if (power == 0) shell.push_back(id);
      } else {
        (side > 0 ? entries : exits).push_back(id);
        pivots[id] = q4_pivot(form, frame, points[0].data(), points[id].data());
      }
    }
  }
  // sign(root[right] - root[left]); reduced degree-five predicate, no
  // cross product of the degree-six P with the degree-three S in i128.
  int compare(u32 left, u32 right) const {
    need(sides[left] != 0 && sides[right] != 0, "nonconstant_comparison");
    return q4_compare(pivots[left], points[0].data(), points[right].data(), sides[right]);
  }
  int power(u32 root, u32 id) const {
    const auto& p = pivots[root];
    i64 v[3];
    for (int k = 0; k < 3; ++k) v[k] = static_cast<i64>(points[id][k]) - points[0][k];
    const i64 vv = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    const i128 q = p.det * vv - (p.num[0]*v[0] + p.num[1]*v[1] + p.num[2]*v[2]);
    return sign128(q) * sign128(p.det);
  }
};
struct Query { u32 root, entry_end, exit_begin, depth; };
struct Sorted {
  std::vector<u32> entries, exits, events;
  std::vector<Query> groups;
  std::uint64_t comparisons = 0, scanned_events = 0;
  Sorted(const Family& f, bool nonstrict = false, bool reversed_exit = false)
      : entries(f.entries), exits(f.exits), events(f.entries) {
    events.insert(events.end(), exits.begin(), exits.end());
    const auto less = [&](u32 a, u32 b) {
      ++comparisons;
      const int order = f.compare(a, b);
      return order > 0 || (order == 0 && a < b);
    };
    std::sort(events.begin(), events.end(), less);
    entries.clear(); exits.clear();
    for (u32 id : events) (f.sides[id] > 0 ? entries : exits).push_back(id);
    u32 en = 0, ex = 0;
    for (std::size_t first = 0; first < events.size();) {
      std::size_t last = first + 1;
      while (last < events.size()) {
        ++comparisons;
        if (f.compare(events[first], events[last]) != 0) break;
        ++last;
      }
      u32 add_en = 0, add_ex = 0;
      for (auto j = first; j < last; ++j) {
        ++scanned_events;
        if (f.sides[events[j]] > 0) ++add_en; else ++add_ex;
      }
      // Strict ball at the group: same-root exits have left; same-root
      // entries have not entered. One scan, no reservoir recovery.
      ex += add_ex;
      const u32 before = en + (nonstrict ? add_en : 0);
      const u32 after = reversed_exit ? ex : static_cast<u32>(exits.size()) - ex;
      groups.push_back({events[first], before, ex,
                        static_cast<u32>(f.constants.size()) + before + after});
      en += add_en;
      first = last;
    }
  }
  std::vector<u32> payload(const Family& f, const Query& q) const {
    std::vector<u32> ids = f.constants;
    ids.insert(ids.end(), entries.begin(), entries.begin() + q.entry_end);
    ids.insert(ids.end(), exits.begin() + q.exit_begin, exits.end());
    std::sort(ids.begin(), ids.end());
    return ids;
  }
};
std::vector<Point> scene(std::size_t n, unsigned seed, bool reverse_ids = false) {
  std::vector<Point> p{{65536,65536,65536}, {131072,65536,65536}, {98304,115536,65536}};
  // Constant interior, repeated root by reflection
  // about the family's x-symmetry plane. The fixed support stays first.
  const std::array<Point, 7> fixed{{{98304,80000,65536},
      {80000,90000,70000}, {116608,90000,70000},
      {80000,90000,60000}, {116608,90000,60000},
      {98304,80000,50000}, {98304,80000,90000}}};
  std::set<Point> used(p.begin(), p.end());
  for (const auto& x : fixed) if (p.size() < n && used.insert(x).second) p.push_back(x);
  std::mt19937 random(seed);
  while (p.size() < n) {
    Point x{static_cast<std::int32_t>(50000 + random() % 100000),
            static_cast<std::int32_t>(45000 + random() % 100000),
            static_cast<std::int32_t>(20000 + random() % 100000)};
    if (used.insert(x).second) p.push_back(x);
  }
  if (reverse_ids) std::reverse(p.begin() + 3, p.end());
  return p;
}
struct Checks { std::uint64_t groups = 0, direct_tests = 0, shallow = 0, payload_ids = 0, root_ties = 0; };
Checks check(const Family& f, const Sorted& sorted) {
  Checks c;
  for (const auto& q : sorted.groups) {
    ++c.groups;
    std::vector<u32> inside;
    for (u32 id = 0; id < f.points.size(); ++id) {
      ++c.direct_tests;
      if (f.power(q.root, id) < 0) inside.push_back(id);
    }
    need(inside.size() == q.depth, "depth_differs");
    for (unsigned k = 3; k <= 10; ++k) {
      if (q.depth >= k - 2) continue;
      ++c.shallow;
      const auto ids = sorted.payload(f, q);
      need(ids == inside, "interior_window_differs");
      need(ids.size() <= k - 3, "payload_bound");
      c.payload_ids += ids.size();
    }
  }
  c.root_ties = sorted.events.size() - sorted.groups.size();
  return c;
}
double ms(Clock::time_point t) { return std::chrono::duration<double, std::milli>(Clock::now()-t).count(); }
void selftest() {
  Checks total;
  unsigned fixtures = 0, killed = 0;
  for (const unsigned n : {10U, 31U, 32U, 33U, 65U, 127U, 257U}) {
    for (unsigned seed = 1; seed <= 3; ++seed) for (bool reverse : {false, true}) {
      const Family f(scene(n, seed, reverse));
      const Sorted s(f);
      const auto c = check(f, s);
      total.groups += c.groups; total.direct_tests += c.direct_tests;
      total.shallow += c.shallow; total.payload_ids += c.payload_ids; total.root_ties += c.root_ties;
      ++fixtures;
    }
  }
  // Non-vacuous K3: no constant interior and a single off-plane root.
  for (unsigned rotate = 0; rotate < 3; ++rotate) {
    std::vector<Point> points{{60000,60000,60000}, {100000,60000,60000},
                              {80000,95000,60000}, {80000,75000,90000}};
    for (auto& p : points) {
      std::rotate(p.begin(), p.begin() + rotate, p.end());
      for (auto& x : p) x += 120000;
    }
    const Family sparse(std::move(points));
    const auto c = check(sparse, Sorted(sparse));
    need(c.shallow == 8, "all_K3_to_K10_exercised");
    total.groups += c.groups; total.direct_tests += c.direct_tests;
    total.shallow += c.shallow; total.payload_ids += c.payload_ids; total.root_ties += c.root_ties;
    ++fixtures;
  }
  {
    // Circle centre (0,7/2,0), radius 25/2: four distinct constant
    // contacts, not a duplicated support point removed by deduplication.
    std::vector<Point> points{{-12,0,0}, {12,0,0}, {0,16,0}, {10,11,0}, {0,4,10}, {0,4,-10}};
    for (auto& p : points) for (auto& x : p) x += 200000;
    const Family contact(std::move(points));
    need(contact.shell.size() == 4, "extra_constant_shell");
    const auto c = check(contact, Sorted(contact));
    total.groups += c.groups; total.direct_tests += c.direct_tests;
    total.shallow += c.shallow; total.payload_ids += c.payload_ids; total.root_ties += c.root_ties;
    ++fixtures;
  }
  const Family f(scene(33, 7));
  for (unsigned mutant = 0; mutant < 2; ++mutant) {
    try { (void)check(f, Sorted(f, mutant == 0, mutant == 1)); }
    catch (const std::runtime_error& e) { need(std::string(e.what()) == "depth_differs", "wrong_mutant_cause"); ++killed; }
  }
  need(killed == 2 && total.shallow != 0 && total.root_ties != 0, "coverage_missing");
  std::cout << "{\"status\":\"pass\",\"fixtures\":" << fixtures << ",\"groups\":" << total.groups
    << ",\"direct_power_tests\":" << total.direct_tests << ",\"shallow_queries\":" << total.shallow
    << ",\"payload_ids\":" << total.payload_ids << ",\"root_ties\":" << total.root_ties
    << ",\"causal_mutants\":" << killed << "}\n";
}
void bench(std::size_t n) {
  const Family f(scene(n, 911));
  for (unsigned repeat = 0; repeat < 3; ++repeat) {
    auto t = Clock::now();
    const Sorted s(f);
    const double sorted_ms = ms(t);
    std::uint64_t queries = 0, ids = 0, point_tests = 0, rejected = 0;
    t = Clock::now();
    for (const auto& q : s.groups) if (q.depth < 3) { ++queries; ids += s.payload(f, q).size(); }
    const double payload_ms = ms(t);
    // Diagnostic baseline: complete family, all root groups, stop as soon
    // as K5 depth reaches 3. No bucket lens or positivity/owner prefilter.
    // This is deliberately not a time comparison with product T1.
    t = Clock::now();
    for (const auto& q : s.groups) {
      unsigned depth = 0;
      for (u32 id = 0; id < f.points.size(); ++id) {
        ++point_tests;
        if (f.power(q.root, id) < 0 && ++depth == 3) break;
      }
      need((depth < 3) == (q.depth < 3), "benchmark_shallow_disagrees");
      rejected += depth == 3 ? 1 : 0;
    }
    const double stopped_ms = ms(t);
    std::cout << "{\"n\":" << n << ",\"repeat\":" << repeat << ",\"events\":" << s.events.size()
      << ",\"groups\":" << s.groups.size() << ",\"sort_comparisons\":" << s.comparisons
      << ",\"scan_events\":" << s.scanned_events << ",\"shallow_queries\":" << queries
      << ",\"payload_ids\":" << ids << ",\"early_point_tests\":" << point_tests
      << ",\"early_rejected\":" << rejected << ",\"sorted_ms\":" << sorted_ms
      << ",\"payload_ms\":" << payload_ms << ",\"early_stop_ms\":" << stopped_ms << "}\n";
  }
}
}  // namespace audit
int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--selftest") { audit::selftest(); return 0; }
    if (argc == 3 && std::string(argv[1]) == "--bench") {
      const std::string arg(argv[2]); std::size_t used = 0;
      const auto n = std::stoull(arg, &used);
      audit::need(used == arg.size() && (n == 8000 || n == 16000 || n == 32000), "fixture_size");
      audit::bench(n); return 0;
    }
    std::cerr << "usage: --selftest | --bench 8000|16000|32000\n"; return 2;
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
