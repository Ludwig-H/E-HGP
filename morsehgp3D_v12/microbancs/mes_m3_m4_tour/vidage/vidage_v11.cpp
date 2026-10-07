// mhgp12_vidage : vidage de la tour FULL de la v11 gelee pour les microbancs de la v12 (MES-M3, MES-M4). Hors produit.
//
// Lie a la construction v11 (libmhgp11.a, profil u21, moteur ac081a06f) : AUCUNE decision n'est reimplantee ici, tout
// est lu dans les objets de la v11 ou recalcule par ses fonctions (bounded_meb, descent_step, PopulationLookup,
// build_cell, classify_range, census, global_support, find_support).
//
// Sorties (README.md, paragraphe « Formats ») dans <dossier> :
//   cat.bin           sites (SiteIdx), Cat_K canonique : rang, p, m, q, S*, populations I puis U en CSR
//   ordre_<k>.bin     naissances (cle, rang, centre exact, noeud v11), cellules, traces strictes (masques de
//                     coquille), graine v11 de chaque trace, parties de descente ou la v11 calcule une plus petite
//                     boule, avec la route prise (catalogue, census sature, census complet) et B(F) dans Cat_K
//   foret_<k>.bin     foret d'ordre k telle que la v11 la publie (noeuds, parents, rangs, enfants, verticales)
//   [ful1]            vidage MHGP11FUL1 de la sonde mhgp11_full_bench, a l'octet (identite avec les empreintes)
// Sur la sortie standard : lignes JSON (phases, compteurs, controles).
//
// Controles internes (refus, code 3, jamais en silence) : graines rejouees = journal de graines de la v11
// (ForestBuilder::seed_log, voie serie) cellule par cellule ; foret de la voie serie = foret publiee (voie des ordres
// concurrents) ; route catalogue <=> support local present dans la table ; naissances de la foret = classification.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "whole_input.hpp"
#include "sched/sched.hpp"
#include "tower/forest_internal.hpp"
#include "tower/forest_parallel.hpp"
#include "tower/locate.hpp"
#include "tower/population_lookup.hpp"
#include "tower/seed_log.hpp"
#include "../common/format.hpp"
#include "../mes_m3/meb_cert.hpp"

using namespace mhgp11;
using namespace mhgp11::tower_detail;

namespace mhgp12 {
namespace {
namespace d = ::mhgp12::dump;
using Clock = std::chrono::steady_clock;
double seconds_since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

struct Args {
  std::string xyz, ids, frame, out, ful1;
  u32 kmax = 5, leaf = 16, threads = 3;
  u64 budget = u64{16} << 30;
  u32 chrono = 0;             // repetitions de la mesure de resolution a un fil (0 : pas de mesure)
  std::vector<bool> journal;  // ordres dont le journal de graines v11 est rejoue et compare (1..K)
};

[[noreturn]] void usage() {
  std::cerr << "usage : mhgp12_vidage <xyz.u32le> <ids.u32le> <trame> <K> <feuille> <fils> <dossier>"
               " [--ful1 <chemin>] [--journal tous|aucun|k1,k2,...] [--budget <octets>]"
               " [--chrono-resolution R]\n";
  std::exit(2);
}

Args parse_args(int argc, char** argv) {
  if (argc < 8) usage();
  Args a;
  a.xyz = argv[1];
  a.ids = argv[2];
  a.frame = argv[3];
  u64 v = 0;
  if (!bench::parse(argv[4], v) || v < 1 || v > 12) usage();
  a.kmax = static_cast<u32>(v);
  if (!bench::parse(argv[5], v) || v < a.kmax + 3 || v > 256) usage();
  a.leaf = static_cast<u32>(v);
  if (!bench::parse(argv[6], v) || v < 1 || v > 64) usage();
  a.threads = static_cast<u32>(v);
  a.out = argv[7];
  a.journal.assign(a.kmax + 1, true);
  a.journal[0] = false;
  for (int i = 8; i < argc; ++i) {
    const std::string opt = argv[i];
    if (opt == "--ful1" && i + 1 < argc) {
      a.ful1 = argv[++i];
    } else if (opt == "--chrono-resolution" && i + 1 < argc) {
      if (!bench::parse(argv[++i], v) || v > 50) usage();
      a.chrono = static_cast<u32>(v);
    } else if (opt == "--budget" && i + 1 < argc) {
      if (!bench::parse(argv[++i], a.budget)) usage();
    } else if (opt == "--journal" && i + 1 < argc) {
      const std::string list = argv[++i];
      std::fill(a.journal.begin(), a.journal.end(), list == "tous");
      a.journal[0] = false;
      if (list != "tous" && list != "aucun") {
        std::size_t at = 0;
        while (at < list.size()) {
          const std::size_t comma = list.find(',', at);
          const std::string item = list.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
          if (!bench::parse(item, v) || v < 1 || v > a.kmax) usage();
          a.journal[v] = true;
          if (comma == std::string::npos) break;
          at = comma + 1;
        }
      }
    } else {
      usage();
    }
  }
  return a;
}

// Parametres de la sonde mhgp11_full_bench au masque 802811 (voie CPU de reference des empreintes, MESURE.md § 4).
CatalogueParams catalogue_params(const Args& a) {
  CatalogueParams p;
  p.kmax = static_cast<int>(a.kmax);
  p.leaf_size = a.leaf;
  p.max_leaf = 256;
  p.max_nodes = 0;
  p.ball_limit = 4294967295ull;
  p.cache_center_lines = true;
  p.indirect_sort = true;
  p.adaptive_frontier = true;
  p.parallel_assembly = true;
  p.single_pass = true;
  p.pair_graph = true;
  return p;
}
FullParams full_params() {
  FullParams f;
  f.memo_capacity = 0;
  f.regular_batch_capacity = 4096;
  f.descent_lanes = 48;
  f.lane_memo_capacity = 0;
  f.parallel_verticals = true;
  f.reuse_census_workspace = true;
  f.dense_birth_lookup = true;
  f.reuse_regular_verticals = true;
  f.population_lookup = true;
  f.concurrent_orders = true;
  f.place_pipeline = true;
  return f;
}

// Sphere exacte d'une boule par son S* (memes formules que forest_build.cpp, birth_sphere).
Result<num::Sphere> ball_sphere(const FullDomain& domain, u32 ball) {
  const auto& data = domain.catalogue().balls_data()[ball];
  const auto& cloud = domain.index().cloud();
  std::array<num::Point, 4> points{};
  for (u8 j = 0; j < data.qmin; ++j) {
    const u32 s = idx(data.support[j]);
    auto point = num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]);
    if (!point.ok()) return point.outcome();
    points[j] = point.value();
  }
  auto sphere = data.qmin == 2   ? num::Sphere::through(points[0], points[1])
                : data.qmin == 3 ? num::Sphere::through(points[0], points[1], points[2])
                                 : num::Sphere::through(points[0], points[1], points[2], points[3]);
  if (!sphere.ok()) return sphere.outcome();
  if (!sphere.value()) return fail(Reason::tower_invariant);
  return *sphere.value();
}

// Centre exact (numerateurs globaux a*D + N, denominateur D) ; |a*D + N| < 2^(5B+6) <= 2^126.
d::CenterRec center_of(const num::Sphere& sphere) {
  d::CenterRec c{};
  const auto anchor = sphere.anchor();
  const i128 den = sphere.denominator();
  c.x = i128{anchor.coordinates()[0]} * den + sphere.numerator()[0];
  c.y = i128{anchor.coordinates()[1]} * den + sphere.numerator()[1];
  c.z = i128{anchor.coordinates()[2]} * den + sphere.numerator()[2];
  c.d = den;
  return c;
}

// Vidage MHGP11FUL1 a l'octet : copie de serialize() de bench/full_probe.cpp (domaine et forets empruntes).
Outcome write_ful1(const std::string& path, const FullDomain& domain,
                   const std::array<std::optional<OrderForest>, kMaxMebSites>& orders, u32 kmax) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return fail(Reason::output_unwritable);
  const auto& cloud = domain.index().cloud();
  out.write("MHGP11FUL1", 10);
  bench::word(out, kCoordBits); bench::word(out, kmax); bench::word(out, cloud.sites()); bench::word(out, cloud.weight());
  for (u32 s = 0; s < cloud.sites(); ++s) {
    bench::word(out, cloud.x()[s]); bench::word(out, cloud.y()[s]); bench::word(out, cloud.z()[s]);
    bench::word(out, cloud.w()[s]);
    for (PointId id : cloud.points(SiteIdx{s})) bench::word(out, idx(id));
  }
  for (u32 k = 1; k <= kmax; ++k) {
    const auto& forest = *orders[k - 1];
    bench::word(out, k); bench::word(out, forest.births()); bench::word(out, forest.nodes().size());
    bench::word(out, forest.edges().size()); bench::word(out, idx(forest.root()));
    for (u32 i = 0; i < forest.nodes().size(); ++i) {
      const auto& node = forest.nodes()[i];
      const auto& level = domain.catalogue().levels()[idx(node.rank)];
      bench::word(out, idx(node.parent)); bench::word(out, node.child_begin); bench::word(out, node.child_count);
      bench::integer(out, level.numerator()); bench::integer(out, level.denominator());
      if (i < forest.births()) {
        if (k == 1) {
          const u32 site = node.birth_key;
          auto p = num::Point::make(cloud.x()[site], cloud.y()[site], cloud.z()[site]);
          if (!p.ok()) return p.outcome();
          const auto sphere = num::Sphere::point(p.value());
          const auto anchor = sphere.anchor();
          const i128 den = sphere.denominator();
          for (u32 axis = 0; axis < 3; ++axis)
            bench::integer(out, i128{anchor.coordinates()[axis]} * den + sphere.numerator()[axis]);
          bench::integer(out, den);
        } else {
          auto sphere = ball_sphere(domain, node.birth_key);
          if (!sphere.ok()) return sphere.outcome();
          const auto anchor = sphere.value().anchor();
          const i128 den = sphere.value().denominator();
          for (u32 axis = 0; axis < 3; ++axis)
            bench::integer(out, i128{anchor.coordinates()[axis]} * den + sphere.value().numerator()[axis]);
          bench::integer(out, den);
        }
      }
      if (k > 1) bench::word(out, idx(forest.lower()[i]));
    }
    for (NodeIdx child : forest.edges()) bench::word(out, idx(child));
  }
  out.close();
  return out ? Outcome{} : fail(Reason::output_unwritable);
}

// ---- Rejeu instrumente des descentes (meme suite d'appels que PopulationLookup::descend_each_step) ----------------
struct OrderCounters {
  u64 cells = 0, regular_cells = 0, extended_cells = 0, traces = 0, steps = 0, parts = 0;
  u64 table_on_trace = 0, table_after_steps = 0, terminal_steps = 0;
  u64 route_catalogue = 0, route_saturated = 0, route_complete = 0;
  u64 action_interior = 0, action_trace = 0, action_terminal = 0;
  u64 sphere_in_catalogue = 0, sstar_in_f = 0, complete_in_catalogue = 0, saturated_in_catalogue = 0;
  u64 trace_meb_calls = 0, part_meb_presentations = 0, census_calls = 0;
  u64 max_parts_per_trace = 0;
  std::array<u64, 16> parts_histogram{};  // traces par nombre de parties (15 = 15 et plus)
  void add(const OrderCounters& o) {
    cells += o.cells; regular_cells += o.regular_cells; extended_cells += o.extended_cells; traces += o.traces;
    steps += o.steps; parts += o.parts; table_on_trace += o.table_on_trace; table_after_steps += o.table_after_steps;
    terminal_steps += o.terminal_steps; route_catalogue += o.route_catalogue; route_saturated += o.route_saturated;
    route_complete += o.route_complete; action_interior += o.action_interior; action_trace += o.action_trace;
    action_terminal += o.action_terminal; sphere_in_catalogue += o.sphere_in_catalogue; sstar_in_f += o.sstar_in_f;
    complete_in_catalogue += o.complete_in_catalogue; saturated_in_catalogue += o.saturated_in_catalogue;
    trace_meb_calls += o.trace_meb_calls; part_meb_presentations += o.part_meb_presentations;
    census_calls += o.census_calls;
    max_parts_per_trace = std::max(max_parts_per_trace, o.max_parts_per_trace);
    for (std::size_t i = 0; i < parts_histogram.size(); ++i) parts_histogram[i] += o.parts_histogram[i];
  }
};

struct BlockOut {
  std::vector<u32> cell_traces;  // traces par cellule du bloc
  std::vector<u64> masks;
  std::vector<d::SeedRec> seeds;
  std::vector<u32> trace_parts;  // parties par trace
  std::vector<u32> parts;        // k SiteIdx par partie
  std::vector<d::PartRec> infos;
  OrderCounters counters;
  Outcome outcome;
};

struct ReplayContext {
  const FullDomain& domain;
  const PopulationLookup& population;
  const OrderForest& forest;
  u32 k, kmax;
  MemoryBudget& budget;
};

// Identification exacte de la sphere d'une partie complete par un census de seuil K (hors chemin de la v11).
struct GlobalQuery {
  const FullDomain& domain;
  const BoundedMeb& meb;
  std::optional<BallIdx> ball;
  bool complete = false;
  static Outcome consume(void* raw, const BorrowedCensus& population) noexcept {
    auto& q = *static_cast<GlobalQuery*>(raw);
    if (population.kind() != CensusKind::complete) return {};
    q.complete = true;
    auto key = global_support(q.domain, q.meb, population);
    if (!key.ok()) return key.outcome();
    q.ball = q.domain.find_support(key.value().sites);
    return {};
  }
};

Outcome replay_trace(const ReplayContext& c, CensusWorkspace* workspace, std::span<const SiteIdx> trace,
                     BlockOut& out) {
  const u32 k = c.k;
  std::array<SiteIdx, kMaxMebSites> current{};
  std::copy(trace.begin(), trace.end(), current.begin());
  u32 steps = 0;
  std::optional<num::Level> previous;
  std::optional<BirthSeed> seed;
  u8 end = 0;
  OrderCounters& n = out.counters;
  for (;;) {
    const std::span<const SiteIdx> part{current.data(), k};
    auto hit = c.population.hit(part, k);
    if (!hit.ok()) return hit.outcome();
    if (hit.value()) {
      if (previous && num::compare(*hit.value()->level, *previous) >= 0) return fail(Reason::tower_invariant);
      seed = hit.value()->seed;
      end = steps == 0 ? 1 : 2;
      ++(steps == 0 ? n.table_on_trace : n.table_after_steps);
      ++n.steps;
      break;
    }
    auto step = descent_step(c.domain, part, k, c.budget, workspace);
    if (!step.ok()) return step.outcome();
    const DescentLedger& work = step.value().ledger();
    d::PartRec info{};
    if (work.catalogue_hits == 1 && work.census_calls == 0) info.route = d::kRouteCatalogue;
    else if (work.census_calls == 1 && work.catalogue_hits == 0)
      info.route = work.interior_steps == 1 ? d::kRouteCensusSaturated : d::kRouteCensusComplete;
    else return fail(Reason::tower_invariant);
    if (work.interior_steps == 1) info.action = d::kActionInterior;
    else if (work.trace_steps == 1) info.action = d::kActionTrace;
    else if (step.value().seed()) info.action = d::kActionTerminal;
    else return fail(Reason::tower_invariant);
    n.trace_meb_calls += work.trace_meb_calls;
    n.part_meb_presentations += work.part_meb.presentations;
    n.census_calls += work.census_calls;
    // B(F) dans Cat_K : support local canonique (celui de bounded_meb) dans la table, sinon census de seuil K.
    auto meb = bounded_meb(c.domain.index().cloud(), part);
    if (!meb.ok()) return meb.outcome();
    std::array<SiteIdx, 4> key{SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}};
    std::copy(meb.value().support().begin(), meb.value().support().end(), key.begin());
    info.ball = kNone;
    if (const auto found = c.domain.find_support(key)) {
      info.ball = idx(*found);
      info.sstar_in_f = 1;
    } else {
      GlobalQuery query{c.domain, meb.value(), std::nullopt, false};
      MHGP11_TRY(workspace->query(c.domain.index(), meb.value().sphere(), c.kmax, &query, GlobalQuery::consume));
      if (query.ball) info.ball = idx(*query.ball);
      if (info.ball != kNone) ++(info.route == d::kRouteCensusSaturated ? n.saturated_in_catalogue
                                                                        : n.complete_in_catalogue);
    }
    if ((info.route == d::kRouteCatalogue) != (info.sstar_in_f == 1)) return fail(Reason::tower_invariant);
    if (previous && num::compare(step.value().level(), *previous) >= 0) return fail(Reason::tower_invariant);
    n.sphere_in_catalogue += info.ball != kNone;
    n.sstar_in_f += info.sstar_in_f;
    ++(info.route == d::kRouteCatalogue ? n.route_catalogue
       : info.route == d::kRouteCensusSaturated ? n.route_saturated : n.route_complete);
    ++(info.action == d::kActionInterior ? n.action_interior
       : info.action == d::kActionTrace ? n.action_trace : n.action_terminal);
    for (u32 i = 0; i < k; ++i) out.parts.push_back(idx(current[i]));
    out.infos.push_back(info);
    ++steps;
    ++n.steps;
    ++n.parts;
    if (step.value().seed()) {
      seed = *step.value().seed();
      end = 3;
      ++n.terminal_steps;
      break;
    }
    previous = step.value().level();
    const auto next = step.value().next().part();
    if (next.size() != k) return fail(Reason::tower_invariant);
    std::copy(next.begin(), next.end(), current.begin());
  }
  if (steps > 255) return fail(Reason::tower_capacity);
  const auto node = c.forest.birth_node(*seed);
  if (!node || idx(*node) >= c.forest.births()) return fail(Reason::tower_invariant);
  d::SeedRec rec{};
  rec.key = seed->ball() ? idx(*seed->ball()) : idx(*seed->site());
  rec.node = idx(*node);
  rec.end = end;
  rec.steps = static_cast<u8>(steps);
  if (c.forest.birth_nodes()[rec.node].birth_key != rec.key) return fail(Reason::tower_invariant);
  out.seeds.push_back(rec);
  out.trace_parts.push_back(steps);
  ++n.traces;
  n.max_parts_per_trace = std::max<u64>(n.max_parts_per_trace, steps);
  ++n.parts_histogram[std::min<u32>(steps, 15)];
  return {};
}

// Traces strictes d'une cellule, dans l'ordre de la v11 (build_cell ; voie reguliere : sommet omis decroissant).
Outcome cell_traces(const ReplayContext& c, u32 ball, std::vector<std::array<SiteIdx, kMaxMebSites>>& traces,
                    std::vector<u64>& masks) {
  traces.clear();
  masks.clear();
  const auto& cat = c.domain.catalogue();
  const auto& data = cat.balls_data()[ball];
  const auto inner = cat.interior(BallIdx{ball}), shell = cat.shell(BallIdx{ball});
  if (data.m > 64) return fail(Reason::parameter_out_of_range);  // masque de coquille sur 64 bits
  auto made = build_cell(c.domain, BallIdx{ball}, static_cast<Order>(c.k), c.budget);
  if (!made.ok()) return made.outcome();
  if (made.value().kind() != CellKind::strict_traces) return fail(Reason::tower_invariant);
  for (const auto& trace : made.value().traces()) {
    if (trace.arity != c.k) return fail(Reason::tower_invariant);
    std::array<SiteIdx, kMaxMebSites> sites{};
    std::copy(trace.part().begin(), trace.part().end(), sites.begin());
    u64 mask = 0;
    u32 in_shell = 0;
    for (u32 j = 0; j < shell.size(); ++j)
      if (std::binary_search(trace.part().begin(), trace.part().end(), shell[j],
                             [](SiteIdx a, SiteIdx b) { return idx(a) < idx(b); })) {
        mask |= u64{1} << j;
        ++in_shell;
      }
    if (in_shell + inner.size() != c.k) return fail(Reason::tower_invariant);
    traces.push_back(sites);
    masks.push_back(mask);
  }
  return {};
}

Outcome replay_block(const ReplayContext& c, std::span<const u32> cells, CensusWorkspace* workspace, BlockOut& out) {
  std::vector<std::array<SiteIdx, kMaxMebSites>> traces;
  std::vector<u64> masks;
  const auto& cat = c.domain.catalogue();
  for (u32 ball : cells) {
    const auto& data = cat.balls_data()[ball];
    MHGP11_TRY(cell_traces(c, ball, traces, masks));
    ++out.counters.cells;
    ++(data.m == data.qmin ? out.counters.regular_cells : out.counters.extended_cells);
    out.cell_traces.push_back(static_cast<u32>(traces.size()));
    for (std::size_t t = 0; t < traces.size(); ++t) {
      out.masks.push_back(masks[t]);
      MHGP11_TRY(replay_trace(c, workspace, {traces[t].data(), c.k}, out));
    }
  }
  return {};
}

// ---- Petit ordonnanceur : taches independantes, sorties a positions fixes (deterministe) -------------------------
template <class Task>
Outcome run_tasks(u32 threads, u64 count, Task&& task) {
  std::atomic<u64> next{0};
  std::vector<Outcome> outcomes(threads);
  std::vector<std::thread> pool;
  for (u32 w = 0; w < threads; ++w)
    pool.emplace_back([&, w]() {
      for (;;) {
        const u64 i = next.fetch_add(1);
        if (i >= count) return;
        const Outcome o = guarded([&]() { return task(i, w); });
        if (!o.ok()) {
          outcomes[w] = merge(outcomes[w], o);
          next.store(count);
          return;
        }
      }
    });
  for (auto& t : pool) t.join();
  Outcome all{};
  for (const auto& o : outcomes) all = merge(all, o);
  return all;
}

bool same_forest(const OrderForest& a, const OrderForest& b) {
  if (a.births() != b.births() || a.nodes().size() != b.nodes().size() || a.edges().size() != b.edges().size() ||
      a.root() != b.root())
    return false;
  for (u32 i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i];
    const auto& y = b.nodes()[i];
    if (x.rank != y.rank || x.parent != y.parent || x.birth_key != y.birth_key || x.child_count != y.child_count)
      return false;
    const auto cx = a.children(NodeIdx{i}), cy = b.children(NodeIdx{i});
    if (!std::equal(cx.begin(), cx.end(), cy.begin(), cy.end())) return false;
  }
  return true;
}

void print_counters(u32 k, const OrderCounters& n, const ForestLedger& v11, double seconds) {
  std::cout << "{\"phase\":\"ordre\",\"k\":" << k << ",\"cells\":" << n.cells << ",\"regular_cells\":" << n.regular_cells
            << ",\"extended_cells\":" << n.extended_cells << ",\"traces\":" << n.traces << ",\"steps\":" << n.steps
            << ",\"parts\":" << n.parts << ",\"table_on_trace\":" << n.table_on_trace
            << ",\"table_after_steps\":" << n.table_after_steps << ",\"terminal_steps\":" << n.terminal_steps
            << ",\"route_catalogue\":" << n.route_catalogue << ",\"route_census_saturated\":" << n.route_saturated
            << ",\"route_census_complete\":" << n.route_complete << ",\"action_interior\":" << n.action_interior
            << ",\"action_trace\":" << n.action_trace << ",\"action_terminal\":" << n.action_terminal
            << ",\"sphere_in_catalogue\":" << n.sphere_in_catalogue << ",\"sstar_in_f\":" << n.sstar_in_f
            << ",\"census_complete_in_catalogue\":" << n.complete_in_catalogue
            << ",\"census_saturated_in_catalogue\":" << n.saturated_in_catalogue
            << ",\"census_calls\":" << n.census_calls << ",\"trace_meb_calls\":" << n.trace_meb_calls
            << ",\"part_meb_presentations\":" << n.part_meb_presentations
            << ",\"max_parts_per_trace\":" << n.max_parts_per_trace << ",\"parts_histogram\":[";
  for (std::size_t i = 0; i < n.parts_histogram.size(); ++i) std::cout << (i ? "," : "") << n.parts_histogram[i];
  std::cout << "],\"v11_ledger\":{\"descent_steps\":" << v11.descent.steps
            << ",\"population_hits\":" << v11.descent.population_hits
            << ",\"catalogue_hits\":" << v11.descent.catalogue_hits << ",\"census_calls\":" << v11.descent.census_calls
            << ",\"trace_meb_calls\":" << v11.descent.trace_meb_calls
            << ",\"part_meb_presentations\":" << v11.descent.part_meb.presentations
            << ",\"traces\":" << v11.trace_resolutions << ",\"vertical_descents\":" << v11.vertical_descents
            << "},\"seconds\":" << seconds << "}\n"
            << std::flush;
}

Outcome dump_catalogue(const Args& a, const FullDomain& domain) {
  const auto& cloud = domain.index().cloud();
  const auto& cat = domain.catalogue();
  d::Writer w(a.out + "/cat.bin", d::kCatalogue, kCoordBits, a.kmax, 0, cloud.sites(), a.frame);
  std::vector<u32> xyz(3 * u64{cloud.sites()});
  for (u32 s = 0; s < cloud.sites(); ++s) {
    xyz[3 * u64{s}] = cloud.x()[s];
    xyz[3 * u64{s} + 1] = cloud.y()[s];
    xyz[3 * u64{s} + 2] = cloud.z()[s];
    if (cloud.w()[s] != 1) return fail(Reason::parameter_out_of_range);  // multiplicites refusees
  }
  w.raw("SITEXYZ", 12, xyz.data(), cloud.sites());
  std::vector<d::BallRec> balls(cat.balls());
  for (u32 b = 0; b < cat.balls(); ++b) {
    const auto& data = cat.balls_data()[b];
    d::BallRec& r = balls[b];
    r.rank = idx(data.rank);
    r.p = data.p;
    r.m = data.m;
    r.q = data.qmin;
    for (u32 j = 0; j < 4; ++j) r.sstar[j] = idx(data.support[j]);
  }
  w.section("BALLS", balls);
  const auto off = cat.population_offsets();
  w.section("POPOFF", off.data(), off.size());
  static_assert(sizeof(SiteIdx) == 4);
  w.raw("POPVAL", 4, cat.population().data(), cat.population().size());
  const u64 levels = cat.levels().size();
  w.section("NLEVELS", &levels, 1);
  w.close();
  return {};
}

Outcome dump_forest(const Args& a, u32 k, const OrderForest& forest) {
  d::Writer w(a.out + "/foret_" + std::to_string(k) + ".bin", d::kForest, kCoordBits, a.kmax, k, 0, a.frame);
  std::vector<d::NodeRec> nodes(forest.nodes().size());
  for (u32 i = 0; i < forest.nodes().size(); ++i) {
    const auto& n = forest.nodes()[i];
    nodes[i] = {idx(n.rank), idx(n.parent), n.birth_key, n.child_count, n.child_begin};
  }
  w.section("FNODES", nodes);
  std::vector<u32> edges(forest.edges().size());
  for (u64 i = 0; i < edges.size(); ++i) edges[i] = idx(forest.edges()[i]);
  w.section("FEDGES", edges);
  if (k > 1) {
    std::vector<u32> lower(forest.lower().size());
    for (u64 i = 0; i < lower.size(); ++i) lower[i] = idx(forest.lower()[i]);
    w.section("FLOWER", lower);
  }
  const std::array<u64, 3> meta{forest.births(), idx(forest.root()), forest.edges().size()};
  w.section("FMETA", meta.data(), meta.size());
  w.close();
  return {};
}

// Naissances de l'ordre k (cle croissante), centres exacts et noeud v11 ; controle contre la classification.
Outcome dump_order(const Args& a, u32 k, const FullDomain& domain, const OrderForest& forest,
                   std::span<const u8> kinds, const std::vector<u32>& cells, std::vector<BlockOut>& blocks,
                   u64 total_traces, u64 total_parts) {
  const auto& cat = domain.catalogue();
  const auto& cloud = domain.index().cloud();
  d::Writer w(a.out + "/ordre_" + std::to_string(k) + ".bin", d::kOrder, kCoordBits, a.kmax, k, cloud.sites(),
              a.frame);
  const u32 nb = forest.births();
  std::vector<d::BirthRec> births(nb);
  std::vector<d::CenterRec> centers(nb);
  std::vector<u32> by_key;  // noeud de naissance par cle
  const u32 keys = k == 1 ? cloud.sites() : cat.balls();
  by_key.assign(keys, kNone);
  for (u32 i = 0; i < nb; ++i) {
    const u32 key = forest.birth_nodes()[i].birth_key;
    if (key >= keys || by_key[key] != kNone) return fail(Reason::tower_invariant);
    by_key[key] = i;
  }
  u32 written = 0;
  for (u32 key = 0; key < keys; ++key) {
    const bool birth = k == 1 || kinds[key] == 1;
    if (birth != (by_key[key] != kNone)) return fail(Reason::tower_invariant);  // naissances = classification
    if (!birth) continue;
    const u32 node = by_key[key];
    d::BirthRec& r = births[written];
    r.key = key;
    r.rank = idx(forest.birth_nodes()[node].rank);
    r.v11_node = node;
    r.flags = 0;
    if (k == 1) {
      centers[written] = {i128{cloud.x()[key]}, i128{cloud.y()[key]}, i128{cloud.z()[key]}, i128{1}};
    } else {
      const auto& data = cat.balls_data()[key];
      r.flags = data.m > data.qmin ? 1u : 0u;
      if (idx(data.rank) != r.rank) return fail(Reason::tower_invariant);
      auto sphere = ball_sphere(domain, key);
      if (!sphere.ok()) return sphere.outcome();
      centers[written] = center_of(sphere.value());
    }
    ++written;
  }
  if (written != nb) return fail(Reason::tower_invariant);
  w.section("BIRTHS", births);
  w.section("BCENTER", centers);
  std::vector<d::CellRec> cell_recs(cells.size());
  for (std::size_t i = 0; i < cells.size(); ++i) {
    const auto& data = cat.balls_data()[cells[i]];
    cell_recs[i] = {cells[i], idx(data.rank), data.p, data.m, data.qmin, data.m > data.qmin ? 1u : 0u};
  }
  w.section("CELLS", cell_recs);
  // Concatenation des blocs dans l'ordre des cellules.
  std::vector<u64> cell_off;
  cell_off.reserve(cells.size() + 1);
  cell_off.push_back(0);
  std::vector<u64> masks;
  masks.reserve(total_traces);
  std::vector<d::SeedRec> seeds;
  seeds.reserve(total_traces);
  std::vector<u64> part_off;
  part_off.reserve(total_traces + 1);
  part_off.push_back(0);
  for (auto& b : blocks) {
    for (u32 t : b.cell_traces) cell_off.push_back(cell_off.back() + t);
    masks.insert(masks.end(), b.masks.begin(), b.masks.end());
    seeds.insert(seeds.end(), b.seeds.begin(), b.seeds.end());
    for (u32 p : b.trace_parts) part_off.push_back(part_off.back() + p);
  }
  if (cell_off.size() != cells.size() + 1 || masks.size() != total_traces || seeds.size() != total_traces ||
      part_off.back() != total_parts)
    return fail(Reason::tower_invariant);
  w.section("CELLOFF", cell_off);
  w.section("TRACEA", masks);
  w.section("SEEDS", seeds);
  w.section("PARTOFF", part_off);
  {  // Parties : k SiteIdx par element ; ecrites bloc par bloc sans copie globale.
    u64 count = 0;
    for (const auto& b : blocks) count += b.infos.size();
    std::vector<u32> parts;
    parts.reserve(count * k);
    for (auto& b : blocks) {
      parts.insert(parts.end(), b.parts.begin(), b.parts.end());
      std::vector<u32>().swap(b.parts);
    }
    w.raw("PARTS", 4 * k, parts.data(), count);
  }
  std::vector<d::PartRec> infos;
  infos.reserve(total_parts);
  for (auto& b : blocks) {
    infos.insert(infos.end(), b.infos.begin(), b.infos.end());
    std::vector<d::PartRec>().swap(b.infos);
  }
  w.section("PARTINF", infos);
  w.close();
  return {};
}

// ---- Mesure de resolution a un fil (regle d'adoption de MES-M3) ----------------------------------------------------
// Trois bras sur les memes traces, dans le meme processus, un seul fil :
//   v11            : PopulationLookup::descend_each_step de la v11 (chemin produit des cellules etendues, et des
//                    regulieres hors voie liee), registres de travail compris ;
//   replique_v11   : la meme descente reecrite pas a pas (DescentBuilder::run, locate, strict_trace de la v11), avec la
//                    plus petite boule de la v11 (bounded_meb, enumeration exhaustive) ;
//   replique_v12   : la meme replique, plus petite boule proposee puis certifiee (mes_m3/meb_cert.hpp, LEM-T1 corrige).
// Les graines des trois bras doivent etre identiques trace par trace (sinon refus, code 3). Seule la plus petite boule
// differe entre les deux repliques : leur rapport isole l'effet de LEV-MEB-CERT sur le CPU de resolution.
struct CensusCopy {
  std::vector<u32>* interior;
  std::vector<u32>* shell;
  bool complete = false;
  static Outcome consume(void* raw, const BorrowedCensus& population) noexcept {
    auto& c = *static_cast<CensusCopy*>(raw);
    c.complete = population.kind() == CensusKind::complete;
    c.interior->clear();
    c.shell->clear();
    for (SiteIdx s : population.interior()) c.interior->push_back(idx(s));
    for (SiteIdx s : population.shell()) c.shell->push_back(idx(s));
    return {};
  }
};

struct Replica {
  const FullDomain& domain;
  const PopulationLookup& population;
  const mebcert::Ctx& ctx;
  u32 kmax;
  std::vector<u32> interior, shell;  // copies du census (un fil)
  std::array<u64, mebcert::kRouteCount> routes{};
  u64 meb_steps = 0, census = 0;
};

template <bool NewMeb>
Result<u32> resolve_replica(Replica& r, CensusWorkspace* ws, const SiteIdx* trace, u32 k) {
  const auto& cat = r.domain.catalogue();
  const auto& cloud = r.domain.index().cloud();
  std::array<SiteIdx, kMaxMebSites> cur{};
  std::copy(trace, trace + k, cur.begin());
  for (;;) {
    auto hit = r.population.hit({cur.data(), k}, k);
    if (!hit.ok()) return hit.outcome();
    if (hit.value()) {
      const auto& seed = hit.value()->seed;
      return seed.ball() ? idx(*seed.ball()) : idx(*seed.site());
    }
    ++r.meb_steps;
    u32 ball = kNone;
    std::optional<num::Sphere> sphere;  // seulement hors table : census et traces
    if constexpr (NewMeb) {
      mebcert::Part part;
      part.k = k;
      for (u32 i = 0; i < k; ++i) part.id[i] = idx(cur[i]);
      const mebcert::NewOut n = mebcert::new_path(r.ctx, part);
      ++r.routes[n.route];
      ball = n.ball;
      if (ball == kNone) {
        auto made = mebcert::sphere_through(r.ctx, n.support, n.arity);
        if (!made.ok()) return made.outcome();
        sphere.emplace(made.value());
      }
    } else {
      auto meb = bounded_meb(cloud, {cur.data(), k});
      if (!meb.ok()) return meb.outcome();
      std::array<u32, 4> key{kNone, kNone, kNone, kNone};
      for (std::size_t i = 0; i < meb.value().support().size(); ++i) key[i] = idx(meb.value().support()[i]);
      ball = r.ctx.table.find(key);
      if (ball == kNone) sphere.emplace(meb.value().sphere());
    }
    // Population de la sphere : catalogue, sinon census de seuil k (v11 locate).
    const u32* inner;
    const u32* shell;
    u32 p, m, q = 0;
    bool complete = true;
    const num::Level* level;
    if (ball != kNone) {
      const auto& rec = r.ctx.cat.rec[ball];
      inner = r.ctx.cat.val + r.ctx.cat.off[ball];
      shell = inner + rec.p;
      p = rec.p;
      m = rec.m;
      q = rec.q;
      level = &cat.levels()[rec.rank];
    } else {
      ++r.census;
      CensusCopy copy{&r.interior, &r.shell, false};
      MHGP11_TRY(ws->query(r.domain.index(), *sphere, k, &copy, CensusCopy::consume));
      complete = copy.complete;
      inner = r.interior.data();
      shell = r.shell.data();
      p = static_cast<u32>(r.interior.size());
      m = static_cast<u32>(r.shell.size());
      level = &sphere->level();
      if (complete) {  // S* global sur la coquille entiere (global_support de la v11), puis table
        std::vector<num::Point> zp(m);
        for (u32 i = 0; i < m; ++i) zp[i] = r.ctx.points[shell[i]];
        std::array<u32, 4> key{kNone, kNone, kNone, kNone};
        const int arity = mebcert::canonical_support(*sphere, shell, zp.data(), m, key);
        if (arity < 2) return fail(Reason::tower_invariant);
        q = static_cast<u32>(arity);
        ball = r.ctx.table.find(key);
        if (ball == kNone && u64{p} + q <= u64{r.kmax} + 1) return fail(Reason::tower_invariant);
      }
    }
    // Pas de descente (DescentBuilder::run de la v11).
    if (p >= k) {
      for (u32 i = 0; i < k; ++i) cur[i] = SiteIdx{inner[i]};
      continue;
    }
    if (!complete || q == 0) return fail(Reason::tower_invariant);
    const u32 t = k - p;
    if (t > m) return fail(Reason::tower_invariant);
    if (t == m) {
      if (ball == kNone) return fail(Reason::tower_invariant);
      return ball;
    }
    std::array<u32, kMaxMebSites> tuple{};
    std::array<SiteIdx, kMaxMebSites> selected{};
    for (u32 i = 0; i < t; ++i) tuple[i] = i;
    bool found = false;
    for (;;) {
      for (u32 i = 0; i < t; ++i) selected[i] = SiteIdx{shell[tuple[i]]};
      bool strict = t < q;
      if (!strict) {
        auto meb = bounded_meb(cloud, {selected.data(), t});
        if (!meb.ok()) return meb.outcome();
        const int side = num::compare(meb.value().sphere().level(), *level);
        if (side > 0) return fail(Reason::tower_invariant);
        strict = side < 0;
      }
      if (strict) {
        found = true;
        u32 a = 0, b = 0, w = 0;
        while (a < p || b < t) {
          if (b == t || (a < p && inner[a] < idx(selected[b]))) cur[w++] = SiteIdx{inner[a++]};
          else cur[w++] = selected[b++];
        }
        break;
      }
      // tuple suivant (ordre lexicographique, comme next_tuple de la v11)
      u32 j = t;
      for (; j != 0; --j) {
        const u32 i = j - 1;
        if (tuple[i] == m - t + i) continue;
        ++tuple[i];
        for (u32 x = i + 1; x < t; ++x) tuple[x] = tuple[x - 1] + 1;
        break;
      }
      if (j == 0) break;
    }
    if (!found) {
      if (ball == kNone) return fail(Reason::tower_invariant);
      return ball;
    }
  }
}

Outcome measure_resolution(const Args& a, const FullDomain& domain, const PopulationLookup& population,
                           const mebcert::Ctx& ctx, MemoryBudget& budget, CensusWorkspace* ws) {
  const auto& cat = domain.catalogue();
  for (u32 k = 2; k <= a.kmax; ++k) {
    std::vector<u8> kinds(cat.balls(), 0);
    ClassifyCounts counts;
    MHGP11_TRY(classify_range(domain, k, std::span<u8>(kinds), 0, cat.balls(), counts));
    // Traces de toutes les cellules, a plat (hors chronometre).
    std::vector<SiteIdx> traces;
    {
      for (u32 b = 0; b < cat.balls(); ++b) {
        if (kinds[b] != 2) continue;
        auto made = build_cell(domain, BallIdx{b}, static_cast<Order>(k), budget);
        if (!made.ok()) return made.outcome();
        for (const auto& tr : made.value().traces()) traces.insert(traces.end(), tr.part().begin(), tr.part().end());
      }
    }
    const u64 nt = traces.size() / k;
    std::vector<u32> seeds_v11(nt), seeds_r11(nt), seeds_r12(nt);
    double best_v11 = 1e300, best_r11 = 1e300, best_r12 = 1e300;
    Replica r11{domain, population, ctx, a.kmax, {}, {}, {}, 0, 0};
    Replica r12{domain, population, ctx, a.kmax, {}, {}, {}, 0, 0};
    r11.interior.reserve(64); r11.shell.reserve(64); r12.interior.reserve(64); r12.shell.reserve(64);
    for (u32 rep = 0; rep < a.chrono; ++rep) {
      auto t0 = Clock::now();
      for (u64 i = 0; i < nt; ++i) {
        auto down = population.descend_each_step({traces.data() + i * k, k}, k, budget, ws, false);
        if (!down.ok()) return down.outcome();
        const auto& seed = down.value().seed();
        seeds_v11[i] = seed.ball() ? idx(*seed.ball()) : idx(*seed.site());
      }
      best_v11 = std::min(best_v11, seconds_since(t0));
      const bool count = rep == 0;
      if (!count) { r11.routes = {}; r11.meb_steps = r11.census = 0; r12.routes = {}; r12.meb_steps = r12.census = 0; }
      t0 = Clock::now();
      for (u64 i = 0; i < nt; ++i) {
        auto seed = resolve_replica<false>(r11, ws, traces.data() + i * k, k);
        if (!seed.ok()) return seed.outcome();
        seeds_r11[i] = seed.value();
      }
      best_r11 = std::min(best_r11, seconds_since(t0));
      t0 = Clock::now();
      for (u64 i = 0; i < nt; ++i) {
        auto seed = resolve_replica<true>(r12, ws, traces.data() + i * k, k);
        if (!seed.ok()) return seed.outcome();
        seeds_r12[i] = seed.value();
      }
      best_r12 = std::min(best_r12, seconds_since(t0));
      if (seeds_v11 != seeds_r11 || seeds_v11 != seeds_r12) return fail(Reason::tower_invariant, static_cast<Order>(k));
    }
    std::cout << "{\"phase\":\"resolution_un_fil\",\"k\":" << k << ",\"traces\":" << nt
              << ",\"pas_plus_petite_boule\":" << r12.meb_steps << ",\"census_replique_v11\":" << r11.census
              << ",\"census_replique_v12\":" << r12.census << ",\"graines_identiques\":true,\"secondes\":{\"v11\":"
              << best_v11 << ",\"replique_v11\":" << best_r11 << ",\"replique_v12\":" << best_r12
              << "},\"rapport_v12_sur_replique_v11\":" << (best_r11 > 0 ? best_r12 / best_r11 : 0.0)
              << ",\"routes_v12\":{";
    for (u32 rt = 0; rt < mebcert::kRouteCount; ++rt)
      std::cout << (rt ? "," : "") << '"' << mebcert::kRouteNames[rt] << "\":" << r12.routes[rt];
    std::cout << "}}\n" << std::flush;
  }
  return {};
}

Outcome run(const Args& a) {
  const auto t_start = Clock::now();
  MemoryBudget budget(a.budget);
  auto input = bench::read_input(a.xyz.c_str(), a.ids.c_str(), budget);
  if (!input.ok()) return input.outcome();
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  input.value() = {};
  auto pool = sched::make_pool({a.threads});
  if (!pool.ok()) return pool.outcome();
  auto index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return index.outcome();
  const CatalogueParams params = catalogue_params(a);
  CatalogueTimings timings;
  auto t0 = Clock::now();
  auto made = prepare_full_domain(std::move(index.value()), params, budget, *pool.value(), &timings);
  if (!made.ok()) return made.outcome();
  const FullDomain& domain = made.value();
  const double domain_s = seconds_since(t0);
  const u32 kmax = a.kmax;
  if (kmax > domain.index().cloud().sites()) return fail(Reason::parameter_out_of_range);
  std::cout << "{\"phase\":\"domaine\",\"trame\":\"" << a.frame << "\",\"K\":" << kmax << ",\"feuille\":" << a.leaf
            << ",\"sites\":" << domain.index().cloud().sites() << ",\"boules\":" << domain.catalogue().balls()
            << ",\"incidences\":" << domain.catalogue().population().size()
            << ",\"niveaux\":" << domain.catalogue().levels().size() << ",\"coord_bits\":" << kCoordBits
            << ",\"seconds\":" << domain_s << "}\n" << std::flush;

  // Forets publiees : meme voie que la sonde (ordres concurrents, masque 802811).
  const FullParams fp = full_params();
  MHGP11_TRY(ForestParallel::validate(fp, pool.value().get()));
  std::array<std::optional<OrderForest>, kMaxMebSites> orders;
  t0 = Clock::now();
  MHGP11_TRY(build_forests(domain, static_cast<Order>(kmax), budget, nullptr, fp, pool.value().get(), OrderLog{},
                           orders));
  std::cout << "{\"phase\":\"forets_publiees\",\"seconds\":" << seconds_since(t0) << "}\n" << std::flush;
  if (!a.ful1.empty()) {
    t0 = Clock::now();
    MHGP11_TRY(write_ful1(a.ful1, domain, orders, kmax));
    std::cout << "{\"phase\":\"ful1\",\"chemin\":\"" << a.ful1 << "\",\"seconds\":" << seconds_since(t0) << "}\n"
              << std::flush;
  }
  pool.value().reset();  // le Pool v11 n'est plus utilise : les taches suivantes ont leurs propres fils

  t0 = Clock::now();
  MHGP11_TRY(dump_catalogue(a, domain));
  std::cout << "{\"phase\":\"catalogue_vide\",\"seconds\":" << seconds_since(t0) << "}\n" << std::flush;

  // Table de populations de la v11 (non liee : voie hit, meme reponse que la voie liee), espaces census par fil.
  auto population = PopulationLookup::make(domain, budget, nullptr);
  if (!population.ok()) return population.outcome();
  std::vector<std::unique_ptr<CensusWorkspace>> workspaces;
  for (u32 w = 0; w < a.threads; ++w) {
    auto ws = CensusWorkspace::make(domain.index(), budget);
    if (!ws.ok()) return ws.outcome();
    workspaces.push_back(std::move(ws.value()));
  }

  // Journaux de graines de la v11, voie serie (ForestBuilder, comme build_order), un ordre par tache.
  std::vector<std::optional<SeedLog>> logs(kmax + 1);
  std::vector<std::optional<OrderForest>> serial(kmax + 1);
  std::vector<u32> wanted;
  for (u32 k = kmax; k >= 1; --k)
    if (a.journal[k]) wanted.push_back(k);  // plus gros ordres d'abord
  t0 = Clock::now();
  {
    std::mutex lock;
    MHGP11_TRY(run_tasks(a.threads, wanted.size(), [&](u64 i, u32) -> Outcome {
      const u32 k = wanted[i];
      auto log = SeedLog::make(domain.catalogue(), k, budget);
      if (!log.ok()) return log.outcome();
      SeedLog journal = std::move(log.value());
      ForestBuilder builder(domain, k, budget, nullptr, nullptr, nullptr, false);
      builder.population = &population.value();
      builder.seed_log = &journal;
      auto forest = builder.run();
      if (!forest.ok()) return forest.outcome();
      journal.close();
      std::lock_guard<std::mutex> guard(lock);
      logs[k].emplace(std::move(journal));
      serial[k].emplace(std::move(forest.value()));
      return {};
    }));
  }
  for (u32 k : wanted)
    if (!same_forest(*serial[k], *orders[k - 1])) return fail(Reason::tower_invariant, static_cast<Order>(k));
  std::cout << "{\"phase\":\"journaux_v11\",\"ordres\":" << wanted.size() << ",\"seconds\":" << seconds_since(t0)
            << ",\"forets_serie_identiques\":true}\n" << std::flush;
  serial.clear();

  for (u32 k = 1; k <= kmax; ++k) {
    const auto tk = Clock::now();
    const OrderForest& forest = *orders[k - 1];
    MHGP11_TRY(dump_forest(a, k, forest));
    // Classification de la v11 (classify_range), cellules = boules de genre 2, dans l'ordre des boules.
    const u32 nballs = domain.catalogue().balls();
    std::vector<u8> kinds(nballs, 0);
    ClassifyCounts counts;
    MHGP11_TRY(classify_range(domain, k, std::span<u8>(kinds), 0, nballs, counts));
    std::vector<u32> cells;
    for (u32 b = 0; b < nballs; ++b)
      if (kinds[b] == 2) cells.push_back(b);
    // Rejeu par blocs de cellules, sorties concatenees dans l'ordre des blocs.
    const u64 width = 2048;
    const u64 nblocks = (cells.size() + width - 1) / width;
    std::vector<BlockOut> blocks(nblocks);
    const ReplayContext context{domain, population.value(), forest, k, kmax, budget};
    MHGP11_TRY(run_tasks(a.threads, nblocks, [&](u64 i, u32 worker) -> Outcome {
      const u64 lo = i * width, hi = std::min<u64>(cells.size(), lo + width);
      return replay_block(context, std::span<const u32>(cells.data() + lo, hi - lo), workspaces[worker].get(),
                          blocks[i]);
    }));
    OrderCounters total;
    for (const auto& b : blocks) total.add(b.counters);
    // Graines rejouees = journal de la v11, cellule par cellule.
    if (a.journal[k]) {
      const SeedLog& log = *logs[k];
      if (log.cells() != cells.size()) return fail(Reason::tower_invariant, static_cast<Order>(k));
      u64 c = 0, s = 0;
      for (const auto& b : blocks) {
        u64 local_seed = 0;
        for (u32 count : b.cell_traces) {
          if (idx(log.ball(static_cast<u32>(c))) != cells[c] || log.end(static_cast<u32>(c)) -
                                                                      log.begin(static_cast<u32>(c)) != count)
            return fail(Reason::tower_invariant, static_cast<Order>(k));
          for (u32 j = 0; j < count; ++j, ++s, ++local_seed)
            if (idx(const_cast<SeedLog&>(log).storage()[s]) != b.seeds[local_seed].node)
              return fail(Reason::tower_invariant, static_cast<Order>(k));
          ++c;
        }
      }
      if (s != log.seeds()) return fail(Reason::tower_invariant, static_cast<Order>(k));
    }
    MHGP11_TRY(dump_order(a, k, domain, forest, kinds, cells, blocks, total.traces, total.parts));
    print_counters(k, total, forest.ledger(), seconds_since(tk));
    std::cout << "{\"phase\":\"graines\",\"k\":" << k << ",\"journal_v11_compare\":"
              << (a.journal[k] ? "true" : "false") << ",\"identiques\":" << (a.journal[k] ? "true" : "null")
              << "}\n" << std::flush;
    logs[k].reset();
  }
  if (a.chrono > 0) {
    const auto& cat = domain.catalogue();
    std::vector<d::BallRec> recs(cat.balls());
    for (u32 b = 0; b < cat.balls(); ++b) {
      const auto& data = cat.balls_data()[b];
      recs[b] = {idx(data.rank), data.p, data.m, data.qmin,
                 {idx(data.support[0]), idx(data.support[1]), idx(data.support[2]), idx(data.support[3])}};
    }
    std::vector<u32> values(cat.population().size());
    std::memcpy(values.data(), cat.population().data(), values.size() * sizeof(u32));
    mebcert::Cat view;
    view.sites = domain.index().cloud().sites();
    view.balls = cat.balls();
    view.rec = recs.data();
    view.off = cat.population_offsets().data();
    view.val = values.data();
    mebcert::SupportTable table;
    table.build(recs.data(), cat.balls());
    const auto& cloud = domain.index().cloud();
    std::vector<num::Point> points(cloud.sites());
    for (u32 s = 0; s < cloud.sites(); ++s) {
      auto point = num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]);
      if (!point.ok()) return point.outcome();
      points[s] = point.value();
    }
    const mebcert::Ctx ctx{cloud, view, table, points};
    const auto t0 = Clock::now();
    MHGP11_TRY(measure_resolution(a, domain, population.value(), ctx, budget, workspaces[0].get()));
    std::cout << "{\"phase\":\"resolution_fin\",\"seconds\":" << seconds_since(t0) << "}\n" << std::flush;
  }
  std::cout << "{\"phase\":\"fin\",\"seconds\":" << seconds_since(t_start) << ",\"pic_budget\":" << budget.peak()
            << "}\n" << std::flush;
  return {};
}

}  // namespace
}  // namespace mhgp12

int main(int argc, char** argv) {
  const auto args = mhgp12::parse_args(argc, argv);
  Outcome result{};
  try {
    result = guarded([&]() { return mhgp12::run(args); });
  } catch (const std::exception& e) {
    std::cout << "{\"phase\":\"exception\",\"message\":\"" << e.what() << "\"}\n";
    return 2;
  }
  std::cout << "{\"phase\":\"exit\",\"status\":\"" << status_name(result.status()) << "\",\"reason\":\""
            << reason_name(result.reason) << "\",\"order\":" << unsigned(result.order) << "}\n";
  return exit_code(result);
}
