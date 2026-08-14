// MorseHGP3D v3 — JUGE PRIMAL DU CERTIFICAT `JungDual` : DECLARATION.
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
#pragma once

namespace mhgp3v {
namespace jjudge {

// Verdict du juge. `kUnknown` n'est jamais un accord : il signale une
// degenerescence explicite, jamais une couverture par defaut.
enum class Verdict { kCouvre, kNeCouvrePas, kUnknown };

// Verdict PRIMAL de couverture d'un groupe de `k <= 3` temoins pour la lane
// `lane_q` (3 ou 4). Aucun poids n'est choisi : le minimum est calcule.
Verdict primal_couvre(const long long a[3], const long long b[3],
                      const long long z[][3], int k, int lane_q);

}  // namespace jjudge
}  // namespace mhgp3v
