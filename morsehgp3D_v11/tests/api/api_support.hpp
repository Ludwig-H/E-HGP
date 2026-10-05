// Aides des portes du module api (hors produit) : Session de test, nuages d'entree possedes, fixtures gravees de la
// specification (paragraphe 2.9, coordonnees entieres), dossier de travail jetable, lecture et inventaire de fichiers,
// egalite de forets.
#pragma once

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>
#include <vector>

#include "api/api.hpp"

namespace mhgp11::api_test {

// Session de test ; un refus de creation termine le processus (precondition des portes : arret anormal, jamais un vert).
inline api::Session session_of(u32 workers, u64 budget = MemoryBudget::kUnlimited) {
  Result<api::Session> made = api::Session::make({budget, workers});
  if (!made.ok()) std::terminate();
  return std::move(made).take();
}

// Nuage d'entree possede, dans l'ordre du fichier.
struct Points {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  api::CloudView view() const { return {x, y, z, ids}; }
  u32 size() const { return static_cast<u32>(x.size()); }
};

// Provenance coherente d'un nuage (12 et 4 octets par point), sans empreintes : le lecteur n'en controle que la forme.
inline api::Provenance provenance_of(const Points& p) {
  api::Provenance provenance;
  provenance.points_bytes = u64{12} * p.size();
  provenance.ids_bytes = u64{4} * p.size();
  return provenance;
}

inline Points points_of(std::initializer_list<std::array<u32, 3>> list, u32 first_id = 7) {
  Points p;
  u32 id = first_id;
  for (const auto& q : list) {
    p.x.push_back(q[0]);
    p.y.push_back(q[1]);
    p.z.push_back(q[2]);
    p.ids.push_back(make_id<PointId>(id));
    id += 3;
  }
  return p;
}

// Generateur congruentiel deterministe (aucune dependance a la bibliotheque standard pour les valeurs).
struct Lcg {
  u64 state;
  u32 next(u32 bound) {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<u32>((state >> 33) % bound);
  }
};

// n points distincts dans un cube de cote `side`, identifiants non denses.
inline Points random_points(u32 n, u32 side, u64 seed) {
  Points p;
  Lcg g{seed};
  std::vector<std::array<u32, 3>> seen;
  while (p.size() < n) {
    const std::array<u32, 3> q{g.next(side), g.next(side), g.next(side)};
    if (std::find(seen.begin(), seen.end(), q) != seen.end()) continue;
    seen.push_back(q);
    p.x.push_back(q[0]);
    p.y.push_back(q[1]);
    p.z.push_back(q[2]);
    p.ids.push_back(make_id<PointId>(0x9E3779B9u * (p.size() + 1)));
  }
  return p;
}

// Fixtures du paragraphe 2.9 de la specification (z = 0 sauf mention), translatees en coordonnees positives.
inline std::vector<Points> fixtures() {
  return {
      points_of({{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}),                          // carre
      points_of({{0, 0, 0}, {4, 0, 0}, {0, 3, 0}}),                                      // triangle droit
      points_of({{1, 8, 0}, {5, 10, 0}, {9, 8, 0}, {5, 0, 0}}),                          // growth_ABCZ
      points_of({{0, 0, 0}, {2, 2, 0}, {4, 0, 0}, {8, 0, 0}}),                           // passagere
      points_of({{0, 0, 0}, {2, 0, 0}, {1, 2, 0}}),                                      // triangle aigu
      points_of({{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}),                                      // ligne
      points_of({{0, 0, 0}, {2, 2, 0}, {2, 0, 2}}),                                      // equilateral
      points_of({{20, 20, 20}, {20, 0, 0}, {0, 20, 0}, {0, 0, 20}, {10, 10, 10}, {11, 10, 10}, {10, 11, 10}}),
      points_of({{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0}, {0, 0, 2}, {2, 0, 2}, {0, 2, 2}, {2, 2, 2}}),  // cube
      points_of({{0, 1, 1}, {2, 1, 1}, {1, 0, 1}, {1, 2, 1}, {1, 1, 0}, {1, 1, 2}}),     // octaedre
      points_of({{0, 0, 0}, {4, 0, 0}, {6, 0, 0}, {8, 0, 0}, {12, 0, 0}}),               // ligne 0 4 6 8 12
  };
}

// Dossier unique sous $TMPDIR (ou /tmp), retire en entier a la destruction.
class Scratch {
 public:
  Scratch() {
    const char* base = std::getenv("TMPDIR");
    std::string pattern = std::string(base != nullptr && *base != '\0' ? base : "/tmp") + "/mhgp11_api_XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    if (::mkdtemp(buffer.data()) != nullptr) root_ = buffer.data();
  }
  Scratch(const Scratch&) = delete;
  Scratch& operator=(const Scratch&) = delete;
  ~Scratch() {
    if (root_.empty()) return;
    std::error_code error;
    std::filesystem::remove_all(root_, error);
  }
  bool ok() const { return !root_.empty(); }
  std::string path(const std::string& name) const { return root_ + "/" + name; }

 private:
  std::string root_;
};

inline std::string read_text(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Present au sens de lstat.
inline bool exists(const std::string& path) {
  struct stat st {};
  return ::lstat(path.c_str(), &st) == 0;
}

// Noms des entrees d'un dossier, tries ; vide si le dossier manque.
inline std::vector<std::string> entries(const std::string& dir) {
  std::vector<std::string> names;
  std::error_code error;
  for (const auto& entry : std::filesystem::directory_iterator(dir, error))
    names.push_back(entry.path().filename().string());
  std::sort(names.begin(), names.end());
  return names;
}

// Memes noeuds (rang, parent, enfants, cle de naissance), memes enfants et memes verticales.
inline bool same_forest(const OrderForest& a, const OrderForest& b) {
  if (a.order() != b.order() || a.births() != b.births() || a.nodes().size() != b.nodes().size() ||
      a.edges().size() != b.edges().size() || a.lower().size() != b.lower().size() || a.root() != b.root() ||
      !std::equal(a.edges().begin(), a.edges().end(), b.edges().begin()) ||
      !std::equal(a.lower().begin(), a.lower().end(), b.lower().begin()))
    return false;
  for (u32 i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i];
    const auto& y = b.nodes()[i];
    if (x.rank != y.rank || x.parent != y.parent || x.child_count != y.child_count ||
        x.child_begin != y.child_begin || x.birth_key != y.birth_key)
      return false;
  }
  return true;
}

inline bool same_tower(const FullTower& a, const FullTower& b) {
  if (a.kmax() != b.kmax()) return false;
  for (u32 k = 1; k <= a.kmax(); ++k)
    if (!same_forest(a.order(static_cast<Order>(k)), b.order(static_cast<Order>(k)))) return false;
  return true;
}

}  // namespace mhgp11::api_test
