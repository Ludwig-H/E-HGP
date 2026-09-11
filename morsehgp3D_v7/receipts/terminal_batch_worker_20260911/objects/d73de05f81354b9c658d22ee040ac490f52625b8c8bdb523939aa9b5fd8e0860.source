#pragma once

// Private geometry-only terminal. No DSU, temporal anchor or FULL publication.
#include "source/morsehgp3D_v7/src/gpu/anchor_meb_key.cuh"
#include "intruder.cuh"

namespace mhgp7::gpu_terminal_private {
namespace meb_key = gpu_meb_key_private;
namespace meb_selection = gpu_meb_private;
namespace spatial = gpu_intruder_private;

using BallId = u32;
inline constexpr BallId kAbsentBall = ~BallId{0};
struct LevelWords { u64 words[5]{}; };
MHGP7_HD inline LevelWords encode_level(const ExactLevel& level) {
  LevelWords result{{level.num[0], level.num[1], level.num[2], 0, 0}};
  meb_key::store_i128(result.words + 3, level.den); return result;
}
MHGP7_HD inline ExactLevel decode_level(const LevelWords& level) {
  return {{level.words[0], level.words[1], level.words[2]}, meb_key::load_i128(level.words + 3)};
}
MHGP7_HD inline int compare_keys(const meb_key::PrimitiveKeyWords& a, const meb_key::PrimitiveKeyWords& b) {
  for (u8 coefficient = 0; coefficient < 5; ++coefficient) {
    const u64 ah = a.words[2 * coefficient + 1] ^ (u64{1} << 63);
    const u64 bh = b.words[2 * coefficient + 1] ^ (u64{1} << 63);
    if (ah != bh) return ah < bh ? -1 : 1;
    const auto al = a.words[2 * coefficient], bl = b.words[2 * coefficient];
    if (al != bl) return al < bl ? -1 : 1;
  }
  return 0;
}
struct CatalogEntry {
  meb_key::PrimitiveKeyWords key;
  LevelWords level;
  u32 arity = 0, interior = 0, shell = 0, reserved = 0;
};
struct CatalogView {
  const CatalogEntry* entries = nullptr;
  const BallId* by_key = nullptr;
  u64 snapshot = 0;
  u32 count = 0;
};
struct View { spatial::IndexView index; CatalogView catalog; };
// Trusted complete populations from the SAME immutable index/catalogue.
// These are geometry indices, never PointId or a selected support alone.
struct Seed { i32 selected[10]{}; BallId target = kAbsentBall; u32 reserved = 0; };
struct SeedView {
  const Seed* entries = nullptr;
  u64 count = 0, snapshot = 0;
  u32 k = 0;
};
MHGP7_HD inline int compare_seed(const i32* a, const i32* b) {
  for (u8 i = 0; i < 10; ++i) {
    if (a[i] < b[i]) return -1;
    if (a[i] > b[i]) return 1;
  }
  return 0;
}
struct Request {
  u64 snapshot = 0, ordinal = 0;
  u32 k = 0;
  i32 selected[10]{};
  LevelWords before;
};
enum class Status : u32 { kOk = 0, kInvalidRequest, kInvalidView, kMebFailure, kCounterOverflow,
    kNotStrict, kCatalogLevelMismatch, kMissingWeakTerminal, kIntruderFailure,
    kRadiusIncreased, kEqualRadiusInvariant, kInvalidSeedView, kSeedNotStrict };
struct Work {
  u64 calls = 0, supports[5]{}, powers = 0, materializations = 0, level_materializations = 0;
  u64 key_lookups = 0, anchor_hits = 0;
  u64 intruder_queries = 0, intruder_nodes = 0, intruder_power_tests = 0, interior_ranges = 0;
  u64 same_radius_steps = 0, descending_steps = 0, max_chain_steps = 0;
  u64 axis_divisions = 0;
  u32 stack_peak = 0;
  u64 post_seed_queries = 0, post_seed_hits = 0, post_seed_terminals = 0;
};
struct Local {
  meb_key::SelectedBall ball;
  LevelWords level;
};
struct TraceRow {
  i32 sites[10]{};
  u32 k = 0;
  meb_selection::Selection selection;
  meb_key::PrimitiveKeyWords key;
  LevelWords level;
  i32 intruder = -1;
  BallId terminal = kAbsentBall;
};
// Bounded diagnostics ONLY: overflow records a missing trace suffix, never
// truncates the terminal search or changes its work/result.
struct TraceBuffer {
  TraceRow* rows = nullptr;
  u64 capacity = 0, written = 0;
  bool overflow = false;
};
struct Result {
  Status status = Status::kInvalidRequest;
  u64 ordinal = 0;
  BallId target = kAbsentBall;
  Work work;
};
static_assert(std::is_trivially_copyable_v<Request> && std::is_standard_layout_v<Request>);
static_assert(std::is_trivially_copyable_v<Result> && std::is_standard_layout_v<Result>);
static_assert(sizeof(LevelWords) == 40 && sizeof(CatalogEntry) == 136);

MHGP7_HD inline bool add(u64& target, u64 value = 1) {
  if (value > ~u64{0} - target) return false;
  target += value; return true;
}
MHGP7_HD inline bool valid_view(const View& view) {
  const auto& index = view.index;
  return index.snapshot && index.snapshot == view.catalog.snapshot && index.positions &&
      index.positions <= 0x7fffffffu && index.nodes == index.positions - 1 && index.positions3 &&
      (!index.nodes || (index.left && index.right && index.first && index.last && index.box6)) &&
      (index.nodes ? index.root == 0 : index.root == -1) &&
      (!view.catalog.count || (view.catalog.entries && view.catalog.by_key));
}
MHGP7_HD inline bool materialize_level(const P3* positions, const meb_key::SelectedBall& selected,
    LevelWords& output) {
  const auto q = selected.selected.q;
  P3 support[4]{};
  for (u8 i = 0; i < q; ++i) support[i] = positions[selected.selected.slots[i]];
  ExactLevel level{{0,0,0}, 1};
  if (q == 2) {
    level.num[0] = static_cast<u64>(p3_norm2(p3_sub(support[0], support[1]))); level.den = 4;
  } else if (q == 3) {
    const auto raw = q3_level_raw(support[0], support[1], support[2]);
    if (raw.num < 0 || raw.den <= 0) return false;
    level.num[0] = static_cast<u64>(static_cast<u128>(raw.num));
    level.num[1] = static_cast<u64>(static_cast<u128>(raw.num) >> 64); level.den = raw.den;
  } else if (q == 4) {
    level = q4_level_raw(q4_form(support[0], support[1], support[2], support[3]));
  } else if (q != 1) return false;
  if (level.den <= 0) return false;
  output = encode_level(level); return true;
}
MHGP7_HD inline bool meb(const View& view, const i32* sites, u32 k, Local& local, Result& result) {
  P3 positions[10]{};
  meb_selection::Request request;
  // The legacy primitive's ordinal is local to this ONE call. Global
  // terminal occurrence identity is u64 and remains in Request/Result.
  request.ordinal = 0; request.count = static_cast<u8>(k);
  for (u32 i = 0; i < k; ++i) {
    const u16* point = view.index.positions3 + 3ull * static_cast<u32>(sites[i]);
    positions[i] = {point[0], point[1], point[2]}; request.sites[i] = static_cast<i32>(i);
  }
  local.ball = meb_key::select_meb_key(positions, k, request);
  const auto& selected = local.ball.selected;
  if (!add(result.work.calls, selected.calls) || !add(result.work.powers, selected.powers) ||
      !add(result.work.materializations, local.ball.key_materializations)) {
    result.status = Status::kCounterOverflow; return false;
  }
  for (u8 q = 0; q < 5; ++q) if (!add(result.work.supports[q], selected.supports[q])) {
    result.status = Status::kCounterOverflow; return false;
  }
  if (selected.status != meb_selection::Status::kOk || local.ball.key_status != meb_key::KeyStatus::kOk ||
      selected.q < 1 || selected.q > 4 || selected.q > k || local.ball.key_materializations != 1) {
    result.status = Status::kMebFailure; return false;
  }
  for (u8 i = 0; i < selected.q; ++i) if (selected.slots[i] >= k ||
      (i && selected.slots[i - 1] >= selected.slots[i])) {
    result.status = Status::kMebFailure; return false;
  }
  if (!materialize_level(positions, local.ball, local.level)) { result.status = Status::kMebFailure; return false; }
  if (!add(result.work.level_materializations)) { result.status = Status::kCounterOverflow; return false; }
  return true;
}
MHGP7_HD inline void note(TraceBuffer* trace, const i32* sites, u32 k, const Local& local,
    i32 intruder, BallId terminal) {
  if (!trace) return;
  if (trace->written < trace->capacity) {
    TraceRow row;
    for (u8 i = 0; i < k; ++i) row.sites[i] = sites[i];
    row.k = k; row.selection = local.ball.selected; row.key = local.ball.key;
    row.level = local.level; row.intruder = intruder; row.terminal = terminal;
    trace->rows[trace->written] = row;
  } else trace->overflow = true;
  ++trace->written;  // <= checked MEB calls, so no independent wrap.
}
MHGP7_HD inline void sort_sites(i32* sites, u32 k) {
  for (u32 i = 1; i < k; ++i) {
    const i32 value = sites[i]; u32 at = i;
    while (at && value < sites[at - 1]) { sites[at] = sites[at - 1]; --at; }
    sites[at] = value;
  }
}

// Full unbounded descent, K2..10. Views must belong to one validated owner;
// cheap local checks do not certify topology, pointer lifetime or completeness.
MHGP7_HD inline Result resolve(const View& view, const Request& request, TraceBuffer* trace = nullptr,
    SeedView seeds = {}) {
  Result result; result.ordinal = request.ordinal;
  if (trace) { trace->written = 0; trace->overflow = false; }
  if (!valid_view(view)) { result.status = Status::kInvalidView; return result; }
  if (request.snapshot != view.index.snapshot || request.k < 2 || request.k > 10 ||
      decode_level(request.before).den <= 0 || (trace && trace->capacity && !trace->rows)) return result;
  const bool seeded = seeds.snapshot != 0;
  if (seeded ? (seeds.snapshot != request.snapshot || seeds.k != request.k ||
          (seeds.count && !seeds.entries)) : (seeds.entries || seeds.count || seeds.k)) {
    result.status = Status::kInvalidSeedView; return result;
  }
  i32 sites[10]{};
  for (u32 i = 0; i < request.k; ++i) {
    const auto site = request.selected[i];
    if (site < 0 || static_cast<u32>(site) >= view.index.positions || (i && sites[i - 1] >= site)) return result;
    sites[i] = site;
  }
  Local local;
  if (!meb(view, sites, request.k, local, result)) return result;
  u64 length = 0;
  for (;;) {
    if (compare_exact_level(decode_level(local.level), decode_level(request.before)) >= 0) {
      result.status = Status::kNotStrict; return result;
    }
    if (!add(result.work.key_lookups)) { result.status = Status::kCounterOverflow; return result; }
    u32 lo = 0, hi = view.catalog.count;
    while (lo < hi) {
      const u32 mid = lo + (hi - lo) / 2;
      const BallId id = view.catalog.by_key[mid];
      if (id >= view.catalog.count) { result.status = Status::kInvalidView; return result; }
      if (compare_keys(view.catalog.entries[id].key, local.ball.key) < 0) lo = mid + 1;
      else hi = mid;
    }
    if (lo < view.catalog.count) {
      const BallId id = view.catalog.by_key[lo];
      if (id >= view.catalog.count) { result.status = Status::kInvalidView; return result; }
      const auto& ball = view.catalog.entries[id];
      if (compare_keys(ball.key, local.ball.key) == 0) {
        if (ball.arity < 2 || ball.arity > 4 || ball.shell < ball.arity || ball.shell > 12 ||
            ball.interior > 9 || ball.reserved) { result.status = Status::kInvalidView; return result; }
        const auto level = decode_level(ball.level);
        if (level.den <= 0 || !same_exact_level(level, decode_level(local.level))) {
          result.status = Status::kCatalogLevelMismatch; return result;
        }
        const u64 begin = static_cast<u64>(ball.interior) + ball.arity - 1;
        const u64 end = static_cast<u64>(ball.interior) + ball.shell;
        if (request.k >= begin && request.k <= end) {
          if (!add(result.work.anchor_hits)) { result.status = Status::kCounterOverflow; return result; }
          result.work.max_chain_steps = length;
          note(trace, sites, request.k, local, -1, id);
          result.target = id; result.status = Status::kOk; return result;
        }
      }
    }
    spatial::Request intruder_request;
    intruder_request.snapshot = request.snapshot; intruder_request.selected_count = request.k;
    for (u32 i = 0; i < request.k; ++i) intruder_request.selected[i] = sites[i];
    for (u8 word = 0; word < 10; ++word) intruder_request.key_words[word] = local.ball.key.words[word];
    const auto intruder = spatial::find_intruder(view.index, intruder_request);
    if (!add(result.work.intruder_queries, intruder.queries) || !add(result.work.intruder_nodes, intruder.nodes) ||
        !add(result.work.intruder_power_tests, intruder.power_tests) ||
        !add(result.work.interior_ranges, intruder.interior_ranges) || !add(result.work.axis_divisions, intruder.axis_divisions)) {
      result.status = Status::kCounterOverflow; return result;
    }
    if (intruder.stack_peak > result.work.stack_peak) result.work.stack_peak = intruder.stack_peak;
    if (intruder.status != spatial::Status::ok) { result.status = Status::kIntruderFailure; return result; }
    if (intruder.intruder < 0) { result.status = Status::kMissingWeakTerminal; return result; }
    const auto& selected = local.ball.selected;
    if (!selected.q || selected.slots[0] >= request.k) { result.status = Status::kMebFailure; return result; }
    note(trace, sites, request.k, local, intruder.intruder, kAbsentBall);
    sites[selected.slots[0]] = intruder.intruder;
    sort_sites(sites, request.k);
    if (seeded) {
      if (!add(result.work.post_seed_queries)) { result.status = Status::kCounterOverflow; return result; }
      u64 first = 0, last = seeds.count;
      while (first < last) {
        const u64 mid = first + (last - first) / 2;
        if (compare_seed(seeds.entries[mid].selected, sites) < 0) first = mid + 1;
        else last = mid;
      }
      if (first < seeds.count && compare_seed(seeds.entries[first].selected, sites) == 0) {
        const auto& seed = seeds.entries[first];
        if (seed.reserved || seed.target >= view.catalog.count) {
          result.status = Status::kInvalidSeedView; return result;
        }
        const auto& target = view.catalog.entries[seed.target];
        const auto level = decode_level(target.level);
        if (target.reserved || request.k != static_cast<u64>(target.interior) + target.shell ||
            level.den <= 0) { result.status = Status::kInvalidSeedView; return result; }
        if (compare_exact_level(level, decode_level(local.level)) >= 0 ||
            compare_exact_level(level, decode_level(request.before)) >= 0) {
          result.status = Status::kSeedNotStrict; return result;
        }
        if (!add(result.work.post_seed_hits) || !add(result.work.post_seed_terminals) ||
            !add(result.work.descending_steps) || !add(length)) {
          result.status = Status::kCounterOverflow; return result;
        }
        result.work.max_chain_steps = length;
        result.target = seed.target; result.status = Status::kOk; return result;
      }
    }
    Local next;
    if (!meb(view, sites, request.k, next, result)) return result;
    const int comparison = compare_exact_level(decode_level(next.level), decode_level(local.level));
    if (comparison > 0) { result.status = Status::kRadiusIncreased; return result; }
    if (comparison == 0) {
      if (compare_keys(next.ball.key, local.ball.key) != 0 ||
          next.ball.selected.shell + 1 != local.ball.selected.shell) {
        result.status = Status::kEqualRadiusInvariant; return result;
      }
      if (!add(result.work.same_radius_steps)) { result.status = Status::kCounterOverflow; return result; }
    } else if (!add(result.work.descending_steps)) { result.status = Status::kCounterOverflow; return result; }
    if (!add(length)) { result.status = Status::kCounterOverflow; return result; }
    local = next;
  }
}
}  // namespace mhgp7::gpu_terminal_private
