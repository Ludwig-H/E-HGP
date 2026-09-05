#pragma once

// Private product-overlay port; not integrated, compiled or qualified.
// Exact authorities:
// d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5
// 0645aa00add4d4cb387861b8f6dbd4fa0734ba5b4f3ad712caad8886b3541c2d
// Internal preconditions: validated unique u16 CloudIndex; sites[] positions
// in range; n in [2,11]; form/ordinal receive valid distinct slots and q 2..4.
// No hostile-candidate admission API, Builder fallback or legacy accounting
// lives here. The caller owns the persistent Work and terminal transaction.
#include <array>
#include <cstddef>
#include <utility>

#include "src/lanes/q2.hpp"
#include "src/lanes/q3.hpp"
#include "src/lanes/q4.hpp"
#include "src/tree/cloud_index.hpp"

namespace mhgp7::meb_proposal_detail {

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

template <class OutBall>
inline OutBall materialize(const CloudIndex& ix,
                           const std::array<i32, 11>& sites,
                           const Candidate& candidate) {
  OutBall ball{};
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
  }
  return ball;
}

inline constexpr const char* kWorkAccounting =
    "reference_ordinal_plus_proposal_v1";
inline constexpr size_t kMaxPivots = 16;

struct Limits {
  // Explicit opt-in: zero disables proposals; the caller owns the F fallback.
  u64 max_meb_proposal_supports = 0;
};

struct Work {
  // This object belongs to one attempt/order and MUST survive every local
  // call and fallback. Never reset this counter inside miniball/propose.
  u64 meb_proposal_supports = 0;
  u64 pivots = 0;
  // Caller-owned terminal observations; propose() does not change these two.
  u64 certified = 0;
  u64 fallback = 0;
};

struct NoObserver {
  void before_pair_selection(const Work&, const Limits&) const noexcept {}
  void before_form(const Work&, const Limits&, u8) const noexcept {}
};

enum class Attempt { kRejected, kAccepted, kExhausted };

// ChargeAfter is a private causal mutant, not a testing-only bypass of the
// nominal route. An observer sees the counter at the actual form-call boundary.
template <bool ChargeAfter = false, class Observer = NoObserver>
inline Attempt charged_form(const CloudIndex& ix,
                            const std::array<i32, 11>& sites,
                            std::array<size_t, 4> slots, u8 q,
                            Candidate* result, const Limits& limits,
                            Work* work, Observer* observer) {
  if (work->meb_proposal_supports >= limits.max_meb_proposal_supports)
    return Attempt::kExhausted;
  // The guard proves that this increment cannot overflow, including limit MAX.
  if constexpr (!ChargeAfter) ++work->meb_proposal_supports;
  observer->before_form(*work, limits, q);
  const bool built = form(ix, sites, slots, q, result);
  if constexpr (ChargeAfter) ++work->meb_proposal_supports;
  return built ? Attempt::kAccepted : Attempt::kRejected;
}

// At most five sites: old positive support and one strict violator. Distinguish
// rejection from exhaustion so the first exhausted attempt exits all loops.
template <bool ChargeAfter = false, class Observer = NoObserver>
inline Attempt small_ball(const CloudIndex& ix,
                          const std::array<i32, 11>& sites,
                          const std::array<size_t, 5>& subset, size_t n,
                          Candidate* result, const Limits& limits,
                          Work* work, Observer* observer) {
  if (n < 2 || n > 5) return Attempt::kRejected;
  const auto accept = [&](std::array<size_t, 4> slots, u8 q) {
    Candidate candidate;
    const Attempt attempt = charged_form<ChargeAfter>(
        ix, sites, slots, q, &candidate, limits, work, observer);
    if (attempt != Attempt::kAccepted) return attempt;
    for (size_t i = 0; i < n; ++i)
      if (candidate.power(point(ix, sites, subset[i])) > 0)
        return Attempt::kRejected;
    *result = candidate;
    return Attempt::kAccepted;
  };
  for (size_t a = 0; a < n; ++a)
    for (size_t b = a + 1; b < n; ++b) {
      const Attempt attempt = accept({subset[a], subset[b], 0, 0}, 2);
      if (attempt != Attempt::kRejected) return attempt;
    }
  for (size_t a = 0; a < n; ++a)
    for (size_t b = a + 1; b < n; ++b)
      for (size_t c = b + 1; c < n; ++c) {
        const Attempt attempt = accept({subset[a], subset[b], subset[c], 0}, 3);
        if (attempt != Attempt::kRejected) return attempt;
      }
  for (size_t a = 0; a < n; ++a)
    for (size_t b = a + 1; b < n; ++b)
      for (size_t c = b + 1; c < n; ++c)
        for (size_t d = c + 1; d < n; ++d) {
          const Attempt attempt = accept(
              {subset[a], subset[b], subset[c], subset[d]}, 4);
          if (attempt != Attempt::kRejected) return attempt;
        }
  return Attempt::kRejected;
}

template <bool ChargeAfter = false, class Observer = NoObserver>
inline bool propose(const CloudIndex& ix, const std::array<i32, 11>& sites,
                    size_t n, Candidate* result, const Limits& limits,
                    Work* work, Observer* observer, size_t pivot_cap = 16) {
  if (n < 2 || n > 11) return false;
  if (work->meb_proposal_supports >= limits.max_meb_proposal_supports)
    return false;  // Before even selecting an extreme pair.
  if (pivot_cap > kMaxPivots) pivot_cap = kMaxPivots;
  observer->before_pair_selection(*work, limits);
  size_t a = 0, b = 1;
  i64 distance = -1;
  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j) {
      const i64 d = p3_norm2(p3_sub(point(ix, sites, i), point(ix, sites, j)));
      if (d > distance) { distance = d; a = i; b = j; }
    }
  Candidate candidate;
  if (charged_form<ChargeAfter>(ix, sites, {a, b, 0, 0}, 2, &candidate,
                               limits, work, observer) != Attempt::kAccepted)
    return false;
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
    const size_t count = static_cast<size_t>(candidate.q) + 1;
    ++work->pivots;
    if (small_ball<ChargeAfter>(ix, sites, subset, count, &candidate,
                               limits, work, observer) != Attempt::kAccepted)
      return false;
  }
  return false;
}

}  // namespace mhgp7::meb_proposal_detail
