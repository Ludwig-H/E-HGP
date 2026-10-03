// Banc CPU du catalogue sur l'entree ENTIERE u32 little-endian, avec PointId externes preserves.
// Hors produit : format de preuve binaire explicite, hache ensuite par le pilote Python.
#include <algorithm>
#include <array>
#include <charconv>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>

#include "catalogue/catalogue.hpp"
#include "catalogue_diagnostics.hpp"
#include "whole_input.hpp"
#include "sched/sched.hpp"

using namespace mhgp11;

namespace {
using namespace mhgp11::bench;

Outcome serialize(const char* path, const Cloud& cloud, const Catalogue& catalogue) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return fail(Reason::output_unwritable);
  out.write("MHGP11CAT1", 10);
  word(out, kCoordBits); word(out, catalogue.kmax()); word(out, cloud.sites());
  for (u32 i = 0; i < cloud.sites(); ++i) {
    word(out, cloud.x()[i]); word(out, cloud.y()[i]); word(out, cloud.z()[i]);
    word(out, cloud.w()[i]);
    for (PointId id : cloud.points(make_id<SiteIdx>(i))) word(out, idx(id));
  }
  word(out, catalogue.levels().size());
  for (const auto& level : catalogue.levels()) {
    integer(out, level.numerator()); integer(out, level.denominator());
  }
  word(out, catalogue.balls());
  for (const auto& ball : catalogue.balls_data()) {
    word(out, ball.qmin); word(out, ball.p); word(out, ball.m); word(out, idx(ball.rank));
    for (SiteIdx s : ball.support) word(out, idx(s));
  }
  for (u64 offset : catalogue.population_offsets()) word(out, offset);
  for (SiteIdx site : catalogue.population()) word(out, idx(site));
  out.close();
  return out ? Outcome{} : fail(Reason::output_unwritable);
}

void status(const Outcome& out) {
  std::cout << "\"status\":\"" << status_name(out.status()) << "\",\"reason\":\""
            << reason_name(out.reason) << '"';
}

void timings(const CatalogueTimings& t) {
  std::cout << ",\"timings\":{\"prefix_ns\":" << t.prefix_ns << ",\"count_ns\":" << t.count_ns
            << ",\"replay_ns\":" << t.replay_ns << ",\"fill_ns\":" << t.fill_ns
            << ",\"sort_ns\":" << t.sort_ns << ",\"level_scan_ns\":" << t.level_scan_ns
            << ",\"allocation_ns\":" << t.allocation_ns << ",\"assembly_ns\":" << t.assembly_ns
            << ",\"count_task_sum_ns\":" << t.count_task_sum_ns
            << ",\"count_task_max_ns\":" << t.count_task_max_ns
            << ",\"fill_task_sum_ns\":" << t.fill_task_sum_ns
            << ",\"fill_task_max_ns\":" << t.fill_task_max_ns
            << ",\"sort_comparisons\":" << t.sort_comparisons << ",\"tasks\":" << t.tasks << '}';
}

Outcome run(char** argv, const CatalogueParams& params, u64 bytes, u32 workers, bool diagnostic_requested) {
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
            << ",\"points\":" << cloud.value().weight() << ",\"sites\":" << cloud.value().sites()
            << ",\"cloud_peak_bytes\":" << cloud_peak << "}\n" << std::flush;
  std::unique_ptr<sched::Pool> pool;
  Stopwatch pool_clock;
  if (workers != 0) {
    auto made = sched::make_pool({workers});
    if (!made.ok()) return made.outcome();
    pool = std::move(made.value());
  }
  const u64 pool_ns = pool_clock.nanoseconds();
  budget.restart_peak();
  const auto cpu_start = std::clock();
  CatalogueTimings measured;
  CatalogueDiagnostics diagnostic;
  Stopwatch watch;
  auto catalogue = pool ? build_catalogue(cloud.value(), params, budget, *pool, &measured,
                                        diagnostic_requested ? &diagnostic : nullptr)
                        : build_catalogue(cloud.value(), params, budget);
  const u64 catalogue_ns = watch.nanoseconds();
  const double cpu_seconds = double(std::clock() - cpu_start) / CLOCKS_PER_SEC;
  std::cout << "{\"phase\":\"catalogue\",";
  status(catalogue.outcome());
  std::cout << ",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << params.kmax
            << ",\"optimizations\":" << (unsigned(params.cache_center_lines) + 2 * unsigned(params.indirect_sort) +
                                            4 * unsigned(params.adaptive_frontier) + 8 * unsigned(params.parallel_assembly))
            << ",\"diagnostics_requested\":" << (diagnostic_requested ? "true" : "false")
            << ",\"wall_ns\":" << catalogue_ns << ",\"cpu_seconds\":" << std::setprecision(12) << cpu_seconds
            << ",\"peak_reserved_bytes\":" << budget.peak() << ",\"reserved_after_bytes\":" << budget.used();
  if (workers != 0) std::cout << ",\"workers\":" << workers << ",\"pool_ns\":" << pool_ns;
  if (catalogue.ok()) {
    if (pool) timings(measured);
    if (diagnostic_requested) {
      std::cout << ",\"diagnostics\":";
      catalogue_diagnostics_json(std::cout, diagnostic);
    }
    const auto& c = catalogue.value(); const auto& l = c.ledger();
    std::cout << ",\"balls\":" << c.balls() << ",\"levels\":" << c.levels().size()
              << ",\"incidences\":" << c.population().size() << ",\"generation_passes\":2"
              << ",\"work\":{\"q4_candidates\":" << l.q4_candidates << ",\"q4_levels\":" << l.q4_levels << '}'
              << ",\"cache_work\":{\"evaluations\":" << l.region_line_evaluations
              << ",\"hits\":" << l.region_line_cache_hits << ",\"fallbacks\":" << l.region_line_fallbacks << '}'
              << ",\"logical\":{\"nodes\":" << l.nodes << ",\"leaves\":" << l.leaves
              << ",\"filter_tests\":" << l.filter_tests << ",\"dominance_tests\":" << l.dominance_tests
              << ",\"region_pair_tests\":" << l.region_pair_tests
              << ",\"region_pair_rejects\":" << l.region_pair_rejects
              << ",\"region_line_tests\":" << l.region_line_tests
              << ",\"region_line_rejects\":" << l.region_line_rejects
              << ",\"prefixes\":" << l.prefixes << ",\"judged\":" << l.judged
              << ",\"census_tests\":" << l.census_tests << ",\"max_leaf\":" << l.max_leaf
              << ",\"max_depth\":" << l.max_depth << '}';
  }
  std::cout << "}\n" << std::flush;
  if (!catalogue.ok()) return catalogue.outcome();
  MHGP11_TRY(serialize(argv[3], cloud.value(), catalogue.value()));
  return {};
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 10 && argc != 11 && argc != 12 && argc != 13) return 2;
  std::array<u64, 6> options{};
  for (int i = 0; i < 6; ++i)
    if (!parse(argv[i + 4], options[i])) return 2;
  if (options[0] > 12 || options[1] > 1024 || options[2] > 1024) return 2;
  u64 workers = 0;
  if (argc >= 11 && (!parse(argv[10], workers) || workers < 1 || workers > sched::kMaxWorkers)) return 2;
  u64 optimizations = 0;
  if (argc >= 12 && (!parse(argv[11], optimizations) || optimizations > 15)) return 2;
  u64 diagnostics = 0;
  if (argc == 13 && (!parse(argv[12], diagnostics) || diagnostics > 1)) return 2;
  CatalogueParams params;
  params.cache_center_lines = (optimizations & 1) != 0;
  params.indirect_sort = (optimizations & 2) != 0;
  params.adaptive_frontier = (optimizations & 4) != 0;
  params.parallel_assembly = (optimizations & 8) != 0;
  params.kmax = static_cast<int>(options[0]); params.leaf_size = static_cast<u32>(options[1]);
  params.max_leaf = static_cast<u32>(options[2]); params.max_nodes = options[3]; params.ball_limit = options[4];
  const Outcome outcome = guarded([&]() {
    return run(argv, params, options[5], static_cast<u32>(workers), diagnostics != 0);
  });
  std::cout << "{\"phase\":\"exit\","; status(outcome); std::cout << "}\n";
  return exit_code(outcome);
}
