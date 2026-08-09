#include "morsehgp3d/gpu/jung_center_cover_prune_mass.hpp"

#include "phase15_jung_center_cover_prune_mass_internal.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu::detail {

Phase15JungCenterCoverPruneMassAdoptedTraversal
Phase15JungCenterCoverPruneMassTraversalAccess::consume(
    MortonLbvhDeviceTraversalLease&& lease) {
  Phase15JungCenterCoverPruneMassAdoptedTraversal result;
  result.ready = lease.ready();
  result.host_fake = result.ready && !lease.cuda_resident();
  result.retained_owner = std::move(lease.retained_resources_);
  result.source_cloud_identity = std::move(lease.source_cloud_identity_);
  result.source_index_identity = std::move(lease.source_index_identity_);
  result.device_coordinate_bits = lease.device_coordinate_bits_;
  result.device_morton_point_ids = lease.device_morton_point_ids_;
  result.device_nodes = lease.device_nodes_;
  result.point_count = lease.audit_.point_count;
  result.certified_node_count = lease.audit_.certified_node_count;
  result.persistent_device_byte_capacity =
      lease.audit_.persistent_device_byte_capacity;
  result.source_snapshot_epoch = lease.audit_.source_snapshot_epoch;
  result.cuda_device = lease.cuda_device_;
  lease.audit_ = {};
  lease.device_coordinate_bits_ = nullptr;
  lease.device_morton_point_ids_ = nullptr;
  lease.device_nodes_ = nullptr;
  lease.cuda_device_ = -1;
  return result;
}

}  // namespace morsehgp3d::gpu::detail

namespace morsehgp3d::gpu {

JungCenterCoverPruneMassContext::JungCenterCoverPruneMassContext(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    JungCenterCoverPruneMassConfig config)
    : config_(config),
      state_(
          std::make_shared<detail::Phase15JungCenterCoverPruneMassContextState>()) {
  if (config_.requested_shard_count < 2U ||
      config_.endpoint_microtile_pair_mass == 0U ||
      config_.stack_capacity_per_shard <
          jung_center_cover_minimum_stack_capacity ||
      config_.requested_shard_count >
          static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) ||
      config_.stack_capacity_per_shard >
          static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
    throw std::invalid_argument("invalid Jung center-cover profiler config");
  }
  source_audit_ = traversal_lease.audit();
  auto adopted =
      detail::Phase15JungCenterCoverPruneMassTraversalAccess::consume(
          std::move(traversal_lease));
  if (!adopted.ready || adopted.point_count < 2U ||
      adopted.certified_node_count != 2U * adopted.point_count - 1U ||
      !adopted.source_cloud_identity || !adopted.source_index_identity ||
      !adopted.retained_owner) {
    throw std::invalid_argument(
        "the Jung center-cover profiler requires a ready certified traversal");
  }
  if (config_.collect_n32_qualification_receipts &&
      adopted.point_count > 32U) {
    throw std::invalid_argument(
        "Jung center-cover qualification receipts are restricted to n<=32");
  }
  source_owner_ = std::move(adopted.retained_owner);
  source_cloud_identity_ = std::move(adopted.source_cloud_identity);
  source_index_identity_ = std::move(adopted.source_index_identity);
  device_coordinate_bits_ = adopted.device_coordinate_bits;
  device_morton_point_ids_ = adopted.device_morton_point_ids;
  device_nodes_ = adopted.device_nodes;
  cuda_device_ = adopted.cuda_device;
  host_fake_ = adopted.host_fake;
}

JungCenterCoverPruneMassContext::~JungCenterCoverPruneMassContext() noexcept =
    default;
JungCenterCoverPruneMassContext::JungCenterCoverPruneMassContext(
    JungCenterCoverPruneMassContext&&) noexcept = default;
JungCenterCoverPruneMassContext&
JungCenterCoverPruneMassContext::operator=(
    JungCenterCoverPruneMassContext&&) noexcept = default;

bool JungCenterCoverPruneMassContext::ready() const noexcept {
  return state_ && !consumed_ && !state_->poisoned.load(std::memory_order_acquire);
}

bool JungCenterCoverPruneMassContext::poisoned() const noexcept {
  return !state_ || state_->poisoned.load(std::memory_order_acquire);
}

JungCenterCoverPruneMassResult
JungCenterCoverPruneMassContext::profile_once() {
  if (!state_) {
    throw std::logic_error("moved-from Jung center-cover profiler context");
  }
  std::lock_guard lock{state_->mutex};
  if (consumed_) {
    throw std::logic_error("the Jung center-cover profiler is single-use");
  }
  if (state_->poisoned.load(std::memory_order_acquire)) {
    throw std::logic_error("the Jung center-cover profiler context is poisoned");
  }
  consumed_ = true;
  detail::Phase15JungCenterCoverPruneMassRequest request;
  request.config = config_;
  request.traversal = detail::Phase15JungCenterCoverPruneMassAdoptedTraversal{
      source_owner_,
      source_cloud_identity_,
      source_index_identity_,
      device_coordinate_bits_,
      device_morton_point_ids_,
      device_nodes_,
      source_audit_.point_count,
      source_audit_.certified_node_count,
      source_audit_.persistent_device_byte_capacity,
      source_audit_.source_snapshot_epoch,
      cuda_device_,
      host_fake_,
      true};
  try {
    auto receipt =
        detail::launch_phase15_jung_center_cover_prune_mass(request);
    if (receipt.status != JungCenterCoverPruneMassStatus::complete) {
      state_->poisoned.store(true, std::memory_order_release);
    }
    return JungCenterCoverPruneMassResult{
        receipt.status,
        std::move(receipt.audit),
        std::move(receipt.qualification_receipts)};
  } catch (...) {
    state_->poisoned.store(true, std::memory_order_release);
    throw;
  }
}

}  // namespace morsehgp3d::gpu
