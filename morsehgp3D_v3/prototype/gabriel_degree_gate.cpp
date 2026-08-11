// MorseHGP3D v3 — LA PORTE DU DEGRE GABRIEL (audit degre Gabriel / kissing /
// smax=11 du 11 aout 2026) : la refutation EXECUTABLE de tout cap de degre 12
// et de toute ligne CSR a largeur fixe 12. L'audit a etabli qu'aucune borne de
// degre Gabriel ne decoule de la dimension ni de `smax`, meme bucket par
// bucket ; cette porte grave ses deux fixtures u16 permanentes :
//
//   1. L'ETOILE A TREIZE FEUILLES DE RANG FERME 2 : p=(600,600,600), treize
//      directions de norme carree 25 avec v_i.v_j <= 20, q_i = p + (100+i)v_i.
//      Chaque paire (p,q_i) a le rang ferme EXACTEMENT 2 (aucun tiers dans la
//      boule diametrale fermee, aucun contact de coquille), les treize
//      distances a p sont distinctes : deg(p) = 13 dans le bucket 2.
//   2. LE BUCKET EXACT 11 : p=(32768,32768,32768), neuf temoins p+(l,0,0)
//      (1<=l<=9) STRICTEMENT interieurs a la boule diametrale de CHAQUE paire,
//      treize feuilles p+v de norme carree 4225 avec v_i.v_j <= 4200 et
//      v_x >= 57 : chaque paire (p,p+v_i) a le rang ferme EXACTEMENT 11
//      (9 temoins + 2 extremites) et deg(p) = 13 dans le bucket 11.
//
// RANG FERME d'une paire (p,q) : |{x du nuage : (x-p).(x-q) <= 0}|,
// extremites comprises (Phi(p)=Phi(q)=0). Le scan est un O(n^2) EXACT en
// i64 : sur u16, |Phi| <= 3*65535^2 < 2^35, marge i64 immense. Toutes les
// proprietes annoncees par l'audit sont re-verifiees ici par des assertions
// d'honnetete (code 3) avant que la porte ne compte quoi que ce soit.
//
// MUTANT `--inject degree-cap-12` : la publication des records du centre
// passe par une arene CSR a largeur fixe 12 — le treizieme record est
// silencieusement perdu, exactement la faute qu'une allocation « kissing
// number 12 » commettrait. La porte confronte le degre tronque au degre
// exact et tue le mutant. Codes : 0 OK ; 1 violation ; 2 CLI ; 3 plancher
// ou invariant viole ; 4 mutant tue. Aucun crash par signal.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "mhgp/mhgp.hpp"

namespace {

using i64 = long long;

// La borne dure du contrat audite : smax=11 = rang ferme maximal publie.
constexpr int kSmax = 11;
// Le cap refute : la largeur de ligne que le kissing number 12 suggererait.
constexpr int kRefutedCap = 12;
// PLANCHER DE COUVERTURE : chaque fixture doit porter exactement 13 paires
// incidentes dans son bucket — 12 ne refuterait rien, 0 serait la vacuite.
constexpr int kLeaves = 13;

struct GateInjections {
  bool degree_cap_12 = false;  // MUTANT : arene CSR a largeur fixe 12
};

// PREDICAT DIAMETRAL EXACT : Phi_{p,q}(x) = (x-p).(x-q). Strictement
// interieur, sur la coquille, strictement exterieur selon le signe.
i64 phi(const mhgp::P3& p, const mhgp::P3& q, const mhgp::P3& x) {
  const i64 ax = (i64)x.x - (i64)p.x, ay = (i64)x.y - (i64)p.y, az = (i64)x.z - (i64)p.z;
  const i64 bx = (i64)x.x - (i64)q.x, by = (i64)x.y - (i64)q.y, bz = (i64)x.z - (i64)q.z;
  return ax * bx + ay * by + az * bz;
}

// RANG FERME EXACT de la paire (pts[ip], pts[iq]) : scan O(n) en i64,
// extremites comprises (leur Phi vaut zero, donc <= 0).
int closed_rank(const std::vector<mhgp::P3>& pts, int ip, int iq) {
  int rank = 0;
  for (const mhgp::P3& x : pts)
    if (phi(pts[ip], pts[iq], x) <= 0) ++rank;
  return rank;
}

// LE COMPTEUR DE REFERENCE, JAMAIS INJECTE : degre exact du centre dans le
// bucket de rang ferme exactement `target_rank`.
int exact_bucket_degree(const std::vector<mhgp::P3>& pts, int center, int target_rank) {
  int degree = 0;
  for (int j = 0; j < (int)pts.size(); ++j) {
    if (j == center) continue;
    if (closed_rank(pts, center, j) == target_rank) ++degree;
  }
  return degree;
}

// LE COMPTEUR DE LA PORTE : il rejoue la publication des records q2 incidents
// au centre dans une ligne d'adjacence (tout record de rang ferme <= smax est
// publie). NOMINAL : la ligne accepte tout. MUTANT degree-cap-12 : la ligne
// est une arene CSR a largeur fixe 12 et le treizieme record est perdu sans
// bruit — c'est la troncature que la porte doit detecter.
int gate_bucket_degree(const std::vector<mhgp::P3>& pts, int center, int target_rank,
                       const GateInjections& inj) {
  int kept_rows = 0;
  int degree = 0;
  for (int j = 0; j < (int)pts.size(); ++j) {
    if (j == center) continue;
    const int r = closed_rank(pts, center, j);
    if (r > kSmax) continue;                                 // hors smax : jamais publie
    if (inj.degree_cap_12 && kept_rows >= kRefutedCap) continue;  // MUTANT : record perdu
    ++kept_rows;
    if (r == target_rank) ++degree;
  }
  return degree;
}

// Une fixture gravee : le nuage, l'index du centre, les index des treize
// feuilles et le rang ferme attendu de chacune de leurs paires.
struct Fixture {
  const char* name = "";
  std::vector<mhgp::P3> pts;
  int center = 0;
  std::vector<int> leaves;
  int expected_rank = 0;
  i64 coord_lo = 0, coord_hi = 0;  // l'emprise annoncee par l'audit
};

// FIXTURE 1 — l'audit, section 3 : p=(600,600,600), q_i = p + (100+i)v_i,
// i = 1..13. La table des v_i est copiee A L'IDENTIQUE du document d'audit.
Fixture build_fixture_rank2() {
  static const i64 kV[kLeaves][3] = {
      {5, 0, 0},  {-5, 0, 0},  {0, 5, 0},  {0, -5, 0},
      {0, 0, 5},  {0, 0, -5},  {3, 4, 0},  {3, -4, 0},
      {-3, 4, 0}, {-3, -4, 0}, {3, 0, 4},  {3, 0, -4},
      {-3, 0, 4}};
  Fixture f;
  f.name = "etoile a treize feuilles, rang ferme 2";
  f.expected_rank = 2;
  f.coord_lo = 35;
  f.coord_hi = 1165;
  const mhgp::P3 p{600, 600, 600};
  f.pts.push_back(p);
  for (int i = 0; i < kLeaves; ++i) {
    const i64 scale = 100 + (i + 1);  // numerotation 1..13 de l'audit
    f.pts.push_back(mhgp::P3{p.x + scale * kV[i][0], p.y + scale * kV[i][1],
                             p.z + scale * kV[i][2]});
    f.leaves.push_back((int)f.pts.size() - 1);
  }
  return f;
}

// FIXTURE 2 — l'audit, section 3.1 : p=(32768,32768,32768), neuf temoins
// p+(l,0,0) pour 1<=l<=9, puis treize feuilles p+v. La table des v est
// copiee A L'IDENTIQUE du document d'audit.
Fixture build_fixture_rank11() {
  static const i64 kV[kLeaves][3] = {
      {65, 0, 0},
      {60, 25, 0},  {60, -25, 0},
      {60, 24, 7},  {60, 24, -7},  {60, -24, 7},  {60, -24, -7},
      {60, 20, 15}, {60, 20, -15}, {60, -20, 15}, {60, -20, -15},
      {57, 20, 24}, {57, -20, -24}};
  Fixture f;
  f.name = "bucket exact 11, neuf temoins communs";
  f.expected_rank = 11;
  f.coord_lo = 32743;
  f.coord_hi = 32833;
  const mhgp::P3 p{32768, 32768, 32768};
  f.pts.push_back(p);
  for (i64 l = 1; l <= 9; ++l) f.pts.push_back(mhgp::P3{p.x + l, p.y, p.z});
  for (int i = 0; i < kLeaves; ++i) {
    f.pts.push_back(
        mhgp::P3{p.x + kV[i][0], p.y + kV[i][1], p.z + kV[i][2]});
    f.leaves.push_back((int)f.pts.size() - 1);
  }
  return f;
}

// ASSERTIONS D'HONNETETE (code 3) : les proprietes que l'audit annonce sont
// RE-DEMONTREES sur les entiers graves, jamais crues sur parole. Ce chemin
// n'est jamais touche par les injections. Rend nullptr ou la violation.
const char* honesty_check(const Fixture& f, i64 norm2_expected, i64 dot_max,
                          i64 phi_third_floor, int interior_expected) {
  const int n = (int)f.pts.size();
  if ((int)f.leaves.size() != kLeaves)
    return "plancher viole : la fixture ne porte pas treize feuilles";
  // Le nuage est exactement { p } + temoins + feuilles : 14 puis 23 points.
  if (n != 1 + interior_expected + kLeaves)
    return "plancher viole : cardinal du nuage inattendu";

  // Emprise annoncee ET domaine u16 : chaque coordonnee, chaque point.
  for (const mhgp::P3& pt : f.pts) {
    const i64 c[3] = {pt.x, pt.y, pt.z};
    for (int k = 0; k < 3; ++k) {
      if (c[k] < 0 || c[k] > 65535) return "une coordonnee sort du domaine u16";
      if (c[k] < f.coord_lo || c[k] > f.coord_hi)
        return "une coordonnee sort de l'emprise annoncee par l'audit";
    }
  }

  // Points deux a deux distincts (clef 51 bits : coordonnees < 2^17).
  std::vector<i64> keys;
  keys.reserve((size_t)n);
  for (const mhgp::P3& pt : f.pts)
    keys.push_back((pt.x << 34) | (pt.y << 17) | pt.z);
  std::sort(keys.begin(), keys.end());
  if (std::adjacent_find(keys.begin(), keys.end()) != keys.end())
    return "deux points partagent la meme coordonnee";

  const mhgp::P3& p = f.pts[(size_t)f.center];

  // Normes carrees et produits scalaires des directions v_i = q_i - p,
  // ramenes aux vecteurs unitaires de l'audit : pour la fixture 1 la norme
  // graduee vaut 25*(100+i)^2, l'audit annonce ||v_i||^2 = 25 sur la table
  // brute ; on verifie donc directement les bornes decisives sur Phi.
  std::vector<i64> d2;
  for (int li = 0; li < kLeaves; ++li) {
    const mhgp::P3& qi = f.pts[(size_t)f.leaves[(size_t)li]];
    const i64 vx = qi.x - p.x, vy = qi.y - p.y, vz = qi.z - p.z;
    d2.push_back(vx * vx + vy * vy + vz * vz);
  }
  if (norm2_expected > 0) {
    // Fixture 2 : toutes les feuilles a la meme distance carree 4225.
    for (const i64 v : d2)
      if (v != norm2_expected) return "une norme carree de feuille contredit l'audit";
  } else {
    // Fixture 1 : les treize distances a p sont deux a deux distinctes.
    std::vector<i64> sorted = d2;
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
      return "deux feuilles sont a la meme distance de p";
  }
  if (dot_max > 0 && norm2_expected > 0) {
    // Fixture 2 : v_i.v_j <= 4200 pour i != j (donc Phi tiers >= 25 > 0).
    for (int i = 0; i < kLeaves; ++i)
      for (int j = i + 1; j < kLeaves; ++j) {
        const mhgp::P3& a = f.pts[(size_t)f.leaves[(size_t)i]];
        const mhgp::P3& b = f.pts[(size_t)f.leaves[(size_t)j]];
        const i64 d = (a.x - p.x) * (b.x - p.x) + (a.y - p.y) * (b.y - p.y) +
                      (a.z - p.z) * (b.z - p.z);
        if (d > dot_max) return "un produit scalaire de directions depasse la borne de l'audit";
      }
  }

  // LE COEUR : pour chaque paire (p, q_i), (a) tout tiers-feuille est
  // STRICTEMENT exterieur, Phi >= plancher de l'audit ; (b) exactement
  // `interior_expected` tiers strictement interieurs dans tout le nuage ;
  // (c) AUCUN contact de coquille (Phi = 0 interdit hors extremites) ;
  // (d) rang ferme EXACTEMENT expected_rank.
  for (int li = 0; li < kLeaves; ++li) {
    const int iq = f.leaves[(size_t)li];
    const mhgp::P3& q = f.pts[(size_t)iq];
    for (int lj = 0; lj < kLeaves; ++lj) {
      if (lj == li) continue;
      const i64 v = phi(p, q, f.pts[(size_t)f.leaves[(size_t)lj]]);
      if (v < phi_third_floor)
        return "Phi d'un tiers-feuille passe sous le plancher strict de l'audit";
    }
    int interior = 0;
    for (int x = 0; x < n; ++x) {
      if (x == f.center || x == iq) continue;
      const i64 v = phi(p, q, f.pts[(size_t)x]);
      if (v == 0) return "contact de coquille interdit : Phi = 0 sur un tiers";
      if (v < 0) ++interior;
    }
    if (interior != interior_expected)
      return "le nombre de temoins strictement interieurs contredit l'audit";
    if (closed_rank(f.pts, f.center, iq) != f.expected_rank)
      return "le rang ferme d'une paire incidente contredit l'audit";
  }
  return nullptr;
}

}  // namespace

int main(int argc, char** argv) {
  GateInjections injections;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--inject")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --inject\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "degree-cap-12")) injections.degree_cap_12 = true;
      else { std::printf("ECHEC : injection inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    std::printf("ECHEC : argument inconnu %s\n", argv[i]);
    return 2;
  }

  // Le verdict de la porte quand le degre publie contredit le degre exact :
  // 4 si une injection est active (mutant tue), 1 sinon (violation reelle).
  const auto fail = [&injections](const char* fixture, int got, int exact) -> int {
    if (injections.degree_cap_12) {
      std::printf("mutant tue par la porte du degre Gabriel : degre tronque %d,"
                  " degre exact %d (%s)\n", got, exact, fixture);
      return 4;
    }
    std::printf("ECHEC de la porte du degre Gabriel : degre publie %d, degre"
                " exact %d (%s)\n", got, exact, fixture);
    return 1;
  };

  Fixture fixtures[2] = {build_fixture_rank2(), build_fixture_rank11()};
  // Fixture 1 : distances distinctes (norme attendue 0 = « distinctes »),
  // Phi tiers >= 26765 (plancher demontre par l'audit), 0 temoin interieur.
  // Fixture 2 : norme carree 4225, dots <= 4200, Phi tiers >= 25, 9 temoins.
  const i64 norms[2] = {0, 4225};
  const i64 dots[2] = {0, 4200};
  const i64 floors[2] = {26765, 25};
  const int interiors[2] = {0, 9};

  i64 pairs_checked = 0;
  i64 permutation_runs = 0;

  for (int fi = 0; fi < 2; ++fi) {
    Fixture& f = fixtures[fi];

    // 1. HONNETETE D'ABORD : la fixture gravee re-demontre l'audit, sinon
    // plancher/invariant viole — la porte ne compte rien sur du faux.
    const char* what = honesty_check(f, norms[fi], dots[fi], floors[fi], interiors[fi]);
    if (what != nullptr) {
      std::printf("ECHEC (plancher/invariant) : %s (fixture %d, %s)\n", what, fi + 1, f.name);
      return 3;
    }

    // 2. LE DEGRE EXACT, imprime avec le rang ferme verifie de chaque paire.
    const int exact = exact_bucket_degree(f.pts, f.center, f.expected_rank);
    std::printf("fixture %d (%s) : degre exact de p = %d dans le bucket de"
                " rang ferme %d\n", fi + 1, f.name, exact, f.expected_rank);
    for (int li = 0; li < kLeaves; ++li) {
      const int r = closed_rank(f.pts, f.center, f.leaves[(size_t)li]);
      std::printf("  paire p-q%02d : rang ferme = %d\n", li + 1, r);
      ++pairs_checked;
    }
    if (exact < kLeaves) {
      std::printf("ECHEC (plancher/invariant) : degre exact %d < 13, la fixture"
                  " ne refute plus le cap (fixture %d)\n", exact, fi + 1);
      return 3;
    }

    // 3. LA PORTE, SOUS TROIS ORDRES DE PointId : identite, miroir, melange
    // deterministe (LCG grave). L'EQUIVARIANCE PAR PERMUTATION est exigee :
    // le degre publie doit retrouver le degre exact dans chaque ordre — une
    // arene a largeur fixe 12 perd un record dans TOUT ordre et meurt ici.
    for (int perm = 0; perm < 3; ++perm) {
      std::vector<int> order((size_t)f.pts.size());
      for (int j = 0; j < (int)order.size(); ++j) order[(size_t)j] = j;
      if (perm == 1) std::reverse(order.begin(), order.end());
      if (perm == 2) {
        unsigned long long lcg = 20260811ull;
        for (int j = (int)order.size() - 1; j > 0; --j) {
          lcg = lcg * 6364136223846793005ull + 1442695040888963407ull;
          std::swap(order[(size_t)j], order[(size_t)(lcg % (unsigned long long)(j + 1))]);
        }
      }
      std::vector<mhgp::P3> permuted;
      permuted.reserve(f.pts.size());
      int center = -1;
      for (int j = 0; j < (int)order.size(); ++j) {
        if (order[(size_t)j] == f.center) center = j;
        permuted.push_back(f.pts[(size_t)order[(size_t)j]]);
      }
      const int gate_degree = gate_bucket_degree(permuted, center, f.expected_rank, injections);
      if (gate_degree < kLeaves || gate_degree != exact)
        return fail(f.name, gate_degree, exact);
      ++permutation_runs;
    }
  }

  // PLANCHERS FINAUX contre le vert-par-vacuite : 26 paires et 6 ordres.
  if (pairs_checked != 2ll * kLeaves || permutation_runs != 6) {
    std::printf("ECHEC (plancher/invariant) : couverture insuffisante (%lld paires,"
                " %lld permutations)\n", pairs_checked, permutation_runs);
    return 3;
  }

  if (injections.degree_cap_12) {
    std::printf("MUTANT SURVIVANT : le cap 12 n'a pas ete detecte\n");
    return 0;
  }

  std::printf("porte du degre Gabriel : %lld paires verifiees, %lld ordres de"
              " PointId, degre 13 aux buckets 2 et 11\n", pairs_checked, permutation_runs);
  std::printf("OK : gate du degre Gabriel recue — le cap 12 est refute aux"
              " rangs fermes 2 et 11\n");
  return 0;
}
