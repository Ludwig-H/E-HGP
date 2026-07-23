#include "morsehgp3d/hierarchy/direct_sparse_gateway_historical_quotient.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <span>
#include <string>
#include <type_traits>
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
static_assert(
    !std::is_copy_constructible_v<
        ExactDirectSparseGatewayClockAuthorityJournal>);
static_assert(
    std::is_nothrow_move_constructible_v<
        ExactDirectSparseGatewayClockAuthorityJournal>);
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

[[nodiscard]] ExactDirectSparseGatewayCandidateLocalizationBudget
generous_localization_budget() {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  return {
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      maximum,
      {maximum, maximum},
  };
}

[[nodiscard]] ExactDirectSparsePositiveFacetPrefixSweepBudget
generous_prefix_sweep_budget() {
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
      {maximum, maximum},
  };
}

[[nodiscard]] ExactDirectSparseGatewayTemporalResolutionBudget
generous_temporal_resolution_budget() {
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
  };
}

[[nodiscard]] ExactDirectSparseGatewayHistoricalQuotientBudget
generous_historical_quotient_budget() {
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
  };
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

void test_in_memory_clock_authority_replay(const SourceFixture& fixture) {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  const ExactDirectSparseGatewayClockAuthorityJournalBudget authority_budget{
      maximum,
      maximum,
      maximum,
      maximum,
  };
  const auto source_identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          fixture.source, generous_identity_budget());
  check(
      source_identity.certified_identity(),
      "AUTH fixture binds one certified 10.7 scientific identity before capture");
  auto locator = build_exact_direct_sparse_positive_facet_locator(
      8U, locator_budget(), {locator_authority_id, ~UINT64_C(0)});
  auto authority = build_exact_direct_sparse_gateway_clock_authority_journal(
      clock_authority_id,
      clock_replay_token,
      source_identity,
      locator,
      authority_budget);
  check(
      authority.certified_initialized_authority() &&
          authority.capture_records().empty() &&
          !authority.certificate_present(),
      "AUTH opens one preallocated in-memory authority");
  const auto source_before = fixture.source;
  const auto locator_before_capture = locator;

  const auto first = authority.capture_source_batch(1U, locator);
  check(
      first.certified_capture() &&
          first.committed_record.chronological_index == 0U &&
          first.committed_record.source_batch_index == 1U &&
          first.committed_record.locator_snapshot_stamp
                  .committed_batch_count == 0U,
      "AUTH captures source batch one first at the live zero prefix");

  const auto records_before_duplicate = authority.capture_records();
  const auto chain_before_duplicate =
      authority.current_capture_chain_digest();
  const auto duplicate = authority.capture_source_batch(1U, locator);
  check(
      duplicate.certified_atomic_failure() &&
          authority.capture_records() == records_before_duplicate &&
          authority.current_capture_chain_digest() ==
              chain_before_duplicate,
      "AUTH rejects a duplicate source batch without advancing its chain");

  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              std::span<const ExactDirectSparseFacetBinding>{})
          .certified_committed_batch(),
      "AUTH fixture inserts one explicit empty locator commit");
  const auto second = authority.capture_source_batch(0U, locator);
  const auto records_before_decreasing_capture = authority.capture_records();
  const auto decreasing =
      authority.capture_source_batch(2U, locator_before_capture);
  check(
      decreasing.certified_atomic_failure() &&
          authority.capture_records() == records_before_decreasing_capture,
      "AUTH rejects a chronologically decreasing live locator prefix");
  const auto third = authority.capture_source_batch(2U, locator);
  check(
      second.certified_capture() && third.certified_capture() &&
          second.committed_record.locator_snapshot_stamp
                  .committed_batch_count == 1U &&
          third.committed_record.locator_snapshot_stamp ==
              second.committed_record.locator_snapshot_stamp,
      "AUTH records a nonmonotone source order and equal live prefix stamps");
  for (std::size_t source_batch_index = 3U;
       source_batch_index < fixture.source.batches.size();
       ++source_batch_index) {
    check(
        authority.capture_source_batch(source_batch_index, locator)
            .certified_capture(),
        "AUTH captures every remaining source batch exactly once");
  }

  const ExactDirectSparseGatewayClockAuthoritySealBudget seal_budget{
      maximum,
      maximum,
      maximum,
      maximum,
      generous_identity_budget(),
      generous_digest_budget(),
  };
  const auto seal =
      authority.seal_clock_certificate(fixture.source, locator, seal_budget);
  check(
      seal.certified_seal() && authority.certified_sealed_once() &&
          authority.sealed_certificate().boundaries.size() ==
              fixture.source.batches.size() &&
          authority.sealed_certificate().boundaries[0U]
                  .strict_pre_locator_prefix_count == 1U &&
          authority.sealed_certificate().boundaries[1U]
                  .strict_pre_locator_prefix_count == 0U,
      "AUTH seals exactly one CLOCK certificate reindexed by source batch (decision " +
          std::to_string(static_cast<unsigned>(seal.decision)) + ")");

  const auto records_after_seal = authority.capture_records();
  const auto certificate_after_seal = authority.sealed_certificate();
  const auto capture_after_seal =
      authority.capture_source_batch(0U, locator);
  check(
      capture_after_seal.certified_atomic_failure() &&
          authority.capture_records() == records_after_seal &&
          authority.sealed_certificate() == certificate_after_seal,
      "AUTH is immutable after its one successful seal");

  const ExactDirectSparseGatewayClockAuthorityVerificationBudget
      verification_budget{
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          generous_clock_budget(locator),
      };
  const ExactDirectSparseGatewayClockAuthorityExternalSealAnchor
      external_seal_anchor{
          clock_authority_id,
          clock_replay_token,
          authority.seal_digest(),
      };
  const auto verify_authority =
      [&](const ExactDirectSparseGatewayClockAuthorityExternalSealAnchor&
              anchor,
          const ExactDirectSparseGatewayClockAuthorityVerificationBudget&
              budget) {
        return verify_exact_direct_sparse_gateway_clock_authority_journal(
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
            authority_budget,
            authority,
            budget);
      };
  const auto locator_before_verification = locator;
  const auto verified =
      verify_authority(external_seal_anchor, verification_budget);
  check(
      verified.certified_external_clock_binding() &&
          verified.external_clock_authority_replayed &&
          !verified.conditional_on_caller_clock_authority_replay &&
          verified.conditional_on_caller_strict_pre_lot_orchestration &&
          !verified.external_freeze_synchronization_replayed &&
          verified.conditional_on_caller_external_freeze_synchronization &&
          verified.in_memory_replay_only && !verified.crash_durable &&
          verified.clock_verification
              .certified_conditional_clock_binding(),
      "AUTH fresh replay discharges CLOCK's external in-memory authority only");
  check(
      fixture.source == source_before && locator == locator_before_verification &&
          locator_before_capture.snapshot_stamp().committed_batch_count == 0U,
      "AUTH replay leaves the source and live locator immutable");
  auto foreign_anchor = external_seal_anchor;
  foreign_anchor.expected_seal_digest =
      flipped_id(foreign_anchor.expected_seal_digest);
  check(
      !verify_authority(foreign_anchor, verification_budget)
           .certified_external_clock_binding(),
      "AUTH rejects a divergent caller-owned final seal anchor");

  auto tight_budget = verification_budget;
  tight_budget.maximum_capture_record_count =
      verified.required_capture_record_count;
  tight_budget.maximum_capture_record_scan_count =
      verified.required_capture_record_scan_count;
  tight_budget.maximum_source_presence_entry_count =
      verified.required_source_presence_entry_count;
  tight_budget.maximum_source_presence_scan_count =
      verified.required_source_presence_scan_count;
  tight_budget.maximum_temporary_scratch_byte_count =
      verified.required_temporary_scratch_byte_count;
  check(
      verify_authority(external_seal_anchor, tight_budget)
          .certified_external_clock_binding(),
      "AUTH fresh replay passes at its exact top-level caps");
  if (tight_budget.maximum_capture_record_scan_count > 0U) {
    --tight_budget.maximum_capture_record_scan_count;
    check(
        !verify_authority(external_seal_anchor, tight_budget)
             .certified_external_clock_binding(),
        "AUTH fresh replay fails closed at one representative exact-minus-one cap");
  }
}

void test_historical_gateway_temporal_resolution(
    const SourceFixture& fixture) {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  const auto localization_budget = generous_localization_budget();
  const auto prefix_sweep_budget = generous_prefix_sweep_budget();
  const auto temporal_budget = generous_temporal_resolution_budget();
  const ExactDirectSparseFacetWitness query_witness{
      locator_authority_id, UINT64_C(0x10d13001)};
  const auto trusted_locator_budget = locator_budget();
  const ExactDirectSparsePositiveFacetLocatorConfig locator_config{
      locator_authority_id, ~UINT64_C(0)};
  auto locator = build_exact_direct_sparse_positive_facet_locator(
      8U, trusted_locator_budget, locator_config);

  const auto empty_localization =
      build_exact_direct_sparse_gateway_candidate_localization(
          fixture.index,
          fixture.cloud,
          fixture.sources.facade,
          fixture.sources.event_journal,
          fixture.sources.arm_budget,
          fixture.sources.arm_journal,
          fixture.sources.incidence_budget,
          fixture.sources.incidence_journal,
          fixture.source_budget,
          fixture.source,
          query_witness,
          locator,
          localization_budget,
          LbvhTraversalOrder::near_first);
  check(
      empty_localization.certified_partial_refinement() &&
          empty_localization.localized_facet_tokens.size() >= 2U,
      "10.13 fixture first exposes at least two latent deletion tokens");
  if (empty_localization.localized_facet_tokens.size() < 2U) {
    return;
  }

  const auto initially_positive_token =
      empty_localization.localized_facet_tokens[0U];
  const std::array<ExactDirectSparseFacetBinding, 1U> initial_binding{{
      {
          0U,
          initially_positive_token.facet_key,
          3U,
          {locator_authority_id, UINT64_C(0x10d13002)},
      },
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              initial_binding)
          .certified_committed_batch(),
      "10.14 fixture publishes one known root before every source capture");

  const auto source_identity =
      compute_exact_direct_sparse_gateway_candidate_scientific_identity(
          fixture.source, generous_identity_budget());
  const ExactDirectSparseGatewayClockAuthorityJournalBudget authority_budget{
      maximum,
      maximum,
      maximum,
      maximum,
  };
  auto authority = build_exact_direct_sparse_gateway_clock_authority_journal(
      clock_authority_id + 10U,
      clock_replay_token + 10U,
      source_identity,
      locator,
      authority_budget);
  for (std::size_t source_batch_index = 0U;
       source_batch_index < fixture.source.batches.size();
       ++source_batch_index) {
    check(
        authority.capture_source_batch(source_batch_index, locator)
            .certified_capture(),
        "10.13 captures every source batch at its strict pre-lot prefix");
  }

  const auto selected_token =
      empty_localization.localized_facet_tokens[1U];
  const std::array<ExactDirectSparseFacetBinding, 1U> binding{{
      {
          0U,
          selected_token.facet_key,
          3U,
          {locator_authority_id, UINT64_C(0x10d13003)},
      },
  }};
  check(
      locator
          .apply_batch(
              std::span<const ExactDirectSparseFacetQuery>{},
              std::span<const ExactDirectSparseComponentUnion>{},
              binding)
          .certified_committed_batch(),
      "10.13 fixture commits one second facet only after every source capture");
  const ExactDirectSparseGatewayClockAuthoritySealBudget seal_budget{
      maximum,
      maximum,
      maximum,
      maximum,
      generous_identity_budget(),
      generous_digest_budget(),
  };
  check(
      authority
          .seal_clock_certificate(fixture.source, locator, seal_budget)
          .certified_seal(),
      "10.13 seals the source-to-prefix authority after the later binding");

  const auto final_localization =
      build_exact_direct_sparse_gateway_candidate_localization(
          fixture.index,
          fixture.cloud,
          fixture.sources.facade,
          fixture.sources.event_journal,
          fixture.sources.arm_budget,
          fixture.sources.arm_journal,
          fixture.sources.incidence_budget,
          fixture.sources.incidence_journal,
          fixture.source_budget,
          fixture.source,
          query_witness,
          locator,
          localization_budget,
          LbvhTraversalOrder::near_first);
  check(
      final_localization.certified_partial_refinement() &&
          final_localization
                  .localized_facet_tokens
                      [selected_token.localized_facet_token_index]
                  .disposition ==
              ExactDirectSparseGatewayFacetLocalizationDisposition::
                  relative_positive,
      "the selected token is positive only in the final locator snapshot");

  const auto result =
      build_exact_direct_sparse_gateway_temporal_resolution(
          fixture.source,
          final_localization,
          authority,
          locator,
          temporal_budget,
          prefix_sweep_budget);
  check(
      result.certified_partial_refinement() &&
          result.projection_to_resolution.size() ==
              final_localization.deletion_projections.size() &&
          !result.temporal_resolutions.empty() &&
          result
              .two_scientific_output_arenas_without_copied_facet_keys &&
          !result.missing_facet_means_isolated &&
          !result.quotient_root_union_or_forest_mutated,
      "10.13 publishes exactly two read-only sparse arenas");
  check(
      result.counters.historical_positive_count > 0U &&
          result.counters.historical_latent_count > 0U,
      "the shared 10.13 fixture carries both historical roots and latent facets");

  std::set<std::pair<std::size_t, std::size_t>> expected_pairs;
  for (const auto& projection : final_localization.deletion_projections) {
    const std::size_t prefix =
        authority.sealed_certificate()
            .boundaries[projection.source_batch_index]
            .strict_pre_locator_prefix_count;
    expected_pairs.emplace(prefix, projection.localized_facet_token_index);
  }
  check(
      result.temporal_resolutions.size() == expected_pairs.size(),
      "10.13 deduplicates by the exact (source prefix, localized token) pair");

  bool selected_historical_latent = false;
  for (const auto& resolution : result.temporal_resolutions) {
    if (resolution.localized_facet_token_index ==
        selected_token.localized_facet_token_index) {
      selected_historical_latent =
          selected_historical_latent ||
          (resolution.strict_pre_locator_prefix_count == 1U &&
           resolution.disposition ==
               ExactDirectSparseGatewayTemporalDisposition::
                   latent_unresolved &&
           !resolution.positive_payload_present);
    }
  }
  check(
      selected_historical_latent &&
          result.final_latent_implies_historical_latent &&
          result.historical_positive_implies_final_positive_with_same_witness,
      "10.13 preserves a historical latent token despite its later positive binding");

  const auto historical_quotient_budget =
      generous_historical_quotient_budget();
  const auto historical_quotient =
      build_exact_direct_sparse_gateway_historical_quotient(
          fixture.source,
          final_localization,
          result,
          historical_quotient_budget);
  check(
      historical_quotient.certified_partial_refinement() &&
          historical_quotient.batches.size() ==
              fixture.source.batches.size() &&
          historical_quotient.candidate_to_component.size() ==
              fixture.source.gateway_candidates.size() &&
          !historical_quotient.components.empty() &&
          !historical_quotient.component_root_ids.empty() &&
          !historical_quotient.component_latent_resolution_indices.empty() &&
          historical_quotient.shared_latent_facets_join_candidate_hyperedges &&
          historical_quotient.closure_strictly_batch_local &&
          historical_quotient.latent_only_components_preserved_as_unresolved &&
          !historical_quotient.latent_facet_means_isolated &&
          !historical_quotient.locator_root_union_or_forest_mutated &&
          !historical_quotient.gateway_attach_published,
      "10.14 closes typed historical incidences without mutating a root or publishing an attach");

  bool latent_only_pair_found = false;
  for (std::size_t first_index = 0U;
       first_index < historical_quotient.candidate_to_component.size();
       ++first_index) {
    const auto& first =
        historical_quotient.candidate_to_component[first_index];
    if (first.historical_positive_projection_count != 0U) {
      continue;
    }
    std::set<std::size_t> first_latent_resolutions;
    for (std::size_t offset = 0U;
         offset < first.deletion_projection_count;
         ++offset) {
      const auto& projection =
          result.projection_to_resolution
              [first.deletion_projection_offset + offset];
      const auto& resolution =
          result.temporal_resolutions
              [projection.temporal_resolution_index];
      if (resolution.disposition ==
          ExactDirectSparseGatewayTemporalDisposition::
              latent_unresolved) {
        first_latent_resolutions.insert(
            projection.temporal_resolution_index);
      }
    }
    for (std::size_t second_index = first_index + 1U;
         second_index <
         historical_quotient.candidate_to_component.size();
         ++second_index) {
      const auto& second =
          historical_quotient.candidate_to_component[second_index];
      if (second.source_batch_index != first.source_batch_index ||
          second.historical_component_index !=
              first.historical_component_index ||
          second.historical_positive_projection_count != 0U) {
        continue;
      }
      for (std::size_t offset = 0U;
           offset < second.deletion_projection_count;
           ++offset) {
        const auto& projection =
            result.projection_to_resolution
                [second.deletion_projection_offset + offset];
        if (first_latent_resolutions.contains(
                projection.temporal_resolution_index)) {
          latent_only_pair_found = true;
          break;
        }
      }
      if (latent_only_pair_found) {
        break;
      }
    }
    if (latent_only_pair_found) {
      break;
    }
  }
  check(
      latent_only_pair_found &&
          historical_quotient.counters.latent_only_component_count > 0U,
      "10.14 joins two candidates solely through a shared historical latent facet and preserves their latent-only component");

  bool two_batches_observed = false;
  bool all_cross_batch_components_separated = true;
  for (std::size_t first_index = 0U;
       first_index < historical_quotient.candidate_to_component.size();
       ++first_index) {
    for (std::size_t second_index = first_index + 1U;
         second_index <
         historical_quotient.candidate_to_component.size();
         ++second_index) {
      const auto& first =
          historical_quotient.candidate_to_component[first_index];
      const auto& second =
          historical_quotient.candidate_to_component[second_index];
      if (first.source_batch_index != second.source_batch_index) {
        two_batches_observed = true;
        all_cross_batch_components_separated =
            all_cross_batch_components_separated &&
            first.historical_component_index !=
                second.historical_component_index;
      }
    }
  }
  check(
      two_batches_observed && all_cross_batch_components_separated,
      "10.14 keeps candidate closure separated across two source batches");

  auto malformed_result = result;
  ++malformed_result.temporal_resolutions.front()
        .projection_reference_count;
  check(
      !malformed_result.certified_partial_refinement(),
      "10.13 self-contained validation rejects an inconsistent reference total");

  const ExactDirectSparseGatewayClockAuthorityExternalSealAnchor
      external_seal_anchor{
          authority.authority_id(),
          authority.session_id(),
          authority.seal_digest(),
      };
  const ExactDirectSparseGatewayClockAuthorityVerificationBudget
      authority_verification_budget{
          maximum,
          maximum,
          maximum,
          maximum,
          maximum,
          generous_clock_budget(locator),
      };
  const auto structure_budget = exact_structure_budget(locator);
  ExactDirectSparseGatewayTemporalResolutionAuthorityBundle authorities;
  authorities.index = &fixture.index;
  authorities.cloud = &fixture.cloud;
  authorities.source_facade = &fixture.sources.facade;
  authorities.source_journal = &fixture.sources.event_journal;
  authorities.source_arm_budget = &fixture.sources.arm_budget;
  authorities.source_arm_journal = &fixture.sources.arm_journal;
  authorities.source_incidence_budget = &fixture.sources.incidence_budget;
  authorities.source_incidence_journal =
      &fixture.sources.incidence_journal;
  authorities.source_gateway_budget = &fixture.source_budget;
  authorities.source_gateway_journal = &fixture.source;
  authorities.traversal_order = LbvhTraversalOrder::near_first;
  authorities.locator_query_witness = &query_witness;
  authorities.locator_budget = &trusted_locator_budget;
  authorities.locator_config = &locator_config;
  authorities.locator = &locator;
  authorities.trusted_component_handle_count = 8U;
  authorities.localization_budget = &localization_budget;
  authorities.observed_localization = &final_localization;
  authorities.external_seal_anchor = &external_seal_anchor;
  authorities.authority_journal_budget = &authority_budget;
  authorities.observed_authority = &authority;
  authorities.authority_verification_budget =
      &authority_verification_budget;
  authorities.prefix_locator_structure_budget = &structure_budget;
  const auto verified =
      verify_exact_direct_sparse_gateway_temporal_resolution(
          authorities,
          temporal_budget,
          prefix_sweep_budget,
          result);
  check(
      verified.result_certified &&
          verified.localization_freshly_replayed &&
          verified.clock_authority_freshly_replayed &&
          verified.prefix_sweep_freshly_replayed &&
          verified.nested_verifiers_replayed &&
          verified.external_clock_authority_replayed &&
          !verified.external_binding_authority_replayed &&
          verified
              .conditional_on_caller_external_binding_authority_replay &&
          verified.conditional_on_caller_strict_pre_lot_orchestration &&
          verified.conditional_on_caller_external_freeze_synchronization &&
          verified.in_memory_replay_only &&
          !verified.crash_durable,
      "10.13 fresh verification composes localization, AUTH and one prefix sweep while retaining external premises");

  const auto quotient_verified =
      verify_exact_direct_sparse_gateway_historical_quotient(
          authorities,
          temporal_budget,
          prefix_sweep_budget,
          result,
          historical_quotient_budget,
          historical_quotient);
  check(
      quotient_verified.result_certified &&
          quotient_verified.temporal_resolution_freshly_replayed &&
          quotient_verified.expected_result_freshly_reconstructed &&
          quotient_verified.observed_result_recursively_equal &&
          quotient_verified.batch_local_typed_closure_freshly_replayed &&
          quotient_verified.external_clock_authority_replayed &&
          !quotient_verified.external_binding_authority_replayed &&
          quotient_verified
              .conditional_on_caller_external_binding_authority_replay &&
          quotient_verified
              .conditional_on_caller_strict_pre_lot_orchestration &&
          quotient_verified
              .conditional_on_caller_external_freeze_synchronization &&
          quotient_verified.in_memory_replay_only &&
          !quotient_verified.crash_durable,
      "10.14 fresh verification replays 10.13 and the batch-local typed closure while retaining external premises");

  auto mutated_quotient = historical_quotient;
  ++mutated_quotient.candidate_to_component.front()
        .historical_component_index;
  const auto mutated_quotient_verification =
      verify_exact_direct_sparse_gateway_historical_quotient(
          authorities,
          temporal_budget,
          prefix_sweep_budget,
          result,
          historical_quotient_budget,
          mutated_quotient);
  check(
      !mutated_quotient_verification.observed_result_recursively_equal &&
          !mutated_quotient_verification.result_certified,
      "10.14 fresh verification rejects one mutated candidate-to-component binding");
}

}  // namespace

int main() {
  const SourceFixture fixture = source_fixture();
  test_identity_layout_golden_mutations_and_caps(fixture);
  test_certificate_digest_layout_and_golden();
  test_empty_source_batches_keep_locator_clock_conditional();
  test_conditional_clock_mapping_mutations_and_caps(fixture);
  test_in_memory_clock_authority_replay(fixture);
  test_historical_gateway_temporal_resolution(fixture);
  if (failures != 0) {
    std::cerr << failures << " direct sparse gateway clock test(s) failed\n";
    return 1;
  }
  std::cout << "direct sparse gateway clock tests passed\n";
  return 0;
}
