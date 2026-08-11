// MorseHGP3D v3 — LA PORTE DU CATALOGUE PARALLELE.
//
// Le front d'onde CPU (`flat_catalogue_parallel`) partitionne l'arbre de la
// reverse search en couronne + sous-arbres distribues. La porte recoit cette
// partition par TROIS certificats, dans cet ordre :
//
//   1. le MULTIENSEMBLE COMPLET des sommets emis (coquille, interieur, niveau),
//      couronne + sous-arbres, compare a la verite du `reverse_search_stream`
//      SEQUENTIEL — d'abord la MULTIPLICITE UN (chaque sommet couvert
//      exactement une fois : le mutant duplicate-root meurt ici), puis
//      l'EGALITE de multiensembles (le mutant drop-odd-roots meurt ici).
//      Le mutant historique qui perdait un sous-arbre montrait pourquoi la
//      deduplication finale de la recolte n'est PAS ce certificat : elle
//      redecouvre les boules perdues depuis d'autres sommets et avale les
//      copies — la porte precedente ne comparait que le catalogue deduplique.
//   2. le DIFFERENTIEL CANONIQUE du catalogue (membres tries, support
//      canonique, rang) contre la recolte sequentielle sur les MEMES sommets ;
//   3. l'INVARIANCE DU TRAVAIL entre `threads` et `threads+1` : la partition
//      d'arbre ne depend pas du nombre de threads, donc les compteurs non plus
//      — egalite insensible a l'horloge, qui juge sous etranglement CPU.
//
// La porte tourne MULTITHREAD (threads >= 4 exiges) : la data race des
// compteurs `mutable` du `CertifiedIndex` partage etant reparee (metriques par
// worker), le probe parallele n'est plus contraint a un thread. Des PLANCHERS
// de couverture (racines de frontiere, sommets de sous-arbres) interdisent le
// vert-par-vacuite : une campagne sans sous-arbre distribue ne recoit rien.
//
// Codes de sortie contractuels : 1 = desaccord, 2 = campagne refusee,
// 3 = plancher/invariant, 4 = mutant tue.
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstring>
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

// Le sommet, reduit a son identite publique complete : coquille, interieur,
// niveau. C'est le grain du multiensemble — pas la boule dedupliquee.
struct VertexKey {
  std::vector<mhgp::i32> shell;
  std::vector<mhgp::i32> interior;
  int level = 0;
  bool operator<(const VertexKey& o) const {
    if (shell != o.shell) return shell < o.shell;
    if (interior != o.interior) return interior < o.interior;
    return level < o.level;
  }
  bool operator==(const VertexKey& o) const {
    return shell == o.shell && interior == o.interior && level == o.level;
  }
};

VertexKey key_of(const mhgp3v::flats::Vertex& v) {
  return VertexKey{v.shell, v.interior, v.level};
}

// Le multiensemble canonique des sommets EMIS par la voie parallele :
// couronne, puis chaque sous-arbre — trie, multiplicites conservees.
std::vector<VertexKey> emitted_multiset(const mhgp3v::ParallelEmission& emission) {
  std::vector<VertexKey> keys;
  for (const auto& v : emission.crown) keys.push_back(key_of(v));
  for (const auto& subtree : emission.subtrees)
    for (const auto& v : subtree) keys.push_back(key_of(v));
  std::sort(keys.begin(), keys.end());
  return keys;
}

}  // namespace

int main(int argc, char** argv) {
  int points = 120, smax = 11, clouds = 3, threads = 4;
  long long seed = 20260810;
  long long min_roots = 0, min_subtree_vertices = 0;
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
    else if (!strcmp(argv[i], "--threads")) threads = (int)value;
    else if (!strcmp(argv[i], "--min-roots")) min_roots = value;
    else if (!strcmp(argv[i], "--min-subtree-vertices")) min_subtree_vertices = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (points < 8 || points > 2000 || smax < 4 || smax > 32 || clouds < 1 || clouds > 64) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  // La porte est MULTITHREAD par contrat : la race du CertifiedIndex etant
  // reparee, une campagne a moins de quatre threads n'exerce plus le regime
  // qu'elle pretend recevoir.
  if (threads < 4 || threads > 64) {
    std::printf("ECHEC : campagne absurde (threads >= 4 exiges, <= 64)\n");
    return 2;
  }
  mhgp3v::ParallelCatalogueMutants mutants{};
  if (mutant_name != nullptr) {
    if (!strcmp(mutant_name, "drop-odd-roots")) mutants.drop_odd_roots = true;
    else if (!strcmp(mutant_name, "duplicate-root")) mutants.duplicate_root = true;
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
  long long total_roots = 0;
  long long total_subtree_vertices = 0;
  long long total_crown_vertices = 0;
  for (int c = 0; c < clouds; ++c) {
    const long long cloud_seed = seed + c;
    const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(
        family, points, mhgp3v::cloud_family_default_coord(family, points), cloud_seed);
    if ((int)pts.size() < points) { std::printf("ECHEC : nuage non genere\n"); return 3; }

    // LA VERITE SEQUENTIELLE : le `reverse_search_stream` sequentiel, indexe
    // comme la voie parallele, emet chaque sommet exactement une fois. Son
    // flux EST le multiensemble de reference — et la recolte sur ces memes
    // sommets donne le catalogue de reference.
    mhgp3v::CertifiedIndex tree;
    tree.build(pts, 16);
    std::vector<mhgp3v::flats::Vertex> truth_vertices;
    mhgp3v::FlatStatistics st_seq{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    mhgp3v::reverse_search_stream(pts, smax - 2, &st_seq, &status,
                                  [&](const mhgp3v::flats::Vertex& v) {
                                    truth_vertices.push_back(v);
                                    return true;
                                  },
                                  &tree);
    if (status != mhgp3v::CloudStatus::kOk) {
      std::printf("ECHEC : statut sequentiel %s\n", mhgp3v::cloud_status_name(status));
      return 3;
    }
    std::vector<VertexKey> truth_keys;
    truth_keys.reserve(truth_vertices.size());
    for (const auto& v : truth_vertices) truth_keys.push_back(key_of(v));
    std::sort(truth_keys.begin(), truth_keys.end());
    if (std::adjacent_find(truth_keys.begin(), truth_keys.end()) != truth_keys.end()) {
      std::printf("ECHEC : la verite sequentielle porte un sommet en double — invariant"
                  " de la reverse search viole\n");
      return 3;
    }
    mhgp3v::FlatStatistics st_harvest{};
    const mhgp::Catalogue sequential =
        mhgp3v::flat_catalogue(pts, smax, &st_harvest, &status, false, true, false,
                               &truth_vertices);
    if (status != mhgp3v::CloudStatus::kOk) {
      std::printf("ECHEC : statut recolte sequentielle %s\n",
                  mhgp3v::cloud_status_name(status));
      return 3;
    }
    const std::vector<CanonicalEntry> truth = canonical_form(sequential);

    // DEUX EXECUTIONS PARALLELES, `threads` et `threads + 1`, toutes deux au
    // dela du plancher multithread : le multiensemble et le catalogue sont
    // exiges sur chacune, l'invariance du travail entre les deux.
    const int runs[2] = {threads, threads + 1};
    mhgp3v::FlatStatistics st_runs[2]{};
    for (int run = 0; run < 2; ++run) {
      mhgp3v::ParallelEmission emission;
      const mhgp::Catalogue parallel = mhgp3v::flat_catalogue_parallel(
          pts, smax, &st_runs[run], &status, runs[run], true, 8, mutants, &emission);
      const std::string label = std::to_string(runs[run]) + " threads";
      if (status != mhgp3v::CloudStatus::kOk)
        return kill(("le statut a " + label).c_str(), mhgp3v::cloud_status_name(status));

      // 1a. MULTIPLICITE UN. Chaque sommet est couvert exactement une fois par
      // la partition couronne+sous-arbres : un doublon dans le multiensemble
      // emis est un sous-arbre rejoue ou une couronne qui deborde — la
      // deduplication de la recolte ne le verrait jamais.
      const std::vector<VertexKey> emitted = emitted_multiset(emission);
      const auto duplicate = std::adjacent_find(emitted.begin(), emitted.end());
      if (duplicate != emitted.end())
        return kill(("la multiplicite du multiensemble a " + label).c_str(),
                    "nuage " + std::to_string(c) + " : un sommet de niveau " +
                        std::to_string(duplicate->level) + " (coquille de " +
                        std::to_string(duplicate->shell.size()) +
                        ") est emis plus d'une fois");

      // 1b. EGALITE DES MULTIENSEMBLES (coquille, interieur, niveau) avec la
      // verite sequentielle : l'identite couronne+sous-arbres, sommet par
      // sommet — un sous-arbre perdu manque ici meme si toutes ses boules sont
      // redecouvertes ailleurs.
      if (emitted != truth_keys)
        return kill(("le multiensemble (coquille, interieur, niveau) a " + label).c_str(),
                    "nuage " + std::to_string(c) + " : " + std::to_string(emitted.size()) +
                        " sommets emis (couronne " + std::to_string(emission.crown.size()) +
                        " + " + std::to_string(emission.subtrees.size()) +
                        " sous-arbres) contre " + std::to_string(truth_keys.size()) +
                        " sequentiels");

      // 2. LE DIFFERENTIEL CANONIQUE du catalogue, contre la recolte
      // sequentielle sur les memes sommets de verite.
      const std::vector<CanonicalEntry> form = canonical_form(parallel);
      if (form != truth)
        return kill(("le differentiel canonique a " + label).c_str(),
                    "nuage " + std::to_string(c) + " : " + std::to_string(form.size()) +
                        " generateurs contre " + std::to_string(truth.size()));

      if (run == 0) {
        total_roots += (long long)emission.subtrees.size();
        total_crown_vertices += (long long)emission.crown.size();
        for (const auto& subtree : emission.subtrees)
          total_subtree_vertices += (long long)subtree.size();
      }
    }

    // 3. L'INVARIANCE DU TRAVAIL entre les deux comptes de threads : la
    // partition d'arbre ne depend pas du nombre de threads, donc les compteurs
    // non plus. Cette egalite est insensible a l'horloge — elle juge meme sous
    // etranglement.
    if (st_runs[0].reverse_flats_enumerated != st_runs[1].reverse_flats_enumerated ||
        st_runs[0].reverse_children_tested != st_runs[1].reverse_children_tested ||
        st_runs[0].reverse_decisions != st_runs[1].reverse_decisions)
      return kill("l'invariance du travail",
                  "nuage " + std::to_string(c) + " : compteurs " +
                      std::to_string(runs[0]) + " threads != " + std::to_string(runs[1]) +
                      " threads");
    total_spheres += (long long)truth.size();
  }
  if (mutant_name != nullptr) {
    std::printf("MUTANT SURVIVANT %s : la porte ne mord pas\n", mutant_name);
    return 0;
  }
  // PLANCHERS DE COUVERTURE : une campagne dont la frontiere est vide, ou dont
  // les sous-arbres n'emettent rien, n'a pas exerce la partition — elle ne
  // recoit rien.
  if (total_roots < min_roots || total_subtree_vertices < min_subtree_vertices) {
    std::printf("ECHEC : plancher de couverture non atteint — racines %lld/%lld,"
                " sommets de sous-arbres %lld/%lld\n",
                total_roots, min_roots, total_subtree_vertices, min_subtree_vertices);
    return 3;
  }
  std::printf("OK : multiensemble (coquille, interieur, niveau) == sequentiel, multiplicite un,"
              " catalogue canonique identique sur %d nuages %s de %d points, travail invariant"
              " %d/%d threads, %lld generateurs, %lld racines, %lld sommets de sous-arbres,"
              " %lld de couronne\n",
              clouds, mhgp3v::cloud_family_name(family), points, threads, threads + 1,
              total_spheres, total_roots, total_subtree_vertices, total_crown_vertices);
  return 0;
}
