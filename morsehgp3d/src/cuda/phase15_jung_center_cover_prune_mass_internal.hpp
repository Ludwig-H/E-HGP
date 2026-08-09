#pragma once

#include "morsehgp3d/gpu/jung_center_cover_prune_mass.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

namespace morsehgp3d::gpu::detail {

struct Phase15JungCenterCoverPruneMassAdoptedTraversal final {
  std::shared_ptr<void> retained_owner;
  std::shared_ptr<const void> source_cloud_identity;
  std::shared_ptr<const void> source_index_identity;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  std::size_t point_count{};
  std::size_t certified_node_count{};
  std::size_t persistent_device_byte_capacity{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  bool host_fake{false};
  bool ready{false};
};

class Phase15JungCenterCoverPruneMassTraversalAccess final {
 public:
  [[nodiscard]] static Phase15JungCenterCoverPruneMassAdoptedTraversal consume(
      MortonLbvhDeviceTraversalLease&& lease);
};

struct Phase15JungCenterCoverPruneMassRequest final {
  Phase15JungCenterCoverPruneMassAdoptedTraversal traversal;
  JungCenterCoverPruneMassConfig config{};
};

struct Phase15JungCenterCoverPruneMassReceipt final {
  JungCenterCoverPruneMassStatus status{
      JungCenterCoverPruneMassStatus::execution_failure};
  JungCenterCoverPruneMassAudit audit{};
  std::vector<JungCenterCoverQualificationReceipt> qualification_receipts;
};

class Phase15JungCenterCoverPruneMassContextState final {
 public:
  std::mutex mutex;
  std::atomic<bool> poisoned{false};
};

[[nodiscard]] Phase15JungCenterCoverPruneMassReceipt
launch_phase15_jung_center_cover_prune_mass(
    const Phase15JungCenterCoverPruneMassRequest& request);

}  // namespace morsehgp3d::gpu::detail
