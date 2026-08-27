// MorseHGP3D v5 — porte de la garde d'arrondi du filtre flottant.
//
// La borne du filtre est prouvee sous arrondi au plus proche. Sous un autre
// mode (FE_UPWARD ici), `float_filter_runtime_enabled()` doit rendre false :
// borne = +inf, repli exact integral, digests INCHANGES et aucun signe
// certifie par le flottant (compteurs float_cert_* nuls). Sous FE_TONEAREST
// le filtre doit certifier une part non nulle des signes (plancher) : la porte
// prouve que le filtre est exerce ET qu'il ne decide jamais de l'objet.
// Codes : 0 conforme, 2 refus, 3 invariant.
#include <cfenv>
#include <cstdio>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

int main() {
  const int n = 400;
  const int coord = cloud_family_default_coord(CloudFamily::kEightClusters, n);
  const std::vector<InputPoint> in = make_family_input(CloudFamily::kEightClusters, n, coord, 3);
  RunOptions o;
  o.threads = 4;
  o.digest = true;
  if (std::fesetround(FE_TONEAREST) != 0) return 2;
  if (!float_filter_runtime_enabled()) {
    std::fprintf(stderr, "filtre coupe sous FE_TONEAREST (build -ffast-math ?)\n");
    return 3;
  }
  const RunResult near = run_pipeline(in, o);
  if (std::fesetround(FE_UPWARD) != 0) return 2;
  const bool enabled_up = float_filter_runtime_enabled();
  const RunResult up = run_pipeline(in, o);
  std::fesetround(FE_TONEAREST);
  if (near.status != PipelineStatus::kCompleteRegular || up.status != PipelineStatus::kCompleteRegular) return 2;
  const u64 cert_near = near.gen.float_cert_neg + near.gen.float_cert_pos;
  const u64 cert_up = up.gen.float_cert_neg + up.gen.float_cert_pos;
  std::printf("float_rounding nearest: certifies=%llu replis=%llu ; upward: filtre=%s certifies=%llu replis=%llu\n",
              (unsigned long long)cert_near, (unsigned long long)near.gen.float_fallback, enabled_up ? "ACTIF" : "coupe",
              (unsigned long long)cert_up, (unsigned long long)up.gen.float_fallback);
  int bad = 0;
  if (enabled_up || cert_up != 0) { std::fprintf(stderr, "INVARIANT : filtre actif hors arrondi au plus proche\n"); ++bad; }
  if (cert_near < 1000) { std::fprintf(stderr, "PLANCHER : filtre non exerce (%llu certifies)\n", (unsigned long long)cert_near); ++bad; }
  if (near.digest_all != up.digest_all || near.digest_balls != up.digest_balls) {
    std::fprintf(stderr, "INVARIANT : le filtre flottant decide de l'objet\n");
    ++bad;
  }
  if (bad) return 3;
  std::printf("float_rounding_gate OK\n");
  return 0;
}
