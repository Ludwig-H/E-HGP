#include "morsehgp3d/hierarchy/direct_saddle_arm_seed_journal.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] std::optional<std::size_t> checked_add(
    std::size_t left,
    std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return std::nullopt;
  }
  return left + right;
}

[[nodiscard]] std::optional<std::size_t> checked_multiply(
    std::size_t left,
    std::size_t right) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return std::nullopt;
  }
  return left * right;
}

[[nodiscard]] std::optional<std::size_t> checked_linear_bound(
    std::size_t point_coefficient,
    std::size_t point_count,
    std::size_t event_coefficient,
    std::size_t event_count) {
  const auto point_term = checked_multiply(point_coefficient, point_count);
  const auto event_term = checked_multiply(event_coefficient, event_count);
  if (!point_term.has_value() || !event_term.has_value()) {
    return std::nullopt;
  }
  return checked_add(*point_term, *event_term);
}

void append_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    const std::size_t shift = (bytes.size() - 1U - index) * 8U;
    bytes[index] = static_cast<std::uint8_t>(value >> shift);
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view text) {
  append_size(builder, text.size());
  builder.update(text);
}

void append_optional_size(
    contract::CanonicalSha256Builder& builder,
    const std::optional<std::size_t>& value) {
  append_u64(builder, value.has_value() ? 1U : 0U);
  if (value.has_value()) {
    append_size(builder, *value);
  }
}

[[nodiscard]] contract::CanonicalId source_event_arm_identity_digest(
    const ExactDirectSupportEvent& event) {
  contract::CanonicalSha256Builder builder;
  append_text(
      builder,
      "MorseHGP3D/phase10/direct-saddle-arm/source-event-identity/v1");
  append_size(builder, event.event_index);
  append_size(builder, static_cast<std::size_t>(event.support_size));
  for (const spatial::PointId point_id : event.support_ids) {
    append_u64(builder, static_cast<std::uint64_t>(point_id));
  }
  append_size(builder, event.interior_ids.size());
  for (const spatial::PointId point_id : event.interior_ids) {
    append_u64(builder, static_cast<std::uint64_t>(point_id));
  }
  append_text(builder, event.squared_level.numerator_string());
  append_text(builder, event.squared_level.denominator_string());
  append_size(builder, event.closed_rank);
  append_size(builder, event.exterior_count);
  append_optional_size(builder, event.birth_order);
  append_optional_size(builder, event.saddle_order);
  return builder.finalize();
}

[[nodiscard]] bool source_facade_authority_matches_seed_journal(
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal) {
  return seed_journal.certified_partial_refinement() &&
         source_facade.terminal_catalog_certified() &&
         source_facade.certificate.requirements.point_count ==
             seed_journal.point_count &&
         source_facade.events.size() ==
             seed_journal.source_direct_event_count &&
         source_facade.certificate.pair_canonical_cloud_digest ==
             seed_journal.source_pair_canonical_cloud_digest &&
         source_facade.certificate.higher_canonical_cloud_digest ==
             seed_journal.source_higher_canonical_cloud_digest &&
         source_facade.certificate.pair_semantic_digest ==
             seed_journal.source_pair_semantic_digest &&
         source_facade.certificate.higher_semantic_digest ==
             seed_journal.source_higher_semantic_digest;
}

[[nodiscard]] bool same_support(
    const ExactDirectMorseEventProjection& projection,
    const ExactDirectSupportEvent& event) {
  return projection.support_size == event.support_size &&
         projection.support_ids == event.support_ids;
}

[[nodiscard]] ExactDirectSaddleArmFacet reconstruct_facet(
    const ExactDirectSupportEvent& event,
    spatial::PointId removed_support_point_id) {
  ExactDirectSaddleArmFacet facet;
  const std::size_t support_size =
      static_cast<std::size_t>(event.support_size);
  if (support_size < 2U || support_size > 4U ||
      event.closed_rank < 2U || event.closed_rank > 11U ||
      event.closed_rank != event.interior_ids.size() + support_size) {
    throw std::invalid_argument(
        "a direct saddle arm source has an unsupported rank or support");
  }

  bool removed_found = false;
  for (const spatial::PointId point_id : event.interior_ids) {
    if (facet.point_count >= facet.point_ids.size()) {
      throw std::length_error(
          "a direct saddle arm facet exceeds ten points");
    }
    facet.point_ids[facet.point_count] = point_id;
    ++facet.point_count;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    const spatial::PointId point_id = event.support_ids[index];
    if (point_id == removed_support_point_id) {
      if (removed_found) {
        throw std::invalid_argument(
            "a removed support PointId occurs more than once");
      }
      removed_found = true;
      continue;
    }
    if (facet.point_count >= facet.point_ids.size()) {
      throw std::length_error(
          "a direct saddle arm facet exceeds ten points");
    }
    facet.point_ids[facet.point_count] = point_id;
    ++facet.point_count;
  }
  if (!removed_found || facet.point_count + 1U != event.closed_rank) {
    throw std::invalid_argument(
        "a direct saddle arm did not remove exactly one support point");
  }

  std::sort(
      facet.point_ids.begin(),
      facet.point_ids.begin() +
          static_cast<std::ptrdiff_t>(facet.point_count));
  const auto unique_end = std::unique(
      facet.point_ids.begin(),
      facet.point_ids.begin() +
          static_cast<std::ptrdiff_t>(facet.point_count));
  if (unique_end !=
      facet.point_ids.begin() +
          static_cast<std::ptrdiff_t>(facet.point_count)) {
    throw std::invalid_argument(
        "a direct saddle arm facet contains duplicate PointIds");
  }
  return facet;
}

void clear_payload(ExactDirectSaddleArmSeedJournalResult& result) {
  result.families.clear();
  result.arm_seeds.clear();
  result.logical_added_storage_entry_count = 0U;
  result.combined_logical_storage_entry_count = 0U;
}

[[nodiscard]] bool budget_is_sufficient(
    const ExactDirectSaddleArmSeedJournalResult& result) {
  return result.required_source_journal_replay_entry_count <=
             result.requested_budget
                 .maximum_source_journal_replay_entry_count &&
         result.required_role_scan_count <=
             result.requested_budget.maximum_role_scan_count &&
         result.required_saddle_family_count <=
             result.requested_budget.maximum_saddle_family_count &&
         result.required_arm_seed_count <=
             result.requested_budget.maximum_arm_seed_count;
}

[[nodiscard]] bool source_join_matches(
    const ExactDirectMorseH0RoleRecord& role,
    const ExactDirectMorseH0Batch& batch,
    const ExactDirectMorseEventProjection& projection,
    const ExactDirectSupportEvent& event) {
  return role.role == ExactDirectMorseH0Role::saddle &&
         role.batch_index == batch.batch_index &&
         role.event_projection_index ==
             projection.event_projection_index &&
         projection.source ==
             ExactDirectMorseEventSource::direct_support_terminal_event &&
         projection.source_index == event.event_index &&
         same_support(projection, event) &&
         projection.squared_level == event.squared_level &&
         projection.closed_rank == event.closed_rank &&
         projection.saddle_order == event.saddle_order &&
         event.saddle_order.has_value() &&
         batch.order == *event.saddle_order &&
         batch.squared_level == event.squared_level;
}

}  // namespace

bool ExactDirectSaddleArmSeedJournalResult::certified_partial_refinement()
    const {
  return schema_version == direct_saddle_arm_seed_journal_schema_version &&
         required_saddle_family_count == families.size() &&
         required_arm_seed_count == arm_seeds.size() &&
         logical_added_storage_entry_count ==
             families.size() + arm_seeds.size() &&
         logical_added_storage_entry_count <=
             logical_added_storage_entry_limit &&
         combined_logical_storage_entry_count <=
             combined_logical_storage_entry_limit &&
         budget_preflight_certified &&
         source_journal_freshly_replayed &&
         source_facade_join_certified &&
         every_saddle_role_projected_once &&
         every_support_point_has_one_arm_seed &&
         arm_seed_order_is_canonical &&
         factorized_facets_reconstruct_exactly &&
         source_relative_strict_miniball_drop_theorem_applies &&
         output_linear_in_direct_events &&
         no_forbidden_global_structure_materialized &&
         !facets_materialized_in_journal &&
         !miniballs_or_global_partitions_computed &&
         !hierarchy_reduction_performed &&
         !forest_or_gateway_attach_performed && !public_status_claimed &&
         partial_refinement_only &&
         decision == ExactDirectSaddleArmSeedDecision::
                         complete_certified_factorized_arm_seeds &&
         scope == ExactDirectSaddleArmSeedScope::
                      terminal_direct_saddle_deleted_facets_only;
}

ExactDirectSaddleArmSeedJournalResult
build_exact_direct_saddle_arm_seed_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& budget) {
  ExactDirectSaddleArmSeedJournalResult result;
  result.requested_budget = budget;
  result.point_count = cloud.size();
  result.source_direct_event_count = source_facade.events.size();
  result.source_pair_canonical_cloud_digest =
      source_facade.certificate.pair_canonical_cloud_digest;
  result.source_higher_canonical_cloud_digest =
      source_facade.certificate.higher_canonical_cloud_digest;
  result.source_pair_semantic_digest =
      source_facade.certificate.pair_semantic_digest;
  result.source_higher_semantic_digest =
      source_facade.certificate.higher_semantic_digest;
  result.scope = ExactDirectSaddleArmSeedScope::
      terminal_direct_saddle_deleted_facets_only;

  const auto replay_bound = checked_linear_bound(
      3U, cloud.size(), 5U, source_facade.events.size());
  const auto added_bound =
      checked_multiply(5U, source_facade.events.size());
  const auto combined_bound = checked_linear_bound(
      3U, cloud.size(), 10U, source_facade.events.size());
  if (!replay_bound.has_value() || !added_bound.has_value() ||
      !combined_bound.has_value()) {
    result.decision = ExactDirectSaddleArmSeedDecision::
        no_seed_journal_capacity_overflow;
    return result;
  }
  result.required_source_journal_replay_entry_count = *replay_bound;
  result.logical_added_storage_entry_limit = *added_bound;
  result.combined_logical_storage_entry_limit = *combined_bound;

  std::size_t required_roles = cloud.size();
  std::size_t required_families = 0U;
  std::size_t required_arms = 0U;
  for (const ExactDirectSupportEvent& event : source_facade.events) {
    const std::size_t support_size =
        static_cast<std::size_t>(event.support_size);
    if (support_size < 2U || support_size > 4U) {
      result.decision = ExactDirectSaddleArmSeedDecision::
          no_seed_journal_source_not_certified;
      return result;
    }
    if (event.birth_order.has_value()) {
      const auto next = checked_add(required_roles, 1U);
      if (!next.has_value()) {
        result.decision = ExactDirectSaddleArmSeedDecision::
            no_seed_journal_capacity_overflow;
        return result;
      }
      required_roles = *next;
    }
    if (event.saddle_order.has_value()) {
      const auto next_roles = checked_add(required_roles, 1U);
      const auto next_families = checked_add(required_families, 1U);
      const auto next_arms = checked_add(required_arms, support_size);
      if (!next_roles.has_value() || !next_families.has_value() ||
          !next_arms.has_value()) {
        result.decision = ExactDirectSaddleArmSeedDecision::
            no_seed_journal_capacity_overflow;
        return result;
      }
      required_roles = *next_roles;
      required_families = *next_families;
      required_arms = *next_arms;
    }
  }
  result.required_role_scan_count = required_roles;
  result.required_saddle_family_count = required_families;
  result.required_arm_seed_count = required_arms;

  if (!budget_is_sufficient(result)) {
    result.decision = ExactDirectSaddleArmSeedDecision::
        no_seed_journal_budget_exhausted;
    return result;
  }
  result.budget_preflight_certified = true;

  const ExactDirectMorseEventJournalStreamingVerification
      source_verification =
          verify_exact_direct_morse_event_journal_streaming(
              cloud, source_facade, source_journal);
  if (!source_verification.result_certified ||
      !source_journal.certified_partial_refinement()) {
    result.decision = ExactDirectSaddleArmSeedDecision::
        no_seed_journal_source_not_certified;
    return result;
  }
  result.source_journal_freshly_replayed = true;

  result.families.reserve(required_families);
  result.arm_seeds.reserve(required_arms);
  try {
    for (const ExactDirectMorseH0RoleRecord& role :
         source_journal.role_records) {
      if (role.role != ExactDirectMorseH0Role::saddle) {
        continue;
      }
      if (role.batch_index >= source_journal.batches.size() ||
          role.event_projection_index >=
              source_journal.event_projections.size()) {
        throw std::invalid_argument(
            "a direct saddle role references an absent journal record");
      }
      const ExactDirectMorseH0Batch& batch =
          source_journal.batches[role.batch_index];
      const ExactDirectMorseEventProjection& projection =
          source_journal.event_projections[role.event_projection_index];
      if (projection.source_index >= source_facade.events.size()) {
        throw std::invalid_argument(
            "a direct saddle projection references an absent source event");
      }
      const ExactDirectSupportEvent& event =
          source_facade.events[projection.source_index];
      if (!source_join_matches(role, batch, projection, event)) {
        throw std::invalid_argument(
            "a direct saddle role disagrees with its Phase-9 source event");
      }

      ExactDirectSaddleArmFamilyRecord family;
      family.family_index = result.families.size();
      family.source_event_index = event.event_index;
      family.journal_event_projection_index =
          projection.event_projection_index;
      family.journal_role_record_index = role.role_record_index;
      family.journal_batch_index = batch.batch_index;
      family.order = batch.order;
      family.critical_squared_level = batch.squared_level;
      family.arm_seed_offset = result.arm_seeds.size();
      family.arm_seed_count =
          static_cast<std::size_t>(event.support_size);
      family.source_event_arm_identity_digest =
          source_event_arm_identity_digest(event);
      result.families.push_back(family);

      for (std::size_t support_index = 0U;
           support_index < family.arm_seed_count;
           ++support_index) {
        ExactDirectSaddleArmSeedRecord seed;
        seed.arm_seed_index = result.arm_seeds.size();
        seed.family_index = family.family_index;
        seed.removed_support_point_id = event.support_ids[support_index];
        const ExactDirectSaddleArmFacet facet =
            reconstruct_facet(event, seed.removed_support_point_id);
        if (facet.point_count != family.order) {
          throw std::logic_error(
              "a factorized direct saddle arm has the wrong order");
        }
        result.arm_seeds.push_back(seed);
      }
    }
  } catch (const std::logic_error&) {
    clear_payload(result);
    result.decision = ExactDirectSaddleArmSeedDecision::
        no_seed_journal_source_join_inconsistent;
    return result;
  }

  if (result.families.size() != required_families ||
      result.arm_seeds.size() != required_arms) {
    clear_payload(result);
    result.decision = ExactDirectSaddleArmSeedDecision::
        no_seed_journal_source_join_inconsistent;
    return result;
  }

  result.logical_added_storage_entry_count =
      result.families.size() + result.arm_seeds.size();
  const auto combined_count = checked_add(
      source_journal.logical_linear_storage_entry_count,
      result.logical_added_storage_entry_count);
  if (!combined_count.has_value()) {
    clear_payload(result);
    result.decision = ExactDirectSaddleArmSeedDecision::
        no_seed_journal_capacity_overflow;
    return result;
  }
  result.combined_logical_storage_entry_count = *combined_count;

  result.source_facade_join_certified = true;
  result.every_saddle_role_projected_once = true;
  result.every_support_point_has_one_arm_seed = true;
  result.arm_seed_order_is_canonical = true;
  result.factorized_facets_reconstruct_exactly = true;
  result.source_relative_strict_miniball_drop_theorem_applies = true;
  result.output_linear_in_direct_events =
      result.logical_added_storage_entry_count <=
          result.logical_added_storage_entry_limit &&
      result.combined_logical_storage_entry_count <=
          result.combined_logical_storage_entry_limit;
  result.no_forbidden_global_structure_materialized = true;
  result.facets_materialized_in_journal = false;
  result.miniballs_or_global_partitions_computed = false;
  result.hierarchy_reduction_performed = false;
  result.forest_or_gateway_attach_performed = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
  result.decision = ExactDirectSaddleArmSeedDecision::
      complete_certified_factorized_arm_seeds;
  return result;
}

ExactDirectSaddleArmFacet reconstruct_exact_direct_saddle_arm_facet(
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectSaddleArmSeedJournalResult& seed_journal,
    std::size_t arm_seed_index) {
  if (!source_facade_authority_matches_seed_journal(
          source_facade, seed_journal)) {
    throw std::invalid_argument(
        "a direct saddle arm reconstruction facade has a different authority");
  }
  if (arm_seed_index >= seed_journal.arm_seeds.size()) {
    throw std::out_of_range(
        "a direct saddle arm seed index is outside the journal");
  }
  const ExactDirectSaddleArmSeedRecord& seed =
      seed_journal.arm_seeds[arm_seed_index];
  if (seed.arm_seed_index != arm_seed_index ||
      seed.family_index >= seed_journal.families.size()) {
    throw std::invalid_argument(
        "a direct saddle arm seed has inconsistent indexing");
  }
  const ExactDirectSaddleArmFamilyRecord& family =
      seed_journal.families[seed.family_index];
  if (family.family_index != seed.family_index ||
      family.source_event_index >= source_facade.events.size() ||
      arm_seed_index < family.arm_seed_offset ||
      arm_seed_index - family.arm_seed_offset >=
          family.arm_seed_count) {
    throw std::invalid_argument(
        "a direct saddle arm family has inconsistent indexing");
  }
  const ExactDirectSupportEvent& event =
      source_facade.events[family.source_event_index];
  if (event.event_index != family.source_event_index ||
      event.saddle_order != std::optional<std::size_t>{family.order} ||
      event.squared_level != family.critical_squared_level ||
      static_cast<std::size_t>(event.support_size) !=
          family.arm_seed_count ||
      source_event_arm_identity_digest(event) !=
          family.source_event_arm_identity_digest) {
    throw std::invalid_argument(
        "a direct saddle arm source event has a different identity");
  }
  ExactDirectSaddleArmFacet facet =
      reconstruct_facet(event, seed.removed_support_point_id);
  if (facet.point_count != family.order) {
    throw std::invalid_argument(
        "a reconstructed direct saddle arm has the wrong order");
  }
  return facet;
}

ExactDirectSaddleArmSeedVerification
verify_exact_direct_saddle_arm_seed_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& budget,
    const ExactDirectSaddleArmSeedJournalResult& observed) {
  ExactDirectSaddleArmSeedVerification verification;
  const ExactDirectSaddleArmSeedJournalResult expected =
      build_exact_direct_saddle_arm_seed_journal(
          cloud, source_facade, source_journal, budget);
  // A complete expected replay has already certified the source exactly
  // once.  Budget-preflight results deliberately do not trigger a second,
  // potentially large source replay merely to verify an allocation refusal.
  verification.source_journal_certified =
      expected.source_journal_freshly_replayed;
  verification.requirements_certified =
      observed.schema_version == expected.schema_version &&
      observed.requested_budget == expected.requested_budget &&
      observed.point_count == expected.point_count &&
      observed.source_direct_event_count ==
          expected.source_direct_event_count &&
      observed.required_source_journal_replay_entry_count ==
          expected.required_source_journal_replay_entry_count &&
      observed.required_role_scan_count ==
          expected.required_role_scan_count &&
      observed.required_saddle_family_count ==
          expected.required_saddle_family_count &&
      observed.required_arm_seed_count ==
          expected.required_arm_seed_count &&
      observed.source_pair_canonical_cloud_digest ==
          expected.source_pair_canonical_cloud_digest &&
      observed.source_higher_canonical_cloud_digest ==
          expected.source_higher_canonical_cloud_digest &&
      observed.source_pair_semantic_digest ==
          expected.source_pair_semantic_digest &&
      observed.source_higher_semantic_digest ==
          expected.source_higher_semantic_digest;
  verification.family_records_certified =
      observed.families == expected.families;
  verification.arm_seed_records_certified =
      observed.arm_seeds == expected.arm_seeds;

  verification.factorized_facets_certified =
      verification.family_records_certified &&
      verification.arm_seed_records_certified;
  if (verification.factorized_facets_certified) {
    try {
      for (std::size_t index = 0U;
           index < observed.arm_seeds.size();
           ++index) {
        if (reconstruct_exact_direct_saddle_arm_facet(
                source_facade, observed, index) !=
            reconstruct_exact_direct_saddle_arm_facet(
                source_facade, expected, index)) {
          verification.factorized_facets_certified = false;
          break;
        }
      }
    } catch (const std::logic_error&) {
      verification.factorized_facets_certified = false;
    }
  }

  verification.result_facts_certified =
      observed.logical_added_storage_entry_count ==
          expected.logical_added_storage_entry_count &&
      observed.logical_added_storage_entry_limit ==
          expected.logical_added_storage_entry_limit &&
      observed.combined_logical_storage_entry_count ==
          expected.combined_logical_storage_entry_count &&
      observed.combined_logical_storage_entry_limit ==
          expected.combined_logical_storage_entry_limit &&
      observed.budget_preflight_certified ==
          expected.budget_preflight_certified &&
      observed.source_journal_freshly_replayed ==
          expected.source_journal_freshly_replayed &&
      observed.source_facade_join_certified ==
          expected.source_facade_join_certified &&
      observed.every_saddle_role_projected_once ==
          expected.every_saddle_role_projected_once &&
      observed.every_support_point_has_one_arm_seed ==
          expected.every_support_point_has_one_arm_seed &&
      observed.arm_seed_order_is_canonical ==
          expected.arm_seed_order_is_canonical &&
      observed.factorized_facets_reconstruct_exactly ==
          expected.factorized_facets_reconstruct_exactly &&
      observed.source_relative_strict_miniball_drop_theorem_applies ==
          expected.source_relative_strict_miniball_drop_theorem_applies &&
      observed.output_linear_in_direct_events ==
          expected.output_linear_in_direct_events &&
      observed.no_forbidden_global_structure_materialized ==
          expected.no_forbidden_global_structure_materialized &&
      observed.facets_materialized_in_journal ==
          expected.facets_materialized_in_journal &&
      observed.miniballs_or_global_partitions_computed ==
          expected.miniballs_or_global_partitions_computed &&
      observed.hierarchy_reduction_performed ==
          expected.hierarchy_reduction_performed &&
      observed.forest_or_gateway_attach_performed ==
          expected.forest_or_gateway_attach_performed &&
      observed.public_status_claimed == expected.public_status_claimed &&
      observed.partial_refinement_only ==
          expected.partial_refinement_only;
  verification.decision_and_scope_certified =
      observed.decision == expected.decision &&
      observed.scope == expected.scope;
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.source_journal_certified &&
      verification.requirements_certified &&
      verification.family_records_certified &&
      verification.arm_seed_records_certified &&
      verification.factorized_facets_certified &&
      verification.result_facts_certified &&
      verification.decision_and_scope_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactDirectSaddleArmSeedStreamingVerification
verify_exact_direct_saddle_arm_seed_journal_streaming(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_budget,
    const ExactDirectSaddleArmSeedJournalResult& observed) {
  ExactDirectSaddleArmSeedStreamingVerification verification;
  const ExactDirectMorseEventJournalStreamingVerification source_verification =
      verify_exact_direct_morse_event_journal_streaming(
          cloud, source_facade, source_journal);
  verification.source_journal_certified =
      source_verification.result_certified &&
      source_journal.certified_partial_refinement();

  const auto replay_bound = checked_linear_bound(
      3U, cloud.size(), 5U, source_facade.events.size());
  const auto added_bound =
      checked_multiply(5U, source_facade.events.size());
  const auto combined_bound = checked_linear_bound(
      3U, cloud.size(), 10U, source_facade.events.size());
  if (!replay_bound.has_value() || !added_bound.has_value() ||
      !combined_bound.has_value()) {
    return verification;
  }

  std::size_t required_roles = cloud.size();
  std::size_t required_families = 0U;
  std::size_t required_arms = 0U;
  for (const ExactDirectSupportEvent& event : source_facade.events) {
    const std::size_t support_size =
        static_cast<std::size_t>(event.support_size);
    if (support_size < 2U || support_size > 4U) {
      return verification;
    }
    if (event.birth_order.has_value()) {
      const auto next = checked_add(required_roles, 1U);
      if (!next.has_value()) {
        return verification;
      }
      required_roles = *next;
    }
    if (event.saddle_order.has_value()) {
      const auto next_roles = checked_add(required_roles, 1U);
      const auto next_families = checked_add(required_families, 1U);
      const auto next_arms = checked_add(required_arms, support_size);
      if (!next_roles.has_value() || !next_families.has_value() ||
          !next_arms.has_value()) {
        return verification;
      }
      required_roles = *next_roles;
      required_families = *next_families;
      required_arms = *next_arms;
    }
  }

  const bool budget_sufficient =
      *replay_bound <=
          trusted_budget.maximum_source_journal_replay_entry_count &&
      required_roles <= trusted_budget.maximum_role_scan_count &&
      required_families <= trusted_budget.maximum_saddle_family_count &&
      required_arms <= trusted_budget.maximum_arm_seed_count;
  verification.requirements_certified =
      observed.schema_version ==
          direct_saddle_arm_seed_journal_schema_version &&
      observed.requested_budget == trusted_budget &&
      observed.point_count == cloud.size() &&
      observed.source_direct_event_count == source_facade.events.size() &&
      observed.required_source_journal_replay_entry_count == *replay_bound &&
      observed.required_role_scan_count == required_roles &&
      observed.required_saddle_family_count == required_families &&
      observed.required_arm_seed_count == required_arms &&
      observed.logical_added_storage_entry_limit == *added_bound &&
      observed.combined_logical_storage_entry_limit == *combined_bound &&
      observed.source_pair_canonical_cloud_digest ==
          source_facade.certificate.pair_canonical_cloud_digest &&
      observed.source_higher_canonical_cloud_digest ==
          source_facade.certificate.higher_canonical_cloud_digest &&
      observed.source_pair_semantic_digest ==
          source_facade.certificate.pair_semantic_digest &&
      observed.source_higher_semantic_digest ==
          source_facade.certificate.higher_semantic_digest &&
      budget_sufficient;

  std::size_t family_index = 0U;
  std::size_t seed_index = 0U;
  bool families_match = true;
  bool seeds_match = true;
  bool facets_match = true;
  try {
    for (const ExactDirectMorseH0RoleRecord& role :
         source_journal.role_records) {
      if (role.role != ExactDirectMorseH0Role::saddle) {
        continue;
      }
      if (role.batch_index >= source_journal.batches.size() ||
          role.event_projection_index >=
              source_journal.event_projections.size()) {
        families_match = false;
        seeds_match = false;
        facets_match = false;
        break;
      }
      const ExactDirectMorseH0Batch& batch =
          source_journal.batches[role.batch_index];
      const ExactDirectMorseEventProjection& projection =
          source_journal.event_projections[role.event_projection_index];
      if (projection.source_index >= source_facade.events.size()) {
        families_match = false;
        seeds_match = false;
        facets_match = false;
        break;
      }
      const ExactDirectSupportEvent& event =
          source_facade.events[projection.source_index];
      if (!source_join_matches(role, batch, projection, event)) {
        families_match = false;
        seeds_match = false;
        facets_match = false;
        break;
      }

      ExactDirectSaddleArmFamilyRecord expected_family;
      expected_family.family_index = family_index;
      expected_family.source_event_index = event.event_index;
      expected_family.journal_event_projection_index =
          projection.event_projection_index;
      expected_family.journal_role_record_index = role.role_record_index;
      expected_family.journal_batch_index = batch.batch_index;
      expected_family.order = batch.order;
      expected_family.critical_squared_level = batch.squared_level;
      expected_family.arm_seed_offset = seed_index;
      expected_family.arm_seed_count =
          static_cast<std::size_t>(event.support_size);
      expected_family.source_event_arm_identity_digest =
          source_event_arm_identity_digest(event);
      ++verification.source_family_scan_count;
      if (family_index >= observed.families.size() ||
          observed.families[family_index] != expected_family) {
        families_match = false;
      }

      for (std::size_t support_index = 0U;
           support_index < expected_family.arm_seed_count;
           ++support_index) {
        const ExactDirectSaddleArmSeedRecord expected_seed{
            seed_index,
            family_index,
            event.support_ids[support_index]};
        ++verification.source_arm_seed_scan_count;
        if (seed_index >= observed.arm_seeds.size() ||
            observed.arm_seeds[seed_index] != expected_seed) {
          seeds_match = false;
        } else {
          const ExactDirectSaddleArmFacet expected_facet =
              reconstruct_facet(
                  event, expected_seed.removed_support_point_id);
          const ExactDirectSaddleArmFacet observed_facet =
              reconstruct_exact_direct_saddle_arm_facet(
                  source_facade, observed, seed_index);
          if (expected_facet != observed_facet ||
              expected_facet.point_count != expected_family.order) {
            facets_match = false;
          }
        }
        ++seed_index;
      }
      ++family_index;
    }
  } catch (const std::logic_error&) {
    families_match = false;
    seeds_match = false;
    facets_match = false;
  }

  verification.family_records_certified =
      families_match && family_index == required_families &&
      observed.families.size() == required_families;
  verification.arm_seed_records_certified =
      seeds_match && seed_index == required_arms &&
      observed.arm_seeds.size() == required_arms;
  verification.factorized_facets_certified =
      facets_match && verification.family_records_certified &&
      verification.arm_seed_records_certified;

  const auto added_count = checked_add(required_families, required_arms);
  const auto combined_count = added_count.has_value()
      ? checked_add(
            source_journal.logical_linear_storage_entry_count,
            *added_count)
      : std::nullopt;
  verification.result_facts_certified =
      added_count.has_value() && combined_count.has_value() &&
      observed.logical_added_storage_entry_count ==
          *added_count &&
      observed.combined_logical_storage_entry_count == *combined_count &&
      observed.budget_preflight_certified &&
      observed.source_journal_freshly_replayed &&
      observed.source_facade_join_certified &&
      observed.every_saddle_role_projected_once &&
      observed.every_support_point_has_one_arm_seed &&
      observed.arm_seed_order_is_canonical &&
      observed.factorized_facets_reconstruct_exactly &&
      observed.source_relative_strict_miniball_drop_theorem_applies &&
      observed.output_linear_in_direct_events &&
      observed.no_forbidden_global_structure_materialized &&
      !observed.facets_materialized_in_journal &&
      !observed.miniballs_or_global_partitions_computed &&
      !observed.hierarchy_reduction_performed &&
      !observed.forest_or_gateway_attach_performed &&
      !observed.public_status_claimed && observed.partial_refinement_only;
  verification.decision_and_scope_certified =
      observed.decision == ExactDirectSaddleArmSeedDecision::
                               complete_certified_factorized_arm_seeds &&
      observed.scope == ExactDirectSaddleArmSeedScope::
                            terminal_direct_saddle_deleted_facets_only;
  verification.constant_auxiliary_record_storage_certified = true;
  verification.fresh_streaming_replay_certified =
      verification.requirements_certified &&
      verification.family_records_certified &&
      verification.arm_seed_records_certified &&
      verification.factorized_facets_certified &&
      verification.result_facts_certified &&
      verification.decision_and_scope_certified;
  verification.result_certified =
      verification.source_journal_certified &&
      verification.fresh_streaming_replay_certified &&
      observed.certified_partial_refinement();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
