#include "narrow_pair.hpp"
#include <boost/multiprecision/cpp_int.hpp>
#include <array>
#include <cstdio>
#include <limits>
#include <random>
#include <stdexcept>

using namespace mhgp9::audit_b::s2_narrow;
using Big = boost::multiprecision::cpp_int;

static void require(bool yes, const char* reason) {
  if (!yes) throw std::runtime_error(reason);
}

static Decision oracle(Bounds4 h, const CrossExtrema& c) {
  Big lo = 0, hi = 0;
  for (int j = 0; j < 3; ++j) {
    const Big a = Big(c.low[j]) * c.low[j], b = Big(c.high[j]) * c.high[j];
    if (!(c.low[j] <= 0 && c.high[j] >= 0)) lo += a < b ? a : b;
    hi += a < b ? b : a;
  }
  Decision d;
  for (unsigned j = 0; j < 2; ++j) {
    const Big alpha = j == 0 ? 3 : 2;
    const u8 bit = static_cast<u8>(2U << j);
    if (alpha * Big(h.maximum4) * h.maximum4 <= 16 * lo) d.excluded |= bit;
    else if (h.minimum4 > 0 && alpha * Big(h.minimum4) * h.minimum4 > 16 * hi) d.admitted |= bit;
  }
  return d;
}

static void equal(Decision a, Decision b) {
  require(a.excluded == b.excluded && a.admitted == b.admitted, "decision mismatch");
}

int main() try {
  std::uint64_t checked = 0, narrow = 0, wide = 0, h_rejected = 0, guard = 0;
  constexpr i64 H = i64{1} << 30, C = i64{1} << 28;
  const CrossExtrema edge{{-C,-C,-C},{C,C,C}};
  require(narrow_safe({H,H}, edge), "inclusive guard boundary"); ++guard;
  require(!narrow_safe({H,H+1}, edge), "H high rejection"); ++guard;
  require(!narrow_safe({1,0}, edge), "H zero rejection"); ++guard;
  require(!narrow_safe({3,2}, edge), "H reversed rejection"); ++guard;
  for (int axis = 0; axis < 3; ++axis) {
    auto c = edge; c.low[axis] = -C-1;
    require(!narrow_safe({1,H}, c), "cross low rejection"); ++guard;
    c = edge; c.high[axis] = C+1;
    require(!narrow_safe({1,H}, c), "cross high rejection"); ++guard;
    c = edge; c.low[axis] = std::numeric_limits<i64>::min();
    require(!narrow_safe({1,H}, c), "INT64_MIN rejection"); ++guard;
    c = edge; c.high[axis] = std::numeric_limits<i64>::max();
    require(!narrow_safe({1,H}, c), "INT64_MAX rejection"); ++guard;
    c = edge; c.low[axis] = 1; c.high[axis] = 0;
    require(!narrow_safe({1,H}, c), "cross reversed rejection"); ++guard;
  }
  // Exact equality on alpha H^2 == 16 Xi must exclude a strict witness,
  // never admit it. These are numeric-bound fixtures, not claimed geometries.
  for (const auto c : {CrossExtrema{{1,1,1},{1,1,1}}, CrossExtrema{{1,1,0},{1,1,0}}}) {
    for (i64 h : {3,4,5}) {
      const auto d = decide_as<i64>({h,h}, c);
      equal(d, oracle({h,h}, c)); ++checked;
      if (h == 4) {
        const u8 bit = c.low[2] == 1 ? 2 : 4;
        require((d.excluded & bit) != 0 && (d.admitted & bit) == 0, "strict equality changed");
      }
    }
  }
  for (const auto c : {edge, CrossExtrema{{C,C,C},{C,C,C}}, CrossExtrema{{-C,-C,-C},{-C,-C,-C}}})
    for (i64 lo : {std::numeric_limits<i64>::min(), i64{-1}, i64{0}, i64{1}, H}) {
      require(narrow_safe({lo,H}, c), "safe edge unexpectedly refused");
      equal(decide_as<i64>({lo,H}, c), oracle({lo,H}, c)); ++checked;
    }
  std::mt19937_64 random(0x9b672913ULL);
  for (unsigned t = 0; t < 200000; ++t) {
    std::int32_t a[3], b[3]; FlatBox box{};
    // Half local geometries and half full-range u18; translations near the
    // u18 ceiling exercise cancellation in cross_constant + products.
    const auto span = t % 2 == 0 ? 4095U : 262143U;
    const auto base = t % 2 == 0 ? 258048U : 0U;
    for (int j = 0; j < 3; ++j) {
      a[j] = static_cast<std::int32_t>(base + (random() & span));
      b[j] = static_cast<std::int32_t>(base + (random() & span));
      const auto x = static_cast<std::int32_t>(base + (random() & span));
      const auto y = static_cast<std::int32_t>(base + (random() & span));
      box.low[j] = x < y ? x : y; box.high[j] = x < y ? y : x;
    }
    const auto p = prepare_pair(a,b); const auto h = pair_h(p,box);
    if (h.maximum4 <= 0) { ++h_rejected; continue; }
    const auto c = cross_extrema(p,box);
    const auto x = pair_xi(p,box);
    i128 lo = 0, hi = 0;
    for (int j = 0; j < 3; ++j) {
      const i128 l = static_cast<i128>(c.low[j])*c.low[j], r = static_cast<i128>(c.high[j])*c.high[j];
      lo += c.low[j] <= 0 && c.high[j] >= 0 ? i128{0} : min_i128(l,r); hi += max_i128(l,r);
    }
    require(lo == x.low && hi == x.high, "existing pair_xi mismatch");
    const auto d = decide(p,box,h); equal(d, oracle(h,c));
    ++checked; ++(d.narrow ? narrow : wide);
  }
  require(narrow > 30000 && wide > 30000 && h_rejected > 10000, "nonvacuity");
  std::printf("{\"status\":\"pass\",\"guard_checks\":%llu,\"numeric_checks\":%llu,\"narrow\":%llu,\"wide\":%llu,\"h_rejected\":%llu}\n",
      static_cast<unsigned long long>(guard), static_cast<unsigned long long>(checked),
      static_cast<unsigned long long>(narrow), static_cast<unsigned long long>(wide),
      static_cast<unsigned long long>(h_rejected));
  return 0;
} catch (const std::exception& e) { std::fprintf(stderr,"failure: %s\n",e.what()); return 1; }
