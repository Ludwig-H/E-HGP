#include "morsehgp3d/hierarchy/direct_sparse_unified_level_plan.hpp"

#include <algorithm>
#include <array>
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

[[nodiscard]] bool complete_verification(
    const ExactDirectSparseUnifiedLevelPlanVerification& verification) {
  return verification.observed_storage_within_budget &&
         verification.source_star_freshly_verified &&
         verification.expected_result_freshly_reconstructed &&
         verification.observed_recursively_equal &&
         verification.no_forbidden_global_structure_materialized &&
         verification.fresh_replay_certified &&
         verification.result_certified;
}

[[nodiscard]] ExactDirectSparseSuccessiveIncidenceStarJournalResult
build_star(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectSources& source,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& budget,
    LbvhTraversalOrder traversal_order) {
  return build_exact_direct_sparse_successive_incidence_star_journal(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      budget,
      traversal_order);
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanResult build_and_verify(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const DirectSources& source,
    const ExactDirectSparseSuccessiveIncidenceStarJournalBudget& star_budget,
    LbvhTraversalOrder traversal_order,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    const ExactDirectSparseUnifiedLevelPlanBudget& plan_budget,
    const std::string& context) {
  const auto result = build_exact_direct_sparse_unified_level_plan(
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
  const auto verification = verify_exact_direct_sparse_unified_level_plan(
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
      plan_budget,
      result);
  check(
      complete_verification(verification),
      context + " closes under a fresh recursive replay");
  return result;
}

[[nodiscard]] std::vector<PointId> star_coface_ids(
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    std::size_t coface_index) {
  const auto key =
      reconstruct_exact_direct_sparse_successive_incidence_star_coface(
          star, coface_index);
  return {
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
  };
}

[[nodiscard]] std::vector<PointId> direct_event_ids(
    const DirectSources& source,
    const ExactDirectSparseUnifiedLevelPlanDirectReference& reference) {
  const ExactDirectMorseEventJournalView view{source.event_journal};
  const auto& projection = view.materialized_direct_event_projection_at(
      reference.source_event_projection_index);
  const auto& event = source.facade.events[projection.source_index];
  std::vector<PointId> ids;
  ids.insert(
      ids.end(),
      event.support_ids.begin(),
      event.support_ids.begin() +
          static_cast<std::ptrdiff_t>(event.support_size));
  ids.insert(ids.end(), event.interior_ids.begin(), event.interior_ids.end());
  std::sort(ids.begin(), ids.end());
  return ids;
}

[[nodiscard]] std::vector<PointId> facet_token_ids(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    std::size_t token_index) {
  const auto& key = plan.facet_tokens[token_index].facet_key;
  return {
      key.point_ids.begin(),
      key.point_ids.begin() + static_cast<std::ptrdiff_t>(key.point_count),
  };
}

[[nodiscard]] const ExactDirectSparseUnifiedLevelPlanBatch* find_batch(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactLevel& squared_level,
    std::size_t order = 2U) {
  const auto found = std::find_if(
      plan.batches.begin(),
      plan.batches.end(),
      [&](const auto& batch) {
        return batch.order == order && batch.squared_level == squared_level;
      });
  return found == plan.batches.end() ? nullptr : &*found;
}

[[nodiscard]] std::vector<std::vector<PointId>> batch_direct_saddles(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const DirectSources& source,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch) {
  std::vector<std::vector<PointId>> ids;
  for (std::size_t local = 0U; local < batch.direct_reference_count;
       ++local) {
    const auto& reference =
        plan.direct_references[batch.direct_reference_offset + local];
    if (reference.role == ExactDirectMorseH0Role::saddle) {
      ids.push_back(direct_event_ids(source, reference));
    }
  }
  std::sort(ids.begin(), ids.end());
  return ids;
}

[[nodiscard]] std::vector<std::vector<PointId>> batch_residuals(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectSparseSuccessiveIncidenceStarJournalResult& star,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch) {
  std::vector<std::vector<PointId>> ids;
  for (std::size_t local = 0U; local < batch.residual_reference_count;
       ++local) {
    const auto& reference =
        plan.residual_references[batch.residual_reference_offset + local];
    ids.push_back(star_coface_ids(star, reference.source_star_coface_index));
  }
  std::sort(ids.begin(), ids.end());
  return ids;
}

[[nodiscard]] std::vector<std::vector<PointId>> batch_target_facets(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    const ExactDirectSparseUnifiedLevelPlanBatch& batch) {
  std::vector<std::vector<PointId>> ids;
  for (std::size_t local = 0U;
       local < batch.coface_facet_reference_count;
       ++local) {
    ids.push_back(facet_token_ids(
        plan,
        plan.coface_facet_references[
                batch.coface_facet_reference_offset + local]
            .facet_token_index));
  }
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

[[nodiscard]] bool plan_arenas_empty(
    const ExactDirectSparseUnifiedLevelPlanResult& plan) {
  return plan.facet_tokens.empty() && plan.direct_references.empty() &&
         plan.residual_references.empty() &&
         plan.coface_facet_references.empty() && plan.batches.empty();
}

[[nodiscard]] ExactDirectSparseUnifiedLevelPlanBudget exact_budget_for(
    const ExactDirectSparseUnifiedLevelPlanResult& plan) {
  return {
      plan.required_source_role_scan_count,
      plan.required_higher_order_direct_role_count,
      plan.required_source_family_scan_count,
      plan.required_higher_order_saddle_family_count,
      plan.required_star_coface_scan_count,
      plan.required_star_direct_coface_count,
      plan.required_star_residual_coface_count,
      plan.required_star_coface_point_scan_count,
      plan.required_coface_deletion_reference_count,
      plan.required_distinct_facet_token_count,
      plan.required_facet_key_point_count,
      plan.required_batch_count,
      plan.required_direct_reference_count,
      plan.required_residual_reference_count,
      plan.logical_storage_entry_count,
  };
}

void test_e5_complete_unified_plan() {
  const CanonicalPointCloud cloud = e5_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 2U);
  const auto star_budget = generous_star_budget();
  const auto star = build_star(
      index, cloud, source, star_budget, LbvhTraversalOrder::near_first);
  const auto plan = build_and_verify(
      index,
      cloud,
      source,
      star_budget,
      LbvhTraversalOrder::near_first,
      star,
      generous_plan_budget(),
      "the complete E5 unified direct-residual plan");

  const std::size_t residual_group_count = static_cast<std::size_t>(
      std::count_if(plan.batches.begin(), plan.batches.end(), [](const auto& b) {
        return b.residual_reference_count != 0U;
      }));
  check(
      plan.certified_bounded_plan() && plan.batches.size() == 12U &&
          plan.direct_references.size() == 10U &&
          plan.required_direct_birth_reference_count == 6U &&
          plan.required_direct_saddle_reference_count == 4U &&
          plan.residual_references.size() == 6U &&
          residual_group_count == 3U &&
          plan.direct_references.size() + residual_group_count == 13U,
      "E5 plans twelve checkpoints for ten direct roles and three downstream residual groups");
  check(
      plan.required_source_role_scan_count == 16U &&
          plan.excluded_order_one_role_count == 6U &&
          plan.required_source_family_scan_count == 10U &&
          plan.excluded_order_one_family_count == 6U &&
          plan.order_one_roles_and_families_excluded_to_preserve_boruvka_authority,
      "E5 scans every physical direct authority but excludes its six K1 roles and families");
  check(
      plan.facet_tokens.size() == 10U &&
          plan.required_facet_key_point_count == 20U &&
          plan.coface_facet_references.size() == 30U &&
          plan.required_star_coface_point_scan_count == 30U &&
          std::all_of(
              plan.facet_tokens.begin(),
              plan.facet_tokens.end(),
              [](const auto& token) {
                return token.facet_key.point_count == 2U;
              }),
      "all ten E5 edge tokens receive all thirty factorized triangle deletions");

  std::size_t birth_reference_count = 0U;
  std::size_t birth_only_checkpoint_count = 0U;
  for (const auto& batch : plan.batches) {
    const bool birth_only =
        batch.direct_reference_count != 0U &&
        batch.residual_reference_count == 0U &&
        batch.coface_facet_reference_count == 0U &&
        std::all_of(
            plan.direct_references.begin() +
                static_cast<std::ptrdiff_t>(batch.direct_reference_offset),
            plan.direct_references.begin() +
                static_cast<std::ptrdiff_t>(
                    batch.direct_reference_offset +
                    batch.direct_reference_count),
            [](const auto& reference) {
              return reference.role == ExactDirectMorseH0Role::birth;
            });
    if (birth_only) {
      ++birth_only_checkpoint_count;
      birth_reference_count += batch.direct_reference_count;
    }
  }
  check(
      birth_only_checkpoint_count == 5U && birth_reference_count == 6U,
      "five E5 birth-only checkpoints retain all six direct facet births in exact level order");

  struct ExpectedSuffix {
    ExactLevel squared_level;
    std::vector<std::vector<PointId>> direct;
    std::vector<std::vector<PointId>> residual;
    std::size_t deletion_count{};
  };
  const std::vector<ExpectedSuffix> expected{
      {level(25, 16), {{0U, 1U, 2U}}, {}, 3U},
      {level(1105, 242), {{2U, 3U, 4U}}, {}, 3U},
      {level(13, 2), {{1U, 2U, 3U}}, {}, 3U},
      {level(17, 2), {}, {{0U, 1U, 3U}, {0U, 2U, 3U}}, 6U},
      {level(9), {{0U, 2U, 4U}}, {}, 3U},
      {level(85, 9), {}, {{0U, 3U, 4U}}, 3U},
      {level(10),
       {},
       {{0U, 1U, 4U}, {1U, 2U, 4U}, {1U, 3U, 4U}},
       9U},
  };
  bool suffix_exact = true;
  for (const auto& item : expected) {
    const auto* batch = find_batch(plan, item.squared_level);
    suffix_exact =
        suffix_exact && batch != nullptr &&
        batch_direct_saddles(plan, source, *batch) == item.direct &&
        batch_residuals(plan, star, *batch) == item.residual &&
        batch->coface_facet_reference_count == item.deletion_count;
  }
  check(
      suffix_exact,
      "the seven E5 suffix batches retain four direct triangles and the three exact residual shells");

  const auto* at_13_2 = find_batch(plan, level(13, 2));
  const auto* at_17_2 = find_batch(plan, level(17, 2));
  const auto* at_9 = find_batch(plan, level(9));
  const auto* at_85_9 = find_batch(plan, level(85, 9));
  const auto* at_10 = find_batch(plan, level(10));
  std::vector<std::vector<PointId>> facets_13_2;
  std::vector<std::vector<PointId>> facets_17_2;
  std::vector<std::vector<PointId>> facets_9;
  std::vector<std::vector<PointId>> facets_85_9;
  std::vector<std::vector<PointId>> facets_10;
  if (at_13_2 != nullptr && at_17_2 != nullptr && at_9 != nullptr &&
      at_85_9 != nullptr && at_10 != nullptr) {
    facets_13_2 = batch_target_facets(plan, *at_13_2);
    facets_17_2 = batch_target_facets(plan, *at_17_2);
    facets_9 = batch_target_facets(plan, *at_9);
    facets_85_9 = batch_target_facets(plan, *at_85_9);
    facets_10 = batch_target_facets(plan, *at_10);
  }
  check(
      at_13_2 != nullptr && at_17_2 != nullptr && at_9 != nullptr &&
          at_85_9 != nullptr && at_10 != nullptr &&
          std::binary_search(
              facets_13_2.begin(),
              facets_13_2.end(),
              std::vector<PointId>{1U, 3U}) &&
          std::binary_search(
              facets_9.begin(),
              facets_9.end(),
              std::vector<PointId>{0U, 4U}) &&
          std::binary_search(
              facets_17_2.begin(),
              facets_17_2.end(),
              std::vector<PointId>{0U, 3U}) &&
          facets_85_9.size() == 3U &&
          std::binary_search(
              facets_10.begin(),
              facets_10.end(),
              std::vector<PointId>{1U, 4U}),
      "complete coface deletions expose the future 13, 04, 03 and 14 facet-token joins without reducer mutation");

  bool every_coface_has_three_deletions = true;
  for (const auto& coface : star.cofaces) {
    const std::size_t count = static_cast<std::size_t>(std::count_if(
        plan.coface_facet_references.begin(),
        plan.coface_facet_references.end(),
        [&](const auto& reference) {
          return reference.source_star_coface_index == coface.coface_index;
        }));
    every_coface_has_three_deletions =
        every_coface_has_three_deletions && count == 3U;
  }
  check(
      every_coface_has_three_deletions &&
          std::none_of(
              plan.residual_references.begin(),
              plan.residual_references.end(),
              [&](const auto& residual) {
                return star.cofaces[residual.source_star_coface_index].kind ==
                       ExactDirectSparseSuccessiveIncidenceStarCofaceKind::
                           direct_family;
              }),
      "every direct or residual star coface contributes all deletions while direct cofaces never duplicate a hyperedge");
}

void test_near_far_scientific_plan_agreement() {
  const CanonicalPointCloud cloud = e5_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 2U);
  const auto star_budget = generous_star_budget();
  const auto near_star = build_star(
      index, cloud, source, star_budget, LbvhTraversalOrder::near_first);
  const auto far_star = build_star(
      index, cloud, source, star_budget, LbvhTraversalOrder::far_first);
  const auto near = build_and_verify(
      index,
      cloud,
      source,
      star_budget,
      LbvhTraversalOrder::near_first,
      near_star,
      generous_plan_budget(),
      "the near-first E5 unified plan");
  const auto far = build_and_verify(
      index,
      cloud,
      source,
      star_budget,
      LbvhTraversalOrder::far_first,
      far_star,
      generous_plan_budget(),
      "the far-first E5 unified plan");
  check(
      near.facet_tokens == far.facet_tokens &&
          near.direct_references == far.direct_references &&
          near.residual_references == far.residual_references &&
          near.coface_facet_references == far.coface_facet_references &&
          near.batches == far.batches &&
          near.logical_storage_entry_count == far.logical_storage_entry_count,
      "near-first and far-first stars produce identical unified scientific arenas");
}

void test_exact_caps_fail_atomically() {
  const CanonicalPointCloud cloud = e5_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 2U);
  const auto star_budget = generous_star_budget();
  const auto star = build_star(
      index, cloud, source, star_budget, LbvhTraversalOrder::near_first);
  const auto baseline = build_and_verify(
      index,
      cloud,
      source,
      star_budget,
      LbvhTraversalOrder::near_first,
      star,
      generous_plan_budget(),
      "the E5 unified exact-cap baseline");
  const auto exact = exact_budget_for(baseline);
  const auto exact_result = build_and_verify(
      index,
      cloud,
      source,
      star_budget,
      LbvhTraversalOrder::near_first,
      star,
      exact,
      "the componentwise exact E5 unified caps");
  check(
      exact_result.facet_tokens == baseline.facet_tokens &&
          exact_result.batches == baseline.batches,
      "the componentwise exact caps reproduce all E5 tokens and batches");

  std::vector<ExactDirectSparseUnifiedLevelPlanBudget> short_budgets;
  const auto push_short = [&short_budgets](const auto& base, auto member) {
    auto candidate = base;
    std::size_t& cap = candidate.*member;
    if (cap != 0U) {
      --cap;
      short_budgets.push_back(std::move(candidate));
    }
  };
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_source_role_scan_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_higher_order_direct_role_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_source_family_scan_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_higher_order_saddle_family_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_star_coface_scan_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_star_direct_coface_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_star_residual_coface_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_star_coface_point_scan_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_coface_deletion_reference_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_distinct_facet_token_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_facet_key_point_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::maximum_batch_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_direct_reference_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_residual_reference_count);
  push_short(
      exact,
      &ExactDirectSparseUnifiedLevelPlanBudget::
          maximum_logical_storage_entry_count);

  bool every_short_cap_is_atomic = short_budgets.size() == 15U;
  for (const auto& short_budget : short_budgets) {
    const auto rejected = build_exact_direct_sparse_unified_level_plan(
        index,
        cloud,
        source.facade,
        source.event_journal,
        source.arm_budget,
        source.arm_journal,
        source.incidence_budget,
        source.incidence_journal,
        star_budget,
        LbvhTraversalOrder::near_first,
        star,
        short_budget);
    every_short_cap_is_atomic =
        every_short_cap_is_atomic && !rejected.certified_bounded_plan() &&
        rejected.no_partial_scientific_payload_published &&
        plan_arenas_empty(rejected);
  }
  check(
      every_short_cap_is_atomic,
      "all fifteen one-short plan caps publish five empty arenas atomically");
}

void test_mutations_and_forged_star_fail_fresh_replay() {
  const CanonicalPointCloud cloud = e5_cloud();
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const DirectSources source = direct_sources(cloud, 2U);
  const auto star_budget = generous_star_budget();
  const auto star = build_star(
      index, cloud, source, star_budget, LbvhTraversalOrder::near_first);
  const auto plan_budget = generous_plan_budget();
  const auto baseline = build_and_verify(
      index,
      cloud,
      source,
      star_budget,
      LbvhTraversalOrder::near_first,
      star,
      plan_budget,
      "the E5 unified mutation baseline");

  std::vector<ExactDirectSparseUnifiedLevelPlanResult> forged;
  auto wrong_role = baseline;
  wrong_role.direct_references.front().role = ExactDirectMorseH0Role::saddle;
  forged.push_back(std::move(wrong_role));
  auto wrong_direct_star = baseline;
  const auto saddle = std::find_if(
      wrong_direct_star.direct_references.begin(),
      wrong_direct_star.direct_references.end(),
      [](const auto& reference) {
        return reference.role == ExactDirectMorseH0Role::saddle;
      });
  saddle->source_star_direct_coface_index =
      (*saddle->source_star_direct_coface_index + 1U) % star.cofaces.size();
  forged.push_back(std::move(wrong_direct_star));
  auto wrong_residual = baseline;
  wrong_residual.residual_references.front().source_star_coface_index = 0U;
  forged.push_back(std::move(wrong_residual));
  auto wrong_token = baseline;
  std::swap(
      wrong_token.facet_tokens.front().facet_key.point_ids[0U],
      wrong_token.facet_tokens.front().facet_key.point_ids[1U]);
  forged.push_back(std::move(wrong_token));
  auto wrong_deletion = baseline;
  wrong_deletion.coface_facet_references.front().facet_token_index =
      (wrong_deletion.coface_facet_references.front().facet_token_index + 1U) %
      wrong_deletion.facet_tokens.size();
  forged.push_back(std::move(wrong_deletion));
  auto wrong_removed = baseline;
  wrong_removed.coface_facet_references.front().removed_point_id =
      static_cast<PointId>(
          (wrong_removed.coface_facet_references.front().removed_point_id +
           1U) %
          cloud.size());
  forged.push_back(std::move(wrong_removed));
  auto wrong_batch = baseline;
  wrong_batch.batches.front().squared_level = level(999);
  forged.push_back(std::move(wrong_batch));
  auto wrong_snapshot = baseline;
  wrong_snapshot.batches.front().future_snapshot_index = 1U;
  forged.push_back(std::move(wrong_snapshot));
  auto wrong_fact = baseline;
  wrong_fact.reducer_snapshot_or_forest_mutated = true;
  forged.push_back(std::move(wrong_fact));

  bool every_mutation_rejected = forged.size() == 9U;
  for (std::size_t mutation_index = 0U; mutation_index < forged.size();
       ++mutation_index) {
    const auto& candidate = forged[mutation_index];
    const auto verification = verify_exact_direct_sparse_unified_level_plan(
        index,
        cloud,
        source.facade,
        source.event_journal,
        source.arm_budget,
        source.arm_journal,
        source.incidence_budget,
        source.incidence_journal,
        star_budget,
        LbvhTraversalOrder::near_first,
        star,
        plan_budget,
        candidate);
    every_mutation_rejected =
        every_mutation_rejected && !verification.result_certified;
  }
  check(
      every_mutation_rejected,
      "fresh verification rejects direct, residual, token, deletion, batch, snapshot and mutation forgeries");

  auto forged_star = star;
  forged_star.cofaces.front().kind =
      ExactDirectSparseSuccessiveIncidenceStarCofaceKind::residual;
  const auto rejected = build_exact_direct_sparse_unified_level_plan(
      index,
      cloud,
      source.facade,
      source.event_journal,
      source.arm_budget,
      source.arm_journal,
      source.incidence_budget,
      source.incidence_journal,
      star_budget,
      LbvhTraversalOrder::near_first,
      forged_star,
      plan_budget);
  check(
      rejected.decision == ExactDirectSparseUnifiedLevelPlanDecision::
                               no_plan_source_star_not_certified &&
          plan_arenas_empty(rejected),
      "a forged star is rejected by fresh replay before any plan payload is published");
}

}  // namespace

int main() {
  test_e5_complete_unified_plan();
  test_near_far_scientific_plan_agreement();
  test_exact_caps_fail_atomically();
  test_mutations_and_forged_star_fail_fresh_replay();
  if (failures != 0) {
    std::cerr << failures << " direct sparse unified-level-plan test(s) failed\n";
    return 1;
  }
  std::cout << "all direct sparse unified-level-plan tests passed\n";
  return 0;
}
