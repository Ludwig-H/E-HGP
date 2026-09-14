#pragma once

#include "pipeline/q2_census.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace mhgp8 {

enum class WspdFrontMode { Pure, MidpointSamples };

// Borrowed index-relative node IDs. This is a producer event, not a public
// adoptable geometric certificate. Its ranges use this index's order only.
// It may be retained as data if the caller also retains the exact index.
struct WspdRectangle {
  std::size_t a_node{};
  std::size_t b_node{};
  std::uint8_t lane_mask{};  // Bit 0=q2, bit 1=q3, bit 2=q4. No tower output.
};

struct WspdFrontWork {
  u64 product_visits{};
  u64 diagonal_splits{};
  u64 diagonal_leaves{};
  u64 disjoint_splits{};
  u64 separation_tests{};
  u64 witness_searches{};
  u64 witness_descent_steps{};
  u64 witness_box_distance_tests{};
  u64 proposed_sites{};
  u64 proposals_in_factors{};
  u64 h_bound_tests{};
  u64 xi_bound_tests{};
  u64 witness_lane_credits{};
  u64 fully_rejected_products{};
  u64 emitted_rectangles{};
  u64 emitted_factor_sites{};
  u64 max_factor_size{};
  u64 leaf_pair_rectangles{};
  u64 max_stack_size{};
  u64 max_product_depth{};
  // Classes of max(|A|,|B|): 1, 2..7, 8..63, 64..1023, >=1024.
  std::array<u64, 5> size_class_rectangles{};
  std::array<u64, 5> size_class_pair_mass{};
  std::array<u64, 3> rejected_pair_mass{};
  std::array<u64, 3> residual_pair_mass{};
  std::array<u64, 3> lane_rectangles{};
};

struct WspdFrontResult {
  u64 total_unordered_pairs{};
  std::uint8_t active_lane_mask{};
  WspdFrontWork work;
};

using WspdRectangleConsumer = std::function<void(const WspdRectangle&)>;

// Exactly the existing v8 convention: squared box gap >= s^2 times the
// largest squared box diagonal. Not the v4 center/radius convention.
// Stream a separated residual cover of every active lane, after safe
// optional early rejection. No copies/validation/scans of n sites or of
// factors per product; no stored complete frontier or rectangle catalogue.
// Kmax is in 1..10; lane q is active iff q<=Kmax+1, threshold Kmax+2-q.
// MidpointSamples descends one spatial path, with no nearest-neighbor
// backtracking, and proposes at most Kmax distinct adjacent ranks. It is
// a rejection heuristic only: missing witnesses never remove any pair.
// Neither this front nor a bounded ledger certifies a complete HGP tower.
// Index/consumer must remain alive and valid throughout this synchronous
// call. A callback exception propagates; prior emissions are not undone.
[[nodiscard]] WspdFrontResult run_wspd_front(
    const Q2CensusIndex& index, unsigned kmax, unsigned separation_s,
    WspdFrontMode mode, const WspdRectangleConsumer& consumer);

}  // namespace mhgp8
