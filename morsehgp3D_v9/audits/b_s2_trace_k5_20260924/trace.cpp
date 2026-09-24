#include "lanes/edge_cover.hpp"
#include "lanes/q34_dead_lanes.hpp"
#include "pipeline/prepared_cloud.hpp"
#include "pipeline/wspd_q34.hpp"
#include "wspd/front.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

using namespace mhgp9::gen;

// An intentionally bounded, audit-only replay of the already published
// 08/000200 no-ground, quarter-density, x>=0/y>=0 physical sector.
constexpr std::size_t kSites = 1288;
constexpr unsigned kMax = 5, kSeparation = 8;
constexpr std::size_t kWorkers = 8, kJobsPerWorker = 64;
constexpr std::uint64_t kRectangles = 37459, kExpanded = 46218;
constexpr std::uint64_t kSurvivors = 27099, kCoreSites = 298205;
constexpr std::uint64_t kCoreClosed = 7020;

std::vector<unsigned char> read_bytes(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("cannot open " + path.string());
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::uint32_t read_u32(const unsigned char* p) {
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

struct Input {
  std::vector<Point3> points;
  std::vector<std::uint32_t> raw_ids;
};

Input read_input(const std::filesystem::path& points_path, const std::filesystem::path& raw_path) {
  const auto xyz = read_bytes(points_path), raw = read_bytes(raw_path);
  if (xyz.size() != 12 * kSites || raw.size() != 4 * kSites)
    throw std::runtime_error("fixture must have exactly 1,288 aligned sites and raw-return IDs");
  Input out;
  out.points.reserve(kSites);
  out.raw_ids.reserve(kSites);
  for (std::size_t i = 0; i < kSites; ++i) {
    const auto x = read_u32(xyz.data() + 12 * i);
    const auto y = read_u32(xyz.data() + 12 * i + 4);
    const auto z = read_u32(xyz.data() + 12 * i + 8);
    if (x >= (1U << 18) || y >= (1U << 18) || z >= (1U << 18))
      throw std::runtime_error("coordinate exceeds the u18 grid");
    out.points.push_back({Coordinate(x), Coordinate(y), Coordinate(z)});
    out.raw_ids.push_back(read_u32(raw.data() + 4 * i));
  }
  auto sorted = out.raw_ids;
  std::sort(sorted.begin(), sorted.end());
  if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
    throw std::runtime_error("duplicate raw-return ID");
  return out;
}

struct Row {
  std::uint64_t rectangle{};
  std::uint32_t local_a{}, local_b{}, raw_a{}, raw_b{}, core_sites{};
  std::uint8_t s2_mask{}, post_core_mask{}, s3_mask{};
};

std::uint64_t raw_edge_key(std::uint32_t a, std::uint32_t b) {
  if (a == b) throw std::runtime_error("self edge");
  if (a > b) std::swap(a, b);
  return (std::uint64_t(a) << 32) | b;
}

void run(const std::filesystem::path& points_path, const std::filesystem::path& raw_path,
         const std::filesystem::path& trace_path, const std::filesystem::path& segments_path) {
  if (trace_path == segments_path || trace_path == points_path || trace_path == raw_path ||
      segments_path == points_path || segments_path == raw_path ||
      std::filesystem::exists(trace_path) || std::filesystem::exists(segments_path))
    throw std::runtime_error("output path conflicts with an input or existing file");
  const auto input = read_input(points_path, raw_path);
  const auto index = make_q2_cloud_index(prepare_cloud(input.points));
  const auto nodes = index->spatial_nodes();
  const auto order = index->spatial_order();

  // Match the product's W8, fine-jobs, mass-first front. A sequential
  // run_job loop still concatenates terminal rectangles in job-index order.
  const auto plan = make_wspd_front_jobs(index, kMax, kSeparation,
      WspdFrontMode::MidpointSamples, kWorkers * kJobsPerWorker, 6, {}, true);
  std::vector<WspdRectangle> rectangles;
  for (std::size_t job = 0; job < plan->job_count(); ++job)
    static_cast<void>(plan->run_job(job, [&](const WspdRectangle& r) { rectangles.push_back(r); }));
  if (rectangles.size() != kRectangles) throw std::runtime_error("front rectangle ledger mismatch");

  const auto s2 = run_q34_filter_batch_cpu(*index, kMax, rectangles, kWorkers);
  if (s2.rectangle_masks.size() != rectangles.size() || s2.expanded_pairs != kExpanded ||
      s2.survivors.size() != kSurvivors)
    throw std::runtime_error("S2 rectangle, expanded-pair, or survivor ledger mismatch");

  // Preserve every rectangle boundary, including empty and trailing segments.
  std::vector<std::uint64_t> begin;
  begin.reserve(rectangles.size() + 1);
  std::vector<std::uint64_t> rectangle_of;
  rectangle_of.reserve(s2.survivors.size());
  std::size_t cursor = 0;
  for (std::size_t i = 0; i < rectangles.size(); ++i) {
    begin.push_back(cursor);
    const auto mask = s2.rectangle_masks[i];
    const auto& r = rectangles[i];
    if ((mask & ~r.lane_mask) != 0) throw std::runtime_error("S2 widened a rectangle mask");
    if (mask == 0) continue;
    const auto a = nodes[r.a_node].range, b = nodes[r.b_node].range;
    std::uint64_t previous = std::numeric_limits<std::uint64_t>::max();
    while (cursor < s2.survivors.size()) {
      const auto& edge = s2.survivors[cursor];
      if (edge.a_rank < a.first || edge.a_rank >= a.last ||
          edge.b_rank < b.first || edge.b_rank >= b.last) break;
      const auto ordinal = std::uint64_t(edge.a_rank - a.first) * b.size() + (edge.b_rank - b.first);
      if ((previous != std::numeric_limits<std::uint64_t>::max() && ordinal <= previous) ||
          edge.mask == 0 || (edge.mask & ~mask) != 0)
        throw std::runtime_error("S2 survivor order or mask mismatch");
      previous = ordinal;
      rectangle_of.push_back(i);
      ++cursor;
    }
  }
  begin.push_back(cursor);
  if (cursor != s2.survivors.size() || rectangle_of.size() != cursor)
    throw std::runtime_error("S2 survivor not covered by the rectangle partition");

  const auto s3 = run_q34_certificate_batch_cpu(index, kMax, true, s2.survivors, kWorkers);
  if (s3.masks.size() != kSurvivors || s3.deferred.size() != kSurvivors ||
      s3.core_builds != kSurvivors || s3.dead_core.loads != kSurvivors ||
      s3.core_sites != kCoreSites || s3.core_closed_edges != kCoreClosed ||
      s3.core_cover.admitted_sites != kCoreSites)
    throw std::runtime_error("S3 count/core ledger mismatch");

  // S3 publishes only the aggregate F. Rebuild the exact same public core
  // once per edge to attach F_e to the audit trace; this replay cost is not
  // a proposed production path or a speed measurement.
  std::vector<Row> rows;
  rows.reserve(s2.survivors.size());
  std::unordered_set<std::uint64_t> distinct_edges;
  distinct_edges.reserve(s2.survivors.size());
  std::uint64_t sum_f = 0, post_zero = 0;
  std::uint64_t q3_core_proved = 0, q4_core_proved = 0, core_closed = 0;
  std::uint64_t q3_core_proved_f = 0, q4_core_proved_f = 0, core_closed_f = 0;
  Q34DeadLaneProver prover;
  for (std::size_t j = 0; j < s2.survivors.size(); ++j) {
    const auto& edge = s2.survivors[j];
    if (s3.deferred[j] != 0 || (s3.masks[j] & ~edge.mask) != 0)
      throw std::runtime_error("S3 deferred or widened a survivor");
    if (edge.a_rank >= order.size() || edge.b_rank >= order.size())
      throw std::runtime_error("S2 spatial rank outside the index");
    const auto a = order[edge.a_rank], b = order[edge.b_rank];
    if (a > std::numeric_limits<std::uint32_t>::max() || b > std::numeric_limits<std::uint32_t>::max())
      throw std::runtime_error("local endpoint ID exceeds u32");
    const auto raw_a = input.raw_ids.at(a), raw_b = input.raw_ids.at(b);
    if (!distinct_edges.insert(raw_edge_key(raw_a, raw_b)).second)
      throw std::runtime_error("duplicate raw endpoint pair");
    const auto core = Q34EdgeCover::make_diametral(index, {a, b});
    const auto f = core->site_count();
    if (f < 2 || f > kSites) throw std::runtime_error("invalid diametral core size");
    Q34DeadLaneWork work{};
    prover.load(*core, work);
    const auto proved = prover.prove(kMax, edge.mask, work);
    const auto post_core = static_cast<std::uint8_t>(edge.mask & ~proved);
    if ((s3.masks[j] & ~post_core) != 0)
      throw std::runtime_error("S3 reopened a core-proved lane");
    q3_core_proved += (proved & 2U) != 0;
    q4_core_proved += (proved & 4U) != 0;
    core_closed += post_core == 0;
    if ((proved & 2U) != 0) q3_core_proved_f += f;
    if ((proved & 4U) != 0) q4_core_proved_f += f;
    if (post_core == 0) core_closed_f += f;
    sum_f += f;
    post_zero += s3.masks[j] == 0;
    rows.push_back(Row{rectangle_of[j], static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(b),
                       raw_a, raw_b, static_cast<std::uint32_t>(f), edge.mask, post_core, s3.masks[j]});
  }
  if (sum_f != s3.core_sites || post_zero < s3.core_closed_edges || core_closed != s3.core_closed_edges ||
      q3_core_proved != s3.dead_core.q3_proved || q4_core_proved != s3.dead_core.q4_proved)
    throw std::runtime_error("per-edge F or core-proof ledger mismatch");

  std::ofstream trace(trace_path, std::ios::binary | std::ios::trunc);
  if (!trace) throw std::runtime_error("cannot create trace output");
  trace << "s2_ordinal\trectangle_ordinal\tlocal_a\tlocal_b\traw_a\traw_b\ts2_mask\tF\tpost_core_mask\tpost_s3_mask\n";
  for (std::size_t j = 0; j < rows.size(); ++j) {
    const auto& r = rows[j];
    trace << j << '\t' << r.rectangle << '\t' << r.local_a << '\t' << r.local_b << '\t'
          << r.raw_a << '\t' << r.raw_b << '\t' << unsigned(r.s2_mask) << '\t'
          << r.core_sites << '\t' << unsigned(r.post_core_mask) << '\t'
          << unsigned(r.s3_mask) << '\n';
  }
  trace.close();
  if (!trace) throw std::runtime_error("failed writing trace");

  std::ofstream segments(segments_path, std::ios::binary | std::ios::trunc);
  if (!segments) throw std::runtime_error("cannot create segment output");
  segments << "rectangle_ordinal\tbegin\tend\ts2_rectangle_mask\n";
  for (std::size_t i = 0; i < rectangles.size(); ++i)
    segments << i << '\t' << begin[i] << '\t' << begin[i + 1] << '\t'
             << unsigned(s2.rectangle_masks[i]) << '\n';
  segments.close();
  if (!segments) throw std::runtime_error("failed writing segments");

  std::cout << "{\"scope\":\"audit_only_no_ground_quarter_08_000200_K5\","
            << "\"K\":" << kMax << ",\"s\":" << kSeparation << ",\"sites\":" << kSites << ",\"rectangles\":" << rectangles.size()
            << ",\"expanded_pairs\":" << s2.expanded_pairs
            << ",\"survivors\":" << rows.size()
            << ",\"core_builds\":" << s3.core_builds
            << ",\"dead_core_loads\":" << s3.dead_core.loads
            << ",\"core_sites\":" << s3.core_sites
            << ",\"sum_F\":" << sum_f
            << ",\"core_closed_edges\":" << s3.core_closed_edges
            << ",\"post_s3_zero_masks\":" << post_zero
            << ",\"q3_core_proved_edges\":" << q3_core_proved
            << ",\"q4_core_proved_edges\":" << q4_core_proved
            << ",\"q3_core_proved_F\":" << q3_core_proved_f
            << ",\"q4_core_proved_F\":" << q4_core_proved_f
            << ",\"core_closed_F\":" << core_closed_f << "}\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::cerr << "usage: mhgp9_b_s2_trace_k5 points.u32le raw_return_ids.u32le trace.tsv segments.tsv\n";
    return 2;
  }
  try {
    run(argv[1], argv[2], argv[3], argv[4]);
  } catch (const std::exception& e) {
    std::cerr << "b_s2_trace: " << e.what() << '\n';
    return 1;
  }
}
