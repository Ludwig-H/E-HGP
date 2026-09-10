// Sonde d'audit prospective (scratchpad) : reutilise UNIQUEMENT la gate du
// developpeur (catalogue oracle Gram/Gamma + juge check_fixture) sur des
// fixtures de l'auditeur absentes de la gate, plus une fixture construite
// pour exercer l'echange a rayon egal en production.
#define main recorded_gate_entry
#include "/workspaces/E-HGP/morsehgp3D_v7/tests/full_ball_tower_gate.cpp"
#undef main

static int run_one(const char* name, std::vector<P3> pts, unsigned kmax) {
  Fixture fx{name, pts, kmax};
  const u64 sr0 = same_radius, iq0 = intruders, ex0 = extra;
  try {
    for (unsigned variant = 0; variant < 2; ++variant) check_fixture(fx, variant);
    std::printf("fixture=%s kmax=%u status=PASS same_radius_steps=%llu intruder_queries=%llu extra_blocks=%llu cuts=%llu\n",
      name, kmax, (unsigned long long)(same_radius - sr0), (unsigned long long)(intruders - iq0),
      (unsigned long long)(extra - ex0), (unsigned long long)cuts);
    return 0;
  } catch (const Failure& f) {
    std::printf("fixture=%s kmax=%u status=FAIL why=%s context=%s\n", name, kmax, f.why, context.c_str());
    return 1;
  } catch (const std::exception& e) {
    std::printf("fixture=%s kmax=%u status=EXCEPTION what=%s\n", name, kmax, e.what());
    return 1;
  }
}

int main() {
  int bad = 0;
  // Contre-fixture README §3 : parents locaux (2) != global (1) via Z=(2,3).
  bad += run_one("external_bridge", {{0,0,0},{4,0,0},{2,2,0},{2,3,0}}, 4);
  // BALL_ANCHORS §3 : arite locale 3 vs q_min global 2 sur la meme boule.
  bad += run_one("arity_mix", {{10,5,0},{0,5,0},{2,1,0},{2,9,0}}, 4);
  // COVERAGE_THRESHOLDS : memes seuils, H0 different (6 vs 8 facettes isolees K3).
  bad += run_one("circle6", {{10,5,5},{8,9,5},{2,9,5},{0,5,5},{2,1,5},{8,1,5}}, 6);
  bad += run_one("octa6", {{10,5,5},{0,5,5},{5,10,5},{5,0,5},{5,5,10},{5,5,0}}, 6);
  // COVERAGE_THRESHOLDS : coquille asymetrique h=5, h_S=3.
  bad += run_one("asym_shell", {{5,5,0},{5,5,10},{8,5,9},{5,8,9},{2,5,9},{5,2,9}}, 6);
  // Fixture construite par l'auditeur prospectif : carre (10..14,10..14,z=10)
  // R^2=8 avec 4 interieurs hors-plan, boule englobante centre (12,12,9) R^2=9
  // de coquille 4 coins + pole W=(12,12,6) : a K4 le representant est le carre,
  // sans ancre (K4 < p+q_min-1 = 5), echange a rayon egal attendu.
  bad += run_one("equal_radius9", {{10,10,10},{14,10,10},{14,14,10},{10,14,10},{12,12,6},
                                    {11,12,12},{13,12,12},{12,11,12},{12,13,12}}, 9);
  bad += run_one("equal_radius9_k5", {{10,10,10},{14,10,10},{14,14,10},{10,14,10},{12,12,6},
                                    {11,12,12},{13,12,12},{12,11,12},{12,13,12}}, 5);
  std::printf("total_cuts=%llu total_facets=%llu total_vertical=%llu births=%llu merges=%llu growth=%llu\n",
    (unsigned long long)cuts, (unsigned long long)facets, (unsigned long long)vertical,
    (unsigned long long)births, (unsigned long long)merges, (unsigned long long)growth);
  return bad ? 1 : 0;
}
