// Sonde bornee de power_bounds : uniquement le protocole du juge Fraction, aucun index ni calcul flottant.
#include <array>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "num/num.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
template <class T>
std::string hex(const T& value) {
  const auto wide = to_wide(value);
  std::ostringstream out;
  if (wide.sign() < 0) out << '-';
  out << std::hex << std::setfill('0');
  for (std::size_t i = wide.words.size(); i != 0; --i) out << std::setw(16) << wide.words[i - 1];
  return out.str();
}

Result<std::optional<Sphere>> sphere(int q, const std::array<Point, 6>& p) {
  if (q == 1) return std::optional<Sphere>{Sphere::point(p[0])};
  if (q == 2) return Sphere::through(p[0], p[1]);
  if (q == 3) return Sphere::through(p[0], p[1], p[2]);
  if (q == 4) return Sphere::through(p[0], p[1], p[2], p[3]);
  return fail(Reason::parameter_out_of_range);
}
}  // namespace

int main() {
  std::cout << "bits " << kCoordBits << '\n';
  int q = 0;
  while (std::cin >> q) {
    std::array<Point, 6> points{};
    for (auto& point : points) {
      i64 x = 0, y = 0, z = 0;
      if (!(std::cin >> x >> y >> z)) return 2;
      const auto made = Point::make(x, y, z);
      if (!made.ok()) return exit_code(made.outcome());
      point = made.value();
    }
    const auto box = Box::make(points[4], points[5]);
    if (!box.ok()) {
      std::cout << "refused " << reason_name(box.outcome().reason) << '\n';
      continue;
    }
    const auto made = sphere(q, points);
    if (!made.ok()) return exit_code(made.outcome());
    if (!made.value()) {
      std::cout << "degenerate\n";
      continue;
    }
    const auto bounds = power_bounds(*made.value(), box.value());
    if (!bounds.ok()) return exit_code(bounds.outcome());
    std::cout << "ok";
    for (const auto n : made.value()->numerator()) std::cout << ' ' << hex(n);
    std::cout << ' ' << hex(made.value()->denominator()) << ' ' << hex(bounds.value().lower) << ' '
              << hex(bounds.value().upper) << '\n';
  }
  return std::cin.eof() ? 0 : 2;
}
