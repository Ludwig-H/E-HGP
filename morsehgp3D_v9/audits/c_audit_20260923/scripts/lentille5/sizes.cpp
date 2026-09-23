#include <cstddef>
#include <cstdio>
#include "forest/full_ball_tower.hpp"
using namespace mhgp9::tower;
int main() {
  std::printf("BallData %zu level_off %zu arity_off %zu interior_off %zu shell_off %zu\n", sizeof(BallData),
              offsetof(BallData, level), offsetof(BallData, arity), offsetof(BallData, interior_ids), offsetof(BallData, shell_ids));
  std::printf("ExactLevel %zu FullNode %zu FullDatedContribution %zu FullCoverageRef %zu\n", sizeof(ExactLevel), sizeof(FullNode),
              sizeof(FullDatedContribution), sizeof(FullCoverageRef));
  std::printf("FullCoverageAction %zu FullCoverageBatch %zu FullCoveragePopulation %zu Request %zu Seed %zu\n",
              sizeof(FullCoverageAction), sizeof(FullCoverageBatch), sizeof(FullCoveragePopulation), sizeof(FullBallBatchRequest), sizeof(FullBallBatchSeed));
}
