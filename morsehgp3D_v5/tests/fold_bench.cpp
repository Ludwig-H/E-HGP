// MorseHGP3D v5 — BANC APPARIE CONTREBALANCE du fold (un instrument, pas une
// porte de vitesse par defaut).
//
// Doctrine (v4 PASSATION § 2.10, reprise ici) : une constante de temps ne se
// conclut QUE par banc apparie contrebalance intra-processus — paires ABBA,
// mediane des RAPPORTS par paire (jamais un rapport de medianes), test de
// signe. Aucun claim sans mesure.
//
// Protocole :
//   1. `run_pipeline` sur (famille, n) ; `on_forest` COPIE les evenements de
//      l'ordre K vise (K = 10 par defaut, le plus lourd) ;
//   2. une paire d'echauffement (A puis B) NON comptee ;
//   3. P paires en ordre alterne AB, BA, AB, ... avec
//        A = build_forest(events, 1)   (chemin sequentiel de reference)
//        B = build_forest(events, T)   (chemin parallele, T = --threads) ;
//      chaque appel chronometre par steady_clock ; rapport = t_A / t_B ;
//   4. statistique : MEDIANE des rapports apparies, victoires de B
//      (t_B < t_A strictement), test de signe binomial a une queue
//      p = Σ_{k >= victoires} C(P, k) / 2^P (pour victoires = P : 1 / 2^P).
//
// Exigences de VALIDITE (sans elles le banc ne mesure rien) :
//   (i)   SIGNATURE IDENTIQUE par paire : digest_forest_v4(K, rA) ==
//         digest_forest_v4(K, rB), sinon code 3 (deux objets differents) ;
//   (ii)  OUVRIERS MESURES : chaque B a cree >= 2 ouvriers et <= T ; chaque A
//         en a cree exactement 1 (sinon A n'est pas la reference) — code 3 ;
//         le champ `ForestResult::workers` est requis, son absence est un
//         refus explicite (code 2) : un ouvrier declare n'est pas mesure ;
//   (iii) `--gate` : exige mediane >= --min-ratio (defaut 1,0) — code 3.
//
// Codes : 0 conforme ; 2 arguments / refus (pipeline non complet, K absent,
// evenements vides, `workers` absent) ; 3 signature differente, ouvriers hors
// contrat, ou plancher --gate ; 4 mutant tue sous `--inject` (les exigences
// de validite se prouvent non vacueuses : `parallel-one-worker` doit faire
// echouer (ii), un mutant survivant rend 3 — porte inefficace).
//
// Sortie : une ligne par paire (tA_ms, tB_ms, rapport) puis la ligne
//   fold_bench famille=.. n=.. K=.. paires=.. mediane=.. victoires=../.. p=..
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/digest.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

namespace {

// Le champ `workers` est LU, jamais suppose : s'il manque, le banc refuse.
template <typename R>
concept HasWorkers = requires(const R& r) { (u64)r.workers; };

template <typename R>
u64 measured_workers(const R& r) {
  if constexpr (HasWorkers<R>) return (u64)r.workers;
  else return 0;
}

double ms_since(std::chrono::steady_clock::time_point t0) {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

// C(P, k) / 2^P en flottant, par recurrence (P <= 60 : exact en double
// jusqu'a 2^53 pres, largement suffisant pour une p-valeur).
double binomial_tail(int P, int wins) {
  double p = 0.0;
  for (int k = wins; k <= P; ++k) {
    double c = 1.0;
    for (int i = 1; i <= k; ++i) c = c * (double)(P - k + i) / (double)i;
    p += c;
  }
  return p / std::ldexp(1.0, P);
}

double median_of(std::vector<double> v) {
  std::sort(v.begin(), v.end());
  const size_t m = v.size();
  return (m % 2 == 1) ? v[m / 2] : 0.5 * (v[m / 2 - 1] + v[m / 2]);
}

struct Timed {
  ForestResult r;
  double ms = 0;
};

Timed timed_fold(const std::vector<ForestEvent>& events, int threads) {
  Timed t;
  const auto t0 = std::chrono::steady_clock::now();
  t.r = build_forest(events, threads);
  t.ms = ms_since(t0);
  return t;
}

}  // namespace

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 4000, threads = 8, pairs = 10, K = 10;
  i64 s = 8;
  long long seed = 3;
  double min_ratio = 1.0;
  bool gate = false;
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) { if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2; }
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else if (arg.rfind("--pairs=", 0) == 0) pairs = std::atoi(arg.c_str() + 8);
    else if (arg.rfind("--K=", 0) == 0) K = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--s=", 0) == 0) s = std::atoll(arg.c_str() + 4);
    else if (arg.rfind("--seed=", 0) == 0) seed = std::atoll(arg.c_str() + 7);
    else if (arg.rfind("--min-ratio=", 0) == 0) min_ratio = std::atof(arg.c_str() + 12);
    else if (arg == "--gate") gate = true;
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else { std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str()); return 2; }
  }
  if (n < 2 || threads < 2 || pairs < 1 || K < 1 || K > (int)kSmaxProfile - 1 || s < 1 || pairs > 60 ||
      !(min_ratio > 0.0)) {
    std::fprintf(stderr, "REFUS : n >= 2, threads >= 2, 1 <= pairs <= 60, 1 <= K <= %d, s >= 1, min-ratio > 0\n",
                 (int)kSmaxProfile - 1);
    return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", inject.c_str());
    return 2;
  }
  if constexpr (!HasWorkers<ForestResult>) {
    std::fprintf(stderr, "REFUS : ForestResult::workers absent — les ouvriers du fold ne sont pas mesurables\n");
    return 2;
  }

  // 1. Evenements du K vise, copies depuis le callback provisoire.
  const int coord = cloud_family_default_coord(family, n);
  const std::vector<InputPoint> in = make_family_input(family, n, coord, seed);
  std::vector<ForestEvent> events;
  bool captured = false;
  RunOptions o;
  o.s = s;
  o.threads = threads;
  o.on_forest = [&](u64 k, const std::vector<ForestEvent>& ev, const ForestResult&) {
    if (k == (u64)K) {
      events = ev;
      captured = true;
    }
  };
  const auto t_pipe = std::chrono::steady_clock::now();
  const RunResult rr = run_pipeline(in, o);
  const double t_pipe_ms = ms_since(t_pipe);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "REFUS : pipeline non complet (%s)\n", rr.message.c_str());
    return 2;
  }
  if (!captured || (u64)K > rr.kmax_eff) {
    std::fprintf(stderr, "REFUS : ordre K=%d absent (kmax_eff=%llu)\n", K, (unsigned long long)rr.kmax_eff);
    return 2;
  }
  if (events.empty()) {
    std::fprintf(stderr, "REFUS : aucun evenement a l'ordre K=%d\n", K);
    return 2;
  }
  std::printf("fold_bench famille=%s n=%d coord=%d s=%lld seed=%lld K=%d evenements=%zu fils=%d paires=%d "
              "pipeline_ms=%.1f\n",
              cloud_family_name(family), n, coord, (long long)s, seed, K, events.size(), threads, pairs, t_pipe_ms);

  // Les trois exigences de validite, verifiees sur CHAQUE appel apparie.
  int bad = 0;
  u64 wb_min = UINT64_MAX, wb_max = 0;
  const auto check_pair = [&](const char* tag, const Timed& a, const Timed& b) {
    const std::string da = digest_forest_v4((u32)K, a.r), db = digest_forest_v4((u32)K, b.r);
    if (da != db) {
      std::fprintf(stderr, "INVARIANT (%s) : signatures differentes A=%s B=%s\n", tag, da.c_str(), db.c_str());
      ++bad;
    }
    if (!a.r.refusal.empty() || !b.r.refusal.empty()) {
      std::fprintf(stderr, "INVARIANT (%s) : refus de fold apres la garde amont\n", tag);
      ++bad;
    }
    const u64 wa = measured_workers(a.r), wb = measured_workers(b.r);
    if (wa != 1) {
      std::fprintf(stderr, "INVARIANT (%s) : A (threads=1) a cree %llu ouvriers, attendu 1\n", tag, (unsigned long long)wa);
      ++bad;
    }
    if (wb < 2) {
      std::fprintf(stderr, "INVARIANT (%s) : B (threads=%d) a cree %llu ouvrier(s) — parallelisme non exerce\n", tag,
                   threads, (unsigned long long)wb);
      ++bad;
    }
    if (wb > (u64)threads) {
      std::fprintf(stderr, "INVARIANT (%s) : B a cree %llu ouvriers, plus que demandes (%d)\n", tag,
                   (unsigned long long)wb, threads);
      ++bad;
    }
    wb_min = std::min(wb_min, wb);
    wb_max = std::max(wb_max, wb);
    if (!(a.ms > 0.0) || !(b.ms > 0.0)) {
      std::fprintf(stderr, "REFUS (%s) : chronometre nul, n trop petit pour un banc\n", tag);
      ++bad;
    }
  };

  // 2. Echauffement, une paire non comptee (A puis B).
  {
    const Timed a = timed_fold(events, 1);
    const Timed b = timed_fold(events, threads);
    std::printf("echauffement (non comptee) ordre=AB tA_ms=%.1f tB_ms=%.1f ouvriers_B=%llu\n", a.ms, b.ms,
                (unsigned long long)measured_workers(b.r));
    check_pair("echauffement", a, b);
  }

  // 3. P paires ABBA : AB, BA, AB, ...
  std::vector<double> ratios;
  ratios.reserve((size_t)pairs);
  int wins = 0;
  for (int p = 0; p < pairs; ++p) {
    const bool ab = (p % 2 == 0);
    Timed a, b;
    if (ab) {
      a = timed_fold(events, 1);
      b = timed_fold(events, threads);
    } else {
      b = timed_fold(events, threads);
      a = timed_fold(events, 1);
    }
    const double ratio = a.ms / b.ms;
    ratios.push_back(ratio);
    if (b.ms < a.ms) ++wins;
    std::printf("paire %d ordre=%s tA_ms=%.1f tB_ms=%.1f rapport=%.4f ouvriers_B=%llu\n", p + 1, ab ? "AB" : "BA",
                a.ms, b.ms, ratio, (unsigned long long)measured_workers(b.r));
    char tag[32];
    std::snprintf(tag, sizeof tag, "paire %d", p + 1);
    check_pair(tag, a, b);
  }

  // 4. Statistique appariee.
  const double median = median_of(ratios);
  const double pval = binomial_tail(pairs, wins);
  std::printf("fold_bench famille=%s n=%d K=%d paires=%d mediane=%.4f victoires=%d/%d p=%.3g ouvriers_B=%llu..%llu\n",
              cloud_family_name(family), n, K, pairs, median, wins, pairs, pval, (unsigned long long)wb_min,
              (unsigned long long)wb_max);
  if (!inject.empty()) {
    if (bad) {
      std::fprintf(stderr, "MUTANT TUE : %s (%d exigence(s) de validite violee(s))\n", inject.c_str(), bad);
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (bad) {
    std::fprintf(stderr, "INVARIANT : %d exigence(s) de validite violee(s) — le banc ne mesure rien\n", bad);
    return 3;
  }
  if (gate && median < min_ratio) {
    std::fprintf(stderr, "PLANCHER : mediane %.4f < --min-ratio %.4f\n", median, min_ratio);
    return 3;
  }
  std::printf("fold_bench OK\n");
  return 0;
}
