#pragma once

#include <cstdint>

namespace morsehgp3d::gpu::test_support {

inline constexpr std::uint64_t exact_phi_force_interval_replay_id =
    UINT64_C(0xfffffffffffffff0);
inline constexpr std::uint64_t exact_phi_force_fallback_replay_id =
    UINT64_C(0xfffffffffffffff1);
inline constexpr std::uint64_t exact_phi_wrong_sign_replay_id =
    UINT64_C(0xfffffffffffffff2);
inline constexpr std::uint64_t exact_phi_invalid_stage_replay_id =
    UINT64_C(0xfffffffffffffff3);
inline constexpr std::uint64_t exact_phi_reserved_byte_replay_id =
    UINT64_C(0xfffffffffffffff4);
inline constexpr std::uint64_t exact_phi_bad_metadata_replay_id =
    UINT64_C(0xfffffffffffffff5);
inline constexpr std::uint64_t exact_phi_poison_replay_id =
    UINT64_C(0xfffffffffffffff6);
inline constexpr std::uint64_t exact_phi_concurrent_wrong_sign_replay_id =
    UINT64_C(0xfffffffffffffff7);

[[nodiscard]] bool exact_phi_concurrent_fixture_entered() noexcept;

}  // namespace morsehgp3d::gpu::test_support
