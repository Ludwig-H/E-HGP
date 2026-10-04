// Fixtures et encodages de test, sans lire les paddings des entiers ou des proprietaires.
#pragma once
#include <iomanip>
#include <sstream>
#include <vector>
#include "catalogue/catalogue.hpp"

namespace adaptive_test {
using namespace mhgp11;
using Position = std::array<u32, 3>;
inline Result<Cloud> prepare(const std::vector<Position>& points, MemoryBudget& budget) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  for (const auto& p : points) {
    x.push_back(p[0]); y.push_back(p[1]); z.push_back(p[2]);
    ids.push_back(PointId{1000 + static_cast<u32>(ids.size())});
  }
  return prepare_cloud(x, y, z, ids, CoordWidth{}, budget);
}
inline std::vector<Position> line(u32 count) {
  std::vector<Position> out;
  for (u32 i = 0; i < count; ++i) out.push_back({i, 0, 0});
  return out;
}
inline std::vector<Position> ghost() {
  return {{0,29,9},{7,17,24},{14,12,4},{15,26,0},{20,24,6},{20,31,23},{22,0,9},{24,12,19},{28,9,29}};
}
template <class T>
inline void integer(std::ostream& out, const T& value) {
  const auto wide = num::to_wide(value);
  out << (wide.sign() < 0 ? '-' : '+') << std::hex << std::setfill('0');
  for (auto word : wide.words) out << std::setw(16) << word;
  out << std::dec;
}
inline std::string canonical(const Catalogue& cat) {
  std::ostringstream out;
  out << cat.kmax() << ':' << cat.balls() << ':';
  for (const auto& level : cat.levels()) { integer(out, level.numerator()); integer(out, level.denominator()); }
  for (const auto& ball : cat.balls_data()) {
    for (auto site : ball.support) out << idx(site) << ',';
    out << idx(ball.rank) << ',' << ball.p << ',' << ball.m << ',' << unsigned(ball.qmin) << ';';
  }
  for (auto n : cat.population_offsets()) out << n << ',';
  out << ':';
  for (auto s : cat.population()) out << idx(s) << ',';
  return out.str();
}
inline std::string plan(const CatalogueDiagnostics& diagnostic) {
  std::ostringstream out;
  const auto& p = diagnostic.planning();
  out << p.adaptive << ',' << p.memory_fallback << ',' << p.plan_nodes << ',' << p.plan_leaves << ','
      << p.empty_leaves << ',' << p.rounds << ',' << p.priority_tests << ',' << p.replay_bytes << ':';
  for (const auto& t : diagnostic.tasks()) {
    out << t.path_known << ',' << t.inside_known << ',' << t.path[0] << ',' << t.path[1] << ',' << t.depth << ','
        << t.count << ',' << t.capacity << ',' << t.inside << ':';
    for (auto n : t.lo) out << n << ',';
    for (auto n : t.hi) out << n << ',';
    out << ';';
  }
  return out.str();
}
}  // namespace adaptive_test
