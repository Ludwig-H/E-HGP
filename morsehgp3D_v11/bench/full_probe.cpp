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

void forests(const FullTower& tower, const FullTimings& timings) {
  std::cout << ",\"orders\":[";
  for (u32 k = 1; k <= tower.kmax(); ++k) {
    const auto& f = tower.order(static_cast<Order>(k)); const auto& l = f.ledger();
    const auto& t = timings.orders[k - 1];
    if (k != 1) std::cout << ',';
    std::cout << "{\"k\":" << k << ",\"nodes\":" << f.nodes().size() << ",\"births\":" << f.births()
              << ",\"edges\":" << f.edges().size() << ",\"verticals\":" << f.lower().size()
              << ",\"node_capacity\":" << f.node_capacity() << ",\"edge_capacity\":" << f.edge_capacity()
              << ",\"timings\":{\"classify_ns\":" << t.classify_ns << ",\"births_ns\":" << t.births_ns
              << ",\"plateaus_ns\":" << t.plateaus_ns << ",\"verticals_ns\":" << t.verticals_ns << '}'
              << ",\"work\":{\"cells\":" << l.classified_cells << ",\"replayed_cells\":" << l.replayed_cells
              << ",\"plateaus\":" << l.plateaus << ",\"traces\":" << l.trace_resolutions
              << ",\"unions\":" << l.unions << ",\"continuations\":" << l.continuations
              << ",\"ancestor_hops\":" << l.ancestor_hops << ",\"descent_steps\":" << l.descent.steps
              << ",\"vertical_descents\":" << l.vertical_descents << ",\"vertical_checks\":" << l.vertical_checks
              << ",\"part_meb_presentations\":" << l.descent.part_meb.presentations
              << ",\"part_diameter_pairs\":" << l.descent.part_meb.diameter_pairs
              << ",\"trace_meb_calls\":" << l.descent.trace_meb_calls
              << ",\"trace_meb_presentations\":" << l.descent.trace_meb.presentations
              << ",\"trace_diameter_pairs\":" << l.descent.trace_meb.diameter_pairs
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

Outcome run(char** argv, const CatalogueParams& params, u64 bytes, u32 workers) {
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
            << ",\"catalogue_balls\":" << domain.value().catalogue().balls() << ",\"pool_ns\":" << pool_ns
            << ",\"sort_ns\":" << timings.sort_ns << ",\"count_ns\":" << timings.count_ns
            << ",\"fill_ns\":" << timings.fill_ns << "}\n" << std::flush;
  Stopwatch forest_clock;
  FullTimings forest_timings;
  auto tower = build_full(std::move(domain.value()), budget, &forest_timings);
  const u64 forest_ns = forest_clock.nanoseconds(), full_ns = full_clock.nanoseconds();
  const double cpu_seconds = double(std::clock() - cpu_start) / CLOCKS_PER_SEC;
  std::cout << "{\"phase\":\"full\","; status(tower.outcome());
  std::cout << ",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << params.kmax << ",\"workers\":" << workers
            << ",\"optimizations\":" << (unsigned(params.cache_center_lines) + 2 * unsigned(params.indirect_sort))
            << ",\"wall_ns\":" << full_ns << ",\"index_ns\":" << index_ns << ",\"domain_ns\":" << domain_ns
            << ",\"forest_ns\":" << forest_ns << ",\"cpu_seconds\":" << std::setprecision(12) << cpu_seconds
            << ",\"peak_reserved_bytes\":" << budget.peak() << ",\"reserved_after_bytes\":" << budget.used();
  if (tower.ok()) forests(tower.value(), forest_timings);
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
  if (argc == 12 && (!parse(argv[11], optimizations) || optimizations > 3)) return 2;
  CatalogueParams params;
  params.cache_center_lines = (optimizations & 1) != 0;
  params.indirect_sort = (optimizations & 2) != 0;
  params.kmax = static_cast<int>(options[0]); params.leaf_size = static_cast<u32>(options[1]);
  params.max_leaf = static_cast<u32>(options[2]); params.max_nodes = options[3]; params.ball_limit = options[4];
  const auto result = guarded([&]() { return run(argv, params, options[5], static_cast<u32>(options[6])); });
  std::cout << "{\"phase\":\"exit\","; status(result); std::cout << "}\n";
  return exit_code(result);
}
