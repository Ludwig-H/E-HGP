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

// Defined in filter_runner.cu when MHGP9_ENABLE_CUDA, else a stub that
// returns available=false.
FilterOutput run_filters(const FilterInput& input);

}  // namespace mhgp9::gpu
