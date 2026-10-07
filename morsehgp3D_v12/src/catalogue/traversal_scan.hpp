// Parcours en largeur : prefixes d'un niveau (ScanA/B/C), ecriture stable des listes (Scatter), enregistrements des
// parents et des feuilles (Emit), liste de la racine (Iota). Port explicite de
// microbancs/mes_m5_parcours/include/mhgp12/traversal/bfs.hpp (MES-M5), sans mutants. Les feuilles d'un niveau sont
// ecrites dans l'arene des feuilles a partir de leaf_base (debuts relatifs a leaf_site_base) : la voie CPU les
// consomme avant le niveau suivant (bases nulles) ; la voie appareil les garde d'un niveau a l'autre jusqu'a remplir
// un lot (feuilles en flux, CONTRAT_CATALOGUE.md, paragraphe 2).
#pragma once

#include "catalogue/traversal_records.hpp"

namespace mhgp12::catalogue_detail::bfs {

MHGP12_HD void child_fields(const ChildOut& o, u64* f, u64& tests, u64& leaf) {
  const bool split = o.kind == kKindSplit, is_leaf = o.kind == kKindLeaf;
  f[0] = split ? 1 : 0;
  f[1] = split ? o.count : 0;
  f[2] = split ? 2 * ((u64{o.count} + kChunk - 1) / kChunk) : 0;
  f[3] = is_leaf ? 1 : 0;
  f[4] = is_leaf ? o.count : 0;
  tests = o.tests;
  leaf = is_leaf ? o.count : 0;
}

// Sommes de la voie l sur ses 32 enfants de la tuile (candidats : taille de la liste du parent).
MHGP12_HD void lane_sums(const Level& lv, u64 tile, u32 l, u64* f, u64& tests, u64& leaf, u64& candidates) {
  for (u32 k = 0; k < kFields; ++k) f[k] = 0;
  tests = 0;
  leaf = 0;
  candidates = 0;
  const u64 first = tile * kTile + u64{l} * 32;
  for (u64 c = first; c < first + 32 && c < lv.n_children; ++c) {
    u64 g[kFields], t = 0, m = 0;
    child_fields(lv.child_out[c], g, t, m);
    for (u32 k = 0; k < kFields; ++k) f[k] += g[k];
    tests += t;
    leaf = m > leaf ? m : leaf;
    candidates += lv.parents[c / 2].count;
  }
}

struct ScanAKernel {
  Level lv;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    Lanes<u64> f[kFields], tests, cand;
    Lanes<u32> leaf;
    MHGP12_LANES(kWarp, l) {
      u64 g[kFields], t = 0, m = 0, n = 0;
      lane_sums(lv, tile, l, g, t, m, n);
      for (u32 k = 0; k < kFields; ++k) f[k][l] = g[k];
      tests[l] = t;
      leaf[l] = static_cast<u32>(m);
      cand[l] = n;
    }
    TileSum s;
    for (u32 k = 0; k < kFields; ++k) s.f[k] = simt::sum64(f[k]);
    s.tests = simt::sum64(tests);
    s.max_leaf = simt::reduce_max(leaf);
    s.candidates = simt::sum64(cand);
    if (simt::leader()) lv.tile_sum[tile] = s;
  }
};

struct ScanBKernel {  // un seul warp
  Level lv;
  struct Shared {};
  MHGP12_HD void operator()(u64, Shared&) const {
    u64 running[kFields] = {0, 0, 0, 0, 0}, tests = 0, leaf = 0, candidates = 0;
    for (u64 base = 0; base < lv.n_tiles; base += kWarp) {
      Lanes<u64> f[kFields], t, n;
      Lanes<u32> m;
      MHGP12_LANES(kWarp, l) {
        const bool in = base + l < lv.n_tiles;
        for (u32 k = 0; k < kFields; ++k) f[k][l] = in ? lv.tile_sum[base + l].f[k] : 0;
        t[l] = in ? lv.tile_sum[base + l].tests : 0;
        m[l] = in ? static_cast<u32>(lv.tile_sum[base + l].max_leaf) : 0;
        n[l] = in ? lv.tile_sum[base + l].candidates : 0;
      }
      Lanes<u64> off[kFields];
      u64 total[kFields];
      for (u32 k = 0; k < kFields; ++k) off[k] = simt::exclusive_scan64(f[k], total[k]);
      MHGP12_LANES(kWarp, l) {
        if (base + l < lv.n_tiles) {
          TileSum o;
          for (u32 k = 0; k < kFields; ++k) o.f[k] = running[k] + off[k][l];
          o.tests = 0;
          o.max_leaf = 0;
          o.candidates = 0;
          lv.tile_offset[base + l] = o;
        }
      }
      for (u32 k = 0; k < kFields; ++k) running[k] += total[k];
      tests += simt::sum64(t);
      candidates += simt::sum64(n);
      const u32 mm = simt::reduce_max(m);
      leaf = mm > leaf ? mm : leaf;
    }
    if (simt::leader()) {
      LevelTotals out;
      for (u32 k = 0; k < kFields; ++k) out.f[k] = running[k];
      out.tests = tests;
      out.max_leaf = leaf;
      out.candidates = candidates;
      *lv.totals = out;
    }
  }
};

struct ScanCKernel {
  Level lv;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    Lanes<u64> f[kFields];
    MHGP12_LANES(kWarp, l) {
      u64 g[kFields], t = 0, m = 0, n = 0;
      lane_sums(lv, tile, l, g, t, m, n);
      for (u32 k = 0; k < kFields; ++k) f[k][l] = g[k];
    }
    Lanes<u64> off[kFields];
    for (u32 k = 0; k < kFields; ++k) {
      u64 total = 0;
      off[k] = simt::exclusive_scan64(f[k], total);
    }
    const TileSum base = lv.tile_offset[tile];
    MHGP12_LANES(kWarp, l) {
      u64 at[kFields];
      for (u32 k = 0; k < kFields; ++k) at[k] = base.f[k] + off[k][l];
      const u64 first = tile * kTile + u64{l} * 32;
      for (u64 c = first; c < first + 32 && c < lv.n_children; ++c) {
        u64 g[kFields], t = 0, m = 0;
        child_fields(lv.child_out[c], g, t, m);
        ChildScan s;
        for (u32 k = 0; k < kFields; ++k) {
          s.f[k] = at[k];
          at[k] += g[k];
        }
        lv.child_scan[c] = s;
      }
    }
  }
};

// Ecriture stable des sites retenus d'une tache (rang = sites gardes des voies inferieures du paquet).
struct ScatterKernel {
  Level lv;
  struct Shared {};
  MHGP12_HD void operator()(u64 t, Shared&) const {
    const TaskRef r = locate(lv, t);
    const ChildOut& o = lv.child_out[r.child];
    if (o.kind == kKindEmpty) return;
    const Parent& p = lv.parents[r.parent];
    const ChildScan& s = lv.child_scan[r.child];
    u32* out = (o.kind == kKindSplit ? lv.next_list + s.f[1] : lv.leaf_sites + lv.leaf_site_base + s.f[4]) +
               lv.task_out[t].out_offset;
    const u32* keep = lv.keep + t * kChunkWords;
    u32 running = 0;
    for (u64 base = r.begin, g = 0; base < r.end; base += kWarp, ++g) {
      const u32 mask = keep[g];
      MHGP12_LANES(kWarp, l) {
        if ((mask >> l) & 1u) {
          const u32 rank = simt::popc(mask & ((1u << l) - 1u));
          out[running + rank] = lv.list[p.list_begin + base + l];
        }
      }
      running += simt::popc(mask);
    }
  }
};

struct EmitKernel {
  Level lv;
  struct Shared {};
  MHGP12_HD void operator()(u64 c, Shared&) const {
    const ChildOut& o = lv.child_out[c];
    if (o.kind == kKindEmpty || !simt::leader()) return;
    const u32 pi = static_cast<u32>(c / 2), side = static_cast<u32>(c % 2);
    const Parent& p = lv.parents[pi];
    const ChildScan& s = lv.child_scan[c];
    u64 path[2];
    child_path(p, side, lv.depth == 0 ? 0 : lv.depth - 1, path);
    if (o.kind == kKindSplit) {
      Parent q;
      for (int a = 0; a < 3; ++a) {
        q.lo[a] = o.lo[a];
        q.hi[a] = o.hi[a];
      }
      q.path[0] = path[0];
      q.path[1] = path[1];
      q.list_begin = s.f[1];
      q.count = o.count;
      q.frame_bits = o.frame_bits;
      q.tasks = (o.count + kChunk - 1) / kChunk;
      q.sides = 2;
      lv.next_parents[s.f[0]] = q;
      lv.next_task_begin[s.f[0]] = static_cast<u32>(s.f[2]);
    } else {
      Leaf f;
      f.begin = lv.leaf_site_base + s.f[4];
      f.m = o.count;
      f.depth = lv.depth;
      for (int a = 0; a < 3; ++a) {
        f.lo[a] = o.lo[a];
        f.hi[a] = o.hi[a];
      }
      f.path[0] = path[0];
      f.path[1] = path[1];
      lv.leaves[lv.leaf_base + s.f[3]] = f;
    }
  }
};

// Liste de la racine : 0, 1, ..., n-1 (une tuile de kTile rangs par warp).
struct IotaKernel {
  u32* list;
  u64 n;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    MHGP12_LANES(kWarp, l) {
      const u64 first = tile * kTile + u64{l} * 32;
      for (u64 i = first; i < first + 32 && i < n; ++i) list[i] = static_cast<u32>(i);
    }
  }
};

}  // namespace mhgp12::catalogue_detail::bfs
