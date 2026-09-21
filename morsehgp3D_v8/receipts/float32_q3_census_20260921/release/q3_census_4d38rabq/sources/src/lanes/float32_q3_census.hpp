#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

#include "core/float32_ball.hpp"
#include "core/float32_q3_block.hpp"
#include "spatial/float32_index.hpp"

namespace mhgp8 {

enum class Float32Q3CensusMode { Individual, SharedPrefix };

struct Float32Q3CensusOptions {
  std::size_t kmax{10};
  Float32Q3CensusMode mode{Float32Q3CensusMode::Individual};
  // Sharing grain only. Every residual seed is examined, never truncated.
  std::size_t relay_sites{8};
};

struct Float32Q3CensusWork {
  Float32Q3BlockWork shared_bounds{}, individual_bounds{};
  Float32BallWork supports{}, power{};
  // SUM, except the final three explicitly marked MAX fields.
  std::uint64_t calls{}, input_seed_slots{}, shared_frames{}, shared_splits{};
  std::uint64_t shared_witness_visits{}, shared_endpoint_skips{}, shared_inside_nodes{};
  std::uint64_t shared_inside_sites{}, shared_outside_nodes{}, shared_witness_splits{};
  std::uint64_t shared_saturated_blocks{}, shared_rejected_seed_slots{}, shared_children_with_credit{};
  std::uint64_t relay_blocks{}, relayed_seed_slots{}, endpoint_seeds{}, support_candidates{};
  std::uint64_t invalid_supports{}, valid_supports{}, relays_with_credit{}, relays_at_eof{};
  std::uint64_t count_node_visits{}, count_point_tests{}, count_inside_nodes{}, count_inside_sites{};
  // count_outside_nodes also includes exactly-zero singleton contacts.
  std::uint64_t count_outside_nodes{}, count_splits{}, saturated_supports{}, accepted_supports{};
  std::uint64_t shell_node_visits{}, shell_point_tests{}, shell_excluded_nodes{}, shell_splits{};
  std::uint64_t shell_ids{}, callbacks{};
  std::uint64_t peak_pending_frames{}, stack_capacity_bytes{}, peak_shell_capacity_bytes{}; // MAX.
  bool operator==(const Float32Q3CensusWork&) const = default;
};

// Views live ONLY during the callback. depth is the complete strict-interior
// count, shell has ALL original IDs on the sphere, without sorting or repeats.
// ball is a certified positive q3 support; build its canonical key only if the
// consumer needs one. No interior-ID list, ball deduplication or q_min here.
struct Float32Q3Emission {
  std::size_t seed{}, depth{};
  const Float32Ball& ball;
  std::span<const std::size_t> shell;
};

// ONE supplied edge (a,b) and ONE certified seed subtree of the SAME index.
// Enumerates every positive triangle (a,b,x) in that subtree whose global
// strict depth is <kmax-1. a/b are omitted only as candidate seeds, not from
// the global shell. No longest-edge ownership, cover, WSPD or FULL contract.
// Invalid candidate seeds remain witnesses for every other candidate.
//
// SharedPrefix classifies an unchanged global preorder prefix for every
// possible positive seed in X. Each X child and each relayed seed gets its
// OWN frozen (count,cursor) ticket. No ticket is externally constructible.
// Unknown witness leaf forces an X split BEFORE consumption. Known fixed
// endpoints a/b have power zero for all seeds and may be skipped in COUNT.
// Every accepted ball gets a NEW GLOBAL shell pass, including prefix contacts.
//
// index is owned and options copied throughout this synchronous call. The
// callback and work are borrowed exclusively for this call. Per-call stack/shell
// are private; immutable index and geometry may be shared by callers. The X
// DFS reserves max_depth()+1 frames from the median index proof, not a quota;
// Z uses escape links without a stack. No census restarts after a credit.
// Invalid arguments leave work/callback untouched. Exceptions after entry
// preserve prior emissions and partial counters; no transactional rollback.
void run_float32_q3_edge_census(
    Float32IndexPtr index, std::size_t a, std::size_t b, std::size_t seed_node,
    const Float32Q3CensusOptions& options,
    const std::function<void(const Float32Q3Emission&)>& emit,
    Float32Q3CensusWork& work);

}  // namespace mhgp8
