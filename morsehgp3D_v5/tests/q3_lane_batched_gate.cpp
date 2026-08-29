// MorseHGP3D v5 — porte de la lane q3 PAR LOTS (src/gpu/q3_lane_batched.hpp)
// contre la lane q3 de production (generate_candidates, candidats d'arite 3) :
// egalite VECTEUR A VECTEUR (cle, niveau, ordre d'emission) et egalite des
// compteurs (ancres, ancres tuees par histogramme, seeds, seeds tues par
// profondeur, candidats, certifications flottantes, replis). Planchers :
// --min-candidates, --min-killed (des seeds doivent mourir), --min-fallback.
// L'ordre brut d'emission est compare a un fil ; a plusieurs fils, l'ordre
// des rectangles entre ouvriers est dynamique et seule l'egalite post-RLE
// (triee, dedoublonnee — l'objet consomme par le pipeline) est exigee.
// Mutant de porte `q3-batched-emit-dead` : code 4. Codes : 0, 2, 3, 4.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/q3_lane_batched.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 400, coord = 0, threads = 1;
  u32 postsep = 0;
  BatchLimits lim;
  u64 min_flushes = 1, min_oversized = 0;
  std::string expect_route = "device";  // device | mixed | host : contrat de NON-VACUITE des routes
  bool scan_noop = false, cover_envelope = false;
  BatchStats bs;
  u64 min_candidates = 1000, min_killed = 10, min_fallback = 10;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) { if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2; }
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--coord=", 0) == 0) coord = std::atoi(arg.c_str() + 8);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else if (arg.rfind("--postsep=", 0) == 0) {
      const char* v = arg.c_str() + 10;
      if (v[0] < '0' || v[0] > '3' || v[1] != '\0') return 2;
      postsep = (u32)(v[0] - '0');
    }
    else if (arg.rfind("--min-candidates=", 0) == 0) min_candidates = (u64)std::atoll(arg.c_str() + 17);
    else if (arg.rfind("--min-killed=", 0) == 0) min_killed = (u64)std::atoll(arg.c_str() + 13);
    else if (arg.rfind("--min-fallback=", 0) == 0) min_fallback = (u64)std::atoll(arg.c_str() + 15);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else if (arg.rfind("--seeds-per-launch=", 0) == 0) {
      const long long v = std::atoll(arg.c_str() + 19);
      if (v < 1) return 2;  // contrat : seuil >= 1
      lim.seeds = (size_t)v;
    } else if (arg.rfind("--device-min-sites=", 0) == 0) {
      const long long v = std::atoll(arg.c_str() + 19);
      if (v < 1) return 2;
      lim.device_min_sites = (size_t)v;
    } else if (arg.rfind("--sites-per-launch=", 0) == 0) {
      const long long v = std::atoll(arg.c_str() + 19);
      if (v < 1) return 2;
      lim.sites = (size_t)v;
    } else if (arg.rfind("--min-flushes=", 0) == 0) min_flushes = (u64)std::atoll(arg.c_str() + 14);
    else if (arg.rfind("--min-oversized=", 0) == 0) min_oversized = (u64)std::atoll(arg.c_str() + 16);
    else if (arg.rfind("--expect-route=", 0) == 0) expect_route = arg.substr(15);
    else if (arg == "--cover-envelope=0") cover_envelope = false;
    else if (arg == "--cover-envelope=1") cover_envelope = true;
    else if (arg == "--scan-noop") scan_noop = true;  // MESURE seulement : executeur vide (tout vivant) — desaccords attendus, code 1
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool mutant = MHGP5_MUTANT("q3-batched-emit-dead");
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  GenerateOptions opt;
  opt.threads = threads;
  opt.postsep_refine_levels = postsep;
  opt.cover_envelope_filter = cover_envelope;
  // Production : toutes les lanes, on garde l'arite 3 (ordre conserve).
  std::vector<BallCandidate> prod_all, prod, batched;
  GenerateStats sp, sb;
  generate_candidates(ix, opt, &prod_all, &sp);
  for (const BallCandidate& c : prod_all)
    if (c.arity == 3) prod.push_back(c);
  if (scan_noop)
    generate_q3_batched_with(ix, opt, &batched, &sb, [](Q3Batch* b, u32, bool) { b->verdicts.assign(b->seeds.size(), Q3BatchVerdict{}); }, lim, &bs);
  else
    generate_q3_batched(ix, opt, &batched, &sb, lim, &bs);
  // Compteurs de la lane q3 seule : la production cumule q2/q4 aussi, on ne
  // compare que les champs de la lane q3 ; les certifications flottantes de la
  // production incluent q4 (jung_*) separement, float_* est q3 seulement.
  u64 bad = 0;
  auto cmp = [&](const char* what, u64 a, u64 b) {
    if (a != b) {
      std::printf("desaccord %s : production=%llu lots=%llu\n", what, (unsigned long long)a, (unsigned long long)b);
      ++bad;
    }
  };
  cmp("rect_alive", sp.rect_alive[1], sb.rect_alive[1]);
  cmp("postsep_base_mass", sp.postsep_base_mass[1], sb.postsep_base_mass[1]);
  cmp("postsep_emitted_mass", sp.postsep_emitted_mass[1], sb.postsep_emitted_mass[1]);
  cmp("postsep_killed_mass", sp.postsep_killed_mass[1], sb.postsep_killed_mass[1]);
  cmp("postsep_parent_rects", sp.postsep_parent_rects[1], sb.postsep_parent_rects[1]);
  cmp("postsep_emitted_rects", sp.postsep_emitted_rects[1], sb.postsep_emitted_rects[1]);
  cmp("postsep_subrects", sp.postsep_subrects[1], sb.postsep_subrects[1]);
  cmp("postsep_core_evals", sp.postsep_core_evals[1], sb.postsep_core_evals[1]);
  cmp("postsep_core_nodes", sp.postsep_core_nodes[1], sb.postsep_core_nodes[1]);
  cmp("postsep_corner_evals", sp.postsep_corner_evals[1], sb.postsep_corner_evals[1]);
  cmp("postsep_rollbacks", sp.postsep_rollbacks[1], sb.postsep_rollbacks[1]);
  cmp("postsep_core_regressions", sp.postsep_core_regressions[1], sb.postsep_core_regressions[1]);
  cmp("anchors", sp.anchors[1], sb.anchors[1]);
  cmp("anchors_killed_hist", sp.anchors_killed_hist[1], sb.anchors_killed_hist[1]);
  cmp("anchors_killed_w3", sp.anchors_killed_w3, sb.anchors_killed_w3);
  cmp("anchors_killed_sectors", sp.anchors_killed_sectors[1], sb.anchors_killed_sectors[1]);
  cmp("anchors_killed_cells", sp.anchors_killed_cells[1], sb.anchors_killed_cells[1]);
  cmp("seeds_killed_cells", sp.seeds_killed_cells[1], sb.seeds_killed_cells[1]);
  // Grilles : tentees (politique), construites (build reussi), toutes mortes — les lanes par lots ne construisent
  // JAMAIS une grille deux fois (jeton kAlreadyAppliedWithGrid sur les routes hote et trop grande).
  cmp("grids_attempted", sp.grids_attempted[1], sb.grids_attempted[1]);
  cmp("grids_built", sp.grids_built[1], sb.grids_built[1]);
  cmp("grids_all_dead", sp.grids_all_dead[1], sb.grids_all_dead[1]);
  if (sp.grids_all_dead[1] != sp.anchors_killed_cells[1] || sp.grids_built[1] > sp.grids_attempted[1]) {
    std::printf("INVARIANT : grilles toutes mortes %llu != ancres tuees %llu, ou construites %llu > tentees %llu\n",
                (unsigned long long)sp.grids_all_dead[1], (unsigned long long)sp.anchors_killed_cells[1], (unsigned long long)sp.grids_built[1],
                (unsigned long long)sp.grids_attempted[1]);
    return 3;
  }
  std::printf("grilles q3 : tentees=%llu construites=%llu toutes_mortes=%llu seeds_tues=%llu\n", (unsigned long long)sp.grids_attempted[1],
              (unsigned long long)sp.grids_built[1], (unsigned long long)sp.grids_all_dead[1], (unsigned long long)sp.seeds_killed_cells[1]);
  cmp("seeds", sp.seeds[0], sb.seeds[0]);
  cmp("depth_killed", sp.depth_killed[1], sb.depth_killed[1]);
  cmp("candidates", sp.candidates[1], sb.candidates[1]);
  cmp("q3_cert_neg", sp.q3_cert[0], sb.q3_cert[0]);
  cmp("q3_cert_pos", sp.q3_cert[1], sb.q3_cert[1]);
  cmp("q3_fallback", sp.q3_cert[2], sb.q3_cert[2]);
  cmp("envelope_anchors_cover", sp.edge_envelope_anchors[1][0], sb.edge_envelope_anchors[1][0]);
  cmp("envelope_anchors_query", sp.edge_envelope_anchors[1][1], sb.edge_envelope_anchors[1][1]);
  cmp("envelope_before_cover", sp.edge_envelope_sites_before[1][0], sb.edge_envelope_sites_before[1][0]);
  cmp("envelope_before_query", sp.edge_envelope_sites_before[1][1], sb.edge_envelope_sites_before[1][1]);
  cmp("envelope_after_cover", sp.edge_envelope_sites_after[1][0], sb.edge_envelope_sites_after[1][0]);
  cmp("envelope_after_query", sp.edge_envelope_sites_after[1][1], sb.edge_envelope_sites_after[1][1]);
  cmp("envelope_cross_cover", sp.edge_envelope_cross_tests[1][0], sb.edge_envelope_cross_tests[1][0]);
  cmp("envelope_cross_query", sp.edge_envelope_cross_tests[1][1], sb.edge_envelope_cross_tests[1][1]);
  // Ordre brut : identique a un fil (ordonnancement dynamique des rectangles
  // entre ouvriers sinon) ; post-RLE : identique a tout nombre de fils.
  auto count_mism = [](const std::vector<BallCandidate>& a, const std::vector<BallCandidate>& b) {
    u64 m = a.size() != b.size() ? 1 : 0;
    for (size_t i = 0; i < a.size() && i < b.size(); ++i)
      if (!(a[i].key == b[i].key) || !(a[i].level == b[i].level) || a[i].arity != b[i].arity) ++m;
    return m;
  };
  u64 vec_mism = (threads == 1 && lim.device_min_sites == 1) ? count_mism(prod, batched) : 0;
  rle_candidates(&prod, 1);
  rle_candidates(&batched, 1);
  vec_mism += count_mism(prod, batched);
  std::printf("famille=%s n=%d fils=%d postsep=%u routage_min_sites=%zu ancres_device=%llu ancres_hote=%llu ancres_trop_grandes=%llu seeds_device=%llu seeds_hote=%llu seuil_seeds=%zu seuil_sites=%zu vidages=%llu max_lot_seeds=%llu max_ancre_seeds=%llu max_lot_sites=%llu max_ancre_sites=%llu candidats_q3=%zu candidats_lots=%zu seeds=%llu tues=%llu replis=%llu desaccords_vecteur=%llu desaccords_compteurs=%llu\n",
              cloud_family_name(family), n, threads, postsep, lim.device_min_sites, (unsigned long long)bs.anchors_device, (unsigned long long)bs.anchors_host, (unsigned long long)bs.anchors_oversized,
              (unsigned long long)bs.seeds_device, (unsigned long long)bs.seeds_host, lim.seeds, lim.sites, (unsigned long long)bs.flushes, (unsigned long long)bs.max_lot_seeds,
              (unsigned long long)bs.max_anchor_seeds, (unsigned long long)bs.max_lot_sites, (unsigned long long)bs.max_anchor_sites, prod.size(), batched.size(), (unsigned long long)sp.seeds[0],
              (unsigned long long)sp.depth_killed[1], (unsigned long long)sp.q3_cert[2], (unsigned long long)vec_mism,
              (unsigned long long)bad);
  const bool ledger_ok = (u128)sp.postsep_emitted_mass[1] + sp.postsep_killed_mass[1] == sp.postsep_base_mass[1] &&
                         (u128)sb.postsep_emitted_mass[1] + sb.postsep_killed_mass[1] == sb.postsep_base_mass[1] &&
                         sp.postsep_core_regressions[1] == 0 && sb.postsep_core_regressions[1] == 0;
  if (!ledger_ok || (postsep > 0 && sp.postsep_core_evals[1] == 0)) {
    std::printf("POSTSEP : grand-livre, monotonie ou non-vacuite hors contrat\n");
    return 3;
  }
  // Non-vacuite des tests d'ancre (V7.3) : W_3 exact ET secteurs doivent chacun tuer au moins une ancre.
  if (sp.anchors_killed_w3 < 1 || sp.anchors_killed_sectors[1] < 1) {
    std::printf("VACUITE : tests d'ancre (W3 %llu, secteurs %llu)\n", (unsigned long long)sp.anchors_killed_w3, (unsigned long long)sp.anchors_killed_sectors[1]);
    return 3;
  }
  if (sp.candidates[1] < min_candidates || sp.depth_killed[1] < min_killed || sp.q3_cert[2] < min_fallback) {
    std::printf("PLANCHER : candidats %llu < %llu ou tues %llu < %llu ou replis %llu < %llu\n", (unsigned long long)sp.candidates[1],
                (unsigned long long)min_candidates, (unsigned long long)sp.depth_killed[1], (unsigned long long)min_killed,
                (unsigned long long)sp.q3_cert[2], (unsigned long long)min_fallback);
    return 3;
  }
  if (cover_envelope) {
    const u64 anchors = sp.edge_envelope_anchors[1][0] + sp.edge_envelope_anchors[1][1];
    const u64 before = sp.edge_envelope_sites_before[1][0] + sp.edge_envelope_sites_before[1][1];
    const u64 after = sp.edge_envelope_sites_after[1][0] + sp.edge_envelope_sites_after[1][1];
    if (anchors == 0 || before <= after) {
      std::printf("VACUITE : enveloppe q3 ancres=%llu sites=%llu->%llu\n", (unsigned long long)anchors,
                  (unsigned long long)before, (unsigned long long)after);
      return 3;
    }
  }
  // Contrat de lotissement : borne dure seuil + plus grosse ancre ; nombre de
  // vidages au moins min_flushes (un code ignorant le seuil resterait vert sinon).
  // Contrat de lotissement : BORNE DURE = les seuils eux-memes (preflight :
  // vidage avant l'ajout qui depasserait ; ancre trop grande -> corps hote).
  if (bs.max_lot_seeds > (u64)lim.seeds || bs.max_lot_sites > (u64)lim.sites || bs.flushes < min_flushes) {
    std::printf("LOTISSEMENT : borne (seeds %llu/%zu, sites %llu/%zu) ou vidages %llu < %llu hors contrat\n",
                (unsigned long long)bs.max_lot_seeds, lim.seeds, (unsigned long long)bs.max_lot_sites, lim.sites,
                (unsigned long long)bs.flushes, (unsigned long long)min_flushes);
    return 1;
  }
  // Contrat de NON-VACUITE des routes (mutant route-ignore-threshold : tout au
  // device -> seeds_hote = 0 -> code 4 sur une porte mixte).
  const bool route_mut = MHGP5_MUTANT("route-ignore-threshold");
  bool route_ok = true;
  if (expect_route == "mixed") route_ok = bs.seeds_host > 0 && bs.seeds_device > 0 && bs.anchors_device > 0;
  else if (expect_route == "host") route_ok = bs.seeds_device == 0 && bs.anchors_device == 0 && bs.seeds_host > 0;
  else if (expect_route == "device") route_ok = bs.seeds_device > 0 && bs.anchors_device > 0 && (bs.seeds_host == 0 || bs.anchors_oversized > 0);
  else return 2;
  if (!route_ok) {
    std::printf("ROUTAGE : contrat '%s' viole (seeds device %llu, hote %llu ; ancres device %llu, hote %llu, trop grandes %llu)\n",
                expect_route.c_str(), (unsigned long long)bs.seeds_device, (unsigned long long)bs.seeds_host,
                (unsigned long long)bs.anchors_device, (unsigned long long)bs.anchors_host, (unsigned long long)bs.anchors_oversized);
    return route_mut ? 4 : 1;
  }
  if (bs.anchors_oversized < min_oversized) {
    std::printf("ROUTAGE : ancres trop grandes %llu < plancher %llu\n",
                (unsigned long long)bs.anchors_oversized, (unsigned long long)min_oversized);
    return 3;
  }
  if (route_mut) {
    std::printf("MUTANT NON TUE (route-ignore-threshold)\n");
    return 1;
  }
  // Mesure (jamais un claim) : temps des rectangles de la lane, production vs lots.
  std::printf("temps_prod_ms=%.1f temps_lots_ms=%.1f (temps MURAL des rectangles de la lane, tous ouvriers confondus)\n", sp.t_rects_ms[1], sb.t_rects_ms[1]);
  if (vec_mism || bad) return mutant ? 4 : 1;
  if (mutant) {
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  return 0;
}
