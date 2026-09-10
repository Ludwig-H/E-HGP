// Exact cache/work checks reuse the nominal gate's independent rational census.
// Its main is not called; no geometric authority or oracle is duplicated.
#define main mhgp7_embedded_facet_resolver_nominal_main
#include "full_ball_tower_gate.cpp"
#undef main

#include <cstdlib>

namespace cache_allocation_fault {
bool fail_next = false;
[[gnu::noinline]] void* allocate(size_t n) {
  if (fail_next) { fail_next = false; throw std::bad_alloc(); }
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
[[gnu::noinline]] void release(void* p) noexcept { std::free(p); }
}
void* operator new(size_t n) { return cache_allocation_fault::allocate(n); }
void* operator new[](size_t n) { return cache_allocation_fault::allocate(n); }
void* operator new(size_t n, const std::nothrow_t&) noexcept {
  try { return cache_allocation_fault::allocate(n); } catch (...) { return nullptr; }
}
void* operator new[](size_t n, const std::nothrow_t&) noexcept {
  try { return cache_allocation_fault::allocate(n); } catch (...) { return nullptr; }
}
void operator delete(void* p) noexcept { cache_allocation_fault::release(p); }
void operator delete[](void* p) noexcept { cache_allocation_fault::release(p); }
void operator delete(void* p, size_t) noexcept { cache_allocation_fault::release(p); }
void operator delete[](void* p, size_t) noexcept { cache_allocation_fault::release(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { cache_allocation_fault::release(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { cache_allocation_fault::release(p); }

namespace {
using Cache = full_ball_detail::ResolverCache;
Cache::Key cache_key(i32 first) {
  Cache::Key result{}; result[0] = first; result[1] = first + 1; return result;
}
void check_cache() {
  FullBallStats stats;
  Cache cache(stats);
  need(cache.lookup(cache_key(0)) == kFullCoverageAbsent, "cache.disabled_miss");
  cache.configure(std::numeric_limits<size_t>::max());
  need(stats.resolver_cache_slots == 0, "cache.unrepresentable_capacity_is_optional");
  cache_allocation_fault::fail_next = true;
  cache.configure(1);
  need(!cache_allocation_fault::fail_next && stats.resolver_cache_slots == 0 &&
      cache.lookup(cache_key(0)) == kFullCoverageAbsent, "cache.allocation_failure_is_optional");
  cache.configure(1);
  need(stats.resolver_cache_slots == 16 &&
      stats.resolver_cache_bytes == 16 * sizeof(full_ball_detail::ResolverCacheEntry), "cache.linear_layout");
  cache.reset();
  need(cache.lookup(cache_key(0)) == kFullCoverageAbsent, "cache.cold_miss");
  cache.store(cache_key(0), 700);
  need(cache.lookup(cache_key(0)) == 700, "cache.exact_hit");
  cache.store(cache_key(0), 701);
  need(cache.lookup(cache_key(0)) == 701, "cache.exact_overwrite");
  for (i32 i = 1; i <= 32; ++i) {
    cache.store(cache_key(i), static_cast<u64>(i) + 1000);
    need(cache.lookup(cache_key(i)) == static_cast<u64>(i) + 1000, "cache.collision_new_key_hit");
    for (i32 old = 0; old < i; ++old) {
      const u64 answer = cache.lookup(cache_key(old));
      need(answer == kFullCoverageAbsent || answer == (old == 0 ? 701 : static_cast<u64>(old) + 1000),
          "cache.collision_never_returns_other_key_token");
    }
  }
  need(stats.resolver_cache_evictions > 0, "cache.collision_nonvacuity");
  cache.reset();
  for (i32 i = 0; i <= 32; ++i)
    need(cache.lookup(cache_key(i)) == kFullCoverageAbsent, "cache.reset_removes_old_tokens");
  cache.store(cache_key(42), 333, true);
  need(cache.lookup(cache_key(42)) == 333 && stats.resolver_cache_seed_stores == 1,
      "cache.seed_exact_token");
  need(stats.resolver_cache_reset_slots == 32, "cache.physical_reset_slots");
  cache.release();
  need(stats.resolver_cache_released_slots == 16 && stats.resolver_cache_slots == 16 &&
      cache.lookup(cache_key(42)) == kFullCoverageAbsent, "cache.released_capacity_accounted");
  cache.release();
  need(stats.resolver_cache_released_slots == 16, "cache.second_release_is_empty");
}

void check_builder_work() {
  u64 clouds = 0, hits = 0, seeds = 0, released = 0;
  for (const auto& fixture : fixtures()) for (unsigned variant = 0; variant < 2; ++variant) {
    const auto in = input(fixture, variant);
    std::vector<P3> points;
    for (const auto& point : in) points.push_back(point.position);
    const oracle::Model model(points);
    const auto ix = build_cloud_index(in);
    auto balls = catalogue(points, ix, model, fixture.kmax);
    if (variant) std::reverse(balls.begin(), balls.end());
    const auto result = build_full_ball_tower(ix, balls, fixture.kmax);
    need(result.status == FullBallStatus::kCompleteRelative, "cache.builder_complete");
    const auto& st = result.stats;
    const auto kmax = static_cast<unsigned>(result.orders.size());
    u64 expected_slots = kmax > 1 ? 1 : 0, expected_seeds = 0;
    if (kmax > 1) while (expected_slots < 16 * points.size()) expected_slots *= 2;
    for (const auto& ball : balls) {
      const unsigned cardinality = ball.n_interior + ball.n_shell;
      if (cardinality >= 2 && cardinality <= kmax) ++expected_seeds;
    }
    need(st.resolver_cache_slots == expected_slots &&
        st.resolver_cache_released_slots == expected_slots &&
        st.resolver_cache_bytes == expected_slots * sizeof(full_ball_detail::ResolverCacheEntry),
        "cache.builder_linear_capacity_released");
    need(st.resolver_cache_reset_slots == expected_slots * (kmax - 1), "cache.builder_reset_work");
    need(st.resolver_cache_seed_stores == expected_seeds, "cache.only_unique_closed_population_seeded");
    need(st.resolver_cache_queries == st.resolver_cache_hits + st.anchor_hits,
        "cache.each_lookup_has_one_disposition");
    need(st.resolve_work.calls == st.anchor_hits + st.intruder_queries,
        "cache.MEB_calls_are_actual_misses_and_descents");
    need(st.resolver_cache_stores == st.anchor_hits + st.resolver_cache_seed_stores,
        "cache.stores_are_actual_resolutions_and_seeds");
    ++clouds; hits += st.resolver_cache_hits; seeds += st.resolver_cache_seed_stores;
    released += st.resolver_cache_released_slots;
  }
  need(clouds == 28 && hits > 0 && seeds > 0 && released > 0, "cache.builder_nonvacuity");
  std::printf("{\"status\":\"passed\",\"clouds\":%llu,\"cache_hits\":%llu,\"seeds\":%llu,"
      "\"released_slots\":%llu,\"entry_bytes\":%zu}\n", static_cast<unsigned long long>(clouds),
      static_cast<unsigned long long>(hits), static_cast<unsigned long long>(seeds),
      static_cast<unsigned long long>(released), sizeof(full_ball_detail::ResolverCacheEntry));
}
}

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try { check_cache(); check_builder_work(); return 0; }
  catch (const Failure& failure) { std::fprintf(stderr, "FAIL cache: %s\n", failure.why); return 1; }
  catch (const full_ball_detail::Failure& failure) { std::fprintf(stderr, "%s\n", failure.reason); return 1; }
  catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
