#pragma once

#include "morsehgp3d/hierarchy/boruvka.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class K1BoruvkaCandidateContextState;
class K1BoruvkaCandidateHostState;
}  // namespace detail

struct K1BoruvkaSeed {
  spatial::PointId source_point_id{};
  spatial::PointId target_point_id{};
  exact::ExactLevel exact_squared_cutoff{};

  friend bool operator==(const K1BoruvkaSeed&, const K1BoruvkaSeed&) = default;
};

struct K1BoruvkaCandidate {
  spatial::PointId source_point_id{};
  spatial::PointId target_point_id{};

  friend bool operator==(
      const K1BoruvkaCandidate&, const K1BoruvkaCandidate&) = default;
};

struct K1BoruvkaCandidateAudit {
  static constexpr const char* proposal_semantics =
      "gpu_stackless_lbvh_fixed_seed_candidate_superset";
  static constexpr const char* decision_semantics =
      "cpu_exact_seed_replay_and_kappa_resolution";

  std::size_t resident_point_count{};
  std::size_t resident_node_count{};
  std::size_t frozen_component_count{};
  std::size_t uniform_lbvh_node_count{};
  std::size_t mixed_lbvh_node_count{};
  std::size_t exact_seed_count{};
  std::size_t gpu_candidate_count{};
  std::size_t gpu_output_capacity{};
  std::size_t gpu_kernel_launch_count{};
  std::size_t gpu_synchronization_count{};
  std::size_t gpu_count_pass_node_visit_count{};
  std::size_t gpu_emit_pass_node_visit_count{};
  std::size_t gpu_uniform_component_prune_count{};
  std::size_t gpu_strict_aabb_prune_count{};
  std::size_t gpu_invalid_bound_descent_count{};
  std::size_t cpu_exact_aabb_bound_evaluation_count{};
  std::size_t cpu_required_candidate_count{};
  std::size_t cpu_exact_candidate_distance_evaluation_count{};
  std::uint64_t buffer_epoch{};
  std::uint64_t proposal_digest_fnv1a{};
  bool frozen_labels_certified{false};
  bool rope_topology_certified{false};
  bool exact_capacity_certified{false};
  bool no_truncation_certified{false};
  bool candidate_superset_certified{false};
  bool cpu_exact_resolution_complete{false};

  friend bool operator==(
      const K1BoruvkaCandidateAudit&, const K1BoruvkaCandidateAudit&) = default;
};

// The GPU output is a proposal only. candidate_offsets groups candidates by
// source PointId; the CPU-certified component minima are a separate field and
// no contraction or public MorseHGP3D status is performed by this API.
struct K1BoruvkaRoundProposal {
  std::vector<spatial::PointId> frozen_component_labels;
  std::vector<K1BoruvkaSeed> seeds;
  std::vector<std::size_t> candidate_offsets;
  std::vector<K1BoruvkaCandidate> candidates;
  std::vector<hierarchy::K1BoruvkaComponentMinimum>
      cpu_exact_component_minima;
  K1BoruvkaCandidateAudit audit;
};

class K1BoruvkaCandidateContext final {
 public:
  K1BoruvkaCandidateContext(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud);
  ~K1BoruvkaCandidateContext() noexcept;

  K1BoruvkaCandidateContext(K1BoruvkaCandidateContext&&) noexcept;
  K1BoruvkaCandidateContext& operator=(
      K1BoruvkaCandidateContext&&) noexcept;

  K1BoruvkaCandidateContext(const K1BoruvkaCandidateContext&) = delete;
  K1BoruvkaCandidateContext& operator=(
      const K1BoruvkaCandidateContext&) = delete;

  [[nodiscard]] K1BoruvkaRoundProposal propose_round(
      const spatial::CanonicalPointCloud& cloud,
      std::span<const spatial::PointId> frozen_component_labels);

  [[nodiscard]] std::size_t point_count() const noexcept;
  [[nodiscard]] std::size_t node_count() const noexcept;

 private:
  void require_matching_cloud(
      const spatial::CanonicalPointCloud& cloud) const;

  std::shared_ptr<detail::K1BoruvkaCandidateContextState> state_;
  std::unique_ptr<detail::K1BoruvkaCandidateHostState> host_;
};

}  // namespace morsehgp3d::gpu
