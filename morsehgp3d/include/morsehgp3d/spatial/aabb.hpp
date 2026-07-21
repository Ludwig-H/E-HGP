#pragma once

#include <array>
#include <cstdint>

namespace morsehgp3d::spatial {

// Closed axis-aligned bounds represented by their exact binary64 input words.
// Enclosure, padding and provenance requirements belong to the calling
// contract; this value type performs no implicit rounding or expansion.
struct ExactDyadicAabb3 {
  std::array<std::uint64_t, 3> lower_binary64_bits;
  std::array<std::uint64_t, 3> upper_binary64_bits;

  friend bool operator==(
      const ExactDyadicAabb3&,
      const ExactDyadicAabb3&) = default;
};

}  // namespace morsehgp3d::spatial
