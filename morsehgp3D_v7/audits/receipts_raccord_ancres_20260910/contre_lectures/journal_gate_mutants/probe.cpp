// Sonde auditeur : entrees acceptees/refusees par le header NON MODIFIE (HEAD 1fbe49d3).
#include <cstdio>
#include <string_view>
#include "../base/morsehgp3D_v7/src/forest/full_coverage_certificate.hpp"
using namespace mhgp7;
static ExactLevel level(u64 n, i128 d = 1) { return {{n, 0, 0}, d}; }
static const char* st(FullCertificateStatus s) { return s == FullCertificateStatus::kOk ? "kOk" : s == FullCertificateStatus::kInvalidInput ? "kInvalidInput" : "kResourceExhausted"; }
static void report(const char* what, const FullCoverageBuildResult& r) {
  std::printf("%-55s -> %s reason=%s nodes=%zu contribs=%zu\n", what, st(r.status), r.reason, r.value.nodes().size(), r.value.contributions().size());
}
int main() {
  std::vector<PointId> dom{0,1,2,3};
  auto two = build_full_coverage_populations(dom, std::vector<FullCoveragePopulation>{{{}, {0,1}}, {{1},{0,2}}, {{},{0,1,2,3}}}).value;
  // P1 birth of cardinal 2 at K3
  report("P1 K3 birth on 2-point population", build_full_coverage_certificate(3, two, std::vector<FullCoverageBatch>{{level(1), {{{}, {{0,3,false}}}}}}));
  // P2 birth with include_interior=false on a row with interior
  report("P2 K2 birth interior row, include_interior=false", build_full_coverage_certificate(2, two, std::vector<FullCoverageBatch>{{level(1), {{{}, {{1,3,false}}}}}}));
  // P3 birth with two contributions
  report("P3 K2 birth with two contributions", build_full_coverage_certificate(2, two, std::vector<FullCoverageBatch>{{level(1), {{{}, {{1,3,true},{2,15,false}}}}}}));
  // P4 order 11
  report("P4 order 11", build_full_coverage_certificate(11, two, std::vector<FullCoverageBatch>{{level(1), {{{}, {{2,15,false}}}}}}));
  // P5 domain smaller than order
  auto small = build_full_coverage_populations(std::vector<PointId>{0,1}, std::vector<FullCoveragePopulation>{{{}, {0,1}}}).value;
  report("P5 order 3, domain of 2", build_full_coverage_certificate(3, small, std::vector<FullCoverageBatch>{{level(1), {{{}, {{0,3,false}}}}}}));
  // P6 K1 missing a singleton
  auto k1 = build_full_coverage_populations(std::vector<PointId>{0,1}, std::vector<FullCoveragePopulation>{{{}, {0}}, {{},{1}}}).value;
  report("P6 K1 only one singleton for domain of 2", build_full_coverage_certificate(1, k1, std::vector<FullCoverageBatch>{{level(0), {{{}, {{0,1,false}}}}}}));
  // P7 unsorted parents without any downstream conflict
  auto s = build_full_coverage_populations(dom, std::vector<FullCoveragePopulation>{{{}, {0,1}}, {{},{2,3}}, {{},{0,1,2,3}}}).value;
  report("P7 unsorted parents {1,0}, no later batch", build_full_coverage_certificate(2, s, std::vector<FullCoverageBatch>{{level(1), {{{}, {{0,3,false}}}, {{}, {{1,3,false}}}}}, {level(2), {{{1,0}, {}}}}}));
  report("P7b sorted parents {0,1}, same journal", build_full_coverage_certificate(2, s, std::vector<FullCoverageBatch>{{level(1), {{{}, {{0,3,false}}}, {{}, {{1,3,false}}}}}, {level(2), {{{0,1}, {}}}}}));
  // P8 reader: contribution with include_interior=false on a row with interior
  auto g = build_full_coverage_populations(dom, std::vector<FullCoveragePopulation>{{{1},{0,2}}, {{1},{0,2,3}}}).value;
  auto r = build_full_coverage_certificate(3, g, std::vector<FullCoverageBatch>{{level(16), {{{}, {{0,3,true}}}}}, {level(25), {{{0}, {{1,4,false}}}}}});
  report("P8 build growth with interior row contribution", r);
  auto cov = full_coverage_at(r.value, 0, level(25), true);
  std::printf("P8 coverage closed 25: status=%s values=", st(cov.status)); for (auto p : cov.values) std::printf("%u ", (unsigned)p); std::printf("\n");
  // P9 absent root
  auto abs = full_coverage_at(r.value, kFullCoverageAbsent, level(25), true);
  std::printf("P9 read absent root: status=%s reason=%s values=%zu\n", st(abs.status), abs.reason, abs.values.size());
  return 0;
}
