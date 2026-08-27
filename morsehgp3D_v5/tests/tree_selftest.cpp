// MorseHGP3D v5 — selftest de l'index spatial : invariants structurels.
//
// Codes : 0 = invariants verifies, 2 = refus avant calcul, 3 = invariant viole.
//   I1 cles de Morton strictement croissantes ;
//   I2 la plage de chaque nœud est la reunion disjointe de celles des enfants,
//      la racine couvre [0, m-1] ;
//   I3 boite serree incluse dans la cellule ; cellule alignee de cote 2^k ;
//   I4 poids : somme des multiplicites = n, plage ponderee = somme des enfants ;
//   I5 equivariance : une permutation de l'ordre d'entree donne le meme arbre
//      et des buckets dont les identites suivent la permutation ;
//   I6 garde d'entree : coordonnee hors profil ou PointId duplique => refus.
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <random>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/tree/cloud_index.hpp"

namespace {

using namespace mhgp5;

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
  const NodeRange l = ix.range_of(n.left), r = ix.range_of(n.right);
  check(l.first == n.first && r.last == n.last && l.last + 1 == r.first,
        "plages enfants non contigues ou hors du parent");
  check(ix.range_weight(n.first, n.last) == ix.range_weight(l.first, l.last) + ix.range_weight(r.first, r.last),
        "poids du parent different de la somme des enfants");
  for (int axis = 0; axis < 3; ++axis) {
    check(n.clo[axis] <= n.tlo[axis] && n.thi[axis] <= n.chi[axis], "boite serree hors de la cellule");
    const i64 side = n.chi[axis] - n.clo[axis] + 1;
    check((side & (side - 1)) == 0, "cote de cellule non puissance de deux");
    check(n.clo[axis] % side == 0, "cellule non alignee");
  }
  walk_ranges(ix, n.left, count_leaves);
  walk_ranges(ix, n.right, count_leaves);
}

void check_index(const std::vector<P3>& pts, const char* label) {
  const CloudIndex ix = build_cloud_index(pts);
  char what[256];
  std::snprintf(what, sizeof(what), "%s : index refuse a tort", label);
  check(ix.valid, what);
  const int m = ix.unique_count();
  for (int u = 1; u < m; ++u) {
    std::snprintf(what, sizeof(what), "%s : cles non strictement croissantes", label);
    check(ix.keys[(size_t)u - 1] < ix.keys[(size_t)u], what);
  }
  std::snprintf(what, sizeof(what), "%s : somme des buckets != n", label);
  check(ix.bucket_ids.size() == pts.size(), what);
  check(ix.wsum[(size_t)m] == pts.size(), what);
  if (m >= 2) {
    check((int)ix.nodes.size() == m - 1, "nombre de nœuds internes != m-1");
    check(ix.nodes[0].first == 0 && ix.nodes[0].last == m - 1, "la racine ne couvre pas toutes les positions");
    int leaves = 0;
    walk_ranges(ix, 0, &leaves);
    std::snprintf(what, sizeof(what), "%s : feuilles atteintes != m", label);
    check(leaves == m, what);
  }
}

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
  check(a.keys == b.keys && a.bucket_start == b.bucket_start && a.wsum == b.wsum, what);
  check(a.nodes.size() == b.nodes.size(), what);
  for (size_t v = 0; v < a.nodes.size() && v < b.nodes.size(); ++v) {
    const RadixNode &na = a.nodes[v], &nb = b.nodes[v];
    check(na.first == nb.first && na.last == nb.last && na.left == nb.left && na.right == nb.right, what);
    for (int axis = 0; axis < 3; ++axis)
      check(na.tlo[axis] == nb.tlo[axis] && na.thi[axis] == nb.thi[axis] && na.clo[axis] == nb.clo[axis] &&
                na.chi[axis] == nb.chi[axis],
            what);
  }
  std::snprintf(what, sizeof(what), "%s : identites non permutees", label);
  for (size_t u = 0; u + 1 < a.bucket_start.size(); ++u) {
    std::vector<PointId> ida(a.bucket_ids.begin() + a.bucket_start[u], a.bucket_ids.begin() + a.bucket_start[u + 1]);
    std::vector<PointId> idb(b.bucket_ids.begin() + b.bucket_start[u], b.bucket_ids.begin() + b.bucket_start[u + 1]);
    for (PointId& id : ida) id = perm[id];
    std::sort(ida.begin(), ida.end());
    check(ida == idb, what);
  }
}

}  // namespace

int main() {
  using namespace mhgp5;
  check_index(collinear_seven_cloud(), "collinear_seven");
  check_index(two_lines_cloud(16, 65536), "two_lines");
  const std::vector<P3> stacked = {{5, 5, 5}, {5, 5, 5}, {5, 5, 5}, {9, 1, 0}, {0, 9, 3}};
  check_index(stacked, "positions_dupliquees");
  check(build_cloud_index(stacked).unique_count() == 3, "bucketisation attendue 3 uniques");
  check(build_cloud_index(stacked).has_duplicate_positions(), "doublons non detectes");
  check_index({}, "vide");
  check_index({{1, 2, 3}}, "singleton");
  check_index({{0, 0, 0}, {65535, 65535, 65535}}, "deux_extremes");
  // I6 : garde d'entree.
  check(!build_cloud_index(std::vector<P3>{{0, 0, 0}, {65536, 0, 0}}).valid, "coordonnee hors profil acceptee");
  check(!build_cloud_index(std::vector<InputPoint>{{7, {0, 0, 0}}, {7, {1, 0, 0}}}).valid, "PointId duplique accepte");
  check(build_cloud_index(std::vector<InputPoint>{{4000000000u, {0, 0, 0}}, {1, {1, 0, 0}}}).point_id(0) == 4000000000u,
        "id au-dessus du bit 31 perdu");

  for (const CloudFamily family : {CloudFamily::kUniform, CloudFamily::kTerrain, CloudFamily::kEightClusters,
                                   CloudFamily::kScanlineSinglePass}) {
    const int n = 512;
    const int coord = cloud_family_default_coord(family, n);
    const std::vector<P3> pts = make_family_cloud(family, n, coord, 3);
    if ((int)pts.size() < n / 2) {
      std::fprintf(stderr, "REFUS : famille %s n'a produit que %zu points\n", cloud_family_name(family), pts.size());
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
