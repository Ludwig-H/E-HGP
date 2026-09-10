#!/usr/bin/env python3
"""Only private probe copies gain physical cache/support/output-arena counters."""
from pathlib import Path

BASE = Path('/workspaces/E-HGP/build/v7_combined_resolver_20260910')
common = r'''
    u64 supports = 0, arena_bytes = 0;
    for (u64 count : st.resolve_work.supports_by_size) supports += count;
    for (const auto& order : tower.orders) {
      const auto& forest = order.forest;
      arena_bytes += forest.nodes().capacity() * sizeof(FullNode);
      arena_bytes += (forest.parents().capacity() + forest.successors().capacity() +
          order.lower_nodes.capacity()) * sizeof(FullNodeId);
      arena_bytes += forest.contributions().capacity() * sizeof(FullDatedContribution);
    }
    std::printf(",\"resolver_supports\":%llu,\"resolver_power_tests\":%llu,"
        "\"output_arenas_with_vertical_capacity_bytes\":%llu",
        static_cast<unsigned long long>(supports), static_cast<unsigned long long>(st.resolve_work.power_tests),
        static_cast<unsigned long long>(arena_bytes));
'''
combined = r'''
    std::printf(",\"resolver_cache_accounting\":\"%s\",\"cache_queries\":%llu,\"cache_hits\":%llu,"
        "\"cache_stores\":%llu,\"cache_evictions\":%llu,\"cache_seed_stores\":%llu,"
        "\"cache_slots\":%llu,\"cache_reset_slots\":%llu,\"cache_bytes\":%llu,\"cache_released_slots\":%llu",
        kFullBallResolverCacheAccounting, static_cast<unsigned long long>(st.resolver_cache_queries),
        static_cast<unsigned long long>(st.resolver_cache_hits), static_cast<unsigned long long>(st.resolver_cache_stores),
        static_cast<unsigned long long>(st.resolver_cache_evictions), static_cast<unsigned long long>(st.resolver_cache_seed_stores),
        static_cast<unsigned long long>(st.resolver_cache_slots), static_cast<unsigned long long>(st.resolver_cache_reset_slots),
        static_cast<unsigned long long>(st.resolver_cache_bytes), static_cast<unsigned long long>(st.resolver_cache_released_slots));
'''
for kind in ['baseline', 'combined']:
    probe = BASE / kind / 'bench/full_ball_tower_probe.cpp'
    source = probe.read_text()
    site = '    std::printf("}\\n");'
    if source.count(site) != 1:
        raise RuntimeError('probe instrumentation site mismatch')
    probe.write_text(source.replace(site, common + (combined if kind == 'combined' else '') + site))
