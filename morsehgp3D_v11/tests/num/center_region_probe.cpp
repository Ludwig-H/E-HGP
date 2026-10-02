// Protocole du juge Fraction : q puis trois points, lo et hi (15 coordonnees). Aucun chronometrage.
#include <array>
#include <iostream>

#include "num/num.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

int main() {
  std::cout << "bits " << kCoordBits << '\n';
  int q = 0;
  while (std::cin >> q) {
    std::array<std::array<i64, 3>, 5> input{};
    for (auto& triple : input)
      for (auto& value : triple)
        if (!(std::cin >> value)) return 2;
    std::array<Point, 3> points{};
    Outcome outcome;
    for (std::size_t i = 0; i < points.size(); ++i) {
      const auto& xyz = input[i];
      const auto made = Point::make(xyz[0], xyz[1], xyz[2]);
      if (!made.ok()) outcome = made.outcome();
      else points[i] = made.value();
    }
    const auto region = CenterRegion::make(input[3], input[4]);
    if (outcome.ok() && !region.ok()) outcome = region.outcome();
    if (outcome.ok() && q != 2 && q != 3) outcome = fail(Reason::parameter_out_of_range);
    if (!outcome.ok()) {
      std::cout << "refused " << reason_name(outcome.reason) << '\n';
      continue;
    }
    if (q == 2) {
      std::cout << (bisector_meets(points[0], points[1], region.value()) ? "intersects" : "disjoint") << '\n';
    } else {
      const auto result = center_line_meets(points[0], points[1], points[2], region.value());
      std::cout << (result == CenterLineRelation::degenerate ? "degenerate" :
                    result == CenterLineRelation::intersects ? "intersects" : "disjoint") << '\n';
    }
  }
  return std::cin.eof() ? 0 : 2;
}
