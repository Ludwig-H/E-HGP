#include "spatial/float32_index.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace mhgp8 {
namespace {

using float32_predicate_detail::count;

std::size_t sum_bytes(std::size_t current, std::size_t size, std::size_t width) {
  if (size > (std::numeric_limits<std::size_t>::max() - current) / width)
    throw std::length_error("mhgp8 float32 index storage is not addressable");
  return current + size * width;
}

bool same_site(const Float32Point3& a, const Float32Point3& b) noexcept {
  for (std::size_t axis = 0; axis != 3; ++axis)
    if (float32_order_key(a.bits()[axis]) != float32_order_key(b.bits()[axis])) return false;
  return true;
}

}  // namespace

Float32Box3 Float32Box3::from_corners(Float32Words low, Float32Words high) {
  static_cast<void>(Float32Point3::from_bits(low));
  static_cast<void>(Float32Point3::from_bits(high));
  for (std::size_t axis = 0; axis != 3; ++axis)
    if (float32_order_key(low[axis]) > float32_order_key(high[axis]))
      throw std::invalid_argument("mhgp8 float32 box has reversed bounds");
  return Float32Box3(low, high);
}

Float32IndexPtr prepare_float32_index(std::span<const Float32Words> input) {
  auto result = std::shared_ptr<Float32CloudIndex>(new Float32CloudIndex());
  {
    const std::vector<Float32Words> snapshot(input.begin(), input.end());
    const auto n = snapshot.size();
    if (n == 0) throw std::invalid_argument("mhgp8 float32 index requires a nonempty cloud");
    count(result->work_.input_word_triples_copied, n);
    result->points_.reserve(n);
    result->peak_bytes_ = sum_bytes(sum_bytes(0, snapshot.capacity(), sizeof(Float32Words)),
                                    result->points_.capacity(), sizeof(Float32Point3));
    for (const auto& words : snapshot) {
      result->points_.push_back(Float32Point3::from_bits(words));
      count(result->work_.finite_points_validated);
      count(result->work_.point_objects_constructed);
    }
  }
  const auto n = result->points_.size();
  std::array<std::vector<std::size_t>, 3> order;
  for (std::size_t axis = 0; axis != 3; ++axis) {
    order[axis].resize(n);
    std::iota(order[axis].begin(), order[axis].end(), std::size_t{0});
    std::sort(order[axis].begin(), order[axis].end(), [&](std::size_t a, std::size_t b) {
      count(result->work_.presort_comparisons);
      // Lexicographic rotation gives a total geometric order. Original ID
      // breaks a full coordinate tie (also making duplicate checks adjacent).
      for (std::size_t offset = 0; offset != 3; ++offset) {
        const auto dim = (axis + offset) % 3;
        const auto av = float32_order_key(result->points_[a].bits()[dim]);
        const auto bv = float32_order_key(result->points_[b].bits()[dim]);
        if (av != bv) return av < bv;
      }
      return a < b;
    });
  }
  for (std::size_t i = 1; i != n; ++i) {
    count(result->work_.duplicate_adjacent_tests);
    if (same_site(result->points_[order[0][i - 1]], result->points_[order[0][i]]))
      throw std::invalid_argument("mhgp8 float32 index requires distinct geometric sites");
  }
  if (n > result->nodes_.max_size() / 2)
    throw std::length_error("mhgp8 float32 index node storage is not addressable");
  result->nodes_.reserve(2 * n - 1);
  result->rank_.resize(n);
  std::vector<std::size_t> scratch(n);
  auto construction_bytes = result->retained_bytes();
  construction_bytes = sum_bytes(construction_bytes, scratch.capacity(), sizeof(std::size_t));
  for (const auto& sorted : order)
    construction_bytes = sum_bytes(construction_bytes, sorted.capacity(), sizeof(std::size_t));
  result->peak_bytes_ = std::max(result->peak_bytes_, construction_bytes);
  static_cast<void>(result->build(order, scratch, 0, n, 0));
  result->permutation_ = std::move(order[0]);
  for (std::size_t i = 0; i != n; ++i) {
    result->rank_[result->permutation_[i]] = i;
    count(result->work_.inverse_rank_writes);
  }
  return result;
}

std::size_t Float32CloudIndex::build(std::array<std::vector<std::size_t>, 3>& order,
                                    std::vector<std::size_t>& scratch, std::size_t first,
                                    std::size_t last, std::size_t depth) {
  Float32Words low{}, high{};
  for (std::size_t axis = 0; axis != 3; ++axis) {
    low[axis] = points_[order[axis][first]].bits()[axis];
    high[axis] = points_[order[axis][last - 1]].bits()[axis];
    count(work_.box_endpoint_reads, 2);
  }
  const auto current = nodes_.size();
  nodes_.push_back({first, last, Float32IndexNode::none, Float32IndexNode::none,
                    0, Float32Box3(low, high)});
  count(work_.nodes);
  max_depth_ = std::max(max_depth_, depth);
  if (last - first == 1) {
    nodes_[current].escape = nodes_.size();
    count(work_.leaves);
    return current;
  }
  // The rounded length chooses a partition only, never certifies a geometric
  // decision. All boxes retain exact endpoint bits; medians guarantee balance
  // even if tiny differences tie under another floating rounding mode.
  std::size_t selected = 0;
  double longest = -1;
  for (std::size_t axis = 0; axis != 3; ++axis) {
    const double length = float32_predicate_detail::as_double(high[axis]) -
                          float32_predicate_detail::as_double(low[axis]);
    if (length > longest) { longest = length; selected = axis; }
  }
  const auto middle = first + (last - first) / 2;
  for (std::size_t i = first; i != last; ++i) {
    rank_[order[selected][i]] = i;
    count(work_.partition_rank_writes);
    count(work_.partition_id_reads);
  }
  for (std::size_t axis = 0; axis != 3; ++axis) {
    if (axis == selected) continue;
    auto left = first, right = middle;
    for (std::size_t i = first; i != last; ++i) {
      const auto id = order[axis][i];
      scratch[rank_[id] < middle ? left++ : right++] = id;
      count(work_.partition_id_reads);
      count(work_.partition_id_writes);
    }
    if (left != middle || right != last)
      throw std::logic_error("mhgp8 float32 median partition lost its common site set");
    for (std::size_t i = first; i != last; ++i) {
      order[axis][i] = scratch[i];
      count(work_.partition_id_reads);
      count(work_.partition_id_writes);
    }
  }
  const auto left = build(order, scratch, first, middle, depth + 1);
  const auto right = build(order, scratch, middle, last, depth + 1);
  nodes_[current].left = left;
  nodes_[current].right = right;
  nodes_[current].escape = nodes_.size();
  return current;
}

std::size_t Float32CloudIndex::retained_bytes() const {
  auto bytes = sum_bytes(0, points_.capacity(), sizeof(Float32Point3));
  bytes = sum_bytes(bytes, permutation_.capacity(), sizeof(std::size_t));
  bytes = sum_bytes(bytes, rank_.capacity(), sizeof(std::size_t));
  return sum_bytes(bytes, nodes_.capacity(), sizeof(Float32IndexNode));
}

void Float32CloudIndex::visit_box(const Float32Box3& query,
    const std::function<void(std::span<const std::size_t>)>& emit,
    Float32BoxQueryWork& work) const {
  if (!emit) throw std::invalid_argument("mhgp8 float32 box query requires a callback");
  std::size_t current = 0;
  while (current != nodes_.size()) {
    const auto& node = nodes_[current];
    count(work.node_visits);
    bool outside = false, inside = true;
    for (std::size_t axis = 0; axis != 3; ++axis) {
      count(work.axis_tests);
      const auto nl = float32_order_key(node.box.low()[axis]);
      const auto nh = float32_order_key(node.box.high()[axis]);
      const auto ql = float32_order_key(query.low()[axis]);
      const auto qh = float32_order_key(query.high()[axis]);
      if (nh < ql || nl > qh) { outside = true; break; }
      inside = inside && ql <= nl && nh <= qh;
    }
    if (outside) {
      count(work.rejected_nodes);
      current = node.escape;
    } else if (inside) {
      count(work.accepted_nodes);
      count(work.emitted_sites, node.last - node.first);
      count(work.callbacks);
      emit(permutation().subspan(node.first, node.last - node.first));
      current = node.escape;
    } else {
      if (node.leaf()) throw std::logic_error("mhgp8 float32 singleton box was undecidable");
      count(work.refined_nodes);
      current = node.left;
    }
  }
}

}  // namespace mhgp8
