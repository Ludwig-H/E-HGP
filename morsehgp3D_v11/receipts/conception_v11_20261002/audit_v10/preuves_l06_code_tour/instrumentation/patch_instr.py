#!/usr/bin/env python3
"""Instrumentation L06 d'une COPIE de tower.cpp (sous /tmp) : compteurs de tailles, histogrammes, vidage des entrees du
Kruskal. Aucune decision du calcul n'est modifiee : seuls des compteurs et des ecritures de diagnostic sont ajoutes."""
import sys
p = '/tmp/v11-audit/l06_code_tour/instr/src/tower/tower.cpp'
s = open(p).read()

def rep(old, new, count=1):
    global s
    assert s.count(old) == count, (old[:70], s.count(old))
    s = s.replace(old, new)

# 0. en-tete : etat d'instrumentation
rep('''namespace mhgp10 {

namespace {

using geom::P3;''', '''#include <cstdio>
#include <cstdlib>
#include <mutex>

namespace mhgp10 {

namespace {

// ---- instrumentation L06 (copie d'audit seulement)
struct L06 {
  static constexpr int kH = 24;
  std::atomic<u64> chain[kH] = {};        // histogramme : pas de descente par resolution (jonctions seulement)
  std::atomic<u64> chain_jumps[kH] = {};  // histogramme : sauts K-NN par resolution
  std::atomic<u64> cb_calls{0}, cb_sum{0}, cb_max{0};  // boules fermees par l'arbre : nombre, somme |I|+|U|, max
  std::atomic<u64> cb_hist[40] = {};      // log2(|I|+|U|)
  std::atomic<u64> cb_judge{0}, cb_judge_sum{0};
  std::atomic<u64> jump_calls{0}, jump_p_sum{0}, jump_p_max{0};  // saut K-NN : interieur p
  std::atomic<u64> jump_cat{0};
  std::atomic<u64> lookup2{0}, lookup2_same{0}, lookup2_hit{0};
  std::atomic<u64> miss_in_window{0};  // sphere dans la fenetre (k >= p + q - 1), absente du catalogue, sans refus
  std::atomic<u64> below_window{0};    // sphere sous la fenetre (k < p + q - 1), p < k
  std::atomic<u64> nonstrict{0};       // MEB certifiee non stricte ou repli
  std::atomic<u64> ext_cells{0}, ext_balls{0}, ext_max_m{0}, ext_refused{0};
  std::atomic<u64> kr_lots{0}, kr_lots_multi{0}, kr_merge2{0}, kr_merge3{0}, kr_merge4p{0}, kr_find_steps{0},
      kr_joins_no_union{0}, kr_max_children{0}, kr_max_lot_reps{0};
  std::atomic<u64> anc_calls{0};
  bool on = false;
};
L06 g_l06;
inline void l06_max(std::atomic<u64>& a, u64 v) {
  u64 c = a.load(std::memory_order_relaxed);
  while (v > c && !a.compare_exchange_weak(c, v, std::memory_order_relaxed)) {}
}
inline int l06_log2(u64 v) {
  int b = 0;
  while (v > 1) {
    v >>= 1;
    ++b;
  }
  return b;
}

using geom::P3;''')

# 1. resolve : compteur de pas et de sauts locaux
rep('''  sc.pend.clear();
  ++cnt.resolves;
  Sphere prev;''', '''  sc.pend.clear();
  ++cnt.resolves;
  u32 l06_steps = 0, l06_jumps = 0;
  Sphere prev;''')
rep('''  for (;;) {
    ++cnt.steps;
    if (k == 1) {
      node = F.s[0];
      break;
    }''', '''  for (;;) {
    ++cnt.steps;
    ++l06_steps;
    if (k == 1) {
      node = F.s[0];
      break;
    }''')
# juge 1/32
rep('''          g.tree.closed_ball(a, S.c, sc.I, sc.U);
          ++cnt.closed_balls;
          if (Ic.size() != p''', '''          g.tree.closed_ball(a, S.c, sc.I, sc.U);
          ++cnt.closed_balls;
          if (g_l06.on) {
            g_l06.cb_judge.fetch_add(1, std::memory_order_relaxed);
            g_l06.cb_judge_sum.fetch_add(sc.I.size() + sc.U.size(), std::memory_order_relaxed);
          }
          if (Ic.size() != p''')
rep('''    if (!from_cat) {
      g.tree.closed_ball(a, S.c, sc.I, sc.U);
      ++cnt.closed_balls;
      I = sc.I;
      U = sc.U;
      p = static_cast<u32>(I.size());
      m = static_cast<u32>(U.size());
    }''', '''    if (!from_cat) {
      g.tree.closed_ball(a, S.c, sc.I, sc.U);
      ++cnt.closed_balls;
      I = sc.I;
      U = sc.U;
      p = static_cast<u32>(I.size());
      m = static_cast<u32>(U.size());
      if (g_l06.on) {
        const u64 tot = u64(p) + m;
        g_l06.cb_calls.fetch_add(1, std::memory_order_relaxed);
        g_l06.cb_sum.fetch_add(tot, std::memory_order_relaxed);
        l06_max(g_l06.cb_max, tot);
        g_l06.cb_hist[l06_log2(tot)].fetch_add(1, std::memory_order_relaxed);
        if (!S.strict) g_l06.nonstrict.fetch_add(1, std::memory_order_relaxed);
      }
    }''')
rep('''    if (p >= k) {
      ++cnt.knn_jumps;''', '''    if (p >= k) {
      ++cnt.knn_jumps;
      ++l06_jumps;
      if (g_l06.on) {
        g_l06.jump_calls.fetch_add(1, std::memory_order_relaxed);
        g_l06.jump_p_sum.fetch_add(p, std::memory_order_relaxed);
        l06_max(g_l06.jump_p_max, p);
        if (from_cat) g_l06.jump_cat.fetch_add(1, std::memory_order_relaxed);
      }''')
rep('''      b = g.find_ball(sup, X.atlas.info.data());
      ++cnt.lookups;
      if (b != kNone) bi = &X.atlas.info[b];
    }''', '''      b = g.find_ball(sup, X.atlas.info.data());
      ++cnt.lookups;
      if (b != kNone) bi = &X.atlas.info[b];
      if (g_l06.on) {
        g_l06.lookup2.fetch_add(1, std::memory_order_relaxed);
        if (S.strict && S.nr == m) g_l06.lookup2_same.fetch_add(1, std::memory_order_relaxed);
        if (b != kNone) g_l06.lookup2_hit.fetch_add(1, std::memory_order_relaxed);
        if (b == kNone) {
          if (k + 1 >= p + q) g_l06.miss_in_window.fetch_add(1, std::memory_order_relaxed);
          else g_l06.below_window.fetch_add(1, std::memory_order_relaxed);
        }
      }
    }''')
rep('''  for (u32 c : sc.pend) std::atomic_ref<u32>(X.atlas.val[c]).store(node, std::memory_order_relaxed);
  return node;
}''', '''  for (u32 c : sc.pend) std::atomic_ref<u32>(X.atlas.val[c]).store(node, std::memory_order_relaxed);
  if (g_l06.on) {
    g_l06.chain[std::min<u32>(l06_steps, L06::kH - 1)].fetch_add(1, std::memory_order_relaxed);
    g_l06.chain_jumps[std::min<u32>(l06_jumps, L06::kH - 1)].fetch_add(1, std::memory_order_relaxed);
  }
  return node;
}''')

# 2. Kruskal : lots, arites, pas de find
rep('''  auto find = [&](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  std::vector<u32>& pre = sc.buf;''', '''  u64 l06_find_steps = 0, l06_lots = 0, l06_lots_multi = 0, l06_m2 = 0, l06_m3 = 0, l06_m4 = 0, l06_nounion = 0,
      l06_maxch = 0, l06_maxlot = 0;
  auto find = [&](u32 x) {
    while (dsu[x] != x) {
      x = dsu[x] = dsu[dsu[x]];
      ++l06_find_steps;
    }
    return x;
  };
  std::vector<u32>& pre = sc.buf;''')
rep('''    for (u32 t = i; t < j; ++t) {
      const u32 r0 = o.join_off[t] - o.join_off[i];
      for (u32 r = r0 + 1; r < o.join_off[t + 1] - o.join_off[i]; ++r) {
        const u32 x = find(pre[r0]), y = find(pre[r]);
        if (x != y) dsu[std::max(x, y)] = std::min(x, y);
      }
    }''', '''    ++l06_lots;
    if (j - i >= 2) ++l06_lots_multi;
    l06_maxlot = std::max<u64>(l06_maxlot, o.join_off[j] - o.join_off[i]);
    for (u32 t = i; t < j; ++t) {
      const u32 r0 = o.join_off[t] - o.join_off[i];
      bool any_union = false;
      for (u32 r = r0 + 1; r < o.join_off[t + 1] - o.join_off[i]; ++r) {
        const u32 x = find(pre[r0]), y = find(pre[r]);
        if (x != y) {
          dsu[std::max(x, y)] = std::min(x, y);
          any_union = true;
        }
      }
      if (!any_union) ++l06_nounion;
    }''')
rep('''        top[members[a].first] = node;
        ++out.merges;''', '''        top[members[a].first] = node;
        ++out.merges;
        if (b - a == 2) ++l06_m2;
        else if (b - a == 3) ++l06_m3;
        else ++l06_m4;
        l06_maxch = std::max<u64>(l06_maxch, b - a);''')
rep('''  u32 roots = 0;
  for (u32 v = 0; v < out.rank.size(); ++v) roots += out.parent[v] == kNone;
  if (roots != 1) return fail(Reason::root_count, u8(k));
  return Outcome{};
}''', '''  if (g_l06.on) {
    g_l06.kr_lots += l06_lots;
    g_l06.kr_lots_multi += l06_lots_multi;
    g_l06.kr_merge2 += l06_m2;
    g_l06.kr_merge3 += l06_m3;
    g_l06.kr_merge4p += l06_m4;
    g_l06.kr_find_steps += l06_find_steps;
    g_l06.kr_joins_no_union += l06_nounion;
    l06_max(g_l06.kr_max_children, l06_maxch);
    l06_max(g_l06.kr_max_lot_reps, l06_maxlot);
    static std::mutex mu;
    std::lock_guard<std::mutex> lk(mu);
    std::fprintf(stderr, "L06_KRUSKAL k=%u births=%u joins=%u reps=%u lots=%llu lots_multi=%llu merges2=%llu merges3=%llu "
                         "merges4p=%llu max_children=%llu max_lot_reps=%llu find_steps=%llu joins_no_union=%llu\\n",
                 k, nb, nj, nj ? o.join_off[nj] : 0u, (unsigned long long)l06_lots, (unsigned long long)l06_lots_multi,
                 (unsigned long long)l06_m2, (unsigned long long)l06_m3, (unsigned long long)l06_m4,
                 (unsigned long long)l06_maxch, (unsigned long long)l06_maxlot, (unsigned long long)l06_find_steps,
                 (unsigned long long)l06_nounion);
    if (const char* dir = std::getenv("L06_DUMP_KRUSKAL")) {
      char path[512];
      std::snprintf(path, sizeof path, "%s/kruskal_k%u.bin", dir, k);
      if (FILE* f = std::fopen(path, "wb")) {
        const u32 hdr[4] = {nb, nj, nj ? o.join_off[nj] : 0u, o.site_births};
        std::fwrite(hdr, 4, 4, f);
        std::vector<u32> jr(nj);
        for (u32 x = 0; x < nj; ++x) jr[x] = cat.rank[o.join_ball[x]] + 1;
        std::fwrite(jr.data(), 4, nj, f);
        std::fwrite(o.join_off.data(), 4, u64(nj) + 1, f);
        std::fwrite(o.rep_node.data(), 4, nj ? o.join_off[nj] : 0u, f);
        std::vector<u32> br(nb);
        for (u32 x = 0; x < nb; ++x) br[x] = out.rank[x];
        std::fwrite(br.data(), 4, nb, f);
        std::fclose(f);
      }
    }
  }
  u32 roots = 0;
  for (u32 v = 0; v < out.rank.size(); ++v) roots += out.parent[v] == kNone;
  if (roots != 1) return fail(Reason::root_count, u8(k));
  return Outcome{};
}''')

# 3. build_tower : activation, tailles memoire, coquilles etendues, sortie finale
rep('''  Tower t;
  TowerStats& ts = t.stats;
  auto tstage = Clock::now();
  const u32 n = cloud.sites();''', '''  Tower t;
  TowerStats& ts = t.stats;
  g_l06.on = std::getenv("L06_INSTR") != nullptr;
  auto tstage = Clock::now();
  const u32 n = cloud.sites();''')
rep('''  ext_first[ext_balls.size()] = static_cast<u32>(ext.size());
  ext_local.clear();''', '''  ext_first[ext_balls.size()] = static_cast<u32>(ext.size());
  ext_local.clear();
  if (g_l06.on) {
    g_l06.ext_balls = ext_balls.size();
    g_l06.ext_cells = ext.size();
    u64 mm = 0, refused = 0;
    for (u32 b : ext_balls) mm = std::max<u64>(mm, cat.shell(b).size());
    for (const ExtCell& e : ext) refused += e.kind == Local::refused;
    g_l06.ext_max_m = mm;
    g_l06.ext_refused = refused;
  }''')
rep('''  ts.t_resolve = seconds_since(tstage);
  tstage = Clock::now();''', '''  ts.t_resolve = seconds_since(tstage);
  if (g_l06.on) {
    u64 bytes_lookup = g.lookup.capacity() * 8, bytes_atlas = u64(nballs) * sizeof(BallInfo) + atlas.cells * 5,
        bytes_vnode = u64(nballs) * 4, bytes_seeds = 0, bytes_spop = 0, bytes_runs = 0;
    for (u32 k = atlas.kfirst; k <= atlas.klast; ++k) {
      const OrderRun& o = *runs[k];
      bytes_seeds += o.seeds.capacity() * 8;
      bytes_spop += o.spop.size() * 4;
      bytes_runs += (o.birth_ball.size() + o.join_ball.size() + o.join_ext.size() + o.join_off.size() + o.rep_node.size()) * 4;
    }
    std::fprintf(stderr, "L06_MEM_AFTER_G nballs=%u cells=%llu lookup=%llu atlas=%llu ball_vnode=%llu seeds=%llu spop=%llu "
                         "runs=%llu P=%llu total=%llu\\n",
                 nballs, (unsigned long long)atlas.cells, (unsigned long long)bytes_lookup, (unsigned long long)bytes_atlas,
                 (unsigned long long)bytes_vnode, (unsigned long long)bytes_seeds, (unsigned long long)bytes_spop,
                 (unsigned long long)bytes_runs, (unsigned long long)(u64(n) * sizeof(P3)),
                 (unsigned long long)(bytes_lookup + bytes_atlas + bytes_vnode + bytes_seeds + bytes_spop + bytes_runs +
                                      u64(n) * sizeof(P3)));
    // histogrammes des descentes des jonctions (avant attaches et verticales)
    std::fprintf(stderr, "L06_CHAIN_JOIN steps_hist=");
    for (int i = 0; i < L06::kH; ++i) std::fprintf(stderr, "%s%llu", i ? "," : "", (unsigned long long)g_l06.chain[i].load());
    std::fprintf(stderr, " jumps_hist=");
    for (int i = 0; i < L06::kH; ++i)
      std::fprintf(stderr, "%s%llu", i ? "," : "", (unsigned long long)g_l06.chain_jumps[i].load());
    std::fprintf(stderr, "\\n");
  }
  tstage = Clock::now();''')
rep('''  ts.t_vertical = seconds_since(tstage);
  for (const Scratch& sc : scratch) ts.meb_fallbacks += sc.meb_fallbacks;
  return t;
}''', '''  ts.t_vertical = seconds_since(tstage);
  for (const Scratch& sc : scratch) ts.meb_fallbacks += sc.meb_fallbacks;
  if (g_l06.on) {
    u64 forest_bytes = 0, nodes = 0;
    for (const OrderForest& f : t.orders) {
      nodes += f.rank.size();
      forest_bytes += (f.rank.size() + f.parent.size() + f.child_off.size() + f.child_val.size() + f.birth.size() +
                       f.lower.size() + f.point_node.size() + f.point_cat_rank.size() + f.ball_node.size()) * 4 +
                      f.point_level.size() * 8;
    }
    u64 jump_bytes = 0;
    for (u32 k = atlas.kfirst; k <= atlas.klast; ++k) jump_bytes += (runs[k]->jumps.jump.size() + runs[k]->jumps.depth.size()) * 4;
    std::fprintf(stderr, "L06_MEM_FOREST nodes=%llu forest_bytes=%llu jumps_bytes=%llu\\n", (unsigned long long)nodes,
                 (unsigned long long)forest_bytes, (unsigned long long)jump_bytes);
    std::fprintf(stderr, "L06_CB calls=%llu sum=%llu max=%llu judge_calls=%llu judge_sum=%llu nonstrict=%llu hist_log2=",
                 (unsigned long long)g_l06.cb_calls.load(), (unsigned long long)g_l06.cb_sum.load(),
                 (unsigned long long)g_l06.cb_max.load(), (unsigned long long)g_l06.cb_judge.load(),
                 (unsigned long long)g_l06.cb_judge_sum.load(), (unsigned long long)g_l06.nonstrict.load());
    for (int i = 0; i < 24; ++i) std::fprintf(stderr, "%s%llu", i ? "," : "", (unsigned long long)g_l06.cb_hist[i].load());
    std::fprintf(stderr, "\\nL06_JUMP calls=%llu p_sum=%llu p_max=%llu from_cat=%llu\\n", (unsigned long long)g_l06.jump_calls.load(),
                 (unsigned long long)g_l06.jump_p_sum.load(), (unsigned long long)g_l06.jump_p_max.load(),
                 (unsigned long long)g_l06.jump_cat.load());
    std::fprintf(stderr, "L06_LOOKUP2 calls=%llu same_key_as_first=%llu hits=%llu miss_in_window=%llu below_window=%llu\\n",
                 (unsigned long long)g_l06.lookup2.load(), (unsigned long long)g_l06.lookup2_same.load(),
                 (unsigned long long)g_l06.lookup2_hit.load(), (unsigned long long)g_l06.miss_in_window.load(),
                 (unsigned long long)g_l06.below_window.load());
    std::fprintf(stderr, "L06_EXT balls=%llu cells=%llu max_m=%llu refused=%llu\\n", (unsigned long long)g_l06.ext_balls.load(),
                 (unsigned long long)g_l06.ext_cells.load(), (unsigned long long)g_l06.ext_max_m.load(),
                 (unsigned long long)g_l06.ext_refused.load());
    std::fprintf(stderr, "L06_KRUSKAL_TOTAL lots=%llu lots_multi=%llu merges2=%llu merges3=%llu merges4p=%llu max_children=%llu "
                         "max_lot_reps=%llu find_steps=%llu joins_no_union=%llu\\n",
                 (unsigned long long)g_l06.kr_lots.load(), (unsigned long long)g_l06.kr_lots_multi.load(),
                 (unsigned long long)g_l06.kr_merge2.load(), (unsigned long long)g_l06.kr_merge3.load(),
                 (unsigned long long)g_l06.kr_merge4p.load(), (unsigned long long)g_l06.kr_max_children.load(),
                 (unsigned long long)g_l06.kr_max_lot_reps.load(), (unsigned long long)g_l06.kr_find_steps.load(),
                 (unsigned long long)g_l06.kr_joins_no_union.load());
    std::fprintf(stderr, "L06_CHAIN_ALL steps_hist=");
    for (int i = 0; i < L06::kH; ++i) std::fprintf(stderr, "%s%llu", i ? "," : "", (unsigned long long)g_l06.chain[i].load());
    std::fprintf(stderr, " jumps_hist=");
    for (int i = 0; i < L06::kH; ++i)
      std::fprintf(stderr, "%s%llu", i ? "," : "", (unsigned long long)g_l06.chain_jumps[i].load());
    std::fprintf(stderr, "\\n");
  }
  return t;
}''')

# 4. entree cover : ambiguite des ex aequo (premieres boules couvrantes de meme rang dans des composantes differentes)
rep('''          for (u32 x = 0; x < n; ++x) out.point_node[x] = out.ball_node[first[x]];''', '''          for (u32 x = 0; x < n; ++x) out.point_node[x] = out.ball_node[first[x]];
          if (g_l06.on) {
            // par site : nombre de boules couvrantes au rang de la premiere, et composantes distinctes parmi elles
            std::vector<u32> ties(n, 0), other(n, 0);
            for (u32 b = 0; b < cat.balls(); ++b) {
              if (!covering(b)) continue;
              for (u64 q = cat.pop_off[b]; q < cat.pop_off[b + 1]; ++q) {
                const u32 x = cat.pop[q];
                if (cat.rank[b] != cat.rank[first[x]]) continue;
                ++ties[x];
                if (out.ball_node[b] != out.ball_node[first[x]]) ++other[x];
              }
            }
            u64 tied = 0, ambiguous = 0;
            for (u32 x = 0; x < n; ++x) {
              tied += ties[x] >= 2;
              ambiguous += other[x] >= 1;
            }
            std::fprintf(stderr, "L06_COVER_TIES k=%u extra=%u sites=%u tied_first_level=%llu ambiguous_component=%llu\\n", k,
                         extra, n, (unsigned long long)tied, (unsigned long long)ambiguous);
          }''')

open(p, 'w').write(s)

# CLI : variable d'environnement pour ball_nodes (mesure des ex aequo de l'entree cover)
c = '/tmp/v11-audit/l06_code_tour/instr/cli/mhgp10_tower.cpp'
t = open(c).read()
old = '''  tp.entry = entry;
  std::vector<double> cat_s, tow_s;'''
assert t.count(old) == 1
t = t.replace(old, '''  tp.entry = entry;
  if (std::getenv("L06_BALL_NODES")) tp.ball_nodes = true;
  if (const char* e = std::getenv("L06_COVER_EXTRA")) tp.cover_extra = std::atoi(e);
  if (const char* e = std::getenv("L06_KCAT")) cp.kmax = std::atoi(e);
  if (std::getenv("L06_NO_VERTICALS")) tp.verticals = false;
  std::vector<double> cat_s, tow_s;''')
open(c, 'w').write(t)
print('ok')
