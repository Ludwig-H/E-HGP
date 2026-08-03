#include "morsehgp3d/hierarchy/direct_sparse_carrier_origin_capability.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

static_assert(
    !std::is_copy_constructible_v<
        ExactDirectSparseCarrierOriginCapabilityResult>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactDirectSparseCarrierOriginCapabilityResult>);
static_assert(
    ExactDirectSparseCarrierOriginCapabilityResult::mode ==
    std::string_view{"exact_sparse_carrier_origin_capability_v1"});
static_assert(
    ExactDirectSparseCarrierOriginCapabilityResult::public_status ==
    std::string_view{"not_claimed"});

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CanonicalId digest(std::string_view text) {
  CanonicalSha256Builder builder;
  builder.update(text);
  return builder.finalize();
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

[[nodiscard]] ExactDirectSparseFacetKey key(
    std::initializer_list<PointId> points) {
  ExactDirectSparseFacetKey output;
  output.point_count = points.size();
  std::copy(points.begin(), points.end(), output.point_ids.begin());
  return output;
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return {BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] CanonicalPointCloud cloud() {
  const std::array<CertifiedPoint3, 5U> points{
      point(0.0, 0.0, 7.0),
      point(0.0, 9.0, 6.0),
      point(1.0, 4.0, 0.0),
      point(0.0, 0.0, 1.0),
      point(4.0, 1.0, 2.0),
  };
  return CanonicalPointCloud::rejecting_duplicates(points);
}

[[nodiscard]] ExactDirectSparseStableFacetForestBudget forest_budget() {
  return {128U, 512U, 1024U, 64U, 64U,
          64U,  512U, 512U, 1024U, 16U};
}

[[nodiscard]] ExactDirectSparseRootLedgerBudget ledger_budget() {
  return {128U, 512U, 512U, 1024U, 2048U, 1024U, 64U,
          64U,  64U,  256U, 512U, 256U,  2048U, 8U};
}

[[nodiscard]] ExactDirectSparseStableFacetForestPreparedPreviewBudget
preview_budget() {
  return {128U, 512U, 256U, 640U};
}

[[nodiscard]] ExactDirectSparseStableFacetDescentClosureBudget
closure_budget() {
  const ExactDirectSparseStableFacetPositiveProbeBudget probe{
      64U, 64U, 64U, 64U};
  return {
      16U,
      16U,
      16U,
      33U,
      4096U,
      probe,
      {1024U, 1024U, 1024U, 1024U, 64U, 16U, 16U},
  };
}

struct Context {
  ExactDirectSparseStableFacetForest forest;
  ExactDirectSparseRootLedger ledger;
  ExactDirectSparseRootLedger empty_sibling_ledger;
};

[[nodiscard]] Context initialize_context() {
  auto forest = initialize_exact_direct_sparse_stable_facet_forest(
      {128U, digest("carrier-origin-source"), 0U}, forest_budget());
  check(
      forest.certified_initialized() && forest.forest.has_value(),
      "the carrier-origin forest initializes");
  auto ledger = initialize_exact_direct_sparse_root_ledger(
      {1U, 0U, 0U}, ledger_budget(), *forest.forest);
  auto sibling = initialize_exact_direct_sparse_root_ledger(
      {1U, 0U, 0U}, ledger_budget(), *forest.forest);
  check(
      ledger.certified_initialized() && sibling.certified_initialized() &&
          ledger.ledger.has_value() && sibling.ledger.has_value(),
      "two independent empty ledgers initialize on the same empty forest");
  return {
      std::move(*forest.forest),
      std::move(*ledger.ledger),
      std::move(*sibling.ledger)};
}

struct PreparedForest {
  ExactDirectSparseStableFacetForestPreparedBatch ticket;
  ExactDirectSparseStableFacetForestPreparedPreview preview;
};

[[nodiscard]] PreparedForest prepare_forest(
    ExactDirectSparseStableFacetForest& forest,
    std::span<const ExactDirectSparseStableFacetInsertion> insertions,
    std::span<const ExactDirectSparseStableFacetUnion> unions,
    std::span<const ExactDirectSparseStableFacetHandle> handles) {
  auto prepared = forest.prepare_batch(insertions, unions);
  check(
      prepared.certified_prepared() && prepared.ticket.has_value(),
      "the fixture forest batch prepares");
  auto preview = forest.preview_prepared_batch(
      *prepared.ticket, handles, preview_budget());
  check(
      preview.certified_prepared_preview() && preview.preview.has_value(),
      "the fixture forest preview certifies");
  return {std::move(*prepared.ticket), std::move(*preview.preview)};
}

void commit_births(
    Context& context,
    std::span<const ExactDirectSparseStableFacetInsertion> insertions,
    std::span<const ExactDirectSparseRootLedgerStandaloneBirth> births,
    std::span<const PointId> points) {
  std::vector<ExactDirectSparseStableFacetHandle> handles;
  handles.reserve(insertions.size());
  for (const auto& insertion : insertions) {
    handles.push_back(insertion.stable_source_facet_token_index);
  }
  auto forest = prepare_forest(context.forest, insertions, {}, handles);
  auto ledger = context.ledger.prepare_transition(
      context.forest,
      std::move(forest.ticket),
      std::move(forest.preview),
      {},
      {},
      {},
      births,
      points);
  check(
      ledger.certified_prepared() && ledger.ticket.has_value(),
      "latent fixture births prepare in the ledger");
  const auto committed = context.ledger.commit(
      std::move(*ledger.ticket), context.forest);
  check(committed.certified_commit(), "latent fixture births commit");
}

void commit_rooting_transition(Context& context) {
  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{3U, key({0U, 4U})}};
  const std::array unions{
      ExactDirectSparseStableFacetUnion{2U, 3U}};
  const std::array<ExactDirectSparseStableFacetHandle, 2U> handles{2U, 3U};
  auto forest = prepare_forest(context.forest, insertions, unions, handles);
  const std::array groups{
      ExactDirectSparseRootLedgerGroupTransition{0U, 3U, 0U, 1U,
                                                  0U, 1U, 1U}};
  // parent_root_count counts carrier handles, while q_R counts only parents
  // that already own a PriorRootId.  Handle 2 is still latent here, hence
  // one carrier parent and q_R=0 are deliberately simultaneous.
  const std::array<ExactDirectSparseStableFacetHandle, 1U> parents{2U};
  const std::array<PointId, 1U> delta{4U};
  auto ledger = context.ledger.prepare_transition(
      context.forest,
      std::move(forest.ticket),
      std::move(forest.preview),
      groups,
      parents,
      delta,
      {},
      {});
  check(
      ledger.certified_prepared() && ledger.ticket.has_value(),
      "qR=0 fixture transition prepares");
  const auto committed = context.ledger.commit(
      std::move(*ledger.ticket), context.forest);
  check(
      committed.certified_commit() &&
          context.ledger.lookup_active_root(2U).entry.prior_root_id == 1U,
      "qR=0 makes root 2 rooted while root 30 remains latent");
}

[[nodiscard]] Context make_context() {
  auto context = initialize_context();
  const std::array insertions{
      ExactDirectSparseStableFacetInsertion{2U, key({0U, 1U})},
      ExactDirectSparseStableFacetInsertion{30U, key({2U, 4U})},
  };
  const std::array births{
      ExactDirectSparseRootLedgerStandaloneBirth{2U, 0U, 2U, 1U},
      ExactDirectSparseRootLedgerStandaloneBirth{30U, 2U, 2U, 1U},
  };
  const std::array<PointId, 4U> points{0U, 1U, 2U, 4U};
  commit_births(context, insertions, births, points);
  commit_rooting_transition(context);
  return context;
}

[[nodiscard]] ExactDirectSparseStableFacetDescentClosureResult build_closure(
    const Context& context,
    const CanonicalPointCloud& points,
    const MortonLbvhIndex& index) {
  const std::array seeds{
      ExactDirectSparseStableFacetDescentClosureSeed{0U, key({1U, 3U})},
      ExactDirectSparseStableFacetDescentClosureSeed{1U, key({2U, 4U})},
      ExactDirectSparseStableFacetDescentClosureSeed{2U, key({0U, 4U})},
  };
  auto closure = build_exact_direct_sparse_stable_facet_descent_closure(
      index,
      points,
      seeds,
      level(33, 2),
      context.forest,
      closure_budget());
  check(
      closure.certified_complete_relative_positive_closure() &&
          closure.seed_projections().size() == 3U &&
          closure.seed_projections()[0U].terminal_node_index ==
              closure.seed_projections()[2U].terminal_node_index,
      "the exact closure yields rooted, latent and one shared terminal");
  return closure;
}

[[nodiscard]] const std::array<
    ExactDirectSparseCarrierOriginSeedBinding,
    3U>&
bindings() {
  static const std::array<ExactDirectSparseCarrierOriginSeedBinding, 3U>
      value{{{0U, 40U}, {1U, 41U}, {2U, 42U}}};
  return value;
}

[[nodiscard]] ExactDirectSparseCarrierOriginCapabilityBudget exact_budget(
    const Context& context) {
  const ExactDirectSparseRootLedgerActiveRootProbeBudget generous{64U};
  const auto rooted = context.ledger.probe_active_root(2U, generous);
  const auto latent = context.ledger.probe_active_root(30U, generous);
  const std::size_t probe_cap = std::max(
      rooted.root_index_slot_visit_count,
      latent.root_index_slot_visit_count);
  const ExactDirectSparseRootLedgerActiveRootProbeBudget exact_probe{
      probe_cap};
  const auto rooted_exact = context.ledger.probe_active_root(2U, exact_probe);
  const auto latent_exact = context.ledger.probe_active_root(30U, exact_probe);
  const std::array<ExactDirectSparseStableFacetHandle, 2U> roots{2U, 30U};
  const auto historical = context.ledger.materialize_active_coverages(
      roots, {64U, 64U, 64U, 64U, 64U, 128U});
  check(
      rooted_exact.certified_observed_rooted() &&
          latent_exact.certified_observed_latent() &&
          historical.certified_requested_only_materialization(),
      "exact capability budgets are measured from authentic bounded proofs");
  const ExactDirectSparseRootCoverageMaterializationBudget coverage{
      historical.requested_root_count,
      historical.reachable_coverage_node_count,
      historical.parent_reference_scan_count,
      historical.point_delta_scan_count,
      historical.point_ids.size(),
      historical.scratch_entry_count};
  return {
      3U,
      2U,
      historical.point_ids.size(),
      exact_probe,
      {2U,
       2U * probe_cap,
       rooted_exact.root_index_slot_visit_count +
           latent_exact.root_index_slot_visit_count,
       rooted_exact.exact_key_comparison_count +
           latent_exact.exact_key_comparison_count,
       2U,
       coverage},
  };
}

void test_positive_composition_and_external_differential() {
  auto context = make_context();
  const auto points = cloud();
  const auto index = MortonLbvhIndex::build(points);
  const auto closure = build_closure(context, points, index);
  const auto budget = exact_budget(context);
  const auto forest_stamp = context.forest.current_stamp();
  const auto ledger_stamp = context.ledger.current_stamp();
  auto result = derive_exact_direct_sparse_carrier_origin_capability(
      closure, bindings(), context.forest, context.ledger, budget);

  const std::array expected_origins{
      ExactDirectSparseCarrierOriginRecord{
          0U,
          40U,
          2U,
          {ExactFrozenIncidenceTokenKind::rooted_carrier, 1U},
          1U,
          3U,
          0U,
          ExactDirectSparseCarrierOriginKind::rooted},
      ExactDirectSparseCarrierOriginRecord{
          1U,
          41U,
          30U,
          {ExactFrozenIncidenceTokenKind::latent_carrier, 30U},
          std::nullopt,
          2U,
          0U,
          ExactDirectSparseCarrierOriginKind::latent},
      ExactDirectSparseCarrierOriginRecord{
          2U,
          42U,
          2U,
          {ExactFrozenIncidenceTokenKind::rooted_carrier, 1U},
          1U,
          3U,
          0U,
          ExactDirectSparseCarrierOriginKind::rooted},
  };
  const std::array expected_resolutions{
      ExactDirectNormalizedH0StableFacetResolution{
          40U, {ExactFrozenIncidenceTokenKind::rooted_carrier, 1U}, 1U},
      ExactDirectNormalizedH0StableFacetResolution{
          41U,
          {ExactFrozenIncidenceTokenKind::latent_carrier, 30U},
          std::nullopt},
      ExactDirectNormalizedH0StableFacetResolution{
          42U, {ExactFrozenIncidenceTokenKind::rooted_carrier, 1U}, 1U},
  };
  const std::array expected_attachments{
      ExactDirectNormalizedH0SparseForestCarrierAttachment{
          {ExactFrozenIncidenceTokenKind::rooted_carrier, 1U}, 2U, 1U},
      ExactDirectNormalizedH0SparseForestCarrierAttachment{
          {ExactFrozenIncidenceTokenKind::latent_carrier, 30U},
          30U,
          std::nullopt},
  };
  const std::array expected_prior_coverages{
      ExactDirectFrozenUnifiedPriorRootCoverage{1U, 0U, 3U}};
  const std::array<PointId, 3U> expected_prior_points{0U, 1U, 4U};
  const std::array expected_latent_coverages{
      ExactDirectFrozenUnifiedLatentCarrierCoverage{30U, 0U, 2U}};
  const std::array<PointId, 2U> expected_latent_points{2U, 4U};
  const auto equals = [](const auto observed, const auto& expected) {
    return observed.size() == expected.size() &&
           std::equal(
               observed.begin(), observed.end(), expected.begin(), expected.end());
  };

  check(
      result.certified_exact_carrier_origin_capability() &&
          result.certified_for(forest_stamp, ledger_stamp) &&
          result.no_external_root_or_coverage_span_consumed() &&
          !result.source_exactness_claimed() &&
          !result.public_status_claimed() &&
          result.counters() ==
              ExactDirectSparseCarrierOriginCapabilityCounters{
                  3U, 2U, 1U, 2U, 1U, 1U, 5U} &&
          equals(result.origins(), expected_origins) &&
          equals(
              result.carrier_stable_facet_resolutions(),
              expected_resolutions) &&
          equals(result.carrier_attachments(), expected_attachments) &&
          equals(result.prior_root_coverages(), expected_prior_coverages) &&
          equals(
              result.prior_root_coverage_point_references(),
              expected_prior_points) &&
          equals(
              result.latent_carrier_coverages(),
              expected_latent_coverages) &&
          equals(
              result.latent_carrier_coverage_point_references(),
              expected_latent_points),
      "the private carrier-only capability exactly replaces the positive manual rooted/latent spans");

  const std::array<ExactDirectSparseStableFacetHandle, 2U> roots{2U, 30U};
  const auto historical = context.ledger.materialize_active_coverages(
      roots, budget.coverage.coverage);
  std::vector<PointId> derived_points{
      result.prior_root_coverage_point_references().begin(),
      result.prior_root_coverage_point_references().end()};
  derived_points.insert(
      derived_points.end(),
      result.latent_carrier_coverage_point_references().begin(),
      result.latent_carrier_coverage_point_references().end());
  std::vector<PointId> historical_points{
      historical.point_ids.begin(), historical.point_ids.end()};
  std::sort(derived_points.begin(), derived_points.end());
  std::sort(historical_points.begin(), historical_points.end());
  check(
      historical.certified_requested_only_materialization() &&
          derived_points == historical_points,
      "the derived capability is differentially equal to the historical external requested-root path");

  ExactDirectSparseCarrierOriginCapabilityResult default_result;
  check(
      !default_result.certified_outcome(),
      "a default object cannot forge the carrier capability");
  auto moved = std::move(result);
  check(
      moved.certified_exact_carrier_origin_capability() &&
          !result.certified_outcome(),
      "move-only ownership transfers and revokes the source object");
}

void test_canonical_non_dense_bound_seed_subsequence() {
  auto context = make_context();
  const auto points = cloud();
  const auto index = MortonLbvhIndex::build(points);
  const auto closure = build_closure(context, points, index);
  const auto budget = exact_budget(context);
  const std::array non_dense{
      ExactDirectSparseCarrierOriginSeedBinding{0U, 40U},
      ExactDirectSparseCarrierOriginSeedBinding{2U, 42U},
  };
  const auto subset = derive_exact_direct_sparse_carrier_origin_capability(
      closure, non_dense, context.forest, context.ledger, budget);
  const std::array expected_origins{
      ExactDirectSparseCarrierOriginRecord{
          0U,
          40U,
          2U,
          {ExactFrozenIncidenceTokenKind::rooted_carrier, 1U},
          1U,
          3U,
          0U,
          ExactDirectSparseCarrierOriginKind::rooted},
      ExactDirectSparseCarrierOriginRecord{
          2U,
          42U,
          2U,
          {ExactFrozenIncidenceTokenKind::rooted_carrier, 1U},
          1U,
          3U,
          0U,
          ExactDirectSparseCarrierOriginKind::rooted},
  };
  const std::array expected_resolutions{
      ExactDirectNormalizedH0StableFacetResolution{
          40U, {ExactFrozenIncidenceTokenKind::rooted_carrier, 1U}, 1U},
      ExactDirectNormalizedH0StableFacetResolution{
          42U, {ExactFrozenIncidenceTokenKind::rooted_carrier, 1U}, 1U},
  };
  check(
      subset.certified_exact_carrier_origin_capability() &&
          subset.counters() ==
              ExactDirectSparseCarrierOriginCapabilityCounters{
                  2U, 1U, 1U, 1U, 1U, 0U, 3U} &&
          std::equal(
              subset.origins().begin(),
              subset.origins().end(),
              expected_origins.begin(),
              expected_origins.end()) &&
          std::equal(
              subset.carrier_stable_facet_resolutions().begin(),
              subset.carrier_stable_facet_resolutions().end(),
              expected_resolutions.begin(),
              expected_resolutions.end()) &&
          subset.carrier_attachments().size() == 1U &&
          subset.latent_carrier_coverages().empty(),
      "a canonical non-dense seed subsequence derives exactly its two bound carriers and makes no claim for the omitted projection");

  const std::array duplicate{
      ExactDirectSparseCarrierOriginSeedBinding{0U, 40U},
      ExactDirectSparseCarrierOriginSeedBinding{0U, 42U},
  };
  const std::array reversed{
      ExactDirectSparseCarrierOriginSeedBinding{2U, 40U},
      ExactDirectSparseCarrierOriginSeedBinding{0U, 42U},
  };
  const std::array out_of_range{
      ExactDirectSparseCarrierOriginSeedBinding{3U, 40U},
  };
  const std::array stable_out_of_namespace{
      ExactDirectSparseCarrierOriginSeedBinding{0U, 128U},
  };
  for (const auto supplied : {
           std::span<const ExactDirectSparseCarrierOriginSeedBinding>{
               duplicate},
           std::span<const ExactDirectSparseCarrierOriginSeedBinding>{
               reversed},
           std::span<const ExactDirectSparseCarrierOriginSeedBinding>{
               out_of_range},
           std::span<const ExactDirectSparseCarrierOriginSeedBinding>{
               stable_out_of_namespace}}) {
    const auto rejected =
        derive_exact_direct_sparse_carrier_origin_capability(
            closure,
            supplied,
            context.forest,
            context.ledger,
            budget);
    check(
        rejected.certified_rejection() &&
            rejected.decision() ==
                ExactDirectSparseCarrierOriginCapabilityDecision::
                    no_input_shape_rejected,
        "duplicate, reversed, seed-out-of-range and stable-token-out-of-namespace bindings fail closed");
  }
}

void test_caps_stamps_and_defensive_unobserved() {
  auto context = make_context();
  const auto points = cloud();
  const auto index = MortonLbvhIndex::build(points);
  const auto closure = build_closure(context, points, index);
  const auto exact = exact_budget(context);
  const auto expect_budget = [&](const auto& budget, const std::string& text) {
    const auto result = derive_exact_direct_sparse_carrier_origin_capability(
        closure, bindings(), context.forest, context.ledger, budget);
    check(
        result.certified_budget_exhaustion() && result.origins().empty(),
        text);
  };
  auto reduced = exact;
  --reduced.maximum_carrier_count;
  expect_budget(reduced, "carrier cap minus one fails closed");
  reduced = exact;
  --reduced.maximum_distinct_terminal_root_count;
  expect_budget(reduced, "deduplicated terminal-root cap minus one fails closed");
  reduced = exact;
  --reduced.active_root_probe.maximum_root_index_slot_visit_count;
  expect_budget(reduced, "per-root proof cap minus one fails closed");
  reduced = exact;
  --reduced.coverage.coverage.maximum_output_point_count;
  expect_budget(reduced, "proof-bound DAG output cap minus one fails closed");
  reduced = exact;
  --reduced.maximum_output_point_reference_count;
  expect_budget(reduced, "carrier output cap minus one fails closed");

  const auto sibling_miss = context.empty_sibling_ledger.probe_active_root(
      2U, ExactDirectSparseRootLedgerActiveRootProbeBudget{0U});
  const auto misaligned =
      derive_exact_direct_sparse_carrier_origin_capability(
          closure,
          bindings(),
          context.forest,
          context.empty_sibling_ledger,
          exact);
  check(
      sibling_miss.certified_unobserved() &&
          misaligned.certified_rejection() &&
          misaligned.decision() ==
              ExactDirectSparseCarrierOriginCapabilityDecision::
                  no_forest_ledger_stamp_alignment_rejected,
      "an unobserved terminal is defensive under the healthy alignment invariant; a stale empty ledger is rejected before it can authorize a miss");

  auto foreign = make_context();
  const auto foreign_points = cloud();
  const auto foreign_index = MortonLbvhIndex::build(foreign_points);
  const auto foreign_closure =
      build_closure(foreign, foreign_points, foreign_index);
  const auto foreign_result =
      derive_exact_direct_sparse_carrier_origin_capability(
          foreign_closure,
          bindings(),
          context.forest,
          context.ledger,
          exact);
  check(
      foreign_result.certified_rejection() &&
          foreign_result.decision() ==
              ExactDirectSparseCarrierOriginCapabilityDecision::
                  no_closure_rejected,
      "a foreign closure stamp is rejected");

  const auto old_forest_stamp = context.forest.current_stamp();
  const auto old_ledger_stamp = context.ledger.current_stamp();
  auto historical = derive_exact_direct_sparse_carrier_origin_capability(
      closure, bindings(), context.forest, context.ledger, exact);
  const std::array later_insertions{
      ExactDirectSparseStableFacetInsertion{50U, key({3U, 4U})}};
  const std::array later_births{
      ExactDirectSparseRootLedgerStandaloneBirth{50U, 0U, 1U, 1U}};
  const std::array<PointId, 1U> later_points{3U};
  commit_births(context, later_insertions, later_births, later_points);
  const auto stale = derive_exact_direct_sparse_carrier_origin_capability(
      closure, bindings(), context.forest, context.ledger, exact);
  check(
      stale.certified_rejection() &&
          stale.decision() ==
              ExactDirectSparseCarrierOriginCapabilityDecision::
                  no_closure_rejected &&
          historical.certified_for(old_forest_stamp, old_ledger_stamp) &&
          !historical.certified_for(
              context.forest.current_stamp(),
              context.ledger.current_stamp()),
      "a stale closure cannot authorize the successor stamps");
}

}  // namespace

int main() {
  test_positive_composition_and_external_differential();
  test_canonical_non_dense_bound_seed_subsequence();
  test_caps_stamps_and_defensive_unobserved();
  if (failures != 0) {
    std::cerr << failures << " carrier-origin capability check(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse carrier-origin capability checks passed\n";
  return 0;
}
