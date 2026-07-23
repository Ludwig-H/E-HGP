#pragma once

#include "morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_industrial_plan_schema_version = 1U;
inline constexpr std::string_view direct_morse_industrial_plan_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_industrial_plan_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_industrial_plan_mode =
    "certified";
inline constexpr std::string_view
    direct_morse_industrial_plan_deployment_status =
        "architecture_only";
inline constexpr std::string_view direct_morse_industrial_plan_public_status =
    "not_claimed";
inline constexpr std::size_t
    direct_morse_interactive_resident_maximum_point_count = 50'000U;

enum class ExactDirectMorseIndustrialPolicy : std::uint8_t {
  unspecified,
  automatic,
  interactive_resident_50k,
  massive_external_streaming,
};

// Every byte coefficient is supplied by the caller.  In particular, the
// planner does not confuse sizeof(host records) with the memory contract of
// a future device, pinned-host or external-run implementation.
struct ExactDirectMorseIndustrialMemoryModel {
  std::uint64_t fixed_chunk_bytes{};
  std::uint64_t checkpoint_boundary_bytes{};
  std::uint64_t external_run_boundary_bytes{};
  std::uint64_t bytes_per_batch{};
  std::uint64_t bytes_per_birth{};
  std::uint64_t bytes_per_saddle{};
  std::uint64_t bytes_per_arm{};
  std::uint64_t bytes_per_key_point_reference{};
  std::uint64_t bytes_per_node_upper_bound{};
  std::uint64_t bytes_per_child_reference_upper_bound{};
  std::uint64_t bytes_per_descent_node_reserve{};
  std::uint64_t descent_node_reserve_per_arm{};

  friend bool operator==(
      const ExactDirectMorseIndustrialMemoryModel&,
      const ExactDirectMorseIndustrialMemoryModel&) = default;
};

struct ExactDirectMorseIndustrialChunkBudget {
  std::uint64_t maximum_bytes{};
  std::uint64_t maximum_batch_count{};
  std::uint64_t maximum_birth_count{};
  std::uint64_t maximum_saddle_count{};
  std::uint64_t maximum_arm_count{};
  std::uint64_t maximum_descent_node_count{};

  friend bool operator==(
      const ExactDirectMorseIndustrialChunkBudget&,
      const ExactDirectMorseIndustrialChunkBudget&) = default;
};

struct ExactDirectMorseIndustrialPlanConfig {
  ExactDirectMorseIndustrialPolicy policy{
      ExactDirectMorseIndustrialPolicy::automatic};
  ExactDirectMorseIndustrialMemoryModel memory_model{};
  ExactDirectMorseIndustrialChunkBudget chunk_budget{};

  friend bool operator==(
      const ExactDirectMorseIndustrialPlanConfig&,
      const ExactDirectMorseIndustrialPlanConfig&) = default;
};

struct ExactDirectMorseIndustrialCounters {
  std::uint64_t batch_count{};
  std::uint64_t birth_count{};
  std::uint64_t saddle_count{};
  std::uint64_t arm_count{};
  // Exact for the source journals: each birth key and each arm key in an
  // (order, level) batch has exactly `order` PointId references.
  std::uint64_t key_point_reference_count{};
  // Conservative forest bounds, not materialized forest records.
  std::uint64_t node_count_upper_bound{};
  std::uint64_t child_reference_count_upper_bound{};
  // Caller-chosen reserve, equal to arms times reserve-per-arm.
  std::uint64_t descent_node_reserve_count{};

  friend bool operator==(
      const ExactDirectMorseIndustrialCounters&,
      const ExactDirectMorseIndustrialCounters&) = default;
};

// A compact interval into the already certified 10.1 batch array.  No exact
// level, key, facet, descent node or forest payload is copied into the plan.
struct ExactDirectMorseIndustrialChunk {
  std::size_t chunk_index{};
  std::size_t source_batch_begin_index{};
  std::size_t source_batch_end_index{};
  ExactDirectMorseIndustrialCounters counters{};
  std::uint64_t estimated_byte_count{};
  bool checkpoint_boundary_after_chunk{false};
  bool external_run_boundary_after_chunk{false};

  friend bool operator==(
      const ExactDirectMorseIndustrialChunk&,
      const ExactDirectMorseIndustrialChunk&) = default;
};

enum class ExactDirectMorseIndustrialPlanDecision : std::uint8_t {
  not_planned,
  no_plan_invalid_policy,
  no_plan_capacity_overflow,
  no_plan_allocation_failed,
  no_plan_source_not_certified,
  no_plan_source_join_inconsistent,
  no_plan_resident_requirements_not_met,
  no_plan_atomic_batch_exceeds_chunk_budget,
  complete_architecture_only_plan,
  // Appended to preserve every schema-v1 decision ordinal.
  no_plan_chunk_count_budget_exhausted,
};

enum class ExactDirectMorseIndustrialPlanScope : std::uint8_t {
  unspecified,
  certified_10_1_10_2_batch_resource_projection_only,
};

struct ExactDirectMorseIndustrialPlanResult {
  static constexpr std::string_view backend =
      direct_morse_industrial_plan_backend;
  static constexpr std::string_view profile =
      direct_morse_industrial_plan_profile;
  static constexpr std::string_view mode =
      direct_morse_industrial_plan_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_industrial_plan_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_industrial_plan_public_status;

  std::uint32_t schema_version{
      direct_morse_industrial_plan_schema_version};
  ExactDirectMorseIndustrialPlanConfig requested_config{};
  ExactDirectMorseIndustrialPolicy selected_policy{
      ExactDirectMorseIndustrialPolicy::unspecified};
  std::size_t point_count{};
  ExactDirectMorseIndustrialCounters source_counters{};
  std::uint64_t total_estimated_byte_count{};
  std::optional<std::size_t> rejected_source_batch_index;
  std::vector<ExactDirectMorseIndustrialChunk> chunks;

  bool source_event_journal_freshly_replayed{false};
  bool source_arm_seed_journal_freshly_replayed{false};
  bool source_batches_and_arm_families_joined{false};
  bool birth_saddle_and_arm_counts_exact{false};
  bool at_most_four_arms_per_saddle{false};
  bool key_reference_formula_exact{false};
  bool node_and_child_bounds_certified{false};
  bool caller_descent_node_reserve_applied{false};
  bool overflow_checked_before_publication{false};
  bool exact_order_level_batches_never_split{false};
  bool chunks_cover_consecutive_batches{false};
  bool resident_plan_has_exactly_one_chunk{false};
  bool streaming_boundaries_after_every_chunk{false};
  bool atomic_rejection_publishes_no_chunks{false};
  bool architecture_only{true};
  bool hierarchy_forest_or_descent_materialized{false};
  bool facet_coface_cell_gamma_or_delaunay_materialized{false};
  bool sub_second_latency_claimed{false};
  bool ten_million_point_capacity_claimed{false};
  bool public_status_claimed{false};
  ExactDirectMorseIndustrialPlanDecision decision{
      ExactDirectMorseIndustrialPlanDecision::not_planned};
  ExactDirectMorseIndustrialPlanScope scope{
      ExactDirectMorseIndustrialPlanScope::unspecified};

  [[nodiscard]] bool complete_architecture_plan() const noexcept;

  friend bool operator==(
      const ExactDirectMorseIndustrialPlanResult&,
      const ExactDirectMorseIndustrialPlanResult&) = default;
};

// The two source journals are freshly replayed in constant auxiliary record
// storage before planning.  The returned chunks only project their certified
// counts; this function performs no hierarchy reduction.
[[nodiscard]] ExactDirectMorseIndustrialPlanResult
build_exact_direct_morse_industrial_plan(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanConfig& config);

// This construction-level cap is checked before every chunk publication.
// The returned scientific payload stays schema-identical to the unbounded
// overload; callers that persist or execute it must retain their own cap as
// part of the enclosing authority.
[[nodiscard]] ExactDirectMorseIndustrialPlanResult
build_exact_direct_morse_industrial_plan_with_chunk_count_cap(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_arm_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_seed_journal,
    const ExactDirectMorseIndustrialPlanConfig& config,
    std::size_t maximum_output_chunk_count);

}  // namespace morsehgp3d::hierarchy
