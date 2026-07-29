#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace morsehgp3d::gpu::test_support {

struct FakeMortonYao48RadialRecord {
  bool propose_prune{};
  bool directed_bounds_valid{};
};

void configure_fake_morton_yao48_radial_records(
    std::vector<FakeMortonYao48RadialRecord> records);
void reset_fake_morton_yao48_radial_records() noexcept;
[[nodiscard]] std::size_t fake_morton_yao48_radial_call_count() noexcept;

}  // namespace morsehgp3d::gpu::test_support
