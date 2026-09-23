#include "tower_chain.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <ctime>
#include <exception>
#include <limits>
#include <mutex>
#include <new>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <utility>

#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/wspd_q2_parallel.hpp"
#include "pipeline/wspd_q34.hpp"

namespace mhgp9 {

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

ChainResult run_tower_chain(std::span<const gen::Point3> points, const ChainOptions& options) {
  ChainResult result;
  const auto total_start = Clock::now();
  const double cpu_start = process_cpu_s();
  try {
    if (options.kmax < 1 || options.kmax > 10) fail(ChainStatus::kInvalidInput, "chain_kmax_outside_1_10");
    if (options.separation_s < 8) fail(ChainStatus::kInvalidInput, "chain_separation_below_8");
    if (options.workers < 1) fail(ChainStatus::kInvalidInput, "chain_workers_zero");
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
          gen::Q2CensusMode::SharedBlocks, consumers, 16, gen::Q2SiblingMode::Saturating,
          gen::Q2WitnessOrder::ComplementFirst, gen::Q2AnchorMode::Individual, 64,
          gen::WspdQ2Schedule{}, gen::WspdFrontProposals{2, 16, true});
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
      const auto r34 = gen::run_wspd_q34_parallel(
          index, kmax, options.separation_s, o, W,
          [&slots](std::size_t slot, const gen::Q34SeedCandidate& c) {
            Presentation p;
            p.arity = static_cast<std::uint8_t>(c.arity);
            for (unsigned j = 0; j < c.arity; ++j) p.support[j] = static_cast<std::uint32_t>(c.support_ids[j]);
            p.key = c.ball.coefficients();
            p.depth = static_cast<std::uint32_t>(c.depth);
            p.shell = static_cast<std::uint32_t>(c.shell_first.size() + c.shell_second.size());
            slots[slot].push_back(p);
          },
          16);
      result.q34_expanded_pairs = r34.pipeline.work.expanded_pairs;
      result.q34_cover_builds = r34.pipeline.work.cover_builds;
      result.q3_emitted = r34.pipeline.work.q3_emitted;
      result.q4_emitted = r34.pipeline.work.q4_emitted;
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
    }
    gathered = GatheredPresentations{};  // groups is not read past this point
    result.catalogue.shell_over_cap = over_cap.load();
    if (result.catalogue.shell_over_cap > 0) {
      census_clock.stop();
      fail(ChainStatus::kUnsupportedDegeneracy, "chain_shell_above_12");
    }
    result.catalogue.balls = unique;
    result.catalogue.bytes = balls.capacity() * sizeof(tower::BallData);
    census_clock.stop();

    // ---- Tour FULL.
    if (options.keep_catalogue) result.catalogue_balls = balls;
    if (options.run_tower) {
      t = Clock::now();
      const int static_threads = options.tower_static_threads >= 0 ? options.tower_static_threads
                                 : (W > 1 ? static_cast<int>(W) : 0);
      result.tower_static_threads = static_threads;
      auto tw = tower::build_full_ball_tower(ix, balls, kmax, static_threads);
      result.times.tower_ms = ms_since(t);
      result.tower_stats = tw.stats;
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
  }
  result.times.total_ms = ms_since(total_start);
  const double cpu_end = process_cpu_s();
  result.times.cpu_s = (cpu_start < 0 || cpu_end < 0) ? -1.0 : cpu_end - cpu_start;
  // The digest is a verification of the published tower, not part of its
  // construction: timed apart (digest_ms), after the chain total and CPU.
  if (result.status == ChainStatus::kComplete && options.run_tower) {
    const auto t = Clock::now();
    try {
      result.tower_digest = tower_digest(result.tower);
    } catch (const std::exception& e) {
      result.status = ChainStatus::kInvariantViolated;
      result.reason = std::string("chain_digest_failed: ") + e.what();
      result.tower = {};
      result.catalogue_balls.clear();
      result.orders.clear();
    }
    result.times.digest_ms = ms_since(t);
  }
  return result;
}

}  // namespace mhgp9
