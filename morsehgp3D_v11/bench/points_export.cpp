// Export compact FULL -> points (banc) : niveaux exacts, forets des ordres demandes, entrees core et incidences de
// couverture forte par site (P3 de MATHEMATIQUES). La regle de pendaison et l'evaluation vivent en Python
// (bench/points_hierarchy.py) ; ce fichier ne choisit aucun proprietaire.
// Les temoins forts et leur noeud vivant reprennent la sonde d'audit validee
// receipts/full_points_20261003/experiment/points_probe.cpp (portes A/B natives), sans autre regle.
// CLI : points.u32le ids.u32le sortie kmax ordres(ex. 2,3,5,10) workers budgetBytes
// Format MHGP11PH : mots u64 petit-boutistes, decrits dans bench/points_hierarchy.py.
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "whole_input.hpp"
#include "index/access.hpp"
#include "sched/sched.hpp"
#include "tower/forest.hpp"

using namespace mhgp11;
using namespace mhgp11::bench;
using namespace mhgp11::tower_detail;

namespace {
constexpr u64 kMemoCapacity = 4096;
// Mots u64 par numerateur ou denominateur de niveau : ceux du budget du profil (8B+12 et 6B+8 bits). u18/u21 : trois
// mots, format version 1 inchange ; u24 : quatre mots, format version 2 (audit P2 du 4 octobre 2026 : le tetraedre
// regulier u24 a un niveau non reduit de 196/148 bits, que trois mots refusaient a tort).
constexpr u64 kLevelWords = (u64{num::Budget::level_numerator} > u64{num::Budget::level_denominator}
                                 ? u64{num::Budget::level_numerator} : u64{num::Budget::level_denominator}) / 64 + 1;
constexpr u64 kVersion = kLevelWords == 3 ? 1 : 2;
static_assert(kLevelWords == 3 || kLevelWords == 4, "export POINTS : trois ou quatre mots par niveau");
static_assert(3 * u64{kCoordMax} * u64{kCoordMax} < (u64{1} << 50));

struct Neighbor { u64 distance; SiteIdx site; };
struct Stats {
  u64 full_ns = 0, export_ns = 0, sites = 0, levels = 0, balls = 0, hops = 0;
  std::array<u64, kMaxMebSites> nodes{}, incidences{}, extended{};
};

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
  const auto lo = box.lo(), hi = box.hi();
  u64 total = 0;
  for (u32 axis = 0; axis < 3; ++axis) {
    const u32 lower = lo.coordinates()[axis], upper = hi.coordinates()[axis];
    const u64 delta = point[axis] < lower ? lower - point[axis] : point[axis] > upper ? point[axis] - upper : 0;
    total += delta * delta;
  }
  return total;
}

// k plus proches sites (soi compris), ordre (distance, SiteIdx) ; une egalite de borne n'est jamais elaguee.
Outcome nearest(const GlobalIndex& index, u32 site, u32 count, std::span<Neighbor> result) {
  const auto& cloud = index.cloud();
  if (count == 0 || count > kMaxMebSites || count > cloud.sites() || result.size() != count)
    return fail(Reason::parameter_out_of_range);
  const std::array<u32, 3> point{cloud.x()[site], cloud.y()[site], cloud.z()[site]};
  std::array<Neighbor, kMaxMebSites> heap{};
  u32 used = 0;
  const auto nodes = index_detail::Access::nodes(index);
  std::array<u64, kMortonBits + 2> stack{};  // arbre radix : profondeur au plus kMortonBits+1 (docs/INDEX.md)
  u32 pending = 1;
  if (nodes.empty() || index.max_depth() == 0 || index.max_depth() > stack.size()) return fail(Reason::tower_invariant);
  while (pending != 0) {
    const u64 cursor = stack[--pending];
    if (cursor >= nodes.size()) return fail(Reason::tower_invariant);
    const auto& node = nodes[cursor];
    if (used == count && box_distance2(point, node.box) > heap[0].distance) continue;
    if (node.end - node.begin <= index.leaf_size()) {
      for (u32 s = node.begin; s < node.end; ++s) {
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
      continue;
    }
    const u64 left = cursor + 1;
    if (left >= nodes.size()) return fail(Reason::tower_invariant);
    const u64 right = nodes[left].escape;
    if (right >= nodes.size() || right >= node.escape || pending + 2 > stack.size()) return fail(Reason::tower_invariant);
    const u64 lb = box_distance2(point, nodes[left].box), rb = box_distance2(point, nodes[right].box);
    const bool right_first = rb < lb || (rb == lb && site >= nodes[right].begin && site < nodes[right].end);
    stack[pending++] = right_first ? left : right;
    stack[pending++] = right_first ? right : left;
  }
  if (used != count) return fail(Reason::tower_invariant);
  std::sort(heap.begin(), heap.begin() + used, before);
  if (idx(heap[0].site) != site || heap[0].distance != 0) return fail(Reason::tower_invariant);
  std::copy_n(heap.begin(), count, result.begin());
  return {};
}

// Noeud vivant a la coupe fermee du niveau entier d (d_k^2 n'est pas forcement un niveau du catalogue).
Result<NodeIdx> at_distance(const OrderForest& forest, std::span<const num::Level> levels, NodeIdx start, u64 d) {
  auto made = num::Level::make(num::Wide<1>::from_u64(d), num::Wide<1>::from_u64(1));
  if (!made.ok()) return made.outcome();
  const auto nodes = forest.nodes();
  if (idx(start) >= nodes.size() || num::compare(levels[idx(nodes[idx(start)].rank)], made.value()) > 0)
    return fail(Reason::tower_invariant);
  for (;;) {
    const NodeIdx parent = nodes[idx(start)].parent;
    if (parent == NodeIdx{kNone}) return start;
    if (idx(parent) >= nodes.size() || idx(nodes[idx(parent)].rank) <= idx(nodes[idx(start)].rank))
      return fail(Reason::tower_invariant);
    if (num::compare(levels[idx(nodes[idx(parent)].rank)], made.value()) > 0) return start;
    start = parent;
  }
}

bool strong(const CatalogueBall& ball, u32 k) noexcept {
  return u64{ball.p} + ball.qmin <= k && k <= u64{ball.p} + ball.m;
}

struct Context {
  const FullTower& tower;
  std::span<const Neighbor> neighbors;  // n x kmax
  std::span<const NodeIdx> singletons;
  MemoryBudget& budget;
  DescentMemo& memo;
  CensusWorkspace& scratch;
  Stats& stats;
};

Result<NodeIdx> descend(const Context& c, std::span<const SiteIdx> part, u32 k, const num::Level& bound) {
  auto down = resolve_descent(c.tower.domain(), part, k, c.budget, &c.memo, &c.scratch);
  if (!down.ok()) return down.outcome();
  if (num::compare(down.value().initial_level(), bound) > 0) return fail(Reason::tower_invariant);
  const auto birth = c.tower.order(static_cast<Order>(k)).birth_node(down.value().seed());
  if (!birth) return fail(Reason::tower_invariant);
  return *birth;
}

Outcome write_core(std::ostream& out, const Context& c, u32 k) {
  const auto& forest = c.tower.order(static_cast<Order>(k));
  const auto levels = c.tower.domain().catalogue().levels();
  const u32 n = c.tower.domain().index().cloud().sites(), width = c.tower.kmax();
  std::array<SiteIdx, kMaxMebSites> part{};
  for (u32 site = 0; site < n; ++site) {
    const auto row = c.neighbors.subspan(u64{site} * width, width);
    const u64 d = row[k - 1].distance;
    NodeIdx start = c.singletons[site];
    if (k > 1) {
      for (u32 j = 0; j < k; ++j) part[j] = row[j].site;
      auto bound = num::Level::make(num::Wide<1>::from_u64(d), num::Wide<1>::from_u64(1));
      if (!bound.ok()) return bound.outcome();
      auto seed = descend(c, std::span<const SiteIdx>(part.data(), k), k, bound.value());
      if (!seed.ok()) return seed.outcome();
      start = seed.value();
    }
    auto node = at_distance(forest, levels, start, d);
    if (!node.ok()) return node.outcome();
    word(out, idx(node.value())); word(out, d);
  }
  return {};
}

// Noeud vivant au niveau de chaque boule forte : naissance directe si P_b est la k-partie, sinon descente d'une
// k-partie de P_b puis ancetre a la coupe fermee de la boule (continuation sans nouveau noeud comprise).
Outcome ball_nodes(const Context& c, u32 k, std::span<NodeIdx> nodes_of) {
  const auto& catalogue = c.tower.domain().catalogue();
  const auto& forest = c.tower.order(static_cast<Order>(k));
  const auto balls = catalogue.balls_data();
  std::fill(nodes_of.begin(), nodes_of.end(), NodeIdx{kNone});
  for (u32 v = 0; v < forest.births(); ++v) {
    const auto& node = forest.nodes()[v];
    if (node.birth_key >= balls.size() || nodes_of[node.birth_key] != NodeIdx{kNone} ||
        node.rank != balls[node.birth_key].rank) return fail(Reason::tower_invariant);
    nodes_of[node.birth_key] = NodeIdx{v};
  }
  std::array<SiteIdx, kMaxMebSites> part{};
  for (u32 b = 0; b < balls.size(); ++b) {
    const auto& ball = balls[b];
    if (!strong(ball, k)) {
      if (nodes_of[b] != NodeIdx{kNone}) return fail(Reason::tower_invariant);  // une naissance est toujours forte
      continue;
    }
    if (u64{ball.p} + ball.m == k) {
      if (nodes_of[b] == NodeIdx{kNone}) return fail(Reason::tower_invariant);
      continue;
    }
    u32 used = 0;
    for (SiteIdx site : catalogue.interior(BallIdx{b})) { if (used == k) break; part[used++] = site; }
    for (SiteIdx site : catalogue.shell(BallIdx{b})) { if (used == k) break; part[used++] = site; }
    if (used != k) return fail(Reason::tower_invariant);
    auto seed = descend(c, std::span<const SiteIdx>(part.data(), k), k, catalogue.levels()[idx(ball.rank)]);
    if (!seed.ok()) return seed.outcome();
    auto raised = forest.ancestor_closed(seed.value(), ball.rank, c.stats.hops);
    if (!raised.ok()) return raised.outcome();
    nodes_of[b] = raised.value();
    c.stats.extended[k - 1] += 1;
  }
  return {};
}

template <class Run>
Outcome population(const Catalogue& catalogue, u32 b, Run&& run) {
  for (SiteIdx site : catalogue.interior(BallIdx{b})) MHGP11_TRY(run(site));
  for (SiteIdx site : catalogue.shell(BallIdx{b})) MHGP11_TRY(run(site));
  return {};
}

// Incidences (rang << 32 | noeud) groupees par site, triees par (rang, noeud) : deux passes, aucun tableau par paire.
Outcome write_incidences(std::ostream& out, const Context& c, u32 k) {
  const auto& catalogue = c.tower.domain().catalogue();
  const u32 n = c.tower.domain().index().cloud().sites();
  const auto balls = catalogue.balls_data();
  Buffer<u64> offsets;
  MHGP11_TRY(c.budget.admit(u64{n + 1} * sizeof(u64)));
  MHGP11_TRY(offsets.allocate(u64{n} + 1, c.budget));
  std::fill(offsets.span().begin(), offsets.span().end(), u64{0});
  Buffer<NodeIdx> nodes_of;
  if (k == 1) {
    for (u32 s = 0; s < n; ++s) offsets[s + 1] = 1;
  } else {
    MHGP11_TRY(c.budget.admit(u64{balls.size()} * sizeof(NodeIdx)));
    MHGP11_TRY(nodes_of.allocate(balls.size(), c.budget));
    MHGP11_TRY(ball_nodes(c, k, nodes_of.span()));
    for (u32 b = 0; b < balls.size(); ++b)
      if (nodes_of[b] != NodeIdx{kNone})
        MHGP11_TRY(population(catalogue, b, [&](SiteIdx s) -> Outcome { offsets[idx(s) + 1] += 1; return {}; }));
  }
  for (u32 s = 0; s < n; ++s) offsets[s + 1] += offsets[s];
  const u64 total = offsets[n];
  Buffer<u64> packed;
  Buffer<u64> cursor;
  MHGP11_TRY(c.budget.admit(total * sizeof(u64) + u64{n} * sizeof(u64)));
  MHGP11_TRY(packed.allocate(total, c.budget));
  MHGP11_TRY(cursor.allocate(n, c.budget));
  std::copy_n(offsets.span().begin(), n, cursor.span().begin());
  if (k == 1) {
    for (u32 s = 0; s < n; ++s) packed[cursor[s]++] = u64{idx(c.singletons[s])};
  } else {
    for (u32 b = 0; b < balls.size(); ++b) {
      if (nodes_of[b] == NodeIdx{kNone}) continue;
      const u64 value = (u64{idx(balls[b].rank)} << 32) | u64{idx(nodes_of[b])};
      MHGP11_TRY(population(catalogue, b, [&](SiteIdx s) -> Outcome { packed[cursor[idx(s)]++] = value; return {}; }));
    }
  }
  for (u32 s = 0; s < n; ++s) {
    if (cursor[s] != offsets[s + 1] || offsets[s + 1] == offsets[s]) return fail(Reason::tower_invariant);
    std::sort(packed.span().begin() + static_cast<std::ptrdiff_t>(offsets[s]),
              packed.span().begin() + static_cast<std::ptrdiff_t>(offsets[s + 1]));
  }
  c.stats.incidences[k - 1] = total;
  word(out, total);
  for (u32 s = 0; s <= n; ++s) word(out, offsets[s]);
  for (u64 j = 0; j < total; ++j) word(out, packed[j]);
  return {};
}

template <class T>
Outcome fixed(std::ostream& out, const T& value) {
  const auto wide = num::to_wide(value);
  if (wide.neg) return fail(Reason::tower_invariant);
  for (u64 j = kLevelWords; j < wide.words.size(); ++j)
    if (wide.words[j] != 0) return fail(Reason::tower_invariant);
  for (u64 j = 0; j < kLevelWords; ++j) word(out, j < wide.words.size() ? wide.words[j] : 0);
  return {};
}

Outcome write_order(std::ostream& out, const Context& c, u32 k) {
  const auto& forest = c.tower.order(static_cast<Order>(k));
  c.stats.nodes[k - 1] = forest.nodes().size();
  word(out, k); word(out, forest.nodes().size()); word(out, forest.births()); word(out, idx(forest.root()));
  for (const auto& node : forest.nodes()) { word(out, idx(node.parent)); word(out, idx(node.rank)); }
  MHGP11_TRY(write_core(out, c, k));
  return write_incidences(out, c, k);
}

Outcome export_all(const std::string& path, const FullTower& tower, std::span<const u32> orders, MemoryBudget& budget,
                   Stats& stats) {
  const auto& domain = tower.domain();
  const auto& cloud = domain.index().cloud();
  const u32 n = cloud.sites(), kmax = tower.kmax();
  auto scratch = CensusWorkspace::make(domain.index(), budget);
  if (!scratch.ok()) return scratch.outcome();
  auto memo = DescentMemo::make(domain, kMemoCapacity, budget, scratch.value().get());
  if (!memo.ok()) return memo.outcome();
  Buffer<Neighbor> neighbors;
  Buffer<NodeIdx> singletons;
  MHGP11_TRY(budget.admit(u64{n} * kmax * sizeof(Neighbor) + u64{n} * sizeof(NodeIdx)));
  MHGP11_TRY(neighbors.allocate(u64{n} * kmax, budget));
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
    MHGP11_TRY(nearest(domain.index(), site, kmax, neighbors.span().subspan(u64{site} * kmax, kmax)));
  const std::filesystem::path final_path(path), pending_path(path + ".pending");
  std::error_code error;
  if (std::filesystem::exists(final_path, error) || std::filesystem::exists(pending_path, error) || error)
    return fail(Reason::output_conflict);
  Outcome outcome;
  {
    std::ofstream out(pending_path, std::ios::binary | std::ios::trunc);
    if (!out) return fail(Reason::output_unwritable);
    out.write("MHGP11PH", 8);
    word(out, kVersion); word(out, kCoordBits); word(out, kmax); word(out, n);
    word(out, domain.catalogue().levels().size()); word(out, orders.size());
    for (u32 k : orders) word(out, k);
    for (u32 site = 0; site < n; ++site) {
      const auto ids = cloud.points(SiteIdx{site});
      if (cloud.w()[site] != 1 || ids.size() != 1) { outcome = fail(Reason::multiplicity_unsupported); break; }
      word(out, cloud.x()[site]); word(out, cloud.y()[site]); word(out, cloud.z()[site]); word(out, idx(ids[0]));
    }
    for (const auto& level : domain.catalogue().levels()) {
      if (!outcome.ok()) break;
      outcome = fixed(out, level.numerator());
      if (outcome.ok()) outcome = fixed(out, level.denominator());
    }
    const Context context{tower, neighbors.span(), singletons.span(), budget, memo.value(), *scratch.value(), stats};
    for (u32 k : orders) {
      if (!outcome.ok()) break;
      outcome = write_order(out, context, k);
    }
    out.close();
    if (outcome.ok() && !out) outcome = fail(Reason::output_unwritable);
  }
  if (!outcome.ok()) { std::filesystem::remove(pending_path, error); return outcome; }
  std::filesystem::rename(pending_path, final_path, error);
  return error ? fail(Reason::output_unwritable) : Outcome{};
}

bool parse_orders(std::string_view text, u32 kmax, std::vector<u32>& orders) {
  while (!text.empty()) {
    const auto comma = text.find(',');
    u64 k = 0;
    if (!parse(text.substr(0, comma), k) || k == 0 || k > kmax) return false;
    if (!orders.empty() && k <= orders.back()) return false;
    orders.push_back(static_cast<u32>(k));
    text = comma == std::string_view::npos ? std::string_view{} : text.substr(comma + 1);
  }
  return !orders.empty();
}

Outcome run(char** argv, u32 kmax, std::span<const u32> orders, u32 workers, u64 bytes, Stats& stats) {
  MemoryBudget budget(bytes);
  auto input = read_input(argv[1], argv[2], budget);
  if (!input.ok()) return input.outcome();
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                            input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return cloud.outcome();
  if (cloud.value().weight() != cloud.value().sites()) return fail(Reason::multiplicity_unsupported);
  if (kmax > cloud.value().sites()) return fail(Reason::parameter_out_of_range);
  stats.sites = cloud.value().sites();
  input.value() = {};
  auto pool = sched::make_pool({workers});
  if (!pool.ok()) return pool.outcome();
  CatalogueParams params;
  params.kmax = static_cast<int>(kmax); params.leaf_size = 16; params.max_leaf = 256;
  params.cache_center_lines = true; params.indirect_sort = true; params.adaptive_frontier = true;
  params.parallel_assembly = true; params.single_pass = true; params.pair_graph = true;
  FullParams full;
  full.regular_batch_capacity = 4096; full.descent_lanes = 48; full.parallel_verticals = true;
  full.reuse_census_workspace = true; full.dense_birth_lookup = true; full.reuse_regular_verticals = true;
  full.population_lookup = true; full.concurrent_orders = true; full.place_pipeline = true;
  Stopwatch full_clock;
  auto index = build_index(std::move(cloud.value()), {}, budget);
  if (!index.ok()) return index.outcome();
  auto domain = prepare_full_domain(std::move(index.value()), params, budget, *pool.value());
  if (!domain.ok()) return domain.outcome();
  auto tower = build_full(std::move(domain.value()), budget, nullptr, full, pool.value().get());
  stats.full_ns = full_clock.nanoseconds();
  if (!tower.ok()) return tower.outcome();
  stats.levels = tower.value().domain().catalogue().levels().size();
  stats.balls = tower.value().domain().catalogue().balls();
  Stopwatch export_clock;
  const auto outcome = export_all(argv[3], tower.value(), orders, budget, stats);
  stats.export_ns = export_clock.nanoseconds();
  return outcome;
}

void report(const Outcome& outcome, u32 kmax, std::span<const u32> orders, const Stats& stats) {
  std::cout << "{\"phase\":\"points_export\",\"status\":\"" << status_name(outcome.status()) << "\",\"reason\":\""
            << reason_name(outcome.reason) << "\",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << kmax
            << ",\"sites\":" << stats.sites << ",\"levels\":" << stats.levels << ",\"balls\":" << stats.balls
            << ",\"full_ns\":" << stats.full_ns << ",\"export_ns\":" << stats.export_ns
            << ",\"ancestor_hops\":" << stats.hops << ",\"orders\":[";
  for (u64 j = 0; j < orders.size(); ++j) {
    const u32 k = orders[j];
    std::cout << (j ? "," : "") << "{\"k\":" << k << ",\"nodes\":" << stats.nodes[k - 1]
              << ",\"incidences\":" << stats.incidences[k - 1] << ",\"extended\":" << stats.extended[k - 1] << '}';
  }
  std::cout << "]}\n";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 8) return 2;
  u64 kmax = 0, workers = 0, bytes = 0;
  if (!parse(argv[4], kmax) || !parse(argv[6], workers) || !parse(argv[7], bytes)) return 2;
  if (kmax == 0 || kmax > kMaxMebSites || workers == 0 || workers > sched::kMaxWorkers) return 2;
  std::vector<u32> orders;
  if (!parse_orders(argv[5], static_cast<u32>(kmax), orders)) return 2;
  Stats stats;
  const auto outcome = guarded([&]() { return run(argv, static_cast<u32>(kmax), orders, static_cast<u32>(workers),
                                                  bytes, stats); });
  report(outcome, static_cast<u32>(kmax), orders, stats);
  return exit_code(outcome);
}
