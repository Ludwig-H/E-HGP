#include "phase15_exact_higher_support_product_cuda_internal.hpp"
#include "phase15_exact_higher_support_product_fixed.cuh"
#include "phase15_exact_higher_support_product_fixed256.cuh"
#include "phase15_exact_higher_support_product_fixed512.cuh"

#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include <array>
#include <atomic>
#include <optional>
#include <span>
#include <stdexcept>

namespace morsehgp3d::gpu::test_support {
namespace {

std::atomic<bool> corrupt_next_receipt{false};
std::atomic<bool> throw_non_std_on_next_launch{false};
std::atomic<bool> force_next_int256_fallback{false};
std::atomic<bool> force_next_int512_fallback{false};
std::atomic<bool> force_false_positive_certificates{false};
std::atomic<bool> force_wrong_terminal_geometry_categories{false};
std::atomic<bool> corrupt_next_terminal_geometry_category{false};
std::atomic<std::size_t> forced_false_positive_count{0U};
std::atomic<std::size_t> launcher_call_count{0U};

}  // namespace

void corrupt_next_exact_higher_support_product_receipt() noexcept {
  corrupt_next_receipt.store(true, std::memory_order_relaxed);
}

void throw_non_std_on_next_exact_higher_support_product_launch() noexcept {
  throw_non_std_on_next_launch.store(true, std::memory_order_relaxed);
}

void force_next_exact_higher_support_product_int256_fallback() noexcept {
  force_next_int256_fallback.store(true, std::memory_order_relaxed);
}

void force_next_exact_higher_support_product_int512_fallback() noexcept {
  force_next_int512_fallback.store(true, std::memory_order_relaxed);
}

void force_exact_higher_support_product_false_positives(
    bool enabled) noexcept {
  force_false_positive_certificates.store(
      enabled, std::memory_order_relaxed);
}

void force_exact_higher_support_product_wrong_terminal_geometry_categories(
    bool enabled) noexcept {
  force_wrong_terminal_geometry_categories.store(
      enabled, std::memory_order_relaxed);
}

void corrupt_next_exact_higher_support_product_terminal_geometry_category()
    noexcept {
  corrupt_next_terminal_geometry_category.store(
      true, std::memory_order_relaxed);
}

std::size_t exact_higher_support_product_forced_false_positive_count()
    noexcept {
  return forced_false_positive_count.load(std::memory_order_relaxed);
}

void reset_exact_higher_support_product_fake() noexcept {
  corrupt_next_receipt.store(false, std::memory_order_relaxed);
  throw_non_std_on_next_launch.store(false, std::memory_order_relaxed);
  force_next_int256_fallback.store(false, std::memory_order_relaxed);
  force_next_int512_fallback.store(false, std::memory_order_relaxed);
  force_false_positive_certificates.store(false, std::memory_order_relaxed);
  force_wrong_terminal_geometry_categories.store(
      false, std::memory_order_relaxed);
  corrupt_next_terminal_geometry_category.store(
      false, std::memory_order_relaxed);
  forced_false_positive_count.store(0U, std::memory_order_relaxed);
  launcher_call_count.store(0U, std::memory_order_relaxed);
}

std::size_t exact_higher_support_product_fake_launcher_call_count() noexcept {
  return launcher_call_count.load(std::memory_order_relaxed);
}

[[nodiscard]] bool consume_receipt_corruption() noexcept {
  return corrupt_next_receipt.exchange(false, std::memory_order_relaxed);
}

[[nodiscard]] bool consume_non_std_launch_exception() noexcept {
  return throw_non_std_on_next_launch.exchange(
      false, std::memory_order_relaxed);
}

[[nodiscard]] bool consume_forced_int256_fallback() noexcept {
  return force_next_int256_fallback.exchange(
      false, std::memory_order_relaxed);
}

[[nodiscard]] bool consume_forced_int512_fallback() noexcept {
  return force_next_int512_fallback.exchange(
      false, std::memory_order_relaxed);
}

[[nodiscard]] bool false_positive_certificates_forced() noexcept {
  return force_false_positive_certificates.load(
      std::memory_order_relaxed);
}

[[nodiscard]] bool wrong_terminal_geometry_categories_forced() noexcept {
  return force_wrong_terminal_geometry_categories.load(
      std::memory_order_relaxed);
}

[[nodiscard]] bool consume_terminal_geometry_category_corruption()
    noexcept {
  return corrupt_next_terminal_geometry_category.exchange(
      false, std::memory_order_relaxed);
}

void count_forced_false_positive() noexcept {
  forced_false_positive_count.fetch_add(1U, std::memory_order_relaxed);
}

void count_launcher_call() noexcept {
  launcher_call_count.fetch_add(1U, std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

namespace fixed = exact_higher_support_product_fixed;
namespace fixed256 = exact_higher_support_product_fixed256;
namespace fixed512 = exact_higher_support_product_fixed512;

[[nodiscard]] spatial::ExactDyadicAabb3 exact_aabb(
    const Phase15ExactHigherSupportProductCudaRawAabb3& source) noexcept {
  spatial::ExactDyadicAabb3 result{};
  for (std::size_t axis = 0U;
       axis < phase15_exact_higher_support_product_axis_count;
       ++axis) {
    result.lower_binary64_bits[axis] = source.lower[axis];
    result.upper_binary64_bits[axis] = source.upper[axis];
  }
  return result;
}

[[nodiscard]] fixed::Binary64Aabb3 fixed_aabb(
    const Phase15ExactHigherSupportProductCudaRawAabb3& source) noexcept {
  fixed::Binary64Aabb3 result{};
  for (std::size_t axis = 0U;
       axis < phase15_exact_higher_support_product_axis_count;
       ++axis) {
    result.lower[axis] = source.lower[axis];
    result.upper[axis] = source.upper[axis];
  }
  return result;
}

[[nodiscard]] bool valid_arithmetic_width(
    const Phase15ExactHigherSupportProductCudaTask& task) noexcept {
  fixed::Binary64Aabb3 support[fixed::maximum_support_size]{};
  for (std::size_t index = 0U; index < task.support_size; ++index) {
    support[index] = fixed_aabb(task.support_boxes[index]);
  }
  const fixed::Binary64Aabb3 query_box = fixed_aabb(task.query_box);
  std::uint64_t width = 0U;
  if (!fixed::aligned_product_coordinate_bit_width(
          support,
          task.support_size,
          task.has_query_box != 0U ? &query_box : nullptr,
          width)) {
    return false;
  }
  const auto expected = fixed256::expression_fits(
                            task.support_size,
                            task.has_query_box != 0U,
                            width)
      ? Phase15ExactHigherSupportProductCudaArithmeticWidth::int256
      : width <= fixed512::aligned_coordinate_bit_limit
      ? Phase15ExactHigherSupportProductCudaArithmeticWidth::int512
      : Phase15ExactHigherSupportProductCudaArithmeticWidth::int1024;
  return task.aligned_coordinate_bit_width == width &&
      task.arithmetic_width == expected;
}

[[nodiscard]] bool valid_compact_task(
    const Phase15ExactHigherSupportProductCudaTask& task,
    const Phase15ExactHigherSupportProductCudaRequest& request) noexcept {
  if (task.source_snapshot_epoch != request.source_snapshot_epoch ||
      (task.support_size != 3U && task.support_size != 4U) ||
      task.support_group_count == 0U ||
      task.support_group_count > task.support_size ||
      task.support_group_count >
          phase15_exact_higher_support_product_maximum_support_size) {
    return false;
  }
  std::size_t expanded_size = 0U;
  std::uint64_t previous_leaf_end = 0U;
  for (std::size_t group_index = 0U;
       group_index < task.support_group_count;
       ++group_index) {
    const std::uint64_t begin = task.support_leaf_begins[group_index];
    const std::uint64_t end = task.support_leaf_ends[group_index];
    const std::uint8_t multiplicity =
        task.support_multiplicities[group_index];
    if (task.support_node_indices[group_index] >=
            request.certified_node_count ||
        begin >= end || end > request.point_count ||
        (group_index != 0U && begin < previous_leaf_end) ||
        multiplicity == 0U || multiplicity > end - begin ||
        expanded_size + multiplicity > task.support_size) {
      return false;
    }
    expanded_size += multiplicity;
    previous_leaf_end = end;
  }
  if (expanded_size != task.support_size) {
    return false;
  }

  if (task.kind == ExactHigherSupportProductCudaTaskKind::support_prune) {
    return !task.has_query_box && task.query_node_index == 0U &&
        task.query_leaf_begin == 0U && task.query_leaf_end == 0U &&
        valid_arithmetic_width(task);
  }
  if (task.kind ==
      ExactHigherSupportProductCudaTaskKind::terminal_support_geometry) {
    if (task.has_query_box || task.query_node_index != 0U ||
        task.query_leaf_begin != 0U || task.query_leaf_end != 0U ||
        task.support_group_count != task.support_size) {
      return false;
    }
    for (std::size_t group_index = 0U;
         group_index < task.support_group_count;
         ++group_index) {
      if (task.support_multiplicities[group_index] != 1U ||
          task.support_leaf_ends[group_index] !=
              task.support_leaf_begins[group_index] + 1U) {
        return false;
      }
    }
    return valid_arithmetic_width(task);
  }
  if (task.kind !=
          ExactHigherSupportProductCudaTaskKind::query_strict_interior ||
      !task.has_query_box ||
      task.query_node_index >= request.certified_node_count ||
      task.query_leaf_begin >= task.query_leaf_end ||
      task.query_leaf_end > request.point_count) {
    return false;
  }
  for (std::size_t group_index = 0U;
       group_index < task.support_group_count;
       ++group_index) {
    if (task.query_leaf_begin < task.support_leaf_ends[group_index] &&
        task.support_leaf_begins[group_index] < task.query_leaf_end) {
      return false;
    }
  }
  return valid_arithmetic_width(task);
}

}  // namespace

Phase15ExactHigherSupportProductCudaReceipt
phase15_launch_exact_higher_support_product_cuda(
    const Phase15ExactHigherSupportProductCudaRequest& request) {
  if (!request.host_fake || request.tasks.empty() ||
      request.tasks.size() > request.maximum_task_count ||
      request.point_count == 0U || request.certified_node_count == 0U ||
      request.source_snapshot_epoch == 0U ||
      !request.resident_lease_index_identity_validated ||
      request.device_coordinate_bits != nullptr ||
      request.device_nodes != nullptr || request.cuda_device != -1 ||
      request.submitted_task_digest !=
          phase15_exact_higher_support_product_task_digest(request.tasks)) {
    throw std::invalid_argument(
        "the fake exact higher-support product launcher rejected its batch");
  }
  test_support::count_launcher_call();
  if (test_support::consume_non_std_launch_exception()) {
    throw 17;
  }

  Phase15ExactHigherSupportProductCudaReceipt receipt;
  receipt.records.reserve(request.tasks.size());
  for (const auto& task : request.tasks) {
    if (!valid_compact_task(task, request)) {
      throw std::invalid_argument(
          "the fake exact higher-support product launcher received an "
          "unauthenticated compact task");
    }
    std::array<
        spatial::ExactDyadicAabb3,
        phase15_exact_higher_support_product_maximum_support_size>
        exact_support_boxes{};
    for (std::size_t support_index = 0U;
         support_index < task.support_size;
         ++support_index) {
      exact_support_boxes[support_index] =
          exact_aabb(task.support_boxes[support_index]);
    }
    const std::span<const spatial::ExactDyadicAabb3> support_boxes{
        exact_support_boxes.data(), task.support_size};
    hierarchy::ExactHigherSupportProductAabbDecisionBackend backend{};
    bool certified = false;
    std::optional<hierarchy::ExactHigherSupportTerminalGeometryDecision>
        terminal_geometry_decision;
    if (task.kind ==
        ExactHigherSupportProductCudaTaskKind::support_prune) {
      certified = hierarchy::
          exact_higher_support_product_no_well_centered_certified(
              support_boxes, &backend);
    } else if (
        task.kind == ExactHigherSupportProductCudaTaskKind::
            query_strict_interior && task.has_query_box) {
      certified = hierarchy::
          exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
              support_boxes, exact_aabb(task.query_box), &backend);
    } else if (
        task.kind == ExactHigherSupportProductCudaTaskKind::
            terminal_support_geometry) {
      terminal_geometry_decision = hierarchy::
          exact_higher_support_terminal_geometry_decision(
              support_boxes, &backend);
      certified = true;
    } else {
      throw std::invalid_argument(
          "the fake exact higher-support product launcher received an "
          "invalid task kind");
    }

    Phase15ExactHigherSupportProductCudaDeviceRecord record;
    record.task_id = task.task_id;
    record.kind = task.kind;
    if (terminal_geometry_decision.has_value()) {
      record.terminal_geometry_decision_present = 1U;
      record.terminal_geometry_decision = *terminal_geometry_decision;
      if (test_support::wrong_terminal_geometry_categories_forced()) {
        const std::uint8_t next = static_cast<std::uint8_t>(
            (static_cast<std::uint8_t>(record.terminal_geometry_decision) +
             1U) %
            4U);
        record.terminal_geometry_decision = static_cast<
            hierarchy::ExactHigherSupportTerminalGeometryDecision>(next);
      }
    }
    if (task.arithmetic_width ==
            Phase15ExactHigherSupportProductCudaArithmeticWidth::int256 &&
        test_support::consume_forced_int256_fallback()) {
      record.outcome = Phase15ExactHigherSupportProductCudaDeviceOutcome::
          requires_host_int512_fallback;
      record.backend = Phase15ExactHigherSupportProductCudaDeviceBackend::
          bounded_dyadic_int512;
    } else if (task.arithmetic_width ==
            Phase15ExactHigherSupportProductCudaArithmeticWidth::int512 &&
        test_support::consume_forced_int512_fallback()) {
      record.outcome = Phase15ExactHigherSupportProductCudaDeviceOutcome::
          requires_host_int1024_fallback;
      record.backend = Phase15ExactHigherSupportProductCudaDeviceBackend::
          bounded_dyadic_int1024;
    } else if (backend == hierarchy::
            ExactHigherSupportProductAabbDecisionBackend::
                arbitrary_precision_rational) {
      record.outcome = Phase15ExactHigherSupportProductCudaDeviceOutcome::
          requires_cpu_rational_fallback;
      record.backend = Phase15ExactHigherSupportProductCudaDeviceBackend::
          arbitrary_precision_rational;
    } else {
      if (test_support::false_positive_certificates_forced() &&
          !certified) {
        certified = true;
        test_support::count_forced_false_positive();
      }
      record.outcome = certified
          ? Phase15ExactHigherSupportProductCudaDeviceOutcome::certified
          : Phase15ExactHigherSupportProductCudaDeviceOutcome::exact_false;
      switch (task.arithmetic_width) {
        case Phase15ExactHigherSupportProductCudaArithmeticWidth::int256:
          record.backend = Phase15ExactHigherSupportProductCudaDeviceBackend::
              bounded_dyadic_int256;
          break;
        case Phase15ExactHigherSupportProductCudaArithmeticWidth::int512:
          record.backend = Phase15ExactHigherSupportProductCudaDeviceBackend::
              bounded_dyadic_int512;
          break;
        case Phase15ExactHigherSupportProductCudaArithmeticWidth::int1024:
          record.backend = Phase15ExactHigherSupportProductCudaDeviceBackend::
              bounded_dyadic_int1024;
          break;
        default:
          throw std::logic_error(
              "the fake exact higher-support product launcher lost its "
              "arithmetic route");
      }
    }
    if (terminal_geometry_decision.has_value() &&
        test_support::consume_terminal_geometry_category_corruption()) {
      record.terminal_geometry_decision = static_cast<
          hierarchy::ExactHigherSupportTerminalGeometryDecision>(255U);
    }
    receipt.records.push_back(record);
  }
  receipt.submitted_task_digest = request.submitted_task_digest;
  receipt.completed_result_digest =
      phase15_exact_higher_support_product_device_result_digest(
          receipt.records);
  receipt.source_identity_authenticated = true;
  receipt.resident_lease_index_identity_validated = true;
  receipt.kernel_elapsed_ns = 0U;
  receipt.kernel_elapsed_ns_available = false;
  receipt.every_task_classified_once = true;
  receipt.host_fake_lifecycle_exercised = true;
  receipt.narrow_int256_kernel_executed = false;
  receipt.narrow_int512_kernel_executed = false;
  if (test_support::consume_receipt_corruption()) {
    ++receipt.completed_result_digest;
  }
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail
