#include "tower/forest/ball_data.hpp"
#include "tower/forest/full_coverage_certificate.hpp"
#include "tower/forest/full_ball_tower.hpp"
using namespace mhgp9::tower;
static_assert(sizeof(ExactLevel) == 48);
static_assert(sizeof(BallKey) == 80);
static_assert(sizeof(BallData) == 224);
static_assert(sizeof(FullNode) == 64);
static_assert(sizeof(FullDatedContribution) == 72);
static_assert(sizeof(FullCoverageRef) == 16);
static_assert(sizeof(FullCoveragePopulation) == 48);
static_assert(sizeof(FullBallBatchRequest) == 56);
int main() {}
