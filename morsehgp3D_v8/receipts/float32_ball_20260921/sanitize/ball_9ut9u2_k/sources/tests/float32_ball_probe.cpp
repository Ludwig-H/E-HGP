#include "core/float32_ball.hpp"

#include <array>
#include <charconv>
#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace {
using Ball = mhgp8::Float32Ball;
using Point = mhgp8::Float32Point3;
using Work = mhgp8::Float32BallWork;
using Mode = mhgp8::Float32PredicateMode;
using Words = std::array<std::uint32_t, 3>;
using Support = std::array<Words, 4>;

static_assert(std::is_copy_constructible_v<Ball> && std::is_move_constructible_v<Ball>);
static_assert(!std::is_copy_assignable_v<Ball> && !std::is_move_assignable_v<Ball>);

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

std::optional<Ball> make(std::size_t arity, const Support& input, Mode mode, Work& work) {
  const auto a = Point::from_bits(input[0]), b = Point::from_bits(input[1]), c = Point::from_bits(input[2]);
  if (arity == 3) return Ball::make_q3({a, b, c}, mode, work);
  require(arity == 4, "support requires arity three or four");
  return Ball::make_q4({a, b, c, Point::from_bits(input[3])}, mode, work);
}

void dump(const Work& work) {
  std::cout << '{';
#define FIELD(name) std::cout << '"' << #name << "\":" << work.name
  FIELD(q3_preparations); std::cout << ',';
  FIELD(q4_preparations); std::cout << ',';
  FIELD(preparation_filter_attempts); std::cout << ',';
  FIELD(preparation_filter_accepts); std::cout << ',';
  FIELD(preparation_exact_fallbacks); std::cout << ',';
  FIELD(preparation_exact_evaluations); std::cout << ',';
  FIELD(accepted_supports); std::cout << ',';
  FIELD(rejected_supports); std::cout << ',';
  FIELD(power_queries); std::cout << ',';
  FIELD(power_filter_attempts); std::cout << ',';
  FIELD(power_filter_accepts); std::cout << ',';
  FIELD(power_exact_fallbacks); std::cout << ',';
  FIELD(power_exact_evaluations); std::cout << ',';
  FIELD(interval_additions); std::cout << ',';
  FIELD(interval_products); std::cout << ',';
  FIELD(exact_additions); std::cout << ',';
  FIELD(exact_products); std::cout << ',';
  FIELD(exact_point_decodes);
#undef FIELD
  std::cout << '}';
}

void selftest() {
  constexpr std::uint32_t one = 0x3f800000U, negative_one = 0xbf800000U;
  constexpr std::uint32_t two = 0x40000000U, maximum = 0x7f7fffffU;
  const Support triangle{{{negative_one, 0, 0}, {one, 0, 0}, {0, two, 0}, {0, 0, 0}}};
  const Support tetra{{{one, one, one}, {one, negative_one, negative_one},
                       {negative_one, one, negative_one}, {negative_one, negative_one, one}}};
  const Support thin3{{{0xff7fffffU, 0, 0}, {maximum, 0, 0}, {0, maximum, 1}, {0, 0, 0}}};
  const Support thin4{{{one, one, 1}, {one, negative_one, 0x80000001U},
                       {negative_one, one, 0x80000001U}, {negative_one, negative_one, 1}}};
  const Support right{{{negative_one, 0, 0}, {one, 0, 0}, {0, one, 0}, {0, 0, 0}}};
  const Support on_facet{{{0x40a00000U, 0, 0}, {0xc0400000U, 0x40800000U, 0},
                          {0xc0400000U, 0xc0800000U, 0}, {0, 0, 0x40a00000U}}};
  std::uint64_t tests = 0, query_checks = 0, invalid_modes = 0, nonfinite_rejections = 0;
  const auto check = [&](bool condition, const char* message) {
    ++tests;
    require(condition, message);
  };
  const auto exercise = [&](std::size_t arity, const Support& support, Words z, bool valid, int sign) {
    for (const auto mode : {Mode::Filtered, Mode::ExactOnly}) {
      Work preparation{};
      const auto prepared = make(arity, support, mode, preparation);
      check(prepared.has_value() == valid, "selftest support positivity mismatch");
      if (prepared) {
        check(prepared->arity() == arity && prepared->mode() == mode, "support lost immutable arity/mode");
        Work query{};
        check(prepared->power_sign(Point::from_bits(z), query) == sign, "selftest power sign mismatch");
        ++query_checks;
      }
    }
  };
  Work preparation{};
  auto input = std::array<Point, 3>{Point::from_bits(triangle[0]), Point::from_bits(triangle[1]), Point::from_bits(triangle[2])};
  const auto stable = Ball::make_q3(input, Mode::Filtered, preparation);
  check(stable.has_value(), "alias fixture has no positive support");
  const auto replacement = Point::from_bits({two, two, two});
  // Point values are not assignable. Replacing the caller's live non-const
  // object exercises lifetime/alias isolation without modifying const bits.
  std::destroy_at(&input[0]);
  std::construct_at(&input[0], replacement);
  Work alias_work{};
  check(stable->power_sign(Point::from_bits(triangle[0]), alias_work) == 0,
        "source object replacement changed prepared support");
  const Ball copied = *stable;
  Work copy_work{};
  check(copied.power_sign(Point::from_bits(triangle[0]), copy_work) == 0, "copied support lost original point bits");
  query_checks += 2;
  for (const std::size_t arity : {std::size_t{3}, std::size_t{4}}) {
    Work unchanged{};
    bool rejected = false;
    try { static_cast<void>(make(arity, arity == 3 ? triangle : tetra, static_cast<Mode>(77), unchanged)); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected && unchanged == Work{}, "invalid mode was accepted or changed work");
    ++invalid_modes;
  }
  for (const auto invalid : std::array<std::uint32_t, 6>{
           0x7f800000U, 0xff800000U, 0x7fc00000U, 0xffc00000U, 0x7f800001U, 0xff800001U}) {
    for (std::size_t axis = 0; axis != 3; ++axis) {
      Words words{}; words[axis] = invalid;
      bool rejected = false;
      try { static_cast<void>(Point::from_bits(words)); }
      catch (const std::invalid_argument&) { rejected = true; }
      check(rejected, "nonfinite point accepted");
      ++nonfinite_rejections;
    }
  }
  auto signed_triangle = triangle;
  for (auto& point : signed_triangle) for (auto& word : point) if (word == 0) word = 0x80000000U;
  exercise(3, signed_triangle, {0, one, 0x80000000U}, true, -1);

  const auto initial_rounding = std::fegetround();
  check(initial_rounding != -1, "cannot read floating rounding mode");
#if defined(__SSE__)
  const auto initial_mxcsr = _mm_getcsr();
  constexpr unsigned flush_modes = 4;
#else
  constexpr unsigned flush_modes = 1;
#endif
  try {
    for (const auto rounding : std::array<int, 4>{FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
      check(std::fesetround(rounding) == 0, "cannot set floating rounding mode");
      for (unsigned flush = 0; flush != flush_modes; ++flush) {
#if defined(__SSE__)
        auto csr = _mm_getcsr() & ~static_cast<unsigned>((1U << 15) | (1U << 6));
        if ((flush & 1U) != 0) csr |= 1U << 15;
        if ((flush & 2U) != 0) csr |= 1U << 6;
        _mm_setcsr(csr);
#endif
        exercise(3, triangle, {0, one, 0}, true, -1);
        exercise(3, triangle, triangle[0], true, 0);
        exercise(3, triangle, {two, two, two}, true, 1);
        exercise(4, tetra, {0, 0, 0}, true, -1);
        exercise(4, tetra, tetra[0], true, 0);
        exercise(4, tetra, {two, 0, 0}, true, 1);
        exercise(3, thin3, {0, maximum, 0}, true, -1);
        exercise(3, thin3, {0, 0xff7fffffU, 0}, true, 1);
        exercise(3, thin3, thin3[2], true, 0);
        exercise(4, thin4, {one, one, 0}, true, -1);
        exercise(4, thin4, {one, one, 2}, true, 1);
        exercise(4, thin4, thin4[0], true, 0);
        exercise(3, right, {0, 0, 0}, false, 0);
        exercise(4, on_facet, {0, 0, 0}, false, 0);
        check(std::feclearexcept(FE_ALL_EXCEPT) == 0, "cannot clear floating exception flags");
        Work exact3{}, exact4{}, power3{}, power4{};
        const auto e3 = make(3, thin3, Mode::ExactOnly, exact3);
        const auto e4 = make(4, thin4, Mode::ExactOnly, exact4);
        require(e3.has_value() && e4.has_value(), "exact-only extreme support missing");
        const auto s3 = e3->power_sign(Point::from_bits({0, maximum, 0}), power3);
        const auto s4 = e4->power_sign(Point::from_bits({one, one, 2}), power4);
        check(std::fetestexcept(FE_ALL_EXCEPT) == 0 && s3 == -1 && s4 == 1,
              "exact-only support/query changed floating exception flags or sign");
        query_checks += 2;
      }
    }
  } catch (...) {
    static_cast<void>(std::fesetround(initial_rounding));
#if defined(__SSE__)
    _mm_setcsr(initial_mxcsr);
#endif
    throw;
  }
  check(std::fesetround(initial_rounding) == 0, "cannot restore floating rounding mode");
#if defined(__SSE__)
  _mm_setcsr(initial_mxcsr);
#endif
  Work p3{}, p4{};
  const auto shared3 = make(3, triangle, Mode::Filtered, p3);
  const auto shared4 = make(4, tetra, Mode::Filtered, p4);
  require(shared3 && shared4, "concurrency fixtures must be valid");
  constexpr std::size_t workers = 4, repetitions = 8;
  std::array<std::exception_ptr, workers> errors{};
  {
    std::vector<std::jthread> threads;
    threads.reserve(workers);
    for (std::size_t worker = 0; worker != workers; ++worker) {
      threads.emplace_back([&, worker] {
        try {
          for (std::size_t repeat = 0; repeat != repetitions; ++repeat) {
            Work a{}, b{}, c{}, d{};
            require(shared3->power_sign(Point::from_bits({0, one, 0}), a) == -1, "concurrent triangle interior");
            require(shared3->power_sign(Point::from_bits(triangle[0]), b) == 0, "concurrent triangle contact");
            require(shared4->power_sign(Point::from_bits({0, 0, 0}), c) == -1, "concurrent tetra interior");
            require(shared4->power_sign(Point::from_bits({two, 0, 0}), d) == 1, "concurrent tetra exterior");
          }
        } catch (...) { errors[worker] = std::current_exception(); }
      });
    }
  }
  for (const auto& error : errors) {
    if (error) std::rethrow_exception(error);
    check(!error, "concurrent readonly support query failed");
  }
  query_checks += workers * repetitions * 4;
  std::cout << "{\"schema\":\"mhgp8_float32_ball_selftest_v1\",\"status\":\"passed\",\"tests\":" << tests
            << ",\"query_checks\":" << query_checks << ",\"invalid_modes\":" << invalid_modes
            << ",\"nonfinite_rejections\":" << nonfinite_rejections
            << ",\"rounding_modes\":4,\"flush_modes\":" << flush_modes
            << ",\"concurrent_workers\":" << workers << ",\"concurrent_queries\":" << workers * repetitions * 4
            << ",\"support_bytes\":" << sizeof(Ball) << "}\n";
}

std::uint32_t hexadecimal(const std::string& token) {
  std::uint32_t result{};
  const auto converted = std::from_chars(token.data(), token.data() + token.size(), result, 16);
  require(token.size() == 8 && converted.ec == std::errc{} && converted.ptr == token.data() + token.size(),
          "each coordinate requires eight hexadecimal digits");
  return result;
}

void probe() {
  std::string line;
  bool received = false;
  while (std::getline(std::cin, line)) {
    std::istringstream input(line);
    std::string token;
    require(static_cast<bool>(input >> token) && (token == "3" || token == "4"), "line requires arity three or four");
    const std::size_t arity = token == "3" ? 3 : 4;
    std::array<Words, 5> words{};
    for (std::size_t i = 0; i != arity + 1; ++i) {
      for (auto& word : words[i]) {
        require(static_cast<bool>(input >> token), "line has too few coordinate words");
        word = hexadecimal(token);
      }
      static_cast<void>(Point::from_bits(words[i]));
    }
    require(!(input >> token), "line has too many coordinate words");
    const Support support{words[0], words[1], words[2], arity == 4 ? words[3] : Words{}};
    const auto query = Point::from_bits(words[arity]);
    Work fp{}, ep{}, fq{}, eq{};
    const auto filtered = make(arity, support, Mode::Filtered, fp);
    const auto exact = make(arity, support, Mode::ExactOnly, ep);
    const std::optional<int> fs = filtered ? std::optional<int>(filtered->power_sign(query, fq)) : std::nullopt;
    const std::optional<int> es = exact ? std::optional<int>(exact->power_sign(query, eq)) : std::nullopt;
    std::cout << "{\"arity\":" << arity << ",\"filtered_valid\":" << (filtered ? "true" : "false")
              << ",\"exact_valid\":" << (exact ? "true" : "false") << ",\"filtered_sign\":";
    if (fs) std::cout << *fs; else std::cout << "null";
    std::cout << ",\"exact_sign\":";
    if (es) std::cout << *es; else std::cout << "null";
    std::cout << ",\"filtered_preparation_work\":"; dump(fp);
    std::cout << ",\"exact_preparation_work\":"; dump(ep);
    std::cout << ",\"filtered_power_work\":"; dump(fq);
    std::cout << ",\"exact_power_work\":"; dump(eq);
    std::cout << "}\n";
    received = true;
  }
  require(received && std::cin.eof() && !std::cin.bad(), "empty input or input stream failure");
}
}  // namespace

int main(int argc, char** argv) {
  const bool testing = argc == 2 && std::string_view(argv[1]) == "--selftest";
  try {
    require(argc == 1 || testing, "usage: probe [--selftest]; stdin: arity and (arity+1) hex triples per line");
    if (testing) selftest(); else probe();
    require(static_cast<bool>(std::cout), "output stream failed");
    return 0;
  } catch (const std::exception& error) {
    if (testing) std::cout << "{\"status\":\"failed\"}\n";
    std::cerr << "float32 ball probe: " << error.what() << '\n';
    return 1;
  }
}
