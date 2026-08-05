#include "morsehgp3d/hierarchy/direct_morse_resident_naturality_square_campaign.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <new>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include <boost/multiprecision/integer.hpp>


namespace morsehgp3d::hierarchy {
namespace {

using Key = ExactDirectNormalizedVerticalSimplexKey;
using CampaignBudget = ExactDirectMorseResidentNaturalitySquareCampaignBudget;
using CampaignCounters =
    ExactDirectMorseResidentNaturalitySquareCampaignCounters;
using CampaignDecision =
    ExactDirectMorseResidentNaturalitySquareCampaignDecision;
using CampaignResult = ExactDirectMorseResidentNaturalitySquareCampaignResult;
using CampaignScope = ExactDirectMorseResidentNaturalitySquareCampaignScope;
using Attestation = ExactDirectMorseResidentNaturalitySquareAttestation;

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& result) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] std::size_t integer_bit_count(
    const exact::BigInt& value) noexcept {
  // ExactLevel numerators are nonnegative and denominators are positive.
  if (value == 0) {
    return 1U;
  }
  return static_cast<std::size_t>(boost::multiprecision::msb(value)) + 1U;
}

[[nodiscard]] bool key_less(const Key& left, const Key& right) noexcept {
  if (left.point_count != right.point_count) {
    return left.point_count < right.point_count;
  }
  for (std::size_t index = 0U; index < left.point_count; ++index) {
    if (left.point_ids[index] != right.point_ids[index]) {
      return left.point_ids[index] < right.point_ids[index];
    }
  }
  return false;
}

[[nodiscard]] bool key_shape_valid(
    const Key& key,
    std::size_t point_count,
    std::size_t expected_cardinality) noexcept {
  if (key.point_count != expected_cardinality || key.point_count == 0U ||
      key.point_count > key.point_ids.size()) {
    return false;
  }
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] >= point_count ||
        (index != 0U && key.point_ids[index - 1U] >= key.point_ids[index])) {
      return false;
    }
  }
  for (std::size_t index = key.point_count; index < key.point_ids.size();
       ++index) {
    if (key.point_ids[index] != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] Key key_from_sparse_facet_key(
    const ExactDirectSparseFacetKey& facet) noexcept {
  Key key{};
  key.point_count =
      std::min<std::size_t>(facet.point_count, key.point_ids.size());
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    key.point_ids[index] = facet.point_ids[index];
  }
  return key;
}

[[nodiscard]] Key key_from_reconstructed_coface(
    const ExactDirectNormalizedH0ReconstructedCoface& coface) noexcept {
  Key key{};
  key.point_count =
      std::min<std::size_t>(coface.point_count, key.point_ids.size());
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    key.point_ids[index] = coface.point_ids[index];
  }
  return key;
}

[[nodiscard]] Key key_deletion(const Key& key, std::size_t skip) noexcept {
  Key deletion{};
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (index == skip) {
      continue;
    }
    deletion.point_ids[deletion.point_count++] = key.point_ids[index];
  }
  return deletion;
}

// Sorted-merge union and intersection of two strictly increasing keys.  The
// spanning-tree contract needs union == coface key (order K+1) and yields the
// common target facet as the intersection (order K-1).
[[nodiscard]] bool key_union_and_intersection(
    const Key& left,
    const Key& right,
    Key& observed_union,
    Key& observed_intersection) noexcept {
  observed_union = {};
  observed_intersection = {};
  std::size_t left_index = 0U;
  std::size_t right_index = 0U;
  while (left_index < left.point_count || right_index < right.point_count) {
    if (right_index == right.point_count ||
        (left_index < left.point_count &&
         left.point_ids[left_index] < right.point_ids[right_index])) {
      if (observed_union.point_count == observed_union.point_ids.size()) {
        return false;
      }
      observed_union.point_ids[observed_union.point_count++] =
          left.point_ids[left_index++];
      continue;
    }
    if (left_index == left.point_count ||
        right.point_ids[right_index] < left.point_ids[left_index]) {
      if (observed_union.point_count == observed_union.point_ids.size()) {
        return false;
      }
      observed_union.point_ids[observed_union.point_count++] =
          right.point_ids[right_index++];
      continue;
    }
    const spatial::PointId shared = left.point_ids[left_index];
    if (observed_union.point_count == observed_union.point_ids.size() ||
        observed_intersection.point_count ==
            observed_intersection.point_ids.size()) {
      return false;
    }
    observed_union.point_ids[observed_union.point_count++] = shared;
    observed_intersection.point_ids[observed_intersection.point_count++] =
        shared;
    ++left_index;
    ++right_index;
  }
  return true;
}

[[nodiscard]] bool key_is_subset(const Key& small, const Key& big) noexcept {
  std::size_t big_index = 0U;
  for (std::size_t index = 0U; index < small.point_count; ++index) {
    while (big_index < big.point_count &&
           big.point_ids[big_index] < small.point_ids[index]) {
      ++big_index;
    }
    if (big_index == big.point_count ||
        big.point_ids[big_index] != small.point_ids[index]) {
      return false;
    }
    ++big_index;
  }
  return true;
}

[[nodiscard]] bool key_contains_point(
    const Key& key,
    spatial::PointId point) noexcept {
  for (std::size_t index = 0U; index < key.point_count; ++index) {
    if (key.point_ids[index] == point) {
      return true;
    }
  }
  return false;
}

class Accounting {
 public:
  Accounting(
      const CampaignBudget& budget,
      CampaignCounters& counters) noexcept
      : budget_(budget), counters_(counters) {}

  [[nodiscard]] bool charge(
      std::size_t& counter,
      std::size_t maximum) noexcept {
    if (counter >= maximum) {
      exhausted_ = true;
      return false;
    }
    ++counter;
    return true;
  }

  [[nodiscard]] bool measure_level(const exact::ExactLevel& level) noexcept {
    const std::size_t observed = std::max(
        integer_bit_count(level.numerator()),
        integer_bit_count(level.denominator()));
    counters_.maximum_observed_exact_level_integer_bit_count = std::max(
        counters_.maximum_observed_exact_level_integer_bit_count, observed);
    if (observed > budget_.maximum_single_exact_level_integer_bit_count) {
      exhausted_ = true;
      return false;
    }
    return true;
  }

  [[nodiscard]] bool level_equal(
      const exact::ExactLevel& left,
      const exact::ExactLevel& right) {
    if (!charge(
            counters_.exact_level_comparison_count,
            budget_.maximum_exact_level_comparison_count)) {
      return false;
    }
    relation_ = left == right ? 0 : (left < right ? -1 : 1);
    return true;
  }

  [[nodiscard]] int relation() const noexcept { return relation_; }

  [[nodiscard]] bool charge_key_points(std::size_t count) noexcept {
    if (count >
        budget_.maximum_key_point_scan_count -
            std::min(
                budget_.maximum_key_point_scan_count,
                counters_.key_point_scan_count)) {
      exhausted_ = true;
      return false;
    }
    counters_.key_point_scan_count += count;
    return true;
  }

  [[nodiscard]] bool charge_key_lookup_comparison() noexcept {
    return charge(
        counters_.key_lookup_comparison_count,
        budget_.maximum_key_lookup_comparison_count);
  }

  [[nodiscard]] bool exhausted() const noexcept { return exhausted_; }

 private:
  const CampaignBudget& budget_;
  CampaignCounters& counters_;
  int relation_{};
  bool exhausted_{false};
};

[[nodiscard]] bool unsupported_campaign_claims_false(
    const CampaignResult& result) noexcept {
  return result.conditional_on_sealed_bridge_and_normalized_source &&
         !result.external_target_authority_replayed &&
         !result.global_morse_obligation_replayed &&
         !result.full_pi0_membership_claimed && !result.m1_replayed &&
         !result.vertical_maps_complete &&
         !result.global_facet_coface_gamma_or_higher_delaunay_materialized &&
         !result.pair_matrix_materialized && !result.public_status_claimed;
}

void clear_campaign_payload(CampaignResult& result) noexcept {
  result.square_attestations.clear();
  result.counters = {};
}

[[nodiscard]] CampaignResult fail_campaign(
    CampaignResult result,
    CampaignDecision decision) noexcept {
  clear_campaign_payload(result);
  result.sealed_bridge_live_verified = false;
  result.final_seal_cross_verified_against_live_bridge = false;
  result.normalized_v7_source_capability_live_required = false;
  result.normalized_source_surface_freshly_verified = false;
  result.canonical_point_namespace_identity_certified = false;
  result.exact_product_order_adjacent_schedule_reconstructed = false;
  result.every_component_membership_derived_incrementally_and_released =
      false;
  result.every_spanning_tree_derived_from_certified_source_adjacencies =
      false;
  result.every_target_carrier_binding_derived_from_sealed_bridge_images =
      false;
  result.every_horizontal_inclusion_bound_to_consecutive_closed_cuts = false;
  result.every_square_receipt_freshly_built = false;
  result.every_square_receipt_freshly_verified = false;
  result.every_singleton_component_skip_justified = false;
  result.terminal_component_square_replayed = false;
  result.all_naturality_squares_replayed = false;
  result.no_partial_scientific_payload_published_on_failure = true;
  result.decision = decision;
  result.scope = CampaignScope::unspecified;
  return result;
}

// Growable disjoint-set forest over label insertion indices.  unite keeps the
// smaller insertion index as the root, so a component root doubles as its
// deterministic canonical label and (root + 1) as its nonzero handle.
class LabelForest {
 public:
  void add() { parents_.push_back(parents_.size()); }

  [[nodiscard]] std::size_t find(std::size_t value) noexcept {
    std::size_t root = value;
    while (parents_[root] != root) {
      root = parents_[root];
    }
    while (parents_[value] != value) {
      const std::size_t next = parents_[value];
      parents_[value] = root;
      value = next;
    }
    return root;
  }

  [[nodiscard]] bool unite(std::size_t left, std::size_t right) noexcept {
    left = find(left);
    right = find(right);
    if (left == right) {
      return false;
    }
    if (right < left) {
      std::swap(left, right);
    }
    parents_[right] = left;
    return true;
  }

  [[nodiscard]] std::size_t size() const noexcept { return parents_.size(); }

  void clear() noexcept { parents_.clear(); }

 private:
  std::vector<std::size_t> parents_;
};

struct CampaignLabel {
  Key key{};
  exact::ExactLevel birth_level{};
};

struct CampaignEdge {
  std::size_t left_label{};
  std::size_t right_label{};
  Key coface_key{};
  exact::ExactLevel coface_level{};
  Key common_facet_key{};
};

struct CampaignCoface {
  Key key{};
  exact::ExactLevel level{};
  std::optional<std::size_t> first_label;
};

// Per-adjacent-order-pair replay state.  Everything here is transient and is
// released when the order pair completes.
struct OrderState {
  std::size_t order{};
  std::vector<CampaignLabel> labels;
  std::vector<std::size_t> sorted_by_key;
  LabelForest forest;
  std::vector<std::size_t> component_size;
  std::vector<CampaignEdge> edges;
  std::vector<CampaignCoface> cofaces;
  std::vector<std::uint64_t> naming;
  std::vector<std::uint8_t> named;

  void clear() noexcept {
    labels.clear();
    sorted_by_key.clear();
    forest.clear();
    component_size.clear();
    edges.clear();
    cofaces.clear();
    naming.clear();
    named.clear();
  }
};

struct ComponentSnapshot {
  std::uint64_t handle{};
  std::uint64_t target_root_naming{};
  std::vector<std::size_t> labels_by_key;
  std::vector<std::size_t> tree_edge_indices;
};

enum class CutKind : std::uint8_t { k2, higher, terminal };

struct ScheduleEntry {
  std::size_t plan_batch_index{};
  std::size_t committed_index{};
  std::size_t order{};
  CutKind kind{CutKind::k2};
};

// One campaign run.  The engine mutates only the supplied result; every
// failure path reports one decision and the caller purges atomically.
//
// Scope notes bound to the maintainer decisions:
// * DECISION 1: closed-component membership is derived relative to the
//   certified candidate-complete normalized source plan only; labels are
//   deduplicated by canonical facet key and duplicate birth records must
//   agree on the exact birth level.
// * DECISION 2: order 1 is excluded as a source order.  Vertical squares are
//   built for source orders 2..K_eff only, so the K=2 squares target order 1
//   and pure order-1 evolution stays covered by the sealed owned K1 history.
// * DECISION 3: direct_normalized_vertical_square_receipt.cpp treats
//   closed_target_root_id as an opaque identifier with no zero sentinel, so
//   K1 node ids are passed through unchanged into target-order-1 carriers.
// * DECISION 4: horizontal inclusions assert
//   all_intermediate_target_levels_accounted on the strength of the final
//   seal facts (terminal monotone union history, terminal root reprobes and
//   the contiguous replay-token range); the campaign never re-probes.
class CampaignEngine {
 public:
  CampaignEngine(
      const ExactDirectMorseResidentAllOrdersVerticalBridge& bridge,
      const ExactDirectMorseResidentAllOrdersVerticalFinalSeal& seal,
      const ExactDirectNormalizedH0SourcePlanResult& plan,
      const ExactDirectSparseGatewayCandidateJournalResult& gateway,
      const ExactDirectSparseGatewayCandidateScientificIdentityResult&
          gateway_identity,
      const ExactDirectNormalizedH0IncidenceReductionAuthority& authority,
      const CampaignBudget& budget,
      CampaignResult& result) noexcept
      : bridge_(bridge),
        seal_(seal),
        plan_(plan),
        gateway_(gateway),
        gateway_identity_(gateway_identity),
        authority_(authority),
        budget_(budget),
        result_(result),
        accounting_(budget, result.counters) {}

  // Returns the terminal campaign decision.  Only
  // complete_conditional_all_naturality_squares_replayed is a success.
  [[nodiscard]] CampaignDecision run() {
    if (auto decision = preconditions()) {
      return *decision;
    }
    if (auto decision = preflight()) {
      return *decision;
    }
    if (auto decision = reconstruct_schedule()) {
      return *decision;
    }
    if (auto decision = replay_all_orders()) {
      return *decision;
    }
    if (auto decision = closure()) {
      return *decision;
    }
    return CampaignDecision::
        complete_conditional_all_naturality_squares_replayed;
  }

 private:
  using MaybeDecision = std::optional<CampaignDecision>;

  [[nodiscard]] CampaignDecision budget_or(
      CampaignDecision semantic) const noexcept {
    // Mid-replay budget exhaustion takes precedence over semantic
    // contradictions observed in the same condition chain.
    return accounting_.exhausted()
               ? CampaignDecision::no_campaign_budget_exhausted
               : semantic;
  }

  [[nodiscard]] MaybeDecision preconditions() {
    if (!bridge_.final_vertical_sealed() ||
        bridge_.final_vertical_seal() == nullptr) {
      return CampaignDecision::no_campaign_bridge_not_sealed;
    }
    if (!bridge_.verify_final_vertical_seal(seal_) ||
        !bridge_.verify_stamp(seal_.sealed_stamp) ||
        !seal_.certified_final_vertical_seal() ||
        bridge_.source_root_target_witnesses().size() !=
            seal_.sealed_persistent_source_root_witness_count) {
      return CampaignDecision::no_campaign_final_seal_mismatch;
    }
    // The v7 capability is required live, never inferred from a copied seal.
    if (!bridge_.normalized_incidence_complete_v7_source_capability() ||
        bridge_.resident_source_kind() !=
            ExactDirectMorseUnifiedResidentSourceKind::
                normalized_direct_h0_candidate_source ||
        !seal_.normalized_incidence_complete_v7_source_capability) {
      return CampaignDecision::
          no_campaign_normalized_v7_source_capability_rejected;
    }
    if (!plan_.certified_complete_candidate_source_plan() ||
        !authority_.certified_horizontal_incidence_reduction()) {
      return CampaignDecision::no_campaign_normalized_source_surface_rejected;
    }
    const contract::CanonicalId zero{};
    if (seal_.sealed_stamp.canonical_cloud_digest == zero ||
        seal_.sealed_stamp.resident_higher_canonical_cloud_digest == zero ||
        seal_.sealed_stamp.canonical_cloud_digest !=
            plan_.source_pair_canonical_cloud_digest ||
        seal_.sealed_stamp.resident_higher_canonical_cloud_digest !=
            plan_.source_higher_canonical_cloud_digest ||
        authority_.source_pair_canonical_cloud_digest !=
            plan_.source_pair_canonical_cloud_digest ||
        authority_.source_higher_canonical_cloud_digest !=
            plan_.source_higher_canonical_cloud_digest ||
        authority_.source_gateway_scientific_identity_digest !=
            plan_.source_gateway_scientific_identity_digest ||
        authority_.point_count != plan_.point_count ||
        result_.normalized_source_authority_id == 0U ||
        result_.target_carrier_authority_id == 0U ||
        result_.target_carrier_authority_id !=
            result_.normalized_source_authority_id) {
      return CampaignDecision::no_campaign_point_namespace_identity_rejected;
    }
    for (const auto& witness : bridge_.source_root_target_witnesses()) {
      if (!accounting_.charge(
              result_.counters.root_witness_scan_count,
              budget_.maximum_root_witness_scan_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      if (!witness.certified_conditional_root_witness()) {
        return CampaignDecision::no_campaign_final_seal_mismatch;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision preflight() {
    const auto& k2_batches = bridge_.committed_k2_batches();
    const auto& higher_batches = bridge_.committed_higher_batches();
    std::size_t committed_batch_total = 0U;
    std::size_t group_image_total = 0U;
    std::size_t k2_birth_binding_total = 0U;
    if (!checked_add(
            k2_batches.size(), higher_batches.size(), committed_batch_total)) {
      return CampaignDecision::no_campaign_capacity_overflow;
    }
    for (const auto& record : k2_batches) {
      if (!checked_add(
              group_image_total,
              record.group_images.size(),
              group_image_total) ||
          !checked_add(
              k2_birth_binding_total,
              record.direct_birth_k1_bindings.size(),
              k2_birth_binding_total)) {
        return CampaignDecision::no_campaign_capacity_overflow;
      }
    }
    for (const auto& record : higher_batches) {
      if (!checked_add(
              group_image_total,
              record.group_images.size(),
              group_image_total)) {
        return CampaignDecision::no_campaign_capacity_overflow;
      }
    }
    std::size_t group_record_scan_requirement = 0U;
    if (!checked_add(
            group_image_total,
            k2_birth_binding_total,
            group_record_scan_requirement)) {
      return CampaignDecision::no_campaign_capacity_overflow;
    }
    if (plan_.batches.size() > budget_.maximum_source_batch_scan_count ||
        committed_batch_total > budget_.maximum_source_batch_scan_count ||
        plan_.direct_birth_references.size() >
            budget_.maximum_source_plan_birth_reference_scan_count ||
        plan_.batch_coface_references.size() >
            budget_.maximum_source_plan_coface_scan_count ||
        plan_.cofaces.size() > budget_.maximum_coface_reconstruction_count ||
        group_record_scan_requirement >
            budget_.maximum_group_record_scan_count ||
        bridge_.source_root_target_witnesses().size() >
            budget_.maximum_root_witness_scan_count) {
      return CampaignDecision::no_campaign_budget_exhausted;
    }
    return std::nullopt;
  }

  // Rebuilds the exact product-order adjacent schedule: plan batches of
  // order >= 2 in product order must correspond one-to-one, in order, with
  // the committed K2 and higher batch records, and the committed group and
  // terminal populations must equal the sealed counters exactly.
  [[nodiscard]] MaybeDecision reconstruct_schedule() {
    const auto& k2_batches = bridge_.committed_k2_batches();
    const auto& higher_batches = bridge_.committed_higher_batches();
    std::size_t k2_cursor = 0U;
    std::size_t higher_cursor = 0U;
    std::size_t observed_k2_group_count = 0U;
    std::size_t observed_higher_group_count = 0U;
    std::size_t observed_terminal_count = 0U;
    bool source_batch_index_seen = false;
    std::size_t last_source_batch_index = 0U;
    for (std::size_t plan_index = 0U; plan_index < plan_.batches.size();
         ++plan_index) {
      if (!accounting_.charge(
              result_.counters.source_batch_scan_count,
              budget_.maximum_source_batch_scan_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      const auto& batch = plan_.batches[plan_index];
      if (batch.batch_index != plan_index ||
          !accounting_.measure_level(batch.squared_level)) {
        return budget_or(
            CampaignDecision::no_campaign_adjacent_order_schedule_rejected);
      }
      if (batch.order < 2U) {
        continue;
      }
      ScheduleEntry entry{};
      entry.plan_batch_index = plan_index;
      entry.order = batch.order;
      std::size_t source_batch_index = 0U;
      if (batch.order == 2U) {
        if (k2_cursor >= k2_batches.size()) {
          return CampaignDecision::
              no_campaign_adjacent_order_schedule_rejected;
        }
        const auto& record = k2_batches[k2_cursor];
        if (record.order != 2U ||
            !record.certified_conditional_k2_to_k1_batch() ||
            !accounting_.level_equal(
                record.squared_level, batch.squared_level) ||
            accounting_.relation() != 0) {
          return budget_or(
              CampaignDecision::no_campaign_adjacent_order_schedule_rejected);
        }
        source_batch_index = record.source_batch_index;
        if (!checked_add(
                observed_k2_group_count,
                record.group_images.size(),
                observed_k2_group_count)) {
          return CampaignDecision::no_campaign_capacity_overflow;
        }
        entry.committed_index = k2_cursor;
        entry.kind = CutKind::k2;
        ++k2_cursor;
      } else {
        if (higher_cursor >= higher_batches.size()) {
          return CampaignDecision::
              no_campaign_adjacent_order_schedule_rejected;
        }
        const auto& record = higher_batches[higher_cursor];
        if (record.source_order != batch.order ||
            record.target_order + 1U != record.source_order ||
            !record.certified_conditional_higher_batch() ||
            !accounting_.level_equal(
                record.squared_level, batch.squared_level) ||
            accounting_.relation() != 0) {
          return budget_or(
              CampaignDecision::no_campaign_adjacent_order_schedule_rejected);
        }
        source_batch_index = record.source_batch_index;
        if (!checked_add(
                observed_higher_group_count,
                record.group_images.size(),
                observed_higher_group_count)) {
          return CampaignDecision::no_campaign_capacity_overflow;
        }
        entry.committed_index = higher_cursor;
        if (record.terminal_component_image.has_value()) {
          if (record.source_order != plan_.point_count ||
              !record.group_images.empty()) {
            return CampaignDecision::
                no_campaign_adjacent_order_schedule_rejected;
          }
          entry.kind = CutKind::terminal;
          ++observed_terminal_count;
        } else {
          entry.kind = CutKind::higher;
        }
        ++higher_cursor;
      }
      if (source_batch_index_seen &&
          source_batch_index <= last_source_batch_index) {
        return CampaignDecision::no_campaign_adjacent_order_schedule_rejected;
      }
      source_batch_index_seen = true;
      last_source_batch_index = source_batch_index;
      if (entry.order >= order_entries_.size()) {
        return CampaignDecision::no_campaign_adjacent_order_schedule_rejected;
      }
      order_entries_[entry.order].push_back(schedule_.size());
      schedule_.push_back(entry);
      track_scratch(schedule_.size());
    }
    if (k2_cursor != k2_batches.size() ||
        higher_cursor != higher_batches.size() ||
        k2_batches.size() != seal_.required_k2_batch_count ||
        higher_batches.size() != seal_.required_higher_batch_count ||
        k2_batches.size() != seal_.sealed_stamp.committed_k2_batch_count ||
        higher_batches.size() !=
            seal_.sealed_stamp.committed_higher_batch_count ||
        observed_k2_group_count != seal_.sealed_k2_group_count ||
        observed_higher_group_count != seal_.sealed_higher_group_count ||
        observed_terminal_count !=
            seal_.sealed_terminal_component_image_count) {
      return CampaignDecision::no_campaign_adjacent_order_schedule_rejected;
    }
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision replay_all_orders() {
    std::size_t order_pair_index = 0U;
    for (std::size_t order = 2U; order < order_entries_.size(); ++order) {
      if (order_entries_[order].empty()) {
        continue;
      }
      if (!accounting_.charge(
              result_.counters.adjacent_order_pair_count,
              budget_.maximum_adjacent_order_pair_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      if (auto decision = replay_order(order)) {
        result_.rejected_adjacent_order_pair_index = order_pair_index;
        return decision;
      }
      ++order_pair_index;
    }
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision replay_order(std::size_t order) {
    OrderState state;
    state.order = order;
    const auto& entries = order_entries_[order];
    if (auto decision = process_batch(state, schedule_[entries[0U]])) {
      return decision;
    }
    for (std::size_t index = 1U; index < entries.size(); ++index) {
      if (!accounting_.charge(
              result_.counters.consecutive_closed_cut_pair_count,
              budget_.maximum_consecutive_closed_cut_pair_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      const std::size_t pair_index =
          result_.counters.consecutive_closed_cut_pair_count - 1U;
      std::vector<ComponentSnapshot> snapshots;
      if (auto decision = snapshot_components(state, snapshots)) {
        result_.rejected_consecutive_closed_cut_pair_index = pair_index;
        return decision;
      }
      if (auto decision = process_batch(state, schedule_[entries[index]])) {
        result_.rejected_consecutive_closed_cut_pair_index = pair_index;
        return decision;
      }
      const auto& before_batch =
          plan_.batches[schedule_[entries[index - 1U]].plan_batch_index];
      const auto& after_batch =
          plan_.batches[schedule_[entries[index]].plan_batch_index];
      if (auto decision = emit_squares(
              state,
              snapshots,
              before_batch.squared_level,
              after_batch.squared_level)) {
        result_.rejected_consecutive_closed_cut_pair_index = pair_index;
        return decision;
      }
      snapshots.clear();
      ++result_.counters.released_transient_arena_count;
    }
    state.clear();
    ++result_.counters.released_transient_arena_count;
    return std::nullopt;
  }

  struct BatchNaming {
    std::vector<std::pair<std::size_t, std::uint64_t>> named_roots;
    std::vector<std::size_t> touched_labels;
  };

  [[nodiscard]] MaybeDecision bump_scratch(
      const OrderState& state,
      std::size_t extra) {
    std::size_t total = schedule_.size();
    if (!checked_add(total, state.labels.size(), total) ||
        !checked_add(total, state.edges.size(), total) ||
        !checked_add(total, state.cofaces.size(), total) ||
        !checked_add(total, extra, total)) {
      return CampaignDecision::no_campaign_capacity_overflow;
    }
    result_.counters.peak_transient_scratch_entry_count = std::max(
        result_.counters.peak_transient_scratch_entry_count, total);
    if (total > budget_.maximum_transient_scratch_entry_count) {
      return CampaignDecision::no_campaign_budget_exhausted;
    }
    return std::nullopt;
  }

  void track_scratch(std::size_t total) noexcept {
    result_.counters.peak_transient_scratch_entry_count = std::max(
        result_.counters.peak_transient_scratch_entry_count, total);
  }

  // Charged binary search over the per-order canonical label index.
  [[nodiscard]] bool sorted_position(
      const OrderState& state,
      const Key& key,
      std::size_t& position,
      bool& found) {
    std::size_t first = 0U;
    std::size_t count = state.sorted_by_key.size();
    while (count != 0U) {
      if (!accounting_.charge_key_lookup_comparison()) {
        return false;
      }
      const std::size_t step = count / 2U;
      const std::size_t middle = first + step;
      if (key_less(state.labels[state.sorted_by_key[middle]].key, key)) {
        first = middle + 1U;
        count -= step + 1U;
      } else {
        count = step;
      }
    }
    position = first;
    found = false;
    if (first < state.sorted_by_key.size()) {
      if (!accounting_.charge_key_lookup_comparison()) {
        return false;
      }
      const Key& candidate = state.labels[state.sorted_by_key[first]].key;
      found = !key_less(key, candidate) && !key_less(candidate, key);
    }
    return true;
  }

  void unite_components(
      OrderState& state,
      std::size_t left,
      std::size_t right) noexcept {
    const std::size_t left_root = state.forest.find(left);
    const std::size_t right_root = state.forest.find(right);
    if (left_root == right_root) {
      return;
    }
    const bool both_named_equal =
        state.named[left_root] != 0U && state.named[right_root] != 0U &&
        state.naming[left_root] == state.naming[right_root];
    const std::uint64_t carried = state.naming[left_root];
    const std::size_t merged_size =
        state.component_size[left_root] + state.component_size[right_root];
    (void)state.forest.unite(left_root, right_root);
    const std::size_t root = state.forest.find(left_root);
    state.component_size[root] = merged_size;
    // A fused component must be renamed by a sealed group image at the same
    // closed cut unless both children already carried one identical naming.
    state.named[root] = both_named_equal ? 1U : 0U;
    state.naming[root] = both_named_equal ? carried : 0U;
  }

  [[nodiscard]] MaybeDecision name_component_root(
      OrderState& state,
      BatchNaming& naming,
      std::size_t root,
      std::uint64_t value) {
    for (const auto& entry : naming.named_roots) {
      if (entry.first == root) {
        return entry.second == value
                   ? MaybeDecision{}
                   : MaybeDecision{CampaignDecision::
                                       no_campaign_target_carrier_rejected};
      }
    }
    naming.named_roots.emplace_back(root, value);
    state.naming[root] = value;
    state.named[root] = 1U;
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision emit_edge(
      OrderState& state,
      BatchNaming& naming,
      std::size_t left,
      std::size_t right,
      const Key& coface_key,
      const exact::ExactLevel& coface_level) {
    if (!accounting_.charge_key_points(
            state.labels[left].key.point_count) ||
        !accounting_.charge_key_points(
            state.labels[right].key.point_count)) {
      return CampaignDecision::no_campaign_budget_exhausted;
    }
    Key observed_union{};
    Key observed_intersection{};
    if (!key_union_and_intersection(
            state.labels[left].key,
            state.labels[right].key,
            observed_union,
            observed_intersection) ||
        observed_union != coface_key ||
        observed_intersection.point_count + 1U != state.order) {
      return CampaignDecision::no_campaign_spanning_tree_rejected;
    }
    state.edges.push_back(CampaignEdge{
        left, right, coface_key, coface_level, observed_intersection});
    if (auto decision = bump_scratch(state, 0U)) {
      return decision;
    }
    unite_components(state, left, right);
    naming.touched_labels.push_back(left);
    naming.touched_labels.push_back(right);
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision append_label(
      OrderState& state,
      BatchNaming& naming,
      std::size_t sorted_insert_position,
      const Key& key,
      const exact::ExactLevel& birth_level) {
    const std::size_t label_index = state.labels.size();
    state.labels.push_back(CampaignLabel{key, birth_level});
    state.sorted_by_key.insert(
        state.sorted_by_key.begin() +
            static_cast<std::ptrdiff_t>(sorted_insert_position),
        label_index);
    state.forest.add();
    state.component_size.push_back(1U);
    state.naming.push_back(0U);
    state.named.push_back(0U);
    naming.touched_labels.push_back(label_index);
    if (auto decision = bump_scratch(state, 0U)) {
      return decision;
    }
    // Geometrically a facet is born no later than any active coface that
    // contains it, so this back-scan should never find a hit; it is retained
    // fail-closed so a late-born label still joins every stored adjacency.
    for (std::size_t coface_index = 0U; coface_index < state.cofaces.size();
         ++coface_index) {
      if (!accounting_.charge_key_points(key.point_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      auto& stored = state.cofaces[coface_index];
      if (!key_is_subset(key, stored.key)) {
        continue;
      }
      naming.touched_labels.push_back(label_index);
      if (!stored.first_label.has_value()) {
        stored.first_label = label_index;
        continue;
      }
      if (auto decision = emit_edge(
              state,
              naming,
              *stored.first_label,
              label_index,
              stored.key,
              stored.level)) {
        return decision;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision process_batch(
      OrderState& state,
      const ScheduleEntry& entry) {
    if (!accounting_.charge(
            result_.counters.source_batch_scan_count,
            budget_.maximum_source_batch_scan_count)) {
      return CampaignDecision::no_campaign_budget_exhausted;
    }
    const auto& batch = plan_.batches[entry.plan_batch_index];
    BatchNaming naming;

    std::size_t birth_end = 0U;
    std::size_t coface_end = 0U;
    if (!checked_add(
            batch.direct_birth_reference_offset,
            batch.direct_birth_reference_count,
            birth_end) ||
        !checked_add(
            batch.coface_reference_offset,
            batch.coface_reference_count,
            coface_end)) {
      return CampaignDecision::no_campaign_capacity_overflow;
    }
    if (birth_end > plan_.direct_birth_references.size() ||
        coface_end > plan_.batch_coface_references.size()) {
      return CampaignDecision::no_campaign_adjacent_order_schedule_rejected;
    }

    for (std::size_t reference = batch.direct_birth_reference_offset;
         reference < birth_end; ++reference) {
      if (!accounting_.charge(
              result_.counters.source_plan_birth_reference_scan_count,
              budget_.maximum_source_plan_birth_reference_scan_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      const auto& birth = plan_.direct_birth_references[reference];
      if (birth.direct_birth_reference_index != reference) {
        return CampaignDecision::no_campaign_adjacent_order_schedule_rejected;
      }
      const Key key = key_from_sparse_facet_key(birth.birth_facet_key);
      if (!accounting_.charge_key_points(key.point_count) ||
          !key_shape_valid(key, plan_.point_count, state.order)) {
        return budget_or(
            CampaignDecision::no_campaign_component_membership_rejected);
      }
      std::size_t position = 0U;
      bool found = false;
      if (!sorted_position(state, key, position, found)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      if (found) {
        // DECISION 1: duplicate birth records for one canonical key must
        // agree on the exact birth level.
        const std::size_t existing = state.sorted_by_key[position];
        if (!accounting_.level_equal(
                state.labels[existing].birth_level, batch.squared_level) ||
            accounting_.relation() != 0) {
          return budget_or(
              CampaignDecision::no_campaign_component_membership_rejected);
        }
        naming.touched_labels.push_back(existing);
        continue;
      }
      if (auto decision = append_label(
              state, naming, position, key, batch.squared_level)) {
        return decision;
      }
    }

    for (std::size_t reference = batch.coface_reference_offset;
         reference < coface_end; ++reference) {
      if (!accounting_.charge(
              result_.counters.source_plan_coface_scan_count,
              budget_.maximum_source_plan_coface_scan_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      const auto& coface_reference = plan_.batch_coface_references[reference];
      if (coface_reference.batch_coface_reference_index != reference ||
          coface_reference.source_coface_index >= plan_.cofaces.size()) {
        return CampaignDecision::no_campaign_adjacent_order_schedule_rejected;
      }
      const auto& coface =
          plan_.cofaces[coface_reference.source_coface_index];
      if (coface.coface_index != coface_reference.source_coface_index ||
          coface.order != state.order ||
          !accounting_.level_equal(
              coface.squared_level, batch.squared_level) ||
          accounting_.relation() != 0) {
        return budget_or(
            CampaignDecision::no_campaign_adjacent_order_schedule_rejected);
      }
      if (!accounting_.charge(
              result_.counters.coface_reconstruction_count,
              budget_.maximum_coface_reconstruction_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      const Key coface_key = key_from_reconstructed_coface(
          reconstruct_exact_direct_normalized_h0_source_coface(
              gateway_,
              gateway_identity_,
              plan_,
              coface_reference.source_coface_index));
      if (!accounting_.charge_key_points(coface_key.point_count) ||
          !key_shape_valid(
              coface_key, plan_.point_count, state.order + 1U)) {
        return budget_or(
            CampaignDecision::no_campaign_adjacent_order_schedule_rejected);
      }
      CampaignCoface stored{coface_key, batch.squared_level, std::nullopt};
      for (std::size_t skip = 0U; skip < coface_key.point_count; ++skip) {
        const Key deletion = key_deletion(coface_key, skip);
        if (!accounting_.charge_key_points(deletion.point_count)) {
          return CampaignDecision::no_campaign_budget_exhausted;
        }
        std::size_t position = 0U;
        bool found = false;
        if (!sorted_position(state, deletion, position, found)) {
          return CampaignDecision::no_campaign_budget_exhausted;
        }
        if (!found) {
          continue;
        }
        const std::size_t label = state.sorted_by_key[position];
        naming.touched_labels.push_back(label);
        if (!stored.first_label.has_value()) {
          stored.first_label = label;
          continue;
        }
        if (auto decision = emit_edge(
                state,
                naming,
                *stored.first_label,
                label,
                coface_key,
                batch.squared_level)) {
          return decision;
        }
      }
      state.cofaces.push_back(stored);
      if (auto decision = bump_scratch(state, 0U)) {
        return decision;
      }
    }

    if (entry.kind == CutKind::k2) {
      if (auto decision = consume_k2_record(state, naming, entry, batch)) {
        return decision;
      }
    } else if (entry.kind == CutKind::higher) {
      if (auto decision = consume_higher_record(state, naming, entry)) {
        return decision;
      }
    } else {
      if (auto decision = consume_terminal_record(state, naming, entry,
                                                  batch)) {
        return decision;
      }
    }

    // Batch closure: every nontrivial component touched at this closed cut
    // must carry one sealed adjacent-order target naming.  A newborn
    // singleton with no hyperedge is a latent carrier: the bridge emits no
    // group image for it, it never enters a square (singleton skips are
    // counted and justified), and its target naming legitimately starts with
    // its first hyperedge group.
    for (const std::size_t label : naming.touched_labels) {
      const std::size_t root = state.forest.find(label);
      if (state.component_size[root] <= 1U) {
        continue;
      }
      if (state.named[root] == 0U) {
        result_.rejected_source_component_handle =
            static_cast<std::uint64_t>(root) + 1U;
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision consume_k2_record(
      OrderState& state,
      BatchNaming& naming,
      const ScheduleEntry& entry,
      const ExactDirectNormalizedH0SourceBatch& batch) {
    const auto& record =
        bridge_.committed_k2_batches()[entry.committed_index];
    std::size_t birth_end = 0U;
    if (!checked_add(
            batch.direct_birth_reference_offset,
            batch.direct_birth_reference_count,
            birth_end)) {
      return CampaignDecision::no_campaign_capacity_overflow;
    }
    for (const auto& binding : record.direct_birth_k1_bindings) {
      if (!accounting_.charge(
              result_.counters.group_record_scan_count,
              budget_.maximum_group_record_scan_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      if (!binding.certified_conditional_birth_k1_binding()) {
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
      std::optional<std::size_t> matched_reference;
      for (std::size_t reference = batch.direct_birth_reference_offset;
           reference < birth_end; ++reference) {
        if (!accounting_.charge_key_lookup_comparison()) {
          return CampaignDecision::no_campaign_budget_exhausted;
        }
        const auto& birth = plan_.direct_birth_references[reference];
        if (birth.source_role_record_index ==
                binding.source_role_record_index &&
            birth.source_event_projection_index ==
                binding.source_event_projection_index) {
          if (matched_reference.has_value()) {
            return CampaignDecision::no_campaign_target_carrier_rejected;
          }
          matched_reference = reference;
        }
      }
      if (!matched_reference.has_value()) {
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
      const Key key = key_from_sparse_facet_key(
          plan_.direct_birth_references[*matched_reference].birth_facet_key);
      std::size_t position = 0U;
      bool found = false;
      if (!sorted_position(state, key, position, found)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      if (!found) {
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
      const std::size_t label = state.sorted_by_key[position];
      naming.touched_labels.push_back(label);
      ++consumed_k2_birth_binding_count_;
      const std::size_t root = state.forest.find(label);
      if (state.component_size[root] == 1U) {
        // DECISION 3: K1 node ids pass through unchanged.
        if (auto decision = name_component_root(
                state, naming, root, binding.closed_k1_root_node_id)) {
          return decision;
        }
      }
    }
    for (const auto& image : record.group_images) {
      if (!accounting_.charge(
              result_.counters.group_record_scan_count,
              budget_.maximum_group_record_scan_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      if (!image.certified_conditional_group_image()) {
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
      std::optional<std::size_t> matched_root;
      for (std::size_t label = 0U; label < state.labels.size(); ++label) {
        if (!accounting_.charge_key_points(
                state.labels[label].key.point_count)) {
          return CampaignDecision::no_campaign_budget_exhausted;
        }
        if (!key_contains_point(
                state.labels[label].key,
                image.canonical_representative_point_id)) {
          continue;
        }
        const std::size_t root = state.forest.find(label);
        if (matched_root.has_value() && *matched_root != root) {
          return CampaignDecision::no_campaign_target_carrier_rejected;
        }
        matched_root = root;
        naming.touched_labels.push_back(label);
      }
      if (!matched_root.has_value()) {
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
      ++consumed_group_image_count_;
      if (auto decision = name_component_root(
              state, naming, *matched_root, image.closed_k1_root_node_id)) {
        return decision;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision consume_higher_record(
      OrderState& state,
      BatchNaming& naming,
      const ScheduleEntry& entry) {
    const auto& record =
        bridge_.committed_higher_batches()[entry.committed_index];
    for (const auto& image : record.group_images) {
      if (!accounting_.charge(
              result_.counters.group_record_scan_count,
              budget_.maximum_group_record_scan_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      if (!image.certified_conditional_group_image()) {
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
      const Key canonical_facet =
          key_from_sparse_facet_key(image.canonical_target_facet_key);
      if (!accounting_.charge_key_points(canonical_facet.point_count) ||
          !key_shape_valid(
              canonical_facet, plan_.point_count, state.order - 1U)) {
        return budget_or(
            CampaignDecision::no_campaign_target_carrier_rejected);
      }
      std::optional<std::size_t> matched_root;
      for (std::size_t label = 0U; label < state.labels.size(); ++label) {
        if (!accounting_.charge_key_points(
                state.labels[label].key.point_count)) {
          return CampaignDecision::no_campaign_budget_exhausted;
        }
        if (!key_is_subset(canonical_facet, state.labels[label].key)) {
          continue;
        }
        const std::size_t root = state.forest.find(label);
        if (matched_root.has_value() && *matched_root != root) {
          return CampaignDecision::no_campaign_target_carrier_rejected;
        }
        matched_root = root;
        naming.touched_labels.push_back(label);
      }
      if (!matched_root.has_value()) {
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
      ++consumed_group_image_count_;
      if (auto decision = name_component_root(
              state, naming, *matched_root, image.resolved_target_root_id)) {
        return decision;
      }
    }
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision consume_terminal_record(
      OrderState& state,
      BatchNaming& naming,
      const ScheduleEntry& entry,
      const ExactDirectNormalizedH0SourceBatch& batch) {
    const auto& record =
        bridge_.committed_higher_batches()[entry.committed_index];
    const auto& image = *record.terminal_component_image;
    if (!accounting_.charge(
            result_.counters.group_record_scan_count,
            budget_.maximum_group_record_scan_count)) {
      return CampaignDecision::no_campaign_budget_exhausted;
    }
    if (!image.certified_conditional_terminal_component_image() ||
        image.source_order != state.order ||
        image.target_order + 1U != image.source_order ||
        image.source_order != plan_.point_count ||
        !accounting_.level_equal(image.squared_level, batch.squared_level) ||
        accounting_.relation() != 0) {
      return budget_or(
          CampaignDecision::no_campaign_adjacent_order_schedule_rejected);
    }
    const Key key = key_from_sparse_facet_key(image.complete_source_facet_key);
    if (!accounting_.charge_key_points(key.point_count) ||
        !key_shape_valid(key, plan_.point_count, state.order)) {
      return budget_or(
          CampaignDecision::no_campaign_target_carrier_rejected);
    }
    std::size_t position = 0U;
    bool found = false;
    if (!sorted_position(state, key, position, found)) {
      return CampaignDecision::no_campaign_budget_exhausted;
    }
    if (!found) {
      return CampaignDecision::no_campaign_target_carrier_rejected;
    }
    const std::size_t label = state.sorted_by_key[position];
    const std::size_t root = state.forest.find(label);
    // K = n owns exactly the single complete facet V, so the terminal
    // component is a singleton: its vertical arrow is certified by the
    // sealed terminal deletion probes and no square receipt is built.
    if (state.component_size[root] != 1U) {
      return CampaignDecision::no_campaign_target_carrier_rejected;
    }
    naming.touched_labels.push_back(label);
    if (auto decision = name_component_root(
            state, naming, root, image.resolved_target_root_id)) {
      return decision;
    }
    ++consumed_terminal_image_count_;
    ++result_.counters.terminal_component_square_count;
    return std::nullopt;
  }

  [[nodiscard]] MaybeDecision snapshot_components(
      OrderState& state,
      std::vector<ComponentSnapshot>& snapshots) {
    std::vector<std::optional<std::size_t>> root_to_snapshot(
        state.labels.size());
    for (std::size_t label = 0U; label < state.labels.size(); ++label) {
      const std::size_t root = state.forest.find(label);
      if (!root_to_snapshot[root].has_value()) {
        if (!accounting_.charge(
                result_.counters.component_scan_count,
                budget_.maximum_component_scan_count)) {
          return CampaignDecision::no_campaign_budget_exhausted;
        }
        // A latent singleton carrier may still be unnamed here; it is
        // excluded from squares and counted as a justified singleton skip.
        if (state.named[root] == 0U && state.component_size[root] > 1U) {
          result_.rejected_source_component_handle =
              static_cast<std::uint64_t>(root) + 1U;
          return CampaignDecision::no_campaign_target_carrier_rejected;
        }
        root_to_snapshot[root] = snapshots.size();
        ComponentSnapshot snapshot{};
        snapshot.handle = static_cast<std::uint64_t>(root) + 1U;
        snapshot.target_root_naming = state.naming[root];
        snapshots.push_back(std::move(snapshot));
      }
      snapshots[*root_to_snapshot[root]].labels_by_key.push_back(label);
    }
    std::vector<std::size_t> label_position(state.labels.size(), 0U);
    for (auto& snapshot : snapshots) {
      std::sort(
          snapshot.labels_by_key.begin(),
          snapshot.labels_by_key.end(),
          [&state](std::size_t left, std::size_t right) {
            return key_less(state.labels[left].key, state.labels[right].key);
          });
      if (snapshot.labels_by_key.size() == 1U) {
        ++result_.counters.singleton_component_skip_count;
      }
      for (std::size_t position = 0U;
           position < snapshot.labels_by_key.size(); ++position) {
        label_position[snapshot.labels_by_key[position]] = position;
      }
    }
    std::vector<LabelForest> local_forests(snapshots.size());
    for (std::size_t snapshot = 0U; snapshot < snapshots.size();
         ++snapshot) {
      for (std::size_t node = 0U;
           node < snapshots[snapshot].labels_by_key.size(); ++node) {
        local_forests[snapshot].add();
      }
    }
    for (std::size_t edge_index = 0U; edge_index < state.edges.size();
         ++edge_index) {
      const auto& edge = state.edges[edge_index];
      const std::size_t root = state.forest.find(edge.left_label);
      const std::size_t snapshot = *root_to_snapshot[root];
      if (local_forests[snapshot].unite(
              label_position[edge.left_label],
              label_position[edge.right_label])) {
        snapshots[snapshot].tree_edge_indices.push_back(edge_index);
      }
    }
    std::size_t snapshot_entry_total = 0U;
    for (const auto& snapshot : snapshots) {
      if (snapshot.tree_edge_indices.size() + 1U !=
          snapshot.labels_by_key.size()) {
        result_.rejected_source_component_handle = snapshot.handle;
        return CampaignDecision::no_campaign_spanning_tree_rejected;
      }
      if (!checked_add(
              snapshot_entry_total,
              snapshot.labels_by_key.size(),
              snapshot_entry_total) ||
          !checked_add(
              snapshot_entry_total,
              snapshot.tree_edge_indices.size(),
              snapshot_entry_total)) {
        return CampaignDecision::no_campaign_capacity_overflow;
      }
    }
    if (auto decision = bump_scratch(state, snapshot_entry_total)) {
      return decision;
    }
    return std::nullopt;
  }

  [[nodiscard]] ExactDirectNormalizedVerticalSourceComponentReceipt
  make_source_receipt(
      const OrderState& state,
      const std::vector<std::size_t>& labels_by_key,
      const std::vector<std::size_t>& tree_edges,
      const exact::ExactLevel& closed_level,
      std::uint64_t handle) const {
    ExactDirectNormalizedVerticalSourceComponentReceipt receipt{};
    receipt.normalized_source_authority_id =
        result_.normalized_source_authority_id;
    receipt.canonical_cloud_digest = result_.canonical_cloud_digest;
    receipt.point_count = plan_.point_count;
    receipt.source_order = state.order;
    receipt.closed_squared_level = closed_level;
    receipt.source_component_handle = handle;
    std::vector<std::size_t> position(state.labels.size(), 0U);
    receipt.labels.reserve(labels_by_key.size());
    for (std::size_t index = 0U; index < labels_by_key.size(); ++index) {
      const std::size_t label = labels_by_key[index];
      position[label] = index;
      receipt.labels.push_back(ExactDirectNormalizedVerticalSourceLabelReceipt{
          index, state.labels[label].key, state.labels[label].birth_level});
    }
    receipt.spanning_tree_edges.reserve(tree_edges.size());
    for (std::size_t index = 0U; index < tree_edges.size(); ++index) {
      const auto& edge = state.edges[tree_edges[index]];
      receipt.spanning_tree_edges.push_back(
          ExactDirectNormalizedVerticalSourceTreeEdgeReceipt{
              index,
              position[edge.left_label],
              position[edge.right_label],
              edge.coface_key,
              edge.coface_level,
              edge.common_facet_key});
    }
    // DECISION 1: membership exhaustiveness is asserted relative to the
    // certified candidate-complete normalized source plan only; the whole
    // campaign result stays conditional on that plan.
    receipt.normalized_source_freshly_verified = true;
    receipt.closed_component_membership_exhaustive = true;
    receipt.source_adjacencies_freshly_verified = true;
    receipt.source_label_and_coface_levels_exact = true;
    return receipt;
  }

  [[nodiscard]] ExactDirectNormalizedVerticalTargetCarrierReceipt
  make_carrier_receipt(
      const OrderState& state,
      const std::vector<std::size_t>& tree_edges,
      const exact::ExactLevel& closed_level,
      std::uint64_t closed_target_root_id,
      const std::optional<Key>& extra_facet) const {
    ExactDirectNormalizedVerticalTargetCarrierReceipt receipt{};
    receipt.target_carrier_authority_id =
        result_.target_carrier_authority_id;
    receipt.canonical_cloud_digest = result_.canonical_cloud_digest;
    receipt.point_count = plan_.point_count;
    receipt.target_order = state.order - 1U;
    receipt.closed_squared_level = closed_level;
    std::vector<Key> facets;
    facets.reserve(tree_edges.size() + 1U);
    for (const std::size_t edge_index : tree_edges) {
      facets.push_back(state.edges[edge_index].common_facet_key);
    }
    if (extra_facet.has_value()) {
      facets.push_back(*extra_facet);
    }
    std::sort(facets.begin(), facets.end(), key_less);
    facets.erase(std::unique(facets.begin(), facets.end()), facets.end());
    receipt.bindings.reserve(facets.size());
    for (std::size_t index = 0U; index < facets.size(); ++index) {
      receipt.bindings.push_back(
          ExactDirectNormalizedVerticalTargetFacetBindingReceipt{
              index, facets[index], closed_target_root_id});
    }
    // The single root per component is the sealed bridge's group image fact:
    // every codimension-one deletion of every member facet resolved to one
    // live adjacent-order root at this closed cut, so every requested common
    // facet binds to that root.  DECISION 4 covers omitted target-only cuts.
    receipt.target_carrier_authority_freshly_verified = true;
    receipt.bindings_complete_for_requested_facets = true;
    receipt.roots_are_closed_at_exact_level = true;
    return receipt;
  }

  [[nodiscard]] MaybeDecision emit_squares(
      OrderState& state,
      const std::vector<ComponentSnapshot>& snapshots,
      const exact::ExactLevel& before_level,
      const exact::ExactLevel& after_level) {
    for (const auto& snapshot : snapshots) {
      if (snapshot.labels_by_key.size() < 2U) {
        continue;
      }
      const std::size_t after_root =
          state.forest.find(snapshot.labels_by_key[0U]);
      if (state.named[after_root] == 0U) {
        result_.rejected_source_component_handle = snapshot.handle;
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
      std::vector<std::size_t> after_labels;
      for (std::size_t label = 0U; label < state.labels.size(); ++label) {
        if (!accounting_.charge_key_lookup_comparison()) {
          return CampaignDecision::no_campaign_budget_exhausted;
        }
        if (state.forest.find(label) == after_root) {
          after_labels.push_back(label);
        }
      }
      std::sort(
          after_labels.begin(),
          after_labels.end(),
          [&state](std::size_t left, std::size_t right) {
            return key_less(state.labels[left].key, state.labels[right].key);
          });
      std::vector<std::size_t> label_position(state.labels.size(), 0U);
      for (std::size_t index = 0U; index < after_labels.size(); ++index) {
        label_position[after_labels[index]] = index;
      }
      LabelForest local_forest;
      for (std::size_t node = 0U; node < after_labels.size(); ++node) {
        local_forest.add();
      }
      std::vector<std::size_t> after_tree;
      for (std::size_t edge_index = 0U; edge_index < state.edges.size();
           ++edge_index) {
        const auto& edge = state.edges[edge_index];
        if (state.forest.find(edge.left_label) != after_root) {
          continue;
        }
        if (local_forest.unite(
                label_position[edge.left_label],
                label_position[edge.right_label])) {
          after_tree.push_back(edge_index);
        }
      }
      if (after_tree.size() + 1U != after_labels.size()) {
        result_.rejected_source_component_handle = snapshot.handle;
        return CampaignDecision::no_campaign_spanning_tree_rejected;
      }
      std::size_t square_scratch = 0U;
      if (!checked_add(
              after_labels.size(), after_tree.size(), square_scratch)) {
        return CampaignDecision::no_campaign_capacity_overflow;
      }
      if (auto decision = bump_scratch(state, square_scratch)) {
        return decision;
      }

      const auto before_source = make_source_receipt(
          state,
          snapshot.labels_by_key,
          snapshot.tree_edge_indices,
          before_level,
          snapshot.handle);
      const auto after_source = make_source_receipt(
          state,
          after_labels,
          after_tree,
          after_level,
          static_cast<std::uint64_t>(after_root) + 1U);
      const auto before_target = make_carrier_receipt(
          state,
          snapshot.tree_edge_indices,
          before_level,
          snapshot.target_root_naming,
          std::nullopt);
      if (before_target.bindings.empty()) {
        result_.rejected_source_component_handle = snapshot.handle;
        return CampaignDecision::no_campaign_target_carrier_rejected;
      }
      // The persistent common target facet is the canonical witness facet of
      // the before component: the smallest common facet over its spanning
      // tree, exactly as build_component_image selects it.
      const Key persistent_facet =
          before_target.bindings.front().target_facet_key;
      const auto after_target = make_carrier_receipt(
          state,
          after_tree,
          after_level,
          state.naming[after_root],
          persistent_facet);

      ExactDirectNormalizedHorizontalCarrierInclusionReceipt horizontal{};
      horizontal.normalized_source_authority_id =
          result_.normalized_source_authority_id;
      horizontal.inclusion_receipt_id = ++next_inclusion_receipt_id_;
      horizontal.from_source_component_handle = snapshot.handle;
      horizontal.to_source_component_handle =
          static_cast<std::uint64_t>(after_root) + 1U;
      horizontal.from_closed_squared_level = before_level;
      horizontal.to_closed_squared_level = after_level;
      horizontal.normalized_horizontal_inclusion_freshly_verified = true;
      horizontal.source_successor_unique = true;
      // DECISION 4: intermediate target-only closed cuts are composed by the
      // sealed bridge facts (terminal monotone union history, terminal root
      // reprobes, contiguous replay-token range); no re-probing here.
      horizontal.all_intermediate_target_levels_accounted = true;

      const auto square = build_exact_direct_normalized_vertical_square_receipt(
          before_source,
          before_target,
          after_source,
          after_target,
          horizontal,
          budget_.per_square_budget);
      if (!square.certified_conditional_vertical_square()) {
        result_.rejected_source_component_handle = snapshot.handle;
        result_.rejected_square_decision = square.decision;
        switch (square.decision) {
          case ExactDirectNormalizedVerticalSquareDecision::
              no_square_capacity_overflow:
            return CampaignDecision::no_campaign_capacity_overflow;
          case ExactDirectNormalizedVerticalSquareDecision::
              no_square_budget_exhausted:
            return CampaignDecision::no_campaign_budget_exhausted;
          case ExactDirectNormalizedVerticalSquareDecision::
              no_square_allocation_failed:
            return CampaignDecision::no_campaign_allocation_failed;
          default:
            return CampaignDecision::no_campaign_square_receipt_rejected;
        }
      }
      const auto verification =
          verify_exact_direct_normalized_vertical_square_receipt(
              before_source,
              before_target,
              after_source,
              after_target,
              horizontal,
              budget_.per_square_budget,
              square);
      // TRAP guarded deliberately: the receipt verifier does not fold
      // trusted_authorities_accepted into result_certified, so both are
      // required explicitly here.
      if (!verification.result_certified ||
          !verification.trusted_authorities_accepted) {
        result_.rejected_source_component_handle = snapshot.handle;
        result_.rejected_square_decision = square.decision;
        return CampaignDecision::no_campaign_square_verification_rejected;
      }

      if (!accounting_.charge(
              result_.counters.replayed_square_count,
              budget_.maximum_replayed_square_count) ||
          !accounting_.charge(
              result_.counters.retained_square_attestation_count,
              budget_.maximum_retained_square_attestation_count) ||
          !accounting_.charge(
              result_.counters.logical_output_entry_count,
              budget_.maximum_logical_output_entry_count)) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }
      result_.counters.peak_component_label_count = std::max(
          {result_.counters.peak_component_label_count,
           before_source.labels.size(),
           after_source.labels.size()});
      result_.counters.peak_component_tree_edge_count = std::max(
          {result_.counters.peak_component_tree_edge_count,
           before_source.spanning_tree_edges.size(),
           after_source.spanning_tree_edges.size()});
      result_.counters.peak_target_binding_count = std::max(
          {result_.counters.peak_target_binding_count,
           before_target.bindings.size(),
           after_target.bindings.size()});
      if (before_target.bindings.size() >
              budget_.maximum_target_binding_count ||
          after_target.bindings.size() >
              budget_.maximum_target_binding_count ||
          before_source.labels.size() >
              budget_.maximum_component_label_count ||
          after_source.labels.size() >
              budget_.maximum_component_label_count ||
          before_source.spanning_tree_edges.size() >
              budget_.maximum_component_tree_edge_count ||
          after_source.spanning_tree_edges.size() >
              budget_.maximum_component_tree_edge_count) {
        return CampaignDecision::no_campaign_budget_exhausted;
      }

      Attestation attestation{};
      attestation.square_attestation_index =
          result_.square_attestations.size();
      attestation.source_order = state.order;
      attestation.target_order = state.order - 1U;
      attestation.from_closed_squared_level = before_level;
      attestation.to_closed_squared_level = after_level;
      attestation.before_source_component_handle = snapshot.handle;
      attestation.after_source_component_handle =
          static_cast<std::uint64_t>(after_root) + 1U;
      attestation.horizontal_inclusion_receipt_id =
          horizontal.inclusion_receipt_id;
      attestation.before_component_label_count =
          before_source.labels.size();
      attestation.after_component_label_count = after_source.labels.size();
      attestation.before_spanning_tree_edge_count =
          before_source.spanning_tree_edges.size();
      attestation.after_spanning_tree_edge_count =
          after_source.spanning_tree_edges.size();
      attestation.before_target_binding_count =
          before_target.bindings.size();
      attestation.after_target_binding_count = after_target.bindings.size();
      attestation.persistent_common_target_facet_key =
          square.squares.front().persistent_common_target_facet_key;
      attestation.from_closed_target_root_id =
          square.squares.front().from_closed_target_root_id;
      attestation.propagated_closed_target_root_id =
          square.squares.front().propagated_closed_target_root_id;
      attestation.to_closed_target_root_id =
          square.squares.front().to_closed_target_root_id;
      attestation.square_decision = square.decision;
      attestation.square_receipt_freshly_built = true;
      attestation.square_receipt_freshly_verified = true;
      attestation.square_positively_certified = true;
      result_.square_attestations.push_back(attestation);
      // The five receipts and the local spanning-tree scratch of this square
      // are released here; only the compact attestation is retained.
      ++result_.counters.released_transient_arena_count;
    }
    return std::nullopt;
  }

  // Exhaustive-closure identity enforced against the final seal (stated per
  // the maintainer instruction): squares are per (component, consecutive
  // closed-cut pair) while the seal partitions events into groups, so a
  // literal square-to-group equality is not derivable.  The exact cross-check
  // enforced instead is: (a) every sealed K2 and higher group image was
  // consumed by exactly one derived component naming event, so the consumed
  // total equals sealed_k2_group_count + sealed_higher_group_count; (b) every
  // sealed K2 direct-birth K1 binding was consumed exactly once; (c) the
  // terminal image population was consumed exactly once; and (d) every
  // scanned component per consecutive cut pair was either replayed as one
  // square or skipped as a justified singleton.
  [[nodiscard]] MaybeDecision closure() {
    std::size_t sealed_group_total = 0U;
    std::size_t scanned_total = 0U;
    if (!checked_add(
            seal_.sealed_k2_group_count,
            seal_.sealed_higher_group_count,
            sealed_group_total) ||
        !checked_add(
            result_.counters.replayed_square_count,
            result_.counters.singleton_component_skip_count,
            scanned_total)) {
      return CampaignDecision::no_campaign_capacity_overflow;
    }
    if (consumed_group_image_count_ != sealed_group_total ||
        consumed_terminal_image_count_ !=
            seal_.sealed_terminal_component_image_count ||
        result_.counters.terminal_component_square_count !=
            consumed_terminal_image_count_ ||
        consumed_k2_birth_binding_count_ !=
            seal_.sealed_stamp.committed_k2_direct_birth_k1_binding_count ||
        scanned_total != result_.counters.component_scan_count ||
        result_.square_attestations.size() !=
            result_.counters.replayed_square_count) {
      return CampaignDecision::no_campaign_square_coverage_incomplete;
    }
    return std::nullopt;
  }

  const ExactDirectMorseResidentAllOrdersVerticalBridge& bridge_;
  const ExactDirectMorseResidentAllOrdersVerticalFinalSeal& seal_;
  const ExactDirectNormalizedH0SourcePlanResult& plan_;
  const ExactDirectSparseGatewayCandidateJournalResult& gateway_;
  const ExactDirectSparseGatewayCandidateScientificIdentityResult&
      gateway_identity_;
  const ExactDirectNormalizedH0IncidenceReductionAuthority& authority_;
  const CampaignBudget& budget_;
  CampaignResult& result_;
  Accounting accounting_;
  std::vector<ScheduleEntry> schedule_;
  std::array<std::vector<std::size_t>, 11U> order_entries_{};
  std::size_t consumed_group_image_count_{};
  std::size_t consumed_k2_birth_binding_count_{};
  std::size_t consumed_terminal_image_count_{};
  std::uint64_t next_inclusion_receipt_id_{};
};

[[nodiscard]] bool campaign_counters_within_budget(
    const CampaignCounters& counters,
    const CampaignBudget& budget) noexcept {
  return counters.adjacent_order_pair_count <=
             budget.maximum_adjacent_order_pair_count &&
         counters.consecutive_closed_cut_pair_count <=
             budget.maximum_consecutive_closed_cut_pair_count &&
         counters.replayed_square_count <=
             budget.maximum_replayed_square_count &&
         counters.retained_square_attestation_count <=
             budget.maximum_retained_square_attestation_count &&
         counters.component_scan_count <=
             budget.maximum_component_scan_count &&
         counters.peak_component_label_count <=
             budget.maximum_component_label_count &&
         counters.peak_component_tree_edge_count <=
             budget.maximum_component_tree_edge_count &&
         counters.peak_target_binding_count <=
             budget.maximum_target_binding_count &&
         counters.source_batch_scan_count <=
             budget.maximum_source_batch_scan_count &&
         counters.group_record_scan_count <=
             budget.maximum_group_record_scan_count &&
         counters.component_state_scan_count <=
             budget.maximum_component_state_scan_count &&
         counters.root_coverage_point_scan_count <=
             budget.maximum_root_coverage_point_scan_count &&
         counters.root_witness_scan_count <=
             budget.maximum_root_witness_scan_count &&
         counters.source_plan_birth_reference_scan_count <=
             budget.maximum_source_plan_birth_reference_scan_count &&
         counters.source_plan_coface_scan_count <=
             budget.maximum_source_plan_coface_scan_count &&
         counters.coface_reconstruction_count <=
             budget.maximum_coface_reconstruction_count &&
         counters.key_point_scan_count <=
             budget.maximum_key_point_scan_count &&
         counters.key_lookup_comparison_count <=
             budget.maximum_key_lookup_comparison_count &&
         counters.exact_level_comparison_count <=
             budget.maximum_exact_level_comparison_count &&
         counters.maximum_observed_exact_level_integer_bit_count <=
             budget.maximum_single_exact_level_integer_bit_count &&
         counters.peak_transient_scratch_entry_count <=
             budget.maximum_transient_scratch_entry_count &&
         counters.logical_output_entry_count <=
             budget.maximum_logical_output_entry_count;
}

}  // namespace

bool ExactDirectMorseResidentNaturalitySquareAttestation::
    certified_conditional_square_attestation() const noexcept {
  return square_receipt_freshly_built && square_receipt_freshly_verified &&
         square_positively_certified &&
         square_decision ==
             ExactDirectNormalizedVerticalSquareDecision::
                 complete_conditional_normalized_common_facet_vertical_square &&
         source_order >= 2U && target_order + 1U == source_order &&
         horizontal_inclusion_receipt_id != 0U &&
         before_source_component_handle != 0U &&
         after_source_component_handle != 0U &&
         before_component_label_count >= 2U &&
         after_component_label_count >= before_component_label_count &&
         before_spanning_tree_edge_count + 1U ==
             before_component_label_count &&
         after_spanning_tree_edge_count + 1U == after_component_label_count &&
         before_target_binding_count != 0U &&
         after_target_binding_count != 0U &&
         persistent_common_target_facet_key.point_count == target_order;
}

bool ExactDirectMorseResidentNaturalitySquareCampaignResult::
    certified_conditional_all_naturality_squares_replayed() const noexcept {
  if (schema_version !=
          direct_morse_resident_naturality_square_campaign_schema_version ||
      decision !=
          ExactDirectMorseResidentNaturalitySquareCampaignDecision::
              complete_conditional_all_naturality_squares_replayed ||
      scope != ExactDirectMorseResidentNaturalitySquareCampaignScope::
                   conditional_on_sealed_bridge_and_normalized_source) {
    return false;
  }
  if (!sealed_bridge_live_verified ||
      !final_seal_cross_verified_against_live_bridge ||
      !normalized_v7_source_capability_live_required ||
      !normalized_source_surface_freshly_verified ||
      !canonical_point_namespace_identity_certified ||
      !exact_product_order_adjacent_schedule_reconstructed ||
      !every_component_membership_derived_incrementally_and_released ||
      !every_spanning_tree_derived_from_certified_source_adjacencies ||
      !every_target_carrier_binding_derived_from_sealed_bridge_images ||
      !every_horizontal_inclusion_bound_to_consecutive_closed_cuts ||
      !every_square_receipt_freshly_built ||
      !every_square_receipt_freshly_verified ||
      !every_singleton_component_skip_justified ||
      !all_naturality_squares_replayed ||
      no_partial_scientific_payload_published_on_failure ||
      !unsupported_campaign_claims_false(*this)) {
    return false;
  }
  // The terminal K = n singleton square exists exactly when the seal
  // committed one terminal component image.
  if (counters.terminal_component_square_count > 1U ||
      terminal_component_square_replayed !=
          (counters.terminal_component_square_count != 0U)) {
    return false;
  }
  if (normalized_source_authority_id == 0U ||
      target_carrier_authority_id == 0U ||
      target_carrier_authority_id != normalized_source_authority_id ||
      canonical_cloud_digest == contract::CanonicalId{} ||
      sealed_bridge_stamp.resident_session_authority_id !=
          normalized_source_authority_id) {
    return false;
  }
  if (square_attestations.size() != counters.replayed_square_count ||
      counters.retained_square_attestation_count !=
          counters.replayed_square_count ||
      counters.logical_output_entry_count !=
          counters.replayed_square_count) {
    return false;
  }
  std::size_t scanned_total = 0U;
  if (!checked_add(
          counters.replayed_square_count,
          counters.singleton_component_skip_count,
          scanned_total) ||
      scanned_total != counters.component_scan_count) {
    return false;
  }
  for (std::size_t index = 0U; index < square_attestations.size(); ++index) {
    const auto& attestation = square_attestations[index];
    if (attestation.square_attestation_index != index ||
        !attestation.certified_conditional_square_attestation()) {
      return false;
    }
  }
  return campaign_counters_within_budget(counters, requested_budget);
}

bool ExactDirectMorseResidentNaturalitySquareCampaignResult::
    certified_atomic_failure() const noexcept {
  const bool failure_decision =
      decision != ExactDirectMorseResidentNaturalitySquareCampaignDecision::
                      not_certified &&
      decision != ExactDirectMorseResidentNaturalitySquareCampaignDecision::
                      complete_conditional_all_naturality_squares_replayed;
  return schema_version ==
             direct_morse_resident_naturality_square_campaign_schema_version &&
         failure_decision && square_attestations.empty() &&
         counters ==
             ExactDirectMorseResidentNaturalitySquareCampaignCounters{} &&
         !sealed_bridge_live_verified &&
         !final_seal_cross_verified_against_live_bridge &&
         !normalized_v7_source_capability_live_required &&
         !normalized_source_surface_freshly_verified &&
         !canonical_point_namespace_identity_certified &&
         !exact_product_order_adjacent_schedule_reconstructed &&
         !every_component_membership_derived_incrementally_and_released &&
         !every_spanning_tree_derived_from_certified_source_adjacencies &&
         !every_target_carrier_binding_derived_from_sealed_bridge_images &&
         !every_horizontal_inclusion_bound_to_consecutive_closed_cuts &&
         !every_square_receipt_freshly_built &&
         !every_square_receipt_freshly_verified &&
         !every_singleton_component_skip_justified &&
         !terminal_component_square_replayed &&
         !all_naturality_squares_replayed &&
         no_partial_scientific_payload_published_on_failure &&
         unsupported_campaign_claims_false(*this) &&
         scope == ExactDirectMorseResidentNaturalitySquareCampaignScope::
                      unspecified;
}

bool ExactDirectMorseResidentNaturalitySquareCampaignResult::
    certified_outcome() const noexcept {
  return certified_conditional_all_naturality_squares_replayed() ||
         certified_atomic_failure();
}

ExactDirectMorseResidentNaturalitySquareCampaignResult
build_exact_direct_morse_resident_naturality_square_campaign(
    const ExactDirectMorseResidentAllOrdersVerticalBridge& sealed_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&
        trusted_final_seal,
    const ExactDirectNormalizedH0SourcePlanResult&
        trusted_normalized_source_plan,
    const ExactDirectSparseGatewayCandidateJournalResult&
        trusted_source_gateway,
    const ExactDirectSparseGatewayCandidateScientificIdentityResult&
        trusted_source_gateway_identity,
    const ExactDirectNormalizedH0IncidenceReductionAuthority&
        trusted_incidence_reduction_authority,
    const ExactDirectMorseResidentNaturalitySquareCampaignBudget& budget) {
  CampaignResult result{};
  result.requested_budget = budget;
  try {
    result.point_count = trusted_normalized_source_plan.point_count;
    result.effective_maximum_order =
        trusted_incidence_reduction_authority.effective_maximum_order;
    result.sealed_bridge_stamp = trusted_final_seal.sealed_stamp;
    result.normalized_source_authority_id =
        trusted_final_seal.sealed_stamp.resident_session_authority_id;
    result.target_carrier_authority_id =
        trusted_final_seal.terminal_locator_stamp.external_authority_id;
    result.canonical_cloud_digest =
        trusted_final_seal.sealed_stamp.canonical_cloud_digest;

    CampaignEngine engine(
        sealed_bridge,
        trusted_final_seal,
        trusted_normalized_source_plan,
        trusted_source_gateway,
        trusted_source_gateway_identity,
        trusted_incidence_reduction_authority,
        budget,
        result);
    const CampaignDecision decision = engine.run();
    if (decision !=
        CampaignDecision::
            complete_conditional_all_naturality_squares_replayed) {
      return fail_campaign(std::move(result), decision);
    }

    result.sealed_bridge_live_verified = true;
    result.final_seal_cross_verified_against_live_bridge = true;
    result.normalized_v7_source_capability_live_required = true;
    result.normalized_source_surface_freshly_verified = true;
    result.canonical_point_namespace_identity_certified = true;
    result.exact_product_order_adjacent_schedule_reconstructed = true;
    result.every_component_membership_derived_incrementally_and_released =
        true;
    result.every_spanning_tree_derived_from_certified_source_adjacencies =
        true;
    result.every_target_carrier_binding_derived_from_sealed_bridge_images =
        true;
    result.every_horizontal_inclusion_bound_to_consecutive_closed_cuts =
        true;
    result.every_square_receipt_freshly_built = true;
    result.every_square_receipt_freshly_verified = true;
    result.every_singleton_component_skip_justified = true;
    result.terminal_component_square_replayed =
        result.counters.terminal_component_square_count != 0U;
    result.all_naturality_squares_replayed = true;
    result.no_partial_scientific_payload_published_on_failure = false;
    result.decision = CampaignDecision::
        complete_conditional_all_naturality_squares_replayed;
    result.scope = CampaignScope::
        conditional_on_sealed_bridge_and_normalized_source;
    if (!result.certified_conditional_all_naturality_squares_replayed()) {
      return fail_campaign(
          std::move(result),
          CampaignDecision::no_campaign_square_coverage_incomplete);
    }
    return result;
  } catch (const std::bad_alloc&) {
    return fail_campaign(
        std::move(result),
        CampaignDecision::no_campaign_allocation_failed);
  } catch (const std::exception&) {
    // reconstruct_exact_direct_normalized_h0_source_coface throws on a
    // gateway/plan association mismatch; contain it fail-closed as a
    // rejected normalized source surface.
    return fail_campaign(
        std::move(result),
        CampaignDecision::no_campaign_normalized_source_surface_rejected);
  }
}

ExactDirectMorseResidentNaturalitySquareCampaignVerification
verify_exact_direct_morse_resident_naturality_square_campaign(
    const ExactDirectMorseResidentAllOrdersVerticalBridge& sealed_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&
        trusted_final_seal,
    const ExactDirectNormalizedH0SourcePlanResult&
        trusted_normalized_source_plan,
    const ExactDirectSparseGatewayCandidateJournalResult&
        trusted_source_gateway,
    const ExactDirectSparseGatewayCandidateScientificIdentityResult&
        trusted_source_gateway_identity,
    const ExactDirectNormalizedH0IncidenceReductionAuthority&
        trusted_incidence_reduction_authority,
    const ExactDirectMorseResidentNaturalitySquareCampaignBudget&
        trusted_budget,
    const ExactDirectMorseResidentNaturalitySquareCampaignResult& observed) {
  ExactDirectMorseResidentNaturalitySquareCampaignVerification verification{};
  verification.sealed_bridge_accepted =
      sealed_bridge.final_vertical_sealed() &&
      sealed_bridge.verify_final_vertical_seal(trusted_final_seal) &&
      sealed_bridge.verify_stamp(trusted_final_seal.sealed_stamp);
  verification.trusted_normalized_source_surface_accepted =
      trusted_normalized_source_plan
          .certified_complete_candidate_source_plan() &&
      trusted_incidence_reduction_authority
          .certified_horizontal_incidence_reduction();
  verification.observed_storage_within_budget =
      observed.square_attestations.size() <=
          trusted_budget.maximum_retained_square_attestation_count &&
      campaign_counters_within_budget(observed.counters, trusted_budget);
  const CampaignResult expected =
      build_exact_direct_morse_resident_naturality_square_campaign(
          sealed_bridge,
          trusted_final_seal,
          trusted_normalized_source_plan,
          trusted_source_gateway,
          trusted_source_gateway_identity,
          trusted_incidence_reduction_authority,
          trusted_budget);
  verification.expected_result_freshly_reconstructed = true;
  verification.observed_recursively_equal = observed == expected;
  verification.unsupported_global_claims_remain_false =
      unsupported_campaign_claims_false(observed);
  verification.result_certified =
      verification.observed_storage_within_budget &&
      verification.expected_result_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      verification.unsupported_global_claims_remain_false &&
      observed.certified_outcome();
  return verification;
}

ExactDirectMorseVerticalConfig
ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding::
    vertical_journal_config() const noexcept {
  ExactDirectMorseVerticalConfig config{};
  config.external_target_authority_id = external_target_authority_id;
  return config;
}

bool ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding::
    certified_named_external_target_authority_binding() const noexcept {
  return schema_version ==
             direct_morse_resident_vertical_external_target_authority_binding_schema_version &&
         sealed_bridge_live_verified &&
         final_seal_cross_verified_against_live_bridge &&
         authority_id_nonzero &&
         authority_id_matches_terminal_locator_stamp &&
         replay_token_range_contiguous &&
         external_target_authority_named_only &&
         !external_target_authority_replayed &&
         !all_naturality_squares_replayed && !public_status_claimed &&
         external_target_authority_id != 0U &&
         external_target_authority_id ==
             terminal_locator_stamp.external_authority_id &&
         external_target_authority_id == resident_session_authority_id &&
         decision ==
             ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingDecision::
                 complete_named_external_target_authority_binding;
}

ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding
build_exact_direct_morse_resident_vertical_external_target_authority_binding(
    const ExactDirectMorseResidentAllOrdersVerticalBridge& sealed_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&
        trusted_final_seal) noexcept {
  using BindingDecision =
      ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingDecision;
  ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding binding{};
  binding.external_target_authority_id =
      trusted_final_seal.terminal_locator_stamp.external_authority_id;
  binding.resident_session_authority_id =
      trusted_final_seal.sealed_stamp.resident_session_authority_id;
  binding.resident_locator_instance_id =
      trusted_final_seal.sealed_stamp.resident_locator_instance_id;
  binding.persistent_source_root_witness_count =
      trusted_final_seal.sealed_persistent_source_root_witness_count;
  binding.final_root_witness_query_replay_token_begin =
      trusted_final_seal.final_root_witness_query_replay_token_begin;
  binding.final_root_witness_query_replay_token_end =
      trusted_final_seal.final_root_witness_query_replay_token_end;
  binding.terminal_locator_stamp = trusted_final_seal.terminal_locator_stamp;
  if (!sealed_bridge.final_vertical_sealed() ||
      sealed_bridge.final_vertical_seal() == nullptr) {
    binding.decision = BindingDecision::no_binding_bridge_not_sealed;
    return binding;
  }
  binding.sealed_bridge_live_verified = true;
  if (!sealed_bridge.verify_final_vertical_seal(trusted_final_seal) ||
      !sealed_bridge.verify_stamp(trusted_final_seal.sealed_stamp) ||
      !trusted_final_seal.certified_final_vertical_seal() ||
      sealed_bridge.source_root_target_witnesses().size() !=
          trusted_final_seal.sealed_persistent_source_root_witness_count) {
    binding.decision = BindingDecision::no_binding_final_seal_mismatch;
    return binding;
  }
  binding.final_seal_cross_verified_against_live_bridge = true;
  if (binding.external_target_authority_id == 0U) {
    binding.decision = BindingDecision::no_binding_authority_id_zero;
    return binding;
  }
  binding.authority_id_nonzero = true;
  if (binding.external_target_authority_id !=
      binding.resident_session_authority_id) {
    binding.decision = BindingDecision::no_binding_authority_id_mismatch;
    return binding;
  }
  binding.authority_id_matches_terminal_locator_stamp = true;
  std::size_t expected_reprobe_total = 0U;
  if (!checked_add(
          trusted_final_seal.sealed_final_root_witness_probe_count,
          trusted_final_seal.sealed_terminal_final_target_reprobe_count,
          expected_reprobe_total)) {
    binding.decision =
        BindingDecision::no_binding_replay_token_range_rejected;
    return binding;
  }
  const bool token_range_contiguous =
      expected_reprobe_total == 0U
          ? binding.final_root_witness_query_replay_token_begin == 0U &&
                binding.final_root_witness_query_replay_token_end == 0U
          : binding.final_root_witness_query_replay_token_begin != 0U &&
                binding.final_root_witness_query_replay_token_end >=
                    binding.final_root_witness_query_replay_token_begin &&
                binding.final_root_witness_query_replay_token_end -
                        binding.final_root_witness_query_replay_token_begin ==
                    expected_reprobe_total - 1U;
  if (!token_range_contiguous) {
    binding.decision =
        BindingDecision::no_binding_replay_token_range_rejected;
    return binding;
  }
  binding.replay_token_range_contiguous = true;
  // DECISION 8: only the NAMING binding is certified here; the replaying
  // authority is a separate follow-up component and the only place that may
  // ever raise external_target_authority_replayed.
  binding.external_target_authority_named_only = true;
  binding.decision =
      BindingDecision::complete_named_external_target_authority_binding;
  return binding;
}

ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingVerification
verify_exact_direct_morse_resident_vertical_external_target_authority_binding(
    const ExactDirectMorseResidentAllOrdersVerticalBridge& sealed_bridge,
    const ExactDirectMorseResidentAllOrdersVerticalFinalSeal&
        trusted_final_seal,
    const ExactDirectMorseResidentVerticalExternalTargetAuthorityBinding&
        observed) noexcept {
  ExactDirectMorseResidentVerticalExternalTargetAuthorityBindingVerification
      verification{};
  verification.sealed_bridge_accepted =
      sealed_bridge.final_vertical_sealed() &&
      sealed_bridge.verify_final_vertical_seal(trusted_final_seal);
  const auto expected =
      build_exact_direct_morse_resident_vertical_external_target_authority_binding(
          sealed_bridge, trusted_final_seal);
  verification.expected_binding_freshly_reconstructed = true;
  verification.observed_recursively_equal = observed == expected;
  verification.replay_never_claimed =
      !observed.external_target_authority_replayed &&
      !observed.all_naturality_squares_replayed &&
      !observed.public_status_claimed;
  verification.result_certified =
      verification.expected_binding_freshly_reconstructed &&
      verification.observed_recursively_equal &&
      verification.replay_never_claimed;
  return verification;
}

}  // namespace morsehgp3d::hierarchy
