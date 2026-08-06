#include "morsehgp3d/hierarchy/direct_morse_tower_producer.hpp"

#include <new>
#include <utility>

namespace morsehgp3d::hierarchy {

bool ExactDirectMorseTowerResult::certified_tower() const noexcept {
  return tower.has_value() &&
         receipt.decision ==
             ExactDirectMorseTowerDecision::complete_exact_tower &&
         receipt.schema_version ==
             direct_morse_tower_producer_schema_version &&
         receipt.cloud_canonical && receipt.pair_stream_complete &&
         receipt.higher_stream_complete &&
         receipt.facade_terminal_catalog_certified &&
         receipt.event_journal_certified &&
         receipt.seed_journal_certified && receipt.single_backend_sealed &&
         !receipt.source_exactness_claimed &&
         !receipt.public_status_claimed;
}

ExactDirectMorseTowerResult build_exact_direct_morse_tower_from_cloud(
    std::span<const exact::CertifiedPoint3> points,
    std::size_t requested_maximum_order,
    ExactDirectMorseTowerBackendKind backend,
    const ExactDirectMorseTowerBudget& budget) {
  ExactDirectMorseTowerResult output;
  output.receipt.backend = backend;
  output.receipt.point_count = points.size();
  output.receipt.requested_maximum_order = requested_maximum_order;
  if (backend != ExactDirectMorseTowerBackendKind::exhaustive_host_v1) {
    output.receipt.decision =
        ExactDirectMorseTowerDecision::no_backend_rejected;
    return output;
  }
  output.receipt.single_backend_sealed = true;
  try {
    std::optional<spatial::CanonicalPointCloud> cloud;
    try {
      cloud.emplace(
          spatial::CanonicalPointCloud::rejecting_duplicates(points));
    } catch (const std::bad_alloc&) {
      throw;
    } catch (...) {
      output.receipt.decision =
          ExactDirectMorseTowerDecision::no_cloud_rejected;
      return output;
    }
    output.receipt.cloud_canonical = true;
    auto index = spatial::MortonLbvhIndex::build(*cloud);

    auto pair = build_exact_pair_support_stream(
        index, *cloud, requested_maximum_order, budget.terminal.pair);
    output.receipt.pair_event_count = pair.events.size();
    output.receipt.pair_stream_complete = pair.stream_complete();
    if (!output.receipt.pair_stream_complete) {
      output.receipt.decision =
          ExactDirectMorseTowerDecision::no_pair_stream_rejected;
      return output;
    }

    auto higher = build_exact_higher_support_stream(
        index, *cloud, requested_maximum_order, budget.terminal.higher);
    output.receipt.higher_event_count = higher.events.size();
    output.receipt.higher_stream_complete = higher.stream_complete();
    if (!output.receipt.higher_stream_complete) {
      output.receipt.decision =
          ExactDirectMorseTowerDecision::no_higher_stream_rejected;
      return output;
    }

    auto facade = build_exact_direct_support_terminal_facade(
        index,
        *cloud,
        requested_maximum_order,
        budget.terminal,
        pair,
        higher);
    output.receipt.facade_event_count = facade.events.size();
    output.receipt.higher_canonical_cloud_digest =
        facade.certificate.higher_canonical_cloud_digest;
    output.receipt.facade_terminal_catalog_certified =
        facade.terminal_catalog_certified();
    if (!output.receipt.facade_terminal_catalog_certified) {
      output.receipt.decision =
          ExactDirectMorseTowerDecision::no_facade_rejected;
      return output;
    }

    auto event_journal =
        build_exact_direct_morse_event_journal(*cloud, facade);
    output.receipt.morse_event_projection_count =
        event_journal.materialized_direct_event_projections.size();
    output.receipt.event_journal_certified =
        event_journal.certified_partial_refinement();
    if (!output.receipt.event_journal_certified) {
      output.receipt.decision =
          ExactDirectMorseTowerDecision::no_event_journal_rejected;
      return output;
    }

    auto seed_journal = build_exact_direct_saddle_arm_seed_journal(
        *cloud, facade, event_journal, budget.seeds);
    output.receipt.saddle_arm_seed_count = seed_journal.arm_seeds.size();
    output.receipt.seed_journal_certified =
        seed_journal.certified_partial_refinement();
    if (!output.receipt.seed_journal_certified) {
      output.receipt.decision =
          ExactDirectMorseTowerDecision::no_seed_journal_rejected;
      return output;
    }

    output.tower.emplace(ExactDirectMorseTower{
        std::move(*cloud),
        std::move(index),
        std::move(pair),
        std::move(higher),
        std::move(facade),
        std::move(event_journal),
        std::move(seed_journal)});
    output.receipt.decision =
        ExactDirectMorseTowerDecision::complete_exact_tower;
    return output;
  } catch (const std::bad_alloc&) {
    output.tower.reset();
    output.receipt.decision =
        ExactDirectMorseTowerDecision::no_allocation_failed;
    return output;
  } catch (...) {
    output.tower.reset();
    output.receipt.decision =
        ExactDirectMorseTowerDecision::no_higher_stream_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
