// MorseHGP3D v9 — sonde S1 du filtre temoin exact q3/q4 sur GPU
// (23 septembre 2026).
//
//   mhgp9_gpu_filter_probe <fichier .u32le|.u16le> K workers [--s=8]
//                          [--repeats=3] [--cpu-only]
//
// Meme population que la chaine : front WSPD q3/q4 (MidpointSamples, voies
// 6, s), rectangles filtres (bornes Affine), rectangles survivants developpes
// en paires a x b (ordre du moteur), paires filtrees sans cache. References
// CPU a `workers` fils : rectangles, paires sans cache, paires avec le cache
// de ligne du moteur (memes masques exiges). Le GPU (src/gpu/filter_runner)
// refait le passage ; chaque masque GPU doit egaler le masque CPU et les
// totaux de noeuds visites doivent etre egaux (meme DFS, meme ordre).
//
// Sortie : un objet JSON (schema mhgp9_gpu_filter_probe_v1) sur stdout.
// Code 0 conforme, 1 masque different, 2 argument ou entree, 3 GPU
// indisponible, erreur CUDA ou borne de pile violee.
#include <sys/resource.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "../src/gen/lanes/q34_witness_search.hpp"
#include "../src/gen/pipeline/q2_census.hpp"
#include "../src/gen/wspd/front.hpp"
#include "../src/gpu/filter_runner.hpp"
#include "../src/gpu/flat_index.hpp"

namespace {

using mhgp9::gen::Point3;
using Clock = std::chrono::steady_clock;

double ms_since(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}

void fnv_word(std::uint64_t& h, std::uint64_t w) {
  for (int i = 0; i < 8; ++i) {
    h ^= (w >> (8 * i)) & 0xffu;
    h *= 1099511628211ull;
  }
}

struct Input {
  std::vector<Point3> points;
  std::uint64_t hash = 14695981039346656037ull;  // FNV-1a 64 de n puis x, y, z (u64 LE), comme tower_probe
};

Input read_points(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::size_t width = 0;
  if (path.ends_with(".u32le")) width = 4;
  else if (path.ends_with(".u16le")) width = 2;
  else throw std::invalid_argument("input must be .u32le or .u16le");
  if (bytes.size() % (3 * width) != 0) throw std::invalid_argument("input length is not a multiple of a site record");
  const std::size_t n = bytes.size() / (3 * width);
  Input out;
  out.points.resize(n);
  fnv_word(out.hash, n);
  for (std::size_t i = 0; i < n; ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (std::size_t b = 0; b < width; ++b) v |= static_cast<std::uint32_t>(bytes[(3 * i + a) * width + b]) << (8 * b);
      if (v > 262143u) throw std::invalid_argument("coordinate outside [0, 2^18)");
      c[a] = v;
      fnv_word(out.hash, v);
    }
    out.points[i] = Point3{static_cast<mhgp9::gen::Coordinate>(c[0]), static_cast<mhgp9::gen::Coordinate>(c[1]),
                           static_cast<mhgp9::gen::Coordinate>(c[2])};
  }
  return out;
}

bool parse_u(std::string_view s, unsigned long long& v) {
  if (s.empty() || s.size() > 18) return false;
  v = 0;
  for (const char ch : s) {
    if (ch < '0' || ch > '9') return false;
    v = v * 10 + static_cast<unsigned>(ch - '0');
  }
  return true;
}

// Dynamic blocks of `grain` items over `workers` threads; per-thread
// exceptions are rethrown after the join.
template <class Body>
void parallel_blocks(std::size_t count, std::size_t workers, std::size_t grain, Body&& body) {
  std::atomic<std::size_t> next{0};
  std::vector<std::exception_ptr> failures(workers);
  const auto run = [&](std::size_t w) {
    try {
      for (;;) {
        const std::size_t begin = next.fetch_add(grain);
        if (begin >= count) break;
        body(begin, std::min(count, begin + grain), w);
      }
    } catch (...) {
      failures[w] = std::current_exception();
    }
  };
  std::vector<std::thread> pool;
  for (std::size_t w = 1; w < workers; ++w) pool.emplace_back(run, w);
  run(0);
  for (auto& t : pool) t.join();
  for (const auto& f : failures)
    if (f) std::rethrow_exception(f);
}

long peak_rss_kb() {
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0) return -1;
  return usage.ru_maxrss;
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp9;
  if (argc < 4) {
    std::fprintf(stderr, "usage: mhgp9_gpu_filter_probe <input> K workers [--s=8] [--repeats=3] [--cpu-only]\n");
    return 2;
  }
  unsigned long long kmax = 0, workers = 0, s = 8, repeats = 3;
  bool cpu_only = false;
  if (!parse_u(argv[2], kmax) || kmax < 3 || kmax > 10 || !parse_u(argv[3], workers) || workers == 0 ||
      workers > 256)
    return 2;
  for (int i = 4; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg.starts_with("--s=")) {
      if (!parse_u(arg.substr(4), s) || s < 8 || s > 64) return 2;
    } else if (arg.starts_with("--repeats=")) {
      if (!parse_u(arg.substr(10), repeats) || repeats == 0 || repeats > 20) return 2;
    } else if (arg == "--cpu-only") cpu_only = true;
    else return 2;
  }
  Input input;
  try {
    input = read_points(argv[1]);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "input: %s\n", e.what());
    return 2;
  }
  const auto k = static_cast<unsigned>(kmax);
  const auto W = static_cast<std::size_t>(workers);

  auto t = Clock::now();
  const auto cloud = gen::prepare_cloud(input.points);
  const auto index = gen::make_q2_cloud_index(cloud);
  const auto nodes = index->spatial_nodes();
  const auto order = index->spatial_order();
  const auto points = index->cloud().points();
  const auto flat = gpu::flatten_nodes(*index);
  std::vector<std::int32_t> rank_points(3 * order.size());
  for (std::size_t r = 0; r < order.size(); ++r) {
    const auto& p = points[order[r]];
    rank_points[3 * r] = p.x;
    rank_points[3 * r + 1] = p.y;
    rank_points[3 * r + 2] = p.z;
  }
  const double index_ms = ms_since(t);

  // Front (sequential): the rectangle stream of the chain's q3/q4 lanes.
  t = Clock::now();
  std::vector<gpu::u32> rect_a, rect_b;
  std::vector<gpu::u8> rect_mask;
  const auto front = gen::run_wspd_front(*index, k, static_cast<unsigned>(s), gen::WspdFrontMode::MidpointSamples,
      [&](const gen::WspdRectangle& r) {
        rect_a.push_back(static_cast<gpu::u32>(r.a_node));
        rect_b.push_back(static_cast<gpu::u32>(r.b_node));
        rect_mask.push_back(r.lane_mask);
      }, 6);
  const double front_ms = ms_since(t);
  const std::size_t R = rect_a.size();

  // CPU reference 1: rectangles (product filter, Affine bounds).
  std::vector<gpu::u8> cpu_rect(R);
  std::vector<std::uint64_t> rect_visits(W);
  t = Clock::now();
  parallel_blocks(R, W, 1024, [&](std::size_t begin, std::size_t end, std::size_t w) {
    gen::Q34WitnessSearchWork work;
    gen::Q34WitnessBoundsWork bounds;
    for (std::size_t i = begin; i < end; ++i)
      cpu_rect[i] = gen::filter_q34_witnesses(*index, nodes[rect_a[i]].box, nodes[rect_b[i]].box,
          static_cast<std::uint8_t>(k), rect_mask[i], work, gen::Q34WitnessBoundsMode::Affine, bounds);
    rect_visits[w] += work.node_visits;
  });
  const double cpu_rect_ms = ms_since(t);

  // Pair offsets of the surviving rectangles, engine order.
  std::vector<std::uint64_t> offsets(R + 1, 0);
  for (std::size_t i = 0; i < R; ++i)
    offsets[i + 1] = offsets[i] + (cpu_rect[i] != 0 ? static_cast<std::uint64_t>(nodes[rect_a[i]].range.size()) *
                                                          nodes[rect_b[i]].range.size()
                                                    : 0);
  const std::uint64_t P = offsets[R];

  // CPU reference 2: every pair, full search (no cache).
  std::vector<gpu::u8> cpu_pair(P);
  std::vector<std::uint64_t> pair_visits(W), pair_queries(W);
  t = Clock::now();
  parallel_blocks(R, W, 64, [&](std::size_t begin, std::size_t end, std::size_t w) {
    gen::Q34WitnessSearchWork work;
    gen::Q34WitnessBoundsWork bounds;
    std::vector<gen::Q34WitnessNode> trace;
    for (std::size_t i = begin; i < end; ++i) {
      if (cpu_rect[i] == 0) continue;
      const auto a = nodes[rect_a[i]].range, b = nodes[rect_b[i]].range;
      std::uint64_t at = offsets[i];
      for (auto ai = a.first; ai < a.last; ++ai)
        for (auto bi = b.first; bi < b.last; ++bi)
          cpu_pair[at++] = gen::filter_q34_witnesses(*index, points[order[ai]], points[order[bi]],
                                                     static_cast<std::uint8_t>(k), cpu_rect[i], work, bounds, trace);
    }
    pair_visits[w] += work.node_visits;
    pair_queries[w] += work.queries;
  });
  const double cpu_pair_ms = ms_since(t);

  // CPU reference 3: the engine's row cache (same masks required).
  std::uint64_t cache_mismatches = 0, cache_searches = 0;
  std::vector<std::uint64_t> cache_mis(W), cache_search(W);
  t = Clock::now();
  parallel_blocks(R, W, 64, [&](std::size_t begin, std::size_t end, std::size_t w) {
    gen::Q34WitnessSearchWork work;
    gen::Q34WitnessBoundsWork bounds;
    gen::Q34WitnessCacheWork cache_work;
    std::vector<gen::Q34WitnessNode> trace, cache;
    std::size_t owner = static_cast<std::size_t>(-1);
    for (std::size_t i = begin; i < end; ++i) {
      if (cpu_rect[i] == 0) continue;
      const auto a = nodes[rect_a[i]].range, b = nodes[rect_b[i]].range;
      std::uint64_t at = offsets[i];
      for (auto ai = a.first; ai < a.last; ++ai)
        for (auto bi = b.first; bi < b.last; ++bi) {
          const auto ia = order[ai], ib = order[bi];
          const auto mask = cpu_rect[i];
          const std::uint8_t cached = owner == ia
              ? gen::q34_cached_witness_rejections(*index, points[ia], points[ib], static_cast<std::uint8_t>(k), mask,
                                                   cache, cache_work)
              : std::uint8_t{0};
          const auto open = static_cast<std::uint8_t>(mask & ~cached);
          std::uint8_t result = 0;
          if (open != 0) {
            result = gen::filter_q34_witnesses(*index, points[ia], points[ib], static_cast<std::uint8_t>(k), open,
                                               work, bounds, trace);
            owner = ia;
            cache.swap(trace);
          }
          if (result != cpu_pair[at++]) ++cache_mis[w];
        }
    }
    cache_search[w] += work.queries;
  });
  const double cpu_cache_ms = ms_since(t);
  for (std::size_t w = 0; w < W; ++w) {
    cache_mismatches += cache_mis[w];
    cache_searches += cache_search[w];
  }

  std::uint64_t cpu_rect_visits = 0, cpu_pair_visits = 0, rect_survivors = 0, pair_survivors = 0;
  for (std::size_t w = 0; w < W; ++w) {
    cpu_rect_visits += rect_visits[w];
    cpu_pair_visits += pair_visits[w];
  }
  std::uint64_t lanes_rejected[2] = {0, 0}, lanes_open[2] = {0, 0};
  for (std::size_t i = 0; i < R; ++i) rect_survivors += cpu_rect[i] != 0;
  for (std::size_t i = 0; i < R; ++i) {
    if (cpu_rect[i] == 0) continue;
    for (std::uint64_t p = offsets[i]; p < offsets[i + 1]; ++p) {
      pair_survivors += cpu_pair[p] != 0;
      for (unsigned lane = 0; lane < 2; ++lane) {
        const unsigned bit = 2U << lane;
        if ((cpu_rect[i] & bit) == 0) continue;
        ++((cpu_pair[p] & bit) != 0 ? lanes_open[lane] : lanes_rejected[lane]);
      }
    }
  }

  gpu::FilterOutput g;
  std::uint64_t rect_mismatches = 0, pair_mismatches = 0;
  bool visits_equal = false;  // same DFS on both sides: identical node visit totals
  if (!cpu_only) {
    gpu::FilterInput in;
    in.nodes = flat.data();
    in.node_count = flat.size();
    in.rank_points = rank_points.data();
    in.rank_count = order.size();
    in.rect_a = rect_a.data();
    in.rect_b = rect_b.data();
    in.rect_mask = rect_mask.data();
    in.rect_count = R;
    in.kmax = k;
    in.repeats = static_cast<unsigned>(repeats);
    g = gpu::run_filters(in);
    if (g.error.empty()) {
      visits_equal = g.rect_visits == cpu_rect_visits && g.pair_visits == cpu_pair_visits;
      for (std::size_t i = 0; i < R; ++i) rect_mismatches += g.rect_masks[i] != cpu_rect[i];
      if (g.pairs != P) pair_mismatches = P > g.pairs ? P - g.pairs : g.pairs - P;
      for (std::uint64_t p = 0; p < std::min<std::uint64_t>(P, g.pairs); ++p) pair_mismatches += g.pair_masks[p] != cpu_pair[p];
    }
  }

  std::printf("{\"schema\":\"mhgp9_gpu_filter_probe_v1\",\"input\":{\"sites\":%zu,\"hash\":\"%016llx\"},"
              "\"options\":{\"K\":%u,\"s\":%llu,\"workers\":%zu,\"repeats\":%llu,\"cpu_only\":%s},"
              "\"population\":{\"rectangles\":%zu,\"rectangle_survivors\":%llu,\"pairs\":%llu,"
              "\"pair_survivors\":%llu,\"q3_rejected\":%llu,\"q3_open\":%llu,\"q4_rejected\":%llu,\"q4_open\":%llu,"
              "\"front_product_visits\":%llu},"
              "\"cpu\":{\"index_ms\":%.3f,\"front_ms\":%.3f,\"rect_ms\":%.3f,\"pair_nocache_ms\":%.3f,"
              "\"pair_cache_ms\":%.3f,\"rect_visits\":%llu,\"pair_visits\":%llu,\"cache_searches\":%llu,"
              "\"cache_mismatches\":%llu},",
              input.points.size(), static_cast<unsigned long long>(input.hash), k, s, W, repeats,
              cpu_only ? "true" : "false", R, static_cast<unsigned long long>(rect_survivors),
              static_cast<unsigned long long>(P), static_cast<unsigned long long>(pair_survivors),
              static_cast<unsigned long long>(lanes_rejected[0]), static_cast<unsigned long long>(lanes_open[0]),
              static_cast<unsigned long long>(lanes_rejected[1]), static_cast<unsigned long long>(lanes_open[1]),
              static_cast<unsigned long long>(front.work.product_visits), index_ms, front_ms, cpu_rect_ms,
              cpu_pair_ms, cpu_cache_ms, static_cast<unsigned long long>(cpu_rect_visits),
              static_cast<unsigned long long>(cpu_pair_visits), static_cast<unsigned long long>(cache_searches),
              static_cast<unsigned long long>(cache_mismatches));
  std::printf("\"gpu\":{\"available\":%s,\"device\":\"%s\",\"error\":\"%s\",\"stack_failure\":%s,\"pairs\":%llu,"
              "\"rect_visits\":%llu,\"pair_visits\":%llu,\"upload_ms\":%.3f,\"rect_ms\":%.3f,\"scan_ms\":%.3f,"
              "\"pair_ms\":%.3f,\"download_ms\":%.3f,\"total_ms\":%.3f,\"first_total_ms\":%.3f,"
              "\"rect_mismatches\":%llu,\"pair_mismatches\":%llu,\"visits_equal\":%s},\"peak_rss_kb\":%ld}\n",
              g.available ? "true" : "false", g.device.c_str(), g.error.c_str(), g.stack_failure ? "true" : "false",
              static_cast<unsigned long long>(g.pairs), static_cast<unsigned long long>(g.rect_visits),
              static_cast<unsigned long long>(g.pair_visits), g.upload_ms, g.rect_ms, g.scan_ms, g.pair_ms,
              g.download_ms, g.total_ms, g.first_total_ms, static_cast<unsigned long long>(rect_mismatches),
              static_cast<unsigned long long>(pair_mismatches), visits_equal ? "true" : "false", peak_rss_kb());
  if (cache_mismatches != 0) return 1;
  if (cpu_only) return 0;
  if (!g.error.empty() || !g.available || g.stack_failure) return 3;
  if (rect_mismatches != 0 || pair_mismatches != 0 || !visits_equal) return 1;
  return 0;
}
