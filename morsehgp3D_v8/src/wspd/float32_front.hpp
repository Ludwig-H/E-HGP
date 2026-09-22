#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include "core/float32_edge_geometry.hpp"

namespace mhgp8 {
enum class Float32FrontWitnessMode { Disabled, MidpointSamples };
struct Float32FrontOptions {
  Float32FrontWitnessMode witnesses{Float32FrontWitnessMode::Disabled};
  bool operator==(const Float32FrontOptions&) const = default;
};
struct Float32FrontRectangle {
  std::size_t node_a{}, node_b{};
  std::uint64_t pairs{};
};
using Float32FrontConsumer = std::function<void(const Float32FrontRectangle&)>;

// SUM except max_stack_size, max_product_depth, peak_stack_bytes (MAX).
// total_unordered_pairs is accumulated once/call, even when K1 is inactive.
// Every active pair belongs to exactly one emitted or rejected product.
// Proposals are distinct IDs per product; no count is inherited or passed to
// a census. Population counters are not geometry operations.
struct Float32FrontWork {
  std::uint64_t queries{}, total_unordered_pairs{}, product_visits{};
  std::uint64_t diagonal_splits{}, diagonal_leaves{}, disjoint_splits{};
  std::uint64_t witness_searches{}, witness_descent_steps{}, witness_box_tests{};
  std::uint64_t proposed_sites{}, proposals_in_factors{}, witness_credits{};
  std::uint64_t rejected_products{}, rejected_pairs{}, emitted_rectangles{}, residual_pairs{};
  std::uint64_t emitted_factor_sites{}, leaf_pair_rectangles{};
  std::uint64_t max_stack_size{}, max_product_depth{}, peak_stack_bytes{};
  Float32EdgeWork geometry{};
  bool operator==(const Float32FrontWork&) const = default;
};

// Q3-only front. K>=1, s>=1 (uint32 representation); K1 emits nothing.
// DFS diagonals LL/LR/RR and single-factor splits partition unordered pairs.
// No global list of pairs/rectangles or factor scans. Index and callback
// copied before traversal so caller reset/replacement is safe. Consumer
// exception propagates; prior emissions and work are not rolled back.
void run_float32_front(Float32IndexPtr index, std::size_t kmax, std::uint32_t s,
                       Float32FrontOptions options, const Float32FrontConsumer& consumer,
                       Float32FrontWork& work);
}  // namespace mhgp8
