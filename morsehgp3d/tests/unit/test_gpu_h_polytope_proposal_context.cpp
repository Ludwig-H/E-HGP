#include "fake_gpu_h_polytope_proposal_launchers.hpp"

#include "morsehgp3d/gpu/h_polytope_proposal.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactAffineForm3;
using morsehgp3d::gpu::HPolytopeProposalBatchDecision;
using morsehgp3d::gpu::HPolytopeProposalBatchResult;
using morsehgp3d::gpu::HPolytopeProposalCellStatus;
using morsehgp3d::gpu::HPolytopeProposalContext;
using morsehgp3d::gpu::HPolytopeProposalRecordStatus;
using morsehgp3d::gpu::test_support::FakeHPolytopeProposalConfiguration;
using morsehgp3d::gpu::test_support::FakeHPolytopeProposalCorruption;
using morsehgp3d::gpu::test_support::FakeHPolytopeProposalValues;
using morsehgp3d::gpu::test_support::
    configure_fake_gpu_h_polytope_proposal;
using morsehgp3d::gpu::test_support::
    fake_gpu_h_polytope_proposal_last_boundary_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_h_polytope_proposal_last_cell_count;
using morsehgp3d::gpu::test_support::
    fake_gpu_h_polytope_proposal_last_record_capacity;
using morsehgp3d::gpu::test_support::
    fake_gpu_h_polytope_proposal_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_h_polytope_proposal;
using morsehgp3d::spatial::ExactBoundedHPolytopeReferenceBudget;
using morsehgp3d::spatial::ExactBoundedHPolytopeReferenceDecision;
using morsehgp3d::spatial::ExactBoundedHPolytopeReferenceResult;
using morsehgp3d::spatial::ExactDyadicAabb3;
using morsehgp3d::spatial::ExactHPolytopeHalfspace3;
using morsehgp3d::spatial::ExactHPolytopeHalfspaceRole;
using morsehgp3d::spatial::HPolytopeConstraintDomain;
using morsehgp3d::spatial::HPolytopeConstraintId;
using morsehgp3d::spatial::build_exact_bounded_h_polytope_reference;

static_assert(!std::is_copy_constructible_v<HPolytopeProposalContext>);
static_assert(!std::is_copy_assignable_v<HPolytopeProposalContext>);
static_assert(std::is_nothrow_move_constructible_v<HPolytopeProposalContext>);
static_assert(std::is_nothrow_move_assignable_v<HPolytopeProposalContext>);

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Exception, typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const Exception&) {
    return;
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "FAIL: " << message << " (unexpected exception: "
              << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] std::uint64_t word(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] ExactDyadicAabb3 unit_box() {
  return ExactDyadicAabb3{
      {word(-1.0), word(-1.0), word(-1.0)},
      {word(1.0), word(1.0), word(1.0)}};
}

[[nodiscard]] HPolytopeConstraintId constraint_id(
    std::uint64_t first,
    std::uint64_t second = 0U,
    HPolytopeConstraintDomain domain =
        HPolytopeConstraintDomain::generic_affine) {
  return HPolytopeConstraintId{domain, first, second};
}

[[nodiscard]] ExactAffineForm3 form(int a, int b, int c, int d) {
  return ExactAffineForm3::from_integer_coefficients(
      {BigInt{a}, BigInt{b}, BigInt{c}, BigInt{d}});
}

[[nodiscard]] ExactHPolytopeHalfspace3 halfspace(
    std::uint64_t id,
    int a,
    int b,
    int c,
    int d,
    ExactHPolytopeHalfspaceRole role =
        ExactHPolytopeHalfspaceRole::new_clip) {
  return ExactHPolytopeHalfspace3{
      constraint_id(id), role, form(a, b, c, d)};
}

struct BatchInput {
  std::vector<std::uint64_t> cell_ids;
  std::vector<std::size_t> offsets;
  std::vector<ExactHPolytopeHalfspace3> halfspaces;
};

[[nodiscard]] BatchInput batch_input(
    std::vector<std::pair<
        std::uint64_t,
        std::vector<ExactHPolytopeHalfspace3>>> cells) {
  BatchInput result;
  result.offsets.push_back(0U);
  for (auto& [cell_id, cell_halfspaces] : cells) {
    result.cell_ids.push_back(cell_id);
    result.halfspaces.insert(
        result.halfspaces.end(),
        std::make_move_iterator(cell_halfspaces.begin()),
        std::make_move_iterator(cell_halfspaces.end()));
    result.offsets.push_back(result.halfspaces.size());
  }
  return result;
}

[[nodiscard]] BatchInput cube_input(std::uint64_t cell_id = 7U) {
  return batch_input({{cell_id, {}}});
}

[[nodiscard]] HPolytopeProposalBatchResult build(
    HPolytopeProposalContext& context,
    const BatchInput& input,
    ExactBoundedHPolytopeReferenceBudget budget = {}) {
  return context.build(
      input.cell_ids, input.offsets, input.halfspaces, budget);
}

[[nodiscard]] ExactBoundedHPolytopeReferenceResult reference(
    const std::vector<ExactHPolytopeHalfspace3>& halfspaces) {
  return build_exact_bounded_h_polytope_reference(
      halfspaces, unit_box());
}

[[nodiscard]] std::map<std::uint64_t, ExactBoundedHPolytopeReferenceResult>
reference_by_id(const BatchInput& input) {
  std::map<std::uint64_t, ExactBoundedHPolytopeReferenceResult> result;
  for (std::size_t index = 0U; index < input.cell_ids.size(); ++index) {
    const auto begin = input.halfspaces.begin() +
                       static_cast<std::ptrdiff_t>(input.offsets[index]);
    const auto end = input.halfspaces.begin() +
                     static_cast<std::ptrdiff_t>(input.offsets[index + 1U]);
    result.emplace(
        input.cell_ids[index],
        reference(std::vector<ExactHPolytopeHalfspace3>{begin, end}));
  }
  return result;
}

void check_exact_cells(
    const HPolytopeProposalBatchResult& result,
    const std::map<std::uint64_t, ExactBoundedHPolytopeReferenceResult>&
        expected,
    const std::string& label) {
  check(
      result.cell_results.size() == expected.size(),
      label + " returns every canonical exact cell");
  for (const auto& cell : result.cell_results) {
    const auto iterator = expected.find(cell.cell_id);
    check(
        iterator != expected.end(),
        label + " returns only requested canonical cell ids");
    if (iterator != expected.end()) {
      check(
          cell.exact_result == iterator->second,
          label + " reconstructs cell " + std::to_string(cell.cell_id) +
              " independently with 7.6");
    }
  }
}

void test_cube_transcript_and_audit() {
  reset_fake_gpu_h_polytope_proposal();
  const BatchInput input = cube_input();
  HPolytopeProposalContext context{unit_box(), 20U};
  const HPolytopeProposalBatchResult result = build(context, input);

  check(
      result.decision ==
              HPolytopeProposalBatchDecision::exact_recertified_local &&
          result.requirements.size() == 1U &&
          result.requirements[0U].boundary_plane_count == 6U &&
          result.requirements[0U].exhaustive_proposal_record_count == 20U,
      "the cube derives B=6 and the exhaustive C(6,3)=20 transcript");
  check(
      result.cell_results.size() == 1U &&
          result.cell_results[0U].status ==
              HPolytopeProposalCellStatus::
                  validated_exhaustive_transcript &&
          result.cell_results[0U].exact_result == reference({}),
      "the healthy cube transcript stays proposal-only beside the exact cube");
  check(
      result.proposal_offsets == std::vector<std::size_t>({0U, 20U}) &&
          result.proposal_records.size() == 20U,
      "the cube closes one direct slot per combinadic ordinal");
  check(
      result.audit.required_exhaustive_proposal_record_count == 20U &&
          result.audit.initialized_proposal_record_count == 20U &&
          result.audit.unknown_proposal_record_count == 12U &&
          result.audit.strict_reject_proposal_record_count == 0U &&
          result.audit.survivor_proposal_record_count == 8U &&
          result.audit.exact_plane_triple_replay_count == 20U &&
          result.audit.exact_unique_vertex_recertification_count == 8U,
      "the cube separates unknown triples, eight survivors, and exact replay");
  check(
      result.audit.canonical_cell_order_validated &&
          result.audit.exhaustive_transcript_validated &&
          result.audit.cpu_exact_recertification_complete &&
          !result.audit.global_status_published &&
          std::string{result.audit.proposal_semantics}.find("proposal_only") !=
              std::string::npos,
      "the audit closes only proposal and local CPU semantics");
  check(
      fake_gpu_h_polytope_proposal_launch_count() == 1U &&
          fake_gpu_h_polytope_proposal_last_cell_count() == 1U &&
          fake_gpu_h_polytope_proposal_last_boundary_count() == 6U &&
          fake_gpu_h_polytope_proposal_last_record_capacity() == 20U,
      "the fake launcher receives the exact cube batch and physical capacity");

  std::array<std::size_t, 3> previous{0U, 0U, 0U};
  bool first_record = true;
  for (const auto& record : result.proposal_records) {
    const std::array<std::size_t, 3> current{
        record.first_boundary_index,
        record.second_boundary_index,
        record.third_boundary_index};
    check(
        current[0U] < current[1U] && current[1U] < current[2U] &&
            (first_record || previous < current),
        "direct transcript slots follow strict lexicographic triplet order");
    previous = current;
    first_record = false;
  }
}

void test_geometry_matrix_and_all_record_kinds() {
  reset_fake_gpu_h_polytope_proposal();
  const BatchInput input = batch_input({
      {50U, {}},
      {10U, {halfspace(1U, 1, 0, 0, 0)}},
      {40U, {halfspace(2U, 1, 1, 1, 0)}},
      {20U,
       {halfspace(3U, 0, 0, 1, 0),
        halfspace(4U, 0, 0, -1, 0)}},
      {30U, {halfspace(5U, 1, 0, 0, 2)}}});
  HPolytopeProposalContext context{unit_box(), 200U};
  const HPolytopeProposalBatchResult result = build(context, input);
  check_exact_cells(result, reference_by_id(input), "the geometry matrix");
  check(
      result.audit.unknown_proposal_record_count > 0U &&
          result.audit.strict_reject_proposal_record_count > 0U &&
          result.audit.survivor_proposal_record_count > 0U,
      "cube, cuts, low dimension, and emptiness exercise all three record statuses");
  check(
      result.cell_results.size() == 5U &&
          result.cell_results[0U].cell_id == 10U &&
          result.cell_results[1U].cell_id == 20U &&
          result.cell_results[2U].cell_id == 30U &&
          result.cell_results[3U].cell_id == 40U &&
          result.cell_results[4U].cell_id == 50U,
      "the public CSR is sorted by canonical cell id");
  const auto low_dimension = std::find_if(
      result.cell_results.begin(),
      result.cell_results.end(),
      [](const auto& cell) { return cell.cell_id == 20U; });
  const auto empty = std::find_if(
      result.cell_results.begin(),
      result.cell_results.end(),
      [](const auto& cell) { return cell.cell_id == 30U; });
  check(
      low_dimension != result.cell_results.end() &&
          low_dimension->exact_result.affine_dimension == 2U,
      "opposite z constraints retain the exact dimension-two square");
  check(
      empty != result.cell_results.end() &&
          empty->exact_result.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_empty,
      "the proposed transcript cannot hide the exact empty cell");
}

void test_canonical_csr_and_digest_permutations() {
  reset_fake_gpu_h_polytope_proposal();
  const ExactHPolytopeHalfspace3 x = halfspace(
      12U, 1, 0, 0, 0,
      ExactHPolytopeHalfspaceRole::parent_constraint);
  const ExactHPolytopeHalfspace3 y = halfspace(11U, 0, 1, 0, 0);
  const ExactHPolytopeHalfspace3 oblique =
      halfspace(3U, 1, 1, 1, 0);
  const BatchInput first = batch_input({{9U, {x, y}}, {3U, {oblique}}});
  const BatchInput second = batch_input({{3U, {oblique}}, {9U, {y, x}}});
  HPolytopeProposalContext first_context{unit_box(), 100U};
  HPolytopeProposalContext second_context{unit_box(), 100U};
  const auto first_result = build(first_context, first);
  const auto second_result = build(second_context, second);
  check(
      first_result == second_result &&
          first_result.audit.proposal_digest_fnv1a != 0U,
      "cell and halfspace input permutations preserve the canonical CSR and digest");
}

void test_all_unknown_and_conservative_false_positives() {
  reset_fake_gpu_h_polytope_proposal();
  configure_fake_gpu_h_polytope_proposal(
      FakeHPolytopeProposalConfiguration{
          FakeHPolytopeProposalValues::all_unknown,
          FakeHPolytopeProposalCorruption::none});
  HPolytopeProposalContext context{unit_box(), 20U};
  const auto result = build(context, cube_input());
  check(
      result.audit.unknown_proposal_record_count == 20U &&
          result.audit.strict_reject_proposal_record_count == 0U &&
          result.audit.survivor_proposal_record_count == 0U &&
          result.cell_results[0U].exact_result == reference({}),
      "an all-unknown transcript still reconstructs the exact cube on CPU");

  reset_fake_gpu_h_polytope_proposal();
  HPolytopeProposalContext superset_context{unit_box(), 20U};
  const auto superset = build(superset_context, cube_input());
  const bool has_strict_false_positive = std::any_of(
      superset.proposal_records.begin(),
      superset.proposal_records.end(),
      [](const auto& record) {
        return record.status ==
                   HPolytopeProposalRecordStatus::proposed_survivor &&
               record.could_be_active_boundary_mask == UINT64_C(0x3f);
      });
  check(
      has_strict_false_positive,
      "a could-be-active mask may conservatively contain non-active boundaries");
}

void test_nonpoisoning_fallbacks() {
  reset_fake_gpu_h_polytope_proposal();
  HPolytopeProposalContext zero_capacity{unit_box(), 0U};
  const auto capacity = build(zero_capacity, cube_input());
  check(
      capacity.cell_results[0U].status ==
              HPolytopeProposalCellStatus::
                  exact_fallback_capacity_exhausted &&
          capacity.proposal_offsets == std::vector<std::size_t>({0U, 0U}) &&
          capacity.proposal_records.empty() &&
          capacity.cell_results[0U].exact_result == reference({}) &&
          fake_gpu_h_polytope_proposal_launch_count() == 0U,
      "zero physical capacity falls back exactly without launching");

  reset_fake_gpu_h_polytope_proposal();
  configure_fake_gpu_h_polytope_proposal(
      FakeHPolytopeProposalConfiguration{
          FakeHPolytopeProposalValues::whole_cell_interval_fallback,
          FakeHPolytopeProposalCorruption::none});
  HPolytopeProposalContext interval_context{unit_box(), 20U};
  const auto interval = build(interval_context, cube_input());
  check(
      interval.cell_results[0U].status ==
              HPolytopeProposalCellStatus::exact_fallback_interval_unknown &&
          interval.proposal_records.empty() &&
          interval.cell_results[0U].exact_result == reference({}),
      "an interval fallback publishes only the independent exact cell");
  configure_fake_gpu_h_polytope_proposal(
      FakeHPolytopeProposalConfiguration{});
  const auto after_interval = build(interval_context, cube_input());
  check(
      after_interval.cell_results[0U].status ==
              HPolytopeProposalCellStatus::
                  validated_exhaustive_transcript &&
          fake_gpu_h_polytope_proposal_launch_count() == 2U,
      "a prudent interval fallback does not poison its context");

  reset_fake_gpu_h_polytope_proposal();
  const std::uint64_t maximum_cell_id =
      std::numeric_limits<std::uint64_t>::max();
  const BatchInput positive_constant = batch_input({
      {maximum_cell_id, {halfspace(77U, 0, 0, 0, 1)}},
      {1U, {}}});
  HPolytopeProposalContext constant_context{unit_box(), 20U};
  const auto constant_result = build(constant_context, positive_constant);
  check(
      constant_result.cell_results.size() == 2U &&
          constant_result.cell_results[0U].cell_id == 1U &&
          constant_result.cell_results[0U].status ==
              HPolytopeProposalCellStatus::
                  validated_exhaustive_transcript &&
          constant_result.cell_results[1U].cell_id == maximum_cell_id &&
          constant_result.cell_results[1U].status ==
              HPolytopeProposalCellStatus::exact_fallback_interval_unknown &&
          constant_result.cell_results[1U].exact_result.decision ==
              ExactBoundedHPolytopeReferenceDecision::complete_empty &&
          constant_result.proposal_offsets ==
              std::vector<std::size_t>({0U, 20U, 20U}),
      "a positive exact constant forces an empty-row interval fallback while UINT64_MAX remains a valid cell id");
  const auto after_constant = build(constant_context, cube_input(2U));
  check(
      after_constant.audit.cpu_exact_recertification_complete &&
          fake_gpu_h_polytope_proposal_launch_count() == 2U,
      "a host-forced constant fallback does not poison the mixed-batch context");

  reset_fake_gpu_h_polytope_proposal();
  BigInt huge{1};
  huge <<= 2000U;
  const ExactHPolytopeHalfspace3 huge_x{
      constraint_id(99U),
      ExactHPolytopeHalfspaceRole::new_clip,
      ExactAffineForm3::from_integer_coefficients(
          {huge, BigInt{0}, BigInt{0}, BigInt{0}})};
  const BatchInput unsupported = batch_input({{4U, {huge_x}}});
  HPolytopeProposalContext projection_context{unit_box(), 35U};
  const auto projection = build(projection_context, unsupported);
  check(
      projection.cell_results[0U].status ==
              HPolytopeProposalCellStatus::
                  exact_fallback_unsupported_projection &&
          projection.proposal_records.empty() &&
          fake_gpu_h_polytope_proposal_launch_count() == 0U,
      "an unprojectable coefficient takes a whole-cell CPU fallback without launch");
  const auto after_projection = build(projection_context, cube_input());
  check(
      after_projection.cell_results[0U].status ==
              HPolytopeProposalCellStatus::
                  validated_exhaustive_transcript &&
          fake_gpu_h_polytope_proposal_launch_count() == 1U,
      "an unsupported projection does not poison later supported work");

  reset_fake_gpu_h_polytope_proposal();
  HPolytopeProposalContext partial_capacity{unit_box(), 20U};
  const BatchInput two_cubes = batch_input({{8U, {}}, {2U, {}}});
  const auto mixed = build(partial_capacity, two_cubes);
  check(
      mixed.cell_results.size() == 2U &&
          mixed.cell_results[0U].cell_id == 2U &&
          mixed.cell_results[0U].status ==
              HPolytopeProposalCellStatus::validated_exhaustive_transcript &&
          mixed.cell_results[1U].cell_id == 8U &&
          mixed.cell_results[1U].status ==
              HPolytopeProposalCellStatus::
                  exact_fallback_capacity_exhausted &&
          mixed.proposal_offsets ==
              std::vector<std::size_t>({0U, 20U, 20U}),
      "physical capacity admits only whole canonical rows and never truncates the next cell");
}

void check_corruption(FakeHPolytopeProposalCorruption corruption,
                      std::size_t capacity,
                      const std::string& label) {
  reset_fake_gpu_h_polytope_proposal();
  configure_fake_gpu_h_polytope_proposal(
      FakeHPolytopeProposalConfiguration{
          FakeHPolytopeProposalValues::actual_binary64_recipe,
          corruption});
  HPolytopeProposalContext poisoned{unit_box(), capacity};
  const BatchInput input = cube_input();
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(build(poisoned, input)); },
      label + " is rejected fail-closed");
  configure_fake_gpu_h_polytope_proposal(
      FakeHPolytopeProposalConfiguration{});
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(build(poisoned, input)); },
      label + " poisons only its resident context");
  check(
      fake_gpu_h_polytope_proposal_launch_count() == 1U,
      label + " prevents a second launch from the poisoned context");

  HPolytopeProposalContext fresh{unit_box(), 20U};
  const auto recovered = build(fresh, input);
  check(
      recovered.decision ==
              HPolytopeProposalBatchDecision::exact_recertified_local &&
          recovered.audit.cpu_exact_recertification_complete &&
          fake_gpu_h_polytope_proposal_launch_count() == 2U,
      label + " remains isolated from a fresh context");
}

void test_corruption_matrix_and_poisoning() {
  const std::array<std::pair<FakeHPolytopeProposalCorruption, const char*>,
                   15>
      cases{{
          {FakeHPolytopeProposalCorruption::missing_slot, "missing slot"},
          {FakeHPolytopeProposalCorruption::duplicate_slot,
           "duplicated slot"},
          {FakeHPolytopeProposalCorruption::omitted_true_incidence,
           "omitted exact incidence"},
          {FakeHPolytopeProposalCorruption::out_of_range_incidence_bit,
           "out-of-range incidence bit"},
          {FakeHPolytopeProposalCorruption::nonfinite_survivor_coordinate,
           "nonfinite survivor interval"},
          {FakeHPolytopeProposalCorruption::false_strict_reject,
           "false strict reject"},
          {FakeHPolytopeProposalCorruption::wrong_epoch, "wrong epoch"},
          {FakeHPolytopeProposalCorruption::wrong_batch_epoch,
           "unauthenticated whole-batch epoch"},
          {FakeHPolytopeProposalCorruption::double_epoch_advance,
           "double epoch advance"},
          {FakeHPolytopeProposalCorruption::wrong_ordinal,
           "wrong combinadic ordinal"},
          {FakeHPolytopeProposalCorruption::invalid_offsets,
           "invalid output CSR offset"},
          {FakeHPolytopeProposalCorruption::invalid_cell_id,
           "forged cell id"},
          {FakeHPolytopeProposalCorruption::invalid_cell_status,
           "invalid cell status"},
          {FakeHPolytopeProposalCorruption::invalid_record_status,
           "invalid record status"},
          {FakeHPolytopeProposalCorruption::simulated_async_failure,
           "asynchronous device failure"},
      }};
  for (const auto& [corruption, label] : cases) {
    check_corruption(corruption, 20U, label);
  }
  check_corruption(
      FakeHPolytopeProposalCorruption::tail_write,
      21U,
      "write into the physical-capacity tail");

  reset_fake_gpu_h_polytope_proposal();
  HPolytopeProposalContext stale{unit_box(), 20U};
  const BatchInput input = cube_input();
  const auto first = build(stale, input);
  check(
      first.audit.buffer_epoch == 1U,
      "the stale-epoch fixture first establishes one healthy epoch");
  configure_fake_gpu_h_polytope_proposal(
      FakeHPolytopeProposalConfiguration{
          FakeHPolytopeProposalValues::actual_binary64_recipe,
          FakeHPolytopeProposalCorruption::stale_epoch_without_advance});
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(build(stale, input)); },
      "a repeated current epoch without advance is rejected");
  configure_fake_gpu_h_polytope_proposal(
      FakeHPolytopeProposalConfiguration{});
  check_throws<std::runtime_error>(
      [&] { static_cast<void>(build(stale, input)); },
      "a stale epoch poisons its resident context");
  check(
      fake_gpu_h_polytope_proposal_launch_count() == 2U,
      "the poisoned stale-epoch context cannot launch a third transaction");
}

void test_preflight_validation_and_exact_budget() {
  reset_fake_gpu_h_polytope_proposal();
  HPolytopeProposalContext context{unit_box(), 20U};
  const std::array<std::uint64_t, 1> one_cell{1U};
  const std::array<std::size_t, 1> bad_offset_count{0U};
  const std::array<ExactHPolytopeHalfspace3, 0> no_halfspaces{};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.build(
            one_cell, bad_offset_count, no_halfspaces));
      },
      "a nonclosing input CSR is rejected before launch");

  const std::array<std::uint64_t, 2> duplicate_cells{2U, 2U};
  const std::array<std::size_t, 3> empty_offsets{0U, 0U, 0U};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.build(
            duplicate_cells, empty_offsets, no_halfspaces));
      },
      "duplicate cell ids are rejected before launch");
  const std::array<std::uint64_t, 2> distinct_cells{1U, 2U};
  const std::array<std::size_t, 3> nonmonotone_offsets{0U, 1U, 0U};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.build(
            distinct_cells, nonmonotone_offsets, no_halfspaces));
      },
      "nonmonotone input CSR offsets are rejected before launch");

  const HPolytopeConstraintId duplicate_id = constraint_id(4U);
  const std::array<ExactHPolytopeHalfspace3, 2> duplicate_halfspaces{
      ExactHPolytopeHalfspace3{
          duplicate_id,
          ExactHPolytopeHalfspaceRole::parent_constraint,
          form(1, 0, 0, 0)},
      ExactHPolytopeHalfspace3{
          duplicate_id,
          ExactHPolytopeHalfspaceRole::new_clip,
          form(0, 1, 0, 0)}};
  const std::array<std::size_t, 2> two_offsets{0U, 2U};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.build(
            one_cell, two_offsets, duplicate_halfspaces));
      },
      "duplicate constraint ids are rejected before launch");

  const std::array<ExactHPolytopeHalfspace3, 1> invalid_domain{
      ExactHPolytopeHalfspace3{
          HPolytopeConstraintId{
              static_cast<HPolytopeConstraintDomain>(255), 1U, 0U},
          ExactHPolytopeHalfspaceRole::new_clip,
          form(1, 0, 0, 0)}};
  const std::array<std::size_t, 2> one_offset{0U, 1U};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.build(
            one_cell, one_offset, invalid_domain));
      },
      "an invalid constraint domain is rejected before launch");

  const std::array<ExactHPolytopeHalfspace3, 1> invalid_role{
      ExactHPolytopeHalfspace3{
          constraint_id(8U),
          static_cast<ExactHPolytopeHalfspaceRole>(255),
          form(1, 0, 0, 0)}};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.build(
            one_cell, one_offset, invalid_role));
      },
      "an invalid halfspace role is rejected before launch");

  std::vector<ExactHPolytopeHalfspace3> too_many;
  for (std::uint64_t index = 0U; index < 56U; ++index) {
    too_many.push_back(halfspace(index, 1, 0, 0, 0));
  }
  const std::array<std::size_t, 2> too_many_offsets{0U, 56U};
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.build(
            one_cell, too_many_offsets, too_many));
      },
      "a 56-halfspace cell is outside the bounded 7.6 domain");

  ExactBoundedHPolytopeReferenceBudget excessive_budget;
  ++excessive_budget.maximum_vertex_count;
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(context.build(
            one_cell,
            std::array<std::size_t, 2>{0U, 0U},
            no_halfspaces,
            excessive_budget));
      },
      "an exact budget cannot exceed its trusted cap");

  ExactBoundedHPolytopeReferenceBudget insufficient_budget;
  insufficient_budget.maximum_boundary_count = 5U;
  const auto insufficient = context.build(
      one_cell,
      std::array<std::size_t, 2>{0U, 0U},
      no_halfspaces,
      insufficient_budget);
  check(
      insufficient.decision ==
              HPolytopeProposalBatchDecision::insufficient_exact_budget &&
          insufficient.requirements.size() == 1U &&
          insufficient.requirements[0U].boundary_plane_count == 6U &&
          insufficient.requirements[0U]
                  .exhaustive_proposal_record_count == 20U &&
          insufficient.cell_results.empty() &&
          insufficient.proposal_records.empty(),
      "an exact budget one below returns requirements without geometry or proposal");

  const auto healthy = build(context, cube_input());
  check(
      healthy.audit.cpu_exact_recertification_complete &&
          fake_gpu_h_polytope_proposal_launch_count() == 1U,
      "all pre-GPU validation failures leave the context usable");

  ExactDyadicAabb3 bad_box = unit_box();
  bad_box.upper_binary64_bits[0U] = bad_box.lower_binary64_bits[0U];
  check_throws<std::invalid_argument>(
      [&] { HPolytopeProposalContext bad{bad_box, 20U}; },
      "a flat clipping box is rejected by context construction");
  ExactDyadicAabb3 noncanonical_box = unit_box();
  noncanonical_box.lower_binary64_bits[0U] = word(-0.0);
  check_throws<std::invalid_argument>(
      [&] { HPolytopeProposalContext bad{noncanonical_box, 20U}; },
      "a noncanonical negative-zero box bound is rejected before allocation");
  check_throws<std::length_error>(
      [] {
        const std::size_t overflowing_capacity =
            std::numeric_limits<std::size_t>::max() /
                (14U * sizeof(std::uint64_t)) +
            1U;
        HPolytopeProposalContext bad{unit_box(), overflowing_capacity};
      },
      "physical proposal byte capacity overflow is rejected before allocation");
}

void test_trusted_cap_requirements_without_long_geometry() {
  reset_fake_gpu_h_polytope_proposal();
  std::vector<ExactHPolytopeHalfspace3> halfspaces;
  halfspaces.reserve(55U);
  for (std::uint64_t index = 0U; index < 55U; ++index) {
    halfspaces.push_back(halfspace(index, 1, 0, 0, 0));
  }
  const BatchInput input = batch_input({{1U, halfspaces}});
  ExactBoundedHPolytopeReferenceBudget short_budget;
  short_budget.maximum_plane_triple_count = 35989U;
  HPolytopeProposalContext context{unit_box(), 0U};
  const auto result = build(context, input, short_budget);
  check(
      result.decision ==
              HPolytopeProposalBatchDecision::insufficient_exact_budget &&
          result.requirements.size() == 1U &&
          result.requirements[0U].boundary_plane_count == 61U &&
          result.requirements[0U].exhaustive_proposal_record_count == 35990U &&
          result.audit.required_exhaustive_proposal_record_count == 35990U &&
          result.cell_results.empty() &&
          fake_gpu_h_polytope_proposal_launch_count() == 0U,
      "the B=61 cap reports 35990 slots entirely in preflight");
}

void test_strings_and_move_only_context() {
  using morsehgp3d::gpu::to_string;
  check(
      to_string(HPolytopeProposalBatchDecision::exact_recertified_local) ==
              "exact_recertified_local" &&
          to_string(
              HPolytopeProposalBatchDecision::insufficient_exact_budget) ==
              "insufficient_exact_budget" &&
          to_string(
              HPolytopeProposalCellStatus::
                  validated_exhaustive_transcript) ==
              "validated_exhaustive_transcript" &&
          to_string(
              HPolytopeProposalCellStatus::
                  exact_fallback_interval_unknown) ==
              "exact_fallback_interval_unknown" &&
          to_string(
              HPolytopeProposalCellStatus::
                  exact_fallback_capacity_exhausted) ==
              "exact_fallback_capacity_exhausted" &&
          to_string(
              HPolytopeProposalCellStatus::
                  exact_fallback_unsupported_projection) ==
              "exact_fallback_unsupported_projection" &&
          to_string(
              HPolytopeProposalRecordStatus::unknown_requires_cpu_exact) ==
              "unknown_requires_cpu_exact" &&
          to_string(
              HPolytopeProposalRecordStatus::proposed_strict_reject) ==
              "proposed_strict_reject" &&
          to_string(HPolytopeProposalRecordStatus::proposed_survivor) ==
              "proposed_survivor",
      "all 7.7 public enum strings are stable");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(to_string(
            static_cast<HPolytopeProposalBatchDecision>(255)));
      },
      "an invalid batch decision cannot be stringified");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(to_string(
            static_cast<HPolytopeProposalCellStatus>(255)));
      },
      "an invalid cell status cannot be stringified");
  check_throws<std::invalid_argument>(
      [&] {
        static_cast<void>(to_string(
            static_cast<HPolytopeProposalRecordStatus>(255)));
      },
      "an invalid record status cannot be stringified");

  reset_fake_gpu_h_polytope_proposal();
  HPolytopeProposalContext source{unit_box(), 20U};
  check(
      source.maximum_total_proposal_record_count() == 20U &&
          source.clipping_box() == unit_box(),
      "the context exposes its immutable box and physical capacity");
  HPolytopeProposalContext moved{std::move(source)};
  check_throws<std::invalid_argument>(
      [&] { static_cast<void>(build(source, cube_input())); },
      "a moved-from proposal context is not queryable");
  const auto result = build(moved, cube_input());
  check(
      result.audit.cpu_exact_recertification_complete,
      "the moved-to proposal context retains its resident state");
}

}  // namespace

int main() {
  test_cube_transcript_and_audit();
  test_geometry_matrix_and_all_record_kinds();
  test_canonical_csr_and_digest_permutations();
  test_all_unknown_and_conservative_false_positives();
  test_nonpoisoning_fallbacks();
  test_corruption_matrix_and_poisoning();
  test_preflight_validation_and_exact_budget();
  test_trusted_cap_requirements_without_long_geometry();
  test_strings_and_move_only_context();

  if (failures != 0) {
    std::cerr << failures
              << " GPU H-polytope proposal context test(s) failed\n";
    return 1;
  }
  std::cout << "GPU H-polytope proposal context tests passed\n";
  return 0;
}
