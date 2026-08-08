#include "morsehgp3d/hierarchy/germinated_higher_support_stream.hpp"

#include "morsehgp3d/exact/integer_circumcenter.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/hierarchy/higher_support_closed_ball.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

using exact::CircumcenterSupportStatus;
using exact::ExactRational3;
using exact::IntegerCircumcenterAnalysis;
using exact::analyze_circumcenter_support_integer;
using spatial::CanonicalPointCloud;
using spatial::MortonLbvhIndex;
using spatial::PointId;

// The arity-dependent half of the classification, kept as a template for the
// same reason the exhaustive census keeps one: the circumcentre analysis takes
// a fixed-size span and the two arities must not share a runtime branch that
// could silently classify a triangle as a tetrahedron.
template <std::size_t SupportSize>
[[nodiscard]] std::optional<GerminatedHigherSupportEvent> classify_sized(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const std::array<PointId, 4>& support_ids,
    std::size_t maximum_relevant_closed_rank,
    std::size_t traversal_frontier_bound,
    GerminatedHigherSupportClassificationCounters& counters) {
  std::array<ExactRational3, SupportSize> support_points{};
  for (std::size_t position = 0U; position < SupportSize; ++position) {
    support_points[position] = cloud.point(support_ids[position]).exact();
  }

  const IntegerCircumcenterAnalysis analysis =
      analyze_circumcenter_support_integer(support_points);
  switch (analysis.status) {
    case CircumcenterSupportStatus::affinely_dependent:
      counters.affinely_dependent += 1U;
      return std::nullopt;
    case CircumcenterSupportStatus::boundary_reduced:
      counters.boundary_reduced += 1U;
      return std::nullopt;
    case CircumcenterSupportStatus::exterior_circumcenter:
      counters.exterior_circumcenter += 1U;
      return std::nullopt;
    case CircumcenterSupportStatus::minimal:
      break;
  }
  if (!analysis.center.has_value() || !analysis.squared_level.has_value()) {
    throw std::logic_error(
        "a minimal higher support omitted its exact sphere");
  }

  const ExactHigherSupportClosedBallClassification classification =
      ExactHigherSupportIndexedClosedBallQuery::classify(
          index,
          cloud,
          support_ids,
          SupportSize,
          *analysis.center,
          *analysis.squared_level,
          maximum_relevant_closed_rank - SupportSize,
          traversal_frontier_bound);
  counters.closed_ball_query_count += 1U;
  counters.closed_ball_node_visit_count +=
      classification.counters.node_visit_count;

  if (classification.outcome ==
      ExactHigherSupportClosedBallOutcome::rank_exceeded) {
    counters.above_rank += 1U;
    return std::nullopt;
  }
  if (classification.shell_count != SupportSize) {
    counters.extra_shell += 1U;
    return std::nullopt;
  }

  counters.accepted += 1U;
  GerminatedHigherSupportEvent event;
  event.support_ids = support_ids;
  for (std::size_t position = SupportSize; position < event.support_ids.size();
       ++position) {
    event.support_ids[position] = PointId{};
  }
  event.support_size = SupportSize;
  event.observed_closed_rank =
      classification.interior_ids.size() + classification.shell_count;
  return event;
}

}  // namespace

std::optional<GerminatedHigherSupportEvent> classify_exact_higher_support(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    const std::array<PointId, 4>& support_ids,
    std::size_t support_size,
    std::size_t maximum_relevant_closed_rank,
    std::size_t traversal_frontier_bound,
    GerminatedHigherSupportClassificationCounters& counters) {
  if (support_size != 3U && support_size != 4U) {
    throw std::invalid_argument(
        "an exact higher support has three or four vertices");
  }
  if (maximum_relevant_closed_rank < support_size) {
    throw std::invalid_argument(
        "a higher support cannot exceed its own closed rank ceiling");
  }
  for (std::size_t position = 1U; position < support_size; ++position) {
    if (!(support_ids[position - 1U] < support_ids[position])) {
      throw std::invalid_argument(
          "an exact higher support requires strictly increasing ids");
    }
  }

  counters.classified += 1U;
  if (support_size == 3U) {
    return classify_sized<3U>(
        index,
        cloud,
        support_ids,
        maximum_relevant_closed_rank,
        traversal_frontier_bound,
        counters);
  }
  return classify_sized<4U>(
      index,
      cloud,
      support_ids,
      maximum_relevant_closed_rank,
      traversal_frontier_bound,
      counters);
}

GerminatedHigherSupportResult build_germinated_higher_support_stream(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_relevant_closed_rank,
    LocalGerminationConfig config) {
  // One s_max for the generator and the classifier, so the two cannot disagree
  // about which supports are relevant.
  config.maximum_relevant_closed_rank = maximum_relevant_closed_rank;

  GerminatedHigherSupportResult result;
  const std::size_t traversal_frontier_bound =
      ExactHigherSupportIndexedClosedBallQuery::proved_traversal_frontier_bound(
          index);

  for (std::size_t arity_index = 0U; arity_index < 2U; ++arity_index) {
    const std::size_t support_size = arity_index + 3U;
    if (maximum_relevant_closed_rank < support_size) {
      // An order too small to admit this arity at all: the generator is not
      // asked, and the audit stays at zero rather than being invented.
      continue;
    }
    LocalGerminationProductionAudit& audit = result.audits[arity_index];

    const LocalGerminationSink sink =
        [&](const std::array<PointId, 4>& support_ids,
            std::size_t emitted_support_size) {
          // Tallied HERE, at the consumer's boundary, from what the consumer
          // actually received.  Copying the producer's counters would make the
          // identity a tautology.
          if (emitted_support_size == 3U) {
            audit.observed_triple_emissions += 1U;
          } else {
            audit.observed_quadruple_emissions += 1U;
          }
          const std::optional<GerminatedHigherSupportEvent> event =
              classify_exact_higher_support(
                  index,
                  cloud,
                  support_ids,
                  emitted_support_size,
                  maximum_relevant_closed_rank,
                  traversal_frontier_bound,
                  result.counters);
          if (event.has_value()) {
            audit.accepted_supports += 1U;
            result.events.push_back(*event);
          }
        };

    result.certificates[arity_index] = generate_local_germination_candidates(
        index, cloud, support_size, config, sink);
  }

  // The generator owns a support once per pair realising its diameter, so an
  // accepted support can arrive more than once.  Deduplicating here is the
  // consumer's job, as the generator's contract says.
  const std::size_t accepted_emissions = result.events.size();
  std::sort(result.events.begin(), result.events.end());
  result.events.erase(
      std::unique(result.events.begin(), result.events.end()),
      result.events.end());
  result.duplicate_accepted_emissions =
      accepted_emissions - result.events.size();

  result.production_identity_holds = true;
  for (std::size_t arity_index = 0U; arity_index < 2U; ++arity_index) {
    const LocalGerminationCertificate& certificate =
        result.certificates[arity_index];
    if (certificate.proof_basis.empty()) {
      // This arity was never generated; there is nothing to certify and
      // nothing to claim.
      continue;
    }
    std::string reason;
    if (!local_germination_certificate_admissible(certificate, reason)) {
      result.production_identity_holds = false;
      result.refusal_reason = std::move(reason);
      return result;
    }
    if (!local_germination_production_identity_holds(
            certificate, result.audits[arity_index], reason)) {
      result.production_identity_holds = false;
      result.refusal_reason = std::move(reason);
      return result;
    }
  }
  return result;
}

std::vector<GerminatedHigherSupportEvent>
enumerate_exhaustive_higher_support_events(
    const MortonLbvhIndex& index,
    const CanonicalPointCloud& cloud,
    std::size_t maximum_relevant_closed_rank,
    GerminatedHigherSupportClassificationCounters& counters) {
  const std::size_t point_count = cloud.size();
  const std::size_t traversal_frontier_bound =
      ExactHigherSupportIndexedClosedBallQuery::proved_traversal_frontier_bound(
          index);
  std::vector<GerminatedHigherSupportEvent> events;
  std::array<PointId, 4> support_ids{};

  const auto classify_one =
      [&](std::size_t support_size) {
        const std::optional<GerminatedHigherSupportEvent> event =
            classify_exact_higher_support(
                index,
                cloud,
                support_ids,
                support_size,
                maximum_relevant_closed_rank,
                traversal_frontier_bound,
                counters);
        if (event.has_value()) {
          events.push_back(*event);
        }
      };

  for (std::size_t first = 0U; first + 2U < point_count; ++first) {
    support_ids[0] = PointId{first};
    for (std::size_t second = first + 1U; second + 1U < point_count;
         ++second) {
      support_ids[1] = PointId{second};
      for (std::size_t third = second + 1U; third < point_count; ++third) {
        support_ids[2] = PointId{third};
        support_ids[3] = PointId{};
        if (maximum_relevant_closed_rank >= 3U) {
          classify_one(3U);
        }
        if (maximum_relevant_closed_rank < 4U) {
          continue;
        }
        for (std::size_t fourth = third + 1U; fourth < point_count;
             ++fourth) {
          support_ids[3] = PointId{fourth};
          classify_one(4U);
        }
        support_ids[3] = PointId{};
      }
    }
  }

  std::sort(events.begin(), events.end());
  return events;
}

}  // namespace morsehgp3d::hierarchy
