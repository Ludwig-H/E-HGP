// Bench serializer qualification only: independent unbounded Boost integers
// judge U192/u128 normalization. No geometric or catalogue authority inferred.
#include <array>
#include <cstdio>
#include <cstring>
#include <exception>
#include <limits>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "../bench/full_gabriel_semantic_digest.hpp"

using namespace mhgp7;
namespace digest = mhgp7::full_probe_digest;
using Big = boost::multiprecision::cpp_int;

namespace {
u64 checks = 0, failures = 0, divisions = 0, reductions = 0, rejections = 0;
void check(bool ok, const char* why) {
  ++checks;
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL digest %s\n", why); }
}
Big integer(u128 value) {
  return (Big(static_cast<u64>(value >> 64)) << 64) + static_cast<u64>(value);
}
Big integer(const std::array<u64, 3>& value) {
  return (Big(value[2]) << 128) + (Big(value[1]) << 64) + value[0];
}
Big gcd(Big a, Big b) {
  while (b != 0) { Big remainder = a % b; a = b; b = remainder; }
  return a;
}
void judge(const std::array<u64, 3>& n, u128 d) {
  const Big numerator = integer(n), denominator = integer(d);
  u128 remainder = 0;
  const auto quotient = digest::divmod(n, d, remainder);
  ++divisions;
  check(integer(quotient) == numerator / denominator, "independent quotient");
  check(integer(remainder) == numerator % denominator, "independent remainder");
  const ExactLevel source{{n[0], n[1], n[2]}, static_cast<i128>(d)};
  const auto actual = digest::normalized(source);
  const Big common = gcd(numerator, denominator);
  ++reductions;
  // Compare the normalized bytes, NOT ExactLevel's rational operator==.
  check(integer(std::array<u64, 3>{actual.num[0], actual.num[1], actual.num[2]}) == numerator / common,
        "canonical numerator limbs");
  check(actual.den > 0 && integer(static_cast<u128>(actual.den)) == denominator / common,
        "canonical positive denominator");
}
template<class F> void refused(F&& invoke, const char* reason) {
  bool caught = false;
  try { invoke(); }
  catch (const std::invalid_argument& error) { caught = std::strcmp(error.what(), reason) == 0; }
  check(caught, reason);
  if (caught) ++rejections;
}
void test() {
  const u64 maximum = std::numeric_limits<u64>::max();
  const std::array<std::array<u64, 3>, 16> numerators{{
      {0,0,0}, {1,0,0}, {2,0,0}, {3,0,0},
      {maximum,0,0}, {0,1,0}, {1,1,0}, {maximum,maximum,0},
      {0,0,1}, {1,0,1}, {maximum,maximum,1}, {maximum,maximum,maximum},
      {0,0,u64{1}<<63}, {2,0,2}, {0,u64{1}<<63,1}, {17,maximum-3,42}}};
  const std::array<u128, 10> denominators{{
      1, 2, 3, 7, 29, u128{1}<<63, u128{1}<<64, u128{1}<<126,
      (u128{1}<<126)+3, static_cast<u128>(std::numeric_limits<i128>::max())}};
  for (const auto& n : numerators) for (u128 d : denominators) judge(n, d);
  u64 state = 0x139475b8b4ae082full;
  const auto next = [&]() {
    state ^= state << 13; state ^= state >> 7; state ^= state << 17;
    return state;
  };
  for (unsigned i = 0; i < 512; ++i) {
    const std::array<u64, 3> n{next(), next(), next()};
    const u64 high = next() >> 1, low = next();
    const u128 d = (static_cast<u128>(high) << 64) | low;
    judge(n, d == 0 ? 1 : d);
  }
  for (i128 d : {i128{0}, i128{-1}})
    refused([&] { (void)digest::normalized({{1,0,0}, d}); }, "digest_level_denominator");
  for (u128 d : {u128{0}, u128{1}<<127})
    refused([&] { u128 r = 0; (void)digest::divmod({1,0,0}, d, r); }, "digest_divisor");
  refused([] { (void)digest::input(std::vector<InputPoint>{{7,{0,0,0}}, {7,{1,1,1}}}); },
          "digest_input_domain");
  refused([] { (void)digest::input(std::vector<InputPoint>{{7,{-1,0,0}}}); },
          "digest_input_domain");
  const auto semantic = digest::selftest();
  check(semantic.checks >= 24 && semantic.failures == 0, "semantic topology, labels, levels, permutations and binding");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::strcmp(argv[1], "--selftest") != 0) return 2;
  try { test(); }
  catch (const std::exception& error) {
    ++failures; std::fprintf(stderr, "FAIL digest exception: %s\n", error.what());
  }
  const bool floor = divisions == 672 && reductions == 672 && rejections == 6 && checks >= 2695;
  std::printf("full_gabriel_digest authority=bench_serializer_only oracle=Boost_cpp_int "
              "divisions=%llu reductions=%llu rejections=%llu checks=%llu failures=%llu floor=%u\n",
              (unsigned long long)divisions, (unsigned long long)reductions,
              (unsigned long long)rejections, (unsigned long long)checks,
              (unsigned long long)failures, floor ? 1u : 0u);
  return failures != 0 ? 1 : floor ? 0 : 3;
}
