// MorseHGP3D v4 — FLUX DE BOULES : les trois lanes WSPD comme generateurs,
// puis sort/RLE par BallKey et UN census par cle (l'ABI SpherePlateau de
// l'audit bloquant, version echelle).
//
// COMPLETUDE SOUS LES SEUILS h_q (derive_v4) : un plateau pertinent pour la
// foret (∃ σ = I_B ∪ T, |σ| <= K_max+1 = 11) dont le support minimal est
// d'arite q verifie |T| >= q donc |I_B| <= 11 - 1 - (q - 1) - ... plus
// simplement : |I_B| <= K_max + 1 - q ; ses temoins de fuseau
// W_q(a,b) ⊆ I_B sont donc au plus K_max + 1 - q - 1 < h_q = s_max - q + 1
// (s_max = K_max + 1). L'ancre du support minimal SURVIT toujours aux
// filtres h_coeur/h_a/h_b : les seuils du profil sont exactement calibres
// pour ne perdre aucun plateau pertinent. (Carathéodory garantit un support
// minimal d'arite 2, 3 ou 4 dans U_B — audit § 2.)
//
// CENSUS PAR CLE : la forme primitive (A, B, C) donne le predicat UNIFORME
// P(z) = A·|z|² + B·z + C (< 0 interieur strict, = 0 coquille), lanes
// confondues. Largeurs u16 : A < 2^68, |B_i| < 2^86, |C| < 2^104 ;
// A|z|² < 2^102, |B·z| < 2^105 → i128. Descente d'arbre separable par axe
// (parabole convexe, minimum de reseau aux entiers voisins du sommet).
//
// NIVEAU CANONIQUE PAR BOULE : au RLE, le representant de niveau retenu est
// celui du generateur d'ARITE MINIMALE (puis plus petite representation) —
// q2/q3 donnent des fractions canoniques, q4 un representant (|N'|², det²) ;
// sujet et juge appliquent la meme regle, les representants coincident.
#pragma once

#include <algorithm>
#include <chrono>
#include <vector>

#include "../events/acute_seed.hpp"
#include "../events/edge_cover.hpp"
#include "../events/q2_event.hpp"
#include "../events/q4_event.hpp"
#include "../events/witness_count.hpp"
#include "../wspd/wavefront.hpp"

namespace mhgp4 {

struct BallCandidate {
  Q3BallKey key;
  Q4Level level;
  u8 arity;  // arite du generateur (2, 3, 4)
};

inline bool ball_candidate_less(const BallCandidate& x, const BallCandidate& y) {
  if (!(x.key == y.key)) return x.key < y.key;
  if (x.arity != y.arity) return x.arity < y.arity;
  return x.level < y.level;  // representation : depart deterministe
}

// Statistiques du flux (compteurs, jamais une autorite).
struct BallStreamStats {
  u64 rect_alive[3] = {0, 0, 0};
  u64 anchors[3] = {0, 0, 0};
  u64 candidates[3] = {0, 0, 0};
  u64 unique_balls = 0;
  u64 census_interior = 0;
  u64 census_shell = 0;
  u64 balls_dead_depth = 0;  // |I_B| >= h_qmin : aucun K <= K_max
  // Passe count-only (audit « prefiltre exact par boule ») : ses couts,
  // separes de ceux du census complet — un gain de census ne doit jamais
  // dissimuler un tri ou une passe qui mange le gain.
  u64 prefilter_nodes = 0;
  u64 prefilter_leaf_tests = 0;
  u64 prefilter_range_add_mass = 0;
  u64 full_census_keys = 0;
  // Filtre de profondeur A LA GENERATION (par lane) : candidats tues par
  // le minorant certifie sur le cover, AVANT toute emission.
  u64 gen_depth_killed[3] = {0, 0, 0};
  // Ancres q4 tuees par le compte W_4 exact (reponse auditeur « minorant
  // q4 et axial streaming » § 1) : W_4 est le plus grand cœur anchor-only
  // (owner + Jung) — n4 >= h_4 rend l'ancre inutile AVANT seed × completion.
  u64 anchors_killed_w4 = 0;
  // Selection axiale bornee (compteurs exiges par l'audit « axial borne ») :
  // seeds traites, sites balayes, seeds morts par permanents (p >= h_4),
  // groupes de mu emis.
  u64 axial_seeds = 0;
  u64 axial_sites = 0;
  u64 axial_seeds_dead_perm = 0;
  u64 axial_groups_emitted = 0;
  // Sweep a DEUX COTES (contre-audit 63d364a) : morts bilaterales — groupes
  // tues par la profondeur exacte d_j AVANT valid_completion/q4_form, ET
  // racines exclues par le seuil du cote OPPOSE (positive sous L : >= k
  // negatifs strictement au-dessus ; negative sur U : >= k positifs
  // strictement en dessous — d >= h_4 exact dans les deux cas). Les
  // exclusions par leur PROPRE cote (positive sur U, negative sous L) ne
  // comptent pas : elles existaient deja dans le sweep unilateral.
  u64 axial_groups_killed_two_sided = 0;
  u64 axial_verify_mismatch = 0;
  // Cœur universel du seed (audit « axial arbre et cœur de seed » § 1) :
  // seeds tues par >= h_4 temoins seed-universels (ou J < 0 : aucun
  // tetraedre admissible), et nœuds visites par la descente du compte.
  u64 seeds_killed_seed_core = 0;
  u64 seed_core_nodes = 0;
  // Decomposition de t_gen exigee par l'audit § 3 (chemin axial) :
  // cœur de seed, materialisation A,B des sites, reduction a deux cotes
  // (seuils + groupes + prefixes + d_j), emission (valid_completion +
  // q4_form + verify). En millisecondes ; le cœur est aussi chronometre
  // sur le chemin baseline.
  double t_seed_core_ms = 0, t_axial_ab_ms = 0, t_reduce_ms = 0,
         t_emit_ms = 0;
};

// Drapeaux du chemin axial (mutants + verification de reception).
enum : u32 {
  kAxialShortGroup = 1u,     // MUTANT : k-1 (prefixe trop court)
  kAxialDropTies = 2u,       // MUTANT : ties de frontiere supprimes
  kAxialFirstRep = 4u,       // MUTANT : premier representant valide
  kAxialIgnoreOpposite = 8u, // MUTANT : d_j sans le cote oppose
  kAxialDepthNonstrict = 16u,// MUTANT : le groupe compte sa propre coquille
  kAxialReverseNeg = 32u,    // MUTANT : suffixe negatif remplace par prefixe
  kAxialVerify = 64u,        // reception : d_j recoupe par le scan complet
  kAxialSeedCoreNonstrict = 128u,  // MUTANT : egalites comptees au cœur
};

// Site axial d'un seed : A = puissance q3, B = normale·(z−a), mu = A/B.
struct AxialSite {
  i128 A;
  i64 B;
  i32 u;
};

// Comparaison exacte 2·P² <=> J·B² (cœur de seed). Sous u16 les produits
// atteignent ~212 bits (audit § 1) : U320, jamais i128. Preconditions :
// P <= 0, J >= 0 ; produits < 2^260 (contrat de mul_192_128_to_320).
inline int cmp_2p2_jb2(i128 P, i128 J, i64 B) {
  const u128 ap = (u128)(-P);
  const u64 pw[3] = {(u64)ap, (u64)(ap >> 64), 0};
  U320 lhs = mul_192_128_to_320(pw, ap);  // P² < 2^209
  u64 carry = 0;                          // ×2 : decalage d'un bit, sans
  for (int i = 0; i < 5; ++i) {           // debordement (2P² < 2^210)
    const u64 nc = lhs.w[i] >> 63;
    lhs.w[i] = (lhs.w[i] << 1) | carry;
    carry = nc;
  }
  const u128 ju = (u128)J;
  const u64 jw[3] = {(u64)ju, (u64)(ju >> 64), 0};
  const u128 b2 = (u128)((i128)B * B);
  return cmp_u320(lhs, mul_192_128_to_320(jw, b2));  // J·B² < 2^210
}

// ---- Cœur universel du seed (audit « axial arbre et cœur de seed » § 1).
// Jung : tout tetraedre ACCEPTE par la production (six aretes <= D,
// centre strictement interieur => circumboule = miniboule) verifie
// R² <= 3D/8, donc sa racine axiale verifie 2·mu² <= J = D·(3G − 2EX)
// (avec R_mu² = DEX/(4G) + mu²/(4G) et |n|² = G). Un site z tel que
// P(z) < 0 et 2·P(z)² > J·B(z)² est STRICTEMENT interieur a TOUTE sphere
// admissible du seed : Phi_mu(z) = P − mu·B <= P + racine(J/2)·|B| < 0.
// L'egalite 2P² = J·B² n'est PAS comptee (le site peut etre SUR la
// sphere extremale — mutant seed-core-nonstrict, qui compte aussi les
// points du faisceau P = 0, B = 0). J < 0 : aucun tetraedre admissible
// (R3² > 3D/8 deja sur le cercle du seed) — le seed meurt sans temoin.
// Compte sur l'antichaine de l'ancre, descente ALL/NONE par boite
// (ALL : Pmax < 0 et 2·Pmax² > J·Babs² ; NONE : Pmin >= 0), arret a h.
// FAIL-OPEN : ne tue que sur des interieurs stricts certifies du nuage ;
// les temoins hors de l'antichaine sont simplement omis. EXACT pour la
// sortie post-RLE : une emission q4 supprimee appartient a un groupe qui
// meurt de toute facon (>= h_4 interieurs si le label q_min est 4 ; si
// une emission d'arite inferieure partage la cle, le groupe subsiste par
// elle, inchange) — meme argument que depth_dead et le filtre W_4.
inline bool seed_core_kills(const CloudIndex& ix,
                            const std::vector<NodeRef>& handles,
                            const Q3Form& f3s, const P3& nrm, i128 J, u64 h,
                            i32 ua, i32 ub, i32 ux, bool nonstrict,
                            BallStreamStats* st) {
  if (J < 0) return true;
  const i64 nn[3] = {nrm.x, nrm.y, nrm.z};
  const i64 aa[3] = {f3s.a.x, f3s.a.y, f3s.a.z};
  u64 count = 0;
  std::vector<NodeRef> stack(handles.begin(), handles.end());
  while (!stack.empty() && count < h) {
    const NodeRef z = stack.back();
    stack.pop_back();
    ++st->seed_core_nodes;
    if (z < 0) {
      const i32 u = -1 - z;
      if (u == ua || u == ub || u == ux) continue;
      const P3& pz = ix.upos[(size_t)u];
      const i128 P = q3_power(f3s, pz);
      if (nonstrict ? (P > 0) : (P >= 0)) continue;
      const i64 B = p3_dot(nrm, p3_sub(pz, f3s.a));
      const int c = cmp_2p2_jb2(P, J, B);
      if (nonstrict ? (c >= 0) : (c > 0)) count += ix.range_weight(u, u);
      continue;
    }
    const AxisBox bz = box_of_node(ix, z);
    i128 pmn = 0, pmx = 0;
    i64 bmn = 0, bmx = 0;
    for (int i = 0; i < 3; ++i) {
      const i64 lo = bz.lo[i] - aa[i];
      const i64 hi = bz.hi[i] - aa[i];
      pmn += detail_q3::axis_min(f3s, i, lo, hi);
      pmx += detail_q3::axis_max(f3s, i, lo, hi);
      bmn += nn[i] >= 0 ? nn[i] * lo : nn[i] * hi;
      bmx += nn[i] >= 0 ? nn[i] * hi : nn[i] * lo;
    }
    if (nonstrict ? (pmn > 0) : (pmn >= 0)) continue;  // NONE
    if (nonstrict ? (pmx <= 0) : (pmx < 0)) {
      // ALL possible : |P| >= |Pmax| et |B| <= Babs sur tout le nœud.
      const i64 babs = bmx > -bmn ? bmx : -bmn;
      const int c = cmp_2p2_jb2(pmx, J, babs);
      if (nonstrict ? (c >= 0) : (c > 0)) {
        // a, b, x ont P = 0 : un nœud ALL strict ne peut pas les contenir
        // (sous mutant l'excedent tue PLUS de seeds, jamais moins).
        const NodeRange r = range_of(ix, z);
        count += ix.range_weight(r.first, r.last);
        continue;
      }
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return count >= h;
}

namespace detail_bs {

// Vague WSPD ternaire d'une lane : rectangles vivants (h_coeur < h).
struct AliveRect {
  WspdRect r;
  u64 core;
};

inline void wspd_alive(const CloudIndex& ix, i64 s, const u64 h_of[3], int lane_idx,
                       u8 mask, u64 h, std::vector<AliveRect>* out) {
  out->clear();
  if (ix.nodes.empty()) return;
  std::vector<WspdRect> wave, next;
  for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
  while (!wave.empty()) {
    next.clear();
    for (const WspdRect& r : wave) {
      const FusedCounts fc = count_universal_witnesses_234(ix, r.a, r.b, h_of, mask, false);
      if (fc.c[lane_idx] >= h) continue;
      i64 ba[3], bb[3];
      const auto va = detail::node_view(ix, r.a, ba);
      const auto vb = detail::node_view(ix, r.b, bb);
      if (detail::separated(va, vb, s, 1)) {
        const FusedCounts ff = count_universal_witnesses_234(ix, r.a, r.b, h_of, mask, true);
        if (ff.c[lane_idx] < h) out->push_back(AliveRect{r, ff.c[lane_idx]});
        continue;
      }
      const i64 w2a = detail::box_w2(va);
      const i64 w2b = detail::box_w2(vb);
      const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
      const NodeRef keep = split_a ? r.b : r.a;
      const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
      next.push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
      next.push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
    }
    wave.swap(next);
  }
}

// Histogrammes h_a/h_b a 8 coins d'un rectangle, pour une lane.
inline void corner_histograms(const CloudIndex& ix, Lane lane, const AliveRect& ar,
                              std::vector<u64>* ha, std::vector<u64>* hb) {
  const NodeRange ra = range_of(ix, ar.r.a);
  const NodeRange rb = range_of(ix, ar.r.b);
  const AxisBox boxA = box_of_node(ix, ar.r.a);
  const AxisBox boxB = box_of_node(ix, ar.r.b);
  const int na = ra.last - ra.first + 1;
  const int nb = rb.last - rb.first + 1;
  ha->assign((size_t)na, 0);
  hb->assign((size_t)nb, 0);
  for (int ia = 0; ia < na; ++ia)
    for (int iz = 0; iz < na; ++iz) {
      if (iz == ia) continue;
      if (universal_over_corners(lane, ix.upos[(size_t)(ra.first + ia)], boxB,
                                 ix.upos[(size_t)(ra.first + iz)]))
        ++(*ha)[(size_t)ia];
    }
  for (int ib = 0; ib < nb; ++ib)
    for (int iz = 0; iz < nb; ++iz) {
      if (iz == ib) continue;
      if (universal_over_corners(lane, ix.upos[(size_t)(rb.first + ib)], boxA,
                                 ix.upos[(size_t)(rb.first + iz)]))
        ++(*hb)[(size_t)ib];
    }
}

}  // namespace detail_bs

// Collecte les boules candidates des trois lanes (generateurs seulement —
// AUCUN census ici : il se fait une fois par cle unique, en aval).
// Comparaison exacte de mu = A/B entre sites du MEME cote (B1, B2 > 0
// exiges — l'appelant normalise) : signe de A1·B2 − A2·B1. Largeurs u16 :
// |A| < 2^107 (puissance q3), |B| < 2^54 (normale × ecart) — produits
// croises < 2^161 < 2^192, U192 suffit.
inline int cmp_mu_same_side(i128 A1, i64 B1, i128 A2, i64 B2) {
  const int s1 = A1 < 0 ? -1 : (A1 > 0 ? 1 : 0);
  const int s2 = A2 < 0 ? -1 : (A2 > 0 ? 1 : 0);
  if (s1 != s2) return s1 < s2 ? -1 : 1;
  if (s1 == 0) return 0;
  const U192 m1 = mul_level_192(detail_ev::uabs(A1), (u128)(u64)B2);
  const U192 m2 = mul_level_192(detail_ev::uabs(A2), (u128)(u64)B1);
  const int c = cmp_u192(m1, m2);
  return s1 > 0 ? c : -c;
}

// FILTRE DE PROFONDEUR A LA GENERATION (reponse a la question du minorant
// par boule) : le cover de l'ancre est un SOUS-ENSEMBLE du nuage, donc
// #{z ∈ cover : P_B(z) < 0} MINORE |I_B| — un minorant qui atteint h_q tue
// le candidat exactement comme la passe count-only l'aurait fait, mais
// AVANT l'emission (ni tri, ni descente d'arbre par boule ; le scan sort
// des l'atteinte du seuil, quelques dizaines de nanosecondes par candidat
// profond). La completude du cover n'est PAS requise : un sous-compte ne
// tue jamais a tort. MUTANT genfilter-nonstrict : P <= 0 compte la
// coquille (et les supports memes) — des boules a plateau meurent a tort,
// le juge voit les evenements manquants.
//
// SELECTION AXIALE BORNEE (lane q4, reponse auditeur « axial borne ») : le
// faisceau des spheres par le seed (a,b,x) est parametre par
// mu_z = A_z/B_z (A = puissance q3, B = normale·(z−a)) ; cote B > 0, tout
// predecesseur STRICT de mu_y est interieur strict a la sphere de y (cote
// B < 0 : successeur) ; les permanents (B = 0, A < 0) sont interieurs a
// TOUTE la famille. Une completion utile appartient donc au prefixe borne
// p + preds <= h_4 − 1 : trois balayages lineaires du cover (p ; seuils
// bornes k = h_4 − p <= 8 par cote, multiplicites comptees ; retenue TIES
// INCLUS — eliminer les egalites de frontiere selon l'ordre memoire serait
// faux), puis UNE emission par groupe exact de mu — un groupe de meme mu
// est une seule sphere, l'aval consomme des BallKeys. Le representant emis
// est le MINIMUM ball_candidate_less des membres VALIDES du groupe (les
// predicats de support ne sont jamais l'autorite sur un seul representant).
// Les deux chemins rendent le MEME objet post-RLE (porte appariee, egalite
// cles + arite + representation). RESULTAT NEGATIF HONNETE (reçu axial
// borne) : sur CPU, la baseline rejette la plupart des completions a la
// LENTILLE (~3 operations i64) tandis que le balayage axial paye A,B sur
// TOUS les sites de TOUS les seeds — mesure : t_gen +7 % a n=1600 malgre
// 18,8 M -> 0,8 M d'evaluations q4 ; les deux postes croissent au meme
// rythme, pas de croisement CPU. Le chemin axial reste OPT-IN pour le port
// GPU (travail borne regulier, sans sorties anticipees divergentes) — la
// production CPU est la baseline (axial_bounded = false par defaut).
// MUTANTS du chemin axial : axial-short-group (k−1 : perd la boule de
// frontiere de profondeur), axial-drop-ties (< au lieu de <= : perd le
// groupe ex aequo), axial-first-rep (premier membre valide au lieu du
// minimum canonique : la representation de niveau post-RLE change).
inline void collect_candidate_balls(const CloudIndex& ix, i64 s, u64 smax_eff,
                                    std::vector<BallCandidate>* out,
                                    BallStreamStats* st,
                                    bool mutant_genfilter_nonstrict = false,
                                    bool axial_bounded = false,
                                    u32 axial_flags = 0) {
  out->clear();
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax_eff), lane_h(Lane::kQ3, smax_eff),
                       lane_h(Lane::kQ4, smax_eff)};
  std::vector<detail_bs::AliveRect> alive;
  std::vector<u64> ha, hb;
  std::vector<CoverPoint> cover;
  std::vector<AxialSite> axial;
  std::vector<NodeRef> handles;
  u64 cover_nodes = 0, visits = 0;
  // ---- q2.
  detail_bs::wspd_alive(ix, s, h_of, 0, 0b001, h_of[0], &alive);
  st->rect_alive[0] = alive.size();
  for (const auto& ar : alive) {
    detail_bs::corner_histograms(ix, Lane::kQ2, ar, &ha, &hb);
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const u64 need = h_of[0] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++st->anchors[0];
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need)
          continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        out->push_back(BallCandidate{q2_ball_key(pa, pb),
                                     promote_q3_level(q2_exact_level(D2)), 2});
        ++st->candidates[0];
      }
  }
  // ---- q3.
  detail_bs::wspd_alive(ix, s, h_of, 1, 0b010, h_of[1], &alive);
  st->rect_alive[1] = alive.size();
  for (const auto& ar : alive) {
    detail_bs::corner_histograms(ix, Lane::kQ3, ar, &ha, &hb);
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const AxisBox boxA = box_of_node(ix, ar.r.a);
    const AxisBox boxB = box_of_node(ix, ar.r.b);
    rect_cover_handles(ix, boxA, boxB, 3, false, &handles, &cover_nodes);
    const u64 need = h_of[1] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++st->anchors[1];
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need)
          continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, handles, pa, pb, D2, 3, &cover, &visits);
        for (const CoverPoint& cp : cover) {
          if (cp.u == ua || cp.u == ub) continue;
          const P3& px = ix.upos[(size_t)cp.u];
          if (!is_acute_seed(pa, pb, px, D2, ix.bucket_ids[ix.bucket_start[(size_t)ua]],
                             ix.bucket_ids[ix.bucket_start[(size_t)ub]],
                             ix.bucket_ids[ix.bucket_start[(size_t)cp.u]]))
            continue;
          const Q3Form f3 = q3_form(pa, pb, px);
          u64 depth = 0;
          bool deep = false;
          for (const CoverPoint& cz : cover) {
            const i128 pw = q3_power(f3, ix.upos[(size_t)cz.u]);
            if ((pw < 0 || (mutant_genfilter_nonstrict && pw <= 0)) &&
                ++depth >= h_of[1]) {
              deep = true;
              break;
            }
          }
          if (deep) {
            ++st->gen_depth_killed[1];
            continue;
          }
          out->push_back(BallCandidate{q3_ball_key(f3),
                                       promote_q3_level(q3_exact_level(pa, pb, px)),
                                       3});
          ++st->candidates[1];
        }
      }
  }
  // ---- q4.
  detail_bs::wspd_alive(ix, s, h_of, 2, 0b100, h_of[2], &alive);
  st->rect_alive[2] = alive.size();
  for (const auto& ar : alive) {
    detail_bs::corner_histograms(ix, Lane::kQ4, ar, &ha, &hb);
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const AxisBox boxA = box_of_node(ix, ar.r.a);
    const AxisBox boxB = box_of_node(ix, ar.r.b);
    rect_cover_handles(ix, boxA, boxB, 3, false, &handles, &cover_nodes);
    const u64 need = h_of[2] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++st->anchors[2];
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need)
          continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, handles, pa, pb, D2, 3, &cover, &visits);
        // COMPTE W_4 EXACT PAR ANCRE : tout z ∈ W_4(a,b) est strictement
        // interieur a TOUTE boule q4 possedee par l'ancre (cœur universel
        // owner + Jung — il n'y a pas de region anchor-only plus grande).
        // n4 >= h_4 tue l'ANCRE entiere avant les boucles seed × completion ;
        // W_4 ⊆ W_2 ⊆ cover coef 3, et un sous-compte ne tue jamais a tort.
        {
          u64 n4 = 0;
          for (const CoverPoint& cz : cover) {
            if (cz.u == ua || cz.u == ub) continue;
            if (in_spindle(Lane::kQ4, pa, pb, ix.upos[(size_t)cz.u]) &&
                ++n4 >= h_of[2])
              break;
          }
          if (n4 >= h_of[2]) {
            ++st->anchors_killed_w4;
            continue;
          }
        }
        const auto pid = [&](i32 u) { return ix.bucket_ids[ix.bucket_start[(size_t)u]]; };
        for (const CoverPoint& cx : cover) {
          if (cx.u == ua || cx.u == ub) continue;
          const P3& px = ix.upos[(size_t)cx.u];
          if (!is_acute_seed(pa, pb, px, D2, pid(ua), pid(ub), pid(cx.u))) continue;
          const i64 l_ax = p3_norm2(p3_sub(px, pa));
          const i64 l_bx = p3_norm2(p3_sub(px, pb));
          // Cœur universel du seed (audit « axial arbre et cœur de seed »)
          // — COMMUN aux deux chemins : la porte appariee reste appariee,
          // et le juge des petits n protege la regle elle-meme (un temoin
          // est un interieur strict de toute sphere admissible, la sortie
          // post-RLE est inchangee — argument depth_dead/W_4 en tete de
          // seed_core_kills).
          const Q3Form f3s = q3_form(pa, pb, px);
          const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
          {
            const auto c0 = std::chrono::steady_clock::now();
            const i128 Jb = (i128)D2 * (3 * f3s.g - 2 * (i128)l_ax * l_bx);
            const bool dead = seed_core_kills(
                ix, handles, f3s, nrm, Jb, h_of[2], ua, ub, cx.u,
                (axial_flags & kAxialSeedCoreNonstrict) != 0, st);
            st->t_seed_core_ms += std::chrono::duration<double, std::milli>(
                                      std::chrono::steady_clock::now() - c0)
                                      .count();
            if (dead) {
              ++st->seeds_killed_seed_core;
              continue;
            }
          }
          // Predicats du chemin de base pour UNE completion : lentille,
          // owner 6 aretes, exact-once du seed, det, centre strict. Rend
          // true et remplit le candidat si la completion est valide.
          const auto valid_completion = [&](i32 uy, BallCandidate* cand,
                                            Q4Form* f4out) {
            if (uy == cx.u || uy == ua || uy == ub) return false;
            const P3& py = ix.upos[(size_t)uy];
            const i64 l_ay = p3_norm2(p3_sub(py, pa));
            const i64 l_by = p3_norm2(p3_sub(py, pb));
            if (l_ay > D2 || l_by > D2) return false;
            const i64 l_xy = p3_norm2(p3_sub(py, px));
            if (l_xy > D2) return false;
            if (!tetra_owned_by(D2, l_ax, l_ay, l_bx, l_by, l_xy, pid(ua),
                                pid(ub), pid(cx.u), pid(uy)))
              return false;
            // Exact-once du seed (le RLE dedupliquerait de toute facon ;
            // ceci borne le flux de candidats).
            const P3 vy{2 * py.x - pa.x - pb.x, 2 * py.y - pa.y - pb.y,
                        2 * py.z - pa.z - pb.z};
            if (p3_norm2(vy) > D2 && pid(uy) < pid(cx.u)) return false;
            const Q4Form f4 = q4_form(pa, pb, px, py);
            if (f4.det == 0) return false;
            if (!q4_center_strictly_inside(f4, pa, pb, px, py)) return false;
            *cand = BallCandidate{q3_ball_key_reduce(q4_ball_form(f4)),
                                  q4_level_raw(f4), 4};
            *f4out = f4;
            return true;
          };
          const auto depth_dead = [&](const Q4Form& f4) {
            u64 depth = 0;
            for (const CoverPoint& cz : cover) {
              const i128 pw = q4_power(f4, ix.upos[(size_t)cz.u]);
              if ((pw < 0 || (mutant_genfilter_nonstrict && pw <= 0)) &&
                  ++depth >= h_of[2])
                return true;
            }
            return false;
          };
          if (!axial_bounded) {
            // BASELINE APPARIEE : enumeration de toutes les completions —
            // meme objet post-RLE, seulement plus de candidats evalues.
            for (const CoverPoint& cy : cover) {
              BallCandidate cand;
              Q4Form f4;
              if (!valid_completion(cy.u, &cand, &f4)) continue;
              if (depth_dead(f4)) {
                ++st->gen_depth_killed[2];
                continue;
              }
              out->push_back(cand);
              ++st->candidates[2];
            }
            continue;
          }
          // --- SELECTION AXIALE BORNEE (en-tete de fonction). ---
          ++st->axial_seeds;
          const auto ta0 = std::chrono::steady_clock::now();
          // Balayage 1 : A,B caches (SoA seed-local), permanents p.
          axial.clear();
          u64 p_perm = 0;
          for (const CoverPoint& cz : cover) {
            if (cz.u == ua || cz.u == ub || cz.u == cx.u) continue;
            const P3& pz = ix.upos[(size_t)cz.u];
            const i128 A = q3_power(f3s, pz);
            const i64 B = p3_dot(nrm, p3_sub(pz, pa));
            if (B == 0) {
              if (A < 0) ++p_perm;  // interieur de TOUTE la famille
              continue;
            }
            axial.push_back(AxialSite{A, B, cz.u});
          }
          st->axial_sites += axial.size();
          if (p_perm >= h_of[2]) {
            ++st->axial_seeds_dead_perm;
            continue;
          }
          u64 kk = h_of[2] - p_perm;
          if ((axial_flags & kAxialShortGroup) && kk > 1) --kk;  // MUTANT
          // Balayage 2 : seuils bornes des deux cotes. Cote negatif
          // normalise (−A, −B) : mu INCHANGE (−A/−B = A/B) mais B > 0 pour
          // le comparateur — la selection y est DESCENDANTE (interieur ssi
          // mu_z > mu_y : garder les k plus GRANDES mu ; cote positif,
          // interieur ssi mu_z < mu_y : les k plus petites). Tableau fixe
          // trie dans l'ordre de la direction, k <= 8 ; plein : remplacer
          // l'extremum si STRICTEMENT meilleur (l'egalite au seuil ne
          // change pas la k-ieme valeur).
          AxialSite side_arr[2][10];
          size_t side_n[2] = {0, 0};
          const auto dcmp = [&](int side, const AxialSite& x, const AxialSite& y) {
            const int c = cmp_mu_same_side(x.A, x.B, y.A, y.B);
            return side == 0 ? c : -c;
          };
          const auto push_bounded = [&](const AxialSite& sx, int side) {
            AxialSite* arr = side_arr[side];
            size_t& n = side_n[side];
            if (n == (size_t)kk && dcmp(side, sx, arr[n - 1]) >= 0) return;
            size_t i = (n < (size_t)kk) ? n++ : (size_t)kk - 1;
            while (i > 0 && dcmp(side, sx, arr[i - 1]) < 0) {
              arr[i] = arr[i - 1];
              --i;
            }
            arr[i] = sx;
          };
          for (const AxialSite& sz : axial) {
            if (sz.B > 0) push_bounded(sz, 0);
            else push_bounded(AxialSite{-sz.A, -sz.B, sz.u}, 1);
          }
          const auto ta1 = std::chrono::steady_clock::now();
          st->t_axial_ab_ms +=
              std::chrono::duration<double, std::milli>(ta1 - ta0).count();
          // Balayage 3 — SWEEP A DEUX COTES (contre-audit 63d364a) : la
          // profondeur stricte sur le cover se LIT dans les deux ordres
          // axiaux, exactement :
          //   d_cover(mu) = p + #{B>0, mu_z < mu} + #{B<0, mu_z > mu}.
          // Les racines viables vivent dans [L, U] (U = k-ieme plus petite
          // mu positive, L = k-ieme plus grande negative, multiplicites
          // comprises ; un cote a moins de k racines => seuil infini).
          // TABLE UNIQUE de groupes de mu exacts fusionnant les DEUX
          // signes (une meme sphere a coquille bilaterale n'est plus emise
          // deux fois), prefixes positifs / suffixes negatifs, et mort du
          // groupe AVANT valid_completion/q4_form — le scan depth_dead du
          // chemin axial est SUPPRIME (assertion de reception kAxialVerify
          // : d_j == scan complet). TIES DE FRONTIERE INCLUS (le mutant
          // drop-ties les perd) ; representant MINIMUM canonique des
          // membres valides (mutant first-rep : premier valide).
          const bool has_U = side_n[0] == (size_t)kk;
          const bool has_L = side_n[1] == (size_t)kk;
          const AxialSite Uth = has_U ? side_arr[0][kk - 1] : AxialSite{};
          const AxialSite Lth = has_L ? side_arr[1][kk - 1] : AxialSite{};
          struct MuGroup {
            AxialSite head;  // normalise (B > 0)
            u64 npos = 0, nneg = 0;
            std::vector<i32> members;
          };
          std::vector<MuGroup> groups;
          u64 pos_lt_L = 0, neg_gt_U = 0;
          for (const AxialSite& s0 : axial) {
            const bool pos = s0.B > 0;
            const AxialSite sz = pos ? s0 : AxialSite{-s0.A, -s0.B, s0.u};
            const int cL =
                has_L ? cmp_mu_same_side(sz.A, sz.B, Lth.A, Lth.B) : 1;
            const int cU =
                has_U ? cmp_mu_same_side(sz.A, sz.B, Uth.A, Uth.B) : -1;
            const bool below =
                (axial_flags & kAxialDropTies) ? cL <= 0 : cL < 0;  // MUTANT
            const bool above =
                (axial_flags & kAxialDropTies) ? cU >= 0 : cU > 0;  // MUTANT
            if (below) {
              // Racine positive sous L : morte par le cote OPPOSE (>= k
              // negatifs strictement au-dessus => d >= h_4) ; elle reste un
              // site du prefixe positif des groupes survivants.
              if (pos) {
                ++pos_lt_L;
                ++st->axial_groups_killed_two_sided;
              }
              continue;
            }
            if (above) {
              // Racine negative sur U : morte par le cote OPPOSE (>= k
              // positifs strictement en dessous) ; elle reste un site du
              // suffixe negatif des groupes survivants.
              if (!pos) {
                ++neg_gt_U;
                ++st->axial_groups_killed_two_sided;
              }
              continue;
            }
            bool placed = false;
            for (auto& g : groups)
              if (cmp_mu_same_side(sz.A, sz.B, g.head.A, g.head.B) == 0) {
                if (pos) ++g.npos;
                else ++g.nneg;
                g.members.push_back(sz.u);
                placed = true;
                break;
              }
            if (!placed) {
              groups.push_back(MuGroup{sz, pos ? 1u : 0u, pos ? 0u : 1u, {sz.u}});
            }
          }
          // Tri des groupes par mu croissante (<= 2k, insertion).
          std::sort(groups.begin(), groups.end(),
                    [](const MuGroup& x, const MuGroup& y) {
                      return cmp_mu_same_side(x.head.A, x.head.B, y.head.A,
                                              y.head.B) < 0;
                    });
          // Prefixes positifs et suffixes negatifs, puis d_j exact.
          const size_t ng = groups.size();
          u64 pref = pos_lt_L;
          std::vector<u64> pos_before(ng), neg_after(ng);
          for (size_t j = 0; j < ng; ++j) {
            pos_before[j] = pref;
            pref += groups[j].npos;
          }
          u64 suff = neg_gt_U;
          for (size_t j = ng; j-- > 0;) {
            neg_after[j] = suff;
            suff += groups[j].nneg;
          }
          const auto ta2 = std::chrono::steady_clock::now();
          st->t_reduce_ms +=
              std::chrono::duration<double, std::milli>(ta2 - ta1).count();
          for (size_t j = 0; j < ng; ++j) {
            u64 dj = p_perm + pos_before[j];
            if (!(axial_flags & kAxialIgnoreOpposite))  // MUTANT : cote oppose
              dj += (axial_flags & kAxialReverseNeg)
                        ? (suff - neg_after[j] - groups[j].nneg)  // MUTANT
                        : neg_after[j];
            if (axial_flags & kAxialDepthNonstrict)  // MUTANT : sa coquille
              dj += groups[j].npos + groups[j].nneg;
            if (dj >= h_of[2]) {
              ++st->axial_groups_killed_two_sided;
              continue;
            }
            bool have = false;
            BallCandidate best{};
            Q4Form bestf4{};
            for (const i32 uy : groups[j].members) {
              BallCandidate cand;
              Q4Form f4;
              if (!valid_completion(uy, &cand, &f4)) continue;
              if (!have || ball_candidate_less(cand, best)) {
                best = cand;
                bestf4 = f4;
                have = true;
              }
              if ((axial_flags & kAxialFirstRep) && have) break;  // MUTANT
            }
            if (!have) continue;
            if (axial_flags & kAxialVerify) {
              // Assertion de reception : d_j est EXACTEMENT le compte du
              // scan q4_power < 0 sur le cover (identite a deux cotes).
              u64 scan = 0;
              for (const CoverPoint& cz : cover)
                if (q4_power(bestf4, ix.upos[(size_t)cz.u]) < 0) ++scan;
              if (scan != p_perm + pos_before[j] + neg_after[j])
                ++st->axial_verify_mismatch;
            }
            out->push_back(best);
            ++st->candidates[2];
            ++st->axial_groups_emitted;
          }
          st->t_emit_ms += std::chrono::duration<double, std::milli>(
                               std::chrono::steady_clock::now() - ta2)
                               .count();
        }
      }
  }
}

// Census EXACT d'une boule par sa forme primitive : interieurs stricts et
// coquille complete. Descente separable par axe (parabole convexe par axe,
// minimum de reseau aux entiers voisins du sommet, maxima aux bornes).
// Retourne false si |I_B| depasse `interior_cap` (boule sans K <= K_max) OU
// si |U_B| depasse `shell_cap` (a traiter en resource_exhausted par
// l'appelant — jamais une troncature silencieuse).
// PASSE COUNT-ONLY DE PROFONDEUR (audit « prefiltre exact par boule ») :
// decide `|I_B| >= h` AVANT toute materialisation de I_B/U_B.
//
// SEUIL EXACT PAR ARITE MINIMALE (audit § 1) : apres le RLE, q_min =
// arite du premier candidat du groupe (le tri met l'arite avant la
// representation) est la cardinalite minimale d'un support de la
// miniboule — tout T d'un evenement du plateau contient un support
// minimal, donc |T| >= q_min, et un evenement utile a K <= K_max exige
// |I_B| <= K_max + 1 - q_min. Regle de mort : |I_B| >= h_qmin =
// K_max + 2 - q_min (10/9/8 interieurs pour q_min = 2/3/4). La regle
// s'appuie sur la completude par lane (en-tete) : une boule au label
// q_min = 4 mais de support reel 2 serait NON pertinente pour q2 (sinon
// la lane q2 l'aurait emise) — la tuer plus tot est exact.
//
// Decisions de descente (les inegalites comptent) :
//   mn >= 0 : aucun point strictement interieur — ELAGUE, y compris les
//             regions de coquille pure (mn == 0) que le census, lui,
//             doit descendre ;
//   mx <  0 : tout le nœud est strictement interieur — range-add sature
//             en O(1), sans allocation. STRICT : a mx == 0 une coquille
//             serait comptee interieure (MUTANT range-add-max-le-zero,
//             tue : des boules a plateau meurent a tort) ;
//   sinon    scission ; a la feuille, test exact (mn == mx).
// Rend true des que count atteint h ; sinon false et *count_out = compte
// EXACT des interieurs stricts (recoupe par la passe census, invariant).
inline bool ball_depth_at_least(const CloudIndex& ix, const Q3BallKey& k,
                                u64 h, u64* count_out,
                                bool mutant_range_add_le = false,
                                BallStreamStats* st = nullptr) {
  *count_out = 0;
  if (ix.nodes.empty()) return h == 0;
  const auto axis_val = [&](int i, i64 t) { return k.a * ((i128)t * t) + k.b[i] * t; };
  const auto axis_min = [&](int i, i64 lo, i64 hi) {
    const i128 num = -k.b[i];
    const i128 den = 2 * k.a;
    i128 q = num / den;
    if (num % den != 0 && ((num < 0) != (den < 0))) --q;
    const i64 t1 = (i64)q;
    i128 best = 0;
    bool first = true;
    for (const i64 cand : {t1, t1 + 1, lo, hi}) {
      const i64 c = std::min(std::max(cand, lo), hi);
      const i128 v = axis_val(i, c);
      if (first || v < best) best = v, first = false;
    }
    return best;
  };
  const auto axis_max = [&](int i, i64 lo, i64 hi) {
    return std::max(axis_val(i, lo), axis_val(i, hi));
  };
  u64 count = 0;
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    if (st) ++st->prefilter_nodes;
    const AxisBox bz = box_of_node(ix, z);
    i128 mn = k.c, mx = k.c;
    for (int i = 0; i < 3; ++i) {
      mn += axis_min(i, bz.lo[i], bz.hi[i]);
      mx += axis_max(i, bz.lo[i], bz.hi[i]);
    }
    if (mn >= 0) continue;  // rien de STRICTEMENT interieur (coquille incluse)
    if (mx < 0 || (mutant_range_add_le && mx <= 0)) {
      const u64 w = (z < 0) ? 1
                            : (u64)(ix.nodes[(size_t)z].last -
                                    ix.nodes[(size_t)z].first + 1);
      if (st) st->prefilter_range_add_mass += w;
      count += w;
      if (count >= h) return true;
      continue;
    }
    if (z < 0) {
      // Test exact au point (la boite serree d'une feuille rend mn == mx,
      // mais le test ne SUPPOSE pas cette etroitesse).
      if (st) ++st->prefilter_leaf_tests;
      const P3& p = ix.upos[(size_t)(-1 - z)];
      const i128 pw = k.a * p3_norm2(p) + k.b[0] * p.x + k.b[1] * p.y +
                      k.b[2] * p.z + k.c;
      if (pw < 0 && ++count >= h) return true;
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  *count_out = count;
  return false;
}

inline bool ball_census(const CloudIndex& ix, const Q3BallKey& k,
                        size_t interior_cap, size_t shell_cap,
                        std::vector<i32>* interior, std::vector<i32>* shell,
                        bool* shell_overflow) {
  interior->clear();
  shell->clear();
  *shell_overflow = false;
  if (ix.nodes.empty()) return true;
  const auto axis_val = [&](int i, i64 t) { return k.a * ((i128)t * t) + k.b[i] * t; };
  const auto axis_min = [&](int i, i64 lo, i64 hi) {
    const i128 num = -k.b[i];
    const i128 den = 2 * k.a;
    i128 q = num / den;
    if (num % den != 0 && ((num < 0) != (den < 0))) --q;
    const i64 t1 = (i64)q;
    i128 best = 0;
    bool first = true;
    for (const i64 cand : {t1, t1 + 1, lo, hi}) {
      const i64 c = std::min(std::max(cand, lo), hi);
      const i128 v = axis_val(i, c);
      if (first || v < best) best = v, first = false;
    }
    return best;
  };
  const auto axis_max = [&](int i, i64 lo, i64 hi) {
    return std::max(axis_val(i, lo), axis_val(i, hi));
  };
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    i128 mn = k.c, mx = k.c;
    for (int i = 0; i < 3; ++i) {
      mn += axis_min(i, bz.lo[i], bz.hi[i]);
      mx += axis_max(i, bz.lo[i], bz.hi[i]);
    }
    if (mn > 0) continue;  // strict : mn == 0 descend (coquilles a voir)
    if (z < 0) {
      const i32 u = -1 - z;
      const P3& p = ix.upos[(size_t)u];
      const i128 pw = k.a * p3_norm2(p) + k.b[0] * p.x + k.b[1] * p.y +
                      k.b[2] * p.z + k.c;
      if (pw < 0) {
        interior->push_back(u);
        if (interior->size() > interior_cap) return false;
      } else if (pw == 0) {
        shell->push_back(u);
        if (shell->size() > shell_cap) {
          *shell_overflow = true;
          return false;
        }
      }
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return true;
}

}  // namespace mhgp4
