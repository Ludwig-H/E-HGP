#include "morsehgp3d/hierarchy/direct_morse_m1_o5_death_accounting.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <new>
#include <numeric>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

[[nodiscard]] bool try_add(
    std::size_t left,
    std::size_t right,
    std::size_t& sum) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  sum = left + right;
  return true;
}

[[nodiscard]] bool try_multiply(
    std::size_t left,
    std::size_t right,
    std::size_t& product) noexcept {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    return false;
  }
  product = left * right;
  return true;
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
    std::string_view value) {
  append_size(builder, value.size());
  builder.update(value);
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

void append_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& value) {
  append_text(builder, value.numerator_string());
  append_text(builder, value.denominator_string());
}

[[nodiscard]] contract::CanonicalId event_audit_digest(
    std::span<const ExactDirectMorseM1O5EventAudit> audits) {
  contract::CanonicalSha256Builder builder;
  append_text(
      builder,
      "MorseHGP3D/phase15/direct-morse-m1-o5-event-audits/v1");
  append_size(builder, audits.size());
  for (const auto& audit : audits) {
    append_size(builder, audit.event_audit_index);
    append_size(builder, audit.order);
    append_level(builder, audit.squared_level);
    append_id(builder, audit.source_event_arm_identity_digest);
    append_size(builder, audit.support_point_count);
    for (std::size_t index = 0U;
         index < audit.support_point_count;
         ++index) {
      append_u64(
          builder,
          static_cast<std::uint64_t>(
              audit.canonical_support_point_ids[index]));
    }
    append_size(builder, audit.arm_count);
    append_size(builder, audit.delta_one);
  }
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId group_audit_digest(
    std::span<const ExactDirectMorseM1O5GroupAudit> audits) {
  contract::CanonicalSha256Builder builder;
  append_text(
      builder,
      "MorseHGP3D/phase15/direct-morse-m1-o5-group-audits/v1");
  append_size(builder, audits.size());
  for (const auto& audit : audits) {
    append_size(builder, audit.group_audit_index);
    append_size(builder, audit.batch_audit_index);
    append_size(builder, audit.order);
    append_level(builder, audit.squared_level);
    append_id(builder, audit.canonical_group_digest);
    append_size(builder, audit.saddle_count);
    append_size(builder, audit.arm_count);
    append_size(builder, audit.distinct_carrier_count);
    append_size(builder, audit.prior_reduced_root_count);
    append_size(builder, audit.death_count);
    append_u64(builder, static_cast<std::uint8_t>(audit.kind));
  }
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId batch_audit_digest(
    std::span<const ExactDirectMorseM1O5BatchAudit> audits) {
  contract::CanonicalSha256Builder builder;
  append_text(
      builder,
      "MorseHGP3D/phase15/direct-morse-m1-o5-batch-audits/v1");
  append_size(builder, audits.size());
  for (const auto& audit : audits) {
    append_size(builder, audit.batch_audit_index);
    append_size(builder, audit.order);
    append_level(builder, audit.squared_level);
    append_size(builder, audit.event_count);
    append_size(builder, audit.group_count);
    append_size(builder, audit.local_multiplicity_capacity);
    append_size(builder, audit.touched_distinct_prior_root_count);
    append_size(builder, audit.positive_root_group_count);
    append_size(builder, audit.global_death_count);
    append_size(builder, audit.sum_group_death_count);
  }
  return builder.finalize();
}

struct ArmIdentity {
  contract::CanonicalId event_digest{};
  PointId removed_point_id{};
  std::array<PointId, ExactDirectSaddleArmFacet::maximum_point_count>
      facet_point_ids{};
  std::size_t facet_point_count{};

  friend bool operator==(const ArmIdentity&, const ArmIdentity&) = default;
};

[[nodiscard]] bool arm_identity_less(
    const ArmIdentity& left,
    const ArmIdentity& right) noexcept {
  if (left.event_digest != right.event_digest) {
    return left.event_digest < right.event_digest;
  }
  if (left.removed_point_id != right.removed_point_id) {
    return left.removed_point_id < right.removed_point_id;
  }
  const std::size_t common =
      std::min(left.facet_point_count, right.facet_point_count);
  for (std::size_t index = 0U; index < common; ++index) {
    if (left.facet_point_ids[index] != right.facet_point_ids[index]) {
      return left.facet_point_ids[index] < right.facet_point_ids[index];
    }
  }
  return left.facet_point_count < right.facet_point_count;
}

[[nodiscard]] contract::CanonicalId canonical_group_identity_digest(
    std::vector<ArmIdentity> identities) {
  std::sort(identities.begin(), identities.end(), arm_identity_less);
  contract::CanonicalSha256Builder builder;
  append_text(
      builder,
      "MorseHGP3D/phase15/direct-morse-m1-o5-canonical-group/v1");
  append_size(builder, identities.size());
  for (const auto& identity : identities) {
    append_id(builder, identity.event_digest);
    append_u64(
        builder, static_cast<std::uint64_t>(identity.removed_point_id));
    append_size(builder, identity.facet_point_count);
    for (std::size_t index = 0U;
         index < identity.facet_point_count;
         ++index) {
      append_u64(
          builder,
          static_cast<std::uint64_t>(identity.facet_point_ids[index]));
    }
  }
  return builder.finalize();
}

class LocalDisjointSet {
 public:
  explicit LocalDisjointSet(std::size_t count) : parents_(count) {
    std::iota(parents_.begin(), parents_.end(), 0U);
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    std::size_t root = value;
    while (parents_[root] != root) {
      root = parents_[root];
    }
    while (parents_[value] != value) {
      const std::size_t next = parents_[value];
      parents_[value] = root;
      value = next;
    }
    return root;
  }

  void unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left != right) {
      parents_[std::max(left, right)] = std::min(left, right);
    }
  }

 private:
  std::vector<std::size_t> parents_;
};

void clear_scientific_facts(
    ExactDirectMorseM1O5DeathAccountingResult& result) {
  result.source_canonical_cloud_digest = {};
  result.event_audit_digest = {};
  result.group_audit_digest = {};
  result.batch_audit_digest = {};
  result.required_logical_output_entry_count = 0U;
  result.counters = {};
  result.event_audits.clear();
  result.group_audits.clear();
  result.batch_audits.clear();
  result.budget_preflight_certified = false;
  result.source_event_journal_freshly_replayed_relative_to_facade = false;
  result.source_seed_journal_freshly_replayed_relative_to_facade = false;
  result.source_forest_freshly_replayed_relative_to_facade = false;
  result.conditional_on_caller_fresh_phase9_facade_replay = true;
  result.every_index_one_event_has_complete_shell_arms = false;
  result.every_arm_seed_point_id_and_exact_level_joined = false;
  result.same_prior_root_carriers_unioned_before_saddle_hyperedges = false;
  result.every_saddle_hyperedge_including_latent_closed_transitively = false;
  result.simultaneous_carrier_quotient_reconstructed = false;
  result.quotient_partition_matches_forest_atomic_groups = false;
  result.group_root_counts_kinds_and_children_replayed = false;
  result.group_death_counts_replayed = false;
  result.batch_global_death_identity_replayed = false;
  result.local_index_one_multiplicity_bounds_replayed = false;
  result.m1_o5_combinatorial_accounting_replayed = false;
  result.event_local_death_attribution_serialized = false;
  result.durable_carrier_root_or_node_ids_serialized = false;
  result.source_catalog_complete_for_full_pi0 = false;
  result.carrier_faithfulness_complete = false;
  result.silent_gamma_group_completeness = false;
  result.bidirectional_gamma_group_completeness = false;
  result.external_target_authority_replayed = false;
  result.global_morse_obligation_replayed = false;
  result.global_m1_claimed = false;
  result.all_naturality_squares_replayed = false;
  result.vertical_maps_complete = false;
  result.forest_semantics_exact = false;
  result.bounded_exhaustive_gamma_oracle_used = false;
  result.gamma_cells_or_global_cofaces_materialized = false;
  result.higher_order_delaunay_materialized = false;
  result.public_status_claimed = false;
  result.scalable_50k_claimed = false;
  result.no_partial_scientific_payload_published_on_failure = true;
  result.scope = ExactDirectMorseM1O5DeathAccountingScope::unspecified;
}

[[nodiscard]] ExactDirectMorseM1O5DeathAccountingResult fail(
    ExactDirectMorseM1O5DeathAccountingResult result,
    ExactDirectMorseM1O5DeathAccountingDecision decision) {
  clear_scientific_facts(result);
  result.decision = decision;
  return result;
}

[[nodiscard]] bool budget_covers(
    const ExactDirectMorseM1O5DeathAccountingBudget& budget,
    const ExactDirectMorseM1O5DeathAccountingCounters& counters,
    std::size_t logical_output_entry_count) noexcept {
  return counters.event_audit_count <= budget.maximum_event_audit_count &&
      counters.group_audit_count <= budget.maximum_group_audit_count &&
      counters.batch_audit_count <= budget.maximum_batch_audit_count &&
      counters.source_family_scan_count <=
          budget.maximum_source_family_scan_count &&
      counters.source_arm_seed_scan_count <=
          budget.maximum_source_arm_seed_scan_count &&
      counters.source_binding_scan_count <=
          budget.maximum_source_binding_scan_count &&
      counters.source_saddle_scan_count <=
          budget.maximum_source_saddle_scan_count &&
      counters.source_group_scan_count <=
          budget.maximum_source_group_scan_count &&
      counters.source_batch_scan_count <=
          budget.maximum_source_batch_scan_count &&
      counters.point_id_reference_scan_count <=
          budget.maximum_point_id_reference_scan_count &&
      counters.prior_root_reference_scan_count <=
          budget.maximum_prior_root_reference_scan_count &&
      counters.exact_level_comparison_count <=
          budget.maximum_exact_level_comparison_count &&
      counters.peak_logical_scratch_entry_count <=
          budget.maximum_logical_scratch_entry_count &&
      logical_output_entry_count <=
          budget.maximum_logical_output_entry_count;
}

[[nodiscard]] bool add_to(
    std::size_t increment,
    std::size_t& value) noexcept {
  std::size_t next = 0U;
  if (!try_add(value, increment, next)) {
    return false;
  }
  value = next;
  return true;
}

[[nodiscard]] bool zero_id(const contract::CanonicalId& id) noexcept {
  return id == contract::CanonicalId{};
}

[[nodiscard]] bool is_atomic_failure_decision(
    ExactDirectMorseM1O5DeathAccountingDecision decision) noexcept {
  using Decision = ExactDirectMorseM1O5DeathAccountingDecision;
  switch (decision) {
    case Decision::no_accounting_capacity_overflow:
    case Decision::no_accounting_allocation_failed:
    case Decision::no_accounting_budget_exhausted:
    case Decision::no_accounting_point_count_or_order_rejected:
    case Decision::no_accounting_source_event_journal_rejected:
    case Decision::no_accounting_source_seed_journal_rejected:
    case Decision::no_accounting_source_forest_rejected:
    case Decision::no_accounting_source_join_inconsistent:
    case Decision::no_accounting_arm_bijection_rejected:
    case Decision::no_accounting_point_id_or_level_join_rejected:
    case Decision::no_accounting_carrier_root_conflict:
    case Decision::no_accounting_quotient_group_mismatch:
    case Decision::no_accounting_death_identity_mismatch:
    case Decision::no_accounting_local_multiplicity_bound_failed:
      return true;
    case Decision::not_certified:
    case Decision::complete_bounded_conditional_m1_o5_death_accounting:
      return false;
  }
  return false;
}

[[nodiscard]] bool valid_success_payload(
    const ExactDirectMorseM1O5DeathAccountingResult& result) noexcept {
  std::size_t audit_count = 0U;
  const bool audit_count_valid = try_add(
      result.event_audits.size(), result.group_audits.size(), audit_count) &&
      add_to(result.batch_audits.size(), audit_count);
  bool audit_digests_match = false;
  try {
    audit_digests_match =
        result.event_audit_digest == event_audit_digest(result.event_audits) &&
        result.group_audit_digest == group_audit_digest(result.group_audits) &&
        result.batch_audit_digest == batch_audit_digest(result.batch_audits);
  } catch (const std::bad_alloc&) {
    return false;
  }
  if (result.schema_version !=
          direct_morse_m1_o5_death_accounting_schema_version ||
      result.point_count < 2U || result.point_count > 14U ||
      result.effective_maximum_order == 0U ||
      result.effective_maximum_order > 10U ||
      result.event_audits.empty() || result.group_audits.empty() ||
      result.batch_audits.empty() ||
      result.event_audits.size() != result.counters.event_audit_count ||
      result.group_audits.size() != result.counters.group_audit_count ||
      result.batch_audits.size() != result.counters.batch_audit_count ||
      !audit_count_valid ||
      result.required_logical_output_entry_count != audit_count ||
      !budget_covers(
          result.requested_budget,
          result.counters,
          result.required_logical_output_entry_count) ||
      !audit_digests_match) {
    return false;
  }

  std::size_t total_capacity = 0U;
  std::size_t total_roots = 0U;
  std::size_t total_positive_groups = 0U;
  std::size_t total_deaths = 0U;
  for (std::size_t index = 0U; index < result.event_audits.size(); ++index) {
    const auto& event = result.event_audits[index];
    if (event.event_audit_index != index || event.order == 0U ||
        event.order > result.effective_maximum_order ||
        event.support_point_count < 2U ||
        event.support_point_count > event.canonical_support_point_ids.size() ||
        event.arm_count != event.support_point_count ||
        event.delta_one != event.arm_count - 1U) {
      return false;
    }
    for (std::size_t local = 1U;
         local < event.support_point_count;
         ++local) {
      if (event.canonical_support_point_ids[local - 1U] >=
          event.canonical_support_point_ids[local]) {
        return false;
      }
    }
  }
  for (std::size_t index = 0U; index < result.group_audits.size(); ++index) {
    const auto& group = result.group_audits[index];
    if (group.group_audit_index != index ||
        group.batch_audit_index >= result.batch_audits.size() ||
        group.saddle_count == 0U || group.arm_count == 0U ||
        group.distinct_carrier_count == 0U ||
        group.prior_reduced_root_count > group.distinct_carrier_count ||
        group.death_count !=
            (group.prior_reduced_root_count == 0U
                 ? 0U
                 : group.prior_reduced_root_count - 1U)) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < result.batch_audits.size(); ++index) {
    const auto& batch = result.batch_audits[index];
    if (batch.batch_audit_index != index || batch.order == 0U ||
        batch.order > result.effective_maximum_order ||
        batch.global_death_count != batch.sum_group_death_count ||
        batch.global_death_count > batch.local_multiplicity_capacity ||
        batch.positive_root_group_count >
            batch.touched_distinct_prior_root_count ||
        batch.global_death_count !=
            batch.touched_distinct_prior_root_count -
                batch.positive_root_group_count ||
        !add_to(batch.local_multiplicity_capacity, total_capacity) ||
        !add_to(batch.touched_distinct_prior_root_count, total_roots) ||
        !add_to(batch.positive_root_group_count, total_positive_groups) ||
        !add_to(batch.global_death_count, total_deaths)) {
      return false;
    }
  }
  return total_capacity == result.counters.total_local_multiplicity_capacity &&
      total_roots ==
          result.counters.total_touched_distinct_prior_root_count &&
      total_positive_groups ==
          result.counters.total_positive_root_group_count &&
      total_deaths == result.counters.total_global_death_count;
}

}  // namespace

bool ExactDirectMorseM1O5DeathAccountingResult::
    certified_bounded_conditional_m1_o5_accounting() const noexcept {
  return decision == ExactDirectMorseM1O5DeathAccountingDecision::
                         complete_bounded_conditional_m1_o5_death_accounting &&
      scope == ExactDirectMorseM1O5DeathAccountingScope::
                   bounded_n14_fresh_direct_equal_level_hypergraph_local_multiplicity_and_global_h0_death_accounting_only &&
      valid_success_payload(*this) && budget_preflight_certified &&
      source_event_journal_freshly_replayed_relative_to_facade &&
      source_seed_journal_freshly_replayed_relative_to_facade &&
      source_forest_freshly_replayed_relative_to_facade &&
      conditional_on_caller_fresh_phase9_facade_replay &&
      every_index_one_event_has_complete_shell_arms &&
      every_arm_seed_point_id_and_exact_level_joined &&
      same_prior_root_carriers_unioned_before_saddle_hyperedges &&
      every_saddle_hyperedge_including_latent_closed_transitively &&
      simultaneous_carrier_quotient_reconstructed &&
      quotient_partition_matches_forest_atomic_groups &&
      group_root_counts_kinds_and_children_replayed &&
      group_death_counts_replayed &&
      batch_global_death_identity_replayed &&
      local_index_one_multiplicity_bounds_replayed &&
      m1_o5_combinatorial_accounting_replayed &&
      !event_local_death_attribution_serialized &&
      !durable_carrier_root_or_node_ids_serialized &&
      !source_catalog_complete_for_full_pi0 &&
      !carrier_faithfulness_complete && !silent_gamma_group_completeness &&
      !bidirectional_gamma_group_completeness &&
      !external_target_authority_replayed &&
      !global_morse_obligation_replayed && !global_m1_claimed &&
      !all_naturality_squares_replayed && !vertical_maps_complete &&
      !forest_semantics_exact && !bounded_exhaustive_gamma_oracle_used &&
      !gamma_cells_or_global_cofaces_materialized &&
      !higher_order_delaunay_materialized && !public_status_claimed &&
      !scalable_50k_claimed &&
      no_partial_scientific_payload_published_on_failure;
}

bool ExactDirectMorseM1O5DeathAccountingResult::certified_atomic_failure()
    const noexcept {
  return schema_version ==
             direct_morse_m1_o5_death_accounting_schema_version &&
      is_atomic_failure_decision(decision) &&
      scope == ExactDirectMorseM1O5DeathAccountingScope::unspecified &&
      event_audits.empty() && group_audits.empty() &&
      batch_audits.empty() && zero_id(source_canonical_cloud_digest) &&
      zero_id(event_audit_digest) && zero_id(group_audit_digest) &&
      zero_id(batch_audit_digest) &&
      required_logical_output_entry_count == 0U &&
      counters == ExactDirectMorseM1O5DeathAccountingCounters{} &&
      !budget_preflight_certified &&
      !source_event_journal_freshly_replayed_relative_to_facade &&
      !source_seed_journal_freshly_replayed_relative_to_facade &&
      !source_forest_freshly_replayed_relative_to_facade &&
      conditional_on_caller_fresh_phase9_facade_replay &&
      !every_index_one_event_has_complete_shell_arms &&
      !every_arm_seed_point_id_and_exact_level_joined &&
      !same_prior_root_carriers_unioned_before_saddle_hyperedges &&
      !every_saddle_hyperedge_including_latent_closed_transitively &&
      !simultaneous_carrier_quotient_reconstructed &&
      !quotient_partition_matches_forest_atomic_groups &&
      !group_root_counts_kinds_and_children_replayed &&
      !group_death_counts_replayed && !batch_global_death_identity_replayed &&
      !local_index_one_multiplicity_bounds_replayed &&
      !m1_o5_combinatorial_accounting_replayed &&
      !event_local_death_attribution_serialized &&
      !durable_carrier_root_or_node_ids_serialized &&
      !source_catalog_complete_for_full_pi0 &&
      !carrier_faithfulness_complete && !silent_gamma_group_completeness &&
      !bidirectional_gamma_group_completeness &&
      !external_target_authority_replayed &&
      !global_morse_obligation_replayed && !global_m1_claimed &&
      !all_naturality_squares_replayed && !vertical_maps_complete &&
      !forest_semantics_exact && !bounded_exhaustive_gamma_oracle_used &&
      !gamma_cells_or_global_cofaces_materialized &&
      !higher_order_delaunay_materialized && !public_status_claimed &&
      !scalable_50k_claimed &&
      no_partial_scientific_payload_published_on_failure;
}

bool ExactDirectMorseM1O5DeathAccountingResult::certified_outcome()
    const noexcept {
  return certified_bounded_conditional_m1_o5_accounting() ||
      certified_atomic_failure();
}

ExactDirectMorseM1O5DeathAccountingResult
build_exact_direct_morse_m1_o5_death_accounting(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& trusted_forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseM1O5DeathAccountingBudget& accounting_budget) {
  ExactDirectMorseM1O5DeathAccountingResult result;
  result.requested_budget = accounting_budget;
  result.point_count = cloud.size();
  result.effective_maximum_order = source_event_journal.effective_maximum_order;
  result.no_partial_scientific_payload_published_on_failure = true;

  if (cloud.size() < 2U || cloud.size() > 14U ||
      source_event_journal.effective_maximum_order == 0U ||
      source_event_journal.effective_maximum_order > 10U) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_point_count_or_order_rejected);
  }

  const auto event_verification = verify_exact_direct_morse_event_journal(
      cloud, source_facade, source_event_journal);
  if (!event_verification.result_certified) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_source_event_journal_rejected);
  }
  const auto seed_verification =
      verify_exact_direct_saddle_arm_seed_journal_streaming(
          cloud,
          source_facade,
          source_event_journal,
          trusted_seed_budget,
          source_seed_journal);
  if (!seed_verification.result_certified) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_source_seed_journal_rejected);
  }
  const auto forest_verification = verify_exact_direct_morse_forest_journal(
      index,
      cloud,
      source_facade,
      source_event_journal,
      trusted_seed_budget,
      source_seed_journal,
      trusted_forest_budget,
      trusted_forest_config,
      traversal_order,
      source_forest);
  if (!forest_verification.result_certified) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_source_forest_rejected);
  }

  ExactDirectMorseM1O5DeathAccountingCounters requirements;
  requirements.event_audit_count = source_seed_journal.families.size();
  requirements.group_audit_count = source_forest.atomic_groups.size();
  requirements.batch_audit_count = source_forest.batches.size();
  requirements.source_family_scan_count =
      source_seed_journal.families.size();
  requirements.source_arm_seed_scan_count =
      source_seed_journal.arm_seeds.size();
  if (!try_multiply(
          2U,
          source_forest.arm_root_bindings.size(),
          requirements.source_binding_scan_count) ||
      !try_multiply(
          3U,
          source_forest.saddle_records.size(),
          requirements.source_saddle_scan_count) ||
      !try_multiply(
          3U,
          source_forest.batches.size(),
          requirements.source_batch_scan_count)) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_capacity_overflow);
  }
  requirements.source_group_scan_count = source_forest.atomic_groups.size();
  std::size_t triple_family_level_comparisons = 0U;
  if (!try_multiply(
          3U,
          source_seed_journal.families.size(),
          triple_family_level_comparisons) ||
      !try_add(
          triple_family_level_comparisons,
          source_forest.saddle_records.size(),
          requirements.exact_level_comparison_count) ||
      !add_to(
          source_forest.batches.size(),
          requirements.exact_level_comparison_count)) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_capacity_overflow);
  }
  for (const auto& event : source_facade.events) {
    if (event.saddle_order.has_value() &&
        !add_to(static_cast<std::size_t>(event.support_size),
                requirements.point_id_reference_scan_count)) {
      return fail(
          std::move(result),
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_capacity_overflow);
    }
  }
  for (const auto& binding : source_forest.arm_root_bindings) {
    if (binding.source_arm_seed_index >= source_seed_journal.arm_seeds.size()) {
      result.rejected_source_binding_index = binding.binding_index;
      return fail(
          std::move(result),
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_source_join_inconsistent);
    }
    const auto facet = reconstruct_exact_direct_saddle_arm_facet(
        source_facade, source_seed_journal, binding.source_arm_seed_index);
    if (!add_to(
            facet.point_count + 1U,
            requirements.point_id_reference_scan_count)) {
      return fail(
          std::move(result),
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_capacity_overflow);
    }
    if (binding.prior_reduced_root_node_id.has_value() &&
        !add_to(1U, requirements.prior_root_reference_scan_count)) {
      return fail(
          std::move(result),
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_capacity_overflow);
    }
  }
  for (const auto& batch : source_forest.batches) {
    std::size_t saddle_entries = 0U;
    std::size_t group_entries = 0U;
    std::size_t scratch = 0U;
    if (!try_multiply(3U, batch.saddle_record_count, saddle_entries) ||
        !try_multiply(2U, batch.atomic_group_count, group_entries) ||
        !try_add(saddle_entries, group_entries, scratch)) {
      return fail(
          std::move(result),
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_capacity_overflow);
    }
    std::size_t binding_count = 0U;
    for (std::size_t local = 0U;
         local < batch.saddle_record_count;
         ++local) {
      const auto& saddle =
          source_forest.saddle_records[batch.saddle_record_offset + local];
      if (!add_to(saddle.arm_binding_count, binding_count)) {
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_capacity_overflow);
      }
    }
    // At peak, the four batch-wide carrier arenas and first-root map cost at
    // most five binding-sized domains.  The two per-saddle payloads cost two
    // more.  The current component carriers and identities plus the disjoint
    // combination of prior touched roots and current roots cost three more.
    // The remaining three saddle-sized and two group-sized domains are
    // charged above.  This is a conservative logical-entry bound; container
    // object bytes and allocator metadata are outside this logical counter.
    std::size_t binding_entries = 0U;
    if (!try_multiply(10U, binding_count, binding_entries) ||
        !add_to(binding_entries, scratch)) {
      return fail(
          std::move(result),
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_capacity_overflow);
    }
    requirements.peak_logical_scratch_entry_count = std::max(
        requirements.peak_logical_scratch_entry_count, scratch);
  }
  std::size_t logical_output_count = 0U;
  if (!try_add(
          requirements.event_audit_count,
          requirements.group_audit_count,
          logical_output_count) ||
      !add_to(requirements.batch_audit_count, logical_output_count)) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_capacity_overflow);
  }
  if (!budget_covers(accounting_budget, requirements, logical_output_count)) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_budget_exhausted);
  }

  try {
    result.event_audits.reserve(requirements.event_audit_count);
    result.group_audits.reserve(requirements.group_audit_count);
    result.batch_audits.reserve(requirements.batch_audit_count);

    const ExactDirectMorseEventJournalView event_view{source_event_journal};
    for (std::size_t family_index = 0U;
         family_index < source_seed_journal.families.size();
         ++family_index) {
      const auto& family = source_seed_journal.families[family_index];
      if (family.family_index != family_index ||
          family.source_event_index >= source_facade.events.size() ||
          family.journal_event_projection_index >=
              event_view.event_projection_count() ||
          family.journal_batch_index >= source_event_journal.batches.size() ||
          family.arm_seed_offset > source_seed_journal.arm_seeds.size() ||
          family.arm_seed_count >
              source_seed_journal.arm_seeds.size() - family.arm_seed_offset) {
        result.rejected_source_family_index = family_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_source_join_inconsistent);
      }
      const auto& event = source_facade.events[family.source_event_index];
      const auto projection = event_view.event_projection_at(
          family.journal_event_projection_index);
      const auto& source_batch =
          source_event_journal.batches[family.journal_batch_index];
      const std::size_t support_count =
          static_cast<std::size_t>(event.support_size);
      if (support_count < 2U || support_count > event.support_ids.size() ||
          event.event_index != family.source_event_index ||
          event.saddle_order != std::optional<std::size_t>{family.order} ||
          event.closed_rank != event.interior_ids.size() + support_count ||
          family.order + 1U != event.closed_rank ||
          family.critical_squared_level != event.squared_level ||
          family.arm_seed_count != support_count ||
          projection.source !=
              ExactDirectMorseEventSource::direct_support_terminal_event ||
          projection.source_index != family.source_event_index ||
          projection.saddle_order != event.saddle_order ||
          projection.squared_level != event.squared_level ||
          source_batch.order != family.order ||
          source_batch.squared_level != family.critical_squared_level) {
        result.rejected_source_family_index = family_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_point_id_or_level_join_rejected);
      }
      std::vector<PointId> removed_ids;
      removed_ids.reserve(family.arm_seed_count);
      for (std::size_t local = 0U; local < family.arm_seed_count; ++local) {
        const auto& seed =
            source_seed_journal.arm_seeds[family.arm_seed_offset + local];
        if (seed.arm_seed_index != family.arm_seed_offset + local ||
            seed.family_index != family_index) {
          result.rejected_source_family_index = family_index;
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_arm_bijection_rejected);
        }
        removed_ids.push_back(seed.removed_support_point_id);
      }
      std::sort(removed_ids.begin(), removed_ids.end());
      if (!std::equal(
              removed_ids.begin(),
              removed_ids.end(),
              event.support_ids.begin())) {
        result.rejected_source_family_index = family_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_arm_bijection_rejected);
      }
      ExactDirectMorseM1O5EventAudit audit;
      audit.event_audit_index = family_index;
      audit.order = family.order;
      audit.squared_level = family.critical_squared_level;
      audit.source_event_arm_identity_digest =
          family.source_event_arm_identity_digest;
      audit.canonical_support_point_ids = event.support_ids;
      audit.support_point_count = support_count;
      audit.arm_count = family.arm_seed_count;
      audit.delta_one = family.arm_seed_count - 1U;
      result.event_audits.push_back(std::move(audit));
    }

    for (std::size_t batch_index = 0U;
         batch_index < source_forest.batches.size();
         ++batch_index) {
      const auto& batch = source_forest.batches[batch_index];
      if (batch.batch_index != batch_index ||
          batch.source_journal_batch_index >=
              source_event_journal.batches.size() ||
          batch.saddle_record_offset > source_forest.saddle_records.size() ||
          batch.saddle_record_count >
              source_forest.saddle_records.size() -
                  batch.saddle_record_offset ||
          batch.atomic_group_offset > source_forest.atomic_groups.size() ||
          batch.atomic_group_count >
              source_forest.atomic_groups.size() -
                  batch.atomic_group_offset) {
        result.rejected_source_batch_index = batch_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_source_join_inconsistent);
      }
      const auto& event_batch =
          source_event_journal.batches[batch.source_journal_batch_index];
      if (event_batch.order != batch.order ||
          event_batch.squared_level != batch.squared_level) {
        result.rejected_source_batch_index = batch_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_point_id_or_level_join_rejected);
      }

      std::map<ExactDirectSparseComponentHandle, std::size_t> carrier_indices;
      std::vector<ExactDirectSparseComponentHandle> carriers;
      std::vector<std::optional<ExactDirectMorseForestNodeId>> carrier_roots;
      std::vector<std::vector<std::size_t>> saddle_carrier_indices;
      std::vector<std::vector<ArmIdentity>> saddle_arm_identities;
      saddle_carrier_indices.reserve(batch.saddle_record_count);
      saddle_arm_identities.reserve(batch.saddle_record_count);

      for (std::size_t local_saddle = 0U;
           local_saddle < batch.saddle_record_count;
           ++local_saddle) {
        const std::size_t saddle_index =
            batch.saddle_record_offset + local_saddle;
        const auto& saddle = source_forest.saddle_records[saddle_index];
        if (saddle.saddle_record_index != saddle_index ||
            saddle.source_family_index >=
                source_seed_journal.families.size() ||
            saddle.source_journal_batch_index !=
                batch.source_journal_batch_index ||
            saddle.arm_binding_offset >
                source_forest.arm_root_bindings.size() ||
            saddle.arm_binding_count >
                source_forest.arm_root_bindings.size() -
                    saddle.arm_binding_offset ||
            saddle.arm_binding_count == 0U) {
          result.rejected_source_batch_index = batch_index;
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_source_join_inconsistent);
        }
        const auto& family =
            source_seed_journal.families[saddle.source_family_index];
        if (family.journal_batch_index != batch.source_journal_batch_index ||
            family.order != batch.order ||
            family.critical_squared_level != batch.squared_level ||
            family.arm_seed_count != saddle.arm_binding_count) {
          result.rejected_source_family_index = saddle.source_family_index;
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_point_id_or_level_join_rejected);
        }
        std::vector<std::size_t> local_carriers;
        std::vector<ArmIdentity> identities;
        local_carriers.reserve(saddle.arm_binding_count);
        identities.reserve(saddle.arm_binding_count);
        for (std::size_t local_binding = 0U;
             local_binding < saddle.arm_binding_count;
             ++local_binding) {
          const std::size_t binding_index =
              saddle.arm_binding_offset + local_binding;
          const auto& binding =
              source_forest.arm_root_bindings[binding_index];
          if (binding.binding_index != binding_index ||
              binding.source_family_index != saddle.source_family_index ||
              binding.source_arm_seed_index >=
                  source_seed_journal.arm_seeds.size()) {
            result.rejected_source_binding_index = binding_index;
            return fail(
                std::move(result),
                ExactDirectMorseM1O5DeathAccountingDecision::
                    no_accounting_source_join_inconsistent);
          }
          const auto& seed = source_seed_journal.arm_seeds[
              binding.source_arm_seed_index];
          if (seed.family_index != saddle.source_family_index) {
            result.rejected_source_binding_index = binding_index;
            return fail(
                std::move(result),
                ExactDirectMorseM1O5DeathAccountingDecision::
                    no_accounting_arm_bijection_rejected);
          }
          const auto facet = reconstruct_exact_direct_saddle_arm_facet(
              source_facade,
              source_seed_journal,
              binding.source_arm_seed_index);
          if (facet.point_count != binding.strict_arm_key.point_count ||
              !std::equal(
                  facet.point_ids.begin(),
                  facet.point_ids.begin() +
                      static_cast<std::ptrdiff_t>(facet.point_count),
                  binding.strict_arm_key.point_ids.begin())) {
            result.rejected_source_binding_index = binding_index;
            return fail(
                std::move(result),
                ExactDirectMorseM1O5DeathAccountingDecision::
                    no_accounting_point_id_or_level_join_rejected);
          }

          const auto inserted = carrier_indices.emplace(
              binding.frozen_carrier_component_handle, carriers.size());
          std::size_t carrier_index = inserted.first->second;
          if (inserted.second) {
            carriers.push_back(binding.frozen_carrier_component_handle);
            carrier_roots.push_back(binding.prior_reduced_root_node_id);
          } else if (carrier_roots[carrier_index] !=
                     binding.prior_reduced_root_node_id) {
            result.rejected_source_binding_index = binding_index;
            return fail(
                std::move(result),
                ExactDirectMorseM1O5DeathAccountingDecision::
                    no_accounting_carrier_root_conflict);
          }
          local_carriers.push_back(carrier_index);
          ArmIdentity identity;
          identity.event_digest = family.source_event_arm_identity_digest;
          identity.removed_point_id = seed.removed_support_point_id;
          identity.facet_point_ids = facet.point_ids;
          identity.facet_point_count = facet.point_count;
          identities.push_back(std::move(identity));
        }
        std::sort(local_carriers.begin(), local_carriers.end());
        local_carriers.erase(
            std::unique(local_carriers.begin(), local_carriers.end()),
            local_carriers.end());
        saddle_carrier_indices.push_back(std::move(local_carriers));
        saddle_arm_identities.push_back(std::move(identities));
      }

      LocalDisjointSet components(carriers.size());
      std::map<ExactDirectMorseForestNodeId, std::size_t>
          first_carrier_by_root;
      for (std::size_t carrier_index = 0U;
           carrier_index < carrier_roots.size();
           ++carrier_index) {
        if (!carrier_roots[carrier_index].has_value()) {
          continue;
        }
        const auto inserted = first_carrier_by_root.emplace(
            *carrier_roots[carrier_index], carrier_index);
        if (!inserted.second) {
          components.unite(inserted.first->second, carrier_index);
        }
      }
      for (const auto& saddle_carriers : saddle_carrier_indices) {
        if (saddle_carriers.empty()) {
          result.rejected_source_batch_index = batch_index;
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_arm_bijection_rejected);
        }
        for (std::size_t local = 1U;
             local < saddle_carriers.size();
             ++local) {
          components.unite(saddle_carriers.front(), saddle_carriers[local]);
        }
      }

      std::map<std::size_t, std::vector<std::size_t>> saddles_by_component;
      for (std::size_t local_saddle = 0U;
           local_saddle < saddle_carrier_indices.size();
           ++local_saddle) {
        const std::size_t component =
            components.find(saddle_carrier_indices[local_saddle].front());
        if (!std::all_of(
                saddle_carrier_indices[local_saddle].begin(),
                saddle_carrier_indices[local_saddle].end(),
                [&components, component](std::size_t carrier_index) {
                  return components.find(carrier_index) == component;
                })) {
          result.rejected_source_batch_index = batch_index;
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_quotient_group_mismatch);
        }
        saddles_by_component[component].push_back(local_saddle);
      }
      if (saddles_by_component.size() != batch.atomic_group_count) {
        result.rejected_source_batch_index = batch_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_quotient_group_mismatch);
      }

      ExactDirectMorseM1O5BatchAudit batch_audit;
      batch_audit.batch_audit_index = batch_index;
      batch_audit.order = batch.order;
      batch_audit.squared_level = batch.squared_level;
      batch_audit.event_count = batch.saddle_record_count;
      batch_audit.group_count = batch.atomic_group_count;
      std::vector<ExactDirectMorseForestNodeId> touched_roots;
      std::map<std::size_t, std::size_t> source_group_by_component;

      for (std::size_t local_group = 0U;
           local_group < batch.atomic_group_count;
           ++local_group) {
        const std::size_t group_index =
            batch.atomic_group_offset + local_group;
        const auto& group = source_forest.atomic_groups[group_index];
        if (group.atomic_group_index != group_index ||
            group.batch_index != batch_index ||
            group.saddle_record_offset < batch.saddle_record_offset ||
            group.saddle_record_count == 0U ||
            group.saddle_record_offset - batch.saddle_record_offset >
                saddle_carrier_indices.size() ||
            group.saddle_record_count >
                saddle_carrier_indices.size() -
                    (group.saddle_record_offset -
                     batch.saddle_record_offset)) {
          result.rejected_atomic_group_index = group_index;
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_quotient_group_mismatch);
        }
        const std::size_t first_local_saddle =
            group.saddle_record_offset - batch.saddle_record_offset;
        const std::size_t component = components.find(
            saddle_carrier_indices[first_local_saddle].front());
        const auto inserted_group =
            source_group_by_component.emplace(component, group_index);
        if (!inserted_group.second ||
            saddles_by_component[component].size() !=
                group.saddle_record_count) {
          result.rejected_atomic_group_index = group_index;
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_quotient_group_mismatch);
        }
        for (std::size_t local = 0U;
             local < group.saddle_record_count;
             ++local) {
          const std::size_t local_saddle = first_local_saddle + local;
          if (components.find(
                  saddle_carrier_indices[local_saddle].front()) !=
              component) {
            result.rejected_atomic_group_index = group_index;
            return fail(
                std::move(result),
                ExactDirectMorseM1O5DeathAccountingDecision::
                    no_accounting_quotient_group_mismatch);
          }
        }

        std::vector<std::size_t> component_carriers;
        std::vector<ExactDirectMorseForestNodeId> component_roots;
        for (std::size_t carrier_index = 0U;
             carrier_index < carriers.size();
             ++carrier_index) {
          if (components.find(carrier_index) != component) {
            continue;
          }
          component_carriers.push_back(carrier_index);
          if (carrier_roots[carrier_index].has_value()) {
            component_roots.push_back(*carrier_roots[carrier_index]);
          }
        }
        std::sort(component_roots.begin(), component_roots.end());
        component_roots.erase(
            std::unique(component_roots.begin(), component_roots.end()),
            component_roots.end());
        const std::size_t q = component_roots.size();
        const std::size_t deaths = q == 0U ? 0U : q - 1U;
        const auto expected_kind =
            q == 0U
                ? ExactDirectMorseForestAtomicGroupKind::reduced_birth
                : (q == 1U
                       ? ExactDirectMorseForestAtomicGroupKind::continuation
                       : ExactDirectMorseForestAtomicGroupKind::multifusion);
        const bool children_match =
            q < 2U
                ? group.child_count == 0U
                : group.child_count == q &&
                      group.child_offset <= source_forest.child_node_ids.size() &&
                      group.child_count <=
                          source_forest.child_node_ids.size() -
                              group.child_offset &&
                      std::equal(
                          component_roots.begin(),
                          component_roots.end(),
                          source_forest.child_node_ids.begin() +
                              static_cast<std::ptrdiff_t>(group.child_offset));
        if (group.frozen_carrier_count != component_carriers.size() ||
            group.prior_reduced_root_count != q ||
            group.latent_carrier_count !=
                component_carriers.size() - q ||
            group.kind != expected_kind || !children_match ||
            (q == 1U && group.created_node_id.has_value()) ||
            (q != 1U && !group.created_node_id.has_value())) {
          result.rejected_atomic_group_index = group_index;
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_quotient_group_mismatch);
        }

        std::vector<ArmIdentity> group_identities;
        std::size_t group_arm_count = 0U;
        for (const std::size_t local_saddle :
             saddles_by_component[component]) {
          const auto& identities = saddle_arm_identities[local_saddle];
          if (!add_to(identities.size(), group_arm_count)) {
            return fail(
                std::move(result),
                ExactDirectMorseM1O5DeathAccountingDecision::
                    no_accounting_capacity_overflow);
          }
          group_identities.insert(
              group_identities.end(), identities.begin(), identities.end());
        }
        ExactDirectMorseM1O5GroupAudit group_audit;
        group_audit.group_audit_index = result.group_audits.size();
        group_audit.batch_audit_index = batch_index;
        group_audit.order = batch.order;
        group_audit.squared_level = batch.squared_level;
        group_audit.canonical_group_digest =
            canonical_group_identity_digest(std::move(group_identities));
        group_audit.saddle_count = group.saddle_record_count;
        group_audit.arm_count = group_arm_count;
        group_audit.distinct_carrier_count = component_carriers.size();
        group_audit.prior_reduced_root_count = q;
        group_audit.death_count = deaths;
        group_audit.kind = expected_kind;
        result.group_audits.push_back(std::move(group_audit));

        if (!add_to(deaths, batch_audit.sum_group_death_count)) {
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_capacity_overflow);
        }
        if (q > 0U) {
          ++batch_audit.positive_root_group_count;
        }
        touched_roots.insert(
            touched_roots.end(),
            component_roots.begin(),
            component_roots.end());
      }
      if (source_group_by_component.size() != saddles_by_component.size()) {
        result.rejected_source_batch_index = batch_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_quotient_group_mismatch);
      }

      for (std::size_t local_saddle = 0U;
           local_saddle < batch.saddle_record_count;
           ++local_saddle) {
        const auto& saddle = source_forest.saddle_records[
            batch.saddle_record_offset + local_saddle];
        const auto& family =
            source_seed_journal.families[saddle.source_family_index];
        if (!add_to(
                family.arm_seed_count - 1U,
                batch_audit.local_multiplicity_capacity)) {
          return fail(
              std::move(result),
              ExactDirectMorseM1O5DeathAccountingDecision::
                  no_accounting_capacity_overflow);
        }
      }
      std::sort(touched_roots.begin(), touched_roots.end());
      if (std::adjacent_find(touched_roots.begin(), touched_roots.end()) !=
          touched_roots.end()) {
        result.rejected_source_batch_index = batch_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_death_identity_mismatch);
      }
      batch_audit.touched_distinct_prior_root_count = touched_roots.size();
      if (batch_audit.positive_root_group_count > touched_roots.size()) {
        result.rejected_source_batch_index = batch_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_death_identity_mismatch);
      }
      batch_audit.global_death_count =
          touched_roots.size() - batch_audit.positive_root_group_count;
      if (batch_audit.global_death_count !=
          batch_audit.sum_group_death_count) {
        result.rejected_source_batch_index = batch_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_death_identity_mismatch);
      }
      if (batch_audit.global_death_count >
          batch_audit.local_multiplicity_capacity) {
        result.rejected_source_batch_index = batch_index;
        return fail(
            std::move(result),
            ExactDirectMorseM1O5DeathAccountingDecision::
                no_accounting_local_multiplicity_bound_failed);
      }
      result.batch_audits.push_back(std::move(batch_audit));
    }
  } catch (const std::bad_alloc&) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_allocation_failed);
  }

  result.required_logical_output_entry_count = logical_output_count;
  result.counters = requirements;
  for (const auto& event : result.event_audits) {
    if (!add_to(
            event.delta_one,
            result.counters.total_local_multiplicity_capacity)) {
      return fail(
          std::move(result),
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_capacity_overflow);
    }
  }
  for (const auto& batch : result.batch_audits) {
    if (!add_to(
            batch.touched_distinct_prior_root_count,
            result.counters.total_touched_distinct_prior_root_count) ||
        !add_to(
            batch.positive_root_group_count,
            result.counters.total_positive_root_group_count) ||
        !add_to(
            batch.global_death_count,
            result.counters.total_global_death_count)) {
      return fail(
          std::move(result),
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_capacity_overflow);
    }
  }
  for (const auto& batch : source_forest.batches) {
    result.counters.maximum_batch_saddle_count = std::max(
        result.counters.maximum_batch_saddle_count,
        batch.saddle_record_count);
  }
  for (const auto& audit : result.group_audits) {
    result.counters.maximum_batch_carrier_count = std::max(
        result.counters.maximum_batch_carrier_count,
        audit.distinct_carrier_count);
  }

  try {
    result.source_canonical_cloud_digest =
        source_forest.source_higher_canonical_cloud_digest;
    result.event_audit_digest = event_audit_digest(result.event_audits);
    result.group_audit_digest = group_audit_digest(result.group_audits);
    result.batch_audit_digest = batch_audit_digest(result.batch_audits);
    result.budget_preflight_certified = true;
    result.source_event_journal_freshly_replayed_relative_to_facade = true;
    result.source_seed_journal_freshly_replayed_relative_to_facade = true;
    result.source_forest_freshly_replayed_relative_to_facade = true;
    result.conditional_on_caller_fresh_phase9_facade_replay = true;
    result.every_index_one_event_has_complete_shell_arms = true;
    result.every_arm_seed_point_id_and_exact_level_joined = true;
    result.same_prior_root_carriers_unioned_before_saddle_hyperedges = true;
    result.every_saddle_hyperedge_including_latent_closed_transitively = true;
    result.simultaneous_carrier_quotient_reconstructed = true;
    result.quotient_partition_matches_forest_atomic_groups = true;
    result.group_root_counts_kinds_and_children_replayed = true;
    result.group_death_counts_replayed = true;
    result.batch_global_death_identity_replayed = true;
    result.local_index_one_multiplicity_bounds_replayed = true;
    result.m1_o5_combinatorial_accounting_replayed = true;
    result.no_partial_scientific_payload_published_on_failure = true;
    result.decision = ExactDirectMorseM1O5DeathAccountingDecision::
        complete_bounded_conditional_m1_o5_death_accounting;
    result.scope = ExactDirectMorseM1O5DeathAccountingScope::
        bounded_n14_fresh_direct_equal_level_hypergraph_local_multiplicity_and_global_h0_death_accounting_only;
    if (!result.certified_bounded_conditional_m1_o5_accounting()) {
      return fail(
          std::move(result),
          ExactDirectMorseM1O5DeathAccountingDecision::
              no_accounting_death_identity_mismatch);
    }
  } catch (const std::bad_alloc&) {
    return fail(
        std::move(result),
        ExactDirectMorseM1O5DeathAccountingDecision::
            no_accounting_allocation_failed);
  }
  return result;
}

ExactDirectMorseM1O5DeathAccountingVerification
verify_exact_direct_morse_m1_o5_death_accounting(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectMorseForestBudget& trusted_forest_budget,
    const ExactDirectMorseForestConfig& trusted_forest_config,
    spatial::LbvhTraversalOrder traversal_order,
    const ExactDirectMorseForestJournalResult& source_forest,
    const ExactDirectMorseM1O5DeathAccountingBudget&
        trusted_accounting_budget,
    const ExactDirectMorseM1O5DeathAccountingResult& observed) {
  ExactDirectMorseM1O5DeathAccountingVerification verification;
  const auto event_verification = verify_exact_direct_morse_event_journal(
      cloud, source_facade, source_event_journal);
  verification.source_event_journal_freshly_replayed =
      event_verification.result_certified;
  const auto seed_verification =
      verify_exact_direct_saddle_arm_seed_journal_streaming(
          cloud,
          source_facade,
          source_event_journal,
          trusted_seed_budget,
          source_seed_journal);
  verification.source_seed_journal_freshly_replayed =
      seed_verification.result_certified;
  const auto forest_verification = verify_exact_direct_morse_forest_journal(
      index,
      cloud,
      source_facade,
      source_event_journal,
      trusted_seed_budget,
      source_seed_journal,
      trusted_forest_budget,
      trusted_forest_config,
      traversal_order,
      source_forest);
  verification.source_forest_freshly_replayed =
      forest_verification.result_certified;
  verification.trusted_inputs_accepted =
      verification.source_event_journal_freshly_replayed &&
      verification.source_seed_journal_freshly_replayed &&
      verification.source_forest_freshly_replayed && cloud.size() >= 2U &&
      cloud.size() <= 14U;
  verification.observed_storage_within_budget =
      observed.event_audits.size() <=
          trusted_accounting_budget.maximum_event_audit_count &&
      observed.group_audits.size() <=
          trusted_accounting_budget.maximum_group_audit_count &&
      observed.batch_audits.size() <=
          trusted_accounting_budget.maximum_batch_audit_count &&
      budget_covers(
          trusted_accounting_budget,
          observed.counters,
          observed.required_logical_output_entry_count);

  const auto expected = build_exact_direct_morse_m1_o5_death_accounting(
      index,
      cloud,
      source_facade,
      source_event_journal,
      trusted_seed_budget,
      source_seed_journal,
      trusted_forest_budget,
      trusted_forest_config,
      traversal_order,
      source_forest,
      trusted_accounting_budget);
  verification.expected_result_freshly_reconstructed =
      expected.certified_outcome();
  verification.observed_recursively_equal = observed == expected;
  verification.result_certified =
      verification.trusted_inputs_accepted &&
      verification.observed_storage_within_budget &&
      verification.expected_result_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      observed.certified_outcome();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
