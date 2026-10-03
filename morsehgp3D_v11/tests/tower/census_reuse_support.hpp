// Comparaisons geometriques exactes et transformation explicite des six compteurs de parcours.
#pragma once
#include "forest_vertical_parallel_support.hpp"

namespace census_reuse_test {
using namespace forest_vertical_test;
inline DescentLedger twice_census(DescentLedger value) {
  auto& c = value.census;
  c.nodes *= 2; c.bounds *= 2; c.point_tests *= 2;
  c.inside_blocks *= 2; c.outside_blocks *= 2; c.passes *= 2;
  return value;
}
inline ForestLedger twice_census(ForestLedger value) { value.descent = twice_census(value.descent); return value; }
inline bool same_population_work(const DescentLedger& a, const DescentLedger& b) {
  return a == twice_census(b) && a.census.passes == 2*a.census_calls && b.census.passes == b.census_calls;
}
inline FullTimings scratch_sentinel() {
  auto times = marked(); times.census_workspaces = 137; times.census_workspace_reserved_bytes = 139;
  return times;
}
}  // namespace census_reuse_test
