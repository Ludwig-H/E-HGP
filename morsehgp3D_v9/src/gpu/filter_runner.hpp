#pragma once

// MorseHGP3D v9 (23 septembre 2026) — device run of the exact q3/q4 witness
// filter (gpu/witness_filter.hpp) on a WSPD rectangle population: one thread
// per rectangle, an exclusive scan of the surviving pair masses, then one
// thread per expanded pair (row-major a x b in rectangle order, no cache).
// Plain host types only: this header is shared by the C++20 host probe and
// the CUDA translation unit.

#include "witness_filter.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mhgp9::gpu {

struct FilterInput {
  const FlatNode* nodes = nullptr;
  std::size_t node_count = 0;
  const std::int32_t* rank_points = nullptr;  // 3 coordinates per spatial rank
  std::size_t rank_count = 0;
  const u32* rect_a = nullptr;  // node ids
  const u32* rect_b = nullptr;
  const u8* rect_mask = nullptr;  // front lane masks (subset of 6)
  std::size_t rect_count = 0;
  unsigned kmax = 0;
  unsigned repeats = 1;  // timed repetitions of the whole device pass
};

struct FilterOutput {
  bool available = false;  // false: built without CUDA or no device
  std::string device;
  std::string error;  // non-empty: CUDA failure, nothing below is valid
  bool stack_failure = false;
  std::vector<u8> rect_masks;
  std::vector<u8> pair_masks;  // surviving rectangles only, row-major
  std::uint64_t pairs = 0, rect_visits = 0, pair_visits = 0;
  // Best of `repeats` device passes (ms), cudaEvent timings.
  double upload_ms = 0, rect_ms = 0, scan_ms = 0, pair_ms = 0, download_ms = 0, total_ms = 0;
  double first_total_ms = 0;  // the first pass, cold
};

// Host-side refusal of a raw input before any device call (contre-audits B
// 13 h 05 and A « domaine u18 ») ; empty string when accepted. The device
// arithmetic is proved only for u18 coordinates, and a box credits or
// excludes every site of its rank range: a forged box that misses one of
// its points makes a false rejection (A's fixture: q4 mask 4 -> 0). Checked:
// K in 1..10 (the filter derives the available lanes); coordinates and box bounds in 0..262143 with low <= high;
// children partition their parent's rank range exactly; every box contains
// the points of its range (O(n * depth)); rectangle ids, lane masks subset
// of 6, and CUB's int item count. Tight hulls are not required: a larger
// box only weakens both certificates.
inline std::string validate_filter_input(const FilterInput& input) {
  constexpr std::int32_t limit = 262143;
  if (input.kmax < 1 || input.kmax > 10) return "kmax outside 1..10";
  if (input.nodes == nullptr || input.node_count == 0 || input.node_count >= absent32) return "empty or huge node array";
  if (input.rank_points == nullptr || input.rank_count == 0 || input.rank_count >= absent32) return "empty rank array";
  if (input.rect_count > static_cast<std::size_t>(0x7fffffff)) return "rectangle count exceeds the CUB int range";
  if (input.rect_count != 0 && (input.rect_a == nullptr || input.rect_b == nullptr || input.rect_mask == nullptr))
    return "null rectangle arrays";
  for (std::size_t i = 0; i < 3 * input.rank_count; ++i)
    if (input.rank_points[i] < 0 || input.rank_points[i] > limit) return "point coordinate outside the u18 domain";
  const FlatNode& root = input.nodes[0];
  if (root.first != 0 || root.last != input.rank_count) return "root does not cover every rank";
  for (std::size_t i = 0; i < input.node_count; ++i) {
    const FlatNode& node = input.nodes[i];
    if (node.first >= node.last || node.last > input.rank_count) return "node rank range outside the index";
    for (int axis = 0; axis < 3; ++axis)
      if (node.box.low[axis] < 0 || node.box.low[axis] > node.box.high[axis] || node.box.high[axis] > limit)
        return "node box outside the u18 domain or inverted";
    const bool leaf = node.left == absent32;
    if (leaf != (node.right == absent32)) return "node with one child";
    if (!leaf) {
      if (node.left >= input.node_count || node.right >= input.node_count || node.left <= i || node.right <= i)
        return "child id outside the index or not after its parent";
      const FlatNode& left = input.nodes[node.left];
      const FlatNode& right = input.nodes[node.right];
      if (left.first != node.first || left.last != right.first || right.last != node.last)
        return "children do not partition their parent's ranks";
    }
    for (u32 r = node.first; r < node.last; ++r)
      for (int axis = 0; axis < 3; ++axis) {
        const std::int32_t c = input.rank_points[3 * static_cast<std::size_t>(r) + axis];
        if (c < node.box.low[axis] || c > node.box.high[axis]) return "node box misses a point of its ranks";
      }
  }
  for (std::size_t i = 0; i < input.rect_count; ++i)
    if (input.rect_a[i] >= input.node_count || input.rect_b[i] >= input.node_count || (input.rect_mask[i] & ~6U) != 0)
      return "rectangle node id or lane mask outside the domain";
  return {};
}

// Defined in filter_runner.cu when MHGP9_ENABLE_CUDA, else a stub that
// returns available=false. Refuses (error) any input validate_filter_input
// refuses, before touching the device.
FilterOutput run_filters(const FilterInput& input);

// S2 (chain): one device pass, then only the surviving pairs come back,
// compacted on the device in rectangle order (row-major inside each
// rectangle), with their spatial ranks and lanes; plus the rectangle masks
// and the lane rejections among the expanded pairs. Same validation and
// kernels as run_filters; `repeats` is ignored (one pass).
struct BatchOutput {
  bool available = false;
  std::string device;
  std::string error;  // non-empty: nothing below is valid
  bool stack_failure = false;
  std::vector<u8> rect_masks;
  std::vector<u32> survivor_a, survivor_b;  // spatial ranks
  std::vector<u8> survivor_mask;
  std::uint64_t pairs = 0, pair_q3_rejected = 0, pair_q4_rejected = 0;
  std::uint64_t rect_visits = 0, pair_visits = 0;
  // cudaEvent timings of the pass (ms): upload, rectangle kernel, mass scan,
  // pair kernel, survivor compaction, download of masks and survivors.
  double upload_ms = 0, rect_ms = 0, scan_ms = 0, pair_ms = 0, select_ms = 0, download_ms = 0, total_ms = 0;
};
BatchOutput run_filter_batch(const FilterInput& input);

}  // namespace mhgp9::gpu
