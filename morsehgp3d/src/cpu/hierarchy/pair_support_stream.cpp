#include "morsehgp3d/hierarchy/pair_support_stream.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/support.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct IntegrityVerifiedCheckpointTag {};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{message});
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(std::string{message});
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_unordered_pair_count(
    std::size_t point_count) {
  if (point_count < 2U) {
    return 0U;
  }
  std::size_t first = point_count;
  std::size_t second = point_count - 1U;
  if ((first & 1U) == 0U) {
    first /= 2U;
  } else {
    second /= 2U;
  }
  return checked_multiply(
      first,
      second,
      "the pair-support unordered pair count overflows size_t");
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    std::string_view message) {
  if (value > static_cast<std::size_t>(
                  std::numeric_limits<std::uint64_t>::max())) {
    throw std::overflow_error(std::string{message});
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    std::string_view message) {
  if (value > static_cast<std::uint64_t>(
                  std::numeric_limits<std::size_t>::max())) {
    throw std::overflow_error(std::string{message});
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] bool can_add_within(
    std::size_t current,
    std::size_t increment,
    std::size_t maximum) noexcept {
  return current <= maximum && increment <= maximum - current;
}

struct ExactBoxCoordinates {
  std::array<exact::ExactRational, 3> lower{};
  std::array<exact::ExactRational, 3> upper{};
};

[[nodiscard]] ExactBoxCoordinates exact_box_coordinates(
    const spatial::ExactDyadicAabb3& box) {
  ExactBoxCoordinates coordinates;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::uint64_t lower_bits =
        exact::canonicalize_binary64_bits(box.lower_binary64_bits[axis]);
    const std::uint64_t upper_bits =
        exact::canonicalize_binary64_bits(box.upper_binary64_bits[axis]);
    coordinates.lower[axis] =
        exact::ExactRational::from_binary64_bits(lower_bits);
    coordinates.upper[axis] =
        exact::ExactRational::from_binary64_bits(upper_bits);
    if (coordinates.upper[axis] < coordinates.lower[axis]) {
      throw std::invalid_argument(
          "an exact dyadic AABB has a reversed axis");
    }
  }
  return coordinates;
}

[[nodiscard]] bool event_less(
    const ExactPairSupportEvent& left,
    const ExactPairSupportEvent& right) {
  return left.support_ids < right.support_ids;
}

[[nodiscard]] bool diagnostic_less(
    const ExactPairSupportExtraShellDiagnostic& left,
    const ExactPairSupportExtraShellDiagnostic& right) {
  return left.support_ids < right.support_ids;
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
    u64(checked_u64(value, "a canonical digest size does not fit uint64"));
  }

  void text(std::string_view value) {
    size(value.size());
    builder_.update(value);
  }

  void identifier(const contract::CanonicalId& identifier) {
    builder_.update(identifier.bytes());
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

void append_frontier_entry(
    DigestWriter& writer,
    const ExactPairSupportFrontierEntry& entry) {
  writer.u64(entry.first_node_index);
  writer.u64(entry.second_node_index);
  writer.u64(entry.first_leaf_begin);
  writer.u64(entry.first_leaf_end);
  writer.u64(entry.second_leaf_begin);
  writer.u64(entry.second_leaf_end);
  writer.byte(entry.self_product);
}

void append_witness_node_entry(
    DigestWriter& writer,
    const ExactPairSupportWitnessNodeEntry& entry) {
  writer.u64(entry.node_index);
  writer.u64(entry.leaf_begin);
  writer.u64(entry.leaf_end);
}

void append_audit(
    DigestWriter& writer,
    const ExactPairSupportStreamAudit& audit) {
#define MORSEHGP3D_APPEND_AUDIT_SIZE(field) writer.size(audit.field)
  MORSEHGP3D_APPEND_AUDIT_SIZE(total_pair_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(work_unit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(support_product_visit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(support_product_expansion_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(self_product_expansion_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(cross_product_expansion_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(diagonal_leaf_discard_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(diagonal_product_rank_search_skip_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(rank_prune_search_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(witness_node_visit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_phi_aabb_bound_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_anchor_ball_minimum_aabb_bound_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(certified_anchor_noninterior_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(certified_anchor_noninterior_point_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(certified_anchor_shell_tangent_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(equality_or_positive_bound_descent_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(strict_interior_witness_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(strict_interior_witness_point_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(rank_pruned_product_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(rank_pruned_pair_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(leaf_pair_classification_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(global_closed_ball_query_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(point_classification_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_node_visit_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_closed_ball_minimum_aabb_bound_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_closed_ball_maximum_aabb_bound_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_bulk_interior_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_bulk_interior_point_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_bulk_exterior_subtree_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(closed_ball_bulk_exterior_point_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(early_closed_rank_rejection_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(exact_point_distance_evaluation_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(accepted_event_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(relevant_extra_shell_diagnostic_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(emitted_point_id_reference_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(above_rank_pair_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(maximum_frontier_entry_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(maximum_witness_frontier_entry_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(maximum_closed_ball_frontier_entry_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(remaining_frontier_pair_count);
  MORSEHGP3D_APPEND_AUDIT_SIZE(resolved_pair_count);
#undef MORSEHGP3D_APPEND_AUDIT_SIZE
  writer.boolean(audit.pair_partition_accounting_certified);
}

[[nodiscard]] contract::CanonicalId initial_output_chain_digest() {
  DigestWriter writer{
      "MorseHGP3D/phase9/pair-support/output-record-chain/v1/empty"};
  return writer.finalize();
}

[[nodiscard]] contract::CanonicalId extend_output_chain(
    const contract::CanonicalId& previous,
    const ExactPairSupportEvent& event) {
  DigestWriter writer{
      "MorseHGP3D/phase9/pair-support/output-record-chain/v1/event"};
  writer.identifier(previous);
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
    const ExactPairSupportExtraShellDiagnostic& diagnostic) {
  DigestWriter writer{
      "MorseHGP3D/phase9/pair-support/output-record-chain/v1/extra-shell"};
  writer.identifier(previous);
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

[[nodiscard]] contract::CanonicalId checkpoint_digest(
    const ExactPairSupportCheckpoint& checkpoint) {
  DigestWriter writer{
      "MorseHGP3D/phase9/pair-support/checkpoint/v1"};
  const ExactPairSupportCheckpointManifest& manifest = checkpoint.manifest;
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
  for (const ExactPairSupportFrontierEntry& entry : checkpoint.frontier) {
    append_frontier_entry(writer, entry);
  }
  writer.boolean(checkpoint.pending_product.has_value());
  if (checkpoint.pending_product.has_value()) {
    const ExactPairSupportPendingProduct& pending =
        *checkpoint.pending_product;
    append_frontier_entry(writer, pending.product);
    writer.byte(static_cast<std::uint8_t>(pending.stage));
    writer.boolean(pending.rank_search_started);
    writer.size(pending.witness_frontier.size());
    for (const ExactPairSupportWitnessNodeEntry& entry :
         pending.witness_frontier) {
      append_witness_node_entry(writer, entry);
    }
    writer.size(pending.strict_witness_receipts.size());
    for (const ExactPairSupportWitnessNodeEntry& entry :
         pending.strict_witness_receipts) {
      append_witness_node_entry(writer, entry);
    }
    writer.boolean(pending.deferred_expansion_node.has_value());
    if (pending.deferred_expansion_node.has_value()) {
      append_witness_node_entry(
          writer, *pending.deferred_expansion_node);
    }
    writer.size(pending.strict_witness_point_count);
  }
  append_audit(writer, checkpoint.cumulative_audit);
  return writer.finalize();
}

}  // namespace

ExactDiametralPhiAabbMaximum exact_diametral_phi_aabb_maximum(
    const spatial::ExactDyadicAabb3& first_support_box,
    const spatial::ExactDyadicAabb3& second_support_box,
    const spatial::ExactDyadicAabb3& query_box) {
  const ExactBoxCoordinates first =
      exact_box_coordinates(first_support_box);
  const ExactBoxCoordinates second =
      exact_box_coordinates(second_support_box);
  const ExactBoxCoordinates query = exact_box_coordinates(query_box);
  ExactDiametralPhiAabbMaximum result;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::array<exact::ExactRational, 2> first_endpoints{
        first.lower[axis], first.upper[axis]};
    const std::array<exact::ExactRational, 2> second_endpoints{
        second.lower[axis], second.upper[axis]};
    const std::array<exact::ExactRational, 2> query_endpoints{
        query.lower[axis], query.upper[axis]};
    bool initialized = false;
    exact::ExactRational axis_maximum;
    for (std::size_t query_selector = 0U;
         query_selector < query_endpoints.size();
         ++query_selector) {
      for (std::size_t first_selector = 0U;
           first_selector < first_endpoints.size();
           ++first_selector) {
        for (std::size_t second_selector = 0U;
             second_selector < second_endpoints.size();
             ++second_selector) {
          const exact::ExactRational candidate =
              (query_endpoints[query_selector] -
               first_endpoints[first_selector]) *
              (query_endpoints[query_selector] -
               second_endpoints[second_selector]);
          if (!initialized || candidate > axis_maximum) {
            initialized = true;
            axis_maximum = candidate;
            result.query_endpoint[axis] =
                static_cast<std::uint8_t>(query_selector);
            result.first_support_endpoint[axis] =
                static_cast<std::uint8_t>(first_selector);
            result.second_support_endpoint[axis] =
                static_cast<std::uint8_t>(second_selector);
          }
        }
      }
    }
    if (!initialized) {
      throw std::logic_error(
          "the exact diametral AABB maximum has no endpoint candidate");
    }
    result.maximum_phi = result.maximum_phi + axis_maximum;
  }
  return result;
}

class ExactPairSupportStreamBuilder {
 public:
  ExactPairSupportStreamBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactPairSupportStreamBudget budget)
      : index_(index),
        cloud_(cloud),
        output_chain_digest_(initial_output_chain_digest()),
        canonical_sort_records_(true) {
    validate_inputs(requested_maximum_order, budget);
    result_.requirements = requirements_for(requested_maximum_order);
    result_.budget = budget;
    result_.audit.total_pair_count =
        checked_unordered_pair_count(cloud_.size());
    if (result_.audit.total_pair_count != 0U) {
      if (budget.maximum_frontier_entry_count == 0U) {
        throw std::invalid_argument(
            "a nonempty pair frontier requires at least one budgeted entry");
      }
      frontier_.push_back(make_entry(index_.root_index_, index_.root_index_));
      result_.audit.maximum_frontier_entry_count = 1U;
    }
    chunk_audit_origin_ = result_.audit;
  }

  ExactPairSupportStreamBuilder(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactPairSupportStreamBudget budget,
      const ExactPairSupportCheckpoint& checkpoint,
      IntegrityVerifiedCheckpointTag)
      : index_(index),
        cloud_(cloud),
        output_chain_digest_(checkpoint.output_chain_digest),
        output_record_count_(checkpoint.output_record_count),
        canonical_sort_records_(false) {
    validate_inputs(requested_maximum_order, budget);
    result_.requirements = requirements_for(requested_maximum_order);
    result_.budget = budget;
    result_.audit = checkpoint.cumulative_audit;
    frontier_ = checkpoint.frontier;
    pending_product_ = checkpoint.pending_product;
    if (!frontier_.empty() &&
        budget.maximum_frontier_entry_count < frontier_.size()) {
      throw std::invalid_argument(
          "the chunk frontier capacity is below the persisted checkpoint frontier");
    }
    if (pending_product_.has_value()) {
      const std::size_t persisted_auxiliary_entry_count = checked_add(
          pending_product_->witness_frontier.size(),
          pending_product_->deferred_expansion_node.has_value() ? 1U : 0U,
          "the persisted auxiliary frontier count overflows size_t");
      if (budget.maximum_auxiliary_frontier_entry_count <
          persisted_auxiliary_entry_count) {
        throw std::invalid_argument(
            "the chunk auxiliary-frontier capacity is below the persisted checkpoint cursor");
      }
    }
    chunk_audit_origin_ = checkpoint.cumulative_audit;
    result_.audit.remaining_frontier_pair_count = 0U;
    result_.audit.pair_partition_accounting_certified = false;
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

  [[nodiscard]] ExactPairSupportStreamResult take_result() {
    return std::move(result_);
  }

  [[nodiscard]] const contract::CanonicalId& output_chain_digest()
      const noexcept {
    return output_chain_digest_;
  }

  [[nodiscard]] std::vector<ExactPairSupportStreamChunk::RecordKind>
  take_record_order() {
    return std::move(record_order_);
  }

  [[nodiscard]] ExactPairSupportCheckpoint snapshot_checkpoint(
      const ExactPairSupportCheckpointManifest& manifest,
      std::uint64_t next_chunk_sequence) const {
    ExactPairSupportCheckpoint checkpoint;
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

  [[nodiscard]] ExactPairSupportCheckpoint take_checkpoint(
      const ExactPairSupportCheckpointManifest& manifest,
      std::uint64_t next_chunk_sequence) {
    ExactPairSupportCheckpoint checkpoint;
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

  [[nodiscard]] static ExactPairSupportCheckpointManifest manifest_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order) {
    return manifest_for_with_audit(
        index, cloud, requested_maximum_order, nullptr);
  }

  [[nodiscard]] static ExactPairSupportCheckpointManifest
  manifest_for_with_audit(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      ExactPairSupportAuthorityContextAudit* audit) {
    if (!index.validated_for(cloud)) {
      throw std::invalid_argument(
          "a pair-support manifest requires the supplied cloud's Morton LBVH");
    }
    if (cloud.size() == 0U || requested_maximum_order == 0U ||
        requested_maximum_order > pair_support_maximum_requested_order) {
      throw std::invalid_argument(
          "a pair-support manifest requires a nonempty cloud and 1<=Kmax<=10");
    }
    if (audit != nullptr) {
      audit->manifest_build_count = checked_add(
          audit->manifest_build_count,
          1U,
          "the pair-support manifest build count overflows size_t");
      audit->manifest_cached = false;
    }
    ExactPairSupportCheckpointManifest manifest;
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
            "the pair-support manifest rank overflows size_t"),
        cloud.size());

    DigestWriter cloud_writer{
        "MorseHGP3D/phase9/pair-support/canonical-cloud/v1"};
    cloud_writer.size(cloud.size());
    for (std::size_t point_index = 0U;
         point_index < cloud.size();
         ++point_index) {
      const PointId point_id = checked_u64(
          point_index, "a canonical point index does not fit PointId");
      const std::array<std::uint64_t, 3> bits =
          cloud.point(point_id).canonical_input_bits();
      for (const std::uint64_t word : bits) {
        cloud_writer.u64(word);
      }
      if (audit != nullptr) {
        audit->canonical_cloud_point_hash_count = checked_add(
            audit->canonical_cloud_point_hash_count,
            1U,
            "the pair-support manifest point hash count overflows size_t");
      }
    }
    manifest.canonical_cloud_digest = cloud_writer.finalize();

    DigestWriter lbvh_writer{
        "MorseHGP3D/phase9/pair-support/morton-lbvh/v1"};
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
        audit->lbvh_leaf_hash_count = checked_add(
            audit->lbvh_leaf_hash_count,
            1U,
            "the pair-support manifest leaf hash count overflows size_t");
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
        audit->lbvh_node_hash_count = checked_add(
            audit->lbvh_node_hash_count,
            1U,
            "the pair-support manifest node hash count overflows size_t");
      }
    }
    manifest.lbvh_digest = lbvh_writer.finalize();

    DigestWriter semantic_writer{
        "MorseHGP3D/phase9/pair-support/checkpoint-manifest/v1"};
    semantic_writer.u32(manifest.schema_version);
    semantic_writer.u32(manifest.traversal_version);
    semantic_writer.text(pair_support_checkpoint_proof_basis);
    semantic_writer.text("reference_cpu");
    semantic_writer.text("hgp_reduced");
    semantic_writer.text("certified");
    semantic_writer.size(2U);
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

  [[nodiscard]] static ExactPairSupportCheckpointVerification
  verify_checkpoint_for(
      const spatial::MortonLbvhIndex& index,
      const spatial::CanonicalPointCloud& cloud,
      std::size_t requested_maximum_order,
      const ExactPairSupportCheckpointManifest& authority_manifest,
      const ExactPairSupportCheckpoint& checkpoint) {
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();
    ExactPairSupportStreamBuilder validator{
        index,
        cloud,
        requested_maximum_order,
        ExactPairSupportStreamBudget{
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

  struct ProductRectangle {
    std::uint64_t x_begin{};
    std::uint64_t x_end{};
    std::uint64_t y_begin{};
    std::uint64_t y_end{};
  };

  struct ProductRectangleSweepEvent {
    std::uint64_t x{};
    std::size_t rectangle_index{};
    bool is_start{false};
  };

  struct ActiveProductRectangle {
    std::uint64_t y_end{};
    std::size_t rectangle_index{};
  };

  struct WitnessInterval {
    std::uint64_t begin{};
    std::uint64_t end{};
  };

  [[nodiscard]] static bool product_rectangles_are_disjoint(
      const std::vector<ProductRectangle>& rectangles,
      ExactPairSupportCheckpointVerification::ValidationAudit& audit) {
    std::vector<ProductRectangleSweepEvent> events;
    events.reserve(checked_multiply(
        rectangles.size(),
        2U,
        "the pair-support rectangle sweep event count overflows size_t"));
    for (std::size_t rectangle_index = 0U;
         rectangle_index < rectangles.size();
         ++rectangle_index) {
      const ProductRectangle& rectangle = rectangles[rectangle_index];
      if (rectangle.x_begin >= rectangle.x_end ||
          rectangle.y_begin >= rectangle.y_end) {
        return false;
      }
      events.push_back(ProductRectangleSweepEvent{
          rectangle.x_begin, rectangle_index, true});
      events.push_back(ProductRectangleSweepEvent{
          rectangle.x_end, rectangle_index, false});
    }
    std::sort(
        events.begin(),
        events.end(),
        [&rectangles](const ProductRectangleSweepEvent& left,
                      const ProductRectangleSweepEvent& right) {
          if (left.x != right.x) {
            return left.x < right.x;
          }
          // Half-open rectangles that only touch at x are disjoint.
          if (left.is_start != right.is_start) {
            return !left.is_start;
          }
          const ProductRectangle& left_rectangle =
              rectangles[left.rectangle_index];
          const ProductRectangle& right_rectangle =
              rectangles[right.rectangle_index];
          if (left_rectangle.y_begin != right_rectangle.y_begin) {
            return left_rectangle.y_begin < right_rectangle.y_begin;
          }
          return left.rectangle_index < right.rectangle_index;
        });

    // Until the first contradiction, active y intervals are disjoint.  This
    // makes predecessor/successor checks sufficient and keeps the sweep
    // O(F log F) with O(F) transient storage.
    std::map<std::uint64_t, ActiveProductRectangle> active;
    for (const ProductRectangleSweepEvent& event : events) {
      audit.frontier_active_set_operation_count = checked_add(
          audit.frontier_active_set_operation_count,
          1U,
          "the checkpoint frontier active-set operation count overflows size_t");
      const ProductRectangle& rectangle =
          rectangles[event.rectangle_index];
      if (!event.is_start) {
        const auto found = active.find(rectangle.y_begin);
        if (found == active.end() ||
            found->second.y_end != rectangle.y_end ||
            found->second.rectangle_index != event.rectangle_index) {
          return false;
        }
        active.erase(found);
        continue;
      }

      const auto successor = active.lower_bound(rectangle.y_begin);
      if (successor != active.end()) {
        audit.frontier_neighbor_test_count = checked_add(
            audit.frontier_neighbor_test_count,
            1U,
            "the checkpoint frontier neighbor count overflows size_t");
        if (successor->first < rectangle.y_end) {
          return false;
        }
      }
      if (successor != active.begin()) {
        audit.frontier_neighbor_test_count = checked_add(
            audit.frontier_neighbor_test_count,
            1U,
            "the checkpoint frontier neighbor count overflows size_t");
        const auto predecessor = std::prev(successor);
        if (predecessor->second.y_end > rectangle.y_begin) {
          return false;
        }
      }
      const auto [inserted, unique] = active.emplace(
          rectangle.y_begin,
          ActiveProductRectangle{
              rectangle.y_end, event.rectangle_index});
      static_cast<void>(inserted);
      if (!unique) {
        return false;
      }
    }
    return active.empty();
  }

  [[nodiscard]] static bool witness_intervals_are_disjoint(
      std::vector<WitnessInterval> intervals,
      ExactPairSupportCheckpointVerification::ValidationAudit& audit) {
    std::sort(
        intervals.begin(),
        intervals.end(),
        [](const WitnessInterval& left, const WitnessInterval& right) {
          if (left.begin != right.begin) {
            return left.begin < right.begin;
          }
          return left.end < right.end;
        });
    bool initialized = false;
    std::uint64_t maximum_end = 0U;
    for (const WitnessInterval& interval : intervals) {
      if (interval.begin >= interval.end) {
        return false;
      }
      if (initialized) {
        audit.witness_adjacent_interval_test_count = checked_add(
            audit.witness_adjacent_interval_test_count,
            1U,
            "the checkpoint witness adjacency count overflows size_t");
        if (interval.begin < maximum_end) {
          return false;
        }
      }
      initialized = true;
      maximum_end = std::max(maximum_end, interval.end);
    }
    return true;
  }

  [[nodiscard]] ExactPairSupportStreamAudit audit_snapshot() const {
    ExactPairSupportStreamAudit snapshot = result_.audit;
    snapshot.remaining_frontier_pair_count = 0U;
    for (const ExactPairSupportFrontierEntry& entry : frontier_) {
      snapshot.remaining_frontier_pair_count = checked_add(
          snapshot.remaining_frontier_pair_count,
          entry_pair_count(entry),
          "the checkpoint remaining-pair count overflows size_t");
    }
    snapshot.pair_partition_accounting_certified =
        checked_add(
            snapshot.resolved_pair_count,
            snapshot.remaining_frontier_pair_count,
            "the checkpoint pair accounting overflows size_t") ==
            snapshot.total_pair_count &&
        snapshot.resolved_pair_count == checked_add(
            snapshot.rank_pruned_pair_count,
            snapshot.leaf_pair_classification_count,
            "the checkpoint terminal accounting overflows size_t");
    return snapshot;
  }

  [[nodiscard]] ExactPairSupportCheckpointVerification
  checkpoint_verification(
      const ExactPairSupportCheckpointManifest& authority_manifest,
      const ExactPairSupportCheckpoint& checkpoint) const {
    ExactPairSupportCheckpointVerification verification;
    verification.manifest_matches_authorities =
        checkpoint.manifest == authority_manifest;
    verification.checksum_matches_payload =
        checkpoint.checkpoint_digest == checkpoint_digest(checkpoint);
    if (!verification.manifest_matches_authorities ||
        !verification.checksum_matches_payload) {
      return verification;
    }

    std::size_t remaining_pair_count = 0U;
    bool frontier_valid = true;
    try {
      std::vector<ProductRectangle> rectangles;
      rectangles.reserve(checked_multiply(
          checkpoint.frontier.size(),
          2U,
          "the checkpoint frontier rectangle count overflows size_t"));
      for (std::size_t index = 0U;
           index < checkpoint.frontier.size();
           ++index) {
        const ExactPairSupportFrontierEntry& entry =
            checkpoint.frontier[index];
        static_cast<void>(entry_nodes(entry));
        verification.validation_audit.frontier_entry_validation_count =
            checked_add(
                verification.validation_audit
                    .frontier_entry_validation_count,
                1U,
                "the checkpoint frontier validation count overflows size_t");
        remaining_pair_count = checked_add(
            remaining_pair_count,
            entry_pair_count(entry),
            "the checkpoint frontier pair count overflows size_t");
        rectangles.push_back(ProductRectangle{
            entry.first_leaf_begin,
            entry.first_leaf_end,
            entry.second_leaf_begin,
            entry.second_leaf_end});
        if (entry.self_product == 0U) {
          rectangles.push_back(ProductRectangle{
              entry.second_leaf_begin,
              entry.second_leaf_end,
              entry.first_leaf_begin,
              entry.first_leaf_end});
        }
      }
      verification.validation_audit.frontier_rectangle_count =
          rectangles.size();
      verification.validation_audit.frontier_sweep_event_count =
          checked_multiply(
              rectangles.size(),
              2U,
              "the checkpoint frontier sweep count overflows size_t");
      frontier_valid = product_rectangles_are_disjoint(
          rectangles, verification.validation_audit);
    } catch (const std::exception&) {
      frontier_valid = false;
    }
    verification.frontier_locally_valid = frontier_valid;

    bool pending_valid = frontier_valid;
    bool pending_rank_search_started = false;
    std::size_t pending_active_witness_entry_count = 0U;
    std::size_t pending_strict_receipt_count = 0U;
    std::size_t pending_strict_receipt_point_count = 0U;
    std::size_t pending_self_expansion_count = 0U;
    if (checkpoint.pending_product.has_value()) {
      if (checkpoint.frontier.empty() ||
          checkpoint.pending_product->product != checkpoint.frontier.back()) {
        pending_valid = false;
      } else {
        try {
          const ExactPairSupportPendingProduct& pending =
              *checkpoint.pending_product;
          const auto [first_node_index, second_node_index] =
              entry_nodes(pending.product);
          const Node& first = node(first_node_index);
          const Node& second = node(second_node_index);
          const bool empty_rank_payload =
              !pending.rank_search_started &&
              pending.witness_frontier.empty() &&
              pending.strict_witness_receipts.empty() &&
              !pending.deferred_expansion_node.has_value() &&
              pending.strict_witness_point_count == 0U;
          switch (pending.stage) {
            case ExactPairSupportPendingStage::rank_search: {
              if (first_node_index == second_node_index ||
                  (first.is_leaf() && second.is_leaf()) ||
                  (!pending.rank_search_started && !empty_rank_payload)) {
                pending_valid = false;
                break;
              }
              std::vector<ExactPairSupportWitnessNodeEntry> active_entries =
                  pending.witness_frontier;
              if (pending.deferred_expansion_node.has_value()) {
                if (node(witness_node_index(
                             *pending.deferred_expansion_node))
                        .is_leaf()) {
                  pending_valid = false;
                }
                active_entries.push_back(*pending.deferred_expansion_node);
              }
              pending_rank_search_started = pending.rank_search_started;
              pending_active_witness_entry_count = active_entries.size();
              pending_strict_receipt_count =
                  pending.strict_witness_receipts.size();
              verification.validation_audit
                  .active_witness_entry_validation_count =
                  active_entries.size();
              verification.validation_audit
                  .strict_witness_receipt_validation_count =
                  pending.strict_witness_receipts.size();
              std::vector<WitnessInterval> witness_intervals;
              witness_intervals.reserve(checked_add(
                  active_entries.size(),
                  pending.strict_witness_receipts.size(),
                  "the checkpoint witness interval count overflows size_t"));
              std::size_t receipt_point_count = 0U;
              const spatial::ExactDyadicAabb3 first_box =
                  node_box(first_node_index);
              const spatial::ExactDyadicAabb3 second_box =
                  node_box(second_node_index);
              for (std::size_t receipt_index = 0U;
                   receipt_index < pending.strict_witness_receipts.size();
                   ++receipt_index) {
                const ExactPairSupportWitnessNodeEntry& receipt =
                    pending.strict_witness_receipts[receipt_index];
                const std::size_t receipt_node_index =
                    witness_node_index(receipt);
                const Node& receipt_node = node(receipt_node_index);
                verification.validation_audit
                    .strict_receipt_geometry_recertification_count =
                    checked_add(
                        verification.validation_audit
                            .strict_receipt_geometry_recertification_count,
                        1U,
                        "the checkpoint receipt recertification count overflows size_t");
                if (node_range_intersects(receipt_node, first) ||
                    node_range_intersects(receipt_node, second) ||
                    exact_diametral_phi_aabb_maximum(
                        first_box,
                        second_box,
                        node_box(receipt_node_index))
                            .maximum_phi.sign() >= 0) {
                  pending_valid = false;
                }
                receipt_point_count = checked_add(
                    receipt_point_count,
                    receipt_node.leaf_end - receipt_node.leaf_begin,
                    "the checkpoint witness receipt count overflows size_t");
                witness_intervals.push_back(WitnessInterval{
                    receipt.leaf_begin, receipt.leaf_end});
              }
              for (const ExactPairSupportWitnessNodeEntry& active :
                   active_entries) {
                static_cast<void>(witness_node_index(active));
                witness_intervals.push_back(WitnessInterval{
                    active.leaf_begin, active.leaf_end});
              }
              verification.validation_audit.witness_interval_count =
                  witness_intervals.size();
              if (!witness_intervals_are_disjoint(
                      std::move(witness_intervals),
                      verification.validation_audit)) {
                pending_valid = false;
              }
              const std::size_t witness_threshold =
                  checkpoint.manifest.maximum_relevant_closed_rank - 1U;
              if (receipt_point_count !=
                      pending.strict_witness_point_count ||
                  receipt_point_count >= witness_threshold ||
                  (pending.rank_search_started && active_entries.empty())) {
                pending_valid = false;
              }
              pending_strict_receipt_point_count = receipt_point_count;
              break;
            }
            case ExactPairSupportPendingStage::expand_product:
              if (!empty_rank_payload ||
                  (first.is_leaf() && second.is_leaf())) {
                pending_valid = false;
              } else if (first_node_index == second_node_index) {
                pending_self_expansion_count = 1U;
              }
              break;
            case ExactPairSupportPendingStage::classify_leaf:
              if (!empty_rank_payload || first_node_index == second_node_index ||
                  !first.is_leaf() || !second.is_leaf()) {
                pending_valid = false;
              }
              break;
            default:
              pending_valid = false;
              break;
          }
        } catch (const std::exception&) {
          pending_valid = false;
        }
      }
    }
    verification.pending_product_locally_valid = pending_valid;

    const ExactPairSupportStreamAudit& audit = checkpoint.cumulative_audit;
    bool audit_valid = false;
    try {
      const std::size_t terminal_count = checked_add(
          audit.rank_pruned_pair_count,
          audit.leaf_pair_classification_count,
          "the checkpoint terminal count overflows size_t");
      const std::size_t classified_categories = checked_add(
          checked_add(
              audit.accepted_event_count,
              audit.relevant_extra_shell_diagnostic_count,
              "the checkpoint record count overflows size_t"),
          audit.above_rank_pair_count,
          "the checkpoint leaf category count overflows size_t");
      const std::size_t completed_product_visits = checked_add(
          checked_add(
              audit.support_product_expansion_count,
              audit.diagonal_leaf_discard_count,
              "the checkpoint product visit count overflows size_t"),
          checked_add(
              audit.rank_pruned_product_count,
              audit.leaf_pair_classification_count,
              "the checkpoint product visit count overflows size_t"),
          "the checkpoint product visit count overflows size_t");
      const std::size_t expected_product_visits = checked_add(
          completed_product_visits,
          checkpoint.pending_product.has_value() ? 1U : 0U,
          "the checkpoint active product count overflows size_t");
      const std::size_t expected_phi_bound_count = checked_add(
          audit.strict_interior_witness_subtree_count,
          audit.exact_anchor_ball_minimum_aabb_bound_count,
          "the checkpoint phi-bound partition overflows size_t");
      const std::size_t expected_anchor_bound_count = checked_add(
          audit.certified_anchor_noninterior_subtree_count,
          audit.equality_or_positive_bound_descent_count,
          "the checkpoint anchor-bound partition overflows size_t");
      const std::size_t expected_closed_ball_node_visits = checked_add(
          audit.exact_closed_ball_maximum_aabb_bound_count,
          audit.closed_ball_bulk_exterior_subtree_count,
          "the checkpoint closed-ball node partition overflows size_t");
      const std::size_t expected_point_classifications = checked_add(
          checked_add(
              audit.closed_ball_bulk_interior_point_count,
              audit.closed_ball_bulk_exterior_point_count,
              "the checkpoint bulk point classification count overflows size_t"),
          audit.exact_point_distance_evaluation_count,
          "the checkpoint point classification count overflows size_t");
      audit_valid =
          audit.total_pair_count ==
              checked_unordered_pair_count(cloud_.size()) &&
          audit.remaining_frontier_pair_count == remaining_pair_count &&
          checked_add(
              audit.resolved_pair_count,
              remaining_pair_count,
              "the checkpoint accounting sum overflows size_t") ==
              audit.total_pair_count &&
          audit.resolved_pair_count == terminal_count &&
          audit.leaf_pair_classification_count == classified_categories &&
          audit.support_product_visit_count == expected_product_visits &&
          audit.support_product_expansion_count == checked_add(
              audit.self_product_expansion_count,
              audit.cross_product_expansion_count,
              "the checkpoint expansion sum overflows size_t") &&
          audit.diagonal_product_rank_search_skip_count == checked_add(
              audit.self_product_expansion_count,
              pending_self_expansion_count,
              "the checkpoint diagonal-skip count overflows size_t") &&
          audit.work_unit_count == checked_add(
              audit.support_product_visit_count,
              audit.witness_node_visit_count,
              "the checkpoint work sum overflows size_t") &&
          audit.exact_phi_aabb_bound_count == expected_phi_bound_count &&
          audit.exact_anchor_ball_minimum_aabb_bound_count ==
              expected_anchor_bound_count &&
          audit.certified_anchor_shell_tangent_subtree_count <=
              audit.certified_anchor_noninterior_subtree_count &&
          audit.rank_pruned_product_count <=
              audit.rank_prune_search_count &&
          audit.rank_prune_search_count <= checked_add(
              audit.witness_node_visit_count,
              pending_rank_search_started ? 1U : 0U,
              "the checkpoint rank-search lower work bound overflows size_t") &&
          audit.global_closed_ball_query_count ==
              audit.leaf_pair_classification_count &&
          audit.closed_ball_node_visit_count ==
              audit.exact_closed_ball_minimum_aabb_bound_count &&
          audit.closed_ball_node_visit_count ==
              expected_closed_ball_node_visits &&
          audit.point_classification_count ==
              expected_point_classifications &&
          audit.early_closed_rank_rejection_count <=
              audit.global_closed_ball_query_count &&
          audit.closed_ball_node_visit_count >=
              audit.global_closed_ball_query_count &&
          (audit.global_closed_ball_query_count == 0U ||
           audit.maximum_closed_ball_frontier_entry_count > 0U) &&
          audit.maximum_frontier_entry_count >= checkpoint.frontier.size() &&
          audit.maximum_witness_frontier_entry_count >=
              pending_active_witness_entry_count &&
          audit.strict_interior_witness_subtree_count >=
              pending_strict_receipt_count &&
          audit.strict_interior_witness_point_count >=
              pending_strict_receipt_point_count &&
          audit.exact_phi_aabb_bound_count >=
              pending_strict_receipt_count &&
          audit.witness_node_visit_count >=
              checked_add(
                  pending_strict_receipt_count,
                  checkpoint.pending_product.has_value() &&
                          checkpoint.pending_product
                              ->deferred_expansion_node.has_value()
                      ? 1U
                      : 0U,
                  "the checkpoint pending witness lower bound overflows size_t") &&
          (!pending_rank_search_started ||
           audit.rank_prune_search_count > 0U) &&
          checkpoint.output_record_count == checked_add(
              audit.accepted_event_count,
              audit.relevant_extra_shell_diagnostic_count,
              "the checkpoint output record count overflows size_t") &&
          audit.pair_partition_accounting_certified;
    } catch (const std::exception&) {
      audit_valid = false;
    }
    verification.required_audit_identities_hold = audit_valid;
    verification.quasi_linear_structure_validation_certified =
        verification.frontier_locally_valid &&
        verification.pending_product_locally_valid &&
        verification.validation_audit.frontier_sweep_event_count ==
            checked_multiply(
                verification.validation_audit.frontier_rectangle_count,
                2U,
                "the checkpoint certified sweep count overflows size_t") &&
        verification.validation_audit
                .frontier_active_set_operation_count ==
            verification.validation_audit.frontier_sweep_event_count &&
        verification.validation_audit.frontier_neighbor_test_count <=
            checked_multiply(
                verification.validation_audit.frontier_rectangle_count,
                2U,
                "the checkpoint certified neighbor count overflows size_t") &&
        verification.validation_audit.witness_interval_count ==
            checked_add(
                verification.validation_audit
                    .active_witness_entry_validation_count,
                verification.validation_audit
                    .strict_witness_receipt_validation_count,
                "the checkpoint certified witness count overflows size_t") &&
        verification.validation_audit
                .witness_adjacent_interval_test_count ==
            (verification.validation_audit.witness_interval_count == 0U
                 ? 0U
                 : verification.validation_audit.witness_interval_count -
                       1U) &&
        verification.validation_audit
                .strict_receipt_geometry_recertification_count ==
            verification.validation_audit
                .strict_witness_receipt_validation_count;
    verification.integrity_verified =
        verification.manifest_matches_authorities &&
        verification.checksum_matches_payload &&
        verification.frontier_locally_valid &&
        verification.pending_product_locally_valid &&
        verification.required_audit_identities_hold &&
        verification.quasi_linear_structure_validation_certified;
    return verification;
  }

  void validate_inputs(
      std::size_t requested_maximum_order,
      const ExactPairSupportStreamBudget& budget) const {
    if (!index_.validated_for(cloud_)) {
      throw std::invalid_argument(
          "the pair-support stream requires the supplied cloud's Morton LBVH");
    }
    if (cloud_.size() == 0U) {
      throw std::invalid_argument(
          "the pair-support stream requires a nonempty point cloud");
    }
    if (requested_maximum_order == 0U ||
        requested_maximum_order > pair_support_maximum_requested_order) {
      throw std::out_of_range(
          "the pair-support stream requires 1<=Kmax<=10");
    }
    static_cast<void>(checked_unordered_pair_count(cloud_.size()));
    static_cast<void>(budget);
  }

  [[nodiscard]] ExactPairSupportRequirements requirements_for(
      std::size_t requested_maximum_order) const {
    ExactPairSupportRequirements requirements;
    requirements.point_count = cloud_.size();
    requirements.requested_maximum_order = requested_maximum_order;
    requirements.effective_maximum_order =
        std::min(requested_maximum_order, cloud_.size());
    requirements.maximum_relevant_closed_rank = std::min(
        checked_add(
            requirements.effective_maximum_order,
            1U,
            "the pair-support relevant closed rank overflows size_t"),
        cloud_.size());
    return requirements;
  }

  [[nodiscard]] const Node& node(std::size_t node_index) const {
    if (node_index >= index_.nodes_.size()) {
      throw std::logic_error(
          "a pair-support frontier references an invalid LBVH node");
    }
    return index_.nodes_[node_index];
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

  [[nodiscard]] exact::ExactLevel node_squared_diagonal(
      std::size_t node_index) const {
    const Node& current = node(node_index);
    exact::ExactRational squared;
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const exact::ExactRational lower =
          cloud_.point(current.lower_point_ids[axis]).coordinate(axis);
      const exact::ExactRational upper =
          cloud_.point(current.upper_point_ids[axis]).coordinate(axis);
      const exact::ExactRational delta = upper - lower;
      squared = squared + delta * delta;
    }
    return exact::ExactLevel{std::move(squared)};
  }

  [[nodiscard]] ExactPairSupportFrontierEntry make_entry(
      std::size_t first_node_index,
      std::size_t second_node_index) const {
    const Node* first = &node(first_node_index);
    const Node* second = &node(second_node_index);
    if (first_node_index != second_node_index &&
        second->leaf_begin < first->leaf_begin) {
      std::swap(first_node_index, second_node_index);
      std::swap(first, second);
    }
    const bool self_product = first_node_index == second_node_index;
    if (!self_product && first->leaf_end > second->leaf_begin) {
      throw std::logic_error(
          "a pair-support cross product has overlapping Morton ranges");
    }
    return ExactPairSupportFrontierEntry{
        checked_u64(
            first_node_index,
            "a pair-support node index does not fit uint64"),
        checked_u64(
            second_node_index,
            "a pair-support node index does not fit uint64"),
        checked_u64(
            first->leaf_begin,
            "a pair-support Morton range does not fit uint64"),
        checked_u64(
            first->leaf_end,
            "a pair-support Morton range does not fit uint64"),
        checked_u64(
            second->leaf_begin,
            "a pair-support Morton range does not fit uint64"),
        checked_u64(
            second->leaf_end,
            "a pair-support Morton range does not fit uint64"),
        static_cast<std::uint8_t>(self_product ? 1U : 0U)};
  }

  [[nodiscard]] std::pair<std::size_t, std::size_t> entry_nodes(
      const ExactPairSupportFrontierEntry& entry) const {
    const std::size_t first_index = checked_size(
        entry.first_node_index,
        "a pair-support frontier node index does not fit size_t");
    const std::size_t second_index = checked_size(
        entry.second_node_index,
        "a pair-support frontier node index does not fit size_t");
    const Node& first = node(first_index);
    const Node& second = node(second_index);
    const bool self_product = entry.self_product == 1U;
    if (entry.self_product > 1U ||
        self_product != (first_index == second_index) ||
        entry.first_leaf_begin != checked_u64(
            first.leaf_begin,
            "a pair-support Morton range does not fit uint64") ||
        entry.first_leaf_end != checked_u64(
            first.leaf_end,
            "a pair-support Morton range does not fit uint64") ||
        entry.second_leaf_begin != checked_u64(
            second.leaf_begin,
            "a pair-support Morton range does not fit uint64") ||
        entry.second_leaf_end != checked_u64(
            second.leaf_end,
            "a pair-support Morton range does not fit uint64") ||
        (!self_product && first.leaf_end > second.leaf_begin)) {
      throw std::logic_error(
          "a pair-support frontier entry contradicts its LBVH nodes");
    }
    return {first_index, second_index};
  }

  [[nodiscard]] ExactPairSupportWitnessNodeEntry make_witness_entry(
      std::size_t node_index) const {
    const Node& current = node(node_index);
    return ExactPairSupportWitnessNodeEntry{
        checked_u64(
            node_index,
            "a pair-support witness node index does not fit uint64"),
        checked_u64(
            current.leaf_begin,
            "a pair-support witness range does not fit uint64"),
        checked_u64(
            current.leaf_end,
            "a pair-support witness range does not fit uint64")};
  }

  [[nodiscard]] std::size_t witness_node_index(
      const ExactPairSupportWitnessNodeEntry& entry) const {
    const std::size_t node_index = checked_size(
        entry.node_index,
        "a pair-support witness node index does not fit size_t");
    const Node& current = node(node_index);
    if (entry.leaf_begin != checked_u64(
                                current.leaf_begin,
                                "a witness range does not fit uint64") ||
        entry.leaf_end != checked_u64(
                              current.leaf_end,
                              "a witness range does not fit uint64")) {
      throw std::logic_error(
          "a pair-support witness entry contradicts its LBVH node");
    }
    return node_index;
  }

  [[nodiscard]] std::size_t entry_pair_count(
      const ExactPairSupportFrontierEntry& entry) const {
    const auto [first_index, second_index] = entry_nodes(entry);
    const Node& first = node(first_index);
    const Node& second = node(second_index);
    const std::size_t first_size = first.leaf_end - first.leaf_begin;
    if (first_index == second_index) {
      return checked_unordered_pair_count(first_size);
    }
    const std::size_t second_size = second.leaf_end - second.leaf_begin;
    return checked_multiply(
        first_size,
        second_size,
        "a pair-support product coverage overflows size_t");
  }

  [[nodiscard]] static std::size_t consumed_since(
      std::size_t current,
      std::size_t origin,
      std::string_view message) {
    if (current < origin) {
      throw std::logic_error(std::string{message});
    }
    return current - origin;
  }

  [[nodiscard]] bool consume_work_unit() {
    if (consumed_since(
            result_.audit.work_unit_count,
            chunk_audit_origin_.work_unit_count,
            "the pair-support work audit moved backwards") >=
        result_.budget.maximum_work_unit_count) {
      stop(ExactPairSupportStopReason::work_unit_limit);
      return false;
    }
    result_.audit.work_unit_count = checked_add(
        result_.audit.work_unit_count,
        1U,
        "the pair-support work count overflows size_t");
    return true;
  }

  void stop(ExactPairSupportStopReason reason) {
    if (!stopped_) {
      stopped_ = true;
      result_.status = ExactPairSupportStreamStatus::budget_exhausted;
      result_.stop_reason = reason;
    }
  }

  [[nodiscard]] bool node_range_intersects(
      const Node& query_node,
      const Node& support_node) const noexcept {
    return query_node.leaf_begin < support_node.leaf_end &&
           support_node.leaf_begin < query_node.leaf_end;
  }

  [[nodiscard]] RankSearchOutcome continue_rank_prune_search() {
    if (!pending_product_.has_value() ||
        pending_product_->stage != ExactPairSupportPendingStage::rank_search ||
        pending_product_->product != frontier_.back()) {
      throw std::logic_error(
          "a pair-support rank search has no matching active product");
    }
    ExactPairSupportPendingProduct& pending = *pending_product_;
    const auto [first_node_index, second_node_index] =
        entry_nodes(pending.product);
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    const std::size_t excluded_count = first_node_index == second_node_index
                                           ? first.leaf_end - first.leaf_begin
                                           : checked_add(
                                                 first.leaf_end - first.leaf_begin,
                                                 second.leaf_end - second.leaf_begin,
                                                 "pair-support exclusion count overflows size_t");
    if (excluded_count > cloud_.size()) {
      throw std::logic_error(
          "pair-support ranges exclude more points than the cloud contains");
    }
    const std::size_t witness_threshold =
        result_.requirements.maximum_relevant_closed_rank - 1U;
    if (cloud_.size() - excluded_count < witness_threshold) {
      return RankSearchOutcome::keep;
    }
    if (!pending.rank_search_started) {
      if (result_.budget.maximum_auxiliary_frontier_entry_count == 0U) {
        stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
        return RankSearchOutcome::budget_exhausted;
      }
      pending.rank_search_started = true;
      pending.witness_frontier.push_back(
          make_witness_entry(index_.root_index_));
      result_.audit.rank_prune_search_count = checked_add(
          result_.audit.rank_prune_search_count,
          1U,
          "the pair-support rank-search count overflows size_t");
      result_.audit.maximum_witness_frontier_entry_count = std::max(
          result_.audit.maximum_witness_frontier_entry_count,
          pending.witness_frontier.size());
    }
    const spatial::ExactDyadicAabb3 first_box = node_box(first_node_index);
    const spatial::ExactDyadicAabb3 second_box = node_box(second_node_index);
    const PointId first_anchor_id =
        index_.leaves_[first.leaf_begin].point_id;
    const PointId second_anchor_id =
        index_.leaves_[second.leaf_begin].point_id;
    const exact::CircumcenterResult anchor_sphere = exact::circumcenter(
        cloud_.point(first_anchor_id), cloud_.point(second_anchor_id));
    if (anchor_sphere.kind() != exact::CircumcenterKind::unique ||
        !anchor_sphere.center().has_value() ||
        !anchor_sphere.squared_level().has_value()) {
      throw std::logic_error(
          "two support-product anchor points did not define a unique sphere");
    }
    while (!pending.witness_frontier.empty() ||
           pending.deferred_expansion_node.has_value()) {
      if (pending.deferred_expansion_node.has_value()) {
        if (!can_add_within(
                pending.witness_frontier.size(),
                2U,
                result_.budget.maximum_auxiliary_frontier_entry_count)) {
          stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
          return RankSearchOutcome::budget_exhausted;
        }
        const std::size_t deferred_index = witness_node_index(
            *pending.deferred_expansion_node);
        const Node& deferred = node(deferred_index);
        if (deferred.is_leaf()) {
          throw std::logic_error(
              "a deferred witness expansion references a leaf");
        }
        pending.witness_frontier.push_back(
            make_witness_entry(deferred.right_child));
        pending.witness_frontier.push_back(
            make_witness_entry(deferred.left_child));
        pending.deferred_expansion_node.reset();
        result_.audit.maximum_witness_frontier_entry_count = std::max(
            result_.audit.maximum_witness_frontier_entry_count,
            pending.witness_frontier.size());
        continue;
      }
      if (!consume_work_unit()) {
        return RankSearchOutcome::budget_exhausted;
      }
      const ExactPairSupportWitnessNodeEntry query_entry =
          pending.witness_frontier.back();
      pending.witness_frontier.pop_back();
      const std::size_t query_node_index = witness_node_index(query_entry);
      result_.audit.witness_node_visit_count = checked_add(
          result_.audit.witness_node_visit_count,
          1U,
          "the pair-support witness-node count overflows size_t");
      const Node& query_node = node(query_node_index);
      const bool overlaps_support =
          node_range_intersects(query_node, first) ||
          (first_node_index != second_node_index &&
           node_range_intersects(query_node, second));
      if (!overlaps_support) {
        const ExactDiametralPhiAabbMaximum maximum =
            exact_diametral_phi_aabb_maximum(
                first_box,
                second_box,
                node_box(query_node_index));
        result_.audit.exact_phi_aabb_bound_count = checked_add(
            result_.audit.exact_phi_aabb_bound_count,
            1U,
            "the pair-support phi-bound count overflows size_t");
        if (maximum.maximum_phi.sign() < 0) {
          const std::size_t subtree_size =
              query_node.leaf_end - query_node.leaf_begin;
          pending.strict_witness_point_count = checked_add(
              pending.strict_witness_point_count,
              subtree_size,
              "the pair-support witness count overflows size_t");
          pending.strict_witness_receipts.push_back(query_entry);
          result_.audit.strict_interior_witness_subtree_count = checked_add(
              result_.audit.strict_interior_witness_subtree_count,
              1U,
              "the pair-support witness-subtree count overflows size_t");
          result_.audit.strict_interior_witness_point_count = checked_add(
              result_.audit.strict_interior_witness_point_count,
              subtree_size,
              "the pair-support witness-point audit overflows size_t");
          if (pending.strict_witness_point_count >= witness_threshold) {
            return RankSearchOutcome::prune;
          }
          continue;
        }
        const exact::ExactLevel anchor_minimum_distance =
            index_.minimum_squared_distance_to_node(
                cloud_, query_node_index, *anchor_sphere.center());
        result_.audit.exact_anchor_ball_minimum_aabb_bound_count = checked_add(
            result_.audit.exact_anchor_ball_minimum_aabb_bound_count,
            1U,
            "the pair-support anchor-bound count overflows size_t");
        if (anchor_minimum_distance >= *anchor_sphere.squared_level()) {
          const std::size_t subtree_size =
              query_node.leaf_end - query_node.leaf_begin;
          if (anchor_minimum_distance == *anchor_sphere.squared_level()) {
            result_.audit.certified_anchor_shell_tangent_subtree_count =
                checked_add(
                    result_.audit
                        .certified_anchor_shell_tangent_subtree_count,
                    1U,
                    "the pair-support anchor-tangent count overflows size_t");
          }
          result_.audit.certified_anchor_noninterior_subtree_count =
              checked_add(
                  result_.audit.certified_anchor_noninterior_subtree_count,
                  1U,
                  "the pair-support anchor-subtree count overflows size_t");
          result_.audit.certified_anchor_noninterior_point_count =
              checked_add(
                  result_.audit.certified_anchor_noninterior_point_count,
                  subtree_size,
                  "the pair-support anchor-point count overflows size_t");
          continue;
        }
        result_.audit.equality_or_positive_bound_descent_count = checked_add(
            result_.audit.equality_or_positive_bound_descent_count,
            1U,
            "the pair-support nonnegative-bound count overflows size_t");
      }
      if (query_node.is_leaf()) {
        continue;
      }
      if (!can_add_within(
              pending.witness_frontier.size(),
              2U,
              result_.budget.maximum_auxiliary_frontier_entry_count)) {
        pending.deferred_expansion_node = query_entry;
        result_.audit.maximum_witness_frontier_entry_count = std::max(
            result_.audit.maximum_witness_frontier_entry_count,
            checked_add(
                pending.witness_frontier.size(),
                1U,
                "the deferred witness frontier size overflows"));
        stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
        return RankSearchOutcome::budget_exhausted;
      }
      pending.witness_frontier.push_back(
          make_witness_entry(query_node.right_child));
      pending.witness_frontier.push_back(
          make_witness_entry(query_node.left_child));
      result_.audit.maximum_witness_frontier_entry_count = std::max(
          result_.audit.maximum_witness_frontier_entry_count,
          pending.witness_frontier.size());
    }
    return RankSearchOutcome::keep;
  }

  [[nodiscard]] bool leaf_preflight() {
    const std::size_t emitted_record_count = checked_add(
        result_.events.size(),
        result_.relevant_extra_shell_diagnostics.size(),
        "the pair-support emitted-record count overflows size_t");
    if (emitted_record_count >=
        result_.budget.maximum_emitted_record_count) {
      stop(ExactPairSupportStopReason::emitted_record_limit);
      return false;
    }
    const std::size_t maximum_record_references = checked_add(
        result_.requirements.maximum_relevant_closed_rank,
        1U,
        "the pair-support record reference bound overflows size_t");
    if (!can_add_within(
            consumed_since(
                result_.audit.emitted_point_id_reference_count,
                chunk_audit_origin_.emitted_point_id_reference_count,
                "the pair-support emitted-reference audit moved backwards"),
            maximum_record_references,
            result_.budget.maximum_emitted_point_id_reference_count)) {
      stop(ExactPairSupportStopReason::emitted_point_id_reference_limit);
      return false;
    }
    if (consumed_since(
            result_.audit.global_closed_ball_query_count,
            chunk_audit_origin_.global_closed_ball_query_count,
            "the pair-support query audit moved backwards") >=
        result_.budget.maximum_global_closed_ball_query_count) {
      stop(ExactPairSupportStopReason::global_closed_ball_query_limit);
      return false;
    }
    if (!can_add_within(
            consumed_since(
                result_.audit.point_classification_count,
                chunk_audit_origin_.point_classification_count,
                "the pair-support classification audit moved backwards"),
            cloud_.size(),
            result_.budget.maximum_point_classification_count)) {
      stop(ExactPairSupportStopReason::point_classification_limit);
      return false;
    }
    const std::size_t required_closed_ball_frontier = checked_add(
        index_.build_counters().maximum_depth,
        1U,
        "the sparse closed-ball frontier bound overflows size_t");
    if (required_closed_ball_frontier >
        result_.budget.maximum_auxiliary_frontier_entry_count) {
      stop(ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
      return false;
    }
    return true;
  }

  void add_point_classifications(std::size_t count) {
    result_.audit.point_classification_count = checked_add(
        result_.audit.point_classification_count,
        count,
        "the sparse closed-ball classification count overflows size_t");
    if (consumed_since(
            result_.audit.point_classification_count,
            chunk_audit_origin_.point_classification_count,
            "the pair-support classification audit moved backwards") >
        result_.budget.maximum_point_classification_count) {
      throw std::logic_error(
          "a sparse closed-ball query exceeded its atomic classification budget");
    }
  }

  [[nodiscard]] SparseBallClassification classify_sparse_closed_ball(
      const std::array<PointId, 2>& support_ids,
      const exact::ExactCenter3& center,
      const exact::ExactLevel& squared_level) {
    SparseBallClassification classification;
    const std::size_t interior_cap =
        result_.requirements.maximum_relevant_closed_rank - 2U;
    classification.interior_ids.reserve(interior_cap);
    result_.audit.global_closed_ball_query_count = checked_add(
        result_.audit.global_closed_ball_query_count,
        1U,
        "the sparse closed-ball query count overflows size_t");
    std::vector<std::size_t> frontier{index_.root_index_};
    result_.audit.maximum_closed_ball_frontier_entry_count = std::max(
        result_.audit.maximum_closed_ball_frontier_entry_count,
        frontier.size());
    std::array<bool, 2> support_seen{false, false};
    std::size_t interior_count = 0U;
    while (!frontier.empty()) {
      const std::size_t node_index = frontier.back();
      frontier.pop_back();
      const Node& current = node(node_index);
      const std::size_t subtree_size =
          current.leaf_end - current.leaf_begin;
      result_.audit.closed_ball_node_visit_count = checked_add(
          result_.audit.closed_ball_node_visit_count,
          1U,
          "the sparse closed-ball node count overflows size_t");
      const exact::ExactLevel minimum_distance =
          index_.minimum_squared_distance_to_node(cloud_, node_index, center);
      result_.audit.exact_closed_ball_minimum_aabb_bound_count = checked_add(
          result_.audit.exact_closed_ball_minimum_aabb_bound_count,
          1U,
          "the sparse closed-ball minimum-bound count overflows size_t");
      if (minimum_distance > squared_level) {
        classification.exterior_count = checked_add(
            classification.exterior_count,
            subtree_size,
            "the sparse closed-ball exterior count overflows size_t");
        result_.audit.closed_ball_bulk_exterior_subtree_count = checked_add(
            result_.audit.closed_ball_bulk_exterior_subtree_count,
            1U,
            "the sparse closed-ball exterior-subtree count overflows size_t");
        result_.audit.closed_ball_bulk_exterior_point_count = checked_add(
            result_.audit.closed_ball_bulk_exterior_point_count,
            subtree_size,
            "the sparse closed-ball exterior-point count overflows size_t");
        add_point_classifications(subtree_size);
        continue;
      }
      const exact::ExactLevel maximum_distance =
          index_.maximum_squared_distance_to_node(cloud_, node_index, center);
      result_.audit.exact_closed_ball_maximum_aabb_bound_count = checked_add(
          result_.audit.exact_closed_ball_maximum_aabb_bound_count,
          1U,
          "the sparse closed-ball maximum-bound count overflows size_t");
      if (maximum_distance < squared_level) {
        interior_count = checked_add(
            interior_count,
            subtree_size,
            "the sparse closed-ball interior count overflows size_t");
        result_.audit.closed_ball_bulk_interior_subtree_count = checked_add(
            result_.audit.closed_ball_bulk_interior_subtree_count,
            1U,
            "the sparse closed-ball interior-subtree count overflows size_t");
        result_.audit.closed_ball_bulk_interior_point_count = checked_add(
            result_.audit.closed_ball_bulk_interior_point_count,
            subtree_size,
            "the sparse closed-ball interior-point count overflows size_t");
        add_point_classifications(subtree_size);
        if (interior_count > interior_cap) {
          result_.audit.early_closed_rank_rejection_count = checked_add(
              result_.audit.early_closed_rank_rejection_count,
              1U,
              "the sparse closed-ball early-rejection count overflows size_t");
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
                center,
                squared_level,
                cloud_.point(point_id));
        result_.audit.exact_point_distance_evaluation_count = checked_add(
            result_.audit.exact_point_distance_evaluation_count,
            1U,
            "the sparse closed-ball exact-distance count overflows size_t");
        add_point_classifications(1U);
        switch (point_classification.location()) {
          case exact::SpherePointLocation::strictly_inside:
            interior_count = checked_add(
                interior_count,
                1U,
                "the sparse closed-ball interior count overflows size_t");
            if (interior_count > interior_cap) {
              result_.audit.early_closed_rank_rejection_count = checked_add(
                  result_.audit.early_closed_rank_rejection_count,
                  1U,
                  "the sparse closed-ball early-rejection count overflows size_t");
              classification.outcome = SparseBallOutcome::rank_exceeded;
              return classification;
            }
            classification.interior_ids.push_back(point_id);
            break;
          case exact::SpherePointLocation::boundary:
            classification.shell_count = checked_add(
                classification.shell_count,
                1U,
                "the sparse closed-ball shell count overflows size_t");
            if (point_id == support_ids[0]) {
              support_seen[0] = true;
            } else if (point_id == support_ids[1]) {
              support_seen[1] = true;
            } else if (!classification.canonical_extra_shell_witness_id.has_value() ||
                       point_id <
                           *classification.canonical_extra_shell_witness_id) {
              classification.canonical_extra_shell_witness_id = point_id;
            }
            break;
          case exact::SpherePointLocation::outside:
            classification.exterior_count = checked_add(
                classification.exterior_count,
                1U,
                "the sparse closed-ball exterior count overflows size_t");
            break;
        }
        continue;
      }
      frontier.push_back(current.right_child);
      frontier.push_back(current.left_child);
      if (frontier.size() >
          result_.budget.maximum_auxiliary_frontier_entry_count) {
        throw std::logic_error(
            "a sparse closed-ball DFS exceeded its preflight frontier bound");
      }
      result_.audit.maximum_closed_ball_frontier_entry_count = std::max(
          result_.audit.maximum_closed_ball_frontier_entry_count,
          frontier.size());
    }
    const std::size_t classified_count = checked_add(
        checked_add(
            interior_count,
            classification.shell_count,
            "the sparse closed-ball partition count overflows size_t"),
        classification.exterior_count,
        "the sparse closed-ball partition count overflows size_t");
    if (classified_count != cloud_.size() ||
        !support_seen[0] || !support_seen[1] ||
        classification.shell_count < 2U ||
        (classification.shell_count == 2U) !=
            !classification.canonical_extra_shell_witness_id.has_value() ||
        interior_count != classification.interior_ids.size()) {
      throw std::logic_error(
          "a sparse closed-ball traversal did not close its exact partition");
    }
    std::sort(
        classification.interior_ids.begin(),
        classification.interior_ids.end());
    classification.outcome = SparseBallOutcome::complete;
    return classification;
  }

  [[nodiscard]] bool classify_leaf_pair(
      std::size_t first_node_index,
      std::size_t second_node_index) {
    if (!leaf_preflight()) {
      return false;
    }
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (!first.is_leaf() || !second.is_leaf() ||
        first_node_index == second_node_index) {
      throw std::logic_error(
          "a pair-support leaf classifier received a nonterminal product");
    }
    std::array<PointId, 2> support_ids{
        index_.leaves_[first.leaf_begin].point_id,
        index_.leaves_[second.leaf_begin].point_id};
    if (support_ids[1] < support_ids[0]) {
      std::swap(support_ids[0], support_ids[1]);
    }
    const exact::CircumcenterResult sphere = exact::circumcenter(
        cloud_.point(support_ids[0]),
        cloud_.point(support_ids[1]));
    if (sphere.kind() != exact::CircumcenterKind::unique ||
        !sphere.center().has_value() ||
        !sphere.squared_level().has_value()) {
      throw std::logic_error(
          "two canonical distinct points did not define a unique sphere");
    }
    SparseBallClassification classification = classify_sparse_closed_ball(
        support_ids,
        *sphere.center(),
        *sphere.squared_level());
    frontier_.pop_back();
    result_.audit.leaf_pair_classification_count = checked_add(
        result_.audit.leaf_pair_classification_count,
        1U,
        "the pair-support leaf-classification count overflows size_t");
    result_.audit.resolved_pair_count = checked_add(
        result_.audit.resolved_pair_count,
        1U,
        "the pair-support resolved-pair count overflows size_t");
    if (classification.outcome == SparseBallOutcome::rank_exceeded) {
      result_.audit.above_rank_pair_count = checked_add(
          result_.audit.above_rank_pair_count,
          1U,
          "the pair-support above-rank count overflows size_t");
      return true;
    }
    const std::size_t observed_closed_rank = checked_add(
        classification.interior_ids.size(),
        classification.shell_count,
        "the pair-support observed rank overflows size_t");
    const std::size_t minimum_possible_closed_rank = checked_add(
        classification.interior_ids.size(),
        2U,
        "the pair-support minimum rank overflows size_t");
    if (minimum_possible_closed_rank >
        result_.requirements.maximum_relevant_closed_rank) {
      throw std::logic_error(
          "a complete sparse shell escaped its interior-rank cap");
    }
    std::size_t emitted_references = 0U;
    if (classification.shell_count == 2U) {
      ExactPairSupportEvent event;
      event.support_ids = support_ids;
      event.center = *sphere.center();
      event.squared_level = *sphere.squared_level();
      event.interior_ids = std::move(classification.interior_ids);
      event.closed_rank = observed_closed_rank;
      event.exterior_count = classification.exterior_count;
      emitted_references = checked_add(
          2U,
          event.interior_ids.size(),
          "the pair-support event reference count overflows size_t");
      output_chain_digest_ = extend_output_chain(output_chain_digest_, event);
      output_record_count_ = checked_add(
          output_record_count_,
          1U,
          "the pair-support output record count overflows size_t");
      result_.events.push_back(std::move(event));
      if (!canonical_sort_records_) {
        record_order_.push_back(
            ExactPairSupportStreamChunk::RecordKind::event);
      }
      result_.audit.accepted_event_count = checked_add(
          result_.audit.accepted_event_count,
          1U,
          "the pair-support event count overflows size_t");
    } else {
      if (!classification.canonical_extra_shell_witness_id.has_value()) {
        throw std::logic_error(
            "an extra-shell pair omitted its canonical extra witness");
      }
      ExactPairSupportExtraShellDiagnostic diagnostic;
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
      emitted_references = checked_add(
          3U,
          diagnostic.interior_ids.size(),
          "the pair-support diagnostic reference count overflows size_t");
      output_chain_digest_ =
          extend_output_chain(output_chain_digest_, diagnostic);
      output_record_count_ = checked_add(
          output_record_count_,
          1U,
          "the pair-support output record count overflows size_t");
      result_.relevant_extra_shell_diagnostics.push_back(
          std::move(diagnostic));
      if (!canonical_sort_records_) {
        record_order_.push_back(
            ExactPairSupportStreamChunk::RecordKind::
                relevant_extra_shell_diagnostic);
      }
      result_.audit.relevant_extra_shell_diagnostic_count = checked_add(
          result_.audit.relevant_extra_shell_diagnostic_count,
          1U,
          "the pair-support diagnostic count overflows size_t");
    }
    result_.audit.emitted_point_id_reference_count = checked_add(
        result_.audit.emitted_point_id_reference_count,
        emitted_references,
        "the pair-support emitted reference count overflows size_t");
    if (consumed_since(
            result_.audit.emitted_point_id_reference_count,
            chunk_audit_origin_.emitted_point_id_reference_count,
            "the pair-support emitted-reference audit moved backwards") >
        result_.budget.maximum_emitted_point_id_reference_count) {
      throw std::logic_error(
          "a pair-support record exceeded its conservative reference preflight");
    }
    return true;
  }

  [[nodiscard]] bool push_replacement_entries(
      std::vector<ExactPairSupportFrontierEntry> replacements) {
    const std::size_t base_size = frontier_.size() - 1U;
    const std::size_t new_size = checked_add(
        base_size,
        replacements.size(),
        "the pair-support frontier size overflows size_t");
    if (new_size > result_.budget.maximum_frontier_entry_count) {
      stop(ExactPairSupportStopReason::frontier_entry_limit);
      return false;
    }
    frontier_.pop_back();
    for (auto iterator = replacements.rbegin();
         iterator != replacements.rend();
         ++iterator) {
      frontier_.push_back(*iterator);
    }
    result_.audit.maximum_frontier_entry_count = std::max(
        result_.audit.maximum_frontier_entry_count,
        frontier_.size());
    return true;
  }

  [[nodiscard]] bool expand_product(
      std::size_t first_node_index,
      std::size_t second_node_index) {
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (first_node_index == second_node_index) {
      if (first.is_leaf()) {
        throw std::logic_error(
            "a pair-support diagonal leaf cannot be expanded");
      }
      if (!push_replacement_entries({
          make_entry(first.left_child, first.left_child),
          make_entry(first.left_child, first.right_child),
          make_entry(first.right_child, first.right_child)})) {
        return false;
      }
      result_.audit.support_product_expansion_count = checked_add(
          result_.audit.support_product_expansion_count,
          1U,
          "the pair-support product-expansion count overflows size_t");
      result_.audit.self_product_expansion_count = checked_add(
          result_.audit.self_product_expansion_count,
          1U,
          "the pair-support self-expansion count overflows size_t");
      return true;
    }
    const bool first_leaf = first.is_leaf();
    const bool second_leaf = second.is_leaf();
    bool split_first = false;
    if (!first_leaf && second_leaf) {
      split_first = true;
    } else if (first_leaf && !second_leaf) {
      split_first = false;
    } else if (!first_leaf && !second_leaf) {
      const exact::ExactLevel first_diagonal =
          node_squared_diagonal(first_node_index);
      const exact::ExactLevel second_diagonal =
          node_squared_diagonal(second_node_index);
      const std::size_t first_size = first.leaf_end - first.leaf_begin;
      const std::size_t second_size = second.leaf_end - second.leaf_begin;
      if (first_diagonal != second_diagonal) {
        split_first = first_diagonal > second_diagonal;
      } else if (first_size != second_size) {
        split_first = first_size > second_size;
      } else {
        split_first = first_node_index > second_node_index;
      }
    } else {
      throw std::logic_error(
          "a pair-support leaf cross product escaped classification");
    }
    if (split_first) {
      if (!push_replacement_entries({
          make_entry(first.left_child, second_node_index),
          make_entry(first.right_child, second_node_index)})) {
        return false;
      }
    } else {
      if (!push_replacement_entries({
          make_entry(first_node_index, second.left_child),
          make_entry(first_node_index, second.right_child)})) {
        return false;
      }
    }
    result_.audit.support_product_expansion_count = checked_add(
        result_.audit.support_product_expansion_count,
        1U,
        "the pair-support product-expansion count overflows size_t");
    result_.audit.cross_product_expansion_count = checked_add(
        result_.audit.cross_product_expansion_count,
        1U,
        "the pair-support cross-expansion count overflows size_t");
    return true;
  }

  void visit_frontier_back() {
    if (frontier_.empty() || pending_product_.has_value()) {
      throw std::logic_error(
          "a pair-support product visit requires one unclaimed frontier back");
    }
    if (!consume_work_unit()) {
      return;
    }
    result_.audit.support_product_visit_count = checked_add(
        result_.audit.support_product_visit_count,
        1U,
        "the pair-support product-visit count overflows size_t");
    const ExactPairSupportFrontierEntry entry = frontier_.back();
    const auto [first_node_index, second_node_index] = entry_nodes(entry);
    const Node& first = node(first_node_index);
    const Node& second = node(second_node_index);
    if (first_node_index == second_node_index && first.is_leaf()) {
      frontier_.pop_back();
      result_.audit.diagonal_leaf_discard_count = checked_add(
          result_.audit.diagonal_leaf_discard_count,
          1U,
          "the pair-support diagonal count overflows size_t");
      return;
    }
    // A diagonal product cannot satisfy the strict phi prune: the relaxed
    // support boxes allow u = v at one endpoint of A, hence
    // phi(x, u, u) = ||x-u||^2 >= 0 for every witness box.  Avoid a global
    // witness traversal whose exact outcome is known in advance.
    if (first_node_index == second_node_index) {
      result_.audit.diagonal_product_rank_search_skip_count = checked_add(
          result_.audit.diagonal_product_rank_search_skip_count,
          1U,
          "the pair-support diagonal skip count overflows size_t");
      pending_product_.emplace();
      pending_product_->product = entry;
      pending_product_->stage =
          ExactPairSupportPendingStage::expand_product;
      continue_pending_product();
      return;
    }
    // At a leaf pair, the sparse closed-ball traversal already performs the
    // exact rank cap.  Running a separate global phi witness search here
    // would traverse the same LBVH twice for every surviving support.
    if (first.is_leaf() && second.is_leaf()) {
      pending_product_.emplace();
      pending_product_->product = entry;
      pending_product_->stage = ExactPairSupportPendingStage::classify_leaf;
      continue_pending_product();
      return;
    }

    pending_product_.emplace();
    pending_product_->product = entry;
    pending_product_->stage = ExactPairSupportPendingStage::rank_search;
    continue_pending_product();
  }

  void continue_pending_product() {
    if (!pending_product_.has_value() || frontier_.empty() ||
        pending_product_->product != frontier_.back()) {
      throw std::logic_error(
          "a pair-support pending product is detached from the frontier");
    }
    if (pending_product_->stage ==
        ExactPairSupportPendingStage::rank_search) {
      const RankSearchOutcome rank_search = continue_rank_prune_search();
      if (rank_search == RankSearchOutcome::budget_exhausted) {
        return;
      }
      if (rank_search == RankSearchOutcome::prune) {
        const std::size_t pair_count =
            entry_pair_count(pending_product_->product);
        frontier_.pop_back();
        result_.audit.rank_pruned_product_count = checked_add(
            result_.audit.rank_pruned_product_count,
            1U,
            "the pair-support pruned-product count overflows size_t");
        result_.audit.rank_pruned_pair_count = checked_add(
            result_.audit.rank_pruned_pair_count,
            pair_count,
            "the pair-support pruned-pair count overflows size_t");
        result_.audit.resolved_pair_count = checked_add(
            result_.audit.resolved_pair_count,
            pair_count,
            "the pair-support resolved-pair count overflows size_t");
        pending_product_.reset();
        return;
      }
      pending_product_->stage =
          ExactPairSupportPendingStage::expand_product;
      pending_product_->rank_search_started = false;
      pending_product_->witness_frontier.clear();
      pending_product_->strict_witness_receipts.clear();
      pending_product_->deferred_expansion_node.reset();
      pending_product_->strict_witness_point_count = 0U;
    }

    const auto [first_node_index, second_node_index] =
        entry_nodes(pending_product_->product);
    if (pending_product_->stage ==
        ExactPairSupportPendingStage::expand_product) {
      if (expand_product(first_node_index, second_node_index)) {
        pending_product_.reset();
      }
      return;
    }
    if (pending_product_->stage ==
        ExactPairSupportPendingStage::classify_leaf) {
      if (classify_leaf_pair(first_node_index, second_node_index)) {
        pending_product_.reset();
      }
      return;
    }
    throw std::logic_error("a pair-support pending stage is invalid");
  }

  void finish_result() {
    if (canonical_sort_records_) {
      std::sort(result_.events.begin(), result_.events.end(), event_less);
      std::sort(
          result_.relevant_extra_shell_diagnostics.begin(),
          result_.relevant_extra_shell_diagnostics.end(),
          diagnostic_less);
    }
    if (canonical_sort_records_) {
      result_.remaining_frontier = frontier_;
    }
    result_.audit.remaining_frontier_pair_count = 0U;
    for (const ExactPairSupportFrontierEntry& entry : frontier_) {
      result_.audit.remaining_frontier_pair_count = checked_add(
          result_.audit.remaining_frontier_pair_count,
          entry_pair_count(entry),
          "the pair-support remaining-pair count overflows size_t");
    }
    const std::size_t accounted_pair_count = checked_add(
        result_.audit.resolved_pair_count,
        result_.audit.remaining_frontier_pair_count,
        "the pair-support accounting sum overflows size_t");
    result_.audit.pair_partition_accounting_certified =
        accounted_pair_count == result_.audit.total_pair_count &&
        result_.audit.resolved_pair_count == checked_add(
            result_.audit.rank_pruned_pair_count,
            result_.audit.leaf_pair_classification_count,
            "the pair-support terminal accounting overflows size_t");
    if (!result_.audit.pair_partition_accounting_certified) {
      throw std::logic_error(
          "the pair-support frontier does not partition all unordered pairs");
    }
    result_.self_product_partition_certified = true;
    result_.witness_antichains_certified = true;
    result_.all_rank_prunes_recertified = true;
    // Every non-rank-rejected leaf query reaches a complete global shell.
    result_.all_rank_relevant_shells_complete = true;
    result_.frontier_exhausted = frontier_.empty();
    result_.no_forbidden_global_structure_materialized = true;
    result_.hierarchy_reduction_performed = false;
    if (frontier_.empty()) {
      result_.status = ExactPairSupportStreamStatus::complete;
      result_.stop_reason = ExactPairSupportStopReason::none;
    } else if (!stopped_) {
      throw std::logic_error(
          "a nonempty pair-support frontier has no budget stop reason");
    }
  }

  const spatial::MortonLbvhIndex& index_;
  const spatial::CanonicalPointCloud& cloud_;
  ExactPairSupportStreamResult result_;
  std::vector<ExactPairSupportFrontierEntry> frontier_;
  std::optional<ExactPairSupportPendingProduct> pending_product_;
  ExactPairSupportStreamAudit chunk_audit_origin_{};
  contract::CanonicalId output_chain_digest_{};
  std::size_t output_record_count_{0U};
  std::vector<ExactPairSupportStreamChunk::RecordKind> record_order_;
  bool canonical_sort_records_{false};
  bool stopped_{false};
};

ExactPairSupportStreamResult build_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget) {
  ExactPairSupportStreamBuilder builder{
      index, cloud, requested_maximum_order, budget};
  builder.execute();
  return builder.take_result();
}

ExactPairSupportCheckpointManifest
make_exact_pair_support_checkpoint_manifest(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  return ExactPairSupportStreamBuilder::manifest_for(
      index, cloud, requested_maximum_order);
}

ExactPairSupportAuthorityContext::ExactPairSupportAuthorityContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order)
    : index_(&index),
      cloud_(&cloud),
      requested_maximum_order_(requested_maximum_order) {
  manifest_ = ExactPairSupportStreamBuilder::manifest_for_with_audit(
      index, cloud, requested_maximum_order, &audit_);
}

ExactPairSupportCheckpoint make_initial_exact_pair_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return make_initial_exact_pair_support_checkpoint(authority);
}

ExactPairSupportCheckpoint make_initial_exact_pair_support_checkpoint(
    const ExactPairSupportAuthorityContext& authority) {
  ExactPairSupportStreamBuilder builder{
      authority.index(),
      authority.cloud(),
      authority.requested_maximum_order(),
      ExactPairSupportStreamBudget{
          0U,
          1U,
          0U,
          0U,
          0U,
          0U,
          0U}};
  return builder.snapshot_checkpoint(authority.manifest(), 0U);
}

contract::CanonicalId compute_exact_pair_support_checkpoint_digest(
    const ExactPairSupportCheckpoint& checkpoint) {
  return checkpoint_digest(checkpoint);
}

ExactPairSupportCheckpointVerification verify_exact_pair_support_checkpoint(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportCheckpoint& checkpoint) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return verify_exact_pair_support_checkpoint(authority, checkpoint);
}

ExactPairSupportCheckpointVerification verify_exact_pair_support_checkpoint(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportCheckpoint& checkpoint) {
  return ExactPairSupportStreamBuilder::verify_checkpoint_for(
      authority.index(),
      authority.cloud(),
      authority.requested_maximum_order(),
      authority.manifest(),
      checkpoint);
}

namespace {

[[nodiscard]] ExactPairSupportStreamChunk
build_exact_pair_support_stream_chunk_after_source_verification(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint) {
  ExactPairSupportStreamChunk chunk;
  chunk.manifest = checkpoint.manifest;
  chunk.budget = chunk_budget;
  chunk.chunk_sequence = checkpoint.next_chunk_sequence;
  chunk.first_output_record_index = checkpoint.output_record_count;
  chunk.source_checkpoint_digest = checkpoint.checkpoint_digest;
  chunk.previous_output_chain_digest =
      checkpoint.output_chain_digest;
  chunk.cumulative_audit_before = checkpoint.cumulative_audit;
  chunk.no_forbidden_global_structure_materialized = true;
  chunk.hierarchy_reduction_performed = false;
  if (checkpoint.complete()) {
    chunk.output_chain_digest =
        checkpoint.output_chain_digest;
    chunk.status = ExactPairSupportStreamStatus::complete;
    chunk.stop_reason = ExactPairSupportStopReason::none;
    chunk.cumulative_audit_after = checkpoint.cumulative_audit;
    chunk.next_checkpoint = checkpoint;
    chunk.candidate_prepared = true;
    return chunk;
  }
  if (checkpoint.next_chunk_sequence ==
      std::numeric_limits<std::uint64_t>::max()) {
    throw std::overflow_error(
        "the pair-support chunk sequence overflows uint64");
  }

  ExactPairSupportStreamBuilder builder{
      index,
      cloud,
      requested_maximum_order,
      chunk_budget,
      checkpoint,
      IntegrityVerifiedCheckpointTag{}};
  builder.execute();
  chunk.next_checkpoint = builder.take_checkpoint(
      checkpoint.manifest, checkpoint.next_chunk_sequence + 1U);
  ExactPairSupportStreamResult result = builder.take_result();
  chunk.output_chain_digest = builder.output_chain_digest();
  chunk.status = result.status;
  chunk.stop_reason = result.stop_reason;
  chunk.events = std::move(result.events);
  chunk.relevant_extra_shell_diagnostics =
      std::move(result.relevant_extra_shell_diagnostics);
  chunk.record_order = builder.take_record_order();
  chunk.cumulative_audit_after = result.audit;
  chunk.candidate_prepared = true;
  return chunk;
}

}  // namespace

ExactPairSupportStreamChunk build_exact_pair_support_stream_chunk(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return build_exact_pair_support_stream_chunk(
      authority, chunk_budget, checkpoint);
}

ExactPairSupportStreamChunk build_exact_pair_support_stream_chunk(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& checkpoint) {
  const ExactPairSupportCheckpointVerification source_verification =
      verify_exact_pair_support_checkpoint(authority, checkpoint);
  if (!source_verification.integrity_verified) {
    throw std::invalid_argument(
        "a pair-support chunk requires an integrity-verified trusted source checkpoint");
  }
  return build_exact_pair_support_stream_chunk_after_source_verification(
      authority.index(),
      authority.cloud(),
      authority.requested_maximum_order(),
      chunk_budget,
      checkpoint);
}

ExactPairSupportStreamChunkVerification verify_exact_pair_support_stream_chunk(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    const ExactPairSupportStreamChunk& observed) {
  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  return verify_exact_pair_support_stream_chunk(
      authority, chunk_budget, source_checkpoint, observed);
}

namespace {

[[nodiscard]] ExactPairSupportStreamChunkVerification
verify_exact_pair_support_stream_chunk_with_trusted_next(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    const ExactPairSupportStreamChunk& observed,
    bool source_checkpoint_already_trusted,
    ExactPairSupportCheckpoint* trusted_next_checkpoint) {
  ExactPairSupportStreamChunkVerification verification;
  verification.source_checkpoint_integrity_verified =
      source_checkpoint_already_trusted ||
      verify_exact_pair_support_checkpoint(authority, source_checkpoint)
          .integrity_verified;
  verification.requested_budget_certified =
      observed.budget == chunk_budget;
  if (!verification.source_checkpoint_integrity_verified) {
    return verification;
  }
  ExactPairSupportStreamChunk expected =
      build_exact_pair_support_stream_chunk_after_source_verification(
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
      observed.output_chain_digest ==
          expected.output_chain_digest &&
      observed.candidate_prepared == expected.candidate_prepared;
  verification.records_individually_exact =
      observed.events == expected.events &&
      observed.relevant_extra_shell_diagnostics ==
          expected.relevant_extra_shell_diagnostics &&
      observed.record_order == expected.record_order;
  verification.next_checkpoint_integrity_verified =
      observed.next_checkpoint == expected.next_checkpoint &&
      verify_exact_pair_support_checkpoint(
          authority, observed.next_checkpoint)
          .integrity_verified;
  verification.fresh_replay_certified = observed == expected;
  verification.chunk_transition_verified =
      verification.source_checkpoint_integrity_verified &&
      verification.requested_budget_certified &&
      verification.prepared_transition_chain_matches &&
      verification.records_individually_exact &&
      verification.next_checkpoint_integrity_verified &&
      verification.fresh_replay_certified;
  if (verification.chunk_transition_verified &&
      trusted_next_checkpoint != nullptr) {
    static_assert(
        std::is_nothrow_move_assignable_v<ExactPairSupportCheckpoint>);
    *trusted_next_checkpoint = std::move(expected.next_checkpoint);
  }
  return verification;
}

}  // namespace

ExactPairSupportStreamChunkVerification verify_exact_pair_support_stream_chunk(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    const ExactPairSupportStreamChunk& observed) {
  return verify_exact_pair_support_stream_chunk_with_trusted_next(
      authority,
      chunk_budget,
      source_checkpoint,
      observed,
      false,
      nullptr);
}

ExactPairSupportIncrementalVerifier::ExactPairSupportIncrementalVerifier(
    const ExactPairSupportAuthorityContext& authority)
    : authority_(authority),
      trusted_checkpoint_(
          make_initial_exact_pair_support_checkpoint(authority_)) {
  status_.initial_checkpoint_reconstructed =
      verify_exact_pair_support_checkpoint(
          authority_, trusted_checkpoint_)
          .integrity_verified;
  status_.every_transition_verified =
      status_.initial_checkpoint_reconstructed;
  status_.terminal_checkpoint_reached =
      status_.initial_checkpoint_reconstructed &&
      trusted_checkpoint_.complete();
  status_.anchored_prefix_certified =
      status_.initial_checkpoint_reconstructed;
  status_.anchored_run_certified =
      status_.anchored_prefix_certified &&
      status_.terminal_checkpoint_reached;
  status_.failed_closed =
      !status_.initial_checkpoint_reconstructed;
  status_.retained_chunk_count = 0U;
}

ExactPairSupportIncrementalVerifier::PreparedNext::PreparedNext(
    ExactPairSupportIncrementalVerifier* owner,
    std::uint64_t source_epoch,
    std::size_t next_verified_chunk_count,
    ExactPairSupportStreamChunkVerification verification,
    ExactPairSupportCheckpoint trusted_next) noexcept
    : owner_(owner),
      source_epoch_(source_epoch),
      next_verified_chunk_count_(next_verified_chunk_count),
      verification_(verification),
      trusted_next_(std::move(trusted_next)),
      valid_(true) {
  static_assert(
      std::is_nothrow_move_constructible_v<ExactPairSupportCheckpoint>);
}

ExactPairSupportIncrementalVerifier::PreparedNext::PreparedNext(
    PreparedNext&& other) noexcept
    : owner_(std::exchange(other.owner_, nullptr)),
      source_epoch_(other.source_epoch_),
      next_verified_chunk_count_(other.next_verified_chunk_count_),
      verification_(other.verification_),
      trusted_next_(std::move(other.trusted_next_)),
      valid_(std::exchange(other.valid_, false)) {}

ExactPairSupportIncrementalVerifier::PreparedNext&
ExactPairSupportIncrementalVerifier::PreparedNext::operator=(
    PreparedNext&& other) noexcept {
  if (this != &other) {
    owner_ = std::exchange(other.owner_, nullptr);
    source_epoch_ = other.source_epoch_;
    next_verified_chunk_count_ = other.next_verified_chunk_count_;
    verification_ = other.verification_;
    trusted_next_ = std::move(other.trusted_next_);
    valid_ = std::exchange(other.valid_, false);
  }
  return *this;
}

void ExactPairSupportIncrementalVerifier::poison() noexcept {
  status_.every_transition_verified = false;
  status_.anchored_prefix_certified = false;
  status_.anchored_run_certified = false;
  status_.failed_closed = true;
}

ExactPairSupportIncrementalVerifier::PreparedNext
ExactPairSupportIncrementalVerifier::prepare_next(
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportStreamChunk& observed) {
  if (status_.failed_closed ||
      !status_.initial_checkpoint_reconstructed ||
      status_.terminal_checkpoint_reached ||
      epoch_ == std::numeric_limits<std::uint64_t>::max()) {
    poison();
    return PreparedNext{};
  }

  ExactPairSupportCheckpoint trusted_next;
  ExactPairSupportStreamChunkVerification verification;
  try {
    verification =
        verify_exact_pair_support_stream_chunk_with_trusted_next(
            authority_,
            chunk_budget,
            trusted_checkpoint_,
            observed,
            true,
            &trusted_next);
  } catch (...) {
    poison();
    throw;
  }
  if (!verification.chunk_transition_verified) {
    poison();
    return PreparedNext{verification};
  }

  std::size_t verified_chunk_count = 0U;
  try {
    verified_chunk_count = checked_add(
        status_.verified_chunk_count,
        1U,
        "the incremental pair-support verified chunk count overflows size_t");
  } catch (...) {
    poison();
    throw;
  }
  return PreparedNext{
      this,
      epoch_,
      verified_chunk_count,
      verification,
      std::move(trusted_next)};
}

bool ExactPairSupportIncrementalVerifier::commit_prepared(
    PreparedNext&& prepared) noexcept {
  if (!prepared.valid_ || prepared.owner_ != this ||
      prepared.source_epoch_ != epoch_ || status_.failed_closed ||
      status_.terminal_checkpoint_reached ||
      !prepared.verification_.chunk_transition_verified) {
    poison();
    prepared.valid_ = false;
    prepared.owner_ = nullptr;
    return false;
  }

  static_assert(
      std::is_nothrow_move_assignable_v<ExactPairSupportCheckpoint>);
  trusted_checkpoint_ = std::move(prepared.trusted_next_);
  status_.verified_chunk_count = prepared.next_verified_chunk_count_;
  status_.terminal_checkpoint_reached = trusted_checkpoint_.complete();
  status_.anchored_prefix_certified = true;
  status_.anchored_run_certified = status_.terminal_checkpoint_reached;
  status_.retained_chunk_count = 0U;
  ++epoch_;
  prepared.valid_ = false;
  prepared.owner_ = nullptr;
  return true;
}

ExactPairSupportStreamChunkVerification
ExactPairSupportIncrementalVerifier::verify_next(
    const ExactPairSupportStreamBudget& chunk_budget,
    const ExactPairSupportStreamChunk& observed) {
  PreparedNext prepared = prepare_next(chunk_budget, observed);
  ExactPairSupportStreamChunkVerification verification =
      prepared.verification();
  if (prepared.prepared() && !commit_prepared(std::move(prepared))) {
    verification.chunk_transition_verified = false;
  }
  return verification;
}

ExactPairSupportStreamRunVerification verify_exact_pair_support_stream_run(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    std::span<const ExactPairSupportStreamBudget> chunk_budgets,
    std::span<const ExactPairSupportStreamChunk> chunks) {
  ExactPairSupportStreamRunVerification verification;
  if (chunk_budgets.size() != chunks.size()) {
    return verification;
  }

  const ExactPairSupportAuthorityContext authority{
      index, cloud, requested_maximum_order};
  ExactPairSupportIncrementalVerifier incremental{authority};
  for (std::size_t chunk_index = 0U;
       chunk_index < chunks.size();
       ++chunk_index) {
    const ExactPairSupportStreamChunkVerification transition =
        incremental.verify_next(
            chunk_budgets[chunk_index],
            chunks[chunk_index]);
    if (!transition.chunk_transition_verified) {
      break;
    }
  }

  const ExactPairSupportIncrementalVerifierStatus& status =
      incremental.status();
  verification.initial_checkpoint_reconstructed =
      status.initial_checkpoint_reconstructed;
  verification.verified_chunk_count = status.verified_chunk_count;
  verification.every_transition_verified =
      status.every_transition_verified &&
      status.verified_chunk_count == chunks.size();
  verification.terminal_checkpoint_reached =
      status.terminal_checkpoint_reached;
  verification.anchored_run_certified =
      verification.initial_checkpoint_reconstructed &&
      verification.every_transition_verified &&
      verification.verified_chunk_count == chunks.size() &&
      verification.terminal_checkpoint_reached &&
      status.anchored_run_certified &&
      !status.failed_closed &&
      status.retained_chunk_count == 0U;
  return verification;
}

ExactPairSupportStreamVerification verify_exact_pair_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order,
    const ExactPairSupportStreamBudget& budget,
    const ExactPairSupportStreamResult& observed) {
  const ExactPairSupportStreamResult expected =
      build_exact_pair_support_stream(
          index, cloud, requested_maximum_order, budget);
  ExactPairSupportStreamVerification verification;
  verification.requested_budget_certified = observed.budget == budget;
  verification.requirements_certified =
      observed.requirements == expected.requirements;
  verification.partial_records_individually_exact =
      observed.events == expected.events &&
      observed.relevant_extra_shell_diagnostics ==
          expected.relevant_extra_shell_diagnostics;
  verification.completion_claim_certified =
      observed.stream_complete() == expected.stream_complete() &&
      observed.status == expected.status &&
      observed.stop_reason == expected.stop_reason &&
      observed.frontier_exhausted == expected.frontier_exhausted;
  verification.absence_claim_certified =
      observed.absence_of_additional_pair_supports_certified() ==
      expected.absence_of_additional_pair_supports_certified();
  verification.fresh_replay_certified = observed == expected;
  verification.result_certified =
      verification.requested_budget_certified &&
      verification.requirements_certified &&
      verification.partial_records_individually_exact &&
      verification.completion_claim_certified &&
      verification.absence_claim_certified &&
      verification.fresh_replay_certified;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
