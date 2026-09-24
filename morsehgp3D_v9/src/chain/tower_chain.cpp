#include "tower_chain.hpp"

#include <algorithm>
#include <bit>
#include <array>
#include <atomic>
#include <chrono>
#include <ctime>
#include <exception>
#include <limits>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <utility>

#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/wspd_q2_parallel.hpp"
#include "pipeline/wspd_q34.hpp"
#include "../gpu/filter_runner.hpp"
#include "../gpu/flat_index.hpp"
#include "../gpu/lanes_host.hpp"

namespace mhgp9 {

const char* euler_status_name(EulerStatus status) {
  switch (status) {
    case EulerStatus::kNotCheckable: return "not_checkable";
    case EulerStatus::kHolds: return "holds";
    case EulerStatus::kFails: return "fails";
  }
  return "unknown";
}

const char* chain_status_name(ChainStatus status) {
  switch (status) {
    case ChainStatus::kComplete: return "complete_relative";
    case ChainStatus::kUnsupportedDegeneracy: return "unsupported_degeneracy";
    case ChainStatus::kInvalidInput: return "invalid_input";
    case ChainStatus::kResourceExhausted: return "resource_exhausted";
    case ChainStatus::kInvariantViolated: return "invariant_violated";
  }
  return "unknown";
}

namespace {

using Clock = std::chrono::steady_clock;
using Key5 = std::array<gen::i128, 5>;

double ms_since(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

double process_cpu_s() {
  timespec ts{};
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0) return -1.0;
  return static_cast<double>(ts.tv_sec) + 1e-9 * static_cast<double>(ts.tv_nsec);
}

struct Failure {
  ChainStatus status;
  std::string reason;
};
[[noreturn]] void fail(ChainStatus status, std::string reason) { throw Failure{status, std::move(reason)}; }
void require(bool ok, const char* reason) {
  if (!ok) fail(ChainStatus::kInvariantViolated, reason);
}

// Why the device batch call gave no answer, as a chain status (never a
// silent CPU fallback): no device or an input-guard refusal (invalid_input,
// the lever cannot be honoured), a capacity limit (resource_exhausted), a
// fault of the pass on a present device (invariant_violated).
struct GpuRefusal : std::runtime_error {
  ChainStatus status;
  GpuRefusal(ChainStatus s, const std::string& reason) : std::runtime_error(reason), status(s) {}
};

// The batch call of gen::run_wspd_q34_batched on the device: flat copy of the
// immutable index (the device guard rechecks u18 boxes and partitions), one
// pass, compacted survivors in rectangle order. A device stack-bound
// violation is an invariant violation of the port, never an answer.
// The device's view of an immutable index (flat nodes, points by spatial
// rank), prepared once per chain.
struct GpuIndex {
  std::vector<gpu::FlatNode> nodes;
  std::vector<gpu::u32> escapes;
  std::vector<std::int32_t> rank_points;
  std::vector<gpu::u32> rank_ids;  // original input ID of every spatial rank (S4a)
};

GpuIndex prepare_gpu_index(const gen::Q2CensusIndex& index) {
  GpuIndex out;
  out.nodes = gpu::flatten_nodes(index);
  out.escapes = gpu::flatten_escapes(index);
  const auto order = index.spatial_order();
  const auto points = index.cloud().points();
  out.rank_points.resize(3 * order.size());
  out.rank_ids.resize(order.size());
  for (std::size_t r = 0; r < order.size(); ++r) {
    const auto& q = points[order[r]];
    out.rank_points[3 * r] = q.x;
    out.rank_points[3 * r + 1] = q.y;
    out.rank_points[3 * r + 2] = q.z;
    if (order[r] > std::numeric_limits<gpu::u32>::max()) throw std::logic_error("chain_gpu_index_id_exceeds_u32");
    out.rank_ids[r] = static_cast<gpu::u32>(order[r]);
  }
  return out;
}

// Opens the CUDA context and prepares the flat index on its own thread while
// q2 runs (the index is immutable: read-only sharing). Joined before the
// batch call and, through the destructor, on every failure path.
class GpuPreparation {
 public:
  GpuPreparation() = default;
  GpuPreparation(const GpuPreparation&) = delete;
  GpuPreparation& operator=(const GpuPreparation&) = delete;
  ~GpuPreparation() { join(); }
  void start(const gen::Q2CensusIndex& index) {
    thread_ = std::thread([this, &index] {
      try {
        static_cast<void>(gpu::warm_up());  // errors are classified by the batch call
        prepared_ = prepare_gpu_index(index);
      } catch (...) {
        failure_ = std::current_exception();
      }
    });
  }
  // The prepared index (rethrows a preparation failure); prepares it here if
  // the thread was never started.
  const GpuIndex& get(const gen::Q2CensusIndex& index) {
    join();
    if (failure_) std::rethrow_exception(failure_);
    if (!prepared_) prepared_ = prepare_gpu_index(index);
    return *prepared_;
  }

 private:
  void join() {
    if (thread_.joinable()) thread_.join();
  }
  std::thread thread_;
  std::optional<GpuIndex> prepared_;
  std::exception_ptr failure_;
};

gen::Q34FilterBatch gpu_filter_batch(const GpuIndex& prepared, std::span<const gen::WspdRectangle> rectangles,
                                     unsigned kmax, double& device_ms, double& kernel_ms, double& transfer_ms) {
  const auto& flat = prepared.nodes;
  const auto& rank_points = prepared.rank_points;
  std::vector<gpu::u32> a(rectangles.size()), b(rectangles.size());
  std::vector<gpu::u8> lanes(rectangles.size());
  for (std::size_t i = 0; i < rectangles.size(); ++i) {
    if (rectangles[i].a_node >= flat.size() || rectangles[i].b_node >= flat.size())
      throw std::logic_error("chain_q34_gpu_rectangle_outside_index");
    a[i] = static_cast<gpu::u32>(rectangles[i].a_node);
    b[i] = static_cast<gpu::u32>(rectangles[i].b_node);
    lanes[i] = rectangles[i].lane_mask;
  }
  gpu::FilterInput in;
  in.nodes = flat.data();
  in.node_count = flat.size();
  in.rank_points = rank_points.data();
  in.rank_count = rank_points.size() / 3;
  in.rect_a = a.data();
  in.rect_b = b.data();
  in.rect_mask = lanes.data();
  in.rect_count = rectangles.size();
  in.kmax = kmax;
  auto out = gpu::run_filter_batch(in);
  switch (out.error_kind) {
    case gpu::BatchError::none:
      break;
    case gpu::BatchError::input_guard:
    case gpu::BatchError::no_device:
      throw GpuRefusal(ChainStatus::kInvalidInput, "chain_q34_gpu_unavailable: " + out.error);
    case gpu::BatchError::capacity:
      throw GpuRefusal(ChainStatus::kResourceExhausted, "chain_q34_gpu_capacity: " + out.error);
    case gpu::BatchError::device_fault:
      throw GpuRefusal(ChainStatus::kInvariantViolated, "chain_q34_gpu_fault: " + out.error);
  }
  if (!out.available || !out.error.empty())
    throw GpuRefusal(ChainStatus::kInvariantViolated, "chain_q34_gpu_fault: unclassified: " + out.error);
  if (out.stack_failure) throw std::logic_error("chain_q34_gpu_stack_bound_violated");
  const std::size_t survivors = out.survivor_mask.size();
  if (out.survivor_a.size() != survivors || out.survivor_b.size() != survivors)
    throw std::logic_error("chain_q34_gpu_survivor_arrays_differ");
  gen::Q34FilterBatch batch;
  batch.backend = out.device;
  batch.rectangle_masks = std::move(out.rect_masks);
  batch.survivors.resize(survivors);
  for (std::size_t j = 0; j < survivors; ++j)
    batch.survivors[j] = {out.survivor_a[j], out.survivor_b[j], out.survivor_mask[j]};
  batch.expanded_pairs = out.pairs;
  batch.pair_q3_rejected = out.pair_q3_rejected;
  batch.pair_q4_rejected = out.pair_q4_rejected;
  batch.rectangle_visits = out.rect_visits;
  batch.pair_visits = out.pair_visits;
  device_ms = out.total_ms;
  kernel_ms = out.rect_ms + out.scan_ms + out.pair_ms + out.select_ms;
  transfer_ms = out.upload_ms + out.download_ms;
  return batch;
}

// S3: the certificate call of gen::run_wspd_q34_batched on the device. Same
// error classes as the filter; an edge the device marks as a fault (a core
// or cover without its endpoints) is an invariant violation of the port.
gen::Q34CertificateBatch gpu_certificate_batch(const GpuIndex& prepared, unsigned kmax, bool dead_core,
                                               std::span<const gen::Q34SurvivingEdge> survivors,
                                               std::uint32_t capacity, double& device_ms, std::uint32_t& warps,
                                               double& kernel_ms, double& transfer_ms) {
  std::vector<gpu::u32> a(survivors.size()), b(survivors.size());
  std::vector<gpu::u8> lanes(survivors.size());
  for (std::size_t i = 0; i < survivors.size(); ++i) {
    a[i] = survivors[i].a_rank;
    b[i] = survivors[i].b_rank;
    lanes[i] = survivors[i].mask;
  }
  gpu::CertificateInput in;
  in.index.nodes = prepared.nodes.data();
  in.index.node_count = prepared.nodes.size();
  in.index.rank_points = prepared.rank_points.data();
  in.index.rank_count = prepared.rank_points.size() / 3;
  in.index.kmax = kmax;
  in.escapes = prepared.escapes.data();
  in.edge_a = a.data();
  in.edge_b = b.data();
  in.edge_mask = lanes.data();
  in.edge_count = survivors.size();
  in.dead_core = dead_core;
  in.capacity = capacity;
  auto out = gpu::run_certificate_batch(in);
  switch (out.error_kind) {
    case gpu::BatchError::none:
      break;
    case gpu::BatchError::input_guard:
    case gpu::BatchError::no_device:
      throw GpuRefusal(ChainStatus::kInvalidInput, "chain_q34_gpu_unavailable: " + out.error);
    case gpu::BatchError::capacity:
      throw GpuRefusal(ChainStatus::kResourceExhausted, "chain_q34_gpu_capacity: " + out.error);
    case gpu::BatchError::device_fault:
      throw GpuRefusal(ChainStatus::kInvariantViolated, "chain_q34_gpu_fault: " + out.error);
  }
  if (!out.available || !out.error.empty())
    throw GpuRefusal(ChainStatus::kInvariantViolated, "chain_q34_gpu_fault: unclassified: " + out.error);
  if (out.faults != 0) throw std::logic_error("chain_q34_gpu_certificate_edge_fault");
  if (out.masks.size() != survivors.size() || out.status.size() != survivors.size())
    throw std::logic_error("chain_q34_gpu_certificate_arrays_differ");
  gen::Q34CertificateBatch batch;
  batch.backend = out.device;
  batch.masks = std::move(out.masks);
  batch.deferred.resize(survivors.size());
  for (std::size_t i = 0; i < survivors.size(); ++i)
    batch.deferred[i] = out.status[i] == static_cast<gpu::u8>(gpu::CertificateStatus::deferred) ? 1 : 0;
  const auto cover = [](const gpu::CoverWork& w) {
    return gen::Q34EdgeCoverWork{w.node_visits,    w.bound_tests,    w.point_tests,     w.admitted_nodes,
                                 w.rejected_nodes, w.split_nodes,    w.admitted_sites,  w.rejected_sites,
                                 w.retained_ranges, w.merged_ranges};
  };
  const auto dead = [](const gpu::DeadWork& w) {
    return gen::Q34DeadLaneWork{w.loads,        w.form_sites,  w.cells,     w.outside_cells, w.deep_cells,
                                w.failed_cells, w.uniform_tests, w.point_tests, w.q3_proved, w.q3_open,
                                w.q4_proved,    w.q4_open};
  };
  const auto& w = out.work;
  batch.core_builds = w.core_builds;
  batch.core_sites = w.core_sites;
  batch.core_closed_edges = w.core_closed_edges;
  batch.core_cover = cover(w.core_cover);
  batch.dead_core = dead(w.dead_core);
  batch.cover_builds = w.cover_builds;
  batch.cover_sites = w.cover_sites;
  batch.max_cover_sites = w.max_cover_sites;
  batch.cover = cover(w.cover);
  batch.dead = dead(w.dead);
  device_ms = out.total_ms;
  warps = out.warps;
  kernel_ms = out.kernel_ms;
  transfer_ms = out.upload_ms + out.download_ms;
  return batch;
}

// S4a: the q3 lanes call of gen::run_wspd_q34_batched, on the device or by
// the host emulation of the same header (gpu/lanes_host.hpp). Only the asked
// survivors are sent; the answer is mapped back to survivor ordinals. Same
// error classes as the other device calls; a faulted edge is an invariant
// violation of the port.
gen::Q34LanesBatch lanes_batch(const GpuIndex& prepared, unsigned kmax, std::span<const gen::Q34SurvivingEdge> survivors,
                               std::span<const std::uint8_t> asked, bool device, std::uint32_t capacity,
                               std::uint32_t events, std::size_t workers, double& device_ms, double& kernel_ms,
                               double& transfer_ms, std::uint32_t& warps) {
  std::vector<std::size_t> where;
  std::vector<gpu::u32> a, b;
  std::vector<gpu::u8> lanes;
  for (std::size_t j = 0; j < survivors.size(); ++j)
    if (asked[j] != 0) {
      where.push_back(j);
      a.push_back(survivors[j].a_rank);
      b.push_back(survivors[j].b_rank);
      lanes.push_back(asked[j]);
    }
  gpu::LanesInput in;
  in.index.nodes = prepared.nodes.data();
  in.index.node_count = prepared.nodes.size();
  in.index.rank_points = prepared.rank_points.data();
  in.index.rank_count = prepared.rank_points.size() / 3;
  in.index.kmax = kmax;
  in.escapes = prepared.escapes.data();
  in.rank_ids = prepared.rank_ids.data();
  in.edge_a = a.data();
  in.edge_b = b.data();
  in.edge_lanes = lanes.data();
  in.edge_count = where.size();
  in.capacity = capacity;
  in.event_capacity = events;
  auto out = device ? gpu::run_lanes_batch(in) : gpu::run_lanes_batch_host(in, workers);
  switch (out.error_kind) {
    case gpu::BatchError::none:
      break;
    case gpu::BatchError::input_guard:
    case gpu::BatchError::no_device:
      throw GpuRefusal(ChainStatus::kInvalidInput, "chain_q34_gpu_unavailable: " + out.error);
    case gpu::BatchError::capacity:
      throw GpuRefusal(ChainStatus::kResourceExhausted, "chain_q34_gpu_capacity: " + out.error);
    case gpu::BatchError::device_fault:
      throw GpuRefusal(ChainStatus::kInvariantViolated, "chain_q34_gpu_fault: " + out.error);
  }
  if (!out.available || !out.error.empty())
    throw GpuRefusal(ChainStatus::kInvariantViolated, "chain_q34_gpu_fault: unclassified: " + out.error);
  if (out.faults != 0) throw std::logic_error("chain_q34_lanes_edge_fault");
  const std::size_t n = where.size();
  if ((n != 0 && (out.status.size() != n || out.record_begin.size() != n || out.record_count.size() != n)))
    throw std::logic_error("chain_q34_lanes_arrays_differ");
  gen::Q34LanesBatch batch;
  batch.backend = out.device;
  batch.decided.assign(survivors.size(), 0);
  batch.record_begin.assign(survivors.size(), 0);
  batch.record_count.assign(survivors.size(), 0);
  for (std::size_t i = 0; i < n; ++i) {
    if (out.status[i] != static_cast<gpu::u8>(gpu::CertificateStatus::decided)) continue;
    batch.decided[where[i]] = lanes[i];
    batch.record_begin[where[i]] = out.record_begin[i];
    batch.record_count[where[i]] = out.record_count[i];
  }
  batch.records.resize(out.records.size());
  tower::parallel_ranges(out.records.size(), static_cast<int>(std::max<std::size_t>(1, workers)),
                         [&](std::size_t first, std::size_t last, std::size_t) {
    for (std::size_t r = first; r < last; ++r) {
      const auto& from = out.records[r];
      auto& to = batch.records[r];
      for (int c = 0; c < 5; ++c) to.key[c] = from.key[c];
      for (int c = 0; c < 4; ++c) to.support[c] = from.support[c];
      if (from.edge >= n) throw std::logic_error("chain_q34_lanes_record_edge_outside_call");
      to.edge = static_cast<std::uint32_t>(where[from.edge]);
      to.depth = from.depth;
      to.shell = from.shell;
      to.arity = static_cast<std::uint8_t>(from.arity == 3 || from.arity == 4 ? from.arity : 0);
      to.shell_sum = from.shell_sum;
      to.shell_xor = from.shell_xor;
    }
  });
  const auto& w = out.work;
  auto& t = batch.work;
  t.edges = w.edges;
  t.cover_sites = w.cover_sites;
  t.max_cover_sites = w.max_cover_sites;
  t.cover = gen::Q34EdgeCoverWork{w.cover.node_visits,    w.cover.bound_tests,    w.cover.point_tests,
                                  w.cover.admitted_nodes, w.cover.rejected_nodes, w.cover.split_nodes,
                                  w.cover.admitted_sites, w.cover.rejected_sites, w.cover.retained_ranges,
                                  w.cover.merged_ranges};
  t.seed_tests = w.seed_tests;
  t.acute_sites = w.acute_sites;
  t.owner_rejections = w.owner_rejections;
  t.seeds = w.seeds;
  t.census_point_tests = w.census_point_tests;
  t.census_inside_sites = w.census_inside_sites;
  t.census_shell_sites = w.census_shell_sites;
  t.census_outside_sites = w.census_outside_sites;
  t.depth_rejections = w.depth_rejections;
  t.emitted = w.emitted;
  t.shell_ids = w.shell_ids;
  t.q3_edges = w.q3_edges;
  t.census_seeds = w.census_seeds;
  const auto& w4 = out.work4;
  batch.work4 = gen::Q34Lanes4Work{w4.edges, w4.seeds, w4.certified, w4.certified_chunk1, w4.survivors,
      w4.pass_chunks, w4.pass_site_tests, w4.buffered_events, w4.max_buffered, w4.live_buckets, w4.filter_steps,
      w4.bucket_events, w4.candidates, w4.foreign_candidates, w4.groups, w4.compare_steps,
      w4.depth_rejected_groups, w4.positivity_tests, w4.groups_without_valid, w4.emitted, w4.emitting_seeds,
      w4.multi_emission_seeds, w4.max_emissions_per_seed, w4.shell_ids, w4.max_group, w4.constant_shell_sites};
  device_ms = out.total_ms;
  kernel_ms = out.kernel_ms;
  transfer_ms = out.upload_ms + out.download_ms;
  warps = out.warps;
  return batch;
}

// Une presentation emise par le generateur : cle, arite presentee (pas q_min),
// support (IDs d'entree, tries), compte exact d'interieurs, taille de coquille.
struct Presentation {
  Key5 key;
  std::uint8_t arity = 0;
  std::array<std::uint32_t, 4> support{};
  std::uint32_t depth = 0;
  std::uint32_t shell = 0;
};

bool presentation_less(const Presentation& a, const Presentation& b) {
  if (a.key != b.key) return a.key < b.key;
  if (a.arity != b.arity) return a.arity < b.arity;
  return a.support < b.support;
}

// The presentations of every worker slot, one representative per key (its
// smallest arity, then support) in increasing key order, after the checks of
// a sorted scan: equal depth and shell within a key, no presentation twice.
// Parallel sample sort, no serial merge: each slot is sorted, key splitters
// cut every slot into the same key ranges, and each range is gathered,
// sorted and scanned on its own. A key never straddles two ranges, so the
// result is the unique sorted order whatever the number of threads.
struct GatheredPresentations {
  std::vector<std::vector<Presentation>> ranges;  // owns the presentations
  std::vector<const Presentation*> representatives;
  std::uint64_t by_arity[5] = {};
};

GatheredPresentations gather_presentations(std::vector<std::vector<Presentation>>& slots, std::size_t workers) {
  const int threads = static_cast<int>(std::max<std::size_t>(1, workers));
  const std::size_t S = slots.size();
  tower::parallel_items(S, threads, [&](std::size_t s, std::size_t) {
    std::sort(slots[s].begin(), slots[s].end(), presentation_less);
  });
  std::size_t total = 0;
  for (const auto& slot : slots) total += slot.size();
  const std::size_t wanted = total < 4096 ? 1 : 4 * std::max<std::size_t>(1, workers);
  std::vector<Key5> sample;
  for (const auto& slot : slots)
    for (std::size_t i = 1; i <= 16 * wanted && !slot.empty(); ++i)
      sample.push_back(slot[(slot.size() * i) / (16 * wanted + 1)].key);
  std::sort(sample.begin(), sample.end());
  sample.erase(std::unique(sample.begin(), sample.end()), sample.end());
  std::vector<Key5> splitters;  // strictly increasing
  for (std::size_t b = 1; b < wanted && !sample.empty(); ++b) {
    const auto& key = sample[(sample.size() * b) / wanted];
    if (splitters.empty() || splitters.back() < key) splitters.push_back(key);
  }
  const std::size_t B = splitters.size() + 1;
  std::vector<std::vector<std::size_t>> cut(S, std::vector<std::size_t>(B + 1, 0));
  tower::parallel_items(S, threads, [&](std::size_t s, std::size_t) {
    const auto& slot = slots[s];
    for (std::size_t b = 1; b < B; ++b)
      cut[s][b] = static_cast<std::size_t>(std::lower_bound(slot.begin(), slot.end(), splitters[b - 1],
          [](const Presentation& p, const Key5& key) { return p.key < key; }) - slot.begin());
    cut[s][B] = slot.size();
  });
  GatheredPresentations out;
  out.ranges.resize(B);
  std::vector<std::vector<const Presentation*>> firsts(B);
  std::vector<std::array<std::uint64_t, 5>> counts(B);
  tower::parallel_items(B, threads, [&](std::size_t b, std::size_t) {
    auto& range = out.ranges[b];
    std::size_t size = 0;
    for (std::size_t s = 0; s < S; ++s) size += cut[s][b + 1] - cut[s][b];
    range.reserve(size);
    for (std::size_t s = 0; s < S; ++s)
      range.insert(range.end(), slots[s].begin() + static_cast<std::ptrdiff_t>(cut[s][b]),
                   slots[s].begin() + static_cast<std::ptrdiff_t>(cut[s][b + 1]));
    std::sort(range.begin(), range.end(), presentation_less);
    auto& count = counts[b];
    count.fill(0);
    for (std::size_t i = 0; i < range.size(); ++i) {
      ++count[std::min<std::size_t>(range[i].arity, 4)];
      if (i == 0 || range[i].key != range[i - 1].key) {
        firsts[b].push_back(&range[i]);
      } else {
        require(range[i].depth == range[i - 1].depth, "chain_presentations_disagree_on_depth");
        require(range[i].shell == range[i - 1].shell, "chain_presentations_disagree_on_shell");
        require(range[i].support != range[i - 1].support || range[i].arity != range[i - 1].arity,
                "chain_duplicate_presentation");
      }
    }
  });
  tower::parallel_items(S, threads, [&](std::size_t s, std::size_t) { std::vector<Presentation>().swap(slots[s]); });
  std::size_t unique = 0;
  for (const auto& f : firsts) unique += f.size();
  out.representatives.reserve(unique);
  for (std::size_t b = 0; b < B; ++b) {
    out.representatives.insert(out.representatives.end(), firsts[b].begin(), firsts[b].end());
    for (std::size_t q = 0; q < 5; ++q) out.by_arity[q] += counts[b][q];
  }
  return out;
}

// Records a phase's elapsed time on every exit, success or exception: work
// paid before a failure is still published.
struct PhaseClock {
  double& out;
  Clock::time_point start = Clock::now();
  bool running = true;
  void stop() {
    if (running) out = ms_since(start);
    running = false;
  }
  ~PhaseClock() { stop(); }
};

tower::P3 to_p3(const gen::Point3& p) { return tower::P3{p.x, p.y, p.z}; }

Key5 to_key5(const tower::BallKey& k) { return Key5{k.a, k.b[0], k.b[1], k.b[2], k.c}; }

// Cle et niveau de la tour depuis un support (formules v7, independantes de v8).
void key_and_level(std::span<const gen::Point3> points, const Presentation& p,
                   tower::BallKey* key, tower::ExactLevel* level) {
  using namespace tower;
  const P3 a = to_p3(points[p.support[0]]), b = to_p3(points[p.support[1]]);
  if (p.arity == 2) {
    *key = q2_ball_key(a, b);
    *level = promote_level(q2_exact_level(p3_norm2(p3_sub(a, b))));
  } else if (p.arity == 3) {
    const P3 x = to_p3(points[p.support[2]]);
    *key = q3_ball_key(q3_form(a, b, x));
    *level = promote_level(q3_exact_level(a, b, x));
  } else {
    const P3 x = to_p3(points[p.support[2]]), y = to_p3(points[p.support[3]]);
    const Q4Form form = q4_form(a, b, x, y);
    require(form.det > 0, "chain_q4_support_flat");
    *key = ball_key_reduce(q4_ball_form(form));
    *level = q4_level_raw(form);
  }
}

// Execute job(i) pour i dans [0, count) sur au plus `workers` fils ; la
// premiere exception arrete la distribution et est relancee apres jointure.
template <class Job>
void parallel_for(std::size_t count, std::size_t workers, Job&& job) {
  workers = std::max<std::size_t>(1, std::min(workers, count));
  if (workers <= 1) {
    for (std::size_t i = 0; i < count; ++i) job(i, std::size_t{0});
    return;
  }
  std::atomic<std::size_t> next{0};
  std::atomic<bool> stop{false};
  std::exception_ptr error;
  std::mutex error_mutex;
  constexpr std::size_t grain = 256;
  const auto body = [&](std::size_t worker) {
    try {
      while (!stop.load(std::memory_order_relaxed)) {
        const std::size_t begin = next.fetch_add(grain);
        if (begin >= count) break;
        const std::size_t end = std::min(count, begin + grain);
        for (std::size_t i = begin; i < end; ++i) job(i, worker);
      }
    } catch (...) {
      stop.store(true);
      std::lock_guard<std::mutex> lock(error_mutex);
      if (!error) error = std::current_exception();
    }
  };
  std::vector<std::thread> threads;
  threads.reserve(workers - 1);
  try {
    for (std::size_t w = 1; w < workers; ++w) threads.emplace_back(body, w);
  } catch (...) {
    stop.store(true);
    for (auto& t : threads) t.join();
    throw;
  }
  body(0);
  for (auto& t : threads) t.join();
  if (error) std::rethrow_exception(error);
}

void fnv(std::uint64_t& h, std::uint64_t word) {
  for (int i = 0; i < 8; ++i) {
    h ^= (word >> (8 * i)) & 0xffu;
    h *= 1099511628211ull;
  }
}
void fnv_level(std::uint64_t& h, const tower::ExactLevel& level) {
  for (auto w : level.num) fnv(h, w);
  const auto den = static_cast<tower::u128>(level.den);
  fnv(h, static_cast<std::uint64_t>(den));
  fnv(h, static_cast<std::uint64_t>(den >> 64));
}

}  // namespace

std::uint64_t tower_digest(const tower::FullBallTowerResult& result) {
  std::uint64_t h = 14695981039346656037ull;
  fnv(h, result.orders.size());
  for (const auto& order : result.orders) {
    const auto& f = order.forest;
    fnv(h, f.order());
    fnv(h, f.nodes().size());
    for (const auto& node : f.nodes()) {
      fnv_level(h, node.level);
      fnv(h, node.first);
      fnv(h, node.parent_count);
    }
    fnv(h, f.parents().size());
    for (auto p : f.parents()) fnv(h, p);
    fnv(h, f.successors().size());
    for (auto s : f.successors()) fnv(h, s);
    fnv(h, f.contributions().size());
    const auto& rows = f.populations()->rows();
    for (const auto& c : f.contributions()) {
      fnv_level(h, c.level);
      fnv(h, c.segment);
      fnv(h, c.ref.shell_mask);
      fnv(h, c.ref.include_interior ? 1u : 0u);
      const auto& row = rows.at(c.ref.population);
      fnv(h, row.interior.size());
      for (auto id : row.interior) fnv(h, id);
      fnv(h, row.shell.size());
      for (auto id : row.shell) fnv(h, id);
    }
    fnv(h, order.lower_nodes.size());
    for (auto l : order.lower_nodes) fnv(h, l);
  }
  return h;
}

std::uint64_t catalogue_digest(const std::vector<tower::BallData>& balls) {
  std::vector<std::size_t> order(balls.size());
  for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](std::size_t x, std::size_t y) { return balls[x].key < balls[y].key; });
  std::uint64_t h = 14695981039346656037ull;
  fnv(h, balls.size());
  std::vector<std::int32_t> ids;
#if defined(MHGP9_CATALOGUE_DIGEST_MUTANT_SKIP_LAST)
  if (!order.empty()) order.pop_back();  // mutant : derniere boule omise (la porte doit le voir)
#endif
  for (const auto i : order) {
    const auto& b = balls[i];
    for (const tower::i128 v : {b.key.a, b.key.b[0], b.key.b[1], b.key.b[2], b.key.c}) {
      fnv(h, static_cast<std::uint64_t>(v));
      fnv(h, static_cast<std::uint64_t>(v >> 64));
    }
    fnv_level(h, b.level);
    fnv(h, b.arity);
    for (const auto part : {b.interior(), b.shell()}) {
      ids.assign(part.begin(), part.end());
#if defined(MHGP9_CATALOGUE_DIGEST_MUTANT_SHELL_ARITY_ONLY)
      // Mutant : coquille etendue hachee sur ses seuls `arite` premiers sites (la porte doit le voir).
      if (part.data() == b.shell().data() && ids.size() > b.arity) ids.resize(b.arity);
#endif
      std::sort(ids.begin(), ids.end());
      fnv(h, ids.size());
      for (const auto id : ids) fnv(h, static_cast<std::uint64_t>(static_cast<std::uint32_t>(id)));
    }
  }
  return h;
}

ChainResult run_tower_chain(std::span<const gen::Point3> points, const ChainOptions& options) {
  ChainResult result;
  const auto total_start = Clock::now();
  const double cpu_start = process_cpu_s();
  // Catalogue digest (verification, not construction): computed while the
  // catalogue is alive, then its wall and CPU are taken out of the chain's.
  double catalogue_ms = 0, catalogue_cpu = 0;
  try {
    if (options.kmax < 1 || options.kmax > 10) fail(ChainStatus::kInvalidInput, "chain_kmax_outside_1_10");
    if (options.separation_s < 8) fail(ChainStatus::kInvalidInput, "chain_separation_below_8");
    if (options.workers < 1) fail(ChainStatus::kInvalidInput, "chain_workers_zero");
    if (options.q34_gpu_filter && !options.q34_batch_filter)
      fail(ChainStatus::kInvalidInput, "chain_q34_gpu_filter_requires_batch_filter");
    if (options.q34_batch_certificates && (!options.q34_batch_filter || !options.q34_dead_lanes))
      fail(ChainStatus::kInvalidInput, "chain_q34_batch_certificates_require_batch_filter_and_dead_lanes");
    if (options.q34_gpu_certificates && !options.q34_batch_certificates)
      fail(ChainStatus::kInvalidInput, "chain_q34_gpu_certificates_require_batch_certificates");
    if (options.q34_certificate_capacity != 0 && (!options.q34_gpu_certificates || options.q34_certificate_capacity < 2))
      fail(ChainStatus::kInvalidInput, "chain_q34_certificate_capacity_requires_gpu_certificates_and_two_sites");
    if (options.q34_certificate_judge && !options.q34_batch_certificates)
      fail(ChainStatus::kInvalidInput, "chain_q34_certificate_judge_requires_batch_certificates");
    if (options.q34_batch_q3 && !options.q34_batch_certificates)
      fail(ChainStatus::kInvalidInput, "chain_q34_batch_q3_requires_batch_certificates");
    if (options.q34_gpu_q3 && !options.q34_batch_q3)
      fail(ChainStatus::kInvalidInput, "chain_q34_gpu_q3_requires_batch_q3");
    if (options.q34_lanes_judge && !options.q34_batch_q3)
      fail(ChainStatus::kInvalidInput, "chain_q34_lanes_judge_requires_batch_q3");
    if (options.q34_lanes_capacity != 0 &&
        (!options.q34_batch_q3 || options.q34_lanes_capacity < 2 || options.q34_lanes_capacity > (1U << 20)))
      fail(ChainStatus::kInvalidInput, "chain_q34_lanes_capacity_requires_batch_q3_and_two_sites");
    if (options.q34_batch_q4 && !options.q34_batch_q3)
      fail(ChainStatus::kInvalidInput, "chain_q34_batch_q4_requires_batch_q3");
    if (options.q34_lanes_events != 0 &&
        (!options.q34_batch_q4 || options.q34_lanes_events > (1U << 20)))
      fail(ChainStatus::kInvalidInput, "chain_q34_lanes_events_requires_batch_q4");
    if (points.size() < 2) fail(ChainStatus::kInvalidInput, "chain_requires_two_sites");
    if (points.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
      fail(ChainStatus::kInvalidInput, "chain_too_many_sites");
    result.sites = points.size();
    const unsigned kmax = options.kmax;
    result.kmax_effective = static_cast<unsigned>(std::min<std::size_t>(kmax, points.size()));
    const std::size_t W = options.workers;

    // ---- Generateur (configuration mesuree des recus v8).
    auto t = Clock::now();
    gen::CloudPtr cloud;
    try {
      cloud = gen::prepare_cloud(points);
    } catch (const std::invalid_argument& e) {
      fail(ChainStatus::kInvalidInput, std::string("chain_prepare: ") + e.what());
    }
    result.times.prepare_ms = ms_since(t);
    t = Clock::now();
    const gen::Q2CensusIndexPtr index = gen::make_q2_cloud_index(cloud);
    result.times.gen_index_ms = ms_since(t);
    // S2: CUDA context and flat index prepared during q2 (joined before q34).
    GpuPreparation gpu_preparation;
    if (options.q34_batch_filter && (options.q34_gpu_filter || options.q34_gpu_certificates || options.q34_gpu_q3) &&
        kmax >= 2)
      gpu_preparation.start(*index);

    std::vector<std::vector<Presentation>> slots(W);
    t = Clock::now();
    {
      std::vector<gen::Q2CensusConsumer> consumers;
      consumers.reserve(W);
      for (std::size_t w = 0; w < W; ++w) {
        auto* out = &slots[w];
        consumers.emplace_back([out, &points](const gen::Q2Support& s) {
          Presentation p;
          const auto lo = std::min(s.a_id, s.b_id), hi = std::max(s.a_id, s.b_id);
          p.arity = 2;
          p.support = {static_cast<std::uint32_t>(lo), static_cast<std::uint32_t>(hi), 0, 0};
          const auto ball = gen::ExactBall::make_q2(points[lo], points[hi]);
          if (!ball) throw std::logic_error("chain_q2_degenerate_pair");
          p.key = ball->coefficients();
          p.depth = static_cast<std::uint32_t>(s.interior.size());
          p.shell = static_cast<std::uint32_t>(s.shell.size());
          out->push_back(p);
        });
      }
      const auto r2 = gen::run_wspd_q2_census_parallel(
          index, kmax, options.separation_s, gen::WspdFrontMode::MidpointSamples,
          gen::Q2CensusMode::SharedBlocks, consumers, options.q2_jobs_by_mass ? 64 : 16,
          gen::Q2SiblingMode::Saturating, gen::Q2WitnessOrder::ComplementFirst, gen::Q2AnchorMode::Individual, 64,
          [&] { gen::WspdQ2Schedule schedule; schedule.mass_first = options.q2_jobs_by_mass; return schedule; }(),
          gen::WspdFrontProposals{2, 16, true});
      result.q2_front_rectangles = r2.input_rectangles;
      result.q2_candidate_pairs = r2.candidate_pairs;
      result.q2_accepted_pairs = r2.accepted_pairs;
    }
    result.times.q2_ms = ms_since(t);

    t = Clock::now();
    if (kmax >= 2) {
      gen::WspdQ34Options o;
      o.front_mode = gen::WspdFrontMode::MidpointSamples;
      o.requested_lane_mask = 6;
      o.q4_backend = gen::WspdQ4Backend::Local28;
      o.local = gen::Q4LocalOptions{};
      o.local.saturate_deep = options.atlas_saturate_deep;
      o.local.retain_q3_fragments = options.q3_leaf_census;
      o.witness_mode = gen::WspdQ34WitnessMode::RectanglePair;
      o.q3_census_mode = gen::WspdQ3CensusMode::GlobalBoxes;
      o.witness_bounds_mode = gen::Q34WitnessBoundsMode::Affine;
      o.q4_seed_cells = gen::Q4SeedCellOptions{gen::Q4SeedCellMode::LiveOnly, 64};
      o.q3_atlas_consultation = true;
      o.q3_leaf_census = options.q3_leaf_census;
      o.dead_lanes = options.q34_dead_lanes;
      o.pair_witness_cache = options.q34_witness_cache;
      o.dead_core = options.q34_dead_core;
      o.jobs_by_mass = options.q34_jobs_by_mass;
      const gen::WspdQ34ParallelConsumer consumer = [&slots](std::size_t slot, const gen::Q34SeedCandidate& c) {
        Presentation p;
        p.arity = static_cast<std::uint8_t>(c.arity);
        for (unsigned j = 0; j < c.arity; ++j) p.support[j] = static_cast<std::uint32_t>(c.support_ids[j]);
        p.key = c.ball.coefficients();
        p.depth = static_cast<std::uint32_t>(c.depth);
        p.shell = static_cast<std::uint32_t>(c.shell_first.size() + c.shell_second.size());
        slots[slot].push_back(p);
      };
      const std::size_t jobs_per_worker = options.q34_fine_jobs ? 64 : 16;
      gen::WspdQ34ParallelResult r34;
      if (!options.q34_batch_filter) {
        r34 = gen::run_wspd_q34_parallel(index, kmax, options.separation_s, o, W, consumer, jobs_per_worker);
      } else {
        double device_ms = 0, filter_kernel_ms = 0, filter_transfer_ms = 0;
        gen::Q34BatchFilter filter;
        if (options.q34_gpu_filter) {
          filter = [&device_ms, &filter_kernel_ms, &filter_transfer_ms, &gpu_preparation](
                       const gen::Q2CensusIndex& ix, unsigned k, std::span<const gen::WspdRectangle> rects) {
            return gpu_filter_batch(gpu_preparation.get(ix), rects, k, device_ms, filter_kernel_ms, filter_transfer_ms);
          };
        } else {
          filter = [W](const gen::Q2CensusIndex& ix, unsigned k, std::span<const gen::WspdRectangle> rects) {
            return gen::run_q34_filter_batch_cpu(ix, k, rects, W);
          };
        }
        double certificate_device_ms = 0, certificate_kernel_ms = 0, certificate_transfer_ms = 0;
        std::uint32_t certificate_warps = 0;
        gen::Q34CertificateJudgeWork judge;
        gen::Q34CertificateFilter certificates;
        if (options.q34_gpu_certificates) {
          const std::uint32_t capacity = options.q34_certificate_capacity;
          certificates = [&certificate_device_ms, &certificate_warps, &certificate_kernel_ms, &certificate_transfer_ms,
                          &gpu_preparation, capacity](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                                                      std::span<const gen::Q34SurvivingEdge> edges) {
            return gpu_certificate_batch(gpu_preparation.get(*ix), k, core, edges, capacity, certificate_device_ms,
                                         certificate_warps, certificate_kernel_ms, certificate_transfer_ms);
          };
        } else if (options.q34_batch_certificates) {
          certificates = [W](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                             std::span<const gen::Q34SurvivingEdge> edges) {
            return gen::run_q34_certificate_batch_cpu(ix, k, core, edges, W);
          };
        }
        if (certificates && options.q34_certificate_judge)
          certificates = gen::judge_certificate_filter(std::move(certificates), W, &judge);
        // S4a: the q3 lanes of the certified survivors, their records turned
        // into presentations of the worker slot that receives them.
        double lanes_device_ms = 0, lanes_kernel_ms = 0, lanes_transfer_ms = 0;
        std::uint32_t lanes_warps = 0;
        gen::Q34LanesJudgeWork lanes_judge;
        gen::Q34LanesStage lanes;
        if (options.q34_batch_q3 && kmax >= 2) {
          const bool device = options.q34_gpu_q3;
          const std::uint32_t capacity = options.q34_lanes_capacity, events = options.q34_lanes_events;
          lanes.filter = [&gpu_preparation, &lanes_device_ms, &lanes_kernel_ms, &lanes_transfer_ms, &lanes_warps,
                          device, capacity, events, W](const gen::Q2CensusIndexPtr& ix, unsigned k,
                                                       std::span<const gen::Q34SurvivingEdge> edges,
                                                       std::span<const std::uint8_t> asked) {
            return lanes_batch(gpu_preparation.get(*ix), k, edges, asked, device, capacity, events, W,
                               lanes_device_ms, lanes_kernel_ms, lanes_transfer_ms, lanes_warps);
          };
          lanes.lanes = options.q34_batch_q4 && kmax >= 3 ? 6 : 2;
          if (options.q34_lanes_judge) lanes.filter = gen::judge_lanes_filter(std::move(lanes.filter), o, W, &lanes_judge);
          lanes.sink = [&slots](std::size_t slot, std::span<const gen::Q34LaneRecord> records) {
            auto& out = slots[slot];
            for (const auto& r : records) {
              Presentation p;
              p.arity = r.arity;
              p.support = {r.support[0], r.support[1], r.support[2], r.arity == 4 ? r.support[3] : 0};
              p.key = r.key;
              p.depth = r.depth;
              p.shell = r.shell;
              out.push_back(p);
            }
          };
          lanes.concurrent = device;
        }
        gen::WspdQ34BatchTiming timing;
        try {
          r34 = gen::run_wspd_q34_batched(index, kmax, options.separation_s, o, W, consumer, jobs_per_worker,
                                          filter, &timing, certificates ? &certificates : nullptr,
                                          lanes.filter ? &lanes : nullptr);
        } catch (const GpuRefusal& e) {
          fail(e.status, e.what());
        }
        auto& b = result.q34_batch;
        b.used = true;
        b.backend = timing.backend;
        b.front_ms = static_cast<double>(timing.front_ns) / 1e6;
        b.filter_ms = static_cast<double>(timing.filter_ns) / 1e6;
        b.edges_ms = static_cast<double>(timing.edges_ns) / 1e6;
        b.device_ms = device_ms;
        b.rectangles = timing.rectangles;
        b.survivors = timing.survivors;
        b.certificate_backend = timing.certificate_backend;
        b.certificate_ms = static_cast<double>(timing.certificate_ns) / 1e6;
        b.certificate_device_ms = certificate_device_ms;
        b.deferred = timing.deferred;
        b.judged_edges = judge.judged;
        b.rebuilt_covers = timing.rebuilt_covers;
        b.certificate_warps = certificate_warps;
        b.filter_kernel_ms = filter_kernel_ms;
        b.filter_transfer_ms = filter_transfer_ms;
        b.certificate_kernel_ms = certificate_kernel_ms;
        b.certificate_transfer_ms = certificate_transfer_ms;
        b.lanes_backend = timing.lanes_backend;
        b.lanes_ms = static_cast<double>(timing.lanes_ns) / 1e6;
        b.lanes_device_ms = options.q34_gpu_q3 ? lanes_device_ms : 0.0;
        b.lanes_kernel_ms = options.q34_gpu_q3 ? lanes_kernel_ms : 0.0;
        b.lanes_transfer_ms = options.q34_gpu_q3 ? lanes_transfer_ms : 0.0;
        b.lanes_wait_ms = static_cast<double>(timing.lanes_wait_ns) / 1e6;
        b.tail_ms = static_cast<double>(timing.tail_ns) / 1e6;
        b.lanes_asked = timing.lanes_asked;
        b.lanes_decided = timing.lanes_decided;
        b.lanes_deferred = timing.lanes_deferred;
        b.lanes_records = timing.lanes_records;
        b.lanes_judged = lanes_judge.judged;
        b.lanes_warps = lanes_warps;
      }
      result.q34_expanded_pairs = r34.pipeline.work.expanded_pairs;
      result.q34_cover_builds = r34.pipeline.work.cover_builds;
      result.q3_emitted = r34.pipeline.work.q3_emitted;
      result.q4_emitted = r34.pipeline.work.q4_emitted;
      {
        auto& o = result.q34_occupancy;
        o.started_workers = r34.parallel.started_workers;
        o.jobs = r34.parallel.jobs;
        o.tasks_published = r34.tasks.published;
        o.tasks_consumed = r34.tasks.consumed;
        o.task_waits = r34.tasks.waits;
        bool first = true;
        for (const auto& timing : r34.worker_timings) {
          const double wall = static_cast<double>(timing.wall_ns) / 1e6;
          o.wall_max_ms = first ? wall : std::max(o.wall_max_ms, wall);
          o.wall_min_ms = first ? wall : std::min(o.wall_min_ms, wall);
          o.cpu_sum_s += static_cast<double>(timing.cpu_ns) / 1e9;
          o.wait_sum_s += static_cast<double>(timing.wait_ns) / 1e9;
          o.job_sum_s += static_cast<double>(timing.job_ns) / 1e9;
          o.max_job_ms = std::max(o.max_job_ms, static_cast<double>(timing.max_job_ns) / 1e6);
          first = false;
        }
      }
      const auto& w = r34.pipeline.work;
      auto& l = result.ledger;
      l.expanded_pairs = w.expanded_pairs; l.cover_builds = w.cover_builds; l.cover_sites = w.cover_sites;
      l.cover_node_visits = w.cover.node_visits; l.q3_edges = w.q3_edges; l.q4_edges = w.q4_edges;
      l.both_edges = w.both_edges; l.witness_input_pair_mass = w.witness.input_pair_mass;
      l.witness_rejected_rectangles = w.witness.rejected_rectangles; l.witness_rejected_pairs = w.witness.rejected_pairs;
      l.q3_seeds = w.q3.seeds; l.q3_ball_builds = w.q3.ball_builds; l.q3_depth_rejections = w.q3.depth_rejections;
      l.q3_census_bounds = w.q3_blocks.count_bounds_prepared; l.q3_census_point_tests = w.q3_blocks.count_point_tests;
      l.q3_atlas_edges = w.q3_atlas.edges_with_atlas; l.q3_atlas_locations = w.q3_atlas.locations;
      l.q3_atlas_rejections = w.q3_atlas.rejections; l.q3_atlas_outside_domain = w.q3_atlas.outside_domain;
      l.atlas_cells = w.local.atlas.cells_created; l.atlas_leaf_cells = w.local.atlas.leaf_cells;
      l.atlas_deep_cells = w.local.atlas.deep_cells; l.atlas_outside_cells = w.local.atlas.outside_cells;
      l.atlas_splits = w.local.atlas.splits; l.atlas_node_visits = w.local.atlas.partition.node_visits;
      l.atlas_block_bounds = w.local.atlas.partition.block_bound_tests;
      l.atlas_point_tests = w.local.atlas.partition.point_tests;
      l.atlas_ids_copied = w.local.atlas.partition.frontier_ids_copied;
      l.q4_seeds = w.local.seeds; l.q4_live_leaves = w.q4_seed_cells.live_leaves;
      l.q4_whole_atlas_skips = w.q4_seed_cells.whole_atlas_skips;
      l.q4_sweep_events = w.local.sweep.kept_events;
      l.q3_leaf_censuses = w.q3_atlas.leaf_censuses; l.q3_leaf_point_tests = w.q3_atlas.leaf_point_tests;
      l.q3_leaf_rejections = w.q3_atlas.leaf_rejections; l.q3_lower_bound_fallbacks = w.q3_atlas.lower_bound_fallbacks;
      l.dead_loads = w.dead.loads; l.dead_form_sites = w.dead.form_sites; l.dead_cells = w.dead.cells;
      l.dead_outside_cells = w.dead.outside_cells; l.dead_deep_cells = w.dead.deep_cells;
      l.dead_failed_cells = w.dead.failed_cells;
      l.dead_uniform_tests = w.dead.uniform_tests; l.dead_point_tests = w.dead.point_tests;
      l.dead_q3_proved = w.dead.q3_proved; l.dead_q3_open = w.dead.q3_open;
      l.dead_q4_proved = w.dead.q4_proved; l.dead_q4_open = w.dead.q4_open;
      l.witness_cache_queries = w.witness_cache.queries; l.witness_cache_node_tests = w.witness_cache.node_tests;
      l.witness_cache_rejected_pairs = w.witness.cache_rejected_pairs;
      l.core_builds = w.core_builds; l.core_sites = w.core_sites; l.core_closed_edges = w.core_closed_edges;
      l.dead_core_loads = w.dead_core.loads; l.dead_core_form_sites = w.dead_core.form_sites;
      l.dead_core_cells = w.dead_core.cells; l.dead_core_uniform_tests = w.dead_core.uniform_tests;
      l.dead_core_point_tests = w.dead_core.point_tests;
      l.dead_core_q3_proved = w.dead_core.q3_proved; l.dead_core_q3_open = w.dead_core.q3_open;
      l.dead_core_q4_proved = w.dead_core.q4_proved; l.dead_core_q4_open = w.dead_core.q4_open;
      l.core_cover_node_visits = w.core_cover.node_visits; l.core_cover_bound_tests = w.core_cover.bound_tests;
      l.core_cover_point_tests = w.core_cover.point_tests; l.dead_core_outside_cells = w.dead_core.outside_cells;
      l.dead_core_deep_cells = w.dead_core.deep_cells; l.dead_core_failed_cells = w.dead_core.failed_cells;
      l.q34_input_rectangles = w.input_rectangles;
      l.witness_rect_queries = w.witness.rectangles.queries; l.witness_rect_node_visits = w.witness.rectangles.node_visits;
      l.witness_pair_queries = w.witness.pairs.queries; l.witness_pair_node_visits = w.witness.pairs.node_visits;
      l.q3_edge_queries = w.q3.edge_queries; l.q3_seed_node_visits = w.q3.seed_node_visits;
      l.q3_seed_point_tests = w.q3.seed_point_tests; l.q3_seed_bound_tests = w.q3.seed_bound_tests;
      l.q4_geometry_preparations = w.local.geometry.preparations;
      l.q4_domain_node_visits = w.local.geometry.domain.node_visits;
      l.q4_cover_decomposition_node_visits = w.local.geometry.cover_node_visits;
      l.q4_seed_node_visits = w.local.node_visits; l.q4_seed_cell_queries = w.q4_seed_cells.queries;
      l.q4_sweep_active_sites = w.local.sweep.active_sites;
      l.lanes_edges = w.lanes.edges; l.lanes_cover_sites = w.lanes.cover_sites;
      l.lanes_cover_node_visits = w.lanes.cover.node_visits; l.lanes_seed_tests = w.lanes.seed_tests;
      l.lanes_acute_sites = w.lanes.acute_sites; l.lanes_owner_rejections = w.lanes.owner_rejections;
      l.lanes_seeds = w.lanes.seeds; l.lanes_census_point_tests = w.lanes.census_point_tests;
      l.lanes_census_inside_sites = w.lanes.census_inside_sites;
      l.lanes_census_shell_sites = w.lanes.census_shell_sites;
      l.lanes_census_outside_sites = w.lanes.census_outside_sites;
      l.lanes_depth_rejections = w.lanes.depth_rejections; l.lanes_emitted = w.lanes.emitted;
      l.lanes_shell_ids = w.lanes.shell_ids;
      l.lanes_q3_edges = w.lanes.q3_edges; l.lanes_census_seeds = w.lanes.census_seeds;
      {
        const auto& q = w.lanes4;
        l.lanes4_edges = q.edges; l.lanes4_seeds = q.seeds; l.lanes4_certified = q.certified;
        l.lanes4_certified_chunk1 = q.certified_chunk1; l.lanes4_survivors = q.survivors;
        l.lanes4_pass_chunks = q.pass_chunks; l.lanes4_pass_site_tests = q.pass_site_tests;
        l.lanes4_buffered_events = q.buffered_events; l.lanes4_max_buffered = q.max_buffered;
        l.lanes4_live_buckets = q.live_buckets; l.lanes4_filter_steps = q.filter_steps;
        l.lanes4_bucket_events = q.bucket_events; l.lanes4_candidates = q.candidates;
        l.lanes4_foreign_candidates = q.foreign_candidates; l.lanes4_groups = q.groups;
        l.lanes4_compare_steps = q.compare_steps; l.lanes4_depth_rejected_groups = q.depth_rejected_groups;
        l.lanes4_positivity_tests = q.positivity_tests; l.lanes4_groups_without_valid = q.groups_without_valid;
        l.lanes4_emitted = q.emitted; l.lanes4_emitting_seeds = q.emitting_seeds;
        l.lanes4_multi_emission_seeds = q.multi_emission_seeds;
        l.lanes4_max_emissions_per_seed = q.max_emissions_per_seed; l.lanes4_shell_ids = q.shell_ids;
        l.lanes4_max_group = q.max_group; l.lanes4_constant_shell_sites = q.constant_shell_sites;
      }
    }
    result.times.q34_ms = ms_since(t);

    // ---- Fusion : une boule par cle (union q2 u q3 u q4).
    PhaseClock merge_clock{result.times.merge_ms};
    auto gathered = gather_presentations(slots, W);
    result.catalogue.q2_presentations = gathered.by_arity[2];
    result.catalogue.q3_presentations = gathered.by_arity[3];
    result.catalogue.q4_presentations = gathered.by_arity[4];
    require(gathered.by_arity[0] == 0 && gathered.by_arity[1] == 0, "chain_presentation_arity");
    result.presentation_ranges = gathered.ranges.size();
    const auto& groups = gathered.representatives;  // one per key, key order
    const std::size_t unique = groups.size();
    result.catalogue.unique_keys = unique;
    merge_clock.stop();

    // ---- Index de la tour (PointId = rang d'entree).
    t = Clock::now();
    tower::CloudIndex ix;
    {
      std::vector<tower::InputPoint> input(points.size());
      for (std::size_t i = 0; i < points.size(); ++i)
        input[i] = tower::InputPoint{static_cast<tower::PointId>(i), to_p3(points[i])};
      ix = tower::build_cloud_index(input);
    }
    require(ix.valid && !ix.has_duplicate_positions() && ix.upos.size() == points.size(),
            "chain_tower_index_invalid");
    std::vector<tower::i32> geo_of_id(points.size(), -1);
    for (tower::i32 u = 0; u < ix.unique_count(); ++u) geo_of_id[ix.point_id(u)] = u;
    result.times.tower_index_ms = ms_since(t);

    // ---- Census exact de chaque cle distincte sur l'index de la tour.
    PhaseClock census_clock{result.times.census_ms};
    std::vector<tower::BallData> balls(unique);
    std::vector<std::uint8_t> keep(unique, 0);
    std::atomic<std::uint64_t> over_cap{0};
    struct WorkerState {
      std::vector<tower::i32> in, sh;
      std::vector<tower::NodeRef> scratch;
      tower::DepthStats depth;
      std::uint64_t extra = 0, max_shell = 0, max_interior = 0;
      std::array<std::uint64_t, 5> by_q{};
      std::array<std::uint64_t, 17> by_shell{};
      std::array<std::int64_t, 11> euler{};  // contributions d'Euler par ordre K (indice K)
    };
    // Coefficient de t^{K-1} dans t^p (t-1)^{j-1} : (-1)^{j-1-i} C(j-1, i), i = K-1-p.
    const auto euler_add = [kmax](std::array<std::int64_t, 11>& e, std::size_t p, std::size_t j, std::int64_t count) {
      static constexpr std::int64_t binom[12][12] = {
          {1}, {1, 1}, {1, 2, 1}, {1, 3, 3, 1}, {1, 4, 6, 4, 1}, {1, 5, 10, 10, 5, 1}, {1, 6, 15, 20, 15, 6, 1},
          {1, 7, 21, 35, 35, 21, 7, 1}, {1, 8, 28, 56, 70, 56, 28, 8, 1}, {1, 9, 36, 84, 126, 126, 84, 36, 9, 1},
          {1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1}, {1, 11, 55, 165, 330, 462, 462, 330, 165, 55, 11, 1}};
      for (std::size_t i = 0; i < j && p + i + 1 <= kmax; ++i)
        e[p + i + 1] += (((j - 1 - i) % 2) ? -1 : 1) * binom[j - 1][i] * count;
    };
    const std::size_t census_workers = std::max<std::size_t>(1, std::min(W, unique / 256 + 1));
    std::vector<WorkerState> states(census_workers);
    parallel_for(unique, census_workers, [&](std::size_t g, std::size_t w) {
      auto& st = states[w];
      const Presentation& rep = *groups[g];  // plus petite arite presentee
      tower::BallKey key;
      tower::ExactLevel level;
      key_and_level(points, rep, &key, &level);
      require(to_key5(key) == rep.key, "chain_key_mismatch_v8_v7");
      const auto status = tower::ball_census(ix, key, rep.depth, std::numeric_limits<std::size_t>::max(),
                                             &st.in, &st.sh, &st.depth, &st.scratch);
      require(status == tower::CensusStatus::kOk, "chain_census_interior_overflow");
      require(st.in.size() == rep.depth, "chain_census_depth_mismatch");
      require(st.sh.size() == rep.shell, "chain_census_shell_mismatch");
      st.by_shell[std::min<std::size_t>(st.sh.size(), 16)]++;
      st.max_shell = std::max<std::uint64_t>(st.max_shell, st.sh.size());
      st.max_interior = std::max<std::uint64_t>(st.max_interior, st.in.size());
      if (st.sh.size() > tower::kBallShellMax) {
        over_cap.fetch_add(1, std::memory_order_relaxed);
        return;  // refus de domaine explicite, jamais une troncature
      }
      require(st.in.size() <= tower::kBallInteriorMax, "chain_interior_above_representation");
      unsigned q = rep.arity;
      if (st.sh.size() != rep.arity) {
        ++st.extra;
        tower::local_plateau::LocalCensus local{key, {}, {}};
        for (auto u : st.in) local.interior.push_back({ix.point_id(u), ix.upos[(std::size_t)u]});
        for (auto u : st.sh) local.shell.push_back({ix.point_id(u), ix.upos[(std::size_t)u]});
        const auto table = tower::local_plateau::ShellTable::prepare(std::move(local));
        q = table.q_min();
        require(q == rep.arity, "chain_qmin_differs_from_min_presented_arity");
        // Euler d'une coquille etendue : sous-coquilles T (|T| >= 2) dont
        // l'enveloppe convexe contient le centre, comptees par taille.
        std::array<std::int64_t, 13> by_size{};
        const auto& contains = table.contains_center();
        for (std::size_t mask = 1; mask < contains.size(); ++mask)
          if (contains[mask]) ++by_size[static_cast<std::size_t>(std::popcount(static_cast<unsigned>(mask)))];
#if defined(MHGP9_EULER_MUTANT_REGULAR_SHELLS_ONLY)
        by_size = {};
        by_size[st.sh.size()] = 1;  // mutant : coquille etendue traitee comme T = U seule
#endif
        for (std::size_t j = 2; j <= st.sh.size(); ++j)
          if (by_size[j]) euler_add(st.euler, st.in.size(), j, by_size[j]);
      } else {
        // Coquille reguliere : T = U seule (centre interieur au support positif).
        euler_add(st.euler, st.in.size(), q, 1);
      }
      require(st.in.size() + q <= std::min<std::size_t>(kmax + 1, points.size()),
              "chain_ball_outside_rank_window");
      st.by_q[q]++;
      auto& b = balls[g];
      b.key = key;
      b.level = level;
      b.arity = static_cast<tower::u8>(q);
      b.n_interior = static_cast<tower::u8>(st.in.size());
      b.n_shell = static_cast<tower::u8>(st.sh.size());
      std::sort(st.in.begin(), st.in.end());
      std::sort(st.sh.begin(), st.sh.end());
      std::copy(st.in.begin(), st.in.end(), b.interior_ids);
      std::copy(st.sh.begin(), st.sh.end(), b.shell_ids);
      keep[g] = 1;
    });
    for (const auto& st : states) {
      result.catalogue.extra_shell_balls += st.extra;
      result.catalogue.max_shell = std::max(result.catalogue.max_shell, st.max_shell);
      result.catalogue.max_interior = std::max(result.catalogue.max_interior, st.max_interior);
      result.catalogue.census_nodes += st.depth.nodes;
      result.catalogue.census_leaf_tests += st.depth.leaf_tests;
      for (std::size_t q = 0; q < 5; ++q) result.catalogue.balls_by_qmin[q] += st.by_q[q];
      for (std::size_t s = 0; s < 17; ++s) result.catalogue.balls_by_shell[s] += st.by_shell[s];
      for (std::size_t k = 1; k <= 10; ++k) result.catalogue.euler_by_k[k] += st.euler[k];
    }
    gathered = GatheredPresentations{};  // groups is not read past this point
    result.catalogue.shell_over_cap = over_cap.load();
    if (result.catalogue.shell_over_cap > 0) {
      census_clock.stop();
      fail(ChainStatus::kUnsupportedDegeneracy, "chain_shell_above_12");
    }
    result.catalogue.balls = unique;
    result.catalogue.bytes = balls.capacity() * sizeof(tower::BallData);
    // Euler : les sites sont les minima de d_1 (terme n a l'ordre 1). Une
    // somme differente de 1 sur un ordre verifiable prouve une cle manquante
    // ou fausse : refus explicite, jamais une tour publiee sur ce catalogue.
#if !defined(MHGP9_EULER_MUTANT_NO_SITE_TERM)
    result.catalogue.euler_by_k[1] += static_cast<std::int64_t>(points.size());
#endif
    result.catalogue.euler_checkable_max_k =
        kmax >= 3 ? static_cast<unsigned>(std::min<std::size_t>(kmax - 2, points.size())) : 0;
    if (result.catalogue.euler_checkable_max_k > 0) {
      result.catalogue.euler_status = EulerStatus::kHolds;
      for (unsigned k = 1; k <= result.catalogue.euler_checkable_max_k; ++k)
        if (result.catalogue.euler_by_k[k] != 1) result.catalogue.euler_status = EulerStatus::kFails;
    }
    census_clock.stop();
    if (result.catalogue.euler_status == EulerStatus::kFails)
      fail(ChainStatus::kInvariantViolated, "chain_catalogue_euler_violated");

    // ---- Tour FULL.
    if (options.keep_catalogue) result.catalogue_balls = balls;
    if (options.run_tower) {
      t = Clock::now();
      const int static_threads = options.tower_static_threads >= 0 ? options.tower_static_threads
                                 : (W > 1 ? static_cast<int>(W) : 0);
      result.tower_static_threads = static_threads;
      auto tw = tower::build_full_ball_tower(ix, balls, kmax, static_threads, {}, options.tower_meb_proposal,
                                             options.tower_overlap_static);
      result.times.tower_ms = ms_since(t);
      result.tower_stats = tw.stats;
      result.tower_times = tw.times;
      if (tw.status != tower::FullBallStatus::kCompleteRelative) {
        const auto s = tw.status == tower::FullBallStatus::kInvalidInput       ? ChainStatus::kInvalidInput
                       : tw.status == tower::FullBallStatus::kResourceExhausted ? ChainStatus::kResourceExhausted
                                                                                : ChainStatus::kInvariantViolated;
        fail(s, std::string("tower: ") + tw.reason);
      }
      for (const auto& order : tw.orders) {
        OrderSummary o;
        o.k = order.forest.order();
        o.nodes = order.forest.nodes().size();
        o.parents = order.forest.parents().size();
        o.contributions = order.forest.contributions().size();
        for (const auto& node : order.forest.nodes()) {
          if (node.parent_count == 0) ++o.births; else ++o.merges;
        }
        result.orders.push_back(o);
      }
      result.tower = std::move(tw);
    }
    if (options.catalogue_digest) {
      const auto digest_start = Clock::now();
      const double digest_cpu = process_cpu_s();
      result.catalogue_digest = catalogue_digest(balls);
      catalogue_ms = ms_since(digest_start);
      const double digest_cpu_end = process_cpu_s();
      if (digest_cpu >= 0 && digest_cpu_end >= 0) catalogue_cpu = digest_cpu_end - digest_cpu;
    }
    result.status = ChainStatus::kComplete;
    result.reason = "complete_relative_to_cross_checked_catalogue";
  } catch (const Failure& f) {
    result.status = f.status;
    result.reason = f.reason;
  } catch (const std::bad_alloc&) {
    result.status = ChainStatus::kResourceExhausted;
    result.reason = "chain_allocation_failed";
  } catch (const std::invalid_argument& e) {
    result.status = ChainStatus::kInvalidInput;
    result.reason = std::string("chain_invalid_argument: ") + e.what();
  } catch (const std::length_error& e) {
    // As in FULL: a size beyond a container's range, or a thread that cannot
    // be launched, is a resource refusal, not an invariant violation.
    result.status = ChainStatus::kResourceExhausted;
    result.reason = std::string("chain_size_overflow: ") + e.what();
  } catch (const std::system_error& e) {
    result.status = ChainStatus::kResourceExhausted;
    result.reason = std::string("chain_thread_launch_failed: ") + e.what();
  } catch (const std::exception& e) {
    result.status = ChainStatus::kInvariantViolated;
    result.reason = std::string("chain_exception: ") + e.what();
  }
  if (result.status != ChainStatus::kComplete) {
    result.tower = {};
    result.catalogue_balls.clear();
    result.orders.clear();
    result.catalogue_digest = 0;
  }
  result.times.total_ms = ms_since(total_start) - catalogue_ms;
  result.times.catalogue_digest_ms = catalogue_ms;
  const double cpu_end = process_cpu_s();
  result.times.cpu_s = (cpu_start < 0 || cpu_end < 0) ? -1.0 : cpu_end - cpu_start - catalogue_cpu;
  // The digest is a verification of the published tower, not part of its
  // construction: timed apart (digest_ms), after the chain total and CPU.
  if (result.status == ChainStatus::kComplete && options.run_tower) {
    const auto t = Clock::now();
    const auto refuse = [&](ChainStatus status, const std::string& reason) {
      result.status = status;
      result.reason = reason;
      result.tower = {};
      result.catalogue_balls.clear();
      result.orders.clear();
      result.tower_digest = 0;
      result.catalogue_digest = 0;
    };
    try {
      result.tower_digest = tower_digest(result.tower);
    } catch (const std::bad_alloc&) {
      refuse(ChainStatus::kResourceExhausted, "chain_digest_allocation_failed");
    } catch (const std::exception& e) {
      refuse(ChainStatus::kInvariantViolated, std::string("chain_digest_failed: ") + e.what());
    }
    result.times.digest_ms = ms_since(t);
  }
  return result;
}

}  // namespace mhgp9
