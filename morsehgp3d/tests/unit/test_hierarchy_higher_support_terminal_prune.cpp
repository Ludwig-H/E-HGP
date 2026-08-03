#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/hierarchy/higher_support_product.hpp"
#include "morsehgp3d/hierarchy/higher_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactRational;
using morsehgp3d::hierarchy::ExactHigherSupportAnchoredSession;
using morsehgp3d::hierarchy::ExactHigherSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactHigherSupportCheckpoint;
using morsehgp3d::hierarchy::ExactHigherSupportFrontierEntry;
using morsehgp3d::hierarchy::ExactHigherSupportPendingStage;
using morsehgp3d::hierarchy::ExactHigherSupportPruneReason;
using morsehgp3d::hierarchy::ExactHigherSupportStreamBudget;
using morsehgp3d::hierarchy::ExactHigherSupportStreamChunk;
using morsehgp3d::hierarchy::build_exact_higher_support_stream;
using morsehgp3d::hierarchy::compute_exact_higher_support_checkpoint_digest;
using morsehgp3d::hierarchy::exact_higher_support_product_aabb_analysis;
using morsehgp3d::hierarchy::verify_exact_higher_support_checkpoint;
using morsehgp3d::hierarchy::verify_exact_higher_support_stream;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] CanonicalPointCloud cloud_from(
    const std::vector<CertifiedPoint3>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return ExactHigherSupportStreamBudget{
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum};
}

[[nodiscard]] bool terminal_product(
    const ExactHigherSupportFrontierEntry& product) {
  if (product.group_count != product.support_size) {
    return false;
  }
  for (std::size_t index = 0U; index < product.group_count; ++index) {
    const auto& group = product.groups[index];
    if (group.multiplicity != 1U ||
        group.leaf_end != group.leaf_begin + 1U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool strict_terminal_rank_certificate(
    const morsehgp3d::hierarchy::ExactHigherSupportPruneCertificate&
        certificate) {
  if (certificate.reason !=
          ExactHigherSupportPruneReason::strict_interior_rank_bound ||
      certificate.pruned_support_count != 1 ||
      !terminal_product(certificate.product) ||
      certificate.rank_receipts.empty() ||
      certificate.required_strict_interior_point_count !=
          5U - certificate.product.support_size) {
    return false;
  }
  morsehgp3d::exact::BigInt authenticated_count{0};
  for (const auto& receipt : certificate.rank_receipts) {
    if (!receipt.query_product_analysis.query_scaled_power.has_value() ||
        !(receipt.query_product_analysis.query_scaled_power->upper <
          ExactRational{}) ||
        !receipt.query_product_analysis
             .query_strictly_inside_every_independent_sphere_certified()) {
      return false;
    }
    for (std::size_t group_index = 0U;
         group_index < certificate.product.group_count;
         ++group_index) {
      const auto& group = certificate.product.groups[group_index];
      if (receipt.query_node.leaf_begin < group.leaf_end &&
          group.leaf_begin < receipt.query_node.leaf_end) {
        return false;
      }
    }
    authenticated_count += receipt.certified_point_count;
  }
  return authenticated_count ==
             certificate.certified_strict_interior_point_count &&
      authenticated_count >=
          certificate.required_strict_interior_point_count;
}

[[nodiscard]] bool has_event(
    const std::vector<morsehgp3d::hierarchy::ExactHigherSupportEvent>& events,
    std::uint8_t support_size,
    std::array<PointId, 4U> support_ids) {
  return std::any_of(events.begin(), events.end(), [&](const auto& event) {
    return event.support_size == support_size &&
        event.support_ids == support_ids;
  });
}

[[nodiscard]] ExactDyadicAabb3 singleton_box(
    const CertifiedPoint3& certified_point) {
  ExactDyadicAabb3 box;
  box.lower_binary64_bits = certified_point.canonical_input_bits();
  box.upper_binary64_bits = certified_point.canonical_input_bits();
  return box;
}

[[nodiscard]] PointId point_id_for(
    const CanonicalPointCloud& cloud,
    const CertifiedPoint3& expected) {
  for (PointId point_id = 0U; point_id < cloud.size(); ++point_id) {
    if (cloud.point(point_id).exact() == expected.exact()) {
      return point_id;
    }
  }
  throw std::logic_error("the canonical cloud omitted an expected point");
}

void test_terminal_rank_prune_precedes_closed_ball() {
  const CanonicalPointCloud cloud = cloud_from({
      point(10.0, 10.0, 10.0),
      point(10.125, 10.0, 10.0),
      point(10.0, -10.0, -10.0),
      point(10.125, -10.0, -10.0),
      point(-10.0, 10.0, -10.0),
      point(-9.875, 10.0, -10.0),
      point(-10.0, -10.0, 10.0),
      point(-9.875, -10.0, 10.0),
      point(0.0, 0.0, 0.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto result = build_exact_higher_support_stream(
      index, cloud, 3U, unlimited_budget());

  const std::size_t terminal_rank_prune_count =
      static_cast<std::size_t>(std::count_if(
          result.prune_certificates.begin(),
          result.prune_certificates.end(),
          [](const auto& certificate) {
            return strict_terminal_rank_certificate(certificate);
          }));
  std::size_t maximum_receipt_point_count = 0U;
  for (const auto& certificate : result.prune_certificates) {
    for (const auto& receipt : certificate.rank_receipts) {
      maximum_receipt_point_count = std::max(
          maximum_receipt_point_count,
          receipt.certified_point_count);
    }
  }

  check(
      result.stream_complete() && terminal_rank_prune_count == 12U &&
          result.audit.rank_pruned_support_count == 44 &&
          result.audit.above_rank_leaf_count == 0U,
      "twelve above-rank singleton supports are rank-pruned before leaf classification");
  check(
      result.audit.global_closed_ball_query_count == 4U &&
          result.audit.global_closed_ball_query_count +
                  terminal_rank_prune_count ==
              16U,
      "terminal W certificates remove twelve exact closed-ball queries from the regression baseline");
  check(
      result.no_forbidden_global_structure_materialized &&
          result.audit.exact_bigint_universe_certified &&
          result.audit.grouped_partition_accounting_certified &&
          result.audit.resolved_support_count ==
              result.audit.total_support_count,
      "terminal-first pruning preserves the sparse BigInt product partition without a global catalog");
  std::cout << "terminal-rank metrics: closed_ball_queries="
            << result.audit.global_closed_ball_query_count
            << " avoided=" << terminal_rank_prune_count
            << " maximum_receipt_mass=" << maximum_receipt_point_count
            << '\n';
}

void test_bulk_morton_cell_rank_receipt() {
  const CanonicalPointCloud cloud = cloud_from({
      point(16.0, 16.0, 16.0),
      point(16.0, -16.0, -16.0),
      point(-16.0, 16.0, -16.0),
      point(5.0, 5.0, 5.0),
      point(5.125, 5.0, 5.0),
      point(5.0, 5.125, 5.0),
      point(5.0, 5.0, 5.125)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto result = build_exact_higher_support_stream(
      index, cloud, 5U, unlimited_budget());
  std::size_t maximum_receipt_point_count = 0U;
  for (const auto& certificate : result.prune_certificates) {
    for (const auto& receipt : certificate.rank_receipts) {
      maximum_receipt_point_count = std::max(
          maximum_receipt_point_count,
          receipt.certified_point_count);
    }
  }
  check(
      result.stream_complete() && maximum_receipt_point_count > 1U &&
          result.audit.maximum_rank_frontier_entry_count == 0U &&
          verify_exact_higher_support_stream(
              index, cloud, 5U, unlimited_budget(), result)
              .result_certified,
      "one bounded exact Morton-cell receipt bulk-certifies several strict witnesses");
  std::cout << "bulk-cell metrics: maximum_receipt_mass="
            << maximum_receipt_point_count << '\n';

  const ExactHigherSupportAuthorityContext authority{index, cloud, 5U};
  ExactHigherSupportAnchoredSession session{authority};
  ExactHigherSupportCheckpoint checkpoint = session.trusted_checkpoint();
  ExactHigherSupportStreamBudget unit_budget = unlimited_budget();
  unit_budget.maximum_work_unit_count = 1U;
  bool observed_mixed_antichain = false;
  for (std::size_t step = 0U;
       step < 8192U && !checkpoint.locally_complete() &&
       !observed_mixed_antichain;
       ++step) {
    const ExactHigherSupportStreamChunk chunk =
        session.prepare_next(unit_budget, checkpoint);
    const auto transition =
        session.commit_prepared(unit_budget, checkpoint, chunk);
    check(
        transition.chunk_transition_verified,
        "the bulk-cell fixture resumes one local rank node at a time");
    checkpoint = session.trusted_checkpoint();
    if (!checkpoint.pending_product.has_value()) {
      continue;
    }
    const auto& pending = *checkpoint.pending_product;
    if (pending.stage != ExactHigherSupportPendingStage::rank_search ||
        !pending.rank_frontier.empty() ||
        !pending.rank_probe_fallback_active) {
      continue;
    }
    observed_mixed_antichain =
        verify_exact_higher_support_checkpoint(authority, checkpoint)
            .local_integrity_verified;
    ExactHigherSupportCheckpoint overlapping = checkpoint;
    const auto& first_group =
        overlapping.pending_product->product.groups[0];
    overlapping.pending_product->rank_frontier.push_back(
        morsehgp3d::hierarchy::ExactHigherSupportNodeReceipt{
            first_group.node_index,
            first_group.leaf_begin,
            first_group.leaf_end});
    overlapping.checkpoint_digest =
        compute_exact_higher_support_checkpoint_digest(overlapping);
    check(
        !verify_exact_higher_support_checkpoint(authority, overlapping)
             .local_integrity_verified,
        "a self-rehashed rank frontier fails closed on the bounded cursor path");
  }
  check(
      observed_mixed_antichain,
      "a checkpoint recertifies an inconclusive-cell singleton-fallback cursor");
}

void test_triangle_side_rank_counterexample_survives() {
  const std::array<CertifiedPoint3, 3U> target{
      point(10.0, 10.0, 10.0),
      point(10.0, -10.0, -10.0),
      point(-10.0, 10.0, -10.0)};
  const CanonicalPointCloud cloud = cloud_from({
      target[0],
      target[1],
      target[2],
      point(17.0, 9.0, 5.0),
      point(18.0, 8.0, 5.0),
      point(3.0, 18.0, 5.0),
      point(3.0, 19.0, 6.0),
      point(-6.0, -8.0, -15.0),
      point(-7.0, -9.0, -14.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto result = build_exact_higher_support_stream(
      index, cloud, 2U, unlimited_budget());
  std::array<PointId, 4U> target_ids{
      point_id_for(cloud, target[0]),
      point_id_for(cloud, target[1]),
      point_id_for(cloud, target[2]),
      0U};
  std::sort(target_ids.begin(), target_ids.begin() + 3);
  check(
      result.stream_complete() &&
          has_event(result.events, 3U, target_ids),
      "the exact rank-three triangle survives although all three sides have closed rank four");
}

void test_tetrahedron_face_filters_are_not_used() {
  const CanonicalPointCloud necessary_counterexample = cloud_from({
      point(0.0, 0.0, 0.0),
      point(-2.0, -2.0, -1.0),
      point(-2.0, 1.0, 0.0),
      point(0.0, -1.0, -1.0)});
  const MortonLbvhIndex necessary_index =
      MortonLbvhIndex::build(necessary_counterexample);
  const auto necessary = build_exact_higher_support_stream(
      necessary_index,
      necessary_counterexample,
      3U,
      unlimited_budget());
  check(
      necessary.stream_complete() &&
          has_event(necessary.events, 4U, {0U, 1U, 2U, 3U}),
      "a well-centred tetrahedron with two obtuse faces is retained by direct support analysis");

  const std::vector<CertifiedPoint3> insufficient_points{
      point(0.0, 0.0, 0.0),
      point(-2.0, -2.0, -1.0),
      point(-2.0, 0.0, 2.0),
      point(-2.0, 2.0, -1.0)};
  const CanonicalPointCloud insufficient_counterexample =
      cloud_from(insufficient_points);
  const MortonLbvhIndex insufficient_index =
      MortonLbvhIndex::build(insufficient_counterexample);
  const auto insufficient = build_exact_higher_support_stream(
      insufficient_index,
      insufficient_counterexample,
      3U,
      unlimited_budget());
  std::array<ExactDyadicAabb3, 4U> boxes{};
  for (std::size_t index = 0U; index < boxes.size(); ++index) {
    boxes[index] = singleton_box(insufficient_points[index]);
  }
  const auto direct_analysis = exact_higher_support_product_aabb_analysis(
      std::span<const ExactDyadicAabb3>{boxes});
  check(
      insufficient.stream_complete() &&
          !has_event(insufficient.events, 4U, {0U, 1U, 2U, 3U}) &&
          direct_analysis.no_well_centered_support_certified(),
      "four acute faces do not admit a tetrahedron whose direct barycentric support test is negative");
}

void test_query_power_equality_fails_open() {
  const std::array<CertifiedPoint3, 3U> support{
      point(1.0, 0.0, 0.0),
      point(-1.0, 0.0, 0.0),
      point(0.0, 1.0, 0.0)};
  const CertifiedPoint3 query = point(0.0, -1.0, 0.0);
  std::array<ExactDyadicAabb3, 3U> support_boxes{};
  for (std::size_t index = 0U; index < support.size(); ++index) {
    support_boxes[index] = singleton_box(support[index]);
  }
  const auto analysis = exact_higher_support_product_aabb_analysis(
      std::span<const ExactDyadicAabb3>{support_boxes},
      singleton_box(query));
  check(
      analysis.query_scaled_power.has_value() &&
          analysis.query_scaled_power->lower == ExactRational{} &&
          analysis.query_scaled_power->upper == ExactRational{} &&
          !analysis
               .query_strictly_inside_every_independent_sphere_certified(),
      "an exactly cospherical Morton witness has zero power and fails open");
}

template <typename Record>
void sort_records(std::vector<Record>& records) {
  std::sort(records.begin(), records.end(), [](const auto& left,
                                               const auto& right) {
    if (left.support_size != right.support_size) {
      return left.support_size < right.support_size;
    }
    return left.support_ids < right.support_ids;
  });
}

void test_unit_work_resume_through_terminal_rank_cursor() {
  const CanonicalPointCloud cloud = cloud_from({
      point(10.0, 10.0, 10.0),
      point(10.125, 10.0, 10.0),
      point(10.0, -10.0, -10.0),
      point(10.125, -10.0, -10.0),
      point(-10.0, 10.0, -10.0),
      point(-9.875, 10.0, -10.0),
      point(-10.0, -10.0, 10.0),
      point(-9.875, -10.0, 10.0),
      point(0.0, 0.0, 0.0)});
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto resident = build_exact_higher_support_stream(
      index, cloud, 3U, unlimited_budget());
  const ExactHigherSupportAuthorityContext authority{index, cloud, 3U};
  ExactHigherSupportAnchoredSession session{authority};
  ExactHigherSupportCheckpoint checkpoint = session.trusted_checkpoint();
  ExactHigherSupportStreamBudget unit_budget = unlimited_budget();
  unit_budget.maximum_work_unit_count = 1U;
  unit_budget.maximum_frontier_entry_count = 128U;
  unit_budget.maximum_auxiliary_frontier_entry_count =
      index.build_counters().maximum_depth + 1U;
  std::vector<morsehgp3d::hierarchy::ExactHigherSupportEvent> events;
  std::vector<
      morsehgp3d::hierarchy::ExactHigherSupportExtraShellDiagnostic>
      diagnostics;
  bool observed_terminal_rank_cursor = false;
  std::size_t chunk_count = 0U;
  while (!checkpoint.locally_complete() && chunk_count < 65536U) {
    ExactHigherSupportStreamChunk chunk =
        session.prepare_next(unit_budget, checkpoint);
    const auto verification =
        session.commit_prepared(unit_budget, checkpoint, chunk);
    check(
        verification.chunk_transition_verified,
        "each one-work-unit terminal-prune transition is exactly replayed");
    events.insert(events.end(), chunk.events.begin(), chunk.events.end());
    diagnostics.insert(
        diagnostics.end(),
        chunk.relevant_extra_shell_diagnostics.begin(),
        chunk.relevant_extra_shell_diagnostics.end());
    checkpoint = session.trusted_checkpoint();
    if (checkpoint.pending_product.has_value()) {
      const auto& pending = *checkpoint.pending_product;
      observed_terminal_rank_cursor =
          observed_terminal_rank_cursor ||
          (pending.stage == ExactHigherSupportPendingStage::rank_search &&
           terminal_product(pending.product));
    }
    ++chunk_count;
  }
  sort_records(events);
  sort_records(diagnostics);
  check(
      checkpoint.locally_complete() && chunk_count > 1U,
      "the one-work-unit schedule reaches a terminal checkpoint");
  check(
      observed_terminal_rank_cursor,
      "the bounded checkpoint exposes and resumes a terminal cell-or-fallback cursor");
  check(
      events == resident.events &&
          diagnostics == resident.relevant_extra_shell_diagnostics,
      "chunk boundaries do not change exact higher-support records");
  check(
      checkpoint.cumulative_audit == resident.audit,
      "chunk boundaries do not change exact higher-support audits");
}

}  // namespace

int main() {
  test_terminal_rank_prune_precedes_closed_ball();
  test_bulk_morton_cell_rank_receipt();
  test_triangle_side_rank_counterexample_survives();
  test_tetrahedron_face_filters_are_not_used();
  test_query_power_equality_fails_open();
  test_unit_work_resume_through_terminal_rank_cursor();
  if (failures != 0) {
    std::cerr << failures << " higher-support terminal-prune test(s) failed\n";
    return 1;
  }
  std::cout << "higher-support terminal prune tests passed\n";
  return 0;
}
