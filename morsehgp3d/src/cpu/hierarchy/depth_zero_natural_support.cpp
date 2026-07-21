#include "morsehgp3d/hierarchy/depth_zero_natural_support.hpp"

#include "morsehgp3d/spatial/brute_force.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

using spatial::PointId;

struct ProposedSupport {
  std::vector<PointId> point_ids;
  std::size_t first_contact_index{};
};

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{message});
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(std::string{message});
  }
  return left * right;
}

[[nodiscard]] std::size_t bounded_binomial(
    std::size_t population,
    std::size_t selection) {
  if (selection > population) {
    return 0U;
  }
  selection = std::min(selection, population - selection);
  std::size_t value = 1U;
  for (std::size_t factor = 1U; factor <= selection; ++factor) {
    value = checked_multiply(
        value,
        population - selection + factor,
        "the natural-support binomial coefficient overflows size_t");
    value /= factor;
  }
  return value;
}

void validate_domain(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::StrictlyPaddedDyadicAabb3Result& clipping_box,
    std::size_t requested_maximum_order,
    const ExactDepthZeroNaturalSupportBudget& budget) {
  if (cloud.size() <
          ExactDepthZeroNaturalSupportResult::
              minimum_supported_point_count ||
      cloud.size() >
          ExactDepthZeroNaturalSupportResult::
              maximum_supported_point_count) {
    throw std::invalid_argument(
        "the bounded natural-support extractor requires 1<=n<=8");
  }
  if (requested_maximum_order <
          ExactDepthZeroNaturalSupportResult::
              minimum_supported_maximum_order ||
      requested_maximum_order >
          ExactDepthZeroNaturalSupportResult::
              maximum_supported_maximum_order) {
    throw std::invalid_argument(
        "the bounded natural-support extractor requires 1<=Kmax<=10");
  }
  if (budget.maximum_source_contact_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_source_contact_count ||
      budget.maximum_raw_support_proposal_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_raw_support_proposal_count ||
      budget.maximum_raw_support_point_id_reference_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_raw_support_point_id_reference_count ||
      budget.maximum_unique_support_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_unique_support_count ||
      budget.maximum_unique_support_point_id_reference_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_unique_support_point_id_reference_count ||
      budget.maximum_point_classification_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_point_classification_count) {
    throw std::invalid_argument(
        "a natural-support extraction budget exceeds its trusted cap");
  }
  const spatial::StrictlyPaddedDyadicAabb3Verification box_verification =
      spatial::verify_strictly_padded_dyadic_aabb(cloud, clipping_box);
  if (!box_verification.result_certified ||
      clipping_box.decision !=
          spatial::StrictDyadicPaddingDecision::complete ||
      !clipping_box.certificate.has_value()) {
    throw std::invalid_argument(
        "natural-support extraction requires a freshly certified clipping box");
  }
}

[[nodiscard]] ExactDepthZeroNaturalSupportRequirements requirements_for(
    std::size_t point_count,
    std::size_t requested_maximum_order) {
  ExactDepthZeroNaturalSupportRequirements requirements;
  requirements.point_count = point_count;
  requirements.effective_maximum_order =
      std::min(requested_maximum_order, point_count);
  requirements.maximum_relevant_support_rank = std::min(
      checked_add(
          requirements.effective_maximum_order,
          1U,
          "the natural-support relevant rank overflows size_t"),
      point_count);

  for (std::size_t carrier_size = 2U;
       carrier_size <= point_count;
       ++carrier_size) {
    const std::size_t carrier_count =
        bounded_binomial(point_count, carrier_size);
    requirements.conservative_source_contact_count = checked_add(
        requirements.conservative_source_contact_count,
        carrier_count,
        "the natural-support source-contact count overflows size_t");
    const std::size_t maximum_support_size =
        std::min(std::size_t{4U}, carrier_size);
    for (std::size_t support_size = 2U;
         support_size <= maximum_support_size;
         ++support_size) {
      const std::size_t proposals_per_carrier =
          bounded_binomial(carrier_size, support_size);
      const std::size_t proposals = checked_multiply(
          carrier_count,
          proposals_per_carrier,
          "the natural-support proposal count overflows size_t");
      requirements.conservative_raw_support_proposal_count = checked_add(
          requirements.conservative_raw_support_proposal_count,
          proposals,
          "the natural-support proposal sum overflows size_t");
      requirements.conservative_raw_support_point_id_reference_count =
          checked_add(
              requirements
                  .conservative_raw_support_point_id_reference_count,
              checked_multiply(
                  proposals,
                  support_size,
                  "the natural-support raw ID count overflows size_t"),
              "the natural-support raw ID sum overflows size_t");
    }
  }

  for (std::size_t support_size = 2U;
       support_size <= std::min(std::size_t{4U}, point_count);
       ++support_size) {
    const std::size_t count =
        bounded_binomial(point_count, support_size);
    requirements.conservative_unique_support_count = checked_add(
        requirements.conservative_unique_support_count,
        count,
        "the natural-support unique-support count overflows size_t");
    requirements.conservative_unique_support_point_id_reference_count =
        checked_add(
            requirements
                .conservative_unique_support_point_id_reference_count,
            checked_multiply(
                count,
                support_size,
                "the natural-support unique ID count overflows size_t"),
            "the natural-support unique ID sum overflows size_t");
  }
  requirements.conservative_point_classification_count = checked_multiply(
      requirements.conservative_unique_support_count,
      point_count,
      "the natural-support point-classification count overflows size_t");

  if (requirements.conservative_source_contact_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_source_contact_count ||
      requirements.conservative_raw_support_proposal_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_raw_support_proposal_count ||
      requirements.conservative_raw_support_point_id_reference_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_raw_support_point_id_reference_count ||
      requirements.conservative_unique_support_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_unique_support_count ||
      requirements.conservative_unique_support_point_id_reference_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_unique_support_point_id_reference_count ||
      requirements.conservative_point_classification_count >
          ExactDepthZeroNaturalSupportBudget::
              trusted_maximum_point_classification_count) {
    throw std::logic_error(
        "a natural-support requirement exceeded its proved n<=8 cap");
  }
  return requirements;
}

[[nodiscard]] bool budget_covers(
    const ExactDepthZeroNaturalSupportBudget& budget,
    const ExactDepthZeroNaturalSupportRequirements& requirements) {
  return budget.maximum_source_contact_count >=
             requirements.conservative_source_contact_count &&
         budget.maximum_raw_support_proposal_count >=
             requirements.conservative_raw_support_proposal_count &&
         budget.maximum_raw_support_point_id_reference_count >=
             requirements
                 .conservative_raw_support_point_id_reference_count &&
         budget.maximum_unique_support_count >=
             requirements.conservative_unique_support_count &&
         budget.maximum_unique_support_point_id_reference_count >=
             requirements
                 .conservative_unique_support_point_id_reference_count &&
         budget.maximum_point_classification_count >=
             requirements.conservative_point_classification_count;
}

[[nodiscard]] bool is_natural_contact_kind(
    spatial::ExactOrdinaryDiagramContactKind kind) {
  return kind == spatial::ExactOrdinaryDiagramContactKind::natural_face ||
         kind == spatial::ExactOrdinaryDiagramContactKind::natural_edge ||
         kind == spatial::ExactOrdinaryDiagramContactKind::natural_vertex;
}

[[nodiscard]] spatial::ExactOrdinaryDiagramContactKind
expected_natural_kind(std::size_t support_size) {
  switch (support_size) {
    case 2U:
      return spatial::ExactOrdinaryDiagramContactKind::natural_face;
    case 3U:
      return spatial::ExactOrdinaryDiagramContactKind::natural_edge;
    case 4U:
      return spatial::ExactOrdinaryDiagramContactKind::natural_vertex;
    default:
      throw std::logic_error(
          "a natural support must contain two to four sites");
  }
}

[[nodiscard]] bool source_verification_complete(
    const spatial::ExactOrdinaryDiagramClosureVerification& verification,
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram) {
  return source_diagram.decision ==
             spatial::ExactOrdinaryDiagramClosureDecision::complete &&
         verification.input_identity_certified &&
         verification.clipping_box_certified &&
         verification.decision_certified &&
         verification.requirements_certified &&
         verification.audit_certified &&
         verification.payload_shape_certified &&
         verification.transcript_replay_certified &&
         verification.all_cells_freshly_verified_certified &&
         verification.all_local_queues_empty_certified &&
         verification.all_cells_full_dimensional_nonempty_certified &&
         verification.global_vertex_occurrence_bijection_certified &&
         verification.natural_incidences_reconciled_certified &&
         verification.artificial_box_boundaries_certified &&
         verification.result_certified;
}

void enumerate_fixed_size_subsets(
    std::span<const PointId> carrier,
    std::size_t target_size,
    std::size_t start,
    std::vector<PointId>& current,
    std::size_t contact_index,
    std::map<std::vector<PointId>, std::size_t>& first_contacts,
    ExactDepthZeroNaturalSupportAudit& audit) {
  if (current.size() == target_size) {
    audit.raw_support_proposal_count = checked_add(
        audit.raw_support_proposal_count,
        1U,
        "the observed natural-support proposal count overflows size_t");
    audit.raw_support_point_id_reference_count = checked_add(
        audit.raw_support_point_id_reference_count,
        target_size,
        "the observed natural-support raw ID count overflows size_t");
    const auto [position, inserted] = first_contacts.emplace(
        current, contact_index);
    if (!inserted && contact_index < position->second) {
      position->second = contact_index;
    }
    return;
  }
  const std::size_t remaining = target_size - current.size();
  if (carrier.size() < remaining || start > carrier.size() - remaining) {
    return;
  }
  const std::size_t last = carrier.size() - remaining;
  for (std::size_t index = start; index <= last; ++index) {
    current.push_back(carrier[index]);
    enumerate_fixed_size_subsets(
        carrier,
        target_size,
        index + 1U,
        current,
        contact_index,
        first_contacts,
        audit);
    current.pop_back();
  }
}

[[nodiscard]] std::vector<ProposedSupport> collect_proposals(
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram,
    const ExactDepthZeroNaturalSupportRequirements& requirements,
    ExactDepthZeroNaturalSupportAudit& audit) {
  audit.source_contact_count = source_diagram.contacts.size();
  if (audit.source_contact_count >
      requirements.conservative_source_contact_count) {
    throw std::logic_error(
        "the source diagram exceeded the support-extraction contact preflight");
  }
  std::map<std::vector<PointId>, std::size_t> first_contacts;
  for (std::size_t contact_index = 0U;
       contact_index < source_diagram.contacts.size();
       ++contact_index) {
    const spatial::ExactOrdinaryDiagramContact& contact =
        source_diagram.contacts[contact_index];
    if (!is_natural_contact_kind(contact.kind)) {
      continue;
    }
    ++audit.natural_carrier_contact_count;
    if (contact.query_ids != contact.carrier_shell_ids ||
        contact.common_artificial_box_face_mask != 0U ||
        contact.carrier_shell_ids.size() < 2U) {
      throw std::logic_error(
          "a certified natural carrier violates its canonical contact invariants");
    }
    const std::size_t maximum_support_size = std::min(
        std::size_t{4U}, contact.carrier_shell_ids.size());
    for (std::size_t support_size = 2U;
         support_size <= maximum_support_size;
         ++support_size) {
      std::vector<PointId> current;
      current.reserve(support_size);
      enumerate_fixed_size_subsets(
          contact.carrier_shell_ids,
          support_size,
          0U,
          current,
          contact_index,
          first_contacts,
          audit);
    }
  }
  const std::size_t expected_natural_carrier_count = checked_add(
      checked_add(
          source_diagram.audit.natural_face_count,
          source_diagram.audit.natural_edge_count,
          "the source natural-contact count overflows size_t"),
      source_diagram.audit.natural_vertex_count,
      "the source natural-contact count overflows size_t");
  if (audit.natural_carrier_contact_count !=
      expected_natural_carrier_count) {
    throw std::logic_error(
        "the source natural-contact audit contradicts its carriers");
  }
  if (audit.raw_support_proposal_count >
          requirements.conservative_raw_support_proposal_count ||
      audit.raw_support_point_id_reference_count >
          requirements
              .conservative_raw_support_point_id_reference_count) {
    throw std::logic_error(
        "natural carriers exceeded the support-proposal preflight");
  }

  std::vector<ProposedSupport> proposals;
  proposals.reserve(first_contacts.size());
  for (const auto& [point_ids, contact_index] : first_contacts) {
    proposals.push_back(ProposedSupport{point_ids, contact_index});
  }
  std::sort(
      proposals.begin(),
      proposals.end(),
      [](const ProposedSupport& left, const ProposedSupport& right) {
        if (left.point_ids.size() != right.point_ids.size()) {
          return left.point_ids.size() < right.point_ids.size();
        }
        return left.point_ids < right.point_ids;
      });
  audit.unique_support_count = proposals.size();
  for (const ProposedSupport& proposal : proposals) {
    audit.unique_support_point_id_reference_count = checked_add(
        audit.unique_support_point_id_reference_count,
        proposal.point_ids.size(),
        "the observed natural-support unique ID count overflows size_t");
  }
  if (audit.unique_support_count >
          requirements.conservative_unique_support_count ||
      audit.unique_support_point_id_reference_count >
          requirements
              .conservative_unique_support_point_id_reference_count) {
    throw std::logic_error(
        "natural carriers exceeded the unique-support preflight");
  }
  return proposals;
}

void copy_sphere_witness(
    const exact::CircumcenterSupportAnalysis& analysis,
    ExactDepthZeroNaturalSupportCandidate& candidate) {
  const exact::CircumcenterResult& sphere =
      analysis.circumcenter_result();
  if (sphere.kind() != exact::CircumcenterKind::unique ||
      !sphere.center().has_value() ||
      !sphere.squared_level().has_value() ||
      !analysis.barycentric().has_value()) {
    throw std::logic_error(
        "an affinely independent natural support omitted its exact witnesses");
  }
  candidate.center = *sphere.center();
  candidate.squared_level = *sphere.squared_level();
  const exact::BarycentricCoordinates& barycentric =
      *analysis.barycentric();
  if (barycentric.support_size() !=
      candidate.support_point_ids.size()) {
    throw std::logic_error(
        "a natural-support barycentric witness has the wrong size");
  }
  candidate.support_barycentric_coordinates.reserve(
      barycentric.support_size());
  candidate.support_barycentric_signs.reserve(
      barycentric.support_size());
  for (std::size_t index = 0U;
       index < barycentric.support_size();
       ++index) {
    candidate.support_barycentric_coordinates.push_back(
        barycentric.coordinate(index));
    candidate.support_barycentric_signs.push_back(
        barycentric.sign(index));
  }
}

[[nodiscard]] bool is_subset(
    std::span<const PointId> subset,
    std::span<const PointId> superset) {
  return std::includes(
      superset.begin(), superset.end(), subset.begin(), subset.end());
}

[[nodiscard]] ExactDepthZeroNaturalSupportPredicateCounters
predicate_snapshot(
    const exact::PredicateCounters& counters) {
  return ExactDepthZeroNaturalSupportPredicateCounters{
      counters.fp64_filtered_certified(),
      counters.expansion_certified(),
      counters.cpu_multiprecision_certified(),
      counters.exact_zeros(),
      counters.remaining_unknown()};
}

[[nodiscard]] std::map<std::vector<PointId>, std::size_t>
natural_contact_indices(
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram) {
  std::map<std::vector<PointId>, std::size_t> result;
  for (std::size_t index = 0U;
       index < source_diagram.contacts.size();
       ++index) {
    const spatial::ExactOrdinaryDiagramContact& contact =
        source_diagram.contacts[index];
    if (!is_natural_contact_kind(contact.kind)) {
      continue;
    }
    const auto [unused, inserted] = result.emplace(
        contact.carrier_shell_ids, index);
    static_cast<void>(unused);
    if (!inserted) {
      throw std::logic_error(
          "a source diagram contains duplicate natural carrier shells");
    }
  }
  return result;
}

[[nodiscard]] std::size_t require_natural_carrier(
    const std::vector<PointId>& shell_point_ids,
    const std::map<std::vector<PointId>, std::size_t>& natural_contacts) {
  const auto found = natural_contacts.find(shell_point_ids);
  if (found == natural_contacts.end()) {
    throw std::logic_error(
        "an empty-interior support has no complete natural carrier");
  }
  return found->second;
}

[[nodiscard]] bool same_diagnostic_identity(
    const ExactDepthZeroNaturalExtraShellDiagnostic& diagnostic,
    const ExactDepthZeroNaturalSupportCandidate& candidate,
    std::size_t carrier_contact_index) {
  return candidate.center.has_value() &&
         candidate.squared_level.has_value() &&
         diagnostic.center == *candidate.center &&
         diagnostic.squared_level == *candidate.squared_level &&
         diagnostic.shell_point_ids == candidate.shell_point_ids &&
         diagnostic.observed_closed_rank ==
             candidate.observed_closed_rank &&
         diagnostic.carrier_contact_index == carrier_contact_index;
}

[[nodiscard]] std::size_t append_diagnostic(
    ExactDepthZeroNaturalSupportResult& result,
    const ExactDepthZeroNaturalSupportCandidate& candidate,
    std::size_t carrier_contact_index) {
  auto found = std::find_if(
      result.relevant_extra_shell_diagnostics.begin(),
      result.relevant_extra_shell_diagnostics.end(),
      [&](const ExactDepthZeroNaturalExtraShellDiagnostic& diagnostic) {
        return same_diagnostic_identity(
            diagnostic, candidate, carrier_contact_index);
      });
  if (found == result.relevant_extra_shell_diagnostics.end()) {
    if (!candidate.center.has_value() ||
        !candidate.squared_level.has_value()) {
      throw std::logic_error(
          "an extra-shell support omitted its sphere witness");
    }
    ExactDepthZeroNaturalExtraShellDiagnostic diagnostic;
    diagnostic.diagnostic_index =
        result.relevant_extra_shell_diagnostics.size();
    diagnostic.center = *candidate.center;
    diagnostic.squared_level = *candidate.squared_level;
    diagnostic.shell_point_ids = candidate.shell_point_ids;
    diagnostic.observed_closed_rank = candidate.observed_closed_rank;
    diagnostic.carrier_contact_index = carrier_contact_index;
    result.relevant_extra_shell_diagnostics.push_back(std::move(diagnostic));
    found = std::prev(result.relevant_extra_shell_diagnostics.end());
  }
  found->support_point_id_sets.push_back(candidate.support_point_ids);
  found->source_candidate_indices.push_back(candidate.candidate_index);
  return found->diagnostic_index;
}

void append_accepted_support(
    ExactDepthZeroNaturalSupportResult& result,
    ExactDepthZeroNaturalSupportCandidate& candidate,
    const spatial::ExactOrdinaryDiagramContact& natural_contact,
    std::size_t natural_contact_index) {
  if (!candidate.center.has_value() ||
      !candidate.squared_level.has_value()) {
    throw std::logic_error(
        "an accepted natural support omitted its sphere witness");
  }
  const std::size_t expected_rank =
      candidate.support_point_ids.size() - 1U;
  const std::size_t expected_dimension =
      4U - candidate.support_point_ids.size();
  const spatial::ExactOrdinaryDiagramContactKind expected_kind =
      expected_natural_kind(candidate.support_point_ids.size());
  if (natural_contact.query_ids != candidate.support_point_ids ||
      natural_contact.carrier_shell_ids != candidate.support_point_ids ||
      natural_contact.kind != expected_kind ||
      natural_contact.site_affine_rank != expected_rank ||
      natural_contact.affine_dimension != expected_dimension ||
      natural_contact.common_artificial_box_face_mask != 0U ||
      candidate.shell_point_ids != candidate.support_point_ids ||
      candidate.observed_closed_rank !=
          candidate.support_point_ids.size()) {
    throw std::logic_error(
        "an accepted support contradicts its canonical natural contact");
  }
  ExactDepthZeroNaturalSupport support;
  support.support_index = result.supports.size();
  support.source_candidate_index = candidate.candidate_index;
  support.natural_contact_index = natural_contact_index;
  support.support_point_ids = candidate.support_point_ids;
  support.center = *candidate.center;
  support.squared_level = *candidate.squared_level;
  support.support_barycentric_coordinates =
      candidate.support_barycentric_coordinates;
  support.support_barycentric_signs =
      candidate.support_barycentric_signs;
  support.closed_rank = candidate.observed_closed_rank;
  support.natural_contact_kind = natural_contact.kind;
  support.contact_affine_dimension = natural_contact.affine_dimension;
  support.contact_site_affine_rank = natural_contact.site_affine_rank;
  candidate.accepted_support_index = support.support_index;
  result.supports.push_back(std::move(support));
}

template <std::size_t SupportSize>
void classify_proposal(
    const spatial::CanonicalPointCloud& cloud,
    const ProposedSupport& proposal,
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram,
    const std::map<std::vector<PointId>, std::size_t>& natural_contacts,
    exact::PredicateCounters& predicate_counters,
    ExactDepthZeroNaturalSupportResult& result) {
  if (proposal.point_ids.size() != SupportSize) {
    throw std::logic_error(
        "a natural-support proposal was dispatched with the wrong size");
  }
  ExactDepthZeroNaturalSupportCandidate candidate;
  candidate.candidate_index = result.candidates.size();
  candidate.support_point_ids = proposal.point_ids;
  candidate.first_proposal_contact_index =
      proposal.first_contact_index;

  std::array<exact::ExactRational3, SupportSize> points{};
  for (std::size_t index = 0U; index < SupportSize; ++index) {
    points[index] = cloud.point(proposal.point_ids[index]).exact();
  }
  const exact::CircumcenterSupportAnalysis analysis =
      exact::analyze_circumcenter_support(points, &predicate_counters);
  candidate.support_status = analysis.status();
  ++result.audit.support_analysis_count;

  switch (analysis.status()) {
    case exact::CircumcenterSupportStatus::affinely_dependent:
      candidate.outcome = ExactDepthZeroNaturalSupportCandidateOutcome::
          affinely_dependent_support;
      ++result.audit.affinely_dependent_support_count;
      result.candidates.push_back(std::move(candidate));
      return;
    case exact::CircumcenterSupportStatus::boundary_reduced:
      copy_sphere_witness(analysis, candidate);
      candidate.outcome = ExactDepthZeroNaturalSupportCandidateOutcome::
          boundary_reduced_support;
      ++result.audit.boundary_reduced_support_count;
      result.candidates.push_back(std::move(candidate));
      return;
    case exact::CircumcenterSupportStatus::exterior_circumcenter:
      copy_sphere_witness(analysis, candidate);
      candidate.outcome = ExactDepthZeroNaturalSupportCandidateOutcome::
          exterior_circumcenter_support;
      ++result.audit.exterior_circumcenter_support_count;
      result.candidates.push_back(std::move(candidate));
      return;
    case exact::CircumcenterSupportStatus::minimal:
      copy_sphere_witness(analysis, candidate);
      ++result.audit.minimal_support_count;
      break;
  }

  if (!std::all_of(
          candidate.support_barycentric_signs.begin(),
          candidate.support_barycentric_signs.end(),
          [](exact::PredicateSign sign) {
            return sign == exact::PredicateSign::positive;
          })) {
    throw std::logic_error(
        "a minimal natural support has a nonpositive barycentric coordinate");
  }
  const spatial::ClosedBallPartition partition =
      spatial::brute_force_closed_ball(
          cloud, *candidate.center, *candidate.squared_level);
  ++result.audit.global_closed_ball_query_count;
  result.audit.point_classification_count = checked_add(
      result.audit.point_classification_count,
      partition.distance_evaluation_count(),
      "the observed natural-support classification count overflows size_t");
  if (!partition.partition_complete() ||
      !partition.validated_for(cloud) ||
      partition.squared_radius() != *candidate.squared_level ||
      partition.evaluation_count() != cloud.size() ||
      partition.distance_evaluation_count() != cloud.size() ||
      partition.query_counters().method !=
          spatial::SpatialQueryMethod::brute_force) {
    throw std::logic_error(
        "a natural-support global closed-ball partition is incomplete");
  }
  candidate.interior_point_ids.assign(
      partition.interior_ids().begin(), partition.interior_ids().end());
  candidate.shell_point_ids.assign(
      partition.shell_ids().begin(), partition.shell_ids().end());
  candidate.exterior_point_ids.assign(
      partition.exterior_ids().begin(), partition.exterior_ids().end());
  candidate.observed_closed_rank = partition.closed_rank();
  candidate.support_relevance_rank = checked_add(
      candidate.interior_point_ids.size(),
      candidate.support_point_ids.size(),
      "the natural-support relevance rank overflows size_t");
  candidate.global_closed_ball_classified = true;
  if (candidate.interior_point_ids.size() +
              candidate.shell_point_ids.size() +
              candidate.exterior_point_ids.size() !=
          cloud.size() ||
      candidate.observed_closed_rank !=
          candidate.interior_point_ids.size() +
              candidate.shell_point_ids.size() ||
      !is_subset(
          candidate.support_point_ids,
          candidate.shell_point_ids)) {
    throw std::logic_error(
        "a natural-support global partition contradicts its support");
  }

  if (!candidate.interior_point_ids.empty()) {
    candidate.outcome = ExactDepthZeroNaturalSupportCandidateOutcome::
        minimal_with_strict_interior_deferred;
    ++result.audit.deferred_strict_interior_support_count;
    result.candidates.push_back(std::move(candidate));
    return;
  }

  const std::size_t carrier_contact_index = require_natural_carrier(
      candidate.shell_point_ids, natural_contacts);
  candidate.carrier_contact_index = carrier_contact_index;
  if (candidate.shell_point_ids != candidate.support_point_ids) {
    if (candidate.support_relevance_rank <=
        result.requirements.maximum_relevant_support_rank) {
      candidate.outcome = ExactDepthZeroNaturalSupportCandidateOutcome::
          relevant_extra_shell_degeneracy;
      candidate.relevant_extra_shell_diagnostic_index = append_diagnostic(
          result, candidate, carrier_contact_index);
      ++result.audit.relevant_extra_shell_support_count;
    } else {
      candidate.outcome = ExactDepthZeroNaturalSupportCandidateOutcome::
          extra_shell_outside_rank_window;
      ++result.audit.outside_window_extra_shell_support_count;
    }
    result.candidates.push_back(std::move(candidate));
    return;
  }

  if (candidate.observed_closed_rank >
      result.requirements.maximum_relevant_support_rank) {
    candidate.outcome = ExactDepthZeroNaturalSupportCandidateOutcome::
        minimal_support_above_rank_window;
    ++result.audit.above_rank_support_count;
    result.candidates.push_back(std::move(candidate));
    return;
  }

  candidate.outcome = ExactDepthZeroNaturalSupportCandidateOutcome::
      accepted_empty_interior_support;
  append_accepted_support(
      result,
      candidate,
      source_diagram.contacts[carrier_contact_index],
      carrier_contact_index);
  ++result.audit.accepted_support_count;
  result.candidates.push_back(std::move(candidate));
}

void classify_proposals(
    const spatial::CanonicalPointCloud& cloud,
    const std::vector<ProposedSupport>& proposals,
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram,
    const std::map<std::vector<PointId>, std::size_t>& natural_contacts,
    exact::PredicateCounters& predicate_counters,
    ExactDepthZeroNaturalSupportResult& result) {
  for (const ProposedSupport& proposal : proposals) {
    switch (proposal.point_ids.size()) {
      case 2U:
        classify_proposal<2U>(
            cloud,
            proposal,
            source_diagram,
            natural_contacts,
            predicate_counters,
            result);
        break;
      case 3U:
        classify_proposal<3U>(
            cloud,
            proposal,
            source_diagram,
            natural_contacts,
            predicate_counters,
            result);
        break;
      case 4U:
        classify_proposal<4U>(
            cloud,
            proposal,
            source_diagram,
            natural_contacts,
            predicate_counters,
            result);
        break;
      default:
        throw std::logic_error(
            "a natural carrier proposed a support outside sizes two to four");
    }
  }
}

[[nodiscard]] bool center_less(
    const exact::ExactCenter3& left,
    const exact::ExactCenter3& right) {
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    if (left.coordinate(axis) != right.coordinate(axis)) {
      return left.coordinate(axis) < right.coordinate(axis);
    }
  }
  return false;
}

[[nodiscard]] bool support_key_less(
    const ExactDepthZeroNaturalSupport& left,
    const ExactDepthZeroNaturalSupport& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  if (left.closed_rank != right.closed_rank) {
    return left.closed_rank < right.closed_rank;
  }
  if (left.support_point_ids != right.support_point_ids) {
    return left.support_point_ids < right.support_point_ids;
  }
  return center_less(left.center, right.center);
}

void canonicalize_supports(ExactDepthZeroNaturalSupportResult& result) {
  std::sort(
      result.supports.begin(), result.supports.end(), support_key_less);
  for (ExactDepthZeroNaturalSupportCandidate& candidate :
       result.candidates) {
    candidate.accepted_support_index.reset();
  }
  for (std::size_t index = 0U; index < result.supports.size(); ++index) {
    ExactDepthZeroNaturalSupport& support = result.supports[index];
    if (index != 0U &&
        !support_key_less(result.supports[index - 1U], support)) {
      throw std::logic_error(
          "natural-support extraction produced duplicate canonical supports");
    }
    if (support.source_candidate_index >= result.candidates.size()) {
      throw std::logic_error(
          "a natural support points outside its candidate catalogue");
    }
    support.support_index = index;
    ExactDepthZeroNaturalSupportCandidate& candidate =
        result.candidates[support.source_candidate_index];
    if (candidate.accepted_support_index.has_value()) {
      throw std::logic_error(
          "two natural supports point to one source candidate");
    }
    candidate.accepted_support_index = index;
  }
}

[[nodiscard]] bool diagnostic_key_less(
    const ExactDepthZeroNaturalExtraShellDiagnostic& left,
    const ExactDepthZeroNaturalExtraShellDiagnostic& right) {
  if (left.squared_level != right.squared_level) {
    return left.squared_level < right.squared_level;
  }
  if (left.shell_point_ids != right.shell_point_ids) {
    return left.shell_point_ids < right.shell_point_ids;
  }
  if (left.support_point_id_sets != right.support_point_id_sets) {
    return left.support_point_id_sets < right.support_point_id_sets;
  }
  return center_less(left.center, right.center);
}

void canonicalize_diagnostics(ExactDepthZeroNaturalSupportResult& result) {
  for (ExactDepthZeroNaturalExtraShellDiagnostic& diagnostic :
       result.relevant_extra_shell_diagnostics) {
    std::vector<std::size_t> positions(
        diagnostic.source_candidate_indices.size());
    for (std::size_t index = 0U; index < positions.size(); ++index) {
      positions[index] = index;
    }
    std::sort(
        positions.begin(),
        positions.end(),
        [&](std::size_t left, std::size_t right) {
          return diagnostic.source_candidate_indices[left] <
                 diagnostic.source_candidate_indices[right];
        });
    std::vector<std::vector<PointId>> support_sets;
    std::vector<std::size_t> candidate_indices;
    support_sets.reserve(positions.size());
    candidate_indices.reserve(positions.size());
    for (const std::size_t position : positions) {
      support_sets.push_back(
          diagnostic.support_point_id_sets[position]);
      candidate_indices.push_back(
          diagnostic.source_candidate_indices[position]);
    }
    diagnostic.support_point_id_sets = std::move(support_sets);
    diagnostic.source_candidate_indices = std::move(candidate_indices);
  }
  std::sort(
      result.relevant_extra_shell_diagnostics.begin(),
      result.relevant_extra_shell_diagnostics.end(),
      diagnostic_key_less);
  for (std::size_t index = 0U;
       index < result.relevant_extra_shell_diagnostics.size();
       ++index) {
    ExactDepthZeroNaturalExtraShellDiagnostic& diagnostic =
        result.relevant_extra_shell_diagnostics[index];
    if (index != 0U &&
        !diagnostic_key_less(
            result.relevant_extra_shell_diagnostics[index - 1U], diagnostic)) {
      throw std::logic_error(
          "natural-support extraction produced duplicate diagnostics");
    }
    diagnostic.diagnostic_index = index;
    for (const std::size_t candidate_index :
         diagnostic.source_candidate_indices) {
      if (candidate_index >= result.candidates.size()) {
        throw std::logic_error(
            "an extra-shell diagnostic points outside its candidates");
      }
      result.candidates[candidate_index]
          .relevant_extra_shell_diagnostic_index = index;
    }
  }
}

[[nodiscard]] bool candidates_complete(
    const ExactDepthZeroNaturalSupportResult& result) {
  if (result.candidates.size() != result.audit.unique_support_count) {
    return false;
  }
  std::size_t previous_size = 0U;
  std::vector<PointId> previous_ids;
  for (std::size_t index = 0U;
       index < result.candidates.size();
       ++index) {
    const ExactDepthZeroNaturalSupportCandidate& candidate =
        result.candidates[index];
    if (candidate.candidate_index != index ||
        candidate.support_point_ids.size() < 2U ||
        candidate.support_point_ids.size() > 4U ||
        !std::is_sorted(
            candidate.support_point_ids.begin(),
            candidate.support_point_ids.end()) ||
        std::adjacent_find(
            candidate.support_point_ids.begin(),
            candidate.support_point_ids.end()) !=
            candidate.support_point_ids.end() ||
        candidate.first_proposal_contact_index >=
            result.source_diagram->contacts.size()) {
      return false;
    }
    if (index != 0U &&
        (candidate.support_point_ids.size() < previous_size ||
         (candidate.support_point_ids.size() == previous_size &&
          !(previous_ids < candidate.support_point_ids)))) {
      return false;
    }
    previous_size = candidate.support_point_ids.size();
    previous_ids = candidate.support_point_ids;
    const spatial::ExactOrdinaryDiagramContact& proposal_contact =
        result.source_diagram->contacts[
            candidate.first_proposal_contact_index];
    if (!is_natural_contact_kind(proposal_contact.kind) ||
        !is_subset(
            candidate.support_point_ids,
            proposal_contact.carrier_shell_ids)) {
      return false;
    }
    const bool has_sphere = candidate.center.has_value() &&
        candidate.squared_level.has_value() &&
        candidate.support_barycentric_coordinates.size() ==
            candidate.support_point_ids.size() &&
        candidate.support_barycentric_signs.size() ==
            candidate.support_point_ids.size();
    if (candidate.support_status ==
        exact::CircumcenterSupportStatus::affinely_dependent) {
      if (candidate.outcome !=
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  affinely_dependent_support ||
          has_sphere || candidate.global_closed_ball_classified ||
          candidate.carrier_contact_index.has_value() ||
          candidate.accepted_support_index.has_value() ||
          candidate.relevant_extra_shell_diagnostic_index.has_value()) {
        return false;
      }
      continue;
    }
    if (!has_sphere) {
      return false;
    }
    if (candidate.support_status !=
        exact::CircumcenterSupportStatus::minimal) {
      const ExactDepthZeroNaturalSupportCandidateOutcome expected_outcome =
          candidate.support_status ==
                  exact::CircumcenterSupportStatus::boundary_reduced
              ? ExactDepthZeroNaturalSupportCandidateOutcome::
                    boundary_reduced_support
              : ExactDepthZeroNaturalSupportCandidateOutcome::
                    exterior_circumcenter_support;
      if (candidate.outcome != expected_outcome ||
          candidate.global_closed_ball_classified ||
          !candidate.interior_point_ids.empty() ||
          !candidate.shell_point_ids.empty() ||
          !candidate.exterior_point_ids.empty() ||
          candidate.observed_closed_rank != 0U ||
          candidate.support_relevance_rank != 0U ||
          candidate.carrier_contact_index.has_value() ||
          candidate.accepted_support_index.has_value() ||
          candidate.relevant_extra_shell_diagnostic_index.has_value()) {
        return false;
      }
      continue;
    }
    if (!candidate.global_closed_ball_classified ||
        candidate.interior_point_ids.size() +
                candidate.shell_point_ids.size() +
                candidate.exterior_point_ids.size() !=
            result.requirements.point_count ||
        candidate.observed_closed_rank !=
            candidate.interior_point_ids.size() +
                candidate.shell_point_ids.size() ||
        !is_subset(
            candidate.support_point_ids,
            candidate.shell_point_ids) ||
        candidate.support_relevance_rank !=
            candidate.interior_point_ids.size() +
                candidate.support_point_ids.size() ||
        !std::all_of(
            candidate.support_barycentric_signs.begin(),
            candidate.support_barycentric_signs.end(),
            [](exact::PredicateSign sign) {
              return sign == exact::PredicateSign::positive;
            })) {
      return false;
    }
    if (!candidate.interior_point_ids.empty()) {
      if (candidate.outcome !=
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  minimal_with_strict_interior_deferred ||
          candidate.carrier_contact_index.has_value() ||
          candidate.accepted_support_index.has_value() ||
          candidate.relevant_extra_shell_diagnostic_index.has_value()) {
        return false;
      }
      continue;
    }
    if (!candidate.carrier_contact_index.has_value() ||
        *candidate.carrier_contact_index >=
            result.source_diagram->contacts.size() ||
        result.source_diagram
                ->contacts[*candidate.carrier_contact_index]
                .carrier_shell_ids != candidate.shell_point_ids) {
      return false;
    }
    if (candidate.shell_point_ids != candidate.support_point_ids) {
      const bool relevant = candidate.support_relevance_rank <=
          result.requirements.maximum_relevant_support_rank;
      const ExactDepthZeroNaturalSupportCandidateOutcome expected_outcome =
          relevant
              ? ExactDepthZeroNaturalSupportCandidateOutcome::
                    relevant_extra_shell_degeneracy
              : ExactDepthZeroNaturalSupportCandidateOutcome::
                    extra_shell_outside_rank_window;
      if (candidate.outcome != expected_outcome ||
          candidate.accepted_support_index.has_value() ||
          candidate.relevant_extra_shell_diagnostic_index.has_value() !=
              relevant) {
        return false;
      }
      continue;
    }
    const bool above_rank = candidate.observed_closed_rank >
        result.requirements.maximum_relevant_support_rank;
    const ExactDepthZeroNaturalSupportCandidateOutcome expected_outcome =
        above_rank
            ? ExactDepthZeroNaturalSupportCandidateOutcome::
                  minimal_support_above_rank_window
            : ExactDepthZeroNaturalSupportCandidateOutcome::
                  accepted_empty_interior_support;
    if (candidate.outcome != expected_outcome ||
        candidate.accepted_support_index.has_value() == above_rank ||
        candidate.relevant_extra_shell_diagnostic_index.has_value()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool supports_complete(
    const ExactDepthZeroNaturalSupportResult& result) {
  if (result.supports.size() != result.audit.accepted_support_count) {
    return false;
  }
  for (std::size_t index = 0U; index < result.supports.size(); ++index) {
    const ExactDepthZeroNaturalSupport& support = result.supports[index];
    if (support.support_index != index ||
        support.source_candidate_index >= result.candidates.size() ||
        support.natural_contact_index >=
            result.source_diagram->contacts.size() ||
        (index != 0U &&
         !support_key_less(result.supports[index - 1U], support))) {
      return false;
    }
    const ExactDepthZeroNaturalSupportCandidate& candidate =
        result.candidates[support.source_candidate_index];
    const spatial::ExactOrdinaryDiagramContact& contact =
        result.source_diagram->contacts[support.natural_contact_index];
    if (candidate.outcome !=
            ExactDepthZeroNaturalSupportCandidateOutcome::
                accepted_empty_interior_support ||
        candidate.accepted_support_index !=
            std::optional<std::size_t>{index} ||
        support.support_point_ids != candidate.support_point_ids ||
        contact.query_ids != support.support_point_ids ||
        contact.carrier_shell_ids != support.support_point_ids ||
        contact.kind != support.natural_contact_kind ||
        contact.common_artificial_box_face_mask != 0U) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool diagnostics_complete(
    const ExactDepthZeroNaturalSupportResult& result) {
  std::size_t referenced_candidate_count = 0U;
  for (std::size_t index = 0U;
       index < result.relevant_extra_shell_diagnostics.size();
       ++index) {
    const ExactDepthZeroNaturalExtraShellDiagnostic& diagnostic =
        result.relevant_extra_shell_diagnostics[index];
    if (diagnostic.diagnostic_index != index ||
        diagnostic.support_point_id_sets.size() !=
            diagnostic.source_candidate_indices.size() ||
        diagnostic.source_candidate_indices.empty() ||
        diagnostic.carrier_contact_index >=
            result.source_diagram->contacts.size() ||
        (index != 0U &&
         !diagnostic_key_less(
             result.relevant_extra_shell_diagnostics[index - 1U], diagnostic))) {
      return false;
    }
    for (std::size_t support_index = 0U;
         support_index < diagnostic.source_candidate_indices.size();
         ++support_index) {
      const std::size_t candidate_index =
          diagnostic.source_candidate_indices[support_index];
      if (candidate_index >= result.candidates.size()) {
        return false;
      }
      const ExactDepthZeroNaturalSupportCandidate& candidate =
          result.candidates[candidate_index];
      if (candidate.outcome !=
              ExactDepthZeroNaturalSupportCandidateOutcome::
                  relevant_extra_shell_degeneracy ||
          candidate.relevant_extra_shell_diagnostic_index !=
              std::optional<std::size_t>{index} ||
          candidate.support_point_ids !=
              diagnostic.support_point_id_sets[support_index] ||
          !same_diagnostic_identity(
              diagnostic,
              candidate,
              diagnostic.carrier_contact_index)) {
        return false;
      }
      ++referenced_candidate_count;
    }
  }
  return referenced_candidate_count ==
      result.audit.relevant_extra_shell_support_count;
}

[[nodiscard]] ExactDepthZeroNaturalSupportResult compute_result(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::StrictlyPaddedDyadicAabb3Result& clipping_box,
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram,
    std::size_t requested_maximum_order,
    ExactDepthZeroNaturalSupportBudget budget) {
  validate_domain(
      cloud, clipping_box, requested_maximum_order, budget);
  ExactDepthZeroNaturalSupportResult result;
  result.requested_budget = budget;
  result.requested_maximum_order = requested_maximum_order;
  result.clipping_box = clipping_box;
  result.requirements =
      requirements_for(cloud.size(), requested_maximum_order);
  result.canonical_point_bits.reserve(cloud.size());
  for (std::size_t index = 0U; index < cloud.size(); ++index) {
    result.canonical_point_bits.push_back(
        cloud.point(static_cast<PointId>(index)).canonical_input_bits());
  }

  if (!budget_covers(budget, result.requirements)) {
    result.decision =
        ExactDepthZeroNaturalSupportDecision::insufficient_budget;
    return result;
  }
  result.preflight_budget_sufficient = true;

  const spatial::ExactOrdinaryDiagramClosureVerification
      source_verification =
          spatial::verify_exact_bounded_ordinary_diagram_closure(
              cloud, clipping_box, source_diagram);
  if (!source_verification_complete(
          source_verification, source_diagram)) {
    result.decision = ExactDepthZeroNaturalSupportDecision::
        source_diagram_not_complete_or_not_certified;
    return result;
  }
  result.source_diagram = source_diagram;
  result.source_diagram_freshly_verified = true;
  result.extraction_started_after_successful_preflight = true;

  const std::vector<ProposedSupport> proposals = collect_proposals(
      source_diagram, result.requirements, result.audit);
  result.proposal_space_complete = true;
  result.candidates.reserve(proposals.size());
  result.supports.reserve(proposals.size());
  exact::PredicateCounters predicate_counters;
  const std::map<std::vector<PointId>, std::size_t> natural_contacts =
      natural_contact_indices(source_diagram);
  classify_proposals(
      cloud,
      proposals,
      source_diagram,
      natural_contacts,
      predicate_counters,
      result);
  result.predicate_counters = predicate_snapshot(predicate_counters);
  const std::size_t disposition_count = checked_add(
      checked_add(
          checked_add(
              result.audit.affinely_dependent_support_count,
              result.audit.boundary_reduced_support_count,
              "the natural-support disposition count overflows size_t"),
          result.audit.exterior_circumcenter_support_count,
          "the natural-support disposition count overflows size_t"),
      result.audit.minimal_support_count,
      "the natural-support disposition count overflows size_t");
  const std::size_t minimal_disposition_count = checked_add(
      checked_add(
          checked_add(
              result.audit.deferred_strict_interior_support_count,
              result.audit.relevant_extra_shell_support_count,
              "the natural-support minimal disposition count overflows size_t"),
          result.audit.outside_window_extra_shell_support_count,
          "the natural-support minimal disposition count overflows size_t"),
      checked_add(
          result.audit.above_rank_support_count,
          result.audit.accepted_support_count,
          "the natural-support minimal disposition count overflows size_t"),
      "the natural-support minimal disposition count overflows size_t");
  if (result.audit.point_classification_count >
          result.requirements.conservative_point_classification_count ||
      result.audit.support_analysis_count != proposals.size() ||
      result.audit.support_analysis_count != disposition_count ||
      result.audit.minimal_support_count !=
          minimal_disposition_count) {
    throw std::logic_error(
        "natural-support classification exceeded its proved preflight");
  }

  canonicalize_supports(result);
  canonicalize_diagnostics(result);
  result.audit.deduplicated_relevant_extra_shell_diagnostic_count =
      result.relevant_extra_shell_diagnostics.size();
  result.all_candidates_classified = candidates_complete(result);
  result.all_minimal_support_partitions_complete =
      result.audit.global_closed_ball_query_count ==
          result.audit.minimal_support_count &&
      result.audit.point_classification_count == checked_multiply(
          result.audit.minimal_support_count,
          cloud.size(),
          "the natural-support completed classification count overflows size_t");
  result.accepted_supports_natural_and_indexed =
      supports_complete(result);
  result.relevant_extra_shell_diagnostics_complete =
      diagnostics_complete(result);
  result.candidate_queue_empty = true;
  result.no_depth_zero_relevant_extra_shell_degeneracy =
      result.audit.relevant_extra_shell_support_count == 0U &&
      result.relevant_extra_shell_diagnostics.empty();
  result.no_artificial_support_emitted = std::all_of(
      result.supports.begin(),
      result.supports.end(),
      [](const ExactDepthZeroNaturalSupport& support) {
        return is_natural_contact_kind(support.natural_contact_kind);
      });
  result.no_remaining_unknown_predicates =
      result.predicate_counters.remaining_unknown_count == 0U;

  if (!result.proposal_space_complete ||
      !result.all_candidates_classified ||
      !result.all_minimal_support_partitions_complete ||
      !result.accepted_supports_natural_and_indexed ||
      !result.relevant_extra_shell_diagnostics_complete ||
      !result.candidate_queue_empty ||
      !result.no_artificial_support_emitted ||
      !result.no_remaining_unknown_predicates ||
      result.audit.accepted_support_count != result.supports.size() ||
      result.audit.deduplicated_relevant_extra_shell_diagnostic_count !=
          result.relevant_extra_shell_diagnostics.size() ||
      result.audit.overflow_count != 0U) {
    throw std::logic_error(
        "the natural-support extraction failed structural closure");
  }
  result.decision = result.no_depth_zero_relevant_extra_shell_degeneracy
      ? ExactDepthZeroNaturalSupportDecision::
            complete_supported_extraction
      : ExactDepthZeroNaturalSupportDecision::
            complete_extraction_with_relevant_extra_shell_degeneracy;
  return result;
}

}  // namespace

std::string_view to_string(
    ExactDepthZeroNaturalSupportCandidateOutcome outcome) {
  switch (outcome) {
    case ExactDepthZeroNaturalSupportCandidateOutcome::not_classified:
      return "not_classified";
    case ExactDepthZeroNaturalSupportCandidateOutcome::
        affinely_dependent_support:
      return "affinely_dependent_support";
    case ExactDepthZeroNaturalSupportCandidateOutcome::
        boundary_reduced_support:
      return "boundary_reduced_support";
    case ExactDepthZeroNaturalSupportCandidateOutcome::
        exterior_circumcenter_support:
      return "exterior_circumcenter_support";
    case ExactDepthZeroNaturalSupportCandidateOutcome::
        minimal_with_strict_interior_deferred:
      return "minimal_with_strict_interior_deferred";
    case ExactDepthZeroNaturalSupportCandidateOutcome::
        relevant_extra_shell_degeneracy:
      return "relevant_extra_shell_degeneracy";
    case ExactDepthZeroNaturalSupportCandidateOutcome::
        extra_shell_outside_rank_window:
      return "extra_shell_outside_rank_window";
    case ExactDepthZeroNaturalSupportCandidateOutcome::
        minimal_support_above_rank_window:
      return "minimal_support_above_rank_window";
    case ExactDepthZeroNaturalSupportCandidateOutcome::
        accepted_empty_interior_support:
      return "accepted_empty_interior_support";
  }
  throw std::invalid_argument(
      "the natural-support candidate outcome is invalid");
}

std::string_view to_string(
    ExactDepthZeroNaturalSupportDecision decision) {
  switch (decision) {
    case ExactDepthZeroNaturalSupportDecision::
        source_diagram_not_complete_or_not_certified:
      return "source_diagram_not_complete_or_not_certified";
    case ExactDepthZeroNaturalSupportDecision::insufficient_budget:
      return "insufficient_budget";
    case ExactDepthZeroNaturalSupportDecision::
        complete_supported_extraction:
      return "complete_supported_extraction";
    case ExactDepthZeroNaturalSupportDecision::
        complete_extraction_with_relevant_extra_shell_degeneracy:
      return "complete_extraction_with_relevant_extra_shell_degeneracy";
  }
  throw std::invalid_argument(
      "the natural-support extraction decision is invalid");
}

ExactDepthZeroNaturalSupportVerification
verify_exact_bounded_depth_zero_natural_supports(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::StrictlyPaddedDyadicAabb3Result& clipping_box,
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram,
    std::size_t requested_maximum_order,
    ExactDepthZeroNaturalSupportBudget budget,
    const ExactDepthZeroNaturalSupportResult& result) {
  const ExactDepthZeroNaturalSupportResult expected = compute_result(
      cloud,
      clipping_box,
      source_diagram,
      requested_maximum_order,
      budget);
  ExactDepthZeroNaturalSupportVerification verification;
  verification.requested_budget_certified =
      result.requested_budget == budget &&
      result.requested_budget == expected.requested_budget;
  verification.input_identity_certified =
      result.requested_maximum_order == requested_maximum_order &&
      result.requested_maximum_order ==
          expected.requested_maximum_order &&
      result.canonical_point_bits == expected.canonical_point_bits &&
      result.clipping_box == clipping_box &&
      result.clipping_box == expected.clipping_box;
  verification.clipping_box_certified =
      result.clipping_box == clipping_box &&
      result.clipping_box == expected.clipping_box &&
      spatial::verify_strictly_padded_dyadic_aabb(
          cloud, result.clipping_box).result_certified;
  verification.requirements_certified =
      result.requirements == expected.requirements;
  verification.source_diagram_certified =
      result.source_diagram == expected.source_diagram &&
      result.source_diagram_freshly_verified ==
          expected.source_diagram_freshly_verified;
  verification.candidates_certified =
      result.candidates == expected.candidates;
  verification.supports_certified =
      result.supports == expected.supports;
  verification.relevant_extra_shell_diagnostics_certified =
      result.relevant_extra_shell_diagnostics ==
          expected.relevant_extra_shell_diagnostics;
  verification.predicate_counters_certified =
      result.predicate_counters == expected.predicate_counters;
  verification.audit_certified = result.audit == expected.audit;
  verification.result_facts_certified =
      result.preflight_budget_sufficient ==
          expected.preflight_budget_sufficient &&
      result.extraction_started_after_successful_preflight ==
          expected.extraction_started_after_successful_preflight &&
      result.proposal_space_complete ==
          expected.proposal_space_complete &&
      result.all_candidates_classified ==
          expected.all_candidates_classified &&
      result.all_minimal_support_partitions_complete ==
          expected.all_minimal_support_partitions_complete &&
      result.accepted_supports_natural_and_indexed ==
          expected.accepted_supports_natural_and_indexed &&
      result.relevant_extra_shell_diagnostics_complete ==
          expected.relevant_extra_shell_diagnostics_complete &&
      result.candidate_queue_empty ==
          expected.candidate_queue_empty &&
      result.no_depth_zero_relevant_extra_shell_degeneracy ==
          expected.no_depth_zero_relevant_extra_shell_degeneracy &&
      result.no_artificial_support_emitted ==
          expected.no_artificial_support_emitted &&
      result.no_remaining_unknown_predicates ==
          expected.no_remaining_unknown_predicates;
  verification.decision_certified =
      result.decision == expected.decision;
  verification.fresh_replay_certified = result == expected;
  verification.result_certified =
      verification.requested_budget_certified &&
      verification.input_identity_certified &&
      verification.clipping_box_certified &&
      verification.requirements_certified &&
      verification.source_diagram_certified &&
      verification.candidates_certified &&
      verification.supports_certified &&
      verification.relevant_extra_shell_diagnostics_certified &&
      verification.predicate_counters_certified &&
      verification.audit_certified &&
      verification.result_facts_certified &&
      verification.decision_certified &&
      verification.fresh_replay_certified;
  return verification;
}

ExactDepthZeroNaturalSupportResult
build_exact_bounded_depth_zero_natural_supports(
    const spatial::CanonicalPointCloud& cloud,
    const spatial::StrictlyPaddedDyadicAabb3Result& clipping_box,
    const spatial::ExactOrdinaryDiagramClosureResult& source_diagram,
    std::size_t requested_maximum_order,
    ExactDepthZeroNaturalSupportBudget budget) {
  ExactDepthZeroNaturalSupportResult result = compute_result(
      cloud,
      clipping_box,
      source_diagram,
      requested_maximum_order,
      budget);
  const ExactDepthZeroNaturalSupportVerification verification =
      verify_exact_bounded_depth_zero_natural_supports(
          cloud,
          clipping_box,
          source_diagram,
          requested_maximum_order,
          budget,
          result);
  if (!verification.result_certified) {
    throw std::logic_error(
        "the natural-support extraction failed its fresh replay");
  }
  return result;
}

}  // namespace morsehgp3d::hierarchy
