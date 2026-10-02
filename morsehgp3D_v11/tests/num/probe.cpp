// Sonde de test uniquement : protocole texte exact, une ligne de reponse par requete ; aucune troncature.
#include <array>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "num/num.hpp"
#include "power_reference.hpp"

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

Result<std::optional<Sphere>> make(int q, const std::array<Point, 4>& points) {
  if (q == 1) return std::optional<Sphere>{Sphere::point(points[0])};
  if (q == 2) return Sphere::through(points[0], points[1]);
  if (q == 3) return Sphere::through(points[0], points[1], points[2]);
  if (q == 4) return Sphere::through(points[0], points[1], points[2], points[3]);
  return fail(Reason::parameter_out_of_range);
}

Outcome write_sphere(const Sphere& s, const std::array<Point, 5>& points, const Level& previous) {
  const auto power_value = power(s, points[4]);
  const auto side_value = side(s, points[4]);
  const auto center_orientation = orientation(points[0], points[1], points[2], s);
  const auto interior = strictly_inside(s, points[0], points[1], points[2], points[3]);
  if (!power_value.ok()) return power_value.outcome();
  if (!side_value.ok()) return side_value.outcome();
  if (!center_orientation.ok()) return center_orientation.outcome();
  if (!interior.ok()) return interior.outcome();
  std::cout << "ok";
  for (const auto v : s.numerator()) std::cout << ' ' << hex(v);
  std::cout << ' ' << hex(s.denominator()) << ' ' << hex(s.level().numerator()) << ' '
            << hex(s.level().denominator()) << ' ' << hex(power_value.value()) << ' '
            << hex(orientation(points[0], points[1], points[2], points[3])) << ' '
            << center_orientation.value() << ' ' << strictly_acute(points[0], points[1], points[2]) << ' '
            << interior.value() << ' ' << is_midpoint(s, points[0], points[1]) << ' '
            << compare(s.level(), previous) << ' ' << side_value.value() << ' '
            << hex(num_test::wide_power(s, points[4])) << ' ' << static_cast<unsigned>(s.presentation_arity());
  return {};
}

Outcome query_q4(const std::array<Point, 5>& points, Level& previous) {
  const auto made = Q4Candidate::through(points[0], points[1], points[2], points[3]);
  if (!made.ok()) return made.outcome();
  if (!made.value()) {
    const auto complete = Sphere::through(points[0], points[1], points[2], points[3]);
    if (!complete.ok()) return complete.outcome();
    if (complete.value()) return fail(Reason::arithmetic_invariant);
    std::cout << "degenerate\n";
    return {};
  }
  const auto& candidate = *made.value();
  // Ces appels sont faits et leurs resultats conserves AVANT toute materialisation ou fabrique Sphere.
  const auto anchor = candidate.anchor().coordinates();
  const auto numerator = candidate.numerator();
  const auto denominator = candidate.denominator();
  const auto arity = candidate.presentation_arity();
  const auto candidate_power = power(candidate, points[4]);
  const auto candidate_side = side(candidate, points[4]);
  const auto candidate_orientation = orientation(points[0], points[1], points[2], candidate);
  const auto candidate_inside = strictly_inside(candidate, points[0], points[1], points[2], points[3]);
  const bool candidate_midpoint = is_midpoint(candidate, points[0], points[1]);
  const auto candidate_wide = num_test::wide_power(candidate, points[4]);
  if (!candidate_power.ok()) return candidate_power.outcome();
  if (!candidate_side.ok()) return candidate_side.outcome();
  if (!candidate_orientation.ok()) return candidate_orientation.outcome();
  if (!candidate_inside.ok()) return candidate_inside.outcome();
  const auto materialized = candidate.materialize();
  if (!materialized.ok()) return materialized.outcome();
  const auto complete = Sphere::through(points[0], points[1], points[2], points[3]);
  if (!complete.ok()) return complete.outcome();
  if (!complete.value()) return fail(Reason::arithmetic_invariant);
  MHGP11_TRY(write_sphere(*complete.value(), points, previous));
  std::cout << " candidate";
  for (const auto coordinate : anchor) std::cout << ' ' << coordinate;
  for (const auto coordinate : numerator) std::cout << ' ' << hex(coordinate);
  std::cout << ' ' << hex(denominator) << ' ' << hex(candidate_power.value()) << ' ' << candidate_side.value()
            << ' ' << candidate_orientation.value() << ' ' << candidate_inside.value() << ' ' << candidate_midpoint
            << ' ' << hex(candidate_wide) << ' ' << static_cast<unsigned>(arity) << " materialized ";
  MHGP11_TRY(write_sphere(materialized.value(), points, previous));
  const auto materialized_anchor = materialized.value().anchor().coordinates();
  for (const auto coordinate : materialized_anchor) std::cout << ' ' << coordinate;
  std::cout << '\n';
  previous = complete.value()->level();
  return {};
}

Outcome query(int q, Level& previous) {
  std::array<Point, 5> points{};
  for (auto& point : points) {
    i64 x = 0, y = 0, z = 0;
    if (!(std::cin >> x >> y >> z)) return fail(Reason::input_unreadable);
    auto made = Point::make(x, y, z);
    if (!made.ok()) return made.outcome();
    point = made.value();
  }
  if (q == 4) return query_q4(points, previous);
  const std::array<Point, 4> support = {points[0], points[1], points[2], points[3]};
  const auto sphere = make(q, support);
  if (!sphere.ok()) return sphere.outcome();
  if (!sphere.value()) {
    std::cout << "degenerate\n";
    return {};
  }
  MHGP11_TRY(write_sphere(*sphere.value(), points, previous));
  std::cout << '\n';
  previous = sphere.value()->level();
  return {};
}

bool read_wide(Wide<4>& out) {
  std::string token;
  if (!(std::cin >> token)) return false;
  std::size_t start = 0;
  const bool negative = !token.empty() && token[0] == '-';
  if (negative) start = 1;
  if (token.size() == start || token.size() - start > 64) return false;
  Wide<4> value;
  for (std::size_t i = start; i < token.size(); ++i) {
    const char c = token[i];
    const int digit = c >= '0' && c <= '9' ? c - '0' : (c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1);
    if (digit < 0) return false;
    u64 carry = static_cast<u64>(digit);
    for (auto& word : value.words) {
      const u64 next = word >> 60;
      word = (word << 4) | carry;
      carry = next;
    }
  }
  value.neg = negative && !value.is_zero();
  out = value;
  return true;
}

bool integer_query() {
  Wide<4> a, b, sum = Wide<4>::from_u64(7), difference = sum;
  if (!read_wide(a) || !read_wide(b)) return false;
  const bool sum_ok = add(a, b, sum), difference_ok = subtract(a, b, difference);
  std::cout << "wide " << sum_ok << ' ' << hex(sum) << ' ' << difference_ok << ' ' << hex(difference) << ' '
            << hex(multiply(a, b)) << ' ' << compare(a, b) << '\n';
  return true;
}
}  // namespace

int main() {
  std::cout << "bits " << kCoordBits << '\n';
  Level previous;
  int q = 0;
  while (std::cin >> q) {
    if (q == 0) {
      if (!integer_query()) return 2;
    } else {
      const auto result = query(q, previous);
      if (!result.ok()) {
        std::cerr << reason_name(result.reason) << '\n';
        return exit_code(result);
      }
    }
  }
  return std::cin.eof() ? 0 : 2;
}
