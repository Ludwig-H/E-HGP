// Sonde distincte : distance native, ancienne puissance q1 et reference entierement Wide, sans changer num_probe.
#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "num/num.hpp"
#include "power_reference.hpp"

using namespace mhgp12;
using namespace mhgp12::num;

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

Outcome query(const std::array<i64, 6>& raw) {
  const auto a = Point::make(raw[0], raw[1], raw[2]), b = Point::make(raw[3], raw[4], raw[5]);
  if (!a.ok()) return a.outcome();
  if (!b.ok()) return b.outcome();
  const auto old = power(Sphere::point(a.value()), b.value());
  if (!old.ok()) return old.outcome();
  std::cout << "ok " << squared_distance(a.value(), b.value()) << ' ' << hex(old.value()) << ' '
            << hex(num_test::wide_power(Sphere::point(a.value()), b.value())) << '\n';
  return {};
}
}  // namespace

int main() {
  std::cout << "bits " << kCoordBits << '\n';
  for (;;) {
    std::cin >> std::ws;
    if (std::cin.eof()) return 0;
    std::array<i64, 6> raw{};
    for (auto& coordinate : raw) {
      if (!(std::cin >> coordinate)) {
        std::cout << "refused input_unreadable\n";
        return 2;
      }
    }
    const auto outcome = query(raw);
    if (!outcome.ok()) {
      std::cout << "refused " << reason_name(outcome.reason) << '\n';
      return exit_code(outcome);
    }
  }
}
