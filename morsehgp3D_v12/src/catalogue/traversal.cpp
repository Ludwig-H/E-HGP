// Pilote du parcours en largeur et executeur hote : un noyau de warps = une boucle sur ses warps, repartis sur le Pool
// par tranches fixes ; chaque warp ecrit ses seules sorties (aucun atomique), donc le resultat ne depend ni du nombre de
// fils ni de leur ordonnancement. Port explicite de Driver::run (microbancs/mes_m5_parcours/include/mhgp12/traversal/
// driver.hpp) ; le microbanc gardait toutes les feuilles, le produit les remet au consommateur niveau par niveau.
#include "catalogue/traversal.hpp"

namespace mhgp12::catalogue_detail {
namespace {

// Lancement d'un noyau de `warps` warps : en ligne si le noyau est petit, sinon sur le Pool.
template <class K>
struct Launch {
  const K* kernel;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const K& k = *static_cast<const Launch*>(context)->kernel;
    typename K::Shared shared;
    for (u64 w = begin; w < end; ++w) k(w, shared);
    return {};
  }
};

template <class K>
Outcome launch(sched::Pool& pool, const K& kernel, u64 warps) noexcept {
  Launch<K> context{&kernel};
  if (warps <= 8 || pool.size() == 1) return Launch<K>::body(&context, 0, warps, 0);
  const u64 grain = warps / (8 * u64{pool.size()}) + 1;
  return pool.parallel_for(warps, grain, &context, &Launch<K>::body);
}

// Tableaux du front, gardes d'un niveau a l'autre.
struct Front {
  FrontArray<bfs::Parent> parents[2];
  FrontArray<u32> task_begin[2], list[2];
  FrontArray<u32> chunk_top, keep, reservoir;
  FrontArray<bfs::TaskOut> task_out;
  FrontArray<bfs::ChildOut> child_out;
  FrontArray<bfs::ChildScan> child_scan;
  FrontArray<bfs::TileSum> tile_sum, tile_offset;
  FrontArray<bfs::LevelTotals> totals;
  FrontArray<bfs::Leaf> leaves;
  FrontArray<u32> leaf_sites;
};

// Pseudo-racine : enveloppe [min, max+1) de tous les sites et repere de sa fermeture.
bfs::Parent root_of(const Cloud& cloud) noexcept {
  const auto x = cloud.x(), y = cloud.y(), z = cloud.z();
  u32 lo[3] = {x[0], y[0], z[0]}, hi[3] = {x[0], y[0], z[0]};
  for (u64 i = 1; i < x.size(); ++i) {
    const u32 c[3] = {x[i], y[i], z[i]};
    for (int a = 0; a < 3; ++a) {
      lo[a] = c[a] < lo[a] ? c[a] : lo[a];
      hi[a] = c[a] > hi[a] ? c[a] : hi[a];
    }
  }
  bfs::Parent root{};
  u64 span = 0;
  for (int a = 0; a < 3; ++a) {
    root.lo[a] = lo[a];
    root.hi[a] = static_cast<i64>(hi[a]) + 1;  // 2^32 au bord du domaine u32 : bornes en i64 (CST-0204)
    const u64 w = static_cast<u64>(root.hi[a] - root.lo[a]);
    span = w > span ? w : span;
  }
  root.count = cloud.sites();
  root.frame_bits = bfs::bit_length(span);
  root.tasks = static_cast<u32>((u64{root.count} + bfs::kChunk - 1) / bfs::kChunk);
  root.sides = 1;
  return root;
}

// Tableaux d'un niveau et vue passee aux noyaux.
Result<bfs::Level> level_view(const Cloud& cloud, Front& f, int cur, u64 n_parents, u64 n_tasks, u32 depth,
                              const bfs::Params& params, MemoryBudget& budget) noexcept {
  const u64 n_children = depth == 0 ? 1 : 2 * n_parents;
  const u64 n_tiles = (n_children + bfs::kTile - 1) / bfs::kTile;
  MHGP12_TRY(f.chunk_top.ensure(n_tasks * bfs::kMaxRes, budget));
  MHGP12_TRY(f.keep.ensure(n_tasks * bfs::kChunkWords, budget));
  MHGP12_TRY(f.task_out.ensure(n_tasks, budget));
  MHGP12_TRY(f.reservoir.ensure(n_children * bfs::kMaxRes, budget));
  MHGP12_TRY(f.child_out.ensure(n_children, budget));
  MHGP12_TRY(f.child_scan.ensure(n_children, budget));
  MHGP12_TRY(f.tile_sum.ensure(n_tiles, budget));
  MHGP12_TRY(f.tile_offset.ensure(n_tiles, budget));
  bfs::Level lv{};
  lv.x = cloud.x().data();
  lv.y = cloud.y().data();
  lv.z = cloud.z().data();
  lv.parents = f.parents[cur].data();
  lv.task_begin = f.task_begin[cur].data();
  lv.list = f.list[cur].data();
  lv.n_parents = static_cast<u32>(n_parents);
  lv.n_children = static_cast<u32>(n_children);
  lv.depth = depth;
  lv.n_tasks = n_tasks;
  lv.n_tiles = n_tiles;
  lv.chunk_top = f.chunk_top.data();
  lv.keep = f.keep.data();
  lv.task_out = f.task_out.data();
  lv.reservoir = f.reservoir.data();
  lv.child_out = f.child_out.data();
  lv.child_scan = f.child_scan.data();
  lv.tile_sum = f.tile_sum.data();
  lv.tile_offset = f.tile_offset.data();
  lv.totals = f.totals.data();
  lv.params = params;
  return lv;
}

// Filtre du niveau : sept noyaux puis lecture des totaux.
Result<bfs::LevelTotals> filter_level(sched::Pool& pool, const bfs::Level& lv) noexcept {
  MHGP12_TRY(launch(pool, bfs::SelectKernel{lv}, lv.n_tasks));
  MHGP12_TRY(launch(pool, bfs::MergeKernel{lv}, lv.n_children));
  MHGP12_TRY(launch(pool, bfs::FilterKernel{lv}, lv.n_tasks));
  MHGP12_TRY(launch(pool, bfs::CloseKernel{lv}, lv.n_children));
  MHGP12_TRY(launch(pool, bfs::ScanAKernel{lv}, lv.n_tiles));
  MHGP12_TRY(launch(pool, bfs::ScanBKernel{lv}, 1));
  MHGP12_TRY(launch(pool, bfs::ScanCKernel{lv}, lv.n_tiles));
  return *lv.totals;
}

// Listes et enregistrements du niveau suivant, feuilles du niveau.
Outcome emit_level(sched::Pool& pool, Front& f, int nxt, bfs::Level& lv, const bfs::LevelTotals& t,
                   MemoryBudget& budget) noexcept {
  MHGP12_TRY(f.list[nxt].ensure(t.f[1], budget));
  MHGP12_TRY(f.parents[nxt].ensure(t.f[0], budget));
  MHGP12_TRY(f.task_begin[nxt].ensure(t.f[0], budget));
  MHGP12_TRY(f.leaves.ensure(t.f[3], budget));
  MHGP12_TRY(f.leaf_sites.ensure(t.f[4], budget));
  lv.next_list = f.list[nxt].data();
  lv.next_parents = f.parents[nxt].data();
  lv.next_task_begin = f.task_begin[nxt].data();
  lv.leaves = f.leaves.data();
  lv.leaf_sites = f.leaf_sites.data();
  MHGP12_TRY(launch(pool, bfs::ScatterKernel{lv}, lv.n_tasks));
  return launch(pool, bfs::EmitKernel{lv}, lv.n_children);
}

}  // namespace

Outcome traverse(const Cloud& cloud, const bfs::Params& params, MemoryBudget& budget, sched::Pool& pool,
                 LeafConsumer& consumer, TraversalLedger& ledger, TraversalDiagnostics& diagnostics) noexcept {
  if (cloud.sites() == 0) return fail(Reason::empty_input);
  Front f;
  const bfs::Parent root = root_of(cloud);
  const u64 n = cloud.sites();
  MHGP12_TRY(f.parents[0].ensure(1, budget));
  MHGP12_TRY(f.task_begin[0].ensure(1, budget));
  MHGP12_TRY(f.list[0].ensure(n, budget));
  MHGP12_TRY(f.totals.ensure(1, budget));
  f.parents[0].data()[0] = root;
  f.task_begin[0].data()[0] = 0;
  MHGP12_TRY(launch(pool, bfs::IotaKernel{f.list[0].data(), n}, (n + bfs::kTile - 1) / bfs::kTile));
  TraversalLedger out;
  TraversalDiagnostics diag;
  u64 n_parents = 1, n_tasks = root.tasks;
  const u64 max_depth = 3 * u64{params.coord_bits};
  int cur = 0;
  for (u32 depth = 0;; ++depth) {
    if (depth > max_depth) return fail(Reason::catalogue_invariant);  // potentiel : profondeur <= 3B (CST-0205)
    auto lv = level_view(cloud, f, cur, n_parents, n_tasks, depth, params, budget);
    if (!lv.ok()) return lv.outcome();
    const auto totals = filter_level(pool, lv.value());
    if (!totals.ok()) return totals.outcome();
    const bfs::LevelTotals& t = totals.value();
    out.nodes += lv.value().n_children;
    out.filter_tests += t.tests;
    out.leaves += t.f[3];
    out.max_leaf = t.max_leaf > out.max_leaf ? t.max_leaf : out.max_leaf;
    out.max_depth = depth;
    ++diag.levels;
    diag.tasks += n_tasks;
    diag.candidates += t.candidates;
    if (t.max_leaf > params.max_leaf) return fail(Reason::wide_leaf);  // run_ready de la v11
    MHGP12_TRY(emit_level(pool, f, cur ^ 1, lv.value(), t, budget));
    MHGP12_TRY(consumer.consume({f.leaves.data(), static_cast<std::size_t>(t.f[3])},
                                {f.leaf_sites.data(), static_cast<std::size_t>(t.f[4])}, depth));
    if (t.f[0] == 0) break;
    if (t.f[2] > 0xFFFFFFFFull || t.f[0] > 0x7FFFFFFFull) return fail(Reason::index_overflow_u32);
    n_parents = t.f[0];
    n_tasks = t.f[2];
    cur ^= 1;
  }
  diag.front_peak_bytes = budget.peak();
  ledger = out;
  diagnostics = diag;
  return {};
}

}  // namespace mhgp12::catalogue_detail
