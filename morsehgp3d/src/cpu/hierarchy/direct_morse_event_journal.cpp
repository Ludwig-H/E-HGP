#include "morsehgp3d/hierarchy/direct_morse_event_journal.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(message);
  }
  return left * right;
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
  static_assert(
      sizeof(std::size_t) <= sizeof(std::uint64_t),
      "the Phase-9 canonical manifest requires sizes no wider than u64");
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view text) {
  append_size(builder, text.size());
  builder.update(text);
}

[[nodiscard]] contract::CanonicalId canonical_phase9_cloud_digest(
    const spatial::CanonicalPointCloud& cloud,
    std::string_view domain) {
  contract::CanonicalSha256Builder builder;
  append_text(builder, domain);
  append_size(builder, cloud.size());
  for (std::size_t point_index = 0U; point_index < cloud.size();
       ++point_index) {
    const spatial::PointId point_id =
        static_cast<spatial::PointId>(point_index);
    for (const std::uint64_t word :
         cloud.point(point_id).canonical_input_bits()) {
      append_u64(builder, word);
    }
  }
  return builder.finalize();
}

[[nodiscard]] bool source_cloud_authorities_match(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& facade) {
  if (cloud.size() != facade.certificate.requirements.point_count) {
    return false;
  }
  return canonical_phase9_cloud_digest(
             cloud,
             "MorseHGP3D/phase9/pair-support/canonical-cloud/v1") ==
             facade.certificate.pair_canonical_cloud_digest &&
         canonical_phase9_cloud_digest(
             cloud,
             "MorseHGP3D/phase9/higher-support/canonical-cloud/v1") ==
             facade.certificate.higher_canonical_cloud_digest;
}

template <class Record>
[[nodiscard]] bool support_less(const Record& left, const Record& right) {
  const std::size_t common_size = std::min(
      static_cast<std::size_t>(left.support_size),
      static_cast<std::size_t>(right.support_size));
  for (std::size_t index = 0U; index < common_size; ++index) {
    if (left.support_ids[index] != right.support_ids[index]) {
      return left.support_ids[index] < right.support_ids[index];
    }
  }
  return left.support_size < right.support_size;
}

[[nodiscard]] bool center_less(
    const exact::ExactCenter3& left,
    const exact::ExactCenter3& right) {
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (left.coordinate(axis) != right.coordinate(axis)) {
      return left.coordinate(axis) < right.coordinate(axis);
    }
  }
  return false;
}

[[nodiscard]] bool source_event_less(
    const ExactDirectSupportEvent& left,
    const ExactDirectSupportEvent& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  if (left.closed_rank != right.closed_rank) {
    return left.closed_rank < right.closed_rank;
  }
  if (left.interior_ids != right.interior_ids) {
    return left.interior_ids < right.interior_ids;
  }
  if (support_less(left, right)) {
    return true;
  }
  if (support_less(right, left)) {
    return false;
  }
  return center_less(left.center, right.center);
}

[[nodiscard]] std::optional<std::size_t> expected_birth_order(
    std::size_t closed_rank,
    std::size_t effective_maximum_order) {
  if (closed_rank <= effective_maximum_order) {
    return closed_rank;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> expected_saddle_order(
    std::size_t closed_rank,
    std::size_t effective_maximum_order) {
  if (closed_rank >= 2U &&
      closed_rank - 1U <= effective_maximum_order) {
    return closed_rank - 1U;
  }
  return std::nullopt;
}

[[nodiscard]] bool source_event_is_locally_consistent(
    const ExactDirectSupportEvent& event,
    std::size_t expected_event_index,
    std::size_t point_count,
    std::size_t effective_maximum_order) {
  const std::size_t support_size =
      static_cast<std::size_t>(event.support_size);
  if (event.event_index != expected_event_index || support_size < 2U ||
      support_size > 4U || event.squared_level.numerator() == 0) {
    return false;
  }
  for (std::size_t index = 0U; index < support_size; ++index) {
    if (event.support_ids[index] >= point_count ||
        (index != 0U &&
         event.support_ids[index - 1U] >= event.support_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = support_size;
       index < event.support_ids.size();
       ++index) {
    if (event.support_ids[index] != spatial::PointId{0}) {
      return false;
    }
  }
  for (std::size_t index = 0U; index < event.interior_ids.size(); ++index) {
    const spatial::PointId point_id = event.interior_ids[index];
    if (point_id >= point_count ||
        (index != 0U && event.interior_ids[index - 1U] >= point_id) ||
        std::binary_search(
            event.support_ids.begin(),
            event.support_ids.begin() +
                static_cast<std::ptrdiff_t>(support_size),
            point_id)) {
      return false;
    }
  }
  if (event.interior_ids.size() > point_count - support_size) {
    return false;
  }
  const std::size_t expected_closed_rank =
      support_size + event.interior_ids.size();
  if (event.closed_rank != expected_closed_rank ||
      event.exterior_count != point_count - expected_closed_rank ||
      event.birth_order !=
          expected_birth_order(
              expected_closed_rank, effective_maximum_order) ||
      event.saddle_order !=
          expected_saddle_order(
              expected_closed_rank, effective_maximum_order) ||
      (!event.birth_order.has_value() &&
       !event.saddle_order.has_value())) {
    return false;
  }
  return true;
}

[[nodiscard]] bool source_payload_is_locally_consistent(
    const ExactDirectSupportTerminalFacade& facade) {
  const std::size_t point_count =
      facade.certificate.requirements.point_count;
  const std::size_t requested_maximum_order =
      facade.certificate.requirements.requested_maximum_order;
  const std::size_t effective_maximum_order =
      facade.certificate.requirements.effective_maximum_order;
  if (point_count == 0U || requested_maximum_order == 0U ||
      requested_maximum_order > 10U ||
      effective_maximum_order !=
          std::min(point_count, requested_maximum_order) ||
      facade.events.size() !=
          facade.certificate.normalized_event_count) {
    return false;
  }

  std::array<std::size_t, 3> event_count_by_arity{};
  for (std::size_t index = 0U; index < facade.events.size(); ++index) {
    const ExactDirectSupportEvent& event = facade.events[index];
    if (!source_event_is_locally_consistent(
            event,
            index,
            point_count,
            effective_maximum_order) ||
        (index != 0U &&
         !source_event_less(facade.events[index - 1U], event))) {
      return false;
    }
    ++event_count_by_arity[
        static_cast<std::size_t>(event.support_size - 2U)];
  }
  for (std::size_t index = 0U; index < event_count_by_arity.size(); ++index) {
    if (event_count_by_arity[index] !=
        facade.certificate.arity_certificates[index]
            .accepted_event_count) {
      return false;
    }
  }
  return true;
}

struct RoleSeed {
  std::size_t order{};
  exact::ExactLevel squared_level{};
  std::size_t event_projection_index{};
  ExactDirectMorseH0Role role{ExactDirectMorseH0Role::birth};
};

[[nodiscard]] bool role_seed_less(
    const RoleSeed& left,
    const RoleSeed& right) {
  if (left.order != right.order) {
    return left.order < right.order;
  }
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  return std::tie(left.event_projection_index, left.role) <
         std::tie(right.event_projection_index, right.role);
}

[[nodiscard]] bool same_batch_key(
    const RoleSeed& left,
    const RoleSeed& right) {
  return left.order == right.order &&
         left.squared_level == right.squared_level;
}

void append_role_seed(
    std::vector<RoleSeed>& seeds,
    std::size_t event_projection_index,
    const exact::ExactLevel& squared_level,
    const std::optional<std::size_t>& order,
    ExactDirectMorseH0Role role) {
  if (order.has_value()) {
    seeds.push_back(RoleSeed{
        *order, squared_level, event_projection_index, role});
  }
}

void build_payload(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    ExactDirectMorseEventJournalResult& result) {
  const std::size_t expected_projection_count = checked_add(
      cloud.size(),
      source_facade.events.size(),
      "the direct Morse event projection count overflows size_t");
  const std::size_t maximum_role_count = checked_add(
      cloud.size(),
      checked_multiply(
          2U,
          source_facade.events.size(),
          "the direct Morse role count overflows size_t"),
      "the direct Morse role count overflows size_t");
  result.event_projections.reserve(expected_projection_count);
  std::vector<RoleSeed> role_seeds;
  role_seeds.reserve(maximum_role_count);

  for (std::size_t point_index = 0U; point_index < cloud.size();
       ++point_index) {
    ExactDirectMorseEventProjection projection;
    projection.event_projection_index = result.event_projections.size();
    projection.source = ExactDirectMorseEventSource::canonical_singleton;
    projection.source_index = point_index;
    projection.support_size = 1U;
    projection.support_ids[0] =
        static_cast<spatial::PointId>(point_index);
    projection.squared_level = exact::ExactLevel{};
    projection.closed_rank = 1U;
    projection.birth_order = 1U;
    result.event_projections.push_back(projection);
    append_role_seed(
        role_seeds,
        projection.event_projection_index,
        projection.squared_level,
        projection.birth_order,
        ExactDirectMorseH0Role::birth);
  }

  for (const ExactDirectSupportEvent& source : source_facade.events) {
    ExactDirectMorseEventProjection projection;
    projection.event_projection_index = result.event_projections.size();
    projection.source =
        ExactDirectMorseEventSource::direct_support_terminal_event;
    projection.source_index = source.event_index;
    projection.support_size = source.support_size;
    projection.support_ids = source.support_ids;
    projection.squared_level = source.squared_level;
    projection.closed_rank = source.closed_rank;
    projection.birth_order = source.birth_order;
    projection.saddle_order = source.saddle_order;
    result.event_projections.push_back(projection);
    append_role_seed(
        role_seeds,
        projection.event_projection_index,
        projection.squared_level,
        projection.birth_order,
        ExactDirectMorseH0Role::birth);
    append_role_seed(
        role_seeds,
        projection.event_projection_index,
        projection.squared_level,
        projection.saddle_order,
        ExactDirectMorseH0Role::saddle);
  }

  std::sort(role_seeds.begin(), role_seeds.end(), role_seed_less);
  result.role_records.reserve(role_seeds.size());
  result.batches.reserve(role_seeds.size());
  std::size_t seed_index = 0U;
  while (seed_index < role_seeds.size()) {
    const std::size_t batch_begin = seed_index;
    while (seed_index < role_seeds.size() &&
           same_batch_key(role_seeds[batch_begin], role_seeds[seed_index])) {
      ++seed_index;
    }

    ExactDirectMorseH0Batch batch;
    batch.batch_index = result.batches.size();
    batch.order = role_seeds[batch_begin].order;
    batch.squared_level = role_seeds[batch_begin].squared_level;
    batch.role_record_offset = result.role_records.size();
    batch.role_record_count = seed_index - batch_begin;
    for (std::size_t index = batch_begin; index < seed_index; ++index) {
      const RoleSeed& seed = role_seeds[index];
      if (seed.role == ExactDirectMorseH0Role::birth) {
        ++batch.birth_role_count;
      } else {
        ++batch.saddle_role_count;
      }
      result.role_records.push_back(ExactDirectMorseH0RoleRecord{
          result.role_records.size(),
          batch.batch_index,
          seed.event_projection_index,
          seed.role});
    }
    result.batches.push_back(std::move(batch));
  }

  result.singleton_event_count = cloud.size();
  result.event_projection_count = result.event_projections.size();
  result.role_record_count = result.role_records.size();
  result.batch_count = result.batches.size();
  result.logical_linear_storage_entry_count = checked_add(
      checked_add(
          result.event_projection_count,
          result.role_record_count,
          "the direct Morse logical storage count overflows size_t"),
      result.batch_count,
      "the direct Morse logical storage count overflows size_t");
  result.logical_linear_storage_entry_limit = checked_add(
      checked_multiply(
          3U,
          cloud.size(),
          "the direct Morse logical storage bound overflows size_t"),
      checked_multiply(
          5U,
          source_facade.events.size(),
          "the direct Morse logical storage bound overflows size_t"),
      "the direct Morse logical storage bound overflows size_t");
  result.canonical_singleton_births_complete =
      result.singleton_event_count == cloud.size();
  result.direct_h0_roles_projected_exactly_once =
      result.role_record_count == role_seeds.size() &&
      result.role_record_count <= maximum_role_count;
  result.batch_keys_strictly_increasing = true;
  result.role_records_canonical_and_partitioned = true;
  result.output_linear_in_singletons_and_direct_events =
      result.logical_linear_storage_entry_count <=
      result.logical_linear_storage_entry_limit;
  result.no_forbidden_global_structure_materialized = true;
  result.partial_refinement_only = true;
}

[[nodiscard]] bool non_payload_facts_equal(
    const ExactDirectMorseEventJournalResult& left,
    const ExactDirectMorseEventJournalResult& right) {
  ExactDirectMorseEventJournalResult left_copy = left;
  ExactDirectMorseEventJournalResult right_copy = right;
  left_copy.event_projections.clear();
  left_copy.role_records.clear();
  left_copy.batches.clear();
  right_copy.event_projections.clear();
  right_copy.role_records.clear();
  right_copy.batches.clear();
  return left_copy == right_copy;
}

}  // namespace

bool ExactDirectMorseEventJournalResult::certified_partial_refinement()
    const {
  return schema_version == direct_morse_event_journal_schema_version &&
         decision == ExactDirectMorseEventJournalDecision::
                         complete_certified_partial_refinement &&
         scope == ExactDirectMorseEventJournalScope::
                      canonical_singletons_and_terminal_direct_supports_h0_roles_only &&
         source_facade_terminal_certified &&
         source_cloud_authorities_match &&
         source_facade_payload_locally_consistent &&
         no_relevant_extra_shell_diagnostics &&
         canonical_singleton_births_complete &&
         direct_h0_roles_projected_exactly_once &&
         batch_keys_strictly_increasing &&
         role_records_canonical_and_partitioned &&
         output_linear_in_singletons_and_direct_events &&
         no_forbidden_global_structure_materialized &&
         !hierarchy_reduction_performed &&
         !forest_or_gateway_attach_performed && !public_status_claimed &&
         partial_refinement_only && singleton_event_count == point_count &&
         source_direct_event_count <= event_projection_count &&
         singleton_event_count ==
             event_projection_count - source_direct_event_count &&
         event_projection_count == event_projections.size() &&
         role_record_count == role_records.size() &&
         batch_count == batches.size() &&
         logical_linear_storage_entry_count <=
             logical_linear_storage_entry_limit;
}

ExactDirectMorseEventJournalResult
build_exact_direct_morse_event_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade) {
  ExactDirectMorseEventJournalResult result;
  result.point_count = source_facade.certificate.requirements.point_count;
  result.requested_maximum_order =
      source_facade.certificate.requirements.requested_maximum_order;
  result.effective_maximum_order =
      source_facade.certificate.requirements.effective_maximum_order;
  result.source_direct_event_count = source_facade.events.size();
  result.source_pair_semantic_digest =
      source_facade.certificate.pair_semantic_digest;
  result.source_higher_semantic_digest =
      source_facade.certificate.higher_semantic_digest;
  result.scope = ExactDirectMorseEventJournalScope::
      canonical_singletons_and_terminal_direct_supports_h0_roles_only;
  result.source_facade_terminal_certified =
      source_facade.terminal_catalog_certified();
  if (!result.source_facade_terminal_certified) {
    result.decision = ExactDirectMorseEventJournalDecision::
        no_journal_source_facade_not_terminal;
    return result;
  }

  result.source_cloud_authorities_match =
      source_cloud_authorities_match(cloud, source_facade);
  if (!result.source_cloud_authorities_match) {
    result.decision = ExactDirectMorseEventJournalDecision::
        no_journal_source_authority_mismatch;
    return result;
  }

  result.source_facade_payload_locally_consistent =
      source_payload_is_locally_consistent(source_facade);
  if (!result.source_facade_payload_locally_consistent) {
    result.decision = ExactDirectMorseEventJournalDecision::
        no_journal_source_facade_payload_inconsistent;
    return result;
  }

  result.no_relevant_extra_shell_diagnostics =
      source_facade.relevant_extra_shell_diagnostics.empty();
  if (!result.no_relevant_extra_shell_diagnostics) {
    result.decision = ExactDirectMorseEventJournalDecision::
        no_journal_relevant_extra_shell_diagnostics;
    return result;
  }

  build_payload(cloud, source_facade, result);
  result.decision = ExactDirectMorseEventJournalDecision::
      complete_certified_partial_refinement;
  if (!result.certified_partial_refinement()) {
    throw std::logic_error(
        "the direct Morse event journal certificate is inconsistent");
  }
  return result;
}

ExactDirectMorseEventJournalVerification
verify_exact_direct_morse_event_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& observed) {
  const ExactDirectMorseEventJournalResult expected =
      build_exact_direct_morse_event_journal(cloud, source_facade);
  ExactDirectMorseEventJournalVerification verification;
  verification.source_facade_terminal_certified =
      expected.source_facade_terminal_certified;
  verification.source_authorities_accepted =
      expected.source_cloud_authorities_match &&
      expected.source_facade_payload_locally_consistent &&
      expected.no_relevant_extra_shell_diagnostics;
  verification.event_projections_certified =
      observed.event_projections == expected.event_projections;
  verification.role_records_certified =
      observed.role_records == expected.role_records;
  verification.batches_certified = observed.batches == expected.batches;
  verification.result_facts_certified =
      non_payload_facts_equal(observed, expected);
  verification.decision_and_scope_certified =
      observed.decision == expected.decision &&
      observed.scope == expected.scope;
  verification.fresh_projection_replay_certified = observed == expected;
  verification.result_certified =
      expected.certified_partial_refinement() &&
      verification.source_facade_terminal_certified &&
      verification.source_authorities_accepted &&
      verification.event_projections_certified &&
      verification.role_records_certified &&
      verification.batches_certified &&
      verification.result_facts_certified &&
      verification.decision_and_scope_certified &&
      verification.fresh_projection_replay_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
