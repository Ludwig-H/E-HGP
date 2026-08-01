#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_frozen_incidence_quotient_schema_version = 1U;
inline constexpr std::string_view
    direct_frozen_incidence_quotient_backend = "reference_cpu";
inline constexpr std::string_view
    direct_frozen_incidence_quotient_profile = "hgp_reduced";
inline constexpr std::string_view direct_frozen_incidence_quotient_mode =
    "certified_frozen_typed_incidence_quotient";
inline constexpr std::string_view
    direct_frozen_incidence_quotient_public_status = "not_claimed";
inline constexpr std::string_view
    direct_frozen_incidence_quotient_proof_basis =
        "frozen_rooted_latent_equal_token_hypergraph_transitive_"
        "equivalence_distinct_root_qr_canonical_flat_quotient_v1";

using ExactFrozenIncidenceTokenId = std::uint64_t;

// A token identity is the complete (kind, token_id) pair.  In particular,
// rooted_carrier(7), latent_carrier(7), and equal_facet(7) are three distinct
// vertices.  Numeric ids remain opaque and their external authority belongs
// to the caller.
enum class ExactFrozenIncidenceTokenKind : std::uint8_t {
  rooted_carrier = 0U,
  latent_carrier = 1U,
  equal_facet = 2U,
};

struct ExactFrozenIncidenceToken {
  ExactFrozenIncidenceTokenKind kind{
      ExactFrozenIncidenceTokenKind::rooted_carrier};
  ExactFrozenIncidenceTokenId token_id{};

  friend bool operator==(
      const ExactFrozenIncidenceToken&,
      const ExactFrozenIncidenceToken&) = default;
};

// The input is one exact CSR partition.  Hyperedge indices must be dense and
// source ordered.  Every hyperedge is non-empty.  Repeated references to the
// same typed token are accepted and do not change the generated relation.
struct ExactFrozenIncidenceHyperedge {
  std::size_t hyperedge_index{};
  std::size_t token_reference_offset{};
  std::size_t token_reference_count{};

  friend bool operator==(
      const ExactFrozenIncidenceHyperedge&,
      const ExactFrozenIncidenceHyperedge&) = default;
};

// Every cap is checked against a conservative allocation upper bound before
// input-shape validation allocates any scratch.  With H hyperedges and T token
// references, the bounds are D<=T distinct tokens, G<=H groups,
// scratch<=3T+H, and flat output<=2T+2H logical entries.
struct ExactFrozenIncidenceQuotientBudget {
  std::size_t maximum_hyperedge_count{};
  std::size_t maximum_token_reference_count{};
  std::size_t maximum_distinct_token_count{};
  std::size_t maximum_group_count{};
  std::size_t maximum_hyperedge_binding_count{};
  std::size_t maximum_token_binding_count{};
  std::size_t maximum_group_token_count{};
  std::size_t maximum_scratch_entry_count{};
  std::size_t maximum_logical_output_entry_count{};

  friend bool operator==(
      const ExactFrozenIncidenceQuotientBudget&,
      const ExactFrozenIncidenceQuotientBudget&) = default;
};

enum class ExactFrozenIncidenceQuotientDisposition : std::uint8_t {
  q_r_zero,
  q_r_one,
  q_r_multiple,
};

// One source-ordered binding is emitted for every input hyperedge.
struct ExactFrozenIncidenceHyperedgeBinding {
  std::size_t source_hyperedge_index{};
  std::size_t source_token_reference_offset{};
  std::size_t source_token_reference_count{};
  std::size_t group_index{};

  friend bool operator==(
      const ExactFrozenIncidenceHyperedgeBinding&,
      const ExactFrozenIncidenceHyperedgeBinding&) = default;
};

// The token-binding table is globally sorted by (kind, token_id), using the
// enum order rooted, latent, equal.  It is exhaustive over distinct tokens.
struct ExactFrozenIncidenceTokenBinding {
  ExactFrozenIncidenceToken token{};
  std::size_t group_index{};

  friend bool operator==(
      const ExactFrozenIncidenceTokenBinding&,
      const ExactFrozenIncidenceTokenBinding&) = default;
};

// Group slices are disjoint and exhaustive.  Groups are ordered by the least
// typed token in their transitive component, and each slice is strictly
// sorted by the same typed-token order.  q_R is rooted_carrier_count only;
// latent carriers and equal facets participate in closure but never in q_R.
struct ExactFrozenIncidenceQuotientGroup {
  std::size_t group_index{};
  std::size_t token_offset{};
  std::size_t token_count{};
  std::size_t rooted_carrier_count{};
  std::size_t latent_carrier_count{};
  std::size_t equal_facet_count{};
  std::size_t hyperedge_count{};
  ExactFrozenIncidenceQuotientDisposition disposition{
      ExactFrozenIncidenceQuotientDisposition::q_r_zero};

  friend bool operator==(
      const ExactFrozenIncidenceQuotientGroup&,
      const ExactFrozenIncidenceQuotientGroup&) = default;
};

struct ExactFrozenIncidenceQuotientCounters {
  std::size_t hyperedge_count{};
  std::size_t token_reference_count{};
  std::size_t distinct_token_count{};
  std::size_t distinct_rooted_carrier_count{};
  std::size_t distinct_latent_carrier_count{};
  std::size_t distinct_equal_facet_count{};
  std::size_t group_count{};
  std::size_t q_r_zero_group_count{};
  std::size_t q_r_one_group_count{};
  std::size_t q_r_multiple_group_count{};
  std::size_t maximum_hyperedge_reference_count{};
  std::size_t maximum_group_token_count{};
  std::size_t maximum_group_q_r{};
  std::size_t logical_output_entry_count{};
  std::size_t logical_scratch_entry_count{};

  friend bool operator==(
      const ExactFrozenIncidenceQuotientCounters&,
      const ExactFrozenIncidenceQuotientCounters&) = default;
};

enum class ExactFrozenIncidenceQuotientDecision : std::uint8_t {
  not_certified,
  no_frozen_incidence_quotient_capacity_overflow,
  no_frozen_incidence_quotient_input_shape_rejected,
  no_frozen_incidence_quotient_budget_exhausted,
  no_frozen_incidence_quotient_allocation_failed,
  complete_certified_frozen_incidence_quotient,
};

enum class ExactFrozenIncidenceQuotientScope : std::uint8_t {
  unspecified,
  exact_equivalence_closure_of_externally_certified_frozen_typed_tokens_only,
};

struct ExactFrozenIncidenceQuotientResult {
  static constexpr std::string_view backend =
      direct_frozen_incidence_quotient_backend;
  static constexpr std::string_view profile =
      direct_frozen_incidence_quotient_profile;
  static constexpr std::string_view mode =
      direct_frozen_incidence_quotient_mode;
  static constexpr std::string_view public_status =
      direct_frozen_incidence_quotient_public_status;
  static constexpr std::string_view proof_basis =
      direct_frozen_incidence_quotient_proof_basis;

  std::uint32_t schema_version{
      direct_frozen_incidence_quotient_schema_version};
  ExactFrozenIncidenceQuotientBudget requested_budget{};
  std::size_t required_hyperedge_capacity{};
  std::size_t required_token_reference_capacity{};
  std::size_t required_distinct_token_capacity{};
  std::size_t required_group_capacity{};
  std::size_t required_hyperedge_binding_capacity{};
  std::size_t required_token_binding_capacity{};
  std::size_t required_group_token_capacity{};
  std::size_t required_scratch_entry_capacity{};
  std::size_t required_logical_output_entry_capacity{};
  std::vector<ExactFrozenIncidenceHyperedgeBinding> hyperedge_bindings;
  std::vector<ExactFrozenIncidenceTokenBinding> token_bindings;
  std::vector<ExactFrozenIncidenceQuotientGroup> groups;
  std::vector<ExactFrozenIncidenceToken> group_tokens;
  ExactFrozenIncidenceQuotientCounters counters{};
  bool input_shape_certified{false};
  bool budget_preflight_certified{false};
  bool typed_token_pairs_certified{false};
  bool equivalence_closure_over_all_token_kinds_certified{false};
  bool groups_and_token_slices_canonical{false};
  bool every_hyperedge_bound_once{false};
  bool every_distinct_token_bound_once{false};
  bool q_r_counts_distinct_rooted_carriers_only{false};
  bool q_r_zero_one_and_multiple_preserved{false};
  bool external_token_snapshot_membership_checked{false};
  bool hgp_batch_actions_claimed{false};
  bool caller_owned_state_or_global_mutation_performed{false};
  bool geometry_gamma_facets_or_cells_materialized{false};
  bool public_status_claimed{false};
  ExactFrozenIncidenceQuotientDecision decision{
      ExactFrozenIncidenceQuotientDecision::not_certified};
  ExactFrozenIncidenceQuotientScope scope{
      ExactFrozenIncidenceQuotientScope::unspecified};

  // Constant-time completion predicate over self-reported facts and sizes.
  // External consumers must use the fresh streaming verifier for payloads.
  [[nodiscard]] bool certified_frozen_incidence_quotient() const noexcept;

  // All non-success exits expose no partially constructed output arena.
  [[nodiscard]] bool atomic_empty_failure() const noexcept;

  friend bool operator==(
      const ExactFrozenIncidenceQuotientResult&,
      const ExactFrozenIncidenceQuotientResult&) = default;
};

// This is a pure combinatorial kernel.  It closes the equivalence generated
// by non-empty typed-token hyperedges and reports q_R; it performs no HGP
// action, geometric lookup, Gamma construction, or global mutation.
[[nodiscard]] ExactFrozenIncidenceQuotientResult
build_exact_direct_frozen_incidence_quotient(
    std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
    std::span<const ExactFrozenIncidenceToken> token_references,
    const ExactFrozenIncidenceQuotientBudget& budget);

struct ExactFrozenIncidenceQuotientStreamingVerification {
  std::size_t source_hyperedge_scan_count{};
  std::size_t source_token_reference_scan_count{};
  bool requested_budget_certified{false};
  bool requirements_certified{false};
  bool input_shape_certified{false};
  bool hyperedge_bindings_certified{false};
  bool token_bindings_certified{false};
  bool groups_certified{false};
  bool group_tokens_certified{false};
  bool counters_certified{false};
  bool result_facts_certified{false};
  bool combinatorial_non_scope_certified{false};
  bool decision_and_scope_certified{false};
  bool no_second_persistent_output_arena_certified{false};
  bool fresh_streaming_replay_certified{false};
  bool result_certified{false};

  friend bool operator==(
      const ExactFrozenIncidenceQuotientStreamingVerification&,
      const ExactFrozenIncidenceQuotientStreamingVerification&) = default;
};

// Replays the CSR authority using a sorted token table, one DSU, one local
// group map and G group facts.  Observed flat arenas are checked in place;
// the verifier never constructs a second hyperedge/token/group payload.
[[nodiscard]] ExactFrozenIncidenceQuotientStreamingVerification
verify_exact_direct_frozen_incidence_quotient_streaming(
    std::span<const ExactFrozenIncidenceHyperedge> hyperedges,
    std::span<const ExactFrozenIncidenceToken> token_references,
    const ExactFrozenIncidenceQuotientBudget& trusted_budget,
    const ExactFrozenIncidenceQuotientResult& observed);

}  // namespace morsehgp3d::hierarchy
