// MorseHGP3D v3 — FIXTURES DU JUGE PRIMAL, BINAIRE AUTONOME.
//
// Les trois configurations viennent de la section 5.1 de
// audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md. Elles
// exercent exactement ce que le selftest dual ne pouvait pas : la couverture
// COLLECTIVE, la dependance aux poids, et la frontiere stricte.
#include <cstdio>
#include <string>

#include "jung_dual_judge.hpp"

using mhgp3v::jjudge::Verdict;

namespace {

struct Fixture {
  const char* nom;
  long long a[3], b[3];
  long long z[3][3];
  int k;
  int lane;
  bool attendu_groupe;      // le groupe couvre-t-il ?
  bool attendu_singletons;  // un singleton couvre-t-il ?
  const char* pourquoi;
};

}  // namespace

int main(int argc, char** argv) {
  bool fixtures = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--fixtures") fixtures = true;
    else { std::fprintf(stderr, "REFUS : argument inconnu\n"); return 2; }
  }
  if (!fixtures) { std::fprintf(stderr, "REFUS : choisir --fixtures\n"); return 2; }

  // Les trois fixtures viennent de la section 5.1 du contre-audit. Elles
  // exercent exactement ce que le selftest dual ne pouvait pas : la couverture
  // COLLECTIVE, la dependance aux poids, et la frontiere.
  const Fixture f[] = {
      {"collective-q4", {0, 0, 0}, {100, 0, 0},
       {{42, 26, 0}, {42, 0, 26}, {0, 0, 0}}, 2, 4, true, false,
       "aucun singleton ne passe q4, le groupe de deux couvre"},
      {"poids-decident", {0, 0, 0}, {6, 0, 0},
       {{1, 0, 1}, {2, 2, 0}, {0, 0, 0}}, 2, 4, true, false,
       "le groupe couvre alors qu'aucun de ses deux membres ne le fait"},
      {"frontiere-q4", {0, 0, 0}, {6, 0, 0},
       {{2, 1, 1}, {0, 0, 0}, {0, 0, 0}}, 1, 4, false, false,
       "g^2 = 2R exactement : l'egalite reste hors du cone ouvert"},
  };

  int refutees = 0;
  for (const Fixture& x : f) {
    const Verdict vg = mhgp3v::jjudge::primal_couvre(x.a, x.b, x.z, x.k, x.lane);
    const bool couvre = (vg == Verdict::kCouvre);
    bool un_singleton = false;
    for (int i = 0; i < x.k; ++i) {
      long long un[1][3] = {{x.z[i][0], x.z[i][1], x.z[i][2]}};
      if (mhgp3v::jjudge::primal_couvre(x.a, x.b, un, 1, x.lane) == Verdict::kCouvre)
        un_singleton = true;
    }
    const bool ok = (couvre == x.attendu_groupe) && (un_singleton == x.attendu_singletons);
    std::printf("%-18s k=%d q%d : groupe=%s singleton=%s %s\n", x.nom, x.k, x.lane,
                couvre ? "COUVRE" : "non", un_singleton ? "COUVRE" : "non",
                ok ? "OK" : "REFUTEE");
    if (!ok) {
      std::fprintf(stderr, "FIXTURE REFUTEE %s : %s\n", x.nom, x.pourquoi);
      ++refutees;
    }
  }
  if (refutees != 0) {
    std::fprintf(stderr, "REFUS : %d fixture(s) primale(s) refutee(s)\n", refutees);
    return 3;
  }
  std::printf("juge_primal accord=OUI fixtures=%zu refutees=0\n",
              sizeof(f) / sizeof(f[0]));
  return 0;
}
