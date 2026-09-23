// Auditeur C / piste D3 : attribution du travail q3/q4 de la chaine v9 par echelle.
// Usage : d3_attrib <fichier.u32le> K workers
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/chain/tower_chain.hpp"
#include "d3probe.hpp"

using mhgp9::gen::Point3;

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
    pts[i] = Point3{(mhgp9::gen::Coordinate)c[0], (mhgp9::gen::Coordinate)c[1], (mhgp9::gen::Coordinate)c[2]};
  }
  return pts;
}

static void arr(const char* name, const std::atomic<std::uint64_t>* a, bool comma = true) {
  std::printf("\"%s\":[", name);
  for (int i = 0; i < d3probe::kBuckets; ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)a[i].load());
  std::printf("]%s", comma ? "," : "");
}

int main(int argc, char** argv) {
  if (argc < 4) { std::fprintf(stderr, "usage: d3_attrib file K workers\n"); return 2; }
  const auto pts = read_u32le(argv[1]);
  mhgp9::ChainOptions opt;
  opt.kmax = (unsigned)std::stoul(argv[2]);
  opt.workers = std::stoul(argv[3]);
  opt.run_tower = false;
  opt.keep_catalogue = true;
  const auto res = mhgp9::run_tower_chain(pts, opt);
  if (res.status != mhgp9::ChainStatus::kComplete) {
    std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", mhgp9::chain_status_name(res.status), res.reason.c_str());
    return 1;
  }
  // rayons des boules uniques du catalogue
  const double hs[] = {100, 200, 250, 400, 500, 800, 1000, 1600, 2000};
  std::vector<std::uint64_t> above(sizeof(hs) / sizeof(hs[0]), 0);
  std::vector<std::uint64_t> above_q4(above.size(), 0), above_q3(above.size(), 0);
  for (const auto& b : res.catalogue_balls) {
    const long double a = (long double)b.key.a;
    long double bb = 0;
    for (int i = 0; i < 3; ++i) bb += (long double)b.key.b[i] * (long double)b.key.b[i];
    const double r = (double)std::sqrt(std::max((bb - 4.0L * a * (long double)b.key.c) / (4.0L * a * a), 0.0L));
    for (std::size_t i = 0; i < above.size(); ++i) if (r > hs[i]) { ++above[i]; if (b.arity == 4) ++above_q4[i]; if (b.arity == 3) ++above_q3[i]; }
  }
  auto& G = d3probe::g;
  std::printf("{\"file\":\"%s\",\"n\":%zu,\"kmax\":%u,\"workers\":%zu,\"balls\":%zu,", argv[1], pts.size(), opt.kmax,
              opt.workers, res.catalogue_balls.size());
  std::printf("\"times_ms\":{\"q2\":%.1f,\"q34\":%.1f,\"merge\":%.1f,\"census\":%.1f,\"total\":%.1f},\"cpu_s\":%.2f,",
              res.times.q2_ms, res.times.q34_ms, res.times.merge_ms, res.times.census_ms, res.times.total_ms, res.times.cpu_s);
  std::printf("\"h_mm\":[100,200,250,400,500,800,1000,1600,2000],\"balls_r_above_h\":[");
  for (std::size_t i = 0; i < above.size(); ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)above[i]);
  std::printf("],\"q3_r_above_h\":[");
  for (std::size_t i = 0; i < above.size(); ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)above_q3[i]);
  std::printf("],\"q4_r_above_h\":[");
  for (std::size_t i = 0; i < above.size(); ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)above_q4[i]);
  std::printf("],\"bucket_upper_mm\":[50,100,200,400,800,1600,3200,6400,12800,-1],");
  arr("edges", G.edges); arr("witness_rejected", G.witness_rejected); arr("cache_rejected", G.cache_rejected);
  arr("core_builds", G.core_builds); arr("core_closed", G.core_closed); arr("cover_builds", G.cover_builds);
  arr("core_sites", G.core_sites); arr("cover_sites", G.cover_sites); arr("atlas_pt", G.atlas_pt);
  arr("q3_census_pt", G.q3_census_pt); arr("q3_leaf_pt", G.q3_leaf_pt); arr("q4_sweep_active", G.q4_sweep_active);
  arr("witness_node_visits", G.witness_node_visits); arr("dead_pt", G.dead_pt); arr("q3_seeds", G.q3_seeds);
  arr("q4_seeds", G.q4_seeds); arr("q3_emitted", G.q3_emitted); arr("q4_emitted", G.q4_emitted); arr("cpu_ns", G.cpu_ns);
  arr("rect_count", G.rect_count); arr("rect_mass", G.rect_mass); arr("rect_rejected_mass", G.rect_rejected_mass);
  arr("rect_cpu_ns", G.rect_cpu_ns); arr("rect_node_visits", G.rect_node_visits);
  std::printf("\"emit_hist\":[");
  for (int i = 0; i < d3probe::kBuckets; ++i) {
    std::printf("%s[", i ? "," : "");
    for (int j = 0; j < d3probe::kBuckets; ++j) std::printf("%s%llu", j ? "," : "", (unsigned long long)G.emit_hist[i][j].load());
    std::printf("]");
  }
  std::printf("],\"min_ab_over_r\":%.6f,", G.min_ratio_e6.load() / 1e6);
  std::printf("\"expanded_pairs_total\":%llu,\"core_sites_total\":%llu,\"cover_sites_total\":%llu,\"atlas_pt_total\":%llu}\n",
              (unsigned long long)res.q34_expanded_pairs, (unsigned long long)res.ledger.core_sites,
              (unsigned long long)res.ledger.cover_sites, (unsigned long long)res.ledger.atlas_point_tests);
  return 0;
}
