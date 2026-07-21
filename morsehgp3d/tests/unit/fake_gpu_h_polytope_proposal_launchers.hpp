#pragma once

#include <cstddef>
#include <cstdint>

namespace morsehgp3d::gpu::test_support {

enum class FakeHPolytopeProposalValues : std::uint8_t {
  actual_binary64_recipe,
  all_unknown,
  whole_cell_interval_fallback,
};

enum class FakeHPolytopeProposalCorruption : std::uint8_t {
  none,
  missing_slot,
  duplicate_slot,
  omitted_true_incidence,
  out_of_range_incidence_bit,
  nonfinite_survivor_coordinate,
  false_strict_reject,
  wrong_epoch,
  wrong_batch_epoch,
  stale_epoch_without_advance,
  double_epoch_advance,
  wrong_ordinal,
  invalid_offsets,
  invalid_cell_id,
  invalid_cell_status,
  invalid_record_status,
  tail_write,
  simulated_async_failure,
};

struct FakeHPolytopeProposalConfiguration {
  FakeHPolytopeProposalValues values{
      FakeHPolytopeProposalValues::actual_binary64_recipe};
  FakeHPolytopeProposalCorruption corruption{
      FakeHPolytopeProposalCorruption::none};
};

void configure_fake_gpu_h_polytope_proposal(
    FakeHPolytopeProposalConfiguration configuration) noexcept;
void reset_fake_gpu_h_polytope_proposal() noexcept;

[[nodiscard]] std::size_t
fake_gpu_h_polytope_proposal_launch_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_h_polytope_proposal_last_cell_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_h_polytope_proposal_last_boundary_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_h_polytope_proposal_last_record_capacity() noexcept;
[[nodiscard]] std::size_t
fake_gpu_h_polytope_proposal_last_incidence_word_capacity() noexcept;

}  // namespace morsehgp3d::gpu::test_support
