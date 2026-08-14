// MorseHGP3D v3 — LA RESTRICTION AUX DIRECTIONS ADMISSIBLES, ET SA REFUTATION.
//
// STATUT : LE THEOREME DU §3 DE LA NOTE EST FAUX. L'auditeur l'a refute dans
// audits/REPONSE_AUDIT_Q18_Q20_CALOTTES_ADMISSIBLES_20260815.md :
//   1. l'admissibilite N'EST PAS MONOTONE de `D` vers `r<D` — l'intervalle est
//      `|s| <= D < 2(s.u)`, et grandir `D` relache la premiere inegalite tout
//      en durcissant la seconde ;
//   2. le test par sommets ECHANGE LES QUANTIFICATEURS : il prouve
//      `exists v, for all u dans C`, alors que la preuve exige de marquer toute
//      cellule verifiant `exists u dans C, exists v` ;
//   3. mon falsificateur comparait `r` a la plus grande ARETE du support, quand
//      le theoreme porte sur le DIAMETRE `2R` de la circumsphere.
//
// Ce fichier conserve la mesure du gain — qui reste un fait — et grave les
// trois contre-fixtures. Il ne certifie plus rien. La forme corrigee,
// `A_r(x) = union_v { u : 2(s.u) > max(r,|s|) }` avec marquage des cellules par
// INTERSECTION et non par inclusion, n'est pas implementee ici.
//
// Specification : audits/NOTE_SOLUTION_CALOTTES_ADMISSIBLES_20260815.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// LE FAIT QUI MOTIVE CE FICHIER
//
// Le certificat par calottes existe et il est contre-audite, mais il echoue la
// ou le contrat vit : `49,8 %` d'ancres NON certifiees sur `uniform` a
// `n=1500`, `99,1 %` sur `terrain`. Une nappe n'a par construction aucun point
// au-dela de sa normale, donc la cellule normale n'est jamais couverte, donc
// l'ancre n'est jamais certifiee.
//
// ---------------------------------------------------------------------------
// POURQUOI L'EXIGENCE EST TROP FORTE
//
// Le theoreme demande que TOUTE direction soit couverte `K` fois. Or toutes ne
// sont pas atteignables.
//
// LEMME DU PARTENAIRE ANTIPODAL. Soit `S` un support positif contenant `x`, de
// circumboule `B(c,R)`, et `u=(c-x)/R`. Il existe `v` dans `S` avec
// `(v-x).u > R`.
//   Preuve : `c` est dans le relint de `conv(S)`, donc `c = somme lambda_i v_i`
//   avec tous `lambda_i > 0` et `somme lambda_i (v_i-c) = 0`. Le produit
//   scalaire avec `x-c` donne `lambda_x R^2 + somme_{i!=x} lambda_i (v_i-c).(x-c) = 0`,
//   dont le premier terme est strictement positif : il existe donc `v` avec
//   `(v-c).(x-c) < 0`. Comme `x-c = -R u`, cela donne `(v-c).u > 0`, puis
//   `(v-x).u = (v-c).u + R > R`.
//
// Une direction n'est donc admissible que si le nuage possede un POINT REEL
// au-dela de l'equateur de la boule. La normale d'une nappe n'en a aucun.
//
// ---------------------------------------------------------------------------
// LE THEOREME PROPOSE
//
// Si, pour un rayon `r`, toute cellule ADMISSIBLE A `r` est couverte par au
// moins `K` calottes strictes, alors toute boule passant par `x` avec au plus
// `K-1` interieurs verifie `diam(B) < r`.
//   Preuve : soit `B` de diametre `D >= r` passant par `x`, de direction `u`.
//   Le lemme donne un partenaire au-dela de l'equateur, donc `u` est admissible
//   a `D`, donc a `r` — la condition s'affaiblit quand le diametre croit. La
//   cellule de `u` est donc couverte `K` fois et `B` a au moins `K` interieurs.
//
// TEST EXACT PAR CELLULE. Le minimum d'une forme lineaire sur un triangle
// spherique est atteint en un SOMMET : la cellule est l'intersection de la
// sphere avec un cone convexe dont les rayons extremes sont ses sommets. Donc
// `C` est admissible a `r` si et seulement s'il existe `v`, `s = v-x`, avec
//
//   |s|^2 <= r^2   et   pour tout sommet g de C :  s.g > 0  et  4 (s.g)^2 > r^2 |g|^2
//
// Aucun flottant, aucune racine.
//
// ---------------------------------------------------------------------------
// CE QUE CE FICHIER PROUVE, ET CE QU'IL NE PROUVE PAS
//
// Il MESURE le gain de certification, et il FALSIFIE le theoreme : le mode
// `--falsifie` enumere tous les supports retenus par ancre d'arete diametrale,
// et verifie que le diametre de chacun respecte le rayon certifie de CHACUN de
// ses sommets. Un seul depassement refute la proposition.
//
// Il ne remplace pas `certified_locality_probe.cpp`, qui reste l'autorite du
// certificat non restreint. Sa grille, son arithmetique et sa selection sont
// reecrites ici independamment, precisement pour qu'un defaut commun ne se
// compense pas.
//
// CODES DE SORTIE
//   0  accord     1  refutation     2  refus avant calcul     3  plancher
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "cloud_families.hpp"
#include "corner8_ball.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;

[[noreturn]] void refuse(const std::string& r) {
  std::fprintf(stderr, "REFUS: %s\n", r.c_str());
  std::exit(2);
}

struct Pt { i64 c[3] = {0, 0, 0}; };

inline i64 d2p(const Pt& a, const Pt& b) {
  const i64 x = a.c[0] - b.c[0], y = a.c[1] - b.c[1], z = a.c[2] - b.c[2];
  return x * x + y * y + z * z;
}

// ---------------------------------------------------------------------------
// LA GRILLE DE DIRECTIONS : subdivision de l'octaedre, sommets ENTIERS
// `|g_x|+|g_y|+|g_z| = m`. Les cellules sont des triangles geodesiques.
// ---------------------------------------------------------------------------
struct DirGrid {
  std::vector<std::array<i64, 3>> vertices;
  std::vector<i64> vnorm2;
  std::vector<std::array<int, 3>> cells;
};

DirGrid build_grid(int m) {
  DirGrid g;
  std::vector<std::array<i64, 3>> vs;
  auto index_of = [&](i64 x, i64 y, i64 z) -> int {
    for (size_t i = 0; i < vs.size(); ++i)
      if (vs[i][0] == x && vs[i][1] == y && vs[i][2] == z) return (int)i;
    vs.push_back({x, y, z});
    return (int)vs.size() - 1;
  };
  for (int sx = 0; sx < 2; ++sx)
    for (int sy = 0; sy < 2; ++sy)
      for (int sz = 0; sz < 2; ++sz) {
        const int ex = sx ? 1 : -1, ey = sy ? 1 : -1, ez = sz ? 1 : -1;
        auto vert = [&](int a, int b) {
          return index_of((i64)ex * a, (i64)ey * b, (i64)ez * (m - a - b));
        };
        for (int a = 0; a + 0 <= m; ++a)
          for (int b = 0; a + b <= m; ++b) {
            if (a + b <= m - 1)
              g.cells.push_back({vert(a, b), vert(a + 1, b), vert(a, b + 1)});
            if (a + b <= m - 2)
              g.cells.push_back({vert(a + 1, b), vert(a, b + 1), vert(a + 1, b + 1)});
          }
      }
  g.vertices = vs;
  g.vnorm2.reserve(vs.size());
  for (const auto& v : vs) g.vnorm2.push_back(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  return g;
}

// `rho^2 = |g|^2 |s|^4 / (g.s)^2`, `den == 0` marquant l'infini.
struct Rho2 { i128 num = 0, den = 0; };
inline bool rho_moins(const Rho2& a, const Rho2& b) {
  if (a.den == 0) return false;
  if (b.den == 0) return true;
  return a.num * b.den < b.num * a.den;
}
// `rho <= r`  <=>  `r^2 den >= num`
inline bool rho_sous(i64 r2, const Rho2& a) {
  if (a.den == 0) return false;
  return (i128)r2 * a.den >= a.num;
}

// ---------------------------------------------------------------------------
// LE CERTIFICAT A RAYON FIXE. Rend vrai si toute cellule — ou toute cellule
// ADMISSIBLE — est couverte au moins `kmin` fois par des calottes de rayon `r`.
// ---------------------------------------------------------------------------
struct Compteurs {
  long long cellules = 0, admissibles = 0, couvertes = 0;
};

bool certifie(const Pt& x, const std::vector<Pt>& P, const std::vector<int>& voisins,
              const DirGrid& g, i64 r, int kmin, bool restreint, Compteurs* cpt) {
  const i64 r2 = r * r;
  const size_t nc = g.cells.size();
  std::vector<int> depth(nc, 0);
  std::vector<char> admis(nc, restreint ? 0 : 1);
  for (int id : voisins) {
    const Pt& v = P[(size_t)id];
    i64 s[3];
    for (int k = 0; k < 3; ++k) s[k] = v.c[k] - x.c[k];
    const i64 ds = s[0] * s[0] + s[1] * s[1] + s[2] * s[2];
    if (ds == 0 || ds > r2) continue;
    for (size_t c = 0; c < nc; ++c) {
      // Couverture : la cellule est incluse dans la calotte `C_v(r)` si et
      // seulement si ses trois sommets y sont, la calotte etant geodesiquement
      // convexe. Sommet couvert : `s.g > 0` et `(s.g)^2 r^2 > |g|^2 (|s|^2)^2`.
      bool couvre = true, adm = true;
      for (int k = 0; k < 3; ++k) {
        const auto& gv = g.vertices[(size_t)g.cells[c][(size_t)k]];
        const i64 dot = gv[0] * s[0] + gv[1] * s[1] + gv[2] * s[2];
        if (dot <= 0) { couvre = false; adm = false; break; }
        const i64 n2 = g.vnorm2[(size_t)g.cells[c][(size_t)k]];
        if (couvre && !((i128)dot * dot * r2 > (i128)n2 * ds * ds)) couvre = false;
        // Admissibilite : `v` est au-dela de l'equateur pour TOUTE direction de
        // la cellule, soit `4 (s.g)^2 > r^2 |g|^2` au sommet minimisant.
        if (adm && !((i128)4 * dot * dot > (i128)r2 * n2)) adm = false;
        if (!couvre && !adm) break;
      }
      if (couvre) ++depth[c];
      if (restreint && adm) admis[c] = 1;
    }
  }
  bool ok = true;
  for (size_t c = 0; c < nc; ++c) {
    ++cpt->cellules;
    if (!admis[c]) continue;
    ++cpt->admissibles;
    if (depth[c] >= kmin) ++cpt->couvertes;
    else ok = false;
  }
  return ok;
}


// ---------------------------------------------------------------------------
// LES TROIS CONTRE-FIXTURES, GRAVEES.
// ---------------------------------------------------------------------------
inline i128 det3i(const i128 u[3], const i128 v[3], const i128 w[3]) {
  return u[0] * (v[1] * w[2] - v[2] * w[1]) - u[1] * (v[0] * w[2] - v[2] * w[0]) +
         u[2] * (v[0] * w[1] - v[1] * w[0]);
}

// `4 R^2` d'un tetraedre, en rationnel exact : `|Num|^2 / O^2`.
// Compare a `r^2` par `|Num|^2` contre `r^2 O^2`, sur 256 bits.
int cmp_4R2_a_r2(const Pt& p0, const Pt& p1, const Pt& p2, const Pt& p3, i64 r) {
  i128 a[3], b[3], g[3];
  for (int i = 0; i < 3; ++i) {
    a[i] = (i128)p1.c[i] - p0.c[i];
    b[i] = (i128)p2.c[i] - p0.c[i];
    g[i] = (i128)p3.c[i] - p0.c[i];
  }
  auto cr = [](const i128 u[3], const i128 v[3], i128 o[3]) {
    o[0] = u[1] * v[2] - u[2] * v[1];
    o[1] = u[2] * v[0] - u[0] * v[2];
    o[2] = u[0] * v[1] - u[1] * v[0];
  };
  i128 bg[3], ga[3], ab[3];
  cr(b, g, bg); cr(g, a, ga); cr(a, b, ab);
  const i128 O = a[0] * bg[0] + a[1] * bg[1] + a[2] * bg[2];
  if (O == 0) return 0;
  const i128 na = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
  const i128 nb = b[0] * b[0] + b[1] * b[1] + b[2] * b[2];
  const i128 ng = g[0] * g[0] + g[1] * g[1] + g[2] * g[2];
  i128 num[3];
  for (int j = 0; j < 3; ++j) num[j] = na * bg[j] + nb * ga[j] + ng * ab[j];
  // `4R^2 = |Num|^2 / O^2` : comparer `|Num|^2` a `r^2 O^2`.
  mhgp::BigInt<4> gauche = mhgp::big_add(
      mhgp::big_add(mhgp::mul128(num[0], num[0]), mhgp::mul128(num[1], num[1])),
      mhgp::mul128(num[2], num[2]));
  const mhgp::BigInt<4> droite = mhgp::mul128((i128)r * r, O * O);
  return mhgp::big_cmp(gauche, droite);
}

// Declaree plus bas : le classifieur exact de la forme corrigee.
bool cellule_intersecte(const DirGrid& g, size_t c, const i64 s[3], i64 T);

int fixtures_refutation() {
  int fautes = 0;
  // (1) LA VACUITE. `x` et `v` distants de dix, coupure cinq : aucun point
  // distinct n'est a distance au plus cinq, donc toutes les cellules sont
  // declarees non admissibles et l'hypothese est vraie PAR VACUITE. Pourtant la
  // boule diametrale de `xv` est une q2 positive, vide, de diametre dix.
  {
    std::vector<Pt> P = {Pt{{0, 0, 0}}, Pt{{10, 0, 0}}};
    const DirGrid g = build_grid(4);
    Compteurs c;
    std::vector<int> v0 = {1}, v1 = {0};
    const bool c0 = certifie(P[0], P, v0, g, 5, 8, true, &c);
    const bool c1 = certifie(P[1], P, v1, g, 5, 8, true, &c);
    const i64 D2 = d2p(P[0], P[1]);
    const bool refute = (c0 || c1) && D2 >= 25;
    std::printf("refutation_vacuite : certifie_x=%d certifie_v=%d D2=%lld r2=25"
                " refute=%d\n", (int)c0, (int)c1, (long long)D2, (int)refute);
    if (!refute) { std::fprintf(stderr, "ATTENDU: la vacuite doit refuter\n"); ++fautes; }
  }
  // (2) LES QUANTIFICATEURS. La cellule `e1,e2,e3` et `s=(10,1,1)`. La
  // direction `u=s/|s|` est admissible — `|s|=D` et `s.u=D>D/2` — mais aux
  // sommets `e2` et `e3` le produit scalaire ne vaut que un : le test par
  // sommets declare la cellule NON admissible alors qu'elle INTERSECTE
  // l'ensemble admissible.
  {
    const i64 s[3] = {10, 1, 1};
    const i64 e[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const i64 D2 = s[0] * s[0] + s[1] * s[1] + s[2] * s[2];   // 102
    bool tous_sommets = true;
    for (int k = 0; k < 3; ++k) {
      const i64 dot = e[k][0] * s[0] + e[k][1] * s[1] + e[k][2] * s[2];
      if (!((i128)4 * dot * dot > (i128)D2 * 1)) tous_sommets = false;
    }
    // `u = s/|s|` : `2 (s.u) = 2|s| > |s| = D`, donc la direction EST admissible.
    const bool direction_admissible = true;
    // LA FORME CORRIGEE DOIT MARQUER CETTE CELLULE. Le classifieur exact
    // calcule le maximum de `s.u` sur le cone : `s` est dans l'octant positif,
    // donc ce maximum vaut `|s|` et `2|s| = 20,2 > T = 11`. L'inclusion aux
    // sommets, elle, echoue. C'est precisement le mutant a tuer.
    const DirGrid g1 = build_grid(1);
    bool intersecte = false;
    for (size_t c = 0; c < g1.cells.size() && !intersecte; ++c) {
      bool octant_positif = true;
      for (int k = 0; k < 3; ++k) {
        const auto& gv = g1.vertices[(size_t)g1.cells[c][(size_t)k]];
        if (gv[0] < 0 || gv[1] < 0 || gv[2] < 0) octant_positif = false;
      }
      if (octant_positif) intersecte = cellule_intersecte(g1, c, s, 11);
    }
    std::printf("refutation_quantificateurs : D2=%lld test_sommets=%d"
                " direction_admissible=%d intersection_corrigee=%d refute=%d\n",
                (long long)D2, (int)tous_sommets, (int)direction_admissible,
                (int)intersecte, (int)(!tous_sommets && direction_admissible));
    if (!intersecte) {
      std::fprintf(stderr, "ATTENDU: le classifieur corrige doit marquer la cellule\n");
      ++fautes;
    }
    if (tous_sommets) {
      std::fprintf(stderr, "ATTENDU: le test par sommets doit echouer ici\n");
      ++fautes;
    }
  }
  // (3) L'ARETE N'EST PAS LE DIAMETRE. Tetraedre regulier translate : ses six
  // aretes valent sqrt(8) = 2,83, mais sa circumsphere a pour diametre
  // sqrt(12) = 3,46. A `r=3`, un falsificateur qui compare la plus grande arete
  // a `r` laisse passer un support dont la boule DEPASSE `r`.
  {
    const Pt q[4] = {Pt{{101, 101, 101}}, Pt{{101, 99, 99}},
                     Pt{{99, 101, 99}}, Pt{{99, 99, 101}}};
    i64 arete2 = 0;
    for (int u = 0; u < 4; ++u)
      for (int w = u + 1; w < 4; ++w) {
        const i64 e = d2p(q[u], q[w]);
        if (e > arete2) arete2 = e;
      }
    const int c = cmp_4R2_a_r2(q[0], q[1], q[2], q[3], 3);
    const bool arete_sous_r = (arete2 < 9);
    const bool boule_sur_r = (c > 0);
    std::printf("refutation_arete_vs_diametre : arete2=%lld r2=9 arete_sous_r=%d"
                " quatre_R2_sur_r2=%d refute=%d\n",
                (long long)arete2, (int)arete_sous_r, (int)boule_sur_r,
                (int)(arete_sous_r && boule_sur_r));
    if (!(arete_sous_r && boule_sur_r)) {
      std::fprintf(stderr, "ATTENDU: arete sous r et boule au-dessus de r\n");
      ++fautes;
    }
  }
  if (fautes) return 1;
  std::printf("fixtures_refutation : trois contre-fixtures reproduites\n");
  return 0;
}


// ---------------------------------------------------------------------------
// LA FORME CORRIGEE PAR L'AUDITEUR.
//
// Enveloppe necessaire des directions de supports positifs de diametre au moins
// `r`, monotone dans la bonne variable :
//
//   A_r(x) = union_v { u sur S2 : 2 (s.u) > max(r, |s|) },   s = v-x.
//
// Pour une miniboule positive reelle de diametre `D >= r`, le partenaire du
// lemme verifie `D < 2(s.u)` ; deux points de sa sphere etant distants d'au plus
// `D`, on a aussi `|s| <= D`. Sa direction appartient donc a `A_r(x)`.
//
// THEOREME. Si chaque direction de `A_r(x)` est couverte par au moins `h`
// calottes interieures a l'echelle `r`, aucune miniboule positive passant par
// `x` de diametre au moins `r` n'a moins de `h` interieurs.
//
// DEUX CORRECTIONS DE FOND PAR RAPPORT A MA VERSION REFUTEE.
//
// D'abord une cellule est marquee POTENTIELLE des qu'elle INTERSECTE une
// calotte de `A_r`, jamais quand un meme point rend toute la cellule
// admissible : `exists u dans C, exists v` et non `exists v, for all u`. Le
// maximum de `s.u` sur la cellule est calcule EXACTEMENT — direction de `s` si
// elle est dans le cone, projections sur les trois arcs quand elles y tombent,
// et sommets — chaque candidat compare par des entiers.
//
// Ensuite les partenaires ne sont PAS restreints a `B(x,r)`. Un point plus
// lointain porte la direction d'un support de diametre superieur a `r`, et se
// limiter aux voisins recree exactement la contre-fixture q2 a deux points.
// ---------------------------------------------------------------------------
inline i128 det3g(const i64 a[3], const i64 b[3], const i64 c[3]) {
  return (i128)a[0] * ((i128)b[1] * c[2] - (i128)b[2] * c[1]) -
         (i128)a[1] * ((i128)b[0] * c[2] - (i128)b[2] * c[0]) +
         (i128)a[2] * ((i128)b[0] * c[1] - (i128)b[1] * c[0]);
}

// La cellule `C` intersecte-t-elle `{ u : 2 (s.u) > T }` ? Exact, entier.
bool cellule_intersecte(const DirGrid& g, size_t c, const i64 s[3], i64 T) {
  const i64* gv[3];
  i64 gn2[3];
  for (int k = 0; k < 3; ++k) {
    gv[k] = g.vertices[(size_t)g.cells[c][(size_t)k]].data();
    gn2[k] = g.vnorm2[(size_t)g.cells[c][(size_t)k]];
  }
  const i128 T2 = (i128)T * T;
  // (a) les sommets : `2 (s.g)/|g| > T`  <=>  `4 (s.g)^2 > T^2 |g|^2`.
  for (int k = 0; k < 3; ++k) {
    const i128 dot = (i128)gv[k][0] * s[0] + (i128)gv[k][1] * s[1] + (i128)gv[k][2] * s[2];
    if (dot > 0 && 4 * dot * dot > T2 * gn2[k]) return true;
  }
  const i128 sn = (i128)s[0] * s[0] + (i128)s[1] * s[1] + (i128)s[2] * s[2];
  // (b) la direction de `s`, si elle est DANS le cone : le maximum vaut alors
  // `|s|`, et la condition devient `4 |s|^2 > T^2`.
  {
    const i128 D = det3g(gv[0], gv[1], gv[2]);
    if (D != 0) {
      const i128 a = det3g(s, gv[1], gv[2]);
      const i128 b = det3g(gv[0], s, gv[2]);
      const i128 cc = det3g(gv[0], gv[1], s);
      const bool dedans_cone = (D > 0) ? (a >= 0 && b >= 0 && cc >= 0)
                                       : (a <= 0 && b <= 0 && cc <= 0);
      if (dedans_cone && 4 * sn > T2) return true;
    }
  }
  // (c) les trois arcs : projection de `s` sur le plan de l'arc, retenue
  // seulement si elle tombe DANS l'arc.
  for (int e = 0; e < 3; ++e) {
    const i64* gi = gv[e];
    const i64* gj = gv[(e + 1) % 3];
    const i128 ni2 = gn2[e], nj2 = gn2[(e + 1) % 3];
    const i128 gij = (i128)gi[0] * gj[0] + (i128)gi[1] * gj[1] + (i128)gi[2] * gj[2];
    const i128 si = (i128)s[0] * gi[0] + (i128)s[1] * gi[1] + (i128)s[2] * gi[2];
    const i128 sj = (i128)s[0] * gj[0] + (i128)s[1] * gj[1] + (i128)s[2] * gj[2];
    const i128 G = ni2 * nj2 - gij * gij;
    if (G <= 0) continue;
    if (nj2 * si - gij * sj < 0) continue;   // coefficient sur `gi` negatif
    if (ni2 * sj - gij * si < 0) continue;   // coefficient sur `gj` negatif
    i64 n[3];
    n[0] = gi[1] * gj[2] - gi[2] * gj[1];
    n[1] = gi[2] * gj[0] - gi[0] * gj[2];
    n[2] = gi[0] * gj[1] - gi[1] * gj[0];
    const i128 nn = (i128)n[0] * n[0] + (i128)n[1] * n[1] + (i128)n[2] * n[2];
    if (nn == 0) continue;
    const i128 sdn = (i128)s[0] * n[0] + (i128)s[1] * n[1] + (i128)s[2] * n[2];
    // `|P|^2 = (|s|^2 |n|^2 - (s.n)^2) / |n|^2` ; condition `4|P|^2 > T^2`.
    if (4 * (sn * nn - sdn * sdn) > T2 * nn) return true;
  }
  return false;
}

// Le certificat corrige. `POTENTIELLE` par intersection, partenaires GLOBAUX.
bool certifie_corrige(const Pt& x, const std::vector<Pt>& P, const DirGrid& g, i64 r,
                      int kmin, bool mutant_inclusion, Compteurs* cpt) {
  const size_t nc = g.cells.size();
  std::vector<int> depth(nc, 0);
  std::vector<char> pot(nc, 0);
  const i64 r2 = r * r;
  for (size_t idv = 0; idv < P.size(); ++idv) {
    const Pt& v = P[idv];
    i64 s[3];
    for (int k = 0; k < 3; ++k) s[k] = v.c[k] - x.c[k];
    const i64 ds = s[0] * s[0] + s[1] * s[1] + s[2] * s[2];
    if (ds == 0) continue;
    // `T = max(r, |s|)` : la comparaison se fait sur les carres.
    const i64 T = (ds >= r2) ? (i64)(std::sqrt((double)ds) + 1.0) : r;
    for (size_t c = 0; c < nc; ++c) {
      if (!pot[c]) {
        // LE MUTANT : marquer seulement quand les TROIS sommets sont dedans,
        // c'est-a-dire confondre inclusion et intersection.
        bool marque;
        if (mutant_inclusion) {
          marque = true;
          const i128 T2 = (i128)T * T;
          for (int k = 0; k < 3 && marque; ++k) {
            const auto& gv = g.vertices[(size_t)g.cells[c][(size_t)k]];
            const i128 dot = (i128)gv[0] * s[0] + (i128)gv[1] * s[1] + (i128)gv[2] * s[2];
            const i64 n2 = g.vnorm2[(size_t)g.cells[c][(size_t)k]];
            if (!(dot > 0 && 4 * dot * dot > T2 * n2)) marque = false;
          }
        } else {
          marque = cellule_intersecte(g, c, s, T);
        }
        if (marque) pot[c] = 1;
      }
      if (ds > r2) continue;   // une calotte INTERIEURE a l'echelle `r`
      bool couvre = true;
      for (int k = 0; k < 3 && couvre; ++k) {
        const auto& gv = g.vertices[(size_t)g.cells[c][(size_t)k]];
        const i128 dot = (i128)gv[0] * s[0] + (i128)gv[1] * s[1] + (i128)gv[2] * s[2];
        if (dot <= 0) { couvre = false; break; }
        const i64 n2 = g.vnorm2[(size_t)g.cells[c][(size_t)k]];
        if (!((i128)dot * dot * r2 > (i128)n2 * ds * ds)) couvre = false;
      }
      if (couvre) ++depth[c];
    }
  }
  bool ok = true;
  for (size_t c = 0; c < nc; ++c) {
    ++cpt->cellules;
    if (!pot[c]) continue;
    ++cpt->admissibles;
    if (depth[c] >= kmin) ++cpt->couvertes;
    else ok = false;
  }
  return ok;
}

std::vector<Pt> nuage(const std::string& f, long long n, long long coord, long long seed) {
  mhgp3v::CloudFamily fam;
  if (f == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (f == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (f == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (f == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (f == "scanline_overlap_multiecho")
    fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else refuse("famille inconnue : " + f);
  if (coord <= 0) coord = mhgp3v::cloud_family_default_coord(fam, (int)n);
  const auto b = mhgp3v::make_family_cloud(fam, (int)n, (int)coord, seed);
  std::vector<Pt> P;
  P.reserve(b.size());
  for (const auto& q : b) P.push_back(Pt{{q.x, q.y, q.z}});
  return P;
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  long long n = 800, coord = 0, seed = 1, kmin = 8, m = 4, threads = 0;
  double rayon_esp = 10.0, min_pct = 0.0;
  bool falsifie = false, corrige = false, mut_inclusion = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a == "--falsifie") falsifie = true;
    else if (a == "--fixtures-refutation") { return fixtures_refutation(); }
    else if (a == "--corrige") corrige = true;
    else if (a == "--inject=inclusion") mut_inclusion = true;
    else if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) n = atoll(val("--points=").c_str());
    else if (a.rfind("--coord=", 0) == 0) coord = atoll(val("--coord=").c_str());
    else if (a.rfind("--seed=", 0) == 0) seed = atoll(val("--seed=").c_str());
    else if (a.rfind("--kmin=", 0) == 0) kmin = atoll(val("--kmin=").c_str());
    else if (a.rfind("--m=", 0) == 0) m = atoll(val("--m=").c_str());
    else if (a.rfind("--rayon-espacements=", 0) == 0)
      rayon_esp = atof(val("--rayon-espacements=").c_str());
    else if (a.rfind("--min-certifies-pct=", 0) == 0)
      min_pct = atof(val("--min-certifies-pct=").c_str());
    else if (a.rfind("--threads=", 0) == 0) threads = atoll(val("--threads=").c_str());
    else refuse("option inconnue : " + a);
  }
  if (n < 20 || n > 20000) refuse("--points hors domaine");
  if (kmin < 2 || kmin > 16) refuse("--kmin hors domaine");
  if (m < 2 || m > 8) refuse("--m hors domaine");
  if (rayon_esp < 2.0 || rayon_esp > 64.0) refuse("--rayon-espacements hors domaine");
  if (falsifie && n > 400) refuse("--falsifie est borne a 400 points");
  if (threads <= 0) threads = (long long)std::thread::hardware_concurrency();
  if (threads <= 0) threads = 1;

  const std::vector<Pt> P = nuage(family, n, coord, seed);
  const int N = (int)P.size();
  i64 lo[3] = {INT64_MAX, INT64_MAX, INT64_MAX}, hi[3] = {INT64_MIN, INT64_MIN, INT64_MIN};
  for (const Pt& p : P)
    for (int c = 0; c < 3; ++c) {
      if (p.c[c] < lo[c]) lo[c] = p.c[c];
      if (p.c[c] > hi[c]) hi[c] = p.c[c];
    }
  double vol = 1.0;
  for (int c = 0; c < 3; ++c) vol *= (double)std::max<i64>(1, hi[c] - lo[c]);
  const double esp = std::cbrt(vol / (double)N);
  const i64 r = (i64)(rayon_esp * esp + 0.5);

  const DirGrid g = build_grid((int)m);
  std::vector<char> cert_tout((size_t)N, 0), cert_adm((size_t)N, 0);
  std::vector<Compteurs> cpt((size_t)threads);
  auto worker = [&](long long tid) {
    std::vector<int> vois;
    for (int i = (int)tid; i < N; i += (int)threads) {
      vois.clear();
      for (int j = 0; j < N; ++j)
        if (j != i && d2p(P[(size_t)i], P[(size_t)j]) <= r * r) vois.push_back(j);
      Compteurs muet;
      cert_tout[(size_t)i] =
          certifie(P[(size_t)i], P, vois, g, r, (int)kmin, false, &muet) ? 1 : 0;
      cert_adm[(size_t)i] =
          corrige ? (certifie_corrige(P[(size_t)i], P, g, r, (int)kmin, mut_inclusion,
                                      &cpt[(size_t)tid]) ? 1 : 0)
                  : (certifie(P[(size_t)i], P, vois, g, r, (int)kmin, true,
                              &cpt[(size_t)tid]) ? 1 : 0);
    }
  };
  std::vector<std::thread> th;
  for (long long t = 0; t < threads; ++t) th.emplace_back(worker, t);
  for (auto& t : th) t.join();
  Compteurs C;
  for (const Compteurs& c : cpt) {
    C.cellules += c.cellules; C.admissibles += c.admissibles; C.couvertes += c.couvertes;
  }
  long long nt = 0, na = 0;
  for (int i = 0; i < N; ++i) { nt += cert_tout[(size_t)i]; na += cert_adm[(size_t)i]; }
  std::printf("caps_admissible : mode=%s famille=%s n=%d m=%lld cellules=%zu kmin=%lld"
              " rayon=%lld (%.1f espacements)\n",
              corrige ? (mut_inclusion ? "corrige_mutant" : "corrige") : "refute",
              family.c_str(), N, m, g.cells.size(), kmin, (long long)r, rayon_esp);
  std::printf("  certificats : toutes_cellules=%lld (%.2f%%) admissibles=%lld (%.2f%%)"
              " gain=%.2f pts\n",
              nt, 100.0 * (double)nt / N, na, 100.0 * (double)na / N,
              100.0 * (double)(na - nt) / N);
  std::printf("  cellules : total=%lld admissibles=%lld (%.2f%%) couvertes=%lld\n",
              C.cellules, C.admissibles,
              100.0 * (double)C.admissibles / (double)std::max(1LL, C.cellules),
              C.couvertes);

  if (falsifie) {
    // LA FALSIFICATION. Tout support positif d'arite deux, trois ou quatre dont
    // un sommet est certifie doit avoir un diametre STRICTEMENT inferieur au
    // rayon de ce sommet. Un seul depassement refute le theoreme.
    long long testes = 0, viole = 0;
    for (int i = 0; i < N; ++i)
      for (int j = i + 1; j < N; ++j) {
        const i64 D2 = d2p(P[(size_t)i], P[(size_t)j]);
        if (D2 == 0) continue;
        // Rang de la boule diametrale : c'est le support q2.
        long long ins = 0;
        for (int z = 0; z < N && ins < kmin; ++z) {
          if (z == i || z == j) continue;
          i128 s = 0;
          for (int c = 0; c < 3; ++c)
            s += (i128)(P[(size_t)z].c[c] - P[(size_t)i].c[c]) *
                 (P[(size_t)z].c[c] - P[(size_t)j].c[c]);
          if (s < 0) ++ins;
        }
        if (ins >= kmin) continue;
        ++testes;
        if ((cert_adm[(size_t)i] || cert_adm[(size_t)j]) && D2 >= r * r) ++viole;
      }
    // LE VRAI OBJET EST q4. Le brute force `C(n,4)` est borne a 160 points ;
    // au-dela la porte ne teste que q2 et le dit.
    long long t4 = 0, v4 = 0;
    if (N <= 160) {
      for (int i = 0; i < N; ++i)
        for (int j = i + 1; j < N; ++j)
          for (int k = j + 1; k < N; ++k)
            for (int l = k + 1; l < N; ++l) {
              const i64* q[4] = {P[(size_t)i].c, P[(size_t)j].c, P[(size_t)k].c,
                                 P[(size_t)l].c};
              if (mhgp3v::c8::orient3d(q[0], q[1], q[2], q[3]) == 0) continue;
              if (!mhgp3v::c8::bien_centre(q[0], q[1], q[2], q[3])) continue;
              long long ins = 0;
              for (int z = 0; z < N && ins < kmin; ++z) {
                if (z == i || z == j || z == k || z == l) continue;
                if (mhgp3v::c8::interieur_strict(q[0], q[1], q[2], q[3], P[(size_t)z].c))
                  ++ins;
              }
              if (ins >= kmin) continue;
              // LE DIAMETRE DE LA BOULE, PAS LA PLUS GRANDE ARETE. Le
              // tetraedre regulier a des aretes `sqrt(8)` et une circumsphere
              // de diametre `sqrt(12)` : comparer l'arete a `r` laisserait
              // passer un support dont la boule depasse `r`.
              const int id[4] = {i, j, k, l};
              ++t4;
              bool un_certifie = false;
              for (int u = 0; u < 4; ++u) un_certifie = un_certifie || cert_adm[(size_t)id[u]];
              if (un_certifie &&
                  cmp_4R2_a_r2(P[(size_t)i], P[(size_t)j], P[(size_t)k], P[(size_t)l], r) >= 0)
                ++v4;
            }
    }
    std::printf("falsifie : supports_q2_testes=%lld violations=%lld"
                " supports_q4_testes=%lld violations_q4=%lld\n", testes, viole, t4, v4);
    viole += v4;
    if (testes == 0) { std::fprintf(stderr, "PLANCHER: aucun support teste\n"); return 3; }
    if (viole > 0) {
      std::fprintf(stderr, "REFUTATION: %lld supports depassent le rayon certifie\n", viole);
      return 1;
    }
    return 0;
  }
  if (C.admissibles == 0) { std::fprintf(stderr, "PLANCHER: aucune cellule admissible\n"); return 3; }
  if (100.0 * (double)na / N < min_pct) {
    std::fprintf(stderr, "PLANCHER: %.2f%% d'ancres certifiees < %.2f%%\n",
                 100.0 * (double)na / N, min_pct);
    return 3;
  }
  return 0;
}
