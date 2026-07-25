#pragma once

#include "morsehgp3d/exact/point.hpp"

#include <cstddef>
#include <vector>

namespace morsehgp3d::tools::pair_support_smoke {

[[nodiscard]] inline std::vector<exact::CertifiedPoint3>
uniform_latin_points(std::size_t point_count) {
  constexpr std::size_t modulus = 65537U;
  std::vector<exact::CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::size_t value = index + 1U;
    const std::size_t permuted_y =
        (value * 25173U + 13849U) % modulus;
    const std::size_t permuted_z =
        (value * 13849U + 25173U) % modulus;
    points.push_back(exact::CertifiedPoint3::from_binary64(
        static_cast<double>(value) / static_cast<double>(modulus),
        static_cast<double>(permuted_y) /
            static_cast<double>(modulus),
        static_cast<double>(permuted_z) /
            static_cast<double>(modulus)));
  }
  return points;
}

[[nodiscard]] inline std::vector<exact::CertifiedPoint3>
eight_clusters_points(std::size_t point_count) {
  constexpr double local_scale = 1.0 / 1048576.0;
  constexpr double transverse_scale = 1.0 / 4194304.0;
  std::vector<exact::CertifiedPoint3> points;
  points.reserve(point_count);
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::size_t cluster = index % 8U;
    const std::size_t local = index / 8U + 1U;
    const double center_x = (cluster & 1U) == 0U ? 0.25 : 0.75;
    const double center_y = (cluster & 2U) == 0U ? 0.25 : 0.75;
    const double center_z = (cluster & 4U) == 0U ? 0.25 : 0.75;
    const std::size_t permuted_y =
        (local * 40503U + cluster * 7919U) % 65536U;
    const std::size_t permuted_z =
        (local * 25717U + cluster * 104729U) % 65536U;
    points.push_back(exact::CertifiedPoint3::from_binary64(
        center_x + static_cast<double>(local) * local_scale,
        center_y + static_cast<double>(permuted_y) * transverse_scale,
        center_z + static_cast<double>(permuted_z) * transverse_scale));
  }
  return points;
}

}  // namespace morsehgp3d::tools::pair_support_smoke
