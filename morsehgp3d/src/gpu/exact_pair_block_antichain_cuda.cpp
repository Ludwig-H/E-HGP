#include "morsehgp3d/gpu/exact_pair_block_antichain_cuda.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

inline constexpr std::uint64_t digest_offset =
    UINT64_C(1469598103934665603);
inline constexpr std::uint64_t digest_prime = UINT64_C(1099511628211);
// Predicate-task ids are private, unique transcript identities.  Their low
// residue deliberately mirrors the witness-node selector used by the shared
// non-authoritative fake without reserving any caller-visible task-id bit.
inline constexpr std::uint64_t antichain_predicate_task_id_stride = 3U;

[[nodiscard]] bool checked_add(
    std::size_t first,
    std::size_t second,
    std::size_t& sum) noexcept {
  if (second > std::numeric_limits<std::size_t>::max() - first) {
    return false;
  }
  sum = first + second;
  return true;
}

[[nodiscard]] bool checked_product(
    std::size_t first,
    std::size_t second,
    std::size_t& product) noexcept {
  if (first != 0U &&
      second > std::numeric_limits<std::size_t>::max() / first) {
    return false;
  }
  product = first * second;
  return true;
}

[[nodiscard]] std::uint64_t make_predicate_task_id(
    std::size_t ordinal,
    std::size_t witness_node_index) {
  constexpr std::uint64_t maximum =
      std::numeric_limits<std::uint64_t>::max();
  if (ordinal >
      static_cast<std::size_t>(
          (maximum - (antichain_predicate_task_id_stride - 1U)) /
          antichain_predicate_task_id_stride)) {
    throw std::overflow_error(
        "an exact pair-block antichain predicate identity overflows");
  }
  return antichain_predicate_task_id_stride *
          static_cast<std::uint64_t>(ordinal) +
      static_cast<std::uint64_t>(
          witness_node_index % antichain_predicate_task_id_stride);
}

[[nodiscard]] bool ranges_overlap(
    const ExactPairBlockNodeAuthority& first,
    const ExactPairBlockNodeAuthority& second) noexcept {
  return first.leaf_begin < second.leaf_end &&
      second.leaf_begin < first.leaf_end;
}

[[nodiscard]] bool node_shape_is_valid(
    const ExactPairBlockNodeAuthority& node,
    std::size_t point_count,
    std::size_t certified_node_count) noexcept {
  return node.node_index < certified_node_count &&
      node.leaf_begin < node.leaf_end &&
      node.leaf_end <= point_count;
}

[[nodiscard]] std::uint64_t digest_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  for (unsigned int byte_index = 0U; byte_index < 8U; ++byte_index) {
    digest ^= word & UINT64_C(0xff);
    digest *= digest_prime;
    word >>= 8U;
  }
  return digest;
}

[[nodiscard]] std::uint64_t transaction_digest(
    std::span<const ExactPairBlockAntichainCudaTransaction>
        transactions) noexcept {
  std::uint64_t digest = digest_offset;
  digest = digest_word(
      digest, static_cast<std::uint64_t>(transactions.size()));
  for (const ExactPairBlockAntichainCudaTransaction& transaction :
       transactions) {
    const auto add_node = [&digest](const ExactPairBlockNodeAuthority& node) {
      digest = digest_word(
          digest, static_cast<std::uint64_t>(node.node_index));
      digest = digest_word(
          digest, static_cast<std::uint64_t>(node.leaf_begin));
      digest = digest_word(
          digest, static_cast<std::uint64_t>(node.leaf_end));
    };
    digest = digest_word(digest, transaction.transaction_id);
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(transaction.support_block.kind));
    add_node(transaction.support_block.first);
    add_node(transaction.support_block.second);
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(
            transaction.support_block.unordered_pair_mass));
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(transaction.witness_node_count));
    for (std::size_t witness_index = 0U;
         witness_index < transaction.witness_node_count &&
         witness_index <
             exact_pair_block_antichain_cuda_maximum_witness_node_count;
         ++witness_index) {
      add_node(transaction.witness_nodes[witness_index]);
    }
  }
  return digest;
}

[[nodiscard]] std::uint64_t result_digest(
    std::span<const ExactPairBlockAntichainCudaRecord> records) noexcept {
  std::uint64_t digest = digest_offset;
  digest =
      digest_word(digest, static_cast<std::uint64_t>(records.size()));
  for (const ExactPairBlockAntichainCudaRecord& record : records) {
    digest = digest_word(digest, record.transaction_id);
    digest = digest_word(
        digest, static_cast<std::uint64_t>(record.decision));
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(record.witness_node_count));
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(
            record.authenticated_witness_point_count));
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(
            record.exact_negative_witness_node_count));
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(record.unordered_pair_mass));
  }
  return digest;
}

struct PlannedTransaction {
  ExactPairBlockAntichainCudaTransaction transaction{};
  std::size_t authenticated_witness_point_count{};
  std::size_t predicate_begin{};
  std::size_t predicate_count{};
  bool requires_predicate_batch{false};
};

[[nodiscard]] std::uint64_t canonical_transaction_digest(
    std::span<const PlannedTransaction> plans) noexcept {
  std::uint64_t digest = digest_offset;
  digest = digest_word(digest, static_cast<std::uint64_t>(plans.size()));
  for (const PlannedTransaction& plan : plans) {
    const ExactPairBlockAntichainCudaTransaction& transaction =
        plan.transaction;
    digest = digest_word(digest, transaction.transaction_id);
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(transaction.support_block.kind));
    const auto add_node = [&digest](const ExactPairBlockNodeAuthority& node) {
      digest = digest_word(
          digest, static_cast<std::uint64_t>(node.node_index));
      digest = digest_word(
          digest, static_cast<std::uint64_t>(node.leaf_begin));
      digest = digest_word(
          digest, static_cast<std::uint64_t>(node.leaf_end));
    };
    add_node(transaction.support_block.first);
    add_node(transaction.support_block.second);
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(
            transaction.support_block.unordered_pair_mass));
    digest = digest_word(
        digest,
        static_cast<std::uint64_t>(transaction.witness_node_count));
    for (std::size_t witness_index = 0U;
         witness_index < transaction.witness_node_count;
         ++witness_index) {
      add_node(transaction.witness_nodes[witness_index]);
    }
  }
  return digest;
}

[[nodiscard]] std::uint64_t process_local_transcript_digest(
    const ExactPairBlockAntichainCudaAudit& audit) noexcept {
  // Domain-separated FNV transcript.  This detects accidental batch mixing
  // inside one process; it is deliberately not a durable cryptographic
  // identity for the cloud or the LBVH snapshot.
  std::uint64_t digest = digest_word(
      digest_offset, UINT64_C(0x4d484750414e5431));
  digest = digest_word(digest, audit.schema_version);
  digest = digest_word(digest, audit.source_snapshot_epoch);
  digest = digest_word(
      digest, static_cast<std::uint64_t>(audit.maximum_closed_rank));
  digest = digest_word(digest, audit.canonical_transaction_digest);
  digest = digest_word(digest, audit.completed_transaction_digest);
  digest = digest_word(digest, audit.predicate_task_digest);
  digest = digest_word(digest, audit.predicate_result_digest);
  digest = digest_word(
      digest,
      static_cast<std::uint64_t>(audit.submitted_unordered_pair_mass));
  digest = digest_word(
      digest, static_cast<std::uint64_t>(audit.pruned_unordered_pair_mass));
  digest = digest_word(
      digest,
      static_cast<std::uint64_t>(audit.residual_unordered_pair_mass));
  return digest;
}

[[nodiscard]] std::uint64_t repeated_transaction_recipe_digest(
    const ExactPairBlockAntichainCudaTransaction& model,
    std::size_t transaction_count,
    std::uint64_t domain) noexcept {
  const std::array<ExactPairBlockAntichainCudaTransaction, 1U> singleton{
      model};
  std::uint64_t digest = digest_word(digest_offset, domain);
  digest = digest_word(digest, transaction_digest(singleton));
  digest = digest_word(
      digest, static_cast<std::uint64_t>(transaction_count));
  digest = digest_word(digest, model.transaction_id);
  if (transaction_count != 0U) {
    digest = digest_word(
        digest,
        model.transaction_id +
            static_cast<std::uint64_t>(transaction_count - 1U));
  }
  return digest;
}

[[nodiscard]] std::uint64_t repeated_result_recipe_digest(
    const ExactPairBlockAntichainCudaRecord& first_record,
    std::size_t transaction_count) noexcept {
  const std::array<ExactPairBlockAntichainCudaRecord, 1U> singleton{
      first_record};
  std::uint64_t digest = digest_word(
      digest_offset, UINT64_C(0x414e545245535031));
  digest = digest_word(digest, result_digest(singleton));
  digest = digest_word(
      digest, static_cast<std::uint64_t>(transaction_count));
  digest = digest_word(digest, first_record.transaction_id);
  if (transaction_count != 0U) {
    digest = digest_word(
        digest,
        first_record.transaction_id +
            static_cast<std::uint64_t>(transaction_count - 1U));
  }
  return digest;
}

void canonicalize_witnesses(
    ExactPairBlockAntichainCudaTransaction& transaction) {
  const auto precedes =
      [](const ExactPairBlockNodeAuthority& first,
         const ExactPairBlockNodeAuthority& second) {
        if (first.leaf_begin != second.leaf_begin) {
          return first.leaf_begin < second.leaf_begin;
        }
        if (first.leaf_end != second.leaf_end) {
          return first.leaf_end < second.leaf_end;
        }
        return first.node_index < second.node_index;
      };
  for (std::size_t sorted_count = 1U;
       sorted_count < transaction.witness_node_count;
       ++sorted_count) {
    const ExactPairBlockNodeAuthority inserted =
        transaction.witness_nodes[sorted_count];
    std::size_t destination = sorted_count;
    while (destination != 0U &&
           precedes(
               inserted,
               transaction.witness_nodes[destination - 1U])) {
      transaction.witness_nodes[destination] =
          transaction.witness_nodes[destination - 1U];
      --destination;
    }
    transaction.witness_nodes[destination] = inserted;
  }
}

void validate_and_plan_transaction(
    PlannedTransaction& plan,
    std::size_t point_count,
    std::size_t certified_node_count,
    std::size_t required_witness_point_count) {
  ExactPairBlockAntichainCudaTransaction& transaction =
      plan.transaction;
  if (transaction.witness_node_count >
      exact_pair_block_antichain_cuda_maximum_witness_node_count) {
    throw std::out_of_range(
        "an exact pair-block antichain transaction has more than ten "
        "witness nodes");
  }
  const ExactPairBlockAuthority& support = transaction.support_block;
  if (support.kind != ExactPairBlockAuthorityKind::cross ||
      !node_shape_is_valid(
          support.first, point_count, certified_node_count) ||
      !node_shape_is_valid(
          support.second, point_count, certified_node_count) ||
      support.first.leaf_end > support.second.leaf_begin) {
    throw std::invalid_argument(
        "an exact pair-block antichain transaction requires one canonical "
        "disjoint cross support");
  }
  std::size_t expected_pair_mass = 0U;
  if (!checked_product(
          support.first.leaf_count(),
          support.second.leaf_count(),
          expected_pair_mass) ||
      expected_pair_mass != support.unordered_pair_mass) {
    throw std::invalid_argument(
        "an exact pair-block antichain transaction lost support-product "
        "mass");
  }

  canonicalize_witnesses(transaction);
  std::size_t witness_point_count = 0U;
  for (std::size_t witness_index = 0U;
       witness_index < transaction.witness_node_count;
       ++witness_index) {
    const ExactPairBlockNodeAuthority witness =
        transaction.witness_nodes[witness_index];
    if (!node_shape_is_valid(
            witness, point_count, certified_node_count)) {
      throw std::invalid_argument(
          "an exact pair-block antichain transaction names a malformed "
          "witness authority");
    }
    if (ranges_overlap(witness, support.first) ||
        ranges_overlap(witness, support.second)) {
      throw std::invalid_argument(
          "an exact pair-block antichain witness overlaps its support");
    }
    if (witness_index != 0U &&
        ranges_overlap(
            transaction.witness_nodes[witness_index - 1U],
            witness)) {
      throw std::invalid_argument(
          "an exact pair-block witness antichain contains overlapping "
          "native ranges");
    }
    if (!checked_add(
            witness_point_count,
            witness.leaf_count(),
            witness_point_count)) {
      throw std::overflow_error(
          "an exact pair-block witness antichain mass overflows size_t");
    }
  }
  plan.authenticated_witness_point_count = witness_point_count;
  plan.requires_predicate_batch =
      witness_point_count >= required_witness_point_count;
}

void accumulate_transaction_record(
    const ExactPairBlockAntichainCudaRecord& record,
    ExactPairBlockAntichainCudaAudit& audit) {
  std::size_t next_mass = 0U;
  if (!checked_add(
          audit.submitted_unordered_pair_mass,
          record.unordered_pair_mass,
          next_mass)) {
    throw std::overflow_error(
        "exact pair-block antichain submitted mass overflows size_t");
  }
  audit.submitted_unordered_pair_mass = next_mass;
  switch (record.decision) {
    case ExactPairBlockAntichainCudaDecision::certified_closed:
      ++audit.certified_transaction_count;
      if (!checked_add(
              audit.pruned_unordered_pair_mass,
              record.unordered_pair_mass,
              next_mass)) {
        throw std::overflow_error(
            "exact pair-block antichain pruned mass overflows size_t");
      }
      audit.pruned_unordered_pair_mass = next_mass;
      return;
    case ExactPairBlockAntichainCudaDecision::
        inconclusive_insufficient_witness_mass:
      ++audit.insufficient_witness_transaction_count;
      break;
    case ExactPairBlockAntichainCudaDecision::
        residual_invalid_native_authority:
      ++audit.invalid_native_authority_transaction_count;
      break;
    case ExactPairBlockAntichainCudaDecision::residual_support_overlap:
      ++audit.support_overlap_transaction_count;
      break;
    case ExactPairBlockAntichainCudaDecision::residual_nonnegative_q:
      ++audit.nonnegative_transaction_count;
      break;
    case ExactPairBlockAntichainCudaDecision::
        residual_fixed_limb_overflow:
      ++audit.fixed_limb_residual_transaction_count;
      break;
  }
  if (!checked_add(
          audit.residual_unordered_pair_mass,
          record.unordered_pair_mass,
          next_mass)) {
    throw std::overflow_error(
        "exact pair-block antichain residual mass overflows size_t");
  }
  audit.residual_unordered_pair_mass = next_mass;
}

}  // namespace

ExactPairBlockAntichainCudaResult
qualify_exact_pair_block_antichain_transactions_cuda(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    std::span<const ExactPairBlockAntichainCudaTransaction> transactions,
    ExactPairBlockAntichainCudaConfig config) {
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "an exact pair-block antichain batch requires a ready traversal "
        "lease");
  }
  if (config.maximum_closed_rank < 2U ||
      config.maximum_closed_rank >
          exact_pair_block_antichain_cuda_maximum_closed_rank) {
    throw std::out_of_range(
        "an exact pair-block antichain maximum closed rank must be in "
        "[2, 11]");
  }
  if (config.transaction_capacity_per_point == 0U ||
      config.transaction_capacity_per_point >
          exact_pair_block_antichain_cuda_maximum_transaction_capacity_per_point) {
    throw std::out_of_range(
        "an exact pair-block antichain transaction factor must be in "
        "[1, 12]");
  }
  if (transactions.empty()) {
    throw std::invalid_argument(
        "an exact pair-block antichain batch cannot be empty");
  }

  const MortonLbvhDeviceTraversalLeaseAudit source_audit =
      traversal_lease.audit();
  ExactPairBlockAntichainCudaResult result;
  ExactPairBlockAntichainCudaAudit& audit = result.audit;
  audit.point_count = source_audit.point_count;
  audit.certified_node_count = source_audit.certified_node_count;
  audit.maximum_closed_rank = config.maximum_closed_rank;
  audit.required_witness_point_count =
      config.maximum_closed_rank - 1U;
  audit.transaction_capacity_per_point =
      config.transaction_capacity_per_point;
  audit.submitted_transaction_count = transactions.size();
  audit.source_snapshot_epoch = source_audit.source_snapshot_epoch;
  audit.submitted_transaction_digest =
      transaction_digest(transactions);
  audit.fixed_linear_transaction_capacity_validated = true;

  std::size_t transaction_capacity = 0U;
  if (!checked_product(
          source_audit.point_count,
          config.transaction_capacity_per_point,
          transaction_capacity)) {
    result.status =
        ExactPairBlockAntichainCudaStatus::capacity_exhausted;
    return result;
  }
  audit.transaction_capacity = transaction_capacity;
  if (transactions.size() > transaction_capacity) {
    result.status =
        ExactPairBlockAntichainCudaStatus::capacity_exhausted;
    return result;
  }

  std::vector<std::uint64_t> transaction_ids;
  transaction_ids.reserve(transactions.size());
  for (const ExactPairBlockAntichainCudaTransaction& transaction :
       transactions) {
    transaction_ids.push_back(transaction.transaction_id);
  }
  std::sort(transaction_ids.begin(), transaction_ids.end());
  if (std::adjacent_find(
          transaction_ids.begin(), transaction_ids.end()) !=
      transaction_ids.end()) {
    throw std::invalid_argument(
        "an exact pair-block antichain batch requires unique transaction "
        "identities");
  }
  audit.unique_transaction_ids_validated = true;

  std::vector<PlannedTransaction> plans;
  plans.reserve(transactions.size());
  std::size_t predicate_task_count = 0U;
  for (const ExactPairBlockAntichainCudaTransaction& transaction :
       transactions) {
    PlannedTransaction plan;
    plan.transaction = transaction;
    validate_and_plan_transaction(
        plan,
        source_audit.point_count,
        source_audit.certified_node_count,
        audit.required_witness_point_count);
    if (!checked_add(
            audit.submitted_witness_node_count,
            plan.transaction.witness_node_count,
            audit.submitted_witness_node_count)) {
      throw std::overflow_error(
          "an exact pair-block antichain witness-node count overflows "
          "size_t");
    }
    plan.predicate_begin = predicate_task_count;
    if (plan.requires_predicate_batch) {
      plan.predicate_count = plan.transaction.witness_node_count;
      if (!checked_add(
              predicate_task_count,
              plan.predicate_count,
              predicate_task_count)) {
        throw std::overflow_error(
            "an exact pair-block antichain predicate-task count overflows "
            "size_t");
      }
    }
    plans.push_back(plan);
  }
  audit.predicate_task_count = predicate_task_count;
  audit.canonical_transaction_digest =
      canonical_transaction_digest(plans);
  audit.canonical_witness_antichains_validated = true;

  std::size_t predicate_task_capacity = 0U;
  if (!checked_product(
          source_audit.point_count,
          exact_pair_block_witness_cuda_maximum_task_capacity_per_point,
          predicate_task_capacity)) {
    result.status =
        ExactPairBlockAntichainCudaStatus::capacity_exhausted;
    return result;
  }
  audit.predicate_task_capacity = predicate_task_capacity;
  audit.fixed_linear_predicate_capacity_validated = true;
  if (source_audit.point_count >
          std::numeric_limits<std::uint32_t>::max() ||
      source_audit.certified_node_count >
          std::numeric_limits<std::uint32_t>::max() ||
      predicate_task_count > predicate_task_capacity) {
    result.status =
        ExactPairBlockAntichainCudaStatus::capacity_exhausted;
    return result;
  }
  audit.compact_index_width_validated = true;

  std::vector<ExactPairBlockWitnessCudaTask> predicate_tasks;
  predicate_tasks.reserve(predicate_task_count);
  for (const PlannedTransaction& plan : plans) {
    if (!plan.requires_predicate_batch) {
      continue;
    }
    for (std::size_t witness_index = 0U;
         witness_index < plan.transaction.witness_node_count;
         ++witness_index) {
      const std::uint64_t predicate_task_id =
          make_predicate_task_id(
              predicate_tasks.size(),
              plan.transaction.witness_nodes[witness_index].node_index);
      predicate_tasks.push_back(ExactPairBlockWitnessCudaTask{
          predicate_task_id,
          plan.transaction.support_block,
          plan.transaction.witness_nodes[witness_index]});
    }
  }
  if (predicate_tasks.size() != predicate_task_count) {
    throw std::logic_error(
        "an exact pair-block antichain plan lost predicate tasks");
  }

  result.records.resize(transactions.size());
  if (predicate_tasks.empty()) {
    for (std::size_t transaction_index = 0U;
         transaction_index < plans.size();
         ++transaction_index) {
      const PlannedTransaction& plan = plans[transaction_index];
      ExactPairBlockAntichainCudaRecord& record =
          result.records[transaction_index];
      record.transaction_id = plan.transaction.transaction_id;
      record.decision = ExactPairBlockAntichainCudaDecision::
          inconclusive_insufficient_witness_mass;
      record.witness_node_count =
          plan.transaction.witness_node_count;
      // No predicate was submitted, so the native LBVH identity was not
      // consumed.  Shape-validated host ranges are not authenticated device
      // authority and must not be reported as such.
      record.authenticated_witness_point_count = 0U;
      record.unordered_pair_mass =
          plan.transaction.support_block.unordered_pair_mass;
      accumulate_transaction_record(record, audit);
    }
    audit.completed_transaction_count = result.records.size();
    std::size_t conserved_mass = 0U;
    audit.transactional_product_mass_conservation_validated =
        checked_add(
            audit.pruned_unordered_pair_mass,
            audit.residual_unordered_pair_mass,
            conserved_mass) &&
        conserved_mass ==
            audit.submitted_unordered_pair_mass;
    audit.every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass =
        true;
    audit.completed_transaction_digest =
        result_digest(result.records);
    audit.process_local_transcript_digest =
        process_local_transcript_digest(audit);
    audit.process_local_transcript_validated = true;
    result.status =
        ExactPairBlockAntichainCudaStatus::preflight_inconclusive;
    return result;
  }

  std::size_t predicate_capacity_factor =
      predicate_tasks.size() / source_audit.point_count;
  if (predicate_tasks.size() % source_audit.point_count != 0U) {
    ++predicate_capacity_factor;
  }
  if (predicate_capacity_factor == 0U ||
      predicate_capacity_factor >
          exact_pair_block_witness_cuda_maximum_task_capacity_per_point) {
    throw std::logic_error(
        "an exact pair-block antichain plan exceeded the proved predicate "
        "batch factor");
  }
  audit.predicate_task_capacity_per_point =
      predicate_capacity_factor;
  ExactPairBlockWitnessCudaResult predicate_result =
      qualify_exact_pair_block_witnesses_cuda(
          std::move(traversal_lease),
          predicate_tasks,
          ExactPairBlockWitnessCudaConfig{
              2U, predicate_capacity_factor});
  if (predicate_result.status ==
          ExactPairBlockWitnessCudaStatus::capacity_exhausted ||
      predicate_result.records.size() != predicate_tasks.size()) {
    throw std::logic_error(
        "an exact pair-block antichain predicate batch failed after "
        "successful preflight");
  }

  const ExactPairBlockWitnessCudaAudit& predicate_audit =
      predicate_result.audit;
  audit.host_to_device_predicate_task_byte_count =
      predicate_audit.host_to_device_task_byte_count;
  audit.device_to_host_predicate_result_byte_count =
      predicate_audit.device_to_host_result_byte_count;
  audit.device_arena_allocation_count =
      predicate_audit.device_arena_allocation_count;
  audit.per_transaction_allocation_count =
      predicate_audit.per_task_allocation_count;
  audit.per_transaction_synchronization_count =
      predicate_audit.per_task_synchronization_count;
  audit.kernel_launch_count = predicate_audit.kernel_launch_count;
  audit.synchronization_count = predicate_audit.synchronization_count;
  audit.predicate_task_digest =
      predicate_audit.submitted_task_digest;
  audit.predicate_result_digest =
      predicate_audit.completed_result_digest;
  audit.predicate_kernel_elapsed_nanoseconds =
      predicate_audit.kernel_elapsed_nanoseconds;
  audit.cuda_device = predicate_audit.cuda_device;
  audit.native_lbvh_authority_consumed =
      predicate_audit.native_lbvh_authority_consumed;
  audit.native_lbvh_nodes_read_on_device =
      predicate_audit.native_lbvh_nodes_read_on_device;
  audit.host_fake_lifecycle_exercised =
      predicate_audit.host_fake_lifecycle_exercised;
  audit.cuda_execution_performed =
      predicate_audit.cuda_execution_performed;
  audit.one_batched_predicate_submission_validated =
      predicate_audit.per_task_allocation_count == 0U &&
      predicate_audit.per_task_synchronization_count == 0U &&
      predicate_audit.submitted_task_count == predicate_tasks.size() &&
      predicate_audit.completed_task_count == predicate_tasks.size();

  for (std::size_t transaction_index = 0U;
       transaction_index < plans.size();
       ++transaction_index) {
    const PlannedTransaction& plan = plans[transaction_index];
    ExactPairBlockAntichainCudaRecord& record =
        result.records[transaction_index];
    record.transaction_id = plan.transaction.transaction_id;
    record.witness_node_count = plan.transaction.witness_node_count;
    record.unordered_pair_mass =
        plan.transaction.support_block.unordered_pair_mass;
    if (!plan.requires_predicate_batch) {
      record.decision = ExactPairBlockAntichainCudaDecision::
          inconclusive_insufficient_witness_mass;
      // This transaction contributed no predicate to the adopted native
      // batch.  Keep its shape-validated mass separate from authenticated
      // witness authority.
      record.authenticated_witness_point_count = 0U;
      accumulate_transaction_record(record, audit);
      continue;
    }

    bool invalid_authority = false;
    bool support_overlap = false;
    bool fixed_limb_residual = false;
    bool nonnegative = false;
    for (std::size_t predicate_index = plan.predicate_begin;
         predicate_index <
         plan.predicate_begin + plan.predicate_count;
         ++predicate_index) {
      const ExactPairBlockWitnessCudaRecord& predicate =
          predicate_result.records[predicate_index];
      const std::uint64_t expected_task_id =
          make_predicate_task_id(
              predicate_index,
              plan.transaction
                  .witness_nodes[predicate_index - plan.predicate_begin]
                  .node_index);
      if (predicate.task_id != expected_task_id ||
          predicate_tasks[predicate_index].task_id != expected_task_id) {
        throw std::logic_error(
            "an exact pair-block antichain predicate result lost its "
            "private task identity");
      }
      switch (predicate.decision) {
        case ExactPairBlockWitnessCudaDecision::certified_closed:
          ++record.exact_negative_witness_node_count;
          ++audit.exact_negative_witness_node_count;
          break;
        case ExactPairBlockWitnessCudaDecision::
            inconclusive_nonnegative_q:
          nonnegative = true;
          ++audit.nonnegative_witness_node_count;
          break;
        case ExactPairBlockWitnessCudaDecision::
            inconclusive_insufficient_witness_mass:
          throw std::logic_error(
              "a nonempty rank-two witness node became insufficient");
        case ExactPairBlockWitnessCudaDecision::
            residual_invalid_authority:
          invalid_authority = true;
          ++audit.invalid_native_authority_witness_node_count;
          break;
        case ExactPairBlockWitnessCudaDecision::residual_support_overlap:
          support_overlap = true;
          ++audit.support_overlap_witness_node_count;
          break;
        case ExactPairBlockWitnessCudaDecision::
            residual_fixed_limb_overflow:
          fixed_limb_residual = true;
          ++audit.fixed_limb_residual_witness_node_count;
          break;
      }
    }
    if (!invalid_authority && !support_overlap) {
      record.authenticated_witness_point_count =
          plan.authenticated_witness_point_count;
    }
    if (invalid_authority) {
      record.decision = ExactPairBlockAntichainCudaDecision::
          residual_invalid_native_authority;
    } else if (support_overlap) {
      record.decision =
          ExactPairBlockAntichainCudaDecision::residual_support_overlap;
    } else if (fixed_limb_residual) {
      record.decision = ExactPairBlockAntichainCudaDecision::
          residual_fixed_limb_overflow;
    } else if (nonnegative) {
      record.decision =
          ExactPairBlockAntichainCudaDecision::residual_nonnegative_q;
    } else if (
        record.exact_negative_witness_node_count ==
            plan.transaction.witness_node_count &&
        record.authenticated_witness_point_count >=
            audit.required_witness_point_count) {
      record.decision =
          ExactPairBlockAntichainCudaDecision::certified_closed;
    } else {
      throw std::logic_error(
          "an exact pair-block antichain transaction lost a predicate "
          "classification");
    }
    accumulate_transaction_record(record, audit);
  }
  audit.predicate_result_identities_validated = true;

  audit.completed_transaction_count = result.records.size();
  std::size_t conserved_mass = 0U;
  audit.transactional_product_mass_conservation_validated =
      checked_add(
          audit.pruned_unordered_pair_mass,
          audit.residual_unordered_pair_mass,
          conserved_mass) &&
      conserved_mass ==
          audit.submitted_unordered_pair_mass;
  audit.every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass =
      true;
  for (const ExactPairBlockAntichainCudaRecord& record :
       result.records) {
    if (record.decision ==
            ExactPairBlockAntichainCudaDecision::certified_closed &&
        (record.authenticated_witness_point_count <
             audit.required_witness_point_count ||
         record.exact_negative_witness_node_count !=
             record.witness_node_count)) {
      audit.every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass =
          false;
      break;
    }
  }
  audit.completed_transaction_digest =
      result_digest(result.records);
  audit.process_local_transcript_digest =
      process_local_transcript_digest(audit);
  audit.process_local_transcript_validated = true;
  if (!audit.transactional_product_mass_conservation_validated ||
      !audit.every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass ||
      !audit.one_batched_predicate_submission_validated ||
      !audit.fixed_linear_predicate_capacity_validated ||
      !audit.compact_index_width_validated ||
      !audit.unique_transaction_ids_validated ||
      !audit.predicate_result_identities_validated ||
      !audit.process_local_transcript_validated ||
      audit.canonical_transaction_digest == 0U ||
      audit.process_local_transcript_digest == 0U ||
      audit.per_transaction_allocation_count != 0U ||
      audit.per_transaction_synchronization_count != 0U ||
      audit.pairwise_disjoint_support_products_validated ||
      audit.double_buffered_transactional_frontier_claimed ||
      audit.global_pair_coverage_closed ||
      audit.pair_catalog_complete_claimed ||
      audit.hierarchy_or_tree_claimed || audit.slo_claimed ||
      audit.global_pair_matrix_materialized ||
      audit.ordinary_or_higher_order_delaunay_materialized ||
      audit.public_status_claimed || audit.durable_receipt_claimed) {
    throw std::logic_error(
        "an exact pair-block antichain receipt violated its bounded "
        "transaction contract");
  }
  result.status =
      predicate_result.status ==
              ExactPairBlockWitnessCudaStatus::
                  non_authoritative_host_fake
          ? ExactPairBlockAntichainCudaStatus::
                non_authoritative_host_fake
          : ExactPairBlockAntichainCudaStatus::
                qualified_component_batch;
  return result;
}

ExactPairBlockAntichainCudaRepeatedResult
qualify_repeated_exact_pair_block_antichain_transaction_cuda(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    ExactPairBlockAntichainCudaTransaction model,
    std::size_t transaction_count,
    ExactPairBlockAntichainCudaConfig config) {
  if (!traversal_lease.ready()) {
    throw std::invalid_argument(
        "a repeated exact pair-block antichain batch requires a ready "
        "traversal lease");
  }
  if (config.maximum_closed_rank < 2U ||
      config.maximum_closed_rank >
          exact_pair_block_antichain_cuda_maximum_closed_rank) {
    throw std::out_of_range(
        "a repeated exact pair-block antichain maximum closed rank must be "
        "in [2, 11]");
  }
  if (config.transaction_capacity_per_point == 0U ||
      config.transaction_capacity_per_point >
          exact_pair_block_antichain_cuda_maximum_transaction_capacity_per_point) {
    throw std::out_of_range(
        "a repeated exact pair-block antichain transaction factor must be in "
        "[1, 12]");
  }
  if (transaction_count == 0U) {
    throw std::invalid_argument(
        "a repeated exact pair-block antichain recipe cannot be empty");
  }
  if (transaction_count - 1U >
      static_cast<std::size_t>(
          std::numeric_limits<std::uint64_t>::max() -
          model.transaction_id)) {
    throw std::overflow_error(
        "the repeated exact pair-block antichain transaction identities "
        "overflow uint64");
  }

  const MortonLbvhDeviceTraversalLeaseAudit source_audit =
      traversal_lease.audit();
  ExactPairBlockAntichainCudaRepeatedResult result;
  ExactPairBlockAntichainCudaAudit& audit = result.audit;
  audit.point_count = source_audit.point_count;
  audit.certified_node_count = source_audit.certified_node_count;
  audit.maximum_closed_rank = config.maximum_closed_rank;
  audit.required_witness_point_count = config.maximum_closed_rank - 1U;
  audit.transaction_capacity_per_point =
      config.transaction_capacity_per_point;
  audit.submitted_transaction_count = transaction_count;
  audit.first_transaction_id = model.transaction_id;
  audit.last_transaction_id = model.transaction_id +
      static_cast<std::uint64_t>(transaction_count - 1U);
  audit.source_snapshot_epoch = source_audit.source_snapshot_epoch;
  audit.submitted_transaction_digest = repeated_transaction_recipe_digest(
      model, transaction_count, UINT64_C(0x414e545355425031));
  audit.fixed_linear_transaction_capacity_validated = true;
  audit.affine_transaction_id_range_validated = true;
  audit.unique_transaction_ids_validated = true;

  std::size_t transaction_capacity = 0U;
  if (!checked_product(
          source_audit.point_count,
          config.transaction_capacity_per_point,
          transaction_capacity)) {
    result.status = ExactPairBlockAntichainCudaStatus::capacity_exhausted;
    return result;
  }
  audit.transaction_capacity = transaction_capacity;
  if (transaction_count > transaction_capacity) {
    result.status = ExactPairBlockAntichainCudaStatus::capacity_exhausted;
    return result;
  }

  PlannedTransaction plan;
  plan.transaction = model;
  validate_and_plan_transaction(
      plan,
      source_audit.point_count,
      source_audit.certified_node_count,
      audit.required_witness_point_count);
  audit.canonical_transaction_digest = repeated_transaction_recipe_digest(
      plan.transaction,
      transaction_count,
      UINT64_C(0x414e5443414e5031));
  audit.canonical_witness_antichains_validated = true;
  audit.compact_repeated_transaction_recipe_validated = true;
  audit.per_transaction_host_materialization_avoided = true;
  audit.predicate_pattern_task_count =
      plan.requires_predicate_batch ? plan.transaction.witness_node_count : 0U;
  if (!checked_product(
          plan.transaction.witness_node_count,
          transaction_count,
          audit.submitted_witness_node_count)) {
    throw std::overflow_error(
        "the repeated exact pair-block antichain witness count overflows");
  }
  audit.predicate_task_count = plan.requires_predicate_batch
      ? audit.submitted_witness_node_count
      : 0U;
  audit.physical_predicate_task_count = audit.predicate_task_count;

  std::size_t predicate_capacity = 0U;
  if (!checked_product(
          source_audit.point_count,
          exact_pair_block_witness_cuda_maximum_task_capacity_per_point,
          predicate_capacity)) {
    result.status = ExactPairBlockAntichainCudaStatus::capacity_exhausted;
    return result;
  }
  audit.predicate_task_capacity = predicate_capacity;
  audit.fixed_linear_predicate_capacity_validated = true;
  if (source_audit.point_count >
          std::numeric_limits<std::uint32_t>::max() ||
      source_audit.certified_node_count >
          std::numeric_limits<std::uint32_t>::max() ||
      audit.predicate_task_count > predicate_capacity) {
    result.status = ExactPairBlockAntichainCudaStatus::capacity_exhausted;
    return result;
  }
  audit.compact_index_width_validated = true;

  const auto finish_mass = [&audit, transaction_count](
                               ExactPairBlockAntichainCudaRecord record) {
    std::size_t submitted_mass = 0U;
    if (!checked_product(
            record.unordered_pair_mass,
            transaction_count,
            submitted_mass)) {
      throw std::overflow_error(
          "the repeated exact pair-block antichain support mass overflows");
    }
    audit.submitted_unordered_pair_mass = submitted_mass;
    switch (record.decision) {
      case ExactPairBlockAntichainCudaDecision::certified_closed:
        audit.certified_transaction_count = transaction_count;
        audit.pruned_unordered_pair_mass = submitted_mass;
        break;
      case ExactPairBlockAntichainCudaDecision::
          inconclusive_insufficient_witness_mass:
        audit.insufficient_witness_transaction_count = transaction_count;
        audit.residual_unordered_pair_mass = submitted_mass;
        break;
      case ExactPairBlockAntichainCudaDecision::
          residual_invalid_native_authority:
        audit.invalid_native_authority_transaction_count = transaction_count;
        audit.residual_unordered_pair_mass = submitted_mass;
        break;
      case ExactPairBlockAntichainCudaDecision::residual_support_overlap:
        audit.support_overlap_transaction_count = transaction_count;
        audit.residual_unordered_pair_mass = submitted_mass;
        break;
      case ExactPairBlockAntichainCudaDecision::residual_nonnegative_q:
        audit.nonnegative_transaction_count = transaction_count;
        audit.residual_unordered_pair_mass = submitted_mass;
        break;
      case ExactPairBlockAntichainCudaDecision::
          residual_fixed_limb_overflow:
        audit.fixed_limb_residual_transaction_count = transaction_count;
        audit.residual_unordered_pair_mass = submitted_mass;
        break;
    }
  };

  ExactPairBlockAntichainCudaRecord first_record;
  first_record.transaction_id = plan.transaction.transaction_id;
  first_record.witness_node_count = plan.transaction.witness_node_count;
  first_record.unordered_pair_mass =
      plan.transaction.support_block.unordered_pair_mass;
  if (!plan.requires_predicate_batch) {
    first_record.decision = ExactPairBlockAntichainCudaDecision::
        inconclusive_insufficient_witness_mass;
    finish_mass(first_record);
    audit.completed_transaction_count = transaction_count;
    audit.every_logical_transaction_represented_once = true;
    audit.transactional_product_mass_conservation_validated = true;
    audit.every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass =
        true;
    audit.completed_transaction_digest = repeated_result_recipe_digest(
        first_record, transaction_count);
    audit.process_local_transcript_digest =
        process_local_transcript_digest(audit);
    audit.process_local_transcript_validated = true;
    result.first_record = first_record;
    result.status = ExactPairBlockAntichainCudaStatus::preflight_inconclusive;
    return result;
  }

  std::vector<ExactPairBlockWitnessCudaTask> pattern;
  pattern.reserve(plan.transaction.witness_node_count);
  for (std::size_t witness_index = 0U;
       witness_index < plan.transaction.witness_node_count;
       ++witness_index) {
    pattern.push_back(ExactPairBlockWitnessCudaTask{
        make_predicate_task_id(
            witness_index,
            plan.transaction.witness_nodes[witness_index].node_index),
        plan.transaction.support_block,
        plan.transaction.witness_nodes[witness_index]});
  }
  std::size_t predicate_capacity_factor =
      audit.predicate_task_count / source_audit.point_count;
  if (audit.predicate_task_count % source_audit.point_count != 0U) {
    ++predicate_capacity_factor;
  }
  audit.predicate_task_capacity_per_point = predicate_capacity_factor;
  ExactPairBlockWitnessCudaRepeatedResult predicate_result =
      qualify_repeated_exact_pair_block_witness_pattern_cuda(
          std::move(traversal_lease),
          pattern,
          transaction_count,
          0U,
          ExactPairBlockWitnessCudaConfig{2U, predicate_capacity_factor});
  if (predicate_result.status ==
          ExactPairBlockWitnessCudaStatus::capacity_exhausted ||
      predicate_result.pattern_records.size() != pattern.size()) {
    throw std::logic_error(
        "the repeated exact pair-block antichain predicate expansion failed "
        "after successful preflight");
  }
  const ExactPairBlockWitnessCudaRepeatedAudit& predicate_audit =
      predicate_result.audit;
  audit.host_to_device_predicate_task_byte_count =
      predicate_audit.host_to_device_pattern_byte_count;
  audit.device_to_host_predicate_result_byte_count =
      predicate_audit.device_to_host_outcome_byte_count;
  audit.device_arena_allocation_count =
      predicate_audit.device_arena_allocation_count;
  audit.kernel_launch_count = predicate_audit.kernel_launch_count;
  audit.synchronization_count = predicate_audit.synchronization_count;
  audit.predicate_task_digest = predicate_audit.repeated_task_recipe_digest;
  audit.predicate_result_digest = predicate_audit.expanded_outcome_digest;
  audit.predicate_kernel_elapsed_nanoseconds =
      predicate_audit.kernel_elapsed_nanoseconds;
  audit.cuda_device = predicate_audit.cuda_device;
  audit.native_lbvh_authority_consumed =
      predicate_audit.native_lbvh_authority_consumed;
  audit.native_lbvh_nodes_read_on_device =
      predicate_audit.native_lbvh_nodes_read_on_device;
  audit.host_fake_lifecycle_exercised =
      predicate_audit.host_fake_lifecycle_exercised;
  audit.cuda_execution_performed = predicate_audit.cuda_execution_performed;
  audit.one_batched_predicate_submission_validated =
      predicate_audit.repeated_pattern_expansion_validated &&
      predicate_audit.every_logical_task_classified_once &&
      predicate_audit.pattern_results_repeated_exactly;
  audit.physical_repeated_predicate_expansion_performed =
      predicate_audit.cuda_execution_performed &&
      predicate_audit.logical_task_count == audit.predicate_task_count;
  audit.predicate_result_identities_validated =
      predicate_audit.affine_task_identity_range_validated;

  bool invalid_authority = false;
  bool support_overlap = false;
  bool fixed_limb_residual = false;
  bool nonnegative = false;
  for (const ExactPairBlockWitnessCudaRecord& predicate :
       predicate_result.pattern_records) {
    switch (predicate.decision) {
      case ExactPairBlockWitnessCudaDecision::certified_closed:
        ++first_record.exact_negative_witness_node_count;
        break;
      case ExactPairBlockWitnessCudaDecision::inconclusive_nonnegative_q:
        nonnegative = true;
        break;
      case ExactPairBlockWitnessCudaDecision::
          inconclusive_insufficient_witness_mass:
        throw std::logic_error(
            "a repeated nonempty rank-two witness became insufficient");
      case ExactPairBlockWitnessCudaDecision::residual_invalid_authority:
        invalid_authority = true;
        break;
      case ExactPairBlockWitnessCudaDecision::residual_support_overlap:
        support_overlap = true;
        break;
      case ExactPairBlockWitnessCudaDecision::
          residual_fixed_limb_overflow:
        fixed_limb_residual = true;
        break;
    }
  }
  if (!invalid_authority && !support_overlap) {
    first_record.authenticated_witness_point_count =
        plan.authenticated_witness_point_count;
  }
  if (invalid_authority) {
    first_record.decision = ExactPairBlockAntichainCudaDecision::
        residual_invalid_native_authority;
  } else if (support_overlap) {
    first_record.decision =
        ExactPairBlockAntichainCudaDecision::residual_support_overlap;
  } else if (fixed_limb_residual) {
    first_record.decision = ExactPairBlockAntichainCudaDecision::
        residual_fixed_limb_overflow;
  } else if (nonnegative) {
    first_record.decision =
        ExactPairBlockAntichainCudaDecision::residual_nonnegative_q;
  } else if (
      first_record.exact_negative_witness_node_count ==
          plan.transaction.witness_node_count &&
      first_record.authenticated_witness_point_count >=
          audit.required_witness_point_count) {
    first_record.decision =
        ExactPairBlockAntichainCudaDecision::certified_closed;
  } else {
    throw std::logic_error(
        "the repeated exact pair-block antichain recipe lost a predicate "
        "classification");
  }
  if (!checked_product(
          first_record.exact_negative_witness_node_count,
          transaction_count,
          audit.exact_negative_witness_node_count) ||
      !checked_product(
          predicate_audit.nonnegative_task_count /
              transaction_count,
          transaction_count,
          audit.nonnegative_witness_node_count) ||
      !checked_product(
          predicate_audit.invalid_authority_task_count /
              transaction_count,
          transaction_count,
          audit.invalid_native_authority_witness_node_count) ||
      !checked_product(
          predicate_audit.support_overlap_task_count /
              transaction_count,
          transaction_count,
          audit.support_overlap_witness_node_count) ||
      !checked_product(
          predicate_audit.fixed_limb_residual_count /
              transaction_count,
          transaction_count,
          audit.fixed_limb_residual_witness_node_count)) {
    throw std::overflow_error(
        "the repeated exact pair-block antichain outcome counts overflow");
  }
  finish_mass(first_record);
  audit.completed_transaction_count = transaction_count;
  audit.every_logical_transaction_represented_once = true;
  std::size_t conserved_mass = 0U;
  audit.transactional_product_mass_conservation_validated =
      checked_add(
          audit.pruned_unordered_pair_mass,
          audit.residual_unordered_pair_mass,
          conserved_mass) &&
      conserved_mass == audit.submitted_unordered_pair_mass;
  audit.every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass =
      first_record.decision !=
          ExactPairBlockAntichainCudaDecision::certified_closed ||
      (first_record.authenticated_witness_point_count >=
           audit.required_witness_point_count &&
       first_record.exact_negative_witness_node_count ==
           first_record.witness_node_count);
  audit.completed_transaction_digest = repeated_result_recipe_digest(
      first_record, transaction_count);
  audit.process_local_transcript_digest =
      process_local_transcript_digest(audit);
  audit.process_local_transcript_validated = true;
  if (!audit.compact_repeated_transaction_recipe_validated ||
      !audit.affine_transaction_id_range_validated ||
      !audit.every_logical_transaction_represented_once ||
      !audit.per_transaction_host_materialization_avoided ||
      !audit.transactional_product_mass_conservation_validated ||
      !audit.every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass ||
      !audit.one_batched_predicate_submission_validated ||
      !audit.fixed_linear_transaction_capacity_validated ||
      !audit.fixed_linear_predicate_capacity_validated ||
      !audit.compact_index_width_validated ||
      !audit.unique_transaction_ids_validated ||
      !audit.predicate_result_identities_validated ||
      !audit.process_local_transcript_validated ||
      audit.submitted_transaction_digest == 0U ||
      audit.canonical_transaction_digest == 0U ||
      audit.completed_transaction_digest == 0U ||
      audit.predicate_task_digest == 0U ||
      audit.predicate_result_digest == 0U ||
      audit.process_local_transcript_digest == 0U ||
      audit.per_transaction_allocation_count != 0U ||
      audit.per_transaction_synchronization_count != 0U ||
      audit.pairwise_disjoint_support_products_validated ||
      audit.double_buffered_transactional_frontier_claimed ||
      audit.global_pair_coverage_closed ||
      audit.pair_catalog_complete_claimed ||
      audit.hierarchy_or_tree_claimed || audit.slo_claimed ||
      audit.global_pair_matrix_materialized ||
      audit.ordinary_or_higher_order_delaunay_materialized ||
      audit.public_status_claimed || audit.durable_receipt_claimed) {
    throw std::logic_error(
        "the repeated exact pair-block antichain receipt violated its "
        "compact transaction recipe");
  }
  result.status =
      predicate_result.status ==
              ExactPairBlockWitnessCudaStatus::non_authoritative_host_fake
          ? ExactPairBlockAntichainCudaStatus::non_authoritative_host_fake
          : ExactPairBlockAntichainCudaStatus::qualified_component_batch;
  result.first_record = first_record;
  return result;
}

}  // namespace morsehgp3d::gpu
