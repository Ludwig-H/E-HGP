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
  BatchLimits lim;
  u64 min_flushes = 1;
  BatchStats bs;
  u64 min_candidates = 1000, min_killed = 10, min_fallback = 10;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) { if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2; }
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--coord=", 0) == 0) coord = std::atoi(arg.c_str() + 8);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else if (arg.rfind("--min-candidates=", 0) == 0) min_candidates = (u64)std::atoll(arg.c_str() + 17);
    else if (arg.rfind("--min-killed=", 0) == 0) min_killed = (u64)std::atoll(arg.c_str() + 13);
    else if (arg.rfind("--min-fallback=", 0) == 0) min_fallback = (u64)std::atoll(arg.c_str() + 15);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else if (arg.rfind("--seeds-per-launch=", 0) == 0) {
      const long long v = std::atoll(arg.c_str() + 19);
      if (v < 1) return 2;  // contrat : seuil >= 1
      lim.seeds = (size_t)v;
    } else if (arg.rfind("--sites-per-launch=", 0) == 0) {
      const long long v = std::atoll(arg.c_str() + 19);
      if (v < 1) return 2;
      lim.sites = (size_t)v;
    } else if (arg.rfind("--min-flushes=", 0) == 0) min_flushes = (u64)std::atoll(arg.c_str() + 14);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool mutant = MHGP5_MUTANT("q3-batched-emit-dead");
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  GenerateOptions opt;
  opt.threads = threads;
  // Production : toutes les lanes, on garde l'arite 3 (ordre conserve).
  std::vector<BallCandidate> prod_all, prod, batched;
  GenerateStats sp, sb;
  generate_candidates(ix, opt, &prod_all, &sp);
  for (const BallCandidate& c : prod_all)
    if (c.arity == 3) prod.push_back(c);
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
  cmp("anchors", sp.anchors[1], sb.anchors[1]);
  cmp("anchors_killed_hist", sp.anchors_killed_hist[1], sb.anchors_killed_hist[1]);
  cmp("seeds", sp.seeds[0], sb.seeds[0]);
  cmp("depth_killed", sp.depth_killed[1], sb.depth_killed[1]);
  cmp("candidates", sp.candidates[1], sb.candidates[1]);
  cmp("q3_cert_neg", sp.q3_cert[0], sb.q3_cert[0]);
  cmp("q3_cert_pos", sp.q3_cert[1], sb.q3_cert[1]);
  cmp("q3_fallback", sp.q3_cert[2], sb.q3_cert[2]);
  // Ordre brut : identique a un fil (ordonnancement dynamique des rectangles
  // entre ouvriers sinon) ; post-RLE : identique a tout nombre de fils.
  auto count_mism = [](const std::vector<BallCandidate>& a, const std::vector<BallCandidate>& b) {
    u64 m = a.size() != b.size() ? 1 : 0;
    for (size_t i = 0; i < a.size() && i < b.size(); ++i)
      if (!(a[i].key == b[i].key) || !(a[i].level == b[i].level) || a[i].arity != b[i].arity) ++m;
    return m;
  };
  u64 vec_mism = threads == 1 ? count_mism(prod, batched) : 0;
  rle_candidates(&prod, 1);
  rle_candidates(&batched, 1);
  vec_mism += count_mism(prod, batched);
  std::printf("famille=%s n=%d fils=%d seuil_seeds=%zu seuil_sites=%zu vidages=%llu max_lot_seeds=%llu max_ancre_seeds=%llu max_lot_sites=%llu max_ancre_sites=%llu candidats_q3=%zu candidats_lots=%zu seeds=%llu tues=%llu replis=%llu desaccords_vecteur=%llu desaccords_compteurs=%llu\n",
              cloud_family_name(family), n, threads, lim.seeds, lim.sites, (unsigned long long)bs.flushes, (unsigned long long)bs.max_lot_seeds,
              (unsigned long long)bs.max_anchor_seeds, (unsigned long long)bs.max_lot_sites, (unsigned long long)bs.max_anchor_sites, prod.size(), batched.size(), (unsigned long long)sp.seeds[0],
              (unsigned long long)sp.depth_killed[1], (unsigned long long)sp.q3_cert[2], (unsigned long long)vec_mism,
              (unsigned long long)bad);
  if (sp.candidates[1] < min_candidates || sp.depth_killed[1] < min_killed || sp.q3_cert[2] < min_fallback) {
    std::printf("PLANCHER : candidats %llu < %llu ou tues %llu < %llu ou replis %llu < %llu\n", (unsigned long long)sp.candidates[1],
                (unsigned long long)min_candidates, (unsigned long long)sp.depth_killed[1], (unsigned long long)min_killed,
                (unsigned long long)sp.q3_cert[2], (unsigned long long)min_fallback);
    return 3;
  }
  // Contrat de lotissement : borne dure seuil + plus grosse ancre ; nombre de
  // vidages au moins min_flushes (un code ignorant le seuil resterait vert sinon).
  if (bs.max_lot_seeds > (u64)lim.seeds + bs.max_anchor_seeds || bs.max_lot_sites > (u64)lim.sites + bs.max_anchor_sites || bs.flushes < min_flushes) {
    std::printf("LOTISSEMENT : max_lot_seeds=%llu > seuil %zu + max_ancre %llu, ou vidages %llu < %llu\n",
                (unsigned long long)bs.max_lot_seeds, lim.seeds, (unsigned long long)bs.max_anchor_seeds,
                (unsigned long long)bs.flushes, (unsigned long long)min_flushes);
    return 1;
  }
  if (vec_mism || bad) return mutant ? 4 : 1;
  if (mutant) {
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  return 0;
}
