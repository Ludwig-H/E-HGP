#pragma once

#include "core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace mhgp8::bench {

// These recipes are new v8 benchmark inputs, not ports of earlier fixtures.
// SplitMix64 is fixed explicitly here; unsigned wraparound is intentional.
struct FrontGenerationWork {
  u64 rng_calls{};
  u64 proposed_points{};
  u64 duplicate_rejections{};
  u64 accepted_points{};
  u64 ordered_set_comparisons{};
};

class FrontSplitMix64 {
 public:
  FrontSplitMix64(u64 seed, FrontGenerationWork& work) : state_(seed), work_(work) {}
  u64 next() {
    counter_add(work_.rng_calls);
    auto value = (state_ += 0x9e3779b97f4a7c15ULL);
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
  }

 private:
  u64 state_;
  FrontGenerationWork& work_;
};

inline void front_hash_word(u64& hash, u64 word) {
  for (unsigned byte = 0; byte < 8; ++byte) {
    hash ^= word & 255U;
    hash *= 1099511628211ULL;
    word >>= 8U;
  }
}

struct FrontFixture {
  std::vector<Point3> points;
  FrontGenerationWork work;
  u64 input_hash{14695981039346656037ULL};
};

inline std::string_view front_recipe(std::string_view family) {
  if (family == "uniform") return "uniform_splitmix64_v1";
  if (family == "terrain") return "terrain_splitmix64_v1";
  if (family == "clusters") return "eight_corner_clusters_splitmix64_v1";
  if (family == "rows") return "parallel_rows_v1";
  throw std::invalid_argument("unknown v8 front fixture family");
}

inline void validate_front_fixture_size(std::size_t n, std::string_view family) {
  static_cast<void>(front_recipe(family));
  if (n < 2) throw std::invalid_argument("v8 front fixture requires n >= 2");
  if (family == "rows") {
    // Two distinct x columns each admit all 65536 u16 y coordinates.
    // This is the physical recipe domain, not an exploration or result cap.
    if (n % 2 != 0 || n > 131072)
      throw std::invalid_argument("parallel_rows_v1 requires even n <= 131072");
    return;
  }
  const u64 domain = family == "uniform" ? (u64{1} << 48U)
                    : family == "terrain" ? (u64{1} << 40U) : (u64{1} << 33U);
  if (n > domain) throw std::invalid_argument("fixture cardinality exceeds its finite u16 domain");
}

inline FrontFixture make_front_fixture(std::size_t n, std::string_view family, u64 seed) {
  validate_front_fixture_size(n, family);
  FrontFixture result;
  result.points.reserve(n);
  front_hash_word(result.input_hash, 1);  // Input hash encoding version.
  front_hash_word(result.input_hash, static_cast<u64>(n));
  const auto accept = [&](Point3 point) {
    result.points.push_back(point);
    counter_add(result.work.accepted_points);
    front_hash_word(result.input_hash, point.x);
    front_hash_word(result.input_hash, point.y);
    front_hash_word(result.input_hash, point.z);
  };
  if (family == "rows") {
    for (const std::uint16_t x : {std::uint16_t{1000}, std::uint16_t{60000}}) {
      for (std::size_t i = 0; i < n / 2; ++i) {
        counter_add(result.work.proposed_points);
        accept({x, static_cast<std::uint16_t>(i), 0});
      }
    }
    return result;  // This deliberately deterministic family ignores seed.
  }

  struct CountedLess {
    u64* comparisons;
    bool operator()(u64 left, u64 right) const {
      counter_add(*comparisons);
      return left < right;
    }
  };
  // The temporary uniqueness set is part of generation time, including its
  // destruction. Its allocations are not represented as a measured RSS peak.
  std::set<u64, CountedLess> seen(CountedLess{&result.work.ordered_set_comparisons});
  FrontSplitMix64 random(seed, result.work);
  while (result.points.size() < n) {  // No attempt cap or silent truncation.
    Point3 point;
    if (family == "clusters") {
      const auto corner = random.next() & 7U;
      const auto coordinate = [&](unsigned axis) {
        const u64 base = (corner & (u64{1} << axis)) != 0 ? 50000 : 20000;
        return static_cast<std::uint16_t>(base + (random.next() & 1023U));
      };
      point = {coordinate(0), coordinate(1), coordinate(2)};
    } else {
      point = {static_cast<std::uint16_t>(random.next() & 65535U),
               static_cast<std::uint16_t>(random.next() & 65535U),
               static_cast<std::uint16_t>(random.next() & (family == "terrain" ? 255U : 65535U))};
    }
    counter_add(result.work.proposed_points);
    const auto key = static_cast<u64>(point.x) | (static_cast<u64>(point.y) << 16U) |
                     (static_cast<u64>(point.z) << 32U);
    if (!seen.insert(key).second) {
      counter_add(result.work.duplicate_rejections);
      continue;
    }
    accept(point);
  }
  return result;
}

}  // namespace mhgp8::bench
