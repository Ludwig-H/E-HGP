#include "fake_gpu_phase14_morton_lbvh_build_launchers.hpp"

#include "morsehgp3d/gpu/exact_pair_block_antichain_cuda.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::ExactPairBlockAntichainCudaConfig;
using morsehgp3d::gpu::ExactPairBlockAntichainCudaDecision;
using morsehgp3d::gpu::ExactPairBlockAntichainCudaStatus;
using morsehgp3d::gpu::ExactPairBlockAntichainCudaTransaction;
using morsehgp3d::gpu::ExactPairBlockAuthority;
using morsehgp3d::gpu::ExactPairBlockAuthorityKind;
using morsehgp3d::gpu::ExactPairBlockNodeAuthority;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::spatial::CanonicalPointCloud;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud line_cloud(std::size_t count) {
  std::vector<CertifiedPoint3> points;
  points.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const double coordinate = static_cast<double>(index);
    points.push_back(point(
        coordinate, coordinate / 16.0, coordinate / 256.0));
  }
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] MortonLbvhDeviceTraversalLease traversal_lease(
    std::size_t point_count) {
  morsehgp3d::gpu::test_support::
      reset_fake_gpu_phase14_morton_lbvh_build();
  const CanonicalPointCloud cloud = line_cloud(point_count);
  MortonLbvhBuildContext builder{point_count + 2U};
  auto build = builder.build(cloud);
  return builder.release_device_traversal_lease(build);
}

[[nodiscard]] ExactPairBlockNodeAuthority node(
    std::size_t node_index,
    std::size_t leaf_begin,
    std::size_t leaf_end) {
  return ExactPairBlockNodeAuthority{
      node_index, leaf_begin, leaf_end};
}

[[nodiscard]] ExactPairBlockAntichainCudaTransaction transaction(
    std::uint64_t transaction_id,
    ExactPairBlockNodeAuthority first,
    ExactPairBlockNodeAuthority second,
    std::span<const ExactPairBlockNodeAuthority> witnesses) {
  ExactPairBlockAntichainCudaTransaction result;
  result.transaction_id = transaction_id;
  result.support_block = ExactPairBlockAuthority{
      ExactPairBlockAuthorityKind::cross,
      first,
      second,
      first.leaf_count() * second.leaf_count()};
  result.witness_node_count = witnesses.size();
  for (std::size_t index = 0U; index < witnesses.size(); ++index) {
    result.witness_nodes[index] = witnesses[index];
  }
  return result;
}

void test_atomic_multi_node_transactions() {
  const std::array<ExactPairBlockNodeAuthority, 5U> first_witnesses{
      node(3U, 2U, 3U),
      node(6U, 3U, 4U),
      node(9U, 4U, 5U),
      node(12U, 5U, 6U),
      node(15U, 6U, 7U)};
  const std::array<ExactPairBlockNodeAuthority, 5U> second_witnesses{
      node(19U, 9U, 10U),
      node(22U, 10U, 11U),
      node(25U, 11U, 12U),
      node(28U, 12U, 13U),
      node(31U, 13U, 14U)};
  const std::array<ExactPairBlockNodeAuthority, 5U> third_witnesses{
      node(32U, 16U, 17U),
      node(35U, 17U, 18U),
      node(38U, 18U, 19U),
      node(41U, 19U, 20U),
      node(44U, 20U, 21U)};
  // Deliberately reversed: the compositor must canonicalize this antichain
  // before flattening it into the exact predicate batch.
  const std::array<ExactPairBlockNodeAuthority, 2U> fourth_witnesses{
      node(48U, 27U, 30U),
      node(45U, 25U, 27U)};

  const std::array<ExactPairBlockAntichainCudaTransaction, 4U>
      transactions{
          transaction(
              0U,
              node(0U, 0U, 1U),
              node(1U, 1U, 2U),
              first_witnesses),
          transaction(
              1U,
              node(7U, 7U, 8U),
              node(8U, 8U, 9U),
              second_witnesses),
          transaction(
              2U,
              node(14U, 14U, 15U),
              node(15U, 15U, 16U),
              third_witnesses),
          transaction(
              3U,
              node(21U, 21U, 23U),
              node(22U, 23U, 25U),
              fourth_witnesses)};

  auto result =
      morsehgp3d::gpu::
          qualify_exact_pair_block_antichain_transactions_cuda(
              traversal_lease(32U),
              transactions,
              ExactPairBlockAntichainCudaConfig{6U, 1U});
  check(
      result.status ==
          ExactPairBlockAntichainCudaStatus::
              non_authoritative_host_fake,
      "the host/fake antichain compositor must remain non-authoritative");
  check(
      result.records.size() == 4U &&
          result.audit.completed_transaction_count == 4U &&
          result.audit.predicate_task_count == 17U,
      "one predicate batch must return every transaction atomically");
  check(
      result.records[0].decision ==
              ExactPairBlockAntichainCudaDecision::certified_closed &&
          result.records[0].exact_negative_witness_node_count == 5U &&
          result.records[0].authenticated_witness_point_count == 5U,
      "five exact-negative singleton witnesses must close rank six");
  check(
      result.records[1].decision ==
              ExactPairBlockAntichainCudaDecision::
                  residual_nonnegative_q &&
          result.records[1].exact_negative_witness_node_count == 0U,
      "one nonnegative antichain must retain the whole support product");
  check(
      result.records[2].decision ==
          ExactPairBlockAntichainCudaDecision::
              residual_fixed_limb_overflow,
      "a fixed-limb residual must retain the whole support product");
  check(
      result.records[3].decision ==
              ExactPairBlockAntichainCudaDecision::certified_closed &&
          result.records[3].witness_node_count == 2U &&
          result.records[3].authenticated_witness_point_count == 5U &&
          result.records[3].exact_negative_witness_node_count == 2U &&
          result.records[3].unordered_pair_mass == 4U,
      "two canonicalized multi-leaf nodes must close one support product");
  check(
      result.audit.submitted_unordered_pair_mass == 7U &&
          result.audit.pruned_unordered_pair_mass == 5U &&
          result.audit.residual_unordered_pair_mass == 2U &&
          result.audit.certified_transaction_count == 2U &&
          result.audit.nonnegative_transaction_count == 1U &&
          result.audit.fixed_limb_residual_transaction_count == 1U,
      "support-product mass must be counted once per transaction");
  check(
      result.audit.exact_negative_witness_node_count == 7U &&
          result.audit.nonnegative_witness_node_count == 5U &&
          result.audit.fixed_limb_residual_witness_node_count == 5U,
      "witness-node decisions must partition the flattened predicate batch");
  check(
      result.audit.canonical_witness_antichains_validated &&
          result.audit.transactional_product_mass_conservation_validated &&
          result.audit.every_certified_transaction_has_sufficient_disjoint_exact_negative_witness_mass &&
          result.audit.one_batched_predicate_submission_validated &&
          result.audit.per_transaction_allocation_count == 0U &&
          result.audit.per_transaction_synchronization_count == 0U,
      "the atomic transaction proof gates must be explicit");
  check(
      result.audit.submitted_transaction_digest != 0U &&
          result.audit.canonical_transaction_digest != 0U &&
          result.audit.completed_transaction_digest != 0U &&
          result.audit.predicate_task_digest != 0U &&
          result.audit.predicate_result_digest != 0U &&
          result.audit.process_local_transcript_digest != 0U &&
          result.audit.submitted_transaction_digest !=
              result.audit.canonical_transaction_digest,
      "transaction and predicate transcripts must carry stable digests");
  check(
      result.audit.predicate_task_capacity == 64U * 32U &&
          result.audit.fixed_linear_predicate_capacity_validated &&
          result.audit.compact_index_width_validated &&
          result.audit.unique_transaction_ids_validated &&
          result.audit.predicate_result_identities_validated &&
          result.audit.process_local_transcript_validated &&
          !result.audit.durable_receipt_claimed,
      "predicate capacity and transcript identities must be preflighted");
  check(
      !result.audit.pairwise_disjoint_support_products_validated &&
          !result.audit.double_buffered_transactional_frontier_claimed &&
          !result.audit.global_pair_coverage_closed &&
          !result.audit.pair_catalog_complete_claimed &&
          !result.audit.hierarchy_or_tree_claimed &&
          !result.audit.slo_claimed &&
          !result.audit.global_pair_matrix_materialized &&
          !result.audit.ordinary_or_higher_order_delaunay_materialized &&
          !result.audit.public_status_claimed,
      "the bounded compositor must not claim a global frontier or product");
}

void test_last_witness_failure_rolls_back_whole_product() {
  const std::array<ExactPairBlockNodeAuthority, 5U> witnesses{
      node(3U, 2U, 3U),
      node(6U, 3U, 4U),
      node(9U, 4U, 5U),
      node(12U, 5U, 6U),
      node(13U, 6U, 7U)};
  const std::array<ExactPairBlockAntichainCudaTransaction, 1U>
      transactions{transaction(
          99U,
          node(0U, 0U, 1U),
          node(1U, 1U, 2U),
          witnesses)};
  auto result =
      morsehgp3d::gpu::
          qualify_exact_pair_block_antichain_transactions_cuda(
              traversal_lease(16U),
              transactions,
              ExactPairBlockAntichainCudaConfig{6U, 1U});
  check(
      result.records[0].decision ==
              ExactPairBlockAntichainCudaDecision::
                  residual_nonnegative_q &&
          result.records[0].exact_negative_witness_node_count == 4U &&
          result.audit.certified_transaction_count == 0U &&
          result.audit.pruned_unordered_pair_mass == 0U &&
          result.audit.residual_unordered_pair_mass == 1U,
      "a nonnegative last witness must roll back the complete support "
      "product after four exact-negative witnesses");
}

void test_insufficient_preflight_preserves_lease() {
  const std::array<ExactPairBlockNodeAuthority, 4U> witnesses{
      node(2U, 2U, 3U),
      node(3U, 3U, 4U),
      node(4U, 4U, 5U),
      node(5U, 5U, 6U)};
  const std::array<ExactPairBlockAntichainCudaTransaction, 1U>
      transactions{transaction(
          0U,
          node(0U, 0U, 1U),
          node(1U, 1U, 2U),
          witnesses)};
  auto lease = traversal_lease(8U);
  auto result =
      morsehgp3d::gpu::
          qualify_exact_pair_block_antichain_transactions_cuda(
              std::move(lease),
              transactions,
              ExactPairBlockAntichainCudaConfig{6U, 1U});
  check(
      result.status ==
              ExactPairBlockAntichainCudaStatus::preflight_inconclusive &&
          result.records[0].decision ==
              ExactPairBlockAntichainCudaDecision::
                  inconclusive_insufficient_witness_mass &&
          result.records[0].authenticated_witness_point_count == 0U &&
          result.audit.predicate_task_count == 0U &&
          !result.audit.native_lbvh_authority_consumed,
      "four shape-validated singleton witnesses must remain unauthenticated "
      "and insufficient without Q work");
  check(
      result.audit.submitted_unordered_pair_mass == 1U &&
          result.audit.pruned_unordered_pair_mass == 0U &&
          result.audit.residual_unordered_pair_mass == 1U &&
          result.audit.transactional_product_mass_conservation_validated,
      "an insufficient transaction must retain its complete pair mass");
  check(
      lease.ready(),
      "an all-insufficient preflight must leave the traversal lease reusable");
}

void test_invalid_preflight_and_capacity_preserve_lease() {
  const std::array<ExactPairBlockNodeAuthority, 2U> overlapping{
      node(2U, 2U, 4U),
      node(3U, 3U, 5U)};
  const std::array<ExactPairBlockAntichainCudaTransaction, 1U>
      invalid_transactions{transaction(
          0U,
          node(0U, 0U, 1U),
          node(1U, 1U, 2U),
          overlapping)};
  auto lease = traversal_lease(8U);
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(
            morsehgp3d::gpu::
                qualify_exact_pair_block_antichain_transactions_cuda(
                    std::move(lease),
                    invalid_transactions,
                    ExactPairBlockAntichainCudaConfig{3U, 1U}));
      },
      "overlapping witness nodes must fail before predicate submission");
  check(
      lease.ready(),
      "an invalid antichain preflight must preserve traversal authority");

  std::vector<ExactPairBlockAntichainCudaTransaction> too_many(
      9U, invalid_transactions[0]);
  auto capacity = morsehgp3d::gpu::
      qualify_exact_pair_block_antichain_transactions_cuda(
          std::move(lease),
          too_many,
          ExactPairBlockAntichainCudaConfig{3U, 1U});
  check(
      capacity.status ==
              ExactPairBlockAntichainCudaStatus::capacity_exhausted &&
          capacity.audit.transaction_capacity == 8U &&
          !capacity.audit.native_lbvh_authority_consumed,
      "linear transaction capacity must fail before input traversal");
  check(
      lease.ready(),
      "a transaction-capacity refusal must preserve traversal authority");

  check_throws<std::out_of_range>(
      [&]() {
        static_cast<void>(
            morsehgp3d::gpu::
                qualify_exact_pair_block_antichain_transactions_cuda(
                    std::move(lease),
                    invalid_transactions,
                    ExactPairBlockAntichainCudaConfig{12U, 1U}));
      },
      "rank greater than eleven must be rejected");
}

void test_rank_eleven_and_predicate_capacity_gate() {
  const std::array<ExactPairBlockNodeAuthority, 10U> witnesses{
      node(3U, 2U, 3U),
      node(6U, 3U, 4U),
      node(9U, 4U, 5U),
      node(12U, 5U, 6U),
      node(15U, 6U, 7U),
      node(18U, 7U, 8U),
      node(21U, 8U, 9U),
      node(24U, 9U, 10U),
      node(27U, 10U, 11U),
      node(30U, 11U, 12U)};
  const std::array<ExactPairBlockAntichainCudaTransaction, 1U>
      rank_eleven{transaction(
          110U,
          node(0U, 0U, 1U),
          node(1U, 1U, 2U),
          witnesses)};
  auto closed = morsehgp3d::gpu::
      qualify_exact_pair_block_antichain_transactions_cuda(
          traversal_lease(16U),
          rank_eleven,
          ExactPairBlockAntichainCudaConfig{11U, 1U});
  check(
      closed.records.size() == 1U &&
          closed.records[0].decision ==
              ExactPairBlockAntichainCudaDecision::certified_closed &&
          closed.records[0].authenticated_witness_point_count == 10U &&
          closed.records[0].exact_negative_witness_node_count == 10U &&
          closed.audit.required_witness_point_count == 10U &&
          closed.audit.predicate_task_count == 10U,
      "ten exact-negative singleton witnesses must close rank eleven");

  std::vector<ExactPairBlockAntichainCudaTransaction> too_many_predicates;
  too_many_predicates.reserve(103U);
  for (std::size_t index = 0U; index < 103U; ++index) {
    too_many_predicates.push_back(transaction(
        static_cast<std::uint64_t>(index),
        node(0U, 0U, 1U),
        node(1U, 1U, 2U),
        witnesses));
  }
  auto lease = traversal_lease(16U);
  auto capacity = morsehgp3d::gpu::
      qualify_exact_pair_block_antichain_transactions_cuda(
          std::move(lease),
          too_many_predicates,
          ExactPairBlockAntichainCudaConfig{11U, 12U});
  check(
      capacity.status ==
              ExactPairBlockAntichainCudaStatus::capacity_exhausted &&
          capacity.audit.transaction_capacity == 192U &&
          capacity.audit.predicate_task_count == 1030U &&
          capacity.audit.predicate_task_capacity == 1024U &&
          capacity.audit.fixed_linear_predicate_capacity_validated &&
          !capacity.audit.native_lbvh_authority_consumed,
      "rank-eleven predicate overflow must fail at the explicit 64n gate");
  check(
      lease.ready(),
      "predicate-capacity refusal must preserve traversal authority");

  const std::array<ExactPairBlockAntichainCudaTransaction, 2U> duplicate_ids{
      rank_eleven[0], rank_eleven[0]};
  check_throws<std::invalid_argument>(
      [&]() {
        static_cast<void>(morsehgp3d::gpu::
            qualify_exact_pair_block_antichain_transactions_cuda(
                std::move(lease),
                duplicate_ids,
                ExactPairBlockAntichainCudaConfig{11U, 1U}));
      },
      "duplicate transaction identities must be rejected before CUDA");
  check(
      lease.ready(),
      "duplicate transaction identities must preserve traversal authority");
}

void test_compact_repeated_transaction_recipe() {
  const std::array<ExactPairBlockNodeAuthority, 5U> witnesses{
      node(3U, 2U, 3U),
      node(6U, 3U, 4U),
      node(9U, 4U, 5U),
      node(12U, 5U, 6U),
      node(15U, 6U, 7U)};
  auto model = transaction(
      500U,
      node(0U, 0U, 1U),
      node(1U, 1U, 2U),
      witnesses);
  constexpr std::size_t repetition_count = 1000U;
  auto result = morsehgp3d::gpu::
      qualify_repeated_exact_pair_block_antichain_transaction_cuda(
          traversal_lease(128U),
          model,
          repetition_count,
          ExactPairBlockAntichainCudaConfig{6U, 12U});
  check(
      result.status ==
              ExactPairBlockAntichainCudaStatus::
                  non_authoritative_host_fake &&
          !result.qualified_cuda_batch() &&
          result.first_record.decision ==
              ExactPairBlockAntichainCudaDecision::certified_closed &&
          result.first_record.exact_negative_witness_node_count == 5U &&
          result.contains_transaction_id(500U) &&
          result.contains_transaction_id(1499U) &&
          !result.contains_transaction_id(1500U),
      "the compact repeated recipe must represent one affine transaction-id "
      "range without claiming native CUDA");
  const auto& audit = result.audit;
  check(
      audit.submitted_transaction_count == repetition_count &&
          audit.completed_transaction_count == repetition_count &&
          audit.certified_transaction_count == repetition_count &&
          audit.submitted_witness_node_count == 5000U &&
          audit.predicate_pattern_task_count == 5U &&
          audit.predicate_task_count == 5000U &&
          audit.physical_predicate_task_count == 5000U &&
          audit.host_to_device_predicate_task_byte_count == 5U * 56U &&
          audit.device_to_host_predicate_result_byte_count == 5000U,
      "the repeated recipe must retain exact logical counts and compact "
      "transfer extents");
  check(
      audit.compact_repeated_transaction_recipe_validated &&
          audit.affine_transaction_id_range_validated &&
          audit.every_logical_transaction_represented_once &&
          audit.per_transaction_host_materialization_avoided &&
          !audit.physical_repeated_predicate_expansion_performed &&
          audit.transactional_product_mass_conservation_validated &&
          audit.submitted_unordered_pair_mass == repetition_count &&
          audit.pruned_unordered_pair_mass == repetition_count &&
          audit.residual_unordered_pair_mass == 0U &&
          audit.submitted_transaction_digest != 0U &&
          audit.canonical_transaction_digest != 0U &&
          audit.completed_transaction_digest != 0U &&
          audit.predicate_task_digest != 0U &&
          audit.predicate_result_digest != 0U &&
          audit.process_local_transcript_digest != 0U,
      "the compact repeated receipt must bind identities, decisions, digests "
      "and mass without a per-transaction host vector");
  check(
      !audit.global_pair_coverage_closed &&
          !audit.hierarchy_or_tree_claimed && !audit.slo_claimed &&
          !audit.pairwise_disjoint_support_products_validated &&
          !audit.ordinary_or_higher_order_delaunay_materialized &&
          !audit.public_status_claimed,
      "the repeated synthetic workload must not claim global coverage or a "
      "hierarchy");

  auto capacity_lease = traversal_lease(128U);
  auto capacity = morsehgp3d::gpu::
      qualify_repeated_exact_pair_block_antichain_transaction_cuda(
          std::move(capacity_lease),
          model,
          1537U,
          ExactPairBlockAntichainCudaConfig{6U, 12U});
  check(
      capacity.status ==
              ExactPairBlockAntichainCudaStatus::capacity_exhausted &&
          capacity.audit.transaction_capacity == 1536U &&
          capacity_lease.ready(),
      "the compact recipe must preserve its lease when the 12n transaction "
      "gate refuses it");

  auto overflow_model = model;
  overflow_model.transaction_id =
      std::numeric_limits<std::uint64_t>::max() - 2U;
  auto identity_lease = traversal_lease(128U);
  check_throws<std::overflow_error>(
      [&]() {
        static_cast<void>(morsehgp3d::gpu::
            qualify_repeated_exact_pair_block_antichain_transaction_cuda(
                std::move(identity_lease),
                overflow_model,
                4U,
                ExactPairBlockAntichainCudaConfig{6U, 12U}));
      },
      "an overflowing affine transaction-id recipe must fail closed");
  check(
      identity_lease.ready(),
      "an overflowing affine identity recipe must preserve its lease");
}

}  // namespace

int main() {
  test_atomic_multi_node_transactions();
  test_last_witness_failure_rolls_back_whole_product();
  test_insufficient_preflight_preserves_lease();
  test_invalid_preflight_and_capacity_preserve_lease();
  test_rank_eleven_and_predicate_capacity_gate();
  test_compact_repeated_transaction_recipe();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout
      << "all exact pair-block antichain CUDA host-contract tests passed\n";
  return 0;
}
