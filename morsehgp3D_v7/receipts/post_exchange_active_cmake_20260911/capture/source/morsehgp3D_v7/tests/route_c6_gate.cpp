// MorseHGP3D v6 — PORTE C6a SOUS STUB (porte JUMELLE de mhgp7_pilot_stub, qui
// reste intacte et verte sur la route C5).
//
// CE QU'ELLE PROUVE, a trois bras sur le MEME pin :
//   1. l'objet de la route C6 (anneau de lots + encodeur pur a offsets fixes)
//      est EXACTEMENT celui de la route C5 et celui de la route CPU de
//      production — survivants et profondeurs, `BallData` champ par champ
//      (cle, niveau, arite, listes interieur/coquille DANS L'ORDRE), stats de
//      census, puis, de bout en bout, digest_all, digest_balls,
//      digest_postprefilter, les dix digests de foret, cartes et totaux ;
//   2. le LOTISSEMENT ne change rien : le meme objet sort pour un lot unique,
//      pour des lots forcant TROIS lots ou plus (rotation de l'anneau) et pour
//      une QUEUE (dernier lot incomplet), avec ou sans temoin d'anneau, et
//      avec un anneau degenere a un seul emplacement de sortie (contre-pression
//      reelle, `blocked_out > 0`) ;
//   3. `cand_idx = base + gid` et l'ordre global tiennent D'UN LOT A L'AUTRE :
//      les survivants publies portent l'index GLOBAL du candidat, et le temoin
//      de l'anneau est exactement 0..nb_total-1 dans cet ordre ;
//   4. un REFUS AU MILIEU (corruption d'un enregistrement descendu du lot 1,
//      alors que le lot 0 est deja reconstruit dans les temporaires) ne publie
//      RIEN : sorties et statistiques canarisees identiques a l'octet.
//
// CE QU'ELLE NE PROUVE PAS : rien du materiel. Sous `tests/cuda_stub.hpp`
// (sequentiel, INTACT) il n'y a ni flux CUDA, ni evenement, ni fil, ni memoire
// epinglee, ni course, ni temps — aucun chronometre n'est pris ici et aucun
// gain n'est revendique. Le recouvrement hote/device vise par C6 n'est pas
// exerce : seule la DISCIPLINE qui l'autorisera l'est (trois tickets logiques
// coexistants sur trois ressources a baux separes).
//
// MUTANTS (chacun tue code 4) : `gpu-lot-base-reset` (point d'injection dans
// src/gpu/route_c6.hpp — la base du census redevient 0, le chainage inter-lots
// est rompu), `c6-wrong-epoch`, `c6-reuse-before-lease`, `c6-publish-prefix`
// (points d'injection dans src/gpu/lot_ring.hpp), et les mutants de noyaux
// deja au registre. La REFERENCE INTERNE d'une scene mutante est le run a LOT
// UNIQUE (aucune rotation possible) : le lotissement ne doit rien changer.
//
// Codes : 0 conforme ; 1 desaccord ou mutant non tue ; 2 refus ; 3 plancher ;
// 4 mutant tue.
#define MHGP7_FAKE_DEVICE 1
#include "cuda_stub.hpp"

#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/gpu/pilot.hpp"
#include "../src/gpu/route_c6.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp7;

namespace {

int failures = 0;
u64 scenes = 0;

void expect(bool ok, const std::string& what) {
  if (!ok) {
    std::printf("ECHEC : %s\n", what.c_str());
    ++failures;
  } else {
    std::printf("ok : %s\n", what.c_str());
  }
}

// Sortie COMPLETE d'une route de prefiltre+census : c'est cet objet, et non
// un digest, qui est confronte champ par champ entre les bras.
struct RouteOut {
  std::string err;
  std::vector<Survivor> surv;
  std::vector<BallData> balls;
  u64 dead = 0, survivors = 0, interior = 0, shell = 0;
};

bool ball_equal(const BallData& x, const BallData& y) {
  if (x.key != y.key || x.level != y.level || x.arity != y.arity) return false;
  if (x.n_interior != y.n_interior || x.n_shell != y.n_shell) return false;
  for (u8 j = 0; j < x.n_interior; ++j)
    if (x.interior_ids[j] != y.interior_ids[j]) return false;
  for (u8 j = 0; j < x.n_shell; ++j)
    if (x.shell_ids[j] != y.shell_ids[j]) return false;
  return true;
}

bool same_out(const RouteOut& a, const RouteOut& b) {
  if (a.err != b.err) return false;
  if (a.surv.size() != b.surv.size() || a.balls.size() != b.balls.size()) return false;
  for (size_t i = 0; i < a.surv.size(); ++i)
    if (a.surv[i].idx != b.surv[i].idx || a.surv[i].depth != b.surv[i].depth) return false;
  for (size_t i = 0; i < a.balls.size(); ++i)
    if (!ball_equal(a.balls[i], b.balls[i])) return false;
  return a.dead == b.dead && a.survivors == b.survivors && a.interior == b.interior && a.shell == b.shell;
}

// Nuage + candidats uniques d'une scene (memes options que la route CPU).
struct Scene {
  std::vector<InputPoint> in;
  CloudIndex ix;
  std::vector<BallCandidate> cands;
};

Scene make_scene(CloudFamily fam, int n, int threads) {
  Scene s;
  s.in = make_family_input(fam, n, cloud_family_default_coord(fam, n), 3);
  s.ix = build_cloud_index(s.in);
  GenerateOptions go;
  go.s = 8;
  go.smax = 11;
  go.threads = threads;
  GenerateStats gs;
  generate_candidates(s.ix, go, &s.cands, &gs);
  sort_candidates(&s.cands, threads);
  deduplicate_candidates(&s.cands);
  return s;
}

RouteOut run_c5(const Scene& s, u32 mut) {
  RouteOut r;
  ExpandStats st{};
  r.err = gpu::stub_prefilter_census_route(s.ix, s.cands, 11, 12, &r.surv, &r.balls, &st, mut);
  r.dead = st.dead_depth;
  r.survivors = st.survivors;
  r.interior = st.census_interior;
  r.shell = st.census_shell;
  return r;
}

RouteOut run_c6(const Scene& s, const gpu::RouteC6Options& opt, gpu::RouteC6Counters* c) {
  RouteOut r;
  ExpandStats st{};
  r.err = gpu::route_c6_prefilter_census(s.ix, s.cands, 11, 12, &r.surv, &r.balls, &st, opt, c);
  r.dead = st.dead_depth;
  r.survivors = st.survivors;
  r.interior = st.census_interior;
  r.shell = st.census_shell;
  return r;
}

// Cumul des compteurs de couverture (planchers contre le vert-par-vacuite).
struct Cover {
  u64 max_lots = 0, tails = 0, rotations = 0, max_triples = 0, blocked = 0, balls = 0, runs = 0;
  void add(const gpu::RouteC6Counters& c) {
    if (c.lots > max_lots) max_lots = c.lots;
    tails += c.tails;
    rotations += c.rotations_in + c.rotations_out + c.rotations_device;
    if (c.max_live_tickets > max_triples) max_triples = c.max_live_tickets;
    blocked += c.blocked_in + c.blocked_out + c.blocked_device;
    balls += c.boules_reconstruites;
    ++runs;
  }
};

}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  i64 min_scenes = 23, min_lots = 3, min_rotations = 8, min_tails = 6, min_triples = 3, min_blocked = 1,
      min_balls = 100000;
  bool ok_args = true;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    const auto val = [&](const char* p) -> const char* {
      return a.rfind(p, 0) == 0 ? a.c_str() + std::strlen(p) : nullptr;
    };
    if (const char* s = val("--inject=")) {
      inject = s;
      if (inject.empty()) return 2;  // REFUS : une injection vide ne prouve rien
    } else if (const char* s = val("--min-scenes=")) ok_args = parse_i64_exact(s, &min_scenes) && ok_args;
    else if (const char* s = val("--min-lots=")) ok_args = parse_i64_exact(s, &min_lots) && ok_args;
    else if (const char* s = val("--min-rotations=")) ok_args = parse_i64_exact(s, &min_rotations) && ok_args;
    else if (const char* s = val("--min-tails=")) ok_args = parse_i64_exact(s, &min_tails) && ok_args;
    else if (const char* s = val("--min-triples=")) ok_args = parse_i64_exact(s, &min_triples) && ok_args;
    else if (const char* s = val("--min-blocages=")) ok_args = parse_i64_exact(s, &min_blocked) && ok_args;
    else if (const char* s = val("--min-boules=")) ok_args = parse_i64_exact(s, &min_balls) && ok_args;
    else {
      std::fprintf(stderr, "REFUS : argument inconnu %s\n", a.c_str());
      return 2;
    }
  }
  if (!ok_args || min_scenes < 0 || min_lots < 0 || min_rotations < 0 || min_tails < 0 || min_triples < 0 ||
      min_blocked < 0 || min_balls < 0) {
    std::fprintf(stderr, "REFUS : parametre hors domaine ou mal forme\n");
    return 2;
  }
  const char* expected_refusal = nullptr;
  if (!inject.empty()) {
    if (inject == "gpu-lot-base-reset")
      expected_refusal = "invariant : cand_idx inattendu au retour device";
    else if (inject == "c6-wrong-epoch")
      expected_refusal = "valeur inattendue : base globale ou bail rompu";
    else if (inject == "c6-reuse-before-lease" || inject == "gpu-skip-ball-write")
      expected_refusal = "invariant : ecriture device omise (sentinelle survivante)";
    else if (inject == "gpu-stack-shallow")
      expected_refusal = "invariant : pile DFS au-dela du profil (49) sur la route device";
    else if (inject == "gpu-census-nonstrict")
      expected_refusal = "invariant : census contredit la passe count-only (route device)";
    else if (inject == "gpu-nshell-overdomain")
      expected_refusal = "invariant : comptes hors profil au retour device";
    else if (inject == "gpu-skip-count-write")
      expected_refusal = "invariant : count jamais ecrit (sentinelle survivante)";
    else if (inject != "c6-publish-prefix")
      return 2;
    if (!mutants_enable(inject)) return 2;
  }
  u32 mut = 0;
  if (MHGP7_MUTANT("gpu-stack-shallow")) mut |= gpu::kMutStackShallow;
  if (MHGP7_MUTANT("gpu-census-nonstrict")) mut |= gpu::kMutCensusNonstrict;
  if (MHGP7_MUTANT("gpu-skip-ball-write")) mut |= gpu::kMutSkipBallWrite;
  if (MHGP7_MUTANT("gpu-nshell-overdomain")) mut |= gpu::kMutNshellOverdomain;
  if (MHGP7_MUTANT("gpu-skip-count-write")) mut |= gpu::kMutSkipCountWrite;
  const bool mutant = !inject.empty();

  const int threads = 4;
  Cover cov;

  // ================================================================ MUTANTS
  // REFERENCE INTERNE : le run a LOT UNIQUE (aucune rotation possible, aucun
  // chainage inter-lots). Le lotissement ne doit rien y changer.
  if (mutant) {
    const Scene s = make_scene(CloudFamily::kUniform, 400, threads);
    gpu::RouteC6Options ref_opt;
    ref_opt.lot = 0;
    ref_opt.witness = false;
    gpu::RouteC6Counters rc;
    const RouteOut ref = run_c6(s, ref_opt, &rc);
    if (!ref.err.empty()) {
      std::printf("REFERENCE INVALIDE : la route C6 refuse le lot unique sous %s (%s)\n", inject.c_str(),
                  ref.err.c_str());
      return 1;
    }
    struct Var {
      size_t lot;
      u32 in_slots, out_slots;
      bool witness;
    };
    const Var vars[6] = {{7, 2, 2, true},   {7, 2, 2, false},   {61, 2, 2, true},
                         {997, 2, 2, true}, {997, 2, 1, true}, {997, 1, 1, false}};
    for (const Var& vv : vars) {
      gpu::RouteC6Options o;
      o.lot = vv.lot;
      o.in_slots = vv.in_slots;
      o.out_slots = vv.out_slots;
      o.witness = vv.witness;
      o.mut = mut;
      gpu::RouteC6Counters c;
      const RouteOut got = run_c6(s, o, &c);
      if (!got.err.empty()) {
        if (expected_refusal == nullptr || got.err.rfind(expected_refusal, 0) != 0) {
          std::printf("REFUS HORS SIGNATURE : mutant %s : %s\n", inject.c_str(), got.err.c_str());
          return 1;
        }
        std::printf("mutant %s TUE : refus du lot %zu (in=%u out=%u temoin=%d, lots=%llu) — %s\n",
                    inject.c_str(), vv.lot, vv.in_slots, vv.out_slots, (int)vv.witness,
                    (unsigned long long)c.lots, got.err.c_str());
        return 4;
      }
      if (!same_out(ref, got)) {
        std::printf("mutant %s TUE : le lotissement %zu change l'objet (lots=%llu)\n", inject.c_str(),
                    vv.lot, (unsigned long long)c.lots);
        return 4;
      }
    }
    // Scene de CORRUPTION TARDIVE : le refus du lot 1 ne doit RIEN publier.
    {
      gpu::RouteC6Options o;
      o.lot = 997;
      o.mut = mut;
      o.tamper_on = true;
      o.tamper_lot = 1;
      o.tamper_local = 3;
      gpu::RouteC6Counters c;
      RouteOut got;
      got.surv.push_back(Survivor{123u, 9u});
      got.balls.resize(1);
      got.balls[0].arity = 7;
      ExpandStats st{};
      st.census_interior = 777;
      got.err = gpu::route_c6_prefilter_census(s.ix, s.cands, 11, 12, &got.surv, &got.balls, &st, o, &c);
      const bool intact = got.surv.size() == 1 && got.surv[0].idx == 123u && got.surv[0].depth == 9u &&
                          got.balls.size() == 1 && got.balls[0].arity == 7 && st.census_interior == 777;
      if (!intact) {
        std::printf("mutant %s TUE : un refus au milieu a publie (canaris detruits, lots=%llu)\n",
                    inject.c_str(), (unsigned long long)c.lots);
        return 4;
      }
      if (got.err.empty()) {
        std::printf("mutant %s TUE : la corruption tardive n'est plus refusee\n", inject.c_str());
        return 4;
      }
    }
    std::printf("MUTANT NON TUE (%s)\n", inject.c_str());
    return 1;
  }

  // =============================================== PARTIE 1 : C5 vs C6 DIRECT
  for (const CloudFamily fam : {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    const Scene s = make_scene(fam, 400, threads);
    const RouteOut ref = run_c5(s, 0);
    expect(ref.err.empty(), std::string("route C5 verte : ") + cloud_family_name(fam));
    if (!ref.err.empty()) return 1;
    const u64 nb = (u64)s.cands.size();
    std::printf("scene %s n=400 : nb_total=%llu survivants=%llu\n", cloud_family_name(fam),
                (unsigned long long)nb, (unsigned long long)ref.survivors);
    struct Var {
      size_t lot;
      u32 in_slots, out_slots;
      bool witness;
      const char* quoi;
    };
    const Var vars[7] = {
        {0, 2, 2, true, "lot unique"},
        {7, 2, 2, true, "lots de 7 (rotation, queue)"},
        {997, 2, 2, true, "lots de 997"},
        {997, 2, 2, false, "lots de 997 sans temoin"},
        {997, 2, 1, true, "une seule sortie (contre-pression)"},
        {997, 1, 1, true, "anneau degenere in=out=1"},
        {(size_t)nb - 1, 2, 2, true, "queue d'UNE boule"},
    };
    for (const Var& vv : vars) {
      gpu::RouteC6Options o;
      o.lot = vv.lot;
      o.in_slots = vv.in_slots;
      o.out_slots = vv.out_slots;
      o.witness = vv.witness;
      gpu::RouteC6Counters c;
      const RouteOut got = run_c6(s, o, &c);
      char what[256];
      std::snprintf(what, sizeof what,
                    "%s / %s : objet C6 == objet C5 (lots=%llu queues=%llu rot=%llu/%llu/%llu "
                    "triples=%llu blocages=%llu)",
                    cloud_family_name(fam), vv.quoi, (unsigned long long)c.lots,
                    (unsigned long long)c.tails, (unsigned long long)c.rotations_in,
                    (unsigned long long)c.rotations_device, (unsigned long long)c.rotations_out,
                    (unsigned long long)c.max_live_tickets, (unsigned long long)c.blocked_out);
      expect(got.err.empty() && same_out(ref, got), what);
      cov.add(c);
      ++scenes;
      // CHAINAGE EXPLICITE, sur la scene a plusieurs lots : les survivants
      // portent l'index GLOBAL du candidat, strictement croissant et dans le
      // domaine — ce que `cand_idx = base + gid` garantit d'un lot a l'autre.
      if (vv.lot == 997 && vv.witness && vv.out_slots == 2) {
        bool croissant = got.err.empty() && !got.surv.empty();
        for (size_t i = 1; i < got.surv.size() && croissant; ++i)
          croissant = got.surv[i - 1].idx < got.surv[i].idx;
        for (size_t i = 0; i < got.surv.size() && croissant; ++i)
          croissant = (u64)got.surv[i].idx < nb;
        expect(croissant, std::string("index globaux strictement croissants et dans le domaine : ") +
                              cloud_family_name(fam));
        ++scenes;
      }
    }
  }

  // ================================== PARTIE 2 : TROIS BRAS DE BOUT EN BOUT
  for (const CloudFamily fam : {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    for (const int n : {400, 2000}) {
      const std::vector<InputPoint> in = make_family_input(fam, n, cloud_family_default_coord(fam, n), 3);
      RunOptions cpu;
      cpu.s = 8;
      cpu.smax = 11;
      cpu.threads = threads;
      cpu.digest = true;
      const RunResult a = run_pipeline(in, cpu);
      if (a.status != PipelineStatus::kCompleteRegular) return 2;
      RunOptions o5 = cpu;
      o5.prefilter_census_override = [&](const CloudIndex& ix, const std::vector<BallCandidate>& cands,
                                         u64 smax_eff, size_t shell_cap, std::vector<Survivor>* sv,
                                         std::vector<BallData>* bd, ExpandStats* st) {
        return gpu::stub_prefilter_census_route(ix, cands, smax_eff, shell_cap, sv, bd, st, 0);
      };
      const RunResult b = run_pipeline(in, o5);
      // n = 400 : lot unique ET lots de 997 ; n = 2000 : lots de 997 seulement
      // (le lot unique y refait exactement le bras C5, deja compare).
      std::vector<size_t> lots_a_juger;
      if (n == 400) lots_a_juger.push_back((size_t)0);
      lots_a_juger.push_back((size_t)997);
      for (const size_t lot : lots_a_juger) {
        gpu::RouteC6Counters c;
        RunOptions o6 = cpu;
        o6.prefilter_census_override = [&](const CloudIndex& ix, const std::vector<BallCandidate>& cands,
                                           u64 smax_eff, size_t shell_cap, std::vector<Survivor>* sv,
                                           std::vector<BallData>* bd, ExpandStats* st) {
          gpu::RouteC6Options oo;
          oo.lot = lot;
          return gpu::route_c6_prefilter_census(ix, cands, smax_eff, shell_cap, sv, bd, st, oo, &c);
        };
        const RunResult d = run_pipeline(in, o6);
        bool same = d.status == PipelineStatus::kCompleteRegular &&
                    b.status == PipelineStatus::kCompleteRegular;
        same = same && a.digest_all == d.digest_all && a.digest_balls == d.digest_balls &&
               a.digest_postprefilter == d.digest_postprefilter && a.digest_forest == d.digest_forest;
        same = same && b.digest_all == d.digest_all && b.digest_postprefilter == d.digest_postprefilter;
        same = same && a.cards == d.cards && a.total_events == d.total_events &&
               a.total_facets == d.total_facets && a.total_fusions == d.total_fusions &&
               a.total_deltas == d.total_deltas && a.total_nodes == d.total_nodes;
        same = same && a.emitted == d.emitted && a.expand.survivors == d.expand.survivors;
        char what[192];
        std::snprintf(what, sizeof what,
                      "trois bras CPU/C5/C6 : objet identique %s n=%d lot=%zu (lots=%llu)",
                      cloud_family_name(fam), n, lot, (unsigned long long)c.lots);
        expect(same, what);
        cov.add(c);
        ++scenes;
      }
    }
  }

  // ============================================ PARTIE 3 : CAS VIDE et BORDS
  {
    const Scene s = make_scene(CloudFamily::kUniform, 400, threads);
    gpu::RouteC6Options o;
    gpu::RouteC6Counters c;
    std::vector<Survivor> sv{Survivor{5u, 5u}};
    std::vector<BallData> bd(1);
    ExpandStats st{};
    const std::vector<BallCandidate> none;
    const std::string e = gpu::route_c6_prefilter_census(s.ix, none, 11, 12, &sv, &bd, &st, o, &c);
    expect(e.empty() && sv.empty() && bd.empty(), "cas vide : la route C6 publie des sorties VIDES");
    ++scenes;
  }

  // ================================= PARTIE 4 : REFUS AU MILIEU, RIEN PUBLIE
  {
    const Scene s = make_scene(CloudFamily::kUniform, 400, threads);
    gpu::RouteC6Options o;
    o.lot = 997;
    o.tamper_on = true;
    o.tamper_lot = 1;  // le lot 0 est DEJA reconstruit dans les temporaires
    o.tamper_local = 3;
    gpu::RouteC6Counters c;
    std::vector<Survivor> sv{Survivor{123u, 9u}};
    std::vector<BallData> bd(1);
    bd[0].arity = 7;
    ExpandStats st{};
    st.census_interior = 777;
    const std::string e = gpu::route_c6_prefilter_census(s.ix, s.cands, 11, 12, &sv, &bd, &st, o, &c);
    expect(!e.empty() && e.find("cand_idx inattendu") != std::string::npos,
           std::string("corruption tardive du lot 1 : refus par le validateur (") + e + ")");
    expect(sv.size() == 1 && sv[0].idx == 123u && sv[0].depth == 9u && bd.size() == 1 &&
               bd[0].arity == 7 && st.census_interior == 777,
           "corruption tardive : sorties et stats CANARISEES intactes (aucun prefixe publie)");
    expect(c.lots >= 3 && c.boules_reconstruites > 0,
           "corruption tardive : la scene a bien plusieurs lots et un lot deja reconstruit");
    scenes += 3;
  }

  // ======================================================= PARTIE 5 : REFUS
  {
    const Scene s = make_scene(CloudFamily::kUniform, 400, threads);
    gpu::RouteC6Options o;
    o.lot = 997;
    o.in_slots = 0;  // hors domaine : l'anneau doit REFUSER avant toute allocation
    gpu::RouteC6Counters c;
    std::vector<Survivor> sv;
    std::vector<BallData> bd;
    ExpandStats st{};
    const std::string e = gpu::route_c6_prefilter_census(s.ix, s.cands, 11, 12, &sv, &bd, &st, o, &c);
    expect(!e.empty() && e.rfind("invalid_input", 0) == 0 && sv.empty() && bd.empty(),
           std::string("configuration d'anneau hors domaine : refus avant calcul (") + e + ")");
    ++scenes;
  }

  std::printf("couverture : scenes=%llu runs=%llu lots_max=%llu queues=%llu rotations=%llu triples=%llu "
              "blocages=%llu boules=%llu\n",
              (unsigned long long)scenes, (unsigned long long)cov.runs, (unsigned long long)cov.max_lots,
              (unsigned long long)cov.tails, (unsigned long long)cov.rotations,
              (unsigned long long)cov.max_triples, (unsigned long long)cov.blocked,
              (unsigned long long)cov.balls);
  if (failures) return 1;
  if ((i64)scenes < min_scenes || (i64)cov.max_lots < min_lots || (i64)cov.rotations < min_rotations ||
      (i64)cov.tails < min_tails || (i64)cov.max_triples < min_triples ||
      (i64)cov.blocked < min_blocked || (i64)cov.balls < min_balls) {
    std::printf("PLANCHER : scenes=%llu(>=%lld) lots=%llu(>=%lld) rotations=%llu(>=%lld) "
                "queues=%llu(>=%lld) triples=%llu(>=%lld) blocages=%llu(>=%lld) boules=%llu(>=%lld)\n",
                (unsigned long long)scenes, (long long)min_scenes, (unsigned long long)cov.max_lots,
                (long long)min_lots, (unsigned long long)cov.rotations, (long long)min_rotations,
                (unsigned long long)cov.tails, (long long)min_tails, (unsigned long long)cov.max_triples,
                (long long)min_triples, (unsigned long long)cov.blocked, (long long)min_blocked,
                (unsigned long long)cov.balls, (long long)min_balls);
    return 3;
  }
  return 0;
}
