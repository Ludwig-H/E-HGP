// Test-only FULL Gamma oracle, deliberately exhaustive and bounded to n <= 8.
// Explicit port of Gram/Cramer OBig640 helpers in tests/silent_incidence_gate.cpp:
// source SHA256 f78a984e577ad76f539acfc43ffeafe195e6dfd0b4eafbb8576462b765678198.
// Arithmetic dependency oracle/obig.hpp at port time:
// SHA256 0e3e9241503e7979fb7da06e97a0a0006362cf4da381c7ce2c66d86acdabf8d5.
// New semantics: singleton MEBs and ALL active facets precede active cofaces.
// No product MEB, geometric predicate, DSU or event reducer is a judge here.
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

#include "obig.hpp"
#include "../src/core/types.hpp"
#include "../src/lanes/level.hpp"  // ExactLevel representation only, not its comparator.

namespace mhgp7_full_oracle {

using Big = mhgp7_oracle::OBig<20>;
using Vec = std::array<Big, 3>;

namespace detail {
using Matrix = std::array<std::array<Big, 3>, 3>;

// Never clear the shared overflow flag: an overflow elsewhere in the oracle
// campaign must not be converted into a success by constructing another Oracle.
inline void check_arithmetic() {
  if (mhgp7_oracle::overflow_seen())
    throw std::overflow_error("FULL Gamma OBig overflow (sticky)");
}
inline Big num(mhgp7::i64 x) { return Big::from_i64(x); }
inline Vec point(const mhgp7::P3& p) { return {num(p.x), num(p.y), num(p.z)}; }
inline Vec sub(const Vec& a, const Vec& b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}
inline Big dot(const Vec& a, const Vec& b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
inline Big det(const Matrix& m, int n) {
  if (n == 1) return m[0][0];
  if (n == 2) return m[0][0] * m[1][1] - m[0][1] * m[1][0];
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
         m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}
}  // namespace detail

struct OracleBall {
  unsigned support = 0;  // Bits are PHYSICAL positions, never external PointIds.
  Vec centre{};  // Euclidean centre = centre / den.
  Big den{}, radius{}, level_den{};  // Squared radius beta = radius / den^2.

  Big power(const mhgp7::P3& p) const {
    detail::check_arithmetic();
    if (den.sign() <= 0) throw std::invalid_argument("FULL Gamma ball denominator");
    Vec v = detail::point(p);
    for (std::size_t d = 0; d < 3; ++d) v[d] = v[d] * den - centre[d];
    const Big result = detail::dot(v, v) - radius;
    detail::check_arithmetic();
    return result;
  }
};

namespace detail {
inline void check_level(const OracleBall& b) {
  check_arithmetic();
  if (b.level_den.sign() <= 0 || b.radius.sign() < 0)
    throw std::invalid_argument("FULL Gamma invalid oracle level");
}

// A positive barycentric support certifies the MEB as soon as its ball contains
// the requested subset. Supports need not span R^3: Gram rank is local to q-1.
inline bool support_ball(const std::vector<mhgp7::P3>& pts, unsigned mask,
                         OracleBall* ball) {
  const int n = std::popcount(mask) - 1;
  if (n < 0 || n > 3) return false;
  std::vector<Vec> vertices;
  for (std::size_t i = 0; i < pts.size(); ++i)
    if (mask & (1u << i)) vertices.push_back(point(pts[i]));
  if (n == 0) {
    ball->support = mask;
    ball->centre = vertices[0];
    ball->den = num(1);
    ball->radius = num(0);
    ball->level_den = num(1);
    return true;
  }
  std::array<Vec, 3> v{};
  Matrix gram{};
  for (int i = 0; i < n; ++i)
    v[(std::size_t)i] = sub(vertices[(std::size_t)i + 1], vertices[0]);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      gram[(std::size_t)i][(std::size_t)j] = dot(v[(std::size_t)i], v[(std::size_t)j]);
  const Big determinant = det(gram, n);
  check_arithmetic();
  if (determinant.sign() <= 0) return false;
  const Big den = num(2) * determinant;
  Big total{};
  std::array<Big, 3> alpha{};
  for (int c = 0; c < n; ++c) {
    Matrix replaced = gram;
    for (int r = 0; r < n; ++r)
      replaced[(std::size_t)r][(std::size_t)c] = gram[(std::size_t)r][(std::size_t)r];
    alpha[(std::size_t)c] = det(replaced, n);
    check_arithmetic();
    if (alpha[(std::size_t)c].sign() <= 0) return false;
    total += alpha[(std::size_t)c];
  }
  check_arithmetic();
  if (total >= den) return false;
  Vec relative{};
  for (std::size_t d = 0; d < 3; ++d)
    for (int i = 0; i < n; ++i)
      relative[d] += v[(std::size_t)i][d] * alpha[(std::size_t)i];
  ball->support = mask;
  ball->den = den;
  ball->level_den = den * den;
  ball->radius = dot(relative, relative);
  for (std::size_t d = 0; d < 3; ++d)
    ball->centre[d] = vertices[0][d] * den + relative[d];
  check_arithmetic();
  return true;
}
}  // namespace detail

inline int compare(const OracleBall& a, const OracleBall& b) {
  detail::check_level(a);
  detail::check_level(b);
  const int result = cmp(a.radius * b.level_den, b.radius * a.level_den);
  detail::check_arithmetic();
  return result;
}
inline int compare(const mhgp7::ExactLevel& a, const OracleBall& b) {
  detail::check_level(b);
  if (a.den <= 0) throw std::invalid_argument("FULL Gamma ExactLevel denominator");
  const int result = cmp(Big::from_u64_words(a.num, 3) * b.level_den,
                         b.radius * Big::from_i128(a.den));
  detail::check_arithmetic();
  return result;
}
inline int compare(const OracleBall& a, const mhgp7::ExactLevel& b) {
  return -compare(b, a);
}

// Usage: check oracle.valid before any access, and oracle.regular before treating
// this configuration as a regular product fixture. regular is GLOBAL: every MEB
// of every nonempty subset has exactly its positive support on the GLOBAL shell.
// Degenerate indexed configurations (including duplicates) remain valid Gamma
// inputs, but may have regular=false. No PointId is accepted or interpreted.
class Oracle {
 public:
  bool valid = false;
  bool regular = false;
  const char* reason = "not_constructed";

  explicit Oracle(const std::vector<mhgp7::P3>& points) {
    // These guards precede the shift, input copy and exhaustive allocations.
    if (points.empty() || points.size() > 8) { reason = "n_outside_1_8"; return; }
    for (const auto& p : points)
      if (p.x < 0 || p.x > 65535 || p.y < 0 || p.y > 65535 ||
          p.z < 0 || p.z > 65535) { reason = "outside_u16"; return; }
    if (mhgp7_oracle::overflow_seen()) { reason = "obig_overflow"; return; }
    pts_ = points;
    const unsigned end = 1u << pts_.size();
    try {
      for (unsigned mask = 1; mask < end; ++mask) {
        if (std::popcount(mask) > 4) continue;
        OracleBall b;
        if (detail::support_ball(pts_, mask, &b)) supports_.push_back(b);
      }
      meb_.assign(end, supports_.size());
      bool all_regular = true;
      for (unsigned mask = 1; mask < end; ++mask) {
        for (std::size_t i = 0; i < supports_.size(); ++i) {
          const OracleBall& b = supports_[i];
          if ((b.support & mask) != b.support) continue;
          bool contains = true;
          for (std::size_t p = 0; p < pts_.size(); ++p)
            if ((mask & (1u << p)) && b.power(pts_[p]).sign() > 0) contains = false;
          if (!contains) continue;
          meb_[mask] = i;
          unsigned shell = 0;
          for (std::size_t p = 0; p < pts_.size(); ++p)
            if (b.power(pts_[p]).sign() == 0) shell |= 1u << p;
          if (shell != b.support) all_regular = false;
          break;
        }
        if (meb_[mask] == supports_.size()) { reason = "missing_meb"; return; }
      }
      detail::check_arithmetic();
      regular = all_regular;
      valid = true;
      reason = "complete";
    } catch (const std::overflow_error&) {
      reason = "obig_overflow";
    }
  }

  std::size_t size() const { return pts_.size(); }
  const OracleBall& ball(unsigned mask) const {
    require_valid();
    if (mask == 0 || mask >= meb_.size())
      throw std::out_of_range("FULL Gamma nonempty physical mask");
    return supports_[meb_[mask]];
  }

  // Gabriel closure: no point outside the indexed subset is STRICTLY inside
  // its MEB. On a regular configuration all such foreign powers are nonzero.
  bool direct(unsigned mask) const {
    const OracleBall& b = ball(mask);
    for (std::size_t p = 0; p < pts_.size(); ++p)
      if (!(mask & (1u << p)) && b.power(pts_[p]).sign() < 0) return false;
    return true;
  }

  // Components are sorted vectors of sorted nonempty PHYSICAL facet masks.
  // closed=false means beta < cut; closed=true means beta <= cut. The cut is a
  // squared radius. K=n is meaningful: the unique n-facet has no coface.
  std::vector<std::vector<unsigned>> full_components(
      unsigned k, const mhgp7::ExactLevel& cut, bool closed) const {
    if (cut.den <= 0) throw std::invalid_argument("FULL Gamma ExactLevel denominator");
    return components(k, [&](const OracleBall& b) {
      const int c = compare(b, cut);
      return c < 0 || (closed && c == 0);
    });
  }
  std::vector<std::vector<unsigned>> full_components(
      unsigned k, const OracleBall& cut, bool closed) const {
    detail::check_level(cut);
    return components(k, [&](const OracleBall& b) {
      const int c = compare(b, cut);
      return c < 0 || (closed && c == 0);
    });
  }

 private:
  std::vector<mhgp7::P3> pts_;
  std::vector<OracleBall> supports_;
  std::vector<std::size_t> meb_;

  void require_valid() const {
    detail::check_arithmetic();
    if (!valid) throw std::logic_error("FULL Gamma invalid Oracle");
  }

  template <class Active>
  std::vector<std::vector<unsigned>> components(unsigned k, Active active) const {
    require_valid();
    if (k == 0 || k > pts_.size()) throw std::invalid_argument("FULL Gamma order");
    const unsigned end = (unsigned)meb_.size();
    std::vector<unsigned> parent(end);
    std::vector<bool> present(end, false);
    for (unsigned mask = 0; mask < end; ++mask) parent[mask] = mask;
    auto find = [&](unsigned mask) {
      while (parent[mask] != mask) {
        parent[mask] = parent[parent[mask]];
        mask = parent[mask];
      }
      return mask;
    };
    // FULL, not the historical reduced oracle: isolated facets are real leaves.
    for (unsigned mask = 1; mask < end; ++mask)
      if ((unsigned)std::popcount(mask) == k && active(ball(mask))) present[mask] = true;
    // Resolve every coface only after all facets at the same cut are available.
    for (unsigned mask = 1; mask < end; ++mask) {
      if ((unsigned)std::popcount(mask) != k + 1 || !active(ball(mask))) continue;
      unsigned first = 0;
      for (unsigned bits = mask; bits != 0; bits &= bits - 1) {
        const unsigned facet = mask ^ (bits & (0u - bits));
        if (!present[facet]) throw std::logic_error("FULL Gamma nonmonotone MEB");
        if (first == 0) first = facet;
        else parent[find(facet)] = find(first);
      }
    }
    std::vector<std::vector<unsigned>> groups(end);
    for (unsigned mask = 1; mask < end; ++mask)
      if (present[mask]) groups[find(mask)].push_back(mask);
    std::vector<std::vector<unsigned>> result;
    for (auto& group : groups)
      if (!group.empty()) result.push_back(std::move(group));
    std::sort(result.begin(), result.end());
    detail::check_arithmetic();
    return result;
  }
};

}  // namespace mhgp7_full_oracle
