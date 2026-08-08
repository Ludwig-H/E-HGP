// MorseHGP3D v3 — M1, le juge.
//
// Oracle INDEPENDANT du chemin teste, sur le domaine DECLARE :
//   - arithmetique : rationnels de precision arbitraire, representation signe et
//     magnitude en chiffres de 32 bits (bigint.hpp), validee contre __int128 et
//     GMP par `mhgp3v_arith_selftest` ;
//   - geometrie : la sphere par un sous-ensemble est resolue par ELIMINATION DE
//     GAUSS du systeme 2<v,B_i> = |B_i|^2, jamais par les formules de CRAMER du
//     chemin de production ;
//   - structure : la foret de reference est reconstruite depuis Gamma_k lui-meme
//     (tous les k-sous-ensembles, tous les (k+1)-sous-ensembles), et non depuis
//     le catalogue du chemin teste.
//
// La campagne est FERMEE : attempted = decided + rejected_domain, aucun cas
// planifie n'est saute en silence, les arguments absurdes sont refuses, des
// planchers durs interdisent une campagne vide, et toute censure inattendue est
// un echec. Le recu publie l'identite pour qu'un lecteur puisse la recalculer.

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "bigint.hpp"
#include "rational.hpp"

#include "mhgp/mhgp.hpp"  // le SUJET, jamais l'autorite
#include "prototype/anchored_catalogue.hpp"
#include "prototype/edge_shallow.hpp"

namespace {

using mhgp3v::BigInt;
using mhgp3v::Rational;

// ---------------------------------------------------------------------------
// Vecteurs rationnels
// ---------------------------------------------------------------------------
struct Vec3 {
  Rational x, y, z;
};

Vec3 subtract(const Vec3& a, const Vec3& b) { return Vec3{a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 add(const Vec3& a, const Vec3& b) { return Vec3{a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 scale(const Vec3& a, const Rational& s) { return Vec3{a.x * s, a.y * s, a.z * s}; }
Rational dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 of_point(const mhgp::P3& p) {
  return Vec3{Rational(BigInt(static_cast<long long>(p.x))),
              Rational(BigInt(static_cast<long long>(p.y))),
              Rational(BigInt(static_cast<long long>(p.z)))};
}

// ---------------------------------------------------------------------------
// Elimination de GAUSS sur les rationnels. Renvoie faux si la matrice est
// singuliere, ce qui signale une dependance affine du sous-ensemble.
// ---------------------------------------------------------------------------
bool gauss_solve(std::vector<std::vector<Rational>> matrix, std::vector<Rational> rhs,
                 std::vector<Rational>* solution) {
  const std::size_t m = rhs.size();
  for (std::size_t column = 0; column < m; ++column) {
    std::size_t pivot = m;
    for (std::size_t row = column; row < m; ++row)
      if (!matrix[row][column].is_zero()) { pivot = row; break; }
    if (pivot == m) return false;
    std::swap(matrix[column], matrix[pivot]);
    std::swap(rhs[column], rhs[pivot]);
    const Rational p = matrix[column][column];
    for (std::size_t k = column; k < m; ++k) matrix[column][k] = matrix[column][k] / p;
    rhs[column] = rhs[column] / p;
    for (std::size_t row = 0; row < m; ++row) {
      if (row == column || matrix[row][column].is_zero()) continue;
      const Rational factor = matrix[row][column];
      for (std::size_t k = column; k < m; ++k)
        matrix[row][k] = matrix[row][k] - factor * matrix[column][k];
      rhs[row] = rhs[row] - factor * rhs[column];
    }
  }
  *solution = rhs;
  return true;
}

// Sphere portee par un sous-ensemble affinement independant : centre dans
// l'enveloppe affine, coordonnees barycentriques publiees.
struct RationalSphere {
  Vec3 centre;
  Rational squared_radius;
  std::vector<Rational> barycentric;
  bool ok = false;
};

RationalSphere sphere_through(const std::vector<mhgp::P3>& points,
                              const std::vector<int>& support) {
  RationalSphere out;
  const std::size_t m = support.size();
  const Vec3 base = of_point(points[static_cast<std::size_t>(support[0])]);
  if (m == 1) {
    out.centre = base;
    out.squared_radius = Rational();
    out.barycentric = {Rational(BigInt(1))};
    out.ok = true;
    return out;
  }
  std::vector<Vec3> spanning;
  for (std::size_t i = 1; i < m; ++i)
    spanning.push_back(subtract(of_point(points[static_cast<std::size_t>(support[i])]), base));
  const std::size_t q = spanning.size();
  std::vector<std::vector<Rational>> matrix(q, std::vector<Rational>(q));
  std::vector<Rational> rhs(q);
  const Rational two(BigInt(2));
  for (std::size_t i = 0; i < q; ++i) {
    for (std::size_t j = 0; j < q; ++j) matrix[i][j] = two * dot(spanning[i], spanning[j]);
    rhs[i] = dot(spanning[i], spanning[i]);
  }
  std::vector<Rational> t;
  if (!gauss_solve(matrix, rhs, &t)) return out;
  Vec3 offset{Rational(), Rational(), Rational()};
  for (std::size_t i = 0; i < q; ++i) offset = add(offset, scale(spanning[i], t[i]));
  out.centre = add(base, offset);
  out.squared_radius = dot(offset, offset);
  Rational sum;
  for (std::size_t i = 0; i < q; ++i) sum = sum + t[i];
  out.barycentric.push_back(Rational(BigInt(1)) - sum);
  for (std::size_t i = 0; i < q; ++i) out.barycentric.push_back(t[i]);
  out.ok = true;
  return out;
}

// -1 strictement dedans, 0 sur le bord, +1 dehors.
int side_of(const RationalSphere& sphere, const mhgp::P3& z) {
  const Vec3 d = subtract(of_point(z), sphere.centre);
  return compare(dot(d, d), sphere.squared_radius);
}

bool well_centred(const RationalSphere& sphere) {
  for (const Rational& lambda : sphere.barycentric)
    if (lambda.sign() <= 0) return false;
  return true;
}

// ---------------------------------------------------------------------------
// Enumeration des sous-ensembles
// ---------------------------------------------------------------------------
void each_subset(int n, int size, const std::function<void(const std::vector<int>&)>& visit) {
  if (size > n || size <= 0) return;
  std::vector<int> index(static_cast<std::size_t>(size));
  for (int i = 0; i < size; ++i) index[static_cast<std::size_t>(i)] = i;
  while (true) {
    visit(index);
    int i = size - 1;
    while (i >= 0 && index[static_cast<std::size_t>(i)] == n - size + i) --i;
    if (i < 0) return;
    ++index[static_cast<std::size_t>(i)];
    for (int j = i + 1; j < size; ++j)
      index[static_cast<std::size_t>(j)] = index[static_cast<std::size_t>(j - 1)] + 1;
  }
}

// Miniboule exacte d'un ensemble fini : plus petit support bien centre couvrant.
bool exact_miniball(const std::vector<mhgp::P3>& points, const std::vector<int>& family,
                    RationalSphere* out, std::vector<int>* support) {
  const int m = static_cast<int>(family.size());
  for (int size = 1; size <= 4 && size <= m; ++size) {
    bool found = false;
    each_subset(m, size, [&](const std::vector<int>& index) {
      if (found) return;
      std::vector<int> candidate;
      for (int i = 0; i < size; ++i)
        candidate.push_back(family[static_cast<std::size_t>(index[static_cast<std::size_t>(i)])]);
      const RationalSphere sphere = sphere_through(points, candidate);
      if (!sphere.ok || !well_centred(sphere)) return;
      for (int z : family)
        if (side_of(sphere, points[static_cast<std::size_t>(z)]) > 0) return;
      *out = sphere;
      *support = candidate;
      found = true;
    });
    if (found) return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Catalogue de reference
// ---------------------------------------------------------------------------
struct ReferenceSphere {
  std::vector<int> support;   // trie
  std::vector<int> members;   // trie, I union U
  Vec3 centre;                // centre EXACT : le niveau seul ne l'identifie pas
  Rational squared_radius;
  int rank = 0;
};

std::vector<ReferenceSphere> reference_catalogue(const std::vector<mhgp::P3>& points, int s_max,
                                                 int* degenerate_shells) {
  const int n = static_cast<int>(points.size());
  std::vector<ReferenceSphere> out;
  *degenerate_shells = 0;
  for (int size = 1; size <= 4 && size <= n; ++size) {
    each_subset(n, size, [&](const std::vector<int>& support) {
      const RationalSphere sphere = sphere_through(points, support);
      if (!sphere.ok || !well_centred(sphere)) return;
      ReferenceSphere reference;
      int on_shell = 0;
      bool extra_on_shell = false;
      for (int z = 0; z < n; ++z) {
        const int side = side_of(sphere, points[static_cast<std::size_t>(z)]);
        if (side > 0) continue;
        if (side == 0) {
          ++on_shell;
          if (std::find(support.begin(), support.end(), z) == support.end()) extra_on_shell = true;
        }
        reference.members.push_back(z);
      }
      if (extra_on_shell) { ++(*degenerate_shells); return; }
      if (on_shell != size) return;
      if (static_cast<int>(reference.members.size()) > s_max) return;
      reference.support = support;
      reference.centre = sphere.centre;
      reference.squared_radius = sphere.squared_radius;
      reference.rank = static_cast<int>(reference.members.size());
      out.push_back(std::move(reference));
    });
  }
  std::sort(out.begin(), out.end(), [](const ReferenceSphere& a, const ReferenceSphere& b) {
    return a.support < b.support;
  });
  return out;
}

// ---------------------------------------------------------------------------
// Foret de reference, reconstruite depuis Gamma_k
// ---------------------------------------------------------------------------
struct ReferenceNode {
  int kind = 0;                 // 0 naissance, 1 multifusion
  Rational level;
  int parent = -1;
  std::vector<int> children;
  std::vector<int> minima_closure;  // indices de minima du sous-arbre, tries
  // Pour une naissance : le k-sous-ensemble du minimum. Pour une multifusion :
  // les (k+1)-sous-ensembles qui ont EFFECTIVEMENT contribue au lot de cette
  // composante. Sans cette liste, une source etrangere de bon rang et de bon
  // niveau passe le juge (audit du 8 aout, §6).
  std::vector<std::vector<int>> admissible_sources;
};

struct DisjointSets {
  std::vector<int> parent;
  int find(int a) {
    while (parent[static_cast<std::size_t>(a)] != a) {
      parent[static_cast<std::size_t>(a)] =
          parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(a)])];
      a = parent[static_cast<std::size_t>(a)];
    }
    return a;
  }
};

struct ReferenceForest {
  std::vector<ReferenceNode> nodes;
  std::vector<std::vector<int>> minima;  // ensembles de k points, tries
  bool built = false;
};

// Construit la foret de fusion des minima de l'ordre k, uniquement a partir de
// Gamma_k : sommets = TOUS les k-sous-ensembles, cofaces = TOUS les
// (k+1)-sous-ensembles, chacun apparaissant au niveau exact de sa miniboule.
ReferenceForest reference_forest(const std::vector<mhgp::P3>& points, int k) {
  const int n = static_cast<int>(points.size());
  ReferenceForest forest;

  std::vector<std::vector<int>> faces;
  std::vector<Rational> face_level;
  std::vector<int> face_rank;
  each_subset(n, k, [&](const std::vector<int>& face) {
    RationalSphere sphere;
    std::vector<int> support;
    if (!exact_miniball(points, face, &sphere, &support)) return;
    int rank = 0;
    for (int z = 0; z < n; ++z)
      if (side_of(sphere, points[static_cast<std::size_t>(z)]) <= 0) ++rank;
    faces.push_back(face);
    face_level.push_back(sphere.squared_radius);
    face_rank.push_back(rank);
  });

  std::map<std::vector<int>, int> face_index;
  for (std::size_t i = 0; i < faces.size(); ++i) face_index.emplace(faces[i], static_cast<int>(i));

  // Un minimum est un k-sous-ensemble dont la miniboule ne contient que lui :
  // caracterisation intrinseque, independante du catalogue teste.
  std::vector<int> minimum_of_face(faces.size(), -1);
  for (std::size_t i = 0; i < faces.size(); ++i) {
    if (face_rank[i] != k) continue;
    minimum_of_face[i] = static_cast<int>(forest.minima.size());
    forest.minima.push_back(faces[i]);
  }

  std::vector<std::vector<int>> cofaces;
  std::vector<Rational> coface_level;
  each_subset(n, k + 1, [&](const std::vector<int>& coface) {
    RationalSphere sphere;
    std::vector<int> support;
    if (!exact_miniball(points, coface, &sphere, &support)) return;
    cofaces.push_back(coface);
    coface_level.push_back(sphere.squared_radius);
  });

  // Balayage par niveau exact croissant.
  std::vector<Rational> levels;
  for (const Rational& r : face_level) levels.push_back(r);
  for (const Rational& r : coface_level) levels.push_back(r);
  std::sort(levels.begin(), levels.end(),
            [](const Rational& a, const Rational& b) { return compare(a, b) < 0; });
  levels.erase(std::unique(levels.begin(), levels.end(),
                           [](const Rational& a, const Rational& b) { return compare(a, b) == 0; }),
               levels.end());

  DisjointSets sets;
  sets.parent.resize(faces.size());
  for (std::size_t i = 0; i < faces.size(); ++i) sets.parent[i] = static_cast<int>(i);
  std::vector<char> present(faces.size(), 0);
  std::vector<int> node_of_component(faces.size(), -1);

  for (const Rational& level : levels) {
    // 1. naissances : les faces dont la miniboule vaut exactement ce niveau.
    for (std::size_t i = 0; i < faces.size(); ++i) {
      if (present[i] || compare(face_level[i], level) != 0) continue;
      present[i] = 1;
      if (minimum_of_face[i] >= 0) {
        ReferenceNode node;
        node.kind = 0;
        node.level = level;
        node.minima_closure = {minimum_of_face[i]};
        node.admissible_sources = {faces[i]};
        forest.nodes.push_back(node);
        node_of_component[static_cast<std::size_t>(sets.find(static_cast<int>(i)))] =
            static_cast<int>(forest.nodes.size()) - 1;
      }
    }
    // 2. cofaces du meme niveau : on collecte les unions AVANT de les appliquer,
    //    puis on contracte, pour obtenir une multifusion et non une chaine.
    std::vector<std::vector<int>> hyperedges;
    std::vector<std::vector<int>> hyperedge_source;
    for (std::size_t c = 0; c < cofaces.size(); ++c) {
      if (compare(coface_level[c], level) != 0) continue;
      std::vector<int> roots;
      for (std::size_t drop = 0; drop < cofaces[c].size(); ++drop) {
        std::vector<int> face;
        for (std::size_t t = 0; t < cofaces[c].size(); ++t)
          if (t != drop) face.push_back(cofaces[c][t]);
        auto it = face_index.find(face);
        if (it == face_index.end() || !present[static_cast<std::size_t>(it->second)]) continue;
        roots.push_back(sets.find(it->second));
      }
      std::sort(roots.begin(), roots.end());
      roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
      if (roots.size() >= 2) {
        hyperedges.push_back(std::move(roots));
        hyperedge_source.push_back(cofaces[c]);
      }
    }
    if (hyperedges.empty()) continue;

    // composantes connexes du lot
    std::map<int, int> local;
    std::vector<int> back;
    for (const std::vector<int>& edge : hyperedges)
      for (int root : edge)
        if (local.emplace(root, static_cast<int>(back.size())).second) back.push_back(root);
    DisjointSets batch;
    batch.parent.resize(back.size());
    for (std::size_t i = 0; i < back.size(); ++i) batch.parent[i] = static_cast<int>(i);
    for (const std::vector<int>& edge : hyperedges) {
      const int first = batch.find(local[edge[0]]);
      for (std::size_t t = 1; t < edge.size(); ++t) {
        const int other = batch.find(local[edge[t]]);
        if (first != other) batch.parent[static_cast<std::size_t>(other)] = first;
      }
    }
    std::map<int, std::vector<int>> groups;
    for (std::size_t i = 0; i < back.size(); ++i)
      groups[batch.find(static_cast<int>(i))].push_back(back[i]);

    for (auto& group : groups) {
      std::vector<int>& live = group.second;
      std::sort(live.begin(), live.end());
      live.erase(std::unique(live.begin(), live.end()), live.end());
      // Seules les composantes portant deja un noeud comptent pour la structure
      // des minima ; une composante sans minimum ne cree pas d'evenement.
      std::vector<int> child_nodes;
      for (int root : live) {
        const int node = node_of_component[static_cast<std::size_t>(root)];
        if (node >= 0) child_nodes.push_back(node);
      }
      // fusion effective dans le DSU global
      const int keep = live[0];
      for (std::size_t t = 1; t < live.size(); ++t)
        sets.parent[static_cast<std::size_t>(live[t])] = keep;
      const int merged_root = sets.find(keep);
      if (child_nodes.size() >= 2) {
        ReferenceNode node;
        node.kind = 1;
        node.level = level;
        for (std::size_t hi = 0; hi < hyperedges.size(); ++hi) {
          const int representative = batch.find(local[hyperedges[hi][0]]);
          if (representative == group.first) node.admissible_sources.push_back(hyperedge_source[hi]);
        }
        std::sort(node.admissible_sources.begin(), node.admissible_sources.end());
        for (int child : child_nodes) {
          forest.nodes[static_cast<std::size_t>(child)].parent =
              static_cast<int>(forest.nodes.size());
          node.children.push_back(child);
          for (int minimum : forest.nodes[static_cast<std::size_t>(child)].minima_closure)
            node.minima_closure.push_back(minimum);
        }
        std::sort(node.minima_closure.begin(), node.minima_closure.end());
        forest.nodes.push_back(node);
        node_of_component[static_cast<std::size_t>(merged_root)] =
            static_cast<int>(forest.nodes.size()) - 1;
      } else if (child_nodes.size() == 1) {
        node_of_component[static_cast<std::size_t>(merged_root)] = child_nodes[0];
      } else {
        node_of_component[static_cast<std::size_t>(merged_root)] = -1;
      }
    }
  }
  forest.built = true;
  return forest;
}

// ---------------------------------------------------------------------------
// Lecture EXACTE du niveau d'un noeud teste, depuis les entiers de sa sphere
// source. On ne lit que des DONNEES du sujet, jamais un de ses predicats.
// ---------------------------------------------------------------------------
Rational exact_level_of(const mhgp::Sphere& sphere) {
  const BigInt nx = BigInt::from_i128(sphere.nx);
  const BigInt ny = BigInt::from_i128(sphere.ny);
  const BigInt nz = BigInt::from_i128(sphere.nz);
  const BigInt den = BigInt::from_i128(sphere.den);
  return Rational(nx * nx + ny * ny + nz * nz, den * den);
}

// ---------------------------------------------------------------------------
// Comptabilite de campagne
// ---------------------------------------------------------------------------
struct Campaign {
  long long attempted = 0;
  long long decided = 0;
  long long rejected_domain = 0;
  long long catalogue_spheres = 0;
  long long forests = 0;
  long long nodes = 0;
  long long failures = 0;
  std::size_t widest_bits = 0;
  std::vector<std::string> first_failures;

  enum class FailureCode {
    other,
    catalogue_members_unsorted,
    catalogue_centre,
    forest_sentinel,
    forest_n_children,
    forest_roots,
    forest_source_noncontributory,
    forest_source_nonminimal,
  };
  std::map<FailureCode, long long> failures_by_code;

  void fail(FailureCode code, const std::string& what) {
    ++failures;
    ++failures_by_code[code];
    if (first_failures.size() < 12) first_failures.push_back(what);
  }

  void fail(const std::string& what) { fail(FailureCode::other, what); }

  long long failure_count(FailureCode code) const {
    const auto it = failures_by_code.find(code);
    return it == failures_by_code.end() ? 0 : it->second;
  }
};

template <typename Integer>
bool parse_decimal_option(const char* option, const char* text, Integer* value) {
  const char* const end = text + std::strlen(text);
  const std::from_chars_result parsed = std::from_chars(text, end, *value, 10);
  if (text == end || parsed.ec != std::errc() || parsed.ptr != end) {
    std::printf("ECHEC : valeur entiere invalide pour %s : %s\n", option, text);
    return false;
  }
  return true;
}

std::string set_to_text(const std::vector<int>& values) {
  std::string out = "{";
  for (std::size_t i = 0; i < values.size(); ++i) {
    out += std::to_string(values[i]);
    if (i + 1 < values.size()) out += ",";
  }
  return out + "}";
}

// ---------------------------------------------------------------------------
// Comparaison structurelle complete d'une foret : genre, niveau exact, arite,
// racines, nombre canonique de noeuds et genealogie. La cle canonique d'un
// noeud est l'ensemble trie des minima de son sous-arbre.
// ---------------------------------------------------------------------------
struct CanonicalNode {
  int kind = 0;
  Rational level;
  int arity = 0;
  bool is_root = false;
  mhgp::i32 source_index = -1;
  std::vector<int> source_members;  // membres tries de la sphere source
};

void compare_forests(const mhgp::Catalogue& catalogue, const mhgp::Forest& subject,
                     const ReferenceForest& reference, int order, int trial,
                     Campaign* campaign) {
  const std::string tag = " trial=" + std::to_string(trial) + " k=" + std::to_string(order);

  std::map<std::vector<int>, int> minimum_index;
  for (std::size_t i = 0; i < reference.minima.size(); ++i)
    minimum_index.emplace(reference.minima[i], static_cast<int>(i));

  std::vector<std::vector<int>> subject_closure(subject.nodes.size());
  std::vector<int> subject_arity(subject.nodes.size(), 0);

  // Lecture hostile de la foret AVANT tout usage.
  for (std::size_t t = 0; t < subject.nodes.size(); ++t) {
    const mhgp::ForestNode& node = subject.nodes[t];
    if (node.source < 0 || node.source >= static_cast<mhgp::i32>(catalogue.spheres.size())) {
      campaign->fail("STRUCTURE noeud sans sphere source" + tag);
      return;
    }
    if (node.kind != 0 && node.kind != 1) {
      campaign->fail("STRUCTURE genre inconnu" + tag); return;
    }
    // Sentinelle STRICTE : le contrat reserve -1 ; -2 et au-dela doivent rougir.
    if (node.parent < -1 || node.first_child < -1 || node.next_sibling < -1) {
      campaign->fail(Campaign::FailureCode::forest_sentinel,
                     "STRUCTURE sentinelle de lien invalide" + tag);
      return;
    }
    if (node.parent >= static_cast<mhgp::i32>(subject.nodes.size())) {
      campaign->fail("STRUCTURE parent hors bornes" + tag); return;
    }
    if (node.first_child >= static_cast<mhgp::i32>(subject.nodes.size())
        || node.next_sibling >= static_cast<mhgp::i32>(subject.nodes.size())) {
      campaign->fail("STRUCTURE lien d'adjacence hors bornes" + tag); return;
    }
    // La sphere source d'une multifusion doit etre de rang k+1, celle d'une
    // naissance de rang k : une source de meme niveau mais sans rapport passait.
    const mhgp::CriticalSphere& source =
        catalogue.spheres[static_cast<std::size_t>(node.source)];
    const int wanted_rank = node.kind == 0 ? order : order + 1;
    if (source.rank != wanted_rank) {
      campaign->fail("STRUCTURE rang de la sphere source " + std::to_string(source.rank)
                     + " contre " + std::to_string(wanted_rank) + tag);
      return;
    }
  }

  // Les deux representations d'adjacence doivent coincider entre elles.
  std::vector<int> children_from_links(subject.nodes.size(), 0);
  for (std::size_t t = 0; t < subject.nodes.size(); ++t) {
    for (mhgp::i32 child = subject.nodes[t].first_child; child >= 0;) {
      ++children_from_links[t];
      if (subject.nodes[static_cast<std::size_t>(child)].parent != static_cast<mhgp::i32>(t)) {
        campaign->fail("STRUCTURE enfant dont le parent ne correspond pas" + tag);
        return;
      }
      child = subject.nodes[static_cast<std::size_t>(child)].next_sibling;
      if (children_from_links[t] > static_cast<int>(subject.nodes.size())) {
        campaign->fail("STRUCTURE cycle dans la liste d'enfants" + tag);
        return;
      }
    }
  }

  for (std::size_t t = 0; t < subject.nodes.size(); ++t) {
    const mhgp::ForestNode& node = subject.nodes[t];
    if (node.kind != 0) continue;
    const mhgp::CriticalSphere& source = catalogue.spheres[static_cast<std::size_t>(node.source)];
    std::vector<int> members;
    for (mhgp::i32 i = 0; i < source.rank; ++i)
      members.push_back(static_cast<int>(
          catalogue.members[static_cast<std::size_t>(source.members_begin + i)]));
    std::sort(members.begin(), members.end());
    auto it = minimum_index.find(members);
    if (it == minimum_index.end()) {
      campaign->fail("STRUCTURE minimum inconnu de la reference " + set_to_text(members) + tag);
      return;
    }
    subject_closure[t] = {it->second};
  }

  // Les noeuds sont crees par niveau croissant, donc un parent a un index
  // superieur a ses enfants : un balayage ascendant suffit a remonter.
  for (std::size_t t = 0; t < subject.nodes.size(); ++t) {
    const mhgp::ForestNode& node = subject.nodes[t];
    if (node.parent < 0) continue;
    if (node.parent <= static_cast<mhgp::i32>(t)) {
      campaign->fail("STRUCTURE parent d'index inferieur au sien" + tag);
      return;
    }
    std::vector<int>& target = subject_closure[static_cast<std::size_t>(node.parent)];
    for (int minimum : subject_closure[t]) target.push_back(minimum);
    ++subject_arity[static_cast<std::size_t>(node.parent)];
  }
  for (std::vector<int>& closure : subject_closure) {
    std::sort(closure.begin(), closure.end());
    closure.erase(std::unique(closure.begin(), closure.end()), closure.end());
  }

  std::map<std::vector<int>, CanonicalNode> subject_map;
  for (std::size_t t = 0; t < subject.nodes.size(); ++t) {
    CanonicalNode canonical;
    canonical.kind = subject.nodes[t].kind;
    canonical.level = exact_level_of(
        catalogue.spheres[static_cast<std::size_t>(subject.nodes[t].source)].sph);
    canonical.arity = subject_arity[t];
    canonical.is_root = subject.nodes[t].parent < 0;
    canonical.source_index = subject.nodes[t].source;
    {
      const mhgp::CriticalSphere& source =
          catalogue.spheres[static_cast<std::size_t>(subject.nodes[t].source)];
      for (mhgp::i32 i = 0; i < source.rank; ++i)
        canonical.source_members.push_back(static_cast<int>(
            catalogue.members[static_cast<std::size_t>(source.members_begin + i)]));
      std::sort(canonical.source_members.begin(), canonical.source_members.end());
    }
    campaign->widest_bits =
        std::max(campaign->widest_bits, canonical.level.widest_bit_length());
    if (!subject_map.emplace(subject_closure[t], canonical).second) {
      campaign->fail("STRUCTURE deux noeuds du sujet de meme fermeture" + tag);
      return;
    }
  }

  std::map<std::vector<int>, CanonicalNode> reference_map;
  for (std::size_t t = 0; t < reference.nodes.size(); ++t) {
    CanonicalNode canonical;
    canonical.kind = reference.nodes[t].kind;
    canonical.level = reference.nodes[t].level;
    canonical.arity = static_cast<int>(reference.nodes[t].children.size());
    canonical.is_root = reference.nodes[t].parent < 0;
    if (!reference_map.emplace(reference.nodes[t].minima_closure, canonical).second) {
      campaign->fail("STRUCTURE deux noeuds de reference de meme fermeture" + tag);
      return;
    }
  }

  if (subject_map.size() != reference_map.size())
    campaign->fail("STRUCTURE nombre canonique de noeuds " + std::to_string(subject_map.size())
                   + " contre " + std::to_string(reference_map.size()) + tag);

  for (const auto& entry : reference_map) {
    auto it = subject_map.find(entry.first);
    if (it == subject_map.end()) {
      campaign->fail("STRUCTURE noeud absent du sujet " + set_to_text(entry.first) + tag);
      continue;
    }
    ++campaign->nodes;
    if (it->second.kind != entry.second.kind) campaign->fail("STRUCTURE genre" + tag);
    if (compare(it->second.level, entry.second.level) != 0)
      campaign->fail("STRUCTURE niveau exact " + set_to_text(entry.first) + tag);
    if (it->second.arity != entry.second.arity)
      campaign->fail("STRUCTURE arite " + std::to_string(it->second.arity) + " contre "
                     + std::to_string(entry.second.arity) + " " + set_to_text(entry.first) + tag);
    if (it->second.is_root != entry.second.is_root)
      campaign->fail("STRUCTURE racine " + set_to_text(entry.first) + tag);
  }
  // La source doit avoir CONTRIBUE, pas seulement avoir le bon rang et le bon
  // niveau : une sphere etrangere passait ce garde.
  for (std::size_t t = 0; t < reference.nodes.size(); ++t) {
    auto it = subject_map.find(reference.nodes[t].minima_closure);
    if (it == subject_map.end()) continue;
    const std::vector<std::vector<int>>& admissible = reference.nodes[t].admissible_sources;
    const bool contributes =
        std::find(admissible.begin(), admissible.end(), it->second.source_members)
        != admissible.end();
    if (!contributes) {
      campaign->fail(Campaign::FailureCode::forest_source_noncontributory,
                     "STRUCTURE source non contributrice "
                         + set_to_text(it->second.source_members) + " pour "
                         + set_to_text(reference.nodes[t].minima_closure) + tag);
      continue;
    }
    if (reference.nodes[t].kind != 1) continue;

    mhgp::i32 canonical_source = -1;
    for (std::size_t si = 0; si < catalogue.spheres.size(); ++si) {
      const mhgp::CriticalSphere& sphere = catalogue.spheres[si];
      if (sphere.rank != order + 1) continue;
      std::vector<int> members;
      for (mhgp::i32 i = 0; i < sphere.rank; ++i)
        members.push_back(static_cast<int>(
            catalogue.members[static_cast<std::size_t>(sphere.members_begin + i)]));
      std::sort(members.begin(), members.end());
      if (std::find(admissible.begin(), admissible.end(), members) == admissible.end()) continue;
      canonical_source = static_cast<mhgp::i32>(si);
      break;
    }
    if (canonical_source < 0) {
      campaign->fail("STRUCTURE aucune source contributrice dans le catalogue" + tag);
    } else if (it->second.source_index != canonical_source) {
      campaign->fail(Campaign::FailureCode::forest_source_nonminimal,
                     "STRUCTURE source contributrice non minimale index="
                         + std::to_string(it->second.source_index) + " attendu="
                         + std::to_string(canonical_source) + tag);
    }
  }
  for (const auto& entry : subject_map)
    if (reference_map.find(entry.first) == reference_map.end())
      campaign->fail("STRUCTURE noeud superflu du sujet " + set_to_text(entry.first) + tag);

  for (std::size_t t = 0; t < subject.nodes.size(); ++t) {
    const mhgp::ForestNode& node = subject.nodes[t];
    if (node.parent < 0) continue;
    const Rational child = exact_level_of(
        catalogue.spheres[static_cast<std::size_t>(node.source)].sph);
    const mhgp::ForestNode& parent = subject.nodes[static_cast<std::size_t>(node.parent)];
    const Rational above = exact_level_of(
        catalogue.spheres[static_cast<std::size_t>(parent.source)].sph);
    if (compare(child, above) > 0)
      campaign->fail("STRUCTURE niveau decroissant vers le parent" + tag);
    if (node.kind == 1 && parent.kind == 1 && compare(child, above) == 0)
      campaign->fail("STRUCTURE chaine de fusions de meme niveau" + tag);
  }
  // Champs publics annonces : ils doivent etre compares, pas recalcules en
  // silence depuis les parents.
  long long births = 0, merges = 0, killed = 0;
  std::vector<mhgp::i32> roots;
  for (std::size_t t = 0; t < subject.nodes.size(); ++t) {
    const mhgp::ForestNode& node = subject.nodes[t];
    if (node.kind == 0) ++births; else { ++merges; killed += subject_arity[t] - 1; }
    if (node.n_children != children_from_links[t] || node.n_children != subject_arity[t])
      campaign->fail(Campaign::FailureCode::forest_n_children,
                     "STRUCTURE n_children incoherent" + tag);
    if (node.parent < 0) roots.push_back(static_cast<mhgp::i32>(t));
  }
  if (subject.order != order) campaign->fail("STRUCTURE ordre publie" + tag);
  if (subject.births != births) campaign->fail("STRUCTURE compteur de naissances" + tag);
  if (subject.merge_events != merges) campaign->fail("STRUCTURE compteur de fusions" + tag);
  if (subject.killed != killed) campaign->fail("STRUCTURE compteur de morts" + tag);
  if (subject.roots != roots)
    campaign->fail(Campaign::FailureCode::forest_roots,
                   "STRUCTURE liste des racines" + tag);
  if (subject.unresolved_arms != 0 || subject.censored_events != 0 || !subject.authoritative)
    campaign->fail("STRUCTURE foret censuree dans une campagne positive" + tag);
  ++campaign->forests;
}

// ---------------------------------------------------------------------------
// Comparaison du catalogue : supports, rang, membres tries, niveau exact, et
// absence de doublon de support cote sujet.
// ---------------------------------------------------------------------------
// Centre exact d'une sphere sujet : base + num/den, lu comme DONNEE.
Vec3 exact_centre_of(const mhgp::Sphere& sphere) {
  const Rational den(BigInt::from_i128(sphere.den));
  return Vec3{Rational(BigInt(static_cast<long long>(sphere.base.x))) +
                  Rational(BigInt::from_i128(sphere.nx)) / den,
              Rational(BigInt(static_cast<long long>(sphere.base.y))) +
                  Rational(BigInt::from_i128(sphere.ny)) / den,
              Rational(BigInt(static_cast<long long>(sphere.base.z))) +
                  Rational(BigInt::from_i128(sphere.nz)) / den};
}

// LECTURE HOSTILE : le juge ne fait jamais confiance a la structure memoire de
// ce qu'il juge. Tout indice, toute taille et tout denominateur sont valides
// AVANT dereferencement, sinon un sujet fautif ferait sortir le juge de ses
// propres tableaux au lieu de le faire rougir.
bool subject_record_is_readable(const mhgp::Catalogue& catalogue,
                                const mhgp::CriticalSphere& sphere, int s_max,
                                int point_count, std::string* why) {
  if (sphere.n_support < 1 || sphere.n_support > mhgp::kMaxSupport) {
    *why = "n_support hors bornes"; return false;
  }
  for (mhgp::i32 i = 0; i < sphere.n_support; ++i) {
    if (sphere.support[i] < 0 || sphere.support[i] >= point_count) {
      *why = "identifiant de support hors bornes"; return false;
    }
    if (i > 0 && sphere.support[i] <= sphere.support[i - 1]) {
      *why = "support non strictement croissant"; return false;
    }
  }
  if (sphere.rank < sphere.n_support || sphere.rank > s_max) {
    *why = "rang incoherent avec le support"; return false;
  }
  if (sphere.members_begin < 0
      || static_cast<std::size_t>(sphere.members_begin) + static_cast<std::size_t>(sphere.rank)
             > catalogue.members.size()) {
    *why = "tranche de membres hors du pool"; return false;
  }
  for (mhgp::i32 i = 0; i < sphere.rank; ++i) {
    const mhgp::i32 member =
        catalogue.members[static_cast<std::size_t>(sphere.members_begin + i)];
    if (member < 0 || member >= point_count) { *why = "membre hors bornes"; return false; }
  }
  if (sphere.sph.support != sphere.n_support) {
    *why = "arite de la sphere differente du support"; return false;
  }
  if (sphere.sph.den <= 0) { *why = "denominateur non strictement positif"; return false; }
  return true;
}

// Renvoie faux si le sujet est illisible : AUCUNE lecture aval ne doit alors
// avoir lieu, sinon une mutation censee faire rougir le juge le ferait sortir de
// ses propres tableaux.
bool compare_catalogues(const mhgp::Catalogue& subject,
                        const std::vector<ReferenceSphere>& reference, int s_max, int point_count,
                        int trial, Campaign* campaign) {
  const std::string tag = " trial=" + std::to_string(trial);
  for (const mhgp::CriticalSphere& sphere : subject.spheres) {
    std::string why;
    if (!subject_record_is_readable(subject, sphere, s_max, point_count, &why)) {
      campaign->fail("LECTURE " + why + tag);
      return false;
    }
  }
  std::map<std::vector<int>, const mhgp::CriticalSphere*> by_support;
  for (const mhgp::CriticalSphere& sphere : subject.spheres) {
    std::vector<int> support;
    for (mhgp::i32 i = 0; i < sphere.n_support; ++i)
      support.push_back(static_cast<int>(sphere.support[i]));
    if (!by_support.emplace(support, &sphere).second)
      campaign->fail("CATALOGUE support en double " + set_to_text(support) + tag);
  }
  if (by_support.size() != subject.spheres.size())
    campaign->fail("CATALOGUE doublons de support" + tag);
  if (by_support.size() != reference.size())
    campaign->fail("CATALOGUE cardinal " + std::to_string(by_support.size()) + " contre "
                   + std::to_string(reference.size()) + tag);

  for (const ReferenceSphere& want : reference) {
    auto it = by_support.find(want.support);
    if (it == by_support.end()) {
      campaign->fail("CATALOGUE support manquant " + set_to_text(want.support) + tag);
      continue;
    }
    ++campaign->catalogue_spheres;
    const mhgp::CriticalSphere& got = *it->second;
    if (got.rank != want.rank)
      campaign->fail("CATALOGUE rang " + set_to_text(want.support) + tag);
    std::vector<int> members;
    for (mhgp::i32 i = 0; i < got.rank; ++i)
      members.push_back(static_cast<int>(
          subject.members[static_cast<std::size_t>(got.members_begin + i)]));
    std::vector<int> sorted = members;
    std::sort(sorted.begin(), sorted.end());
    if (sorted != want.members)
      campaign->fail("CATALOGUE membres " + set_to_text(want.support) + tag);
    if (members != sorted)
      campaign->fail(Campaign::FailureCode::catalogue_members_unsorted,
                     "CATALOGUE tranche de membres non triee "
                         + set_to_text(want.support) + tag);
    const Rational level = exact_level_of(got.sph);
    campaign->widest_bits = std::max(campaign->widest_bits, level.widest_bit_length());
    if (compare(level, want.squared_radius) != 0)
      campaign->fail("CATALOGUE niveau exact " + set_to_text(want.support) + tag);
    // Le niveau ne suffit pas : un numerateur tourne garde la meme norme.
    const Vec3 centre = exact_centre_of(got.sph);
    if (compare(centre.x, want.centre.x) != 0 || compare(centre.y, want.centre.y) != 0
        || compare(centre.z, want.centre.z) != 0)
      campaign->fail(Campaign::FailureCode::catalogue_centre,
                     "CATALOGUE centre exact " + set_to_text(want.support) + tag);
  }
  for (const auto& entry : by_support) {
    bool present = false;
    for (const ReferenceSphere& want : reference)
      if (want.support == entry.first) { present = true; break; }
    if (!present)
      campaign->fail("CATALOGUE support superflu " + set_to_text(entry.first) + tag);
  }
  return true;
}

bool injection_failure_code(const std::string& injection, Campaign::FailureCode* code) {
  if (injection == "member_unsorted") {
    *code = Campaign::FailureCode::catalogue_members_unsorted;
  } else if (injection == "rotated_centre") {
    *code = Campaign::FailureCode::catalogue_centre;
  } else if (injection == "sentinel") {
    *code = Campaign::FailureCode::forest_sentinel;
  } else if (injection == "n_children") {
    *code = Campaign::FailureCode::forest_n_children;
  } else if (injection == "roots") {
    *code = Campaign::FailureCode::forest_roots;
  } else if (injection == "merge_source_foreign") {
    *code = Campaign::FailureCode::forest_source_noncontributory;
  } else if (injection == "merge_source_nonminimal_contributor") {
    *code = Campaign::FailureCode::forest_source_nonminimal;
  } else {
    return false;
  }
  return true;
}

struct InjectionProbe {
  bool applied = false;
  Campaign observed;
};

// Exerce un seul garde sur une COPIE d'un sujet deja passe au vert. La campagne
// positive et le probe hostile ont des comptabilites distinctes : un plancher,
// un rejet de domaine ou une regression preexistante ne peut donc pas servir de
// preuve qu'une injection a ete attrapee.
InjectionProbe exercise_injection(const std::string& injection,
                                  const mhgp::Catalogue& clean_catalogue,
                                  const std::vector<mhgp::Forest>& clean_forests,
                                  const std::vector<ReferenceSphere>& reference,
                                  const std::vector<mhgp::P3>& points, int s_max, int order,
                                  int trial) {
  InjectionProbe result;
  const int point_count = static_cast<int>(points.size());

  if (injection == "member_unsorted") {
    mhgp::Catalogue mutated = clean_catalogue;
    for (mhgp::CriticalSphere& sphere : mutated.spheres) {
      if (sphere.rank < 2) continue;
      std::swap(mutated.members[static_cast<std::size_t>(sphere.members_begin)],
                mutated.members[static_cast<std::size_t>(sphere.members_begin + 1)]);
      result.applied = true;
      break;
    }
    if (result.applied)
      compare_catalogues(mutated, reference, s_max, point_count, trial, &result.observed);
    return result;
  }

  if (injection == "rotated_centre") {
    mhgp::Catalogue mutated = clean_catalogue;
    for (mhgp::CriticalSphere& sphere : mutated.spheres) {
      if (sphere.sph.nx == sphere.sph.ny) continue;
      std::swap(sphere.sph.nx, sphere.sph.ny);
      result.applied = true;
      break;
    }
    if (result.applied)
      compare_catalogues(mutated, reference, s_max, point_count, trial, &result.observed);
    return result;
  }

  for (std::size_t fi = 0; fi < clean_forests.size(); ++fi) {
    const int forest_order = static_cast<int>(fi) + 1;
    const ReferenceForest expected = reference_forest(points, forest_order);

    if (injection == "sentinel") {
      if (clean_forests[fi].nodes.empty()) continue;
      mhgp::Forest mutated = clean_forests[fi];
      mutated.nodes.front().parent = -2;
      result.applied = true;
      compare_forests(clean_catalogue, mutated, expected, forest_order, trial, &result.observed);
      return result;
    }

    if (injection == "n_children") {
      mhgp::Forest mutated = clean_forests[fi];
      for (mhgp::ForestNode& node : mutated.nodes) {
        if (node.n_children <= 0) continue;
        node.n_children = 0;
        result.applied = true;
        compare_forests(clean_catalogue, mutated, expected, forest_order, trial,
                        &result.observed);
        return result;
      }
    }

    if (injection == "roots") {
      if (clean_forests[fi].roots.empty()) continue;
      mhgp::Forest mutated = clean_forests[fi];
      mutated.roots.pop_back();
      result.applied = true;
      compare_forests(clean_catalogue, mutated, expected, forest_order, trial, &result.observed);
      return result;
    }

    if (injection == "merge_source_foreign"
        || injection == "merge_source_nonminimal_contributor") {
      for (std::size_t ni = 0; ni < clean_forests[fi].nodes.size(); ++ni) {
        const mhgp::ForestNode& node = clean_forests[fi].nodes[ni];
        if (node.kind != 1 || node.source < 0
            || node.source >= static_cast<mhgp::i32>(clean_catalogue.spheres.size()))
          continue;
        const mhgp::CriticalSphere& original =
            clean_catalogue.spheres[static_cast<std::size_t>(node.source)];
        const Rational original_level = exact_level_of(original.sph);
        for (std::size_t si = 0; si < clean_catalogue.spheres.size(); ++si) {
          const mhgp::CriticalSphere& candidate = clean_catalogue.spheres[si];
          if (candidate.rank != forest_order + 1
              || static_cast<mhgp::i32>(si) == node.source)
            continue;
          if (compare(exact_level_of(candidate.sph), original_level) != 0) continue;

          // Les fixtures dediees fournissent deux VRAIES sphères du catalogue,
          // de meme rang et de meme niveau exact : disjointes pour la source
          // etrangere, contributrices au meme lot pour la source non minimale.
          // Aucun record hybride n'est fabrique pour aider le garde teste.
          mhgp::Forest mutated_forest = clean_forests[fi];
          mutated_forest.nodes[ni].source = static_cast<mhgp::i32>(si);
          result.applied = true;
          compare_forests(clean_catalogue, mutated_forest, expected, forest_order, trial,
                          &result.observed);
          return result;
        }
      }
    }
  }

  (void)order;
  return result;
}

}  // namespace

int main(int argc, char** argv) {
  constexpr int kMaximumOraclePoints = 64;
  constexpr int kMaximumDiagnosticPoints = 1'000'000;
  int clouds = 40;
  unsigned long long seed = 4242ULL;
  int minimum_points = 8, maximum_points = 11, maximum_order = 3;
  long long coordinate_maximum = mhgp::kCoordMax;  // LA GRILLE DECLAREE
  long long minimum_clouds_decided = 1, minimum_nodes = 1;
  std::string receipt_path;
  std::string subject = "v2";
  bool measure_only = false;
  int fixed_points = 0;
  int seed_neighbours = 16;
  std::string regime = "exhaustive";
  std::string injection;
  long long require_incomplete_anchors = -1;
  std::string fixture;

  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    auto next = [&](const char* what) -> const char* {
      if (i + 1 >= argc) {
        std::printf("ECHEC : option %s sans valeur\n", what);
        std::exit(2);
      }
      return argv[++i];
    };
    if (argument == "--clouds") {
      if (!parse_decimal_option("--clouds", next("--clouds"), &clouds)) return 2;
    } else if (argument == "--seed") {
      if (!parse_decimal_option("--seed", next("--seed"), &seed)) return 2;
    } else if (argument == "--min-points") {
      if (!parse_decimal_option("--min-points", next("--min-points"), &minimum_points)) return 2;
    } else if (argument == "--max-points") {
      if (!parse_decimal_option("--max-points", next("--max-points"), &maximum_points)) return 2;
    } else if (argument == "--max-order") {
      if (!parse_decimal_option("--max-order", next("--max-order"), &maximum_order)) return 2;
    } else if (argument == "--coord-max") {
      if (!parse_decimal_option("--coord-max", next("--coord-max"), &coordinate_maximum)) return 2;
    } else if (argument == "--min-decided") {
      if (!parse_decimal_option("--min-decided", next("--min-decided"),
                                &minimum_clouds_decided))
        return 2;
    } else if (argument == "--min-nodes") {
      if (!parse_decimal_option("--min-nodes", next("--min-nodes"), &minimum_nodes)) return 2;
    }
    else if (argument == "--receipt") receipt_path = next("--receipt");
    else if (argument == "--subject") subject = next("--subject");
    else if (argument == "--measure-only") measure_only = true;
    else if (argument == "--points") {
      if (!parse_decimal_option("--points", next("--points"), &fixed_points)) return 2;
    } else if (argument == "--seed-neighbours") {
      if (!parse_decimal_option("--seed-neighbours", next("--seed-neighbours"),
                                &seed_neighbours))
        return 2;
    }
    else if (argument == "--regime") regime = next("--regime");
    else if (argument == "--inject") injection = next("--inject");
    else if (argument == "--fixture") fixture = next("--fixture");
    else if (argument == "--require-incomplete-anchors") {
      if (!parse_decimal_option("--require-incomplete-anchors",
                                next("--require-incomplete-anchors"),
                                &require_incomplete_anchors)) return 2;
    }
    else if (argument == "--fixture") fixture = next("--fixture");
    else {
      std::printf("ECHEC : option inconnue %s\n", argument.c_str());
      return 2;
    }
  }

  // Refus des arguments absurdes : une campagne vide ne doit jamais rendre OK.
  // Les planchers doivent etre STRICTEMENT positifs : avec des planchers nuls,
  // une campagne qui ne decide rien satisfait l'identite de fermeture et sort
  // avec le code 0 — exactement le defaut reproche a la porte de la v2.
  if (clouds <= 0 || minimum_points < 4 || maximum_points < minimum_points
      || maximum_points > kMaximumOraclePoints || maximum_order < 1
      || maximum_order >= mhgp::kMaxRank || coordinate_maximum < 1
      || minimum_clouds_decided <= 0 || minimum_nodes <= 0 || fixed_points < 0
      || seed_neighbours <= 0
      || coordinate_maximum > mhgp::kCoordMax) {
    std::printf("ECHEC : campagne absurde (clouds=%d points=[%d,%d] ordre=%d coord-max=%lld "
                "min-decided=%lld min-nodes=%lld)\n",
                clouds, minimum_points, maximum_points, maximum_order,
                static_cast<long long>(coordinate_maximum), minimum_clouds_decided,
                minimum_nodes);
    return 2;
  }

  // Le parsing integral ne suffit pas : une valeur syntaxiquement valide mais
  // hors contrat doit rendre 2 AVANT toute arithmetique. `maximum_order + 1`
  // debordait sur 2147483647.
  if (maximum_order > mhgp::kMaxRank - 1) {
    std::printf("ECHEC : --max-order %d hors contrat (au plus %d, car s_max = ordre + 1 "
                "et kMaxRank = %d)\n", maximum_order, mhgp::kMaxRank - 1, mhgp::kMaxRank);
    return 2;
  }
  if (maximum_points > 4096) {
    std::printf("ECHEC : --max-points %d hors contrat pour un oracle exhaustif\n",
                maximum_points);
    return 2;
  }
  if (fixed_points < 0 || fixed_points > 1000000) {
    std::printf("ECHEC : --points %d hors contrat\n", fixed_points);
    return 2;
  }
  if (seed_neighbours < 1 || seed_neighbours > 1000000) {
    std::printf("ECHEC : --seed-neighbours %d hors contrat\n", seed_neighbours);
    return 2;
  }
  if (clouds > 1000000) {
    std::printf("ECHEC : --clouds %d hors contrat\n", clouds);
    return 2;
  }

  if (regime != "exhaustive" && regime != "assumed_window") {
    std::printf("ECHEC : regime inconnu %s (exhaustive ou assumed_window)\n", regime.c_str());
    return 2;
  }
  if (subject != "v2" && subject != "anchored" && subject != "edge_shallow") {
    std::printf("ECHEC : sujet inconnu %s (v2, anchored ou edge_shallow)\n", subject.c_str());
    return 2;
  }
  if (!fixture.empty() && fixture != "foreign_source_same_level"
      && fixture != "nonminimal_contributor_same_level"
      && fixture != "noncritical_shell_tie") {
    std::printf("ECHEC : fixture inconnue %s\n", fixture.c_str());
    return 2;
  }
  if (!fixture.empty()
      && fixture == "foreign_source_same_level"
      && (measure_only || subject != "v2" || clouds != 1 || minimum_points != 4
          || maximum_points != 4 || maximum_order != 1 || coordinate_maximum != 2)) {
    std::printf("ECHEC : la fixture foreign_source_same_level exige --subject v2 --clouds 1 "
                "--min-points 4 --max-points 4 --max-order 1 --coord-max 2\n");
    return 2;
  }
  if (fixture == "noncritical_shell_tie"
      && (measure_only || subject != "anchored" || regime != "exhaustive" || clouds != 1
          || minimum_points != 4 || maximum_points != 4 || maximum_order != 2
          || coordinate_maximum != 2000)) {
    std::printf("ECHEC : la fixture noncritical_shell_tie exige --subject anchored "
                "--regime exhaustive --clouds 1 --min-points 4 --max-points 4 "
                "--max-order 2 --coord-max 2000\n");
    return 2;
  }
  if (fixture == "nonminimal_contributor_same_level"
      && (measure_only || subject != "v2" || clouds != 1 || minimum_points != 4
          || maximum_points != 4 || maximum_order != 1 || coordinate_maximum != 10)) {
    std::printf("ECHEC : la fixture nonminimal_contributor_same_level exige --subject v2 "
                "--clouds 1 --min-points 4 --max-points 4 --max-order 1 --coord-max 10\n");
    return 2;
  }

  Campaign::FailureCode expected_injection_code = Campaign::FailureCode::other;
  if (!injection.empty() && !injection_failure_code(injection, &expected_injection_code)) {
    std::printf("ECHEC : injection inconnue %s\n", injection.c_str());
    return 2;
  }

  // Mode MESURE : pas d'oracle (il est exponentiel en n), donc pas de jugement.
  // Il ne sert qu'a publier la distribution du voisinage CERTIFIE, c'est-a-dire
  // le chiffre que la v2 ne pouvait pas produire. Il ne qualifie rien.
  if (measure_only) {
    if (subject != "anchored" || fixed_points <= 0
        || fixed_points > kMaximumDiagnosticPoints || maximum_order >= fixed_points) {
      std::printf("ECHEC : --measure-only exige --subject anchored et --points N\n");
      return 2;
    }
    if (!receipt_path.empty() || !injection.empty()) {
      std::printf("ECHEC : --measure-only ne prend encore ni --receipt ni --inject; "
                  "aucune sortie ne doit etre ignoree silencieusement\n");
      return 2;
    }
    std::mt19937_64 measure_rng(seed);
    std::uniform_int_distribution<long long> measure_coordinate(0, coordinate_maximum);
    for (int trial = 0; trial < clouds; ++trial) {
      std::vector<mhgp::P3> points(static_cast<std::size_t>(fixed_points));
      for (int i = 0; i < fixed_points; ++i)
        points[static_cast<std::size_t>(i)] = mhgp::P3{measure_coordinate(measure_rng),
                                                       measure_coordinate(measure_rng),
                                                       measure_coordinate(measure_rng)};
      mhgp3v::AnchoredCampaign anchored;
      const mhgp::Catalogue catalogue =
          mhgp3v::anchored_catalogue(points, maximum_order + 1, seed_neighbours,
                                     regime == "exhaustive" ? mhgp3v::Regime::exhaustive
                                                            : mhgp3v::Regime::assumed_window,
                                     &anchored);
      std::vector<int> sizes;
      for (const mhgp3v::AnchorStatistics& stat : anchored.per_anchor)
        sizes.push_back(stat.sufficient_neighbours);
      std::sort(sizes.begin(), sizes.end());
      const auto quantile = [&](double q) {
        return sizes[static_cast<std::size_t>(q * static_cast<double>(sizes.size() - 1))];
      };
      long long incidence_total = 0, centred_incidence_total = 0;
      for (int m = 1; m <= 4; ++m) {
        incidence_total += anchored.canonical_support_incidence[m];
        centred_incidence_total += anchored.centred_support_incidence[m];
      }
      std::printf("  incidences support-ancre par arite (candidate / bien centree) :");
      for (int m = 1; m <= 4; ++m)
        std::printf("  m=%d %lld/%lld", m, anchored.canonical_support_incidence[m],
                    anchored.centred_support_incidence[m]);
      const double centred_percent =
          incidence_total > 0
              ? 100.0 * static_cast<double>(centred_incidence_total)
                    / static_cast<double>(incidence_total)
              : 0.0;
      const double incidences_per_emitted =
          catalogue.spheres.empty()
              ? 0.0
              : static_cast<double>(incidence_total)
                    / static_cast<double>(catalogue.spheres.size());
      const double candidates_per_emitted =
          catalogue.spheres.empty()
              ? 0.0
              : static_cast<double>(anchored.candidates)
                    / static_cast<double>(catalogue.spheres.size());
      std::printf("\n  incidences totales=%lld bien centrees=%lld (%.1f %%) | emises=%zu "
                  "| incidences par sphere emise=%.2f | candidats par sphere emise=%.1f\n",
                  incidence_total, centred_incidence_total, centred_percent,
                  catalogue.spheres.size(), incidences_per_emitted, candidates_per_emitted);
      std::printf("status=diagnostic_only regime=%s n=%d s_max=%d | spheres=%zu (%.1f/point) | "
                  "fenetre SUFFISANTE p50=%d p95=%d p99=%d max=%d moyenne=%.1f | fenetre employee "
                  "moyenne=%.1f | candidats=%lld temoins=%lld | paires=%lld | non exhaustives=%d "
                  "| degeneres=%lld\n", regime.c_str(),
                  fixed_points, maximum_order + 1, catalogue.spheres.size(),
                  static_cast<double>(catalogue.spheres.size()) / fixed_points,
                  quantile(0.50), quantile(0.95), quantile(0.99), sizes.back(),
                  anchored.sufficient_mean, anchored.neighbourhood_mean,
                  anchored.candidates, anchored.witness_tests,
                  anchored.anchor_member_pairs, anchored.incomplete_anchors,
                  anchored.degenerate_shells);
    }
    return 0;
  }

  Campaign campaign;
  long long injections_applied = 0, injection_escapes = 0;
  mhgp3v::AnchoredCampaign anchored_total;
  mhgp3v::EdgeShallowStatistics edge_shallow_total;
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<long long> coordinate(0, coordinate_maximum);

  for (int trial = 0; trial < clouds; ++trial) {
    ++campaign.attempted;
    int n = minimum_points
          + static_cast<int>(rng() % static_cast<unsigned long long>(
                maximum_points - minimum_points + 1));
    int order = 1 + static_cast<int>(rng() % static_cast<unsigned long long>(maximum_order));
    std::vector<mhgp::P3> points;
    if (fixture == "foreign_source_same_level") {
      // Deux arêtes disjointes de longueur 1. Leurs sphères de rang 2 ont le
      // même niveau exact 1/4 et déclenchent deux multifusions distinctes.
      points = {{0, 0, 2}, {2, 0, 0}, {0, 0, 1}, {1, 0, 0}};
      n = static_cast<int>(points.size());
      order = 1;
    } else if (fixture == "noncritical_shell_tie") {
      // Quatre points entiers cocycliques sur un arc court. Les triples portent
      // une coquille supplementaire mais ne sont pas bien centres : ce tie est
      // diagnostic, pas une degenerescence de sphère critique.
      points = {{1065, 1000, 100}, {1063, 1016, 100},
                {1060, 1025, 100}, {1056, 1033, 100}};
      n = static_cast<int>(points.size());
      order = 2;
    } else if (fixture == "nonminimal_contributor_same_level") {
      // Triangle entier equilateral (distances au carre egales a 2) : ses
      // trois arêtes contribuent au meme lot. Le point 3 est lointain.
      points = {{0, 0, 0}, {1, 1, 0}, {1, 0, 1}, {10, 10, 10}};
      n = static_cast<int>(points.size());
      order = 1;
    } else {
      points.resize(static_cast<std::size_t>(n));
      for (int i = 0; i < n; ++i)
        points[static_cast<std::size_t>(i)] =
            mhgp::P3{coordinate(rng), coordinate(rng), coordinate(rng)};
    }
    const int s_max = std::min(order + 1, n);

    int reference_degenerate = 0;
    const std::vector<ReferenceSphere> reference =
        reference_catalogue(points, s_max, &reference_degenerate);

    mhgp::Options options;
    options.k_max = order;
    options.threads = 1;
    options.all_orders = true;

    mhgp::Catalogue catalogue;
    std::vector<mhgp::Forest> forests;
    bool subject_out_of_domain = false;
    bool subject_suppressed = false;
    if (subject == "v2") {
      const mhgp::Result result = mhgp::run(points, options);
      catalogue = result.catalogue;
      forests = result.forests;
      subject_out_of_domain = result.out_of_declared_domain;
      subject_suppressed = result.forests_suppressed;
    } else if (subject == "edge_shallow") {
      mhgp3v::AnchoredCampaign anchored;
      mhgp3v::EdgeShallowStatistics shallow;
      catalogue = mhgp3v::edge_shallow_catalogue(points, s_max, &shallow, &anchored);
      subject_out_of_domain = anchored.degenerate_shells > 0;
      subject_suppressed = subject_out_of_domain;
      edge_shallow_total.edges_examined += shallow.edges_examined;
      edge_shallow_total.edges_retained += shallow.edges_retained;
      edge_shallow_total.lines_active += shallow.lines_active;
      edge_shallow_total.lines_constant_inside += shallow.lines_constant_inside;
      edge_shallow_total.vertices_examined += shallow.vertices_examined;
      edge_shallow_total.vertices_shallow += shallow.vertices_shallow;
      edge_shallow_total.emitted_arity_four += shallow.emitted_arity_four;
      edge_shallow_total.depth_tests += shallow.depth_tests;
      edge_shallow_total.dictionary_refuted += shallow.dictionary_refuted;
      if (!subject_out_of_domain)
        for (int k = 1; k <= order; ++k) forests.push_back(mhgp::build_forest(points, catalogue, k));
    } else {
      mhgp3v::AnchoredCampaign anchored;
      catalogue = mhgp3v::anchored_catalogue(points, s_max, seed_neighbours,
                                             regime == "exhaustive" ? mhgp3v::Regime::exhaustive
                                                                    : mhgp3v::Regime::assumed_window,
                                             &anchored);
      subject_out_of_domain = anchored.degenerate_shells > 0;
      subject_suppressed = subject_out_of_domain;
      anchored_total.candidates += anchored.candidates;
      anchored_total.witness_tests += anchored.witness_tests;
      anchored_total.emitted += anchored.emitted;
      anchored_total.anchor_member_pairs += anchored.anchor_member_pairs;
      anchored_total.incomplete_anchors += anchored.incomplete_anchors;
      anchored_total.exhaustive_anchors += anchored.exhaustive_anchors;
      anchored_total.sufficient_max = std::max(anchored_total.sufficient_max,
                                               anchored.sufficient_max);
      anchored_total.sufficient_mean += anchored.sufficient_mean;
      anchored_total.neighbourhood_max =
          std::max(anchored_total.neighbourhood_max, anchored.neighbourhood_max);
      anchored_total.neighbourhood_mean += anchored.neighbourhood_mean;
      if (!subject_out_of_domain)
        for (int k = 1; k <= order; ++k) forests.push_back(mhgp::build_forest(points, catalogue, k));
    }

    // ---- domaine declare, garde SYMETRIQUE --------------------------------
    const bool subject_out = subject_out_of_domain;
    const bool reference_out = reference_degenerate > 0;
    if (subject_out != reference_out) {
      campaign.fail(std::string("DOMAINE desaccord : sujet=") + (subject_out ? "hors" : "dans")
                    + " reference=" + (reference_out ? "hors" : "dans")
                    + " trial=" + std::to_string(trial));
      continue;
    }
    if (reference_out) {
      ++campaign.rejected_domain;
      if (!subject_suppressed || !forests.empty())
        campaign.fail("DOMAINE nuage hors domaine publie quand meme trial="
                      + std::to_string(trial));
      continue;
    }

    // Une censure inattendue n'est PAS un succes : la campagne est positive.
    if (subject_suppressed || forests.empty()) {
      campaign.fail("CENSURE inattendue dans une campagne positive trial="
                    + std::to_string(trial));
      continue;
    }
    if (static_cast<int>(forests.size()) != order) {
      campaign.fail("CENSURE nombre de forets " + std::to_string(forests.size())
                    + " contre " + std::to_string(order) + " trial=" + std::to_string(trial));
      continue;
    }

    if (!compare_catalogues(catalogue, reference, s_max, n, trial, &campaign)) continue;
    for (int k = 1; k <= order; ++k) {
      const ReferenceForest expected = reference_forest(points, k);
      compare_forests(catalogue, forests[static_cast<std::size_t>(k - 1)], expected, k, trial,
                      &campaign);
    }
    ++campaign.decided;

    // Campagne NEGATIVE : le probe hostile ne s'exerce que sur un sujet DEJA
    // vert, sur une copie, et sa comptabilite est distincte de la campagne
    // positive — un plancher ou un rejet de domaine ne peut donc pas servir de
    // preuve qu'une faute a ete attrapee.
    if (!injection.empty() && injections_applied == 0 && campaign.failures == 0) {
      const InjectionProbe probe =
          exercise_injection(injection, catalogue, forests, reference, points, s_max, order, trial);
      if (probe.applied) {
        ++injections_applied;
        if (probe.observed.failures != 1
            || probe.observed.failure_count(expected_injection_code) != 1)
          ++injection_escapes;
      }
    }
  }

  // ---- identite de fermeture ---------------------------------------------
  const bool closed = (campaign.attempted == campaign.decided + campaign.rejected_domain);
  if (!closed)
    campaign.fail("CAMPAGNE identite non fermee : attempted="
                  + std::to_string(campaign.attempted) + " decided="
                  + std::to_string(campaign.decided) + " rejected_domain="
                  + std::to_string(campaign.rejected_domain));
  if (campaign.decided < minimum_clouds_decided)
    campaign.fail("CAMPAGNE plancher de nuages decides non atteint ("
                  + std::to_string(campaign.decided) + ")");
  if (campaign.nodes < minimum_nodes)
    campaign.fail("CAMPAGNE plancher de noeuds compares non atteint ("
                  + std::to_string(campaign.nodes) + ")");

  for (const std::string& message : campaign.first_failures)
    std::printf("  %s\n", message.c_str());

  if (subject == "anchored" && campaign.decided > 0)
    std::printf("ancre[%s] : candidats=%lld temoins=%lld emis=%lld paires=%lld | fenetre "
                "moyenne=%.1f max=%d | fenetre SUFFISANTE moyenne=%.1f max=%d | ancres non "
                "exhaustives=%d\n", regime.c_str(),
                anchored_total.candidates, anchored_total.witness_tests, anchored_total.emitted,
                anchored_total.anchor_member_pairs,
                anchored_total.neighbourhood_mean / static_cast<double>(campaign.decided),
                anchored_total.neighbourhood_max,
                anchored_total.sufficient_mean / static_cast<double>(campaign.decided),
                anchored_total.sufficient_max, anchored_total.incomplete_anchors);

  if (subject == "edge_shallow" && campaign.decided > 0)
    std::printf("arete[profondeur] : aretes=%lld dont retenues=%lld | droites actives=%lld "
                "constantes interieures=%lld | sommets examines=%lld dont peu profonds=%lld "
                "| arite4 emise=%lld | tests de profondeur=%lld | DICTIONNAIRE REFUTE=%lld\n",
                edge_shallow_total.edges_examined, edge_shallow_total.edges_retained,
                edge_shallow_total.lines_active, edge_shallow_total.lines_constant_inside,
                edge_shallow_total.vertices_examined, edge_shallow_total.vertices_shallow,
                edge_shallow_total.emitted_arity_four, edge_shallow_total.depth_tests,
                edge_shallow_total.dictionary_refuted);

  std::printf("sujet=%s | attempted=%lld decided=%lld rejected_domain=%lld | spheres=%lld forets=%lld "
              "noeuds=%lld | largeur max=%zu bits | grille=[0,%lld]\n", subject.c_str(),
              campaign.attempted, campaign.decided, campaign.rejected_domain,
              campaign.catalogue_spheres, campaign.forests, campaign.nodes,
              campaign.widest_bits, static_cast<long long>(coordinate_maximum));

  if (!receipt_path.empty()) {
    std::FILE* file = std::fopen(receipt_path.c_str(), "w");
    if (file == nullptr) {
      std::printf("ECHEC : recu non ecrit (%s)\n", receipt_path.c_str());
      return 1;
    }
    const int written = std::fprintf(file,
        "{\n  \"schema\": \"morsehgp3d.v3.oracle.campaign.v1\",\n"
        "  \"subject\": \"%s\",\n"
        "  \"oracle_arithmetic\": \"arbitrary precision sign-magnitude base 2^32\",\n"
        "  \"oracle_geometry\": \"gauss elimination, not cramer\",\n"
        "  \"oracle_structure\": \"merge forest rebuilt from gamma_k\",\n"
        "  \"regime\": \"%s\",\n  \"fixture\": \"%s\",\n"
        "  \"injection\": \"%s\",\n  \"injections_applied\": %lld,\n"
        "  \"injection_escapes\": %lld,\n  \"seed_neighbours\": %d,\n"
        "  \"compiler\": \"" __VERSION__ "\",\n"
        "  \"built\": \"" __DATE__ " " __TIME__ "\",\n"
        "  \"seed\": %llu,\n  \"clouds_requested\": %d,\n"
        "  \"points\": [%d, %d],\n  \"maximum_order\": %d,\n"
        "  \"coordinate_maximum\": %lld,\n  \"declared_grid_maximum\": %lld,\n"
        "  \"attempted\": %lld,\n  \"decided\": %lld,\n  \"rejected_domain\": %lld,\n"
        "  \"identity_closed\": %s,\n"
        "  \"minimum_clouds_decided\": %lld,\n  \"minimum_nodes\": %lld,\n"
        "  \"catalogue_spheres_compared\": %lld,\n  \"forests_compared\": %lld,\n"
        "  \"canonical_nodes_compared\": %lld,\n"
        "  \"widest_exact_level_bits\": %zu,\n  \"failures\": %lld\n}\n",
        subject == "v2" ? "morsehgp3D_v2 build_catalogue + run" : "mhgp3v anchored_catalogue",
        regime.c_str(), fixture.c_str(), injection.c_str(), injections_applied,
        injection_escapes, seed_neighbours, seed, clouds, minimum_points, maximum_points,
        maximum_order,
        static_cast<long long>(coordinate_maximum), static_cast<long long>(mhgp::kCoordMax),
        campaign.attempted, campaign.decided, campaign.rejected_domain,
        closed ? "true" : "false", minimum_clouds_decided, minimum_nodes,
        campaign.catalogue_spheres, campaign.forests,
        campaign.nodes, campaign.widest_bits, campaign.failures);
    if (written < 0 || std::fflush(file) != 0 || std::ferror(file) != 0) {
      std::fclose(file);
      std::printf("ECHEC : ecriture du recu incomplete (%s)\n", receipt_path.c_str());
      return 1;
    }
    if (std::fclose(file) != 0) {
      std::printf("ECHEC : fermeture du recu en erreur (%s)\n", receipt_path.c_str());
      return 1;
    }
  }

  // Non-regression du faux certificat local : sous une fenetre supposee trop
  // etroite, le generateur DOIT se declarer incomplet. Si un certificat errone
  // etait reintroduit, ce compteur retomberait a zero.
  if (require_incomplete_anchors >= 0) {
    if (subject != "anchored") {
      std::printf("ECHEC : --require-incomplete-anchors exige --subject anchored\n");
      return 2;
    }
    if (anchored_total.incomplete_anchors < require_incomplete_anchors) {
      std::printf("ECHEC : %d ancres incompletes declarees, au moins %lld attendues — un "
                  "certificat de localite errone a-t-il ete reintroduit ?\n",
                  anchored_total.incomplete_anchors, require_incomplete_anchors);
      return 1;
    }
    // Cette fixture ne juge PAS la completude : sous fenetre supposee le
    // catalogue est legitimement incomplet, et les echecs ci-dessus le disent.
    // Elle verifie une seule chose : que le generateur le DECLARE.
    std::printf("OK : %d ancres se declarent incompletes sous fenetre supposee "
                "(la completude n'est pas jugee ici)\n", anchored_total.incomplete_anchors);
    return 0;
  }

  // Le dictionnaire de profondeur est l'enonce central mis a l'epreuve ici :
  // une seule refutation invalide la campagne, meme si tout le reste concorde.
  if (subject == "edge_shallow" && edge_shallow_total.dictionary_refuted != 0) {
    std::printf("ECHEC : dictionnaire rang = 4 + c_e + delta_e REFUTE %lld fois\n",
                edge_shallow_total.dictionary_refuted);
    return 1;
  }

  if (!injection.empty()) {
    if (campaign.failures != 0) {
      std::printf("ECHEC : la campagne SUPPORT doit etre verte avant tout probe hostile "
                  "(%lld echecs)\n", campaign.failures);
      return 1;
    }
    if (injections_applied != 1) {
      std::printf("ECHEC : l'injection \"%s\" devait etre appliquee exactement une fois, "
                  "observe=%lld\n", injection.c_str(), injections_applied);
      return 1;
    }
    if (injection_escapes != 0) {
      std::printf("ECHEC : %lld injections \"%s\" n'ont pas declenche exactement leur garde\n",
                  injection_escapes, injection.c_str());
      return 1;
    }
    std::printf("OK : faute \"%s\" appliquee exactement une fois, garde vise declenche "
                "exactement une fois, aucun diagnostic etranger\n", injection.c_str());
    return 0;
  }
  if (campaign.failures != 0) {
    std::printf("ECHEC : %lld\n", campaign.failures);
    return 1;
  }
  std::printf("OK : campagne fermee, structure complete comparee sur la grille declaree\n");
  return 0;
}
