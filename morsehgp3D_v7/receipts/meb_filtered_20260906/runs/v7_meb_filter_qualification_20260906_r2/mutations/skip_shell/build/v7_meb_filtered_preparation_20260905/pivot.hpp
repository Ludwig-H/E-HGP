#pragma once

// Private filtered preparation: not compiled, qualified, dispatched or integrated.
// Explicit derivative of dual_budget_prototype/pivot.hpp SHA256 0645aa00...
// Reuse the pinned exact forms, ordinal and raw materialization from historical
// d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5.
// The fallback Builder still comes from the unchanged F include closure.
#include "../v7_meb_pivot_prototype/pivot.hpp"

namespace mhgp7::meb_filtered_preparation {

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
};

struct NoObserver {
  void before_pair_selection(const Work&, const Limits&) const noexcept {}
  void before_form(const Work&, const Limits&, u8) const noexcept {}
};

using pivot_prototype::Candidate;
using pivot_prototype::materialize;
using pivot_prototype::ordinal;
using pivot_prototype::point;

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
  const bool built = pivot_prototype::form(ix, sites, slots, q, result);
  if constexpr (ChargeAfter) ++work->meb_proposal_supports;
  return built ? Attempt::kAccepted : Attempt::kRejected;
}

// Native trajectory ONLY, not a generic small-ball solver. The old positive
// support Q comes from global-diameter initialization and radius-increasing
// native pivots; subset[n-1] is their first strict violator z. Consequently every
// acceptable pivot support contains z and has q>=3. Filtering must be stable:
// q3 before q4, with the original pairs/triples of Q completed by the final z.
// No charge is made for a removed candidate; this is a new P calendar.
// Distinguish rejection from exhaustion so exhaustion exits every loop.
template <bool ChargeAfter = false, class Observer = NoObserver>
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
    const Attempt attempt = charged_form<ChargeAfter>(
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
      if (false && shell != candidate.q) return false;
      *result = candidate;
      return true;
    }
    if (step == pivot_cap) return false;
    std::array<size_t, 5> subset{};
    for (size_t i = 0; i < candidate.q; ++i) subset[i] = candidate.slots[i];
    subset[candidate.q] = outside;
    const size_t count = static_cast<size_t>(candidate.q) + 1;
    ++work->pivots;
    if (native_pivot_ball<ChargeAfter>(ix, sites, subset, count, &candidate,
                                     limits, work, observer) != Attempt::kAccepted)
      return false;
  }
  return false;
}

template <bool ChargeAfter = false, class Observer = NoObserver>
inline bool miniball(const CloudIndex& ix, const std::vector<ForestEvent>& direct,
                     const SilentIncidenceLimits& caps, SilentIncidenceResult* out,
                     const std::array<i32, 11>& sites, size_t n,
                     silent_detail::LocalBall* ball, const Limits& limits,
                     Work* work, Observer* observer, size_t pivot_cap = 16) {
  if (out->stats.meb_supports >= caps.max_meb_supports) {
    ++out->stats.meb_calls;
    out->status = SilentIncidenceStatus::kResourceExhausted;
    out->reason = "silent_meb_support_budget";
    return false;
  }
  Candidate candidate;
  // Short circuit is intentional: an exhausted P never searches for a pair.
  if (work->meb_proposal_supports >= limits.max_meb_proposal_supports ||
      !propose<ChargeAfter>(ix, sites, n, &candidate, limits,
                            work, observer, pivot_cap)) {
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
  // Finalization of this accepted candidate, not another speculative support.
  // The historical helper preserves canonical slots and literal q4 raw level.
  *ball = materialize(ix, sites, candidate);
  return true;
}

}  // namespace mhgp7::meb_filtered_preparation
