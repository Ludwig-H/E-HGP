#include "morsehgp3d/hierarchy/direct_sparse_gateway_clock.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace morsehgp3d::hierarchy;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::LbvhTraversalOrder;
using morsehgp3d::spatial::MortonLbvhIndex;
using morsehgp3d::spatial::PointId;

constexpr std::uint64_t locator_authority_id = UINT64_C(0x10ca70a);
constexpr std::uint64_t clock_authority_id = UINT64_C(0xc10c0001);
constexpr std::uint64_t clock_replay_token = UINT64_C(0xc10c0002);
int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

[[nodiscard]] CertifiedPoint3 point(
    double x,
    double y = 0.0,
    double z = 0.0) {
  return CertifiedPoint3::from_binary64(x, y, z);
}

template <std::size_t Size>
[[nodiscard]] CanonicalPointCloud canonical_cloud(
    const std::array<CertifiedPoint3, Size>& points) {
  return CanonicalPointCloud::rejecting_duplicates(
      std::span<const CertifiedPoint3>{points});
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

[[nodiscard]] ExactDirectSparseFirstIncidenceBudget
generous_first_incidence_budget() {
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

[[nodiscard]] ExactDirectSparseGatewayCandidateBudget
generous_source_budget() {
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
      generous_first_incidence_budget(),
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
    std::size_t maximum_order) {
  const ExactDirectSupportTerminalBudget terminal_budget{
      unlimited_pair_budget(), unlimited_higher_budget()};
  const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  const auto pair = build_exact_pair_support_stream(
      index, cloud, maximum_order, terminal_budget.pair);
  const auto higher = build_exact_higher_support_stream(
      index, cloud, maximum_order, terminal_budget.higher);
  auto facade = build_exact_direct_support_terminal_facade(
      index, cloud, maximum_order, terminal_budget, pair, higher);
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

struct SourceFixture {
  CanonicalPointCloud cloud;
  MortonLbvhIndex index;
  DirectSources sources;
  ExactDirectSparseGatewayCandidateBudget source_budget;
  ExactDirectSparseGatewayCandidateJournalResult source;
};

[[nodiscard]] SourceFixture source_fixture() {
  const std::array<CertifiedPoint3, 4U> points{
      point(-2.0, 0.0),
      point(0.0, -3.0),
      point(0.0, 3.0),
      point(2.0, 0.0),
  };
  CanonicalPointCloud cloud = canonical_cloud(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  DirectSources sources = direct_sources(cloud, 4U);
  const auto source_budget = generous_source_budget();
  auto source = build_exact_direct_sparse_gateway_candidate_journal(
      index,
      cloud,
      sources.facade,
      sources.event_journal,
      sources.arm_budget,
      sources.arm_journal,
      sources.incidence_budget,
      sources.incidence_journal,
      source_budget,
      LbvhTraversalOrder::near_first);
  check(
      source.certified_partial_refinement() && source.batches.size() >= 3U &&
          !source.deletion_projections.empty() &&
          !source.facet_tokens.empty() && !source.gateway_candidates.empty() &&
          !source.batch_facet_token_indices.empty(),
      "the four-point clock fixture exposes all five nonempty arenas and at least three source batches");
  return {
      std::move(cloud),
      std::move(index),
      std::move(sources),
      source_budget,
      std::move(source),
  };
}

[[nodiscard]] SourceFixture empty_source_fixture() {
  const std::array<CertifiedPoint3, 1U> points{point(0.0, 0.0, 0.0)};
  CanonicalPointCloud cloud = canonical_cloud(points);
  MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
  DirectSources sources = direct_sources(cloud, 1U);
  const auto source_budget = generous_source_budget();
  auto source = build_exact_direct_sparse_gateway_candidate_journal(
      index,
      cloud,
      sources.facade,
      sources.event_journal,
      sources.arm_budget,
      sources.arm_journal,
      sources.incidence_budget,
      sources.incidence_journal,
      source_budget,
      LbvhTraversalOrder::near_first);
  check(
      source.certified_partial_refinement() && source.batches.empty(),
      "the one-point source has S=0 certified gateway batches");
  return {
      std::move(cloud),
      std::move(index),
      std::move(sources),
      source_budget,
      std::move(source),
  };
}

[[nodiscard]] ExactDirectSparseGatewayCandidateScientificIdentityBudget
generous_identity_budget() {
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

[[nodiscard]] ExactDirectSparseGatewayClockCertificateDigestBudget
generous_digest_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {maximum, maximum};
}

[[nodiscard]] ExactDirectSparseFacetKey facet_key(
    std::initializer_list<PointId> ids) {
  ExactDirectSparseFacetKey key;
  key.point_count = ids.size();
  std::size_t index = 0U;
  for (const PointId id : ids) {
    key.point_ids[index] = id;
    ++index;
  }
  return key;
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocatorBudget locator_budget() {
  return {
      8U,
      8U,
      80U,
      8U,
      8U,
      8U,
      8U,
      8U,
      80U,
      17U,
      17U,
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetLocator make_locator() {
  auto locator = build_exact_direct_sparse_positive_facet_locator(
      8U, locator_budget(), {locator_authority_id, ~UINT64_C(0)});
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "locator batch zero is an explicit accepted empty commit");
  const std::array<ExactDirectSparseFacetBinding, 1U> bindings{{
      {0U,
       facet_key({0U, 2U}),
       3U,
       {locator_authority_id, UINT64_C(0x501)}},
  }};
  const std::array<ExactDirectSparseComponentUnion, 1U> unions{{
      {0U,
       3U,
       2U,
       {locator_authority_id, UINT64_C(0x500)}},
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              unions,
              bindings)
          .certified_committed_batch(),
      "locator batch one publishes one bounded positive binding");
  check(
      locator.committed_batches().size() == 2U,
      "the clock locator reaches prefix two after an empty and a nonempty commit");
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch() &&
          locator.committed_batches().size() == 3U,
      "the clock locator retains a final empty suffix commit at prefix three");
  return locator;
}

[[nodiscard]]
ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget
exact_structure_budget(
    const ExactDirectSparsePositiveFacetLocator& locator) {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  ExactDirectSparsePositiveFacetLocatorStructuralVerificationBudget budget{
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
  const auto measured =
      verify_exact_direct_sparse_positive_facet_locator_structure(
          8U,
          locator_budget(),
          {locator_authority_id, ~UINT64_C(0)},
          budget,
          locator.state_view());
  check(
      measured.result_certified,
      "the clock locator admits a fresh finite structural accounting pass");
  return {
      measured.required_table_slot_count,
      measured.required_key_point_count,
      measured.required_component_parent_count,
      measured.required_union_record_count,
      measured.required_batch_record_count,
      measured.required_binding_scratch_entry_count,
      measured.required_key_point_scratch_entry_count,
      measured.required_table_slot_scratch_entry_count,
      measured.required_component_parent_scratch_entry_count,
      measured.required_temporary_scratch_byte_count,
      measured.fingerprint_search_slot_visit_count,
      measured.insertion_chronology_slot_visit_count,
      measured.union_parent_hop_count,
  };
}

[[nodiscard]]
ExactDirectSparsePositiveFacetLocatorPrefixStampSweepBudget
generous_prefix_stamp_budget() {
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

[[nodiscard]] ExactDirectSparseGatewayClockVerificationBudget
generous_clock_budget(
    const ExactDirectSparsePositiveFacetLocator& locator) {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      generous_identity_budget(),
      generous_digest_budget(),
      exact_structure_budget(locator),
      generous_prefix_stamp_budget(),
  };
}

[[nodiscard]] ExactDirectSparseGatewayClockCertificate make_certificate(
    const ExactDirectSparseGatewayCandidateJournalResult& source,
    const ExactDirectSparsePositiveFacetLocator& locator,
    std::span<const std::size_t> source_prefixes) {
  check(
      source_prefixes.size() == source.batches.size(),
      "certificate construction receives one prefix per dense source batch");
  const auto identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          source, generous_identity_budget());
  check(identity.certified_identity(), "source identity construction succeeds");

  std::vector<std::pair<std::size_t, std::size_t>> order;
  order.reserve(source_prefixes.size());
  for (std::size_t source_index = 0U;
       source_index < source_prefixes.size();
       ++source_index) {
    order.emplace_back(source_prefixes[source_index], source_index);
  }
  std::sort(order.begin(), order.end());
  std::vector<std::size_t> sorted_prefixes;
  sorted_prefixes.reserve(order.size());
  for (const auto& [prefix, source_index] : order) {
    static_cast<void>(source_index);
    sorted_prefixes.push_back(prefix);
  }
  const auto stamps =
      build_exact_direct_sparse_positive_facet_locator_prefix_stamp_sweep(
          sorted_prefixes, locator, generous_prefix_stamp_budget());
  check(
      stamps.certified_outcome() &&
          stamps.prefix_stamps.size() == source_prefixes.size(),
      "certificate construction uses one monotone PSTAMP sweep");

  ExactDirectSparseGatewayClockCertificate certificate;
  certificate.authority_id = clock_authority_id;
  certificate.replay_token = clock_replay_token;
  certificate.source_scientific_identity_digest =
      identity.scientific_identity_digest;
  certificate.final_locator_stamp = locator.snapshot_stamp();
  certificate.boundaries.resize(source_prefixes.size());
  for (std::size_t sorted_index = 0U;
       sorted_index < order.size();
       ++sorted_index) {
    const auto [prefix, source_index] = order[sorted_index];
    certificate.boundaries[source_index] = {
        source_index, prefix, stamps.prefix_stamps[sorted_index]};
  }
  const auto digest =
      compute_exact_direct_sparse_gateway_clock_certificate_digest(
          certificate, generous_digest_budget());
  check(digest.certified_digest(), "certificate digest construction succeeds");
  certificate.certificate_digest = digest.certificate_digest;
  certificate.digest_present = true;
  return certificate;
}

[[nodiscard]] ExactDirectSparseGatewayExternalClockAnchor anchor_for(
    const ExactDirectSparseGatewayClockCertificate& certificate) {
  return {
      certificate.authority_id,
      certificate.replay_token,
      certificate.certificate_digest,
  };
}

[[nodiscard]] ExactDirectSparseGatewayClockVerification verify_clock(
    const SourceFixture& fixture,
    const ExactDirectSparsePositiveFacetLocator& locator,
    const ExactDirectSparseGatewayExternalClockAnchor& anchor,
    const ExactDirectSparseGatewayClockCertificate& certificate,
    const ExactDirectSparseGatewayClockVerificationBudget& budget) {
  return verify_exact_direct_sparse_gateway_clock_certificate(
      fixture.index,
      fixture.cloud,
      fixture.sources.facade,
      fixture.sources.event_journal,
      fixture.sources.arm_budget,
      fixture.sources.arm_journal,
      fixture.sources.incidence_budget,
      fixture.sources.incidence_journal,
      fixture.source_budget,
      LbvhTraversalOrder::near_first,
      fixture.source,
      8U,
      locator_budget(),
      {locator_authority_id, ~UINT64_C(0)},
      locator,
      anchor,
      certificate,
      budget);
}

void rehash_certificate(
    ExactDirectSparseGatewayClockCertificate& certificate) {
  const auto digest =
      compute_exact_direct_sparse_gateway_clock_certificate_digest(
          certificate, generous_digest_budget());
  check(digest.certified_digest(), "mutated certificate can be rehashed");
  certificate.certificate_digest = digest.certificate_digest;
  certificate.digest_present = true;
}

[[nodiscard]] morsehgp3d::contract::CanonicalId flipped_id(
    const morsehgp3d::contract::CanonicalId& value,
    std::size_t byte_index = 0U) {
  auto bytes = value.bytes();
  bytes[byte_index % bytes.size()] ^= UINT8_C(1);
  return morsehgp3d::contract::CanonicalId{bytes};
}

void test_identity_layout_golden_mutations_and_caps(
    const SourceFixture& fixture) {
  ExactDirectSparseGatewayCandidateJournalResult empty;
  empty.traversal_order = LbvhTraversalOrder::near_first;
  const auto empty_identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          empty, generous_identity_budget());
  check(
      empty_identity.certified_identity() &&
          empty_identity.required_digest_payload_byte_count == 194U,
      "the empty source identity has the exact 194-byte canonical payload");
  constexpr std::string_view expected_empty_identity_digest =
      "ccfc328c790f8ccb2e81aaadde653826608e09c277117fd04af6b0c739f07482";
  check(
      empty_identity.scientific_identity_digest.to_lower_hex() ==
          expected_empty_identity_digest,
      "the empty source identity locks one SHA-256 field-order golden (actual " +
          empty_identity.scientific_identity_digest.to_lower_hex() + ")");

  const auto identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          fixture.source, generous_identity_budget());
  check(identity.certified_identity(), "the complete fixture identity succeeds");
  const std::size_t expected_payload =
      194U + 75U * fixture.source.deletion_projections.size() +
      313U * fixture.source.facet_tokens.size() +
      66U * fixture.source.gateway_candidates.size() +
      48U * fixture.source.batches.size() +
      8U * fixture.source.batch_facet_token_indices.size() +
      identity.required_exact_level_decimal_byte_count;
  check(
      identity.required_digest_payload_byte_count == expected_payload,
      "the nonempty identity matches 194+75R+313D+66C+48S+8I+E");

  const auto expect_digest_change =
      [&](ExactDirectSparseGatewayCandidateJournalResult mutated,
          const std::string& field) {
        const auto result =
            compute_exact_direct_sparse_gateway_candidate_scientific_identity(
                mutated, generous_identity_budget());
        check(
            result.certified_identity() &&
                result.scientific_identity_digest !=
                    identity.scientific_identity_digest,
            "the canonical source identity binds " + field);
      };

  auto mutated = fixture.source;
  ++mutated.schema_version;
  expect_digest_change(std::move(mutated), "the journal schema");
  mutated = fixture.source;
  mutated.traversal_order = LbvhTraversalOrder::far_first;
  expect_digest_change(std::move(mutated), "the LBVH traversal-order tag");
  mutated = fixture.source;
  ++mutated.point_count;
  expect_digest_change(std::move(mutated), "the source point count");
  mutated = fixture.source;
  ++mutated.source_direct_event_count;
  expect_digest_change(std::move(mutated), "the source event count");
  mutated = fixture.source;
  mutated.source_pair_canonical_cloud_digest =
      flipped_id(mutated.source_pair_canonical_cloud_digest);
  expect_digest_change(std::move(mutated), "the pair cloud authority");
  mutated = fixture.source;
  mutated.source_higher_canonical_cloud_digest =
      flipped_id(mutated.source_higher_canonical_cloud_digest);
  expect_digest_change(std::move(mutated), "the higher cloud authority");
  mutated = fixture.source;
  mutated.source_pair_semantic_digest =
      flipped_id(mutated.source_pair_semantic_digest);
  expect_digest_change(std::move(mutated), "the pair semantic authority");
  mutated = fixture.source;
  mutated.source_higher_semantic_digest =
      flipped_id(mutated.source_higher_semantic_digest);
  expect_digest_change(std::move(mutated), "the higher semantic authority");

  mutated = fixture.source;
  ++mutated.deletion_projections[0U].deletion_projection_index;
  expect_digest_change(std::move(mutated), "projection index");
  mutated = fixture.source;
  ++mutated.deletion_projections[0U].source_family_index;
  expect_digest_change(std::move(mutated), "projection source family");
  mutated = fixture.source;
  mutated.deletion_projections[0U].source =
      ExactDirectSparseGatewayDeletionSource::unspecified;
  expect_digest_change(std::move(mutated), "projection source tag");
  mutated = fixture.source;
  ++mutated.deletion_projections[0U].source_deletion_index;
  expect_digest_change(std::move(mutated), "projection source deletion");
  mutated = fixture.source;
  ++mutated.deletion_projections[0U].source_event_index;
  expect_digest_change(std::move(mutated), "projection source event");
  mutated = fixture.source;
  ++mutated.deletion_projections[0U].source_order;
  expect_digest_change(std::move(mutated), "projection source order");
  mutated = fixture.source;
  ++mutated.deletion_projections[0U].removed_point_id;
  expect_digest_change(std::move(mutated), "projection removed point");
  mutated = fixture.source;
  ++mutated.deletion_projections[0U].facet_token_index;
  expect_digest_change(std::move(mutated), "projection token reference");
  mutated = fixture.source;
  mutated.deletion_projections[0U].saddle_squared_level =
      morsehgp3d::exact::ExactLevel{
          mutated.deletion_projections[0U].saddle_squared_level.numerator() +
              1,
          mutated.deletion_projections[0U].saddle_squared_level.denominator()};
  expect_digest_change(std::move(mutated), "projection exact level");
  mutated = fixture.source;
  mutated.deletion_projections[0U].level_relation =
      ExactDirectSparseGatewayLevelRelation::unspecified;
  expect_digest_change(std::move(mutated), "projection level relation");
  mutated = fixture.source;
  mutated.deletion_projections[0U]
      .removed_point_is_first_incidence_cominimizer =
      !mutated.deletion_projections[0U]
           .removed_point_is_first_incidence_cominimizer;
  expect_digest_change(std::move(mutated), "projection cominimizer flag");

  mutated = fixture.source;
  ++mutated.facet_tokens[0U].facet_token_index;
  expect_digest_change(std::move(mutated), "facet-token index");
  mutated = fixture.source;
  ++mutated.facet_tokens[0U].source_facet_key.point_count;
  expect_digest_change(std::move(mutated), "facet-token key cardinality");
  for (std::size_t point_index = 0U;
       point_index <
       mutated.facet_tokens[0U].source_facet_key.point_ids.size();
       ++point_index) {
    mutated = fixture.source;
    ++mutated.facet_tokens[0U].source_facet_key.point_ids[point_index];
    expect_digest_change(
        std::move(mutated),
        "facet-token fixed key slot " + std::to_string(point_index));
  }
  mutated = fixture.source;
  mutated.facet_tokens[0U].source_miniball_squared_level =
      morsehgp3d::exact::ExactLevel{
          mutated.facet_tokens[0U]
                  .source_miniball_squared_level.numerator() +
              1,
          mutated.facet_tokens[0U]
              .source_miniball_squared_level.denominator()};
  expect_digest_change(std::move(mutated), "facet-token miniball level");
  mutated = fixture.source;
  mutated.facet_tokens[0U].first_incidence_squared_level =
      morsehgp3d::exact::ExactLevel{
          mutated.facet_tokens[0U]
                  .first_incidence_squared_level.numerator() +
              1,
          mutated.facet_tokens[0U]
              .first_incidence_squared_level.denominator()};
  expect_digest_change(std::move(mutated), "facet-token incidence level");

  using Audit = ExactDirectSparseFirstIncidenceAudit;
  constexpr std::array<std::size_t Audit::*, 18U> audit_counters{
      &Audit::eligible_coface_point_count,
      &Audit::source_support_enumeration_count,
      &Audit::node_visit_count,
      &Audit::internal_node_expansion_count,
      &Audit::exact_aabb_bound_evaluation_count,
      &Audit::exact_point_evaluation_count,
      &Audit::excluded_facet_point_count,
      &Audit::coface_support_enumeration_count,
      &Audit::candidate_point_classification_count,
      &Audit::inside_or_boundary_source_ball_point_count,
      &Audit::outside_source_ball_point_count,
      &Audit::pruned_node_count,
      &Audit::pruned_eligible_point_count,
      &Audit::peak_frontier_entry_count,
      &Audit::peak_cominimizer_entry_count,
      &Audit::incumbent_improvement_count,
      &Audit::equal_incumbent_observation_count,
      &Audit::provisional_cominimizer_overflow_count,
  };
  for (std::size_t counter_index = 0U;
       counter_index < audit_counters.size();
       ++counter_index) {
    mutated = fixture.source;
    ++(mutated.facet_tokens[0U].first_incidence_audit.*
       audit_counters[counter_index]);
    expect_digest_change(
        std::move(mutated),
        "facet-token 10.6 audit counter " + std::to_string(counter_index));
  }
  mutated = fixture.source;
  mutated.facet_tokens[0U].first_incidence_audit.traversal_complete =
      !mutated.facet_tokens[0U].first_incidence_audit.traversal_complete;
  expect_digest_change(std::move(mutated), "facet-token 10.6 audit flag");
  constexpr std::array<
      std::size_t ExactDirectSparseGatewayFacetToken::*,
      5U>
      token_indices{
          &ExactDirectSparseGatewayFacetToken::deletion_projection_offset,
          &ExactDirectSparseGatewayFacetToken::deletion_projection_count,
          &ExactDirectSparseGatewayFacetToken::gateway_candidate_offset,
          &ExactDirectSparseGatewayFacetToken::gateway_candidate_count,
          &ExactDirectSparseGatewayFacetToken::batch_index,
      };
  for (std::size_t field_index = 0U;
       field_index < token_indices.size();
       ++field_index) {
    mutated = fixture.source;
    ++(mutated.facet_tokens[0U].*token_indices[field_index]);
    expect_digest_change(
        std::move(mutated),
        "facet-token range/index field " + std::to_string(field_index));
  }

  mutated = fixture.source;
  ++mutated.gateway_candidates[0U].gateway_candidate_index;
  expect_digest_change(std::move(mutated), "candidate index");
  mutated = fixture.source;
  ++mutated.gateway_candidates[0U].facet_token_index;
  expect_digest_change(std::move(mutated), "candidate token reference");
  mutated = fixture.source;
  ++mutated.gateway_candidates[0U].added_point_id;
  expect_digest_change(std::move(mutated), "candidate added point");
  for (std::size_t point_index = 0U;
       point_index <
       mutated.gateway_candidates[0U].positive_support_point_ids.size();
       ++point_index) {
    mutated = fixture.source;
    ++mutated.gateway_candidates[0U]
          .positive_support_point_ids[point_index];
    expect_digest_change(
        std::move(mutated),
        "candidate support slot " + std::to_string(point_index));
  }
  mutated = fixture.source;
  ++mutated.gateway_candidates[0U].positive_support_point_count;
  expect_digest_change(std::move(mutated), "candidate support cardinality");
  mutated = fixture.source;
  mutated.gateway_candidates[0U].added_point_in_source_closed_ball =
      !mutated.gateway_candidates[0U].added_point_in_source_closed_ball;
  expect_digest_change(std::move(mutated), "candidate source-ball flag");
  mutated = fixture.source;
  mutated.gateway_candidates[0U]
      .added_point_in_selected_positive_support =
      !mutated.gateway_candidates[0U]
           .added_point_in_selected_positive_support;
  expect_digest_change(std::move(mutated), "candidate support flag");

  mutated = fixture.source;
  ++mutated.batches[0U].batch_index;
  expect_digest_change(std::move(mutated), "batch index");
  mutated = fixture.source;
  ++mutated.batches[0U].facet_cardinality;
  expect_digest_change(std::move(mutated), "batch facet cardinality");
  mutated = fixture.source;
  mutated.batches[0U].first_incidence_squared_level =
      morsehgp3d::exact::ExactLevel{
          mutated.batches[0U].first_incidence_squared_level.numerator() + 1,
          mutated.batches[0U].first_incidence_squared_level.denominator()};
  expect_digest_change(std::move(mutated), "batch exact level");
  mutated = fixture.source;
  ++mutated.batches[0U].facet_token_index_offset;
  expect_digest_change(std::move(mutated), "batch token offset");
  mutated = fixture.source;
  ++mutated.batches[0U].facet_token_index_count;
  expect_digest_change(std::move(mutated), "batch token count");
  mutated = fixture.source;
  ++mutated.batch_facet_token_indices[0U];
  expect_digest_change(std::move(mutated), "batch token-index arena");

  mutated = fixture.source;
  mutated.traversal_order =
      static_cast<LbvhTraversalOrder>(std::numeric_limits<std::uint8_t>::max());
  check(
      !compute_exact_direct_sparse_gateway_candidate_scientific_identity(
           mutated, generous_identity_budget())
           .certified_identity(),
      "an invalid traversal enum is rejected instead of hashed");
  mutated = fixture.source;
  mutated.deletion_projections[0U].source =
      static_cast<ExactDirectSparseGatewayDeletionSource>(
          std::numeric_limits<std::uint8_t>::max());
  check(
      !compute_exact_direct_sparse_gateway_candidate_scientific_identity(
           mutated, generous_identity_budget())
           .certified_identity(),
      "an invalid deletion-source enum is rejected instead of hashed");
  mutated = fixture.source;
  mutated.deletion_projections[0U].level_relation =
      static_cast<ExactDirectSparseGatewayLevelRelation>(
          std::numeric_limits<std::uint8_t>::max());
  check(
      !compute_exact_direct_sparse_gateway_candidate_scientific_identity(
           mutated, generous_identity_budget())
           .certified_identity(),
      "an invalid level-relation enum is rejected instead of hashed");

  mutated = fixture.source;
  constexpr std::size_t hostile_bit_count = 131072U;
  mutated.deletion_projections[0U].saddle_squared_level =
      morsehgp3d::exact::ExactLevel{
          morsehgp3d::exact::BigInt{1} << (hostile_bit_count - 1U)};
  auto hostile_budget = generous_identity_budget();
  hostile_budget.maximum_single_exact_level_integer_bit_count = 4096U;
  const auto hostile_identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          mutated, hostile_budget);
  check(
      !hostile_identity.certified_identity() &&
          hostile_identity.required_maximum_single_exact_level_integer_bit_count ==
              hostile_bit_count &&
          hostile_identity.required_exact_level_decimal_byte_count == 0U &&
          hostile_identity.decision ==
              ExactDirectSparseGatewayCandidateScientificIdentityDecision::
                  no_identity_budget_exhausted,
      "a hostile ExactLevel is rejected by its binary cap before decimal conversion");

  ExactDirectSparseGatewayCandidateScientificIdentityBudget exact{
      identity.required_deletion_projection_count,
      identity.required_facet_token_count,
      identity.required_gateway_candidate_count,
      identity.required_batch_count,
      identity.required_batch_facet_token_index_count,
      identity.required_maximum_single_exact_level_integer_bit_count,
      identity.required_exact_level_decimal_byte_count,
      identity.required_digest_payload_byte_count,
  };
  check(
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          fixture.source, exact)
          .certified_identity(),
      "all exact identity caps succeed");
  const auto check_population_minus_one = [&](std::size_t member_index) {
    auto limited = exact;
    std::array<std::size_t*, 5U> caps{
        &limited.maximum_deletion_projection_count,
        &limited.maximum_facet_token_count,
        &limited.maximum_gateway_candidate_count,
        &limited.maximum_batch_count,
        &limited.maximum_batch_facet_token_index_count,
    };
    --*caps[member_index];
    check(
        !compute_exact_direct_sparse_gateway_candidate_scientific_identity(
             fixture.source, limited)
             .certified_identity(),
        "each scientific arena population cap fails at exact minus one");
  };
  for (std::size_t index = 0U; index < 5U; ++index) {
    check_population_minus_one(index);
  }
  auto limited = exact;
  --limited.maximum_single_exact_level_integer_bit_count;
  check(
      !compute_exact_direct_sparse_gateway_candidate_scientific_identity(
           fixture.source, limited)
           .certified_identity(),
      "the pre-conversion ExactLevel bit cap fails at exact minus one");
  limited = exact;
  --limited.maximum_exact_level_decimal_byte_count;
  check(
      !compute_exact_direct_sparse_gateway_candidate_scientific_identity(
           fixture.source, limited)
           .certified_identity(),
      "the cumulative ExactLevel decimal-byte cap fails at exact minus one");
  limited = exact;
  --limited.maximum_digest_payload_byte_count;
  check(
      !compute_exact_direct_sparse_gateway_candidate_scientific_identity(
           fixture.source, limited)
           .certified_identity(),
      "the source digest payload-byte cap fails at exact minus one");
}

void test_certificate_digest_layout_and_golden() {
  ExactDirectSparseGatewayCandidateJournalResult empty_source;
  const auto identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          empty_source, generous_identity_budget());
  ExactDirectSparseGatewayClockCertificate certificate;
  certificate.authority_id = clock_authority_id;
  certificate.replay_token = clock_replay_token;
  certificate.source_scientific_identity_digest =
      identity.scientific_identity_digest;
  certificate.final_locator_stamp.schema_version = 1U;
  certificate.final_locator_stamp.external_authority_id = locator_authority_id;

  const auto empty_digest =
      compute_exact_direct_sparse_gateway_clock_certificate_digest(
          certificate, {0U, 136U});
  check(
      empty_digest.certified_digest() &&
          empty_digest.required_digest_payload_byte_count == 136U,
      "the empty certificate exact cap locks a 136-byte payload");
  constexpr std::string_view expected_empty_certificate_digest =
      "8d132290691d3d7ee7a1862cd1a3e9c591908e2da46259a4bf10f42552234630";
  check(
      empty_digest.certificate_digest.to_lower_hex() ==
          expected_empty_certificate_digest,
      "the empty certificate locks one SHA-256 field-order golden (actual " +
          empty_digest.certificate_digest.to_lower_hex() + ")");

  certificate.boundaries.push_back({0U, 0U, {}});
  const auto one =
      compute_exact_direct_sparse_gateway_clock_certificate_digest(
          certificate, {1U, 228U});
  certificate.boundaries.push_back({1U, 0U, {}});
  const auto two =
      compute_exact_direct_sparse_gateway_clock_certificate_digest(
          certificate, {2U, 320U});
  check(
      one.certified_digest() &&
          one.required_digest_payload_byte_count == 228U &&
          two.certified_digest() &&
          two.required_digest_payload_byte_count == 320U,
      "one and two boundaries have exact 228-byte and 320-byte payloads");
  check(
      !compute_exact_direct_sparse_gateway_clock_certificate_digest(
           certificate, {2U, 319U})
           .certified_digest(),
      "the certificate payload cap fails at exact minus one");
  check(
      !compute_exact_direct_sparse_gateway_clock_certificate_digest(
           certificate, {1U, 320U})
           .certified_digest(),
      "the certificate boundary population cap fails at exact minus one");
}

void test_empty_source_batches_keep_locator_clock_conditional() {
  const SourceFixture fixture = empty_source_fixture();
  const auto locator = make_locator();
  const std::vector<std::size_t> no_prefixes;
  const auto certificate =
      make_certificate(fixture.source, locator, no_prefixes);
  const auto verification = verify_clock(
      fixture,
      locator,
      anchor_for(certificate),
      certificate,
      generous_clock_budget(locator));
  check(
      verification.certified_conditional_clock_binding() &&
          verification.required_boundary_count == 0U &&
          verification.boundary_scan_count == 0U &&
          verification.sort_comparison_count == 0U &&
          certificate.final_locator_stamp.committed_batch_count == 3U,
      "S=0 verifies against a locator containing empty commits without inventing a boundary");
}

void test_conditional_clock_mapping_mutations_and_caps(
    const SourceFixture& fixture) {
  auto locator = make_locator();
  std::vector<std::size_t> source_prefixes(fixture.source.batches.size(), 1U);
  source_prefixes[0U] = 2U;
  source_prefixes[1U] = 0U;
  source_prefixes[2U] = 2U;
  const auto certificate =
      make_certificate(fixture.source, locator, source_prefixes);
  const auto anchor = anchor_for(certificate);
  const auto generous = generous_clock_budget(locator);

  const auto source_before = fixture.source;
  const auto locator_before = locator;
  const auto* source_projection_data = fixture.source.deletion_projections.data();
  const auto* source_token_data = fixture.source.facet_tokens.data();
  const auto* source_candidate_data = fixture.source.gateway_candidates.data();
  const auto* source_batch_data = fixture.source.batches.data();
  const auto* source_index_data = fixture.source.batch_facet_token_indices.data();
  const std::array<std::size_t, 5U> source_capacities{
      fixture.source.deletion_projections.capacity(),
      fixture.source.facet_tokens.capacity(),
      fixture.source.gateway_candidates.capacity(),
      fixture.source.batches.capacity(),
      fixture.source.batch_facet_token_indices.capacity(),
  };
  const auto* locator_slot_data = locator.slots().data();
  const auto* locator_key_data = locator.key_point_arena().data();
  const auto* locator_parent_data = locator.component_parents().data();
  const auto* locator_union_data = locator.committed_unions().data();
  const auto* locator_batch_data = locator.committed_batches().data();
  const std::array<std::size_t, 5U> locator_capacities{
      locator.slots().capacity(),
      locator.key_point_arena().capacity(),
      locator.component_parents().capacity(),
      locator.committed_unions().capacity(),
      locator.committed_batches().capacity(),
  };

  const auto verification =
      verify_clock(fixture, locator, anchor, certificate, generous);
  check(
      verification.certified_conditional_clock_binding() &&
          verification.result_certified &&
          !verification.external_clock_authority_replayed &&
          verification.conditional_on_caller_clock_authority_replay &&
          verification.boundary_scan_count == 2U * source_prefixes.size(),
      "the {2,0,2,...} mapping verifies without a source-order monotonicity premise and remains conditional");

  auto identity_result_mutation = verification.source_identity_result;
  ++identity_result_mutation.arena_record_scan_count;
  check(
      !identity_result_mutation.certified_identity(),
      "the identity predicate rejects a mutated arena scan count");
  identity_result_mutation = verification.source_identity_result;
  ++identity_result_mutation.required_digest_payload_byte_count;
  check(
      !identity_result_mutation.certified_identity(),
      "the identity predicate rejects a mutated exact payload size");
  identity_result_mutation = verification.source_identity_result;
  identity_result_mutation.digest_present = false;
  check(
      !identity_result_mutation.certified_identity(),
      "the identity predicate rejects a missing digest-presence fact");
  auto digest_result_mutation = verification.certificate_digest_result;
  ++digest_result_mutation.boundary_scan_count;
  check(
      !digest_result_mutation.certified_digest(),
      "the certificate-digest predicate rejects a mutated boundary scan");
  digest_result_mutation = verification.certificate_digest_result;
  ++digest_result_mutation.required_digest_payload_byte_count;
  check(
      !digest_result_mutation.certified_digest(),
      "the certificate-digest predicate rejects a mutated 136+92*S size");
  auto verification_mutation = verification;
  ++verification_mutation.boundary_scan_count;
  check(
      !verification_mutation.certified_conditional_clock_binding(),
      "the CLOCK predicate rejects a mutated top-level scan count");
  verification_mutation = verification;
  verification_mutation.source_identity_result.digest_present = false;
  check(
      !verification_mutation.certified_conditional_clock_binding(),
      "the CLOCK predicate recursively rejects a mutated nested identity");
  verification_mutation = verification;
  ++verification_mutation.prefix_stamp_sweep_counters.emitted_stamp_count;
  check(
      !verification_mutation.certified_conditional_clock_binding(),
      "the CLOCK predicate rejects a mutated PSTAMP emission count");
  verification_mutation = verification;
  ++verification_mutation.locator_stamp_at_exit.committed_batch_count;
  check(
      !verification_mutation.certified_conditional_clock_binding(),
      "the CLOCK predicate rejects unequal entry and exit stamps");
  check(
      fixture.source == source_before && locator == locator_before &&
          fixture.source.deletion_projections.data() == source_projection_data &&
          fixture.source.facet_tokens.data() == source_token_data &&
          fixture.source.gateway_candidates.data() == source_candidate_data &&
          fixture.source.batches.data() == source_batch_data &&
          fixture.source.batch_facet_token_indices.data() == source_index_data &&
          source_capacities ==
              std::array<std::size_t, 5U>{
                  fixture.source.deletion_projections.capacity(),
                  fixture.source.facet_tokens.capacity(),
                  fixture.source.gateway_candidates.capacity(),
                  fixture.source.batches.capacity(),
                  fixture.source.batch_facet_token_indices.capacity()} &&
          locator.slots().data() == locator_slot_data &&
          locator.key_point_arena().data() == locator_key_data &&
          locator.component_parents().data() == locator_parent_data &&
          locator.committed_unions().data() == locator_union_data &&
          locator.committed_batches().data() == locator_batch_data &&
          locator_capacities ==
              std::array<std::size_t, 5U>{
                  locator.slots().capacity(),
                  locator.key_point_arena().capacity(),
                  locator.component_parents().capacity(),
                  locator.committed_unions().capacity(),
                  locator.committed_batches().capacity()},
      "fresh verification preserves source/locator values, addresses and capacities for every arena");

  auto exact = generous;
  exact.maximum_boundary_count = verification.required_boundary_count;
  exact.maximum_boundary_scan_count = verification.required_boundary_scan_count;
  exact.maximum_sort_comparison_count = verification.sort_comparison_count;
  exact.maximum_sort_scratch_entry_count =
      verification.required_sort_scratch_entry_count;
  exact.maximum_prefix_scratch_entry_count =
      verification.required_prefix_scratch_entry_count;
  exact.maximum_temporary_scratch_byte_count =
      verification.required_temporary_scratch_byte_count;
  exact.source_identity_budget = {
      verification.source_identity_result.required_deletion_projection_count,
      verification.source_identity_result.required_facet_token_count,
      verification.source_identity_result.required_gateway_candidate_count,
      verification.source_identity_result.required_batch_count,
      verification.source_identity_result
          .required_batch_facet_token_index_count,
      verification.source_identity_result
          .required_maximum_single_exact_level_integer_bit_count,
      verification.source_identity_result.required_exact_level_decimal_byte_count,
      verification.source_identity_result.required_digest_payload_byte_count,
  };
  exact.certificate_digest_budget = {
      verification.certificate_digest_result.required_boundary_count,
      verification.certificate_digest_result.required_digest_payload_byte_count,
  };
  const auto& prefix_counters = verification.prefix_stamp_sweep_counters;
  exact.prefix_stamp_sweep_budget = {
      prefix_counters.prefix_request_scan_count,
      prefix_counters.batch_record_scan_count,
      prefix_counters.table_slot_scan_count,
      prefix_counters.binding_record_replay_count,
      prefix_counters.union_record_replay_count,
      prefix_counters.binding_record_replay_count,
      prefix_counters.key_point_replay_count,
      prefix_counters.binding_record_replay_count * sizeof(std::size_t),
  };
  check(
      verify_clock(fixture, locator, anchor, certificate, exact)
          .certified_conditional_clock_binding(),
      "all exact top-level, identity and certificate caps succeed");

  auto limited = exact;
  --limited.maximum_boundary_count;
  check(
      !verify_clock(fixture, locator, anchor, certificate, limited)
           .certified_conditional_clock_binding(),
      "boundary population fails before allocation at exact minus one");
  limited = exact;
  --limited.maximum_boundary_scan_count;
  check(
      !verify_clock(fixture, locator, anchor, certificate, limited)
           .certified_conditional_clock_binding(),
      "the two-pass boundary scan cap fails at exact minus one");
  limited = exact;
  --limited.maximum_sort_comparison_count;
  check(
      !verify_clock(fixture, locator, anchor, certificate, limited)
           .certified_conditional_clock_binding(),
      "heapsort comparisons fail at exact minus one");
  limited = exact;
  --limited.maximum_sort_scratch_entry_count;
  check(
      !verify_clock(fixture, locator, anchor, certificate, limited)
           .certified_conditional_clock_binding(),
      "sort scratch entries fail before allocation at exact minus one");
  limited = exact;
  --limited.maximum_prefix_scratch_entry_count;
  check(
      !verify_clock(fixture, locator, anchor, certificate, limited)
           .certified_conditional_clock_binding(),
      "prefix scratch entries fail before allocation at exact minus one");
  limited = exact;
  --limited.maximum_temporary_scratch_byte_count;
  check(
      !verify_clock(fixture, locator, anchor, certificate, limited)
           .certified_conditional_clock_binding(),
      "top-level scratch bytes fail before allocation at exact minus one");

  const auto check_prefix_budget_minus_one =
      [&](std::size_t member_index) {
        auto nested_limited = exact;
        auto& prefix_budget = nested_limited.prefix_stamp_sweep_budget;
        std::array<std::size_t*, 8U> caps{
            &prefix_budget.maximum_prefix_request_count,
            &prefix_budget.maximum_batch_record_scan_count,
            &prefix_budget.maximum_table_slot_scan_count,
            &prefix_budget.maximum_binding_slot_index_scratch_count,
            &prefix_budget.maximum_union_record_replay_count,
            &prefix_budget.maximum_binding_record_replay_count,
            &prefix_budget.maximum_key_point_replay_count,
            &prefix_budget.maximum_temporary_scratch_byte_count,
        };
        check(
            *caps[member_index] > 0U,
            "every exercised PSTAMP cap has positive exact work");
        --*caps[member_index];
        check(
            !verify_clock(
                 fixture, locator, anchor, certificate, nested_limited)
                 .certified_conditional_clock_binding(),
            "each nested PSTAMP cap fails at exact minus one");
      };
  for (std::size_t index = 0U; index < 8U; ++index) {
    check_prefix_budget_minus_one(index);
  }

  std::size_t positive_structural_caps_tested = 0U;
  for (std::size_t member_index = 0U; member_index < 13U; ++member_index) {
    auto structural_limited = exact;
    auto& structure_budget = structural_limited.locator_structure_budget;
    std::array<std::size_t*, 13U> caps{
        &structure_budget.maximum_table_slot_count,
        &structure_budget.maximum_key_point_count,
        &structure_budget.maximum_component_parent_count,
        &structure_budget.maximum_union_record_count,
        &structure_budget.maximum_batch_record_count,
        &structure_budget.maximum_binding_scratch_entry_count,
        &structure_budget.maximum_key_point_scratch_entry_count,
        &structure_budget.maximum_table_slot_scratch_entry_count,
        &structure_budget.maximum_component_parent_scratch_entry_count,
        &structure_budget.maximum_temporary_scratch_byte_count,
        &structure_budget.maximum_fingerprint_search_slot_visit_count,
        &structure_budget.maximum_insertion_chronology_slot_visit_count,
        &structure_budget.maximum_union_parent_hop_count,
    };
    if (*caps[member_index] == 0U) {
      continue;
    }
    ++positive_structural_caps_tested;
    --*caps[member_index];
    check(
        !verify_clock(
             fixture, locator, anchor, certificate, structural_limited)
             .certified_conditional_clock_binding(),
        "every positive nested structural-verifier cap fails at exact minus one");
  }
  check(
      positive_structural_caps_tested >= 12U,
      "the hostile locator exercises at least twelve positive structural caps");

  std::vector<std::size_t> final_prefixes = source_prefixes;
  final_prefixes[0U] = locator.committed_batches().size();
  const auto final_certificate =
      make_certificate(fixture.source, locator, final_prefixes);
  check(
      verify_clock(
          fixture,
          locator,
          anchor_for(final_certificate),
          final_certificate,
          generous)
          .certified_conditional_clock_binding(),
      "a source boundary may name the final locator prefix as well as zero and repeated prefixes");

  auto bad_anchor = anchor;
  bad_anchor.authority_id = 0U;
  check(
      !verify_clock(fixture, locator, bad_anchor, certificate, generous)
           .certified_conditional_clock_binding(),
      "a zero external clock authority is rejected");
  bad_anchor = anchor;
  bad_anchor.replay_token = 0U;
  check(
      !verify_clock(fixture, locator, bad_anchor, certificate, generous)
           .certified_conditional_clock_binding(),
      "a zero external replay token is rejected");
  auto malformed = certificate;
  malformed.digest_present = false;
  check(
      !verify_clock(fixture, locator, anchor, malformed, generous)
           .certified_conditional_clock_binding(),
      "digest_present is mandatory even when digest bytes are retained");
  malformed = certificate;
  ++malformed.schema_version;
  check(
      !verify_clock(fixture, locator, anchor, malformed, generous)
           .certified_conditional_clock_binding(),
      "an unknown certificate schema is rejected before replay");
  malformed = certificate;
  malformed.certificate_digest = morsehgp3d::contract::CanonicalId{};
  malformed.digest_present = true;
  auto zero_digest_anchor = anchor_for(malformed);
  check(
      !verify_clock(
           fixture, locator, zero_digest_anchor, malformed, generous)
           .certified_conditional_clock_binding(),
      "an explicitly present all-zero digest is data and is rejected only by fresh digest mismatch");
  bad_anchor = anchor;
  ++bad_anchor.authority_id;
  check(
      !verify_clock(fixture, locator, bad_anchor, certificate, generous)
           .certified_conditional_clock_binding(),
      "a nonzero foreign external clock authority is rejected");
  bad_anchor = anchor;
  ++bad_anchor.replay_token;
  check(
      !verify_clock(fixture, locator, bad_anchor, certificate, generous)
           .certified_conditional_clock_binding(),
      "a nonzero foreign external replay token is rejected");
  bad_anchor = anchor;
  bad_anchor.expected_certificate_digest =
      flipped_id(bad_anchor.expected_certificate_digest);
  check(
      !verify_clock(fixture, locator, bad_anchor, certificate, generous)
           .certified_conditional_clock_binding(),
      "a foreign external expected digest is rejected");

  std::vector<std::size_t> replacement_prefixes = source_prefixes;
  replacement_prefixes[0U] = 0U;
  const auto replacement =
      make_certificate(fixture.source, locator, replacement_prefixes);
  check(
      !verify_clock(fixture, locator, anchor, replacement, generous)
           .certified_conditional_clock_binding(),
      "a rehashed payload is rejected while the separate anchor stays unchanged");
  const auto replacement_anchor = anchor_for(replacement);
  const auto replacement_verification = verify_clock(
      fixture, locator, replacement_anchor, replacement, generous);
  check(
      replacement_verification.certified_conditional_clock_binding() &&
          !replacement_verification.external_clock_authority_replayed &&
          replacement_verification.conditional_on_caller_clock_authority_replay,
      "a replaced matching anchor accepts only a conditional clock binding");

  auto mutated = certificate;
  ++mutated.boundaries[0U].historical_locator_stamp.committed_batch_count;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed historical stamp mutation is rejected by PSTAMP replay");
  mutated = certificate;
  ++mutated.boundaries[0U].historical_locator_stamp.schema_version;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed historical stamp schema is rejected by PSTAMP replay");
  mutated = certificate;
  ++mutated.boundaries[0U].historical_locator_stamp.external_authority_id;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed historical locator authority is rejected by PSTAMP replay");
  mutated = certificate;
  ++mutated.boundaries[0U].historical_locator_stamp.inserted_key_count;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed historical inserted-key count is rejected by PSTAMP replay");
  mutated = certificate;
  ++mutated.boundaries[0U].historical_locator_stamp.component_union_count;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed historical union count is rejected by PSTAMP replay");
  mutated = certificate;
  ++mutated.boundaries[0U].historical_locator_stamp.binding_count;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed historical binding count is rejected by PSTAMP replay");
  mutated = certificate;
  mutated.boundaries[0U].historical_locator_stamp.committed_history_digest =
      flipped_id(
          mutated.boundaries[0U]
              .historical_locator_stamp.committed_history_digest);
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed historical locator digest is rejected by PSTAMP replay");
  mutated = certificate;
  mutated.boundaries[0U].strict_pre_locator_prefix_count = 0U;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed prefix without its matching stamp is rejected");
  mutated = certificate;
  ++mutated.boundaries[0U].source_batch_index;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed non-dense source index is rejected");
  mutated = certificate;
  auto source_digest_bytes =
      mutated.source_scientific_identity_digest.bytes();
  source_digest_bytes[0U] ^= UINT8_C(1);
  mutated.source_scientific_identity_digest =
      morsehgp3d::contract::CanonicalId{source_digest_bytes};
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed source-identity mutation is rejected by fresh 10.7 identity");
  mutated = certificate;
  ++mutated.final_locator_stamp.committed_batch_count;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed final-stamp mutation is rejected");
  mutated = certificate;
  ++mutated.final_locator_stamp.schema_version;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed final locator schema is rejected");
  mutated = certificate;
  ++mutated.final_locator_stamp.external_authority_id;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed final locator authority is rejected");
  mutated = certificate;
  ++mutated.final_locator_stamp.inserted_key_count;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed final inserted-key count is rejected");
  mutated = certificate;
  ++mutated.final_locator_stamp.component_union_count;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed final union count is rejected");
  mutated = certificate;
  ++mutated.final_locator_stamp.binding_count;
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed final binding count is rejected");
  mutated = certificate;
  mutated.final_locator_stamp.committed_history_digest =
      flipped_id(mutated.final_locator_stamp.committed_history_digest);
  rehash_certificate(mutated);
  check(
      !verify_clock(fixture, locator, anchor_for(mutated), mutated, generous)
           .certified_conditional_clock_binding(),
      "a rehashed final locator digest is rejected");

  auto advanced_locator = locator;
  check(
      advanced_locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "the stale-certificate test advances the locator with an empty commit");
  const auto advanced_budget = generous_clock_budget(advanced_locator);
  check(
      !verify_clock(
           fixture, advanced_locator, anchor, certificate, advanced_budget)
           .certified_conditional_clock_binding(),
      "a certificate anchored to the prior final locator snapshot is stale");

  auto hostile_suffix_locator = locator;
  auto& hostile_batches = const_cast<
      std::vector<ExactDirectSparseCommittedBatchRecord>&>(
      hostile_suffix_locator.committed_batches());
  ++hostile_batches[2U].committed_batch_index;
  check(
      !verify_clock(
           fixture,
           hostile_suffix_locator,
           anchor,
           certificate,
           generous)
           .certified_conditional_clock_binding(),
      "full locator structure replay rejects a corrupt suffix beyond max(p_s)=2 before PSTAMP");
}

}  // namespace

int main() {
  const SourceFixture fixture = source_fixture();
  test_identity_layout_golden_mutations_and_caps(fixture);
  test_certificate_digest_layout_and_golden();
  test_empty_source_batches_keep_locator_clock_conditional();
  test_conditional_clock_mapping_mutations_and_caps(fixture);
  if (failures != 0) {
    std::cerr << failures << " direct sparse gateway clock test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse gateway clock tests passed\n";
  return 0;
}
