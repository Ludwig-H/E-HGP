// Predicats geometriques exacts sur coordonnees entieres du palier servi (B <= kEngineMaxBits = 21 bits).
//
// Une sphere candidate est donnee par un site d'ancrage a et son centre rationnel c = a + N / D (D > 0),
// ou N et D sont des i128. Le cote d'un site z : signe de D|z-a|^2 - 2 N.(z-a) (< 0 interieur strict, 0 coquille).
// Niveau r^2 = num / den : q2 |u|^2 / 4 ; q3 |u|^2 |v|^2 |u-v|^2 / (4 |u x v|^2) ; q4 |N|^2 / D^2.
//
// Bornes (E = 2^B - 1 majore toute coordonnee et toute difference ; formes construites sur des points du domaine ;
// derivation par inegalite triangulaire, dans l'ordre d'evaluation du code ; entre parentheses : bits a B = 18 / 21) :
//  q2 : N = b - a, |N_i| <= E ; D = 2.
//  q3 : u, v differences, w = u x v : |w_i| <= 2 E^2 ; uu, vv <= 3 E^2 ; t = uu v - vv u : |t_i| <= 6 E^3 ;
//       N = t x w : |N_i| <= 2 (6 E^3)(2 E^2) = 24 E^5 (95 / 110) ; D = 2 |w|^2 <= 24 E^4 (77 / 89).
//  q4 : |det| <= 6 E^3 (six termes de Leibniz), D = 2 |det| <= 12 E^3 (58 / 67) ;
//       N_i = uu (v x s)_i + vv (s x u)_i + ss (u x v)_i : |N_i| <= 3 (3 E^2)(2 E^2) = 18 E^4 (77 / 89).
//  cote q3 : D |d|^2 <= 72 E^6 et 2 N.d <= 144 E^6 (116 / 134 bits) : au-dela de l'i128 des B = 20 ; cle <= 216 E^6.
//  cote q4 : D |d|^2 <= 36 E^5, 2 N.d <= 108 E^5 (97 / 112) ; cote q2 : <= 6 E^2.
//  orientation du centre (plan p, q, r) : |w_i| <= 2 E^2 ; cc_i = N_i + D (a_i - p_i) : q4 <= 30 E^4 (77 / 89),
//       q3 <= 48 E^5 (96 / 111) ; somme q4 <= 3 (2 E^2)(30 E^4) = 180 E^6 (116 / 134) : au-dela de l'i128 des B = 20.
//  niveaux : q3 num <= 27 E^6 (113 / 131), den = 4 |w|^2 <= 48 E^4 (78 / 90) ; q4 num = |N|^2 <= 972 E^8 (154 / 178),
//       den = D^2 <= 144 E^6 (116 / 134 : le produit u128 de la v10 d'origine tronquait des B = 21) ; num et den sur
//       192 bits (I192) jusqu'a B = 22 ; a B >= 23, level4 refuse (arith_guard) au lieu de tronquer.
//  comparaison de niveaux : num den' < 2^384, produit Wide<6> par construction.
// Les cotes et orientations sont DISPATCHES : voie courte i128 si une condition prouvee sur les longueurs en bits des
// magnitudes reelles la garantit (side_fits_narrow, orient_fits_narrow), voie large sinon ; la voie large est exacte
// par construction pour tout centre i128 et toutes differences i64 (sommes et produits a largeur croissante).
#pragma once

#include <algorithm>

#include "arith/wide.hpp"
#include "core/types.hpp"

namespace mhgp10::geom {

struct P3 {
  i64 x, y, z;
};

inline P3 sub(const P3& a, const P3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline i64 dot(const P3& a, const P3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline P3 cross(const P3& a, const P3& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

struct Center {
  i128 N[3];  // relatif au site d'ancrage
  i128 D;     // > 0
};

// Niveau exact (rayon carre) : num / den, den > 0 (64 octets).
struct Level {
  arith::I192 num;
  arith::I192 den;
  double approx() const;  // erreur relative < 2^-50 (arith::to_double)
};

// Comparaison exacte de deux niveaux : -1, 0, 1.
int compare(const Level& a, const Level& b);

// num / den <= e, exact : comparaison directe num <= e den, sans raccourci (num >= 0, den > 0). Generique en largeurs
// (niveaux I192 aujourd'hui, Wide<4> au palier B24) : e den tient sur D + 1 mots par construction, la comparaison se
// fait sur la plus grande des deux largeurs. Audit AT1 / PR8 : l'ancien raccourci « e den ne tient pas => vrai »
// supposait num < 2^192 ; il devient faux des que num s'elargit.
template <int NumL, int DenL>
bool level_at_most(const arith::Wide<NumL>& num, const arith::Wide<DenL>& den, u64 e) {
  constexpr int W = NumL > DenL + 1 ? NumL : DenL + 1;
  const arith::Wide<DenL + 1> ed = arith::mul(arith::Wide<1>::from_u64(e), den);
  return arith::cmp(arith::widen<W>(num), arith::widen<W>(ed)) <= 0;
}
inline bool level_at_most(const Level& l, u64 e) { return level_at_most(l.num, l.den, e); }

// Centre du cercle circonscrit (dans le plan) de a, b, c relatif a a ; false si alignes.
inline bool center3(const P3& a, const P3& b, const P3& c, Center& out) {
  const P3 u = sub(b, a), v = sub(c, a);
  const P3 w = cross(u, v);
  if (w.x == 0 && w.y == 0 && w.z == 0) return false;
  const i128 uu = dot(u, u), vv = dot(v, v);
  const i128 tx = uu * v.x - vv * u.x, ty = uu * v.y - vv * u.y, tz = uu * v.z - vv * u.z;
  out.N[0] = ty * w.z - tz * w.y;
  out.N[1] = tz * w.x - tx * w.z;
  out.N[2] = tx * w.y - ty * w.x;
  out.D = 2 * (i128(w.x) * w.x + i128(w.y) * w.y + i128(w.z) * w.z);
  return true;
}

// Centre de la sphere circonscrite a a, b, c, d relatif a a ; false si coplanaires.
inline bool center4(const P3& a, const P3& b, const P3& c, const P3& d, Center& out) {
  const P3 u = sub(b, a), v = sub(c, a), s = sub(d, a);
  const i128 det = i128(u.x) * (i128(v.y) * s.z - i128(v.z) * s.y) - i128(u.y) * (i128(v.x) * s.z - i128(v.z) * s.x) +
                   i128(u.z) * (i128(v.x) * s.y - i128(v.y) * s.x);
  if (det == 0) return false;
  const i128 uu = dot(u, u), vv = dot(v, v), ss = dot(s, s);
  const i128 vsx = i128(v.y) * s.z - i128(v.z) * s.y, vsy = i128(v.z) * s.x - i128(v.x) * s.z,
             vsz = i128(v.x) * s.y - i128(v.y) * s.x;
  const i128 sux = i128(s.y) * u.z - i128(s.z) * u.y, suy = i128(s.z) * u.x - i128(s.x) * u.z,
             suz = i128(s.x) * u.y - i128(s.y) * u.x;
  const i128 uvx = i128(u.y) * v.z - i128(u.z) * v.y, uvy = i128(u.z) * v.x - i128(u.x) * v.z,
             uvz = i128(u.x) * v.y - i128(u.y) * v.x;
  i128 D = 2 * det;
  i128 N0 = uu * vsx + vv * sux + ss * uvx, N1 = uu * vsy + vv * suy + ss * uvy, N2 = uu * vsz + vv * suz + ss * uvz;
  if (D < 0) {
    D = -D;
    N0 = -N0;
    N1 = -N1;
    N2 = -N2;
  }
  out.N[0] = N0;
  out.N[1] = N1;
  out.N[2] = N2;
  out.D = D;
  return true;
}

inline void center2(const P3& a, const P3& b, Center& out) {
  out.N[0] = b.x - a.x;
  out.N[1] = b.y - a.y;
  out.N[2] = b.z - a.z;
  out.D = 2;
}

// ---------------------------------------------------------------- voies courte et large

// Longueur en bits de |v| (0 pour 0).
inline int bits_abs(i64 v) {
  const u64 m = v < 0 ? u64(0) - static_cast<u64>(v) : static_cast<u64>(v);
  return m ? 64 - __builtin_clzll(m) : 0;
}
inline int bits_abs(i128 v) {
  const u128 m = v < 0 ? u128(0) - static_cast<u128>(v) : static_cast<u128>(v);
  const u64 hi = static_cast<u64>(m >> 64), lo = static_cast<u64>(m);
  return hi ? 128 - __builtin_clzll(hi) : (lo ? 64 - __builtin_clzll(lo) : 0);
}
inline int bits_max_n(const Center& c) {
  return std::max(bits_abs(c.N[0]), std::max(bits_abs(c.N[1]), bits_abs(c.N[2])));
}

// Signe impossible, rendu par un predicat en mode narrow quand sa condition de voie courte est fausse (refus explicite,
// l'appelant le transforme en Reason::arith_guard).
inline constexpr int kArithRefused = 2;

// Voie courte du cote et de la cle : bd = b(max_i |d_i|), d = z - a. Si b(D) + 2 bd + 2 <= 125 et
// max b(N_i) + bd + 3 <= 125, alors |d|^2 <= 3 max|d_i|^2 < 2^(2 bd + 2), lhs = D |d|^2 < 2^(b(D) + 2 bd + 2) <= 2^125,
// |N_i d_i| < 2^(b(N) + bd), |N.d| < 2^(b(N) + bd + 2), rhs = 2 N.d < 2^125 : produits, sommes partielles,
// comparaison et difference lhs - rhs (< 2^126) tiennent en i128. Cout : quelques clz.
inline bool side_fits_narrow(const Center& c, int bd) {
  return bits_abs(c.D) + 2 * bd + 2 <= 125 && bits_max_n(c) + bd + 3 <= 125;
}
inline int bits_max_d(const P3& a, const P3& z) {
  return std::max(bits_abs(z.x - a.x), std::max(bits_abs(z.y - a.y), bits_abs(z.z - a.z)));
}

// Cle de puissance exacte s(z) = D |z - a|^2 - 2 N.(z - a) = D (|z - c|^2 - r^2), en i128 : precondition
// side_fits_narrow(c, bits_max_d(a, z)) (sinon debordement). Toute forme q2/q3/q4 de points u18 la satisfait (|s| < 2^122).
inline i128 side_key(const Center& c, const P3& a, const P3& z) {
  const i128 dx = z.x - a.x, dy = z.y - a.y, dz = z.z - a.z;
  return c.D * (dx * dx + dy * dy + dz * dz) - 2 * (c.N[0] * dx + c.N[1] * dy + c.N[2] * dz);
}

// Cote du site z, voie courte i128 : -1 interieur, 0 coquille, 1 exterieur ; meme precondition que side_key.
inline int side_narrow(const Center& c, const P3& a, const P3& z) {
  const i128 dx = z.x - a.x, dy = z.y - a.y, dz = z.z - a.z;
  const i128 lhs = c.D * (dx * dx + dy * dy + dz * dz);
  const i128 rhs = 2 * (c.N[0] * dx + c.N[1] * dy + c.N[2] * dz);
  return lhs < rhs ? -1 : (lhs == rhs ? 0 : 1);
}

// Voie large, exacte par construction pour tout centre i128 et |z_i - a_i| < 2^63 (differences i64) : d_i^2 < 2^126
// (Wide<2>), |d|^2 somme a largeur croissante (Wide<4>), lhs = D |d|^2 (Wide<6>) ; N_i d_i (Wide<3>), somme (Wide<5>),
// rhs = 2 N.d (Wide<6>) ; cle = lhs - rhs (Wide<7>). Aucune operation ne peut deborder : aucun cas d'echec.
int side_wide(const Center& c, const P3& a, const P3& z);
arith::Wide<7> side_key_wide(const Center& c, const P3& a, const P3& z);

// Cote dispatche (defaut du moteur) : voie courte si sa condition tient, sinon voie large. Exact pour tout centre i128.
inline int side(const Center& c, const P3& a, const P3& z) {
  return side_fits_narrow(c, bits_max_d(a, z)) ? side_narrow(c, a, z) : side_wide(c, a, z);
}

// Cote selon le mode ; `wide` compte les appels servis par la voie large. narrow : kArithRefused si la condition est
// fausse (jamais de calcul faux).
inline int side(ArithMode mode, const Center& c, const P3& a, const P3& z, u64& wide) {
  if (mode != ArithMode::wide && side_fits_narrow(c, bits_max_d(a, z))) return side_narrow(c, a, z);
  if (mode == ArithMode::narrow) return kArithRefused;
  ++wide;
  return side_wide(c, a, z);
}

// Orientation de s par rapport au plan (p, q, r) : signe de det[q-p, r-p, s-p] (6 E^3 : i128 jusqu'a B = 41).
inline int orient(const P3& p, const P3& q, const P3& r, const P3& s) {
  const P3 a = sub(q, p), b = sub(r, p), c = sub(s, p);
  const i128 v = i128(a.x) * (i128(b.y) * c.z - i128(b.z) * c.y) - i128(a.y) * (i128(b.x) * c.z - i128(b.z) * c.x) +
                 i128(a.z) * (i128(b.x) * c.y - i128(b.y) * c.x);
  return v > 0 ? 1 : (v < 0 ? -1 : 0);
}

// Orientation du centre rationnel (anchor + N/D) par rapport au plan (p, q, r) : signe de w . cc, w = (q - p) x (r - p),
// cc = N + D (anchor - p) (meme signe que w . (c - p), D > 0).
//
// Voie courte i128 (orient_fits_narrow) : b(D) + b(max |anchor_i - p_i|) <= 125 et max b(N_i) <= 125 donnent
// |cc_i| < 2^126 sans debordement ; puis max b(w_i) + max b(cc_i) + 2 <= 125 donne chaque produit < 2^123 et la somme
// des trois < 2^125. Toute forme q4 d'une feuille d'etendue < 2^19 la satisfait (b(w) <= 39, b(cc) <= 81 : 122).
// Precondition des deux voies : differences (q - p), (r - p), (anchor - p) sous 2^62 en valeur absolue (w_i < 2^125).
bool orient_fits_narrow(const P3& p, const P3& q, const P3& r, const P3& anchor, const Center& c);
int orient_center_narrow(const P3& p, const P3& q, const P3& r, const P3& anchor, const Center& c);
// Voie large, exacte par construction pour tout centre i128 : cc_i = N_i + D (anchor_i - p_i) en Wide<4> (somme de deux
// Wide<3>), produits w_i cc_i en Wide<6>, somme des trois en Wide<8>.
int orient_center_wide(const P3& p, const P3& q, const P3& r, const P3& anchor, const Center& c);
// Dispatche (defaut du moteur) : termes i128 calcules une fois, condition testee, voie large si elle est fausse.
int orient_center(const P3& p, const P3& q, const P3& r, const P3& anchor, const Center& c);
// Selon le mode (memes conventions que side) ; `wide` compte les appels servis par la voie large.
int orient_center(ArithMode mode, const P3& p, const P3& q, const P3& r, const P3& anchor, const Center& c, u64& wide);

// Triangle strictement aigu (centre circonscrit strictement interieur).
inline bool acute(const P3& a, const P3& b, const P3& c) {
  return dot(sub(b, a), sub(c, a)) > 0 && dot(sub(a, b), sub(c, b)) > 0 && dot(sub(a, c), sub(b, c)) > 0;
}

// Centre (anchor + N/D) strictement interieur au tetraedre t[0..3] (orientations dispatchees).
bool strictly_inside_tetra(const P3* t[4], const P3& anchor, const Center& c);
// Selon le mode : 1 interieur strict, 0 sinon, kArithRefused si le mode narrow refuse une orientation.
int strictly_inside_tetra(ArithMode mode, const P3* t[4], const P3& anchor, const Center& c, u64& wide);
// Voie courte seule : precondition orient_fits_narrow pour les quatre faces (feuille d'etendue < 2^19 du generateur).
bool strictly_inside_tetra_narrow(const P3* t[4], const P3& anchor, const Center& c);

// Centre (anchor + N/D) dans le plan de a, b, c (voie large : exact pour tout centre i128).
inline bool center_in_plane(const P3& a, const P3& b, const P3& c, const P3& anchor, const Center& ctr) {
  return orient_center_wide(a, b, c, anchor, ctr) == 0;
}

// Milieu de a, b egal au centre (anchor + N/D) : 2 (anchor D + N) = (a + b) D. i128 : q3 96 E^5 (112 bits a B = 21),
// valide jusqu'a B = 23.
inline bool is_midpoint(const P3& a, const P3& b, const P3& anchor, const Center& c) {
  return 2 * (anchor.x * c.D + c.N[0]) == (i128(a.x) + b.x) * c.D &&
         2 * (anchor.y * c.D + c.N[1]) == (i128(a.y) + b.y) * c.D &&
         2 * (anchor.z * c.D + c.N[2]) == (i128(a.z) + b.z) * c.D;
}

// Niveaux exacts des trois formes. level3 et level4 rendent false si num ou den ne tient pas sur 192 bits : impossible
// dans le palier servi (B <= 21, bornes de l'en-tete), garde de palier (l'appelant rend Reason::arith_guard).
Level level2(const P3& a, const P3& b);
[[nodiscard]] bool level3(const P3& a, const P3& b, const P3& c, Level& out);
[[nodiscard]] bool level4(const Center& c, Level& out);

// Marge certifiee des filtres de distances carrees en double (SiteTree, tour), pour sites, ancre et centre dans le cube
// [0, L]^3, L = 2^bits - 1. Erreur absolue d'une distance carree approchee au centre approche (preuve dans
// site_tree.cpp) : eps(B) = 3 gamma_5 (L + delta)^2 + 3 delta (2 L + delta), delta = 4,02 u L, u = 2^-53, donc
// eps(B) <= 39,2 u L^2 < 4,9 2^(2B - 50). Deux distances interviennent dans chaque decision : la marge doit majorer
// 2 eps(B) <= 4,9 2^(2B - 49). filter_margin(B) = max(0,02 ; 5 2^(2B - 49)) : 0,02 pour B <= 20 (2 eps(20) = 9,6e-3 ;
// sorties u18 inchangees), 0,0390625 a B = 21 (2 eps(21) = 0,0382). Chaque decision compare l'ecart approche des deux
// distances a +-marge (double exact, arrondi monotone) : aucun seuil arrondi r2a +- marge, dont l'arrondi pres de 2^43
// mangerait la reserve de B21 (m - 2 eps(21) = 8,6e-4 < 2^-10). Portes : FX-M (valeur) et FX-BANDE (sites d'usage) de
// mhgp10_precision_b21.
double filter_margin(int bits);

}  // namespace mhgp10::geom
