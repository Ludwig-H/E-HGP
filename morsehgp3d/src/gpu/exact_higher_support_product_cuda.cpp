#include "morsehgp3d/gpu/exact_higher_support_product_cuda.hpp"

#include "phase15_exact_higher_support_product_cuda_internal.hpp"
#include "phase15_exact_higher_support_product_fixed.cuh"
#include "phase15_exact_higher_support_product_fixed256.cuh"
#include "phase15_exact_higher_support_product_fixed512.cuh"

#include "morsehgp3d/hierarchy/higher_support_product.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

namespace fixed = detail::exact_higher_support_product_fixed;
namespace fixed256 = detail::exact_higher_support_product_fixed256;
namespace fixed512 = detail::exact_higher_support_product_fixed512;

[[nodiscard]] std::uint64_t public_result_digest(
    std::span<const ExactHigherSupportProductCudaRecord> records) noexcept {
  std::uint64_t digest =
      detail::phase15_exact_higher_support_product_digest_offset;
  digest = detail::phase15_exact_higher_support_product_digest_word(
      digest, static_cast<std::uint64_t>(records.size()));
  for (const auto& record : records) {
    digest = detail::phase15_exact_higher_support_product_digest_word(
        digest, record.task_id);
    digest = detail::phase15_exact_higher_support_product_digest_word(
        digest, static_cast<std::uint64_t>(record.kind));
    digest = detail::phase15_exact_higher_support_product_digest_word(
        digest, static_cast<std::uint64_t>(record.outcome));
    digest = detail::phase15_exact_higher_support_product_digest_word(
        digest, static_cast<std::uint64_t>(record.backend));
    digest = detail::phase15_exact_higher_support_product_digest_word(
        digest, record.cpu_fallback_performed ? UINT64_C(1) : UINT64_C(0));
  }
  return digest;
}

[[nodiscard]] bool fits_size_t(std::uint64_t value) noexcept {
  if constexpr (
      std::numeric_limits<std::size_t>::max() <
      std::numeric_limits<std::uint64_t>::max()) {
    return value <= std::numeric_limits<std::size_t>::max();
  }
  return true;
}

[[nodiscard]] detail::Phase15ExactHigherSupportProductCudaRawAabb3
raw_aabb(const spatial::ExactDyadicAabb3& source) noexcept {
  detail::Phase15ExactHigherSupportProductCudaRawAabb3 result{};
  for (std::size_t axis = 0U;
       axis < detail::phase15_exact_higher_support_product_axis_count;
       ++axis) {
    result.lower[axis] = source.lower_binary64_bits[axis];
    result.upper[axis] = source.upper_binary64_bits[axis];
  }
  return result;
}

[[nodiscard]] spatial::ExactDyadicAabb3 exact_aabb(
    const detail::Phase15ExactHigherSupportProductCudaRawAabb3& source)
    noexcept {
  spatial::ExactDyadicAabb3 result{};
  for (std::size_t axis = 0U;
       axis < detail::phase15_exact_higher_support_product_axis_count;
       ++axis) {
    result.lower_binary64_bits[axis] = source.lower[axis];
    result.upper_binary64_bits[axis] = source.upper[axis];
  }
  return result;
}

[[nodiscard]] fixed::Binary64Aabb3 fixed_aabb(
    const detail::Phase15ExactHigherSupportProductCudaRawAabb3& source)
    noexcept {
  fixed::Binary64Aabb3 result{};
  for (std::size_t axis = 0U;
       axis < detail::phase15_exact_higher_support_product_axis_count;
       ++axis) {
    result.lower[axis] = source.lower[axis];
    result.upper[axis] = source.upper[axis];
  }
  return result;
}

void select_arithmetic_width(
    detail::Phase15ExactHigherSupportProductCudaTask& task) {
  fixed::Binary64Aabb3 support_boxes[fixed::maximum_support_size]{};
  for (std::size_t index = 0U; index < task.support_size; ++index) {
    support_boxes[index] = fixed_aabb(task.support_boxes[index]);
  }
  const fixed::Binary64Aabb3 query_box = fixed_aabb(task.query_box);
  const fixed::Binary64Aabb3* query = task.has_query_box != 0U
      ? &query_box
      : nullptr;
  std::uint64_t coordinate_width = 0U;
  if (!fixed::aligned_product_coordinate_bit_width(
          support_boxes,
          task.support_size,
          query,
          coordinate_width)) {
    throw std::logic_error(
        "the exact higher-support product width preflight rejected "
        "host-certified finite AABBs");
  }
  task.aligned_coordinate_bit_width = coordinate_width;
  if (fixed256::expression_fits(
          task.support_size,
          task.has_query_box != 0U,
          coordinate_width)) {
    task.arithmetic_width =
        detail::Phase15ExactHigherSupportProductCudaArithmeticWidth::int256;
  } else if (coordinate_width <= fixed512::aligned_coordinate_bit_limit) {
    task.arithmetic_width =
        detail::Phase15ExactHigherSupportProductCudaArithmeticWidth::int512;
  } else {
    task.arithmetic_width =
        detail::Phase15ExactHigherSupportProductCudaArithmeticWidth::int1024;
  }
}

}  // namespace

struct ExactHigherSupportProductCudaContext::Impl {
  const spatial::MortonLbvhIndex* index{};
  const spatial::CanonicalPointCloud* cloud{};
  std::shared_ptr<MortonLbvhDeviceTraversalLease> traversal_owner;
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t maximum_task_count{};
  std::uint64_t source_snapshot_epoch{};
  const std::uint64_t* device_coordinate_bits{};
  const void* device_nodes{};
  int cuda_device{-1};
  bool resident_lease_index_identity_validated{false};
  bool host_fake{false};
  bool poisoned{false};
  std::mutex mutex;
};

ExactHigherSupportProductCudaContext::
ExactHigherSupportProductCudaContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    std::size_t maximum_task_count) {
  if (maximum_task_count == 0U) {
    throw std::invalid_argument(
        "an exact higher-support product context requires positive task "
        "capacity");
  }
  if (!index.validated_for(cloud) || !traversal_lease.ready()) {
    throw std::invalid_argument(
        "an exact higher-support product context requires matching ready "
        "LBVH authorities");
  }
  const MortonLbvhDeviceTraversalLeaseAudit audit =
      traversal_lease.audit();
  const std::size_t point_count = cloud.size();
  const std::size_t node_count = index.nodes_.size();
  const bool coordinate_extent_fits =
      point_count <= std::numeric_limits<std::size_t>::max() / 3U;
  if (point_count == 0U || node_count == 0U ||
      index.build_counters().point_count != point_count ||
      index.build_counters().node_count != node_count ||
      audit.point_count != point_count ||
      audit.certified_node_count != node_count ||
      audit.maximum_point_count < point_count ||
      audit.maximum_node_count < node_count || !coordinate_extent_fits ||
      audit.retained_coordinate_word_capacity < 3U * point_count ||
      audit.retained_morton_point_id_capacity < point_count ||
      audit.retained_node_capacity < node_count ||
      audit.source_snapshot_epoch == 0U ||
      !audit.source_snapshot_import_certified ||
      !audit.canonical_coordinate_words_retained ||
      !audit.active_morton_point_ids_retained ||
      !audit.certified_device_nodes_retained ||
      !audit.source_index_identity_retained ||
      !audit.builder_transients_released ||
      audit.retained_host_snapshot_byte_count != 0U ||
      audit.second_host_snapshot_retained ||
      audit.higher_order_delaunay_mosaic_materialized ||
      audit.global_cell_or_coface_arena_materialized ||
      audit.public_status_claimed ||
      !traversal_lease.source_cloud_identity_ ||
      traversal_lease.source_cloud_identity_.get() !=
          static_cast<const void*>(cloud.identity_.get()) ||
      !traversal_lease.source_index_identity_ ||
      traversal_lease.source_index_identity_.get() !=
          static_cast<const void*>(index.identity_.get()) ||
      !traversal_lease.retained_resources_) {
    throw std::invalid_argument(
        "the exact higher-support product context rejected an "
        "unauthenticated traversal lease");
  }

  const bool fake_shape = audit.host_fake_lifecycle_exercised &&
      !audit.cuda_device_storage_retained &&
      !traversal_lease.cuda_resident() &&
      traversal_lease.device_coordinate_bits_ == nullptr &&
      traversal_lease.device_morton_point_ids_ == nullptr &&
      traversal_lease.device_nodes_ == nullptr &&
      traversal_lease.cuda_device_ == -1;
  const bool cuda_shape = !audit.host_fake_lifecycle_exercised &&
      audit.cuda_device_storage_retained && traversal_lease.cuda_resident() &&
      traversal_lease.device_coordinate_bits_ != nullptr &&
      traversal_lease.device_morton_point_ids_ != nullptr &&
      traversal_lease.device_nodes_ != nullptr &&
      traversal_lease.cuda_device_ >= 0;
  if (!fake_shape && !cuda_shape) {
    throw std::invalid_argument(
        "the exact higher-support product context rejected a mixed "
        "host/CUDA lease shape");
  }

  auto owner = std::make_shared<MortonLbvhDeviceTraversalLease>(
      std::move(traversal_lease));
  auto impl = std::make_unique<Impl>();
  impl->index = &index;
  impl->cloud = &cloud;
  impl->traversal_owner = std::move(owner);
  impl->point_count = point_count;
  impl->certified_node_count = node_count;
  impl->maximum_task_count = maximum_task_count;
  impl->source_snapshot_epoch = audit.source_snapshot_epoch;
  impl->device_coordinate_bits =
      impl->traversal_owner->device_coordinate_bits_;
  impl->device_nodes = impl->traversal_owner->device_nodes_;
  impl->cuda_device = impl->traversal_owner->cuda_device_;
  impl->resident_lease_index_identity_validated = true;
  impl->host_fake = fake_shape;
  impl_ = std::move(impl);
}

ExactHigherSupportProductCudaContext::
~ExactHigherSupportProductCudaContext() noexcept = default;

ExactHigherSupportProductCudaContext::
ExactHigherSupportProductCudaContext(
    ExactHigherSupportProductCudaContext&&) noexcept = default;

ExactHigherSupportProductCudaResult
ExactHigherSupportProductCudaContext::evaluate(
    std::span<const ExactHigherSupportProductCudaTask> tasks) {
  if (!impl_) {
    throw std::logic_error(
        "a moved-from exact higher-support product context cannot evaluate");
  }
  std::lock_guard lock{impl_->mutex};
  if (impl_->poisoned) {
    throw std::logic_error(
        "a poisoned exact higher-support product context cannot evaluate");
  }
  if (!impl_->index->validated_for(*impl_->cloud) ||
      !impl_->traversal_owner || !impl_->traversal_owner->ready() ||
      !impl_->traversal_owner->source_index_identity_ ||
      impl_->traversal_owner->source_index_identity_.get() !=
          static_cast<const void*>(impl_->index->identity_.get())) {
    impl_->poisoned = true;
    throw std::logic_error(
        "the exact higher-support product context lost source authority");
  }

  ExactHigherSupportProductCudaAudit base_audit;
  base_audit.point_count = impl_->point_count;
  base_audit.certified_node_count = impl_->certified_node_count;
  base_audit.maximum_task_count = impl_->maximum_task_count;
  base_audit.submitted_task_count = tasks.size();
  base_audit.source_snapshot_epoch = impl_->source_snapshot_epoch;
  base_audit.cuda_device = impl_->cuda_device;
  base_audit.source_authority_validated = true;
  base_audit.resident_lease_index_identity_validated =
      impl_->resident_lease_index_identity_validated;
  base_audit.host_fake_lifecycle_exercised = impl_->host_fake;

  if (tasks.size() > impl_->maximum_task_count) {
    ExactHigherSupportProductCudaResult result;
    result.status =
        ExactHigherSupportProductCudaStatus::capacity_exhausted;
    result.stop_reason =
        ExactHigherSupportProductCudaStopReason::task_capacity;
    result.audit = base_audit;
    return result;
  }
  if (tasks.empty()) {
    ExactHigherSupportProductCudaResult result;
    result.status = ExactHigherSupportProductCudaStatus::invalid_batch;
    result.stop_reason = ExactHigherSupportProductCudaStopReason::invalid_task;
    result.audit = base_audit;
    return result;
  }

  const auto node_matches_receipt =
      [this](const hierarchy::ExactHigherSupportNodeReceipt& receipt) {
        if (!fits_size_t(receipt.node_index) ||
            !fits_size_t(receipt.leaf_begin) ||
            !fits_size_t(receipt.leaf_end)) {
          return false;
        }
        const std::size_t node_index =
            static_cast<std::size_t>(receipt.node_index);
        if (node_index >= impl_->index->nodes_.size()) {
          return false;
        }
        const auto& node = impl_->index->nodes_[node_index];
        return receipt.leaf_begin == node.leaf_begin &&
            receipt.leaf_end == node.leaf_end &&
            node.leaf_begin < node.leaf_end &&
            node.leaf_end <= impl_->point_count;
      };

  const auto node_box = [this](std::uint64_t raw_node_index) {
    const auto& node = impl_->index->nodes_[
        static_cast<std::size_t>(raw_node_index)];
    spatial::ExactDyadicAabb3 box{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      box.lower_binary64_bits[axis] =
          impl_->cloud->point(node.lower_point_ids[axis])
              .canonical_input_bits()[axis];
      box.upper_binary64_bits[axis] =
          impl_->cloud->point(node.upper_point_ids[axis])
              .canonical_input_bits()[axis];
    }
    return box;
  };

  std::vector<detail::Phase15ExactHigherSupportProductCudaTask> prepared;
  prepared.reserve(tasks.size());
  bool valid = true;
  std::uint64_t previous_task_id{};
  bool have_previous_task_id = false;
  for (const auto& task : tasks) {
    if (task.source_snapshot_epoch != impl_->source_snapshot_epoch ||
        (have_previous_task_id && task.task_id <= previous_task_id) ||
        (task.product.support_size != 3U &&
         task.product.support_size != 4U) ||
        task.product.group_count == 0U ||
        task.product.group_count > task.product.support_size ||
        task.product.group_count > task.product.groups.size()) {
      valid = false;
      break;
    }
    previous_task_id = task.task_id;
    have_previous_task_id = true;

    detail::Phase15ExactHigherSupportProductCudaTask compact;
    compact.task_id = task.task_id;
    compact.source_snapshot_epoch = task.source_snapshot_epoch;
    compact.kind = task.kind;
    compact.support_size = task.product.support_size;
    compact.support_group_count = task.product.group_count;
    std::size_t expanded_index = 0U;
    std::uint64_t previous_leaf_end{};
    for (std::size_t group_index = 0U;
         group_index < task.product.group_count;
         ++group_index) {
      const auto& group = task.product.groups[group_index];
      const hierarchy::ExactHigherSupportNodeReceipt receipt{
          group.node_index, group.leaf_begin, group.leaf_end};
      const std::uint64_t leaf_count =
          group.leaf_end >= group.leaf_begin
          ? group.leaf_end - group.leaf_begin
          : 0U;
      if (!node_matches_receipt(receipt) || group.multiplicity == 0U ||
          group.multiplicity > leaf_count ||
          (group_index != 0U && group.leaf_begin < previous_leaf_end) ||
          expanded_index + group.multiplicity >
              task.product.support_size) {
        valid = false;
        break;
      }
      previous_leaf_end = group.leaf_end;
      compact.support_node_indices[group_index] = group.node_index;
      compact.support_leaf_begins[group_index] = group.leaf_begin;
      compact.support_leaf_ends[group_index] = group.leaf_end;
      compact.support_multiplicities[group_index] = group.multiplicity;
      const spatial::ExactDyadicAabb3 bounds = node_box(group.node_index);
      for (std::size_t copy = 0U; copy < group.multiplicity; ++copy) {
        compact.support_boxes[expanded_index++] = raw_aabb(bounds);
      }
    }
    if (!valid || expanded_index != task.product.support_size) {
      valid = false;
      break;
    }
    for (std::size_t group_index = task.product.group_count;
         group_index < task.product.groups.size();
         ++group_index) {
      if (task.product.groups[group_index] !=
          hierarchy::ExactHigherSupportNodeGroup{}) {
        valid = false;
        break;
      }
    }
    if (!valid) {
      break;
    }

    switch (task.kind) {
      case ExactHigherSupportProductCudaTaskKind::support_prune:
        if (task.query_node.has_value()) {
          valid = false;
        }
        ++base_audit.support_prune_task_count;
        break;
      case ExactHigherSupportProductCudaTaskKind::query_strict_interior:
        if (!task.query_node.has_value() ||
            !node_matches_receipt(*task.query_node)) {
          valid = false;
        } else {
          for (std::size_t group_index = 0U;
               group_index < task.product.group_count;
               ++group_index) {
            const auto& group = task.product.groups[group_index];
            if (task.query_node->leaf_begin < group.leaf_end &&
                group.leaf_begin < task.query_node->leaf_end) {
              valid = false;
              break;
            }
          }
        }
        if (valid) {
          compact.query_node_index = task.query_node->node_index;
          compact.query_leaf_begin = task.query_node->leaf_begin;
          compact.query_leaf_end = task.query_node->leaf_end;
          compact.query_box = raw_aabb(
              node_box(task.query_node->node_index));
          compact.has_query_box = 1U;
        }
        ++base_audit.query_strict_interior_task_count;
        break;
      default:
        valid = false;
        break;
    }
    if (!valid) {
      break;
    }
    select_arithmetic_width(compact);
    prepared.push_back(compact);
  }

  if (!valid || prepared.size() != tasks.size()) {
    ExactHigherSupportProductCudaResult result;
    result.status = ExactHigherSupportProductCudaStatus::invalid_batch;
    result.stop_reason = ExactHigherSupportProductCudaStopReason::invalid_task;
    result.audit = base_audit;
    return result;
  }

  base_audit.every_task_validated_before_launch = true;
  const std::size_t prepared_int256_count = static_cast<std::size_t>(
      std::count_if(
          prepared.begin(),
          prepared.end(),
          [](const auto& task) {
            return task.arithmetic_width == detail::
                Phase15ExactHigherSupportProductCudaArithmeticWidth::int256;
          }));
  const std::size_t prepared_int512_count = static_cast<std::size_t>(
      std::count_if(
          prepared.begin(),
          prepared.end(),
          [](const auto& task) {
            return task.arithmetic_width == detail::
                Phase15ExactHigherSupportProductCudaArithmeticWidth::int512;
          }));
  const std::size_t prepared_int1024_count =
      prepared.size() - prepared_int256_count - prepared_int512_count;
  const std::size_t expected_cuda_kernel_count =
      (prepared_int256_count != 0U ? 1U : 0U) +
      (prepared_int512_count != 0U ? 1U : 0U) +
      (prepared_int1024_count != 0U ? 1U : 0U);
  base_audit.submitted_task_digest =
      detail::phase15_exact_higher_support_product_task_digest(prepared);
  const detail::Phase15ExactHigherSupportProductCudaRequest request{
      prepared,
      impl_->point_count,
      impl_->certified_node_count,
      impl_->maximum_task_count,
      impl_->source_snapshot_epoch,
      base_audit.submitted_task_digest,
      impl_->device_coordinate_bits,
      impl_->device_nodes,
      impl_->cuda_device,
      impl_->resident_lease_index_identity_validated,
      impl_->host_fake};

  try {
    detail::Phase15ExactHigherSupportProductCudaReceipt receipt =
        detail::phase15_launch_exact_higher_support_product_cuda(request);
    if (!receipt.source_identity_authenticated ||
        !receipt.resident_lease_index_identity_validated ||
        !receipt.every_task_classified_once ||
        receipt.floating_point_decision_performed ||
        receipt.bigint_support_mass_transferred ||
        receipt.forbidden_intermediate_materialized ||
        receipt.records.size() != prepared.size() ||
        receipt.submitted_task_digest !=
            base_audit.submitted_task_digest ||
        receipt.completed_result_digest !=
            detail::phase15_exact_higher_support_product_device_result_digest(
                receipt.records)) {
      throw std::logic_error(
          "the exact higher-support product launcher returned a malformed "
          "atomic receipt");
    }
    if (impl_->host_fake) {
      if (!receipt.host_fake_lifecycle_exercised ||
          receipt.cuda_execution_performed ||
          receipt.native_lbvh_nodes_read_on_device ||
          receipt.kernel_launch_count != 0U ||
          receipt.synchronization_count != 0U ||
          receipt.kernel_elapsed_ns_available ||
          receipt.kernel_elapsed_ns != 0U ||
          receipt.narrow_int256_kernel_executed ||
          receipt.narrow_int512_kernel_executed ||
          receipt.cuda_device != -1) {
        throw std::logic_error(
            "the exact higher-support product fake claimed CUDA execution");
      }
    } else if (
        receipt.host_fake_lifecycle_exercised ||
        !receipt.cuda_execution_performed ||
        !receipt.native_lbvh_nodes_read_on_device ||
        receipt.kernel_launch_count != expected_cuda_kernel_count ||
        receipt.synchronization_count != 1U ||
        !receipt.kernel_elapsed_ns_available ||
        receipt.narrow_int256_kernel_executed !=
            (prepared_int256_count != 0U) ||
        receipt.narrow_int512_kernel_executed !=
            (prepared_int512_count != 0U) ||
        receipt.cuda_device != impl_->cuda_device) {
      throw std::logic_error(
          "the exact higher-support product launcher lost CUDA authority");
    }

    std::vector<ExactHigherSupportProductCudaRecord> staged_records;
    staged_records.reserve(tasks.size());
    for (std::size_t index = 0U; index < tasks.size(); ++index) {
      const auto& device_record = receipt.records[index];
      const auto& prepared_task = prepared[index];
      if (device_record.task_id != prepared_task.task_id ||
          device_record.kind != prepared_task.kind) {
        throw std::logic_error(
            "the exact higher-support product launcher permuted its batch");
      }
      ExactHigherSupportProductCudaRecord record;
      record.task_id = device_record.task_id;
      record.kind = device_record.kind;
      const auto bounded_backend = [&]() {
        detail::Phase15ExactHigherSupportProductCudaDeviceBackend
            expected_backend{};
        switch (prepared_task.arithmetic_width) {
          case detail::
              Phase15ExactHigherSupportProductCudaArithmeticWidth::int256:
            expected_backend = detail::
                Phase15ExactHigherSupportProductCudaDeviceBackend::
                    bounded_dyadic_int256;
            break;
          case detail::
              Phase15ExactHigherSupportProductCudaArithmeticWidth::int512:
            expected_backend = detail::
                Phase15ExactHigherSupportProductCudaDeviceBackend::
                    bounded_dyadic_int512;
            break;
          case detail::
              Phase15ExactHigherSupportProductCudaArithmeticWidth::int1024:
            expected_backend = detail::
                Phase15ExactHigherSupportProductCudaDeviceBackend::
                    bounded_dyadic_int1024;
            break;
          default:
            throw std::logic_error(
                "the exact higher-support product host selected an unknown "
                "arithmetic route");
        }
        if (device_record.backend != expected_backend) {
          throw std::logic_error(
              "the exact higher-support product launcher changed the "
              "preflight arithmetic route");
        }
        switch (device_record.backend) {
          case detail::Phase15ExactHigherSupportProductCudaDeviceBackend::
              bounded_dyadic_int256:
            ++base_audit.bounded_dyadic_int256_count;
            return ExactHigherSupportProductCudaBackend::
                bounded_dyadic_int256;
          case detail::Phase15ExactHigherSupportProductCudaDeviceBackend::
              bounded_dyadic_int512:
            ++base_audit.bounded_dyadic_int512_count;
            return ExactHigherSupportProductCudaBackend::
                bounded_dyadic_int512;
          case detail::Phase15ExactHigherSupportProductCudaDeviceBackend::
              bounded_dyadic_int1024:
            ++base_audit.bounded_dyadic_int1024_count;
            return ExactHigherSupportProductCudaBackend::
                bounded_dyadic_int1024;
          default:
            throw std::logic_error(
                "the exact higher-support product launcher attached a "
                "non-integral backend to a bounded result");
        }
      };
      switch (device_record.outcome) {
        case detail::Phase15ExactHigherSupportProductCudaDeviceOutcome::
            certified:
          record.outcome = ExactHigherSupportProductCudaOutcome::certified;
          record.backend = bounded_backend();
          break;
        case detail::Phase15ExactHigherSupportProductCudaDeviceOutcome::
            exact_false:
          record.outcome = ExactHigherSupportProductCudaOutcome::fail_open;
          record.backend = bounded_backend();
          break;
        case detail::Phase15ExactHigherSupportProductCudaDeviceOutcome::
            requires_cpu_rational_fallback:
        case detail::Phase15ExactHigherSupportProductCudaDeviceOutcome::
            requires_host_int1024_fallback:
        case detail::Phase15ExactHigherSupportProductCudaDeviceOutcome::
            requires_host_int512_fallback: {
          const bool int512_required = device_record.outcome == detail::
              Phase15ExactHigherSupportProductCudaDeviceOutcome::
                  requires_host_int512_fallback;
          const bool int1024_required = device_record.outcome == detail::
              Phase15ExactHigherSupportProductCudaDeviceOutcome::
                  requires_host_int1024_fallback;
          const bool rational_required = device_record.outcome == detail::
              Phase15ExactHigherSupportProductCudaDeviceOutcome::
                  requires_cpu_rational_fallback;
          const bool route_matches =
              (int512_required && prepared_task.arithmetic_width == detail::
                      Phase15ExactHigherSupportProductCudaArithmeticWidth::
                          int256) ||
              (int1024_required && prepared_task.arithmetic_width == detail::
                      Phase15ExactHigherSupportProductCudaArithmeticWidth::
                          int512) ||
              (rational_required && prepared_task.arithmetic_width == detail::
                      Phase15ExactHigherSupportProductCudaArithmeticWidth::
                          int1024);
          if (!route_matches) {
            throw std::logic_error(
                "the exact higher-support product fallback escaped its "
                "preflight arithmetic route");
          }
          const auto expected_device_backend = int512_required
              ? detail::Phase15ExactHigherSupportProductCudaDeviceBackend::
                    bounded_dyadic_int512
              : int1024_required
              ? detail::Phase15ExactHigherSupportProductCudaDeviceBackend::
                    bounded_dyadic_int1024
              : detail::Phase15ExactHigherSupportProductCudaDeviceBackend::
                    arbitrary_precision_rational;
          if (device_record.backend != expected_device_backend) {
            throw std::logic_error(
                "the exact higher-support product fallback carried the "
                "wrong backend sentinel");
          }
          hierarchy::ExactHigherSupportProductAabbDecisionBackend backend{};
          std::array<
              spatial::ExactDyadicAabb3,
              detail::phase15_exact_higher_support_product_maximum_support_size>
              exact_support_boxes{};
          for (std::size_t support_index = 0U;
               support_index < prepared_task.support_size;
               ++support_index) {
            exact_support_boxes[support_index] =
                exact_aabb(prepared_task.support_boxes[support_index]);
          }
          const std::span<const spatial::ExactDyadicAabb3> support_boxes{
              exact_support_boxes.data(), prepared_task.support_size};
          bool certified = false;
          bool resolved_by_int512 = false;
          if (int512_required) {
            fixed::Binary64Aabb3 fixed_support_boxes[
                fixed::maximum_support_size]{};
            for (std::size_t support_index = 0U;
                 support_index < prepared_task.support_size;
                 ++support_index) {
              fixed_support_boxes[support_index] =
                  fixed_aabb(prepared_task.support_boxes[support_index]);
            }
            const fixed512::Decision decision = prepared_task.kind ==
                    ExactHigherSupportProductCudaTaskKind::support_prune
                ? fixed512::no_well_centered_support(
                      fixed_support_boxes, prepared_task.support_size)
                : fixed512::
                      query_strictly_inside_every_independent_sphere(
                          fixed_support_boxes,
                          prepared_task.support_size,
                          fixed_aabb(prepared_task.query_box));
            if (decision !=
                fixed512::Decision::requires_cpu_rational_fallback) {
              certified = decision == fixed512::Decision::certified;
              resolved_by_int512 = true;
            }
          }
          if (!resolved_by_int512) {
            if (prepared_task.kind ==
                ExactHigherSupportProductCudaTaskKind::support_prune) {
              certified = hierarchy::
                  exact_higher_support_product_no_well_centered_certified(
                      support_boxes, &backend);
            } else {
              certified = hierarchy::
                  exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
                      support_boxes,
                      exact_aabb(prepared_task.query_box),
                      &backend);
            }
          }
          if (!resolved_by_int512 && rational_required &&
              backend != hierarchy::
                  ExactHigherSupportProductAabbDecisionBackend::
                      arbitrary_precision_rational) {
            throw std::logic_error(
                "the exact higher-support product launcher requested an "
                "unnecessary rational fallback");
          }
          record.outcome = certified
              ? ExactHigherSupportProductCudaOutcome::certified
              : ExactHigherSupportProductCudaOutcome::fail_open;
          if (resolved_by_int512) {
            record.backend = ExactHigherSupportProductCudaBackend::
                bounded_dyadic_int512;
            ++base_audit.bounded_dyadic_int512_count;
          } else if (backend == hierarchy::
                  ExactHigherSupportProductAabbDecisionBackend::
                      arbitrary_precision_rational) {
            record.backend = ExactHigherSupportProductCudaBackend::
                arbitrary_precision_rational;
            ++base_audit.arbitrary_precision_rational_fallback_count;
          } else {
            record.backend = ExactHigherSupportProductCudaBackend::
                bounded_dyadic_int1024;
            ++base_audit.bounded_dyadic_int1024_count;
          }
          record.cpu_fallback_performed = true;
          break;
        }
        default:
          throw std::logic_error(
              "the exact higher-support product launcher returned an "
              "unknown outcome");
      }
      if (record.outcome ==
          ExactHigherSupportProductCudaOutcome::certified) {
        ++base_audit.certified_count;
      } else {
        ++base_audit.fail_open_count;
      }
      staged_records.push_back(record);
    }

    if (base_audit.bounded_dyadic_int256_count +
                base_audit.bounded_dyadic_int512_count +
                base_audit.bounded_dyadic_int1024_count +
                base_audit.arbitrary_precision_rational_fallback_count !=
            tasks.size() ||
        base_audit.certified_count + base_audit.fail_open_count !=
            tasks.size()) {
      throw std::logic_error(
          "the exact higher-support product result partition is incomplete");
    }
    base_audit.completed_task_count = tasks.size();
    base_audit.launcher_call_count = 1U;
    base_audit.kernel_launch_count = receipt.kernel_launch_count;
    base_audit.synchronization_count = receipt.synchronization_count;
    base_audit.kernel_elapsed_ns = receipt.kernel_elapsed_ns;
    base_audit.kernel_elapsed_ns_available =
        receipt.kernel_elapsed_ns_available;
    base_audit.completed_result_digest =
        public_result_digest(staged_records);
    base_audit.every_result_validated_once = true;
    base_audit.output_published_atomically = true;
    base_audit.cuda_execution_performed =
        receipt.cuda_execution_performed;
    base_audit.native_lbvh_nodes_read_on_device =
        receipt.native_lbvh_nodes_read_on_device;
    base_audit.narrow_int256_kernel_executed =
        receipt.narrow_int256_kernel_executed;
    base_audit.narrow_int512_kernel_executed =
        receipt.narrow_int512_kernel_executed;

    ExactHigherSupportProductCudaResult result;
    result.status =
        ExactHigherSupportProductCudaStatus::complete_exact_batch;
    result.stop_reason = ExactHigherSupportProductCudaStopReason::none;
    result.audit = base_audit;
    result.records = std::move(staged_records);
    return result;
  } catch (...) {
    impl_->poisoned = true;
    throw;
  }
}

std::size_t ExactHigherSupportProductCudaContext::maximum_task_count()
    const noexcept {
  return impl_ ? impl_->maximum_task_count : 0U;
}

std::uint64_t ExactHigherSupportProductCudaContext::source_snapshot_epoch()
    const noexcept {
  return impl_ ? impl_->source_snapshot_epoch : 0U;
}

bool ExactHigherSupportProductCudaContext::host_fake() const noexcept {
  return impl_ && impl_->host_fake;
}

bool ExactHigherSupportProductCudaContext::bound_to(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud) const noexcept {
  return impl_ != nullptr && impl_->index == &index &&
      impl_->cloud == &cloud && index.validated_for(cloud);
}

}  // namespace morsehgp3d::gpu
