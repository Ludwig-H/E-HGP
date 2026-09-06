// Persistent allocation failures below the FULL Gabriel public API only.
// Explicit standalone port of the global new/delete instrumentation from
// tests/full_certificate_gate.cpp, SHA256 at port time:
// 17f5e2bafecc66556bcc1cfbfc51ed20e47cca82716ad0f2e34b1e5994a266a7.
// No other test translation unit/main is included. E5 generation and census,
// snapshots, checks and formatting occur OUTSIDE every injected fault window.
// This gate is not an independent geometric completeness qualification.
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
size_t checks = 0, failures = 0, positives = 0, allocation_count = 0;
size_t fault_runs = 0, denied_total = 0, escaped = 0, portal_positives = 0;
void check(bool good, const char* message) {
  ++checks;
  if (!good) { ++failures; std::fprintf(stderr, "FAIL %s\n", message); }
}
struct FaultGuard { ~FaultGuard() { allocation_fault::reset(); } };

FullGabrielLimits roomy() {
  FullGabrielLimits result;
  result.certificate = {1000000, 1000000, 1000000};
  result.max_points = 10000000;
  result.max_input_records = 10000000;
  result.max_aliases = 10000000;
  result.max_face_visits = 10000000;
  result.max_portal_requests = 10000000;
  result.max_chain_steps = 10000000;
  result.max_successor_steps = 10000000;
  result.max_meb_calls = 10000000;
  result.max_query_nodes = 10000000;
  result.max_meb_supports = 10000000;
  return result;
}
bool empty(const FullCertificate& c) {
  return c.order() == 0 && c.nodes().empty() && c.minima().empty() && c.parents().empty();
}
bool same_certificate(const FullCertificate& a, const FullCertificate& b) {
  if (a.order() != b.order() || a.minima() != b.minima() || a.parents() != b.parents() ||
      a.nodes().size() != b.nodes().size()) return false;
  for (size_t i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i]; const auto& y = b.nodes()[i];
    if (x.level != y.level || x.first != y.first || x.parent_count != y.parent_count) return false;
  }
  return true;
}
bool same_proposal(const FullGabrielStats& a, const FullGabrielStats& b) {
  const auto& x = a.meb_proposal;
  const auto& y = b.meb_proposal;
  return x.meb_proposal_supports == y.meb_proposal_supports && x.pivots == y.pivots &&
      x.certified == y.certified && x.fallback == y.fallback &&
      x.reference_supports == y.reference_supports;
}
bool same_events(const std::vector<ForestEvent>& a, const std::vector<ForestEvent>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i].q != b[i].q || a[i].d != b[i].d || a[i].active_mask != b[i].active_mask ||
        a[i].level != b[i].level) return false;
    for (size_t j = 0; j < 11; ++j) if (a[i].support[j] != b[i].support[j]) return false;
    for (size_t j = 0; j < 9; ++j) if (a[i].interior[j] != b[i].interior[j]) return false;
  }
  return true;
}
std::vector<unsigned> masks(const std::vector<ForestEvent>& source) {
  std::vector<unsigned> result;
  for (const auto& event : source) {
    unsigned mask = 0;
    check(event.q <= 11 && event.d <= 9, "fixture event bounds");
    if (event.q > 11 || event.d > 9) continue;
    for (size_t j = 0; j < event.q; ++j) {
      check(event.support[j] < 5, "fixture support identity");
      if (event.support[j] < 5) mask |= 1u << event.support[j];
    }
    for (size_t j = 0; j < event.d; ++j) {
      check(event.interior[j] < 5, "fixture interior identity");
      if (event.interior[j] < 5) mask |= 1u << event.interior[j];
    }
    result.push_back(mask);
  }
  std::sort(result.begin(), result.end());
  return result;
}

struct Fixture {
  CloudIndex ix;
  std::vector<ForestEvent> minima, direct;
  bool valid = false;
  Fixture() : ix(build_cloud_index(std::vector<InputPoint>{
      {0, {0, 0, 7}}, {1, {0, 9, 6}}, {2, {1, 4, 0}},
      {3, {0, 0, 1}}, {4, {4, 1, 2}}})) {
    const size_t before = failures;
    check(ix.valid && !ix.has_duplicate_positions(), "E5 index is valid");
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
          "E5 real generation completes without cap or invariant refusal");
    const u128 expected = expected_pair_mass(ix);
    for (size_t lane = 0; lane < 3; ++lane)
      check(generated.ledger_emitted_mass[lane] + generated.ledger_killed_mass[lane] == expected,
            "E5 generator pair ledger closes for every lane");
    if (failures != before) return;
    sort_candidates(&candidates, 1);
    deduplicate_candidates(&candidates);
    ExpandStats expansion;
    std::vector<Survivor> survivors;
    std::vector<BallData> balls;
    prefilter_balls(ix, candidates, options.smax, 1, &survivors, &expansion);
    const auto status = census_balls(ix, candidates, survivors, options.smax, 12, 1, &balls, &expansion);
    check(status == PipelineStatus::kCompleteRegular, "E5 real census completes regularly");
    if (status != PipelineStatus::kCompleteRegular) return;
    expand_events_k(ix, balls, 1, 2, 1, &minima, &expansion);
    expand_events_k(ix, balls, 2, 2, 1, &direct, &expansion);
    check(masks(minima) == std::vector<unsigned>({3, 6, 9, 12, 17, 20, 24}),
          "E5 seven real Gabriel minima have the named inventory");
    check(masks(direct) == std::vector<unsigned>({7, 14, 22, 25, 28}),
          "E5 five real Gabriel triangles have the named inventory");
    valid = failures == before;
  }
};

void stats_coherent(const FullGabrielStats& s, const FullGabrielLimits& caps) {
  check(s.input_records <= 12 && s.aliases <= caps.max_aliases &&
        s.face_visits <= caps.max_face_visits && s.portal_requests <= caps.max_portal_requests &&
        s.chain_steps <= caps.max_chain_steps && s.successor_steps <= caps.max_successor_steps &&
        s.meb_calls <= caps.max_meb_calls, "partial FULL work stays inside prospective budgets");
  check(s.alias_hits <= s.face_visits && s.terminal_direct <= s.portal_requests &&
        s.max_chain_length <= s.chain_steps && s.normalized_anchors <= s.successor_steps,
        "partial FULL counters preserve their causal upper bounds");
  check(s.geometry.query_nodes <= caps.max_query_nodes &&
        s.geometry.meb_supports <= caps.max_meb_supports &&
        s.geometry.query_leaves <= s.geometry.query_nodes &&
        s.geometry.meb_calls <= s.meb_calls,
        "partial geometry counters remain bounded without requiring completed statistics");
  check(s.meb_proposal.meb_proposal_supports <= caps.max_meb_proposal_supports &&
        s.meb_proposal.reference_supports <= s.geometry.meb_supports,
        "proposal work and physical F work remain bounded on every unwind");
}
bool accepted(const FullGabrielResult& result, const FullGabrielLimits& caps) {
  ++positives;
  const bool success = result.status == FullGabrielStatus::kCompleteRelative &&
      result.reason != nullptr && std::strcmp(result.reason, kFullGabrielAuthority) == 0;
  check(success, "unfaulted E5 public call completes relatively");
  if (!success) return false;
  check(result.forest.order() == 2 && result.forest.minima().size() == 7 &&
        result.forest.nodes().size() == 10 && result.forest.parents().size() == 9,
        "unfaulted E5 publishes seven minima and three ternary merges");
  const bool portal = result.stats.portal_requests > 0 && result.stats.chain_steps > 0 &&
      result.stats.terminal_direct > 0 && result.stats.normalized_anchors > 0;
  check(portal, "unfaulted E5 traverses a strict portal and normalizes a historical anchor");
  if (portal) ++portal_positives;
  stats_coherent(result.stats, caps);
  return success;
}

void allocation_gate(u64 proposal_cap) {
  const Fixture fixture;  // Includes ALL geometry/catalogue work, with faults disabled.
  check(fixture.valid, "all source preparation completed before injection");
  if (!fixture.valid) return;
  auto caps = roomy();
  caps.max_meb_proposal_supports = proposal_cap;
  const auto minima_snapshot = fixture.minima;
  const auto direct_snapshot = fixture.direct;
  const auto operation = [&]() {
    return build_full_gabriel_order(fixture.ix, 2, fixture.minima, fixture.direct, caps);
  };
  const auto sentinel = operation();
  if (!accepted(sentinel, caps)) return;
  if (proposal_cap != 0)
    check(sentinel.stats.meb_proposal.certified > 0,
          "opt-in eager allocation sweep actually uses a certificate");
  if (proposal_cap == 1)
    check(sentinel.stats.meb_proposal.fallback > 0,
          "low-P eager sweep has a persistent exhaustion followed by F");
  {
    FaultGuard guard;
    allocation_fault::reset();
    allocation_fault::calls = 0;
    allocation_fault::counting = true;
    const auto census = operation();
    allocation_count = allocation_fault::calls;
    allocation_fault::reset();
    if (!accepted(census, caps)) return;
    check(same_certificate(census.forest, sentinel.forest) && same_proposal(census.stats, sentinel.stats),
          "allocation census preserves the exact sentinel and all proposal counters");
  }
  // A bounded complete sweep, not a sampled collection of failure positions.
  check(allocation_count >= 16 && allocation_count <= 4096,
        "allocation census is non-vacuous and bounded before the complete sweep");
  if (allocation_count < 16 || allocation_count > 4096) return;
  for (size_t at = 0; at < allocation_count; ++at) {
    FaultGuard guard;
    allocation_fault::calls = 0;
    allocation_fault::denied = 0;
    allocation_fault::counting = true;
    allocation_fault::remaining = static_cast<long long>(at);
    try {
      // Only the immutable-source PUBLIC API runs while failures are armed.
      // Its return/move/destruction path must not need emergency allocations.
      const auto result = operation();
      const size_t attempted = allocation_fault::calls;
      const size_t denied = allocation_fault::denied;
      allocation_fault::reset();
      ++fault_runs;
      denied_total += denied;
      check(attempted > at && denied == attempted - at && denied != 0,
            "targeted allocation reached and every subsequent allocation persistently denied");
      check(result.status == FullGabrielStatus::kResourceExhausted,
            "persistent bad_alloc returns resource_exhausted");
      check(result.reason != nullptr && std::strcmp(result.reason, "full_gabriel_allocation_failed") == 0,
            "persistent bad_alloc has the exact public FULL Gabriel reason");
      check(empty(result.forest), "every injected failure publishes order zero and all empty arenas");
      stats_coherent(result.stats, caps);
      if (at + 1 == allocation_count) {
        check(same_proposal(result.stats, sentinel.stats),
              "last eager allocation failure retains all five MEB work fields");
      }
    } catch (...) {
      allocation_fault::reset();
      ++escaped;
      check(false, "allocation failure must not escape the public API");
    }
  }
  check(same_events(fixture.minima, minima_snapshot) && same_events(fixture.direct, direct_snapshot),
        "complete failure sweep leaves both source catalogues byte-field identical");
  const auto retry = operation();
  if (accepted(retry, caps))
    check(same_certificate(retry.forest, sentinel.forest) && same_proposal(retry.stats, sentinel.stats),
          "explicit fresh post-failure retry reproduces the pre-existing exact certificate");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  u64 proposal_cap = 0;
  if (std::string_view(argv[1]) == "--proposal-low") proposal_cap = 1;
  else if (std::string_view(argv[1]) == "--proposal-large") proposal_cap = 1000000;
  else if (std::string_view(argv[1]) != "--selftest") return 2;
  try {
    allocation_gate(proposal_cap);
  } catch (const std::exception& error) {
    allocation_fault::reset();
    std::fprintf(stderr, "unexpected=%s\n", error.what());
    return 1;
  } catch (...) {
    allocation_fault::reset();
    std::fprintf(stderr, "unexpected=nonstandard_exception\n");
    return 1;
  }
  const bool floor = positives == 3 && portal_positives == 3 && allocation_count >= 16 &&
      allocation_count <= 4096 && fault_runs == allocation_count && denied_total >= fault_runs &&
      escaped == 0 && checks >= 100;
  std::printf("full_gabriel_allocation positives=%zu portal_positives=%zu allocations=%zu "
              "fault_runs=%zu denied=%zu escaped=%zu checks=%zu failures=%zu floor=%d\n",
              positives, portal_positives, allocation_count, fault_runs, denied_total,
              escaped, checks, failures, static_cast<int>(floor));
  return failures != 0 ? 1 : floor ? 0 : 3;
}
