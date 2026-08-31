// MorseHGP3D v6 — TEST D'ANCRE PAR SECTEURS (temoins universels sectoriels).
//
// Objet : pour une ancre (a,b), D² = |b−a|², m = (a+b)/2, toute boule-candidate
// de la lane a son centre c dans le plan bissecteur de ab, et v = c − m verifie
//   q3 : R² = |v|² + D²/4 avec R = D/(2 sin γ), γ = angle en x ∈ [60°, 90°)
//        (ab arete la plus longue d'un triangle aigu) ⟹ |v|² <= D²/12 ;
//   q4 : R² <= 3D²/8 (Jung, cœur de seed) ⟹ |v|² <= D²/8.
// Un site z est STRICTEMENT interieur a la boule de centre c ssi, avec w = z − m,
//   |w − v|² < |v|² + D²/4  ⟺  2 w·v > |w|² − D²/4
// (w·v = w_∥·v puisque v ⟂ b−a) : pour z fixe, l'ensemble des centres qui le
// contiennent est un DEMI-PLAN du plan bissecteur. Le disque des centres est
// recouvert par K triangles (0, p_k, p_{k+1}) d'un polygone CONVEXE a sommets
// entiers du plan bissecteur qui le contient ; le minimum d'une forme lineaire
// sur un triangle est atteint en un sommet, donc z est « universel sur le
// secteur k » ssi
//   4|w|² < D²  (sommet 0)  et  8 w·p_k > 4|w|² − D²  et  8 w·p_{k+1} > 4|w|² − D²
// (tout en entiers avec w2 = 2w = 2z − (a+b) : |w2|² < D², 4 w2·p > |w2|² − D²).
// THEOREME (suffisance) : si chaque secteur compte >= h sites universels, alors
// toute boule-candidate de l'ancre contient >= h sites strictement interieurs,
// donc aucun seed n'est emis : l'ancre est MORTE et l'objet (candidats emis)
// est inchange. Le test n'est jamais necessaire. Il generalise le test W_q
// (K = 1 : temoins universels sur tout le disque) et le rend efficace sur les
// ancres denses ou chaque boule est profonde sans temoin commun. Preuve de
// contenance du polygone : parallelogramme (±u, ±v), u = A·(d×e_i), v = B·(d×e_j)
// pour les deux plus grands produits d×axe, contenant le disque de rayon ρ ssi
// |u×v|² >= ρ²|u−v|² et |u×v|² >= ρ²|u+v|² (distances de 0 aux aretes) ;
// l'octogone (±u, ±v, ±(u+v), ±(u−v)) contient le parallelogramme (K = 8).
// Mutant `sector-kill-nonstrict` : >= au lieu de > (faux positifs) — tue par
// les conformites v4 et par la porte de fixture.
// DEUX TESTS CUMULES (aucun ne contient l'autre — contre-exemple grave dans
// tests/anchor_kill_fixture.cpp) : (1) W_q EXACT : temoins universels sur tout
// le disque ferme (in_spindle : q < 0 et 3q² > 4|d×u|² en q3) — absent de la
// lane q3 jusqu'ici (la lane q4 le faisait) ; (2) secteurs sur les ancres
// survivantes. Les deux ne regardent que les sites de la boule diametrale
// ouverte (|2w|² < D²) : le cover etant trie en 32 classes radiales de
// |2w|²/(3D²+1), la classe 11 et les suivantes n'en contiennent aucun —
// sortie de boucle des la classe 11 (economie x5-9 sur les covers denses).
// Mutant `anchor-kill-h-minus-one` : les deux tests tuent a h−1 (faux
// positifs) — tue par la fixture 8+8 (deux candidats disparaissent).
#pragma once

#include <algorithm>
#include <vector>

#include "../core/mutants.hpp"
#include "../core/types.hpp"
#include "../spindle/spindle.hpp"
#include "edge_cover.hpp"

namespace mhgp6 {

namespace sector_detail {
inline i128 sq(i64 x) { return (i128)x * x; }
// Classe radiale de anchor_cover_from_handles : 32·d2/(3D²+1) ; les sites de
// la boule diametrale ouverte (d2 < D²) ont une classe <= 10.
inline bool beyond_diametral_bins(i64 dist2q, i64 D2) { return (i128)32 * dist2q / ((i128)3 * D2 + 1) >= 11; }
}  // namespace sector_detail

// BASE ENTIERE DU PLAN BISSECTEUR (partagee par les secteurs et par la grille
// de cellules, cell_grid.hpp) : u = A·(d×e_i), v = B·(d×e_j) pour les deux
// plus grands produits d×axe, A et B croissant jusqu'a ce que le losange
// (±u, ±v) contienne le disque des centres de rayon² D²/rho2_den (distances
// de 0 aux aretes : |u×v|² >= ρ²|u−v|² et |u×v|² >= ρ²|u+v|²). Rend false si
// le plan est degenere (jamais : D2 > 0) ou si la garde fail-open est atteinte.
inline bool bisector_basis(const P3& pa, const P3& pb, i64 D2, i128 rho2_den, i64 u[3], i64 v[3]) {
  using sector_detail::sq;
  const i64 dx = pb.x - pa.x, dy = pb.y - pa.y, dz = pb.z - pa.z;
  const i64 cands[3][3] = {{0, dz, -dy}, {-dz, 0, dx}, {dy, -dx, 0}};
  i128 nn[3];
  for (int k = 0; k < 3; ++k) nn[k] = sq(cands[k][0]) + sq(cands[k][1]) + sq(cands[k][2]);
  int b1 = 0;
  for (int k = 1; k < 3; ++k) if (nn[k] > nn[b1]) b1 = k;
  int b2 = (b1 + 1) % 3;
  for (int k = 0; k < 3; ++k) if (k != b1 && nn[k] > nn[b2]) b2 = k;
  if (nn[b2] == 0) return false;  // d nul ou degenere : pas de plan (jamais : D2 > 0 et deux produits non nuls)
  const i64* e1 = cands[b1];
  const i64* e2 = cands[b2];
  i64 A = 1, B = 1;
  for (int iter = 0; iter < 128; ++iter) {
    for (int i = 0; i < 3; ++i) { u[i] = A * e1[i]; v[i] = B * e2[i]; }
    const i64 cx = u[1] * v[2] - u[2] * v[1], cy = u[2] * v[0] - u[0] * v[2], cz = u[0] * v[1] - u[1] * v[0];
    const i128 cross2 = sq(cx) + sq(cy) + sq(cz);
    const i128 dm = sq(u[0] - v[0]) + sq(u[1] - v[1]) + sq(u[2] - v[2]);
    const i128 dp = sq(u[0] + v[0]) + sq(u[1] + v[1]) + sq(u[2] + v[2]);
    if (cross2 * rho2_den >= (i128)D2 * dm && cross2 * rho2_den >= (i128)D2 * dp) return true;
    if (A <= B) ++A; else ++B;
  }
  return false;  // garde fail-open (jamais atteinte : |e| >= D·sqrt(2/3))
}

// CREDIT D EXTREMITE deja acquis pour cette ancre (cascade du 30 aout, audit
// 7d173a37) : `base = h_a(a) + h_b(b)`, les temoins de A et de B certifies
// universels sur la boite opposee. Leur domaine est DISJOINT des temoins pris
// hors de A union B, donc les deux comptes s additionnent :
//     depth(a,b,x) >= |W_q inter (cover \ (A u B))| + h_a(a) + h_b(b).
// Le test devient l UNION des deux formes,
//     max( |W_q inter (cover \ {a,b})| , |W_q inter (cover \ (A u B))| + base ) >= h,
// qui DOMINE les deux prises separement. Mesure du 30 aout : la forme
// residuelle seule est strictement plus faible (elle ne tue jamais une ancre
// que la forme complete rate, et en rate 2 a 239 selon la configuration),
// parce que h_a(a) <= |W_q inter (A \ a)| ; l union recupere donc la sortie
// anticipee sans rien perdre.
struct EndpointCredit {
  u64 base = 0;
  i32 a_first = 0, a_last = -1;
  i32 b_first = 0, b_last = -1;
  bool active() const { return base > 0 && a_last >= a_first; }
  bool in_boxes(i32 u) const { return (u >= a_first && u <= a_last) || (u >= b_first && u <= b_last); }
};

// Test W_q EXACT sur le cover trie : rend true si >= h sites sont universels
// (in_spindle) ; sortie anticipee a h et a la classe radiale 11.
inline bool anchor_universal_kill(const std::vector<CoverPoint>& cover, const std::vector<P3>& upos, i32 ua, i32 ub,
                                  const P3& pa, const P3& pb, i64 D2, Lane lane, u64 h, bool radially_sorted = true,
                                  const EndpointCredit* ec = nullptr) {
  const u64 hh = MHGP6_MUTANT("anchor-kill-h-minus-one") && h > 1 ? h - 1 : h;
  const bool use_ec = ec != nullptr && ec->active() && ec->base < hh;
  u64 n = 0, n_out = 0;
  for (const CoverPoint& cz : cover) {
    if (sector_detail::beyond_diametral_bins(cz.dist2q, D2)) { if (radially_sorted) break; else continue; }
    if (cz.u == ua || cz.u == ub) continue;
    if (!in_spindle(lane, pa, pb, upos[(size_t)cz.u])) continue;
    if (++n >= hh) return true;
    // UNION avec la forme residuelle : les temoins pris HORS de A union B ont
    // un domaine disjoint de celui de h_a et h_b, donc leur compte s y ajoute.
    // Cette branche ne peut que tuer PLUS TOT ; elle ne retire jamais une mort.
    if (use_ec && !ec->in_boxes(cz.u) && ++n_out + ec->base >= hh) return true;
  }
  return false;
}

// Rend true si l'ancre est morte par secteurs (K = 8). `rho2_den` : 12 (q3) ou
// 8 (q4) — rho² = D²/den. Compte dans `*witness_min` le minimum sur les
// secteurs du nombre de temoins (mesure).
//
// CREDIT D'EXTREMITE PAR SECTEUR (30 aout). Les temoins comptes par h_a(a) et
// h_b(b) sont universels sur la boite opposée : ils appartiennent a W_q(a,b)
// pour l'ancre courante, donc ils sont temoins QUEL QUE SOIT le porteur, donc
// dans CHAQUE secteur. Ils vivent dans A union B ; les temoins de secteur pris
// HORS boites leur sont donc disjoints, et le verdict devient
//     min_k max( cnt[k] , cnt_hors[k] + base ) >= h,
// qui domine les deux formes prises separement. `*witness_min` et
// `sector_counts` gardent leur semantique de MESURE (comptes purs, sans
// credit) : seule la decision change.
inline bool anchor_sector_kill(const std::vector<CoverPoint>& cover, const std::vector<P3>& upos, i32 ua, i32 ub, const P3& pa,
                               const P3& pb, i64 D2, i128 rho2_den, u64 h, u64* witness_min, u32* sector_counts = nullptr,
                               bool radially_sorted = true, const EndpointCredit* ec = nullptr) {
  using sector_detail::sq;
  const bool nonstrict = MHGP6_MUTANT("sector-kill-nonstrict");
  const u64 hh = MHGP6_MUTANT("anchor-kill-h-minus-one") && h > 1 ? h - 1 : h;
  i64 u[3], v[3];
  if (!bisector_basis(pa, pb, D2, rho2_den, u, v)) return false;
  // Sommets de l'octogone dans l'ordre angulaire : u, u+v, v, −u+v, −u, −u−v, −v, u−v.
  i64 P[8][3];
  for (int i = 0; i < 3; ++i) {
    P[0][i] = u[i]; P[1][i] = u[i] + v[i]; P[2][i] = v[i]; P[3][i] = -u[i] + v[i];
    P[4][i] = -u[i]; P[5][i] = -u[i] - v[i]; P[6][i] = -v[i]; P[7][i] = u[i] - v[i];
  }
  u32 cnt[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  u32 cnt_out[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  const bool use_ec = ec != nullptr && ec->active() && ec->base < hh;
  const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
  for (const CoverPoint& cz : cover) {
    if (sector_detail::beyond_diametral_bins(cz.dist2q, D2)) { if (radially_sorted) break; else continue; }  // cover trie par classes radiales
    if (cz.u == ua || cz.u == ub) continue;
    const P3& z = upos[(size_t)cz.u];
    const i64 w0 = 2 * z.x - sx, w1 = 2 * z.y - sy, w2 = 2 * z.z - sz;  // w2 = 2w
    const i128 n2w = sq(w0) + sq(w1) + sq(w2);
    if (nonstrict ? (n2w > (i128)D2) : (n2w >= (i128)D2)) continue;  // sommet 0 : |2w|² < D²
    const i128 rhs = n2w - (i128)D2;
    bool ok[8];
    for (int k = 0; k < 8; ++k) {
      const i128 dot = (i128)w0 * P[k][0] + (i128)w1 * P[k][1] + (i128)w2 * P[k][2];
      ok[k] = nonstrict ? (4 * dot >= rhs) : (4 * dot > rhs);
    }
    // Un temoin DANS A union B est deja compte par base : ne pas le recrediter.
    const bool hors = use_ec && !(MHGP6_MUTANT("sector-credit-inbox") ? false : ec->in_boxes(cz.u));
    for (int k = 0; k < 8; ++k)
      if (ok[k] && ok[(k + 1) & 7]) { ++cnt[k]; if (hors) ++cnt_out[k]; }
  }
  u32 mn = cnt[0];
  for (int k = 1; k < 8; ++k) mn = std::min(mn, cnt[k]);
  if (sector_counts) for (int k = 0; k < 8; ++k) sector_counts[k] = cnt[k];
  *witness_min = mn;  // MESURE : compte pur, sans credit
  if ((u64)mn >= hh) return true;
  if (!use_ec) return false;
  // Le maximum est pris SECTEUR PAR SECTEUR, AVANT le minimum. La forme
  // globale max( min_k cnt[k], min_k (cnt_hors[k] + base) ) est strictement
  // plus faible : elle rate les ancres dont certains secteurs sont fermes par
  // le compte pur et les autres par le credit. Mutant `sector-credit-global`.
  const bool global_form = MHGP6_MUTANT("sector-credit-global");
  u64 mn_u = std::max((u64)cnt[0], (u64)cnt_out[0] + ec->base);
  u64 mn_g = (u64)cnt_out[0] + ec->base;
  for (int k = 1; k < 8; ++k) {
    mn_u = std::min(mn_u, std::max((u64)cnt[k], (u64)cnt_out[k] + ec->base));
    mn_g = std::min(mn_g, (u64)cnt_out[k] + ec->base);
  }
  return (global_form ? mn_g : mn_u) >= hh;
}

// Les deux tests cumules : W_q exact d'abord (bon marche, sortie a h), secteurs
// sur les survivantes. `rho2_den` : 12 (q3) / 8 (q4). Rend 1 (W_q), 2
// (secteurs) ou 0 (vivante).
inline int anchor_kill_cumulated(const std::vector<CoverPoint>& cover, const std::vector<P3>& upos, i32 ua, i32 ub,
                                 const P3& pa, const P3& pb, i64 D2, Lane lane, i128 rho2_den, u64 h,
                                 bool radially_sorted = true, const EndpointCredit* ec = nullptr) {
  if (anchor_universal_kill(cover, upos, ua, ub, pa, pb, D2, lane, h, radially_sorted, ec)) return 1;
  u64 wmin = 0;
  if (anchor_sector_kill(cover, upos, ua, ub, pa, pb, D2, rho2_den, h, &wmin, nullptr, radially_sorted, ec)) return 2;
  return 0;
}

// PRETESTS AVANT LE COVER : les deux tests ne lisent que les sites de la boule
// diametrale ouverte ; sur un rectangle dense (beaucoup de points de handles),
// une requete d'arbre de coefficient 1 par ancre (cover_query : O(log n +
// sortie)) coute moins que le balayage des points de handles, et l'ancre morte
// n'a jamais son cover complet construit. Meme verdict quel que soit le chemin
// (sur-ensemble exact des temoins) : l'objet ne depend pas de la politique.
inline int anchor_kill_from_query(const CloudIndex& ix, i32 ua, i32 ub, const P3& pa, const P3& pb, i64 D2, Lane lane,
                                  i128 rho2_den, u64 h, std::vector<CoverPoint>* tmp) {
  cover_query(ix, pa, pb, D2, 1, tmp, /*sorted=*/false);  // ordre indifferent pour les verdicts ; le tri dominait le cout
  return anchor_kill_cumulated(*tmp, ix.upos, ua, ub, pa, pb, D2, lane, rho2_den, h, /*radially_sorted=*/false);
}
// Variante par RECTANGLE : les candidats diametraux du rectangle
// (rect_diametral_candidates, une traversee par rectangle) contiennent la boule
// diametrale de chaque ancre ; le CoverPoint::dist2q qu'ils portent est la
// distance a la boite des sommes, pas |2z − (a+b)|² : on la recalcule ici
// (les tests ne lisent dist2q que pour la sortie anticipee, desactivee).
inline int anchor_kill_from_candidates(const std::vector<CoverPoint>& cand, const std::vector<P3>& upos, i32 ua, i32 ub,
                                       const P3& pa, const P3& pb, i64 D2, Lane lane, i128 rho2_den, u64 h,
                                       const EndpointCredit* ec = nullptr) {
  return anchor_kill_cumulated(cand, upos, ua, ub, pa, pb, D2, lane, rho2_den, h, /*radially_sorted=*/false, ec);
}

}  // namespace mhgp6
