#include "core/float32_ball_key.hpp"

#include <array>
#include <charconv>
#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace {
using Point = mhgp8::Float32Point3;
using Ball = mhgp8::Float32Ball;
using Key = mhgp8::Float32BallKey;
using Work = mhgp8::Float32KeyWork;
using Mode = mhgp8::Float32PredicateMode;
using Words = std::array<std::uint32_t, 3>;
using Support = std::array<Words, 4>;

static_assert(std::is_copy_constructible_v<Key> && std::is_move_constructible_v<Key>);
static_assert(!std::is_copy_assignable_v<Key> && !std::is_move_assignable_v<Key>);
static_assert(std::is_same_v<decltype(std::declval<const Key&>().serialized_words()), std::span<const std::uint32_t>>);

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

std::optional<Key> make(std::size_t arity, const Support& words, Work& work) {
  const auto a = Point::from_bits(words[0]), b = Point::from_bits(words[1]);
  if (arity == 2) return Key::make_q2(a, b, work);
  const auto c = Point::from_bits(words[2]);
  if (arity == 3) return Key::make_q3({a, b, c}, work);
  require(arity == 4, "key requires arity two, three or four");
  return Key::make_q4({a, b, c, Point::from_bits(words[3])}, work);
}

std::optional<Ball> prepare(std::size_t arity, const Support& words, mhgp8::Float32BallWork& work) {
  const auto a = Point::from_bits(words[0]), b = Point::from_bits(words[1]), c = Point::from_bits(words[2]);
  if (arity == 3) return Ball::make_q3({a, b, c}, Mode::ExactOnly, work);
  require(arity == 4, "positive support requires arity three or four");
  return Ball::make_q4({a, b, c, Point::from_bits(words[3])}, Mode::ExactOnly, work);
}

void dump(const mhgp8::Float32BallWork& work) {
  std::cout << '{';
#define FIELD(name) std::cout << '"' << #name << "\":" << work.name
  FIELD(q3_preparations); std::cout << ','; FIELD(q4_preparations); std::cout << ',';
  FIELD(preparation_filter_attempts); std::cout << ','; FIELD(preparation_filter_accepts); std::cout << ',';
  FIELD(preparation_exact_fallbacks); std::cout << ','; FIELD(preparation_exact_evaluations); std::cout << ',';
  FIELD(accepted_supports); std::cout << ','; FIELD(rejected_supports); std::cout << ',';
  FIELD(power_queries); std::cout << ','; FIELD(power_filter_attempts); std::cout << ',';
  FIELD(power_filter_accepts); std::cout << ','; FIELD(power_exact_fallbacks); std::cout << ',';
  FIELD(power_exact_evaluations); std::cout << ','; FIELD(interval_additions); std::cout << ',';
  FIELD(interval_products); std::cout << ','; FIELD(exact_additions); std::cout << ',';
  FIELD(exact_products); std::cout << ','; FIELD(exact_point_decodes);
#undef FIELD
  std::cout << '}';
}

void dump(const Work& work) {
  std::cout << "{\"support\":"; dump(work.support); std::cout << ',';
#define FIELD(name) std::cout << '"' << #name << "\":" << work.name
  FIELD(q2_requests); std::cout << ','; FIELD(q3_requests); std::cout << ',';
  FIELD(q4_requests); std::cout << ','; FIELD(from_support_requests); std::cout << ',';
  FIELD(rejected_supports); std::cout << ','; FIELD(keys_created); std::cout << ',';
  FIELD(canonical_gcd_calls); std::cout << ','; FIELD(canonical_divisions); std::cout << ',';
  FIELD(packed_words);
#undef FIELD
  std::cout << '}';
}

void dump_words(std::span<const std::uint32_t> words) {
  std::cout << '[';
  for (std::size_t i = 0; i != words.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << words[i];
  }
  std::cout << ']';
}

std::array<std::string, 5> coefficients(std::span<const std::uint32_t> packed) {
  require(!packed.empty() && packed[0] == 1, "unsupported key word encoding");
  std::size_t cursor = 1;
  std::array<std::string, 5> result;
  for (auto& text : result) {
    require(cursor < packed.size(), "missing coefficient header");
    const auto header = packed[cursor++];
    if (header == 0) { text = "0"; continue; }
    const auto count = static_cast<std::size_t>(header / 2);
    require(count != 0 && cursor < packed.size(), "invalid coefficient header");
    const auto trailing = packed[cursor++];
    require(count <= packed.size() - cursor && (packed[cursor] & 1U) != 0 && packed[cursor + count - 1] != 0,
            "noncanonical odd coefficient magnitude");
    const auto offset = static_cast<std::size_t>(trailing / 32U);
    const auto shift = trailing % 32U;
    std::vector<std::uint32_t> limbs(count + offset + 1, 0);
    for (std::size_t i = 0; i != count; ++i) {
      const auto word = packed[cursor++];
      limbs[offset + i] |= word << shift;
      if (shift != 0) limbs[offset + i + 1] |= word >> (32U - shift);
    }
    while (limbs.size() > 1 && limbs.back() == 0) limbs.pop_back();
    std::ostringstream output;
    if ((header & 1U) != 0) output << '-';
    output << std::hex << limbs.back();
    for (std::size_t i = limbs.size() - 1; i != 0; --i)
      output << std::setfill('0') << std::setw(8) << limbs[i - 1];
    text = output.str();
  }
  require(cursor == packed.size(), "trailing coefficient words");
  return result;
}

void selftest() {
  constexpr std::uint32_t five = 0x40a00000U, minus_five = 0xc0a00000U;
  constexpr std::uint32_t three = 0x40400000U, minus_three = 0xc0400000U;
  constexpr std::uint32_t four = 0x40800000U, minus_four = 0xc0800000U;
  const Support diameter{{{minus_five, 0, 0}, {five, 0, 0}, {0, 0, 0}, {0, 0, 0}}};
  const Support triangle{{{five, 0, 0}, {minus_three, four, 0}, {minus_three, minus_four, 0}, {0, 0, 0}}};
  const Support tetra{{{three, 0, four}, {three, 0, minus_four}, {minus_three, four, 0}, {minus_three, minus_four, 0}}};
  std::uint64_t tests = 0;
  const auto check = [&](bool condition, const char* message) { ++tests; require(condition, message); };
  Work a{}, b{}, c{};
  const auto k2 = make(2, diameter, a), k3 = make(3, triangle, b), k4 = make(4, tetra, c);
  check(k2 && k3 && k4, "common-ball supports must be valid");
  check(*k2 == *k3 && *k3 == *k4, "cross-arity canonical equality failed");
  const Key copied = *k2;
  Key move_source = copied;
  const Key moved = std::move(move_source);
  check(copied == *k2 && moved == *k2 && move_source == *k2, "copy/move published an empty or changed key");
  auto aliased = diameter;
  Work alias_work{};
  const auto isolated = make(2, aliased, alias_work);
  aliased[0] = {0, 0, 0}; aliased[1] = {0x3f800000U, 0, 0};
  check(isolated && *isolated == *k2, "mutable caller words changed a canonical key");
  Work other_work{}, radius_work{};
  const auto other = make(2, Support{{{0xc0800000U, 0, 0}, {0x40c00000U, 0, 0}, {}, {}}}, other_work);
  const auto radius = make(2, Support{{{minus_four, 0, 0}, {four, 0, 0}, {}, {}}}, radius_work);
  check(other && !(*other == *k2), "radius-only equality confused different centres");
  check(radius && !(*radius == *k2), "same-centre equality confused different radii");
  for (const std::size_t arity : {std::size_t{3}, std::size_t{4}}) {
    mhgp8::Float32BallWork preparation{};
    const auto support = prepare(arity, arity == 3 ? triangle : tetra, preparation);
    require(support.has_value(), "emission fixture is invalid");
    Work emission{};
    const auto key = Key::from_support(*support, emission);
    check(key == *k2 && emission.from_support_requests == 1 && emission.keys_created == 1 &&
          emission.support.q3_preparations == 0 && emission.support.q4_preparations == 0 &&
          emission.support.power_queries == 0, "emission repeated support validity or changed ball");
  }
  Work zeros{};
  const auto duplicate = Key::make_q2(Point::from_bits({0, 0, 0}), Point::from_bits({0x80000000U, 0, 0}), zeros);
  check(!duplicate && zeros.rejected_supports == 1 && zeros.keys_created == 0, "duplicate +/-zero diameter accepted");
  auto signed_triangle = triangle;
  for (auto& p : signed_triangle) for (auto& w : p) if (w == 0) w = 0x80000000U;
  Work signed_work{};
  const auto signed_key = make(3, signed_triangle, signed_work);
  check(signed_key && *signed_key == *k2, "signed zero changed key identity");

  const auto initial_rounding = std::fegetround();
  check(initial_rounding != -1, "cannot read rounding mode");
#if defined(__SSE__)
  const auto initial_mxcsr = _mm_getcsr();
  constexpr unsigned flush_modes = 4;
#else
  constexpr unsigned flush_modes = 1;
#endif
  try {
    for (const auto rounding : std::array<int, 4>{FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
      check(std::fesetround(rounding) == 0, "cannot set rounding mode");
      for (unsigned flush = 0; flush != flush_modes; ++flush) {
#if defined(__SSE__)
        auto csr = _mm_getcsr() & ~static_cast<unsigned>((1U << 15) | (1U << 6));
        if ((flush & 1U) != 0) csr |= 1U << 15;
        if ((flush & 2U) != 0) csr |= 1U << 6;
        _mm_setcsr(csr);
#endif
        check(std::feclearexcept(FE_ALL_EXCEPT) == 0, "cannot clear exception flags");
        Work w2{}, w3{}, w4{};
        const auto q2 = make(2, diameter, w2), q3 = make(3, triangle, w3), q4 = make(4, tetra, w4);
        check(std::fetestexcept(FE_ALL_EXCEPT) == 0 && q2 && q3 && q4 && *q2 == *k2 && *q3 == *k2 && *q4 == *k2,
              "exact key depended on floating environment or raised floating exception");
      }
    }
  } catch (...) {
    static_cast<void>(std::fesetround(initial_rounding));
#if defined(__SSE__)
    _mm_setcsr(initial_mxcsr);
#endif
    throw;
  }
  check(std::fesetround(initial_rounding) == 0, "cannot restore rounding mode");
#if defined(__SSE__)
  _mm_setcsr(initial_mxcsr);
#endif
  constexpr std::size_t workers = 4, repetitions = 16;
  std::array<std::exception_ptr, workers> errors{};
  {
    std::vector<std::jthread> threads;
    threads.reserve(workers);
    for (std::size_t worker = 0; worker != workers; ++worker) {
      threads.emplace_back([&, worker] {
        try {
          for (std::size_t repeat = 0; repeat != repetitions; ++repeat) {
            require(*k2 == *k3 && *k3 == *k4, "concurrent equality changed");
            require(coefficients(k2->serialized_words()) == coefficients(k4->serialized_words()),
                    "concurrent serialized encoding changed");
          }
        } catch (...) { errors[worker] = std::current_exception(); }
      });
    }
  }
  for (const auto& error : errors) {
    if (error) std::rethrow_exception(error);
    check(!error, "concurrent key reader failed");
  }
  std::cout << "{\"schema\":\"mhgp8_float32_key_selftest_v1\",\"status\":\"passed\",\"tests\":" << tests
            << ",\"rounding_modes\":4,\"flush_modes\":" << flush_modes
            << ",\"concurrent_workers\":" << workers << ",\"concurrent_iterations\":" << workers * repetitions
            << ",\"key_object_bytes\":" << sizeof(Key) << "}\n";
}

std::uint32_t hexadecimal(const std::string& token) {
  std::uint32_t value{};
  const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value, 16);
  require(token.size() == 8 && parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size(), "requires eight hexadecimal digits");
  return value;
}

void probe() {
  std::string line;
  bool received = false;
  while (std::getline(std::cin, line)) {
    std::istringstream input(line);
    std::string token;
    require(static_cast<bool>(input >> token) && (token == "2" || token == "3" || token == "4"), "requires arity two, three or four");
    const std::size_t arity = token == "2" ? 2 : token == "3" ? 3 : 4;
    Support words{};
    for (std::size_t i = 0; i != arity; ++i) for (auto& word : words[i]) {
      require(static_cast<bool>(input >> token), "missing coordinate word");
      word = hexadecimal(token);
    }
    require(!(input >> token), "extra coordinate word");
    Work work{}, emitted_work{};
    const auto key = make(arity, words, work);
    const auto emitted = [&]() -> std::optional<Key> {
      if (!key || arity == 2) return std::nullopt;
      mhgp8::Float32BallWork validation{};
      const auto support = prepare(arity, words, validation);
      require(support.has_value(), "key validity disagrees with prepared support");
      return Key::from_support(*support, emitted_work);
    }();
    std::cout << "{\"arity\":" << arity << ",\"valid\":" << (key ? "true" : "false") << ",\"words\":";
    if (key) dump_words(key->serialized_words()); else std::cout << "null";
    std::cout << ",\"coefficients\":";
    if (key) {
      const auto values = coefficients(key->serialized_words());
      std::cout << '[';
      for (std::size_t i = 0; i != values.size(); ++i) {
        if (i != 0) std::cout << ',';
        std::cout << '"' << values[i] << '"';
      }
      std::cout << ']';
    } else std::cout << "null";
    std::cout << ",\"work\":"; dump(work);
    std::cout << ",\"from_support_words\":";
    if (emitted) dump_words(emitted->serialized_words()); else std::cout << "null";
    std::cout << ",\"from_support_work\":";
    if (emitted) dump(emitted_work); else std::cout << "null";
    std::cout << "}\n";
    received = true;
  }
  require(received && std::cin.eof() && !std::cin.bad(), "empty input or stream failure");
}
}  // namespace

int main(int argc, char** argv) {
  const bool testing = argc == 2 && std::string_view(argv[1]) == "--selftest";
  try {
    require(argc == 1 || testing, "usage: key probe [--selftest]; stdin: arity then support hex triples");
    if (testing) selftest(); else probe();
    require(static_cast<bool>(std::cout), "output stream failed");
    return 0;
  } catch (const std::exception& error) {
    if (testing) std::cout << "{\"status\":\"failed\"}\n";
    std::cerr << "float32 key probe: " << error.what() << '\n';
    return 1;
  }
}
