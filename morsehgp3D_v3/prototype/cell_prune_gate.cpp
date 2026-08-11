// MorseHGP3D v3 — LA PORTE DU PRUNE CONVEXE CERTIFIE.
//
// Les fixtures de la reponse pont/q4 (§6) :
//
//   1. CONTACT N'EST PAS SEPARATION : un point de A exactement sur la face
//      de la cellule — un centre de vrai support peut vivre SUR cette face ;
//      la cellule est conservee. Le mutant contact-as-separation prune a
//      tort et meurt ici.
//   2. PICS LOINTAINS : closure(C) rencontre conv(X) (les pics portent
//      l'enveloppe au-dessus de la cellule), mais conv(A_C) local reste
//      sous un plan separateur — le prune honnete separe par l'axe z ; le
//      mutant prune-from-cloud, qui teste conv(X), perd le prune et meurt.
//   3. NAPPE INCLINEE : aucune separation d'AXE n'existe, la direction du
//      barycentre certifie — la partie « plan general » mord au-dela du v1.
//
// Codes : 0 OK ; 1 ecart ; 2 CLI ; 4 mutant tue.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "prototype/cell_prune.hpp"

namespace {

mhgp3v::CellPruneVerdict run_prune(const std::vector<mhgp::P3>& local,
                                   const std::vector<mhgp::P3>& cloud,
                                   const mhgp::i64* box_lo, const mhgp::i64* box_hi,
                                   mhgp3v::CellPruneMutants mutants) {
  // Le mutant prune-from-cloud remplace A_C par X entier — la ou l'honnete
  // ne certifie que sur les points locaux.
  const std::vector<mhgp::P3>& subject = mutants.prune_from_cloud ? cloud : local;
  return mhgp3v::cell_prune(box_lo, box_hi,
                            [&](std::size_t i) { return subject[i]; }, subject.size(),
                            mutants);
}

}  // namespace

int main(int argc, char** argv) {
  const char* mutant_name = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--mutant") && i + 1 < argc) {
      mutant_name = argv[++i];
      continue;
    }
    std::printf("ECHEC : argument inconnu %s\n", argv[i]);
    return 2;
  }
  mhgp3v::CellPruneMutants mutants{};
  if (mutant_name != nullptr) {
    if (!strcmp(mutant_name, "contact-as-separation")) mutants.contact_as_separation = true;
    else if (!strcmp(mutant_name, "prune-from-cloud")) mutants.prune_from_cloud = true;
    else { std::printf("ECHEC : mutant inconnu %s\n", mutant_name); return 2; }
  }
  const auto kill = [&](const std::string& where, const std::string& why) {
    if (mutant_name != nullptr) {
      std::printf("mutant tue par %s : %s\n", where.c_str(), why.c_str());
      return 4;
    }
    std::printf("ECHEC %s : %s\n", where.c_str(), why.c_str());
    return 1;
  };

  // FIXTURE 1 : le point au contact de la face x=2 — conserve.
  {
    const mhgp::i64 box_lo[3] = {0, 0, 0}, box_hi[3] = {2, 2, 2};
    const std::vector<mhgp::P3> contact = {mhgp::P3{2, 1, 1}};
    const auto verdict = run_prune(contact, contact, box_lo, box_hi, mutants);
    if (verdict.separated)
      return kill("la fixture du contact",
                  "un contact a ete accepte comme separation — un centre de frontiere"
                  " serait perdu");
  }
  // FIXTURE 2 : la nappe locale sous la cellule, les pics lointains portant
  // conv(X) au-dessus — le prune honnete separe, le mutant nuage le perd.
  {
    const mhgp::i64 box_lo[3] = {10, 10, 40}, box_hi[3] = {12, 12, 42};
    std::vector<mhgp::P3> local;
    for (mhgp::i64 x = 0; x <= 22; x += 2)
      for (mhgp::i64 y = 0; y <= 22; y += 2)
        local.push_back(mhgp::P3{x, y, (x + y) % 3});
    std::vector<mhgp::P3> cloud = local;
    cloud.push_back(mhgp::P3{0, 0, 100});
    cloud.push_back(mhgp::P3{22, 22, 100});
    const auto verdict = run_prune(local, cloud, box_lo, box_hi, mutants);
    if (!verdict.separated)
      return kill("la fixture des pics lointains",
                  "le plan separateur local n'a pas ete certifie (conv(X) n'est pas"
                  " l'autorite)");
  }
  // FIXTURE 3 : nappe INCLINEE z = x sous une cellule decalee — aucune
  // separation d'axe (la nappe monte au-dessus de lo_z de la cellule sur un
  // cote, et la cellule est dans l'emprise x/y), la direction du barycentre
  // certifie.
  {
    const mhgp::i64 box_lo[3] = {8, 8, 30}, box_hi[3] = {10, 10, 32};
    std::vector<mhgp::P3> slope;
    for (mhgp::i64 x = 0; x <= 40; x += 2)
      for (mhgp::i64 y = 0; y <= 18; y += 2)
        slope.push_back(mhgp::P3{x, y, x});
    const auto verdict = run_prune(slope, slope, box_lo, box_hi, mutants);
    if (!verdict.separated)
      return kill("la fixture de la nappe inclinee",
                  "le plan general n'a pas certifie une separation reelle");
    if (mutant_name == nullptr && verdict.by_axis)
      return kill("la fixture de la nappe inclinee",
                  "la separation devait exiger le plan general, pas un axe — la"
                  " fixture ne mord plus");
  }

  if (mutant_name != nullptr) {
    std::printf("MUTANT SURVIVANT %s : la porte ne mord pas\n", mutant_name);
    return 0;
  }
  std::printf("OK : prune convexe certifie — contact conserve, pics lointains separes en"
              " local, nappe inclinee separee par le plan general\n");
  return 0;
}
