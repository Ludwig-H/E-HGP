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

}  // namespace

PairSupportPhiContext::PairSupportPhiContext(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t maximum_query_count)
    : state_(std::make_shared<detail::PairSupportPhiContextState>()),
      maximum_query_count_(maximum_query_count) {
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
  if (index.nodes_.empty() || index.root_index_ >= index.nodes_.size() ||
      index.leaves_.size() != cloud.size()) {
    throw std::logic_error(
        "the Phase 9 pair-support phi LBVH authority is incomplete");
  }
  nodes_.reserve(index.nodes_.size());
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
