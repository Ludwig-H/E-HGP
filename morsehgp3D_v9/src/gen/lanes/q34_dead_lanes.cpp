#include "lanes/q34_dead_lanes.hpp"

#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace mhgp9::gen {
namespace {
constexpr unsigned scale_bits = 20;  // Q4LocalCell::scale_bits
constexpr i64 scale = i64{1} << scale_bits;
using Vec = std::array<i64, 3>;
// Disk factors f with f*|u1*A+u2*B|^2 <= |v|^2: q3 (L^2/12) and q4 (L^2/8).
constexpr i64 q3_disk_factor = 3;
constexpr i64 q4_disk_factor = 2;
i64 dot(const Vec& a, const Vec& b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
}  // namespace

Q34DeadLaneProver::Q34DeadLaneProver(unsigned max_depth, unsigned min_depth)
    : max_depth_(max_depth), min_depth_(min_depth) {
  // Depth d halves the 2^22-wide root d times: integer corners up to 2^20.
  if (max_depth > 20 || min_depth > max_depth)
    throw std::invalid_argument("mhgp9 gen dead-lane prover requires min<=max<=20 depths");
  levels_.resize(max_depth + 1);
}

void Q34DeadLaneProver::load(const Q34EdgeCover& cover, Q34DeadLaneWork& work) {
  const auto& index = *cover.index();
  const auto order = index.spatial_order();
  if (ordered_owner_ != &index) {
    // Coordinates in spatial rank order, once per index and prover: the
    // cover ranges are then read sequentially (no ID indirection).
    const auto points = index.cloud().points();
    ordered_.resize(order.size());
    for (std::size_t rank = 0; rank < order.size(); ++rank) ordered_[rank] = points[order[rank]];
    ordered_owner_ = &index;
  }
  const auto ids = cover.edge_ids();
  const auto points = index.cloud().points();
  const auto a = points[ids[0]], b = points[ids[1]];
  static_cast<void>(order);
  Vec v{}, midpoint_twice{};
  std::size_t main_axis = 0;
  for (std::size_t i = 0; i < 3; ++i) {
    v[i] = static_cast<i64>(b[i]) - a[i];
    midpoint_twice[i] = static_cast<i64>(a[i]) + b[i];
    if (std::llabs(v[i]) > std::llabs(v[main_axis])) main_axis = i;
  }
  // Same integer basis as Q4LocalGeometry (proofs there): |basis| <= M.
  const std::size_t axis_i = (main_axis + 1) % 3, axis_j = (main_axis + 2) % 3;
  const i64 h = std::llabs(v[main_axis]), sign = v[main_axis] > 0 ? 1 : -1;
  a_basis_ = {0, 0, 0}; b_basis_ = {0, 0, 0};
  a_basis_[axis_i] = h; a_basis_[main_axis] = -sign * v[axis_i];
  b_basis_[axis_j] = h; b_basis_[main_axis] = -sign * v[axis_j];
  diameter_squared_ = dot(v, v);
  // Invalid until this load completes: a failed load leaves no usable state.
  loaded_ = false;
  counter_add(work.loads);
  forms_.resize(cover.site_count());
  std::size_t n = 0;
  // Each basis vector has two nonzero coordinates: two products each.
  const i64 am = a_basis_[main_axis], ai = a_basis_[axis_i], bm = b_basis_[main_axis], bj = b_basis_[axis_j];
  // a and b are loaded too: w = -+v gives k = 0 and x = y = 0 (A, B are
  // orthogonal to v), an identically zero form that is never negative, so
  // they are never credited nor kept in a frontier. No branch in the loop.
  for (const auto range : cover.ranges())
    for (auto rank = range.first; rank < range.last; ++rank) {
      const auto& z = ordered_[rank];
      Vec w{};
      for (std::size_t i = 0; i < 3; ++i) w[i] = 2 * static_cast<i64>(z[i]) - midpoint_twice[i];
      // M=262143: |w_i| <= 2M, |dot(w,w)-|v|^2| <= 15M^2 < 2^40 (scaled < 2^60),
      // |x|,|y| <= 8M^2 < 2^39 (Q4LocalGeometry::form).
      forms_[n++] = {scale * (dot(w, w) - diameter_squared_), -2 * (w[main_axis] * am + w[axis_i] * ai),
                     -2 * (w[main_axis] * bm + w[axis_j] * bj)};
    }
  forms_.resize(n);
  if (forms_.size() > std::numeric_limits<std::uint32_t>::max())
    throw std::overflow_error("mhgp9 gen dead-lane prover cover exceeds u32 frontier IDs");
  // Identity frontier 0..n-1, extended only when a larger cover appears.
  const auto old = all_.size();
  if (old < n) {
    all_.resize(n);
    for (std::size_t i = old; i < n; ++i) all_[i] = static_cast<std::uint32_t>(i);
  }
  if (n < 2) throw std::logic_error("mhgp9 gen dead-lane prover cover lost its endpoints");
  counter_add(work.form_sites, static_cast<u64>(n - 2));  // sites other than a and b
  loaded_ = true;
}

std::size_t Q34DeadLaneProver::retained_bytes() const {
  std::size_t bytes = forms_.capacity() * sizeof(Form) + all_.capacity() * sizeof(std::uint32_t) +
                      ordered_.capacity() * sizeof(Point3);
  for (const auto& level : levels_) bytes += level.capacity() * sizeof(std::uint32_t);
  return bytes;
}

std::uint8_t Q34DeadLaneProver::prove(unsigned kmax, std::uint8_t lanes, Q34DeadLaneWork& work) {
  if (!loaded_) throw std::logic_error("mhgp9 gen dead-lane prover used before load");
  if ((lanes & ~6U) != 0) throw std::invalid_argument("mhgp9 gen dead-lane prover lanes must be a subset of 2|4");
  // T3 = K-1 (q3 seeds rejected at depth K-1), T4 = K-2 (q4 kept below K-2).
  // A zero threshold proves nothing: such a lane is left to the exact path.
  std::uint8_t tried = lanes;
  if (kmax < 2) tried = static_cast<std::uint8_t>(tried & ~2U);
  if (kmax < 3) tried = static_cast<std::uint8_t>(tried & ~4U);
  threshold3_ = kmax >= 2 ? kmax - 1 : 0;
  threshold4_ = kmax >= 3 ? kmax - 2 : 0;
  std::uint8_t proved = 0;
  if (tried != 0)
    proved = cell({-2 * scale, 2 * scale, -2 * scale, 2 * scale}, 0,
                  std::span<const std::uint32_t>(all_.data(), forms_.size()), 0, tried, work);
  if ((lanes & 2U) != 0) counter_add((proved & 2U) != 0 ? work.q3_proved : work.q3_open);
  if ((lanes & 4U) != 0) counter_add((proved & 4U) != 0 ? work.q4_proved : work.q4_open);
  return proved;
}

bool Q34DeadLaneProver::outside(const Cell& c, i64 disk_factor) const {
  // Exact lower bound of |alpha*A+beta*B|^2 over the closed cell, axis by
  // axis (nearest value of each affine coordinate to zero). |basis| <= 2^18
  // and |corner| <= 2^21: each coordinate < 2^40, the norm < 3*2^80, times
  // 3 < 2^84; |v|^2*2^40 < 3*2^76. All in i128.
  i128 norm = 0;
  for (std::size_t i = 0; i < 3; ++i) {
    const i64 x = a_basis_[i], y = b_basis_[i];
    const i64 low = x * (x < 0 ? c.right : c.left) + y * (y < 0 ? c.top : c.bottom);
    const i64 high = x * (x < 0 ? c.left : c.right) + y * (y < 0 ? c.bottom : c.top);
    const i128 nearest = low > 0 ? low : (high < 0 ? high : 0);
    norm += nearest * nearest;
  }
  return static_cast<i128>(disk_factor) * norm > static_cast<i128>(diameter_squared_) * scale * scale;
}

bool Q34DeadLaneProver::center_inside(i64 alpha, i64 beta, i64 disk_factor) const {
  i128 norm = 0;
  for (std::size_t i = 0; i < 3; ++i) {
    const i128 t = static_cast<i128>(a_basis_[i]) * alpha + static_cast<i128>(b_basis_[i]) * beta;
    norm += t * t;
  }
  return static_cast<i128>(disk_factor) * norm <= static_cast<i128>(diameter_squared_) * scale * scale;
}

std::uint8_t Q34DeadLaneProver::cell(const Cell& c, unsigned depth, std::span<const std::uint32_t> frontier,
                                     std::size_t inherited, std::uint8_t lanes, Q34DeadLaneWork& work) {
  // Returns the lanes of `lanes` that no center of this closed cell refutes:
  // each is certified on the cell (disjoint from its disk, or T uniform
  // interiors) or by its sub-cells. q3's disk lies inside q4's and T3>T4.
  counter_add(work.cells);
  std::uint8_t need = 0;
  if ((lanes & 2U) != 0 && !outside(c, q3_disk_factor)) need = static_cast<std::uint8_t>(need | 2U);
  if ((lanes & 4U) != 0 && !outside(c, q4_disk_factor)) need = static_cast<std::uint8_t>(need | 4U);
  if (need == 0) { counter_add(work.outside_cells); return lanes; }
  std::size_t inside = inherited;
  if (depth >= min_depth_) {
    const std::size_t target = (need & 2U) != 0 ? threshold3_ : threshold4_;
    if (inside >= target) { counter_add(work.deep_cells); return lanes; }
    auto& next = levels_[depth];
    next.clear();
    u64 tests = 0;
    for (const auto id : frontier) {
      ++tests;
      const auto& f = forms_[id];
      const i64 maximum = f.constant + f.x * (f.x < 0 ? c.left : c.right) + f.y * (f.y < 0 ? c.bottom : c.top);
      if (maximum < 0) {
        if (++inside >= target) {
          counter_add(work.uniform_tests, tests);
          counter_add(work.deep_cells);
          return lanes;
        }
        continue;
      }
      const i64 minimum = f.constant + f.x * (f.x < 0 ? c.right : c.left) + f.y * (f.y < 0 ? c.top : c.bottom);
      if (minimum < 0) next.push_back(id);  // strictly inside somewhere in the cell
    }
    counter_add(work.uniform_tests, tests);
    // q4 may already hold here (T4 <= inside < T3): only q3 is refined.
    if ((need & 4U) != 0 && inside >= threshold4_) need = static_cast<std::uint8_t>(need & ~4U);
    // Exact depth at a lane center of the cell (its corner): below the
    // lane's T no certificate exists, the lane stops at once (cost only).
    // Every site dropped above is positive there, every credited one negative.
    std::size_t count = inside;
    u64 points = 0;
    bool counted = false, refuted = false;
    for (const std::uint8_t lane : {std::uint8_t{2}, std::uint8_t{4}}) {
      if ((need & lane) == 0) continue;
      const std::size_t lane_target = lane == 2U ? threshold3_ : threshold4_;
      if (!center_inside(c.left, c.bottom, lane == 2U ? q3_disk_factor : q4_disk_factor)) continue;
      if (!counted) {
        for (const auto id : next) {
          ++points;
          const auto& f = forms_[id];
          if (f.constant + f.x * c.left + f.y * c.bottom < 0 && ++count >= threshold3_) break;
        }
        counted = true;
      }
      if (count < lane_target) {
        need = static_cast<std::uint8_t>(need & ~lane);
        lanes = static_cast<std::uint8_t>(lanes & ~lane);
        refuted = true;
      }
    }
    counter_add(work.point_tests, points);
    if (refuted) counter_add(work.failed_cells);
    if (need == 0) return lanes;
    frontier = next;
  }
  if (depth == max_depth_) {
    counter_add(work.failed_cells);
    return static_cast<std::uint8_t>(lanes & ~need);
  }
  const i64 x = c.left + (c.right - c.left) / 2, y = c.bottom + (c.top - c.bottom) / 2;
  const Cell children[4] = {{c.left, x, c.bottom, y}, {x, c.right, c.bottom, y},
                            {c.left, x, y, c.top}, {x, c.right, y, c.top}};
  for (const auto& child : children) {
    const std::uint8_t kept = cell(child, depth + 1, frontier, inside, need, work);
    lanes = static_cast<std::uint8_t>(lanes & ~(need & ~kept));  // lanes refuted in the child
    need = static_cast<std::uint8_t>(need & kept);
    if (need == 0) break;
  }
  return lanes;
}

}  // namespace mhgp9::gen
