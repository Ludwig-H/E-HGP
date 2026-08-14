// MorseHGP3D v3 — JUGE PRIMAL DU CERTIFICAT `JungDual`, UNITE SEPAREE.
//
// Specification : section 5.1 de
// audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md.
// Cadre : phase=exploration_v3_hors_registre, backend=oracle_borne,
//         profile=quantized_u16_input_only, mode=autorite_independante,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// POURQUOI CE FICHIER EXISTE
//
// `prototype/jung_dual.hpp` decide la couverture par la voie DUALE : il choisit
// des poids et teste `A^2 > 2R`. Le contre-audit reproche a juste titre que son
// selftest ne compare que `k=1` a `(g,Q)` et qu'aucun chemin ne construit de
// centres pour juger un groupe `k>=2`. Un mutant de poids y est detecte parce
// que deux appels au meme predicat different — ce n'est pas un oracle de
// couverture.
//
// Ce juge-ci prend la voie PRIMALE, et il ne partage rien avec le sujet : autre
// unite de traduction, autre arithmetique — `mhgp3v::BigInt`, signe et
// magnitude en chiffres de 32 bits — et surtout autre objet mathematique. Il ne
// choisit aucun poids : il minimise directement.
//
// ---------------------------------------------------------------------------
// L'OBJET PRIMAL
//
// Le centre d'une sphere par `a,b` s'ecrit `c = m + w` avec `m=(a+b)/2` et
// `w` orthogonal a `d=b-a`. En posant `s = 2w` et `u_z = a+b-2z` :
//
//   z est INTERIEUR  <=>  2 s.u_z < D - ||u_z||^2 = g_z
//   z est MAUVAIS    <=>  2 s.u_z >= g_z
//
// Le domaine admissible est le disque de Jung `||s||^2 <= D/2` pour q4 et
// `||s||^2 <= D/3` pour q3 — car `||w|| <= kappa ||h||` avec `kappa^2 = 1/2`
// puis `1/3`, et `||h||^2 = D/4`.
//
// Le groupe COUVRE la lane si et seulement si l'intersection de ses demi-plans
// mauvais ne rencontre pas le disque. Comme cette intersection est un convexe
// ferme, cela equivaut a :
//
//   min { ||s||^2 : s.d = 0, 2 s.u_z >= g_z pour tout z du groupe }  >  rayon^2
//
// et le minimum d'une forme quadratique sur une intersection de demi-plans est
// atteint a l'origine, sur la projection d'un bord, ou a l'intersection de deux
// bords. Pour trois IDs au plus, c'est un nombre CONSTANT de candidats exacts.
//
// ---------------------------------------------------------------------------
// LA FORME ENTIERE, ET SES LARGEURS
//
// Seule la composante de `u_z` orthogonale a `d` compte. On travaille donc avec
// les quantites reduites de l'audit :
//
//   g_i  = D - ||u_i||^2
//   K_ij = D (u_i.u_j) - (u_i.d)(u_j.d)
//
// Pour un seul bord actif, `||s||^2 = (D/4) g_i^2 / K_ii`, donc la couverture
// q4 s'ecrit `g_i^2 > 2 K_ii` — et l'on retrouve exactement le predicat
// ponctuel `(g,Q)`, ce qui est la premiere verification de coherence.
//
// Pour deux bords actifs, avec
//   Delta = K_ii K_jj - K_ij^2      et
//   N     = g_i^2 K_jj - 2 g_i g_j K_ij + g_j^2 K_ii,
// on a `||s||^2 = (D/4) N / Delta`, donc :
//
//   q4 : N > 2 Delta        q3 : 3 N > 4 Delta
//
// Sous u16, `K` atteint 70 bits, donc `Delta` et `N` environ 140. `i128` ne
// suffit pas : tout passe par `BigInt`.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "bigint.hpp"
#include "jung_dual_judge.hpp"

namespace {

using mhgp3v::BigInt;
using mhgp3v::jjudge::Verdict;

struct P3 { long long x, y, z; };

long long dot(const P3& a, const P3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

}  // namespace

namespace mhgp3v {
namespace jjudge {

// Rend le verdict PRIMAL de couverture d'un groupe de `k <= 3` temoins pour la
// lane `q` (3 ou 4). Aucun poids n'est choisi : le minimum est calcule.
Verdict primal_couvre(const long long a[3], const long long b[3],
                      const long long z[][3], int k, int lane_q) {
  if (k <= 0 || k > 3) return Verdict::kUnknown;
  if (lane_q != 3 && lane_q != 4) return Verdict::kUnknown;
  const P3 A{a[0], a[1], a[2]}, B{b[0], b[1], b[2]};
  const P3 d{B.x - A.x, B.y - A.y, B.z - A.z};
  const long long D = dot(d, d);
  if (D <= 0) return Verdict::kUnknown;

  P3 u[3];
  long long g[3], ud[3];
  for (int i = 0; i < k; ++i) {
    u[i] = P3{A.x + B.x - 2 * z[i][0], A.y + B.y - 2 * z[i][1], A.z + B.z - 2 * z[i][2]};
    g[i] = D - dot(u[i], u[i]);
    ud[i] = dot(u[i], d);
  }
  // `K_ij = D (u_i.u_j) - (u_i.d)(u_j.d)`. Sous u16 chaque terme atteint 70
  // bits : `BigInt` des la formation, jamais un `i128` implicite.
  BigInt K[3][3];
  for (int i = 0; i < k; ++i)
    for (int j = 0; j < k; ++j)
      K[i][j] = BigInt::from_i128((__int128)D * (__int128)dot(u[i], u[j])) -
                BigInt::from_i128((__int128)ud[i] * (__int128)ud[j]);

  // Coefficients du rayon : q4 exige `||s||^2 > D/2`, q3 `> D/3`. Avec
  // `||s||^2 = (D/4) N / Delta`, cela devient `cn * N > cd * Delta`.
  const long long cn = (lane_q == 4) ? 1 : 3;
  const long long cd = (lane_q == 4) ? 2 : 4;

  // ---- CANDIDAT ZERO. L'origine est admissible si elle satisfait tous les
  // demi-plans mauvais, c'est-a-dire si `g_i <= 0` partout. Le minimum vaut
  // alors zero et le groupe ne couvre pas : le centre `m` lui-meme n'a aucun
  // temoin du groupe a l'interieur.
  bool origine_admissible = true;
  for (int i = 0; i < k; ++i)
    if (g[i] > 0) { origine_admissible = false; break; }
  if (origine_admissible) return Verdict::kNeCouvrePas;

  // ---- CANDIDATS A UN BORD ACTIF. `s_i` realise `2 s.u_i = g_i` au plus pres
  // de l'origine. Il est admissible s'il satisfait les autres contraintes :
  // `2 s_i.u_j >= g_j`, ce qui s'ecrit `g_i K_ij >= g_j K_ii` apres reduction
  // (le facteur commun `D/(2 K_ii)` est positif).
  bool region_vide = true;
  BigInt meilleur_n, meilleur_d;   // le minimum courant, sous la forme N/Delta
  bool a_minimum = false;
  for (int i = 0; i < k; ++i) {
    if (g[i] <= 0) continue;       // ce bord ne contraint pas l'origine
    bool admissible = true;
    for (int j = 0; j < k && admissible; ++j) {
      if (j == i) continue;
      // `g_i K_ij >= g_j K_ii`
      const BigInt gauche = BigInt((long long)g[i]) * K[i][j];
      const BigInt droite = BigInt((long long)g[j]) * K[i][i];
      if (gauche < droite) admissible = false;
    }
    if (!admissible) continue;
    region_vide = false;
    const BigInt n = BigInt((long long)g[i]) * BigInt((long long)g[i]);
    const BigInt dd = K[i][i];
    if (!a_minimum || (n * meilleur_d) < (meilleur_n * dd)) {
      meilleur_n = n; meilleur_d = dd; a_minimum = true;
    }
  }

  // ---- CANDIDATS A DEUX BORDS ACTIFS.
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j) {
      const BigInt delta = K[i][i] * K[j][j] - K[i][j] * K[i][j];
      if (delta.sign() <= 0) continue;   // bords paralleles : pas de sommet
      // Multiplicateurs de Lagrange : le sommet n'est le minimum que si les
      // deux contraintes y sont ACTIVES avec des multiplicateurs positifs,
      // c'est-a-dire `g_i K_jj - g_j K_ij >= 0` et `g_j K_ii - g_i K_ij >= 0`.
      const BigInt li = BigInt((long long)g[i]) * K[j][j] - BigInt((long long)g[j]) * K[i][j];
      const BigInt lj = BigInt((long long)g[j]) * K[i][i] - BigInt((long long)g[i]) * K[i][j];
      if (li.sign() < 0 || lj.sign() < 0) continue;
      // Admissibilite vis-a-vis du troisieme bord, s'il existe.
      bool admissible = true;
      for (int l = 0; l < k && admissible; ++l) {
        if (l == i || l == j) continue;
        // `2 s.u_l >= g_l` avec `2 s = (D/Delta) (li u_i + lj u_j)` reduit :
        // `li K_il + lj K_jl >= g_l Delta`.
        const BigInt gauche = li * K[i][l] + lj * K[j][l];
        const BigInt droite = BigInt((long long)g[l]) * delta;
        if (gauche < droite) admissible = false;
      }
      if (!admissible) continue;
      region_vide = false;
      const BigInt gi((long long)g[i]), gj((long long)g[j]);
      const BigInt n = gi * gi * K[j][j] - BigInt(2) * gi * gj * K[i][j] + gj * gj * K[i][i];
      if (!a_minimum || (n * meilleur_d) < (meilleur_n * delta)) {
        meilleur_n = n; meilleur_d = delta; a_minimum = true;
      }
    }

  // ---- VERDICT. Une region mauvaise VIDE est couverte trivialement.
  if (region_vide && !a_minimum) return Verdict::kCouvre;
  if (!a_minimum) return Verdict::kUnknown;
  // `cn * N > cd * Delta`, avec `Delta > 0`.
  const BigInt gauche = BigInt(cn) * meilleur_n;
  const BigInt droite = BigInt(cd) * meilleur_d;
  return (gauche > droite) ? Verdict::kCouvre : Verdict::kNeCouvrePas;
}

}  // namespace jjudge
}  // namespace mhgp3v

