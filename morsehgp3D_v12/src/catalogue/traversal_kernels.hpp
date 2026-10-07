// Parcours en largeur : noyaux du filtre d'un niveau (Select, Merge, Filter, Close), source unique. Port explicite de
// microbancs/mes_m5_parcours/include/mhgp12/traversal/bfs.hpp (MES-M5), sans mutants. Tous des warps independants :
// aucun atomique, aucune synchronisation de bloc, resultats independants de l'ordre d'execution des warps.
//   Select (tache)  : sommet local des 3K cles (cle, rang) du morceau : tri bitonique de warp par paquet de 32, fusion
//                     par rangs avec le sommet courant ; paquet saute s'il n'a aucune cle sous le seuil ;
//   Merge  (enfant) : reservoir = sommet des sommets locaux (meme fusion) ; rien si l'enfant a une seule tache ;
//   Filter (tache)  : termes G1 des temoins, test par couple (enfant, candidat) dans l'ordre du reservoir avec arret au
//                     K-ieme, masques de garde par vote, comptes, tests, enveloppe locale ;
//   Close  (enfant) : prefixe des comptes de ses taches (compactage stable), enveloppe, boite ajustee, genre, repere.
#pragma once

#include "catalogue/traversal_records.hpp"

namespace mhgp12::catalogue_detail::bfs {

// ------------------------------------------------------------------------------------ sommet des 3K cles d'un warp
template <class D>
struct TopShared {
  Key<D> top[2][kMaxRes];  // double tampon trie
  Key<D> group[kWarp];
};

// Tri bitonique croissant des 32 cles du warp (une par voie).
template <class D>
MHGP12_HD void bitonic32(Lanes<Key<D>>& item) {
  for (u32 k = 2; k <= kWarp; k <<= 1) {
    for (u32 j = k >> 1; j > 0; j >>= 1) {
      const Lanes<Key<D>> other = simt::shfl_xor(item, j);
      MHGP12_LANES(kWarp, l) {
        const bool up = (l & k) == 0;
        const bool lower = (l & j) == 0;
        const bool take_min = lower == up;
        const bool other_less = key_less<D>(other[l], item[l]);
        if (take_min == other_less) item[l] = other[l];
      }
    }
  }
}

template <class D>
MHGP12_HD u32 lower_bound(const Key<D>* a, u32 n, const Key<D>& k) {
  u32 lo = 0, hi = n;
  while (lo < hi) {
    const u32 mid = (lo + hi) / 2;
    if (key_less<D>(a[mid], k)) lo = mid + 1;
    else hi = mid;
  }
  return lo;
}

// Fusionne le paquet (ng cles valides) dans le sommet trie de taille st (au plus cap) ; cles toutes distinctes (rang).
template <class D>
MHGP12_HD void merge_group(TopShared<D>& sh, u32& cur, u32& st, u32 cap, Lanes<Key<D>>& item, u32 ng) {
  if (st == cap) {  // aucun candidat sous le seuil : rien a faire
    const Key<D> threshold = sh.top[cur][cap - 1];
    Lanes<bool> contender;
    MHGP12_LANES(kWarp, l) { contender[l] = key_less<D>(item[l], threshold); }
    if (simt::ballot(contender) == 0) return;
  }
  bitonic32<D>(item);
  MHGP12_LANES(kWarp, l) { sh.group[l] = item[l]; }
  simt::sync();
  const Key<D>* t = sh.top[cur];
  Key<D>* out = sh.top[cur ^ 1u];
  MHGP12_LANES(kWarp, l) {
    if (l < ng) {
      const u32 at = l + lower_bound<D>(t, st, item[l]);
      if (at < cap) out[at] = item[l];
    }
    for (u32 e = l; e < st; e += kWarp) {
      const u32 at = e + lower_bound<D>(sh.group, ng, t[e]);
      if (at < cap) out[at] = t[e];
    }
  }
  simt::sync();
  st = min_u32(cap, st + ng);
  cur ^= 1u;
}

// Rang d'element -> cle. Source : la liste du parent (rangs consecutifs) ou des sommets locaux (rangs indirects).
template <class D>
MHGP12_HD void select_top(const Level& lv, const Parent& p, const i64* lo, const i64* hi, u64 first, u64 n, u32 cap,
                          TopShared<D>& sh, u32* out) {
  u32 cur = 0, st = 0;
  for (u64 base = 0; base < n; base += kWarp) {
    const u32 ng = static_cast<u32>(min_u64(kWarp, n - base));
    Lanes<Key<D>> item;
    MHGP12_LANES(kWarp, l) {
      if (l < ng) {
        const u32 pos = static_cast<u32>(first + base + l);
        const u32 s = lv.list[p.list_begin + pos];
        item[l].dist = reservoir_key<D>(lv.x[s], lv.y[s], lv.z[s], lo, hi);
        item[l].pos = pos;
        item[l].pad = 0;
      } else {
        item[l] = key_invalid<D>();
      }
    }
    merge_group<D>(sh, cur, st, cap, item, ng);
  }
  MHGP12_LANES(kWarp, l) {
    for (u32 e = l; e < st; e += kWarp) out[e] = sh.top[cur][e].pos;
  }
}

// ------------------------------------------------------------------------------------------------------- noyaux
struct SelectKernel {
  Level lv;
  union Shared {
    TopShared<i64> narrow;
    TopShared<i128> wide;
  };
  MHGP12_HD void operator()(u64 t, Shared& sh) const {
    const TaskRef r = locate(lv, t);
    const Parent& p = lv.parents[r.parent];
    i64 lo[3], hi[3];
    child_box(p, r.side, lo, hi);
    const u32 cap = min_u32(reservoir_capacity(lv.params), static_cast<u32>(r.end - r.begin));
    u32* out = lv.chunk_top + t * kMaxRes;
    if (filter_bits(p, lo, hi) <= kNarrowBits) select_top<i64>(lv, p, lo, hi, r.begin, r.end - r.begin, cap, sh.narrow, out);
    else select_top<i128>(lv, p, lo, hi, r.begin, r.end - r.begin, cap, sh.wide, out);
  }
};

struct MergeKernel {
  Level lv;
  union Shared {
    TopShared<i64> narrow;
    TopShared<i128> wide;
  };
  // Reservoir d'un enfant a plusieurs taches : sommet des sommets locaux, chacun trie (Select) et de min(cap, longueur)
  // cles. Les taches sont prises par paquets de 32, rangees par leur PLUS PETITE cle (entree 0) ; une tache n'est
  // fusionnee que si cette cle est sous le seuil courant (la cap-ieme cle du sommet, des qu'il est plein) : sinon aucune
  // de ses cles ne l'est, ni celles des taches suivantes du paquet, et le seuil ne fait que decroitre. Elagage exact :
  // le sommet final est l'ensemble des cap plus petites cles (toutes distinctes), dans l'ordre.
  template <class D>
  MHGP12_HD void run(const Parent& p, const i64* lo, const i64* hi, u64 first, u32 cap, TopShared<D>& sh,
                     u32* out) const {
    u32 cur = 0, st = 0;
    for (u32 tb = 0; tb < p.tasks; tb += kWarp) {
      Lanes<Key<D>> head;  // plus petite cle de chaque tache du paquet ; pad = rang de la tache dans l'enfant
      MHGP12_LANES(kWarp, l) {
        head[l] = key_invalid<D>();
        if (tb + l < p.tasks) {
          const u32 pos = lv.chunk_top[(first + tb + l) * kMaxRes];
          const u32 s = lv.list[p.list_begin + pos];
          head[l].dist = reservoir_key<D>(lv.x[s], lv.y[s], lv.z[s], lo, hi);
          head[l].pos = pos;
          head[l].pad = tb + l;
        }
      }
      bitonic32<D>(head);
      for (u32 i = 0; i < kWarp; ++i) {
        const Key<D> h = simt::shfl(head, i);
        if (h.pos == 0xFFFFFFFFu) break;  // taches epuisees (cles invalides en queue)
        if (st == cap && !key_less<D>(h, sh.top[cur][cap - 1])) break;
        const u32 task = h.pad;
        const u64 len = min_u64(kChunk, u64{p.count} - u64{task} * kChunk);
        const u32 n = static_cast<u32>(min_u64(cap, len));
        for (u32 base = 0; base < n; base += kWarp) {
          const u32 ng = min_u32(kWarp, n - base);
          Lanes<Key<D>> item;
          MHGP12_LANES(kWarp, l) {
            item[l] = key_invalid<D>();
            if (l < ng) {
              item[l].pos = lv.chunk_top[(first + task) * kMaxRes + base + l];
              const u32 s = lv.list[p.list_begin + item[l].pos];
              item[l].dist = reservoir_key<D>(lv.x[s], lv.y[s], lv.z[s], lo, hi);
              item[l].pad = 0;
            }
          }
          merge_group<D>(sh, cur, st, cap, item, ng);
        }
      }
    }
    MHGP12_LANES(kWarp, l) {
      for (u32 e = l; e < st; e += kWarp) out[e] = sh.top[cur][e].pos;
    }
  }
  MHGP12_HD void operator()(u64 c, Shared& sh) const {
    const u32 pi = static_cast<u32>(c / 2), side = static_cast<u32>(c % 2);
    const Parent& p = lv.parents[pi];
    if (p.tasks == 1) return;  // le sommet du morceau unique est le reservoir
    i64 lo[3], hi[3];
    child_box(p, side, lo, hi);
    const u32 cap = min_u32(reservoir_capacity(lv.params), p.count);
    const u64 first = u64{lv.task_begin[pi]} + u64{side} * p.tasks;
    u32* out = lv.reservoir + c * kMaxRes;
    if (filter_bits(p, lo, hi) <= kNarrowBits) run<i64>(p, lo, hi, first, cap, sh.narrow, out);
    else run<i128>(p, lo, hi, first, cap, sh.wide, out);
  }
};

struct FilterKernel {
  Level lv;
  union Shared {
    Terms<i64> narrow[kMaxRes];
    Terms<i128> wide[kMaxRes];
  };
  // Termes des temoins, puis G1 par candidat, masques de garde, enveloppe locale et sortie de la tache.
  template <class D>
  MHGP12_HD void run(u64 t, const TaskRef& r, const Parent& p, const i64* lo, const i64* hi, Terms<D>* w) const {
    const u32 kmax = lv.params.kmax;
    const u32 witnesses = min_u32(reservoir_capacity(lv.params), p.count);
    const u32* ranks = p.tasks == 1 ? lv.chunk_top + t * kMaxRes : lv.reservoir + u64{r.child} * kMaxRes;
    MHGP12_LANES(kWarp, l) {
      for (u32 e = l; e < witnesses; e += kWarp) {
        const u32 s = lv.list[p.list_begin + ranks[e]];
        w[e] = terms<D>(lv.x[s], lv.y[s], lv.z[s], lo, hi);
      }
    }
    simt::sync();
    Lanes<u32> tests, mn[3], mx[3];
    MHGP12_LANES(kWarp, l) {
      tests[l] = 0;
      for (int a = 0; a < 3; ++a) {
        mn[a][l] = 0xFFFFFFFFu;
        mx[a][l] = 0;
      }
    }
    u32 kept = 0;
    u32* keep = lv.keep + t * kChunkWords;
    for (u64 base = r.begin, g = 0; base < r.end; base += kWarp, ++g) {
      const u32 ng = static_cast<u32>(min_u64(kWarp, r.end - base));
      Lanes<bool> keep_lane;
      MHGP12_LANES(kWarp, l) {
        keep_lane[l] = false;
        if (l < ng) {
          const u32 s = lv.list[p.list_begin + base + l];
          const u32 c[3] = {lv.x[s], lv.y[s], lv.z[s]};
          const Terms<D> x = terms<D>(c[0], c[1], c[2], lo, hi);
          u32 found = 0, i = 0;
          for (; i < witnesses && found < kmax; ++i) found += dominates<D>(x, w[i]) ? 1u : 0u;
          tests[l] += i;
          keep_lane[l] = found < kmax;
          if (keep_lane[l]) {
            for (int a = 0; a < 3; ++a) {
              mn[a][l] = c[a] < mn[a][l] ? c[a] : mn[a][l];
              mx[a][l] = c[a] > mx[a][l] ? c[a] : mx[a][l];
            }
          }
        }
      }
      const u32 mask = simt::ballot(keep_lane);
      if (simt::leader()) keep[g] = mask;
      kept += simt::popc(mask);
    }
    const u32 total_tests = static_cast<u32>(simt::sum(tests));  // <= 256 candidats * 36 temoins
    u32 env_min[3], env_max[3];  // collectives en code uniforme, jamais sous la branche du chef
    for (int a = 0; a < 3; ++a) {
      env_min[a] = simt::reduce_min(mn[a]);
      env_max[a] = simt::reduce_max(mx[a]);
    }
    if (simt::leader()) {
      TaskOut o;
      o.kept = kept;
      o.tests = total_tests;
      o.out_offset = 0;
      o.pad = 0;
      for (int a = 0; a < 3; ++a) {
        o.env_min[a] = env_min[a];
        o.env_max[a] = env_max[a];
      }
      lv.task_out[t] = o;
    }
  }
  MHGP12_HD void operator()(u64 t, Shared& sh) const {
    const TaskRef r = locate(lv, t);
    const Parent& p = lv.parents[r.parent];
    i64 lo[3], hi[3];
    child_box(p, r.side, lo, hi);
    if (filter_bits(p, lo, hi) <= kNarrowBits) run<i64>(t, r, p, lo, hi, sh.narrow);
    else run<i128>(t, r, p, lo, hi, sh.wide);
  }
};

struct CloseKernel {
  Level lv;
  struct Shared {};
  // Compactage stable : rang de sortie de chaque tache = prefixe des comptes de ses predecesseurs.
  MHGP12_HD void offsets(u64 first, u32 tasks, u32& running, Lanes<u64>& tests, Lanes<u32> (&mn)[3],
                         Lanes<u32> (&mx)[3]) const {
    for (u32 base = 0; base < tasks; base += kWarp) {
      Lanes<u32> kept;
      MHGP12_LANES(kWarp, l) {
        kept[l] = 0;
        if (base + l < tasks) {
          const TaskOut& o = lv.task_out[first + base + l];
          kept[l] = o.kept;
          tests[l] += o.tests;
          for (int a = 0; a < 3; ++a) {
            mn[a][l] = o.env_min[a] < mn[a][l] ? o.env_min[a] : mn[a][l];
            mx[a][l] = o.env_max[a] > mx[a][l] ? o.env_max[a] : mx[a][l];
          }
        }
      }
      u32 total = 0;
      const Lanes<u32> offset = simt::exclusive_scan(kept, total);
      MHGP12_LANES(kWarp, l) {
        if (base + l < tasks) lv.task_out[first + base + l].out_offset = running + offset[l];
      }
      running += total;
    }
  }
  MHGP12_HD void operator()(u64 c, Shared&) const {
    const u32 pi = static_cast<u32>(c / 2), side = static_cast<u32>(c % 2);
    const Parent& p = lv.parents[pi];
    i64 lo[3], hi[3];
    child_box(p, side, lo, hi);
    u32 running = 0;
    Lanes<u64> tests;
    Lanes<u32> mn[3], mx[3];
    MHGP12_LANES(kWarp, l) {
      tests[l] = 0;
      for (int a = 0; a < 3; ++a) {
        mn[a][l] = 0xFFFFFFFFu;
        mx[a][l] = 0;
      }
    }
    offsets(u64{lv.task_begin[pi]} + u64{side} * p.tasks, p.tasks, running, tests, mn, mx);
    ChildOut out;
    out.tests = simt::sum64(tests);
    u32 env_min[3], env_max[3];
    for (int a = 0; a < 3; ++a) {
      env_min[a] = simt::reduce_min(mn[a]);
      env_max[a] = simt::reduce_max(mx[a]);
    }
    out.count = running;
    out.kind = kKindEmpty;
    out.frame_bits = 0;
    out.pad = 0;
    bool empty = running == 0;
    for (int a = 0; a < 3; ++a) {
      out.lo[a] = 0;
      out.hi[a] = 0;
      if (!empty) {
        out.lo[a] = max_i64(static_cast<i64>(env_min[a]), lo[a]);
        out.hi[a] = min_i64(static_cast<i64>(env_max[a]) + 1, hi[a]);
        if (out.lo[a] >= out.hi[a]) empty = true;
      }
    }
    if (empty) {
      out.count = 0;
      for (int a = 0; a < 3; ++a) out.lo[a] = out.hi[a] = 0;
    } else {
      const int axis = split_axis(out.lo, out.hi);
      const i64 width = out.hi[axis] - out.lo[axis];
      out.kind = (running <= lv.params.leaf_size || width <= 1) ? kKindLeaf : kKindSplit;
      out.frame_bits = parent_frame_bits(out.hi, env_min, env_max);
    }
    if (simt::leader()) lv.child_out[c] = out;
  }
};

}  // namespace mhgp12::catalogue_detail::bfs
