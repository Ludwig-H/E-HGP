// MorseHGP3D v5 — ORDRE EXACT DES NIVEAUX : compare_exact_level (U320) et
// compare_rational (U192) de src/lanes/level.hpp contre l'oracle OBig<12>
// (384 bits, limbes 32 bits — oracle/obig.hpp), sur TOUTES les paires de
// niveaux recoltes.
//
// Le SUJET forme les niveaux avec src/lanes/q2.hpp, q3.hpp, q4.hpp :
//   q2 : D²/4 (Rational128 reduite) ; q3 : q3_exact_level = D·E·X/(4G) reduite ;
//   q4 : q4_level_raw = |N'|² (U192) / det² (i128), NON reduits.
// L'ORACLE ne partage aucune primitive de decision avec lui : dot/cross/norm2
// locaux sur des OBig (jamais p3_dot ni BallKey::power), et il compare
//   num1·den2  ?  num2·den1
// en OBig, les U192 num[3] convertis PAR LIMBES (from_u64_words), les i128 par
// from_i128. Formules RE-DERIVEES pour la re-formation des niveaux (chaque
// niveau recolte est aussi recalcule depuis les coordonnees u16) :
//   q2  boule diametrale de (a,b) : R² = |a-b|²/4 ;
//   q3  circumboule du triangle (a,b,x), d = b-a, u = x-a : 16·Aire² =
//       4·|d×u|² (la norme du produit vectoriel est le double de l'aire), et
//       R = |ab|·|ax|·|bx| / (4·Aire), donc R² = D·E·X / (4·|d×u|²) avec
//       D = |d|², E = |u|², X = |b-x|² calcule DIRECTEMENT (pas D+E-2F) et
//       G_o = |d×u|² (pas DE-F²) ;
//   q4  circumcentre par Cramer, M = 2·[e1; e2; e3] (lignes e_i = v_i - a),
//       r_i = |e_i|² ; M(c-a) = r ; les colonnes de adj(M) sont les produits
//       vectoriels des lignes : adj(M)·r = Σ_i r_i (m_j × m_k) sur les
//       cycliques (i,j,k), et det M = m1·(m2×m3). Avec m_i = 2e_i :
//       N' = 4·Σ r_i (e_j × e_k), det = 8·e1·(e2×e3), c-a = N'/det,
//       R² = |N'|²/det² — insensible au signe de det (orientation canonique
//       du sujet), et EGAL EN REPRESENTATION au sujet (num = |N'|²,
//       den = det², non reduits : c'est le contrat de q4.hpp).
// Portes :
//   P1  accord got == want sur toutes les paires (ExactLevel, toutes lanes
//       confondues apres promote_level) ; compare_rational == want sur les
//       paires q2/q3 ;
//   P2  antisymetrie compare(y,x) == -compare(x,y) ;
//   P3  canonicite q2/q3 : egalite semantique <=> memes (num, den) reduits ;
//   P4  plancher de plateaux : >= --min-ties paires semantiquement EGALES
//       (les macro-lots), dont >= --min-repr-diff a representations
//       DIFFERENTES (la grande cosphere : tetraedres cospheriques, meme boule,
//       (|N'|², det²) distincts ; et le tie inter-lanes q2/q4 de ses paires
//       antipodales, R² = 12000² + 16000² = 400 000 000) ;
//   P5  re-formation par l'oracle : chaque niveau q2/q3 est semantiquement
//       egal a la fraction re-derivee ; chaque niveau q4 lui est egal EN
//       REPRESENTATION (num, den) ;
//   F1  fixture GRAVEE de largeur mot-haut : le tetraedre regulier des coins
//       de la grille a = (0,0,0), b = (M,M,0), x = (M,0,M), y = (0,M,M),
//       M = 65535 : det = 16·M³, N' = 8·M⁴·(1,1,1), |N'|² = 192·M⁸ ≈ 2^135,6
//       => num[2] = 0xbf != 0 ; den = 256·M⁶ ; R² = 3M²/4 ;
//   F2  fixtures synthetiques de largeur (preconditions declarees respectees,
//       hors geometrie u16 — voir la borne ci-dessous) : produits croises qui
//       ne different QUE dans le mot haut w[4] (U320) ou w[2] (U192) ;
//   F3  cosphere : tout niveau q4 recolte sur la grande cosphere vaut
//       400 000 000 exactement.
// BORNE (constatee en ecrivant F2, a graver dans MATHEMATIQUES) : pour un
// tetraedre BIEN CENTRE u16, le centre est dans l'enveloppe donc R² < 3M²
// < 2^33,6 ; det <= 16M³ < 2^52 ; |N'|² = det²R² < 2^137,6 ; tout produit
// croise q4/q4 reel est < 2^241,6 < 2^256 : le mot w[4] de U320 n'est
// JAMAIS atteint par des donnees u16, seule F2 l'exerce. La precondition
// « < 2^260 » de wide.hpp est correcte mais lache.
// Mutant `level-trunc-hi` (mot haut de U192 et U320 tronque) : tue par F1
// (num[2] devient 0 a la formation), P5 et F2. Codes : 0 accord ; 1 desaccord
// du juge ; 2 refus avant calcul ; 3 plancher, invariant ou debordement de
// l'oracle (fail-closed) ; 4 mutant tue.
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "../oracle/obig.hpp"
#include "../src/cloud/families.hpp"
#include "../src/core/mutants.hpp"
#include "../src/lanes/q2.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/lanes/q4.hpp"

using namespace mhgp5;

namespace {

using OB = mhgp5_oracle::OBig384;

// ---- primitives LOCALES de l'oracle (jamais celles de la production) ---------

OB ob_i64(i64 v) { return OB::from_i64(v); }
OB ob_i128(i128 v) { return OB::from_i128((mhgp5_oracle::oi128)v); }
OB ob_u192(const u64 w[3]) { return OB::from_u64_words(w, 3); }

struct OV3 {
  OB x, y, z;
};

OV3 ov_diff(const P3& p, const P3& q) { return OV3{ob_i64(p.x - q.x), ob_i64(p.y - q.y), ob_i64(p.z - q.z)}; }
OB ov_dot(const OV3& a, const OV3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
OB ov_norm2(const OV3& a) { return ov_dot(a, a); }
OV3 ov_cross(const OV3& a, const OV3& b) {
  return OV3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
OV3 ov_scale(const OB& s, const OV3& v) { return OV3{s * v.x, s * v.y, s * v.z}; }
OV3 ov_add(const OV3& a, const OV3& b) { return OV3{a.x + b.x, a.y + b.y, a.z + b.z}; }

struct Level {
  int lane;         // 2, 3, 4
  int cloud;        // index du nuage source
  ExactLevel ex;    // representant commun (sujet)
  Rational128 rat;  // q2/q3 seulement (sujet, reduite)
  OB num, den;      // conversion par limbes du representant du sujet
};

int g_fail = 0;
void fail(const char* what) {
  if (g_fail < 30) std::fprintf(stderr, "DESACCORD niveau : %s\n", what);
  ++g_fail;
}

Level make_level(int lane, int cloud, const ExactLevel& ex, const Rational128& rat) {
  Level l;
  l.lane = lane;
  l.cloud = cloud;
  l.ex = ex;
  l.rat = rat;
  l.num = ob_u192(ex.num);
  l.den = ob_i128(ex.den);
  return l;
}

// Egalite semantique de deux fractions (den > 0) en OBig.
bool same_fraction(const OB& n1, const OB& d1, const OB& n2, const OB& d2) { return cmp(n1 * d2, n2 * d1) == 0; }

// ---- recolte : le sujet forme, l'oracle re-forme ------------------------------

void harvest_q2(int cloud, const P3& a, const P3& b, std::vector<Level>* out) {
  const Rational128 rat = q2_exact_level(p3_norm2(p3_sub(a, b)));
  const Level l = make_level(2, cloud, promote_level(rat), rat);
  const OB onum = ov_norm2(ov_diff(a, b));
  if (!same_fraction(onum, ob_i64(4), l.num, l.den)) fail("re-formation q2 : D²/4");
  out->push_back(l);
}

bool strictly_acute(const P3& a, const P3& b, const P3& x) {
  // Acuite stricte par les trois produits scalaires aux sommets (i64).
  const auto dotv = [](const P3& p, const P3& q, const P3& o) {
    return (p.x - o.x) * (q.x - o.x) + (p.y - o.y) * (q.y - o.y) + (p.z - o.z) * (q.z - o.z);
  };
  return dotv(b, x, a) > 0 && dotv(a, x, b) > 0 && dotv(a, b, x) > 0;
}

void harvest_q3(int cloud, const P3& a, const P3& b, const P3& x, std::vector<Level>* out) {
  if (!strictly_acute(a, b, x)) return;
  const Rational128 rat = q3_exact_level(a, b, x);
  const Level l = make_level(3, cloud, promote_level(rat), rat);
  const OV3 d = ov_diff(b, a), u = ov_diff(x, a), e = ov_diff(b, x);
  const OB D = ov_norm2(d), E = ov_norm2(u), X = ov_norm2(e);
  const OB g = ov_norm2(ov_cross(d, u));
  if (g.sign() <= 0) fail("re-formation q3 : |d×u|² doit etre > 0 (triangle aigu)");
  const OB onum = D * E * X;
  const OB oden = ob_i64(4) * g;
  if (!same_fraction(onum, oden, l.num, l.den)) fail("re-formation q3 : D·E·X/(4|d×u|²)");
  out->push_back(l);
}

// Rend true si (a,b,x,y) est un support q4 du sujet (det != 0, centre
// strictement interieur) ; le niveau est alors recolte et re-forme.
bool harvest_q4(int cloud, const P3& a, const P3& b, const P3& x, const P3& y, std::vector<Level>* out) {
  const Q4Form f = q4_form(a, b, x, y);
  if (f.det == 0) return false;
  if (!q4_center_strictly_inside(f, a, b, x, y)) return false;
  const ExactLevel ex = q4_level_raw(f);
  const Level l = make_level(4, cloud, ex, Rational128{0, 1});
  const OV3 e1 = ov_diff(b, a), e2 = ov_diff(x, a), e3 = ov_diff(y, a);
  const OB r1 = ov_norm2(e1), r2 = ov_norm2(e2), r3 = ov_norm2(e3);
  const OV3 s = ov_add(ov_add(ov_scale(r1, ov_cross(e2, e3)), ov_scale(r2, ov_cross(e3, e1))),
                       ov_scale(r3, ov_cross(e1, e2)));
  const OV3 np = ov_scale(ob_i64(4), s);
  const OB det = ob_i64(8) * ov_dot(e1, ov_cross(e2, e3));
  if (cmp(det.abs(), ob_i128(f.det)) != 0) fail("re-formation q4 : |det| = 8·e1·(e2×e3)");
  const OB onum = ov_norm2(np);
  const OB oden = det * det;
  if (cmp(onum, l.num) != 0) fail("re-formation q4 : num = |N'|² (representation)");
  if (cmp(oden, l.den) != 0) fail("re-formation q4 : den = det² (representation)");
  out->push_back(l);
  return true;
}

// ---- nuages graves -------------------------------------------------------------

std::vector<P3> equilateral_max_cloud() {
  const i64 M = 65535;
  return {{0, 0, 0}, {M, M, 0}, {M, 0, M}, {0, M, M}, {M, M, M}, {30000, 20000, 10000}};
}

std::vector<P3> near_right_cloud() {
  return {{0, 0, 0}, {40000, 0, 0}, {20000, 20001, 0}, {20000, 10000, 1000}, {1000, 19000, 2000}};
}

// Grande cosphere : permutations signees de (12000, 16000, 0) autour de
// (cx, cx, cx), rayon 20000 ; plus le centre et un point interieur.
std::vector<P3> cosphere_cloud(i64 cx, const P3& inner) {
  const i64 pat[3] = {12000, 16000, 0};
  const int perm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  std::vector<P3> pts;
  std::set<long long> seen;
  for (const auto& pr : perm)
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
          const i64 x = cx + sx * pat[pr[0]], y = cx + sy * pat[pr[1]], z = cx + sz * pat[pr[2]];
          const long long key = (x << 34) | (y << 17) | z;
          if (!seen.insert(key).second) continue;
          pts.push_back(P3{x, y, z});
        }
  pts.push_back(P3{cx, cx, cx});
  pts.push_back(inner);
  return pts;
}

struct Args {
  bool ok = true;
  CloudFamily family = CloudFamily::kUniform;
  int n = 300;
  int coord = 0;
  long long seed = 3;
  int m2 = 40, m3 = 28, m4 = 18;  // sous-ensembles (premiers points) du nuage de famille
  int cap_cosphere = 400;         // niveaux q4 gardes sur la cosphere (pas deterministe)
  u64 min_levels = 200, min_ties = 1, min_repr_diff = 1, min_q4 = 50;
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
    else if (const char* v = val("--m2=")) a.m2 = std::atoi(v);
    else if (const char* v = val("--m3=")) a.m3 = std::atoi(v);
    else if (const char* v = val("--m4=")) a.m4 = std::atoi(v);
    else if (const char* v = val("--cap-cosphere=")) a.cap_cosphere = std::atoi(v);
    else if (const char* v = val("--min-levels=")) a.min_levels = (u64)std::atoll(v);
    else if (const char* v = val("--min-ties=")) a.min_ties = (u64)std::atoll(v);
    else if (const char* v = val("--min-repr-diff=")) a.min_repr_diff = (u64)std::atoll(v);
    else if (const char* v = val("--min-q4=")) a.min_q4 = (u64)std::atoll(v);
    else if (const char* v = val("--inject=")) a.inject = v;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.ok = false;
    }
  }
  return a;
}

}  // namespace

int main(int argc, char** argv) {
  const Args a = parse(argc, argv);
  if (!a.ok || a.n < 4 || a.m2 < 2 || a.m3 < 3 || a.m4 < 4 || a.cap_cosphere < 1) {
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
  if ((int)fam.size() < std::max(a.m2, std::max(a.m3, a.m4))) {
    std::fprintf(stderr, "REFUS : la famille n'a produit que %zu points\n", fam.size());
    return 2;
  }
  mhgp5_oracle::clear_overflow();

  // ---- 1. Recolte --------------------------------------------------------------
  std::vector<Level> levels;
  u64 q4_cosphere = 0;
  {
    // Nuage de famille : sous-ensembles deterministes (premiers points).
    for (int i = 0; i < a.m2; ++i)
      for (int j = i + 1; j < a.m2; ++j) harvest_q2(0, fam[(size_t)i], fam[(size_t)j], &levels);
    for (int i = 0; i < a.m3; ++i)
      for (int j = i + 1; j < a.m3; ++j)
        for (int k = j + 1; k < a.m3; ++k) harvest_q3(0, fam[(size_t)i], fam[(size_t)j], fam[(size_t)k], &levels);
    for (int i = 0; i < a.m4; ++i)
      for (int j = i + 1; j < a.m4; ++j)
        for (int k = j + 1; k < a.m4; ++k)
          for (int l = k + 1; l < a.m4; ++l)
            harvest_q4(0, fam[(size_t)i], fam[(size_t)j], fam[(size_t)k], fam[(size_t)l], &levels);
    // Nuages graves : exhaustifs (la cosphere plafonnee par pas deterministe).
    const std::vector<std::vector<P3>> engraved = {equilateral_max_cloud(), near_right_cloud(),
                                                   cosphere_cloud(32768, P3{30000, 30000, 30000})};
    for (size_t c = 0; c < engraved.size(); ++c) {
      const std::vector<P3>& pts = engraved[c];
      const int cloud = (int)c + 1;
      const size_t m = pts.size();
      for (size_t i = 0; i < m; ++i)
        for (size_t j = i + 1; j < m; ++j) harvest_q2(cloud, pts[i], pts[j], &levels);
      for (size_t i = 0; i < m; ++i)
        for (size_t j = i + 1; j < m; ++j)
          for (size_t k = j + 1; k < m; ++k) harvest_q3(cloud, pts[i], pts[j], pts[k], &levels);
      std::vector<Level> q4;
      for (size_t i = 0; i < m; ++i)
        for (size_t j = i + 1; j < m; ++j)
          for (size_t k = j + 1; k < m; ++k)
            for (size_t l = k + 1; l < m; ++l) harvest_q4(cloud, pts[i], pts[j], pts[k], pts[l], &q4);
      if (cloud == 3) {
        q4_cosphere = q4.size();
        // F3 : tout niveau q4 de la cosphere vaut exactement 400 000 000.
        const OB r2 = ob_i64(400000000);
        for (const Level& l : q4)
          if (!same_fraction(l.num, l.den, r2, ob_i64(1))) fail("F3 cosphere : niveau q4 != 400000000");
        const size_t stride = (q4.size() + (size_t)a.cap_cosphere - 1) / (size_t)a.cap_cosphere;
        for (size_t t = 0; t < q4.size(); t += std::max<size_t>(1, stride)) levels.push_back(q4[t]);
      } else {
        for (const Level& l : q4) levels.push_back(l);
      }
    }
  }
  u64 n_q2 = 0, n_q3 = 0, n_q4 = 0;
  for (const Level& l : levels) (l.lane == 2 ? n_q2 : (l.lane == 3 ? n_q3 : n_q4))++;

  // ---- 2. Toutes les paires : P1, P2, P3, P4 ------------------------------------
  u64 pairs = 0, ties = 0, ties_repr_diff = 0, ties_cross_lane = 0, rational_pairs = 0;
  for (size_t i = 0; i < levels.size(); ++i)
    for (size_t j = i + 1; j < levels.size(); ++j) {
      const Level &x = levels[i], &y = levels[j];
      const int got = compare_exact_level(x.ex, y.ex);
      const int back = compare_exact_level(y.ex, x.ex);
      const int want = cmp(x.num * y.den, y.num * x.den);
      if (got != want) fail("P1 produit croise U320");
      if (back != -got) fail("P2 antisymetrie");
      if (x.lane < 4 && y.lane < 4) {
        const int gr = compare_rational(x.rat, y.rat);
        if (gr != want) fail("P1 produit croise U192 (compare_rational)");
        if (compare_rational(y.rat, x.rat) != -gr) fail("P2 antisymetrie (compare_rational)");
        const bool same_repr = x.rat.num == y.rat.num && x.rat.den == y.rat.den;
        if ((want == 0) != same_repr) fail("P3 canonicite q2/q3 : egalite semantique <=> memes fractions reduites");
        ++rational_pairs;
      }
      if (want == 0) {
        ++ties;
        if (x.ex != y.ex) ++ties_repr_diff;
        if (x.lane != y.lane) ++ties_cross_lane;
      }
      ++pairs;
    }

  // ---- F1 : tetraedre regulier des coins (num[2] != 0) --------------------------
  {
    const i64 M = 65535;
    const P3 a4{0, 0, 0}, b4{M, M, 0}, x4{M, 0, M}, y4{0, M, M};
    std::vector<Level> one;
    if (!harvest_q4(9, a4, b4, x4, y4, &one)) fail("F1 : le tetraedre des coins doit etre un support q4");
    else {
      const Level& l = one[0];
      if (l.ex.num[2] == 0) fail("F1 : num[2] == 0 — |N'|² = 192·M⁸ doit traverser le mot haut");
      if (l.ex.num[2] != 0xbfull || l.ex.num[1] != 0xfa0014ffd600347full || l.ex.num[0] != 0xd60014fffa0000c0ull)
        fail("F1 : num != 0xbffa0014ffd600347fd60014fffa0000c0 (192·M⁸)");
      if (l.den.hex() != "0xfffa000effec000efffa000100") fail("F1 : den != 256·M⁶");
      // R² = 3M²/4 : num·4 == 3M²·den.
      if (!same_fraction(l.num, l.den, ob_i64(3 * M * M), ob_i64(4))) fail("F1 : R² != 3M²/4");
      // Contre chaque niveau recolte, l'ordre du sujet suit l'oracle.
      for (const Level& o : levels) {
        const int got = compare_exact_level(l.ex, o.ex);
        const int want = cmp(l.num * o.den, o.num * l.den);
        if (got != want) fail("F1 : ordre contre un niveau recolte");
      }
    }
  }

  // ---- F2 : fixtures synthetiques de largeur mot-haut ----------------------------
  {
    ExactLevel x{{0, 0, (u64)1 << 17}, (i128)1 << 113};                        // 2^145 / 2^113
    ExactLevel y{{0, 0, ((u64)1 << 17) | ((u64)1 << 15)}, (i128)1 << 113};    // (2^145 + 2^143) / 2^113
    if (compare_exact_level(x, y) != -1) fail("F2 : mot w[4] (U320) — x < y attendu");
    if (compare_exact_level(y, x) != 1) fail("F2 : mot w[4] (U320) — y > x attendu");
    if (cmp(ob_u192(x.num) * ob_i128(y.den), ob_u192(y.num) * ob_i128(x.den)) != -1) fail("F2 : oracle w[4]");
    const Rational128 p{(i128)1 << 100, (i128)1 << 69};
    const Rational128 q{((i128)1 << 100) + ((i128)1 << 71), (i128)1 << 69};
    if (compare_rational(p, q) != -1) fail("F2 : mot w[2] (U192) — p < q attendu");
    if (compare_rational(q, p) != 1) fail("F2 : mot w[2] (U192) — q > p attendu");
    if (cmp(ob_i128(p.num) * ob_i128(q.den), ob_i128(q.num) * ob_i128(p.den)) != -1) fail("F2 : oracle w[2]");
  }

  if (mhgp5_oracle::overflow_seen()) {
    std::fprintf(stderr, "REFUS numeric_failure : debordement de l'oracle OBig (fail-closed)\n");
    return 3;
  }

  std::printf(
      "level_cmp : famille=%s n=%d coord=%d niveaux=%zu (q2=%llu q3=%llu q4=%llu, cosphere q4=%llu) paires=%llu "
      "rationnelles=%llu plateaux=%llu repr_differentes=%llu inter_lanes=%llu desaccords=%d\n",
      cloud_family_name(a.family), a.n, coord, levels.size(), (unsigned long long)n_q2, (unsigned long long)n_q3,
      (unsigned long long)n_q4, (unsigned long long)q4_cosphere, (unsigned long long)pairs,
      (unsigned long long)rational_pairs, (unsigned long long)ties, (unsigned long long)ties_repr_diff,
      (unsigned long long)ties_cross_lane, g_fail);

  if (mutant) {
    if (g_fail > 0) {
      std::printf("MUTANT TUE : %s\n", a.inject.c_str());
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s non discrimine\n", a.inject.c_str());
    return 3;
  }
  if (g_fail > 0) return 1;
  if ((u64)levels.size() < a.min_levels || n_q4 < a.min_q4 || ties < a.min_ties || ties_repr_diff < a.min_repr_diff) {
    std::fprintf(stderr,
                 "PLANCHER : niveaux=%zu (>= %llu), q4=%llu (>= %llu), plateaux=%llu (>= %llu), "
                 "repr_differentes=%llu (>= %llu)\n",
                 levels.size(), (unsigned long long)a.min_levels, (unsigned long long)n_q4,
                 (unsigned long long)a.min_q4, (unsigned long long)ties, (unsigned long long)a.min_ties,
                 (unsigned long long)ties_repr_diff, (unsigned long long)a.min_repr_diff);
    return 3;
  }
  std::printf("level_cmp OK\n");
  return 0;
}
