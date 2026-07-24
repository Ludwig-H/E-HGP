#pragma once

#include <cstddef>
#include <cstdint>

namespace morsehgp3d::gpu::test_support {

enum class FakePhase14FacetTopKProposalCorruption : std::uint8_t {
  none,
  duplicate_query_index,
  wrong_key_fingerprint,
  zero_epoch,
  stale_epoch_without_advance,
  jumped_epoch,
  tail_write,
  duplicate_candidate,
  out_of_domain_candidate,
  out_of_window_candidate,
  counter_overrun,
  counter_partition_mismatch,
  simulated_async_failure,
};

struct FakePhase14FacetTopKProposalConfiguration {
  FakePhase14FacetTopKProposalCorruption corruption{
      FakePhase14FacetTopKProposalCorruption::none};
};

void configure_fake_gpu_phase14_facet_top_k_proposal(
    FakePhase14FacetTopKProposalConfiguration configuration) noexcept;
void reset_fake_gpu_phase14_facet_top_k_proposal() noexcept;

[[nodiscard]] std::size_t
fake_gpu_phase14_facet_top_k_proposal_launch_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_phase14_facet_top_k_proposal_epoch_advance_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_phase14_facet_top_k_proposal_last_query_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_phase14_facet_top_k_proposal_last_record_capacity() noexcept;
[[nodiscard]] std::size_t
fake_gpu_phase14_facet_top_k_proposal_last_window_radius() noexcept;

}  // namespace morsehgp3d::gpu::test_support
