// MorseHGP3D v6 — PORTE DES DEUX GARDES DURES DU FOLD (palier P5 de
// docs/ECHELLE.md).
//
// `fold_capacity_ok` porte les DEUX PREMIERS refus types que rencontrerait une
// campagne d'echelle : evenements >= (2^32-1)/11 (positions d'incidence u32) et
// incidences > 2^31-1. Ces seuils tirent au-dela du million de points a K=10 :
// AUCUNE porte ne les exercait, alors qu'ils decident le statut d'un run massif.
// Le palier P5 les rend exercables a petit n par deux crochets abaissables sous
// MHGP6_TESTING (meme patron que csr_keys_cap_for_tests) ; le binaire produit
// ne connait aucun crochet.
//
// SCENES (--garde=evenements|incidences) :
//   temoin  : la MEME scene sans abaissement doit etre COMPLETE (anti-vacuite :
//             sans lui, un refus pourrait venir d'autre chose que du plafond) ;
//   abaisse : le plafond est pose JUSTE SOUS la cardinalite mesuree au temoin,
//             le run doit rendre resource_exhausted, le message doit nommer le
//             plafond STRUCTUREL (jamais la valeur abaissee : un reçu de
//             campagne ne doit pas etre confondu avec une scene de test),
//             l'ordre K fautif doit etre nomme, AUCUN callback on_forest ne
//             doit avoir ete appele et AUCUN provisoire ne doit survivre ;
//   le refus doit precedeer l'etage B : zero phase de fold observee.
// Chaque scene est rejouee pour fold_inflight de {1, 2, 8} : l'ordonnancement
// ne change pas un refus decide sur des comptes.
//
// MUTANT `caps-fold-guard-skip` : la garde est sautee. Le run redevient complet
// (a n=400 les vraies structures u32 ne debordent pas), donc le refus ATTENDU
// est ABSENT : code 4.
//
// CODES : 0 conforme ; 1 desaccord ; 2 refus d'argument ; 3 plancher ou contrat
// viole ; 4 mutant tue.
#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp6;

namespace {

bool g_bad = false;
void bad(const std::string& m) {
  std::printf("DESACCORD : %s\n", m.c_str());
  g_bad = true;
}

bool provisoires_vides(const RunResult& rr) {
  return rr.digest_raw_candidates.empty() && rr.digest_balls.empty() && rr.digest_postprefilter.empty() &&
         rr.digest_all.empty() && rr.digest_forest.empty() && rr.cards.empty() && rr.total_events == 0 &&
         rr.total_facets == 0 && rr.total_fusions == 0 && rr.total_deltas == 0 && rr.total_nodes == 0 &&
         rr.forest_storage.empty() && rr.sum_parents_by_k.empty() && rr.sum_parents_total == 0;
}

int refus_argument(const char* why) {
  std::printf("REFUS : %s\n", why);
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  std::string garde;
  std::string inject;
  long long n = 400, threads = 4;
  u64 min_events = 1000, min_incidences = 1000;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    const auto val = [&](const char* p) -> const char* {
      const size_t l = std::strlen(p);
      return a.compare(0, l, p) == 0 ? a.c_str() + l : nullptr;
    };
    i64 v = 0;
    if (const char* s = val("--garde=")) garde = s;
    else if (const char* s = val("--inject=")) inject = s;
    else if (const char* s = val("--n=")) { if (!parse_i64_exact(s, &v) || v < 100) return refus_argument("--n"); n = (long long)v; }
    else if (const char* s = val("--threads=")) { if (!parse_i64_exact(s, &v) || v < 1) return refus_argument("--threads"); threads = (long long)v; }
    else if (const char* s = val("--min-evenements=")) { if (!parse_i64_exact(s, &v) || v < 0) return refus_argument("--min-evenements"); min_events = (u64)v; }
    else if (const char* s = val("--min-incidences=")) { if (!parse_i64_exact(s, &v) || v < 0) return refus_argument("--min-incidences"); min_incidences = (u64)v; }
    else return refus_argument("argument inconnu");
  }
  if (garde != "evenements" && garde != "incidences") return refus_argument("--garde=evenements|incidences attendu");
  if (!inject.empty() && !mutants_enable(inject)) return refus_argument("mutant inconnu du registre");

  const std::vector<InputPoint> in =
      make_family_input(CloudFamily::kUniform, (int)n, cloud_family_default_coord(CloudFamily::kUniform, (int)n), 3);
  if (in.empty()) return refus_argument("famille vide");

  // TEMOIN : sans abaissement, la scene DOIT etre complete, et elle donne les
  // cardinalites reelles sur lesquelles le plafond sera pose.
  RunOptions o0;
  o0.threads = (int)threads;
  o0.s = 8;
  o0.smax = 11;
  std::atomic<u64> cb0{0};
  o0.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++cb0; };
  const RunResult t0 = run_pipeline(in, o0);
  if (t0.status != PipelineStatus::kCompleteRegular) {
    bad("temoin : la scene sans abaissement n'est pas complete (" + t0.message + ")");
    return 1;
  }
  u64 max_events = 0, max_incidences = 0;
  for (u64 K = 1; K < (u64)t0.cards.size() && K <= t0.kmax_eff; ++K) {
    if (t0.cards[K].events > max_events) max_events = t0.cards[K].events;
    // Les incidences ne sont pas publiees par ordre : la somme des supports
    // d'un ordre les majore, on prend la cardinalite d'attachements+fusions
    // comme minorant observable de la charge.
    const u64 inc = t0.cards[K].attachments + t0.cards[K].fusions;
    if (inc > max_incidences) max_incidences = inc;
  }
  std::printf("temoin : complet, callbacks=%llu, max_evenements=%llu, minorant_incidences=%llu\n",
              (unsigned long long)cb0.load(), (unsigned long long)max_events, (unsigned long long)max_incidences);
  if (max_events < min_events) {
    std::printf("PLANCHER : max_evenements=%llu < %llu — scene trop maigre pour exercer la garde\n",
                (unsigned long long)max_events, (unsigned long long)min_events);
    return 3;
  }
  if (max_incidences < min_incidences) {
    std::printf("PLANCHER : minorant_incidences=%llu < %llu\n", (unsigned long long)max_incidences,
                (unsigned long long)min_incidences);
    return 3;
  }

  // ABAISSEMENT : juste sous la cardinalite observee, donc au moins un ordre
  // refuse, et le refus vient du plafond vise et d'aucun autre.
  const char* attendu = nullptr;
  if (garde == "evenements") {
    fold_events_cap_for_tests() = max_events;  // la garde est `>=` : egal suffit
    attendu = "evenements >= (2^32-1)/11";
  } else {
    // Les incidences ne sont PAS publiees par ordre (elles sont comptees par
    // count_events_by_k et consommees sur place) : la porte ne peut pas poser
    // le plafond « juste sous » la valeur reelle. Elle exerce donc la BRANCHE
    // avec un plafond de 1, ce qui garantit le refus des le premier ordre ; le
    // temoin ci-dessus et le plancher de couverture attestent que la scene
    // porte de vraies incidences (leur minorant attachements + fusions).
    fold_incidences_cap_for_tests() = 1;
    attendu = "incidences > 2^31-1";
  }

  u64 refus = 0;
  for (const int infl : {1, 2, 8}) {
    RunOptions o;
    o.threads = (int)threads;
    o.s = 8;
    o.smax = 11;
    o.fold_inflight = infl;
    std::atomic<u64> cb{0}, phases{0};
    o.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++cb; };
    o.on_fold_phase = [&](u64, FoldPhase) { ++phases; };
    const RunResult rr = run_pipeline(in, o);
    const std::string tag = "garde " + garde + " inflight=" + std::to_string(infl);
    if (rr.status != PipelineStatus::kResourceExhausted) {
      std::printf("%s : refus ATTENDU absent (statut %d, message '%s')\n", tag.c_str(), (int)rr.status,
                  rr.message.c_str());
      if (!inject.empty()) {
        std::printf("MUTANT TUE : la garde sautee laisse passer un ordre au-dela du plafond\n");
        return 4;
      }
      bad(tag + " : aucun refus");
      continue;
    }
    ++refus;
    if (status_exit_code(rr.status) != 2) bad(tag + " : code transactionnel != 2");
    if (rr.message.find(attendu) == std::string::npos)
      bad(tag + " : le message ne nomme pas le plafond structurel (" + rr.message + ")");
    if (rr.message.compare(0, 7, "fold K=") != 0) bad(tag + " : l'ordre fautif n'est pas nomme (" + rr.message + ")");
    if (cb.load() != 0) bad(tag + " : callback on_forest appele malgre le refus");
    if (phases.load() != 0) bad(tag + " : phases de fold observees alors que le refus precede l'etage B");
    if (!provisoires_vides(rr)) bad(tag + " : provisoire NON vide apres refus");
    std::printf("%s : %s\n", tag.c_str(), rr.message.c_str());
  }
  if (!inject.empty()) {
    std::printf("MUTANT NON TUE : le refus est intact sous %s\n", inject.c_str());
    return 1;
  }
  if (refus != 3) {
    std::printf("PLANCHER : %llu refus sur 3 ordonnancements\n", (unsigned long long)refus);
    return 3;
  }
  std::printf("fold_caps_gate garde=%s refus=%llu temoin=complet — %s\n", garde.c_str(),
              (unsigned long long)refus, g_bad ? "desaccord" : "conforme");
  return g_bad ? 1 : 0;
}
