// MorseHGP3D v5 — porte des GARDES D'API et des cas limites (audit bloquant
// 87e915bd, P0/P1). Chaque cas exige le statut contractuel, AUCUN callback et
// AUCUN payload sur un refus ; sous ASan/UBSan (option MHGP5_ENABLE_SANITIZERS)
// tout debordement serait un crash par signal, refuse par run_expect.
//   entree vide, singleton, deux points, positions dupliquees, coordonnee hors
//   profil, PointId duplique, smax ∈ {0, 1, 11, 12}, s < 1, threads <= 0,
//   plafond de coquille < 4 ; census sur un singleton (P1 : nodes.empty() n'est
//   pas le vide) ; expansion a kmax = 1 (smax = 2).
// Codes : 0 conforme, 3 contrat viole.
#include <cstdio>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/census.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {
int g_bad = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    std::fprintf(stderr, "CONTRAT VIOLE : %s\n", what);
    ++g_bad;
  }
}
// Execute et verifie : statut attendu, callbacks = 0 sur refus, aucun digest.
void run_case(const char* label, const std::vector<InputPoint>& in, RunOptions opt, PipelineStatus want) {
  int callbacks = 0;
  opt.digest = true;
  opt.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++callbacks; };
  const RunResult rr = run_pipeline(in, opt);
  char what[200];
  std::snprintf(what, sizeof(what), "%s : statut %d attendu %d (%s)", label, (int)rr.status, (int)want, rr.message.c_str());
  expect(rr.status == want, what);
  if (want != PipelineStatus::kCompleteRegular) {
    std::snprintf(what, sizeof(what), "%s : %d callbacks sur un refus", label, callbacks);
    expect(callbacks == 0, what);
    std::snprintf(what, sizeof(what), "%s : payload publie sur un refus", label);
    expect(rr.digest_all.empty() && rr.cards.empty() && rr.total_events == 0, what);
  } else {
    std::snprintf(what, sizeof(what), "%s : %d callbacks, kmax_eff=%llu", label, callbacks, (unsigned long long)rr.kmax_eff);
    expect(callbacks == (int)rr.kmax_eff, what);
  }
}
}  // namespace

int main() {
  const std::vector<InputPoint> two = {{0, {0, 0, 0}}, {1, {10, 0, 0}}};
  const std::vector<InputPoint> twelve = make_family_input(CloudFamily::kUniform, 12, 12, 3);
  const std::vector<InputPoint> small = make_family_input(CloudFamily::kUniform, 60, 0, 3);
  RunOptions base;
  base.threads = 2;
  run_case("vide", {}, base, PipelineStatus::kInvalidInput);
  run_case("singleton", {{0, {5, 5, 5}}}, base, PipelineStatus::kInvalidInput);
  run_case("deux points", two, base, PipelineStatus::kCompleteRegular);
  run_case("positions dupliquees", {{0, {1, 1, 1}}, {1, {1, 1, 1}}, {2, {3, 0, 0}}}, base, PipelineStatus::kUnsupportedDegeneracy);
  run_case("coordonnee hors profil", {{0, {0, 0, 0}}, {1, {65536, 0, 0}}}, base, PipelineStatus::kInvalidInput);
  run_case("PointId duplique", {{7, {0, 0, 0}}, {7, {1, 0, 0}}}, base, PipelineStatus::kInvalidInput);
  for (const u64 smax : {0ull, 1ull, 12ull, 100ull}) {
    RunOptions o = base;
    o.smax = smax;
    char label[64];
    std::snprintf(label, sizeof(label), "smax=%llu", (unsigned long long)smax);
    run_case(label, twelve, o, PipelineStatus::kInvalidInput);
  }
  {
    RunOptions o = base;
    o.smax = 11;
    run_case("smax=11 sur douze points", twelve, o, PipelineStatus::kCompleteRegular);
    o.smax = 2;
    run_case("smax=2 (kmax=1)", small, o, PipelineStatus::kCompleteRegular);
    o = base;
    o.s = 0;
    run_case("s=0", small, o, PipelineStatus::kInvalidInput);
    o = base;
    o.threads = 0;
    run_case("threads=0", small, o, PipelineStatus::kInvalidInput);
    o = base;
    o.shell_cap = 3;
    run_case("shell_cap=3", small, o, PipelineStatus::kInvalidInput);
  }
  // P1 : census sur un singleton — le point (1,1,1) est strictement interieur a
  // P(z) = |z|² − 4 : at_least(1) vrai ; at_least(2) faux avec count = 1 ;
  // census : un interieur, aucune coquille.
  {
    const CloudIndex ix = build_cloud_index(std::vector<P3>{{1, 1, 1}});
    const BallKey key{1, {0, 0, 0}, -4};
    u64 count = 0;
    expect(ball_depth_at_least(ix, key, 1, &count), "singleton : at_least(1) faux");
    expect(!ball_depth_at_least(ix, key, 2, &count) && count == 1, "singleton : at_least(2) ou count != 1");
    std::vector<i32> in, sh;
    expect(ball_census(ix, key, 4, 4, &in, &sh) == CensusStatus::kOk && in.size() == 1 && sh.empty(), "singleton : census");
    // Deux points, l'un sur la coquille : |z|² = 4 pour (2,0,0).
    const CloudIndex ix2 = build_cloud_index(std::vector<P3>{{1, 1, 1}, {2, 0, 0}});
    expect(ball_census(ix2, key, 4, 4, &in, &sh) == CensusStatus::kOk && in.size() == 1 && sh.size() == 1, "deux points : coquille");
  }
  if (g_bad) return 3;
  std::printf("api_guard_gate OK\n");
  return 0;
}
