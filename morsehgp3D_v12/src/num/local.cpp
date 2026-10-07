// Predicats en repere de feuille (num/local.hpp). Construction neuve de la v12 : la v11 calculait la distance du
// reservoir dans src/catalogue/boxes.cpp en i64 sur le seul seuil de G1 (CST-0208).
#include "num/local.hpp"

namespace mhgp12::num {
namespace {

bool covered(const Frame& frame, const std::array<u64, 3>& point) noexcept {
  for (int j = 0; j < 3; ++j)
    if (point[j] < frame.origin()[j] || point[j] > frame.upper()[j]) return false;
  return true;
}

}  // namespace

Result<u128> reservoir_distance(const Frame& frame, const std::array<u32, 3>& site, const std::array<u64, 3>& lo,
                                const std::array<u64, 3>& hi, LaneCount* lanes) noexcept {
  const std::array<u64, 3> point{site[0], site[1], site[2]};
  if (frame.empty() || !covered(frame, point) || !covered(frame, lo) || !covered(frame, hi))
    return fail(Reason::parameter_out_of_range);
  for (int j = 0; j < 3; ++j)
    if (lo[j] > hi[j]) return fail(Reason::parameter_out_of_range);
  // Ecarts dans le repere : x_j - lo_j et hi_j - x_j sont dans ]-2^s, 2^s[, donc |2x_j - lo_j - hi_j| < 2^(s+1).
  // Coordonnees <= 2^32 : les differences tiennent en i64 a toute etendue.
  std::array<i64, 3> doubled{};
  for (int j = 0; j < 3; ++j)
    doubled[j] = (static_cast<i64>(point[j]) - static_cast<i64>(lo[j])) +
                 (static_cast<i64>(point[j]) - static_cast<i64>(hi[j]));
  if (frame.tier() != Tier::wide) {
    static_assert(TierBudgets<Tier::medium>::reservoir <= 63);
    count_lane(lanes, Lane::native);
    return static_cast<u128>(doubled[0] * doubled[0] + doubled[1] * doubled[1] + doubled[2] * doubled[2]);
  }
  static_assert(TierBudgets<Tier::wide>::reservoir <= 127);
  count_lane(lanes, Lane::wide);
  return static_cast<u128>(i128{doubled[0]} * doubled[0] + i128{doubled[1]} * doubled[1] +
                           i128{doubled[2]} * doubled[2]);
}

}  // namespace mhgp12::num
