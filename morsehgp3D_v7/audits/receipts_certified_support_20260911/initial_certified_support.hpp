#pragma once

// Audit-only helper. All predicates come from the frozen production header;
// there is no geometric oracle or proposal generator in this path.
#include "src/forest/anchor_meb.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <vector>

namespace certified_support_audit {

using namespace mhgp7;
using anchor_meb_detail::Candidate;

struct Counts {
  u64 attempts = 0;
  u64 positive_forms = 0;
  u64 powers = 0;
};

struct Work {
  Counts certification;
  Counts canonicalization;
  u64 skipped_support_intersections = 0;
  u64 direct_returns = 0;
};

enum class Mode { kAllSites, kCertifiedSupport };
enum class State { kNotCertified, kCanonicalFailure, kOk };

struct Outcome {
  State state = State::kNotCertified;
  const char* reason = "invalid_local_input";
  AnchorMebResult result;
};

inline AnchorMebResult materialize(std::span<const P3> sites,
                                   const Candidate& candidate, u8 shell_size) {
  AnchorMebResult result;
  result.support_size = candidate.q;
  result.support_slots = candidate.slots;
  result.selected_shell_count = shell_size;
  if (candidate.q == 2) {
    result.key = q2_ball_key(candidate.a, candidate.b);
    result.level = promote_level(q2_exact_level(p3_norm2(p3_sub(candidate.a, candidate.b))));
  } else if (candidate.q == 3) {
    result.key = q3_ball_key(candidate.three);
    result.level = promote_level(q3_exact_level(candidate.a, candidate.b,
                                                sites[candidate.slots[2]]));
  } else {
    result.key = ball_key_reduce(q4_ball_form(candidate.four));
    result.level = q4_level_raw(candidate.four);
  }
  result.status = AnchorMebStatus::kOk;
  result.reason = "anchor_meb_exact_local";
  return result;
}

// Counts are local to one bounded call. No existing work is overwritten.
// The proposal is only slots: no caller-supplied certification bit is trusted.
inline Outcome run(std::span<const P3> sites, std::array<u8, 4> support,
                   u8 support_size, Mode mode, Work& work) {
  Outcome outcome;
  if (sites.size() < 2 || sites.size() > static_cast<std::size_t>(kFacetMaxK) ||
      support_size < 2 || support_size > 4) return outcome;
  for (std::size_t i = 0; i < sites.size(); ++i) {
    if (!p3_in_profile(sites[i])) return outcome;
    for (std::size_t j = 0; j < i; ++j)
      if (sites[i] == sites[j]) return outcome;
  }
  for (u8 i = 0; i < support_size; ++i) {
    if (support[i] >= sites.size()) return outcome;
    if (i != 0 && support[i - 1] >= support[i]) return outcome;
  }
  ++work.certification.attempts;
  Candidate certified;
  if (!anchor_meb_detail::form(sites, support, support_size, certified)) {
    outcome.reason = "proposal_support_not_positive";
    return outcome;
  }
  ++work.certification.positive_forms;
  std::vector<u8> shell;
  for (u8 i = 0; i < sites.size(); ++i) {
    ++work.certification.powers;
    const i128 power = certified.power(sites[i]);
    if (power > 0) {
      outcome.reason = "proposal_not_enclosing";
      return outcome;
    }
    if (power == 0) shell.push_back(i);
  }
  // Formation fixes the support on the boundary; retain this executable guard
  // so the precondition S subset U is explicit at this interface.
  for (u8 i = 0; i < support_size; ++i) {
    if (std::find(shell.begin(), shell.end(), support[i]) == shell.end()) {
      outcome.reason = "proposal_support_off_shell";
      return outcome;
    }
  }
  if (mode == Mode::kCertifiedSupport && shell.size() == support_size) {
    ++work.direct_returns;
    outcome.result = materialize(sites, certified, static_cast<u8>(shell.size()));
    outcome.state = State::kOk;
    outcome.reason = "certified_shell_equals_support";
    return outcome;
  }
  outcome.state = State::kCanonicalFailure;
  outcome.reason = "no_canonical_support";
  const auto attempt = [&](std::array<u8, 4> slots, u8 q) {
    ++work.canonicalization.attempts;
    Candidate candidate;
    if (!anchor_meb_detail::form(sites, slots, q, candidate)) return false;
    ++work.canonicalization.positive_forms;
    if (mode == Mode::kAllSites) {
      for (const auto& point : sites) {
        ++work.canonicalization.powers;
        if (candidate.power(point) > 0) return false;
      }
    } else {
      for (u8 i = 0; i < support_size; ++i) {
        const auto slot = support[i];
        if (std::find(slots.begin(), slots.begin() + q, slot) != slots.begin() + q) {
          ++work.skipped_support_intersections;
          continue;
        }
        ++work.canonicalization.powers;
        if (candidate.power(sites[slot]) > 0) return false;
      }
    }
    // The entire certified shell is retained; it is not reduced to S or T.
    outcome.result = materialize(sites, candidate, static_cast<u8>(shell.size()));
    outcome.state = State::kOk;
    outcome.reason = "certified_support_canonicalized";
    return true;
  };
  const auto n = static_cast<u8>(shell.size());
  for (u8 a = 0; a < n; ++a)
    for (u8 b = a + 1; b < n; ++b)
      if (attempt({shell[a], shell[b], 0, 0}, 2)) return outcome;
  for (u8 a = 0; a < n; ++a)
    for (u8 b = a + 1; b < n; ++b)
      for (u8 c = b + 1; c < n; ++c)
        if (attempt({shell[a], shell[b], shell[c], 0}, 3)) return outcome;
  for (u8 a = 0; a < n; ++a)
    for (u8 b = a + 1; b < n; ++b)
      for (u8 c = b + 1; c < n; ++c)
        for (u8 d = c + 1; d < n; ++d)
          if (attempt({shell[a], shell[b], shell[c], shell[d]}, 4)) return outcome;
  return outcome;
}

inline bool same(const AnchorMebResult& a, const AnchorMebResult& b) {
  return a.status == b.status && a.key == b.key && same_exact_level(a.level, b.level) &&
         a.support_size == b.support_size && a.support_slots == b.support_slots &&
         a.selected_shell_count == b.selected_shell_count;
}

}  // namespace certified_support_audit
