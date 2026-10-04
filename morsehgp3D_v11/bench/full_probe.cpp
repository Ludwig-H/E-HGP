// Tour entiere : entree -> Cloud -> index -> CatK/lookup -> forets et verticales, puis dump hors chrono moteur.
#include <ctime>
#include <iomanip>
#include <iostream>

#include "whole_input.hpp"
#include "sched/sched.hpp"
#include "tower/forest.hpp"

using namespace mhgp11;
using namespace mhgp11::bench;
using namespace mhgp11::tower_detail;

namespace {
Result<num::Sphere> birth_sphere(const FullTower& tower, const OrderForest& forest, const ForestNode& node) {
  const auto& domain = tower.domain();
  const auto& cloud = domain.index().cloud();
  std::array<num::Point, 4> points{};
  if (forest.order() == 1) {
    const u32 site = node.birth_key;
    auto p = num::Point::make(cloud.x()[site], cloud.y()[site], cloud.z()[site]);
    if (!p.ok()) return p.outcome();
    return num::Sphere::point(p.value());
  }
  const auto& ball = domain.catalogue().balls_data()[node.birth_key];
  for (u32 j = 0; j < ball.qmin; ++j) {
    const u32 site = idx(ball.support[j]);
    auto p = num::Point::make(cloud.x()[site], cloud.y()[site], cloud.z()[site]);
    if (!p.ok()) return p.outcome();
    points[j] = p.value();
  }
  auto made = ball.qmin == 2 ? num::Sphere::through(points[0], points[1]) : ball.qmin == 3 ?
      num::Sphere::through(points[0], points[1], points[2]) :
      num::Sphere::through(points[0], points[1], points[2], points[3]);
  if (!made.ok()) return made.outcome();
  if (!made.value()) return fail(Reason::tower_invariant);
  return *made.value();
}

Outcome serialize(const char* path, const FullTower& tower) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return fail(Reason::output_unwritable);
  const auto& domain = tower.domain();
  const auto& cloud = domain.index().cloud();
  out.write("MHGP11FUL1", 10);
  word(out, kCoordBits); word(out, tower.kmax()); word(out, cloud.sites()); word(out, cloud.weight());
  for (u32 s = 0; s < cloud.sites(); ++s) {
    word(out, cloud.x()[s]); word(out, cloud.y()[s]); word(out, cloud.z()[s]); word(out, cloud.w()[s]);
    for (PointId id : cloud.points(SiteIdx{s})) word(out, idx(id));
  }
  for (u32 k = 1; k <= tower.kmax(); ++k) {
    const auto& forest = tower.order(static_cast<Order>(k));
    word(out, k); word(out, forest.births()); word(out, forest.nodes().size());
    word(out, forest.edges().size()); word(out, idx(forest.root()));
    for (u32 i = 0; i < forest.nodes().size(); ++i) {
      const auto& node = forest.nodes()[i];
      const auto& level = domain.catalogue().levels()[idx(node.rank)];
      word(out, idx(node.parent)); word(out, node.child_begin); word(out, node.child_count);
      integer(out, level.numerator()); integer(out, level.denominator());
      if (i < forest.births()) {
        auto sphere = birth_sphere(tower, forest, node);
        if (!sphere.ok()) return sphere.outcome();
        const auto anchor = sphere.value().anchor();
        const i128 den = sphere.value().denominator();
        for (u32 axis = 0; axis < 3; ++axis)
          integer(out, i128{anchor.coordinates()[axis]} * den + sphere.value().numerator()[axis]);
        integer(out, den);
      }
      if (k > 1) word(out, idx(forest.lower()[i]));
    }
    for (NodeIdx child : forest.edges()) word(out, idx(child));
  }
  out.close();
  return out ? Outcome{} : fail(Reason::output_unwritable);
}

void status(const Outcome& out) {
  std::cout << "\"status\":\"" << status_name(out.status()) << "\",\"reason\":\""
            << reason_name(out.reason) << '"';
}

void parallel_order(const OrderTimings& t) {
  std::cout << ",\"parallel\":{\"regular_batches\":" << t.regular_batches
            << ",\"regular_cells\":" << t.regular_cells << ",\"regular_traces\":" << t.regular_traces
            << ",\"extended_cells\":" << t.extended_cells << ",\"max_regular_batch\":" << t.max_regular_batch
            << ",\"regular_dispatch_ns\":" << t.regular_dispatch_ns
            << ",\"regular_task_sum_ns\":" << t.regular_task_sum_ns
            << ",\"regular_task_max_ns\":" << t.regular_task_max_ns
            << ",\"regular_publish_ns\":" << t.regular_publish_ns << ",\"extended_ns\":" << t.extended_ns << '}';
}

void vertical_order(const OrderTimings& t) {
  std::cout << ",\"vertical_parallel\":{\"vertical_batches\":" << t.vertical_batches
            << ",\"vertical_resolutions\":" << t.vertical_resolutions
            << ",\"max_vertical_batch\":" << t.max_vertical_batch
            << ",\"vertical_dispatch_ns\":" << t.vertical_dispatch_ns
            << ",\"vertical_task_sum_ns\":" << t.vertical_task_sum_ns
            << ",\"vertical_task_max_ns\":" << t.vertical_task_max_ns
            << ",\"vertical_sweep_ns\":" << t.vertical_sweep_ns << '}';
}

void forests(const FullTower& tower, const FullTimings& timings) {
  std::cout << ",\"orders\":[";
  for (u32 k = 1; k <= tower.kmax(); ++k) {
    const auto& f = tower.order(static_cast<Order>(k)); const auto& l = f.ledger();
    const auto& t = timings.orders[k - 1];
    if (k != 1) std::cout << ',';
    std::cout << "{\"k\":" << k << ",\"nodes\":" << f.nodes().size() << ",\"births\":" << f.births()
              << ",\"edges\":" << f.edges().size() << ",\"verticals\":" << f.lower().size()
              << ",\"dense_birth_lookup\":" << (f.dense_birth_lookup() ? "true" : "false")
              << ",\"lookup_reserved_bytes\":" << f.lookup_reserved_bytes()
              << ",\"node_capacity\":" << f.node_capacity() << ",\"edge_capacity\":" << f.edge_capacity()
              << ",\"timings\":{\"classify_ns\":" << t.classify_ns << ",\"births_ns\":" << t.births_ns
              << ",\"plateaus_ns\":" << t.plateaus_ns << ",\"verticals_ns\":" << t.verticals_ns << '}';
    parallel_order(t); vertical_order(t);
    std::cout << ",\"work\":{\"cells\":" << l.classified_cells << ",\"replayed_cells\":" << l.replayed_cells
              << ",\"plateaus\":" << l.plateaus << ",\"traces\":" << l.trace_resolutions
              << ",\"unions\":" << l.unions << ",\"continuations\":" << l.continuations
              << ",\"ancestor_hops\":" << l.ancestor_hops << ",\"descent_steps\":" << l.descent.steps
              << ",\"singleton_hits\":" << l.descent.singleton_hits
              << ",\"population_hits\":" << l.descent.population_hits
              << ",\"catalogue_hits\":" << l.descent.catalogue_hits << ",\"census_calls\":" << l.descent.census_calls
              << ",\"vertical_descents\":" << l.vertical_descents << ",\"vertical_reuses\":" << l.vertical_reuses
              << ",\"vertical_checks\":" << l.vertical_checks
              << ",\"part_meb_presentations\":" << l.descent.part_meb.presentations
              << ",\"part_diameter_pairs\":" << l.descent.part_meb.diameter_pairs
              << ",\"trace_meb_calls\":" << l.descent.trace_meb_calls
              << ",\"trace_meb_presentations\":" << l.descent.trace_meb.presentations
              << ",\"trace_diameter_pairs\":" << l.descent.trace_meb.diameter_pairs
              << ",\"memo_queries\":" << l.descent.memo.queries
              << ",\"memo_lookups\":" << l.descent.memo.lookups
              << ",\"memo_hits\":" << l.descent.memo.hits
              << ",\"memo_misses\":" << l.descent.memo.misses
              << ",\"memo_collisions\":" << l.descent.memo.collisions
              << ",\"memo_insertions\":" << l.descent.memo.insertions
              << ",\"memo_evictions\":" << l.descent.memo.evictions
              << ",\"memo_suffix_hits\":" << l.descent.memo.suffix_hits
              << ",\"census_point_tests\":" << l.descent.census.point_tests
              << ",\"classification_combinations\":" << l.classification.combinations
              << ",\"classification_examined\":" << l.classification.examined
              << ",\"classification_meb_calls\":" << l.classification.meb_calls
              << ",\"classification_meb_presentations\":" << l.classification.meb.presentations
              << ",\"classification_diameter_pairs\":" << l.classification.meb.diameter_pairs
              << ",\"replay_trace_tests\":" << l.cells.trace_tests
              << ",\"replay_meb_calls\":" << l.cells.meb_calls
              << ",\"replay_meb_presentations\":" << l.cells.meb.presentations
              << ",\"replay_diameter_pairs\":" << l.cells.meb.diameter_pairs
              << ",\"ancestor_queries\":" << l.ancestor_queries
              << ",\"ancestor_activations\":" << l.ancestor_activations
              << ",\"ancestor_unions\":" << l.ancestor_unions
              << ",\"ancestor_find_steps\":" << l.ancestor_find_steps << "}}";
  }
  std::cout << ']';
}

void catalogue_execution(const Catalogue& catalogue, const CatalogueTimings& timings) {
  const auto& e = catalogue.execution();
  std::cout << ",\"catalogue_incidences\":" << catalogue.population().size()
            << ",\"single_pass_ns\":" << timings.single_pass_ns << ",\"compact_ns\":" << timings.compact_ns
            << ",\"execution\":{\"geometry_passes\":" << e.geometry_passes
            << ",\"arena_blocks\":" << e.arena_blocks << ",\"arena_capacity_bytes\":" << e.arena_capacity_bytes
            << ",\"arena_metadata_bytes\":" << e.arena_metadata_bytes
            << ",\"compact_records\":" << e.compact_records << ",\"compact_population\":" << e.compact_population << '}';
}

void catalogue_work(const Catalogue& catalogue, bool pair_graph) {
  const auto& w = catalogue.ledger();
  std::cout << ",\"pair_graph\":" << (pair_graph ? "true" : "false")
            << ",\"catalogue_work\":{\"nodes\":" << w.nodes << ",\"leaves\":" << w.leaves
            << ",\"filter_tests\":" << w.filter_tests << ",\"dominance_tests\":" << w.dominance_tests
            << ",\"prefixes\":" << w.prefixes << ",\"judged\":" << w.judged
            << ",\"census_tests\":" << w.census_tests << ",\"emitted\":" << w.emitted
            << ",\"incidences\":" << w.incidences << ",\"q4_candidates\":" << w.q4_candidates
            << ",\"q4_levels\":" << w.q4_levels << ",\"region_pair_tests\":" << w.region_pair_tests
            << ",\"region_pair_rejects\":" << w.region_pair_rejects
            << ",\"region_line_tests\":" << w.region_line_tests << ",\"region_line_rejects\":" << w.region_line_rejects
            << ",\"region_line_evaluations\":" << w.region_line_evaluations
            << ",\"region_line_cache_hits\":" << w.region_line_cache_hits
            << ",\"region_line_fallbacks\":" << w.region_line_fallbacks
            << ",\"max_leaf\":" << w.max_leaf << ",\"max_depth\":" << w.max_depth << '}';
}

Outcome run(char** argv, const CatalogueParams& params, const FullParams& full_params, u64 bytes, u32 workers) {
  MemoryBudget budget(bytes);
  Stopwatch read_clock;
  auto input = read_input(argv[1], argv[2], budget);
  const u64 read_ns = read_clock.nanoseconds();
  if (!input.ok()) return input.outcome();
  Stopwatch cloud_clock;
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                            input.value().ids.span(), CoordWidth(), budget);
  const u64 cloud_ns = cloud_clock.nanoseconds(), cloud_peak = budget.peak();
  if (!cloud.ok()) return cloud.outcome();
  input.value() = {};
  std::cout << "{\"phase\":\"cloud\",\"read_ns\":" << read_ns << ",\"cloud_ns\":" << cloud_ns
            << ",\"sites\":" << cloud.value().sites() << ",\"points\":" << cloud.value().weight()
            << ",\"cloud_peak_bytes\":" << cloud_peak << "}\n" << std::flush;
  Stopwatch pool_clock;
  auto pool = sched::make_pool({workers});
  if (!pool.ok()) return pool.outcome();
  const u64 pool_ns = pool_clock.nanoseconds();
  budget.restart_peak();
  const auto cpu_start = std::clock();
  Stopwatch full_clock, index_clock;
  auto index = build_index(std::move(cloud.value()), {}, budget);
  const u64 index_ns = index_clock.nanoseconds();
  if (!index.ok()) return index.outcome();
  CatalogueTimings timings;
  Stopwatch domain_clock;
  auto domain = prepare_full_domain(std::move(index.value()), params, budget, *pool.value(), &timings);
  const u64 domain_ns = domain_clock.nanoseconds();
  if (!domain.ok()) return domain.outcome();
  std::cout << "{\"phase\":\"domain\",\"index_ns\":" << index_ns << ",\"domain_ns\":" << domain_ns
            << ",\"leaf_size\":" << params.leaf_size
            << ",\"catalogue_balls\":" << domain.value().catalogue().balls() << ",\"pool_ns\":" << pool_ns
            << ",\"sort_ns\":" << timings.sort_ns << ",\"count_ns\":" << timings.count_ns
            << ",\"fill_ns\":" << timings.fill_ns << ",\"prefix_ns\":" << timings.prefix_ns
            << ",\"replay_ns\":" << timings.replay_ns << ",\"level_scan_ns\":" << timings.level_scan_ns
            << ",\"allocation_ns\":" << timings.allocation_ns << ",\"assembly_ns\":" << timings.assembly_ns
            << ",\"catalogue_optimizations\":" << (unsigned(params.cache_center_lines) +
                2 * unsigned(params.indirect_sort) + 4 * unsigned(params.adaptive_frontier) +
                8 * unsigned(params.parallel_assembly) + 16 * unsigned(params.single_pass) +
                32 * unsigned(params.pair_graph));
  catalogue_execution(domain.value().catalogue(), timings);
  catalogue_work(domain.value().catalogue(), params.pair_graph);
  std::cout << "}\n" << std::flush;
  Stopwatch forest_clock;
  FullTimings forest_timings;
  auto tower = build_full(std::move(domain.value()), budget, &forest_timings, full_params, pool.value().get());
  const u64 forest_ns = forest_clock.nanoseconds(), full_ns = full_clock.nanoseconds();
  const double cpu_seconds = double(std::clock() - cpu_start) / CLOCKS_PER_SEC;
  std::cout << "{\"phase\":\"full\","; status(tower.outcome());
  std::cout << ",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << params.kmax << ",\"workers\":" << workers
            << ",\"optimizations\":" << (unsigned(params.cache_center_lines) + 2 * unsigned(params.indirect_sort) +
                                        4 * unsigned(full_params.memo_capacity != 0) +
                                        8 * unsigned(full_params.regular_batch_capacity != 0) +
                                        16 * unsigned(params.adaptive_frontier) + 32 * unsigned(params.parallel_assembly) +
                                        64 * unsigned(params.single_pass) + 128 * unsigned(full_params.parallel_verticals) +
                                        256 * unsigned(full_params.reuse_census_workspace) +
                                        512 * unsigned(full_params.dense_birth_lookup) +
                                        1024 * unsigned(full_params.reuse_regular_verticals) + 2048 * unsigned(params.pair_graph) +
                                        4096 * unsigned(full_params.population_lookup) +
                                        8192 * unsigned(full_params.concurrent_orders))
            << ",\"wall_ns\":" << full_ns << ",\"index_ns\":" << index_ns << ",\"domain_ns\":" << domain_ns
            << ",\"forest_ns\":" << forest_ns << ",\"cpu_seconds\":" << std::setprecision(12) << cpu_seconds
            << ",\"peak_reserved_bytes\":" << budget.peak() << ",\"reserved_after_bytes\":" << budget.used();
  if (tower.ok()) {
    u64 lookup_bytes = 0;
    for (u32 k = 1; k <= tower.value().kmax(); ++k)
      lookup_bytes += tower.value().order(static_cast<Order>(k)).lookup_reserved_bytes();
    // Au plus12 tables, chacune<=8*(kNone-1) : somme<2^39.
    std::cout << ",\"dense_birth_lookup\":" << (full_params.dense_birth_lookup ? "true" : "false")
              << ",\"lookup_reserved_bytes\":" << lookup_bytes;
    std::cout << ",\"memo_capacity\":" << forest_timings.memo_capacity
              << ",\"memo_slot_bytes\":" << forest_timings.memo_slot_bytes
              << ",\"memo_reserved_bytes\":" << forest_timings.memo_reserved_bytes
              << ",\"parallel\":{\"regular_batch_capacity\":" << forest_timings.regular_batch_capacity
              << ",\"descent_lanes\":" << forest_timings.descent_lanes
              << ",\"lane_memo_capacity\":" << forest_timings.lane_memo_capacity
              << ",\"lane_memo_reserved_bytes\":" << forest_timings.lane_memo_reserved_bytes << '}';
    std::cout << ",\"parallel_verticals\":" << (forest_timings.parallel_verticals ? "true" : "false")
              << ",\"reuse_regular_verticals\":" << (forest_timings.reuse_regular_verticals ? "true" : "false")
              << ",\"regular_vertical_reserved_bytes\":" << forest_timings.regular_vertical_reserved_bytes
              << ",\"reuse_census_workspace\":" << (full_params.reuse_census_workspace ? "true" : "false")
              << ",\"census_workspaces\":" << forest_timings.census_workspaces
              << ",\"census_workspace_reserved_bytes\":" << forest_timings.census_workspace_reserved_bytes
              << ",\"population_lookup\":" << (forest_timings.population_lookup ? "true" : "false")
              << ",\"population_lookup_entries\":" << forest_timings.population_lookup_entries
              << ",\"population_lookup_reserved_bytes\":" << forest_timings.population_lookup_reserved_bytes
              << ",\"concurrent_orders\":" << (forest_timings.concurrent_orders ? "true" : "false")
              << ",\"phases\":{\"classify_ns\":" << forest_timings.classify_phase_ns
              << ",\"births_ns\":" << forest_timings.birth_phase_ns
              << ",\"regular_ns\":" << forest_timings.regular_phase_ns
              << ",\"publish_ns\":" << forest_timings.publish_phase_ns
              << ",\"verticals_ns\":" << forest_timings.vertical_phase_ns << '}';
    // Diagnostic T0 du pipeline (hors ledger, hors phases disjointes) : par ordre, debut, CPU du fil et attente
    // bloquee du publieur et du balayage dont il est l'ordre haut ; pour les voies, dernier depart, premiere fin, CPU.
    std::cout << ",\"pipeline_tasks\":{\"lanes_last_start_ns\":" << forest_timings.lanes_last_start_ns
              << ",\"lanes_first_finish_ns\":" << forest_timings.lanes_first_finish_ns
              << ",\"lanes_cpu_ns\":" << forest_timings.lanes_cpu_ns << ",\"orders\":[";
    for (u32 k = 1; tower.ok() && k <= tower.value().kmax(); ++k) {
      const auto& t = forest_timings.orders[k - 1];
      std::cout << (k == 1 ? "" : ",") << "{\"k\":" << k << ",\"publish_start_ns\":" << t.publish_start_ns
                << ",\"publish_cpu_ns\":" << t.publish_cpu_ns << ",\"publish_wait_ns\":" << t.publish_wait_ns
                << ",\"vertical_start_ns\":" << t.vertical_start_ns << ",\"vertical_cpu_ns\":" << t.vertical_cpu_ns
                << ",\"vertical_wait_ns\":" << t.vertical_wait_ns << '}';
    }
    std::cout << "]}";
    forests(tower.value(), forest_timings);
  }
  std::cout << "}\n" << std::flush;
  if (!tower.ok()) return tower.outcome();
  return serialize(argv[3], tower.value());
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 11 && argc != 12) return 2;
  std::array<u64, 7> options{};
  for (int i = 0; i < 7; ++i) if (!parse(argv[i + 4], options[i])) return 2;
  if (options[0] > 12 || options[1] > 1024 || options[2] > 1024 ||
      options[6] < 1 || options[6] > sched::kMaxWorkers) return 2;
  u64 optimizations = 0;
  if (argc == 12 && (!parse(argv[11], optimizations) || optimizations > 16383)) return 2;
  if ((optimizations & 8192) != 0 && (optimizations & 8) == 0) return 2;
  if ((optimizations & 128) != 0 && (optimizations & 8) == 0) return 2;
  CatalogueParams params;
  FullParams full_params{(optimizations & 4) != 0 ? u64{65536} : u64{0}};
  if ((optimizations & 8) != 0) {
    full_params.regular_batch_capacity = 4096;
    full_params.descent_lanes = 48;
    full_params.lane_memo_capacity = (optimizations & 4) != 0 ? u64{4096} : u64{0};
  }
  full_params.parallel_verticals = (optimizations & 128) != 0;
  full_params.reuse_census_workspace = (optimizations & 256) != 0;
  full_params.dense_birth_lookup = (optimizations & 512) != 0;
  full_params.reuse_regular_verticals = (optimizations & 1024) != 0;
  full_params.population_lookup = (optimizations & 4096) != 0;
  full_params.concurrent_orders = (optimizations & 8192) != 0;
  params.cache_center_lines = (optimizations & 1) != 0;
  params.indirect_sort = (optimizations & 2) != 0;
  params.adaptive_frontier = (optimizations & 16) != 0;
  params.parallel_assembly = (optimizations & 32) != 0;
  params.single_pass = (optimizations & 64) != 0;
  params.pair_graph = (optimizations & 2048) != 0;
  params.kmax = static_cast<int>(options[0]); params.leaf_size = static_cast<u32>(options[1]);
  params.max_leaf = static_cast<u32>(options[2]); params.max_nodes = options[3]; params.ball_limit = options[4];
  const auto result = guarded([&]() { return run(argv, params, full_params, options[5], static_cast<u32>(options[6])); });
  std::cout << "{\"phase\":\"exit\","; status(result); std::cout << "}\n";
  return exit_code(result);
}
