// Persistent allocation failures for the separate lazy FULL public API.
// Explicit standalone port of new/delete instrumentation from
// tests/full_gabriel_allocation_gate.cpp, SHA256 at port time:
// 5d3f8e86c2ab2e89bdc7e532d6d85921d4c894c3ff3a026e1a52d809042de8a4.
// No other test main is included. Both allocation gates additionally qualify
// the opt-in proposal path, with P shared across each complete sweep cell.
// E5 and analytical J1 catalogues are generated once, outside fault windows.
// This tests transactional memory behavior, NOT catalogue completeness.
// Cache admissions are prospective: a failed emplace may retain inserts=1
// without a resident entry. No fault is allowed to become a cache bypass.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <new>
#include <string_view>
#include <vector>

#include "../src/forest/full_gabriel.hpp"
#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"

namespace allocation_fault {
bool counting = false, persistent = false;
long long remaining = -1;
size_t calls = 0, denied = 0;
void before() {
  if (counting) ++calls;
  if (remaining >= 0 && remaining-- == 0) persistent = true;
  if (persistent) { ++denied; throw std::bad_alloc(); }
}
void reset() noexcept { counting = false; persistent = false; remaining = -1; }
[[gnu::noinline]] void* allocate(size_t n) {
  before();
  if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
  throw std::bad_alloc();
}
[[gnu::noinline]] void* aligned(size_t n, size_t alignment) {
  before();
  void* p = nullptr;
  if (::posix_memalign(&p, alignment, n == 0 ? 1 : n) == 0) return p;
  throw std::bad_alloc();
}
[[gnu::noinline]] void release(void* p) noexcept { std::free(p); }
}  // namespace allocation_fault

void* operator new(size_t n) { return allocation_fault::allocate(n); }
void* operator new[](size_t n) { return allocation_fault::allocate(n); }
void* operator new(size_t n, std::align_val_t a) {
  return allocation_fault::aligned(n, static_cast<size_t>(a));
}
void* operator new[](size_t n, std::align_val_t a) {
  return allocation_fault::aligned(n, static_cast<size_t>(a));
}
void* operator new(size_t n, const std::nothrow_t&) noexcept {
  try { return allocation_fault::allocate(n); } catch (...) { return nullptr; }
}
void* operator new[](size_t n, const std::nothrow_t&) noexcept {
  try { return allocation_fault::allocate(n); } catch (...) { return nullptr; }
}
void* operator new(size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  try { return allocation_fault::aligned(n, static_cast<size_t>(a)); } catch (...) { return nullptr; }
}
void* operator new[](size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  try { return allocation_fault::aligned(n, static_cast<size_t>(a)); } catch (...) { return nullptr; }
}
void operator delete(void* p) noexcept { allocation_fault::release(p); }
void operator delete[](void* p) noexcept { allocation_fault::release(p); }
void operator delete(void* p, size_t) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, size_t) noexcept { allocation_fault::release(p); }
void operator delete(void* p, std::align_val_t) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, std::align_val_t) noexcept { allocation_fault::release(p); }
void operator delete(void* p, size_t, std::align_val_t) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, size_t, std::align_val_t) noexcept { allocation_fault::release(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { allocation_fault::release(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { allocation_fault::release(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { allocation_fault::release(p); }

using namespace mhgp7;
namespace {
size_t checks = 0, failures = 0, cells = 0, positives = 0;
size_t allocation_total = 0, fault_runs = 0, denied_total = 0, escaped = 0;
size_t portal_positives = 0, j1_positives = 0, strict_positives = 0;
size_t late_stats_cells = 0, retained_work_cells = 0, admitted_failure_cells = 0;

void check(bool good, const char* message) {
  ++checks;
  if (!good) { ++failures; std::fprintf(stderr, "FAIL %s\n", message); }
}
struct FaultGuard { ~FaultGuard() { allocation_fault::reset(); } };

FullGabrielLimits roomy() {
  FullGabrielLimits r;
  r.certificate = {1000000, 1000000, 1000000};
  r.max_points = 8;
  r.max_input_records = 10000000;
  r.max_aliases = 0;  // Required by the DISTINCT lazy-cache contract.
  r.max_face_visits = 10000000;
  r.max_portal_requests = 10000000;
  r.max_chain_steps = 10000000;
  r.max_successor_steps = 10000000;
  r.max_meb_calls = 10000000;
  r.max_query_nodes = 10000000;
  r.max_meb_supports = 10000000;
  return r;
}
bool empty(const FullCertificate& c) {
  return c.order() == 0 && c.nodes().empty() && c.minima().empty() && c.parents().empty();
}
bool lazy_policy(const FullGabrielResult& r) {
  return r.alias_policy != nullptr &&
      std::strcmp(r.alias_policy, "lazy_first_c_strict_resolutions_v1") == 0;
}
bool same_certificate(const FullCertificate& a, const FullCertificate& b) {
  if (a.order() != b.order() || a.minima() != b.minima() ||
      a.parents() != b.parents() || a.nodes().size() != b.nodes().size()) return false;
  for (size_t i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i]; const auto& y = b.nodes()[i];
    if (x.level != y.level || x.first != y.first || x.parent_count != y.parent_count) return false;
  }
  return true;
}
bool same_events(const std::vector<ForestEvent>& a, const std::vector<ForestEvent>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    // ExactLevel::operator== compares the raw fraction representation.
    if (a[i].q != b[i].q || a[i].d != b[i].d ||
        a[i].active_mask != b[i].active_mask || a[i].level != b[i].level) return false;
    for (size_t j = 0; j < 11; ++j) if (a[i].support[j] != b[i].support[j]) return false;
    for (size_t j = 0; j < 9; ++j) if (a[i].interior[j] != b[i].interior[j]) return false;
  }
  return true;
}
bool same_index(const CloudIndex& a, const CloudIndex& b) {
  if (a.keys != b.keys || a.bucket_start != b.bucket_start || a.bucket_ids != b.bucket_ids ||
      a.wsum != b.wsum || a.input_count != b.input_count || a.valid != b.valid ||
      a.upos.size() != b.upos.size() || a.nodes.size() != b.nodes.size()) return false;
  for (size_t i = 0; i < a.upos.size(); ++i)
    if (a.upos[i].x != b.upos[i].x || a.upos[i].y != b.upos[i].y ||
        a.upos[i].z != b.upos[i].z) return false;
  for (size_t i = 0; i < a.nodes.size(); ++i) {
    const auto& x = a.nodes[i]; const auto& y = b.nodes[i];
    if (x.left != y.left || x.right != y.right || x.first != y.first ||
        x.last != y.last || x.parent != y.parent) return false;
    for (size_t j = 0; j < 3; ++j)
      if (x.clo[j] != y.clo[j] || x.chi[j] != y.chi[j] ||
          x.tlo[j] != y.tlo[j] || x.thi[j] != y.thi[j]) return false;
  }
  return true;
}
bool same_stats(const FullGabrielStats& a, const FullGabrielStats& b) {
  // Field-by-field, never memcmp over padding. This list includes every
  // public FULL and geometry counter, including prospective admissions.
#define MHGP7_LAZY_ALLOC_SAME(field) if (a.field != b.field) return false
  MHGP7_LAZY_ALLOC_SAME(input_records); MHGP7_LAZY_ALLOC_SAME(face_visits);
  MHGP7_LAZY_ALLOC_SAME(aliases); MHGP7_LAZY_ALLOC_SAME(alias_hits);
  MHGP7_LAZY_ALLOC_SAME(portal_requests); MHGP7_LAZY_ALLOC_SAME(chain_steps);
  MHGP7_LAZY_ALLOC_SAME(terminal_direct); MHGP7_LAZY_ALLOC_SAME(max_chain_length);
  MHGP7_LAZY_ALLOC_SAME(normalized_anchors); MHGP7_LAZY_ALLOC_SAME(successor_steps);
  MHGP7_LAZY_ALLOC_SAME(no_op_connections); MHGP7_LAZY_ALLOC_SAME(meb_calls);
  MHGP7_LAZY_ALLOC_SAME(minimum_lookups); MHGP7_LAZY_ALLOC_SAME(minimum_hits);
  MHGP7_LAZY_ALLOC_SAME(cache_lookups); MHGP7_LAZY_ALLOC_SAME(cache_hits);
  MHGP7_LAZY_ALLOC_SAME(cache_inserts); MHGP7_LAZY_ALLOC_SAME(cache_skips);
  MHGP7_LAZY_ALLOC_SAME(singleton_intruder_resolutions); MHGP7_LAZY_ALLOC_SAME(direct_lookups);
  MHGP7_LAZY_ALLOC_SAME(geometry.core_records); MHGP7_LAZY_ALLOC_SAME(geometry.core_facets);
  MHGP7_LAZY_ALLOC_SAME(geometry.facets_with_two_intruders); MHGP7_LAZY_ALLOC_SAME(geometry.chain_steps);
  MHGP7_LAZY_ALLOC_SAME(geometry.added_cofaces); MHGP7_LAZY_ALLOC_SAME(geometry.terminal_direct);
  MHGP7_LAZY_ALLOC_SAME(geometry.terminal_cached); MHGP7_LAZY_ALLOC_SAME(geometry.max_chain_length);
  MHGP7_LAZY_ALLOC_SAME(geometry.query_nodes); MHGP7_LAZY_ALLOC_SAME(geometry.query_leaves);
  MHGP7_LAZY_ALLOC_SAME(geometry.query_range_skips); MHGP7_LAZY_ALLOC_SAME(geometry.meb_calls);
  MHGP7_LAZY_ALLOC_SAME(geometry.meb_supports);
  MHGP7_LAZY_ALLOC_SAME(meb_proposal.meb_proposal_supports);
  MHGP7_LAZY_ALLOC_SAME(meb_proposal.pivots);
  MHGP7_LAZY_ALLOC_SAME(meb_proposal.certified);
  MHGP7_LAZY_ALLOC_SAME(meb_proposal.fallback);
  MHGP7_LAZY_ALLOC_SAME(meb_proposal.reference_supports);
#undef MHGP7_LAZY_ALLOC_SAME
  return true;
}

enum class Scene { kE5, kJ1 };
std::vector<InputPoint> scene_points(Scene scene) {
  if (scene == Scene::kE5)
    return {{0, {0, 0, 7}}, {1, {0, 9, 6}}, {2, {1, 4, 0}},
            {3, {0, 0, 1}}, {4, {4, 1, 2}}};
  // Proof: audits/receipts_full_producer_20260905/lazy_alias_next_step_review.md.
  // ABC has essential AB and interior C. At ABW, AB has the unique intruder C.
  return {{0, {0, 5, 0}}, {1, {4, 5, 0}}, {2, {2, 6, 0}}, {3, {2, 0, 0}}};
}
unsigned event_mask(const ForestEvent& e, size_t n) {
  unsigned mask = 0;
  check(e.q <= 11 && e.d <= 9, "fixture event bounds");
  if (e.q > 11 || e.d > 9) return 0;
  for (size_t i = 0; i < e.q; ++i) {
    check(e.support[i] < n, "dense fixture support identity is in range");
    if (e.support[i] < n) mask |= 1u << e.support[i];
  }
  for (size_t i = 0; i < e.d; ++i) {
    check(e.interior[i] < n, "dense fixture interior identity is in range");
    if (e.interior[i] < n) mask |= 1u << e.interior[i];
  }
  return mask;
}
std::vector<unsigned> masks(const std::vector<ForestEvent>& events, size_t n) {
  std::vector<unsigned> result;
  for (const auto& e : events) result.push_back(event_mask(e, n));
  std::sort(result.begin(), result.end());
  return result;
}
ExactLevel level(u64 num, i128 den) { return ExactLevel{{num, 0, 0}, den}; }

struct Fixture {
  Scene scene;
  const char* name;
  CloudIndex ix;
  std::vector<ForestEvent> minima, direct;
  bool valid = false;

  explicit Fixture(Scene s)
      : scene(s), name(s == Scene::kE5 ? "E5" : "J1"), ix(build_cloud_index(scene_points(s))) {
    const size_t before = failures;
    check(ix.valid && !ix.has_duplicate_positions(), "fixture checked index is valid");
    if (!ix.valid || ix.has_duplicate_positions()) return;
    GenerateOptions options;
    options.s = 8;
    options.smax = 3;
    options.threads = 1;
    options.max_raw_candidates = 100000;
    GenerateStats generated;
    std::vector<BallCandidate> candidates;
    generate_candidates(ix, options, &candidates, &generated);
    check(generated.cap_refus == kCapRefusNone && generated.invariant_jneg == 0,
          "real fixture generation completes without cap or invariant refusal");
    const u128 expected = expected_pair_mass(ix);
    for (size_t lane = 0; lane < 3; ++lane)
      check(generated.ledger_emitted_mass[lane] + generated.ledger_killed_mass[lane] == expected,
            "real generation pair ledger closes in each lane");
    if (failures != before) return;
    sort_candidates(&candidates, 1);
    deduplicate_candidates(&candidates);
    ExpandStats expansion;
    std::vector<Survivor> survivors;
    std::vector<BallData> balls;
    prefilter_balls(ix, candidates, options.smax, 1, &survivors, &expansion);
    const auto status = census_balls(ix, candidates, survivors, options.smax, 12, 1, &balls, &expansion);
    check(status == PipelineStatus::kCompleteRegular, "real fixture census completes regularly");
    if (status != PipelineStatus::kCompleteRegular) return;
    expand_events_k(ix, balls, 1, 2, 1, &minima, &expansion);
    expand_events_k(ix, balls, 2, 2, 1, &direct, &expansion);
    if (scene == Scene::kE5) {
      check(masks(minima, 5) == std::vector<unsigned>({3, 6, 9, 12, 17, 20, 24}),
            "E5 real seven-minimum inventory");
      check(masks(direct, 5) == std::vector<unsigned>({7, 14, 22, 25, 28}),
            "E5 real five-direct inventory");
    } else {
      check(masks(minima, 4) == std::vector<unsigned>({5, 6, 9, 10}),
            "J1 real AC BC AW BW minima");
      check(masks(direct, 4) == std::vector<unsigned>({7, 11}),
            "J1 real ABC ABW direct connections");
      for (const auto& e : minima) {
        const unsigned mask = event_mask(e, 4);
        check(same_exact_level(e.level, mask == 5 || mask == 6 ? level(5, 4) : level(29, 4)),
              "J1 analytical minimum level");
      }
      for (const auto& e : direct) {
        const unsigned mask = event_mask(e, 4);
        check(same_exact_level(e.level, mask == 7 ? level(4, 1) : level(841, 100)),
              "J1 analytical direct level");
        if (mask == 7)
          check(e.q == 2 && e.d == 1 && e.support[0] == 0 && e.support[1] == 1 &&
                e.interior[0] == 2, "J1 ABC essential AB with C strictly inside");
      }
    }
    valid = failures == before;
  }
};
struct Snapshot {
  CloudIndex ix;
  std::vector<ForestEvent> minima, direct;
  explicit Snapshot(const Fixture& f) : ix(f.ix), minima(f.minima), direct(f.direct) {}
  bool unchanged(const Fixture& f) const {
    return same_index(ix, f.ix) && same_events(minima, f.minima) && same_events(direct, f.direct);
  }
};

void stats_coherent(const FullGabrielStats& s, const Fixture& f,
                    const FullGabrielLimits& caps, u64 cache) {
  check(s.input_records <= f.minima.size() + f.direct.size() &&
        s.input_records <= caps.max_input_records && s.aliases == 0 && s.alias_hits == 0 &&
        s.face_visits <= caps.max_face_visits && s.portal_requests <= caps.max_portal_requests &&
        s.chain_steps <= caps.max_chain_steps && s.successor_steps <= caps.max_successor_steps &&
        s.meb_calls <= caps.max_meb_calls, "partial lazy work remains in prospective budgets");
  check(s.minimum_hits <= s.minimum_lookups && s.minimum_lookups <= s.face_visits &&
        s.cache_lookups <= s.minimum_lookups && s.cache_hits <= s.cache_lookups &&
        s.cache_inserts <= cache && s.cache_inserts + s.cache_skips <= s.portal_requests &&
        s.direct_lookups <= s.meb_calls && s.singleton_intruder_resolutions <= s.terminal_direct &&
        s.terminal_direct <= s.portal_requests && s.max_chain_length <= s.chain_steps &&
        s.normalized_anchors <= s.successor_steps, "partial lazy counters preserve causal upper bounds");
  check(s.geometry.query_nodes <= caps.max_query_nodes &&
        s.geometry.meb_supports <= caps.max_meb_supports &&
        s.geometry.query_leaves <= s.geometry.query_nodes &&
        s.geometry.meb_calls <= s.meb_calls,
        "partial geometry remains bounded without demanding completed counters");
  check(s.geometry.core_records == 0 && s.geometry.core_facets == 0 &&
        s.geometry.added_cofaces == 0, "neither success nor refusal materializes the silent core");
  const auto& p = s.meb_proposal;
  check(p.meb_proposal_supports <= caps.max_meb_proposal_supports &&
        p.reference_supports <= s.geometry.meb_supports &&
        p.certified + p.fallback <= s.geometry.meb_calls,
        "proposal and actual reference work survive allocation unwinds within their budgets");
  if (caps.max_meb_proposal_supports == 0)
    check(p.meb_proposal_supports == 0 && p.pivots == 0 && p.certified == 0 &&
          p.reference_supports == s.geometry.meb_supports,
          "default path is physical F with zero proposal work");
  if (cache == 0)
    check(s.cache_inserts == 0 && s.cache_hits == 0, "capacity zero never admits or hits a cache entry");
}
void named_shape(const Fixture& f, const FullCertificate& forest) {
  const bool e5 = f.scene == Scene::kE5;
  check(forest.order() == 2 && forest.minima().size() == (e5 ? 7u : 4u) &&
        forest.nodes().size() == (e5 ? 10u : 6u) && forest.parents().size() == (e5 ? 9u : 5u),
        "named FULL shape includes every minimum and the exact merge arities");
  if (forest.nodes().size() != (e5 ? 10u : 6u)) return;
  const std::vector<ExactLevel> levels = e5 ?
      std::vector<ExactLevel>{level(9, 2), level(9, 2), level(11, 2), level(162, 25),
          level(9, 1), level(21, 2), level(189, 17), level(31, 2),
          level(41, 2), level(83886, 3563)} :
      std::vector<ExactLevel>{level(5, 4), level(5, 4), level(4, 1),
          level(29, 4), level(29, 4), level(841, 100)};
  for (size_t i = 0; i < levels.size(); ++i) {
    check(same_exact_level(forest.nodes()[i].level, levels[i]), "named exact rational node level");
    const u64 arity = e5 ? ((i == 3 || i == 6 || i == 9) ? 3u : 0u) :
        (i == 2 ? 2u : i == 5 ? 3u : 0u);
    check(forest.nodes()[i].parent_count == arity, "named atomic merge arity");
  }
  const std::vector<FullNodeId> parents = e5 ?
      std::vector<FullNodeId>{0, 1, 2, 3, 4, 5, 6, 7, 8} :
      std::vector<FullNodeId>{0, 1, 2, 3, 4};
  check(forest.parents() == parents, "named causal parent CSR");
}
bool accepted(const FullGabrielResult& r, const Fixture& f,
              const FullGabrielLimits& caps, u64 cache) {
  ++positives;
  const bool success = r.status == FullGabrielStatus::kCompleteRelative &&
      r.reason != nullptr && std::strcmp(r.reason, kFullGabrielAuthority) == 0;
  check(success, "unfaulted lazy public call completes relatively");
  check(lazy_policy(r), "success names the distinct lazy first-C policy");
  if (!success) return false;
  named_shape(f, r.forest);
  const bool portal = r.stats.portal_requests > 0 && r.stats.terminal_direct > 0 &&
      r.stats.meb_calls > 0 && r.stats.geometry.meb_calls > 0;
  check(portal, "every successful cell physically traverses a portal");
  if (portal) ++portal_positives;
  if (f.scene == Scene::kJ1) {
    const bool j1 = r.stats.portal_requests == 1 && r.stats.singleton_intruder_resolutions == 1 &&
        r.stats.terminal_direct == 1 && r.stats.chain_steps == 0 && r.stats.max_chain_length == 0 &&
        r.stats.meb_calls == 1 && r.stats.geometry.meb_calls == 1 && r.stats.direct_lookups == 1;
    check(j1, "J1 resolves AB via ABC without a second MEB or a strict-chain step");
    if (j1) ++j1_positives;
  } else {
    const bool strict = r.stats.chain_steps > 0 && r.stats.normalized_anchors > 0;
    check(strict, "E5 strict descent normalizes a historical terminal");
    if (strict) ++strict_positives;
  }
  stats_coherent(r.stats, f, caps, cache);
  if (cache == 0) check(r.stats.cache_skips > 0, "capacity-zero success actually bypasses insertion");
  else if (cache == 1) check(r.stats.cache_inserts == 1, "capacity-one success fills its cache");
  else check(r.stats.cache_inserts > 0, "large-cache success actually inserts");
  return success;
}

FullGabrielResult exercise_cell(const Fixture& fixture, const Snapshot& snapshot, u64 cache,
                               const FullCertificate* other_capacity, u64 proposal_cap = 0) {
  auto caps = roomy();
  caps.max_meb_proposal_supports = proposal_cap;
  const auto operation = [&]() {
    return build_full_gabriel_order_lazy(fixture.ix, 2, fixture.minima, fixture.direct,
                                         caps, FullGabrielCacheLimits{cache});
  };
  auto sentinel = operation();
  if (!accepted(sentinel, fixture, caps, cache)) return sentinel;
  check(std::strcmp(sentinel.meb_accounting, kFullGabrielMebAccounting) == 0,
        "explicit proposal accounting version on the public result");
  if (proposal_cap != 0) {
    check(sentinel.stats.meb_proposal.certified > 0 &&
          sentinel.stats.meb_proposal.meb_proposal_supports > 0,
          "opt-in allocation sweep has actually certified a proposal");
    if (proposal_cap == 1 && fixture.scene == Scene::kE5)
      check(sentinel.stats.meb_proposal.fallback > 0,
            "shared exhausted P actually falls back after its first certificate");
  }
  if (other_capacity)
    check(same_certificate(sentinel.forest, *other_capacity), "capacity does not change the FULL certificate");
  check(snapshot.unchanged(fixture), "sentinel leaves immutable inputs field-identical");

  size_t allocation_count = 0;
  {
    FaultGuard guard;
    allocation_fault::reset();
    allocation_fault::calls = 0;
    allocation_fault::counting = true;
    const auto census = operation();
    allocation_count = allocation_fault::calls;
    allocation_fault::reset();
    if (!accepted(census, fixture, caps, cache)) return sentinel;
    check(same_certificate(census.forest, sentinel.forest) && same_stats(census.stats, sentinel.stats),
          "allocation census exactly reproduces the sentinel and all work counters");
    check(snapshot.unchanged(fixture), "allocation census leaves immutable inputs field-identical");
  }
  check(allocation_count >= 16 && allocation_count <= 4096,
        "each allocation census is non-vacuous and bounded before the complete sweep");
  if (allocation_count < 16 || allocation_count > 4096) return sentinel;
  ++cells;
  allocation_total += allocation_count;
  size_t retained_work = 0, admitted_failures = 0, late_stats = 0;
  const size_t runs_before = fault_runs;
  for (size_t at = 0; at < allocation_count; ++at) {
    FaultGuard guard;
    allocation_fault::reset();
    allocation_fault::calls = 0;
    allocation_fault::denied = 0;
    allocation_fault::counting = true;
    allocation_fault::remaining = static_cast<long long>(at);
    ++fault_runs;
    try {
      // ONLY this immutable-source PUBLIC API runs with persistent faults.
      // No snapshots, checks, comparisons, output or fixture preparation here.
      const auto result = operation();
      const size_t attempted = allocation_fault::calls;
      const size_t denied = allocation_fault::denied;
      allocation_fault::reset();
      denied_total += denied;
      check(attempted > at && denied == attempted - at && denied != 0,
            "targeted allocation is reached and every later attempt is persistently denied");
      check(result.status == FullGabrielStatus::kResourceExhausted,
            "persistent bad_alloc is a global resource refusal, never a cache bypass");
      check(result.reason != nullptr &&
            std::strcmp(result.reason, "full_gabriel_allocation_failed") == 0,
            "persistent bad_alloc has the exact public reason");
      check(lazy_policy(result), "refusal retains the lazy policy");
      check(empty(result.forest), "refusal publishes order zero and all arenas empty");
      stats_coherent(result.stats, fixture, caps, cache);
      if (result.stats.portal_requests > 0 && result.stats.meb_calls > 0 &&
          result.stats.geometry.meb_calls > 0) ++retained_work;
      if (result.stats.cache_inserts > 0) ++admitted_failures;
      if (at + 1 == allocation_count) {
        // Final certificate validation follows ALL geometry. Its final
        // allocation failure must retain the entire successful work record.
        const bool retained = same_stats(result.stats, sentinel.stats);
        check(retained, "last allocation refusal retains every completed work counter exactly");
        if (retained) ++late_stats;
      }
    } catch (...) {
      allocation_fault::reset();
      ++escaped;
      check(false, "persistent allocation failure cannot escape the public API");
    }
    check(snapshot.unchanged(fixture), "every injected failure leaves all input and index fields unchanged");
  }
  check(retained_work > 0, "each cell includes a refusal after actual portal geometry");
  if (retained_work > 0) ++retained_work_cells;
  check(late_stats == 1, "each cell has one terminal full-statistics refusal sentinel");
  if (late_stats == 1) ++late_stats_cells;
  if (cache != 0) {
    check(admitted_failures > 0, "positive-capacity refusal preserves prospective cache admissions");
    if (admitted_failures > 0) ++admitted_failure_cells;
  } else check(admitted_failures == 0, "capacity-zero refusal never claims cache admission");
  const auto retry = operation();
  if (accepted(retry, fixture, caps, cache))
    check(same_certificate(retry.forest, sentinel.forest) && same_stats(retry.stats, sentinel.stats),
          "explicit fresh post-sweep call reproduces the complete sentinel");
  check(snapshot.unchanged(fixture), "post-sweep success leaves every source and index field unchanged");
  std::printf("lazy_allocation_cell scene=%s cache=%llu proposal_cap=%llu allocations=%zu fault_runs=%zu "
              "retained_work=%zu admitted_failures=%zu late_stats=%zu portals=%llu "
              "singleton=%llu inserts=%llu skips=%llu\n", fixture.name,
              static_cast<unsigned long long>(cache),
              static_cast<unsigned long long>(proposal_cap), allocation_count, fault_runs - runs_before,
              retained_work, admitted_failures, late_stats,
              static_cast<unsigned long long>(sentinel.stats.portal_requests),
              static_cast<unsigned long long>(sentinel.stats.singleton_intruder_resolutions),
              static_cast<unsigned long long>(sentinel.stats.cache_inserts),
              static_cast<unsigned long long>(sentinel.stats.cache_skips));
  return sentinel;
}

void exercise_fixture(Scene scene, u64 proposal_cap = 0) {
  const Fixture fixture(scene);  // All generation/census work, exactly once.
  check(fixture.valid, "fixture preparation closes before every injection window");
  if (!fixture.valid) return;
  const Snapshot snapshot(fixture);  // All allocation-bearing snapshots are outside injection.
  const auto zero = exercise_cell(fixture, snapshot, 0, nullptr, proposal_cap);
  const auto one = exercise_cell(fixture, snapshot, 1, &zero.forest, proposal_cap);
  const auto large = exercise_cell(fixture, snapshot, 1000000, &zero.forest, proposal_cap);
  check(same_certificate(one.forest, large.forest), "positive capacities have the same exact forest");
}
}  // namespace

int main(int argc, char** argv) {
  // Exact argv gate precedes all fixture generation and allocation injection.
  if (argc != 2) return 2;
  u64 proposal_cap = 0;
  if (std::string_view(argv[1]) == "--proposal-low") proposal_cap = 1;
  else if (std::string_view(argv[1]) == "--proposal-large") proposal_cap = 1000000;
  else if (std::string_view(argv[1]) != "--selftest") return 2;
  try {
    exercise_fixture(Scene::kE5, proposal_cap);
    exercise_fixture(Scene::kJ1, proposal_cap);
  } catch (const std::exception& error) {
    allocation_fault::reset();
    std::fprintf(stderr, "unexpected=%s\n", error.what());
    return 1;
  } catch (...) {
    allocation_fault::reset();
    std::fprintf(stderr, "unexpected=nonstandard_exception\n");
    return 1;
  }
  const bool floor = cells == 6 && positives == 18 && portal_positives == 18 &&
      j1_positives == 9 && strict_positives == 9 &&
      allocation_total >= 96 && allocation_total <= 24576 &&
      fault_runs == allocation_total && denied_total >= fault_runs && escaped == 0 &&
      retained_work_cells == 6 && late_stats_cells == 6 && admitted_failure_cells == 4 && checks >= 600;
  std::printf("full_gabriel_lazy_allocation cells=%zu positives=%zu portal_positives=%zu "
              "j1_positives=%zu strict_positives=%zu allocations=%zu fault_runs=%zu "
              "denied=%zu escaped=%zu retained_work_cells=%zu late_stats_cells=%zu "
              "admitted_failure_cells=%zu checks=%zu failures=%zu floor=%d\n",
              cells, positives, portal_positives, j1_positives, strict_positives,
              allocation_total, fault_runs, denied_total, escaped, retained_work_cells,
              late_stats_cells, admitted_failure_cells, checks, failures, static_cast<int>(floor));
  return failures != 0 ? 1 : floor ? 0 : 3;
}
