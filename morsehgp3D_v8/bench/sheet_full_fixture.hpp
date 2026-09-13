#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "pipeline/local_credits.hpp"

namespace mhgp8::bench {

// Recipe v2 extracted from axis_probe: complete corresponding yz grids.
// No last-row truncation, new seed, alternate point IDs or changed coordinates.
[[nodiscard]] inline RectangleInput make_sheet_full_fixture(std::size_t n) {
  if (n < 2) {
    throw std::invalid_argument("sheet_full version 2 requires n>=2");
  }
  if (n % 2 != 0) {
    throw std::invalid_argument("sheet_full version 2 requires an even n");
  }
  const std::size_t count = n / 2;
  constexpr std::size_t coordinate_capacity = 65536 - 1000;
  if (count > coordinate_capacity * coordinate_capacity) {
    throw std::invalid_argument("sheet_full cannot fit the required u16 coordinates");
  }
  // Exact integer square root, then largest divisor at or below it.
  std::size_t low = 1;
  std::size_t high = std::min(count, coordinate_capacity);
  while (low < high) {
    const std::size_t middle = low + (high - low + 1) / 2;
    if (middle <= count / middle) {
      low = middle;
    } else {
      high = middle - 1;
    }
  }
  std::size_t width = low;
  while (count % width != 0) {
    --width;
  }
  const std::size_t height = count / width;
  if (height > coordinate_capacity) {
    throw std::invalid_argument("sheet_full full rectangle exceeds its u16 coordinate domain");
  }
  RectangleInput input;
  input.points.reserve(n);
  for (const std::uint16_t x : {std::uint16_t{1000}, std::uint16_t{60000}}) {
    for (std::size_t index = 0; index < count; ++index) {
      input.points.push_back({x, static_cast<std::uint16_t>(1000 + index % width),
                              static_cast<std::uint16_t>(1000 + index / width)});
    }
  }
  input.a = {0, count};
  input.b = {count, n};
  return input;
}

}  // namespace mhgp8::bench
