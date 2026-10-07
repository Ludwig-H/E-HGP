// Parcours des boites de centres EN LARGEUR, source unique __host__ __device__ : enregistrements, arithmetique exacte
// et outils scalaires. Port explicite de microbancs/mes_m5_parcours/include/mhgp12/traversal/bfs.hpp (MES-M5, adopte
// sur G4 le 7 octobre 2026), sans ses mutants (les mutants de la v12 sont des correctifs appliques a une copie,
// tests/mutants/catalogue.json). Objet : le meme ensemble final de feuilles que le parcours en profondeur de la v11
// (src/catalogue/boxes.cpp, ac081a06f), boites et listes, et le meme grand livre. Regles de la v11, sans exception :
//   - racine : tous les sites (ordre SiteIdx), boite = enveloppe [min, max+1) ;
//   - noeud (boite B, liste parente L) : reservoir des min(|L|, 3K) sites de L les plus proches du centre de B, cle
//     (somme des (2x - lo - hi)^2, rang dans L), tri croissant ; un site de L est retire s'il a K dominateurs G1
//     stricts parmi les temoins, examines dans l'ordre du reservoir avec arret au K-ieme (tests = temoins examines) ;
//     liste retenue dans l'ordre de L ; boite ajustee = enveloppe de la liste [min, max+1) inter B ; vide si liste vide
//     ou boite ajustee vide ;
//   - coupe : axe de plus grande largeur de la boite ajustee (le premier a egalite), milieu lo + largeur/2, gauche
//     [lo, milieu), droite [milieu, hi) ; feuille si |liste| <= taille de feuille ou largeur <= 1.
// Arithmetique (CONTRAT_NUMERIQUE.md, paragraphes 2 et 3) : le filtre d'un enfant s'evalue dans le REPERE DU PARENT,
// E = fermeture(boite ajustee du parent) U liste du parent (NUM-COUVERTURE, CST-0112), d'etendue s ; cle du reservoir
// sur 2s+4 bits, termes G1 sur 2s+3 bits : voie native i64 si s <= 29 (CST-0208), voie large i128 au-dela (s <= 33 :
// boites fermees jusqu'a 2^32, CST-0204). Au profil 21, s <= 22 : toujours la voie native, comme la v11.
#pragma once

#include "catalogue/simt.hpp"

namespace mhgp12::catalogue_detail::bfs {

using simt::kWarp;
using simt::Lanes;

inline constexpr u32 kMaxOrder = 12;
inline constexpr u32 kMaxRes = 3 * kMaxOrder;  // 36 temoins au plus
inline constexpr u32 kChunk = 256;             // candidats par tache (8 paquets de 32)
inline constexpr u32 kChunkWords = kChunk / 32;
inline constexpr u32 kTile = 1024;  // enfants par tuile des prefixes (32 par voie)
inline constexpr u32 kNarrowBits = 29;
static_assert(2 * kNarrowBits + 4 <= 63, "voie native : cle du reservoir en i64 (CST-0208)");
static_assert(2 * 33 + 4 <= 127, "voie large : cle du reservoir en i128 jusqu'a s = 33 (CST-0204)");
static_assert(kChunk >= kMaxRes && kChunk % 32 == 0, "taches : un morceau complet contient un reservoir entier");

enum : u32 { kKindEmpty = 0, kKindLeaf = 1, kKindSplit = 2 };

struct Params {
  u32 kmax = 5, leaf_size = 24, max_leaf = 256, coord_bits = 21;
};

// Noeud coupe d'un niveau : ses deux enfants (un seul, de meme boite, pour la pseudo-racine) sont traites au niveau
// suivant.
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

// Feuille emise : disposition de LeafJob de la v11 (64 premiers octets) ; begin dans l'arene des sites du niveau.
struct Leaf {
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

MHGP12_HD u32 reservoir_capacity(const Params& p) { return 3 * p.kmax; }

// Axe et milieu de coupe d'une boite ajustee (split_ready de la v11) : premier maximum.
MHGP12_HD int split_axis(const i64* lo, const i64* hi) {
  int axis = 0;
  for (int i = 1; i < 3; ++i)
    if (hi[i] - lo[i] > hi[axis] - lo[axis]) axis = i;
  return axis;
}

// Boite de l'enfant side du parent p (la pseudo-racine a un seul enfant, de meme boite).
MHGP12_HD void child_box(const Parent& p, u32 side, i64* lo, i64* hi) {
  for (int a = 0; a < 3; ++a) {
    lo[a] = p.lo[a];
    hi[a] = p.hi[a];
  }
  if (p.sides == 1) return;
  const int axis = split_axis(p.lo, p.hi);
  const i64 width = p.hi[axis] - p.lo[axis];
  const i64 middle = p.lo[axis] + width / 2;
  if (side == 0) hi[axis] = middle;
  else lo[axis] = middle;
}

MHGP12_HD void child_path(const Parent& p, u32 side, u32 parent_depth, u64* path) {
  path[0] = p.path[0];
  path[1] = p.path[1];
  if (p.sides == 2 && side == 1) path[parent_depth / 64] |= u64{1} << (63 - parent_depth % 64);
}

// Etendue du repere E = fermeture(boite ajustee) U liste, l'enveloppe de la liste etant [env_min, env_max] ; le coin
// minimal est celui de la liste (boite ajustee = enveloppe inter boite, donc lo ajuste >= env_min).
MHGP12_HD u32 parent_frame_bits(const i64* adj_hi, const u32* env_min, const u32* env_max) {
  u64 span = 0;
  for (int a = 0; a < 3; ++a) {
    const i64 top = max_i64(static_cast<i64>(env_max[a]), adj_hi[a]);
    const u64 w = static_cast<u64>(top - static_cast<i64>(env_min[a]));
    span = w > span ? w : span;
  }
  return bit_length(span);
}

// Etendue du repere du filtre d'un enfant de boite [lo, hi) : celle du repere du PARENT (CST-0112), qui contient la
// boite de l'enfant et tous les sites de la liste parente que le filtre lit ; la seule boite de l'enfant ne couvre pas
// ces sites et ne choisit jamais la voie.
MHGP12_HD u32 filter_bits(const Parent& p, const i64* lo, const i64* hi) {
  (void)lo;
  (void)hi;
  return p.frame_bits;
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

// Ordre du reservoir : distance, puis rang dans la liste parente (le tri par insertion de la v11 garde le premier
// arrive a egalite et evince le dernier : les 3K premiers de L tries par (cle, rang)).
template <class D>
MHGP12_HD bool key_less(const Key<D>& a, const Key<D>& b) {
  if (a.dist != b.dist) return a.dist < b.dist;
  return a.pos < b.pos;
}

template <class D>
MHGP12_HD Key<D> key_invalid() {
  Key<D> k;
  k.dist = key_max<D>();
  k.pos = 0xFFFFFFFFu;
  k.pad = 0;
  return k;
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

}  // namespace mhgp12::catalogue_detail::bfs
