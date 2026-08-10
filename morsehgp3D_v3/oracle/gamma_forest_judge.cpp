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
#include <climits>
#include <cstdio>
#include <cstring>
#include <functional>
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
#include "prototype/saturated_fold.hpp"    // le SUJET CANDIDAT : fold sature

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
  // DEUX PROJECTIONS SEPAREES, exigees par l'audit live : la composante
  // canonique est la liste triee de ses k-FACETTES (les facettes sont
  // disjointes entre composantes), la couverture est l'union triee des
  // identifiants (les couvertures PEUVENT se chevaucher pour k >= 2 — aucune
  // map point->composante n'est licite). Le juge de la foret compare la
  // seconde ; la premiere attend le journal d'incidences.
  std::vector<std::vector<std::vector<std::vector<int>>>> facet_partitions;
  std::vector<Partition> partitions;       // couvertures a la coupe FERMEE de chaque niveau
  // Les types d'evenement PAR NIVEAU, alignes sur `levels` — le payload que la
  // reception du transcript compare au fold marque par q_min.
  std::vector<long long> births_at_level, continuations_at_level, fusions_at_level;
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
    long long births_here = 0, continuations_here = 0, fusions_here = 0;
    for (const auto& group : groups) {
      if (changed_roots.find(group.first) == changed_roots.end()) continue;
      std::set<long long> strict_roots;
      for (std::size_t i : group.second)
        if (strict_node[i] >= 0) strict_roots.insert(strict_node[i]);
      if (strict_roots.empty()) {
        ++truth.births;
        ++births_here;
        node_of_root[(std::size_t)group.first] = (int)next_node++;
      } else if (strict_roots.size() == 1) {
        ++truth.continuations;
        ++continuations_here;
        node_of_root[(std::size_t)group.first] = (int)*strict_roots.begin();
      } else {
        ++truth.fusions;
        ++fusions_here;
        node_of_root[(std::size_t)group.first] = (int)next_node++;
      }
    }
    truth.births_at_level.push_back(births_here);
    truth.continuations_at_level.push_back(continuations_here);
    truth.fusions_at_level.push_back(fusions_here);

    // 5. LA COUPE FERMEE, sous ses deux projections : composantes de facettes
    // (canoniques, disjointes) et couvertures (theoreme 2 : l'union des
    // identifiants est deja l'amas discret couvert).
    Partition partition;
    std::vector<std::vector<std::vector<int>>> facet_partition;
    for (const auto& group : groups) {
      std::set<int> ids;
      std::vector<std::vector<int>> component;
      for (std::size_t i : group.second) {
        for (int z : faces[i]) ids.insert(z);
        component.push_back(faces[i]);
      }
      std::sort(component.begin(), component.end());
      facet_partition.push_back(std::move(component));
      partition.push_back(std::vector<int>(ids.begin(), ids.end()));
    }
    canonicalise(&partition);
    std::sort(facet_partition.begin(), facet_partition.end());
    truth.levels.push_back(level);
    truth.partitions.push_back(std::move(partition));
    truth.facet_partitions.push_back(std::move(facet_partition));
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
  const char* refusal = "";
};

SubjectForest read_subject(const mhgp::Catalogue& catalogue, const mhgp::Forest& forest) {
  SubjectForest subject;
  const std::size_t count = forest.nodes.size();
  subject.node_level.resize(count);
  subject.node_parent.resize(count);
  subject.subtree_ids.resize(count);
  std::vector<std::set<int>> gather(count);
  // FAIL-CLOSED SUR LES AUTORITES DU SUJET (audit live) : une foret non
  // autoritative est un payload censure, jamais une foret complete ; un parent
  // hors plage n'est pas une racine vivante, c'est un refus.
  if (!forest.authoritative) { subject.refusal = "foret non autoritative"; return subject; }
  for (std::size_t i = 0; i < count; ++i) {
    const mhgp::ForestNode& node = forest.nodes[i];
    if (node.parent < -1 || node.parent >= (int)count) {
      subject.refusal = "parent hors plage";
      return subject;
    }
    if (node.source < 0 || node.source >= (int)catalogue.spheres.size()) {
      subject.refusal = "source hors catalogue";
      return subject;
    }
    const mhgp::CriticalSphere& sphere = catalogue.spheres[(std::size_t)node.source];
    if (sphere.members_begin < 0 ||
        (std::size_t)sphere.members_begin + (std::size_t)sphere.rank >
            catalogue.members.size()) {
      subject.refusal = "tranche de pool hors catalogue";
      return subject;
    }
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
      if (++steps > (long long)count) { subject.refusal = "cycle de parents"; return subject; }
      for (int z : ids) gather[(std::size_t)a].insert(z);
      a = subject.node_parent[(std::size_t)a];
    }
  }
  for (std::size_t i = 0; i < count; ++i)
    subject.subtree_ids[i] = std::vector<int>(gather[i].begin(), gather[i].end());
  subject.ok = true;
  return subject;
}

Partition subject_partition_at(const SubjectForest& subject, const Rational& level,
                               bool strict) {
  Partition partition;
  for (std::size_t i = 0; i < subject.node_level.size(); ++i) {
    const int mine = compare(subject.node_level[i], level);
    if (strict ? mine >= 0 : mine > 0) continue;
    const int parent = subject.node_parent[i];
    bool alive = parent < 0;
    if (!alive) {
      const int his = compare(subject.node_level[(std::size_t)parent], level);
      alive = strict ? his >= 0 : his > 0;
    }
    if (alive) partition.push_back(subject.subtree_ids[i]);
  }
  canonicalise(&partition);
  return partition;
}

// La verite a une coupe quelconque se lit dans ses niveaux d'evenement : la
// coupe fermee en a est la coupe fermee du plus grand niveau <= a, la coupe
// stricte celle du plus grand niveau < a. Rien ne change entre deux niveaux.
const Partition* truth_partition_at(const GammaTruth& truth, const Rational& level,
                                    bool strict) {
  static const Partition empty;
  const Partition* found = &empty;
  for (std::size_t li = 0; li < truth.levels.size(); ++li) {
    const int c = compare(truth.levels[li], level);
    if (strict ? c < 0 : c <= 0) found = &truth.partitions[li];
    else break;
  }
  return found;
}

// PAS DE map<PointId, composante> : les couvertures HGP peuvent se chevaucher
// pour k >= 2, l'appartenance unique est une hypothese fausse (audit live). La
// comparaison est l'egalite des collections canoniques ; la distinction
// compte/contenu est DESCRIPTIVE, jamais une preuve de structure.
enum class Mismatch { kNone, kCount, kContent };

Mismatch classify(const Partition& truth, const Partition& subject) {
  if (truth == subject) return Mismatch::kNone;
  return truth.size() != subject.size() ? Mismatch::kCount : Mismatch::kContent;
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
  long long min_judged = 0;
  long long show_degenerate = 0;
  int shift_level = 0;   // mutant : decale un noeud sujet vers un niveau etranger
  int subject_fold = 0;  // 0 = chaine build_forest, 1 = fold sature (S.4/S.5)
  int require_degenerate = 0;   // exiger l'accord AUSSI sur les ordres degeneres
  // LE PREDICAT STRUCTUREL D'EVENEMENT GAMMA (theoremes 1--2 de la note
  // NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN) : un niveau du sature est un vrai
  // niveau Gamma_k ssi son lot contient un generateur avec |M| >= k et
  // q_min(B) <= k+1, ou q_min est la PLUS PETITE taille d'un sous-ensemble du
  // sature dont la miniboule est B — calculee ICI par enumeration exacte de
  // l'oracle, jamais lue du produit (`n_support` coinciderait sur ce chemin :
  // un tel mutant serait une porte morte, l'audit 621ee80 demande le DECALAGE
  // q_min+1).
  int check_predicate = 0;
  int force_qmin_shift = 0;
  long long min_event_levels = 0;
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
    else if (!strcmp(argv[i], "--min-judged")) wide = &min_judged;
    else if (!strcmp(argv[i], "--show-degenerate")) wide = &show_degenerate;
    else if (!strcmp(argv[i], "--force-shift-level")) target = &shift_level;
    else if (!strcmp(argv[i], "--subject-fold")) target = &subject_fold;
    else if (!strcmp(argv[i], "--require-degenerate-agreement")) target = &require_degenerate;
    else if (!strcmp(argv[i], "--check-event-predicate")) target = &check_predicate;
    else if (!strcmp(argv[i], "--force-qmin-shift")) target = &force_qmin_shift;
    else if (!strcmp(argv[i], "--min-event-levels")) wide = &min_event_levels;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { std::printf("ECHEC : valeur entiere invalide pour %s\n", argv[i]); return 2; }
    ++i;
    if (wide != nullptr) *wide = value; else *target = (int)value;
  }
  // LES BORNES SONT CELLES DE L'EXHAUSTIF : la verite enumere C(n, K+1)
  // miniboules rationnelles ; au-dela le juge serait un four, pas une porte.
  if (clouds < 1 || clouds > 2000 || n < 4 || n > 14 || coord < 2 || coord > 65536 ||
      smax < 2 || smax > mhgp::kMaxRank || max_order < 1 || max_order > 6 ||
      max_order + 1 > smax || shift_level < 0 || shift_level > 1 || subject_fold < 0 ||
      subject_fold > 1 || require_degenerate < 0 || require_degenerate > 1) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  // Le mutant de niveau appartient au sujet chaine ; le fold n'a pas de noeud a
  // decaler, la combinaison serait une porte morte.
  if (subject_fold == 1 && shift_level == 1) {
    std::printf("ECHEC : --force-shift-level n'a de sens que pour le sujet chaine\n");
    return 2;
  }
  if (check_predicate < 0 || check_predicate > 1 || force_qmin_shift < 0 ||
      force_qmin_shift > 1) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  // Le predicat lit les lots du fold sature ; sans lui il n'y a pas de lot.
  if (check_predicate == 1 && subject_fold == 0) {
    std::printf("ECHEC : --check-event-predicate exige --subject-fold 1\n");
    return 2;
  }
  if (force_qmin_shift == 1 && check_predicate == 0) {
    std::printf("ECHEC : --force-qmin-shift exige --check-event-predicate 1\n");
    return 2;
  }

  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);

  long long decided = 0, degenerate_clouds = 0, nondegenerate_clouds = 0;
  long long levels_compared = 0, failures = 0;
  long long clean_orders = 0, coverage_orders = 0, structure_orders = 0;
  long long degenerate_clean = 0, degenerate_coverage = 0, degenerate_structure = 0;
  long long rank_censored_orders = 0, foreign_levels = 0, refused_subject = 0;
  long long refused_orders = 0, judged_orders = 0;
  long long truth_births = 0, truth_fusions = 0, truth_continuations = 0;
  long long predicate_orders_agreed = 0, predicate_mismatch_clean = 0;
  long long predicate_mismatch_degenerate = 0, predicate_event_levels = 0;
  long long qmin_subset_failures = 0, provenance_mismatches = 0;
  long long transcript_orders_agreed = 0, transcript_mismatch_clean = 0;
  long long transcript_mismatch_degenerate = 0;

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
    // UN PAYLOAD CENSURE N'EST PAS UNE FORET COMPLETE : un statut non kOk sort
    // le nuage du denominateur, avec sa categorie (audit live).
    if (status != mhgp3v::CloudStatus::kOk) {
      std::printf("  nuage %d : statut sujet %s, refuse avant comparaison\n", c,
                  mhgp3v::cloud_status_name(status));
      ++refused_subject;
      continue;
    }

    mhgp3v::SaturatedFold fold;
    if (subject_fold == 1) {
      // La garde d'evenement du fold ne refuse que sous famille complete.
      fold = mhgp3v::build_saturated_fold(catalogue, max_order, true,
                                          /*enforce_event_guard=*/smax >= n);
      if (!fold.ok) {
        std::printf("[nuage %d] fold sature refuse : %s\n", c, fold.refusal);
        ++refused_subject;
        continue;
      }
    }

    // q_min PAR GENERATEUR, une fois par nuage : la plus petite taille d'un
    // sous-ensemble du sature dont la miniboule est EXACTEMENT la boule du
    // generateur (centre et rayon rationnels), agregee sur TOUTES les
    // provenances par enumeration bornee a max_order+1 — jamais la taille du
    // support canonique, qu'une boule degeneree peut depasser (note §5). Le
    // predicat n'a besoin que de q_min <= k+1 <= max_order+1.
    std::vector<int> qmin_cap;
    if (check_predicate == 1) {
      qmin_cap.assign(catalogue.spheres.size(), INT_MAX);
      for (std::size_t s = 0; s < catalogue.spheres.size(); ++s) {
        const mhgp::CriticalSphere& sphere = catalogue.spheres[s];
        const Rational target_radius = exact_level_of(sphere.sph);
        const Rational target_x =
            Rational(BigInt(static_cast<long long>(sphere.sph.base.x))) +
            Rational(BigInt::from_i128(sphere.sph.nx), BigInt::from_i128(sphere.sph.den));
        const Rational target_y =
            Rational(BigInt(static_cast<long long>(sphere.sph.base.y))) +
            Rational(BigInt::from_i128(sphere.sph.ny), BigInt::from_i128(sphere.sph.den));
        const Rational target_z =
            Rational(BigInt(static_cast<long long>(sphere.sph.base.z))) +
            Rational(BigInt::from_i128(sphere.sph.nz), BigInt::from_i128(sphere.sph.den));
        std::vector<int> generator_members;
        for (int t = 0; t < sphere.rank; ++t)
          generator_members.push_back(
              (int)catalogue.members[(std::size_t)(sphere.members_begin + t)]);
        const int cap = std::min(max_order + 1, (int)generator_members.size());
        std::vector<int> subset;
        bool found = false;
        std::function<void(std::size_t, int)> descend = [&](std::size_t from, int need) {
          if (found) return;
          if (need == 0) {
            RationalSphere ball;
            std::vector<int> support;
            if (!exact_miniball(pts, subset, &ball, &support)) {
              ++qmin_subset_failures;
              return;
            }
            if (compare(ball.squared_radius, target_radius) == 0 &&
                compare(ball.centre.x, target_x) == 0 &&
                compare(ball.centre.y, target_y) == 0 &&
                compare(ball.centre.z, target_z) == 0)
              found = true;
            return;
          }
          for (std::size_t i = from; i + (std::size_t)need <= generator_members.size(); ++i) {
            subset.push_back(generator_members[i]);
            descend(i + 1, need - 1);
            subset.pop_back();
            if (found) return;
          }
        };
        for (int size = 1; size <= cap && !found; ++size) {
          descend(0, size);
          if (found) qmin_cap[s] = size;
        }
        // MUTANT force_qmin_shift : decaler q_min d'une unite doit etre vu par
        // la reception (des niveaux verite perdent leur generateur d'evenement,
        // ou des generateurs q=k+2 entrent a tort).
        if (force_qmin_shift == 1 && qmin_cap[s] != INT_MAX) ++qmin_cap[s];
        // CERTIFICATION DE PROVENANCE : le fold lit q_min dans n_support ; le
        // juge exige que le support canonique du produit soit EXACTEMENT la
        // cardinalite minimale enumeree — sinon le transcript marque est faux.
        const int enumeration_cap = std::min(max_order + 1, (int)generator_members.size());
        const bool coherent = qmin_cap[s] != INT_MAX
                                  ? (int)sphere.n_support == qmin_cap[s]
                                  : (int)sphere.n_support > enumeration_cap;
        if (!coherent) {
          ++provenance_mismatches;
          ++failures;
          if (provenance_mismatches <= 3)
            std::printf("[nuage %d] PROVENANCE REFUTEE : n_support=%d mais q_min"
                        " enumere=%d (generateur %zu, rang %d)\n", c,
                        (int)sphere.n_support,
                        qmin_cap[s] == INT_MAX ? -1 : qmin_cap[s], s, (int)sphere.rank);
        }
      }
    }

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

      // RECEPTION DU PREDICAT STRUCTUREL : les niveaux predits — lots du
      // catalogue contenant un generateur avec |M| >= k et q_min <= k+1 —
      // doivent etre EXACTEMENT les niveaux d'evenement de la verite Gamma_k.
      // Un ecart dans un sens (niveau predit etranger) ou l'autre (niveau
      // verite sans generateur d'evenement) refute le lemme sur ce nuage.
      if (check_predicate == 1) {
        std::vector<Rational> predicted;
        for (std::size_t s = 0; s < catalogue.spheres.size(); ++s) {
          if ((int)catalogue.spheres[s].rank < k) continue;
          if (qmin_cap[s] > k + 1) continue;
          predicted.push_back(exact_level_of(catalogue.spheres[s].sph));
        }
        std::sort(predicted.begin(), predicted.end(),
                  [](const Rational& a, const Rational& b) { return compare(a, b) < 0; });
        predicted.erase(std::unique(predicted.begin(), predicted.end(),
                                    [](const Rational& a, const Rational& b) {
                                      return compare(a, b) == 0;
                                    }),
                        predicted.end());
        predicate_event_levels += (long long)predicted.size();
        const Rational* foreign_predicted = nullptr;
        const Rational* orphan_truth = nullptr;
        for (const Rational& level : predicted) {
          bool known = false;
          for (const Rational& truth_level : truth.levels)
            if (compare(level, truth_level) == 0) { known = true; break; }
          if (!known) { foreign_predicted = &level; break; }
        }
        for (const Rational& truth_level : truth.levels) {
          bool covered = false;
          for (const Rational& level : predicted)
            if (compare(level, truth_level) == 0) { covered = true; break; }
          if (!covered) { orphan_truth = &truth_level; break; }
        }
        const bool degenerate_here = truth.degenerate_events || rank_censored;
        if (foreign_predicted == nullptr && orphan_truth == nullptr) {
          ++predicate_orders_agreed;
        } else if (degenerate_here && require_degenerate == 0) {
          ++predicate_mismatch_degenerate;
        } else {
          ++predicate_mismatch_clean;
          ++failures;
          if (foreign_predicted != nullptr)
            std::printf("[nuage %d ordre %d] PREDICAT REFUTE : niveau predit %s/%s sans"
                        " evenement Gamma (%zu niveaux verite)\n", c, k,
                        foreign_predicted->numerator().to_decimal().c_str(),
                        foreign_predicted->denominator().to_decimal().c_str(),
                        truth.levels.size());
          if (orphan_truth != nullptr)
            std::printf("[nuage %d ordre %d] PREDICAT REFUTE : niveau Gamma %s/%s sans"
                        " generateur d'evenement (|M|>=%d, q_min<=%d)\n", c, k,
                        orphan_truth->numerator().to_decimal().c_str(),
                        orphan_truth->denominator().to_decimal().c_str(), k, k + 1);
        }
      }

      // LE SUJET DE CET ORDRE : la chaine v2 (build_forest) ou le fold sature.
      // Le fold expose directement ses partitions par niveau ; la chaine passe
      // par la reconstruction de foret.
      std::vector<Rational> fold_levels;
      std::vector<Partition> fold_partitions;
      if (subject_fold == 1) {
        const mhgp3v::SaturatedOrderFold& order_fold = fold.orders[(std::size_t)(k - 1)];
        for (std::size_t li = 0; li < order_fold.level_representative.size(); ++li) {
          fold_levels.push_back(exact_level_of(
              catalogue.spheres[(std::size_t)order_fold.level_representative[li]].sph));
          Partition partition;
          for (const std::vector<mhgp::i32>& cluster : order_fold.closed_partitions[li])
            partition.push_back(std::vector<int>(cluster.begin(), cluster.end()));
          canonicalise(&partition);
          fold_partitions.push_back(std::move(partition));
        }
      }
      const auto fold_partition_at = [&](const Rational& level, bool strict) {
        static const Partition empty;
        const Partition* found = &empty;
        for (std::size_t li = 0; li < fold_levels.size(); ++li) {
          const int cmp = compare(fold_levels[li], level);
          if (strict ? cmp < 0 : cmp <= 0) found = &fold_partitions[li];
          else break;
        }
        return *found;
      };

      // RECEPTION DU TRANSCRIPT MARQUE : les types d'evenement du fold
      // (naissance/continuation/multifusion PAR COMPOSANTE, racines marquees
      // par q_min) doivent egaler ceux de la verite A CHAQUE niveau, et un
      // niveau du fold etranger a la verite doit porter un triple nul.
      if (check_predicate == 1 && subject_fold == 1) {
        const mhgp3v::SaturatedOrderFold& order_fold = fold.orders[(std::size_t)(k - 1)];
        const char* transcript_witness = nullptr;
        std::size_t witness_level = 0;
        for (std::size_t li = 0; li < truth.levels.size() && transcript_witness == nullptr;
             ++li) {
          long long fold_births = 0, fold_continuations = 0, fold_multifusions = 0;
          for (std::size_t fi = 0; fi < fold_levels.size(); ++fi)
            if (compare(fold_levels[fi], truth.levels[li]) == 0) {
              fold_births = order_fold.gamma_birth_at_level[fi];
              fold_continuations = order_fold.gamma_continuation_at_level[fi];
              fold_multifusions = order_fold.gamma_multifusion_at_level[fi];
              break;
            }
          if (fold_births != truth.births_at_level[li] ||
              fold_continuations != truth.continuations_at_level[li] ||
              fold_multifusions != truth.fusions_at_level[li]) {
            transcript_witness = "types divergents au niveau verite";
            witness_level = li;
          }
        }
        for (std::size_t fi = 0; fi < fold_levels.size() && transcript_witness == nullptr;
             ++fi) {
          bool known = false;
          for (const Rational& truth_level : truth.levels)
            if (compare(fold_levels[fi], truth_level) == 0) { known = true; break; }
          if (!known && (order_fold.gamma_birth_at_level[fi] != 0 ||
                         order_fold.gamma_continuation_at_level[fi] != 0 ||
                         order_fold.gamma_multifusion_at_level[fi] != 0)) {
            transcript_witness = "triple non nul a un niveau etranger a la verite";
            witness_level = fi;
          }
        }
        const bool degenerate_here = truth.degenerate_events || rank_censored;
        if (transcript_witness == nullptr) {
          ++transcript_orders_agreed;
        } else if (degenerate_here && require_degenerate == 0) {
          ++transcript_mismatch_degenerate;
        } else {
          ++transcript_mismatch_clean;
          ++failures;
          std::printf("[nuage %d ordre %d] TRANSCRIPT REFUTE : %s (indice %zu)\n", c, k,
                      transcript_witness, witness_level);
        }
      }

      const mhgp::Forest forest =
          subject_fold == 1 ? mhgp::Forest{} : mhgp::build_forest(pts, catalogue, k);
      SubjectForest subject;
      if (subject_fold == 0) subject = read_subject(catalogue, forest);
      if (subject_fold == 0 && !subject.ok) {
        // LE SUJET A CENSURE (foret non autoritative, source ou parent hors
        // plage) : un payload censure ne se compare pas comme une foret
        // complete, il sort du denominateur AVEC sa categorie — jamais compte
        // comme un desaccord, jamais comme un accord.
        std::printf("[nuage %d ordre %d] foret sujet refusee : %s\n", c, k,
                    subject.refusal[0] ? subject.refusal : "illisible");
        ++refused_orders;
        continue;
      }
      ++judged_orders;
      // LE MUTANT DE NIVEAU DEPLACE : il pousse le premier noeud de niveau
      // positif vers un niveau ETRANGER (le double du sien). Seule une
      // comparaison sur l'UNION des niveaux, aux deux coupes, peut le voir —
      // c'etait le trou releve par l'audit live (echantillonnage aux seuls
      // niveaux de la verite).
      if (shift_level == 1)
        for (std::size_t i = 0; i < subject.node_level.size(); ++i)
          if (subject.node_level[i].sign() > 0) {
            subject.node_level[i] =
                subject.node_level[i] + subject.node_level[i];
            break;
          }

      // L'UNION DES NIVEAUX de la verite ET du sujet : un niveau sujet
      // etranger devient une coupe TESTEE, pas un compteur diagnostique.
      std::vector<Rational> cut_levels = truth.levels;
      const std::vector<Rational>& subject_levels =
          subject_fold == 1 ? fold_levels : subject.node_level;
      for (const Rational& node_level : subject_levels) {
        bool known = false;
        for (const Rational& level : truth.levels)
          if (compare(level, node_level) == 0) { known = true; break; }
        if (!known) {
          ++foreign_levels;
          cut_levels.push_back(node_level);
        }
      }
      std::sort(cut_levels.begin(), cut_levels.end(),
                [](const Rational& a, const Rational& b) { return compare(a, b) < 0; });
      cut_levels.erase(std::unique(cut_levels.begin(), cut_levels.end(),
                                   [](const Rational& a, const Rational& b) {
                                     return compare(a, b) == 0;
                                   }),
                       cut_levels.end());

      Mismatch worst = Mismatch::kNone;
      bool worst_strict = false;
      const Rational* first_level = nullptr;
      for (std::size_t li = 0; li < cut_levels.size(); ++li) {
        ++levels_compared;
        for (int strict = 0; strict < 2; ++strict) {
          const Partition subject_partition =
              subject_fold == 1 ? fold_partition_at(cut_levels[li], strict != 0)
                                : subject_partition_at(subject, cut_levels[li], strict != 0);
          const Partition* truth_partition =
              truth_partition_at(truth, cut_levels[li], strict != 0);
          const Mismatch verdict = classify(*truth_partition, subject_partition);
          if (verdict != Mismatch::kNone && worst == Mismatch::kNone) {
            first_level = &cut_levels[li];
            worst_strict = strict != 0;
          }
          if (verdict == Mismatch::kCount) worst = Mismatch::kCount;
          else if (verdict == Mismatch::kContent && worst == Mismatch::kNone)
            worst = Mismatch::kContent;
        }
      }

      const bool degenerate = truth.degenerate_events || rank_censored;
      if (worst == Mismatch::kNone) {
        ++(degenerate ? degenerate_clean : clean_orders);
      } else if (worst == Mismatch::kContent) {
        ++(degenerate ? degenerate_coverage : coverage_orders);
      } else {
        ++(degenerate ? degenerate_structure : structure_orders);
      }
      // SUR LES NUAGES SANS EVENEMENT DEGENERE NI CENSURE DE RANG, L'ACCORD EST
      // EXIGE : c'est le domaine ou la position generale aurait tenu, et ou la
      // chaine pretend l'exactitude. Les etiquettes compte/contenu sont
      // DESCRIPTIVES (audit live), jamais une preuve de structure.
      const auto dump = [&](const char* label) {
        std::printf("[nuage %d ordre %d] %s a la coupe %s du niveau montre (rang max"
                    " pertinent %d)\n  verite : %s\n  sujet  : %s\n", c, k, label,
                    worst_strict ? "STRICTE" : "FERMEE", truth.maximum_relevant_rank,
                    partition_text(*truth_partition_at(truth, *first_level, worst_strict))
                        .c_str(),
                    partition_text(subject_fold == 1
                                       ? fold_partition_at(*first_level, worst_strict)
                                       : subject_partition_at(subject, *first_level,
                                                              worst_strict))
                        .c_str());
      };
      if (degenerate && worst != Mismatch::kNone && show_degenerate > 0) {
        --show_degenerate;
        dump(worst == Mismatch::kCount ? "divergence de COMPTE sous degenerescence"
                                       : "divergence de CONTENU sous degenerescence");
      }
      // LE FOLD SATURE PRETEND L'EXACTITUDE SUR LE REGIME DEGENERE AUSSI :
      // c'est tout son objet. Sous ce drapeau, une divergence degeneree est un
      // DESACCORD, plus une carte.
      if (require_degenerate == 1 && degenerate && worst != Mismatch::kNone) {
        dump(worst == Mismatch::kCount ? "DESACCORD de COMPTE sous degenerescence"
                                       : "DESACCORD de CONTENU sous degenerescence");
        ++failures;
      }
      if (!degenerate && worst != Mismatch::kNone) {
        dump(worst == Mismatch::kCount ? "DESACCORD de COMPTE hors degenerescence"
                                       : "DESACCORD de CONTENU hors degenerescence");
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
  if (refused_subject > 0)
    std::printf("refus      : %lld nuage(s) au statut sujet non kOk, hors du denominateur\n",
                refused_subject);
  std::printf("ordres     : juges=%lld  refuses par censure du sujet=%lld\n", judged_orders,
              refused_orders);
  std::printf("           : hors degenerescence — accord=%lld ecart de contenu=%lld"
              " ecart de compte=%lld\n", clean_orders, coverage_orders, structure_orders);
  std::printf("           : degeneres ou censures — accord=%lld contenu=%lld"
              " compte=%lld  (censures de rang=%lld, niveaux etrangers testes=%lld)\n",
              degenerate_clean, degenerate_coverage, degenerate_structure,
              rank_censored_orders, foreign_levels);
  std::printf("           : la carte des degeneres MESURE la frontiere Q1 ; elle n'est"
              " pas un desaccord du sujet sur son domaine declare, et ses etiquettes"
              " compte/contenu sont descriptives\n");
  if (check_predicate == 1) {
    std::printf("predicat   : ordres en accord=%lld  refutations hors degenerescence=%lld"
                "  ecarts degeneres (carte)=%lld  niveaux d'evenement predits=%lld"
                "  echecs miniboule de sous-ensemble=%lld\n",
                predicate_orders_agreed, predicate_mismatch_clean,
                predicate_mismatch_degenerate, predicate_event_levels,
                qmin_subset_failures);
    std::printf("transcript : ordres en accord=%lld  refutations hors degenerescence=%lld"
                "  ecarts degeneres (carte)=%lld  provenances n_support!=q_min=%lld\n",
                transcript_orders_agreed, transcript_mismatch_clean,
                transcript_mismatch_degenerate, provenance_mismatches);
  }

  struct Floor { const char* name; long long value; long long required; };
  const Floor floors[] = {
      {"nuages decides", decided, min_decided},
      {"nuages degeneres", degenerate_clouds, min_degenerate},
      {"nuages non degeneres", nondegenerate_clouds, min_nondegenerate},
      {"niveaux compares", levels_compared, min_levels},
      {"ordres juges", judged_orders, min_judged},
      {"niveaux d'evenement du predicat", predicate_event_levels, min_event_levels},
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
  // LES DEUX OBJECTIFS NE PARTAGENT PAS UN MEME OK (audit live) : une campagne
  // sans aucun ordre hors degenerescence est une CARTE, pas une porte
  // d'exactitude, et elle le dit.
  if (clean_orders + coverage_orders + structure_orders == 0)
    std::printf("\nCARTE DE FRONTIERE : aucun ordre hors degenerescence dans cette"
                " campagne ; les couvertures ci-dessus decrivent le regime"
                " multiplicitaire, AUCUNE exactitude n'est recue\n");
  else
    std::printf("\nOK : sur les %lld ordres sans evenement degenere ni censure de rang,"
                " les COUVERTURES de Gamma_k coincident a chaque niveau de l'union"
                " verite+sujet, aux coupes stricte ET fermee ; les partitions de"
                " facettes et le journal d'incidences restent hors de ce claim\n",
                clean_orders);
  return 0;
}
