// Pilote du parcours en largeur (microbanc MES-M5, hors produit) : boucle des niveaux, ecrite une fois pour les deux
// executeurs (hote : warp simule ; appareil : CUDA). L'executeur B fournit des tableaux a capacite croissante, le
// lancement d'un noyau de warps et la lecture des totaux d'un niveau ; le pilote ne contient aucun code d'appareil.
//
// Un niveau : Select, Merge, Filter, Close, ScanA, ScanB, ScanC, lecture des totaux (un aller-retour), Scatter, Emit.
// b.mark(point, profondeur) jalonne le niveau (0 debut, 1 avant la lecture des totaux, 2 apres, 3 fin) pour le profil
// par niveau de l'appareil ; l'executeur hote l'ignore.
// Les tampons sont gardes d'une passe a l'autre (regime residant) : apres la premiere passe, aucune reservation.
// Refus transactionnels (v11) : feuille plus large que max_leaf, profondeur au-dela de 3B ; rien n'est publie alors.
#pragma once

#include <algorithm>
#include <cstring>
#include <vector>

#include "mhgp12/traversal/bfs.hpp"

namespace mhgp12::traversal {

using bfs::i64;
using bfs::u32;
using bfs::u64;

struct Ledger {
  u64 nodes = 0, leaves = 0, filter_tests = 0, max_depth = 0, max_leaf = 0;
  friend bool operator==(const Ledger&, const Ledger&) = default;
};

// Statuts : ceux du format MHGP12TR (v11), plus un refus de capacite propre au microbanc (indices 32 bits).
enum : u64 { kStatusOk = 0, kStatusWideLeaf = 1, kStatusDepth = 2, kStatusCapacity = 3 };

struct LevelStat {
  u32 depth = 0, parents = 0, children = 0;
  u64 tasks = 0, candidates = 0, tests = 0, splits = 0, leaves = 0, next_list = 0;
};

struct RunResult {
  u64 status = kStatusOk;
  Ledger ledger;
  u64 n_leaves = 0, n_leaf_sites = 0;
  u32 levels = 0;
  u64 allocations = 0;  // reservations faites pendant cette passe (0 en regime residant)
  std::vector<LevelStat> stats;
};

struct NoHook {
  template <class L>
  void after_level(const L&) {}
};

template <class B, int M>
struct Driver {
  B& b;
  bfs::Params params;
  typename B::template Array<u32> x, y, z;
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
  u64 n_sites = 0;
  bool keep_stats = true;

  explicit Driver(B& backend, const bfs::Params& p) : b(backend), params(p) {}

  // Nuage en ordre SiteIdx : copie sur l'executeur (premier transfert de la passe).
  void upload_cloud(const u32* hx, const u32* hy, const u32* hz, u64 n, u64& allocations) {
    n_sites = n;
    allocations += b.upload(x, hx, n) + b.upload(y, hy, n) + b.upload(z, hz, n);
  }

  // Une passe complete depuis le nuage deja copie ; hx/hy/hz (hote) servent a l'enveloppe de la racine.
  template <class Hook = NoHook>
  RunResult run(const u32* hx, const u32* hy, const u32* hz, Hook& hook) {
    RunResult res;
    const u64 n = n_sites;
    // Racine : enveloppe [min, max+1) et repere de sa fermeture.
    bfs::Parent root{};
    u32 lo[3] = {hx[0], hy[0], hz[0]}, hi[3] = {hx[0], hy[0], hz[0]};
    for (u64 i = 1; i < n; ++i) {
      const u32 c[3] = {hx[i], hy[i], hz[i]};
      for (int a = 0; a < 3; ++a) {
        lo[a] = std::min(lo[a], c[a]);
        hi[a] = std::max(hi[a], c[a]);
      }
    }
    u64 span = 0;
    for (int a = 0; a < 3; ++a) {
      root.lo[a] = lo[a];
      root.hi[a] = static_cast<i64>(hi[a]) + 1;
      span = std::max<u64>(span, static_cast<u64>(root.hi[a] - root.lo[a]));
    }
    root.path[0] = root.path[1] = 0;
    root.list_begin = 0;
    root.count = static_cast<u32>(n);
    root.frame_bits = bfs::bit_length(span);
    root.tasks = static_cast<u32>(bfs::tasks_of(n));  // n < 2^32 (lecteur) : au plus 2^24 taches
    root.sides = 1;
    const u32 zero = 0;
    int cur = 0;
    res.allocations += b.upload(parents[cur], &root, 1) + b.upload(task_begin[cur], &zero, 1);
    res.allocations += b.ensure(list[cur], n) + b.ensure(totals, 1);
    b.launch(bfs::IotaKernel{list[cur].data(), n}, (n + bfs::kTile - 1) / bfs::kTile);
    u64 n_parents = 1, n_tasks = root.tasks, leaf_count = 0, site_count = 0;
    const u64 max_depth = 3 * u64{params.coord_bits};
    for (u32 depth = 0;; ++depth) {
      if (depth > max_depth) {  // prepare_node de la v11 : depth > kMaxDepth
        res.status = kStatusDepth;
        break;
      }
      const u64 n_children = depth == 0 ? 1 : 2 * n_parents;
      const u64 n_tiles = (n_children + bfs::kTile - 1) / bfs::kTile;
      res.allocations += b.ensure(chunk_top, n_tasks * bfs::kMaxRes) + b.ensure(keep, n_tasks * bfs::kChunkWords) +
                         b.ensure(task_out, n_tasks) + b.ensure(reservoir, n_children * bfs::kMaxRes) +
                         b.ensure(child_out, n_children) + b.ensure(child_scan, n_children) +
                         b.ensure(tile_sum, n_tiles) + b.ensure(tile_offset, n_tiles);
      bfs::Level lv{};
      lv.x = x.data();
      lv.y = y.data();
      lv.z = z.data();
      lv.parents = parents[cur].data();
      lv.task_begin = task_begin[cur].data();
      lv.list = list[cur].data();
      lv.n_parents = static_cast<u32>(n_parents);
      lv.n_children = static_cast<u32>(n_children);
      lv.depth = depth;
      lv.n_tasks = n_tasks;
      lv.n_tiles = n_tiles;
      lv.chunk_top = chunk_top.data();
      lv.keep = keep.data();
      lv.task_out = task_out.data();
      lv.reservoir = reservoir.data();
      lv.child_out = child_out.data();
      lv.child_scan = child_scan.data();
      lv.tile_sum = tile_sum.data();
      lv.tile_offset = tile_offset.data();
      lv.totals = totals.data();
      lv.params = params;
      b.mark(0, depth);
      b.launch(bfs::SelectKernel<M>{lv}, n_tasks);
      b.launch(bfs::MergeKernel<M>{lv}, n_children);
      b.launch(bfs::FilterKernel<M>{lv}, n_tasks);
      b.launch(bfs::CloseKernel<M>{lv}, n_children);
      b.launch(bfs::ScanAKernel{lv}, n_tiles);
      b.launch(bfs::ScanBKernel{lv}, 1);
      b.launch(bfs::ScanCKernel{lv}, n_tiles);
      b.mark(1, depth);
      const bfs::LevelTotals t = b.read_totals(totals);
      b.mark(2, depth);
      res.ledger.nodes += n_children;
      res.ledger.filter_tests += t.tests;
      res.ledger.leaves += t.f[3];
      res.ledger.max_leaf = std::max(res.ledger.max_leaf, t.max_leaf);
      res.ledger.max_depth = depth;
      res.levels = depth + 1;
      if (keep_stats) {
        LevelStat s;
        s.depth = depth;
        s.parents = static_cast<u32>(n_parents);
        s.children = static_cast<u32>(n_children);
        s.tasks = n_tasks;
        s.candidates = t.candidates;
        s.tests = t.tests;
        s.splits = t.f[0];
        s.leaves = t.f[3];
        s.next_list = t.f[1];
        res.stats.push_back(s);
      }
      if (t.max_leaf > params.max_leaf) {  // run_ready de la v11 : wide_leaf
        res.status = kStatusWideLeaf;
        break;
      }
      // Admission du niveau suivant AVANT toute reservation, Scatter, Emit ou conversion vers u32 (observation de
      // l'auditeur jointe a CST-0222) : indices de taches et d'enfants sur 32 bits.
      if (t.f[0] != 0 && (t.f[2] > 0xFFFFFFFFull || t.f[0] > 0x7FFFFFFFull)) {
        res.status = kStatusCapacity;
        break;
      }
      const int nxt = cur ^ 1;
      res.allocations += b.ensure(list[nxt], t.f[1]) + b.ensure(parents[nxt], t.f[0]) +
                         b.ensure(task_begin[nxt], t.f[0]) + b.ensure_keep(leaves, leaf_count + t.f[3], leaf_count) +
                         b.ensure_keep(leaf_sites, site_count + t.f[4], site_count);
      lv.next_list = list[nxt].data();
      lv.next_parents = parents[nxt].data();
      lv.next_task_begin = task_begin[nxt].data();
      lv.leaves = leaves.data();
      lv.leaf_sites = leaf_sites.data();
      lv.leaf_base = leaf_count;
      lv.leaf_site_base = site_count;
      b.launch(bfs::ScatterKernel<M>{lv}, n_tasks);
      b.launch(bfs::EmitKernel<M>{lv}, n_children);
      b.mark(3, depth);
      hook.after_level(lv);
      leaf_count += t.f[3];
      site_count += t.f[4];
      if (t.f[0] == 0) break;
      n_parents = t.f[0];
      n_tasks = t.f[2];
      cur = nxt;
    }
    if (res.status != kStatusOk) {  // aucun prefixe publie
      res.ledger = Ledger{};
      leaf_count = site_count = 0;
    }
    res.n_leaves = leaf_count;
    res.n_leaf_sites = site_count;
    return res;
  }
};

}  // namespace mhgp12::traversal
