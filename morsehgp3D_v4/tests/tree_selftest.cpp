// MorseHGP3D v4 — SELFTEST DE L'ARBRE RADIX : invariants structurels.
//
// Codes de sortie : 0 = invariants verifies, 2 = refus avant calcul,
// 3 = invariant viole. Aucun crash par signal n'est un resultat.
//
// Invariants juges (sur fixtures gravees ET familles pseudo-aleatoires) :
//   I1  cles de Morton strictement croissantes (positions uniques) ;
//   I2  la plage de chaque nœud est la reunion disjointe de celles des
//       enfants, et la racine couvre [0, m-1] ;
//   I3  boite serree incluse dans la cellule ; cellule de cote 2^k alignee ;
//   I4  poids : somme des multiplicites des feuilles = n, et la plage ponderee
//       de chaque nœud egale la somme de celles des enfants ;
//   I5  determinisme : une permutation de l'ordre d'entree des points donne
//       exactement le meme arbre (memes cles, memes plages, memes boites) et
//       les memes buckets a renommage PointId pres — l'equivariance exige que
//       les identites suivent la permutation.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <random>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/tree/radix_tree.hpp"

namespace {

using namespace mhgp4;

int g_failures = 0;

void check(bool ok, const char* what) {
  if (!ok) {
    std::fprintf(stderr, "INVARIANT VIOLE : %s\n", what);
    ++g_failures;
  }
}

void walk_ranges(const CloudIndex& ix, NodeRef v, int* count_leaves) {
  if (v < 0) {
    ++*count_leaves;
    return;
  }
  const RadixNode& n = ix.nodes[(size_t)v];
  const auto range_of = [&](NodeRef c, int* lo, int* hi) {
    if (c < 0) { *lo = *hi = -1 - c; return; }
    *lo = ix.nodes[(size_t)c].first;
    *hi = ix.nodes[(size_t)c].last;
  };
  int llo, lhi, rlo, rhi;
  range_of(n.left, &llo, &lhi);
  range_of(n.right, &rlo, &rhi);
  check(llo == n.first && rhi == n.last && lhi + 1 == rlo,
        "plages enfants non contigues ou hors du parent");
  check(ix.range_weight(n.first, n.last) ==
            ix.range_weight(llo, lhi) + ix.range_weight(rlo, rhi),
        "poids du parent different de la somme des enfants");
  for (int axis = 0; axis < 3; ++axis) {
    check(n.clo[axis] <= n.tlo[axis] && n.thi[axis] <= n.chi[axis],
          "boite serree hors de la cellule");
    const i64 side = n.chi[axis] - n.clo[axis] + 1;
    check((side & (side - 1)) == 0, "cote de cellule non puissance de deux");
    check(n.clo[axis] % side == 0, "cellule non alignee");
  }
  walk_ranges(ix, n.left, count_leaves);
  walk_ranges(ix, n.right, count_leaves);
}

void check_index(const std::vector<P3>& pts, const char* label) {
  const CloudIndex ix = build_cloud_index(pts);
  const int m = ix.unique_count();
  char what[256];
  for (int u = 1; u < m; ++u) {
    std::snprintf(what, sizeof(what), "%s : cles non strictement croissantes", label);
    check(ix.keys[(size_t)u - 1] < ix.keys[(size_t)u], what);
  }
  std::snprintf(what, sizeof(what), "%s : somme des buckets != n", label);
  check(ix.bucket_ids.size() == pts.size(), what);
  check(ix.wsum[(size_t)m] == pts.size(), what);
  if (m >= 2) {
    check((int)ix.nodes.size() == m - 1, "nombre de nœuds internes != m-1");
    check(ix.nodes[0].first == 0 && ix.nodes[0].last == m - 1,
          "la racine ne couvre pas toutes les positions uniques");
    int leaves = 0;
    walk_ranges(ix, 0, &leaves);
    std::snprintf(what, sizeof(what), "%s : feuilles atteintes != m", label);
    check(leaves == m, what);
  }
}

// I5 : permutation de l'ordre d'entree — meme geometrie, identites permutees.
void check_permutation(const std::vector<P3>& pts, unsigned seed, const char* label) {
  std::vector<u32> perm(pts.size());
  std::iota(perm.begin(), perm.end(), 0u);
  std::mt19937 rng(seed);
  std::shuffle(perm.begin(), perm.end(), rng);
  std::vector<P3> shuffled(pts.size());
  for (size_t i = 0; i < pts.size(); ++i) shuffled[perm[i]] = pts[i];

  const CloudIndex a = build_cloud_index(pts);
  const CloudIndex b = build_cloud_index(shuffled);
  char what[256];
  std::snprintf(what, sizeof(what), "%s : geometrie non equivariante", label);
  check(a.keys == b.keys, what);
  check(a.bucket_start == b.bucket_start, what);
  check(a.wsum == b.wsum, what);
  check(a.nodes.size() == b.nodes.size(), what);
  for (size_t v = 0; v < a.nodes.size(); ++v) {
    const RadixNode &na = a.nodes[v], &nb = b.nodes[v];
    check(na.first == nb.first && na.last == nb.last && na.left == nb.left &&
              na.right == nb.right,
          what);
    for (int axis = 0; axis < 3; ++axis)
      check(na.tlo[axis] == nb.tlo[axis] && na.thi[axis] == nb.thi[axis] &&
                na.clo[axis] == nb.clo[axis] && na.chi[axis] == nb.chi[axis],
            what);
  }
  // Les identites suivent la permutation : bucket a bucket, les PointId de `b`
  // sont l'image par `perm` de ceux de `a` (a l'ordre pres, les buckets etant
  // tries par identite croissante).
  std::snprintf(what, sizeof(what), "%s : identites non permutees", label);
  for (size_t u = 0; u + 1 < a.bucket_start.size(); ++u) {
    std::vector<PointId> ida(a.bucket_ids.begin() + a.bucket_start[u],
                             a.bucket_ids.begin() + a.bucket_start[u + 1]);
    std::vector<PointId> idb(b.bucket_ids.begin() + b.bucket_start[u],
                             b.bucket_ids.begin() + b.bucket_start[u + 1]);
    for (PointId& id : ida) id = perm[id];
    std::sort(ida.begin(), ida.end());
    check(ida == idb, what);
  }
}

}  // namespace

int main() {
  using namespace mhgp4;

  // Fixtures gravees.
  check_index(collinear_seven_cloud(), "collinear_seven");
  check_index(two_lines_cloud(16, 65536), "two_lines");
  // Doublons de position : trois points empiles plus deux isoles.
  const std::vector<P3> stacked = {{5, 5, 5}, {5, 5, 5}, {5, 5, 5}, {9, 1, 0}, {0, 9, 3}};
  check_index(stacked, "positions_dupliquees");
  {
    const CloudIndex ix = build_cloud_index(stacked);
    if (ix.unique_count() != 3) {
      std::fprintf(stderr, "INVARIANT VIOLE : bucketisation attendue 3 uniques, lu %d\n",
                   ix.unique_count());
      ++g_failures;
    }
  }
  // Cas limites : vide, singleton, deux points.
  check_index({}, "vide");
  check_index({{1, 2, 3}}, "singleton");
  check_index({{0, 0, 0}, {65535, 65535, 65535}}, "deux_extremes");

  // Familles pseudo-aleatoires aux petites tailles d'oracle.
  for (const CloudFamily family :
       {CloudFamily::kUniform, CloudFamily::kTerrain, CloudFamily::kEightClusters,
        CloudFamily::kScanlineSinglePass}) {
    const int n = 512;
    const int coord = cloud_family_default_coord(family, n);
    const std::vector<P3> pts = make_family_cloud(family, n, coord, 3);
    if ((int)pts.size() < n / 2) {
      std::fprintf(stderr, "REFUS : famille %s n'a produit que %zu points\n",
                   cloud_family_name(family), pts.size());
      return 2;
    }
    check_index(pts, cloud_family_name(family));
    check_permutation(pts, 17, cloud_family_name(family));
  }
  check_permutation(stacked, 5, "positions_dupliquees");

  if (g_failures > 0) return 3;
  std::printf("tree_selftest OK\n");
  return 0;
}
