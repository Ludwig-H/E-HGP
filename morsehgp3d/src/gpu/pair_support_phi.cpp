#include "morsehgp3d/gpu/pair_support_phi.hpp"

#include "../cuda/phase9_pair_support_phi_internal.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <algorithm>
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

[[nodiscard]] bool leaf_node(
    const detail::PairSupportPhiNodeInputRecord& node) noexcept {
  return node.left_child == detail::pair_support_phi_sentinel &&
         node.right_child == detail::pair_support_phi_sentinel;
}

[[nodiscard]] std::vector<std::uint32_t> build_escape_node_indices(
    std::span<const detail::PairSupportPhiNodeInputRecord> nodes,
    std::uint64_t root_node_index) {
  if (nodes.size() >
      static_cast<std::size_t>(
          std::numeric_limits<std::uint32_t>::max())) {
    // The rope is an optional optimization.  Keeping it empty selects the
    // historical bounded traversal without invalidating the context.
    return {};
  }
  const std::size_t root_index = checked_size(
      root_node_index,
      "the Phase 14Q stackless root index does not fit size_t");
  if (nodes.empty() || root_index != nodes.size() - 1U) {
    throw std::logic_error(
        "the Phase 14Q stackless LBVH is not strict postorder");
  }

  std::vector<std::uint32_t> escape(
      nodes.size(), detail::pair_support_rank_escape_sentinel);

  for (std::size_t parent_plus_one = nodes.size();
       parent_plus_one != 0U;
       --parent_plus_one) {
    const std::size_t parent_index = parent_plus_one - 1U;
    const detail::PairSupportPhiNodeInputRecord& parent =
        nodes[parent_index];
    const bool left_missing =
        parent.left_child == detail::pair_support_phi_sentinel;
    const bool right_missing =
        parent.right_child == detail::pair_support_phi_sentinel;
    if (left_missing != right_missing) {
      throw std::logic_error(
          "the Phase 14Q stackless LBVH has incomplete topology");
    }
    if (left_missing) {
      continue;
    }

    const std::size_t left_index = checked_size(
        parent.left_child,
        "a Phase 14Q stackless left child does not fit size_t");
    const std::size_t right_index = checked_size(
        parent.right_child,
        "a Phase 14Q stackless right child does not fit size_t");
    if (left_index >= parent_index || right_index >= parent_index ||
        left_index == right_index) {
      throw std::logic_error(
          "the Phase 14Q stackless LBVH is not a strict postorder tree");
    }
    const detail::PairSupportPhiNodeInputRecord& left =
        nodes[left_index];
    const detail::PairSupportPhiNodeInputRecord& right =
        nodes[right_index];
    if (left.leaf_begin != parent.leaf_begin ||
        left.leaf_begin >= left.leaf_end ||
        left.leaf_end != right.leaf_begin ||
        right.leaf_begin >= right.leaf_end ||
        right.leaf_end != parent.leaf_end) {
      throw std::logic_error(
          "the Phase 14Q stackless LBVH child ranges do not tile their parent");
    }

    escape[left_index] = static_cast<std::uint32_t>(right_index);
    escape[right_index] = escape[parent_index];
  }
  return escape;
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

[[nodiscard]] std::uint64_t authenticated_first_leaf_node(
    std::span<const detail::PairSupportPhiNodeInputRecord> nodes,
    std::size_t support_node_index) {
  if (support_node_index >= nodes.size()) {
    throw std::invalid_argument(
        "a Phase 9 rank-prune support node is unavailable");
  }
  const detail::PairSupportPhiNodeInputRecord& support =
      nodes[support_node_index];
  if (support.leaf_begin >= support.leaf_end) {
    throw std::logic_error(
        "a Phase 9 rank-prune support has an empty Morton range");
  }
  const std::uint64_t expected_leaf_begin = support.leaf_begin;
  const std::uint64_t support_leaf_end = support.leaf_end;
  std::size_t current_index = support_node_index;
  for (std::size_t depth = 0U; depth < nodes.size(); ++depth) {
    if (current_index >= nodes.size()) {
      throw std::logic_error(
          "a Phase 9 rank-prune anchor path leaves the resident LBVH");
    }
    const detail::PairSupportPhiNodeInputRecord& current =
        nodes[current_index];
    if (current.leaf_begin != expected_leaf_begin ||
        current.leaf_begin >= current.leaf_end ||
        current.leaf_end > support_leaf_end) {
      throw std::logic_error(
          "a Phase 9 rank-prune anchor path contradicts Morton ranges");
    }
    const bool left_missing =
        current.left_child == detail::pair_support_phi_sentinel;
    const bool right_missing =
        current.right_child == detail::pair_support_phi_sentinel;
    if (left_missing != right_missing) {
      throw std::logic_error(
          "a Phase 9 rank-prune anchor path has incomplete topology");
    }
    if (left_missing) {
      if (current.leaf_end - current.leaf_begin != 1U) {
        throw std::logic_error(
            "a Phase 9 rank-prune anchor is not a singleton leaf");
      }
      for (std::size_t axis = 0U; axis < 3U; ++axis) {
        if (current.lower_bits[axis] != current.upper_bits[axis]) {
          throw std::logic_error(
              "a Phase 9 rank-prune anchor leaf is not degenerate");
        }
      }
      return checked_u64(
          current_index,
          "a Phase 9 rank-prune anchor node index does not fit uint64");
    }

    const std::size_t left_index = checked_size(
        current.left_child,
        "a Phase 9 rank-prune left child index does not fit size_t");
    const std::size_t right_index = checked_size(
        current.right_child,
        "a Phase 9 rank-prune right child index does not fit size_t");
    if (left_index >= nodes.size() || right_index >= nodes.size() ||
        left_index == right_index || left_index == current_index ||
        right_index == current_index) {
      throw std::logic_error(
          "a Phase 9 rank-prune anchor path has invalid children");
    }
    const detail::PairSupportPhiNodeInputRecord& left = nodes[left_index];
    const detail::PairSupportPhiNodeInputRecord& right = nodes[right_index];
    if (left.leaf_begin != current.leaf_begin ||
        left.leaf_begin >= left.leaf_end ||
        left.leaf_end != right.leaf_begin ||
        right.leaf_begin >= right.leaf_end ||
        right.leaf_end != current.leaf_end) {
      throw std::logic_error(
          "a Phase 9 rank-prune anchor path has unauthenticated child ranges");
    }
    current_index = left_index;
  }
  throw std::logic_error(
      "a Phase 9 rank-prune anchor path contains a cycle");
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
    const std::uint64_t first_anchor =
        authenticated_first_leaf_node(nodes, first);
    const std::uint64_t second_anchor =
        authenticated_first_leaf_node(nodes, second);
    packed.push_back(detail::PairSupportRankProductInputRecord{
        product.first_node_index,
        product.second_node_index,
        first_anchor,
        second_anchor});
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
    std::uint64_t root_node_index,
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
      sizeof(detail::PairSupportRankDeviceTerminal),
      "the Phase 9 rank-prune terminal byte capacity overflows size_t");
  const std::size_t root_index = checked_size(
      root_node_index,
      "the Phase 9 rank-prune root node index does not fit size_t");
  if (root_index >= nodes.size() ||
      nodes[root_index].leaf_begin != 0U ||
      nodes[root_index].leaf_begin >= nodes[root_index].leaf_end) {
    throw std::logic_error(
        "the Phase 9 rank-prune root authority is invalid");
  }
  const std::uint64_t root_leaf_end = nodes[root_index].leaf_end;
  const std::size_t snapshot_bytes =
      nodes.size() * sizeof(detail::PairSupportPhiNodeInputRecord);
  const std::size_t product_bytes =
      products.size() * sizeof(detail::PairSupportRankProductInputRecord);
  const std::size_t initial_frontier_bytes =
      products.size() * sizeof(detail::PairSupportRankWorkItem);
  const std::size_t frontier_capacity_bytes =
      capacity.maximum_work_item_count *
      (2U * sizeof(detail::PairSupportRankWorkItem));
  const std::size_t terminal_capacity_bytes =
      capacity.maximum_receipt_count *
      sizeof(detail::PairSupportRankDeviceTerminal);
  validate_allocation_product(
      nodes.size(),
      sizeof(std::uint32_t),
      "the Phase 14Q rank-prune escape snapshot overflows size_t");
  const std::size_t escape_snapshot_bytes =
      nodes.size() * sizeof(std::uint32_t);
  const bool legacy =
      batch.traversal_backend ==
      detail::PairSupportRankTraversalBackend::two_frontier;
  const bool stackless =
      batch.traversal_backend ==
      detail::PairSupportRankTraversalBackend::stackless_single_product;
  if (!legacy && !stackless) {
    throw std::runtime_error(
        "the GPU rank-prune proposal returned an invalid traversal backend");
  }
  if (batch.terminals.size() != batch.terminal_count ||
      batch.input_product_count != products.size() ||
      batch.product_capacity != capacity.maximum_product_count ||
      batch.work_item_capacity != capacity.maximum_work_item_count ||
      batch.terminal_capacity != capacity.maximum_receipt_count ||
      batch.terminal_count > capacity.maximum_receipt_count ||
      batch.buffer_epoch == 0U ||
      batch.buffer_epoch <= previous_buffer_epoch ||
      batch.peak_frontier_count == 0U ||
      batch.peak_frontier_count > batch.visited_work_item_count ||
      batch.terminal_count > batch.visited_work_item_count ||
      (batch.snapshot_h2d_byte_count != 0U &&
       batch.snapshot_h2d_byte_count != snapshot_bytes) ||
      batch.active_product_h2d_byte_count != product_bytes ||
      batch.active_terminal_d2h_byte_count !=
          batch.terminal_count *
              sizeof(detail::PairSupportRankDeviceTerminal) ||
      batch.physical_terminal_d2h_byte_count !=
          batch.active_terminal_d2h_byte_count ||
      batch.device_terminal_byte_capacity != terminal_capacity_bytes ||
      !batch.anchor_ball_culling_enabled) {
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

  if (legacy) {
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
         6U * sizeof(std::uint64_t));
    std::size_t expected_fixed_workspace_bytes = checked_add_size(
        fixed_product_bytes,
        fixed_work_bytes,
        "the Phase 9 rank-prune fixed workspace byte count overflows size_t");
    expected_fixed_workspace_bytes = checked_add_size(
        expected_fixed_workspace_bytes,
        terminal_capacity_bytes,
        "the Phase 9 rank-prune fixed workspace byte count overflows size_t");
    expected_fixed_workspace_bytes = checked_add_size(
        expected_fixed_workspace_bytes,
        sizeof(std::uint64_t),
        "the Phase 9 rank-prune fixed workspace byte count overflows size_t");
    expected_fixed_workspace_bytes = checked_add_size(
        expected_fixed_workspace_bytes,
        batch.device_scan_workspace_byte_capacity,
        "the Phase 9 rank-prune fixed workspace byte count overflows size_t");
    if (batch.traversal_epoch_count == 0U ||
        batch.traversal_epoch_count > budget.maximum_epoch_count ||
        batch.count_kernel_launch_count != batch.traversal_epoch_count ||
        batch.traversal_epoch_count >
            std::numeric_limits<std::size_t>::max() / 2U ||
        batch.exclusive_scan_count !=
            2U * batch.traversal_epoch_count ||
        batch.host_synchronization_count !=
            batch.traversal_epoch_count + 1U ||
        batch.peak_frontier_count < products.size() ||
        batch.peak_frontier_count > capacity.maximum_work_item_count ||
        batch.initial_frontier_h2d_byte_count != initial_frontier_bytes ||
        batch.traversal_metadata_d2h_byte_count !=
            expected_metadata_d2h_bytes ||
        batch.device_frontier_double_buffer_byte_capacity !=
            frontier_capacity_bytes ||
        batch.escape_snapshot_h2d_byte_count != 0U ||
        batch.device_escape_snapshot_byte_capacity != 0U ||
        batch.device_scan_workspace_byte_capacity == 0U ||
        batch.device_fixed_workspace_byte_capacity !=
            expected_fixed_workspace_bytes ||
        batch.visit_budget_count != 0U ||
        batch.visit_budget_exhausted ||
        (capacity_exhausted &&
         (batch.emit_kernel_launch_count + 1U !=
              batch.traversal_epoch_count ||
          batch.frontier_exhausted)) ||
        (!capacity_exhausted &&
         batch.emit_kernel_launch_count !=
             batch.traversal_epoch_count) ||
        (!batch.frontier_exhausted && !capacity_exhausted &&
         batch.traversal_epoch_count != budget.maximum_epoch_count)) {
      throw std::runtime_error(
          "the GPU two-frontier rank-prune metadata is inconsistent");
    }
  } else {
    validate_allocation_product(
        capacity.maximum_work_item_count,
        budget.maximum_epoch_count,
        "the Phase 14Q stackless visit budget overflows size_t");
    const std::size_t multiplied_visit_budget =
        capacity.maximum_work_item_count *
        budget.maximum_epoch_count;
    const std::size_t expected_visit_budget =
        std::min(nodes.size(), multiplied_visit_budget);
    const std::size_t expected_fixed_workspace_bytes =
        checked_add_size(
            checked_add_size(
                capacity.maximum_product_count *
                    sizeof(detail::PairSupportRankProductInputRecord),
                terminal_capacity_bytes,
                "the Phase 14Q stackless workspace overflows size_t"),
            detail::pair_support_rank_stackless_control_byte_count,
            "the Phase 14Q stackless workspace overflows size_t");
    if (products.size() != 1U ||
        capacity.maximum_product_count != 1U ||
        batch.traversal_epoch_count != 1U ||
        batch.count_kernel_launch_count != 0U ||
        batch.exclusive_scan_count != 0U ||
        batch.emit_kernel_launch_count != 1U ||
        batch.host_synchronization_count !=
            1U + static_cast<std::size_t>(
                     batch.terminal_count != 0U) ||
        batch.peak_frontier_count != 1U ||
        batch.initial_frontier_h2d_byte_count != 0U ||
        batch.traversal_metadata_d2h_byte_count !=
            detail::pair_support_rank_stackless_control_byte_count ||
        batch.device_frontier_double_buffer_byte_capacity != 0U ||
        (batch.escape_snapshot_h2d_byte_count != 0U &&
         batch.escape_snapshot_h2d_byte_count !=
             escape_snapshot_bytes) ||
        batch.device_escape_snapshot_byte_capacity !=
            escape_snapshot_bytes ||
        batch.device_scan_workspace_byte_capacity != 0U ||
        batch.device_fixed_workspace_byte_capacity !=
            expected_fixed_workspace_bytes ||
        batch.visit_budget_count != expected_visit_budget ||
        batch.visited_work_item_count > batch.visit_budget_count ||
        batch.capacity_stop ==
            detail::PairSupportRankCapacityStop::work_item_capacity ||
        (capacity_exhausted &&
         (batch.capacity_stop !=
              detail::PairSupportRankCapacityStop::receipt_capacity ||
          batch.terminal_count != capacity.maximum_receipt_count ||
          batch.frontier_exhausted ||
          batch.visit_budget_exhausted)) ||
        (batch.visit_budget_exhausted &&
         (batch.frontier_exhausted || capacity_exhausted ||
          batch.visited_work_item_count !=
              batch.visit_budget_count)) ||
        (!batch.frontier_exhausted && !capacity_exhausted &&
         !batch.visit_budget_exhausted) ||
        (batch.frontier_exhausted &&
         (capacity_exhausted || batch.visit_budget_exhausted))) {
      throw std::runtime_error(
          "the GPU stackless rank-prune metadata is inconsistent");
    }
  }
  PairSupportRankPruneBatchResult result;
  PairSupportRankPruneAudit& audit = result.audit;
  audit.capacity = capacity;
  audit.budget = budget;
  audit.traversal_backend =
      stackless
          ? PairSupportRankTraversalBackend::stackless_single_product
          : PairSupportRankTraversalBackend::two_frontier;
  audit.input_product_count = products.size();
  audit.required_strict_interior_point_count =
      required_strict_interior_point_count;
  audit.gpu_traversal_epoch_count = batch.traversal_epoch_count;
  audit.gpu_count_kernel_launch_count =
      batch.count_kernel_launch_count;
  audit.gpu_exclusive_scan_count = batch.exclusive_scan_count;
  audit.gpu_emit_kernel_launch_count = batch.emit_kernel_launch_count;
  audit.gpu_stackless_kernel_launch_count =
      stackless ? batch.emit_kernel_launch_count : 0U;
  audit.gpu_host_synchronization_count =
      batch.host_synchronization_count;
  audit.gpu_visited_work_item_count = batch.visited_work_item_count;
  audit.gpu_visit_budget_count = batch.visit_budget_count;
  audit.gpu_peak_frontier_count = batch.peak_frontier_count;
  audit.gpu_output_terminal_count = batch.terminal_count;
  audit.gpu_output_receipt_count = batch.terminal_count;
  audit.buffer_epoch = batch.buffer_epoch;
  audit.work_item_capacity_exhausted =
      batch.capacity_stop ==
      detail::PairSupportRankCapacityStop::work_item_capacity;
  audit.receipt_capacity_exhausted =
      batch.capacity_stop ==
      detail::PairSupportRankCapacityStop::receipt_capacity;
  audit.device_frontier_exhausted = batch.frontier_exhausted;
  audit.epoch_budget_exhausted =
      legacy
          ? !batch.frontier_exhausted && !capacity_exhausted
          : batch.visit_budget_exhausted;
  audit.visit_budget_exhausted = batch.visit_budget_exhausted;
  audit.immutable_lbvh_snapshot_validated = true;
  audit.product_records_validated = true;
  audit.anchor_ball_culling_enabled =
      batch.anchor_ball_culling_enabled;
  audit.stackless_single_product_traversal_used = stackless;

  std::vector<std::vector<PairSupportRankTerminalCertificate>>
      terminals_by_product(products.size());
  std::vector<std::size_t> point_counts(products.size(), 0U);
  std::vector<std::uint64_t> first_anchor_indices(products.size());
  std::vector<std::uint64_t> second_anchor_indices(products.size());
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
    first_anchor_indices[product_index] =
        authenticated_first_leaf_node(
            nodes,
            checked_size(
                product.first_node_index,
                "a Phase 9 rank-prune first support does not fit size_t"));
    second_anchor_indices[product_index] =
        authenticated_first_leaf_node(
            nodes,
            checked_size(
                product.second_node_index,
                "a Phase 9 rank-prune second support does not fit size_t"));
  }
  for (std::size_t position = 0U;
       position < batch.terminal_count;
       ++position) {
    const detail::PairSupportRankDeviceTerminal& terminal =
        batch.terminals[position];
    const std::size_t product_index = checked_size(
        terminal.product_slot,
        "a GPU rank-prune product slot does not fit size_t");
    const std::size_t witness_index = checked_size(
        terminal.witness_node_index,
        "a GPU rank-prune witness node index does not fit size_t");
    if (product_index >= products.size() || witness_index >= nodes.size()) {
      throw std::runtime_error(
          "a GPU rank-prune terminal references an unknown authority");
    }
    const detail::PairSupportPhiNodeInputRecord& witness =
        nodes[witness_index];
    const hierarchy::ExactPairSupportFrontierEntry& product =
        products[product_index];
    const detail::PairSupportPhiNodeInputRecord& first =
        nodes[static_cast<std::size_t>(product.first_node_index)];
    const detail::PairSupportPhiNodeInputRecord& second =
        nodes[static_cast<std::size_t>(product.second_node_index)];
    if (witness.leaf_begin >= witness.leaf_end ||
        witness.leaf_end > root_leaf_end ||
        intervals_intersect(witness, first) ||
        intervals_intersect(witness, second)) {
      throw std::runtime_error(
          "a GPU rank-prune terminal intersects its support authority");
    }

    const int exact_phi_sign =
        hierarchy::exact_diametral_phi_aabb_maximum_sign(
            node_box(first), node_box(second), node_box(witness));
    ++audit.cpu_exact_terminal_recertification_count;
    ++audit.cpu_exact_phi_recertification_count;

    PairSupportRankTerminalKind kind =
        PairSupportRankTerminalKind::unresolved_external_leaf;
    if (exact_phi_sign < 0) {
      kind = PairSupportRankTerminalKind::strict_interior;
      ++audit.strict_interior_terminal_count;
    } else {
      const bool cross_product =
          product.first_node_index != product.second_node_index;
      bool anchor_noninterior = false;
      if (cross_product) {
        const auto& first_anchor = nodes[checked_size(
            first_anchor_indices[product_index],
            "a Phase 9 first anchor index does not fit size_t")];
        const auto& second_anchor = nodes[checked_size(
            second_anchor_indices[product_index],
            "a Phase 9 second anchor index does not fit size_t")];
        anchor_noninterior =
            hierarchy::exact_diametral_anchor_phi_aabb_minimum_sign(
                node_box(first_anchor),
                node_box(second_anchor),
                node_box(witness)) >= 0;
      }
      if (anchor_noninterior) {
        kind = PairSupportRankTerminalKind::anchor_noninterior;
        ++audit.anchor_noninterior_terminal_count;
      } else if (leaf_node(witness)) {
        ++audit.unresolved_external_leaf_terminal_count;
      } else {
        throw std::runtime_error(
            "a GPU rank-prune terminal is an unresolved internal node");
      }
    }

    const std::size_t subtree_size = checked_size(
        witness.leaf_end - witness.leaf_begin,
        "a GPU rank-prune terminal cardinality does not fit size_t");
    if (kind == PairSupportRankTerminalKind::strict_interior) {
      if (point_counts[product_index] >
          std::numeric_limits<std::size_t>::max() - subtree_size) {
        throw std::runtime_error(
            "the GPU rank-prune strict cardinality overflows size_t");
      }
      point_counts[product_index] += subtree_size;
    }
    terminals_by_product[product_index].push_back(
        PairSupportRankTerminalCertificate{
            hierarchy::ExactPairSupportWitnessNodeEntry{
                terminal.witness_node_index,
                witness.leaf_begin,
                witness.leaf_end},
            kind});

    hash_word(
        digest,
        checked_u64(
            position,
            "a Phase 9 rank-prune terminal position does not fit uint64"));
    hash_word(digest, terminal.product_slot);
    hash_word(digest, terminal.witness_node_index);
    hash_word(digest, product.first_node_index);
    hash_word(digest, product.second_node_index);
    hash_word(digest, product.first_leaf_begin);
    hash_word(digest, product.first_leaf_end);
    hash_word(digest, product.second_leaf_begin);
    hash_word(digest, product.second_leaf_end);
    hash_word(digest, product.self_product);
  }
  if (audit.cpu_exact_terminal_recertification_count !=
          batch.terminal_count ||
      audit.strict_interior_terminal_count +
              audit.anchor_noninterior_terminal_count +
              audit.unresolved_external_leaf_terminal_count !=
          batch.terminal_count) {
    throw std::logic_error(
        "the exact terminal classification counters do not close");
  }
  audit.stable_terminal_transcript_validated = true;
  audit.stable_receipt_transcript_validated = true;

  for (std::size_t product_index = 0U;
       product_index < products.size();
       ++product_index) {
    auto& canonical_terminals = terminals_by_product[product_index];
    std::sort(
        canonical_terminals.begin(),
        canonical_terminals.end(),
        [](const auto& left, const auto& right) {
          return std::tuple{
                     left.terminal.leaf_begin,
                     left.terminal.leaf_end,
                     left.terminal.node_index} <
                 std::tuple{
                     right.terminal.leaf_begin,
                     right.terminal.leaf_end,
                     right.terminal.node_index};
        });
    for (std::size_t terminal_index = 1U;
         terminal_index < canonical_terminals.size();
         ++terminal_index) {
      if (canonical_terminals[terminal_index - 1U].terminal.leaf_end >
          canonical_terminals[terminal_index].terminal.leaf_begin) {
        throw std::runtime_error(
            "GPU rank-prune terminals do not form a disjoint antichain");
      }
    }

    // A stackless Q/C stop authenticates its returned active prefix for
    // diagnostics only.  No partial transcript may become a prune proposal
    // or a keep certificate, even if that prefix happens to contain K exact
    // strict witnesses.
    if (stackless && !batch.frontier_exhausted) {
      continue;
    }

    if (point_counts[product_index] >=
        required_strict_interior_point_count) {
      std::vector<hierarchy::ExactPairSupportWitnessNodeEntry>
          minimal_prefix;
      minimal_prefix.reserve(std::min(
          canonical_terminals.size(),
          required_strict_interior_point_count));
      std::size_t prefix_point_count = 0U;
      for (const auto& terminal : canonical_terminals) {
        if (prefix_point_count >=
            required_strict_interior_point_count) {
          break;
        }
        if (terminal.kind !=
            PairSupportRankTerminalKind::strict_interior) {
          continue;
        }
        const std::size_t subtree_size = checked_size(
            terminal.terminal.leaf_end -
                terminal.terminal.leaf_begin,
            "a canonical rank-prune terminal cardinality does not fit size_t");
        if (prefix_point_count >
            std::numeric_limits<std::size_t>::max() - subtree_size) {
          throw std::runtime_error(
              "the canonical rank-prune terminal prefix overflows size_t");
        }
        prefix_point_count += subtree_size;
        minimal_prefix.push_back(terminal.terminal);
      }
      if (prefix_point_count <
              required_strict_interior_point_count ||
          minimal_prefix.size() >
              required_strict_interior_point_count) {
        throw std::logic_error(
            "the canonical rank-prune terminal prefix did not close");
      }
      result.proposals.push_back(
          PairSupportRankPruneProductProposal{
              product_index,
              products[product_index],
              std::move(minimal_prefix),
              prefix_point_count});
      continue;
    }

    struct CoverageInterval {
      std::uint64_t leaf_begin{};
      std::uint64_t leaf_end{};
    };
    std::vector<CoverageInterval> coverage;
    coverage.reserve(checked_add_size(
        canonical_terminals.size(),
        2U,
        "the rank-prune coverage interval count overflows size_t"));
    const auto& product = products[product_index];
    coverage.push_back(
        CoverageInterval{
            product.first_leaf_begin, product.first_leaf_end});
    if (product.first_leaf_begin != product.second_leaf_begin ||
        product.first_leaf_end != product.second_leaf_end) {
      coverage.push_back(
          CoverageInterval{
              product.second_leaf_begin, product.second_leaf_end});
    }
    for (const auto& terminal : canonical_terminals) {
      coverage.push_back(
          CoverageInterval{
              terminal.terminal.leaf_begin,
              terminal.terminal.leaf_end});
    }
    std::sort(
        coverage.begin(),
        coverage.end(),
        [](const auto& left, const auto& right) {
          return std::tuple{left.leaf_begin, left.leaf_end} <
                 std::tuple{right.leaf_begin, right.leaf_end};
        });
    bool exact_coverage = !coverage.empty();
    std::uint64_t cursor = 0U;
    for (const CoverageInterval interval : coverage) {
      if (interval.leaf_begin != cursor ||
          interval.leaf_begin >= interval.leaf_end ||
          interval.leaf_end > root_leaf_end) {
        exact_coverage = false;
        break;
      }
      cursor = interval.leaf_end;
    }
    exact_coverage =
        exact_coverage && cursor == root_leaf_end &&
        product.self_product == 0U;
    if (exact_coverage) {
      result.keep_certificates.push_back(
          PairSupportRankKeepProductCertificate{
              product_index,
              product,
              std::move(canonical_terminals),
              point_counts[product_index]});
    }
  }
  audit.disjoint_terminal_antichains_validated = true;
  audit.disjoint_receipt_antichains_validated = true;
  audit.keep_coverage_recertification_complete = true;
  audit.cpu_exact_recertification_complete = true;
  audit.prune_product_count = result.proposals.size();
  audit.keep_certificate_product_count =
      result.keep_certificates.size();
  audit.proposed_product_count = result.proposals.size();
  if (audit.prune_product_count >
          products.size() - audit.keep_certificate_product_count) {
    throw std::logic_error(
        "the rank-prune product classification counters overflow");
  }
  audit.fallback_product_count = products.size() -
      audit.prune_product_count -
      audit.keep_certificate_product_count;
  audit.snapshot_h2d_byte_count = batch.snapshot_h2d_byte_count;
  audit.escape_snapshot_h2d_byte_count =
      batch.escape_snapshot_h2d_byte_count;
  audit.active_product_h2d_byte_count =
      batch.active_product_h2d_byte_count;
  audit.initial_frontier_h2d_byte_count =
      batch.initial_frontier_h2d_byte_count;
  audit.traversal_metadata_d2h_byte_count =
      batch.traversal_metadata_d2h_byte_count;
  audit.physical_terminal_d2h_byte_count =
      batch.physical_terminal_d2h_byte_count;
  audit.active_terminal_d2h_byte_count =
      batch.active_terminal_d2h_byte_count;
  audit.physical_receipt_d2h_byte_count =
      batch.physical_terminal_d2h_byte_count;
  audit.active_receipt_d2h_byte_count =
      batch.active_terminal_d2h_byte_count;
  audit.device_frontier_double_buffer_byte_capacity =
      batch.device_frontier_double_buffer_byte_capacity;
  audit.device_escape_snapshot_byte_capacity =
      batch.device_escape_snapshot_byte_capacity;
  audit.device_terminal_byte_capacity =
      batch.device_terminal_byte_capacity;
  audit.device_receipt_byte_capacity =
      batch.device_terminal_byte_capacity;
  audit.device_scan_workspace_byte_capacity =
      batch.device_scan_workspace_byte_capacity;
  audit.device_fixed_workspace_byte_capacity =
      batch.device_fixed_workspace_byte_capacity;
  audit.terminal_digest_fnv1a = digest;
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
      sizeof(detail::PairSupportRankDeviceTerminal),
      "the Phase 9 rank-prune terminal workspace size overflows size_t");
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
  if (rank_prune_capacity_.maximum_product_count == 1U) {
    escape_node_indices_ =
        build_escape_node_indices(nodes_, root_node_index_);
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
            escape_node_indices_,
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
            root_node_index_,
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
