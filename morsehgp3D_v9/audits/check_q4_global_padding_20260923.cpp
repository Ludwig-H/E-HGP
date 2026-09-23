// Audit-only stress probe: a fixed positive q4 ball whose four q3 faces
// fail K=3, embedded in a much larger, geometrically remote u18 cloud.
// Success proves membership of ONE target key in the raw q3/q4 stream,
// not completeness of the stream or of the FULL tower.
// Usage: check_q4_global_padding N permutation s pattern
//   N = total sites (at least 12, at most 32000 in this probe)
//   permutation = 0 (fixture first) or 1 (remote first, support IDs reversed)
//   s = 8, 10 or 12; pattern = 0 (remote line) or 1 (interleaved 64^3 cube).

#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/wspd_q34.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {
using mhgp9::gen::Point3;
using mhgp9::gen::i128;

struct Found final {};

unsigned parse(const char* s) {
  if (!s || !*s) throw std::invalid_argument("empty integer");
  char* end = nullptr;
  const unsigned long value = std::strtoul(s, &end, 10);
  if (!end || *end || value > 32000UL) throw std::invalid_argument("integer outside probe domain");
  return static_cast<unsigned>(value);
}

bool same(Point3 a, Point3 b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

constexpr std::array<Point3, 4> support{{
    {30, 30, 30}, {30, 10, 10}, {10, 30, 10}, {10, 10, 30}}};

std::vector<Point3> make_points(unsigned n, unsigned permutation, unsigned pattern) {
  std::vector<Point3> fixture;
  fixture.reserve(12);
  for (const auto p : support) fixture.push_back(p);
  for (const auto p : support) {
    for (const int radius : {11, 12}) {
      fixture.push_back({20 - radius * ((p.x - 20) / 10),
                         20 - radius * ((p.y - 20) / 10),
                         20 - radius * ((p.z - 20) / 10)});
    }
  }
  std::vector<Point3> remote;
  remote.reserve(n - 12);
  if (pattern == 0) {
    for (unsigned i = 0; i < n - 12; ++i)
      remote.push_back({static_cast<std::int32_t>(1000 + i), 1000, 1000});
  } else {
    // An odd multiplier permutes 2^18 integer cells. Sites are spatially
    // interleaved with the fixture but strictly outside its target ball.
    for (unsigned i = 0; remote.size() < n - 12 && i < (1U << 18); ++i) {
      const unsigned word = (i * 131071U + 12345U) & ((1U << 18) - 1);
      const Point3 p{static_cast<std::int32_t>(word & 63U),
                     static_cast<std::int32_t>((word >> 6) & 63U),
                     static_cast<std::int32_t>((word >> 12) & 63U)};
      const auto dx = p.x - 20, dy = p.y - 20, dz = p.z - 20;
      if (dx * dx + dy * dy + dz * dz <= 300) continue;
      if (std::any_of(fixture.begin(), fixture.end(), [&](Point3 q) { return same(p, q); })) continue;
      remote.push_back(p);
    }
    if (remote.size() != n - 12) throw std::runtime_error("dense padding exhausted");
  }
  std::vector<Point3> points;
  points.reserve(n);
  if (permutation == 0) {
    points.insert(points.end(), fixture.begin(), fixture.end());
    points.insert(points.end(), remote.begin(), remote.end());
  } else {
    points.insert(points.end(), remote.begin(), remote.end());
    points.insert(points.end(), fixture.begin() + 4, fixture.end());
    points.insert(points.end(), support.rbegin(), support.rend());
  }
  return points;
}

void check_target(const std::vector<Point3>& points,
                  const mhgp9::gen::Q34SeedCandidate& c) {
  if (c.depth != 0 || c.shell_first.size() + c.shell_second.size() != 4)
    throw std::runtime_error("target depth or complete shell differs");
  std::array<bool, 4> support_seen{};
  std::array<bool, 4> shell_seen{};
  const auto accept = [&](std::size_t id, std::array<bool, 4>& seen) {
    if (id >= points.size()) throw std::runtime_error("target ID outside cloud");
    for (std::size_t j = 0; j < support.size(); ++j) {
      if (!same(points[id], support[j])) continue;
      if (seen[j]) throw std::runtime_error("target duplicate support or shell site");
      seen[j] = true;
      return;
    }
    throw std::runtime_error("target support or shell contains a remote site");
  };
  for (std::size_t j = 0; j < 4; ++j) accept(c.support_ids[j], support_seen);
  for (const auto id : c.shell_first) accept(id, shell_seen);
  for (const auto id : c.shell_second) accept(id, shell_seen);
  if (!std::all_of(support_seen.begin(), support_seen.end(), [](bool v) { return v; }) ||
      !std::all_of(shell_seen.begin(), shell_seen.end(), [](bool v) { return v; }))
    throw std::runtime_error("target support or shell is incomplete");
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 5) throw std::invalid_argument("usage: check_q4_global_padding N permutation s pattern");
    const unsigned n = parse(argv[1]), permutation = parse(argv[2]), s = parse(argv[3]), pattern = parse(argv[4]);
    if (n < 12 || permutation > 1 || pattern > 1 || (s != 8 && s != 10 && s != 12))
      throw std::invalid_argument("probe domain: 12<=N<=32000, permutation/pattern 0/1, s 8/10/12");
    const auto points = make_points(n, permutation, pattern);
    const auto index = mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    mhgp9::gen::WspdQ34Options opt;
    opt.front_mode = mhgp9::gen::WspdFrontMode::MidpointSamples;
    opt.requested_lane_mask = 6;
    opt.q4_backend = mhgp9::gen::WspdQ4Backend::Local28;
    opt.local.saturate_deep = true;
    opt.local.retain_q3_fragments = true;
    opt.witness_mode = mhgp9::gen::WspdQ34WitnessMode::RectanglePair;
    opt.q3_census_mode = mhgp9::gen::WspdQ3CensusMode::GlobalBoxes;
    opt.witness_bounds_mode = mhgp9::gen::Q34WitnessBoundsMode::Affine;
    opt.q4_seed_cells = {mhgp9::gen::Q4SeedCellMode::LiveOnly, 64};
    opt.q3_atlas_consultation = true;
    opt.q3_leaf_census = true;
    opt.dead_lanes = true;
    opt.dead_core = true;
    opt.pair_witness_cache = true;
    const std::array<i128, 5> target{1, -40, -40, -40, 900};
    std::uint64_t callbacks = 0;
    try {
      static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index, 3, s, opt,
          [&](const mhgp9::gen::Q34SeedCandidate& c) {
            ++callbacks;
            if (c.arity == 4 && c.ball.coefficients() == target) {
              check_target(points, c);
              throw Found{};
            }
          }));
    } catch (const Found&) {
      std::printf("{\"status\":\"PASS\",\"sites\":%u,\"permutation\":%u,\"s\":%u,\"pattern\":%u,\"callbacks_before_target\":%llu}\n",
                  n, permutation, s, pattern, static_cast<unsigned long long>(callbacks));
      return 0;
    }
    throw std::runtime_error("target q4 BallKey absent from raw generator stream");
  } catch (const std::exception& e) {
    std::fprintf(stderr, "FAIL: %s\n", e.what());
    return 1;
  }
}
