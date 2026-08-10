// MorseHGP3D v3 — LA PORTE DU CATALOGUE PARALLELE.
//
// Le front d'onde CPU (`flat_catalogue_parallel`) partitionne l'arbre de la
// reverse search en couronne + sous-arbres distribues. L'ORDRE d'emission des
// sommets differe du DFS sequentiel : la comparaison est donc CANONIQUE, par
// la clef compacte (lemme : la liste triee des membres identifie la boule,
// miniball(M) == B pour un sature) plus le support canonique — independant du
// sommet decouvreur — et le rang. Trois etages :
//
//   1. le DIFFERENTIEL sequentiel == parallele (2 et 3 threads) sur une
//      campagne de nuages du MEME protocole que le pipeline (densite 1e-3) ;
//   2. l'INVARIANCE DU TRAVAIL : les compteurs du parcours (flats enumeres,
//      enfants testes, decisions) sont IDENTIQUES a 2 et 3 threads — la
//      partition d'arbre ne duplique ni ne perd aucun travail, et cette
//      egalite ne depend pas de l'horloge (elle juge sous etranglement CPU) ;
//   3. le MUTANT drop-odd-roots : perdre un sous-arbre de frontiere sur deux
//      doit etre tue par le differentiel, code 4 attendu. (Perdre seulement le
//      DERNIER sous-arbre survivait : trop petit, toutes ses boules etaient
//      redecouvertes depuis d'autres sommets — la porte exige un mutant qui
//      ne puisse pas se cacher derriere la deduplication de la recolte.)
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "prototype/cloud_families.hpp"
#include "prototype/parallel_catalogue.hpp"

namespace {

// La forme canonique d'un catalogue : multiensemble trie de
// (membres tries, support canonique, rang). La clef compacte identifie la
// boule ; le support et le rang verifient que la voie parallele publie la
// MEME identite publique, pas seulement le meme ensemble de boules.
struct CanonicalEntry {
  std::vector<mhgp::i32> members;
  std::vector<mhgp::i32> support;
  mhgp::i32 rank = 0;
  bool operator<(const CanonicalEntry& o) const {
    if (members != o.members) return members < o.members;
    if (support != o.support) return support < o.support;
    return rank < o.rank;
  }
  bool operator==(const CanonicalEntry& o) const {
    return members == o.members && support == o.support && rank == o.rank;
  }
};

std::vector<CanonicalEntry> canonical_form(const mhgp::Catalogue& catalogue) {
  std::vector<CanonicalEntry> form;
  form.reserve(catalogue.spheres.size());
  for (const mhgp::CriticalSphere& sphere : catalogue.spheres) {
    CanonicalEntry entry;
    entry.rank = sphere.rank;
    entry.members.assign(
        catalogue.members.begin() + sphere.members_begin,
        catalogue.members.begin() + sphere.members_begin + sphere.rank);
    for (mhgp::i32 s = 0; s < sphere.n_support; ++s)
      entry.support.push_back(sphere.support[s]);
    form.push_back(std::move(entry));
  }
  std::sort(form.begin(), form.end());
  return form;
}

}  // namespace

int main(int argc, char** argv) {
  int points = 120, smax = 11, clouds = 3;
  long long seed = 20260810;
  const char* mutant_name = nullptr;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kUniform;
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
    if (!strcmp(argv[i], "--family")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --family\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "uniform")) family = mhgp3v::CloudFamily::kUniform;
      else if (!strcmp(argv[i], "terrain")) family = mhgp3v::CloudFamily::kTerrain;
      else { std::printf("ECHEC : famille inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--points")) points = (int)value;
    else if (!strcmp(argv[i], "--smax")) smax = (int)value;
    else if (!strcmp(argv[i], "--clouds")) clouds = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (points < 8 || points > 2000 || smax < 4 || smax > 32 || clouds < 1 || clouds > 64) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  mhgp3v::ParallelCatalogueMutants mutants{};
  if (mutant_name != nullptr) {
    if (!strcmp(mutant_name, "drop-odd-roots")) mutants.drop_odd_roots = true;
    else { std::printf("ECHEC : mutant inconnu %s\n", mutant_name); return 2; }
  }
  const auto kill = [&](const char* where, const std::string& why) {
    if (mutant_name != nullptr) {
      std::printf("mutant tue par %s : %s\n", where, why.c_str());
      return 4;
    }
    std::printf("ECHEC %s : %s\n", where, why.c_str());
    return 1;
  };

  long long total_spheres = 0;
  for (int c = 0; c < clouds; ++c) {
    const long long cloud_seed = seed + c;
    const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(
        family, points, mhgp3v::cloud_family_default_coord(family, points), cloud_seed);
    if ((int)pts.size() < points) { std::printf("ECHEC : nuage non genere\n"); return 3; }

    mhgp3v::FlatStatistics st_seq{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    const mhgp::Catalogue sequential =
        mhgp3v::flat_catalogue(pts, smax, &st_seq, &status, false, true);
    if (status != mhgp3v::CloudStatus::kOk) {
      std::printf("ECHEC : statut sequentiel %s\n", mhgp3v::cloud_status_name(status));
      return 3;
    }
    const std::vector<CanonicalEntry> truth = canonical_form(sequential);

    mhgp3v::FlatStatistics st_two{}, st_three{};
    const mhgp::Catalogue two = mhgp3v::flat_catalogue_parallel(
        pts, smax, &st_two, &status, 2, true, 8, mutants);
    if (status != mhgp3v::CloudStatus::kOk)
      return kill("le statut a 2 threads", mhgp3v::cloud_status_name(status));
    const mhgp::Catalogue three = mhgp3v::flat_catalogue_parallel(
        pts, smax, &st_three, &status, 3, true, 8, mutants);
    if (status != mhgp3v::CloudStatus::kOk)
      return kill("le statut a 3 threads", mhgp3v::cloud_status_name(status));

    // 1. LE DIFFERENTIEL CANONIQUE, sequentiel contre chaque voie parallele.
    const std::vector<CanonicalEntry> form_two = canonical_form(two);
    if (form_two != truth)
      return kill("le differentiel a 2 threads",
                  "nuage " + std::to_string(c) + " : " + std::to_string(form_two.size()) +
                      " generateurs contre " + std::to_string(truth.size()));
    const std::vector<CanonicalEntry> form_three = canonical_form(three);
    if (form_three != truth)
      return kill("le differentiel a 3 threads",
                  "nuage " + std::to_string(c) + " : " + std::to_string(form_three.size()) +
                      " generateurs contre " + std::to_string(truth.size()));

    // 2. L'INVARIANCE DU TRAVAIL entre 2 et 3 threads : la partition d'arbre
    // ne depend pas du nombre de threads, donc les compteurs non plus. Cette
    // egalite est insensible a l'horloge — elle juge meme sous etranglement.
    if (st_two.reverse_flats_enumerated != st_three.reverse_flats_enumerated ||
        st_two.reverse_children_tested != st_three.reverse_children_tested ||
        st_two.reverse_decisions != st_three.reverse_decisions)
      return kill("l'invariance du travail",
                  "nuage " + std::to_string(c) + " : compteurs 2 threads != 3 threads");
    total_spheres += (long long)truth.size();
  }
  if (mutant_name != nullptr) {
    std::printf("MUTANT SURVIVANT %s : la porte ne mord pas\n", mutant_name);
    return 0;
  }
  std::printf("OK : catalogue parallele == sequentiel (clef compacte + support + rang) sur"
              " %d nuages %s de %d points, travail invariant 2/3 threads, %lld generateurs\n",
              clouds, mhgp3v::cloud_family_name(family), points, total_spheres);
  return 0;
}
