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
#include <algorithm>
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
  u32 inj_axial = 0;  // masque kAxial* des mutants du chemin axial
  u64 min_balls = 0;
  u64 min_fusions = 0;
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
template <typename Fn>
void parallel_ranges(size_t n, int threads, Fn&& fn) {
  const size_t T =
      n == 0 ? 1 : std::min((size_t)std::max(threads, 1), n);
  if (T <= 1) {
    fn((size_t)0, n, (size_t)0);
    return;
  }
  std::vector<std::thread> pool;
  for (size_t t = 0; t < T; ++t) {
    const size_t b = n * t / T, e = n * (t + 1) / T;
    pool.emplace_back([&fn, b, e, t] { fn(b, e, t); });
  }
  for (auto& th : pool) th.join();
}

void prefilter_balls(const CloudIndex& ix,
                     const std::vector<BallCandidate>& cands, u64 smax_eff,
                     bool inj_threshold_minus_one, bool inj_range_add_le,
                     std::vector<Survivor>* survivors, BallStreamStats* st,
                     int threads = 1) {
  const size_t T = cands.empty()
                       ? 1
                       : std::min((size_t)std::max(threads, 1), cands.size());
  std::vector<std::vector<Survivor>> lsv(T);
  std::vector<BallStreamStats> lst(T);
  parallel_ranges(cands.size(), threads, [&](size_t b, size_t e, size_t t) {
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
  const size_t T =
      survivors.empty()
          ? 1
          : std::min((size_t)std::max(threads, 1), survivors.size());
  std::vector<std::vector<BallData>> lb(T);
  std::vector<BallStreamStats> lst(T);
  // Un refus/invariant arrete la tranche ; le verdict fusionne est celui
  // de la tranche d'indices la plus BASSE (deterministe — la meme boule
  // qu'aurait vue le sequentiel), message imprime une seule fois.
  std::vector<int> lrc(T, 0);
  parallel_ranges(survivors.size(), threads, [&](size_t b, size_t e,
                                                 size_t t) {
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
                       int threads = 1) {
  // PHASE A — expansion des plateaux, par TRANCHES de boules : chaque
  // ouvrier remplit ses propres listes par K, la fusion en ordre de
  // tranche restitue EXACTEMENT l'ordre sequentiel des evenements.
  const size_t T =
      balls.empty() ? 1 : std::min((size_t)std::max(threads, 1), balls.size());
  std::vector<std::vector<std::vector<ForestEvent>>> lev(
      T, std::vector<std::vector<ForestEvent>>(11));
  parallel_ranges(balls.size(), threads, [&](size_t bg, size_t en, size_t t) {
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
  std::vector<std::vector<ForestEvent>> ev_k(11);
  for (size_t t = 0; t < T; ++t)
    for (size_t K = 1; K <= (size_t)kmax_eff; ++K)
      ev_k[K].insert(ev_k[K].end(), lev[t][K].begin(), lev[t][K].end());
  // PHASE B — folds INDEPENDANTS par K (chaque K possede son build_forest
  // et son resultat ; le rapport de violation reste au plus petit K).
  parallel_ranges((size_t)kmax_eff, threads, [&](size_t bg, size_t en,
                                                 size_t) {
    for (size_t k0 = bg; k0 < en; ++k0) {
      const int K = (int)k0 + 1;
      per_k_events[K] = ev_k[(size_t)K].size();
      per_k_result[K] = build_forest(ev_k[(size_t)K]);
    }
  });
  for (int K = 1; K <= (int)kmax_eff; ++K)
    if (per_k_result[K].attach_violations != 0 ||
        per_k_result[K].birth_violations != 0) {
      std::fprintf(stderr, "INVARIANT : violations de roles (K=%d)\n", K);
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
                          a.threads, a.inj_par_drop);
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
  u64 sev[11] = {};
  ForestResult sres[11];
  std::vector<std::vector<ForestEvent>> sevk;
  {
    const int rc = forests_from_balls(balls, ix.upos, pid_of, kmax_eff, sev,
                                      sres, &sevk, a.threads);
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
      "t_emit_ms=%.1f\n",
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
      st.t_axial_ab_ms, st.t_reduce_ms, st.t_emit_ms);

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
