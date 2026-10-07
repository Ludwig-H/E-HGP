// Sonde de comparaison, deux supports de 1..4 sites ; padding fixe, aucune formule du juge Fraction.
#include <array>
#include <iostream>

#include "num/num.hpp"

using namespace mhgp12;
using namespace mhgp12::num;

namespace {
Result<std::optional<Sphere>> make(int q, const std::array<Point, 4>& p) {
  if (q == 1) return std::optional<Sphere>{Sphere::point(p[0])};
  if (q == 2) return Sphere::through(p[0], p[1]);
  if (q == 3) return Sphere::through(p[0], p[1], p[2]);
  if (q == 4) return Sphere::through(p[0], p[1], p[2], p[3]);
  return fail(Reason::parameter_out_of_range);
}
Outcome query(int qa, int qb) {
  std::array<Point, 8> points{};
  for (auto& p : points) {
    i64 x = 0, y = 0, z = 0;
    if (!(std::cin >> x >> y >> z)) return fail(Reason::input_unreadable);
    const auto made = Point::make(x, y, z);
    if (!made.ok()) return made.outcome();
    p = made.value();
  }
  const auto a = make(qa, {points[0], points[1], points[2], points[3]});
  const auto b = make(qb, {points[4], points[5], points[6], points[7]});
  if (!a.ok()) return a.outcome();
  if (!b.ok()) return b.outcome();
  if (!a.value() || !b.value()) std::cout << "degenerate\n";
  else std::cout << "ok " << compare_centers(*a.value(), *b.value()) << '\n';
  return {};
}
}  // namespace

int main() {
  std::cout << "bits " << kCoordBits << '\n';
  int qa = 0, qb = 0;
  while (std::cin >> qa) {
    if (!(std::cin >> qb)) return 2;
    const auto outcome = query(qa, qb);
    if (!outcome.ok()) {
      std::cout << "refused " << reason_name(outcome.reason) << '\n';
      return exit_code(outcome);
    }
  }
  return std::cin.eof() ? 0 : 2;
}
