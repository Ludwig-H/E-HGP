#include "morsehgp3d/hierarchy/direct_k1_normalized_product_stream.hpp"

#include <array>
#include <atomic>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view scientific_digest_domain =
    "MorseHGP3D/phase15/direct-k1-normalized-product/science/v1/sha256/";
constexpr std::string_view initial_chain_domain =
    "MorseHGP3D/phase15/direct-k1-normalized-product/initial/v1/sha256/";
constexpr std::string_view batch_chain_domain =
    "MorseHGP3D/phase15/direct-k1-normalized-product/batch/v1/sha256/";
constexpr std::string_view at_least20_initial_domain =
    "MorseHGP3D/phase15/direct-k1-normalized-product/at-least20-initial/v1/sha256/";
constexpr std::string_view at_least20_transition_domain =
    "MorseHGP3D/phase15/direct-k1-normalized-product/at-least20-transitions/v1/sha256/";
constexpr std::string_view at_least20_view_step_domain =
    "MorseHGP3D/phase15/direct-k1-normalized-product/at-least20-step/v1/sha256/";
constexpr std::string_view checkpoint_domain =
    "MorseHGP3D/phase15/direct-k1-normalized-product/checkpoint/v1/sha256/";
constexpr std::string_view final_seal_domain =
    "MorseHGP3D/phase15/direct-k1-normalized-product/final-seal/v1/sha256/";

static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
static_assert(std::is_nothrow_move_constructible_v<exact::ExactLevel>);
static_assert(std::is_nothrow_move_constructible_v<
              ExactDirectK1NormalizedProductBatchRecord>);
static_assert(std::is_nothrow_move_assignable_v<
              std::vector<ExactDirectK1NormalizedAtLeast20Transition>>);
static_assert(
    std::is_nothrow_move_assignable_v<std::vector<K1NodeId>>);

std::atomic<std::uint64_t> next_session_instance_id{1U};

struct StreamSeal final {};

struct PreparedTicketRegistry {
  std::size_t outstanding_count{};
};

[[nodiscard]] std::uint64_t allocate_session_instance_id() noexcept {
  std::uint64_t candidate =
      next_session_instance_id.load(std::memory_order_relaxed);
  while (candidate != 0U) {
    const std::uint64_t successor = candidate + 1U;
    if (next_session_instance_id.compare_exchange_weak(
            candidate,
            successor,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
      return candidate;
    }
  }
  return 0U;
}

[[nodiscard]] bool checked_add(
    std::size_t left, std::size_t right, std::size_t& output) noexcept {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] bool checked_multiply(
    std::size_t left, std::size_t right, std::size_t& output) noexcept {
  if (right != 0U &&
      left > std::numeric_limits<std::size_t>::max() / right) {
    return false;
  }
  output = left * right;
  return true;
}

[[nodiscard]] bool id_is_zero(
    const contract::CanonicalId& value) noexcept {
  return value == contract::CanonicalId{};
}

void append_u32(
    contract::CanonicalSha256Builder& builder, std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - index - 1U) * 8U));
  }
  builder.update(bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder, std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - index - 1U) * 8U));
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder, std::size_t value) {
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_bool(
    contract::CanonicalSha256Builder& builder, bool value) {
  append_u64(builder, value ? 1U : 0U);
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

void append_text(
    contract::CanonicalSha256Builder& builder, std::string_view value) {
  append_size(builder, value.size());
  builder.update(value);
}

void append_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& level) {
  append_text(builder, level.numerator_string());
  append_text(builder, level.denominator_string());
}

[[nodiscard]] ExactDirectK1NormalizedProductStreamRequirements
derive_requirements(std::size_t point_count, bool& overflow) noexcept {
  ExactDirectK1NormalizedProductStreamRequirements output;
  output.point_count = point_count;
  if (point_count == 0U) {
    overflow = true;
    return output;
  }
  const std::size_t edge_count = point_count - 1U;
  output.product_batch_count_upper_bound = point_count;
  output.merge_node_count_upper_bound = edge_count;
  if (!checked_multiply(
          2U, edge_count, output.child_reference_count_upper_bound) ||
      !checked_add(
          point_count,
          1U,
          output.history_digest_count_upper_bound)) {
    overflow = true;
    return output;
  }
  output.checkpoint_replay_batch_count_upper_bound = point_count;
  output.batch_at_least20_transition_count_upper_bound = edge_count;
  output.batch_at_least20_visible_child_reference_count_upper_bound =
      output.child_reference_count_upper_bound;
  output.active_at_least20_root_count_upper_bound =
      point_count / direct_k1_normalized_product_stream_at_least20_threshold;
  output.checkpoint_active_at_least20_root_count_upper_bound =
      output.active_at_least20_root_count_upper_bound;
  overflow = false;
  return output;
}

[[nodiscard]] bool budget_covers(
    const ExactDirectK1NormalizedProductStreamRequirements& requirements,
    const ExactDirectK1NormalizedProductStreamBudget& budget) noexcept {
  return requirements.point_count <= budget.maximum_point_count &&
      requirements.product_batch_count_upper_bound <=
          budget.maximum_product_batch_count &&
      requirements.merge_node_count_upper_bound <=
          budget.maximum_merge_node_count &&
      requirements.child_reference_count_upper_bound <=
          budget.maximum_child_reference_count &&
      requirements.history_digest_count_upper_bound <=
          budget.maximum_history_digest_count &&
      requirements.checkpoint_replay_batch_count_upper_bound <=
          budget.maximum_checkpoint_replay_batch_count &&
      budget.maximum_outstanding_prepared_batch_count != 0U &&
      requirements.batch_at_least20_transition_count_upper_bound <=
          budget.maximum_batch_at_least20_transition_count &&
      requirements.batch_at_least20_visible_child_reference_count_upper_bound <=
          budget.maximum_batch_at_least20_visible_child_reference_count &&
      requirements.active_at_least20_root_count_upper_bound <=
          budget.maximum_active_at_least20_root_count &&
      requirements.checkpoint_active_at_least20_root_count_upper_bound <=
          budget.maximum_checkpoint_active_at_least20_root_count;
}

[[nodiscard]] contract::CanonicalId compute_scientific_digest(
    const contract::CanonicalId& canonical_cloud_digest,
    const contract::CanonicalId& source_forest_digest,
    std::size_t point_count) {
  contract::CanonicalSha256Builder builder;
  builder.update(scientific_digest_domain);
  append_u32(builder, direct_k1_normalized_product_stream_schema_version);
  append_size(builder, direct_k1_normalized_product_stream_order);
  append_size(builder, point_count);
  append_id(builder, canonical_cloud_digest);
  append_id(builder, source_forest_digest);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_initial_chain_digest(
    const contract::CanonicalId& scientific_digest,
    const contract::CanonicalId& source_forest_digest) {
  contract::CanonicalSha256Builder builder;
  builder.update(initial_chain_domain);
  append_u32(builder, direct_k1_normalized_product_stream_schema_version);
  append_id(builder, scientific_digest);
  append_id(builder, source_forest_digest);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_at_least20_initial_digest(
    const contract::CanonicalId& scientific_digest,
    const contract::CanonicalId& source_forest_digest) {
  contract::CanonicalSha256Builder builder;
  builder.update(at_least20_initial_domain);
  append_u32(builder, direct_k1_normalized_product_stream_schema_version);
  append_size(
      builder, direct_k1_normalized_product_stream_at_least20_threshold);
  append_id(builder, scientific_digest);
  append_id(builder, source_forest_digest);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_batch_chain_digest(
    const ExactDirectK1NormalizedProductBatchRecord& record) {
  contract::CanonicalSha256Builder builder;
  builder.update(batch_chain_domain);
  append_u32(builder, record.schema_version);
  append_id(builder, record.previous_scientific_chain_digest);
  append_id(builder, record.source_forest_digest);
  append_size(builder, record.normalized_batch_index);
  append_size(builder, record.order);
  append_size(builder, record.order_batch_index);
  append_level(builder, record.squared_level);
  append_u64(builder, static_cast<std::uint8_t>(record.kind));
  append_size(builder, record.group_count);
  append_size(builder, record.qr0_group_count);
  append_size(builder, record.qr1_group_count);
  append_size(builder, record.qr_greater_equal2_group_count);
  append_size(builder, record.core_facet_delta_count);
  append_size(builder, record.parent_delta_count);
  append_size(builder, record.node_delta_count);
  append_size(builder, record.point_delta_count);
  append_size(builder, record.merge_node_offset);
  append_size(builder, record.merge_node_count);
  append_size(builder, record.child_reference_offset);
  append_size(builder, record.child_reference_count);
  append_u64(builder, record.implicit_singleton_first_root_id);
  append_size(builder, record.implicit_singleton_count);
  append_id(builder, record.previous_at_least20_view_digest);
  append_id(builder, record.at_least20_transition_digest);
  append_id(builder, record.at_least20_view_digest);
  append_size(builder, record.at_least20_hidden_result_count);
  append_size(builder, record.at_least20_threshold_entry_count);
  append_size(builder, record.at_least20_continuation_count);
  append_size(builder, record.at_least20_multifusion_count);
  append_size(builder, record.at_least20_visible_child_reference_count);
  append_size(builder, record.active_at_least20_root_count_before);
  append_size(builder, record.active_at_least20_root_count_after);
  append_bool(builder, record.complete_qr_partition_certified);
  append_bool(builder, record.exact_parent_node_point_deltas_certified);
  append_bool(builder, record.compact_csr_references_certified);
  append_bool(builder, record.closed_zero_level_partition_certified);
  append_bool(builder, record.at_least20_complete_compact_fold_certified);
  append_bool(
      builder,
      record.implicit_singleton_genesis_without_group_materialization);
  append_bool(builder, record.gamma_star_or_delaunay_materialized);
  append_bool(builder, record.public_status_claimed);
  return builder.finalize();
}

struct ChildRange {
  std::size_t offset{};
  std::size_t count{};
};

[[nodiscard]] ChildRange child_range(
    const K1CompactForest& forest,
    const K1CompactEqualLevelBatch& batch) {
  if (batch.merge_node_count == 0U ||
      batch.merge_node_offset > forest.merge_nodes.size() ||
      batch.merge_node_count >
          forest.merge_nodes.size() - batch.merge_node_offset) {
    throw std::logic_error("a normalized K1 batch has no closed merge arena");
  }
  const K1CompactMergeNode& first =
      forest.merge_nodes.at(batch.merge_node_offset);
  const K1CompactMergeNode& last = forest.merge_nodes.at(
      batch.merge_node_offset + batch.merge_node_count - 1U);
  std::size_t last_end = 0U;
  if (!checked_add(last.child_offset, last.child_count, last_end) ||
      first.child_offset > last_end || last_end > forest.child_ids.size()) {
    throw std::logic_error("a normalized K1 batch CSR range is invalid");
  }
  return ChildRange{first.child_offset, last_end - first.child_offset};
}

[[nodiscard]] ExactDirectK1NormalizedProductBatchRecord make_record_shape(
    const K1CompactForest& forest,
    const contract::CanonicalId& source_forest_digest,
    const contract::CanonicalId& previous_chain_digest,
    std::size_t normalized_batch_index) {
  ExactDirectK1NormalizedProductBatchRecord output;
  output.normalized_batch_index = normalized_batch_index;
  output.order_batch_index = normalized_batch_index;
  output.source_forest_digest = source_forest_digest;
  output.previous_scientific_chain_digest = previous_chain_digest;
  output.complete_qr_partition_certified = true;
  output.exact_parent_node_point_deltas_certified = true;
  output.compact_csr_references_certified = true;
  output.closed_zero_level_partition_certified = true;
  output.gamma_star_or_delaunay_materialized = false;
  output.public_status_claimed = false;

  if (normalized_batch_index == 0U) {
    output.kind =
        ExactDirectK1NormalizedProductBatchKind::implicit_singleton_genesis;
    output.group_count = forest.point_count;
    output.qr0_group_count = forest.point_count;
    output.core_facet_delta_count = forest.point_count;
    output.node_delta_count = forest.point_count;
    output.point_delta_count = forest.point_count;
    output.implicit_singleton_first_root_id = K1NodeId{0U};
    output.implicit_singleton_count = forest.point_count;
    output.implicit_singleton_genesis_without_group_materialization = true;
    return output;
  }

  const std::size_t equal_batch_index = normalized_batch_index - 1U;
  const K1CompactEqualLevelBatch& batch =
      forest.equal_level_batches.at(equal_batch_index);
  const ChildRange children = child_range(forest, batch);
  output.kind = ExactDirectK1NormalizedProductBatchKind::
      compact_equal_level_multifusions;
  output.squared_level = forest.levels.at(batch.level_index);
  output.group_count = batch.merge_node_count;
  output.qr_greater_equal2_group_count = batch.merge_node_count;
  output.parent_delta_count = children.count;
  output.node_delta_count = batch.merge_node_count;
  output.merge_node_offset = batch.merge_node_offset;
  output.merge_node_count = batch.merge_node_count;
  output.child_reference_offset = children.offset;
  output.child_reference_count = children.count;
  return output;
}

[[nodiscard]] std::size_t node_coverage_size(
    const K1CompactForest& forest, K1NodeId node_id) {
  if (node_id < forest.point_count) {
    return 1U;
  }
  const std::size_t merge_index =
      static_cast<std::size_t>(node_id) - forest.point_count;
  if (merge_index >= forest.merge_nodes.size() ||
      forest.merge_nodes[merge_index].id != node_id) {
    throw std::logic_error("an at_least20 K1 root id is not in the forest");
  }
  return forest.merge_nodes[merge_index].coverage_size;
}

struct AtLeast20Fold {
  std::vector<ExactDirectK1NormalizedAtLeast20Transition> transitions;
  std::vector<K1NodeId> visible_child_root_ids;
  contract::CanonicalId transition_digest{};
  contract::CanonicalId view_digest{};
  std::size_t hidden_result_count{};
  std::size_t threshold_entry_count{};
  std::size_t continuation_count{};
  std::size_t multifusion_count{};
  std::size_t visible_child_reference_count{};
  std::size_t active_root_count_before{};
  std::size_t active_root_count_after{};
};

[[nodiscard]] AtLeast20Fold fold_at_least20_batch(
    const K1CompactForest& forest,
    std::size_t normalized_batch_index,
    const contract::CanonicalId& previous_view_digest,
    std::vector<std::uint8_t>& active_merge_flags,
    std::size_t active_root_count,
    const ExactDirectK1NormalizedProductStreamBudget& budget,
    bool apply,
    bool collect_transitions) {
  if (active_merge_flags.size() != forest.merge_nodes.size()) {
    throw std::logic_error("the at_least20 active-root arena is not closed");
  }
  AtLeast20Fold output;
  output.active_root_count_before = active_root_count;
  output.active_root_count_after = active_root_count;
  contract::CanonicalSha256Builder transition_builder;
  transition_builder.update(at_least20_transition_domain);
  append_u32(
      transition_builder,
      direct_k1_normalized_product_stream_schema_version);
  append_size(transition_builder, normalized_batch_index);

  if (normalized_batch_index == 0U) {
    output.hidden_result_count = forest.point_count;
    append_size(transition_builder, forest.point_count);
  } else {
    const K1CompactEqualLevelBatch& batch =
        forest.equal_level_batches.at(normalized_batch_index - 1U);
    append_level(transition_builder, forest.levels.at(batch.level_index));
    append_size(transition_builder, batch.merge_node_count);
    if (collect_transitions) {
      output.transitions.reserve(batch.merge_node_count);
      output.visible_child_root_ids.reserve(child_range(forest, batch).count);
    }
    std::size_t logical_visible_child_offset = 0U;
    for (std::size_t local_index = 0U;
         local_index < batch.merge_node_count;
         ++local_index) {
      const K1CompactMergeNode& node =
          forest.merge_nodes.at(batch.merge_node_offset + local_index);
      std::size_t q_visible = 0U;
      for (std::size_t child_local = 0U;
           child_local < node.child_count;
           ++child_local) {
        const K1NodeId child_id =
            forest.child_ids.at(node.child_offset + child_local);
        if (node_coverage_size(forest, child_id) <
            direct_k1_normalized_product_stream_at_least20_threshold) {
          continue;
        }
        if (child_id < forest.point_count) {
          throw std::logic_error(
              "an implicit singleton cannot be visible at threshold 20");
        }
        const std::size_t child_merge_index =
            static_cast<std::size_t>(child_id) - forest.point_count;
        if (child_merge_index >= active_merge_flags.size() ||
            active_merge_flags[child_merge_index] == 0U) {
          throw std::logic_error(
              "an at_least20 transition references an inactive child root");
        }
        ++q_visible;
      }

      if (node.coverage_size <
          direct_k1_normalized_product_stream_at_least20_threshold) {
        if (q_visible != 0U) {
          throw std::logic_error(
              "a hidden K1 result contains a visible child root");
        }
        ++output.hidden_result_count;
        continue;
      }

      ExactDirectK1NormalizedAtLeast20Transition transition;
      transition.source_group_index = local_index;
      transition.resultant_root_id = node.id;
      transition.source_q_r = node.child_count;
      transition.q_visible = q_visible;
      transition.coverage_size = node.coverage_size;
      transition.visible_child_reference_offset =
          logical_visible_child_offset;
      transition.visible_child_reference_count = q_visible;
      transition.coverage_size_at_least_20 = true;
      if (q_visible == 0U) {
        transition.kind = ExactDirectK1NormalizedAtLeast20TransitionKind::
            threshold_entry;
        ++output.threshold_entry_count;
      } else if (q_visible == 1U) {
        transition.kind = ExactDirectK1NormalizedAtLeast20TransitionKind::
            continuation;
        ++output.continuation_count;
      } else {
        transition.kind = ExactDirectK1NormalizedAtLeast20TransitionKind::
            multifusion;
        ++output.multifusion_count;
      }
      append_size(transition_builder, transition.source_group_index);
      append_u64(transition_builder, transition.resultant_root_id);
      append_size(transition_builder, transition.source_q_r);
      append_size(transition_builder, transition.q_visible);
      append_size(transition_builder, transition.coverage_size);
      append_size(
          transition_builder,
          transition.visible_child_reference_offset);
      append_size(
          transition_builder,
          transition.visible_child_reference_count);
      append_u64(
          transition_builder,
          static_cast<std::uint8_t>(transition.kind));
      append_bool(
          transition_builder, transition.coverage_size_at_least_20);

      for (std::size_t child_local = 0U;
           child_local < node.child_count;
           ++child_local) {
        const K1NodeId child_id =
            forest.child_ids.at(node.child_offset + child_local);
        if (node_coverage_size(forest, child_id) <
            direct_k1_normalized_product_stream_at_least20_threshold) {
          continue;
        }
        append_u64(transition_builder, child_id);
        if (collect_transitions) {
          output.visible_child_root_ids.push_back(child_id);
        }
        if (apply) {
          const std::size_t child_merge_index =
              static_cast<std::size_t>(child_id) - forest.point_count;
          active_merge_flags[child_merge_index] = 0U;
        }
      }
      logical_visible_child_offset += q_visible;
      if (output.active_root_count_after < q_visible) {
        throw std::logic_error("the at_least20 active-root count underflows");
      }
      output.active_root_count_after -= q_visible;
      ++output.active_root_count_after;
      if (apply) {
        const std::size_t result_merge_index =
            static_cast<std::size_t>(node.id) - forest.point_count;
        if (result_merge_index >= active_merge_flags.size() ||
            active_merge_flags[result_merge_index] != 0U) {
          throw std::logic_error(
              "an at_least20 resultant root was already active");
        }
        active_merge_flags[result_merge_index] = 1U;
      }
      if (collect_transitions) {
        output.transitions.push_back(std::move(transition));
      }
    }
    if (logical_visible_child_offset >
            budget.maximum_batch_at_least20_visible_child_reference_count ||
        output.threshold_entry_count + output.continuation_count +
                output.multifusion_count >
            budget.maximum_batch_at_least20_transition_count) {
      throw std::length_error("an at_least20 batch exceeds its budget");
    }
    output.visible_child_reference_count = logical_visible_child_offset;
  }
  if (output.active_root_count_after >
      budget.maximum_active_at_least20_root_count) {
    throw std::length_error("the active at_least20 roots exceed their budget");
  }
  output.transition_digest = transition_builder.finalize();
  contract::CanonicalSha256Builder view_builder;
  view_builder.update(at_least20_view_step_domain);
  append_u32(
      view_builder, direct_k1_normalized_product_stream_schema_version);
  append_id(view_builder, previous_view_digest);
  append_id(view_builder, output.transition_digest);
  append_size(view_builder, normalized_batch_index);
  append_size(view_builder, output.hidden_result_count);
  append_size(view_builder, output.threshold_entry_count);
  append_size(view_builder, output.continuation_count);
  append_size(view_builder, output.multifusion_count);
  append_size(view_builder, output.active_root_count_before);
  append_size(view_builder, output.active_root_count_after);
  output.view_digest = view_builder.finalize();
  return output;
}

void bind_at_least20_fold(
    ExactDirectK1NormalizedProductBatchRecord& record,
    const contract::CanonicalId& previous_view_digest,
    const AtLeast20Fold& fold) {
  record.previous_at_least20_view_digest = previous_view_digest;
  record.at_least20_transition_digest = fold.transition_digest;
  record.at_least20_view_digest = fold.view_digest;
  record.at_least20_hidden_result_count = fold.hidden_result_count;
  record.at_least20_threshold_entry_count = fold.threshold_entry_count;
  record.at_least20_continuation_count = fold.continuation_count;
  record.at_least20_multifusion_count = fold.multifusion_count;
  record.at_least20_visible_child_reference_count =
      fold.visible_child_reference_count;
  record.active_at_least20_root_count_before =
      fold.active_root_count_before;
  record.active_at_least20_root_count_after =
      fold.active_root_count_after;
  record.at_least20_complete_compact_fold_certified = true;
}

struct HistoryPrefixes {
  std::vector<contract::CanonicalId> scientific;
  std::vector<contract::CanonicalId> at_least20;
};

[[nodiscard]] HistoryPrefixes build_history_prefixes(
    const K1CompactForest& forest,
    const contract::CanonicalId& source_forest_digest,
    const contract::CanonicalId& initial_chain_digest,
    const contract::CanonicalId& initial_at_least20_digest,
    const ExactDirectK1NormalizedProductStreamBudget& budget) {
  const std::size_t total_batch_count =
      forest.equal_level_batches.size() + 1U;
  HistoryPrefixes output;
  output.scientific.reserve(total_batch_count + 1U);
  output.at_least20.reserve(total_batch_count + 1U);
  output.scientific.push_back(initial_chain_digest);
  output.at_least20.push_back(initial_at_least20_digest);
  std::vector<std::uint8_t> active_merge_flags(
      forest.merge_nodes.size(), std::uint8_t{0U});
  std::size_t active_root_count = 0U;
  for (std::size_t batch_index = 0U;
       batch_index < total_batch_count;
       ++batch_index) {
    ExactDirectK1NormalizedProductBatchRecord record = make_record_shape(
        forest, source_forest_digest, output.scientific.back(), batch_index);
    AtLeast20Fold fold = fold_at_least20_batch(
        forest,
        batch_index,
        output.at_least20.back(),
        active_merge_flags,
        active_root_count,
        budget,
        true,
        false);
    bind_at_least20_fold(record, output.at_least20.back(), fold);
    record.scientific_chain_digest = compute_batch_chain_digest(record);
    output.scientific.push_back(record.scientific_chain_digest);
    output.at_least20.push_back(record.at_least20_view_digest);
    active_root_count = fold.active_root_count_after;
  }
  return output;
}

[[nodiscard]] exact::ExactLevel last_level_for_cursor(
    const K1CompactForest& forest, std::size_t cursor) {
  if (cursor <= 1U) {
    return exact::ExactLevel{};
  }
  return forest.levels.at(cursor - 2U);
}

[[nodiscard]] contract::CanonicalId compute_checkpoint_digest(
    const ExactDirectK1NormalizedProductStreamCheckpoint& checkpoint) {
  contract::CanonicalSha256Builder builder;
  builder.update(checkpoint_domain);
  append_u32(builder, checkpoint.schema_version);
  append_u32(builder, checkpoint.stamp.schema_version);
  // The live instance id is an operational anti-forgery capability and is
  // intentionally absent from the semantic checkpoint identity.
  append_size(builder, checkpoint.stamp.next_batch_cursor);
  append_size(builder, checkpoint.stamp.epoch);
  append_level(builder, checkpoint.stamp.last_committed_squared_level);
  append_id(builder, checkpoint.stamp.canonical_cloud_digest);
  append_id(builder, checkpoint.stamp.source_forest_digest);
  append_id(builder, checkpoint.stamp.scientific_digest);
  append_id(builder, checkpoint.stamp.committed_scientific_chain_digest);
  append_id(builder, checkpoint.stamp.at_least20_view_digest);
  append_size(builder, checkpoint.stamp.active_at_least20_root_count);
  append_size(builder, checkpoint.total_batch_count);
  append_size(builder, checkpoint.total_merge_node_count);
  append_size(builder, checkpoint.total_child_reference_count);
  append_size(builder, checkpoint.active_at_least20_root_ids.size());
  for (const K1NodeId root_id : checkpoint.active_at_least20_root_ids) {
    append_u64(builder, root_id);
  }
  append_bool(builder, checkpoint.durable_file_codec_claimed);
  append_bool(builder, checkpoint.public_status_claimed);
  return builder.finalize();
}

[[nodiscard]] bool semantic_checkpoint_equal(
    const ExactDirectK1NormalizedProductStreamCheckpoint& left,
    const ExactDirectK1NormalizedProductStreamCheckpoint& right) noexcept {
  return left.schema_version == right.schema_version &&
      left.stamp.schema_version == right.stamp.schema_version &&
      left.stamp.next_batch_cursor == right.stamp.next_batch_cursor &&
      left.stamp.epoch == right.stamp.epoch &&
      left.stamp.last_committed_squared_level ==
          right.stamp.last_committed_squared_level &&
      left.stamp.canonical_cloud_digest ==
          right.stamp.canonical_cloud_digest &&
      left.stamp.source_forest_digest == right.stamp.source_forest_digest &&
      left.stamp.scientific_digest == right.stamp.scientific_digest &&
      left.stamp.committed_scientific_chain_digest ==
          right.stamp.committed_scientific_chain_digest &&
      left.stamp.at_least20_view_digest ==
          right.stamp.at_least20_view_digest &&
      left.stamp.active_at_least20_root_count ==
          right.stamp.active_at_least20_root_count &&
      left.total_batch_count == right.total_batch_count &&
      left.total_merge_node_count == right.total_merge_node_count &&
      left.total_child_reference_count == right.total_child_reference_count &&
      left.active_at_least20_root_ids ==
          right.active_at_least20_root_ids &&
      left.state_digest == right.state_digest &&
      left.durable_file_codec_claimed == right.durable_file_codec_claimed &&
      left.public_status_claimed == right.public_status_claimed;
}

[[nodiscard]] contract::CanonicalId compute_final_seal_digest(
    const ExactDirectK1NormalizedProductFinalSeal& seal) {
  contract::CanonicalSha256Builder builder;
  builder.update(final_seal_domain);
  append_u32(builder, seal.schema_version);
  append_size(builder, seal.terminal_stamp.next_batch_cursor);
  append_size(builder, seal.terminal_stamp.epoch);
  append_level(builder, seal.terminal_stamp.last_committed_squared_level);
  append_id(builder, seal.terminal_stamp.canonical_cloud_digest);
  append_id(builder, seal.terminal_stamp.source_forest_digest);
  append_id(builder, seal.terminal_stamp.scientific_digest);
  append_id(builder, seal.terminal_stamp.committed_scientific_chain_digest);
  append_id(builder, seal.terminal_stamp.at_least20_view_digest);
  append_size(builder, seal.terminal_stamp.active_at_least20_root_count);
  append_u64(builder, seal.root_node_id);
  append_size(builder, seal.total_batch_count);
  append_size(builder, seal.total_merge_node_count);
  append_size(builder, seal.total_child_reference_count);
  append_id(builder, seal.terminal_scientific_chain_digest);
  append_id(builder, seal.terminal_at_least20_view_digest);
  append_bool(builder, seal.source_stream_exhausted);
  append_bool(builder, seal.compact_k1_root_closed);
  append_bool(builder, seal.at_least20_terminal_state_closed);
  append_bool(builder, seal.no_outstanding_prepared_capability);
  append_bool(builder, seal.gamma_star_or_delaunay_materialized);
  append_bool(builder, seal.public_status_claimed);
  return builder.finalize();
}

}  // namespace

struct ExactDirectK1NormalizedProductPreparedBatch::Impl {
  std::shared_ptr<const StreamSeal> seal;
  std::shared_ptr<PreparedTicketRegistry> registry;
  ExactDirectK1NormalizedProductBatchRecord record{};
  std::vector<ExactDirectK1NormalizedAtLeast20Transition>
      at_least20_transitions;
  std::vector<K1NodeId> at_least20_visible_child_root_ids;
  ExactDirectK1NormalizedProductStreamStamp pre_stamp{};
  ExactDirectK1NormalizedProductStreamStamp post_stamp{};
  std::size_t expected_cursor{};
  std::size_t expected_epoch{};
  contract::CanonicalId expected_chain_digest{};
  contract::CanonicalId expected_at_least20_view_digest{};
  std::size_t expected_active_at_least20_root_count{};
  std::size_t predicted_active_at_least20_root_count{};
  bool consumed{false};

  ~Impl() { release(); }

  void release() noexcept {
    if (consumed) {
      return;
    }
    consumed = true;
    if (registry != nullptr && registry->outstanding_count != 0U) {
      --registry->outstanding_count;
    }
  }
};

struct ExactDirectK1NormalizedProductStreamSession::Impl {
  K1CompactForest forest;
  ExactDirectK1NormalizedProductStreamBudget budget{};
  std::vector<contract::CanonicalId> history_prefixes;
  std::vector<contract::CanonicalId> at_least20_view_prefixes;
  std::vector<std::uint8_t> active_at_least20_merge_flags;
  std::shared_ptr<const StreamSeal> seal;
  std::shared_ptr<PreparedTicketRegistry> registry;
  std::uint64_t session_instance_id{};
  std::size_t cursor{};
  std::size_t epoch{};
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId source_forest_digest{};
  contract::CanonicalId scientific_digest{};
  contract::CanonicalId committed_chain_digest{};
  contract::CanonicalId committed_at_least20_view_digest{};
  std::size_t active_at_least20_root_count{};
  bool ready{false};
};

bool ExactDirectK1NormalizedGenesisSingletonSlice::certified()
    const noexcept {
  return root_id_equals_point_id && singleton_coverage_is_implicit &&
      count <= maximum_requested_count &&
      decision == ExactDirectK1NormalizedGenesisSliceDecision::
          complete_implicit_singleton_slice;
}

K1NodeId ExactDirectK1NormalizedGenesisSingletonSlice::root_id(
    std::size_t local_index) const {
  if (!certified() || local_index >= count ||
      first_index > std::numeric_limits<std::size_t>::max() - local_index) {
    throw std::out_of_range("a compact K1 genesis root is out of range");
  }
  return static_cast<K1NodeId>(first_index + local_index);
}

spatial::PointId ExactDirectK1NormalizedGenesisSingletonSlice::point_id(
    std::size_t local_index) const {
  const K1NodeId id = root_id(local_index);
  if (id > spatial::CanonicalPointCloud::max_point_id) {
    throw std::out_of_range("a compact K1 genesis PointId is out of range");
  }
  return static_cast<spatial::PointId>(id);
}

ExactDirectK1NormalizedProductPreparedBatch::
    ExactDirectK1NormalizedProductPreparedBatch() noexcept = default;
ExactDirectK1NormalizedProductPreparedBatch::
    ~ExactDirectK1NormalizedProductPreparedBatch() = default;
ExactDirectK1NormalizedProductPreparedBatch::
    ExactDirectK1NormalizedProductPreparedBatch(
        ExactDirectK1NormalizedProductPreparedBatch&&) noexcept = default;
ExactDirectK1NormalizedProductPreparedBatch&
ExactDirectK1NormalizedProductPreparedBatch::operator=(
    ExactDirectK1NormalizedProductPreparedBatch&&) noexcept = default;

ExactDirectK1NormalizedProductPreparedBatch::
    ExactDirectK1NormalizedProductPreparedBatch(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectK1NormalizedProductPreparedBatch::valid() const noexcept {
  return impl_ != nullptr && impl_->seal != nullptr &&
      impl_->registry != nullptr && !impl_->consumed;
}

bool ExactDirectK1NormalizedProductPreparedBatch::consumed() const noexcept {
  return impl_ == nullptr || impl_->consumed;
}

const ExactDirectK1NormalizedProductBatchRecord&
ExactDirectK1NormalizedProductPreparedBatch::prepared_record()
    const noexcept {
  static const ExactDirectK1NormalizedProductBatchRecord empty{};
  return impl_ == nullptr ? empty : impl_->record;
}

std::span<const ExactDirectK1NormalizedAtLeast20Transition>
ExactDirectK1NormalizedProductPreparedBatch::at_least20_transitions()
    const noexcept {
  return impl_ == nullptr
      ? std::span<const ExactDirectK1NormalizedAtLeast20Transition>{}
      : std::span<const ExactDirectK1NormalizedAtLeast20Transition>{
            impl_->at_least20_transitions};
}

std::span<const K1NodeId>
ExactDirectK1NormalizedProductPreparedBatch::
    at_least20_visible_child_root_ids() const noexcept {
  return impl_ == nullptr
      ? std::span<const K1NodeId>{}
      : std::span<const K1NodeId>{
            impl_->at_least20_visible_child_root_ids};
}

bool ExactDirectK1NormalizedProductPreparationResult::
    certified_prepared_batch() const noexcept {
  return ticket.has_value() && ticket->valid() &&
      no_scientific_state_mutated && all_allocations_and_digests_completed &&
      decision == ExactDirectK1NormalizedProductPrepareDecision::
          complete_certified_prepared_batch;
}

bool ExactDirectK1NormalizedProductCommitResult::
    certified_atomic_batch_commit() const noexcept {
  return record.has_value() && ticket_consumed && state_mutated &&
      no_allocation_after_preparation && scientific_chain_advanced &&
      !public_status_claimed &&
      decision == ExactDirectK1NormalizedProductCommitDecision::
          complete_certified_atomic_batch_commit &&
      post_stamp.next_batch_cursor == pre_stamp.next_batch_cursor + 1U &&
      post_stamp.epoch == pre_stamp.epoch + 1U &&
      record->scientific_chain_digest ==
          post_stamp.committed_scientific_chain_digest &&
      record->at_least20_view_digest == post_stamp.at_least20_view_digest &&
      record->active_at_least20_root_count_after ==
          post_stamp.active_at_least20_root_count &&
      record->at_least20_threshold_entry_count +
              record->at_least20_continuation_count +
              record->at_least20_multifusion_count ==
          at_least20_transitions.size() &&
      record->at_least20_visible_child_reference_count ==
          at_least20_visible_child_root_ids.size();
}

bool ExactDirectK1NormalizedProductCheckpointResult::certified_checkpoint()
    const noexcept {
  return checkpoint.has_value() &&
      decision == ExactDirectK1NormalizedProductCheckpointDecision::
          complete_semantic_checkpoint;
}

bool ExactDirectK1NormalizedProductFinalSeal::certified_terminal_closure()
    const noexcept {
  return terminal_stamp.session_instance_id != 0U &&
      terminal_stamp.next_batch_cursor == total_batch_count &&
      terminal_stamp.epoch == total_batch_count &&
      terminal_stamp.committed_scientific_chain_digest ==
          terminal_scientific_chain_digest &&
      terminal_stamp.at_least20_view_digest ==
          terminal_at_least20_view_digest &&
      !id_is_zero(seal_digest) && source_stream_exhausted &&
      compact_k1_root_closed && at_least20_terminal_state_closed &&
      no_outstanding_prepared_capability &&
      !gamma_star_or_delaunay_materialized && !public_status_claimed &&
      decision == ExactDirectK1NormalizedProductFinalSealDecision::
          complete_certified_terminal_closure;
}

ExactDirectK1NormalizedProductStreamSession::
    ExactDirectK1NormalizedProductStreamSession() noexcept = default;
ExactDirectK1NormalizedProductStreamSession::
    ~ExactDirectK1NormalizedProductStreamSession() = default;
ExactDirectK1NormalizedProductStreamSession::
    ExactDirectK1NormalizedProductStreamSession(
        ExactDirectK1NormalizedProductStreamSession&&) noexcept = default;
ExactDirectK1NormalizedProductStreamSession&
ExactDirectK1NormalizedProductStreamSession::operator=(
    ExactDirectK1NormalizedProductStreamSession&&) noexcept = default;

ExactDirectK1NormalizedProductStreamSession::
    ExactDirectK1NormalizedProductStreamSession(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectK1NormalizedProductStreamSession::certified_stream()
    const noexcept {
  return impl_ != nullptr && impl_->ready && impl_->seal != nullptr &&
      impl_->registry != nullptr && impl_->session_instance_id != 0U &&
      !id_is_zero(impl_->canonical_cloud_digest) &&
      !id_is_zero(impl_->source_forest_digest) &&
      !id_is_zero(impl_->scientific_digest) &&
      !id_is_zero(impl_->committed_chain_digest) &&
      !id_is_zero(impl_->committed_at_least20_view_digest) &&
      impl_->history_prefixes.size() == total_batch_count() + 1U &&
      impl_->at_least20_view_prefixes.size() == total_batch_count() + 1U &&
      impl_->active_at_least20_merge_flags.size() ==
          impl_->forest.merge_nodes.size() &&
      impl_->cursor < impl_->history_prefixes.size() &&
      impl_->committed_chain_digest == impl_->history_prefixes[impl_->cursor] &&
      impl_->committed_at_least20_view_digest ==
          impl_->at_least20_view_prefixes[impl_->cursor] &&
      impl_->active_at_least20_root_count <=
          impl_->budget.maximum_active_at_least20_root_count;
}

bool ExactDirectK1NormalizedProductStreamSession::complete() const noexcept {
  return certified_stream() && impl_->cursor == total_batch_count();
}

std::size_t ExactDirectK1NormalizedProductStreamSession::point_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->forest.point_count;
}

std::size_t ExactDirectK1NormalizedProductStreamSession::total_batch_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->forest.equal_level_batches.size() + 1U;
}

std::size_t ExactDirectK1NormalizedProductStreamSession::next_batch_cursor()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->cursor;
}

std::size_t ExactDirectK1NormalizedProductStreamSession::epoch()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->epoch;
}

std::size_t ExactDirectK1NormalizedProductStreamSession::
    outstanding_prepared_batch_count() const noexcept {
  return impl_ == nullptr || impl_->registry == nullptr
      ? 0U
      : impl_->registry->outstanding_count;
}

K1NodeId ExactDirectK1NormalizedProductStreamSession::root_node_id()
    const noexcept {
  return impl_ == nullptr ? K1NodeId{} : impl_->forest.root_node_id;
}

const contract::CanonicalId&
ExactDirectK1NormalizedProductStreamSession::canonical_cloud_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->canonical_cloud_digest;
}

const contract::CanonicalId&
ExactDirectK1NormalizedProductStreamSession::source_forest_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->source_forest_digest;
}

const contract::CanonicalId&
ExactDirectK1NormalizedProductStreamSession::scientific_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->scientific_digest;
}

const contract::CanonicalId&
ExactDirectK1NormalizedProductStreamSession::scientific_chain_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->committed_chain_digest;
}

const contract::CanonicalId&
ExactDirectK1NormalizedProductStreamSession::at_least20_view_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->committed_at_least20_view_digest;
}

std::size_t
ExactDirectK1NormalizedProductStreamSession::active_at_least20_root_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->active_at_least20_root_count;
}

bool ExactDirectK1NormalizedProductStreamSession::at_least20_root_active(
    K1NodeId root_id) const noexcept {
  if (impl_ == nullptr || root_id < impl_->forest.point_count) {
    return false;
  }
  const std::size_t merge_index =
      static_cast<std::size_t>(root_id) - impl_->forest.point_count;
  return merge_index < impl_->active_at_least20_merge_flags.size() &&
      impl_->forest.merge_nodes[merge_index].id == root_id &&
      impl_->active_at_least20_merge_flags[merge_index] != 0U;
}

ExactDirectK1NormalizedProductStreamStamp
ExactDirectK1NormalizedProductStreamSession::current_stamp() const {
  ExactDirectK1NormalizedProductStreamStamp output;
  if (!certified_stream()) {
    return output;
  }
  output.session_instance_id = impl_->session_instance_id;
  output.next_batch_cursor = impl_->cursor;
  output.epoch = impl_->epoch;
  output.last_committed_squared_level =
      last_level_for_cursor(impl_->forest, impl_->cursor);
  output.canonical_cloud_digest = impl_->canonical_cloud_digest;
  output.source_forest_digest = impl_->source_forest_digest;
  output.scientific_digest = impl_->scientific_digest;
  output.committed_scientific_chain_digest = impl_->committed_chain_digest;
  output.at_least20_view_digest = impl_->committed_at_least20_view_digest;
  output.active_at_least20_root_count =
      impl_->active_at_least20_root_count;
  return output;
}

ExactDirectK1NormalizedProductPreparationResult
ExactDirectK1NormalizedProductStreamSession::prepare_next() noexcept {
  ExactDirectK1NormalizedProductPreparationResult output;
  output.no_scientific_state_mutated = true;
  if (!certified_stream()) {
    output.decision =
        ExactDirectK1NormalizedProductPrepareDecision::no_session_not_ready;
    return output;
  }
  if (complete()) {
    output.decision =
        ExactDirectK1NormalizedProductPrepareDecision::no_stream_complete;
    return output;
  }
  if (impl_->registry->outstanding_count >=
      impl_->budget.maximum_outstanding_prepared_batch_count) {
    output.decision = ExactDirectK1NormalizedProductPrepareDecision::
        no_outstanding_ticket_budget_exhausted;
    return output;
  }

  try {
    auto prepared = std::make_unique<
        ExactDirectK1NormalizedProductPreparedBatch::Impl>();
    prepared->seal = impl_->seal;
    prepared->registry = impl_->registry;
    prepared->expected_cursor = impl_->cursor;
    prepared->expected_epoch = impl_->epoch;
    prepared->expected_chain_digest = impl_->committed_chain_digest;
    prepared->expected_at_least20_view_digest =
        impl_->committed_at_least20_view_digest;
    prepared->expected_active_at_least20_root_count =
        impl_->active_at_least20_root_count;
    prepared->record = make_record_shape(
        impl_->forest,
        impl_->source_forest_digest,
        impl_->committed_chain_digest,
        impl_->cursor);
    AtLeast20Fold at_least20_fold = fold_at_least20_batch(
        impl_->forest,
        impl_->cursor,
        impl_->committed_at_least20_view_digest,
        impl_->active_at_least20_merge_flags,
        impl_->active_at_least20_root_count,
        impl_->budget,
        false,
        true);
    bind_at_least20_fold(
        prepared->record,
        impl_->committed_at_least20_view_digest,
        at_least20_fold);
    prepared->predicted_active_at_least20_root_count =
        at_least20_fold.active_root_count_after;
    prepared->at_least20_transitions =
        std::move(at_least20_fold.transitions);
    prepared->at_least20_visible_child_root_ids =
        std::move(at_least20_fold.visible_child_root_ids);
    prepared->record.scientific_chain_digest =
        impl_->history_prefixes.at(impl_->cursor + 1U);
    if (prepared->record.at_least20_view_digest !=
            impl_->at_least20_view_prefixes.at(impl_->cursor + 1U) ||
        compute_batch_chain_digest(prepared->record) !=
            prepared->record.scientific_chain_digest) {
      output.decision = ExactDirectK1NormalizedProductPrepareDecision::
          no_session_not_ready;
      return output;
    }
    prepared->pre_stamp = current_stamp();
    prepared->post_stamp = prepared->pre_stamp;
    ++prepared->post_stamp.next_batch_cursor;
    ++prepared->post_stamp.epoch;
    prepared->post_stamp.last_committed_squared_level =
        prepared->record.squared_level;
    prepared->post_stamp.committed_scientific_chain_digest =
        prepared->record.scientific_chain_digest;
    prepared->post_stamp.at_least20_view_digest =
        prepared->record.at_least20_view_digest;
    prepared->post_stamp.active_at_least20_root_count =
        prepared->predicted_active_at_least20_root_count;
    ++impl_->registry->outstanding_count;
    output.ticket.emplace(
        ExactDirectK1NormalizedProductPreparedBatch{std::move(prepared)});
    output.all_allocations_and_digests_completed = true;
    output.decision = ExactDirectK1NormalizedProductPrepareDecision::
        complete_certified_prepared_batch;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision = ExactDirectK1NormalizedProductPrepareDecision::
        no_allocation_failed;
    return output;
  } catch (const std::exception&) {
    output.decision =
        ExactDirectK1NormalizedProductPrepareDecision::no_session_not_ready;
    return output;
  }
}

ExactDirectK1NormalizedProductCommitResult
ExactDirectK1NormalizedProductStreamSession::commit(
    ExactDirectK1NormalizedProductPreparedBatch&& ticket) noexcept {
  ExactDirectK1NormalizedProductCommitResult output;
  if (!certified_stream()) {
    if (ticket.impl_ != nullptr) {
      ticket.impl_->release();
      output.ticket_consumed = true;
    }
    output.decision =
        ExactDirectK1NormalizedProductCommitDecision::no_session_not_ready;
    return output;
  }
  if (!ticket.valid()) {
    output.decision =
        ExactDirectK1NormalizedProductCommitDecision::no_ticket_not_valid;
    return output;
  }
  if (ticket.impl_->seal != impl_->seal ||
      ticket.impl_->registry != impl_->registry) {
    ticket.impl_->release();
    output.ticket_consumed = true;
    output.decision = ExactDirectK1NormalizedProductCommitDecision::
        no_foreign_ticket_rejected;
    return output;
  }
  if (ticket.impl_->expected_cursor != impl_->cursor ||
      ticket.impl_->expected_epoch != impl_->epoch ||
      ticket.impl_->expected_chain_digest != impl_->committed_chain_digest ||
      ticket.impl_->expected_at_least20_view_digest !=
          impl_->committed_at_least20_view_digest ||
      ticket.impl_->expected_active_at_least20_root_count !=
          impl_->active_at_least20_root_count ||
      ticket.impl_->record.normalized_batch_index != impl_->cursor ||
      ticket.impl_->record.previous_scientific_chain_digest !=
          impl_->committed_chain_digest ||
      ticket.impl_->record.scientific_chain_digest !=
          impl_->history_prefixes[impl_->cursor + 1U] ||
      ticket.impl_->record.previous_at_least20_view_digest !=
          impl_->committed_at_least20_view_digest ||
      ticket.impl_->record.at_least20_view_digest !=
          impl_->at_least20_view_prefixes[impl_->cursor + 1U] ||
      ticket.impl_->predicted_active_at_least20_root_count !=
          ticket.impl_->record.active_at_least20_root_count_after) {
    ticket.impl_->release();
    output.ticket_consumed = true;
    output.decision = ExactDirectK1NormalizedProductCommitDecision::
        no_stale_sibling_ticket_rejected;
    return output;
  }

  for (const K1NodeId child_id :
       ticket.impl_->at_least20_visible_child_root_ids) {
    if (child_id < impl_->forest.point_count) {
      ticket.impl_->release();
      output.ticket_consumed = true;
      output.decision = ExactDirectK1NormalizedProductCommitDecision::
          no_stale_sibling_ticket_rejected;
      return output;
    }
    const std::size_t merge_index =
        static_cast<std::size_t>(child_id) - impl_->forest.point_count;
    if (merge_index >= impl_->active_at_least20_merge_flags.size() ||
        impl_->active_at_least20_merge_flags[merge_index] == 0U) {
      ticket.impl_->release();
      output.ticket_consumed = true;
      output.decision = ExactDirectK1NormalizedProductCommitDecision::
          no_stale_sibling_ticket_rejected;
      return output;
    }
  }
  for (const auto& transition : ticket.impl_->at_least20_transitions) {
    if (transition.resultant_root_id < impl_->forest.point_count) {
      ticket.impl_->release();
      output.ticket_consumed = true;
      output.decision = ExactDirectK1NormalizedProductCommitDecision::
          no_stale_sibling_ticket_rejected;
      return output;
    }
    const std::size_t merge_index =
        static_cast<std::size_t>(transition.resultant_root_id) -
        impl_->forest.point_count;
    if (merge_index >= impl_->active_at_least20_merge_flags.size() ||
        impl_->active_at_least20_merge_flags[merge_index] != 0U) {
      ticket.impl_->release();
      output.ticket_consumed = true;
      output.decision = ExactDirectK1NormalizedProductCommitDecision::
          no_stale_sibling_ticket_rejected;
      return output;
    }
  }

  // Every potentially allocating operation, including ExactLevel copies and
  // SHA-256 computation, completed in prepare_next().  The moves below are
  // statically no-throw and the scientific state changes only after all
  // validity checks have closed.
  output.pre_stamp = std::move(ticket.impl_->pre_stamp);
  output.post_stamp = std::move(ticket.impl_->post_stamp);
  output.record.emplace(std::move(ticket.impl_->record));
  output.at_least20_transitions =
      std::move(ticket.impl_->at_least20_transitions);
  output.at_least20_visible_child_root_ids =
      std::move(ticket.impl_->at_least20_visible_child_root_ids);
  for (const K1NodeId child_id : output.at_least20_visible_child_root_ids) {
    const std::size_t merge_index =
        static_cast<std::size_t>(child_id) - impl_->forest.point_count;
    impl_->active_at_least20_merge_flags[merge_index] = 0U;
  }
  for (const auto& transition : output.at_least20_transitions) {
    const std::size_t merge_index =
        static_cast<std::size_t>(transition.resultant_root_id) -
        impl_->forest.point_count;
    impl_->active_at_least20_merge_flags[merge_index] = 1U;
  }
  ticket.impl_->release();
  impl_->cursor = output.post_stamp.next_batch_cursor;
  impl_->epoch = output.post_stamp.epoch;
  impl_->committed_chain_digest =
      output.post_stamp.committed_scientific_chain_digest;
  impl_->committed_at_least20_view_digest =
      output.post_stamp.at_least20_view_digest;
  impl_->active_at_least20_root_count =
      output.post_stamp.active_at_least20_root_count;
  output.ticket_consumed = true;
  output.state_mutated = true;
  output.no_allocation_after_preparation = true;
  output.scientific_chain_advanced =
      output.pre_stamp.committed_scientific_chain_digest !=
      output.post_stamp.committed_scientific_chain_digest;
  output.public_status_claimed = false;
  output.decision = ExactDirectK1NormalizedProductCommitDecision::
      complete_certified_atomic_batch_commit;
  return output;
}

ExactDirectK1NormalizedProductBatchView
ExactDirectK1NormalizedProductStreamSession::private_batch_view(
    std::size_t normalized_batch_index) const noexcept {
  ExactDirectK1NormalizedProductBatchView output;
  if (impl_ == nullptr || normalized_batch_index >= total_batch_count()) {
    return output;
  }
  if (normalized_batch_index == 0U) {
    output.kind =
        ExactDirectK1NormalizedProductBatchKind::implicit_singleton_genesis;
    output.implicit_singleton_count = impl_->forest.point_count;
    return output;
  }
  const auto& batch =
      impl_->forest.equal_level_batches[normalized_batch_index - 1U];
  output.kind = ExactDirectK1NormalizedProductBatchKind::
      compact_equal_level_multifusions;
  output.merge_nodes = std::span<const K1CompactMergeNode>{
      impl_->forest.merge_nodes}.subspan(
      batch.merge_node_offset, batch.merge_node_count);
  if (!output.merge_nodes.empty()) {
    const std::size_t child_offset = output.merge_nodes.front().child_offset;
    const std::size_t child_end =
        output.merge_nodes.back().child_offset +
        output.merge_nodes.back().child_count;
    output.child_root_ids = std::span<const K1NodeId>{impl_->forest.child_ids}
                                .subspan(
                                    child_offset,
                                    child_end - child_offset);
  }
  output.at_least20_prior_roots_are_csr_children = true;
  output.at_least20_coverage_delta_is_empty_after_genesis = true;
  return output;
}

const K1CompactForest&
ExactDirectK1NormalizedProductStreamSession::private_forest()
    const noexcept {
  static const K1CompactForest empty{};
  return impl_ == nullptr ? empty : impl_->forest;
}

std::optional<ExactDirectK1NormalizedProductBatchView>
ExactDirectK1NormalizedProductStreamSession::batch_view(
    const ExactDirectK1NormalizedProductBatchRecord& record) const noexcept {
  if (!certified_stream() ||
      record.schema_version !=
          direct_k1_normalized_product_stream_schema_version ||
      record.normalized_batch_index >= total_batch_count() ||
      record.normalized_batch_index > impl_->cursor ||
      record.order != direct_k1_normalized_product_stream_order ||
      record.order_batch_index != record.normalized_batch_index ||
      record.source_forest_digest != impl_->source_forest_digest ||
      record.previous_scientific_chain_digest !=
          impl_->history_prefixes[record.normalized_batch_index] ||
      record.scientific_chain_digest !=
          impl_->history_prefixes[record.normalized_batch_index + 1U] ||
      record.previous_at_least20_view_digest !=
          impl_->at_least20_view_prefixes[record.normalized_batch_index] ||
      record.at_least20_view_digest !=
          impl_->at_least20_view_prefixes[record.normalized_batch_index + 1U] ||
      !record.complete_qr_partition_certified ||
      !record.exact_parent_node_point_deltas_certified ||
      !record.compact_csr_references_certified ||
      !record.closed_zero_level_partition_certified ||
      !record.at_least20_complete_compact_fold_certified ||
      record.gamma_star_or_delaunay_materialized ||
      record.public_status_claimed) {
    return std::nullopt;
  }

  const auto view = private_batch_view(record.normalized_batch_index);
  if (record.normalized_batch_index == 0U) {
    if (record.kind != ExactDirectK1NormalizedProductBatchKind::
                           implicit_singleton_genesis ||
        record.squared_level != exact::ExactLevel{} ||
        record.group_count != impl_->forest.point_count ||
        record.qr0_group_count != impl_->forest.point_count ||
        record.qr1_group_count != 0U ||
        record.qr_greater_equal2_group_count != 0U ||
        record.core_facet_delta_count != impl_->forest.point_count ||
        record.parent_delta_count != 0U ||
        record.node_delta_count != impl_->forest.point_count ||
        record.point_delta_count != impl_->forest.point_count ||
        record.merge_node_offset != 0U || record.merge_node_count != 0U ||
        record.child_reference_offset != 0U ||
        record.child_reference_count != 0U ||
        record.implicit_singleton_first_root_id != K1NodeId{0U} ||
        record.implicit_singleton_count != impl_->forest.point_count ||
        record.at_least20_hidden_result_count != impl_->forest.point_count ||
        record.at_least20_threshold_entry_count != 0U ||
        record.at_least20_continuation_count != 0U ||
        record.at_least20_multifusion_count != 0U ||
        record.at_least20_visible_child_reference_count != 0U ||
        record.active_at_least20_root_count_before != 0U ||
        record.active_at_least20_root_count_after != 0U ||
        !record.implicit_singleton_genesis_without_group_materialization) {
      return std::nullopt;
    }
    return view;
  }

  const auto& source_batch =
      impl_->forest.equal_level_batches[record.normalized_batch_index - 1U];
  if (record.kind != ExactDirectK1NormalizedProductBatchKind::
                         compact_equal_level_multifusions ||
      record.squared_level !=
          impl_->forest.levels[source_batch.level_index] ||
      record.group_count != source_batch.merge_node_count ||
      record.qr0_group_count != 0U || record.qr1_group_count != 0U ||
      record.qr_greater_equal2_group_count !=
          source_batch.merge_node_count ||
      record.core_facet_delta_count != 0U ||
      record.parent_delta_count != view.child_root_ids.size() ||
      record.node_delta_count != source_batch.merge_node_count ||
      record.point_delta_count != 0U ||
      record.merge_node_offset != source_batch.merge_node_offset ||
      record.merge_node_count != source_batch.merge_node_count ||
      record.child_reference_offset !=
          (view.merge_nodes.empty() ? 0U
                                    : view.merge_nodes.front().child_offset) ||
      record.child_reference_count != view.child_root_ids.size() ||
      record.implicit_singleton_first_root_id != K1NodeId{} ||
      record.implicit_singleton_count != 0U ||
      record.implicit_singleton_genesis_without_group_materialization ||
      !view.at_least20_prior_roots_are_csr_children ||
      !view.at_least20_coverage_delta_is_empty_after_genesis) {
    return std::nullopt;
  }
  const std::size_t visible_transition_count =
      record.at_least20_threshold_entry_count +
      record.at_least20_continuation_count +
      record.at_least20_multifusion_count;
  if (record.at_least20_hidden_result_count + visible_transition_count !=
          record.group_count ||
      record.active_at_least20_root_count_before <
          record.at_least20_visible_child_reference_count ||
      record.active_at_least20_root_count_before -
                  record.at_least20_visible_child_reference_count +
              visible_transition_count !=
          record.active_at_least20_root_count_after) {
    return std::nullopt;
  }
  for (const K1CompactMergeNode& node : view.merge_nodes) {
    if (node.child_count < 2U ||
        node.child_offset < record.child_reference_offset ||
        node.child_offset + node.child_count >
            record.child_reference_offset + record.child_reference_count) {
      return std::nullopt;
    }
  }
  return view;
}

ExactDirectK1NormalizedGenesisSingletonSlice
ExactDirectK1NormalizedProductStreamSession::genesis_slice(
    const ExactDirectK1NormalizedProductBatchRecord& genesis_record,
    std::size_t first_index,
    std::size_t count,
    std::size_t maximum_requested_count) const noexcept {
  ExactDirectK1NormalizedGenesisSingletonSlice output;
  output.first_index = first_index;
  output.count = count;
  output.maximum_requested_count = maximum_requested_count;
  const auto view = batch_view(genesis_record);
  if (!view.has_value() ||
      view->kind != ExactDirectK1NormalizedProductBatchKind::
                        implicit_singleton_genesis) {
    output.decision = ExactDirectK1NormalizedGenesisSliceDecision::
        no_record_not_authentic_genesis;
    return output;
  }
  if (count > maximum_requested_count) {
    output.decision = ExactDirectK1NormalizedGenesisSliceDecision::
        no_requested_slice_budget_exhausted;
    return output;
  }
  if (first_index > view->implicit_singleton_count ||
      count > view->implicit_singleton_count - first_index) {
    output.decision = ExactDirectK1NormalizedGenesisSliceDecision::
        no_requested_slice_out_of_range;
    return output;
  }
  output.root_id_equals_point_id = true;
  output.singleton_coverage_is_implicit = true;
  output.decision = ExactDirectK1NormalizedGenesisSliceDecision::
      complete_implicit_singleton_slice;
  return output;
}

ExactDirectK1NormalizedProductCheckpointResult
ExactDirectK1NormalizedProductStreamSession::export_checkpoint()
    const noexcept {
  ExactDirectK1NormalizedProductCheckpointResult output;
  if (!certified_stream()) {
    output.decision = ExactDirectK1NormalizedProductCheckpointDecision::
        no_session_not_ready;
    return output;
  }
  if (outstanding_prepared_batch_count() != 0U) {
    output.decision = ExactDirectK1NormalizedProductCheckpointDecision::
        no_outstanding_prepared_ticket;
    return output;
  }
  if (impl_->active_at_least20_root_count >
      impl_->budget.maximum_checkpoint_active_at_least20_root_count) {
    output.decision = ExactDirectK1NormalizedProductCheckpointDecision::
        no_checkpoint_budget_exhausted;
    return output;
  }
  try {
    ExactDirectK1NormalizedProductStreamCheckpoint checkpoint;
    checkpoint.stamp = current_stamp();
    checkpoint.total_batch_count = total_batch_count();
    checkpoint.total_merge_node_count = impl_->forest.merge_nodes.size();
    checkpoint.total_child_reference_count = impl_->forest.child_ids.size();
    checkpoint.active_at_least20_root_ids.reserve(
        impl_->active_at_least20_root_count);
    for (std::size_t merge_index = 0U;
         merge_index < impl_->active_at_least20_merge_flags.size();
         ++merge_index) {
      if (impl_->active_at_least20_merge_flags[merge_index] != 0U) {
        checkpoint.active_at_least20_root_ids.push_back(
            impl_->forest.merge_nodes[merge_index].id);
      }
    }
    if (checkpoint.active_at_least20_root_ids.size() !=
        impl_->active_at_least20_root_count) {
      output.decision = ExactDirectK1NormalizedProductCheckpointDecision::
          no_session_not_ready;
      return output;
    }
    checkpoint.state_digest = compute_checkpoint_digest(checkpoint);
    output.checkpoint.emplace(std::move(checkpoint));
    output.decision = ExactDirectK1NormalizedProductCheckpointDecision::
        complete_semantic_checkpoint;
    return output;
  } catch (const std::exception&) {
    output.decision = ExactDirectK1NormalizedProductCheckpointDecision::
        no_allocation_failed;
    return output;
  }
}

bool ExactDirectK1NormalizedProductStreamSession::verify_checkpoint(
    const ExactDirectK1NormalizedProductStreamCheckpoint& checkpoint)
    const noexcept {
  const auto expected = export_checkpoint();
  return expected.certified_checkpoint() &&
      expected.checkpoint.value() == checkpoint;
}

ExactDirectK1NormalizedProductFinalSeal
ExactDirectK1NormalizedProductStreamSession::seal() const noexcept {
  ExactDirectK1NormalizedProductFinalSeal output;
  if (!certified_stream()) {
    output.decision = ExactDirectK1NormalizedProductFinalSealDecision::
        no_session_not_ready;
    return output;
  }
  if (!complete()) {
    output.decision = ExactDirectK1NormalizedProductFinalSealDecision::
        no_stream_not_complete;
    return output;
  }
  if (outstanding_prepared_batch_count() != 0U) {
    output.decision = ExactDirectK1NormalizedProductFinalSealDecision::
        no_outstanding_prepared_ticket;
    return output;
  }
  const bool root_should_be_visible =
      impl_->forest.point_count >=
      direct_k1_normalized_product_stream_at_least20_threshold;
  const std::size_t expected_active_count = root_should_be_visible ? 1U : 0U;
  if (impl_->active_at_least20_root_count != expected_active_count ||
      (root_should_be_visible !=
       at_least20_root_active(impl_->forest.root_node_id))) {
    output.decision = ExactDirectK1NormalizedProductFinalSealDecision::
        no_terminal_at_least20_state_rejected;
    return output;
  }
  try {
    output.terminal_stamp = current_stamp();
    output.root_node_id = impl_->forest.root_node_id;
    output.total_batch_count = total_batch_count();
    output.total_merge_node_count = impl_->forest.merge_nodes.size();
    output.total_child_reference_count = impl_->forest.child_ids.size();
    output.terminal_scientific_chain_digest = impl_->committed_chain_digest;
    output.terminal_at_least20_view_digest =
        impl_->committed_at_least20_view_digest;
    output.source_stream_exhausted = true;
    output.compact_k1_root_closed = true;
    output.at_least20_terminal_state_closed = true;
    output.no_outstanding_prepared_capability = true;
    output.seal_digest = compute_final_seal_digest(output);
    output.decision = ExactDirectK1NormalizedProductFinalSealDecision::
        complete_certified_terminal_closure;
    return output;
  } catch (const std::exception&) {
    output.decision = ExactDirectK1NormalizedProductFinalSealDecision::
        no_allocation_failed;
    return output;
  }
}

bool ExactDirectK1NormalizedProductStreamSession::verify_final_seal(
    const ExactDirectK1NormalizedProductFinalSeal& candidate) const noexcept {
  const auto expected = seal();
  return expected.certified_terminal_closure() && expected == candidate;
}

bool ExactDirectK1NormalizedProductStreamInitialization::
    certified_initialized_stream() const noexcept {
  return session.certified_stream() &&
      allocation_free_structural_preflight_certified &&
      boruvka_authority_freshly_replayed &&
      source_forest_digest_reused_from_k1_authority &&
      compact_k1_forest_certified &&
      implicit_genesis_and_csr_stream_certified &&
      at_least20_complete_compact_view_certified &&
      !gamma_star_or_delaunay_materialized && !public_status_claimed &&
      decision == ExactDirectK1NormalizedProductStreamInitializationDecision::
          complete_certified_stream;
}

ExactDirectK1NormalizedProductStreamInitialization
initialize_exact_direct_k1_normalized_product_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& observed_boruvka,
    const ExactDirectK1BoruvkaClosedCutSessionBudget& k1_authority_budget,
    const ExactDirectK1NormalizedProductStreamBudget& stream_budget) noexcept {
  ExactDirectK1NormalizedProductStreamInitialization output;
  output.requested_budget = stream_budget;
  bool overflow = false;
  output.requirements = derive_requirements(cloud.size(), overflow);
  if (overflow) {
    output.decision = ExactDirectK1NormalizedProductStreamInitializationDecision::
        no_preflight_capacity_overflow;
    return output;
  }
  if (!budget_covers(output.requirements, stream_budget)) {
    output.decision = ExactDirectK1NormalizedProductStreamInitializationDecision::
        no_preflight_budget_exhausted;
    return output;
  }
  output.allocation_free_structural_preflight_certified = true;

  auto authority = build_exact_direct_k1_boruvka_closed_cut_session(
      index, cloud, observed_boruvka, k1_authority_budget);
  if (!authority.certified_ready_session()) {
    output.decision = ExactDirectK1NormalizedProductStreamInitializationDecision::
        no_k1_boruvka_authority_rejected;
    return output;
  }
  output.canonical_cloud_digest = authority.canonical_cloud_digest;
  output.source_forest_digest = authority.source_forest_digest;
  output.replayed_boruvka_round_count =
      authority.replayed_boruvka_round_count;
  output.replayed_boruvka_component_minimum_count =
      authority.replayed_boruvka_component_minimum_count;
  output.boruvka_authority_freshly_replayed =
      authority.boruvka_authority_freshly_replayed;
  output.source_forest_digest_reused_from_k1_authority =
      !id_is_zero(authority.source_forest_digest);
  // Release the authenticated closed-cut forest before constructing the one
  // compact forest owned by this stream.  Peak persistent ownership therefore
  // stays linear and no second forest is retained.
  authority.session = ExactDirectK1BoruvkaClosedCutSession{};

  try {
    K1CompactForest forest = build_compact_k1_forest(
        cloud.size(), observed_boruvka.emst_edges);
    const std::size_t total_batch_count =
        forest.equal_level_batches.size() + 1U;
    bool all_merge_levels_strictly_positive = true;
    for (const exact::ExactLevel& level : forest.levels) {
      if (level == exact::ExactLevel{}) {
        all_merge_levels_strictly_positive = false;
        break;
      }
    }
    if (forest.point_count != output.requirements.point_count ||
        !all_merge_levels_strictly_positive ||
        total_batch_count >
            output.requirements.product_batch_count_upper_bound ||
        forest.merge_nodes.size() >
            output.requirements.merge_node_count_upper_bound ||
        forest.child_ids.size() >
            output.requirements.child_reference_count_upper_bound ||
        total_batch_count > stream_budget.maximum_product_batch_count ||
        forest.merge_nodes.size() > stream_budget.maximum_merge_node_count ||
        forest.child_ids.size() >
            stream_budget.maximum_child_reference_count) {
      output.decision = ExactDirectK1NormalizedProductStreamInitializationDecision::
          no_authenticated_forest_shape_mismatch;
      return output;
    }

    output.scientific_digest = compute_scientific_digest(
        output.canonical_cloud_digest,
        output.source_forest_digest,
        forest.point_count);
    output.initial_scientific_chain_digest = compute_initial_chain_digest(
        output.scientific_digest, output.source_forest_digest);
    output.initial_at_least20_view_digest =
        compute_at_least20_initial_digest(
            output.scientific_digest, output.source_forest_digest);
    HistoryPrefixes histories = build_history_prefixes(
        forest,
        output.source_forest_digest,
        output.initial_scientific_chain_digest,
        output.initial_at_least20_view_digest,
        stream_budget);
    if (histories.scientific.size() != total_batch_count + 1U ||
        histories.at_least20.size() != total_batch_count + 1U ||
        histories.scientific.size() >
            stream_budget.maximum_history_digest_count ||
        histories.at_least20.size() >
            stream_budget.maximum_history_digest_count) {
      output.decision = ExactDirectK1NormalizedProductStreamInitializationDecision::
          no_authenticated_forest_shape_mismatch;
      return output;
    }

    auto impl = std::make_unique<
        ExactDirectK1NormalizedProductStreamSession::Impl>();
    impl->forest = std::move(forest);
    impl->budget = stream_budget;
    impl->history_prefixes = std::move(histories.scientific);
    impl->at_least20_view_prefixes = std::move(histories.at_least20);
    impl->active_at_least20_merge_flags.assign(
        impl->forest.merge_nodes.size(), std::uint8_t{0U});
    impl->seal = std::make_shared<const StreamSeal>();
    impl->registry = std::make_shared<PreparedTicketRegistry>();
    impl->session_instance_id = allocate_session_instance_id();
    if (impl->session_instance_id == 0U) {
      output.decision = ExactDirectK1NormalizedProductStreamInitializationDecision::
          no_session_instance_id_exhausted;
      return output;
    }
    impl->canonical_cloud_digest = output.canonical_cloud_digest;
    impl->source_forest_digest = output.source_forest_digest;
    impl->scientific_digest = output.scientific_digest;
    impl->committed_chain_digest =
        output.initial_scientific_chain_digest;
    impl->committed_at_least20_view_digest =
        output.initial_at_least20_view_digest;
    impl->ready = true;

    output.exact_product_batch_count = total_batch_count;
    output.exact_merge_node_count = impl->forest.merge_nodes.size();
    output.exact_child_reference_count = impl->forest.child_ids.size();
    output.compact_k1_forest_certified = true;
    output.implicit_genesis_and_csr_stream_certified = true;
    output.at_least20_complete_compact_view_certified = true;
    output.session = ExactDirectK1NormalizedProductStreamSession{
        std::move(impl)};
    output.decision = ExactDirectK1NormalizedProductStreamInitializationDecision::
        complete_certified_stream;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision = ExactDirectK1NormalizedProductStreamInitializationDecision::
        no_allocation_failed;
    return output;
  } catch (const std::exception&) {
    output.decision = ExactDirectK1NormalizedProductStreamInitializationDecision::
        no_compact_k1_forest_rejected;
    return output;
  }
}

bool ExactDirectK1NormalizedProductStreamRestore::certified_restored_stream()
    const noexcept {
  return session.certified_stream() &&
      receipt.boruvka_authority_freshly_replayed &&
      receipt.compact_k1_forest_freshly_rebuilt &&
      receipt.product_prefix_freshly_replayed &&
      receipt.at_least20_view_freshly_replayed &&
      receipt.semantic_checkpoint_equality_certified &&
      receipt.session_instance_ids_distinct &&
      !receipt.durable_file_codec_claimed &&
      !receipt.gamma_star_or_delaunay_materialized &&
      !receipt.public_status_claimed &&
      receipt.decision == ExactDirectK1NormalizedProductRestoreDecision::
          complete_certified_fresh_semantic_replay_restore;
}

ExactDirectK1NormalizedProductStreamRestore
restore_exact_direct_k1_normalized_product_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& observed_boruvka,
    const ExactDirectK1NormalizedProductStreamCheckpoint& checkpoint,
    const ExactDirectK1BoruvkaClosedCutSessionBudget& k1_authority_budget,
    const ExactDirectK1NormalizedProductStreamBudget& stream_budget) noexcept {
  ExactDirectK1NormalizedProductStreamRestore output;
  output.receipt.checkpoint_stamp = checkpoint.stamp;
  try {
    if (checkpoint.schema_version !=
            direct_k1_normalized_product_stream_checkpoint_schema_version ||
        checkpoint.stamp.schema_version !=
            direct_k1_normalized_product_stream_schema_version ||
        checkpoint.stamp.session_instance_id == 0U ||
        checkpoint.stamp.epoch != checkpoint.stamp.next_batch_cursor ||
        checkpoint.stamp.next_batch_cursor > checkpoint.total_batch_count ||
        id_is_zero(checkpoint.stamp.canonical_cloud_digest) ||
        id_is_zero(checkpoint.stamp.source_forest_digest) ||
        id_is_zero(checkpoint.stamp.scientific_digest) ||
        id_is_zero(checkpoint.stamp.committed_scientific_chain_digest) ||
        id_is_zero(checkpoint.stamp.at_least20_view_digest) ||
        checkpoint.stamp.active_at_least20_root_count !=
            checkpoint.active_at_least20_root_ids.size() ||
        checkpoint.active_at_least20_root_ids.size() >
            stream_budget.maximum_checkpoint_active_at_least20_root_count ||
        checkpoint.durable_file_codec_claimed ||
        checkpoint.public_status_claimed ||
        compute_checkpoint_digest(checkpoint) != checkpoint.state_digest) {
      output.receipt.decision = ExactDirectK1NormalizedProductRestoreDecision::
          no_checkpoint_shape_rejected;
      return output;
    }
    for (std::size_t index = 1U;
         index < checkpoint.active_at_least20_root_ids.size();
         ++index) {
      if (checkpoint.active_at_least20_root_ids[index] <=
          checkpoint.active_at_least20_root_ids[index - 1U]) {
        output.receipt.decision =
            ExactDirectK1NormalizedProductRestoreDecision::
                no_checkpoint_shape_rejected;
        return output;
      }
    }
    if (checkpoint.stamp.next_batch_cursor >
        stream_budget.maximum_checkpoint_replay_batch_count) {
      output.receipt.decision = ExactDirectK1NormalizedProductRestoreDecision::
          no_checkpoint_replay_budget_exhausted;
      return output;
    }

    auto fresh = initialize_exact_direct_k1_normalized_product_stream(
        index,
        cloud,
        observed_boruvka,
        k1_authority_budget,
        stream_budget);
    if (!fresh.certified_initialized_stream()) {
      output.receipt.decision = ExactDirectK1NormalizedProductRestoreDecision::
          no_fresh_initialization_rejected;
      return output;
    }
    output.receipt.boruvka_authority_freshly_replayed =
        fresh.boruvka_authority_freshly_replayed;
    output.receipt.compact_k1_forest_freshly_rebuilt =
        fresh.compact_k1_forest_certified;

    for (std::size_t replay_index = 0U;
         replay_index < checkpoint.stamp.next_batch_cursor;
         ++replay_index) {
      auto prepared = fresh.session.prepare_next();
      if (!prepared.certified_prepared_batch()) {
        output.receipt.decision =
            ExactDirectK1NormalizedProductRestoreDecision::
                no_fresh_prefix_replay_rejected;
        return output;
      }
      auto committed = fresh.session.commit(std::move(*prepared.ticket));
      if (!committed.certified_atomic_batch_commit()) {
        output.receipt.decision =
            ExactDirectK1NormalizedProductRestoreDecision::
                no_fresh_prefix_replay_rejected;
        return output;
      }
      ++output.receipt.replayed_product_batch_count;
    }
    output.receipt.product_prefix_freshly_replayed = true;
    output.receipt.at_least20_view_freshly_replayed = true;
    const auto fresh_checkpoint = fresh.session.export_checkpoint();
    if (!fresh_checkpoint.certified_checkpoint()) {
      output.receipt.decision = ExactDirectK1NormalizedProductRestoreDecision::
          no_fresh_checkpoint_export_rejected;
      return output;
    }
    if (!semantic_checkpoint_equal(
            checkpoint, fresh_checkpoint.checkpoint.value())) {
      output.receipt.decision = ExactDirectK1NormalizedProductRestoreDecision::
          no_semantic_checkpoint_mismatch;
      return output;
    }
    output.receipt.semantic_checkpoint_equality_certified = true;
    output.receipt.restored_stamp = fresh.session.current_stamp();
    output.receipt.session_instance_ids_distinct =
        checkpoint.stamp.session_instance_id !=
        output.receipt.restored_stamp.session_instance_id;
    if (!output.receipt.session_instance_ids_distinct) {
      output.receipt.decision = ExactDirectK1NormalizedProductRestoreDecision::
          no_session_instance_id_not_distinct;
      return output;
    }
    output.session = std::move(fresh.session);
    output.receipt.decision = ExactDirectK1NormalizedProductRestoreDecision::
        complete_certified_fresh_semantic_replay_restore;
    return output;
  } catch (const std::exception&) {
    output.receipt.decision = ExactDirectK1NormalizedProductRestoreDecision::
        no_checkpoint_shape_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
