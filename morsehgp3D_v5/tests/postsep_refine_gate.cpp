// MorseHGP3D v5 — PORTE DU RAFFINEMENT POST-SEPARATION
// (docs/MESURES_ECHELLE.md § 4). Le raffinement prolonge la descente ternaire
// A L INTERIEUR d'un rectangle vivant : les sous-rectangles PARTITIONNENT les
// paires du parent, donc l'objet ne change pas. Ce que la porte exige :
//   (1) DIGEST IDENTIQUE a L = 0, 1, 2, 3 sur plusieurs familles — c'est la
//       preuve que l'objet est inchange (et non l'ordre d'enumeration, qui
//       change quand B est scinde) ;
//   (2) GRAND-LIVRE exact par lane : `emis + tues == base`, sans quoi une
//       paire a ete perdue ou comptee deux fois ;
//   (3) ROUTE q2 INTERDITE : `tues[q2] == 0` et `emis[q2] == base[q2]` a tout
//       L — en q2 aucun pretest ponctuel ne referme la couture du temoin du
//       frere ;
//   (4) POSITIONS `refine-hist-wakeup` de l'audit du 28 aout 2026 (quatre
//       points, s = 1, smax = 3) jouees comme fixture de NON-REGRESSION : le
//       raffinement n'y change ni le digest ni les candidats q2. Le
//       « reveil » d'histogramme que l'audit y decrit n'a PAS pu etre
//       exhibe — ni sur ces positions, ni sur 18 000 nuages entiers
//       aleatoires — le cœur de l'enfant croissant et `need` diminuant en
//       compensation ; la route q2 reste donc fermee par CONCEPTION et
//       gardee par l'invariant `tues[q2] == 0`, non par un mutant ;
//   (5) PLANCHERS de non-vacuite : le raffinement doit tuer une masse de
//       paires strictement positive en q3 ET en q4, sinon la porte est verte
//       par vacuite.
// Codes : 0 ; 1 desaccord ; 2 refus ; 3 plancher ; 4 mutant tue.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {
int failures = 0;
u64 g_killed3 = 0, g_killed4 = 0;
void expect(bool ok, const char* what) {
  if (!ok) { std::printf("ECHEC : %s\n", what); ++failures; }
}

struct Out {
  std::string digest;
  u64 base[3] = {0, 0, 0}, emitted[3] = {0, 0, 0}, killed[3] = {0, 0, 0};
  u64 candidates[3] = {0, 0, 0};
  bool ok = false;
};

Out run_one(const std::vector<InputPoint>& in, u32 levels, i64 s, u64 smax, int threads) {
  Out o;
  RunOptions opt;
  opt.s = s;
  opt.smax = smax;
  opt.threads = threads;
  opt.digest = true;
  opt.postsep_refine_levels = levels;
  const RunResult rr = run_pipeline(in, opt);
  if (rr.status != PipelineStatus::kCompleteRegular) return o;
  o.digest = rr.digest_all;
  for (int i = 0; i < 3; ++i) {
    o.base[i] = rr.gen.postsep_base_mass[i];
    o.emitted[i] = rr.gen.postsep_emitted_mass[i];
    o.killed[i] = rr.gen.postsep_killed_mass[i];
    o.candidates[i] = rr.gen.candidates[i];
  }
  o.ok = true;
  return o;
}
}  // namespace

int main(int argc, char** argv) {
  u64 min_killed = 1000;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--min-killed=", 0) == 0) min_killed = (u64)std::atoll(a.c_str() + 13);
    else if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool mutant = MHGP5_MUTANT("postsep-drop-child") || MHGP5_MUTANT("postsep-kill-h-minus-one");

  // ---- (4) CONTRE-FIXTURE `refine-hist-wakeup` (positions de l'audit).
  // Ordre Morton u3, u0, u1, u2 ; parent A = {u0}, B = {u1, u2, u3}.
  {
    const std::vector<InputPoint> in = {InputPoint{0, P3{64, 183, 31}}, InputPoint{1, P3{90, 7, 26}},
                                        InputPoint{2, P3{52, 146, 28}}, InputPoint{3, P3{91, 156, 28}}};
    const Out a = run_one(in, 0, 1, 3, 1), b = run_one(in, 3, 1, 3, 1);
    if (!a.ok || !b.ok) { std::printf("REFUS : contre-fixture refine-hist-wakeup\n"); return 2; }
    std::printf("refine-hist-wakeup L=0 digest=%.16s candidats_q2=%llu | L=3 digest=%.16s candidats_q2=%llu\n", a.digest.c_str(),
                (unsigned long long)a.candidates[0], b.digest.c_str(), (unsigned long long)b.candidates[0]);
    expect(a.digest == b.digest, "refine-hist-wakeup : digest inchange par le raffinement");
    expect(a.candidates[0] == b.candidates[0], "refine-hist-wakeup : candidats q2 inchanges");
    expect(b.killed[0] == 0, "refine-hist-wakeup : la route q2 n'a rien tue");
  }

  // ---- (1) (2) (3) (5) sur les familles de mesure.
  const struct { CloudFamily f; int n; } clouds[] = {{CloudFamily::kScanlineSinglePass, 1500}, {CloudFamily::kUniform, 1200},
                                                     {CloudFamily::kTerrain, 1000}, {CloudFamily::kEightClusters, 1200},
                                                     {CloudFamily::kTwoLines, 400}, {CloudFamily::kCollinearSeven, 7}};
  for (const auto& c : clouds) {
    const std::vector<InputPoint> in = make_family_input(c.f, c.n, cloud_family_default_coord(c.f, c.n), 3);
    const char* fam = cloud_family_name(c.f);
    const Out ref = run_one(in, 0, 8, 11, 4);
    if (!ref.ok) { std::printf("REFUS %s\n", fam); return 2; }
    for (u32 L = 1; L <= 3; ++L) {
      const Out o = run_one(in, L, 8, 11, 4);
      if (!o.ok) { std::printf("REFUS %s L=%u\n", fam, L); return 2; }
      char what[160];
      std::snprintf(what, sizeof(what), "%s L=%u : digest identique a L=0", fam, L);
      expect(o.digest == ref.digest, what);
      for (int q = 0; q < 3; ++q) {
        std::snprintf(what, sizeof(what), "%s L=%u q%d : grand-livre emis+tues==base (%llu+%llu vs %llu)", fam, L, q + 2,
                      (unsigned long long)o.emitted[q], (unsigned long long)o.killed[q], (unsigned long long)o.base[q]);
        expect(o.emitted[q] + o.killed[q] == o.base[q], what);
        std::snprintf(what, sizeof(what), "%s L=%u q%d : base inchangee par L", fam, L, q + 2);
        expect(o.base[q] == ref.base[q], what);
      }
      std::snprintf(what, sizeof(what), "%s L=%u : route q2 INTERDITE (tues=%llu)", fam, L, (unsigned long long)o.killed[0]);
      expect(o.killed[0] == 0 && o.emitted[0] == o.base[0], what);
      if (L == 3) { g_killed3 += o.killed[1]; g_killed4 += o.killed[2]; }
    }
  }

  std::printf("postsep_refine_gate echecs=%d paires_tuees_L3 q3=%llu q4=%llu\n", failures, (unsigned long long)g_killed3,
              (unsigned long long)g_killed4);
  if (mutant) {
    if (failures) return 4;
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (g_killed3 < min_killed || g_killed4 < min_killed) { std::printf("PLANCHER\n"); return 3; }
  if (failures) return 1;
  std::printf("postsep_refine_gate OK\n");
  return 0;
}
