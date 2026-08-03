#include "morsehgp3d/hierarchy/direct_projectable_contribution_window.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view resident_batch_identity_domain =
    "MorseHGP3D/phase15/normalized-resident-provider/batch/v1/sha256/";
constexpr std::string_view resident_chain_step_domain =
    "MorseHGP3D/phase15/normalized-resident-provider/chain/v1/sha256/";
constexpr std::string_view contribution_window_digest_domain =
    "MorseHGP3D/phase15/direct-projectable-contribution-window/"
    "projection/v1/sha256/";

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& output) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& output) noexcept {
  if (right != 0U &&
      left > std::numeric_limits<std::size_t>::max() / right) {
    return false;
  }
  output = left * right;
  return true;
}

[[nodiscard]] bool checked_increment(std::size_t& value) noexcept {
  return checked_add(value, 1U, value);
}

void append_u8(
    contract::CanonicalSha256Builder& builder,
    std::uint8_t value) {
  const std::array<std::uint8_t, 1U> bytes{value};
  builder.update(bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_bool(
    contract::CanonicalSha256Builder& builder,
    bool value) {
  append_u8(builder, value ? std::uint8_t{1U} : std::uint8_t{0U});
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

void append_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view value) {
  append_size(builder, value.size());
  builder.update(value);
}

void append_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& value) {
  append_text(builder, value.canonical_key());
}

void append_optional_size(
    contract::CanonicalSha256Builder& builder,
    const std::optional<std::size_t>& value) {
  append_bool(builder, value.has_value());
  if (value.has_value()) {
    append_size(builder, *value);
  }
}

void append_facet_key(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseFacetKey& key) {
  append_size(builder, key.point_count);
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    append_u64(builder, static_cast<std::uint64_t>(key.point_ids[index]));
  }
}

void append_direct_reference(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseUnifiedLevelPlanDirectReference& reference) {
  append_size(builder, reference.direct_reference_index);
  append_size(builder, reference.source_role_record_index);
  append_size(builder, reference.source_event_projection_index);
  append_u8(builder, static_cast<std::uint8_t>(reference.role));
  append_optional_size(builder, reference.source_incidence_family_index);
  append_optional_size(builder, reference.source_star_direct_coface_index);
  append_optional_size(builder, reference.direct_birth_facet_token_index);
}

void append_residual_reference(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseUnifiedLevelPlanResidualReference& reference) {
  append_size(builder, reference.residual_reference_index);
  append_size(builder, reference.source_star_coface_index);
}

void append_coface_facet_reference(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectSparseUnifiedLevelPlanCofaceFacetReference& reference) {
  append_size(builder, reference.coface_facet_reference_index);
  append_size(builder, reference.source_star_coface_index);
  append_size(builder, reference.removed_union_point_index);
  append_u64(builder, static_cast<std::uint64_t>(reference.removed_point_id));
  append_size(builder, reference.facet_token_index);
}

[[nodiscard]] contract::CanonicalId compute_successor_chain_digest(
    const contract::CanonicalId& source_chain_digest,
    const contract::CanonicalId& batch_identity_digest) {
  contract::CanonicalSha256Builder builder;
  builder.update(resident_chain_step_domain);
  append_id(builder, source_chain_digest);
  append_id(builder, batch_identity_digest);
  return builder.finalize();
}

struct SourceCofaceBinding {
  ExactDirectProjectableContributionSourceKind kind{
      ExactDirectProjectableContributionSourceKind::direct_saddle};
  std::size_t source_coface_index{};
  std::size_t source_reference_index{};
  std::optional<std::size_t> source_role_record_index;
  std::optional<std::size_t> source_event_projection_index;
  std::optional<std::size_t> source_incidence_family_index;
};

[[nodiscard]] bool binding_less(
    const SourceCofaceBinding& left,
    const SourceCofaceBinding& right) noexcept {
  return left.source_coface_index < right.source_coface_index;
}

[[nodiscard]] bool atom_less(
    const ExactDirectProjectableContributionAtom& left,
    const ExactDirectProjectableContributionAtom& right) noexcept {
  return std::tuple{
             left.source_coface_index,
             left.removed_union_point_index,
             left.stable_source_facet_token_index,
             left.removed_point_id,
             left.source_deletion_reference_index,
             static_cast<std::uint8_t>(left.source_kind),
             left.source_reference_index} <
         std::tuple{
             right.source_coface_index,
             right.removed_union_point_index,
             right.stable_source_facet_token_index,
             right.removed_point_id,
             right.source_deletion_reference_index,
             static_cast<std::uint8_t>(right.source_kind),
             right.source_reference_index};
}

[[nodiscard]] contract::CanonicalId compute_contribution_window_digest(
    const ExactDirectProjectableContributionWindowReceipt& receipt,
    std::span<const ExactDirectProjectableContributionFacetDefinition>
        definitions,
    std::span<const spatial::PointId> point_references,
    std::span<const ExactDirectProjectableContributionAtom> atoms) {
  contract::CanonicalSha256Builder builder;
  builder.update(contribution_window_digest_domain);
  append_u64(builder, receipt.scientific_authority_id);
  append_id(builder, receipt.manifest_digest);
  append_id(builder, receipt.source_identity_digest);
  append_id(builder, receipt.horizontal_incidence_authority_digest);
  append_id(builder, receipt.source_chain_digest);
  append_id(builder, receipt.batch_identity_digest);
  append_id(builder, receipt.successor_chain_digest);
  append_size(builder, receipt.source_batch_index);
  append_size(builder, receipt.source_future_snapshot_index);
  append_size(builder, receipt.order);
  append_level(builder, receipt.exact_squared_radius);
  append_size(builder, receipt.source_direct_coface_count);
  append_size(builder, receipt.source_residual_coface_count);
  append_size(builder, definitions.size());
  for (const auto& definition : definitions) {
    append_size(builder, definition.facet_definition_index);
    append_size(builder, definition.stable_source_facet_token_index);
    append_size(builder, definition.order);
    append_size(builder, definition.point_reference_offset);
    append_size(builder, definition.point_reference_count);
  }
  append_size(builder, point_references.size());
  for (const spatial::PointId point_id : point_references) {
    append_u64(builder, static_cast<std::uint64_t>(point_id));
  }
  append_size(builder, atoms.size());
  for (const auto& atom : atoms) {
    append_size(builder, atom.contribution_atom_index);
    append_u8(builder, static_cast<std::uint8_t>(atom.source_kind));
    append_size(builder, atom.source_coface_index);
    append_size(builder, atom.source_reference_index);
    append_optional_size(builder, atom.source_role_record_index);
    append_optional_size(builder, atom.source_event_projection_index);
    append_optional_size(builder, atom.source_incidence_family_index);
    append_size(builder, atom.source_deletion_reference_index);
    append_size(builder, atom.removed_union_point_index);
    append_u64(builder, static_cast<std::uint64_t>(atom.removed_point_id));
    append_size(builder, atom.stable_source_facet_token_index);
    append_size(builder, atom.facet_definition_index);
    append_size(builder, atom.order);
    append_level(builder, atom.exact_squared_radius);
  }
  return builder.finalize();
}

struct TokenUseCount {
  std::size_t direct_birth_count{};
  std::size_t coface_deletion_count{};
};

}  // namespace

struct ExactDirectProjectableContributionWindowResult::Impl {
  ExactDirectProjectableContributionWindowReceipt attested_receipt{};
  std::vector<ExactDirectProjectableContributionFacetDefinition>
      facet_definitions;
  std::vector<spatial::PointId> facet_point_references;
  std::vector<ExactDirectProjectableContributionAtom> contribution_atoms;
  std::shared_ptr<const void> source_control_identity;
  std::shared_ptr<const void> scientific_window_identity;
};

ExactDirectProjectableContributionWindowResult::
    ExactDirectProjectableContributionWindowResult() noexcept = default;
ExactDirectProjectableContributionWindowResult::
    ~ExactDirectProjectableContributionWindowResult() = default;
ExactDirectProjectableContributionWindowResult::
    ExactDirectProjectableContributionWindowResult(
        ExactDirectProjectableContributionWindowResult&&) noexcept = default;
ExactDirectProjectableContributionWindowResult&
ExactDirectProjectableContributionWindowResult::operator=(
    ExactDirectProjectableContributionWindowResult&&) noexcept = default;

const ExactDirectProjectableContributionWindowReceipt&
ExactDirectProjectableContributionWindowResult::receipt() const noexcept {
  return receipt_;
}

std::span<const ExactDirectProjectableContributionFacetDefinition>
ExactDirectProjectableContributionWindowResult::facet_definitions()
    const noexcept {
  return impl_ == nullptr
             ? std::span<
                   const ExactDirectProjectableContributionFacetDefinition>{}
             : std::span<
                   const ExactDirectProjectableContributionFacetDefinition>{
                   impl_->facet_definitions};
}

std::span<const spatial::PointId>
ExactDirectProjectableContributionWindowResult::facet_point_references()
    const noexcept {
  return impl_ == nullptr ? std::span<const spatial::PointId>{}
                          : std::span<const spatial::PointId>{
                                impl_->facet_point_references};
}

std::span<const ExactDirectProjectableContributionAtom>
ExactDirectProjectableContributionWindowResult::contribution_atoms()
    const noexcept {
  return impl_ == nullptr
             ? std::span<const ExactDirectProjectableContributionAtom>{}
             : std::span<const ExactDirectProjectableContributionAtom>{
                   impl_->contribution_atoms};
}

bool ExactDirectProjectableContributionWindowResult::
    certified_exhaustive_projection() const noexcept {
  if (impl_ == nullptr || impl_->source_control_identity == nullptr ||
      impl_->scientific_window_identity == nullptr ||
      receipt_ != impl_->attested_receipt ||
      receipt_.schema_version !=
          direct_projectable_contribution_window_schema_version ||
      receipt_.scientific_authority_id == 0U ||
      receipt_.contribution_window_digest == contract::CanonicalId{} ||
      impl_->facet_definitions.size() !=
          receipt_.required_touched_facet_definition_count ||
      impl_->facet_point_references.size() !=
          receipt_.required_facet_point_reference_count ||
      impl_->contribution_atoms.size() !=
          receipt_.required_contribution_atom_count ||
      receipt_.required_window_facet_scan_count >
          receipt_.requested_budget.maximum_window_facet_scan_count ||
      receipt_.required_window_facet_point_scan_count >
          receipt_.requested_budget.maximum_window_facet_point_scan_count ||
      receipt_.required_direct_reference_scan_count >
          receipt_.requested_budget.maximum_direct_reference_scan_count ||
      receipt_.required_residual_reference_scan_count >
          receipt_.requested_budget.maximum_residual_reference_scan_count ||
      receipt_.required_coface_facet_reference_scan_count >
          receipt_.requested_budget
              .maximum_coface_facet_reference_scan_count ||
      receipt_.required_touched_facet_definition_count >
          receipt_.requested_budget
              .maximum_touched_facet_definition_count ||
      receipt_.required_facet_point_reference_count >
          receipt_.requested_budget.maximum_facet_point_reference_count ||
      receipt_.required_contribution_atom_count >
          receipt_.requested_budget.maximum_contribution_atom_count ||
      receipt_.required_scratch_entry_count >
          receipt_.requested_budget.maximum_scratch_entry_count) {
    return false;
  }

  std::size_t expected_atom_count = 0U;
  std::size_t source_coface_count = 0U;
  std::size_t expected_scratch = 0U;
  if (!checked_add(
          receipt_.source_direct_coface_count,
          receipt_.source_residual_coface_count,
          source_coface_count) ||
      !checked_multiply(
          source_coface_count, receipt_.order + 1U, expected_atom_count) ||
      expected_atom_count != receipt_.required_contribution_atom_count ||
      !checked_add(
          receipt_.required_window_facet_scan_count,
          receipt_.required_direct_reference_scan_count,
          expected_scratch) ||
      !checked_add(
          expected_scratch,
          receipt_.required_residual_reference_scan_count,
          expected_scratch) ||
      !checked_add(
          expected_scratch,
          receipt_.required_coface_facet_reference_scan_count,
          expected_scratch) ||
      expected_scratch != receipt_.required_scratch_entry_count ||
      receipt_.direct_contribution_atom_count +
              receipt_.residual_contribution_atom_count !=
          receipt_.required_contribution_atom_count) {
    return false;
  }

  std::size_t point_cursor = 0U;
  for (std::size_t index = 0U;
       index < impl_->facet_definitions.size();
       ++index) {
    const auto& definition = impl_->facet_definitions[index];
    if (definition.facet_definition_index != index ||
        definition.order != receipt_.order ||
        definition.point_reference_offset != point_cursor ||
        definition.point_reference_count != receipt_.order ||
        definition.point_reference_count >
            impl_->facet_point_references.size() - point_cursor ||
        (index != 0U &&
         impl_->facet_definitions[index - 1U]
                 .stable_source_facet_token_index >=
             definition.stable_source_facet_token_index)) {
      return false;
    }
    const auto first = impl_->facet_point_references.begin() +
                       static_cast<std::ptrdiff_t>(point_cursor);
    const auto last = first +
                      static_cast<std::ptrdiff_t>(
                          definition.point_reference_count);
    if (!std::is_sorted(first, last) ||
        std::adjacent_find(first, last) != last ||
        !checked_add(
            point_cursor,
            definition.point_reference_count,
            point_cursor)) {
      return false;
    }
  }
  if (point_cursor != impl_->facet_point_references.size()) {
    return false;
  }

  std::size_t direct_atoms = 0U;
  std::size_t residual_atoms = 0U;
  for (std::size_t index = 0U;
       index < impl_->contribution_atoms.size();
       ++index) {
    const auto& atom = impl_->contribution_atoms[index];
    if (atom.contribution_atom_index != index ||
        atom.order != receipt_.order ||
        atom.exact_squared_radius != receipt_.exact_squared_radius ||
        atom.facet_definition_index >= impl_->facet_definitions.size() ||
        impl_->facet_definitions[atom.facet_definition_index]
                .stable_source_facet_token_index !=
            atom.stable_source_facet_token_index ||
        (index != 0U && atom_less(atom, impl_->contribution_atoms[index - 1U]))) {
      return false;
    }
    if (atom.source_kind ==
        ExactDirectProjectableContributionSourceKind::direct_saddle) {
      if (!atom.source_role_record_index.has_value() ||
          !atom.source_event_projection_index.has_value() ||
          !atom.source_incidence_family_index.has_value() ||
          !checked_increment(direct_atoms)) {
        return false;
      }
    } else if (atom.source_kind ==
               ExactDirectProjectableContributionSourceKind::
                   residual_incidence) {
      if (atom.source_role_record_index.has_value() ||
          atom.source_event_projection_index.has_value() ||
          atom.source_incidence_family_index.has_value() ||
          !checked_increment(residual_atoms)) {
        return false;
      }
    } else {
      return false;
    }
  }

  return direct_atoms == receipt_.direct_contribution_atom_count &&
         residual_atoms == receipt_.residual_contribution_atom_count &&
         receipt_.private_capability_check_count == 4U &&
         receipt_.authenticated_payload_full_replay_count == 1U &&
         receipt_.scientific_source_stamp_bound &&
         receipt_.scientific_window_private_identity_bound &&
         receipt_.manifest_source_batch_and_chain_bound &&
         receipt_.complete_authenticated_window_payload_replayed &&
         receipt_.every_coface_facet_reference_projected_once &&
         receipt_.direct_and_residual_cofaces_retained &&
         receipt_.source_records_not_filtered_by_h0_node_presence &&
         receipt_.touched_facet_definitions_canonical_and_deduplicated &&
         receipt_.facet_point_csr_canonical &&
         receipt_.every_atom_retains_exact_batch_squared_radius &&
         !receipt_.source_session_window_forest_or_ledger_mutated &&
         !receipt_
              .global_facet_coface_incidence_or_gamma_catalog_materialized &&
         !receipt_.ordinary_or_higher_order_delaunay_materialized &&
         !receipt_.contribution_aggregation_performed &&
         !receipt_.source_exactness_claimed && !receipt_.vertical_maps_complete &&
         !receipt_.durable_restart_supported &&
         !receipt_.public_status_claimed &&
         receipt_.decision ==
             ExactDirectProjectableContributionWindowDecision::
                 complete_exhaustive_authenticated_window_projection &&
         receipt_.scope ==
             ExactDirectProjectableContributionWindowScope::
                 exhaustive_projection_of_one_already_authenticated_scientific_window_only;
}

ExactDirectProjectableContributionWindowResult
ExactDirectProjectableContributionWindow::build(
    const ExactDirectNormalizedH0ScientificSourceStamp& source_stamp,
    const ExactDirectNormalizedH0ScientificWindowCapabilityPreparedWindow&
        scientific_window,
    const ExactDirectProjectableContributionWindowBudget& budget) noexcept {
  ExactDirectProjectableContributionWindowResult output;
  auto& receipt = output.receipt_;
  receipt.requested_budget = budget;

  if (!source_stamp.certified_scientific_source_stamp()) {
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            no_scientific_source_stamp_rejected;
    return output;
  }
  receipt.scientific_authority_id = source_stamp.scientific_authority_id();
  receipt.manifest_digest = source_stamp.manifest_digest();
  receipt.source_identity_digest = source_stamp.source_identity_digest();
  receipt.horizontal_incidence_authority_digest =
      source_stamp.horizontal_incidence_authority_digest();

  if (!scientific_window.privately_attests_projection_payload()) {
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            no_scientific_window_rejected;
    return output;
  }
  const void* const source_control_identity =
      source_stamp.private_control_identity_.get();
  const void* const window_control_identity =
      scientific_window.scientific_control_identity();
  const auto scientific_window_identity =
      scientific_window.share_projection_capability_identity();
  if (source_control_identity == nullptr ||
      window_control_identity != source_control_identity ||
      scientific_window_identity == nullptr ||
      scientific_window.projection_capability_identity() !=
          scientific_window_identity.get()) {
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            no_private_source_or_window_identity_mismatch;
    return output;
  }

  const auto& owned = scientific_window.owned_window();
  const auto& plan = owned.local_plan;
  if (owned.manifest_digest != source_stamp.manifest_digest() ||
      owned.source_batch_index >= source_stamp.batch_count() ||
      plan.batches.size() != 1U ||
      plan.facet_tokens.size() !=
          owned.local_to_stable_facet_token_indices.size() ||
      !owned.exact_window_copied || !owned.no_future_payload_owned) {
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            no_public_source_or_batch_identity_mismatch;
    return output;
  }
  const auto& batch = plan.batches.front();
  if (batch.batch_index != owned.source_batch_index ||
      batch.future_snapshot_index != owned.source_batch_index ||
      batch.order < 2U ||
      batch.order > direct_sparse_positive_facet_maximum_point_count ||
      batch.direct_reference_offset != 0U ||
      batch.direct_reference_count != plan.direct_references.size() ||
      batch.residual_reference_offset != 0U ||
      batch.residual_reference_count != plan.residual_references.size() ||
      batch.coface_facet_reference_offset != 0U ||
      batch.coface_facet_reference_count !=
          plan.coface_facet_references.size() ||
      (owned.source_batch_index == 0U &&
       owned.source_chain_digest != source_stamp.initial_chain_digest()) ||
      (owned.source_batch_index + 1U == source_stamp.batch_count() &&
       owned.successor_chain_digest != source_stamp.final_chain_digest())) {
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            no_public_source_or_batch_identity_mismatch;
    return output;
  }

  receipt.source_chain_digest = owned.source_chain_digest;
  receipt.batch_identity_digest = owned.batch_identity_digest;
  receipt.successor_chain_digest = owned.successor_chain_digest;
  receipt.source_batch_index = owned.source_batch_index;
  receipt.source_future_snapshot_index = batch.future_snapshot_index;
  receipt.order = batch.order;
  receipt.exact_squared_radius = batch.squared_level;
  receipt.required_window_facet_scan_count = plan.facet_tokens.size();
  receipt.required_direct_reference_scan_count =
      plan.direct_references.size();
  receipt.required_residual_reference_scan_count =
      plan.residual_references.size();
  receipt.required_coface_facet_reference_scan_count =
      plan.coface_facet_references.size();
  receipt.required_contribution_atom_count =
      plan.coface_facet_references.size();

  std::size_t binding_capacity = 0U;
  if (!checked_multiply(
          receipt.required_window_facet_scan_count,
          direct_sparse_positive_facet_maximum_point_count,
          receipt.required_window_facet_point_scan_count) ||
      !checked_add(
          receipt.required_direct_reference_scan_count,
          receipt.required_residual_reference_scan_count,
          binding_capacity) ||
      !checked_add(
          receipt.required_window_facet_scan_count,
          binding_capacity,
          receipt.required_scratch_entry_count) ||
      !checked_add(
          receipt.required_scratch_entry_count,
          receipt.required_coface_facet_reference_scan_count,
          receipt.required_scratch_entry_count)) {
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            no_projection_capacity_overflow;
    return output;
  }

  // No input payload is traversed and no projection-sized arena is allocated
  // until all structurally known scans, scratch entries and atom outputs have
  // been checked.  Definition and CSR sizes become exact after the bounded
  // coface scan and are capped before their output arenas are allocated.
  if (receipt.required_window_facet_scan_count >
          budget.maximum_window_facet_scan_count ||
      receipt.required_window_facet_point_scan_count >
          budget.maximum_window_facet_point_scan_count ||
      receipt.required_direct_reference_scan_count >
          budget.maximum_direct_reference_scan_count ||
      receipt.required_residual_reference_scan_count >
          budget.maximum_residual_reference_scan_count ||
      receipt.required_coface_facet_reference_scan_count >
          budget.maximum_coface_facet_reference_scan_count ||
      receipt.required_contribution_atom_count >
          budget.maximum_contribution_atom_count ||
      receipt.required_scratch_entry_count >
          budget.maximum_scratch_entry_count) {
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            no_projection_budget_exhausted;
    return output;
  }

  try {
    std::vector<TokenUseCount> token_uses(plan.facet_tokens.size());
    std::vector<SourceCofaceBinding> bindings;
    bindings.reserve(binding_capacity);
    std::vector<std::size_t> touched_local_facet_indices;
    touched_local_facet_indices.reserve(plan.coface_facet_references.size());
    std::vector<ExactDirectProjectableContributionAtom> atoms;
    atoms.reserve(plan.coface_facet_references.size());

    contract::CanonicalSha256Builder batch_digest_builder;
    batch_digest_builder.update(resident_batch_identity_domain);
    append_id(batch_digest_builder, source_stamp.source_identity_digest());
    append_size(batch_digest_builder, owned.source_batch_index);
    append_size(batch_digest_builder, batch.future_snapshot_index);
    append_level(batch_digest_builder, batch.squared_level);
    append_size(batch_digest_builder, batch.order);
    append_bool(batch_digest_builder, true);
    append_bool(batch_digest_builder, true);
    append_bool(batch_digest_builder, true);
    append_bool(batch_digest_builder, true);
    append_bool(batch_digest_builder, true);
    append_bool(batch_digest_builder, true);
    append_bool(batch_digest_builder, false);
    append_size(batch_digest_builder, plan.facet_tokens.size());

    std::size_t facet_point_count = 0U;
    for (std::size_t local = 0U; local < plan.facet_tokens.size(); ++local) {
      const auto& token = plan.facet_tokens[local];
      const auto& key = token.facet_key;
      const std::size_t stable =
          owned.local_to_stable_facet_token_indices[local];
      if (token.facet_token_index != local ||
          token.source_star_facet_token_index.has_value() ||
          stable >= source_stamp.stable_facet_token_count() ||
          (local != 0U &&
           owned.local_to_stable_facet_token_indices[local - 1U] >= stable) ||
          key.point_count != batch.order) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }
      for (std::size_t point_index = 0U;
           point_index < key.point_ids.size();
           ++point_index) {
        if ((point_index < key.point_count && point_index != 0U &&
             key.point_ids[point_index - 1U] >= key.point_ids[point_index]) ||
            (point_index >= key.point_count && key.point_ids[point_index] != 0U)) {
          receipt.decision =
              ExactDirectProjectableContributionWindowDecision::
                  contradiction_authenticated_window_payload;
          return output;
        }
      }
      if (!checked_add(facet_point_count, key.point_count, facet_point_count)) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                no_projection_capacity_overflow;
        return output;
      }
      append_size(batch_digest_builder, local);
      append_size(batch_digest_builder, stable);
      append_facet_key(batch_digest_builder, key);
    }

    append_size(batch_digest_builder, plan.direct_references.size());
    std::size_t direct_birth_count = 0U;
    for (std::size_t index = 0U;
         index < plan.direct_references.size();
         ++index) {
      const auto& reference = plan.direct_references[index];
      if (index != 0U &&
          plan.direct_references[index - 1U].direct_reference_index >=
              reference.direct_reference_index) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }
      if (reference.role == ExactDirectMorseH0Role::birth) {
        if (reference.source_incidence_family_index.has_value() ||
            reference.source_star_direct_coface_index.has_value() ||
            !reference.direct_birth_facet_token_index.has_value() ||
            *reference.direct_birth_facet_token_index >= token_uses.size() ||
            !checked_increment(direct_birth_count) ||
            !checked_increment(
                token_uses[*reference.direct_birth_facet_token_index]
                    .direct_birth_count)) {
          receipt.decision =
              ExactDirectProjectableContributionWindowDecision::
                  contradiction_authenticated_window_payload;
          return output;
        }
      } else if (reference.role == ExactDirectMorseH0Role::saddle) {
        if (!reference.source_incidence_family_index.has_value() ||
            !reference.source_star_direct_coface_index.has_value() ||
            reference.direct_birth_facet_token_index.has_value()) {
          receipt.decision =
              ExactDirectProjectableContributionWindowDecision::
                  contradiction_authenticated_window_payload;
          return output;
        }
        bindings.push_back(
            {ExactDirectProjectableContributionSourceKind::direct_saddle,
             *reference.source_star_direct_coface_index,
             reference.direct_reference_index,
             reference.source_role_record_index,
             reference.source_event_projection_index,
             reference.source_incidence_family_index});
        if (!checked_increment(receipt.source_direct_coface_count)) {
          receipt.decision =
              ExactDirectProjectableContributionWindowDecision::
                  no_projection_capacity_overflow;
          return output;
        }
      } else {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }
      append_direct_reference(batch_digest_builder, reference);
    }

    append_size(batch_digest_builder, plan.residual_references.size());
    for (std::size_t index = 0U;
         index < plan.residual_references.size();
         ++index) {
      const auto& reference = plan.residual_references[index];
      if (index != 0U &&
          plan.residual_references[index - 1U].residual_reference_index >=
              reference.residual_reference_index) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }
      bindings.push_back(
          {ExactDirectProjectableContributionSourceKind::residual_incidence,
           reference.source_star_coface_index,
           reference.residual_reference_index,
           std::nullopt,
           std::nullopt,
           std::nullopt});
      if (!checked_increment(receipt.source_residual_coface_count)) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                no_projection_capacity_overflow;
        return output;
      }
      append_residual_reference(batch_digest_builder, reference);
    }

    std::sort(bindings.begin(), bindings.end(), binding_less);
    if (std::adjacent_find(
            bindings.begin(),
            bindings.end(),
            [](const SourceCofaceBinding& left,
               const SourceCofaceBinding& right) {
              return left.source_coface_index == right.source_coface_index;
            }) != bindings.end()) {
      receipt.decision =
          ExactDirectProjectableContributionWindowDecision::
              contradiction_authenticated_window_payload;
      return output;
    }

    std::size_t coface_point_count = 0U;
    std::size_t expected_coface_reference_count = 0U;
    if (!checked_add(batch.order, 1U, coface_point_count) ||
        !checked_multiply(
            bindings.size(),
            coface_point_count,
            expected_coface_reference_count)) {
      receipt.decision =
          ExactDirectProjectableContributionWindowDecision::
              no_projection_capacity_overflow;
      return output;
    }
    if (expected_coface_reference_count !=
        plan.coface_facet_references.size()) {
      receipt.decision =
          ExactDirectProjectableContributionWindowDecision::
              contradiction_authenticated_window_payload;
      return output;
    }

    append_size(
        batch_digest_builder, plan.coface_facet_references.size());
    std::optional<std::size_t> current_source_coface;
    std::array<bool, 11U> removed_positions{};
    std::array<spatial::PointId, 11U> current_coface_points{};
    std::size_t current_group_reference_count = 0U;
    std::size_t observed_coface_group_count = 0U;
    bool current_coface_points_bound = false;

    const auto complete_current_group = [&]() noexcept {
      return current_source_coface.has_value() &&
             current_group_reference_count == coface_point_count &&
             std::all_of(
                 removed_positions.begin(),
                 removed_positions.begin() +
                     static_cast<std::ptrdiff_t>(coface_point_count),
                 [](bool value) { return value; });
    };

    for (std::size_t index = 0U;
         index < plan.coface_facet_references.size();
         ++index) {
      const auto& reference = plan.coface_facet_references[index];
      if (index != 0U &&
          plan.coface_facet_references[index - 1U]
                  .coface_facet_reference_index >=
              reference.coface_facet_reference_index) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }

      if (!current_source_coface.has_value() ||
          *current_source_coface != reference.source_star_coface_index) {
        if (current_source_coface.has_value()) {
          if (!complete_current_group() ||
              reference.source_star_coface_index <= *current_source_coface ||
              !checked_increment(observed_coface_group_count)) {
            receipt.decision =
                ExactDirectProjectableContributionWindowDecision::
                    contradiction_authenticated_window_payload;
            return output;
          }
        }
        current_source_coface = reference.source_star_coface_index;
        removed_positions.fill(false);
        current_coface_points.fill(0U);
        current_group_reference_count = 0U;
        current_coface_points_bound = false;
      }

      const auto binding = std::lower_bound(
          bindings.begin(),
          bindings.end(),
          reference.source_star_coface_index,
          [](const SourceCofaceBinding& candidate, std::size_t value) {
            return candidate.source_coface_index < value;
          });
      if (binding == bindings.end() ||
          binding->source_coface_index != reference.source_star_coface_index ||
          reference.removed_union_point_index >= coface_point_count ||
          removed_positions[reference.removed_union_point_index] ||
          reference.facet_token_index >= plan.facet_tokens.size()) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }
      const auto& key =
          plan.facet_tokens[reference.facet_token_index].facet_key;
      if (std::binary_search(
              key.point_ids.begin(),
              key.point_ids.begin() +
                  static_cast<std::ptrdiff_t>(key.point_count),
              reference.removed_point_id)) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }

      std::array<spatial::PointId, 11U> reconstructed{};
      std::size_t destination = 0U;
      bool removed_inserted = false;
      for (std::size_t key_index = 0U;
           key_index < key.point_count;
           ++key_index) {
        if (!removed_inserted &&
            reference.removed_point_id < key.point_ids[key_index]) {
          reconstructed[destination++] = reference.removed_point_id;
          removed_inserted = true;
        }
        reconstructed[destination++] = key.point_ids[key_index];
      }
      if (!removed_inserted) {
        reconstructed[destination++] = reference.removed_point_id;
      }
      if (destination != coface_point_count ||
          reconstructed[reference.removed_union_point_index] !=
              reference.removed_point_id ||
          (current_coface_points_bound &&
           reconstructed != current_coface_points)) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }
      if (!current_coface_points_bound) {
        current_coface_points = reconstructed;
        current_coface_points_bound = true;
      }
      removed_positions[reference.removed_union_point_index] = true;
      if (!checked_increment(current_group_reference_count) ||
          !checked_increment(
              token_uses[reference.facet_token_index]
                  .coface_deletion_count)) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                no_projection_capacity_overflow;
        return output;
      }

      const std::size_t stable_facet_token_index =
          owned.local_to_stable_facet_token_indices
              [reference.facet_token_index];
      atoms.push_back(
          {0U,
           binding->kind,
           binding->source_coface_index,
           binding->source_reference_index,
           binding->source_role_record_index,
           binding->source_event_projection_index,
           binding->source_incidence_family_index,
           reference.coface_facet_reference_index,
           reference.removed_union_point_index,
           reference.removed_point_id,
           stable_facet_token_index,
           0U,
           batch.order,
           batch.squared_level});
      touched_local_facet_indices.push_back(reference.facet_token_index);
      if (binding->kind ==
          ExactDirectProjectableContributionSourceKind::direct_saddle) {
        if (!checked_increment(receipt.direct_contribution_atom_count)) {
          receipt.decision =
              ExactDirectProjectableContributionWindowDecision::
                  no_projection_capacity_overflow;
          return output;
        }
      } else if (!checked_increment(
                     receipt.residual_contribution_atom_count)) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                no_projection_capacity_overflow;
        return output;
      }
      append_coface_facet_reference(batch_digest_builder, reference);
    }

    if ((!bindings.empty() &&
         (!complete_current_group() ||
          !checked_increment(observed_coface_group_count))) ||
        (bindings.empty() && current_source_coface.has_value()) ||
        observed_coface_group_count != bindings.size()) {
      receipt.decision =
          ExactDirectProjectableContributionWindowDecision::
              contradiction_authenticated_window_payload;
      return output;
    }

    for (std::size_t local = 0U; local < token_uses.size(); ++local) {
      if (token_uses[local].direct_birth_count !=
              plan.facet_tokens[local].direct_birth_reference_count ||
          token_uses[local].coface_deletion_count !=
              plan.facet_tokens[local].coface_deletion_reference_count ||
          (token_uses[local].direct_birth_count == 0U &&
           token_uses[local].coface_deletion_count == 0U)) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }
    }

    std::size_t expected_logical_storage = plan.facet_tokens.size();
    for (const std::size_t increment : {
             facet_point_count,
             plan.direct_references.size(),
             plan.residual_references.size(),
             plan.coface_facet_references.size(),
             plan.batches.size()}) {
      if (!checked_add(
              expected_logical_storage,
              increment,
              expected_logical_storage)) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                no_projection_capacity_overflow;
        return output;
      }
    }
    if (plan.required_distinct_facet_token_count !=
            plan.facet_tokens.size() ||
        plan.required_facet_key_point_count != facet_point_count ||
        plan.required_batch_count != 1U ||
        plan.required_direct_reference_count != plan.direct_references.size() ||
        plan.required_residual_reference_count !=
            plan.residual_references.size() ||
        plan.required_coface_deletion_reference_count !=
            plan.coface_facet_references.size() ||
        plan.required_direct_birth_reference_count != direct_birth_count ||
        plan.required_direct_saddle_reference_count !=
            receipt.source_direct_coface_count ||
        plan.logical_storage_entry_count != expected_logical_storage ||
        !plan.no_partial_scientific_payload_published ||
        !plan.no_k_plus_one_coface_key_persisted ||
        !plan.no_global_facet_or_coface_catalog_materialized ||
        !plan.partial_refinement_only || plan.public_status_claimed ||
        plan.decision !=
            ExactDirectSparseUnifiedLevelPlanDecision::not_certified ||
        plan.scope != ExactDirectSparseUnifiedLevelPlanScope::unspecified) {
      receipt.decision =
          ExactDirectProjectableContributionWindowDecision::
              contradiction_authenticated_window_payload;
      return output;
    }

    const auto recomputed_batch_digest = batch_digest_builder.finalize();
    if (recomputed_batch_digest != owned.batch_identity_digest ||
        compute_successor_chain_digest(
            owned.source_chain_digest, recomputed_batch_digest) !=
            owned.successor_chain_digest) {
      receipt.decision =
          ExactDirectProjectableContributionWindowDecision::
              contradiction_batch_or_chain_digest;
      return output;
    }

    std::sort(
        touched_local_facet_indices.begin(),
        touched_local_facet_indices.end());
    touched_local_facet_indices.erase(
        std::unique(
            touched_local_facet_indices.begin(),
            touched_local_facet_indices.end()),
        touched_local_facet_indices.end());
    receipt.required_touched_facet_definition_count =
        touched_local_facet_indices.size();
    for (const std::size_t local : touched_local_facet_indices) {
      if (!checked_add(
              receipt.required_facet_point_reference_count,
              plan.facet_tokens[local].facet_key.point_count,
              receipt.required_facet_point_reference_count)) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                no_projection_capacity_overflow;
        return output;
      }
    }
    if (receipt.required_touched_facet_definition_count >
            budget.maximum_touched_facet_definition_count ||
        receipt.required_facet_point_reference_count >
            budget.maximum_facet_point_reference_count) {
      receipt.decision =
          ExactDirectProjectableContributionWindowDecision::
              no_projection_budget_exhausted;
      return output;
    }

    auto impl = std::make_unique<
        ExactDirectProjectableContributionWindowResult::Impl>();
    impl->facet_definitions.reserve(
        receipt.required_touched_facet_definition_count);
    impl->facet_point_references.reserve(
        receipt.required_facet_point_reference_count);
    for (const std::size_t local : touched_local_facet_indices) {
      const auto& key = plan.facet_tokens[local].facet_key;
      const std::size_t definition_index = impl->facet_definitions.size();
      const std::size_t point_offset = impl->facet_point_references.size();
      impl->facet_definitions.push_back(
          {definition_index,
           owned.local_to_stable_facet_token_indices[local],
           batch.order,
           point_offset,
           key.point_count});
      impl->facet_point_references.insert(
          impl->facet_point_references.end(),
          key.point_ids.begin(),
          key.point_ids.begin() +
              static_cast<std::ptrdiff_t>(key.point_count));
    }

    std::sort(atoms.begin(), atoms.end(), atom_less);
    for (std::size_t index = 0U; index < atoms.size(); ++index) {
      auto& atom = atoms[index];
      atom.contribution_atom_index = index;
      const auto definition = std::lower_bound(
          impl->facet_definitions.begin(),
          impl->facet_definitions.end(),
          atom.stable_source_facet_token_index,
          [](const ExactDirectProjectableContributionFacetDefinition& item,
             std::size_t stable_index) {
            return item.stable_source_facet_token_index < stable_index;
          });
      if (definition == impl->facet_definitions.end() ||
          definition->stable_source_facet_token_index !=
              atom.stable_source_facet_token_index) {
        receipt.decision =
            ExactDirectProjectableContributionWindowDecision::
                contradiction_authenticated_window_payload;
        return output;
      }
      atom.facet_definition_index =
          static_cast<std::size_t>(
              definition - impl->facet_definitions.begin());
    }
    impl->contribution_atoms = std::move(atoms);

    receipt.contribution_window_digest = compute_contribution_window_digest(
        receipt,
        impl->facet_definitions,
        impl->facet_point_references,
        impl->contribution_atoms);

    // Freshly recheck both private seams after every payload traversal.  The
    // result retains both process-local capabilities, but never exposes them.
    if (!source_stamp.certified_scientific_source_stamp() ||
        source_stamp.private_control_identity_.get() !=
            source_control_identity ||
        !scientific_window.privately_attests_projection_payload() ||
        scientific_window.scientific_control_identity() !=
            source_control_identity ||
        scientific_window.projection_capability_identity() !=
            scientific_window_identity.get() ||
        owned.batch_identity_digest != receipt.batch_identity_digest ||
        owned.source_chain_digest != receipt.source_chain_digest ||
        owned.successor_chain_digest != receipt.successor_chain_digest) {
      receipt.decision =
          ExactDirectProjectableContributionWindowDecision::
              no_private_source_or_window_identity_mismatch;
      return output;
    }

    receipt.private_capability_check_count = 4U;
    receipt.authenticated_payload_full_replay_count = 1U;
    receipt.scientific_source_stamp_bound = true;
    receipt.scientific_window_private_identity_bound = true;
    receipt.manifest_source_batch_and_chain_bound = true;
    receipt.complete_authenticated_window_payload_replayed = true;
    receipt.every_coface_facet_reference_projected_once = true;
    receipt.direct_and_residual_cofaces_retained = true;
    receipt.source_records_not_filtered_by_h0_node_presence = true;
    receipt.touched_facet_definitions_canonical_and_deduplicated = true;
    receipt.facet_point_csr_canonical = true;
    receipt.every_atom_retains_exact_batch_squared_radius = true;
    receipt.source_session_window_forest_or_ledger_mutated = false;
    receipt.global_facet_coface_incidence_or_gamma_catalog_materialized = false;
    receipt.ordinary_or_higher_order_delaunay_materialized = false;
    receipt.contribution_aggregation_performed = false;
    receipt.source_exactness_claimed = false;
    receipt.vertical_maps_complete = false;
    receipt.durable_restart_supported = false;
    receipt.public_status_claimed = false;
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            complete_exhaustive_authenticated_window_projection;
    receipt.scope =
        ExactDirectProjectableContributionWindowScope::
            exhaustive_projection_of_one_already_authenticated_scientific_window_only;

    impl->attested_receipt = receipt;
    impl->source_control_identity = source_stamp.private_control_identity_;
    impl->scientific_window_identity = scientific_window_identity;
    output.impl_ = std::move(impl);
    if (!output.certified_exhaustive_projection()) {
      output.impl_.reset();
      receipt.decision =
          ExactDirectProjectableContributionWindowDecision::
              contradiction_authenticated_window_payload;
      receipt.scope =
          ExactDirectProjectableContributionWindowScope::unspecified;
    }
    return output;
  } catch (const std::bad_alloc&) {
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            no_projection_allocation_failed;
    return output;
  } catch (...) {
    receipt.decision =
        ExactDirectProjectableContributionWindowDecision::
            contradiction_authenticated_window_payload;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
