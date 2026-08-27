// MorseHGP3D v5 — porte du rendu § 9.1 : fixtures gravees et mutants.
//   carre cocyclique (110,100,100),(100,110,100),(90,100,100),(100,90,100),
//   sphere de centre (100,100,100), R² = 100 :
//     K=2 : six facettes (quatre cotes, deux diagonales), EXACTEMENT deux
//           incidences chacune, un seul lot, 12 incidences
//           (mutant render-collapse-mult : 6) ;
//     K=3 : quatre triangles, tous attachements, presents au rendu
//           (mutant render-active-only : rendu vide) ;
//     naissances : cote a rho² = 50, diagonale a 100 (mutant
//           birth-from-events — niveau de la premiere incidence, 100 pour
//           les deux — est un mutant DE PORTE : il remplace la miniboule par
//           le niveau de premiere incidence, la fixture le tue).
// Codes : 0 conforme, 3 invariant, 4 mutant tue.
#include <cstdio>
#include <cstring>
#include <string>

#include "../src/forest/render.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool birth_mutant = MHGP5_MUTANT("birth-from-events");  // mutant DE PORTE (tests/), declare dans kMutants
  const std::vector<InputPoint> sq = {{0, {110, 100, 100}}, {1, {100, 110, 100}}, {2, {90, 100, 100}}, {3, {100, 90, 100}}};
  RunOptions o;
  o.threads = 1;
  o.smax = 4;  // K <= 3
  RenderResult r2, r3;
  std::vector<ForestEvent> ev2;
  o.on_forest = [&](u64 K, const std::vector<ForestEvent>& events, const ForestResult&) {
    if (K == 2) { r2 = build_render(events); ev2 = events; }
    if (K == 3) r3 = build_render(events);
  };
  const RunResult rr = run_pipeline(sq, o);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    return 2;
  }
  int bad = 0;
  // K=2 : 6 facettes, 12 incidences, un lot, 2 par facette.
  if (r2.facets.size() != 6 || r2.batch_levels.size() != 1) { std::fprintf(stderr, "K=2 : %zu facettes, %zu lots\n", r2.facets.size(), r2.batch_levels.size()); ++bad; }
  if (r2.incidences != 12) { std::fprintf(stderr, "K=2 : %llu incidences (attendu 12)\n", (unsigned long long)r2.incidences); ++bad; }
  for (const FacetIncidences& fi : r2.facets)
    if (fi.per_batch.size() != 1 || fi.per_batch[0].second != 2) { std::fprintf(stderr, "K=2 : multiplicite != 2\n"); ++bad; break; }
  // K=3 : quatre triangles au rendu.
  if (r3.facets.size() != 4) { std::fprintf(stderr, "K=3 : %zu facettes (attendu 4)\n", r3.facets.size()); ++bad; }
  // Naissances : cotes a 50, diagonales a 100.
  const P3 pos[4] = {{110, 100, 100}, {100, 110, 100}, {90, 100, 100}, {100, 90, 100}};
  u64 sides50 = 0, diag100 = 0;
  for (const FacetIncidences& fi : r2.facets) {
    const P3 pts[2] = {pos[fi.facet.p[0]], pos[fi.facet.p[1]]};
    ExactLevel lvl;
    if (birth_mutant) lvl = r2.batch_levels[fi.per_batch[0].first];  // MUTANT de porte
    else if (!facet_birth_level(pts, 2, &lvl)) { ++bad; continue; }
    const ExactLevel l50 = promote_level(Rational128{50, 1}), l100 = promote_level(Rational128{100, 1});
    if (same_exact_level(lvl, l50)) ++sides50;
    else if (same_exact_level(lvl, l100)) ++diag100;
  }
  if (sides50 != 4 || diag100 != 2) { std::fprintf(stderr, "naissances : %llu cotes a 50, %llu diagonales a 100\n", (unsigned long long)sides50, (unsigned long long)diag100); ++bad; }
  if (!inject.empty()) {
    if (bad) { std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str()); return 4; }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (bad) return 3;
  std::printf("render_gate OK\n");
  return 0;
}
