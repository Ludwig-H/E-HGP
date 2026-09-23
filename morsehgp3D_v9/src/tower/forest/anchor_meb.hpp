#pragma once

// Exact local MEB for ball-anchor descent, not the regular F resolver.
// Additional selected boundary sites are valid. No cloud census, catalogue,
// virtual ordinal, work quota or allocation lives here. Positive supports of
// at most four sites certify minimality by convexity; the first enclosing one
// suffices by uniqueness of the Euclidean MEB.
//
// anchor_meb is the reference: first maximal pair, then every triple and
// quadruple in lexicographic order. anchor_meb_proposed returns the SAME
// result (key, level, support slots, selected shell) with less work: a
// double-precision Welzl run only PROPOSES a support, which the same exact
// attempt verifies (positive support, every site contained). A verified
// support is the MEB by uniqueness, and every valid support lies on its exact
// boundary; the reference's first valid support is therefore found by the
// same lexicographic enumeration restricted to the boundary sites. A
// proposal that fails verification falls back to the full enumeration. The
// double values decide nothing.
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <span>

#include "../lanes/q2.hpp"
#include "../lanes/q3.hpp"
#include "../lanes/q4.hpp"

namespace mhgp9::tower {

inline constexpr const char* kAnchorMebWorkAccounting =
    "anchor_meb_first_maximal_pair_then_lexicographic_supports_extremes_first_v2";
inline constexpr const char* kAnchorMebProposedWorkAccounting =
    "anchor_meb_first_maximal_pair_then_double_welzl_proposal_exact_boundary_canonical_v3";

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
  // anchor_meb_proposed only: proposals made, verified by the exact attempt,
  // canonicalized over a boundary with more sites than the support, and
  // fallbacks to the full enumeration.
  u64 proposals = 0, verified_proposals = 0, boundary_canonicalizations = 0, proposal_fallbacks = 0;
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

namespace anchor_meb_detail {

// Double-precision proposal only (never a decision): the ball of 1..4 sites
// with every one on its boundary, and Welzl's recursion over n <= 10 sites.
struct Proposal {
  double center[3] = {0, 0, 0};
  double radius2 = -1;  // < 0: degenerate, no proposal
  std::array<u8, 4> support{};
  u8 size = 0;
};

inline Proposal boundary_ball(const double (*p)[3], const u8* r, u8 nr) noexcept {
  Proposal out;
  out.size = nr;
  for (u8 i = 0; i < nr; ++i) out.support[i] = r[i];
  if (nr == 0) return out;
  const double* a = p[r[0]];
  if (nr == 1) {
    for (int k = 0; k < 3; ++k) out.center[k] = a[k];
    out.radius2 = 0;
    return out;
  }
  double u[3], v[3], w[3];
  for (int k = 0; k < 3; ++k) u[k] = p[r[1]][k] - a[k];
  if (nr == 2) {
    for (int k = 0; k < 3; ++k) out.center[k] = a[k] + u[k] / 2;
    out.radius2 = (u[0] * u[0] + u[1] * u[1] + u[2] * u[2]) / 4;
    return out;
  }
  for (int k = 0; k < 3; ++k) v[k] = p[r[2]][k] - a[k];
  double offset[3];
  if (nr == 3) {
    w[0] = u[1] * v[2] - u[2] * v[1]; w[1] = u[2] * v[0] - u[0] * v[2]; w[2] = u[0] * v[1] - u[1] * v[0];
    const double ww = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
    const double uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2], vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (!(ww > 1e-9 * uu * vv)) return out;  // collinear: no proposal
    // center - a = (|u|^2 (v x w) + |v|^2 (w x u)) / (2 |w|^2)
    const double vw[3] = {v[1] * w[2] - v[2] * w[1], v[2] * w[0] - v[0] * w[2], v[0] * w[1] - v[1] * w[0]};
    const double wu[3] = {w[1] * u[2] - w[2] * u[1], w[2] * u[0] - w[0] * u[2], w[0] * u[1] - w[1] * u[0]};
    for (int k = 0; k < 3; ++k) offset[k] = (uu * vw[k] + vv * wu[k]) / (2 * ww);
  } else {
    for (int k = 0; k < 3; ++k) w[k] = p[r[3]][k] - a[k];
    // 2 M x = rhs with rows u, v, w.
    const double rhs[3] = {u[0] * u[0] + u[1] * u[1] + u[2] * u[2], v[0] * v[0] + v[1] * v[1] + v[2] * v[2],
                           w[0] * w[0] + w[1] * w[1] + w[2] * w[2]};
    const double det = u[0] * (v[1] * w[2] - v[2] * w[1]) - u[1] * (v[0] * w[2] - v[2] * w[0]) +
                       u[2] * (v[0] * w[1] - v[1] * w[0]);
    if (!(std::fabs(det) > 1e-12 * std::sqrt(rhs[0] * rhs[1] * rhs[2]))) return out;  // coplanar
    const double m[3][3] = {{u[0], u[1], u[2]}, {v[0], v[1], v[2]}, {w[0], w[1], w[2]}};
    for (int k = 0; k < 3; ++k) {
      double c[3][3];
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) c[i][j] = j == k ? rhs[i] / 2 : m[i][j];
      offset[k] = (c[0][0] * (c[1][1] * c[2][2] - c[1][2] * c[2][1]) - c[0][1] * (c[1][0] * c[2][2] - c[1][2] * c[2][0]) +
                   c[0][2] * (c[1][0] * c[2][1] - c[1][1] * c[2][0])) / det;
    }
  }
  for (int k = 0; k < 3; ++k) out.center[k] = a[k] + offset[k];
  out.radius2 = offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2];
  return out;
}

inline bool proposal_contains(const Proposal& ball, const double* q) noexcept {
  if (ball.radius2 < 0) return false;
  const double d0 = q[0] - ball.center[0], d1 = q[1] - ball.center[1], d2 = q[2] - ball.center[2];
  const double d = d0 * d0 + d1 * d1 + d2 * d2;
  return d <= ball.radius2 * (1 + 1e-12) + 1e-6;
}

// Welzl over the first n entries of order, with r (nr <= 4) on the boundary.
inline Proposal welzl(const double (*p)[3], const u8* order, u8 n, u8* r, u8 nr) noexcept {
  if (n == 0 || nr == 4) return boundary_ball(p, r, nr);
  const u8 last = order[n - 1];
  Proposal ball = welzl(p, order, static_cast<u8>(n - 1), r, nr);
  if (proposal_contains(ball, p[last])) return ball;
  r[nr] = last;
  return welzl(p, order, static_cast<u8>(n - 1), r, static_cast<u8>(nr + 1));
}

}  // namespace anchor_meb_detail

inline AnchorMebResult anchor_meb_impl(std::span<const P3> sites, AnchorMebWork& work, bool propose) noexcept {
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
  std::array<u8, kFacetMaxK> boundary{};  // exact boundary sites of the last contained candidate
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
      if (power == 0) { boundary[shell] = power_order[position]; ++shell; }
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
  if (!finished && propose) {
    // Proposal (q2 is settled: only the first maximal pair can be a q2 MEB).
    if (!anchor_meb_detail::charge(work.proposals)) return failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_proposals_overflow");
    double coordinates[kFacetMaxK][3];
    for (u8 i = 0; i < n; ++i) {
      coordinates[i][0] = static_cast<double>(sites[i].x);
      coordinates[i][1] = static_cast<double>(sites[i].y);
      coordinates[i][2] = static_cast<double>(sites[i].z);
    }
    std::array<u8, kFacetMaxK> order{};
    for (u8 i = 0; i < n; ++i) order[i] = power_order[static_cast<u8>(n - 1 - i)];  // extremes enter first
    u8 boundary_set[4] = {0, 0, 0, 0};
    auto proposal = anchor_meb_detail::welzl(coordinates, order.data(), n, boundary_set, 0);
#if defined(MHGP9_MEB_PROPOSED_TEST_CORRUPT)
    // Test build only: every third proposal names a wrong site, so that the
    // exact verification refuses it and the fallback is exercised.
    if (work.proposals % 3 == 0 && proposal.size >= 3)
      proposal.support[0] = static_cast<u8>((proposal.support[0] + 1) % n);
#endif
    if (proposal.radius2 >= 0 && proposal.size >= 3) {
      std::array<u8, 4> slots{};
      for (u8 i = 0; i < proposal.size; ++i) slots[i] = proposal.support[i];
      std::sort(slots.begin(), slots.begin() + proposal.size);
      const bool verified = attempt(slots, proposal.size);
      if (verified && result.status != AnchorMebStatus::kOk) return result;  // counter overflow inside
      if (verified) {
        if (!anchor_meb_detail::charge(work.verified_proposals))
          return failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_proposals_overflow");
#if !defined(MHGP9_MEB_PROPOSED_MUTANT_NO_CANONICAL)
        if (result.selected_shell_count == result.support_size) return result;
#else
        return result;  // mutant: the verified support even on a larger boundary
#endif
        // More boundary sites than the support: the reference's first valid
        // support, triples then quadruples, among the boundary sites only.
        if (!anchor_meb_detail::charge(work.boundary_canonicalizations))
          return failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_proposals_overflow");
        const u8 m = result.selected_shell_count;
        std::array<u8, kFacetMaxK> on{};
        for (u8 i = 0; i < m; ++i) on[i] = boundary[i];
        std::sort(on.begin(), on.begin() + m);
        finished = false;
        for (u8 a = 0; a < m && !finished; ++a)
          for (u8 b = a + 1; b < m && !finished; ++b)
            for (u8 c = b + 1; c < m && !finished; ++c)
              finished = attempt({on[a], on[b], on[c], 0}, 3);
        for (u8 a = 0; a < m && !finished; ++a)
          for (u8 b = a + 1; b < m && !finished; ++b)
            for (u8 c = b + 1; c < m && !finished; ++c)
              for (u8 d = c + 1; d < m && !finished; ++d)
                finished = attempt({on[a], on[b], on[c], on[d]}, 4);
        if (finished) return result;
      }
    }
    if (!anchor_meb_detail::charge(work.proposal_fallbacks))
      return failure(AnchorMebStatus::kCounterOverflow, "anchor_meb_proposals_overflow");
    finished = false;
  }
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

inline AnchorMebResult anchor_meb(std::span<const P3> sites, AnchorMebWork& work) noexcept {
  return anchor_meb_impl(sites, work, false);
}

inline AnchorMebResult anchor_meb_proposed(std::span<const P3> sites, AnchorMebWork& work) noexcept {
  return anchor_meb_impl(sites, work, true);
}

}  // namespace mhgp9::tower
