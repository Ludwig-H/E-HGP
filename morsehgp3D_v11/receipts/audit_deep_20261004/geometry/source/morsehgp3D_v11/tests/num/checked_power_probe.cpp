// Protocole Fraction q3 : coefficients, puissance, signe, bornes et trois essais internes observes separement.
#include <array>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "checked_power_support.hpp"

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
}

int main() {
  std::cout << "bits " << kCoordBits << '\n';
  i64 first = 0;
  while (std::cin >> first) {
    std::array<Point, 6> points{};
    std::optional<Outcome> invalid;
    for (u32 i = 0; i < points.size(); ++i) {
      i64 x = first, y = 0, z = 0;
      if ((i != 0 && !(std::cin >> x)) || !(std::cin >> y >> z)) return 2;
      const auto made = Point::make(x, y, z);
      if (!made.ok()) { if (!invalid) invalid = made.outcome(); }
      else points[i] = made.value();
    }
    if (invalid) { std::cout << "refused " << reason_name(invalid->reason) << '\n'; continue; }
    const auto box = Box::make(points[4], points[5]);
    if (!box.ok()) { std::cout << "refused " << reason_name(box.outcome().reason) << '\n'; continue; }
    const auto made = Sphere::through(points[0], points[1], points[2]);
    if (!made.ok()) return exit_code(made.outcome());
    if (!made.value()) { std::cout << "degenerate\n"; continue; }
    const auto& sphere = *made.value();
    const auto value = power(sphere, points[3]);
    const auto sign = side(sphere, points[3]);
    const auto bounds = power_bounds(sphere, box.value());
    if (!value.ok() || !sign.ok() || !bounds.ok()) return 3;
    std::cout << "ok";
    for (const auto n : sphere.numerator()) std::cout << ' ' << hex(n);
    std::cout << ' ' << hex(sphere.denominator()) << ' ' << hex(value.value()) << ' ' << sign.value()
              << ' ' << hex(bounds.value().lower) << ' ' << hex(bounds.value().upper);
    const auto terms = checked_test::box_terms(sphere, box.value());
    std::cout << ' ' << checked_test::attempt(sphere, checked_test::query_terms(sphere, points[3])).has_value()
              << ' ' << checked_test::attempt(sphere, terms[0]).has_value()
              << ' ' << checked_test::attempt(sphere, terms[1]).has_value() << '\n';
  }
  return std::cin.eof() ? 0 : 2;
}
