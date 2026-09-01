// MorseHGP3D v6 — generation des boules candidates : les trois lanes.
//
// CE QUE LA v6 CHANGE PAR RAPPORT A LA v5 (audits/NOTE_CLAUDE_CONCEPTION_V6) :
//   1. UNE SEULE descente WSPD a masques de lanes (`alive_rectangles_fused`) au
//      lieu de trois descentes par lane : les decisions de scission (separated,
//      box_w2, ordre enfant/garde) sont independantes de la lane, l'arbre de
//      rectangles est identique ; chaque lane sort du masque des que son cœur
//      atteint h_q. Grand-livre GLOBAL par lane :
//      masses emises + masses tuees == C(n,2) − Σ C(mult_u, 2).
//   2. Le RESCAN DE PROFONDEUR PAR CANDIDAT q4 n'existe plus : le SWEEP DE
//      CORDE UNIFIE mutualise la profondeur PAR SEED (certificats C1/C2 de
//      docs/MATHEMATIQUES.md § 3). Chaque completion d est une racine
//      μ_d = P(d)/B(d) de la corde du seed (th. 10.4) ; la profondeur au point
//      de racine — regle de bloc : sorties retirees, incidents a zero, entrees
//      apres — est EXACTEMENT le compte du filtre v5 (identite affine
//      signe(q4_power) = signe(P_z − μ·B_z)). Une racine STRICTEMENT hors
//      corde (2P² > J·B²) correspond exactement a un tetraedre non bien
//      centre (Jung) : la sauter est un rejet exact, jamais une perte.
//      HISTORIQUE : au checkpoint J2 (cover coefficient 3 herite), le
//      multiensemble emis etait identique a la v5 ; depuis le correctif P0
//      (coefficient 4), il en diverge legitimement — la conformite d'objet
//      juge digest_all + forets, jamais les candidats.
//      HONNETETE DE COUT (audit v6 du 31 aout) : l'incidence seed–completion
//      reste materialisee (une racine par site eligible du tape) ; ce qui
//      disparait est le rescan O(m) PAR candidat — le cout passe de
//      O(p_e × m_e) a O(m_e log m_e + p_e) par seed survivant, avec p_e les
//      completions soumises a la cascade (compteur q4_completions).
//   3. Non repris de la v5 (docs/PROVENANCE.md) : raffinement post-separation
//      (mesure +34 % de mur, defaut 0), filtre d'enveloppe (opt-in negatif),
//      LaneOverride device (aucun recu de gain G4).
//
// Le reste est la doctrine v5 inchangee : rectangle MORT si cœur >= h_q sans
// descente, TERMINAL si separe, SCINDE sinon (plus grand diametre) ;
// histogrammes h_a/h_b aux 8 coins ; ancres survivantes par
// h_coeur + h_a + h_b < h_q (disjonction, fail-open) ; q2 toujours emise ;
// q3 seeds aigus + filtre de profondeur a la generation (etage flottant
// certifie, repli affine exact) ; q4 W_4 exact + secteurs + grille (10.5) +
// cœur de Jung + morceaux de corde (10.4) en passe 1. Toutes les decisions
// entieres ; sortie multiensemble INDEPENDANTE du decoupage parallele
// (brouillons par ouvrier, fusion en ordre d'ouvrier, tri stable + RLE
// canonisent). Completude : une boule pertinente pour K <= K_max a
// |I_B| <= K_max + 1 − q et ses temoins de fuseau sous h_q : aucun filtre ne
// perd un plateau pertinent — la porte de conformite v5 le grave.
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <vector>

#include "../lanes/cell_grid.hpp"
#include "../core/caps.hpp"
#include "../lanes/chord_kill.hpp"
#include "../lanes/edge_cover.hpp"
#include "../lanes/q2.hpp"
#include "../lanes/q3.hpp"
#include "../lanes/q4.hpp"
#include "../lanes/sector_kill.hpp"
#include "../parallel/pool.hpp"
#include "../spindle/witness_count.hpp"
#include "../wspd/wavefront.hpp"
#include "candidates.hpp"
#include "float_filter.hpp"

namespace mhgp6 {

// Compteurs de la generation (jamais une autorite). Voir docs/GRAND_LIVRE.md.
struct GenerateStats {
  // Refus de capacite de la generation (kCapRefus*, caps.hpp) — pose par le
  // NIVEAU APPELANT (jamais par les shards, jamais fusionne par add_from) ;
  // run_pipeline le mappe vers resource_exhausted.
  u64 cap_refus = kCapRefusNone;
  // Pics OBSERVES des files du front fusionne (top-level, jamais fusionnes
  // par add_from) — la fixture du moment de garde les compare au plafond.
  u64 wave_peak_tasks = 0, alive_peak_rects = 0;
  // Emission exacte observee au REFUS (top-level). DEUX semantiques selon le
  // code (§ 5.9) : au refus de cap BRUT (kCapRefusRawCandidates), pour un cap
  // H et T ouvriers la porte verifie H < emitted_at_refus <= H + 4096 x T —
  // borne PROUVEE : chaque ouvrier publie au plus toutes les 4096 emissions
  // (les relectures du drapeau toutes les 64 sont INCLUSES dans ce bloc) et
  // s'arrete a l'observation, l'arret etant remonte a la boucle d'ancres. Au
  // refus BUDGETAIRE 2E (kCapRefusFusionBudget), c'est E : la somme exacte
  // des shards au moment du refus pre-fusion (aucun overshoot possible).
  u64 emitted_at_refus = 0;
  // Front fusionne : visites de paires de nœuds (une visite sert les lanes du
  // masque), rectangles vivants par lane a l'emission, grand-livre GLOBAL des
  // masses de paires (PointId) par lane : emitted + killed == expected.
  u64 rect_visited_fused = 0;
  u64 rect_alive[3] = {0, 0, 0};
  u128 ledger_emitted_mass[3] = {0, 0, 0};
  u128 ledger_killed_mass[3] = {0, 0, 0};
  // V_wspd — deux composantes DECLAREES (population : appels de
  // `count_universal_witnesses` pendant la descente fusionnee, comptages
  // initial et terminal compris) : nœuds d'index visites, et evaluations de
  // COUPLES de coins (`corner_evals` de FusedCounts — ce n'est pas un nombre
  // d'« appels temoins », c'est la masse d'evaluations geometriques payee).
  u64 wspd_witness_nodes = 0;
  u64 wspd_corner_evals = 0;
  // MUTANT wspd-drop-rect (test-only) : UNE omission par descente, appliquee
  // apres la fusion ordonnee ; la masse emise du rectangle omis est retiree
  // du grand-livre (reconstruit depuis les rectangles reellement remis), donc
  // emis + tues + omis == attendu et la cloture de production echoue.
  u64 mutant_dropped_rects = 0;
  u128 mutant_dropped_mass[3] = {0, 0, 0};
  u64 workers_wspd = 0;
  u64 workers_rects = 0;
  double t_wspd_ms = 0;
  double t_rects_ms = 0;
  // Cascade histogramme — trois classes DISJOINTES par lane :
  // anchors = rows + thresh + survivors.
  u64 anchors[3] = {0, 0, 0};
  u64 anchors_killed_hist[3] = {0, 0, 0};
  u64 hist_killed_rows[3] = {0, 0, 0};
  u64 hist_killed_thresh[3] = {0, 0, 0};
  u64 hist_survivors[3] = {0, 0, 0};
  u64 p_factor[3] = {0, 0, 0};  // P_factor : evaluations d'auto-produits des histogrammes (|A|²+|B|² par rectangle)
  u64 h_rect[3] = {0, 0, 0};    // H_rect : somme des points de handles par rectangle vivant (une fois par rectangle)
  // M_anchor — population COMMUNE q3/q4 (audit du 31 aout) : somme des
  // tailles de cover a l'ENTREE du corps par ancre, pour chaque ancre entree
  // (apres le prétest par requete, avant tout tueur W3/W4/secteurs/grille).
  // Identite fermante : la population est `anchor_entries[q]`.
  u64 m_anchor[3] = {0, 0, 0};
  // Entrees du corps par ancre (population de M_anchor et du vecteur
  // `ancres` de la sonde) : hist_survivors − tues du prétest par requete
  // − paires degenerees D2 == 0 (inatteignables sur positions uniques).
  u64 anchor_entries[3] = {0, 0, 0};
  // H_scan : nœuds d'index visites par `anchor_cover_from_handles` (somme de
  // `AnchorScratch::visits` sur les ancres entrees ; population identique a
  // M_anchor).
  u64 h_scan[3] = {0, 0, 0};
  // SONDE DE QUEUE E6 (audit du 31 aout : « les deux depassements viennent
  // de la seule graine 5 ») : distribution des ancres q4 par octave de
  // taille de cover (octave o = floor(log2(taille)), 0..15) et attribution
  // de W_sweep1 (evaluations eligibles) a l'octave de l'ancre porteuse.
  // Identites fermantes : Σ ancres == anchor_entries[2] ; Σ w1 ==
  // q4_core_site_tests ; Σ seeds == seeds[1] ; et par octave o,
  // seeds[o] == cellules[o] + coeur[o] + corde[o] + passe2[o] (les quatre
  // issues d'un seed q4 : tue par cellule, par cœur/J, par corde, ou passe 2).
  u64 q4_anchors_by_octave[16] = {};
  u64 q4_w1_by_octave[16] = {};
  u64 q4_seeds_by_octave[16] = {};
  u64 q4_seedcells_by_octave[16] = {};
  u64 q4_seedcore_by_octave[16] = {};
  u64 q4_seedchord_by_octave[16] = {};
  u64 q4_seedpass2_by_octave[16] = {};
  // SONDE E6 opt-in (--sonde-e6, lecture seule) : pour chaque seed q4 tuee
  // par CŒUR sur une ancre a grille construite, minimum des temoins des
  // cellules de corde consultees — la contrainte qui a empeche le kill par
  // cellules. Godets : [0] min == 0 ; [1] 0 < min < h/2 ; [2] h/2 <= min
  // < h-1 ; [3] min == h-1 ; [4] boite hors domaine (min illisible).
  // Si les godets [2]+[3] dominent, un RAFFINEMENT de grille (G > 8, ou
  // hierarchique) convertit ces scans de cœur en kills de cellules ; si [0]
  // domine, les temoins ne sont pas communs aux cellules et il faut des
  // structures PAR SEED (moteur plan / contrat 2). e6_sans_grille : seeds
  // tuees par cœur sans grille construite (la politique les a exclues).
  u64 e6_coeur_cellules[5] = {};
  u64 e6_sans_grille = 0;
  // Ventilation de e6_sans_grille par raison d'exclusion de la politique :
  // [1] cover < min_sites, [2] ratio seeds/cover, [3] near_m >= h,
  // [4] construction refusee/echouee ([0] inutilise).
  u64 e6_sans_grille_raison[5] = {};
  u64 e6_sondes = 0;
  u64 e6_grids16_built = 0;   // grilles raffinees G=16 construites (E3/G16)
  u64 e3_g8_heavy_built = 0;  // bras g8_lourdes : grilles G8 forcees sur ancres lourdes
  // Cout de la POLITIQUE (audit : le scan nacute/near_m saute par les bras
  // forces doit etre visible) et de la CONSULTATION des cellules.
  u64 policy_scan_sites = 0;
  u64 policy_scan_skipped_sites = 0;
  u64 cells_consulted_g8 = 0;
  u64 cells_consulted_g16 = 0;
  // Tueurs d'ancre.
  u64 anchors_killed_w3 = 0;
  u64 anchors_killed_w4 = 0;
  u64 anchors_killed_sectors[3] = {0, 0, 0};
  u64 anchors_killed_cells[3] = {0, 0, 0};
  u64 seeds_killed_cells[3] = {0, 0, 0};
  u64 grids_attempted[3] = {0, 0, 0};
  u64 grids_built[3] = {0, 0, 0};
  u64 grids_all_dead[3] = {0, 0, 0};
  // Seeds et sweep q4.
  u64 seeds[2] = {0, 0};       // q3, q4
  u64 seeds_killed_core = 0;   // q4 : cœur de seed (Jung)
  u64 seeds_killed_chord = 0;  // q4 : morceaux de corde (passe 1)
  u64 invariant_jneg = 0;      // J < 0 : inatteignable par theoreme (refus en invariant)
  u64 q4_core_site_tests = 0;  // W_sweep1 : EVALUATIONS ELIGIBLES du scan cœur+corde (apres le saut des trois indices du seed)
  u64 q4_core_iters = 0;       // iterations COMPLETES de la boucle de passe 1 (chaque entree de cover parcourue)
  u64 q4_pass2_iters = 0;      // iterations COMPLETES de la boucle de passe 2
  u64 q3_depth_site_tests = 0; // masse du filtre de profondeur q3 (sites testes)
  u64 sweep_pass2_seeds = 0;   // seeds q4 survivants entres en passe 2
  u64 sweep_pass2_site_tests = 0;  // W_sweep2 : sites rescannes par la passe 2 (P, B par site)
  u64 sweep_roots_onchord = 0;   // racines triees (observable du grand-livre)
  u64 sweep_root_groups = 0;     // blocs de racines egales traites (regle de bloc)
  u64 sweep_root_comparisons = 0;  // comparaisons exactes payees par le tri des racines
  u64 sweep_roots_offchord = 0;  // racines strictement hors corde (rejet exact, jamais enumerees)
  u64 sweep_const_interior = 0;  // sites a contribution constante interieure (c0)
  // Completions q4 (cascade au point de racine).
  u64 q4_completions = 0, q4_rej_lens = 0, q4_rej_owner = 0, q4_rej_once = 0, q4_rej_i64 = 0,
      q4_rej_face_power = 0, q4_rej_det = 0, q4_rej_center = 0;
  // Emission et profondeur a la generation.
  u64 candidates[3] = {0, 0, 0};
  u64 depth_killed[3] = {0, 0, 0};
  // Certification flottante.
  u64 float_cert_neg = 0, float_cert_pos = 0, float_fallback = 0;
  u64 q3_cert[3] = {0, 0, 0};
  u64 q4_cert[6] = {0, 0, 0, 0, 0, 0};
  u64 jung_cert_kill = 0, jung_cert_skip = 0, jung_fallback = 0;
  void add_from(const GenerateStats& o) {
    rect_visited_fused += o.rect_visited_fused;
    wspd_witness_nodes += o.wspd_witness_nodes;
    wspd_corner_evals += o.wspd_corner_evals;
    for (int i = 0; i < 3; ++i) {
      rect_alive[i] += o.rect_alive[i];
      ledger_emitted_mass[i] += o.ledger_emitted_mass[i];
      ledger_killed_mass[i] += o.ledger_killed_mass[i];
      anchors[i] += o.anchors[i];
      anchors_killed_hist[i] += o.anchors_killed_hist[i];
      hist_killed_rows[i] += o.hist_killed_rows[i];
      hist_killed_thresh[i] += o.hist_killed_thresh[i];
      hist_survivors[i] += o.hist_survivors[i];
      p_factor[i] += o.p_factor[i];
      h_rect[i] += o.h_rect[i];
      m_anchor[i] += o.m_anchor[i];
      anchor_entries[i] += o.anchor_entries[i];
      h_scan[i] += o.h_scan[i];
      mutant_dropped_mass[i] += o.mutant_dropped_mass[i];
      anchors_killed_sectors[i] += o.anchors_killed_sectors[i];
      anchors_killed_cells[i] += o.anchors_killed_cells[i];
      seeds_killed_cells[i] += o.seeds_killed_cells[i];
      grids_attempted[i] += o.grids_attempted[i];
      grids_built[i] += o.grids_built[i];
      grids_all_dead[i] += o.grids_all_dead[i];
      candidates[i] += o.candidates[i];
      depth_killed[i] += o.depth_killed[i];
      q3_cert[i] += o.q3_cert[i];
    }
    anchors_killed_w3 += o.anchors_killed_w3;
    anchors_killed_w4 += o.anchors_killed_w4;
    seeds[0] += o.seeds[0];
    seeds[1] += o.seeds[1];
    seeds_killed_core += o.seeds_killed_core;
    seeds_killed_chord += o.seeds_killed_chord;
    invariant_jneg += o.invariant_jneg;
    q4_core_site_tests += o.q4_core_site_tests;
    q4_core_iters += o.q4_core_iters;
    q4_pass2_iters += o.q4_pass2_iters;
    q3_depth_site_tests += o.q3_depth_site_tests;
    sweep_pass2_seeds += o.sweep_pass2_seeds;
    sweep_pass2_site_tests += o.sweep_pass2_site_tests;
    sweep_roots_onchord += o.sweep_roots_onchord;
    sweep_root_groups += o.sweep_root_groups;
    sweep_root_comparisons += o.sweep_root_comparisons;
    sweep_roots_offchord += o.sweep_roots_offchord;
    sweep_const_interior += o.sweep_const_interior;
    for (int i = 0; i < 16; ++i) {
      q4_anchors_by_octave[i] += o.q4_anchors_by_octave[i];
      q4_w1_by_octave[i] += o.q4_w1_by_octave[i];
      q4_seeds_by_octave[i] += o.q4_seeds_by_octave[i];
      q4_seedcells_by_octave[i] += o.q4_seedcells_by_octave[i];
      q4_seedcore_by_octave[i] += o.q4_seedcore_by_octave[i];
      q4_seedchord_by_octave[i] += o.q4_seedchord_by_octave[i];
      q4_seedpass2_by_octave[i] += o.q4_seedpass2_by_octave[i];
    }
    for (int i = 0; i < 5; ++i) {
      e6_coeur_cellules[i] += o.e6_coeur_cellules[i];
      e6_sans_grille_raison[i] += o.e6_sans_grille_raison[i];
    }
    e6_sans_grille += o.e6_sans_grille;
    e6_sondes += o.e6_sondes;
    e6_grids16_built += o.e6_grids16_built;
    e3_g8_heavy_built += o.e3_g8_heavy_built;
    policy_scan_sites += o.policy_scan_sites;
    policy_scan_skipped_sites += o.policy_scan_skipped_sites;
    cells_consulted_g8 += o.cells_consulted_g8;
    cells_consulted_g16 += o.cells_consulted_g16;
    mutant_dropped_rects += o.mutant_dropped_rects;
    q4_completions += o.q4_completions;
    q4_rej_lens += o.q4_rej_lens;
    q4_rej_owner += o.q4_rej_owner;
    q4_rej_once += o.q4_rej_once;
    q4_rej_i64 += o.q4_rej_i64;
    q4_rej_face_power += o.q4_rej_face_power;
    q4_rej_det += o.q4_rej_det;
    q4_rej_center += o.q4_rej_center;
    float_cert_neg += o.float_cert_neg;
    float_cert_pos += o.float_cert_pos;
    float_fallback += o.float_fallback;
    for (int i = 0; i < 6; ++i) q4_cert[i] += o.q4_cert[i];
    jung_cert_kill += o.jung_cert_kill;
    jung_cert_skip += o.jung_cert_skip;
    jung_fallback += o.jung_fallback;
  }
};

// Rectangle terminal vivant du front fusionne : masque des lanes encore
// ouvertes et minorant de cœur (autorite de coins) par lane du masque.
struct MultiAliveRect {
  WspdRect r;
  u8 mask = 0;
  u64 core[3] = {0, 0, 0};
};

namespace generate_detail {

inline double ms_since(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

}  // namespace generate_detail

// DESCENTE WSPD FUSIONNEE. Une seule descente pour toutes les lanes du masque
// initial : par paire de nœuds, `count_universal_witnesses` (masques, ecretage
// a h_q) ; une lane dont le cœur atteint h_q sort du masque et sa masse de
// paires est versee au grand-livre `killed` ; le rectangle n'est scinde que si
// le masque reste non vide ; a un terminal, un second comptage AVEC autorite
// de coins fournit core[3] et peut encore fermer des lanes. Les decisions de
// scission (separated sur boites serrees, plus grand diametre, ordre
// enfant/garde) sont INDEPENDANTES de la lane : l'arbre de rectangles est
// identique a celui de descentes separees — la porte `mhgp6_fused_descent`
// compare a des descentes a masque singleton (meme code, masque reduit).
// Parallele par tranches ORDONNEES de la vague : sortie bit-identique au
// sequentiel. Mutant `fused-mask-stuck` : une lane morte reste dans le masque
// (sur-emission, tuee par le grand-livre et la porte d'egalite).
// GARDE DE PROFIL : s < 8 refuse sans opt-in test-only greppable.
inline void alive_rectangles_fused(const CloudIndex& ix, i64 s, const u64 h_of[3], u8 initial_mask, int threads,
                                   std::vector<MultiAliveRect>* out, GenerateStats* st,
                                   bool allow_subprofile_separation = false,
                                   u64 wave_cap = kMaxWaveTasks, u64 alive_cap = kMaxAliveRects) {
  out->clear();
  if (s < kSeparationProfileMin && !allow_subprofile_separation)
    throw std::invalid_argument("alive_rectangles_fused : separation s < 8 sans opt-in test-only");
  if (ix.nodes.empty()) return;
  const bool mask_stuck = MHGP6_MUTANT("fused-mask-stuck");
  // MUTANTS de la descente elle-meme (porte d'ownership --wspd-ownership) :
  // cap = un rectangle de petite masse est declare terminal SANS etre separe
  // (le pavage quadratique v3) — tue par l'assertion de separation ; split =
  // scission du facteur de PLUS PETIT diametre — l'arbre change, tue par la
  // fixture gravee du nombre de rectangles.
  const bool mut_cap = MHGP6_MUTANT("wspd-cap-terminal");
  const bool mut_split_flip = MHGP6_MUTANT("wspd-split-heaviest");
  struct Task {
    WspdRect r;
    u8 mask;
  };
  const auto pair_mass = [&](const WspdRect& r) -> u128 {
    return (u128)ix.node_weight(r.a) * ix.node_weight(r.b);
  };
  std::vector<Task> wave, next;
  // La VAGUE INITIALE (m-1 taches, une par nœud interne) est de la meme
  // classe d'allocation que les suivantes : gardee avant l'insertion dans le
  // vecteur GLOBAL elle aussi
  // (le mutant caps-late-wave-check retablit le comportement tardif).
  if (!MHGP6_MUTANT("caps-late-wave-check") && (u64)ix.nodes.size() > wave_cap) {
    st->cap_refus = kCapRefusWaveTasks;
    return;
  }
  wave.reserve(ix.nodes.size());
  for (const RadixNode& n : ix.nodes) wave.push_back(Task{WspdRect{n.left, n.right}, initial_mask});
  std::vector<std::vector<MultiAliveRect>> lout;
  std::vector<std::vector<Task>> lnext;
  std::vector<GenerateStats> lst;
  while (!wave.empty()) {
    const size_t nw = wave.size();
    const int t_eff = nw < 256 ? 1 : threads;  // les premieres vagues sont minuscules
    const size_t T = planned_workers(nw, t_eff);
    const size_t chunk = std::max<size_t>(1, (nw + 8 * T - 1) / (8 * T));
    const size_t nchunks = (nw + chunk - 1) / chunk;
    lout.assign(nchunks, {});
    lnext.assign(nchunks, {});
    lst.assign(nchunks, GenerateStats{});
    const size_t created = parallel_items(nchunks, (int)T, [&](size_t c, size_t) {
      for (size_t i = c * chunk; i < std::min(nw, c * chunk + chunk); ++i) {
        const Task& t = wave[i];
        ++lst[c].rect_visited_fused;
        const FusedCounts fc = count_universal_witnesses(ix, t.r.a, t.r.b, h_of, t.mask, false);
        lst[c].wspd_witness_nodes += fc.nodes_visited;
        lst[c].wspd_corner_evals += fc.corner_evals;
        u8 m = t.mask;
        for (int q = 0; q < 3; ++q) {
          if (!(m & (1u << q))) continue;
          if (fc.c[q] >= h_of[q] && !mask_stuck) {
            m = (u8)(m & ~(1u << q));  // lane MORTE sans descente
            lst[c].ledger_killed_mass[q] += pair_mass(t.r);
          }
        }
        if (m == 0) continue;  // rectangle mort pour toutes les lanes restantes
        const AxisBox va = ix.box_of(t.r.a), vb = ix.box_of(t.r.b);
        if (wspd_detail::separated(va, vb, s, 1) || (mut_cap && pair_mass(t.r) <= 9)) {
          // TERMINAL : recomptage avec autorite de coins ; il peut fermer des
          // lanes de plus (les masses correspondantes vont au grand-livre).
          const FusedCounts ff = count_universal_witnesses(ix, t.r.a, t.r.b, h_of, m, true);
          lst[c].wspd_witness_nodes += ff.nodes_visited;
          lst[c].wspd_corner_evals += ff.corner_evals;
          MultiAliveRect ar;
          ar.r = t.r;
          for (int q = 0; q < 3; ++q) {
            if (!(m & (1u << q))) continue;
            if (ff.c[q] >= h_of[q] && !mask_stuck) {
              lst[c].ledger_killed_mass[q] += pair_mass(t.r);
              continue;
            }
            ar.mask = (u8)(ar.mask | (1u << q));
            ar.core[q] = ff.c[q];
            lst[c].ledger_emitted_mass[q] += pair_mass(t.r);
            ++lst[c].rect_alive[q];
          }
          if (ar.mask != 0) lout[c].push_back(ar);
          continue;
        }
        // SCISSION du facteur de plus grand diametre (jamais une feuille).
        const i64 w2a = wspd_detail::box_w2(va), w2b = wspd_detail::box_w2(vb);
        const bool split_a = (t.r.a >= 0) && (t.r.b < 0 || (mut_split_flip ? w2a < w2b : w2a >= w2b));
        const NodeRef keep = split_a ? t.r.b : t.r.a;
        const RadixNode& n = ix.nodes[(size_t)(split_a ? t.r.a : t.r.b)];
        lnext[c].push_back(Task{split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left}, m});
        lnext[c].push_back(Task{split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right}, m});
      }
    });
    st->workers_wspd = std::max(st->workers_wspd, (u64)created);
    // PLAFONDS DE VAGUE (caps.hpp) : tailles PROSPECTIVES verifiees AVANT
    // toute insertion dans next/out (note auditeur du 1er septembre). Les
    // shards lnext/lout, eux, sont BORNES PAR CONSTRUCTION : chaque tache de
    // la vague courante produit au plus deux taches et un terminal, donc
    // Σ|lnext| <= 2 x |wave| <= 2 x wave_cap et Σ|lout| <= |wave| — la vague
    // courante ayant elle-meme passe le controle precedent (amorcage
    // compris). Le mutant caps-late-wave-check retablit le controle tardif —
    // la fixture l'observe par les pics enregistres.
    const bool mut_late = MHGP6_MUTANT("caps-late-wave-check");
    u64 next_prospective = 0, alive_prospective = (u64)out->size();
    for (size_t c = 0; c < nchunks; ++c) {
      next_prospective += (u64)lnext[c].size();
      alive_prospective += (u64)lout[c].size();
    }
    if (!mut_late && (next_prospective > wave_cap || alive_prospective > alive_cap)) {
      st->cap_refus = next_prospective > wave_cap ? kCapRefusWaveTasks : kCapRefusAliveRects;
      st->wave_peak_tasks = std::max(st->wave_peak_tasks, (u64)wave.size());
      st->alive_peak_rects = std::max(st->alive_peak_rects, (u64)out->size());
      return;
    }
    next.clear();
    for (size_t c = 0; c < nchunks; ++c) {
      out->insert(out->end(), lout[c].begin(), lout[c].end());
      next.insert(next.end(), lnext[c].begin(), lnext[c].end());
      st->add_from(lst[c]);
      // Liberation IMMEDIATE des tranches fusionnees (balayage du 1er
      // septembre : la coexistence wave + lnext + next triplait le pic).
      std::vector<MultiAliveRect>().swap(lout[c]);
      std::vector<Task>().swap(lnext[c]);
    }
    st->wave_peak_tasks = std::max(st->wave_peak_tasks, (u64)next.size());
    st->alive_peak_rects = std::max(st->alive_peak_rects, (u64)out->size());
    if (mut_late && ((u64)next.size() > wave_cap || (u64)out->size() > alive_cap)) {
      st->cap_refus = (u64)next.size() > wave_cap ? kCapRefusWaveTasks : kCapRefusAliveRects;
      return;
    }
    wave.swap(next);
  }
  // MUTANT wspd-drop-rect (audit du 31 aout, option 1) : UNE omission par
  // DESCENTE, appliquee apres la fusion ordonnee — le premier rectangle
  // vivant de la sortie est retire, et sa masse emise est SOUSTRAITE du
  // grand-livre (reconstruction depuis les rectangles reellement remis) :
  // emis + tues == attendu − masse_omise, la cloture de production echoue et
  // `mutant_dropped_rects == 1` grave litteralement le delta −1. Meme
  // semantique une-par-descente que le site homonyme de wspd/wavefront.hpp.
  if (MHGP6_MUTANT("wspd-drop-rect") && !out->empty()) {
    const MultiAliveRect dropped = out->front();
    out->erase(out->begin());
    ++st->mutant_dropped_rects;
    for (int q = 0; q < 3; ++q) {
      if (!(dropped.mask & (1u << q))) continue;
      st->ledger_emitted_mass[q] -= pair_mass(dropped.r);
      st->mutant_dropped_mass[q] += pair_mass(dropped.r);
      --st->rect_alive[q];
    }
  }
}

// Experimentation E3/G16 — BRAS SEPARES (attribution economique, quatrieme
// tour d'audit) ; kOff = production. Voir anchor_grid_stage.
enum class E3G16Mode : u8 { kOff = 0, kG8Lourdes, kG16Politique, kG16NearM, kG16Ratio, kG16Leve };

namespace generate_detail {

// Histogrammes h_a/h_b (autorite 8 coins, exacte) d'un rectangle, pour une
// lane. O(|A|² + |B|²) par rectangle : assume, |A||B| ~ 2 a s >= 8 (route S) ;
// la route M saturee (docs/MATHEMATIQUES.md § C7) est un chantier E2 distinct.
inline void corner_histograms(const CloudIndex& ix, Lane lane, const WspdRect& r, std::vector<u64>* ha,
                              std::vector<u64>* hb) {
  const NodeRange ra = ix.range_of(r.a), rb = ix.range_of(r.b);
  const AxisBox boxA = ix.box_of(r.a), boxB = ix.box_of(r.b);
  const int na = ra.last - ra.first + 1, nb = rb.last - rb.first + 1;
  ha->assign((size_t)na, 0);
  hb->assign((size_t)nb, 0);
  for (int ia = 0; ia < na; ++ia)
    for (int iz = 0; iz < na; ++iz)
      if (iz != ia &&
          universal_over_corners(lane, ix.upos[(size_t)(ra.first + ia)], boxB, ix.upos[(size_t)(ra.first + iz)]))
        ++(*ha)[(size_t)ia];
  for (int ib = 0; ib < nb; ++ib)
    for (int iz = 0; iz < nb; ++iz)
      if (iz != ib &&
          universal_over_corners(lane, ix.upos[(size_t)(rb.first + ib)], boxA, ix.upos[(size_t)(rb.first + iz)]))
        ++(*hb)[(size_t)ib];
}

// Brouillon par ouvrier : histogrammes, handles, cover, lentille, sites
// affines de l'ancre (u = 2z−a−b, q = |u|²−D², entiers < 2^36 exacts en
// binaire64), grille de cellules, et tampons du sweep de corde.
// Throttle d'emission PARTAGE (plafond declare, caps.hpp) : chaque ouvrier
// publie son compte au plus toutes les 4096 emissions puis observe le
// drapeau, et chaque site d'emission ABANDONNE son ancre des l'observation —
// overshoot borne par 4096 x T (l'arret est remonte a la boucle d'ancres —
// aucune ancre entiere apres l'observation ; le contenu sous le plafond est
// inchange, et tout depassement conclut par un refus qui jette les shards).
struct EmitThrottle {
  std::atomic<u64>* total = nullptr;
  std::atomic<bool>* stop = nullptr;
  u64 cap = kMaxRawCandidates;
  u64 pending = 0;
  bool stopped = false;
  inline void tick() {
    ++pending;
    if ((pending & 63ull) == 0 && stop != nullptr)
      stopped = stopped || stop->load(std::memory_order_relaxed);
    if ((pending & 0xFFFull) != 0) return;
    sync();
  }
  inline void sync() {
    if (total == nullptr) return;
    const u64 t = total->fetch_add(pending, std::memory_order_relaxed) + pending;
    pending = 0;
    if (t > cap) stop->store(true, std::memory_order_relaxed);
    stopped = stop->load(std::memory_order_relaxed);
  }
};

struct AnchorScratch {
  EmitThrottle emit;
  std::vector<u64> ha, hb;
  std::vector<NodeRef> handles;
  std::vector<CoverPoint> cover, cover_tmp, lens, query;
  u64 handle_points = 0;
  u64 cover_nodes = 0, visits = 0;
  std::vector<i64> su0, su1, su2, sq;
  double qmax_d = 1.0, umax_d = 1.0;
  bool affine_filled = false;
  CellGrid grid;
  // Etage E6 (opt-in --e6-grille) : grille RAFFINEE G = 16 des ancres q4
  // lourdes — meme certificat 10.5, cellules quatre fois plus petites.
  CellGridFine grid16;
  size_t cell_min_sites = kCellGridMinSites;
  // Sonde E6 : pourquoi la grille n'a pas ete construite sur cette ancre
  // (0 = construite, 1 = cover < min_sites, 2 = ratio seeds/cover, 3 =
  // near_m >= h, 4 = construction refusee/echouee). Diagnostique seulement.
  u8 grid_skip_reason = 0;
  // Tampons de la passe 2 du sweep (par ouvrier, reutilises par seed).
  struct ChordRoot {
    i128 num;  // μ = num/den, den > 0 (normalise par le signe de B)
    i64 den;
    i32 u;      // indice upos du site
    bool entry;  // B > 0 : le site devient interieur pour μ > μ_z
  };
  std::vector<ChordRoot> roots;
  const std::vector<CoverPoint>& scan_sites() const { return cover; }
  void fill_affine_sites(const CloudIndex& ix, const P3& pa, const P3& pb, i64 D2) {
    const size_t nc = cover.size();
    su0.resize(nc);
    su1.resize(nc);
    su2.resize(nc);
    sq.resize(nc);
    i64 qmax = 1, umax = 1;
    const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
    for (size_t i = 0; i < nc; ++i) {
      const P3& pz = ix.upos[(size_t)cover[i].u];
      const i64 u0 = 2 * pz.x - sx, u1 = 2 * pz.y - sy, u2 = 2 * pz.z - sz;
      const i64 qz = u0 * u0 + u1 * u1 + u2 * u2 - D2;
      su0[i] = u0;
      su1[i] = u1;
      su2[i] = u2;
      sq[i] = qz;
      qmax = std::max(qmax, qz < 0 ? -qz : qz);
      umax = std::max({umax, u0 < 0 ? -u0 : u0, u1 < 0 ? -u1 : u1, u2 < 0 ? -u2 : u2});
    }
    qmax_d = (double)qmax;
    umax_d = (double)umax;
    affine_filled = true;
  }
};

// GRILLE DE CELLULES (cell_grid.hpp) — centre v3 = N/(2G), N = W − G·d (q3) ;
// corde (N ± μ̂·n)/(2G), μ̂ = isqrt(J/2) + 1 (q4, theoreme 10.4).
template <typename Grid>
inline void seed_center_coords(const Grid& g, const Q3Form& f3, const i64 d[3], i128* pu, i128* pv, i128* den) {
  *pu = 0;
  *pv = 0;
  for (int k = 0; k < 3; ++k) {
    const i128 Nk = f3.w[k] - f3.g * (i128)d[k];
    *pu += Nk * g.u[k];
    *pv += Nk * g.v[k];
  }
  *den = 2 * f3.g;
}
template <typename Grid>
inline bool seed_center_cell_dead(const Grid& g, const Q3Form& f3, const i64 d[3]) {
  i128 pu, pv, den;
  seed_center_coords(g, f3, d, &pu, &pv, &den);
  return g.point_dead(pu, pv, den);
}
template <typename Grid>
inline bool seed_chord_coords(const Grid& g, const Q3Form& f3, const i64 d[3], const P3& nrm, i64 D2, i64 l_ax,
                              i64 l_bx, i128* pu0, i128* pv0, i128* pu1, i128* pv1, i128* den) {
  const i128 J = (i128)D2 * (3 * f3.g - 2 * (i128)l_ax * l_bx);
  if (J < 0) return false;
  const i128 mu_hat = isqrt128_floor(J / 2) + 1;
  const i64 nn[3] = {nrm.x, nrm.y, nrm.z};
  i128 pu = 0, pv = 0, qu = 0, qv = 0;
  for (int k = 0; k < 3; ++k) {
    const i128 Nk = f3.w[k] - f3.g * (i128)d[k];
    pu += Nk * g.u[k];
    pv += Nk * g.v[k];
    qu += (i128)nn[k] * g.u[k];
    qv += (i128)nn[k] * g.v[k];
  }
  *pu0 = pu + mu_hat * qu;
  *pv0 = pv + mu_hat * qv;
  *pu1 = pu - mu_hat * qu;
  *pv1 = pv - mu_hat * qv;
  *den = 2 * f3.g;
  return true;
}
template <typename Grid>
inline bool seed_chord_cell_dead(const Grid& g, const Q3Form& f3, const i64 d[3], const P3& nrm, i64 D2, i64 l_ax,
                                 i64 l_bx) {
  i128 pu0, pv0, pu1, pv1, den;
  if (!seed_chord_coords(g, f3, d, nrm, D2, l_ax, l_bx, &pu0, &pv0, &pu1, &pv1, &den)) return false;
  return g.segment_dead(pu0, pv0, pu1, pv1, den);
}

// Seuil des ancres LOURDES de l'etage E6 (sonde du 31 aout : la queue des
// pentes superquadratiques vit aux octaves >= 10, cover >= 1024).
inline constexpr size_t kE6HeavyCover = (size_t)1 << 10;


// ETAGE GRILLE d'une ancre : politique + construction + compteurs (UNE
// definition). Rend true si l'ancre est MORTE (toutes cellules mortes).
// Experimentation E3/G16 par BRAS (attribution economique, audit du
// quatrieme tour) sur les ancres q4 LOURDES (cover >= 2^10) :
//   g8_lourdes    : G8 force (les deux vetos leves), aucune G16 ;
//   g16_politique : G16 remplace G8 la ou la POLITIQUE HISTORIQUE construit ;
//   g16_nearm     : G16 forcee en levant near_m SEUL (ratio maintenu) ;
//   g16_ratio     : G16 forcee en levant le ratio SEUL (near_m maintenu) ;
//   g16_leve      : G16 forcee, les deux vetos leves.
// REPLI : un echec de construction du bras retombe sur la voie historique
// (fail-open borne, jamais un trou). L'objet est inchange dans tous les
// bras (certificats du theoreme 10.5, quelle que soit la resolution).
inline bool anchor_grid_stage(const CloudIndex& ix, AnchorScratch& sc, i32 ua, i32 ub, const P3& pa, const P3& pb,
                              i64 D2, Lane lane, u64 h, bool float_on, E3G16Mode e3, GenerateStats* ls) {
  const int li = lane == Lane::kQ3 ? 1 : 2;
  sc.grid.built = false;
  sc.grid16.built = false;
  sc.grid_skip_reason = 0;
  using E3 = E3G16Mode;
  const bool heavy = lane == Lane::kQ4 && sc.cover.size() >= kE6HeavyCover;
  const bool forced_arm = heavy && (e3 == E3::kG8Lourdes || e3 == E3::kG16Leve);
  // Bras a veto partiel : la politique est evaluee mais UN veto est leve.
  const bool partial_arm = heavy && (e3 == E3::kG16NearM || e3 == E3::kG16Ratio);
  if (forced_arm) {
    // Scan de politique SAUTE : son cout retire est compte (jamais confondu
    // avec le gain de resolution).
    ls->policy_scan_skipped_sites += sc.cover.size();
    ++ls->grids_attempted[li];
    const bool ok = (e3 == E3::kG8Lourdes)
                        ? sc.grid.build(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, h, float_on)
                        : sc.grid16.build(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, h, float_on);
    if (ok) {
      ++ls->grids_built[li];
      if (e3 == E3::kG8Lourdes) ++ls->e3_g8_heavy_built;
      else ++ls->e6_grids16_built;
      const bool all_dead = (e3 == E3::kG8Lourdes) ? sc.grid.all_dead : sc.grid16.all_dead;
      if (all_dead) {
        ++ls->grids_all_dead[li];
        ++ls->anchors_killed_cells[li];
        return true;
      }
      return false;
    }
    sc.grid_skip_reason = 4;
    // REPLI : la voie historique reprend ci-dessous (G8, politique entiere).
    sc.grid.built = false;
    sc.grid16.built = false;
  }
  if (sc.cell_min_sites != 0 && sc.cover.size() < sc.cell_min_sites) {
    sc.grid_skip_reason = 1;
    return false;
  }
  size_t nacute = 0, near_m = 0;
  ls->policy_scan_sites += sc.cover.size();
  for (const CoverPoint& cz : sc.cover) {
    if (cz.u == ua || cz.u == ub) continue;
    if (cell_grid_near_m(cz.dist2q, D2)) ++near_m;
    if (is_acute_seed(pa, pb, ix.upos[(size_t)cz.u], D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cz.u)))
      ++nacute;
  }
  const size_t ratio = lane == Lane::kQ3 ? kCellGridSeedsRatioQ3 : kCellGridSeedsRatioQ4;
  // Bras a veto partiel : la clause levee est neutralisee APRES le scan (le
  // cout de la politique reste paye et compte).
  const size_t eff_near_m = (partial_arm && e3 == E3::kG16NearM) ? 0 : near_m;
  const size_t eff_nacute = (partial_arm && e3 == E3::kG16Ratio) ? sc.cover.size() : nacute;
  if (!cell_grid_wanted(sc.cover.size(), eff_nacute, eff_near_m, h, sc.cell_min_sites, ratio)) {
    sc.grid_skip_reason = (sc.cell_min_sites != 0 && eff_nacute * ratio < sc.cover.size()) ? 2 : 3;
    return false;
  }
  // Resolution du bras : G16 pour g16_politique (partout ou la politique
  // construit en q4) et pour les bras partiels sur les ancres lourdes.
  const bool use_g16 = lane == Lane::kQ4 &&
                       ((e3 == E3::kG16Politique) || partial_arm || (heavy && e3 == E3::kG16Leve));
  ++ls->grids_attempted[li];
  const bool built = use_g16 ? sc.grid16.build(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, h, float_on)
                             : sc.grid.build(sc.cover, ix.upos, ua, ub, pa, pb, D2, lane == Lane::kQ3 ? 12 : 8, h,
                                             float_on);
  if (!built) {
    sc.grid_skip_reason = 4;
    if (use_g16) {
      // REPLI G8 : jamais un trou de couverture par la resolution fine.
      if (sc.grid.build(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, h, float_on)) {
        ++ls->grids_built[li];
        if (sc.grid.all_dead) {
          ++ls->grids_all_dead[li];
          ++ls->anchors_killed_cells[li];
          return true;
        }
      }
    }
    return false;
  }
  if (use_g16) ++ls->e6_grids16_built;
  ++ls->grids_built[li];
  const bool all_dead = use_g16 ? sc.grid16.all_dead : sc.grid.all_dead;
  if (!all_dead) return false;
  ++ls->grids_all_dead[li];
  ++ls->anchors_killed_cells[li];
  return true;
}

// Kernel affine d'un seed sur les sites de l'ancre : N = W − G·d, borne E.
struct AffineSeed {
  i128 N0, N1, N2;
  double Gd, Nd0, Nd1, Nd2, bound;
  i128 G;
  AffineSeed(const Q3Form& f3, const P3& pa, const P3& pb, const AnchorScratch& sc, bool float_on)
      : N0(f3.w[0] - f3.g * (i128)(pb.x - pa.x)),
        N1(f3.w[1] - f3.g * (i128)(pb.y - pa.y)),
        N2(f3.w[2] - f3.g * (i128)(pb.z - pa.z)),
        Gd((double)f3.g),
        Nd0((double)N0),
        Nd1((double)N1),
        Nd2((double)N2),
        bound(float_on ? affine_l_bound(Gd, Nd0, Nd1, Nd2, sc.qmax_d, sc.umax_d)
                       : std::numeric_limits<double>::infinity()),
        G(f3.g) {}
  double l_hat(const AnchorScratch& sc, size_t iz) const {
    return affine_l_hat(Gd, Nd0, Nd1, Nd2, (double)sc.su0[iz], (double)sc.su1[iz], (double)sc.su2[iz],
                        (double)sc.sq[iz]);
  }
  // L = 4P exact (i128, |L| < 2^105).
  i128 l_exact(const AnchorScratch& sc, size_t iz) const {
    return G * (i128)sc.sq[iz] - 2 * ((i128)sc.su0[iz] * N0 + (i128)sc.su1[iz] * N1 + (i128)sc.su2[iz] * N2);
  }
};

// CORPS PAR ANCRE de la lane q3 : tests d'ancre cumules (W_3 exact puis
// secteurs), grille de cellules, seeds aigus, filtre de profondeur a la
// generation (etage flottant certifie, repli exact i128), emission.
inline void scan_anchor_q3(const CloudIndex& ix, AnchorScratch& sc, i32 ua, i32 ub, const P3& pa, const P3& pb,
                           i64 D2, u64 h3, bool float_on, bool genfilter_nonstrict, bool pretested,
                           std::vector<BallCandidate>* lo, GenerateStats* ls, const EndpointCredit* ec) {
  if (!pretested) {
    const int k = anchor_kill_cumulated(sc.cover, ix.upos, ua, ub, pa, pb, D2, Lane::kQ3, 12, h3, true, ec);
    if (k == 1) {
      ++ls->anchors_killed_w3;
      return;
    }
    if (k == 2) {
      ++ls->anchors_killed_sectors[1];
      return;
    }
  }
  if (anchor_grid_stage(ix, sc, ua, ub, pa, pb, D2, Lane::kQ3, h3, float_on, E3G16Mode::kOff, ls)) return;
  const i64 d3[3] = {pb.x - pa.x, pb.y - pa.y, pb.z - pa.z};
  sc.affine_filled = false;
  for (const CoverPoint& cp : sc.cover) {
    if (cp.u == ua || cp.u == ub) continue;
    const P3& px = ix.upos[(size_t)cp.u];
    if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cp.u))) continue;
    ++ls->seeds[0];
    const Q3Form f3 = q3_form(pa, pb, px);
    if (sc.grid.built && seed_center_cell_dead(sc.grid, f3, d3)) {
      ++ls->seeds_killed_cells[1];
      continue;
    }
    if (!sc.affine_filled) sc.fill_affine_sites(ix, pa, pb, D2);
    const AffineSeed seed(f3, pa, pb, sc, float_on);
    u64 depth = 0;
    bool deep = false;
    for (size_t iz = 0; iz < sc.cover.size() && !deep; ++iz) {
      ++ls->q3_depth_site_tests;
      const double lh = seed.l_hat(sc, iz);
      bool interior;
      if (lh < -seed.bound) {
        ++ls->float_cert_neg;
        ++ls->q3_cert[0];
        interior = true;
      } else if (lh > seed.bound) {
        ++ls->float_cert_pos;
        ++ls->q3_cert[1];
        interior = false;
      } else {
        ++ls->float_fallback;
        ++ls->q3_cert[2];
        const i128 L = seed.l_exact(sc, iz);
        interior = L < 0 || (genfilter_nonstrict && L <= 0);
      }
      if (interior && ++depth >= h3) deep = true;
    }
    if (deep) {
      ++ls->depth_killed[1];
      continue;
    }
    lo->push_back(BallCandidate{q3_ball_key(f3), promote_level(q3_exact_level(pa, pb, px)), 3});
    sc.emit.tick();
    if (sc.emit.stopped) return;
    ++ls->candidates[1];
  }
}

// Comparateur exact de deux racines normalisees (num/den, den > 0) :
// signe de num1·den2 − num2·den1. |num| < 2^101, den < 2^55 : produits
// < 2^156, magnitude par mul_128x128_192, comparaison U192.
inline int cmp_chord_roots(const AnchorScratch::ChordRoot& r1, const AnchorScratch::ChordRoot& r2) {
  const i128 n1 = r1.num, n2 = r2.num;
  const bool neg1 = n1 < 0, neg2 = n2 < 0;
  if (neg1 != neg2) return neg1 ? -1 : 1;
  const U192 m1 = mul_128x128_192(uabs128(n1), (u128)r2.den);
  const U192 m2 = mul_128x128_192(uabs128(n2), (u128)r1.den);
  const int c = cmp_u192(m1, m2);
  return neg1 ? -c : c;
}

// CORPS PAR ANCRE de la lane q4. Passe 1 (v5 inchangee) : W_4 exact + credit,
// secteurs, grille, lentille, seeds aigus, scan partage cœur de Jung +
// morceaux de corde avec sortie anticipee. Passe 2 (LE NEUF, certificats
// C1/C2) : pour chaque seed survivant, racines μ_z = P(z)/B(z) triees ;
// chaque racine sur la corde fermee (2P² <= J·B², egalite comprise — borne de
// Jung admissible) est une completion potentielle, evaluee a sa profondeur de
// point (regle de bloc) puis par la cascade exacte O(1). Une racine
// strictement hors corde ne peut pas etre bien centree (rejet exact) mais
// reste TEMOIN constant si P < 0. Le multiensemble emis est identique a la
// boucle de completions v5 (identite affine du th. 10.4) ; l'incidence
// seed–completion est toujours payee (une racine par site), seul le rescan
// de profondeur par candidat disparait. CONTRAT DE PROFONDEUR (choix 1 de
// l'audit v6) : le verdict est depth_at(mu_d) >= h4 sur le COVER COMPLET,
// sans aucun credit ajoute (les temoins des credits figurent deja dans ce
// compte) ; la composition residuelle (AnchorCredit/ResidualTape) est le
// contrat 2, un chantier J3 distinct.
inline void process_anchor_q4(const CloudIndex& ix, AnchorScratch& sc, i32 ua, i32 ub, const P3& pa, const P3& pb,
                              i64 D2, u64 h4, bool float_on, bool seed_core_nonstrict, bool no_canonical,
                              bool pretested, bool e6_probe, E3G16Mode e3,
                              std::vector<BallCandidate>* lo, GenerateStats* ls, const EndpointCredit* ec) {
  // Sonde de queue E6 : octave de la taille du cover de l'ancre. Le vecteur
  // `ancres` compte les ENTREES de ce corps (population = anchor_entries[2],
  // creditee au site d'appel commun) ; M_anchor y est credite aussi.
  int oct = 0;
  for (size_t sz = sc.cover.size(); sz > 1 && oct < 15; sz >>= 1) ++oct;
  ++ls->q4_anchors_by_octave[oct];
  if (!pretested) {
    u64 n4 = 0, n4_out = 0;
    const bool use_ec = ec != nullptr && ec->active() && ec->base < h4;
    for (const CoverPoint& cz : sc.cover) {
      if (cz.u == ua || cz.u == ub) continue;
      if (!in_spindle(Lane::kQ4, pa, pb, ix.upos[(size_t)cz.u])) continue;
      if (++n4 >= h4) break;
      if (use_ec && !ec->in_boxes(cz.u) && ++n4_out + ec->base >= h4) {
        n4 = h4;
        break;
      }
    }
    if (n4 >= h4) {
      ++ls->anchors_killed_w4;
      return;
    }
    u64 wmin = 0;
    if (anchor_sector_kill(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, h4, &wmin, nullptr, true, ec)) {
      ++ls->anchors_killed_sectors[2];
      return;
    }
  }
  sc.affine_filled = false;
  sc.lens.clear();
  for (const CoverPoint& cz : sc.cover) {
    const P3& pz = ix.upos[(size_t)cz.u];
    if (p3_norm2(p3_sub(pz, pa)) <= D2 && p3_norm2(p3_sub(pz, pb)) <= D2) sc.lens.push_back(cz);
  }
  if (anchor_grid_stage(ix, sc, ua, ub, pa, pb, D2, Lane::kQ4, h4, float_on, e3, ls)) return;
  const i64 d4[3] = {pb.x - pa.x, pb.y - pa.y, pb.z - pa.z};
  for (const CoverPoint& cx : sc.lens) {
    if (cx.u == ua || cx.u == ub) continue;
    const P3& px = ix.upos[(size_t)cx.u];
    if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u))) continue;
    ++ls->seeds[1];
    ++ls->q4_seeds_by_octave[oct];
    const i64 l_ax = p3_norm2(p3_sub(px, pa));
    const i64 l_bx = p3_norm2(p3_sub(px, pb));
    const Q3Form f3s = q3_form(pa, pb, px);
    const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
    if ((sc.grid16.built && seed_chord_cell_dead(sc.grid16, f3s, d4, nrm, D2, l_ax, l_bx)) ||
        (sc.grid.built && seed_chord_cell_dead(sc.grid, f3s, d4, nrm, D2, l_ax, l_bx))) {
      ++ls->seeds_killed_cells[2];
      ++ls->q4_seedcells_by_octave[oct];
      continue;
    }
    // Cœur universel du seed (Jung) : J = D²(3G − 2 l_ax l_bx) >= G·D²/3 > 0
    // pour tout seed aigu — la branche J < 0 est INATTEIGNABLE par theoreme.
    const i128 Jb = (i128)D2 * (3 * f3s.g - 2 * (i128)l_ax * l_bx);
    if (Jb < 0) {
      ++ls->invariant_jneg;  // signale ; run_pipeline refuse en invariant
      ++ls->seeds_killed_core;
      ++ls->q4_seedcore_by_octave[oct];
      continue;
    }
    if (!sc.affine_filled) sc.fill_affine_sites(ix, pa, pb, D2);
    const AffineSeed seed(f3s, pa, pb, sc, float_on);
    bool dead = false;
    bool dead_by_chord = false;
    {
      const double Jd = (double)Jb;
      const double Jlo = Jd * (1.0 - kJungGuard), Jhi = Jd * (1.0 + kJungGuard);
      ChordPieces chord;
      chord.init(Jb, MHGP6_MUTANT("chord-nonstrict"));
      u64 fcount = 0;
      for (size_t iz = 0; iz < sc.cover.size(); ++iz) {
        ++ls->q4_core_iters;
        const CoverPoint& cz = sc.cover[iz];
        if (cz.u == ua || cz.u == ub || cz.u == cx.u) continue;
        ++ls->q4_core_site_tests;
        ++ls->q4_w1_by_octave[oct];
        const double lh = seed.l_hat(sc, iz);
        const P3& pz = ix.upos[(size_t)cz.u];
        const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
        // La corde voit aussi les sites certifies P > 0 (audit du 30 aout) :
        // l'enregistrement precede le saut, la mort se constate apres.
        const bool skip_pos = lh > seed.bound;
        if (!(skip_pos && MHGP6_MUTANT("chord-skip-positive")))
          chord.update(lh, seed.bound, Bz, [&]() { return seed.l_exact(sc, iz); });
        if (skip_pos) {
          ++ls->float_cert_pos;
          ++ls->q4_cert[0];
          if (!MHGP6_MUTANT("chord-dead-skip-positive") && chord.dead(h4)) {
            dead_by_chord = true;
            break;
          }
          continue;  // P > 0 certifie : jamais temoin du CŒUR (μ = 0)
        }
        if (lh < -seed.bound) {
          ++ls->float_cert_neg;
          ++ls->q4_cert[1];
          const int js = jung_interval_sign(lh, seed.bound, Jlo, Jhi, Bz);
          if (js != 0) {
            if (js > 0) {
              ++ls->jung_cert_kill;
              ++ls->q4_cert[2];
              if (++fcount >= h4) break;
            } else {
              ++ls->jung_cert_skip;
              ++ls->q4_cert[3];
            }
          } else {
            ++ls->jung_fallback;
            ++ls->q4_cert[4];
            const i128 Pz = seed.l_exact(sc, iz) / 4;
            const int c = cmp_2p2_jb2(Pz, Jb, Bz);
            if ((seed_core_nonstrict ? (c >= 0) : (c > 0)) && ++fcount >= h4) break;
          }
        } else {
          ++ls->float_fallback;
          ++ls->q4_cert[5];
          const i128 Pz = seed.l_exact(sc, iz) / 4;
          if (!(seed_core_nonstrict ? (Pz > 0) : (Pz >= 0))) {
            const int c = cmp_2p2_jb2(Pz, Jb, Bz);
            if ((seed_core_nonstrict ? (c >= 0) : (c > 0)) && ++fcount >= h4) break;
          }
        }
        if (chord.dead(h4)) {
          dead_by_chord = true;
          break;
        }
      }
      dead = fcount >= h4 || dead_by_chord;
    }
    if (dead) {
      if (dead_by_chord) {
        ++ls->seeds_killed_chord;
        ++ls->q4_seedchord_by_octave[oct];
      } else {
        ++ls->seeds_killed_core;
        ++ls->q4_seedcore_by_octave[oct];
        // SONDE E6 (opt-in, lecture seule) : la contrainte de cellules qui a
        // laisse cette seed atteindre le scan de cœur.
        if (e6_probe) {
          if (!sc.grid.built && !sc.grid16.built) {
            ++ls->e6_sans_grille;
            ++ls->e6_sans_grille_raison[sc.grid_skip_reason < 5 ? sc.grid_skip_reason : 4];
          } else {
            ++ls->e6_sondes;
            i128 pu0, pv0, pu1, pv1, den;
            long long mn = -1;
            if (sc.grid16.built) {
              if (seed_chord_coords(sc.grid16, f3s, d4, nrm, D2, l_ax, l_bx, &pu0, &pv0, &pu1, &pv1, &den))
                mn = sc.grid16.segment_min_count(pu0, pv0, pu1, pv1, den);
            } else if (seed_chord_coords(sc.grid, f3s, d4, nrm, D2, l_ax, l_bx, &pu0, &pv0, &pu1, &pv1, &den))
              mn = sc.grid.segment_min_count(pu0, pv0, pu1, pv1, den);
            if (mn < 0)
              ++ls->e6_coeur_cellules[4];
            else if (mn == 0)
              ++ls->e6_coeur_cellules[0];
            else if ((u64)mn + 1 == h4)
              ++ls->e6_coeur_cellules[3];
            else if ((u64)(2 * mn) >= h4)
              ++ls->e6_coeur_cellules[2];
            else
              ++ls->e6_coeur_cellules[1];
          }
        }
      }
      continue;
    }
    // ---- PASSE 2 : sweep de corde unifie (remplace la boucle C×D v5).
    ++ls->sweep_pass2_seeds;
    ++ls->q4_seedpass2_by_octave[oct];
    u64 c0 = 0;  // temoins constants sur toute la corde fermee
    sc.roots.clear();
    for (size_t iz = 0; iz < sc.cover.size(); ++iz) {
      ++ls->q4_pass2_iters;
      const CoverPoint& cz = sc.cover[iz];
      if (cz.u == ua || cz.u == ub || cz.u == cx.u) continue;
      ++ls->sweep_pass2_site_tests;
      const P3& pz = ix.upos[(size_t)cz.u];
      const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
      const i128 Pz = seed.l_exact(sc, iz) / 4;
      if (Bz == 0) {
        // Coplanaire au seed : jamais une completion (det = 0) ; temoin
        // constant ssi strictement interieur au centre (P < 0).
        if (Pz < 0) {
          ++c0;
          ++ls->sweep_const_interior;
        }
        continue;
      }
      const int on = cmp_2p2_jb2(Pz > 0 ? -Pz : Pz, Jb, Bz);
      if (on > 0) {
        // Racine STRICTEMENT hors corde : signe de (P − μB) constant sur la
        // corde fermee (= signe de P en μ = 0) ; et le tetraedre correspondant
        // n'est pas bien centre (Jung) — rejet exact, jamais une perte.
        ++ls->sweep_roots_offchord;
        if (Pz < 0) {
          ++c0;
          ++ls->sweep_const_interior;
        }
        continue;
      }
      // Racine sur la corde FERMEE (egalite = borne de Jung, admissible).
      AnchorScratch::ChordRoot r;
      if (Bz > 0) {
        r.num = Pz;
        r.den = Bz;
        r.entry = true;
      } else {
        r.num = -Pz;
        r.den = -Bz;
        r.entry = false;
      }
      r.u = cz.u;
      sc.roots.push_back(r);
      if (MHGP6_MUTANT("sweep-drop-exit-root") && !r.entry) sc.roots.pop_back();
    }
    ls->sweep_roots_onchord += sc.roots.size();
    u64 sort_cmps = 0;
    std::sort(sc.roots.begin(), sc.roots.end(),
              [&sort_cmps](const AnchorScratch::ChordRoot& x, const AnchorScratch::ChordRoot& y) {
                ++sort_cmps;
                const int c = cmp_chord_roots(x, y);
                if (c != 0) return c < 0;
                return x.u < y.u;  // departage canonique : determinisme du parcours
              });
    ls->sweep_root_comparisons += sort_cmps;
    u64 ent = 0;
    u64 ext_after = 0;
    for (const AnchorScratch::ChordRoot& r : sc.roots)
      if (!r.entry) ++ext_after;
    size_t i = 0;
    const u64 base_depth = c0;
    while (i < sc.roots.size()) {
      size_t j = i + 1;
      while (j < sc.roots.size() && cmp_chord_roots(sc.roots[i], sc.roots[j]) == 0) ++j;
      u64 ext_at = 0, ent_at = 0;
      for (size_t k = i; k < j; ++k) {
        if (sc.roots[k].entry)
          ++ent_at;
        else
          ++ext_at;
      }
      // REGLE DE BLOC (certificat C2) : sorties retirees, incidents a zero,
      // entrees ajoutees apres. Mutant `sweep-nonstrict-depth` : les incidents
      // (coquille) comptent comme interieurs — fausse profondeur, tuee par la
      // porte du sweep.
      ext_after -= ext_at;
      ++ls->sweep_root_groups;
      u64 depth_at = base_depth + ent + ext_after;
      if (MHGP6_MUTANT("sweep-nonstrict-depth")) depth_at += (u64)(j - i);
      for (size_t k = i; k < j; ++k) {
        const i32 uy = sc.roots[k].u;
        ++ls->q4_completions;
        const P3& py = ix.upos[(size_t)uy];
        const i64 l_ay = p3_norm2(p3_sub(py, pa));
        const i64 l_by = p3_norm2(p3_sub(py, pb));
        const i64 l_xy = p3_norm2(p3_sub(py, px));
        if (l_ay > D2 || l_by > D2 || l_xy > D2) {
          ++ls->q4_rej_lens;
          continue;
        }
        // Profondeur au point de racine : identique au filtre v5 (identite
        // affine du th. 10.4), decidee AVANT la cascade — le cout par
        // candidat est O(1), le produit candidat × scan n'existe plus.
        const bool deep = depth_at >= h4;
        if (!tetra_owned_by(D2, l_ax, l_ay, l_bx, l_by, l_xy, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u),
                            ix.point_id(uy))) {
          ++ls->q4_rej_owner;
          continue;
        }
        const P3 vy{(i64)(2 * py.x - pa.x - pb.x), (i64)(2 * py.y - pa.y - pb.y), (i64)(2 * py.z - pa.z - pb.z)};
        if (!no_canonical && p3_norm2(vy) > D2 && ix.point_id(uy) < ix.point_id(cx.u)) {
          ++ls->q4_rej_once;
          continue;
        }
        if (!q4_i64_prefilter(D2, l_ax, l_bx, l_ay, l_by, l_xy)) {
          ++ls->q4_rej_i64;
          continue;
        }
        if (!q4_face_power_prefilter(f3s, py)) {
          ++ls->q4_rej_face_power;
          continue;
        }
        const Q4Form f4 = q4_form(pa, pb, px, py);
        if (f4.det == 0) {
          ++ls->q4_rej_det;
          continue;
        }
        if (!q4_center_strictly_inside(f4, pa, pb, px, py)) {
          ++ls->q4_rej_center;
          continue;
        }
        if (deep) {
          ++ls->depth_killed[2];
          continue;
        }
        lo->push_back(BallCandidate{ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4), 4});
        sc.emit.tick();
        if (sc.emit.stopped) return;
        ++ls->candidates[2];
      }
      ent += ent_at;
      i = j;
    }
  }
  // Recolte de la monnaie de consultation des cellules (une fois par ancre).
  ls->cells_consulted_g8 += sc.grid.cells_consulted;
  ls->cells_consulted_g16 += sc.grid16.cells_consulted;
  sc.grid.cells_consulted = 0;
  sc.grid16.cells_consulted = 0;
}

}  // namespace generate_detail

struct GenerateOptions {
  i64 s = 8;
  u64 smax = 11;
  int threads = 1;
  // Plafond d'emission des candidats BRUTS (caps.hpp) : verifie PENDANT
  // l'emission (compteur approche par tranches de 4096, puis somme exacte a
  // la fusion) — le refus precede la FUSION GLOBALE et le tri. Abaissable
  // (tests, campagnes), jamais releve au-dela du structurel.
  u64 max_raw_candidates = kMaxRawCandidates;
  // Budget partiel (proxy de payload logique nomme, 0 = aucun) : ici il ne
  // garde QUE le payload 2E de la fusion globale (sortie a reserver + shards
  // nommes encore vivants — des TAILLES, jamais un pic d'allocation) ; les
  // autres consommateurs nommes sont gardes par run.hpp.
  u64 memory_budget_bytes = 0;
#if defined(MHGP6_TESTING)
  // Caps ABAISSABLES en test (0 = structurel) : exercer les branches de
  // refus du front fusionne et leur instant pre-insertion dans les vecteurs
  // GLOBAUX wave/next/out (jamais les shards locaux) a petit n.
  u64 wave_tasks_cap_for_tests = 0;
  u64 alive_rects_cap_for_tests = 0;
#endif
#if defined(MHGP6_TESTING)
  // Uniquement compile dans les cibles de test : aucune API ou CLI produit ne
  // peut contourner le profil s >= 8 par ce champ.
  bool allow_subprofile_separation_for_tests = false;
  // Porte de la descente fusionnee : masque initial reduit (une lane), pour la
  // comparaison fused(m=7) vs trois descentes fused(m=1<<q).
  u8 fused_mask_for_tests = 0;
#endif
  size_t pretest_query_min_points = 512;
  size_t cell_grid_min_sites = kCellGridMinSites;
  // Sonde E6 opt-in (--sonde-e6) : compte le min des temoins des cellules de
  // corde pour les seeds tuees par cœur. Lecture seule, objet inchange.
  bool e6_probe = false;
  // Experimentation E3/G16 (nommage impose par l'audit : ce n'est PAS le
  // Tier R de l'architecture, c'est le raffinement du tueur E3 par ancre).
  // BRAS SEPARES pour l'attribution economique (quatrieme tour) : chaque
  // bras change UN facteur ; g16_leve = les deux vetos leves (l'ancien
  // --e6-grille). Objet INCHANGE dans tous les bras (certificats 10.5).
  // Repli : sur un echec de construction G16, la voie historique (G8,
  // politique) reprend — jamais un trou de couverture.
  E3G16Mode e3_mode = E3G16Mode::kOff;
};

inline void generate_candidates(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                GenerateStats* st) {
  using namespace generate_detail;
  out->clear();
  const bool float_on = float_filter_runtime_enabled();
  const bool genfilter_nonstrict = MHGP6_MUTANT("genfilter-nonstrict");
  const bool seed_core_nonstrict = MHGP6_MUTANT("q4-seed-core-nonstrict");
  const bool no_canonical = MHGP6_MUTANT("q4-no-canonical");
  const u64 h_of[3] = {lane_h(Lane::kQ2, opt.smax), lane_h(Lane::kQ3, opt.smax), lane_h(Lane::kQ4, opt.smax)};
#if defined(MHGP6_TESTING)
  const bool allow_subprofile = opt.allow_subprofile_separation_for_tests;
  const u8 initial_mask = opt.fused_mask_for_tests != 0 ? opt.fused_mask_for_tests : (u8)0b111;
#else
  constexpr bool allow_subprofile = false;
  constexpr u8 initial_mask = 0b111;
#endif

  // ---- Front fusionne : UNE descente pour les trois lanes.
  const auto t0 = std::chrono::steady_clock::now();
  std::vector<MultiAliveRect> alive;
#if defined(MHGP6_TESTING)
  const u64 wave_cap_eff = opt.wave_tasks_cap_for_tests ? opt.wave_tasks_cap_for_tests : kMaxWaveTasks;
  const u64 alive_cap_eff = opt.alive_rects_cap_for_tests ? opt.alive_rects_cap_for_tests : kMaxAliveRects;
#else
  const u64 wave_cap_eff = kMaxWaveTasks;
  const u64 alive_cap_eff = kMaxAliveRects;
#endif
  alive_rectangles_fused(ix, opt.s, h_of, initial_mask, opt.threads, &alive, st, allow_subprofile,
                         wave_cap_eff, alive_cap_eff);
  st->t_wspd_ms += ms_since(t0);
  if (st->cap_refus != kCapRefusNone) return;

  // ---- Corps par rectangle : chaque lane du masque, dans l'ordre q2, q3, q4.
  const auto t1 = std::chrono::steady_clock::now();
  const size_t nrect = alive.size();
  const size_t T = planned_workers(nrect, opt.threads);
  std::vector<std::vector<BallCandidate>> louts(T);
  std::vector<GenerateStats> lst(T);
  std::vector<AnchorScratch> lsc(T);
  for (AnchorScratch& x : lsc) x.cell_min_sites = opt.cell_grid_min_sites;
  // CAP DE CARDINALITE A OVERSHOOT BORNE (caps.hpp) — garde PRE-FUSION
  // GLOBALE, PAS un arret « avant materialisation » : les shards locaux
  // materialisent jusqu'a l'observation du drapeau. Compte publie par
  // tranches de 4096 emissions au site meme (drapeau relu toutes les 64,
  // INCLUSES dans le bloc), arret remonte a la boucle d'ancres, somme
  // EXACTE avant la fusion : pour un cap H, overshoot borne par 4096 x T.
  // L'ordre et le contenu des emissions sous le plafond sont inchanges
  // (bit-identite).
  std::atomic<u64> emitted_approx{0};
  std::atomic<bool> cap_stop{false};
  for (AnchorScratch& x : lsc) {
    x.emit.total = &emitted_approx;
    x.emit.stop = &cap_stop;
    x.emit.cap = opt.max_raw_candidates;
  }
  const size_t created = parallel_items(nrect, (int)T, [&](size_t ri, size_t t) {
    if (cap_stop.load(std::memory_order_relaxed)) return;
    const MultiAliveRect& ar = alive[ri];
    AnchorScratch& sc = lsc[t];
    std::vector<BallCandidate>* lo = &louts[t];
    GenerateStats* ls = &lst[t];
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    const u64 nA = (u64)(ra.last - ra.first + 1), nB = (u64)(rb.last - rb.first + 1);
    for (int li = 0; li < 3; ++li) {
      if (!(ar.mask & (1u << li))) continue;
      const Lane lane = li == 0 ? Lane::kQ2 : li == 1 ? Lane::kQ3 : Lane::kQ4;
      corner_histograms(ix, lane, ar.r, &sc.ha, &sc.hb);
      // P_factor = evaluations reellement payees par corner_histograms (les
      // diagonales z == a sont sautees avant universal_over_corners).
      ls->p_factor[li] += nA * (nA - 1) + nB * (nB - 1);
      const u64 need = h_of[li] > ar.core[li] ? h_of[li] - ar.core[li] : 0;
      ls->anchors[li] += nA * nB;  // le grand-livre reste ferme : toutes les paires comptees
      if (need == 0) {
        ls->anchors_killed_hist[li] += nA * nB;
        continue;
      }
      // Handles et route de pretest par requete : une fois par (rectangle,
      // lane q3/q4) — q2 n'a ni cover ni pretest.
      bool pretest_by_query = false;
      // COEFFICIENT DE COVER PAR LANE (P0 audit v6 31 aout) : 3 pour q3
      // (sharp : contient tout interieur strict q3), 4 pour q4 (Jung : les
      // interieurs q4 vivent dans le coefficient 4 — le coefficient 3 v5
      // perdait des temoins, contre-fixture au tetraedre regulier + z
      // interieur, |2z-a-b|² = 2916 entre 3D² = 2400 et 4D² = 3200).
      const i64 cover_coef = (li == 2 && !MHGP6_MUTANT("q4-cover-coef3")) ? 4 : 3;
      if (li >= 1) {
        rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), cover_coef, &sc.handles, &sc.cover_nodes);
        sc.handle_points = 0;
        for (const NodeRef h : sc.handles) {
          const NodeRange r = ix.range_of(h);
          sc.handle_points += (u64)(r.last - r.first + 1);
        }
        ls->h_rect[li] += sc.handle_points;
        pretest_by_query = sc.handle_points >= opt.pretest_query_min_points;
        if (pretest_by_query)
          rect_diametral_candidates(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), &sc.query, &sc.cover_nodes);
      }
      u64 visitees = 0, tues_ligne = 0, tues_seuil = 0;
      for (i32 ua = ra.first; ua <= ra.last; ++ua) {
        const u64 ha_a = sc.ha[(size_t)(ua - ra.first)];
        if (ha_a >= need) {
          tues_ligne += nB;
          continue;  // la ligne entiere meurt d'un seul test
        }
        for (i32 ub = rb.first; ub <= rb.last; ++ub) {
          if (ha_a + sc.hb[(size_t)(ub - rb.first)] >= need) {
            ++tues_seuil;
            continue;
          }
          ++visitees;
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (D2 == 0) continue;
          if (li == 0) {
            lo->push_back(BallCandidate{q2_ball_key(pa, pb), promote_level(q2_exact_level(D2)), 2});
            sc.emit.tick();
            if (sc.emit.stopped) return;
            ++ls->candidates[0];
            continue;
          }
          // Credit d'extremite deja acquis, transmis a la cascade aval.
          const EndpointCredit ec{ha_a + sc.hb[(size_t)(ub - rb.first)], ra.first, ra.last, rb.first, rb.last};
          bool pretested = false;
          if (pretest_by_query) {
            const int k = anchor_kill_from_candidates(sc.query, ix.upos, ua, ub, pa, pb, D2, lane,
                                                      li == 1 ? 12 : 8, h_of[li], &ec);
            if (k == 1) {
              if (li == 1)
                ++ls->anchors_killed_w3;
              else
                ++ls->anchors_killed_w4;
              continue;
            }
            if (k == 2) {
              ++ls->anchors_killed_sectors[li];
              continue;
            }
            pretested = true;
          }
          // Population COMMUNE des monnaies par ancre (audit du 31 aout) :
          // une ancre « entree » = ce point precis, apres le prétest par
          // requete — M_anchor, H_scan et `anchor_entries` partagent cette
          // population pour les deux lanes.
          const u64 visits_before = sc.visits;
          anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, cover_coef, &sc.cover, &sc.visits, &sc.cover_tmp);
          ++ls->anchor_entries[li];
          ls->m_anchor[li] += sc.cover.size();
          ls->h_scan[li] += sc.visits - visits_before;
          if (li == 1) {
            scan_anchor_q3(ix, sc, ua, ub, pa, pb, D2, h_of[1], float_on, genfilter_nonstrict, pretested, lo, ls,
                           &ec);
          } else {
            process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h_of[2], float_on, seed_core_nonstrict, no_canonical,
                              pretested, opt.e6_probe, opt.e3_mode, lo, ls, &ec);
          }
          // ARRET D'EMISSION REMONTE (second jet auditeur) : l'appelant
          // n'entame pas l'ancre suivante — overshoot borne par 4096 x T,
          // plus aucune ancre entiere.
          if (sc.emit.stopped) return;
        }
      }
      ls->hist_killed_rows[li] += tues_ligne;
      ls->hist_killed_thresh[li] += tues_seuil;
      ls->hist_survivors[li] += visitees;
      ls->anchors_killed_hist[li] += tues_ligne + tues_seuil;
    }
    lsc[t].emit.sync();  // flush du reliquat de l'ancre (borne d'overshoot)
  });
  st->workers_rects = std::max(st->workers_rects, (u64)created);
  // Somme EXACTE avant fusion : le refus precede la fusion globale.
  u64 exact_fusion = 0;
  {
    u64 exact = 0;
    for (size_t t = 0; t < T; ++t) exact += (u64)louts[t].size();
    const bool mut_skip = MHGP6_MUTANT("caps-drop-emission");
    if (!mut_skip && (cap_stop.load(std::memory_order_relaxed) || exact > opt.max_raw_candidates)) {
      st->cap_refus = kCapRefusRawCandidates;
      st->emitted_at_refus = exact;
      for (size_t t = 0; t < T; ++t) std::vector<BallCandidate>().swap(louts[t]);
      st->t_rects_ms += ms_since(t1);
      return;
    }
    exact_fusion = exact;
  }
  // GARDE BUDGETAIRE 2E AVANT LA FUSION GLOBALE (REPONSE_AUDITEURS § 6.1,
  // vocabulaire § 5.9 3e contre-lecture) : a la fusion, le PAYLOAD LOGIQUE
  // NOMME vaut 2E x sizeof(BallCandidate) — la sortie a reserver (E) plus
  // les shards nommes encore vivants (E) ; ce sont des TAILLES, jamais un
  // pic d'allocation « au pire » (les capacites geometriques des shards
  // peuvent depasser leurs tailles, le budget ne promet ni RSS ni absence
  // d'OOM). `out` est structurellement VIDE ici (unique appel de
  // generate_candidates). Le facteur 2 passe DANS le helper (jamais un
  // `2 * E` pre-calcule qui contournerait sa protection d'overflow). Le
  // refus PRECEDE la reserve : la garde du tri en aval (meme arithmetique)
  // arrive APRES que le payload a ete materialise. Mutant
  // caps-skip-prefusion-budget : garde sautee, le refus retombe sur le tri
  // (message DIFFERENT — tue par la fenetre dediee du selftest).
  if (opt.memory_budget_bytes != 0 && !MHGP6_MUTANT("caps-skip-prefusion-budget") &&
      !fits_budget(exact_fusion, (u64)sizeof(BallCandidate), 2, opt.memory_budget_bytes)) {
    st->cap_refus = kCapRefusFusionBudget;
    st->emitted_at_refus = exact_fusion;
    for (size_t t = 0; t < T; ++t) std::vector<BallCandidate>().swap(louts[t]);
    st->t_rects_ms += ms_since(t1);
    return;
  }
  // RESERVE UNIQUE demandee a la somme exacte (dernier jet auditeur) :
  // evite les croissances geometriques de la fusion sans compactage
  // pre-garde. C++20 ne garantit que capacity() >= exact — la capacite
  // effectivement OBSERVEE est exposee en diagnostic avant le tri et les
  // fenetres causales de la porte se calculent sur elle.
  out->reserve(out->size() + (size_t)exact_fusion);
  const bool drop = MHGP6_MUTANT("par-drop-shard");
  for (size_t t = 0; t < T; ++t) {
    if (drop && t == 0 && T > 1) continue;
    out->insert(out->end(), louts[t].begin(), louts[t].end());
    st->add_from(lst[t]);
    // Liberation du shard des sa fusion (balayage du 1er septembre : la
    // coexistence louts + out doublait le pic et abaissait le mur a ~1,6M).
    std::vector<BallCandidate>().swap(louts[t]);
  }
  st->t_rects_ms += ms_since(t1);
}

}  // namespace mhgp6
