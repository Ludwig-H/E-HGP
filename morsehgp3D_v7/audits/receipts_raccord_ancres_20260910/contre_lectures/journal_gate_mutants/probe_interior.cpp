#include <cstdio>
#ifndef HDR
#define HDR "../base/morsehgp3D_v7/src/forest/full_coverage_certificate.hpp"
#endif
#include HDR
using namespace mhgp7;
static ExactLevel level(u64 n) { return {{n, 0, 0}, 1}; }
int main() {
  std::vector<PointId> dom{0,1,2,3,5};
  auto g = build_full_coverage_populations(dom, std::vector<FullCoveragePopulation>{{{1},{0,2}}, {{5},{3}}}).value;
  auto r = build_full_coverage_certificate(3, g, std::vector<FullCoverageBatch>{{level(16), {{{}, {{0,3,true}}}}}, {level(25), {{{0}, {{1,1,false}}}}}});
  auto cov = full_coverage_at(r.value, 0, level(25), true);
  std::printf("contribution {pop1, mask=1, include_interior=false} on row I={5},U={3}: coverage closed 25 = "); for (auto p : cov.values) std::printf("%u ", (unsigned)p); std::printf("\n");
}
