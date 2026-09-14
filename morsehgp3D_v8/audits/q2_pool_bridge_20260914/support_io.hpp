// Explicit unchanged utility excerpt from morsehgp3D_v8/audits/q2_order_lidar_20260914/lidar_order_probe.cpp.
// Whole source SHA256 a5607a70b1597b758d552d1e4169fcc5c6774b98508da63eaa9dff49bcf0b39c.
#pragma once
// Explicit adaptation of q2_front_20260914/lidar_q2_probe.cpp, SHA256
// 217b2bca2956e75df52a4ad1877959a5bd3e2078384b4c06faabf7c0123e8f99.
// Adds the published sibling/order options and their counters; digest unchanged.
// Explicit audit adaptation of lidar08_20260914/front_input_probe.cpp:
// SHA256 c4aea05be88d2a07c775a926fdb7f525b3592d6968bcd5f0d54328bc3a553bcb.
// Canonical support encoding and callback checks adapted from
// morsehgp3D_v8/bench/wspd_q2_census_probe.cpp, SHA256
// 04523b566cfa5395f0ac2d23e9925c4ff5bcf18882cd1c5e0f2f0d5a1bae6e0f.
// Checksums compare streams without storing them; they are not an oracle.
#include "pipeline/wspd_q2_census.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace mhgp8;
using Clock = std::chrono::steady_clock;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
unsigned number(std::string_view text) {
  unsigned result{};
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
  require(!text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size(),
          "invalid unsigned parameter");
  return result;
}
u64 product(u64 a, u64 b) {
  require(b == 0 || a <= std::numeric_limits<u64>::max() / b, "counter product overflow");
  return a * b;
}
double ms(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}
void hash_word(u64& hash, u64 value) {
  for (unsigned byte = 0; byte < 8; ++byte) {
    hash ^= value & 255U; hash *= 1099511628211ULL; value >>= 8U;
  }
}
struct CallbackWork {
  u64 copied_ids{}, sort_calls{}, sort_comparisons{}, validation_ids{};
  u64 adjacent_tests{}, cross_set_comparisons{}, support_key_axis_checks{}, hash_words{};
};
struct OutputDigest {
  u64 supports{}, interior_ids{}, shell_ids{}, sum{}, xor_value{};
  CallbackWork work;
  std::vector<std::size_t> interior, shell;
  void consume(const Q2Support& support, std::span<const Point3> points, unsigned kmax) {
    require(support.a_id < points.size() && support.b_id < points.size() &&
            support.a_id != support.b_id && support.interior.size() < kmax &&
            support.shell.size() >= 2, "invalid materialized q2 support");
    const auto& a = points[support.a_id];
    const auto& b = points[support.b_id];
    u64 diameter = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      counter_add(work.support_key_axis_checks);
      require(support.key.center_twice[axis] == static_cast<unsigned>(a[axis]) + b[axis],
              "support midpoint disagrees with endpoints");
      const i64 delta = static_cast<i64>(a[axis]) - b[axis];
      counter_add(diameter, static_cast<u64>(delta * delta));
    }
    require(diameter == support.key.diameter_squared, "support diameter disagrees with endpoints");
    interior.assign(support.interior.begin(), support.interior.end());
    shell.assign(support.shell.begin(), support.shell.end());
    counter_add(work.copied_ids, interior.size()); counter_add(work.copied_ids, shell.size());
    bool saw_a = false, saw_b = false;
    for (unsigned part = 0; part < 2; ++part) {
      auto& ids = part == 0 ? interior : shell;
      counter_add(work.sort_calls);
      std::sort(ids.begin(), ids.end(), [&](std::size_t left, std::size_t right) {
        counter_add(work.sort_comparisons); return left < right;
      });
      for (std::size_t i = 0; i < ids.size(); ++i) {
        counter_add(work.validation_ids);
        require(ids[i] < points.size(), "materialized ID escaped the cloud");
        require(part != 0 || (ids[i] != support.a_id && ids[i] != support.b_id),
                "support endpoint emitted as strict interior");
        if (i != 0) {
          counter_add(work.adjacent_tests);
          require(ids[i - 1] != ids[i], "duplicate ID inside a support");
        }
        if (part != 0) { saw_a = saw_a || ids[i] == support.a_id; saw_b = saw_b || ids[i] == support.b_id; }
      }
    }
    require(saw_a && saw_b, "shell lost an endpoint");
    std::size_t i = 0, j = 0;
    while (i < interior.size() && j < shell.size()) {
      counter_add(work.cross_set_comparisons);
      require(interior[i] != shell[j], "interior and shell overlap");
      if (interior[i] < shell[j]) ++i; else ++j;
    }
    u64 hash = 14695981039346656037ULL;
    const auto word = [&](u64 value) { counter_add(work.hash_words); hash_word(hash, value); };
    word(2); word(std::min(support.a_id, support.b_id)); word(std::max(support.a_id, support.b_id));
    for (const auto coordinate : support.key.center_twice) word(coordinate);
    word(support.key.diameter_squared);
    word(interior.size()); for (const auto id : interior) word(id);
    word(shell.size()); for (const auto id : shell) word(id);
    counter_add(supports); counter_add(interior_ids, interior.size()); counter_add(shell_ids, shell.size());
    sum += hash; xor_value ^= hash;  // Deliberate modulo arithmetic only for checksums.
  }
};
struct Fields {
  bool first = true;
  void add(const char* name, u64 value) {
    if (!first) std::cout << ',';
    first = false; std::cout << '"' << name << "\":" << value;
  }
};
template <std::size_t N> void array(const char* name, const std::array<u64, N>& values) {
  std::cout << ",\"" << name << "\":[";
  for (std::size_t i = 0; i < N; ++i) { if (i != 0) std::cout << ','; std::cout << values[i]; }
  std::cout << ']';
}

}  // namespace
