// Auditeur B (14 sept. 2026) — COPIE D AUDIT de morsehgp3D_v8/src/pipeline/tube_credits.hpp
// (da366f7f) sous le namespace tube_audit : seule modification, largeur de cellule
// multipliee par MHGP8_TUBE_WIDTH_MUL (defaut 1 = produit) pour diagnostiquer le
// certificat des tubes sur des nuages irreguliers. Pas un moteur.
#pragma once

#include "local_credits.hpp"

#include <algorithm>
#include <cstdlib>
#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp8::tube_audit {

// New C++ realization of the independently supplied geometric certificate:
// audits/P0_TUBES_ET_RANGS.md, sections 1-3. No historical qualification is
// inherited. Geometry, grid, sort and transverse bounds can now be prepared
// once for all lanes. Only this temporary record holds grid/transverse data;
// the immutable preparation retains compact projections/IDs and cell bounds.
// Suffix populations are counted without expanding witness identities.
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

class PreparedTubes final {
 private:
  struct Projection {
    i64 projection;
    std::size_t point_id;
  };

  struct Cell {
    Range records;
    i128 bound;
  };

  Range range_;
  std::vector<Projection> records_;
  std::vector<Cell> cells_;

 public:
  // The input span exists only while constructing this object. No borrowed
  // coordinates, mutable records or externally supplied credits survive.
  // The API which publishes the resulting plans keeps their RectanglePtr.
  PreparedTubes(const PreparedRectangle& rectangle, Range range, const Box3& own,
                const Box3& opposite, Work& work) : range_(range) {
    require_valid_box(own);
    require_valid_box(opposite);
    const auto points = rectangle.points();
    if (range.first > range.last || range.last > points.size()) {
      throw std::invalid_argument("mhgp8 tube range is outside its owner");
    }
    if (range.first == range.last) {
      return;
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
      return;
    }

    // M=65535: |d_i|<=2M, |d.p|<=6M^2, |d cross p|_i<=4M^2.
    // Differences along the same d satisfy |Delta|<=6M^2 and transverse
    // coordinate spans <=4M^2. Q<=48M^4, so 16Q and 9Delta^2 are <2^74.
    // Products in Q and the acceptance test are widened BEFORE multiplying.
    // Raw coordinates, signed differences, cell offsets and width fit i64.
    // AUDIT: largeur de cellule configurable (multiplicateur MHGP8_TUBE_WIDTH_MUL, defaut 1 = produit).
    const i64 width_mul = std::getenv("MHGP8_TUBE_WIDTH_MUL") ? std::atoll(std::getenv("MHGP8_TUBE_WIDTH_MUL")) : 1;
    const i64 width = 4 * maximum_component * (width_mul > 0 ? width_mul : 1);
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

    records_.reserve(records.size());
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
      cells_.push_back(Cell{{begin, end}, bound});
      for (std::size_t index = begin; index < end; ++index) {
        records_.push_back(Projection{records[index].projection, records[index].point_id});
      }
      begin = end;
    }
    // Temporary cell coordinates/transverse vectors are released here. Each
    // permanent record stores only its projection and original point ID.
  }

  PreparedTubes(const PreparedTubes&) = delete;
  PreparedTubes& operator=(const PreparedTubes&) = delete;
  PreparedTubes(PreparedTubes&&) = delete;
  PreparedTubes& operator=(PreparedTubes&&) = delete;

  [[nodiscard]] Range range() const noexcept { return range_; }

  [[nodiscard]] std::vector<std::uint8_t> credits(
      Lane lane, std::uint8_t need, Work& work) const {
    require_valid_lane(lane);
    std::vector<std::uint8_t> result(range_.size(), 0);
    if (need == 0) {
      return result;
    }
    const i128 q_coefficient = lane == Lane::Q4 ? 16 : 1;
    const i128 gap_coefficient = lane == Lane::Q3 ? 1 : 9;
    for (const Cell& cell : cells_) {
      // For increasing anchor projections the first admissible witness never
      // moves backwards. Each lane pays <=2m tests over all cells, not m^2;
      // every success denotes a suffix counted by one subtraction.
      const auto begin = cell.records.first;
      const auto end = cell.records.last;
      std::size_t first = begin;
      for (std::size_t anchor = begin; anchor < end; ++anchor) {
        while (first < end) {
          counter_add(work.tube_sweep_tests);
          const i64 delta = records_[first].projection - records_[anchor].projection;
          if (delta > 0 && q_coefficient * cell.bound <=
                               gap_coefficient * static_cast<i128>(delta) * delta) {
            break;
          }
          ++first;
        }
        const auto count = static_cast<std::uint8_t>(
            std::min<std::size_t>(need, end - first));
        result[records_[anchor].point_id - range_.first] = count;
        // Sum of SATURATED per-anchor credits, not expanded witness identities.
        counter_add(work.tube_credited_sites, count);
      }
    }
    return result;
  }
};

// Preparation counters (records/cells/sort/fallback) are charged only by the
// constructor. This overload changes only sweep_tests and credited_sites;
// callers can pass separate Work objects instead of counting preparation
// three times or silently charging it to one particular lane.
[[nodiscard]] inline std::vector<std::uint8_t> credits(
    const PreparedTubes& prepared, Lane lane, std::uint8_t need, Work& work) {
  return prepared.credits(lane, need, work);
}

// Compatibility path: one preparation and one query, as before. Its zero-need
// case validates the same cheap inputs but does not prepare unused geometry.
[[nodiscard]] inline std::vector<std::uint8_t> credits(
    const PreparedRectangle& rectangle, Range range, const Box3& own,
    const Box3& opposite, Lane lane, std::uint8_t need, Work& work) {
  require_valid_lane(lane);
  if (need == 0) {
    require_valid_box(own);
    require_valid_box(opposite);
    if (range.first > range.last || range.last > rectangle.points().size()) {
      throw std::invalid_argument("mhgp8 tube range is outside its owner");
    }
    return std::vector<std::uint8_t>(range.size(), 0);
  }
  const PreparedTubes prepared(rectangle, range, own, opposite, work);
  return credits(prepared, lane, need, work);
}

}  // namespace mhgp8::tube_audit
