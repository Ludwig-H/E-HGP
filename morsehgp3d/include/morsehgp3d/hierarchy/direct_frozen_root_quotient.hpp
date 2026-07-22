#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_frozen_root_quotient_schema_version =
    1U;
inline constexpr std::string_view direct_frozen_root_quotient_backend =
    "reference_cpu";
inline constexpr std::string_view direct_frozen_root_quotient_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_frozen_root_quotient_mode =
    "certified_combinatorial_kernel";
inline constexpr std::string_view direct_frozen_root_quotient_public_status =
    "not_claimed";
inline constexpr std::string_view direct_frozen_root_quotient_proof_basis =
    "frozen_root_hypergraph_equivalence_closure_canonical_flat_quotient_v1";

using ExactFrozenRootId = std::uint64_t;

// The input is one exact CSR partition.  Hyperedge ids must be dense and in
// canonical source order.  A root may occur more than once: repetitions do
// not alter the generated equivalence relation.
struct ExactFrozenRootHyperedge {
  std::size_t hyperedge_index{};
  std::size_t root_reference_offset{};
  std::size_t root_reference_count{};

  friend bool operator==(
      const ExactFrozenRootHyperedge&,
      const ExactFrozenRootHyperedge&) = default;
};

// Every cap is checked against an allocation-safe upper bound before any
// scratch or output arena is allocated.  Distinct roots and groups are
// conservatively bounded by H and E, respectively; the scratch bound is
// 3H+E logical entries.
struct ExactFrozenRootQuotientBudget {
  std::size_t maximum_hyperedge_count{};
  std::size_t maximum_root_reference_count{};
  std::size_t maximum_distinct_root_count{};
  std::size_t maximum_group_count{};
  std::size_t maximum_scratch_entry_count{};

  friend bool operator==(
      const ExactFrozenRootQuotientBudget&,
      const ExactFrozenRootQuotientBudget&) = default;
};

enum class ExactFrozenRootQuotientDisposition : std::uint8_t {
  one_root_class,
  multiple_root_class,
};

// There is exactly one binding per input hyperedge.  Keeping this arena in
// source order preserves event provenance without nesting vectors in groups.
struct ExactFrozenRootHyperedgeBinding {
  std::size_t source_hyperedge_index{};
  std::size_t source_root_reference_offset{};
  std::size_t source_root_reference_count{};
  std::size_t group_index{};

  friend bool operator==(
      const ExactFrozenRootHyperedgeBinding&,
      const ExactFrozenRootHyperedgeBinding&) = default;
};

// Root slices are disjoint, sorted and exhaustive over the touched roots.
// Groups are ordered by their smallest RootId.  The disposition describes
// only the cardinality of a root class; it is not an HGP batch action.
struct ExactFrozenRootQuotientGroup {
  std::size_t group_index{};
  std::size_t root_offset{};
  std::size_t root_count{};
  std::size_t hyperedge_count{};
  ExactFrozenRootQuotientDisposition disposition{
      ExactFrozenRootQuotientDisposition::one_root_class};

  friend bool operator==(
      const ExactFrozenRootQuotientGroup&,
      const ExactFrozenRootQuotientGroup&) = default;
};

struct ExactFrozenRootQuotientCounters {
  std::size_t hyperedge_count{};
  std::size_t root_reference_count{};
  std::size_t distinct_root_count{};
  std::size_t group_count{};
  std::size_t one_root_group_count{};
  std::size_t multiple_root_group_count{};
  std::size_t maximum_hyperedge_reference_count{};
  std::size_t maximum_group_root_count{};
  std::size_t logical_output_entry_count{};
  std::size_t logical_scratch_entry_count{};

  friend bool operator==(
      const ExactFrozenRootQuotientCounters&,
      const ExactFrozenRootQuotientCounters&) = default;
};

enum class ExactFrozenRootQuotientDecision : std::uint8_t {
  not_certified,
  no_frozen_root_quotient_capacity_overflow,
  no_frozen_root_quotient_input_shape_rejected,
  no_frozen_root_quotient_budget_exhausted,
  complete_certified_frozen_root_quotient,
};

enum class ExactFrozenRootQuotientScope : std::uint8_t {
  unspecified,
  exact_equivalence_closure_of_externally_certified_frozen_roots_only,
};

struct ExactFrozenRootQuotientResult {
  static constexpr std::string_view backend =
      direct_frozen_root_quotient_backend;
  static constexpr std::string_view profile =
      direct_frozen_root_quotient_profile;
  static constexpr std::string_view mode = direct_frozen_root_quotient_mode;
  static constexpr std::string_view public_status =
      direct_frozen_root_quotient_public_status;
  static constexpr std::string_view proof_basis =
      direct_frozen_root_quotient_proof_basis;

  std::uint32_t schema_version{direct_frozen_root_quotient_schema_version};
  ExactFrozenRootQuotientBudget requested_budget{};
  std::size_t required_hyperedge_capacity{};
  std::size_t required_root_reference_capacity{};
  std::size_t required_distinct_root_capacity{};
  std::size_t required_group_capacity{};
  std::size_t required_scratch_entry_capacity{};
  std::size_t logical_output_entry_limit{};
  std::vector<ExactFrozenRootHyperedgeBinding> hyperedge_bindings;
  std::vector<ExactFrozenRootQuotientGroup> groups;
  std::vector<ExactFrozenRootId> group_root_ids;
  ExactFrozenRootQuotientCounters counters{};
  bool input_shape_certified{false};
  bool budget_preflight_certified{false};
  bool equivalence_closure_certified{false};
  bool groups_and_root_slices_canonical{false};
  bool every_hyperedge_bound_once{false};
  bool one_root_groups_preserved{false};
  bool external_root_snapshot_membership_checked{false};
  bool zero_root_hyperedges_supported{false};
  bool latent_facet_tokens_supported{false};
  bool hgp_batch_actions_claimed{false};
  bool root_attachment_or_global_mutation_performed{false};
  bool geometry_facets_or_cells_materialized{false};
  bool public_status_claimed{false};
  ExactFrozenRootQuotientDecision decision{
      ExactFrozenRootQuotientDecision::not_certified};
  ExactFrozenRootQuotientScope scope{
      ExactFrozenRootQuotientScope::unspecified};

  // Constant-time completion predicate over self-reported facts and sizes.
  // It is not a trust boundary for payload values; external consumers must
  // use the fresh streaming verifier below.
  [[nodiscard]] bool certified_frozen_root_quotient() const noexcept;

  friend bool operator==(
      const ExactFrozenRootQuotientResult&,
      const ExactFrozenRootQuotientResult&) = default;
};

// Root ids are opaque labels whose strict pre-batch authority belongs to the
// caller.  This kernel proves only their hypergraph equivalence closure.  It
// performs no facet localization and mutates no caller-owned disjoint set.
// In particular, an empty hyperedge is rejected: the q_R=0 mirror case needs
// latent-facet tokens and cannot be represented by this root-only ABI.
[[nodiscard]] ExactFrozenRootQuotientResult
build_exact_direct_frozen_root_quotient(
    std::span<const ExactFrozenRootHyperedge> hyperedges,
    std::span<const ExactFrozenRootId> root_references,
    const ExactFrozenRootQuotientBudget& budget);

struct ExactFrozenRootQuotientStreamingVerification {
  std::size_t source_hyperedge_scan_count{};
  std::size_t source_root_reference_scan_count{};
  bool requested_budget_certified{false};
  bool requirements_certified{false};
  bool input_shape_certified{false};
  bool hyperedge_bindings_certified{false};
  bool groups_certified{false};
  bool group_root_ids_certified{false};
  bool counters_certified{false};
  bool result_facts_certified{false};
  bool root_only_non_scope_certified{false};
  bool decision_and_scope_certified{false};
  bool no_second_persistent_output_arena_certified{false};
  bool fresh_streaming_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactFrozenRootQuotientStreamingVerification&,
      const ExactFrozenRootQuotientStreamingVerification&) = default;
};

// Replays the CSR authority with H+2R+G peak logical scratch.  Bindings are
// compared one at a time, group records use only G temporary facts, and the
// observed root arena is checked in place; no second E+G+R payload is built.
[[nodiscard]] ExactFrozenRootQuotientStreamingVerification
verify_exact_direct_frozen_root_quotient_streaming(
    std::span<const ExactFrozenRootHyperedge> hyperedges,
    std::span<const ExactFrozenRootId> root_references,
    const ExactFrozenRootQuotientBudget& trusted_budget,
    const ExactFrozenRootQuotientResult& observed);

}  // namespace morsehgp3d::hierarchy
