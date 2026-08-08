// MorseHGP3D v2 — regressions permanentes issues des audits.
//
// R1  WARNING_AUDIT_COMPLETUDE §1        croissance qui s'arrete trop tot
// R2  WARNING_AUDIT_COMPLETUDE §3        support non minimal accepte
// R3  WARNING_AUDIT_IMPLEMENTATION_2 §2  filtre par cliques de faces
// R4  WARNING_AUDIT_IMPLEMENTATION_2 §3  cone_directions degenere
// R5  WARNING_AUDIT_IMPLEMENTATION_2 §1  permutation impaire d un tetraedre
// R6  WARNING_AUDIT_IMPLEMENTATION_2 §4  multifusion contractee par lot
// R7  WARNING_AUDIT_IMPLEMENTATION_2 §4  descente non resolue -> rien publie
// R8  echec O2 du 8 aout                 niveau exact lisible sur chaque noeud

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"

using namespace mhgp;

static int failures = 0;
static void check(bool c, const std::string& what) {
  if (!c) { ++failures; std::printf("  ECHEC %s\n", what.c_str()); }
  else std::printf("  ok    %s\n", what.c_str());
}

static bool has_support(const Catalogue& cat, std::vector<i32> want, int* rank_out) {
  std::sort(want.begin(), want.end());
  for (const CriticalSphere& s : cat.spheres) {
    if (s.n_support != static_cast<int>(want.size())) continue;
    bool same = true;
    for (size_t i = 0; i < want.size(); ++i)
      if (s.support[i] != want[i]) { same = false; break; }
    if (same) { if (rank_out) *rank_out = s.rank; return true; }
  }
  return false;
}

int main() {
  // ---- R5 : permutation impaire d'un tetraedre regulier ------------------
  {
    const P3 v[4] = {{2, 2, 2}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2}};
    int perm[4] = {0, 1, 2, 3};
    bool all_ok = true;
    do {
      Sphere s;
      const bool built = sphere4(v[perm[0]], v[perm[1]], v[perm[2]], v[perm[3]], &s);
      if (!built) { all_ok = false; break; }
      if (!well_centered4(s, v[perm[0]], v[perm[1]], v[perm[2]], v[perm[3]]))
        all_ok = false;
      // centre (1,1,1), niveau 3
      for (int i = 0; i < 4; ++i)
        if (sphere_side(s, v[i]) != 0) all_ok = false;
      if (sphere_side(s, P3{1, 1, 1}) >= 0) all_ok = false;
    } while (std::next_permutation(perm, perm + 4));
    check(all_ok, "R5 bon centrage invariant par permutation");
  }

  // ---- R2 : le support minimal de {p,u,v} est {u,v} ----------------------
  {
    const P3 p{0, 0, 0}, u{1000, 0, 0}, v{0, 1000, 0};
    check(!well_centered3(p, u, v), "R2 support non minimal rejete");
    std::vector<P3> pts{p, u, v};
    i32 ids[3] = {0, 1, 2};
    const MiniballResult mb = miniball_of(pts, ids, 3);
    check(mb.ok && mb.n_support == 2, "R2 miniball de support 2");
  }

  // ---- R1 : dix points serres a gauche, un point tres loin a droite ------
  {
    std::vector<P3> pts;
    pts.push_back(P3{1000, 500, 500});          // p = indice 0
    for (int i = 0; i < 10; ++i)
      pts.push_back(P3{900 - i, 500 + (i % 3), 500 - (i % 2)});
    pts.push_back(P3{60000, 500, 500});         // z, tres loin
    Options opt;
    opt.k_max = 10;
    opt.threads = 1;
    const Catalogue cat = build_catalogue(pts, 11, opt);
    int rank = -1;
    const bool found = has_support(cat, {0, 11}, &rank);
    check(found, "R1 paire lointaine retrouvee");
    check(found && rank == 2, "R1 rang ferme 2 (" + std::to_string(rank) + ")");
  }

  // ---- R4 : recouvrement degenere ne doit jamais certifier ---------------
  {
    std::vector<P3> pts;
    pts.push_back(P3{100, 0, 0});
    for (int i = 0; i < 10; ++i) pts.push_back(P3{i, 0, 0});
    pts.push_back(P3{65535, 0, 0});
    for (int cones : {0, 1, 2}) {
      Options opt;
      opt.k_max = 10;
      opt.threads = 1;
      opt.cone_directions = cones;
      const Catalogue cat = build_catalogue(pts, 11, opt);
      int rank = -1;
      const bool found = has_support(cat, {0, 11}, &rank);
      check(found && rank == 2,
            "R4 cones=" + std::to_string(cones) + " paire {p,z} retrouvee");
    }
  }

  // ---- R3 : tetraedre regulier bien centre dont les 4 faces sont lourdes --
  {
    std::vector<P3> pts;
    pts.push_back(P3{160, 160, 160});
    pts.push_back(P3{160, 40, 40});
    pts.push_back(P3{40, 160, 40});
    pts.push_back(P3{40, 40, 160});
    const int sig[4][3] = {{1, 1, -1}, {1, -1, 1}, {-1, 1, 1}, {-1, -1, -1}};
    const int del[9][3] = {{0, 0, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0},
                           {0, 0, 1}, {0, 0, -1}, {1, 1, 0}, {-1, -1, 0}};
    for (int s = 0; s < 4; ++s)
      for (int d = 0; d < 9; ++d)
        pts.push_back(P3{100 + 70 * sig[s][0] + del[d][0],
                         100 + 70 * sig[s][1] + del[d][1],
                         100 + 70 * sig[s][2] + del[d][2]});

    // verification independante des hypotheses de la fixture
    Sphere U;
    const bool built = sphere4(pts[0], pts[1], pts[2], pts[3], &U);
    check(built && well_centered4(U, pts[0], pts[1], pts[2], pts[3]),
          "R3 tetraedre bien centre");
    int inside_or_on = 0;
    for (const P3& q : pts) if (sphere_side(U, q) <= 0) ++inside_or_on;
    check(inside_or_on == 4, "R3 circumboule ne contient que les 4 sommets ("
                                 + std::to_string(inside_or_on) + ")");
    // chaque face doit bien etre lourde : c'est ce qui piegeait l'ancien filtre
    Sphere F;
    check(sphere3(pts[0], pts[1], pts[2], &F), "R3 face construite");
    int face_rank = 0;
    for (const P3& q : pts) if (sphere_side(F, q) <= 0) ++face_rank;
    check(face_rank >= 12, "R3 face de rang >= 12 (" + std::to_string(face_rank) + ")");

    Options opt;
    opt.k_max = 10;
    opt.threads = 1;
    const Catalogue cat = build_catalogue(pts, 11, opt);
    int rank = -1;
    const bool found = has_support(cat, {0, 1, 2, 3}, &rank);
    check(found, "R3 support quatre retrouve malgre ses faces lourdes");
    check(found && rank == 4, "R3 rang ferme 4 (" + std::to_string(rank) + ")");
  }

  // ---- R6 : multifusion simultanee, carre a l'ordre 1 --------------------
  {
    std::vector<P3> pts{{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}};
    Options opt;
    opt.k_max = 1;
    opt.threads = 1;
    opt.exhaustive_neighbourhood = true;
    const Catalogue cat = build_catalogue(pts, 2, opt);
    check(cat.spheres.size() == 8,
          "R6 catalogue de 8 spheres (" + std::to_string(cat.spheres.size()) + ")");
    const Forest f = build_forest(pts, cat, 1);
    check(f.births == 4, "R6 quatre naissances (" + std::to_string(f.births) + ")");
    check(f.unresolved_arms == 0, "R6 tous les bras resolus");
    check(f.merge_events == 1,
          "R6 un seul evenement de fusion (" + std::to_string(f.merge_events) + ")");
    int quad = 0, chained = 0;
    for (const ForestNode& n : f.nodes) {
      if (n.kind != 1) continue;
      if (n.n_children == 4) ++quad;
      if (n.parent >= 0 && f.nodes[static_cast<size_t>(n.parent)].kind == 1) ++chained;
    }
    check(quad == 1, "R6 un noeud de multifusion a quatre enfants");
    check(chained == 0, "R6 aucune chaine de fusions de meme niveau");
    check(f.roots.size() == 1, "R6 une racine");
  }

  // ---- R7 : descente non resolue, aucune foret ne doit sortir ------------
  {
    // Nuage volontairement degenere de WARNING_AUDIT_IMPLEMENTATION_2 §4 :
    // trois points colineaires et un quatrieme. A l'ordre 3 la descente d'un
    // evenement n'aboutit pas ; l'evenement doit etre censure atomiquement et
    // `run` ne doit publier AUCUNE foret, pas meme celles des ordres sains.
    const std::vector<P3> pts{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {1, 2, 0}};
    Options opt;
    opt.k_max = 3;
    opt.threads = 1;
    opt.all_orders = true;
    opt.exhaustive_neighbourhood = true;

    const Catalogue cat = build_catalogue(pts, 4, opt);
    const Forest f1 = build_forest(pts, cat, 1);
    const Forest f3 = build_forest(pts, cat, 3);
    check(f1.authoritative && f1.censored_events == 0, "R7 ordre 1 sain");
    check(f3.unresolved_arms == 2,
          "R7 ordre 3 : deux bras non resolus (" + std::to_string(f3.unresolved_arms) + ")");
    check(f3.censored_events == 1,
          "R7 ordre 3 : un evenement censure (" + std::to_string(f3.censored_events) + ")");
    check(!f3.authoritative, "R7 ordre 3 non autoritaire");

    // Le garde de domaine est le plus EXTERIEUR : cette fixture est aussi
    // cospherique (deux boules de paire portent le point median sur leur bord),
    // et un bras non resolu est precisement ce que produit une cospherie.
    // `run` refuse donc au titre du domaine, avant meme d'atteindre la censure
    // par ordre -- les deux mecanismes sont verifies separement.
    const Result r = run(pts, opt);
    check(r.uncertified_points == 0, "R7 catalogue certifie (le refus ne vient pas du rayon)");
    check(r.catalogue.degenerate_shells > 0,
          "R7 cospherie detectee et comptee (" + std::to_string(r.catalogue.degenerate_shells) + ")");
    check(r.out_of_declared_domain, "R7 hors domaine declare");
    check(r.forests_suppressed, "R7 forets supprimees");
    check(r.forests.empty(), "R7 aucune foret publiee (" + std::to_string(r.forests.size()) + ")");
  }

  // ---- R8 : chaque noeud publie porte sa sphere source -------------------
  {
    // L'echec O2 du 8 aout venait de la seule voie de lecture du niveau d'un
    // noeud : un `double`. Aux niveaux critiques -- les seuls ou la comparaison
    // decide -- les deux flottants du meme rationnel different d'un ULP. Le
    // niveau exact doit donc etre lisible sur CHAQUE noeud, fusions comprises,
    // via `source`, et coincider avec le rationnel de sa sphere.
    // Fixture en position generale : le contrat de R8 porte sur le domaine
    // declare, il ne doit pas dependre d'une cospherie.
    std::vector<P3> pts{{0, 0, 0}, {10, 1, 2}, {3, 11, 1}, {2, 3, 12}, {9, 9, 9}, {5, 2, 8}};
    Options opt;
    opt.k_max = 2;
    opt.threads = 1;
    opt.exhaustive_neighbourhood = true;
    const Catalogue cat = build_catalogue(pts, 3, opt);
    check(cat.degenerate_shells == 0 && cat.shell_anomalies == 0,
          "R8 fixture en position generale");
    bool all_sourced = true, merge_seen = false, level_ok = true, monotone = true;
    for (int k = 1; k <= 2; ++k) {
      const Forest f = build_forest(pts, cat, k);
      if (!f.authoritative) continue;
      for (const ForestNode& nd : f.nodes) {
        if (nd.source < 0 || nd.source >= static_cast<i32>(cat.spheres.size())) {
          all_sourced = false;
          continue;
        }
        const CriticalSphere& cs = cat.spheres[static_cast<size_t>(nd.source)];
        if (nd.kind == 1) {
          merge_seen = true;
          if (cs.rank != k + 1) level_ok = false;   // une fusion vient du rang k+1
        } else if (cs.rank != k) {
          level_ok = false;                         // une naissance vient du rang k
        }
        if (nd.parent >= 0) {
          const ForestNode& par = f.nodes[static_cast<size_t>(nd.parent)];
          if (par.source < 0) { all_sourced = false; continue; }
          if (sphere_cmp_beta(cs.sph, cat.spheres[static_cast<size_t>(par.source)].sph) > 0)
            monotone = false;
        }
      }
    }
    check(all_sourced, "R8 tout noeud porte une sphere source");
    check(merge_seen, "R8 la fixture contient bien des multifusions");
    check(level_ok, "R8 rang de la sphere source coherent avec le type du noeud");
    check(monotone, "R8 niveau exact croissant vers la racine");
  }

  if (failures) { std::printf("ECHEC : %d regressions\n", failures); return 1; }
  std::printf("OK : regressions\n");
  return 0;
}
