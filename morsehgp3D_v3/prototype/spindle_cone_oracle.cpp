// MorseHGP3D v3 — JUGE INDEPENDANT DU CONE CIBLE.
//
// ---------------------------------------------------------------------------
// CE QUI REND CE FICHIER INDEPENDANT
//
// 1. Il n'inclut AUCUN en-tete de production : ni `spindle_cone.hpp`, ni
//    `anchor_envelope.hpp`, ni `mhgp/mhgp.hpp`. Le compilateur garantit donc
//    qu'aucune ligne du sujet ne s'execute ici.
// 2. Il REDEFINIT le seuil de mort de lane. Le sujet appelle
//    `anchor::lane_death_threshold(int, int)`; le contre-audit du 13 aout 2026
//    a montre qu'un `smax` hors largeur `int` y produit des seuils negatifs, et
//    que le juge partageant l'appel fermait alors les memes 380/380 paires sans
//    un seul temoin. Ici le seuil est calcule en `long long`, et un `smax` hors
//    domaine REFUSE au lieu de convertir.
// 3. Il REECRIT son arithmetique 128 bits a la main, sur deux limbes `u64` en
//    complement a deux. La production emploie le `__int128` du compilateur. Un
//    defaut de l'un ne peut donc pas se compenser dans l'autre.
// 4. Cette arithmetique est elle-meme jugee : `selftest_against_bigint` la
//    compare a `mhgp3v::BigInt` (signe et magnitude, chiffres de 32 bits), qui
//    est encore une troisieme representation.
//
// ---------------------------------------------------------------------------
// LE PREDICAT
//
// Pour une ancre orientee `(a,b)` et un temoin `z`, dans l'ecriture historique
// du lemme du noeud spindle :
//
//   d = b-a,  U = 2z-a-b,  D2 = d.d,  UU = U.U,  UD = U.d,
//   g = D2 - UU,  Q = D2*UU - UD^2,
//   q2 : g > 0        q3 : g > 0 et 3g^2 > 4Q        q4 : g > 0 et g^2 > 2Q.
//
// Largeurs sur le profil u16, coordonnees dans [0,65535] :
//   |d_i| <= 65535, |U_i| <= 131070,
//   D2 <= 3*65535^2 = 1,29e10,  UU <= 3*131070^2 = 5,15e10,
//   |UD| <= 3*65535*131070 = 2,58e10,   tous dans i64 ;
//   Q <= D2*UU <= 6,64e20 et g^2 <= 1,66e20  -> 70 bits,
//   4Q <= 2,66e21 et 3g^2 <= 4,98e20         -> 72 bits, donc 128 bits suffisent.
#include "prototype/spindle_cone_oracle.hpp"

#include "oracle/bigint.hpp"

namespace mhgp3v {
namespace cone_oracle {
namespace {

// ---------------------------------------------------------------------------
// ENTIER SIGNE 128 BITS, ECRIT ICI, EN COMPLEMENT A DEUX SUR DEUX LIMBES.
// Aucune ligne de ce bloc n'existe ailleurs dans le depot.
// ---------------------------------------------------------------------------
struct S128 {
  std::uint64_t lo = 0;
  std::uint64_t hi = 0;
};

inline S128 neg128(S128 v) {
  S128 r;
  r.lo = ~v.lo + 1ULL;
  r.hi = ~v.hi + (r.lo == 0ULL ? 1ULL : 0ULL);
  return r;
}

inline S128 add128(S128 a, S128 b) {
  S128 r;
  r.lo = a.lo + b.lo;
  r.hi = a.hi + b.hi + (r.lo < a.lo ? 1ULL : 0ULL);
  return r;
}

inline S128 sub128(S128 a, S128 b) { return add128(a, neg128(b)); }

// Produit non signe 64x64 -> 128 par decoupage en demi-mots de 32 bits.
inline void umul64(std::uint64_t a, std::uint64_t b, std::uint64_t* hi, std::uint64_t* lo) {
  const std::uint64_t a0 = a & 0xFFFFFFFFULL, a1 = a >> 32;
  const std::uint64_t b0 = b & 0xFFFFFFFFULL, b1 = b >> 32;
  const std::uint64_t p00 = a0 * b0;
  const std::uint64_t p01 = a0 * b1;
  const std::uint64_t p10 = a1 * b0;
  const std::uint64_t p11 = a1 * b1;
  const std::uint64_t mid = (p00 >> 32) + (p01 & 0xFFFFFFFFULL) + (p10 & 0xFFFFFFFFULL);
  *lo = (p00 & 0xFFFFFFFFULL) | (mid << 32);
  *hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
}

// Valeur absolue definie meme pour LLONG_MIN : le complement a deux non signe
// est exactement la magnitude.
inline std::uint64_t magnitude(long long v) {
  return v < 0 ? (~static_cast<std::uint64_t>(v) + 1ULL) : static_cast<std::uint64_t>(v);
}

inline S128 mul_i64(long long a, long long b) {
  S128 r;
  umul64(magnitude(a), magnitude(b), &r.hi, &r.lo);
  const bool negative = (a < 0) != (b < 0);
  return negative ? neg128(r) : r;
}

inline S128 from_i64(long long v) {
  S128 r;
  r.lo = static_cast<std::uint64_t>(v);
  r.hi = (v < 0) ? ~0ULL : 0ULL;
  return r;
}

// Multiplication par un petit entier POSITIF (2, 3, 4 seulement ici), par
// additions repetees : le resultat reste dans 128 bits sur ce domaine.
inline S128 mul_small(S128 v, int k) {
  S128 r = from_i64(0);
  for (int i = 0; i < k; ++i) r = add128(r, v);
  return r;
}

// Comparaison signee : le signe est le bit de poids fort de `hi`.
inline int cmp128(S128 a, S128 b) {
  const bool na = (a.hi >> 63) != 0ULL;
  const bool nb = (b.hi >> 63) != 0ULL;
  if (na != nb) return na ? -1 : 1;
  if (a.hi != b.hi) return a.hi < b.hi ? -1 : 1;
  if (a.lo != b.lo) return a.lo < b.lo ? -1 : 1;
  return 0;
}

// ---------------------------------------------------------------------------
// LANE LA PLUS FORTE SATISFAITE, DANS L'ARITHMETIQUE CI-DESSUS.
// Rend 0, 2, 3 ou 4.
// ---------------------------------------------------------------------------
int lane_of(const long long a[3], const long long b[3], const long long z[3]) {
  long long d[3], u[3];
  for (int i = 0; i < 3; ++i) {
    d[i] = b[i] - a[i];
    u[i] = 2 * z[i] - a[i] - b[i];
  }
  const long long d2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
  const long long uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
  const long long ud = u[0] * d[0] + u[1] * d[1] + u[2] * d[2];
  const long long g = d2 - uu;
  if (g <= 0) return 0;
  const S128 q = sub128(mul_i64(d2, uu), mul_i64(ud, ud));
  const S128 gg = mul_i64(g, g);
  if (cmp128(gg, mul_small(q, 2)) > 0) return 4;
  if (cmp128(mul_small(gg, 3), mul_small(q, 4)) > 0) return 3;
  return 2;
}

// SEUIL DE MORT, REDEFINI ICI EN `long long`. Le sujet le calcule en `int` ;
// c'est exactement la divergence que ce juge doit pouvoir observer.
inline long long threshold(long long smax, long long q) { return smax - q + 1; }

long long g_witness_evaluations = 0;

}  // namespace

long long last_witness_evaluations() { return g_witness_evaluations; }

LaneTruth judge(const std::vector<int>& xyz, int n, long long smax) {
  LaneTruth t{};
  g_witness_evaluations = 0;
  if (n < 2 || (long long)xyz.size() != 3LL * (long long)n) {
    t.refused = true;
    t.refusal = "cardinalite incoherente avec le tableau de coordonnees";
    return t;
  }
  // LE REFUS PLUTOT QUE LA CONVERSION. Un seuil non positif fermerait toutes
  // les paires sans temoin ; c'est le faux vert que le contre-audit a exhibe.
  const long long need2 = threshold(smax, 2);
  const long long need3 = threshold(smax, 3);
  const long long need4 = threshold(smax, 4);
  if (need4 < 1 || need2 > (long long)n) {
    t.refused = true;
    t.refusal = "seuils de lane hors du domaine utile pour ce nuage";
    return t;
  }

  const std::size_t nn = (std::size_t)n * (std::size_t)n;
  t.dead_q2.assign(nn, 0);
  t.dead_q3.assign(nn, 0);
  t.dead_q4.assign(nn, 0);

  std::vector<long long> p((std::size_t)n * 3);
  for (std::size_t i = 0; i < p.size(); ++i) p[i] = (long long)xyz[i];

  for (int a = 0; a < n; ++a) {
    const long long* pa = &p[(std::size_t)a * 3];
    for (int b = 0; b < n; ++b) {
      if (b == a) continue;
      const long long* pb = &p[(std::size_t)b * 3];
      long long c2 = 0, c3 = 0, c4 = 0;
      for (int z = 0; z < n; ++z) {
        if (z == a || z == b) continue;
        const int lane = lane_of(pa, pb, &p[(std::size_t)z * 3]);
        ++g_witness_evaluations;
        if (lane >= 2) ++c2;
        if (lane >= 3) ++c3;
        if (lane >= 4) ++c4;
      }
      const std::size_t k = (std::size_t)a * (std::size_t)n + (std::size_t)b;
      const bool d2 = c2 >= need2;
      const bool d3 = c3 >= need3;
      const bool d4 = c4 >= need4;
      if (d2) { t.dead_q2[k] = 1; ++t.closable_q2; }
      if (d3) { t.dead_q3[k] = 1; ++t.closable_q3; }
      if (d4) { t.dead_q4[k] = 1; ++t.closable_q4; }
      if (d2 && d3 && d4) ++t.closable_all;
    }
  }
  return t;
}

void count_witnesses(const std::vector<int>& xyz, int n, int a, int b, long long c[3]) {
  c[0] = c[1] = c[2] = 0;
  if (n < 3 || (long long)xyz.size() != 3LL * (long long)n) return;
  if (a == b || a < 0 || b < 0 || a >= n || b >= n) return;
  long long pa[3], pb[3], pz[3];
  for (int i = 0; i < 3; ++i) {
    pa[i] = (long long)xyz[(std::size_t)a * 3 + (std::size_t)i];
    pb[i] = (long long)xyz[(std::size_t)b * 3 + (std::size_t)i];
  }
  for (int z = 0; z < n; ++z) {
    if (z == a || z == b) continue;
    for (int i = 0; i < 3; ++i) pz[i] = (long long)xyz[(std::size_t)z * 3 + (std::size_t)i];
    const int lane = lane_of(pa, pb, pz);
    ++g_witness_evaluations;
    if (lane >= 2) ++c[0];
    if (lane >= 3) ++c[1];
    if (lane >= 4) ++c[2];
  }
}

// ---------------------------------------------------------------------------
// JUGER LE JUGE : la meme decision, recalculee avec `mhgp3v::BigInt`.
// ---------------------------------------------------------------------------
namespace {

int lane_of_bigint(const long long a[3], const long long b[3], const long long z[3]) {
  long long d[3], u[3];
  for (int i = 0; i < 3; ++i) {
    d[i] = b[i] - a[i];
    u[i] = 2 * z[i] - a[i] - b[i];
  }
  BigInt d2(0), uu(0), ud(0);
  for (int i = 0; i < 3; ++i) {
    d2 = d2 + BigInt(d[i]) * BigInt(d[i]);
    uu = uu + BigInt(u[i]) * BigInt(u[i]);
    ud = ud + BigInt(u[i]) * BigInt(d[i]);
  }
  const BigInt g = d2 - uu;
  if (!(g > BigInt(0))) return 0;
  const BigInt q = d2 * uu - ud * ud;
  const BigInt gg = g * g;
  if (gg > BigInt(2) * q) return 4;
  if (BigInt(3) * gg > BigInt(4) * q) return 3;
  return 2;
}

}  // namespace

long long selftest_against_bigint(long long draws, long long span, unsigned long long seed) {
  // LCG en arithmetique NON SIGNEE : un debordement signe serait indefini, et
  // UBSan le signalerait a juste titre.
  unsigned long long s = seed * 6364136223846793005ULL + 1442695040888963407ULL;
  auto next = [&s, span]() -> long long {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return (long long)((s >> 17) % (unsigned long long)(span + 1));
  };
  long long disagreements = 0;
  for (long long it = 0; it < draws; ++it) {
    long long a[3], b[3], z[3];
    for (int i = 0; i < 3; ++i) { a[i] = next(); b[i] = next(); z[i] = next(); }
    if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]) continue;
    if (lane_of(a, b, z) != lane_of_bigint(a, b, z)) ++disagreements;
  }
  return disagreements;
}

}  // namespace cone_oracle
}  // namespace mhgp3v
