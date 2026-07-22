#include "morsehgp3d/hierarchy/direct_closed_saddle_incidence_journal.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>

namespace morsehgp3d::hierarchy {
namespace {

struct IncidenceRequirements {
  std::size_t family_count{};
  std::size_t arm_seed_count{};
  std::size_t equal_level_facet_seed_count{};
};

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

[[nodiscard]] std::optional<IncidenceRequirements> derive_requirements(
    const ExactDirectSupportTerminalFacade& source_facade) {
  IncidenceRequirements requirements;
  for (const ExactDirectSupportEvent& event : source_facade.events) {
    const std::size_t support_size =
        static_cast<std::size_t>(event.support_size);
    if (support_size < 2U || support_size > 4U ||
        event.closed_rank < 2U || event.closed_rank > 11U ||
        event.closed_rank != support_size + event.interior_ids.size()) {
      return std::nullopt;
    }
    if (!event.saddle_order.has_value()) {
      continue;
    }
    if (*event.saddle_order + 1U != event.closed_rank) {
      return std::nullopt;
    }
    const auto next_families = checked_add(requirements.family_count, 1U);
    const auto next_arms =
        checked_add(requirements.arm_seed_count, support_size);
    const auto next_equal = checked_add(
        requirements.equal_level_facet_seed_count,
        event.interior_ids.size());
    if (!next_families.has_value() || !next_arms.has_value() ||
        !next_equal.has_value()) {
      return std::nullopt;
    }
    requirements.family_count = *next_families;
    requirements.arm_seed_count = *next_arms;
    requirements.equal_level_facet_seed_count = *next_equal;
  }
  return requirements;
}

[[nodiscard]] bool budget_is_sufficient(
    const IncidenceRequirements& requirements,
    const ExactDirectClosedSaddleIncidenceBudget& budget) {
  return requirements.family_count <=
             budget.maximum_source_family_scan_count &&
         requirements.arm_seed_count <=
             budget.maximum_source_arm_seed_scan_count &&
         requirements.family_count <=
             budget.maximum_incidence_family_count &&
         requirements.equal_level_facet_seed_count <=
             budget.maximum_equal_level_facet_seed_count;
}

[[nodiscard]] bool ids_are_strictly_increasing(
    const std::vector<spatial::PointId>& ids) {
  return std::adjacent_find(
             ids.begin(),
             ids.end(),
             [](spatial::PointId left, spatial::PointId right) {
               return left >= right;
             }) == ids.end();
}

[[nodiscard]] bool family_join_matches(
    const ExactDirectSaddleArmFamilyRecord& source_family,
    const ExactDirectSupportEvent& event) {
  return source_family.source_event_index == event.event_index &&
         event.saddle_order ==
             std::optional<std::size_t>{source_family.order} &&
         event.squared_level == source_family.critical_squared_level &&
         source_family.arm_seed_count ==
             static_cast<std::size_t>(event.support_size) &&
         source_family.order + 1U == event.closed_rank &&
         event.closed_rank == event.interior_ids.size() +
                                  static_cast<std::size_t>(
                                      event.support_size);
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceFamilyRecord
expected_family_record(
    const ExactDirectSaddleArmFamilyRecord& source_family,
    const ExactDirectSupportEvent& event,
    std::size_t family_index,
    std::size_t equal_level_facet_seed_offset) {
  ExactDirectClosedSaddleIncidenceFamilyRecord record;
  record.family_index = family_index;
  record.source_arm_family_index = source_family.family_index;
  record.source_event_index = source_family.source_event_index;
  record.journal_batch_index = source_family.journal_batch_index;
  record.order = source_family.order;
  record.critical_squared_level = source_family.critical_squared_level;
  record.strict_arm_seed_offset = source_family.arm_seed_offset;
  record.strict_arm_seed_count = source_family.arm_seed_count;
  record.equal_level_facet_seed_offset =
      equal_level_facet_seed_offset;
  record.equal_level_facet_seed_count = event.interior_ids.size();
  record.closed_facet_count = event.closed_rank;
  record.source_event_identity_digest =
      source_family.source_event_arm_identity_digest;
  return record;
}

[[nodiscard]] ExactDirectSaddleArmFacet reconstruct_equal_level_facet(
    const ExactDirectSupportEvent& event,
    spatial::PointId removed_interior_point_id) {
  ExactDirectSaddleArmFacet facet;
  const std::size_t support_size =
      static_cast<std::size_t>(event.support_size);
  if (support_size < 2U || support_size > 4U ||
      event.closed_rank < 3U || event.closed_rank > 11U ||
      event.closed_rank != support_size + event.interior_ids.size()) {
    throw std::invalid_argument(
        "an equal-level saddle facet source has an unsupported rank");
  }

  bool removed_found = false;
  for (const spatial::PointId point_id : event.interior_ids) {
    if (point_id == removed_interior_point_id) {
      if (removed_found) {
        throw std::invalid_argument(
            "a removed interior PointId occurs more than once");
      }
      removed_found = true;
      continue;
    }
    if (facet.point_count >= facet.point_ids.size()) {
      throw std::length_error(
          "an equal-level saddle facet exceeds ten points");
    }
    facet.point_ids[facet.point_count] = point_id;
    ++facet.point_count;
  }
  for (std::size_t support_index = 0U;
       support_index < support_size;
       ++support_index) {
    if (facet.point_count >= facet.point_ids.size()) {
      throw std::length_error(
          "an equal-level saddle facet exceeds ten points");
    }
    facet.point_ids[facet.point_count] = event.support_ids[support_index];
    ++facet.point_count;
  }
  if (!removed_found || facet.point_count + 1U != event.closed_rank) {
    throw std::invalid_argument(
        "an equal-level saddle facet did not remove one interior point");
  }

  std::sort(
      facet.point_ids.begin(),
      facet.point_ids.begin() +
          static_cast<std::ptrdiff_t>(facet.point_count));
  const auto unique_end = std::unique(
      facet.point_ids.begin(),
      facet.point_ids.begin() +
          static_cast<std::ptrdiff_t>(facet.point_count));
  if (unique_end != facet.point_ids.begin() +
                        static_cast<std::ptrdiff_t>(facet.point_count)) {
    throw std::invalid_argument(
        "an equal-level saddle facet contains duplicate PointIds");
  }
  return facet;
}

[[nodiscard]] bool source_authority_matches(
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence_journal) {
  return source_facade.terminal_catalog_certified() &&
         source_arm_journal.certified_partial_refinement() &&
         incidence_journal.certified_partial_refinement() &&
         incidence_journal.point_count ==
             source_facade.certificate.requirements.point_count &&
         incidence_journal.source_direct_event_count ==
             source_facade.events.size() &&
         incidence_journal.source_pair_canonical_cloud_digest ==
             source_facade.certificate.pair_canonical_cloud_digest &&
         incidence_journal.source_higher_canonical_cloud_digest ==
             source_facade.certificate.higher_canonical_cloud_digest &&
         incidence_journal.source_pair_semantic_digest ==
             source_facade.certificate.pair_semantic_digest &&
         incidence_journal.source_higher_semantic_digest ==
             source_facade.certificate.higher_semantic_digest &&
         source_arm_journal.source_pair_canonical_cloud_digest ==
             incidence_journal.source_pair_canonical_cloud_digest &&
         source_arm_journal.source_higher_canonical_cloud_digest ==
             incidence_journal.source_higher_canonical_cloud_digest &&
         source_arm_journal.source_pair_semantic_digest ==
             incidence_journal.source_pair_semantic_digest &&
         source_arm_journal.source_higher_semantic_digest ==
             incidence_journal.source_higher_semantic_digest;
}

void clear_payload(ExactDirectClosedSaddleIncidenceJournalResult& result) {
  result.families.clear();
  result.equal_level_facet_seeds.clear();
  result.logical_added_storage_entry_count = 0U;
  result.combined_logical_storage_entry_count = 0U;
}

}  // namespace

bool ExactDirectClosedSaddleIncidenceJournalResult::
    certified_partial_refinement() const {
  return schema_version ==
             direct_closed_saddle_incidence_journal_schema_version &&
         required_incidence_family_count == families.size() &&
         required_equal_level_facet_seed_count ==
             equal_level_facet_seeds.size() &&
         logical_added_storage_entry_count ==
             families.size() + equal_level_facet_seeds.size() &&
         logical_added_storage_entry_count <=
             logical_added_storage_entry_limit &&
         combined_logical_storage_entry_count <=
             combined_logical_storage_entry_limit &&
         budget_preflight_certified &&
         source_arm_journal_freshly_replayed &&
         source_facade_join_certified &&
         every_source_family_projected_once &&
         every_interior_point_has_one_equal_level_seed &&
         strict_and_equal_deletions_partition_every_saddle &&
         equal_level_seed_order_is_canonical &&
         factorized_facets_reconstruct_exactly &&
         equal_level_miniball_sandwich_theorem_applies &&
         strict_arms_reused_without_copy &&
         output_linear_in_direct_events &&
         no_forbidden_global_structure_materialized &&
         !facets_materialized_in_journal &&
         !miniballs_or_global_partitions_computed &&
         !frozen_quotient_performed &&
         !non_direct_gateway_generation_complete &&
         !hierarchy_reduction_performed &&
         !forest_or_gateway_attach_performed &&
         !missing_facet_means_isolated && !public_status_claimed &&
         partial_refinement_only &&
         decision == ExactDirectClosedSaddleIncidenceDecision::
                         complete_certified_direct_saddle_deletion_incidences &&
         scope == ExactDirectClosedSaddleIncidenceScope::
                      direct_saddle_strict_arms_and_equal_level_interior_facets_only;
}

ExactDirectClosedSaddleIncidenceJournalResult
build_exact_direct_closed_saddle_incidence_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& budget) {
  ExactDirectClosedSaddleIncidenceJournalResult result;
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
  result.scope = ExactDirectClosedSaddleIncidenceScope::
      direct_saddle_strict_arms_and_equal_level_interior_facets_only;

  const auto added_bound =
      checked_multiply(10U, source_facade.events.size());
  const auto combined_bound = checked_linear_bound(
      3U, cloud.size(), 20U, source_facade.events.size());
  const auto requirements = derive_requirements(source_facade);
  if (!added_bound.has_value() || !combined_bound.has_value() ||
      !requirements.has_value()) {
    result.decision = ExactDirectClosedSaddleIncidenceDecision::
        no_incidence_journal_capacity_overflow;
    return result;
  }
  result.required_source_family_scan_count = requirements->family_count;
  result.required_source_arm_seed_scan_count =
      requirements->arm_seed_count;
  result.required_incidence_family_count = requirements->family_count;
  result.required_equal_level_facet_seed_count =
      requirements->equal_level_facet_seed_count;
  result.logical_added_storage_entry_limit = *added_bound;
  result.combined_logical_storage_entry_limit = *combined_bound;

  if (!budget_is_sufficient(*requirements, budget)) {
    result.decision = ExactDirectClosedSaddleIncidenceDecision::
        no_incidence_journal_budget_exhausted;
    return result;
  }
  result.budget_preflight_certified = true;

  const auto source_verification =
      verify_exact_direct_saddle_arm_seed_journal_streaming(
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal);
  if (!source_verification.result_certified ||
      !source_arm_journal.certified_partial_refinement()) {
    result.decision = ExactDirectClosedSaddleIncidenceDecision::
        no_incidence_journal_source_not_certified;
    return result;
  }
  result.source_arm_journal_freshly_replayed = true;

  result.families.reserve(requirements->family_count);
  result.equal_level_facet_seeds.reserve(
      requirements->equal_level_facet_seed_count);
  std::size_t scanned_arms = 0U;
  try {
    for (const ExactDirectSaddleArmFamilyRecord& source_family :
         source_arm_journal.families) {
      if (source_family.family_index != result.families.size() ||
          source_family.source_event_index >= source_facade.events.size()) {
        throw std::invalid_argument(
            "a source arm family has inconsistent indexing");
      }
      const ExactDirectSupportEvent& event =
          source_facade.events[source_family.source_event_index];
      if (!family_join_matches(source_family, event) ||
          !ids_are_strictly_increasing(event.interior_ids)) {
        throw std::invalid_argument(
            "a source arm family disagrees with its terminal event");
      }
      const ExactDirectClosedSaddleIncidenceFamilyRecord family =
          expected_family_record(
              source_family,
              event,
              result.families.size(),
              result.equal_level_facet_seeds.size());
      if (family.strict_arm_seed_count +
              family.equal_level_facet_seed_count !=
          family.closed_facet_count) {
        throw std::logic_error(
            "strict and equal-level deletions do not partition a saddle");
      }
      result.families.push_back(family);

      for (const spatial::PointId removed_id : event.interior_ids) {
        const ExactDirectEqualLevelFacetSeedRecord seed{
            result.equal_level_facet_seeds.size(),
            family.family_index,
            removed_id};
        const ExactDirectSaddleArmFacet facet =
            reconstruct_equal_level_facet(event, removed_id);
        if (facet.point_count != family.order) {
          throw std::logic_error(
              "an equal-level direct saddle facet has the wrong order");
        }
        result.equal_level_facet_seeds.push_back(seed);
      }
      const auto next_arms =
          checked_add(scanned_arms, family.strict_arm_seed_count);
      if (!next_arms.has_value()) {
        throw std::overflow_error("source arm scan count overflow");
      }
      scanned_arms = *next_arms;
    }
  } catch (const std::logic_error&) {
    clear_payload(result);
    result.decision = ExactDirectClosedSaddleIncidenceDecision::
        no_incidence_journal_source_join_inconsistent;
    return result;
  }

  if (result.families.size() != requirements->family_count ||
      scanned_arms != requirements->arm_seed_count ||
      result.equal_level_facet_seeds.size() !=
          requirements->equal_level_facet_seed_count) {
    clear_payload(result);
    result.decision = ExactDirectClosedSaddleIncidenceDecision::
        no_incidence_journal_source_join_inconsistent;
    return result;
  }

  result.logical_added_storage_entry_count =
      result.families.size() + result.equal_level_facet_seeds.size();
  const auto combined_count = checked_add(
      source_arm_journal.combined_logical_storage_entry_count,
      result.logical_added_storage_entry_count);
  if (!combined_count.has_value()) {
    clear_payload(result);
    result.decision = ExactDirectClosedSaddleIncidenceDecision::
        no_incidence_journal_capacity_overflow;
    return result;
  }
  result.combined_logical_storage_entry_count = *combined_count;
  if (result.logical_added_storage_entry_count >
          result.logical_added_storage_entry_limit ||
      result.combined_logical_storage_entry_count >
          result.combined_logical_storage_entry_limit) {
    clear_payload(result);
    result.decision = ExactDirectClosedSaddleIncidenceDecision::
        no_incidence_journal_source_join_inconsistent;
    return result;
  }

  result.source_facade_join_certified = true;
  result.every_source_family_projected_once = true;
  result.every_interior_point_has_one_equal_level_seed = true;
  result.strict_and_equal_deletions_partition_every_saddle = true;
  result.equal_level_seed_order_is_canonical = true;
  result.factorized_facets_reconstruct_exactly = true;
  result.equal_level_miniball_sandwich_theorem_applies = true;
  result.strict_arms_reused_without_copy = true;
  result.output_linear_in_direct_events =
      result.logical_added_storage_entry_count <=
          result.logical_added_storage_entry_limit &&
      result.combined_logical_storage_entry_count <=
          result.combined_logical_storage_entry_limit;
  result.no_forbidden_global_structure_materialized = true;
  result.facets_materialized_in_journal = false;
  result.miniballs_or_global_partitions_computed = false;
  result.frozen_quotient_performed = false;
  result.non_direct_gateway_generation_complete = false;
  result.hierarchy_reduction_performed = false;
  result.forest_or_gateway_attach_performed = false;
  result.missing_facet_means_isolated = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
  result.decision = ExactDirectClosedSaddleIncidenceDecision::
      complete_certified_direct_saddle_deletion_incidences;
  return result;
}

ExactDirectSaddleArmFacet
reconstruct_exact_direct_equal_level_saddle_facet(
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceJournalResult& incidence_journal,
    std::size_t equal_level_facet_seed_index) {
  if (!source_authority_matches(
          source_facade, source_arm_journal, incidence_journal)) {
    throw std::invalid_argument(
        "an equal-level facet reconstruction has a different authority");
  }
  if (equal_level_facet_seed_index >=
      incidence_journal.equal_level_facet_seeds.size()) {
    throw std::out_of_range(
        "an equal-level facet seed index is outside the journal");
  }
  const ExactDirectEqualLevelFacetSeedRecord& seed =
      incidence_journal
          .equal_level_facet_seeds[equal_level_facet_seed_index];
  if (seed.equal_level_facet_seed_index !=
          equal_level_facet_seed_index ||
      seed.family_index >= incidence_journal.families.size()) {
    throw std::invalid_argument(
        "an equal-level facet seed has inconsistent indexing");
  }
  const ExactDirectClosedSaddleIncidenceFamilyRecord& family =
      incidence_journal.families[seed.family_index];
  if (family.family_index != seed.family_index ||
      family.source_arm_family_index >= source_arm_journal.families.size() ||
      family.source_event_index >= source_facade.events.size() ||
      equal_level_facet_seed_index <
          family.equal_level_facet_seed_offset ||
      equal_level_facet_seed_index -
              family.equal_level_facet_seed_offset >=
          family.equal_level_facet_seed_count) {
    throw std::invalid_argument(
        "an equal-level facet family has inconsistent indexing");
  }
  const ExactDirectSaddleArmFamilyRecord& source_family =
      source_arm_journal.families[family.source_arm_family_index];
  const ExactDirectSupportEvent& event =
      source_facade.events[family.source_event_index];
  if (source_family.family_index != family.source_arm_family_index ||
      !family_join_matches(source_family, event) ||
      source_family.source_event_arm_identity_digest !=
          family.source_event_identity_digest ||
      family.strict_arm_seed_offset != source_family.arm_seed_offset ||
      family.strict_arm_seed_count != source_family.arm_seed_count ||
      family.equal_level_facet_seed_count != event.interior_ids.size() ||
      family.closed_facet_count != event.closed_rank) {
    throw std::invalid_argument(
        "an equal-level facet source event has a different identity");
  }
  ExactDirectSaddleArmFacet facet = reconstruct_equal_level_facet(
      event, seed.removed_interior_point_id);
  if (facet.point_count != family.order) {
    throw std::invalid_argument(
        "a reconstructed equal-level saddle facet has the wrong order");
  }
  return facet;
}

ExactDirectClosedSaddleIncidenceStreamingVerification
verify_exact_direct_closed_saddle_incidence_journal_streaming(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& source_arm_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_arm_journal,
    const ExactDirectClosedSaddleIncidenceBudget& trusted_budget,
    const ExactDirectClosedSaddleIncidenceJournalResult& observed) {
  ExactDirectClosedSaddleIncidenceStreamingVerification verification;
  const auto added_bound =
      checked_multiply(10U, source_facade.events.size());
  const auto combined_bound = checked_linear_bound(
      3U, cloud.size(), 20U, source_facade.events.size());
  const auto requirements = derive_requirements(source_facade);
  if (!added_bound.has_value() || !combined_bound.has_value() ||
      !requirements.has_value() ||
      !budget_is_sufficient(*requirements, trusted_budget)) {
    return verification;
  }

  const auto source_verification =
      verify_exact_direct_saddle_arm_seed_journal_streaming(
          cloud,
          source_facade,
          source_journal,
          source_arm_budget,
          source_arm_journal);
  verification.source_arm_journal_certified =
      source_verification.result_certified &&
      source_arm_journal.certified_partial_refinement();
  if (!verification.source_arm_journal_certified) {
    return verification;
  }

  verification.requirements_certified =
      observed.schema_version ==
          direct_closed_saddle_incidence_journal_schema_version &&
      observed.requested_budget == trusted_budget &&
      observed.point_count == cloud.size() &&
      observed.source_direct_event_count == source_facade.events.size() &&
      observed.required_source_family_scan_count ==
          requirements->family_count &&
      observed.required_source_arm_seed_scan_count ==
          requirements->arm_seed_count &&
      observed.required_incidence_family_count ==
          requirements->family_count &&
      observed.required_equal_level_facet_seed_count ==
          requirements->equal_level_facet_seed_count &&
      observed.logical_added_storage_entry_limit == *added_bound &&
      observed.combined_logical_storage_entry_limit == *combined_bound &&
      observed.source_pair_canonical_cloud_digest ==
          source_facade.certificate.pair_canonical_cloud_digest &&
      observed.source_higher_canonical_cloud_digest ==
          source_facade.certificate.higher_canonical_cloud_digest &&
      observed.source_pair_semantic_digest ==
          source_facade.certificate.pair_semantic_digest &&
      observed.source_higher_semantic_digest ==
          source_facade.certificate.higher_semantic_digest;

  bool families_match = true;
  bool seeds_match = true;
  bool partitions_match = true;
  bool facets_match = true;
  std::size_t family_index = 0U;
  std::size_t seed_index = 0U;
  std::size_t arm_scan_count = 0U;
  try {
    for (const ExactDirectSaddleArmFamilyRecord& source_family :
         source_arm_journal.families) {
      if (source_family.family_index != family_index ||
          source_family.source_event_index >= source_facade.events.size()) {
        families_match = false;
        seeds_match = false;
        partitions_match = false;
        facets_match = false;
        break;
      }
      const ExactDirectSupportEvent& event =
          source_facade.events[source_family.source_event_index];
      if (!family_join_matches(source_family, event) ||
          !ids_are_strictly_increasing(event.interior_ids)) {
        families_match = false;
        seeds_match = false;
        partitions_match = false;
        facets_match = false;
        break;
      }
      const auto expected_family = expected_family_record(
          source_family, event, family_index, seed_index);
      ++verification.source_family_scan_count;
      verification.source_arm_seed_scan_count +=
          expected_family.strict_arm_seed_count;
      arm_scan_count += expected_family.strict_arm_seed_count;
      if (family_index >= observed.families.size() ||
          observed.families[family_index] != expected_family) {
        families_match = false;
      }
      if (expected_family.strict_arm_seed_count +
              expected_family.equal_level_facet_seed_count !=
          expected_family.closed_facet_count) {
        partitions_match = false;
      }

      for (const spatial::PointId removed_id : event.interior_ids) {
        const ExactDirectEqualLevelFacetSeedRecord expected_seed{
            seed_index, family_index, removed_id};
        ++verification.equal_level_facet_seed_scan_count;
        if (seed_index >= observed.equal_level_facet_seeds.size() ||
            observed.equal_level_facet_seeds[seed_index] != expected_seed) {
          seeds_match = false;
        } else {
          const ExactDirectSaddleArmFacet expected_facet =
              reconstruct_equal_level_facet(event, removed_id);
          const ExactDirectSaddleArmFacet observed_facet =
              reconstruct_exact_direct_equal_level_saddle_facet(
                  source_facade,
                  source_arm_journal,
                  observed,
                  seed_index);
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
    partitions_match = false;
    facets_match = false;
  }

  verification.family_records_certified =
      families_match && family_index == requirements->family_count &&
      observed.families.size() == requirements->family_count;
  verification.equal_level_facet_seed_records_certified =
      seeds_match &&
      seed_index == requirements->equal_level_facet_seed_count &&
      observed.equal_level_facet_seeds.size() ==
          requirements->equal_level_facet_seed_count;
  verification.deletion_partition_certified =
      partitions_match && arm_scan_count == requirements->arm_seed_count &&
      verification.family_records_certified;
  verification.factorized_facets_certified =
      facets_match && verification.family_records_certified &&
      verification.equal_level_facet_seed_records_certified;

  const auto added_count = checked_add(
      requirements->family_count,
      requirements->equal_level_facet_seed_count);
  const auto combined_count = added_count.has_value()
      ? checked_add(
            source_arm_journal.combined_logical_storage_entry_count,
            *added_count)
      : std::nullopt;
  verification.result_facts_certified =
      added_count.has_value() && combined_count.has_value() &&
      observed.logical_added_storage_entry_count == *added_count &&
      observed.combined_logical_storage_entry_count == *combined_count &&
      observed.budget_preflight_certified &&
      observed.source_arm_journal_freshly_replayed &&
      observed.source_facade_join_certified &&
      observed.every_source_family_projected_once &&
      observed.every_interior_point_has_one_equal_level_seed &&
      observed.strict_and_equal_deletions_partition_every_saddle &&
      observed.equal_level_seed_order_is_canonical &&
      observed.factorized_facets_reconstruct_exactly &&
      observed.equal_level_miniball_sandwich_theorem_applies &&
      observed.strict_arms_reused_without_copy &&
      observed.output_linear_in_direct_events &&
      observed.no_forbidden_global_structure_materialized &&
      !observed.facets_materialized_in_journal &&
      !observed.miniballs_or_global_partitions_computed &&
      !observed.frozen_quotient_performed &&
      !observed.non_direct_gateway_generation_complete &&
      !observed.hierarchy_reduction_performed &&
      !observed.forest_or_gateway_attach_performed &&
      !observed.missing_facet_means_isolated &&
      !observed.public_status_claimed && observed.partial_refinement_only;
  verification.decision_and_scope_certified =
      observed.decision == ExactDirectClosedSaddleIncidenceDecision::
                               complete_certified_direct_saddle_deletion_incidences &&
      observed.scope == ExactDirectClosedSaddleIncidenceScope::
                            direct_saddle_strict_arms_and_equal_level_interior_facets_only;
  verification.constant_auxiliary_record_storage_certified = true;
  verification.fresh_streaming_replay_certified =
      verification.requirements_certified &&
      verification.family_records_certified &&
      verification.equal_level_facet_seed_records_certified &&
      verification.deletion_partition_certified &&
      verification.factorized_facets_certified &&
      verification.result_facts_certified &&
      verification.decision_and_scope_certified;
  verification.result_certified =
      verification.source_arm_journal_certified &&
      verification.fresh_streaming_replay_certified &&
      observed.certified_partial_refinement();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
