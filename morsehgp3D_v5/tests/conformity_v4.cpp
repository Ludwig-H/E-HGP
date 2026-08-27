// MorseHGP3D v5 — PORTE DE CONFORMITE v4 ≡ v5.
//
// Lit le recu `receipts/conformite_v4/digests_v4.txt` (digests canoniques
// calcules PAR LA V4 sur les memes entrees), execute le pipeline v5 sur
// (famille, n, s=8, smax=11, seed=3) et exige l'egalite de digest_balls ET
// digest_all. Codes : 0 conforme, 1 divergence (a documenter : choix de
// contrat ou fixture v5, jamais une difference silencieuse), 2 entree absente
// du recu ou refus, 3 invariant.
// PORTE DE MUTANTS DU PIPELINE : avec --inject=<mutant>, la porte execute
// d'abord son BRAS NOMINAL APPARIE (le meme binaire, memes arguments sans
// injection, en sous-processus : les points d'injection sont memorises par
// site, une bascule intra-processus est impossible) et exige que ce bras soit
// conforme au recu ; puis la divergence du bras mutant par rapport au nominal
// EST la mise a mort (code 4). Un digest inchange sous mutant est une porte
// inefficace (code 3) ; un nominal non conforme rend 3 (jamais un faux 4).
// Un mutant qui fait REFUSER le pipeline est tue lui aussi.
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  std::string receipt, fam_name, inject;
  int n = 0, threads = 1;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--receipt=", 0) == 0) receipt = arg.substr(10);
    else if (arg.rfind("--family=", 0) == 0) fam_name = arg.substr(9);
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  CloudFamily family;
  if (receipt.empty() || n < 2 || !parse_cloud_family(fam_name.c_str(), &family)) return 2;
  if (!inject.empty() && !mutants_enable(inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", inject.c_str());
    return 2;
  }
  std::ifstream f(receipt);
  if (!f) {
    std::fprintf(stderr, "REFUS : recu %s illisible\n", receipt.c_str());
    return 2;
  }
  std::string line, exp_balls, exp_all;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream is(line);
    std::string fam, db, da;
    int nn = 0;
    is >> fam >> nn >> db >> da;
    if (fam == fam_name && nn == n) {
      exp_balls = db;
      exp_all = da;
    }
  }
  if (exp_all.empty()) {
    std::fprintf(stderr, "REFUS : aucune entree (%s, %d) dans le recu\n", fam_name.c_str(), n);
    return 2;
  }
  // Bras nominal apparie (sous-processus) quand un mutant est injecte.
  std::string nom_balls, nom_all;
  if (!inject.empty()) {
    char cmd[2048];
    std::snprintf(cmd, sizeof(cmd), "%s --receipt=%s --family=%s --n=%d --threads=%d", argv[0], receipt.c_str(),
                  fam_name.c_str(), n, threads);
    FILE* pf = popen(cmd, "r");
    if (!pf) return 2;
    char line[512];
    while (std::fgets(line, sizeof(line), pf)) {
      if (std::strncmp(line, "digest_balls=", 13) == 0) nom_balls = std::string(line + 13, 64);
      if (std::strncmp(line, "digest_all=", 11) == 0) nom_all = std::string(line + 11, 64);
    }
    const int st = pclose(pf);
    if (st != 0 || nom_balls != exp_balls || nom_all != exp_all) {
      std::fprintf(stderr, "PORTE INEFFICACE : le bras nominal n'est pas conforme au recu (statut %d)\n", st);
      return 3;
    }
  }
  RunOptions opt;
  opt.threads = threads;
  opt.digest = true;
  const int coord = cloud_family_default_coord(family, n);
  const RunResult rr = run_pipeline(make_family_input(family, n, coord, 3), opt);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    if (!inject.empty()) {
      std::fprintf(stderr, "MUTANT TUE : %s change le statut\n", inject.c_str());
      return 4;
    }
    return status_exit_code(rr.status);
  }
  print_run(stdout, fam_name.c_str(), n, coord, 3, opt, rr);
  const bool ok_b = rr.digest_balls == exp_balls, ok_a = rr.digest_all == exp_all;
  std::printf("conformite_v4 famille=%s n=%d balls=%s all=%s\n", fam_name.c_str(), n, ok_b ? "egal" : "DIFFERENT",
              ok_a ? "egal" : "DIFFERENT");
  if (!inject.empty()) {
    const bool same_as_nominal = rr.digest_balls == nom_balls && rr.digest_all == nom_all;
    if (same_as_nominal) {
      std::fprintf(stderr, "PORTE INEFFICACE : mutant %s sans effet sur les digests\n", inject.c_str());
      return 3;
    }
    std::fprintf(stderr, "MUTANT TUE : %s (balls %s, all %s)\n", inject.c_str(), ok_b ? "=" : "!=", ok_a ? "=" : "!=");
    return 4;
  }
  if (!ok_b || !ok_a) {
    std::fprintf(stderr, "DIVERGENCE v4/v5 : balls %s / all %s\n", ok_b ? "=" : "!=", ok_a ? "=" : "!=");
    return 1;
  }
  return 0;
}
