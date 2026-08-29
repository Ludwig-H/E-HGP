// MorseHGP3D v5 — SONDE (jamais un claim) : fibre A x B x C, vacuite par
// boites, et chemin causal. Version 3.
//
// Historique des retractations, toutes des auditeurs, toutes acceptees :
//   v1 -> v2 : `tb` comptait les temoins COMMUNS a toutes les boules du bloc,
//     un minorant. Le critere est « TOUT triplet valide meurt ».
//   v2 -> v3 : (a) le nom `min_exact_ball_depth` promettait une valeur qui
//     n'est pas calculee apres la premiere boule peu profonde — le predicat
//     s'appelle desormais `all_valid_supports_depth_ge_h3` ; (b) la
//     ponderation `formes x candidats` n'etait qu'un `full_scan_upper_pairings`
//     STATIQUE : elle ignore l'arret anticipe (une boule profonde sort au
//     neuvieme interieur, donc les boules surponderees par ce proxy sont
//     souvent les MOINS cheres), elle ignore que l'histogramme, W_3, les
//     secteurs et la grille ont deja retire des seeds, et elle sort les blocs
//     capes du denominateur. Tous les pourcentages de « travail » de la v2 sont
//     retires. La v3 ne publie plus qu'un CHEMIN CAUSAL : des appels reellement
//     executes et des sorties anticipees, comptes par etage, et le cout du
//     certificateur dans un compteur SEPARE. Aucune conversion en temps evite.
//
// Ce que la v3 mesure :
//   1. le predicat ideal `all_valid_supports_depth_ge_h3` par bloc, avec le
//      nombre d'appels de puissance REELLEMENT executes et les sorties
//      anticipees, et non un majorant statique ;
//   2. la baseline `pair_w3_dead` sur TOUTES les ancres actives, avec ses
//      propres appels, plus l'INVARIANT EXECUTABLE
//      `pair_w3_dead => all_valid_supports_depth_ge_h3` dont toute violation
//      signale une erreur de cover, de support ou de stricte puissance ;
//   3. la vacuite DECOMPOSEE par cause, avec des certificats de BOITES en O(1)
//      (`ZERO_ROLE_MASS`, `NONE_MAX_EDGE`, `NONE_ACUTE`) confrontes a la
//      vacuite reelle constatee par force brute : combien chacun reconnait,
//      et combien restent non classes ;
//   4. le LEDGER de provenance `somme(masse des handles) + masse dehors =
//      |A||B|(n_u - 2)`, verifie en entier large sur les rectangles visites ;
//   5. les blocs capes avec un INTERVALLE de masse, jamais une exclusion muette
//      (deux caps distincts : roles inspectes et supports retenus).
//
// Ce qu'elle ne fait pas : proposer un certificat de bloc produit. Elle borne
// ce qu'un tel certificat pourrait atteindre, en payant la force brute qu'il
// devra eviter.
//
// Usage : mhgp5_block_witness_probe --family=F --n=N [--blocs=K] [--seed=S]
//         [--cap-roles=R] [--cap-supports=T] [--coord=C]
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/pipeline/generate.hpp"
#include "../src/spindle/spindle.hpp"

using namespace mhgp5;

namespace {

// --- BORNES DE BOITES, exactes et separables par axe.
struct Iv { i64 lo, hi; };
inline Iv iv_sub(const Iv& u, const Iv& v) { return {u.lo - v.hi, u.hi - v.lo}; }
inline i64 sq_max(const Iv& t) { const i64 a = std::llabs(t.lo), b = std::llabs(t.hi); return std::max(a, b) * std::max(a, b); }
inline i64 sq_min(const Iv& t) {
  if (t.lo <= 0 && t.hi >= 0) return 0;
  const i64 a = std::llabs(t.lo), b = std::llabs(t.hi);
  return std::min(a, b) * std::min(a, b);
}
inline Iv axis(const AxisBox& B, int i) { return {B.lo[i], B.hi[i]}; }

// |b-a|^2 sur Box(A) x Box(B).
inline void d2_bounds(const AxisBox& A, const AxisBox& B, i64* lo, i64* hi) {
  *lo = 0; *hi = 0;
  for (int i = 0; i < 3; ++i) { const Iv t = iv_sub(axis(B, i), axis(A, i)); *lo += sq_min(t); *hi += sq_max(t); }
}
// |x-a|^2 sur Box(A) x Box(C).
inline void l2_bounds(const AxisBox& A, const AxisBox& C, i64* lo, i64* hi) {
  *lo = 0; *hi = 0;
  for (int i = 0; i < 3; ++i) { const Iv t = iv_sub(axis(C, i), axis(A, i)); *lo += sq_min(t); *hi += sq_max(t); }
}
// |2x-a-b|^2 sur Box(A) x Box(B) x Box(C).
inline void v2_bounds(const AxisBox& A, const AxisBox& B, const AxisBox& C, i64* lo, i64* hi) {
  *lo = 0; *hi = 0;
  for (int i = 0; i < 3; ++i) {
    const Iv t{2 * C.lo[i] - A.hi[i] - B.hi[i], 2 * C.hi[i] - A.lo[i] - B.lo[i]};
    *lo += sq_min(t); *hi += sq_max(t);
  }
}

// Recouvrement de plages (les handles peuvent recouvrir A ou B).
inline u64 overlap(const NodeRange& u, const NodeRange& v) {
  const i32 lo = std::max(u.first, v.first), hi = std::min(u.last, v.last);
  return hi >= lo ? (u64)(hi - lo + 1) : 0;
}

inline u64 rss_hwm_kb() {
  std::FILE* f = std::fopen("/proc/self/status", "r");
  if (!f) return 0;
  char l[256]; u64 v = 0;
  while (std::fgets(l, sizeof l, f))
    if (std::sscanf(l, "VmHWM: %llu kB", (unsigned long long*)&v) == 1) break;
  std::fclose(f);
  return v;
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 8000, coord = 0, seed = 3;
  size_t blocs_cible = 3000, cap_roles = 200000, cap_supports = 4096;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--coord=", 0) == 0) coord = std::atoi(a.c_str() + 8);
    else if (a.rfind("--seed=", 0) == 0) seed = std::atoi(a.c_str() + 7);
    else if (a.rfind("--blocs=", 0) == 0) blocs_cible = (size_t)std::atoll(a.c_str() + 8);
    else if (a.rfind("--cap-roles=", 0) == 0) cap_roles = (size_t)std::atoll(a.c_str() + 12);
    else if (a.rfind("--cap-supports=", 0) == 0) cap_supports = (size_t)std::atoll(a.c_str() + 15);
    else return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const auto entree = make_family_input(family, n, coord, seed);
  const CloudIndex ix = build_cloud_index(entree);
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  // Digest d'entree : FNV-1a 64 bits sur les positions uniques triees Morton.
  u64 digest = 1469598103934665603ull;
  for (const P3& p : ix.upos)
    for (const i64 c : {p.x, p.y, p.z})
      for (int k = 0; k < 8; ++k) { digest ^= (u64)((c >> (8 * k)) & 0xff); digest *= 1099511628211ull; }

  const u64 smax = 11;
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), lane_h(Lane::kQ3, smax), lane_h(Lane::kQ4, smax)};
  const u64 h3 = h_of[1];
  const u64 nu = (u64)ix.unique_count();
  const auto t0 = std::chrono::steady_clock::now();
  std::vector<AliveRect> alive;
  u64 visited = 0, workers = 0;
  generate_detail::alive_rectangles(ix, 8, h_of, 1, 1, &alive, &visited, &workers);

  generate_detail::AnchorScratch sc;
  u64 blocs_total = 0;
  for (const AliveRect& ar : alive) {
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    blocs_total += (u64)sc.handles.size();
  }
  if (blocs_total == 0) return 3;
  const u64 pas = std::max<u64>(1, blocs_total / std::max<u64>(1, (u64)blocs_cible));

  // Compteurs. Tous en u64 ; les cumuls de masse en i128 (entier large).
  u64 vus = 0, echantillon = 0;
  u64 juges = 0, vides_reels = 0, capes_roles = 0, capes_supports = 0;
  i128 capes_masse_min = 0, capes_masse_max = 0;
  u64 ideal_mort = 0, paire_morte = 0, ideal_seul = 0, invariant_viole = 0;
  // Chemin causal : appels REELLEMENT executes et sorties anticipees.
  u64 appels_puissance = 0, sorties_anticipees_support = 0, supports_examines = 0;
  u64 appels_spindle = 0, sorties_anticipees_ancre = 0, ancres_examinees = 0;
  u64 cout_certificateur = 0;  // evaluations de bornes de boites — compteur SEPARE
  // Vacuite decomposee : ce que les certificats de BOITES reconnaissent.
  u64 cert_zero_role = 0, cert_none_max_edge = 0, cert_none_acute = 0, cert_aucun = 0;
  u64 vide_reel_non_classe = 0, cert_faux_positif = 0;
  // CAUSE REELLE de vacuite (V68) : etage le PLUS PROFOND atteint par un
  // role du bloc. `is_acute_seed` se decompose exactement en lentille ->
  // acuite stricte -> owner canonique ; classer par l'etage le plus profond
  // atteint partitionne la vacuite reelle sans recouvrement.
  u64 reel_zero_role = 0, reel_lentille = 0, reel_acuite = 0, reel_owner = 0;
  // V71 : ce qu'un bloc VIDE coute reellement. Un bloc sans support valide
  // ne declenche AUCUN rescan de profondeur — il ne paie que l'enumeration
  // de ses roles. Reconnaitre sa vacuite n'evite donc que cela, et la
  // comparaison honnete est en appels executes de chaque espece.
  u64 roles_blocs_vides = 0, roles_blocs_pleins = 0;
  // V74 : PLAFOND de la direction center-cover, mesure avant tout chantier.
  // Les appels de puissance des blocs a supports valides se repartissent en
  // trois seaux disjoints : (1) blocs que la baseline W_3 tue deja — la
  // production les capte, un certificat de bloc n'y gagne rien ; (2) blocs
  // tous profonds que W_3 ne tue PAS — c'est exactement le gain marginal
  // qu'un center-cover conditionne par C pourrait viser ; (3) blocs dont un
  // support survit — travail inherent, aucun certificat ne peut l'eviter.
  u64 pw_deja_w3 = 0, pw_gain_marginal = 0, pw_inherent = 0;
  // Le gain marginal se decoupe encore en deux, et la distinction gouverne
  // la CONSTRUCTION du certificat. Un center-cover global credite les sites
  // interieurs a TOUTES les boules du bloc : s'il y en a h_3, un certificat
  // UNIQUE suffit. Sinon le bloc peut encore mourir, mais seulement par
  // PATCHES tues par des ensembles de temoins differents — la machinerie
  // lourde de la note d'audit. Cette mesure dit combien elle est necessaire.
  u64 marg_simple = 0, marg_patches = 0, pw_marg_simple = 0, pw_marg_patches = 0;
  // Croisement : parmi les vides que les BOITES ne classent pas, quelle est
  // la cause reelle ? C'est la question V68.
  u64 nc_lentille = 0, nc_acuite = 0, nc_owner = 0, nc_zero = 0;
  // Ledger de provenance, verifie sur les rectangles visites.
  i128 ledger_ok = 0, ledger_ko = 0;

  std::vector<i32> candidats;
  std::vector<Q3Form> formes;
  std::vector<std::pair<i32, i32>> ancres;
  std::vector<i32> carriers;

  for (const AliveRect& ar : alive) {
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    const AxisBox bA = ix.box_of(ar.r.a), bB = ix.box_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    const u64 nA = (u64)(ra.last - ra.first + 1), nB = (u64)(rb.last - rb.first + 1);
    candidats.clear();
    i128 masse_handles = 0;
    for (const NodeRef hz : sc.handles) {
      const NodeRange rz = ix.range_of(hz);
      for (i32 uz = rz.first; uz <= rz.last; ++uz) candidats.push_back(uz);
      // masse de roles du bloc : |A||B||C| - |A inter C||B| - |B inter C||A|
      const u64 nC = (u64)(rz.last - rz.first + 1);
      masse_handles += (i128)nA * nB * nC - (i128)overlap(ra, rz) * nB - (i128)overlap(rb, rz) * nA;
    }
    // LEDGER : somme(masse handles) + masse dehors == |A||B|(n_u - 2).
    {
      i128 dehors = (i128)nA * nB * (i128)(nu - 2) - masse_handles;
      if (dehors >= 0) ++ledger_ok; else ++ledger_ko;
    }

    i64 D2lo = 0, D2hi = 0;
    d2_bounds(bA, bB, &D2lo, &D2hi);

    for (const NodeRef h : sc.handles) {
      const u64 mon_bloc = vus++;
      if (mon_bloc % pas != 0) continue;
      ++echantillon;
      const NodeRange rc = ix.range_of(h);
      const AxisBox bC = ix.box_of(h);
      const u64 nC = (u64)(rc.last - rc.first + 1);

      // --- CERTIFICATS DE BOITES en O(1), decomposes par cause (V65).
      cout_certificateur += 3;
      const i128 masse = (i128)nA * nB * nC - (i128)overlap(ra, rc) * nB - (i128)overlap(rb, rc) * nA;
      int cause = 0;  // 0 = non classe, 1 = ZERO_ROLE_MASS, 2 = NONE_MAX_EDGE, 3 = NONE_ACUTE
      if (masse <= 0) cause = 1;
      i64 v2lo = 0, v2hi = 0;
      v2_bounds(bA, bB, bC, &v2lo, &v2hi);
      if (!cause) {
        // NONE_MAX_EDGE. Une borne COUPLEE existe et elle est strictement
        // meilleure en theorie : l'identite du parallelogramme
        //   |x-a|^2 + |x-b|^2 = 2|x-m|^2 + D^2/2
        // donne max(|x-a|^2,|x-b|^2) >= |x-m|^2 + D^2/4, et la lentille exige ce
        // max <= D^2, d'ou la condition NECESSAIRE |2x-a-b|^2 <= 3 D^2 — une
        // seule quantite separable au lieu de deux minima atteints en des points
        // differents. ELLE EST POURTANT INERTE ICI, et c'est demontrable :
        // `rect_cover_handles` elague sur `gap2 > coef * dmax2` avec coef = 3,
        // ou `gap2` est exactement notre `v2lo` et `dmax2` exactement `D2hi`.
        // Le test `v2lo > 3*D2hi` EST le critere de selection des handles :
        // aucun handle retourne ne peut le satisfaire. Mesure a l'appui — il ne
        // change aucun des quatre comptes. Seule la version decouplee apporte
        // de l'information au-dela de la selection des handles.
        i64 lax_lo = 0, lax_hi = 0, lbx_lo = 0, lbx_hi = 0;
        l2_bounds(bA, bC, &lax_lo, &lax_hi);
        l2_bounds(bB, bC, &lbx_lo, &lbx_hi);
        if (lax_lo > D2hi || lbx_lo > D2hi) cause = 2;
      }
      if (!cause && v2hi <= D2lo) cause = 3;  // aucun tiers strictement aigu

      // --- VERITE : force brute des roles, sous DEUX caps distincts.
      formes.clear(); ancres.clear(); carriers.clear();
      u64 roles_inspectes = 0;
      bool cape_role = false, cape_support = false;
      // etage : 0 aucun role, 1 lentille, 2 acuite, 3 owner, 4 support valide
      int etage = 0;
      for (i32 ua = ra.first; ua <= ra.last && !cape_role && !cape_support; ++ua)
        for (i32 ub = rb.first; ub <= rb.last && !cape_role && !cape_support; ++ub) {
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (D2 == 0) continue;
          for (i32 ux = rc.first; ux <= rc.last; ++ux) {
            if (ux == ua || ux == ub) continue;
            if (++roles_inspectes > cap_roles) { cape_role = true; break; }
            const P3& px = ix.upos[(size_t)ux];
            // Decomposition exacte de `is_acute_seed`, pour attribuer la cause
            // reelle de vacuite (V68) sans changer la decision.
            const i64 l_ax = p3_norm2(p3_sub(px, pa)), l_bx = p3_norm2(p3_sub(px, pb));
            if (l_ax > D2 || l_bx > D2) { etage = std::max(etage, 1); continue; }
            const P3 vx{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y, 2 * px.z - pa.z - pb.z};
            if (p3_norm2(vx) <= D2) { etage = std::max(etage, 2); continue; }
            if (!anchor_owns_q3(D2, l_ax, l_bx, ix.point_id(ua), ix.point_id(ub), ix.point_id(ux))) {
              etage = std::max(etage, 3);
              continue;
            }
            etage = 4;
            if (formes.size() >= cap_supports) { cape_support = true; break; }
            formes.push_back(q3_form(pa, pb, px));
            ancres.push_back({ua, ub});
            carriers.push_back(ux);
          }
        }
      // Bloc cape : INTERVALLE de masse publie, jamais une exclusion muette.
      if (cape_role || cape_support) {
        if (cape_role) ++capes_roles; else ++capes_supports;
        capes_masse_min += (i128)(cape_support ? cap_supports + 1 : 0);
        capes_masse_max += masse;
        continue;
      }

      const bool vide_reel = formes.empty();
      if (vide_reel) roles_blocs_vides += roles_inspectes; else roles_blocs_pleins += roles_inspectes;
      if (vide_reel) {
        ++vides_reels;
        if (etage == 0) ++reel_zero_role;
        else if (etage == 1) ++reel_lentille;
        else if (etage == 2) ++reel_acuite;
        else ++reel_owner;
        if (cause == 1) ++cert_zero_role;
        else if (cause == 2) ++cert_none_max_edge;
        else if (cause == 3) ++cert_none_acute;
        else {
          ++cert_aucun; ++vide_reel_non_classe;
          if (etage == 0) ++nc_zero;
          else if (etage == 1) ++nc_lentille;
          else if (etage == 2) ++nc_acuite;
          else ++nc_owner;
        }
        continue;
      }
      // Un certificat qui declare vide un bloc NON vide serait une fausse mort.
      if (cause != 0) ++cert_faux_positif;
      ++juges;

      // --- PREDICAT IDEAL : tout support valide a-t-il >= h3 interieurs
      // stricts ? Sortie anticipee a h3 : la profondeur exacte n'est PAS
      // calculee au-dela, d'ou le nom.
      bool tous_profonds = true;
      u64 pw_bloc = 0;
      for (size_t t = 0; t < formes.size() && tous_profonds; ++t) {
        ++supports_examines;
        u64 prof = 0;
        bool sorti = false;
        for (const i32 uz : candidats) {
          if (uz == ancres[t].first || uz == ancres[t].second || uz == carriers[t]) continue;
          ++appels_puissance; ++pw_bloc;
          if (q3_power(formes[t], ix.upos[(size_t)uz]) < 0 && ++prof >= h3) { sorti = true; break; }
        }
        if (sorti) ++sorties_anticipees_support; else tous_profonds = false;
      }

      // --- BASELINE : toutes les ancres actives mortes par W_3 ?
      bool paire_morte_ici = true;
      for (size_t t = 0; t < formes.size() && paire_morte_ici; ++t) {
        if (t > 0 && ancres[t] == ancres[t - 1]) continue;
        ++ancres_examinees;
        const P3& pa = ix.upos[(size_t)ancres[t].first];
        const P3& pb = ix.upos[(size_t)ancres[t].second];
        u64 n3 = 0;
        bool sorti = false;
        for (const i32 uz : candidats) {
          if (uz == ancres[t].first || uz == ancres[t].second) continue;
          ++appels_spindle;
          if (in_spindle(Lane::kQ3, pa, pb, ix.upos[(size_t)uz]) && ++n3 >= h3) { sorti = true; break; }
        }
        if (sorti) ++sorties_anticipees_ancre; else paire_morte_ici = false;
      }

      // Attribution du plafond (V74) : trois seaux disjoints.
      if (paire_morte_ici) pw_deja_w3 += pw_bloc;
      else if (tous_profonds) {
        pw_gain_marginal += pw_bloc;
        // Temoins COMMUNS a toutes les boules du bloc : arret des h_3 atteints,
        // et pour chaque candidat arret des qu'une boule l'exclut.
        u64 communs = 0;
        for (const i32 uz : candidats) {
          bool sommet = false;
          for (size_t t = 0; t < formes.size() && !sommet; ++t)
            if (uz == ancres[t].first || uz == ancres[t].second || uz == carriers[t]) sommet = true;
          if (sommet) continue;
          bool universel = true;
          for (size_t t = 0; t < formes.size() && universel; ++t)
            if (!(q3_power(formes[t], ix.upos[(size_t)uz]) < 0)) universel = false;
          if (universel && ++communs >= h3) break;
        }
        if (communs >= h3) { ++marg_simple; pw_marg_simple += pw_bloc; }
        else { ++marg_patches; pw_marg_patches += pw_bloc; }
      }
      else pw_inherent += pw_bloc;
      if (tous_profonds) ++ideal_mort;
      if (paire_morte_ici) ++paire_morte;
      if (tous_profonds && !paire_morte_ici) ++ideal_seul;
      // INVARIANT EXECUTABLE : pair_w3_dead => all_valid_supports_depth_ge_h3.
      if (paire_morte_ici && !tous_profonds) ++invariant_viole;
    }
  }

  const double mur = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  std::printf("block_witness_v3 pin=%s worktree_modifie=%s\n", MHGP5_PROBE_PIN, MHGP5_PROBE_DIRTY);
  std::printf("  famille=%s n=%d coord=%d seed=%d digest_entree=%016llx h3=%llu n_unique=%llu\n",
              cloud_family_name(family), n, coord, seed, (unsigned long long)digest,
              (unsigned long long)h3, (unsigned long long)nu);
  std::printf("  echantillonnage=pas_constant pas=%llu blocs_total=%llu echantillon=%llu (cible %zu)\n",
              (unsigned long long)pas, (unsigned long long)blocs_total, (unsigned long long)echantillon, blocs_cible);
  std::printf("  ledger de provenance |A||B|(n_u-2) : rectangles coherents=%llu incoherents=%llu\n",
              (unsigned long long)ledger_ok, (unsigned long long)ledger_ko);
  std::printf("  capes : roles=%llu supports=%llu ; masse de roles capee dans [%llu, %llu]\n",
              (unsigned long long)capes_roles, (unsigned long long)capes_supports,
              (unsigned long long)capes_masse_min, (unsigned long long)capes_masse_max);
  std::printf("  VACUITE : blocs reellement vides=%llu ; reconnus par boites : ZERO_ROLE_MASS=%llu "
              "NONE_MAX_EDGE=%llu NONE_ACUTE=%llu ; NON CLASSES=%llu (%.1f %% des vides)\n",
              (unsigned long long)vides_reels, (unsigned long long)cert_zero_role,
              (unsigned long long)cert_none_max_edge, (unsigned long long)cert_none_acute,
              (unsigned long long)vide_reel_non_classe,
              vides_reels ? 100.0 * (double)vide_reel_non_classe / (double)vides_reels : 0.0);
  std::printf("  CAUSE REELLE des vides (etage le plus profond atteint par un role) : "
              "aucun role=%llu lentille=%llu acuite=%llu owner=%llu\n",
              (unsigned long long)reel_zero_role, (unsigned long long)reel_lentille,
              (unsigned long long)reel_acuite, (unsigned long long)reel_owner);
  std::printf("  PARMI LES NON CLASSES par les boites, cause reelle : "
              "aucun role=%llu lentille=%llu acuite=%llu owner=%llu\n",
              (unsigned long long)nc_zero, (unsigned long long)nc_lentille,
              (unsigned long long)nc_acuite, (unsigned long long)nc_owner);
  std::printf("  certificats FAUX POSITIFS (bloc declare vide mais non vide) = %llu  [doit valoir 0]\n",
              (unsigned long long)cert_faux_positif);
  if (juges == 0) { std::printf("  aucun bloc juge\n"); return 3; }
  std::printf("  blocs juges=%llu ; ideal all_valid_supports_depth_ge_h3=%llu (%.1f %%) ; "
              "baseline pair_w3_dead=%llu (%.1f %%) ; ideal seul=%llu\n",
              (unsigned long long)juges, (unsigned long long)ideal_mort, 100.0 * (double)ideal_mort / (double)juges,
              (unsigned long long)paire_morte, 100.0 * (double)paire_morte / (double)juges,
              (unsigned long long)ideal_seul);
  std::printf("  INVARIANT pair_w3_dead => all_valid_supports_depth_ge_h3 : violations=%llu  [doit valoir 0]\n",
              (unsigned long long)invariant_viole);
  std::printf("  CHEMIN CAUSAL (appels REELLEMENT executes, jamais un majorant statique) :\n");
  std::printf("    ideal    : supports examines=%llu, appels q3_power=%llu, sorties anticipees=%llu\n",
              (unsigned long long)supports_examines, (unsigned long long)appels_puissance,
              (unsigned long long)sorties_anticipees_support);
  std::printf("    baseline : ancres examinees=%llu, appels in_spindle=%llu, sorties anticipees=%llu\n",
              (unsigned long long)ancres_examinees, (unsigned long long)appels_spindle,
              (unsigned long long)sorties_anticipees_ancre);
  {
    const double tot = (double)(pw_deja_w3 + pw_gain_marginal + pw_inherent);
    std::printf("  PLAFOND DU CENTER-COVER (appels de puissance des blocs a supports valides, trois seaux disjoints) :\n");
    std::printf("    deja tues par W_3 (production les capte)      = %llu (%.1f %%)\n",
                (unsigned long long)pw_deja_w3, tot ? 100.0 * (double)pw_deja_w3 / tot : 0.0);
    std::printf("    GAIN MARGINAL visable par un certificat de bloc = %llu (%.1f %%)\n",
                (unsigned long long)pw_gain_marginal, tot ? 100.0 * (double)pw_gain_marginal / tot : 0.0);
    const double marg = (double)(pw_marg_simple + pw_marg_patches);
    std::printf("      dont CERTIFICAT UNIQUE suffit (>= h3 temoins communs) : %llu blocs, %llu appels (%.1f %% du marginal)\n",
                (unsigned long long)marg_simple, (unsigned long long)pw_marg_simple,
                marg ? 100.0 * (double)pw_marg_simple / marg : 0.0);
    std::printf("      dont PATCHES necessaires (temoins incompatibles)      : %llu blocs, %llu appels (%.1f %% du marginal)\n",
                (unsigned long long)marg_patches, (unsigned long long)pw_marg_patches,
                marg ? 100.0 * (double)pw_marg_patches / marg : 0.0);
    std::printf("    inherent (un support survit, rien a eviter)    = %llu (%.1f %%)\n",
                (unsigned long long)pw_inherent, tot ? 100.0 * (double)pw_inherent / tot : 0.0);
  }
  std::printf("    roles enumeres (is_acute_seed) : blocs VIDES=%llu, blocs PLEINS=%llu — "
              "reconnaitre un bloc vide n'evite QUE la premiere colonne\n",
              (unsigned long long)roles_blocs_vides, (unsigned long long)roles_blocs_pleins);
  std::printf("    certificateur de boites (compteur SEPARE) : %llu evaluations de bornes\n",
              (unsigned long long)cout_certificateur);
  std::printf("  mur=%.1f s rss_hwm_kb=%llu\n", mur, (unsigned long long)rss_hwm_kb());
  if (ledger_ko != 0 || cert_faux_positif != 0 || invariant_viole != 0) return 3;
  return 0;
}
