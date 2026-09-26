#pragma once

// AUDIT ONLY. This header includes the product unchanged. It does not shadow
// gpu::q3_census_range and is not linked into any product executable.
#include "../../src/gpu/lanes.hpp"

namespace mhgp9::audit_q3_payload {
using namespace gpu;

struct InteriorIds { u32 ids[8]; };
static_assert(sizeof(InteriorIds) == 32);
struct PayloadSlab {
  u32* scratch;          // 8 SHARED words per group, alive for the whole call
  InteriorIds* records;  // exactly slab.record_capacity slots
};
struct PayloadWork {
  u64 scratch_writes = 0, rejected_scratch_writes = 0;
  u64 output_writes = 0, payload_syncs = 0;
};

// Only complete non-saturating chunks can enlarge the candidate payload.
// A rejected seed never publishes a slot, even after earlier chunks wrote
// scratch. Accepted q3 has depth < K-1 <= 9, hence at most 8 original IDs.
// Group::for_set and sync match HostGroup AND the product WarpGroup contract;
// scratch must be shared, not a thread-private array. No full-census replay.
template <class Group>
MHGP9_HD CertificateStatus census_range(
    const Group& group, const LanesIndex& index, u32 a_rank, u32 b_rank,
    unsigned kmax, const LanesSlab& slab, const PayloadSlab& payload,
    u32 sites, u32 first, u32 last, u32& record_count,
    EdgeQ3Work& local, PayloadWork& work) {
  if (kmax < 2 || kmax > 10) return CertificateStatus::fault;
  const auto* a = index.tree.rank_points + 3 * static_cast<std::size_t>(a_rank);
  const auto* b = index.tree.rank_points + 3 * static_cast<std::size_t>(b_rank);
  const u32 id_a = index.rank_ids[a_rank], id_b = index.rank_ids[b_rank];
  const u32 threshold = kmax - 1;
  for (u32 i = first; i < last; ++i) {
    const auto* x = slab.points + 3 * static_cast<std::size_t>(slab.seeds[i]);
    Q3Form form{};
    if (!q3_form(a, b, x, form)) return CertificateStatus::fault;
    Q3Census c{0, 0, 0, 0, 0, false};
    u32 pending = 0;
    for (u32 base = 0; base < sites && !c.rejected; base += Group::size) {
      u32 inside = 0, zero = 0;
      group.ballot2(base, sites, [&](u32 t) -> u32 {
        const i128 power = q3_power(form, a, slab.points + 3 * static_cast<std::size_t>(t));
        return (power < 0 ? 1U : 0U) | (power == 0 ? 2U : 0U);
      }, inside, zero);
      const u32 add = popcount32(inside);
      if (add < threshold - c.depth && add != 0) {
        group.for_set(inside, [&](u32 lane, u32 order) {
          const u32 rank = slab.ranks[base + lane];
#if defined(AUDIT_Q3_PAYLOAD_MUTANT_RANK_AS_ID)
          payload.scratch[c.depth + order] = rank;
#else
          payload.scratch[c.depth + order] = index.rank_ids[rank];
#endif
        });
        pending += add;
        work.scratch_writes += add;
        // Writers in different lanes may become readers of this slot at
        // emission (i = lane in for_each below); establish visibility now.
        group.sync();
        ++work.payload_syncs;
      }
      q3_census_chunk(group, index, slab, sites, threshold, base, inside, zero, c);
    }
    q3_census_count(c, local);
    if (c.rejected) {
      ++local.depth_rejections;
      work.rejected_scratch_writes += pending;
      continue;
    }
    if (record_count == slab.record_capacity) return CertificateStatus::deferred;
    if (group.leader())
      q3_record(form, a, id_a, id_b, index.rank_ids[slab.ranks[slab.seeds[i]]], c,
                slab.records[record_count]);
    group.for_each(8, [&](u32 j) {
      const bool present = j < c.depth;
#if defined(AUDIT_Q3_PAYLOAD_MUTANT_OMIT_LAST)
      payload.records[record_count].ids[j] = present && j + 1 < c.depth ? payload.scratch[j] : absent32;
#else
      payload.records[record_count].ids[j] = present ? payload.scratch[j] : absent32;
#endif
    });
    work.output_writes += 8;
    ++record_count;
    ++local.emitted;
    local.shell_ids += c.shell;
    // Prevent the next seed from overwriting shared scratch while another
    // lane is still reading it. The product's end-of-range sync is too late.
    group.sync();
    ++work.payload_syncs;
  }
  group.sync();
  return CertificateStatus::decided;
}
}  // namespace mhgp9::audit_q3_payload
