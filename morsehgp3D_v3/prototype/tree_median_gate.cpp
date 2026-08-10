// MorseHGP3D v3 — LA PORTE DU ROUTEUR MEDIAN (note dendrogramme, §14).
//
// Trois etages : les FIXTURES GRAVEES de la note (5 : branche 6=3+3 contre 5,
// la mediane s'arrete au NOEUD interne, pas a une feuille ; 6 : etoile
// 34/33/33, la mediane reste au PARENT ; 7 : egalite 1/2--1/2, le terminal est
// le noeud LE PLUS HAUT, stable sous permutation des enfants) ; le
// DIFFERENTIEL contre l'oracle par enumeration de TOUS les terminaux sur des
// arbres et mesures aleatoires (perte minimale ET noeud le plus haut du
// segment de medianes) ; les MUTANTS (glouton renomme mediane, seuil >= 1/2,
// stay structurel interdit, descente sur egalite) — chacun tue par une
// fixture ou l'enumeration, code 4 attendu.
#include <charconv>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "prototype/tree_median_router.hpp"

namespace {

bool check_median_against_enumeration(const mhgp3v::MasterTree& tree,
                                      const mhgp3v::PointMeasure& measure,
                                      mhgp3v::MedianMutants mutants, std::string* why) {
  const int routed = mhgp3v::route_median(tree, measure, mutants);
  const long long routed_loss = mhgp3v::enumeration_loss(tree, measure, routed);
  long long best = -1;
  for (std::size_t t = 0; t < tree.parent.size(); ++t) {
    const long long loss = mhgp3v::enumeration_loss(tree, measure, (int)t);
    if (best < 0 || loss < best) best = loss;
  }
  if (routed_loss != best) {
    *why = "perte " + std::to_string(routed_loss) + " != minimum " + std::to_string(best);
    return false;
  }
  // Le plus haut du segment : l'ancetre immediat du terminal ne doit pas etre
  // lui aussi minimal (sinon le contrat exigeait de rester plus haut).
  const int up = tree.parent[(std::size_t)routed];
  if (up >= 0 && mhgp3v::enumeration_loss(tree, measure, up) == best) {
    *why = "terminal " + std::to_string(routed) + " n'est pas le plus haut du segment";
    return false;
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  int trees = 200, nodes = 24;
  long long seed = 20260810;
  const char* mutant_name = nullptr;
  auto integer = [](const char* text, long long* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 100000000ULL) return false;
    *value = (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--mutant")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --mutant\n"); return 2; }
      mutant_name = argv[++i];
      continue;
    }
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!strcmp(argv[i], "--trees")) { if (!has) { std::printf("ECHEC : valeur\n"); return 2; } trees = (int)value; ++i; }
    else if (!strcmp(argv[i], "--nodes")) { if (!has) { std::printf("ECHEC : valeur\n"); return 2; } nodes = (int)value; ++i; }
    else if (!strcmp(argv[i], "--seed")) { if (!has) { std::printf("ECHEC : valeur\n"); return 2; } seed = value; ++i; }
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
  }
  if (trees < 1 || trees > 100000 || nodes < 2 || nodes > 4096) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  mhgp3v::MedianMutants mutants{};
  if (mutant_name != nullptr) {
    if (!strcmp(mutant_name, "greedy-plurality")) mutants.greedy_plurality = true;
    else if (!strcmp(mutant_name, "half-inclusive")) mutants.half_inclusive = true;
    else if (!strcmp(mutant_name, "forbid-zero-stay")) mutants.forbid_zero_stay = true;
    else if (!strcmp(mutant_name, "descend-on-tie")) mutants.descend_on_tie = true;
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

  // FIXTURE 5 : racine R -> {A -> {a1, a2}, B}, masses a1=3, a2=3, B=5.
  // La mediane est le NOEUD A (masse 6/11 majoritaire, puis 3/11-3/11-5/11
  // toutes < 1/2) ; le glouton fonce dans une feuille de masse 3.
  {
    const mhgp3v::MasterTree tree = mhgp3v::make_master_tree({-1, 0, 0, 1, 1});
    mhgp3v::PointMeasure measure;
    measure.total = 11;
    measure.mass = {{3, 3}, {4, 3}, {2, 5}};
    const int terminal = mhgp3v::route_median(tree, measure, mutants);
    if (terminal != 1) return kill("la fixture 6=3+3 contre 5",
                                   "terminal " + std::to_string(terminal) + " != noeud A");
    std::string why;
    if (!check_median_against_enumeration(tree, measure, mutants, &why))
      return kill("l'enumeration de la fixture 6=3+3", why);
  }
  // FIXTURE 6 : etoile a trois feuilles 34/33/33 — la mediane reste au PARENT.
  {
    const mhgp3v::MasterTree tree = mhgp3v::make_master_tree({-1, 0, 0, 0});
    mhgp3v::PointMeasure measure;
    measure.total = 100;
    measure.mass = {{1, 34}, {2, 33}, {3, 33}};
    const int terminal = mhgp3v::route_median(tree, measure, mutants);
    if (terminal != 0) return kill("l'etoile 34/33/33",
                                   "terminal " + std::to_string(terminal) + " != parent");
  }
  // FIXTURE 7 : chaine R -> u -> v, masses 1/2 en R et 1/2 en v — le segment
  // de medianes est [R, v], le contrat prend LE PLUS HAUT (R), stable sous
  // permutation (l'arbre renomme donne le meme terminal canonique).
  {
    const mhgp3v::MasterTree tree = mhgp3v::make_master_tree({-1, 0, 1});
    mhgp3v::PointMeasure measure;
    measure.total = 2;
    measure.mass = {{0, 1}, {2, 1}};
    const int terminal = mhgp3v::route_median(tree, measure, mutants);
    if (terminal != 0) return kill("l'egalite 1/2--1/2",
                                   "terminal " + std::to_string(terminal) + " != le plus haut");
    // Permutation : memes masses, enfants declares dans l'autre ordre.
    const mhgp3v::MasterTree mirrored = mhgp3v::make_master_tree({1, -1, 1});
    mhgp3v::PointMeasure mirrored_measure;
    mirrored_measure.total = 2;
    mirrored_measure.mass = {{1, 1}, {0, 1}};   // racine=1, chaine 1->0 ? non :
    // arbre miroir : racine 1, enfants {0, 2} — la masse est en racine et en 0.
    const int mirrored_terminal = mhgp3v::route_median(mirrored, mirrored_measure, mutants);
    if (mirrored_terminal != 1)
      return kill("la permutation de l'egalite", "terminal deplace par la permutation");
  }
  // DIFFERENTIEL : arbres aleatoires (parent[i] < i), mesures aleatoires —
  // perte minimale ET plus haut du segment, contre l'enumeration complete.
  std::mt19937 rng((unsigned)seed);
  for (int t = 0; t < trees; ++t) {
    std::vector<int> parent((std::size_t)nodes, -1);
    for (int v = 1; v < nodes; ++v)
      parent[(std::size_t)v] = (int)(rng() % (unsigned)v);
    const mhgp3v::MasterTree tree = mhgp3v::make_master_tree(parent);
    mhgp3v::PointMeasure measure;
    const int atoms = 1 + (int)(rng() % 7u);
    for (int a = 0; a < atoms; ++a) {
      const long long weight = 1 + (long long)(rng() % 9u);
      measure.mass.push_back({(int)(rng() % (unsigned)nodes), weight});
      measure.total += weight;
    }
    std::string why;
    if (!check_median_against_enumeration(tree, measure, mutants, &why))
      return kill("l'enumeration, arbre " + std::to_string(t), why);
  }
  if (mutant_name != nullptr) {
    std::printf("MUTANT SURVIVANT %s : la porte ne mord pas\n", mutant_name);
    return 0;
  }
  std::printf("OK : mediane == enumeration (perte minimale, plus haut du segment) sur"
              " %d arbres, fixtures 6=3+3, etoile et egalite 1/2--1/2 gravees\n", trees);
  return 0;
}
