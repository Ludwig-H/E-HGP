#include "morsehgp3d/hierarchy/direct_k1_forest_journal.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

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

class CanonicalDisjointSet {
 public:
  explicit CanonicalDisjointSet(std::size_t point_count)
      : parent_(point_count), component_node_ids_(point_count) {
    std::iota(parent_.begin(), parent_.end(), 0U);
    for (std::size_t index = 0U; index < point_count; ++index) {
      component_node_ids_[index] =
          static_cast<ExactDirectK1NodeId>(index);
    }
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    std::size_t root = value;
    while (parent_[root] != root) {
      root = parent_[root];
    }
    while (parent_[value] != value) {
      const std::size_t next = parent_[value];
      parent_[value] = root;
      value = next;
    }
    return root;
  }

  [[nodiscard]] std::size_t unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left == right) {
      return left;
    }
    const std::size_t root = std::min(left, right);
    const std::size_t child = std::max(left, right);
    parent_[child] = root;
    --component_count_;
    return root;
  }

  [[nodiscard]] ExactDirectK1NodeId component_node_id(
      std::size_t point) {
    return component_node_ids_[find(point)];
  }

  void set_component_node_id(
      std::size_t point,
      ExactDirectK1NodeId node_id) {
    component_node_ids_[find(point)] = node_id;
  }

  [[nodiscard]] std::size_t component_count() const noexcept {
    return component_count_;
  }

  void initialize_component_count(std::size_t point_count) noexcept {
    component_count_ = point_count;
  }

 private:
  std::vector<std::size_t> parent_;
  std::vector<ExactDirectK1NodeId> component_node_ids_;
  std::size_t component_count_{};
};

class LocalDisjointSet {
 public:
  explicit LocalDisjointSet(std::size_t count) : parent_(count) {
    std::iota(parent_.begin(), parent_.end(), 0U);
  }

  [[nodiscard]] std::size_t find(std::size_t value) {
    std::size_t root = value;
    while (parent_[root] != root) {
      root = parent_[root];
    }
    while (parent_[value] != value) {
      const std::size_t next = parent_[value];
      parent_[value] = root;
      value = next;
    }
    return root;
  }

  void unite(std::size_t left, std::size_t right) {
    left = find(left);
    right = find(right);
    if (left != right) {
      parent_[std::max(left, right)] = std::min(left, right);
    }
  }

 private:
  std::vector<std::size_t> parent_;
};

struct TemporarySaddle {
  std::size_t family_index{};
  std::size_t seed_indices[2U]{};
  spatial::PointId singleton_point_ids[2U]{};
  std::size_t pre_batch_component_roots[2U]{};
  ExactDirectK1NodeId pre_batch_root_node_ids[2U]{};
};

struct TemporaryGroup {
  std::vector<std::size_t> component_roots;
  std::vector<std::size_t> saddle_indices;
};

struct ReplayOutcome {
  ExactDirectK1ForestJournalResult result;
  bool source_seed_journal_certified{false};
  bool arm_root_bindings_match{true};
  bool saddle_records_match{true};
  bool atomic_groups_match{true};
  bool child_references_match{true};
  bool batches_match{true};
};

void clear_payload(ExactDirectK1ForestJournalResult& result) {
  result.arm_root_bindings.clear();
  result.saddle_records.clear();
  result.atomic_groups.clear();
  result.child_node_ids.clear();
  result.batches.clear();
  result.logical_added_storage_entry_count = 0U;
  result.combined_logical_storage_entry_count = 0U;
  result.root_node_id = 0U;
  result.node_count = 0U;
  result.counters = {};
}

[[nodiscard]] bool budget_is_sufficient(
    const ExactDirectK1ForestJournalResult& result) {
  return result.required_source_replay_entry_count <=
             result.requested_budget.maximum_source_replay_entry_count &&
         result.required_point_scratch_entry_count <=
             result.requested_budget.maximum_point_scratch_entry_count &&
         result.required_equal_level_scratch_entry_count <=
             result.requested_budget
                 .maximum_equal_level_scratch_entry_count &&
         result.required_arm_root_binding_count <=
             result.requested_budget.maximum_arm_root_binding_count &&
         result.required_saddle_record_count <=
             result.requested_budget.maximum_saddle_record_count &&
         result.required_atomic_group_capacity <=
             result.requested_budget.maximum_atomic_group_count &&
         result.required_child_reference_capacity <=
             result.requested_budget.maximum_child_reference_count &&
         result.required_batch_record_capacity <=
             result.requested_budget.maximum_batch_record_count;
}

template <typename Record>
void emit_record(
    std::vector<Record>& target,
    const std::vector<Record>* observed,
    const Record& record,
    std::size_t index,
    bool retain_payload,
    bool& matches) {
  if (observed != nullptr &&
      (index >= observed->size() || (*observed)[index] != record)) {
    matches = false;
  }
  if (retain_payload) {
    target.push_back(record);
  }
}

void emit_child(
    std::vector<ExactDirectK1NodeId>& target,
    const std::vector<ExactDirectK1NodeId>* observed,
    ExactDirectK1NodeId node_id,
    std::size_t index,
    bool retain_payload,
    bool& matches) {
  if (observed != nullptr &&
      (index >= observed->size() || (*observed)[index] != node_id)) {
    matches = false;
  }
  if (retain_payload) {
    target.push_back(node_id);
  }
}

[[nodiscard]] ReplayOutcome replay_direct_k1_forest(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectK1ForestBudget& budget,
    const ExactDirectK1ForestJournalResult* observed,
    bool retain_payload) {
  ReplayOutcome outcome;
  ExactDirectK1ForestJournalResult& result = outcome.result;
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
  result.scope = ExactDirectK1ForestScope::
      exact_order_one_forest_and_atomic_continuations_only;

  if (cloud.size() == 0U) {
    result.decision =
        ExactDirectK1ForestDecision::no_k1_rank_two_source_inconsistent;
    return outcome;
  }

  std::size_t order_one_family_count = 0U;
  std::size_t order_one_batch_count = 0U;
  std::size_t maximum_batch_saddle_count = 0U;
  std::size_t source_saddle_family_count = 0U;
  std::size_t source_arm_seed_count = 0U;
  std::optional<exact::ExactLevel> previous_order_one_level;
  std::size_t current_batch_saddle_count = 0U;
  for (const ExactDirectSupportEvent& event : source_facade.events) {
    if (event.saddle_order.has_value()) {
      const auto next_family_count =
          checked_add(source_saddle_family_count, 1U);
      const auto next_seed_count = checked_add(
          source_arm_seed_count,
          static_cast<std::size_t>(event.support_size));
      if (!next_family_count.has_value() || !next_seed_count.has_value()) {
        result.decision =
            ExactDirectK1ForestDecision::no_k1_capacity_overflow;
        return outcome;
      }
      source_saddle_family_count = *next_family_count;
      source_arm_seed_count = *next_seed_count;
    }
    if (event.saddle_order != std::optional<std::size_t>{1U}) {
      continue;
    }
    ++order_one_family_count;
    if (!previous_order_one_level.has_value() ||
        *previous_order_one_level != event.squared_level) {
      if (previous_order_one_level.has_value()) {
        maximum_batch_saddle_count =
            std::max(maximum_batch_saddle_count, current_batch_saddle_count);
      }
      previous_order_one_level = event.squared_level;
      current_batch_saddle_count = 1U;
      ++order_one_batch_count;
    } else {
      ++current_batch_saddle_count;
    }
  }
  maximum_batch_saddle_count =
      std::max(maximum_batch_saddle_count, current_batch_saddle_count);

  const auto source_journal_replay_bound = checked_linear_bound(
      3U, cloud.size(), 5U, source_facade.events.size());
  const auto source_family_seed_count = checked_add(
      source_saddle_family_count, source_arm_seed_count);
  std::optional<std::size_t> source_replay_count;
  if (source_journal_replay_bound.has_value() &&
      source_family_seed_count.has_value()) {
    source_replay_count = checked_add(
        *source_journal_replay_bound, *source_family_seed_count);
  }
  const auto point_scratch_count = checked_multiply(2U, cloud.size());
  const auto equal_level_scratch_count =
      checked_multiply(11U, maximum_batch_saddle_count);
  const auto binding_count = checked_multiply(2U, order_one_family_count);
  const auto child_capacity = checked_multiply(2U, cloud.size() - 1U);
  const auto added_limit_base = checked_linear_bound(
      2U, cloud.size(), 5U, order_one_family_count);
  const auto combined_limit_base = checked_linear_bound(
      5U, cloud.size(), 15U, source_facade.events.size());
  if (!source_journal_replay_bound.has_value() ||
      !source_replay_count.has_value() ||
      !point_scratch_count.has_value() ||
      !equal_level_scratch_count.has_value() ||
      !binding_count.has_value() || !child_capacity.has_value() ||
      !added_limit_base.has_value() ||
      !combined_limit_base.has_value() || *added_limit_base < 2U ||
      *combined_limit_base < 2U) {
    result.decision =
        ExactDirectK1ForestDecision::no_k1_capacity_overflow;
    return outcome;
  }

  result.required_source_replay_entry_count = *source_replay_count;
  result.required_point_scratch_entry_count = *point_scratch_count;
  result.required_equal_level_scratch_entry_count =
      *equal_level_scratch_count;
  result.required_arm_root_binding_count = *binding_count;
  result.required_saddle_record_count = order_one_family_count;
  result.required_atomic_group_capacity = order_one_family_count;
  result.required_child_reference_capacity = *child_capacity;
  result.required_batch_record_capacity = order_one_batch_count;
  result.logical_added_storage_entry_limit = *added_limit_base - 2U;
  result.combined_logical_storage_entry_limit =
      *combined_limit_base - 2U;

  if (!budget_is_sufficient(result)) {
    result.decision =
        ExactDirectK1ForestDecision::no_k1_budget_exhausted;
    return outcome;
  }
  result.budget_preflight_certified = true;

  const ExactDirectSaddleArmSeedStreamingVerification source_verification =
      verify_exact_direct_saddle_arm_seed_journal_streaming(
          cloud,
          source_facade,
          source_journal,
          trusted_seed_budget,
          source_seed_journal);
  outcome.source_seed_journal_certified =
      source_verification.result_certified;
  if (!outcome.source_seed_journal_certified) {
    result.decision = ExactDirectK1ForestDecision::
        no_k1_source_seed_journal_rejected;
    return outcome;
  }
  result.source_seed_journal_streaming_replayed = true;

  if (retain_payload) {
    result.arm_root_bindings.reserve(*binding_count);
    result.saddle_records.reserve(order_one_family_count);
    result.atomic_groups.reserve(order_one_family_count);
    result.child_node_ids.reserve(*child_capacity);
    result.batches.reserve(order_one_batch_count);
  }

  const std::vector<ExactDirectK1ArmRootBinding>* observed_bindings =
      observed == nullptr ? nullptr : &observed->arm_root_bindings;
  const std::vector<ExactDirectK1SaddleRecord>* observed_saddles =
      observed == nullptr ? nullptr : &observed->saddle_records;
  const std::vector<ExactDirectK1AtomicGroup>* observed_groups =
      observed == nullptr ? nullptr : &observed->atomic_groups;
  const std::vector<ExactDirectK1NodeId>* observed_children =
      observed == nullptr ? nullptr : &observed->child_node_ids;
  const std::vector<ExactDirectK1AtomicBatch>* observed_batches =
      observed == nullptr ? nullptr : &observed->batches;

  CanonicalDisjointSet components{cloud.size()};
  components.initialize_component_count(cloud.size());
  std::size_t binding_index = 0U;
  std::size_t saddle_record_index = 0U;
  std::size_t atomic_group_index = 0U;
  std::size_t child_index = 0U;
  std::size_t batch_index = 0U;
  std::size_t created_node_count = 0U;
  std::size_t continuation_group_count = 0U;
  std::size_t multifusion_count = 0U;
  std::size_t maximum_merge_arity = 0U;

  std::size_t family_begin = 0U;
  while (family_begin < source_seed_journal.families.size() &&
         source_seed_journal.families[family_begin].order == 1U) {
    const std::size_t source_batch_index =
        source_seed_journal.families[family_begin].journal_batch_index;
    std::size_t family_end = family_begin;
    while (family_end < source_seed_journal.families.size() &&
           source_seed_journal.families[family_end].order == 1U &&
           source_seed_journal.families[family_end].journal_batch_index ==
               source_batch_index) {
      ++family_end;
    }
    if (source_batch_index >= source_journal.batches.size()) {
      clear_payload(result);
      result.decision = ExactDirectK1ForestDecision::
          no_k1_rank_two_source_inconsistent;
      return outcome;
    }
    const ExactDirectMorseH0Batch& source_batch =
        source_journal.batches[source_batch_index];
    if (source_batch.order != 1U ||
        source_batch.squared_level == exact::ExactLevel{}) {
      clear_payload(result);
      result.decision = ExactDirectK1ForestDecision::
          no_k1_rank_two_source_inconsistent;
      return outcome;
    }

    const std::size_t strict_root_count = components.component_count();
    std::vector<TemporarySaddle> temporary_saddles;
    temporary_saddles.reserve(family_end - family_begin);
    std::vector<std::size_t> touched_roots;
    touched_roots.reserve(2U * (family_end - family_begin));
    try {
      for (std::size_t family_index = family_begin;
           family_index < family_end;
           ++family_index) {
        const ExactDirectSaddleArmFamilyRecord& family =
            source_seed_journal.families[family_index];
        if (family.family_index != family_index ||
            family.order != 1U || family.arm_seed_count != 2U ||
            family.critical_squared_level != source_batch.squared_level ||
            family.source_event_index >= source_facade.events.size() ||
            family.arm_seed_offset > source_seed_journal.arm_seeds.size() ||
            family.arm_seed_count >
                source_seed_journal.arm_seeds.size() -
                    family.arm_seed_offset) {
          throw std::logic_error("an order-one family is inconsistent");
        }
        const ExactDirectSupportEvent& event =
            source_facade.events[family.source_event_index];
        if (event.closed_rank != 2U || event.support_size != 2U ||
            !event.interior_ids.empty() ||
            event.saddle_order != std::optional<std::size_t>{1U}) {
          throw std::logic_error("an order-one saddle is not rank two");
        }

        TemporarySaddle saddle;
        saddle.family_index = family_index;
        for (std::size_t local = 0U; local < 2U; ++local) {
          const std::size_t seed_index = family.arm_seed_offset + local;
          const ExactDirectSaddleArmFacet facet =
              reconstruct_exact_direct_saddle_arm_facet(
                  source_facade, source_seed_journal, seed_index);
          if (facet.point_count != 1U ||
              facet.point_ids[0U] >= cloud.size()) {
            throw std::logic_error("an order-one arm is not a singleton");
          }
          saddle.seed_indices[local] = seed_index;
          saddle.singleton_point_ids[local] = facet.point_ids[0U];
          saddle.pre_batch_component_roots[local] = components.find(
              static_cast<std::size_t>(facet.point_ids[0U]));
          saddle.pre_batch_root_node_ids[local] =
              components.component_node_id(
                  saddle.pre_batch_component_roots[local]);
          touched_roots.push_back(
              saddle.pre_batch_component_roots[local]);
        }
        temporary_saddles.push_back(saddle);
      }
    } catch (const std::logic_error&) {
      clear_payload(result);
      result.decision = ExactDirectK1ForestDecision::
          no_k1_rank_two_source_inconsistent;
      return outcome;
    }

    std::sort(touched_roots.begin(), touched_roots.end());
    touched_roots.erase(
        std::unique(touched_roots.begin(), touched_roots.end()),
        touched_roots.end());
    LocalDisjointSet quotient{touched_roots.size()};
    for (const TemporarySaddle& saddle : temporary_saddles) {
      const auto left = std::lower_bound(
          touched_roots.begin(),
          touched_roots.end(),
          saddle.pre_batch_component_roots[0U]);
      const auto right = std::lower_bound(
          touched_roots.begin(),
          touched_roots.end(),
          saddle.pre_batch_component_roots[1U]);
      if (left == touched_roots.end() || right == touched_roots.end() ||
          *left != saddle.pre_batch_component_roots[0U] ||
          *right != saddle.pre_batch_component_roots[1U]) {
        clear_payload(result);
        result.decision = ExactDirectK1ForestDecision::
            no_k1_quotient_did_not_close;
        return outcome;
      }
      quotient.unite(
          static_cast<std::size_t>(left - touched_roots.begin()),
          static_cast<std::size_t>(right - touched_roots.begin()));
    }

    std::vector<TemporaryGroup> temporary_groups;
    std::vector<std::size_t> group_by_local_root(
        touched_roots.size(), std::numeric_limits<std::size_t>::max());
    for (std::size_t local = 0U; local < touched_roots.size(); ++local) {
      const std::size_t quotient_root = quotient.find(local);
      if (group_by_local_root[quotient_root] ==
          std::numeric_limits<std::size_t>::max()) {
        group_by_local_root[quotient_root] = temporary_groups.size();
        temporary_groups.emplace_back();
      }
      temporary_groups[group_by_local_root[quotient_root]]
          .component_roots.push_back(touched_roots[local]);
    }
    for (std::size_t local_saddle = 0U;
         local_saddle < temporary_saddles.size();
         ++local_saddle) {
      const auto root_position = std::lower_bound(
          touched_roots.begin(),
          touched_roots.end(),
          temporary_saddles[local_saddle]
              .pre_batch_component_roots[0U]);
      if (root_position == touched_roots.end()) {
        clear_payload(result);
        result.decision = ExactDirectK1ForestDecision::
            no_k1_quotient_did_not_close;
        return outcome;
      }
      const std::size_t quotient_root = quotient.find(
          static_cast<std::size_t>(
              root_position - touched_roots.begin()));
      const std::size_t group = group_by_local_root[quotient_root];
      if (group >= temporary_groups.size()) {
        clear_payload(result);
        result.decision = ExactDirectK1ForestDecision::
            no_k1_quotient_did_not_close;
        return outcome;
      }
      temporary_groups[group].saddle_indices.push_back(local_saddle);
    }

    const std::size_t batch_saddle_offset = saddle_record_index;
    const std::size_t batch_group_offset = atomic_group_index;
    for (const TemporaryGroup& group : temporary_groups) {
      if (group.component_roots.empty() || group.saddle_indices.empty()) {
        clear_payload(result);
        result.decision = ExactDirectK1ForestDecision::
            no_k1_quotient_did_not_close;
        return outcome;
      }
      const std::size_t group_saddle_offset = saddle_record_index;
      const std::size_t group_index = atomic_group_index;
      for (const std::size_t local_saddle_index : group.saddle_indices) {
        const TemporarySaddle& saddle =
            temporary_saddles[local_saddle_index];
        const ExactDirectSaddleArmFamilyRecord& family =
            source_seed_journal.families[saddle.family_index];
        const std::size_t saddle_binding_offset = binding_index;
        for (std::size_t local = 0U; local < 2U; ++local) {
          const ExactDirectK1ArmRootBinding binding{
              binding_index,
              saddle.seed_indices[local],
              saddle.family_index,
              saddle.singleton_point_ids[local],
              saddle.pre_batch_root_node_ids[local]};
          emit_record(
              result.arm_root_bindings,
              observed_bindings,
              binding,
              binding_index,
              retain_payload,
              outcome.arm_root_bindings_match);
          ++binding_index;
        }
        const std::size_t distinct_root_count =
            saddle.pre_batch_component_roots[0U] ==
                    saddle.pre_batch_component_roots[1U]
                ? 1U
                : 2U;
        const ExactDirectK1SaddleRecord record{
            saddle_record_index,
            saddle.family_index,
            family.source_event_index,
            source_batch_index,
            saddle_binding_offset,
            2U,
            distinct_root_count,
            group_index};
        emit_record(
            result.saddle_records,
            observed_saddles,
            record,
            saddle_record_index,
            retain_payload,
            outcome.saddle_records_match);
        ++saddle_record_index;
      }

      ExactDirectK1AtomicGroup group_record;
      group_record.atomic_group_index = atomic_group_index;
      group_record.batch_index = batch_index;
      group_record.saddle_record_offset = group_saddle_offset;
      group_record.saddle_record_count =
          saddle_record_index - group_saddle_offset;
      group_record.child_offset = child_index;
      const std::size_t arity = group.component_roots.size();
      maximum_merge_arity = std::max(maximum_merge_arity, arity);
      if (arity == 1U) {
        ++continuation_group_count;
        group_record.child_count = 0U;
        group_record.created_node_id = std::nullopt;
        group_record.resulting_root_node_id =
            components.component_node_id(group.component_roots.front());
      } else {
        group_record.child_count = arity;
        const ExactDirectK1NodeId created_node_id =
            static_cast<ExactDirectK1NodeId>(
                cloud.size() + created_node_count);
        group_record.created_node_id = created_node_id;
        group_record.resulting_root_node_id = created_node_id;
        if (arity >= 3U) {
          ++multifusion_count;
        }
        for (const std::size_t component_root : group.component_roots) {
          emit_child(
              result.child_node_ids,
              observed_children,
              components.component_node_id(component_root),
              child_index,
              retain_payload,
              outcome.child_references_match);
          ++child_index;
        }
        std::size_t resulting_component = group.component_roots.front();
        for (std::size_t local = 1U;
             local < group.component_roots.size();
             ++local) {
          resulting_component = components.unite(
              resulting_component, group.component_roots[local]);
        }
        components.set_component_node_id(
            resulting_component, created_node_id);
        ++created_node_count;
      }
      emit_record(
          result.atomic_groups,
          observed_groups,
          group_record,
          atomic_group_index,
          retain_payload,
          outcome.atomic_groups_match);
      ++atomic_group_index;
    }

    const ExactDirectK1AtomicBatch batch{
        batch_index,
        source_batch_index,
        source_batch.squared_level,
        batch_saddle_offset,
        saddle_record_index - batch_saddle_offset,
        batch_group_offset,
        atomic_group_index - batch_group_offset,
        strict_root_count,
        components.component_count(),
        true,
        true};
    emit_record(
        result.batches,
        observed_batches,
        batch,
        batch_index,
        retain_payload,
        outcome.batches_match);
    ++batch_index;
    family_begin = family_end;
  }

  if (family_begin != order_one_family_count ||
      binding_index != *binding_count ||
      saddle_record_index != order_one_family_count ||
      batch_index != order_one_batch_count ||
      atomic_group_index > order_one_family_count ||
      child_index > *child_capacity || components.component_count() != 1U) {
    clear_payload(result);
    result.decision =
        ExactDirectK1ForestDecision::no_k1_quotient_did_not_close;
    return outcome;
  }

  if (observed != nullptr) {
    outcome.arm_root_bindings_match =
        outcome.arm_root_bindings_match &&
        observed->arm_root_bindings.size() == binding_index;
    outcome.saddle_records_match =
        outcome.saddle_records_match &&
        observed->saddle_records.size() == saddle_record_index;
    outcome.atomic_groups_match =
        outcome.atomic_groups_match &&
        observed->atomic_groups.size() == atomic_group_index;
    outcome.child_references_match =
        outcome.child_references_match &&
        observed->child_node_ids.size() == child_index;
    outcome.batches_match =
        outcome.batches_match && observed->batches.size() == batch_index;
  }

  const auto first_root = components.find(0U);
  result.root_node_id = components.component_node_id(first_root);
  result.node_count = cloud.size() + created_node_count;
  result.counters = ExactDirectK1ForestCounters{
      order_one_family_count,
      binding_index,
      saddle_record_index,
      atomic_group_index,
      continuation_group_count,
      created_node_count,
      multifusion_count,
      child_index,
      batch_index,
      maximum_batch_saddle_count,
      maximum_merge_arity};
  result.logical_added_storage_entry_count =
      binding_index + saddle_record_index + atomic_group_index +
      child_index + batch_index;
  const auto combined_count = checked_add(
      source_seed_journal.combined_logical_storage_entry_count,
      result.logical_added_storage_entry_count);
  if (!combined_count.has_value()) {
    clear_payload(result);
    result.decision =
        ExactDirectK1ForestDecision::no_k1_capacity_overflow;
    return outcome;
  }
  result.combined_logical_storage_entry_count = *combined_count;

  result.every_order_one_saddle_is_rank_two = true;
  result.every_order_one_arm_is_singleton = true;
  result.every_arm_bound_to_strict_pre_batch_root = true;
  result.equal_level_quotients_resolved_before_mutation = true;
  result.continuation_groups_preserved_without_new_node = true;
  result.closed_diameter_strict_edge_descent_theorem_applies = true;
  result.exact_order_one_forest_reduction_performed = true;
  result.output_linear_in_points_and_order_one_saddles =
      result.logical_added_storage_entry_count <=
          result.logical_added_storage_entry_limit &&
      result.combined_logical_storage_entry_count <=
          result.combined_logical_storage_entry_limit;
  result.no_forbidden_global_structure_materialized = true;
  result.forbidden_global_geometry_computed = false;
  result.higher_order_root_localization_claimed = false;
  result.public_status_claimed = false;
  result.partial_refinement_only = true;
  result.decision = ExactDirectK1ForestDecision::
      complete_certified_direct_k1_forest_journal;
  return outcome;
}

[[nodiscard]] bool same_scalar_facts(
    const ExactDirectK1ForestJournalResult& observed,
    const ExactDirectK1ForestJournalResult& expected) {
  return observed.schema_version == expected.schema_version &&
         observed.requested_budget == expected.requested_budget &&
         observed.point_count == expected.point_count &&
         observed.source_direct_event_count ==
             expected.source_direct_event_count &&
         observed.required_source_replay_entry_count ==
             expected.required_source_replay_entry_count &&
         observed.required_point_scratch_entry_count ==
             expected.required_point_scratch_entry_count &&
         observed.required_equal_level_scratch_entry_count ==
             expected.required_equal_level_scratch_entry_count &&
         observed.required_arm_root_binding_count ==
             expected.required_arm_root_binding_count &&
         observed.required_saddle_record_count ==
             expected.required_saddle_record_count &&
         observed.required_atomic_group_capacity ==
             expected.required_atomic_group_capacity &&
         observed.required_child_reference_capacity ==
             expected.required_child_reference_capacity &&
         observed.required_batch_record_capacity ==
             expected.required_batch_record_capacity &&
         observed.logical_added_storage_entry_count ==
             expected.logical_added_storage_entry_count &&
         observed.logical_added_storage_entry_limit ==
             expected.logical_added_storage_entry_limit &&
         observed.combined_logical_storage_entry_count ==
             expected.combined_logical_storage_entry_count &&
         observed.combined_logical_storage_entry_limit ==
             expected.combined_logical_storage_entry_limit &&
         observed.source_pair_canonical_cloud_digest ==
             expected.source_pair_canonical_cloud_digest &&
         observed.source_higher_canonical_cloud_digest ==
             expected.source_higher_canonical_cloud_digest &&
         observed.source_pair_semantic_digest ==
             expected.source_pair_semantic_digest &&
         observed.source_higher_semantic_digest ==
             expected.source_higher_semantic_digest &&
         observed.root_node_id == expected.root_node_id &&
         observed.node_count == expected.node_count &&
         observed.budget_preflight_certified ==
             expected.budget_preflight_certified &&
         observed.source_seed_journal_streaming_replayed ==
             expected.source_seed_journal_streaming_replayed &&
         observed.every_order_one_saddle_is_rank_two ==
             expected.every_order_one_saddle_is_rank_two &&
         observed.every_order_one_arm_is_singleton ==
             expected.every_order_one_arm_is_singleton &&
         observed.every_arm_bound_to_strict_pre_batch_root ==
             expected.every_arm_bound_to_strict_pre_batch_root &&
         observed.equal_level_quotients_resolved_before_mutation ==
             expected.equal_level_quotients_resolved_before_mutation &&
         observed.continuation_groups_preserved_without_new_node ==
             expected.continuation_groups_preserved_without_new_node &&
         observed.closed_diameter_strict_edge_descent_theorem_applies ==
             expected.closed_diameter_strict_edge_descent_theorem_applies &&
         observed.exact_order_one_forest_reduction_performed ==
             expected.exact_order_one_forest_reduction_performed &&
         observed.output_linear_in_points_and_order_one_saddles ==
             expected.output_linear_in_points_and_order_one_saddles &&
         observed.no_forbidden_global_structure_materialized ==
             expected.no_forbidden_global_structure_materialized &&
         observed.forbidden_global_geometry_computed ==
             expected.forbidden_global_geometry_computed &&
         observed.higher_order_root_localization_claimed ==
             expected.higher_order_root_localization_claimed &&
         observed.public_status_claimed == expected.public_status_claimed &&
         observed.partial_refinement_only == expected.partial_refinement_only;
}

}  // namespace

bool ExactDirectK1ForestJournalResult::certified_order_one_forest() const {
  return schema_version == direct_k1_forest_journal_schema_version &&
         required_arm_root_binding_count == arm_root_bindings.size() &&
         required_saddle_record_count == saddle_records.size() &&
         atomic_groups.size() <= required_atomic_group_capacity &&
         child_node_ids.size() <= required_child_reference_capacity &&
         required_batch_record_capacity == batches.size() &&
         counters.arm_root_binding_count == arm_root_bindings.size() &&
         counters.saddle_record_count == saddle_records.size() &&
         counters.atomic_group_count == atomic_groups.size() &&
         counters.child_reference_count == child_node_ids.size() &&
         counters.batch_record_count == batches.size() &&
         logical_added_storage_entry_count ==
             arm_root_bindings.size() + saddle_records.size() +
                 atomic_groups.size() + child_node_ids.size() +
                 batches.size() &&
         logical_added_storage_entry_count <=
             logical_added_storage_entry_limit &&
         combined_logical_storage_entry_count <=
             combined_logical_storage_entry_limit &&
         node_count == point_count + counters.created_node_count &&
         root_node_id < node_count && budget_preflight_certified &&
         source_seed_journal_streaming_replayed &&
         every_order_one_saddle_is_rank_two &&
         every_order_one_arm_is_singleton &&
         every_arm_bound_to_strict_pre_batch_root &&
         equal_level_quotients_resolved_before_mutation &&
         continuation_groups_preserved_without_new_node &&
         closed_diameter_strict_edge_descent_theorem_applies &&
         exact_order_one_forest_reduction_performed &&
         output_linear_in_points_and_order_one_saddles &&
         no_forbidden_global_structure_materialized &&
         !forbidden_global_geometry_computed &&
         !higher_order_root_localization_claimed && !public_status_claimed &&
         partial_refinement_only &&
         decision == ExactDirectK1ForestDecision::
                         complete_certified_direct_k1_forest_journal &&
         scope == ExactDirectK1ForestScope::
                      exact_order_one_forest_and_atomic_continuations_only;
}

ExactDirectK1ForestJournalResult build_exact_direct_k1_forest_journal(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectK1ForestBudget& budget) {
  return replay_direct_k1_forest(
             cloud,
             source_facade,
             source_journal,
             trusted_seed_budget,
             source_seed_journal,
             budget,
             nullptr,
             true)
      .result;
}

ExactDirectK1ForestStreamingVerification
verify_exact_direct_k1_forest_journal_streaming(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal,
    const ExactDirectK1ForestBudget& trusted_budget,
    const ExactDirectK1ForestJournalResult& observed) {
  ExactDirectK1ForestStreamingVerification verification;
  const ReplayOutcome replay = replay_direct_k1_forest(
      cloud,
      source_facade,
      source_journal,
      trusted_seed_budget,
      source_seed_journal,
      trusted_budget,
      &observed,
      false);
  verification.source_seed_journal_certified =
      replay.source_seed_journal_certified;
  verification.requirements_certified =
      observed.schema_version == replay.result.schema_version &&
      observed.requested_budget == replay.result.requested_budget &&
      observed.point_count == replay.result.point_count &&
      observed.source_direct_event_count ==
          replay.result.source_direct_event_count &&
      observed.required_source_replay_entry_count ==
          replay.result.required_source_replay_entry_count &&
      observed.required_point_scratch_entry_count ==
          replay.result.required_point_scratch_entry_count &&
      observed.required_equal_level_scratch_entry_count ==
          replay.result.required_equal_level_scratch_entry_count &&
      observed.required_arm_root_binding_count ==
          replay.result.required_arm_root_binding_count &&
      observed.required_saddle_record_count ==
          replay.result.required_saddle_record_count &&
      observed.required_atomic_group_capacity ==
          replay.result.required_atomic_group_capacity &&
      observed.required_child_reference_capacity ==
          replay.result.required_child_reference_capacity &&
      observed.required_batch_record_capacity ==
          replay.result.required_batch_record_capacity;
  const bool complete_replay =
      replay.result.decision == ExactDirectK1ForestDecision::
                                    complete_certified_direct_k1_forest_journal;
  verification.arm_root_bindings_certified =
      complete_replay && replay.arm_root_bindings_match;
  verification.saddle_records_certified =
      complete_replay && replay.saddle_records_match;
  verification.atomic_groups_certified =
      complete_replay && replay.atomic_groups_match;
  verification.child_references_certified =
      complete_replay && replay.child_references_match;
  verification.batches_certified =
      complete_replay && replay.batches_match;
  verification.result_facts_certified =
      same_scalar_facts(observed, replay.result);
  verification.counters_certified =
      observed.counters == replay.result.counters;
  verification.decision_and_scope_certified =
      observed.decision == replay.result.decision &&
      observed.scope == replay.result.scope;
  verification.no_second_persistent_output_arena_certified = true;
  verification.fresh_streaming_replay_certified =
      verification.requirements_certified &&
      verification.arm_root_bindings_certified &&
      verification.saddle_records_certified &&
      verification.atomic_groups_certified &&
      verification.child_references_certified &&
      verification.batches_certified &&
      verification.result_facts_certified &&
      verification.counters_certified &&
      verification.decision_and_scope_certified;
  verification.result_certified =
      verification.source_seed_journal_certified &&
      verification.fresh_streaming_replay_certified &&
      observed.certified_order_one_forest();
  return verification;
}

}  // namespace morsehgp3d::hierarchy
