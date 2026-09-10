// ABI sizes only. No tower construction, geometry, timing or allocation test.
#include <cstdio>
#include "../../src/forest/full_coverage_certificate.hpp"

int main() {
  using namespace mhgp7;
  std::printf("{\"ExactLevel\":%zu,\"FullNode\":%zu,"
      "\"FullDatedContribution\":%zu,\"FullCoveragePopulation\":%zu,"
      "\"FullCoverageBatch\":%zu,\"FullCoverageAction\":%zu,"
      "\"FullCoverageRef\":%zu,\"u64\":%zu,\"u32\":%zu}\n",
      sizeof(ExactLevel), sizeof(FullNode), sizeof(FullDatedContribution),
      sizeof(FullCoveragePopulation), sizeof(FullCoverageBatch),
      sizeof(FullCoverageAction), sizeof(FullCoverageRef), sizeof(u64), sizeof(u32));
  return 0;
}
