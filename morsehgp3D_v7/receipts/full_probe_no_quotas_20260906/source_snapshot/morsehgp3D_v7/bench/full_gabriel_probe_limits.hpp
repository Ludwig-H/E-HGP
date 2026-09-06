// Bench-only v5 admission policy. No arithmetic-work quotas: UINT64_MAX is
// the representable counter ceiling, still guarded before each increment.
// Storage limits are derived from named logical arena sizes and addressable
// element counts, not allocator capacity/RSS or a sum over resident arenas.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <utility>

#include "../src/core/caps.hpp"
#include "../src/forest/full_gabriel.hpp"

namespace mhgp7::full_probe_limits {

inline constexpr const char* kProfile = "memory_guarded_no_operation_quotas_v1";
inline constexpr const char* kStorageKind = "per_named_logical_arena_and_representation";
inline constexpr u64 kCounterCeiling = std::numeric_limits<u64>::max();
inline constexpr u64 kSizeMax = std::numeric_limits<size_t>::max();
inline constexpr u64 kDifferenceMax = std::numeric_limits<std::ptrdiff_t>::max();
using AliasEntryPayload = std::pair<const FacetKey, FullNodeId>;
static_assert(sizeof(size_t) <= sizeof(u64));
static_assert(sizeof(std::ptrdiff_t) <= sizeof(u64));

template <class T> constexpr u64 elements(u64 bytes) {
  return std::min({bytes / sizeof(T), kSizeMax / sizeof(T), kDifferenceMax / sizeof(T)});
}

inline constexpr bool product_sum(u64 n, u64 a, u64 m, u64 b, u64& result) {
  if (mul_would_overflow_u64(n, a) || mul_would_overflow_u64(m, b)) return false;
  const u64 first = n * a, second = m * b;
  if (second > kCounterCeiling - first) return false;
  result = first + second;
  return true;
}

struct Policy {
  FullGabrielLimits full;
  u64 max_cache_entries = 0, max_read_point_refs = 0;
  u64 max_digest_scratch_logical_bytes = 0;
  bool valid = false;
};

inline Policy make(u64 bytes) {
  Policy p;
  auto& c = p.full;
  c.max_points = std::min(elements<InputPoint>(bytes), kMaxTreePositions);
  c.max_input_records = std::min(elements<ForestEvent>(bytes), elements<full_gabriel_detail::Record>(bytes));
  c.max_aliases = elements<AliasEntryPayload>(bytes);
  c.certificate.max_batches = elements<FullBatch>(bytes);
  c.certificate.max_nodes = std::min({elements<FullNode>(bytes), elements<FacetKey>(bytes),
                                    elements<FullNodeId>(bytes), elements<u8>(bytes)});
  c.certificate.max_parent_refs = elements<FullNodeId>(bytes);
  c.max_face_visits = c.max_portal_requests = c.max_chain_steps = kCounterCeiling;
  c.max_meb_calls = c.max_query_nodes = c.max_meb_supports = kCounterCeiling;
  c.max_successor_steps = kCounterCeiling;
  p.max_cache_entries = c.max_aliases;
  p.max_read_point_refs = elements<PointId>(bytes);
  p.valid = product_sum(c.certificate.max_nodes, 3 * sizeof(FullNodeId) + sizeof(u8),
                       c.certificate.max_parent_refs, sizeof(FullNodeId),
                       p.max_digest_scratch_logical_bytes);
  return p;
}

// Strict unsigned decimal, including UINT64_MAX; no signed intermediary,
// whitespace, exponent, sign or wrap. Output is unchanged on failure.
inline bool decimal(const char* text, u64& output) {
  if (text == nullptr || *text == '\0') return false;
  u64 value = 0;
  for (const char* p = text; *p != '\0'; ++p) {
    if (*p < '0' || *p > '9') return false;
    const u64 digit = static_cast<u64>(*p - '0');
    if (value > kCounterCeiling / 10 ||
        (value == kCounterCeiling / 10 && digit > kCounterCeiling % 10)) return false;
    value = value * 10 + digit;
  }
  output = value;
  return true;
}

enum class ProposalKind { kDisabled, kFinite, kUnlimited };

inline const char* proposal_kind(ProposalKind kind) {
  switch (kind) {
    case ProposalKind::kDisabled: return "disabled";
    case ProposalKind::kFinite: return "finite";
    case ProposalKind::kUnlimited: return "unlimited";
  }
  return "invalid";
}

inline bool proposal(const char* text, u64& output, ProposalKind& kind) {
  u64 value = 0;
  if (text != nullptr && std::strcmp(text, "unlimited") == 0) {
    output = kCounterCeiling;
    kind = ProposalKind::kUnlimited;
    return true;
  }
  if (!decimal(text, value)) return false;
  output = value;
  kind = value == 0 ? ProposalKind::kDisabled : ProposalKind::kFinite;
  return true;
}

}  // namespace mhgp7::full_probe_limits
