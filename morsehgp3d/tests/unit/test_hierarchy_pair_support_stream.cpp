#include "morsehgp3d/exact/center.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::hierarchy::ExactPairSupportEvent;
using morsehgp3d::hierarchy::ExactPairSupportExtraShellDiagnostic;
using morsehgp3d::hierarchy::ExactPairSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactPairSupportCheckpoint;
using morsehgp3d::hierarchy::ExactPairSupportCheckpointVerification;
using morsehgp3d::hierarchy::ExactPairSupportIncrementalVerifier;
using morsehgp3d::hierarchy::ExactPairSupportPendingStage;
using morsehgp3d::hierarchy::ExactPairSupportPendingProduct;
using morsehgp3d::hierarchy::ExactPairSupportWitnessNodeEntry;
using morsehgp3d::hierarchy::ExactPairSupportStopReason;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportStreamChunk;
using morsehgp3d::hierarchy::ExactPairSupportStreamChunkVerification;
using morsehgp3d::hierarchy::ExactPairSupportStreamResult;
using morsehgp3d::hierarchy::ExactPairSupportStreamRunVerification;
using morsehgp3d::hierarchy::ExactPairSupportStreamStatus;
using morsehgp3d::hierarchy::ExactPairSupportStreamVerification;
using morsehgp3d::hierarchy::build_exact_pair_support_stream;
using morsehgp3d::hierarchy::build_exact_pair_support_stream_chunk;
using morsehgp3d::hierarchy::compute_exact_pair_support_checkpoint_digest;
using morsehgp3d::hierarchy::exact_diametral_phi_aabb_maximum;
using morsehgp3d::hierarchy::make_initial_exact_pair_support_checkpoint;
using morsehgp3d::hierarchy::verify_exact_pair_support_checkpoint;
using morsehgp3d::hierarchy::verify_exact_pair_support_stream;
using morsehgp3d::hierarchy::verify_exact_pair_support_stream_chunk;
using morsehgp3d::hierarchy::verify_exact_pair_support_stream_run;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(!std::is_constructible_v<
              ExactPairSupportAuthorityContext,
              MortonLbvhIndex&&,
              const CanonicalPointCloud&,
              std::size_t>);
static_assert(!std::is_constructible_v<
              ExactPairSupportAuthorityContext,
              const MortonLbvhIndex&,
              CanonicalPointCloud&&,
              std::size_t>);
static_assert(!std::is_constructible_v<
              ExactPairSupportAuthorityContext,
              const MortonLbvhIndex&&,
              const CanonicalPointCloud&,
              std::size_t>);
static_assert(!std::is_constructible_v<
              ExactPairSupportAuthorityContext,
              const MortonLbvhIndex&,
              const CanonicalPointCloud&&,
              std::size_t>);
static_assert(!std::is_constructible_v<
              ExactPairSupportIncrementalVerifier,
              ExactPairSupportAuthorityContext&&>);
static_assert(!std::is_constructible_v<
              ExactPairSupportIncrementalVerifier,
              const ExactPairSupportAuthorityContext&&>);

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
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y = 0.0,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    std::initializer_list<CertifiedPoint3> points) {
  const std::vector<CertifiedPoint3> storage{points};
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{storage});
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactPairSupportStreamBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactPairSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] bool verification_closes(
    const ExactPairSupportStreamVerification& verification) {
  return verification.requested_budget_certified &&
         verification.requirements_certified &&
         verification.partial_records_individually_exact &&
         verification.completion_claim_certified &&
         verification.absence_claim_certified &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] bool chunk_verification_closes(
    const ExactPairSupportStreamChunkVerification& verification) {
  return verification.source_checkpoint_integrity_verified &&
         verification.requested_budget_certified &&
         verification.prepared_transition_chain_matches &&
         verification.records_individually_exact &&
         verification.next_checkpoint_integrity_verified &&
         verification.fresh_replay_certified &&
         verification.chunk_transition_verified;
}

void check_quasi_linear_checkpoint_validation(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportCheckpoint& checkpoint,
    const std::string& message) {
  const ExactPairSupportCheckpointVerification verification =
      verify_exact_pair_support_checkpoint(authority, checkpoint);
  std::size_t rectangle_count = 0U;
  for (const auto& entry : checkpoint.frontier) {
    rectangle_count += entry.self_product == 0U ? 2U : 1U;
  }
  std::size_t active_witness_count = 0U;
  std::size_t strict_receipt_count = 0U;
  if (checkpoint.pending_product.has_value() &&
      checkpoint.pending_product->stage ==
          ExactPairSupportPendingStage::rank_search) {
    active_witness_count =
        checkpoint.pending_product->witness_frontier.size() +
        (checkpoint.pending_product->deferred_expansion_node.has_value()
             ? 1U
             : 0U);
    strict_receipt_count =
        checkpoint.pending_product->strict_witness_receipts.size();
  }
  const std::size_t witness_interval_count =
      active_witness_count + strict_receipt_count;
  const auto& audit = verification.validation_audit;
  check(
      verification.integrity_verified &&
          verification.quasi_linear_structure_validation_certified &&
          audit.frontier_entry_validation_count ==
              checkpoint.frontier.size() &&
          audit.frontier_rectangle_count == rectangle_count &&
          audit.frontier_sweep_event_count == 2U * rectangle_count &&
          audit.frontier_active_set_operation_count ==
              audit.frontier_sweep_event_count &&
          audit.frontier_neighbor_test_count <= 2U * rectangle_count &&
          audit.active_witness_entry_validation_count ==
              active_witness_count &&
          audit.strict_witness_receipt_validation_count ==
              strict_receipt_count &&
          audit.witness_interval_count == witness_interval_count &&
          audit.witness_adjacent_interval_test_count ==
              (witness_interval_count == 0U
                   ? 0U
                   : witness_interval_count - 1U) &&
          audit.strict_receipt_geometry_recertification_count ==
              strict_receipt_count,
      message);
}

struct ChunkedPairRun {
  std::vector<ExactPairSupportEvent> events;
  std::vector<ExactPairSupportExtraShellDiagnostic> diagnostics;
  std::vector<ExactPairSupportStreamChunk::RecordKind> record_order;
  ExactPairSupportCheckpoint checkpoint;
  std::vector<ExactPairSupportStreamBudget> budgets;
  std::vector<ExactPairSupportStreamChunk> chunks;
  std::size_t chunk_count{};
  bool observed_active_rank_cursor{false};
  bool all_chunks_freshly_verified{true};
  bool anchored_run_certified{false};
};

[[nodiscard]] ChunkedPairRun run_chunked(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_order,
    const ExactPairSupportStreamBudget& chunk_budget,
    std::size_t maximum_chunk_count = 100000U) {
  ChunkedPairRun run;
  run.checkpoint = make_initial_exact_pair_support_checkpoint(
      index, cloud, maximum_order);
  check(
      verify_exact_pair_support_checkpoint(
          index, cloud, maximum_order, run.checkpoint)
          .integrity_verified,
      "the initial pair-support checkpoint has locally verified integrity against its exact authorities");
  while (!run.checkpoint.complete()) {
    if (run.chunk_count >= maximum_chunk_count) {
      throw std::logic_error(
          "the chunked pair-support fixture exceeded its progress bound");
    }
    const ExactPairSupportCheckpoint source = run.checkpoint;
    const ExactPairSupportStreamChunk chunk =
        build_exact_pair_support_stream_chunk(
            index, cloud, maximum_order, chunk_budget, source);
    run.all_chunks_freshly_verified =
        run.all_chunks_freshly_verified &&
        chunk_verification_closes(verify_exact_pair_support_stream_chunk(
            index,
            cloud,
            maximum_order,
            chunk_budget,
            source,
            chunk));
    check(
        chunk.candidate_prepared &&
            chunk.chunk_sequence == source.next_chunk_sequence &&
            chunk.first_output_record_index ==
                source.output_record_count &&
            chunk.source_checkpoint_digest == source.checkpoint_digest &&
            chunk.previous_output_chain_digest ==
                source.output_chain_digest &&
            chunk.record_order.size() ==
                chunk.events.size() +
                    chunk.relevant_extra_shell_diagnostics.size() &&
            chunk.cumulative_audit_before == source.cumulative_audit &&
            chunk.next_checkpoint.next_chunk_sequence ==
                source.next_chunk_sequence + 1U,
        "each pair-support chunk prepares one exact checkpoint transition");
    if (chunk.next_checkpoint.pending_product.has_value() &&
        chunk.next_checkpoint.pending_product->stage ==
            ExactPairSupportPendingStage::rank_search &&
        chunk.next_checkpoint.pending_product->rank_search_started) {
      run.observed_active_rank_cursor = true;
    }
    run.events.insert(
        run.events.end(), chunk.events.begin(), chunk.events.end());
    run.diagnostics.insert(
        run.diagnostics.end(),
        chunk.relevant_extra_shell_diagnostics.begin(),
        chunk.relevant_extra_shell_diagnostics.end());
    run.record_order.insert(
        run.record_order.end(),
        chunk.record_order.begin(),
        chunk.record_order.end());
    run.checkpoint = chunk.next_checkpoint;
    run.budgets.push_back(chunk_budget);
    run.chunks.push_back(chunk);
    ++run.chunk_count;
  }
  const ExactPairSupportStreamRunVerification anchored =
      verify_exact_pair_support_stream_run(
          index,
          cloud,
          maximum_order,
          run.budgets,
          run.chunks);
  run.anchored_run_certified =
      anchored.initial_checkpoint_reconstructed &&
      anchored.every_transition_verified &&
      anchored.verified_chunk_count == run.chunks.size() &&
      anchored.terminal_checkpoint_reached &&
      anchored.anchored_run_certified;
  return run;
}

void check_fresh_replay(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_order,
    const ExactPairSupportStreamBudget& budget,
    const ExactPairSupportStreamResult& result,
    const std::string& message) {
  check(
      verification_closes(verify_exact_pair_support_stream(
          index, cloud, maximum_order, budget, result)),
      message);
}

[[nodiscard]] std::uint64_t bits(double value) {
  return morsehgp3d::exact::canonicalize_binary64_bits(
      std::bit_cast<std::uint64_t>(value));
}

[[nodiscard]] ExactDyadicAabb3 box(
    std::array<double, 3> lower,
    std::array<double, 3> upper) {
  ExactDyadicAabb3 result{};
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    result.lower_binary64_bits[axis] = bits(lower[axis]);
    result.upper_binary64_bits[axis] = bits(upper[axis]);
  }
  return result;
}

struct OraclePairRecords {
  std::vector<ExactPairSupportEvent> events;
  std::vector<ExactPairSupportExtraShellDiagnostic> diagnostics;
};

[[nodiscard]] OraclePairRecords brute_force_pair_records(
    const CanonicalPointCloud& cloud,
    std::size_t maximum_order) {
  OraclePairRecords records;
  const std::size_t effective_order =
      std::min(maximum_order, cloud.size());
  const std::size_t maximum_closed_rank =
      std::min(effective_order + 1U, cloud.size());
  if (maximum_closed_rank < 2U) {
    return records;
  }
  const std::size_t interior_cap = maximum_closed_rank - 2U;
  for (std::size_t first_index = 0U;
       first_index < cloud.size();
       ++first_index) {
    for (std::size_t second_index = first_index + 1U;
         second_index < cloud.size();
         ++second_index) {
      const PointId first = static_cast<PointId>(first_index);
      const PointId second = static_cast<PointId>(second_index);
      const auto sphere = morsehgp3d::exact::circumcenter(
          cloud.point(first), cloud.point(second));
      if (!sphere.center().has_value() ||
          !sphere.squared_level().has_value()) {
        throw std::logic_error(
            "an exhaustive pair did not define a unique sphere");
      }
      const auto partition = morsehgp3d::spatial::brute_force_closed_ball(
          cloud, *sphere.center(), *sphere.squared_level());
      if (partition.interior_ids().size() > interior_cap) {
        continue;
      }
      const std::array<PointId, 2> support{first, second};
      const std::vector<PointId> interior{
          partition.interior_ids().begin(),
          partition.interior_ids().end()};
      if (partition.shell_ids().size() == 2U &&
          std::equal(
              partition.shell_ids().begin(),
              partition.shell_ids().end(),
              support.begin(),
              support.end())) {
        records.events.push_back(ExactPairSupportEvent{
            support,
            *sphere.center(),
            *sphere.squared_level(),
            interior,
            partition.closed_rank(),
            partition.exterior_ids().size()});
        continue;
      }
      const auto extra = std::find_if(
          partition.shell_ids().begin(),
          partition.shell_ids().end(),
          [support](PointId point_id) {
            return point_id != support[0] && point_id != support[1];
          });
      if (extra == partition.shell_ids().end()) {
        throw std::logic_error(
            "an exhaustive extra shell omitted its extra point");
      }
      records.diagnostics.push_back(
          ExactPairSupportExtraShellDiagnostic{
              support,
              *sphere.center(),
              *sphere.squared_level(),
              interior,
              partition.shell_ids().size(),
              *extra,
              interior.size() + 2U,
              partition.closed_rank(),
              partition.exterior_ids().size()});
    }
  }
  return records;
}

void check_complete_accounting(
    const ExactPairSupportStreamResult& result,
    const std::string& message) {
  const auto& audit = result.audit;
  check(
      result.stream_complete() &&
          result.absence_of_additional_pair_supports_certified() &&
          audit.resolved_pair_count == audit.total_pair_count &&
          audit.remaining_frontier_pair_count == 0U &&
          audit.rank_pruned_pair_count +
                  audit.leaf_pair_classification_count ==
              audit.resolved_pair_count &&
          audit.accepted_event_count +
                  audit.relevant_extra_shell_diagnostic_count +
                  audit.above_rank_pair_count ==
              audit.leaf_pair_classification_count &&
          audit.pair_partition_accounting_certified &&
          result.self_product_partition_certified &&
          result.witness_antichains_certified &&
          result.all_rank_prunes_recertified &&
          result.all_rank_relevant_shells_complete &&
          result.no_forbidden_global_structure_materialized &&
          !result.hierarchy_reduction_performed,
      message);
}

void test_exact_phi_aabb_maximum() {
  const ExactDyadicAabb3 first = box(
      {-7.0, -7.0, -7.0}, {-6.0, -6.0, -6.0});
  const ExactDyadicAabb3 second = box(
      {-7.0, -2.0, -7.0}, {-6.0, -1.0, -6.0});
  const ExactDyadicAabb3 query = box(
      {-7.0, -5.0, -7.0}, {-5.0, -4.0, -5.0});
  const auto maximum = exact_diametral_phi_aabb_maximum(
      first, second, query);
  check(
      maximum.maximum_phi == ExactRational{BigInt{5}} &&
          maximum.query_endpoint ==
              std::array<std::uint8_t, 3>{1U, 0U, 1U} &&
          maximum.first_support_endpoint ==
              std::array<std::uint8_t, 3>{0U, 1U, 0U} &&
          maximum.second_support_endpoint ==
              std::array<std::uint8_t, 3>{0U, 0U, 0U},
      "the exact phi bound maximizes each trivariate endpoint box with a canonical witness");

  const ExactDyadicAabb3 left_point = box(
      {-1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0});
  const ExactDyadicAabb3 right_point = box(
      {1.0, 0.0, 0.0}, {1.0, 0.0, 0.0});
  const ExactDyadicAabb3 inside_point = box(
      {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
  const ExactDyadicAabb3 shell_point = box(
      {1.0, 0.0, 0.0}, {1.0, 0.0, 0.0});
  check(
      exact_diametral_phi_aabb_maximum(
          left_point, right_point, inside_point).maximum_phi ==
          ExactRational{BigInt{-1}} &&
          exact_diametral_phi_aabb_maximum(
              left_point, right_point, shell_point).maximum_phi.is_zero(),
      "strict negativity certifies an interior witness while equality remains non-prunable");

  const ExactDyadicAabb3 zero_query = box(
      {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0});
  const std::array<ExactDyadicAabb3, 2> correlated_first{
      box({2.0, 1.0, 0.0}, {2.0, 1.0, 0.0}),
      box({1.0, 2.0, 0.0}, {1.0, 2.0, 0.0})};
  const std::array<ExactDyadicAabb3, 2> correlated_second{
      box({1.0, -3.0, 0.0}, {1.0, -3.0, 0.0}),
      box({-3.0, 1.0, 0.0}, {-3.0, 1.0, 0.0})};
  bool every_real_pair_is_strict = true;
  for (const ExactDyadicAabb3& first_real : correlated_first) {
    for (const ExactDyadicAabb3& second_real : correlated_second) {
      every_real_pair_is_strict =
          every_real_pair_is_strict &&
          exact_diametral_phi_aabb_maximum(
              first_real, second_real, zero_query).maximum_phi.sign() < 0;
    }
  }
  const ExactDyadicAabb3 correlated_first_box = box(
      {1.0, 1.0, 0.0}, {2.0, 2.0, 0.0});
  const ExactDyadicAabb3 correlated_second_box = box(
      {-3.0, -3.0, 0.0}, {1.0, 1.0, 0.0});
  check(
      every_real_pair_is_strict &&
          exact_diametral_phi_aabb_maximum(
              correlated_first_box,
              correlated_second_box,
              zero_query).maximum_phi == ExactRational{BigInt{4}},
      "artificial AABB corners cannot certify the absence of a universal witness for correlated real sites");

  ExactDyadicAabb3 reversed = inside_point;
  reversed.lower_binary64_bits[0] = bits(2.0);
  reversed.upper_binary64_bits[0] = bits(1.0);
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(exact_diametral_phi_aabb_maximum(
            left_point, right_point, reversed));
      },
      "the exact phi bound rejects a reversed AABB");
}

void test_complete_self_dual_partition_and_long_pair() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(0.0), point(1.0), point(4.0), point(10.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 10U, budget);
  const OraclePairRecords oracle = brute_force_pair_records(cloud, 10U);
  check(
      result.events == oracle.events &&
          result.relevant_extra_shell_diagnostics == oracle.diagnostics &&
          result.events.size() == 6U &&
          result.relevant_extra_shell_diagnostics.empty() &&
          result.audit.total_pair_count == 6U &&
          result.audit.rank_pruned_pair_count == 0U &&
          result.audit.leaf_pair_classification_count == 6U &&
          result.audit.diagonal_leaf_discard_count == 4U &&
          result.audit.diagonal_product_rank_search_skip_count > 0U &&
          result.audit.global_closed_ball_query_count == 6U &&
          result.audit.point_classification_count == 24U,
      "the self-dual traversal emits every collinear pair exactly once without a global cell arena");
  const auto long_pair = std::find_if(
      result.events.begin(),
      result.events.end(),
      [](const ExactPairSupportEvent& event) {
        return event.support_ids == std::array<PointId, 2>{0U, 3U};
      });
  check(
      long_pair != result.events.end() &&
          long_pair->interior_ids == std::vector<PointId>{1U, 2U} &&
          long_pair->closed_rank == 4U,
      "the direct traversal retains a long pair that a fixed 1-NN list would miss");
  check_complete_accounting(
      result,
      "the complete collinear stream closes its unordered-pair accounting");
  check_fresh_replay(
      index,
      cloud,
      10U,
      budget,
      result,
      "the complete collinear stream passes fresh replay");
}

void test_leaf_rank_cap_and_sparse_queries() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(-1.0), point(0.0), point(1.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 1U, budget);
  const OraclePairRecords oracle = brute_force_pair_records(cloud, 1U);
  check(
      result.events == oracle.events &&
          result.relevant_extra_shell_diagnostics == oracle.diagnostics &&
          result.events.size() == 2U &&
          result.audit.rank_pruned_pair_count == 0U &&
          result.audit.early_closed_rank_rejection_count == 1U &&
          result.audit.global_closed_ball_query_count == 3U &&
          result.audit.point_classification_count < 9U,
      "the sparse leaf query stops the long rank-three pair as soon as one strict interior is certified at K=1");
  check_complete_accounting(
      result,
      "the rank-capped stream still partitions every unordered pair");
  check_fresh_replay(
      index,
      cloud,
      1U,
      budget,
      result,
      "the rank-capped stream passes exact fresh replay");
}

void test_extra_shell_and_equality_descent() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(-1.0, 0.0), point(0.0, 1.0), point(1.0, 0.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 1U, budget);
  const OraclePairRecords oracle = brute_force_pair_records(cloud, 1U);
  check(
      result.events == oracle.events &&
          result.relevant_extra_shell_diagnostics == oracle.diagnostics &&
          result.events.size() == 2U &&
          result.relevant_extra_shell_diagnostics.size() == 1U,
      "the right triangle separates its two regular sides from the hypotenuse extra shell");
  if (result.relevant_extra_shell_diagnostics.size() == 1U) {
    const auto& diagnostic = result.relevant_extra_shell_diagnostics.front();
    check(
        diagnostic.center == ExactRational3{} &&
            diagnostic.squared_level ==
                morsehgp3d::exact::ExactLevel{BigInt{1}} &&
            diagnostic.interior_ids.empty() &&
            diagnostic.shell_count == 3U &&
            diagnostic.minimum_possible_closed_rank == 2U &&
            diagnostic.observed_closed_rank == 3U &&
            diagnostic.exterior_count == 0U &&
            diagnostic.canonical_extra_shell_witness_id !=
                diagnostic.support_ids[0] &&
            diagnostic.canonical_extra_shell_witness_id !=
                diagnostic.support_ids[1],
        "the extra-shell diagnostic stores one canonical witness and a complete count, not the full shell");
  }
  check_complete_accounting(
      result,
      "the extra-shell stream closes all pair categories");
  check_fresh_replay(
      index,
      cloud,
      1U,
      budget,
      result,
      "the extra-shell stream passes fresh replay");
}

void test_real_anchor_shell_equality_exclusion() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(-1.0, 0.0),
       point(0.0, -1.0),
       point(0.0, 1.0),
       point(1.0, 0.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 1U, budget);
  const OraclePairRecords oracle = brute_force_pair_records(cloud, 1U);
  check(
      result.events == oracle.events &&
          result.relevant_extra_shell_diagnostics == oracle.diagnostics &&
          result.audit.certified_anchor_shell_tangent_subtree_count == 1U &&
          result.audit.certified_anchor_noninterior_subtree_count == 2U &&
          result.audit.rank_prune_search_count == 2U &&
          result.audit.rank_pruned_pair_count == 0U,
      "a real anchor may exclude an internal witness subtree at exact shell equality without fabricating a rank prune");
  check_complete_accounting(
      result,
      "the anchor-shell equality fixture keeps complete pair accounting");
  check_fresh_replay(
      index,
      cloud,
      1U,
      budget,
      result,
      "the anchor-shell equality fixture passes exact fresh replay");
}

void check_budget_stop(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const ExactPairSupportStreamBudget& budget,
    ExactPairSupportStopReason expected_reason,
    const std::string& message) {
  const ExactPairSupportStreamResult result =
      build_exact_pair_support_stream(index, cloud, 1U, budget);
  check(
      result.status == ExactPairSupportStreamStatus::budget_exhausted &&
          result.stop_reason == expected_reason &&
          !result.stream_complete() &&
          !result.absence_of_additional_pair_supports_certified() &&
          !result.frontier_exhausted &&
          !result.remaining_frontier.empty() &&
          result.audit.resolved_pair_count +
                  result.audit.remaining_frontier_pair_count ==
              result.audit.total_pair_count &&
          result.audit.pair_partition_accounting_certified,
      message);
  check_fresh_replay(
      index,
      cloud,
      1U,
      budget,
      result,
      message + " passes fresh replay as an honest partial result");
}

void test_transactional_budgets() {
  const CanonicalPointCloud cloud = cloud_from({point(-1.0), point(1.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();

  ExactPairSupportStreamBudget budget = unlimited_budget();
  budget.maximum_work_unit_count = 0U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::work_unit_limit,
      "zero work budget preserves the initial self product");

  budget = unlimited_budget();
  budget.maximum_frontier_entry_count = 1U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::frontier_entry_limit,
      "an undersized product frontier refuses a partial self expansion");

  budget = unlimited_budget();
  budget.maximum_emitted_record_count = 0U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::emitted_record_limit,
      "zero record capacity leaves the terminal pair unresolved");

  budget = unlimited_budget();
  budget.maximum_emitted_point_id_reference_count = 0U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::emitted_point_id_reference_limit,
      "zero PointId capacity cannot be bypassed by a fixed-size support record");

  budget = unlimited_budget();
  budget.maximum_global_closed_ball_query_count = 0U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::global_closed_ball_query_limit,
      "zero closed-ball capacity leaves the leaf pair atomic");

  budget = unlimited_budget();
  budget.maximum_point_classification_count = cloud.size() - 1U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::point_classification_limit,
      "a leaf query refuses to start without a full-cloud logical classification budget");

  budget = unlimited_budget();
  budget.maximum_auxiliary_frontier_entry_count = 1U;
  check_budget_stop(
      index,
      cloud,
      budget,
      ExactPairSupportStopReason::auxiliary_frontier_entry_limit,
      "a sparse query refuses an auxiliary frontier below its certified depth bound");

  check_throws<std::invalid_argument>(
      [&] {
        ExactPairSupportStreamBudget invalid = unlimited_budget();
        invalid.maximum_frontier_entry_count = 0U;
        static_cast<void>(
            build_exact_pair_support_stream(index, cloud, 1U, invalid));
      },
      "a nonempty pair domain rejects an impossible zero-sized persisted frontier");

  const CanonicalPointCloud singleton = cloud_from({point(0.0)});
  const MortonLbvhIndex singleton_index = MortonLbvhIndex::build(singleton);
  const ExactPairSupportStreamBudget zero_budget{};
  const ExactPairSupportStreamResult singleton_result =
      build_exact_pair_support_stream(
          singleton_index, singleton, 10U, zero_budget);
  check(
      singleton_result.stream_complete() &&
          singleton_result.audit.total_pair_count == 0U &&
          singleton_result.events.empty() &&
          singleton_result.remaining_frontier.empty(),
      "a singleton completes without consuming any pair-stream budget");
  check_fresh_replay(
      singleton_index,
      singleton,
      10U,
      zero_budget,
      singleton_result,
      "the zero-work singleton result passes fresh replay");
  static_cast<void>(maximum);
}

void test_resumable_chunks_match_resident_stream() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(0.0), point(1.0), point(4.0), point(10.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportCheckpoint initial =
      make_initial_exact_pair_support_checkpoint(index, cloud, 10U);
  const ExactPairSupportStreamChunk resident =
      build_exact_pair_support_stream_chunk(
          index, cloud, 10U, unlimited_budget(), initial);
  check(
      resident.relative_stream_complete() && resident.events.size() == 6U &&
          resident.relevant_extra_shell_diagnostics.empty(),
      "the resident checkpoint lane closes the six-pair fixture in one transaction");

  ExactPairSupportStreamBudget unit_budget = unlimited_budget();
  unit_budget.maximum_work_unit_count = 1U;
  unit_budget.maximum_emitted_record_count = 1U;
  const ChunkedPairRun chunked =
      run_chunked(index, cloud, 10U, unit_budget);
  check(
      chunked.chunk_count > 1U &&
          chunked.all_chunks_freshly_verified &&
          chunked.anchored_run_certified &&
          chunked.events == resident.events &&
          chunked.diagnostics ==
              resident.relevant_extra_shell_diagnostics &&
          chunked.record_order == resident.record_order &&
          chunked.checkpoint.cumulative_audit ==
              resident.next_checkpoint.cumulative_audit &&
          chunked.checkpoint.output_record_count ==
              resident.next_checkpoint.output_record_count &&
          chunked.checkpoint.output_chain_digest ==
              resident.next_checkpoint.output_chain_digest &&
          chunked.checkpoint.complete(),
      "one-work-unit chunks preserve the exact resident record order, audit, and output chain without replayed work");
  const auto long_pair = std::find_if(
      chunked.events.begin(),
      chunked.events.end(),
      [](const ExactPairSupportEvent& event) {
        return event.support_ids == std::array<PointId, 2>{0U, 3U};
      });
  check(
      long_pair != chunked.events.end() &&
          long_pair->interior_ids == std::vector<PointId>{1U, 2U} &&
          long_pair->closed_rank == 4U,
      "the resumable stream retains the nonlocal long pair and its exact rank");
}

void test_persistent_authority_and_incremental_verifier() {
  std::vector<CertifiedPoint3> points;
  points.reserve(14U);
  for (std::size_t point_index = 0U; point_index < 14U; ++point_index) {
    const double x = static_cast<double>(point_index) - 7.0;
    const double y =
        static_cast<double>((point_index * point_index + 3U) % 17U) - 8.0;
    const double z = static_cast<double>(
                         (point_index * point_index * point_index + 5U) %
                         19U) -
                     9.0;
    points.push_back(point(x, y, z));
  }
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportAuthorityContext authority{index, cloud, 10U};
  check(
      authority.audit().manifest_cached &&
          authority.audit().manifest_build_count == 1U &&
          authority.audit().canonical_cloud_point_hash_count ==
              cloud.size() &&
          authority.audit().lbvh_leaf_hash_count ==
              authority.manifest().lbvh_leaf_count &&
          authority.audit().lbvh_node_hash_count ==
              authority.manifest().lbvh_node_count,
      "the persistent pair-support authority hashes the cloud and LBVH exactly once");

  const ExactPairSupportCheckpoint initial =
      make_initial_exact_pair_support_checkpoint(authority);
  check_quasi_linear_checkpoint_validation(
      authority,
      initial,
      "the authority-backed initial checkpoint uses bounded sweep validation");
  const ExactPairSupportStreamChunk resident =
      build_exact_pair_support_stream_chunk(
          authority, unlimited_budget(), initial);

  std::optional<ExactPairSupportIncrementalVerifier>
      verifier_after_context_destruction;
  {
    const ExactPairSupportAuthorityContext short_lived_authority{
        index, cloud, 10U};
    verifier_after_context_destruction.emplace(short_lived_authority);
  }
  ExactPairSupportStreamBudget short_context_budget = unlimited_budget();
  short_context_budget.maximum_work_unit_count = 1U;
  short_context_budget.maximum_emitted_record_count = 1U;
  const ExactPairSupportStreamChunk first_after_context_destruction =
      build_exact_pair_support_stream_chunk(
          authority,
          short_context_budget,
          verifier_after_context_destruction->trusted_checkpoint());
  check(
      verifier_after_context_destruction
          ->verify_next(
              first_after_context_destruction.budget,
              first_after_context_destruction)
          .chunk_transition_verified,
      "the incremental verifier owns its authority cache after the construction context is destroyed");

  ExactPairSupportStreamBudget unit_budget = unlimited_budget();
  unit_budget.maximum_work_unit_count = 1U;
  unit_budget.maximum_emitted_record_count = 1U;
  ExactPairSupportIncrementalVerifier verifier{authority};
  std::vector<ExactPairSupportEvent> events;
  std::vector<ExactPairSupportExtraShellDiagnostic> diagnostics;
  std::vector<ExactPairSupportStreamChunk::RecordKind> record_order;
  std::size_t chunk_count = 0U;
  bool observed_active_rank_cursor = false;
  bool observed_strict_receipt = false;
  while (!verifier.status().terminal_checkpoint_reached) {
    if (chunk_count >= 100000U) {
      throw std::logic_error(
          "the incremental pair-support fixture exceeded its progress bound");
    }
    check_quasi_linear_checkpoint_validation(
        authority,
        verifier.trusted_checkpoint(),
        "each incremental source checkpoint uses quasi-linear structural validation");
    const ExactPairSupportStreamChunk chunk =
        build_exact_pair_support_stream_chunk(
            authority, unit_budget, verifier.trusted_checkpoint());
    const ExactPairSupportStreamChunkVerification transition =
        verifier.verify_next(unit_budget, chunk);
    check(
        chunk_verification_closes(transition) &&
            verifier.status().anchored_prefix_certified &&
            !verifier.status().failed_closed &&
            verifier.status().retained_chunk_count == 0U,
        "verify_next advances one anchored transition without retaining its chunk");
    events.insert(events.end(), chunk.events.begin(), chunk.events.end());
    diagnostics.insert(
        diagnostics.end(),
        chunk.relevant_extra_shell_diagnostics.begin(),
        chunk.relevant_extra_shell_diagnostics.end());
    record_order.insert(
        record_order.end(),
        chunk.record_order.begin(),
        chunk.record_order.end());
    if (verifier.trusted_checkpoint().pending_product.has_value() &&
        verifier.trusted_checkpoint().pending_product->stage ==
            ExactPairSupportPendingStage::rank_search) {
      observed_active_rank_cursor =
          observed_active_rank_cursor ||
          verifier.trusted_checkpoint()
              .pending_product->rank_search_started;
      observed_strict_receipt =
          observed_strict_receipt ||
          !verifier.trusted_checkpoint()
               .pending_product->strict_witness_receipts.empty();
    }
    ++chunk_count;
  }
  check_quasi_linear_checkpoint_validation(
      authority,
      verifier.trusted_checkpoint(),
      "the terminal incremental checkpoint keeps the quasi-linear validation certificate");
  check(
      chunk_count > 20U && observed_active_rank_cursor &&
          observed_strict_receipt,
      "the incremental fixture spans many chunks and persists an active strict-witness cursor");
  check(
      verifier.status().verified_chunk_count == chunk_count &&
          verifier.status().every_transition_verified &&
          verifier.status().terminal_checkpoint_reached &&
          verifier.status().anchored_run_certified &&
          verifier.status().retained_chunk_count == 0U,
      "the incremental verifier closes an anchored terminal run while retaining no chunk history");
  check(
      events == resident.events &&
          diagnostics == resident.relevant_extra_shell_diagnostics &&
          record_order == resident.record_order &&
          verifier.trusted_checkpoint().cumulative_audit ==
              resident.next_checkpoint.cumulative_audit &&
          verifier.trusted_checkpoint().output_chain_digest ==
              resident.next_checkpoint.output_chain_digest,
      "the incremental stream reproduces the resident records, audit, and output digest");
  check(
      authority.audit().manifest_build_count == 1U,
      "all incremental production and verification calls reuse one authority manifest");

  ExactPairSupportIncrementalVerifier poisoned{authority};
  const ExactPairSupportCheckpoint trusted_before =
      poisoned.trusted_checkpoint();
  const ExactPairSupportStreamChunk valid_first =
      build_exact_pair_support_stream_chunk(
          authority, unit_budget, trusted_before);
  ExactPairSupportStreamChunk mutated_first = valid_first;
  mutated_first.source_checkpoint_digest = {};
  check(
      !poisoned.verify_next(unit_budget, mutated_first)
           .chunk_transition_verified &&
          poisoned.status().failed_closed &&
          !poisoned.status().anchored_prefix_certified &&
          poisoned.status().verified_chunk_count == 0U &&
          poisoned.status().retained_chunk_count == 0U &&
          poisoned.trusted_checkpoint() == trusted_before,
      "a rejected incremental transition poisons the verifier without advancing its trusted checkpoint");
  check(
      !poisoned.verify_next(unit_budget, valid_first)
           .chunk_transition_verified &&
          poisoned.trusted_checkpoint() == trusted_before,
      "a poisoned incremental verifier cannot later accept a retry of the rejected transition");
  const ExactPairSupportStreamChunk skipped_successor =
      build_exact_pair_support_stream_chunk(
          authority,
          unit_budget,
          valid_first.next_checkpoint);
  check(
      !poisoned.verify_next(unit_budget, skipped_successor)
           .chunk_transition_verified &&
          poisoned.trusted_checkpoint() == trusted_before,
      "a poisoned incremental verifier cannot skip the rejected transition by presenting its successor");

  ExactPairSupportIncrementalVerifier exceptional{authority};
  ExactPairSupportStreamBudget undersized_budget = unit_budget;
  undersized_budget.maximum_frontier_entry_count = 0U;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(
            exceptional.verify_next(undersized_budget, valid_first));
      },
      "an incremental replay propagates an invalid persisted-frontier capacity");
  check(
      exceptional.status().failed_closed &&
          !exceptional.status().anchored_prefix_certified &&
          exceptional.status().verified_chunk_count == 0U &&
          exceptional.status().retained_chunk_count == 0U &&
          exceptional.trusted_checkpoint() == trusted_before,
      "an exceptional incremental replay poisons without advancing its trusted checkpoint");

  const CanonicalPointCloud singleton = cloud_from({point(0.0)});
  const MortonLbvhIndex singleton_index = MortonLbvhIndex::build(singleton);
  const ExactPairSupportAuthorityContext singleton_authority{
      singleton_index, singleton, 10U};
  ExactPairSupportIncrementalVerifier singleton_verifier{
      singleton_authority};
  const ExactPairSupportStreamChunk redundant_terminal =
      build_exact_pair_support_stream_chunk(
          singleton_authority,
          ExactPairSupportStreamBudget{},
          singleton_verifier.trusted_checkpoint());
  check(
      singleton_verifier.status().anchored_run_certified &&
          singleton_verifier.status().verified_chunk_count == 0U &&
          singleton_verifier.status().retained_chunk_count == 0U &&
          !singleton_verifier
               .verify_next(
                   ExactPairSupportStreamBudget{}, redundant_terminal)
               .chunk_transition_verified &&
          singleton_verifier.status().failed_closed,
      "the incremental singleton run is canonically empty and rejects a redundant no-op chunk");
}

void test_every_prepared_stop_resumes_without_recount() {
  const CanonicalPointCloud cloud =
      cloud_from({point(-1.0), point(1.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportCheckpoint initial =
      make_initial_exact_pair_support_checkpoint(index, cloud, 1U);
  const ExactPairSupportStreamChunk resident =
      build_exact_pair_support_stream_chunk(
          index, cloud, 1U, unlimited_budget(), initial);

  std::vector<std::pair<ExactPairSupportStreamBudget,
                        ExactPairSupportStopReason>> cases;
  ExactPairSupportStreamBudget limited = unlimited_budget();
  limited.maximum_work_unit_count = 0U;
  cases.emplace_back(limited, ExactPairSupportStopReason::work_unit_limit);
  limited = unlimited_budget();
  limited.maximum_frontier_entry_count = 1U;
  cases.emplace_back(limited, ExactPairSupportStopReason::frontier_entry_limit);
  limited = unlimited_budget();
  limited.maximum_auxiliary_frontier_entry_count = 1U;
  cases.emplace_back(
      limited, ExactPairSupportStopReason::auxiliary_frontier_entry_limit);
  limited = unlimited_budget();
  limited.maximum_emitted_record_count = 0U;
  cases.emplace_back(limited, ExactPairSupportStopReason::emitted_record_limit);
  limited = unlimited_budget();
  limited.maximum_emitted_point_id_reference_count = 0U;
  cases.emplace_back(
      limited,
      ExactPairSupportStopReason::emitted_point_id_reference_limit);
  limited = unlimited_budget();
  limited.maximum_global_closed_ball_query_count = 0U;
  cases.emplace_back(
      limited,
      ExactPairSupportStopReason::global_closed_ball_query_limit);
  limited = unlimited_budget();
  limited.maximum_point_classification_count = cloud.size() - 1U;
  cases.emplace_back(
      limited, ExactPairSupportStopReason::point_classification_limit);

  for (const auto& [first_budget, expected_reason] : cases) {
    const ExactPairSupportStreamChunk partial =
        build_exact_pair_support_stream_chunk(
            index, cloud, 1U, first_budget, initial);
    check(
        partial.status == ExactPairSupportStreamStatus::budget_exhausted &&
            partial.stop_reason == expected_reason &&
            !partial.next_checkpoint.complete(),
        "each chunk budget refusal returns an honest resumable checkpoint");
    const ExactPairSupportStreamChunk resumed =
        build_exact_pair_support_stream_chunk(
            index,
            cloud,
            1U,
            unlimited_budget(),
            partial.next_checkpoint);
    check(
        resumed.relative_stream_complete() &&
            resumed.next_checkpoint.cumulative_audit ==
                resident.next_checkpoint.cumulative_audit &&
            resumed.next_checkpoint.output_record_count == 1U &&
            resumed.next_checkpoint.output_chain_digest ==
                resident.next_checkpoint.output_chain_digest,
        "resuming a refused transaction reaches the resident audit without charging its active product twice");
  }
}

void test_resume_inside_rank_witness_search() {
  std::vector<CertifiedPoint3> points;
  points.reserve(14U);
  for (std::size_t point_index = 0U; point_index < 14U; ++point_index) {
    const double x = static_cast<double>(point_index) - 7.0;
    const double y =
        static_cast<double>((point_index * point_index + 3U) % 17U) - 8.0;
    const double z = static_cast<double>(
                         (point_index * point_index * point_index + 5U) %
                         19U) -
                     9.0;
    points.push_back(point(x, y, z));
  }
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportCheckpoint initial =
      make_initial_exact_pair_support_checkpoint(index, cloud, 1U);
  const ExactPairSupportStreamChunk resident =
      build_exact_pair_support_stream_chunk(
          index, cloud, 1U, unlimited_budget(), initial);
  ExactPairSupportStreamBudget unit_budget = unlimited_budget();
  unit_budget.maximum_work_unit_count = 1U;
  const ChunkedPairRun chunked =
      run_chunked(index, cloud, 1U, unit_budget);
  check(
      chunked.observed_active_rank_cursor &&
          chunked.all_chunks_freshly_verified &&
          chunked.anchored_run_certified &&
          chunked.events == resident.events &&
          chunked.diagnostics ==
              resident.relevant_extra_shell_diagnostics &&
          chunked.record_order == resident.record_order &&
          chunked.checkpoint.cumulative_audit ==
              resident.next_checkpoint.cumulative_audit &&
          chunked.checkpoint.output_chain_digest ==
              resident.next_checkpoint.output_chain_digest &&
          chunked.checkpoint.cumulative_audit.total_pair_count == 91U &&
          chunked.checkpoint.cumulative_audit.rank_pruned_product_count > 0U &&
          chunked.checkpoint.cumulative_audit.rank_pruned_pair_count > 0U,
      "a serialized mid-search witness cursor resumes without double-counting products, witnesses, or rank prunes");
}

void test_pending_witness_checkpoint_invariants() {
  std::vector<CertifiedPoint3> points;
  points.reserve(14U);
  for (std::size_t point_index = 0U; point_index < 14U; ++point_index) {
    const double x = static_cast<double>(point_index) - 7.0;
    const double y =
        static_cast<double>((point_index * point_index + 3U) % 17U) - 8.0;
    const double z = static_cast<double>(
                         (point_index * point_index * point_index + 5U) %
                         19U) -
                     9.0;
    points.push_back(point(x, y, z));
  }
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);

  ExactPairSupportCheckpoint cursor =
      make_initial_exact_pair_support_checkpoint(index, cloud, 10U);
  ExactPairSupportStreamBudget unit_work = unlimited_budget();
  unit_work.maximum_work_unit_count = 1U;
  std::optional<ExactPairSupportCheckpoint> receipt_checkpoint;
  std::optional<ExactPairSupportCheckpoint> active_rank_checkpoint;
  std::optional<ExactPairSupportWitnessNodeEntry> observed_leaf_entry;
  std::optional<ExactPairSupportWitnessNodeEntry>
      observed_receipt_descendant_leaf;
  for (std::size_t step = 0U;
       step < 10000U && !cursor.complete() &&
       (!receipt_checkpoint.has_value() ||
        !observed_leaf_entry.has_value() ||
        !active_rank_checkpoint.has_value() ||
        !observed_receipt_descendant_leaf.has_value());
       ++step) {
    const ExactPairSupportStreamChunk chunk =
        build_exact_pair_support_stream_chunk(
            index, cloud, 10U, unit_work, cursor);
    cursor = chunk.next_checkpoint;
    if (!receipt_checkpoint.has_value() &&
        cursor.pending_product.has_value()) {
      const bool has_internal_receipt = std::any_of(
          cursor.pending_product->strict_witness_receipts.begin(),
          cursor.pending_product->strict_witness_receipts.end(),
          [](const ExactPairSupportWitnessNodeEntry& receipt) {
            return receipt.leaf_end - receipt.leaf_begin > 1U;
          });
      if (has_internal_receipt) {
        receipt_checkpoint = cursor;
      }
    }
    if (!active_rank_checkpoint.has_value() &&
        cursor.pending_product.has_value() &&
        cursor.pending_product->stage ==
            ExactPairSupportPendingStage::rank_search &&
        cursor.pending_product->rank_search_started &&
        !cursor.pending_product->witness_frontier.empty()) {
      active_rank_checkpoint = cursor;
    }
    const auto observe_leaf = [&](ExactPairSupportWitnessNodeEntry leaf) {
      if (!observed_leaf_entry.has_value()) {
        observed_leaf_entry = leaf;
      }
      if (receipt_checkpoint.has_value() &&
          !observed_receipt_descendant_leaf.has_value()) {
        for (const auto& receipt :
             receipt_checkpoint->pending_product
                 ->strict_witness_receipts) {
          if (receipt.leaf_begin <= leaf.leaf_begin &&
              leaf.leaf_end <= receipt.leaf_end &&
              receipt.leaf_end - receipt.leaf_begin > 1U) {
            observed_receipt_descendant_leaf = leaf;
            break;
          }
        }
      }
    };
    for (const auto& entry : cursor.frontier) {
      if (entry.first_leaf_end == entry.first_leaf_begin + 1U) {
        observe_leaf(ExactPairSupportWitnessNodeEntry{
            entry.first_node_index,
            entry.first_leaf_begin,
            entry.first_leaf_end});
      }
      if (entry.second_leaf_end == entry.second_leaf_begin + 1U) {
        observe_leaf(ExactPairSupportWitnessNodeEntry{
            entry.second_node_index,
            entry.second_leaf_begin,
            entry.second_leaf_end});
      }
    }
  }
  check(
      receipt_checkpoint.has_value(),
      "one-work-unit K=10 chunks persist a nonempty strict-witness receipt set");
  check(
      observed_leaf_entry.has_value(),
      "the resumable traversal exposes a valid leaf-node witness identity");
  check(
      active_rank_checkpoint.has_value(),
      "one-work-unit chunks persist an active witness frontier before expansion");
  check(
      observed_receipt_descendant_leaf.has_value(),
      "the fixture exposes a real leaf descendant of a persisted strict receipt");

  std::optional<ExactPairSupportCheckpoint> deferred_checkpoint;
  if (active_rank_checkpoint.has_value()) {
    ExactPairSupportStreamBudget deferred_budget = unlimited_budget();
    deferred_budget.maximum_auxiliary_frontier_entry_count = 1U;
    const ExactPairSupportStreamChunk deferred_chunk =
        build_exact_pair_support_stream_chunk(
            index,
            cloud,
            10U,
            deferred_budget,
            *active_rank_checkpoint);
    if (deferred_chunk.next_checkpoint.pending_product.has_value() &&
        deferred_chunk.next_checkpoint.pending_product
            ->deferred_expansion_node.has_value()) {
      deferred_checkpoint = deferred_chunk.next_checkpoint;
    }
    check(
        deferred_chunk.stop_reason ==
                ExactPairSupportStopReason::
                    auxiliary_frontier_entry_limit &&
            deferred_checkpoint.has_value(),
        "an auxiliary cap of one persists a decided internal witness expansion instead of replaying it");
  }
  if (deferred_checkpoint.has_value()) {
    ExactPairSupportStreamBudget undersized_resume = unlimited_budget();
    undersized_resume.maximum_auxiliary_frontier_entry_count = 0U;
    check_throws<std::invalid_argument>(
        [&] {
          static_cast<void>(build_exact_pair_support_stream_chunk(
              index,
              cloud,
              10U,
              undersized_resume,
              *deferred_checkpoint));
        },
        "resume rejects an auxiliary budget below the persisted witness cursor");
  }

  if (receipt_checkpoint.has_value()) {
    const auto reseal = [](ExactPairSupportCheckpoint checkpoint) {
      checkpoint.checkpoint_digest =
          compute_exact_pair_support_checkpoint_digest(checkpoint);
      return checkpoint;
    };
    const ExactPairSupportCheckpoint& valid = *receipt_checkpoint;
    check(
        valid.checkpoint_digest.to_lower_hex() ==
            "91ee653c988b4de2037921a9cde35539c15c726d82ba909515c72c748a2b482b",
        "checkpoint schema v1 preserves its golden mid-witness cursor digest");
    const auto valid_verification = verify_exact_pair_support_checkpoint(
        index, cloud, 10U, valid);
    check(
        valid_verification.integrity_verified,
        "the naturally persisted strict-witness receipt checkpoint verifies locally");

    ExactPairSupportCheckpoint overlapping_frontier = valid;
    overlapping_frontier.frontier.push_back(
        overlapping_frontier.frontier.back());
    overlapping_frontier.checkpoint_digest =
        compute_exact_pair_support_checkpoint_digest(overlapping_frontier);
    const auto overlapping_frontier_verification =
        verify_exact_pair_support_checkpoint(
            index, cloud, 10U, overlapping_frontier);
    check(
        overlapping_frontier_verification.checksum_matches_payload &&
            !overlapping_frontier_verification.frontier_locally_valid &&
            !overlapping_frontier_verification.integrity_verified,
        "a self-rehashed checkpoint cannot duplicate an unresolved product domain");

    const bool has_distinct_nonactive_frontier_entry =
        valid.frontier.size() > 1U &&
        valid.frontier.front() != valid.frontier.back();
    check(
        has_distinct_nonactive_frontier_entry,
        "the pending-product fixture exposes a distinct nonactive frontier entry");
    if (has_distinct_nonactive_frontier_entry) {
      ExactPairSupportCheckpoint detached_pending = valid;
      detached_pending.pending_product->product =
          detached_pending.frontier.front();
      detached_pending.checkpoint_digest =
          compute_exact_pair_support_checkpoint_digest(detached_pending);
      const auto detached_pending_verification =
          verify_exact_pair_support_checkpoint(
              index, cloud, 10U, detached_pending);
      check(
          detached_pending_verification.checksum_matches_payload &&
              detached_pending_verification.frontier_locally_valid &&
              !detached_pending_verification
                   .pending_product_locally_valid &&
              !detached_pending_verification.integrity_verified,
          "a self-rehashed checkpoint cannot detach its active product from the frontier back");
    }

    const auto rejects_forged_audit =
        [&](ExactPairSupportCheckpoint checkpoint,
            const std::string& message) {
          checkpoint = reseal(std::move(checkpoint));
          const auto verification = verify_exact_pair_support_checkpoint(
              index, cloud, 10U, checkpoint);
          check(
              verification.checksum_matches_payload &&
                  verification.pending_product_locally_valid &&
                  !verification.required_audit_identities_hold &&
                  !verification.integrity_verified,
              message);
        };
    ExactPairSupportCheckpoint missing_receipt_points = valid;
    missing_receipt_points.cumulative_audit
        .strict_interior_witness_point_count = 0U;
    rejects_forged_audit(
        std::move(missing_receipt_points),
        "a self-rehashed audit cannot omit persisted strict-witness points");
    ExactPairSupportCheckpoint missing_receipt_subtrees = valid;
    missing_receipt_subtrees.cumulative_audit
        .strict_interior_witness_subtree_count = 0U;
    rejects_forged_audit(
        std::move(missing_receipt_subtrees),
        "a self-rehashed audit cannot omit persisted strict-witness subtrees");
    ExactPairSupportCheckpoint missing_phi_visits = valid;
    missing_phi_visits.cumulative_audit.exact_phi_aabb_bound_count = 0U;
    rejects_forged_audit(
        std::move(missing_phi_visits),
        "a self-rehashed audit cannot omit the exact phi tests behind strict receipts");
    ExactPairSupportCheckpoint missing_rank_search = valid;
    missing_rank_search.cumulative_audit.rank_prune_search_count = 0U;
    rejects_forged_audit(
        std::move(missing_rank_search),
        "a self-rehashed audit cannot omit the active rank-prune search");
    ExactPairSupportCheckpoint missing_witness_peak = valid;
    missing_witness_peak.cumulative_audit
        .maximum_witness_frontier_entry_count = 0U;
    rejects_forged_audit(
        std::move(missing_witness_peak),
        "a self-rehashed audit cannot understate the persisted witness frontier peak");

    ExactPairSupportCheckpoint wrong_count = valid;
    ++wrong_count.pending_product->strict_witness_point_count;
    wrong_count = reseal(std::move(wrong_count));
    const auto wrong_count_verification =
        verify_exact_pair_support_checkpoint(
            index, cloud, 10U, wrong_count);
    check(
        wrong_count_verification.checksum_matches_payload &&
            !wrong_count_verification.pending_product_locally_valid &&
            !wrong_count_verification.integrity_verified,
        "a self-rehashed checkpoint cannot forge the strict-witness point sum");

    ExactPairSupportCheckpoint duplicate_receipt = valid;
    const ExactPairSupportWitnessNodeEntry duplicate =
        duplicate_receipt.pending_product->strict_witness_receipts.front();
    duplicate_receipt.pending_product->strict_witness_receipts.push_back(
        duplicate);
    duplicate_receipt.pending_product->strict_witness_point_count +=
        static_cast<std::size_t>(
            duplicate.leaf_end - duplicate.leaf_begin);
    duplicate_receipt = reseal(std::move(duplicate_receipt));
    const auto duplicate_verification =
        verify_exact_pair_support_checkpoint(
            index, cloud, 10U, duplicate_receipt);
    check(
        duplicate_verification.checksum_matches_payload &&
            !duplicate_verification.pending_product_locally_valid,
        "a self-rehashed checkpoint cannot duplicate a strict-witness range");

    if (observed_receipt_descendant_leaf.has_value()) {
      ExactPairSupportCheckpoint ancestor_descendant = valid;
      ancestor_descendant.pending_product->strict_witness_receipts.push_back(
          *observed_receipt_descendant_leaf);
      ancestor_descendant.pending_product->strict_witness_point_count += 1U;
      ancestor_descendant = reseal(std::move(ancestor_descendant));
      const auto ancestor_descendant_verification =
          verify_exact_pair_support_checkpoint(
              index, cloud, 10U, ancestor_descendant);
      check(
          ancestor_descendant_verification.checksum_matches_payload &&
              !ancestor_descendant_verification
                   .pending_product_locally_valid,
          "a self-rehashed checkpoint cannot count an ancestor receipt together with its leaf descendant");
    }

    ExactPairSupportCheckpoint receipt_active_overlap = valid;
    receipt_active_overlap.pending_product->witness_frontier.push_back(
        receipt_active_overlap.pending_product
            ->strict_witness_receipts.front());
    receipt_active_overlap = reseal(std::move(receipt_active_overlap));
    const auto receipt_active_verification =
        verify_exact_pair_support_checkpoint(
            index, cloud, 10U, receipt_active_overlap);
    check(
        receipt_active_verification.checksum_matches_payload &&
            !receipt_active_verification.pending_product_locally_valid,
        "a self-rehashed checkpoint cannot overlap strict receipts with the active witness frontier");

    ExactPairSupportCheckpoint support_overlap = valid;
    const auto& product = support_overlap.pending_product->product;
    support_overlap.pending_product->strict_witness_receipts.front() =
        ExactPairSupportWitnessNodeEntry{
            product.first_node_index,
            product.first_leaf_begin,
            product.first_leaf_end};
    std::size_t support_overlap_count = 0U;
    for (const auto& receipt :
         support_overlap.pending_product->strict_witness_receipts) {
      support_overlap_count += static_cast<std::size_t>(
          receipt.leaf_end - receipt.leaf_begin);
    }
    support_overlap.pending_product->strict_witness_point_count =
        support_overlap_count;
    support_overlap = reseal(std::move(support_overlap));
    const auto support_overlap_verification =
        verify_exact_pair_support_checkpoint(
            index, cloud, 10U, support_overlap);
    check(
        support_overlap_verification.checksum_matches_payload &&
            !support_overlap_verification.pending_product_locally_valid,
        "a self-rehashed checkpoint cannot count a support range as a strict witness");

    ExactPairSupportCheckpoint wrong_stage = valid;
    wrong_stage.pending_product->stage =
        ExactPairSupportPendingStage::classify_leaf;
    wrong_stage = reseal(std::move(wrong_stage));
    const auto wrong_stage_verification =
        verify_exact_pair_support_checkpoint(
            index, cloud, 10U, wrong_stage);
    check(
        wrong_stage_verification.checksum_matches_payload &&
            !wrong_stage_verification.pending_product_locally_valid,
        "a self-rehashed checkpoint cannot relabel an active rank cursor as a leaf classification");
  }

  if (deferred_checkpoint.has_value() && observed_leaf_entry.has_value()) {
    ExactPairSupportCheckpoint deferred_leaf = *deferred_checkpoint;
    deferred_leaf.pending_product->deferred_expansion_node =
        *observed_leaf_entry;
    deferred_leaf.checkpoint_digest =
        compute_exact_pair_support_checkpoint_digest(deferred_leaf);
    const auto deferred_leaf_verification =
        verify_exact_pair_support_checkpoint(
            index, cloud, 10U, deferred_leaf);
    check(
        deferred_leaf_verification.checksum_matches_payload &&
            !deferred_leaf_verification.pending_product_locally_valid &&
            !deferred_leaf_verification.integrity_verified,
        "a self-rehashed checkpoint rejects a leaf masquerading as a deferred internal expansion");
  }
}

void test_checkpoint_manifest_and_prepared_retry() {
  const std::vector<CertifiedPoint3> points{
      point(-1.0, 0.0), point(0.0, 1.0), point(1.0, 0.0)};
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportCheckpoint initial =
      make_initial_exact_pair_support_checkpoint(index, cloud, 1U);
  check(
      initial.manifest.canonical_cloud_digest.to_lower_hex() ==
              "97a45d5d6cf2c698ec22ed5fe3cadfdc3043ffc7ee035d65b6de8446693550ba" &&
          initial.manifest.lbvh_digest.to_lower_hex() ==
              "c70d65a1a04f78d1310ca78d1fe931901fa17a3cae8d9369343bb58904a9acb1" &&
          initial.manifest.semantic_digest.to_lower_hex() ==
              "829aabde192f6ab20bbb769810805f7ee3fc25dbe6de2ae038c01c6d0909ae14" &&
          initial.checkpoint_digest.to_lower_hex() ==
              "bbfedd6b2aae19bcfc66e507715785e63a1497a48bc82485cf3ae5230ca653dd",
      "checkpoint schema v1 preserves its golden cloud, LBVH, semantic, and initial-state digests");
  ExactPairSupportCheckpoint forged_initial_audit = initial;
  forged_initial_audit.cumulative_audit.rank_prune_search_count = 1U;
  forged_initial_audit.checkpoint_digest =
      compute_exact_pair_support_checkpoint_digest(forged_initial_audit);
  const auto forged_initial_audit_verification =
      verify_exact_pair_support_checkpoint(
          index, cloud, 1U, forged_initial_audit);
  check(
      forged_initial_audit_verification.checksum_matches_payload &&
          !forged_initial_audit_verification
               .required_audit_identities_hold &&
          !forged_initial_audit_verification.integrity_verified,
      "a self-rehashed initial checkpoint cannot invent a rank search without witness work");

  std::vector<CertifiedPoint3> reversed = points;
  std::reverse(reversed.begin(), reversed.end());
  const CanonicalPointCloud equivalent_cloud = cloud_from(reversed);
  const MortonLbvhIndex equivalent_index =
      MortonLbvhIndex::build(equivalent_cloud);
  check(
      verify_exact_pair_support_checkpoint(
          equivalent_index, equivalent_cloud, 1U, initial)
          .integrity_verified,
      "a reconstructed canonical cloud and LBVH accept the same compact manifest independently of input permutation");

  ExactPairSupportStreamBudget one_record_budget = unlimited_budget();
  one_record_budget.maximum_emitted_record_count = 1U;
  const ExactPairSupportStreamChunk first_attempt =
      build_exact_pair_support_stream_chunk(
          index, cloud, 1U, one_record_budget, initial);
  const ExactPairSupportStreamChunk retried_attempt =
      build_exact_pair_support_stream_chunk(
          index, cloud, 1U, one_record_budget, initial);
  check(
      first_attempt == retried_attempt &&
          initial.output_record_count == 0U &&
          first_attempt.next_checkpoint.output_record_count == 1U &&
          !first_attempt.next_checkpoint.complete(),
      "discarding an unpublished chunk leaves the source checkpoint authoritative and retry-equivalent in canonical value and digest");

  ExactPairSupportCheckpoint mutated = first_attempt.next_checkpoint;
  ++mutated.manifest.schema_version;
  mutated.checkpoint_digest =
      compute_exact_pair_support_checkpoint_digest(mutated);
  const auto schema_mutation_verification =
      verify_exact_pair_support_checkpoint(index, cloud, 1U, mutated);
  check(
      schema_mutation_verification.checksum_matches_payload &&
          !schema_mutation_verification.manifest_matches_authorities &&
          !schema_mutation_verification.integrity_verified,
      "checkpoint validation rejects a mutated manifest schema");
  ExactPairSupportCheckpoint unsealed = first_attempt.next_checkpoint;
  ++unsealed.cumulative_audit.resolved_pair_count;
  const auto unsealed_verification =
      verify_exact_pair_support_checkpoint(index, cloud, 1U, unsealed);
  check(
      !unsealed_verification.checksum_matches_payload &&
          !unsealed_verification.integrity_verified,
      "checkpoint validation rejects a payload mutation before semantic validation when its checksum is stale");
  mutated = first_attempt.next_checkpoint;
  ++mutated.cumulative_audit.resolved_pair_count;
  mutated.checkpoint_digest =
      compute_exact_pair_support_checkpoint_digest(mutated);
  const auto audit_mutation_verification =
      verify_exact_pair_support_checkpoint(index, cloud, 1U, mutated);
  check(
      audit_mutation_verification.checksum_matches_payload &&
          !audit_mutation_verification
               .required_audit_identities_hold &&
          !audit_mutation_verification.integrity_verified,
      "checkpoint validation rejects a mutated cumulative audit");
  mutated = first_attempt.next_checkpoint;
  ++mutated.frontier.front().first_leaf_end;
  mutated.checkpoint_digest =
      compute_exact_pair_support_checkpoint_digest(mutated);
  const auto range_mutation_verification =
      verify_exact_pair_support_checkpoint(index, cloud, 1U, mutated);
  check(
      range_mutation_verification.checksum_matches_payload &&
          !range_mutation_verification.frontier_locally_valid &&
          !range_mutation_verification.integrity_verified,
      "checkpoint validation rejects a stale Morton range");
  check(
      !verify_exact_pair_support_checkpoint(index, cloud, 2U, initial)
           .integrity_verified,
      "the checkpoint manifest binds the requested maximum order");

  const CanonicalPointCloud different_cloud = cloud_from(
      {point(-1.0, 0.0), point(0.0, 1.0), point(2.0, 0.0)});
  const MortonLbvhIndex different_index =
      MortonLbvhIndex::build(different_cloud);
  check(
      !verify_exact_pair_support_checkpoint(
           different_index, different_cloud, 1U, initial)
           .integrity_verified,
      "the checkpoint manifest rejects a one-coordinate cloud mutation");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(build_exact_pair_support_stream_chunk(
            index, cloud, 1U, one_record_budget, mutated));
      },
      "a corrupted checkpoint is rejected before any chunk work or append");

  const ChunkedPairRun chunked =
      run_chunked(index, cloud, 1U, one_record_budget);
  check(
      chunked.checkpoint.output_chain_digest.to_lower_hex() ==
              "8658c2f4ab4d3e560d9abaeee1ea433aa55e57f72d4c10a6fc7f9b6f11077fc8" &&
          chunked.checkpoint.checkpoint_digest.to_lower_hex() ==
              "1b2e20782ed262e0daa886aa5856ab5e86a56f2fe1126dd57c93ac023e717be3",
      "checkpoint schema v1 preserves its golden mixed-record output chain and terminal digest");
  const ExactPairSupportStreamChunk resident =
      build_exact_pair_support_stream_chunk(
          index, cloud, 1U, unlimited_budget(), initial);
  check(
      chunked.events == resident.events &&
          chunked.diagnostics ==
              resident.relevant_extra_shell_diagnostics &&
          chunked.record_order == resident.record_order &&
          chunked.events.size() == 2U &&
          chunked.diagnostics.size() == 1U &&
          chunked.checkpoint.output_record_count == 3U &&
          chunked.checkpoint.output_chain_digest ==
              resident.next_checkpoint.output_chain_digest,
      "one-record prepared transitions expose the right-triangle catalogue exactly once");

  check(
      chunked.chunks.size() > 1U,
      "the anchored-lineage fixture contains several prepared transitions");
  if (chunked.chunks.size() > 1U) {
    const ExactPairSupportStreamRunVerification incomplete =
        verify_exact_pair_support_stream_run(
            index,
            cloud,
            1U,
            std::span<const ExactPairSupportStreamBudget>{chunked.budgets}
                .first(chunked.budgets.size() - 1U),
            std::span<const ExactPairSupportStreamChunk>{chunked.chunks}
                .first(chunked.chunks.size() - 1U));
    check(
        incomplete.initial_checkpoint_reconstructed &&
            incomplete.every_transition_verified &&
            !incomplete.terminal_checkpoint_reached &&
            !incomplete.anchored_run_certified,
        "an exact but incomplete checkpoint prefix is not a certified terminal run");

    std::vector<ExactPairSupportStreamBudget> dropped_budgets{
        chunked.budgets.begin() + 1, chunked.budgets.end()};
    std::vector<ExactPairSupportStreamChunk> dropped_chunks{
        chunked.chunks.begin() + 1, chunked.chunks.end()};
    check(
        !verify_exact_pair_support_stream_run(
             index,
             cloud,
             1U,
             dropped_budgets,
             dropped_chunks)
             .anchored_run_certified,
        "an anchored run rejects a missing first transition");

    std::vector<ExactPairSupportStreamBudget> reordered_budgets =
        chunked.budgets;
    std::vector<ExactPairSupportStreamChunk> reordered_chunks =
        chunked.chunks;
    std::swap(reordered_budgets[0], reordered_budgets[1]);
    std::swap(reordered_chunks[0], reordered_chunks[1]);
    check(
        !verify_exact_pair_support_stream_run(
             index,
             cloud,
             1U,
             reordered_budgets,
             reordered_chunks)
             .anchored_run_certified,
        "an anchored run rejects reordered transitions");

    std::vector<ExactPairSupportStreamBudget> duplicated_budgets =
        chunked.budgets;
    std::vector<ExactPairSupportStreamChunk> duplicated_chunks =
        chunked.chunks;
    duplicated_budgets.insert(
        duplicated_budgets.begin() + 1, duplicated_budgets.front());
    duplicated_chunks.insert(
        duplicated_chunks.begin() + 1, duplicated_chunks.front());
    check(
        !verify_exact_pair_support_stream_run(
             index,
             cloud,
             1U,
             duplicated_budgets,
             duplicated_chunks)
             .anchored_run_certified,
        "an anchored run rejects a duplicated transition");

    std::vector<ExactPairSupportStreamChunk> wrong_source_chunks =
        chunked.chunks;
    wrong_source_chunks.front().source_checkpoint_digest = {};
    check(
        !verify_exact_pair_support_stream_run(
             index,
             cloud,
             1U,
             chunked.budgets,
             wrong_source_chunks)
             .anchored_run_certified,
        "an anchored run rejects a mutated source-checkpoint digest");

    std::vector<ExactPairSupportStreamChunk> wrong_order_chunks =
        chunked.chunks;
    const auto ordered_chunk = std::find_if(
        wrong_order_chunks.begin(),
        wrong_order_chunks.end(),
        [](const ExactPairSupportStreamChunk& chunk) {
          return !chunk.record_order.empty();
        });
    check(
        ordered_chunk != wrong_order_chunks.end(),
        "the mixed-record fixture exposes a typed record ordinal");
    if (ordered_chunk != wrong_order_chunks.end()) {
      ordered_chunk->record_order.front() =
          ordered_chunk->record_order.front() ==
                  ExactPairSupportStreamChunk::RecordKind::event
              ? ExactPairSupportStreamChunk::RecordKind::
                    relevant_extra_shell_diagnostic
              : ExactPairSupportStreamChunk::RecordKind::event;
      check(
          !verify_exact_pair_support_stream_run(
               index,
               cloud,
               1U,
               chunked.budgets,
               wrong_order_chunks)
               .anchored_run_certified,
          "an anchored run rejects a mutated inter-type record order");
    }
  }

  ExactPairSupportCheckpoint forged_terminal = chunked.checkpoint;
  forged_terminal.next_chunk_sequence += 7U;
  forged_terminal.checkpoint_digest =
      compute_exact_pair_support_checkpoint_digest(forged_terminal);
  check(
      verify_exact_pair_support_checkpoint(
          index, cloud, 1U, forged_terminal)
          .integrity_verified,
      "a self-rehashed terminal checkpoint can have local integrity without proving lineage");
  const ExactPairSupportStreamBudget no_op_budget{};
  const ExactPairSupportStreamChunk forged_no_op =
      build_exact_pair_support_stream_chunk(
          index, cloud, 1U, no_op_budget, forged_terminal);
  const std::array<ExactPairSupportStreamBudget, 1U> forged_budgets{
      no_op_budget};
  const std::array<ExactPairSupportStreamChunk, 1U> forged_chunks{
      forged_no_op};
  check(
      !verify_exact_pair_support_stream_run(
           index,
           cloud,
           1U,
           forged_budgets,
           forged_chunks)
           .anchored_run_certified,
      "a locally valid terminal checkpoint without descent from the reconstructed initial state is not an anchored run");
}

void test_terminal_checkpoint_is_idempotent() {
  const CanonicalPointCloud cloud = cloud_from({point(0.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportCheckpoint initial =
      make_initial_exact_pair_support_checkpoint(index, cloud, 10U);
  const ExactPairSupportStreamChunk terminal =
      build_exact_pair_support_stream_chunk(
          index, cloud, 10U, ExactPairSupportStreamBudget{}, initial);
  check(
      initial.complete() && terminal.relative_stream_complete() &&
          terminal.events.empty() &&
          terminal.relevant_extra_shell_diagnostics.empty() &&
          terminal.next_checkpoint == initial &&
          terminal.chunk_sequence == 0U,
      "a singleton checkpoint is already terminal and a no-op resume does not advance its sequence");
  const ExactPairSupportStreamRunVerification empty_anchored_run =
      verify_exact_pair_support_stream_run(
          index,
          cloud,
          10U,
          std::span<const ExactPairSupportStreamBudget>{},
          std::span<const ExactPairSupportStreamChunk>{});
  const std::array<ExactPairSupportStreamBudget, 1U> redundant_budgets{
      ExactPairSupportStreamBudget{}};
  const std::array<ExactPairSupportStreamChunk, 1U> redundant_chunks{
      terminal};
  check(
      empty_anchored_run.anchored_run_certified &&
          !verify_exact_pair_support_stream_run(
               index,
               cloud,
               10U,
               redundant_budgets,
               redundant_chunks)
               .anchored_run_certified,
      "the anchored singleton run is canonically empty and rejects a redundant terminal no-op transition");
}

void test_bounded_exhaustive_oracle_agreement() {
  std::vector<CertifiedPoint3> points;
  points.reserve(14U);
  for (std::size_t index = 0U; index < 14U; ++index) {
    const double x = static_cast<double>(index) - 7.0;
    const double y = static_cast<double>((index * index + 3U) % 17U) - 8.0;
    const double z =
        static_cast<double>((index * index * index + 5U) % 19U) - 9.0;
    points.push_back(point(x, y, z));
  }
  const CanonicalPointCloud cloud = cloud_from(points);
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  for (const std::size_t maximum_order : {1U, 4U, 9U, 10U}) {
    const ExactPairSupportStreamResult result =
        build_exact_pair_support_stream(
            index, cloud, maximum_order, budget);
    const OraclePairRecords oracle =
        brute_force_pair_records(cloud, maximum_order);
    check(
        result.events == oracle.events &&
            result.relevant_extra_shell_diagnostics == oracle.diagnostics,
        "the direct pair stream agrees with exhaustive closed-ball enumeration at n=14");
    if (maximum_order == 1U) {
      check(
          result.audit.rank_pruned_product_count > 0U &&
              result.audit.rank_pruned_pair_count > 0U &&
              result.audit.exact_anchor_ball_minimum_aabb_bound_count > 0U &&
              result.audit.certified_anchor_noninterior_subtree_count > 0U &&
              result.audit.certified_anchor_noninterior_point_count > 0U,
          "the n=14 K=1 differential exercises strict rank pruning and safe real-anchor witness exclusion");
    }
    check_complete_accounting(
        result,
        "the n=14 differential closes its complete pair partition");
    check_fresh_replay(
        index,
        cloud,
        maximum_order,
        budget,
        result,
        "the n=14 differential passes exact fresh replay");
  }
}

void test_hostile_replay_mutations() {
  const CanonicalPointCloud cloud = cloud_from(
      {point(-1.0, 0.0), point(0.0, 1.0), point(1.0, 0.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const ExactPairSupportStreamBudget budget = unlimited_budget();
  const ExactPairSupportStreamResult complete =
      build_exact_pair_support_stream(index, cloud, 2U, budget);
  check(
      !complete.events.empty() &&
          !complete.relevant_extra_shell_diagnostics.empty(),
      "the replay-mutation fixture exposes both record kinds");

  ExactPairSupportStreamResult mutated = complete;
  mutated.budget.maximum_work_unit_count -= 1U;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a mutated embedded budget");

  mutated = complete;
  mutated.events.front().center = ExactRational3{
      BigInt{1}, BigInt{0}, BigInt{0}, BigInt{1}};
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a mutated event center");

  mutated = complete;
  ++mutated.relevant_extra_shell_diagnostics.front().shell_count;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a mutated sparse shell count");

  mutated = complete;
  ++mutated.audit.resolved_pair_count;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects mutated pair accounting");

  mutated = complete;
  mutated.hierarchy_reduction_performed = true;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a fabricated hierarchy reduction claim");

  mutated = complete;
  mutated.no_forbidden_global_structure_materialized = false;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a removed no-mosaic architecture assertion");

  ExactPairSupportStreamBudget partial_budget = unlimited_budget();
  partial_budget.maximum_work_unit_count = 0U;
  const ExactPairSupportStreamResult partial =
      build_exact_pair_support_stream(
          index, cloud, 2U, partial_budget);
  check(
      !partial.remaining_frontier.empty(),
      "the partial replay fixture retains a frontier entry");
  mutated = partial;
  ++mutated.remaining_frontier.front().first_leaf_end;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, partial_budget, mutated).result_certified,
      "fresh replay rejects a mutated residual Morton range");

  mutated = complete;
  mutated.status = ExactPairSupportStreamStatus::budget_exhausted;
  check(
      !verify_exact_pair_support_stream(
           index, cloud, 2U, budget, mutated).result_certified,
      "fresh replay rejects a false completion status");
}

}  // namespace

int main() {
  test_exact_phi_aabb_maximum();
  test_complete_self_dual_partition_and_long_pair();
  test_leaf_rank_cap_and_sparse_queries();
  test_extra_shell_and_equality_descent();
  test_real_anchor_shell_equality_exclusion();
  test_transactional_budgets();
  test_resumable_chunks_match_resident_stream();
  test_persistent_authority_and_incremental_verifier();
  test_every_prepared_stop_resumes_without_recount();
  test_resume_inside_rank_witness_search();
  test_pending_witness_checkpoint_invariants();
  test_checkpoint_manifest_and_prepared_retry();
  test_terminal_checkpoint_is_idempotent();
  test_bounded_exhaustive_oracle_agreement();
  test_hostile_replay_mutations();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "pair-support stream tests passed\n";
  return 0;
}
