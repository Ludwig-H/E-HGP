#include "pipeline/local_credits.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>

namespace {

using namespace mhgp8;

void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

// Midpoint formulation, independent of the producer's product expression.
// The exhaustive loops below are bounded judges, never a producer path.
bool oracle(Lane lane, Point3 a, Point3 b, Point3 z) {
  std::array<i128, 3> direction{}, offset{};
  i128 h4 = 0;
  i128 xi4 = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    direction[axis] = i128(b[axis]) - a[axis];
    offset[axis] = 2 * i128(z[axis]) - a[axis] - b[axis];
    h4 += direction[axis] * direction[axis] - offset[axis] * offset[axis];
  }
  if (h4 <= 0) return false;
  if (lane == Lane::Q2) return true;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i128 cross = direction[(axis + 1) % 3] * offset[(axis + 2) % 3] -
                       direction[(axis + 2) % 3] * offset[(axis + 1) % 3];
    xi4 += cross * cross;
  }
  return (lane == Lane::Q3 ? 3 : 2) * h4 * h4 > 4 * xi4;
}

u64 plans = 0;
u64 credit_checks = 0;
u64 pair_checks = 0;
u64 credits = 0;
u64 sweeps = 0;
u64 fallbacks = 0;
u64 inactive = 0;

void examine(RectangleInput input, unsigned kmax, unsigned separation,
             int expected_fallback = -1) {
  const auto rectangle = prepare_rectangle(std::move(input), kmax, separation);
  for (const auto lane : {Lane::Q2, Lane::Q3, Lane::Q4}) {
    const auto plan = make_credit_plan(rectangle, lane, Strategy::Tubes);
    ++plans;
    const auto& work = plan.work();
    u64 sum = 0;
    for (const auto credit : plan.a_credits()) sum += credit;
    for (const auto credit : plan.b_credits()) sum += credit;
    require(sum == work.tube_credited_sites, "credited_sites count");
    require(work.tube_sweep_tests <= 2 * work.tube_records, "sweep linear bound");
    require(work.tube_cells <= work.tube_records, "cell count");
    require(work.predicates.point_tests == 0 && work.predicates.universal_queries == 0,
            "tube point test hidden");
    const unsigned need = plan.threshold() - plan.core_credit();
    if (need == 0) {
      ++inactive;
      require(work.tube_records == 0 && work.tube_separation_fallbacks == 0,
              "inactive work");
    } else if (expected_fallback >= 0) {
      require(work.tube_separation_fallbacks == u64(expected_fallback),
              "fallback boundary");
    }
    if (work.tube_separation_fallbacks != 0) {
      require(work.tube_separation_fallbacks == 2, "asymmetric fallback");
      require(work.tube_records == 0 && sum == 0, "fallback did work");
    }
    credits += sum;
    sweeps += work.tube_sweep_tests;
    fallbacks += work.tube_separation_fallbacks;
    for (unsigned side = 0; side < 2; ++side) {
      const auto range = side != 0 ? rectangle->b_range() : rectangle->a_range();
      const auto& opposite = side != 0 ? rectangle->a_box() : rectangle->b_box();
      const auto values = side != 0 ? plan.b_credits() : plan.a_credits();
      for (std::size_t anchor = range.first; anchor < range.last; ++anchor) {
        unsigned exact = 0;
        for (std::size_t witness = range.first; witness < range.last; ++witness) {
          if (witness == anchor) continue;
          bool universal = true;
          for (unsigned corner = 0; corner < 8; ++corner) {
            const Point3 b{
                (corner & 1U) != 0 ? opposite.high.x : opposite.low.x,
                (corner & 2U) != 0 ? opposite.high.y : opposite.low.y,
                (corner & 4U) != 0 ? opposite.high.z : opposite.low.z};
            const bool value = oracle(lane, rectangle->points()[anchor], b,
                                      rectangle->points()[witness]);
            universal = universal && value;
          }
          if (universal) ++exact;
        }
        ++credit_checks;
        require(values[anchor - range.first] <= std::min(need, exact),
                "unsafe tube credit");
      }
    }
    std::set<std::pair<std::size_t, std::size_t>> expansion;
    plan.for_each_candidate([&expansion](std::size_t a, std::size_t b) {
      require(expansion.emplace(a, b).second, "duplicate expansion");
    });
    require(expansion.size() == plan.candidate_pairs(), "expansion count");
    for (std::size_t a = rectangle->a_range().first; a < rectangle->a_range().last; ++a) {
      for (std::size_t b = rectangle->b_range().first; b < rectangle->b_range().last; ++b) {
        ++pair_checks;
        require(plan.keeps(a, b) == expansion.contains({a, b}), "keeps mismatch");
        if (!plan.keeps(a, b) && plan.threshold() > 0) {
          unsigned exact = 0;
          for (std::size_t z = 0; z < rectangle->points().size(); ++z) {
            if (z != a && z != b && oracle(lane, rectangle->points()[a],
                                          rectangle->points()[b], rectangle->points()[z])) {
              ++exact;
            }
          }
          require(exact >= plan.threshold(), "unsafe pair rejection");
        }
      }
    }
  }
}

}  // namespace

int main() {
  try {
    // Box diameter 10, center distance D=d. The boundary is D=50=10R.
    // d=49 specifically catches the erroneous 25*diagonal^2 certificate.
    for (const unsigned distance : {20U, 49U, 50U, 51U}) {
      RectangleInput input{{{0, 0, 0}, {10, 0, 0},
                             {std::uint16_t(distance), 0, 0},
                             {std::uint16_t(distance + 10), 0, 0}},
                            {0, 2}, {2, 4}, {}};
      examine(input, 10, 1, distance < 50 ? 2 : 0);
      std::swap(input.a, input.b);
      examine(input, 10, 1, distance < 50 ? 2 : 0);
    }
    // Here Delta can reach 3*10^9, so 9*Delta^2 exceeds signed i64.
    // Reflecting coordinates exercises signed directions and cross products.
    for (unsigned reflected = 0; reflected < 8; ++reflected) {
      RectangleInput wide;
      for (unsigned side = 0; side < 2; ++side) {
        for (unsigned vertex = 0; vertex < 8; ++vertex) {
          std::array<unsigned, 3> xyz{};
          for (std::size_t axis = 0; axis < 3; ++axis) {
            const unsigned value = side * 50000 + ((vertex >> axis) & 1U) * 10000;
            xyz[axis] = ((reflected >> axis) & 1U) != 0 ? 65535 - value : value;
          }
          wide.points.push_back({std::uint16_t(xyz[0]), std::uint16_t(xyz[1]),
                                 std::uint16_t(xyz[2])});
        }
      }
      wide.a = {0, 8};
      wide.b = {8, 16};
      examine(std::move(wide), 10, 1, 0);
    }
    RectangleInput extremes{{{0, 0, 0}, {65535, 65535, 65535}}, {0, 1}, {1, 2}, {}};
    examine(extremes, 10, 4294967295U, 0);
    std::mt19937_64 rng(930174);
    for (unsigned sample = 0; sample < 1000; ++sample) {
      unsigned code = 1 + sample % 26;
      std::array<int, 3> sign{};
      for (auto& component : sign) {
        component = int(code % 3) - 1;
        code /= 3;
      }
      if (sign == std::array<int, 3>{0, 0, 0}) sign[0] = 1;
      RectangleInput input;
      std::set<std::array<unsigned, 3>> unique;
      for (unsigned side = 0; side < 2; ++side) {
        const auto first = input.points.size();
        const unsigned count = 2 + unsigned(rng() % 19);
        while (input.points.size() < first + count) {
          std::array<unsigned, 3> xyz{};
          for (std::size_t axis = 0; axis < 3; ++axis) {
            const int component = sign[axis];
            const unsigned base = component == 0 ? 32000U :
                ((component > 0) == (side == 0) ? 0U : 65530U);
            xyz[axis] = base + unsigned(rng() % 6);
          }
          if (unique.insert(xyz).second) {
            input.points.push_back({std::uint16_t(xyz[0]), std::uint16_t(xyz[1]),
                                    std::uint16_t(xyz[2])});
          }
        }
        if (side == 0) input.a = {first, input.points.size()};
        else input.b = {first, input.points.size()};
      }
      if (sample % 7 == 0) {
        const auto a = input.points[input.a.first], b = input.points[input.b.first];
        std::array<unsigned, 3> midpoint{};
        for (std::size_t axis = 0; axis < 3; ++axis)
          midpoint[axis] = (unsigned(a[axis]) + b[axis]) / 2;
        if (unique.insert(midpoint).second) {
          input.core_candidates.push_back(input.points.size());
          input.points.push_back({std::uint16_t(midpoint[0]), std::uint16_t(midpoint[1]),
                                  std::uint16_t(midpoint[2])});
        }
      }
      examine(std::move(input), 1 + sample % 10, 12, 0);
    }
    require(credits > 0 && fallbacks == 24 && inactive > 0, "nonvacuity");
    std::cout << "PASS plans=" << plans << " credits_checked=" << credit_checks
              << " pair_checks=" << pair_checks << " credited_sites=" << credits
              << " sweep_tests=" << sweeps << " fallbacks=" << fallbacks
              << " inactive_or_core_saturated=" << inactive << '\n';
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
