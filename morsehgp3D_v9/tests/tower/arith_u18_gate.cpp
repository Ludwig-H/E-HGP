// MorseHGP3D v9 — porte arith_u18 : l'arithmetique exacte de la tour FULL au
// domaine 18 bits (M = 262143), jugee contre une arithmetique INDEPENDANTE.
//
// Objet juge (fonctions de PRODUCTION appelees telles quelles, jamais
// recopiees) :
//   1. cles et niveaux : q2_ball_key, q3_form / q3_ball_form / q3_ball_key,
//      q4_form / q4_ball_form / ball_key_reduce, q2_exact_level,
//      q3_level_raw / q3_exact_level, q4_level_raw, compare_exact_level,
//      same_exact_level, compare_rational, q4_center_strictly_inside ;
//   2. BallKey::power (et q3_power / q4_power) aux huit coins de [0,M]^3, sur
//      les supports (coquille : zero exact), pres du centre et au hasard ;
//   3. census_detail::AxisBounds::bounds sur des boites entieres (pres de 0,
//      pres de M, contenant ou non le minimiseur, au-dela de 65535) ;
//   4. plateau_detail::pair_diametral, triangle_closed, tetra_closed avec un
//      centre ball_center(key) ;
//   5. anchor_meb (MEB exacte de <= 10 sites).
//
// Juge : boost::multiprecision::cpp_int et cpp_rational, par des formules
// DISTINCTES de celles de la tour (aucune fonction de la tour dans le juge) :
//   - centre q3 par la formule du double produit vectoriel
//     c = a + ((|d|^2 u - |u|^2 d) x (d x u)) / (2 |d x u|^2) (la tour : forme
//     de Gram W) ;
//   - centre q4 par la formule des produits mixtes
//     c = a + (|e1|^2 e2xe3 + |e2|^2 e3xe1 + |e3|^2 e1xe2) / (2 e1.(e2xe3))
//     (la tour : adjointe de Cramer de la matrice 2e) ;
//   - forme primitive recalculee depuis (centre n/d, rayon^2 q/d^2) :
//     |dz - n|^2 - q, divisee par le pgcd de ses coefficients ;
//   - appartenance a l'enveloppe fermee par barycentriques : produits
//     vectoriels pour le triangle, substitution de determinants (Cramer) pour
//     le tetraedre (la tour : Gram 2x2 et cotes de faces) ;
//   - bornes d'AxisBounds par ENUMERATION de tous les points entiers de la
//     boite (la tour : separabilite par axe) ;
//   - MEB par enumeration de TOUS les supports de cardinal <= 4 et minimum des
//     boules englobantes (la tour : premier support positif englobant).
// Le juge est lui-meme juge : valeurs fixes calculees a la main et recoupement
// avec l'oracle rationnel borne `local_plateau_oracle` (Gauss sur
// boost::rational, autre chemin) sur les supports bien centres et les MEB de
// <= 5 sites.
//
// Bornes documentees (docs/PROVENANCE.md, « a confirmer par la porte
// arith_u18 ») : chaque valeur observee est confrontee a sa borne ; un
// depassement est un desaccord (cause=bound.*).
//
// Planchers de non-vacuite : chaque categorie exige un nombre minimal de cas,
// dont des cas adversariaux nommes (triangles quasi degeneres a rayon^2 > 2^60,
// tetraedres quasi plats a centre strictement interieur, paires de niveaux a
// ecart relatif < 2^-40, egalites de niveaux a representations distinctes,
// boites ou l'ancien ecretage u16 du minimiseur surestimait le minimum, et au
// moins un produit normale.(centre - sommet) >= 2^127 dans triangle_closed et
// dans tetra_closed, c'est-a-dire un cas ou l'ancien calcul i128 de la v7
// debordait). Deux fixtures gravees aux coordonnees exactes (magnitudes en
// tete de generate_fixture) : un debordement des produits intermediaires, et
// un tetraedre ou la somme exacte sort de l'i128 si bien que l'ancien
// tetra_closed rendait false (meme en arithmetique rebouclee, -fwrapv) pour un
// centre situe sur une face.
//
// Aucune verification exhaustive : les theoremes invoques (separabilite et
// convexite de la puissance, unicite de la MEB) ne sont pas re-parcourus ; le
// hasard est a graine fixe (std::mt19937_64), la sortie est deterministe.
//
// Codes : 0 conforme (une ligne JSON {"status":"passed",...}) ; 1 desaccord
// du juge (ligne `cause=<raison>` sur stdout, details sur stderr) ; 2 argument
// refuse ; 3 plancher de non-vacuite ou invariant du harnais (`cause=...`).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "../../src/tower/core/types.hpp"
#include "../../src/tower/core/wide.hpp"
#include "../../src/tower/forest/anchor_meb.hpp"
#include "../../src/tower/forest/plateau.hpp"
#include "../../src/tower/lanes/level.hpp"
#include "../../src/tower/lanes/q2.hpp"
#include "../../src/tower/lanes/q3.hpp"
#include "../../src/tower/lanes/q4.hpp"
#include "../../src/tower/pipeline/census.hpp"
#include "../../oracle/tower/local_plateau_oracle.hpp"

namespace {

using namespace mhgp9::tower;
using Int = boost::multiprecision::cpp_int;
using Rat = boost::multiprecision::cpp_rational;

constexpr i64 kM = kCoordMax;  // 262143

// ---------------------------------------------------------------- echecs

struct Failure {
  int code;
  std::string cause;
  std::string detail;
};
[[noreturn]] void disagree(const std::string& cause, const std::string& detail) {
  throw Failure{1, cause, detail};
}
[[noreturn]] void harness(const std::string& cause, const std::string& detail) {
  throw Failure{3, cause, detail};
}

// ------------------------------------------------ conversions (pas du juge)

Int to_int(i128 v) {
  const bool neg = v < 0;
  const u128 mag = neg ? (u128)0 - (u128)v : (u128)v;
  Int out = static_cast<u64>(mag >> 64);
  out <<= 64;
  out += static_cast<u64>(mag);
  return neg ? Int(-out) : out;
}
Int level_num(const ExactLevel& l) {
  Int n = l.num[2];
  n <<= 64;
  n += l.num[1];
  n <<= 64;
  n += l.num[0];
  return n;
}
unsigned bits(const Int& v) {
  if (v == 0) return 0;
  return static_cast<unsigned>(boost::multiprecision::msb(boost::multiprecision::abs(v))) + 1;
}
Int igcd(const Int& a, const Int& b) {
  return boost::multiprecision::gcd(boost::multiprecision::abs(a), boost::multiprecision::abs(b));
}
Int floor_div(const Int& a, const Int& b) {
  Int q = a / b;
  if (q * b != a && ((a < 0) != (b < 0))) --q;
  return q;
}
u64 word(const Int& v, unsigned i) {
  const Int mask = (Int(1) << 64) - 1;
  return ((v >> (64 * i)) & mask).convert_to<u64>();
}
// Representation ExactLevel arbitraire (num < 2^192, 0 < den < 2^127).
ExactLevel make_level(const Int& num, const Int& den) {
  if (num < 0 || bits(num) > 192 || den <= 0 || bits(den) > 126)
    harness("harness.level_repr_range", "num=" + num.str() + " den=" + den.str());
  ExactLevel l{{word(num, 0), word(num, 1), word(num, 2)}, 0};
  l.den = (i128)(((u128)word(den, 1) << 64) | (u128)word(den, 0));
  return l;
}

std::string str(const P3& p) {
  return "(" + std::to_string(p.x) + "," + std::to_string(p.y) + "," + std::to_string(p.z) + ")";
}
std::string str(const std::vector<P3>& pts) {
  std::string out = "[";
  for (size_t i = 0; i < pts.size(); ++i) out += (i ? "," : "") + str(pts[i]);
  return out + "]";
}
std::string str(const BallKey& k) {
  return "A=" + to_int(k.a).str() + " B=(" + to_int(k.b[0]).str() + "," + to_int(k.b[1]).str() + "," +
         to_int(k.b[2]).str() + ") C=" + to_int(k.c).str();
}
std::string str(const ExactLevel& l) { return level_num(l).str() + "/" + to_int(l.den).str(); }

// ------------------------------------------------------------ le juge

namespace orc {

struct V {
  Int c[3];
};
V pt(const P3& p) {
  V v;
  v.c[0] = Int(p.x);
  v.c[1] = Int(p.y);
  v.c[2] = Int(p.z);
  return v;
}
V sub(const V& a, const V& b) {
  V r;
  for (int i = 0; i < 3; ++i) r.c[i] = a.c[i] - b.c[i];
  return r;
}
V add(const V& a, const V& b) {
  V r;
  for (int i = 0; i < 3; ++i) r.c[i] = a.c[i] + b.c[i];
  return r;
}
V mul(const Int& k, const V& a) {
  V r;
  for (int i = 0; i < 3; ++i) r.c[i] = k * a.c[i];
  return r;
}
Int dot(const V& a, const V& b) { return a.c[0] * b.c[0] + a.c[1] * b.c[1] + a.c[2] * b.c[2]; }
V cross(const V& a, const V& b) {
  V r;
  r.c[0] = a.c[1] * b.c[2] - a.c[2] * b.c[1];
  r.c[1] = a.c[2] * b.c[0] - a.c[0] * b.c[2];
  r.c[2] = a.c[0] * b.c[1] - a.c[1] * b.c[0];
  return r;
}
bool is_zero(const V& a) { return a.c[0] == 0 && a.c[1] == 0 && a.c[2] == 0; }
Int det3(const V& a, const V& b, const V& c) { return dot(a, cross(b, c)); }

// Sphere : centre n/d (d > 0, pgcd(n, d) = 1), rayon^2 q/d^2.
struct Sphere {
  V n;
  Int d;
  Int q;
};

Sphere finish(V n, Int d, const P3& on) {
  if (d < 0) {
    d = -d;
    for (auto& x : n.c) x = -x;
  }
  Int g = d;
  for (const auto& x : n.c) g = igcd(g, x);
  if (g > 1) {
    d /= g;
    for (auto& x : n.c) x /= g;
  }
  const V r = sub(mul(d, pt(on)), n);
  return Sphere{n, d, dot(r, r)};
}
Sphere sphere1(const P3& a) { return Sphere{pt(a), Int(1), Int(0)}; }
Sphere sphere2(const P3& a, const P3& b) { return finish(add(pt(a), pt(b)), Int(2), a); }
// Cercle circonscrit dans le plan du triangle (double produit vectoriel).
std::optional<Sphere> sphere3(const P3& a, const P3& b, const P3& x) {
  const V al = sub(pt(b), pt(a)), be = sub(pt(x), pt(a));
  const V nn = cross(al, be);
  if (is_zero(nn)) return std::nullopt;
  const V w = sub(mul(dot(al, al), be), mul(dot(be, be), al));
  const V num = cross(w, nn);
  const Int den = 2 * dot(nn, nn);
  return finish(add(mul(den, pt(a)), num), den, a);
}
// Sphere circonscrite (formule des produits mixtes).
std::optional<Sphere> sphere4(const P3& a, const P3& b, const P3& x, const P3& y) {
  const V e1 = sub(pt(b), pt(a)), e2 = sub(pt(x), pt(a)), e3 = sub(pt(y), pt(a));
  const Int vol = det3(e1, e2, e3);
  if (vol == 0) return std::nullopt;
  const V num = add(add(mul(dot(e1, e1), cross(e2, e3)), mul(dot(e2, e2), cross(e3, e1))),
                    mul(dot(e3, e3), cross(e1, e2)));
  return finish(add(mul(2 * vol, pt(a)), num), 2 * vol, a);
}
Int gram(const P3& a, const P3& b, const P3& x) {
  const V nn = cross(sub(pt(b), pt(a)), sub(pt(x), pt(a)));
  return dot(nn, nn);
}
Int vol6(const P3& a, const P3& b, const P3& x, const P3& y) {
  return det3(sub(pt(b), pt(a)), sub(pt(x), pt(a)), sub(pt(y), pt(a)));
}

// Forme primitive A|z|^2 + B.z + C (A > 0) de |dz - n|^2 - q.
struct Form {
  Int a;
  Int b[3];
  Int c;
};
Form form_of(const Sphere& s) {
  Form f;
  f.a = s.d * s.d;
  for (int i = 0; i < 3; ++i) f.b[i] = -2 * s.d * s.n.c[i];
  f.c = dot(s.n, s.n) - s.q;
  Int g = f.a;
  for (const auto& b : f.b) g = igcd(g, b);
  g = igcd(g, f.c);
  if (g > 1) {
    f.a /= g;
    for (auto& b : f.b) b /= g;
    f.c /= g;
  }
  return f;
}
Int eval(const Form& f, const P3& z) {
  const V v = pt(z);
  return f.a * dot(v, v) + f.b[0] * v.c[0] + f.b[1] * v.c[1] + f.b[2] * v.c[2] + f.c;
}
// |dz - n|^2 - q (= d^2 (|z - c|^2 - R^2)).
Int scaled_power(const Sphere& s, const P3& z) {
  const V r = sub(mul(s.d, pt(z)), s.n);
  return dot(r, r) - s.q;
}
Rat level(const Sphere& s) { return Rat(s.q, s.d * s.d); }
bool smaller(const Sphere& x, const Sphere& y) { return x.q * y.d * y.d < y.q * x.d * x.d; }
bool encloses(const Sphere& s, const std::vector<P3>& pts) {
  for (const auto& p : pts)
    if (scaled_power(s, p) > 0) return false;
  return true;
}

// Centre cn/cd dans le triangle FERME : -1 degenere, 0 dehors (ou hors du
// plan), 1 bord, 2 interieur relatif. Barycentriques par produits vectoriels.
int tri_class(const V& cn, const Int& cd, const P3& t0, const P3& t1, const P3& t2) {
  const V p0 = pt(t0);
  const V e1 = sub(pt(t1), p0), e2 = sub(pt(t2), p0);
  const V nn = cross(e1, e2);
  if (is_zero(nn)) return -1;
  const V r = sub(cn, mul(cd, p0));
  if (dot(nn, r) != 0) return 0;
  const Int al = dot(cross(r, e2), nn), be = dot(cross(e1, r), nn);
  const Int full = cd * dot(nn, nn);
  if (al < 0 || be < 0 || al + be > full) return 0;
  if (al == 0 || be == 0 || al + be == full) return 1;
  return 2;
}
// Centre dans le tetraedre FERME : -1 degenere, 0 dehors, 1 bord, 2
// strictement interieur. Barycentriques par substitution de determinants.
int tet_class(const V& cn, const Int& cd, const std::array<P3, 4>& t) {
  std::array<V, 4> p;
  for (int i = 0; i < 4; ++i) p[(size_t)i] = mul(cd, pt(t[(size_t)i]));
  const auto d4 = [](const std::array<V, 4>& q) { return det3(sub(q[1], q[0]), sub(q[2], q[0]), sub(q[3], q[0])); };
  const Int d0 = d4(p);
  if (d0 == 0) return -1;
  bool boundary = false;
  for (int i = 0; i < 4; ++i) {
    auto q = p;
    q[(size_t)i] = cn;
    const int sgn = d4(q).sign() * d0.sign();
    if (sgn < 0) return 0;
    if (sgn == 0) boundary = true;
  }
  return boundary ? 1 : 2;
}

// MEB : minimum des boules circonscrites (dans l'enveloppe affine) de TOUS
// les supports de cardinal <= 4 qui englobent le nuage. La MEB est l'une
// d'elles et toute boule englobante a un rayon >= au sien (unicite invoquee).
Sphere meb(const std::vector<P3>& pts) {
  std::optional<Sphere> best;
  const size_t n = pts.size();
  const auto offer = [&](const std::optional<Sphere>& s) {
    if (!s) return;
    if (best && !smaller(*s, *best)) return;
    if (encloses(*s, pts)) best = *s;
  };
  for (size_t a = 0; a < n; ++a) {
    offer(sphere1(pts[a]));
    for (size_t b = a + 1; b < n; ++b) {
      offer(sphere2(pts[a], pts[b]));
      for (size_t c = b + 1; c < n; ++c) {
        offer(sphere3(pts[a], pts[b], pts[c]));
        for (size_t d = c + 1; d < n; ++d) offer(sphere4(pts[a], pts[b], pts[c], pts[d]));
      }
    }
  }
  if (!best) harness("oracle.meb_empty", str(pts));
  return *best;
}

}  // namespace orc

// ------------------------------------------------------------ compteurs

struct Counters {
  // 1. cles et niveaux
  u64 q2_keys = 0, q3_keys = 0, q4_keys = 0, q3_degenerate = 0, q4_degenerate = 0;
  u64 q3_near_degenerate = 0;  // rayon^2 > 2^60
  u64 q4_flat_inside = 0;      // 6 vol / L^3 < 2^-10 et centre strictement interieur
  u64 q4_flat_far = 0;         // 6 vol / L^3 < 2^-10 et rayon^2 > 2^60
  u64 q4_inside_checks = 0, q4_inside_true = 0;
  u64 level_repr_checks = 0, level_pairs = 0, level_rational_pairs = 0;
  u64 level_close_pairs = 0, level_ties_distinct_repr = 0, level_designated_pairs = 0;
  // 2. puissance
  u64 power_checks = 0, power_shell_zero = 0, power_high = 0, form_power_checks = 0;
  // 3. AxisBounds
  u64 boxes = 0, box_points = 0, boxes_min_interior = 0, boxes_u16_clip_overestimate = 0;
  u64 boxes_beyond_u16 = 0, full_domain_boxes = 0;
  // 4. plateau
  u64 tri_checks = 0, tri_inside = 0, tri_boundary = 0, tri_outside = 0, tri_degenerate = 0;
  u64 tet_checks = 0, tet_inside = 0, tet_boundary = 0, tet_outside = 0, tet_degenerate = 0;
  u64 pair_checks = 0, pair_true = 0, centre_checks = 0;
  u64 tri_old_i128_overflow = 0, tet_old_i128_overflow = 0;
  u64 tri_old_sum_overflow = 0, tet_old_sum_overflow = 0, tet_old_wrapped_sign_flip = 0;
  unsigned tri_max_term_bits = 0, tet_max_term_bits = 0;
  // 5. MEB
  u64 meb_sets = 0, meb_cross_checked = 0, meb_extra_shell = 0;
  std::array<u64, 5> meb_support{};
  // juge du juge
  u64 oracle_fixed = 0, oracle_cross_supports = 0;
  // largeurs observees (bits de |x|)
  unsigned q3_a = 0, q3_b = 0, q3_c = 0, q3_num = 0, q3_den = 0, q3_power = 0;
  unsigned q4_det = 0, q4_np = 0, q4_a = 0, q4_b = 0, q4_c = 0, q4_num = 0, q4_den = 0, q4_power = 0;
  unsigned q2_b = 0, q2_c = 0;
};
Counters C;

void bound(unsigned& slot, const Int& v, unsigned limit, const char* cause, const std::string& ctx) {
  const unsigned b = bits(v);
  slot = std::max(slot, b);
  if (b > limit) disagree(cause, ctx + " value=" + v.str() + " bits=" + std::to_string(b));
}

// ------------------------------------------------------------ generateurs

std::mt19937_64 rng(20260922);
i64 uni(i64 lo, i64 hi) { return lo + (i64)(rng() % (u64)(hi - lo + 1)); }
i64 clampc(i64 v) { return std::min(std::max(v, (i64)0), kM); }
bool in_dom(const P3& p) { return p.x >= 0 && p.x <= kM && p.y >= 0 && p.y <= kM && p.z >= 0 && p.z <= kM; }
i64 edge_coord() {
  switch (rng() % 6) {
    case 0: return 0;
    case 1: return kM;
    case 2: return uni(0, 7);
    case 3: return uni(kM - 7, kM);
    default: return uni(0, kM);
  }
}
P3 rand_pt() { return P3{uni(0, kM), uni(0, kM), uni(0, kM)}; }
P3 edge_pt() { return P3{edge_coord(), edge_coord(), edge_coord()}; }
P3 corner(unsigned m) { return P3{(m & 1) ? kM : 0, (m & 2) ? kM : 0, (m & 4) ? kM : 0}; }
P3 inward(const P3& c, i64 r) {
  return P3{c.x == 0 ? uni(0, r) : kM - uni(0, r), c.y == 0 ? uni(0, r) : kM - uni(0, r),
            c.z == 0 ? uni(0, r) : kM - uni(0, r)};
}
P3 axis_perm(const P3& v, unsigned perm) {
  const i64 c[3] = {v.x, v.y, v.z};
  static constexpr int kP[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  return P3{c[kP[perm % 6][0]], c[kP[perm % 6][1]], c[kP[perm % 6][2]]};
}
// Racine entiere du generateur (independante de intmath.hpp).
i64 isqrt_gen(i64 v) {
  if (v < 0) return -1;
  i64 r = (i64)std::sqrt((double)v);
  while (r > 0 && r * r > v) --r;
  while ((r + 1) * (r + 1) <= v) ++r;
  return r;
}
// Point entier p avec |p - c|^2 = s (hors du plan z = c.z si off_plane).
std::optional<P3> sphere_point(const P3& c, i64 s, bool off_plane, int tries) {
  const i64 r = isqrt_gen(s);
  for (int t = 0; t < tries; ++t) {
    const i64 x = uni(-r, r);
    const i64 rem1 = s - x * x;
    const i64 ry = isqrt_gen(rem1);
    const i64 y = uni(-ry, ry);
    const i64 rem = rem1 - y * y;
    const i64 z = isqrt_gen(rem);
    if (z * z != rem) continue;
    if (off_plane && z == 0) continue;
    const P3 p{c.x + x, c.y + y, c.z + ((rng() & 1) ? z : -z)};
    if (in_dom(p)) return p;
  }
  return std::nullopt;
}
// Points entiers du cercle |(x,y) - (c.x,c.y)|^2 = r^2 dans le plan z = c.z.
std::vector<P3> circle_points(const P3& c, i64 r) {
  std::vector<P3> out;
  for (i64 x = -r; x <= r; ++x) {
    const i64 rem = r * r - x * x;
    const i64 y = isqrt_gen(rem);
    if (y * y != rem) continue;
    out.push_back(P3{c.x + x, c.y + y, c.z});
    if (y != 0) out.push_back(P3{c.x + x, c.y - y, c.z});
  }
  return out;
}

// ------------------------------------------------------------ cas de boules

struct BallCase {
  int lane = 0;  // 2, 3, 4
  const char* family = "";
  std::vector<P3> support;
  BallKey key{};
  ExactLevel level{};
  bool has_r128 = false;
  Rational128 r128{0, 1};
  Q3Form f3{};
  Q4Form f4{};
  orc::Sphere sphere;
  orc::Form form;
};
std::vector<BallCase> cases;
std::vector<std::pair<size_t, size_t>> designated;

std::string ctx_of(const char* family, const std::vector<P3>& support) {
  return std::string("family=") + family + " support=" + str(support);
}

void check_key(const BallKey& key, const orc::Form& f, const char* cause, const std::string& ctx) {
  if (to_int(key.a) != f.a || to_int(key.b[0]) != f.b[0] || to_int(key.b[1]) != f.b[1] ||
      to_int(key.b[2]) != f.b[2] || to_int(key.c) != f.c)
    disagree(cause, ctx + " got{" + str(key) + "} want{A=" + f.a.str() + " B=(" + f.b[0].str() + "," + f.b[1].str() +
                        "," + f.b[2].str() + ") C=" + f.c.str() + "}");
}

// Niveau de la tour contre le niveau exact de l'oracle, par compare_exact_level
// et same_exact_level sur des representations de l'oracle : egale, a +-1 du
// numerateur, puis mise a l'echelle maximale (num < 2^192, den < 2^127) pour
// que les produits croises U320 traversent leurs mots hauts.
void check_level_repr(const ExactLevel& t, const Rat& truth, const std::string& ctx) {
  const Int num = boost::multiprecision::numerator(truth), den = boost::multiprecision::denominator(truth);
  const auto expect = [&](const ExactLevel& other, int want, const char* what) {
    const int got = compare_exact_level(t, other);
    const int back = compare_exact_level(other, t);
    const bool same = same_exact_level(t, other);
    ++C.level_repr_checks;
    if (got != want || back != -want || same != (want == 0))
      disagree(std::string("level.compare_exact_level.") + what,
               ctx + " tower=" + str(t) + " oracle_repr=" + str(other) + " got=" + std::to_string(got) +
                   " back=" + std::to_string(back) + " want=" + std::to_string(want));
  };
  expect(make_level(num, den), 0, "equal");
  expect(make_level(num + 1, den), -1, "plus_one");
  if (num >= 1) expect(make_level(num - 1, den), 1, "minus_one");
  const int s = std::min(191 - (int)bits(num + 1), 126 - (int)bits(den));
  if (s > 0) {
    const Int k = Int(1) << s;
    expect(make_level(k * num, k * den), 0, "scaled_equal");
    expect(make_level(k * num + 1, k * den), -1, "scaled_plus_one");
    if (num >= 1) expect(make_level(k * num - 1, k * den), 1, "scaled_minus_one");
  }
}

std::optional<size_t> add_q2(const P3& a, const P3& b, const char* family) {
  const std::string ctx = ctx_of(family, {a, b});
  const orc::Sphere sph = orc::sphere2(a, b);
  const orc::Form of = orc::form_of(sph);
  const BallKey key = q2_ball_key(a, b);
  check_key(key, of, "q2.key", ctx);
  const Rational128 l = q2_exact_level(p3_norm2(p3_sub(a, b)));
  const Rat truth = orc::level(sph);
  if (l.den <= 0 || igcd(to_int(l.num), to_int(l.den)) != 1 || Rat(to_int(l.num), to_int(l.den)) != truth)
    disagree("q2.level", ctx + " got=" + to_int(l.num).str() + "/" + to_int(l.den).str() + " want=" + truth.str());
  for (int i = 0; i < 3; ++i) bound(C.q2_b, to_int(key.b[i]), 20, "bound.q2.key_b", ctx);
  bound(C.q2_c, to_int(key.c), 38, "bound.q2.key_c", ctx);
  const ExactLevel el = promote_level(l);
  check_level_repr(el, truth, ctx);
  BallCase bc;
  bc.lane = 2;
  bc.family = family;
  bc.support = {a, b};
  bc.key = key;
  bc.level = el;
  bc.has_r128 = true;
  bc.r128 = l;
  bc.sphere = sph;
  bc.form = of;
  cases.push_back(bc);
  ++C.q2_keys;
  return cases.size() - 1;
}

std::optional<size_t> add_q3(const P3& a, const P3& b, const P3& x, const char* family) {
  const std::string ctx = ctx_of(family, {a, b, x});
  const auto sph = orc::sphere3(a, b, x);
  const Q3Form f = q3_form(a, b, x);
  if (!sph) {
    if (f.g != 0) disagree("q3.degeneracy", ctx + " g=" + to_int(f.g).str());
    ++C.q3_degenerate;
    return std::nullopt;
  }
  const Int g = orc::gram(a, b, x);
  if (to_int(f.g) != g) disagree("q3.form_g", ctx + " got=" + to_int(f.g).str() + " want=" + g.str());
  // W = 2G (c - a) : W_i d = 2G (n_i - d a_i).
  const orc::V av = orc::pt(a);
  for (int i = 0; i < 3; ++i)
    if (to_int(f.w[i]) * sph->d != 2 * g * (sph->n.c[i] - sph->d * av.c[i]))
      disagree("q3.form_w", ctx + " axis=" + std::to_string(i) + " got=" + to_int(f.w[i]).str());
  const BallForm raw = q3_ball_form(f);
  bound(C.q3_a, to_int(raw.a), 76, "bound.q3.form_a", ctx);
  for (int i = 0; i < 3; ++i) bound(C.q3_b, to_int(raw.b[i]), 96, "bound.q3.form_b", ctx);
  bound(C.q3_c, to_int(raw.c), 116, "bound.q3.form_c", ctx);
  const BallKey key = q3_ball_key(f);
  const orc::Form of = orc::form_of(*sph);
  check_key(key, of, "q3.key", ctx);
  const Rat truth = orc::level(*sph);
  const Rational128 lr = q3_level_raw(a, b, x);
  if (lr.den <= 0 || Rat(to_int(lr.num), to_int(lr.den)) != truth)
    disagree("q3.level_raw", ctx + " got=" + to_int(lr.num).str() + "/" + to_int(lr.den).str() + " want=" + truth.str());
  bound(C.q3_num, to_int(lr.num), 113, "bound.q3.level_num", ctx);
  bound(C.q3_den, to_int(lr.den), 79, "bound.q3.level_den", ctx);
  const Rational128 l = q3_exact_level(a, b, x);
  if (l.den <= 0 || igcd(to_int(l.num), to_int(l.den)) != 1 || Rat(to_int(l.num), to_int(l.den)) != truth)
    disagree("q3.level", ctx + " got=" + to_int(l.num).str() + "/" + to_int(l.den).str() + " want=" + truth.str());
  const ExactLevel el = promote_level(l);
  check_level_repr(el, truth, ctx);
  if (truth > Rat(Int(1) << 60)) ++C.q3_near_degenerate;
  BallCase bc;
  bc.lane = 3;
  bc.family = family;
  bc.support = {a, b, x};
  bc.key = key;
  bc.level = el;
  bc.has_r128 = true;
  bc.r128 = l;
  bc.f3 = f;
  bc.sphere = *sph;
  bc.form = of;
  cases.push_back(bc);
  ++C.q3_keys;
  return cases.size() - 1;
}

std::optional<size_t> add_q4(const P3& a, const P3& b, const P3& x, const P3& y, const char* family) {
  const std::string ctx = ctx_of(family, {a, b, x, y});
  const auto sph = orc::sphere4(a, b, x, y);
  const Q4Form f = q4_form(a, b, x, y);
  if (!sph) {
    if (f.det != 0) disagree("q4.degeneracy", ctx + " det=" + to_int(f.det).str());
    ++C.q4_degenerate;
    return std::nullopt;
  }
  const Int vol = orc::vol6(a, b, x, y);
  const Int det = 8 * boost::multiprecision::abs(vol);
  if (to_int(f.det) != det) disagree("q4.form_det", ctx + " got=" + to_int(f.det).str() + " want=" + det.str());
  // N' = det (c - a) : N'_i d = det (n_i - d a_i).
  const orc::V av = orc::pt(a);
  for (int i = 0; i < 3; ++i)
    if (to_int(f.np[i]) * sph->d != det * (sph->n.c[i] - sph->d * av.c[i]))
      disagree("q4.form_np", ctx + " axis=" + std::to_string(i) + " got=" + to_int(f.np[i]).str());
  bound(C.q4_det, det, 60, "bound.q4.det", ctx);
  for (int i = 0; i < 3; ++i) bound(C.q4_np, to_int(f.np[i]), 79, "bound.q4.np", ctx);
  const BallForm raw = q4_ball_form(f);
  bound(C.q4_a, to_int(raw.a), 60, "bound.q4.form_a", ctx);
  for (int i = 0; i < 3; ++i) bound(C.q4_b, to_int(raw.b[i]), 81, "bound.q4.form_b", ctx);
  bound(C.q4_c, to_int(raw.c), 100, "bound.q4.form_c", ctx);
  const BallKey key = ball_key_reduce(raw);
  const orc::Form of = orc::form_of(*sph);
  check_key(key, of, "q4.key", ctx);
  const Rat truth = orc::level(*sph);
  const ExactLevel el = q4_level_raw(f);
  if (el.den <= 0 || Rat(level_num(el), to_int(el.den)) != truth)
    disagree("q4.level", ctx + " got=" + str(el) + " want=" + truth.str());
  bound(C.q4_num, level_num(el), 160, "bound.q4.level_num", ctx);
  bound(C.q4_den, to_int(el.den), 120, "bound.q4.level_den", ctx);
  check_level_repr(el, truth, ctx);
  const int cls = orc::tet_class(sph->n, sph->d, {a, b, x, y});
  const bool inside = q4_center_strictly_inside(f, a, b, x, y);
  ++C.q4_inside_checks;
  if (inside != (cls == 2))
    disagree("q4.center_strictly_inside", ctx + " got=" + std::to_string(inside) + " oracle_class=" + std::to_string(cls));
  if (inside) ++C.q4_inside_true;
  // Platitude 6 vol / L^3 < 2^-10, L = plus longue arete : (1024 vol6)^2 < (L^2)^3.
  Int l2 = 0;
  const std::array<P3, 4> v{a, b, x, y};
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = i + 1; j < 4; ++j) {
      const orc::V e = orc::sub(orc::pt(v[i]), orc::pt(v[j]));
      l2 = std::max(l2, orc::dot(e, e));
    }
  const bool flat = (1024 * vol) * (1024 * vol) < l2 * l2 * l2;
  if (flat && cls == 2) ++C.q4_flat_inside;
  if (flat && truth > Rat(Int(1) << 60)) ++C.q4_flat_far;
  BallCase bc;
  bc.lane = 4;
  bc.family = family;
  bc.support = {a, b, x, y};
  bc.key = key;
  bc.level = el;
  bc.f4 = f;
  bc.sphere = *sph;
  bc.form = of;
  cases.push_back(bc);
  ++C.q4_keys;
  return cases.size() - 1;
}

void designate(const std::optional<size_t>& i, const std::optional<size_t>& j) {
  if (i && j) designated.emplace_back(*i, *j);
}

// ------------------------------------------------------------ 1. generation

void generate_q2() {
  for (int t = 0; t < 150; ++t) add_q2(rand_pt(), rand_pt(), "q2_random");
  for (int t = 0; t < 150; ++t) add_q2(edge_pt(), edge_pt(), "q2_extreme");
  for (unsigned m = 0; m < 8; ++m)
    for (unsigned n = m + 1; n < 8; ++n) add_q2(corner(m), corner(n), "q2_corners");
  for (int t = 0; t < 10; ++t) {
    const P3 p = edge_pt();
    add_q2(p, p, "q2_singleton");  // rayon nul : la cle du singleton d'anchor_meb
  }
  // Egalites de niveau : (3k,4k,0), (0,0,5k), (0,5k,0).
  for (int t = 0; t < 20; ++t) {
    const i64 k = uni(1, 52428);
    const P3 base{uni(0, kM - 5 * k), uni(0, kM - 5 * k), uni(0, kM - 5 * k)};
    const auto i = add_q2(base, p3_add(base, P3{3 * k, 4 * k, 0}), "q2_tie");
    const auto j = add_q2(base, p3_add(base, P3{0, 0, 5 * k}), "q2_tie");
    const auto l = add_q2(p3_add(base, P3{0, 5 * k, 0}), base, "q2_tie");
    designate(i, j);
    designate(j, l);
  }
}

void generate_q3() {
  for (int t = 0; t < 250; ++t) add_q3(rand_pt(), rand_pt(), rand_pt(), "q3_random");
  for (int t = 0; t < 250; ++t) add_q3(edge_pt(), edge_pt(), edge_pt(), "q3_extreme");
  for (unsigned m = 0; m < 8; ++m)
    for (unsigned n = m + 1; n < 8; ++n)
      for (unsigned o = n + 1; o < 8; ++o) add_q3(corner(m), corner(n), corner(o), "q3_corners");
  // Quasi alignes sur une diagonale du cube (longueur ~ M sqrt 3) : rayon
  // enorme, G minuscule, centre tres loin hors du domaine.
  for (int t = 0; t < 200; ++t) {
    const unsigned c0 = (unsigned)(rng() % 8);
    const P3 a = inward(corner(c0), 3), b = inward(corner(7 - c0), 3);
    const P3 dir = p3_sub(b, a);
    const i64 num = uni(1, 1023);
    const P3 x{clampc(a.x + dir.x * num / 1024 + uni(-2, 2)), clampc(a.y + dir.y * num / 1024 + uni(-2, 2)),
               clampc(a.z + dir.z * num / 1024 + uni(-2, 2))};
    add_q3(a, b, x, "q3_near_collinear_diagonal");
  }
  // Quasi alignes sur une droite quelconque.
  for (int t = 0; t < 100; ++t) {
    const P3 a = rand_pt(), b = rand_pt();
    const P3 dir = p3_sub(b, a);
    const i64 num = uni(1, 4095);
    const P3 x{clampc(a.x + dir.x * num / 4096 + uni(-1, 1)), clampc(a.y + dir.y * num / 4096 + uni(-1, 1)),
               clampc(a.z + dir.z * num / 4096 + uni(-1, 1))};
    add_q3(a, b, x, "q3_near_collinear_line");
  }
  // Grands triangles quasi equilateraux entre coins opposes (A ~ 2^74) : les
  // produits normale.(centre - sommet) du plateau y depassent 2^127.
  static constexpr unsigned kTri[8][3] = {{0, 3, 5}, {0, 3, 6}, {0, 5, 6}, {3, 5, 6},
                                          {1, 2, 4}, {1, 2, 7}, {1, 4, 7}, {2, 4, 7}};
  for (int t = 0; t < 160; ++t) {
    const auto& tri = kTri[t % 8];
    const i64 r = t < 80 ? 7 : 4000;
    add_q3(inward(corner(tri[0]), r), inward(corner(tri[1]), r), inward(corner(tri[2]), r), "q3_big_equilateral");
  }
  // Triangles rectangles (egalite exacte avec la q2 de l'hypotenuse) et quasi
  // rectangles aigus : |x - m|^2 = D^2/4 + 1, rayon^2 = D^2/4 + O(1/L^2), soit
  // un ecart relatif ~ 2^-68 avec la q2 de l'hypotenuse.
  const auto along = [](unsigned axis, i64 s) {
    P3 p{0, 0, 0};
    (axis == 0 ? p.x : axis == 1 ? p.y : p.z) = s;
    return p;
  };
  for (int t = 0; t < 40; ++t) {
    const i64 L = uni(1 << 14, 65535);
    const unsigned i = (unsigned)(rng() % 3), j = (i + 1 + (unsigned)(rng() % 2)) % 3, k = 3 - i - j;
    const P3 base{uni(2, kM - 2 * L - 4), uni(2, kM - 2 * L - 4), uni(2, kM - 2 * L - 4)};
    const P3 a = base, b = p3_add(base, along(i, 2 * L));
    const P3 apex = p3_add(p3_add(base, along(i, L)), along(j, L));
    const auto hyp = add_q2(a, b, "q3_right_hypotenuse");
    designate(hyp, add_q3(a, b, apex, "q3_right"));
    designate(hyp, add_q3(a, b, p3_add(apex, along(k, 1)), "q3_near_right"));
    designate(hyp, add_q3(a, b, p3_add(apex, along(i, 1)), "q3_near_right_shift"));
    // Hypotenuse diagonale ab = 2L(e_i + e_j), x = m + L(e_i - e_j) + e_k.
    const P3 b2 = p3_add(b, along(j, 2 * L));
    designate(add_q2(a, b2, "q3_right_hypotenuse"), add_q3(a, b2, p3_add(b, along(k, 1)), "q3_near_right_diag"));
  }
}

void generate_q4() {
  for (int t = 0; t < 250; ++t) add_q4(rand_pt(), rand_pt(), rand_pt(), rand_pt(), "q4_random");
  for (int t = 0; t < 250; ++t) add_q4(edge_pt(), edge_pt(), edge_pt(), edge_pt(), "q4_extreme");
  for (unsigned m = 0; m < 8; ++m)
    for (unsigned n = m + 1; n < 8; ++n)
      for (unsigned o = n + 1; o < 8; ++o)
        for (unsigned p = o + 1; p < 8; ++p) add_q4(corner(m), corner(n), corner(o), corner(p), "q4_corners");
  // Disphenoides quasi plats a centre strictement interieur : image d'un
  // sommet par la symetrie d'ordre 4 (x, y, z) -> (-y, x, -z) autour de C,
  //   C + (a, b, h), C + (-b, a, -h), C + (-a, -b, h), C + (b, -a, -h),
  // equidistants de C qui est aussi le centroide (donc strictement interieur) ;
  // 6 vol / L^3 ~ h / sqrt(a^2 + b^2) ~ 2^-15. Un tiers aligne sur les axes
  // (b = 0), un tiers tourne, un tiers tourne et perturbe dans le plan (la
  // cocircularite a h^2 pres est perdue : centre loin, en general dehors).
  for (int t = 0; t < 150; ++t) {
    const i64 R = uni(20000, 120000), h = uni(1, 4), jit = t % 3 == 2 ? 3 : 0;
    const i64 a = t % 3 == 0 ? R : uni(R / 2, R), b = t % 3 == 0 ? 0 : uni(-R / 2, R / 2);
    const P3 c{uni(R + 8, kM - R - 8), uni(R + 8, kM - R - 8), uni(R + 8, kM - R - 8)};
    const unsigned perm = (unsigned)(rng() % 6);
    const P3 off[4] = {{a, b, h}, {-b, a, -h}, {-a, -b, h}, {b, -a, -h}};
    P3 v[4];
    for (int i = 0; i < 4; ++i)
      v[i] = p3_add(c, axis_perm(p3_add(off[i], P3{uni(-jit, jit), uni(-jit, jit), 0}), perm));
    add_q4(v[0], v[1], v[2], v[3], "q4_flat_disphenoid");
  }
  // Quasi coplanaires quelconques : y = combinaison entiere de a, b, x a une
  // unite pres ; det minuscule, centre tres loin hors du domaine.
  for (int t = 0; t < 100; ++t) {
    const P3 a = rand_pt(), b = rand_pt(), x = rand_pt();
    const i64 s1 = uni(0, 1024), s2 = uni(0, 1024 - s1);
    const P3 y{clampc(a.x + ((b.x - a.x) * s1 + (x.x - a.x) * s2) / 1024 + uni(-1, 1)),
               clampc(a.y + ((b.y - a.y) * s1 + (x.y - a.y) * s2) / 1024 + uni(-1, 1)),
               clampc(a.z + ((b.z - a.z) * s1 + (x.z - a.z) * s2) / 1024 + uni(-1, 1))};
    add_q4(a, b, x, y, "q4_flat_far");
  }
  // Quatre coins d'une face du cube, souleves de 0..3 : quasi cocycliques et
  // quasi coplanaires aux extremes du domaine.
  for (int t = 0; t < 60; ++t) {
    const unsigned axis = (unsigned)(rng() % 3), side = (unsigned)(rng() % 2);
    P3 v[4];
    for (unsigned q = 0; q < 4; ++q) {
      const i64 u = (q & 1) ? kM - uni(0, 5) : uni(0, 5), w = (q & 2) ? kM - uni(0, 5) : uni(0, 5);
      const i64 f = side ? kM - uni(0, 3) : uni(0, 3);
      v[q] = axis == 0 ? P3{f, u, w} : axis == 1 ? P3{u, f, w} : P3{u, w, f};
    }
    add_q4(v[0], v[1], v[3], v[2], "q4_flat_cube_face");
  }
}

// Coquilles entieres : sphere de centre entier c et de rayon r, grand cercle
// z = c.z, points hors plan, antipodes 2c - p.
struct SphereSet {
  P3 c;
  i64 r = 0;
  std::vector<P3> circle, off, all;
  std::vector<std::pair<P3, P3>> antipodal;
  size_t key_case = 0;
};
std::vector<SphereSet> spheres;

std::vector<size_t> distinct_indices(size_t n, size_t k) {
  std::vector<size_t> out;
  while (out.size() < k) {
    const size_t i = (size_t)(rng() % n);
    if (std::find(out.begin(), out.end(), i) == out.end()) out.push_back(i);
  }
  return out;
}

void generate_spheres() {
  // Rayons IMPAIRS (r^2 = 1 mod 8 : pas de facteur 4^a qui rarefie les points
  // entiers de la sphere) et riches en premiers = 1 mod 4 (points du grand
  // cercle) : 96135 = 3.5.13.17.29, 127075 = 5^2.13.17.23 (sphere de rayon ~
  // M/2 : magnitudes maximales), 40885 = 5.13.17.37 (touche 0, puis M).
  const std::array<std::pair<P3, i64>, 4> specs{{{P3{131071, 131072, 131070}, 96135},
                                                 {P3{131071, 131071, 131071}, 127075},
                                                 {P3{40885, 40885, 40885}, 40885},
                                                 {P3{kM - 40885, kM - 40885, kM - 40885}, 40885}}};
  for (const auto& [c, r] : specs) {
    SphereSet s;
    s.c = c;
    s.r = r;
    auto circ = circle_points(c, r);
    std::shuffle(circ.begin(), circ.end(), rng);
    if (circ.size() < 10) harness("harness.circle_points", str(c));
    s.circle.assign(circ.begin(), circ.begin() + 10);
    while (s.off.size() < 10) {
      const auto p = sphere_point(c, r * r, true, 4000000);
      if (!p) harness("harness.sphere_point", str(c));
      if (std::find(s.off.begin(), s.off.end(), *p) == s.off.end()) s.off.push_back(*p);
    }
    for (size_t i = 0; i < 5; ++i) {
      const P3 q{2 * c.x - s.off[i].x, 2 * c.y - s.off[i].y, 2 * c.z - s.off[i].z};
      s.antipodal.emplace_back(s.off[i], q);
      if (std::find(s.off.begin(), s.off.end(), q) == s.off.end()) s.off.push_back(q);
    }
    s.all = s.circle;
    s.all.insert(s.all.end(), s.off.begin(), s.off.end());
    // Quadruplets cospheriques : meme niveau r^2, representations |N'|^2/det^2
    // toutes distinctes (egalites a representations distinctes).
    std::optional<size_t> prev;
    bool have_key = false;
    for (int t = 0; t < 25; ++t) {
      const auto ix = distinct_indices(s.all.size(), 4);
      const auto k = add_q4(s.all[ix[0]], s.all[ix[1]], s.all[ix[2]], s.all[ix[3]], "q4_sphere");
      if (!k) continue;
      designate(prev, k);
      prev = k;
      if (!have_key) {
        s.key_case = *k;
        have_key = true;
      }
    }
    if (!have_key) harness("harness.sphere_key", str(c));
    const orc::Sphere& ks = cases[s.key_case].sphere;
    if (ks.d != 1 || ks.n.c[0] != c.x || ks.n.c[1] != c.y || ks.n.c[2] != c.z)
      harness("harness.sphere_centre", str(c));
    // Triangles du grand cercle : niveau r^2 exact.
    for (int t = 0; t < 8; ++t) {
      const auto ix = distinct_indices(s.circle.size(), 3);
      designate(prev, add_q3(s.circle[ix[0]], s.circle[ix[1]], s.circle[ix[2]], "q3_great_circle"));
    }
    // Relevement hors plan : y avec |y - c|^2 = r^2 + delta ; rayon^2 du
    // tetraedre = r^2 + delta^2/(4 h^2), a comparer au q3 du cercle (niveaux
    // tres proches, egaux pour delta = 0).
    for (const i64 delta : {0, 0, 1, 1, 2, 2, 4, 4}) {  // r^2 + delta jamais de la forme 4^a(8b+7)
      const auto y = sphere_point(c, r * r + delta, true, 4000000);
      if (!y) continue;
      const auto ix = distinct_indices(s.circle.size(), 3);
      const auto q3 = add_q3(s.circle[ix[0]], s.circle[ix[1]], s.circle[ix[2]], "q3_great_circle");
      const auto q4 = add_q4(s.circle[ix[0]], s.circle[ix[1]], s.circle[ix[2]], *y, "q4_circle_lift");
      designate(q3, q4);
      designate(prev, q4);
      prev = q4;
    }
    spheres.push_back(s);
  }
}

// Fixture permanente de debordement de l'ancien calcul i128 de la v7
// (dc57ffd5 : `n.x * v[0] + n.y * v[1] + n.z * v[2]` en i128 dans
// triangle_closed, meme forme dans tetra_closed). Magnitudes exactes :
//   triangle t0 = (262142,1,262136), t1 = (4,262139,262139), t2 = (6,0,7),
//   aigu, quasi equilateral (cote ~ 3,7e5), dans CET ordre (t0 = sommet de
//   base) ; cle q3 primitive A = 14165226042146669737337 (74 bits), donc
//   cden = 2A (75 bits), |cnum_i| < 2^92 ; normale n = t1-t0 x t2-t0 =
//   (-68713971799, -68714758210, 68716068906) (36 bits) ; v = cnum - cden t0
//   (91 a 92 bits). Premier terme n_x v_x =
//   340201245312615046342419719922262820384 ~ 1,9995 . 2^127 > 2^127 - 1 :
//   l'ancien produit i128 debordait (comportement indefini) alors que la
//   somme exacte vaut 0 (centre coplanaire). Tetraedre (t0, t1, t2, w) avec
//   w = (0,0,0) : la face (t0, t1, w) donne un premier terme ~ 1,9996 . 2^127
//   et une somme NON nulle 649024717901242026348240107875620 (le signe
//   decidait). Largeur S192 de la v9 : termes < 2^129 observes, < 2^134 bornes.
//
// Seconde fixture : l'ancien tetra_closed se TROMPAIT, meme en arithmetique
// qui reboucle modulo 2^128. Triangle aigu t0 = (262143,262143,5),
// t1 = (243337,4,262143), t2 = (0,262140,262143), cle q3 A =
// 13512897593168712106541 (74 bits), |cnum_i| < 2^92 ; w = (181257,18785,14619).
// Le centre est sur la face (t0,t1,t2), a l'interieur du triangle : il est
// dans le tetraedre FERME. Face 0 (t1,t2,w) : somme exacte n.(cnum - cden t1)
// = -189157758314857635157508048876062260950 ~ -1,11 . 2^127, hors i128 ;
// rebouclee, elle devient +151124608606080828305866558555705950506, du
// mauvais cote (faces 1 et 2 idem) : l'ancien code rendait false.
size_t overflow_fixture_case = 0, flip_fixture_case = 0;
const P3 kFixT0{262142, 1, 262136}, kFixT1{4, 262139, 262139}, kFixT2{6, 0, 7}, kFixW{0, 0, 0};
const char* const kFixKeyA = "14165226042146669737337";
const P3 kFlipT0{262143, 262143, 5}, kFlipT1{243337, 4, 262143}, kFlipT2{0, 262140, 262143};
const P3 kFlipW{181257, 18785, 14619};
const char* const kFlipKeyA = "13512897593168712106541";
void generate_fixture() {
  const auto k = add_q3(kFixT0, kFixT1, kFixT2, "fixture_i128_overflow");
  const auto f = add_q3(kFlipT0, kFlipT1, kFlipT2, "fixture_i128_wrapped_sign_flip");
  if (!k || !f) harness("harness.fixture", "degenerate");
  overflow_fixture_case = *k;
  flip_fixture_case = *f;
}

// ------------------------------------------------------------ 2. puissance

i64 clamp_int(const Int& v) {
  if (v < 0) return 0;
  if (v > kM) return kM;
  return v.convert_to<i64>();
}
P3 centre_floor(const orc::Sphere& s) {
  return P3{clamp_int(floor_div(s.n.c[0], s.d)), clamp_int(floor_div(s.n.c[1], s.d)),
            clamp_int(floor_div(s.n.c[2], s.d))};
}

void check_powers(const BallCase& bc) {
  const std::string ctx = ctx_of(bc.family, bc.support);
  std::vector<std::pair<P3, bool>> zs;
  for (unsigned m = 0; m < 8; ++m) zs.emplace_back(corner(m), false);
  for (const auto& p : bc.support) zs.emplace_back(p, true);
  for (int i = 0; i < 3; ++i) zs.emplace_back(rand_pt(), false);
  for (int i = 0; i < 2; ++i) zs.emplace_back(edge_pt(), false);
  const P3 cf = centre_floor(bc.sphere);
  zs.emplace_back(cf, false);
  zs.emplace_back(P3{std::min(cf.x + 1, kM), std::min(cf.y + 1, kM), std::min(cf.z + 1, kM)}, false);
  Int scale = 0;
  if (bc.lane == 3) scale = orc::gram(bc.support[0], bc.support[1], bc.support[2]);
  if (bc.lane == 4)
    scale = 8 * boost::multiprecision::abs(orc::vol6(bc.support[0], bc.support[1], bc.support[2], bc.support[3]));
  const Int d2 = bc.sphere.d * bc.sphere.d;
  for (const auto& [z, shell] : zs) {
    const i128 got = bc.key.power(z);
    const Int want = orc::eval(bc.form, z);
    ++C.power_checks;
    if (to_int(got) != want)
      disagree("power.value", ctx + " z=" + str(z) + " got=" + to_int(got).str() + " want=" + want.str());
    const Int sp = orc::scaled_power(bc.sphere, z);
    if (want.sign() != sp.sign()) harness("oracle.power_sign", ctx + " z=" + str(z));
    if (shell) {
      if (want != 0) harness("oracle.shell", ctx + " z=" + str(z));
      ++C.power_shell_zero;
    }
    if (bits(want) > 100) ++C.power_high;
    if (bc.lane == 3) {
      const Int p = to_int(q3_power(bc.f3, z));
      ++C.form_power_checks;
      if (p * d2 != scale * sp) disagree("power.q3_power", ctx + " z=" + str(z) + " got=" + p.str());
      bound(C.q3_power, p, 117, "bound.q3.power", ctx + " z=" + str(z));
    } else if (bc.lane == 4) {
      const Int p = to_int(q4_power(bc.f4, z));
      ++C.form_power_checks;
      if (p * d2 != scale * sp) disagree("power.q4_power", ctx + " z=" + str(z) + " got=" + p.str());
      bound(C.q4_power, p, 102, "bound.q4.power", ctx + " z=" + str(z));
    }
  }
}

// ------------------------------------------------------------ 3. AxisBounds

Int key_eval(const BallKey& k, i64 x, i64 y, i64 z) {
  const Int X = x, Y = y, Z = z;
  return to_int(k.a) * (X * X + Y * Y + Z * Z) + to_int(k.b[0]) * X + to_int(k.b[1]) * Y + to_int(k.b[2]) * Z +
         to_int(k.c);
}

std::string str(const AxisBox& b) {
  return "[" + std::to_string(b.lo[0]) + "," + std::to_string(b.hi[0]) + "]x[" + std::to_string(b.lo[1]) + "," +
         std::to_string(b.hi[1]) + "]x[" + std::to_string(b.lo[2]) + "," + std::to_string(b.hi[2]) + "]";
}

// Bornes par ENUMERATION de tous les points entiers de la boite.
void check_box(const BallCase& bc, const AxisBox& box) {
  const census_detail::AxisBounds ab{bc.key};
  i128 mn = 0, mx = 0;
  ab.bounds(box, &mn, &mx);
  const Int A = to_int(bc.key.a), B0 = to_int(bc.key.b[0]), B1 = to_int(bc.key.b[1]), B2 = to_int(bc.key.b[2]);
  const Int Cc = to_int(bc.key.c);
  bool first = true;
  Int best_min, best_max;
  i64 arg[3] = {0, 0, 0};
  for (i64 x = box.lo[0]; x <= box.hi[0]; ++x)
    for (i64 y = box.lo[1]; y <= box.hi[1]; ++y)
      for (i64 z = box.lo[2]; z <= box.hi[2]; ++z) {
        const Int X = x, Y = y, Z = z;
        const Int v = A * (X * X + Y * Y + Z * Z) + B0 * X + B1 * Y + B2 * Z + Cc;
        ++C.box_points;
        if (first || v < best_min) {
          best_min = v;
          arg[0] = x;
          arg[1] = y;
          arg[2] = z;
        }
        if (first || v > best_max) best_max = v;
        first = false;
      }
  ++C.boxes;
  const std::string ctx = ctx_of(bc.family, bc.support) + " key{" + str(bc.key) + "} box=" + str(box);
  if (to_int(mn) != best_min) disagree("axis_bounds.min", ctx + " got=" + to_int(mn).str() + " want=" + best_min.str());
  if (to_int(mx) != best_max) disagree("axis_bounds.max", ctx + " got=" + to_int(mx).str() + " want=" + best_max.str());
  bool interior = true, beyond = false, over = false;
  for (int i = 0; i < 3; ++i) {
    interior = interior && box.lo[i] < arg[i] && arg[i] < box.hi[i];
    // Ancien ecretage u16 du minimiseur (65535) : sur un axe ou lo > 65535, il
    // evaluait l'axe en lo ; surestimation ssi la valeur en lo est plus grande.
    if (box.lo[i] > 65535) {
      beyond = true;
      if (arg[i] > box.lo[i]) {
        i64 alt[3] = {arg[0], arg[1], arg[2]};
        alt[i] = box.lo[i];
        if (key_eval(bc.key, alt[0], alt[1], alt[2]) > best_min) over = true;
      }
    }
  }
  if (interior) ++C.boxes_min_interior;
  if (beyond) ++C.boxes_beyond_u16;
  if (over) ++C.boxes_u16_clip_overestimate;
}

// Domaine entier [0,M]^3 : minimum par axe aux entiers voisins du sommet
// rationnel -B_i/(2A), maximum aux bornes (separabilite et convexite invoquees).
void check_full_domain(const BallCase& bc) {
  const census_detail::AxisBounds ab{bc.key};
  const AxisBox box{{0, 0, 0}, {kM, kM, kM}};
  i128 mn = 0, mx = 0;
  ab.bounds(box, &mn, &mx);
  const Int A = to_int(bc.key.a);
  Int want_min = to_int(bc.key.c), want_max = want_min;
  for (int i = 0; i < 3; ++i) {
    const Int B = to_int(bc.key.b[i]);
    const auto f = [&](const Int& t) -> Int { return A * t * t + B * t; };
    const Int fl = floor_div(-B, 2 * A);
    Int best = f(Int(0));
    for (const Int& t : {fl, Int(fl + 1)}) {
      const Int tc = t < 0 ? Int(0) : (t > kM ? Int(kM) : t);
      best = std::min(best, f(tc));
    }
    best = std::min(best, f(Int(kM)));
    want_min += best;
    want_max += std::max(f(Int(0)), f(Int(kM)));
  }
  ++C.full_domain_boxes;
  const std::string ctx = ctx_of(bc.family, bc.support) + " key{" + str(bc.key) + "} box=domain";
  if (to_int(mn) != want_min) disagree("axis_bounds.domain_min", ctx + " got=" + to_int(mn).str() + " want=" + want_min.str());
  if (to_int(mx) != want_max) disagree("axis_bounds.domain_max", ctx + " got=" + to_int(mx).str() + " want=" + want_max.str());
}

void check_boxes(const BallCase& bc) {
  // Minimiseur entier par axe : entier le plus proche du centre n_i/d
  // (floor((2 n_i + d) / (2d))), ecrete au domaine.
  i64 m[3];
  for (int i = 0; i < 3; ++i) m[i] = clamp_int(floor_div(2 * bc.sphere.n.c[i] + bc.sphere.d, 2 * bc.sphere.d));
  const auto axis_range = [&](int mode, int i, i64& lo, i64& hi) {
    switch (mode) {
      case 0: lo = uni(0, 3); hi = lo + uni(0, 3); break;                   // pres de 0
      case 1: hi = uni(kM - 3, kM); lo = hi - uni(0, 3); break;             // pres de M
      case 2: lo = clampc(m[i] - uni(1, 2)); hi = clampc(m[i] + uni(1, 2)); break;  // contient le minimiseur
      case 5:                                                                  // le touche par un bord
        if (rng() & 1) {
          lo = m[i];
          hi = clampc(m[i] + uni(0, 2));
        } else {
          hi = m[i];
          lo = clampc(m[i] - uni(0, 2));
        }
        break;
      case 3:                                                                  // le jouxte sans le contenir
        if (m[i] + 5 <= kM) {
          lo = m[i] + uni(1, 2);
          hi = lo + uni(0, 2);
        } else {
          hi = m[i] - uni(1, 2);
          lo = hi - uni(0, 2);
        }
        break;
      default: lo = uni(0, kM - 3); hi = lo + uni(0, 3); break;             // au hasard
    }
  };
  for (int mode = 0; mode < 8; ++mode) {
    AxisBox box{};
    for (int i = 0; i < 3; ++i) axis_range(mode < 6 ? mode : (int)(rng() % 6), i, box.lo[i], box.hi[i]);
    check_box(bc, box);
  }
  check_full_domain(bc);
}

// ------------------------------------------------------------ niveaux (paires)

void compare_pair(size_t i, size_t j, bool is_designated) {
  const BallCase& x = cases[i];
  const BallCase& y = cases[j];
  const Rat lx = orc::level(x.sphere), ly = orc::level(y.sphere);
  const int want = lx < ly ? -1 : (lx > ly ? 1 : 0);
  const int got = compare_exact_level(x.level, y.level);
  ++C.level_pairs;
  if (got != want || same_exact_level(x.level, y.level) != (want == 0))
    disagree("level.pair_order", ctx_of(x.family, x.support) + " vs " + ctx_of(y.family, y.support) + " levels " +
                                     str(x.level) + " vs " + str(y.level) + " got=" + std::to_string(got) +
                                     " want=" + std::to_string(want));
  if (x.has_r128 && y.has_r128) {
    ++C.level_rational_pairs;
    const int gr = compare_rational(x.r128, y.r128);
    if (gr != want)
      disagree("level.pair_rational", ctx_of(x.family, x.support) + " vs " + ctx_of(y.family, y.support) +
                                          " got=" + std::to_string(gr) + " want=" + std::to_string(want));
  }
  if (want == 0 && x.level != y.level) ++C.level_ties_distinct_repr;
  if (want != 0) {
    const Rat diff = lx > ly ? Rat(lx - ly) : Rat(ly - lx);
    if (diff * Rat(Int(1) << 40) <= std::max(lx, ly)) ++C.level_close_pairs;
  }
  if (is_designated) ++C.level_designated_pairs;
}

void run_levels() {
  for (size_t i = 0; i + 1 < cases.size(); ++i) compare_pair(i, i + 1, false);
  for (int t = 0; t < 4000; ++t) compare_pair((size_t)(rng() % cases.size()), (size_t)(rng() % cases.size()), false);
  for (const auto& [i, j] : designated) {
    compare_pair(i, j, true);
    compare_pair(j, i, true);
  }
}

// ------------------------------------------------------------ 4. plateau

const Int kI128Max = (Int(1) << 127) - 1;
const Int kI128Min = -(Int(1) << 127);

// Intermediaires de l'ancien produit scalaire i128 de la v7 : n0 v0, n1 v1,
// n2 v2, n0 v0 + n1 v1, total (l'ancien tetra_closed ecrivait n1 = -(...),
// memes magnitudes).
// `sum_overflow` : la somme EXACTE elle-meme sort de l'i128 ; `wrapped_flip` :
// meme en arithmetique qui reboucle modulo 2^128, le signe de l'ancien
// resultat serait faux (l'ancien tetra_closed decidait sur ce signe).
struct OldMag {
  unsigned bits = 0;
  bool overflow = false, sum_overflow = false, wrapped_flip = false;
};
Int wrap128(const Int& v) {
  const Int two128 = Int(1) << 128;
  Int r = (v - kI128Min) % two128;
  if (r < 0) r += two128;
  return r + kI128Min;
}
void old_dot(OldMag& m, const orc::V& n, const orc::V& v) {
  const Int t0 = n.c[0] * v.c[0], t1 = n.c[1] * v.c[1], t2 = n.c[2] * v.c[2];
  const Int s01 = t0 + t1, s = s01 + t2;
  for (const Int* x : {&t0, &t1, &t2, &s01, &s}) {
    m.bits = std::max(m.bits, bits(*x));
    if (*x > kI128Max || *x < kI128Min) m.overflow = true;
  }
  if (s > kI128Max || s < kI128Min) {
    m.sum_overflow = true;
    if (wrap128(s).sign() != s.sign()) m.wrapped_flip = true;
  }
}
orc::V centre_minus(const BallRat& c, const P3& t) {
  orc::V r;
  const Int cd = to_int(c.cden);
  r.c[0] = to_int(c.cnum[0]) - cd * t.x;
  r.c[1] = to_int(c.cnum[1]) - cd * t.y;
  r.c[2] = to_int(c.cnum[2]) - cd * t.z;
  return r;
}

void check_centre(const BallRat& c, const orc::Sphere& s, const std::string& ctx) {
  ++C.centre_checks;
  const Int cd = to_int(c.cden);
  if (cd <= 0) disagree("plateau.ball_center_den", ctx + " cden=" + cd.str());
  for (int i = 0; i < 3; ++i)
    if (to_int(c.cnum[i]) * s.d != s.n.c[i] * cd)
      disagree("plateau.ball_center", ctx + " axis=" + std::to_string(i) + " cnum=" + to_int(c.cnum[i]).str() +
                                          " cden=" + cd.str());
}

OldMag plateau_tri(const BallCase& bc, const P3& t0, const P3& t1, const P3& t2, const char* src) {
  const BallRat c = ball_center(bc.key);
  const std::string ctx = std::string("src=") + src + " " + ctx_of(bc.family, bc.support) + " triangle=" +
                          str(std::vector<P3>{t0, t1, t2});
  check_centre(c, bc.sphere, ctx);
  const int truth = orc::tri_class(bc.sphere.n, bc.sphere.d, t0, t1, t2);
  const bool got = plateau_detail::triangle_closed(c, t0, t1, t2);
  ++C.tri_checks;
  if (got != (truth >= 1))
    disagree("plateau.triangle_closed", ctx + " got=" + std::to_string(got) + " oracle_class=" + std::to_string(truth));
  if (truth == 2) ++C.tri_inside;
  if (truth == 1) ++C.tri_boundary;
  if (truth == 0) ++C.tri_outside;
  if (truth == -1) ++C.tri_degenerate;
  OldMag m;
  old_dot(m, orc::cross(orc::sub(orc::pt(t1), orc::pt(t0)), orc::sub(orc::pt(t2), orc::pt(t0))), centre_minus(c, t0));
  C.tri_max_term_bits = std::max(C.tri_max_term_bits, m.bits);
  if (m.overflow) ++C.tri_old_i128_overflow;
  if (m.sum_overflow) ++C.tri_old_sum_overflow;
  return m;
}

OldMag plateau_tet(const BallCase& bc, const std::array<P3, 4>& t, const char* src) {
  const BallRat c = ball_center(bc.key);
  const std::string ctx = std::string("src=") + src + " " + ctx_of(bc.family, bc.support) + " tetra=" +
                          str(std::vector<P3>{t[0], t[1], t[2], t[3]});
  check_centre(c, bc.sphere, ctx);
  const int truth = orc::tet_class(bc.sphere.n, bc.sphere.d, t);
  const bool got = plateau_detail::tetra_closed(c, t[0], t[1], t[2], t[3]);
  ++C.tet_checks;
  if (got != (truth >= 1))
    disagree("plateau.tetra_closed", ctx + " got=" + std::to_string(got) + " oracle_class=" + std::to_string(truth));
  if (truth == 2) ++C.tet_inside;
  if (truth == 1) ++C.tet_boundary;
  if (truth == 0) ++C.tet_outside;
  if (truth == -1) ++C.tet_degenerate;
  OldMag m;
  for (size_t f = 0; f < 4; ++f) {
    std::array<P3, 3> fp;
    size_t k = 0;
    for (size_t i = 0; i < 4; ++i)
      if (i != f) fp[k++] = t[i];
    old_dot(m, orc::cross(orc::sub(orc::pt(fp[1]), orc::pt(fp[0])), orc::sub(orc::pt(fp[2]), orc::pt(fp[0]))),
            centre_minus(c, fp[0]));
  }
  C.tet_max_term_bits = std::max(C.tet_max_term_bits, m.bits);
  if (m.overflow) ++C.tet_old_i128_overflow;
  if (m.sum_overflow) ++C.tet_old_sum_overflow;
  if (m.wrapped_flip) ++C.tet_old_wrapped_sign_flip;
  return m;
}

void plateau_pair(const BallCase& bc, const P3& t0, const P3& t1, const char* src) {
  const BallRat c = ball_center(bc.key);
  const std::string ctx = std::string("src=") + src + " " + ctx_of(bc.family, bc.support) + " pair=" +
                          str(std::vector<P3>{t0, t1});
  check_centre(c, bc.sphere, ctx);
  const orc::V s = orc::add(orc::pt(t0), orc::pt(t1));
  bool want = true;
  for (int i = 0; i < 3; ++i) want = want && 2 * bc.sphere.n.c[i] == bc.sphere.d * s.c[i];
  const bool got = plateau_detail::pair_diametral(c, t0, t1);
  ++C.pair_checks;
  if (got != want) disagree("plateau.pair_diametral", ctx + " got=" + std::to_string(got));
  if (want) ++C.pair_true;
}

void run_plateau() {
  const size_t n_cases = cases.size();
  for (size_t i = 0; i < n_cases; ++i) {
    const BallCase& bc = cases[i];
    const auto& s = bc.support;
    if (bc.lane == 2) {
      if (s[0] == s[1]) continue;
      const P3 w = rand_pt(), w2 = rand_pt();
      plateau_pair(bc, s[0], s[1], "q2_own_pair");
      plateau_pair(bc, s[1], s[0], "q2_own_pair_swapped");
      plateau_pair(bc, s[0], w, "q2_foreign_pair");
      plateau_tri(bc, s[0], w, s[1], "q2_centre_on_edge");
      plateau_tet(bc, {s[0], w, s[1], w2}, "q2_centre_on_edge");
    } else if (bc.lane == 3) {
      const P3 w = rand_pt();
      plateau_tri(bc, s[0], s[1], s[2], "q3_own");
      plateau_tri(bc, s[1], s[2], s[0], "q3_own_rotated");
      plateau_tri(bc, s[2], s[1], s[0], "q3_own_reversed");
      plateau_tri(bc, s[0], s[1], w, "q3_foreign_vertex");
      plateau_tet(bc, {s[0], s[1], s[2], w}, "q3_centre_in_face_plane");
      plateau_tet(bc, {w, s[2], s[0], s[1]}, "q3_centre_in_face_plane_perm");
      plateau_pair(bc, s[0], s[2], "q3_pair");
    } else {
      plateau_tet(bc, {s[0], s[1], s[2], s[3]}, "q4_own");
      plateau_tet(bc, {s[3], s[1], s[0], s[2]}, "q4_own_perm");
      for (size_t f = 0; f < 4; ++f) {
        std::array<P3, 3> fp;
        size_t k = 0;
        for (size_t v = 0; v < 4; ++v)
          if (v != f) fp[k++] = s[v];
        plateau_tri(bc, fp[0], fp[1], fp[2], "q4_face");
      }
    }
  }
  // Centres d'autres boules contre des supports quelconques.
  for (int t = 0; t < 400; ++t) {
    const BallCase& bc = cases[(size_t)(rng() % n_cases)];
    const BallCase& other = cases[(size_t)(rng() % n_cases)];
    if (other.support.size() >= 3) plateau_tri(bc, other.support[0], other.support[1], other.support[2], "foreign_triangle");
    if (other.support.size() == 4)
      plateau_tet(bc, {other.support[0], other.support[1], other.support[2], other.support[3]}, "foreign_tetra");
  }
  // Coquilles cospheriques entieres : centre entier, rayon 4e4 a 1e5.
  for (const auto& sp : spheres) {
    const BallCase& bc = cases[sp.key_case];
    const auto& circ = sp.circle;
    const auto& all = sp.all;
    for (size_t a = 0; a < circ.size(); ++a)
      for (size_t b = a + 1; b < circ.size(); ++b)
        for (size_t c = b + 1; c < circ.size(); ++c) plateau_tri(bc, circ[a], circ[b], circ[c], "sphere_great_circle");
    for (const auto& [p, q] : sp.antipodal) {
      plateau_pair(bc, p, q, "sphere_antipodal");
      for (int t = 0; t < 6; ++t) {
        const auto ix = distinct_indices(all.size(), 2);
        plateau_tri(bc, p, all[ix[0]], q, "sphere_antipodal_edge");
        plateau_tet(bc, {p, all[ix[0]], q, all[ix[1]]}, "sphere_antipodal_edge");
      }
    }
    for (int t = 0; t < 80; ++t) {
      const auto ix = distinct_indices(circ.size(), 3);
      plateau_tet(bc, {circ[ix[0]], sp.off[(size_t)(rng() % sp.off.size())], circ[ix[1]], circ[ix[2]]}, "sphere_face");
    }
    for (int t = 0; t < 150; ++t) {
      const auto ix = distinct_indices(all.size(), 4);
      plateau_tet(bc, {all[ix[0]], all[ix[1]], all[ix[2]], all[ix[3]]}, "sphere_random");
      plateau_tri(bc, all[ix[0]], all[ix[1]], all[ix[2]], "sphere_random");
    }
  }
  // Fixture documentee de debordement de l'ancien calcul i128 (magnitudes
  // exactes en tete de generate_fixture).
  const BallCase& fx = cases[overflow_fixture_case];
  const OldMag mt = plateau_tri(fx, kFixT0, kFixT1, kFixT2, "fixture_i128_overflow");
  const OldMag mq = plateau_tet(fx, {kFixT0, kFixT1, kFixT2, kFixW}, "fixture_i128_overflow");
  if (!mt.overflow || !mq.overflow || mt.bits != 128 || mq.bits != 128)
    harness("floor.fixture_i128_overflow", "tri_bits=" + std::to_string(mt.bits) + " tet_bits=" + std::to_string(mq.bits));
  if (to_int(fx.key.a) != Int(kFixKeyA)) disagree("plateau.fixture_key", "A=" + to_int(fx.key.a).str());
  const BallCase& fl = cases[flip_fixture_case];
  if (to_int(fl.key.a) != Int(kFlipKeyA)) disagree("plateau.fixture_key", "A=" + to_int(fl.key.a).str());
  const OldMag mf = plateau_tet(fl, {kFlipT0, kFlipT1, kFlipT2, kFlipW}, "fixture_i128_wrapped_sign_flip");
  if (!mf.wrapped_flip) harness("floor.fixture_i128_wrapped_sign_flip", "bits=" + std::to_string(mf.bits));
  if (!plateau_detail::tetra_closed(ball_center(fl.key), kFlipT0, kFlipT1, kFlipT2, kFlipW))
    disagree("plateau.fixture_wrapped_sign_flip", "the centre lies on the face (t0,t1,t2) of the closed tetrahedron");
  if (!plateau_detail::triangle_closed(ball_center(fx.key), kFixT0, kFixT1, kFixT2) ||
      !plateau_detail::tetra_closed(ball_center(fx.key), kFixT0, kFixT1, kFixT2, kFixW))
    disagree("plateau.fixture_closed", "the circumcentre of an acute triangle lies in it and in the tetrahedron face");
}

// ------------------------------------------------------------ 5. anchor_meb

void check_meb(std::vector<P3> pts, const char* family, AnchorMebWork& work) {
  std::vector<P3> uniq;
  for (const auto& p : pts)
    if (std::find(uniq.begin(), uniq.end(), p) == uniq.end()) uniq.push_back(p);
  pts = uniq;
  if (pts.empty() || pts.size() > 10) return;
  const std::string ctx = ctx_of(family, pts);
  const orc::Sphere truth = orc::meb(pts);
  const AnchorMebResult got = anchor_meb(std::span<const P3>(pts.data(), pts.size()), work);
  if (got.status != AnchorMebStatus::kOk) disagree("meb.status", ctx + " reason=" + got.reason);
  check_key(got.key, orc::form_of(truth), "meb.key", ctx);
  const Rat lt = orc::level(truth);
  if (got.level.den <= 0 || Rat(level_num(got.level), to_int(got.level.den)) != lt)
    disagree("meb.level", ctx + " got=" + str(got.level) + " want=" + lt.str());
  check_level_repr(got.level, lt, ctx);
  u8 shell = 0;
  for (const auto& p : pts)
    if (orc::scaled_power(truth, p) == 0) ++shell;
  if (got.selected_shell_count != shell)
    disagree("meb.shell_count", ctx + " got=" + std::to_string(got.selected_shell_count) + " want=" + std::to_string(shell));
  if (got.support_size < 1 || got.support_size > 4) disagree("meb.support_size", ctx);
  for (unsigned i = 0; i < got.support_size; ++i) {
    const u8 slot = got.support_slots[i];
    if (slot >= pts.size() || (i > 0 && got.support_slots[i - 1] >= slot))
      disagree("meb.support_slots", ctx);
    if (orc::scaled_power(truth, pts[slot]) != 0) disagree("meb.support_off_shell", ctx + " slot=" + std::to_string(slot));
  }
  ++C.meb_support[got.support_size];
  if (shell > got.support_size) ++C.meb_extra_shell;
  ++C.meb_sets;
  // Juge du juge : l'oracle borne local_plateau_oracle (Gauss rationnel).
  if (pts.size() <= 5) {
    namespace lpo = local_plateau_oracle;
    using BR = boost::rational<Int>;
    const lpo::Model model(pts);
    const auto ball = model.meb((u32{1} << pts.size()) - 1);
    for (int i = 0; i < 3; ++i)
      if (ball.center[(size_t)i] != BR(truth.n.c[i], truth.d)) harness("oracle.cross_meb_centre", ctx);
    if (ball.radius2 != BR(truth.q, truth.d * truth.d)) harness("oracle.cross_meb_radius", ctx);
    ++C.meb_cross_checked;
  }
}

void run_meb() {
  AnchorMebWork work;
  std::vector<P3> corners;
  for (unsigned m = 0; m < 8; ++m) {
    corners.push_back(corner(m));
    check_meb({corner(m)}, "meb_singleton_corner", work);
  }
  check_meb(corners, "meb_cube_corners", work);
  for (int t = 0; t < 40; ++t) {
    std::vector<P3> pts;
    const unsigned mask = 1 + (unsigned)(rng() % 255);
    for (unsigned m = 0; m < 8; ++m)
      if (mask & (1u << m)) pts.push_back(corner(m));
    if (t % 2) pts.push_back(P3{kM / 2, kM / 2 + 1, uni(0, kM)});
    check_meb(pts, "meb_corner_subsets", work);
  }
  for (int t = 0; t < 30; ++t) {
    std::vector<P3> pts;
    for (i64 k = uni(1, 10); k > 0; --k) pts.push_back(rand_pt());
    check_meb(pts, "meb_random", work);
  }
  for (int t = 0; t < 30; ++t) {
    std::vector<P3> pts;
    for (i64 k = uni(2, 10); k > 0; --k) pts.push_back(edge_pt());
    check_meb(pts, "meb_extreme", work);
  }
  for (int t = 0; t < 15; ++t) {
    const unsigned c0 = (unsigned)(rng() % 8);
    const P3 a = inward(corner(c0), 3), b = inward(corner(7 - c0), 3);
    std::vector<P3> pts{a, b};
    for (i64 k = uni(1, 8); k > 0; --k) {
      const i64 num = uni(1, 1023);
      pts.push_back(P3{clampc(a.x + (b.x - a.x) * num / 1024 + uni(-2, 2)),
                       clampc(a.y + (b.y - a.y) * num / 1024 + uni(-2, 2)),
                       clampc(a.z + (b.z - a.z) * num / 1024 + uni(-2, 2))});
    }
    check_meb(pts, "meb_near_collinear_diagonal", work);
  }
  for (const auto& sp : spheres) {
    for (int t = 0; t < 6; ++t) {
      std::vector<P3> pts;
      for (const size_t i : distinct_indices(sp.all.size(), (size_t)uni(4, 10))) pts.push_back(sp.all[i]);
      check_meb(pts, "meb_sphere_shell", work);
    }
    for (int t = 0; t < 3; ++t) {
      std::vector<P3> pts;
      for (const size_t i : distinct_indices(sp.circle.size(), (size_t)uni(3, 8))) pts.push_back(sp.circle[i]);
      pts.push_back(sp.c);
      check_meb(pts, "meb_great_circle", work);
    }
    std::vector<P3> ap{sp.antipodal[0].first, sp.antipodal[0].second, sp.off[1], sp.off[2], sp.c};
    check_meb(ap, "meb_antipodal", work);
  }
  for (const auto& bc : cases) {
    if (std::string_view(bc.family) != "q4_flat_disphenoid" || (rng() % 10) != 0) continue;
    std::vector<P3> pts = bc.support;
    pts.push_back(centre_floor(bc.sphere));
    check_meb(pts, "meb_flat_disphenoid", work);
  }
  for (int t = 0; t < 12; ++t) {
    std::vector<P3> pts;
    const bool high = t % 2;
    for (i64 k = uni(2, 10); k > 0; --k) {
      const P3 p{uni(0, 4), uni(0, 4), uni(0, 4)};
      pts.push_back(high ? P3{kM - p.x, kM - p.y, kM - p.z} : p);
    }
    check_meb(pts, high ? "meb_cluster_at_M" : "meb_cluster_at_0", work);
  }
}

// ------------------------------------------------------------ juge du juge

void oracle_selftest() {
  const auto expect = [](bool ok, const char* what) {
    ++C.oracle_fixed;
    if (!ok) harness(std::string("oracle.selftest.") + what, "");
  };
  // Triangle rectangle (0,0,0),(2,0,0),(0,2,0) : centre (1,1,0), rayon^2 2,
  // forme |z|^2 - 2x - 2y, centre au bord (hypotenuse).
  const auto s3 = orc::sphere3({0, 0, 0}, {2, 0, 0}, {0, 2, 0});
  expect(s3 && s3->d == 1 && s3->n.c[0] == 1 && s3->n.c[1] == 1 && s3->n.c[2] == 0 && s3->q == 2, "right_triangle");
  const orc::Form f3 = orc::form_of(*s3);
  expect(f3.a == 1 && f3.b[0] == -2 && f3.b[1] == -2 && f3.b[2] == 0 && f3.c == 0, "right_triangle_form");
  expect(orc::tri_class(s3->n, s3->d, {0, 0, 0}, {2, 0, 0}, {0, 2, 0}) == 1, "right_triangle_boundary");
  // Triangle obtus (0,0,0),(4,0,0),(1,1,0) : centre (2,-1,0), rayon^2 5, dehors.
  const auto s3o = orc::sphere3({0, 0, 0}, {4, 0, 0}, {1, 1, 0});
  expect(s3o && s3o->d == 1 && s3o->n.c[0] == 2 && s3o->n.c[1] == -1 && s3o->q == 5, "obtuse_triangle");
  expect(orc::tri_class(s3o->n, s3o->d, {0, 0, 0}, {4, 0, 0}, {1, 1, 0}) == 0, "obtuse_triangle_outside");
  // Triangle (0,0,0),(3,0,0),(0,4,0) aux coordonnees non entieres du centre
  // (3/2, 2, 0), rayon^2 25/4.
  const auto s3h = orc::sphere3({0, 0, 0}, {3, 0, 0}, {0, 4, 0});
  expect(s3h && s3h->d == 2 && s3h->n.c[0] == 3 && s3h->n.c[1] == 4 && orc::level(*s3h) == Rat(25, 4), "half_centre");
  // Tetra de coin (0,0,0),(2,0,0),(0,2,0),(0,0,2) : centre (1,1,1) dehors.
  const auto s4 = orc::sphere4({0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2});
  expect(s4 && s4->d == 1 && s4->n.c[0] == 1 && s4->n.c[1] == 1 && s4->n.c[2] == 1 && s4->q == 3, "corner_tetra");
  expect(orc::tet_class(s4->n, s4->d, {P3{0, 0, 0}, P3{2, 0, 0}, P3{0, 2, 0}, P3{0, 0, 2}}) == 0, "corner_tetra_outside");
  // Tetra regulier (2,0,0),(0,2,0),(0,0,2),(2,2,2) : centre (1,1,1) interieur.
  const auto s4r = orc::sphere4({2, 0, 0}, {0, 2, 0}, {0, 0, 2}, {2, 2, 2});
  expect(s4r && s4r->d == 1 && s4r->n.c[0] == 1 && s4r->q == 3, "regular_tetra");
  expect(orc::tet_class(s4r->n, s4r->d, {P3{2, 0, 0}, P3{0, 2, 0}, P3{0, 0, 2}, P3{2, 2, 2}}) == 2, "regular_tetra_inside");
  const orc::Sphere mb = orc::meb({{2, 0, 0}, {0, 2, 0}, {0, 0, 2}, {2, 2, 2}, {1, 1, 1}});
  expect(mb.d == 1 && mb.n.c[0] == 1 && mb.n.c[1] == 1 && mb.n.c[2] == 1 && mb.q == 3, "meb_regular_tetra");
  expect(!orc::sphere3({0, 0, 0}, {1, 1, 1}, {5, 5, 5}) && !orc::sphere4({0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}),
         "degenerate_supports");
}

// Recoupement du centre et du rayon avec local_plateau_oracle::detail::
// support_ball (Gauss sur boost::rational) sur les supports bien centres.
void oracle_cross_supports() {
  namespace lpo = local_plateau_oracle;
  using BR = boost::rational<Int>;
  for (size_t i = 0; i < cases.size(); i += 3) {
    const BallCase& bc = cases[i];
    if (bc.lane < 3) continue;
    const std::vector<P3> pts = bc.support;
    const auto ball = lpo::detail::support_ball(pts, (u32{1} << pts.size()) - 1);
    if (!ball) continue;
    for (int k = 0; k < 3; ++k)
      if (ball->center[(size_t)k] != BR(bc.sphere.n.c[k], bc.sphere.d))
        harness("oracle.cross_support_centre", ctx_of(bc.family, bc.support));
    if (ball->radius2 != BR(bc.sphere.q, bc.sphere.d * bc.sphere.d))
      harness("oracle.cross_support_radius", ctx_of(bc.family, bc.support));
    ++C.oracle_cross_supports;
  }
}

// ------------------------------------------------------------ planchers

void floor_at_least(u64 value, u64 minimum, const char* name) {
  if (value < minimum)
    harness(std::string("floor.") + name, "value=" + std::to_string(value) + " minimum=" + std::to_string(minimum));
}

// Planchers de non-vacuite : ~55 a 75 % des valeurs deterministes observees
// (graine 20260922) ; les cas adversariaux nommes ont chacun le leur.
void check_floors() {
  floor_at_least(C.oracle_fixed, 12, "oracle_fixed");
  floor_at_least(C.oracle_cross_supports, 150, "oracle_cross_supports");
  floor_at_least(C.q2_keys, 400, "q2_keys");
  floor_at_least(C.q3_keys, 1100, "q3_keys");
  floor_at_least(C.q4_keys, 900, "q4_keys");
  floor_at_least(C.q3_near_degenerate, 150, "q3_near_degenerate");
  floor_at_least(C.q4_flat_inside, 80, "q4_flat_inside");
  floor_at_least(C.q4_flat_far, 60, "q4_flat_far");
  floor_at_least(C.q4_inside_true, 100, "q4_inside_true");
  floor_at_least(C.level_repr_checks, 12000, "level_repr_checks");
  floor_at_least(C.level_pairs, 6000, "level_pairs");
  floor_at_least(C.level_rational_pairs, 2500, "level_rational_pairs");
  floor_at_least(C.level_close_pairs, 200, "level_close_pairs");
  floor_at_least(C.level_ties_distinct_repr, 200, "level_ties_distinct_repr");
  floor_at_least(C.level_designated_pairs, 500, "level_designated_pairs");
  floor_at_least(C.power_checks, 40000, "power_checks");
  floor_at_least(C.power_shell_zero, 7000, "power_shell_zero");
  floor_at_least(C.power_high, 3000, "power_high");
  floor_at_least(C.form_power_checks, 30000, "form_power_checks");
  floor_at_least(C.boxes, 20000, "boxes");
  floor_at_least(C.boxes_min_interior, 1000, "boxes_min_interior");
  floor_at_least(C.boxes_u16_clip_overestimate, 5000, "boxes_u16_clip_overestimate");
  floor_at_least(C.full_domain_boxes, 2500, "full_domain_boxes");
  floor_at_least(C.tri_checks, 8000, "tri_checks");
  floor_at_least(C.tri_inside, 1000, "tri_inside");
  floor_at_least(C.tri_boundary, 500, "tri_boundary");
  floor_at_least(C.tri_outside, 5000, "tri_outside");
  floor_at_least(C.tet_checks, 4000, "tet_checks");
  floor_at_least(C.tet_inside, 250, "tet_inside");
  floor_at_least(C.tet_boundary, 1000, "tet_boundary");
  floor_at_least(C.tet_outside, 2000, "tet_outside");
  floor_at_least(C.pair_checks, 2000, "pair_checks");
  floor_at_least(C.pair_true, 600, "pair_true");
  floor_at_least(C.tri_old_i128_overflow, 50, "tri_old_i128_overflow");
  floor_at_least(C.tet_old_i128_overflow, 30, "tet_old_i128_overflow");
  floor_at_least(C.tet_old_wrapped_sign_flip, 1, "tet_old_wrapped_sign_flip");
  floor_at_least(C.meb_sets, 150, "meb_sets");
  floor_at_least(C.meb_cross_checked, 60, "meb_cross_checked");
  floor_at_least(C.meb_extra_shell, 30, "meb_extra_shell");
  for (size_t q = 1; q <= 4; ++q) floor_at_least(C.meb_support[q], 5, "meb_support_arity");
}

unsigned long long ull(u64 v) { return static_cast<unsigned long long>(v); }

void print_json() {
  std::printf(
      "{\"status\":\"passed\",\"gate\":\"arith_u18\",\"coord_max\":%lld,\"seed\":20260922,"
      "\"keys\":{\"q2\":%llu,\"q3\":%llu,\"q4\":%llu,\"q3_degenerate\":%llu,\"q4_degenerate\":%llu,"
      "\"q3_near_degenerate\":%llu,\"q4_flat_inside\":%llu,\"q4_flat_far\":%llu,\"q4_inside_checks\":%llu,"
      "\"q4_inside_true\":%llu},"
      "\"levels\":{\"repr_checks\":%llu,\"pairs\":%llu,\"rational_pairs\":%llu,\"close_pairs\":%llu,"
      "\"ties_distinct_repr\":%llu,\"designated_pairs\":%llu},"
      "\"power\":{\"checks\":%llu,\"shell_zero\":%llu,\"above_2e100\":%llu,\"form_checks\":%llu},"
      "\"axis_bounds\":{\"boxes\":%llu,\"points\":%llu,\"min_interior\":%llu,\"beyond_u16\":%llu,"
      "\"u16_clip_overestimate\":%llu,\"full_domain\":%llu},"
      "\"plateau\":{\"triangle\":%llu,\"tri_inside\":%llu,\"tri_boundary\":%llu,\"tri_outside\":%llu,"
      "\"tri_degenerate\":%llu,\"tetra\":%llu,\"tet_inside\":%llu,\"tet_boundary\":%llu,\"tet_outside\":%llu,"
      "\"tet_degenerate\":%llu,\"pair\":%llu,\"pair_true\":%llu,\"centres\":%llu,"
      "\"tri_old_i128_overflow\":%llu,\"tet_old_i128_overflow\":%llu,\"tri_old_sum_overflow\":%llu,"
      "\"tet_old_sum_overflow\":%llu,\"tet_old_wrapped_sign_flip\":%llu,\"tri_max_term_bits\":%u,"
      "\"tet_max_term_bits\":%u},"
      "\"anchor_meb\":{\"sets\":%llu,\"support\":[%llu,%llu,%llu,%llu],\"extra_shell\":%llu,"
      "\"cross_checked\":%llu},"
      "\"oracle\":{\"fixed\":%llu,\"cross_supports\":%llu},"
      "\"observed_bits\":{\"q2_b\":%u,\"q2_c\":%u,\"q3_a\":%u,\"q3_b\":%u,\"q3_c\":%u,\"q3_level_num\":%u,"
      "\"q3_level_den\":%u,\"q3_power\":%u,\"q4_det\":%u,\"q4_np\":%u,\"q4_a\":%u,\"q4_b\":%u,\"q4_c\":%u,"
      "\"q4_level_num\":%u,\"q4_level_den\":%u,\"q4_power\":%u},"
      "\"gcp_used\":false,\"public_status\":\"not_claimed\"}\n",
      static_cast<long long>(kM), ull(C.q2_keys), ull(C.q3_keys), ull(C.q4_keys), ull(C.q3_degenerate),
      ull(C.q4_degenerate), ull(C.q3_near_degenerate), ull(C.q4_flat_inside), ull(C.q4_flat_far),
      ull(C.q4_inside_checks), ull(C.q4_inside_true), ull(C.level_repr_checks), ull(C.level_pairs),
      ull(C.level_rational_pairs), ull(C.level_close_pairs), ull(C.level_ties_distinct_repr),
      ull(C.level_designated_pairs), ull(C.power_checks), ull(C.power_shell_zero), ull(C.power_high),
      ull(C.form_power_checks), ull(C.boxes), ull(C.box_points), ull(C.boxes_min_interior), ull(C.boxes_beyond_u16),
      ull(C.boxes_u16_clip_overestimate), ull(C.full_domain_boxes), ull(C.tri_checks), ull(C.tri_inside),
      ull(C.tri_boundary), ull(C.tri_outside), ull(C.tri_degenerate), ull(C.tet_checks), ull(C.tet_inside),
      ull(C.tet_boundary), ull(C.tet_outside), ull(C.tet_degenerate), ull(C.pair_checks), ull(C.pair_true),
      ull(C.centre_checks), ull(C.tri_old_i128_overflow), ull(C.tet_old_i128_overflow), ull(C.tri_old_sum_overflow),
      ull(C.tet_old_sum_overflow), ull(C.tet_old_wrapped_sign_flip), C.tri_max_term_bits,
      C.tet_max_term_bits, ull(C.meb_sets), ull(C.meb_support[1]), ull(C.meb_support[2]), ull(C.meb_support[3]),
      ull(C.meb_support[4]), ull(C.meb_extra_shell), ull(C.meb_cross_checked), ull(C.oracle_fixed),
      ull(C.oracle_cross_supports), C.q2_b, C.q2_c, C.q3_a, C.q3_b, C.q3_c, C.q3_num, C.q3_den, C.q3_power, C.q4_det,
      C.q4_np, C.q4_a, C.q4_b, C.q4_c, C.q4_num, C.q4_den, C.q4_power);
}

void run() {
  oracle_selftest();
  generate_q2();
  generate_q3();
  generate_q4();
  generate_spheres();
  generate_fixture();
  for (const auto& bc : cases) {
    check_powers(bc);
    check_boxes(bc);
  }
  oracle_cross_supports();
  run_levels();
  run_plateau();
  run_meb();
  check_floors();
  print_json();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::printf("cause=argument.refused\n");
    return 2;
  }
  try {
    run();
    return 0;
  } catch (const Failure& failure) {
    std::printf("cause=%s\n", failure.cause.c_str());
    std::fprintf(stderr, "%s\n", failure.detail.c_str());
    return failure.code;
  } catch (const std::exception& error) {
    std::printf("cause=harness.exception\n");
    std::fprintf(stderr, "%s\n", error.what());
    return 3;
  }
}
