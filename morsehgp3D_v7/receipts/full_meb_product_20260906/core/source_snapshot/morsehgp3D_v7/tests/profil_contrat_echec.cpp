// MorseHGP3D v6 — PORTE COMPILEE du contrat d'echec sous PROFIL (§ 5.10,
// contre-lectures 7ec81064 puis 71528f8a) : le CLI n'appelle JAMAIS
// print_run apres un refus — une porte qui inspecte stdout resterait verte
// meme si `fold_profiles` fuyait. Celle-ci inspecte le `RunResult`
// DIRECTEMENT : au refus transactionnel (budget partiel minuscule), les
// records de profil sont VIDES et le pic de reduction nul (le canal
// provisoire n'est pas rouvert par l'instrumentation) ; au succes, il y a
// EXACTEMENT un record par K, aux fenetres finies et couvrantes.
// SCENE DE PANNE NON VACUEUSE (contre-lecture ac6b4bc1 : un budget de 4 Kio
// refuse AVANT fold_profiles.assign — le vecteur vide ne prouverait rien) :
// panne de l'etage A injectee a K=2 (--inject=fold-inject-a-failure-k2,
// ACTIVE avant tout premier run — cache statique des sites mutants), le
// callback K=1 doit avoir EU LIEU (le vecteur a ete assigne et le record K1
// rempli en vol), puis au retour terminal profils et pic sont EFFACES.
// Compilee UNIQUEMENT avec MHGP7_PROFILE_REDUCE (cible dediee, l'identite
// de build est signee par la cible). Codes : 0 conforme (nominal) ;
// 1 desaccord ; 2 refus ; 3 plancher ; 4 = scene injectee conforme (la
// panne observee ET le canal du profil efface — meme convention que
// mhgp7_contrat_echec_fold_k2).
#include <cmath>
#include <cstdio>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/core/mutants.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp7;

namespace {
int failures = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    std::printf("ECHEC : %s\n", what);
    ++failures;
  } else {
    std::printf("ok : %s\n", what);
  }
}
}  // namespace

int main(int argc, char** argv) {
#ifndef MHGP7_PROFILE_REDUCE
  (void)argc;
  (void)argv;
  std::fprintf(stderr, "REFUS : cette porte se compile avec MHGP7_PROFILE_REDUCE (cible mhgp7_profil_contrat)\n");
  return 2;
#else
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  // ACTIVATION AVANT TOUT RUN (cache statique des sites mutants).
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool m_k2 = MHGP7_MUTANT("fold-inject-a-failure-k2");
  const std::vector<InputPoint> in =
      make_family_input(CloudFamily::kUniform, 400, cloud_family_default_coord(CloudFamily::kUniform, 400), 3);
  RunOptions o;
  o.s = 8;
  o.smax = 11;
  o.threads = 4;
  o.digest = true;

  if (m_k2) {
    // Scene NON VACUEUSE : K=1 publie, panne A a K=2, retour terminal avec
    // profils et pic EFFACES. Durcissement 01bd14a9 : la scene tourne sous
    // fold_join_before_next_k=true et le callback verifie que le
    // ForestResult::profile K1 est STRICTEMENT NON VIDE — la jonction
    // garantit que sa copie vers RunResult::fold_profiles precede A(K=2) ;
    // le recit ne depasse plus la preuve.
    o.fold_join_before_next_k = true;
    u64 k1_callbacks = 0;
    bool k1_profile_filled = false;
    o.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult& fr) {
      if (K == 1) {
        ++k1_callbacks;
        const double mur = std::chrono::duration<double, std::milli>(fr.profile.end - fr.profile.begin).count();
        k1_profile_filled = mur > 0 && fr.profile.somme() > 0;
      }
    };
    const RunResult r = run_pipeline(in, o);
    const bool failed_as_expected = r.status == PipelineStatus::kInvariantViolated;
    const bool k1_seen = k1_callbacks == 1 && k1_profile_filled;
    const bool erased = r.fold_profiles.empty() && r.peak_reduce_active == 0;
    const bool provisional_clean = r.digest_all.empty() && r.digest_forest.empty() && r.cards.empty();
    if (failed_as_expected && k1_seen && erased && provisional_clean) {
      std::printf("scene injectee conforme : K1 publie (profil K1 non vide, jonction active) puis panne A K=2, profils et pic EFFACES au terminal\n");
      return 4;
    }
    std::printf("DESACCORD : panne=%d k1=%d efface=%d provisoires=%d\n", failed_as_expected ? 1 : 0,
                k1_seen ? 1 : 0, erased ? 1 : 0, provisional_clean ? 1 : 0);
    return 1;
  }

  // (1) SUCCES : un record par K, fenetres finies non negatives, couvrantes.
  {
    const RunResult r = run_pipeline(in, o);
    if (r.status != PipelineStatus::kCompleteRegular) {
      std::fprintf(stderr, "REFUS : temoin non complet\n");
      return 2;
    }
    if (r.kmax_eff == 0) {
      std::printf("PLANCHER : kmax_eff nul\n");
      return 3;
    }
    expect(r.fold_profiles.size() == (size_t)r.kmax_eff + 1, "succes : un record par K (taille kmax_eff + 1)");
    for (u64 K = 1; K < (u64)r.fold_profiles.size(); ++K) {
      const ReduceProfile& pf = r.fold_profiles[K];
      const double mur = std::chrono::duration<double, std::milli>(pf.end - pf.begin).count();
      const double somme = pf.somme();
      if (!(std::isfinite(somme) && somme >= 0 && std::isfinite(mur) && mur >= 0)) {
        expect(false, "succes : fenetres finies et non negatives");
        break;
      }
      // PLANCHER PAR K, NON ARRONDI (9041c191) + ATTRIBUTION NON NULLE
      // (99eec23d : tout-dans-le-residuel restait vert) : sur les doubles
      // bruts, chaque record K porte une somme de fenetres STRICTEMENT
      // positive — les horloges ns rendent toute fenetre reelle > 0.
      if (!(somme > 0.0 && mur > 0.0)) {
        expect(false, "succes : chaque record K porte somme > 0 ET mur > 0 (attribution non nulle)");
        break;
      }
      if (mur - somme < -0.005) {
        expect(false, "succes : residuel non negatif (memes bornes que les fenetres)");
        break;
      }
      if (pf.a_end < pf.a_begin || pf.end < pf.begin) {
        expect(false, "succes : intervalles A et reduce_interne ordonnes");
        break;
      }
    }
  }

  // La scene de refus PRE-fold (budget minuscule) serait VACUEUSE : le refus
  // precede fold_profiles.assign (ac6b4bc1). La scene causale est la panne
  // injectee ci-dessus (porte mhgp7_profil_contrat_echec_k2, code 4).
  return failures ? 1 : 0;
#endif
}

