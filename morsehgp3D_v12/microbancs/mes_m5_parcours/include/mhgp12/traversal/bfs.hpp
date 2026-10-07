// Parcours des boites du catalogue EN LARGEUR, en source unique __host__ __device__ (microbanc MES-M5, hors produit).
//
// Objet : le meme ensemble final de feuilles que le parcours en profondeur de la v11 (boxes.cpp, moteur ac081a06f),
// boites et listes, et le meme grand livre (noeuds, feuilles, tests G1, profondeur et feuille maximales). Les regles
// sont celles de la v11, sans exception :
//   - racine : tous les sites (ordre SiteIdx), boite = enveloppe [min, max+1) ;
//   - noeud (boite B, liste parente L) : reservoir des min(|L|, 3K) sites de L les plus proches du centre de B, cle
//     (somme des (2x - lo - hi)^2, rang dans L), tri croissant ; un site de L est retire s'il a K dominateurs G1 stricts
//     parmi les temoins, examines dans l'ordre du reservoir avec arret au K-ieme (tests = temoins examines) ; liste
//     retenue dans l'ordre de L ; boite ajustee = enveloppe de la liste [min, max+1) inter B ; vide si liste vide ou
//     boite ajustee vide ;
//   - coupe : axe de plus grande largeur de la boite ajustee (le premier a egalite), milieu lo + largeur/2, gauche
//     [lo, milieu), droite [milieu, hi) ; feuille si |liste| <= taille de feuille ou largeur <= 1 ; refus wide_leaf si
//     une feuille depasse max_leaf ; refus si une profondeur depasse 3B (prepare_node de la v11).
//
// Forme en largeur : un niveau = tous les enfants d'une meme profondeur. Chaque enfant lit la liste retenue de son
// parent ; ses candidats sont decoupes en taches de kChunk sites, une tache par warp. Noyaux d'un niveau (tous des
// warps independants, aucun atomique, aucune synchronisation de bloc ; resultats independants de l'ordre des warps) :
//   Select  (tache)   : sommet local des 3K cles du morceau (tri bitonique de warp par paquet de 32, fusion par rangs) ;
//   Merge   (enfant)  : reservoir de l'enfant = sommet des sommets locaux, taches rangees par leur plus petite cle et
//                       elaguees au seuil courant (exact) ; rien a faire si l'enfant n'a qu'une tache ;
//   Filter  (tache)   : test G1 par couple (enfant, candidat) contre les temoins du reservoir, masques de garde,
//                       comptes, tests, enveloppe locale ;
//   Close   (enfant)  : prefixe des comptes des taches (compactage stable), enveloppe par reduction, boite ajustee,
//                       genre, repere de l'enfant ;
//   ScanA/B/C (tuile) : prefixes sur les enfants (parents du niveau suivant, listes, taches, feuilles, sites) ;
//   Scatter (tache)   : ecriture stable des sites retenus, dans la liste du niveau suivant ou dans l'arene des feuilles ;
//   Emit    (enfant)  : enregistrements des parents du niveau suivant et des feuilles.
// Le pilote (driver.hpp) lit les totaux du niveau apres ScanB (un seul aller-retour par niveau).
//
// Arithmetique (contrat numerique de la v12, § 2 et § 3) : toutes les decisions sont entieres. Le filtre d'un enfant
// s'evalue dans le REPERE DU PARENT, E = fermeture(boite ajustee du parent) U liste du parent (NUM-COUVERTURE,
// CST-0112), d'etendue s ; cle du reservoir sur 2s+4 bits, termes G1 sur 2s+3 bits : voie native i64 si s <= 29
// (CST-0208), voie large i128 au-dela (s <= 33 : boites fermees jusqu'a 2^32, CST-0204). Au profil u21, s <= 22 :
// toujours la voie native, comme la v11. Aucune decision en flottant.
//
// Mutants (parametre de modele M, jamais dans le banc chronometre) : temoin perdu du reservoir, repere de l'enfant
// (etendue prise sur la seule boite de l'enfant), compactage instable, bissection decalee d'un, ex aequo du reservoir
// inverses, axe de coupe au dernier maximum.
#pragma once

#include "mhgp12/traversal/warp.hpp"

namespace mhgp12::traversal::bfs {

using warp::i128;
using warp::i64;
using warp::kWarp;
using warp::Lanes;
using warp::u32;
using warp::u128;
using warp::u64;

inline constexpr u32 kMaxOrder = 12;
inline constexpr u32 kMaxRes = 3 * kMaxOrder;  // 36 temoins au plus
inline constexpr u32 kChunk = 256;             // candidats par tache (8 paquets de 32)
inline constexpr u32 kChunkWords = kChunk / 32;
inline constexpr u32 kTile = 1024;  // enfants par tuile des prefixes (32 par voie)
inline constexpr u32 kNarrowBits = 29;
static_assert(2 * kNarrowBits + 4 <= 63, "voie native : cle du reservoir en i64 (CST-0208)");
static_assert(2 * 33 + 4 <= 127, "voie large : cle du reservoir en i128 jusqu'a s = 33 (CST-0204)");
static_assert(kChunk >= kMaxRes && kChunk % 32 == 0, "taches : un morceau complet contient un reservoir entier");

enum Mutant : int {
  kNone = 0,
  kLostWitness = 1,         // reservoir de 3K - 1 temoins
  kFrameChildBox = 2,       // repere pris sur la seule boite de l'enfant (CONTRAT_NUMERIQUE § 7)
  kUnstableCompaction = 3,  // sites retenus d'un paquet ecrits en ordre inverse des voies
  kBisectOffByOne = 4,      // milieu lo + (largeur + 1) / 2
  kTieReversed = 5,         // ex aequo du reservoir departages par rang decroissant
  kAxisLastMax = 6,         // axe de coupe : dernier maximum
};
inline constexpr int kMutantCount = 7;
inline const char* mutant_name(int m) {
  switch (m) {
    case kNone: return "aucun";
    case kLostWitness: return "temoin_perdu";
    case kFrameChildBox: return "repere_enfant";
    case kUnstableCompaction: return "compactage_instable";
    case kBisectOffByOne: return "bissection_decalee";
    case kTieReversed: return "ex_aequo_inverses";
    case kAxisLastMax: return "axe_dernier_maximum";
    default: return "?";
  }
}

enum : u32 { kKindEmpty = 0, kKindLeaf = 1, kKindSplit = 2 };

struct Params {
  u32 kmax = 5, leaf_size = 24, max_leaf = 256, coord_bits = 21;
};

// Noeud coupe d'un niveau : ses deux enfants (un seul, de meme boite, pour la racine) sont traites au niveau suivant.
struct Parent {
  i64 lo[3], hi[3];  // boite ajustee demi-ouverte (racine : enveloppe)
  u64 path[2];       // chemin du noeud (bit de la profondeur d au mot d/64, position 63 - d%64)
  u64 list_begin;    // liste retenue du noeud dans le tampon de listes du niveau
  u32 count;
  u32 frame_bits;  // etendue s du repere E = fermeture(boite ajustee) U liste
  u32 tasks;       // taches par enfant : ceil(count / kChunk)
  u32 sides;       // 2 ; 1 pour la pseudo-racine
};
static_assert(sizeof(Parent) == 88);

struct TaskOut {
  u32 kept, tests, out_offset, pad;
  u32 env_min[3], env_max[3];
};

struct ChildOut {
  i64 lo[3], hi[3];  // boite ajustee (zeros si vide)
  u64 tests;
  u32 count;       // sites retenus (0 si vide)
  u32 kind;        // vide, feuille, coupe
  u32 frame_bits;  // repere de l'enfant pris comme parent
  u32 pad;
};

inline constexpr u32 kFields = 5;  // parents suivants, sites des listes suivantes, taches suivantes, feuilles, sites
struct ChildScan {
  u64 f[kFields];
};
struct TileSum {
  u64 f[kFields];
  u64 tests, max_leaf, candidates;
};
struct LevelTotals {
  u64 f[kFields];
  u64 tests, max_leaf, candidates;
};

struct Leaf {  // disposition de format::Leaf (64 premiers octets : LeafJob de la v11)
  u64 begin;
  u32 m, depth;
  i64 lo[3], hi[3];
  u64 path[2];
};
static_assert(sizeof(Leaf) == 80);

// Vue d'un niveau, passee par valeur a chaque noyau.
struct Level {
  const u32 *x, *y, *z;
  const Parent* parents;
  const u32* task_begin;  // premiere tache de chaque parent (prefixe de sides * tasks)
  const u32* list;        // listes des parents
  u32 n_parents, n_children, depth;
  u64 n_tasks, n_tiles;
  u32* chunk_top;  // n_tasks * kMaxRes rangs dans la liste du parent
  u32* keep;       // n_tasks * kChunkWords
  TaskOut* task_out;
  u32* reservoir;  // n_children * kMaxRes
  ChildOut* child_out;
  ChildScan* child_scan;
  TileSum* tile_sum;
  TileSum* tile_offset;
  LevelTotals* totals;
  u32* next_list;
  Parent* next_parents;
  u32* next_task_begin;
  Leaf* leaves;
  u32* leaf_sites;
  u64 leaf_base, leaf_site_base;
  Params params;
};

// ------------------------------------------------------------------------------------------------- outils scalaires
MHGP12_HD u32 min_u32(u32 a, u32 b) { return a < b ? a : b; }
MHGP12_HD u64 min_u64(u64 a, u64 b) { return a < b ? a : b; }
MHGP12_HD i64 max_i64(i64 a, i64 b) { return a > b ? a : b; }
MHGP12_HD i64 min_i64(i64 a, i64 b) { return a < b ? a : b; }
MHGP12_HD u32 bit_length(u64 v) {
  u32 n = 0;
  while (v != 0) {
    ++n;
    v >>= 1;
  }
  return n;
}

MHGP12_HD u32 reservoir_capacity(const Params& p, int mutant) { return 3 * p.kmax - (mutant == kLostWitness ? 1 : 0); }

// Axe et milieu de coupe d'une boite ajustee (split_ready de la v11).
MHGP12_HD int split_axis(const i64* lo, const i64* hi, int mutant) {
  int axis = 0;
  for (int i = 1; i < 3; ++i) {
    const i64 w = hi[i] - lo[i], wa = hi[axis] - lo[axis];
    if (mutant == kAxisLastMax ? w >= wa : w > wa) axis = i;
  }
  return axis;
}

// Boite de l'enfant side du parent p (la pseudo-racine a un seul enfant, de meme boite).
MHGP12_HD void child_box(const Parent& p, u32 side, i64* lo, i64* hi, int mutant) {
  for (int a = 0; a < 3; ++a) {
    lo[a] = p.lo[a];
    hi[a] = p.hi[a];
  }
  if (p.sides == 1) return;
  const int axis = split_axis(p.lo, p.hi, mutant);
  const i64 width = p.hi[axis] - p.lo[axis];
  const i64 middle = p.lo[axis] + (mutant == kBisectOffByOne ? (width + 1) / 2 : width / 2);
  if (side == 0) hi[axis] = middle;
  else lo[axis] = middle;
}

MHGP12_HD void child_path(const Parent& p, u32 side, u32 parent_depth, u64* path) {
  path[0] = p.path[0];
  path[1] = p.path[1];
  if (p.sides == 2 && side == 1) path[parent_depth / 64] |= u64{1} << (63 - parent_depth % 64);
}

// Etendue du repere E = fermeture(boite ajustee) U liste, l'enveloppe de la liste etant [env_min, env_max].
MHGP12_HD u32 parent_frame_bits(const i64* adj_lo, const i64* adj_hi, const u32* env_min, const u32* env_max) {
  u64 span = 0;
  for (int a = 0; a < 3; ++a) {
    (void)adj_lo;  // adj_lo >= env_min : le coin minimal est celui de la liste
    const i64 top = max_i64(static_cast<i64>(env_max[a]), adj_hi[a]);
    const u64 w = static_cast<u64>(top - static_cast<i64>(env_min[a]));
    span = w > span ? w : span;
  }
  return bit_length(span);
}

// Mutant : etendue de la seule fermeture de la boite de l'enfant.
MHGP12_HD u32 box_frame_bits(const i64* lo, const i64* hi) {
  u64 span = 0;
  for (int a = 0; a < 3; ++a) {
    const u64 w = static_cast<u64>(hi[a] - lo[a]);
    span = w > span ? w : span;
  }
  return bit_length(span);
}

// --------------------------------------------------------------------------------------------- arithmetique exacte
// D = i64 (voie native, s <= 29) ou i128 (voie large). Memes entiers que boxes.cpp (v11).
template <class D>
MHGP12_HD D reservoir_key(u32 x, u32 y, u32 z, const i64* lo, const i64* hi) {
  const D dx = D(2) * D(x) - D(lo[0]) - D(hi[0]);
  const D dy = D(2) * D(y) - D(lo[1]) - D(hi[1]);
  const D dz = D(2) * D(z) - D(lo[2]) - D(hi[2]);
  return dx * dx + dy * dy + dz * dz;
}

template <class D>
struct Terms {
  D scaled[3];
  D square;
};

template <class D>
MHGP12_HD Terms<D> terms(u32 x, u32 y, u32 z, const i64* lo, const i64* hi) {
  Terms<D> t;
  const u32 c[3] = {x, y, z};
  t.square = 0;
  for (int a = 0; a < 3; ++a) {
    const D d = D(c[a]) - D(lo[a]);
    t.scaled[a] = D(2) * D(hi[a] - lo[a]) * d;
    t.square += d * d;
  }
  return t;
}

// G1 : y domine strictement x sur la fermeture de la boite (egalite conservee).
template <class D>
MHGP12_HD bool dominates(const Terms<D>& x, const Terms<D>& y) {
  D right = 0;
  for (int a = 0; a < 3; ++a) {
    const D d = x.scaled[a] - y.scaled[a];
    right += d > 0 ? d : D(0);
  }
  return x.square - y.square > right;
}

template <class D>
MHGP12_HD D key_max() {
  if constexpr (sizeof(D) == 8) return static_cast<D>(0x7FFFFFFFFFFFFFFFll);
  else return static_cast<D>((static_cast<u128>(1) << 127) - 1);
}

template <class D>
struct Key {
  D dist;
  u32 pos;
  u32 pad;
};

template <class D, int M>
MHGP12_HD bool key_less(const Key<D>& a, const Key<D>& b) {
  if (a.dist != b.dist) return a.dist < b.dist;
  if constexpr (M == kTieReversed) {
    // les cles invalides (pos maximal) restent en queue
    if (a.pos == 0xFFFFFFFFu || b.pos == 0xFFFFFFFFu) return a.pos < b.pos;
    return a.pos > b.pos;
  } else {
    return a.pos < b.pos;
  }
}

template <class D>
MHGP12_HD Key<D> key_invalid() {
  Key<D> k;
  k.dist = key_max<D>();
  k.pos = 0xFFFFFFFFu;
  k.pad = 0;
  return k;
}

// ------------------------------------------------------------------------------------ sommet des 3K cles d'un warp
template <class D>
struct TopShared {
  Key<D> top[2][kMaxRes];  // double tampon trie
  Key<D> group[kWarp];
};

// Tri bitonique croissant des 32 cles du warp (une par voie).
template <class D, int M>
MHGP12_HD void bitonic32(Lanes<Key<D>>& item) {
  for (u32 k = 2; k <= kWarp; k <<= 1) {
    for (u32 j = k >> 1; j > 0; j >>= 1) {
      const Lanes<Key<D>> other = warp::shfl_xor(item, j);
      MHGP12_LANES(l) {
        const bool up = (l & k) == 0;
        const bool lower = (l & j) == 0;
        const bool take_min = lower == up;
        const bool other_less = key_less<D, M>(other[l], item[l]);
        if (take_min == other_less) item[l] = other[l];
      }
    }
  }
}

template <class D, int M>
MHGP12_HD u32 lower_bound(const Key<D>* a, u32 n, const Key<D>& k) {
  u32 lo = 0, hi = n;
  while (lo < hi) {
    const u32 mid = (lo + hi) / 2;
    if (key_less<D, M>(a[mid], k)) lo = mid + 1;
    else hi = mid;
  }
  return lo;
}

// Fusionne le paquet (ng cles valides) dans le sommet trie de taille st (au plus cap) ; cles toutes distinctes (rang).
template <class D, int M>
MHGP12_HD void merge_group(TopShared<D>& sh, u32& cur, u32& st, u32 cap, Lanes<Key<D>>& item, u32 ng) {
  if (st == cap) {  // aucun candidat sous le seuil : rien a faire
    const Key<D> threshold = sh.top[cur][cap - 1];
    Lanes<bool> contender;
    MHGP12_LANES(l) { contender[l] = key_less<D, M>(item[l], threshold); }
    if (simt::ballot(contender) == 0) return;
  }
  bitonic32<D, M>(item);
  MHGP12_LANES(l) { sh.group[l] = item[l]; }
  simt::sync();
  const Key<D>* t = sh.top[cur];
  Key<D>* out = sh.top[cur ^ 1u];
  MHGP12_LANES(l) {
    if (l < ng) {
      const u32 at = l + lower_bound<D, M>(t, st, item[l]);
      if (at < cap) out[at] = item[l];
    }
    for (u32 e = l; e < st; e += kWarp) {
      const u32 at = e + lower_bound<D, M>(sh.group, ng, t[e]);
      if (at < cap) out[at] = t[e];
    }
  }
  simt::sync();
  st = min_u32(cap, st + ng);
  cur ^= 1u;
}

// Rang d'element -> cle. Source : la liste du parent (rangs consecutifs) ou des sommets locaux (rangs indirects).
template <class D, int M>
MHGP12_HD void select_top(const Level& lv, const Parent& p, const i64* lo, const i64* hi, const u32* ranks,
                          u64 first, u64 n, u32 cap, TopShared<D>& sh, u32* out) {
  u32 cur = 0, st = 0;
  for (u64 base = 0; base < n; base += kWarp) {
    const u32 ng = static_cast<u32>(min_u64(kWarp, n - base));
    Lanes<Key<D>> item;
    MHGP12_LANES(l) {
      if (l < ng) {
        const u32 pos = ranks != nullptr ? ranks[base + l] : static_cast<u32>(first + base + l);
        const u32 s = lv.list[p.list_begin + pos];
        item[l].dist = reservoir_key<D>(lv.x[s], lv.y[s], lv.z[s], lo, hi);
        item[l].pos = pos;
        item[l].pad = 0;
      } else {
        item[l] = key_invalid<D>();
      }
    }
    merge_group<D, M>(sh, cur, st, cap, item, ng);
  }
  MHGP12_LANES(l) {
    for (u32 e = l; e < st; e += kWarp) out[e] = sh.top[cur][e].pos;
  }
}

// ------------------------------------------------------------------------------------------- reperage des taches
struct TaskRef {
  u32 parent, side, child, chunk;
  u64 begin, end;  // rangs des candidats du morceau dans la liste du parent
};

MHGP12_HD TaskRef locate(const Level& lv, u64 t) {
  u32 lo = 0, hi = lv.n_parents;  // plus grand p tel que task_begin[p] <= t
  while (hi - lo > 1) {
    const u32 mid = lo + (hi - lo) / 2;
    if (lv.task_begin[mid] <= t) lo = mid;
    else hi = mid;
  }
  TaskRef r;
  r.parent = lo;
  const Parent& p = lv.parents[lo];
  const u64 off = t - lv.task_begin[lo];
  r.side = static_cast<u32>(off / p.tasks);
  r.chunk = static_cast<u32>(off % p.tasks);
  r.child = 2 * lo + r.side;
  r.begin = u64{r.chunk} * kChunk;
  r.end = min_u64(r.begin + kChunk, p.count);
  return r;
}

template <int M>
MHGP12_HD u32 filter_bits(const Parent& p, const i64* lo, const i64* hi) {
  if constexpr (M == kFrameChildBox) return box_frame_bits(lo, hi);
  else return p.frame_bits;
}

// ------------------------------------------------------------------------------------------------------- noyaux
template <int M>
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
    child_box(p, r.side, lo, hi, M);
    const u32 cap = min_u32(reservoir_capacity(lv.params, M), static_cast<u32>(r.end - r.begin));
    u32* out = lv.chunk_top + t * kMaxRes;
    if (filter_bits<M>(p, lo, hi) <= kNarrowBits) select_top<i64, M>(lv, p, lo, hi, nullptr, r.begin, r.end - r.begin, cap, sh.narrow, out);
    else select_top<i128, M>(lv, p, lo, hi, nullptr, r.begin, r.end - r.begin, cap, sh.wide, out);
  }
};

template <int M>
struct MergeKernel {
  Level lv;
  union Shared {
    TopShared<i64> narrow;
    TopShared<i128> wide;
  };
  // Reservoir d'un enfant a plusieurs taches : sommet des sommets locaux, chacun trie (Select) et de min(cap, longueur)
  // cles. Les taches sont prises par paquets de 32, rangees par leur PLUS PETITE cle (entree 0) ; une tache n'est
  // fusionnee que si cette cle est sous le seuil courant (la cap-ieme cle du sommet, des qu'il est plein) : sinon
  // aucune de ses cles ne l'est, ni celles des taches suivantes du paquet, et le seuil ne fait que decroitre. Elagage
  // exact : le sommet final est l'ensemble des cap plus petites cles (toutes distinctes), dans l'ordre.
  template <class D>
  MHGP12_HD void run(const Parent& p, const i64* lo, const i64* hi, u64 first, u32 cap, TopShared<D>& sh,
                     u32* out) const {
    u32 cur = 0, st = 0;
    for (u32 tb = 0; tb < p.tasks; tb += kWarp) {
      Lanes<Key<D>> head;  // plus petite cle de chaque tache du paquet ; pad = rang de la tache dans l'enfant
      MHGP12_LANES(l) {
        head[l] = key_invalid<D>();
        if (tb + l < p.tasks) {
          const u32 pos = lv.chunk_top[(first + tb + l) * kMaxRes];
          const u32 s = lv.list[p.list_begin + pos];
          head[l].dist = reservoir_key<D>(lv.x[s], lv.y[s], lv.z[s], lo, hi);
          head[l].pos = pos;
          head[l].pad = tb + l;
        }
      }
      bitonic32<D, M>(head);
      for (u32 i = 0; i < kWarp; ++i) {
        const Key<D> h = simt::shfl(head, i);
        if (h.pos == 0xFFFFFFFFu) break;  // taches epuisees (cles invalides en queue)
        if (st == cap && !key_less<D, M>(h, sh.top[cur][cap - 1])) break;
        const u32 task = h.pad;
        const u64 len = min_u64(kChunk, u64{p.count} - u64{task} * kChunk);
        const u32 n = static_cast<u32>(min_u64(cap, len));
        for (u32 base = 0; base < n; base += kWarp) {
          const u32 ng = min_u32(kWarp, n - base);
          Lanes<Key<D>> item;
          MHGP12_LANES(l) {
            item[l] = key_invalid<D>();
            if (l < ng) {
              item[l].pos = lv.chunk_top[(first + task) * kMaxRes + base + l];
              const u32 s = lv.list[p.list_begin + item[l].pos];
              item[l].dist = reservoir_key<D>(lv.x[s], lv.y[s], lv.z[s], lo, hi);
              item[l].pad = 0;
            }
          }
          merge_group<D, M>(sh, cur, st, cap, item, ng);
        }
      }
    }
    MHGP12_LANES(l) {
      for (u32 e = l; e < st; e += kWarp) out[e] = sh.top[cur][e].pos;
    }
  }
  MHGP12_HD void operator()(u64 c, Shared& sh) const {
    const u32 pi = static_cast<u32>(c / 2), side = static_cast<u32>(c % 2);
    const Parent& p = lv.parents[pi];
    if (p.tasks == 1) return;  // le sommet du morceau unique est le reservoir
    i64 lo[3], hi[3];
    child_box(p, side, lo, hi, M);
    const u32 cap = min_u32(reservoir_capacity(lv.params, M), p.count);
    const u64 first = u64{lv.task_begin[pi]} + u64{side} * p.tasks;
    u32* out = lv.reservoir + c * kMaxRes;
    if (filter_bits<M>(p, lo, hi) <= kNarrowBits) run<i64>(p, lo, hi, first, cap, sh.narrow, out);
    else run<i128>(p, lo, hi, first, cap, sh.wide, out);
  }
};

template <int M>
struct FilterKernel {
  Level lv;
  union Shared {
    Terms<i64> narrow[kMaxRes];
    Terms<i128> wide[kMaxRes];
  };
  template <class D>
  MHGP12_HD void run(u64 t, const TaskRef& r, const Parent& p, const i64* lo, const i64* hi, Terms<D>* w) const {
    const u32 kmax = lv.params.kmax;
    const u32 witnesses = min_u32(reservoir_capacity(lv.params, M), p.count);
    const u32* ranks = p.tasks == 1 ? lv.chunk_top + t * kMaxRes : lv.reservoir + u64{r.child} * kMaxRes;
    MHGP12_LANES(l) {
      for (u32 e = l; e < witnesses; e += kWarp) {
        const u32 s = lv.list[p.list_begin + ranks[e]];
        w[e] = terms<D>(lv.x[s], lv.y[s], lv.z[s], lo, hi);
      }
    }
    simt::sync();
    Lanes<u32> tests, mn[3], mx[3];
    MHGP12_LANES(l) {
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
      MHGP12_LANES(l) {
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
    const u32 total_tests = simt::sum(tests);
    u32 env_min[3], env_max[3];
    for (int a = 0; a < 3; ++a) {
      env_min[a] = warp::reduce_min(mn[a]);
      env_max[a] = warp::reduce_max(mx[a]);
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
    child_box(p, r.side, lo, hi, M);
    if (filter_bits<M>(p, lo, hi) <= kNarrowBits) run<i64>(t, r, p, lo, hi, sh.narrow);
    else run<i128>(t, r, p, lo, hi, sh.wide);
  }
};

template <int M>
struct CloseKernel {
  Level lv;
  struct Shared {};
  MHGP12_HD void operator()(u64 c, Shared&) const {
    const u32 pi = static_cast<u32>(c / 2), side = static_cast<u32>(c % 2);
    const Parent& p = lv.parents[pi];
    i64 lo[3], hi[3];
    child_box(p, side, lo, hi, M);
    const u64 first = u64{lv.task_begin[pi]} + u64{side} * p.tasks;
    u32 running = 0;
    Lanes<u64> tests;
    Lanes<u32> mn[3], mx[3];
    MHGP12_LANES(l) {
      tests[l] = 0;
      for (int a = 0; a < 3; ++a) {
        mn[a][l] = 0xFFFFFFFFu;
        mx[a][l] = 0;
      }
    }
    for (u32 base = 0; base < p.tasks; base += kWarp) {
      Lanes<u32> kept;
      MHGP12_LANES(l) {
        kept[l] = 0;
        if (base + l < p.tasks) {
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
      MHGP12_LANES(l) {
        if (base + l < p.tasks) lv.task_out[first + base + l].out_offset = running + offset[l];
      }
      running += total;
    }
    ChildOut out;
    out.tests = warp::sum64(tests);
    u32 env_min[3], env_max[3];
    for (int a = 0; a < 3; ++a) {
      env_min[a] = warp::reduce_min(mn[a]);
      env_max[a] = warp::reduce_max(mx[a]);
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
      for (int a = 0; a < 3; ++a) {
        out.lo[a] = 0;
        out.hi[a] = 0;
      }
    } else {
      const int axis = split_axis(out.lo, out.hi, M);
      const i64 width = out.hi[axis] - out.lo[axis];
      out.kind = (running <= lv.params.leaf_size || width <= 1) ? kKindLeaf : kKindSplit;
      out.frame_bits = parent_frame_bits(out.lo, out.hi, env_min, env_max);
    }
    if (simt::leader()) lv.child_out[c] = out;
  }
};

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
    MHGP12_LANES(l) {
      u64 g[kFields], t = 0, m = 0, n = 0;
      lane_sums(lv, tile, l, g, t, m, n);
      for (u32 k = 0; k < kFields; ++k) f[k][l] = g[k];
      tests[l] = t;
      leaf[l] = static_cast<u32>(m);
      cand[l] = n;
    }
    TileSum s;
    for (u32 k = 0; k < kFields; ++k) s.f[k] = warp::sum64(f[k]);
    s.tests = warp::sum64(tests);
    s.max_leaf = warp::reduce_max(leaf);
    s.candidates = warp::sum64(cand);
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
      MHGP12_LANES(l) {
        const bool in = base + l < lv.n_tiles;
        for (u32 k = 0; k < kFields; ++k) f[k][l] = in ? lv.tile_sum[base + l].f[k] : 0;
        t[l] = in ? lv.tile_sum[base + l].tests : 0;
        m[l] = in ? static_cast<u32>(lv.tile_sum[base + l].max_leaf) : 0;
        n[l] = in ? lv.tile_sum[base + l].candidates : 0;
      }
      Lanes<u64> off[kFields];
      u64 total[kFields];
      for (u32 k = 0; k < kFields; ++k) off[k] = warp::exclusive_scan64(f[k], total[k]);
      MHGP12_LANES(l) {
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
      tests += warp::sum64(t);
      candidates += warp::sum64(n);
      const u32 mm = warp::reduce_max(m);
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
    MHGP12_LANES(l) {
      u64 g[kFields], t = 0, m = 0, n = 0;
      lane_sums(lv, tile, l, g, t, m, n);
      for (u32 k = 0; k < kFields; ++k) f[k][l] = g[k];
    }
    Lanes<u64> off[kFields];
    for (u32 k = 0; k < kFields; ++k) {
      u64 total = 0;
      off[k] = warp::exclusive_scan64(f[k], total);
    }
    const TileSum base = lv.tile_offset[tile];
    MHGP12_LANES(l) {
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

template <int M>
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
      MHGP12_LANES(l) {
        if ((mask >> l) & 1u) {
          const u32 rank = M == kUnstableCompaction ? simt::popc(mask & simt::above(l)) : simt::popc(mask & simt::below(l));
          out[running + rank] = lv.list[p.list_begin + base + l];
        }
      }
      running += simt::popc(mask);
    }
  }
};

template <int M>
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
    MHGP12_LANES(l) {
      const u64 first = tile * kTile + u64{l} * 32;
      for (u64 i = first; i < first + 32 && i < n; ++i) list[i] = static_cast<u32>(i);
    }
  }
};

}  // namespace mhgp12::traversal::bfs
