#include "/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/tower/forest/full_coverage_certificate.hpp"
#include "/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/tower/forest/ball_data.hpp"
using namespace mhgp9::tower;
template<size_t N> struct Show;
static_assert(sizeof(ExactLevel) == 48);
static_assert(alignof(ExactLevel) == 16);
static_assert(sizeof(FullNode) == 64);
static_assert(sizeof(FullDatedContribution) == 80);
static_assert(sizeof(FullCoverageRef) == 16);
static_assert(sizeof(BallKey) == 80);
static_assert(sizeof(BallData) == 224);
static_assert(offsetof(BallData, level) == 80);
static_assert(offsetof(BallData, interior_ids) == 132);
static_assert(sizeof(FullCoveragePopulation) == 48);
int main(){}
