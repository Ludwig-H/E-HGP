#pragma once

#include <cstddef>
#include <cstdint>

namespace morsehgp3d::gpu::test_support {

enum class FakePairSupportPhiCorruption : std::uint8_t {
  none,
  duplicate_transcript_query,
  changed_transcript_query,
  stale_tail,
  false_strict_interior,
  zero_epoch,
  stale_epoch_without_advance,
  simulated_async_failure,
  rank_capacity_metadata,
  rank_zero_epoch,
  rank_stale_epoch_without_advance,
  rank_changed_receipt,
  rank_false_strict_receipt,
  rank_stale_tail,
  rank_epoch_count_over_budget,
  rank_simulated_async_failure,
};

void configure_fake_gpu_pair_support_phi(
    FakePairSupportPhiCorruption corruption) noexcept;
void reset_fake_gpu_pair_support_phi() noexcept;

[[nodiscard]] std::size_t
fake_gpu_pair_support_phi_launch_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_pair_support_phi_last_node_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_pair_support_phi_last_query_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_pair_support_phi_last_record_capacity() noexcept;
[[nodiscard]] std::size_t
fake_gpu_pair_support_rank_launch_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_pair_support_rank_last_product_count() noexcept;
[[nodiscard]] std::size_t
fake_gpu_pair_support_rank_last_work_item_capacity() noexcept;
[[nodiscard]] std::size_t
fake_gpu_pair_support_rank_last_receipt_capacity() noexcept;

}  // namespace morsehgp3d::gpu::test_support
