// MorseHGP3D v5 — porte des GARDES D'API et des cas limites (audit bloquant
// 87e915bd, P0/P1). Chaque cas exige le statut contractuel, AUCUN callback et
// AUCUN payload sur un refus ; sous ASan/UBSan (option MHGP5_ENABLE_SANITIZERS
// et UBSAN_OPTIONS=halt_on_error=1), tout debordement fait echouer la porte.
//   entree vide, singleton, deux points, positions dupliquees, coordonnee hors
//   profil, PointId duplique, smax ∈ {0, 1, 11, 12}, s ∈ {0, 1, 7, 8},
//   threads <= 0,
//   fold_inflight hors de [1, 16], postsep hors de [0,3] ou combine a un
//   override non propage, plafond de coquille < 4 ; census sur un singleton
//   (P1 : nodes.empty() n'est pas le vide) ; expansion a kmax = 1 (smax = 2).
// Codes : 0 conforme, 3 contrat viole.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
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
void run_case(const char* label, const std::vector<InputPoint>& in, RunOptions opt, PipelineStatus want,
              const char* want_message = nullptr) {
  int callbacks = 0;
  opt.digest = true;
  opt.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++callbacks; };
  const RunResult rr = run_pipeline(in, opt);
  char what[200];
  std::snprintf(what, sizeof(what), "%s : statut %d attendu %d (%s)", label, (int)rr.status, (int)want, rr.message.c_str());
  expect(rr.status == want, what);
  if (want_message != nullptr) {
    std::snprintf(what, sizeof(what), "%s : message '%s' attendu '%s'", label, rr.message.c_str(), want_message);
    expect(rr.message == want_message, what);
  }
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
    for (const i64 separation : {std::numeric_limits<i64>::min(), (i64)-1, (i64)0, (i64)1, (i64)7}) {
      o = base;
      o.s = separation;
      char label[64];
      std::snprintf(label, sizeof(label), "s=%lld", (long long)separation);
      run_case(label, small, o, PipelineStatus::kInvalidInput, "invalid_input : separation s < 8");
    }
    o = base;
    o.s = kSeparationProfileMin;
    run_case("s=8 (limite basse du profil)", small, o, PipelineStatus::kCompleteRegular);
    o.s = 10;
    run_case("s=10", small, o, PipelineStatus::kCompleteRegular);
    o.s = std::numeric_limits<i64>::max();
    run_case("s=INT64_MAX", twelve, o, PipelineStatus::kCompleteRegular);
    o = base;
    o.threads = 0;
    run_case("threads=0", small, o, PipelineStatus::kInvalidInput);
    o = base;
    o.postsep_refine_levels = 4;
    run_case("postsep=4", small, o, PipelineStatus::kInvalidInput);
    o = base;
    o.postsep_refine_levels = 1;
    o.q3_override = [](const CloudIndex&, const GenerateOptions&, std::vector<BallCandidate>*, GenerateStats*) {};
    run_case("postsep avec override q3", small, o, PipelineStatus::kInvalidInput);
    o = base;
    o.postsep_refine_levels = 1;
    o.q4_override = [](const CloudIndex&, const GenerateOptions&, std::vector<BallCandidate>*, GenerateStats*) {};
    run_case("postsep avec override q4", small, o, PipelineStatus::kInvalidInput);
    o = base;
    o.postsep_refine_levels = 3;
    run_case("postsep=3 (limite haute du domaine)", small, o, PipelineStatus::kCompleteRegular);
    // Domaine de fold_inflight [1, kFoldInflightMax] : hors domaine = refus
    // explicite AVANT calcul (jamais un std::max silencieux).
    for (const int f : {0, -1, kFoldInflightMax + 1, 1000}) {
      o = base;
      o.fold_inflight = f;
      char label[64];
      std::snprintf(label, sizeof(label), "fold_inflight=%d", f);
      run_case(label, small, o, PipelineStatus::kInvalidInput);
    }
    o = base;
    o.fold_inflight = kFoldInflightMax;
    run_case("fold_inflight=16 (limite haute du domaine)", small, o, PipelineStatus::kCompleteRegular);
    o = base;
    o.shell_cap = 3;
    run_case("shell_cap=3", small, o, PipelineStatus::kInvalidInput);
    // P0 (audit 9762daaf) : plafond de coquille au-dela du profil [4, 12] —
    // l'enumeration des plateaux indexe 1u << |coquille| ; 13, 32 et SIZE_MAX
    // sont refuses AVANT calcul, zero callback.
    for (const size_t cap : {(size_t)13, (size_t)32, (size_t)SIZE_MAX}) {
      o = base;
      o.shell_cap = cap;
      char label[64];
      std::snprintf(label, sizeof(label), "shell_cap=%zu", cap);
      run_case(label, small, o, PipelineStatus::kInvalidInput);
    }
    o = base;
    o.shell_cap = 12;
    run_case("shell_cap=12 (limite haute du profil)", small, o, PipelineStatus::kCompleteRegular);
  }
  // P1 (audit 9762daaf) : une exception de `on_forest` est propagee par
  // run_pipeline apres jonction du fil d'arriere-plan — pas d'abort.
  {
    RunOptions o;
    o.threads = 2;
    std::atomic<int> calls{0};
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult&) {
      ++calls;
      if (K == 2) throw std::runtime_error("callback K=2");
    };
    bool caught = false;
    try {
      (void)run_pipeline(small, o);
    } catch (const std::runtime_error& e) {
      caught = std::string(e.what()) == "callback K=2";
    }
    expect(caught, "exception du callback non propagee");
    expect(calls == 2, "exception du callback : le pipeline a continue apres K=2");
  }
  // ETAGE B CONCURRENT PAR ORDRE : sortie bit-identique quel que soit
  // `fold_inflight` (1, 2, 3, 8), callbacks strictement dans l'ordre des K
  // (1..K_max, un seul a la fois — journal sous MUTEX : `last_k`, `ordered`,
  // `overlapped` entreraient eux-memes en course sur le chevauchement que la
  // porte exclut), PIC MESURE d'ordres en vol : == 1 pour fold_inflight=1,
  // >= 2 pour fold_inflight >= 2 (le callback de K=1 attend, par le hook de
  // phase, que la reduction de K=2 ait commence : entrelacement force, jamais
  // un vert par vacuite d'une execution serielle), toujours <= fold_inflight ;
  // et exception d'un callback avec trois ordres en vol : propagee,
  // publication arretee a l'ordre fautif.
  {
    const std::vector<InputPoint> mid = make_family_input(CloudFamily::kUniform, 300, 0, 3);
    std::string ref;
    for (const int f : {1, 2, 3, 8}) {
      RunOptions o;
      o.threads = 4;
      o.digest = true;
      o.fold_inflight = f;
      std::mutex log_m;
      std::condition_variable log_cv;
      u64 last_k = 0;
      int inside = 0;
      bool ordered = true, overlapped = false, k2_reduce_begun = false, handshake = true;
      o.on_fold_phase = [&](u64 K, FoldPhase p) {
        if (K == 2 && p == FoldPhase::kReduceBegin) {
          {
            std::lock_guard<std::mutex> lk(log_m);
            k2_reduce_begun = true;
          }
          log_cv.notify_all();
        }
      };
      o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult&) {
        std::unique_lock<std::mutex> lk(log_m);
        if (inside++ != 0) overlapped = true;
        if (K != last_k + 1) ordered = false;
        last_k = K;
        if (K == 1 && f >= 2)
          handshake = log_cv.wait_for(lk, std::chrono::seconds(30), [&] { return k2_reduce_begun; });
        --inside;
      };
      const RunResult rr = run_pipeline(mid, o);
      expect(rr.status == PipelineStatus::kCompleteRegular, "fold_inflight : statut non complet");
      expect(ordered && last_k == rr.kmax_eff, "fold_inflight : callbacks hors ordre ou incomplets");
      expect(!overlapped, "fold_inflight : deux callbacks simultanes");
      expect(handshake, "fold_inflight >= 2 : PLANCHER — la reduction de K=2 n'a pas commence pendant la publication de K=1");
      expect(rr.peak_fold_inflight <= (u64)f, "fold_inflight : pic en vol mesure > fold_inflight");
      expect(f == 1 ? rr.peak_fold_inflight == 1 : rr.peak_fold_inflight >= 2,
             "fold_inflight : pic en vol mesure incoherent (== 1 attendu pour 1, >= 2 au-dela)");
      if (f == 1) ref = rr.digest_all;
      else expect(rr.digest_all == ref && !ref.empty(), "fold_inflight : digest_all different de la reference sequentielle");
    }
    for (const u64 kfail : {2ull, 5ull}) {
      RunOptions o;
      o.threads = 4;
      o.fold_inflight = 3;
      std::atomic<int> calls{0};
      o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult&) {
        ++calls;
        if (K == kfail) throw std::runtime_error("callback en vol");
      };
      bool caught = false;
      try {
        (void)run_pipeline(mid, o);
      } catch (const std::runtime_error& e) {
        caught = std::string(e.what()) == "callback en vol";
      }
      expect(caught, "fold_inflight=3 : exception du callback non propagee");
      expect(calls == (int)kfail, "fold_inflight=3 : publication poursuivie apres l'ordre fautif");
    }
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
