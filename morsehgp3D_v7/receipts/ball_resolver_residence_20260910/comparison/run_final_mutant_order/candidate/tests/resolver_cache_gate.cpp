#include <cstdio>
#include <string_view>
#include "../src/forest/full_ball_tower.hpp"
namespace {
using namespace mhgp7;
void need(bool value, const char* why) { if (!value) throw why; }
using Cache = full_ball_detail::ResolverCache;
Cache::Key key(i32 first) { Cache::Key result{}; result[0] = first; result[1] = first + 1; return result; }
void run() {
  FullBallStats stats;
  Cache cache(stats);
  need(cache.lookup(key(0)) == kFullCoverageAbsent, "disabled_cache_miss");
  cache.configure(std::numeric_limits<size_t>::max());
  need(stats.resolver_cache_slots == 0 && cache.lookup(key(0)) == kFullCoverageAbsent,
       "unrepresentable_optional_cache_remains_disabled");
  cache.configure(1);
  need(stats.resolver_cache_slots == 16 && stats.resolver_cache_bytes == 16 * sizeof(full_ball_detail::ResolverCacheEntry),
       "linear_capacity_layout");
  cache.reset();
  need(cache.lookup(key(0)) == kFullCoverageAbsent, "cold_miss");
  cache.store(key(0), 700);
  need(cache.lookup(key(0)) == 700, "exact_hit");
  cache.store(key(0), 701);
  need(cache.lookup(key(0)) == 701, "exact_overwrite");
  for (i32 i = 1; i <= 32; ++i) {
    cache.store(key(i), static_cast<u64>(i) + 1000);
    need(cache.lookup(key(i)) == static_cast<u64>(i) + 1000, "collision_new_key_hit");
    for (i32 old = 0; old < i; ++old) {
      const u64 answer = cache.lookup(key(old));
      need(answer == kFullCoverageAbsent || answer == (old == 0 ? 701 : static_cast<u64>(old) + 1000),
           "collision_never_returns_other_key_token");
    }
  }
  need(stats.resolver_cache_evictions > 0, "collision_nonvacuity");
  cache.reset();
  for (i32 i = 0; i <= 32; ++i)
    need(cache.lookup(key(i)) == kFullCoverageAbsent, "order_reset_clears_old_tokens");
  cache.store(key(42), 333, true);
  need(cache.lookup(key(42)) == 333 && stats.resolver_cache_seed_stores == 1, "seed_exact_token");
  need(stats.resolver_cache_reset_slots == 32, "physical_reset_slots");
  std::printf("{\"status\":\"passed\",\"entry_bytes\":%zu,\"slots\":%llu,\"evictions\":%llu}\n",
      sizeof(full_ball_detail::ResolverCacheEntry), static_cast<unsigned long long>(stats.resolver_cache_slots),
      static_cast<unsigned long long>(stats.resolver_cache_evictions));
}
}
int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try { run(); return 0; }
  catch (const char* why) { std::fprintf(stderr, "%s\n", why); return 1; }
  catch (const mhgp7::full_ball_detail::Failure& failure) { std::fprintf(stderr, "%s\n", failure.reason); return 1; }
  catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
