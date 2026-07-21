#include "morsehgp3d/gpu/h_polytope_proposal.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactAffineForm3;
using morsehgp3d::gpu::HPolytopeProposalBatchDecision;
using morsehgp3d::gpu::HPolytopeProposalBatchResult;
using morsehgp3d::gpu::HPolytopeProposalCellStatus;
using morsehgp3d::gpu::HPolytopeProposalContext;
using morsehgp3d::spatial::ExactBoundedHPolytopeReferenceDecision;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::ExactHPolytopeHalfspace3;
using morsehgp3d::spatial::ExactHPolytopeHalfspaceRole;
using morsehgp3d::spatial::HPolytopeConstraintDomain;
using morsehgp3d::spatial::HPolytopeConstraintId;
using morsehgp3d::spatial::build_exact_bounded_h_polytope_reference;

[[nodiscard]] std::uint64_t word(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] ExactDyadicAabb3 unit_box() {
  return ExactDyadicAabb3{
      {word(-1.0), word(-1.0), word(-1.0)},
      {word(1.0), word(1.0), word(1.0)}};
}

[[nodiscard]] ExactHPolytopeHalfspace3 halfspace(
    std::uint64_t id,
    int a,
    int b,
    int c,
    int d) {
  return ExactHPolytopeHalfspace3{
      HPolytopeConstraintId{
          HPolytopeConstraintDomain::generic_affine, id, 0U},
      ExactHPolytopeHalfspaceRole::new_clip,
      ExactAffineForm3::from_integer_coefficients(
          {BigInt{a}, BigInt{b}, BigInt{c}, BigInt{d}})};
}

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void require_qualification_result(
    const HPolytopeProposalBatchResult& result,
    std::uint64_t expected_epoch) {
  const ExactDyadicAabb3 box = unit_box();
  const std::array<ExactHPolytopeHalfspace3, 0U> no_halfspaces{};
  const std::array<ExactHPolytopeHalfspace3, 1U> cut{
      halfspace(2U, 1, 0, 0, 0)};
  const std::array<ExactHPolytopeHalfspace3, 1U> positive_constant{
      halfspace(1U, 0, 0, 0, 1)};
  const auto exact_cube =
      build_exact_bounded_h_polytope_reference(no_halfspaces, box);
  const auto exact_cut =
      build_exact_bounded_h_polytope_reference(cut, box);
  const auto exact_empty = build_exact_bounded_h_polytope_reference(
      positive_constant, box);

  require(
      result.decision ==
          HPolytopeProposalBatchDecision::exact_recertified_local,
      "the qualification batch was not recertified exactly on CPU");
  require(
      result.requirements.size() == 4U &&
          result.requirements[0U].cell_id == 10U &&
          result.requirements[0U].boundary_plane_count == 6U &&
          result.requirements[0U].exhaustive_proposal_record_count == 20U &&
          result.requirements[1U].cell_id == 20U &&
          result.requirements[1U].boundary_plane_count == 7U &&
          result.requirements[1U].exhaustive_proposal_record_count == 35U &&
          result.requirements[2U].cell_id == 30U &&
          result.requirements[2U].boundary_plane_count == 6U &&
          result.requirements[2U].exhaustive_proposal_record_count == 20U &&
          result.requirements[3U].cell_id == 40U &&
          result.requirements[3U].boundary_plane_count == 6U &&
          result.requirements[3U].exhaustive_proposal_record_count == 20U,
      "the analytic B choose 3 requirements are incorrect");
  require(
      result.cell_results.size() == 4U &&
          result.cell_results[0U].cell_id == 10U &&
          result.cell_results[0U].status ==
              HPolytopeProposalCellStatus::validated_exhaustive_transcript &&
          result.cell_results[0U].exact_result == exact_cube &&
          result.cell_results[1U].cell_id == 20U &&
          result.cell_results[1U].status ==
              HPolytopeProposalCellStatus::validated_exhaustive_transcript &&
          result.cell_results[1U].exact_result == exact_cut &&
          result.cell_results[2U].cell_id == 30U &&
          result.cell_results[2U].status ==
              HPolytopeProposalCellStatus::exact_fallback_interval_unknown &&
          result.cell_results[2U].exact_result == exact_empty &&
          result.cell_results[3U].cell_id == 40U &&
          result.cell_results[3U].status ==
              HPolytopeProposalCellStatus::
                  exact_fallback_capacity_exhausted &&
          result.cell_results[3U].exact_result == exact_cube,
      "the canonical cells, exact references, or whole-row fallbacks differ");
  require(
      exact_cube.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_nonempty &&
          exact_cut.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_nonempty &&
          exact_empty.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_empty,
      "the hard-coded CPU reference cases have unexpected decisions");
  require(
      result.proposal_offsets ==
              std::vector<std::size_t>{0U, 20U, 55U, 55U, 55U} &&
          result.proposal_records.size() == 55U,
      "the GPU transcript is not closed on complete canonical rows");

  const auto& audit = result.audit;
  require(
      audit.canonical_input_cell_count == 4U &&
          audit.projectable_cell_count == 4U &&
          audit.validated_exhaustive_transcript_cell_count == 2U &&
          audit.interval_unknown_fallback_cell_count == 1U &&
          audit.capacity_exhausted_fallback_cell_count == 1U &&
          audit.unsupported_projection_fallback_cell_count == 0U &&
          audit.physical_proposal_record_capacity == 55U &&
          audit.required_exhaustive_proposal_record_count == 95U &&
          audit.initialized_proposal_record_count == 55U &&
          audit.exact_plane_triple_replay_count == 55U &&
          audit.gpu_launch_count == 1U &&
          audit.buffer_epoch == expected_epoch,
      "the CUDA launch, capacity, epoch, or replay accounting is open");
  require(
      audit.unknown_proposal_record_count > 0U &&
          audit.strict_reject_proposal_record_count > 0U &&
          audit.survivor_proposal_record_count > 0U &&
          audit.unknown_proposal_record_count +
                  audit.strict_reject_proposal_record_count +
                  audit.survivor_proposal_record_count ==
              audit.initialized_proposal_record_count,
      "the analytic batch did not exercise and close all proposal statuses");
  require(
      audit.canonical_cell_order_validated &&
          audit.exhaustive_transcript_validated &&
          audit.cpu_exact_recertification_complete &&
          !audit.global_status_published,
      "the proposal transcript or exact CPU recertification is not closed");
}

[[nodiscard]] HPolytopeProposalBatchResult run_batch(
    HPolytopeProposalContext& context) {
  // Deliberately noncanonical input order.  Capacity admits the canonical
  // cube and cut rows, then forces whole-row fallbacks for the exact-empty
  // constant and the final cube.
  const std::array<std::uint64_t, 4U> cell_ids{40U, 30U, 20U, 10U};
  const std::array<std::size_t, 5U> offsets{0U, 0U, 1U, 2U, 2U};
  const std::array<ExactHPolytopeHalfspace3, 2U> halfspaces{
      halfspace(1U, 0, 0, 0, 1),
      halfspace(2U, 1, 0, 0, 0)};
  return context.build(cell_ids, offsets, halfspaces);
}

}  // namespace

int main() {
  try {
    HPolytopeProposalContext context{unit_box(), 55U};
    const HPolytopeProposalBatchResult first = run_batch(context);
    require_qualification_result(first, 1U);
    const HPolytopeProposalBatchResult second = run_batch(context);
    require_qualification_result(second, 2U);

    HPolytopeProposalBatchResult normalized_first = first;
    HPolytopeProposalBatchResult normalized_second = second;
    normalized_first.audit.buffer_epoch = 0U;
    normalized_second.audit.buffer_epoch = 0U;
    require(
        normalized_first == normalized_second,
        "two resident epochs produced different public transcripts");

    std::cout
        << "{\"schema\":\"morsehgp3d.phase7.h_polytope_cuda_qualification.v1\""
        << ",\"cell_count\":4,\"record_count\":55"
        << ",\"unknown_count\":"
        << second.audit.unknown_proposal_record_count
        << ",\"strict_reject_count\":"
        << second.audit.strict_reject_proposal_record_count
        << ",\"survivor_count\":"
        << second.audit.survivor_proposal_record_count
        << ",\"first_epoch\":1,\"second_epoch\":2"
        << ",\"proposal_digest_fnv1a\":"
        << second.audit.proposal_digest_fnv1a
        << ",\"exact_cpu_recertification\":true"
        << ",\"deterministic\":true}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "MorseHGP3D H-polytope CUDA qualification failed: "
              << error.what() << '\n';
    return 1;
  }
}
