// MorseHGP3D v3 — `BallFormToBallEvent-v0` : LA TRANCHE VERTICALE, ETAPE 0A.
//
// Ce que ce fichier fait, et pourquoi il vient AVANT toute parcimonie : la
// chaine v3 comptait des paires depuis des jours sans jamais produire un objet.
// Un certificat qui ferme `96 %` d'une masse ne se juge pas contre un autre
// certificat ; il se juge contre l'objet qu'il est cense preserver. Tant que
// `BallKey`, `I_B`, `U_B` et l'owner n'existent pas, aucune mesure amont n'est
// causale — c'est la directive de l'audit `1aa487d`, et son ordre `0A -> 0B`.
//
// TROIS IDENTITES, SEPAREES EXPRES.
//
//   `PrimitiveSphereKey` — l'identite de la SPHERE, AVANT tout census. Elle ne
//   contient ni shell, ni rang, ni rien de derive du nuage : seulement les
//   coefficients entiers reduits du polynome. C'est la condition pour pouvoir
//   dedupliquer AVANT de payer le census, et l'audit `92d0c0f` §6 insiste :
//   une cle qui contiendrait `shell_min` rendrait la deduplication circulaire.
//
//   `SupportKey` — l'identite du SUPPORT, les `PointId` TRIES. Jamais des
//   positions de tri. Plusieurs `SupportKey` distinctes peuvent partager une
//   meme `PrimitiveSphereKey` : c'est exactement la cosphere lourde.
//
//   `BallEvent` — ce qui sort apres le census : la sphere, ses supports, ses
//   membres interieurs stricts `I_B`, sa coquille `U_B`, son owner et sa
//   DISPOSITION.
//
// LE POLYNOME PLUTOT QUE LE CENTRE. Une sphere s'ecrit
// `A||z||^2 + B.z + C = 0` a coefficients ENTIERS, avec `A > 0` apres
// reduction par le pgcd. Le signe de `P(z) = A||z||^2 + B.z + C` decide :
// negatif interieur strict, nul sur la coquille, positif exterieur. Aucun
// centre rationnel n'est jamais forme, aucune division n'est faite, et la cle
// canonique tombe du meme calcul.
//
// LARGEURS, sur `coord < 2^10` et arite au plus quatre : `A < 2^81`,
// `B < 2^92`, `C < 2^102` et `P < 2^104`. Tout tient dans un `__int128`, et le
// domaine est REFUSE au-dela — un oracle borne refuse, il ne tronque pas.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace mhgp3v {

using i128 = __int128;

struct Pt3 { long long x[3]; };

// ---- La cle primitive de sphere : cinq entiers reduits, `A > 0`.
struct PrimitiveSphereKey {
  i128 a = 0, b[3] = {0, 0, 0}, c = 0;
  bool operator<(const PrimitiveSphereKey& o) const {
    if (a != o.a) return a < o.a;
    for (int i = 0; i < 3; ++i) if (b[i] != o.b[i]) return b[i] < o.b[i];
    return c < o.c;
  }
  bool operator==(const PrimitiveSphereKey& o) const {
    return a == o.a && b[0] == o.b[0] && b[1] == o.b[1] && b[2] == o.b[2] && c == o.c;
  }
};

inline i128 be_gcd(i128 x, i128 y) {
  if (x < 0) x = -x;
  if (y < 0) y = -y;
  while (y) { const i128 t = x % y; x = y; y = t; }
  return x;
}

// `c = N/den`, et `p` est un point DE la sphere. On developpe
// `||den z - N||^2 = ||den p - N||^2`, ce qui donne les coefficients entiers
// sans jamais diviser.
inline PrimitiveSphereKey be_key_from_center(const long long N[3], i128 den,
                                             const long long p[3]) {
  PrimitiveSphereKey k;
  k.a = (i128)den * den;
  i128 n2 = 0, r2 = 0;
  for (int i = 0; i < 3; ++i) {
    k.b[i] = -2 * den * (i128)N[i];
    n2 += (i128)N[i] * N[i];
    const i128 e = den * (i128)p[i] - N[i];
    r2 += e * e;
  }
  k.c = n2 - r2;
  // REDUCTION CANONIQUE : pgcd des cinq, puis `A > 0`.
  i128 g = k.a;
  for (int i = 0; i < 3; ++i) g = be_gcd(g, k.b[i]);
  g = be_gcd(g, k.c);
  if (g > 1) { k.a /= g; for (int i = 0; i < 3; ++i) k.b[i] /= g; k.c /= g; }
  if (k.a < 0) { k.a = -k.a; for (int i = 0; i < 3; ++i) k.b[i] = -k.b[i]; k.c = -k.c; }
  return k;
}

// `P(z)`, evalue en entiers. Negatif = interieur strict, nul = coquille.
inline i128 be_power(const PrimitiveSphereKey& k, const long long z[3]) {
  i128 p = k.c;
  for (int i = 0; i < 3; ++i) p += k.a * ((i128)z[i] * z[i]) + k.b[i] * (i128)z[i];
  return p;
}

// ---- La sphere d'un support, par arite. `ok=false` = support degenere
// (colineaire ou coplanaire), qui n'est pas un support propre.
//
// Le CENTRE est rendu comme `N/den`, jamais comme un rationnel reduit : c'est
// `be_key_from_center` qui canonicalise, une seule fois.
inline bool be_sphere2(const Pt3& a, const Pt3& b, long long N[3], i128* den) {
  for (int i = 0; i < 3; ++i) N[i] = a.x[i] + b.x[i];
  *den = 2;
  return !(a.x[0] == b.x[0] && a.x[1] == b.x[1] && a.x[2] == b.x[2]);
}

// Arite trois : le centre de la circumsphere MINIMALE, dans le plan du
// triangle. `G = D E - F^2 > 0` est exactement l'independance affine.
inline bool be_sphere3(const Pt3& a, const Pt3& b, const Pt3& x,
                       long long N[3], i128* den) {
  long long d[3], u[3];
  for (int i = 0; i < 3; ++i) { d[i] = b.x[i] - a.x[i]; u[i] = x.x[i] - a.x[i]; }
  long long D = 0, E = 0, F = 0;
  for (int i = 0; i < 3; ++i) { D += d[i] * d[i]; E += u[i] * u[i]; F += d[i] * u[i]; }
  const i128 G = (i128)D * E - (i128)F * F;
  if (G <= 0) return false;                       // colineaire
  // `W = E (D-F) d + D (E-F) u`, puis `c = a + W/(2G)`.
  for (int i = 0; i < 3; ++i) {
    const i128 w = (i128)E * (D - F) * d[i] + (i128)D * (E - F) * u[i];
    const i128 n = 2 * G * (i128)a.x[i] + w;
    N[i] = (long long)n;                          // borne verifiee par l'appelant
  }
  *den = 2 * G;
  return true;
}

// Arite quatre : Cramer sur `2 (p_i - p_0) . y = ||p_i - p_0||^2`.
inline bool be_sphere4(const Pt3& p0, const Pt3& p1, const Pt3& p2, const Pt3& p3,
                       long long N[3], i128* den) {
  long long M[3][3], r[3];
  const Pt3* q[3] = {&p1, &p2, &p3};
  for (int i = 0; i < 3; ++i) {
    r[i] = 0;
    for (int j = 0; j < 3; ++j) {
      M[i][j] = 2 * (q[i]->x[j] - p0.x[j]);
      const long long e = q[i]->x[j] - p0.x[j];
      r[i] += e * e;
    }
  }
  const i128 det =
      (i128)M[0][0] * ((i128)M[1][1] * M[2][2] - (i128)M[1][2] * M[2][1]) -
      (i128)M[0][1] * ((i128)M[1][0] * M[2][2] - (i128)M[1][2] * M[2][0]) +
      (i128)M[0][2] * ((i128)M[1][0] * M[2][1] - (i128)M[1][1] * M[2][0]);
  if (det == 0) return false;                     // coplanaire
  for (int col = 0; col < 3; ++col) {
    i128 mc[3][3];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) mc[i][j] = (j == col) ? (i128)r[i] : (i128)M[i][j];
    const i128 dc = mc[0][0] * (mc[1][1] * mc[2][2] - mc[1][2] * mc[2][1]) -
                    mc[0][1] * (mc[1][0] * mc[2][2] - mc[1][2] * mc[2][0]) +
                    mc[0][2] * (mc[1][0] * mc[2][1] - mc[1][1] * mc[2][0]);
    N[col] = (long long)(det * (i128)p0.x[col] + dc);
  }
  *den = det;
  return true;
}

// ---- POSITIVITE : le centre doit etre dans l'interieur RELATIF de `conv(S)`.
//
// C'est ce qui distingue un support PROPRE d'un simple tuple cospherique. Pour
// l'arite deux le centre est le milieu, donc toujours interieur. Pour trois et
// quatre on resout les coordonnees barycentriques et on exige qu'elles soient
// TOUTES strictement positives — une coordonnee nulle signifie que le centre
// est sur une face, donc que le vrai support est cette face.
inline bool be_positive(const std::vector<Pt3>& pts, const std::vector<int>& S,
                        const long long N[3], i128 den) {
  const int k = (int)S.size();
  if (k == 2) return true;
  const Pt3& p0 = pts[(size_t)S[0]];
  // `y = c - p0 = N/den - p0`, en numerateurs sur `den`.
  i128 y[3];
  for (int i = 0; i < 3; ++i) y[i] = (i128)N[i] - den * (i128)p0.x[i];
  if (k == 3) {
    long long d[3], u[3];
    for (int i = 0; i < 3; ++i) {
      d[i] = pts[(size_t)S[1]].x[i] - p0.x[i];
      u[i] = pts[(size_t)S[2]].x[i] - p0.x[i];
    }
    i128 D = 0, E = 0, F = 0, yd = 0, yu = 0;
    for (int i = 0; i < 3; ++i) {
      D += (i128)d[i] * d[i]; E += (i128)u[i] * u[i]; F += (i128)d[i] * u[i];
      yd += y[i] * d[i]; yu += y[i] * u[i];
    }
    const i128 g = D * E - F * F;
    if (g <= 0) return false;
    // `beta = (E yd - F yu)/g`, `gamma = (D yu - F yd)/g`, sur `den`.
    const i128 nb = E * yd - F * yu, ng = D * yu - F * yd;
    const i128 sg = (den > 0) ? 1 : -1;           // signe de la division par `den`
    const i128 b2 = nb * sg, g2 = ng * sg;
    const i128 da = (den > 0) ? den : -den;
    return b2 > 0 && g2 > 0 && (b2 + g2) < g * da;
  }
  // arite quatre : Cramer dans la base des trois aretes issues de `p0`
  i128 A[3][3];
  for (int j = 0; j < 3; ++j)
    for (int i = 0; i < 3; ++i) A[i][j] = (i128)(pts[(size_t)S[j + 1]].x[i] - p0.x[i]);
  const i128 dt = A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) -
                  A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
                  A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
  if (dt == 0) return false;
  i128 lam[3];
  for (int col = 0; col < 3; ++col) {
    i128 mc[3][3];
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) mc[i][j] = (j == col) ? y[i] : A[i][j];
    lam[col] = mc[0][0] * (mc[1][1] * mc[2][2] - mc[1][2] * mc[2][1]) -
               mc[0][1] * (mc[1][0] * mc[2][2] - mc[1][2] * mc[2][0]) +
               mc[0][2] * (mc[1][0] * mc[2][1] - mc[1][1] * mc[2][0]);
  }
  // `lambda_j = lam[j]/(dt den)`. Normaliser le signe du denominateur commun.
  const i128 dd = dt * den;
  const i128 s = (dd > 0) ? 1 : -1;
  const i128 ad = (dd > 0) ? dd : -dd;
  i128 sum = 0;
  for (int j = 0; j < 3; ++j) { if (lam[j] * s <= 0) return false; sum += lam[j] * s; }
  return sum < ad;
}

// ---- L'OWNER : l'arete maximale du support, tie-break par `EdgeKey` de
// `PointId` TRIES. Jamais des positions de tri — l'audit `1aa487d` a pris ce
// defaut en flagrant delit dans la vague q3.
struct EdgeKey { int lo = -1, hi = -1; };

inline EdgeKey be_owner(const std::vector<Pt3>& pts, const std::vector<int>& S) {
  EdgeKey best;
  long long bd = -1;
  for (size_t i = 0; i < S.size(); ++i)
    for (size_t j = i + 1; j < S.size(); ++j) {
      long long d = 0;
      for (int t = 0; t < 3; ++t) {
        const long long e = pts[(size_t)S[i]].x[t] - pts[(size_t)S[j]].x[t];
        d += e * e;
      }
      const int lo = std::min(S[i], S[j]), hi = std::max(S[i], S[j]);
      if (d > bd || (d == bd && (lo < best.lo || (lo == best.lo && hi < best.hi)))) {
        bd = d; best.lo = lo; best.hi = hi;
      }
    }
  return best;
}

// ---- La DISPOSITION, telle que l'audit `1aa487d` §4 la fixe.
//
// `regular`  : la coquille vaut exactement le support ;
// `unsupported` : la coquille porte un point EXTERIEUR au support, ce qui viole
//                 `RelevantGP`. Le contrat exact courant refuse ATOMIQUEMENT ;
//                 il ne choisit pas un support arbitraire, ne perturbe pas et
//                 ne tronque pas. La cosphere u16 a `384` points est donc une
//                 fixture de REFUS de domaine, pas une obligation de
//                 materialiser ses deux millions de `SupportKey` ;
// `plateau_pending` : reserve au quotient sature, pas encore prouve.
//
// Le record reste LOSSLESS pour que ce refus ne fige pas l'avenir.
enum class Disposition { kRegular, kPlateauPending, kUnsupported };

// ---- LE NIVEAU EXACT, COMPARE SANS JAMAIS ETRE FORME.
//
// L'audit `1aa487d` §3 exige que le fold ne consomme plus la representation
// arithmetique native mais une IDENTITE : comparaison exacte, niveau exact,
// serialisation versionnee. C'est la couture `ExactKernel / SphereIdentity`,
// et elle se prepare MAINTENANT pour que le passage a un profil `binary64` ne
// force pas a reecrire le fold.
//
// Le rayon carre d'une sphere `A||z||^2 + B.z + C = 0` vaut
// `R^2 = (||B||^2 - 4 A C) / (4 A^2)`. On ne le forme jamais : on compare deux
// niveaux par produit croise, `(||B1||^2-4A1C1) A2^2` contre
// `(||B2||^2-4A2C2) A1^2`. Rend `-1`, `0` ou `+1`, et `false` si le domaine
// deborde — un juge borne refuse, il ne tronque pas.
inline bool be_level_num(const PrimitiveSphereKey& k, i128* num, i128* den2) {
  i128 b2 = 0;
  for (int i = 0; i < 3; ++i) {
    const i128 t = k.b[i] * k.b[i];
    if (k.b[i] != 0 && t / k.b[i] != k.b[i]) return false;
    b2 += t;
  }
  const i128 ac = 4 * k.a * k.c;
  *num = b2 - ac;
  *den2 = k.a;
  return true;
}

inline bool be_level_cmp(const PrimitiveSphereKey& x, const PrimitiveSphereKey& y, int* out) {
  i128 nx = 0, ax = 0, ny = 0, ay = 0;
  if (!be_level_num(x, &nx, &ax) || !be_level_num(y, &ny, &ay)) return false;
  // `nx / (4 ax^2)` contre `ny / (4 ay^2)` : produit croise sur `ax^2 ay^2 > 0`.
  const i128 l = nx * ay * ay, r = ny * ax * ax;
  if (ay != 0 && (nx != 0) && (l / (ay * ay)) != nx) return false;   // debordement
  if (ax != 0 && (ny != 0) && (r / (ax * ax)) != ny) return false;
  *out = (l < r) ? -1 : (l > r ? 1 : 0);
  return true;
}

struct SphereRun {
  PrimitiveSphereKey key;
  std::vector<std::vector<int>> supports;   // `SupportKey`, `PointId` tries
  std::vector<EdgeKey> owners;              // un par support
  std::vector<int> interior;                // `I_B`, strictement interieur
  std::vector<int> shell;                   // `U_B`, sur la coquille
  Disposition disposition = Disposition::kRegular;
  int rank = 0;                             // `|I_B| + |U_B|`
};

}  // namespace mhgp3v
