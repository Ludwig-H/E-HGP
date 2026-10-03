// Sonde d'audit seulement : FULL natif puis core, premiere couverture conventionnelle et incidences fortes.
// CLI : points.u32le ids.u32le dump kmax workers budgetBytes. Aucun sous-echantillonnage ni poids developpe.
// MHGP11PTS1 : mots u64 LE ; integer(num),integer(den) suivent bench/whole_input.hpp.
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>

#include "whole_input.hpp"
#include "index/access.hpp"
#include "sched/sched.hpp"
#include "tower/forest.hpp"

using namespace mhgp11;
using namespace mhgp11::bench;
using namespace mhgp11::tower_detail;

namespace {
constexpr u32 kFastOptions = 16379;
constexpr u64 kProbeMemoCapacity = 4096;
// Toute distance et toute borne de boite tiennent dans u64 aux trois profils (B24 : strictement <2^50).
static_assert(3 * u64{kCoordMax} * u64{kCoordMax} < (u64{1} << 50));

struct Neighbor { u64 distance; SiteIdx site; };
struct CoreEntry { NodeIdx node; u64 distance; };
struct CoverEntry { NodeIdx node; LevelRank rank; };
struct OrderStats { u64 nodes = 0, strong_records = 0, strong_incidences = 0, extended_resolutions = 0; };
struct Stats {
  u64 read_ns = 0, cloud_ns = 0, pool_ns = 0, full_ns = 0, points_export_ns = 0;
  u64 sites = 0, levels = 0, balls = 0, knn_nodes = 0, knn_points = 0, ancestor_hops = 0;
  u64 peak_reserved_bytes = 0, cloud_peak_reserved_bytes = 0, full_peak_reserved_bytes = 0;
  u64 memo_reserved_bytes = 0, neighbor_reserved_bytes = 0;
  DescentLedger descents;
  std::array<OrderStats, kMaxMebSites> orders{};
};

Outcome add(u64& target, u64 value) noexcept {
  if (value > std::numeric_limits<u64>::max() - target) return fail(Reason::tower_capacity);
  target += value;
  return {};
}

bool before(const Neighbor& a, const Neighbor& b) noexcept {
  return a.distance < b.distance || (a.distance == b.distance && idx(a.site) < idx(b.site));
}

u64 distance2(const std::array<u32, 3>& a, const std::array<u32, 3>& b) noexcept {
  u64 total = 0;
  for (u32 axis = 0; axis < 3; ++axis) {
    const u64 delta = a[axis] >= b[axis] ? a[axis] - b[axis] : b[axis] - a[axis];
    total += delta * delta;
  }
  return total;
}

u64 box_distance2(const std::array<u32, 3>& point, const num::Box& box) noexcept {
  // Garder les Points possedes : coordinates() ne doit pas emprunter un temporaire detruit.
  const auto lo = box.lo(), hi = box.hi();
  u64 total = 0;
  for (u32 axis = 0; axis < 3; ++axis) {
    const u32 lower = lo.coordinates()[axis], upper = hi.coordinates()[axis];
    const u64 delta = point[axis] < lower ? lower - point[axis] :
                      point[axis] > upper ? point[axis] - upper : 0;
    total += delta * delta;
  }
  return total;
}

// Max-heap <=12 : distance puis SiteIdx. Self est inclus ; egalite de borne JAMAIS rejetee.
Outcome nearest(const GlobalIndex& index, u32 site, u32 count, std::span<Neighbor> result, Stats& stats) {
  const auto& cloud = index.cloud();
  if (site >= cloud.sites() || count == 0 || count > kMaxMebSites || count > cloud.sites() || result.size() != count)
    return fail(Reason::parameter_out_of_range);
  const std::array<u32, 3> point{cloud.x()[site], cloud.y()[site], cloud.z()[site]};
  std::array<Neighbor, kMaxMebSites> heap{};
  u32 used = 0;
  const auto nodes = index_detail::Access::nodes(index);
  // Index median publie : profondeur<=ceil(log2(n))+1<=33, n<kNone. Aucune pile liee a n.
  std::array<u64, 33> stack{};
  u32 pending = 1;
  if (nodes.empty() || index.max_depth() == 0 || index.max_depth() > stack.size())
    return fail(Reason::tower_invariant);
  while (pending != 0) {
    const u64 cursor = stack[--pending];
    if (cursor >= nodes.size()) return fail(Reason::tower_invariant);
    const auto& node = nodes[cursor];
    MHGP11_TRY(add(stats.knn_nodes, 1));
    if (used == count && box_distance2(point, node.box) > heap[0].distance) {
      continue;
    } else if (node.end - node.begin <= index.leaf_size()) {
      for (u32 s = node.begin; s < node.end; ++s) {
        MHGP11_TRY(add(stats.knn_points, 1));
        const Neighbor candidate{distance2(point, {cloud.x()[s], cloud.y()[s], cloud.z()[s]}), SiteIdx{s}};
        if (used < count) {
          heap[used++] = candidate;
          std::push_heap(heap.begin(), heap.begin() + used, before);
        } else if (before(candidate, heap[0])) {
          std::pop_heap(heap.begin(), heap.begin() + used, before);
          heap[used - 1] = candidate;
          std::push_heap(heap.begin(), heap.begin() + used, before);
        }
      }
    } else {
      const u64 left = cursor + 1;
      if (left >= nodes.size()) return fail(Reason::tower_invariant);
      const u64 right = nodes[left].escape;
      if (right >= nodes.size() || right >= node.escape || pending + 2 > stack.size())
        return fail(Reason::tower_invariant);
      const u64 lb = box_distance2(point, nodes[left].box), rb = box_distance2(point, nodes[right].box);
      // La borne ordonne seulement les enfants ; aucun contact n'est retire. En tie, branche du site d'abord.
      const bool right_first = rb < lb || (rb == lb && site >= nodes[right].begin && site < nodes[right].end);
      stack[pending++] = right_first ? left : right;
      stack[pending++] = right_first ? right : left;
    }
  }
  if (used != count) return fail(Reason::tower_invariant);
  std::sort(heap.begin(), heap.begin() + used, before);
  if (idx(heap[0].site) != site || heap[0].distance != 0) return fail(Reason::tower_invariant);
  std::copy_n(heap.begin(), count, result.begin());
  return {};
}

// dk^2 n'est pas necessairement un niveau du catalogue : remontee rationnelle exacte, coupe fermee.
Result<NodeIdx> at_distance(const OrderForest& forest, std::span<const num::Level> levels,
                            NodeIdx start, u64 distance, u64& hops) {
  auto made = num::Level::make(num::Wide<1>::from_u64(distance), num::Wide<1>::from_u64(1));
  if (!made.ok()) return made.outcome();
  const auto& level = made.value();
  const auto nodes = forest.nodes();
  if (idx(start) >= nodes.size() || idx(nodes[idx(start)].rank) >= levels.size() ||
      num::compare(levels[idx(nodes[idx(start)].rank)], level) > 0) return fail(Reason::tower_invariant);
  for (;;) {
    const auto& current = nodes[idx(start)];
    const NodeIdx parent = current.parent;
    if (parent == NodeIdx{kNone}) return start;
    if (idx(parent) >= nodes.size() || idx(nodes[idx(parent)].rank) >= levels.size() ||
        idx(nodes[idx(parent)].rank) <= idx(current.rank)) return fail(Reason::tower_invariant);
    if (num::compare(levels[idx(nodes[idx(parent)].rank)], level) > 0) return start;
    MHGP11_TRY(add(hops, 1));
    start = parent;
  }
}

bool strong(const CatalogueBall& ball, u32 k) noexcept {
  return u64{ball.p} + ball.qmin <= k && k <= u64{ball.p} + ball.m;
}

// Une seule population complete I puis U, jamais les seuls supports ni les seules naissances.
template <class Run>
Outcome population(const Catalogue& catalogue, u32 b, Run&& run) {
  for (SiteIdx site : catalogue.interior(BallIdx{b})) MHGP11_TRY(run(site));
  for (SiteIdx site : catalogue.shell(BallIdx{b})) MHGP11_TRY(run(site));
  return {};
}

Outcome core_entries(const FullTower& tower, u32 k, std::span<const Neighbor> neighbors,
                      std::span<const NodeIdx> singletons, std::span<CoreEntry> entries,
                      MemoryBudget& budget, DescentMemo& memo, CensusWorkspace& scratch, Stats& stats) {
  const auto& domain = tower.domain();
  const auto& forest = tower.order(static_cast<Order>(k));
  const u32 n = domain.index().cloud().sites(), width = tower.kmax();
  std::array<SiteIdx, kMaxMebSites> part{};
  for (u32 site = 0; site < n; ++site) {
    const auto row = neighbors.subspan(u64{site} * width, width);
    const u64 distance = row[k - 1].distance;
    NodeIdx start{kNone};
    if (k == 1) {
      start = singletons[site];
    } else {
      for (u32 j = 0; j < k; ++j) part[j] = row[j].site;
      auto down = resolve_descent(domain, std::span<const SiteIdx>(part.data(), k), k, budget, &memo, &scratch);
      if (!down.ok()) return down.outcome();
      MHGP11_TRY(add_descent(stats.descents, down.value().ledger()));
      auto bound = num::Level::make(num::Wide<1>::from_u64(distance), num::Wide<1>::from_u64(1));
      if (!bound.ok()) return bound.outcome();
      if (num::compare(down.value().initial_level(), bound.value()) > 0) return fail(Reason::tower_invariant);
      const auto birth = forest.birth_node(down.value().seed());
      if (!birth) return fail(Reason::tower_invariant);
      start = *birth;
    }
    auto node = at_distance(forest, domain.catalogue().levels(), start, distance, stats.ancestor_hops);
    if (!node.ok()) return node.outcome();
    entries[site] = {node.value(), distance};
  }
  return {};
}

Outcome strong_entries(const FullTower& tower, u32 k, std::span<NodeIdx> ball_nodes,
                        std::span<CoverEntry> first, MemoryBudget& budget, DescentMemo& memo,
                        CensusWorkspace& scratch, Stats& stats) {
  const auto& domain = tower.domain();
  const auto& catalogue = domain.catalogue();
  const auto& forest = tower.order(static_cast<Order>(k));
  const auto balls = catalogue.balls_data();
  std::fill(ball_nodes.begin(), ball_nodes.end(), NodeIdx{kNone});
  for (u32 v = 0; v < forest.births(); ++v) {
    const auto& node = forest.nodes()[v];
    if (node.birth_key >= balls.size() || ball_nodes[node.birth_key] != NodeIdx{kNone} ||
        node.rank != balls[node.birth_key].rank) return fail(Reason::tower_invariant);
    ball_nodes[node.birth_key] = NodeIdx{v};
  }
  std::array<SiteIdx, kMaxMebSites> part{};
  auto& counts = stats.orders[k - 1];
  for (u32 b = 0; b < balls.size(); ++b) {
    const auto& ball = balls[b];
    if (!strong(ball, k)) continue;
    NodeIdx node{kNone};
    if (u64{ball.p} + ball.m == k) {
      // Toute la population est la k-partie : naissance directe, sans MEB/census par boule reguliere.
      node = ball_nodes[b];
      if (node == NodeIdx{kNone}) return fail(Reason::tower_invariant);
    } else {
      u32 used = 0;
      for (SiteIdx site : catalogue.interior(BallIdx{b})) {
        if (used == k) break;
        part[used++] = site;
      }
      for (SiteIdx site : catalogue.shell(BallIdx{b})) {
        if (used == k) break;
        part[used++] = site;
      }
      if (used != k) return fail(Reason::tower_invariant);
      auto down = resolve_descent(domain, std::span<const SiteIdx>(part.data(), k), k, budget, &memo, &scratch);
      if (!down.ok()) return down.outcome();
      MHGP11_TRY(add_descent(stats.descents, down.value().ledger()));
      if (idx(ball.rank) >= catalogue.levels().size() ||
          num::compare(down.value().initial_level(), catalogue.levels()[idx(ball.rank)]) > 0)
        return fail(Reason::tower_invariant);
      const auto birth = forest.birth_node(down.value().seed());
      if (!birth) return fail(Reason::tower_invariant);
      auto raised = forest.ancestor_closed(*birth, ball.rank, stats.ancestor_hops);
      if (!raised.ok()) return raised.outcome();
      node = raised.value();
      MHGP11_TRY(add(counts.extended_resolutions, 1));
    }
    ball_nodes[b] = node;
    MHGP11_TRY(add(counts.strong_records, 1));
    MHGP11_TRY(add(counts.strong_incidences, u64{ball.p} + ball.m));
    // Convention premiere boule forte CANONIQUE. Tous les ties restent disponibles dans les records suivants.
    MHGP11_TRY(population(catalogue, b, [&](SiteIdx site) -> Outcome {
      if (idx(site) >= first.size()) return fail(Reason::tower_invariant);
      if (first[idx(site)].node == NodeIdx{kNone}) first[idx(site)] = {node, ball.rank};
      return {};
    }));
  }
  for (const auto& entry : first)
    if (entry.node == NodeIdx{kNone}) return fail(Reason::tower_invariant);
  return {};
}

Outcome write_order(std::ostream& out, const FullTower& tower, u32 k, std::span<const Neighbor> neighbors,
                     std::span<const NodeIdx> singletons, MemoryBudget& budget, DescentMemo& memo,
                     CensusWorkspace& scratch, Stats& stats) {
  const auto& catalogue = tower.domain().catalogue();
  const auto& forest = tower.order(static_cast<Order>(k));
  const u32 n = tower.domain().index().cloud().sites();
  // Ces tableaux coexistent ; admission commune et stockage prive a cet ordre synchrone.
  const u64 bytes = u64{n} * (sizeof(CoreEntry) + sizeof(CoverEntry)) +
                    (k == 1 ? 0 : u64{catalogue.balls()} * sizeof(NodeIdx));
  MHGP11_TRY(budget.admit(bytes));
  Buffer<CoreEntry> cores;
  Buffer<CoverEntry> first;
  Buffer<NodeIdx> ball_nodes;
  MHGP11_TRY(cores.allocate(n, budget));
  MHGP11_TRY(first.allocate(n, budget));
  MHGP11_TRY(ball_nodes.allocate(k == 1 ? 0 : catalogue.balls(), budget));
  std::fill(first.span().begin(), first.span().end(), CoverEntry{NodeIdx{kNone}, LevelRank{kNone}});
  MHGP11_TRY(core_entries(tower, k, neighbors, singletons, cores.span(), budget, memo, scratch, stats));
  auto& counts = stats.orders[k - 1];
  counts.nodes = forest.nodes().size();
  if (k == 1) {
    counts.strong_records = n;
    counts.strong_incidences = n;
    for (u32 s = 0; s < n; ++s) first[s] = {singletons[s], LevelRank{0}};
  } else {
    MHGP11_TRY(strong_entries(tower, k, ball_nodes.span(), first.span(), budget, memo, scratch, stats));
  }
  word(out, k); word(out, forest.births()); word(out, forest.nodes().size());
  word(out, forest.edges().size()); word(out, idx(forest.root()));
  for (const auto& node : forest.nodes()) {
    word(out, idx(node.parent)); word(out, idx(node.rank)); word(out, node.child_begin); word(out, node.child_count);
  }
  for (NodeIdx child : forest.edges()) word(out, idx(child));
  for (const auto& entry : cores.span()) { word(out, idx(entry.node)); word(out, entry.distance); }
  for (const auto& entry : first.span()) { word(out, idx(entry.node)); word(out, idx(entry.rank)); }
  word(out, counts.strong_records);
  if (k == 1) {
    for (u32 s = 0; s < n; ++s) {
      word(out, idx(singletons[s])); word(out, 0); word(out, 1); word(out, s);
    }
  } else {
    const auto balls = catalogue.balls_data();
    for (u32 b = 0; b < balls.size(); ++b) {
      if (!strong(balls[b], k)) continue;
      word(out, idx(ball_nodes[b])); word(out, idx(balls[b].rank)); word(out, u64{balls[b].p} + balls[b].m);
      MHGP11_TRY(population(catalogue, b, [&](SiteIdx site) -> Outcome { word(out, idx(site)); return {}; }));
    }
  }
  return out ? Outcome{} : fail(Reason::output_unwritable);
}

// Un seul pilote dans un repertoire d'experience prive. Refuse fichiers existants ; aucun artefact final partiel.
class PendingOutput {
 public:
  explicit PendingOutput(const char* path) : final_(path), pending_(final_) { pending_ += ".pending"; }
  PendingOutput(const PendingOutput&) = delete;
  PendingOutput& operator=(const PendingOutput&) = delete;
  ~PendingOutput() {
    if (owned_) { std::error_code error; std::filesystem::remove(pending_, error); }
  }
  Outcome open(std::ofstream& out) {
    std::error_code error;
    const bool final_exists = std::filesystem::exists(final_, error);
    if (error) return fail(Reason::output_unwritable);
    const bool pending_exists = std::filesystem::exists(pending_, error);
    if (error) return fail(Reason::output_unwritable);
    if (final_exists || pending_exists) return fail(Reason::output_conflict);
    owned_ = true;
    out.open(pending_, std::ios::binary | std::ios::trunc);
    return out ? Outcome{} : fail(Reason::output_unwritable);
  }
  Outcome publish() {
    std::error_code error;
    if (std::filesystem::exists(final_, error)) return fail(Reason::output_conflict);
    if (error) return fail(Reason::output_unwritable);
    std::filesystem::rename(pending_, final_, error);
    if (error) return fail(Reason::output_unwritable);
    owned_ = false;
    return {};
  }
 private:
  // Construire les paths avant creation du fichier ; aucune conversion string->path dans le destructeur.
  std::filesystem::path final_, pending_;
  bool owned_ = false;
};

Outcome export_points(const char* path, const FullTower& tower, MemoryBudget& budget, Stats& stats) {
  const auto& domain = tower.domain();
  const auto& cloud = domain.index().cloud();
  const u32 n = cloud.sites(), kmax = tower.kmax();
  // Domaine/tour immobiles jusqu'a destruction des contextes prives : scratch puis memo puis export.
  auto scratch = CensusWorkspace::make(domain.index(), budget);
  if (!scratch.ok()) return scratch.outcome();
  auto memo = DescentMemo::make(domain, kProbeMemoCapacity, budget, scratch.value().get());
  if (!memo.ok()) return memo.outcome();
  stats.memo_reserved_bytes = kProbeMemoCapacity * DescentMemo::slot_bytes();
  Buffer<Neighbor> neighbors;
  Buffer<NodeIdx> singletons;
  MHGP11_TRY(budget.admit(u64{n} * kmax * sizeof(Neighbor) + u64{n} * sizeof(NodeIdx)));
  MHGP11_TRY(neighbors.allocate(u64{n} * kmax, budget));
  stats.neighbor_reserved_bytes = neighbors.size() * sizeof(Neighbor);
  MHGP11_TRY(singletons.allocate(n, budget));
  std::fill(singletons.span().begin(), singletons.span().end(), NodeIdx{kNone});
  const auto& first = tower.order(1);
  if (first.births() != n) return fail(Reason::tower_invariant);
  for (u32 v = 0; v < first.births(); ++v) {
    const auto& node = first.nodes()[v];
    if (node.birth_key >= n || idx(node.rank) != 0 || singletons[node.birth_key] != NodeIdx{kNone})
      return fail(Reason::tower_invariant);
    singletons[node.birth_key] = NodeIdx{v};
  }
  for (u32 site = 0; site < n; ++site)
    MHGP11_TRY(nearest(domain.index(), site, kmax, neighbors.span().subspan(u64{site} * kmax, kmax), stats));
  PendingOutput output(path);
  std::ofstream out;
  MHGP11_TRY(output.open(out));
  out.write("MHGP11PTS1", 10);
  word(out, kCoordBits); word(out, kmax); word(out, n); word(out, domain.catalogue().levels().size());
  for (u32 site = 0; site < n; ++site) {
    const auto ids = cloud.points(SiteIdx{site});
    if (cloud.w()[site] != 1 || ids.size() != 1) return fail(Reason::multiplicity_unsupported);
    word(out, cloud.x()[site]); word(out, cloud.y()[site]); word(out, cloud.z()[site]); word(out, idx(ids[0]));
  }
  for (const auto& level : domain.catalogue().levels()) {
    integer(out, level.numerator()); integer(out, level.denominator());
  }
  for (u32 k = 1; k <= kmax; ++k)
    MHGP11_TRY(write_order(out, tower, k, neighbors.span(), singletons.span(), budget, memo.value(),
                          *scratch.value(), stats));
  out.close();
  if (!out) return fail(Reason::output_unwritable);
  return output.publish();
}

Outcome run(char** argv, u32 kmax, u32 workers, u64 bytes, Stats& stats) {
  MemoryBudget budget(bytes);
  Stopwatch read_clock;
  auto input = read_input(argv[1], argv[2], budget);
  stats.read_ns = read_clock.nanoseconds();
  if (!input.ok()) return input.outcome();
  Stopwatch cloud_clock;
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                            input.value().ids.span(), CoordWidth(), budget);
  stats.cloud_ns = cloud_clock.nanoseconds();
  stats.cloud_peak_reserved_bytes = budget.peak();
  if (!cloud.ok()) return cloud.outcome();
  if (cloud.value().weight() != cloud.value().sites()) return fail(Reason::multiplicity_unsupported);
  if (kmax > cloud.value().sites()) return fail(Reason::parameter_out_of_range);
  stats.sites = cloud.value().sites();
  input.value() = {};
  Stopwatch pool_clock;
  auto pool = sched::make_pool({workers});
  stats.pool_ns = pool_clock.nanoseconds();
  if (!pool.ok()) return pool.outcome();
  CatalogueParams params;
  params.kmax = static_cast<int>(kmax); params.leaf_size = 16; params.max_leaf = 256;
  params.cache_center_lines = true; params.indirect_sort = true; params.adaptive_frontier = true;
  params.parallel_assembly = true; params.single_pass = true; params.pair_graph = true;
  FullParams full_params;
  full_params.regular_batch_capacity = 4096; full_params.descent_lanes = 48;
  full_params.parallel_verticals = true; full_params.reuse_census_workspace = true;
  full_params.dense_birth_lookup = true; full_params.reuse_regular_verticals = true;
  full_params.population_lookup = true; full_params.concurrent_orders = true;
  budget.restart_peak();
  Stopwatch full_clock;
  auto index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return index.outcome();
  auto domain = prepare_full_domain(std::move(index.value()), params, budget, *pool.value());
  if (!domain.ok()) return domain.outcome();
  auto tower = build_full(std::move(domain.value()), budget, nullptr, full_params, pool.value().get());
  stats.full_ns = full_clock.nanoseconds();
  stats.full_peak_reserved_bytes = budget.peak();
  if (!tower.ok()) return tower.outcome();
  stats.levels = tower.value().domain().catalogue().levels().size();
  stats.balls = tower.value().domain().catalogue().balls();
  Stopwatch points_clock;
  const auto outcome = export_points(argv[3], tower.value(), budget, stats);
  stats.points_export_ns = points_clock.nanoseconds();
  stats.peak_reserved_bytes = std::max(stats.cloud_peak_reserved_bytes, budget.peak());
  return outcome;
}

void report(const Outcome& outcome, u32 kmax, u32 workers, u64 bytes, const Stats& stats) {
  std::cout << "{\"phase\":\"points_probe\",\"status\":\"" << status_name(outcome.status())
            << "\",\"reason\":\"" << reason_name(outcome.reason) << "\",\"coord_bits\":" << kCoordBits
            << ",\"kmax\":" << kmax << ",\"workers\":" << workers << ",\"optimizations\":" << kFastOptions
            << ",\"budget_bytes\":" << bytes << ",\"sites\":" << stats.sites
            << ",\"levels\":" << stats.levels << ",\"balls\":" << stats.balls
            << ",\"first_cover_convention\":\"first_strong_ball_canonical\",\"counts\":{\"knn_nodes\":"
            << stats.knn_nodes << ",\"knn_points\":" << stats.knn_points
            << ",\"ancestor_hops\":" << stats.ancestor_hops
            << ",\"descent_steps\":" << stats.descents.steps << ",\"census_calls\":" << stats.descents.census_calls
            << ",\"memo_hits\":" << stats.descents.memo.hits << "},\"timings\":{\"read_ns\":" << stats.read_ns
            << ",\"cloud_ns\":" << stats.cloud_ns << ",\"pool_ns\":" << stats.pool_ns
            << ",\"full_ns\":" << stats.full_ns << ",\"points_export_ns\":" << stats.points_export_ns
            << "},\"memory\":{\"peak_reserved_bytes\":" << stats.peak_reserved_bytes
            << ",\"cloud_peak_reserved_bytes\":" << stats.cloud_peak_reserved_bytes
            << ",\"full_peak_reserved_bytes\":" << stats.full_peak_reserved_bytes
            << ",\"memo_reserved_bytes\":" << stats.memo_reserved_bytes
            << ",\"neighbor_reserved_bytes\":" << stats.neighbor_reserved_bytes << "},\"orders\":[";
  for (u32 k = 1; k <= kmax; ++k) {
    if (k != 1) std::cout << ',';
    const auto& order = stats.orders[k - 1];
    std::cout << "{\"k\":" << k << ",\"nodes\":" << order.nodes << ",\"strong_records\":" << order.strong_records
              << ",\"strong_incidences\":" << order.strong_incidences
              << ",\"extended_resolutions\":" << order.extended_resolutions << '}';
  }
  std::cout << "]}\n";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 7) return 2;
  std::array<u64, 3> options{};
  for (u32 j = 0; j < options.size(); ++j) if (!parse(argv[j + 4], options[j])) return 2;
  if (options[0] == 0 || options[0] > kMaxMebSites || options[1] == 0 || options[1] > sched::kMaxWorkers)
    return 2;
  const u32 kmax = static_cast<u32>(options[0]), workers = static_cast<u32>(options[1]);
  Stats stats;
  const auto outcome = guarded([&]() { return run(argv, kmax, workers, options[2], stats); });
  report(outcome, kmax, workers, options[2], stats);
  return exit_code(outcome);
}
