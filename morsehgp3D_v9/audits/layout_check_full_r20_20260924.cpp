// Audit-only, syntax-only ABI layout check for the R20 FULL output.
// Run from the repository root:
// g++ -std=c++20 -fsyntax-only morsehgp3D_v9/audits/layout_check_full_r20_20260924.cpp
#include "../src/tower/forest/full_coverage_certificate.hpp"

static_assert(sizeof(mhgp9::tower::ExactLevel) == 48);
static_assert(sizeof(mhgp9::tower::FullNode) == 64);
static_assert(sizeof(mhgp9::tower::FullNodeId) == 8);
static_assert(sizeof(mhgp9::tower::FullDatedContribution) == 80);
static_assert(sizeof(mhgp9::tower::FullCoveragePopulation) == 48);
static_assert(sizeof(mhgp9::tower::PointId) == 4);
