// Audit-only shadow: H_a on real residual WSPD rectangles, never affects expansion.
#include "gen/lanes/q34_witness_search.hpp"
#include "gen/pipeline/prepared_cloud.hpp"
#include "gen/spindle/predicates.hpp"
#include "gen/wspd/front.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mhgp9::gen;
using Clock = std::chrono::steady_clock;

struct Phase {
  Clock::time_point wall = Clock::now();
  std::clock_t cpu = std::clock();
};
struct Duration { double wall_s{}, cpu_s{}; };
Duration elapsed(Phase start) {
  return {std::chrono::duration<double>(Clock::now() - start.wall).count(),
          double(std::clock() - start.cpu) / CLOCKS_PER_SEC};
}

struct Rect { std::size_t a_node{}, b_node{}; std::uint8_t mask{}; };
struct Sample { std::size_t a_node{}, b_node{}, a_rank{}; std::uint8_t mask{}, proven{}; };
struct ReplayState {
  std::size_t owner = std::numeric_limits<std::size_t>::max();
  std::vector<Q34WitnessNode> cache, trace;
  Q34WitnessSearchWork search{};
  Q34WitnessBoundsWork bounds{};
  Q34WitnessCacheWork cached{};
  std::uint64_t pairs{}, searches{}, cache_full{}, q3_open{}, q4_open{}, covers{};
};
static std::uint8_t replay_pair(const Q2CensusIndex& index,
    std::size_t a_id, std::size_t b_id, std::uint8_t mask, unsigned k,
    ReplayState& state) {
  const auto points = index.cloud().points();
  const auto cached = state.owner == a_id
      ? q34_cached_witness_rejections(index, points[a_id], points[b_id],
          static_cast<std::uint8_t>(k), mask, state.cache, state.cached)
      : std::uint8_t{0};
  const auto open = static_cast<std::uint8_t>(mask & ~cached);
  std::uint8_t result = 0;
  if (open) {
    result = filter_q34_witnesses(index, points[a_id], points[b_id],
        static_cast<std::uint8_t>(k), open, state.search, state.bounds, state.trace);
    state.owner = a_id;
    state.cache.swap(state.trace);
    ++state.searches;
  } else ++state.cache_full;
  ++state.pairs;
  if (result & 2U) ++state.q3_open;
  if (result & 4U) ++state.q4_open;
  if (result) ++state.covers;
  return result;
}
struct Bucket {
  std::uint64_t rectangles{}, rows{}, pair_mass{}, q3_mass{}, q4_mass{};
  std::uint64_t palette_q3_mass{}, palette_q4_mass{}, palette_full_mass{};
  std::uint64_t palette_proposals{}, palette_skipped_b{}, palette_calls{}, palette_corners{};
  std::uint64_t palette_q3_credits{}, palette_q4_credits{}, palette_failed_rows{};
  std::uint64_t dfs_sample_rows{}, dfs_sample_mass{}, dfs_full_mass{}, dfs_q3_mass{}, dfs_q4_mass{};
  std::uint64_t dfs_visits{}, dfs_h_tests{}, dfs_xi_tests{};
  std::uint64_t pair_sample_pairs{}, pair_calls{}, pair_visits{}, pair_cached_tests{};
  std::uint64_t pair_q3_mass{}, pair_q4_mass{}, pair_full_mass{};
  std::uint64_t palette_sample_q3_mass{}, palette_sample_q4_mass{}, palette_sample_full_mass{};
  std::uint64_t palette_sample_full_cache_hits{}, palette_sample_full_pair_calls{};
  std::uint64_t palette_sample_full_cache_tests{}, palette_sample_full_pair_visits{};
  std::uint64_t dfs_missed_palette_q3_mass{}, dfs_missed_palette_q4_mass{};
};

static std::uint64_t hash64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}
static unsigned bucket_of(std::size_t b) {
  return b < 2 ? 0 : b < 4 ? 1 : b < 8 ? 2 : b < 16 ? 3 :
         b < 32 ? 4 : b < 64 ? 5 : b < 128 ? 6 : b < 256 ? 7 : 8;
}
static std::uint64_t dist2(Point3 a, Point3 b) {
  std::uint64_t sum = 0;
  for (unsigned axis = 0; axis < 3; ++axis) {
    const auto d = std::int64_t(a[axis]) - std::int64_t(b[axis]);
    sum += std::uint64_t(d * d);
  }
  return sum;
}
// Audit-only fused form of the same strict u18 corner predicate. Returning
// an open-bit mask lets one H/Xi computation serve q3 and q4 at each corner.
static std::uint8_t fused_witness(Point3 a, Box3 b, Point3 z,
                                  std::uint8_t mask, std::uint64_t& corners) {
  for (unsigned corner = 0; corner < 8 && mask; ++corner) {
    ++corners;
    const auto p = box_corner(b, corner);
    std::array<i64, 3> u{}, w{};
    i64 h = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
      u[axis] = i64(z[axis]) - a[axis];
      w[axis] = i64(p[axis]) - z[axis];
      h += u[axis] * w[axis];
    }
    if (h <= 0) return 0;
    i128 xi = 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
      const auto j = (axis + 1) % 3, k = (axis + 2) % 3;
      const i64 cross = u[j] * w[k] - u[k] * w[j];
      xi += i128(cross) * cross;
    }
    const auto h2 = i128(h) * h;
    if (i128(3) * h2 <= xi) mask &= std::uint8_t(~2U);
    if (i128(2) * h2 <= xi) mask &= std::uint8_t(~4U);
  }
  return mask;
}

int main(int argc, char** argv) {
  if (argc != 5 && argc != 6) {
    std::cerr << "usage: shadow full.u32le K s row_sample_pow2 [replay]\n";
    return 2;
  }
  const bool replay = argc == 6 && std::string(argv[5]) == "replay";
  if (argc == 6 && !replay) throw std::invalid_argument("unknown replay option");
  const unsigned k = std::stoul(argv[2]), s = std::stoul(argv[3]);
  const std::uint64_t sample_mod = std::stoull(argv[4]);
  if ((k != 5 && k != 10) || (s != 8 && s != 10 && s != 12) ||
      !sample_mod || (sample_mod & (sample_mod - 1)) != 0)
    throw std::invalid_argument("unsupported K/s/sample modulus");
  std::ifstream input(argv[1], std::ios::binary);
  if (!input) throw std::runtime_error("cannot open input");
  const std::vector<unsigned char> bytes(
      (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  if (bytes.empty() || bytes.size() % 12) throw std::runtime_error("bad u32le XYZ length");
  std::vector<Point3> points(bytes.size() / 12);
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::uint32_t xyz[3]{};
    for (unsigned axis = 0; axis < 3; ++axis)
      for (unsigned byte = 0; byte < 4; ++byte)
        xyz[axis] |= std::uint32_t(bytes[12 * i + 4 * axis + byte]) << (8 * byte);
    points[i] = {static_cast<Coordinate>(xyz[0]), static_cast<Coordinate>(xyz[1]),
                 static_cast<Coordinate>(xyz[2])};
  }
  const Phase prep_start;
  const auto index = make_q2_cloud_index(prepare_cloud(points));
  const Duration index_time = elapsed(prep_start);
  const auto nodes = index->spatial_nodes();
  const auto order = index->spatial_order();
  std::vector<std::size_t> rank_of(points.size());
  for (std::size_t r = 0; r < order.size(); ++r) rank_of[order[r]] = r;

  // Propose the 2K nearest of the 8K adjacent spatial ranks. Approximate,
  // deterministic and O(n K log K); only universal_witness may certify.
  const Phase palette_start;
  std::vector<std::vector<std::size_t>> palette(points.size());
  std::uint64_t candidate_distances = 0;
  for (std::size_t r = 0; r < order.size(); ++r) {
    const auto first = r > 4 * k ? r - 4 * k : 0;
    const auto last = std::min(order.size(), r + 4 * k + 1);
    std::vector<std::pair<std::uint64_t, std::size_t>> near;
    near.reserve(last - first - 1);
    for (auto q = first; q < last; ++q) {
      if (q == r) continue;
      near.push_back({dist2(points[order[r]], points[order[q]]), q});
      ++candidate_distances;
    }
    const auto take = std::min<std::size_t>(2 * k, near.size());
    std::partial_sort(near.begin(), near.begin() + take, near.end());
    auto& out = palette[r];
    out.reserve(take);
    for (std::size_t i = 0; i < take; ++i) out.push_back(near[i].second);
  }
  const Duration palette_time = elapsed(palette_start);

  std::vector<Rect> rectangles;
  const Phase front_start;
  const auto front = run_wspd_front(*index, k, s, WspdFrontMode::MidpointSamples,
      [&](const WspdRectangle& r) { rectangles.push_back({r.a_node, r.b_node, r.lane_mask}); }, 6);
  const Duration front_time = elapsed(front_start);

  const Phase rect_start;
  Q34WitnessSearchWork rect_work{};
  Q34WitnessBoundsWork rect_bounds{};
  std::uint64_t front_mass = 0, residual_mass = 0, rect_rejected_mass = 0;
  std::size_t out_rect = 0;
  for (auto r : rectangles) {
    const auto ar = nodes[r.a_node].range, br = nodes[r.b_node].range;
    const auto mass = std::uint64_t(ar.size()) * br.size();
    front_mass += mass;
    r.mask = filter_q34_witnesses(*index, nodes[r.a_node].box, nodes[r.b_node].box,
        static_cast<std::uint8_t>(k), r.mask, rect_work, Q34WitnessBoundsMode::Affine, rect_bounds);
    if (!r.mask) { rect_rejected_mass += mass; continue; }
    residual_mass += mass;
    rectangles[out_rect++] = r;
  }
  rectangles.resize(out_rect);
  const Duration rect_time = elapsed(rect_start);

  std::array<Bucket, 9> buckets{};
  std::vector<Sample> samples;
  std::vector<std::uint8_t> row_masks;
  std::uint64_t fused_corners = 0, fused_calls = 0;
  PredicateWork predicate{};
  const Phase row_start;
  for (const auto& r : rectangles) {
    const auto ar = nodes[r.a_node].range, br = nodes[r.b_node].range;
    const auto B = nodes[r.b_node].box;
    auto& bucket = buckets[bucket_of(br.size())];
    ++bucket.rectangles;
    bucket.rows += ar.size();
    bucket.pair_mass += std::uint64_t(ar.size()) * br.size();
    if (r.mask & 2U) bucket.q3_mass += std::uint64_t(ar.size()) * br.size();
    if (r.mask & 4U) bucket.q4_mass += std::uint64_t(ar.size()) * br.size();
    for (auto ai = ar.first; ai < ar.last; ++ai) {
      const auto a = points[order[ai]];
      unsigned q3 = 0, q4 = 0;
      std::uint8_t proven = 0;
      if (br.size() >= 8) {
        for (auto zr : palette[ai]) {
          ++bucket.palette_proposals;
          if (zr >= br.first && zr < br.last) { ++bucket.palette_skipped_b; continue; }
          const auto z = points[order[zr]];
          bool q4_ok = false;
          if ((r.mask & 4U) && !(proven & 4U)) {
            q4_ok = universal_witness(Lane::Q4, a, B, z, predicate);
            if (q4_ok && ++q4 >= k - 2) proven |= 4U;
          }
          if ((r.mask & 2U) && !(proven & 2U)) {
            // Q4 strict implies Q3 strict for the same exact 8-corner test.
            if (q4_ok || universal_witness(Lane::Q3, a, B, z, predicate))
              if (++q3 >= k - 1) proven |= 2U;
          }
          if (proven == r.mask) break;
        }
        bucket.palette_q3_credits += q3;
        bucket.palette_q4_credits += q4;
        if (!proven) ++bucket.palette_failed_rows;
        if (proven & 2U) bucket.palette_q3_mass += br.size();
        if (proven & 4U) bucket.palette_q4_mass += br.size();
        if (proven == r.mask) bucket.palette_full_mass += br.size();
      }
      row_masks.push_back(proven);
      const auto sampled = (hash64((std::uint64_t(r.a_node) << 33) ^
                         (std::uint64_t(r.b_node) << 7) ^ ai) & (sample_mod - 1)) == 0;
      if (!sampled || br.size() < 8) continue;
      samples.push_back({r.a_node, r.b_node, ai, r.mask, proven});
      ++bucket.dfs_sample_rows;
      bucket.dfs_sample_mass += br.size();
      if (proven & 2U) bucket.palette_sample_q3_mass += br.size();
      if (proven & 4U) bucket.palette_sample_q4_mass += br.size();
      if (proven == r.mask) bucket.palette_sample_full_mass += br.size();
    }
  }
  const Duration rows_time = elapsed(row_start);
  const Phase fused_start;
  std::size_t row_cursor = 0;
  for (const auto& r : rectangles) {
    const auto ar = nodes[r.a_node].range, br = nodes[r.b_node].range;
    const auto B = nodes[r.b_node].box;
    for (auto ai = ar.first; ai < ar.last; ++ai) {
      std::uint8_t proven = 0;
      unsigned q3 = 0, q4 = 0;
      if (br.size() >= 8) {
        const auto a = points[order[ai]];
        for (auto zr : palette[ai]) {
          if (zr >= br.first && zr < br.last) continue;
          const auto z = points[order[zr]];
          ++fused_calls;
          const auto result = fused_witness(a, B, z,
              static_cast<std::uint8_t>(r.mask & ~proven), fused_corners);
          if ((result & 2U) && ++q3 >= k - 1) proven |= 2U;
          if ((result & 4U) && ++q4 >= k - 2) proven |= 4U;
          if (proven == r.mask) break;
        }
      }
      if (row_cursor >= row_masks.size() || proven != row_masks[row_cursor++])
        throw std::logic_error("fused q3/q4 corner predicate disagrees with public predicates");
    }
  }
  if (row_cursor != row_masks.size()) throw std::logic_error("fused row count mismatch");
  const Duration fused_time = elapsed(fused_start);
  // Keep the sample paths separate in time from the full H_a pass.
  const Phase comparison_start;
  for (const auto& sample : samples) {
    const auto br = nodes[sample.b_node].range;
    const auto B = nodes[sample.b_node].box;
    auto& bucket = buckets[bucket_of(br.size())];
      const auto a = points[order[sample.a_rank]];
      Q34WitnessSearchWork dfs{};
      Q34WitnessBoundsWork dfs_bounds{};
      const auto dfs_mask = filter_q34_witnesses(*index, singleton_box(a), B,
          static_cast<std::uint8_t>(k), sample.mask, dfs, Q34WitnessBoundsMode::Affine, dfs_bounds);
      // The row DFS separates Hmin and Xi_max; its general-box leaf decision
      // can stay undecided after exact corner tests certify a point witness.
      if ((dfs_mask & sample.proven & 2U) != 0) bucket.dfs_missed_palette_q3_mass += br.size();
      if ((dfs_mask & sample.proven & 4U) != 0) bucket.dfs_missed_palette_q4_mass += br.size();
      bucket.dfs_visits += dfs.node_visits;
      bucket.dfs_h_tests += dfs.h_bound_tests;
      bucket.dfs_xi_tests += dfs.xi_bound_tests;
      if ((sample.mask & 2U) && !(dfs_mask & 2U)) bucket.dfs_q3_mass += br.size();
      if ((sample.mask & 4U) && !(dfs_mask & 4U)) bucket.dfs_q4_mass += br.size();
      if (!dfs_mask) bucket.dfs_full_mass += br.size();

      std::vector<Q34WitnessNode> cache, trace;
      for (auto bi = br.first; bi < br.last; ++bi) {
        const auto b = points[order[bi]];
        Q34WitnessCacheWork cw{};
        const std::uint8_t cached = cache.empty() ? 0 :
            q34_cached_witness_rejections(*index, a, b,
                static_cast<std::uint8_t>(k), sample.mask, cache, cw);
        const auto open = static_cast<std::uint8_t>(sample.mask & ~cached);
        bucket.pair_cached_tests += cw.node_tests;
        const bool palette_full = sample.proven == sample.mask;
        if (palette_full) {
          bucket.palette_sample_full_cache_tests += cw.node_tests;
          if (open) ++bucket.palette_sample_full_pair_calls;
          else ++bucket.palette_sample_full_cache_hits;
        }
        std::uint8_t filtered = 0;
        if (open) {
          Q34WitnessSearchWork pw{};
          Q34WitnessBoundsWork pb{};
          filtered = filter_q34_witnesses(*index, a, b, static_cast<std::uint8_t>(k),
              open, pw, pb, trace);
          ++bucket.pair_calls;
          bucket.pair_visits += pw.node_visits;
          if (palette_full) bucket.palette_sample_full_pair_visits += pw.node_visits;
          cache.swap(trace);
        }
        if (filtered & sample.proven)
          throw std::logic_error("palette certificate not reproduced by exact pair filter");
        ++bucket.pair_sample_pairs;
        if ((sample.mask & 2U) && !(filtered & 2U)) ++bucket.pair_q3_mass;
        if ((sample.mask & 4U) && !(filtered & 4U)) ++bucket.pair_q4_mass;
        if (!filtered) ++bucket.pair_full_mass;
      }
  }
  const Duration comparison_time = elapsed(comparison_start);

  ReplayState baseline_replay{}, palette_replay{};
  Duration baseline_replay_time{}, palette_replay_time{};
  std::uint64_t skipped_pairs = 0, skipped_rows = 0;
  if (replay) {
    std::vector<std::uint8_t> baseline_masks;
    baseline_masks.reserve(residual_mass);
    std::vector<std::uint8_t> baseline_row_or;
    baseline_row_or.reserve(row_masks.size());
    const Phase baseline_start;
    for (const auto& r : rectangles) {
      const auto ar = nodes[r.a_node].range, br = nodes[r.b_node].range;
      for (auto ai = ar.first; ai < ar.last; ++ai) {
        std::uint8_t row_or = 0;
        for (auto bi = br.first; bi < br.last; ++bi) {
          const auto result = replay_pair(*index, order[ai], order[bi], r.mask, k, baseline_replay);
          baseline_masks.push_back(result);
          row_or |= result;
        }
        baseline_row_or.push_back(row_or);
      }
    }
    baseline_replay_time = elapsed(baseline_start);
    if (baseline_masks.size() != residual_mass || baseline_row_or.size() != row_masks.size())
      throw std::logic_error("baseline pair replay count mismatch");
    std::size_t row_cursor = 0, pair_cursor = 0;
    const Phase palette_replay_start;
    for (const auto& r : rectangles) {
      const auto ar = nodes[r.a_node].range, br = nodes[r.b_node].range;
      for (auto ai = ar.first; ai < ar.last; ++ai) {
        const auto proven = row_masks[row_cursor];
        const auto open = static_cast<std::uint8_t>(r.mask & ~proven);
        if (!open) {
          if (baseline_row_or[row_cursor] != 0)
            throw std::logic_error("palette skipped a pair surviving baseline exact filter");
          pair_cursor += br.size();
          skipped_pairs += br.size();
          ++skipped_rows;
        } else {
          for (auto bi = br.first; bi < br.last; ++bi) {
            const auto result = replay_pair(*index, order[ai], order[bi], open, k, palette_replay);
            if (result != baseline_masks[pair_cursor])
              throw std::logic_error("palette replay changed an exact pair lane mask");
            ++pair_cursor;
          }
        }
        ++row_cursor;
      }
    }
    palette_replay_time = elapsed(palette_replay_start);
    if (row_cursor != row_masks.size() || pair_cursor != baseline_masks.size())
      throw std::logic_error("palette pair replay count mismatch");
  }

  std::cout << "meta sites " << points.size() << " k " << k << " s " << s
      << " sample_mod " << sample_mod << " palette_size " << 2*k
      << " palette_window " << 8*k << " candidate_distances " << candidate_distances << '\n';
  auto print_time = [](const char* name, Duration d) {
    std::cout << "time " << name << " wall_s " << d.wall_s << " cpu_s " << d.cpu_s << '\n';
  };
  print_time("index", index_time); print_time("palette_prepare", palette_time);
  print_time("front", front_time); print_time("rectangle_filter", rect_time);
  print_time("palette_rows_public", rows_time); print_time("palette_rows_fused", fused_time);
  print_time("sample_dfs_cache_pairs", comparison_time);
  if (replay) {
    print_time("baseline_pair_cache_replay", baseline_replay_time);
    print_time("palette_pair_cache_replay", palette_replay_time);
    std::cout << "replay skipped_rows " << skipped_rows << " skipped_pairs " << skipped_pairs
        << " baseline_pairs " << baseline_replay.pairs << " baseline_searches " << baseline_replay.searches
        << " baseline_cache_full " << baseline_replay.cache_full
        << " baseline_cache_tests " << baseline_replay.cached.node_tests
        << " baseline_pair_visits " << baseline_replay.search.node_visits
        << " baseline_covers " << baseline_replay.covers
        << " palette_pairs " << palette_replay.pairs << " palette_searches " << palette_replay.searches
        << " palette_cache_full " << palette_replay.cache_full
        << " palette_cache_tests " << palette_replay.cached.node_tests
        << " palette_pair_visits " << palette_replay.search.node_visits
        << " palette_covers " << palette_replay.covers << '\n';
  }
  std::cout << "front rectangles " << front.work.emitted_rectangles << " mass " << front_mass
      << " rejected_mass " << rect_rejected_mass << " residual_rectangles " << rectangles.size()
      << " residual_mass " << residual_mass << " rectangle_visits " << rect_work.node_visits << '\n';
  std::cout << "palette_predicate universal_queries " << predicate.universal_queries
      << " corner_tests " << predicate.corner_tests << " point_tests " << predicate.point_tests
      << " fused_calls " << fused_calls << " fused_corners " << fused_corners << '\n';
  for (unsigned i=0; i<buckets.size(); ++i) {
    auto& b=buckets[i];
    std::cout << "bucket " << i << " rectangles " << b.rectangles << " rows " << b.rows
        << " mass " << b.pair_mass << " q3_mass " << b.q3_mass << " q4_mass " << b.q4_mass
        << " palette_q3_mass " << b.palette_q3_mass << " palette_q4_mass " << b.palette_q4_mass
        << " palette_full_mass " << b.palette_full_mass
        << " proposals " << b.palette_proposals << " skipped_b " << b.palette_skipped_b
        << " q3_credits " << b.palette_q3_credits << " q4_credits " << b.palette_q4_credits
        << " failed_rows " << b.palette_failed_rows
        << " sample_rows " << b.dfs_sample_rows << " sample_mass " << b.dfs_sample_mass
        << " sample_palette_q3 " << b.palette_sample_q3_mass
        << " sample_palette_q4 " << b.palette_sample_q4_mass
        << " sample_palette_full " << b.palette_sample_full_mass
        << " sample_dfs_q3 " << b.dfs_q3_mass << " sample_dfs_q4 " << b.dfs_q4_mass
        << " sample_dfs_full " << b.dfs_full_mass << " sample_dfs_visits " << b.dfs_visits
        << " sample_pair_count " << b.pair_sample_pairs << " sample_pair_calls " << b.pair_calls
        << " sample_pair_visits " << b.pair_visits << " sample_cache_node_tests " << b.pair_cached_tests
        << " sample_pair_q3 " << b.pair_q3_mass << " sample_pair_q4 " << b.pair_q4_mass
        << " sample_pair_full " << b.pair_full_mass
        << " palette_full_cache_hits " << b.palette_sample_full_cache_hits
        << " palette_full_pair_calls " << b.palette_sample_full_pair_calls
        << " palette_full_cache_tests " << b.palette_sample_full_cache_tests
        << " palette_full_pair_visits " << b.palette_sample_full_pair_visits
        << " dfs_missed_palette_q3 " << b.dfs_missed_palette_q3_mass
        << " dfs_missed_palette_q4 " << b.dfs_missed_palette_q4_mass << '\n';
  }
}
