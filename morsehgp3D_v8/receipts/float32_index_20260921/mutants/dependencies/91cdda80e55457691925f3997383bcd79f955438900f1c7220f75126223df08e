#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "core/float32_predicates.hpp"

namespace mhgp8 {

using Float32Words = std::array<std::uint32_t, 3>;

// Total numerical order on finite binary32, with the two zero encodings equal.
[[nodiscard]] inline std::uint32_t float32_order_key(std::uint32_t word) noexcept {
  return (word & 0x80000000U) != 0 ? ~word : word ^ 0x80000000U;
}

class Float32Box3 final {
 public:
  [[nodiscard]] static Float32Box3 from_corners(Float32Words low, Float32Words high);
  [[nodiscard]] const Float32Words& low() const noexcept { return low_; }
  [[nodiscard]] const Float32Words& high() const noexcept { return high_; }

 private:
  Float32Box3(Float32Words low, Float32Words high) : low_(low), high_(high) {}
  Float32Words low_, high_;
  friend class Float32CloudIndex;
};

struct Float32IndexWork {
  std::uint64_t input_word_triples_copied{};
  std::uint64_t finite_points_validated{};
  std::uint64_t point_objects_constructed{};
  std::uint64_t presort_comparisons{};
  std::uint64_t duplicate_adjacent_tests{};
  std::uint64_t box_endpoint_reads{};
  std::uint64_t partition_rank_writes{};
  std::uint64_t partition_id_reads{};
  std::uint64_t partition_id_writes{};
  std::uint64_t inverse_rank_writes{};
  std::uint64_t nodes{};
  std::uint64_t leaves{};
};

struct Float32BoxQueryWork {
  std::uint64_t node_visits{};
  std::uint64_t axis_tests{};
  std::uint64_t rejected_nodes{};
  std::uint64_t accepted_nodes{};
  std::uint64_t refined_nodes{};
  std::uint64_t emitted_sites{};
  std::uint64_t callbacks{};
};

// Flat preorder node. IDs/ranks are size_t, never packed with coordinates.
// escape is the first node after this entire subtree (possibly nodes.size()).
struct Float32IndexNode {
  static constexpr std::size_t none = static_cast<std::size_t>(-1);
  std::size_t first{}, last{};
  std::size_t left{none}, right{none}, escape{};
  Float32Box3 box;
  [[nodiscard]] bool leaf() const noexcept { return left == none; }
};

class Float32CloudIndex;
using Float32IndexPtr = std::shared_ptr<const Float32CloudIndex>;

// Copies all raw triples into private storage before validation. Input order
// defines original IDs; duplicate geometric sites (including +/-0) are rejected.
// Median splits reuse three presorted ID arrays: O(n log n) construction work,
// O(n) storage, depth <= ceil(log2 n), independently of exponent range.
[[nodiscard]] Float32IndexPtr prepare_float32_index(std::span<const Float32Words> input);

class Float32CloudIndex final {
 public:
  Float32CloudIndex(const Float32CloudIndex&) = delete;
  Float32CloudIndex& operator=(const Float32CloudIndex&) = delete;
  Float32CloudIndex(Float32CloudIndex&&) = delete;
  Float32CloudIndex& operator=(Float32CloudIndex&&) = delete;
  [[nodiscard]] std::span<const Float32Point3> points() const noexcept { return points_; }
  [[nodiscard]] std::span<const std::size_t> permutation() const noexcept { return permutation_; }
  [[nodiscard]] std::span<const std::size_t> inverse() const noexcept { return rank_; }
  [[nodiscard]] std::span<const Float32IndexNode> nodes() const noexcept { return nodes_; }
  [[nodiscard]] const Float32IndexWork& work() const noexcept { return work_; }
  [[nodiscard]] std::size_t max_depth() const noexcept { return max_depth_; }
  [[nodiscard]] std::size_t retained_bytes() const;
  // Coupled vector capacities during construction, not RSS/allocator overhead.
  [[nodiscard]] std::size_t construction_peak_vector_bytes() const noexcept { return peak_bytes_; }

  // Closed-box query using exact bit-order comparisons, no floating predicate.
  // Emits borrowed spans of ORIGINAL IDs; arbitrary callback order is not
  // promised. No mutable owner state, allocation or explicit traversal stack.
  // The index/callback/work must outlive this synchronous call. On exception,
  // prior callbacks/work remain; different workers must use private work.
  void visit_box(const Float32Box3& query,
                 const std::function<void(std::span<const std::size_t>)>& emit,
                 Float32BoxQueryWork& work) const;

 private:
  Float32CloudIndex() = default;
  friend Float32IndexPtr prepare_float32_index(std::span<const Float32Words>);
  std::size_t build(std::array<std::vector<std::size_t>, 3>& order,
                    std::vector<std::size_t>& scratch, std::size_t first,
                    std::size_t last, std::size_t depth);
  std::vector<Float32Point3> points_;
  std::vector<std::size_t> permutation_, rank_;
  std::vector<Float32IndexNode> nodes_;
  Float32IndexWork work_{};
  std::size_t max_depth_{}, peak_bytes_{};
};

}  // namespace mhgp8
