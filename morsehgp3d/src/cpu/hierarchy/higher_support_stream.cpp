#include "morsehgp3d/hierarchy/higher_support_stream.hpp"

#include "morsehgp3d/exact/support.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

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
      ExactHigherSupportStreamBudget budget)
      : index_(index), cloud_(cloud) {
    validate_inputs(requested_maximum_order);
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
  }

  void execute() {
    while (!frontier_.empty() && !stopped_) {
      visit_frontier_back();
    }
    finish_result();
  }

  [[nodiscard]] ExactHigherSupportStreamResult take_result() {
    return std::move(result_);
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

  [[nodiscard]] bool consume_work_unit() {
    if (result_.audit.work_unit_count >=
        result_.budget.maximum_work_unit_count) {
      stop(ExactHigherSupportStopReason::work_unit_limit);
      return false;
    }
    increment(
        result_.audit.work_unit_count,
        "the higher-support work count overflows size_t");
    return true;
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

  [[nodiscard]] bool node_range_inside_support_domain(
      const Node& query,
      const ExactHigherSupportFrontierEntry& entry) const {
    for (std::size_t index = 0U; index < entry.group_count; ++index) {
      const Node& support = node(group_node_index(entry.groups[index]));
      if (support.leaf_begin <= query.leaf_begin &&
          query.leaf_end <= support.leaf_end) {
        return true;
      }
    }
    return false;
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

  [[nodiscard]] RankSearchResult search_rank_prune(
      const ExactHigherSupportFrontierEntry& entry,
      const std::array<spatial::ExactDyadicAabb3, 4>& boxes) {
    RankSearchResult result;
    const std::size_t required =
        required_strict_interior_count(entry.support_size);
    if (required == 0U) {
      result.outcome = RankSearchOutcome::prune;
      return result;
    }
    if (possible_external_witness_count(entry) < required) {
      return result;
    }
    if (!auxiliary_frontier_preflight()) {
      result.outcome = RankSearchOutcome::budget_exhausted;
      return result;
    }
    increment(
        result_.audit.rank_search_count,
        "the higher-support rank-search count overflows size_t");
    std::vector<std::size_t> frontier{index_.root_index_};
    result_.audit.maximum_rank_frontier_entry_count = std::max(
        result_.audit.maximum_rank_frontier_entry_count,
        frontier.size());
    const std::span<const spatial::ExactDyadicAabb3> support_box_span{
        boxes.data(), entry.support_size};
    while (!frontier.empty()) {
      if (!consume_work_unit()) {
        result.outcome = RankSearchOutcome::budget_exhausted;
        return result;
      }
      const std::size_t query_node_index = frontier.back();
      frontier.pop_back();
      const Node& query = node(query_node_index);
      increment(
          result_.audit.rank_witness_node_visit_count,
          "the higher-support rank witness count overflows size_t");

      if (node_range_inside_support_domain(query, entry)) {
        continue;
      }
      if (node_range_intersects_support_domain(query, entry)) {
        if (query.is_leaf()) {
          throw std::logic_error(
              "a leaf partially overlaps a higher-support Morton domain");
        }
        frontier.push_back(query.right_child);
        frontier.push_back(query.left_child);
        if (frontier.size() >
            result_.budget.maximum_auxiliary_frontier_entry_count) {
          throw std::logic_error(
              "a higher-support rank DFS exceeded its preflight bound");
        }
        result_.audit.maximum_rank_frontier_entry_count = std::max(
            result_.audit.maximum_rank_frontier_entry_count,
            frontier.size());
        continue;
      }

      ExactHigherSupportProductAabbAnalysis analysis =
          exact_higher_support_product_aabb_analysis(
              support_box_span, node_box(query_node_index));
      increment(
          result_.audit.exact_product_analysis_count,
          "the higher-support product-analysis count overflows size_t");
      if (analysis
              .query_strictly_inside_every_independent_sphere_certified()) {
        const std::size_t point_count =
            query.leaf_end - query.leaf_begin;
        result.certified_point_count += point_count;
        result.receipts.push_back(ExactHigherSupportRankReceipt{
            make_node_receipt(query_node_index),
            point_count,
            std::move(analysis)});
        if (result.certified_point_count >= exact::BigInt{required}) {
          result.outcome = RankSearchOutcome::prune;
          return result;
        }
        continue;
      }
      if (!query.is_leaf()) {
        frontier.push_back(query.right_child);
        frontier.push_back(query.left_child);
        if (frontier.size() >
            result_.budget.maximum_auxiliary_frontier_entry_count) {
          throw std::logic_error(
              "a higher-support rank DFS exceeded its preflight bound");
        }
        result_.audit.maximum_rank_frontier_entry_count = std::max(
            result_.audit.maximum_rank_frontier_entry_count,
            frontier.size());
      }
    }
    return result;
  }

  [[nodiscard]] bool emit_prune_certificate(
      const ExactHigherSupportFrontierEntry& entry,
      ExactHigherSupportPruneReason reason,
      ExactHigherSupportProductAabbAnalysis support_analysis,
      RankSearchResult rank_result) {
    if (result_.audit.emitted_record_count >=
        result_.budget.maximum_emitted_record_count) {
      stop(ExactHigherSupportStopReason::emitted_record_limit);
      return false;
    }
    if (!can_add_within(
            result_.audit.emitted_rank_receipt_count,
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
    frontier_.pop_back();
    return true;
  }

  void expand_product(const ExactHigherSupportFrontierEntry& entry) {
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
      return;
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
  }

  [[nodiscard]] bool leaf_query_preflight(std::size_t support_size) {
    if (result_.audit.global_closed_ball_query_count >=
        result_.budget.maximum_global_closed_ball_query_count) {
      stop(ExactHigherSupportStopReason::global_closed_ball_query_limit);
      return false;
    }
    if (!can_add_within(
            result_.audit.point_classification_count,
            cloud_.size(),
            result_.budget.maximum_point_classification_count)) {
      stop(ExactHigherSupportStopReason::point_classification_limit);
      return false;
    }
    if (result_.audit.emitted_record_count >=
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
    if (!can_add_within(
            result_.audit.emitted_point_id_reference_count,
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
    if (result_.audit.point_classification_count >
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
  void classify_terminal_support(
      const ExactHigherSupportFrontierEntry& entry,
      const std::array<PointId, 4>& support_ids) {
    std::array<exact::ExactRational3, SupportSize> support_points{};
    for (std::size_t index = 0U; index < SupportSize; ++index) {
      support_points[index] = cloud_.point(support_ids[index]).exact();
    }
    const exact::CircumcenterSupportAnalysis analysis =
        exact::analyze_circumcenter_support(support_points);
    increment(
        result_.audit.leaf_support_analysis_count,
        "the higher-support leaf-analysis count overflows size_t");
    switch (analysis.status()) {
      case exact::CircumcenterSupportStatus::affinely_dependent:
        increment(
            result_.audit.affinely_dependent_leaf_count,
            "the higher-support dependent-leaf count overflows size_t");
        resolve_leaf();
        return;
      case exact::CircumcenterSupportStatus::boundary_reduced:
        increment(
            result_.audit.boundary_reduced_leaf_count,
            "the higher-support boundary-leaf count overflows size_t");
        resolve_leaf();
        return;
      case exact::CircumcenterSupportStatus::exterior_circumcenter:
        increment(
            result_.audit.exterior_circumcenter_leaf_count,
            "the higher-support exterior-centre count overflows size_t");
        resolve_leaf();
        return;
      case exact::CircumcenterSupportStatus::minimal:
        increment(
            result_.audit.minimal_leaf_count,
            "the higher-support minimal-leaf count overflows size_t");
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
      return;
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
      return;
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
      increment(
          result_.audit.relevant_extra_shell_diagnostic_count,
          "the higher-support diagnostic count overflows size_t");
    }
    if (!can_add_within(
            result_.audit.emitted_point_id_reference_count,
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
  }

  void classify_terminal(
      const ExactHigherSupportFrontierEntry& entry) {
    const std::array<PointId, 4> support_ids =
        terminal_support_ids(entry);
    if (entry.support_size == 3U) {
      classify_terminal_support<3U>(entry, support_ids);
    } else if (entry.support_size == 4U) {
      classify_terminal_support<4U>(entry, support_ids);
    } else {
      throw std::logic_error(
          "a terminal higher-support product has an invalid arity");
    }
  }

  void visit_frontier_back() {
    if (!consume_work_unit()) {
      return;
    }
    const ExactHigherSupportFrontierEntry entry = frontier_.back();
    increment(
        result_.audit.support_product_visit_count,
        "the higher-support product-visit count overflows size_t");
    if (is_terminal(entry)) {
      classify_terminal(entry);
      return;
    }

    const std::array<spatial::ExactDyadicAabb3, 4> boxes =
        support_boxes(entry);
    const std::span<const spatial::ExactDyadicAabb3> support_box_span{
        boxes.data(), entry.support_size};
    ExactHigherSupportProductAabbAnalysis support_analysis =
        exact_higher_support_product_aabb_analysis(support_box_span);
    increment(
        result_.audit.exact_product_analysis_count,
        "the higher-support product-analysis count overflows size_t");
    if (support_analysis.no_well_centered_support_certified()) {
      static_cast<void>(emit_prune_certificate(
          entry,
          ExactHigherSupportPruneReason::no_well_centered_support,
          std::move(support_analysis),
          RankSearchResult{}));
      return;
    }

    RankSearchResult rank_result = search_rank_prune(entry, boxes);
    if (rank_result.outcome == RankSearchOutcome::budget_exhausted) {
      return;
    }
    if (rank_result.outcome == RankSearchOutcome::prune) {
      static_cast<void>(emit_prune_certificate(
          entry,
          ExactHigherSupportPruneReason::strict_interior_rank_bound,
          std::move(support_analysis),
          std::move(rank_result)));
      return;
    }
    expand_product(entry);
  }

  void finish_result() {
    std::sort(
        result_.events.begin(),
        result_.events.end(),
        support_record_less<ExactHigherSupportEvent>);
    std::sort(
        result_.relevant_extra_shell_diagnostics.begin(),
        result_.relevant_extra_shell_diagnostics.end(),
        support_record_less<ExactHigherSupportExtraShellDiagnostic>);
    result_.remaining_frontier = frontier_;
    result_.audit.remaining_frontier_support_count = 0;
    for (const ExactHigherSupportFrontierEntry& entry : frontier_) {
      result_.audit.remaining_frontier_support_count +=
          entry_support_count(entry);
    }
    const exact::BigInt terminal_sum =
        result_.audit.well_centering_pruned_support_count +
        result_.audit.rank_pruned_support_count +
        result_.audit.leaf_classified_support_count;
    result_.audit.grouped_partition_accounting_certified =
        result_.audit.resolved_support_count +
                    result_.audit.remaining_frontier_support_count ==
                result_.audit.total_support_count &&
        result_.audit.resolved_support_count == terminal_sum;
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
    if (typed_record_count != result_.audit.emitted_record_count ||
        result_.prune_certificates.size() !=
            result_.audit.emitted_prune_certificate_count) {
      throw std::logic_error(
          "the higher-support emitted-record audit is inconsistent");
    }
    result_.grouped_frontier_partition_certified = true;
    result_.all_prunes_replayable = true;
    result_.all_rank_relevant_shells_complete = true;
    result_.frontier_exhausted = frontier_.empty();
    result_.no_forbidden_global_structure_materialized = true;
    result_.hierarchy_reduction_performed = false;
    if (frontier_.empty()) {
      result_.status = ExactHigherSupportStreamStatus::complete;
      result_.stop_reason = ExactHigherSupportStopReason::none;
    } else if (!stopped_) {
      throw std::logic_error(
          "a nonempty higher-support frontier has no budget stop reason");
    }
  }

  const spatial::MortonLbvhIndex& index_;
  const spatial::CanonicalPointCloud& cloud_;
  ExactHigherSupportStreamResult result_;
  std::vector<ExactHigherSupportFrontierEntry> frontier_;
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
