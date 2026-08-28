// MorseHGP3D v5 — porte de la lane q4 PAR LOTS (src/gpu/q4_lane_batched.hpp)
// contre la lane q4 de production (generate_candidates, arite 4) : egalite
// post-RLE des candidats (ordre brut a un fil) et de VINGT-DEUX compteurs de
// la lane q4 (seize exiges non nuls : vacuite refusee, code 3). Planchers : --min-candidates, --min-deep. Mutant de porte
// `q4-batched-emit-deep` : code 4. Codes : 0, 2, 3, 4.
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/q4_lane_batched.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 400, coord = 0, threads = 1;
  u32 postsep = 0;
  BatchLimits lim;
  u64 min_flushes = 1;
  std::string expect_route = "device";  // device | mixed | host : contrat de NON-VACUITE des routes
  BatchStats bs;
  u64 min_candidates = 1000, min_deep = 100;
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
    else if (arg.rfind("--min-deep=", 0) == 0) min_deep = (u64)std::atoll(arg.c_str() + 11);
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
    } else if (arg.rfind("--pairs-per-launch=", 0) == 0) {
      const long long v = std::atoll(arg.c_str() + 19);
      if (v < 1) return 2;
      lim.pairs = (size_t)v;
    } else if (arg.rfind("--min-flushes=", 0) == 0) min_flushes = (u64)std::atoll(arg.c_str() + 14);
    else if (arg.rfind("--expect-route=", 0) == 0) expect_route = arg.substr(15);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool mutant = MHGP5_MUTANT("q4-batched-emit-deep");
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  GenerateOptions opt;
  opt.threads = threads;
  opt.postsep_refine_levels = postsep;
  std::vector<BallCandidate> prod_all, prod, batched;
  GenerateStats sp, sb;
  generate_candidates(ix, opt, &prod_all, &sp);
  for (const BallCandidate& c : prod_all)
    if (c.arity == 4) prod.push_back(c);
  generate_q4_batched(ix, opt, &batched, &sb, lim, &bs);
  u64 bad = 0;
  auto cmp = [&](const char* what, u64 a, u64 b) {
    if (a != b) {
      std::printf("desaccord %s : production=%llu lots=%llu\n", what, (unsigned long long)a, (unsigned long long)b);
      ++bad;
    }
  };
  cmp("rect_alive", sp.rect_alive[2], sb.rect_alive[2]);
  cmp("postsep_base_mass", sp.postsep_base_mass[2], sb.postsep_base_mass[2]);
  cmp("postsep_emitted_mass", sp.postsep_emitted_mass[2], sb.postsep_emitted_mass[2]);
  cmp("postsep_killed_mass", sp.postsep_killed_mass[2], sb.postsep_killed_mass[2]);
  cmp("postsep_parent_rects", sp.postsep_parent_rects[2], sb.postsep_parent_rects[2]);
  cmp("postsep_emitted_rects", sp.postsep_emitted_rects[2], sb.postsep_emitted_rects[2]);
  cmp("postsep_subrects", sp.postsep_subrects[2], sb.postsep_subrects[2]);
  cmp("postsep_core_evals", sp.postsep_core_evals[2], sb.postsep_core_evals[2]);
  cmp("postsep_core_nodes", sp.postsep_core_nodes[2], sb.postsep_core_nodes[2]);
  cmp("postsep_corner_evals", sp.postsep_corner_evals[2], sb.postsep_corner_evals[2]);
  cmp("postsep_rollbacks", sp.postsep_rollbacks[2], sb.postsep_rollbacks[2]);
  cmp("postsep_core_regressions", sp.postsep_core_regressions[2], sb.postsep_core_regressions[2]);
  cmp("anchors", sp.anchors[2], sb.anchors[2]);
  cmp("anchors_killed_hist", sp.anchors_killed_hist[2], sb.anchors_killed_hist[2]);
  cmp("anchors_killed_w4", sp.anchors_killed_w4, sb.anchors_killed_w4);
  cmp("anchors_killed_sectors", sp.anchors_killed_sectors[2], sb.anchors_killed_sectors[2]);
  cmp("anchors_killed_cells", sp.anchors_killed_cells[2], sb.anchors_killed_cells[2]);
  cmp("seeds_killed_cells", sp.seeds_killed_cells[2], sb.seeds_killed_cells[2]);
  // Grilles : tentees (politique), construites (build reussi), toutes mortes — les lanes par lots ne construisent
  // JAMAIS une grille deux fois (jeton kAlreadyAppliedWithGrid sur la route trop grande).
  cmp("grids_attempted", sp.grids_attempted[2], sb.grids_attempted[2]);
  cmp("grids_built", sp.grids_built[2], sb.grids_built[2]);
  cmp("grids_all_dead", sp.grids_all_dead[2], sb.grids_all_dead[2]);
  if (sp.grids_all_dead[2] != sp.anchors_killed_cells[2] || sp.grids_built[2] > sp.grids_attempted[2]) {
    std::printf("INVARIANT : grilles toutes mortes %llu != ancres tuees %llu, ou construites %llu > tentees %llu\n",
                (unsigned long long)sp.grids_all_dead[2], (unsigned long long)sp.anchors_killed_cells[2], (unsigned long long)sp.grids_built[2],
                (unsigned long long)sp.grids_attempted[2]);
    return 3;
  }
  std::printf("grilles q4 : tentees=%llu construites=%llu toutes_mortes=%llu seeds_tues=%llu\n", (unsigned long long)sp.grids_attempted[2],
              (unsigned long long)sp.grids_built[2], (unsigned long long)sp.grids_all_dead[2], (unsigned long long)sp.seeds_killed_cells[2]);
  cmp("seeds", sp.seeds[1], sb.seeds[1]);
  cmp("seeds_killed_core", sp.seeds_killed_core, sb.seeds_killed_core);
  cmp("seeds_killed_chord", sp.seeds_killed_chord, sb.seeds_killed_chord);
  cmp("q4_completions", sp.q4_completions, sb.q4_completions);
  cmp("q4_rej_lens", sp.q4_rej_lens, sb.q4_rej_lens);
  cmp("q4_rej_owner", sp.q4_rej_owner, sb.q4_rej_owner);
  cmp("q4_rej_once", sp.q4_rej_once, sb.q4_rej_once);
  cmp("q4_rej_i64", sp.q4_rej_i64, sb.q4_rej_i64);
  cmp("q4_rej_face_power", sp.q4_rej_face_power, sb.q4_rej_face_power);
  cmp("q4_rej_det", sp.q4_rej_det, sb.q4_rej_det);
  cmp("q4_rej_center", sp.q4_rej_center, sb.q4_rej_center);
  cmp("depth_killed", sp.depth_killed[2], sb.depth_killed[2]);
  cmp("candidates", sp.candidates[2], sb.candidates[2]);
  const char* cn[6] = {"q4_cert_pos", "q4_cert_neg", "q4_jung_kill", "q4_jung_skip", "q4_jung_fallback", "q4_float_fallback"};
  for (int i = 0; i < 6; ++i) cmp(cn[i], sp.q4_cert[i], sb.q4_cert[i]);
  // VACUITE : les vingt-deux compteurs compares sont imprimes ; seize d'entre
  // eux doivent etre NON NULS sur toute famille exercee (une egalite 0 = 0 ne
  // prouve rien) — det et jung_fallback peuvent etre nuls en position generale
  // et restent exiges par la seule famille cocirculaire (--min-deep, etc.).
  const u64 must[16] = {sp.anchors[2], sp.anchors_killed_hist[2], sp.anchors_killed_w4, sp.seeds[1], sp.seeds_killed_core,
                        sp.q4_completions, sp.q4_rej_lens, sp.q4_rej_owner, sp.q4_rej_once, sp.q4_rej_i64,
                        sp.q4_rej_face_power, sp.q4_rej_center, sp.q4_cert[0], sp.q4_cert[1], sp.q4_cert[2], sp.q4_cert[3]};
  const char* must_n[16] = {"anchors", "anchors_killed_hist", "anchors_killed_w4", "seeds", "seeds_killed_core", "completions",
                            "rej_lens", "rej_owner", "rej_once", "rej_i64", "rej_face_power", "rej_center", "cert_pos",
                            "cert_neg", "jung_kill", "jung_skip"};
  u64 vacuous = 0;
  if (sp.anchors_killed_sectors[2] == 0) { std::printf("VACUITE : secteurs q4 = 0\n"); ++vacuous; }
  if (sp.seeds_killed_chord == 0) { std::printf("VACUITE : morceaux de corde q4 = 0\n"); ++vacuous; }
  for (int i = 0; i < 16; ++i)
    if (must[i] == 0) { std::printf("VACUITE : %s = 0\n", must_n[i]); ++vacuous; }
  std::printf("compteurs anchors=%llu hist=%llu w4=%llu seeds=%llu core=%llu compl=%llu lens=%llu owner=%llu once=%llu i64=%llu "
              "face=%llu det=%llu centre=%llu profond=%llu cand=%llu cpos=%llu cneg=%llu jkill=%llu jskip=%llu jfb=%llu ffb=%llu\n",
              (unsigned long long)sp.anchors[2], (unsigned long long)sp.anchors_killed_hist[2], (unsigned long long)sp.anchors_killed_w4,
              (unsigned long long)sp.seeds[1], (unsigned long long)sp.seeds_killed_core, (unsigned long long)sp.q4_completions,
              (unsigned long long)sp.q4_rej_lens, (unsigned long long)sp.q4_rej_owner, (unsigned long long)sp.q4_rej_once,
              (unsigned long long)sp.q4_rej_i64, (unsigned long long)sp.q4_rej_face_power, (unsigned long long)sp.q4_rej_det,
              (unsigned long long)sp.q4_rej_center, (unsigned long long)sp.depth_killed[2], (unsigned long long)sp.candidates[2],
              (unsigned long long)sp.q4_cert[0], (unsigned long long)sp.q4_cert[1], (unsigned long long)sp.q4_cert[2],
              (unsigned long long)sp.q4_cert[3], (unsigned long long)sp.q4_cert[4], (unsigned long long)sp.q4_cert[5]);
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
  std::printf("q4_lane_batched famille=%s n=%d fils=%d postsep=%u routage_min_sites=%zu ancres_device=%llu ancres_hote=%llu ancres_trop_grandes=%llu seeds_device=%llu seeds_hote=%llu seuil_seeds=%zu seuil_sites=%zu vidages=%llu max_lot_seeds=%llu max_ancre_seeds=%llu max_lot_sites=%llu max_ancre_sites=%llu max_lot_paires=%llu max_ancre_paires=%llu candidats_q4=%zu candidats_lots=%zu seeds=%llu coeur_tues=%llu completions=%llu "
              "profonds=%llu desaccords_vecteur=%llu desaccords_compteurs=%llu\n",
              cloud_family_name(family), n, threads, postsep, lim.device_min_sites, (unsigned long long)bs.anchors_device, (unsigned long long)bs.anchors_host, (unsigned long long)bs.anchors_oversized,
              (unsigned long long)bs.seeds_device, (unsigned long long)bs.seeds_host, lim.seeds, lim.sites, (unsigned long long)bs.flushes, (unsigned long long)bs.max_lot_seeds,
              (unsigned long long)bs.max_anchor_seeds, (unsigned long long)bs.max_lot_sites, (unsigned long long)bs.max_anchor_sites, (unsigned long long)bs.max_lot_pairs,
              (unsigned long long)bs.max_anchor_pairs, prod.size(), batched.size(), (unsigned long long)sp.seeds[1],
              (unsigned long long)sp.seeds_killed_core, (unsigned long long)sp.q4_completions,
              (unsigned long long)sp.depth_killed[2], (unsigned long long)vec_mism, (unsigned long long)bad);
  const bool ledger_ok = (u128)sp.postsep_emitted_mass[2] + sp.postsep_killed_mass[2] == sp.postsep_base_mass[2] &&
                         (u128)sb.postsep_emitted_mass[2] + sb.postsep_killed_mass[2] == sb.postsep_base_mass[2] &&
                         sp.postsep_core_regressions[2] == 0 && sb.postsep_core_regressions[2] == 0;
  if (!ledger_ok || (postsep > 0 && sp.postsep_core_evals[2] == 0)) {
    std::printf("POSTSEP : grand-livre, monotonie ou non-vacuite hors contrat\n");
    return 3;
  }
  if (sp.candidates[2] < min_candidates || sp.depth_killed[2] < min_deep || vacuous) {
    std::printf("PLANCHER\n");
    return 3;
  }
  // Contrat de lotissement : borne dure seuil + plus grosse ancre ; nombre de
  // vidages au moins min_flushes (un code ignorant le seuil resterait vert sinon).
  // Contrat de lotissement : BORNE DURE = les seuils eux-memes (preflight :
  // vidage avant l'ajout qui depasserait ; ancre trop grande -> corps hote).
  if (bs.max_lot_seeds > (u64)lim.seeds || bs.max_lot_sites > (u64)lim.sites || bs.max_lot_pairs > (u64)lim.pairs || bs.flushes < min_flushes) {
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
  if (route_mut) {
    std::printf("MUTANT NON TUE (route-ignore-threshold)\n");
    return 1;
  }
  // Mesure (jamais un claim) : temps des rectangles de la lane, production vs lots.
  std::printf("temps_prod_ms=%.1f temps_lots_ms=%.1f (temps MURAL des rectangles de la lane, tous ouvriers confondus)\n", sp.t_rects_ms[2], sb.t_rects_ms[2]);
  if (vec_mism || bad) return mutant ? 4 : 1;
  if (mutant) { std::printf("MUTANT NON TUE\n"); return 1; }
  return 0;
}
