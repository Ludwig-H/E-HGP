#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "lanes/edge_cover.hpp"

namespace mhgp9::gen {

// v9 (23 septembre 2026). Work of the dead-lane certificate, summed over
// edges. uniform_tests and point_tests are form evaluations (one affine
// form of one cover site on one cell corner set / one center).
struct Q34DeadLaneWork {
  u64 loads{}, form_sites{}, cells{}, outside_cells{}, deep_cells{}, failed_cells{};
  u64 uniform_tests{}, point_tests{}, q3_proved{}, q3_open{}, q4_proved{}, q4_open{};
  bool operator==(const Q34DeadLaneWork&) const = default;
};

// Exact certificate that one lane of an owner edge ab emits NOTHING.
//
// Centers of the balls through a and b lie in the bisector plane, with the
// parametrization of Q4LocalGeometry: 2(c-m) = u1*A + u2*B (m the midpoint,
// v = b-a, A and B the integer basis orthogonal to v), cells are dyadic
// squares of (u1, u2) scaled by 2^20 inside the root [-2,2]^2. For a site z,
// w = 2z-a-b and L_z(u) = |w|^2-|v|^2-2w.(u1*A+u2*B) is four times the power
// of z to the ball of center c through a and b: z is strictly inside iff
// L_z<0. L_z is affine in u, so its maximum over a closed cell is reached at
// a corner and is exact in i64 (|2^20*k|<2^60, |x|,|y|<2^39 and corners
// |u|<=2^21: every sum stays below 2^62, as in Q4LocalGeometry::bounds).
//
// Lane disks. A strictly acute triangle abx owned by its longest edge ab
// has barycentric circumcenter weights l_i>=0, so R^2 = sum_{i<j} l_i l_j
// |p_i-p_j|^2 <= L^2 (1-sum l_i^2)/2 <= L^2/3 and |c-m|^2 = R^2-L^2/4 <=
// L^2/12, i.e. 3|u1*A+u2*B|^2 <= |v|^2. A positive tetrahedron abxy owned by
// ab gives R^2 <= 3L^2/8 and |c-m|^2 <= L^2/8, i.e. 2|u1*A+u2*B|^2 <= |v|^2
// (the Q4LocalGeometry disk). The minimal eigenvalue of the Gram of (A, B)
// is at least h^2 >= |v|^2/3, so both disks lie in |u| <= sqrt(3/2) < 2.
//
// Certificate. The lane is dead when an adaptive dyadic cover of the root
// has every closed cell either disjoint from the lane disk (exact lower
// bound of |u1*A+u2*B|^2 on the cell) or holding at least T DISTINCT cover
// sites with max_cell L_z<0: every ball of the lane with its center in the
// cell then has at least T strict interiors. T = K-1 for q3 (a q3 seed is
// rejected at depth K-1) and T = K-2 for q4 (a q4 center is kept only at
// depth < K-2). a and b have L=0 and are never counted. The cover (closed
// ball of radius |ab| around m) contains every site inside any lane ball
// (R+|c-m| <= (sqrt(3)+1)L/(2 sqrt(2)) < L), so the pointwise depth used to
// stop early is exact too; stopping is only a cost decision.
//
// Work: a cell below min_depth only splits. From min_depth on, a cell scans
// its parent's FRONTIER (sites neither uniformly inside nor uniformly
// outside the parent cell): uniformly inside sites of a closed cell stay so
// on its closed sub-cells (inherited count), uniformly outside ones never
// enter; the scan stops as soon as the count reaches T. An unproved cell
// then counts the exact depth at one of its lane centers (inherited +
// frontier sites negative there): below T no certificate can exist and the
// proof stops at once (cost decision only).
//
// A failed proof states nothing: the caller runs the exact lane unchanged.
// A successful proof removes exactly an empty lane: same emitted stream.
// Not thread-safe: one prover per worker, reused across edges (it keeps
// the form buffer's capacity).
class Q34DeadLaneProver final {
 public:
  static constexpr unsigned default_max_depth = 6;
  static constexpr unsigned default_min_depth = 2;
  explicit Q34DeadLaneProver(unsigned max_depth = default_max_depth,
                             unsigned min_depth = default_min_depth);

  // Forms of every cover site in cover rank order; a and b get identically
  // zero forms (never credited). form_sites counts the other sites.
  void load(const Q34EdgeCover& cover, Q34DeadLaneWork& work);
  // Requires load(). K is the tower's Kmax (1..10), `lanes` a subset of
  // 2 (q3) | 4 (q4). Returns the subset proved dead, both lanes in ONE pass
  // over the cells (the q3 disk lies inside the q4 disk). A lane whose
  // threshold is zero (K<2 for q3, K<3 for q4) is never proved here.
  [[nodiscard]] std::uint8_t prove(unsigned kmax, std::uint8_t lanes, Q34DeadLaneWork& work);
  // Capacity of the reused private buffers (forms, IDs, frontiers).
  [[nodiscard]] std::size_t retained_bytes() const;

 private:
  struct Cell { i64 left, right, bottom, top; };
  struct Form { i64 constant, x, y; };  // 2^20*k, x, y of L_z
  std::uint8_t cell(const Cell& c, unsigned depth, std::span<const std::uint32_t> frontier, std::size_t inherited,
                    std::uint8_t lanes, Q34DeadLaneWork& work);
  [[nodiscard]] bool outside(const Cell& c, i64 disk_factor) const;
  [[nodiscard]] bool center_inside(i64 alpha, i64 beta, i64 disk_factor) const;

  unsigned max_depth_, min_depth_;
  std::array<i64, 3> a_basis_{}, b_basis_{};
  i64 diameter_squared_{};
  std::size_t threshold3_{}, threshold4_{};
  bool loaded_{false};
  std::vector<Form> forms_;
  std::vector<std::uint32_t> all_;                   // identity prefix 0..n-1
  std::vector<std::vector<std::uint32_t>> levels_;  // frontier by depth
};

}  // namespace mhgp9::gen
