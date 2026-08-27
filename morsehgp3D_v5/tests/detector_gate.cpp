// MorseHGP3D v5 — porte du DETECTEUR de coherence des rayons de naissance
// (fold.hpp : `attach_violations`, `birth_violations`). Flux d'evenements
// FABRIQUE, incoherent par construction (K = 1) :
//   lot 1 (niveau 1) : σ = {A, B}, les deux facettes {A} et {B} ACTIVES ;
//   lot 2 (niveau 2) : σ = {A, C}, facette {C} active, facette {A} ATTACHEMENT
//     (bit actif a 0) — or {A} a deja ete vue au lot 1 : un attachement deja vu
//     est une violation des rayons de naissance (attach_violations = 1) ;
//   lot 3 (niveau 3) : σ = {B, C} avec {B} a la fois ACTIVE (bit 1) dans un
//     second evenement du meme lot et ATTACHEMENT dans le premier :
//     birth_violations = 1.
// Nominal : les deux compteurs sont non nuls (la fixture est bien incoherente)
// et la partition reste juste. Mutant `attach-detector-disabled` : les deux
// compteurs restent a zero -> code 4.
#include <cstdio>
#include <cstring>
#include <string>

#include "../src/forest/fold.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const auto ev = [](PointId p, PointId q, u16 mask, i128 level) {
    ForestEvent e;
    e.q = 2;
    e.d = 0;
    e.active_mask = mask;
    e.support[0] = p;
    e.support[1] = q;
    e.level = promote_level(Rational128{level, 1});
    return e;
  };
  const PointId A = 10, B = 20, C = 30;
  std::vector<ForestEvent> events = {ev(A, B, 0b11, 1), ev(A, C, 0b10, 2), ev(B, C, 0b10, 3), ev(B, C, 0b11, 3)};
  const ForestResult r = build_forest(events);
  std::printf("detector_gate facettes=%llu lots=%llu attach_violations=%llu birth_violations=%llu\n",
              (unsigned long long)r.facets, (unsigned long long)r.batches, (unsigned long long)r.attach_violations,
              (unsigned long long)r.birth_violations);
  int bad = 0;
  if (r.facets != 3 || r.batches != 3) ++bad;
  if (r.attach_violations < 1 || r.birth_violations < 1) ++bad;
  if (!inject.empty()) {
    if (bad) { std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str()); return 4; }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (bad) return 3;
  std::printf("detector_gate OK\n");
  return 0;
}
