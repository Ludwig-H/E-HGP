#pragma once

// Explicit port of exact primitives d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5
// and filtered proposal 484a89bc2dbd472cc0571ed31d59631d5f31f9b0a425118040c916fc16e5abcf.
// Internal domain: valid unique u16 index, 2..11 distinct valid sites, authentic
// candidates, immutable inputs/caps and coherent counters with A<=c. This is
// not a hostile-candidate admission API or a certificate of catalogue completeness.
// New product-port qualification is distinct from the pinned private receipts.
// No proposal is enabled by default; the unchanged F Builder owns its fallback.
#include <utility>

#include "silent_incidence.hpp"

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
  }
  return ball;
}

inline constexpr const char* kWorkAccounting =
    "reference_ordinal_plus_native_z_q3_q4_proposal_v2";
inline constexpr size_t kMaxPivots = 16;

struct Limits {
  // Explicit opt-in: zero disables proposals and retains the F fallback.
  u64 max_meb_proposal_supports = 0;
};

struct Work {
  // This object belongs to one attempt/order and MUST survive every local
  // call and fallback. Never reset this counter inside miniball/propose.
  u64 meb_proposal_supports = 0;
  u64 pivots = 0;
  u64 certified = 0;
  u64 fallback = 0;
  // Real supports tried by F, not the virtual reference ordinal charges.
  u64 reference_supports = 0;
};

struct NoObserver {
  void before_pair_selection(const Work&, const Limits&) const noexcept {}
  void before_form(const Work& work, const Limits&, u8 q) const {
    ::mhgp7_test_before_form(work.meb_proposal_supports, work.certified, q);
  }
};

enum class Attempt { kRejected, kAccepted, kExhausted };

// Every attempted form is charged before its observer and predicates.
// Observers are passive; the production NoObserver neither mutates nor throws.
template <class Observer = NoObserver>
inline Attempt charged_form(const CloudIndex& ix,
                            const std::array<i32, 11>& sites,
                            std::array<size_t, 4> slots, u8 q,
                            Candidate* result, const Limits& limits,
                            Work* work, Observer* observer) {
  if (work->meb_proposal_supports >= limits.max_meb_proposal_supports)
    return Attempt::kExhausted;
  // The guard proves that this increment cannot overflow, including limit MAX.
  ++work->meb_proposal_supports;
  observer->before_form(*work, limits, q);
  const bool built = form(ix, sites, slots, q, result);
  return built ? Attempt::kAccepted : Attempt::kRejected;
}

// Native trajectory ONLY, not a generic small-ball solver. The old positive
// support Q comes from global-diameter initialization and radius-increasing
// native pivots; subset[n-1] is their first strict violator z. Consequently every
// acceptable pivot support contains z and has q>=3. Filtering must be stable:
// q3 before q4, with the original pairs/triples of Q completed by the final z.
// No charge is made for a removed candidate; this is a new P calendar.
// Distinguish rejection from exhaustion so exhaustion exits every loop.
template <class Observer = NoObserver>
inline Attempt native_pivot_ball(const CloudIndex& ix,
                                 const std::array<i32, 11>& sites,
                                 const std::array<size_t, 5>& subset, size_t n,
                                 Candidate* result, const Limits& limits,
                                 Work* work, Observer* observer) {
  if (n < 3 || n > 5) return Attempt::kRejected;
  const size_t old_count = n - 1;
  const size_t violator = subset[old_count];
  const auto accept = [&](std::array<size_t, 4> slots, u8 q) {
    Candidate candidate;
    const Attempt attempt = charged_form(
        ix, sites, slots, q, &candidate, limits, work, observer);
    if (attempt != Attempt::kAccepted) return attempt;
    // Containment remains over all of T, not just the proposed support.
    // Intermediate extra-shells are permitted; only the FINAL shell is tested.
    for (size_t i = 0; i < n; ++i)
      if (candidate.power(point(ix, sites, subset[i])) > 0)
        return Attempt::kRejected;
    *result = candidate;
    return Attempt::kAccepted;
  };
  for (size_t a = 0; a < old_count; ++a)
    for (size_t b = a + 1; b < old_count; ++b) {
      const Attempt attempt = accept({subset[a], subset[b], violator, 0}, 3);
      if (attempt != Attempt::kRejected) return attempt;
    }
  for (size_t a = 0; a < old_count; ++a)
    for (size_t b = a + 1; b < old_count; ++b)
      for (size_t c = b + 1; c < old_count; ++c) {
        const Attempt attempt = accept(
            {subset[a], subset[b], subset[c], violator}, 4);
        if (attempt != Attempt::kRejected) return attempt;
      }
  return Attempt::kRejected;
}

template <class Observer = NoObserver>
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
  if (charged_form(ix, sites, {a, b, 0, 0}, 2, &candidate,
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
    if (native_pivot_ball(ix, sites, subset, count, &candidate,
                          limits, work, observer) != Attempt::kAccepted)
      return false;
  }
  return false;
}

// The callable must execute only reference work: c never decreases and each
// increment is one real F support. Starting from A<=c gives A+delta<=c_final,
// even on stack unwinding. Neither this guard nor the caller sums A+p in u64.
template <class Callable>
inline bool reference_counted(SilentIncidenceStats& stats, Work& work,
                              Callable&& callable) {
  struct ReferenceWorkMirror {
    const SilentIncidenceStats& stats;
    Work& work;
    const u64 prior;
    ~ReferenceWorkMirror() noexcept {
      work.reference_supports += stats.meb_supports - prior;
    }
  } mirror{stats, work, stats.meb_supports};
  return std::forward<Callable>(callable)();
}

// Internal dispatcher: reference must already bind the same ix, caps and out.
// It owns no Builder, no global catalogue and no public-output transaction.
// Work belongs to the whole order; paid proposal/reference charges survive
// fallback and exceptions. The FULL caller retains its external call budget.
template <class Observer = NoObserver>
inline bool miniball(const CloudIndex& ix, silent_detail::Builder& reference,
                     const SilentIncidenceLimits& caps, SilentIncidenceResult* out,
                     const std::array<i32, 11>& sites, size_t n,
                     silent_detail::LocalBall* ball, const Limits& limits,
                     Work* work, Observer* observer, size_t pivot_cap = 16) {
  const auto fallback = [&]() {
    return reference_counted(out->stats, *work, [&]() {
      return reference.miniball(sites, n, ball);
    });
  };
  if (limits.max_meb_proposal_supports == 0) {
    // P0 keeps F's literal path, including its own call/legacy-budget guard.
    // fallback counts decisions with legacy room, not all entries into F.
    if (out->stats.meb_supports < caps.max_meb_supports) ++work->fallback;
    return fallback();
  }
  if (out->stats.meb_supports >= caps.max_meb_supports) {
    ++out->stats.meb_calls;
    out->status = SilentIncidenceStatus::kResourceExhausted;
    out->reason = "silent_meb_support_budget";
    return false;
  }
  Candidate candidate;
  // Short circuit is intentional: an exhausted P never searches for a pair.
  if (work->meb_proposal_supports >= limits.max_meb_proposal_supports ||
      !propose(ix, sites, n, &candidate, limits, work, observer, pivot_cap)) {
    ++work->fallback;
    return fallback();
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
  // Finalization of this accepted candidate, not another speculative support.
  // Canonical slots and literal unreduced q4 levels are retained.
  *ball = materialize(ix, sites, candidate);
  return true;
}

}  // namespace mhgp7::meb_proposal_detail
