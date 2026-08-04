#include "morsehgp3d/hierarchy/higher_support_stream.hpp"

#include "morsehgp3d/exact/support.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct IntegrityVerifiedHigherCheckpointTag {};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] bool can_add_within(
    std::size_t current,
    std::size_t addition,
    std::size_t limit) noexcept {
  return current <= limit && addition <= limit - current;
}

void increment(std::size_t& value, const char* message) {
  value = checked_add(value, 1U, message);
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if constexpr (sizeof(std::size_t) > sizeof(std::uint64_t)) {
    if (value > std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(message);
    }
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    const char* message) {
  if constexpr (sizeof(std::size_t) < sizeof(std::uint64_t)) {
    if (value > std::numeric_limits<std::size_t>::max()) {
      throw std::overflow_error(message);
    }
  }
  return static_cast<std::size_t>(value);
}

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

template <class Record>
[[nodiscard]] bool support_record_less(
    const Record& left,
    const Record& right) {
  if (left.support_size != right.support_size) {
    return left.support_size < right.support_size;
  }
  return std::lexicographical_compare(
      left.support_ids.begin(),
      left.support_ids.begin() + left.support_size,
      right.support_ids.begin(),
      right.support_ids.begin() + right.support_size);
}

class DigestWriter {
 public:
  explicit DigestWriter(std::string_view domain) {
    text(domain);
  }

  void byte(std::uint8_t value) {
    builder_.update(std::span<const std::uint8_t>{&value, 1U});
  }

  void boolean(bool value) {
    byte(static_cast<std::uint8_t>(value ? 1U : 0U));
  }

  void u32(std::uint32_t value) {
    std::array<std::uint8_t, 4U> bytes{};
    for (std::size_t index = 0U; index < bytes.size(); ++index) {
      const std::size_t shift = (bytes.size() - 1U - index) * 8U;
      bytes[index] = static_cast<std::uint8_t>(value >> shift);
    }
    builder_.update(bytes);
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
    u64(checked_u64(value, "a higher-support digest size does not fit uint64"));
  }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void identifier(const contract::CanonicalId& identifier) {
    builder_.update(identifier.bytes());
  }

  void bigint(const exact::BigInt& value) {
    text(value.str());
  }

  void rational(const exact::ExactRational& value) {
    text(value.canonical_key());
  }

  void center(const exact::ExactCenter3& value) {
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      rational(value.coordinate(axis));
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

void append_node_group(
    DigestWriter& writer,
    const ExactHigherSupportNodeGroup& group) {
  writer.u64(group.node_index);
  writer.u64(group.leaf_begin);
  writer.u64(group.leaf_end);
  writer.byte(group.multiplicity);
}

void append_frontier_entry(
    DigestWriter& writer,
    const ExactHigherSupportFrontierEntry& entry) {
  writer.byte(entry.support_size);
  writer.byte(entry.group_count);
  for (const ExactHigherSupportNodeGroup& group : entry.groups) {
    append_node_group(writer, group);
  }
}

void append_node_receipt(
    DigestWriter& writer,
    const ExactHigherSupportNodeReceipt& receipt) {
  writer.u64(receipt.node_index);
  writer.u64(receipt.leaf_begin);
  writer.u64(receipt.leaf_end);
}

void append_rank_receipt(
    DigestWriter& writer,
    const ExactHigherSupportRankReceipt& receipt) {
  append_node_receipt(writer, receipt.query_node);
  writer.size(receipt.certified_point_count);
}

void append_audit(
    DigestWriter& writer,
    const ExactHigherSupportStreamAudit& audit) {
#define MORSEHGP3D_APPEND_HIGHER_BIGINT(field) writer.bigint(audit.field)
  MORSEHGP3D_APPEND_HIGHER_BIGINT(total_support_count);
  MORSEHGP3D_APPEND_HIGHER_BIGINT(well_centering_pruned_support_count);
  MORSEHGP3D_APPEND_HIGHER_BIGINT(rank_pruned_support_count);
  MORSEHGP3D_APPEND_HIGHER_BIGINT(leaf_classified_support_count);
  MORSEHGP3D_APPEND_HIGHER_BIGINT(resolved_support_count);
  MORSEHGP3D_APPEND_HIGHER_BIGINT(remaining_frontier_support_count);
#undef MORSEHGP3D_APPEND_HIGHER_BIGINT
#define MORSEHGP3D_APPEND_HIGHER_SIZE(field) writer.size(audit.field)
  MORSEHGP3D_APPEND_HIGHER_SIZE(work_unit_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(support_product_visit_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(support_product_expansion_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(generated_child_product_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(exact_product_analysis_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_search_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_witness_node_visit_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_local_probe_attempt_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_local_probe_center_seed_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_local_probe_center_path_node_visit_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_local_probe_candidate_evaluation_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_local_probe_pruned_product_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_local_probe_fail_open_product_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_local_probe_geometric_gate_skip_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_query_outside_or_boundary_node_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(rank_query_outside_or_boundary_point_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(emitted_prune_certificate_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(emitted_rank_receipt_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(leaf_support_analysis_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(affinely_dependent_leaf_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(boundary_reduced_leaf_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(exterior_circumcenter_leaf_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(minimal_leaf_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(above_rank_leaf_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(global_closed_ball_query_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(point_classification_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(closed_ball_node_visit_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(closed_ball_bulk_interior_subtree_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(closed_ball_bulk_exterior_subtree_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(exact_point_distance_evaluation_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(accepted_event_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(relevant_extra_shell_diagnostic_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(emitted_record_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(emitted_point_id_reference_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(maximum_frontier_entry_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(maximum_rank_frontier_entry_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(maximum_rank_local_probe_candidate_count);
  MORSEHGP3D_APPEND_HIGHER_SIZE(maximum_closed_ball_frontier_entry_count);
#undef MORSEHGP3D_APPEND_HIGHER_SIZE
  writer.boolean(audit.exact_bigint_universe_certified);
  writer.boolean(audit.grouped_partition_accounting_certified);
}

[[nodiscard]] contract::CanonicalId initial_output_chain_digest() {
  DigestWriter writer{
      "MorseHGP3D/phase9/higher-support/output-record-chain/v2/empty"};
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId extend_output_chain(
    const contract::CanonicalId& previous,
    const ExactHigherSupportEvent& event) {
  DigestWriter writer{
      "MorseHGP3D/phase9/higher-support/output-record-chain/v2/event"};
  writer.identifier(previous);
  writer.byte(event.support_size);
  for (const PointId point_id : event.support_ids) {
    writer.u64(point_id);
  }
  writer.center(event.center);
  writer.level(event.squared_level);
  writer.size(event.interior_ids.size());
  for (const PointId point_id : event.interior_ids) {
    writer.u64(point_id);
  }
  writer.size(event.closed_rank);
  writer.size(event.exterior_count);
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId extend_output_chain(
    const contract::CanonicalId& previous,
    const ExactHigherSupportExtraShellDiagnostic& diagnostic) {
  DigestWriter writer{
      "MorseHGP3D/phase9/higher-support/output-record-chain/v2/extra-shell"};
  writer.identifier(previous);
  writer.byte(diagnostic.support_size);
  for (const PointId point_id : diagnostic.support_ids) {
    writer.u64(point_id);
  }
  writer.center(diagnostic.center);
  writer.level(diagnostic.squared_level);
  writer.size(diagnostic.interior_ids.size());
  for (const PointId point_id : diagnostic.interior_ids) {
    writer.u64(point_id);
  }
  writer.size(diagnostic.shell_count);
  writer.u64(diagnostic.canonical_extra_shell_witness_id);
  writer.size(diagnostic.minimum_possible_closed_rank);
  writer.size(diagnostic.observed_closed_rank);
  writer.size(diagnostic.exterior_count);
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId extend_output_chain(
    const contract::CanonicalId& previous,
    const ExactHigherSupportPruneCertificate& certificate) {
  DigestWriter writer{
      "MorseHGP3D/phase9/higher-support/output-record-chain/v2/prune"};
  writer.identifier(previous);
  append_frontier_entry(writer, certificate.product);
  writer.byte(static_cast<std::uint8_t>(certificate.reason));
  writer.bigint(certificate.pruned_support_count);
  writer.size(certificate.required_strict_interior_point_count);
  writer.bigint(certificate.certified_strict_interior_point_count);
  writer.size(certificate.rank_receipts.size());
  for (const ExactHigherSupportRankReceipt& receipt :
       certificate.rank_receipts) {
    append_rank_receipt(writer, receipt);
  }
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId checkpoint_digest(
    const ExactHigherSupportCheckpoint& checkpoint) {
  DigestWriter writer{
      "MorseHGP3D/phase9/higher-support/checkpoint/v6"};
  const ExactHigherSupportCheckpointManifest& manifest = checkpoint.manifest;
  writer.u32(manifest.schema_version);
  writer.u32(manifest.traversal_version);
  writer.size(manifest.point_count);
  writer.size(manifest.lbvh_node_count);
  writer.size(manifest.lbvh_leaf_count);
  writer.size(manifest.requested_maximum_order);
  writer.size(manifest.effective_maximum_order);
  writer.size(manifest.maximum_relevant_closed_rank);
  writer.identifier(manifest.canonical_cloud_digest);
  writer.identifier(manifest.lbvh_digest);
  writer.identifier(manifest.semantic_digest);
  writer.u64(checkpoint.next_chunk_sequence);
  writer.size(checkpoint.output_record_count);
  writer.identifier(checkpoint.output_chain_digest);
  writer.size(checkpoint.frontier.size());
  for (const ExactHigherSupportFrontierEntry& entry : checkpoint.frontier) {
    append_frontier_entry(writer, entry);
  }
  writer.boolean(checkpoint.pending_product.has_value());
  if (checkpoint.pending_product.has_value()) {
    const ExactHigherSupportPendingProduct& pending =
        *checkpoint.pending_product;
    append_frontier_entry(writer, pending.product);
    writer.byte(static_cast<std::uint8_t>(pending.stage));
    writer.boolean(pending.rank_search_started);
    writer.boolean(pending.leaf_analysis_started);
    writer.size(pending.rank_frontier.size());
    for (const ExactHigherSupportNodeReceipt& receipt :
         pending.rank_frontier) {
      append_node_receipt(writer, receipt);
    }
    writer.size(pending.rank_probe_next_candidate_index);
    writer.size(pending.rank_probe_next_fallback_leaf_index);
    writer.boolean(pending.rank_probe_fallback_active);
    writer.boolean(
        pending.rank_probe_center_seed_leaf_position.has_value());
    if (pending.rank_probe_center_seed_leaf_position.has_value()) {
      writer.u64(*pending.rank_probe_center_seed_leaf_position);
    }
    writer.size(pending.strict_interior_receipts.size());
    for (const ExactHigherSupportNodeReceipt& receipt :
         pending.strict_interior_receipts) {
      append_node_receipt(writer, receipt);
    }
    writer.bigint(pending.certified_strict_interior_point_count);
  }
  append_audit(writer, checkpoint.cumulative_audit);
  return writer.finalize();
}

}  // namespace

exact::BigInt exact_higher_support_candidate_universe_size(
    std::size_t point_count) {
  return exact_binomial(point_count, 3U) +
         exact_binomial(point_count, 4U);
}

class ExactHigherSupportStreamBuilder {
 public:
  ExactHigherSupportStreamBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactHigherSupportStreamBudget budget,
      const ExactHigherSupportPositiveDecisionSource*
          positive_decision_source = nullptr)
      : index_(index),
        cloud_(cloud),
        positive_decision_source_(positive_decision_source),
        terminal_geometry_decision_source_(
            positive_decision_source != nullptr
                ? &positive_decision_source->terminal_geometry_source()
                : nullptr),
        output_chain_digest_(initial_output_chain_digest()),
        canonical_sort_records_(true) {
    validate_inputs(requested_maximum_order);
    validate_positive_decision_source();
    result_.budget = budget;
    result_.requirements.point_count = cloud_.size();
    result_.requirements.requested_maximum_order =
        requested_maximum_order;
    result_.requirements.effective_maximum_order =
        std::min(requested_maximum_order, cloud_.size());
    result_.requirements.maximum_relevant_closed_rank = std::min(
        checked_add(
            result_.requirements.effective_maximum_order,
            1U,
            "the higher-support maximum rank overflows size_t"),
        cloud_.size());
    result_.audit.total_support_count =
        exact_higher_support_candidate_universe_size(cloud_.size());
    result_.audit.exact_bigint_universe_certified = true;

    if (cloud_.size() >= 3U) {
      frontier_.push_back(make_root_entry(3U));
    }
    if (cloud_.size() >= 4U) {
      frontier_.push_back(make_root_entry(4U));
    }
    if (frontier_.size() >
        result_.budget.maximum_frontier_entry_count) {
      throw std::invalid_argument(
          "the higher-support initial frontier exceeds its budget");
    }
    result_.audit.maximum_frontier_entry_count = frontier_.size();
    chunk_audit_origin_ = result_.audit;
  }

  ExactHigherSupportStreamBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactHigherSupportStreamBudget budget,
      const ExactHigherSupportCheckpoint& checkpoint,
      IntegrityVerifiedHigherCheckpointTag,
      const ExactHigherSupportPositiveDecisionSource*
          positive_decision_source = nullptr)
      : index_(index),
        cloud_(cloud),
        positive_decision_source_(positive_decision_source),
        terminal_geometry_decision_source_(
            positive_decision_source != nullptr
                ? &positive_decision_source->terminal_geometry_source()
                : nullptr),
        output_chain_digest_(checkpoint.output_chain_digest),
        output_record_count_(checkpoint.output_record_count),
        canonical_sort_records_(false) {
    validate_inputs(requested_maximum_order);
    validate_positive_decision_source();
    result_.budget = budget;
    result_.requirements.point_count = cloud_.size();
    result_.requirements.requested_maximum_order =
        requested_maximum_order;
    result_.requirements.effective_maximum_order =
        std::min(requested_maximum_order, cloud_.size());
    result_.requirements.maximum_relevant_closed_rank = std::min(
        checked_add(
            result_.requirements.effective_maximum_order,
            1U,
            "the higher-support maximum rank overflows size_t"),
        cloud_.size());
    result_.audit = checkpoint.cumulative_audit;
    frontier_ = checkpoint.frontier;
    pending_product_ = checkpoint.pending_product;
    if (frontier_.size() > budget.maximum_frontier_entry_count) {
      throw std::invalid_argument(
          "the chunk frontier capacity is below the higher-support checkpoint frontier");
    }
    if (pending_product_.has_value() &&
        pending_product_->rank_frontier.size() >
            budget.maximum_auxiliary_frontier_entry_count) {
      throw std::invalid_argument(
          "the chunk auxiliary capacity is below the persisted higher-support rank cursor");
    }
    chunk_audit_origin_ = checkpoint.cumulative_audit;
    result_.audit.remaining_frontier_support_count = 0;
    result_.audit.grouped_partition_accounting_certified = false;
  }

  void execute() {
    while ((!frontier_.empty() || pending_product_.has_value()) && !stopped_) {
      if (pending_product_.has_value()) {
        continue_pending_product();
      } else {
        visit_frontier_back();
      }
    }
    finish_result();
  }

  [[nodiscard]] ExactHigherSupportStreamResult take_result() {
    return std::move(result_);
  }

  [[nodiscard]] const contract::CanonicalId& output_chain_digest()
      const noexcept {
    return output_chain_digest_;
  }

  [[nodiscard]] std::vector<ExactHigherSupportStreamChunk::RecordKind>
  take_record_order() {
    return std::move(record_order_);
  }

  [[nodiscard]] ExactHigherSupportCheckpoint snapshot_checkpoint(
      const ExactHigherSupportCheckpointManifest& manifest,
      std::uint64_t next_chunk_sequence) const {
    ExactHigherSupportCheckpoint checkpoint;
    checkpoint.manifest = manifest;
    checkpoint.next_chunk_sequence = next_chunk_sequence;
    checkpoint.output_record_count = output_record_count_;
    checkpoint.output_chain_digest = output_chain_digest_;
    checkpoint.frontier = frontier_;
    checkpoint.pending_product = pending_product_;
    checkpoint.cumulative_audit = audit_snapshot();
    checkpoint.checkpoint_digest = checkpoint_digest(checkpoint);
    return checkpoint;
  }

  [[nodiscard]] ExactHigherSupportCheckpoint take_checkpoint(
      const ExactHigherSupportCheckpointManifest& manifest,
      std::uint64_t next_chunk_sequence) {
    ExactHigherSupportCheckpoint checkpoint;
    checkpoint.manifest = manifest;
    checkpoint.next_chunk_sequence = next_chunk_sequence;
    checkpoint.output_record_count = output_record_count_;
    checkpoint.output_chain_digest = output_chain_digest_;
    checkpoint.cumulative_audit = audit_snapshot();
    checkpoint.frontier = std::move(frontier_);
    checkpoint.pending_product = std::move(pending_product_);
    checkpoint.checkpoint_digest = checkpoint_digest(checkpoint);
    return checkpoint;
  }

  [[nodiscard]] static ExactHigherSupportCheckpointManifest manifest_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactHigherSupportAuthorityContextAudit* audit = nullptr) {
    if (!index.validated_for(cloud) || cloud.size() == 0U ||
        requested_maximum_order == 0U ||
        requested_maximum_order > higher_support_maximum_requested_order) {
      throw std::invalid_argument(
          "a higher-support manifest requires a valid nonempty cloud, LBVH and 1<=Kmax<=10");
    }
    if (audit != nullptr) {
      increment(
          audit->manifest_build_count,
          "the higher-support manifest build count overflows size_t");
      audit->manifest_cached = false;
    }
    ExactHigherSupportCheckpointManifest manifest;
    manifest.point_count = cloud.size();
    manifest.lbvh_node_count = index.nodes_.size();
    manifest.lbvh_leaf_count = index.leaves_.size();
    manifest.requested_maximum_order = requested_maximum_order;
    manifest.effective_maximum_order =
        std::min(requested_maximum_order, cloud.size());
    manifest.maximum_relevant_closed_rank = std::min(
        checked_add(
            manifest.effective_maximum_order,
            1U,
            "the higher-support manifest rank overflows size_t"),
        cloud.size());

    DigestWriter cloud_writer{
        "MorseHGP3D/phase9/higher-support/canonical-cloud/v1"};
    cloud_writer.size(cloud.size());
    for (std::size_t point_index = 0U;
         point_index < cloud.size();
         ++point_index) {
      const PointId point_id = checked_u64(
          point_index,
          "a higher-support canonical point index does not fit PointId");
      for (const std::uint64_t word :
           cloud.point(point_id).canonical_input_bits()) {
        cloud_writer.u64(word);
      }
      if (audit != nullptr) {
        increment(
            audit->canonical_cloud_point_hash_count,
            "the higher-support manifest point hash count overflows size_t");
      }
    }
    manifest.canonical_cloud_digest = cloud_writer.finalize();

    DigestWriter lbvh_writer{
        "MorseHGP3D/phase9/higher-support/morton-lbvh/v1"};
    lbvh_writer.size(spatial::MortonLbvhIndex::morton_bits_per_axis);
    lbvh_writer.size(index.point_count_);
    lbvh_writer.size(index.nodes_.size());
    lbvh_writer.size(index.leaves_.size());
    lbvh_writer.size(index.root_index_);
    lbvh_writer.size(index.build_counters_.point_count);
    lbvh_writer.size(index.build_counters_.node_count);
    lbvh_writer.size(index.build_counters_.maximum_depth);
    lbvh_writer.size(index.build_counters_.morton_collision_group_count);
    lbvh_writer.size(index.build_counters_.maximum_morton_collision_size);
    for (const std::uint64_t word : index.root_aabb_.lower_binary64_bits) {
      lbvh_writer.u64(word);
    }
    for (const std::uint64_t word : index.root_aabb_.upper_binary64_bits) {
      lbvh_writer.u64(word);
    }
    for (const spatial::MortonLeafRecord& leaf : index.leaves_) {
      lbvh_writer.u64(leaf.morton_code);
      lbvh_writer.u64(leaf.point_id);
      if (audit != nullptr) {
        increment(
            audit->lbvh_leaf_hash_count,
            "the higher-support manifest leaf hash count overflows size_t");
      }
    }
    for (const Node& current : index.nodes_) {
      for (const PointId point_id : current.lower_point_ids) {
        lbvh_writer.u64(point_id);
      }
      for (const PointId point_id : current.upper_point_ids) {
        lbvh_writer.u64(point_id);
      }
      if (current.is_leaf()) {
        lbvh_writer.u64(std::numeric_limits<std::uint64_t>::max());
        lbvh_writer.u64(std::numeric_limits<std::uint64_t>::max());
      } else {
        lbvh_writer.size(current.left_child);
        lbvh_writer.size(current.right_child);
      }
      lbvh_writer.size(current.leaf_begin);
      lbvh_writer.size(current.leaf_end);
      if (audit != nullptr) {
        increment(
            audit->lbvh_node_hash_count,
            "the higher-support manifest node hash count overflows size_t");
      }
    }
    manifest.lbvh_digest = lbvh_writer.finalize();

    DigestWriter semantic_writer{
        "MorseHGP3D/phase9/higher-support/checkpoint-manifest/v6"};
    semantic_writer.u32(manifest.schema_version);
    semantic_writer.u32(manifest.traversal_version);
    semantic_writer.text(higher_support_stream_proof_basis);
    semantic_writer.text("reference_cpu");
    semantic_writer.text("hgp_reduced");
    semantic_writer.text("certified");
    semantic_writer.size(3U);
    semantic_writer.size(4U);
    semantic_writer.size(manifest.point_count);
    semantic_writer.size(manifest.lbvh_node_count);
    semantic_writer.size(manifest.lbvh_leaf_count);
    semantic_writer.size(manifest.requested_maximum_order);
    semantic_writer.size(manifest.effective_maximum_order);
    semantic_writer.size(manifest.maximum_relevant_closed_rank);
    semantic_writer.identifier(manifest.canonical_cloud_digest);
    semantic_writer.identifier(manifest.lbvh_digest);
    manifest.semantic_digest = semantic_writer.finalize();
    if (audit != nullptr) {
      audit->manifest_cached = true;
    }
    return manifest;
  }

  [[nodiscard]] static ExactHigherSupportCheckpointVerification
  verify_checkpoint_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      const ExactHigherSupportCheckpointManifest& authority_manifest,
      const ExactHigherSupportCheckpoint& checkpoint) {
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();
    ExactHigherSupportStreamBuilder validator{
        index,
        cloud,
        requested_maximum_order,
        ExactHigherSupportStreamBudget{
            maximum,
            maximum,
            maximum,
            maximum,
            maximum,
            maximum,
            maximum,
            maximum}};
    return validator.checkpoint_verification(
        authority_manifest, checkpoint);
  }

 private:
  using Node = spatial::MortonLbvhIndex::Node;

  enum class RankSearchOutcome : std::uint8_t {
    keep,
    prune,
    budget_exhausted,
  };

  enum class SparseBallOutcome : std::uint8_t {
    complete,
    rank_exceeded,
  };

  struct SparseBallClassification {
    SparseBallOutcome outcome{SparseBallOutcome::rank_exceeded};
    std::vector<PointId> interior_ids;
    std::size_t shell_count{};
    std::optional<PointId> canonical_extra_shell_witness_id;
    std::size_t exterior_count{};
  };

  struct RankSearchResult {
    RankSearchOutcome outcome{RankSearchOutcome::keep};
    exact::BigInt certified_point_count{0};
    std::vector<ExactHigherSupportRankReceipt> receipts;
  };

  struct LocalRankProbeCell {
    std::size_t root_node_index{};
    std::vector<std::size_t> fallback_leaf_node_indices;
  };

  void validate_inputs(std::size_t requested_maximum_order) const {
    if (!index_.validated_for(cloud_)) {
      throw std::invalid_argument(
          "the higher-support stream requires the supplied cloud's Morton LBVH");
    }
    if (cloud_.size() == 0U) {
      throw std::invalid_argument(
          "the higher-support stream requires a nonempty point cloud");
    }
    if (requested_maximum_order == 0U ||
        requested_maximum_order > higher_support_maximum_requested_order) {
      throw std::out_of_range(
          "the higher-support stream requires 1<=Kmax<=10");
    }
  }

  void validate_positive_decision_source() const {
    if (positive_decision_source_ != nullptr &&
        !positive_decision_source_->bound_to(index_, cloud_)) {
      throw std::invalid_argument(
          "the higher-support accelerator decision source is not bound to "
          "the supplied cloud and LBVH");
    }
    if (terminal_geometry_decision_source_ != nullptr &&
        !terminal_geometry_decision_source_->bound_to(index_, cloud_)) {
      throw std::invalid_argument(
          "the higher-support terminal geometry source is not bound to "
          "the supplied cloud and LBVH");
    }
  }

  void prefetch_sparse_morton_frontier_slice() const {
    if (positive_decision_source_ == nullptr || frontier_.empty()) {
      return;
    }
    const std::size_t positive_capacity =
        positive_decision_source_->maximum_prefetch_task_count();
    const std::size_t terminal_capacity =
        terminal_geometry_decision_source_ != nullptr
        ? terminal_geometry_decision_source_->maximum_prefetch_task_count()
        : 0U;
    const std::size_t capacity =
        std::max(positive_capacity, terminal_capacity);
    const std::size_t count = std::min(capacity, frontier_.size());
    if (count == 0U) {
      return;
    }
    // frontier_.back() is the deterministic next Morton product.  No dense
    // neighbourhood is invented here: this bounded slice contains only live
    // nodes of the already-certified grouped sparse partition.  The slice is
    // already contiguous, so no auxiliary product catalogue is allocated.
    std::vector<ExactHigherSupportFrontierEntry> product_requests;
    std::vector<ExactHigherSupportFrontierEntry> terminal_requests;
    product_requests.reserve(std::min(positive_capacity, count));
    terminal_requests.reserve(std::min(terminal_capacity, count));
    const std::size_t begin = frontier_.size() - count;
    for (std::size_t index = begin; index < frontier_.size(); ++index) {
      const ExactHigherSupportFrontierEntry& entry = frontier_[index];
      if (is_terminal(entry)) {
        if (terminal_requests.size() < terminal_capacity) {
          terminal_requests.push_back(entry);
        }
      } else if (product_requests.size() < positive_capacity) {
        product_requests.push_back(entry);
      }
    }
    if (!product_requests.empty()) {
      positive_decision_source_->prefetch_no_well_centered_supports(
          product_requests);
    }
    if (!terminal_requests.empty()) {
      terminal_geometry_decision_source_->prefetch(terminal_requests);
    }
  }

  [[nodiscard]] const Node& node(std::size_t node_index) const {
    if (node_index >= index_.nodes_.size()) {
      throw std::logic_error(
          "a higher-support record references an invalid LBVH node");
    }
    return index_.nodes_[node_index];
  }

  [[nodiscard]] ExactHigherSupportNodeGroup make_group(
      std::size_t node_index,
      std::size_t multiplicity) const {
    const Node& current = node(node_index);
    if (multiplicity == 0U || multiplicity > 4U ||
        current.leaf_end - current.leaf_begin < multiplicity) {
      throw std::logic_error(
          "a higher-support group has an invalid multiplicity");
    }
    return ExactHigherSupportNodeGroup{
        checked_u64(
            node_index,
            "a higher-support node index does not fit uint64"),
        checked_u64(
            current.leaf_begin,
            "a higher-support Morton range does not fit uint64"),
        checked_u64(
            current.leaf_end,
            "a higher-support Morton range does not fit uint64"),
        static_cast<std::uint8_t>(multiplicity)};
  }

  [[nodiscard]] ExactHigherSupportFrontierEntry make_root_entry(
      std::size_t support_size) const {
    ExactHigherSupportFrontierEntry entry;
    entry.support_size = static_cast<std::uint8_t>(support_size);
    entry.group_count = 1U;
    entry.groups[0] = make_group(index_.root_index_, support_size);
    static_cast<void>(entry_support_count(entry));
    return entry;
  }

  [[nodiscard]] ExactHigherSupportFrontierEntry make_entry(
      std::size_t support_size,
      std::vector<std::pair<std::size_t, std::size_t>> groups) const {
    std::sort(
        groups.begin(),
        groups.end(),
        [this](const auto& left, const auto& right) {
          const Node& left_node = node(left.first);
          const Node& right_node = node(right.first);
          if (left_node.leaf_begin != right_node.leaf_begin) {
            return left_node.leaf_begin < right_node.leaf_begin;
          }
          return left.first < right.first;
        });
    if (groups.empty() || groups.size() > support_size ||
        groups.size() > 4U) {
      throw std::logic_error(
          "a higher-support product has an invalid group count");
    }
    ExactHigherSupportFrontierEntry entry;
    entry.support_size = static_cast<std::uint8_t>(support_size);
    entry.group_count = static_cast<std::uint8_t>(groups.size());
    for (std::size_t index = 0U; index < groups.size(); ++index) {
      entry.groups[index] = make_group(
          groups[index].first, groups[index].second);
    }
    static_cast<void>(entry_support_count(entry));
    return entry;
  }

  [[nodiscard]] std::size_t group_node_index(
      const ExactHigherSupportNodeGroup& group) const {
    const std::size_t node_index = checked_size(
        group.node_index,
        "a higher-support node index does not fit size_t");
    const Node& current = node(node_index);
    if (group.leaf_begin != checked_u64(
                                current.leaf_begin,
                                "a Morton range does not fit uint64") ||
        group.leaf_end != checked_u64(
                              current.leaf_end,
                              "a Morton range does not fit uint64")) {
      throw std::logic_error(
          "a higher-support group contradicts its LBVH node receipt");
    }
    return node_index;
  }

  [[nodiscard]] exact::BigInt entry_support_count(
      const ExactHigherSupportFrontierEntry& entry) const {
    const std::size_t support_size = entry.support_size;
    const std::size_t group_count = entry.group_count;
    if ((support_size != 3U && support_size != 4U) ||
        group_count == 0U || group_count > support_size) {
      throw std::logic_error(
          "a higher-support frontier entry has an invalid arity");
    }
    std::size_t multiplicity_sum = 0U;
    exact::BigInt coverage{1};
    std::uint64_t previous_end = 0U;
    for (std::size_t index = 0U; index < group_count; ++index) {
      const ExactHigherSupportNodeGroup& group = entry.groups[index];
      const std::size_t node_index = group_node_index(group);
      static_cast<void>(node_index);
      const std::size_t multiplicity = group.multiplicity;
      const std::size_t leaf_begin = checked_size(
          group.leaf_begin,
          "a higher-support Morton range does not fit size_t");
      const std::size_t leaf_end = checked_size(
          group.leaf_end,
          "a higher-support Morton range does not fit size_t");
      if (multiplicity == 0U || leaf_begin >= leaf_end ||
          leaf_end - leaf_begin < multiplicity ||
          (index != 0U && previous_end > group.leaf_begin)) {
        throw std::logic_error(
            "a higher-support frontier entry has overlapping or invalid groups");
      }
      previous_end = group.leaf_end;
      multiplicity_sum = checked_add(
          multiplicity_sum,
          multiplicity,
          "a higher-support multiplicity sum overflows size_t");
      coverage *= exact_binomial(leaf_end - leaf_begin, multiplicity);
    }
    const ExactHigherSupportNodeGroup padding{};
    for (std::size_t index = group_count; index < entry.groups.size(); ++index) {
      if (entry.groups[index] != padding) {
        throw std::logic_error(
            "a higher-support frontier entry has noncanonical padding");
      }
    }
    if (multiplicity_sum != support_size || coverage <= 0) {
      throw std::logic_error(
          "a higher-support frontier entry has invalid exact coverage");
    }
    return coverage;
  }

  [[nodiscard]] spatial::ExactDyadicAabb3 node_box(
      std::size_t node_index) const {
    const Node& current = node(node_index);
    spatial::ExactDyadicAabb3 box{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      box.lower_binary64_bits[axis] =
          cloud_.point(current.lower_point_ids[axis])
              .canonical_input_bits()[axis];
      box.upper_binary64_bits[axis] =
          cloud_.point(current.upper_point_ids[axis])
              .canonical_input_bits()[axis];
    }
    return box;
  }

  [[nodiscard]] std::array<spatial::ExactDyadicAabb3, 4>
  support_boxes(const ExactHigherSupportFrontierEntry& entry) const {
    static_cast<void>(entry_support_count(entry));
    std::array<spatial::ExactDyadicAabb3, 4> boxes{};
    std::size_t output_index = 0U;
    for (std::size_t group_index = 0U;
         group_index < entry.group_count;
         ++group_index) {
      const ExactHigherSupportNodeGroup& group = entry.groups[group_index];
      const spatial::ExactDyadicAabb3 box =
          node_box(group_node_index(group));
      for (std::size_t copy = 0U; copy < group.multiplicity; ++copy) {
        boxes[output_index] = box;
        ++output_index;
      }
    }
    if (output_index != entry.support_size) {
      throw std::logic_error(
          "a higher-support product omitted a support box");
    }
    return boxes;
  }

  [[nodiscard]] bool is_terminal(
      const ExactHigherSupportFrontierEntry& entry) const {
    static_cast<void>(entry_support_count(entry));
    if (entry.group_count != entry.support_size) {
      return false;
    }
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      if (entry.groups[index].multiplicity != 1U ||
          !node(group_node_index(entry.groups[index])).is_leaf()) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] std::array<PointId, 4> terminal_support_ids(
      const ExactHigherSupportFrontierEntry& entry) const {
    if (!is_terminal(entry)) {
      throw std::logic_error(
          "a nonterminal higher-support product has no unique support");
    }
    std::array<PointId, 4> support_ids{};
    for (std::size_t index = 0U; index < entry.support_size; ++index) {
      const Node& current = node(group_node_index(entry.groups[index]));
      support_ids[index] = index_.leaves_[current.leaf_begin].point_id;
    }
    bool repeated = false;
    if (entry.support_size == 3U) {
      const auto end = support_ids.begin() + 3;
      std::sort(support_ids.begin(), end);
      repeated = std::adjacent_find(support_ids.begin(), end) != end;
    } else if (entry.support_size == 4U) {
      std::sort(support_ids.begin(), support_ids.end());
      repeated =
          std::adjacent_find(support_ids.begin(), support_ids.end()) !=
          support_ids.end();
    } else {
      throw std::logic_error(
          "a terminal higher-support product has an invalid support size");
    }
    if (repeated) {
      throw std::logic_error(
          "a terminal higher-support product repeated a point");
    }
    return support_ids;
  }

  [[nodiscard]] ExactHigherSupportNodeReceipt make_node_receipt(
      std::size_t node_index) const {
    const Node& current = node(node_index);
    return ExactHigherSupportNodeReceipt{
        checked_u64(
            node_index,
            "a higher-support receipt node does not fit uint64"),
        checked_u64(
            current.leaf_begin,
            "a higher-support receipt range does not fit uint64"),
        checked_u64(
            current.leaf_end,
            "a higher-support receipt range does not fit uint64")};
  }

  [[nodiscard]] std::size_t receipt_node_index(
      const ExactHigherSupportNodeReceipt& receipt) const {
    const std::size_t node_index = checked_size(
        receipt.node_index,
        "a higher-support receipt node does not fit size_t");
    const Node& current = node(node_index);
    if (receipt.leaf_begin != checked_u64(
                                  current.leaf_begin,
                                  "a receipt range does not fit uint64") ||
        receipt.leaf_end != checked_u64(
                                current.leaf_end,
                                "a receipt range does not fit uint64")) {
      throw std::logic_error(
          "a higher-support rank receipt contradicts its LBVH node");
    }
    return node_index;
  }

  [[nodiscard]] ExactHigherSupportCheckpointVerification
  checkpoint_verification(
      const ExactHigherSupportCheckpointManifest& authority_manifest,
      const ExactHigherSupportCheckpoint& checkpoint) const {
    ExactHigherSupportCheckpointVerification verification;
    verification.manifest_matches_authorities =
        checkpoint.manifest == authority_manifest;
    verification.checksum_matches_payload =
        checkpoint.checkpoint_digest == checkpoint_digest(checkpoint);
    if (!verification.manifest_matches_authorities ||
        !verification.checksum_matches_payload) {
      return verification;
    }

    exact::BigInt remaining_support_count{0};
    bool frontier_valid = true;
    try {
      for (const ExactHigherSupportFrontierEntry& entry :
           checkpoint.frontier) {
        remaining_support_count += entry_support_count(entry);
      }
    } catch (const std::exception&) {
      frontier_valid = false;
    }
    verification.frontier_locally_valid = frontier_valid;

    bool pending_valid = frontier_valid;
    bool receipts_valid = frontier_valid;
    bool pending_leaf_analysis_started = false;
    if (checkpoint.pending_product.has_value()) {
      if (checkpoint.frontier.empty() ||
          checkpoint.pending_product->product != checkpoint.frontier.back()) {
        pending_valid = false;
        receipts_valid = false;
      } else {
        try {
          const ExactHigherSupportPendingProduct& pending =
              *checkpoint.pending_product;
          const ExactHigherSupportFrontierEntry& entry = pending.product;
          static_cast<void>(entry_support_count(entry));
          const bool empty_rank_payload =
              !pending.rank_search_started &&
              pending.rank_frontier.empty() &&
              pending.rank_probe_next_candidate_index == 0U &&
              pending.rank_probe_next_fallback_leaf_index == 0U &&
              !pending.rank_probe_fallback_active &&
              !pending.rank_probe_center_seed_leaf_position.has_value() &&
              pending.strict_interior_receipts.empty() &&
              pending.certified_strict_interior_point_count == 0;
          if (pending.stage !=
                  ExactHigherSupportPendingStage::classify_leaf &&
              pending.leaf_analysis_started) {
            pending_valid = false;
          }
          pending_leaf_analysis_started =
              pending.stage == ExactHigherSupportPendingStage::classify_leaf &&
              pending.leaf_analysis_started;
          const std::size_t required_rank_receipt_count =
              required_strict_interior_count(entry.support_size);
          bool rank_probe_state_valid = pending.rank_frontier.empty();
          std::size_t rank_probe_candidate_count = 0U;
          std::vector<LocalRankProbeCell> rank_probe_candidates;
          if (pending.rank_search_started) {
            const std::optional<std::size_t> expected_center_seed =
                representative_center_seed_leaf_position(entry);
            std::optional<std::size_t> persisted_center_seed;
            if (pending.rank_probe_center_seed_leaf_position.has_value()) {
              persisted_center_seed = checked_size(
                  *pending.rank_probe_center_seed_leaf_position,
                  "a persisted higher-support local rank center seed does not fit size_t");
            }
            rank_probe_candidates = local_rank_probe_candidate_cells(
                entry, expected_center_seed);
            rank_probe_candidate_count = rank_probe_candidates.size();
            rank_probe_state_valid = rank_probe_state_valid &&
                persisted_center_seed == expected_center_seed &&
                pending.rank_probe_next_candidate_index <=
                    rank_probe_candidate_count;
            if (pending.rank_probe_fallback_active) {
              rank_probe_state_valid = rank_probe_state_valid &&
                  pending.rank_probe_next_candidate_index > 0U;
              if (pending.rank_probe_next_candidate_index > 0U &&
                  pending.rank_probe_next_candidate_index <=
                      rank_probe_candidate_count) {
                const LocalRankProbeCell& active =
                    rank_probe_candidates[
                        pending.rank_probe_next_candidate_index - 1U];
                const std::array<spatial::ExactDyadicAabb3, 4> boxes =
                    support_boxes(entry);
                rank_probe_state_valid = rank_probe_state_valid &&
                    pending.rank_probe_next_fallback_leaf_index <
                        active.fallback_leaf_node_indices.size() &&
                    exact_higher_support_product_query_cell_decision(
                        std::span<const spatial::ExactDyadicAabb3>{
                            boxes.data(), entry.support_size},
                        node_box(active.root_node_index)) ==
                        ExactHigherSupportProductQueryCellDecision::
                            inconclusive;
              }
            } else {
              rank_probe_state_valid = rank_probe_state_valid &&
                  pending.rank_probe_next_fallback_leaf_index == 0U;
            }
          } else {
            rank_probe_state_valid = rank_probe_state_valid &&
                pending.rank_frontier.empty() &&
                pending.rank_probe_next_candidate_index == 0U &&
                pending.rank_probe_next_fallback_leaf_index == 0U &&
                !pending.rank_probe_fallback_active &&
                !pending.rank_probe_center_seed_leaf_position.has_value();
          }
          const bool receipt_payload_within_caps = rank_probe_state_valid &&
              required_rank_receipt_count <= 9U &&
              pending.strict_interior_receipts.size() <=
                  required_rank_receipt_count &&
              pending.certified_strict_interior_point_count >= 0 &&
              pending.certified_strict_interior_point_count <=
                  exact::BigInt{cloud_.size()};
          if (!receipt_payload_within_caps) {
            pending_valid = false;
            receipts_valid = false;
          }

          if (receipt_payload_within_caps) {
            switch (pending.stage) {
              case ExactHigherSupportPendingStage::analyze_product:
                if (!empty_rank_payload) {
                  pending_valid = false;
                }
                break;
              case ExactHigherSupportPendingStage::emit_well_prune:
                if (!empty_rank_payload ||
                    !no_well_centered_support_certified(entry)) {
                  pending_valid = false;
                }
                break;
              case ExactHigherSupportPendingStage::rank_search: {
                if (required_strict_interior_count(entry.support_size) == 0U ||
                    !all_well_centered_support_certified(entry) ||
                    (!pending.rank_search_started && !empty_rank_payload) ||
                    (pending.rank_search_started &&
                     !pending.rank_probe_fallback_active &&
                     pending.rank_probe_next_candidate_index >=
                         rank_probe_candidate_count)) {
                  pending_valid = false;
                }
                break;
              }
              case ExactHigherSupportPendingStage::emit_rank_prune:
                if (!pending.rank_frontier.empty() ||
                    pending.rank_probe_fallback_active ||
                    pending.rank_probe_next_fallback_leaf_index != 0U ||
                    pending.certified_strict_interior_point_count <
                        exact::BigInt{required_strict_interior_count(
                            entry.support_size)} ||
                    no_well_centered_support_certified(entry)) {
                  pending_valid = false;
                }
                break;
              case ExactHigherSupportPendingStage::expand_product:
                if (!empty_rank_payload || is_terminal(entry)) {
                  pending_valid = false;
                }
                break;
              case ExactHigherSupportPendingStage::classify_leaf: {
                if (!empty_rank_payload || !is_terminal(entry) ||
                    required_strict_interior_count(entry.support_size) == 0U) {
                  pending_valid = false;
                }
                if (pending.leaf_analysis_started) {
                  const std::array<PointId, 4> support_ids =
                      terminal_support_ids(entry);
                  if (entry.support_size == 3U) {
                    std::array<exact::ExactRational3, 3> points{};
                    for (std::size_t index = 0U; index < points.size();
                         ++index) {
                      points[index] = cloud_.point(support_ids[index]).exact();
                    }
                    if (exact::analyze_circumcenter_support(points).status() !=
                        exact::CircumcenterSupportStatus::minimal) {
                      pending_valid = false;
                    }
                  } else {
                    std::array<exact::ExactRational3, 4> points{};
                    for (std::size_t index = 0U; index < points.size();
                         ++index) {
                      points[index] = cloud_.point(support_ids[index]).exact();
                    }
                    if (exact::analyze_circumcenter_support(points).status() !=
                        exact::CircumcenterSupportStatus::minimal) {
                      pending_valid = false;
                    }
                  }
                }
                break;
              }
              default:
                pending_valid = false;
                break;
            }

            std::vector<std::pair<std::uint64_t, std::uint64_t>> intervals;
            intervals.reserve(checked_add(
                0U,
                pending.strict_interior_receipts.size(),
                "the higher-support checkpoint receipt count overflows size_t"));
            exact::BigInt recomputed_receipt_count{0};
            const std::array<spatial::ExactDyadicAabb3, 4> boxes =
                support_boxes(entry);
            const std::span<const spatial::ExactDyadicAabb3>
                support_box_span{boxes.data(), entry.support_size};
            for (const ExactHigherSupportNodeReceipt& receipt :
                 pending.strict_interior_receipts) {
              const std::size_t query_node_index =
                  receipt_node_index(receipt);
              const Node& query = node(query_node_index);
              bool receipt_was_locally_probed = false;
              const std::size_t consumed_candidate_count = std::min(
                  pending.rank_probe_next_candidate_index,
                  rank_probe_candidates.size());
              for (std::size_t candidate_index = 0U;
                   candidate_index < consumed_candidate_count;
                   ++candidate_index) {
                const LocalRankProbeCell& candidate =
                    rank_probe_candidates[candidate_index];
                if (candidate.root_node_index == query_node_index) {
                  receipt_was_locally_probed = true;
                  break;
                }
                const auto fallback = std::find(
                    candidate.fallback_leaf_node_indices.begin(),
                    candidate.fallback_leaf_node_indices.end(),
                    query_node_index);
                if (fallback ==
                    candidate.fallback_leaf_node_indices.end()) {
                  continue;
                }
                const auto root_decision =
                    exact_higher_support_product_query_cell_decision(
                        support_box_span,
                        node_box(candidate.root_node_index));
                std::size_t consumed_fallback_count =
                    candidate.fallback_leaf_node_indices.size();
                if (pending.rank_probe_fallback_active &&
                    candidate_index + 1U ==
                        consumed_candidate_count) {
                  consumed_fallback_count =
                      pending.rank_probe_next_fallback_leaf_index;
                }
                const std::size_t fallback_index =
                    static_cast<std::size_t>(std::distance(
                        candidate.fallback_leaf_node_indices.begin(),
                        fallback));
                if (root_decision ==
                        ExactHigherSupportProductQueryCellDecision::
                            inconclusive &&
                    fallback_index < consumed_fallback_count) {
                  receipt_was_locally_probed = true;
                  break;
                }
              }
              if (!receipt_was_locally_probed ||
                  node_range_intersects_support_domain(query, entry) ||
                  !exact_higher_support_product_query_strictly_inside_every_independent_sphere_certified(
                      support_box_span,
                      node_box(query_node_index))) {
                receipts_valid = false;
              }
              recomputed_receipt_count +=
                  query.leaf_end - query.leaf_begin;
              intervals.emplace_back(receipt.leaf_begin, receipt.leaf_end);
            }
            std::sort(intervals.begin(), intervals.end());
            for (std::size_t index = 1U; index < intervals.size(); ++index) {
              if (intervals[index].first < intervals[index - 1U].second) {
                receipts_valid = false;
              }
            }
            if (recomputed_receipt_count !=
                pending.certified_strict_interior_point_count) {
              receipts_valid = false;
            }
            if (pending.stage ==
                    ExactHigherSupportPendingStage::rank_search &&
                pending.certified_strict_interior_point_count >=
                    exact::BigInt{required_strict_interior_count(
                        entry.support_size)}) {
              pending_valid = false;
            }
          }
        } catch (const std::exception&) {
          pending_valid = false;
          receipts_valid = false;
        }
      }
    }
    verification.pending_product_locally_valid = pending_valid;
    verification.pending_receipts_recertified = receipts_valid;

    bool audit_valid = frontier_valid && pending_valid && receipts_valid;
    try {
      const ExactHigherSupportStreamAudit& audit =
          checkpoint.cumulative_audit;
      const exact::BigInt terminal_sum =
          audit.well_centering_pruned_support_count +
          audit.rank_pruned_support_count +
          audit.leaf_classified_support_count;
      const std::size_t classified_nonminimal = checked_add(
          checked_add(
              audit.affinely_dependent_leaf_count,
              audit.boundary_reduced_leaf_count,
              "the higher-support leaf identity overflows size_t"),
          audit.exterior_circumcenter_leaf_count,
          "the higher-support leaf identity overflows size_t");
      const std::size_t classified_minimal = checked_add(
          checked_add(
              audit.above_rank_leaf_count,
              audit.accepted_event_count,
              "the higher-support minimal identity overflows size_t"),
          audit.relevant_extra_shell_diagnostic_count,
          "the higher-support minimal identity overflows size_t");
      const std::size_t emitted_record_identity = checked_add(
          checked_add(
              audit.emitted_prune_certificate_count,
              audit.accepted_event_count,
              "the higher-support record identity overflows size_t"),
          audit.relevant_extra_shell_diagnostic_count,
          "the higher-support record identity overflows size_t");
      const std::size_t leaf_analysis_identity = checked_add(
          classified_nonminimal,
          audit.minimal_leaf_count,
          "the higher-support leaf-analysis identity overflows size_t");
      const std::size_t leaf_classified_identity = checked_add(
          classified_nonminimal,
          classified_minimal,
          "the higher-support leaf-classified identity overflows size_t");
      const std::size_t minimal_identity = checked_add(
          classified_minimal,
          pending_leaf_analysis_started ? 1U : 0U,
          "the higher-support pending-minimal identity overflows size_t");
      const std::size_t active_rank_probe_count =
          checkpoint.pending_product.has_value() &&
                  checkpoint.pending_product->stage ==
                      ExactHigherSupportPendingStage::rank_search &&
                  checkpoint.pending_product->rank_search_started
              ? 1U
              : 0U;
      const std::size_t finished_or_active_rank_probe_count = checked_add(
          checked_add(
              audit.rank_local_probe_pruned_product_count,
              audit.rank_local_probe_fail_open_product_count,
              "the higher-support local rank outcome count overflows size_t"),
          active_rank_probe_count,
          "the higher-support local rank active count overflows size_t");
      audit_valid = audit_valid &&
          audit.total_support_count ==
              exact_higher_support_candidate_universe_size(cloud_.size()) &&
          audit.total_support_count >= 0 &&
          audit.well_centering_pruned_support_count >= 0 &&
          audit.rank_pruned_support_count >= 0 &&
          audit.leaf_classified_support_count >= 0 &&
          audit.resolved_support_count >= 0 &&
          remaining_support_count >= 0 &&
          audit.remaining_frontier_support_count == remaining_support_count &&
          audit.resolved_support_count + remaining_support_count ==
              audit.total_support_count &&
          audit.resolved_support_count == terminal_sum &&
          audit.work_unit_count == checked_add(
              audit.support_product_visit_count,
              audit.rank_witness_node_visit_count,
              "the higher-support work identity overflows size_t") &&
          audit.rank_local_probe_attempt_count == audit.rank_search_count &&
          audit.rank_local_probe_attempt_count ==
              finished_or_active_rank_probe_count &&
          checked_add(
              audit.rank_local_probe_attempt_count,
              audit.rank_local_probe_geometric_gate_skip_count,
              "the higher-support probe-gate audit overflows size_t") <=
              audit.support_product_visit_count &&
          audit.rank_local_probe_center_seed_count <=
              audit.rank_local_probe_attempt_count &&
          audit.rank_local_probe_center_path_node_visit_count >=
              audit.rank_local_probe_center_seed_count &&
          audit.rank_local_probe_candidate_evaluation_count ==
              audit.rank_witness_node_visit_count &&
          exact::BigInt{
              audit.rank_local_probe_candidate_evaluation_count} <=
              exact::BigInt{audit.rank_local_probe_attempt_count} *
                  higher_support_local_rank_probe_maximum_evaluation_count &&
          audit.maximum_rank_local_probe_candidate_count <=
              higher_support_local_rank_probe_maximum_evaluation_count &&
          audit.maximum_rank_frontier_entry_count == 0U &&
          audit.rank_query_outside_or_boundary_node_count <=
              audit.rank_local_probe_candidate_evaluation_count &&
          audit.rank_query_outside_or_boundary_point_count >=
              audit.rank_query_outside_or_boundary_node_count &&
          exact::BigInt{
              audit.rank_query_outside_or_boundary_point_count} <=
              exact::BigInt{
                  audit.rank_query_outside_or_boundary_node_count} *
                  higher_support_local_rank_probe_maximum_threshold &&
          (audit.rank_query_outside_or_boundary_node_count != 0U ||
           audit.rank_query_outside_or_boundary_point_count == 0U) &&
          audit.leaf_support_analysis_count == leaf_analysis_identity &&
          audit.leaf_classified_support_count ==
              exact::BigInt{leaf_classified_identity} &&
          audit.minimal_leaf_count == minimal_identity &&
          audit.emitted_record_count == emitted_record_identity &&
          checkpoint.output_record_count == audit.emitted_record_count &&
          (checkpoint.output_record_count != 0U ||
           checkpoint.output_chain_digest == initial_output_chain_digest()) &&
          audit.maximum_frontier_entry_count >= checkpoint.frontier.size() &&
          (!checkpoint.pending_product.has_value() ||
           audit.maximum_rank_frontier_entry_count >=
               checkpoint.pending_product->rank_frontier.size()) &&
          audit.exact_bigint_universe_certified &&
          audit.grouped_partition_accounting_certified;
    } catch (const std::exception&) {
      audit_valid = false;
    }
    verification.required_audit_identities_hold = audit_valid;
    verification.local_integrity_verified =
        verification.manifest_matches_authorities &&
        verification.checksum_matches_payload &&
        verification.frontier_locally_valid &&
        verification.pending_product_locally_valid &&
        verification.pending_receipts_recertified &&
        verification.required_audit_identities_hold;
    return verification;
  }

  [[nodiscard]] bool consume_work_unit() {
    if (consumed_since(
            result_.audit.work_unit_count,
            chunk_audit_origin_.work_unit_count,
            "the higher-support work audit moved backwards") >=
        result_.budget.maximum_work_unit_count) {
      stop(ExactHigherSupportStopReason::work_unit_limit);
      return false;
    }
    increment(
        result_.audit.work_unit_count,
        "the higher-support work count overflows size_t");
    return true;
  }

  [[nodiscard]] static std::size_t consumed_since(
      std::size_t current,
      std::size_t origin,
      const char* message) {
    if (current < origin) {
      throw std::logic_error(message);
    }
    return current - origin;
  }

  void append_output_record(
      const ExactHigherSupportEvent& event,
      ExactHigherSupportStreamChunk::RecordKind kind) {
    if (kind != ExactHigherSupportStreamChunk::RecordKind::event) {
      throw std::logic_error("a higher-support event has the wrong record kind");
    }
    output_chain_digest_ = extend_output_chain(output_chain_digest_, event);
    increment(
        output_record_count_,
        "the higher-support output-record count overflows size_t");
    record_order_.push_back(kind);
  }

  void append_output_record(
      const ExactHigherSupportExtraShellDiagnostic& diagnostic,
      ExactHigherSupportStreamChunk::RecordKind kind) {
    if (kind != ExactHigherSupportStreamChunk::RecordKind::
                    relevant_extra_shell_diagnostic) {
      throw std::logic_error(
          "a higher-support diagnostic has the wrong record kind");
    }
    output_chain_digest_ =
        extend_output_chain(output_chain_digest_, diagnostic);
    increment(
        output_record_count_,
        "the higher-support output-record count overflows size_t");
    record_order_.push_back(kind);
  }

  void append_output_record(
      const ExactHigherSupportPruneCertificate& certificate,
      ExactHigherSupportStreamChunk::RecordKind kind) {
    if (kind != ExactHigherSupportStreamChunk::RecordKind::prune_certificate) {
      throw std::logic_error(
          "a higher-support prune has the wrong record kind");
    }
    output_chain_digest_ =
        extend_output_chain(output_chain_digest_, certificate);
    increment(
        output_record_count_,
        "the higher-support output-record count overflows size_t");
    record_order_.push_back(kind);
  }

  void stop(ExactHigherSupportStopReason reason) {
    if (!stopped_) {
      stopped_ = true;
      result_.status = ExactHigherSupportStreamStatus::budget_exhausted;
      result_.stop_reason = reason;
    }
  }

  [[nodiscard]] bool auxiliary_frontier_preflight() {
    const std::size_t required = checked_add(
        index_.build_counters().maximum_depth,
        1U,
        "the higher-support auxiliary frontier bound overflows size_t");
    if (required >
        result_.budget.maximum_auxiliary_frontier_entry_count) {
      stop(ExactHigherSupportStopReason::auxiliary_frontier_entry_limit);
      return false;
    }
    return true;
  }

  [[nodiscard]] std::size_t required_strict_interior_count(
      std::size_t support_size) const {
    const std::size_t maximum_rank =
        result_.requirements.maximum_relevant_closed_rank;
    if (support_size > maximum_rank) {
      return 0U;
    }
    return maximum_rank - support_size + 1U;
  }

  [[nodiscard]] bool leaf_position_in_support_domain(
      std::size_t leaf_position,
      const ExactHigherSupportFrontierEntry& entry) const {
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      const Node& support = node(group_node_index(entry.groups[index]));
      if (support.leaf_begin <= leaf_position &&
          leaf_position < support.leaf_end) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] std::size_t bounded_external_node_index_at_position(
      std::size_t leaf_position,
      const ExactHigherSupportFrontierEntry& entry,
      std::size_t maximum_point_count) const {
    if (leaf_position >= index_.leaves_.size()) {
      throw std::out_of_range(
          "a higher-support local rank probe leaf position is outside the LBVH");
    }
    if (maximum_point_count == 0U ||
        leaf_position_in_support_domain(leaf_position, entry)) {
      throw std::logic_error(
          "a higher-support local rank probe requires one external leaf and a positive cell bound");
    }
    std::size_t current_index = index_.root_index_;
    while (true) {
      const Node& current = node(current_index);
      const std::size_t current_point_count =
          current.leaf_end - current.leaf_begin;
      if (current_point_count <= maximum_point_count &&
          !node_range_intersects_support_domain(current, entry)) {
        return current_index;
      }
      if (current.is_leaf()) {
        throw std::logic_error(
            "an external higher-support leaf did not yield a bounded external LBVH cell");
      }
      const Node& left = node(current.left_child);
      const Node& right = node(current.right_child);
      if (left.leaf_begin <= leaf_position &&
          leaf_position < left.leaf_end) {
        current_index = current.left_child;
      } else if (
          right.leaf_begin <= leaf_position &&
          leaf_position < right.leaf_end) {
        current_index = current.right_child;
      } else {
        throw std::logic_error(
            "a higher-support local rank probe lost its LBVH leaf position");
      }
    }
  }

  [[nodiscard]] std::size_t leaf_node_index_below(
      std::size_t ancestor_node_index,
      std::size_t leaf_position) const {
    std::size_t current_index = ancestor_node_index;
    while (true) {
      const Node& current = node(current_index);
      if (!(current.leaf_begin <= leaf_position &&
            leaf_position < current.leaf_end)) {
        throw std::logic_error(
            "a higher-support fallback leaf left its bounded candidate cell");
      }
      if (current.is_leaf()) {
        return current_index;
      }
      const Node& left = node(current.left_child);
      const Node& right = node(current.right_child);
      if (left.leaf_begin <= leaf_position &&
          leaf_position < left.leaf_end) {
        current_index = current.left_child;
      } else if (right.leaf_begin <= leaf_position &&
                 leaf_position < right.leaf_end) {
        current_index = current.right_child;
      } else {
        throw std::logic_error(
            "a higher-support fallback path lost its leaf position");
      }
    }
  }

  [[nodiscard]] std::optional<exact::ExactCenter3>
  representative_minimal_center(
      const ExactHigherSupportFrontierEntry& entry) const {
    static_cast<void>(entry_support_count(entry));
    std::array<exact::ExactRational3, 4U> representative{};
    std::size_t output_index = 0U;
    for (std::size_t group_index = 0U;
         group_index < entry.group_count;
         ++group_index) {
      const ExactHigherSupportNodeGroup& group = entry.groups[group_index];
      const Node& support = node(group_node_index(group));
      for (std::size_t copy = 0U; copy < group.multiplicity; ++copy) {
        representative[output_index] =
            cloud_.point(index_.leaves_[support.leaf_begin + copy].point_id)
                .exact();
        ++output_index;
      }
    }
    if (output_index != entry.support_size) {
      throw std::logic_error(
          "a higher-support local rank probe omitted a representative support point");
    }
    if (entry.support_size == 3U) {
      const std::array<exact::ExactRational3, 3U> support{
          representative[0], representative[1], representative[2]};
      const exact::CircumcenterSupportAnalysis analysis =
          exact::analyze_circumcenter_support(support);
      if (analysis.status() != exact::CircumcenterSupportStatus::minimal) {
        return std::nullopt;
      }
      return analysis.circumcenter_result().center();
    }
    if (entry.support_size == 4U) {
      const exact::CircumcenterSupportAnalysis analysis =
          exact::analyze_circumcenter_support(representative);
      if (analysis.status() != exact::CircumcenterSupportStatus::minimal) {
        return std::nullopt;
      }
      return analysis.circumcenter_result().center();
    }
    throw std::logic_error(
        "a higher-support local rank probe has an invalid support arity");
  }

  [[nodiscard]] std::optional<std::size_t>
  representative_center_seed_leaf_position(
      const ExactHigherSupportFrontierEntry& entry,
      std::size_t* path_node_visit_count = nullptr) const {
    const std::optional<exact::ExactCenter3> center =
        representative_minimal_center(entry);
    if (!center.has_value()) {
      return std::nullopt;
    }
    std::size_t current_index = index_.root_index_;
    while (true) {
      if (path_node_visit_count != nullptr) {
        increment(
            *path_node_visit_count,
            "the higher-support local rank center path count overflows size_t");
      }
      const Node& current = node(current_index);
      if (current.is_leaf()) {
        return current.leaf_begin;
      }
      const exact::ExactLevel left_distance =
          index_.minimum_squared_distance_to_node(
              cloud_, current.left_child, *center);
      const exact::ExactLevel right_distance =
          index_.minimum_squared_distance_to_node(
              cloud_, current.right_child, *center);
      const Node& left = node(current.left_child);
      const Node& right = node(current.right_child);
      if (left_distance < right_distance ||
          (left_distance == right_distance &&
           left.leaf_begin < right.leaf_begin)) {
        current_index = current.left_child;
      } else {
        current_index = current.right_child;
      }
    }
  }

  [[nodiscard]] std::vector<LocalRankProbeCell>
  local_rank_probe_candidate_cells(
      const ExactHigherSupportFrontierEntry& entry,
      std::optional<std::size_t> center_seed_leaf_position) const {
    const std::size_t threshold =
        required_strict_interior_count(entry.support_size);
    if (threshold == 0U ||
        threshold > higher_support_local_rank_probe_maximum_threshold) {
      throw std::logic_error(
          "a higher-support local rank probe threshold is outside [1, 9]");
    }
    std::vector<std::size_t> leaf_positions;
    leaf_positions.reserve(
        higher_support_local_rank_probe_maximum_candidate_count);
    const auto append_leaf_position =
        [this, &entry, &leaf_positions](std::size_t position) {
          if (position >= index_.leaves_.size() ||
              leaf_position_in_support_domain(position, entry) ||
              std::find(
                  leaf_positions.begin(), leaf_positions.end(), position) !=
                  leaf_positions.end()) {
            return;
          }
          leaf_positions.push_back(position);
        };

    // The representative-center path is proposal-only.  Its leaf and Morton
    // halo are visited first, but every retained witness is still recertified
    // universally against the complete support-box product below.
    if (center_seed_leaf_position.has_value()) {
      append_leaf_position(*center_seed_leaf_position);
      for (std::size_t distance = 1U; distance <= threshold; ++distance) {
        if (*center_seed_leaf_position >= distance) {
          append_leaf_position(*center_seed_leaf_position - distance);
        }
        if (distance < index_.leaves_.size() &&
            *center_seed_leaf_position <
                index_.leaves_.size() - distance) {
          append_leaf_position(*center_seed_leaf_position + distance);
        }
      }
    }

    for (std::size_t distance = 1U; distance <= threshold; ++distance) {
      for (std::size_t group_index = 0U;
           group_index < entry.group_count;
           ++group_index) {
        const Node& support =
            node(group_node_index(entry.groups[group_index]));
        if (support.leaf_begin >= distance) {
          append_leaf_position(support.leaf_begin - distance);
        }
        if (distance <= index_.leaves_.size() &&
            support.leaf_end <= index_.leaves_.size() - distance) {
          append_leaf_position(support.leaf_end + distance - 1U);
        }
      }
    }
    if (leaf_positions.size() >
        higher_support_local_rank_probe_maximum_candidate_count) {
      throw std::logic_error(
          "a higher-support local rank probe exceeded its fixed candidate bound");
    }
    std::vector<LocalRankProbeCell> candidate_cells;
    candidate_cells.reserve(leaf_positions.size());
    for (const std::size_t position : leaf_positions) {
      const std::size_t candidate_node_index =
          bounded_external_node_index_at_position(
              position, entry, threshold);
      auto candidate = std::find_if(
          candidate_cells.begin(),
          candidate_cells.end(),
          [candidate_node_index](const LocalRankProbeCell& cell) {
            return cell.root_node_index == candidate_node_index;
          });
      if (candidate == candidate_cells.end()) {
        candidate_cells.push_back(
            LocalRankProbeCell{candidate_node_index, {}});
        candidate = std::prev(candidate_cells.end());
      }
      candidate->fallback_leaf_node_indices.push_back(
          leaf_node_index_below(candidate_node_index, position));
    }
    for (std::size_t left = 0U;
         left < candidate_cells.size();
         ++left) {
      const Node& left_node = node(candidate_cells[left].root_node_index);
      if (left_node.leaf_end - left_node.leaf_begin > threshold ||
          node_range_intersects_support_domain(left_node, entry) ||
          candidate_cells[left].fallback_leaf_node_indices.empty()) {
        throw std::logic_error(
            "a higher-support local rank root violates its bounded external-cell contract");
      }
      for (const std::size_t fallback_leaf_node_index :
           candidate_cells[left].fallback_leaf_node_indices) {
        const Node& fallback_leaf = node(fallback_leaf_node_index);
        if (!fallback_leaf.is_leaf() ||
            fallback_leaf.leaf_begin < left_node.leaf_begin ||
            fallback_leaf.leaf_end > left_node.leaf_end) {
          throw std::logic_error(
              "a higher-support fallback is not a leaf of its bounded candidate cell");
        }
      }
      for (std::size_t right = left + 1U;
           right < candidate_cells.size();
           ++right) {
        const Node& right_node = node(candidate_cells[right].root_node_index);
        if (left_node.leaf_begin < right_node.leaf_end &&
            right_node.leaf_begin < left_node.leaf_end) {
          throw std::logic_error(
              "higher-support local rank roots do not form an LBVH antichain");
        }
      }
    }
    return candidate_cells;
  }

  [[nodiscard]] bool node_range_intersects_support_domain(
      const Node& query,
      const ExactHigherSupportFrontierEntry& entry) const {
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      const Node& support = node(group_node_index(entry.groups[index]));
      if (query.leaf_begin < support.leaf_end &&
          support.leaf_begin < query.leaf_end) {
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] std::size_t possible_external_witness_count(
      const ExactHigherSupportFrontierEntry& entry) const {
    std::size_t excluded = 0U;
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      const Node& support = node(group_node_index(entry.groups[index]));
      excluded = checked_add(
          excluded,
          support.leaf_end - support.leaf_begin,
          "the higher-support exclusion count overflows size_t");
    }
    if (excluded > cloud_.size()) {
      throw std::logic_error(
          "higher-support groups exclude more points than the cloud contains");
    }
    return cloud_.size() - excluded;
  }

  [[nodiscard]] RankSearchOutcome continue_rank_prune_search() {
    if (!pending_product_.has_value() || frontier_.empty() ||
        pending_product_->stage !=
            ExactHigherSupportPendingStage::rank_search ||
        pending_product_->product != frontier_.back()) {
      throw std::logic_error(
          "a higher-support rank search has no matching active product");
    }
    ExactHigherSupportPendingProduct& pending = *pending_product_;
    const ExactHigherSupportFrontierEntry& entry = pending.product;
    const std::size_t required =
        required_strict_interior_count(entry.support_size);
    if (required == 0U) {
      return RankSearchOutcome::prune;
    }
    if (required > higher_support_local_rank_probe_maximum_threshold) {
      throw std::logic_error(
          "a higher-support local rank threshold exceeds nine witnesses");
    }
    if (possible_external_witness_count(entry) < required) {
      return RankSearchOutcome::keep;
    }
    if (!pending.rank_frontier.empty()) {
      throw std::logic_error(
          "the bounded higher-support local probe cannot resume a rank DFS");
    }
    if (!pending.rank_search_started) {
      pending.rank_search_started = true;
      pending.rank_probe_next_candidate_index = 0U;
      pending.rank_probe_next_fallback_leaf_index = 0U;
      pending.rank_probe_fallback_active = false;
      std::size_t center_path_node_visit_count = 0U;
      const std::optional<std::size_t> center_seed =
          representative_center_seed_leaf_position(
              entry, &center_path_node_visit_count);
      if (center_seed.has_value()) {
        pending.rank_probe_center_seed_leaf_position = checked_u64(
            *center_seed,
            "a higher-support local rank center seed does not fit uint64");
        increment(
            result_.audit.rank_local_probe_center_seed_count,
            "the higher-support local rank center-seed count overflows size_t");
      }
      result_.audit.rank_local_probe_center_path_node_visit_count =
          checked_add(
              result_.audit.rank_local_probe_center_path_node_visit_count,
              center_path_node_visit_count,
              "the higher-support local rank center-path count overflows size_t");
      increment(
          result_.audit.rank_search_count,
          "the higher-support rank-search count overflows size_t");
      increment(
          result_.audit.rank_local_probe_attempt_count,
          "the higher-support local rank attempt count overflows size_t");
    }
    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        support_boxes(entry);
    const std::span<const spatial::ExactDyadicAabb3> support_box_span{
        boxes.data(), entry.support_size};
    std::optional<std::size_t> center_seed;
    if (pending.rank_probe_center_seed_leaf_position.has_value()) {
      center_seed = checked_size(
          *pending.rank_probe_center_seed_leaf_position,
          "a higher-support local rank center seed does not fit size_t");
    }
    const std::vector<LocalRankProbeCell> candidate_cells =
        local_rank_probe_candidate_cells(entry, center_seed);
    std::size_t maximum_evaluation_count = candidate_cells.size();
    for (const LocalRankProbeCell& candidate : candidate_cells) {
      maximum_evaluation_count = checked_add(
          maximum_evaluation_count,
          candidate.fallback_leaf_node_indices.size(),
          "the higher-support local rank plan size overflows size_t");
    }
    if (maximum_evaluation_count >
        higher_support_local_rank_probe_maximum_evaluation_count) {
      throw std::logic_error(
          "a higher-support local rank plan exceeded its fixed evaluation bound");
    }
    result_.audit.maximum_rank_local_probe_candidate_count = std::max(
        result_.audit.maximum_rank_local_probe_candidate_count,
        maximum_evaluation_count);
    if (pending.rank_probe_next_candidate_index >
        candidate_cells.size() ||
        (pending.rank_probe_fallback_active &&
         (pending.rank_probe_next_candidate_index == 0U ||
          pending.rank_probe_next_candidate_index >
              candidate_cells.size() ||
          pending.rank_probe_next_fallback_leaf_index >=
              candidate_cells[
                  pending.rank_probe_next_candidate_index - 1U]
                  .fallback_leaf_node_indices.size())) ||
        (!pending.rank_probe_fallback_active &&
         pending.rank_probe_next_fallback_leaf_index != 0U)) {
      throw std::logic_error(
          "a higher-support local rank cursor exceeds its candidate list");
    }
    if (positive_decision_source_ != nullptr &&
        positive_decision_source_->maximum_prefetch_task_count() != 0U) {
      std::vector<ExactHigherSupportQueryDecisionRequest> requests;
      requests.reserve(maximum_evaluation_count);
      const auto append_query =
          [this, &entry, &requests](std::size_t query_node_index) {
            requests.push_back(ExactHigherSupportQueryDecisionRequest{
                entry, make_node_receipt(query_node_index)});
          };
      std::size_t next_candidate_index =
          pending.rank_probe_next_candidate_index;
      if (pending.rank_probe_fallback_active) {
        const LocalRankProbeCell& active = candidate_cells[
            pending.rank_probe_next_candidate_index - 1U];
        for (std::size_t fallback_index =
                 pending.rank_probe_next_fallback_leaf_index;
             fallback_index < active.fallback_leaf_node_indices.size();
             ++fallback_index) {
          append_query(active.fallback_leaf_node_indices[fallback_index]);
        }
      }
      for (; next_candidate_index < candidate_cells.size();
           ++next_candidate_index) {
        const LocalRankProbeCell& candidate =
            candidate_cells[next_candidate_index];
        append_query(candidate.root_node_index);
        for (const std::size_t fallback_leaf_node_index :
             candidate.fallback_leaf_node_indices) {
          append_query(fallback_leaf_node_index);
        }
      }
      // This is the complete remaining fixed-bound local Morton witness plan,
      // never a scan of all points or all support tuples.  Negative proposals
      // stay non-authoritative and are replayed by the CPU below.
      positive_decision_source_->prefetch_query_strictly_inside(requests);
    }
    while (pending.rank_probe_fallback_active ||
           pending.rank_probe_next_candidate_index <
               candidate_cells.size()) {
      if (!consume_work_unit()) {
        return RankSearchOutcome::budget_exhausted;
      }
      const bool evaluating_fallback =
          pending.rank_probe_fallback_active;
      const std::size_t candidate_index = evaluating_fallback
          ? pending.rank_probe_next_candidate_index - 1U
          : pending.rank_probe_next_candidate_index;
      const LocalRankProbeCell& candidate =
          candidate_cells[candidate_index];
      std::size_t query_node_index = candidate.root_node_index;
      if (evaluating_fallback) {
        query_node_index = candidate.fallback_leaf_node_indices[
            pending.rank_probe_next_fallback_leaf_index];
        ++pending.rank_probe_next_fallback_leaf_index;
        if (pending.rank_probe_next_fallback_leaf_index ==
            candidate.fallback_leaf_node_indices.size()) {
          pending.rank_probe_next_fallback_leaf_index = 0U;
          pending.rank_probe_fallback_active = false;
        }
      } else {
        ++pending.rank_probe_next_candidate_index;
      }
      const Node& query = node(query_node_index);
      if (query.leaf_end - query.leaf_begin > required ||
          node_range_intersects_support_domain(query, entry)) {
        throw std::logic_error(
            "a higher-support local rank cursor left its bounded external LBVH cell");
      }
      increment(
          result_.audit.rank_witness_node_visit_count,
          "the higher-support rank witness count overflows size_t");
      increment(
          result_.audit.rank_local_probe_candidate_evaluation_count,
          "the higher-support local rank candidate count overflows size_t");

      const ExactHigherSupportProductQueryCellDecision query_cell_decision =
          positive_decision_source_ != nullptr &&
                  positive_decision_source_->
                      certifies_query_strictly_inside(
                          entry, make_node_receipt(query_node_index)) &&
                  positive_decision_source_->native_exact_authority()
              ? ExactHigherSupportProductQueryCellDecision::
                    strictly_inside_every_independent_sphere
              : exact_higher_support_product_query_cell_decision(
                    support_box_span,
                    node_box(query_node_index));
      increment(
          result_.audit.exact_product_analysis_count,
          "the higher-support product-analysis count overflows size_t");
      if (query_cell_decision ==
          ExactHigherSupportProductQueryCellDecision::
              strictly_inside_every_independent_sphere) {
        const std::size_t point_count =
            query.leaf_end - query.leaf_begin;
        pending.certified_strict_interior_point_count += point_count;
        pending.strict_interior_receipts.push_back(
            make_node_receipt(query_node_index));
        if (pending.certified_strict_interior_point_count >=
            exact::BigInt{required}) {
          pending.rank_probe_next_fallback_leaf_index = 0U;
          pending.rank_probe_fallback_active = false;
          increment(
              result_.audit.rank_local_probe_pruned_product_count,
              "the higher-support local rank prune count overflows size_t");
          return RankSearchOutcome::prune;
        }
        continue;
      }
      if (query_cell_decision ==
          ExactHigherSupportProductQueryCellDecision::
              outside_or_boundary_every_independent_sphere) {
        // The exact lower power endpoint is nonnegative throughout this
        // Morton cell and throughout the support product.  Equality supplies
        // no strict rank witness, so the complete query subtree can be
        // skipped without making any decision about the support product.
        increment(
            result_.audit.rank_query_outside_or_boundary_node_count,
            "the higher-support outside-or-boundary node count overflows size_t");
        result_.audit.rank_query_outside_or_boundary_point_count =
            checked_add(
                result_.audit.rank_query_outside_or_boundary_point_count,
                query.leaf_end - query.leaf_begin,
                "the higher-support outside-or-boundary point count overflows size_t");
        continue;
      }
      if (!evaluating_fallback && !query.is_leaf()) {
        pending.rank_probe_next_fallback_leaf_index = 0U;
        pending.rank_probe_fallback_active = true;
      }
    }
    increment(
        result_.audit.rank_local_probe_fail_open_product_count,
        "the higher-support local rank fail-open count overflows size_t");
    return RankSearchOutcome::keep;
  }

  [[nodiscard]] bool emit_prune_certificate(
      const ExactHigherSupportFrontierEntry& entry,
      ExactHigherSupportPruneReason reason,
      ExactHigherSupportProductAabbAnalysis support_analysis,
      RankSearchResult rank_result) {
    if (consumed_since(
            result_.audit.emitted_record_count,
            chunk_audit_origin_.emitted_record_count,
            "the higher-support record audit moved backwards") >=
        result_.budget.maximum_emitted_record_count) {
      stop(ExactHigherSupportStopReason::emitted_record_limit);
      return false;
    }
    const std::size_t emitted_receipts_in_chunk = consumed_since(
        result_.audit.emitted_rank_receipt_count,
        chunk_audit_origin_.emitted_rank_receipt_count,
        "the higher-support receipt audit moved backwards");
    if (!can_add_within(
            emitted_receipts_in_chunk,
            rank_result.receipts.size(),
            result_.budget.maximum_prune_receipt_count)) {
      stop(ExactHigherSupportStopReason::prune_receipt_limit);
      return false;
    }
    const exact::BigInt coverage = entry_support_count(entry);
    ExactHigherSupportPruneCertificate certificate;
    certificate.product = entry;
    certificate.reason = reason;
    certificate.pruned_support_count = coverage;
    certificate.support_product_analysis = std::move(support_analysis);
    certificate.required_strict_interior_point_count =
        reason == ExactHigherSupportPruneReason::strict_interior_rank_bound
            ? required_strict_interior_count(entry.support_size)
            : 0U;
    certificate.certified_strict_interior_point_count =
        std::move(rank_result.certified_point_count);
    certificate.rank_receipts = std::move(rank_result.receipts);

    if (reason == ExactHigherSupportPruneReason::no_well_centered_support) {
      if (!certificate.support_product_analysis
               .no_well_centered_support_certified() ||
          !certificate.rank_receipts.empty() ||
          certificate.certified_strict_interior_point_count != 0) {
        throw std::logic_error(
            "an invalid well-centring prune certificate was prepared");
      }
      result_.audit.well_centering_pruned_support_count += coverage;
    } else {
      if (certificate.support_product_analysis
              .no_well_centered_support_certified() ||
          certificate.certified_strict_interior_point_count <
              exact::BigInt{
                  certificate.required_strict_interior_point_count}) {
        throw std::logic_error(
            "an invalid rank prune certificate was prepared");
      }
      exact::BigInt receipt_point_sum{0};
      std::vector<std::pair<std::uint64_t, std::uint64_t>>
          receipt_intervals;
      receipt_intervals.reserve(certificate.rank_receipts.size());
      for (const ExactHigherSupportRankReceipt& receipt :
           certificate.rank_receipts) {
        const std::size_t query_node_index =
            receipt_node_index(receipt.query_node);
        const Node& query = node(query_node_index);
        const ExactHigherSupportProductAabbAnalysis& query_analysis =
            receipt.query_product_analysis;
        if (node_range_intersects_support_domain(query, entry) ||
            receipt.certified_point_count !=
                query.leaf_end - query.leaf_begin ||
            query_analysis.support_size !=
                certificate.support_product_analysis.support_size ||
            query_analysis.gram_determinant !=
                certificate.support_product_analysis.gram_determinant ||
            query_analysis.barycentric_numerators !=
                certificate.support_product_analysis
                    .barycentric_numerators ||
            !query_analysis
                 .query_strictly_inside_every_independent_sphere_certified()) {
          throw std::logic_error(
              "a higher-support rank receipt is not a coherent strict-interior certificate");
        }
        receipt_point_sum += receipt.certified_point_count;
        receipt_intervals.emplace_back(
            receipt.query_node.leaf_begin,
            receipt.query_node.leaf_end);
      }
      std::sort(receipt_intervals.begin(), receipt_intervals.end());
      for (std::size_t index = 1U; index < receipt_intervals.size(); ++index) {
        if (receipt_intervals[index].first <
            receipt_intervals[index - 1U].second) {
          throw std::logic_error(
              "higher-support rank receipts do not form a disjoint antichain");
        }
      }
      if (receipt_point_sum !=
          certificate.certified_strict_interior_point_count) {
        throw std::logic_error(
            "higher-support rank receipts contradict their certified cardinality");
      }
      result_.audit.rank_pruned_support_count += coverage;
    }
    result_.audit.resolved_support_count += coverage;
    result_.audit.emitted_rank_receipt_count = checked_add(
        result_.audit.emitted_rank_receipt_count,
        certificate.rank_receipts.size(),
        "the higher-support emitted receipt count overflows size_t");
    increment(
        result_.audit.emitted_prune_certificate_count,
        "the higher-support prune-certificate count overflows size_t");
    increment(
        result_.audit.emitted_record_count,
        "the higher-support emitted-record count overflows size_t");
    result_.prune_certificates.push_back(std::move(certificate));
    append_output_record(
        result_.prune_certificates.back(),
        ExactHigherSupportStreamChunk::RecordKind::prune_certificate);
    frontier_.pop_back();
    return true;
  }

  [[nodiscard]] RankSearchResult rank_result_from_pending_receipts(
      const ExactHigherSupportFrontierEntry& entry) const {
    if (!pending_product_.has_value() ||
        pending_product_->product != entry) {
      throw std::logic_error(
          "a higher-support rank prune omitted its active cursor");
    }
    RankSearchResult result;
    result.outcome = RankSearchOutcome::prune;
    result.certified_point_count =
        pending_product_->certified_strict_interior_point_count;
    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        support_boxes(entry);
    const std::span<const spatial::ExactDyadicAabb3> support_box_span{
        boxes.data(), entry.support_size};
    exact::BigInt recomputed_count{0};
    result.receipts.reserve(
        pending_product_->strict_interior_receipts.size());
    for (const ExactHigherSupportNodeReceipt& node_receipt :
         pending_product_->strict_interior_receipts) {
      const std::size_t query_node_index =
          receipt_node_index(node_receipt);
      const Node& query = node(query_node_index);
      const std::size_t point_count =
          query.leaf_end - query.leaf_begin;
      ExactHigherSupportProductAabbAnalysis analysis =
          exact_higher_support_product_aabb_analysis(
              support_box_span, node_box(query_node_index));
      if (node_range_intersects_support_domain(query, entry) ||
          !analysis
               .query_strictly_inside_every_independent_sphere_certified()) {
        throw std::logic_error(
            "a persisted higher-support receipt is not a strict-interior certificate");
      }
      recomputed_count += point_count;
      result.receipts.push_back(ExactHigherSupportRankReceipt{
          node_receipt, point_count, std::move(analysis)});
    }
    if (recomputed_count != result.certified_point_count) {
      throw std::logic_error(
          "persisted higher-support receipts contradict their exact count");
    }
    return result;
  }

  [[nodiscard]] bool expand_product(
      const ExactHigherSupportFrontierEntry& entry) {
    std::size_t split_group_index = entry.group_count;
    std::size_t largest_range = 0U;
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      const Node& current = node(group_node_index(entry.groups[index]));
      const std::size_t range = current.leaf_end - current.leaf_begin;
      if (!current.is_leaf() && range > largest_range) {
        split_group_index = index;
        largest_range = range;
      }
    }
    if (split_group_index == entry.group_count) {
      throw std::logic_error(
          "a nonterminal higher-support product has no splittable group");
    }
    const ExactHigherSupportNodeGroup& split_group =
        entry.groups[split_group_index];
    const Node& parent = node(group_node_index(split_group));
    const Node& left = node(parent.left_child);
    const Node& right = node(parent.right_child);
    const std::size_t multiplicity = split_group.multiplicity;
    const std::size_t left_size = left.leaf_end - left.leaf_begin;
    const std::size_t right_size = right.leaf_end - right.leaf_begin;
    const std::size_t minimum_left =
        multiplicity > right_size ? multiplicity - right_size : 0U;
    const std::size_t maximum_left = std::min(multiplicity, left_size);
    if (minimum_left > maximum_left) {
      throw std::logic_error(
          "a higher-support split has no feasible multiplicity distribution");
    }

    std::vector<ExactHigherSupportFrontierEntry> children;
    children.reserve(maximum_left - minimum_left + 1U);
    exact::BigInt child_coverage{0};
    for (std::size_t left_multiplicity = minimum_left;
         left_multiplicity <= maximum_left;
         ++left_multiplicity) {
      std::vector<std::pair<std::size_t, std::size_t>> groups;
      groups.reserve(4U);
      for (std::size_t index = 0U; index < entry.group_count; ++index) {
        if (index != split_group_index) {
          groups.emplace_back(
              group_node_index(entry.groups[index]),
              entry.groups[index].multiplicity);
        }
      }
      if (left_multiplicity != 0U) {
        groups.emplace_back(parent.left_child, left_multiplicity);
      }
      const std::size_t right_multiplicity =
          multiplicity - left_multiplicity;
      if (right_multiplicity != 0U) {
        groups.emplace_back(parent.right_child, right_multiplicity);
      }
      children.push_back(make_entry(entry.support_size, std::move(groups)));
      child_coverage += entry_support_count(children.back());
    }
    if (child_coverage != entry_support_count(entry)) {
      throw std::logic_error(
          "a grouped higher-support split did not partition its parent");
    }
    const std::size_t retained_frontier_size = frontier_.size() - 1U;
    if (!can_add_within(
            retained_frontier_size,
            children.size(),
            result_.budget.maximum_frontier_entry_count)) {
      stop(ExactHigherSupportStopReason::frontier_entry_limit);
      return false;
    }
    frontier_.pop_back();
    for (auto iterator = children.rbegin(); iterator != children.rend();
         ++iterator) {
      frontier_.push_back(std::move(*iterator));
    }
    increment(
        result_.audit.support_product_expansion_count,
        "the higher-support expansion count overflows size_t");
    result_.audit.generated_child_product_count = checked_add(
        result_.audit.generated_child_product_count,
        children.size(),
        "the higher-support child-product count overflows size_t");
    result_.audit.maximum_frontier_entry_count = std::max(
        result_.audit.maximum_frontier_entry_count,
        frontier_.size());
    return true;
  }

  [[nodiscard]] bool leaf_query_preflight(std::size_t support_size) {
    if (consumed_since(
            result_.audit.global_closed_ball_query_count,
            chunk_audit_origin_.global_closed_ball_query_count,
            "the higher-support query audit moved backwards") >=
        result_.budget.maximum_global_closed_ball_query_count) {
      stop(ExactHigherSupportStopReason::global_closed_ball_query_limit);
      return false;
    }
    const std::size_t classifications_in_chunk = consumed_since(
        result_.audit.point_classification_count,
        chunk_audit_origin_.point_classification_count,
        "the higher-support classification audit moved backwards");
    if (!can_add_within(
            classifications_in_chunk,
            cloud_.size(),
            result_.budget.maximum_point_classification_count)) {
      stop(ExactHigherSupportStopReason::point_classification_limit);
      return false;
    }
    if (consumed_since(
            result_.audit.emitted_record_count,
            chunk_audit_origin_.emitted_record_count,
            "the higher-support record audit moved backwards") >=
        result_.budget.maximum_emitted_record_count) {
      stop(ExactHigherSupportStopReason::emitted_record_limit);
      return false;
    }
    const std::size_t maximum_rank =
        result_.requirements.maximum_relevant_closed_rank;
    if (support_size > maximum_rank) {
      throw std::logic_error(
          "an intrinsically above-rank support reached a leaf query");
    }
    const std::size_t maximum_references = checked_add(
        checked_add(
            support_size,
            maximum_rank - support_size,
            "the higher-support leaf reference bound overflows size_t"),
        1U,
        "the higher-support leaf reference bound overflows size_t");
    const std::size_t references_in_chunk = consumed_since(
        result_.audit.emitted_point_id_reference_count,
        chunk_audit_origin_.emitted_point_id_reference_count,
        "the higher-support reference audit moved backwards");
    if (!can_add_within(
            references_in_chunk,
            maximum_references,
            result_.budget.maximum_emitted_point_id_reference_count)) {
      stop(ExactHigherSupportStopReason::emitted_point_id_reference_limit);
      return false;
    }
    return auxiliary_frontier_preflight();
  }

  void add_point_classifications(std::size_t count) {
    result_.audit.point_classification_count = checked_add(
        result_.audit.point_classification_count,
        count,
        "the higher-support point-classification count overflows size_t");
    if (consumed_since(
            result_.audit.point_classification_count,
            chunk_audit_origin_.point_classification_count,
            "the higher-support classification audit moved backwards") >
        result_.budget.maximum_point_classification_count) {
      throw std::logic_error(
          "a higher-support atomic leaf query exceeded its preflight budget");
    }
  }

  [[nodiscard]] SparseBallClassification classify_sparse_closed_ball(
      const std::array<PointId, 4>& support_ids,
      std::size_t support_size,
      const exact::ExactCenter3& center,
      const exact::ExactLevel& squared_level) {
    SparseBallClassification classification;
    const std::size_t interior_cap =
        result_.requirements.maximum_relevant_closed_rank - support_size;
    classification.interior_ids.reserve(interior_cap);
    increment(
        result_.audit.global_closed_ball_query_count,
        "the higher-support closed-ball query count overflows size_t");
    std::vector<std::size_t> frontier{index_.root_index_};
    result_.audit.maximum_closed_ball_frontier_entry_count = std::max(
        result_.audit.maximum_closed_ball_frontier_entry_count,
        frontier.size());
    std::array<bool, 4> support_seen{};
    std::size_t interior_count = 0U;
    while (!frontier.empty()) {
      const std::size_t node_index = frontier.back();
      frontier.pop_back();
      const Node& current = node(node_index);
      const std::size_t subtree_size =
          current.leaf_end - current.leaf_begin;
      increment(
          result_.audit.closed_ball_node_visit_count,
          "the higher-support closed-ball node count overflows size_t");
      const exact::ExactLevel minimum_distance =
          index_.minimum_squared_distance_to_node(
              cloud_, node_index, center);
      if (minimum_distance > squared_level) {
        classification.exterior_count = checked_add(
            classification.exterior_count,
            subtree_size,
            "the higher-support exterior count overflows size_t");
        increment(
            result_.audit.closed_ball_bulk_exterior_subtree_count,
            "the higher-support exterior-subtree count overflows size_t");
        add_point_classifications(subtree_size);
        continue;
      }
      const exact::ExactLevel maximum_distance =
          index_.maximum_squared_distance_to_node(
              cloud_, node_index, center);
      if (maximum_distance < squared_level) {
        interior_count = checked_add(
            interior_count,
            subtree_size,
            "the higher-support interior count overflows size_t");
        increment(
            result_.audit.closed_ball_bulk_interior_subtree_count,
            "the higher-support interior-subtree count overflows size_t");
        add_point_classifications(subtree_size);
        if (interior_count > interior_cap) {
          classification.outcome = SparseBallOutcome::rank_exceeded;
          return classification;
        }
        for (std::size_t position = current.leaf_begin;
             position < current.leaf_end;
             ++position) {
          classification.interior_ids.push_back(
              index_.leaves_[position].point_id);
        }
        continue;
      }
      if (current.is_leaf()) {
        const PointId point_id =
            index_.leaves_[current.leaf_begin].point_id;
        const exact::SpherePointClassification point_classification =
            exact::classify_sphere_point(
                center, squared_level, cloud_.point(point_id));
        increment(
            result_.audit.exact_point_distance_evaluation_count,
            "the higher-support exact-distance count overflows size_t");
        add_point_classifications(1U);
        switch (point_classification.location()) {
          case exact::SpherePointLocation::strictly_inside:
            increment(
                interior_count,
                "the higher-support interior count overflows size_t");
            if (interior_count > interior_cap) {
              classification.outcome = SparseBallOutcome::rank_exceeded;
              return classification;
            }
            classification.interior_ids.push_back(point_id);
            break;
          case exact::SpherePointLocation::boundary: {
            increment(
                classification.shell_count,
                "the higher-support shell count overflows size_t");
            bool is_support = false;
            for (std::size_t support_index = 0U;
                 support_index < support_size;
                 ++support_index) {
              if (point_id == support_ids[support_index]) {
                support_seen[support_index] = true;
                is_support = true;
                break;
              }
            }
            if (!is_support &&
                (!classification.canonical_extra_shell_witness_id.has_value() ||
                 point_id <
                     *classification.canonical_extra_shell_witness_id)) {
              classification.canonical_extra_shell_witness_id = point_id;
            }
            break;
          }
          case exact::SpherePointLocation::outside:
            increment(
                classification.exterior_count,
                "the higher-support exterior count overflows size_t");
            break;
        }
        continue;
      }
      frontier.push_back(current.right_child);
      frontier.push_back(current.left_child);
      if (frontier.size() >
          result_.budget.maximum_auxiliary_frontier_entry_count) {
        throw std::logic_error(
            "a higher-support closed-ball DFS exceeded its preflight bound");
      }
      result_.audit.maximum_closed_ball_frontier_entry_count = std::max(
          result_.audit.maximum_closed_ball_frontier_entry_count,
          frontier.size());
    }
    const std::size_t classified_count = checked_add(
        checked_add(
            interior_count,
            classification.shell_count,
            "the higher-support partition count overflows size_t"),
        classification.exterior_count,
        "the higher-support partition count overflows size_t");
    bool every_support_seen = true;
    for (std::size_t index = 0U; index < support_size; ++index) {
      every_support_seen = every_support_seen && support_seen[index];
    }
    if (classified_count != cloud_.size() || !every_support_seen ||
        classification.shell_count < support_size ||
        (classification.shell_count == support_size) !=
            !classification.canonical_extra_shell_witness_id.has_value() ||
        interior_count != classification.interior_ids.size()) {
      throw std::logic_error(
          "a higher-support sparse closed-ball traversal did not close its partition");
    }
    std::sort(
        classification.interior_ids.begin(),
        classification.interior_ids.end());
    classification.outcome = SparseBallOutcome::complete;
    return classification;
  }

  void resolve_leaf() {
    result_.audit.leaf_classified_support_count += 1;
    result_.audit.resolved_support_count += 1;
    frontier_.pop_back();
  }

  template <std::size_t SupportSize>
  [[nodiscard]] bool classify_terminal_support(
      const ExactHigherSupportFrontierEntry& entry,
      const std::array<PointId, 4>& support_ids) {
    if (!pending_product_.has_value() ||
        pending_product_->stage !=
            ExactHigherSupportPendingStage::classify_leaf ||
        pending_product_->product != entry) {
      throw std::logic_error(
          "a higher-support leaf classification has no active cursor");
    }
    std::array<exact::ExactRational3, SupportSize> support_points{};
    for (std::size_t index = 0U; index < SupportSize; ++index) {
      support_points[index] = cloud_.point(support_ids[index]).exact();
    }
    const exact::CircumcenterSupportAnalysis analysis =
        exact::analyze_circumcenter_support(support_points);
    ExactHigherSupportTerminalGeometryDecision analysis_decision{};
    switch (analysis.status()) {
      case exact::CircumcenterSupportStatus::affinely_dependent:
        analysis_decision =
            ExactHigherSupportTerminalGeometryDecision::affinely_dependent;
        break;
      case exact::CircumcenterSupportStatus::boundary_reduced:
        analysis_decision =
            ExactHigherSupportTerminalGeometryDecision::boundary_reduced;
        break;
      case exact::CircumcenterSupportStatus::exterior_circumcenter:
        analysis_decision = ExactHigherSupportTerminalGeometryDecision::
            exterior_circumcenter;
        break;
      case exact::CircumcenterSupportStatus::minimal:
        analysis_decision =
            ExactHigherSupportTerminalGeometryDecision::minimal;
        break;
    }
    if (terminal_geometry_decision(entry) != analysis_decision) {
      throw std::logic_error(
          "the terminal support geometry gate disagreed with exact leaf "
          "analysis");
    }
    const bool first_analysis =
        !pending_product_->leaf_analysis_started;
    if (first_analysis) {
      pending_product_->leaf_analysis_started = true;
      increment(
          result_.audit.leaf_support_analysis_count,
          "the higher-support leaf-analysis count overflows size_t");
    }
    switch (analysis.status()) {
      case exact::CircumcenterSupportStatus::affinely_dependent:
        if (!first_analysis) {
          throw std::logic_error(
              "a persisted higher-support minimal leaf became dependent");
        }
        increment(
            result_.audit.affinely_dependent_leaf_count,
            "the higher-support dependent-leaf count overflows size_t");
        resolve_leaf();
        return true;
      case exact::CircumcenterSupportStatus::boundary_reduced:
        if (!first_analysis) {
          throw std::logic_error(
              "a persisted higher-support minimal leaf became boundary-reduced");
        }
        increment(
            result_.audit.boundary_reduced_leaf_count,
            "the higher-support boundary-leaf count overflows size_t");
        resolve_leaf();
        return true;
      case exact::CircumcenterSupportStatus::exterior_circumcenter:
        if (!first_analysis) {
          throw std::logic_error(
              "a persisted higher-support minimal leaf became exterior");
        }
        increment(
            result_.audit.exterior_circumcenter_leaf_count,
            "the higher-support exterior-centre count overflows size_t");
        resolve_leaf();
        return true;
      case exact::CircumcenterSupportStatus::minimal:
        if (first_analysis) {
          increment(
              result_.audit.minimal_leaf_count,
              "the higher-support minimal-leaf count overflows size_t");
        }
        break;
    }
    const exact::CircumcenterResult& sphere =
        analysis.circumcenter_result();
    if (!sphere.center().has_value() ||
        !sphere.squared_level().has_value()) {
      throw std::logic_error(
          "a minimal higher support omitted its exact sphere");
    }
    if (!leaf_query_preflight(SupportSize)) {
      return false;
    }
    SparseBallClassification classification = classify_sparse_closed_ball(
        support_ids,
        SupportSize,
        *sphere.center(),
        *sphere.squared_level());
    if (classification.outcome == SparseBallOutcome::rank_exceeded) {
      increment(
          result_.audit.above_rank_leaf_count,
          "the higher-support above-rank count overflows size_t");
      resolve_leaf();
      return true;
    }
    const std::size_t observed_closed_rank = checked_add(
        classification.interior_ids.size(),
        classification.shell_count,
        "the higher-support observed rank overflows size_t");
    const std::size_t minimum_possible_closed_rank = checked_add(
        classification.interior_ids.size(),
        SupportSize,
        "the higher-support relevance rank overflows size_t");
    if (minimum_possible_closed_rank >
        result_.requirements.maximum_relevant_closed_rank) {
      throw std::logic_error(
          "a complete higher-support shell escaped its interior cap");
    }
    std::size_t emitted_references = checked_add(
        SupportSize,
        classification.interior_ids.size(),
        "the higher-support emitted-reference count overflows size_t");
    if (classification.shell_count == SupportSize) {
      ExactHigherSupportEvent event;
      event.support_size = static_cast<std::uint8_t>(SupportSize);
      event.support_ids = support_ids;
      event.center = *sphere.center();
      event.squared_level = *sphere.squared_level();
      event.interior_ids = std::move(classification.interior_ids);
      event.closed_rank = observed_closed_rank;
      event.exterior_count = classification.exterior_count;
      result_.events.push_back(std::move(event));
      append_output_record(
          result_.events.back(),
          ExactHigherSupportStreamChunk::RecordKind::event);
      increment(
          result_.audit.accepted_event_count,
          "the higher-support accepted-event count overflows size_t");
    } else {
      if (!classification.canonical_extra_shell_witness_id.has_value()) {
        throw std::logic_error(
            "a higher-support extra shell omitted its canonical witness");
      }
      emitted_references = checked_add(
          emitted_references,
          1U,
          "the higher-support diagnostic reference count overflows size_t");
      ExactHigherSupportExtraShellDiagnostic diagnostic;
      diagnostic.support_size = static_cast<std::uint8_t>(SupportSize);
      diagnostic.support_ids = support_ids;
      diagnostic.center = *sphere.center();
      diagnostic.squared_level = *sphere.squared_level();
      diagnostic.interior_ids = std::move(classification.interior_ids);
      diagnostic.shell_count = classification.shell_count;
      diagnostic.canonical_extra_shell_witness_id =
          *classification.canonical_extra_shell_witness_id;
      diagnostic.minimum_possible_closed_rank =
          minimum_possible_closed_rank;
      diagnostic.observed_closed_rank = observed_closed_rank;
      diagnostic.exterior_count = classification.exterior_count;
      result_.relevant_extra_shell_diagnostics.push_back(
          std::move(diagnostic));
      append_output_record(
          result_.relevant_extra_shell_diagnostics.back(),
          ExactHigherSupportStreamChunk::RecordKind::
              relevant_extra_shell_diagnostic);
      increment(
          result_.audit.relevant_extra_shell_diagnostic_count,
          "the higher-support diagnostic count overflows size_t");
    }
    if (!can_add_within(
            consumed_since(
                result_.audit.emitted_point_id_reference_count,
                chunk_audit_origin_.emitted_point_id_reference_count,
                "the higher-support reference audit moved backwards"),
            emitted_references,
            result_.budget.maximum_emitted_point_id_reference_count)) {
      throw std::logic_error(
          "a higher-support leaf exceeded its reference preflight");
    }
    result_.audit.emitted_point_id_reference_count = checked_add(
        result_.audit.emitted_point_id_reference_count,
        emitted_references,
        "the higher-support emitted-reference count overflows size_t");
    increment(
        result_.audit.emitted_record_count,
        "the higher-support emitted-record count overflows size_t");
    resolve_leaf();
    static_cast<void>(entry);
    return true;
  }

  [[nodiscard]] bool classify_terminal(
      const ExactHigherSupportFrontierEntry& entry) {
    const std::array<PointId, 4> support_ids =
        terminal_support_ids(entry);
    if (entry.support_size == 3U) {
      return classify_terminal_support<3U>(entry, support_ids);
    } else if (entry.support_size == 4U) {
      return classify_terminal_support<4U>(entry, support_ids);
    } else {
      throw std::logic_error(
          "a terminal higher-support product has an invalid arity");
    }
  }

  void visit_frontier_back() {
    if (frontier_.empty() || pending_product_.has_value()) {
      throw std::logic_error(
          "a higher-support product visit requires one unclaimed frontier back");
    }
    if (!consume_work_unit()) {
      return;
    }
    const ExactHigherSupportFrontierEntry entry = frontier_.back();
    increment(
        result_.audit.support_product_visit_count,
        "the higher-support product-visit count overflows size_t");
    pending_product_.emplace();
    pending_product_->product = entry;
    // Every product, including a singleton terminal support, must first pass
    // the same exact well-centring and strict-interior rank decisions.  A
    // terminal closed-ball classification is only the fail-open continuation
    // after both block prunes failed.
    pending_product_->stage =
        ExactHigherSupportPendingStage::analyze_product;
    continue_pending_product();
  }

  [[nodiscard]] ExactHigherSupportProductAabbAnalysis
  recompute_support_analysis(
      const ExactHigherSupportFrontierEntry& entry) const {
    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        support_boxes(entry);
    return exact_higher_support_product_aabb_analysis(
        std::span<const spatial::ExactDyadicAabb3>{
            boxes.data(), entry.support_size});
  }

  [[nodiscard]] ExactHigherSupportTerminalGeometryDecision
  terminal_geometry_decision(
      const ExactHigherSupportFrontierEntry& entry) const {
    if (!is_terminal(entry)) {
      throw std::logic_error(
          "a categorical higher-support geometry decision requires a "
          "terminal product");
    }
    if (terminal_geometry_cache_.has_value() &&
        terminal_geometry_cache_->first == entry) {
      return terminal_geometry_cache_->second;
    }

    std::optional<ExactHigherSupportTerminalGeometryDecision> proposal;
    if (terminal_geometry_decision_source_ != nullptr) {
      proposal = terminal_geometry_decision_source_->decide(entry);
      if (proposal.has_value() &&
          terminal_geometry_decision_source_->native_exact_authority()) {
        terminal_geometry_cache_ = std::pair{entry, *proposal};
        return *proposal;
      }
    }
    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        support_boxes(entry);
    const ExactHigherSupportTerminalGeometryDecision cpu_decision =
        exact_higher_support_terminal_geometry_decision(
            std::span<const spatial::ExactDyadicAabb3>{
                boxes.data(), entry.support_size});

    // Missing, disabled and non-native proposals have no scientific
    // authority.  In particular, a well-formed but hostile host-fake category
    // is ignored after exact CPU replay.
    static_cast<void>(proposal);
    terminal_geometry_cache_ = std::pair{entry, cpu_decision};
    return cpu_decision;
  }

  [[nodiscard]] bool no_well_centered_support_certified(
      const ExactHigherSupportFrontierEntry& entry) const {
    if (is_terminal(entry)) {
      return terminal_geometry_decision(entry) !=
          ExactHigherSupportTerminalGeometryDecision::minimal;
    }
    if (positive_decision_source_ != nullptr &&
        positive_decision_source_->certifies_no_well_centered_support(
            entry) &&
        positive_decision_source_->native_exact_authority()) {
      return true;
    }
    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        support_boxes(entry);
    return exact_higher_support_product_no_well_centered_certified(
        std::span<const spatial::ExactDyadicAabb3>{
            boxes.data(), entry.support_size});
  }

  [[nodiscard]] bool all_well_centered_support_certified(
      const ExactHigherSupportFrontierEntry& entry) const {
    if (is_terminal(entry)) {
      return terminal_geometry_decision(entry) ==
          ExactHigherSupportTerminalGeometryDecision::minimal;
    }
    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        support_boxes(entry);
    return exact_higher_support_product_all_well_centered_certified(
        std::span<const spatial::ExactDyadicAabb3>{
            boxes.data(), entry.support_size});
  }

  void clear_pending_rank_state() {
    if (!pending_product_.has_value()) {
      throw std::logic_error(
          "a higher-support rank cursor cannot be cleared while absent");
    }
    pending_product_->rank_search_started = false;
    pending_product_->rank_frontier.clear();
    pending_product_->rank_probe_next_candidate_index = 0U;
    pending_product_->rank_probe_next_fallback_leaf_index = 0U;
    pending_product_->rank_probe_fallback_active = false;
    pending_product_->rank_probe_center_seed_leaf_position.reset();
    pending_product_->strict_interior_receipts.clear();
    pending_product_->certified_strict_interior_point_count = 0;
  }

  void continue_pending_product() {
    if (!pending_product_.has_value() || frontier_.empty() ||
        pending_product_->product != frontier_.back()) {
      throw std::logic_error(
          "a higher-support pending product is detached from the frontier");
    }
    while (pending_product_.has_value() && !stopped_) {
      const ExactHigherSupportFrontierEntry entry =
          pending_product_->product;
      switch (pending_product_->stage) {
        case ExactHigherSupportPendingStage::analyze_product: {
          prefetch_sparse_morton_frontier_slice();
          const bool no_well_centered =
              no_well_centered_support_certified(entry);
          increment(
              result_.audit.exact_product_analysis_count,
              "the higher-support product-analysis count overflows size_t");
          if (no_well_centered) {
            pending_product_->stage =
                ExactHigherSupportPendingStage::emit_well_prune;
          } else if (
              required_strict_interior_count(entry.support_size) == 0U) {
            pending_product_->stage =
                ExactHigherSupportPendingStage::emit_rank_prune;
          } else {
            const bool rank_probe_eligible =
                all_well_centered_support_certified(entry);
            increment(
                result_.audit.exact_product_analysis_count,
                "the higher-support product-analysis count overflows size_t");
            if (rank_probe_eligible) {
              pending_product_->stage =
                  ExactHigherSupportPendingStage::rank_search;
            } else {
              // This positive-only probe cannot certify a mixed or still
              // geometrically unresolved support box.  Splitting the support
              // product is an exact fail-open continuation: no support tuple
              // is discarded and the gate is reconsidered on tighter cells.
              increment(
                  result_.audit.rank_local_probe_geometric_gate_skip_count,
                  "the higher-support probe-gate skip count overflows size_t");
              pending_product_->stage = is_terminal(entry)
                  ? ExactHigherSupportPendingStage::classify_leaf
                  : ExactHigherSupportPendingStage::expand_product;
            }
          }
          continue;
        }
        case ExactHigherSupportPendingStage::emit_well_prune: {
          ExactHigherSupportProductAabbAnalysis support_analysis =
              recompute_support_analysis(entry);
          if (emit_prune_certificate(
                  entry,
                  ExactHigherSupportPruneReason::no_well_centered_support,
                  std::move(support_analysis),
                  RankSearchResult{})) {
            pending_product_.reset();
          }
          return;
        }
        case ExactHigherSupportPendingStage::rank_search: {
          const RankSearchOutcome outcome = continue_rank_prune_search();
          if (outcome == RankSearchOutcome::budget_exhausted) {
            return;
          }
          if (outcome == RankSearchOutcome::prune) {
            pending_product_->stage =
                ExactHigherSupportPendingStage::emit_rank_prune;
          } else {
            clear_pending_rank_state();
            pending_product_->stage = is_terminal(entry)
                ? ExactHigherSupportPendingStage::classify_leaf
                : ExactHigherSupportPendingStage::expand_product;
          }
          continue;
        }
        case ExactHigherSupportPendingStage::emit_rank_prune: {
          ExactHigherSupportProductAabbAnalysis support_analysis =
              recompute_support_analysis(entry);
          RankSearchResult rank_result =
              rank_result_from_pending_receipts(entry);
          if (emit_prune_certificate(
                  entry,
                  ExactHigherSupportPruneReason::strict_interior_rank_bound,
                  std::move(support_analysis),
                  std::move(rank_result))) {
            pending_product_.reset();
          }
          return;
        }
        case ExactHigherSupportPendingStage::expand_product:
          if (expand_product(entry)) {
            pending_product_.reset();
          }
          return;
        case ExactHigherSupportPendingStage::classify_leaf:
          if (classify_terminal(entry)) {
            pending_product_.reset();
          }
          return;
      }
      throw std::logic_error(
          "a higher-support pending stage is invalid");
    }
  }

  [[nodiscard]] ExactHigherSupportStreamAudit audit_snapshot() const {
    ExactHigherSupportStreamAudit snapshot = result_.audit;
    snapshot.remaining_frontier_support_count = 0;
    for (const ExactHigherSupportFrontierEntry& entry : frontier_) {
      snapshot.remaining_frontier_support_count += entry_support_count(entry);
    }
    const exact::BigInt terminal_sum =
        snapshot.well_centering_pruned_support_count +
        snapshot.rank_pruned_support_count +
        snapshot.leaf_classified_support_count;
    snapshot.grouped_partition_accounting_certified =
        snapshot.resolved_support_count +
                    snapshot.remaining_frontier_support_count ==
                snapshot.total_support_count &&
        snapshot.resolved_support_count == terminal_sum;
    return snapshot;
  }

  void finish_result() {
    if (canonical_sort_records_) {
      std::sort(
          result_.events.begin(),
          result_.events.end(),
          support_record_less<ExactHigherSupportEvent>);
      std::sort(
          result_.relevant_extra_shell_diagnostics.begin(),
          result_.relevant_extra_shell_diagnostics.end(),
          support_record_less<ExactHigherSupportExtraShellDiagnostic>);
    }
    result_.remaining_frontier = frontier_;
    result_.audit = audit_snapshot();
    if (!result_.audit.grouped_partition_accounting_certified) {
      throw std::logic_error(
          "the higher-support frontier does not partition its BigInt universe");
    }
    const std::size_t typed_record_count = checked_add(
        checked_add(
            result_.events.size(),
            result_.relevant_extra_shell_diagnostics.size(),
            "the higher-support typed record count overflows size_t"),
        result_.prune_certificates.size(),
        "the higher-support typed record count overflows size_t");
    const std::size_t emitted_record_delta = consumed_since(
        result_.audit.emitted_record_count,
        chunk_audit_origin_.emitted_record_count,
        "the higher-support record audit moved backwards");
    const std::size_t prune_record_delta = consumed_since(
        result_.audit.emitted_prune_certificate_count,
        chunk_audit_origin_.emitted_prune_certificate_count,
        "the higher-support prune audit moved backwards");
    if (typed_record_count != emitted_record_delta ||
        result_.prune_certificates.size() != prune_record_delta ||
        record_order_.size() != typed_record_count) {
      throw std::logic_error(
          "the higher-support emitted-record audit is inconsistent");
    }
    result_.grouped_frontier_partition_certified = true;
    result_.all_prunes_replayable = true;
    result_.all_rank_relevant_shells_complete = true;
    result_.frontier_exhausted =
        frontier_.empty() && !pending_product_.has_value();
    result_.no_forbidden_global_structure_materialized = true;
    result_.hierarchy_reduction_performed = false;
    if (frontier_.empty() && !pending_product_.has_value()) {
      result_.status = ExactHigherSupportStreamStatus::complete;
      result_.stop_reason = ExactHigherSupportStopReason::none;
    } else if (!stopped_) {
      throw std::logic_error(
          "a nonempty higher-support frontier has no budget stop reason");
    }
  }

  const spatial::MortonLbvhIndex& index_;
  const spatial::CanonicalPointCloud& cloud_;
  const ExactHigherSupportPositiveDecisionSource*
      positive_decision_source_{};
  const ExactHigherSupportTerminalGeometryDecisionSource*
      terminal_geometry_decision_source_{};
  mutable std::optional<std::pair<
      ExactHigherSupportFrontierEntry,
      ExactHigherSupportTerminalGeometryDecision>>
      terminal_geometry_cache_;
  ExactHigherSupportStreamResult result_;
  std::vector<ExactHigherSupportFrontierEntry> frontier_;
  std::optional<ExactHigherSupportPendingProduct> pending_product_;
  ExactHigherSupportStreamAudit chunk_audit_origin_{};
  contract::CanonicalId output_chain_digest_{};
  std::size_t output_record_count_{0U};
  std::vector<ExactHigherSupportStreamChunk::RecordKind> record_order_;
  bool canonical_sort_records_{false};
  bool stopped_{false};
};

ExactHigherSupportStreamResult build_exact_higher_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& budget) {
  ExactHigherSupportStreamBuilder builder{
      index, cloud, requested_maximum_order, budget};
  builder.execute();
  return builder.take_result();
}

ExactHigherSupportStreamResult build_exact_higher_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& budget,
    const ExactHigherSupportPositiveDecisionSource&
        positive_decision_source) {
  ExactHigherSupportStreamBuilder builder{
      index,
      cloud,
      requested_maximum_order,
      budget,
      &positive_decision_source};
  builder.execute();
  return builder.take_result();
}

ExactHigherSupportCheckpointManifest
make_exact_higher_support_checkpoint_manifest(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  return ExactHigherSupportStreamBuilder::manifest_for(
      index, cloud, requested_maximum_order);
}

ExactHigherSupportAuthorityContext::ExactHigherSupportAuthorityContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order)
    : index_(&index),
      cloud_(&cloud),
      requested_maximum_order_(requested_maximum_order),
      index_identity_(index.identity_),
      cloud_identity_(cloud.identity_) {
  manifest_ = ExactHigherSupportStreamBuilder::manifest_for(
      index, cloud, requested_maximum_order, &audit_);
}

ExactHigherSupportCheckpoint make_initial_exact_higher_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactHigherSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return make_initial_exact_higher_support_checkpoint(authority);
}

ExactHigherSupportCheckpoint make_initial_exact_higher_support_checkpoint(
    const ExactHigherSupportAuthorityContext& authority) {
  const std::size_t root_count =
      authority.cloud().size() >= 4U
          ? 2U
          : (authority.cloud().size() >= 3U ? 1U : 0U);
  ExactHigherSupportStreamBuilder builder{
      authority.index(),
      authority.cloud(),
      authority.requested_maximum_order(),
      ExactHigherSupportStreamBudget{
          0U,
          root_count,
          0U,
          0U,
          0U,
          0U,
          0U,
          0U}};
  return builder.snapshot_checkpoint(authority.manifest(), 0U);
}

contract::CanonicalId compute_exact_higher_support_checkpoint_digest(
    const ExactHigherSupportCheckpoint& checkpoint) {
  return checkpoint_digest(checkpoint);
}

ExactHigherSupportCheckpointVerification
verify_exact_higher_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportCheckpoint& checkpoint) {
  const ExactHigherSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return verify_exact_higher_support_checkpoint(authority, checkpoint);
}

ExactHigherSupportCheckpointVerification
verify_exact_higher_support_checkpoint(
    const ExactHigherSupportAuthorityContext& authority,
    const ExactHigherSupportCheckpoint& checkpoint) {
  return ExactHigherSupportStreamBuilder::verify_checkpoint_for(
      authority.index(),
      authority.cloud(),
      authority.requested_maximum_order(),
      authority.manifest(),
      checkpoint);
}

bool ExactHigherSupportStreamChunk::builder_invariants_hold() const {
  if (!candidate_prepared ||
      !no_forbidden_global_structure_materialized ||
      hierarchy_reduction_performed ||
      manifest != next_checkpoint.manifest ||
      cumulative_audit_after != next_checkpoint.cumulative_audit ||
      output_chain_digest != next_checkpoint.output_chain_digest ||
      source_checkpoint_digest == contract::CanonicalId{} ||
      chunk_sequence == std::numeric_limits<std::uint64_t>::max() ||
      next_checkpoint.next_chunk_sequence != chunk_sequence + 1U ||
      next_checkpoint.output_record_count < first_output_record_index ||
      next_checkpoint.output_record_count - first_output_record_index !=
          record_order.size() ||
      compute_exact_higher_support_checkpoint_digest(next_checkpoint) !=
          next_checkpoint.checkpoint_digest) {
    return false;
  }

  const auto audit_delta = [](
                               std::size_t after,
                               std::size_t before,
                               std::size_t* delta) {
    if (after < before) {
      return false;
    }
    *delta = after - before;
    return true;
  };
  std::size_t emitted_record_delta = 0U;
  std::size_t event_delta = 0U;
  std::size_t diagnostic_delta = 0U;
  std::size_t prune_delta = 0U;
  std::size_t rank_receipt_delta = 0U;
  std::size_t point_reference_delta = 0U;
  std::size_t work_delta = 0U;
  std::size_t global_query_delta = 0U;
  std::size_t point_classification_delta = 0U;
  if (!audit_delta(
          cumulative_audit_after.emitted_record_count,
          cumulative_audit_before.emitted_record_count,
          &emitted_record_delta) ||
      !audit_delta(
          cumulative_audit_after.accepted_event_count,
          cumulative_audit_before.accepted_event_count,
          &event_delta) ||
      !audit_delta(
          cumulative_audit_after.relevant_extra_shell_diagnostic_count,
          cumulative_audit_before.relevant_extra_shell_diagnostic_count,
          &diagnostic_delta) ||
      !audit_delta(
          cumulative_audit_after.emitted_prune_certificate_count,
          cumulative_audit_before.emitted_prune_certificate_count,
          &prune_delta) ||
      !audit_delta(
          cumulative_audit_after.emitted_rank_receipt_count,
          cumulative_audit_before.emitted_rank_receipt_count,
          &rank_receipt_delta) ||
      !audit_delta(
          cumulative_audit_after.emitted_point_id_reference_count,
          cumulative_audit_before.emitted_point_id_reference_count,
          &point_reference_delta) ||
      !audit_delta(
          cumulative_audit_after.work_unit_count,
          cumulative_audit_before.work_unit_count,
          &work_delta) ||
      !audit_delta(
          cumulative_audit_after.global_closed_ball_query_count,
          cumulative_audit_before.global_closed_ball_query_count,
          &global_query_delta) ||
      !audit_delta(
          cumulative_audit_after.point_classification_count,
          cumulative_audit_before.point_classification_count,
          &point_classification_delta) ||
      emitted_record_delta != record_order.size() ||
      event_delta != events.size() ||
      diagnostic_delta != relevant_extra_shell_diagnostics.size() ||
      prune_delta != prune_certificates.size() ||
      work_delta > budget.maximum_work_unit_count ||
      emitted_record_delta > budget.maximum_emitted_record_count ||
      point_reference_delta >
          budget.maximum_emitted_point_id_reference_count ||
      rank_receipt_delta > budget.maximum_prune_receipt_count ||
      global_query_delta >
          budget.maximum_global_closed_ball_query_count ||
      point_classification_delta >
          budget.maximum_point_classification_count ||
      next_checkpoint.frontier.size() >
          budget.maximum_frontier_entry_count ||
      (next_checkpoint.pending_product.has_value() &&
       next_checkpoint.pending_product->rank_frontier.size() >
           budget.maximum_auxiliary_frontier_entry_count)) {
    return false;
  }

  std::size_t event_index = 0U;
  std::size_t diagnostic_index = 0U;
  std::size_t prune_index = 0U;
  std::size_t rank_receipt_count = 0U;
  contract::CanonicalId rebuilt_output_chain =
      previous_output_chain_digest;
  for (const RecordKind kind : record_order) {
    switch (kind) {
      case RecordKind::event:
        if (event_index >= events.size()) {
          return false;
        }
        rebuilt_output_chain =
            extend_output_chain(rebuilt_output_chain, events[event_index]);
        ++event_index;
        break;
      case RecordKind::relevant_extra_shell_diagnostic:
        if (diagnostic_index >=
            relevant_extra_shell_diagnostics.size()) {
          return false;
        }
        rebuilt_output_chain = extend_output_chain(
            rebuilt_output_chain,
            relevant_extra_shell_diagnostics[diagnostic_index]);
        ++diagnostic_index;
        break;
      case RecordKind::prune_certificate:
        if (prune_index >= prune_certificates.size()) {
          return false;
        }
        if (prune_certificates[prune_index].rank_receipts.size() >
            std::numeric_limits<std::size_t>::max() -
                rank_receipt_count) {
          return false;
        }
        rank_receipt_count +=
            prune_certificates[prune_index].rank_receipts.size();
        rebuilt_output_chain = extend_output_chain(
            rebuilt_output_chain, prune_certificates[prune_index]);
        ++prune_index;
        break;
      default:
        return false;
    }
  }
  if (event_index != events.size() ||
      diagnostic_index != relevant_extra_shell_diagnostics.size() ||
      prune_index != prune_certificates.size() ||
      rank_receipt_count != rank_receipt_delta ||
      rebuilt_output_chain != output_chain_digest) {
    return false;
  }

  if (status == ExactHigherSupportStreamStatus::complete) {
    return stop_reason == ExactHigherSupportStopReason::none &&
           next_checkpoint.locally_complete();
  }
  return status == ExactHigherSupportStreamStatus::budget_exhausted &&
         stop_reason != ExactHigherSupportStopReason::none &&
         !next_checkpoint.locally_complete();
}

namespace {

[[nodiscard]] ExactHigherSupportStreamChunk
build_exact_higher_support_stream_chunk_after_source_verification(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& chunk_budget,
    const ExactHigherSupportCheckpoint& checkpoint,
    const ExactHigherSupportPositiveDecisionSource*
        positive_decision_source = nullptr) {
  ExactHigherSupportStreamChunk chunk;
  chunk.manifest = checkpoint.manifest;
  chunk.budget = chunk_budget;
  chunk.chunk_sequence = checkpoint.next_chunk_sequence;
  chunk.first_output_record_index = checkpoint.output_record_count;
  chunk.source_checkpoint_digest = checkpoint.checkpoint_digest;
  chunk.previous_output_chain_digest = checkpoint.output_chain_digest;
  chunk.cumulative_audit_before = checkpoint.cumulative_audit;
  chunk.no_forbidden_global_structure_materialized = true;
  chunk.hierarchy_reduction_performed = false;
  if (checkpoint.locally_complete()) {
    chunk.output_chain_digest = checkpoint.output_chain_digest;
    chunk.status = ExactHigherSupportStreamStatus::complete;
    chunk.stop_reason = ExactHigherSupportStopReason::none;
    chunk.cumulative_audit_after = checkpoint.cumulative_audit;
    chunk.next_checkpoint = checkpoint;
    chunk.candidate_prepared = true;
    return chunk;
  }
  if (checkpoint.next_chunk_sequence ==
      std::numeric_limits<std::uint64_t>::max()) {
    throw std::overflow_error(
        "the higher-support chunk sequence overflows uint64");
  }

  ExactHigherSupportStreamBuilder builder{
      index,
      cloud,
      requested_maximum_order,
      chunk_budget,
      checkpoint,
      IntegrityVerifiedHigherCheckpointTag{},
      positive_decision_source};
  builder.execute();
  chunk.next_checkpoint = builder.take_checkpoint(
      checkpoint.manifest, checkpoint.next_chunk_sequence + 1U);
  ExactHigherSupportStreamResult result = builder.take_result();
  chunk.output_chain_digest = builder.output_chain_digest();
  chunk.status = result.status;
  chunk.stop_reason = result.stop_reason;
  chunk.events = std::move(result.events);
  chunk.relevant_extra_shell_diagnostics =
      std::move(result.relevant_extra_shell_diagnostics);
  chunk.prune_certificates = std::move(result.prune_certificates);
  chunk.record_order = builder.take_record_order();
  chunk.cumulative_audit_after = result.audit;
  chunk.candidate_prepared = true;
  if (!chunk.builder_invariants_hold()) {
    throw std::logic_error(
        "the exact higher-support builder emitted an inconsistent chunk");
  }
  return chunk;
}

[[nodiscard]] ExactHigherSupportStreamChunkVerification
verify_exact_higher_support_stream_chunk_from_trusted_source(
    const ExactHigherSupportAuthorityContext& authority,
    const ExactHigherSupportStreamBudget& chunk_budget,
    const ExactHigherSupportCheckpoint& source_checkpoint,
    const ExactHigherSupportStreamChunk& observed,
    ExactHigherSupportCheckpoint* freshly_replayed_next) {
  ExactHigherSupportStreamChunkVerification verification;
  verification.source_checkpoint_local_integrity_verified =
      verify_exact_higher_support_checkpoint(authority, source_checkpoint)
          .local_integrity_verified;
  verification.source_checkpoint_anchored = true;
  verification.requested_budget_certified =
      observed.budget == chunk_budget;
  if (!verification.source_checkpoint_local_integrity_verified) {
    return verification;
  }
  ExactHigherSupportStreamChunk expected =
      build_exact_higher_support_stream_chunk_after_source_verification(
          authority.index(),
          authority.cloud(),
          authority.requested_maximum_order(),
          chunk_budget,
          source_checkpoint);
  verification.prepared_transition_chain_matches =
      observed.manifest == expected.manifest &&
      observed.chunk_sequence == expected.chunk_sequence &&
      observed.first_output_record_index ==
          expected.first_output_record_index &&
      observed.source_checkpoint_digest ==
          expected.source_checkpoint_digest &&
      observed.previous_output_chain_digest ==
          expected.previous_output_chain_digest &&
      observed.output_chain_digest == expected.output_chain_digest &&
      observed.candidate_prepared == expected.candidate_prepared;
  verification.records_individually_exact =
      observed.events == expected.events &&
      observed.relevant_extra_shell_diagnostics ==
          expected.relevant_extra_shell_diagnostics &&
      observed.prune_certificates == expected.prune_certificates &&
      observed.record_order == expected.record_order;
  verification.next_checkpoint_local_integrity_verified =
      observed.next_checkpoint == expected.next_checkpoint &&
      verify_exact_higher_support_checkpoint(
          authority, observed.next_checkpoint)
          .local_integrity_verified;
  verification.fresh_replay_certified = observed == expected;
  verification.next_checkpoint_anchored =
      verification.source_checkpoint_anchored &&
      verification.fresh_replay_certified;
  verification.chunk_transition_verified =
      verification.source_checkpoint_local_integrity_verified &&
      verification.source_checkpoint_anchored &&
      verification.requested_budget_certified &&
      verification.prepared_transition_chain_matches &&
      verification.records_individually_exact &&
      verification.next_checkpoint_local_integrity_verified &&
      verification.next_checkpoint_anchored &&
      verification.fresh_replay_certified;
  if (verification.chunk_transition_verified &&
      freshly_replayed_next != nullptr) {
    *freshly_replayed_next = std::move(expected.next_checkpoint);
  }
  return verification;
}

}  // namespace

static_assert(
    std::is_nothrow_move_assignable_v<ExactHigherSupportCheckpoint>,
    "anchored higher-support commit requires atomic nothrow checkpoint move");

ExactHigherSupportAnchoredSession::ExactHigherSupportAnchoredSession(
    const ExactHigherSupportAuthorityContext& authority)
    : authority_(authority),
      trusted_checkpoint_(
          make_initial_exact_higher_support_checkpoint(authority_)) {
  if (!verify_exact_higher_support_checkpoint(
           authority_, trusted_checkpoint_)
           .local_integrity_verified) {
    throw std::logic_error(
        "the canonical higher-support roots failed local verification");
  }
}

ExactHigherSupportStreamChunk
ExactHigherSupportAnchoredSession::prepare_next(
    const ExactHigherSupportStreamBudget& chunk_budget,
    const ExactHigherSupportCheckpoint& reinjected_source) const {
  if (reinjected_source != trusted_checkpoint_) {
    throw std::invalid_argument(
        "a higher-support session can prepare only from its anchored checkpoint");
  }
  if (trusted_checkpoint_.locally_complete()) {
    throw std::logic_error(
        "a terminal higher-support session has no successor chunk");
  }
  return build_exact_higher_support_stream_chunk_after_source_verification(
      authority_.index(),
      authority_.cloud(),
      authority_.requested_maximum_order(),
      chunk_budget,
      trusted_checkpoint_);
}

ExactHigherSupportStreamChunkVerification
ExactHigherSupportAnchoredSession::commit_prepared(
    const ExactHigherSupportStreamBudget& chunk_budget,
    const ExactHigherSupportCheckpoint& reinjected_source,
    const ExactHigherSupportStreamChunk& candidate) {
  if (reinjected_source != trusted_checkpoint_) {
    return {};
  }
  if (trusted_checkpoint_.locally_complete()) {
    return {};
  }
  ExactHigherSupportCheckpoint freshly_replayed_next;
  ExactHigherSupportStreamChunkVerification verification =
      verify_exact_higher_support_stream_chunk_from_trusted_source(
          authority_,
          chunk_budget,
          trusted_checkpoint_,
          candidate,
          &freshly_replayed_next);
  if (verification.chunk_transition_verified) {
    trusted_checkpoint_ = std::move(freshly_replayed_next);
  }
  return verification;
}

static_assert(
    std::is_nothrow_move_constructible_v<
        ExactHigherSupportTerminalSegment>,
    "terminal higher-support append requires a nothrow segment move");
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactHigherSupportCheckpointManifest>,
    "an unsealed higher-support drain requires a nothrow manifest move");
static_assert(
    std::is_nothrow_move_constructible_v<ExactHigherSupportCheckpoint>,
    "terminal higher-support seal requires a nothrow checkpoint move");
static_assert(
    std::is_nothrow_move_assignable_v<ExactHigherSupportCheckpoint>,
    "terminal higher-support append requires a nothrow checkpoint assignment");

ExactHigherSupportUnsealedDrain::ExactHigherSupportUnsealedDrain(
    ExactHigherSupportUnsealedDrain&& other) noexcept
    : status_(other.status_),
      outstanding_segment_(std::move(other.outstanding_segment_)),
      successfully_consumed_(other.successfully_consumed_),
      moved_from_(other.moved_from_) {
  if (other.manifest_) {
    manifest_.emplace(std::move(*other.manifest_));
  }
  if (other.segment_) {
    segment_.emplace(std::move(*other.segment_));
  }
  other.invalidate_moved_from();
}

ExactHigherSupportUnsealedDrain::ExactHigherSupportUnsealedDrain(
    ExactHigherSupportUnsealedDrainStatus status) noexcept
    : status_(status) {}

ExactHigherSupportUnsealedDrain::ExactHigherSupportUnsealedDrain(
    ExactHigherSupportTerminalSegment&& segment,
    ExactHigherSupportCheckpointManifest&& manifest,
    std::shared_ptr<bool> outstanding_segment) noexcept
    : status_(ExactHigherSupportUnsealedDrainStatus::segment_ready),
      manifest_(std::move(manifest)),
      segment_(std::move(segment)),
      outstanding_segment_(std::move(outstanding_segment)) {}

void ExactHigherSupportUnsealedDrain::consume() noexcept {
  if (outstanding_segment_ != nullptr) {
    *outstanding_segment_ = false;
  }
  segment_.reset();
  manifest_.reset();
  outstanding_segment_.reset();
  successfully_consumed_ = true;
  moved_from_ = false;
}

void ExactHigherSupportUnsealedDrain::invalidate_moved_from() noexcept {
  segment_.reset();
  manifest_.reset();
  outstanding_segment_.reset();
  successfully_consumed_ = false;
  moved_from_ = true;
}

ExactHigherSupportTerminalAuthority::
    ExactHigherSupportTerminalAuthority(
        ExactHigherSupportTerminalAuthority&& other) noexcept
    : ExactHigherSupportTerminalAuthority() {
  *this = std::move(other);
}

ExactHigherSupportTerminalAuthority&
ExactHigherSupportTerminalAuthority::operator=(
    ExactHigherSupportTerminalAuthority&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  index_ = other.index_;
  cloud_ = other.cloud_;
  chunk_budget_ = other.chunk_budget_;
  maximum_chunk_count_ = other.maximum_chunk_count_;
  chunk_count_ = other.chunk_count_;
  event_count_ = other.event_count_;
  relevant_extra_shell_diagnostic_count_ =
      other.relevant_extra_shell_diagnostic_count_;
  destroyed_prune_certificate_count_ =
      other.destroyed_prune_certificate_count_;
  destroyed_rank_receipt_count_ =
      other.destroyed_rank_receipt_count_;
  index_identity_ = std::move(other.index_identity_);
  cloud_identity_ = std::move(other.cloud_identity_);
  terminal_checkpoint_ = std::move(other.terminal_checkpoint_);
  segments_ = std::move(other.segments_);
  terminal_checkpoint_local_integrity_verified_ =
      other.terminal_checkpoint_local_integrity_verified_;
  canonical_root_anchored_internal_production_ =
      other.canonical_root_anchored_internal_production_;
  committed_chunks_captured_once_without_fresh_replay_ =
      other.committed_chunks_captured_once_without_fresh_replay_;
  no_forbidden_global_structure_materialized_ =
      other.no_forbidden_global_structure_materialized_;
  consumed_ = other.consumed_;
  other.invalidate();
  return *this;
}

void ExactHigherSupportTerminalAuthority::invalidate() noexcept {
  index_ = nullptr;
  cloud_ = nullptr;
  index_identity_.reset();
  cloud_identity_.reset();
  consumed_ = true;
}

ExactHigherSupportTerminalAuthority::
    ExactHigherSupportTerminalAuthority(
        const spatial::MortonLbvhIndex& index,
        const spatial::CanonicalPointCloud& cloud,
        ExactHigherSupportStreamBudget chunk_budget,
        std::size_t maximum_chunk_count,
        std::size_t event_count,
        std::size_t relevant_extra_shell_diagnostic_count,
        std::size_t destroyed_prune_certificate_count,
        std::size_t destroyed_rank_receipt_count,
        ExactHigherSupportCheckpoint&& terminal_checkpoint,
        std::vector<ExactHigherSupportTerminalSegment>&& segments) noexcept
    : index_(&index),
      cloud_(&cloud),
      chunk_budget_(chunk_budget),
      maximum_chunk_count_(maximum_chunk_count),
      chunk_count_(segments.size()),
      event_count_(event_count),
      relevant_extra_shell_diagnostic_count_(
          relevant_extra_shell_diagnostic_count),
      destroyed_prune_certificate_count_(
          destroyed_prune_certificate_count),
      destroyed_rank_receipt_count_(destroyed_rank_receipt_count),
      index_identity_(index.identity_),
      cloud_identity_(cloud.identity_),
      terminal_checkpoint_(std::move(terminal_checkpoint)),
      segments_(std::move(segments)),
      terminal_checkpoint_local_integrity_verified_(true),
      canonical_root_anchored_internal_production_(true),
      committed_chunks_captured_once_without_fresh_replay_(true),
      no_forbidden_global_structure_materialized_(true),
      consumed_(false) {}

bool ExactHigherSupportTerminalAuthority::
    sealed_in_process_terminal_authority() const noexcept {
  const bool first_sum_fits =
      relevant_extra_shell_diagnostic_count_ <=
      std::numeric_limits<std::size_t>::max() - event_count_;
  const std::size_t retained_record_count =
      first_sum_fits
          ? event_count_ + relevant_extra_shell_diagnostic_count_
          : 0U;
  const bool complete_sum_fits =
      first_sum_fits &&
      destroyed_prune_certificate_count_ <=
          std::numeric_limits<std::size_t>::max() -
              retained_record_count;
  const std::size_t total_record_count =
      complete_sum_fits
          ? retained_record_count +
                destroyed_prune_certificate_count_
          : 0U;
  return !consumed_ && index_ != nullptr && cloud_ != nullptr &&
         terminal_checkpoint_local_integrity_verified_ &&
         canonical_root_anchored_internal_production_ &&
         committed_chunks_captured_once_without_fresh_replay_ &&
         no_forbidden_global_structure_materialized_ &&
         index_identity_ != nullptr && cloud_identity_ != nullptr &&
         index_->identity_ == index_identity_ &&
         cloud_->identity_ == cloud_identity_ &&
         index_->validated_for(*cloud_) &&
         terminal_checkpoint_.locally_complete() &&
         terminal_checkpoint_.next_chunk_sequence == chunk_count_ &&
         chunk_count_ <= maximum_chunk_count_ &&
         segments_.size() == chunk_count_ &&
         terminal_checkpoint_.cumulative_audit.accepted_event_count ==
             event_count_ &&
         terminal_checkpoint_.cumulative_audit
                 .relevant_extra_shell_diagnostic_count ==
             relevant_extra_shell_diagnostic_count_ &&
         terminal_checkpoint_.cumulative_audit
                 .emitted_prune_certificate_count ==
             destroyed_prune_certificate_count_ &&
         terminal_checkpoint_.cumulative_audit
                 .emitted_rank_receipt_count ==
             destroyed_rank_receipt_count_ &&
         complete_sum_fits &&
         terminal_checkpoint_.output_record_count ==
             total_record_count &&
         terminal_checkpoint_.cumulative_audit.emitted_record_count ==
             total_record_count &&
         terminal_checkpoint_.cumulative_audit
             .exact_bigint_universe_certified &&
         !fresh_replay_performed() && !durable_authority_claimed() &&
         !public_status_claimed();
}

ExactHigherSupportTerminalSession::ExactHigherSupportTerminalSession(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    ExactHigherSupportStreamBudget chunk_budget,
    std::size_t maximum_chunk_count)
    : ExactHigherSupportTerminalSession(
          ExactHigherSupportAuthorityContext{
              index, cloud, requested_maximum_order},
          chunk_budget,
          maximum_chunk_count) {}

ExactHigherSupportTerminalSession::ExactHigherSupportTerminalSession(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportPositiveDecisionSource&
        positive_decision_source,
    ExactHigherSupportStreamBudget chunk_budget,
    std::size_t maximum_chunk_count)
    : ExactHigherSupportTerminalSession(
          ExactHigherSupportAuthorityContext{
              index, cloud, requested_maximum_order},
          positive_decision_source,
          chunk_budget,
          maximum_chunk_count) {}

ExactHigherSupportTerminalSession::ExactHigherSupportTerminalSession(
    const ExactHigherSupportAuthorityContext& authority,
    ExactHigherSupportStreamBudget chunk_budget,
    std::size_t maximum_chunk_count)
    : authority_(authority),
      chunk_budget_(chunk_budget),
      maximum_chunk_count_(maximum_chunk_count),
      trusted_checkpoint_(
          make_initial_exact_higher_support_checkpoint(authority_)),
      unsealed_segment_outstanding_(std::make_shared<bool>(false)) {
  if (!verify_exact_higher_support_checkpoint(
           authority_, trusted_checkpoint_)
           .local_integrity_verified) {
    throw std::logic_error(
        "the terminal higher-support session canonical roots failed local verification");
  }
  if (trusted_checkpoint_.frontier.size() >
      chunk_budget_.maximum_frontier_entry_count) {
    throw std::invalid_argument(
        "the terminal higher-support chunk budget cannot hold the canonical roots");
  }
  released_output_chain_digest_ =
      trusted_checkpoint_.output_chain_digest;
  released_checkpoint_digest_ =
      trusted_checkpoint_.checkpoint_digest;
  if (!retained_segment_accounting_holds()) {
    throw std::logic_error(
        "the terminal higher-support initial retained accounting is inconsistent");
  }
}

ExactHigherSupportTerminalSession::ExactHigherSupportTerminalSession(
    const ExactHigherSupportAuthorityContext& authority,
    const ExactHigherSupportPositiveDecisionSource&
        positive_decision_source,
    ExactHigherSupportStreamBudget chunk_budget,
    std::size_t maximum_chunk_count)
    : ExactHigherSupportTerminalSession(
          authority, chunk_budget, maximum_chunk_count) {
  if (!positive_decision_source.bound_to(
          authority_.index(), authority_.cloud())) {
    throw std::invalid_argument(
        "the terminal higher-support accelerator decision source is not "
        "bound to the session authorities");
  }
  positive_decision_source_ = &positive_decision_source;
}

bool ExactHigherSupportTerminalSession::
retained_segment_accounting_holds() const noexcept {
  if (released_segment_count_ > chunk_count_ ||
      segments_.size() >
          std::numeric_limits<std::size_t>::max() -
              released_segment_count_ ||
      released_segment_count_ + segments_.size() != chunk_count_ ||
      chunk_count_ > maximum_chunk_count_ ||
      trusted_checkpoint_.next_chunk_sequence != chunk_count_ ||
      unsealed_segment_drain_performed_ !=
          (released_segment_count_ != 0U) ||
      (unsealed_segment_drain_performed_ && segments_.size() > 1U) ||
      released_event_count_ > event_count_ ||
      released_relevant_extra_shell_diagnostic_count_ >
          relevant_extra_shell_diagnostic_count_ ||
      released_destroyed_prune_certificate_count_ >
          destroyed_prune_certificate_count_ ||
      released_destroyed_rank_receipt_count_ >
          destroyed_rank_receipt_count_ ||
      released_output_record_count_ >
          trusted_checkpoint_.output_record_count ||
      released_point_id_reference_count_ >
          trusted_checkpoint_.cumulative_audit
              .emitted_point_id_reference_count ||
      trusted_checkpoint_.output_record_count !=
          trusted_checkpoint_.cumulative_audit.emitted_record_count ||
      event_count_ !=
          trusted_checkpoint_.cumulative_audit.accepted_event_count ||
      relevant_extra_shell_diagnostic_count_ !=
          trusted_checkpoint_.cumulative_audit
              .relevant_extra_shell_diagnostic_count ||
      destroyed_prune_certificate_count_ !=
          trusted_checkpoint_.cumulative_audit
              .emitted_prune_certificate_count ||
      destroyed_rank_receipt_count_ !=
          trusted_checkpoint_.cumulative_audit
              .emitted_rank_receipt_count) {
    return false;
  }

  if (segments_.empty()) {
    return released_event_count_ == event_count_ &&
        released_relevant_extra_shell_diagnostic_count_ ==
            relevant_extra_shell_diagnostic_count_ &&
        released_destroyed_prune_certificate_count_ ==
            destroyed_prune_certificate_count_ &&
        released_destroyed_rank_receipt_count_ ==
            destroyed_rank_receipt_count_ &&
        released_output_record_count_ ==
            trusted_checkpoint_.output_record_count &&
        released_point_id_reference_count_ ==
            trusted_checkpoint_.cumulative_audit
                .emitted_point_id_reference_count &&
        released_output_chain_digest_ ==
            trusted_checkpoint_.output_chain_digest &&
        released_checkpoint_digest_ ==
            trusted_checkpoint_.checkpoint_digest;
  }

  const ExactHigherSupportTerminalSegment& first = segments_.front();
  const ExactHigherSupportTerminalSegment& last = segments_.back();
  if (chunk_count_ == 0U ||
      first.chunk_sequence != released_segment_count_ ||
      last.chunk_sequence != chunk_count_ - 1U ||
      first.first_output_record_index != released_output_record_count_ ||
      first.source_checkpoint_digest != released_checkpoint_digest_ ||
      first.previous_output_chain_digest !=
          released_output_chain_digest_ ||
      last.next_checkpoint_digest !=
          trusted_checkpoint_.checkpoint_digest ||
      last.output_chain_digest !=
          trusted_checkpoint_.output_chain_digest ||
      last.events.size() >
          std::numeric_limits<std::size_t>::max() -
              last.relevant_extra_shell_diagnostics.size()) {
    return false;
  }
  if (segments_.size() == 1U &&
      (last.events.size() != event_count_ - released_event_count_ ||
       last.relevant_extra_shell_diagnostics.size() !=
           relevant_extra_shell_diagnostic_count_ -
               released_relevant_extra_shell_diagnostic_count_ ||
       last.destroyed_prune_certificate_count !=
           destroyed_prune_certificate_count_ -
               released_destroyed_prune_certificate_count_ ||
       last.destroyed_rank_receipt_count !=
           destroyed_rank_receipt_count_ -
               released_destroyed_rank_receipt_count_ ||
       last.emitted_point_id_reference_count !=
           trusted_checkpoint_.cumulative_audit
                   .emitted_point_id_reference_count -
               released_point_id_reference_count_)) {
    return false;
  }
  const std::size_t retained_record_count =
      last.events.size() +
      last.relevant_extra_shell_diagnostics.size();
  if (last.destroyed_prune_certificate_count >
          std::numeric_limits<std::size_t>::max() -
              retained_record_count ||
      last.first_output_record_index >
          std::numeric_limits<std::size_t>::max() -
              last.emitted_record_count()) {
    return false;
  }
  return retained_record_count +
          last.destroyed_prune_certificate_count ==
          last.emitted_record_count() &&
      last.first_output_record_index + last.emitted_record_count() ==
          trusted_checkpoint_.output_record_count;
}

void ExactHigherSupportTerminalSession::append_next_internal_chunk() {
  if (sealed_) {
    throw std::logic_error(
        "a sealed terminal higher-support session cannot execute a chunk");
  }
  if (trusted_checkpoint_.locally_complete()) {
    throw std::logic_error(
        "a terminal higher-support checkpoint has no successor chunk");
  }
  if (chunk_count_ >= maximum_chunk_count_) {
    throw std::logic_error(
        "the terminal higher-support chunk cap has already been reached");
  }
  if (!retained_segment_accounting_holds()) {
    throw std::logic_error(
        "the terminal higher-support retained accounting changed before a chunk");
  }
  if (!authority_.bound_to_original_sources()) {
    throw std::logic_error(
        "the terminal higher-support source identity changed before a chunk");
  }
  if (segments_.size() == segments_.capacity()) {
    std::size_t next_capacity =
        segments_.capacity() == 0U ? 1U : segments_.capacity();
    if (next_capacity < maximum_chunk_count_) {
      next_capacity = checked_add(
          next_capacity,
          std::min(
              next_capacity,
              maximum_chunk_count_ - next_capacity),
          "the terminal higher-support geometric segment capacity overflows size_t");
    }
    // Allocation happens before any exact traversal.  A failure therefore
    // cannot make a retry execute the same scientific chunk twice.
    segments_.reserve(next_capacity);
  }
  const std::size_t next_chunk_count = checked_add(
      chunk_count_,
      1U,
      "the terminal higher-support chunk count overflows size_t");

  // This is the only exact traversal execution for this sequence.  Unlike the
  // externally supplied anchored protocol, no candidate is replayed.
  ExactHigherSupportStreamChunk chunk =
      build_exact_higher_support_stream_chunk_after_source_verification(
          authority_.index(),
          authority_.cloud(),
          authority_.requested_maximum_order(),
          chunk_budget_,
          trusted_checkpoint_,
          positive_decision_source_);
  if (!chunk.builder_invariants_hold() ||
      chunk.manifest != authority_.manifest() ||
      chunk.budget != chunk_budget_ ||
      chunk.chunk_sequence != trusted_checkpoint_.next_chunk_sequence ||
      chunk.chunk_sequence != chunk_count_ ||
      chunk.first_output_record_index !=
          trusted_checkpoint_.output_record_count ||
      chunk.source_checkpoint_digest !=
          trusted_checkpoint_.checkpoint_digest ||
      chunk.previous_output_chain_digest !=
          trusted_checkpoint_.output_chain_digest ||
      chunk.cumulative_audit_before !=
          trusted_checkpoint_.cumulative_audit) {
    throw std::logic_error(
        "the internally produced higher-support chunk broke its root-anchored chain");
  }
  const bool scientific_progress =
      chunk.cumulative_audit_after.work_unit_count >
          chunk.cumulative_audit_before.work_unit_count ||
      chunk.cumulative_audit_after.emitted_record_count >
          chunk.cumulative_audit_before.emitted_record_count ||
      chunk.next_checkpoint.locally_complete();
  if (!scientific_progress) {
    throw std::length_error(
        "the fixed higher-support chunk budget made no scientific progress");
  }
  if (!verify_exact_higher_support_checkpoint(
           authority_, chunk.next_checkpoint)
           .local_integrity_verified) {
    throw std::logic_error(
        "the internally produced higher-support successor failed local verification");
  }

  const std::size_t next_event_count = checked_add(
      event_count_,
      chunk.events.size(),
      "the terminal higher-support event count overflows size_t");
  const std::size_t next_diagnostic_count = checked_add(
      relevant_extra_shell_diagnostic_count_,
      chunk.relevant_extra_shell_diagnostics.size(),
      "the terminal higher-support diagnostic count overflows size_t");
  const std::size_t next_prune_certificate_count = checked_add(
      destroyed_prune_certificate_count_,
      chunk.prune_certificates.size(),
      "the terminal higher-support prune count overflows size_t");
  std::size_t chunk_rank_receipt_count = 0U;
  for (const ExactHigherSupportPruneCertificate& certificate :
       chunk.prune_certificates) {
    chunk_rank_receipt_count = checked_add(
        chunk_rank_receipt_count,
        certificate.rank_receipts.size(),
        "the terminal higher-support rank-receipt count overflows size_t");
  }
  const std::size_t next_rank_receipt_count = checked_add(
      destroyed_rank_receipt_count_,
      chunk_rank_receipt_count,
      "the terminal higher-support rank-receipt count overflows size_t");
  if (chunk.cumulative_audit_after.emitted_rank_receipt_count <
          chunk.cumulative_audit_before.emitted_rank_receipt_count ||
      chunk.cumulative_audit_after.emitted_rank_receipt_count -
              chunk.cumulative_audit_before.emitted_rank_receipt_count !=
          chunk_rank_receipt_count) {
    throw std::logic_error(
        "the terminal higher-support rank-receipt audit is inconsistent");
  }
  if (chunk.cumulative_audit_after.emitted_point_id_reference_count <
      chunk.cumulative_audit_before.emitted_point_id_reference_count) {
    throw std::logic_error(
        "the terminal higher-support point-reference audit moved backwards");
  }

  ExactHigherSupportTerminalSegment segment;
  segment.chunk_sequence = chunk.chunk_sequence;
  segment.first_output_record_index =
      chunk.first_output_record_index;
  segment.source_checkpoint_digest =
      chunk.source_checkpoint_digest;
  segment.next_checkpoint_digest =
      chunk.next_checkpoint.checkpoint_digest;
  segment.previous_output_chain_digest =
      chunk.previous_output_chain_digest;
  segment.output_chain_digest = chunk.output_chain_digest;
  segment.status = chunk.status;
  segment.stop_reason = chunk.stop_reason;
  segment.events = std::move(chunk.events);
  segment.relevant_extra_shell_diagnostics =
      std::move(chunk.relevant_extra_shell_diagnostics);
  segment.emitted_record_count_value = chunk.record_order.size();
  segment.destroyed_prune_certificate_count =
      chunk.prune_certificates.size();
  segment.destroyed_rank_receipt_count = chunk_rank_receipt_count;
  segment.emitted_point_id_reference_count =
      chunk.cumulative_audit_after.emitted_point_id_reference_count -
      chunk.cumulative_audit_before.emitted_point_id_reference_count;

  // Certificate payloads have already contributed to the chain and counters.
  // Destroy them before the session retains anything from this chunk.
  chunk.prune_certificates.clear();
  ExactHigherSupportCheckpoint next_checkpoint =
      std::move(chunk.next_checkpoint);

  // Geometric capacity was secured before traversal and the segment move is
  // nothrow, so the append and all following commits cannot throw.
  segments_.push_back(std::move(segment));
  trusted_checkpoint_ = std::move(next_checkpoint);
  event_count_ = next_event_count;
  relevant_extra_shell_diagnostic_count_ =
      next_diagnostic_count;
  destroyed_prune_certificate_count_ =
      next_prune_certificate_count;
  destroyed_rank_receipt_count_ = next_rank_receipt_count;
  chunk_count_ = next_chunk_count;
  // No recovery boundary remains after the nothrow scientific commit.  An
  // impossible invariant failure must therefore fail-stop instead of
  // exposing a catchable exception whose retry would skip this chunk.
  if (!retained_segment_accounting_holds()) {
    std::terminate();
  }
}

ExactHigherSupportTerminalRunStatus
ExactHigherSupportTerminalSession::run_to_terminal() {
  if (sealed_) {
    throw std::logic_error(
        "a sealed terminal higher-support session cannot run");
  }
  if (unsealed_segment_drain_performed_) {
    throw std::logic_error(
        "an unsealed-drain higher-support session must advance one segment at a time");
  }
  if (!authority_.bound_to_original_sources()) {
    throw std::logic_error(
        "the terminal higher-support source identity changed before execution");
  }
  while (!trusted_checkpoint_.locally_complete()) {
    if (chunk_count_ >= maximum_chunk_count_) {
      return ExactHigherSupportTerminalRunStatus::
          maximum_chunk_count_reached;
    }
    append_next_internal_chunk();
  }
  return ExactHigherSupportTerminalRunStatus::terminal;
}

ExactHigherSupportResidentAdvanceStatus
ExactHigherSupportTerminalSession::advance_one_resident_chunk() & {
  if (sealed_) {
    throw std::logic_error(
        "a sealed terminal higher-support session cannot run");
  }
  if (unsealed_segment_drain_performed_) {
    throw std::logic_error(
        "an unsealed-drain higher-support session must advance one segment at a time");
  }
  if (!authority_.bound_to_original_sources()) {
    throw std::logic_error(
        "the terminal higher-support source identity changed before execution");
  }
  if (trusted_checkpoint_.locally_complete()) {
    return ExactHigherSupportResidentAdvanceStatus::terminal;
  }
  if (chunk_count_ >= maximum_chunk_count_) {
    return ExactHigherSupportResidentAdvanceStatus::
        maximum_chunk_count_reached;
  }
  append_next_internal_chunk();
  return ExactHigherSupportResidentAdvanceStatus::chunk_committed;
}

ExactHigherSupportUnsealedDrain
ExactHigherSupportTerminalSession::drain_next_unsealed_segment() & {
  if (sealed_) {
    throw std::logic_error(
        "a sealed terminal higher-support session cannot drain a segment");
  }
  if (!segments_.empty()) {
    throw std::logic_error(
        "a higher-support drain cannot skip a resident segment prefix");
  }
  if (unconsumed_segment_outstanding()) {
    throw std::logic_error(
        "a higher-support session cannot skip an unconsumed drained segment");
  }
  if (!retained_segment_accounting_holds()) {
    throw std::logic_error(
        "the terminal higher-support retained accounting changed before a drain");
  }
  if (!authority_.bound_to_original_sources()) {
    throw std::logic_error(
        "the terminal higher-support source identity changed before a drain");
  }
  if (trusted_checkpoint_.locally_complete()) {
    return ExactHigherSupportUnsealedDrain{
        ExactHigherSupportUnsealedDrainStatus::terminal};
  }
  if (chunk_count_ >= maximum_chunk_count_) {
    return ExactHigherSupportUnsealedDrain{
        ExactHigherSupportUnsealedDrainStatus::
            maximum_chunk_count_reached};
  }

  // Copy the fixed-size source contract before the scientific transaction.
  // Even if a future manifest representation allocates, a failed copy cannot
  // advance the checkpoint or lose the next segment.
  ExactHigherSupportCheckpointManifest source_manifest =
      trusted_checkpoint_.manifest;

  // append_next_internal_chunk executes exactly one traversal and retains one
  // bounded suffix segment.  Moving that sole suffix advances the released
  // prefix without replaying or manufacturing any durable authority.
  append_next_internal_chunk();
  ExactHigherSupportTerminalSegment released =
      std::move(segments_.back());
  segments_.clear();
  released_segment_count_ = chunk_count_;
  released_event_count_ = event_count_;
  released_relevant_extra_shell_diagnostic_count_ =
      relevant_extra_shell_diagnostic_count_;
  released_destroyed_prune_certificate_count_ =
      destroyed_prune_certificate_count_;
  released_destroyed_rank_receipt_count_ =
      destroyed_rank_receipt_count_;
  released_output_record_count_ =
      trusted_checkpoint_.output_record_count;
  released_point_id_reference_count_ =
      trusted_checkpoint_.cumulative_audit
          .emitted_point_id_reference_count;
  released_output_chain_digest_ =
      trusted_checkpoint_.output_chain_digest;
  released_checkpoint_digest_ =
      trusted_checkpoint_.checkpoint_digest;
  unsealed_segment_drain_performed_ = true;
  // Every value above is a direct no-throw projection of the already checked
  // resident suffix.  Continuing after an impossible post-commit mismatch
  // could skip the detached scientific segment, so fail-stop instead of
  // exposing a catchable exception with a partially released prefix.
  if (!retained_segment_accounting_holds()) {
    std::terminate();
  }
  *unsealed_segment_outstanding_ = true;
  return ExactHigherSupportUnsealedDrain{
      std::move(released), std::move(source_manifest),
      unsealed_segment_outstanding_};
}

ExactHigherSupportTerminalAuthority
ExactHigherSupportTerminalSession::seal() && {
  if (sealed_) {
    throw std::logic_error(
        "a terminal higher-support session can be sealed only once");
  }
  if (unsealed_segment_drain_performed_) {
    throw std::logic_error(
        "a drained higher-support session cannot mint a resident authority");
  }
  if (!trusted_checkpoint_.locally_complete()) {
    throw std::logic_error(
        "a nonterminal higher-support session cannot mint an authority");
  }
  if (!authority_.bound_to_original_sources()) {
    throw std::logic_error(
        "the terminal higher-support source identity changed before sealing");
  }
  const ExactHigherSupportCheckpointVerification verification =
      verify_exact_higher_support_checkpoint(
          authority_, trusted_checkpoint_);
  if (!verification.local_integrity_verified) {
    throw std::logic_error(
        "a terminal higher-support authority requires a locally verified checkpoint");
  }
  if (!retained_segment_accounting_holds() ||
      trusted_checkpoint_.next_chunk_sequence != chunk_count_ ||
      chunk_count_ != segments_.size() ||
      trusted_checkpoint_.cumulative_audit.accepted_event_count !=
          event_count_ ||
      trusted_checkpoint_.cumulative_audit
              .relevant_extra_shell_diagnostic_count !=
          relevant_extra_shell_diagnostic_count_ ||
      trusted_checkpoint_.cumulative_audit
              .emitted_prune_certificate_count !=
          destroyed_prune_certificate_count_ ||
      trusted_checkpoint_.cumulative_audit.emitted_rank_receipt_count !=
          destroyed_rank_receipt_count_) {
    throw std::logic_error(
        "the terminal higher-support retained-segment accounting is inconsistent");
  }

  std::size_t expected_first_output_record_index = 0U;
  contract::CanonicalId expected_previous_output_chain =
      initial_output_chain_digest();
  contract::CanonicalId expected_source_checkpoint_digest =
      make_initial_exact_higher_support_checkpoint(authority_)
          .checkpoint_digest;
  for (std::size_t segment_index = 0U;
       segment_index < segments_.size();
       ++segment_index) {
    const ExactHigherSupportTerminalSegment& segment =
        segments_[segment_index];
    const std::size_t retained_and_destroyed_record_count = checked_add(
        checked_add(
            segment.events.size(),
            segment.relevant_extra_shell_diagnostics.size(),
            "the terminal higher-support segment record count overflows size_t"),
        segment.destroyed_prune_certificate_count,
        "the terminal higher-support segment record count overflows size_t");
    if (segment.chunk_sequence != segment_index ||
        segment.first_output_record_index !=
            expected_first_output_record_index ||
        segment.emitted_record_count() !=
            retained_and_destroyed_record_count ||
        segment.source_checkpoint_digest !=
            expected_source_checkpoint_digest ||
        segment.previous_output_chain_digest !=
            expected_previous_output_chain) {
      throw std::logic_error(
          "the terminal higher-support segment chain is not dense");
    }
    expected_first_output_record_index = checked_add(
        expected_first_output_record_index,
        segment.emitted_record_count(),
        "the terminal higher-support output-record count overflows size_t");
    expected_previous_output_chain = segment.output_chain_digest;
    expected_source_checkpoint_digest =
        segment.next_checkpoint_digest;
  }
  if (expected_first_output_record_index !=
          trusted_checkpoint_.output_record_count ||
      expected_previous_output_chain !=
          trusted_checkpoint_.output_chain_digest ||
      expected_source_checkpoint_digest !=
          trusted_checkpoint_.checkpoint_digest) {
    throw std::logic_error(
        "the terminal higher-support segment chain does not reach its checkpoint");
  }

  sealed_ = true;
  return ExactHigherSupportTerminalAuthority{
      authority_.index(),
      authority_.cloud(),
      chunk_budget_,
      maximum_chunk_count_,
      event_count_,
      relevant_extra_shell_diagnostic_count_,
      destroyed_prune_certificate_count_,
      destroyed_rank_receipt_count_,
      std::move(trusted_checkpoint_),
      std::move(segments_)};
}

ExactHigherSupportStreamRunVerification
verify_exact_higher_support_stream_run(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    std::span<const ExactHigherSupportStreamBudget> chunk_budgets,
    std::span<const ExactHigherSupportStreamChunk> chunks) {
  ExactHigherSupportStreamRunVerification verification;
  if (chunk_budgets.size() != chunks.size()) {
    return verification;
  }
  const ExactHigherSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  ExactHigherSupportCheckpoint trusted_checkpoint =
      make_initial_exact_higher_support_checkpoint(authority);
  verification.initial_checkpoint_reconstructed =
      verify_exact_higher_support_checkpoint(authority, trusted_checkpoint)
          .local_integrity_verified;
  if (!verification.initial_checkpoint_reconstructed) {
    return verification;
  }
  verification.every_transition_verified = true;
  for (std::size_t chunk_index = 0U;
       chunk_index < chunks.size();
       ++chunk_index) {
    if (trusted_checkpoint.locally_complete()) {
      verification.every_transition_verified = false;
      break;
    }
    ExactHigherSupportCheckpoint freshly_replayed_next;
    const ExactHigherSupportStreamChunkVerification transition =
        verify_exact_higher_support_stream_chunk_from_trusted_source(
            authority,
            chunk_budgets[chunk_index],
            trusted_checkpoint,
            chunks[chunk_index],
            &freshly_replayed_next);
    if (!transition.chunk_transition_verified) {
      verification.every_transition_verified = false;
      break;
    }
    trusted_checkpoint = std::move(freshly_replayed_next);
    increment(
        verification.verified_chunk_count,
        "the higher-support verified chunk count overflows size_t");
  }
  verification.every_transition_verified =
      verification.every_transition_verified &&
      verification.verified_chunk_count == chunks.size();
  verification.terminal_checkpoint_reached =
      trusted_checkpoint.locally_complete();
  verification.anchored_run_certified =
      verification.initial_checkpoint_reconstructed &&
      verification.every_transition_verified &&
      verification.terminal_checkpoint_reached;
  return verification;
}

ExactHigherSupportStreamVerification verify_exact_higher_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactHigherSupportStreamBudget& budget,
    const ExactHigherSupportStreamResult& observed) {
  const ExactHigherSupportStreamResult expected =
      build_exact_higher_support_stream(
          index, cloud, requested_maximum_order, budget);
  ExactHigherSupportStreamVerification verification;
  verification.requested_budget_certified = observed.budget == budget;
  verification.requirements_certified =
      observed.requirements == expected.requirements;
  verification.exact_bigint_universe_certified =
      observed.audit.exact_bigint_universe_certified &&
      observed.audit.total_support_count ==
          exact_higher_support_candidate_universe_size(cloud.size()) &&
      observed.audit.total_support_count == expected.audit.total_support_count;
  verification.partial_records_individually_exact =
      observed.events == expected.events &&
      observed.relevant_extra_shell_diagnostics ==
          expected.relevant_extra_shell_diagnostics;
  verification.prune_certificates_replayed =
      observed.prune_certificates == expected.prune_certificates &&
      observed.all_prunes_replayable == expected.all_prunes_replayable;
  verification.grouped_frontier_replayed =
      observed.remaining_frontier == expected.remaining_frontier &&
      observed.audit.remaining_frontier_support_count ==
          expected.audit.remaining_frontier_support_count &&
      observed.audit.grouped_partition_accounting_certified ==
          expected.audit.grouped_partition_accounting_certified &&
      observed.grouped_frontier_partition_certified ==
          expected.grouped_frontier_partition_certified;
  verification.completion_claim_certified =
      observed.status == expected.status &&
      observed.stop_reason == expected.stop_reason &&
      observed.stream_complete() == expected.stream_complete();
  verification.absence_claim_certified =
      observed.absence_of_additional_higher_supports_certified() ==
      expected.absence_of_additional_higher_supports_certified();
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.requested_budget_certified &&
      verification.requirements_certified &&
      verification.exact_bigint_universe_certified &&
      verification.partial_records_individually_exact &&
      verification.prune_certificates_replayed &&
      verification.grouped_frontier_replayed &&
      verification.completion_claim_certified &&
      verification.absence_claim_certified &&
      verification.fresh_replay_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
