#include "morsehgp3d/hierarchy/direct_support_terminal.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] exact::BigInt exact_binomial(
    std::size_t point_count,
    std::size_t support_size) {
  if (support_size > point_count) {
    return exact::BigInt{0};
  }
  support_size = std::min(support_size, point_count - support_size);
  exact::BigInt result{1};
  for (std::size_t index = 1U; index <= support_size; ++index) {
    result *= point_count - support_size + index;
    result /= index;
  }
  return result;
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] bool source_manifest_contracts_match(
    const ExactPairSupportCheckpointManifest& pair,
    const ExactHigherSupportCheckpointManifest& higher) {
  return pair.point_count == higher.point_count &&
         pair.lbvh_node_count == higher.lbvh_node_count &&
         pair.lbvh_leaf_count == higher.lbvh_leaf_count &&
         pair.requested_maximum_order == higher.requested_maximum_order &&
         pair.effective_maximum_order == higher.effective_maximum_order &&
         pair.maximum_relevant_closed_rank ==
             higher.maximum_relevant_closed_rank;
}

[[nodiscard]] bool source_requirements_match(
    const ExactPairSupportRequirements& pair,
    const ExactHigherSupportRequirements& higher,
    const ExactPairSupportCheckpointManifest& authority) {
  return pair.point_count == authority.point_count &&
         pair.requested_maximum_order ==
             authority.requested_maximum_order &&
         pair.effective_maximum_order ==
             authority.effective_maximum_order &&
         pair.maximum_relevant_closed_rank ==
             authority.maximum_relevant_closed_rank &&
         higher.point_count == authority.point_count &&
         higher.requested_maximum_order ==
             authority.requested_maximum_order &&
         higher.effective_maximum_order ==
             authority.effective_maximum_order &&
         higher.maximum_relevant_closed_rank ==
             authority.maximum_relevant_closed_rank;
}

void attach_h0_orders(
    ExactDirectSupportEvent& event,
    std::size_t effective_maximum_order) {
  if (event.closed_rank == 0U) {
    throw std::logic_error(
        "a direct support event cannot have closed rank zero");
  }
  if (event.closed_rank <= effective_maximum_order) {
    event.birth_order = event.closed_rank;
  }
  if (event.closed_rank >= 2U &&
      event.closed_rank - 1U <= effective_maximum_order) {
    event.saddle_order = event.closed_rank - 1U;
  }
  if (!event.birth_order.has_value() &&
      !event.saddle_order.has_value()) {
    throw std::logic_error(
        "a rank-relevant direct support event has no H0 order");
  }
}

[[nodiscard]] ExactDirectSupportEvent normalize_event(
    const ExactPairSupportEvent& source,
    std::size_t effective_maximum_order) {
  ExactDirectSupportEvent event;
  event.support_size = 2U;
  event.support_ids[0] = source.support_ids[0];
  event.support_ids[1] = source.support_ids[1];
  event.center = source.center;
  event.squared_level = source.squared_level;
  event.interior_ids = source.interior_ids;
  event.closed_rank = source.closed_rank;
  event.exterior_count = source.exterior_count;
  attach_h0_orders(event, effective_maximum_order);
  return event;
}

[[nodiscard]] ExactDirectSupportEvent normalize_event(
    const ExactHigherSupportEvent& source,
    std::size_t effective_maximum_order) {
  if (source.support_size < 3U || source.support_size > 4U) {
    throw std::logic_error(
        "a verified higher-support event has an invalid arity");
  }
  ExactDirectSupportEvent event;
  event.support_size = source.support_size;
  event.support_ids = source.support_ids;
  event.center = source.center;
  event.squared_level = source.squared_level;
  event.interior_ids = source.interior_ids;
  event.closed_rank = source.closed_rank;
  event.exterior_count = source.exterior_count;
  attach_h0_orders(event, effective_maximum_order);
  return event;
}

[[nodiscard]] ExactDirectSupportExtraShellDiagnostic normalize_diagnostic(
    const ExactPairSupportExtraShellDiagnostic& source) {
  ExactDirectSupportExtraShellDiagnostic diagnostic;
  diagnostic.support_size = 2U;
  diagnostic.support_ids[0] = source.support_ids[0];
  diagnostic.support_ids[1] = source.support_ids[1];
  diagnostic.center = source.center;
  diagnostic.squared_level = source.squared_level;
  diagnostic.interior_ids = source.interior_ids;
  diagnostic.shell_count = source.shell_count;
  diagnostic.canonical_extra_shell_witness_id =
      source.canonical_extra_shell_witness_id;
  diagnostic.minimum_possible_closed_rank =
      source.minimum_possible_closed_rank;
  diagnostic.observed_closed_rank = source.observed_closed_rank;
  diagnostic.exterior_count = source.exterior_count;
  return diagnostic;
}

[[nodiscard]] ExactDirectSupportExtraShellDiagnostic normalize_diagnostic(
    const ExactHigherSupportExtraShellDiagnostic& source) {
  if (source.support_size < 3U || source.support_size > 4U) {
    throw std::logic_error(
        "a verified higher-support diagnostic has an invalid arity");
  }
  ExactDirectSupportExtraShellDiagnostic diagnostic;
  diagnostic.support_size = source.support_size;
  diagnostic.support_ids = source.support_ids;
  diagnostic.center = source.center;
  diagnostic.squared_level = source.squared_level;
  diagnostic.interior_ids = source.interior_ids;
  diagnostic.shell_count = source.shell_count;
  diagnostic.canonical_extra_shell_witness_id =
      source.canonical_extra_shell_witness_id;
  diagnostic.minimum_possible_closed_rank =
      source.minimum_possible_closed_rank;
  diagnostic.observed_closed_rank = source.observed_closed_rank;
  diagnostic.exterior_count = source.exterior_count;
  return diagnostic;
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

[[nodiscard]] bool event_less(
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

[[nodiscard]] bool diagnostic_less(
    const ExactDirectSupportExtraShellDiagnostic& left,
    const ExactDirectSupportExtraShellDiagnostic& right) {
  return support_less(left, right);
}

[[nodiscard]] std::size_t arity_index(std::uint8_t support_size) {
  if (support_size < 2U || support_size > 4U) {
    throw std::logic_error(
        "a normalized direct support record has an invalid arity");
  }
  return static_cast<std::size_t>(support_size - 2U);
}

void normalize_terminal_records(
    const ExactPairSupportStreamResult& pair_result,
    const ExactHigherSupportStreamResult& higher_result,
    ExactDirectSupportTerminalFacade& facade) {
  const std::size_t event_count = checked_add(
      pair_result.events.size(),
      higher_result.events.size(),
      "the normalized direct event count overflows size_t");
  const std::size_t diagnostic_count = checked_add(
      pair_result.relevant_extra_shell_diagnostics.size(),
      higher_result.relevant_extra_shell_diagnostics.size(),
      "the normalized direct diagnostic count overflows size_t");
  facade.events.reserve(event_count);
  facade.relevant_extra_shell_diagnostics.reserve(diagnostic_count);

  const std::size_t effective_maximum_order =
      facade.certificate.requirements.effective_maximum_order;
  for (const ExactPairSupportEvent& event : pair_result.events) {
    facade.events.push_back(
        normalize_event(event, effective_maximum_order));
  }
  for (const ExactHigherSupportEvent& event : higher_result.events) {
    facade.events.push_back(
        normalize_event(event, effective_maximum_order));
  }
  for (const ExactPairSupportExtraShellDiagnostic& diagnostic :
       pair_result.relevant_extra_shell_diagnostics) {
    facade.relevant_extra_shell_diagnostics.push_back(
        normalize_diagnostic(diagnostic));
  }
  for (const ExactHigherSupportExtraShellDiagnostic& diagnostic :
       higher_result.relevant_extra_shell_diagnostics) {
    facade.relevant_extra_shell_diagnostics.push_back(
        normalize_diagnostic(diagnostic));
  }

  std::sort(facade.events.begin(), facade.events.end(), event_less);
  for (std::size_t index = 1U; index < facade.events.size(); ++index) {
    if (!event_less(facade.events[index - 1U], facade.events[index])) {
      throw std::logic_error(
          "the direct support facade produced duplicate canonical events");
    }
  }
  std::sort(
      facade.relevant_extra_shell_diagnostics.begin(),
      facade.relevant_extra_shell_diagnostics.end(),
      diagnostic_less);
  for (std::size_t index = 1U;
       index < facade.relevant_extra_shell_diagnostics.size();
       ++index) {
    if (!diagnostic_less(
            facade.relevant_extra_shell_diagnostics[index - 1U],
            facade.relevant_extra_shell_diagnostics[index])) {
      throw std::logic_error(
          "the direct support facade produced duplicate diagnostics");
    }
  }

  for (std::size_t index = 0U; index < facade.events.size(); ++index) {
    ExactDirectSupportEvent& event = facade.events[index];
    event.event_index = index;
    ++facade.certificate
          .arity_certificates[arity_index(event.support_size)]
          .accepted_event_count;
  }
  for (std::size_t index = 0U;
       index < facade.relevant_extra_shell_diagnostics.size();
       ++index) {
    ExactDirectSupportExtraShellDiagnostic& diagnostic =
        facade.relevant_extra_shell_diagnostics[index];
    diagnostic.diagnostic_index = index;
    ++facade.certificate
          .arity_certificates[arity_index(diagnostic.support_size)]
          .relevant_extra_shell_diagnostic_count;
  }

  facade.certificate.normalized_event_count = facade.events.size();
  facade.certificate.normalized_extra_shell_diagnostic_count =
      facade.relevant_extra_shell_diagnostics.size();
  facade.certificate.normalized_records_canonical_and_indexed = true;
  facade.certificate.output_only_normalization_certified = true;
}

}  // namespace

bool ExactDirectSupportTerminalCertificate::terminal_catalog_certified()
    const {
  const bool complete_decision =
      decision == ExactDirectSupportTerminalDecision::
                      complete_direct_support_catalog ||
      decision == ExactDirectSupportTerminalDecision::
                      complete_direct_support_catalog_with_relevant_extra_shell_diagnostics;
  exact::BigInt universe_sum{0};
  exact::BigInt event_count_sum{0};
  exact::BigInt diagnostic_count_sum{0};
  bool arities_certified = true;
  for (std::size_t index = 0U; index < arity_certificates.size(); ++index) {
    const ExactDirectSupportArityTerminalCertificate& arity =
        arity_certificates[index];
    arities_certified =
        arities_certified &&
        arity.support_size == static_cast<std::uint8_t>(index + 2U) &&
        arity.candidate_universe_size_certified &&
        arity.terminal_absence_of_additional_supports_certified;
    universe_sum += arity.exact_candidate_universe_size;
    event_count_sum += arity.accepted_event_count;
    diagnostic_count_sum += arity.relevant_extra_shell_diagnostic_count;
  }
  return schema_version ==
             direct_support_terminal_certificate_schema_version &&
         complete_decision &&
         scope == ExactDirectSupportTerminalScope::
                      direct_support_catalog_arities_two_through_four_only &&
         source_authorities_match && source_requirements_match &&
         pair_result_freshly_replayed &&
         higher_result_freshly_replayed && pair_stream_terminal &&
         higher_stream_terminal && all_arities_terminal &&
         arities_certified && exact_candidate_universe_size_certified &&
         exact_candidate_universe_size == universe_sum &&
         event_count_sum == normalized_event_count &&
         diagnostic_count_sum ==
             normalized_extra_shell_diagnostic_count &&
         normalized_records_canonical_and_indexed &&
         output_only_normalization_certified &&
         no_forbidden_global_structure_materialized &&
         !hierarchy_reduction_performed &&
         !common_durable_checkpoint_certified &&
         !hierarchy_or_forest_certified && !public_status_claimed;
}

bool ExactDirectSupportTerminalFacade::terminal_catalog_certified() const {
  return certificate.terminal_catalog_certified() &&
         certificate.normalized_event_count == events.size() &&
         certificate.normalized_extra_shell_diagnostic_count ==
             relevant_extra_shell_diagnostics.size();
}

ExactDirectSupportTerminalFacade build_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactDirectSupportTerminalBudget& budget,
    const ExactPairSupportStreamResult& pair_result,
    const ExactHigherSupportStreamResult& higher_result) {
  const ExactPairSupportCheckpointManifest pair_manifest =
      make_exact_pair_support_checkpoint_manifest(
          index, cloud, requested_maximum_order);
  const ExactHigherSupportCheckpointManifest higher_manifest =
      make_exact_higher_support_checkpoint_manifest(
          index, cloud, requested_maximum_order);
  const ExactPairSupportStreamVerification pair_verification =
      verify_exact_pair_support_stream(
          index,
          cloud,
          requested_maximum_order,
          budget.pair,
          pair_result);
  const ExactHigherSupportStreamVerification higher_verification =
      verify_exact_higher_support_stream(
          index,
          cloud,
          requested_maximum_order,
          budget.higher,
          higher_result);

  ExactDirectSupportTerminalFacade facade;
  ExactDirectSupportTerminalCertificate& certificate = facade.certificate;
  certificate.requested_budget = budget;
  certificate.requirements = ExactDirectSupportTerminalRequirements{
      pair_manifest.point_count,
      pair_manifest.requested_maximum_order,
      pair_manifest.effective_maximum_order,
      pair_manifest.maximum_relevant_closed_rank};
  certificate.pair_canonical_cloud_digest =
      pair_manifest.canonical_cloud_digest;
  certificate.higher_canonical_cloud_digest =
      higher_manifest.canonical_cloud_digest;
  certificate.pair_lbvh_digest = pair_manifest.lbvh_digest;
  certificate.higher_lbvh_digest = higher_manifest.lbvh_digest;
  certificate.pair_semantic_digest = pair_manifest.semantic_digest;
  certificate.higher_semantic_digest = higher_manifest.semantic_digest;
  certificate.source_authorities_match =
      source_manifest_contracts_match(pair_manifest, higher_manifest);
  certificate.source_requirements_match = source_requirements_match(
      pair_result.requirements,
      higher_result.requirements,
      pair_manifest);
  certificate.pair_result_freshly_replayed =
      pair_verification.result_certified;
  certificate.higher_result_freshly_replayed =
      higher_verification.result_certified;
  certificate.pair_stream_terminal =
      certificate.pair_result_freshly_replayed &&
      pair_result.stream_complete() &&
      pair_result.absence_of_additional_pair_supports_certified();
  certificate.higher_stream_terminal =
      certificate.higher_result_freshly_replayed &&
      higher_result.stream_complete() &&
      higher_result.absence_of_additional_higher_supports_certified();
  certificate.no_forbidden_global_structure_materialized =
      pair_result.no_forbidden_global_structure_materialized &&
      higher_result.no_forbidden_global_structure_materialized;
  certificate.hierarchy_reduction_performed =
      pair_result.hierarchy_reduction_performed ||
      higher_result.hierarchy_reduction_performed;
  certificate.scope = ExactDirectSupportTerminalScope::
      direct_support_catalog_arities_two_through_four_only;

  for (std::size_t index_value = 0U; index_value < 3U; ++index_value) {
    ExactDirectSupportArityTerminalCertificate& arity =
        certificate.arity_certificates[index_value];
    arity.support_size =
        static_cast<std::uint8_t>(index_value + 2U);
    arity.exact_candidate_universe_size = exact_binomial(
        cloud.size(), static_cast<std::size_t>(arity.support_size));
    certificate.exact_candidate_universe_size +=
        arity.exact_candidate_universe_size;
  }

  const bool pair_universe_certified =
      certificate.pair_result_freshly_replayed &&
      exact::BigInt{pair_result.audit.total_pair_count} ==
          certificate.arity_certificates[0].exact_candidate_universe_size;
  const bool higher_universe_certified =
      certificate.higher_result_freshly_replayed &&
      higher_result.audit.exact_bigint_universe_certified &&
      higher_result.audit.total_support_count ==
          certificate.arity_certificates[1].exact_candidate_universe_size +
              certificate.arity_certificates[2]
                  .exact_candidate_universe_size;
  certificate.arity_certificates[0]
      .candidate_universe_size_certified = pair_universe_certified;
  certificate.arity_certificates[1]
      .candidate_universe_size_certified = higher_universe_certified;
  certificate.arity_certificates[2]
      .candidate_universe_size_certified = higher_universe_certified;
  certificate.exact_candidate_universe_size_certified =
      pair_universe_certified && higher_universe_certified;

  if (!certificate.pair_result_freshly_replayed ||
      !certificate.higher_result_freshly_replayed ||
      !certificate.source_authorities_match ||
      !certificate.source_requirements_match) {
    certificate.decision =
        ExactDirectSupportTerminalDecision::source_result_not_certified;
    return facade;
  }

  certificate.all_arities_terminal =
      certificate.pair_stream_terminal &&
      certificate.higher_stream_terminal &&
      certificate.exact_candidate_universe_size_certified &&
      certificate.no_forbidden_global_structure_materialized &&
      !certificate.hierarchy_reduction_performed;
  if (!certificate.all_arities_terminal) {
    certificate.decision =
        ExactDirectSupportTerminalDecision::source_stream_not_terminal;
    return facade;
  }

  for (ExactDirectSupportArityTerminalCertificate& arity :
       certificate.arity_certificates) {
    arity.terminal_absence_of_additional_supports_certified = true;
  }
  normalize_terminal_records(pair_result, higher_result, facade);
  certificate.decision =
      facade.relevant_extra_shell_diagnostics.empty()
          ? ExactDirectSupportTerminalDecision::
                complete_direct_support_catalog
          : ExactDirectSupportTerminalDecision::
                complete_direct_support_catalog_with_relevant_extra_shell_diagnostics;
  if (!facade.terminal_catalog_certified()) {
    throw std::logic_error(
        "the composed direct support terminal certificate is inconsistent");
  }
  return facade;
}

ExactDirectSupportTerminalVerification
verify_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactDirectSupportTerminalBudget& budget,
    const ExactPairSupportStreamResult& pair_result,
    const ExactHigherSupportStreamResult& higher_result,
    const ExactDirectSupportTerminalFacade& observed) {
  const ExactDirectSupportTerminalFacade expected =
      build_exact_direct_support_terminal_facade(
          index,
          cloud,
          requested_maximum_order,
          budget,
          pair_result,
          higher_result);
  ExactDirectSupportTerminalVerification verification;
  verification.pair_source_result_freshly_replayed =
      expected.certificate.pair_result_freshly_replayed;
  verification.higher_source_result_freshly_replayed =
      expected.certificate.higher_result_freshly_replayed;
  verification.source_terminality_certified =
      expected.certificate.pair_stream_terminal &&
      expected.certificate.higher_stream_terminal &&
      expected.certificate.all_arities_terminal;
  verification.certificate_certified =
      observed.certificate == expected.certificate;
  verification.normalized_events_certified =
      observed.events == expected.events;
  verification.normalized_extra_shell_diagnostics_certified =
      observed.relevant_extra_shell_diagnostics ==
      expected.relevant_extra_shell_diagnostics;
  verification.terminal_claim_certified =
      observed.terminal_catalog_certified() ==
      expected.terminal_catalog_certified();
  verification.fresh_composition_certified = observed == expected;
  verification.result_certified =
      expected.terminal_catalog_certified() &&
      verification.pair_source_result_freshly_replayed &&
      verification.higher_source_result_freshly_replayed &&
      verification.source_terminality_certified &&
      verification.certificate_certified &&
      verification.normalized_events_certified &&
      verification.normalized_extra_shell_diagnostics_certified &&
      verification.terminal_claim_certified &&
      verification.fresh_composition_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
