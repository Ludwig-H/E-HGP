#pragma once

#include "local_credits.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp8::tube_detail {

// New C++ realization of the independently supplied geometric certificate:
// audits/P0_TUBES_ET_RANGS.md, sections 1-3. No historical qualification is
// inherited. This first interface pays its grid/sort separately for each lane.
// It counts suffix populations, never expands their witness identities.
struct Record {
  std::array<i64, 3> cell{};
  i64 projection{};
  std::size_t point_id{};
  std::array<i64, 3> transverse{};
};

[[nodiscard]] inline i64 diameter_squared(const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(box.high[axis]) -
                      static_cast<i64>(box.low[axis]);
    result += delta * delta;
  }
  return result;
}

[[nodiscard]] inline std::vector<std::uint8_t> credits(
    const PreparedRectangle& rectangle, Range range, const Box3& own,
    const Box3& opposite, Lane lane, std::uint8_t need, Work& work) {
  require_valid_lane(lane);
  require_valid_box(own);
  require_valid_box(opposite);
  const auto points = rectangle.points();
  if (range.first > range.last || range.last > points.size()) {
    throw std::invalid_argument("mhgp8 tube range is outside its owner");
  }
  std::vector<std::uint8_t> result(range.size(), 0);
  if (need == 0 || range.first == range.last) {
    return result;
  }

  std::array<i64, 3> direction{};
  i64 direction_squared = 0;
  i64 maximum_component = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    direction[axis] = static_cast<i64>(opposite.low[axis]) +
                      static_cast<i64>(opposite.high[axis]) -
                      static_cast<i64>(own.low[axis]) -
                      static_cast<i64>(own.high[axis]);
    direction_squared += direction[axis] * direction[axis];
    const i64 magnitude = direction[axis] < 0 ? -direction[axis] : direction[axis];
    maximum_component = std::max(maximum_component, magnitude);
  }

  // d=2(c_opposite-c_own), whereas each box diagonal has length 2R.
  // Thus D>=10R is d^2>=100*max(diagonal^2), NOT 25 times that quantity.
  // The factory also admits weaker separation settings. Do not silently use
  // the tube lemma on those: zero is still a valid minorant and retains the
  // complete residual. This is an observable fallback, not an inactive lane.
  const i64 maximum_diameter_squared =
      std::max(diameter_squared(own), diameter_squared(opposite));
  if (direction_squared == 0 ||
      static_cast<i128>(direction_squared) <
          100 * static_cast<i128>(maximum_diameter_squared)) {
    counter_add(work.tube_separation_fallbacks);
    return result;
  }

  // M=65535: |d_i|<=2M, |d.p|<=6M^2, |d cross p|_i<=4M^2.
  // Differences along the same d satisfy |Delta|<=6M^2 and transverse
  // coordinate spans <=4M^2. Q<=48M^4, so 16Q and 9Delta^2 are <2^74.
  // Products in Q and the acceptance test are widened BEFORE multiplying.
  // Raw coordinates, signed differences, cell offsets and width fit i64.
  const i64 width = 4 * maximum_component;
  std::array<i64, 3> origin{
      std::numeric_limits<i64>::max(), std::numeric_limits<i64>::max(),
      std::numeric_limits<i64>::max()};
  std::vector<Record> records;
  records.reserve(range.size());
  for (std::size_t id = range.first; id < range.last; ++id) {
    const Point3& point = points[id];
    Record record;
    record.point_id = id;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      if (point[axis] < own.low[axis] || point[axis] > own.high[axis]) {
        throw std::invalid_argument("mhgp8 tube point is outside its own box");
      }
      record.projection += direction[axis] * static_cast<i64>(point[axis]);
      const std::size_t j = (axis + 1) % 3;
      const std::size_t k = (axis + 2) % 3;
      record.transverse[axis] = direction[j] * static_cast<i64>(point[k]) -
                                direction[k] * static_cast<i64>(point[j]);
      origin[axis] = std::min(origin[axis], record.transverse[axis]);
    }
    records.push_back(record);
    counter_add(work.tube_records);
  }
  for (Record& record : records) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      // Translation by the common minimum makes the numerator nonnegative:
      // C++ integer division is exactly the desired floor, even when the
      // original cross-product coordinates or d itself were negative.
      record.cell[axis] = (record.transverse[axis] - origin[axis]) / width;
    }
  }
  std::sort(records.begin(), records.end(), [&work](const Record& left,
                                                  const Record& right) {
    counter_add(work.tube_sort_comparisons);
    if (left.cell != right.cell) {
      return left.cell < right.cell;
    }
    if (left.projection != right.projection) {
      return left.projection < right.projection;
    }
    return left.point_id < right.point_id;
  });

  const i128 q_coefficient = lane == Lane::Q4 ? 16 : 1;
  const i128 gap_coefficient = lane == Lane::Q3 ? 1 : 9;
  std::size_t begin = 0;
  while (begin < records.size()) {
    std::size_t end = begin + 1;
    std::array<i64, 3> low = records[begin].transverse;
    std::array<i64, 3> high = low;
    while (end < records.size() && records[end].cell == records[begin].cell) {
      for (std::size_t axis = 0; axis < 3; ++axis) {
        low[axis] = std::min(low[axis], records[end].transverse[axis]);
        high[axis] = std::max(high[axis], records[end].transverse[axis]);
      }
      ++end;
    }
    counter_add(work.tube_cells);
    i128 bound = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 span = high[axis] - low[axis];
      bound += static_cast<i128>(span) * span;
    }

    // For increasing anchor projections the first admissible witness never
    // moves backwards. The loop pays <=2m tests in all cells, not m^2; every
    // success denotes a suffix whose cardinal is obtained by subtraction.
    std::size_t first = begin;
    for (std::size_t anchor = begin; anchor < end; ++anchor) {
      while (first < end) {
        counter_add(work.tube_sweep_tests);
        const i64 delta = records[first].projection - records[anchor].projection;
        if (delta > 0 && q_coefficient * bound <=
                             gap_coefficient * static_cast<i128>(delta) * delta) {
          break;
        }
        ++first;
      }
      const auto count = static_cast<std::uint8_t>(
          std::min<std::size_t>(need, end - first));
      result[records[anchor].point_id - range.first] = count;
      // Sum of SATURATED per-anchor credits, not expanded witness identities.
      counter_add(work.tube_credited_sites, count);
    }
    begin = end;
  }
  return result;
}

}  // namespace mhgp8::tube_detail
