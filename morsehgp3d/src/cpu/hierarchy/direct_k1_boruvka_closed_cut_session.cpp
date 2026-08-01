#include "morsehgp3d/hierarchy/direct_k1_boruvka_closed_cut_session.hpp"

#include "morsehgp3d/hierarchy/k1_forest.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <boost/multiprecision/integer.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view canonical_cloud_domain =
    "MorseHGP3D/phase9/pair-support/canonical-cloud/v1";
constexpr std::string_view source_forest_domain =
    "MorseHGP3D/phase15/k1-boruvka-closed-cut/source-forest/v1/sha256/";
constexpr std::string_view source_batch_domain =
    "MorseHGP3D/phase15/k1-boruvka-closed-cut/source-batch/v1/sha256/";
constexpr std::string_view initial_history_domain =
    "MorseHGP3D/phase15/k1-boruvka-closed-cut/history-initial/v1/sha256/";
constexpr std::string_view successor_history_domain =
    "MorseHGP3D/phase15/k1-boruvka-closed-cut/history-step/v1/sha256/";
constexpr std::string_view checkpoint_state_domain =
    "MorseHGP3D/phase15/k1-boruvka-closed-cut/checkpoint-state/v1/sha256/";

std::atomic<std::uint64_t> next_session_instance_id{1U};

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

void append_u32(
    contract::CanonicalSha256Builder& builder, std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder, std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(bytes);
}

void append_size(
    contract::CanonicalSha256Builder& builder, std::size_t value) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(value));
}

void append_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

void append_text(
    contract::CanonicalSha256Builder& builder, std::string_view text) {
  append_size(builder, text.size());
  builder.update(text);
}

void append_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& level) {
  const std::string key = level.canonical_key();
  append_text(builder, key);
}

[[nodiscard]] std::size_t integer_bit_count(const exact::BigInt& value) {
  if (value == 0) {
    return 1U;
  }
  if (value < 0) {
    throw std::logic_error(
        "a K1 exact-level integer must be nonnegative in this scanner");
  }
  return static_cast<std::size_t>(boost::multiprecision::msb(value)) + 1U;
}

[[nodiscard]] std::size_t exact_level_bit_count(
    const exact::ExactLevel& level) {
  return std::max(
      integer_bit_count(level.numerator()),
      integer_bit_count(level.denominator()));
}

[[nodiscard]] contract::CanonicalId canonical_cloud_digest(
    const spatial::CanonicalPointCloud& cloud) {
  contract::CanonicalSha256Builder builder;
  // Match the canonical pair-support DigestWriter domain encoding exactly:
  // a length-prefixed text value followed by the point-count and coordinate
  // words.  Raw domain bytes would mint a distinct namespace for the same
  // cloud and make a sealed K1 session impossible to bind to its resident
  // pair-source authority.
  append_text(builder, canonical_cloud_domain);
  append_size(builder, cloud.size());
  for (std::size_t point_index = 0U;
       point_index < cloud.size();
       ++point_index) {
    const auto point_id = static_cast<spatial::PointId>(point_index);
    for (const std::uint64_t word :
         cloud.point(point_id).canonical_input_bits()) {
      append_u64(builder, word);
    }
  }
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_source_forest_digest(
    const contract::CanonicalId& cloud_digest,
    const K1CompactForest& forest) {
  contract::CanonicalSha256Builder builder;
  builder.update(source_forest_domain);
  append_u32(
      builder, direct_k1_boruvka_closed_cut_session_schema_version);
  append_id(builder, cloud_digest);
  append_size(builder, forest.point_count);
  append_size(builder, forest.levels.size());
  for (const exact::ExactLevel& level : forest.levels) {
    append_level(builder, level);
  }
  append_size(builder, forest.selected_edges.size());
  for (const ExactEmstEdge& edge : forest.selected_edges) {
    append_u64(builder, edge.u);
    append_u64(builder, edge.v);
    append_level(builder, edge.squared_length);
    append_level(builder, edge.merge_level);
  }
  append_size(builder, forest.merge_nodes.size());
  for (const K1CompactMergeNode& node : forest.merge_nodes) {
    append_u64(builder, node.id);
    append_size(builder, node.level_index);
    append_size(builder, node.child_offset);
    append_size(builder, node.child_count);
    append_u64(builder, node.minimum_point_id);
    append_size(builder, node.coverage_size);
  }
  append_size(builder, forest.child_ids.size());
  for (const K1NodeId child_id : forest.child_ids) {
    append_u64(builder, child_id);
  }
  append_size(builder, forest.equal_level_batches.size());
  for (const K1CompactEqualLevelBatch& batch : forest.equal_level_batches) {
    append_size(builder, batch.level_index);
    append_size(builder, batch.selected_edge_offset);
    append_size(builder, batch.selected_edge_count);
    append_size(builder, batch.merge_node_offset);
    append_size(builder, batch.merge_node_count);
    append_size(builder, batch.pre_batch_component_count);
    append_size(builder, batch.post_batch_component_count);
  }
  append_u64(builder, forest.root_node_id);
  append_level(builder, forest.total_squared_weight);
  append_level(builder, forest.total_hgp_weight);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_batch_digest(
    const K1CompactForest& forest, std::size_t batch_index) {
  const K1CompactEqualLevelBatch& batch =
      forest.equal_level_batches.at(batch_index);
  contract::CanonicalSha256Builder builder;
  builder.update(source_batch_domain);
  append_size(builder, batch_index);
  append_level(builder, forest.levels.at(batch.level_index));
  append_size(builder, batch.selected_edge_count);
  for (std::size_t local_index = 0U;
       local_index < batch.selected_edge_count;
       ++local_index) {
    const ExactEmstEdge& edge =
        forest.selected_edges.at(batch.selected_edge_offset + local_index);
    append_u64(builder, edge.u);
    append_u64(builder, edge.v);
    append_level(builder, edge.squared_length);
    append_level(builder, edge.merge_level);
  }
  append_size(builder, batch.merge_node_count);
  for (std::size_t local_index = 0U;
       local_index < batch.merge_node_count;
       ++local_index) {
    const K1CompactMergeNode& node =
        forest.merge_nodes.at(batch.merge_node_offset + local_index);
    append_u64(builder, node.id);
    append_u64(builder, node.minimum_point_id);
    append_size(builder, node.child_count);
    for (std::size_t child_index = 0U;
         child_index < node.child_count;
         ++child_index) {
      append_u64(
          builder, forest.child_ids.at(node.child_offset + child_index));
    }
  }
  return builder.finalize();
}

[[nodiscard]] std::vector<contract::CanonicalId> build_history_prefixes(
    const K1CompactForest& forest,
    const contract::CanonicalId& source_forest_digest) {
  std::vector<contract::CanonicalId> prefixes;
  prefixes.reserve(forest.equal_level_batches.size() + 1U);
  contract::CanonicalSha256Builder initial_builder;
  initial_builder.update(initial_history_domain);
  append_id(initial_builder, source_forest_digest);
  prefixes.push_back(initial_builder.finalize());
  for (std::size_t batch_index = 0U;
       batch_index < forest.equal_level_batches.size();
       ++batch_index) {
    const contract::CanonicalId batch_digest =
        compute_batch_digest(forest, batch_index);
    contract::CanonicalSha256Builder successor_builder;
    successor_builder.update(successor_history_domain);
    append_id(successor_builder, prefixes.back());
    append_id(successor_builder, batch_digest);
    prefixes.push_back(successor_builder.finalize());
  }
  return prefixes;
}

[[nodiscard]] contract::CanonicalId compute_checkpoint_state_digest(
    const ExactDirectK1BoruvkaClosedCutCheckpoint& checkpoint) {
  contract::CanonicalSha256Builder builder;
  builder.update(checkpoint_state_domain);
  append_u32(builder, checkpoint.schema_version);
  append_u32(builder, checkpoint.stamp.schema_version);
  // session_instance_id is a live, process-local anti-forgery capability.
  // It is deliberately excluded from this scientific checkpoint identity so
  // a fresh replay in another process produces the same state digest.
  append_size(builder, checkpoint.stamp.committed_level_cursor);
  append_level(builder, checkpoint.stamp.closed_squared_level);
  append_id(builder, checkpoint.stamp.canonical_cloud_digest);
  append_id(builder, checkpoint.stamp.committed_history_digest);
  append_id(builder, checkpoint.source_forest_digest);
  append_size(
      builder, checkpoint.canonical_component_minimum_by_point.size());
  for (const spatial::PointId point_id :
       checkpoint.canonical_component_minimum_by_point) {
    append_u64(builder, point_id);
  }
  append_size(builder, checkpoint.closed_root_node_id_by_point.size());
  for (const K1NodeId node_id : checkpoint.closed_root_node_id_by_point) {
    append_u64(builder, node_id);
  }
  return builder.finalize();
}

[[nodiscard]] bool boruvka_verification_closes(
    const K1BoruvkaVerification& verification) noexcept {
  return verification.index_identity_certified &&
         verification.round_count_bound_certified &&
         verification.round_replay_certified &&
         verification.component_minima_certified &&
         verification.accepted_edges_certified &&
         verification.canonical_contractions_certified &&
         verification.spanning_tree_certified &&
         verification.exact_weights_certified &&
         verification.emst_witness_certified;
}

[[nodiscard]] bool budget_covers(
    const ExactDirectK1BoruvkaClosedCutSessionRequirements& requirements,
    const ExactDirectK1BoruvkaClosedCutSessionBudget& budget) noexcept {
  return requirements.point_count <= budget.maximum_point_count &&
         requirements.theoretical_boruvka_round_count <=
             budget.maximum_theoretical_boruvka_round_count &&
         requirements.source_edge_count <= budget.maximum_source_edge_count &&
         requirements.distinct_level_count_upper_bound <=
             budget.maximum_distinct_level_count &&
         requirements.merge_node_count_upper_bound <=
             budget.maximum_merge_node_count &&
         requirements.child_reference_count_upper_bound <=
             budget.maximum_child_reference_count &&
         requirements.parent_state_entry_count <=
             budget.maximum_parent_state_entry_count &&
         requirements.component_size_state_entry_count <=
             budget.maximum_component_size_state_entry_count &&
         requirements.component_minimum_state_entry_count <=
             budget.maximum_component_minimum_state_entry_count &&
         requirements.component_root_state_entry_count <=
             budget.maximum_component_root_state_entry_count &&
         requirements.history_digest_count_upper_bound <=
             budget.maximum_history_digest_count &&
         requirements.checkpoint_state_entry_count <=
             budget.maximum_checkpoint_state_entry_count &&
         requirements.maximum_observed_exact_level_integer_bit_count <=
             budget.maximum_single_exact_level_integer_bit_count;
}

[[nodiscard]] std::size_t observed_boruvka_maximum_level_bit_count(
    const K1ExactBoruvkaResult& observed) {
  std::size_t maximum = std::max(
      exact_level_bit_count(observed.total_squared_weight),
      exact_level_bit_count(observed.total_hgp_weight));
  for (const ExactEmstEdge& edge : observed.emst_edges) {
    maximum = std::max(maximum, exact_level_bit_count(edge.squared_length));
    maximum = std::max(maximum, exact_level_bit_count(edge.merge_level));
  }
  return maximum;
}

[[nodiscard]] std::size_t forest_maximum_level_bit_count(
    const K1CompactForest& forest) {
  std::size_t maximum = std::max(
      exact_level_bit_count(forest.total_squared_weight),
      exact_level_bit_count(forest.total_hgp_weight));
  for (const exact::ExactLevel& level : forest.levels) {
    maximum = std::max(maximum, exact_level_bit_count(level));
  }
  for (const ExactEmstEdge& edge : forest.selected_edges) {
    maximum = std::max(maximum, exact_level_bit_count(edge.squared_length));
    maximum = std::max(maximum, exact_level_bit_count(edge.merge_level));
  }
  return maximum;
}

}  // namespace

struct ExactDirectK1BoruvkaClosedCutSession::Impl {
  struct Seal {
    std::uint64_t session_instance_id{};
    contract::CanonicalId canonical_cloud_digest{};
    contract::CanonicalId source_forest_digest{};
  };

  ExactDirectK1BoruvkaClosedCutSessionBudget budget{};
  K1CompactForest forest{};
  std::vector<std::size_t> parents;
  std::vector<std::size_t> component_sizes;
  std::vector<spatial::PointId> component_minimums;
  std::vector<K1NodeId> component_root_node_ids;
  std::vector<contract::CanonicalId> history_prefixes;
  contract::CanonicalId canonical_cloud_digest{};
  contract::CanonicalId source_forest_digest{};
  exact::ExactLevel closed_squared_level{};
  std::size_t committed_level_cursor{};
  bool poisoned{false};
  std::unique_ptr<const Seal> seal;

  [[nodiscard]] std::size_t find_mutating(std::size_t point_index) noexcept {
    std::size_t root = point_index;
    while (parents[root] != root) {
      root = parents[root];
    }
    while (parents[point_index] != point_index) {
      const std::size_t parent = parents[point_index];
      parents[point_index] = root;
      point_index = parent;
    }
    return root;
  }

  [[nodiscard]] std::size_t find_const(
      std::size_t point_index, std::size_t* hop_count = nullptr) const
      noexcept {
    std::size_t hops = 0U;
    while (parents[point_index] != point_index) {
      point_index = parents[point_index];
      ++hops;
    }
    if (hop_count != nullptr) {
      *hop_count = hops;
    }
    return point_index;
  }

  [[nodiscard]] bool unite(
      std::size_t left, std::size_t right) noexcept {
    left = find_mutating(left);
    right = find_mutating(right);
    if (left == right) {
      return false;
    }
    if (component_sizes[left] < component_sizes[right] ||
        (component_sizes[left] == component_sizes[right] && right < left)) {
      std::swap(left, right);
    }
    parents[right] = left;
    component_sizes[left] += component_sizes[right];
    component_minimums[left] =
        std::min(component_minimums[left], component_minimums[right]);
    return true;
  }

  [[nodiscard]] bool structurally_ready() const noexcept {
    return !poisoned && seal != nullptr &&
           seal->session_instance_id != 0U &&
           seal->canonical_cloud_digest == canonical_cloud_digest &&
           seal->source_forest_digest == source_forest_digest &&
           forest.point_count != 0U && parents.size() == forest.point_count &&
           component_sizes.size() == forest.point_count &&
           component_minimums.size() == forest.point_count &&
           component_root_node_ids.size() == forest.point_count &&
           history_prefixes.size() == forest.levels.size() + 1U &&
           committed_level_cursor < history_prefixes.size();
  }

  [[nodiscard]] ExactDirectK1BoruvkaClosedCutSessionStamp stamp() const {
    if (!structurally_ready()) {
      throw std::logic_error("the sealed K1 closed-cut session is not ready");
    }
    return ExactDirectK1BoruvkaClosedCutSessionStamp{
        direct_k1_boruvka_closed_cut_session_schema_version,
        seal->session_instance_id,
        committed_level_cursor,
        closed_squared_level,
        canonical_cloud_digest,
        history_prefixes[committed_level_cursor]};
  }

  [[nodiscard]] bool current_stamp_equal(
      const ExactDirectK1BoruvkaClosedCutSessionStamp& stamp) const noexcept {
    return structurally_ready() &&
           stamp.schema_version ==
               direct_k1_boruvka_closed_cut_session_schema_version &&
           stamp.session_instance_id == seal->session_instance_id &&
           stamp.committed_level_cursor == committed_level_cursor &&
           stamp.closed_squared_level == closed_squared_level &&
           stamp.canonical_cloud_digest == canonical_cloud_digest &&
           stamp.committed_history_digest ==
               history_prefixes[committed_level_cursor];
  }

  [[nodiscard]] bool replay_stamp_valid(
      const ExactDirectK1BoruvkaClosedCutSessionStamp& stamp) const {
    if (!structurally_ready() ||
        stamp.schema_version !=
            direct_k1_boruvka_closed_cut_session_schema_version ||
        stamp.session_instance_id != seal->session_instance_id ||
        stamp.canonical_cloud_digest != canonical_cloud_digest ||
        stamp.committed_level_cursor >= history_prefixes.size() ||
        stamp.committed_history_digest !=
            history_prefixes[stamp.committed_level_cursor]) {
      return false;
    }
    std::size_t expected_cursor = 0U;
    while (expected_cursor < forest.levels.size() &&
           forest.levels[expected_cursor] <= stamp.closed_squared_level) {
      ++expected_cursor;
    }
    return expected_cursor == stamp.committed_level_cursor;
  }
};

ExactDirectK1BoruvkaClosedCutSession::
    ExactDirectK1BoruvkaClosedCutSession() noexcept = default;

ExactDirectK1BoruvkaClosedCutSession::
    ~ExactDirectK1BoruvkaClosedCutSession() = default;

ExactDirectK1BoruvkaClosedCutSession::
    ExactDirectK1BoruvkaClosedCutSession(
        ExactDirectK1BoruvkaClosedCutSession&&) noexcept = default;

ExactDirectK1BoruvkaClosedCutSession&
ExactDirectK1BoruvkaClosedCutSession::operator=(
    ExactDirectK1BoruvkaClosedCutSession&&) noexcept = default;

ExactDirectK1BoruvkaClosedCutSession::
    ExactDirectK1BoruvkaClosedCutSession(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectK1BoruvkaClosedCutSession::ready() const noexcept {
  return impl_ != nullptr && impl_->structurally_ready();
}

bool ExactDirectK1BoruvkaClosedCutSession::poisoned() const noexcept {
  return impl_ != nullptr && impl_->poisoned;
}

std::size_t ExactDirectK1BoruvkaClosedCutSession::point_count() const
    noexcept {
  return impl_ == nullptr ? 0U : impl_->forest.point_count;
}

ExactDirectK1BoruvkaClosedCutSessionStamp
ExactDirectK1BoruvkaClosedCutSession::current_stamp() const {
  if (impl_ == nullptr) {
    throw std::logic_error("the sealed K1 closed-cut session was moved from");
  }
  return impl_->stamp();
}

ExactDirectK1BoruvkaClosedCutAdvanceReceipt
ExactDirectK1BoruvkaClosedCutSession::advance_closed_to(
    const exact::ExactLevel& closed_squared_level) & {
  ExactDirectK1BoruvkaClosedCutAdvanceReceipt receipt;
  if (!ready()) {
    receipt.decision =
        ExactDirectK1BoruvkaClosedCutAdvanceDecision::no_session_not_ready;
    return receipt;
  }

  try {
    receipt.pre_stamp = impl_->stamp();
    receipt.post_stamp = receipt.pre_stamp;
  } catch (const std::bad_alloc&) {
    receipt.decision =
        ExactDirectK1BoruvkaClosedCutAdvanceDecision::
            no_exact_comparison_failed;
    return receipt;
  }

  if (exact_level_bit_count(closed_squared_level) >
      impl_->budget.maximum_single_exact_level_integer_bit_count) {
    receipt.decision =
        ExactDirectK1BoruvkaClosedCutAdvanceDecision::
            no_exact_level_bit_budget_exhausted;
    return receipt;
  }

  std::size_t target_cursor = impl_->committed_level_cursor;
  try {
    if (closed_squared_level < impl_->closed_squared_level) {
      receipt.decision =
          ExactDirectK1BoruvkaClosedCutAdvanceDecision::
              no_nonmonotone_closed_cut;
      return receipt;
    }
    while (target_cursor < impl_->forest.levels.size() &&
           impl_->forest.levels[target_cursor] <= closed_squared_level) {
      ++target_cursor;
    }

    exact::ExactLevel prepared_internal_level = closed_squared_level;
    receipt.post_stamp.closed_squared_level = closed_squared_level;
    receipt.post_stamp.committed_level_cursor = target_cursor;
    receipt.post_stamp.committed_history_digest =
        impl_->history_prefixes[target_cursor];

    for (std::size_t batch_index = impl_->committed_level_cursor;
         batch_index < target_cursor;
         ++batch_index) {
      const K1CompactEqualLevelBatch& batch =
          impl_->forest.equal_level_batches[batch_index];
      receipt.consumed_edge_count += batch.selected_edge_count;
      receipt.consumed_merge_node_count += batch.merge_node_count;
    }
    receipt.consumed_intermediate_level_count =
        target_cursor - impl_->committed_level_cursor;
    receipt.state_mutated =
        target_cursor != impl_->committed_level_cursor ||
        closed_squared_level != impl_->closed_squared_level;

    for (std::size_t batch_index = impl_->committed_level_cursor;
         batch_index < target_cursor;
         ++batch_index) {
      const K1CompactEqualLevelBatch& batch =
          impl_->forest.equal_level_batches[batch_index];
      for (std::size_t local_index = 0U;
           local_index < batch.selected_edge_count;
           ++local_index) {
        const ExactEmstEdge& edge = impl_->forest.selected_edges[
            batch.selected_edge_offset + local_index];
        if (!impl_->unite(
                static_cast<std::size_t>(edge.u),
                static_cast<std::size_t>(edge.v))) {
          impl_->poisoned = true;
          receipt.post_stamp = {};
          receipt.state_mutated = true;
          receipt.decision =
              ExactDirectK1BoruvkaClosedCutAdvanceDecision::
                  no_internal_source_contradiction;
          return receipt;
        }
      }
      for (std::size_t local_index = 0U;
           local_index < batch.merge_node_count;
           ++local_index) {
        const K1CompactMergeNode& node = impl_->forest.merge_nodes[
            batch.merge_node_offset + local_index];
        const std::size_t root = impl_->find_mutating(
            static_cast<std::size_t>(node.minimum_point_id));
        impl_->component_root_node_ids[root] = node.id;
      }
    }

    impl_->closed_squared_level = std::move(prepared_internal_level);
    impl_->committed_level_cursor = target_cursor;
    receipt.decision =
        ExactDirectK1BoruvkaClosedCutAdvanceDecision::
            complete_certified_monotone_closed_cut;
    return receipt;
  } catch (const std::bad_alloc&) {
    receipt.post_stamp = receipt.pre_stamp;
    receipt.consumed_intermediate_level_count = 0U;
    receipt.consumed_edge_count = 0U;
    receipt.consumed_merge_node_count = 0U;
    receipt.state_mutated = false;
    receipt.decision =
        ExactDirectK1BoruvkaClosedCutAdvanceDecision::
            no_exact_comparison_failed;
    return receipt;
  }
}

ExactDirectK1BoruvkaClosedCutRootQueryResult
ExactDirectK1BoruvkaClosedCutSession::query_singleton_root(
    spatial::PointId singleton_point_id,
    const ExactDirectK1BoruvkaClosedCutSessionStamp& stamp) const noexcept {
  ExactDirectK1BoruvkaClosedCutRootQueryResult result;
  if (!ready()) {
    result.decision =
        ExactDirectK1BoruvkaClosedCutRootQueryDecision::no_session_not_ready;
    return result;
  }
  try {
    result.stamp = stamp;
  } catch (...) {
    result.decision =
        ExactDirectK1BoruvkaClosedCutRootQueryDecision::
            no_stale_or_foreign_stamp;
    return result;
  }
  result.singleton_point_id = singleton_point_id;
  if (!impl_->current_stamp_equal(stamp)) {
    result.decision =
        ExactDirectK1BoruvkaClosedCutRootQueryDecision::
            no_stale_or_foreign_stamp;
    return result;
  }
  if (singleton_point_id >= impl_->forest.point_count) {
    result.decision =
        ExactDirectK1BoruvkaClosedCutRootQueryDecision::
            no_singleton_out_of_range;
    return result;
  }
  const std::size_t root = impl_->find_const(
      static_cast<std::size_t>(singleton_point_id),
      &result.parent_hop_count);
  result.closed_root_node_id = impl_->component_root_node_ids[root];
  result.decision =
      ExactDirectK1BoruvkaClosedCutRootQueryDecision::
          complete_certified_singleton_closed_root;
  return result;
}

bool ExactDirectK1BoruvkaClosedCutSession::verify_stamp(
    const ExactDirectK1BoruvkaClosedCutSessionStamp& stamp) const noexcept {
  return impl_ != nullptr && impl_->current_stamp_equal(stamp);
}

bool ExactDirectK1BoruvkaClosedCutSession::verify_advance_receipt(
    const ExactDirectK1BoruvkaClosedCutAdvanceReceipt& receipt) const {
  if (!ready() ||
      receipt.schema_version !=
          direct_k1_boruvka_closed_cut_session_schema_version ||
      receipt.gamma_cells_or_global_cofaces_materialized ||
      receipt.ordinary_or_higher_order_delaunay_materialized ||
      receipt.public_status_claimed) {
    return false;
  }
  if (receipt.decision ==
      ExactDirectK1BoruvkaClosedCutAdvanceDecision::
          complete_certified_monotone_closed_cut) {
    if (!impl_->replay_stamp_valid(receipt.pre_stamp) ||
        !impl_->replay_stamp_valid(receipt.post_stamp) ||
        receipt.post_stamp.closed_squared_level <
            receipt.pre_stamp.closed_squared_level ||
        receipt.post_stamp.committed_level_cursor <
            receipt.pre_stamp.committed_level_cursor) {
      return false;
    }
    std::size_t expected_edge_count = 0U;
    std::size_t expected_merge_node_count = 0U;
    for (std::size_t batch_index =
             receipt.pre_stamp.committed_level_cursor;
         batch_index < receipt.post_stamp.committed_level_cursor;
         ++batch_index) {
      const K1CompactEqualLevelBatch& batch =
          impl_->forest.equal_level_batches[batch_index];
      expected_edge_count += batch.selected_edge_count;
      expected_merge_node_count += batch.merge_node_count;
    }
    const std::size_t expected_level_count =
        receipt.post_stamp.committed_level_cursor -
        receipt.pre_stamp.committed_level_cursor;
    const bool expected_mutation =
        expected_level_count != 0U ||
        receipt.pre_stamp.closed_squared_level !=
            receipt.post_stamp.closed_squared_level;
    return receipt.consumed_intermediate_level_count ==
               expected_level_count &&
           receipt.consumed_edge_count == expected_edge_count &&
           receipt.consumed_merge_node_count ==
               expected_merge_node_count &&
           receipt.state_mutated == expected_mutation;
  }
  const bool replayable_nonmutation =
      receipt.decision ==
          ExactDirectK1BoruvkaClosedCutAdvanceDecision::
              no_nonmonotone_closed_cut ||
      receipt.decision ==
          ExactDirectK1BoruvkaClosedCutAdvanceDecision::
              no_exact_level_bit_budget_exhausted ||
      receipt.decision ==
          ExactDirectK1BoruvkaClosedCutAdvanceDecision::
              no_exact_comparison_failed;
  return replayable_nonmutation &&
         receipt.pre_stamp == receipt.post_stamp &&
         impl_->current_stamp_equal(receipt.pre_stamp) &&
         receipt.consumed_intermediate_level_count == 0U &&
         receipt.consumed_edge_count == 0U &&
         receipt.consumed_merge_node_count == 0U &&
         !receipt.state_mutated;
}

bool ExactDirectK1BoruvkaClosedCutSession::verify_root_query_result(
    const ExactDirectK1BoruvkaClosedCutRootQueryResult& result) const
    noexcept {
  if (!ready() ||
      result.schema_version !=
          direct_k1_boruvka_closed_cut_session_schema_version ||
      result.decision !=
          ExactDirectK1BoruvkaClosedCutRootQueryDecision::
              complete_certified_singleton_closed_root ||
      !result.closed_root_node_id.has_value() ||
      !impl_->current_stamp_equal(result.stamp) ||
      result.singleton_point_id >= impl_->forest.point_count) {
    return false;
  }
  std::size_t expected_hops = 0U;
  const std::size_t root = impl_->find_const(
      static_cast<std::size_t>(result.singleton_point_id),
      &expected_hops);
  return *result.closed_root_node_id ==
             impl_->component_root_node_ids[root] &&
         result.parent_hop_count == expected_hops;
}

ExactDirectK1BoruvkaClosedCutCheckpointExportResult
ExactDirectK1BoruvkaClosedCutSession::export_checkpoint() const {
  ExactDirectK1BoruvkaClosedCutCheckpointExportResult output;
  if (!ready()) {
    output.decision =
        ExactDirectK1BoruvkaClosedCutCheckpointDecision::
            no_session_not_ready;
    return output;
  }
  std::size_t required_checkpoint_entries = 0U;
  if (!checked_multiply(
          2U, impl_->forest.point_count, required_checkpoint_entries) ||
      required_checkpoint_entries >
          impl_->budget.maximum_checkpoint_state_entry_count) {
    output.decision =
        ExactDirectK1BoruvkaClosedCutCheckpointDecision::
            no_checkpoint_budget_exhausted;
    return output;
  }
  try {
    output.checkpoint.stamp = impl_->stamp();
    output.checkpoint.source_forest_digest = impl_->source_forest_digest;
    output.checkpoint.canonical_component_minimum_by_point.reserve(
        impl_->forest.point_count);
    output.checkpoint.closed_root_node_id_by_point.reserve(
        impl_->forest.point_count);
    for (std::size_t point_index = 0U;
         point_index < impl_->forest.point_count;
         ++point_index) {
      const std::size_t root = impl_->find_const(point_index);
      output.checkpoint.canonical_component_minimum_by_point.push_back(
          impl_->component_minimums[root]);
      output.checkpoint.closed_root_node_id_by_point.push_back(
          impl_->component_root_node_ids[root]);
    }
    output.checkpoint.state_digest =
        compute_checkpoint_state_digest(output.checkpoint);
    output.decision =
        ExactDirectK1BoruvkaClosedCutCheckpointDecision::
            complete_checkpoint_ready_state;
    return output;
  } catch (const std::bad_alloc&) {
    output.checkpoint = {};
    output.decision =
        ExactDirectK1BoruvkaClosedCutCheckpointDecision::no_allocation_failed;
    return output;
  }
}

bool ExactDirectK1BoruvkaClosedCutSession::verify_checkpoint(
    const ExactDirectK1BoruvkaClosedCutCheckpoint& checkpoint) const {
  if (!ready() ||
      checkpoint.schema_version !=
          direct_k1_boruvka_closed_cut_session_schema_version ||
      !impl_->current_stamp_equal(checkpoint.stamp) ||
      checkpoint.source_forest_digest != impl_->source_forest_digest ||
      checkpoint.canonical_component_minimum_by_point.size() !=
          impl_->forest.point_count ||
      checkpoint.closed_root_node_id_by_point.size() !=
          impl_->forest.point_count ||
      compute_checkpoint_state_digest(checkpoint) !=
          checkpoint.state_digest) {
    return false;
  }
  for (std::size_t point_index = 0U;
       point_index < impl_->forest.point_count;
       ++point_index) {
    const std::size_t root = impl_->find_const(point_index);
    if (checkpoint.canonical_component_minimum_by_point[point_index] !=
            impl_->component_minimums[root] ||
        checkpoint.closed_root_node_id_by_point[point_index] !=
            impl_->component_root_node_ids[root]) {
      return false;
    }
  }
  return true;
}

bool ExactDirectK1BoruvkaClosedCutSessionInitialization::
    certified_ready_session() const noexcept {
  return schema_version ==
             direct_k1_boruvka_closed_cut_session_schema_version &&
         allocation_free_structural_preflight_certified &&
         boruvka_authority_freshly_replayed &&
         compact_k1_equal_level_forest_certified &&
         checkpoint_semantic_state_ready &&
         !gamma_cells_or_global_cofaces_materialized &&
         !ordinary_or_higher_order_delaunay_materialized &&
         !public_status_claimed &&
         decision ==
             ExactDirectK1BoruvkaClosedCutInitializationDecision::
                 complete_certified_sealed_session &&
         session.ready();
}

ExactDirectK1BoruvkaClosedCutSessionInitialization
build_exact_direct_k1_boruvka_closed_cut_session(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const K1ExactBoruvkaResult& observed_boruvka,
    const ExactDirectK1BoruvkaClosedCutSessionBudget& budget) {
  ExactDirectK1BoruvkaClosedCutSessionInitialization output;
  output.requested_budget = budget;

  const std::size_t point_count = cloud.size();
  if (point_count == 0U) {
    output.decision =
        ExactDirectK1BoruvkaClosedCutInitializationDecision::
            no_boruvka_authority_rejected;
    return output;
  }
  auto& requirements = output.requirements;
  requirements.point_count = point_count;
  requirements.theoretical_boruvka_round_count =
      point_count <= 1U
          ? 0U
          : static_cast<std::size_t>(std::bit_width(point_count - 1U));
  requirements.source_edge_count = point_count - 1U;
  requirements.distinct_level_count_upper_bound = point_count - 1U;
  requirements.merge_node_count_upper_bound = point_count - 1U;
  requirements.parent_state_entry_count = point_count;
  requirements.component_size_state_entry_count = point_count;
  requirements.component_minimum_state_entry_count = point_count;
  requirements.component_root_state_entry_count = point_count;
  if (!checked_multiply(
          2U,
          point_count - 1U,
          requirements.child_reference_count_upper_bound) ||
      !checked_add(
          requirements.distinct_level_count_upper_bound,
          1U,
          requirements.history_digest_count_upper_bound) ||
      !checked_multiply(
          2U,
          point_count,
          requirements.checkpoint_state_entry_count)) {
    output.decision =
        ExactDirectK1BoruvkaClosedCutInitializationDecision::
            no_preflight_capacity_overflow;
    return output;
  }
  if (observed_boruvka.point_count != point_count ||
      observed_boruvka.emst_edges.size() !=
          requirements.source_edge_count ||
      observed_boruvka.rounds.size() >
          requirements.theoretical_boruvka_round_count) {
    output.decision =
        ExactDirectK1BoruvkaClosedCutInitializationDecision::
            no_boruvka_authority_rejected;
    return output;
  }
  requirements.maximum_observed_exact_level_integer_bit_count =
      observed_boruvka_maximum_level_bit_count(observed_boruvka);
  if (!budget_covers(requirements, budget)) {
    output.decision =
        ExactDirectK1BoruvkaClosedCutInitializationDecision::
            no_preflight_budget_exhausted;
    return output;
  }
  output.allocation_free_structural_preflight_certified = true;

  try {
    output.canonical_cloud_digest = canonical_cloud_digest(cloud);
    const K1BoruvkaVerification verification =
        verify_exact_lbvh_boruvka(index, cloud, observed_boruvka);
    output.replayed_boruvka_round_count =
        verification.replayed_round_count;
    output.replayed_boruvka_component_minimum_count =
        verification.replayed_component_minimum_count;
    if (!boruvka_verification_closes(verification)) {
      output.decision =
          ExactDirectK1BoruvkaClosedCutInitializationDecision::
              no_boruvka_authority_rejected;
      return output;
    }
    output.boruvka_authority_freshly_replayed = true;

    K1CompactForest forest = build_compact_k1_forest(
        point_count, observed_boruvka.emst_edges);
    requirements.maximum_observed_exact_level_integer_bit_count =
        forest_maximum_level_bit_count(forest);
    if (requirements.maximum_observed_exact_level_integer_bit_count >
            budget.maximum_single_exact_level_integer_bit_count ||
        forest.levels.size() > budget.maximum_distinct_level_count ||
        forest.merge_nodes.size() > budget.maximum_merge_node_count ||
        forest.child_ids.size() > budget.maximum_child_reference_count ||
        forest.equal_level_batches.size() + 1U >
            budget.maximum_history_digest_count) {
      output.decision =
          ExactDirectK1BoruvkaClosedCutInitializationDecision::
              no_preflight_budget_exhausted;
      return output;
    }
    output.compact_k1_equal_level_forest_certified = true;
    output.source_forest_digest = compute_source_forest_digest(
        output.canonical_cloud_digest, forest);
    std::vector<contract::CanonicalId> history_prefixes =
        build_history_prefixes(forest, output.source_forest_digest);

    auto impl = std::make_unique<ExactDirectK1BoruvkaClosedCutSession::Impl>();
    impl->budget = budget;
    impl->forest = std::move(forest);
    impl->parents.resize(point_count);
    std::iota(impl->parents.begin(), impl->parents.end(), std::size_t{0U});
    impl->component_sizes.assign(point_count, 1U);
    impl->component_minimums.reserve(point_count);
    impl->component_root_node_ids.reserve(point_count);
    for (std::size_t point_index = 0U;
         point_index < point_count;
         ++point_index) {
      const auto point_id = static_cast<spatial::PointId>(point_index);
      impl->component_minimums.push_back(point_id);
      impl->component_root_node_ids.push_back(
          static_cast<K1NodeId>(point_index));
    }
    impl->history_prefixes = std::move(history_prefixes);
    impl->canonical_cloud_digest = output.canonical_cloud_digest;
    impl->source_forest_digest = output.source_forest_digest;
    const std::uint64_t session_instance_id =
        allocate_session_instance_id();
    if (session_instance_id == 0U) {
      output.decision =
          ExactDirectK1BoruvkaClosedCutInitializationDecision::
              no_session_instance_id_exhausted;
      return output;
    }
    impl->seal =
        std::make_unique<const ExactDirectK1BoruvkaClosedCutSession::Impl::Seal>(
            ExactDirectK1BoruvkaClosedCutSession::Impl::Seal{
                session_instance_id,
                output.canonical_cloud_digest,
                output.source_forest_digest});
    if (!impl->structurally_ready()) {
      output.decision =
          ExactDirectK1BoruvkaClosedCutInitializationDecision::
              no_compact_k1_forest_rejected;
      return output;
    }
    output.checkpoint_semantic_state_ready = true;
    output.session = ExactDirectK1BoruvkaClosedCutSession{std::move(impl)};
    output.decision =
        ExactDirectK1BoruvkaClosedCutInitializationDecision::
            complete_certified_sealed_session;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectK1BoruvkaClosedCutInitializationDecision::
            no_allocation_failed;
    return output;
  } catch (const std::exception&) {
    output.decision =
        output.boruvka_authority_freshly_replayed
            ? ExactDirectK1BoruvkaClosedCutInitializationDecision::
                  no_compact_k1_forest_rejected
            : ExactDirectK1BoruvkaClosedCutInitializationDecision::
                  no_boruvka_authority_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
