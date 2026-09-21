#include "core/float32_q4_events.hpp"

#include <array>
#include <charconv>
#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
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
using Events = mhgp8::Float32Q4Events;
using Point = mhgp8::Float32Point3;
using Work = mhgp8::Float32Q4EventsWork;
using Mode = mhgp8::Float32PredicateMode;
using Words = std::array<std::uint32_t, 3>;
using Input = std::array<Words, 5>;
static_assert(std::is_copy_constructible_v<Events> && std::is_move_constructible_v<Events>);
static_assert(!std::is_copy_assignable_v<Events> && !std::is_move_assignable_v<Events>);

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

std::array<Point, 3> seed(const Input& input) {
  return {Point::from_bits(input[0]), Point::from_bits(input[1]), Point::from_bits(input[2])};
}

void dump(const mhgp8::Float32BallWork& work) {
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

void dump(const Work& work) {
  std::cout << "{\"seed\":"; dump(work.seed); std::cout << ',';
#define FIELD(name) std::cout << '"' << #name << "\":" << work.name
  FIELD(preparations); std::cout << ',';
  FIELD(accepted_seeds); std::cout << ',';
  FIELD(rejected_seeds); std::cout << ',';
  FIELD(interval_preparations); std::cout << ',';
  FIELD(side_queries); std::cout << ',';
  FIELD(side_filter_attempts); std::cout << ',';
  FIELD(side_filter_accepts); std::cout << ',';
  FIELD(side_exact_fallbacks); std::cout << ',';
  FIELD(side_exact_evaluations); std::cout << ',';
  FIELD(root_queries); std::cout << ',';
  FIELD(root_coplanar_rejections); std::cout << ',';
  FIELD(root_filter_attempts); std::cout << ',';
  FIELD(root_filter_accepts); std::cout << ',';
  FIELD(root_exact_fallbacks); std::cout << ',';
  FIELD(root_exact_evaluations); std::cout << ',';
  FIELD(root_equalities); std::cout << ',';
  FIELD(interval_additions); std::cout << ',';
  FIELD(interval_products); std::cout << ',';
  FIELD(exact_additions); std::cout << ',';
  FIELD(exact_products); std::cout << ',';
  FIELD(exact_point_decodes);
#undef FIELD
  std::cout << '}';
}

void selftest() {
  constexpr std::uint32_t one = 0x3f800000U, neg_one = 0xbf800000U;
  constexpr std::uint32_t two = 0x40000000U, neg_two = 0xc0000000U;
  constexpr std::uint32_t maximum = 0x7f7fffffU, neg_maximum = 0xff7fffffU;
  const Input base{{{neg_one, 0, 0}, {one, 0, 0}, {0, two, 0}, {0, 0, two}, {0, 0, one}}};
  const Input thin{{{neg_one, 0, 0}, {one, 0, 0}, {0, one, 1}, {0, 0, 1}, {0, 0, 0x80000001U}}};
  const Input extreme{{{neg_maximum, 0, 0}, {maximum, 0, 0}, {0, maximum, maximum},
                        {0, maximum, 0}, {0, 0, maximum}}};
  std::uint64_t tests = 0, queries = 0, coplanar_rejections = 0, equality_checks = 0;
  const auto check = [&](bool value, const char* message) { ++tests; require(value, message); };
  const auto exercise = [&](const Input& input, int side1, int side2, int order) {
    for (const auto mode : {Mode::Filtered, Mode::ExactOnly}) {
      Work preparation{}, side_work{}, comparison{};
      const auto events = Events::make(seed(input), mode, preparation);
      check(events.has_value() && events->mode() == mode, "acute seed was rejected or changed mode");
      const auto a = Point::from_bits(input[3]), b = Point::from_bits(input[4]);
      check(events->side(a, side_work) == side1 && events->side(b, side_work) == side2,
            "event side differs from known orientation");
      check(events->compare_roots(a, b, comparison) == order, "root order differs from known rational order");
      check(events->compare_roots(b, a, comparison) == -order, "root comparator is not antisymmetric");
      check(events->compare_roots(a, a, comparison) == 0, "identical root is not equal");
      check(comparison.root_equalities >= 1, "root equality did not use exact arithmetic");
      queries += 5;
      ++equality_checks;
    }
  };
  exercise(base, 1, 1, 1);
  auto reversed = base; std::swap(reversed[0], reversed[1]);
  exercise(reversed, -1, -1, -1);
  auto opposite = base; opposite[3] = {0, 0, neg_two}; opposite[4] = {0, 0, two};
  exercise(opposite, -1, 1, -1);
  auto equal = base; equal[3] = {0, 0, one}; equal[4] = {0, 0, neg_one};
  exercise(equal, 1, -1, 0);

  Work prepared_work{};
  auto original = seed(base);
  const auto shared = Events::make(original, Mode::Filtered, prepared_work);
  require(shared.has_value(), "shared event fixture missing");
  const auto replacement = Point::from_bits({two, two, two});
  std::destroy_at(&original[0]); std::construct_at(&original[0], replacement);
  const Events copy = *shared;
  Work alias_work{};
  check(copy.side(Point::from_bits(base[0]), alias_work) == 0 && copy.points()[0].bits() == base[0],
        "replacing caller point changed owned event seed");
  std::uint64_t invalid_modes = 0, rejected_seeds = 0;
  for (const auto mode : {Mode::Filtered, Mode::ExactOnly}) {
    for (const auto x : std::array<Words, 4>{{{0, one, 0}, {0, 0x3f000000U, 0}, {0, 0, 0}, base[0]}}) {
      auto bad = base; bad[2] = x;
      Work work{};
      check(!Events::make(seed(bad), mode, work), "nonacute or repeated event seed accepted");
      check(work.rejected_seeds == 1 && work.interval_preparations == 0, "invalid seed prepared event coefficients");
      ++rejected_seeds;
    }
    Work work{};
    const auto events = Events::make(seed(base), mode, work);
    for (const bool first : {false, true}) {
      Work comparison{};
      bool rejected = false;
      try {
        const auto coplanar = Point::from_bits(base[0]), other = Point::from_bits(base[3]);
        static_cast<void>(events->compare_roots(first ? coplanar : other, first ? other : coplanar, comparison));
      } catch (const std::invalid_argument&) { rejected = true; }
      check(rejected && comparison.root_coplanar_rejections == 1 && comparison.side_queries == 2 &&
            comparison.root_filter_attempts == 0 && comparison.root_exact_evaluations == 0,
            "coplanar root did not reject before determinant comparison");
      ++coplanar_rejections;
    }
  }
  Work invalid_work{};
  bool rejected = false;
  try { static_cast<void>(Events::make(seed(base), static_cast<Mode>(77), invalid_work)); }
  catch (const std::invalid_argument&) { rejected = true; }
  check(rejected && invalid_work == Work{}, "invalid mode changed work or was accepted");
  ++invalid_modes;
  Work overflow_work{}; overflow_work.root_queries = std::numeric_limits<std::uint64_t>::max();
  const auto before = overflow_work;
  bool overflow = false;
  try { static_cast<void>(shared->compare_roots(Point::from_bits(base[3]), Point::from_bits(base[4]), overflow_work)); }
  catch (const std::overflow_error&) { overflow = true; }
  check(overflow && overflow_work == before, "root query counter overflow did not fail before mutation");

  const int initial_rounding = std::fegetround();
  check(initial_rounding != -1, "cannot read floating rounding mode");
#if defined(__SSE__)
  const auto initial_mxcsr = _mm_getcsr();
  constexpr unsigned flush_modes = 4;
#else
  constexpr unsigned flush_modes = 1;
#endif
  try {
    for (const int rounding : std::array<int, 4>{FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
      check(std::fesetround(rounding) == 0, "cannot set floating rounding mode");
      for (unsigned flush = 0; flush != flush_modes; ++flush) {
#if defined(__SSE__)
        auto csr = _mm_getcsr() & ~static_cast<unsigned>((1U << 15) | (1U << 6));
        if ((flush & 1U) != 0) csr |= 1U << 15;
        if ((flush & 2U) != 0) csr |= 1U << 6;
        _mm_setcsr(csr);
#endif
        exercise(thin, 1, -1, -1);
        exercise(extreme, -1, 1, 1);
        exercise(equal, 1, -1, 0);
        check(std::feclearexcept(FE_ALL_EXCEPT) == 0, "cannot clear floating exception flags");
        Work preparation{}, query{};
        const auto events = Events::make(seed(thin), Mode::ExactOnly, preparation);
        require(events.has_value(), "exact-only thin seed missing");
        const int order = events->compare_roots(Point::from_bits(thin[3]), Point::from_bits(thin[4]), query);
        check(order == -1 && std::fetestexcept(FE_ALL_EXCEPT) == 0,
              "ExactOnly event query performed floating arithmetic or changed order");
        ++queries;
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
  constexpr std::size_t workers = 4, repetitions = 8;
  std::array<std::exception_ptr, workers> errors{};
  {
    std::vector<std::jthread> threads;
    for (std::size_t worker = 0; worker != workers; ++worker) {
      threads.emplace_back([&, worker] {
        try {
          for (std::size_t repeat = 0; repeat != repetitions; ++repeat) {
            Work work{};
            require(shared->compare_roots(Point::from_bits(base[3]), Point::from_bits(base[4]), work) == 1,
                    "shared immutable event comparator changed across workers");
          }
        } catch (...) { errors[worker] = std::current_exception(); }
      });
    }
  }
  for (const auto& error : errors) {
    if (error) std::rethrow_exception(error);
    check(!error, "concurrent event query failed");
  }
  queries += workers * repetitions;
  std::cout << "{\"schema\":\"mhgp8_float32_q4_events_selftest_v1\",\"status\":\"passed\",\"tests\":" << tests
            << ",\"queries\":" << queries << ",\"equality_checks\":" << equality_checks
            << ",\"coplanar_rejections\":" << coplanar_rejections << ",\"rejected_seeds\":" << rejected_seeds
            << ",\"invalid_modes\":" << invalid_modes << ",\"counter_overflows\":1,\"owner_replacements\":1"
            << ",\"rounding_modes\":4,\"flush_modes\":" << flush_modes
            << ",\"concurrent_workers\":" << workers << ",\"concurrent_queries\":" << workers * repetitions
            << ",\"prepared_bytes\":" << sizeof(Events) << "}\n";
}

std::uint32_t hexadecimal(const std::string& token) {
  std::uint32_t result{};
  const auto parsed = std::from_chars(token.data(), token.data() + token.size(), result, 16);
  require(token.size() == 8 && parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size(),
          "each coordinate requires eight hexadecimal digits");
  return result;
}

struct Answer {
  bool valid{}, coplanar{};
  std::array<int, 2> sides{};
  std::optional<int> order;
  Work preparation{}, side{}, comparison{};
};
Answer answer(const Input& input, Mode mode) {
  Answer result;
  const auto events = Events::make(seed(input), mode, result.preparation);
  result.valid = events.has_value();
  if (!events) return result;
  const auto z1 = Point::from_bits(input[3]), z2 = Point::from_bits(input[4]);
  result.sides = {events->side(z1, result.side), events->side(z2, result.side)};
  try { result.order = events->compare_roots(z1, z2, result.comparison); }
  catch (const std::invalid_argument&) { result.coplanar = true; }
  return result;
}

void probe() {
  std::string line;
  bool received = false;
  while (std::getline(std::cin, line)) {
    std::istringstream stream(line);
    std::string token;
    Input input{};
    for (auto& point : input) {
      for (auto& word : point) {
        require(static_cast<bool>(stream >> token), "line requires fifteen coordinate words");
        word = hexadecimal(token);
      }
      static_cast<void>(Point::from_bits(point));
    }
    require(!(stream >> token), "line has too many coordinate words");
    const auto filtered = answer(input, Mode::Filtered), exact = answer(input, Mode::ExactOnly);
    std::cout << '{';
    const auto fields = [&](const char* prefix, const Answer& value) {
      std::cout << '"' << prefix << "_valid\":" << (value.valid ? "true" : "false") << ",\"" << prefix << "_sides\":";
      if (value.valid) std::cout << '[' << value.sides[0] << ',' << value.sides[1] << ']'; else std::cout << "null";
      std::cout << ",\"" << prefix << "_order\":";
      if (value.order) std::cout << *value.order; else std::cout << "null";
      std::cout << ",\"" << prefix << "_coplanar\":" << (value.coplanar ? "true" : "false");
      std::cout << ",\"" << prefix << "_preparation_work\":"; dump(value.preparation);
      std::cout << ",\"" << prefix << "_side_work\":"; dump(value.side);
      std::cout << ",\"" << prefix << "_comparison_work\":"; dump(value.comparison);
    };
    fields("filtered", filtered); std::cout << ','; fields("exact", exact);
    std::cout << "}\n";
    received = true;
  }
  require(received && std::cin.eof() && !std::cin.bad(), "empty input or input stream failure");
}
}  // namespace

int main(int argc, char** argv) {
  const bool testing = argc == 2 && std::string_view(argv[1]) == "--selftest";
  try {
    require(argc == 1 || testing, "usage: probe [--selftest]; stdin: five hex triples per line");
    if (testing) selftest(); else probe();
    require(static_cast<bool>(std::cout), "output stream failed");
    return 0;
  } catch (const std::exception& error) {
    if (testing) std::cout << "{\"status\":\"failed\"}\n";
    std::cerr << "float32 q4 events probe: " << error.what() << '\n';
    return 1;
  }
}
