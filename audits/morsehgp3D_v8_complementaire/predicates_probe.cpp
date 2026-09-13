#include "spindle/predicates.hpp"

#include <iostream>
#include <random>
#include <vector>

namespace {

using namespace mhgp8;

// Independent midpoint formulation: 4H = |b-a|^2 - |2z-a-b|^2 and
// |(b-a) x (2z-a-b)|^2 = 4Xi. All intermediates fit signed i128 for u16.
bool oracle(Lane lane, Point3 a, Point3 b, Point3 z) {
  std::array<i128, 3> direction{}, offset{};
  i128 h4 = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    direction[axis] = i128(b[axis]) - a[axis];
    offset[axis] = 2 * i128(z[axis]) - a[axis] - b[axis];
    h4 += direction[axis] * direction[axis] - offset[axis] * offset[axis];
  }
  if (h4 <= 0) return false;
  if (lane == Lane::Q2) return true;
  i128 xi4 = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i128 cross = direction[(axis + 1) % 3] * offset[(axis + 2) % 3] -
                       direction[(axis + 2) % 3] * offset[(axis + 1) % 3];
    xi4 += cross * cross;
  }
  return (lane == Lane::Q3 ? 3 : 2) * h4 * h4 > 4 * xi4;
}

std::vector<Point3> samples(Box3 box) {
  std::vector<Point3> points;
  for (unsigned corner = 0; corner < 8; ++corner)
    points.push_back(box_corner(box, corner));
  points.push_back({
      std::uint16_t((unsigned(box.low.x) + box.high.x) / 2),
      std::uint16_t((unsigned(box.low.y) + box.high.y) / 2),
      std::uint16_t((unsigned(box.low.z) + box.high.z) / 2)});
  return points;
}

void print_point(const char* name, Point3 point) {
  std::cerr << ' ' << name << "=(" << point.x << ',' << point.y << ','
            << point.z << ')';
}

void print_box(const char* name, Box3 box) {
  std::cerr << ' ' << name << "=[(" << box.low.x << ',' << box.low.y << ','
            << box.low.z << "),(" << box.high.x << ',' << box.high.y << ','
            << box.high.z << ")]";
}

}  // namespace

int main() {
  std::mt19937_64 rng(937104);
  const auto point = [&rng](unsigned scale) {
    return Point3{std::uint16_t(rng() % scale), std::uint16_t(rng() % scale),
                  std::uint16_t(rng() % scale)};
  };
  const auto box = [&point](unsigned scale) {
    const Point3 p = point(scale), q = point(scale);
    return Box3{{std::min(p.x, q.x), std::min(p.y, q.y), std::min(p.z, q.z)},
                {std::max(p.x, q.x), std::max(p.y, q.y), std::max(p.z, q.z)}};
  };
  PredicateWork work;
  std::uint64_t points = 0, universal = 0, credit = 0;
  std::uint64_t negative = 0, uncertain = 0, checks = 0;
  for (unsigned index = 0; index < 300000; ++index) {
    const unsigned scale = index < 150000 ? 6 : 65536;
    const Point3 a = point(scale), b = point(scale), z = point(scale);
    for (const auto lane : {Lane::Q2, Lane::Q3, Lane::Q4}) {
      ++points;
      if (point_witness(lane, a, b, z, work) != oracle(lane, a, b, z)) {
        std::cerr << "point mismatch sample=" << index
                  << " lane=" << unsigned(lane);
        print_point("a", a); print_point("b", b); print_point("z", z);
        std::cerr << '\n';
        return 1;
      }
    }
  }
  for (unsigned index = 0; index < 30000; ++index) {
    const unsigned scale = index < 15000 ? 6 : 65536;
    Box3 a = box(scale), b = box(scale), z = box(scale);
    // Include point boxes often enough to exercise credited regions.
    if (index % 3 == 0) a = singleton_box(point(scale));
    if (index % 4 == 0) b = singleton_box(point(scale));
    if (index % 5 == 0) z = singleton_box(point(scale));
    const auto anchors = samples(a), opposites = samples(b), witnesses = samples(z);
    for (const auto lane : {Lane::Q2, Lane::Q3, Lane::Q4}) {
      const auto verdict = classify_witness_block(lane, a, b, z, work);
      if (verdict == BlockDecision::Uncertain) ++uncertain;
      else if (verdict == BlockDecision::Credit) ++credit;
      else ++negative;
      for (const auto& anchor : anchors) {
        for (const auto& witness : witnesses) {
          const bool entire = universal_witness(lane, anchor, b, witness, work);
          ++universal;
          bool discrete = true;
          for (const auto& opposite : opposites) {
            ++checks;
            const bool value = oracle(lane, anchor, opposite, witness);
            discrete = discrete && value;
          }
          int failure = 0;
          if (entire != discrete) failure = 2;
          else if (verdict == BlockDecision::Credit && !discrete) failure = 3;
          else if (verdict == BlockDecision::NoCredit && discrete) failure = 4;
          if (failure != 0) {
            std::cerr << (failure == 2 ? "universal mismatch" :
                          failure == 3 ? "credit mismatch" : "negative mismatch")
                      << " sample=" << index << " lane=" << unsigned(lane);
            print_box("A", a); print_box("B", b); print_box("Z", z);
            print_point("anchor", anchor); print_point("witness", witness);
            std::cerr << '\n';
            return failure;
          }
        }
      }
    }
  }
  std::cout << "PASS point_tests=" << points << " universal_queries=" << universal
            << " credit_blocks=" << credit << " negative_blocks=" << negative
            << " uncertain_blocks=" << uncertain << " oracle_checks=" << checks
            << '\n';
}
