// MorseHGP3D v3 — LE JUGE GAMMA_k : la definition du manuscrit contre la chaine.
//
// La reponse Q1 de l'auditeur tranche : le catalogue critique de rang borne
// n'est PAS demontre suffisant hors position generale, et l'equivalence doit
// etre « prouvee et testee aux coupes ouverte et fermee ». Ce binaire est la
// moitie TEST : il calcule, en arithmetique rationnelle de l'oracle et sans
// AUCUNE primitive de la production, la verite du theoreme 2 du manuscrit —
// les composantes de Gamma_k(X, r), sommets = k-sous-ensembles presents des
// que r >= rho(sigma) (miniboule), aretes elementaires sigma~tau des que
// rho(sigma union tau) <= r — puis la confronte a la chaine sujet
// `mhgp3v::flat_catalogue` + `mhgp::build_forest`.
//
// L'UNITE DE COMPARAISON EST LA PARTITION PAR NIVEAU, PAS LA CONVENTION DE
// NOEUD. Les partitions d'identifiants couverts a chaque niveau d'evenement
// (coupe fermee ; la coupe ouverte d'un niveau est la coupe fermee du niveau
// precedent) determinent la foret de Hartigan comme quotient : deux chaines
// qui rendent les memes partitions partout portent la meme foret, quelles que
// soient leurs conventions de `source`, d'ordre interne ou de multifusion.
//
// LE PROTOCOLE DE LOT EST CELUI DE LA REPONSE Q1.2, PAS CELUI DU CODE :
// composantes de la coupe stricte figees, TOUTES les activations du niveau
// appliquees, composantes du lot entier, classification par nombre de racines
// strictes distinctes (0 naissance, 1 continuation, >=2 multifusion). Les
// naissances d'un niveau ne sont JAMAIS relues comme racines strictement
// anterieures par une fusion du meme lot.
//
// CE QUE CE JUGE NE FAIT PAS : il ne prouve aucun theoreme. Il MESURE ou la
// chaine coincide avec la definition et ou elle s'en ecarte, et il SEPARE les
// nuages a evenements degeneres (lots multiples, naissances a extras
// cospheriques, multifusions d'arite > 2) des nuages ou la position generale
// aurait tenu : sur ces derniers l'accord est EXIGE, sur les premiers il est
// MESURE et publie — c'est la carte de la frontiere Q1.

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "bigint.hpp"
#include "exact_geometry.hpp"
#include "rational.hpp"

#include "mhgp/mhgp.hpp"                    // le SUJET, jamais l'autorite
#include "prototype/order_k_flats.hpp"      // le SUJET : flat_catalogue

namespace {

using mhgp3v::BigInt;
using mhgp3v::Rational;
using mhgp3v::geometry::RationalSphere;
using mhgp3v::geometry::each_subset;
using mhgp3v::geometry::exact_miniball;
using mhgp3v::geometry::side_of;

// Une partition = collection triee d'ensembles tries d'identifiants.
using Partition = std::vector<std::vector<int>>;

void canonicalise(Partition* partition) {
  for (std::vector<int>& cluster : *partition) std::sort(cluster.begin(), cluster.end());
  std::sort(partition->begin(), partition->end());
}

// ---------------------------------------------------------------------------
// LA VERITE : Gamma_k par le protocole de lot normatif.
// ---------------------------------------------------------------------------
struct GammaTruth {
  std::vector<Rational> levels;            // niveaux d'evenement, croissants
  std::vector<Partition> partitions;       // partition a la coupe FERMEE de chaque niveau
  bool degenerate_events = false;
  long long births = 0, fusions = 0, continuations = 0;
  long long faces = 0, cofaces = 0;
  int maximum_relevant_rank = 0;           // rang max des miniboules d'evenement
  bool ok = false;
};

struct SimpleSets {
  std::vector<int> parent;
  int find(int a) {
    while (parent[(std::size_t)a] != a) {
      parent[(std::size_t)a] = parent[(std::size_t)parent[(std::size_t)a]];
      a = parent[(std::size_t)a];
    }
    return a;
  }
};

GammaTruth gamma_truth(const std::vector<mhgp::P3>& points, int k) {
  const int n = (int)points.size();
  GammaTruth truth;

  std::vector<std::vector<int>> faces;
  std::vector<Rational> face_level;
  std::vector<int> face_rank;
  bool miniball_failure = false;
  bool cospherical_extra = false;
  each_subset(n, k, [&](const std::vector<int>& face) {
    RationalSphere sphere;
    std::vector<int> support;
    if (!exact_miniball(points, face, &sphere, &support)) { miniball_failure = true; return; }
    int rank = 0;
    for (int z = 0; z < n; ++z) {
      const int side = side_of(sphere, points[(std::size_t)z]);
      if (side <= 0) ++rank;
      // LA DEGENERESCENCE EST CELLE DE LA DEF. 26 DU MANUSCRIT, PAS LE RANG :
      // un point interieur est GENERIQUE (toute face obtuse en a), un point
      // surnumeraire SUR la sphere ne l'est pas — c'est aussi exactement la
      // frontiere de domaine que le sujet v2 declare (coquille cospherique).
      if (side == 0 &&
          std::find(face.begin(), face.end(), z) == face.end())
        cospherical_extra = true;
    }
    faces.push_back(face);
    face_level.push_back(sphere.squared_radius);
    face_rank.push_back(rank);
  });
  std::vector<std::vector<int>> cofaces;
  std::vector<Rational> coface_level;
  std::vector<int> coface_rank;
  each_subset(n, k + 1, [&](const std::vector<int>& coface) {
    RationalSphere sphere;
    std::vector<int> support;
    if (!exact_miniball(points, coface, &sphere, &support)) { miniball_failure = true; return; }
    int rank = 0;
    for (int z = 0; z < n; ++z) {
      const int side = side_of(sphere, points[(std::size_t)z]);
      if (side <= 0) ++rank;
      if (side == 0 &&
          std::find(coface.begin(), coface.end(), z) == coface.end())
        cospherical_extra = true;
    }
    cofaces.push_back(coface);
    coface_level.push_back(sphere.squared_radius);
    coface_rank.push_back(rank);
  });
  if (miniball_failure) return truth;   // ok=false : fail-closed, jamais silencieux
  truth.degenerate_events = cospherical_extra;
  truth.faces = (long long)faces.size();
  truth.cofaces = (long long)cofaces.size();

  std::map<std::vector<int>, int> face_index;
  for (std::size_t i = 0; i < faces.size(); ++i) face_index.emplace(faces[i], (int)i);

  std::vector<Rational> levels;
  for (const Rational& r : face_level) levels.push_back(r);
  for (const Rational& r : coface_level) levels.push_back(r);
  std::sort(levels.begin(), levels.end(),
            [](const Rational& a, const Rational& b) { return compare(a, b) < 0; });
  levels.erase(std::unique(levels.begin(), levels.end(),
                           [](const Rational& a, const Rational& b) {
                             return compare(a, b) == 0;
                           }),
               levels.end());

  SimpleSets sets;
  sets.parent.resize(faces.size());
  for (std::size_t i = 0; i < faces.size(); ++i) sets.parent[i] = (int)i;
  std::vector<char> present(faces.size(), 0);
  // Le noeud de chaque composante STRICTE : toute composante nee porte
  // exactement un noeud (sa naissance) ; les continuations n'en creent pas.
  std::vector<int> node_of_root(faces.size(), -1);
  long long next_node = 0;

  for (const Rational& level : levels) {
    // 1. FIGER LA COUPE STRICTE : le noeud de la composante stricte de chaque
    // face deja presente, AVANT toute activation de ce niveau.
    std::vector<long long> strict_node(faces.size(), -1);
    for (std::size_t i = 0; i < faces.size(); ++i)
      if (present[i]) strict_node[i] = node_of_root[(std::size_t)sets.find((int)i)];

    // 2. ACTIVER toutes les faces du niveau.
    std::vector<std::size_t> touched;
    for (std::size_t i = 0; i < faces.size(); ++i) {
      if (present[i] || compare(face_level[i], level) != 0) continue;
      present[i] = 1;
      touched.push_back(i);
      truth.maximum_relevant_rank = std::max(truth.maximum_relevant_rank, face_rank[i]);
    }
    // 3. APPLIQUER toutes les aretes des cofaces du niveau. Toute facette d'une
    // coface active est deja presente (rho(facette) <= rho(coface)) : une
    // absence est une violation d'invariant du juge lui-meme.
    bool coface_event = false;
    for (std::size_t c = 0; c < cofaces.size(); ++c) {
      if (compare(coface_level[c], level) != 0) continue;
      coface_event = true;
      truth.maximum_relevant_rank = std::max(truth.maximum_relevant_rank, coface_rank[c]);
      int previous = -1;
      for (std::size_t drop = 0; drop < cofaces[c].size(); ++drop) {
        std::vector<int> face;
        for (std::size_t t = 0; t < cofaces[c].size(); ++t)
          if (t != drop) face.push_back(cofaces[c][t]);
        const auto it = face_index.find(face);
        if (it == face_index.end() || !present[(std::size_t)it->second]) {
          std::printf("ECHEC ORACLE : facette absente d'une coface active — invariant du"
                      " juge viole\n");
          return truth;   // ok=false
        }
        const int root = sets.find(it->second);
        if (previous >= 0 && root != sets.find(previous))
          sets.parent[(std::size_t)root] = sets.find(previous);
        previous = it->second;
        touched.push_back((std::size_t)it->second);
      }
    }
    (void)coface_event;

    // 4. COMPOSANTES DU LOT ENTIER, et classification par racines strictes.
    std::map<int, std::vector<std::size_t>> groups;
    for (std::size_t i = 0; i < faces.size(); ++i)
      if (present[i]) groups[sets.find((int)i)].push_back(i);

    // LES NIVEAUX EGAUX NE SONT PAS EN EUX-MEMES UNE DEGENERESCENCE : les
    // naissances simultanees des singletons au niveau zero (k=1) sont la
    // convention du manuscrit ; l'effondrement de miniboule d'une face obtuse
    // sur son sous-support cree des niveaux egaux GENERIQUES ; une seule
    // coface peut legitimement fusionner jusqu'a k+1 composantes d'un coup ;
    // et deux miniboules congruentes eloignees ne violent pas la Def. 26. La
    // degenerescence est la cosphericite (drapeau calcule a l'enumeration).
    std::set<int> changed_roots;
    for (std::size_t i : touched) changed_roots.insert(sets.find((int)i));
    for (const auto& group : groups) {
      if (changed_roots.find(group.first) == changed_roots.end()) continue;
      std::set<long long> strict_roots;
      for (std::size_t i : group.second)
        if (strict_node[i] >= 0) strict_roots.insert(strict_node[i]);
      if (strict_roots.empty()) {
        ++truth.births;
        node_of_root[(std::size_t)group.first] = (int)next_node++;
      } else if (strict_roots.size() == 1) {
        ++truth.continuations;
        node_of_root[(std::size_t)group.first] = (int)*strict_roots.begin();
      } else {
        ++truth.fusions;
        node_of_root[(std::size_t)group.first] = (int)next_node++;
      }
    }

    // 5. LA PARTITION DE LA COUPE FERMEE : union des identifiants des faces de
    // chaque composante (theoreme 2 : c'est deja l'amas discret couvert).
    Partition partition;
    for (const auto& group : groups) {
      std::set<int> ids;
      for (std::size_t i : group.second)
        for (int z : faces[i]) ids.insert(z);
      partition.push_back(std::vector<int>(ids.begin(), ids.end()));
    }
    canonicalise(&partition);
    truth.levels.push_back(level);
    truth.partitions.push_back(std::move(partition));
  }
  truth.ok = true;
  return truth;
}

// ---------------------------------------------------------------------------
// LE SUJET : flat_catalogue + build_forest, lus comme des DONNEES.
// ---------------------------------------------------------------------------
Rational exact_level_of(const mhgp::Sphere& sphere) {
  const BigInt nx = BigInt::from_i128(sphere.nx);
  const BigInt ny = BigInt::from_i128(sphere.ny);
  const BigInt nz = BigInt::from_i128(sphere.nz);
  const BigInt den = BigInt::from_i128(sphere.den);
  return Rational(nx * nx + ny * ny + nz * nz, den * den);
}

struct SubjectForest {
  std::vector<Rational> node_level;
  std::vector<int> node_parent;
  std::vector<std::vector<int>> subtree_ids;   // union des membres du sous-arbre
  bool ok = false;
};

SubjectForest read_subject(const mhgp::Catalogue& catalogue, const mhgp::Forest& forest) {
  SubjectForest subject;
  const std::size_t count = forest.nodes.size();
  subject.node_level.resize(count);
  subject.node_parent.resize(count);
  subject.subtree_ids.resize(count);
  std::vector<std::set<int>> gather(count);
  for (std::size_t i = 0; i < count; ++i) {
    const mhgp::ForestNode& node = forest.nodes[i];
    if (node.source < 0 || node.source >= (int)catalogue.spheres.size()) return subject;
    const mhgp::CriticalSphere& sphere = catalogue.spheres[(std::size_t)node.source];
    if (sphere.members_begin < 0 ||
        (std::size_t)sphere.members_begin + (std::size_t)sphere.rank >
            catalogue.members.size())
      return subject;
    subject.node_level[i] = exact_level_of(sphere.sph);
    subject.node_parent[i] = node.parent;
    for (int t = 0; t < sphere.rank; ++t)
      gather[i].insert(catalogue.members[(std::size_t)(sphere.members_begin + t)]);
  }
  // Propager les membres vers TOUS les ancetres : dans un arbre de fusion, le
  // sous-arbre d'un noeud vivant est entierement au niveau <= le sien.
  for (std::size_t i = 0; i < count; ++i) {
    std::set<int> ids = gather[i];
    int a = subject.node_parent[i];
    long long steps = 0;
    while (a >= 0 && a < (int)count) {
      if (++steps > (long long)count) return subject;   // cycle : refus
      for (int z : ids) gather[(std::size_t)a].insert(z);
      a = subject.node_parent[(std::size_t)a];
    }
  }
  for (std::size_t i = 0; i < count; ++i)
    subject.subtree_ids[i] = std::vector<int>(gather[i].begin(), gather[i].end());
  subject.ok = true;
  return subject;
}

Partition subject_partition_at(const SubjectForest& subject, const Rational& level) {
  Partition partition;
  for (std::size_t i = 0; i < subject.node_level.size(); ++i) {
    if (compare(subject.node_level[i], level) > 0) continue;
    const int parent = subject.node_parent[i];
    const bool alive = parent < 0 || parent >= (int)subject.node_level.size() ||
                       compare(subject.node_level[(std::size_t)parent], level) > 0;
    if (alive) partition.push_back(subject.subtree_ids[i]);
  }
  canonicalise(&partition);
  return partition;
}

// Deux identifiants ensemble dans une partition, separes dans l'autre : faute
// de STRUCTURE. Memes regroupements sur les identifiants communs mais couverture
// differente : faute de COUVERTURE.
enum class Mismatch { kNone, kCoverage, kStructure };

Mismatch classify(const Partition& truth, const Partition& subject) {
  if (truth == subject) return Mismatch::kNone;
  std::map<int, int> truth_of, subject_of;
  for (std::size_t c = 0; c < truth.size(); ++c)
    for (int z : truth[c]) truth_of[z] = (int)c;
  for (std::size_t c = 0; c < subject.size(); ++c)
    for (int z : subject[c]) subject_of[z] = (int)c;
  // Les identifiants communs doivent etre regroupes pareil des deux cotes.
  std::map<std::pair<int, int>, int> pairing;
  for (const auto& entry : truth_of) {
    const auto it = subject_of.find(entry.first);
    if (it == subject_of.end()) continue;
    const std::pair<int, int> key{entry.second, it->second};
    pairing.emplace(key, entry.first);
  }
  std::map<int, int> forward_seen, backward_seen;
  for (const auto& entry : pairing) {
    const auto forward = forward_seen.emplace(entry.first.first, entry.first.second);
    if (!forward.second && forward.first->second != entry.first.second)
      return Mismatch::kStructure;
    const auto backward = backward_seen.emplace(entry.first.second, entry.first.first);
    if (!backward.second && backward.first->second != entry.first.first)
      return Mismatch::kStructure;
  }
  return Mismatch::kCoverage;
}

std::string partition_text(const Partition& partition) {
  std::string out = "{";
  for (const std::vector<int>& cluster : partition) {
    out += "{";
    for (std::size_t i = 0; i < cluster.size(); ++i) {
      if (i) out += ",";
      out += std::to_string(cluster[i]);
    }
    out += "}";
  }
  out += "}";
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  int clouds = 40, n = 8, coord = 5, smax = 11, max_order = 3;
  long long seed = 20260810;
  long long min_decided = 0, min_degenerate = 0, min_nondegenerate = 0, min_levels = 0;
  long long show_degenerate = 0;
  auto integer = [](const char* text, long long* value) {
    const char* first = text;
    const char* last = text + strlen(text);
    if (first == last) return false;
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(first, last, magnitude);
    if (r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 100000000ULL) return false;
    *value = (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    int* target = nullptr;
    long long* wide = nullptr;
    if (!strcmp(argv[i], "--clouds")) target = &clouds;
    else if (!strcmp(argv[i], "--points")) target = &n;
    else if (!strcmp(argv[i], "--coord")) target = &coord;
    else if (!strcmp(argv[i], "--smax")) target = &smax;
    else if (!strcmp(argv[i], "--max-order")) target = &max_order;
    else if (!strcmp(argv[i], "--seed")) wide = &seed;
    else if (!strcmp(argv[i], "--min-decided")) wide = &min_decided;
    else if (!strcmp(argv[i], "--min-degenerate")) wide = &min_degenerate;
    else if (!strcmp(argv[i], "--min-nondegenerate")) wide = &min_nondegenerate;
    else if (!strcmp(argv[i], "--min-levels")) wide = &min_levels;
    else if (!strcmp(argv[i], "--show-degenerate")) wide = &show_degenerate;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { std::printf("ECHEC : valeur entiere invalide pour %s\n", argv[i]); return 2; }
    ++i;
    if (wide != nullptr) *wide = value; else *target = (int)value;
  }
  // LES BORNES SONT CELLES DE L'EXHAUSTIF : la verite enumere C(n, K+1)
  // miniboules rationnelles ; au-dela le juge serait un four, pas une porte.
  if (clouds < 1 || clouds > 2000 || n < 4 || n > 14 || coord < 2 || coord > 65536 ||
      smax < 2 || smax > mhgp::kMaxRank || max_order < 1 || max_order > 6 ||
      max_order + 1 > smax) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }

  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);

  long long decided = 0, degenerate_clouds = 0, nondegenerate_clouds = 0;
  long long levels_compared = 0, failures = 0;
  long long clean_orders = 0, coverage_orders = 0, structure_orders = 0;
  long long degenerate_clean = 0, degenerate_coverage = 0, degenerate_structure = 0;
  long long rank_censored_orders = 0, foreign_levels = 0;
  long long truth_births = 0, truth_fusions = 0, truth_continuations = 0;

  for (int c = 0; c < clouds; ++c) {
    std::vector<mhgp::P3> pts;
    {
      std::set<long long> keys;
      for (int guard = 0; (int)pts.size() < n && guard < 200 * n; ++guard) {
        mhgp::P3 q{};
        q.x = (mhgp::i32)pick(rng);
        q.y = (mhgp::i32)pick(rng);
        q.z = (mhgp::i32)pick(rng);
        const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
        if (!keys.insert(key).second) continue;
        pts.push_back(q);
      }
    }
    if ((int)pts.size() < n) { std::printf("ECHEC : nuage %d non genere\n", c); return 3; }

    mhgp3v::FlatStatistics st{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    const mhgp::Catalogue catalogue = mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);

    bool cloud_degenerate = false;
    for (int k = 1; k <= max_order; ++k) {
      const GammaTruth truth = gamma_truth(pts, k);
      if (!truth.ok) {
        std::printf("[nuage %d ordre %d] la verite Gamma n'a pas pu etre construite\n", c, k);
        ++failures;
        continue;
      }
      truth_births += truth.births;
      truth_fusions += truth.fusions;
      truth_continuations += truth.continuations;
      if (truth.degenerate_events) cloud_degenerate = true;
      const bool rank_censored = truth.maximum_relevant_rank > smax;
      if (rank_censored) ++rank_censored_orders;

      const mhgp::Forest forest = mhgp::build_forest(pts, catalogue, k);
      const SubjectForest subject = read_subject(catalogue, forest);
      if (!subject.ok) {
        std::printf("[nuage %d ordre %d] foret sujet illisible (source ou tranche hors"
                    " catalogue, ou cycle)\n", c, k);
        ++failures;
        continue;
      }
      // Tout niveau de noeud sujet doit etre un niveau d'evenement de la verite.
      for (const Rational& node_level : subject.node_level) {
        bool known = false;
        for (const Rational& level : truth.levels)
          if (compare(level, node_level) == 0) { known = true; break; }
        if (!known) ++foreign_levels;
      }

      Mismatch worst = Mismatch::kNone;
      const Rational* first_level = nullptr;
      for (std::size_t li = 0; li < truth.levels.size(); ++li) {
        ++levels_compared;
        const Partition subject_partition = subject_partition_at(subject, truth.levels[li]);
        const Mismatch verdict = classify(truth.partitions[li], subject_partition);
        if (verdict != Mismatch::kNone && worst == Mismatch::kNone)
          first_level = &truth.levels[li];
        if (verdict == Mismatch::kStructure) worst = Mismatch::kStructure;
        else if (verdict == Mismatch::kCoverage && worst == Mismatch::kNone)
          worst = Mismatch::kCoverage;
      }

      const bool degenerate = truth.degenerate_events || rank_censored;
      if (worst == Mismatch::kNone) {
        ++(degenerate ? degenerate_clean : clean_orders);
      } else if (worst == Mismatch::kCoverage) {
        ++(degenerate ? degenerate_coverage : coverage_orders);
      } else {
        ++(degenerate ? degenerate_structure : structure_orders);
      }
      // SUR LES NUAGES SANS EVENEMENT DEGENERE NI CENSURE DE RANG, L'ACCORD EST
      // EXIGE : c'est le domaine ou la position generale aurait tenu, et ou la
      // chaine pretend l'exactitude.
      if (degenerate && worst != Mismatch::kNone && show_degenerate > 0) {
        --show_degenerate;
        std::size_t li = 0;
        for (; li < truth.levels.size(); ++li)
          if (compare(truth.levels[li], *first_level) == 0) break;
        std::printf("[nuage %d ordre %d] divergence %s SOUS degenerescence, premier niveau"
                    " fautif #%zu (rang max pertinent %d)\n  verite : %s\n  sujet  : %s\n",
                    c, k, worst == Mismatch::kStructure ? "STRUCTURE" : "COUVERTURE", li,
                    truth.maximum_relevant_rank,
                    partition_text(truth.partitions[li]).c_str(),
                    partition_text(subject_partition_at(subject, truth.levels[li])).c_str());
      }
      if (!degenerate && worst != Mismatch::kNone) {
        std::size_t li = 0;
        for (; li < truth.levels.size(); ++li)
          if (compare(truth.levels[li], *first_level) == 0) break;
        std::printf("[nuage %d ordre %d] DESACCORD %s hors degenerescence, premier niveau"
                    " fautif #%zu\n  verite : %s\n  sujet  : %s\n", c, k,
                    worst == Mismatch::kStructure ? "STRUCTURE" : "COUVERTURE", li,
                    partition_text(truth.partitions[li]).c_str(),
                    partition_text(subject_partition_at(subject, truth.levels[li])).c_str());
        ++failures;
      }
    }
    if (cloud_degenerate) ++degenerate_clouds; else ++nondegenerate_clouds;
    ++decided;
  }

  std::printf("provenance : --clouds %d --points %d --coord %d --smax %d --max-order %d"
              " --seed %lld\n", clouds, n, coord, smax, max_order, seed);
  std::printf("nuages     : decides=%lld  degeneres=%lld  non degeneres=%lld\n", decided,
              degenerate_clouds, nondegenerate_clouds);
  std::printf("verite     : naissances=%lld fusions=%lld continuations=%lld  niveaux"
              " compares=%lld\n", truth_births, truth_fusions, truth_continuations,
              levels_compared);
  std::printf("ordres     : hors degenerescence — accord=%lld couverture=%lld"
              " structure=%lld\n", clean_orders, coverage_orders, structure_orders);
  std::printf("           : degeneres ou censures — accord=%lld couverture=%lld"
              " structure=%lld  (censures de rang=%lld, niveaux etrangers=%lld)\n",
              degenerate_clean, degenerate_coverage, degenerate_structure,
              rank_censored_orders, foreign_levels);
  std::printf("           : la carte des degeneres MESURE la frontiere Q1 ; elle n'est"
              " pas un desaccord du sujet sur son domaine declare\n");

  struct Floor { const char* name; long long value; long long required; };
  const Floor floors[] = {
      {"nuages decides", decided, min_decided},
      {"nuages degeneres", degenerate_clouds, min_degenerate},
      {"nuages non degeneres", nondegenerate_clouds, min_nondegenerate},
      {"niveaux compares", levels_compared, min_levels},
  };
  for (const Floor& floor : floors)
    if (floor.value < floor.required) {
      std::printf("ECHEC : plancher « %s » non atteint — %lld/%lld\n", floor.name,
                  floor.value, floor.required);
      return 3;
    }
  if (failures != 0) {
    std::printf("\n%lld desaccords hors degenerescence\n", failures);
    return 1;
  }
  std::printf("\nOK : sur les ordres sans evenement degenere ni censure de rang, la chaine"
              " catalogue+foret rend EXACTEMENT les partitions de Gamma_k a chaque niveau"
              " d'evenement, aux deux coupes\n");
  return 0;
}
