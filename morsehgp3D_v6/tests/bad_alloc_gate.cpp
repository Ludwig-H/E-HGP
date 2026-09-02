// MorseHGP3D v6 — ETAGE NOMME D'UN bad_alloc (alerte G4 du 2 septembre 2026,
// audits/ALERTE_G4_ECHELLE_V6_20260902.md).
//
// L'auditeur : « un std::bad_alloc generique n'identifie pas l'etage qui
// deborde : les RSS par etage ne sont imprimes qu'apres succes ». Un refus
// TYPE qui NOMME l'etage vaut une donnee ; un abort (code 134) n'en vaut
// aucune. Cette porte grave le contrat de la conversion.
//
// SCENES
//   temoin (sans --inject) : le MEME nuage et les MEMES options COMPLETENT —
//     anti-vacuite (la scene injectee n'est pas verte parce qu'elle echouerait
//     de toute facon), curseur final `publication`, callbacks 1..kmax_eff.
//   --inject=caps-throw-bad-alloc-provision : panne AVANT le corps du
//     pipeline, a la provision du message de refus elle-meme (retour
//     auditeur : placee hors de la garde, elle rendait un abort) — etage
//     attendu `entree`, aucune phase de fold, aucun callback.
//   --inject=caps-throw-bad-alloc-census : panne d'allocation sur le FIL
//     PRINCIPAL, entre le prefiltre et le census.
//   --inject=caps-throw-bad-alloc-fold : panne dans un WORKER de l'etage B au
//     PREMIER ordre (K=1) — le chemin que l'alerte vise vraiment : le fold est
//     le seul etage a workers, et son exception est relancee par le fil
//     principal HORS du corps du pipeline. Sans enrobage elle terminait le
//     processus.
//
// CONTRAT EXIGE de chaque scene injectee, pour chaque fold_inflight de
// {1, 2, 8} (l'ordonnancement ne change rien au refus) :
//   - statut resource_exhausted, code de sortie transactionnel 2 ;
//   - message prefixe EXACTEMENT par « resource_exhausted : bad_alloc a
//     l'etage <nom> ( » et portant les RSS d'etage ;
//   - curseur `stage_reached` egal a l'etage annonce ;
//   - ZERO callback on_forest — aucun prefixe de payload publie ;
//   - AUCUN provisoire survivant (digests, forets, cartes, totaux, stockage) ;
//   - phases de fold observees pour la scene `fold` (elle a bien atteint B) et
//     AUCUNE pour la scene `census` (le refus precede l'etage B).
//
// CODES : 0 temoin conforme ; 1 desaccord du temoin ; 2 refus transactionnel
// attendu ET conforme (le code que la porte epingle) ; 3 contrat viole ou
// argument refuse — un refus d'ARGUMENT ne rend JAMAIS 2, ce code est reserve
// au refus du pipeline lui-meme.
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp6;

namespace {

int g_bad = 0;

void bad(const std::string& what) {
  ++g_bad;
  std::printf("DESACCORD : %s\n", what.c_str());
}

// Les provisoires EXACTEMENT au sens de invalidate_provisional : rien de tout
// cela ne survit a un refus (gen/expand restent des compteurs de diagnostic,
// comme sur tout autre refus transactionnel).
bool provisoires_vides(const RunResult& rr) {
  return rr.digest_raw_candidates.empty() && rr.digest_balls.empty() && rr.digest_postprefilter.empty() &&
         rr.digest_all.empty() && rr.digest_forest.empty() && rr.cards.empty() && rr.total_events == 0 &&
         rr.total_facets == 0 && rr.total_fusions == 0 && rr.total_deltas == 0 && rr.total_nodes == 0 &&
         rr.forest_storage.empty() && rr.csr_fallback == 0 && rr.forest_storage_conformes == 0;
}

int refus_argument(const char* why) {
  std::printf("REFUS : %s\n", why);
  return 3;
}

}  // namespace

int main(int argc, char** argv) {
  std::string inject, etage;
  long long n = 400, threads = 4;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    i64 v = 0;
    if (const char* s = val("--inject=")) {
      inject = s;
    } else if (const char* s = val("--etage=")) {
      etage = s;
    } else if (const char* s = val("--n=")) {
      if (!parse_i64_exact(s, &v) || v < 2 || v > 100000) return refus_argument("--n hors [2, 100000]");
      n = (long long)v;
    } else if (const char* s = val("--threads=")) {
      if (!parse_i64_exact(s, &v) || v < 1 || v > 1024) return refus_argument("--threads hors [1, 1024]");
      threads = (long long)v;
    } else {
      return refus_argument("argument inconnu");
    }
  }
  const std::vector<InputPoint> in =
      make_family_input(CloudFamily::kUniform, (int)n, cloud_family_default_coord(CloudFamily::kUniform, (int)n), 3);
  if (in.size() < 2) return refus_argument("famille vide");

  if (inject.empty()) {
    // TEMOIN : sans mutant, la MEME scene complete (anti-vacuite).
    RunOptions o;
    o.threads = (int)threads;
    o.digest = true;
    o.diagnostic_raw_candidates_digest = true;
    std::vector<u64> ks;
    std::mutex ks_mutex;
    std::atomic<u64> phases{0};
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult&) {
      std::lock_guard<std::mutex> hold(ks_mutex);
      ks.push_back(K);
    };
    o.on_fold_phase = [&](u64, FoldPhase) { ++phases; };
    const RunResult rr = run_pipeline(in, o);
    if (rr.status != PipelineStatus::kCompleteRegular) bad("temoin : statut non complet : " + rr.message);
    if (rr.stage_reached != kRunStagePublication)
      bad(std::string("temoin : curseur final ") + run_stage_name(rr.stage_reached) + " != publication");
    if ((u64)ks.size() != rr.kmax_eff) bad("temoin : callbacks on_forest != kmax_eff");
    if (phases.load() == 0) bad("temoin : aucune phase de fold observee");
    if (rr.digest_all.empty()) bad("temoin : digest_all vide");
    std::printf("temoin : run complet, etage_atteint=%s, %zu callbacks (kmax_eff=%llu) — %s\n",
                run_stage_name(rr.stage_reached), ks.size(), (unsigned long long)rr.kmax_eff,
                g_bad ? "desaccord" : "conforme");
    return g_bad ? 1 : 0;
  }

  if (inject != "caps-throw-bad-alloc-census" && inject != "caps-throw-bad-alloc-fold" &&
      inject != "caps-throw-bad-alloc-provision")
    return refus_argument("--inject hors des trois mutants de cette porte");
  if (etage != "census" && etage != "fold" && etage != "entree")
    return refus_argument("--etage attendu : entree, census ou fold");
  if (!mutants_enable(inject)) return refus_argument("mutant inconnu du registre (cible hors MHGP6_TESTING ?)");

  // Le texte NE PORTE PAS la sous-chaine `bad_alloc` : classes d'issue
  // mutuellement exclusives du validateur de campagne (code 2 = refus type,
  // code 134 = allocation non capturee).
  const std::string prefixe = "resource_exhausted : allocation impossible a l'etage " + etage + " (";
  u64 refus = 0;
  for (const int infl : {1, 2, 8}) {
    RunOptions o;
    o.threads = (int)threads;
    o.digest = true;
    o.diagnostic_raw_candidates_digest = true;
    o.fold_inflight = infl;
    std::atomic<u64> callbacks{0}, phases{0};
    o.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++callbacks; };
    o.on_fold_phase = [&](u64, FoldPhase) { ++phases; };
    const RunResult rr = run_pipeline(in, o);
    char tag[64];
    std::snprintf(tag, sizeof tag, "%s inflight=%d", inject.c_str(), infl);
    if (rr.status != PipelineStatus::kResourceExhausted) {
      bad(std::string(tag) + " : statut != resource_exhausted (message='" + rr.message + "')");
      continue;
    }
    ++refus;
    if (status_exit_code(rr.status) != 2) bad(std::string(tag) + " : code de sortie transactionnel != 2");
    if (rr.message.compare(0, prefixe.size(), prefixe) != 0)
      bad(std::string(tag) + " : message sans le prefixe exact '" + prefixe + "' : " + rr.message);
    if (rr.message.find("rss_mb apres_generation=") == std::string::npos)
      bad(std::string(tag) + " : message sans les RSS d'etage : " + rr.message);
    if (etage != run_stage_name(rr.stage_reached))
      bad(std::string(tag) + " : curseur " + run_stage_name(rr.stage_reached) + " != " + etage);
    if (callbacks.load() != 0) bad(std::string(tag) + " : callbacks on_forest publies apres bad_alloc");
    if (!provisoires_vides(rr)) bad(std::string(tag) + " : provisoire NON vide apres bad_alloc");
    if (etage == "fold" && phases.load() == 0)
      bad(std::string(tag) + " : aucune phase de fold observee (scene vacue, l'etage B n'a pas ete atteint)");
    if (etage == "census" && phases.load() != 0)
      bad(std::string(tag) + " : phases de fold observees alors que le refus precede l'etage B");
    if (etage == "entree" && phases.load() != 0)
      bad(std::string(tag) + " : phases de fold observees alors que le refus precede tout calcul");
    if (etage == "entree" && rr.rss_mb[0] != 0.0)
      bad(std::string(tag) + " : RSS de generation non nul alors que le corps n'a pas commence");
    std::printf("%s : %s\n", tag, rr.message.c_str());
  }
  if (refus != 3) {
    std::printf("DESACCORD : %llu refus sur 3 inflights — chaque ordonnancement doit refuser\n",
                (unsigned long long)refus);
    return 3;
  }
  if (g_bad) return 3;
  std::printf("bad_alloc a l'etage %s : refus transactionnel conforme (statut, curseur, zero callback, "
              "aucun provisoire) sur inflight 1/2/8 — jamais un abort\n",
              etage.c_str());
  return 2;
}
