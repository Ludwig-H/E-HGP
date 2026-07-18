#include "fake_gpu_spatial_bounds_launchers.hpp"

#include "morsehgp3d/gpu/spatial_bounds.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using morsehgp3d::exact::BigInt;
using morsehgp3d::exact::ExactLevel;
using morsehgp3d::exact::ExactRational3;
using morsehgp3d::gpu::DirectedEnclosureStatus;
using morsehgp3d::gpu::SpatialBoundsAudit;
using morsehgp3d::gpu::SpatialBoundsContext;
using morsehgp3d::gpu::SpatialBoundsDecision;
using morsehgp3d::gpu::test_support::FakeSpatialBoundsProposalConfiguration;
using morsehgp3d::gpu::test_support::FakeSpatialBoundsProposalCorruption;
using morsehgp3d::gpu::test_support::FakeSpatialBoundsProposalPermutation;
using morsehgp3d::gpu::test_support::FakeSpatialBoundsProposalValues;
using morsehgp3d::gpu::test_support::configure_fake_gpu_spatial_bounds;
using morsehgp3d::gpu::test_support::fake_gpu_spatial_bounds_last_box_count;
using morsehgp3d::gpu::test_support::fake_gpu_spatial_bounds_last_cutoff_bits;
using morsehgp3d::gpu::test_support::
    fake_gpu_spatial_bounds_last_query_lower_bits;
using morsehgp3d::gpu::test_support::
    fake_gpu_spatial_bounds_last_query_upper_bits;
using morsehgp3d::gpu::test_support::fake_gpu_spatial_bounds_launch_count;
using morsehgp3d::gpu::test_support::reset_fake_gpu_spatial_bounds;
using morsehgp3d::spatial::ExactDyadicAabb3;

static_assert(!std::is_copy_constructible_v<SpatialBoundsContext>);
static_assert(!std::is_copy_assignable_v<SpatialBoundsContext>);
static_assert(std::is_nothrow_move_constructible_v<SpatialBoundsContext>);
static_assert(std::is_nothrow_move_assignable_v<SpatialBoundsContext>);

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
    std::cerr << "FAIL: " << message
              << " (unexpected exception: " << error.what() << ")\n";
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

[[nodiscard]] std::uint64_t word(double value) {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] ExactDyadicAabb3 box(
    double lower_x,
    double lower_y,
    double lower_z,
    double upper_x,
    double upper_y,
    double upper_z) {
  return ExactDyadicAabb3{
      {word(lower_x), word(lower_y), word(lower_z)},
      {word(upper_x), word(upper_y), word(upper_z)}};
}

[[nodiscard]] ExactDyadicAabb3 point_box(
    double x, double y = 0.0, double z = 0.0) {
  return box(x, y, z, x, y, z);
}

[[nodiscard]] ExactRational3 origin() {
  return ExactRational3{};
}

[[nodiscard]] std::vector<SpatialBoundsDecision> decisions(
    std::initializer_list<SpatialBoundsDecision> values) {
  return std::vector<SpatialBoundsDecision>{values};
}

void check_supported_audit(
    const SpatialBoundsAudit& audit,
    std::size_t box_count,
    std::size_t prune_count,
    std::size_t visit_count,
    std::size_t unknown_count,
    std::uint64_t epoch,
    const std::string& label) {
  check(
      audit.gpu_input_box_count == box_count &&
          audit.gpu_output_record_count == box_count &&
          audit.gpu_unique_box_index_count == box_count,
      label + " closes the GPU box-index permutation");
  check(
      audit.gpu_prune_proposal_count == prune_count &&
          audit.gpu_visit_proposal_count == visit_count &&
          audit.gpu_unknown_proposal_count == unknown_count,
      label + " reports every proposal decision");
  check(
      audit.gpu_launch_count == 1U && audit.buffer_epoch == epoch,
      label + " reports one serialized launch and its epoch");
  check(
      audit.cpu_exact_prune_recertification_count == prune_count &&
          audit.certified_prune_count == prune_count,
      label + " recertifies every proposed strict prune exactly");
  check(
      audit.unsupported_range_fallback_count == 0U,
      label + " does not claim a range fallback");
  check(
      audit.proposal_permutation_complete &&
          audit.cpu_exact_recertification_complete &&
          audit.all_boxes_classified,
      label + " publishes only a closed classification");
  check(
      audit.query_enclosure ==
              std::array<DirectedEnclosureStatus, 3>{
                  DirectedEnclosureStatus::exact,
                  DirectedEnclosureStatus::exact,
                  DirectedEnclosureStatus::exact} &&
          audit.cutoff_enclosure == DirectedEnclosureStatus::exact,
      label + " preserves exactly representable query inputs");
  check(
      audit.minimum_certified_strict_margin.has_value() ==
          (prune_count != 0U),
      label + " binds a strict margin exactly to certified prunes");
  if (audit.minimum_certified_strict_margin.has_value()) {
    check(
        *audit.minimum_certified_strict_margin > ExactLevel{},
        label + " exposes a strictly positive minimum margin");
  }
}

void test_all_unknown_and_visit_are_nonterminal_hints() {
  reset_fake_gpu_spatial_bounds();
  const std::array<ExactDyadicAabb3, 2> boxes{
      point_box(0.0), point_box(0.5)};
  SpatialBoundsContext context{std::span<const ExactDyadicAabb3>{boxes}};

  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
      FakeSpatialBoundsProposalPermutation::canonical,
      FakeSpatialBoundsProposalValues::all_unknown,
      FakeSpatialBoundsProposalCorruption::none});
  const auto unknown =
      context.classify_strict_prune(origin(), ExactLevel{BigInt{1}});
  check(
      unknown.decisions == decisions({
                               SpatialBoundsDecision::unknown,
                               SpatialBoundsDecision::unknown}),
      "all-unknown proposals remain explicit nonterminal hints");
  check_supported_audit(
      unknown.audit, 2U, 0U, 0U, 2U, 1U, "the all-unknown query");

  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
      FakeSpatialBoundsProposalPermutation::canonical,
      FakeSpatialBoundsProposalValues::all_visit,
      FakeSpatialBoundsProposalCorruption::none});
  const auto visit =
      context.classify_strict_prune(origin(), ExactLevel{BigInt{1}});
  check(
      visit.decisions == decisions({
                             SpatialBoundsDecision::visit,
                             SpatialBoundsDecision::visit}),
      "visit proposals remain safe traversal hints");
  check_supported_audit(
      visit.audit, 2U, 0U, 2U, 0U, 2U, "the all-visit query");
  check(
      fake_gpu_spatial_bounds_launch_count() == 2U &&
          fake_gpu_spatial_bounds_last_box_count() == boxes.size(),
      "supported hint queries launch once each over the complete box batch");
}

void test_valid_prunes_have_exact_positive_margins() {
  reset_fake_gpu_spatial_bounds();
  const std::array<ExactDyadicAabb3, 2> boxes{
      point_box(2.0), point_box(3.0)};
  SpatialBoundsContext context{std::span<const ExactDyadicAabb3>{boxes}};
  const auto result =
      context.classify_strict_prune(origin(), ExactLevel{BigInt{1}});

  check(
      result.decisions == decisions({
                              SpatialBoundsDecision::prune,
                              SpatialBoundsDecision::prune}),
      "strictly separated boxes are certified as prunable");
  check_supported_audit(
      result.audit, 2U, 2U, 0U, 0U, 1U, "the strict-prune query");
  check(
      result.audit.minimum_certified_strict_margin ==
          std::optional<ExactLevel>{ExactLevel{BigInt{3}}},
      "the minimum exact margin is distance four minus cutoff one");
  check(
      fake_gpu_spatial_bounds_last_query_lower_bits() ==
              std::array<std::uint64_t, 3>{0U, 0U, 0U} &&
          fake_gpu_spatial_bounds_last_query_upper_bits() ==
              std::array<std::uint64_t, 3>{0U, 0U, 0U} &&
          fake_gpu_spatial_bounds_last_cutoff_bits() ==
              std::array<std::uint64_t, 2>{word(1.0), word(1.0)},
      "the fake launcher receives the exact directed query and cutoff words");
}

void test_equality_never_prunes_and_permutation_is_canonicalized() {
  reset_fake_gpu_spatial_bounds();
  const std::array<ExactDyadicAabb3, 3> boxes{
      point_box(1.0),
      point_box(2.0),
      box(-1.0, -1.0, -1.0, 1.0, 1.0, 1.0)};
  SpatialBoundsContext context{std::span<const ExactDyadicAabb3>{boxes}};
  const auto canonical =
      context.classify_strict_prune(origin(), ExactLevel{BigInt{1}});
  check(
      canonical.decisions == decisions({
                                 SpatialBoundsDecision::unknown,
                                 SpatialBoundsDecision::prune,
                                 SpatialBoundsDecision::visit}),
      "AABB lower-bound equality falls back instead of pruning");
  check_supported_audit(
      canonical.audit, 3U, 1U, 1U, 1U, 1U, "the canonical mixed query");

  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
      FakeSpatialBoundsProposalPermutation::reversed,
      FakeSpatialBoundsProposalValues::actual_interval_recipe,
      FakeSpatialBoundsProposalCorruption::none});
  const auto reversed =
      context.classify_strict_prune(origin(), ExactLevel{BigInt{1}});
  check(
      reversed.decisions == canonical.decisions,
      "reversing proposal records preserves canonical box-index decisions");
  check_supported_audit(
      reversed.audit, 3U, 1U, 1U, 1U, 2U, "the reversed mixed query");
  check(
      reversed.audit.proposal_digest_fnv1a ==
          canonical.audit.proposal_digest_fnv1a,
      "the proposal digest is invariant to record permutation");
}

void test_unsupported_range_falls_back_without_launch_or_poisoning() {
  reset_fake_gpu_spatial_bounds();
  const std::array<ExactDyadicAabb3, 2> boxes{
      point_box(1.0), point_box(2.0)};
  SpatialBoundsContext context{std::span<const ExactDyadicAabb3>{boxes}};
  const ExactRational3 unsupported_query{
      BigInt{1} << 1024U, BigInt{0}, BigInt{0}, BigInt{1}};
  const auto unsupported = context.classify_strict_prune(
      unsupported_query, ExactLevel{BigInt{1}});

  check(
      unsupported.decisions == decisions({
                                   SpatialBoundsDecision::unknown,
                                   SpatialBoundsDecision::unknown}),
      "an unsupported directed enclosure falls back for every box");
  check(
      fake_gpu_spatial_bounds_launch_count() == 0U &&
          unsupported.audit.gpu_launch_count == 0U &&
          unsupported.audit.gpu_output_record_count == 0U,
      "unsupported range is rejected before the fake GPU section");
  check(
      unsupported.audit.query_enclosure[0] ==
              DirectedEnclosureStatus::unsupported_range &&
          unsupported.audit.unsupported_range_fallback_count == boxes.size(),
      "unsupported range is visible in the fallback audit");
  check(
      unsupported.audit.cpu_exact_recertification_complete &&
          unsupported.audit.all_boxes_classified,
      "the unsupported query returns a complete fallback instruction");

  const auto supported =
      context.classify_strict_prune(origin(), ExactLevel{BigInt{1}});
  check(
      fake_gpu_spatial_bounds_launch_count() == 1U &&
          supported.audit.buffer_epoch == 1U,
      "a pre-GPU unsupported range does not poison or advance the context");
}

void test_post_gpu_corruption_poisons_only_its_context() {
  const std::array<ExactDyadicAabb3, 2> boxes{
      point_box(1.0), point_box(2.0)};
  struct CorruptionCase {
    FakeSpatialBoundsProposalCorruption corruption;
    std::size_t expected_launch_count;
    const char* label;
  };
  const std::array<CorruptionCase, 6> cases{{
      {FakeSpatialBoundsProposalCorruption::missing_record,
       1U,
       "missing record"},
      {FakeSpatialBoundsProposalCorruption::duplicate_box_index,
       1U,
       "duplicate box index"},
      {FakeSpatialBoundsProposalCorruption::out_of_range_box_index,
       1U,
       "out-of-range box index"},
      {FakeSpatialBoundsProposalCorruption::invalid_decision,
       1U,
       "invalid decision"},
      {FakeSpatialBoundsProposalCorruption::false_prune,
       1U,
       "false strict prune"},
      {FakeSpatialBoundsProposalCorruption::simulated_gpu_failure,
       0U,
       "GPU launch/copy failure"},
  }};

  for (const CorruptionCase& test_case : cases) {
    reset_fake_gpu_spatial_bounds();
    SpatialBoundsContext context{std::span<const ExactDyadicAabb3>{boxes}};
    configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
        FakeSpatialBoundsProposalPermutation::canonical,
        FakeSpatialBoundsProposalValues::actual_interval_recipe,
        test_case.corruption});
    check_throws<std::runtime_error>(
        [&] {
          static_cast<void>(context.classify_strict_prune(
              origin(), ExactLevel{BigInt{1}}));
        },
        std::string{"post-GPU spatial-bounds corruption is rejected: "} +
            test_case.label);

    configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{});
    check_throws<std::runtime_error>(
        [&] {
          static_cast<void>(context.classify_strict_prune(
              origin(), ExactLevel{BigInt{1}}));
        },
        std::string{"corruption poisons its spatial-bounds context: "} +
            test_case.label);
    check(
        fake_gpu_spatial_bounds_launch_count() ==
            test_case.expected_launch_count,
        std::string{"a poisoned context cannot launch again: "} +
            test_case.label);
  }

  reset_fake_gpu_spatial_bounds();
  SpatialBoundsContext poisoned{std::span<const ExactDyadicAabb3>{boxes}};
  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{
      FakeSpatialBoundsProposalPermutation::canonical,
      FakeSpatialBoundsProposalValues::actual_interval_recipe,
      FakeSpatialBoundsProposalCorruption::missing_record});
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.classify_strict_prune(
            origin(), ExactLevel{BigInt{1}}));
      },
      "the isolated context is poisoned by its own corrupt batch");

  configure_fake_gpu_spatial_bounds(FakeSpatialBoundsProposalConfiguration{});
  SpatialBoundsContext fresh{std::span<const ExactDyadicAabb3>{boxes}};
  const auto fresh_result =
      fresh.classify_strict_prune(origin(), ExactLevel{BigInt{1}});
  check(
      fresh_result.audit.cpu_exact_recertification_complete &&
          fresh_result.audit.proposal_permutation_complete &&
          fake_gpu_spatial_bounds_launch_count() == 2U,
      "a fresh independent context remains usable after another is poisoned");
  check_throws<std::runtime_error>(
      [&] {
        static_cast<void>(poisoned.classify_strict_prune(
            origin(), ExactLevel{BigInt{1}}));
      },
      "the original context remains poisoned after an independent success");
  check(
      fake_gpu_spatial_bounds_launch_count() == 2U,
      "retrying the poisoned context does not affect the independent launch");
}

}  // namespace

int main() {
  test_all_unknown_and_visit_are_nonterminal_hints();
  test_valid_prunes_have_exact_positive_margins();
  test_equality_never_prunes_and_permutation_is_canonicalized();
  test_unsupported_range_falls_back_without_launch_or_poisoning();
  test_post_gpu_corruption_poisons_only_its_context();
  if (failures != 0) {
    std::cerr << failures << " GPU spatial-bounds context test(s) failed\n";
    return 1;
  }
  std::cout << "GPU spatial-bounds context tests passed\n";
  return 0;
}
