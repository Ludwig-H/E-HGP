// MorseHGP3D v5 — porte de SURETE DU FOLD CONCURRENT (P1 de l'audit du
// 28 aout 2026, ETAT_COURANT § « P1 — surete du fold concurrent »).
//
// Le pipeline lance un fil d'etage B par ordre K (jusqu'a `fold_inflight` en
// vol) ; la publication est strictement dans l'ordre des K et le PREMIER DEFAUT
// DANS L'ORDRE DES K fait foi. Cette porte force les entrelacements par le hook
// d'observation `RunOptions::on_fold_phase` (un callback qui bloque jusqu'a
// une phase precise d'un autre ordre) : chaque scenario est DETERMINISTE, et un
// entrelacement non atteint est un plancher viole (code 3), jamais un vert par
// vacuite. Compilee avec -fno-elide-constructors (le `return rr` copie/deplace
// la valeur de retour AVANT la destruction des locales : sans jonction explicite,
// un fil B ecrirait encore dans `rr`) et executee aussi sous -fsanitize=thread.
//
// Sans injection (code 0 conforme, 3 viole) :
//   N1. pic mesure : fold_inflight=3, le callback de K=1 attend le debut des
//       reductions de K=2 et K=3 -> peak_fold_inflight >= 2 (et <= 3), digest_all
//       identique a fold_inflight=1, callbacks ordonnes et jamais simultanes
//       (journal protege par mutex), phases coherentes ;
//   N2. fold_inflight=1 : peak == 1 ;
//   N3. exception du CALLBACK de K=2 alors que K=3 a fini sa reduction et
//       attend son tour : K=1 et K=2 publies, K=3 non publie, exception
//       propagee apres jonction ;
//   N4. domaine de fold_inflight : 0, -1, 17 refuses (invalid_input) avant tout
//       calcul, 1 et 16 acceptes.
// Avec --inject (code 4 = defaut injecte arbitre selon le contrat, 3 sinon) :
//   A.  fold-inject-a-failure-k2 (fold_inflight=2) : echec d'etage A a K=2
//       PENDANT que l'ordre 1 est en vol (son callback attend la phase
//       kStageAFailed de K=2) : K=1 publie, statut invariant_violated (K=2),
//       aucun terminate, tous les fils joints avant le retour ;
//   B.  fold-inject-b-exception-k3 (fold_inflight=3) : exception de reduction
//       de K=3 CONSERVEE dans son slot AVANT la fin de K=1 (dont le callback
//       attend kReduceFailed de K=3 et kReduceEnd de K=2) : K=1 ET K=2 publies,
//       K=3 non publie, exception de K=3 propagee.
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {

int g_bad = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    std::fprintf(stderr, "CONTRAT VIOLE : %s\n", what);
    ++g_bad;
  } else {
    std::printf("ok : %s\n", what);
  }
}

// Journal des phases, protege : ecrit depuis le fil principal et les fils B.
struct PhaseLog {
  std::mutex m;
  std::condition_variable cv;
  std::vector<std::pair<u64, FoldPhase>> log;
  void record(u64 K, FoldPhase p) {
    {
      std::lock_guard<std::mutex> lk(m);
      log.emplace_back(K, p);
    }
    cv.notify_all();
  }
  bool seen_locked(u64 K, FoldPhase p) const {
    for (const auto& e : log)
      if (e.first == K && e.second == p) return true;
    return false;
  }
  bool seen(u64 K, FoldPhase p) {
    std::lock_guard<std::mutex> lk(m);
    return seen_locked(K, p);
  }
  // Attend (borne : 30 s) la phase (K, p) ; false = plancher d'entrelacement non atteint.
  bool wait_for(u64 K, FoldPhase p) {
    std::unique_lock<std::mutex> lk(m);
    return cv.wait_for(lk, std::chrono::seconds(30), [&] { return seen_locked(K, p); });
  }
  size_t count(FoldPhase p) {
    std::lock_guard<std::mutex> lk(m);
    size_t c = 0;
    for (const auto& e : log)
      if (e.second == p) ++c;
    return c;
  }
  // Chaque ordre 1..kmax a ete observe kPublished EXACTEMENT une fois, aucun
  // ordre au-dela. L'ordre STRICT des publications est juge par le callback
  // `on_forest` (sous le verrou de publication, `CallbackLog::ordered`), pas
  // par ce journal : le hook est appele HORS verrou depuis le fil B de chaque
  // ordre (contrat de `RunOptions::on_fold_phase`), donc le fil de K+1 peut
  // journaliser son kPublished avant que le fil de K n'ait atteint le sien.
  bool published_complete(u64 kmax) {
    std::lock_guard<std::mutex> lk(m);
    std::vector<u32> seen(kmax + 1, 0);
    for (const auto& e : log)
      if (e.second == FoldPhase::kPublished) {
        if (e.first < 1 || e.first > kmax) return false;
        ++seen[e.first];
      }
    for (u64 k = 1; k <= kmax; ++k)
      if (seen[k] != 1) return false;
    return true;
  }
};

// Journal des callbacks, protege (P1 : `last_k`/`ordered`/`overlapped` entraient
// eux-memes en course sur le chevauchement que la porte cherche a exclure).
struct CallbackLog {
  std::mutex m;
  int calls = 0, inside = 0;
  u64 last_k = 0;
  bool ordered = true, overlapped = false;
  void enter(u64 K) {
    std::lock_guard<std::mutex> lk(m);
    if (inside++ != 0) overlapped = true;
    if (K != last_k + 1) ordered = false;
    last_k = K;
    ++calls;
  }
  void leave() {
    std::lock_guard<std::mutex> lk(m);
    --inside;
  }
};

RunOptions base_options(int threads, int inflight, PhaseLog* ph) {
  RunOptions o;
  o.threads = threads;
  o.digest = true;
  o.fold_inflight = inflight;
  o.on_fold_phase = [ph](u64 K, FoldPhase p) { ph->record(K, p); };
  return o;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 300, threads = 4;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (n < 12 || threads < 1) return 2;
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const std::vector<InputPoint> in = make_family_input(CloudFamily::kUniform, n, 0, 3);

  if (inject == "fold-inject-a-failure-k2") {
    // A. Echec d'etage A a K=2 pendant que l'ordre 1 est en vol.
    PhaseLog ph;
    CallbackLog cb;
    bool handshake = false;
    RunOptions o = base_options(threads, 2, &ph);
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult&) {
      cb.enter(K);
      if (K == 1) handshake = ph.wait_for(2, FoldPhase::kStageAFailed);
      cb.leave();
    };
    bool threw = false;
    RunResult rr;
    try {
      rr = run_pipeline(in, o);
    } catch (...) {
      threw = true;
    }
    expect(!threw, "A : aucune exception (le defaut d'etage A est un statut)");
    expect(handshake, "A : PLANCHER — l'echec d'etage A de K=2 a ete observe pendant la publication de K=1");
    expect(rr.status == PipelineStatus::kInvariantViolated, "A : statut invariant_violated");
    expect(rr.message.find("(K=2)") != std::string::npos, "A : le message designe K=2");
    expect(cb.calls == 1 && cb.last_k == 1, "A : K=1 publie (un callback), rien apres");
    expect(ph.seen(1, FoldPhase::kPublished), "A : phase kPublished de K=1");
    expect(!ph.seen(2, FoldPhase::kReduceBegin), "A : aucune reduction de K=2 lancee");
    expect(rr.kmax_eff >= 3, "A : PLANCHER — le defaut n'est pas au dernier ordre");
    expect(rr.peak_fold_inflight == 1, "A : pic en vol = 1 (un seul ordre lance)");
    std::printf("fold_inflight_safety_gate : mutant %s arbitre (statut=%d, message=%s)\n", inject.c_str(), (int)rr.status,
                rr.message.c_str());
    return g_bad ? 3 : 4;
  }
  if (inject == "fold-inject-b-exception-k3") {
    // B. Exception de reduction de K=3 conservee avant la fin de K=1.
    PhaseLog ph;
    CallbackLog cb;
    bool hs2 = false, hs3 = false;
    RunOptions o = base_options(threads, 3, &ph);
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult&) {
      cb.enter(K);
      if (K == 1) {
        hs3 = ph.wait_for(3, FoldPhase::kReduceFailed);
        hs2 = ph.wait_for(2, FoldPhase::kReduceEnd);
      }
      cb.leave();
    };
    bool caught = false;
    std::string what;
    try {
      (void)run_pipeline(in, o);
    } catch (const std::runtime_error& e) {
      caught = true;
      what = e.what();
    }
    expect(caught, "B : exception de K=3 propagee par run_pipeline (apres jonction)");
    expect(what.find("fold-inject-b-exception-k3") != std::string::npos, "B : c'est l'exception injectee de K=3");
    expect(hs3, "B : PLANCHER — l'exception de K=3 a ete conservee pendant la publication de K=1");
    expect(hs2, "B : PLANCHER — K=2 attendait son tour pendant la publication de K=1");
    expect(cb.calls == 2 && cb.last_k == 2 && cb.ordered && !cb.overlapped, "B : K=1 ET K=2 publies, dans l'ordre");
    expect(ph.seen(2, FoldPhase::kPublished), "B : phase kPublished de K=2 (jamais annulee par K=3)");
    expect(ph.seen(3, FoldPhase::kNotPublished), "B : K=3 non publie");
    expect(!ph.seen(3, FoldPhase::kPublished) && !ph.seen(4, FoldPhase::kPublished), "B : rien publie au-dela de K=2");
    std::printf("fold_inflight_safety_gate : mutant %s arbitre (%s)\n", inject.c_str(), what.c_str());
    return g_bad ? 3 : 4;
  }
  if (!inject.empty()) return 2;  // mutant connu mais sans scenario ici

  // N1 + N2. Pic mesure et identite des digests.
  std::string ref;
  {
    for (const int f : {1, 3}) {
      PhaseLog ph;
      CallbackLog cb;
      bool hs = true;
      RunOptions o = base_options(threads, f, &ph);
      o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult&) {
        cb.enter(K);
        if (K == 1 && f >= 2) hs = ph.wait_for(2, FoldPhase::kReduceBegin) && ph.wait_for(3, FoldPhase::kReduceBegin);
        cb.leave();
      };
      const RunResult rr = run_pipeline(in, o);
      char what[160];
      std::snprintf(what, sizeof(what), "N%d : fold_inflight=%d statut complet (%s)", f == 1 ? 2 : 1, f, rr.message.c_str());
      expect(rr.status == PipelineStatus::kCompleteRegular, what);
      expect(rr.kmax_eff >= 4, "N1 : PLANCHER — au moins quatre ordres");
      if (f >= 2) expect(hs, "N1 : PLANCHER — K=2 et K=3 ont commence leur reduction pendant la publication de K=1");
      expect(cb.ordered && !cb.overlapped && cb.last_k == rr.kmax_eff && cb.calls == (int)rr.kmax_eff,
             "N : callbacks 1..K_max, ordonnes, jamais simultanes");
      expect(ph.published_complete(rr.kmax_eff), "N : phases kPublished 1..K_max, une fois chacune, rien au-dela");
      expect(ph.count(FoldPhase::kReduceBegin) == rr.kmax_eff && ph.count(FoldPhase::kReduceEnd) == rr.kmax_eff,
             "N : une reduction commencee et terminee par ordre");
      expect(ph.count(FoldPhase::kReduceFailed) == 0 && ph.count(FoldPhase::kNotPublished) == 0 &&
                 ph.count(FoldPhase::kStageAFailed) == 0,
             "N : aucun defaut de phase");
      std::snprintf(what, sizeof(what), "N : pic en vol mesure %llu <= fold_inflight=%d", (unsigned long long)rr.peak_fold_inflight, f);
      expect(rr.peak_fold_inflight <= (u64)f, what);
      if (f == 1) {
        expect(rr.peak_fold_inflight == 1, "N2 : fold_inflight=1 -> pic en vol == 1");
        ref = rr.digest_all;
      } else {
        std::snprintf(what, sizeof(what), "N1 : PLANCHER de concurrence — pic en vol mesure %llu >= 2",
                      (unsigned long long)rr.peak_fold_inflight);
        expect(rr.peak_fold_inflight >= 2, what);
        expect(!ref.empty() && rr.digest_all == ref, "N1 : digest_all identique a fold_inflight=1");
      }
    }
  }
  // N3. Exception du callback de K=2 alors que K=3 attend son tour.
  {
    PhaseLog ph;
    CallbackLog cb;
    bool hs = false;
    RunOptions o = base_options(threads, 3, &ph);
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult&) {
      cb.enter(K);
      if (K == 2) {
        hs = ph.wait_for(3, FoldPhase::kReduceEnd);
        cb.leave();
        throw std::runtime_error("callback K=2");
      }
      cb.leave();
    };
    bool caught = false;
    try {
      (void)run_pipeline(in, o);
    } catch (const std::runtime_error& e) {
      caught = std::string(e.what()) == "callback K=2";
    }
    expect(caught, "N3 : exception du callback de K=2 propagee");
    expect(hs, "N3 : PLANCHER — K=3 avait fini sa reduction quand K=2 a leve");
    expect(cb.calls == 2 && cb.last_k == 2, "N3 : K=1 et K=2 publies, rien apres");
    expect(ph.seen(3, FoldPhase::kNotPublished) && !ph.seen(3, FoldPhase::kPublished), "N3 : K=3 non publie");
  }
  // N4. Domaine de fold_inflight.
  {
    for (const int f : {0, -1, kFoldInflightMax + 1}) {
      RunOptions o;
      o.threads = threads;
      o.fold_inflight = f;
      int calls = 0;
      o.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++calls; };
      const RunResult rr = run_pipeline(in, o);
      char what[160];
      std::snprintf(what, sizeof(what), "N4 : fold_inflight=%d refuse (invalid_input, zero callback) : %s", f, rr.message.c_str());
      expect(rr.status == PipelineStatus::kInvalidInput && calls == 0 && rr.cards.empty(), what);
    }
    for (const int f : {1, kFoldInflightMax}) {
      RunOptions o;
      o.threads = threads;
      o.digest = true;
      o.fold_inflight = f;
      const RunResult rr = run_pipeline(in, o);
      char what[160];
      std::snprintf(what, sizeof(what), "N4 : fold_inflight=%d accepte, digest identique", f);
      expect(rr.status == PipelineStatus::kCompleteRegular && rr.digest_all == ref, what);
    }
  }
  if (g_bad) return 3;
  std::printf("fold_inflight_safety_gate OK (n=%d threads=%d)\n", n, threads);
  return 0;
}
