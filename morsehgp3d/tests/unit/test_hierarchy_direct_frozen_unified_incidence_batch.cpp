#include "morsehgp3d/hierarchy/direct_frozen_unified_incidence_batch.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(double x, double y, double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
}

[[nodiscard]] ExactLevel level(
    std::int64_t numerator,
    std::int64_t denominator = 1) {
  return ExactLevel{BigInt{numerator}, BigInt{denominator}};
}

[[nodiscard]] ExactPairSupportStreamBudget unlimited_pair_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactHigherSupportStreamBudget unlimited_higher_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

[[nodiscard]] ExactDirectSaddleArmSeedBudget unlimited_arm_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectClosedSaddleIncidenceBudget
unlimited_incidence_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum, maximum, maximum};
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceBudget
generous_successive_budget() {
  return {
      1024U,
      65536U,
      65536U,
      1048576U,
      65536U,
      1048576U,
      16777216U,
      65536U,
      65536U,
  };
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalBudget
generous_star_budget() {
  return {
      1024U,
      11264U,
      11264U,
      112640U,
      131072U,
      131072U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      generous_successive_budget(),
  };
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanBudget generous_plan_budget() {
  return {
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      1048576U,
      8388608U,
      8388608U,
      1048576U,
      8388608U,
      1048576U,
      1048576U,
      1048576U,
      16777216U,
  };
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchBudget
unlimited_batch_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
  };
}

struct DirectSources {
  ExactDirectSupportTerminalFacade facade;
  ExactDirectMorseEventJournalResult event_journal;
  ExactDirectSaddleArmSeedBudget arm_budget;
  ExactDirectSaddleArmSeedJournalResult arm_journal;
  ExactDirectClosedSaddleIncidenceBudget incidence_budget;
  ExactDirectClosedSaddleIncidenceJournalResult incidence_journal;
};

[[nodiscard]] DirectSources direct_sources(
    const CanonicalPointCloud& cloud,
    std::size_t requested_maximum_order) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair = build_exact_pair_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, requested_maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index,
      cloud,
      requested_maximum_order,
      terminal_budget,
      pair,
      higher);
  auto event_journal =
      build_exact_direct_morse_event_journal(cloud, facade);
  const auto arm_budget = unlimited_arm_budget();
  auto arm_journal = build_exact_direct_saddle_arm_seed_journal(
      cloud, facade, event_journal, arm_budget);
  const auto incidence_budget = unlimited_incidence_budget();
  auto incidence_journal =
      build_exact_direct_closed_saddle_incidence_journal(
          cloud,
          facade,
          event_journal,
          arm_budget,
          arm_journal,
          incidence_budget);
  return {
      std::move(facade),
      std::move(event_journal),
      arm_budget,
      std::move(arm_journal),
      incidence_budget,
      std::move(incidence_journal),
  };
}

[[nodiscard]] CanonicalPointCloud e5_cloud() {
  return canonical_cloud(std::array{
      point(-2.0, -1.0),
      point(-2.0, 1.0),
      point(0.0, 0.0),
      point(3.0, 2.0),
      point(4.0, -1.0)});
}

[[nodiscard]] CanonicalPointCloud mixed_direct_residual_cloud() {
  return canonical_cloud(std::array{
      point(-2.0, -1.0, 0.0),
      point(-2.0, 1.0, 0.0),
      point(0.0, 0.0, 0.0),
      point(3.0, 2.0, 0.0),
      point(4.0, -1.0, 0.0),
      point(20.0, 0.0, 0.0),
      point(23.0, 3.0, 4.0),
      point(40.0, 0.0, 0.0),
      point(41.5, 1.5, 2.0),
      point(43.0, 3.0, 4.0)});
}

struct E5Context {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources source;
  ExactDirectSparseSuccessiveIncidenceStarJournalBudget star_budget;
  ExactDirectSparseSuccessiveIncidenceStarJournalResult star;
  ExactDirectSparseUnifiedLevelPlanBudget plan_budget;
  ExactDirectSparseUnifiedLevelPlanResult plan;
};

[[nodiscard]] E5Context context_for(
    CanonicalPointCloud cloud,
    LbvhTraversalOrder traversal_order) {
  auto index = MortonLbvhIndex::build(cloud);
  auto source = direct_sources(cloud, 2U);
  auto star_budget = generous_star_budget();
  auto star =
      build_exact_direct_sparse_successive_incidence_star_journal(
          index,
          cloud,
          source.facade,
          source.event_journal,
          source.arm_budget,
          source.arm_journal,
          source.incidence_budget,
          source.incidence_journal,
          star_budget,
          traversal_order);
  auto plan_budget = generous_plan_budget();
  auto plan = build_exact_direct_sparse_unified_level_plan(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      star_budget,
      traversal_order,
      star,
      plan_budget);
  return {
      std::move(cloud),
      std::move(index),
      std::move(source),
      star_budget,
      std::move(star),
      plan_budget,
      std::move(plan),
  };
}

[[nodiscard]] E5Context e5_context(LbvhTraversalOrder traversal_order) {
  return context_for(e5_cloud(), traversal_order);
}

[[nodiscard]] std::vector<PointId> facet_ids(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    std::size_t facet_token_index) {
  const auto& key = plan.facet_tokens[facet_token_index].facet_key;
  return {
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
  };
}

[[nodiscard]] bool ids_equal(
    const std::vector<PointId>& ids,
    std::initializer_list<PointId> expected) {
  return ids == std::vector<PointId>(expected);
}

struct BatchAuthority {
  std::vector<ExactDirectFrozenUnifiedFacetResolution> resolutions;
  std::vector<ExactDirectFrozenUnifiedPriorRootCoverage> coverages;
  std::vector<PointId> coverage_points;
  std::vector<ExactDirectFrozenUnifiedLatentCarrierCoverage>
      latent_coverages;
  std::vector<PointId> latent_coverage_points;
};

struct ResolutionChoice {
  ExactFrozenIncidenceTokenKind kind{
      ExactFrozenIncidenceTokenKind::latent_carrier};
  ExactFrozenIncidenceTokenId token_id{};
  std::optional<ExactFrozenIncidencePriorRootId> prior_root_id;
};

[[nodiscard]] ResolutionChoice choice_for(
    const ExactLevel& squared_level,
    std::size_t facet_token_index,
    const std::vector<PointId>& ids) {
  if (squared_level == level(25, 16) ||
      squared_level == level(1105, 242)) {
    return {
        ExactFrozenIncidenceTokenKind::latent_carrier,
        10000U + facet_token_index,
        std::nullopt};
  }
  if (squared_level == level(13, 2)) {
    if (ids_equal(ids, {1U, 3U})) {
      return {
          ExactFrozenIncidenceTokenKind::equal_facet,
          20000U + facet_token_index,
          std::nullopt};
    }
    if (ids_equal(ids, {1U, 2U})) {
      return {
          ExactFrozenIncidenceTokenKind::rooted_carrier,
          10U,
          1000U};
    }
    if (ids_equal(ids, {2U, 3U})) {
      return {
          ExactFrozenIncidenceTokenKind::rooted_carrier,
          20U,
          2000U};
    }
  }
  const bool at_17_2 = squared_level == level(17, 2);
  const bool at_9 = squared_level == level(9);
  const bool at_85_9 = squared_level == level(85, 9);
  const bool at_10 = squared_level == level(10);
  if ((at_17_2 && ids_equal(ids, {0U, 3U})) ||
      (at_9 && ids_equal(ids, {0U, 4U})) ||
      (at_10 && ids_equal(ids, {1U, 4U}))) {
    return {
        ExactFrozenIncidenceTokenKind::equal_facet,
        20000U + facet_token_index,
        std::nullopt};
  }
  if (at_17_2 || at_9 || at_85_9 || at_10) {
    return {
        ExactFrozenIncidenceTokenKind::rooted_carrier,
        30U,
        3000U};
  }
  return {
      ExactFrozenIncidenceTokenKind::latent_carrier,
      10000U + facet_token_index,
      std::nullopt};
}

[[nodiscard]] BatchAuthority authority_for(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch) {
  BatchAuthority authority;
  std::vector<std::size_t> touched;
  for (std::size_t local = 0U;
       local < batch.coface_facet_reference_count;
       ++local) {
    touched.push_back(
        plan.coface_facet_references
            [batch.coface_facet_reference_offset + local]
                .facet_token_index);
  }
  std::sort(touched.begin(), touched.end());
  touched.erase(std::unique(touched.begin(), touched.end()), touched.end());
  std::vector<ExactFrozenIncidencePriorRootId> roots;
  std::vector<std::pair<ExactFrozenIncidenceTokenId, std::vector<PointId>>>
      latent_coverage_by_token;
  for (const std::size_t facet_token_index : touched) {
    const auto choice = choice_for(
        batch.squared_level,
        facet_token_index,
        facet_ids(plan, facet_token_index));
    authority.resolutions.push_back(
        {facet_token_index,
         {choice.kind, choice.token_id},
         choice.prior_root_id});
    if (choice.prior_root_id.has_value()) {
      roots.push_back(*choice.prior_root_id);
    } else if (choice.kind ==
               ExactFrozenIncidenceTokenKind::latent_carrier) {
      const auto found = std::find_if(
          latent_coverage_by_token.begin(),
          latent_coverage_by_token.end(),
          [&](const auto& entry) { return entry.first == choice.token_id; });
      if (found == latent_coverage_by_token.end()) {
        latent_coverage_by_token.push_back(
            {choice.token_id, facet_ids(plan, facet_token_index)});
      } else {
        const auto ids = facet_ids(plan, facet_token_index);
        found->second.insert(found->second.end(), ids.begin(), ids.end());
      }
    }
  }
  std::sort(roots.begin(), roots.end());
  roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
  for (const auto root : roots) {
    const std::size_t offset = authority.coverage_points.size();
    if (root == 1000U) {
      authority.coverage_points.insert(
          authority.coverage_points.end(), {0U, 1U, 2U});
    } else if (root == 2000U) {
      authority.coverage_points.insert(
          authority.coverage_points.end(), {2U, 3U, 4U});
    } else if (root == 3000U) {
      authority.coverage_points.insert(
          authority.coverage_points.end(), {0U, 1U, 2U, 3U, 4U});
    }
    authority.coverages.push_back(
        {root, offset, authority.coverage_points.size() - offset});
  }
  std::sort(
      latent_coverage_by_token.begin(),
      latent_coverage_by_token.end(),
      [](const auto& left, const auto& right) {
        return left.first < right.first;
      });
  for (auto& [token_id, points] : latent_coverage_by_token) {
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    const std::size_t offset = authority.latent_coverage_points.size();
    authority.latent_coverage_points.insert(
        authority.latent_coverage_points.end(),
        points.begin(),
        points.end());
    authority.latent_coverages.push_back(
        {token_id,
         offset,
         authority.latent_coverage_points.size() - offset});
  }
  return authority;
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchResult run_batch(
    const E5Context& context,
    LbvhTraversalOrder traversal_order,
    std::size_t batch_index,
    const BatchAuthority& authority,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget) {
  return build_exact_direct_frozen_unified_incidence_batch(
      context.index,
      context.cloud,
      context.source.facade,
      context.source.event_journal,
      context.source.arm_budget,
      context.source.arm_journal,
      context.source.incidence_budget,
      context.source.incidence_journal,
      context.star_budget,
      traversal_order,
      context.star,
      context.plan_budget,
      context.plan,
      batch_index,
      authority.resolutions,
      authority.coverages,
      authority.coverage_points,
      authority.latent_coverages,
      authority.latent_coverage_points,
      budget);
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchVerification verify_batch(
    const E5Context& context,
    LbvhTraversalOrder traversal_order,
    std::size_t batch_index,
    const BatchAuthority& authority,
    const ExactDirectFrozenUnifiedIncidenceBatchBudget& budget,
    const ExactDirectFrozenUnifiedIncidenceBatchResult& observed) {
  return verify_exact_direct_frozen_unified_incidence_batch(
      context.index,
      context.cloud,
      context.source.facade,
      context.source.event_journal,
      context.source.arm_budget,
      context.source.arm_journal,
      context.source.incidence_budget,
      context.source.incidence_journal,
      context.star_budget,
      traversal_order,
      context.star,
      context.plan_budget,
      context.plan,
      batch_index,
      authority.resolutions,
      authority.coverages,
      authority.coverage_points,
      authority.latent_coverages,
      authority.latent_coverage_points,
      budget,
      observed);
}

[[nodiscard]] bool complete_verification(
    const ExactDirectFrozenUnifiedIncidenceBatchVerification& verification) {
  return verification.requested_budget_certified &&
         verification.source_plan_freshly_verified &&
         !verification.source_plan_immutable_resident_authority_certified &&
         verification.expected_result_freshly_reconstructed &&
         verification.supplied_latent_carrier_coverage_freshly_replayed &&
         verification.quotient_freshly_streaming_verified &&
         verification.action_plan_freshly_streaming_verified &&
         verification.observed_recursively_equal &&
         verification.result_facts_and_scope_certified &&
         verification.no_forbidden_global_structure_or_mutation &&
         verification.fresh_replay_certified &&
         !verification
              .immutable_authority_batch_reconstruction_certified &&
         verification.result_certified;
}

[[nodiscard]] const ExactDirectSparseUnifiedLevelPlanBatch* batch_at(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactLevel& squared_level) {
  const auto found = std::find_if(
      plan.batches.begin(), plan.batches.end(), [&](const auto& batch) {
        return batch.order == 2U && batch.squared_level == squared_level;
      });
  return found == plan.batches.end() ? nullptr : &*found;
}

[[nodiscard]] ExactDirectFrozenUnifiedIncidenceBatchBudget exact_budget_for(
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result) {
  return {
      result.required_facet_resolution_capacity,
      result.required_prior_root_coverage_capacity,
      result.required_prior_root_coverage_point_reference_capacity,
      result.required_latent_carrier_coverage_capacity,
      result.required_latent_carrier_coverage_point_reference_capacity,
      result.required_batch_direct_reference_scan_capacity,
      result.required_direct_saddle_hyperedge_capacity,
      result.required_residual_hyperedge_capacity,
      result.required_coface_facet_reference_scan_capacity,
      result.required_source_point_reference_scan_capacity,
      result.required_hyperedge_capacity,
      result.required_token_reference_capacity,
      result.required_distinct_typed_token_capacity,
      result.required_root_attachment_capacity,
      result.required_group_capacity,
      result.required_residual_incidence_record_capacity,
      result.required_equal_facet_binding_record_capacity,
      result.required_coverage_delta_record_capacity,
      result.required_coverage_delta_facet_reference_capacity,
      result.required_coverage_delta_point_reference_capacity,
      result.required_scratch_entry_capacity,
      result.required_logical_output_entry_capacity,
  };
}

[[nodiscard]] std::size_t expected_source_point_reference_scans(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const BatchAuthority& authority,
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result) {
  std::size_t expected = 2U * authority.coverage_points.size() +
      authority.latent_coverage_points.size();
  for (const auto& resolution : authority.resolutions) {
    if (resolution.token.kind ==
        ExactFrozenIncidenceTokenKind::equal_facet) {
      continue;
    }
    std::size_t coverage_point_count = 0U;
    if (resolution.token.kind ==
        ExactFrozenIncidenceTokenKind::rooted_carrier) {
      const auto coverage = std::find_if(
          authority.coverages.begin(),
          authority.coverages.end(),
          [&](const auto& record) {
            return resolution.prior_root_id.has_value() &&
                record.prior_root_id == *resolution.prior_root_id;
          });
      if (coverage != authority.coverages.end()) {
        coverage_point_count = coverage->point_reference_count;
      }
    } else {
      const auto coverage = std::find_if(
          authority.latent_coverages.begin(),
          authority.latent_coverages.end(),
          [&](const auto& record) {
            return record.latent_carrier_token_id ==
                resolution.token.token_id;
          });
      if (coverage != authority.latent_coverages.end()) {
        coverage_point_count = coverage->point_reference_count;
      }
    }
    if (coverage_point_count != 0U) {
      const std::size_t conservative_probes =
          static_cast<std::size_t>(std::bit_width(coverage_point_count)) +
          1U;
      expected += plan.facet_tokens[resolution.facet_token_index]
          .facet_key.point_count * conservative_probes;
    }
  }
  for (const auto& hyperedge : result.quotient_hyperedges) {
    std::vector<ExactFrozenIncidenceTokenId> latent_ids;
    for (std::size_t local = 0U;
         local < hyperedge.token_reference_count;
         ++local) {
      const std::size_t reference_index =
          hyperedge.token_reference_offset + local;
      expected += plan.facet_tokens
          [result.incidence_facet_token_indices[reference_index]]
              .facet_key.point_count;
      const auto& token = result.quotient_token_references[reference_index];
      if (token.kind == ExactFrozenIncidenceTokenKind::latent_carrier) {
        latent_ids.push_back(token.token_id);
      }
    }
    std::sort(latent_ids.begin(), latent_ids.end());
    latent_ids.erase(
        std::unique(latent_ids.begin(), latent_ids.end()), latent_ids.end());
    for (const auto token_id : latent_ids) {
      const auto coverage = std::find_if(
          authority.latent_coverages.begin(),
          authority.latent_coverages.end(),
          [&](const auto& record) {
            return record.latent_carrier_token_id == token_id;
          });
      if (coverage != authority.latent_coverages.end()) {
        expected += coverage->point_reference_count;
      }
    }
  }
  return expected;
}

[[nodiscard]] BatchAuthority one_full_latent_authority(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch,
    ExactFrozenIncidenceTokenId token_id) {
  BatchAuthority authority;
  std::vector<std::size_t> touched;
  for (std::size_t local = 0U;
       local < batch.coface_facet_reference_count;
       ++local) {
    touched.push_back(
        plan.coface_facet_references
            [batch.coface_facet_reference_offset + local]
                .facet_token_index);
  }
  std::sort(touched.begin(), touched.end());
  touched.erase(std::unique(touched.begin(), touched.end()), touched.end());
  for (const std::size_t facet_token_index : touched) {
    authority.resolutions.push_back(
        {facet_token_index,
         {ExactFrozenIncidenceTokenKind::latent_carrier, token_id},
         std::nullopt});
  }
  for (std::size_t point_id = 0U; point_id < plan.point_count; ++point_id) {
    authority.latent_coverage_points.push_back(
        static_cast<PointId>(point_id));
  }
  authority.latent_coverages.push_back(
      {token_id, 0U, authority.latent_coverage_points.size()});
  return authority;
}

void insert_latent_coverage_point(
    BatchAuthority& authority,
    std::size_t coverage_index,
    PointId point_id) {
  auto& coverage = authority.latent_coverages[coverage_index];
  const auto begin = authority.latent_coverage_points.begin() +
      static_cast<std::ptrdiff_t>(coverage.point_reference_offset);
  const auto end = begin +
      static_cast<std::ptrdiff_t>(coverage.point_reference_count);
  const auto position = std::lower_bound(begin, end, point_id);
  authority.latent_coverage_points.insert(position, point_id);
  ++coverage.point_reference_count;
  for (std::size_t index = coverage_index + 1U;
       index < authority.latent_coverages.size();
       ++index) {
    ++authority.latent_coverages[index].point_reference_offset;
  }
}

[[nodiscard]] bool group_delta_contains_point(
    const ExactDirectFrozenUnifiedIncidenceBatchResult& result,
    std::size_t group_index,
    PointId point_id) {
  if (group_index >= result.coverage_deltas.size()) {
    return false;
  }
  const auto& delta = result.coverage_deltas[group_index];
  const auto begin = result.coverage_delta_points.begin() +
      static_cast<std::ptrdiff_t>(delta.point_reference_offset);
  const auto end = begin +
      static_cast<std::ptrdiff_t>(delta.point_reference_count);
  return std::any_of(begin, end, [&](const auto& reference) {
    return reference.point_id == point_id;
  });
}

void test_e5_all_batches_and_exact_records(const E5Context& context) {
  std::size_t deferred_births = 0U;
  std::size_t direct_hyperedges = 0U;
  std::size_t residual_hyperedges = 0U;
  std::size_t token_bindings = 0U;
  std::size_t residual_records = 0U;
  std::size_t residual_owned_facets = 0U;
  std::size_t residual_owned_points = 0U;
  std::size_t equal_bindings = 0U;
  std::size_t group_count = 0U;
  std::size_t q0 = 0U;
  std::size_t q1 = 0U;
  std::size_t q_multiple = 0U;
  std::size_t delta_facets = 0U;
  std::size_t delta_points = 0U;
  std::size_t redundant_deltas = 0U;
  std::vector<std::vector<PointId>> equal_facets;
  bool every_batch_verified = true;
  bool every_owner_is_canonical = true;
  bool every_delta_slice_partitions_its_flat_arena = true;

  for (const auto& batch : context.plan.batches) {
    const BatchAuthority authority = authority_for(context.plan, batch);
    const auto result = run_batch(
        context,
        LbvhTraversalOrder::near_first,
        batch.batch_index,
        authority,
        unlimited_batch_budget());
    const auto verification = verify_batch(
        context,
        LbvhTraversalOrder::near_first,
        batch.batch_index,
        authority,
        unlimited_batch_budget(),
        result);
    every_batch_verified =
        every_batch_verified &&
        result.certified_frozen_unified_incidence_batch() &&
        complete_verification(verification);
    deferred_births += result.counters.deferred_direct_birth_count;
    direct_hyperedges += result.counters.direct_saddle_hyperedge_count;
    residual_hyperedges += result.counters.residual_hyperedge_count;
    token_bindings += result.counters.token_reference_count;
    residual_records += result.residual_incidence_records.size();
    for (const auto& record : result.residual_incidence_records) {
      residual_owned_facets +=
          record.canonically_owned_facet_delta_count;
      residual_owned_points +=
          record.canonically_owned_point_delta_count;
    }
    equal_bindings += result.equal_facet_binding_records.size();
    group_count += result.coverage_deltas.size();
    q0 += result.counters.q_r_zero_group_count;
    q1 += result.counters.q_r_one_group_count;
    q_multiple += result.counters.q_r_multiple_group_count;
    delta_facets += result.coverage_delta_facets.size();
    delta_points += result.coverage_delta_points.size();
    redundant_deltas +=
        result.counters.fully_redundant_coverage_delta_count;
    for (const auto& binding : result.equal_facet_binding_records) {
      equal_facets.push_back(
          facet_ids(context.plan, binding.facet_token_index));
    }
    std::size_t sliced_facets = 0U;
    std::size_t sliced_points = 0U;
    for (const auto& delta : result.coverage_deltas) {
      sliced_facets += delta.facet_reference_count;
      sliced_points += delta.point_reference_count;
    }
    every_delta_slice_partitions_its_flat_arena =
        every_delta_slice_partitions_its_flat_arena &&
        sliced_facets == result.coverage_delta_facets.size() &&
        sliced_points == result.coverage_delta_points.size();
    for (const auto& reference : result.coverage_delta_facets) {
      every_owner_is_canonical =
          every_owner_is_canonical &&
          reference.first_source_hyperedge_index <
              result.quotient_hyperedges.size() &&
          result.quotient.hyperedge_bindings
                  [reference.first_source_hyperedge_index]
                      .group_index == reference.owner_group_index;
    }
    for (const auto& reference : result.coverage_delta_points) {
      every_owner_is_canonical =
          every_owner_is_canonical &&
          reference.first_source_hyperedge_index <
              result.quotient_hyperedges.size() &&
          result.quotient.hyperedge_bindings
                  [reference.first_source_hyperedge_index]
                      .group_index == reference.owner_group_index;
    }
  }
  std::sort(equal_facets.begin(), equal_facets.end());
  check(
      every_batch_verified,
      "all twelve E5 batches close through the full source, quotient and action replays");
  check(
      deferred_births == 6U && direct_hyperedges == 4U &&
          residual_hyperedges == 6U && token_bindings == 30U &&
          residual_records == 6U && residual_owned_facets == 2U &&
          residual_owned_points == 0U,
      "E5 defers six direct births and emits exactly 4+6 hyperedges with thirty factorized bindings");
  check(
      residual_owned_facets == 2U && residual_owned_points == 0U,
      "the overlapping residual shells assign shared facets 03 and 14 to one first hyperedge each without double counting");
  check(
      group_count == 7U && q0 == 2U && q1 == 4U &&
          q_multiple == 1U && equal_bindings == 4U,
      "the seven non-deferred E5 groups have qR partition 2/4/1 and four equal facets");
  check(
      equal_facets ==
          std::vector<std::vector<PointId>>(
              {{0U, 3U}, {0U, 4U}, {1U, 3U}, {1U, 4U}}),
      "the equal-facet bindings are exactly 03, 04, 13 and 14");
  check(
      delta_facets == 10U && delta_points == 6U &&
          redundant_deltas == 1U && every_owner_is_canonical &&
          every_delta_slice_partitions_its_flat_arena,
      "E5 preserves ten facet deltas, six point references and one fully redundant delta with canonical owners");
}

void test_latent_coverage_and_point_scan_regressions(
    const E5Context& context) {
  bool full_latent_coverage_exercised = false;
  bool shared_facet_scan_exercised = false;
  bool missing_latent_authority_rejected = false;
  for (const auto& batch : context.plan.batches) {
    const BatchAuthority authority = authority_for(context.plan, batch);
    const auto baseline = run_batch(
        context,
        LbvhTraversalOrder::near_first,
        batch.batch_index,
        authority,
        unlimited_batch_budget());
    if (!baseline.certified_frozen_unified_incidence_batch()) {
      continue;
    }
    check(
        baseline.required_source_point_reference_scan_capacity ==
                expected_source_point_reference_scans(
                    context.plan, authority, baseline) &&
            baseline.counters.source_point_reference_scan_count ==
                baseline.required_source_point_reference_scan_capacity,
        "the PointId scan requirement equals the independently replayed per-occurrence source reads");

    std::size_t unique_key_point_count = 0U;
    for (const auto& resolution : authority.resolutions) {
      unique_key_point_count +=
          context.plan.facet_tokens[resolution.facet_token_index]
              .facet_key.point_count;
    }
    std::size_t occurrence_key_point_count = 0U;
    for (std::size_t local = 0U;
         local < batch.coface_facet_reference_count;
         ++local) {
      const auto& reference = context.plan.coface_facet_references
          [batch.coface_facet_reference_offset + local];
      occurrence_key_point_count +=
          context.plan.facet_tokens[reference.facet_token_index]
              .facet_key.point_count;
    }
    if (!shared_facet_scan_exercised &&
        occurrence_key_point_count > unique_key_point_count) {
      auto short_scan = exact_budget_for(baseline);
      --short_scan.maximum_source_point_reference_scan_count;
      const auto rejected = run_batch(
          context,
          LbvhTraversalOrder::near_first,
          batch.batch_index,
          authority,
          short_scan);
      shared_facet_scan_exercised =
          baseline.required_source_point_reference_scan_capacity >
              unique_key_point_count &&
          rejected.atomic_empty_failure() &&
          rejected.decision ==
              ExactDirectFrozenUnifiedIncidenceBatchDecision::
                  no_batch_budget_exhausted;
    }

    if (!missing_latent_authority_rejected &&
        !authority.latent_coverages.empty()) {
      BatchAuthority missing = authority;
      missing.latent_coverages.clear();
      missing.latent_coverage_points.clear();
      const auto rejected = run_batch(
          context,
          LbvhTraversalOrder::near_first,
          batch.batch_index,
          missing,
          unlimited_batch_budget());
      missing_latent_authority_rejected =
          rejected.atomic_empty_failure() &&
          rejected.decision ==
              ExactDirectFrozenUnifiedIncidenceBatchDecision::
                  no_batch_latent_carrier_coverage_authority_rejected;
    }

    if (full_latent_coverage_exercised) {
      continue;
    }
    for (std::size_t coverage_index = 0U;
         coverage_index < authority.latent_coverages.size();
         ++coverage_index) {
      const auto& coverage = authority.latent_coverages[coverage_index];
      const auto token_binding = std::find_if(
          baseline.quotient.token_bindings.begin(),
          baseline.quotient.token_bindings.end(),
          [&](const auto& binding) {
            return binding.token.kind ==
                       ExactFrozenIncidenceTokenKind::latent_carrier &&
                binding.token.token_id ==
                       coverage.latent_carrier_token_id;
          });
      if (token_binding == baseline.quotient.token_bindings.end() ||
          token_binding->group_index >= baseline.coverage_deltas.size() ||
          baseline.coverage_deltas[token_binding->group_index].q_r != 0U) {
        continue;
      }
      const auto coverage_begin = authority.latent_coverage_points.begin() +
          static_cast<std::ptrdiff_t>(coverage.point_reference_offset);
      const auto coverage_end = coverage_begin +
          static_cast<std::ptrdiff_t>(coverage.point_reference_count);
      std::optional<PointId> extra_point;
      for (std::size_t point_index = 0U;
           point_index < context.plan.point_count;
           ++point_index) {
        const PointId candidate = static_cast<PointId>(point_index);
        if (!std::binary_search(
                coverage_begin, coverage_end, candidate) &&
            !group_delta_contains_point(
                baseline, token_binding->group_index, candidate)) {
          extra_point = candidate;
          break;
        }
      }
      if (!extra_point.has_value()) {
        continue;
      }
      BatchAuthority augmented = authority;
      insert_latent_coverage_point(
          augmented, coverage_index, *extra_point);
      const auto observed = run_batch(
          context,
          LbvhTraversalOrder::near_first,
          batch.batch_index,
          augmented,
          unlimited_batch_budget());
      const auto verification = verify_batch(
          context,
          LbvhTraversalOrder::near_first,
          batch.batch_index,
          augmented,
          unlimited_batch_budget(),
          observed);
      const auto observed_binding = std::find_if(
          observed.quotient.token_bindings.begin(),
          observed.quotient.token_bindings.end(),
          [&](const auto& binding) {
            return binding.token.kind ==
                       ExactFrozenIncidenceTokenKind::latent_carrier &&
                binding.token.token_id ==
                       coverage.latent_carrier_token_id;
          });
      full_latent_coverage_exercised =
          observed.certified_frozen_unified_incidence_batch() &&
          complete_verification(verification) &&
          observed_binding != observed.quotient.token_bindings.end() &&
          group_delta_contains_point(
              observed, observed_binding->group_index, *extra_point) &&
          observed.coverage_delta_points.size() ==
              baseline.coverage_delta_points.size() + 1U &&
          observed.required_source_point_reference_scan_capacity >
              baseline.required_source_point_reference_scan_capacity;
      break;
    }
  }
  check(
      full_latent_coverage_exercised,
      "a complete latent coverage contributes a PointId absent from every source facet in its qR=0 group");
  check(
      shared_facet_scan_exercised,
      "a shared facet is counted per occurrence, exceeds the former unique-key count and fails atomically at scan cap minus one");
  check(
      missing_latent_authority_rejected,
      "an incomplete latent-carrier CSR authority has its dedicated atomic rejection");
}

void test_mixed_local_delta_and_canonical_ownership() {
  const E5Context context = context_for(
      mixed_direct_residual_cloud(), LbvhTraversalOrder::near_first);
  const auto* batch = batch_at(context.plan, level(17, 2));
  check(batch != nullptr, "the mixed fixture exposes its level-17/2 batch");
  if (batch == nullptr) {
    return;
  }
  const bool has_direct_saddle = std::any_of(
      context.plan.direct_references.begin() +
          static_cast<std::ptrdiff_t>(batch->direct_reference_offset),
      context.plan.direct_references.begin() +
          static_cast<std::ptrdiff_t>(
              batch->direct_reference_offset + batch->direct_reference_count),
      [](const auto& reference) {
        return reference.role == ExactDirectMorseH0Role::saddle;
      });
  check(
      has_direct_saddle && batch->direct_reference_count == 2U &&
          batch->residual_reference_count == 2U,
      "the level-17/2 batch naturally combines one direct saddle with two residual cofaces");
  if (!has_direct_saddle || batch->residual_reference_count == 0U) {
    return;
  }

  const BatchAuthority authority = one_full_latent_authority(
      context.plan, *batch, 777777U);
  const auto observed = run_batch(
      context,
      LbvhTraversalOrder::near_first,
      batch->batch_index,
      authority,
      unlimited_batch_budget());
  const auto record = std::find_if(
      observed.residual_incidence_records.begin(),
      observed.residual_incidence_records.end(),
      [](const auto& candidate) {
        return candidate.local_added_point_count != 0U &&
            candidate.canonically_owned_point_delta_count == 0U;
      });
  const bool mixed_record_found =
      observed.certified_frozen_unified_incidence_batch() &&
      observed.action_plan.groups.size() == 1U &&
      observed.action_plan.groups.front().mixed_provenance &&
      record != observed.residual_incidence_records.end() &&
      record->owner_group_index < observed.action_plan.groups.size() &&
      observed.action_plan.groups[record->owner_group_index].mixed_provenance;
  check(
      mixed_record_found,
      "the direct and residual incidences resolve into one certified mixed group");
  if (!mixed_record_found) {
    return;
  }

  auto forged = observed;
  forged.residual_incidence_records
      [record->residual_incidence_record_index]
          .local_zero_point_delta_certified = true;
  const auto forged_verification = verify_batch(
      context,
      LbvhTraversalOrder::near_first,
      batch->batch_index,
      authority,
      unlimited_batch_budget(),
      forged);
  const bool regression_exercised =
      !record->local_zero_point_delta_certified &&
      !record->local_fully_redundant_certified &&
      record->local_added_point_count == context.plan.point_count &&
      record->canonically_owned_point_delta_count == 0U &&
      !forged.certified_frozen_unified_incidence_batch() &&
      !forged_verification.result_certified;
  check(
      regression_exercised,
      "a mixed group separates a nonzero local residual point delta from zero canonical point ownership and rejects a forged local certificate");
}

void test_permutations_and_caps(const E5Context& near_context) {
  const auto* final_batch = batch_at(near_context.plan, level(10));
  check(final_batch != nullptr, "E5 exposes its final residual batch");
  if (final_batch == nullptr) {
    return;
  }
  const BatchAuthority authority =
      authority_for(near_context.plan, *final_batch);
  const auto baseline = run_batch(
      near_context,
      LbvhTraversalOrder::near_first,
      final_batch->batch_index,
      authority,
      unlimited_batch_budget());
  const auto exact = exact_budget_for(baseline);
  const auto exact_result = run_batch(
      near_context,
      LbvhTraversalOrder::near_first,
      final_batch->batch_index,
      authority,
      exact);
  check(
      exact_result.certified_frozen_unified_incidence_batch() &&
          exact_result.quotient_hyperedges ==
              baseline.quotient_hyperedges &&
          exact_result.quotient_token_references ==
              baseline.quotient_token_references &&
          exact_result.residual_incidence_records ==
              baseline.residual_incidence_records &&
          exact_result.coverage_deltas == baseline.coverage_deltas,
      "componentwise exact caps reproduce the final E5 frozen batch");

  bool budget_gate_precedes_point_authority_reads =
      !authority.coverage_points.empty() &&
      exact.maximum_source_point_reference_scan_count != 0U &&
      exact.maximum_scratch_entry_count != 0U;
  if (budget_gate_precedes_point_authority_reads) {
    BatchAuthority malformed_points = authority;
    malformed_points.coverage_points.front() =
        static_cast<PointId>(near_context.plan.point_count);
    auto short_scan = exact;
    --short_scan.maximum_source_point_reference_scan_count;
    auto short_scratch = exact;
    --short_scratch.maximum_scratch_entry_count;
    const auto scan_rejected = run_batch(
        near_context,
        LbvhTraversalOrder::near_first,
        final_batch->batch_index,
        malformed_points,
        short_scan);
    const auto scratch_rejected = run_batch(
        near_context,
        LbvhTraversalOrder::near_first,
        final_batch->batch_index,
        malformed_points,
        short_scratch);
    budget_gate_precedes_point_authority_reads =
        scan_rejected.atomic_empty_failure() &&
        scratch_rejected.atomic_empty_failure() &&
        scan_rejected.decision ==
            ExactDirectFrozenUnifiedIncidenceBatchDecision::
                no_batch_budget_exhausted &&
        scratch_rejected.decision ==
            ExactDirectFrozenUnifiedIncidenceBatchDecision::
                no_batch_budget_exhausted;
  }
  check(
      budget_gate_precedes_point_authority_reads,
      "scan and scratch caps fail before malformed PointId authorities are read");

  using Budget = ExactDirectFrozenUnifiedIncidenceBatchBudget;
  using BudgetMember = std::size_t Budget::*;
  const std::array<BudgetMember, 22U> cap_members{
      &Budget::maximum_facet_resolution_count,
      &Budget::maximum_prior_root_coverage_count,
      &Budget::maximum_prior_root_coverage_point_reference_count,
      &Budget::maximum_latent_carrier_coverage_count,
      &Budget::maximum_latent_carrier_coverage_point_reference_count,
      &Budget::maximum_batch_direct_reference_scan_count,
      &Budget::maximum_direct_saddle_hyperedge_count,
      &Budget::maximum_residual_hyperedge_count,
      &Budget::maximum_coface_facet_reference_scan_count,
      &Budget::maximum_source_point_reference_scan_count,
      &Budget::maximum_hyperedge_count,
      &Budget::maximum_token_reference_count,
      &Budget::maximum_distinct_typed_token_count,
      &Budget::maximum_root_attachment_count,
      &Budget::maximum_group_count,
      &Budget::maximum_residual_incidence_record_count,
      &Budget::maximum_equal_facet_binding_record_count,
      &Budget::maximum_coverage_delta_record_count,
      &Budget::maximum_coverage_delta_facet_reference_count,
      &Budget::maximum_coverage_delta_point_reference_count,
      &Budget::maximum_scratch_entry_count,
      &Budget::maximum_logical_output_entry_count,
  };
  struct CapCase {
    std::size_t batch_index{};
    BatchAuthority authority;
    Budget exact_budget{};
  };
  std::vector<CapCase> cap_cases;
  bool every_cap_baseline_certified = true;
  for (const auto& batch : near_context.plan.batches) {
    auto batch_authority = authority_for(near_context.plan, batch);
    const auto batch_result = run_batch(
        near_context,
        LbvhTraversalOrder::near_first,
        batch.batch_index,
        batch_authority,
        unlimited_batch_budget());
    every_cap_baseline_certified =
        every_cap_baseline_certified &&
        batch_result.certified_frozen_unified_incidence_batch();
    cap_cases.push_back(
        {batch.batch_index,
         std::move(batch_authority),
         exact_budget_for(batch_result)});
  }
  bool every_nonzero_cap_exercised_and_atomic =
      every_cap_baseline_certified;
  for (const BudgetMember member : cap_members) {
    bool exercised = false;
    bool atomic = false;
    for (const auto& candidate : cap_cases) {
      if (candidate.exact_budget.*member == 0U) {
        continue;
      }
      Budget short_budget = candidate.exact_budget;
      --(short_budget.*member);
      const auto rejected = run_batch(
          near_context,
          LbvhTraversalOrder::near_first,
          candidate.batch_index,
          candidate.authority,
          short_budget);
      exercised = true;
      atomic = rejected.atomic_empty_failure();
      break;
    }
    every_nonzero_cap_exercised_and_atomic =
        every_nonzero_cap_exercised_and_atomic && exercised && atomic;
  }
  check(
      every_nonzero_cap_exercised_and_atomic,
      "all twenty-two nonzero capacity dimensions fail one-short and atomically across E5 batches");

  BatchAuthority permuted = authority;
  std::reverse(permuted.resolutions.begin(), permuted.resolutions.end());
  const auto rejected_permutation = run_batch(
      near_context,
      LbvhTraversalOrder::near_first,
      final_batch->batch_index,
      permuted,
      unlimited_batch_budget());
  check(
      rejected_permutation.atomic_empty_failure() &&
          rejected_permutation.decision ==
              ExactDirectFrozenUnifiedIncidenceBatchDecision::
                  no_batch_facet_resolution_authority_rejected,
      "a noncanonical permutation of the external facet authority is rejected atomically");

  const E5Context far_context =
      e5_context(LbvhTraversalOrder::far_first);
  const auto* far_final_batch = batch_at(far_context.plan, level(10));
  bool traversal_agreement = far_final_batch != nullptr;
  if (far_final_batch != nullptr) {
    const auto far_authority =
        authority_for(far_context.plan, *far_final_batch);
    const auto far = run_batch(
        far_context,
        LbvhTraversalOrder::far_first,
        far_final_batch->batch_index,
        far_authority,
        unlimited_batch_budget());
    traversal_agreement =
        traversal_agreement && far.certified_frozen_unified_incidence_batch() &&
        far.quotient_hyperedges == baseline.quotient_hyperedges &&
        far.quotient_token_references == baseline.quotient_token_references &&
        far.incidence_facet_token_indices ==
            baseline.incidence_facet_token_indices &&
        far.residual_incidence_records ==
            baseline.residual_incidence_records &&
        far.coverage_deltas == baseline.coverage_deltas;
  }
  check(
      traversal_agreement,
      "near-first and far-first source permutations produce the same final E5 frozen batch");
}

void test_mutations_fail_fresh_replay(const E5Context& context) {
  const auto* final_batch = batch_at(context.plan, level(10));
  const auto* first_birth = batch_at(context.plan, level(25, 16));
  check(
      final_batch != nullptr && first_birth != nullptr,
      "E5 exposes mutation targets");
  if (final_batch == nullptr || first_birth == nullptr) {
    return;
  }
  const auto final_authority = authority_for(context.plan, *final_batch);
  const auto baseline = run_batch(
      context,
      LbvhTraversalOrder::near_first,
      final_batch->batch_index,
      final_authority,
      unlimited_batch_budget());
  const auto rejects = [&](const auto& candidate, const std::string& label) {
    const auto verification = verify_batch(
        context,
        LbvhTraversalOrder::near_first,
        final_batch->batch_index,
        final_authority,
        unlimited_batch_budget(),
        candidate);
    check(
        !verification.observed_recursively_equal &&
            !verification.result_certified,
        label);
  };

  auto bad_residual = baseline;
  ++bad_residual.residual_incidence_records.front().owner_group_index;
  check(
      !bad_residual.certified_frozen_unified_incidence_batch(),
      "the structural predicate rejects a residual record with a forged owner");
  rejects(
      bad_residual,
      "fresh replay rejects a residual record with a forged owner");
  auto bad_equal = baseline;
  ++bad_equal.equal_facet_binding_records.front().equal_facet_token_id;
  check(
      !bad_equal.certified_frozen_unified_incidence_batch(),
      "the structural predicate rejects a forged equal-facet binding");
  rejects(
      bad_equal,
      "fresh replay rejects a forged equal-facet binding");
  auto bad_facet_owner = baseline;
  ++bad_facet_owner.coverage_delta_facets.front()
        .first_source_hyperedge_index;
  check(
      !bad_facet_owner.certified_frozen_unified_incidence_batch(),
      "the structural predicate rejects a forged first facet owner");
  rejects(
      bad_facet_owner,
      "fresh replay rejects a forged first facet provenance owner");
  auto bad_action = baseline;
  bad_action.action_plan.groups.front().q_r = 0U;
  check(
      !bad_action.certified_frozen_unified_incidence_batch(),
      "the structural predicate rejects a forged subordinate qR action");
  rejects(
      bad_action,
      "fresh replay rejects a forged subordinate qR action");

  auto bad_delta_q_r = baseline;
  ++bad_delta_q_r.coverage_deltas.front().q_r;
  check(
      !bad_delta_q_r.certified_frozen_unified_incidence_batch(),
      "the structural predicate rejects a forged delta qR");
  rejects(
      bad_delta_q_r,
      "fresh replay rejects a forged delta qR");

  auto bad_delta_action = baseline;
  bad_delta_action.coverage_deltas.front().action =
      bad_delta_action.coverage_deltas.front().action ==
              ExactFrozenIncidenceHgpAction::reduced_birth
          ? ExactFrozenIncidenceHgpAction::continuation
          : ExactFrozenIncidenceHgpAction::reduced_birth;
  check(
      !bad_delta_action.certified_frozen_unified_incidence_batch(),
      "the structural predicate rejects a forged delta action");
  rejects(
      bad_delta_action,
      "fresh replay rejects a forged delta action");

  const auto birth_authority = authority_for(context.plan, *first_birth);
  const auto birth = run_batch(
      context,
      LbvhTraversalOrder::near_first,
      first_birth->batch_index,
      birth_authority,
      unlimited_batch_budget());
  auto bad_point_owner = birth;
  ++bad_point_owner.coverage_delta_points.front()
        .first_source_hyperedge_index;
  const auto birth_verification = verify_batch(
      context,
      LbvhTraversalOrder::near_first,
      first_birth->batch_index,
      birth_authority,
      unlimited_batch_budget(),
      bad_point_owner);
  check(
      !birth_verification.observed_recursively_equal &&
          !birth_verification.result_certified,
      "fresh replay rejects a forged first point provenance owner");

  BatchAuthority bad_coverage = final_authority;
  bad_coverage.coverage_points.pop_back();
  --bad_coverage.coverages.front().point_reference_count;
  const auto rejected_coverage = run_batch(
      context,
      LbvhTraversalOrder::near_first,
      final_batch->batch_index,
      bad_coverage,
      unlimited_batch_budget());
  check(
      rejected_coverage.atomic_empty_failure(),
      "an incomplete pre-batch root coverage fails without partial records");

  auto forged_plan = context.plan;
  forged_plan.decision =
      ExactDirectSparseUnifiedLevelPlanDecision::not_certified;
  const auto rejected_plan =
      build_exact_direct_frozen_unified_incidence_batch(
          context.index,
          context.cloud,
          context.source.facade,
          context.source.event_journal,
          context.source.arm_budget,
          context.source.arm_journal,
          context.source.incidence_budget,
          context.source.incidence_journal,
          context.star_budget,
          LbvhTraversalOrder::near_first,
          context.star,
          context.plan_budget,
          forged_plan,
          final_batch->batch_index,
          final_authority.resolutions,
          final_authority.coverages,
          final_authority.coverage_points,
          final_authority.latent_coverages,
          final_authority.latent_coverage_points,
          unlimited_batch_budget());
  check(
      rejected_plan.atomic_empty_failure() &&
          rejected_plan.decision ==
              ExactDirectFrozenUnifiedIncidenceBatchDecision::
                  no_batch_source_plan_not_freshly_verified,
      "the batch builder rejects a plan result that fails its original-authority replay");
}

}  // namespace

int main() {
  const ExactDirectFrozenUnifiedIncidenceBatchResult default_result;
  check(
      !default_result.certified_frozen_unified_incidence_batch() &&
          !default_result.atomic_empty_failure(),
      "a default non-certified result is neither success nor an atomic failure");
  const E5Context context = e5_context(LbvhTraversalOrder::near_first);
  test_e5_all_batches_and_exact_records(context);
  test_latent_coverage_and_point_scan_regressions(context);
  test_mixed_local_delta_and_canonical_ownership();
  test_permutations_and_caps(context);
  test_mutations_fail_fresh_replay(context);
  if (failures != 0) {
    std::cerr << failures
              << " frozen unified incidence batch test(s) failed\n";
    return 1;
  }
  std::cout << "all frozen unified incidence batch tests passed\n";
  return 0;
}
