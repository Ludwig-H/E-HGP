#include "lanes/q34_dead_lanes.hpp"

#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace mhgp9::gen {
namespace {
constexpr unsigned scale_bits = 20;  // Q4LocalCell::scale_bits
constexpr i64 scale = i64{1} << scale_bits;
using Vec = std::array<i64, 3>;
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
  const auto points = index.cloud().points();
  const auto order = index.spatial_order();
  const auto ids = cover.edge_ids();
  const auto a = points[ids[0]], b = points[ids[1]];
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
  forms_.clear();
  counter_add(work.loads);
  for (const auto range : cover.ranges())
    for (auto rank = range.first; rank < range.last; ++rank) {
      const auto id = order[rank];
      if (id == ids[0] || id == ids[1]) continue;
      Vec w{};
      for (std::size_t i = 0; i < 3; ++i) w[i] = 2 * static_cast<i64>(points[id][i]) - midpoint_twice[i];
      // M=262143: |w_i| <= 2M, |dot(w,w)-|v|^2| <= 15M^2 < 2^40 (scaled < 2^60),
      // |x|,|y| <= 8M^2 < 2^39 (Q4LocalGeometry::form).
      forms_.push_back({scale * (dot(w, w) - diameter_squared_), -2 * dot(w, a_basis_), -2 * dot(w, b_basis_)});
    }
  if (forms_.size() > std::numeric_limits<std::uint32_t>::max())
    throw std::overflow_error("mhgp9 gen dead-lane prover cover exceeds u32 frontier IDs");
  all_.resize(forms_.size());
  for (std::size_t i = 0; i < all_.size(); ++i) all_[i] = static_cast<std::uint32_t>(i);
  counter_add(work.form_sites, static_cast<u64>(forms_.size()));
  loaded_ = true;
}

std::size_t Q34DeadLaneProver::retained_bytes() const {
  std::size_t bytes = forms_.capacity() * sizeof(Form) + all_.capacity() * sizeof(std::uint32_t);
  for (const auto& level : levels_) bytes += level.capacity() * sizeof(std::uint32_t);
  return bytes;
}

bool Q34DeadLaneProver::prove_q3(unsigned kmax, Q34DeadLaneWork& work) {
  if (kmax < 2) return false;
  const bool dead = prove(3, kmax - 1, work);
  counter_add(dead ? work.q3_proved : work.q3_open);
  return dead;
}

bool Q34DeadLaneProver::prove_q4(unsigned kmax, Q34DeadLaneWork& work) {
  if (kmax < 3) return false;
  const bool dead = prove(2, kmax - 2, work);
  counter_add(dead ? work.q4_proved : work.q4_open);
  return dead;
}

bool Q34DeadLaneProver::prove(i64 disk_factor, std::size_t threshold, Q34DeadLaneWork& work) {
  if (!loaded_) throw std::logic_error("mhgp9 gen dead-lane prover used before load");
  disk_factor_ = disk_factor;
  threshold_ = threshold;
  return cell({-2 * scale, 2 * scale, -2 * scale, 2 * scale}, 0, all_, 0, work);
}

bool Q34DeadLaneProver::outside(const Cell& c) const {
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
  return static_cast<i128>(disk_factor_) * norm > static_cast<i128>(diameter_squared_) * scale * scale;
}

bool Q34DeadLaneProver::center_inside(i64 alpha, i64 beta) const {
  i128 norm = 0;
  for (std::size_t i = 0; i < 3; ++i) {
    const i128 t = static_cast<i128>(a_basis_[i]) * alpha + static_cast<i128>(b_basis_[i]) * beta;
    norm += t * t;
  }
  return static_cast<i128>(disk_factor_) * norm <= static_cast<i128>(diameter_squared_) * scale * scale;
}

bool Q34DeadLaneProver::cell(const Cell& c, unsigned depth, std::span<const std::uint32_t> frontier,
                             std::size_t inherited, Q34DeadLaneWork& work) {
  counter_add(work.cells);
  if (outside(c)) { counter_add(work.outside_cells); return true; }
  std::size_t inside = inherited;
  if (depth >= min_depth_) {
    if (inside >= threshold_) { counter_add(work.deep_cells); return true; }
    auto& next = levels_[depth];
    next.clear();
    u64 tests = 0;
    for (const auto id : frontier) {
      ++tests;
      const auto& f = forms_[id];
      const i64 maximum = f.constant + f.x * (f.x < 0 ? c.left : c.right) + f.y * (f.y < 0 ? c.bottom : c.top);
      if (maximum < 0) {
        if (++inside >= threshold_) {
          counter_add(work.uniform_tests, tests);
          counter_add(work.deep_cells);
          return true;
        }
        continue;
      }
      const i64 minimum = f.constant + f.x * (f.x < 0 ? c.right : c.left) + f.y * (f.y < 0 ? c.top : c.bottom);
      if (minimum < 0) next.push_back(id);  // strictly inside somewhere in the cell
    }
    counter_add(work.uniform_tests, tests);
    // Exact depth at the lane center (left, bottom) when it is one: every
    // site dropped above is positive there, every credited one negative.
    if (center_inside(c.left, c.bottom)) {
      std::size_t count = inside;
      u64 points = 0;
      for (const auto id : next) {
        ++points;
        const auto& f = forms_[id];
        if (f.constant + f.x * c.left + f.y * c.bottom < 0 && ++count >= threshold_) break;
      }
      counter_add(work.point_tests, points);
      if (count < threshold_) { counter_add(work.failed_cells); return false; }
    }
    frontier = next;
  }
  if (depth == max_depth_) { counter_add(work.failed_cells); return false; }
  const i64 x = c.left + (c.right - c.left) / 2, y = c.bottom + (c.top - c.bottom) / 2;
  return cell({c.left, x, c.bottom, y}, depth + 1, frontier, inside, work) &&
         cell({x, c.right, c.bottom, y}, depth + 1, frontier, inside, work) &&
         cell({c.left, x, y, c.top}, depth + 1, frontier, inside, work) &&
         cell({x, c.right, y, c.top}, depth + 1, frontier, inside, work);
}

}  // namespace mhgp9::gen
