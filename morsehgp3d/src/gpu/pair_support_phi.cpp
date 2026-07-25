#include "morsehgp3d/gpu/pair_support_phi.hpp"

#include "../cuda/phase9_pair_support_phi_internal.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);
constexpr std::uint64_t kFnvOffsetBasis = UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value, const char* message) {
  if (value > static_cast<std::size_t>(
                  std::numeric_limits<std::uint64_t>::max())) {
    throw std::length_error(message);
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value, const char* message) {
  if (value > static_cast<std::uint64_t>(
                  std::numeric_limits<std::size_t>::max())) {
    throw std::runtime_error(message);
  }
  return static_cast<std::size_t>(value);
}

void validate_allocation_product(
    std::size_t count, std::size_t width, const char* message) {
  if (count != 0U &&
      width > std::numeric_limits<std::size_t>::max() / count) {
    throw std::length_error(message);
  }
}

[[nodiscard]] std::size_t checked_add_size(
    std::size_t left, std::size_t right, const char* message) {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    throw std::length_error(message);
  }
  return left + right;
}

void hash_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

[[nodiscard]] bool intervals_intersect(
    const detail::PairSupportPhiNodeInputRecord& left,
    const detail::PairSupportPhiNodeInputRecord& right) noexcept {
  return left.leaf_begin < right.leaf_end &&
         right.leaf_begin < left.leaf_end;
}

[[nodiscard]] spatial::ExactDyadicAabb3 node_box(
    const detail::PairSupportPhiNodeInputRecord& node) {
  return spatial::ExactDyadicAabb3{
      {node.lower_bits[0U], node.lower_bits[1U], node.lower_bits[2U]},
      {node.upper_bits[0U], node.upper_bits[1U], node.upper_bits[2U]}};
}

[[nodiscard]] bool sentinel_record(
    const detail::PairSupportPhiDeviceRecord& record) noexcept {
  return record.query_index == detail::pair_support_phi_sentinel &&
         record.first_support_node_index ==
             detail::pair_support_phi_sentinel &&
         record.second_support_node_index ==
             detail::pair_support_phi_sentinel &&
         record.witness_node_index == detail::pair_support_phi_sentinel &&
         record.upper_phi_bits == detail::pair_support_phi_sentinel &&
         record.proposal_code == detail::pair_support_phi_sentinel;
}

[[nodiscard]] bool sentinel_rank_receipt(
    const detail::PairSupportRankDeviceReceipt& receipt) noexcept {
  return receipt.receipt_index == detail::pair_support_phi_sentinel &&
         receipt.product_slot == detail::pair_support_phi_sentinel &&
         receipt.witness_node_index == detail::pair_support_phi_sentinel &&
         receipt.leaf_begin == detail::pair_support_phi_sentinel &&
         receipt.leaf_end == detail::pair_support_phi_sentinel &&
         receipt.upper_phi_bits == detail::pair_support_phi_sentinel;
}

[[nodiscard]] bool finite_canonical_binary64(
    std::uint64_t bits) noexcept {
  try {
    return exact::canonicalize_binary64_bits(bits) == bits;
  } catch (const std::exception&) {
    return false;
  }
}

[[nodiscard]] bool strictly_negative_binary64(std::uint64_t bits) noexcept {
  if (!finite_canonical_binary64(bits)) {
    return false;
  }
  return std::bit_cast<double>(bits) < 0.0;
}

[[nodiscard]] auto query_key(const PairSupportPhiWitnessQuery& query) {
  return std::tuple{
      query.first_support_node_index,
      query.second_support_node_index,
      query.witness_node_index};
}

[[nodiscard]] auto rank_product_key(
    const hierarchy::ExactPairSupportFrontierEntry& product) {
  return std::tuple{
      product.first_leaf_begin,
      product.first_leaf_end,
      product.second_leaf_begin,
      product.second_leaf_end,
      product.first_node_index,
      product.second_node_index,
      product.self_product};
}

[[nodiscard]] std::vector<detail::PairSupportPhiQueryInputRecord>
validate_and_pack_queries(
    std::span<const PairSupportPhiWitnessQuery> queries,
    std::span<const detail::PairSupportPhiNodeInputRecord> nodes,
    std::size_t maximum_query_count) {
  if (queries.empty()) {
    throw std::invalid_argument(
        "a Phase 9 pair-support phi batch must be nonempty");
  }
  if (queries.size() > maximum_query_count) {
    throw std::invalid_argument(
        "a Phase 9 pair-support phi batch exceeds its fixed query capacity");
  }
  std::vector<detail::PairSupportPhiQueryInputRecord> packed;
  packed.reserve(queries.size());
  for (std::size_t query_index = 0U;
       query_index < queries.size();
       ++query_index) {
    const PairSupportPhiWitnessQuery& query = queries[query_index];
    if (query_index != 0U &&
        query_key(queries[query_index - 1U]) >= query_key(query)) {
      throw std::invalid_argument(
          "Phase 9 pair-support phi queries must be strictly canonical");
    }
    const std::size_t first = checked_size(
        query.first_support_node_index,
        "a Phase 9 first support node index does not fit size_t");
    const std::size_t second = checked_size(
        query.second_support_node_index,
        "a Phase 9 second support node index does not fit size_t");
    const std::size_t witness = checked_size(
        query.witness_node_index,
        "a Phase 9 witness node index does not fit size_t");
    if (first >= nodes.size() || second >= nodes.size() ||
        witness >= nodes.size()) {
      throw std::invalid_argument(
          "a Phase 9 pair-support phi query references a missing LBVH node");
    }
    const detail::PairSupportPhiNodeInputRecord& first_node = nodes[first];
    const detail::PairSupportPhiNodeInputRecord& second_node = nodes[second];
    const detail::PairSupportPhiNodeInputRecord& witness_node = nodes[witness];
    if (first_node.leaf_begin >= first_node.leaf_end ||
        second_node.leaf_begin >= second_node.leaf_end ||
        witness_node.leaf_begin >= witness_node.leaf_end ||
        first_node.leaf_end > second_node.leaf_begin) {
      throw std::invalid_argument(
          "a Phase 9 support-box pair is not canonically disjoint in Morton order");
    }
    if (intervals_intersect(first_node, witness_node) ||
        intervals_intersect(second_node, witness_node)) {
      throw std::invalid_argument(
          "a Phase 9 phi witness subtree intersects its support product");
    }
    packed.push_back(detail::PairSupportPhiQueryInputRecord{
        query.first_support_node_index,
        query.second_support_node_index,
        query.witness_node_index});
  }
  return packed;
}

[[nodiscard]] std::vector<detail::PairSupportRankProductInputRecord>
validate_and_pack_rank_products(
    std::span<const hierarchy::ExactPairSupportFrontierEntry> products,
    std::span<const detail::PairSupportPhiNodeInputRecord> nodes,
    PairSupportRankPruneCapacity capacity,
    std::size_t required_strict_interior_point_count,
    PairSupportRankPruneBudget budget) {
  if (products.empty()) {
    throw std::invalid_argument(
        "a Phase 9 rank-prune proposal batch must be nonempty");
  }
  if (capacity.maximum_product_count == 0U ||
      capacity.maximum_work_item_count == 0U ||
      capacity.maximum_receipt_count == 0U) {
    throw std::invalid_argument(
        "the Phase 9 rank-prune proposal context has no fixed P2 capacity");
  }
  if (products.size() > capacity.maximum_product_count ||
      products.size() > capacity.maximum_work_item_count) {
    throw std::invalid_argument(
        "a Phase 9 rank-prune proposal batch exceeds its initial frontier capacity");
  }
  if (required_strict_interior_point_count == 0U ||
      budget.maximum_epoch_count == 0U) {
    throw std::invalid_argument(
        "a Phase 9 rank-prune proposal needs positive rank and epoch bounds");
  }
  static_cast<void>(checked_u64(
      required_strict_interior_point_count,
      "the Phase 9 rank-prune threshold does not fit uint64"));
  static_cast<void>(checked_u64(
      capacity.maximum_work_item_count,
      "the Phase 9 rank-prune work capacity does not fit uint64"));
  static_cast<void>(checked_u64(
      capacity.maximum_receipt_count,
      "the Phase 9 rank-prune receipt capacity does not fit uint64"));

  std::vector<detail::PairSupportRankProductInputRecord> packed;
  packed.reserve(products.size());
  for (std::size_t product_index = 0U;
       product_index < products.size();
       ++product_index) {
    const hierarchy::ExactPairSupportFrontierEntry& product =
        products[product_index];
    if (product_index != 0U &&
        rank_product_key(products[product_index - 1U]) >=
            rank_product_key(product)) {
      throw std::invalid_argument(
          "Phase 9 rank-prune products must be strictly canonical");
    }
    const std::size_t first = checked_size(
        product.first_node_index,
        "a Phase 9 rank-prune first node index does not fit size_t");
    const std::size_t second = checked_size(
        product.second_node_index,
        "a Phase 9 rank-prune second node index does not fit size_t");
    if (first >= nodes.size() || second >= nodes.size()) {
      throw std::invalid_argument(
          "a Phase 9 rank-prune product references a missing LBVH node");
    }
    const detail::PairSupportPhiNodeInputRecord& first_node = nodes[first];
    const detail::PairSupportPhiNodeInputRecord& second_node = nodes[second];
    const bool self_product = product.self_product == 1U;
    if (product.self_product > 1U ||
        self_product != (first == second) ||
        product.first_leaf_begin != first_node.leaf_begin ||
        product.first_leaf_end != first_node.leaf_end ||
        product.second_leaf_begin != second_node.leaf_begin ||
        product.second_leaf_end != second_node.leaf_end ||
        (!self_product &&
         first_node.leaf_end > second_node.leaf_begin)) {
      throw std::invalid_argument(
          "a Phase 9 rank-prune product contradicts the resident LBVH");
    }
    packed.push_back(detail::PairSupportRankProductInputRecord{
        product.first_node_index, product.second_node_index});
  }
  return packed;
}

[[nodiscard]] PairSupportPhiBatchResult validate_and_recertify(
    const detail::PairSupportPhiDeviceBatch& batch,
    std::span<const PairSupportPhiWitnessQuery> queries,
    std::span<const detail::PairSupportPhiNodeInputRecord> nodes,
    std::size_t maximum_query_count,
    std::uint64_t previous_buffer_epoch) {
  if (batch.records.size() != maximum_query_count ||
      batch.record_count != queries.size() ||
      batch.kernel_launch_count != 1U || batch.buffer_epoch == 0U ||
      batch.buffer_epoch <= previous_buffer_epoch) {
    throw std::runtime_error(
        "the GPU pair-support phi batch returned invalid extent metadata");
  }
  for (std::size_t index = batch.record_count;
       index < batch.records.size();
       ++index) {
    if (!sentinel_record(batch.records[index])) {
      throw std::runtime_error(
          "the GPU pair-support phi batch exposed a stale tail record");
    }
  }

  PairSupportPhiBatchResult result;
  result.proposals.resize(queries.size());
  result.decisions.resize(queries.size());
  PairSupportPhiAudit& audit = result.audit;
  audit.resident_lbvh_node_count = nodes.size();
  audit.maximum_query_count = maximum_query_count;
  audit.canonical_query_count = queries.size();
  audit.gpu_output_record_count = batch.record_count;
  audit.gpu_kernel_launch_count = batch.kernel_launch_count;
  audit.buffer_epoch = batch.buffer_epoch;
  audit.immutable_lbvh_snapshot_validated = true;
  audit.canonical_query_order_validated = true;

  std::vector<unsigned char> seen(queries.size(), 0U);
  std::vector<detail::PairSupportPhiDeviceRecord> records_by_query(
      queries.size());
  for (std::size_t position = 0U; position < batch.record_count; ++position) {
    const detail::PairSupportPhiDeviceRecord& record =
        batch.records[position];
    const std::size_t query_index = checked_size(
        record.query_index,
        "a GPU pair-support phi query index does not fit size_t");
    if (query_index >= queries.size() || seen[query_index] != 0U) {
      throw std::runtime_error(
          "the GPU pair-support phi transcript is not a query permutation");
    }
    seen[query_index] = 1U;
    const PairSupportPhiWitnessQuery& query = queries[query_index];
    if (record.first_support_node_index !=
            query.first_support_node_index ||
        record.second_support_node_index !=
            query.second_support_node_index ||
        record.witness_node_index != query.witness_node_index) {
      throw std::runtime_error(
          "the GPU pair-support phi transcript changed a canonical query");
    }
    if (record.proposal_code ==
        detail::pair_support_phi_strict_interior_code) {
      if (!strictly_negative_binary64(record.upper_phi_bits)) {
        throw std::runtime_error(
            "a GPU strict-interior proposal lacks a finite negative upper bound");
      }
      ++audit.gpu_strict_interior_proposal_count;
    } else if (record.proposal_code ==
               detail::pair_support_phi_requires_descent_code) {
      if (record.upper_phi_bits != kPositiveInfinityBits &&
          (!finite_canonical_binary64(record.upper_phi_bits) ||
           strictly_negative_binary64(record.upper_phi_bits))) {
        throw std::runtime_error(
            "a GPU descent proposal carries an invalid phi upper bound");
      }
      ++audit.gpu_requires_descent_count;
    } else {
      throw std::runtime_error(
          "the GPU pair-support phi transcript has an invalid proposal code");
    }
    records_by_query[query_index] = record;
  }
  if (audit.gpu_strict_interior_proposal_count +
          audit.gpu_requires_descent_count !=
      queries.size()) {
    throw std::runtime_error(
        "the GPU pair-support phi proposal counters do not close");
  }
  audit.exhaustive_proposal_permutation_validated = true;

  std::uint64_t digest = kFnvOffsetBasis;
  for (std::size_t query_index = 0U;
       query_index < queries.size();
       ++query_index) {
    const detail::PairSupportPhiDeviceRecord& record =
        records_by_query[query_index];
    const PairSupportPhiWitnessQuery& query = queries[query_index];
    const bool strict = record.proposal_code ==
        detail::pair_support_phi_strict_interior_code;
    result.proposals[query_index] = PairSupportPhiProposalRecord{
        query,
        strict ? PairSupportPhiProposalKind::proposed_strict_interior
               : PairSupportPhiProposalKind::requires_descent,
        record.upper_phi_bits};
    result.decisions[query_index].query = query;

    hash_word(digest, static_cast<std::uint64_t>(query_index));
    hash_word(digest, query.first_support_node_index);
    hash_word(digest, query.second_support_node_index);
    hash_word(digest, query.witness_node_index);
    hash_word(digest, record.upper_phi_bits);
    hash_word(digest, record.proposal_code);

    if (!strict) {
      result.decisions[query_index].decision =
          PairSupportPhiDecision::descend;
      continue;
    }

    const std::size_t first = static_cast<std::size_t>(
        query.first_support_node_index);
    const std::size_t second = static_cast<std::size_t>(
        query.second_support_node_index);
    const std::size_t witness = static_cast<std::size_t>(
        query.witness_node_index);
    hierarchy::ExactDiametralPhiAabbMaximum exact_receipt =
        hierarchy::exact_diametral_phi_aabb_maximum(
            node_box(nodes[first]),
            node_box(nodes[second]),
            node_box(nodes[witness]));
    ++audit.cpu_exact_phi_recertification_count;
    const exact::ExactRational proposed_upper =
        exact::ExactRational::from_binary64_bits(record.upper_phi_bits);
    if (exact_receipt.maximum_phi > proposed_upper ||
        exact_receipt.maximum_phi.sign() >= 0) {
      throw std::runtime_error(
          "the GPU pair-support phi proposal failed exact CPU recertification");
    }
    const exact::ExactRational margin = -exact_receipt.maximum_phi;
    if (!audit.minimum_certified_strict_margin.has_value() ||
        margin < *audit.minimum_certified_strict_margin) {
      audit.minimum_certified_strict_margin = margin;
    }
    result.decisions[query_index].decision =
        PairSupportPhiDecision::certified_strict_interior;
    result.decisions[query_index].exact_receipt =
        PairSupportPhiCertifiedReceipt{
            hierarchy::ExactPairSupportWitnessNodeEntry{
                query.witness_node_index,
                nodes[witness].leaf_begin,
                nodes[witness].leaf_end},
            std::move(exact_receipt)};
    ++audit.certified_strict_interior_receipt_count;
  }
  if (audit.cpu_exact_phi_recertification_count !=
          audit.gpu_strict_interior_proposal_count ||
      audit.certified_strict_interior_receipt_count !=
          audit.gpu_strict_interior_proposal_count ||
      (audit.certified_strict_interior_receipt_count == 0U) !=
          !audit.minimum_certified_strict_margin.has_value()) {
    throw std::logic_error(
        "the exact pair-support phi receipt counters do not close");
  }
  audit.proposal_digest_fnv1a = digest;
  audit.cpu_exact_recertification_complete = true;
  audit.global_support_product_prune_published = false;
  audit.public_status_published = false;
  return result;
}

[[nodiscard]] PairSupportRankPruneBatchResult
validate_and_recertify_rank_prunes(
    const detail::PairSupportRankDeviceBatch& batch,
    std::span<const hierarchy::ExactPairSupportFrontierEntry> products,
    std::span<const detail::PairSupportPhiNodeInputRecord> nodes,
    PairSupportRankPruneCapacity capacity,
    std::size_t required_strict_interior_point_count,
    PairSupportRankPruneBudget budget,
    std::uint64_t previous_buffer_epoch) {
  validate_allocation_product(
      nodes.size(),
      sizeof(detail::PairSupportPhiNodeInputRecord),
      "the Phase 9 rank-prune snapshot byte count overflows size_t");
  validate_allocation_product(
      products.size(),
      sizeof(detail::PairSupportRankProductInputRecord),
      "the Phase 9 rank-prune product byte count overflows size_t");
  validate_allocation_product(
      products.size(),
      sizeof(detail::PairSupportRankWorkItem),
      "the Phase 9 rank-prune initial-frontier byte count overflows size_t");
  validate_allocation_product(
      capacity.maximum_product_count,
      sizeof(detail::PairSupportRankProductInputRecord) +
          sizeof(std::uint64_t),
      "the Phase 9 rank-prune product byte capacity overflows size_t");
  validate_allocation_product(
      capacity.maximum_work_item_count,
      2U * sizeof(detail::PairSupportRankWorkItem) +
          4U * sizeof(std::uint64_t) +
          2U * sizeof(std::uint64_t),
      "the Phase 9 rank-prune work byte capacity overflows size_t");
  validate_allocation_product(
      capacity.maximum_receipt_count,
      sizeof(detail::PairSupportRankDeviceReceipt),
      "the Phase 9 rank-prune receipt byte capacity overflows size_t");
  const std::size_t snapshot_bytes =
      nodes.size() * sizeof(detail::PairSupportPhiNodeInputRecord);
  const std::size_t product_bytes =
      products.size() * sizeof(detail::PairSupportRankProductInputRecord);
  const std::size_t initial_frontier_bytes =
      products.size() * sizeof(detail::PairSupportRankWorkItem);
  const std::size_t frontier_capacity_bytes =
      capacity.maximum_work_item_count *
      (2U * sizeof(detail::PairSupportRankWorkItem));
  const std::size_t receipt_capacity_bytes =
      capacity.maximum_receipt_count *
      sizeof(detail::PairSupportRankDeviceReceipt);
  validate_allocation_product(
      batch.traversal_epoch_count,
      5U * sizeof(std::uint64_t),
      "the Phase 9 rank-prune metadata D2H byte count overflows size_t");
  const std::size_t expected_metadata_d2h_bytes = checked_add_size(
      batch.traversal_epoch_count * (5U * sizeof(std::uint64_t)),
      sizeof(std::uint64_t),
      "the Phase 9 rank-prune metadata D2H byte count overflows size_t");
  const std::size_t fixed_product_bytes =
      capacity.maximum_product_count *
      (sizeof(detail::PairSupportRankProductInputRecord) +
       sizeof(std::uint64_t));
  const std::size_t fixed_work_bytes =
      capacity.maximum_work_item_count *
      (2U * sizeof(detail::PairSupportRankWorkItem) +
       4U * sizeof(std::uint64_t) +
       2U * sizeof(std::uint64_t));
  std::size_t expected_fixed_workspace_bytes = checked_add_size(
      fixed_product_bytes,
      fixed_work_bytes,
      "the Phase 9 rank-prune fixed workspace byte count overflows size_t");
  expected_fixed_workspace_bytes = checked_add_size(
      expected_fixed_workspace_bytes,
      receipt_capacity_bytes,
      "the Phase 9 rank-prune fixed workspace byte count overflows size_t");
  expected_fixed_workspace_bytes = checked_add_size(
      expected_fixed_workspace_bytes,
      sizeof(std::uint64_t),
      "the Phase 9 rank-prune fixed workspace byte count overflows size_t");
  expected_fixed_workspace_bytes = checked_add_size(
      expected_fixed_workspace_bytes,
      batch.device_scan_workspace_byte_capacity,
      "the Phase 9 rank-prune fixed workspace byte count overflows size_t");
  if (batch.receipts.size() != capacity.maximum_receipt_count ||
      batch.input_product_count != products.size() ||
      batch.product_capacity != capacity.maximum_product_count ||
      batch.work_item_capacity != capacity.maximum_work_item_count ||
      batch.receipt_capacity != capacity.maximum_receipt_count ||
      batch.receipt_count > capacity.maximum_receipt_count ||
      batch.traversal_epoch_count == 0U ||
      batch.traversal_epoch_count > budget.maximum_epoch_count ||
      batch.count_kernel_launch_count != batch.traversal_epoch_count ||
      batch.traversal_epoch_count >
          std::numeric_limits<std::size_t>::max() / 2U ||
      batch.exclusive_scan_count !=
          2U * batch.traversal_epoch_count ||
      batch.buffer_epoch == 0U ||
      batch.buffer_epoch <= previous_buffer_epoch ||
      batch.peak_frontier_count == 0U ||
      batch.peak_frontier_count < products.size() ||
      batch.peak_frontier_count > capacity.maximum_work_item_count ||
      batch.peak_frontier_count > batch.visited_work_item_count ||
      batch.receipt_count > batch.visited_work_item_count ||
      (batch.snapshot_h2d_byte_count != 0U &&
       batch.snapshot_h2d_byte_count != snapshot_bytes) ||
      batch.active_product_h2d_byte_count != product_bytes ||
      batch.initial_frontier_h2d_byte_count != initial_frontier_bytes ||
      batch.traversal_metadata_d2h_byte_count !=
          expected_metadata_d2h_bytes ||
      batch.physical_receipt_d2h_byte_count != receipt_capacity_bytes ||
      batch.active_receipt_d2h_byte_count !=
          batch.receipt_count *
              sizeof(detail::PairSupportRankDeviceReceipt) ||
      batch.device_frontier_double_buffer_byte_capacity !=
          frontier_capacity_bytes ||
      batch.device_receipt_byte_capacity != receipt_capacity_bytes ||
      batch.device_scan_workspace_byte_capacity == 0U ||
      batch.device_fixed_workspace_byte_capacity !=
          expected_fixed_workspace_bytes) {
    throw std::runtime_error(
        "the GPU rank-prune proposal returned invalid extent metadata");
  }

  if (batch.capacity_stop != detail::PairSupportRankCapacityStop::none &&
      batch.capacity_stop !=
          detail::PairSupportRankCapacityStop::work_item_capacity &&
      batch.capacity_stop !=
          detail::PairSupportRankCapacityStop::receipt_capacity) {
    throw std::runtime_error(
        "the GPU rank-prune proposal returned an invalid capacity stop");
  }
  const bool capacity_exhausted =
      batch.capacity_stop != detail::PairSupportRankCapacityStop::none;
  if ((capacity_exhausted &&
       (batch.emit_kernel_launch_count + 1U !=
            batch.traversal_epoch_count ||
        batch.frontier_exhausted)) ||
      (!capacity_exhausted &&
       batch.emit_kernel_launch_count != batch.traversal_epoch_count) ||
      (!batch.frontier_exhausted && !capacity_exhausted &&
       batch.traversal_epoch_count != budget.maximum_epoch_count)) {
    throw std::runtime_error(
        "the GPU rank-prune proposal returned inconsistent traversal metadata");
  }
  for (std::size_t index = batch.receipt_count;
       index < batch.receipts.size();
       ++index) {
    if (!sentinel_rank_receipt(batch.receipts[index])) {
      throw std::runtime_error(
          "the GPU rank-prune proposal exposed a stale receipt tail");
    }
  }

  PairSupportRankPruneBatchResult result;
  PairSupportRankPruneAudit& audit = result.audit;
  audit.capacity = capacity;
  audit.budget = budget;
  audit.input_product_count = products.size();
  audit.required_strict_interior_point_count =
      required_strict_interior_point_count;
  audit.gpu_traversal_epoch_count = batch.traversal_epoch_count;
  audit.gpu_count_kernel_launch_count =
      batch.count_kernel_launch_count;
  audit.gpu_exclusive_scan_count = batch.exclusive_scan_count;
  audit.gpu_emit_kernel_launch_count = batch.emit_kernel_launch_count;
  audit.gpu_visited_work_item_count = batch.visited_work_item_count;
  audit.gpu_peak_frontier_count = batch.peak_frontier_count;
  audit.gpu_output_receipt_count = batch.receipt_count;
  audit.buffer_epoch = batch.buffer_epoch;
  audit.work_item_capacity_exhausted =
      batch.capacity_stop ==
      detail::PairSupportRankCapacityStop::work_item_capacity;
  audit.receipt_capacity_exhausted =
      batch.capacity_stop ==
      detail::PairSupportRankCapacityStop::receipt_capacity;
  audit.device_frontier_exhausted = batch.frontier_exhausted;
  audit.epoch_budget_exhausted =
      !batch.frontier_exhausted && !capacity_exhausted;
  audit.immutable_lbvh_snapshot_validated = true;
  audit.product_records_validated = true;

  std::vector<std::vector<hierarchy::ExactPairSupportWitnessNodeEntry>>
      receipts_by_product(products.size());
  std::vector<std::vector<std::pair<std::uint64_t, std::uint64_t>>>
      ranges_by_product(products.size());
  std::vector<std::size_t> point_counts(products.size(), 0U);
  std::uint64_t digest = kFnvOffsetBasis;
  hash_word(
      digest,
      checked_u64(
          required_strict_interior_point_count,
          "the Phase 9 rank-prune threshold does not fit uint64"));
  for (std::size_t product_index = 0U;
       product_index < products.size();
       ++product_index) {
    const auto& product = products[product_index];
    hash_word(
        digest,
        checked_u64(
            product_index,
            "a Phase 9 rank-prune product position does not fit uint64"));
    hash_word(digest, product.first_node_index);
    hash_word(digest, product.second_node_index);
    hash_word(digest, product.first_leaf_begin);
    hash_word(digest, product.first_leaf_end);
    hash_word(digest, product.second_leaf_begin);
    hash_word(digest, product.second_leaf_end);
    hash_word(digest, product.self_product);
  }
  for (std::size_t position = 0U;
       position < batch.receipt_count;
       ++position) {
    const detail::PairSupportRankDeviceReceipt& receipt =
        batch.receipts[position];
    if (receipt.receipt_index != checked_u64(
            position,
            "a Phase 9 rank-prune receipt position does not fit uint64")) {
      throw std::runtime_error(
          "the GPU rank-prune receipt transcript is not stably indexed");
    }
    const std::size_t product_index = checked_size(
        receipt.product_slot,
        "a GPU rank-prune product slot does not fit size_t");
    const std::size_t witness_index = checked_size(
        receipt.witness_node_index,
        "a GPU rank-prune witness node index does not fit size_t");
    if (product_index >= products.size() || witness_index >= nodes.size()) {
      throw std::runtime_error(
          "a GPU rank-prune receipt references an unknown authority");
    }
    const detail::PairSupportPhiNodeInputRecord& witness =
        nodes[witness_index];
    const hierarchy::ExactPairSupportFrontierEntry& product =
        products[product_index];
    const detail::PairSupportPhiNodeInputRecord& first =
        nodes[static_cast<std::size_t>(product.first_node_index)];
    const detail::PairSupportPhiNodeInputRecord& second =
        nodes[static_cast<std::size_t>(product.second_node_index)];
    if (receipt.leaf_begin != witness.leaf_begin ||
        receipt.leaf_end != witness.leaf_end ||
        intervals_intersect(witness, first) ||
        intervals_intersect(witness, second) ||
        !strictly_negative_binary64(receipt.upper_phi_bits)) {
      throw std::runtime_error(
          "a GPU rank-prune receipt contradicts its LBVH identity");
    }

    hierarchy::ExactDiametralPhiAabbMaximum exact_receipt =
        hierarchy::exact_diametral_phi_aabb_maximum(
            node_box(first), node_box(second), node_box(witness));
    ++audit.cpu_exact_phi_recertification_count;
    const exact::ExactRational proposed_upper =
        exact::ExactRational::from_binary64_bits(receipt.upper_phi_bits);
    if (exact_receipt.maximum_phi > proposed_upper ||
        exact_receipt.maximum_phi.sign() >= 0) {
      throw std::runtime_error(
          "a GPU rank-prune receipt failed exact CPU recertification");
    }

    const std::size_t subtree_size = checked_size(
        witness.leaf_end - witness.leaf_begin,
        "a GPU rank-prune witness cardinality does not fit size_t");
    if (point_counts[product_index] >
        std::numeric_limits<std::size_t>::max() - subtree_size) {
      throw std::runtime_error(
          "the GPU rank-prune receipt cardinality overflows size_t");
    }
    point_counts[product_index] += subtree_size;
    receipts_by_product[product_index].push_back(
        hierarchy::ExactPairSupportWitnessNodeEntry{
            receipt.witness_node_index,
            receipt.leaf_begin,
            receipt.leaf_end});
    ranges_by_product[product_index].push_back(
        {receipt.leaf_begin, receipt.leaf_end});

    hash_word(digest, receipt.receipt_index);
    hash_word(digest, receipt.product_slot);
    hash_word(digest, receipt.witness_node_index);
    hash_word(digest, receipt.leaf_begin);
    hash_word(digest, receipt.leaf_end);
    hash_word(digest, receipt.upper_phi_bits);
  }
  audit.stable_receipt_transcript_validated = true;

  for (std::size_t product_index = 0U;
       product_index < products.size();
       ++product_index) {
    auto sorted_ranges = ranges_by_product[product_index];
    std::sort(sorted_ranges.begin(), sorted_ranges.end());
    for (std::size_t range_index = 1U;
         range_index < sorted_ranges.size();
         ++range_index) {
      if (sorted_ranges[range_index - 1U].second >
          sorted_ranges[range_index].first) {
        throw std::runtime_error(
            "GPU rank-prune receipts do not form a disjoint antichain");
      }
    }
    if (point_counts[product_index] <
        required_strict_interior_point_count) {
      continue;
    }
    auto& canonical_receipts = receipts_by_product[product_index];
    std::sort(
        canonical_receipts.begin(),
        canonical_receipts.end(),
        [](const auto& left, const auto& right) {
          return std::tuple{
                     left.leaf_begin, left.leaf_end, left.node_index} <
                 std::tuple{
                     right.leaf_begin, right.leaf_end, right.node_index};
        });
    std::vector<hierarchy::ExactPairSupportWitnessNodeEntry>
        minimal_prefix;
    minimal_prefix.reserve(std::min(
        canonical_receipts.size(),
        required_strict_interior_point_count));
    std::size_t prefix_point_count = 0U;
    for (const auto& receipt : canonical_receipts) {
      if (prefix_point_count >= required_strict_interior_point_count) {
        break;
      }
      const std::size_t subtree_size = checked_size(
          receipt.leaf_end - receipt.leaf_begin,
          "a canonical rank-prune receipt cardinality does not fit size_t");
      if (prefix_point_count >
          std::numeric_limits<std::size_t>::max() - subtree_size) {
        throw std::runtime_error(
            "the canonical rank-prune receipt prefix overflows size_t");
      }
      prefix_point_count += subtree_size;
      minimal_prefix.push_back(receipt);
    }
    if (prefix_point_count < required_strict_interior_point_count ||
        minimal_prefix.size() >
            required_strict_interior_point_count) {
      throw std::logic_error(
          "the canonical rank-prune receipt prefix did not close");
    }
    result.proposals.push_back(PairSupportRankPruneProductProposal{
        product_index,
        products[product_index],
        std::move(minimal_prefix),
        prefix_point_count});
  }
  audit.disjoint_receipt_antichains_validated = true;
  audit.cpu_exact_recertification_complete = true;
  audit.proposed_product_count = result.proposals.size();
  audit.fallback_product_count =
      products.size() - result.proposals.size();
  audit.snapshot_h2d_byte_count = batch.snapshot_h2d_byte_count;
  audit.active_product_h2d_byte_count =
      batch.active_product_h2d_byte_count;
  audit.initial_frontier_h2d_byte_count =
      batch.initial_frontier_h2d_byte_count;
  audit.traversal_metadata_d2h_byte_count =
      batch.traversal_metadata_d2h_byte_count;
  audit.physical_receipt_d2h_byte_count =
      batch.physical_receipt_d2h_byte_count;
  audit.active_receipt_d2h_byte_count =
      batch.active_receipt_d2h_byte_count;
  audit.device_frontier_double_buffer_byte_capacity =
      batch.device_frontier_double_buffer_byte_capacity;
  audit.device_receipt_byte_capacity =
      batch.device_receipt_byte_capacity;
  audit.device_scan_workspace_byte_capacity =
      batch.device_scan_workspace_byte_capacity;
  audit.device_fixed_workspace_byte_capacity =
      batch.device_fixed_workspace_byte_capacity;
  audit.receipt_digest_fnv1a = digest;
  audit.global_support_product_prune_published = false;
  audit.public_status_published = false;
  return result;
}

}  // namespace

PairSupportPhiContext::PairSupportPhiContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t maximum_query_count)
    : PairSupportPhiContext(
          index,
          cloud,
          maximum_query_count,
          PairSupportRankPruneCapacity{}) {}

PairSupportPhiContext::PairSupportPhiContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t maximum_query_count,
    PairSupportRankPruneCapacity rank_prune_capacity)
    : state_(std::make_shared<detail::PairSupportPhiContextState>()),
      maximum_query_count_(maximum_query_count),
      rank_prune_capacity_(rank_prune_capacity) {
  if (!index.validated_for(cloud) || cloud.size() < 2U) {
    throw std::invalid_argument(
        "a Phase 9 pair-support phi context requires a matching nontrivial LBVH");
  }
  if (maximum_query_count_ == 0U) {
    throw std::invalid_argument(
        "a Phase 9 pair-support phi query capacity must be nonzero");
  }
  validate_allocation_product(
      maximum_query_count_,
      sizeof(detail::PairSupportPhiQueryInputRecord) +
          2U * sizeof(detail::PairSupportPhiDeviceRecord),
      "the Phase 9 pair-support phi query workspace size overflows size_t");
  if ((rank_prune_capacity_.maximum_product_count == 0U) !=
          (rank_prune_capacity_.maximum_work_item_count == 0U) ||
      (rank_prune_capacity_.maximum_product_count == 0U) !=
          (rank_prune_capacity_.maximum_receipt_count == 0U)) {
    throw std::invalid_argument(
        "the Phase 9 rank-prune capacities must be all zero or all positive");
  }
  validate_allocation_product(
      rank_prune_capacity_.maximum_product_count,
      sizeof(detail::PairSupportRankProductInputRecord) +
          sizeof(std::uint64_t),
      "the Phase 9 rank-prune product workspace size overflows size_t");
  validate_allocation_product(
      rank_prune_capacity_.maximum_work_item_count,
      2U * sizeof(detail::PairSupportRankWorkItem) +
          6U * sizeof(std::uint64_t),
      "the Phase 9 rank-prune work-item workspace size overflows size_t");
  validate_allocation_product(
      rank_prune_capacity_.maximum_receipt_count,
      sizeof(detail::PairSupportRankDeviceReceipt),
      "the Phase 9 rank-prune receipt workspace size overflows size_t");
  if (index.nodes_.empty() || index.root_index_ >= index.nodes_.size() ||
      index.leaves_.size() != cloud.size()) {
    throw std::logic_error(
        "the Phase 9 pair-support phi LBVH authority is incomplete");
  }
  nodes_.reserve(index.nodes_.size());
  root_node_index_ = checked_u64(
      index.root_index_,
      "the Phase 9 pair-support root node index does not fit uint64");
  leaf_node_index_by_point_id_.assign(
      cloud.size(), detail::pair_support_phi_sentinel);
  for (std::size_t node_index = 0U;
       node_index < index.nodes_.size();
       ++node_index) {
    const auto& node = index.nodes_[node_index];
    if (node.leaf_begin >= node.leaf_end ||
        node.leaf_end > index.leaves_.size()) {
      throw std::logic_error(
          "a Phase 9 pair-support phi LBVH node has an invalid Morton range");
    }
    detail::PairSupportPhiNodeInputRecord packed{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      packed.lower_bits[axis] = cloud.point(node.lower_point_ids[axis])
                                    .canonical_input_bits()[axis];
      packed.upper_bits[axis] = cloud.point(node.upper_point_ids[axis])
                                    .canonical_input_bits()[axis];
      if (exact::binary64_total_order_key(packed.lower_bits[axis]) >
          exact::binary64_total_order_key(packed.upper_bits[axis])) {
        throw std::logic_error(
            "a Phase 9 pair-support phi LBVH node has a reversed AABB");
      }
    }
    packed.leaf_begin = checked_u64(
        node.leaf_begin,
        "a Phase 9 pair-support phi leaf begin does not fit uint64");
    packed.leaf_end = checked_u64(
        node.leaf_end,
        "a Phase 9 pair-support phi leaf end does not fit uint64");
    if (!node.is_leaf()) {
      packed.left_child = checked_u64(
          node.left_child,
          "a Phase 9 pair-support left child index does not fit uint64");
      packed.right_child = checked_u64(
          node.right_child,
          "a Phase 9 pair-support right child index does not fit uint64");
    }
    nodes_.push_back(packed);
    if (node.is_leaf()) {
      const spatial::PointId point_id =
          index.leaves_[node.leaf_begin].point_id;
      const std::size_t point_index = checked_size(
          point_id,
          "a Phase 9 pair-support phi leaf PointId does not fit size_t");
      if (point_index >= leaf_node_index_by_point_id_.size() ||
          leaf_node_index_by_point_id_[point_index] !=
              detail::pair_support_phi_sentinel) {
        throw std::logic_error(
            "a Phase 9 pair-support phi leaf map repeats a PointId");
      }
      leaf_node_index_by_point_id_[point_index] = checked_u64(
          node_index,
          "a Phase 9 pair-support phi leaf node index does not fit uint64");
    }
  }
  if (std::find(
          leaf_node_index_by_point_id_.begin(),
          leaf_node_index_by_point_id_.end(),
          detail::pair_support_phi_sentinel) !=
      leaf_node_index_by_point_id_.end()) {
    throw std::logic_error(
        "the Phase 9 pair-support phi leaf map loses a PointId");
  }
}

PairSupportPhiContext::~PairSupportPhiContext() noexcept = default;
PairSupportPhiContext::PairSupportPhiContext(
    PairSupportPhiContext&&) noexcept = default;
PairSupportPhiContext& PairSupportPhiContext::operator=(
    PairSupportPhiContext&&) noexcept = default;

PairSupportPhiBatchResult PairSupportPhiContext::classify_witnesses(
    std::span<const PairSupportPhiWitnessQuery> canonical_queries) {
  if (state_ == nullptr || nodes_.empty() || maximum_query_count_ == 0U) {
    throw std::invalid_argument(
        "a moved-from Phase 9 pair-support phi context is not queryable");
  }
  const std::vector<detail::PairSupportPhiQueryInputRecord> packed_queries =
      validate_and_pack_queries(
          canonical_queries, nodes_, maximum_query_count_);
  return state_->with_gpu_section([&] {
    const detail::PairSupportPhiDeviceBatch batch =
        detail::propose_pair_support_phi_on_gpu(
            *state_, nodes_, packed_queries, maximum_query_count_);
    PairSupportPhiBatchResult result = validate_and_recertify(
        batch,
        canonical_queries,
        nodes_,
        maximum_query_count_,
        last_buffer_epoch_);
    last_buffer_epoch_ = batch.buffer_epoch;
    return result;
  });
}

PairSupportRankPruneBatchResult PairSupportPhiContext::propose_rank_prunes(
    std::span<const hierarchy::ExactPairSupportFrontierEntry> products,
    std::size_t required_strict_interior_point_count,
    PairSupportRankPruneBudget budget) {
  if (state_ == nullptr || nodes_.empty() || maximum_query_count_ == 0U) {
    throw std::invalid_argument(
        "a moved-from Phase 9 pair-support phi context is not queryable");
  }
  const std::vector<detail::PairSupportRankProductInputRecord>
      packed_products = validate_and_pack_rank_products(
          products,
          nodes_,
          rank_prune_capacity_,
          required_strict_interior_point_count,
          budget);
  return state_->with_gpu_section([&] {
    const detail::PairSupportRankDeviceBatch batch =
        detail::propose_pair_support_rank_prunes_on_gpu(
            *state_,
            nodes_,
            root_node_index_,
            packed_products,
            required_strict_interior_point_count,
            rank_prune_capacity_.maximum_product_count,
            rank_prune_capacity_.maximum_work_item_count,
            rank_prune_capacity_.maximum_receipt_count,
            budget.maximum_epoch_count);
    PairSupportRankPruneBatchResult result =
        validate_and_recertify_rank_prunes(
            batch,
            products,
            nodes_,
            rank_prune_capacity_,
            required_strict_interior_point_count,
            budget,
            last_rank_prune_buffer_epoch_);
    last_rank_prune_buffer_epoch_ = batch.buffer_epoch;
    return result;
  });
}

PairSupportPhiWitnessQuery PairSupportPhiContext::make_leaf_witness_query(
    spatial::PointId first_support_id,
    spatial::PointId second_support_id,
    spatial::PointId witness_id) const {
  if (state_ == nullptr || first_support_id == second_support_id ||
      first_support_id == witness_id || second_support_id == witness_id) {
    throw std::invalid_argument(
        "a Phase 9 leaf phi query requires three distinct PointIds");
  }
  const std::size_t first_point = checked_size(
      first_support_id,
      "a Phase 9 first leaf PointId does not fit size_t");
  const std::size_t second_point = checked_size(
      second_support_id,
      "a Phase 9 second leaf PointId does not fit size_t");
  const std::size_t witness_point = checked_size(
      witness_id,
      "a Phase 9 witness leaf PointId does not fit size_t");
  if (first_point >= leaf_node_index_by_point_id_.size() ||
      second_point >= leaf_node_index_by_point_id_.size() ||
      witness_point >= leaf_node_index_by_point_id_.size()) {
    throw std::out_of_range(
        "a Phase 9 leaf phi query PointId is outside the resident snapshot");
  }
  std::uint64_t first_node = leaf_node_index_by_point_id_[first_point];
  std::uint64_t second_node = leaf_node_index_by_point_id_[second_point];
  const std::uint64_t witness_node =
      leaf_node_index_by_point_id_[witness_point];
  const std::size_t first_node_index = static_cast<std::size_t>(first_node);
  const std::size_t second_node_index = static_cast<std::size_t>(second_node);
  if (nodes_[second_node_index].leaf_begin <
      nodes_[first_node_index].leaf_begin) {
    std::swap(first_node, second_node);
  }
  return PairSupportPhiWitnessQuery{
      first_node, second_node, witness_node};
}

std::size_t PairSupportPhiContext::node_count() const noexcept {
  return nodes_.size();
}

PairSupportPhiNodeDescriptor PairSupportPhiContext::node_descriptor(
    std::size_t node_index) const {
  if (state_ == nullptr || node_index >= nodes_.size()) {
    throw std::out_of_range(
        "a Phase 9 pair-support phi node descriptor is unavailable");
  }
  const detail::PairSupportPhiNodeInputRecord& node = nodes_[node_index];
  return PairSupportPhiNodeDescriptor{
      checked_u64(
          node_index,
          "a Phase 9 pair-support phi node index does not fit uint64"),
      node.leaf_begin,
      node.leaf_end,
      node_box(node)};
}

}  // namespace morsehgp3d::gpu
