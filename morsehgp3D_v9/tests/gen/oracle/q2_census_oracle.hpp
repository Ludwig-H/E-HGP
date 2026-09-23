#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "core/types.hpp"

// A bounded test oracle, independent of the production census, index and
// spindle predicates. It enumerates every site using the exact squared
// distance to the doubled centre. It does not certify a FULL HGP producer.
namespace mhgp9::gen::q2_oracle {

using Integer = boost::multiprecision::cpp_int;

struct BallKey {
  std::array<Integer, 3> center_twice;
  Integer diameter_squared;
};

struct Census {
  BallKey key;
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;
};

[[nodiscard]] inline BallKey ball_key(const Point3& a, const Point3& b) {
  BallKey result{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result.center_twice[axis] = Integer(a[axis]) + Integer(b[axis]);
    const Integer difference = Integer(a[axis]) - Integer(b[axis]);
    result.diameter_squared += difference * difference;
  }
  return result;
}

[[nodiscard]] inline Integer power_four(const BallKey& key, const Point3& site) {
  Integer power = -key.diameter_squared;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const Integer difference = 2 * Integer(site[axis]) - key.center_twice[axis];
    power += difference * difference;
  }
  return power;
}

[[nodiscard]] inline Census census(std::span<const Point3> points,
                                    std::size_t a, std::size_t b) {
  if (a >= points.size() || b >= points.size() || a == b) {
    throw std::invalid_argument("q2 oracle requires two distinct valid endpoint IDs");
  }
  Census result{ball_key(points[a], points[b]), {}, {}};
  for (std::size_t index = 0; index < points.size(); ++index) {
    const Integer power = power_four(result.key, points[index]);
    if (power < 0) result.interior.push_back(index);
    if (power == 0) result.shell.push_back(index);
  }
  return result;
}

}  // namespace mhgp9::gen::q2_oracle
