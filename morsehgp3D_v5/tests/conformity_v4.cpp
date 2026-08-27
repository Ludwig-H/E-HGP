// MorseHGP3D v5 — PORTE DE CONFORMITE v4 ≡ v5.
//
// Lit le recu `receipts/conformite_v4/digests_v4.txt` (digests canoniques
// calcules PAR LA V4 sur les memes entrees), execute le pipeline v5 sur
// (famille, n, s=8, smax=11, seed=3) et exige l'egalite de digest_balls ET
// digest_all. Codes : 0 conforme, 1 divergence (a documenter : choix de
// contrat ou fixture v5, jamais une difference silencieuse), 2 entree absente
// du recu ou refus, 3 invariant.
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  std::string receipt, fam_name;
  int n = 0, threads = 1;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--receipt=", 0) == 0) receipt = arg.substr(10);
    else if (arg.rfind("--family=", 0) == 0) fam_name = arg.substr(9);
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else return 2;
  }
  CloudFamily family;
  if (receipt.empty() || n < 2 || !parse_cloud_family(fam_name.c_str(), &family)) return 2;
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
  RunOptions opt;
  opt.threads = threads;
  opt.digest = true;
  const int coord = cloud_family_default_coord(family, n);
  const RunResult rr = run_pipeline(make_family_input(family, n, coord, 3), opt);
  if (rr.status != PipelineStatus::kCompleteRegular) {
    std::fprintf(stderr, "REFUS %s\n", rr.message.c_str());
    return status_exit_code(rr.status);
  }
  print_run(stdout, fam_name.c_str(), n, coord, 3, opt, rr);
  const bool ok_b = rr.digest_balls == exp_balls, ok_a = rr.digest_all == exp_all;
  std::printf("conformite_v4 famille=%s n=%d balls=%s all=%s\n", fam_name.c_str(), n, ok_b ? "egal" : "DIFFERENT",
              ok_a ? "egal" : "DIFFERENT");
  if (!ok_b || !ok_a) {
    std::fprintf(stderr, "DIVERGENCE v4/v5 : balls %s / all %s\n", ok_b ? "=" : "!=", ok_a ? "=" : "!=");
    return 1;
  }
  return 0;
}
