// Sonde L12 : rejoue sur la tete PUBLIEE (head.cpp f583da40) trois constats des auditeurs.
#include <cmath>
#include <cstdio>
#include <limits>
#include "head/head.hpp"
using namespace mhgp10;

static PointDendrogram ab_tree() {
  // A : trois points admis a beta = 1, 4, 9 ; B : deux points a beta = 16 ; fusion R a beta = 25.
  PointDendrogram d;
  d.level = {1, 4, 9, 16, 25};
  d.node_rank = {0, 3, 4};            // A, B, R
  d.child_off = {0, 0, 0, 2};
  d.child_val = {0, 1};
  d.parent = {2, 2, kNone};
  d.point_node = {0, 0, 0, 1, 1};
  d.point_rank = {0, 1, 2, 3, 3};
  d.point_weight = {1, 1, 1, 1, 1};
  return d;
}

int main() {
  int bad = 0;
  {  // 1. departs de points sans controle de masse (audit continu, point_condensation_20260930)
    PointDendrogram d = ab_tree();
    Outcome o = validate(d);
    ClusterParams p;
    p.min_cluster_size = 2;
    p.z = 1.0;
    p.allow_single_cluster = true;
    Clustering c = cluster(d, p);
    std::printf("cas1 validate_ok=%d clusters=%zu\n", int(o.ok()), c.tree.parent.size());
    for (size_t i = 0; i < c.tree.parent.size(); ++i)
      std::printf("  cluster %zu parent=%d birth=%.17g stability=%.17g mass=%llu\n", i,
                  c.tree.parent[i] == kNone ? -1 : int(c.tree.parent[i]), c.tree.birth[i], c.tree.stability[i],
                  (unsigned long long)c.tree.mass[i]);
    std::printf("  selected=%zu labels=", c.selected.size());
    for (int l : c.label) std::printf("%d ", l);
    std::printf("\n  attendu (condensation HDBSCAN des points) : stab(A)=11/15=%.17g ; code publie : 37/30=%.17g\n",
                11.0 / 15.0, 37.0 / 30.0);
  }
  {  // 2. H3 : lambda finis, stabilites infinies (poids u32 maximaux, niveaux 1e-300 / 9e-300, z = 2)
    PointDendrogram d;
    d.level = {1e-300, 9e-300};
    d.node_rank = {0, 0, 1};
    d.child_off = {0, 0, 0, 2};
    d.child_val = {0, 1};
    d.parent = {2, 2, kNone};
    d.point_node = {0, 1};
    d.point_rank = {0, 0};
    for (u32 w : {1u, 4294967295u}) {
      d.point_weight = {w, w};
      ClusterParams p;
      p.min_cluster_size = w;
      p.z = 2.0;
      p.allow_single_cluster = true;
      Outcome o = validate(d);
      Clustering c = cluster(d, p);
      std::printf("cas2 w=%u validate_ok=%d stabilites:", w, int(o.ok()));
      for (double s : c.tree.stability) std::printf(" %g", s);
      std::printf(" | selection:");
      for (u32 s : c.selected) std::printf(" %u", s);
      std::printf(" (0 = racine ; attendu analytique : les deux feuilles, car L_fusion > 2 L_feuille)\n");
    }
  }
  {  // 3. fusion a niveau zero admise par validate : NaN par inf - inf
    PointDendrogram d;
    d.level = {0.0, 0.25};
    d.node_rank = {0, 0, 0, 1};  // deux feuilles et leur fusion au rang 0, puis racine
    d.child_off = {0, 0, 0, 2, 3};
    d.child_val = {0, 1, 2};
    d.parent = {2, 2, 3, kNone};
    d.point_node = {0, 1};
    d.point_rank = {0, 0};
    d.point_weight = {2, 2};
    ClusterParams p;
    p.min_cluster_size = 2;
    p.z = 1.0;
    p.allow_single_cluster = true;
    Outcome o = validate(d);
    Clustering c = cluster(d, p);
    std::printf("cas3 validate_ok=%d stabilites:", int(o.ok()));
    int nans = 0;
    for (double s : c.tree.stability) {
      std::printf(" %g", s);
      nans += std::isnan(s);
    }
    std::printf(" | NaN=%d\n", nans);
  }
  {  // 4. racine singleton a niveau zero (ADDENDUM_ZERO_ET_STATUTS) : lambda et stabilite infinis meme si mcs > masse
    PointDendrogram d;
    d.level = {0.0};
    d.node_rank = {0};
    d.child_off = {0, 0};
    d.parent = {kNone};
    d.point_node = {0};
    d.point_rank = {0};
    d.point_weight = {1};
    ClusterParams p;
    p.min_cluster_size = 5;
    Outcome o = validate(d);
    Clustering c = cluster(d, p);
    std::printf("cas4 validate_ok=%d stabilite_racine=%g point_lambda=%g\n", int(o.ok()), c.tree.stability[0],
                c.tree.point_lambda[0]);
  }
  return bad;
}
