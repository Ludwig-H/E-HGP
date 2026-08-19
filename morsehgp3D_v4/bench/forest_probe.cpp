// MorseHGP3D v4 — PROBE DE FORET SUR FLUX REELS : les trois lanes WSPD
// alimentent le SpherePlateau d'echelle puis le fold a macro-lots.
//
// Chaine : WSPD (q2/q3/q4, generateurs seulement) -> sort/RLE par BallKey
// primitive inter-lanes -> UN census exact par cle (forme (A,B,C) uniforme,
// descente d'arbre separable par axe), collectant I_B ET U_B COMPLETS ->
// expansion des plateaux (§ 5.3bis, roles § 5.2) -> ForestEvents par K ->
// build_forest (macro-lots same_exact_level).
//
// FRONTIERE D'IDENTITE (audits e7e4d5e) : le census, l'arbre et le plateau
// vivent en GeometryIndex (rangs denses de `upos`, ordre Morton) ; la foret
// combinatoire vit en PointId EXTERNES. La conversion a lieu une seule fois,
// a l'entree de `forests_from_balls`, via la table `pid_of` — jamais par un
// cast du rang. La generation est aveugle aux ids (BallKey primitive) : la
// porte --relabel-gate le VERIFIE (equivariance au relabeling π, invariance
// a la permutation physique, aucune cle publique hors des ids fournis).
//
// JUGE (--judge, borne) : la MEME semantique depuis une enumeration brute
// aux predicats de production (toutes paires / triangles aigus / tetraedres
// centres) avec census brut point a point — il juge la COMPLETUDE WSPD, le
// census d'arbre et le RLE. Sa table geometry_index -> id est reconstruite
// INDEPENDAMMENT depuis les enregistrements d'entree (position -> id), sans
// appeler la conversion du sujet. Compare par K : nombre d'evenements, lots,
// multiensemble des nœuds, deltas de composantes, attachements nes au lot,
// partition finale.
// Codes : 0 conforme, 1 desaccord, 2 refus (dont resource_exhausted),
// 3 invariant/plancher, 4 mutant tue (--inject=rle-drop | census-nonstrict |
// dense-pointid).
#include <sched.h>

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <cfenv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/forest/forest.hpp"
#include "../src/forest/render.hpp"
#include "../src/forest/sphere_plateau.hpp"
#include "../src/pipeline/ball_stream.hpp"
#include "../src/util/sha256.hpp"

namespace {

using namespace mhgp4;

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool family_ok = true;
  int n = 120;
  int coord = 0;
  long long seed = 3;
  i64 s = 8;
  u64 smax = 11;
  size_t shell_cap = 12;
  bool judge = false;
  bool relabel_gate = false;
  bool depth_gate = false;
  bool kmax_gate = false;
  int guard = 0;  // 1 = dup-id, 2 = coord-range
  bool inj_rle_drop = false;
  bool inj_census_nonstrict = false;
  bool inj_dense_pointid = false;
  bool inj_threshold_minus_one = false;
  bool inj_range_add_le = false;
  bool inj_skip_full = false;
  bool inj_shell_first = false;
  bool inj_fold_kmax10 = false;
  bool inj_genfilter_nonstrict = false;
  bool axial_on = false;  // opt-in GPU-oriente ; production CPU = baseline
  bool axial_pair_gate = false;
  bool axial_sweep_gate = false;
  bool par_gate = false;
  int threads = 1;
  bool inj_par_drop = false;
  bool inj_par_drop_census = false;
  bool preflight = false;      // compter la sortie sans la materialiser
  bool q2_birth_gate = false;  // oracle Poisson : injection des facettes
  bool inj_birth_dup = false;
  u64 max_output_bytes = 0;  // 0 = pas de plafond ; sinon refus AVANT ev_k
  bool fold_compact_gate = false;
  bool inj_canon_root = false;
  bool float_gate = false;
  bool q3_affine_gate = false;
  bool float_rounding_gate = false;
  bool inj_jung_swap = false;
  bool fold_capacity_gate = false;
  bool digest = false;  // signature canonique (audit 9223888 § 2.2)
  bool workers_gate = false;
  bool inj_par_one_worker = false;
  bool inj_ranges_one_worker = false;
  bool inj_q3_one_worker = false;
  bool inj_wspd_one_worker = false;
  bool q4_eq_prefilter = true;  // prefiltre par puissance equatoriale
  bool q4_eq_gate = false;
  bool q4_prefilter_bench = false;
  bool inj_q4_eq_wrong = false;
  bool digest_gate = false;
  int inj_fold_capacity = 0;  // 1 = u32-event-wrap, 2 = i32-fid-wrap,
                              // 3 = epoch-sentinel-collision
  int inj_fold_intern = 0;    // 1 = fid en ordre de rencontre,
                              // 2 = empreinte sans verification de cle
  int inj_detector = 0;       // 1 = detecteur desactive,
                              // 2 = `seen` marque AVANT le controle
  u32 inj_axial = 0;  // masque kAxial* des mutants du chemin axial
  u64 min_balls = 0;
  u64 min_fusions = 0;
  bool intern_bench = false;  // banc d'alternance des deux internements
  bool bench_report_gate = false;
  bool schedule_bench = false;
  u64 fold_budget = 2ull << 30;  // 2 Gio par defaut
  int bench_repeat = 10;
};

bool parse_family(const char* name, CloudFamily* out) {
  const CloudFamily all[] = {CloudFamily::kUniform,
                             CloudFamily::kTerrain,
                             CloudFamily::kScanlineSinglePass,
                             CloudFamily::kScanlineOverlapMultiecho,
                             CloudFamily::kEightClusters,
                             CloudFamily::kTwoLines,
                             CloudFamily::kCollinearSeven};
  for (const CloudFamily f : all)
    if (std::strcmp(name, cloud_family_name(f)) == 0) {
      *out = f;
      return true;
    }
  return false;
}

Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--family=")) a.family_ok = parse_family(v, &a.family);
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--s=")) a.s = std::atoll(v);
    else if (const char* v = val("--smax=")) a.smax = (u64)std::atoll(v);
    else if (const char* v = val("--shell-cap=")) a.shell_cap = (size_t)std::atoll(v);
    else if (const char* v = val("--min-balls=")) a.min_balls = (u64)std::atoll(v);
    else if (const char* v = val("--min-fusions=")) a.min_fusions = (u64)std::atoll(v);
    else if (arg == "--judge") a.judge = true;
    else if (arg == "--relabel-gate") a.relabel_gate = true;
    else if (arg == "--depth-gate") a.depth_gate = true;
    else if (arg == "--kmax-gate") a.kmax_gate = true;
    else if (arg == "--guard=dup-id") a.guard = 1;
    else if (arg == "--guard=coord-range") a.guard = 2;
    else if (arg == "--inject=rle-drop") a.inj_rle_drop = true;
    else if (arg == "--inject=census-nonstrict") a.inj_census_nonstrict = true;
    else if (arg == "--inject=dense-pointid") a.inj_dense_pointid = true;
    else if (arg == "--inject=threshold-minus-one")
      a.inj_threshold_minus_one = true;
    else if (arg == "--inject=range-add-max-le-zero") a.inj_range_add_le = true;
    else if (arg == "--inject=skip-full-census") a.inj_skip_full = true;
    else if (arg == "--inject=shell-cap-before-depth") a.inj_shell_first = true;
    else if (arg == "--inject=fold-hardcodes-kmax10") a.inj_fold_kmax10 = true;
    else if (arg == "--inject=genfilter-nonstrict")
      a.inj_genfilter_nonstrict = true;
    else if (arg == "--axial-on") a.axial_on = true;
    else if (arg == "--axial-pair-gate") a.axial_pair_gate = true;
    else if (arg == "--axial-sweep-gate") a.axial_sweep_gate = true;
    else if (arg == "--par-gate") a.par_gate = true;
    else if (const char* v = val("--threads=")) a.threads = std::atoi(v);
    else if (arg == "--inject=par-drop-shard") a.inj_par_drop = true;
    else if (arg == "--inject=par-drop-ball-chunk")
      a.inj_par_drop_census = true;
    else if (arg == "--output-preflight-only") a.preflight = true;
    else if (arg == "--q2-birth-gate") a.q2_birth_gate = true;
    else if (arg == "--inject=birth-dup-tau") a.inj_birth_dup = true;
    else if (const char* v = val("--max-output-bytes="))
      a.max_output_bytes = (u64)std::atoll(v);
    else if (arg == "--fold-compact-gate") a.fold_compact_gate = true;
    else if (arg == "--inject=canonical-is-uf-root") a.inj_canon_root = true;
    else if (arg == "--float-gate") a.float_gate = true;
    else if (arg == "--q3-affine-gate") a.q3_affine_gate = true;
    else if (arg == "--float-rounding-gate") a.float_rounding_gate = true;
    else if (arg == "--inject=jung-swap-bounds") a.inj_jung_swap = true;
    else if (arg == "--fold-capacity-gate") a.fold_capacity_gate = true;
    else if (arg == "--digest") a.digest = true;
    else if (arg == "--workers-gate") a.workers_gate = true;
    else if (arg == "--digest-gate") a.digest_gate = true;
    else if (arg == "--inject=parallel-hardcodes-one-worker")
      a.inj_par_one_worker = true;
    else if (arg == "--inject=parallel-ranges-one-worker")
      a.inj_ranges_one_worker = true;
    else if (arg == "--inject=q3-one-worker") a.inj_q3_one_worker = true;
    else if (arg == "--inject=wspd-one-worker") a.inj_wspd_one_worker = true;
    else if (arg == "--q4-no-eq-prefilter") a.q4_eq_prefilter = false;
    else if (arg == "--q4-eq-gate") a.q4_eq_gate = true;
    else if (arg == "--q4-prefilter-bench") a.q4_prefilter_bench = true;
    else if (arg == "--inject=q4-eq-wrong-length") a.inj_q4_eq_wrong = true;
    else if (arg == "--inject=fold-u32-event-wrap") a.inj_fold_capacity = 1;
    else if (arg == "--inject=fold-i32-fid-wrap") a.inj_fold_capacity = 2;
    else if (arg == "--inject=fold-epoch-sentinel-collision")
      a.inj_fold_capacity = 3;
    else if (arg == "--fold-intern-bench") a.intern_bench = true;
    else if (arg == "--bench-report-gate") a.bench_report_gate = true;
    else if (arg == "--fold-schedule-bench") a.schedule_bench = true;
    else if (const char* v = val("--fold-memory-budget="))
      a.fold_budget = (u64)std::atoll(v);
    else if (const char* v = val("--bench-repeat=")) a.bench_repeat = std::atoi(v);
    else if (arg == "--inject=intern-fid-first-seen") a.inj_fold_intern = 1;
    else if (arg == "--inject=intern-hash-no-verify") a.inj_fold_intern = 2;
    else if (arg == "--inject=attach-detector-disabled") a.inj_detector = 1;
    else if (arg == "--inject=seen-before-check") a.inj_detector = 2;
    else if (arg == "--inject=float-ignore-rounding")
      a.inj_axial |= kFloatIgnoreRounding;
    else if (arg == "--inject=float-threshold-too-small")
      a.inj_axial |= kFloatSmallThreshold;
    else if (arg == "--inject=axial-short-group") a.inj_axial |= kAxialShortGroup;
    else if (arg == "--inject=axial-drop-ties") a.inj_axial |= kAxialDropTies;
    else if (arg == "--inject=axial-first-rep") a.inj_axial |= kAxialFirstRep;
    else if (arg == "--inject=axial-ignore-opposite-side")
      a.inj_axial |= kAxialIgnoreOpposite;
    else if (arg == "--inject=axial-depth-nonstrict")
      a.inj_axial |= kAxialDepthNonstrict;
    else if (arg == "--inject=axial-reverse-negative")
      a.inj_axial |= kAxialReverseNeg;
    else if (arg == "--inject=seed-core-nonstrict")
      a.inj_axial |= kAxialSeedCoreNonstrict;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.family_ok = false;
    }
  }
  return a;
}

struct BallData {
  Q3BallKey key;
  Q4Level level;
  std::vector<i32> interior, shell;
};

// Generateurs WSPD -> tri -> RLE par BallKey (arite minimale d'abord).
void collect_rle(const CloudIndex& ix, i64 s, u64 smax_eff, bool inj_rle_drop,
                 std::vector<BallCandidate>* cands, BallStreamStats* st) {
  collect_candidate_balls(ix, s, smax_eff, cands, st);
  std::stable_sort(cands->begin(), cands->end(), ball_candidate_less);
  if (!inj_rle_drop)  // MUTANT : dedupe saute, boules re-censusees
    cands->erase(std::unique(cands->begin(), cands->end(),
                             [](const BallCandidate& x, const BallCandidate& y) {
                               return x.key == y.key;
                             }),
                 cands->end());
  st->unique_balls = cands->size();
}

// PASSE 1 (audit « prefiltre exact par boule ») : count-only par cle,
// seuil de mort h_qmin = 12 - q_min par ARITE MINIMALE du groupe RLE
// (le premier candidat du groupe porte q_min : le tri met l'arite avant
// la representation). La regle par arite est JUGEE, pas supposee : le
// juge brut garde ses boules au plafond uniforme et les expanse — un
// label q_min faux (completude de lane violee) donnerait des evenements
// que le sujet n'a pas. MUTANTS : threshold-minus-one (mort a h-1 :
// perd les evenements de bord K=10), range-add-max-le-zero (coquilles
// comptees interieures : boules a plateau tuees a tort).
struct Survivor {
  size_t idx;
  u64 depth;  // compte EXACT des interieurs stricts (recoupe en passe 2)
};

// PARALLELISME DE L'AVAL : decoupage en TRANCHES CONTIGUES d'indices —
// la fusion en ordre de tranche rend la sortie BIT-IDENTIQUE au
// sequentiel quel que soit le nombre de fils (le prefiltre est
// independant par candidat, le census par survivante, l'expansion par
// boule, les folds par K). threads <= 1 : chemin sequentiel pur.
// Plan du decoupage (dimensionne les tampons des appelants) — la
// primitive elle-meme RETOURNE le nombre de workers reellement crees
// (audit 7d921ff § 2) : les stats publient la valeur retournee, jamais
// le plan. MUTANT parallel-ranges-hardcodes-one-worker : la primitive
// serialise tout l'aval sans toucher CLI ni digests (les tranches
// vides des tampons plans fusionnent a l'identique) — seule la mesure
// le trahit.
inline size_t planned_workers(size_t n, int requested) {
  return n == 0 ? 1 : std::min((size_t)std::max(requested, 1), n);
}
bool g_inj_parallel_ranges_one = false;  // MUTANT (7d921ff § 4.1)

// Affinite CPU EFFECTIVE du processus (audit c9c3a48) : mesuree par le
// processus lui-meme via sched_getaffinity — jamais recopiee d'une
// intention (cpu_set du runner). Rend le nombre de CPU autorises et le
// masque canonique en plages ("0-47", "0-3,5"). -1 si illisible.
int effective_affinity(std::string* mask_out) {
  cpu_set_t m;
  CPU_ZERO(&m);
  if (sched_getaffinity(0, sizeof(m), &m) != 0) return -1;
  int count = 0;
  std::string txt;
  int run_start = -1;
  for (int c = 0; c <= CPU_SETSIZE; ++c) {
    const bool on = c < CPU_SETSIZE && CPU_ISSET(c, &m);
    if (on) {
      ++count;
      if (run_start < 0) run_start = c;
    } else if (run_start >= 0) {
      if (!txt.empty()) txt += ',';
      txt += std::to_string(run_start);
      if (c - 1 > run_start) {
        txt += '-';
        txt += std::to_string(c - 1);
      }
      run_start = -1;
    }
  }
  *mask_out = txt;
  return count;
}

template <typename Fn>
size_t parallel_ranges(size_t n, int threads, Fn&& fn) {
  size_t T = planned_workers(n, threads);
  if (g_inj_parallel_ranges_one) T = 1;  // MUTANT
  if (T <= 1) {
    fn((size_t)0, n, (size_t)0);
    return n == 0 ? 0 : 1;
  }
  std::vector<std::thread> pool;
  pool.reserve(T);
  for (size_t t = 0; t < T; ++t) {
    const size_t b = n * t / T, e = n * (t + 1) / T;
    pool.emplace_back([&fn, b, e, t] { fn(b, e, t); });
  }
  const size_t actual = pool.size();
  for (auto& th : pool) th.join();
  return actual;
}

// ---- ORDONNANCEMENT DES DIX FOLDS (reponse d'audit `95061c1` § 3) ----
// Les folds par K ont des couts tres inegaux : a n=8000 les incidences
// se repartissent en 0,2 / 0,7 / 1,6 / 3,1 / 5,2 / 8,1 / 11,9 / 16,7 /
// 22,6 / 29,7 % pour K=1..10. Le decoupage CONTIGU de `parallel_ranges`
// donne alors {1,2} / {3,4,5} / {6,7} / {8,9,10} : un ouvrier porte 69 %
// du travail. Mais lancer naivement les quatre K les plus lourds
// ensemble est incompatible avec le contrat memoire — d'ou un
// ordonnanceur A BUDGET, et non un choix binaire.
//
// MAJORANT DES OCTETS TEMPORAIRES d'un fold, calculable AVANT lancement
// (toutes les tailles internes sont majorees par le nombre d'incidences
// W, puisque l'internement dedoublonne : nfid <= W).
u64 fold_bytes_upper(const std::vector<ForestEvent>& ev) {
  u64 W = 0;
  for (const ForestEvent& e : ev) W += (u64)e.q + e.d;
  const u64 E = (u64)ev.size();
  u64 cap = 1024;
  while (cap < W * 2 + 2) cap <<= 1;
  const u64 kf = (u64)sizeof(FacetKey);
  return cap * 8                 // table d'internement (TOUCHEE)
       + 4 * 11 * E              // ev_fid
       + W * (kf + 4)            // pool (cle, tid)
       + W * 4                   // rank
       + W * kf                  // keys
       + W * 4                   // union-find
       + W * 4                   // canon_fid
       + W * (4 + 1 + 4 + 4 + 4 + 4)  // epoques et roles
       + W                       // seen
       + W * (kf + 4);           // sortie dense
}

// Pic de RSS du processus, et remise a zero du compteur (Linux) : le
// noyau ne fait que MONTER VmHWM, donc comparer trois modes dans le meme
// processus exige de le remettre au RSS courant entre les modes.
u64 peak_rss_kib() {
  std::FILE* f = std::fopen("/proc/self/status", "r");
  if (!f) return 0;
  char line[256];
  u64 v = 0;
  while (std::fgets(line, sizeof(line), f))
    if (std::strncmp(line, "VmHWM:", 6) == 0) {
      v = (u64)std::strtoull(line + 6, nullptr, 10);
      break;
    }
  std::fclose(f);
  return v;
}
void reset_peak_rss() {
  std::FILE* f = std::fopen("/proc/self/clear_refs", "w");
  if (!f) return;
  std::fputs("5\n", f);
  std::fclose(f);
}

// Ordonnanceur a BUDGET MEMOIRE : taches par W decroissant (departage
// deterministe par K), un ouvrier ne demarre une tache que si son
// majorant tient dans le budget restant. GARDE ANTI-BLOCAGE explicite :
// une tache seule plus grosse que le budget entier s'execute quand rien
// d'autre ne tourne — refuser serait un interblocage, tronquer serait un
// mensonge.
struct FoldSchedule {
  size_t workers = 0;
  double wall_ms = 0;
  u64 peak_rss_kib_after = 0;
  u64 max_reserved = 0;
};
template <typename Fn>
FoldSchedule run_folds_budgeted(const std::vector<u64>& weights,
                                const std::vector<u64>& bytes, int threads,
                                u64 budget, Fn&& fn) {
  FoldSchedule out;
  std::vector<size_t> order(weights.size());
  for (size_t i = 0; i < order.size(); ++i) order[i] = i;
  std::stable_sort(order.begin(), order.end(), [&](size_t x, size_t y) {
    if (weights[x] != weights[y]) return weights[x] > weights[y];
    return x < y;
  });
  size_t T = planned_workers(order.size(), threads);
  if (g_inj_parallel_ranges_one) T = 1;  // MUTANT (meme primitive aval)
  if (T < 1) T = 1;
  std::mutex m;
  std::condition_variable cv;
  size_t next = 0;
  u64 reserved = 0, running = 0;
  const auto t0 = std::chrono::steady_clock::now();
  std::vector<std::thread> pool;
  pool.reserve(T);
  for (size_t t = 0; t < T; ++t)
    pool.emplace_back([&] {
      for (;;) {
        size_t idx = 0;
        {
          std::unique_lock<std::mutex> lk(m);
          for (;;) {
            if (next >= order.size()) return;
            idx = order[next];
            const bool fits = reserved + bytes[idx] <= budget;
            const bool alone = running == 0;  // garde anti-blocage
            if (fits || alone) {
              ++next;
              reserved += bytes[idx];
              ++running;
              if (reserved > out.max_reserved) out.max_reserved = reserved;
              break;
            }
            cv.wait(lk);
          }
        }
        fn(idx);
        {
          std::lock_guard<std::mutex> lk(m);
          reserved -= bytes[idx];
          --running;
        }
        cv.notify_all();
      }
    });
  out.workers = pool.size();
  for (auto& th : pool) th.join();
  out.wall_ms = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - t0)
                    .count();
  out.peak_rss_kib_after = peak_rss_kib();
  return out;
}


void prefilter_balls(const CloudIndex& ix,
                     const std::vector<BallCandidate>& cands, u64 smax_eff,
                     bool inj_threshold_minus_one, bool inj_range_add_le,
                     std::vector<Survivor>* survivors, BallStreamStats* st,
                     int threads = 1) {
  const size_t T = planned_workers(cands.size(), threads);
  std::vector<std::vector<Survivor>> lsv(T);
  std::vector<BallStreamStats> lst(T);
  const size_t actual_pf =
      parallel_ranges(cands.size(), threads,
                      [&](size_t b, size_t e, size_t t) {
    for (size_t i = b; i < e; ++i) {
      const BallCandidate& bc = cands[i];
      // Mort a |I_B| >= smax_eff + 1 - q_min (audit « smax dynamique » :
      // le parametre qui definit l'objet en amont existe aussi en aval).
      // Au profil maximal smax=11 : 10/9/8 interieurs pour q2/q3/q4.
      u64 h = smax_eff + 1 - (u64)bc.arity;
      if (inj_threshold_minus_one) --h;  // MUTANT
      u64 depth = 0;
      if (ball_depth_at_least(ix, bc.key, h, &depth, inj_range_add_le,
                              &lst[t])) {
        ++lst[t].balls_dead_depth;
        continue;
      }
      lsv[t].push_back({i, depth});
    }
  });
  st->prefilter_workers = std::max(st->prefilter_workers, (u64)actual_pf);
  survivors->reserve(cands.size() / 8 + 16);
  for (size_t t = 0; t < T; ++t) {
    survivors->insert(survivors->end(), lsv[t].begin(), lsv[t].end());
    st->add_from(lst[t]);
  }
}

// PASSE 2 : census complet I_B/U_B sur les SEULES survivantes, avec
// recoupement du compte de la passe 1 (invariant : passe1 == passe2).
// MUTANT skip-full-census : la passe count-only pretend suffire (I_B/U_B
// vides) — elle ne connait pas U_B, le juge voit les evenements manquants.
int census_balls(const CloudIndex& ix, const std::vector<BallCandidate>& cands,
                 const std::vector<Survivor>& survivors, u64 smax_eff,
                 size_t shell_cap, bool inj_census_nonstrict,
                 bool inj_skip_full, std::vector<BallData>* balls,
                 BallStreamStats* st, int threads = 1,
                 bool inj_drop_chunk = false) {
  const size_t T = planned_workers(survivors.size(), threads);
  std::vector<std::vector<BallData>> lb(T);
  std::vector<BallStreamStats> lst(T);
  // Un refus/invariant arrete la tranche ; le verdict fusionne est celui
  // de la tranche d'indices la plus BASSE (deterministe — la meme boule
  // qu'aurait vue le sequentiel), message imprime une seule fois.
  std::vector<int> lrc(T, 0);
  const size_t actual_cs = parallel_ranges(
      survivors.size(), threads, [&](size_t b, size_t e, size_t t) {
    for (size_t i = b; i < e; ++i) {
      const Survivor& sv = survivors[i];
      const BallCandidate& bc = cands[sv.idx];
      BallData bd;
      bd.key = bc.key;
      bd.level = bc.level;
      if (inj_skip_full) {  // MUTANT
        lb[t].push_back(std::move(bd));
        continue;
      }
      ++lst[t].full_census_keys;
      bool overflow = false;
      if (!ball_census(ix, bc.key, (size_t)(smax_eff - bc.arity), shell_cap,
                       &bd.interior, &bd.shell, &overflow)) {
        lrc[t] = overflow ? 2 : 3;
        return;
      }
      if (bd.interior.size() != (size_t)sv.depth) {
        lrc[t] = 3;
        return;
      }
      if (inj_census_nonstrict) {
        // MUTANT : la coquille comptee interieure (P <= 0).
        for (const i32 u : bd.shell) bd.interior.push_back(u);
        bd.shell.clear();
      }
      lst[t].census_interior += bd.interior.size();
      lst[t].census_shell += bd.shell.size();
      lb[t].push_back(std::move(bd));
    }
  });
  st->census_workers = std::max(st->census_workers, (u64)actual_cs);
  for (size_t t = 0; t < T; ++t)
    if (lrc[t] != 0) {
      if (lrc[t] == 2)
        std::fprintf(stderr,
                     "REFUS resource_exhausted : coquille > %zu (plafond "
                     "explicite, jamais de troncature)\n",
                     shell_cap);
      else
        std::fprintf(stderr,
                     "INVARIANT : census contredit la passe count-only ou "
                     "le compte passe1 != passe2\n");
      return lrc[t];
    }
  balls->reserve(survivors.size());
  for (size_t t = 0; t < T; ++t) {
    if (inj_drop_chunk && T > 1 && t == 0) continue;  // MUTANT
    for (auto& bd : lb[t]) balls->push_back(std::move(bd));
    st->add_from(lst[t]);
  }
  return 0;
}

// Foret par K depuis une liste de boules censusees : expansion + fold.
// `pid_of` : table GeometryIndex -> PointId externe — LA frontiere
// d'identite. `events_out` (optionnel) : evenements par K, en PointId.
int forests_from_balls(const std::vector<BallData>& balls,
                       const std::vector<P3>& pos,
                       const std::vector<PointId>& pid_of, u64 kmax_eff,
                       u64 per_k_events[11], ForestResult per_k_result[11],
                       std::vector<std::vector<ForestEvent>>* events_out = nullptr,
                       int threads = 1, bool legacy_partition = true,
                       BallStreamStats* wst = nullptr,
                       u64 fold_budget = 2ull << 30) {
  // PHASE A — expansion des plateaux, par TRANCHES de boules : chaque
  // ouvrier remplit ses propres listes par K, la fusion en ordre de
  // tranche restitue EXACTEMENT l'ordre sequentiel des evenements.
  const size_t T = planned_workers(balls.size(), threads);
  std::vector<std::vector<std::vector<ForestEvent>>> lev(
      T, std::vector<std::vector<ForestEvent>>(11));
  const size_t actual_ex = parallel_ranges(
      balls.size(), threads, [&](size_t bg, size_t en, size_t t) {
    std::vector<PlateauEvent> pevents;
    for (size_t bi = bg; bi < en; ++bi) {
      const BallData& b = balls[bi];
      const BallRat c = ball_center(b.key);
      pevents.clear();
      expand_plateau(c, pos, b.interior, b.shell, (size_t)(kmax_eff + 1),
                     &pevents);
      for (const PlateauEvent& pe : pevents) {
        const size_t K = pe.tpart.size() + pe.ipart.size() - 1;
        if (K < 1 || K > (size_t)kmax_eff) continue;
        ForestEvent ev;
        ev.q = (u8)pe.tpart.size();
        ev.d = (u8)pe.ipart.size();
        ev.active_mask = pe.active_mask;
        // Conversion GeometryIndex -> PointId ICI et seulement ici.
        // L'ordre de `support` reste celui de T (aligne sur active_mask —
        // ne jamais le retrier independamment du masque ; facet_minus trie
        // les FacetKey).
        for (size_t tt = 0; tt < pe.tpart.size(); ++tt)
          ev.support[tt] = pid_of[(size_t)pe.tpart[tt]];
        for (size_t tt = 0; tt < pe.ipart.size(); ++tt)
          ev.interior[tt] = pid_of[(size_t)pe.ipart[tt]];
        ev.level = b.level;
        lev[t][K].push_back(ev);
      }
    }
  });
  if (wst)
    wst->expansion_workers = std::max(wst->expansion_workers, (u64)actual_ex);
  std::vector<std::vector<ForestEvent>> ev_k(11);
  for (size_t t = 0; t < T; ++t)
    for (size_t K = 1; K <= (size_t)kmax_eff; ++K)
      ev_k[K].insert(ev_k[K].end(), lev[t][K].begin(), lev[t][K].end());
  // PHASE B — folds INDEPENDANTS par K (chaque K possede son build_forest
  // et son resultat ; le rapport de violation reste au plus petit K).
  // `legacy_partition` : la vue map n'est remplie que pour les
  // consommateurs qui la lisent (juge, portes) — le chemin d'echelle
  // vit sur la representation dense facet_keys + final_canon_fid.
  // ORDONNANCEMENT A BUDGET MEMOIRE (reponse d'audit `95061c1` § 3,
  // adopte APRES la comparaison intra-processus exigee — banc
  // `--fold-schedule-bench`, mesures au recu). Le decoupage contigu
  // donnait a un ouvrier 69 % du travail ; l'ordre decroissant sans
  // borne divise la latence par 2,80 mais reserve 4,32 Go, soit deux
  // fois le budget. Sous budget de 2 Gio : latence /1,40 pour +0,4 % de
  // pic RSS, et la reserve reste sous le plafond. Les sorties sont
  // INDEPENDANTES de l'ordonnancement (signature identique aux trois
  // modes) : seule la latence et le pic bougent.
  std::vector<u64> fold_w((size_t)kmax_eff, 0), fold_m((size_t)kmax_eff, 0);
  for (size_t i = 0; i < (size_t)kmax_eff; ++i) {
    for (const ForestEvent& e : ev_k[i + 1]) fold_w[i] += (u64)e.q + e.d;
    fold_m[i] = fold_bytes_upper(ev_k[i + 1]);
  }
  const FoldSchedule fsched =
      run_folds_budgeted(fold_w, fold_m, threads, fold_budget, [&](size_t i) {
        const int K = (int)i + 1;
        per_k_events[K] = ev_k[(size_t)K].size();
        per_k_result[K] = build_forest(ev_k[(size_t)K], false, false, nullptr,
                                       false, false, false, legacy_partition);
      });
  if (wst) {
    wst->fold_workers_max =
        std::max(wst->fold_workers_max, (u64)fsched.workers);
    wst->fold_budget_bytes = fold_budget;
    wst->fold_reserved_max =
        std::max(wst->fold_reserved_max, fsched.max_reserved);
  }
  for (int K = 1; K <= (int)kmax_eff; ++K)
    if (!per_k_result[K].refusal.empty()) {
      // Refus transactionnel de capacite (contre-audit 5d274a1 § 7) :
      // avant tout calcul du fold, code 2 — jamais une troncature.
      std::fprintf(stderr, "REFUS fold K=%d : %s\n", K,
                   per_k_result[K].refusal.c_str());
      return 2;
    }
  for (int K = 1; K <= (int)kmax_eff; ++K)
    if (per_k_result[K].attach_violations != 0 ||
        per_k_result[K].birth_violations != 0 ||
        per_k_result[K].partition_violations != 0) {
      std::fprintf(stderr,
                   "INVARIANT : violations de roles ou de partition (K=%d)\n",
                   K);
      return 3;
    }
  if (events_out) *events_out = std::move(ev_k);
  return 0;
}

// ------------------------------------------------------------------
// PORTE DE RELABELING (audits e7e4d5e § 6) : la sortie publique vit dans
// l'espace des PointId fournis par l'appelant, jamais dans les rangs Morton.
// Trois runs sur la MEME geometrie : ids brouilles A (non monotones, au-dela
// du bit 31), ids π(A) (bijection non monotone, positions fixes), permutation
// physique des couples (id, position). Exigences :
//   - BallKeys et niveaux inchanges (la generation est aveugle aux ids) ;
//   - ForestEvents de run1 = π(ForestEvents de run0), point a point ;
//   - blocs de la partition finale transportes par π (les REPRESENTANTS
//     canoniques sont des minima de FacetKey : equivariants par blocs,
//     pas point a point — on compare les blocs) ;
//   - deltas : multiensemble (lot, |parents|, facettes nees transportees) ;
//   - toute cle publique appartient a l'ensemble d'ids fourni ;
//   - le run permute est BIT-IDENTIQUE a run0.
// Le mutant dense-pointid (cast du rang, l'ancien code) doit mourir ici.
// ------------------------------------------------------------------

// Bijections u32 (multiplication impaire puis xor : inversibles, non
// monotones ; des valeurs au-dessus de 2^31 apparaissent).
u32 gate_base_id(u32 i) { return (i + 1u) * 0x9E3779B9u ^ 0x5A5A5A5Au; }
u32 gate_pi(u32 v) { return v * 0x85EBCA6Bu ^ 0xC2B2AE35u; }

struct GateOut {
  std::vector<Q3BallKey> keys;                   // apres RLE (ordre canonique)
  std::vector<std::vector<ForestEvent>> events;  // par K, en PointId
  ForestResult res[11];
};

int run_gate_chain(const std::vector<InputPoint>& in, bool dense_mutant,
                   GateOut* out) {
  const CloudIndex ix = build_cloud_index(in);
  if ((size_t)ix.unique_count() != in.size()) {
    std::fprintf(stderr, "REFUS unsupported_degeneracy : positions dupliquees\n");
    return 2;
  }
  std::vector<BallCandidate> cands;
  BallStreamStats st;
  collect_rle(ix, 8, std::min<u64>(11, in.size()), false, &cands, &st);
  std::vector<Survivor> surv;
  prefilter_balls(ix, cands, 11, false, false, &surv, &st);
  std::vector<BallData> balls;
  if (const int rc =
          census_balls(ix, cands, surv, 11, 12, false, false, &balls, &st))
    return rc;
  for (const BallData& b : balls) out->keys.push_back(b.key);
  std::vector<PointId> pid((size_t)ix.unique_count());
  for (size_t u = 0; u < pid.size(); ++u)
    pid[u] = dense_mutant ? (PointId)u : ix.point_id((i32)u);
  u64 ev[11] = {};
  return forests_from_balls(balls, ix.upos, pid, 10, ev, out->res,
                            &out->events);
}

bool gate_same_event(const ForestEvent& x, const ForestEvent& y) {
  if (x.q != y.q || x.d != y.d || x.active_mask != y.active_mask) return false;
  for (int t = 0; t < (int)x.q; ++t)
    if (x.support[t] != y.support[t]) return false;
  for (int t = 0; t < (int)x.d; ++t)
    if (x.interior[t] != y.interior[t]) return false;
  return same_level_representation(x.level, y.level);
}

FacetKey gate_pi_facet(FacetKey f) {
  for (int t = 0; t < (int)f.k; ++t) f.p[(size_t)t] = gate_pi(f.p[(size_t)t]);
  for (int t = 1; t < (int)f.k; ++t) {  // tri par insertion (tableau fixe)
    const PointId v = f.p[(size_t)t];
    int j = t - 1;
    while (j >= 0 && f.p[(size_t)j] > v) {
      f.p[(size_t)(j + 1)] = f.p[(size_t)j];
      --j;
    }
    f.p[(size_t)(j + 1)] = v;
  }
  return f;
}

// Blocs d'une partition facette -> racine, comme ensemble d'ensembles.
std::set<std::vector<FacetKey>> gate_blocks(
    const std::map<FacetKey, FacetKey>& fp) {
  std::map<FacetKey, std::vector<FacetKey>> by_root;
  for (const auto& kv : fp) by_root[kv.second].push_back(kv.first);
  std::set<std::vector<FacetKey>> out;
  for (auto& kv : by_root) out.insert(kv.second);  // deja triees (ordre map)
  return out;
}

int run_relabel_gate(bool dense_mutant) {
  // Nuage : uniforme n=36 decale en x (+64), plus le carre cocirculaire
  // {(0,0,0),(6,0,0),(0,6,0),(6,6,0)} — evenements q2/q3/q4 ET un plateau
  // spherique garantis (les deux diagonales partagent la boule R²=18).
  std::vector<P3> pts = make_family_cloud(CloudFamily::kUniform, 36, 40, 7);
  for (P3& p : pts) p.x += 64;
  pts.push_back(P3{0, 0, 0});
  pts.push_back(P3{6, 0, 0});
  pts.push_back(P3{0, 6, 0});
  pts.push_back(P3{6, 6, 0});
  const size_t n = pts.size();

  std::vector<InputPoint> in0(n), in1(n), in2(n);
  std::set<PointId> ids0, ids1;
  for (size_t i = 0; i < n; ++i) {
    const PointId base = gate_base_id((u32)i);
    in0[i] = {base, pts[i]};
    in1[i] = {gate_pi(base), pts[i]};  // relabeling π, positions fixes
    ids0.insert(base);
    ids1.insert(gate_pi(base));
  }
  if (ids0.size() != n || ids1.size() != n) {
    std::fprintf(stderr, "PLANCHER : ids de porte non distincts\n");
    return 3;
  }
  // Permutation physique deterministe des couples (id, position) de run0.
  for (size_t i = 0; i < n; ++i) in2[i] = in0[(i * 17 + 5) % n];

  GateOut o0, o1, o2;
  if (const int rc = run_gate_chain(in0, dense_mutant, &o0)) return rc;
  if (const int rc = run_gate_chain(in1, dense_mutant, &o1)) return rc;
  if (const int rc = run_gate_chain(in2, dense_mutant, &o2)) return rc;

  u64 bad = 0;
  const auto fail = [&](const char* what) {
    std::fprintf(stderr, "RELABEL : %s\n", what);
    ++bad;
  };

  // 1. Les BallKeys ne dependent pas des ids (generation aveugle).
  if (!(o0.keys == o1.keys)) fail("BallKeys changees par le relabeling");
  if (!(o0.keys == o2.keys)) fail("BallKeys changees par la permutation");

  // 2. Toute cle publique de run0 est un id fourni ; couverture > 2^31.
  PointId max_seen = 0;
  bool all_in = true;
  for (int K = 1; K <= 10; ++K) {
    for (const ForestEvent& e : o0.events[(size_t)K]) {
      for (int t = 0; t < (int)e.q; ++t) {
        all_in = all_in && ids0.count(e.support[t]) != 0;
        max_seen = std::max(max_seen, e.support[t]);
      }
      for (int t = 0; t < (int)e.d; ++t)
        all_in = all_in && ids0.count(e.interior[t]) != 0;
    }
    for (const auto& kv : o0.res[K].final_partition)
      for (int t = 0; t < (int)kv.first.k; ++t)
        all_in = all_in && ids0.count(kv.first.p[(size_t)t]) != 0;
    for (const ComponentDelta& cd : o0.res[K].deltas) {
      for (int t = 0; t < (int)cd.output.k; ++t)
        all_in = all_in && ids0.count(cd.output.p[(size_t)t]) != 0;
      for (const FacetKey& f : cd.born)
        for (int t = 0; t < (int)f.k; ++t)
          all_in = all_in && ids0.count(f.p[(size_t)t]) != 0;
    }
  }
  if (!all_in) fail("cle publique hors de l'ensemble d'ids fourni");
  if (max_seen < 0x80000000u) fail("aucun id au-dessus du bit 31 exerce");

  // 3. Evenements : run1 = π(run0) point a point (l'ordre est geometrique,
  //    identique entre runs) ; run2 = run0 exactement.
  for (int K = 1; K <= 10; ++K) {
    const auto &e0 = o0.events[(size_t)K], &e1 = o1.events[(size_t)K],
               &e2 = o2.events[(size_t)K];
    if (e0.size() != e1.size() || e0.size() != e2.size()) {
      fail("nombre d'evenements change");
      continue;
    }
    for (size_t i = 0; i < e0.size(); ++i) {
      ForestEvent t = e0[i];
      for (int v = 0; v < (int)t.q; ++v) t.support[v] = gate_pi(t.support[v]);
      for (int v = 0; v < (int)t.d; ++v) t.interior[v] = gate_pi(t.interior[v]);
      if (!gate_same_event(t, e1[i])) fail("evenement non transporte par π");
      if (!gate_same_event(e0[i], e2[i])) fail("evenement change par permutation");
    }
  }

  // 4. Partitions finales : blocs transportes par π ; run2 bit-identique.
  //    (Les representants canoniques sont des minima : compares via blocs.)
  for (int K = 1; K <= 10; ++K) {
    std::set<std::vector<FacetKey>> b0t;
    for (const std::vector<FacetKey>& blk : gate_blocks(o0.res[K].final_partition)) {
      std::vector<FacetKey> t;
      t.reserve(blk.size());
      for (const FacetKey& f : blk) t.push_back(gate_pi_facet(f));
      std::sort(t.begin(), t.end());
      b0t.insert(t);
    }
    if (b0t != gate_blocks(o1.res[K].final_partition))
      fail("blocs de partition non transportes par π");
    if (!(o0.res[K].final_partition == o2.res[K].final_partition))
      fail("partition changee par permutation");
    if (!(o0.res[K].deltas == o2.res[K].deltas))
      fail("deltas changes par permutation");
  }

  // 5. Deltas : multiensemble (lot, |parents|, nees transportees) preserve.
  for (int K = 1; K <= 10; ++K) {
    using DSig = std::pair<std::pair<u64, size_t>, std::vector<FacetKey>>;
    std::vector<DSig> d0, d1;
    for (const ComponentDelta& cd : o0.res[K].deltas) {
      std::vector<FacetKey> born;
      born.reserve(cd.born.size());
      for (const FacetKey& f : cd.born) born.push_back(gate_pi_facet(f));
      std::sort(born.begin(), born.end());
      d0.push_back({{cd.batch, cd.parents.size()}, std::move(born)});
    }
    for (const ComponentDelta& cd : o1.res[K].deltas)
      d1.push_back({{cd.batch, cd.parents.size()}, cd.born});
    std::sort(d0.begin(), d0.end());
    std::sort(d1.begin(), d1.end());
    if (d0 != d1) fail("deltas non transportes par π");
  }

  // 6. Planchers contre le vert-par-vacuite : les trois arites, un plateau
  //    (facette d'attachement), une naissance, une fusion, une facette nee.
  u64 nq[5] = {}, attach_ev = 0, births = 0, merges = 0, borns = 0;
  for (int K = 1; K <= 10; ++K) {
    for (const ForestEvent& e : o0.events[(size_t)K]) {
      if (e.q >= 2 && e.q <= 4) ++nq[e.q];
      if (__builtin_popcount((unsigned)e.active_mask) < (int)e.q) ++attach_ev;
    }
    for (const ComponentDelta& cd : o0.res[K].deltas) {
      if (cd.parents.empty()) ++births;
      if (cd.parents.size() >= 2) ++merges;
      borns += cd.born.size();
    }
  }
  const bool floors_ok = nq[2] > 0 && nq[3] > 0 && nq[4] > 0 &&
                         attach_ev > 0 && births > 0 && merges > 0 && borns > 0;

  std::printf(
      "relabel_gate n=%zu boules=%zu q2=%llu q3=%llu q4=%llu attach=%llu "
      "naissances=%llu fusions=%llu nees=%llu violations=%llu\n",
      n, o0.keys.size(), (unsigned long long)nq[2], (unsigned long long)nq[3],
      (unsigned long long)nq[4], (unsigned long long)attach_ev,
      (unsigned long long)births, (unsigned long long)merges,
      (unsigned long long)borns, (unsigned long long)bad);

  if (dense_mutant) {
    if (bad > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (!floors_ok) {
    std::fprintf(stderr, "PLANCHER : couverture insuffisante\n");
    return 3;
  }
  return bad ? 3 : 0;
}

// ------------------------------------------------------------------
// PORTE DE PROFONDEUR (audit « prefiltre exact par boule » § 4) : mort
// EXACTE au seuil par arite, sans appeler le census complet ; les
// coquilles ne comptent jamais ; les jumelles a h-1 interieurs survivent
// avec le compte exact. Mutants : threshold-minus-one (les jumelles
// meurent a tort), range-add-max-le-zero (les coquilles comptent).
// ------------------------------------------------------------------
int run_depth_gate(bool inj_thr, bool inj_le, bool inj_shell_first) {
  u64 bad = 0;
  const auto expect = [&](bool cond, const char* what) {
    if (!cond) {
      std::fprintf(stderr, "PROFONDEUR : %s\n", what);
      ++bad;
    }
  };
  const auto h_eff = [&](u64 h) { return inj_thr ? h - 1 : h; };  // MUTANT
  {
    // q2 (h=10) : boule diametrale (0,0,0)-(40,0,0), R²=400 ; EXACTEMENT
    // 10 interieurs (20+k,1,0), k=-4..5.
    std::vector<P3> pts = {P3{0, 0, 0}, P3{40, 0, 0}};
    for (i64 kk = -4; kk <= 5; ++kk) pts.push_back(P3{20 + kk, 1, 0});
    const CloudIndex ix = build_cloud_index(pts);
    if ((size_t)ix.unique_count() != pts.size()) return 3;
    const Q3BallKey key = q2_ball_key(pts[0], pts[1]);
    u64 c = 0;
    expect(ball_depth_at_least(ix, key, h_eff(10), &c, inj_le),
           "q2 : 10 interieurs, morte a h=10 sans census");
    pts.pop_back();
    const CloudIndex ix2 = build_cloud_index(pts);
    u64 c2 = 0;
    const bool dead2 = ball_depth_at_least(ix2, key, h_eff(10), &c2, inj_le);
    expect(!dead2 && c2 == 9, "q2 : 9 interieurs, survit avec compte exact 9");
  }
  {
    // q3 (h=9) : circonscrite du triangle strictement aigu
    // (0,0,10),(40,0,10),(20,30,10) — centre (20,25/3,10), R²=4225/9 ;
    // EXACTEMENT 9 interieurs (20,8,10+z), z=-4..4 (dist² <= 145/9 ; le
    // plan est a z=10 pour rester dans le profil u16, garde d'entree).
    std::vector<P3> pts = {P3{0, 0, 10}, P3{40, 0, 10}, P3{20, 30, 10}};
    for (i64 zz = -4; zz <= 4; ++zz) pts.push_back(P3{20, 8, 10 + zz});
    const CloudIndex ix = build_cloud_index(pts);
    if ((size_t)ix.unique_count() != pts.size()) return 3;
    const Q3BallKey key = q3_ball_key(q3_form(pts[0], pts[1], pts[2]));
    u64 c = 0;
    expect(ball_depth_at_least(ix, key, h_eff(9), &c, inj_le),
           "q3 : 9 interieurs, morte a h=9 sans census");
    pts.pop_back();
    const CloudIndex ix2 = build_cloud_index(pts);
    u64 c2 = 0;
    const bool dead2 = ball_depth_at_least(ix2, key, h_eff(9), &c2, inj_le);
    expect(!dead2 && c2 == 8, "q3 : 8 interieurs, survit avec compte exact 8");
  }
  {
    // q4 (h=8) : circonscrite du tetraedre regulier (coins alternes du
    // cube 10..30) — centre (20,20,20), R²=300 ; EXACTEMENT 8 interieurs
    // (20+k,20,20), k=-3..4.
    std::vector<P3> pts = {P3{30, 30, 30}, P3{10, 10, 30}, P3{10, 30, 10},
                           P3{30, 10, 10}};
    for (i64 kk = -3; kk <= 4; ++kk) pts.push_back(P3{20 + kk, 20, 20});
    const CloudIndex ix = build_cloud_index(pts);
    if ((size_t)ix.unique_count() != pts.size()) return 3;
    const Q4Form f4 = q4_form(pts[0], pts[1], pts[2], pts[3]);
    if (f4.det == 0) return 3;
    const Q3BallKey key = q3_ball_key_reduce(q4_ball_form(f4));
    u64 c = 0;
    expect(ball_depth_at_least(ix, key, h_eff(8), &c, inj_le),
           "q4 : 8 interieurs, morte a h=8 sans census");
    pts.pop_back();
    const CloudIndex ix2 = build_cloud_index(pts);
    u64 c2 = 0;
    const bool dead2 = ball_depth_at_least(ix2, key, h_eff(8), &c2, inj_le);
    expect(!dead2 && c2 == 7, "q4 : 7 interieurs, survit avec compte exact 7");
  }
  {
    // Coquille jamais comptee : boule du diametre (110,100,100)-(90,100,100)
    // (R²=100), DEUX points SUR la sphere et 9 interieurs (100+k,101,100),
    // k=-4..4 — survit a h=10 avec compte 9. Le mutant range-add-max-le-zero
    // compte les coquilles et la tue a tort.
    std::vector<P3> pts = {P3{110, 100, 100}, P3{90, 100, 100},
                           P3{100, 110, 100}, P3{100, 90, 100}};
    for (i64 kk = -4; kk <= 4; ++kk) pts.push_back(P3{100 + kk, 101, 100});
    const CloudIndex ix = build_cloud_index(pts);
    if ((size_t)ix.unique_count() != pts.size()) return 3;
    const Q3BallKey key = q2_ball_key(pts[0], pts[1]);
    u64 c = 0;
    const bool dead = ball_depth_at_least(ix, key, h_eff(10), &c, inj_le);
    expect(!dead && c == 9,
           "coquille : points SUR la sphere jamais comptes (survit a 9)");
  }
  {
    // DOUBLE DEBORDEMENT (audit « profondeur avant coquille ») : 10
    // interieurs (Morton bas) ET 15 points de coquille (Morton haut) sur
    // la boule R²=50 centree (100,100,100). Le verdict est dead_depth PAR
    // LA PASSE 1 — jamais resource_exhausted, quel que soit l'ordre des
    // sous-arbres. Le mutant shell-cap-before-depth (l'ancien census a
    // une passe, DFS droite d'abord = Morton decroissant) rencontre la
    // 13e coquille avant le 10e interieur et rend le mauvais statut.
    std::vector<P3> pts;
    const i64 sh[15][3] = {{5, 5, 0}, {5, 0, 5}, {0, 5, 5}, {7, 1, 0},
                           {7, 0, 1}, {1, 7, 0}, {0, 7, 1}, {1, 0, 7},
                           {0, 1, 7}, {5, 4, 3}, {5, 3, 4}, {4, 5, 3},
                           {3, 5, 4}, {4, 3, 5}, {3, 4, 5}};
    for (const auto& s : sh) pts.push_back(P3{100 + s[0], 100 + s[1], 100 + s[2]});
    for (i64 kk = 0; kk <= 6; ++kk) pts.push_back(P3{100 - kk, 99, 100});
    pts.push_back(P3{99, 98, 100});
    pts.push_back(P3{98, 98, 100});
    pts.push_back(P3{97, 98, 100});
    const CloudIndex ix = build_cloud_index(pts);
    if ((size_t)ix.unique_count() != pts.size()) return 3;
    const Q3BallKey key{1, {-200, -200, -200}, (i128)100 * 100 * 3 - 50};
    u64 c = 0;
    const bool dead = ball_depth_at_least(ix, key, h_eff(10), &c, inj_le);
    if (inj_shell_first) {
      std::vector<i32> in_, sh_;
      bool over = false;
      const bool ok = ball_census(ix, key, 9, 12, &in_, &sh_, &over);
      expect(ok || !over,
             "double debordement : dead_depth attendu, resource_exhausted "
             "rendu par le census-d'abord");
      (void)dead;
    } else {
      expect(dead, "double debordement : mort par profondeur en passe 1");
    }
    // Variante : profondeur 9 (< h) mais coquille 15 (> cap 12) — la
    // SURVIVANTE doit rendre resource_exhausted en passe 2.
    pts.pop_back();
    const CloudIndex ix2 = build_cloud_index(pts);
    if ((size_t)ix2.unique_count() != pts.size()) return 3;
    u64 c2 = 0;
    const bool dead2 = ball_depth_at_least(ix2, key, h_eff(10), &c2, inj_le);
    std::vector<i32> in2, sh2;
    bool over2 = false;
    const bool ok2 =
        dead2 ? true : ball_census(ix2, key, 9, 12, &in2, &sh2, &over2);
    expect(!dead2 && !ok2 && over2,
           "profondeur survivante + coquille > cap : resource_exhausted");
  }
  std::printf("depth_gate violations=%llu\n", (unsigned long long)bad);
  if (inj_thr || inj_le || inj_shell_first) {
    if (bad > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return bad ? 3 : 0;
}

// FIXTURE DE FRONTIERE K_max=5/6 (audit « smax dynamique » § 4) : la boule
// diametrale de ab (R²=100) a CINQ interieurs — son evenement est
// exactement d'ordre K=6. smax=6 (K_max=5) : la boule meurt au prefiltre
// (5 >= 6+1-2), aucune sortie K=6 ; smax=7 (K_max=6) : l'evenement K=6
// est present au niveau 100. Le mutant fold-hardcodes-kmax10 retablit les
// constantes 9/11/10 de l'aval : K=6 apparait sous smax=6, tue. La porte
// est posee directement sur BallData -> expansion -> fold, sans dependre
// d'un certificat WSPD sur cette petite geometrie.
int run_kmax_gate(bool inj_kmax10) {
  u64 bad = 0;
  const auto expect = [&](bool cond, const char* what) {
    if (!cond) {
      std::fprintf(stderr, "KMAX : %s\n", what);
      ++bad;
    }
  };
  const std::vector<P3> pts = {P3{0, 10, 10}, P3{20, 10, 10}, P3{10, 10, 10},
                               P3{10, 11, 10}, P3{10, 9, 10},  P3{9, 10, 10},
                               P3{11, 10, 10}};
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) return 3;
  const std::vector<BallCandidate> cands = {BallCandidate{
      q2_ball_key(pts[0], pts[1]),
      promote_q3_level(q2_exact_level(p3_norm2(p3_sub(pts[1], pts[0])))), 2}};
  std::vector<PointId> pid((size_t)ix.unique_count());
  for (size_t u = 0; u < pid.size(); ++u) pid[u] = ix.point_id((i32)u);
  for (int cas = 0; cas < 2; ++cas) {
    const u64 smax_eff = (cas == 0) ? 6 : 7;
    const u64 smax_caps = inj_kmax10 ? 11 : smax_eff;  // MUTANT
    const u64 kmax_eff = inj_kmax10 ? 10 : smax_eff - 1;
    BallStreamStats st;
    std::vector<Survivor> surv;
    prefilter_balls(ix, cands, smax_caps, false, false, &surv, &st);
    std::vector<BallData> balls;
    if (census_balls(ix, cands, surv, smax_caps, 12, false, false, &balls,
                     &st))
      return 3;
    u64 ev[11] = {};
    ForestResult res[11];
    if (forests_from_balls(balls, ix.upos, pid, kmax_eff, ev, res)) return 3;
    const u64 k6 = (kmax_eff >= 6) ? ev[6] : 0;
    if (cas == 0) {
      expect(st.balls_dead_depth == 1 && k6 == 0,
             "smax=6 : boule ecartee au 5e interieur, aucune sortie K=6");
    } else {
      const bool lvl_ok =
          k6 == 1 && res[6].batch_levels.size() == 1 &&
          compare_exact_level(res[6].batch_levels[0],
                              promote_q3_level(q2_exact_level(400))) == 0;
      expect(lvl_ok, "smax=7 : evenement K=6 present au niveau R²=100");
    }
  }
  std::printf("kmax_gate violations=%llu\n", (unsigned long long)bad);
  if (inj_kmax10) {
    if (bad > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return bad ? 3 : 0;
}

// PORTE q2_birth_lower_bound (contre-audits Poisson q2, § portes) :
// ORACLE exhaustif borne (n = 200, O(n³) — la regle « jamais de
// verification exhaustive » exclut explicitement les oracles bornes qui
// ETABLISSENT la verite). Pour chaque paire {a,b} : j = interieurs
// STRICTS de la boule diametral ouverte (|2z−(a+b)|² < |b−a|², exact
// entier). Verifications de l'argument d'INJECTION du contre-audit,
// pour 1 <= j <= 9 : (a) {a,b} est le diametre unique de chaque
// tau_z = sigma∖{z} — toute autre paire de tau_z est STRICTEMENT plus
// courte, et c'est un THEOREME (durcissement d'audit) : les autres
// points vivent dans la boule OUVERTE de rayon D/2, une egalite exacte
// est donc impossible et toute occurrence est une VIOLATION ;
// (b) les tau_z propres sont globalement DISTINCTES
// (l'application (sigma, z) -> tau_z est injective) ; (c) planchers
// N_0, N_1, N_2 >= 1 contre le vert-par-vacuite ; (d) distinction K=1 :
// N_0 >= 2(n−1) — le nombre de certificats de Gabriel n'est pas le
// nombre d'aretes critiques (asymptote 4n contre n−1 au MST). MUTANT
// birth-dup-tau : un tau duplique est injecte dans le verificateur de
// distinction, qui doit le voir.
int run_q2_birth_gate(bool inj_dup) {
  const int n = 200;
  const std::vector<P3> pts = make_family_cloud(
      CloudFamily::kUniform, n, cloud_family_default_coord(CloudFamily::kUniform, n), 3);
  const CloudIndex ix = build_cloud_index(pts);
  const int m = ix.unique_count();
  if (m < 3) return 3;
  u64 nj[10] = {};
  u64 violations = 0;
  std::set<std::vector<i32>> taus;
  std::vector<i32> inter;
  for (i32 i = 0; i < m; ++i)
    for (i32 j2 = i + 1; j2 < m; ++j2) {
      const P3 &pa = ix.upos[(size_t)i], &pb = ix.upos[(size_t)j2];
      const i64 D2 = p3_norm2(p3_sub(pb, pa));
      if (D2 == 0) continue;
      const i64 c2[3] = {pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
      inter.clear();
      for (i32 z = 0; z < m && inter.size() <= 10; ++z) {
        if (z == i || z == j2) continue;
        const P3& pz = ix.upos[(size_t)z];
        const i64 dx = 2 * pz.x - c2[0], dy = 2 * pz.y - c2[1],
                  dz = 2 * pz.z - c2[2];
        if (dx * dx + dy * dy + dz * dz < D2) inter.push_back(z);
      }
      const size_t jj = inter.size();
      if (jj <= 9) ++nj[jj];
      if (jj < 1 || jj > 9) continue;
      // sigma = {a, b} ∪ inter ; pour chaque z omis : tau_z.
      for (size_t oz = 0; oz < jj; ++oz) {
        std::vector<i32> tau;
        tau.push_back(i);
        tau.push_back(j2);
        for (size_t t = 0; t < jj; ++t)
          if (t != oz) tau.push_back(inter[t]);
        // (a) diametre unique : toute autre paire de tau STRICTEMENT
        // plus courte que D2. Le durcissement d'audit (reponse « fold
        // compact » § 3) PROUVE qu'une egalite est IMPOSSIBLE : tous
        // les autres points sont strictement dans la boule OUVERTE de
        // rayon D/2, donc toute autre distance est strictement < D —
        // une egalite observee est une VIOLATION, jamais une
        // degenerescence a exclure.
        bool unique_diam = true;
        for (size_t u = 0; u < tau.size() && unique_diam; ++u)
          for (size_t v = u + 1; v < tau.size(); ++v) {
            if (tau[u] == i && tau[v] == j2) continue;
            const i64 l2 = p3_norm2(
                p3_sub(ix.upos[(size_t)tau[v]], ix.upos[(size_t)tau[u]]));
            if (l2 >= D2) {
              unique_diam = false;  // contredit le theoreme : violation
              break;
            }
          }
        if (!unique_diam) {
          ++violations;
          continue;
        }
        // (b) injection : tau propre jamais vue.
        std::sort(tau.begin(), tau.end());
        if (!taus.insert(tau).second) ++violations;
      }
    }
  if (inj_dup && !taus.empty()) {
    // MUTANT : un tau deja emis est re-presente au verificateur.
    if (!taus.insert(*taus.begin()).second) ++violations;
  }
  const bool floors = nj[0] >= 1 && nj[1] >= 1 && nj[2] >= 1 &&
                      nj[0] >= 2 * ((u64)m - 1);
  std::printf(
      "q2_birth_gate N0..4=%llu/%llu/%llu/%llu/%llu taus=%zu "
      "violations=%llu planchers=%d\n",
      (unsigned long long)nj[0], (unsigned long long)nj[1],
      (unsigned long long)nj[2], (unsigned long long)nj[3],
      (unsigned long long)nj[4], taus.size(),
      (unsigned long long)violations, (int)floors);
  if (inj_dup) {
    if (violations > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return (violations == 0 && floors) ? 0 : 3;
}

// PORTE DE L'ETAGE FLOTTANT (audit « filtre flottant et q3 demi-plans »
// § 1.4) : sous kFloatVerify, CHAQUE signe certifie est recoupe par
// l'exact — zero desaccord exige. Plancher de replis garanti par la
// fixture-cœur cocirculaire MISE A L'ECHELLE ×1999 (pas une puissance
// de 2 — reçu « filtre flottant », piege grave : ×2048 laissait les
// mantisses EXACTES ; a ×1999 les conversions de G et N deviennent
// inexactes, le bruit des sites P = 0 est reel et reste sous la borne,
// donc repli obligatoire) ; planchers de certification des deux signes
// sur les familles. MUTANT float-threshold-too-small (borne 2^20) : les
// P = 0 cocirculaires sortent de la bande retrecie et sont certifies
// avec le signe du bruit contre un exact NUL -> desaccords comptes.
int run_float_gate(bool inj_small) {
  u64 neg = 0, pos = 0, fb = 0, mm = 0, jk = 0, js = 0, jf = 0;
  const u32 flags = kFloatVerify | (inj_small ? kFloatSmallThreshold : 0u);
  std::vector<std::pair<std::vector<P3>, u64>> clouds;
  for (const CloudFamily fam :
       {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    const int n = fam == CloudFamily::kUniform ? 300 : 200;
    clouds.push_back(
        {make_family_cloud(fam, n, cloud_family_default_coord(fam, n), 3),
         11});
  }
  {
    std::vector<P3> fx = {{11, 7, 10},  {20, 10, 10}, {15, 15, 10},
                          {11, 13, 10}, {18, 14, 10}, {12, 14, 10},
                          {15, 10, 16}};
    for (P3& p : fx) {
      p.x *= 1999;
      p.y *= 1999;
      p.z *= 1999;
    }
    clouds.push_back({std::move(fx), 6});
  }
  for (const auto& cl : clouds) {
    const CloudIndex ix = build_cloud_index(cl.first);
    if ((size_t)ix.unique_count() != cl.first.size()) return 3;
    std::vector<BallCandidate> cs;
    BallStreamStats ss;
    collect_candidate_balls(ix, 8, cl.second, &cs, &ss, false, false, flags);
    neg += ss.float_cert_neg;
    pos += ss.float_cert_pos;
    fb += ss.float_fallback;
    mm += ss.float_mismatch;
    jk += ss.jung_cert_kill;
    js += ss.jung_cert_skip;
    jf += ss.jung_fallback;
  }
  // Planchers de l'etage d'intervalles de Jung : les DEUX certifications
  // (temoin et non-temoin) et le repli doivent exister — chaque
  // certification est recoupee par l'exact sous kFloatVerify (desaccords
  // dans mm).
  const bool floors = neg > 0 && pos > 0 && fb > 0 && jk > 0 && js > 0;
  std::printf("float_gate certifies_neg=%llu certifies_pos=%llu replis=%llu "
              "desaccords=%llu jung=%llu/%llu/%llu planchers=%d\n",
              (unsigned long long)neg, (unsigned long long)pos,
              (unsigned long long)fb, (unsigned long long)mm,
              (unsigned long long)jk, (unsigned long long)js,
              (unsigned long long)jf, (int)floors);
  if (inj_small) {
    if (mm > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return (mm == 0 && floors) ? 0 : 3;
}

// PORTE PERMANENTE DU KERNEL AFFINE (audit « filtre certifie et niveaux
// q3 » § 1 + 6). Deux volets :
// (1) IDENTITE EXHAUSTIVE : pour TOUTE ancre (a,b), TOUT seed x et TOUT
//     site z de petits nuages (y compris z ∈ {a, b, x} : u_a = a−b,
//     q_a = 0 et d·N = d·W − G·D2 = 0 par P(b) = 0, donc L(a) = 0),
//     L = G·q_z − 2·u_z·N — les MEMES formules que la production —
//     verifie L == 4·q3_power(f3, z) ET L ≡ 0 (mod 4) : la division
//     P = L/4 du cœur de seed est exacte, pas une approximation.
//     Chaque decision flottante (affine_l_hat / affine_l_bound, les
//     fonctions de production) certifiee est recoupee contre le signe
//     exact. Nuages : uniform/eight_clusters aux coordonnees par defaut
//     (arithmetique double EXACTE : decisions toujours justes),
//     uniform a coord=50000 (G et N inexacts en binaire64 : la borne
//     travaille aux grandeurs reelles) et la fixture-cœur cocirculaire
//     ×1999 (sites a L = 0 exact, bruit reel sous la borne : plancher
//     de replis, et sous mutant le bruit certifie contredit l'exact).
// (2) TEMOIN DE FORTE ANNULATION ± (audit § 6.1), constantes GRAVEES :
//     G = 2^67 − 12345, u = (131071, 0, 0), q = 2^35 + 7,
//     N0 = floor(G·q / (2·u0)) — deux termes ~2^102 s'annulent a
//     L = (G·q) mod (2·u0) = +216577 ; variante N0 + 1 :
//     L − 2·u0 = −45565. binaire64 rend le MEME L^ (bruit ~ −2^49,
//     (double)N0 == (double)(N0+1)) pour les deux : la borne saine
//     (E ~ 2^55) declare INCERTAIN les deux ; la borne retrecie du
//     mutant (E·2^-20 ~ 2^35 < |bruit|) certifie le signe du bruit pour
//     les DEUX variantes, en desaccord avec au moins un exact -> tue.
int run_q3_affine_gate(bool inj_small, bool inj_jung_swap) {
  u64 ids = 0, viol = 0, cneg = 0, cpos = 0, fb = 0, mm = 0;
  std::vector<std::pair<std::vector<P3>, const char*>> clouds;
  clouds.push_back({make_family_cloud(CloudFamily::kUniform, 40,
                                      cloud_family_default_coord(
                                          CloudFamily::kUniform, 40),
                                      3),
                    "uniform40"});
  clouds.push_back({make_family_cloud(CloudFamily::kEightClusters, 32,
                                      cloud_family_default_coord(
                                          CloudFamily::kEightClusters, 32),
                                      3),
                    "eight32"});
  clouds.push_back(
      {make_family_cloud(CloudFamily::kUniform, 28, 50000, 5), "uniform28g"});
  {
    std::vector<P3> fx = {{11, 7, 10},  {20, 10, 10}, {15, 15, 10},
                          {11, 13, 10}, {18, 14, 10}, {12, 14, 10},
                          {15, 10, 16}};
    for (P3& p : fx) {
      p.x *= 1999;
      p.y *= 1999;
      p.z *= 1999;
    }
    clouds.push_back({std::move(fx), "cocirc1999"});
  }
  std::vector<i64> su0, su1, su2, sq;
  std::vector<double> sud0, sud1, sud2, sqd;
  for (const auto& cl : clouds) {
    const CloudIndex ix = build_cloud_index(cl.first);
    const size_t m = (size_t)ix.unique_count();
    if (m != cl.first.size()) return 3;
    su0.resize(m); su1.resize(m); su2.resize(m); sq.resize(m);
    sud0.resize(m); sud1.resize(m); sud2.resize(m); sqd.resize(m);
    for (size_t ua = 0; ua + 1 < m; ++ua)
      for (size_t ub = ua + 1; ub < m; ++ub) {
        const P3& pa = ix.upos[ua];
        const P3& pb = ix.upos[ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        // Sites affines de l'ancre — memes formules que fill_affine_sites.
        i64 qmax = 1, umax = 1;
        const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
        for (size_t i = 0; i < m; ++i) {
          const P3& pz = ix.upos[i];
          const i64 u0 = 2 * pz.x - sx, u1 = 2 * pz.y - sy,
                    u2 = 2 * pz.z - sz;
          const i64 qz = u0 * u0 + u1 * u1 + u2 * u2 - D2;
          su0[i] = u0; su1[i] = u1; su2[i] = u2; sq[i] = qz;
          sud0[i] = (double)u0; sud1[i] = (double)u1; sud2[i] = (double)u2;
          sqd[i] = (double)qz;
          const i64 qa = qz < 0 ? -qz : qz;
          if (qa > qmax) qmax = qa;
          for (const i64 uu : {u0 < 0 ? -u0 : u0, u1 < 0 ? -u1 : u1,
                               u2 < 0 ? -u2 : u2})
            if (uu > umax) umax = uu;
        }
        for (size_t x = 0; x < m; ++x) {
          if (x == ua || x == ub) continue;
          const Q3Form f3 = q3_form(pa, pb, ix.upos[x]);
          const i128 N0 = f3.w[0] - f3.g * (i128)(pb.x - pa.x);
          const i128 N1 = f3.w[1] - f3.g * (i128)(pb.y - pa.y);
          const i128 N2 = f3.w[2] - f3.g * (i128)(pb.z - pa.z);
          const double Gd = (double)f3.g;
          const double Nd0 = (double)N0, Nd1 = (double)N1,
                       Nd2 = (double)N2;
          double bnd = affine_l_bound(Gd, Nd0, Nd1, Nd2, (double)qmax,
                                      (double)umax);
          if (inj_small) bnd *= kFloatMutantShrink;  // MUTANT
          for (size_t iz = 0; iz < m; ++iz) {
            const i128 L = f3.g * (i128)sq[iz] -
                           2 * ((i128)su0[iz] * N0 + (i128)su1[iz] * N1 +
                                (i128)su2[iz] * N2);
            ++ids;
            if (L != 4 * q3_power(f3, ix.upos[iz])) ++viol;
            if (((u64)(u128)L & 3u) != 0) ++viol;
            const double Lh = affine_l_hat(Gd, Nd0, Nd1, Nd2, sud0[iz],
                                           sud1[iz], sud2[iz], sqd[iz]);
            if (Lh < -bnd) {
              ++cneg;
              if (!(L < 0)) ++mm;
            } else if (Lh > bnd) {
              ++cpos;
              if (!(L > 0)) ++mm;
            } else {
              ++fb;
            }
          }
        }
      }
  }
  u64 wit_unc = 0, wit_mm = 0;
  bool wit_fixture_ok = true;
  {
    const i128 gw = ((i128)1 << 67) - 12345;
    const i64 u0 = 131071, qw = (1LL << 35) + 7;
    const i128 n0 = (gw * (i128)qw) / (2 * (i128)u0);
    for (int variant = 0; variant < 2; ++variant) {
      const i128 nv = n0 + variant;
      const i128 L = gw * (i128)qw - 2 * (i128)u0 * nv;
      if (L != (variant ? (i128)-45565 : (i128)216577))
        wit_fixture_ok = false;  // constantes gravees : toute derive casse
      const double gd = (double)gw, nd = (double)nv;
      const double lh = affine_l_hat(gd, nd, 0.0, 0.0, (double)u0, 0.0, 0.0,
                                     (double)qw);
      double bnd = affine_l_bound(gd, nd, 0.0, 0.0, (double)qw, (double)u0);
      if (inj_small) bnd *= kFloatMutantShrink;  // MUTANT
      if (lh < -bnd) {
        if (!(L < 0)) ++wit_mm;
      } else if (lh > bnd) {
        if (!(L > 0)) ++wit_mm;
      } else {
        ++wit_unc;
      }
    }
  }
  // TEMOIN D'INTERVALLES DE JUNG, constantes GRAVEES : lh = -2^60,
  // e = 2^55 => P ∈ [-(2^58+2^53), -(2^58-2^53)] (2P² ∈
  // [~2^116,91 ; ~2^117,09]) ;
  //   (1) J = 2^40, B = 2^20 : J·B² = 2^80  << inf(2P²) -> +1 (kill) ;
  //   (2) J = 2^80, B = 2^25 : J·B² = 2^130 >> sup(2P²) -> -1 (skip) ;
  //   (3) J = 2^59, B = 2^29 : J·B² = 2^117 A CHEVAL   -> 0 (repli).
  // Le mutant jung-swap-bounds teste 2·Pl² au kill : (3) rend +1 au
  // lieu du repli obligatoire -> tue.
  u64 jw_bad = 0, jw_mut = 0;
  {
    const double lh = -0x1p60, e = 0x1p55;
    const struct { double j; i64 b; int want; } jw[3] = {
        {0x1p40, (i64)1 << 20, 1},
        {0x1p80, (i64)1 << 25, -1},
        {0x1p59, (i64)1 << 29, 0},
    };
    for (const auto& w : jw) {
      const double jlo = w.j * (1.0 - kJungGuard);
      const double jhi = w.j * (1.0 + kJungGuard);
      if (jung_interval_sign(lh, e, jlo, jhi, w.b, false) != w.want)
        ++jw_bad;
      if (inj_jung_swap &&
          jung_interval_sign(lh, e, jlo, jhi, w.b, true) != w.want)
        ++jw_mut;
    }
  }
  const bool floors = ids >= 1000000 && cneg >= 100 && cpos >= 100 && fb >= 1;
  std::printf(
      "q3_affine_gate identites=%llu violations=%llu certifies_neg=%llu "
      "certifies_pos=%llu replis=%llu desaccords=%llu temoin_incertains=%llu "
      "temoin_desaccords=%llu jung_temoin_faux=%llu planchers=%d\n",
      (unsigned long long)ids, (unsigned long long)viol,
      (unsigned long long)cneg, (unsigned long long)cpos,
      (unsigned long long)fb, (unsigned long long)mm,
      (unsigned long long)wit_unc, (unsigned long long)wit_mm,
      (unsigned long long)jw_bad, (int)floors);
  if (!wit_fixture_ok) return 3;
  if (inj_jung_swap) {
    if (jw_mut > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (inj_small) {
    if (mm + wit_mm > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return (viol == 0 && mm == 0 && wit_unc == 2 && jw_bad == 0 && floors)
             ? 0
             : 3;
}



// SIGNATURE CANONIQUE DE L'OBJET (audit bloquant 9223888 § 2.2) : les
// totaux ne prouvent pas l'egalite de deux forets — les campagnes hors
// juge (echelle de fils) comparent des digests SHA-256 d'une
// serialisation VERSIONNEE explicite (petit-boutiste, largeurs fixes) du
// flux canonique : boules post-RLE, puis par K facet_keys /
// final_canon_fid / deltas, puis chainage ordonne des K.
struct CanonicalDigest {
  Sha256 h;
  void tag(const char* t) { h.update(t, std::strlen(t)); }
  void u8v(u8 v) { h.update(&v, 1); }
  void u32v(u32 v) {
    u8 b[4];
    for (int i = 0; i < 4; ++i) b[i] = (u8)(v >> (8 * i));
    h.update(b, 4);
  }
  void u64v(u64 v) {
    u8 b[8];
    for (int i = 0; i < 8; ++i) b[i] = (u8)(v >> (8 * i));
    h.update(b, 8);
  }
  void i128v(i128 v) {
    const u128 u = (u128)v;
    u64v((u64)u);
    u64v((u64)(u >> 64));
  }
  void facet(const FacetKey& f) {
    u8v(f.k);
    for (int i = 0; i < 10; ++i) u32v(f.p[(size_t)i]);
  }
  void level(const Q4Level& l) {
    for (int i = 0; i < 3; ++i) u64v(l.num[i]);
    i128v(l.den);
  }
};

// Rend les digests dans l'ordre : [0]=balls, [1..kmax]=forest_K,
// [kmax+1]=all. La porte de SENSIBILITE (--digest-gate) et l'impression
// consomment le MEME calcul.
void compute_canonical_digests(const std::vector<BallCandidate>& cands,
                               const ForestResult* res, u64 kmax_eff,
                               std::vector<std::string>* out) {
  out->clear();
  {
    CanonicalDigest d;
    d.tag("mhgp4-digest-v1:balls");
    d.u64v((u64)cands.size());
    for (const BallCandidate& c : cands) {
      d.i128v(c.key.a);
      for (int i = 0; i < 3; ++i) d.i128v(c.key.b[i]);
      d.i128v(c.key.c);
      d.level(c.level);
      d.u8v(c.arity);
    }
    out->push_back(d.h.hex());
  }
  CanonicalDigest all;
  all.tag("mhgp4-digest-v1:all");
  for (u64 K = 1; K <= kmax_eff; ++K) {
    CanonicalDigest d;
    d.tag("mhgp4-digest-v1:forest");
    d.u32v((u32)K);
    const ForestResult& r = res[K];
    d.u64v((u64)r.facet_keys.size());
    for (const FacetKey& f : r.facet_keys) d.facet(f);
    d.u64v((u64)r.final_canon_fid.size());
    for (const u32 v : r.final_canon_fid) d.u32v(v);
    d.u64v((u64)r.deltas.size());
    for (const ComponentDelta& cd : r.deltas) {
      d.u64v(cd.batch);
      d.level(cd.level);
      d.facet(cd.output);
      d.u64v((u64)cd.parents.size());
      for (const FacetKey& f : cd.parents) d.facet(f);
      d.u64v((u64)cd.born.size());
      for (const FacetKey& f : cd.born) d.facet(f);
    }
    const std::string hx = d.h.hex();
    all.h.update(hx.data(), hx.size());
    out->push_back(hx);
  }
  out->push_back(all.h.hex());
}

void print_canonical_digests(const std::vector<BallCandidate>& cands,
                             const ForestResult* res, u64 kmax_eff) {
  std::vector<std::string> dg;
  compute_canonical_digests(cands, res, kmax_eff, &dg);
  std::printf("digest_balls=%s\n", dg[0].c_str());
  for (u64 K = 1; K <= kmax_eff; ++K)
    std::printf("digest_forest_K%llu=%s\n", (unsigned long long)K,
                dg[(size_t)K].c_str());
  std::printf("digest_all=%s\n", dg[(size_t)kmax_eff + 1].c_str());
}

// PORTE DE SENSIBILITE DU SERIALISEUR (audit 66886c0 § 3) : modifier une
// BallKey, un final_canon_fid et une facette de delta doit changer
// respectivement digest_balls, le digest du BON K (et digest_all), et
// rien d'autre. Protege contre un serialiseur qui ignore un champ.
int run_digest_gate() {
  const std::vector<P3> pts = make_family_cloud(
      CloudFamily::kEightClusters, 120,
      cloud_family_default_coord(CloudFamily::kEightClusters, 120), 3);
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) return 3;
  std::vector<BallCandidate> cs;
  BallStreamStats ss;
  collect_candidate_balls(ix, 8, 11, &cs, &ss);
  std::stable_sort(cs.begin(), cs.end(), ball_candidate_less);
  cs.erase(std::unique(cs.begin(), cs.end(),
                       [](const BallCandidate& x, const BallCandidate& y) {
                         return x.key == y.key;
                       }),
           cs.end());
  std::vector<Survivor> sv;
  BallStreamStats ds;
  prefilter_balls(ix, cs, 11, false, false, &sv, &ds, 1);
  std::vector<BallData> balls;
  if (census_balls(ix, cs, sv, 11, 12, false, false, &balls, &ds, 1, false) !=
      0)
    return 3;
  std::vector<PointId> pid_of((size_t)ix.unique_count());
  for (size_t u = 0; u < pid_of.size(); ++u)
    pid_of[u] = ix.point_id((i32)u);
  u64 e1[11] = {};
  ForestResult rr[11];
  std::vector<std::vector<ForestEvent>> evk;
  if (forests_from_balls(balls, ix.upos, pid_of, 10, e1, rr, &evk, 1, true) !=
      0)
    return 3;
  if (cs.empty() || rr[10].final_canon_fid.empty() || rr[10].deltas.empty())
    return 3;
  std::vector<std::string> base;
  compute_canonical_digests(cs, rr, 10, &base);
  u64 bad = 0;
  const auto diff_count = [&](const std::vector<std::string>& v,
                              std::vector<size_t>* idx) {
    idx->clear();
    for (size_t i = 0; i < base.size(); ++i)
      if (v[i] != base[i]) idx->push_back(i);
  };
  std::vector<size_t> idx;
  {
    // (a) une BallKey modifiee -> digest_balls change, forets inchangees
    // (digest_all ne chaine que les forets : inchange).
    std::vector<BallCandidate> cs2 = cs;
    cs2[0].key.c += 1;
    std::vector<std::string> d2;
    compute_canonical_digests(cs2, rr, 10, &d2);
    diff_count(d2, &idx);
    if (!(idx.size() == 1 && idx[0] == 0)) ++bad;
  }
  {
    // (b) un final_canon_fid de K=10 -> digest_forest_K10 ET digest_all.
    ForestResult rr2[11];
    for (int k = 0; k <= 10; ++k) rr2[k] = rr[k];
    rr2[10].final_canon_fid[0] ^= 1u;
    std::vector<std::string> d2;
    compute_canonical_digests(cs, rr2, 10, &d2);
    diff_count(d2, &idx);
    if (!(idx.size() == 2 && idx[0] == 10 && idx[1] == 11)) ++bad;
  }
  {
    // (c) une facette de delta de K=3 -> digest_forest_K3 ET digest_all.
    ForestResult rr2[11];
    for (int k = 0; k <= 10; ++k) rr2[k] = rr[k];
    if (rr2[3].deltas.empty()) return 3;
    rr2[3].deltas[0].output.p[0] += 1;
    std::vector<std::string> d2;
    compute_canonical_digests(cs, rr2, 10, &d2);
    diff_count(d2, &idx);
    if (!(idx.size() == 2 && idx[0] == 3 && idx[1] == 11)) ++bad;
  }
  std::printf("digest_gate mutations_invisibles=%llu\n",
              (unsigned long long)bad);
  return bad == 0 ? 0 : 3;
}

// PORTE DES WORKERS MESURES (audits 66886c0 § 2, 7d921ff, c9c3a48) :
// a threads=4 sur un nuage a taches abondantes, chaque etage doit avoir
// CREE 4 ouvriers — generation PAR LANE (q2/q3/q4), aval publie par la
// VALEUR RETOURNEE de parallel_ranges, fold min(4, K_max) = 4 — et
// l'affinite effective du processus doit etre lisible et coherente.
// Trois mutants, tous a CLI et digests inchanges, tues par la mesure :
// parallel-hardcodes-one-worker (run_rects), parallel-ranges-one-worker
// (la primitive aval serialise tout), q3-one-worker (seule la lane
// dominante serialise — gen_workers_max reste aveugle, la porte lit la
// lane).
int run_workers_gate(bool inj_one, bool inj_ranges_one, bool inj_q3_one,
                     bool inj_wspd_one) {
  const std::vector<P3> pts = make_family_cloud(
      CloudFamily::kUniform, 300, cloud_family_default_coord(
                                      CloudFamily::kUniform, 300), 3);
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) return 3;
  std::vector<BallCandidate> cs;
  BallStreamStats ss;
  g_inj_parallel_ranges_one = inj_ranges_one;  // MUTANT primitive aval
  collect_candidate_balls(ix, 8, 11, &cs, &ss, false, false, 0, 4, false,
                          inj_one, inj_q3_one, inj_wspd_one);
  std::stable_sort(cs.begin(), cs.end(), ball_candidate_less);
  cs.erase(std::unique(cs.begin(), cs.end(),
                       [](const BallCandidate& x, const BallCandidate& y) {
                         return x.key == y.key;
                       }),
           cs.end());
  std::vector<Survivor> sv;
  prefilter_balls(ix, cs, 11, false, false, &sv, &ss, 4);
  std::vector<BallData> balls;
  if (census_balls(ix, cs, sv, 11, 12, false, false, &balls, &ss, 4, false) !=
      0)
    return 3;
  std::vector<PointId> pid_of((size_t)ix.unique_count());
  for (size_t u = 0; u < pid_of.size(); ++u)
    pid_of[u] = ix.point_id((i32)u);
  u64 e1[11] = {};
  ForestResult rr[11];
  std::vector<std::vector<ForestEvent>> evk;
  if (forests_from_balls(balls, ix.upos, pid_of, 10, e1, rr, &evk, 4, true,
                         &ss) != 0)
    return 3;
  std::string aff_mask;
  const int aff = effective_affinity(&aff_mask);
  std::printf("workers_gate gen_q2=%llu gen_q3=%llu gen_q4=%llu "
              "prefiltre=%llu census=%llu expansion=%llu fold=%llu "
              "wspd_q2=%llu wspd_q3=%llu wspd_q4=%llu "
              "affinite=%d masque=%s\n",
              (unsigned long long)ss.gen_workers[0],
              (unsigned long long)ss.gen_workers[1],
              (unsigned long long)ss.gen_workers[2],
              (unsigned long long)ss.prefilter_workers,
              (unsigned long long)ss.census_workers,
              (unsigned long long)ss.expansion_workers,
              (unsigned long long)ss.fold_workers_max,
              (unsigned long long)ss.wspd_workers[0],
              (unsigned long long)ss.wspd_workers[1],
              (unsigned long long)ss.wspd_workers[2], aff,
              aff_mask.empty() ? "?" : aff_mask.c_str());
  g_inj_parallel_ranges_one = false;
  if (inj_one) {
    if (ss.gen_workers_max == 1) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (inj_ranges_one) {
    if (ss.prefilter_workers == 1 && ss.census_workers == 1 &&
        ss.expansion_workers == 1 && ss.fold_workers_max == 1 &&
        ss.gen_workers_max == 4) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (inj_q3_one) {
    if (ss.gen_workers[1] == 1 && ss.gen_workers[0] == 4 &&
        ss.gen_workers_max == 4) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  // MUTANT wspd-one-worker : la DESCENTE serialise (72 % de t_gen a
  // n=8000 avant sa parallelisation), CLI et digests inchanges — seule
  // la mesure au point de creation le trahit, et les lanes restent a 4.
  if (inj_wspd_one) {
    if (ss.wspd_workers[0] == 1 && ss.wspd_workers[1] == 1 &&
        ss.wspd_workers[2] == 1 && ss.gen_workers_max == 4) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return (ss.gen_workers[0] == 4 && ss.gen_workers[1] == 4 &&
          ss.gen_workers[2] == 4 && ss.prefilter_workers == 4 &&
          ss.census_workers == 4 && ss.expansion_workers == 4 &&
          ss.fold_workers_max == 4 && ss.wspd_workers[0] == 4 &&
          ss.wspd_workers[1] == 4 && ss.wspd_workers[2] == 4 && aff >= 1)
             ? 0
             : 3;
}

// PORTE DE LA GARDE DE CAPACITE DU FOLD (contre-audit 5d274a1 § 7) :
// les bases FICTIVES cap_base_* approchent les limites des index locaux
// sans allocation geante. Quatre cas AU-DESSUS (evenements > u32,
// nfid-majorant > i32 au bord et par enroulement u32, lots atteignant
// la sentinelle UINT32_MAX) doivent etre REFUSES avant calcul ; trois
// cas JUSTE SOUS la limite doivent etre acceptes avec un resultat
// IDENTIQUE a la base (la garde est une pure verification). Chaque
// mutant degrade UNE comparaison : u32-event-wrap (le compte
// d'evenements passe par u32 : jamais de refus), i32-fid-wrap (le
// majorant de fid s'enroule : 2^32 + r accepte a tort),
// epoch-sentinel-collision (<= au lieu de < : le lot sentinelle passe).
int run_fold_capacity_gate(int mutant) {
  const std::vector<P3> pts = make_family_cloud(
      CloudFamily::kEightClusters, 120,
      cloud_family_default_coord(CloudFamily::kEightClusters, 120), 3);
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) return 3;
  std::vector<BallCandidate> cs;
  BallStreamStats ss;
  collect_candidate_balls(ix, 8, 11, &cs, &ss);
  std::stable_sort(cs.begin(), cs.end(), ball_candidate_less);
  cs.erase(std::unique(cs.begin(), cs.end(),
                       [](const BallCandidate& x, const BallCandidate& y) {
                         return x.key == y.key;
                       }),
           cs.end());
  std::vector<Survivor> sv;
  BallStreamStats ds;
  prefilter_balls(ix, cs, 11, false, false, &sv, &ds, 1);
  std::vector<BallData> balls;
  if (census_balls(ix, cs, sv, 11, 12, false, false, &balls, &ds, 1, false) !=
      0)
    return 3;
  std::vector<PointId> pid_of((size_t)ix.unique_count());
  for (size_t u = 0; u < pid_of.size(); ++u)
    pid_of[u] = ix.point_id((i32)u);
  u64 e1[11] = {};
  ForestResult rr[11];
  std::vector<std::vector<ForestEvent>> evk;
  if (forests_from_balls(balls, ix.upos, pid_of, 10, e1, rr, &evk, 1, true) !=
      0)
    return 3;
  const std::vector<ForestEvent>& ev = evk[10];
  u64 total = 0;
  for (const ForestEvent& e : ev) total += (u64)e.q + e.d;
  const u64 nev = ev.size();
  if (nev == 0 || total == 0) return 3;
  const ForestResult base = build_forest(ev);
  const auto run = [&](u64 be, u64 br, u64 bb) {
    return build_forest(ev, false, false, nullptr, false, false, false, true,
                        be, br, bb, mutant);
  };
  struct Case {
    u64 be, br, bb;
  };
  const Case over[4] = {{(u64)UINT32_MAX + 1 - nev, 0, 0},
                        {0, (u64)INT32_MAX + 1 - total, 0},
                        {0, (u64)1 << 32, 0},
                        {0, 0, (u64)UINT32_MAX - nev}};
  u64 refused = 0, accepted_wrong = 0;
  for (const Case& c : over) {
    const ForestResult t = run(c.be, c.br, c.bb);
    if (!t.refusal.empty())
      ++refused;
    else
      ++accepted_wrong;
  }
  const Case under[3] = {{(u64)UINT32_MAX - nev, 0, 0},
                         {0, (u64)INT32_MAX - total, 0},
                         {0, 0, (u64)UINT32_MAX - 1 - nev}};
  u64 under_bad = 0;
  for (const Case& c : under) {
    const ForestResult t = run(c.be, c.br, c.bb);
    if (!t.refusal.empty() || t.facets != base.facets ||
        t.fusions != base.fusions || t.deltas.size() != base.deltas.size() ||
        !(t.final_partition == base.final_partition))
      ++under_bad;
  }
  std::printf(
      "fold_capacity_gate evenements=%llu recs=%llu refus=%llu/4 "
      "acceptes_a_tort=%llu sous_limite_ko=%llu\n",
      (unsigned long long)nev, (unsigned long long)total,
      (unsigned long long)refused, (unsigned long long)accepted_wrong,
      (unsigned long long)under_bad);
  if (mutant != 0) {
    if (accepted_wrong > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return (refused == 4 && under_bad == 0) ? 0 : 3;
}

// PORTE DE LA GARDE D'ARRONDI (contre-audit 04c71a2 § 4) : sous un mode
// d'arrondi != FE_TONEAREST la preuve de la borne ne tient plus — le
// filtre doit se desactiver SEUL (zero certification, tout en repli
// exact, sortie inchangee). Sous FE_TONEAREST il doit certifier (le
// meme nuage donne des certifications > 0 : la garde n'est pas un
// interrupteur toujours-off). MUTANT float-ignore-rounding : la garde
// est ignoree — des certifications apparaissent sous FE_UPWARD, la
// porte le voit et tue.
int run_float_rounding_gate(bool inj_ignore) {
  const std::vector<P3> pts = make_family_cloud(
      CloudFamily::kUniform, 300,
      cloud_family_default_coord(CloudFamily::kUniform, 300), 3);
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) return 3;
  const u32 flags = inj_ignore ? kFloatIgnoreRounding : 0u;
  std::vector<BallCandidate> cs_up, cs_near;
  BallStreamStats ss_up, ss_near;
  if (std::fesetround(FE_UPWARD) != 0) return 3;
  collect_candidate_balls(ix, 8, 11, &cs_up, &ss_up, false, false, flags);
  if (std::fesetround(FE_TONEAREST) != 0) return 3;
  collect_candidate_balls(ix, 8, 11, &cs_near, &ss_near, false, false, flags);
  const u64 cert_up = ss_up.float_cert_neg + ss_up.float_cert_pos;
  const u64 cert_near = ss_near.float_cert_neg + ss_near.float_cert_pos;
  const bool same_out =
      cs_up.size() == cs_near.size() &&
      std::equal(cs_up.begin(), cs_up.end(), cs_near.begin(),
                 [](const BallCandidate& x, const BallCandidate& y) {
                   return x.key == y.key && x.arity == y.arity;
                 });
  std::printf("float_rounding_gate certifies_upward=%llu "
              "certifies_nearest=%llu replis_upward=%llu sortie_identique=%d\n",
              (unsigned long long)cert_up, (unsigned long long)cert_near,
              (unsigned long long)ss_up.float_fallback, (int)same_out);
  if (inj_ignore) {
    if (cert_up > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return (cert_up == 0 && ss_up.float_fallback > 0 && cert_near > 0 &&
          same_out)
             ? 0
             : 3;
}

// PORTE A DEUX BACKENDS DU FOLD COMPACT (reponse d'audit « fold
// compact » § 2) : le backend FIGE build_forest_legacy CONTRE le fold
// compact (canonique par min-fid, tables a epoque), sur le pipeline
// reel de deux familles — egalite des compteurs (facettes, fusions,
// lots), des niveaux de lot, des nœuds, des DELTAS paire a paire
// (operator==) et de la partition (map legacy contre map compacte ET
// contre la vue dense facet_keys/final_canon_fid) ; les invariants
// structurels denses sont verifies par build_forest lui-meme
// (partition_violations == 0). MUTANT canonical-is-uf-root : le
// canonique suit la racine union-find au lieu du minimum — les unions
// generiques placent la racine ailleurs que le minimum, partition et
// deltas divergent.
// PORTE DU PREFILTRE PAR PUISSANCE EQUATORIALE (reponse d'audit
// `5b89bc6`) : le prefiltre court-circuite `q4_form` sur les paires
// (seed, y) que la seule face `abx` suffit a rejeter. Comme chaque face
// est une condition NECESSAIRE du bien-centrage — caracterisation
// prouvee et recue par l'oracle q4 — l'objet ne doit pas bouger d'un
// bit. La porte compare les DEUX chemins dans le MEME processus, sur
// les memes nuages, apres tri et RLE, et exige : flux identique, aucun
// FAUX REJET sur le flux reel (le cablage face/sommet, que l'oracle ne
// teste pas), et un plancher de rejets (sans quoi elle serait verte par
// vacuite). MUTANT `q4-eq-wrong-face` : la puissance est evaluee sur la
// face `aby` au lieu de `abx` tout en gardant le sommet oppose y — un
// prefiltre qui n'est plus une condition necessaire de ce qu'il filtre.
int run_q4_eq_gate(bool inj_wrong_face) {
  u64 bad = 0, tot_reject = 0, tot_false = 0;
  for (const CloudFamily fam :
       {CloudFamily::kUniform, CloudFamily::kEightClusters,
        CloudFamily::kTerrain}) {
    const int n = fam == CloudFamily::kUniform ? 300 : 200;
    const std::vector<P3> pts =
        make_family_cloud(fam, n, cloud_family_default_coord(fam, n), 3);
    const CloudIndex ix = build_cloud_index(pts);
    if ((size_t)ix.unique_count() != pts.size()) return 3;
    std::vector<BallCandidate> con, coff;
    BallStreamStats son, soff;
    // Le prefiltre est evalue SANS court-circuit dans ce run : on veut
    // que Cramer tranche chaque paire pour pouvoir compter les FAUX
    // REJETS du prefiltre sur le flux reel (avec court-circuit, une
    // paire faussement rejetee ne serait jamais confrontee a Cramer).
    collect_candidate_balls(ix, 8, 11, &con, &son, false, false, 0, 1, false,
                            false, false, false, /*q4_eq_prefilter=*/false,
                            /*mutant_q4_eq_swap=*/inj_wrong_face);
    // Chemin de production : court-circuit actif, sans mutant. Son flux
    // doit etre celui du run precedent, au bit pres.
    collect_candidate_balls(ix, 8, 11, &coff, &soff, false, false, 0, 1, false,
                            false, false, true);
    const auto compact = [](std::vector<BallCandidate>* c) {
      std::stable_sort(c->begin(), c->end(), ball_candidate_less);
      c->erase(std::unique(c->begin(), c->end(),
                           [](const BallCandidate& x, const BallCandidate& y) {
                             return x.key == y.key;
                           }),
               c->end());
    };
    compact(&con);
    compact(&coff);
    bool same = con.size() == coff.size() &&
                son.candidates[2] == soff.candidates[2] &&
                son.q4_reach_depth == soff.q4_reach_depth;
    for (size_t i = 0; same && i < con.size(); ++i)
      same = con[i].key == coff[i].key && con[i].arity == coff[i].arity &&
             compare_exact_level(con[i].level, coff[i].level) == 0;
    if (!same) {
      std::fprintf(stderr, "PREFILTRE q4 : flux different (%s)\n",
                   cloud_family_name(fam));
      ++bad;
    }
    if (son.q4_eq_false_reject != 0) {
      std::fprintf(stderr, "PREFILTRE q4 : %llu faux rejets (%s)\n",
                   (unsigned long long)son.q4_eq_false_reject,
                   cloud_family_name(fam));
      ++bad;
    }
    tot_reject += son.q4_eq_reject;
    tot_false += son.q4_eq_false_reject;
  }
  std::printf("q4_eq_gate violations=%llu rejets=%llu faux_rejets=%llu\n",
              (unsigned long long)bad, (unsigned long long)tot_reject,
              (unsigned long long)tot_false);
  if (inj_wrong_face) {
    if (bad > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (bad) return 3;
  if (tot_reject < 100000) {
    std::fprintf(stderr, "PLANCHER : rejets=%llu\n",
                 (unsigned long long)tot_reject);
    return 3;
  }
  return 0;
}

// BANC D'ALTERNANCE DES DEUX INTERNEMENTS (mesure, pas une porte).
// Motif : sur ce conteneur, `t_fold` du MEME binaire varie de ±40 %
// d'un processus a l'autre (allocations a l'echelle du Go), si bien
// qu'une comparaison entre processus ne departage rien. Ici les deux
// modes tournent dans le MEME processus, sur les MEMES evenements,
// alternes R fois, et chaque temps est publie — jamais une moyenne
// seule. La porte a trois backends garantit par ailleurs que les deux
// modes rendent le meme objet : le banc ne mesure donc qu'un cout.
// ---- RAPPORTEUR DU BANC APPARIE (contre-audit 21e617d § 2) -----------
// Les deux modes sont mesures DANS LE MEME processus, donc le plan est
// APPARIE : la statistique pertinente est la mediane des rapports
// r_i = t_streaming_i / t_tri_i, jamais le rapport de deux medianes
// marginales. Sur les cinq paires publiees le 18 aout, les deux
// divergent : rapport de medianes 0,843 (« x1,19 ») contre mediane
// appariee 0,926 (« x1,08 »). Le rapport de medianes reste publie, en
// second, parce qu'il permet de relire les anciens recus — jamais comme
// estimateur principal.
struct BenchPair {
  double tri = 0, stream = 0;
  bool tri_first = false;  // ordre d'execution REEL de la paire
};
struct PairedReport {
  double median_ratio = 0;      // mediane des r_i (ESTIMATEUR PRINCIPAL)
  double median_log_diff = 0;   // mediane de ln(stream) - ln(tri)
  double ratio_of_medians = 0;  // second, pour relire les anciens recus
  u64 wins_stream = 0, wins_tri = 0;
  u64 pairs_tri_first = 0, pairs_stream_first = 0;
};

double median_of(std::vector<double> v) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  const size_t m = v.size() / 2;
  return (v.size() % 2) ? v[m] : 0.5 * (v[m - 1] + v[m]);
}

PairedReport paired_report(const std::vector<BenchPair>& pairs) {
  PairedReport rep;
  std::vector<double> ratios, logs, tri, stream;
  for (const BenchPair& p : pairs) {
    if (p.tri <= 0 || p.stream <= 0) continue;
    ratios.push_back(p.stream / p.tri);
    logs.push_back(std::log(p.stream) - std::log(p.tri));
    tri.push_back(p.tri);
    stream.push_back(p.stream);
    if (p.stream < p.tri) ++rep.wins_stream; else ++rep.wins_tri;
    if (p.tri_first) ++rep.pairs_tri_first; else ++rep.pairs_stream_first;
  }
  rep.median_ratio = median_of(ratios);
  rep.median_log_diff = median_of(logs);
  const double mt = median_of(tri);
  rep.ratio_of_medians = mt > 0 ? median_of(stream) / mt : 0.0;
  return rep;
}

// Le contrebalancement exige un nombre PAIR de paires (bloc ABBA) et au
// moins quatre : en deca, aucune conclusion ne tient (le test des signes
// unilateral sur cinq paires donne deja P = 0,1875 pour quatre
// victoires). Refus AVANT calcul, code 2.
bool bench_schedule_ok(int repeat) { return repeat >= 4 && (repeat % 2) == 0; }

// Signature du resultat COMPLET, hors chronometrage : le banc doit
// prouver que l'objet massif qu'il mesure reste identique au fil de la
// session, pas seulement sur les fixtures de la porte.
u64 forest_signature(const ForestResult& r) {
  u64 h = 1469598103934665603ull;
  const auto mix = [&](u64 v) {
    h ^= v;
    h *= 1099511628211ull;
  };
  mix(r.facets); mix(r.fusions); mix(r.batches);
  mix(r.new_attachments); mix(r.attach_violations); mix(r.birth_violations);
  mix(r.deltas.size()); mix(r.nodes.size()); mix(r.partition_violations);
  for (const u32 c : r.final_canon_fid) mix(c);
  for (const FacetKey& k : r.facet_keys) {
    mix(k.k);
    for (u8 i = 0; i < k.k; ++i) mix(k.p[i]);
  }
  return h;
}

// PORTE SYNTHETIQUE DU RAPPORTEUR (contre-audit § 3) : des temps
// FICTIFS suffisent a verifier que le banc publie bien la statistique
// appariee et refuse un plan non contrebalance — aucun calcul reel,
// aucune fixture geometrique.
int run_bench_report_gate() {
  u64 bad = 0;
  // Jeu construit pour que les deux statistiques DIVERGENT : le rapport
  // de medianes vaut 1,0 alors que le streaming gagne trois paires sur
  // quatre (mediane appariee 0,75).
  const std::vector<BenchPair> pairs = {
      {100.0, 75.0, true}, {200.0, 150.0, false},
      {50.0, 200.0, true}, {400.0, 300.0, false}};
  const PairedReport rep = paired_report(pairs);
  // Rapports {0,75 ; 0,75 ; 4,0 ; 0,75} -> tries {0,75 ; 0,75 ; 0,75 ;
  // 4,0}, mediane paire = 0,75. Rapport de medianes : 175/150 = 1,1667.
  // (Premiere ecriture de cette fixture : 0,875 — la porte a refuse
  // AVANT tout banc, exactement son travail.)
  const double want_ratio = 0.75;
  if (std::fabs(rep.median_ratio - want_ratio) > 1e-12) {
    std::fprintf(stderr, "RAPPORTEUR : mediane appariee %.6f (attendu %.6f)\n",
                 rep.median_ratio, want_ratio);
    ++bad;
  }
  if (std::fabs(rep.ratio_of_medians - rep.median_ratio) < 1e-9) {
    std::fprintf(stderr,
                 "PORTE INEFFICACE : le jeu ne separe pas les deux "
                 "statistiques\n");
    ++bad;
  }
  if (rep.wins_stream != 3 || rep.wins_tri != 1) {
    std::fprintf(stderr, "RAPPORTEUR : victoires %llu/%llu\n",
                 (unsigned long long)rep.wins_stream,
                 (unsigned long long)rep.wins_tri);
    ++bad;
  }
  if (rep.pairs_tri_first != rep.pairs_stream_first) {
    std::fprintf(stderr, "RAPPORTEUR : ordre non equilibre %llu/%llu\n",
                 (unsigned long long)rep.pairs_tri_first,
                 (unsigned long long)rep.pairs_stream_first);
    ++bad;
  }
  for (const int r : {1, 2, 3, 5, 7}) {
    if (bench_schedule_ok(r)) {
      std::fprintf(stderr, "PLAN : repeat=%d accepte a tort\n", r);
      ++bad;
    }
  }
  for (const int r : {4, 6, 10}) {
    if (!bench_schedule_ok(r)) {
      std::fprintf(stderr, "PLAN : repeat=%d refuse a tort\n", r);
      ++bad;
    }
  }
  std::printf("bench_report_gate violations=%llu mediane_appariee=%.4f "
              "rapport_de_medianes=%.4f victoires=%llu/%llu ordre=%llu/%llu\n",
              (unsigned long long)bad, rep.median_ratio, rep.ratio_of_medians,
              (unsigned long long)rep.wins_stream,
              (unsigned long long)rep.wins_tri,
              (unsigned long long)rep.pairs_tri_first,
              (unsigned long long)rep.pairs_stream_first);
  return bad == 0 ? 0 : 3;
}

// Construction commune aux deux bancs : le flux WSPD reel jusqu'aux
// evenements par K. Aucune mesure ici — seulement de quoi mesurer.
int build_events_for_bench(const Args& a,
                           std::vector<std::vector<ForestEvent>>* evk,
                           u64* kmax_out) {
  const std::vector<P3> pts = make_family_cloud(
      a.family, a.n,
      a.coord ? a.coord : cloud_family_default_coord(a.family, a.n), a.seed);
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) return 3;
  std::vector<BallCandidate> cs;
  BallStreamStats ss;
  collect_candidate_balls(ix, a.s, a.smax, &cs, &ss, false, false, 0,
                          a.threads);
  std::stable_sort(cs.begin(), cs.end(), ball_candidate_less);
  cs.erase(std::unique(cs.begin(), cs.end(),
                       [](const BallCandidate& x, const BallCandidate& y) {
                         return x.key == y.key;
                       }),
           cs.end());
  std::vector<Survivor> sv;
  BallStreamStats ds;
  prefilter_balls(ix, cs, a.smax, false, false, &sv, &ds, a.threads);
  std::vector<BallData> balls;
  if (census_balls(ix, cs, sv, a.smax, a.shell_cap, false, false, &balls, &ds,
                   a.threads, false) != 0)
    return 3;
  std::vector<PointId> pid_of((size_t)ix.unique_count());
  for (size_t u = 0; u < pid_of.size(); ++u) pid_of[u] = ix.point_id((i32)u);
  u64 e1[11] = {};
  ForestResult rr[11];
  *kmax_out = std::min<u64>(10, a.smax ? a.smax - 1 : 10);
  return forests_from_balls(balls, ix.upos, pid_of, *kmax_out, e1, rr, evk, 1,
                            false);
}

// BANC D'ORDONNANCEMENT DES FOLDS (reponse d'audit `95061c1` § 3) : les
// TROIS modes sont confrontes DANS LE MEME processus, sur les MEMES
// evenements — `contiguous_reference` (le defaut historique),
// `LPT_unbounded` (borne de latence, JAMAIS un defaut : il fait tourner
// ensemble les K les plus lourds) et `memory_budgeted_LPT`. Chacun
// publie latence, pic RSS et signature ; la signature doit etre
// IDENTIQUE aux trois, sans quoi le banc ne mesure pas le meme objet.
int run_schedule_bench(const Args& a) {
  std::vector<std::vector<ForestEvent>> evk;
  u64 kmax = 0;
  if (build_events_for_bench(a, &evk, &kmax) != 0) return 3;
  const size_t nk = (size_t)kmax;
  if (nk == 0) return 3;
  std::vector<u64> W(nk, 0), M(nk, 0);
  u64 wtot = 0, mtot = 0;
  for (size_t i = 0; i < nk; ++i) {
    for (const ForestEvent& e : evk[i + 1]) W[i] += (u64)e.q + e.d;
    M[i] = fold_bytes_upper(evk[i + 1]);
    wtot += W[i];
    mtot += M[i];
  }
  for (size_t i = 0; i < nk; ++i)
    std::printf("fold_cout K=%zu evenements=%zu incidences=%llu "
                "part_pct=%.1f octets_majorant=%llu\n",
                i + 1, evk[i + 1].size(), (unsigned long long)W[i],
                wtot ? 100.0 * (double)W[i] / (double)wtot : 0.0,
                (unsigned long long)M[i]);
  std::printf("fold_cout total_incidences=%llu total_octets_majorant=%llu "
              "budget=%llu\n",
              (unsigned long long)wtot, (unsigned long long)mtot,
              (unsigned long long)a.fold_budget);

  std::vector<ForestResult> res(nk + 1);
  const auto one_fold = [&](size_t idx) {
    res[idx + 1] = build_forest(evk[idx + 1], false, false, nullptr, false,
                                false, false, false);
  };
  const auto signature_all = [&]() {
    u64 h = 0;
    for (size_t i = 1; i <= nk; ++i) h = h * 1099511628211ull ^
                                         forest_signature(res[i]);
    return h;
  };
  u64 sig_ref = 0;
  bool same = true;
  // 1. Reference contigue (le defaut actuel).
  reset_peak_rss();
  const auto t0 = std::chrono::steady_clock::now();
  const size_t w_ref = parallel_ranges(nk, a.threads,
                                       [&](size_t bg, size_t en, size_t) {
    for (size_t i = bg; i < en; ++i) one_fold(i);
  });
  const double ms_ref = std::chrono::duration<double, std::milli>(
                            std::chrono::steady_clock::now() - t0).count();
  const u64 rss_ref = peak_rss_kib();
  sig_ref = signature_all();
  std::printf("ordonnancement mode=contiguous_reference ouvriers=%zu "
              "latence_ms=%.1f pic_rss_kio=%llu octets_reserves_max=0 "
              "signature=%llx\n",
              w_ref, ms_ref, (unsigned long long)rss_ref,
              (unsigned long long)sig_ref);
  // 2 et 3. LPT sans borne, puis LPT sous budget.
  const struct { const char* name; u64 budget; } modes[2] = {
      {"LPT_unbounded", UINT64_MAX}, {"memory_budgeted_LPT", a.fold_budget}};
  for (const auto& md : modes) {
    for (size_t i = 1; i <= nk; ++i) res[i] = ForestResult{};
    reset_peak_rss();
    const FoldSchedule fs =
        run_folds_budgeted(W, M, a.threads, md.budget, one_fold);
    const u64 sig = signature_all();
    same = same && sig == sig_ref;
    std::printf("ordonnancement mode=%s ouvriers=%zu latence_ms=%.1f "
                "pic_rss_kio=%llu octets_reserves_max=%llu signature=%llx\n",
                md.name, fs.workers, fs.wall_ms,
                (unsigned long long)fs.peak_rss_kib_after,
                (unsigned long long)fs.max_reserved,
                (unsigned long long)sig);
  }
  if (!same) {
    std::fprintf(stderr, "ORDONNANCEMENT : signature differente entre modes\n");
    return 3;
  }
  return 0;
}

// BANC APPARIE DU PREFILTRE q4 (reponse d'audit `5b89bc6` § 5.4 : « son
// cout apparie intra-processus »). Meme discipline que le banc
// d'internement : echauffement non chronometre, ordre ABBA, plan refuse
// si `--bench-repeat` est impair ou < 4, signature du flux verifiee a
// chaque execution, mediane des rapports APPARIES comme estimateur.
int run_q4_prefilter_bench(const Args& a) {
  if (!bench_schedule_ok(a.bench_repeat)) {
    std::fprintf(stderr,
                 "REFUS : --bench-repeat=%d — le contrebalancement exige "
                 "un nombre PAIR de paires >= 4\n",
                 a.bench_repeat);
    return 2;
  }
  const std::vector<P3> pts = make_family_cloud(
      a.family, a.n,
      a.coord ? a.coord : cloud_family_default_coord(a.family, a.n), a.seed);
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) return 3;
  struct Run {
    double ms = 0;
    u64 sig = 0;
    u64 rejects = 0, tested = 0;
  };
  const auto one = [&](bool prefilter) {
    std::vector<BallCandidate> cs;
    BallStreamStats ss;
    const auto t0 = std::chrono::steady_clock::now();
    collect_candidate_balls(ix, a.s, a.smax, &cs, &ss, false, false, 0,
                            a.threads, false, false, false, false, prefilter);
    const double ms = std::chrono::duration<double, std::milli>(
                          std::chrono::steady_clock::now() - t0).count();
    std::stable_sort(cs.begin(), cs.end(), ball_candidate_less);
    cs.erase(std::unique(cs.begin(), cs.end(),
                         [](const BallCandidate& x, const BallCandidate& y) {
                           return x.key == y.key;
                         }),
             cs.end());
    u64 h = 1469598103934665603ull;
    const auto mix = [&](u64 v) { h ^= v; h *= 1099511628211ull; };
    mix(cs.size());
    for (const BallCandidate& c : cs) {
      mix((u64)c.arity);
      mix((u64)(c.key.a & 0xFFFFFFFFFFFFFFFFull));
      mix((u64)(c.key.c & 0xFFFFFFFFFFFFFFFFull));
    }
    return Run{ms, h, ss.q4_eq_reject, ss.q4_eq_tested};
  };
  const Run warm_on = one(true), warm_off = one(false);
  if (warm_on.sig != warm_off.sig) {
    std::fprintf(stderr, "BANC : les deux chemins ne rendent pas le meme flux\n");
    return 3;
  }
  std::printf("q4_prefilter_bench famille=%s n=%d s=%lld smax=%llu "
              "paires_testees=%llu rejets=%llu\n",
              cloud_family_name(a.family), a.n, (long long)a.s,
              (unsigned long long)a.smax,
              (unsigned long long)warm_off.tested,
              (unsigned long long)warm_off.rejects);
  std::vector<BenchPair> pairs;
  bool same = true;
  for (int i = 0; i < a.bench_repeat; ++i) {
    const bool off_first = (i % 2) == 0;  // « tri » = sans prefiltre
    BenchPair bp;
    bp.tri_first = off_first;
    for (int k = 0; k < 2; ++k) {
      const bool prefilter = (k == 0) != off_first;
      const Run r = one(prefilter);
      (prefilter ? bp.stream : bp.tri) = r.ms;
      same = same && r.sig == warm_on.sig;
      std::printf("  repet=%d position=%d prefiltre=%s t_gen_ms=%.1f\n", i, k,
                  prefilter ? "oui" : "non", r.ms);
    }
    pairs.push_back(bp);
  }
  const PairedReport rep = paired_report(pairs);
  std::printf("q4_prefilter_bench_paires");
  for (const BenchPair& bp : pairs)
    std::printf(" %.4f", bp.tri > 0 ? bp.stream / bp.tri : 0.0);
  std::printf("\n");
  std::printf("q4_prefilter_bench mediane_appariee=%.4f mediane_log=%.4f "
              "rapport_de_medianes=%.4f victoires_prefiltre=%llu/%llu "
              "ordre_sans_premier=%llu/%llu flux_identique=%s\n",
              rep.median_ratio, rep.median_log_diff, rep.ratio_of_medians,
              (unsigned long long)rep.wins_stream,
              (unsigned long long)(rep.wins_stream + rep.wins_tri),
              (unsigned long long)rep.pairs_tri_first,
              (unsigned long long)rep.pairs_stream_first,
              same ? "oui" : "NON");
  return same ? 0 : 3;
}

int run_intern_bench(const Args& a) {
  std::vector<std::vector<ForestEvent>> evk;
  u64 kmax = 0;
  if (build_events_for_bench(a, &evk, &kmax) != 0) return 3;
  // Le K le plus lourd porte l'essentiel des incidences : c'est lui que
  // le banc mesure (les petits K ne discriminent rien).
  int kbest = 1;
  u64 best = 0, recs_best = 0;
  for (int K = 1; K <= (int)kmax; ++K) {
    u64 rc = 0;
    for (const ForestEvent& ev : evk[(size_t)K]) rc += (u64)ev.q + ev.d;
    if (rc > best) best = rc, kbest = K, recs_best = rc;
  }
  if (recs_best == 0) return 3;
  std::printf("intern_bench famille=%s n=%d s=%lld smax=%llu K=%d "
              "evenements=%zu incidences=%llu\n",
              cloud_family_name(a.family), a.n, (long long)a.s,
              (unsigned long long)a.smax, kbest, evk[(size_t)kbest].size(),
              (unsigned long long)recs_best);
  if (!bench_schedule_ok(a.bench_repeat)) {
    std::fprintf(stderr,
                 "REFUS : --bench-repeat=%d — le contrebalancement exige "
                 "un nombre PAIR de paires >= 4\n",
                 a.bench_repeat);
    return 2;
  }
  const auto run_mode = [&](int mode) {
    return build_forest(evk[(size_t)kbest], false, false, nullptr, false,
                        false, false, false, 0, 0, 0, 0, 0, mode);
  };
  // ECHAUFFEMENT : un passage NON chronometre de chaque mode, pour que la
  // premiere paire ne porte pas seule le cout de demarrage (pages,
  // frequence, code chaud).
  const u64 sig = forest_signature(run_mode(1));
  if (forest_signature(run_mode(0)) != sig) {
    std::fprintf(stderr, "BANC : les deux modes ne rendent pas le meme objet\n");
    return 3;
  }
  // ORDRE CONTREBALANCE (contre-audit § 1) : bloc ABBA — paire paire =
  // tri puis streaming, paire impaire = streaming puis tri. Chaque mode
  // occupe alors autant de fois chaque position, et l'etat laisse par le
  // premier passage ne peut plus favoriser systematiquement le second.
  std::vector<BenchPair> pairs;
  bool same = true;
  for (int i = 0; i < a.bench_repeat; ++i) {
    const bool tri_first = (i % 2) == 0;
    BenchPair bp;
    bp.tri_first = tri_first;
    for (int k = 0; k < 2; ++k) {
      const int mode = (k == 0) == tri_first ? 1 : 0;
      const ForestResult rres = run_mode(mode);
      (mode == 1 ? bp.tri : bp.stream) = rres.t_intern_ms;
      same = same && forest_signature(rres) == sig;
      std::printf("  repet=%d position=%d mode=%s t_intern_ms=%.1f "
                  "t_scan_ms=%.1f t_sort_ms=%.1f t_remap_ms=%.1f "
                  "facettes=%llu\n",
                  i, k, mode == 1 ? "tri" : "streaming", rres.t_intern_ms,
                  rres.t_intern_scan_ms, rres.t_intern_sort_ms,
                  rres.t_intern_remap_ms, (unsigned long long)rres.facets);
    }
    pairs.push_back(bp);
  }
  const PairedReport rep = paired_report(pairs);
  std::printf("intern_bench_paires");
  for (const BenchPair& bp : pairs)
    std::printf(" %.4f", bp.tri > 0 ? bp.stream / bp.tri : 0.0);
  std::printf("\n");
  std::printf("intern_bench mediane_appariee=%.4f mediane_log=%.4f "
              "rapport_de_medianes=%.4f victoires_streaming=%llu/%llu "
              "ordre_tri_premier=%llu/%llu objet_identique=%s\n",
              rep.median_ratio, rep.median_log_diff, rep.ratio_of_medians,
              (unsigned long long)rep.wins_stream,
              (unsigned long long)(rep.wins_stream + rep.wins_tri),
              (unsigned long long)rep.pairs_tri_first,
              (unsigned long long)rep.pairs_stream_first,
              same ? "oui" : "NON");
  return same ? 0 : 3;
}

int run_fold_compact_gate(bool inj_root, int inj_intern, int inj_det) {
  u64 bad = 0;
  // PLANCHERS DE NON-VACUITE (doctrine : jamais de vert par vacuite) :
  // la porte ne prouve l'internement que si elle DEDOUBLONNE vraiment
  // (incidences > facettes uniques), si elle voit plusieurs lots et si
  // les facettes se comptent en centaines de milliers.
  u64 tot_recs = 0, tot_facets = 0, tot_batches = 0;
  // FIXTURE DE FLUX INCOHERENT (permanente, coordonnees symboliques) :
  // deux evenements K=2 dont la MEME facette {1,2} est un ATTACHEMENT
  // dans deux lots distincts. L'invariant des rayons de naissance
  // (MATHEMATIQUES § 5.2) l'interdit sur un flux geometrique — c'est
  // precisement ce que compte `attach_violations`, et RIEN jusqu'ici ne
  // prouvait que ce detecteur puisse se declencher : tous les tests ne
  // verifiaient que sa nullite (vert par vacuite du compteur lui-meme).
  //
  // Elle grave AUSSI la semantique de `first_batch` : sur un flux
  // coherent, `existed` est REDONDANT avec `active` (une facette deja
  // vue est active au lot suivant, sinon elle serait une violation),
  // donc le minimum des lots n'est observable QUE sur un flux
  // incoherent. C'est le seul endroit ou le mutant
  // intern-first-batch-last peut mourir — mesure du 18 aout : sur les
  // familles geometriques il ne change AUCUNE sortie.
  {
    std::vector<ForestEvent> fx(2);
    for (int i = 0; i < 2; ++i) {
      fx[(size_t)i].q = 2;
      fx[(size_t)i].d = 1;
      fx[(size_t)i].active_mask = 0x3;  // les deux retraits de support
      fx[(size_t)i].support[0] = 1;
      fx[(size_t)i].support[1] = 2;
      fx[(size_t)i].interior[0] = (PointId)(3 + i);
      fx[(size_t)i].level = promote_q3_level(q2_exact_level(i == 0 ? 100 : 400));
    }
    const ForestResult lg = build_forest_legacy(fx);
    if (lg.batches != 2 || lg.attach_violations != 1 ||
        lg.new_attachments != 1) {
      std::fprintf(stderr,
                   "FIXTURE : flux incoherent non detecte (lots=%llu "
                   "attach=%llu nees=%llu)\n",
                   (unsigned long long)lg.batches,
                   (unsigned long long)lg.attach_violations,
                   (unsigned long long)lg.new_attachments);
      return 3;
    }
    const ForestResult cp =
        build_forest(fx, false, false, nullptr, false, false, inj_root, true,
                     0, 0, 0, 0, inj_intern, 0, inj_det);
    // Ce que la fixture exige ICI est le DETECTEUR, pas l'egalite des
    // sorties : le flux viole par construction l'hypothese du theoreme
    // (`attach_violations = 0`) sous laquelle les deux backends sont
    // prouves egaux. Depuis la reponse d'audit `95061c1`, le fold ne lit
    // plus `first_batch` dans sa semantique — donc hors hypothese, le
    // backend fige et le fold compact classent legitimement `{1,2}`
    // differemment. Exiger leur egalite ici reviendrait a graver un
    // comportement hors contrat.
    if (cp.batches != lg.batches || cp.attach_violations != 1) {
      std::fprintf(stderr,
                   "FIXTURE : detecteur muet ou lots faux (lots=%llu "
                   "attach=%llu)\n",
                   (unsigned long long)cp.batches,
                   (unsigned long long)cp.attach_violations);
      ++bad;
    }
  }
  for (const CloudFamily fam :
       {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    const int n = fam == CloudFamily::kUniform ? 300 : 120;
    const std::vector<P3> pts =
        make_family_cloud(fam, n, cloud_family_default_coord(fam, n), 3);
    const CloudIndex ix = build_cloud_index(pts);
    if ((size_t)ix.unique_count() != pts.size()) return 3;
    std::vector<BallCandidate> cs;
    BallStreamStats ss;
    collect_candidate_balls(ix, 8, 11, &cs, &ss);
    std::stable_sort(cs.begin(), cs.end(), ball_candidate_less);
    cs.erase(std::unique(cs.begin(), cs.end(),
                         [](const BallCandidate& x, const BallCandidate& y) {
                           return x.key == y.key;
                         }),
             cs.end());
    std::vector<Survivor> sv;
    BallStreamStats ds;
    prefilter_balls(ix, cs, 11, false, false, &sv, &ds, 1);
    std::vector<BallData> balls;
    if (census_balls(ix, cs, sv, 11, 12, false, false, &balls, &ds, 1,
                     false) != 0)
      return 3;
    std::vector<PointId> pid_of((size_t)ix.unique_count());
    for (size_t u = 0; u < pid_of.size(); ++u)
      pid_of[u] = ix.point_id((i32)u);
    u64 e1[11] = {};
    ForestResult rr[11];
    std::vector<std::vector<ForestEvent>> evk;
    if (forests_from_balls(balls, ix.upos, pid_of, 10, e1, rr, &evk, 1,
                           true) != 0)
      return 3;
    for (int K = 1; K <= 10; ++K) {
      const ForestResult lg = build_forest_legacy(evk[(size_t)K]);
      const ForestResult cp = build_forest(evk[(size_t)K], false, false,
                                           nullptr, false, false, inj_root,
                                           true, 0, 0, 0, 0, inj_intern);
      for (const ForestEvent& ev : evk[(size_t)K])
        tot_recs += (u64)ev.q + ev.d;
      tot_facets += lg.facets;
      tot_batches += lg.batches;
      // TROISIEME BACKEND : le meme fold compact avec l'internement par
      // TRI GLOBAL (mode 1), conserve comme comparande mesurable. Les
      // deux internements doivent rendre des sorties IDENTIQUES — c'est
      // la porte qui autorise `--fold-intern-bench` a les alterner dans
      // un meme processus sans changer l'objet.
      {
        const ForestResult tr =
            build_forest(evk[(size_t)K], false, false, nullptr, false, false,
                         inj_root, true, 0, 0, 0, 0, inj_intern, 1, inj_det);
        if (tr.facets != cp.facets || tr.fusions != cp.fusions ||
            tr.batches != cp.batches ||
            tr.new_attachments != cp.new_attachments ||
            tr.attach_violations != cp.attach_violations ||
            tr.deltas.size() != cp.deltas.size() ||
            !(tr.final_partition == cp.final_partition) ||
            tr.facet_keys != cp.facet_keys ||
            tr.final_canon_fid != cp.final_canon_fid) {
          std::fprintf(stderr,
                       "FOLD : divergence tri/streaming (%s, K=%d)\n",
                       cloud_family_name(fam), K);
          ++bad;
        }
      }
      bool same = lg.facets == cp.facets && lg.fusions == cp.fusions &&
                  lg.batches == cp.batches &&
                  lg.new_attachments == cp.new_attachments &&
                  lg.attach_violations == cp.attach_violations &&
                  lg.birth_violations == cp.birth_violations &&
                  lg.nodes.size() == cp.nodes.size() &&
                  lg.deltas.size() == cp.deltas.size() &&
                  lg.batch_levels.size() == cp.batch_levels.size() &&
                  lg.final_partition == cp.final_partition &&
                  cp.partition_violations == 0 &&
                  cp.facet_keys.size() == cp.final_canon_fid.size();
      for (size_t i = 0; same && i < lg.nodes.size(); ++i)
        same = lg.nodes[i].batch == cp.nodes[i].batch &&
               lg.nodes[i].absorbed == cp.nodes[i].absorbed;
      for (size_t i = 0; same && i < lg.deltas.size(); ++i)
        same = lg.deltas[i] == cp.deltas[i];
      for (size_t i = 0; same && i < lg.batch_levels.size(); ++i)
        same = same_level_representation(lg.batch_levels[i],
                                         cp.batch_levels[i]);
      // Vue dense contre map legacy, paire a paire.
      size_t fid = 0;
      for (const auto& kv : lg.final_partition) {
        same = same && fid < cp.facet_keys.size() &&
               kv.first == cp.facet_keys[fid] &&
               kv.second ==
                   cp.facet_keys[(size_t)cp.final_canon_fid[fid]];
        ++fid;
      }
      if (!same) {
        std::fprintf(stderr,
                     "FOLD COMPACT : divergence legacy/dense (%s, K=%d)\n",
                     cloud_family_name(fam), K);
        ++bad;
      }
    }
  }
  std::printf("fold_compact_gate violations=%llu incidences=%llu "
              "facettes=%llu lots=%llu\n",
              (unsigned long long)bad, (unsigned long long)tot_recs,
              (unsigned long long)tot_facets,
              (unsigned long long)tot_batches);
  if (inj_root || inj_intern || inj_det) {
    if (bad > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (bad) return 3;
  // Planchers : sans dedoublonnage effectif ni pluralite de lots, la
  // porte ne dirait rien de l'internement.
  if (tot_facets < 200000 || tot_recs <= tot_facets || tot_batches < 1000) {
    std::fprintf(stderr,
                 "PLANCHER : incidences=%llu facettes=%llu lots=%llu\n",
                 (unsigned long long)tot_recs, (unsigned long long)tot_facets,
                 (unsigned long long)tot_batches);
    return 3;
  }
  return 0;
}

// PORTE DE PARALLELISME (directive « paralléliser ») : l'objet post-RLE
// est INDEPENDANT du decoupage. collect a 1 fil CONTRE 4 fils — egalite
// au bit pres des cles/arites/representations apres tri+RLE, ET egalite
// des compteurs de generation (sommes independantes de l'ordre :
// candidats, ancres, morts de profondeur, W4, cœur, groupes axiaux).
// Les deux chemins (baseline et axial) sont couverts. MUTANT
// par-drop-shard : la fusion oublie le premier ouvrier — les compteurs
// le voient toujours, le jeu de cles souvent.
int run_par_gate(bool inj_drop, bool inj_drop_chunk) {
  u64 bad = 0;
  for (const CloudFamily fam :
       {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    const int n = 400;
    const std::vector<P3> pts =
        make_family_cloud(fam, n, cloud_family_default_coord(fam, n), 3);
    const CloudIndex ix = build_cloud_index(pts);
    if ((size_t)ix.unique_count() != pts.size()) return 3;
    for (int axial = 0; axial < 2; ++axial) {
      std::vector<BallCandidate> c1, c4;
      BallStreamStats s1, s4;
      collect_candidate_balls(ix, 8, 11, &c1, &s1, false, axial != 0, 0, 1,
                              false);
      collect_candidate_balls(ix, 8, 11, &c4, &s4, false, axial != 0, 0, 4,
                              inj_drop);
      const auto rle = [](std::vector<BallCandidate>* v) {
        std::stable_sort(v->begin(), v->end(), ball_candidate_less);
        v->erase(
            std::unique(v->begin(), v->end(),
                        [](const BallCandidate& x, const BallCandidate& y) {
                          return x.key == y.key;
                        }),
            v->end());
      };
      rle(&c1);
      rle(&c4);
      bool same = c1.size() == c4.size();
      for (size_t i = 0; same && i < c1.size(); ++i)
        same = c1[i].key == c4[i].key && c1[i].arity == c4[i].arity &&
               same_level_representation(c1[i].level, c4[i].level);
      bool counters = true;
      for (int i = 0; i < 3; ++i)
        counters = counters && s1.candidates[i] == s4.candidates[i] &&
                   s1.anchors[i] == s4.anchors[i] &&
                   s1.gen_depth_killed[i] == s4.gen_depth_killed[i];
      counters = counters && s1.anchors_killed_w4 == s4.anchors_killed_w4 &&
                 s1.seeds_killed_seed_core == s4.seeds_killed_seed_core &&
                 s1.seed_core_sites == s4.seed_core_sites &&
                 s1.axial_groups_emitted == s4.axial_groups_emitted &&
                 s1.axial_roots_pruned_cross == s4.axial_roots_pruned_cross &&
                 s1.axial_groups_killed_depth == s4.axial_groups_killed_depth;
      if (!same || !counters) {
        std::fprintf(stderr,
                     "PAR : divergence 1 fil / 4 fils (%s, axial=%d, "
                     "cles=%d compteurs=%d)\n",
                     cloud_family_name(fam), axial, (int)same, (int)counters);
        ++bad;
      }
    }
    // AVAL : prefiltre + census + expansion + folds, 1 fil CONTRE 4 —
    // survivantes identiques (idx, profondeur), evenements BIT-EXACTS
    // par K (q, d, masque, support, interieurs, representation de
    // niveau), resumes de foret identiques (fusions, nœuds, lots). Le
    // mutant par-drop-ball-chunk (une tranche de census oubliee a la
    // fusion) est applique au seul cote 4 fils.
    {
      std::vector<BallCandidate> cs;
      BallStreamStats ss;
      collect_candidate_balls(ix, 8, 11, &cs, &ss);
      std::stable_sort(cs.begin(), cs.end(), ball_candidate_less);
      cs.erase(std::unique(cs.begin(), cs.end(),
                           [](const BallCandidate& x, const BallCandidate& y) {
                             return x.key == y.key;
                           }),
               cs.end());
      std::vector<PointId> pid_of((size_t)ix.unique_count());
      for (size_t u = 0; u < pid_of.size(); ++u)
        pid_of[u] = ix.point_id((i32)u);
      bool down_ok = true;
      std::vector<Survivor> sv1, sv4;
      BallStreamStats d1, d4;
      prefilter_balls(ix, cs, 11, false, false, &sv1, &d1, 1);
      prefilter_balls(ix, cs, 11, false, false, &sv4, &d4, 4);
      down_ok = down_ok && sv1.size() == sv4.size();
      for (size_t i = 0; down_ok && i < sv1.size(); ++i)
        down_ok = sv1[i].idx == sv4[i].idx && sv1[i].depth == sv4[i].depth;
      std::vector<BallData> b1, b4;
      if (census_balls(ix, cs, sv1, 11, 12, false, false, &b1, &d1, 1,
                       false) != 0)
        return 3;
      if (census_balls(ix, cs, sv4, 11, 12, false, false, &b4, &d4, 4,
                       inj_drop_chunk) != 0)
        return 3;
      down_ok = down_ok && b1.size() == b4.size();
      u64 e1[11] = {}, e4[11] = {};
      ForestResult r1[11], r4[11];
      std::vector<std::vector<ForestEvent>> k1, k4;
      if (forests_from_balls(b1, ix.upos, pid_of, 10, e1, r1, &k1, 1) != 0)
        return 3;
      if (forests_from_balls(b4, ix.upos, pid_of, 10, e4, r4, &k4, 4) != 0)
        return 3;
      for (int K = 1; K <= 10 && down_ok; ++K) {
        down_ok = e1[K] == e4[K] && r1[K].fusions == r4[K].fusions &&
                  r1[K].nodes.size() == r4[K].nodes.size() &&
                  r1[K].batches == r4[K].batches &&
                  k1[(size_t)K].size() == k4[(size_t)K].size();
        for (size_t i = 0; down_ok && i < k1[(size_t)K].size(); ++i) {
          const ForestEvent &x = k1[(size_t)K][i], &y = k4[(size_t)K][i];
          down_ok = x.q == y.q && x.d == y.d &&
                    x.active_mask == y.active_mask &&
                    same_level_representation(x.level, y.level);
          for (int t = 0; down_ok && t < x.q; ++t)
            down_ok = x.support[t] == y.support[t];
          for (int t = 0; down_ok && t < x.d; ++t)
            down_ok = x.interior[t] == y.interior[t];
        }
      }
      if (!down_ok) {
        std::fprintf(stderr, "PAR : divergence AVAL 1 fil / 4 fils (%s)\n",
                     cloud_family_name(fam));
        ++bad;
      }
    }
  }
  std::printf("par_gate violations=%llu\n", (unsigned long long)bad);
  if (inj_drop || inj_drop_chunk) {
    if (bad > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return bad ? 3 : 0;
}

// PORTE SYNTHETIQUE DE LA PRIMITIVE DE SWEEP (audit « sweep reçu et
// kernel sans alloc » § 3) : multisets de racines SANS geometrie — la
// causalite des mutants de fenetre et de d_j s'etablit ICI (la fixture
// geometrique 1513/49 tue son completeur a la classification, elle ne
// peut pas isoler ignore-opposite/reverse-negative a elle seule) ; la
// fixture geometrique reste la porte d'integration vers les BallKey.
int run_axial_sweep_gate(u32 inj) {
  u64 bad = 0;
  // Multiset grave par l'audit : une positive mu=0, trois negatives
  // mu=1,2,3, p=0, h=3. Normal : la positive est rejetee par le cote
  // OPPOSE (sous L=1), le groupe mu=1 meurt en fenetre (d_j=3), mu=2 et
  // mu=3 vivent. ignore-opposite : la positive ne lit plus L et survit ;
  // reverse-negative : les verdicts de mu=1 et mu=3 s'INVERSENT.
  {
    const AxialSite sites[4] = {
        {0, 1, 10}, {-1, -1, 11}, {-2, -1, 12}, {-3, -1, 13}};
    u8 gid[4];
    const AxialSweepResult r =
        axial_two_sided_sweep(sites, 4, 0, 3, inj, gid);
    const bool ok = r.roots_pruned_cross == 1 && r.ngroups == 3 &&
                    !r.groups[0].alive && r.groups[0].dj == 3 &&
                    r.groups[1].alive && r.groups[2].alive &&
                    r.groups_killed_depth == 1 && gid[0] == 0xff &&
                    !r.overflow;
    if (!ok) {
      std::fprintf(stderr, "SWEEP : multiset 0 | 1,2,3 devie\n");
      ++bad;
    }
  }
  // Groupe MIXTE : une meme mu exacte portee par les DEUX signes ne
  // forme qu'un groupe (npos=1, nneg=1), fenetre [0,5] pleine, aucune
  // racine croisee, tous vivants.
  {
    const AxialSite sites[4] = {{2, 1, 1}, {5, 1, 2}, {-2, -1, 3}, {0, -1, 4}};
    u8 gid[4];
    const AxialSweepResult r =
        axial_two_sided_sweep(sites, 4, 0, 2, inj, gid);
    const bool ok = r.ngroups == 3 && r.roots_pruned_cross == 0 &&
                    r.groups[1].npos == 1 && r.groups[1].nneg == 1 &&
                    r.groups[0].alive && r.groups[1].alive &&
                    r.groups[2].alive && gid[0] == gid[2];
    if (!ok) {
      std::fprintf(stderr, "SWEEP : multiset mixte devie\n");
      ++bad;
    }
  }
  // Ties AU seuil : U=2 porte par deux racines egales — les deux restent
  // en fenetre (drop-ties les perd), p=1 compte dans d_j.
  {
    const AxialSite sites[3] = {{1, 1, 1}, {2, 1, 2}, {2, 1, 3}};
    u8 gid[3];
    const AxialSweepResult r =
        axial_two_sided_sweep(sites, 3, 1, 3, inj, gid);
    const bool ok = r.ngroups == 2 && r.groups[1].npos == 2 &&
                    r.groups[0].alive && r.groups[1].alive &&
                    r.groups[1].dj == 2;
    if (!ok) {
      std::fprintf(stderr, "SWEEP : multiset ties devie\n");
      ++bad;
    }
  }
  std::printf("axial_sweep_gate violations=%llu\n", (unsigned long long)bad);
  if (inj != 0) {
    if (bad > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return bad ? 3 : 0;
}

// PORTE APPARIEE DE LA SELECTION AXIALE (audit « axial borne » § 6.1,
// etendue par le contre-audit 63d364a « sweep a deux cotes ») : baseline
// enumeree CONTRE axial borne, comparees apres tri/RLE — cles, arite et
// REPRESENTATION de niveau a l'identique (jamais les candidats bruts : le
// but est precisement de ne plus les produire). Chaque run axial porte
// kAxialVerify : d_j est recoupe par le scan q4_power complet sur TOUS
// les groupes en fenetre (morts compris — audit « sweep reçu » § 3), et
// tout desaccord est une violation. Mutants tues ici : short-group et
// drop-ties PERDENT des cles ; first-rep emet un representant non
// canonique (discrimine PRE-RLE) ; ignore-opposite-side, devenu CAUSAL
// (il saute aussi le seuil croise), fait survivre la sphere 1513/49 de
// la FIXTURE § 5 a smax=6 ; depth-nonstrict la tue a tort a smax=7 ;
// reverse-negative, qui ne touche que d_j en fenetre, est isole
// causalement par la porte synthetique --axial-sweep-gate et meurt ici
// sur les nuages generaux. Le chemin --axial-off n'est pas un mutant :
// c'est cette baseline.
int run_axial_pair_gate(u32 inj) {
  u64 bad = 0;
  u64 base_candidates = 0, axial_candidates = 0, groups = 0;
  // Trois nuages : deux familles + la SPHERE COSPHERIQUE R²=50 (84 points
  // de reseau, centre (100,100,100)) — un meme groupe de mu y possede de
  // NOMBREUX membres valides aux representations differentes : c'est elle
  // qui discrimine le mutant first-rep (le minimum canonique doit gagner).
  std::vector<std::vector<P3>> clouds;
  for (const CloudFamily fam :
       {CloudFamily::kUniform, CloudFamily::kEightClusters})
    clouds.push_back(
        make_family_cloud(fam, 120, cloud_family_default_coord(fam, 120), 3));
  {
    std::vector<P3> sph;
    const i64 offs[3][3] = {{5, 5, 0}, {7, 1, 0}, {5, 4, 3}};
    for (const auto& o : offs) {
      i64 v[3] = {o[0], o[1], o[2]};
      std::sort(v, v + 3);
      do {
        for (int sx = -1; sx <= 1; sx += 2)
          for (int sy = -1; sy <= 1; sy += 2)
            for (int sz = -1; sz <= 1; sz += 2) {
              const P3 p{100 + sx * v[0], 100 + sy * v[1], 100 + sz * v[2]};
              bool seen = false;
              for (const P3& q : sph)
                seen = seen || (q.x == p.x && q.y == p.y && q.z == p.z);
              if (!seen) sph.push_back(p);
            }
      } while (std::next_permutation(v, v + 3));
    }
    clouds.push_back(std::move(sph));
  }
  for (const std::vector<P3>& pts : clouds) {
    const CloudIndex ix = build_cloud_index(pts);
    if ((size_t)ix.unique_count() != pts.size()) return 3;
    std::vector<BallCandidate> cb, ca;
    BallStreamStats sb, sa;
    collect_candidate_balls(ix, 8, 11, &cb, &sb, false, false);
    collect_candidate_balls(ix, 8, 11, &ca, &sa, false, true,
                            inj | kAxialVerify);
    base_candidates += sb.candidates[2];
    axial_candidates += sa.candidates[2];
    groups += sa.axial_groups_emitted;
    if (sa.axial_verify_mismatch != 0) {
      std::fprintf(stderr,
                   "AXIAL : d_j != scan complet sur %llu groupes (n=%zu)\n",
                   (unsigned long long)sa.axial_verify_mismatch, pts.size());
      ++bad;
    }
    if (inj & kAxialFirstRep) {
      // Le mutant first-rep est masque post-RLE par la re-canonicalisation
      // inter-seeds (le minimum global d'une cle revient par un autre
      // seed). Discrimination PRE-RLE : les emissions brutes du mutant
      // doivent differer de celles du chemin normal quelque part — cela
      // prouve que le choix du minimum canonique est exerce, et l'egalite
      // appariee (hors mutant) prouve qu'il est le bon.
      std::vector<BallCandidate> cn;
      BallStreamStats sn;
      collect_candidate_balls(ix, 8, 11, &cn, &sn, false, true,
                              (inj & ~(u32)kAxialFirstRep) | kAxialVerify);
      std::vector<BallCandidate> ra = ca, rn = cn;
      std::stable_sort(ra.begin(), ra.end(), ball_candidate_less);
      std::stable_sort(rn.begin(), rn.end(), ball_candidate_less);
      bool raw_same = ra.size() == rn.size();
      for (size_t i = 0; raw_same && i < ra.size(); ++i)
        raw_same = ra[i].key == rn[i].key &&
                   same_level_representation(ra[i].level, rn[i].level);
      if (!raw_same) {
        std::fprintf(stderr,
                     "AXIAL : first-rep devie du minimum canonique (n=%zu)\n",
                     pts.size());
        ++bad;
      }
    }
    const auto rle = [](std::vector<BallCandidate>* v) {
      std::stable_sort(v->begin(), v->end(), ball_candidate_less);
      v->erase(std::unique(v->begin(), v->end(),
                           [](const BallCandidate& x, const BallCandidate& y) {
                             return x.key == y.key;
                           }),
               v->end());
    };
    rle(&cb);
    rle(&ca);
    bool same = cb.size() == ca.size();
    for (size_t i = 0; same && i < cb.size(); ++i)
      same = cb[i].key == ca[i].key && cb[i].arity == ca[i].arity &&
             same_level_representation(cb[i].level, ca[i].level);
    if (!same) {
      std::fprintf(stderr, "AXIAL : divergence baseline/borne (n=%zu)\n",
                   pts.size());
      ++bad;
    }
  }
  // FIXTURE-CŒUR (audit « axial arbre et cœur de seed » § 4.2) : la
  // frontiere du cœur seed-local n'est PAS comptee. Six points sur le
  // cercle R² = 25 du plan z = 10 (centre (15,10)) : le seed (a,b,x) est
  // strictement aigu (arcs 90°/126,87°/143,13°), c1,c2,c3 sont
  // COCIRCULAIRES au seed (P = 0, B = 0 : sur toute sphere du faisceau —
  // le cas d'egalite 2P² = J·B²), et y = (15,10,16) complete la sphere
  // R² = 3721/144 dont les c_i sont des points de COQUILLE (aucun
  // interieur strict dans le nuage). AUCUNE paire antipodale parmi les
  // six points du cercle (216,87°, 0°, 90°, 143,13°, 53,13°, 126,87°) :
  // un diametre + y donnerait un grand cercle de la MEME sphere et la
  // lane q3 emettrait la cle en secours — c'est arrive avec c1 a 36,87°,
  // l'antipode de a. Regle stricte : aucun temoin, la cle est emise a
  // smax=6 par les DEUX chemins. Mutant seed-core-nonstrict : les
  // cocirculaires hors seed comptent (0 >= 0), tout seed porte par le
  // cercle meurt, et l'exact-once (pid(y) maximal — y dernier point)
  // comme le centre hors tetraedre interdisent toute emission de secours
  // par un seed hors du plan : la cle disparait du chemin axial.
  {
    const std::vector<P3> fx = {{11, 7, 10},  {20, 10, 10}, {15, 15, 10},
                                {11, 13, 10}, {18, 14, 10}, {12, 14, 10},
                                {15, 10, 16}};
    const CloudIndex ix = build_cloud_index(fx);
    if ((size_t)ix.unique_count() != fx.size()) return 3;
    const Q4Form f4 = q4_form(fx[0], fx[1], fx[2], fx[6]);
    const Q3BallKey key = q3_ball_key_reduce(q4_ball_form(f4));
    std::vector<BallCandidate> cb, ca;
    BallStreamStats sb, sa;
    collect_candidate_balls(ix, 8, 6, &cb, &sb, false, false);
    collect_candidate_balls(ix, 8, 6, &ca, &sa, false, true,
                            inj | kAxialVerify);
    if (sa.axial_verify_mismatch != 0) {
      std::fprintf(stderr, "AXIAL fixture-cœur : d_j != scan complet\n");
      ++bad;
    }
    const auto has_key = [&](const std::vector<BallCandidate>& v) {
      for (const BallCandidate& c : v)
        if (c.key == key) return true;
      return false;
    };
    if (!has_key(cb) || !has_key(ca)) {
      std::fprintf(stderr,
                   "AXIAL fixture-cœur : sphere 3721/144 absente "
                   "(base=%d axial=%d)\n",
                   (int)has_key(cb), (int)has_key(ca));
      ++bad;
    }
  }
  // FIXTURE § 5 (contre-audit 63d364a) — la mort NE se lit que sur le cote
  // OPPOSE au completeur. Sphere circonscrite de {a,b,x,y} : centre
  // (15, 82/7, 82/7), niveau R² = 1513/49. Les trois z_i en sont
  // strictement interieurs, hors W_4 de l'ancre (2H² = 450 < Xi = 1000),
  // et tous du cote B < 0 de l'axe du seed (a,b,x) (plan z=10) tandis que
  // le completeur y est du cote B > 0 : d_cover = 3 vient du SEUL suffixe
  // negatif. A smax=6 (h4=3) la cle doit etre ABSENTE (emissions brutes)
  // et la mort par le cote oppose comptee ; a smax=7 (h4=4) la cle doit
  // etre PRESENTE au niveau semantique 1513/49. CAUSALITE (audit « sweep
  // reçu » § 3) : la mort se produit ici a la CLASSIFICATION (racine du
  // completeur sous L), donc seul ignore-opposite — qui saute le seuil
  // croise — fait survivre la sphere a smax=6 ; depth-nonstrict la tue a
  // tort a smax=7 ; reverse-negative ne touche que d_j en fenetre et
  // n'est PAS isole par cette fixture : sa causalite vit dans la porte
  // synthetique --axial-sweep-gate (inversion des verdicts mu=1/mu=3).
  // Deux variantes d'interieurs : (0) le triplet GRAVE par le contre-audit
  // — z_i si profonds (|mu| = 1770 > racine(J/2) ~ 433) qu'ils sont
  // temoins UNIVERSELS : c'est le cœur de seed qui tue, avant tout
  // tableau axial ; (1) un triplet NON universel dans la bande admissible
  // (mu = 400, 400, 320 dans (mu_y = 240, 433]) : le cœur ne compte rien
  // et c'est la lecture bilaterale du sweep qui tue. Les deux variantes
  // partagent la meme sphere 1513/49 (absente a smax=6, presente au bon
  // niveau a smax=7).
  for (int variant = 0; variant < 2; ++variant) {
    const std::vector<P3> fx =
        variant == 0
            ? std::vector<P3>{{10, 10, 10}, {20, 10, 10}, {15, 17, 10},
                              {15, 10, 17}, {15, 13, 9},  {14, 13, 9},
                              {16, 13, 9}}
            : std::vector<P3>{{10, 10, 10}, {20, 10, 10}, {15, 17, 10},
                              {15, 10, 17}, {19, 14, 9},  {11, 14, 9},
                              {17, 15, 8}};
    const CloudIndex ix = build_cloud_index(fx);
    if ((size_t)ix.unique_count() != fx.size()) return 3;
    const Q4Form f4 = q4_form(fx[0], fx[1], fx[2], fx[3]);
    const Q3BallKey key = q3_ball_key_reduce(q4_ball_form(f4));
    const Q4Level lvl = q4_level_raw(f4);
    for (const u64 sm : {u64(6), u64(7)}) {
      std::vector<BallCandidate> cb, ca;
      BallStreamStats sb, sa;
      collect_candidate_balls(ix, 8, sm, &cb, &sb, false, false);
      collect_candidate_balls(ix, 8, sm, &ca, &sa, false, true,
                              inj | kAxialVerify);
      if (sa.axial_verify_mismatch != 0) {
        std::fprintf(stderr,
                     "AXIAL fixture : d_j != scan complet (smax=%llu)\n",
                     (unsigned long long)sm);
        ++bad;
      }
      bool present = false, lvl_ok = false;
      for (const BallCandidate& c : ca)
        if (c.key == key) {
          present = true;
          lvl_ok = compare_exact_level(c.level, lvl) == 0;
          break;
        }
      if (sm == 6) {
        if (present) {
          std::fprintf(stderr,
                       "AXIAL fixture : sphere 1513/49 emise a smax=6 (v%d)\n",
                       variant);
          ++bad;
        }
        const bool killed = variant == 0
                                ? sa.seeds_killed_seed_core > 0
                                : sa.axial_roots_pruned_cross > 0;
        if (!killed) {
          std::fprintf(stderr,
                       "AXIAL fixture : aucune mort %s a smax=6 (v%d)\n",
                       variant == 0 ? "au cœur de seed" : "bilaterale",
                       variant);
          ++bad;
        }
      } else if (!present || !lvl_ok) {
        std::fprintf(stderr,
                     "AXIAL fixture : sphere 1513/49 %s a smax=7 (v%d)\n",
                     present ? "au mauvais niveau" : "absente", variant);
        ++bad;
      }
      const auto rle = [](std::vector<BallCandidate>* v) {
        std::stable_sort(v->begin(), v->end(), ball_candidate_less);
        v->erase(
            std::unique(v->begin(), v->end(),
                        [](const BallCandidate& x, const BallCandidate& y) {
                          return x.key == y.key;
                        }),
            v->end());
      };
      rle(&cb);
      rle(&ca);
      bool same = cb.size() == ca.size();
      for (size_t i = 0; same && i < cb.size(); ++i)
        same = cb[i].key == ca[i].key && cb[i].arity == ca[i].arity &&
               same_level_representation(cb[i].level, ca[i].level);
      if (!same) {
        std::fprintf(stderr,
                     "AXIAL fixture : divergence baseline/borne (smax=%llu)\n",
                     (unsigned long long)sm);
        ++bad;
      }
    }
  }
  if (axial_candidates > base_candidates || axial_candidates == 0) {
    std::fprintf(stderr, "AXIAL : plancher de reduction viole\n");
    ++bad;
  }
  std::printf(
      "axial_pair_gate base=%llu axial=%llu groupes=%llu violations=%llu\n",
      (unsigned long long)base_candidates, (unsigned long long)axial_candidates,
      (unsigned long long)groups, (unsigned long long)bad);
  if (inj != 0) {
    if (bad > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  return bad ? 3 : 0;
}

// GARDE D'ENTREE (audit § 5) : ids dupliques et coordonnees hors u16
// refuses AVANT la construction de l'arbre (refus = index vide).
int run_guard_gate(int mode) {
  if (mode == 1) {
    const std::vector<InputPoint> in = {{7, P3{1, 2, 3}}, {7, P3{4, 5, 6}},
                                        {9, P3{2, 2, 2}}};
    if ((size_t)build_cloud_index(in).unique_count() != in.size()) {
      std::fprintf(stderr, "REFUS invalid_input : PointId duplique\n");
      return 2;
    }
    std::fprintf(stderr, "GARDE INEFFICACE : doublon d'id accepte\n");
    return 3;
  }
  const std::vector<InputPoint> hi = {{1, P3{70000, 2, 3}}, {2, P3{4, 5, 6}}};
  const std::vector<InputPoint> neg = {{1, P3{-1, 2, 3}}, {2, P3{4, 5, 6}}};
  const bool r_hi = (size_t)build_cloud_index(hi).unique_count() != hi.size();
  const bool r_neg = (size_t)build_cloud_index(neg).unique_count() != neg.size();
  if (r_hi && r_neg) {
    std::fprintf(stderr, "REFUS invalid_input : coordonnee hors u16\n");
    return 2;
  }
  std::fprintf(stderr, "GARDE INEFFICACE : coordonnee hors profil acceptee\n");
  return 3;
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  const Args a = parse(argc, argv);
  if (!a.family_ok || a.n < 3 || a.s < 1) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  if (a.smax > 11) {
    std::fprintf(stderr, "REFUS : profil K_max<=10 (smax<=11)\n");
    return 2;
  }
  if (a.relabel_gate) return run_relabel_gate(a.inj_dense_pointid);
  if (a.depth_gate)
    return run_depth_gate(a.inj_threshold_minus_one, a.inj_range_add_le,
                          a.inj_shell_first);
  if (a.kmax_gate) return run_kmax_gate(a.inj_fold_kmax10);
  if (a.axial_sweep_gate) return run_axial_sweep_gate(a.inj_axial);
  if (a.axial_pair_gate) return run_axial_pair_gate(a.inj_axial);
  if (a.par_gate)
    return run_par_gate(a.inj_par_drop, a.inj_par_drop_census);
  if (a.q2_birth_gate) return run_q2_birth_gate(a.inj_birth_dup);
  if (a.q4_eq_gate) return run_q4_eq_gate(a.inj_q4_eq_wrong);
  if (a.q4_prefilter_bench) return run_q4_prefilter_bench(a);
  if (a.bench_report_gate) return run_bench_report_gate();
  if (a.schedule_bench) return run_schedule_bench(a);
  if (a.intern_bench) return run_intern_bench(a);
  if (a.fold_compact_gate)
    return run_fold_compact_gate(a.inj_canon_root, a.inj_fold_intern,
                                 a.inj_detector);
  if (a.float_gate)
    return run_float_gate((a.inj_axial & kFloatSmallThreshold) != 0);
  if (a.q3_affine_gate)
    return run_q3_affine_gate((a.inj_axial & kFloatSmallThreshold) != 0,
                              a.inj_jung_swap);
  if (a.float_rounding_gate)
    return run_float_rounding_gate((a.inj_axial & kFloatIgnoreRounding) != 0);
  if (a.fold_capacity_gate) return run_fold_capacity_gate(a.inj_fold_capacity);
  if (a.workers_gate)
    return run_workers_gate(a.inj_par_one_worker, a.inj_ranges_one_worker,
                            a.inj_q3_one_worker, a.inj_wspd_one_worker);
  if (a.digest_gate) return run_digest_gate();
  if (a.guard != 0) return run_guard_gate(a.guard);
  const std::vector<P3> pts = make_family_cloud(
      a.family, a.n,
      a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n), a.seed);
  // Enregistrements explicites {id, position} : le probe fabrique id = index
  // d'entree (commodite) — le noyau n'en deduit rien, la porte --relabel-gate
  // le garantit.
  std::vector<InputPoint> inputs(pts.size());
  for (size_t i = 0; i < pts.size(); ++i) inputs[i] = {(PointId)i, pts[i]};
  const u64 smax_eff = std::min<u64>(a.smax, pts.size());
  if (smax_eff < 5) {
    std::fprintf(stderr, "REFUS : s_max effectif trop petit\n");
    return 2;
  }
  const auto t0 = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(inputs);
  if ((size_t)ix.unique_count() != pts.size()) {
    std::fprintf(stderr, "REFUS unsupported_degeneracy : positions dupliquees\n");
    return 2;
  }

  // 1. Generateurs WSPD -> RLE par BallKey (arite minimale d'abord).
  std::vector<BallCandidate> cands;
  BallStreamStats st;
  collect_candidate_balls(ix, a.s, smax_eff, &cands, &st,
                          a.inj_genfilter_nonstrict, a.axial_on, a.inj_axial,
                          a.threads, a.inj_par_drop, false, false, false,
                          a.q4_eq_prefilter);
  const auto t0b = std::chrono::steady_clock::now();
  std::stable_sort(cands.begin(), cands.end(), ball_candidate_less);
  if (!a.inj_rle_drop)  // MUTANT : dedupe saute, boules re-censusees
    cands.erase(std::unique(cands.begin(), cands.end(),
                            [](const BallCandidate& x, const BallCandidate& y) {
                              return x.key == y.key;
                            }),
                cands.end());
  st.unique_balls = cands.size();
  const auto t1 = std::chrono::steady_clock::now();

  // 2. PASSE 1 count-only (seuil h_qmin par arite minimale), puis PASSE 2
  // census complet I_B/U_B sur les seules survivantes — temps SEPARES.
  // Sous mutant, un invariant qui se declenche (recoupement passe1 !=
  // passe2, roles) EST la mise a mort — convention du selftest.
  const bool any_inject = a.inj_rle_drop || a.inj_census_nonstrict ||
                          a.inj_dense_pointid || a.inj_threshold_minus_one ||
                          a.inj_range_add_le || a.inj_skip_full ||
                          a.inj_fold_kmax10 || a.inj_genfilter_nonstrict ||
                          a.inj_axial != 0 || a.inj_par_drop ||
                          a.inj_par_drop_census;
  // LE PARAMETRE QUI DEFINIT L'OBJET EN AMONT EXISTE EN AVAL (audit « smax
  // dynamique ») : caps de census par arite, expansion, folds et totaux
  // suivent tous smax_eff — plus jamais les constantes 9/11/10 (MUTANT
  // fold-hardcodes-kmax10, tue par la porte de frontiere K_max=5/6).
  const u64 smax_caps = a.inj_fold_kmax10 ? 11 : smax_eff;
  const u64 kmax_eff = a.inj_fold_kmax10 ? 10 : smax_eff - 1;
  std::vector<Survivor> surv;
  prefilter_balls(ix, cands, smax_caps, a.inj_threshold_minus_one,
                  a.inj_range_add_le, &surv, &st, a.threads);
  const auto t1b = std::chrono::steady_clock::now();
  std::vector<BallData> balls;
  {
    const int rc = census_balls(ix, cands, surv, smax_caps, a.shell_cap,
                                a.inj_census_nonstrict, a.inj_skip_full,
                                &balls, &st, a.threads, a.inj_par_drop_census);
    if (rc == 3 && any_inject) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    if (rc) return rc;
  }
  const auto t2 = std::chrono::steady_clock::now();

  // 3. Expansion + fold par K, en PointId externes (frontiere d'identite).
  std::vector<PointId> pid_of((size_t)ix.unique_count());
  for (size_t u = 0; u < pid_of.size(); ++u)
    pid_of[u] = a.inj_dense_pointid ? (PointId)u  // MUTANT : le rang casté
                                    : ix.point_id((i32)u);
  if (a.preflight || a.max_output_bytes > 0) {
    // PREFLIGHT DE SORTIE (contre-audits Poisson + audit bloquant C829,
    // § actions minimales) : compter la sortie SANS la materialiser —
    // expansion par tranches de boules, evenements liberes aussitot
    // comptes ; compteurs u64 par K (evenements, incidences = q + d,
    // octets projetes au format ForestEvent resident). Aucun ev_k,
    // aucun fold : le preflight dit ce que couterait la materialisation
    // AVANT de la payer.
    const size_t Tpf =
        balls.empty()
            ? 1
            : std::min((size_t)std::max(a.threads, 1), balls.size());
    std::vector<std::array<u64, 11>> lev(Tpf), linc(Tpf);
    for (auto& x : lev) x.fill(0);
    for (auto& x : linc) x.fill(0);
    parallel_ranges(balls.size(), a.threads,
                    [&](size_t bg, size_t en, size_t t) {
                      std::vector<PlateauEvent> pev;
                      for (size_t bi = bg; bi < en; ++bi) {
                        const BallData& b = balls[bi];
                        const BallRat c = ball_center(b.key);
                        pev.clear();
                        expand_plateau(c, ix.upos, b.interior, b.shell,
                                       (size_t)(kmax_eff + 1), &pev);
                        for (const PlateauEvent& pe : pev) {
                          const size_t K =
                              pe.tpart.size() + pe.ipart.size() - 1;
                          if (K < 1 || K > (size_t)kmax_eff) continue;
                          ++lev[t][K];
                          linc[t][K] +=
                              (u64)pe.tpart.size() + pe.ipart.size();
                        }
                      }
                    });
    u64 tot_ev = 0, tot_inc = 0;
    for (int K = 1; K <= (int)kmax_eff; ++K) {
      u64 e = 0, inc = 0;
      for (size_t t = 0; t < Tpf; ++t) {
        e += lev[t][(size_t)K];
        inc += linc[t][(size_t)K];
      }
      tot_ev += e;
      tot_inc += inc;
      if (a.preflight)
        std::printf("preflight K=%d evenements=%llu incidences=%llu "
                    "bytes_forest_events=%llu\n",
                    K, (unsigned long long)e, (unsigned long long)inc,
                    (unsigned long long)(e * sizeof(ForestEvent)));
    }
    if (a.preflight) {
      // NOM HONNETE (reponse d'audit « fold compact » § 3) : ce chemin
      // est un event_expansion_preflight_after_census — il evite ev_k
      // et le fold, mais cands et balls ont ete materialises ; le
      // preflight PAR TUILE DE CLES (aucun vecteur global de BallData)
      // viendra avec le contrat product/max_output_bytes. Bornes par
      // buffer du fold (jamais le seul flux d'evenements) :
      // unique_facets_upper <= incidences suffit avant internement.
      const u64 uf_upper = tot_inc;
      std::printf("preflight total evenements=%llu incidences=%llu "
                  "bytes_forest_events=%llu boules=%zu\n",
                  (unsigned long long)tot_ev, (unsigned long long)tot_inc,
                  (unsigned long long)(tot_ev * sizeof(ForestEvent)),
                  balls.size());
      std::printf(
          "preflight buffers bytes_facet_incidence_records=%llu "
          "bytes_event_to_fid=%llu bytes_unique_facets_upper=%llu "
          "bytes_union_find=%llu bytes_deltas_upper=%llu "
          "bytes_partition_upper=%llu portee=event_expansion_after_census\n",
          (unsigned long long)(tot_inc * (sizeof(FacetKey) + 8)),
          (unsigned long long)(tot_ev * 11 * sizeof(u32)),
          (unsigned long long)(uf_upper * sizeof(FacetKey)),
          (unsigned long long)(uf_upper * sizeof(u32)),
          (unsigned long long)(tot_ev * 64),
          (unsigned long long)(uf_upper * (sizeof(FacetKey) + sizeof(u32))));
      return 0;
    }
    // PLAFOND TRANSACTIONNEL (audit bloquant C829 § 5.3) : le compte
    // precede le remplissage — count -> preflight -> fill — et le refus
    // tombe AVANT toute allocation d'ev_k ; jamais un reserve optimiste
    // sur des milliards d'enregistrements. Le contrat porte sur les
    // octets residents projetes du flux d'evenements.
    const u64 projected = tot_ev * (u64)sizeof(ForestEvent);
    if (projected > a.max_output_bytes) {
      std::fprintf(stderr,
                   "REFUS resource_exhausted : sortie projetee %llu octets "
                   "(%llu evenements) > plafond max_output_bytes=%llu — "
                   "aucune materialisation\n",
                   (unsigned long long)projected, (unsigned long long)tot_ev,
                   (unsigned long long)a.max_output_bytes);
      return 2;
    }
  }
  u64 sev[11] = {};
  ForestResult sres[11];
  std::vector<std::vector<ForestEvent>> sevk;
  {
    const int rc = forests_from_balls(balls, ix.upos, pid_of, kmax_eff, sev,
                                      sres, &sevk, a.threads, a.judge, &st);
    if (rc == 3 && any_inject) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    if (rc) return rc;
  }
  const auto t3 = std::chrono::steady_clock::now();

  // 4. JUGE : meme semantique depuis l'enumeration brute aux predicats de
  // production, census brut point a point (juge la completude WSPD, le
  // census d'arbre et le RLE).
  u64 disagreements = 0;
  if (a.judge) {
    const int m = ix.unique_count();
    // Table independante geometry_index -> id externe : reconstruite depuis
    // les enregistrements d'entree (position -> id), PAS via ix.point_id.
    std::vector<PointId> jpid((size_t)m);
    for (i32 u = 0; u < m; ++u) {
      const P3& q = ix.upos[(size_t)u];
      bool found = false;
      for (const InputPoint& p : inputs)
        if (p.position.x == q.x && p.position.y == q.y && p.position.z == q.z) {
          jpid[(size_t)u] = p.id;
          found = true;
          break;
        }
      if (!found) {
        std::fprintf(stderr, "INVARIANT : position unique sans enregistrement\n");
        return 3;
      }
    }
    std::vector<BallCandidate> jcands;
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j) {
        const P3 &pa = ix.upos[(size_t)i], &pb = ix.upos[(size_t)j];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        jcands.push_back(BallCandidate{q2_ball_key(pa, pb),
                                       promote_q3_level(q2_exact_level(D2)), 2});
      }
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k) {
          const i32 vs[3] = {i, j, k};
          int bu = 0, bv = 1;
          i64 bl2 = -1;
          for (int s0 = 0; s0 < 3; ++s0)
            for (int s1 = s0 + 1; s1 < 3; ++s1) {
              const i64 l2 = p3_norm2(
                  p3_sub(ix.upos[(size_t)vs[s1]], ix.upos[(size_t)vs[s0]]));
              if (l2 > bl2) bl2 = l2, bu = s0, bv = s1;
            }
          const i32 ia = vs[bu], ib = vs[bv];
          i32 ixx = -1;
          for (const i32 u : vs)
            if (u != ia && u != ib) ixx = u;
          const P3 &pa = ix.upos[(size_t)ia], &pb = ix.upos[(size_t)ib],
                   &px = ix.upos[(size_t)ixx];
          const P3 vv{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y,
                      2 * px.z - pa.z - pb.z};
          if (p3_norm2(vv) <= bl2) continue;
          const Q3Form f3 = q3_form(pa, pb, px);
          jcands.push_back(BallCandidate{
              q3_ball_key(f3), promote_q3_level(q3_exact_level(pa, pb, px)), 3});
        }
    const auto compact = [&]() {
      std::stable_sort(jcands.begin(), jcands.end(), ball_candidate_less);
      jcands.erase(std::unique(jcands.begin(), jcands.end(),
                               [](const BallCandidate& x, const BallCandidate& y) {
                                 return x.key == y.key;
                               }),
                   jcands.end());
    };
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k)
          for (i32 l = k + 1; l < m; ++l) {
            if (jcands.size() > 2000000) compact();
            const i32 vs[4] = {i, j, k, l};
            const Q4Form f4 = q4_form(ix.upos[(size_t)vs[0]], ix.upos[(size_t)vs[1]],
                                      ix.upos[(size_t)vs[2]], ix.upos[(size_t)vs[3]]);
            if (f4.det == 0) continue;
            if (!q4_center_strictly_inside(f4, ix.upos[(size_t)vs[0]],
                                           ix.upos[(size_t)vs[1]],
                                           ix.upos[(size_t)vs[2]],
                                           ix.upos[(size_t)vs[3]]))
              continue;
            jcands.push_back(BallCandidate{q3_ball_key_reduce(q4_ball_form(f4)),
                                           q4_level_raw(f4), 4});
          }
    std::stable_sort(jcands.begin(), jcands.end(), ball_candidate_less);
    jcands.erase(std::unique(jcands.begin(), jcands.end(),
                             [](const BallCandidate& x, const BallCandidate& y) {
                               return x.key == y.key;
                             }),
                 jcands.end());
    std::vector<BallData> jballs;
    for (const BallCandidate& bc : jcands) {
      BallData b;
      b.key = bc.key;
      b.level = bc.level;
      bool dead = false, over = false;
      for (i32 u = 0; u < m && !dead && !over; ++u) {
        const P3& p = ix.upos[(size_t)u];
        const i128 pw = bc.key.a * p3_norm2(p) + bc.key.b[0] * p.x +
                        bc.key.b[1] * p.y + bc.key.b[2] * p.z + bc.key.c;
        if (pw < 0) {
          b.interior.push_back(u);
          if (b.interior.size() > (size_t)(smax_caps - 2)) dead = true;
        } else if (pw == 0) {
          b.shell.push_back(u);
          if (b.shell.size() > a.shell_cap) over = true;
        }
      }
      if (over) {
        std::fprintf(stderr, "REFUS resource_exhausted (juge)\n");
        return 2;
      }
      if (dead) continue;
      jballs.push_back(std::move(b));
    }
    u64 jev[11] = {};
    ForestResult jres[11];
    std::vector<std::vector<ForestEvent>> jevk;
    {
      const int rc = forests_from_balls(jballs, ix.upos, jpid, kmax_eff, jev,
                                        jres, &jevk);
      if (rc) return rc;
    }
    for (int K = 1; K <= (int)kmax_eff; ++K) {
      const bool same_nodes = [&] {
        std::vector<std::pair<u64, u64>> sn, jn;
        for (const ForestNode& nd : sres[K].nodes) sn.push_back({nd.batch, nd.absorbed});
        for (const ForestNode& nd : jres[K].nodes) jn.push_back({nd.batch, nd.absorbed});
        std::sort(sn.begin(), sn.end());
        std::sort(jn.begin(), jn.end());
        return sn == jn;
      }();
      const bool same_deltas = [&] {
        std::vector<ComponentDelta> sd = sres[K].deltas, jd = jres[K].deltas;
        std::sort(sd.begin(), sd.end());
        std::sort(jd.begin(), jd.end());
        return sd == jd;
      }();
      // RENDU § 9.1 : F_K^render et multiplicites d'incidence, flux WSPD
      // contre brut (le juge paye ses propres evenements).
      const bool same_render = [&] {
        const RenderResult a = build_render(sevk[(size_t)K]);
        const RenderResult b = build_render(jevk[(size_t)K]);
        if (a.facets.size() != b.facets.size() ||
            a.incidences != b.incidences ||
            a.batch_levels.size() != b.batch_levels.size())
          return false;
        for (size_t i = 0; i < a.facets.size(); ++i)
          if (!(a.facets[i].facet == b.facets[i].facet) ||
              a.facets[i].per_batch != b.facets[i].per_batch)
            return false;
        return true;
      }();
      if (sev[K] != jev[K] || sres[K].batches != jres[K].batches ||
          !same_nodes || !same_deltas || !same_render ||
          sres[K].new_attachments != jres[K].new_attachments ||
          sres[K].final_partition != jres[K].final_partition) {
        std::fprintf(stderr, "DESACCORD foret K=%d (flux WSPD contre brut)\n", K);
        ++disagreements;
      }
    }
  }
  const auto t4 = std::chrono::steady_clock::now();

  const auto ms = [](auto d) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(d).count() /
           1000.0;
  };
  u64 events_total = 0, fusions_total = 0, nodes_total = 0;
  for (int K = 1; K <= (int)kmax_eff; ++K) {
    events_total += sev[K];
    fusions_total += sres[K].fusions;
    nodes_total += sres[K].nodes.size();
  }
  std::printf(
      "famille=%s n=%zu s=%lld smax=%llu seed=%lld candidats=%llu/%llu/%llu "
      "gen_tues=%llu/%llu ancres_w4=%llu seeds=%llu groupes=%llu "
      "racines_croisees=%llu groupes_fenetre=%llu groupes_tues_dj=%llu "
      "appels_completion=%llu boules_uniques=%llu mortes_profondeur=%llu "
      "prefiltre_feuilles=%llu "
      "prefiltre_range_add=%llu census_keys=%llu census_int=%llu "
      "census_shell=%llu evenements=%llu fusions=%llu noeuds=%llu "
      "juge=%s desaccords=%s t_gen_ms=%.1f t_tri_ms=%.1f t_prefiltre_ms=%.1f "
      "t_census_ms=%.1f t_fold_ms=%.1f t_juge_ms=%.1f seeds_core=%llu "
      "sites_core=%llu t_core_ms=%.1f t_ab_ms=%.1f t_reduce_ms=%.1f "
      "t_emit_ms=%.1f flottant=%llu/%llu/%llu jung=%llu/%llu/%llu\n",
      cloud_family_name(a.family), pts.size(), (long long)a.s,
      (unsigned long long)smax_eff, a.seed, (unsigned long long)st.candidates[0],
      (unsigned long long)st.candidates[1], (unsigned long long)st.candidates[2],
      (unsigned long long)st.gen_depth_killed[1],
      (unsigned long long)st.gen_depth_killed[2],
      (unsigned long long)st.anchors_killed_w4,
      (unsigned long long)st.axial_seeds,
      (unsigned long long)st.axial_groups_emitted,
      (unsigned long long)st.axial_roots_pruned_cross,
      (unsigned long long)st.axial_groups_in_window,
      (unsigned long long)st.axial_groups_killed_depth,
      (unsigned long long)st.axial_completion_calls,
      (unsigned long long)st.unique_balls,
      (unsigned long long)st.balls_dead_depth,
      (unsigned long long)st.prefilter_leaf_tests,
      (unsigned long long)st.prefilter_range_add_mass,
      (unsigned long long)st.full_census_keys,
      (unsigned long long)st.census_interior, (unsigned long long)st.census_shell,
      (unsigned long long)events_total, (unsigned long long)fusions_total,
      (unsigned long long)nodes_total, a.judge ? "on" : "off",
      // Format machine (audit « campagne transactionnelle » § 4) : hors
      // regime juge, desaccords=NA — jamais un zero qu'un parseur de
      // campagne prendrait pour un accord verifie.
      a.judge ? std::to_string(disagreements).c_str() : "NA",
      ms(t0b - t0), ms(t1 - t0b), ms(t1b - t1), ms(t2 - t1b), ms(t3 - t2),
      ms(t4 - t3), (unsigned long long)st.seeds_killed_seed_core,
      (unsigned long long)st.seed_core_sites, st.t_seed_core_ms,
      st.t_axial_ab_ms, st.t_reduce_ms, st.t_emit_ms,
      (unsigned long long)st.float_cert_neg,
      (unsigned long long)st.float_cert_pos,
      (unsigned long long)st.float_fallback,
      (unsigned long long)st.jung_cert_kill,
      (unsigned long long)st.jung_cert_skip,
      (unsigned long long)st.jung_fallback);
  // Profil grossier des lanes (cumule CPU en multi-fils) — decompose
  // t_gen pour designer le POSTE dominant avant toute optimisation.
  std::printf(
      "profil_gen t_hist_ms=%.1f t_rect_cover_ms=%.1f t_anchor_cover_ms=%.1f "
      "t_q3_scan_ms=%.1f t_core_ms=%.1f\n",
      st.t_hist_ms, st.t_rect_cover_ms, st.t_anchor_cover_ms, st.t_q3_scan_ms,
      st.t_seed_core_ms);
  // DESCENTE WSPD PAR LANE : sequentielle, et jusqu'ici hors de tout
  // chrono — sans elle la somme des postes ne rendait pas compte de
  // `t_gen`, et le « poste dominant » designe par les recus precedents
  // etait une conclusion tiree d'un profil incomplet.
  std::printf("profil_wspd t_q2_ms=%.1f t_q3_ms=%.1f t_q4_ms=%.1f "
              "rects_q2=%llu rects_q3=%llu rects_q4=%llu "
              "ouvriers_q2=%llu ouvriers_q3=%llu ouvriers_q4=%llu\n",
              st.t_wspd_alive_ms[0], st.t_wspd_alive_ms[1],
              st.t_wspd_alive_ms[2],
              (unsigned long long)st.wspd_rects_visited[0],
              (unsigned long long)st.wspd_rects_visited[1],
              (unsigned long long)st.wspd_rects_visited[2],
              (unsigned long long)st.wspd_workers[0],
              (unsigned long long)st.wspd_workers[1],
              (unsigned long long)st.wspd_workers[2]);
  // ENTONNOIR DE LA COMPLETION q4 : ou meurent les paires (seed, y).
  std::printf("q4_entonnoir paires=%llu self=%llu lentille=%llu owner=%llu "
              "exact_once=%llu det=%llu centre=%llu atteint_profondeur=%llu\n",
              (unsigned long long)st.q4_pairs,
              (unsigned long long)st.q4_rej_self,
              (unsigned long long)st.q4_rej_lens,
              (unsigned long long)st.q4_rej_owner,
              (unsigned long long)st.q4_rej_once,
              (unsigned long long)st.q4_rej_det,
              (unsigned long long)st.q4_rej_center,
              (unsigned long long)st.q4_reach_depth);
  std::printf("q4_puissance_equatoriale testees=%llu rejets=%llu "
              "part_des_rejets_centre=%.1f%% faux_rejets=%llu\n",
              (unsigned long long)st.q4_eq_tested,
              (unsigned long long)st.q4_eq_reject,
              st.q4_rej_center ? 100.0 * (double)st.q4_eq_reject /
                                     (double)st.q4_rej_center
                               : 0.0,
              (unsigned long long)st.q4_eq_false_reject);
  std::printf("profil_q4 t_completion_ms=%.1f t_depth_ms=%.1f sites_depth=%llu\n",
              st.t_q4_completion_ms, st.t_q4_depth_ms,
              (unsigned long long)st.q4_depth_sites);
  // Metadonnees d'execution (audits 9223888 § 2.1, 66886c0 § 2,
  // 7d921ff, c9c3a48) : workers MESURES au point de creation des
  // std::thread (generation PAR LANE — gen_workers_max n'est qu'un
  // resume), et affinite CPU EFFECTIVE publiee par le processus mesure.
  std::string aff_mask;
  const int aff = effective_affinity(&aff_mask);
  std::printf("execution threads_requested=%d gen_workers_q2=%llu "
              "gen_workers_q3=%llu gen_workers_q4=%llu "
              "gen_workers_max=%llu prefilter_workers=%llu "
              "census_workers=%llu expansion_workers=%llu "
              "fold_workers_max=%llu affinity_cpus_effective=%d "
              "affinity_mask=%s\n",
              a.threads, (unsigned long long)st.gen_workers[0],
              (unsigned long long)st.gen_workers[1],
              (unsigned long long)st.gen_workers[2],
              (unsigned long long)st.gen_workers_max,
              (unsigned long long)st.prefilter_workers,
              (unsigned long long)st.census_workers,
              (unsigned long long)st.expansion_workers,
              (unsigned long long)st.fold_workers_max, aff,
              aff_mask.empty() ? "?" : aff_mask.c_str());
  // Ordonnancement des folds : plafond declare et reserve maximale
  // effectivement atteinte (ligne SEPAREE — le validateur de campagne
  // ancre `execution` de ^ a $).
  std::printf("fold_ordonnancement budget_octets=%llu reserves_max_octets=%llu\n",
              (unsigned long long)st.fold_budget_bytes,
              (unsigned long long)st.fold_reserved_max);
  if (a.digest) print_canonical_digests(cands, sres, kmax_eff);
  // CHARGE DU SCAN q3 PAR ANCRE : ce qui decide de l'assiette d'un index
  // par ancre. Un index construit une fois par ancre s'amortit sur les
  // seeds ; on publie donc la part CUMULEE du travail portee par les
  // ancres d'au moins S seeds (amortissement possible) et par les ancres
  // les plus lourdes (assiette). Les parts sont des FRACTIONS DU TRAVAIL,
  // jamais des comptes d'ancres : une majorite d'ancres legeres ne dit
  // rien du cout.
  {
    const double wt = (double)st.q3_work_total;
    std::printf("q3_charge ancres_scannees=%llu seeds=%llu travail=%llu "
                "cover_moyen=%.1f seeds_moyen=%.2f travail_moyen=%.1f\n",
                (unsigned long long)st.q3_anchors_scanned,
                (unsigned long long)st.q3_seeds_total,
                (unsigned long long)st.q3_work_total,
                st.q3_anchors_scanned
                    ? (double)st.q3_cover_total / (double)st.q3_anchors_scanned
                    : 0.0,
                st.q3_anchors_scanned
                    ? (double)st.q3_seeds_total / (double)st.q3_anchors_scanned
                    : 0.0,
                st.q3_anchors_scanned
                    ? wt / (double)st.q3_anchors_scanned
                    : 0.0);
    std::printf("q3_charge_par_seeds");
    for (int b = 0; b < 8; ++b) {
      u64 w = 0, na = 0;
      for (int i = b; i < 8; ++i) {
        w += st.q3_work_by_seeds[i];
        na += st.q3_anchors_by_seeds[i];
      }
      std::printf(" s>=%d:travail=%.1f%%,ancres=%llu", 1 << b,
                  wt > 0 ? 100.0 * (double)w / wt : 0.0,
                  (unsigned long long)na);
    }
    std::printf("\n");
    std::printf("q3_charge_par_travail");
    for (int b = 39; b >= 0; --b) {
      if (st.q3_anchor_hist[b] == 0) continue;
      u64 w = 0, na = 0;
      for (int i = b; i < 40; ++i) {
        w += st.q3_work_hist[i];
        na += st.q3_anchor_hist[i];
      }
      std::printf(" w>=2^%d:travail=%.1f%%,ancres=%llu", b,
                  wt > 0 ? 100.0 * (double)w / wt : 0.0,
                  (unsigned long long)na);
    }
    std::printf("\n");
  }
  std::printf("q3_scan_sites_par_cover <256=%llu <1024=%llu <4096=%llu "
              ">=4096=%llu\n",
              (unsigned long long)st.q3_scan_sites_by_cover[0],
              (unsigned long long)st.q3_scan_sites_by_cover[1],
              (unsigned long long)st.q3_scan_sites_by_cover[2],
              (unsigned long long)st.q3_scan_sites_by_cover[3]);
  // TROIS CARDINALITES PAR K (contre-audits Poisson, § 6.2) : evenements
  // generes / facettes nees uniques (sommets du K-graphe vus) / deltas
  // critiques emis. Le rapport deltas/evenements mesure le gain encore
  // possible (RNG-HGP, Boruvka) ; facettes/evenements la part
  // incompressible d'une sortie au niveau des facettes.
  for (int K = 1; K <= (int)kmax_eff; ++K)
    std::printf("cardinalites K=%d evenements=%llu facettes=%llu "
                "deltas=%llu attachements=%llu fusions=%llu\n",
                K, (unsigned long long)sev[K],
                (unsigned long long)sres[K].facets,
                (unsigned long long)sres[K].deltas.size(),
                (unsigned long long)sres[K].new_attachments,
                (unsigned long long)sres[K].fusions);
  // Chronos GROSSIERS du fold, sommes sur les K (temps CPU cumule a
  // N fils — audit « fold compact » § 1.4).
  {
    double tb = 0, ti = 0, trd = 0, tp = 0, tis = 0, tit = 0, tir = 0;
    for (int K = 1; K <= (int)kmax_eff; ++K) {
      tb += sres[K].t_batching_ms;
      ti += sres[K].t_intern_ms;
      trd += sres[K].t_reduce_ms;
      tp += sres[K].t_partition_ms;
      tis += sres[K].t_intern_scan_ms;
      tit += sres[K].t_intern_sort_ms;
      tir += sres[K].t_intern_remap_ms;
    }
    std::printf("fold_phases t_batching_ms=%.1f t_intern_ms=%.1f "
                "t_reduce_ms=%.1f t_partition_ms=%.1f\n",
                tb, ti, trd, tp);
    std::printf("fold_intern t_scan_ms=%.1f t_sort_ms=%.1f t_remap_ms=%.1f\n",
                tis, tit, tir);
  }

  if (any_inject) {
    if (a.judge && disagreements > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (a.judge && disagreements > 0) return 1;
  if (st.unique_balls < a.min_balls || fusions_total < a.min_fusions) {
    std::fprintf(stderr, "PLANCHER : boules=%llu fusions=%llu\n",
                 (unsigned long long)st.unique_balls,
                 (unsigned long long)fusions_total);
    return 3;
  }
  return 0;
}
