#pragma once

// Isolated audit extension of the f7edd646 q2 snapshot. This is not a product API.
#include "pipeline/wspd_q2_census.hpp"

namespace mhgp8::audit {

enum class SiblingMode { Baseline, Autonomous, Remaining };
struct SiblingWork {
  u64 child_entries{};
  u64 population_eligible{};
  u64 bound_tests{};
  u64 rejected_children{};
  u64 rejected_pairs{};
};

// Only the current SharedBlocks prefix invariant authorizes Remaining.
// Work is reset once per call. Baseline delegates to the original entry point.
[[nodiscard]] WspdQ2CensusResult run_sibling_census(
    const Q2CensusIndex& index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, Q2CensusMode census_mode,
    const Q2CensusConsumer& consumer, SiblingMode mode, SiblingWork& work);

// Bounded test helper ONLY: a separate B permutation deliberately exercises
// query_node(sibling), as opposed to aliasing sibling IDs into the Z tree.
// No freely adoptable product continuation is introduced by this fixture API.
[[nodiscard]] Q2CensusResult sibling_group_fixture(
    const Q2CensusIndex& index, std::size_t a_id,
    std::span<const std::size_t> b_ids, unsigned kmax,
    const Q2CensusConsumer& consumer, SiblingMode mode, SiblingWork& work);

}  // namespace mhgp8::audit
