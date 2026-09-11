#pragma once
// PRIVATE: same canonical MEB, one lexicographically first maximum pair.
// The extra counter charges each squared-distance pair actually evaluated.
#include "source/morsehgp3D_v7/src/forest/anchor_meb.hpp"
#ifndef MHGP7_DIAMETER_MUTANT
#define MHGP7_DIAMETER_MUTANT 0
#endif
namespace mhgp7 {
inline AnchorMebResult anchor_meb_diameter(std::span<const P3> sites, AnchorMebWork& work, u64& diameter_pairs) noexcept {
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
  const auto n = static_cast<u8>(sites.size());
  i64 diameter = -1;
  u8 extreme_a = 0, extreme_b = 1;
  for (u8 a = 0; a < n; ++a) for (u8 b = a + 1; b < n; ++b) {
    if (!anchor_meb_detail::charge(diameter_pairs))
      return failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_diameter_pairs_overflow");
#if MHGP7_DIAMETER_MUTANT == 2
    --diameter_pairs;  // Deliberately hide a physically evaluated distance.
#endif
    const i64 distance = p3_norm2(p3_sub(sites[a], sites[b]));
#if MHGP7_DIAMETER_MUTANT == 1
    const bool farther = distance >= diameter;
#else
    const bool farther = distance > diameter;
#endif
    if (farther) { diameter = distance; extreme_a = a; extreme_b = b; }
  }
  // Visit each site exactly once: the two witnesses first, then the old order.
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
#if MHGP7_DIAMETER_MUTANT == 3
      const auto& point = sites[position];
#else
      const auto& point = sites[power_order[position]];
#endif
      if (!anchor_meb_detail::charge(work.power_tests)) {
        result = failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_power_tests_overflow");
        return true;
      }
      const i128 power = candidate.power(point);
      if (power > 0) return false;
      if (power == 0) ++shell;
#if MHGP7_DIAMETER_MUTANT == 4
      if (position < 2 && power == 0) ++shell;
#endif
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

}  // namespace mhgp7
