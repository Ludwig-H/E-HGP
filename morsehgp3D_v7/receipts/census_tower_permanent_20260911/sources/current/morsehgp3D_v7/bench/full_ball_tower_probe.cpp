// Retained FULL tower (horizontal + vertical), relative to the generated exact
// census under S1. A diagnostic, NOT the industrial archive/performance gate.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/forest/full_ball_tower.hpp"
#include "../src/pipeline/generate.hpp"
#include "full_gabriel_semantic_digest.hpp"
#ifdef MHGP7_FULL_BALL_CUDA
#include "../src/gpu/census_route.cuh"
#endif

#ifdef MHGP7_TESTING
#error This probe requires nominal product primitives
#endif

namespace {
using namespace mhgp7;
using Clock = std::chrono::steady_clock;
struct Options { i64 n = 0, s = 0, kmax = 0, threads = 0, static_threads = 0; };
bool options(int argc, char** argv, Options& result) {
  unsigned seen = 0;
  for (int a = 1; a < argc; ++a) {
    i64* out = nullptr; const char* value = nullptr; unsigned bit = 0;
    if (std::strncmp(argv[a], "--n=", 4) == 0) { out = &result.n; value = argv[a] + 4; bit = 1; }
    if (std::strncmp(argv[a], "--s=", 4) == 0) { out = &result.s; value = argv[a] + 4; bit = 2; }
    if (std::strncmp(argv[a], "--kmax=", 7) == 0) { out = &result.kmax; value = argv[a] + 7; bit = 4; }
    if (std::strncmp(argv[a], "--threads=", 10) == 0) { out = &result.threads; value = argv[a] + 10; bit = 8; }
    if (std::strncmp(argv[a], "--static-threads=", 17) == 0) { out = &result.static_threads; value = argv[a] + 17; bit = 16; }
    if (!out || (seen & bit) || !parse_i64_exact(value, out)) return false;
    seen |= bit;
  }
  return (seen & 15) == 15 && result.n > 0 && result.n <= std::numeric_limits<i32>::max() &&
      result.s >= 8 && result.s <= 32767 && result.kmax >= 1 && result.kmax <= kFacetMaxK &&
      result.threads >= 1 && result.threads <= std::numeric_limits<int>::max() &&
      result.static_threads >= 0 && result.static_threads <= std::numeric_limits<int>::max();
}
double elapsed(Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); }
template<class Function> double stage(const char* name, Function&& operation) {
  std::fprintf(stderr, "stage_start=%s\n", name); std::fflush(stderr);
  const auto start = Clock::now(); operation();
  const double seconds = elapsed(start);
  std::fprintf(stderr, "stage_complete=%s seconds=%.9f\n", name, seconds); std::fflush(stderr);
  return seconds;
}
std::string digest(const FullBallTowerResult& tower) {
  // Deterministic dense payload fingerprint, not a canonical geometric oracle.
  Sha256 hash; hash.tag("mhgp7-full-ball-dense-payload-v1"); hash.u64le(tower.orders.size());
  for (const auto& order : tower.orders) {
    const auto& forest = order.forest;
    hash.u64le(forest.order()); hash.u64le(forest.nodes().size());
    for (const auto& node : forest.nodes()) {
      full_probe_digest::level_wire(hash, node.level); hash.u64le(node.parent_count);
      for (u64 j = 0; j < node.parent_count; ++j) hash.u64le(forest.parents()[node.first + j]);
    }
    hash.u64le(forest.contributions().size());
    for (const auto& record : forest.contributions()) {
      full_probe_digest::level_wire(hash, record.level); hash.u64le(record.segment);
      const auto& row = forest.populations()->rows()[record.ref.population];
      hash.u64le(record.ref.include_interior ? row.interior.size() : 0);
      if (record.ref.include_interior) for (PointId id : row.interior) hash.u64le(id);
      hash.u64le(std::popcount(record.ref.shell_mask));
      for (size_t bit = 0; bit < row.shell.size(); ++bit)
        if (record.ref.shell_mask & (u16{1} << bit)) hash.u64le(row.shell[bit]);
    }
    for (u64 target : order.lower_nodes) hash.u64le(target);
  }
  return hash.hex();
}
}

int main(int argc, char** argv) {
  Options opt;
  if (!options(argc, argv, opt)) return 2;
  const auto start = Clock::now();
  try {
    auto input = make_family_input(CloudFamily::kUniform, static_cast<int>(opt.n), 65536, 3);
    full_ball_detail::require(input.size() == static_cast<size_t>(opt.n), "probe_incomplete_input");
    const auto input_digest = full_probe_digest::input(input);
    CloudIndex ix;
    const double index_s = stage("index", [&] { ix = build_cloud_index(input); });
    full_ball_detail::require(ix.valid && !ix.has_duplicate_positions(), "probe_invalid_index");
    const unsigned kmax = static_cast<unsigned>(std::min<i64>(opt.kmax, opt.n));
    const u64 smax = std::min<u64>(kmax + 1, ix.input_count);
    std::vector<BallCandidate> candidates;
    GenerateOptions go; go.s = opt.s; go.smax = smax; go.threads = static_cast<int>(opt.threads);
    GenerateStats gen;
    const double generate_s = stage("generate", [&] { generate_candidates(ix, go, &candidates, &gen); });
    full_ball_detail::require(gen.cap_refus == kCapRefusNone && !gen.invariant_jneg, "probe_generation_refused");
    const u128 mass = expected_pair_mass(ix);
    for (size_t q = 0; q < 3; ++q)
      full_ball_detail::require(gen.ledger_emitted_mass[q] + gen.ledger_killed_mass[q] == mass, "probe_pair_mass_ledger");
    const auto raw = candidates.size();
    const double sort_s = stage("sort_rle", [&] { rle_candidates(&candidates, static_cast<int>(opt.threads)); });
    full_ball_detail::require(candidates_capacity_ok(candidates.size()), "probe_candidate_representation");
    const auto unique = candidates.size();
    std::vector<Survivor> survivors; std::vector<BallData> balls; ExpandStats census_stats;
#ifdef MHGP7_FULL_BALL_CUDA
    constexpr const char* backend = "cuda_census_cpu_full";
    gpu::CensusRouteCosts device_costs;
    const double prefilter_s = 0;  // included in the joint CUDA census stage
    const double census_s = stage("cuda_prefilter_census", [&] {
      // Terminal singleton: no geometric candidate and no device work exists.
      if (smax == 1 && ix.input_count == 1 && candidates.empty()) return;
      const auto error = gpu::device_prefilter_census_route(ix, candidates, smax, kBallShellMax,
          &survivors, &balls, &census_stats, 262144, &device_costs);
      if (!error.empty()) throw std::runtime_error(error);
    });
#else
    constexpr const char* backend = "cpu_reference";
    const double prefilter_s = stage("prefilter", [&] {
      prefilter_balls(ix, candidates, smax, static_cast<int>(opt.threads), &survivors, &census_stats);
    });
    const double census_s = stage("census", [&] {
      full_ball_detail::require(census_balls(ix, candidates, survivors, smax, kBallShellMax,
          static_cast<int>(opt.threads), &balls, &census_stats) == PipelineStatus::kCompleteRegular,
          "probe_census_refused");
    });
#endif
    std::vector<Survivor>().swap(survivors); std::vector<BallCandidate>().swap(candidates);
    FullBallTowerResult tower;
    const double tower_s = stage("full_ball_tower", [&] {
      tower = build_full_ball_tower(ix, balls, kmax, static_cast<int>(opt.static_threads));
    });
    full_ball_detail::require(tower.status == FullBallStatus::kCompleteRelative, tower.reason, tower.status);
    std::string payload_digest;
    const double digest_s = stage("payload_digest", [&] { payload_digest = digest(tower); });
    u64 nodes = 0, parent_refs = 0, contributions = 0, vertical_refs = 0;
    for (const auto& order : tower.orders) {
      nodes += order.forest.nodes().size(); parent_refs += order.forest.parents().size();
      contributions += order.forest.contributions().size();
      if (order.forest.order() > 1) vertical_refs += order.lower_nodes.size();
    }
    const auto& st = tower.stats;
    u64 resolver_supports = 0;
    for (u64 count : st.resolve_work.supports_by_size) full_ball_detail::add(resolver_supports, count);
    std::printf("{\"schema\":\"mhgp7-full-ball-tower-probe-v1\",\"status\":\"completed_relative\","
        "\"public_status\":\"not_claimed\",\"backend\":\"%s\",\"contract_qualified\":false,"
        "\"n\":%lld,\"s\":%lld,\"kmax\":%u,\"threads\":%lld,\"seed\":3,\"coord\":65536,"
        "\"orders\":%zu,\"raw\":%zu,\"unique\":%zu,\"balls\":%zu,\"nodes\":%llu,\"parent_refs\":%llu,"
        "\"contributions\":%llu,\"vertical_refs\":%llu,\"extra_records\":%llu,\"anchor_blocks\":%llu,"
        "\"representatives\":%llu,\"anchor_hits\":%llu,\"intruder_queries\":%llu,\"same_radius_steps\":%llu,"
        "\"validation_meb_calls\":%llu,\"resolver_meb_calls\":%llu,\"input_digest\":\"%s\",\"payload_digest\":\"%s\","
        "\"declared_support_checks\":%llu,\"singleton_lots\":%llu,\"grouped_lots\":%llu,\"lot_dsu_slots\":%llu,"
        "\"lower_edges_indexed\":%llu,\"lower_nodes_activated\":%llu,\"lower_edges_activated\":%llu,"
        "\"lower_queries\":%llu,\"lower_find_steps\":%llu,\"lower_path_writes\":%llu,"
        "\"index_s\":%.9f,\"generate_s\":%.9f,\"sort_s\":%.9f,\"prefilter_s\":%.9f,\"census_s\":%.9f,"
        "\"tower_s\":%.9f,\"digest_s\":%.9f,\"total_s\":%.9f",
        backend, static_cast<long long>(opt.n), static_cast<long long>(opt.s), kmax, static_cast<long long>(opt.threads),
        tower.orders.size(), raw, unique, balls.size(), static_cast<unsigned long long>(nodes),
        static_cast<unsigned long long>(parent_refs), static_cast<unsigned long long>(contributions),
        static_cast<unsigned long long>(vertical_refs), static_cast<unsigned long long>(st.extra_records),
        static_cast<unsigned long long>(st.anchor_blocks), static_cast<unsigned long long>(st.representatives),
        static_cast<unsigned long long>(st.anchor_hits), static_cast<unsigned long long>(st.intruder_queries),
        static_cast<unsigned long long>(st.same_radius_steps), static_cast<unsigned long long>(st.validation_work.calls),
        static_cast<unsigned long long>(st.resolve_work.calls), input_digest.c_str(), payload_digest.c_str(),
        static_cast<unsigned long long>(st.declared_support_checks), static_cast<unsigned long long>(st.singleton_lots),
        static_cast<unsigned long long>(st.grouped_lots), static_cast<unsigned long long>(st.lot_dsu_slots),
        static_cast<unsigned long long>(st.lower_edges_indexed), static_cast<unsigned long long>(st.lower_nodes_activated),
        static_cast<unsigned long long>(st.lower_edges_activated), static_cast<unsigned long long>(st.lower_queries),
        static_cast<unsigned long long>(st.lower_find_steps), static_cast<unsigned long long>(st.lower_path_writes),
        index_s, generate_s, sort_s, prefilter_s, census_s, tower_s, digest_s, elapsed(start));
    std::printf(",\"resolver_cache_accounting\":\"%s\",\"residence_accounting\":\"%s\","
        "\"resolver_cache_queries\":%llu,\"resolver_cache_hits\":%llu,\"resolver_cache_stores\":%llu,"
        "\"resolver_cache_evictions\":%llu,\"resolver_cache_seed_stores\":%llu,"
        "\"resolver_cache_slots\":%llu,\"resolver_cache_reset_slots\":%llu,"
        "\"resolver_cache_bytes\":%llu,\"resolver_cache_released_slots\":%llu,"
        "\"resolver_supports_tested\":%llu",
        opt.static_threads ? kFullBallStaticResolverAccounting : kFullBallResolverCacheAccounting,
        kFullBallResidenceAccounting,
        static_cast<unsigned long long>(st.resolver_cache_queries), static_cast<unsigned long long>(st.resolver_cache_hits),
        static_cast<unsigned long long>(st.resolver_cache_stores), static_cast<unsigned long long>(st.resolver_cache_evictions),
        static_cast<unsigned long long>(st.resolver_cache_seed_stores), static_cast<unsigned long long>(st.resolver_cache_slots),
        static_cast<unsigned long long>(st.resolver_cache_reset_slots), static_cast<unsigned long long>(st.resolver_cache_bytes),
        static_cast<unsigned long long>(st.resolver_cache_released_slots),
        static_cast<unsigned long long>(resolver_supports));
    std::printf(",\"static_threads\":%lld,"
        "\"static_worker_threads_created_in_completed_pools\":%llu,\"static_lanes_used\":%llu,"
        "\"static_request_capacity_peak_bytes\":%llu,\"static_target_capacity_peak_bytes\":%llu,"
        "\"static_seed_capacity_peak_bytes\":%llu,\"static_group_capacity_peak_bytes\":%llu,"
        "\"static_worker_capacity_peak_bytes\":%llu,\"static_sampled_retained_capacity_peak_bytes\":%llu,"
        "\"static_orders\":[",
        static_cast<long long>(opt.static_threads), static_cast<unsigned long long>(st.static_workers_created),
        static_cast<unsigned long long>(st.static_lanes_used),
        static_cast<unsigned long long>(st.static_peak_request_bytes),
        static_cast<unsigned long long>(st.static_peak_target_bytes),
        static_cast<unsigned long long>(st.static_peak_seed_bytes),
        static_cast<unsigned long long>(st.static_peak_group_bytes),
        static_cast<unsigned long long>(st.static_peak_worker_bytes),
        static_cast<unsigned long long>(st.static_peak_retained_bytes));
    for (unsigned k = 1; k <= kmax; ++k)
      std::printf("%s{\"K\":%u,\"requests\":%llu,\"unique\":%llu,\"seeded_unique\":%llu,"
          "\"post_seed_queries\":%llu,\"post_seed_hits\":%llu,\"post_seed_terminals\":%llu}",
          k == 1 ? "" : ",", k, static_cast<unsigned long long>(st.static_requests[k]),
          static_cast<unsigned long long>(st.static_unique[k]),
          static_cast<unsigned long long>(st.static_seeded[k]),
          static_cast<unsigned long long>(st.static_post_seed_queries[k]),
          static_cast<unsigned long long>(st.static_post_seed_hits[k]),
          static_cast<unsigned long long>(st.static_post_seed_terminals[k]));
    std::printf("]");
#ifdef MHGP7_FULL_BALL_CUDA
    const auto& c = device_costs;
    std::printf(",\"cuda_route\":\"%s\",\"cuda_cold_wrapper\":true,\"cuda_lots\":%llu,"
        "\"cuda_index_wire_ms\":%.9f,\"cuda_pack_ms\":%.9f,\"cuda_alloc_ms\":%.9f,"
        "\"cuda_h2d_ms\":%.9f,\"cuda_kernel_ms\":%.9f,\"cuda_d2h_ms\":%.9f,"
        "\"cuda_rebuild_ms\":%.9f,\"cuda_release_ms\":%.9f,\"cuda_peak_bytes\":%llu,"
        "\"cuda_h2d_index_bytes\":%llu,\"cuda_h2d_ball_bytes\":%llu,"
        "\"cuda_h2d_sentinel_bytes\":%llu,\"cuda_d2h_bytes\":%llu",
        gpu::kCensusRouteVersion, static_cast<unsigned long long>(c.lots_launched),
        c.index_wire_ms, c.pack_ms, c.setup_alloc_ms, c.h2d_ms, c.kernel_ms, c.d2h_ms,
        c.rebuild_ms, c.release_ms, static_cast<unsigned long long>(c.peak_device_bytes),
        static_cast<unsigned long long>(c.h2d_index_bytes), static_cast<unsigned long long>(c.h2d_ball_bytes),
        static_cast<unsigned long long>(c.h2d_sentinel_bytes), static_cast<unsigned long long>(c.d2h_bytes));
#endif
    std::printf("}\n");
    return std::fflush(stdout) == 0 ? 0 : 2;
  } catch (const full_ball_detail::Failure& error) {
    std::fprintf(stderr, "{\"status\":\"failed\",\"reason\":\"%s\",\"elapsed_s\":%.9f}\n", error.reason, elapsed(start));
    return 2;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "exception=%s elapsed_s=%.9f\n", error.what(), elapsed(start)); return 2;
  }
}
