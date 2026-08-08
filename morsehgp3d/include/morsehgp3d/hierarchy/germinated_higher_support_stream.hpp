#pragma once

// The higher-support stream driven by the local germination proposal generator
// instead of by a subdivision of the product universe.
//
// WHY THIS EXISTS.  The product subdivision does not prune.  Measured on the
// sealed sweep of 8 August 2026, a prune certificate covers 1.105 support
// tuples at n=128 and between 1.05 and 1.13 at every size from n=12, while the
// traversal visits 1.83 U nodes where a binary tree over U leaves has 2U-1.
// That is not a tuning defect: both block gates are one-sided, so neither can
// decide a coarse product that contains well-centred and non-well-centred
// tuples at once -- and the well-centred fraction is constant in n, so a coarse
// product contains both almost surely.  The subdivision therefore cannot be
// output-sensitive, by the same argument that already removed the well-centring
// gate as a source of sensitivity.
//
// WHAT REPLACES IT.  `generate_local_germination_candidates` emits candidate
// supports keyed on a diametral pair.  Jung proves the disc-and-segment geometry
// but the current binary64 rejection path does not yet certify completeness.
// This component is its consumer: it classifies every emission with the same exact terminal
// classification the exhaustive stream uses, tallies the production audit
// independently of the producer's own counters, and deduplicates the accepted
// events.
//
// WHAT IT DOES NOT CARRY.  There is no mass partition here.  The universe is
// never enumerated, so R + C(F) = C(n,3) + C(n,4) is neither produced nor
// available, and nothing in this file pretends otherwise.  What a consumer can
// still check is the identity of PRODUCTION, and it is checked against this
// consumer's own tally rather than against the producer's counters.

#include "morsehgp3d/exact/rational.hpp"
#include "morsehgp3d/hierarchy/local_germination.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <array>
#include <compare>
#include <cstddef>
#include <optional>
#include <vector>

namespace morsehgp3d::hierarchy {

// One accepted minimal well-centred support, in the canonical form the
// exhaustive stream emits: ids strictly increasing, arity, and the closed rank
// the exact closed-ball query observed.
struct GerminatedHigherSupportEvent {
  std::array<spatial::PointId, 4> support_ids{};
  std::size_t support_size{};
  std::size_t observed_closed_rank{};

  friend auto operator<=>(
      const GerminatedHigherSupportEvent&,
      const GerminatedHigherSupportEvent&) = default;
  friend bool operator==(
      const GerminatedHigherSupportEvent&,
      const GerminatedHigherSupportEvent&) = default;
};

// Why each classified support left the classifier.  The categories are the
// ones the exhaustive census publishes, so the two are comparable term by term.
struct GerminatedHigherSupportClassificationCounters {
  std::size_t classified{};
  std::size_t affinely_dependent{};
  std::size_t boundary_reduced{};
  std::size_t exterior_circumcenter{};
  std::size_t above_rank{};
  std::size_t extra_shell{};
  std::size_t accepted{};
  std::size_t closed_ball_query_count{};
  std::size_t closed_ball_node_visit_count{};
};

// The single exact classification of one canonical support.  Both the
// germinated generator and any exhaustive reference must call THIS function,
// so that a differential between them isolates the candidate source and
// nothing else.
//
// `support_ids` must hold `support_size` ids in strictly increasing order in
// its first entries; `support_size` must be three or four.  Returns the event
// when the support is accepted, and nothing otherwise; `counters` is advanced
// in every case.
[[nodiscard]] std::optional<GerminatedHigherSupportEvent>
classify_exact_higher_support(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    const std::array<spatial::PointId, 4>& support_ids,
    std::size_t support_size,
    std::size_t maximum_relevant_closed_rank,
    std::size_t traversal_frontier_bound,
    GerminatedHigherSupportClassificationCounters& counters);

struct GerminatedHigherSupportResult {
  // Accepted supports, deduplicated and in canonical order.
  std::vector<GerminatedHigherSupportEvent> events;
  // Index 0 is arity three, index 1 arity four.
  std::array<LocalGerminationCertificate, 2> certificates{};
  // Tallied by this consumer at the sink boundary, never copied from the
  // producer -- that independence is what makes the production identity say
  // something.
  std::array<LocalGerminationProductionAudit, 2> audits{};
  GerminatedHigherSupportClassificationCounters counters{};
  // Emissions that named a support already accepted.  The generator owns a
  // support once per pair realising its diameter, so this is expected to be
  // small and non-zero, and it is published rather than hidden.
  std::size_t duplicate_accepted_emissions{};
  // True only when every EXECUTED arity carries an admissible certificate whose
  // production identity holds against `audits`.  A non-applicable arity has no
  // production to certify.  An applicable arity skipped after an earlier
  // operational deadline is explicitly `executed=false`; it likewise has no
  // production identity, and its certificate cannot claim completeness.
  // False leaves `refusal_reason` naming the contract that bit.
  bool production_identity_holds{};
  std::string refusal_reason;
  // True when the operational guard cut a seed loop short.  The events below
  // are then a PREFIX of what the configuration produces, not its whole
  // output, and no completeness claim survives -- the certificates say so
  // themselves.  The production identity still holds: it is an identity on the
  // accounting, and a censured run accounts for exactly what it produced.
  bool censored_by_operational_deadline{};
};

// Runs the generator for arity three and arity four, classifies every emission
// exactly, and returns the accepted events.
//
// `config.maximum_relevant_closed_rank` is overwritten with
// `maximum_relevant_closed_rank` so the generator and the classifier cannot
// disagree about s_max.
[[nodiscard]] GerminatedHigherSupportResult
build_germinated_higher_support_stream(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t maximum_relevant_closed_rank,
    LocalGerminationConfig config,
    const LocalGerminationGuard& guard = LocalGerminationGuard{});

// The exhaustive reference: every subset of size three and four, classified by
// the same `classify_exact_higher_support`.  Its only purpose is to be the
// other side of a differential, and its cost is the universe, so it is for
// small clouds.
[[nodiscard]] std::vector<GerminatedHigherSupportEvent>
enumerate_exhaustive_higher_support_events(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t maximum_relevant_closed_rank,
    GerminatedHigherSupportClassificationCounters& counters);

}  // namespace morsehgp3d::hierarchy
