#pragma once

// Private selection primitive only. No terminal lookup, intruder or FULL path.
#include "../forest/anchor_meb.hpp"
#if defined(__CUDACC__)
#include <cuda_runtime.h>
#endif

namespace mhgp7::gpu_meb_private {

enum class Status : u8 { kOk = 0, kInvalidInput = 1, kInvariant = 2, kUnwritten = 0xee };
inline constexpr u32 kSkipContainment = 1, kDiscardFirst = 2, kShellIsSupport = 4, kSkipWrite = 8;
inline constexpr u32 kSkipPositive = 16;  // private causal test, never a product option
struct Request {
  u32 ordinal = 0;
  u8 count = 0;
  i32 sites[kFacetMaxK]{};
};
struct Selection {
  u32 ordinal = 0;
  Status status = Status::kUnwritten;
  u8 q = 0, shell = 0, slots[4]{};
  u64 calls = 0, supports[5]{}, powers = 0;
};

MHGP7_HD inline bool valid_input(const P3* positions, u64 position_count, const Request& request) {
  if (request.count == 0 || request.count > kFacetMaxK || positions == nullptr) return false;
  for (u8 i = 0; i < request.count; ++i) {
    const i32 index = request.sites[i];
    if (index < 0 || static_cast<u64>(index) >= position_count) return false;
    const P3 p = positions[index];
    if (!p3_in_profile(p)) return false;
    for (u8 j = 0; j < i; ++j)
      if (request.sites[j] == index || positions[request.sites[j]] == p) return false;
  }
  return true;
}

struct Candidate {
  P3 a{}, b{};
  Q3Form three{};
  Q4Form four{};
  u8 q = 0;
  MHGP7_HD i128 power(const P3& point) const {
    if (q == 2) return p3_dot(p3_sub(point, a), p3_sub(point, b));
    if (q == 3) return q3_power(three, point);
    return q4_power(four, point);
  }
};

MHGP7_HD inline bool form(const P3* sites, const u8 slots[4], u8 q, Candidate& candidate, u32 flags = 0) {
  candidate.q = q; candidate.a = sites[slots[0]]; candidate.b = sites[slots[1]];
  if (q == 2) return true;
  const P3 a = candidate.a, b = candidate.b, c = sites[slots[2]];
  if (q == 3) {
    if (!(flags & kSkipPositive) && (p3_dot(p3_sub(b, a), p3_sub(c, a)) <= 0 ||
        p3_dot(p3_sub(a, b), p3_sub(c, b)) <= 0 ||
        p3_dot(p3_sub(a, c), p3_sub(b, c)) <= 0)) return false;
    candidate.three = q3_form(a, b, c);
    return candidate.three.g > 0;
  }
  const P3 d = sites[slots[3]];
  candidate.four = q4_form(a, b, c, d);
  return candidate.four.det > 0 && ((flags & kSkipPositive) || q4_center_strictly_inside(candidate.four, a, b, c, d));
}

MHGP7_HD inline bool attempt(const P3* sites, u8 count, const u8 slots[4], u8 q,
    Selection& result, u32 flags, bool& discarded) {
  ++result.supports[q];
  Candidate candidate;
  if (!form(sites, slots, q, candidate, flags)) return false;
  u8 shell = 0;
  if (!(flags & kSkipContainment)) {
    for (u8 i = 0; i < count; ++i) {
      ++result.powers;
      const i128 power = candidate.power(sites[i]);
      if (power > 0) return false;
      if (power == 0) ++shell;
    }
  } else shell = q;
  if ((flags & kDiscardFirst) && !discarded) { discarded = true; return false; }
  result.q = q; result.shell = flags & kShellIsSupport ? q : shell;
  for (u8 i = 0; i < 4; ++i) result.slots[i] = slots[i];
  result.status = Status::kOk;
  return true;
}

// Counters are per request, initially zero. At K<=10 there are at most 375
// supports and 3750 power tests: no device counter can wrap. Aggregation into
// a long-lived host counter is a separate checked operation, not hidden here.
MHGP7_HD inline Selection select_one(const P3* positions, u64 position_count,
    const Request& request, u32 flags = 0) {
  Selection result;
  result.ordinal = request.ordinal; result.calls = 1;
  result.status = Status::kInvalidInput;
  if (!valid_input(positions, position_count, request)) return result;
  P3 sites[kFacetMaxK];
  for (u8 i = 0; i < request.count; ++i) sites[i] = positions[request.sites[i]];
  if (request.count == 1) {
    result.status = Status::kOk; result.q = result.shell = 1; result.supports[1] = 1;
    return result;
  }
  result.status = Status::kInvariant;
  bool discarded = false;
  const u8 n = request.count;
  for (u8 a = 0; a < n; ++a) for (u8 b = a + 1; b < n; ++b) {
    const u8 slots[4]{a, b, 0, 0};
    if (attempt(sites, n, slots, 2, result, flags, discarded)) return result;
  }
  for (u8 a = 0; a < n; ++a) for (u8 b = a + 1; b < n; ++b)
    for (u8 c = b + 1; c < n; ++c) {
      const u8 slots[4]{a, b, c, 0};
      if (attempt(sites, n, slots, 3, result, flags, discarded)) return result;
    }
  for (u8 a = 0; a < n; ++a) for (u8 b = a + 1; b < n; ++b)
    for (u8 c = b + 1; c < n; ++c) for (u8 d = c + 1; d < n; ++d) {
      const u8 slots[4]{a, b, c, d};
      if (attempt(sites, n, slots, 4, result, flags, discarded)) return result;
    }
  return result;
}

MHGP7_HD inline void dispatch_one(const P3* positions, u64 position_count,
    const Request* requests, u32 request_count, Selection* output, u64 gid, u32 flags = 0) {
  if (gid >= request_count || ((flags & kSkipWrite) && gid == 0)) return;
  output[gid] = select_one(positions, position_count, requests[gid], flags);
}

#if defined(__CUDACC__)
__global__ void select_kernel(const P3* positions, u64 position_count,
    const Request* requests, u32 request_count, Selection* output, u32 flags) {
  const u64 gid = static_cast<u64>(blockIdx.x) * blockDim.x + threadIdx.x;
  dispatch_one(positions, position_count, requests, request_count, output, gid, flags);
}
#endif

inline void select_stub(const P3* positions, u64 position_count,
    const Request* requests, u32 request_count, Selection* output, u32 flags = 0) {
  for (u64 gid = 0; gid < static_cast<u64>(request_count) + 3; ++gid)
    dispatch_one(positions, position_count, requests, request_count, output, gid, flags);
}

inline u64 choose(unsigned n, unsigned q) {
  if (q > n) return 0;
  u64 result = 1;
  for (unsigned i = 1; i <= q; ++i) result = result * (n + 1 - i) / i;
  return result;
}
struct Materialized {
  AnchorMebResult value;
  // Populated only for successful materializations. On ANY failure the raw
  // Selection remains the only record of selection work already paid.
  AnchorMebWork selection_work{};
  // Validation is EXTRA host work, never charged as if the kernel paid it.
  u64 host_validation_power_tests = 0;
};

// Validate every output field before indexing positions or using a slot.
// This is not a transport wire/ABI contract and does not execute a new MEB
// enumeration: materialization checks only the support selected by the kernel.
// A later positive support can certify the same MEB. Rank bounds do not prove
// that an unobserved enumeration prefix was executed or rejected correctly.
#if defined(MHGP7_MEB_KEY_FORBID_LEGACY_HOST_MATERIALIZE)
inline Materialized materialize(const P3*, u64, const Request&, const Selection&) = delete;
#else
inline Materialized materialize(const P3* positions, u64 position_count,
    const Request& request, const Selection& selected) {
  Materialized out;
  out.value.status = AnchorMebStatus::kInvariantViolated;
  out.value.reason = "gpu_meb_invalid_output";
  if (selected.ordinal != request.ordinal || selected.calls != 1 || selected.supports[0] != 0)
    return out;
  if (selected.status != Status::kOk) {
    if (selected.status != Status::kInvalidInput && selected.status != Status::kInvariant) return out;
    if (selected.q || selected.shell) return out;
    for (u8 slot : selected.slots) if (slot) return out;
    if (selected.status == Status::kInvalidInput) {
      if (selected.powers) return out;
      for (u64 count : selected.supports) if (count) return out;
      out.value.status = AnchorMebStatus::kInvalidInput;
      out.value.reason = "gpu_meb_invalid_input";
    }
    return out;
  }
  if (!valid_input(positions, position_count, request) || selected.q < 1 || selected.q > 4 ||
      selected.q > request.count || selected.shell < selected.q || selected.shell > request.count ||
      (request.count == 1) != (selected.q == 1)) return out;
  for (u8 i = 0; i < 4; ++i) {
    if (i < selected.q) {
      if (selected.slots[i] >= request.count || (i && selected.slots[i - 1] >= selected.slots[i])) return out;
    } else if (selected.slots[i] != 0) return out;
  }
  u64 rank = 0;
  for (unsigned i = 0; i < selected.q; ++i) {
    const unsigned begin = i == 0 ? 0 : selected.slots[i - 1] + 1;
    for (unsigned v = begin; v < selected.slots[i]; ++v)
      rank += choose(request.count - v - 1, selected.q - i - 1);
  }
  u64 attempted = 0;
  for (unsigned q = 1; q <= 4; ++q) {
    const u64 expected = q == selected.q ? rank + 1 :
        (q >= 2 && q < selected.q ? choose(request.count, q) : 0);
    if (selected.supports[q] != expected) return out;
    attempted += expected;
  }
  if (selected.q == 1 ? selected.powers != 0 :
      (selected.powers < request.count || selected.powers > attempted * request.count)) return out;
  P3 sites[kFacetMaxK];
  for (u8 i = 0; i < request.count; ++i) sites[i] = positions[request.sites[i]];
  AnchorMebResult result;
  if (selected.q == 1) {
    result.key = q2_ball_key(sites[0], sites[0]);
    result.level = promote_level(Rational128{0, 1});
  } else {
    std::array<u8, 4> slots{};
    for (u8 i = 0; i < 4; ++i) slots[i] = selected.slots[i];
    anchor_meb_detail::Candidate candidate;
    if (!anchor_meb_detail::form(std::span<const P3>(sites, request.count), slots, selected.q, candidate)) return out;
    u8 shell = 0;
    for (u8 i = 0; i < request.count; ++i) {
      ++out.host_validation_power_tests;
      const auto power = candidate.power(sites[i]);
      if (power > 0) return out;
      if (power == 0) ++shell;
    }
    if (shell != selected.shell) return out;
    if (selected.q == 2) {
      result.key = q2_ball_key(candidate.a, candidate.b);
      result.level = promote_level(q2_exact_level(p3_norm2(p3_sub(candidate.a, candidate.b))));
    } else if (selected.q == 3) {
      result.key = q3_ball_key(candidate.three);
      result.level = promote_level(q3_exact_level(candidate.a, candidate.b, sites[slots[2]]));
    } else {
      result.key = ball_key_reduce(q4_ball_form(candidate.four));
      result.level = q4_level_raw(candidate.four);
    }
  }
  out.selection_work.calls = selected.calls;
  for (unsigned q = 0; q < 5; ++q) out.selection_work.supports_by_size[q] = selected.supports[q];
  out.selection_work.power_tests = selected.powers;
  out.selection_work.materializations = 1;  // one actual host materialization
  result.status = AnchorMebStatus::kOk; result.reason = "gpu_meb_host_materialized";
  result.support_size = selected.q; result.selected_shell_count = selected.shell;
  for (u8 i = 0; i < 4; ++i) result.support_slots[i] = selected.slots[i];
  out.value = result;
  return out;
}
#endif

}  // namespace mhgp7::gpu_meb_private
