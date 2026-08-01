#include "morsehgp3d/hierarchy/direct_at_least20_stream_view.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
static_assert(
    std::is_nothrow_move_constructible_v<exact::ExactLevel> &&
    std::is_nothrow_move_assignable_v<exact::ExactLevel>);

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] bool checked_sub_add(
    std::size_t value,
    std::size_t subtract,
    std::size_t add,
    std::size_t& result) noexcept {
  if (subtract > value) {
    return false;
  }
  return checked_add(value - subtract, add, result);
}

void hash_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    const std::size_t shift = (bytes.size() - 1U - index) * 8U;
    bytes[index] = static_cast<std::uint8_t>(value >> shift);
  }
  builder.update(bytes);
}

void hash_size(
    contract::CanonicalSha256Builder& builder,
    std::size_t value) {
  hash_u64(builder, static_cast<std::uint64_t>(value));
}

void hash_bool(contract::CanonicalSha256Builder& builder, bool value) {
  const std::array<std::uint8_t, 1U> byte{
      static_cast<std::uint8_t>(value ? 1U : 0U)};
  builder.update(byte);
}

template <class Enum>
void hash_enum(contract::CanonicalSha256Builder& builder, Enum value) {
  using Underlying = std::underlying_type_t<Enum>;
  hash_u64(
      builder,
      static_cast<std::uint64_t>(static_cast<Underlying>(value)));
}

void hash_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

[[nodiscard]] bool id_is_zero(
    const contract::CanonicalId& value) noexcept {
  return std::all_of(
      value.bytes().begin(),
      value.bytes().end(),
      [](std::uint8_t byte) { return byte == 0U; });
}

void hash_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view value) {
  hash_size(builder, value.size());
  builder.update(value);
}

void hash_level(
    contract::CanonicalSha256Builder& builder,
    const exact::ExactLevel& level) {
  hash_text(builder, level.numerator_string());
  hash_text(builder, level.denominator_string());
}

[[nodiscard]] bool action_shape_is_valid(
    const ExactDirectAtLeast20CommittedGroupInput& group) noexcept {
  if (group.q_r != group.prior_root_ids.size()) {
    return false;
  }
  switch (group.source_action) {
    case ExactDirectAtLeast20SourceAction::reduced_birth:
      return group.q_r == 0U &&
          !group.coverage_delta_point_ids.empty();
    case ExactDirectAtLeast20SourceAction::continuation:
      return group.q_r == 1U;
    case ExactDirectAtLeast20SourceAction::multifusion:
      return group.q_r >= 2U;
  }
  return false;
}

[[nodiscard]] bool budget_shape_is_valid(
    const ExactDirectAtLeast20StreamViewBudget& budget) noexcept {
  return budget.maximum_batch_group_count != 0U &&
      budget.maximum_batch_prior_root_reference_count != 0U &&
      budget.maximum_batch_coverage_delta_point_reference_count != 0U &&
      budget.maximum_batch_visible_transition_count != 0U &&
      budget.maximum_batch_visible_child_reference_count != 0U &&
      budget.maximum_root_state_count != 0U &&
      budget.maximum_active_root_count != 0U &&
      budget.maximum_active_root_count <= budget.maximum_root_state_count &&
      budget.maximum_retained_point_reference_count >=
          direct_at_least20_stream_view_threshold &&
      budget.maximum_staging_entry_count != 0U &&
      budget.maximum_checkpoint_root_state_count >=
          budget.maximum_root_state_count &&
      budget.maximum_checkpoint_retained_point_reference_count >=
          budget.maximum_retained_point_reference_count;
}

[[nodiscard]] contract::CanonicalId committed_batch_digest(
    const ExactDirectAtLeast20CommittedBatchInput& input) {
  contract::CanonicalSha256Builder builder;
  builder.update(
      "MorseHGP3D/direct-at-least20/committed-source-batch/v1/");
  hash_id(builder, input.previous_source_commit_digest);
  hash_id(builder, input.source_scientific_digest);
  hash_size(builder, input.source_batch_cursor_before);
  hash_size(builder, input.source_batch_cursor_after);
  hash_size(builder, input.source_epoch_before);
  hash_size(builder, input.source_epoch_after);
  hash_size(builder, input.order);
  hash_level(builder, input.squared_level);
  hash_size(builder, input.groups.size());
  for (const ExactDirectAtLeast20CommittedGroupInput& group : input.groups) {
    hash_size(builder, group.source_group_index);
    hash_enum(builder, group.source_action);
    hash_size(builder, group.q_r);
    hash_u64(builder, group.resultant_root_id);
    hash_size(builder, group.prior_root_ids.size());
    for (const ExactDirectAtLeast20RootId root_id : group.prior_root_ids) {
      hash_u64(builder, root_id);
    }
    hash_size(builder, group.coverage_delta_point_ids.size());
    for (const spatial::PointId point_id :
         group.coverage_delta_point_ids) {
      hash_u64(builder, point_id);
    }
  }
  hash_bool(builder, input.complete_equal_level_batch_committed);
  hash_bool(
      builder,
      input.source_actions_qr_root_successors_and_coverage_delta_certified);
  hash_bool(builder, input.source_pruned_by_min_cluster_size);
  hash_bool(builder, input.source_public_status_claimed);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId initial_view_digest(
    std::size_t next_source_batch_cursor,
    std::size_t source_epoch,
    const contract::CanonicalId& source_scientific_digest,
    const contract::CanonicalId& source_commit_digest) {
  contract::CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/direct-at-least20/view-chain-anchor/v1/");
  hash_size(builder, next_source_batch_cursor);
  hash_size(builder, source_epoch);
  hash_id(builder, source_scientific_digest);
  hash_id(builder, source_commit_digest);
  hash_size(builder, direct_at_least20_stream_view_threshold);
  return builder.finalize();
}

struct StackCappedCoverage {
  std::array<spatial::PointId, direct_at_least20_stream_view_threshold>
      point_ids{};
  std::size_t size{};

  void insert(spatial::PointId point_id) noexcept {
    const auto begin = point_ids.begin();
    const auto end = begin + static_cast<std::ptrdiff_t>(size);
    const auto position = std::lower_bound(begin, end, point_id);
    if (position != end && *position == point_id) {
      return;
    }
    if (size < point_ids.size()) {
      std::move_backward(position, end, end + 1);
      *position = point_id;
      ++size;
      return;
    }
    if (position == end) {
      return;
    }
    std::move_backward(position, end - 1, end);
    *position = point_id;
  }

  void merge(std::span<const spatial::PointId> other) noexcept {
    for (const spatial::PointId point_id : other) {
      insert(point_id);
    }
  }

  [[nodiscard]] bool visible() const noexcept {
    return size == point_ids.size();
  }
};

struct RootKey {
  std::size_t order{};
  ExactDirectAtLeast20RootId root_id{};

  friend bool operator==(const RootKey&, const RootKey&) = default;
  friend bool operator<(const RootKey& left, const RootKey& right) noexcept {
    return left.order < right.order ||
        (left.order == right.order && left.root_id < right.root_id);
  }
};

struct RootState {
  RootState(
      RootKey key_value,
      CappedDistinctPointCoverage coverage_value,
      bool active_value) noexcept
      : key(key_value),
        coverage(std::move(coverage_value)),
        active(active_value) {}

  RootState(RootState&&) noexcept = default;
  RootState& operator=(RootState&&) noexcept = default;
  RootState(const RootState&) = delete;
  RootState& operator=(const RootState&) = delete;

  RootKey key{};
  CappedDistinctPointCoverage coverage{
      direct_at_least20_stream_view_threshold};
  bool active{false};
};

struct RootLookupEntry {
  RootKey key{};
  std::size_t root_state_index{};
};

struct OrderCursorState {
  std::size_t order{};
  exact::ExactLevel last_squared_level{};
};

struct StagedGroup {
  StagedGroup(
      const ExactDirectAtLeast20CommittedGroupInput* source_value,
      CappedDistinctPointCoverage coverage_value)
      : source(source_value), coverage(std::move(coverage_value)) {}

  StagedGroup(StagedGroup&&) noexcept = default;
  StagedGroup& operator=(StagedGroup&&) noexcept = default;
  StagedGroup(const StagedGroup&) = delete;
  StagedGroup& operator=(const StagedGroup&) = delete;

  const ExactDirectAtLeast20CommittedGroupInput* source{};
  std::vector<std::size_t> child_root_state_indices;
  CappedDistinctPointCoverage coverage{
      direct_at_least20_stream_view_threshold};
  bool reuses_continuation_root_state{false};
  std::size_t continuation_root_state_index{};
  std::vector<ExactDirectAtLeast20RootId> prior_visible_root_ids;
};

[[nodiscard]] std::vector<RootLookupEntry>::iterator find_root_lookup(
    std::vector<RootLookupEntry>& lookup,
    RootKey key) noexcept {
  const auto position = std::lower_bound(
      lookup.begin(),
      lookup.end(),
      key,
      [](const RootLookupEntry& entry, const RootKey& candidate) {
        return entry.key < candidate;
      });
  if (position == lookup.end() || !(position->key == key)) {
    return lookup.end();
  }
  return position;
}

[[nodiscard]] std::vector<OrderCursorState>::iterator find_order_cursor(
    std::vector<OrderCursorState>& cursors,
    std::size_t order) noexcept {
  const auto position = std::lower_bound(
      cursors.begin(),
      cursors.end(),
      order,
      [](const OrderCursorState& cursor, std::size_t candidate) {
        return cursor.order < candidate;
      });
  if (position == cursors.end() || position->order != order) {
    return cursors.end();
  }
  return position;
}

[[nodiscard]] ExactDirectAtLeast20VisibleTransitionKind visible_kind(
    std::size_t q_visible) noexcept {
  if (q_visible == 0U) {
    return ExactDirectAtLeast20VisibleTransitionKind::threshold_entry;
  }
  if (q_visible == 1U) {
    return ExactDirectAtLeast20VisibleTransitionKind::continuation;
  }
  return ExactDirectAtLeast20VisibleTransitionKind::multifusion;
}

[[nodiscard]] bool batch_key_less(
    const exact::ExactLevel& left_level,
    std::size_t left_order,
    const exact::ExactLevel& right_level,
    std::size_t right_order) {
  if (left_level < right_level) {
    return true;
  }
  if (right_level < left_level) {
    return false;
  }
  return left_order < right_order;
}

[[nodiscard]] contract::CanonicalId next_view_digest(
    const contract::CanonicalId& previous_view_digest,
    const contract::CanonicalId& source_commit_digest,
    std::size_t order,
    const exact::ExactLevel& squared_level,
    std::span<const StagedGroup> staged_groups,
    std::span<const ExactDirectAtLeast20VisibleTransition> transitions) {
  contract::CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/direct-at-least20/view-batch/v1/");
  hash_id(builder, previous_view_digest);
  hash_id(builder, source_commit_digest);
  hash_size(builder, order);
  hash_level(builder, squared_level);
  hash_size(builder, staged_groups.size());
  for (const StagedGroup& staged : staged_groups) {
    hash_size(builder, staged.source->source_group_index);
    hash_u64(builder, staged.source->resultant_root_id);
    hash_size(builder, staged.coverage.retained_distinct_count());
    for (const spatial::PointId point_id :
         staged.coverage.smallest_distinct_point_ids()) {
      hash_u64(builder, point_id);
    }
    hash_bool(builder, staged.coverage.coverage_size_at_least_threshold());
  }
  hash_size(builder, transitions.size());
  for (const ExactDirectAtLeast20VisibleTransition& transition :
       transitions) {
    hash_size(builder, transition.source_group_index);
    hash_enum(builder, transition.source_action);
    hash_size(builder, transition.source_q_r);
    hash_size(builder, transition.q_visible);
    hash_u64(builder, transition.resultant_root_id);
    hash_enum(builder, transition.kind);
    hash_size(builder, transition.prior_visible_root_ids.size());
    for (const ExactDirectAtLeast20RootId root_id :
         transition.prior_visible_root_ids) {
      hash_u64(builder, root_id);
    }
    hash_size(builder, transition.retained_smallest_point_ids.size());
    for (const spatial::PointId point_id :
         transition.retained_smallest_point_ids) {
      hash_u64(builder, point_id);
    }
  }
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId checkpoint_digest(
    const ExactDirectAtLeast20StreamViewCheckpoint& checkpoint) {
  contract::CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/direct-at-least20/checkpoint/v1/");
  hash_u64(builder, checkpoint.source_session_authority_id);
  hash_size(builder, checkpoint.next_source_batch_cursor);
  hash_size(builder, checkpoint.source_epoch);
  hash_id(builder, checkpoint.source_scientific_digest);
  hash_id(builder, checkpoint.source_commit_digest);
  hash_id(builder, checkpoint.view_digest);
  hash_size(builder, checkpoint.roots.size());
  for (const ExactDirectAtLeast20CheckpointRootRecord& root :
       checkpoint.roots) {
    hash_size(builder, root.order);
    hash_u64(builder, root.root_id);
    hash_bool(builder, root.active);
    hash_size(builder, root.retained_smallest_point_ids.size());
    for (const spatial::PointId point_id :
         root.retained_smallest_point_ids) {
      hash_u64(builder, point_id);
    }
  }
  hash_size(builder, checkpoint.order_cursors.size());
  for (const ExactDirectAtLeast20CheckpointOrderCursor& cursor :
       checkpoint.order_cursors) {
    hash_size(builder, cursor.order);
    hash_level(builder, cursor.last_squared_level);
  }
  hash_size(builder, checkpoint.active_root_count);
  hash_size(builder, checkpoint.retained_point_reference_count);
  hash_bool(builder, checkpoint.source_pruning_performed);
  hash_bool(builder, checkpoint.public_status_claimed);
  return builder.finalize();
}

[[nodiscard]] bool checkpoint_shape_is_valid(
    const ExactDirectAtLeast20StreamViewCheckpoint& checkpoint,
    const ExactDirectAtLeast20StreamViewBudget& budget) noexcept {
  if (checkpoint.schema_version !=
          direct_at_least20_stream_view_schema_version ||
      checkpoint.source_session_authority_id == 0U ||
      id_is_zero(checkpoint.source_scientific_digest) ||
      checkpoint.next_source_batch_cursor != checkpoint.source_epoch ||
      (checkpoint.next_source_batch_cursor == 0U &&
       (!id_is_zero(checkpoint.source_commit_digest) ||
        !checkpoint.roots.empty() || !checkpoint.order_cursors.empty())) ||
      (checkpoint.next_source_batch_cursor != 0U &&
       id_is_zero(checkpoint.source_commit_digest)) ||
      checkpoint.source_pruning_performed ||
      checkpoint.public_status_claimed ||
      checkpoint.roots.size() > budget.maximum_root_state_count ||
      checkpoint.roots.size() >
          budget.maximum_checkpoint_root_state_count ||
      checkpoint.active_root_count > budget.maximum_active_root_count ||
      checkpoint.retained_point_reference_count >
          budget.maximum_retained_point_reference_count ||
      checkpoint.retained_point_reference_count >
          budget.maximum_checkpoint_retained_point_reference_count ||
      checkpoint.order_cursors.size() >
          direct_at_least20_stream_view_maximum_order) {
    return false;
  }

  std::size_t active_root_count = 0U;
  std::size_t retained_point_reference_count = 0U;
  std::optional<RootKey> previous_root;
  for (const ExactDirectAtLeast20CheckpointRootRecord& root :
       checkpoint.roots) {
    const RootKey key{root.order, root.root_id};
    if (root.order == 0U ||
        root.order > direct_at_least20_stream_view_maximum_order ||
        (previous_root.has_value() && !(previous_root.value() < key)) ||
        root.retained_smallest_point_ids.size() >
            direct_at_least20_stream_view_threshold ||
        (!root.active && !root.retained_smallest_point_ids.empty()) ||
        (root.active && root.retained_smallest_point_ids.empty()) ||
        !std::is_sorted(
            root.retained_smallest_point_ids.begin(),
            root.retained_smallest_point_ids.end()) ||
        std::adjacent_find(
            root.retained_smallest_point_ids.begin(),
            root.retained_smallest_point_ids.end()) !=
            root.retained_smallest_point_ids.end()) {
      return false;
    }
    previous_root = key;
    if (root.active) {
      ++active_root_count;
    }
    if (!checked_add(
            retained_point_reference_count,
            root.retained_smallest_point_ids.size(),
            retained_point_reference_count)) {
      return false;
    }
  }
  if (active_root_count != checkpoint.active_root_count ||
      retained_point_reference_count !=
          checkpoint.retained_point_reference_count) {
    return false;
  }

  std::size_t previous_order = 0U;
  for (const ExactDirectAtLeast20CheckpointOrderCursor& cursor :
       checkpoint.order_cursors) {
    if (cursor.order == 0U ||
        cursor.order > direct_at_least20_stream_view_maximum_order ||
        cursor.order <= previous_order) {
      return false;
    }
    previous_order = cursor.order;
  }
  for (const ExactDirectAtLeast20CheckpointRootRecord& root :
       checkpoint.roots) {
    const auto cursor = std::lower_bound(
        checkpoint.order_cursors.begin(),
        checkpoint.order_cursors.end(),
        root.order,
        [](const ExactDirectAtLeast20CheckpointOrderCursor& candidate,
           std::size_t order) { return candidate.order < order; });
    if (cursor == checkpoint.order_cursors.end() ||
        cursor->order != root.order) {
      return false;
    }
  }
  return true;
}

}  // namespace

struct ExactDirectAtLeast20CommittedBatch::Impl {
  ExactDirectAtLeast20CommittedBatchInput input{};
  ExactDirectAtLeast20CommittedBatchCounters counters{};
  contract::CanonicalId source_commit_digest{};
  bool sealed{false};
  bool consumed{false};
};

struct ExactDirectAtLeast20StreamViewSession::Impl {
  Impl(
      std::uint64_t authority_id,
      std::size_t next_cursor,
      std::size_t epoch_value,
      contract::CanonicalId scientific_digest,
      contract::CanonicalId source_digest,
      contract::CanonicalId view_digest_value,
      ExactDirectAtLeast20StreamViewBudget budget_value)
      : source_session_authority_id(authority_id),
        next_source_batch_cursor(next_cursor),
        source_epoch(epoch_value),
        source_scientific_digest(std::move(scientific_digest)),
        source_commit_digest(std::move(source_digest)),
        view_digest(std::move(view_digest_value)),
        budget(std::move(budget_value)) {}

  std::uint64_t source_session_authority_id{};
  std::size_t next_source_batch_cursor{};
  std::size_t source_epoch{};
  contract::CanonicalId source_scientific_digest{};
  contract::CanonicalId source_commit_digest{};
  contract::CanonicalId view_digest{};
  ExactDirectAtLeast20StreamViewBudget budget{};
  std::vector<RootState> roots;
  std::vector<RootLookupEntry> root_lookup;
  std::vector<OrderCursorState> order_cursors;
  std::size_t active_root_count{};
  std::size_t retained_point_reference_count{};
};

ExactDirectAtLeast20CommittedBatch::ExactDirectAtLeast20CommittedBatch()
    noexcept = default;
ExactDirectAtLeast20CommittedBatch::~ExactDirectAtLeast20CommittedBatch() =
    default;
ExactDirectAtLeast20CommittedBatch::ExactDirectAtLeast20CommittedBatch(
    ExactDirectAtLeast20CommittedBatch&&) noexcept = default;
ExactDirectAtLeast20CommittedBatch&
ExactDirectAtLeast20CommittedBatch::operator=(
    ExactDirectAtLeast20CommittedBatch&&) noexcept = default;
ExactDirectAtLeast20CommittedBatch::ExactDirectAtLeast20CommittedBatch(
    std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectAtLeast20CommittedBatch::valid_relative_transcript()
    const noexcept {
  return impl_ != nullptr && impl_->sealed && !impl_->consumed;
}

bool ExactDirectAtLeast20CommittedBatch::consumed() const noexcept {
  return impl_ == nullptr || impl_->consumed;
}

std::uint64_t
ExactDirectAtLeast20CommittedBatch::source_session_authority_id()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->input.source_session_authority_id;
}

std::size_t ExactDirectAtLeast20CommittedBatch::source_batch_cursor_before()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->input.source_batch_cursor_before;
}

std::size_t ExactDirectAtLeast20CommittedBatch::source_batch_cursor_after()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->input.source_batch_cursor_after;
}

std::size_t ExactDirectAtLeast20CommittedBatch::source_epoch_before()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->input.source_epoch_before;
}

std::size_t ExactDirectAtLeast20CommittedBatch::source_epoch_after()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->input.source_epoch_after;
}

std::size_t ExactDirectAtLeast20CommittedBatch::order() const noexcept {
  return impl_ == nullptr ? 0U : impl_->input.order;
}

const exact::ExactLevel& ExactDirectAtLeast20CommittedBatch::squared_level()
    const noexcept {
  static const exact::ExactLevel empty_level{};
  return impl_ == nullptr ? empty_level : impl_->input.squared_level;
}

const contract::CanonicalId&
ExactDirectAtLeast20CommittedBatch::source_scientific_digest()
    const noexcept {
  static const contract::CanonicalId empty_digest{};
  return impl_ == nullptr ? empty_digest
                          : impl_->input.source_scientific_digest;
}

const contract::CanonicalId&
ExactDirectAtLeast20CommittedBatch::previous_source_commit_digest()
    const noexcept {
  static const contract::CanonicalId empty_digest{};
  return impl_ == nullptr ? empty_digest
                          : impl_->input.previous_source_commit_digest;
}

const contract::CanonicalId&
ExactDirectAtLeast20CommittedBatch::source_commit_digest() const noexcept {
  static const contract::CanonicalId empty_digest{};
  return impl_ == nullptr ? empty_digest : impl_->source_commit_digest;
}

const ExactDirectAtLeast20CommittedBatchCounters&
ExactDirectAtLeast20CommittedBatch::counters() const noexcept {
  static const ExactDirectAtLeast20CommittedBatchCounters empty_counters{};
  return impl_ == nullptr ? empty_counters : impl_->counters;
}

bool ExactDirectAtLeast20CommittedBatchBuildResult::
    certified_relative_transcript() const noexcept {
  return decision == ExactDirectAtLeast20CommittedBatchDecision::
                         complete_canonical_committed_batch_relative_transcript &&
      batch.has_value() && batch->valid_relative_transcript() &&
      batch->counters() == counters;
}

ExactDirectAtLeast20CommittedBatchBuildResult
seal_exact_direct_at_least20_committed_batch_relative_transcript(
    ExactDirectAtLeast20CommittedBatchInput&& input,
    const ExactDirectAtLeast20StreamViewBudget& budget) {
  ExactDirectAtLeast20CommittedBatchBuildResult result;
  if (input.schema_version !=
      direct_at_least20_stream_view_schema_version) {
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        no_batch_schema_rejected;
    return result;
  }
  if (input.source_session_authority_id == 0U ||
      input.source_batch_cursor_before ==
          std::numeric_limits<std::size_t>::max() ||
      input.source_epoch_before == std::numeric_limits<std::size_t>::max() ||
      input.source_batch_cursor_after !=
          input.source_batch_cursor_before + 1U ||
      input.source_epoch_after != input.source_epoch_before + 1U) {
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        no_batch_stamp_rejected;
    return result;
  }
  if (input.order == 0U ||
      input.order > direct_at_least20_stream_view_maximum_order) {
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        no_batch_order_rejected;
    return result;
  }
  if (!input.complete_equal_level_batch_committed ||
      !input.source_actions_qr_root_successors_and_coverage_delta_certified ||
      id_is_zero(input.source_scientific_digest) ||
      input.source_pruned_by_min_cluster_size ||
      input.source_public_status_claimed) {
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        no_batch_source_commit_premise_rejected;
    return result;
  }
  if (input.groups.size() > budget.maximum_batch_group_count) {
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        no_batch_group_budget_exhausted;
    return result;
  }

  std::size_t prior_root_reference_count = 0U;
  std::size_t raw_delta_count = 0U;
  for (const ExactDirectAtLeast20CommittedGroupInput& group : input.groups) {
    if (!checked_add(
            prior_root_reference_count,
            group.prior_root_ids.size(),
            prior_root_reference_count) ||
        !checked_add(
            raw_delta_count,
            group.coverage_delta_point_ids.size(),
            raw_delta_count)) {
      result.decision = ExactDirectAtLeast20CommittedBatchDecision::
          no_batch_input_capacity_overflow;
      return result;
    }
  }
  if (prior_root_reference_count >
      budget.maximum_batch_prior_root_reference_count) {
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        no_batch_prior_root_budget_exhausted;
    return result;
  }
  if (raw_delta_count >
      budget.maximum_batch_coverage_delta_point_reference_count) {
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        no_batch_coverage_delta_budget_exhausted;
    return result;
  }

  try {
    std::sort(
        input.groups.begin(),
        input.groups.end(),
        [](const ExactDirectAtLeast20CommittedGroupInput& left,
           const ExactDirectAtLeast20CommittedGroupInput& right) {
          return left.source_group_index < right.source_group_index;
        });
    std::vector<ExactDirectAtLeast20RootId> all_prior_roots;
    std::vector<ExactDirectAtLeast20RootId> all_resultant_roots;
    all_prior_roots.reserve(prior_root_reference_count);
    all_resultant_roots.reserve(input.groups.size());
    std::size_t deduplicated_delta_count = 0U;
    std::optional<std::size_t> previous_group_index;
    for (ExactDirectAtLeast20CommittedGroupInput& group : input.groups) {
      if (previous_group_index.has_value() &&
          previous_group_index.value() == group.source_group_index) {
        result.decision = ExactDirectAtLeast20CommittedBatchDecision::
            no_batch_group_shape_rejected;
        return result;
      }
      previous_group_index = group.source_group_index;
      std::sort(group.prior_root_ids.begin(), group.prior_root_ids.end());
      if (std::adjacent_find(
              group.prior_root_ids.begin(),
              group.prior_root_ids.end()) != group.prior_root_ids.end()) {
        result.decision = ExactDirectAtLeast20CommittedBatchDecision::
            no_batch_group_shape_rejected;
        return result;
      }
      std::sort(
          group.coverage_delta_point_ids.begin(),
          group.coverage_delta_point_ids.end());
      group.coverage_delta_point_ids.erase(
          std::unique(
              group.coverage_delta_point_ids.begin(),
              group.coverage_delta_point_ids.end()),
          group.coverage_delta_point_ids.end());
      if (!checked_add(
              deduplicated_delta_count,
              group.coverage_delta_point_ids.size(),
              deduplicated_delta_count) ||
          !action_shape_is_valid(group)) {
        result.decision = ExactDirectAtLeast20CommittedBatchDecision::
            no_batch_group_shape_rejected;
        return result;
      }
      if (group.source_action !=
              ExactDirectAtLeast20SourceAction::continuation &&
          std::binary_search(
              group.prior_root_ids.begin(),
              group.prior_root_ids.end(),
              group.resultant_root_id)) {
        result.decision = ExactDirectAtLeast20CommittedBatchDecision::
            no_batch_group_shape_rejected;
        return result;
      }
      all_prior_roots.insert(
          all_prior_roots.end(),
          group.prior_root_ids.begin(),
          group.prior_root_ids.end());
      all_resultant_roots.push_back(group.resultant_root_id);
    }
    std::sort(all_prior_roots.begin(), all_prior_roots.end());
    std::sort(all_resultant_roots.begin(), all_resultant_roots.end());
    // Root/facet components form a partition, but their observation coverage
    // is a cover rather than a partition: distinct order-k components can
    // legitimately contain facets sharing PointIds while their connecting
    // coface is inactive.  Therefore only root ownership is exclusive across
    // groups; coverage deltas are deduplicated locally and may overlap.
    if (std::adjacent_find(all_prior_roots.begin(), all_prior_roots.end()) !=
            all_prior_roots.end() ||
        std::adjacent_find(
            all_resultant_roots.begin(),
            all_resultant_roots.end()) != all_resultant_roots.end()) {
      result.decision = ExactDirectAtLeast20CommittedBatchDecision::
          no_batch_cross_group_partition_rejected;
      return result;
    }
    for (const ExactDirectAtLeast20CommittedGroupInput& group : input.groups) {
      if (std::binary_search(
              all_prior_roots.begin(),
              all_prior_roots.end(),
              group.resultant_root_id) &&
          !(group.source_action ==
                ExactDirectAtLeast20SourceAction::continuation &&
            group.prior_root_ids.front() == group.resultant_root_id)) {
        result.decision = ExactDirectAtLeast20CommittedBatchDecision::
            no_batch_cross_group_partition_rejected;
        return result;
      }
    }

    std::size_t logical_input_entry_count = input.groups.size();
    if (!checked_add(
            logical_input_entry_count,
            prior_root_reference_count,
            logical_input_entry_count) ||
        !checked_add(
            logical_input_entry_count,
            deduplicated_delta_count,
            logical_input_entry_count)) {
      result.decision = ExactDirectAtLeast20CommittedBatchDecision::
          no_batch_input_capacity_overflow;
      return result;
    }

    auto impl = std::make_unique<ExactDirectAtLeast20CommittedBatch::Impl>();
    impl->input = std::move(input);
    impl->counters.group_count = impl->input.groups.size();
    impl->counters.prior_root_reference_count = prior_root_reference_count;
    impl->counters.coverage_delta_point_reference_count = raw_delta_count;
    impl->counters.deduplicated_coverage_delta_point_reference_count =
        deduplicated_delta_count;
    impl->counters.logical_input_entry_count = logical_input_entry_count;
    impl->source_commit_digest = committed_batch_digest(impl->input);
    impl->sealed = true;
    result.counters = impl->counters;
    result.batch.emplace(
        ExactDirectAtLeast20CommittedBatch{std::move(impl)});
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        complete_canonical_committed_batch_relative_transcript;
    return result;
  } catch (const std::bad_alloc&) {
    result.batch.reset();
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        no_batch_allocation_failed;
    return result;
  } catch (const std::exception&) {
    result.batch.reset();
    result.decision = ExactDirectAtLeast20CommittedBatchDecision::
        no_batch_group_shape_rejected;
    return result;
  }
}

bool ExactDirectAtLeast20StreamConsumeResult::
    certified_atomic_downstream_batch_fold() const noexcept {
  if (decision != ExactDirectAtLeast20StreamConsumeDecision::
                      complete_certified_atomic_downstream_batch_fold ||
      source_batch_cursor_before ==
          std::numeric_limits<std::size_t>::max() ||
      source_epoch_before == std::numeric_limits<std::size_t>::max() ||
      source_batch_cursor_after != source_batch_cursor_before + 1U ||
      source_epoch_after != source_epoch_before + 1U ||
      !complete_equal_level_batch_observed_before_visibility ||
      !hidden_roots_retained_in_source_fold || source_pruning_performed ||
      scientific_state_mutated_on_failure ||
      upstream_source_commit_capability_verified ||
      source_exactness_claimed || public_status_claimed ||
      !exact_relative_downstream_fold_certified ||
      counters.processed_group_count !=
          counters.hidden_result_count + counters.threshold_entry_count +
              counters.continuation_count + counters.multifusion_count ||
      visible_transitions.size() !=
          counters.threshold_entry_count + counters.continuation_count +
              counters.multifusion_count) {
    return false;
  }
  for (const ExactDirectAtLeast20VisibleTransition& transition :
       visible_transitions) {
    if (!transition.coverage_size_at_least_20 ||
        transition.retained_smallest_point_ids.size() !=
            direct_at_least20_stream_view_threshold ||
        transition.prior_visible_root_ids.size() != transition.q_visible ||
        visible_kind(transition.q_visible) != transition.kind) {
      return false;
    }
  }
  return true;
}

ExactDirectAtLeast20StreamViewSession::
    ExactDirectAtLeast20StreamViewSession() noexcept = default;
ExactDirectAtLeast20StreamViewSession::~ExactDirectAtLeast20StreamViewSession() =
    default;
ExactDirectAtLeast20StreamViewSession::
    ExactDirectAtLeast20StreamViewSession(
        ExactDirectAtLeast20StreamViewSession&&) noexcept = default;
ExactDirectAtLeast20StreamViewSession&
ExactDirectAtLeast20StreamViewSession::operator=(
    ExactDirectAtLeast20StreamViewSession&&) noexcept = default;
ExactDirectAtLeast20StreamViewSession::
    ExactDirectAtLeast20StreamViewSession(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectAtLeast20StreamViewSession::initialized() const noexcept {
  return impl_ != nullptr;
}

std::uint64_t
ExactDirectAtLeast20StreamViewSession::source_session_authority_id()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->source_session_authority_id;
}

std::size_t
ExactDirectAtLeast20StreamViewSession::next_source_batch_cursor()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->next_source_batch_cursor;
}

std::size_t ExactDirectAtLeast20StreamViewSession::source_epoch()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->source_epoch;
}

std::size_t ExactDirectAtLeast20StreamViewSession::root_state_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->roots.size();
}

std::size_t ExactDirectAtLeast20StreamViewSession::active_root_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->active_root_count;
}

std::size_t
ExactDirectAtLeast20StreamViewSession::retained_point_reference_count()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->retained_point_reference_count;
}

const contract::CanonicalId&
ExactDirectAtLeast20StreamViewSession::source_scientific_digest()
    const noexcept {
  static const contract::CanonicalId empty_digest{};
  return impl_ == nullptr ? empty_digest : impl_->source_scientific_digest;
}

const contract::CanonicalId&
ExactDirectAtLeast20StreamViewSession::source_commit_digest()
    const noexcept {
  static const contract::CanonicalId empty_digest{};
  return impl_ == nullptr ? empty_digest : impl_->source_commit_digest;
}

const contract::CanonicalId&
ExactDirectAtLeast20StreamViewSession::view_digest() const noexcept {
  static const contract::CanonicalId empty_digest{};
  return impl_ == nullptr ? empty_digest : impl_->view_digest;
}

ExactDirectAtLeast20StreamConsumeResult
ExactDirectAtLeast20StreamViewSession::consume(
    ExactDirectAtLeast20CommittedBatch&& batch) noexcept {
  ExactDirectAtLeast20StreamConsumeResult result;
  result.source_pruning_performed = false;
  result.scientific_state_mutated_on_failure = false;
  if (impl_ == nullptr) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_session_not_initialized;
    return result;
  }
  result.view_digest_before = impl_->view_digest;
  if (!batch.valid_relative_transcript()) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_batch_not_sealed_or_already_consumed;
    return result;
  }
  ExactDirectAtLeast20CommittedBatch::Impl& source = *batch.impl_;
  const ExactDirectAtLeast20CommittedBatchInput& input = source.input;
  result.source_batch_cursor_before = input.source_batch_cursor_before;
  result.source_batch_cursor_after = input.source_batch_cursor_after;
  result.source_epoch_before = input.source_epoch_before;
  result.source_epoch_after = input.source_epoch_after;
  result.order = input.order;
  result.source_commit_digest = source.source_commit_digest;
  result.counters.processed_group_count = input.groups.size();
  result.counters.processed_prior_root_reference_count =
      source.counters.prior_root_reference_count;
  result.counters.processed_coverage_delta_point_reference_count =
      source.counters.deduplicated_coverage_delta_point_reference_count;
  result.counters.active_root_count_before = impl_->active_root_count;
  result.counters.retained_point_reference_count_before =
      impl_->retained_point_reference_count;

  // ExactLevel owns multiprecision integers.  Even this diagnostic copy may
  // allocate, so keep it in the prepare region covered by a failure result.
  try {
    result.squared_level = input.squared_level;
  } catch (const std::exception&) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_allocation_failed;
    return result;
  }

  if (input.source_session_authority_id !=
      impl_->source_session_authority_id) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_foreign_source_session_rejected;
    return result;
  }
  if (input.source_scientific_digest !=
      impl_->source_scientific_digest) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_foreign_source_science_rejected;
    return result;
  }
  if (input.source_batch_cursor_before != impl_->next_source_batch_cursor ||
      input.source_epoch_before != impl_->source_epoch ||
      input.previous_source_commit_digest != impl_->source_commit_digest) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_stale_source_stamp_rejected;
    return result;
  }
  try {
    for (const OrderCursorState& cursor : impl_->order_cursors) {
      if (!batch_key_less(
              cursor.last_squared_level,
              cursor.order,
              input.squared_level,
              input.order)) {
        result.decision = ExactDirectAtLeast20StreamConsumeDecision::
            no_nonincreasing_order_level_rejected;
        return result;
      }
    }
  } catch (const std::exception&) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_allocation_failed;
    return result;
  }
  const auto existing_order_cursor =
      find_order_cursor(impl_->order_cursors, input.order);

  std::size_t retired_root_count = 0U;
  std::size_t retired_retained_count = 0U;
  std::size_t new_root_state_count = 0U;
  std::size_t staged_retained_count = 0U;
  std::size_t visible_transition_count = 0U;
  std::size_t visible_child_reference_count = 0U;
  for (const ExactDirectAtLeast20CommittedGroupInput& group : input.groups) {
    StackCappedCoverage staged_coverage;
    for (const ExactDirectAtLeast20RootId child_root_id :
         group.prior_root_ids) {
      const auto lookup = find_root_lookup(
          impl_->root_lookup, RootKey{input.order, child_root_id});
      if (lookup == impl_->root_lookup.end() ||
          !impl_->roots[lookup->root_state_index].active) {
        result.decision = ExactDirectAtLeast20StreamConsumeDecision::
            no_unknown_or_inactive_prior_root_rejected;
        return result;
      }
      const RootState& child = impl_->roots[lookup->root_state_index];
      staged_coverage.merge(child.coverage.smallest_distinct_point_ids());
      if (!checked_add(retired_root_count, 1U, retired_root_count) ||
          !checked_add(
              retired_retained_count,
              child.coverage.retained_distinct_count(),
              retired_retained_count)) {
        result.decision = ExactDirectAtLeast20StreamConsumeDecision::
            no_batch_partition_rejected;
        return result;
      }
      if (child.coverage.coverage_size_at_least_threshold()) {
        ++visible_child_reference_count;
      }
    }
    const std::size_t delta_cap = std::min(
        direct_at_least20_stream_view_threshold,
        group.coverage_delta_point_ids.size());
    staged_coverage.merge(std::span<const spatial::PointId>{
        group.coverage_delta_point_ids.data(), delta_cap});
    if (!checked_add(
            staged_retained_count,
            staged_coverage.size,
            staged_retained_count)) {
      result.decision = ExactDirectAtLeast20StreamConsumeDecision::
          no_staging_budget_exhausted;
      return result;
    }
    if (staged_coverage.visible()) {
      ++visible_transition_count;
    }

    const RootKey result_key{input.order, group.resultant_root_id};
    const auto result_lookup = find_root_lookup(impl_->root_lookup, result_key);
    const bool reuses_own_continuation_root =
        group.source_action ==
            ExactDirectAtLeast20SourceAction::continuation &&
        group.prior_root_ids.front() == group.resultant_root_id;
    if (result_lookup != impl_->root_lookup.end()) {
      if (!reuses_own_continuation_root ||
          result_lookup->root_state_index >= impl_->roots.size() ||
          !impl_->roots[result_lookup->root_state_index].active) {
        result.decision = ExactDirectAtLeast20StreamConsumeDecision::
            no_resultant_root_collision_rejected;
        return result;
      }
    } else {
      ++new_root_state_count;
    }
  }

  std::size_t target_root_state_count = 0U;
  std::size_t target_active_root_count = 0U;
  std::size_t target_retained_point_count = 0U;
  if (!checked_add(
          impl_->roots.size(),
          new_root_state_count,
          target_root_state_count) ||
      !checked_sub_add(
          impl_->active_root_count,
          retired_root_count,
          input.groups.size(),
          target_active_root_count) ||
      !checked_sub_add(
          impl_->retained_point_reference_count,
          retired_retained_count,
          staged_retained_count,
          target_retained_point_count)) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_batch_partition_rejected;
    return result;
  }
  result.counters.active_root_count_after = target_active_root_count;
  result.counters.retained_point_reference_count_after =
      target_retained_point_count;
  if (target_root_state_count > impl_->budget.maximum_root_state_count) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_root_state_budget_exhausted;
    return result;
  }
  if (target_active_root_count > impl_->budget.maximum_active_root_count) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_active_root_budget_exhausted;
    return result;
  }
  if (target_retained_point_count >
      impl_->budget.maximum_retained_point_reference_count) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_retained_point_budget_exhausted;
    return result;
  }
  if (visible_transition_count >
      impl_->budget.maximum_batch_visible_transition_count) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_visible_transition_budget_exhausted;
    return result;
  }
  if (visible_child_reference_count >
      impl_->budget.maximum_batch_visible_child_reference_count) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_visible_child_reference_budget_exhausted;
    return result;
  }

  std::size_t required_staging_entry_count = input.groups.size();
  if (!checked_add(
          required_staging_entry_count,
          source.counters.prior_root_reference_count,
          required_staging_entry_count) ||
      !checked_add(
          required_staging_entry_count,
          staged_retained_count,
          required_staging_entry_count) ||
      !checked_add(
          required_staging_entry_count,
          visible_transition_count,
          required_staging_entry_count) ||
      !checked_add(
          required_staging_entry_count,
          visible_child_reference_count,
          required_staging_entry_count) ||
      required_staging_entry_count >
          impl_->budget.maximum_staging_entry_count) {
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_staging_budget_exhausted;
    return result;
  }
  result.counters.required_staging_entry_count =
      required_staging_entry_count;

  try {
    // Capacity-only mutations happen after every logical cap has passed and
    // before any scientific field changes.  Subsequent commit operations are
    // allocation-free and use only nothrow moves.
    impl_->roots.reserve(target_root_state_count);
    impl_->root_lookup.reserve(target_root_state_count);
    if (existing_order_cursor == impl_->order_cursors.end()) {
      impl_->order_cursors.reserve(impl_->order_cursors.size() + 1U);
    }

    std::vector<StagedGroup> staged_groups;
    staged_groups.reserve(input.groups.size());
    result.visible_transitions.reserve(visible_transition_count);
    for (const ExactDirectAtLeast20CommittedGroupInput& group : input.groups) {
      CappedDistinctPointCoverage coverage{
          direct_at_least20_stream_view_threshold};
      std::vector<std::size_t> child_indices;
      child_indices.reserve(group.prior_root_ids.size());
      std::vector<ExactDirectAtLeast20RootId> prior_visible_roots;
      prior_visible_roots.reserve(group.prior_root_ids.size());
      for (const ExactDirectAtLeast20RootId child_root_id :
           group.prior_root_ids) {
        const auto lookup = find_root_lookup(
            impl_->root_lookup, RootKey{input.order, child_root_id});
        const RootState& child = impl_->roots[lookup->root_state_index];
        coverage.merge_from(child.coverage);
        child_indices.push_back(lookup->root_state_index);
        if (child.coverage.coverage_size_at_least_threshold()) {
          prior_visible_roots.push_back(child_root_id);
        }
      }
      CappedDistinctPointCoverage delta{
          direct_at_least20_stream_view_threshold};
      const std::size_t delta_cap = std::min(
          direct_at_least20_stream_view_threshold,
          group.coverage_delta_point_ids.size());
      for (std::size_t index = 0U; index < delta_cap; ++index) {
        static_cast<void>(
            delta.insert_point_id(group.coverage_delta_point_ids[index]));
      }
      coverage.merge_from(delta);

      StagedGroup staged{&group, std::move(coverage)};
      staged.child_root_state_indices = std::move(child_indices);
      staged.prior_visible_root_ids = std::move(prior_visible_roots);
      staged.reuses_continuation_root_state =
          group.source_action ==
              ExactDirectAtLeast20SourceAction::continuation &&
          group.prior_root_ids.front() == group.resultant_root_id;
      if (staged.reuses_continuation_root_state) {
        staged.continuation_root_state_index =
            staged.child_root_state_indices.front();
      }

      if (staged.coverage.coverage_size_at_least_threshold()) {
        ExactDirectAtLeast20VisibleTransition transition;
        transition.source_group_index = group.source_group_index;
        transition.order = input.order;
        transition.squared_level = input.squared_level;
        transition.source_action = group.source_action;
        transition.source_q_r = group.q_r;
        transition.q_visible = staged.prior_visible_root_ids.size();
        transition.resultant_root_id = group.resultant_root_id;
        transition.prior_visible_root_ids = staged.prior_visible_root_ids;
        transition.retained_smallest_point_ids.assign(
            staged.coverage.smallest_distinct_point_ids().begin(),
            staged.coverage.smallest_distinct_point_ids().end());
        transition.kind = visible_kind(transition.q_visible);
        transition.coverage_size_at_least_20 = true;
        switch (transition.kind) {
          case ExactDirectAtLeast20VisibleTransitionKind::threshold_entry:
            ++result.counters.threshold_entry_count;
            break;
          case ExactDirectAtLeast20VisibleTransitionKind::continuation:
            ++result.counters.continuation_count;
            break;
          case ExactDirectAtLeast20VisibleTransitionKind::multifusion:
            ++result.counters.multifusion_count;
            break;
        }
        result.visible_transitions.push_back(std::move(transition));
      } else {
        ++result.counters.hidden_result_count;
      }
      staged_groups.push_back(std::move(staged));
    }

    exact::ExactLevel committed_level = input.squared_level;
    const contract::CanonicalId committed_view_digest = next_view_digest(
        impl_->view_digest,
        source.source_commit_digest,
        input.order,
        input.squared_level,
        staged_groups,
        result.visible_transitions);

    // Allocation-free scientific commit.
    for (StagedGroup& staged : staged_groups) {
      for (const std::size_t child_index : staged.child_root_state_indices) {
        RootState& child = impl_->roots[child_index];
        child.active = false;
        child.coverage = CappedDistinctPointCoverage{
            direct_at_least20_stream_view_threshold};
      }
      const RootKey result_key{
          input.order, staged.source->resultant_root_id};
      if (staged.reuses_continuation_root_state) {
        RootState& result_root =
            impl_->roots[staged.continuation_root_state_index];
        result_root.coverage = std::move(staged.coverage);
        result_root.active = true;
      } else {
        const std::size_t new_index = impl_->roots.size();
        impl_->roots.emplace_back(
            result_key, std::move(staged.coverage), true);
        const auto insert_position = std::lower_bound(
            impl_->root_lookup.begin(),
            impl_->root_lookup.end(),
            result_key,
            [](const RootLookupEntry& entry, const RootKey& key) {
              return entry.key < key;
            });
        impl_->root_lookup.insert(
            insert_position, RootLookupEntry{result_key, new_index});
      }
    }
    auto order_cursor = find_order_cursor(impl_->order_cursors, input.order);
    if (order_cursor == impl_->order_cursors.end()) {
      const auto insert_position = std::lower_bound(
          impl_->order_cursors.begin(),
          impl_->order_cursors.end(),
          input.order,
          [](const OrderCursorState& cursor, std::size_t order) {
            return cursor.order < order;
          });
      impl_->order_cursors.insert(
          insert_position,
          OrderCursorState{input.order, std::move(committed_level)});
    } else {
      order_cursor->last_squared_level = std::move(committed_level);
    }
    impl_->active_root_count = target_active_root_count;
    impl_->retained_point_reference_count = target_retained_point_count;
    impl_->next_source_batch_cursor = input.source_batch_cursor_after;
    impl_->source_epoch = input.source_epoch_after;
    impl_->source_commit_digest = source.source_commit_digest;
    impl_->view_digest = committed_view_digest;
    source.consumed = true;

    result.view_digest_after = impl_->view_digest;
    result.complete_equal_level_batch_observed_before_visibility = true;
    result.hidden_roots_retained_in_source_fold = true;
    result.exact_relative_downstream_fold_certified = true;
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        complete_certified_atomic_downstream_batch_fold;
    return result;
  } catch (const std::bad_alloc&) {
    result.visible_transitions.clear();
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_allocation_failed;
    return result;
  } catch (const std::exception&) {
    result.visible_transitions.clear();
    result.decision = ExactDirectAtLeast20StreamConsumeDecision::
        no_batch_partition_rejected;
    return result;
  }
}

bool ExactDirectAtLeast20CheckpointBuildResult::certified_checkpoint()
    const noexcept {
  return decision == ExactDirectAtLeast20CheckpointDecision::
                         complete_certified_checkpoint &&
      checkpoint.has_value() && !checkpoint->source_pruning_performed &&
      !checkpoint->public_status_claimed;
}

ExactDirectAtLeast20CheckpointBuildResult
ExactDirectAtLeast20StreamViewSession::make_checkpoint() const noexcept {
  ExactDirectAtLeast20CheckpointBuildResult result;
  if (impl_ == nullptr) {
    result.decision = ExactDirectAtLeast20CheckpointDecision::
        no_session_not_initialized;
    return result;
  }
  if (impl_->roots.size() >
          impl_->budget.maximum_checkpoint_root_state_count ||
      impl_->retained_point_reference_count >
          impl_->budget.maximum_checkpoint_retained_point_reference_count) {
    result.decision = ExactDirectAtLeast20CheckpointDecision::
        no_checkpoint_budget_exhausted;
    return result;
  }
  try {
    ExactDirectAtLeast20StreamViewCheckpoint checkpoint;
    checkpoint.source_session_authority_id =
        impl_->source_session_authority_id;
    checkpoint.next_source_batch_cursor = impl_->next_source_batch_cursor;
    checkpoint.source_epoch = impl_->source_epoch;
    checkpoint.source_scientific_digest =
        impl_->source_scientific_digest;
    checkpoint.source_commit_digest = impl_->source_commit_digest;
    checkpoint.view_digest = impl_->view_digest;
    checkpoint.active_root_count = impl_->active_root_count;
    checkpoint.retained_point_reference_count =
        impl_->retained_point_reference_count;
    checkpoint.roots.reserve(impl_->roots.size());
    for (const RootState& root : impl_->roots) {
      ExactDirectAtLeast20CheckpointRootRecord record;
      record.order = root.key.order;
      record.root_id = root.key.root_id;
      record.active = root.active;
      record.retained_smallest_point_ids.assign(
          root.coverage.smallest_distinct_point_ids().begin(),
          root.coverage.smallest_distinct_point_ids().end());
      checkpoint.roots.push_back(std::move(record));
    }
    std::sort(
        checkpoint.roots.begin(),
        checkpoint.roots.end(),
        [](const ExactDirectAtLeast20CheckpointRootRecord& left,
           const ExactDirectAtLeast20CheckpointRootRecord& right) {
          return RootKey{left.order, left.root_id} <
              RootKey{right.order, right.root_id};
        });
    checkpoint.order_cursors.reserve(impl_->order_cursors.size());
    for (const OrderCursorState& cursor : impl_->order_cursors) {
      checkpoint.order_cursors.push_back(
          ExactDirectAtLeast20CheckpointOrderCursor{
              cursor.order, cursor.last_squared_level});
    }
    checkpoint.checkpoint_digest = checkpoint_digest(checkpoint);
    if (!checkpoint_shape_is_valid(checkpoint, impl_->budget)) {
      result.decision = ExactDirectAtLeast20CheckpointDecision::
          no_checkpoint_shape_rejected;
      return result;
    }
    result.checkpoint.emplace(std::move(checkpoint));
    result.decision = ExactDirectAtLeast20CheckpointDecision::
        complete_certified_checkpoint;
    return result;
  } catch (const std::bad_alloc&) {
    result.checkpoint.reset();
    result.decision = ExactDirectAtLeast20CheckpointDecision::
        no_allocation_failed;
    return result;
  } catch (const std::exception&) {
    result.checkpoint.reset();
    result.decision = ExactDirectAtLeast20CheckpointDecision::
        no_checkpoint_shape_rejected;
    return result;
  }
}

bool ExactDirectAtLeast20StreamViewSessionInitializationResult::
    certified_initialized_session() const noexcept {
  const bool decision_valid =
      decision == ExactDirectAtLeast20SessionInitializationDecision::
                      complete_initialized_empty_session ||
      decision == ExactDirectAtLeast20SessionInitializationDecision::
                      complete_restored_certified_session;
  return decision_valid && session.has_value() && session->initialized() &&
      budget_preflight_certified && !source_pruning_enabled &&
      !upstream_resident_adapter_certified;
}

ExactDirectAtLeast20StreamViewSessionInitializationResult
initialize_exact_direct_at_least20_stream_view_session(
    std::uint64_t source_session_authority_id,
    std::size_t next_source_batch_cursor,
    std::size_t source_epoch,
    const contract::CanonicalId& source_scientific_digest,
    const contract::CanonicalId& source_commit_digest,
    const ExactDirectAtLeast20StreamViewBudget& budget) {
  ExactDirectAtLeast20StreamViewSessionInitializationResult result;
  if (source_session_authority_id == 0U) {
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_source_session_authority_id_rejected;
    return result;
  }
  // An empty root arena is only a genesis constructor.  Attaching that empty
  // arena to an arbitrary committed prefix would neither replay nor certify
  // its hidden roots.  Every non-genesis start must use verified restore.
  if (next_source_batch_cursor != 0U || source_epoch != 0U ||
      !id_is_zero(source_commit_digest)) {
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_session_stamp_rejected;
    return result;
  }
  if (id_is_zero(source_scientific_digest)) {
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_source_scientific_digest_rejected;
    return result;
  }
  if (!budget_shape_is_valid(budget)) {
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_session_budget_rejected;
    return result;
  }
  result.budget_preflight_certified = true;
  try {
    const contract::CanonicalId view_digest = initial_view_digest(
        next_source_batch_cursor,
        source_epoch,
        source_scientific_digest,
        source_commit_digest);
    auto impl =
        std::make_unique<ExactDirectAtLeast20StreamViewSession::Impl>(
            source_session_authority_id,
            next_source_batch_cursor,
            source_epoch,
            source_scientific_digest,
            source_commit_digest,
            view_digest,
            budget);
    result.session.emplace(
        ExactDirectAtLeast20StreamViewSession{std::move(impl)});
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        complete_initialized_empty_session;
    return result;
  } catch (const std::bad_alloc&) {
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_allocation_failed;
    return result;
  } catch (const std::exception&) {
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_session_stamp_rejected;
    return result;
  }
}

ExactDirectAtLeast20StreamViewSessionInitializationResult
restore_exact_direct_at_least20_stream_view_session(
    ExactDirectAtLeast20StreamViewCheckpoint&& checkpoint,
    std::uint64_t reminted_source_session_authority_id,
    const ExactDirectAtLeast20StreamViewBudget& budget) {
  ExactDirectAtLeast20StreamViewSessionInitializationResult result;
  if (reminted_source_session_authority_id == 0U) {
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_source_session_authority_id_rejected;
    return result;
  }
  if (!budget_shape_is_valid(budget)) {
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_session_budget_rejected;
    return result;
  }
  result.budget_preflight_certified = true;
  if (!checkpoint_shape_is_valid(checkpoint, budget)) {
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_checkpoint_shape_rejected;
    return result;
  }
  try {
    if (checkpoint.checkpoint_digest != checkpoint_digest(checkpoint)) {
      result.decision = ExactDirectAtLeast20SessionInitializationDecision::
          no_checkpoint_digest_rejected;
      return result;
    }
    result.checkpoint_freshly_verified = true;
    auto impl =
        std::make_unique<ExactDirectAtLeast20StreamViewSession::Impl>(
            reminted_source_session_authority_id,
            checkpoint.next_source_batch_cursor,
            checkpoint.source_epoch,
            checkpoint.source_scientific_digest,
            checkpoint.source_commit_digest,
            checkpoint.view_digest,
            budget);
    impl->roots.reserve(checkpoint.roots.size());
    impl->root_lookup.reserve(checkpoint.roots.size());
    for (const ExactDirectAtLeast20CheckpointRootRecord& record :
         checkpoint.roots) {
      CappedDistinctPointCoverage coverage{
          direct_at_least20_stream_view_threshold};
      for (const spatial::PointId point_id :
           record.retained_smallest_point_ids) {
        static_cast<void>(coverage.insert_point_id(point_id));
      }
      const RootKey key{record.order, record.root_id};
      const std::size_t index = impl->roots.size();
      impl->roots.emplace_back(key, std::move(coverage), record.active);
      impl->root_lookup.push_back(RootLookupEntry{key, index});
    }
    impl->order_cursors.reserve(checkpoint.order_cursors.size());
    for (const ExactDirectAtLeast20CheckpointOrderCursor& cursor :
         checkpoint.order_cursors) {
      impl->order_cursors.push_back(
          OrderCursorState{cursor.order, cursor.last_squared_level});
    }
    impl->active_root_count = checkpoint.active_root_count;
    impl->retained_point_reference_count =
        checkpoint.retained_point_reference_count;
    result.session.emplace(
        ExactDirectAtLeast20StreamViewSession{std::move(impl)});
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        complete_restored_certified_session;
    return result;
  } catch (const std::bad_alloc&) {
    result.session.reset();
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_allocation_failed;
    return result;
  } catch (const std::exception&) {
    result.session.reset();
    result.decision = ExactDirectAtLeast20SessionInitializationDecision::
        no_checkpoint_shape_rejected;
    return result;
  }
}

}  // namespace morsehgp3d::hierarchy
