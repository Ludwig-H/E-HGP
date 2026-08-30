// MorseHGP3D v5 — generation des boules candidates : les trois lanes.
//
// Pour chaque lane q ∈ {2,3,4} :
//   1. descente WSPD ternaire (`alive_rectangles`) : une paire de nœuds est
//      MORTE si son cœur compte >= h_q temoins universels (sans descente),
//      TERMINALE si separee (instruite), SCINDEE sinon (plus grand diametre) ;
//   2. par rectangle vivant : histogrammes h_a/h_b (autorite 8 coins, exacte),
//      ancres survivantes (h_coeur + h_a + h_b < h_q — theoreme de
//      disjonction, fail-open) ;
//   3. par ancre : cover (coef 3), puis
//        q2 : la boule diametrale, toujours emise (la profondeur se decide au
//             census) ;
//        q3 : seeds aigus canoniques ; filtre de profondeur A LA GENERATION
//             (#{z ∈ cover : P(z) < 0} minore |I_B| — atteindre h_3 tue
//             AVANT l'emission ; etage flottant certifie, repli affine exact) ;
//        q4 : ancre tuee par le compte W_4 exact ; seed aigu ; cœur universel
//             du seed (Jung : P < 0 et 2P² > JB² ⟹ interieur de TOUTE sphere
//             admissible, compte a h_4) ; completions sur la lentille :
//             owner 6 aretes, exact-once du seed, prefiltres du bien-centrage
//             (etage i64, puissance de face), Cramer, centre strict ; filtre
//             de profondeur a la generation.
// Toutes les decisions sont entieres ; le flottant n'est qu'un filtre certifie
// (pipeline/float_filter.hpp). Sorties : un multiensemble de BallCandidate,
// INDEPENDANT du decoupage parallele (chaque ouvrier a son brouillon et son
// vecteur ; fusion en ordre d'ouvrier, puis tri stable + RLE canonisent).
//
// Completude (docs/MATHEMATIQUES.md § 2) : une boule pertinente pour K <= K_max
// a |I_B| <= K_max + 1 − q ; ses temoins de fuseau sont sous h_q : aucun filtre
// ci-dessus ne perd un plateau pertinent. Toute optimisation ici change le
// COUT de decouverte, jamais l'objet — c'est la porte de conformite v4 qui le
// grave (digest_balls).
#pragma once

#include <chrono>
#include <functional>
#include <limits>
#include <vector>

#include "../lanes/edge_cover.hpp"
#include "../lanes/cell_grid.hpp"
#include "../lanes/chord_kill.hpp"
#include "../lanes/sector_kill.hpp"
#include "../lanes/q2.hpp"
#include "../lanes/q3.hpp"
#include "../lanes/q4.hpp"
#include "../parallel/pool.hpp"
#include "../spindle/witness_count.hpp"
#include "../wspd/wavefront.hpp"
#include "candidates.hpp"
#include "float_filter.hpp"

namespace mhgp5 {

// Compteurs de la generation (jamais une autorite).
struct GenerateStats {
  u64 rect_alive[3] = {0, 0, 0};
  // GRAND-LIVRE DU RAFFINEMENT POST-SEPARATION (docs/MESURES_ECHELLE.md § 4).
  // Identite exigee par la porte : emis + tues == base, exactement, par lane.
  u64 postsep_base_mass[3] = {0, 0, 0};     // Σ |A||B| des rectangles vivants AVANT raffinement
  u64 postsep_emitted_mass[3] = {0, 0, 0};  // Σ |A||B| des sous-rectangles EMIS
  u64 postsep_killed_mass[3] = {0, 0, 0};   // Σ |A||B| des sous-rectangles certifies MORTS
  u64 postsep_parent_rects[3] = {0, 0, 0};  // rectangles vivants du front WSPD canonique
  u64 postsep_emitted_rects[3] = {0, 0, 0}; // sous-produits effectivement remis aux lanes
  u64 postsep_subrects[3] = {0, 0, 0};      // etats depiles, racines incluses
  u64 postsep_core_evals[3] = {0, 0, 0};    // appels de comptage universel payes par le raffinement
  u64 postsep_core_nodes[3] = {0, 0, 0};    // nœuds d'index visites dans ces appels
  u64 postsep_corner_evals[3] = {0, 0, 0};  // evaluations de coins dans ces appels
  u64 postsep_rollbacks[3] = {0, 0, 0};     // scissions annulees : au moins un enfant non separe
  u64 postsep_core_regressions[3] = {0, 0, 0};  // compte frais < minorant du parent (invariant)
  u64 postsep_ledger_violations = 0;             // rempli et refuse par run_pipeline avant publication
  u64 rect_visited[3] = {0, 0, 0};
  u64 anchors[3] = {0, 0, 0};
  u64 anchors_killed_hist[3] = {0, 0, 0};
  // Les trois classes DISJOINTES de la porte histogramme (audit 732529b3) :
  // hist_total_pairs = hist_killed_rows + hist_killed_thresh + hist_survivors.
  u64 hist_killed_rows[3] = {0, 0, 0};   // lignes A fermees : h_a(a) >= need
  u64 hist_killed_thresh[3] = {0, 0, 0}; // seuil B sur les lignes A restantes
  u64 hist_survivors[3] = {0, 0, 0};     // couples effectivement transmis
  u64 anchors_killed_w4 = 0;
  u64 anchors_killed_sectors[3] = {0, 0, 0};  // test d'ancre par secteurs (sector_kill.hpp) : ancres mortes sans enumerer les seeds
  u64 anchors_killed_cells[3] = {0, 0, 0};    // grille de cellules (cell_grid.hpp, theoreme 10.5) : toutes les cellules mortes
  u64 seeds_killed_cells[3] = {0, 0, 0};      // seeds dont le centre (q3) / la corde (q4) ne rencontre que des cellules mortes
  u64 grids_attempted[3] = {0, 0, 0};         // grilles VOULUES par la politique (cell_grid_wanted) : tentatives de construction
  u64 grids_built[3] = {0, 0, 0};             // grilles effectivement construites (CellGrid::build a reussi) ; attempted − built = replis fail-open
  u64 grids_all_dead[3] = {0, 0, 0};          // grilles construites dont toutes les cellules necessaires sont mortes (= anchors_killed_cells)
  u64 anchors_killed_w3 = 0;                   // test W_3 EXACT (temoins universels du disque des centres), lane q3
  u64 invariant_jneg = 0;                      // seeds q4 aigus avec J < 0 : INATTEIGNABLE par theoreme — toute occurrence est une violation d'invariant
  u64 candidates[3] = {0, 0, 0};
  u64 depth_killed[3] = {0, 0, 0};
  u64 seeds[2] = {0, 0};             // q3, q4
  u64 seeds_killed_core = 0;         // q4 : cœur de seed (Jung, K = 1)
  u64 seeds_killed_chord = 0;        // q4 : morceaux de corde (chord_kill.hpp) — seed vivant au cœur, mort par morceaux
  // Filtre d'enveloppe d'ancre EXPERIMENTAL, opt-in. Deux routes : 0 =
  // pretests sur le cover, 1 = pretests par requete de rectangle. `before`
  // conserve le sens du cover historique coefficient 3 ; `after` est la vue
  // transmise aux scans. Ces masses mesurent le travail, jamais l'objet.
  u64 edge_envelope_anchors[3][2] = {};
  u64 edge_envelope_sites_before[3][2] = {};
  u64 edge_envelope_sites_after[3][2] = {};
  u64 edge_envelope_cross_tests[3][2] = {};
  // Profil q4 par etage (MESURE, compile seulement avec -DMHGP5_PROFILE_Q4 : cible mhgp5_q4_stage_probe) :
  // nanosecondes cumulees dans les tests d'ancre, l'enumeration des seeds, les scans de cœur/corde, les completions.
  u64 prof_q4_anchor_ns = 0, prof_q4_core_ns = 0, prof_q4_compl_ns = 0, prof_q4_cover_ns = 0;
  // Masses de boucle q4 du meme profil. `cover_visits` est le delta exact des
  // points balayes dans les handles ; `cover_sites` est la sortie retenue. Les trois
  // compteurs suivants ferment les boucles cœur et profondeur sans resommer les
  // sous-categories imbriquees de q4_cert.
  u64 q4_covers_built = 0, q4_cover_visits = 0, q4_cover_sites = 0;
  u64 q4_core_site_tests = 0, q4_depth_entries = 0, q4_power_tests = 0;
  u64 q4_completions = 0, q4_rej_lens = 0, q4_rej_owner = 0, q4_rej_once = 0, q4_rej_i64 = 0,
      q4_rej_face_power = 0, q4_rej_det = 0, q4_rej_center = 0;
  u64 float_cert_neg = 0, float_cert_pos = 0, float_fallback = 0;
  u64 q3_cert[3] = {0, 0, 0};  // certifications de la lane q3 seule (neg, pos, repli) — contrat de la lane par lots
  u64 q4_cert[6] = {0, 0, 0, 0, 0, 0};  // lane q4 seule : cert_pos, cert_neg, jung_kill, jung_skip, jung_fallback, float_fallback
  u64 jung_cert_kill = 0, jung_cert_skip = 0, jung_fallback = 0;
  u64 workers_wspd[3] = {0, 0, 0};
  u64 workers_rects[3] = {0, 0, 0};
  double t_wspd_ms[3] = {0, 0, 0};
  double t_rects_ms[3] = {0, 0, 0};
  void add_from(const GenerateStats& o) {
    for (int i = 0; i < 3; ++i) {
      anchors[i] += o.anchors[i];
      anchors_killed_hist[i] += o.anchors_killed_hist[i];
      hist_killed_rows[i] += o.hist_killed_rows[i];
      hist_killed_thresh[i] += o.hist_killed_thresh[i];
      hist_survivors[i] += o.hist_survivors[i];
      candidates[i] += o.candidates[i];
      depth_killed[i] += o.depth_killed[i];
    }
    anchors_killed_w4 += o.anchors_killed_w4;
    for (int i = 0; i < 3; ++i) anchors_killed_sectors[i] += o.anchors_killed_sectors[i];
    for (int i = 0; i < 3; ++i) anchors_killed_cells[i] += o.anchors_killed_cells[i];
    for (int i = 0; i < 3; ++i) seeds_killed_cells[i] += o.seeds_killed_cells[i];
    for (int i = 0; i < 3; ++i) grids_attempted[i] += o.grids_attempted[i];
    for (int i = 0; i < 3; ++i) grids_built[i] += o.grids_built[i];
    for (int i = 0; i < 3; ++i) grids_all_dead[i] += o.grids_all_dead[i];
    for (int i = 0; i < 3; ++i) {
      postsep_base_mass[i] += o.postsep_base_mass[i];
      postsep_emitted_mass[i] += o.postsep_emitted_mass[i];
      postsep_killed_mass[i] += o.postsep_killed_mass[i];
      postsep_parent_rects[i] += o.postsep_parent_rects[i];
      postsep_emitted_rects[i] += o.postsep_emitted_rects[i];
      postsep_subrects[i] += o.postsep_subrects[i];
      postsep_core_evals[i] += o.postsep_core_evals[i];
      postsep_core_nodes[i] += o.postsep_core_nodes[i];
      postsep_corner_evals[i] += o.postsep_corner_evals[i];
      postsep_rollbacks[i] += o.postsep_rollbacks[i];
      postsep_core_regressions[i] += o.postsep_core_regressions[i];
    }
    postsep_ledger_violations += o.postsep_ledger_violations;
    prof_q4_anchor_ns += o.prof_q4_anchor_ns; prof_q4_core_ns += o.prof_q4_core_ns;
    prof_q4_compl_ns += o.prof_q4_compl_ns; prof_q4_cover_ns += o.prof_q4_cover_ns;
    q4_covers_built += o.q4_covers_built;
    q4_cover_visits += o.q4_cover_visits;
    q4_cover_sites += o.q4_cover_sites;
    q4_core_site_tests += o.q4_core_site_tests;
    q4_depth_entries += o.q4_depth_entries;
    q4_power_tests += o.q4_power_tests;
    anchors_killed_w3 += o.anchors_killed_w3;
    invariant_jneg += o.invariant_jneg;
    seeds[0] += o.seeds[0];
    seeds[1] += o.seeds[1];
    seeds_killed_core += o.seeds_killed_core;
    seeds_killed_chord += o.seeds_killed_chord;
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
    for (int i = 0; i < 3; ++i) q3_cert[i] += o.q3_cert[i];
    for (int i = 0; i < 6; ++i) q4_cert[i] += o.q4_cert[i];
    for (int i = 0; i < 3; ++i)
      for (int r = 0; r < 2; ++r) {
        edge_envelope_anchors[i][r] += o.edge_envelope_anchors[i][r];
        edge_envelope_sites_before[i][r] += o.edge_envelope_sites_before[i][r];
        edge_envelope_sites_after[i][r] += o.edge_envelope_sites_after[i][r];
        edge_envelope_cross_tests[i][r] += o.edge_envelope_cross_tests[i][r];
      }
    jung_cert_kill += o.jung_cert_kill;
    jung_cert_skip += o.jung_cert_skip;
    jung_fallback += o.jung_fallback;
  }
};

inline void record_edge_envelope(GenerateStats* st, Lane lane, bool pretest_by_query,
                                 const EdgeEnvelopeCounts& c) {
  const int li = lane_index(lane);
  const int route = pretest_by_query ? 1 : 0;
  ++st->edge_envelope_anchors[li][route];
  st->edge_envelope_sites_before[li][route] += c.sites_before;
  st->edge_envelope_sites_after[li][route] += c.sites_after;
  st->edge_envelope_cross_tests[li][route] += c.cross_tests;
}

struct AliveRect {
  WspdRect r;
  u64 core;  // h_coeur de la lane
};

namespace generate_detail {

inline double ms_since(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

// Descente WSPD ternaire d'une lane, parallele par tranches ORDONNEES de la
// vague : chaque tranche ecrit ses tampons, la concatenation se fait en ordre
// de tranche — sortie bit-identique au sequentiel.
// RAFFINEMENT POST-SEPARATION d'UN rectangle vivant : partition post-WSPD,
// jamais une nouvelle WSPD. Une scission n'est publiee que si ses DEUX enfants
// satisfont encore le predicat entier `separated`; sinon elle est annulee et le
// parent est emis atomiquement. Les produits enfants partitionnent alors les
// paires du parent (`ix.nodes[v].left/right` partitionne sa plage de feuilles).
// Le minorant de cœur du parent reste prouve sur tout sous-produit : on conserve
// donc `max(parent, frais)`, jamais leur somme. En q2 la route reste desactivee :
// voir `GenerateOptions::postsep_refine_levels`.
struct PostsepLedger {
  // Les plages de positions sont indexees en i32 : leur masse totale est
  // strictement inferieure a C(2^31, 2), donc u64 ferme sans ambiguite ce
  // ledger, y compris `emitted + killed`. Les multiplicites de PointId ont un
  // ledger distinct dans la WSPD et ne sont pas revendiquees ici.
  u64 base = 0, emitted = 0, killed = 0;
  u64 parents = 0, emitted_rects = 0, subrects = 0;
  u64 core_evals = 0, core_nodes = 0, corner_evals = 0;
  u64 rollbacks = 0, core_regressions = 0;
};
inline void postsep_refine(const CloudIndex& ix, const WspdRect& r, u64 core, const u64 h_of[3], int lane_idx, u8 mask,
                           i64 separation_s, u32 levels, std::vector<AliveRect>* out, PostsepLedger* led,
                           std::vector<WspdRect>* killed_trace = nullptr) {
  const auto mass = [&](const WspdRect& q) -> u64 {
    const NodeRange qa = ix.range_of(q.a), qb = ix.range_of(q.b);
    return (u64)(qa.last - qa.first + 1) * (u64)(qb.last - qb.first + 1);
  };
  const auto emit = [&](const WspdRect& q, u64 qcore) {
    led->emitted += mass(q);
    ++led->emitted_rects;
    out->push_back(AliveRect{q, qcore});
    if (MHGP5_MUTANT("postsep-duplicate-child")) {
      led->emitted += mass(q);
      ++led->emitted_rects;
      out->push_back(AliveRect{q, qcore});
    }
  };
  led->base += mass(r);
  ++led->parents;
  // ROUTE q2 INTERDITE : la fixture radix a six points de la porte montre un
  // reveil reel d'histogramme (13 -> 14 candidats, digest_balls different) sans
  // aucune masse q2 tuee. Le ledger de paires reste donc vert et ne suffit pas.
  // `postsep-refine-q2` ouvre seulement cette faute sous MHGP5_TESTING afin que
  // la contre-fixture permanente la tue par sa divergence semantique.
  const bool refine_q2 = MHGP5_MUTANT("postsep-refine-q2");
  if (levels == 0 || (lane_idx == 0 && !refine_q2)) {  // desactive, ou lane q2 fermee
    emit(r, core);
    return;
  }
  const u64 h = h_of[lane_idx];
  struct Sub {
    WspdRect r;
    u64 core;
    u32 d;
  };
  Sub st[32];  // profondeur <= 3 => au plus 8 feuilles et 15 nœuds ; 32 est un majorant franc
  int top = 0;
  st[top++] = Sub{r, core, 0};
  while (top > 0) {
    const Sub cur = st[--top];
    ++led->subrects;
    const bool a_leaf = cur.r.a < 0, b_leaf = cur.r.b < 0;
    if (cur.d >= levels || (a_leaf && b_leaf)) {
      emit(cur.r, cur.core);
      continue;
    }
    const AxisBox va = ix.box_of(cur.r.a), vb = ix.box_of(cur.r.b);
    const i64 w2a = wspd_detail::box_w2(va), w2b = wspd_detail::box_w2(vb);
    const bool split_a = (!a_leaf) && (b_leaf || w2a >= w2b);  // toujours le plus grand DIAMETRE, jamais le plus peuple
    const NodeRef keep = split_a ? cur.r.b : cur.r.a;
    const RadixNode& nd = ix.nodes[(size_t)(split_a ? cur.r.a : cur.r.b)];
    const WspdRect kids[2] = {split_a ? WspdRect{nd.left, keep} : WspdRect{keep, nd.left},
                              split_a ? WspdRect{nd.right, keep} : WspdRect{keep, nd.right}};
    // Le test de boites centre/rayon n'est pas hereditaire quand le centre de
    // la boite enfant se deplace. Aucune decision enfant n'est donc consommee
    // tant que les DEUX enfants ne sont pas separes (rollback transactionnel).
    if (!wspd_detail::separated(ix.box_of(kids[0].a), ix.box_of(kids[0].b), separation_s, 1) ||
        !wspd_detail::separated(ix.box_of(kids[1].a), ix.box_of(kids[1].b), separation_s, 1)) {
      ++led->rollbacks;
      emit(cur.r, cur.core);
      continue;
    }
    for (const WspdRect& k : kids) {
      ++led->core_evals;
      // MUTANT : retire l'autorite exacte aux feuilles. Une fixture q3 minimale
      // rend alors le compte frais strictement inferieur au minorant herite ;
      // la garde `core_regressions` doit refuser avant toute publication.
      const bool with_corners = !MHGP5_MUTANT("postsep-core-without-corners");
      const FusedCounts fc = count_universal_witnesses(ix, k.a, k.b, h_of, mask, with_corners);
      led->core_nodes += fc.nodes_visited;
      led->corner_evals += fc.corner_evals;
      if (fc.c[lane_idx] < cur.core) ++led->core_regressions;
      const u64 child_core = std::max(cur.core, fc.c[lane_idx]);
      // MUTANT `postsep-drop-child` : un enfant vivant est jete au lieu d'etre
      // emis — perte de paires, donc perte de boules ; le grand-livre
      // (emis + tues == base) le voit avant meme le digest.
      if (MHGP5_MUTANT("postsep-drop-child") && k.a != cur.r.a) continue;
      // MUTANT `postsep-kill-h-minus-one` : le seuil de mort passe a h - 1, ce
      // qui SUR-TUE — des sous-rectangles vivants sont certifies morts et
      // leurs boules disparaissent. C'est la faute la plus grave possible ici
      // (perte de completude), et le digest la voit.
      const u64 hk = MHGP5_MUTANT("postsep-kill-h-minus-one") && h > 0 ? h - 1 : h;
      if (child_core >= hk) {  // sous-rectangle CERTIFIE MORT : ses paires ne seront jamais enumerees
        led->killed += mass(k);
        if (killed_trace) killed_trace->push_back(k);
        continue;
      }
      if (top >= 32) {  // majorant franc : on emet plutot que de deborder (fail-open, jamais une perte)
        emit(k, child_core);
        continue;
      }
      st[top++] = Sub{k, child_core, cur.d + 1};
    }
  }
}

// GARDE DE PROFIL. La descente refuse s < 8, sauf opt-in EXPLICITE et greppable
// `allow_subprofile_separation=true`, reserve aux fixtures qui documentent leur
// sortie du profil. Le seuil assure une marge uniforme forte du cœur q3/q4 ; il
// ne prouve pas que tout cœur particulier est vide sous 8. L'oubli de l'opt-in
// leve une erreur, au lieu de produire silencieusement une mesure vide.
inline void alive_rectangles(const CloudIndex& ix, i64 s, const u64 h_of[3], int lane_idx, int threads,
                             std::vector<AliveRect>* out, u64* visited, u64* workers, u32 levels = 0,
                             PostsepLedger* ledger_out = nullptr,
                             bool allow_subprofile_separation = false) {
  out->clear();
  if (s < kSeparationProfileMin && !allow_subprofile_separation)
    throw std::invalid_argument("alive_rectangles : separation s < 8 sans opt-in test-only");
  if (ix.nodes.empty()) return;
  const u8 mask = (u8)(1u << lane_idx);
  const u64 h = h_of[lane_idx];
  std::vector<WspdRect> wave, next;
  wave.reserve(ix.nodes.size());
  for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
  std::vector<std::vector<AliveRect>> lout;
  std::vector<std::vector<WspdRect>> lnext;
  std::vector<u64> lvis;
  std::vector<PostsepLedger> lstat;
  PostsepLedger led_total;
  while (!wave.empty()) {
    const size_t nw = wave.size();
    const int t_eff = nw < 256 ? 1 : threads;  // les premieres vagues sont minuscules
    const size_t T = planned_workers(nw, t_eff);
    const size_t chunk = std::max<size_t>(1, (nw + 8 * T - 1) / (8 * T));
    const size_t nchunks = (nw + chunk - 1) / chunk;
    lout.assign(nchunks, {});
    lnext.assign(nchunks, {});
    lvis.assign(nchunks, 0);
    lstat.assign(nchunks, PostsepLedger{});
    const size_t created = parallel_items(nchunks, (int)T, [&](size_t c, size_t) {
      const size_t b0 = c * chunk, b1 = std::min(nw, b0 + chunk);
      for (size_t i = b0; i < b1; ++i) {
        const WspdRect& r = wave[i];
        ++lvis[c];
        const FusedCounts fc = count_universal_witnesses(ix, r.a, r.b, h_of, mask, false);
        if (fc.c[lane_idx] >= h) continue;  // MORTE sans descente
        const AxisBox va = ix.box_of(r.a), vb = ix.box_of(r.b);
        if (wspd_detail::separated(va, vb, s, 1)) {
          const FusedCounts ff = count_universal_witnesses(ix, r.a, r.b, h_of, mask, true);
          if (ff.c[lane_idx] < h)
            postsep_refine(ix, r, ff.c[lane_idx], h_of, lane_idx, mask, s, levels, &lout[c], &lstat[c]);
          continue;
        }
        const i64 w2a = wspd_detail::box_w2(va), w2b = wspd_detail::box_w2(vb);
        const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
        const NodeRef keep = split_a ? r.b : r.a;
        const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
        lnext[c].push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
        lnext[c].push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
      }
    });
    *workers = std::max(*workers, (u64)created);
    next.clear();
    for (size_t c = 0; c < nchunks; ++c) {
      out->insert(out->end(), lout[c].begin(), lout[c].end());
      next.insert(next.end(), lnext[c].begin(), lnext[c].end());
      *visited += lvis[c];
      led_total.base += lstat[c].base;
      led_total.emitted += lstat[c].emitted;
      led_total.killed += lstat[c].killed;
      led_total.parents += lstat[c].parents;
      led_total.emitted_rects += lstat[c].emitted_rects;
      led_total.subrects += lstat[c].subrects;
      led_total.core_evals += lstat[c].core_evals;
      led_total.core_nodes += lstat[c].core_nodes;
      led_total.corner_evals += lstat[c].corner_evals;
      led_total.rollbacks += lstat[c].rollbacks;
      led_total.core_regressions += lstat[c].core_regressions;
    }
    wave.swap(next);
  }
  if (ledger_out) *ledger_out = led_total;
}

// Histogrammes h_a/h_b (autorite 8 coins, exacte) d'un rectangle, pour une lane.
inline void corner_histograms(const CloudIndex& ix, Lane lane, const WspdRect& r, std::vector<u64>* ha,
                              std::vector<u64>* hb) {
  const NodeRange ra = ix.range_of(r.a), rb = ix.range_of(r.b);
  const AxisBox boxA = ix.box_of(r.a), boxB = ix.box_of(r.b);
  const int na = ra.last - ra.first + 1, nb = rb.last - rb.first + 1;
  ha->assign((size_t)na, 0);
  hb->assign((size_t)nb, 0);
  for (int ia = 0; ia < na; ++ia)
    for (int iz = 0; iz < na; ++iz)
      if (iz != ia && universal_over_corners(lane, ix.upos[(size_t)(ra.first + ia)], boxB, ix.upos[(size_t)(ra.first + iz)]))
        ++(*ha)[(size_t)ia];
  for (int ib = 0; ib < nb; ++ib)
    for (int iz = 0; iz < nb; ++iz)
      if (iz != ib && universal_over_corners(lane, ix.upos[(size_t)(rb.first + ib)], boxA, ix.upos[(size_t)(rb.first + iz)]))
        ++(*hb)[(size_t)ib];
}

// Brouillon par ouvrier : cover de l'ancre, lentille, sites affines.
struct AnchorScratch {
  std::vector<u64> ha, hb;
  // COUCHE PAR POINT de la cascade (audit 7d173a37, « le cœur et les facteurs
  // forment une seule cascade »). Les seuls couples a transmettre sont
  //     S_AB = { (a,b) : h_a(a) + h_b(b) < need },  need = h_q - h_coeur.
  // La ligne entiere d un `a` tel que h_a(a) >= need meurt d UN SEUL test, sans
  // developper ses |B| ancres. Le tri par classes de h_b, qui eviterait aussi
  // des `b` DANS une ligne vivante, a ete retire : il change l ordre croissant
  // historique de `ub` et casse les portes de parite CPU/batch vecteur a
  // vecteur (audit 732529b3 : 920 desaccords sur q3_lane_batched_cocirc). A
  // s >= 8 — domaine produit fixe. Ce seuil assure la marge uniforme forte du
  // coeur q3/q4 ; il ne signifie pas que tout citron particulier est vide en
  // dessous. Sur les campagnes de reference, |A||B| ~ 2, donc ce raffinement
  // ne valait rien. L ORDRE DE PARCOURS EST DONC EXACTEMENT L HISTORIQUE.
  std::vector<NodeRef> handles;
  // `cover_tmp` alterne avec `cover` comme tampon du counting sort, puis sert
  // de vue enveloppe jusqu'a l'ancre suivante. Deux capacites suffisent donc
  // pour conserver simultanement l'autorite historique et la vue de scan.
  std::vector<CoverPoint> cover, cover_tmp, lens, query;
  bool scan_cover_active = false;
  bool scan_cover_requested = false;
  bool scan_cover_query_route = false;
  u64 handle_points = 0;                                    // points des handles du rectangle courant (politique de pretest)
  u64 cover_nodes = 0, visits = 0;
  // Sites affines de l'ancre : u = 2z−a−b, q = |u|²−D² (entiers < 2^36, exacts
  // en binaire64) ; remplis PARESSEUSEMENT au premier seed.
  std::vector<i64> su0, su1, su2, sq;
  double qmax_d = 1.0, umax_d = 1.0;
  bool affine_filled = false;
  CellGrid grid;  // grille de cellules de l'ancre courante (cell_grid.hpp) ; grid.built = false si non construite
  size_t cell_min_sites = kCellGridMinSites;  // politique : grille seulement si cover >= ce seuil ; 0 = mode FORCE (toute ancre, ratio et near_m ignores ; tests)
  const std::vector<CoverPoint>& scan_sites() const { return scan_cover_active ? cover_tmp : cover; }
  void fill_affine_sites(const CloudIndex& ix, const P3& pa, const P3& pb, i64 D2) {
    const std::vector<CoverPoint>& sites = scan_sites();
    const size_t nc = sites.size();
    su0.resize(nc); su1.resize(nc); su2.resize(nc); sq.resize(nc);
    i64 qmax = 1, umax = 1;
    const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
    for (size_t i = 0; i < nc; ++i) {
      const P3& pz = ix.upos[(size_t)sites[i].u];
      const i64 u0 = 2 * pz.x - sx, u1 = 2 * pz.y - sy, u2 = 2 * pz.z - sz;
      const i64 qz = u0 * u0 + u1 * u1 + u2 * u2 - D2;
      su0[i] = u0; su1[i] = u1; su2[i] = u2; sq[i] = qz;
      qmax = std::max(qmax, qz < 0 ? -qz : qz);
      umax = std::max({umax, u0 < 0 ? -u0 : u0, u1 < 0 ? -u1 : u1, u2 < 0 ? -u2 : u2});
    }
    qmax_d = (double)qmax;
    umax_d = (double)umax;
    affine_filled = true;
  }
};

inline void configure_anchor_scan_cover(AnchorScratch& sc, bool enabled, bool pretest_by_query) {
  sc.scan_cover_active = false;
  sc.scan_cover_requested = enabled;
  sc.scan_cover_query_route = pretest_by_query;
  sc.affine_filled = false;
}

// Prepare au PREMIER seed effectivement scanne. Le compactage et la formation
// affine sont fusionnes en une seule passe : le filtre n'ajoute donc pas une
// seconde lecture complete du cover a la passe affine historique.
inline void ensure_anchor_scan_affine(const CloudIndex& ix, AnchorScratch& sc, const P3& pa, const P3& pb,
                                      i64 D2, Lane lane, GenerateStats* st) {
  if (sc.affine_filled) return;
  if (!sc.scan_cover_requested) {
    sc.fill_affine_sites(ix, pa, pb, D2);
    return;
  }
  const EdgeEnvelope envelope = lane == Lane::kQ3 ? EdgeEnvelope::kQ3 : EdgeEnvelope::kQ4Jung;
  EdgeEnvelopeCounts counts;
  counts.sites_before = sc.cover.size();
  sc.cover_tmp.clear();
  sc.cover_tmp.reserve(sc.cover.size());
  sc.su0.clear(); sc.su1.clear(); sc.su2.clear(); sc.sq.clear();
  sc.su0.reserve(sc.cover.size()); sc.su1.reserve(sc.cover.size());
  sc.su2.reserve(sc.cover.size()); sc.sq.reserve(sc.cover.size());
  i64 qmax = 1, umax = 1;
  const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
  const i64 d0 = pb.x - pa.x, d1 = pb.y - pa.y, d2 = pb.z - pa.z;
  for (const CoverPoint& cp : sc.cover) {
    const P3& pz = ix.upos[(size_t)cp.u];
    const i64 u0 = 2 * pz.x - sx, u1 = 2 * pz.y - sy, u2 = 2 * pz.z - sz;
    const i64 qz = cp.dist2q - D2;
    if (qz > 0) {
      ++counts.cross_tests;
      const i64 dw = d0 * u0 + d1 * u1 + d2 * u2;
      const i128 xi = (i128)D2 * cp.dist2q - (i128)dw * dw;
      if (!edge_envelope_outer_contains(envelope, qz, xi)) continue;
    }
    sc.cover_tmp.push_back(cp);
    sc.su0.push_back(u0); sc.su1.push_back(u1); sc.su2.push_back(u2); sc.sq.push_back(qz);
    qmax = std::max(qmax, qz < 0 ? -qz : qz);
    umax = std::max({umax, u0 < 0 ? -u0 : u0, u1 < 0 ? -u1 : u1, u2 < 0 ? -u2 : u2});
  }
  counts.sites_after = sc.cover_tmp.size();
  sc.qmax_d = (double)qmax;
  sc.umax_d = (double)umax;
  sc.scan_cover_active = true;
  sc.affine_filled = true;
  record_edge_envelope(st, lane, sc.scan_cover_query_route, counts);
}

// GRILLE DE CELLULES (cell_grid.hpp) — helpers partages par la production et
// les lanes par lots : centre v3 = N/(2G), N = W − G·d (q3) ; corde
// (N ± μ̂·n)/(2G), μ̂ = isqrt(J/2) + 1 (q4, theoreme 10.4).
// Produits scalaires entiers EXACTS du centre v3 = N/(2G3) avec la base :
// pu = N·u, pv = N·v, den = 2·G3 (v3·u = pu/den) — partages avec l'oracle du
// localisateur rationnel (tests/cell_grid_oracle.cpp).
inline void seed_center_coords(const CellGrid& g, const Q3Form& f3, const i64 d[3], i128* pu, i128* pv, i128* den) {
  *pu = 0; *pv = 0;
  for (int k = 0; k < 3; ++k) {
    const i128 Nk = f3.w[k] - f3.g * (i128)d[k];
    *pu += Nk * g.u[k];
    *pv += Nk * g.v[k];
  }
  *den = 2 * f3.g;
}
inline bool seed_center_cell_dead(const CellGrid& g, const Q3Form& f3, const i64 d[3]) {
  i128 pu, pv, den;
  seed_center_coords(g, f3, d, &pu, &pv, &den);
  return g.point_dead(pu, pv, den);
}
// Extremites de la corde (N ± μ̂·n)/(2G3) en produits scalaires exacts ; rend
// false (fail-open) si J < 0 (inatteignable par theoreme).
inline bool seed_chord_coords(const CellGrid& g, const Q3Form& f3, const i64 d[3], const P3& nrm, i64 D2, i64 l_ax, i64 l_bx,
                              i128* pu0, i128* pv0, i128* pu1, i128* pv1, i128* den) {
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
  *pu0 = pu + mu_hat * qu; *pv0 = pv + mu_hat * qv;
  *pu1 = pu - mu_hat * qu; *pv1 = pv - mu_hat * qv;
  *den = 2 * f3.g;
  return true;
}
inline bool seed_chord_cell_dead(const CellGrid& g, const Q3Form& f3, const i64 d[3], const P3& nrm, i64 D2, i64 l_ax, i64 l_bx) {
  i128 pu0, pv0, pu1, pv1, den;
  if (!seed_chord_coords(g, f3, d, nrm, D2, l_ax, l_bx, &pu0, &pv0, &pu1, &pv1, &den)) return false;
  return g.segment_dead(pu0, pv0, pu1, pv1, den);
}

// ETAGE GRILLE d'une ancre (politique cell_grid_wanted + construction +
// compteurs) : UNE definition pour les corps de production et les lanes par
// lots (aucun chemin ne construit la grille deux fois : le jeton
// AnchorPretests::kAlreadyAppliedWithGrid transmet « deja appliquee »).
// Rend true si l'ancre est MORTE (toutes les cellules necessaires mortes) ;
// sinon sc.grid.built dit si les seeds doivent etre localises. `acute_out` :
// nombre de seeds aigus canoniques de l'ancre (is_acute_seed exige deja la
// lentille : le compte vaut pour q3 comme pour q4), rendu meme quand la
// politique ne veut pas la grille (preflight des lanes par lots) ; s'il est
// nul et que le cover est sous le seuil, aucune passe n'est faite.
inline bool anchor_grid_stage(const CloudIndex& ix, AnchorScratch& sc, i32 ua, i32 ub, const P3& pa, const P3& pb, i64 D2,
                              Lane lane, u64 h, bool float_on, GenerateStats* ls, size_t* acute_out = nullptr) {
  const int li = lane == Lane::kQ3 ? 1 : 2;
  sc.grid.built = false;
  if (acute_out) *acute_out = 0;
  if (sc.cell_min_sites != 0 && sc.cover.size() < sc.cell_min_sites && !acute_out) return false;
  size_t nacute = 0, near_m = 0;
  for (const CoverPoint& cz : sc.cover) {
    if (cz.u == ua || cz.u == ub) continue;
    if (cell_grid_near_m(cz.dist2q, D2)) ++near_m;
    if (is_acute_seed(pa, pb, ix.upos[(size_t)cz.u], D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cz.u))) ++nacute;
  }
  if (acute_out) *acute_out = nacute;
  const size_t ratio = lane == Lane::kQ3 ? kCellGridSeedsRatioQ3 : kCellGridSeedsRatioQ4;
  if (!cell_grid_wanted(sc.cover.size(), nacute, near_m, h, sc.cell_min_sites, ratio)) return false;
  ++ls->grids_attempted[li];
  if (!sc.grid.build(sc.cover, ix.upos, ua, ub, pa, pb, D2, lane == Lane::kQ3 ? 12 : 8, h, float_on)) return false;
  ++ls->grids_built[li];
  if (!sc.grid.all_dead) return false;
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
        Gd((double)f3.g), Nd0((double)N0), Nd1((double)N1), Nd2((double)N2),
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

// CORPS PAR ANCRE de la lane q3 (le cover de l'ancre est deja dans sc.cover) :
// seeds aigus, filtre de profondeur (kernel affine, etage flottant certifie,
// repli exact i128), emission. Partage par generate_candidates et par les
// lanes par lots (routage hote : src/gpu/q3_lane_batched.hpp) — une seule
// definition de la lane de production.
// Jeton TYPE des pretests d'ancre (W_q exact puis secteurs, puis grille de
// cellules) : kApply (defaut, production) ; kAlreadyApplied — l'appelant a
// DEJA applique W_q et secteurs sur ce cover (pretests par requete de la
// production, route hote des lanes par lots), la grille reste a faire ;
// kAlreadyAppliedWithGrid — l'appelant a aussi fait l'etage grille
// (anchor_grid_stage) et laisse sc.grid dans l'etat qui en resulte (route
// hote apres la grille / ancre trop grande des lanes par lots : la grille
// n'est JAMAIS construite deux fois) ; kCounterfactual — mesure seulement
// (sondes, oracle ON/OFF), jamais en production : ni pretests, ni grille.
enum class AnchorPretests : u8 { kApply, kAlreadyApplied, kAlreadyAppliedWithGrid, kCounterfactual };

inline void scan_anchor_q3(const CloudIndex& ix, AnchorScratch& sc, i32 ua, i32 ub, const P3& pa, const P3& pb, i64 D2,
                           u64 h3, bool float_on, bool genfilter_nonstrict, std::vector<BallCandidate>* lo, GenerateStats* ls,
                           AnchorPretests pretests = AnchorPretests::kApply, const EndpointCredit* ec = nullptr) {
  // Tests d'ancre cumules (sector_kill.hpp) : W_3 exact puis secteurs — suffisants, l'objet est inchange.
  if (pretests == AnchorPretests::kApply) {
    const int k = anchor_kill_cumulated(sc.cover, ix.upos, ua, ub, pa, pb, D2, Lane::kQ3, 12, h3, true, ec);
    if (k == 1) { ++ls->anchors_killed_w3; return; }
    if (k == 2) { ++ls->anchors_killed_sectors[1]; return; }
  }
  // Grille de cellules (theoreme 10.5) sur les covers denses : l'ancre entiere
  // si toutes les cellules sont mortes, sinon chaque seed par son centre.
  if (pretests == AnchorPretests::kApply || pretests == AnchorPretests::kAlreadyApplied) {
    if (anchor_grid_stage(ix, sc, ua, ub, pa, pb, D2, Lane::kQ3, h3, float_on, ls)) return;
  } else if (pretests == AnchorPretests::kCounterfactual) {
    sc.grid.built = false;
  }  // kAlreadyAppliedWithGrid : sc.grid tel que laisse par l'appelant
  const i64 d3[3] = {pb.x - pa.x, pb.y - pa.y, pb.z - pa.z};
  if (!sc.scan_cover_active) sc.affine_filled = false;
  for (const CoverPoint& cp : sc.cover) {
    if (cp.u == ua || cp.u == ub) continue;
    const P3& px = ix.upos[(size_t)cp.u];
    if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cp.u))) continue;
    ++ls->seeds[0];
    const Q3Form f3 = q3_form(pa, pb, px);
    if (sc.grid.built && seed_center_cell_dead(sc.grid, f3, d3)) { ++ls->seeds_killed_cells[1]; continue; }
    if (!sc.affine_filled) ensure_anchor_scan_affine(ix, sc, pa, pb, D2, Lane::kQ3, ls);
    const std::vector<CoverPoint>& scan_sites = sc.scan_sites();
    const AffineSeed seed(f3, pa, pb, sc, float_on);
    // Filtre de profondeur a la generation : minorant certifie.
    u64 depth = 0;
    bool deep = false;
    for (size_t iz = 0; iz < scan_sites.size() && !deep; ++iz) {
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
    ++ls->candidates[1];
  }
}

#if defined(MHGP5_PROFILE_Q4)
#define MHGP5_Q4_TICK() std::chrono::steady_clock::now()
#define MHGP5_Q4_ACC(field, t0) (ls->field += (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - (t0)).count())
#define MHGP5_Q4_ADD(field, value) (ls->field += (u64)(value))
#else
#define MHGP5_Q4_TICK() 0
#define MHGP5_Q4_ACC(field, t0) ((void)(t0))
#define MHGP5_Q4_ADD(field, value) ((void)(value))
#endif

// CORPS PAR ANCRE de la lane q4 (cover deja dans sc.cover) : W_4, lentille,
// seeds aigus, cœur de seed de Jung, completions (owner, exact-once,
// prefiltres, Cramer, centre strict, profondeur), emission. Partage par
// generate_candidates et par les lanes par lots (routage hote).
inline void process_anchor_q4(const CloudIndex& ix, AnchorScratch& sc, i32 ua, i32 ub, const P3& pa, const P3& pb, i64 D2,
                              u64 h4, bool float_on, bool genfilter_nonstrict, bool seed_core_nonstrict, bool no_canonical,
                              std::vector<BallCandidate>* lo, GenerateStats* ls,
                              AnchorPretests pretests = AnchorPretests::kApply) {
  const auto q4_t_anchor = MHGP5_Q4_TICK();
  // Compte W_4 exact par ancre : n4 >= h_4 tue l'ancre entiere ; puis secteurs.
  if (pretests == AnchorPretests::kApply) {
    u64 n4 = 0;
    for (const CoverPoint& cz : sc.cover) {
      if (cz.u == ua || cz.u == ub) continue;
      if (in_spindle(Lane::kQ4, pa, pb, ix.upos[(size_t)cz.u]) && ++n4 >= h4) break;
    }
    if (n4 >= h4) {
      ++ls->anchors_killed_w4;
      MHGP5_Q4_ACC(prof_q4_anchor_ns, q4_t_anchor);
      return;
    }
    u64 wmin = 0;
    if (anchor_sector_kill(sc.cover, ix.upos, ua, ub, pa, pb, D2, 8, h4, &wmin)) {
      ++ls->anchors_killed_sectors[2];
      MHGP5_Q4_ACC(prof_q4_anchor_ns, q4_t_anchor);
      return;
    }
  }
  MHGP5_Q4_ACC(prof_q4_anchor_ns, q4_t_anchor);
  if (!sc.scan_cover_active) sc.affine_filled = false;
  sc.lens.clear();
  for (const CoverPoint& cz : sc.cover) {
    const P3& pz = ix.upos[(size_t)cz.u];
    if (p3_norm2(p3_sub(pz, pa)) <= D2 && p3_norm2(p3_sub(pz, pb)) <= D2) sc.lens.push_back(cz);
  }
  // Grille de cellules (theoreme 10.5) : ancre entiere ou corde de chaque seed.
  if (pretests == AnchorPretests::kApply || pretests == AnchorPretests::kAlreadyApplied) {
    if (anchor_grid_stage(ix, sc, ua, ub, pa, pb, D2, Lane::kQ4, h4, float_on, ls)) return;
  } else if (pretests == AnchorPretests::kCounterfactual) {
    sc.grid.built = false;
  }  // kAlreadyAppliedWithGrid : sc.grid tel que laisse par l'appelant
  const i64 d4[3] = {pb.x - pa.x, pb.y - pa.y, pb.z - pa.z};
  for (const CoverPoint& cx : sc.lens) {
    if (cx.u == ua || cx.u == ub) continue;
    const P3& px = ix.upos[(size_t)cx.u];
    if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u))) continue;
    ++ls->seeds[1];
    const i64 l_ax = p3_norm2(p3_sub(px, pa));
    const i64 l_bx = p3_norm2(p3_sub(px, pb));
    const Q3Form f3s = q3_form(pa, pb, px);
    const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
    if (sc.grid.built && seed_chord_cell_dead(sc.grid, f3s, d4, nrm, D2, l_ax, l_bx)) { ++ls->seeds_killed_cells[2]; continue; }
    // Cœur universel du seed (Jung) : J = D²(3G − 2 l_ax l_bx) = G(D² − 8|v3|²)
    // >= G·D²/3 > 0 pour tout seed aigu (|v3|² <= D²/12) : la branche Jb < 0 est
    // INATTEIGNABLE par theoreme et reste une garde fail-closed.
    const i128 Jb = (i128)D2 * (3 * f3s.g - 2 * (i128)l_ax * l_bx);
    if (Jb < 0) ++ls->invariant_jneg;  // signale, ne decide pas silencieusement (run_pipeline refuse en invariant)
    bool dead = Jb < 0;
    bool dead_by_chord = false;
    const auto q4_t_core = MHGP5_Q4_TICK();
    if (!dead) {
      if (!sc.affine_filled) ensure_anchor_scan_affine(ix, sc, pa, pb, D2, Lane::kQ4, ls);
      const std::vector<CoverPoint>& scan_sites = sc.scan_sites();
      const AffineSeed seed(f3s, pa, pb, sc, float_on);
      const double Jd = (double)Jb;
      const double Jlo = Jd * (1.0 - kJungGuard), Jhi = Jd * (1.0 + kJungGuard);
      // Morceaux de corde (chord_kill.hpp), cumules avec le cœur : desactives en
      // mode contrefactuel seulement (sondes, oracle ON/OFF).
      const bool chord_on = pretests != AnchorPretests::kCounterfactual;
      ChordPieces chord;
      if (chord_on) chord.init(Jb, MHGP5_MUTANT("chord-nonstrict"));
      u64 fcount = 0;
      for (size_t iz = 0; iz < scan_sites.size(); ++iz) {
        const CoverPoint& cz = scan_sites[iz];
        if (cz.u == ua || cz.u == ub || cz.u == cx.u) continue;
        MHGP5_Q4_ADD(q4_core_site_tests, 1);
        const double lh = seed.l_hat(sc, iz);
        if (lh > seed.bound) {
          ++ls->float_cert_pos;
          ++ls->q4_cert[0];
          continue;  // P > 0 certifie : jamais temoin (d'aucune boule, d'aucun morceau)
        }
        const P3& pz = ix.upos[(size_t)cz.u];
        const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
        if (chord_on) chord.update(lh, seed.bound, Bz, [&]() { return seed.l_exact(sc, iz); });
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
        if (chord_on && chord.dead(h4)) { dead_by_chord = true; break; }
      }
      dead = fcount >= h4 || dead_by_chord;
    }
    MHGP5_Q4_ACC(prof_q4_core_ns, q4_t_core);
    if (dead) {
      if (dead_by_chord) ++ls->seeds_killed_chord; else ++ls->seeds_killed_core;
      continue;
    }
    const std::vector<CoverPoint>& scan_sites = sc.scan_sites();
    // Completions y sur la lentille.
    const auto q4_t_compl = MHGP5_Q4_TICK();
    for (const CoverPoint& cy : sc.lens) {
      const i32 uy = cy.u;
      if (uy == cx.u || uy == ua || uy == ub) continue;
      ++ls->q4_completions;
      const P3& py = ix.upos[(size_t)uy];
      const i64 l_ay = p3_norm2(p3_sub(py, pa));
      const i64 l_by = p3_norm2(p3_sub(py, pb));
      const i64 l_xy = p3_norm2(p3_sub(py, px));
      if (l_ay > D2 || l_by > D2 || l_xy > D2) { ++ls->q4_rej_lens; continue; }
      if (!tetra_owned_by(D2, l_ax, l_ay, l_bx, l_by, l_xy, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u), ix.point_id(uy))) {
        ++ls->q4_rej_owner;
        continue;
      }
      // Exact-once du seed : le carrier est le plus petit PointId
      // parmi les faces incidentes aigues (mutant q4-no-canonical :
      // toutes les faces emettent ; le RLE masque, la porte compte).
      const P3 vy{2 * py.x - pa.x - pb.x, 2 * py.y - pa.y - pb.y, 2 * py.z - pa.z - pb.z};
      if (!no_canonical && p3_norm2(vy) > D2 && ix.point_id(uy) < ix.point_id(cx.u)) { ++ls->q4_rej_once; continue; }
      // Prefiltres du bien-centrage : etage i64 puis puissance de face.
      if (!q4_i64_prefilter(D2, l_ax, l_bx, l_ay, l_by, l_xy)) { ++ls->q4_rej_i64; continue; }
      if (!q4_face_power_prefilter(f3s, py)) { ++ls->q4_rej_face_power; continue; }
      const Q4Form f4 = q4_form(pa, pb, px, py);
      if (f4.det == 0) { ++ls->q4_rej_det; continue; }
      if (!q4_center_strictly_inside(f4, pa, pb, px, py)) { ++ls->q4_rej_center; continue; }
      // Filtre de profondeur a la generation.
      MHGP5_Q4_ADD(q4_depth_entries, 1);
      u64 depth = 0;
      bool deep = false;
      for (const CoverPoint& cz : scan_sites) {
        MHGP5_Q4_ADD(q4_power_tests, 1);
        const i128 pw = q4_power(f4, ix.upos[(size_t)cz.u]);
        if ((pw < 0 || (genfilter_nonstrict && pw <= 0)) && ++depth >= h4) { deep = true; break; }
      }
      if (deep) { ++ls->depth_killed[2]; continue; }
      lo->push_back(BallCandidate{ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4), 4});
      ++ls->candidates[2];
    }
    MHGP5_Q4_ACC(prof_q4_compl_ns, q4_t_compl);
  }
}

}  // namespace generate_detail

struct GenerateOptions;
// Remplacement d'une lane entiere (q3 ou q4) par un executeur externe — la
// lane device (src/gpu/*_lane_device.cuh). Meme signature d'effet que la lane
// integree : candidats ajoutes a `out`, statistiques cumulees. L'objet ne
// change pas : la porte d'egalite post-RLE de chaque lane en est la preuve.
using LaneOverride = std::function<void(const CloudIndex&, const GenerateOptions&, std::vector<BallCandidate>*, GenerateStats*)>;

// Politique (sans effet sur l'objet) : au-dela de ce nombre de points de
// handles dans le rectangle, les pretests d'ancre (W_q exact + secteurs) sont
// faits sur une requete d'arbre de coefficient 1 AVANT le cover, et le cover
// complet n'est construit que pour les ancres survivantes ; en dessous, sur
// le cover (balayage des handles). 0 = toujours par requete ; SIZE_MAX = jamais.
inline constexpr size_t kPretestQueryMinPoints = 512;

struct GenerateOptions {
  i64 s = 8;
  u64 smax = 11;
  int threads = 1;
#if defined(MHGP5_TESTING)
  // Uniquement compile dans les cibles de test : aucune API ou CLI produit ne
  // peut contourner le profil s>=8 par ce champ.
  bool allow_subprofile_separation_for_tests = false;
#endif
  size_t pretest_query_min_points = kPretestQueryMinPoints;
  size_t cell_grid_min_sites = kCellGridMinSites;  // grille de cellules (theoreme 10.5) des que le cover atteint ce seuil
  // Filtre ponctuel entier du cover par l'enveloppe continue des boules de
  // l'ancre. EXPERIMENTAL et desactive par defaut jusqu'a reception ON/OFF ;
  // q4 reste l'intersection du cover historique coefficient 3 avec Jung.
  bool cover_envelope_filter = false;
  // RAFFINEMENT POST-SEPARATION, niveaux L in [0, 3] (0 = desactive, defaut).
  // Prolonge la descente ternaire A L INTERIEUR d'un rectangle vivant : la
  // partition des paires est INCHANGEE (donc l'objet aussi), mais les boites
  // se resserrent, le comptage universel croit, et un sous-rectangle peut
  // mourir la ou son parent vivait. Une scission dont un enfant n'est plus
  // `separated` est annulee avant tout effet. Cout borne : au plus 2/6/14
  // nouveaux comptages par parent pour L = 1/2/3.
  // ROUTE q2 INTERDITE : une contre-fixture radix a six points reveille une
  // boule q2 apres subdivision alors que le ledger de paires reste exact. Le
  // mutant test-only `postsep-refine-q2` garde cette fermeture. En q3/q4 le
  // pretest ponctuel est toujours applique (`kApply` ou `kAlreadyApplied`).
  u32 postsep_refine_levels = 0;
  LaneOverride q3_override;  // vide : lane q3 integree
  LaneOverride q4_override;  // vide : lane q4 integree
};

// Execute une lane sur ses rectangles vivants : chaque ouvrier a son brouillon,
// son vecteur d'emissions et ses compteurs ; fusion en ordre d'ouvrier.
// Mutant `par-drop-shard` : la fusion oublie le premier ouvrier.
template <typename Body>
inline void run_lane(const std::vector<AliveRect>& alive, int threads, int lane_idx, size_t cell_min_sites,
                     std::vector<BallCandidate>* out, GenerateStats* st, Body&& body) {
  const size_t nrect = alive.size();
  const size_t T = planned_workers(nrect, threads);
  std::vector<std::vector<BallCandidate>> louts(T);
  std::vector<GenerateStats> lst(T);
  std::vector<generate_detail::AnchorScratch> lsc(T);
  for (generate_detail::AnchorScratch& x : lsc) x.cell_min_sites = cell_min_sites;
  const size_t created = parallel_items(nrect, (int)T, [&](size_t i, size_t t) { body(alive[i], lsc[t], &louts[t], &lst[t]); });
  st->workers_rects[lane_idx] = std::max(st->workers_rects[lane_idx], (u64)created);
  const bool drop = MHGP5_MUTANT("par-drop-shard");
  for (size_t t = 0; t < T; ++t) {
    if (drop && t == 0 && T > 1) continue;
    out->insert(out->end(), louts[t].begin(), louts[t].end());
    st->add_from(lst[t]);
  }
}

inline void generate_candidates(const CloudIndex& ix, const GenerateOptions& opt, std::vector<BallCandidate>* out,
                                GenerateStats* st) {
  using namespace generate_detail;
  out->clear();
  const bool float_on = float_filter_runtime_enabled();
  const bool genfilter_nonstrict = MHGP5_MUTANT("genfilter-nonstrict");
  const bool seed_core_nonstrict = MHGP5_MUTANT("q4-seed-core-nonstrict");
  const bool q4_from_q3_live = MHGP5_MUTANT("q4-seeds-from-q3-live");
  const bool no_canonical = MHGP5_MUTANT("q4-no-canonical");
  // MUTANT q4-cover-coef4 : cover au coefficient 4 — sans effet sur l'objet,
  // mais il tue des candidats profonds avant l'emission et change
  // `digest_balls` (fixture q4_cover_fixture : la boule differentielle).
  const i64 q4_cover_coef = MHGP5_MUTANT("q4-cover-coef4") ? 4 : 3;
  const u64 h_of[3] = {lane_h(Lane::kQ2, opt.smax), lane_h(Lane::kQ3, opt.smax), lane_h(Lane::kQ4, opt.smax)};
  std::vector<AliveRect> alive;
#if defined(MHGP5_TESTING)
  const bool allow_subprofile = opt.allow_subprofile_separation_for_tests;
#else
  constexpr bool allow_subprofile = false;
#endif

  // ---- q2.
  {
    const auto t0 = std::chrono::steady_clock::now();
    PostsepLedger led2;
    alive_rectangles(ix, opt.s, h_of, 0, opt.threads, &alive, &st->rect_visited[0], &st->workers_wspd[0],
                     opt.postsep_refine_levels, &led2, allow_subprofile);
    st->postsep_base_mass[0] = led2.base; st->postsep_emitted_mass[0] = led2.emitted;
    st->postsep_killed_mass[0] = led2.killed; st->postsep_subrects[0] = led2.subrects;
    st->postsep_core_evals[0] = led2.core_evals;
    st->postsep_parent_rects[0] = led2.parents; st->postsep_emitted_rects[0] = led2.emitted_rects;
    st->postsep_core_nodes[0] = led2.core_nodes; st->postsep_corner_evals[0] = led2.corner_evals;
    st->postsep_rollbacks[0] = led2.rollbacks; st->postsep_core_regressions[0] = led2.core_regressions;
    st->t_wspd_ms[0] += ms_since(t0);
    st->rect_alive[0] = alive.size();
    const auto t1 = std::chrono::steady_clock::now();
    run_lane(alive, opt.threads, 0, opt.cell_grid_min_sites, out, st, [&](const AliveRect& ar, AnchorScratch& sc, std::vector<BallCandidate>* lo, GenerateStats* ls) {
      corner_histograms(ix, Lane::kQ2, ar.r, &sc.ha, &sc.hb);
      const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
      const u64 need = h_of[0] > ar.core ? h_of[0] - ar.core : 0;
      const u64 nA = (u64)(ra.last - ra.first + 1), nB = (u64)(rb.last - rb.first + 1);
      ls->anchors[0] += nA * nB;  // le grand-livre reste ferme : toutes les paires sont comptees
      if (need == 0) { ls->anchors_killed_hist[0] += nA * nB; return; }
      u64 visitees = 0, tues_ligne = 0, tues_seuil = 0;
      for (i32 ua = ra.first; ua <= ra.last; ++ua) {
        const u64 ha_a = sc.ha[(size_t)(ua - ra.first)];
        if (ha_a >= need) { tues_ligne += nB; continue; }  // la ligne entiere meurt d un seul test
        for (i32 ub = rb.first; ub <= rb.last; ++ub) {
          if (ha_a + sc.hb[(size_t)(ub - rb.first)] >= need) { ++tues_seuil; continue; }
          ++visitees;
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (D2 == 0) continue;
          lo->push_back(BallCandidate{q2_ball_key(pa, pb), promote_level(q2_exact_level(D2)), 2});
          ++ls->candidates[0];
        }
      }
      // TROIS CLASSES DISJOINTES, dans l ordre demande par l audit 732529b3 :
      // lignes A fermees d abord, puis le seuil B sur les seules lignes restantes.
      ls->hist_killed_rows[0] += tues_ligne;
      ls->hist_killed_thresh[0] += tues_seuil;
      ls->hist_survivors[0] += visitees;
      ls->anchors_killed_hist[0] += tues_ligne + tues_seuil;
    });
    st->t_rects_ms[0] += ms_since(t1);
  }

  // ---- q3.
  if (opt.q3_override) {
    opt.q3_override(ix, opt, out, st);
  } else {
    const auto t0 = std::chrono::steady_clock::now();
    PostsepLedger led3;
    alive_rectangles(ix, opt.s, h_of, 1, opt.threads, &alive, &st->rect_visited[1], &st->workers_wspd[1],
                     opt.postsep_refine_levels, &led3, allow_subprofile);
    st->postsep_base_mass[1] = led3.base; st->postsep_emitted_mass[1] = led3.emitted;
    st->postsep_killed_mass[1] = led3.killed; st->postsep_subrects[1] = led3.subrects;
    st->postsep_core_evals[1] = led3.core_evals;
    st->postsep_parent_rects[1] = led3.parents; st->postsep_emitted_rects[1] = led3.emitted_rects;
    st->postsep_core_nodes[1] = led3.core_nodes; st->postsep_corner_evals[1] = led3.corner_evals;
    st->postsep_rollbacks[1] = led3.rollbacks; st->postsep_core_regressions[1] = led3.core_regressions;
    st->t_wspd_ms[1] += ms_since(t0);
    st->rect_alive[1] = alive.size();
    const auto t1 = std::chrono::steady_clock::now();
    run_lane(alive, opt.threads, 1, opt.cell_grid_min_sites, out, st, [&](const AliveRect& ar, AnchorScratch& sc, std::vector<BallCandidate>* lo, GenerateStats* ls) {
      corner_histograms(ix, Lane::kQ3, ar.r, &sc.ha, &sc.hb);
      const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
      rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
      sc.handle_points = 0;
      for (const NodeRef h : sc.handles) { const NodeRange r = ix.range_of(h); sc.handle_points += (u64)(r.last - r.first + 1); }
      const bool pretest_by_query = sc.handle_points >= opt.pretest_query_min_points;
      if (pretest_by_query) rect_diametral_candidates(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), &sc.query, &sc.cover_nodes);
      const u64 need = h_of[1] > ar.core ? h_of[1] - ar.core : 0;
      const u64 nA = (u64)(ra.last - ra.first + 1), nB = (u64)(rb.last - rb.first + 1);
      ls->anchors[1] += nA * nB;  // le grand-livre reste ferme : toutes les paires sont comptees
      if (need == 0) { ls->anchors_killed_hist[1] += nA * nB; return; }
      u64 visitees = 0, tues_ligne = 0, tues_seuil = 0;
      for (i32 ua = ra.first; ua <= ra.last; ++ua) {
        const u64 ha_a = sc.ha[(size_t)(ua - ra.first)];
        if (ha_a >= need) { tues_ligne += nB; continue; }  // la ligne entiere meurt d un seul test
        for (i32 ub = rb.first; ub <= rb.last; ++ub) {
          if (ha_a + sc.hb[(size_t)(ub - rb.first)] >= need) { ++tues_seuil; continue; }
          ++visitees;
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (D2 == 0) continue;
          // Credit d extremite deja acquis, transmis a la cascade aval.
          const EndpointCredit ec{ha_a + sc.hb[(size_t)(ub - rb.first)], ra.first, ra.last, rb.first, rb.last};
          if (pretest_by_query) {
            const int k = anchor_kill_from_candidates(sc.query, ix.upos, ua, ub, pa, pb, D2, Lane::kQ3, 12, h_of[1], &ec);
            if (k == 1) { ++ls->anchors_killed_w3; continue; }
            if (k == 2) { ++ls->anchors_killed_sectors[1]; continue; }
          }
          anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, 3, &sc.cover, &sc.visits, &sc.cover_tmp);
          configure_anchor_scan_cover(sc, opt.cover_envelope_filter, pretest_by_query);
          scan_anchor_q3(ix, sc, ua, ub, pa, pb, D2, h_of[1], float_on, genfilter_nonstrict, lo, ls,
                         pretest_by_query ? AnchorPretests::kAlreadyApplied : AnchorPretests::kApply, &ec);
        }
      }
      // TROIS CLASSES DISJOINTES, dans l ordre demande par l audit 732529b3 :
      // lignes A fermees d abord, puis le seuil B sur les seules lignes restantes.
      ls->hist_killed_rows[1] += tues_ligne;
      ls->hist_killed_thresh[1] += tues_seuil;
      ls->hist_survivors[1] += visitees;
      ls->anchors_killed_hist[1] += tues_ligne + tues_seuil;
    });
    st->t_rects_ms[1] += ms_since(t1);
  }

  // ---- q4.
  if (opt.q4_override) {
    opt.q4_override(ix, opt, out, st);
  } else {
    const auto t0 = std::chrono::steady_clock::now();
    // MUTANT q4-seeds-from-q3-live : la source q4 branchee sur la lane q3
    // (perd les ancres q3-mortes / q4-vivantes — fixture bloquante).
    PostsepLedger led4;
    alive_rectangles(ix, opt.s, h_of, q4_from_q3_live ? 1 : 2, opt.threads, &alive, &st->rect_visited[2],
                     &st->workers_wspd[2], opt.postsep_refine_levels, &led4, allow_subprofile);
    st->postsep_base_mass[2] = led4.base; st->postsep_emitted_mass[2] = led4.emitted;
    st->postsep_killed_mass[2] = led4.killed; st->postsep_subrects[2] = led4.subrects;
    st->postsep_core_evals[2] = led4.core_evals;
    st->postsep_parent_rects[2] = led4.parents; st->postsep_emitted_rects[2] = led4.emitted_rects;
    st->postsep_core_nodes[2] = led4.core_nodes; st->postsep_corner_evals[2] = led4.corner_evals;
    st->postsep_rollbacks[2] = led4.rollbacks; st->postsep_core_regressions[2] = led4.core_regressions;
    st->t_wspd_ms[2] += ms_since(t0);
    st->rect_alive[2] = alive.size();
    const auto t1 = std::chrono::steady_clock::now();
    run_lane(alive, opt.threads, 2, opt.cell_grid_min_sites, out, st, [&](const AliveRect& ar, AnchorScratch& sc, std::vector<BallCandidate>* lo, GenerateStats* ls) {
      corner_histograms(ix, Lane::kQ4, ar.r, &sc.ha, &sc.hb);
      const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
      rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), q4_cover_coef, &sc.handles, &sc.cover_nodes);
      sc.handle_points = 0;
      for (const NodeRef h : sc.handles) { const NodeRange r = ix.range_of(h); sc.handle_points += (u64)(r.last - r.first + 1); }
      const bool pretest_by_query = sc.handle_points >= opt.pretest_query_min_points;
      if (pretest_by_query) rect_diametral_candidates(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), &sc.query, &sc.cover_nodes);
      const u64 need = h_of[2] > ar.core ? h_of[2] - ar.core : 0;
      const u64 nA = (u64)(ra.last - ra.first + 1), nB = (u64)(rb.last - rb.first + 1);
      ls->anchors[2] += nA * nB;  // le grand-livre reste ferme : toutes les paires sont comptees
      if (need == 0) { ls->anchors_killed_hist[2] += nA * nB; return; }
      u64 visitees = 0, tues_ligne = 0, tues_seuil = 0;
      for (i32 ua = ra.first; ua <= ra.last; ++ua) {
        const u64 ha_a = sc.ha[(size_t)(ua - ra.first)];
        if (ha_a >= need) { tues_ligne += nB; continue; }  // la ligne entiere meurt d un seul test
        for (i32 ub = rb.first; ub <= rb.last; ++ub) {
          if (ha_a + sc.hb[(size_t)(ub - rb.first)] >= need) { ++tues_seuil; continue; }
          ++visitees;
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (D2 == 0) continue;
          if (pretest_by_query) {
            const int k = anchor_kill_from_candidates(sc.query, ix.upos, ua, ub, pa, pb, D2, Lane::kQ4, 8, h_of[2]);
            if (k == 1) { ++ls->anchors_killed_w4; continue; }
            if (k == 2) { ++ls->anchors_killed_sectors[2]; continue; }
          }
          // Cover au coefficient 3 (la lentille des sommets). Les interieurs
          // et la coquille d'une boule q4 vivent dans le coefficient 4 (Jung),
          // mais ici le cover n'est qu'un SOUS-ENSEMBLE pour des minorants
          // fail-open (W_4, cœur de seed, profondeur a la generation) : le
          // census exact passe par l'arbre entier. Le coefficient 3 est celui
          // de la v4 ; un coefficient 4 tuerait quelques boules profondes de
          // plus avant l'emission (23 sur 1,4 M a uniform n=8000) sans changer
          // l'objet — mais changerait `digest_balls`, qui compte les candidats
          // profonds (docs/PROVENANCE.md, conformite).
          const u64 cover_visits_before = sc.visits;
          anchor_cover_from_handles(ix, sc.handles, pa, pb, D2, q4_cover_coef, &sc.cover, &sc.visits, &sc.cover_tmp);
          MHGP5_Q4_ADD(q4_covers_built, 1);
          MHGP5_Q4_ADD(q4_cover_visits, sc.visits - cover_visits_before);
          MHGP5_Q4_ADD(q4_cover_sites, sc.cover.size());
          configure_anchor_scan_cover(sc, opt.cover_envelope_filter, pretest_by_query);
          process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h_of[2], float_on, genfilter_nonstrict, seed_core_nonstrict,
                            no_canonical, lo, ls, pretest_by_query ? AnchorPretests::kAlreadyApplied : AnchorPretests::kApply);
        }
      }
      // TROIS CLASSES DISJOINTES, dans l ordre demande par l audit 732529b3 :
      // lignes A fermees d abord, puis le seuil B sur les seules lignes restantes.
      ls->hist_killed_rows[2] += tues_ligne;
      ls->hist_killed_thresh[2] += tues_seuil;
      ls->hist_survivors[2] += visitees;
      ls->anchors_killed_hist[2] += tues_ligne + tues_seuil;
    });
    st->t_rects_ms[2] += ms_since(t1);
  }
}

}  // namespace mhgp5
