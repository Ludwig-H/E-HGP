// Structural FULL certificate gate. Coordinates below explain the exact small
// fixtures; this test neither calls geometry nor certifies Gabriel completeness.
// Allocation failures are injected below the public API, never in the product.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <new>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../src/forest/full_certificate.hpp"

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
static_assert(!std::is_copy_constructible_v<FullCertificate>);
static_assert(!std::is_copy_assignable_v<FullCertificate>);
static_assert(std::is_nothrow_move_constructible_v<FullCertificate>);
static_assert(std::is_nothrow_move_assignable_v<FullCertificate>);
namespace {
unsigned checks = 0, failures = 0, positive_cases = 0;
unsigned rejected_builds = 0, rejected_reads = 0, allocation_failures = 0;
void check(bool condition, const char* message) {
  ++checks;
  if (!condition) { ++failures; std::fprintf(stderr, "FAIL %s\n", message); }
}
ExactLevel level(u64 numerator, i128 denominator = 1) {
  return {{numerator, 0, 0}, denominator};
}
FacetKey facet(std::initializer_list<PointId> ids) {
  FacetKey f;
  f.k = static_cast<u8>(ids.size());
  size_t j = 0;
  for (PointId id : ids) f.p[j++] = id;
  return f;
}
struct Fixture {
  unsigned order = 2;
  std::vector<PointId> points{0, 1, 2};
  std::vector<FullBatch> batches;
};
constexpr FullCertificateLimits roomy{128, 256, 512};
Fixture acute() {
  // A=(0,0,0), B=(6,0,0), C=(2,3,0): three isolated minima,
  // then ONE three-parent fusion. No reduced-profile zero-parent birth.
  return {2, {0, 1, 2}, {{level(13, 4), {facet({0, 2})}, {}},
      {level(25, 4), {facet({1, 2})}, {}}, {level(9), {facet({0, 1})}, {}},
      {level(325, 36), {}, {{0, 1, 2}}}}};
}
Fixture paired_plateau() {
  return {2, {0, 1, 2, 3, 4, 5},
      {{level(1), {facet({0, 1}), facet({0, 2}), facet({3, 4}), facet({3, 5})}, {}},
       {level(2), {}, {{0, 1}, {2, 3}}}, {level(3), {}, {{4, 5}}}}};
}
FullBuildResult build(const Fixture& f, FullCertificateLimits limits = roomy) {
  return build_full_certificate(f.order, f.points, f.batches, limits);
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
void accepted(const FullBuildResult& r, const char* message) {
  ++positive_cases;
  check(r.status == FullCertificateStatus::kOk && std::strcmp(r.reason, "structural_only") == 0 &&
        !empty(r.value), message);
}
void reject(const Fixture& f, const char* reason,
            FullCertificateStatus status = FullCertificateStatus::kInvalidInput,
            FullCertificateLimits limits = roomy) {
  const auto r = build(f, limits);
  ++rejected_builds;
  check(r.status == status && std::strcmp(r.reason, reason) == 0, reason);
  check(empty(r.value), "build refusal publishes no order or arena");
}
template <typename T> void reject_read(const FullReadResult<T>& r, const char* reason,
                                     FullCertificateStatus status = FullCertificateStatus::kResourceExhausted) {
  ++rejected_reads;
  check(r.status == status && std::strcmp(r.reason, reason) == 0 && r.values.empty(), reason);
}
void roots(const FullCertificate& c, ExactLevel cut, bool closed,
           std::initializer_list<FullNodeId> expected) {
  const auto r = full_certificate_roots_at(c, cut, closed, c.nodes().size());
  check(r.status == FullCertificateStatus::kOk && std::strcmp(r.reason, "structural_only") == 0 &&
        r.values == std::vector<FullNodeId>(expected), "exact open/closed root replay");
}
void coverage(const FullCertificate& c, FullNodeId id, u64 node_cap, u64 refs,
              std::initializer_list<PointId> expected) {
  const auto r = full_certificate_coverage(c, id, node_cap, refs);
  check(r.status == FullCertificateStatus::kOk && std::strcmp(r.reason, "structural_only") == 0 &&
        r.values == std::vector<PointId>(expected), "exact sorted distinct point coverage");
}

void positive_gate() {
  check(std::strcmp(kFullCertificateAuthority, "structural_only") == 0 &&
        std::strcmp(kFullCertificateSchema, "full_minima_merge_forest_v1") == 0,
        "separate structural authority and FULL schema");
  const auto a = build(acute(), {4, 4, 3});
  accepted(a, "acute at exact batch/node/parent caps");
  if (a.status != FullCertificateStatus::kOk) return;
  check(a.value.order() == 2 && a.value.minima().size() == 3 && a.value.nodes().size() == 4 &&
        a.value.parents() == std::vector<FullNodeId>({0, 1, 2}), "acute arenas and dense IDs");
  check(a.value.nodes()[3].first == 0 && a.value.nodes()[3].parent_count == 3 &&
        a.value.nodes()[0].first == 0 && a.value.nodes()[1].first == 1 && a.value.nodes()[2].first == 2,
        "minimum indices and parent CSR offset have distinct meanings");
  roots(a.value, level(0), true, {});
  roots(a.value, level(13, 4), false, {}); roots(a.value, level(13, 4), true, {0});
  roots(a.value, level(25, 4), false, {0}); roots(a.value, level(25, 4), true, {0, 1});
  roots(a.value, level(9), false, {0, 1}); roots(a.value, level(18, 2), true, {0, 1, 2});
  roots(a.value, level(325, 36), false, {0, 1, 2});
  roots(a.value, level(650, 72), true, {3}); roots(a.value, level(100), true, {3});
  coverage(a.value, 0, 1, 2, {0, 2}); coverage(a.value, 1, 1, 2, {1, 2});
  coverage(a.value, 2, 1, 2, {0, 1}); coverage(a.value, 3, 4, 6, {0, 1, 2});
  const auto rebuilt = build(acute());
  check(rebuilt.status == FullCertificateStatus::kOk && same_certificate(rebuilt.value, a.value),
        "independent build preserves all semantic fields");

  // Obtuse A=(0,0,0), B=(6,0,0), C=(1,1,0): AB is not a minimum.
  Fixture obtuse{2, {0, 1, 2}, {{level(1, 2), {facet({0, 2})}, {}},
      {level(13, 2), {facet({1, 2})}, {}}, {level(9), {}, {{0, 1}}}}};
  const auto o = build(obtuse, {3, 3, 2}); accepted(o, "obtuse has exactly two minima");
  if (o.status == FullCertificateStatus::kOk) {
    check(o.value.minima().size() == 2 && o.value.nodes()[2].parent_count == 2, "obtuse binary merge");
    roots(o.value, level(9), false, {0, 1}); roots(o.value, level(9), true, {2});
    coverage(o.value, 2, 3, 4, {0, 1, 2});
  }

  // Symmetric obtuse q2 support: A=(0,0), B=(4,0), C=(2,1).
  Fixture symmetric{2, {0, 1, 2}, {{level(5, 4), {facet({0, 2}), facet({1, 2})}, {}},
      {level(4), {}, {{0, 1}}}}};
  const auto s = build(symmetric); accepted(s, "two q2 minima in a single exact-level batch");
  if (s.status == FullCertificateStatus::kOk) {
    roots(s.value, level(10, 8), false, {}); roots(s.value, level(10, 8), true, {0, 1});
    roots(s.value, level(4), false, {0, 1}); roots(s.value, level(4), true, {2});
  }

  const PointId hi = std::numeric_limits<PointId>::max();
  Fixture k1{1, {0, 17, hi}, {{level(0), {facet({0}), facet({17}), facet({hi})}, {}},
      {level(1), {}, {{0, 1}}}, {level(2), {}, {{2, 3}}}}};
  const auto one = build(k1); accepted(one, "K1 roots include PointId zero and UINT32_MAX");
  if (one.status == FullCertificateStatus::kOk) {
    roots(one.value, level(0), false, {}); roots(one.value, level(0), true, {0, 1, 2});
    roots(one.value, level(1), false, {0, 1, 2}); roots(one.value, level(1), true, {2, 3});
    roots(one.value, level(2), true, {4}); coverage(one.value, 4, 5, 3, {0, 17, hi});
  }
  Fixture singleton{1, {hi}, {{level(0), {facet({hi})}, {}}}};
  const auto single = build(singleton, {1, 1, 0}); accepted(single, "K1=n singleton library certificate");
  if (single.status == FullCertificateStatus::kOk) coverage(single.value, 0, 1, 1, {hi});

  Fixture terminal{3, {0, 1, 2}, {{level(325, 36), {facet({0, 1, 2})}, {}}}};
  const auto t = build(terminal, {1, 1, 0}); accepted(t, "K=n isolated terminal minimum needs no coface");
  if (t.status == FullCertificateStatus::kOk) {
    roots(t.value, level(325, 36), false, {}); roots(t.value, level(325, 36), true, {0});
    roots(t.value, level(1000), true, {0}); coverage(t.value, 0, 1, 3, {0, 1, 2});
  }
  Fixture maximal{10, {0, 1, 2, 3, 4, 5, 6, 7, 8, hi},
      {{level(1), {facet({0, 1, 2, 3, 4, 5, 6, 7, 8, hi})}, {}}}};
  const auto ten = build(maximal, {1, 1, 0}); accepted(ten, "K10 uses all ten facet slots");
  if (ten.status == FullCertificateStatus::kOk)
    coverage(ten.value, 0, 1, 10, {0, 1, 2, 3, 4, 5, 6, 7, 8, hi});

  const auto p = build(paired_plateau(), {3, 7, 6}); accepted(p, "independent merges in one plateau");
  if (p.status == FullCertificateStatus::kOk) {
    roots(p.value, level(2), false, {0, 1, 2, 3}); roots(p.value, level(2), true, {4, 5});
    roots(p.value, level(3), true, {6}); coverage(p.value, 6, 7, 8, {0, 1, 2, 3, 4, 5});
    check(p.value.nodes()[4].first == 0 && p.value.nodes()[5].first == 2 &&
          p.value.nodes()[6].first == 4, "nonzero CSR offsets replayed");
  }
  Fixture mixed{2, {0, 1, 2, 3}, {{level(1), {facet({0, 1}), facet({0, 2})}, {}},
      {level(2), {facet({2, 3})}, {{0, 1}}}}};
  const auto m = build(mixed); accepted(m, "unrelated birth and old-root merge in same batch");
  if (m.status == FullCertificateStatus::kOk) {
    roots(m.value, level(2), true, {2, 3});
    check(m.value.nodes()[2].parent_count == 0 && m.value.nodes()[3].parent_count == 2,
          "same-batch dense IDs assign births before merges");
  }
  Fixture wide{2, {0, 1, 2}, {{{{0, 0, 1}, 1}, {facet({0, 1})}, {}},
      {{{0, 0, 2}, 1}, {facet({1, 2})}, {}}, {{{0, 0, 3}, 1}, {}, {{0, 1}}}}};
  const auto w = build(wide); accepted(w, "structural wide U192 levels are not truncated to u64");
  if (w.status == FullCertificateStatus::kOk) {
    roots(w.value, {{0, 0, 4}, 2}, false, {0}); roots(w.value, {{0, 0, 4}, 2}, true, {0, 1});
    roots(w.value, {{0, 0, 6}, 2}, true, {2});
  }

  auto move_source = build(acute()); accepted(move_source, "move-only source certificate");
  if (move_source.status == FullCertificateStatus::kOk) {
    allocation_fault::persistent = true;
    FullCertificate moved(std::move(move_source.value));
    allocation_fault::reset();
    check(empty(move_source.value) && same_certificate(moved, a.value),
          "nothrow move constructor transfers all arenas and invalidates source");
    reject_read(full_certificate_roots_at(move_source.value, level(100), true, 10),
                "full_invalid_read", FullCertificateStatus::kInvalidInput);
    reject_read(full_certificate_coverage(move_source.value, 0, 10, 10),
                "full_invalid_read", FullCertificateStatus::kInvalidInput);
    auto destination = build(paired_plateau());
    check(destination.status == FullCertificateStatus::kOk, "nonempty move-assignment destination");
    allocation_fault::persistent = true;
    destination.value = std::move(moved);
    allocation_fault::reset();
    check(empty(moved) && same_certificate(destination.value, a.value),
          "move assignment replaces old arenas and invalidates source");
    FullCertificate* alias = &destination.value;
    destination.value = std::move(*alias);
    check(same_certificate(destination.value, a.value), "self move preserves all semantic fields");
    FullCertificate invalid;
    destination.value = std::move(invalid);
    check(empty(invalid) && empty(destination.value), "move from invalid value empties old destination");
    FullCertificate invalid_moved(std::move(invalid));
    check(empty(invalid) && empty(invalid_moved), "moving default source leaves both values invalid");
    reject_read(full_certificate_roots_at(destination.value, level(100), true, 10),
                "full_invalid_read", FullCertificateStatus::kInvalidInput);
  }
}

void rejection_gate() {
  using Status = FullCertificateStatus;
  Fixture f = acute();
  f.order = 0; reject(f, "full_invalid_domain"); f.order = 11; reject(f, "full_invalid_domain");
  f = acute(); f.points.clear(); reject(f, "full_invalid_domain");
  f = acute(); f.order = 4; reject(f, "full_invalid_domain");
  f = acute(); f.batches.clear(); reject(f, "full_invalid_domain");
  f = acute(); f.points = {0, 2, 1}; reject(f, "full_point_order");
  f = acute(); f.points = {0, 1, 1}; reject(f, "full_point_order");
  f = acute(); f.batches[0].level.den = 0; reject(f, "full_invalid_level");
  f = acute(); f.batches[1].level.den = -1; reject(f, "full_invalid_level");
  f = acute(); f.batches[1].level = level(26, 8); reject(f, "full_nonincreasing_batch");
  f = acute(); f.batches[3].level = level(18, 2); reject(f, "full_nonincreasing_batch");
  f = acute(); f.batches[1].level = level(1); reject(f, "full_nonincreasing_batch");
  f = acute(); f.batches[0].births.clear(); reject(f, "full_empty_batch");
  f = acute(); f.batches[0].births[0].k = 1; reject(f, "full_minimum_order");
  f = acute(); f.batches[0].births[0].p[1] = 99; reject(f, "full_minimum_points");
  f = acute(); f.batches[0].births[0].p[1] = 0; reject(f, "full_minimum_points");
  f = acute(); f.batches[0].births[0] = facet({2, 0}); reject(f, "full_minimum_points");
  f = acute(); f.batches[0].births[0].p[2] = 1; reject(f, "full_minimum_padding");
  f = acute(); f.batches[0].births.push_back(f.batches[0].births[0]); reject(f, "full_minimum_sort");
  f = paired_plateau(); std::swap(f.batches[0].births[0], f.batches[0].births[1]);
  reject(f, "full_minimum_sort");
  f = acute(); f.batches[1].births = f.batches[0].births; reject(f, "full_duplicate_minimum");
  f = acute(); f.batches[3].merges = {{}}; reject(f, "full_not_multifusion");
  f = acute(); f.batches[3].merges = {{0}}; reject(f, "full_not_multifusion");
  f = acute(); f.batches[3].merges = {{0, 0}}; reject(f, "full_parent_order");
  f = acute(); f.batches[3].merges = {{1, 0}}; reject(f, "full_parent_order");
  f = paired_plateau(); std::swap(f.batches[1].merges[0], f.batches[1].merges[1]);
  reject(f, "full_merge_sort");
  f = paired_plateau(); f.batches[1].merges[1] = f.batches[1].merges[0]; reject(f, "full_merge_sort");
  f = paired_plateau(); f.batches[1].merges[1] = {1, 2}; reject(f, "full_parent_not_prebatch_root");
  f = paired_plateau(); f.batches[2].merges = {{0, 4}}; reject(f, "full_parent_not_prebatch_root");
  f = acute(); f.batches[3].merges = {{0, 3}}; reject(f, "full_parent_not_prebatch_root");
  f = acute(); f.batches[3].merges = {{0, std::numeric_limits<FullNodeId>::max()}};
  reject(f, "full_parent_not_prebatch_root");
  f = paired_plateau(); f.batches[1].births = {facet({1, 2})}; f.batches[1].merges = {{0, 4}};
  reject(f, "full_parent_not_prebatch_root");
  f = paired_plateau(); f.batches[1].merges = {{0, 1}, {2, 4}};
  reject(f, "full_parent_not_prebatch_root");
  f = acute(); f.batches = {{level(1), {}, {{0, 1}}}}; reject(f, "full_no_minimum");
  f = acute(); f.batches[0].level = level(0); reject(f, "full_positive_level_required");

  Fixture k1{1, {0, 1}, {{level(0), {facet({0}), facet({1})}, {}}}};
  f = k1; f.batches[0].level = level(1); reject(f, "full_k1_roots");
  f = k1; f.batches[0].births.pop_back(); reject(f, "full_k1_roots");
  f = k1; f.batches[0].merges = {{0, 1}}; reject(f, "full_k1_roots");
  f = k1; f.batches.push_back({level(1), {facet({0})}, {}}); reject(f, "full_k1_late_birth");

  reject(acute(), "full_batch_budget", Status::kResourceExhausted, {0, 4, 3});
  reject(acute(), "full_batch_budget", Status::kResourceExhausted, {3, 4, 3});
  reject(acute(), "full_node_budget", Status::kResourceExhausted, {4, 0, 3});
  reject(acute(), "full_node_budget", Status::kResourceExhausted, {4, 3, 3});
  reject(acute(), "full_parent_budget", Status::kResourceExhausted, {4, 4, 0});
  reject(acute(), "full_parent_budget", Status::kResourceExhausted, {4, 4, 2});

  const auto a = build(acute()); accepted(a, "positive sentinel for refusal/read tests");
  if (a.status != Status::kOk) return;
  const auto snapshot = build(acute());
  reject_read(full_certificate_roots_at(FullCertificate{}, level(1), true, 10),
              "full_invalid_read", Status::kInvalidInput);
  reject_read(full_certificate_roots_at(a.value, level(1, 0), true, 10),
              "full_invalid_read", Status::kInvalidInput);
  reject_read(full_certificate_roots_at(a.value, level(1, -1), true, 10),
              "full_invalid_read", Status::kInvalidInput);
  reject_read(full_certificate_roots_at(a.value, level(0), true, 0), "full_read_node_budget");
  reject_read(full_certificate_roots_at(a.value, level(100), true, 3), "full_read_node_budget");
  reject_read(full_certificate_coverage(FullCertificate{}, 0, 10, 10),
              "full_invalid_read", Status::kInvalidInput);
  reject_read(full_certificate_coverage(a.value, 4, 10, 10), "full_invalid_read", Status::kInvalidInput);
  reject_read(full_certificate_coverage(a.value, std::numeric_limits<FullNodeId>::max(), 10, 10),
              "full_invalid_read", Status::kInvalidInput);
  reject_read(full_certificate_coverage(a.value, 0, 0, 2), "full_read_node_budget");
  reject_read(full_certificate_coverage(a.value, 3, 1, 6), "full_read_node_budget");
  reject_read(full_certificate_coverage(a.value, 3, 3, 6), "full_read_node_budget");
  reject_read(full_certificate_coverage(a.value, 0, 1, 0), "full_read_point_budget");
  reject_read(full_certificate_coverage(a.value, 0, 1, 1), "full_read_point_budget");
  reject_read(full_certificate_coverage(a.value, 3, 4, 5), "full_read_point_budget");
  coverage(a.value, 3, 4, 6, {0, 1, 2});
  check(snapshot.status == Status::kOk && same_certificate(a.value, snapshot.value),
        "all invalid/exhausted reads preserve the complete certificate");

  const auto p = build(paired_plateau());
  if (p.status == Status::kOk) {
    // The cap counts every scheduled node, not just depth or current stack.
    reject_read(full_certificate_coverage(p.value, 6, 6, 8), "full_read_node_budget");
    coverage(p.value, 6, 7, 8, {0, 1, 2, 3, 4, 5});
  } else check(false, "nested prospective read fixture built");
  const Fixture overlapping{2, {0, 1, 2},
      {{level(1), {facet({0, 2}), facet({1, 2})}, {}}, {level(2), {}, {{0, 1}}}}};
  const auto shared = build(overlapping);
  if (shared.status == Status::kOk) {
    reject_read(full_certificate_coverage(shared.value, 2, 3, 3), "full_read_point_budget");
    coverage(shared.value, 2, 3, 4, {0, 1, 2});
  } else check(false, "overlapping point-reference budget fixture built");
}

struct FaultGuard { ~FaultGuard() { allocation_fault::reset(); } };
template <typename Operation, typename Inspect>
void allocation_sweep(Operation operation, Inspect inspect) {
  allocation_fault::reset();
  allocation_fault::calls = 0;
  size_t count = 0;
  {
    FaultGuard guard;
    allocation_fault::counting = true;
    auto success = operation();
    allocation_fault::reset();
    count = allocation_fault::calls;
    check(success.status == FullCertificateStatus::kOk, "allocation census reaches real successful operation");
  }
  check(count > 0 && count <= 64, "bounded non-vacuous allocation census");
  if (count == 0 || count > 64) return;
  for (size_t at = 0; at < count; ++at) {
    allocation_fault::denied = 0;
    FaultGuard guard;
    allocation_fault::remaining = static_cast<long long>(at);
    try {
      auto failed = operation();
      const bool observed = allocation_fault::denied != 0;
      allocation_fault::reset();
      ++allocation_failures;
      check(observed, "persistent allocation failure actually reached below API");
      inspect(failed);
    } catch (...) {
      allocation_fault::reset();
      check(false, "allocation refusal must return without escaping the API");
    }
  }
}
void allocation_gate() {
  const auto input = acute();
  const auto sentinel = build(input);
  accepted(sentinel, "persistent allocation failure sentinel");
  if (sentinel.status != FullCertificateStatus::kOk) return;
  const auto snapshot = build(input);
  allocation_sweep([&]() { return build(input, {4, 4, 3}); }, [](const FullBuildResult& r) {
    check(r.status == FullCertificateStatus::kResourceExhausted &&
          std::strcmp(r.reason, "full_allocation_failed") == 0 && empty(r.value),
          "allocation build refusal exposes empty arenas and no valid order");
  });
  allocation_sweep([&]() { return full_certificate_roots_at(sentinel.value, level(9), true, 4); },
      [](const FullReadResult<FullNodeId>& r) {
        check(r.status == FullCertificateStatus::kResourceExhausted &&
              std::strcmp(r.reason, "full_read_allocation_failed") == 0 && r.values.empty(),
              "allocation roots refusal clears partial values");
      });
  allocation_sweep([&]() { return full_certificate_coverage(sentinel.value, 3, 4, 6); },
      [](const FullReadResult<PointId>& r) {
        check(r.status == FullCertificateStatus::kResourceExhausted &&
              std::strcmp(r.reason, "full_read_allocation_failed") == 0 && r.values.empty(),
              "allocation coverage refusal clears partial values");
      });
  check(snapshot.status == FullCertificateStatus::kOk && same_certificate(sentinel.value, snapshot.value),
        "allocation sweeps preserve pre-existing certificate entirely");
  const auto retry = build(input, {4, 4, 3});
  check(retry.status == FullCertificateStatus::kOk && same_certificate(retry.value, snapshot.value),
        "fresh explicit post-failure call produces the exact original value");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string_view mode(argv[1]);
  if (mode != "--selftest" && mode != "--rejects") return 2;
  try {
    positive_gate();
    if (mode == "--rejects") { rejection_gate(); allocation_gate(); }
  } catch (const std::exception& error) {
    allocation_fault::reset();
    std::fprintf(stderr, "unexpected=%s\n", error.what());
    return 1;
  } catch (...) {
    allocation_fault::reset();
    std::fprintf(stderr, "unexpected=nonstandard_exception\n");
    return 1;
  }
  const bool floor = positive_cases >= 11 && checks >= 55 &&
      (mode != "--rejects" || (rejected_builds >= 45 && rejected_reads >= 15 && allocation_failures >= 10));
  std::printf("full_certificate mode=%s authority=%s positives=%u build_refusals=%u read_refusals=%u allocation_failures=%u checks=%u failures=%u floor=%d\n",
              argv[1], kFullCertificateAuthority, positive_cases, rejected_builds, rejected_reads,
              allocation_failures, checks, failures, static_cast<int>(floor));
  return failures != 0 ? 1 : floor ? 0 : 3;
}
