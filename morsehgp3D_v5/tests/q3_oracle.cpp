// MorseHGP3D v5 — ORACLE RATIONNEL INDEPENDANT DE LA LANE q3.
//
// Sujet : src/lanes/q3.hpp (is_acute_seed, anchor_owns_q3, q3_form, q3_power,
// q3_ball_key, q3_ball_depth, q3_exact_level). Juge : ce fichier, en OBig<12>
// (oracle/obig.hpp, limbes de 32 bits en signe-magnitude — jamais U192/U320
// ni i128 de decision de la production), avec ses PROPRES primitives
// osub/odot/onorm2/ocross ; ni p3_dot, ni p3_cross, ni BallKey::power
// n'entrent dans une decision de l'oracle. Les points sont recus en
// OraclePoint{id, pos} (l'id est une BIJECTION du rang, avec des valeurs
// au-dessus du bit 31), jamais l'ordre interne de CloudIndex.
//
// Pour de petits nuages (familles uniform / eight_clusters, n ~ 60-120) et
// les fixtures gravees, sur TOUS les triangles {p<q<r} du nuage :
//
//   1. OWNER : arete de longueur carree maximale, ex aequo departages par la
//      plus petite paire (min id, max id) lexicographique — recalcule
//      localement. Le sujet doit repondre `anchor_owns_q3` = vrai sur cette
//      arete et SEULEMENT elle (trois appels, exactement un vrai).
//   2. ACUITE : les TROIS produits scalaires d'angles > 0 (jamais V² > D²).
//      Le sujet doit repondre `is_acute_seed` = vrai sur (owner, apex) si et
//      seulement si le triangle est aigu, et faux sur les deux autres
//      affectations (le seed est canonique : au plus UNE representation).
//   3. CIRCUMCENTRE par Cramer 3×3 DIRECT sur le systeme
//        2(b-a)·c = |b|²-|a|²,  2(x-a)·c = |x|²-|a|²,  n·c = n·a,
//      n = (b-a)×(x-a) : c = N/det, det = det[2e1 ; 2e2 ; n]. Identite de
//      Lagrange : det = 2e1·(2e2×n) = 4 e1·(e2×(e1×e2)) = 4(|e1|²|e2|² -
//      (e1·e2)²) = 4|e1×e2|² = 4 G_o — verifiee en OBig sur chaque triangle
//      (un oracle qui ne se verifie pas lui-meme n'est pas un juge).
//   4. INTERIEUR / COQUILLE de chaque autre point z par comparaison
//      |z·det - N|² vs |a·det - N|² (distance homogene au centre rationnel) :
//      < interieur strict, = coquille, > exterieur. Compare au sujet POINT PAR
//      POINT (signe de q3_power) puis en COMPTE (q3_ball_depth : profondeur
//      stricte, coquilles, liste des interieurs en PointId externes quand la
//      profondeur <= 8).
//   5. NIVEAU EXACT (rayon au carre) par l'egalite croisee
//        |a·det - N|² · (4 G_o) == D·E·X · det²,
//      avec D = |b-a|², E = |x-a|², X = |b-x|² calcule DIRECTEMENT et
//      G_o = |(b-a)×(x-a)|² (pas DE-F²) : R = |ab||ax||bx| / (4·Aire),
//      16·Aire² = 4·G_o, donc R² = D·E·X / (4 G_o), et |a - c|² =
//      |a·det - N|² / det². PUIS le niveau public du sujet (Rational128
//      canonique num/den) : num · det² == den · |a·det - N|².
//   6. BALLKEY PRIMITIVE du sujet contre la forme oracle A_o = det²,
//      B_o = -2·det·N, C_o = |N|² - |a·det - N|², comparees PROJECTIVEMENT
//      (produits croises ; les deux formes ont un coefficient de tete > 0).
//   7. ORDRE des niveaux publics (compare_rational, U192) contre les produits
//      croises OBig, toutes paires des nuages graves et fenetre glissante sur
//      les familles : c'est le seul chemin de ce juge qui traverse
//      src/core/wide.hpp (mutant `level-trunc-hi`).
//
// LARGEURS (re-derivees, profil u16, deltas |e_i| < 2^16 par composante) :
//   lignes 2e_i < 2^17, n_i < 2^33 ; seconds membres < 2^34, 2^34, n·a < 2^52 ;
//   det = 4 G_o < 4·3·2^66 < 2^70 ; N_i (six termes de cofacteurs) < 2^88 ;
//   z·det - N < 2^89, |z·det - N|² < 2^180 ; produit du niveau < 2^250 ;
//   ballkey : a_k·C_o < 2^248. Tout < 2^384 avec une marge > 2^130. Un
//   debordement leve le drapeau collant fail-closed de l'oracle => code 3.
//
// Fixtures u16 EXTREMES (coordonnees exactes reprises de la v4) : triangle
// equilateral entier a l'echelle maximale (cinq points cospheriques a
// M = 65535, aretes 2M², owner departage par les seuls ids), triangle presque
// rectangle mais aigu (produit scalaire a l'apex 40001 sur des aretes de
// 1,6·10^9 : marge de Thales minimale), grande cosphere de rayon 20000
// centree (32768, 32768, 32768) (24 points de coquille + centre + interieur),
// petite cosphere et tetraedre (coquilles a petite echelle), et rejeu du
// triangle presque rectangle translate au bord de la grille (max = 65535
// sur chaque axe) ; plus une grille large GENERIQUE (dix points quelconques
// sur toute l'etendue u16, niveaux reduits larges : la seule fixture dont les
// produits croises de niveaux q3 depassent 2^128). Fixture owner au-dessus
// du bit 31 sur anchor_owns_q3.
//
// Limbes hauts MESURES (plus haut limbe u32 non nul de det, N, |z·det-N|²,
// produit du niveau) et PLANCHER : le produit du niveau atteint >= 192 bits
// (--min-level-bits, defaut 192) ; planchers de non-vacuite --min-acute,
// --min-events, --min-shell, --min-deep, --min-order-pairs.
//
// Codes : 0 accord total ; 1 desaccord (une fixture a graver, jamais a
// cacher) ; 2 refus avant calcul ; 3 plancher / invariant / debordement de
// l'oracle ; 4 mutant tue. Mutants (--inject=<nom>, registre unique
// src/core/mutants.hpp) :
//   du SUJET : q3-prune-ge (elagage mn >= 0 : perd les coquilles), q3-level-4g
//   (denominateur G au lieu de 4G), level-trunc-hi (mot haut de U192 a zero,
//   porte 7) ; de l'ARITHMETIQUE DE L'ORACLE : obig-carry-lost (retenue jetee
//   au-dela de 2^160, tue par les produits du niveau et du ballkey) ;
//   de l'ORACLE LUI-MEME (points d'injection DANS CE FICHIER, le sujet v5 n'a
//   pas de mutant de signe ni de Cramer) : q3-sign-p (signe de P inverse dans
//   la classification oracle) et q3-cramer-swap (deux numerateurs de Cramer
//   echanges) — ils prouvent que la porte discrimine un oracle FAUX, pas
//   seulement un sujet faux.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../oracle/obig.hpp"
#include "../src/cloud/families.hpp"
#include "../src/core/mutants.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/spindle/spindle.hpp"

using namespace mhgp5;

namespace {

using OB = mhgp5_oracle::OBig384;

OB ob(i64 v) { return OB::from_i64(v); }
OB ob128(i128 v) { return OB::from_i128((mhgp5_oracle::oi128)v); }

// ---- Primitives LOCALES de l'oracle : jamais p3_dot / p3_cross / p3_norm2
// de la production dans une decision de l'oracle. P3 n'est qu'un conteneur.
P3 osub(const P3& a, const P3& b) { return P3{a.x - b.x, a.y - b.y, a.z - b.z}; }
i64 odot(const P3& a, const P3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
i64 onorm2(const P3& a) { return odot(a, a); }
P3 ocross(const P3& a, const P3& b) {
  return P3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
int osign(i128 v) { return v < 0 ? -1 : (v > 0 ? 1 : 0); }

// ---- Compteurs de limbes (u32, 0..11) : plus haut limbe non nul observe.
struct LimbStats {
  int det = -1, num = -1, dist2 = -1, level = -1;
  int level_bits = 0;
};
LimbStats g_limbs;
void bump(int& g, const OB& v) { g = std::max(g, v.top_limb()); }

// ---- Points de l'oracle : identite externe + position, jamais un rang.
struct OraclePoint {
  PointId id;
  P3 pos;
};

// Arete de l'oracle : (min id, max id), ordre lexicographique — recalcule
// localement, pas EdgeKey.
struct OEdge {
  PointId lo, hi;
};
OEdge oedge(PointId x, PointId y) { return x < y ? OEdge{x, y} : OEdge{y, x}; }
bool oedge_less(const OEdge& a, const OEdge& b) { return a.lo != b.lo ? a.lo < b.lo : a.hi < b.hi; }

struct OracleBall {
  OB det;     // det[2e1 ; 2e2 ; n] = 4 G_o > 0 pour un triangle non degenere
  OB num[3];  // c = num / det
};

OB det3(const OB m[3][3]) {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

// Cramer 3×3 direct en OBig, colonnes remplacees une a une.
OracleBall oracle_circumball(const P3& a, const P3& b, const P3& x) {
  const P3 e1 = osub(b, a);
  const P3 e2 = osub(x, a);
  const P3 n = ocross(e1, e2);
  const i64 rows[3][3] = {{2 * e1.x, 2 * e1.y, 2 * e1.z}, {2 * e2.x, 2 * e2.y, 2 * e2.z}, {n.x, n.y, n.z}};
  const i128 rhs[3] = {(i128)onorm2(b) - onorm2(a), (i128)onorm2(x) - onorm2(a),
                       (i128)n.x * a.x + (i128)n.y * a.y + (i128)n.z * a.z};
  OB m[3][3];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) m[i][j] = ob(rows[i][j]);
  OracleBall res;
  res.det = det3(m);
  for (int col = 0; col < 3; ++col) {
    OB mc[3][3];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) mc[i][j] = (j == col) ? ob128(rhs[i]) : ob(rows[i][j]);
    res.num[col] = det3(mc);
  }
  if (MHGP5_MUTANT("q3-cramer-swap")) {  // MUTANT DE L'ORACLE : deux numerateurs du centre echanges
    const OB t = res.num[0];
    res.num[0] = res.num[1];
    res.num[1] = t;
  }
  bump(g_limbs.det, res.det);
  for (int col = 0; col < 3; ++col) bump(g_limbs.num, res.num[col]);
  return res;
}

// |p·det - N|² : carre de distance homogene au centre rationnel.
OB oracle_dist2(const OracleBall& bl, const P3& p) {
  const i64 c[3] = {p.x, p.y, p.z};
  OB s = ob(0);
  for (int i = 0; i < 3; ++i) {
    const OB d = ob(c[i]) * bl.det - bl.num[i];
    s = s + d * d;
  }
  bump(g_limbs.dist2, s);
  return s;
}

// Signe de |z-c|² - R² (R = distance de c au sommet a) : -1 interieur strict,
// 0 coquille, +1 exterieur.
int oracle_power_sign(const OracleBall& bl, const OB& dist2_a, const P3& z) {
  int s = cmp(oracle_dist2(bl, z), dist2_a);
  if (MHGP5_MUTANT("q3-sign-p")) s = -s;  // MUTANT DE L'ORACLE : signe de P inverse
  return s;
}

int g_disagreements = 0;
void report(const char* what, const OraclePoint& p, const OraclePoint& q, const OraclePoint& r) {
  if (g_disagreements < 40)
    std::fprintf(stderr, "DESACCORD %s sur le triangle ids (%u,%u,%u) = (%lld,%lld,%lld) (%lld,%lld,%lld) (%lld,%lld,%lld)\n",
                 what, (unsigned)p.id, (unsigned)q.id, (unsigned)r.id, (long long)p.pos.x, (long long)p.pos.y,
                 (long long)p.pos.z, (long long)q.pos.x, (long long)q.pos.y, (long long)q.pos.z, (long long)r.pos.x,
                 (long long)r.pos.y, (long long)r.pos.z);
  ++g_disagreements;
}

// ---- Fixtures gravees (coordonnees exactes de la v4) --------------------------

// Cosphere : permutations signees du motif autour du centre (cx,cx,cx), plus
// le centre et un point interieur excentre. Les points hors grille (x < 0)
// sont ecartes (petite cosphere a cx = 4).
std::vector<P3> cosphere_cloud(i64 cx, const i64 pat[3], const P3& inner) {
  const int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  std::vector<P3> pts;
  std::set<long long> seen;
  for (const auto& pr : perm)
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
          const i64 x = cx + sx * pat[pr[0]], y = cx + sy * pat[pr[1]], z = cx + sz * pat[pr[2]];
          if (x < 0 || y < 0 || z < 0) continue;
          const long long key = (x << 34) | (y << 17) | z;
          if (!seen.insert(key).second) continue;
          pts.push_back(P3{x, y, z});
        }
  pts.push_back(P3{cx, cx, cx});
  pts.push_back(inner);
  return pts;
}

std::vector<P3> tetra_cloud() { return {{0, 0, 0}, {2, 2, 0}, {2, 0, 2}, {0, 2, 2}, {9, 9, 9}, {1, 9, 3}}; }

// Triangle equilateral entier a l'echelle maximale : cinq points cospheriques
// (centre (M/2,M/2,M/2), R² = 3M²/4), toutes les aretes du triangle de base
// valent 2M² — owner departage par les seuls ids sous les plus grands
// coefficients de la grille ; sixieme point interieur.
std::vector<P3> equilateral_max_cloud() {
  const i64 M = 65535;
  return {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}, {M, M, M}, {30000, 20000, 10000}};
}

// Triangle presque rectangle mais strictement aigu : produit scalaire a
// l'apex (0-20000)(40000-20000) + (0-20001)(0-20001) = -4·10^8 + 400040001 =
// 40001 > 0 sur des aretes de 1,6·10^9. Un point profond et un point externe.
// `t` translate le nuage (rejeu au bord de la grille : max 65535 par axe).
std::vector<P3> near_right_cloud(const P3& t) {
  return {{t.x + 0, t.y + 0, t.z + 0},
          {t.x + 40000, t.y + 0, t.z + 0},
          {t.x + 20000, t.y + 20001, t.z + 0},
          {t.x + 20000, t.y + 10000, t.z + 1000},
          {t.x + 1000, t.y + 19000, t.z + 2000}};
}

// Grille large GENERIQUE : dix points a coordonnees quelconques sur toute
// l'etendue u16 (aucune symetrie, aucun alignement voulu). Leurs niveaux
// publics REDUITS restent larges (num ~ 2^99, den ~ 2^68 : pgcd petit), donc
// les produits croises de la porte 7 traversent le mot haut de U192
// (> 2^128) — c'est ce qui rend `level-trunc-hi` discriminable par la lane
// q3 seule (les cospheres et l'equilateral maximal se reduisent a des
// fractions minuscules : R² = 4·10^8 / 1, 3M²/4).
std::vector<P3> wide_generic_cloud() {
  return {{12345, 54321, 6789}, {65535, 1, 32768},    {777, 65000, 4242},  {40001, 20002, 60003}, {31337, 3, 65534},
          {2, 44444, 22222},    {55555, 55555, 5},     {9999, 9, 49999},    {60000, 30001, 15000}, {23456, 65535, 34567}};
}

// Identite externe : bijection du rang par multiplication impaire modulo
// 2^32 (distincte pour des rangs distincts), valeurs au-dessus du bit 31.
PointId id_of_rank(size_t rank) { return (PointId)(0x9E3779B1u * (u32)(rank + 1)); }

struct Args {
  bool ok = true;
  CloudFamily family = CloudFamily::kUniform;
  int n = 80;
  int coord = 0;
  long long seed = 3;
  int window = 48;  // fenetre de la porte 7 sur les familles
  u64 min_acute = 1, min_events = 1, min_shell = 1, min_deep = 1, min_order_pairs = 1;
  int min_level_bits = 192;
  std::string inject;
};

Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--family=")) a.ok = parse_cloud_family(v, &a.family) && a.ok;
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--window=")) a.window = std::atoi(v);
    else if (const char* v = val("--min-acute=")) a.min_acute = (u64)std::atoll(v);
    else if (const char* v = val("--min-events=")) a.min_events = (u64)std::atoll(v);
    else if (const char* v = val("--min-shell=")) a.min_shell = (u64)std::atoll(v);
    else if (const char* v = val("--min-deep=")) a.min_deep = (u64)std::atoll(v);
    else if (const char* v = val("--min-order-pairs=")) a.min_order_pairs = (u64)std::atoll(v);
    else if (const char* v = val("--min-level-bits=")) a.min_level_bits = std::atoi(v);
    else if (const char* v = val("--inject=")) a.inject = v;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.ok = false;
    }
  }
  return a;
}

// Niveau public d'un triangle aigu, retenu pour la porte 7 (ordre).
struct LevelRec {
  Rational128 rat;
  OB num, den;
};

struct Counters {
  u64 triangles = 0, acute = 0, events = 0, shell_supports = 0, deep = 0, order_pairs = 0;
  std::set<BallKey> degenerate_keys;
};

// Fixture owner AU-DESSUS DU BIT 31 : triangle equilateral entier (0,0,0),
// (1,1,0), (1,0,1) — trois aretes de longueur carree 2 : l'owner est choisi
// PAR la seule paire d'ids minimale. Un PointId signe i32 inverserait
// l'ordre au-dessus de 2^31 et deplacerait l'owner.
void owner_bit31_fixture() {
  const PointId H = (PointId)1u << 31;
  struct OwnerCase {
    PointId ia, ib, ix;
    int owner;  // 0 = arete ab, 1 = ax, 2 = bx
  };
  const OwnerCase cases[] = {
      {7, 3, 5, 2},                      // (3,5) minimale : bx
      {H + 7, H + 3, 5, 2},              // (5, 2^31+3) minimale : bx
      {5, H + 3, H + 7, 0},              // (5, 2^31+3) minimale : ab
      {0xFFFFFFFEu, 1, 0xFFFFFFFFu, 0},  // (1, 2^32-2) minimale : ab
  };
  for (const OwnerCase& c : cases) {
    const bool owns[3] = {anchor_owns_q3(2, 2, 2, c.ia, c.ib, c.ix), anchor_owns_q3(2, 2, 2, c.ia, c.ix, c.ib),
                          anchor_owns_q3(2, 2, 2, c.ib, c.ix, c.ia)};
    int owner = -1, count = 0;
    for (int t = 0; t < 3; ++t)
      if (owns[t]) {
        owner = t;
        ++count;
      }
    if (count != 1 || owner != c.owner) {
      std::fprintf(stderr, "DESACCORD fixture owner bit31 : ids (%u,%u,%u) -> owner %d (attendu %d, %d gagnants)\n",
                   (unsigned)c.ia, (unsigned)c.ib, (unsigned)c.ix, owner, c.owner, count);
      ++g_disagreements;
    }
  }
}

// Juge un nuage entier : tous les triangles {p<q<r} de la liste OraclePoint.
// Rend false sur un refus (positions dupliquees ou index invalide).
bool judge_cloud(const std::vector<P3>& pts, bool engraved, const Args& a, Counters* ct) {
  const size_t n = pts.size();
  std::vector<InputPoint> in(n);
  std::vector<OraclePoint> op(n);
  for (size_t p = 0; p < n; ++p) {
    op[p] = OraclePoint{id_of_rank(p), pts[p]};
    in[p] = InputPoint{op[p].id, pts[p]};
  }
  const CloudIndex ix = build_cloud_index(in);
  if (!ix.valid || (size_t)ix.unique_count() != n) return false;
  const int m = ix.unique_count();
  // Rang -> index unique du sujet, par l'identite externe (jamais par cast).
  std::map<PointId, i32> u_of_id;
  for (i32 u = 0; u < m; ++u) u_of_id[ix.point_id(u)] = u;
  std::vector<i32> uidx(n);
  for (size_t p = 0; p < n; ++p) {
    const auto it = u_of_id.find(op[p].id);
    if (it == u_of_id.end()) return false;
    uidx[p] = it->second;
  }
  const u64 smax = 11;
  const u64 h3 = lane_h(Lane::kQ3, std::min<u64>(smax, (u64)m));
  std::vector<LevelRec> levels;

  for (size_t p = 0; p < n; ++p)
    for (size_t q = p + 1; q < n; ++q)
      for (size_t r = q + 1; r < n; ++r) {
        ++ct->triangles;
        const OraclePoint &P = op[p], &Q = op[q], &R = op[r];
        // Trois affectations (arete, apex) : t = 0 : (p,q|r), 1 : (p,r|q), 2 : (q,r|p).
        const size_t e0[3] = {p, p, q}, e1[3] = {q, r, r}, ap[3] = {r, q, p};
        i64 len[3];
        for (int t = 0; t < 3; ++t) len[t] = onorm2(osub(op[e0[t]].pos, op[e1[t]].pos));
        // 1. Owner oracle : longueur maximale, ex aequo par la plus petite paire d'ids.
        int to = 0;
        for (int t = 1; t < 3; ++t) {
          const bool better = len[t] > len[to] ||
                              (len[t] == len[to] && oedge_less(oedge(op[e0[t]].id, op[e1[t]].id),
                                                                oedge(op[e0[to]].id, op[e1[to]].id)));
          if (better) to = t;
        }
        // 2. Acuite oracle : trois produits scalaires d'angles > 0.
        const bool o_acute = odot(osub(Q.pos, P.pos), osub(R.pos, P.pos)) > 0 &&
                             odot(osub(P.pos, Q.pos), osub(R.pos, Q.pos)) > 0 &&
                             odot(osub(P.pos, R.pos), osub(Q.pos, R.pos)) > 0;
        // Sujet : owner unique et seed canonique.
        int s_owner = -1, s_owner_count = 0, s_seed = -1, s_seed_count = 0;
        for (int t = 0; t < 3; ++t) {
          const OraclePoint &A = op[e0[t]], &B = op[e1[t]], &X = op[ap[t]];
          const i64 lax = onorm2(osub(X.pos, A.pos)), lbx = onorm2(osub(X.pos, B.pos));
          if (anchor_owns_q3(len[t], lax, lbx, A.id, B.id, X.id)) {
            s_owner = t;
            ++s_owner_count;
          }
          if (is_acute_seed(A.pos, B.pos, X.pos, len[t], A.id, B.id, X.id)) {
            s_seed = t;
            ++s_seed_count;
          }
        }
        if (s_owner_count != 1 || s_owner != to) {
          report("owner (anchor_owns_q3 : exactement une arete, la maximale)", P, Q, R);
          continue;
        }
        if (s_seed_count != (o_acute ? 1 : 0) || (o_acute && s_seed != to)) {
          report("acuite / seed canonique (is_acute_seed)", P, Q, R);
          continue;
        }
        if (!o_acute) continue;
        ++ct->acute;

        const OraclePoint &A = op[e0[to]], &B = op[e1[to]], &X = op[ap[to]];
        // 3. Circumcentre oracle et identite de Lagrange det = 4 G_o.
        const OracleBall bl = oracle_circumball(A.pos, B.pos, X.pos);
        const P3 d = osub(B.pos, A.pos), u = osub(X.pos, A.pos), bx = osub(B.pos, X.pos);
        const P3 cr = ocross(d, u);
        const OB g_o = ob(cr.x) * ob(cr.x) + ob(cr.y) * ob(cr.y) + ob(cr.z) * ob(cr.z);
        if (bl.det.sign() <= 0 || cmp(bl.det, ob(4) * g_o) != 0) {
          report("oracle : det != 4|d×u|² (identite de Lagrange)", P, Q, R);
          continue;
        }
        const OB dist2_a = oracle_dist2(bl, A.pos);

        // 4. Classification point par point : oracle contre q3_power, puis
        // comptes contre q3_ball_depth.
        const Q3Form f = q3_form(A.pos, B.pos, X.pos);
        const BallKey bk = q3_ball_key(f);
        u64 o_depth = 0, o_shell = 0;
        std::vector<PointId> o_interior;
        bool sign_ok = true, ballkey_sign_ok = true;
        for (size_t z = 0; z < n; ++z) {
          if (z == p || z == q || z == r) continue;
          const int sg = oracle_power_sign(bl, dist2_a, op[z].pos);
          if (sg < 0) {
            ++o_depth;
            o_interior.push_back(op[z].id);
          } else if (sg == 0) {
            ++o_shell;
          }
          const i128 pw = q3_power(f, op[z].pos);
          if (osign(pw) != sg) sign_ok = false;
          if (osign(bk.power(op[z].pos)) != osign(pw)) ballkey_sign_ok = false;
        }
        if (!sign_ok) {
          report("signe de q3_power", P, Q, R);
          continue;
        }
        if (!ballkey_sign_ok) {
          report("invariant sujet : signe de BallKey::power != signe de q3_power", P, Q, R);
          continue;
        }
        u64 s_shell = 0;
        i32 s_int[8];
        u8 s_ni = 0;
        const u64 s_depth = q3_ball_depth(ix, f, uidx[e0[to]], uidx[e1[to]], uidx[ap[to]], (u64)m, &s_shell, s_int, &s_ni);
        if (s_depth != o_depth) {
          report("profondeur (q3_ball_depth)", P, Q, R);
          continue;
        }
        if (s_shell != o_shell) {
          report("coquille (q3_ball_depth)", P, Q, R);
          continue;
        }
        if (o_depth <= 8) {
          std::vector<PointId> s_ids;
          for (u8 t = 0; t < s_ni; ++t) s_ids.push_back(ix.point_id(s_int[t]));
          std::sort(s_ids.begin(), s_ids.end());
          std::sort(o_interior.begin(), o_interior.end());
          if (s_ids != o_interior) {
            report("liste des interieurs (PointId externes)", P, Q, R);
            continue;
          }
          if (o_depth >= 1) ++ct->deep;
        }

        // 5. Niveau exact : |a·det - N|²·(4 G_o) == D·E·X·det², puis le
        // niveau public du sujet.
        const OB D = ob(onorm2(d)), E = ob(onorm2(u)), Xn = ob(onorm2(bx));
        const OB lhs = dist2_a * (ob(4) * g_o);
        const OB rhs = D * E * Xn * (bl.det * bl.det);
        bump(g_limbs.level, lhs);
        bump(g_limbs.level, rhs);
        g_limbs.level_bits = std::max(g_limbs.level_bits, std::max(lhs.bit_length(), rhs.bit_length()));
        if (cmp(lhs, rhs) != 0) {
          report("niveau exact (oracle : Cramer contre D·E·X/(4G))", P, Q, R);
          continue;
        }
        const Rational128 lv = q3_exact_level(A.pos, B.pos, X.pos);
        if (lv.num <= 0 || lv.den <= 0 || cmp(ob128(lv.num) * (bl.det * bl.det), ob128(lv.den) * dist2_a) != 0) {
          report("niveau public (q3_exact_level)", P, Q, R);
          continue;
        }
        levels.push_back(LevelRec{lv, ob128(lv.num), ob128(lv.den)});

        // 6. BallKey primitive, projectivement.
        {
          const OB A_o = bl.det * bl.det;
          bool ok = bk.a > 0;
          OB nn = ob(0);
          for (int t = 0; t < 3; ++t) {
            const OB B_ot = ob(-2) * (bl.det * bl.num[t]);
            if (cmp(A_o * ob128(bk.b[t]), ob128(bk.a) * B_ot) != 0) ok = false;
            nn = nn + bl.num[t] * bl.num[t];
          }
          const OB C_o = nn - dist2_a;
          if (cmp(A_o * ob128(bk.c), ob128(bk.a) * C_o) != 0) ok = false;
          if (!ok) {
            report("ballkey (forme primitive contre det², -2·det·N, |N|² - |a·det-N|²)", P, Q, R);
            continue;
          }
        }
        if (o_depth < h3 && o_shell == 0) ++ct->events;
        if (o_shell > 0) {
          ++ct->shell_supports;
          ct->degenerate_keys.insert(bk);
        }
      }

  // 7. Ordre des niveaux publics : compare_rational (U192) contre OBig.
  const size_t L = levels.size();
  for (size_t i = 0; i < L; ++i) {
    const size_t j0 = engraved ? 0 : (i > (size_t)a.window ? i - (size_t)a.window : 0);
    for (size_t j = j0; j < i; ++j) {
      const int got = compare_rational(levels[i].rat, levels[j].rat);
      const int want = cmp(levels[i].num * levels[j].den, levels[j].num * levels[i].den);
      ++ct->order_pairs;
      if (got != want || compare_rational(levels[j].rat, levels[i].rat) != -want) {
        if (g_disagreements < 40)
          std::fprintf(stderr, "DESACCORD ordre des niveaux (compare_rational) : got=%d want=%d\n", got, want);
        ++g_disagreements;
      }
    }
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  const Args a = parse(argc, argv);
  if (!a.ok || a.n < 4 || a.window < 1 || a.min_level_bits < 0) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  if (!a.inject.empty() && !mutants_enable(a.inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", a.inject.c_str());
    return 2;
  }
  const bool mutant = !a.inject.empty();
  const int coord = a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n);
  const std::vector<P3> fam = make_family_cloud(a.family, a.n, coord, a.seed);
  if ((int)fam.size() < 4) {
    std::fprintf(stderr, "REFUS : la famille n'a produit que %zu points\n", fam.size());
    return 2;
  }
  mhgp5_oracle::clear_overflow();

  owner_bit31_fixture();

  const i64 small_pat[3] = {3, 4, 0};
  const i64 big_pat[3] = {12000, 16000, 0};
  struct Cloud {
    const char* name;
    std::vector<P3> pts;
    bool engraved;
  };
  const std::vector<Cloud> clouds = {
      {"famille", fam, false},
      {"petite_cosphere", cosphere_cloud(4, small_pat, P3{1, 1, 1}), true},
      {"tetraedre", tetra_cloud(), true},
      {"equilateral_max", equilateral_max_cloud(), true},
      {"grande_cosphere", cosphere_cloud(32768, big_pat, P3{30000, 30000, 30000}), true},
      {"near_right", near_right_cloud(P3{0, 0, 0}), true},
      {"near_right_bord", near_right_cloud(P3{25535, 45534, 63535}), true},
      {"grille_large", wide_generic_cloud(), true},
  };
  Counters ct;
  for (const Cloud& c : clouds) {
    for (const P3& p : c.pts)
      if (!p3_in_profile(p)) {
        std::fprintf(stderr, "REFUS : nuage %s hors profil u16\n", c.name);
        return 2;
      }
    if (!judge_cloud(c.pts, c.engraved, a, &ct)) {
      std::fprintf(stderr, "REFUS : nuage %s a positions dupliquees ou index invalide\n", c.name);
      return 2;
    }
  }

  std::printf(
      "q3_oracle : famille=%s n=%d coord=%d nuages=%zu triangles=%llu aigus=%llu evenements=%llu "
      "supports_a_coquille=%llu ballkeys_degenerees_uniques=%zu profonds=%llu paires_ordre=%llu desaccords=%d\n",
      cloud_family_name(a.family), a.n, coord, clouds.size(), (unsigned long long)ct.triangles,
      (unsigned long long)ct.acute, (unsigned long long)ct.events, (unsigned long long)ct.shell_supports,
      ct.degenerate_keys.size(), (unsigned long long)ct.deep, (unsigned long long)ct.order_pairs, g_disagreements);
  std::printf("limbes max (u32, 0..11) : det=%d num=%d dist2=%d niveau=%d ; bits du produit du niveau=%d\n",
              g_limbs.det, g_limbs.num, g_limbs.dist2, g_limbs.level, g_limbs.level_bits);

  if (mhgp5_oracle::overflow_seen()) {
    std::fprintf(stderr, "REFUS numeric_failure : debordement de l'oracle OBig (fail-closed)\n");
    return 3;
  }
  if (mutant) {
    if (g_disagreements > 0) {
      std::printf("MUTANT TUE : %s\n", a.inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s non discrimine\n", a.inject.c_str());
    return 3;
  }
  if (g_disagreements > 0) return 1;
  if (ct.acute < a.min_acute || ct.events < a.min_events || ct.shell_supports < a.min_shell || ct.deep < a.min_deep ||
      ct.order_pairs < a.min_order_pairs) {
    std::fprintf(stderr,
                 "PLANCHER : aigus=%llu (>= %llu), evenements=%llu (>= %llu), supports_a_coquille=%llu (>= %llu), "
                 "profonds=%llu (>= %llu), paires_ordre=%llu (>= %llu)\n",
                 (unsigned long long)ct.acute, (unsigned long long)a.min_acute, (unsigned long long)ct.events,
                 (unsigned long long)a.min_events, (unsigned long long)ct.shell_supports,
                 (unsigned long long)a.min_shell, (unsigned long long)ct.deep, (unsigned long long)a.min_deep,
                 (unsigned long long)ct.order_pairs, (unsigned long long)a.min_order_pairs);
    return 3;
  }
  if (g_limbs.level_bits < a.min_level_bits) {
    std::fprintf(stderr, "PLANCHER : produit du niveau %d bits < %d\n", g_limbs.level_bits, a.min_level_bits);
    return 3;
  }
  std::printf("q3_oracle OK\n");
  return 0;
}
