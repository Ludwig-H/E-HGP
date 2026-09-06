// Independent bounded rational oracle; never a product geometry dependency.
#pragma once

#include "src/core/types.hpp"

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace local_plateau_oracle {

using Int = boost::multiprecision::cpp_int;
using Rat = boost::rational<Int>;
using u32 = mhgp7::u32;
using P3 = mhgp7::P3;
using Vec = std::array<Rat, 3>;
struct Ball { Vec center; Rat radius2; };

namespace detail {

inline u32 domain(const std::vector<P3>& points) {
  if (points.empty() || points.size() > 8)
    throw std::invalid_argument("oracle requires 1..8 points");
  for (const auto& p : points)
    if (p.x < 0 || p.x > 65535 || p.y < 0 || p.y > 65535 ||
        p.z < 0 || p.z > 65535)
      throw std::invalid_argument("oracle requires u16 coordinates");
  return (u32{1} << points.size()) - 1;
}

inline Vec point(const P3& p) { return {Rat(p.x), Rat(p.y), Rat(p.z)}; }
inline Vec difference(const Vec& a, const Vec& b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
inline Rat dot(const Vec& a, const Vec& b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

using Matrix = std::array<std::array<Rat, 4>, 3>;
inline bool solve(Matrix a, unsigned n, std::array<Rat, 3>& solution) {
  for (unsigned col = 0; col < n; ++col) {
    unsigned pivot = col;
    while (pivot < n && a[pivot][col] == Rat(0)) ++pivot;
    if (pivot == n) return false;
    std::swap(a[col], a[pivot]);
    const Rat divisor = a[col][col];
    for (unsigned j = col; j <= n; ++j) a[col][j] /= divisor;
    for (unsigned row = 0; row < n; ++row) {
      if (row == col) continue;
      const Rat multiplier = a[row][col];
      for (unsigned j = col; j <= n; ++j)
        a[row][j] -= multiplier * a[col][j];
    }
  }
  for (unsigned i = 0; i < n; ++i) solution[i] = a[i][n];
  return true;
}

inline unsigned basis(const std::vector<P3>& points, u32 mask, Vec& base,
                      std::array<Vec, 3>& edges) {
  const unsigned first = std::countr_zero(mask);
  base = point(points[first]);
  mask &= mask - 1;
  unsigned n = 0;
  while (mask != 0) {
    edges[n++] = difference(point(points[std::countr_zero(mask)]), base);
    mask &= mask - 1;
  }
  return n;
}

inline std::optional<Ball> support_ball(const std::vector<P3>& points,
                                        u32 support) {
  Vec base{};
  std::array<Vec, 3> edges{};
  const unsigned n = basis(points, support, base, edges);
  Matrix gram{};
  for (unsigned i = 0; i < n; ++i) {
    for (unsigned j = 0; j < n; ++j) gram[i][j] = dot(edges[i], edges[j]);
    gram[i][n] = dot(edges[i], edges[i]) / Rat(2);
  }
  std::array<Rat, 3> weights{};
  if (!solve(gram, n, weights)) return std::nullopt;
  Rat first_weight(1);
  Vec center = base;
  for (unsigned i = 0; i < n; ++i) {
    if (weights[i] <= Rat(0)) return std::nullopt;
    first_weight -= weights[i];
    for (unsigned j = 0; j < 3; ++j) center[j] += weights[i] * edges[i][j];
  }
  if (first_weight <= Rat(0)) return std::nullopt;
  const Vec delta = difference(center, base);
  return Ball{center, dot(delta, delta)};
}

inline bool enclosed(const Ball& ball, const std::vector<P3>& points, u32 mask) {
  while (mask != 0) {
    const Vec delta = difference(point(points[std::countr_zero(mask)]), ball.center);
    if (dot(delta, delta) > ball.radius2) return false;
    mask &= mask - 1;
  }
  return true;
}

}  // namespace detail

inline bool contains_center(const Vec& center, const std::vector<P3>& points,
                            u32 mask) {
  if ((mask & ~detail::domain(points)) != 0)
    throw std::invalid_argument("oracle mask outside point domain");
  for (u32 support = mask; support != 0; support = (support - 1) & mask) {
    if (std::popcount(support) > 4) continue;
    Vec base{};
    std::array<Vec, 3> edges{};
    const unsigned n = detail::basis(points, support, base, edges);
    const Vec offset = detail::difference(center, base);
    detail::Matrix gram{};
    for (unsigned i = 0; i < n; ++i) {
      for (unsigned j = 0; j < n; ++j) gram[i][j] = detail::dot(edges[i], edges[j]);
      gram[i][n] = detail::dot(edges[i], offset);
    }
    std::array<Rat, 3> weights{};
    if (!detail::solve(gram, n, weights)) continue;
    Rat first_weight(1);
    Vec reconstructed = base;
    bool nonnegative = true;
    for (unsigned i = 0; i < n; ++i) {
      nonnegative = nonnegative && weights[i] >= Rat(0);
      first_weight -= weights[i];
      for (unsigned j = 0; j < 3; ++j)
        reconstructed[j] += weights[i] * edges[i][j];
    }
    if (nonnegative && first_weight >= Rat(0) && reconstructed == center) return true;
  }
  return false;
}

class Model {
 public:
  explicit Model(std::vector<P3> points) : points_(std::move(points)),
      domain_(detail::domain(points_)), balls_(domain_ + 1) {
    for (u32 mask = 1; mask <= domain_; ++mask) {
      std::optional<Ball> best;
      for (u32 support = mask; support != 0; support = (support - 1) & mask) {
        if (std::popcount(support) > 4) continue;
        const auto ball = detail::support_ball(points_, support);
        if (ball && (!best || ball->radius2 < best->radius2) &&
            detail::enclosed(*ball, points_, mask)) best = ball;
      }
      if (!best) throw std::logic_error("oracle found no enclosing support");
      balls_[mask] = *best;
    }
  }

  Ball meb(u32 mask) const {
    if (mask == 0 || (mask & ~domain_) != 0)
      throw std::invalid_argument("oracle MEB requires a nonempty domain subset");
    return balls_[mask];
  }

  std::vector<std::vector<u32>> components(unsigned K, const Rat& radius2,
                                          bool closed, u32 allowed_mask) const {
    if (K == 0 || (allowed_mask & ~domain_) != 0)
      throw std::invalid_argument("invalid oracle component query");
    const auto present = [&](u32 mask) {
      return closed ? balls_[mask].radius2 <= radius2 : balls_[mask].radius2 < radius2;
    };
    std::vector<u32> facets;
    for (u32 mask = allowed_mask; mask != 0; mask = (mask - 1) & allowed_mask)
      if (static_cast<unsigned>(std::popcount(mask)) == K && present(mask))
        facets.push_back(mask);
    std::sort(facets.begin(), facets.end());
    std::vector<bool> seen(facets.size(), false);
    std::vector<std::vector<u32>> result;
    for (std::size_t start = 0; start < facets.size(); ++start) {
      if (seen[start]) continue;
      seen[start] = true;
      std::vector<u32> component{facets[start]};
      for (std::size_t next = 0; next < component.size(); ++next)
        for (std::size_t j = 0; j < facets.size(); ++j) {
          const u32 united = component[next] | facets[j];
          if (!seen[j] && static_cast<unsigned>(std::popcount(united)) == K + 1 &&
              present(united)) {
            seen[j] = true;
            component.push_back(facets[j]);
          }
        }
      std::sort(component.begin(), component.end());
      result.push_back(std::move(component));
    }
    std::sort(result.begin(), result.end());
    return result;
  }

 private:
  std::vector<P3> points_;
  u32 domain_;
  std::vector<Ball> balls_;
};

}  // namespace local_plateau_oracle
