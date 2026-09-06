#pragma once

// Bounded experimental proposal, not integrated into the product.
#include "src/forest/silent_incidence.hpp"

namespace mhgp7::pivot_prototype {

struct Work {
  u64 candidates = 0;
  u64 pivots = 0;
  u64 certified = 0;
  u64 fallback = 0;
};

struct Candidate {
  std::array<size_t, 4> slots{};
  u8 q = 0;
  P3 a{}, b{};
  Q3Form three{};
  Q4Form four{};

  i128 power(const P3& z) const {
    if (q == 2) return p3_dot(p3_sub(z, a), p3_sub(z, b));
    if (q == 3) return q3_power(three, z);
    return q4_power(four, z);
  }
};

inline const P3& point(const CloudIndex& ix, const std::array<i32, 11>& sites, size_t slot) {
  return ix.upos[(size_t)sites[slot]];
}

inline bool form(const CloudIndex& ix, const std::array<i32, 11>& sites,
                 std::array<size_t, 4> slots, u8 q, Candidate* result) {
  // Canonical order is position in sites[], not PointId or Morton index.
  for (size_t i = 1; i < q; ++i)
    for (size_t j = i; j > 0 && slots[j] < slots[j - 1]; --j)
      std::swap(slots[j], slots[j - 1]);
  Candidate candidate;
  candidate.slots = slots;
  candidate.q = q;
  candidate.a = point(ix, sites, slots[0]);
  candidate.b = point(ix, sites, slots[1]);
  if (q >= 3) {
    const P3& c = point(ix, sites, slots[2]);
    if (q == 3) {
      if (p3_dot(p3_sub(candidate.b, candidate.a), p3_sub(c, candidate.a)) <= 0 ||
          p3_dot(p3_sub(candidate.a, candidate.b), p3_sub(c, candidate.b)) <= 0 ||
          p3_dot(p3_sub(candidate.a, c), p3_sub(candidate.b, c)) <= 0) return false;
      candidate.three = q3_form(candidate.a, candidate.b, c);
      if (candidate.three.g <= 0) return false;
    } else {
      const P3& d = point(ix, sites, slots[3]);
      candidate.four = q4_form(candidate.a, candidate.b, c, d);
      if (candidate.four.det <= 0 ||
          !q4_center_strictly_inside(candidate.four, candidate.a, candidate.b, c, d)) return false;
    }
  }
  *result = candidate;
  return true;
}

// At most five sites: old positive support and one strict violator.
inline bool small_ball(const CloudIndex& ix, const std::array<i32, 11>& sites,
                       const std::array<size_t, 5>& subset, size_t n,
                       Candidate* result, Work* work) {
  const auto accept = [&](std::array<size_t, 4> slots, u8 q) {
    ++work->candidates;
    Candidate candidate;
    if (!form(ix, sites, slots, q, &candidate)) return false;
    for (size_t i = 0; i < n; ++i)
      if (candidate.power(point(ix, sites, subset[i])) > 0) return false;
    *result = candidate;
    return true;
  };
  for (size_t a = 0; a < n; ++a)
    for (size_t b = a + 1; b < n; ++b)
      if (accept({subset[a], subset[b], 0, 0}, 2)) return true;
  for (size_t a = 0; a < n; ++a)
    for (size_t b = a + 1; b < n; ++b)
      for (size_t c = b + 1; c < n; ++c)
        if (accept({subset[a], subset[b], subset[c], 0}, 3)) return true;
  for (size_t a = 0; a < n; ++a)
    for (size_t b = a + 1; b < n; ++b)
      for (size_t c = b + 1; c < n; ++c)
        for (size_t d = c + 1; d < n; ++d)
          if (accept({subset[a], subset[b], subset[c], subset[d]}, 4)) return true;
  return false;
}

inline bool propose(const CloudIndex& ix, const std::array<i32, 11>& sites,
                    size_t n, Candidate* result, Work* work, size_t pivot_cap = 16) {
  if (n < 2 || n > 11) return false;
  size_t a = 0, b = 1;
  i64 distance = -1;
  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j) {
      const i64 d = p3_norm2(p3_sub(point(ix, sites, i), point(ix, sites, j)));
      if (d > distance) { distance = d; a = i; b = j; }
    }
  Candidate candidate;
  if (!form(ix, sites, {a, b, 0, 0}, 2, &candidate)) return false;
  ++work->candidates;
  for (size_t step = 0; step <= pivot_cap; ++step) {
    size_t outside = n, shell = 0;
    for (size_t i = 0; i < n; ++i) {
      const i128 power = candidate.power(point(ix, sites, i));
      if (power > 0) { outside = i; break; }
      if (power == 0) ++shell;
    }
    if (outside == n) {
      if (shell != candidate.q) return false;
      *result = candidate;
      return true;
    }
    if (step == pivot_cap) return false;
    std::array<size_t, 5> subset{};
    for (size_t i = 0; i < candidate.q; ++i) subset[i] = candidate.slots[i];
    subset[candidate.q] = outside;
    const size_t count = (size_t)candidate.q + 1;
    ++work->pivots;
    if (!small_ball(ix, sites, subset, count, &candidate, work)) return false;
  }
  return false;
}

inline u64 choose(size_t n, size_t k) {
  if (k > n) return 0;
  u64 value = 1;
  for (size_t i = 1; i <= k; ++i) value = value * (n - k + i) / i;
  return value;
}

inline u64 ordinal(size_t n, const Candidate& candidate) {
  u64 count = 1;
  for (size_t q = 2; q < candidate.q; ++q) count += choose(n, q);
  size_t next = 0;
  for (size_t i = 0; i < candidate.q; ++i) {
    for (size_t slot = next; slot < candidate.slots[i]; ++slot)
      count += choose(n - slot - 1, candidate.q - i - 1);
    next = candidate.slots[i] + 1;
  }
  return count;
}

inline silent_detail::LocalBall materialize(const CloudIndex& ix,
                                           const std::array<i32, 11>& sites,
                                           const Candidate& candidate) {
  silent_detail::LocalBall ball;
  ball.q = candidate.q;
  for (size_t i = 0; i < candidate.q; ++i) ball.support[i] = sites[candidate.slots[i]];
  if (candidate.q == 2) {
    ball.key = q2_ball_key(candidate.a, candidate.b);
    ball.level = promote_level(q2_exact_level(p3_norm2(p3_sub(candidate.a, candidate.b))));
  } else if (candidate.q == 3) {
    ball.key = q3_ball_key(candidate.three);
    ball.level = promote_level(q3_exact_level(candidate.a, candidate.b,
                                            point(ix, sites, candidate.slots[2])));
  } else {
    ball.key = ball_key_reduce(q4_ball_form(candidate.four));
    ball.level = q4_level_raw(candidate.four);
    for (size_t j = 2; j > 0; --j) ball.level.num[j] = (ball.level.num[j] << 1) | (ball.level.num[j - 1] >> 63);
    ball.level.num[0] <<= 1; ball.level.den *= 2;
  }
  return ball;
}

inline bool miniball(const CloudIndex& ix, const std::vector<ForestEvent>& direct,
                     const SilentIncidenceLimits& caps, SilentIncidenceResult* out,
                     const std::array<i32, 11>& sites, size_t n,
                     silent_detail::LocalBall* ball, Work* work, size_t pivot_cap = 16) {
  if (out->stats.meb_supports >= caps.max_meb_supports) {
    ++out->stats.meb_calls;
    out->status = SilentIncidenceStatus::kResourceExhausted;
    out->reason = "silent_meb_support_budget";
    return false;
  }
  Candidate candidate;
  if (!propose(ix, sites, n, &candidate, work, pivot_cap)) {
    ++work->fallback;
    silent_detail::Builder reference(ix, direct, caps, out);
    return reference.miniball(sites, n, ball);
  }
  ++work->certified;
  ++out->stats.meb_calls;
  const u64 count = ordinal(n, candidate);
  const u64 prior = out->stats.meb_supports;
  if (prior >= caps.max_meb_supports || count > caps.max_meb_supports - prior) {
    if (prior < caps.max_meb_supports) out->stats.meb_supports = caps.max_meb_supports;
    out->status = SilentIncidenceStatus::kResourceExhausted;
    out->reason = "silent_meb_support_budget";
    return false;
  }
  out->stats.meb_supports += count;
  *ball = materialize(ix, sites, candidate);
  return true;
}

}  // namespace mhgp7::pivot_prototype
