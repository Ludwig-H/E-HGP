#pragma once

// MorseHGP3D v9 (23 septembre 2026) — device run of the exact q3/q4 witness
// filter (gpu/witness_filter.hpp) on a WSPD rectangle population: one thread
// per rectangle, an exclusive scan of the surviving pair masses, then one
// thread per expanded pair (row-major a x b in rectangle order, no cache).
// Plain host types only: this header is shared by the C++20 host probe and
// the CUDA translation unit.

#include "certificate.hpp"
#include "lanes_tasks.hpp"
#include "q4_lanes.hpp"
#include "witness_filter.hpp"
#include "../common/raw_vector.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mhgp9::gpu {

struct FilterInput {
  const FlatNode* nodes = nullptr;
  std::size_t node_count = 0;
  const std::int32_t* rank_points = nullptr;  // 3 coordinates per spatial rank
  std::size_t rank_count = 0;
  const u32* rect_a = nullptr;  // node ids
  const u32* rect_b = nullptr;
  const u8* rect_mask = nullptr;  // front lane masks (subset of 6)
  std::size_t rect_count = 0;
  unsigned kmax = 0;
  unsigned repeats = 1;  // timed repetitions of the whole device pass
};

struct FilterOutput {
  bool available = false;  // false: built without CUDA or no device
  std::string device;
  std::string error;  // non-empty: CUDA failure, nothing below is valid
  bool stack_failure = false;
  std::vector<u8> rect_masks;
  std::vector<u8> pair_masks;  // surviving rectangles only, row-major
  std::uint64_t pairs = 0, rect_visits = 0, pair_visits = 0;
  // Best of `repeats` device passes (ms), cudaEvent timings.
  double upload_ms = 0, rect_ms = 0, scan_ms = 0, pair_ms = 0, download_ms = 0, total_ms = 0;
  double first_total_ms = 0;  // the first pass, cold
};

// Host-side refusal of a raw input before any device call (contre-audits B
// 13 h 05 and A « domaine u18 ») ; empty string when accepted. The device
// arithmetic is proved only for u18 coordinates, and a box credits or
// excludes every site of its rank range: a forged box that misses one of
// its points makes a false rejection (A's fixture: q4 mask 4 -> 0). Checked:
// K in 1..10 (the filter derives the available lanes); coordinates and box bounds in 0..262143 with low <= high;
// children partition their parent's rank range exactly; every box contains
// the points of its range (O(n * depth)); rectangle ids, lane masks subset
// of 6, and CUB's int item count. Tight hulls are not required: a larger
// box only weakens both certificates.
inline std::string validate_filter_input(const FilterInput& input) {
  constexpr std::int32_t limit = 262143;
  if (input.kmax < 1 || input.kmax > 10) return "kmax outside 1..10";
  if (input.nodes == nullptr || input.node_count == 0 || input.node_count >= absent32) return "empty or huge node array";
  if (input.rank_points == nullptr || input.rank_count == 0 || input.rank_count >= absent32) return "empty rank array";
  if (input.rect_count > static_cast<std::size_t>(0x7fffffff)) return "rectangle count exceeds the CUB int range";
  if (input.rect_count != 0 && (input.rect_a == nullptr || input.rect_b == nullptr || input.rect_mask == nullptr))
    return "null rectangle arrays";
  for (std::size_t i = 0; i < 3 * input.rank_count; ++i)
    if (input.rank_points[i] < 0 || input.rank_points[i] > limit) return "point coordinate outside the u18 domain";
  const FlatNode& root = input.nodes[0];
  if (root.first != 0 || root.last != input.rank_count) return "root does not cover every rank";
  for (std::size_t i = 0; i < input.node_count; ++i) {
    const FlatNode& node = input.nodes[i];
    if (node.first >= node.last || node.last > input.rank_count) return "node rank range outside the index";
    for (int axis = 0; axis < 3; ++axis)
      if (node.box.low[axis] < 0 || node.box.low[axis] > node.box.high[axis] || node.box.high[axis] > limit)
        return "node box outside the u18 domain or inverted";
    const bool leaf = node.left == absent32;
    if (leaf != (node.right == absent32)) return "node with one child";
    if (!leaf) {
      if (node.left >= input.node_count || node.right >= input.node_count || node.left <= i || node.right <= i)
        return "child id outside the index or not after its parent";
      const FlatNode& left = input.nodes[node.left];
      const FlatNode& right = input.nodes[node.right];
      if (left.first != node.first || left.last != right.first || right.last != node.last)
        return "children do not partition their parent's ranks";
    }
    for (u32 r = node.first; r < node.last; ++r)
      for (int axis = 0; axis < 3; ++axis) {
        const std::int32_t c = input.rank_points[3 * static_cast<std::size_t>(r) + axis];
        if (c < node.box.low[axis] || c > node.box.high[axis]) return "node box misses a point of its ranks";
      }
  }
  for (std::size_t i = 0; i < input.rect_count; ++i)
    if (input.rect_a[i] >= input.node_count || input.rect_b[i] >= input.node_count || (input.rect_mask[i] & ~6U) != 0)
      return "rectangle node id or lane mask outside the domain";
  return {};
}

// Defined in filter_runner.cu when MHGP9_ENABLE_CUDA, else a stub that
// returns available=false. Refuses (error) any input validate_filter_input
// refuses, before touching the device.
FilterOutput run_filters(const FilterInput& input);

// S2 (chain): one device pass, then only the surviving pairs come back,
// compacted on the device in rectangle order (row-major inside each
// rectangle), with their spatial ranks and lanes; plus the rectangle masks
// and the lane rejections among the expanded pairs. Same validation and
// kernels as run_filters; `repeats` is ignored (one pass).
// Why a batch pass produced no answer: the input guard refused it, no device
// (or a build without CUDA), a capacity limit (CUB int range, device memory,
// host allocation), or a fault of the pass on a present device.
enum class BatchError : u8 { none, input_guard, no_device, capacity, device_fault };

struct BatchOutput {
  bool available = false;
  std::string device;
  std::string error;  // non-empty: nothing below is valid
  BatchError error_kind = BatchError::none;
  bool stack_failure = false;
  std::vector<u8> rect_masks;
  std::vector<u32> survivor_a, survivor_b;  // spatial ranks
  std::vector<u8> survivor_mask;
  std::uint64_t pairs = 0, pair_q3_rejected = 0, pair_q4_rejected = 0;
  std::uint64_t rect_visits = 0, pair_visits = 0;
  // cudaEvent timings of the pass (ms): upload, rectangle kernel, mass scan,
  // pair kernel, survivor compaction, download of masks and survivors.
  double upload_ms = 0, rect_ms = 0, scan_ms = 0, pair_ms = 0, select_ms = 0, download_ms = 0, total_ms = 0;
};
BatchOutput run_filter_batch(const FilterInput& input);

// ---- S3 (chain): dead-lane certificates of surviving edges ------------
//
// One warp per edge (gpu/certificate.hpp): the diametral core and its
// certificate, then the cover and its certificate for the lanes left open,
// exactly as Engine::filtered_edge. Each warp owns a slab of `capacity`
// sites; an edge whose core or cover exceeds it comes back DEFERRED (status
// 1) with no counter, and the CPU runs its whole engine edge.
struct CertificateInput {
  FilterInput index;       // nodes, rank points and kmax (rectangle fields unused)
  const u32* escapes = nullptr;  // preorder escape links, node_count at the end
  const u32* edge_a = nullptr;   // spatial ranks
  const u32* edge_b = nullptr;
  const u8* edge_mask = nullptr;  // surviving lanes (nonzero subset of 6)
  std::size_t edge_count = 0;
  bool dead_core = false;
  u32 capacity = 0;  // sites per warp slab; 0 selects the default
};

inline constexpr u32 default_certificate_capacity = 1U << 16;

// Host-side refusal before any device call; empty when accepted. The index
// part is validate_filter_input; the escape links must be exactly those of
// the preorder tree (root: node_count; a leaf: the next node; children: the
// right sibling, then the parent's escape), so every walk strictly advances
// and ends. Edges: ranks inside the index, distinct, lanes a nonzero subset
// of 6; at most 2^31-1 edges; capacity 0 or >= 2.
inline std::string validate_certificate_input(const CertificateInput& input) {
  FilterInput index = input.index;
  index.rect_count = 0;
  if (auto error = validate_filter_input(index); !error.empty()) return error;
  if (input.escapes == nullptr) return "null escape links";
  const std::size_t n = index.node_count;
  if (input.escapes[0] != n) return "root escape is not the end of the index";
  for (std::size_t i = 0; i < n; ++i) {
    const FlatNode& node = index.nodes[i];
    if (input.escapes[i] <= i || input.escapes[i] > n) return "escape link does not advance inside the index";
    if (node.left == absent32) {
      // A leaf's subtree is itself: its escape is the next preorder node. The
      // cover walk tests a leaf as ONE point (auditor B): a multi-rank leaf
      // with an ambiguous box would be split into nothing and lose sites.
      if (input.escapes[i] != i + 1) return "leaf escape is not the next preorder node";
      if (node.last - node.first != 1) return "leaf with more than one rank";
      continue;
    }
    if (node.left != i + 1) return "left child does not follow its parent in preorder";
    if (input.escapes[node.left] != node.right || input.escapes[node.right] != input.escapes[i])
      return "child escape links differ from the preorder structure";
  }
  if (input.edge_count > static_cast<std::size_t>(0x7fffffff)) return "edge count exceeds 2^31-1";
  if (input.edge_count != 0 && (input.edge_a == nullptr || input.edge_b == nullptr || input.edge_mask == nullptr))
    return "null edge arrays";
  // Lanes available at this K (auditor B): none at K1, q3 only at K2.
  const unsigned lanes = index.kmax >= 3 ? 6U : (index.kmax == 2 ? 2U : 0U);
  for (std::size_t i = 0; i < input.edge_count; ++i)
    if (input.edge_a[i] >= index.rank_count || input.edge_b[i] >= index.rank_count ||
        input.edge_a[i] == input.edge_b[i] || input.edge_mask[i] == 0 || (input.edge_mask[i] & ~lanes) != 0)
      return "edge rank or lane mask outside the domain of K";
  if (input.capacity == 1) return "slab capacity below two sites";
  return {};
}

struct CertificateOutput {
  bool available = false;
  std::string device;
  std::string error;  // non-empty: nothing below is valid
  BatchError error_kind = BatchError::none;
  std::vector<u8> masks;   // per edge: lanes left open (decided), the input mask otherwise
  std::vector<u8> status;  // per edge: CertificateStatus (0 decided, 1 deferred, 2 fault)
  CertificateWork work{};  // decided edges only
  std::uint64_t deferred = 0, faults = 0;
  std::uint32_t capacity = 0, warps = 0;
  // cudaEvent timings (ms): upload, kernel, download, whole pass.
  double upload_ms = 0, kernel_ms = 0, download_ms = 0, total_ms = 0;
};
CertificateOutput run_certificate_batch(const CertificateInput& input);

// ---- S4a/S4b (chain): q3 and q4 lanes of certified edges -----------------
//
// The lanes of each edge (gpu/lanes.hpp, q4_lanes.hpp): the cover rebuilt,
// the seeds drawn from it, one exact ScalarCover census per seed, the q4
// lens pass per seed, the key of every accepted ball. Since the S4b tasks
// (gpu/lanes_tasks.hpp, 24 septembre 2026) the call runs in three steps:
// P per edge (cover and seeds into a cover arena), T per (edge, seed range)
// task of at most `task_budget` seed-chunks (records into a staging arena),
// C per edge (replay of the sequential deferral rules, arena rule in edge
// order, gather in (phase, task) order). A decided edge's records are one
// slice of the output, the slices in edge order (never a prefix). An edge
// whose cover (`capacity` sites), q4 events (`event_capacity` per seed),
// records (`record_capacity` in all) or arena share (`arena_capacity`,
// edge order) exceed them comes back DEFERRED (status 1) with no counter:
// the CPU runs its lanes. A cover or staging arena overflow is an explicit
// capacity refusal of the whole call.
struct LanesInput {
  FilterInput index;              // nodes, rank points and kmax >= 2 (rectangle fields unused)
  const u32* escapes = nullptr;   // preorder escape links
  const u32* rank_ids = nullptr;  // original input ID of every spatial rank
  const u32* edge_a = nullptr;    // spatial ranks
  const u32* edge_b = nullptr;
  const u8* edge_lanes = nullptr;  // S4b: lanes asked per edge (2, 4 or 6); null: q3 only
  std::size_t edge_count = 0;
  u32 capacity = 0;                // sites per slab; 0 selects the default
  u32 record_capacity = 0;         // records per slab; 0 selects the default
  std::size_t arena_capacity = 0;  // records of the call; 0 selects the default
  u32 event_capacity = 0;          // S4b: buffered q4 events per seed; 0 selects the default
  // S4b tasks: B in seed-chunks (0: default_task_budget; single_task_budget:
  // one task per edge), the cover arena in sites (0: a sixth of the device's
  // total memory at 20 bytes a site, refused above half of the free memory;
  // no limit on the host) and the staging arena in records (0:
  // default_staging_capacity). None of them changes a decided output.
  u64 task_budget = 0;
  u64 cover_capacity = 0;
  u64 staging_capacity = 0;
};

inline constexpr u32 default_lanes_capacity = 1U << 16;
inline constexpr u32 default_record_capacity = 1U << 12;
inline constexpr u32 default_event_capacity = 1U << 12;
// Default arena: sixteen records per edge plus one slab (R15: about one q3
// ball per edge at K5, two at K10; with the q4 lane, 3.2 records per edge on
// LiDAR at K10 and up to 8 on the uniform family). The device clamps it to
// an eighth of the free memory; an overflow defers edges, never refuses.
inline std::size_t default_arena_capacity(std::size_t edges) { return 16 * edges + default_record_capacity; }
// Default staging arena of the tasks: twice the default arena (the staged
// records also hold those of edges deferred afterwards). The device clamps
// it to an eighth of the free memory (and the arena to half of that); an
// overflow defers every non-faulty edge of the call to the CPU tail, never
// refuses it (review before R18).
inline std::size_t default_staging_capacity(std::size_t edges) { return 2 * default_arena_capacity(edges); }

// Host-side refusal before any device call; empty when accepted: the index
// and escape links of validate_certificate_input, the rank IDs, K >= 2, edge
// ranks inside the index and distinct, capacities 0 or >= 2 sites and >= 1
// record.
inline std::string validate_lanes_input(const LanesInput& input) {
  CertificateInput shape;
  shape.index = input.index;
  shape.escapes = input.escapes;
  if (auto error = validate_certificate_input(shape); !error.empty()) return error;
  if (input.index.kmax < 2) return "q3 lanes require K >= 2";
  if (input.rank_ids == nullptr) return "null rank IDs";
  if (input.edge_count > static_cast<std::size_t>(0x7fffffff)) return "edge count exceeds 2^31-1";
  if (input.edge_count != 0 && (input.edge_a == nullptr || input.edge_b == nullptr)) return "null edge arrays";
  for (std::size_t i = 0; i < input.edge_count; ++i)
    if (input.edge_a[i] >= input.index.rank_count || input.edge_b[i] >= input.index.rank_count ||
        input.edge_a[i] == input.edge_b[i])
      return "edge rank outside the index";
  if (input.edge_lanes != nullptr)
    for (std::size_t i = 0; i < input.edge_count; ++i) {
      const u8 lanes = input.edge_lanes[i];
      if (lanes == 0 || (lanes & ~6U) != 0 || ((lanes & 4U) != 0 && input.index.kmax < 3))
        return "edge lanes outside the domain of K";
    }
  if (input.capacity == 1) return "slab capacity below two sites";
  if (input.arena_capacity > static_cast<std::size_t>(0xffffffffU)) return "record arena exceeds 2^32-1";
  if (input.staging_capacity > (u64{1} << 40)) return "staging arena exceeds 2^40 records";
  return {};
}

struct LanesOutput {
  bool available = false;
  std::string device;
  std::string error;  // non-empty: nothing below is valid
  BatchError error_kind = BatchError::none;
  std::vector<u8> status;                       // per edge: CertificateStatus
  std::vector<u32> record_begin, record_count;  // per edge (decided): its slice of records
  RawVector<LaneRecord> records;                // slices in edge order; edge = input edge index
  Q3Work work{};                                // decided edges only (prologue and q3 lanes)
  Q4Work work4{};                               // decided edges only (S4b q4 lanes)
  std::uint64_t deferred = 0, faults = 0;
  std::uint32_t capacity = 0, record_capacity = 0, warps = 0;
  // cudaEvent timings (ms): upload, kernel, download, whole pass.
  double upload_ms = 0, kernel_ms = 0, download_ms = 0, total_ms = 0;
  // v9 H1: host sub-timers outside the events: setup (entry to the first
  // event: guards, probe, sizing) and finish (last event to return: ledger
  // reduction, statuses).
  double setup_ms = 0, finish_ms = 0;
  // S4b tasks: the tasks of the call and the largest declared work of one
  // task (lanes_task_steps), both functions of the input and B; the three
  // steps' times (device: events, P with its scan, T with its table, C with
  // its scan; host: wall clock of each phase).
  std::uint64_t tasks = 0, max_task_steps = 0;
  double plan_ms = 0, task_ms = 0, compact_ms = 0;
};
LanesOutput run_lanes_batch(const LanesInput& input);
// Reserves the lanes call's resident per-warp slabs (v9 H1), to be called
// during q2; empty string on success. Any error is classified again by the
// batch call.
std::string warm_up_lanes(u32 capacity, u32 record_capacity, u32 event_capacity);

// Opens the device's primary context (process-wide), so that the first
// batch call does not pay it; empty string on success. Any error is left to
// the batch call itself, which classifies it.
std::string warm_up();

}  // namespace mhgp9::gpu
