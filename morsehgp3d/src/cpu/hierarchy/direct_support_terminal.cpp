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

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
    if (value > static_cast<std::size_t>(
                    std::numeric_limits<std::uint64_t>::max())) {
      throw std::overflow_error(message);
    }
  }
  return static_cast<std::uint64_t>(value);
}

class DigestWriter {
 public:
  explicit DigestWriter(std::string_view domain) { text(domain); }

  void byte(std::uint8_t value) {
    builder_.update(std::span<const std::uint8_t>{&value, 1U});
  }

  void u64(std::uint64_t value) {
    std::array<std::uint8_t, 8U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
  }

  void size(std::size_t value) {
    u64(checked_u64(
        value, "a sparse pair digest size does not fit uint64"));
  }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void identifier(const contract::CanonicalId& identifier) {
    builder_.update(identifier.bytes());
  }

  void center(const exact::ExactCenter3& value) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      text(value.coordinate(axis).canonical_key());
    }
  }

  void level(const exact::ExactLevel& value) {
    text(value.canonical_key());
  }

  [[nodiscard]] contract::CanonicalId finalize() {
    return builder_.finalize();
  }

 private:
  contract::CanonicalSha256Builder builder_;
};

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

[[nodiscard]] bool source_requirements_match(
    const ExactPairSupportRequirements& pair,
    const ExactHigherSupportCheckpointManifest& higher,
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
    ExactPairSupportEvent&& source,
    std::size_t effective_maximum_order) {
  ExactDirectSupportEvent event;
  event.support_size = 2U;
  event.support_ids[0] = source.support_ids[0];
  event.support_ids[1] = source.support_ids[1];
  event.center = std::move(source.center);
  event.squared_level = std::move(source.squared_level);
  event.interior_ids = std::move(source.interior_ids);
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

[[nodiscard]] ExactDirectSupportEvent normalize_event(
    ExactHigherSupportEvent&& source,
    std::size_t effective_maximum_order) {
  if (source.support_size < 3U || source.support_size > 4U) {
    throw std::logic_error(
        "a sealed higher-support event has an invalid arity");
  }
  ExactDirectSupportEvent event;
  event.support_size = source.support_size;
  event.support_ids = source.support_ids;
  event.center = std::move(source.center);
  event.squared_level = std::move(source.squared_level);
  event.interior_ids = std::move(source.interior_ids);
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
    ExactPairSupportExtraShellDiagnostic&& source) {
  ExactDirectSupportExtraShellDiagnostic diagnostic;
  diagnostic.support_size = 2U;
  diagnostic.support_ids[0] = source.support_ids[0];
  diagnostic.support_ids[1] = source.support_ids[1];
  diagnostic.center = std::move(source.center);
  diagnostic.squared_level = std::move(source.squared_level);
  diagnostic.interior_ids = std::move(source.interior_ids);
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

[[nodiscard]] ExactDirectSupportExtraShellDiagnostic normalize_diagnostic(
    ExactHigherSupportExtraShellDiagnostic&& source) {
  if (source.support_size < 3U || source.support_size > 4U) {
    throw std::logic_error(
        "a sealed higher-support diagnostic has an invalid arity");
  }
  ExactDirectSupportExtraShellDiagnostic diagnostic;
  diagnostic.support_size = source.support_size;
  diagnostic.support_ids = source.support_ids;
  diagnostic.center = std::move(source.center);
  diagnostic.squared_level = std::move(source.squared_level);
  diagnostic.interior_ids = std::move(source.interior_ids);
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

[[nodiscard]] contract::CanonicalId normalized_terminal_output_digest(
    const ExactDirectSupportTerminalFacade& facade);

void finalize_terminal_records(ExactDirectSupportTerminalFacade& facade) {
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
  facade.certificate.normalized_terminal_output_digest =
      normalized_terminal_output_digest(facade);
}

[[nodiscard]] bool normalized_terminal_records_valid(
    const ExactDirectSupportTerminalFacade& facade) {
  std::array<std::size_t, 3U> event_counts{};
  std::array<std::size_t, 3U> diagnostic_counts{};
  const std::size_t effective_maximum_order =
      facade.certificate.requirements.effective_maximum_order;
  for (std::size_t index = 0U; index < facade.events.size(); ++index) {
    const ExactDirectSupportEvent& event = facade.events[index];
    if (event.event_index != index || event.support_size < 2U ||
        event.support_size > 4U || event.closed_rank == 0U ||
        (index != 0U && !event_less(facade.events[index - 1U], event))) {
      return false;
    }
    for (std::size_t support_index = 1U;
         support_index < static_cast<std::size_t>(event.support_size);
         ++support_index) {
      if (event.support_ids[support_index - 1U] >=
          event.support_ids[support_index]) {
        return false;
      }
    }
    if (!std::is_sorted(event.interior_ids.begin(), event.interior_ids.end()) ||
        std::adjacent_find(
            event.interior_ids.begin(), event.interior_ids.end()) !=
            event.interior_ids.end()) {
      return false;
    }
    const bool birth_expected =
        event.closed_rank <= effective_maximum_order;
    const bool saddle_expected =
        event.closed_rank >= 2U &&
        event.closed_rank - 1U <= effective_maximum_order;
    if ((!birth_expected && !saddle_expected) ||
        (birth_expected
             ? event.birth_order != event.closed_rank
             : event.birth_order.has_value()) ||
        (saddle_expected
             ? event.saddle_order != event.closed_rank - 1U
             : event.saddle_order.has_value())) {
      return false;
    }
    ++event_counts[static_cast<std::size_t>(event.support_size - 2U)];
  }

  for (std::size_t index = 0U;
       index < facade.relevant_extra_shell_diagnostics.size();
       ++index) {
    const ExactDirectSupportExtraShellDiagnostic& diagnostic =
        facade.relevant_extra_shell_diagnostics[index];
    if (diagnostic.diagnostic_index != index ||
        diagnostic.support_size < 2U || diagnostic.support_size > 4U ||
        (index != 0U &&
         !diagnostic_less(
             facade.relevant_extra_shell_diagnostics[index - 1U],
             diagnostic))) {
      return false;
    }
    for (std::size_t support_index = 1U;
         support_index < static_cast<std::size_t>(diagnostic.support_size);
         ++support_index) {
      if (diagnostic.support_ids[support_index - 1U] >=
          diagnostic.support_ids[support_index]) {
        return false;
      }
    }
    if (!std::is_sorted(
            diagnostic.interior_ids.begin(),
            diagnostic.interior_ids.end()) ||
        std::adjacent_find(
            diagnostic.interior_ids.begin(),
            diagnostic.interior_ids.end()) !=
            diagnostic.interior_ids.end()) {
      return false;
    }
    ++diagnostic_counts[
        static_cast<std::size_t>(diagnostic.support_size - 2U)];
  }

  for (std::size_t index = 0U; index < event_counts.size(); ++index) {
    if (event_counts[index] !=
            facade.certificate.arity_certificates[index]
                .accepted_event_count ||
        diagnostic_counts[index] !=
            facade.certificate.arity_certificates[index]
                .relevant_extra_shell_diagnostic_count) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] contract::CanonicalId pair_terminal_output_digest(
    const ExactDirectSupportTerminalFacade& facade,
    std::string_view domain) {
  DigestWriter writer{domain};
  const std::size_t pair_event_count = static_cast<std::size_t>(
      std::count_if(
          facade.events.begin(),
          facade.events.end(),
          [](const ExactDirectSupportEvent& event) {
            return event.support_size == 2U;
          }));
  writer.size(pair_event_count);
  for (const ExactDirectSupportEvent& event : facade.events) {
    if (event.support_size != 2U) {
      continue;
    }
    writer.byte(1U);
    writer.u64(event.support_ids[0]);
    writer.u64(event.support_ids[1]);
    writer.center(event.center);
    writer.level(event.squared_level);
    writer.size(event.interior_ids.size());
    for (const spatial::PointId point_id : event.interior_ids) {
      writer.u64(point_id);
    }
    writer.size(event.closed_rank);
    writer.size(event.exterior_count);
  }
  const std::size_t pair_diagnostic_count = static_cast<std::size_t>(
      std::count_if(
          facade.relevant_extra_shell_diagnostics.begin(),
          facade.relevant_extra_shell_diagnostics.end(),
          [](const ExactDirectSupportExtraShellDiagnostic& diagnostic) {
            return diagnostic.support_size == 2U;
          }));
  writer.size(pair_diagnostic_count);
  for (const ExactDirectSupportExtraShellDiagnostic& diagnostic :
       facade.relevant_extra_shell_diagnostics) {
    if (diagnostic.support_size != 2U) {
      continue;
    }
    writer.byte(2U);
    writer.u64(diagnostic.support_ids[0]);
    writer.u64(diagnostic.support_ids[1]);
    writer.center(diagnostic.center);
    writer.level(diagnostic.squared_level);
    writer.size(diagnostic.interior_ids.size());
    for (const spatial::PointId point_id : diagnostic.interior_ids) {
      writer.u64(point_id);
    }
    writer.size(diagnostic.shell_count);
    writer.u64(diagnostic.canonical_extra_shell_witness_id);
    writer.size(diagnostic.minimum_possible_closed_rank);
    writer.size(diagnostic.observed_closed_rank);
    writer.size(diagnostic.exterior_count);
  }
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId normalized_terminal_output_digest(
    const ExactDirectSupportTerminalFacade& facade) {
  DigestWriter writer{
      "MorseHGP3D/direct-support/normalized-terminal-output/v1"};
  writer.size(facade.events.size());
  for (const ExactDirectSupportEvent& event : facade.events) {
    writer.size(event.event_index);
    writer.byte(event.support_size);
    for (const spatial::PointId point_id : event.support_ids) {
      writer.u64(point_id);
    }
    writer.center(event.center);
    writer.level(event.squared_level);
    writer.size(event.interior_ids.size());
    for (const spatial::PointId point_id : event.interior_ids) {
      writer.u64(point_id);
    }
    writer.size(event.closed_rank);
    writer.size(event.exterior_count);
    writer.byte(event.birth_order.has_value() ? 1U : 0U);
    if (event.birth_order.has_value()) {
      writer.size(*event.birth_order);
    }
    writer.byte(event.saddle_order.has_value() ? 1U : 0U);
    if (event.saddle_order.has_value()) {
      writer.size(*event.saddle_order);
    }
  }
  writer.size(facade.relevant_extra_shell_diagnostics.size());
  for (const ExactDirectSupportExtraShellDiagnostic& diagnostic :
       facade.relevant_extra_shell_diagnostics) {
    writer.size(diagnostic.diagnostic_index);
    writer.byte(diagnostic.support_size);
    for (const spatial::PointId point_id : diagnostic.support_ids) {
      writer.u64(point_id);
    }
    writer.center(diagnostic.center);
    writer.level(diagnostic.squared_level);
    writer.size(diagnostic.interior_ids.size());
    for (const spatial::PointId point_id : diagnostic.interior_ids) {
      writer.u64(point_id);
    }
    writer.size(diagnostic.shell_count);
    writer.u64(diagnostic.canonical_extra_shell_witness_id);
    writer.size(diagnostic.minimum_possible_closed_rank);
    writer.size(diagnostic.observed_closed_rank);
    writer.size(diagnostic.exterior_count);
  }
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId sparse_pair_terminal_output_digest(
    const ExactDirectSupportTerminalFacade& facade) {
  return pair_terminal_output_digest(
      facade,
      "MorseHGP3D/phase14Q/P8m/sparse-pair-terminal-output/v1");
}

[[nodiscard]] contract::CanonicalId
transactional_pair_terminal_output_digest(
    const ExactDirectSupportTerminalFacade& facade) {
  return pair_terminal_output_digest(
      facade,
      "MorseHGP3D/phase15/direct-support/transactional-pair-terminal-output/"
      "v1");
}

[[nodiscard]] contract::CanonicalId sparse_pair_semantic_digest(
    const contract::CanonicalId& canonical_cloud_digest,
    const contract::CanonicalId& lbvh_digest,
    const ExactDirectSupportTerminalRequirements& requirements,
    const contract::CanonicalId& output_digest) {
  DigestWriter writer{
      "MorseHGP3D/phase14Q/P8m/sparse-pair-semantic/v1"};
  writer.text(ExactSparseAnchoredPairTerminalAuthority::proof_basis);
  writer.identifier(canonical_cloud_digest);
  writer.identifier(lbvh_digest);
  writer.size(requirements.point_count);
  writer.size(requirements.requested_maximum_order);
  writer.size(requirements.effective_maximum_order);
  writer.size(requirements.maximum_relevant_closed_rank);
  writer.identifier(output_digest);
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId transactional_pair_semantic_digest(
    const contract::CanonicalId& canonical_cloud_digest,
    const contract::CanonicalId& lbvh_digest,
    const ExactDirectSupportTerminalRequirements& requirements,
    const ExactDirectPairTerminalAudit& audit,
    const contract::CanonicalId& output_digest) {
  DigestWriter writer{
      "MorseHGP3D/phase15/direct-support/transactional-pair-semantic/v1"};
  writer.text(ExactDirectPairTerminalAuthority::proof_basis);
  writer.identifier(canonical_cloud_digest);
  writer.identifier(lbvh_digest);
  writer.size(requirements.point_count);
  writer.size(requirements.requested_maximum_order);
  writer.size(requirements.effective_maximum_order);
  writer.size(requirements.maximum_relevant_closed_rank);
  writer.u64(audit.schema_version);
  writer.byte(static_cast<std::uint8_t>(audit.source_kind));
  writer.size(audit.point_count);
  writer.size(audit.requested_maximum_order);
  writer.size(audit.effective_maximum_order);
  writer.size(audit.maximum_closed_rank);
  writer.size(audit.unordered_pair_universe_size);
  writer.size(audit.source_prune_receipt_count);
  writer.size(audit.pruned_unordered_pair_mass);
  writer.size(audit.source_terminal_pair_count);
  writer.u64(audit.source_final_cut_digest);
  writer.size(audit.classification_started_count);
  writer.size(audit.classification_terminal_count);
  writer.size(audit.classification_node_visit_count);
  writer.size(audit.classification_budget_exhaustion_count);
  writer.size(audit.above_rank_count);
  writer.size(audit.accepted_event_count);
  writer.size(audit.relevant_extra_shell_diagnostic_count);
  writer.size(audit.emitted_record_count);
  writer.size(audit.emitted_point_id_reference_count);
  writer.byte(audit.source_partition_validated ? 1U : 0U);
  writer.byte(audit.every_prune_recertified ? 1U : 0U);
  writer.byte(audit.unordered_pair_coverage_certified ? 1U : 0U);
  writer.byte(
      audit.terminal_pair_classification_partition_certified ? 1U : 0U);
  writer.byte(audit.output_partition_certified ? 1U : 0U);
  writer.byte(audit.retained_records_certified ? 1U : 0U);
  writer.byte(audit.qualified_cuda_execution ? 1U : 0U);
  writer.byte(audit.host_fake_execution ? 1U : 0U);
  writer.byte(
      audit.no_forbidden_global_structure_materialized ? 1U : 0U);
  writer.byte(audit.global_pair_matrix_materialized ? 1U : 0U);
  writer.byte(
      audit.ordinary_or_higher_order_delaunay_materialized ? 1U : 0U);
  writer.byte(
      audit.global_facet_coface_or_incidence_arena_materialized ? 1U : 0U);
  writer.byte(audit.hierarchy_reduction_performed ? 1U : 0U);
  writer.byte(audit.public_status_claimed ? 1U : 0U);
  writer.byte(audit.complete ? 1U : 0U);
  writer.byte(audit.poisoned ? 1U : 0U);
  writer.identifier(output_digest);
  return writer.finalize();
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
  finalize_terminal_records(facade);
}

void normalize_terminal_records(
    std::vector<ExactDirectPairTerminalRecord>&& pair_records,
    std::size_t pair_event_count,
    std::size_t pair_diagnostic_count,
    std::vector<ExactHigherSupportTerminalSegment>&& higher_segments,
    std::size_t higher_event_count,
    std::size_t higher_diagnostic_count,
    ExactDirectSupportTerminalFacade& facade) {
  const std::size_t event_count = checked_add(
      pair_event_count,
      higher_event_count,
      "the authority-backed direct event count overflows size_t");
  const std::size_t diagnostic_count = checked_add(
      pair_diagnostic_count,
      higher_diagnostic_count,
      "the authority-backed direct diagnostic count overflows size_t");
  facade.events.reserve(event_count);
  facade.relevant_extra_shell_diagnostics.reserve(diagnostic_count);

  const std::size_t effective_maximum_order =
      facade.certificate.requirements.effective_maximum_order;
  std::size_t observed_pair_event_count = 0U;
  std::size_t observed_pair_diagnostic_count = 0U;
  for (ExactDirectPairTerminalRecord& record : pair_records) {
    if (auto* event = std::get_if<ExactPairSupportEvent>(&record)) {
      observed_pair_event_count = checked_add(
          observed_pair_event_count,
          1U,
          "the sealed pair event count overflows size_t");
      facade.events.push_back(
          normalize_event(std::move(*event), effective_maximum_order));
    } else {
      observed_pair_diagnostic_count = checked_add(
          observed_pair_diagnostic_count,
          1U,
          "the sealed pair diagnostic count overflows size_t");
      facade.relevant_extra_shell_diagnostics.push_back(
          normalize_diagnostic(std::move(
              std::get<ExactPairSupportExtraShellDiagnostic>(record))));
    }
  }
  if (observed_pair_event_count != pair_event_count ||
      observed_pair_diagnostic_count != pair_diagnostic_count) {
    throw std::logic_error(
        "the sealed pair record counts are inconsistent");
  }

  std::size_t observed_higher_event_count = 0U;
  std::size_t observed_higher_diagnostic_count = 0U;
  for (ExactHigherSupportTerminalSegment& segment : higher_segments) {
    observed_higher_event_count = checked_add(
        observed_higher_event_count,
        segment.events.size(),
        "the sealed higher-support event count overflows size_t");
    observed_higher_diagnostic_count = checked_add(
        observed_higher_diagnostic_count,
        segment.relevant_extra_shell_diagnostics.size(),
        "the sealed higher-support diagnostic count overflows size_t");
    for (ExactHigherSupportEvent& event : segment.events) {
      facade.events.push_back(
          normalize_event(std::move(event), effective_maximum_order));
    }
    for (ExactHigherSupportExtraShellDiagnostic& diagnostic :
         segment.relevant_extra_shell_diagnostics) {
      facade.relevant_extra_shell_diagnostics.push_back(
          normalize_diagnostic(std::move(diagnostic)));
    }
  }
  if (observed_higher_event_count != higher_event_count ||
      observed_higher_diagnostic_count != higher_diagnostic_count) {
    throw std::logic_error(
        "the sealed higher-support segment counts are inconsistent");
  }
  finalize_terminal_records(facade);
}

void normalize_terminal_records(
    const ExactPairSupportStreamResult& pair_result,
    std::vector<ExactHigherSupportTerminalSegment>&& higher_segments,
    std::size_t higher_event_count,
    std::size_t higher_diagnostic_count,
    ExactDirectSupportTerminalFacade& facade) {
  const std::size_t event_count = checked_add(
      pair_result.events.size(),
      higher_event_count,
      "the sealed normalized direct event count overflows size_t");
  const std::size_t diagnostic_count = checked_add(
      pair_result.relevant_extra_shell_diagnostics.size(),
      higher_diagnostic_count,
      "the sealed normalized direct diagnostic count overflows size_t");
  facade.events.reserve(event_count);
  facade.relevant_extra_shell_diagnostics.reserve(diagnostic_count);

  const std::size_t effective_maximum_order =
      facade.certificate.requirements.effective_maximum_order;
  for (const ExactPairSupportEvent& event : pair_result.events) {
    facade.events.push_back(
        normalize_event(event, effective_maximum_order));
  }
  for (const ExactPairSupportExtraShellDiagnostic& diagnostic :
       pair_result.relevant_extra_shell_diagnostics) {
    facade.relevant_extra_shell_diagnostics.push_back(
        normalize_diagnostic(diagnostic));
  }
  std::size_t observed_higher_event_count = 0U;
  std::size_t observed_higher_diagnostic_count = 0U;
  for (ExactHigherSupportTerminalSegment& segment : higher_segments) {
    observed_higher_event_count = checked_add(
        observed_higher_event_count,
        segment.events.size(),
        "the sealed higher-support event count overflows size_t");
    observed_higher_diagnostic_count = checked_add(
        observed_higher_diagnostic_count,
        segment.relevant_extra_shell_diagnostics.size(),
        "the sealed higher-support diagnostic count overflows size_t");
    for (ExactHigherSupportEvent& event : segment.events) {
      facade.events.push_back(
          normalize_event(
              std::move(event), effective_maximum_order));
    }
    for (ExactHigherSupportExtraShellDiagnostic& diagnostic :
         segment.relevant_extra_shell_diagnostics) {
      facade.relevant_extra_shell_diagnostics.push_back(
          normalize_diagnostic(std::move(diagnostic)));
    }
  }
  if (observed_higher_event_count != higher_event_count ||
      observed_higher_diagnostic_count != higher_diagnostic_count) {
    throw std::logic_error(
        "the sealed higher-support segment counts are inconsistent");
  }
  finalize_terminal_records(facade);
}

}  // namespace

bool ExactDirectSupportTerminalCertificate::terminal_catalog_certified()
    const {
  const bool complete_decision =
      decision == ExactDirectSupportTerminalDecision::
                      complete_direct_support_catalog ||
      decision == ExactDirectSupportTerminalDecision::
                      complete_direct_support_catalog_with_relevant_extra_shell_diagnostics;
  const bool no_sparse_session_pair_provenance =
      pair_sparse_schedule_config ==
          ExactMortonGroupedAnchoredPairScheduleConfig{} &&
      pair_sparse_total_capacity ==
          ExactSparseAnchoredPairSessionTotalCapacity{} &&
      pair_sparse_maximum_closed_rank == 0U &&
      pair_directed_pair_universe_size == 0U &&
      pair_authenticated_pruned_directed_pair_count == 0U &&
      pair_orientation_check_count == 0U &&
      pair_reverse_or_self_orientation_skip_count == 0U &&
      pair_admitted_candidate_count == 0U &&
      pair_classification_terminal_count == 0U &&
      pair_above_rank_count == 0U && pair_terminal_record_count == 0U &&
      !pair_sparse_directed_coverage_certified &&
      !pair_sparse_orientation_partition_certified &&
      !pair_sparse_classification_partition_certified &&
      !pair_sparse_output_partition_certified;
  const bool no_terminal_authority_pair_provenance =
      !pair_terminal_authority_consumed &&
      !pair_terminal_records_captured_once &&
      pair_terminal_output_digest == contract::CanonicalId{};
  const bool no_transactional_pair_provenance =
      pair_transactional_audit == ExactDirectPairTerminalAudit{};
  const bool fresh_pair_source =
      pair_source_kind ==
          ExactDirectSupportPairSourceKind::fresh_resident_replay &&
      pair_legacy_budget_applicable && pair_result_freshly_replayed &&
      no_sparse_session_pair_provenance &&
      no_terminal_authority_pair_provenance &&
      no_transactional_pair_provenance;
  const exact::BigInt directed_pair_universe{
      pair_directed_pair_universe_size};
  const exact::BigInt point_count{requirements.point_count};
  const bool triangular_pair_schedule =
      pair_sparse_schedule_config.use_triangular_block_pair_schedule;
  const bool sealed_pair_coverage_identity = triangular_pair_schedule
      ? exact::BigInt{pair_authenticated_pruned_directed_pair_count} +
                exact::BigInt{2U} * pair_admitted_candidate_count +
                point_count ==
            directed_pair_universe
      : exact::BigInt{pair_authenticated_pruned_directed_pair_count} +
                pair_orientation_check_count ==
            directed_pair_universe;
  const bool sealed_pair_orientation_identity = triangular_pair_schedule
      ? pair_admitted_candidate_count == pair_orientation_check_count &&
          pair_reverse_or_self_orientation_skip_count == 0U
      : exact::BigInt{pair_admitted_candidate_count} +
                pair_reverse_or_self_orientation_skip_count ==
            pair_orientation_check_count;
  const bool sealed_pair_source =
      pair_source_kind ==
          ExactDirectSupportPairSourceKind::
              sealed_sparse_anchored_session &&
      !pair_legacy_budget_applicable && !pair_result_freshly_replayed &&
      no_transactional_pair_provenance &&
      requested_budget.pair == ExactPairSupportStreamBudget{} &&
      pair_sparse_maximum_closed_rank ==
          std::max<std::size_t>(
              2U, requirements.maximum_relevant_closed_rank) &&
      pair_sparse_schedule_config !=
          ExactMortonGroupedAnchoredPairScheduleConfig{} &&
      pair_sparse_directed_coverage_certified &&
      pair_sparse_orientation_partition_certified &&
      pair_sparse_classification_partition_certified &&
      pair_sparse_output_partition_certified &&
      pair_terminal_authority_consumed &&
      pair_terminal_records_captured_once &&
      pair_terminal_output_digest != contract::CanonicalId{} &&
      pair_semantic_digest != contract::CanonicalId{} &&
      directed_pair_universe == point_count * point_count &&
      sealed_pair_coverage_identity && sealed_pair_orientation_identity &&
      pair_admitted_candidate_count ==
          pair_classification_terminal_count &&
      exact::BigInt{pair_above_rank_count} + pair_terminal_record_count ==
          pair_classification_terminal_count;
  const bool transactional_audit_matches_requirements =
      pair_transactional_audit.point_count == requirements.point_count &&
      pair_transactional_audit.requested_maximum_order ==
          requirements.requested_maximum_order &&
      pair_transactional_audit.effective_maximum_order ==
          requirements.effective_maximum_order &&
      pair_transactional_audit.maximum_closed_rank ==
          requirements.maximum_relevant_closed_rank;
  const bool sealed_transactional_pair_source =
      pair_source_kind ==
          ExactDirectSupportPairSourceKind::
              sealed_transactional_pair_terminal_authority &&
      !pair_legacy_budget_applicable && !pair_result_freshly_replayed &&
      requested_budget.pair == ExactPairSupportStreamBudget{} &&
      no_sparse_session_pair_provenance &&
      pair_terminal_authority_consumed &&
      pair_terminal_records_captured_once &&
      pair_terminal_output_digest != contract::CanonicalId{} &&
      pair_semantic_digest != contract::CanonicalId{} &&
      pair_transactional_audit.terminal_identities_hold() &&
      transactional_audit_matches_requirements &&
      exact::BigInt{
          pair_transactional_audit.unordered_pair_universe_size} ==
          arity_certificates[0].exact_candidate_universe_size &&
      pair_transactional_audit.accepted_event_count ==
          arity_certificates[0].accepted_event_count &&
      pair_transactional_audit.relevant_extra_shell_diagnostic_count ==
          arity_certificates[0]
              .relevant_extra_shell_diagnostic_count &&
      exact::BigInt{pair_transactional_audit.emitted_record_count} ==
          exact::BigInt{arity_certificates[0].accepted_event_count} +
              arity_certificates[0]
                  .relevant_extra_shell_diagnostic_count;
  const bool fresh_higher_source =
      higher_source_kind ==
          ExactDirectSupportHigherSourceKind::fresh_resident_replay &&
      higher_result_freshly_replayed &&
      higher_maximum_chunk_count == 0U &&
      higher_anchored_chunk_count == 0U &&
      !higher_root_anchored_run_certified &&
      !higher_terminal_authority_consumed &&
      !higher_terminal_records_captured_once &&
      higher_output_chain_digest == contract::CanonicalId{} &&
      higher_terminal_checkpoint_digest == contract::CanonicalId{};
  const bool sealed_higher_source =
      higher_source_kind ==
          ExactDirectSupportHigherSourceKind::
              sealed_anchored_fixed_chunk_run &&
      !higher_result_freshly_replayed &&
      higher_anchored_chunk_count <= higher_maximum_chunk_count &&
      higher_root_anchored_run_certified &&
      higher_terminal_authority_consumed &&
      higher_terminal_records_captured_once &&
      higher_output_chain_digest != contract::CanonicalId{} &&
      higher_terminal_checkpoint_digest != contract::CanonicalId{};
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
         (fresh_pair_source ? 1U : 0U) +
                 (sealed_pair_source ? 1U : 0U) +
                 (sealed_transactional_pair_source ? 1U : 0U) ==
             1U &&
         (fresh_higher_source != sealed_higher_source) &&
         pair_stream_terminal &&
         higher_stream_terminal && all_arities_terminal &&
         arities_certified && exact_candidate_universe_size_certified &&
         exact_candidate_universe_size == universe_sum &&
         (!sealed_pair_source ||
          exact::BigInt{pair_terminal_record_count} ==
              exact::BigInt{
                  arity_certificates[0].accepted_event_count} +
                  arity_certificates[0]
                      .relevant_extra_shell_diagnostic_count) &&
         event_count_sum == normalized_event_count &&
         diagnostic_count_sum ==
             normalized_extra_shell_diagnostic_count &&
         normalized_records_canonical_and_indexed &&
         output_only_normalization_certified &&
         normalized_terminal_output_digest != contract::CanonicalId{} &&
         no_forbidden_global_structure_materialized &&
         !hierarchy_reduction_performed &&
         !common_durable_checkpoint_certified &&
         !hierarchy_or_forest_certified && !public_status_claimed;
}

bool ExactDirectSupportTerminalFacade::terminal_catalog_certified() const {
  const bool sparse_pair_digests_match =
      certificate.pair_source_kind !=
          ExactDirectSupportPairSourceKind::
              sealed_sparse_anchored_session ||
      (certificate.pair_terminal_output_digest ==
           sparse_pair_terminal_output_digest(*this) &&
       certificate.pair_semantic_digest == sparse_pair_semantic_digest(
           certificate.pair_canonical_cloud_digest,
           certificate.pair_lbvh_digest,
           certificate.requirements,
           certificate.pair_terminal_output_digest));
  const bool transactional_pair_digests_match =
      certificate.pair_source_kind !=
          ExactDirectSupportPairSourceKind::
              sealed_transactional_pair_terminal_authority ||
      (certificate.pair_terminal_output_digest ==
           transactional_pair_terminal_output_digest(*this) &&
       certificate.pair_semantic_digest ==
           transactional_pair_semantic_digest(
               certificate.pair_canonical_cloud_digest,
               certificate.pair_lbvh_digest,
               certificate.requirements,
               certificate.pair_transactional_audit,
               certificate.pair_terminal_output_digest));
  return certificate.terminal_catalog_certified() &&
         sparse_pair_digests_match &&
         transactional_pair_digests_match &&
         certificate.normalized_terminal_output_digest ==
             normalized_terminal_output_digest(*this) &&
         normalized_terminal_records_valid(*this) &&
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
  certificate.pair_source_kind =
      ExactDirectSupportPairSourceKind::fresh_resident_replay;
  certificate.pair_legacy_budget_applicable = true;
  certificate.pair_result_freshly_replayed =
      pair_verification.result_certified;
  certificate.higher_result_freshly_replayed =
      higher_verification.result_certified;
  certificate.higher_source_kind =
      ExactDirectSupportHigherSourceKind::fresh_resident_replay;
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

ExactDirectSupportTerminalFacade build_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactDirectSupportTerminalBudget& budget,
    const ExactPairSupportStreamResult& pair_result,
    ExactHigherSupportTerminalAuthority higher_authority) {
  const ExactPairSupportCheckpointManifest pair_manifest =
      make_exact_pair_support_checkpoint_manifest(
          index, cloud, requested_maximum_order);
  const ExactHigherSupportCheckpointManifest higher_manifest =
      higher_authority.manifest();
  const ExactPairSupportStreamVerification pair_verification =
      verify_exact_pair_support_stream(
          index,
          cloud,
          requested_maximum_order,
          budget.pair,
          pair_result);
  const bool higher_authority_certified =
      higher_authority.sealed_in_process_terminal_authority() &&
      higher_authority.bound_to(
          index, cloud, requested_maximum_order) &&
      higher_authority.chunk_budget() == budget.higher &&
      !higher_authority.fresh_replay_performed() &&
      !higher_authority.durable_authority_claimed() &&
      !higher_authority.hierarchy_reduction_performed() &&
      !higher_authority.public_status_claimed();

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
      higher_authority.bound_to(
          index, cloud, requested_maximum_order) &&
      source_manifest_contracts_match(pair_manifest, higher_manifest);
  certificate.source_requirements_match = source_requirements_match(
      pair_result.requirements, higher_manifest, pair_manifest);
  certificate.pair_source_kind =
      ExactDirectSupportPairSourceKind::fresh_resident_replay;
  certificate.pair_legacy_budget_applicable = true;
  certificate.pair_result_freshly_replayed =
      pair_verification.result_certified;
  certificate.higher_result_freshly_replayed = false;
  certificate.higher_source_kind =
      ExactDirectSupportHigherSourceKind::
          sealed_anchored_fixed_chunk_run;
  certificate.higher_maximum_chunk_count =
      higher_authority.maximum_chunk_count();
  certificate.higher_anchored_chunk_count =
      higher_authority.chunk_count();
  certificate.higher_root_anchored_run_certified =
      higher_authority.canonical_root_anchored_internal_production() &&
      higher_authority.terminal_checkpoint_local_integrity_verified();
  certificate.higher_terminal_authority_consumed = true;
  certificate.higher_terminal_records_captured_once =
      higher_authority
          .committed_chunks_captured_once_without_fresh_replay();
  certificate.higher_output_chain_digest =
      higher_authority.output_chain_digest();
  certificate.higher_terminal_checkpoint_digest =
      higher_authority.checkpoint_digest();
  certificate.pair_stream_terminal =
      certificate.pair_result_freshly_replayed &&
      pair_result.stream_complete() &&
      pair_result.absence_of_additional_pair_supports_certified();
  certificate.higher_stream_terminal = higher_authority_certified;
  certificate.no_forbidden_global_structure_materialized =
      pair_result.no_forbidden_global_structure_materialized &&
      higher_authority.no_forbidden_global_structure_materialized();
  certificate.hierarchy_reduction_performed =
      pair_result.hierarchy_reduction_performed ||
      higher_authority.hierarchy_reduction_performed();
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
  const ExactHigherSupportStreamAudit& higher_audit =
      higher_authority.terminal_audit();
  const bool higher_universe_certified =
      higher_authority_certified &&
      higher_audit.exact_bigint_universe_certified &&
      higher_audit.total_support_count ==
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
      !higher_authority_certified ||
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
  const std::size_t higher_event_count =
      higher_authority.event_count();
  const std::size_t higher_diagnostic_count =
      higher_authority.relevant_extra_shell_diagnostic_count();
  std::vector<ExactHigherSupportTerminalSegment> higher_segments =
      std::move(higher_authority).release_segments();
  normalize_terminal_records(
      pair_result,
      std::move(higher_segments),
      higher_event_count,
      higher_diagnostic_count,
      facade);
  certificate.decision =
      facade.relevant_extra_shell_diagnostics.empty()
          ? ExactDirectSupportTerminalDecision::
                complete_direct_support_catalog
          : ExactDirectSupportTerminalDecision::
                complete_direct_support_catalog_with_relevant_extra_shell_diagnostics;
  if (!facade.terminal_catalog_certified()) {
    throw std::logic_error(
        "the sealed direct support terminal certificate is inconsistent");
  }
  return facade;
}

ExactDirectSupportTerminalFacade build_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& higher_budget,
    ExactSparseAnchoredPairTerminalAuthority pair_authority,
    ExactHigherSupportTerminalAuthority higher_authority) {
  const ExactPairSupportCheckpointManifest pair_manifest =
      make_exact_pair_support_checkpoint_manifest(
          index, cloud, requested_maximum_order);
  const ExactHigherSupportCheckpointManifest higher_manifest =
      higher_authority.manifest();
  const ExactMortonGroupedAnchoredPairScheduleConfig pair_schedule_config =
      pair_authority.schedule_config();
  const ExactSparseAnchoredPairSessionTotalCapacity pair_total_capacity =
      pair_authority.total_capacity();
  const std::size_t sparse_pair_required_closed_rank =
      std::max<std::size_t>(
          2U, pair_manifest.maximum_relevant_closed_rank);
  const bool pair_authority_bound = pair_authority.bound_to(
      index,
      cloud,
      sparse_pair_required_closed_rank,
      pair_schedule_config,
      pair_total_capacity);
  const bool pair_authority_certified =
      pair_authority.sealed_in_process_terminal_authority() &&
      pair_authority_bound;
  const bool higher_authority_bound = higher_authority.bound_to(
      index, cloud, requested_maximum_order);
  const bool higher_authority_certified =
      higher_authority.sealed_in_process_terminal_authority() &&
      higher_authority_bound &&
      higher_authority.chunk_budget() == higher_budget &&
      !higher_authority.fresh_replay_performed() &&
      !higher_authority.durable_authority_claimed() &&
      !higher_authority.hierarchy_reduction_performed() &&
      !higher_authority.public_status_claimed();

  ExactDirectSupportTerminalFacade facade;
  ExactDirectSupportTerminalCertificate& certificate = facade.certificate;
  certificate.requested_budget.higher = higher_budget;
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
  certificate.higher_semantic_digest = higher_manifest.semantic_digest;
  certificate.source_authorities_match =
      pair_authority_bound && higher_authority_bound &&
      source_manifest_contracts_match(pair_manifest, higher_manifest);

  const ExactSparseAnchoredPairSessionAudit& pair_audit =
      pair_authority.audit();
  const ExactMortonGroupedAnchoredPairCandidateAudit& pair_candidate_audit =
      pair_authority.candidate_audit();
  certificate.source_requirements_match =
      pair_audit.point_count == pair_manifest.point_count &&
      pair_authority.maximum_closed_rank() ==
          sparse_pair_required_closed_rank &&
      source_manifest_contracts_match(pair_manifest, higher_manifest);
  certificate.pair_source_kind =
      ExactDirectSupportPairSourceKind::sealed_sparse_anchored_session;
  certificate.pair_legacy_budget_applicable = false;
  certificate.pair_result_freshly_replayed = false;
  certificate.pair_sparse_maximum_closed_rank =
      pair_authority.maximum_closed_rank();
  certificate.pair_sparse_schedule_config = pair_schedule_config;
  certificate.pair_sparse_total_capacity = pair_total_capacity;
  certificate.pair_directed_pair_universe_size =
      pair_audit.directed_pair_universe_size;
  certificate.pair_authenticated_pruned_directed_pair_count =
      pair_audit.authenticated_pruned_directed_pair_count;
  certificate.pair_orientation_check_count =
      pair_candidate_audit.orientation_check_count;
  certificate.pair_reverse_or_self_orientation_skip_count =
      pair_candidate_audit.reverse_or_self_orientation_skip_count;
  certificate.pair_admitted_candidate_count =
      pair_audit.admitted_candidate_count;
  certificate.pair_classification_terminal_count =
      pair_audit.classification_terminal_count;
  certificate.pair_above_rank_count = pair_audit.above_rank_count;
  certificate.pair_terminal_record_count = pair_audit.emitted_record_count;
  certificate.pair_sparse_directed_coverage_certified =
      pair_audit.directed_coverage_certified;
  certificate.pair_sparse_orientation_partition_certified =
      pair_audit.orientation_partition_certified;
  certificate.pair_sparse_classification_partition_certified =
      pair_audit.candidate_classification_partition_certified;
  certificate.pair_sparse_output_partition_certified =
      pair_audit.output_partition_certified &&
      pair_audit.retained_records_certified;
  certificate.pair_stream_terminal = pair_authority_certified;

  certificate.higher_result_freshly_replayed = false;
  certificate.higher_source_kind =
      ExactDirectSupportHigherSourceKind::
          sealed_anchored_fixed_chunk_run;
  certificate.higher_maximum_chunk_count =
      higher_authority.maximum_chunk_count();
  certificate.higher_anchored_chunk_count =
      higher_authority.chunk_count();
  certificate.higher_root_anchored_run_certified =
      higher_authority.canonical_root_anchored_internal_production() &&
      higher_authority.terminal_checkpoint_local_integrity_verified();
  certificate.higher_terminal_authority_consumed = true;
  certificate.higher_terminal_records_captured_once =
      higher_authority
          .committed_chunks_captured_once_without_fresh_replay();
  certificate.higher_output_chain_digest =
      higher_authority.output_chain_digest();
  certificate.higher_terminal_checkpoint_digest =
      higher_authority.checkpoint_digest();
  certificate.higher_stream_terminal = higher_authority_certified;
  certificate.no_forbidden_global_structure_materialized =
      pair_audit.no_forbidden_global_structure_materialized &&
      pair_candidate_audit.no_dynamic_candidate_or_output_arena_materialized &&
      pair_authority.schedule_audit()
          .no_global_anchor_pair_or_output_arena_materialized &&
      higher_authority.no_forbidden_global_structure_materialized();
  certificate.hierarchy_reduction_performed =
      higher_authority.hierarchy_reduction_performed();
  certificate.scope = ExactDirectSupportTerminalScope::
      direct_support_catalog_arities_two_through_four_only;

  for (std::size_t index_value = 0U; index_value < 3U; ++index_value) {
    ExactDirectSupportArityTerminalCertificate& arity =
        certificate.arity_certificates[index_value];
    arity.support_size = static_cast<std::uint8_t>(index_value + 2U);
    arity.exact_candidate_universe_size = exact_binomial(
        cloud.size(), static_cast<std::size_t>(arity.support_size));
    certificate.exact_candidate_universe_size +=
        arity.exact_candidate_universe_size;
  }

  const exact::BigInt point_count{cloud.size()};
  const bool pair_universe_certified =
      pair_authority_certified &&
      exact::BigInt{pair_audit.directed_pair_universe_size} ==
          point_count * point_count;
  const ExactHigherSupportStreamAudit& higher_audit =
      higher_authority.terminal_audit();
  const bool higher_universe_certified =
      higher_authority_certified &&
      higher_audit.exact_bigint_universe_certified &&
      higher_audit.total_support_count ==
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

  if (!pair_authority_certified || !higher_authority_certified ||
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
  const std::size_t pair_event_count = pair_audit.accepted_event_count;
  const std::size_t pair_diagnostic_count =
      pair_audit.relevant_extra_shell_diagnostic_count;
  const std::size_t higher_event_count = higher_authority.event_count();
  const std::size_t higher_diagnostic_count =
      higher_authority.relevant_extra_shell_diagnostic_count();
  std::vector<ExactSparseAnchoredPairRecord> pair_records =
      std::move(pair_authority).release_records();
  certificate.pair_terminal_authority_consumed = true;
  certificate.pair_terminal_records_captured_once = true;
  std::vector<ExactHigherSupportTerminalSegment> higher_segments =
      std::move(higher_authority).release_segments();
  normalize_terminal_records(
      std::move(pair_records),
      pair_event_count,
      pair_diagnostic_count,
      std::move(higher_segments),
      higher_event_count,
      higher_diagnostic_count,
      facade);
  certificate.pair_terminal_output_digest =
      sparse_pair_terminal_output_digest(facade);
  certificate.pair_semantic_digest = sparse_pair_semantic_digest(
      certificate.pair_canonical_cloud_digest,
      certificate.pair_lbvh_digest,
      certificate.requirements,
      certificate.pair_terminal_output_digest);
  certificate.decision =
      facade.relevant_extra_shell_diagnostics.empty()
          ? ExactDirectSupportTerminalDecision::
                complete_direct_support_catalog
          : ExactDirectSupportTerminalDecision::
                complete_direct_support_catalog_with_relevant_extra_shell_diagnostics;
  if (!facade.terminal_catalog_certified()) {
    throw std::logic_error(
        "the sparse authority-backed direct support certificate is inconsistent");
  }
  return facade;
}

ExactDirectSupportTerminalFacade build_exact_direct_support_terminal_facade(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& higher_budget,
    ExactDirectPairTerminalAuthority pair_authority,
    ExactHigherSupportTerminalAuthority higher_authority) {
  const ExactPairSupportCheckpointManifest pair_manifest =
      make_exact_pair_support_checkpoint_manifest(
          index, cloud, requested_maximum_order);
  const ExactHigherSupportCheckpointManifest higher_manifest =
      higher_authority.manifest();
  const ExactDirectPairTerminalAudit pair_audit = pair_authority.audit();
  const bool pair_authority_bound = pair_authority.bound_to(
      index, cloud, requested_maximum_order);
  const bool pair_authority_certified =
      pair_authority.sealed_in_process_terminal_authority() &&
      pair_authority_bound;
  const bool higher_authority_bound = higher_authority.bound_to(
      index, cloud, requested_maximum_order);
  const bool higher_authority_certified =
      higher_authority.sealed_in_process_terminal_authority() &&
      higher_authority_bound &&
      higher_authority.chunk_budget() == higher_budget &&
      !higher_authority.fresh_replay_performed() &&
      !higher_authority.durable_authority_claimed() &&
      !higher_authority.hierarchy_reduction_performed() &&
      !higher_authority.public_status_claimed();

  ExactDirectSupportTerminalFacade facade;
  ExactDirectSupportTerminalCertificate& certificate = facade.certificate;
  certificate.requested_budget.higher = higher_budget;
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
  certificate.higher_semantic_digest = higher_manifest.semantic_digest;
  certificate.source_authorities_match =
      pair_authority_bound && higher_authority_bound &&
      source_manifest_contracts_match(pair_manifest, higher_manifest);
  certificate.source_requirements_match =
      pair_audit.point_count == pair_manifest.point_count &&
      pair_audit.requested_maximum_order ==
          pair_manifest.requested_maximum_order &&
      pair_audit.effective_maximum_order ==
          pair_manifest.effective_maximum_order &&
      pair_audit.maximum_closed_rank ==
          pair_manifest.maximum_relevant_closed_rank &&
      source_manifest_contracts_match(pair_manifest, higher_manifest);
  certificate.pair_source_kind =
      ExactDirectSupportPairSourceKind::
          sealed_transactional_pair_terminal_authority;
  certificate.pair_transactional_audit = pair_audit;
  certificate.pair_legacy_budget_applicable = false;
  certificate.pair_result_freshly_replayed = false;
  certificate.pair_stream_terminal = pair_authority_certified;

  certificate.higher_result_freshly_replayed = false;
  certificate.higher_source_kind =
      ExactDirectSupportHigherSourceKind::
          sealed_anchored_fixed_chunk_run;
  certificate.higher_maximum_chunk_count =
      higher_authority.maximum_chunk_count();
  certificate.higher_anchored_chunk_count =
      higher_authority.chunk_count();
  certificate.higher_root_anchored_run_certified =
      higher_authority.canonical_root_anchored_internal_production() &&
      higher_authority.terminal_checkpoint_local_integrity_verified();
  certificate.higher_terminal_authority_consumed = true;
  certificate.higher_terminal_records_captured_once =
      higher_authority
          .committed_chunks_captured_once_without_fresh_replay();
  certificate.higher_output_chain_digest =
      higher_authority.output_chain_digest();
  certificate.higher_terminal_checkpoint_digest =
      higher_authority.checkpoint_digest();
  certificate.higher_stream_terminal = higher_authority_certified;
  certificate.no_forbidden_global_structure_materialized =
      pair_audit.no_forbidden_global_structure_materialized &&
      higher_authority.no_forbidden_global_structure_materialized();
  certificate.hierarchy_reduction_performed =
      pair_audit.hierarchy_reduction_performed ||
      higher_authority.hierarchy_reduction_performed();
  certificate.scope = ExactDirectSupportTerminalScope::
      direct_support_catalog_arities_two_through_four_only;

  for (std::size_t index_value = 0U; index_value < 3U; ++index_value) {
    ExactDirectSupportArityTerminalCertificate& arity =
        certificate.arity_certificates[index_value];
    arity.support_size = static_cast<std::uint8_t>(index_value + 2U);
    arity.exact_candidate_universe_size = exact_binomial(
        cloud.size(), static_cast<std::size_t>(arity.support_size));
    certificate.exact_candidate_universe_size +=
        arity.exact_candidate_universe_size;
  }

  const bool pair_universe_certified =
      pair_authority_certified &&
      exact::BigInt{pair_audit.unordered_pair_universe_size} ==
          certificate.arity_certificates[0].exact_candidate_universe_size;
  const ExactHigherSupportStreamAudit& higher_audit =
      higher_authority.terminal_audit();
  const bool higher_universe_certified =
      higher_authority_certified &&
      higher_audit.exact_bigint_universe_certified &&
      higher_audit.total_support_count ==
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

  if (!pair_authority_certified || !higher_authority_certified ||
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
  const std::size_t pair_event_count = pair_audit.accepted_event_count;
  const std::size_t pair_diagnostic_count =
      pair_audit.relevant_extra_shell_diagnostic_count;
  const std::size_t higher_event_count = higher_authority.event_count();
  const std::size_t higher_diagnostic_count =
      higher_authority.relevant_extra_shell_diagnostic_count();
  std::vector<ExactDirectPairTerminalRecord> pair_records =
      std::move(pair_authority).release_records();
  certificate.pair_terminal_authority_consumed = true;
  certificate.pair_terminal_records_captured_once = true;
  std::vector<ExactHigherSupportTerminalSegment> higher_segments =
      std::move(higher_authority).release_segments();
  normalize_terminal_records(
      std::move(pair_records),
      pair_event_count,
      pair_diagnostic_count,
      std::move(higher_segments),
      higher_event_count,
      higher_diagnostic_count,
      facade);
  certificate.pair_terminal_output_digest =
      transactional_pair_terminal_output_digest(facade);
  certificate.pair_semantic_digest = transactional_pair_semantic_digest(
      certificate.pair_canonical_cloud_digest,
      certificate.pair_lbvh_digest,
      certificate.requirements,
      certificate.pair_transactional_audit,
      certificate.pair_terminal_output_digest);
  certificate.decision =
      facade.relevant_extra_shell_diagnostics.empty()
          ? ExactDirectSupportTerminalDecision::
                complete_direct_support_catalog
          : ExactDirectSupportTerminalDecision::
                complete_direct_support_catalog_with_relevant_extra_shell_diagnostics;
  if (!facade.terminal_catalog_certified()) {
    throw std::logic_error(
        "the transactional authority-backed direct support certificate is inconsistent");
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
