// Pilote du parcours en largeur, ecrit une fois pour deux executeurs. Port explicite de Driver<B> de
// microbancs/mes_m5_parcours/include/mhgp12/traversal/driver.hpp (MES-M5) : l'executeur B fournit des tableaux a
// capacite croissante (B::Array<T>, ensure, ensure_keep, put), le lancement d'un noyau de warps (launch) et la lecture
// des totaux d'un niveau (read_totals) ; le pilote ne contient aucun code d'appareil. Executeurs : hote (traversal.cpp,
// warps simules repartis sur le Pool : voie CPU de reference) et appareil (device_cuda.cu, voie appareil de la tranche
// T1-b). Memes noyaux, meme ordre de lancement, memes refus que le pilote de la voie CPU qu'il remplace.
//
// Un niveau : Select, Merge, Filter, Close, ScanA, ScanB, ScanC, lecture des totaux, refus (feuille plus large que
// max_leaf : wide_leaf), Scatter, Emit, puis le crochet H recoit les feuilles du niveau (feuilles en flux) ; enfin le
// refus des indices de taches ou d'enfants au-dela de 32 bits (index_overflow_u32). Profondeur au-dela de 3B :
// catalogue_invariant, impossible par le potentiel (CST-0205). Le crochet place les feuilles dans l'arene (leaf_base,
// leaf_site_base) : bases nulles pour la voie CPU, lot accumule d'un niveau a l'autre pour la voie appareil.
#pragma once

#include <span>

#include "catalogue/traversal.hpp"

namespace mhgp12::catalogue_detail {

// Tableaux du front d'un executeur ; ceux de la voie appareil sont gardes d'un appel a l'autre (regime resident).
template <class B>
struct Front {
  typename B::template Array<bfs::Parent> parents[2];
  typename B::template Array<u32> task_begin[2], list[2];
  typename B::template Array<u32> chunk_top, keep, reservoir;
  typename B::template Array<bfs::TaskOut> task_out;
  typename B::template Array<bfs::ChildOut> child_out;
  typename B::template Array<bfs::ChildScan> child_scan;
  typename B::template Array<bfs::TileSum> tile_sum, tile_offset;
  typename B::template Array<bfs::LevelTotals> totals;
  typename B::template Array<bfs::Leaf> leaves;
  typename B::template Array<u32> leaf_sites;
};

// Pseudo-racine : enveloppe [min, max+1) de tous les sites et repere de sa fermeture (calcul hote).
inline bfs::Parent traversal_root(std::span<const u32> x, std::span<const u32> y, std::span<const u32> z) noexcept {
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
  root.count = static_cast<u32>(x.size());
  root.frame_bits = bfs::bit_length(span);
  root.tasks = static_cast<u32>((u64{root.count} + bfs::kChunk - 1) / bfs::kChunk);
  root.sides = 1;
  return root;
}

// Entree du parcours : coordonnees dans la memoire de l'executeur, n >= 1 sites, racine calculee sur l'hote.
struct TraversalInput {
  const u32 *x, *y, *z;
  u64 n;
  bfs::Parent root;
};

// Tableaux d'un niveau et vue passee aux noyaux.
template <class B>
Result<bfs::Level> traversal_level(B& b, Front<B>& f, const TraversalInput& in, int cur, u64 n_parents, u64 n_tasks,
                                   u32 depth, const bfs::Params& params) noexcept {
  const u64 n_children = depth == 0 ? 1 : 2 * n_parents;
  const u64 n_tiles = (n_children + bfs::kTile - 1) / bfs::kTile;
  MHGP12_TRY(b.ensure(f.chunk_top, n_tasks * bfs::kMaxRes));
  MHGP12_TRY(b.ensure(f.keep, n_tasks * bfs::kChunkWords));
  MHGP12_TRY(b.ensure(f.task_out, n_tasks));
  MHGP12_TRY(b.ensure(f.reservoir, n_children * bfs::kMaxRes));
  MHGP12_TRY(b.ensure(f.child_out, n_children));
  MHGP12_TRY(b.ensure(f.child_scan, n_children));
  MHGP12_TRY(b.ensure(f.tile_sum, n_tiles));
  MHGP12_TRY(b.ensure(f.tile_offset, n_tiles));
  bfs::Level lv{};
  lv.x = in.x;
  lv.y = in.y;
  lv.z = in.z;
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
template <class B>
Result<bfs::LevelTotals> traversal_filter(B& b, Front<B>& f, const bfs::Level& lv) noexcept {
  MHGP12_TRY(b.launch(bfs::SelectKernel{lv}, lv.n_tasks));
  MHGP12_TRY(b.launch(bfs::MergeKernel{lv}, lv.n_children));
  MHGP12_TRY(b.launch(bfs::FilterKernel{lv}, lv.n_tasks));
  MHGP12_TRY(b.launch(bfs::CloseKernel{lv}, lv.n_children));
  MHGP12_TRY(b.launch(bfs::ScanAKernel{lv}, lv.n_tiles));
  MHGP12_TRY(b.launch(bfs::ScanBKernel{lv}, 1));
  MHGP12_TRY(b.launch(bfs::ScanCKernel{lv}, lv.n_tiles));
  return b.read_totals(f.totals);
}

// Listes et enregistrements du niveau suivant, feuilles du niveau a partir des bases du crochet.
template <class B, class H>
Outcome traversal_emit(B& b, Front<B>& f, int nxt, bfs::Level& lv, const bfs::LevelTotals& t, H& hook) noexcept {
  const u64 leaf_base = hook.leaf_base(), site_base = hook.leaf_site_base();
  MHGP12_TRY(b.ensure(f.list[nxt], t.f[1]));
  MHGP12_TRY(b.ensure(f.parents[nxt], t.f[0]));
  MHGP12_TRY(b.ensure(f.task_begin[nxt], t.f[0]));
  MHGP12_TRY(b.ensure_keep(f.leaves, leaf_base + t.f[3], leaf_base));
  MHGP12_TRY(b.ensure_keep(f.leaf_sites, site_base + t.f[4], site_base));
  lv.next_list = f.list[nxt].data();
  lv.next_parents = f.parents[nxt].data();
  lv.next_task_begin = f.task_begin[nxt].data();
  lv.leaves = f.leaves.data();
  lv.leaf_sites = f.leaf_sites.data();
  lv.leaf_base = leaf_base;
  lv.leaf_site_base = site_base;
  MHGP12_TRY(b.launch(bfs::ScatterKernel{lv}, lv.n_tasks));
  return b.launch(bfs::EmitKernel{lv}, lv.n_children);
}

// Parcours complet ; ledger et diagnostics publies au succes seulement. Le crochet H recoit after_level(lv, t, depth)
// apres l'emission de chaque niveau ; ses refus arretent le parcours.
template <class B, class H>
Outcome traverse_with(B& b, Front<B>& f, const TraversalInput& in, const bfs::Params& params, H& hook,
                      TraversalLedger& ledger, TraversalDiagnostics& diagnostics) noexcept {
  if (in.n == 0) return fail(Reason::empty_input);
  MHGP12_TRY(b.ensure(f.parents[0], 1));
  MHGP12_TRY(b.ensure(f.task_begin[0], 1));
  MHGP12_TRY(b.ensure(f.list[0], in.n));
  MHGP12_TRY(b.ensure(f.totals, 1));
  MHGP12_TRY(b.put(f.parents[0], in.root, 0));
  MHGP12_TRY(b.put(f.task_begin[0], u32{0}, 0));
  MHGP12_TRY(b.launch(bfs::IotaKernel{f.list[0].data(), in.n}, (in.n + bfs::kTile - 1) / bfs::kTile));
  TraversalLedger out;
  TraversalDiagnostics diag;
  u64 n_parents = 1, n_tasks = in.root.tasks;
  const u64 max_depth = 3 * u64{params.coord_bits};
  int cur = 0;
  for (u32 depth = 0;; ++depth) {
    if (depth > max_depth) return hook.refuse(fail(Reason::catalogue_invariant));  // potentiel : profondeur <= 3B
    auto lv = traversal_level(b, f, in, cur, n_parents, n_tasks, depth, params);
    if (!lv.ok()) return lv.outcome();
    const auto totals = traversal_filter(b, f, lv.value());
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
    if (t.max_leaf > params.max_leaf) return hook.refuse(fail(Reason::wide_leaf));  // run_ready de la v11
    MHGP12_TRY(traversal_emit(b, f, cur ^ 1, lv.value(), t, hook));
    MHGP12_TRY(hook.after_level(lv.value(), t, depth));
    if (t.f[0] == 0) break;
    if (t.f[2] > 0xFFFFFFFFull || t.f[0] > 0x7FFFFFFFull) return hook.refuse(fail(Reason::index_overflow_u32));
    n_parents = t.f[0];
    n_tasks = t.f[2];
    cur ^= 1;
  }
  MHGP12_TRY(hook.finish());
  ledger = out;
  diagnostics = diag;
  return {};
}

}  // namespace mhgp12::catalogue_detail
