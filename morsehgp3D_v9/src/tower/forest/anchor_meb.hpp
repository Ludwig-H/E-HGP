#pragma once

// Exact local MEB for ball-anchor descent, not the regular F resolver.
// Additional selected boundary sites are valid. No cloud census, catalogue,
// floating proposal, virtual ordinal, work quota or allocation lives here.
// Positive supports of at most four sites certify minimality by convexity;
// the first enclosing one suffices by uniqueness of the Euclidean MEB.
#include <array>
#include <limits>
#include <span>

#include "../lanes/q2.hpp"
#include "../lanes/q3.hpp"
#include "../lanes/q4.hpp"

namespace mhgp9::tower {

inline constexpr const char* kAnchorMebWorkAccounting =
    "anchor_meb_first_maximal_pair_then_lexicographic_supports_extremes_first_v2";

enum class AnchorMebStatus { kOk, kInvalidInput, kCounterOverflow, kInvariantViolated };

struct AnchorMebWork {
  // Owned by the whole order. Failed calls retain the work already paid.
  u64 calls = 0;
  std::array<u64, 5> supports_by_size{};
  u64 power_tests = 0;
  u64 materializations = 0;
  // Pair distances paid to find the first maximal pair (port v9 of the
  // qualified v7 anchor_meb_diameter, receipts/meb_diameter_20260911).
  u64 pair_distances = 0;
};

struct AnchorMebResult {
  AnchorMebStatus status = AnchorMebStatus::kInvalidInput;
  const char* reason = "anchor_meb_invalid_input";
  BallKey key{};
  ExactLevel level{};
  u8 support_size = 0;
  std::array<u8, 4> support_slots{};
  u8 selected_shell_count = 0;
};

namespace anchor_meb_detail {

inline bool charge(u64& counter) noexcept {
  if (counter == std::numeric_limits<u64>::max()) return false;
  ++counter;
  return true;
}

struct Candidate {
  std::array<u8, 4> slots{};
  u8 q = 0;
  P3 a{}, b{};
  Q3Form three{};
  Q4Form four{};

  i128 power(const P3& point) const noexcept {
    if (q == 2) return p3_dot(p3_sub(point, a), p3_sub(point, b));
    if (q == 3) return q3_power(three, point);
    return q4_power(four, point);
  }
};

inline bool form(std::span<const P3> sites, std::array<u8, 4> slots, u8 q,
                 Candidate& candidate) noexcept {
  candidate.slots = slots;
  candidate.q = q;
  candidate.a = sites[slots[0]];
  candidate.b = sites[slots[1]];
  if (q == 2) return true;
  const auto& a = candidate.a;
  const auto& b = candidate.b;
  const auto& c = sites[slots[2]];
  if (q == 3) {
    if (p3_dot(p3_sub(b, a), p3_sub(c, a)) <= 0 ||
        p3_dot(p3_sub(a, b), p3_sub(c, b)) <= 0 ||
        p3_dot(p3_sub(a, c), p3_sub(b, c)) <= 0) return false;
    candidate.three = q3_form(a, b, c);
    return candidate.three.g > 0;
  }
  const auto& d = sites[slots[3]];
  candidate.four = q4_form(a, b, c, d);
  return candidate.four.det > 0 && q4_center_strictly_inside(candidate.four, a, b, c, d);
}

}  // namespace anchor_meb_detail

inline AnchorMebResult anchor_meb(std::span<const P3> sites, AnchorMebWork& work) noexcept {
  const auto failure = [](AnchorMebStatus status, const char* reason) {
    AnchorMebResult result;
    result.status = status;
    result.reason = reason;
    return result;  // Failed geometry is always empty, never a partial MEB.
  };
  if (!anchor_meb_detail::charge(work.calls))
    return failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_calls_overflow");
  if (sites.empty() || sites.size() > static_cast<std::size_t>(kFacetMaxK))
    return failure(AnchorMebStatus::kInvalidInput, "anchor_meb_site_count");
  for (std::size_t i = 0; i < sites.size(); ++i) {
    if (!p3_in_profile(sites[i]))
      return failure(AnchorMebStatus::kInvalidInput, "anchor_meb_coordinate_profile");
    for (std::size_t j = 0; j < i; ++j)
      if (sites[i] == sites[j])
        return failure(AnchorMebStatus::kInvalidInput, "anchor_meb_duplicate_position");
  }
  AnchorMebResult result;
  if (sites.size() == 1) {
    if (!anchor_meb_detail::charge(work.supports_by_size[1]) ||
        !anchor_meb_detail::charge(work.materializations))
      return failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_singleton_work_overflow");
    result.key = q2_ball_key(sites[0], sites[0]);
    result.level = promote_level(Rational128{0, 1});
    result.support_size = result.selected_shell_count = 1;
    result.status = AnchorMebStatus::kOk;
    result.reason = "anchor_meb_exact_local";
    return result;
  }
  // (a) Only the FIRST maximal pair (strict improvement keeps the
  // lexicographically first) can be the q=2 MEB: an enclosing diametral ball
  // realises the diameter, and any other maximal pair is then antipodal in
  // that same ball. Proof: morsehgp3D_v7/receipts/meb_diameter_20260911/
  // sources/current/PROOF.md. (b) Containment visits the two extremes first,
  // then the old order: a pure reordering of the same exact tests.
  const auto n = static_cast<u8>(sites.size());
  i64 diameter = -1;
  u8 extreme_a = 0, extreme_b = 1;
  for (u8 a = 0; a < n; ++a) for (u8 b = a + 1; b < n; ++b) {
    if (!anchor_meb_detail::charge(work.pair_distances))
      return failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_pair_distances_overflow");
    const i64 distance = p3_norm2(p3_sub(sites[a], sites[b]));
#if defined(MHGP9_MEB_MUTANT_LAST_MAXIMUM)
    const bool farther = distance >= diameter;  // MUTANT de compilation : derniere paire maximale
#else
    const bool farther = distance > diameter;
#endif
    if (farther) { diameter = distance; extreme_a = a; extreme_b = b; }
  }
  std::array<u8, kFacetMaxK> power_order{};
  power_order[0] = extreme_a; power_order[1] = extreme_b;
  u8 at = 2;
  for (u8 i = 0; i < n; ++i) if (i != extreme_a && i != extreme_b) power_order[at++] = i;
  bool finished = false;
  const auto attempt = [&](std::array<u8, 4> slots, u8 q) {
    if (!anchor_meb_detail::charge(work.supports_by_size[q])) {
      result = failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_supports_overflow");
      return true;
    }
    anchor_meb_detail::Candidate candidate;
    if (!anchor_meb_detail::form(sites, slots, q, candidate)) return false;
    u8 shell = 0;
    for (u8 position = 0; position < n; ++position) {
      const auto& point = sites[power_order[position]];
      if (!anchor_meb_detail::charge(work.power_tests)) {
        result = failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_power_tests_overflow");
        return true;
      }
      const i128 power = candidate.power(point);
      if (power > 0) return false;
      if (power == 0) ++shell;
    }
    if (!anchor_meb_detail::charge(work.materializations)) {
      result = failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_materializations_overflow");
      return true;
    }
    // Materialize only the accepted support; the containment pass already
    // counted the complete selected shell, including non-support sites.
    result.support_size = q;
    result.support_slots = slots;
    result.selected_shell_count = shell;
    if (q == 2) {
      result.key = q2_ball_key(candidate.a, candidate.b);
      result.level = promote_level(q2_exact_level(p3_norm2(p3_sub(candidate.a, candidate.b))));
    } else if (q == 3) {
      result.key = q3_ball_key(candidate.three);
      result.level = promote_level(q3_exact_level(candidate.a, candidate.b, sites[slots[2]]));
    } else {
      result.key = ball_key_reduce(q4_ball_form(candidate.four));
      result.level = q4_level_raw(candidate.four);
    }
    result.status = AnchorMebStatus::kOk;
    result.reason = "anchor_meb_exact_local";
    return true;
  };
  finished = attempt({extreme_a, extreme_b, 0, 0}, 2);
  for (u8 a = 0; a < n && !finished; ++a)
    for (u8 b = a + 1; b < n && !finished; ++b)
      for (u8 c = b + 1; c < n && !finished; ++c)
        finished = attempt({a, b, c, 0}, 3);
  for (u8 a = 0; a < n && !finished; ++a)
    for (u8 b = a + 1; b < n && !finished; ++b)
      for (u8 c = b + 1; c < n && !finished; ++c)
        for (u8 d = c + 1; d < n && !finished; ++d)
          finished = attempt({a, b, c, d}, 4);
  if (!finished)
    return failure(AnchorMebStatus::kInvariantViolated, "anchor_meb_no_containing_positive_support");
  return result;
}

}  // namespace mhgp9::tower
