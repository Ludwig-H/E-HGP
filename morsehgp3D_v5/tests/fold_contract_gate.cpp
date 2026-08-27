// MorseHGP3D v5 — porte du CONTRAT STRUCTUREL du fold (P0 de l'audit 9762daaf) :
// build_forest refuse AVANT toute allocation tout evenement hors contrat
// (q in [2, 11], d <= 9, q + d <= 11, K constant dans l'appel, identifiants
// distincts, active_mask < 2^q) et accepte les deux limites positives
// q11+d0 et q2+d9. Rejets permanents : q11+d1 (le probe de l'auditeur),
// q12, d10, q<2, K mixtes, doublon, bit de masque haut. A executer aussi sous
// MHGP5_ENABLE_SANITIZERS : aucune ecriture hors de FacetKey ne doit avoir
// lieu sur un refus. Codes : 0 conforme, 1 desaccord.
#include <cstdio>
#include <string>
#include <vector>

#include "../src/forest/fold.hpp"

using namespace mhgp5;

namespace {
int failures = 0;
ForestEvent make(int q, int d, u16 mask = 0) {
  ForestEvent e;
  e.q = (u8)q;
  e.d = (u8)d;
  e.active_mask = mask;
  e.level.num[0] = 1;
  e.level.num[1] = 0;
  e.level.num[2] = 0;
  e.level.den = 1;  // contrat ExactLevel : den > 0 (le constructeur par defaut donne den = 0, invalide)
  int id = 0;
  for (int t = 0; t < q && t < 11; ++t) e.support[t] = (PointId)id++;
  for (int t = 0; t < d && t < 9; ++t) e.interior[t] = (PointId)id++;
  return e;
}
void expect_refusal(const char* label, const std::vector<ForestEvent>& ev) {
  const ForestResult r = build_forest(ev, 2);
  if (r.refusal.empty() || r.refusal.rfind("invalid_input", 0) != 0) {
    std::printf("NON REFUSE : %s (%s)\n", label, r.refusal.c_str());
    ++failures;
  } else {
    std::printf("refuse : %s -> %s\n", label, r.refusal.c_str());
  }
}
void expect_accept(const char* label, const std::vector<ForestEvent>& ev) {
  const ForestResult r = build_forest(ev, 2);
  if (!r.refusal.empty()) {
    std::printf("REFUSE A TORT : %s (%s)\n", label, r.refusal.c_str());
    ++failures;
  } else {
    std::printf("accepte : %s (facettes=%llu)\n", label, (unsigned long long)r.facets);
  }
}
}  // namespace

int main() {
  // Le probe de l'auditeur : q=11, d=1 (12 enregistrements, 11 cases de FacetKey).
  {
    ForestEvent e = make(11, 0);
    e.d = 1;
    e.interior[0] = 11;
    expect_refusal("q11+d1", {e});
  }
  {
    ForestEvent e = make(11, 0);
    e.q = 12;
    expect_refusal("q12", {e});
  }
  {
    ForestEvent e = make(2, 9);
    e.d = 10;
    expect_refusal("d10", {e});
  }
  expect_refusal("q1", {make(1, 0)});
  expect_refusal("q0", {make(0, 0)});
  expect_refusal("K mixtes (q2+d0 puis q3+d0)", {make(2, 0), make(3, 0)});
  {
    ForestEvent e = make(3, 1);
    e.interior[0] = e.support[0];
    expect_refusal("doublon support/interieur", {e});
  }
  {
    ForestEvent e = make(3, 0);
    e.support[2] = e.support[1];
    expect_refusal("doublon dans le support", {e});
  }
  expect_refusal("bit de masque haut (q=3, mask=8)", {make(3, 0, 8)});
  expect_refusal("bit de masque haut (q=2, mask=4) parmi des valides", {make(2, 0, 3), make(2, 0, 4)});
  // Contrat ExactLevel : den > 0.
  {
    ForestEvent e = make(3, 0);
    e.level.den = 0;
    expect_refusal("niveau den = 0", {e});
  }
  {
    ForestEvent e = make(3, 0);
    e.level.den = -1;
    expect_refusal("niveau den < 0", {e});
  }
  // Limites positives.
  expect_accept("q11+d0 (mask plein)", {make(11, 0, 0x7ff)});
  expect_accept("q2+d9", {make(2, 9, 3)});
  expect_accept("vide", {});
  if (failures) {
    std::printf("DESACCORDS : %d\n", failures);
    return 1;
  }
  std::printf("fold_contract OK\n");
  return 0;
}
