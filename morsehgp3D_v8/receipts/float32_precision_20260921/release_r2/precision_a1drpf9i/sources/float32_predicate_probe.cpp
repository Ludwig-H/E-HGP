#include "core/float32_predicates.hpp"

#include <array>
#include <charconv>
#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using Point = mhgp8::Float32Point3;
using Work = mhgp8::Float32PredicateWork;
using Mode = mhgp8::Float32PredicateMode;
using Bits = std::array<std::uint32_t, 3>;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

void dump(const Work& work) {
  std::cout << '{';
#define FIELD(name) std::cout << '"' << #name << "\":" << work.name
  FIELD(queries); std::cout << ',';
  FIELD(exact_queries); std::cout << ',';
  FIELD(filter_queries); std::cout << ',';
  FIELD(filter_accepts); std::cout << ',';
  FIELD(filter_fallbacks); std::cout << ',';
  FIELD(exact_terms); std::cout << ',';
  FIELD(filter_axis_products); std::cout << ',';
  FIELD(filter_interval_additions);
#undef FIELD
  std::cout << '}';
}

void verify_work(const Work& filtered, const Work& exact) {
  require(exact.queries == 1 && exact.exact_queries == 1 && exact.exact_terms == 12 &&
          exact.filter_queries == 0 && exact.filter_accepts == 0 && exact.filter_fallbacks == 0 &&
          exact.filter_axis_products == 0 && exact.filter_interval_additions == 0,
          "exact-only work mismatch");
  require(filtered.queries == 1 && filtered.filter_queries == 1 &&
          filtered.filter_accepts + filtered.filter_fallbacks == 1 &&
          filtered.exact_queries == filtered.filter_fallbacks &&
          filtered.exact_terms == 12 * filtered.exact_queries &&
          filtered.filter_axis_products == 3 && filtered.filter_interval_additions == 3,
          "filtered work mismatch");
}

void selftest() {
  std::uint64_t tests = 0, accepted_filters = 0, fallbacks = 0;
  const auto check = [&](Bits a, Bits b, Bits z, int expected) {
    const auto ap = Point::from_bits(a), bp = Point::from_bits(b), zp = Point::from_bits(z);
    Work filtered{}, exact{};
    const auto first = mhgp8::q2_power_sign(ap, bp, zp, Mode::Filtered, filtered);
    const auto second = mhgp8::q2_power_sign(ap, bp, zp, Mode::ExactOnly, exact);
    require(first == expected && second == expected, "selftest exact sign mismatch");
    verify_work(filtered, exact);
    accepted_filters += filtered.filter_accepts;
    fallbacks += filtered.filter_fallbacks;
    ++tests;
  };
  constexpr std::uint32_t one = 0x3f800000U, two = 0x40000000U;
  constexpr std::uint32_t half = 0x3f000000U, maximum = 0x7f7fffffU;
  check({0,0,0}, {one,0,0}, {half,0,0}, -1);
  check({0,0,0}, {one,0,0}, {two,0,0}, 1);
  check({0,0,0}, {one,0,0}, {0,0,0}, 0);
  check({0,0,0}, {one,0,0}, {one,0,0}, 0);
  check({0x80000000U,0,0x80000000U}, {one,0,0}, {0,0x80000000U,0}, 0);
  check({one,0,0}, {0,one,0}, {one,one,0}, 0);
  check({0,0,0}, {2,0,0}, {1,0,0}, -1);  // Binary32 subnormals.
  check({0,0,0}, {1,0,0}, {2,0,0}, 1);
  check({0x80000001U,0,0}, {1,0,0}, {0,0,0}, -1);
  check({maximum,0,0}, {maximum | 0x80000000U,0,0}, {0,0,0}, -1);
  check({maximum,0,0}, {maximum | 0x80000000U,0,0}, {0,maximum,0}, 0);
  check({maximum,0,0}, {maximum | 0x80000000U,0,0}, {0,maximum - 1,0}, -1);
  check({maximum,maximum,maximum}, {maximum,maximum,maximum},
        {maximum,maximum,maximum}, 0);
  // Complete cancellation of huge terms leaves one squared subnormal.
  check({maximum,0,0}, {maximum | 0x80000000U,0,0}, {0,maximum,1}, 1);
  const int initial_rounding = std::fegetround();
  require(initial_rounding != -1, "cannot read floating rounding mode");
  try {
    for (const auto rounding : std::array<int, 4>{
             FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
      require(std::fesetround(rounding) == 0 && std::fegetround() == rounding,
              "cannot install floating rounding mode");
      check({0,0,0}, {one,0,0}, {half,0,0}, -1);
      check({0,0,0}, {one,0,0}, {two,0,0}, 1);
      check({maximum,0,0}, {maximum | 0x80000000U,0,0}, {0,maximum,0}, 0);
      check({maximum,0,0}, {maximum | 0x80000000U,0,0}, {0,maximum,1}, 1);
    }
  } catch (...) {
    static_cast<void>(std::fesetround(initial_rounding));
    throw;
  }
  require(std::fesetround(initial_rounding) == 0 && std::fegetround() == initial_rounding,
          "cannot restore floating rounding mode");
  for (const auto invalid : std::array<std::uint32_t, 6>{
           0x7f800000U, 0xff800000U, 0x7fc00000U, 0xffc00000U, 0x7f800001U, 0xff800001U}) {
    for (std::size_t axis = 0; axis != 3; ++axis) {
      Bits bits{}; bits[axis] = invalid;
      bool rejected = false;
      try { static_cast<void>(Point::from_bits(bits)); }
      catch (const std::invalid_argument&) { rejected = true; }
      require(rejected, "nonfinite coordinate was accepted");
      ++tests;
    }
  }
  Work work{};
  const auto origin = Point::from_bits({0,0,0});
  bool rejected = false;
  try { static_cast<void>(mhgp8::q2_power_sign(origin, origin, origin, static_cast<Mode>(91), work)); }
  catch (const std::invalid_argument&) { rejected = true; }
  require(rejected && work == Work{}, "invalid mode mutated work or was accepted");
  ++tests;
  require(accepted_filters != 0 && fallbacks != 0, "selftest has no filter or fallback coverage");
  std::cout << "{\"status\":\"passed\",\"tests\":" << tests
            << ",\"filter_accepts\":" << accepted_filters
            << ",\"filter_fallbacks\":" << fallbacks << ",\"rounding_modes\":4}\n";
}

std::uint32_t hexadecimal(const std::string& token) {
  std::uint32_t result{};
  const auto converted = std::from_chars(token.data(), token.data() + token.size(), result, 16);
  require(token.size() == 8 && converted.ec == std::errc{} &&
          converted.ptr == token.data() + token.size(), "each coordinate requires exactly eight hexadecimal digits");
  return result;
}
}  // namespace

int main(int argc, char** argv) {
  const bool testing = argc == 2 && std::string_view(argv[1]) == "--selftest";
  try {
    if (testing) { selftest(); return 0; }
    require(argc == 1, "usage: probe [--selftest]; stdin has nine hex32 words per line");
    std::string line;
    bool received = false;
    while (std::getline(std::cin, line)) {
      std::istringstream input(line);
      std::array<Bits, 3> points{};
      std::string token;
      for (auto& point : points) for (auto& coordinate : point) {
        require(static_cast<bool>(input >> token), "input line has fewer than nine hex32 words");
        coordinate = hexadecimal(token);
      }
      require(!(input >> token), "input line has more than nine hex32 words");
      const auto a = Point::from_bits(points[0]), b = Point::from_bits(points[1]), z = Point::from_bits(points[2]);
      Work filtered{}, exact{};
      const auto first = mhgp8::q2_power_sign(a, b, z, Mode::Filtered, filtered);
      const auto second = mhgp8::q2_power_sign(a, b, z, Mode::ExactOnly, exact);
      verify_work(filtered, exact);
      require(first == second, "filtered and exact signs differ");
      std::cout << "{\"filtered\":" << first << ",\"exact\":" << second << ",\"filtered_work\":";
      dump(filtered); std::cout << ",\"exact_work\":"; dump(exact); std::cout << "}\n";
      received = true;
    }
    require(!std::cin.bad() && std::cin.eof(), "input stream failed");
    require(received, "input requires at least one case");
    require(static_cast<bool>(std::cout), "output stream failed");
    return 0;
  } catch (const std::exception& error) {
    if (testing) std::cout << "{\"status\":\"failed\"}\n";
    std::cerr << "float32 predicate probe: " << error.what() << '\n';
    return 1;
  }
}
