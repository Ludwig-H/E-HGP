// Audit L02 (lecture seule, hors depot) : grand-livre complet de la voie q2 v9
// sur des coupes u32le (LiDAR 1 mm emboitees ou familles synthetiques),
// configuration de la chaine (tower_chain.cpp:1416-1421) et ablations.
// Compile contre les sources gen du worktree ce8a649dd (chemin q2 inchange
// depuis f685461af). Sortie : une ligne JSON par execution.
#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/wspd_q2_parallel.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mhgp9::gen;

static std::vector<Point3> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.size() % 12) throw std::invalid_argument("bad length");
  std::vector<Point3> pts(bytes.size() / 12);
  for (std::size_t i = 0; i < pts.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= std::uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b);
      if (v > 262143u) throw std::invalid_argument("coordinate outside 18 bits");
      c[a] = v;
    }
    pts[i] = Point3{(Coordinate)c[0], (Coordinate)c[1], (Coordinate)c[2]};
  }
  return pts;
}

struct Config {
  const char* name;
  WspdFrontProposals proposals;
  Q2CensusMode census;
  Q2SiblingMode sibling;
  Q2WitnessOrder order;
  std::size_t pool;
};

int main(int argc, char** argv) {
  if (argc < 5) {
    std::fprintf(stderr, "usage: probe <file.u32le> <K> <config> <workers> [knn]\n");
    return 2;
  }
  const std::string file = argv[1];
  const unsigned k = static_cast<unsigned>(std::stoul(argv[2]));
  const std::string cname = argv[3];
  const std::size_t workers = std::stoul(argv[4]);
  const bool knn = argc > 5 && std::string(argv[5]) == "knn";
  const auto max = std::numeric_limits<std::size_t>::max();
  const std::array<Config, 7> configs{{
      {"prod", {2, 16, true}, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 64},
      {"w1", {1, max, false}, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 64},
      {"w4all", {4, max, true}, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 64},
      {"nosib", {2, 16, true}, Q2CensusMode::SharedBlocks, Q2SiblingMode::Disabled, Q2WitnessOrder::ComplementFirst, 64},
      {"dfs", {2, 16, true}, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::GlobalDfs, 64},
      {"nopool", {2, 16, true}, Q2CensusMode::SharedBlocks, Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, 0},
      {"bare", {2, 16, true}, Q2CensusMode::SharedBlocks, Q2SiblingMode::Disabled, Q2WitnessOrder::GlobalDfs, 0},
  }};
  const Config* config = nullptr;
  for (const auto& c : configs) if (cname == c.name) config = &c;
  if (!config) { std::fprintf(stderr, "unknown config\n"); return 2; }

  const auto points = read_u32le(file);
  const auto t0 = std::chrono::steady_clock::now();
  auto cloud = prepare_cloud(points);
  const auto t1 = std::chrono::steady_clock::now();
  const auto index = make_q2_cloud_index(cloud);
  const auto t2 = std::chrono::steady_clock::now();

  // Per-worker accumulators: shell histogram and, optionally, accepted pairs.
  struct Slot { std::array<std::uint64_t, 8> shell{}; std::array<std::uint64_t, 11> depth{};
                std::vector<std::pair<std::uint32_t, std::uint32_t>> pairs; std::vector<std::uint8_t> depths; };
  std::vector<Slot> slots(workers);
  std::vector<Q2CensusConsumer> consumers;
  for (std::size_t w = 0; w < workers; ++w) {
    auto* s = &slots[w];
    consumers.emplace_back([s, knn](const Q2Support& support) {
      s->shell[std::min<std::size_t>(support.shell.size(), 7)]++;
      s->depth[std::min<std::size_t>(support.interior.size(), 10)]++;
      if (knn) {
        s->pairs.emplace_back(static_cast<std::uint32_t>(support.a_id), static_cast<std::uint32_t>(support.b_id));
        s->depths.push_back(static_cast<std::uint8_t>(support.interior.size()));
      }
    });
  }
  WspdQ2Schedule schedule;
  schedule.mass_first = true;
  const auto r = run_wspd_q2_census_parallel(index, k, 8, WspdFrontMode::MidpointSamples, config->census, consumers,
                                             64, config->sibling, config->order, Q2AnchorMode::Individual, config->pool,
                                             schedule, config->proposals);
  const auto t3 = std::chrono::steady_clock::now();
  const auto ms = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
  std::array<std::uint64_t, 8> shell{};
  std::array<std::uint64_t, 11> depth{};
  for (const auto& s : slots) {
    for (int i = 0; i < 8; ++i) shell[i] += s.shell[i];
    for (int i = 0; i < 11; ++i) depth[i] += s.depth[i];
  }
  const auto& f = r.front.work;
  const auto& c = r.census_work;
  std::printf("{\"file\":\"%s\",\"n\":%zu,\"K\":%u,\"config\":\"%s\",\"workers\":%zu,", file.c_str(), points.size(), k,
              config->name, workers);
  std::printf("\"prepare_ms\":%.3f,\"index_ms\":%.3f,\"q2_ms\":%.3f,\"partition_ms\":%.3f,\"worker_ms_sum\":%.3f,"
              "\"payload_ms_sum\":%.3f,",
              ms(t0, t1), ms(t1, t2), ms(t2, t3), r.partition_ms, r.worker_ms_sum, r.payload_ms_sum);
  std::printf("\"rectangles\":%llu,\"anchor_queries\":%llu,\"candidates\":%llu,\"accepted\":%llu,",
              (unsigned long long)r.input_rectangles, (unsigned long long)r.anchor_queries,
              (unsigned long long)r.candidate_pairs, (unsigned long long)r.accepted_pairs);
  std::printf("\"front\":{\"products\":%llu,\"searches\":%llu,\"descent\":%llu,\"box_tests\":%llu,\"proposed\":%llu,"
              "\"h_tests\":%llu,\"credits\":%llu,\"rejected_products\":%llu,\"emitted\":%llu,\"leaf_pairs\":%llu,"
              "\"max_factor\":%llu,\"max_depth\":%llu,\"size_rect\":[%llu,%llu,%llu,%llu,%llu],"
              "\"size_mass\":[%llu,%llu,%llu,%llu,%llu],\"inherited\":%llu,\"inh_dup\":%llu,\"ext_products\":%llu},",
              (unsigned long long)f.product_visits, (unsigned long long)f.witness_searches,
              (unsigned long long)f.witness_descent_steps, (unsigned long long)f.witness_box_distance_tests,
              (unsigned long long)f.proposed_sites, (unsigned long long)f.h_bound_tests,
              (unsigned long long)f.witness_lane_credits, (unsigned long long)f.fully_rejected_products,
              (unsigned long long)f.emitted_rectangles, (unsigned long long)f.leaf_pair_rectangles,
              (unsigned long long)f.max_factor_size, (unsigned long long)f.max_product_depth,
              (unsigned long long)f.size_class_rectangles[0], (unsigned long long)f.size_class_rectangles[1],
              (unsigned long long)f.size_class_rectangles[2], (unsigned long long)f.size_class_rectangles[3],
              (unsigned long long)f.size_class_rectangles[4], (unsigned long long)f.size_class_pair_mass[0],
              (unsigned long long)f.size_class_pair_mass[1], (unsigned long long)f.size_class_pair_mass[2],
              (unsigned long long)f.size_class_pair_mass[3], (unsigned long long)f.size_class_pair_mass[4],
              (unsigned long long)f.inherited_credits, (unsigned long long)f.inherited_duplicates,
              (unsigned long long)f.extended_products);
  std::printf("\"census\":{\"query_tasks\":%llu,\"query_splits\":%llu,\"witness_splits\":%llu,\"node_visits\":%llu,"
              "\"bound_tests\":%llu,\"point_tests\":%llu,\"payload_node_visits\":%llu,\"payload_point_tests\":%llu,"
              "\"payload_interior\":%llu,\"payload_shell\":%llu},",
              (unsigned long long)c.query_tasks, (unsigned long long)c.query_splits,
              (unsigned long long)c.witness_splits, (unsigned long long)c.count_node_visits,
              (unsigned long long)c.count_bound_tests, (unsigned long long)c.count_point_tests,
              (unsigned long long)c.payload_node_visits, (unsigned long long)c.payload_point_tests,
              (unsigned long long)c.payload_interior_sites, (unsigned long long)c.payload_shell_sites);
  std::printf("\"sibling\":{\"proposals\":%llu,\"card_skips\":%llu,\"rejected_pairs\":%llu},",
              (unsigned long long)r.sibling_work.proposals, (unsigned long long)r.sibling_work.cardinality_skips,
              (unsigned long long)r.sibling_work.rejected_pairs);
  std::printf("\"order\":{\"structural_splits\":%llu,\"anchor_skips\":%llu},",
              (unsigned long long)r.order_work.structural_splits, (unsigned long long)r.order_work.anchor_skips);
  std::printf("\"pool\":{\"selected_rect\":%llu,\"selected_pairs\":%llu,\"filtered\":%llu,\"pair_roots\":%llu,"
              "\"passthrough_rect\":%llu},",
              (unsigned long long)r.pool_work.selected_rectangles, (unsigned long long)r.pool_work.selected_pairs,
              (unsigned long long)r.pool_work.filtered_pairs, (unsigned long long)r.pool_work.pair_roots,
              (unsigned long long)r.pool_work.passthrough_rectangles);
  std::printf("\"shell_hist\":[");
  for (int i = 0; i < 8; ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)shell[i]);
  std::printf("],\"depth_hist\":[");
  for (int i = 0; i < 11; ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)depth[i]);
  std::printf("]");

  if (knn) {
    // Rang de voisinage : rho_a(b) = #sites strictement plus proches de a que b.
    // Lemme (auditeur C) : p(a,b) <= min(rho_a(b), rho_b(a)). Histogramme de
    // m = min(rho_a, rho_b) en multiples de K, tronque a 64K (brute force).
    const std::size_t n = points.size();
    const std::size_t cap = std::min<std::size_t>(n - 1, 64 * k);
    std::vector<std::vector<std::int64_t>> near(n);
    std::vector<std::int64_t> d2(n);
    for (std::size_t a = 0; a < n; ++a) {
      for (std::size_t b = 0; b < n; ++b) {
        const std::int64_t dx = points[a].x - points[b].x, dy = points[a].y - points[b].y, dz = points[a].z - points[b].z;
        d2[b] = dx * dx + dy * dy + dz * dz;
      }
      d2[a] = std::numeric_limits<std::int64_t>::max();
      std::vector<std::int64_t> copy(d2);
      std::nth_element(copy.begin(), copy.begin() + (cap - 1), copy.end());
      copy.resize(cap);
      std::sort(copy.begin(), copy.end());
      near[a] = std::move(copy);
    }
    const auto rank = [&](std::size_t a, std::int64_t dist) {
      return static_cast<std::size_t>(std::lower_bound(near[a].begin(), near[a].end(), dist) - near[a].begin());
    };
    std::array<std::uint64_t, 9> hist{};  // [0,K) [K,2K) [2K,4K) [4K,8K) [8K,16K) [16K,32K) [32K,64K) >=64K(cap)
    std::uint64_t below_depth = 0;         // sanity: m >= p must hold
    for (const auto& s : slots) {
      for (std::size_t i = 0; i < s.pairs.size(); ++i) {
        const auto a = s.pairs[i].first, b = s.pairs[i].second;
        const std::int64_t dx = points[a].x - points[b].x, dy = points[a].y - points[b].y, dz = points[a].z - points[b].z;
        const auto dist = dx * dx + dy * dy + dz * dz;
        const auto m = std::min(rank(a, dist), rank(b, dist));
        if (m < s.depths[i]) ++below_depth;
        int bin = 0;
        if (m >= cap) bin = 7;
        else if (m < k) bin = 0;
        else { std::size_t lim = 2 * k; bin = 1; while (m >= lim && bin < 6) { lim *= 2; ++bin; } }
        hist[bin]++;
      }
    }
    std::printf(",\"knn_min_rank_hist\":[");
    for (int i = 0; i < 8; ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)hist[i]);
    std::printf("],\"knn_cap\":%zu,\"lemma_violations\":%llu", cap, (unsigned long long)below_depth);
  }
  std::printf("}\n");
  return 0;
}
