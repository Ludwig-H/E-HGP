#include "morsehgp3d/gpu/exact_higher_support_product_cuda.hpp"

#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactHigherSupportProductCudaBackend;
using morsehgp3d::gpu::ExactHigherSupportProductCudaContext;
using morsehgp3d::gpu::ExactHigherSupportProductCudaOutcome;
using morsehgp3d::gpu::ExactHigherSupportProductCudaStatus;
using morsehgp3d::gpu::ExactHigherSupportProductCudaStopReason;
using morsehgp3d::gpu::ExactHigherSupportProductCudaTask;
using morsehgp3d::gpu::ExactHigherSupportProductCudaTaskKind;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::hierarchy::
    ExactHigherSupportProductAabbDecisionBackend;
using morsehgp3d::hierarchy::ExactHigherSupportFrontierEntry;
using morsehgp3d::hierarchy::ExactHigherSupportNodeReceipt;
using morsehgp3d::hierarchy::
    exact_higher_support_product_no_well_centered_certified;
using morsehgp3d::hierarchy::
    exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

inline constexpr std::size_t qualification_task_count = 11U;
inline constexpr std::size_t fixture_family_count = 6U;

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

// Six logical fixture families share one immutable PointId namespace so all
// eleven predicates use one resident epoch, one batch and one synchronization;
// the mixed exact-width batch exercises the separate int512/int1024 kernels.
[[nodiscard]] CanonicalPointCloud qualification_cloud() {
  const double denormal = std::numeric_limits<double>::denorm_min();
  const std::array<CertifiedPoint3, 24> points{
      // Acute triangle, exact centre and a distinct shell point.
      point(-1.0, 0.0, 0.0),
      point(1.0, 0.0, 0.0),
      point(0.0, 2.0, 0.0),
      point(0.0, 0.75, 0.0),
      point(0.0, -0.5, 0.0),
      // Collinear positive support-prune fixture.
      point(10.0, 0.0, 0.0),
      point(11.0, 0.0, 0.0),
      point(12.0, 0.0, 0.0),
      // Regular tetrahedron and its centre.
      point(1.0, 1.0, 1.0),
      point(1.0, -1.0, -1.0),
      point(-1.0, 1.0, -1.0),
      point(-1.0, -1.0, 1.0),
      point(0.0, 0.0, 0.0),
      // Translated right tetrahedron and a distinct exact shell point.
      point(20.0, 0.0, 0.0),
      point(22.0, 0.0, 0.0),
      point(20.0, 2.0, 0.0),
      point(20.0, 0.0, 2.0),
      point(22.0, 2.0, 2.0),
      // Exponent span above 124 bits: integral kernel must fall back.
      point(denormal, 30.0, 0.0),
      point(1.0, 30.0, 0.0),
      point(1.0, 31.0, 0.0),
      // W=62: outside int512, inside the existing int1024 envelope.
      point(0.0, 0.0, std::ldexp(1.0, -61)),
      point(1.0, 0.0, std::ldexp(1.0, -61)),
      point(0.0, 1.0, std::ldexp(1.0, -61))};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] PointId canonical_point_id_for_source(
    const CanonicalPointCloud& cloud,
    std::size_t source_index) {
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    const auto point_id = static_cast<PointId>(index);
    if (cloud.source_index(point_id) == source_index) {
      return point_id;
    }
  }
  throw std::logic_error(
      "a qualification source point has no canonical PointId");
}

struct LeafNodeFixture {
  std::vector<std::uint64_t> node_index_by_leaf_position;
  std::vector<std::size_t> leaf_position_by_point_id;
  std::vector<ExactHigherSupportNodeReceipt> internal_node_receipts;
};

// Certified snapshots are strict left/right postorder.  Replaying the public
// Morton radix split therefore recovers leaf node indices without exposing a
// second node snapshot; the production context authenticates every receipt.
[[nodiscard]] LeafNodeFixture individual_leaf_nodes(
    const MortonLbvhIndex& index) {
  const auto leaves = index.leaves();
  if (leaves.empty()) {
    throw std::logic_error("the qualification LBVH has no leaves");
  }
  LeafNodeFixture fixture;
  fixture.node_index_by_leaf_position.resize(leaves.size());
  fixture.leaf_position_by_point_id.resize(leaves.size());
  std::size_t next_node_index = 0U;

  const auto find_split = [leaves](std::size_t begin, std::size_t end) {
    const std::uint64_t first_code = leaves[begin].morton_code;
    const std::uint64_t last_code = leaves[end - 1U].morton_code;
    if (first_code == last_code) {
      return begin + (end - begin) / 2U;
    }
    const std::uint64_t difference = first_code ^ last_code;
    const unsigned int highest_bit =
        static_cast<unsigned int>(std::bit_width(difference)) - 1U;
    const std::uint64_t mask = UINT64_C(1) << highest_bit;
    std::size_t low = begin + 1U;
    std::size_t high = end;
    while (low < high) {
      const std::size_t middle = low + (high - low) / 2U;
      if ((leaves[middle].morton_code & mask) == 0U) {
        low = middle + 1U;
      } else {
        high = middle;
      }
    }
    if (low <= begin || low >= end) {
      throw std::logic_error(
          "the qualification radix split did not divide its range");
    }
    return low;
  };

  std::function<void(std::size_t, std::size_t)> replay_postorder;
  replay_postorder = [&](std::size_t begin, std::size_t end) {
    if (end - begin == 1U) {
      if (!std::in_range<std::uint64_t>(next_node_index)) {
        throw std::length_error(
            "a qualification leaf node index does not fit uint64");
      }
      fixture.node_index_by_leaf_position[begin] =
          static_cast<std::uint64_t>(next_node_index++);
      return;
    }
    const std::size_t split = find_split(begin, end);
    replay_postorder(begin, split);
    replay_postorder(split, end);
    if (!std::in_range<std::uint64_t>(next_node_index) ||
        !std::in_range<std::uint64_t>(begin) ||
        !std::in_range<std::uint64_t>(end)) {
      throw std::length_error(
          "a qualification internal node receipt does not fit uint64");
    }
    fixture.internal_node_receipts.push_back(
        ExactHigherSupportNodeReceipt{
            static_cast<std::uint64_t>(next_node_index),
            static_cast<std::uint64_t>(begin),
            static_cast<std::uint64_t>(end)});
    ++next_node_index;
  };
  replay_postorder(0U, leaves.size());
  if (next_node_index != index.build_counters().node_count) {
    throw std::logic_error(
        "the qualification postorder replay lost an LBVH node");
  }
  for (std::size_t position = 0U; position < leaves.size(); ++position) {
    const PointId point_id = leaves[position].point_id;
    if (!std::in_range<std::size_t>(point_id) ||
        static_cast<std::size_t>(point_id) >= leaves.size()) {
      throw std::logic_error(
          "a qualification LBVH leaf PointId is out of range");
    }
    fixture.leaf_position_by_point_id[static_cast<std::size_t>(point_id)] =
        position;
  }
  return fixture;
}

[[nodiscard]] ExactHigherSupportNodeReceipt smallest_internal_receipt(
    const LeafNodeFixture& fixture,
    std::uint64_t minimum_leaf_count) {
  const auto found = std::min_element(
      fixture.internal_node_receipts.begin(),
      fixture.internal_node_receipts.end(),
      [minimum_leaf_count](
          const ExactHigherSupportNodeReceipt& left,
          const ExactHigherSupportNodeReceipt& right) {
        const std::uint64_t left_count = left.leaf_end - left.leaf_begin;
        const std::uint64_t right_count = right.leaf_end - right.leaf_begin;
        const bool left_eligible = left_count >= minimum_leaf_count;
        const bool right_eligible = right_count >= minimum_leaf_count;
        if (left_eligible != right_eligible) {
          return left_eligible;
        }
        return left_count != right_count
            ? left_count < right_count
            : left.leaf_begin < right.leaf_begin;
      });
  if (found == fixture.internal_node_receipts.end() ||
      found->leaf_end - found->leaf_begin < minimum_leaf_count) {
    throw std::logic_error(
        "the qualification LBVH has no sufficiently large internal node");
  }
  return *found;
}

[[nodiscard]] ExactHigherSupportNodeReceipt individual_leaf_receipt(
    const LeafNodeFixture& fixture,
    PointId point_id) {
  if (!std::in_range<std::size_t>(point_id) ||
      static_cast<std::size_t>(point_id) >=
          fixture.leaf_position_by_point_id.size()) {
    throw std::logic_error(
        "a qualification singleton PointId is out of range");
  }
  const std::size_t position =
      fixture.leaf_position_by_point_id[static_cast<std::size_t>(point_id)];
  return ExactHigherSupportNodeReceipt{
      fixture.node_index_by_leaf_position[position],
      static_cast<std::uint64_t>(position),
      static_cast<std::uint64_t>(position + 1U)};
}

template <std::size_t SupportSize>
struct ProductFixture {
  ExactHigherSupportFrontierEntry product{};
  std::array<PointId, SupportSize> ordered_point_ids{};
};

template <std::size_t SupportSize>
[[nodiscard]] ProductFixture<SupportSize> individual_leaf_product(
    const LeafNodeFixture& fixture,
    const std::array<PointId, SupportSize>& point_ids) {
  static_assert(SupportSize == 3U || SupportSize == 4U);
  struct Entry {
    ExactHigherSupportNodeReceipt receipt{};
    PointId point_id{};
  };
  std::array<Entry, SupportSize> entries{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    entries[index] = Entry{
        individual_leaf_receipt(fixture, point_ids[index]),
        point_ids[index]};
  }
  std::sort(
      entries.begin(),
      entries.end(),
      [](const Entry& left, const Entry& right) {
        return left.receipt.leaf_begin < right.receipt.leaf_begin;
      });

  ProductFixture<SupportSize> fixture_result;
  fixture_result.product.support_size =
      static_cast<std::uint8_t>(SupportSize);
  fixture_result.product.group_count =
      static_cast<std::uint8_t>(SupportSize);
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    fixture_result.product.groups[index] = {
        entries[index].receipt.node_index,
        entries[index].receipt.leaf_begin,
        entries[index].receipt.leaf_end,
        1U};
    fixture_result.ordered_point_ids[index] = entries[index].point_id;
  }
  return fixture_result;
}

[[nodiscard]] ExactDyadicAabb3 point_box(
    const CanonicalPointCloud& cloud,
    PointId point_id) {
  const auto words = cloud.point(point_id).canonical_input_bits();
  return ExactDyadicAabb3{words, words};
}

[[nodiscard]] ExactDyadicAabb3 public_range_box(
    const CanonicalPointCloud& cloud,
    const MortonLbvhIndex& index,
    const ExactHigherSupportNodeReceipt& receipt) {
  const auto leaves = index.leaves();
  if (!std::in_range<std::size_t>(receipt.leaf_begin) ||
      !std::in_range<std::size_t>(receipt.leaf_end)) {
    throw std::length_error(
        "a qualification internal range does not fit size_t");
  }
  const std::size_t begin =
      static_cast<std::size_t>(receipt.leaf_begin);
  const std::size_t end = static_cast<std::size_t>(receipt.leaf_end);
  if (begin >= end || end > leaves.size()) {
    throw std::logic_error(
        "a qualification internal range is outside the public leaves");
  }
  std::array<PointId, 3> lower_ids{
      leaves[begin].point_id,
      leaves[begin].point_id,
      leaves[begin].point_id};
  std::array<PointId, 3> upper_ids = lower_ids;
  for (std::size_t position = begin + 1U; position < end; ++position) {
    const PointId point_id = leaves[position].point_id;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      if (cloud.point(point_id).coordinate(axis) <
          cloud.point(lower_ids[axis]).coordinate(axis)) {
        lower_ids[axis] = point_id;
      }
      if (cloud.point(upper_ids[axis]).coordinate(axis) <
          cloud.point(point_id).coordinate(axis)) {
        upper_ids[axis] = point_id;
      }
    }
  }
  ExactDyadicAabb3 box{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    box.lower_binary64_bits[axis] =
        cloud.point(lower_ids[axis]).canonical_input_bits()[axis];
    box.upper_binary64_bits[axis] =
        cloud.point(upper_ids[axis]).canonical_input_bits()[axis];
  }
  return box;
}

struct QualifiedTask {
  ExactHigherSupportProductCudaTask task{};
  bool expected_certified{false};
  bool uses_internal_node_receipt{false};
  ExactHigherSupportProductAabbDecisionBackend expected_backend{
      ExactHigherSupportProductAabbDecisionBackend::
          arbitrary_precision_rational};
};

template <std::size_t SupportSize>
[[nodiscard]] QualifiedTask support_task(
    const CanonicalPointCloud& cloud,
    const LeafNodeFixture& leaf_nodes,
    std::uint64_t epoch,
    std::uint64_t task_id,
    const std::array<std::size_t, SupportSize>& source_indices) {
  std::array<PointId, SupportSize> point_ids{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    point_ids[index] =
        canonical_point_id_for_source(cloud, source_indices[index]);
  }
  const auto product = individual_leaf_product(leaf_nodes, point_ids);
  std::array<ExactDyadicAabb3, SupportSize> boxes{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    boxes[index] = point_box(cloud, product.ordered_point_ids[index]);
  }
  QualifiedTask result;
  result.task.task_id = task_id;
  result.task.source_snapshot_epoch = epoch;
  result.task.kind = ExactHigherSupportProductCudaTaskKind::support_prune;
  result.task.product = product.product;
  result.expected_certified =
      exact_higher_support_product_no_well_centered_certified(
          boxes, &result.expected_backend);
  return result;
}

[[nodiscard]] QualifiedTask internal_node_support_task(
    const CanonicalPointCloud& cloud,
    const MortonLbvhIndex& index,
    std::uint64_t epoch,
    std::uint64_t task_id,
    const ExactHigherSupportNodeReceipt& receipt) {
  const std::uint64_t leaf_count = receipt.leaf_end - receipt.leaf_begin;
  if (leaf_count < 3U) {
    throw std::logic_error(
        "an internal qualification support requires at least three leaves");
  }
  const ExactDyadicAabb3 box = public_range_box(cloud, index, receipt);
  const std::array<ExactDyadicAabb3, 3> boxes{box, box, box};
  QualifiedTask result;
  result.task.task_id = task_id;
  result.task.source_snapshot_epoch = epoch;
  result.task.kind = ExactHigherSupportProductCudaTaskKind::support_prune;
  result.task.product.support_size = 3U;
  result.task.product.group_count = 1U;
  result.task.product.groups[0] = {
      receipt.node_index,
      receipt.leaf_begin,
      receipt.leaf_end,
      3U};
  result.expected_certified =
      exact_higher_support_product_no_well_centered_certified(
          boxes, &result.expected_backend);
  result.uses_internal_node_receipt = true;
  return result;
}

template <std::size_t SupportSize>
[[nodiscard]] QualifiedTask query_task(
    const CanonicalPointCloud& cloud,
    const LeafNodeFixture& leaf_nodes,
    std::uint64_t epoch,
    std::uint64_t task_id,
    const std::array<std::size_t, SupportSize>& source_indices,
    std::size_t query_source_index) {
  QualifiedTask result = support_task(
      cloud, leaf_nodes, epoch, task_id, source_indices);
  result.task.kind =
      ExactHigherSupportProductCudaTaskKind::query_strict_interior;
  const PointId query_point_id =
      canonical_point_id_for_source(cloud, query_source_index);
  result.task.query_node =
      individual_leaf_receipt(leaf_nodes, query_point_id);

  std::array<ExactDyadicAabb3, SupportSize> boxes{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    const auto& group = result.task.product.groups[index];
    const auto found = std::find_if(
        leaf_nodes.leaf_position_by_point_id.begin(),
        leaf_nodes.leaf_position_by_point_id.end(),
        [&](std::size_t position) {
          return position == group.leaf_begin;
        });
    if (found == leaf_nodes.leaf_position_by_point_id.end()) {
      throw std::logic_error(
          "a qualification product group lost its singleton point");
    }
    const std::size_t point_index = static_cast<std::size_t>(
        found - leaf_nodes.leaf_position_by_point_id.begin());
    boxes[index] = point_box(cloud, static_cast<PointId>(point_index));
  }
  result.expected_certified =
      exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
          boxes,
          point_box(cloud, query_point_id),
          &result.expected_backend);
  return result;
}

[[nodiscard]] std::string json_escape(const std::string& value) {
  std::string output;
  output.reserve(value.size());
  for (const char character : value) {
    switch (character) {
      case '\\':
        output += "\\\\";
        break;
      case '"':
        output += "\\\"";
        break;
      case '\n':
        output += "\\n";
        break;
      case '\r':
        output += "\\r";
        break;
      case '\t':
        output += "\\t";
        break;
      default:
        output += character;
        break;
    }
  }
  return output;
}

[[nodiscard]] std::uint64_t nanoseconds_between(
    std::chrono::steady_clock::time_point begin,
    std::chrono::steady_clock::time_point end) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count());
}

}  // namespace

int main() {
  const auto total_begin = std::chrono::steady_clock::now();
  try {
    const CanonicalPointCloud cloud = qualification_cloud();
    MortonLbvhBuildContext builder{cloud.size()};
    const auto lbvh_begin = std::chrono::steady_clock::now();
    auto build = builder.build(cloud);
    const auto lbvh_end = std::chrono::steady_clock::now();
    if (!build.cuda_qualified_build()) {
      throw std::logic_error(
          "qualification requires a real CUDA-certified LBVH build");
    }
    const MortonLbvhIndex& index = build.certified_index();
    const LeafNodeFixture leaf_nodes = individual_leaf_nodes(index);
    const ExactHigherSupportNodeReceipt internal_receipt =
        smallest_internal_receipt(leaf_nodes, 3U);
    auto traversal = builder.release_device_traversal_lease(build);
    if (!traversal.cuda_resident() ||
        traversal.audit().host_fake_lifecycle_exercised) {
      throw std::logic_error(
          "qualification requires a real resident CUDA traversal lease");
    }
    const std::uint64_t epoch = traversal.audit().source_snapshot_epoch;

    std::array<QualifiedTask, qualification_task_count> qualified_tasks{
        support_task(
            cloud, leaf_nodes, epoch, 100U,
            std::array<std::size_t, 3>{5U, 6U, 7U}),
        support_task(
            cloud, leaf_nodes, epoch, 101U,
            std::array<std::size_t, 3>{0U, 1U, 2U}),
        query_task(
            cloud, leaf_nodes, epoch, 102U,
            std::array<std::size_t, 3>{0U, 1U, 2U}, 3U),
        query_task(
            cloud, leaf_nodes, epoch, 103U,
            std::array<std::size_t, 3>{0U, 1U, 2U}, 4U),
        support_task(
            cloud, leaf_nodes, epoch, 104U,
            std::array<std::size_t, 4>{13U, 14U, 15U, 16U}),
        support_task(
            cloud, leaf_nodes, epoch, 105U,
            std::array<std::size_t, 4>{8U, 9U, 10U, 11U}),
        query_task(
            cloud, leaf_nodes, epoch, 106U,
            std::array<std::size_t, 4>{8U, 9U, 10U, 11U}, 12U),
        query_task(
            cloud, leaf_nodes, epoch, 107U,
            std::array<std::size_t, 4>{13U, 14U, 15U, 16U}, 17U),
        support_task(
            cloud, leaf_nodes, epoch, 108U,
            std::array<std::size_t, 3>{18U, 19U, 20U}),
        internal_node_support_task(
            cloud, index, epoch, 109U, internal_receipt),
        support_task(
            cloud, leaf_nodes, epoch, 110U,
            std::array<std::size_t, 3>{21U, 22U, 23U})};

    std::vector<ExactHigherSupportProductCudaTask> tasks;
    tasks.reserve(qualified_tasks.size());
    std::size_t expected_certified_count = 0U;
    std::size_t expected_fallback_count = 0U;
    std::size_t expected_support_task_count = 0U;
    std::size_t expected_query_task_count = 0U;
    std::size_t internal_node_receipt_task_count = 0U;
    for (const QualifiedTask& task : qualified_tasks) {
      tasks.push_back(task.task);
      expected_certified_count += task.expected_certified ? 1U : 0U;
      expected_fallback_count +=
          task.expected_backend ==
                  ExactHigherSupportProductAabbDecisionBackend::
                      arbitrary_precision_rational
          ? 1U
          : 0U;
      expected_support_task_count +=
          task.task.kind ==
                  ExactHigherSupportProductCudaTaskKind::support_prune
          ? 1U
          : 0U;
      expected_query_task_count +=
          task.task.kind == ExactHigherSupportProductCudaTaskKind::
                  query_strict_interior
          ? 1U
          : 0U;
      internal_node_receipt_task_count +=
          task.uses_internal_node_receipt ? 1U : 0U;
    }
    if (qualified_tasks[8].expected_backend !=
            ExactHigherSupportProductAabbDecisionBackend::
                arbitrary_precision_rational ||
        expected_fallback_count == 0U) {
      throw std::logic_error(
          "qualification lost its explicit wide-exponent fallback");
    }
    if (internal_node_receipt_task_count != 1U) {
      throw std::logic_error(
          "qualification did not isolate one internal-node receipt task");
    }

    ExactHigherSupportProductCudaContext context{
        index, cloud, std::move(traversal), tasks.size()};
    const auto batch_begin = std::chrono::steady_clock::now();
    const auto result = context.evaluate(tasks);
    const auto batch_end = std::chrono::steady_clock::now();

    std::size_t mismatch_count = 0U;
    if (result.records.size() != qualified_tasks.size()) {
      mismatch_count = qualified_tasks.size();
    } else {
      for (std::size_t task_index = 0U;
           task_index < qualified_tasks.size();
           ++task_index) {
        const QualifiedTask& expected = qualified_tasks[task_index];
        const auto& actual = result.records[task_index];
        const bool expected_rational_backend =
            expected.expected_backend ==
            ExactHigherSupportProductAabbDecisionBackend::
                arbitrary_precision_rational;
        const bool expected_cpu_fallback =
            expected_rational_backend;
        const bool backend_matches = expected_rational_backend
            ? actual.backend == ExactHigherSupportProductCudaBackend::
                  arbitrary_precision_rational
            : actual.backend == ExactHigherSupportProductCudaBackend::
                      bounded_dyadic_int512 ||
                  actual.backend == ExactHigherSupportProductCudaBackend::
                      bounded_dyadic_int1024;
        const bool explicit_int1024_boundary_matches =
            expected.task.task_id != 110U ||
            actual.backend == ExactHigherSupportProductCudaBackend::
                bounded_dyadic_int1024;
        const ExactHigherSupportProductCudaOutcome expected_outcome =
            expected.expected_certified
            ? ExactHigherSupportProductCudaOutcome::certified
            : ExactHigherSupportProductCudaOutcome::fail_open;
        if (actual.task_id != expected.task.task_id ||
            actual.kind != expected.task.kind ||
            actual.outcome != expected_outcome ||
            !backend_matches || !explicit_int1024_boundary_matches ||
            actual.cpu_fallback_performed != expected_cpu_fallback) {
          ++mismatch_count;
        }
      }
    }

    ExactHigherSupportProductCudaTask overlapping =
        qualified_tasks[2].task;
    overlapping.task_id = 1000U;
    const auto& first_group = overlapping.product.groups[0];
    overlapping.query_node = ExactHigherSupportNodeReceipt{
        first_group.node_index,
        first_group.leaf_begin,
        first_group.leaf_end};
    const auto overlap_result = context.evaluate(
        std::span<const ExactHigherSupportProductCudaTask>{
            &overlapping, 1U});
    const bool overlap_rejected_without_launch =
        overlap_result.status ==
            ExactHigherSupportProductCudaStatus::invalid_batch &&
        overlap_result.stop_reason ==
            ExactHigherSupportProductCudaStopReason::invalid_task &&
        overlap_result.records.empty() &&
        overlap_result.audit.launcher_call_count == 0U &&
        overlap_result.audit.kernel_launch_count == 0U &&
        overlap_result.audit.synchronization_count == 0U;

    const auto& audit = result.audit;
    const bool audit_qualified =
        result.status ==
            ExactHigherSupportProductCudaStatus::complete_exact_batch &&
        result.stop_reason == ExactHigherSupportProductCudaStopReason::none &&
        audit.submitted_task_count == tasks.size() &&
        audit.completed_task_count == tasks.size() &&
        audit.support_prune_task_count == expected_support_task_count &&
        audit.query_strict_interior_task_count ==
            expected_query_task_count &&
        audit.certified_count == expected_certified_count &&
        audit.fail_open_count == tasks.size() - expected_certified_count &&
        audit.bounded_dyadic_int512_count +
                audit.bounded_dyadic_int1024_count ==
            tasks.size() - expected_fallback_count &&
        audit.bounded_dyadic_int512_count != 0U &&
        audit.bounded_dyadic_int1024_count != 0U &&
        audit.arbitrary_precision_rational_fallback_count ==
            expected_fallback_count &&
        audit.launcher_call_count == 1U &&
        audit.kernel_launch_count == 2U &&
        audit.synchronization_count == 1U &&
        audit.kernel_elapsed_ns_available &&
        audit.narrow_int512_kernel_executed &&
        audit.source_snapshot_epoch == epoch &&
        audit.submitted_task_digest != 0U &&
        audit.completed_result_digest != 0U && audit.cuda_device >= 0 &&
        audit.source_authority_validated &&
        audit.resident_lease_index_identity_validated &&
        audit.every_task_validated_before_launch &&
        audit.every_result_validated_once &&
        audit.output_published_atomically &&
        !audit.floating_point_decision_performed &&
        !audit.bigint_support_mass_transferred &&
        !audit.host_fake_lifecycle_exercised &&
        audit.cuda_execution_performed &&
        audit.native_lbvh_nodes_read_on_device &&
        !audit.global_product_frontier_mutated &&
        !audit.ordinary_or_higher_order_delaunay_materialized &&
        !audit.global_cell_coface_or_incidence_arena_materialized &&
        !audit.hierarchy_or_tree_claimed && !audit.slo_claimed &&
        !audit.public_status_claimed;
    const bool qualified = mismatch_count == 0U && audit_qualified &&
        overlap_rejected_without_launch && !context.host_fake();
    const auto total_end = std::chrono::steady_clock::now();

    std::cout
        << "{\"backend\":\"cuda_g4\","
        << "\"bounded_int512_count\":"
        << audit.bounded_dyadic_int512_count << ','
        << "\"bounded_int1024_count\":"
        << audit.bounded_dyadic_int1024_count << ','
        << "\"certified_count\":" << audit.certified_count << ','
        << "\"completed_result_digest\":"
        << audit.completed_result_digest << ','
        << "\"cuda_device\":" << audit.cuda_device << ','
        << "\"cuda_execution_performed\":"
        << (audit.cuda_execution_performed ? "true" : "false") << ','
        << "\"every_result_validated_once\":"
        << (audit.every_result_validated_once ? "true" : "false") << ','
        << "\"every_task_validated_before_launch\":"
        << (audit.every_task_validated_before_launch ? "true" : "false")
        << ','
        << "\"fail_open_count\":" << audit.fail_open_count << ','
        << "\"fixture_family_count\":" << fixture_family_count << ','
        << "\"floating_point_decision_performed\":"
        << (audit.floating_point_decision_performed ? "true" : "false")
        << ','
        << "\"global_cell_coface_or_incidence_arena_materialized\":"
        << (audit.global_cell_coface_or_incidence_arena_materialized
                ? "true"
                : "false")
        << ','
        << "\"global_product_frontier_mutated\":"
        << (audit.global_product_frontier_mutated ? "true" : "false")
        << ','
        << "\"host_fake_lifecycle_exercised\":"
        << (audit.host_fake_lifecycle_exercised ? "true" : "false") << ','
        << "\"host_batch_total_ns\":"
        << nanoseconds_between(batch_begin, batch_end) << ','
        << "\"internal_node_receipt_leaf_count\":"
        << internal_receipt.leaf_end - internal_receipt.leaf_begin << ','
        << "\"internal_node_receipt_task_count\":"
        << internal_node_receipt_task_count << ','
        << "\"kernel_launch_count\":" << audit.kernel_launch_count << ','
        << "\"kernel_elapsed_ns\":" << audit.kernel_elapsed_ns << ','
        << "\"kernel_elapsed_ns_available\":"
        << (audit.kernel_elapsed_ns_available ? "true" : "false") << ','
        << "\"kernel_ns\":" << audit.kernel_elapsed_ns << ','
        << "\"kernel_ns_available\":"
        << (audit.kernel_elapsed_ns_available ? "true" : "false") << ','
        << "\"lbvh_build_total_ns\":"
        << nanoseconds_between(lbvh_begin, lbvh_end) << ','
        << "\"mismatch_count\":" << mismatch_count << ','
        << "\"mode\":\"batched_exact_support_prune_and_query_strict_"
           "interior\","
        << "\"native_lbvh_nodes_read_on_device\":"
        << (audit.native_lbvh_nodes_read_on_device ? "true" : "false")
        << ','
        << "\"narrow_int512_kernel_executed\":"
        << (audit.narrow_int512_kernel_executed ? "true" : "false")
        << ','
        << "\"ordinary_or_higher_order_delaunay_materialized\":"
        << (audit.ordinary_or_higher_order_delaunay_materialized
                ? "true"
                : "false")
        << ','
        << "\"overlap_rejected_without_launch\":"
        << (overlap_rejected_without_launch ? "true" : "false") << ','
        << "\"output_published_atomically\":"
        << (audit.output_published_atomically ? "true" : "false") << ','
        << "\"point_count\":" << cloud.size() << ','
        << "\"profile\":\"hgp_reduced\","
        << "\"qualified\":" << (qualified ? "true" : "false") << ','
        << "\"rational_fallback_count\":"
        << audit.arbitrary_precision_rational_fallback_count << ','
        << "\"resident_lease_index_identity_validated\":"
        << (audit.resident_lease_index_identity_validated
                ? "true"
                : "false")
        << ','
        << "\"schema_version\":"
        << morsehgp3d::gpu::
               exact_higher_support_product_cuda_schema_version
        << ",\"source_authority_validated\":"
        << (audit.source_authority_validated ? "true" : "false") << ','
        << "\"source_snapshot_epoch\":" << epoch << ','
        << "\"submitted_task_digest\":"
        << audit.submitted_task_digest << ','
        << "\"synchronization_count\":"
        << audit.synchronization_count << ','
        << "\"task_count\":" << tasks.size() << ','
        << "\"total_qualification_ns\":"
        << nanoseconds_between(total_begin, total_end) << "}\n";
    return qualified ? 0 : 1;
  } catch (const std::exception& error) {
    const auto total_end = std::chrono::steady_clock::now();
    std::cout
        << "{\"backend\":\"cuda_g4\",\"error\":\""
        << json_escape(error.what())
        << "\",\"kernel_elapsed_ns\":null,"
        << "\"kernel_elapsed_ns_available\":false,"
        << "\"kernel_ns\":null,\"kernel_ns_available\":false,"
        << "\"host_batch_total_ns\":null,"
        << "\"host_batch_total_ns_available\":false,"
        << "\"qualified\":false,\"schema_version\":"
        << morsehgp3d::gpu::
               exact_higher_support_product_cuda_schema_version
        << ','
        << "\"total_qualification_ns\":"
        << nanoseconds_between(total_begin, total_end) << "}\n";
    return 1;
  }
}
