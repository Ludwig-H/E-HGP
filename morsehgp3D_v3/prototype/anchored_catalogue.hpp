// MorseHGP3D v3 — M2.1, generateur ancre par point, avec certificat de localite
// A POSTERIORI.
//
// Ce que ce composant change par rapport a la v2, et c'est le point :
//
//   la v2 dimensionne son voisinage par une borne A PRIORI (relaxation conique
//   du theoreme 4), qui vaut +infini des qu'un cone est trop pauvre — d'ou
//   |W_p| = n et le mur en Theta(n^5) ;
//
//   ici, aucune borne a priori. On travaille sur les M plus proches voisins,
//   puis on CERTIFIE : tout support U contenant p a sa circumboule passant par
//   p, donc tous ses membres sont a distance au plus 2r de p. Si 2*r_max des
//   supports emis est au plus la distance du premier voisin exclu, aucun support
//   n'a pu etre manque. Sinon on double M et on recommence.
//
// Le certificat est exact (comparaison entiere), il ne peut pas valoir l'infini,
// et le nombre de voisins reellement necessaire devient une MESURE au lieu d'une
// hypothese. C'est precisement le chiffre que les audits reclament.
//
// Deux flux, jamais confondus (audit 2 §2.2) :
//   - le flux TEMOIN, qui compte le rang ferme, porte sur TOUS les points du
//     voisinage, sans aucun filtre ;
//   - le masque PORTEUR ne concerne que les sommets candidats du support.
//
// L'arithmetique est celle du chemin de production candidat (`mhgp::sphere.hpp`,
// entiere et exacte). Le juge, lui, emploie Gauss en precision arbitraire : les
// deux ne partagent aucune primitive.
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {

struct AnchorStatistics {
  int anchor = -1;
  int neighbourhood = 0;        // |W_p| finalement certifie
  int growth_rounds = 0;        // doublements
  bool certified = false;       // 2 r_max <= distance du premier exclu
  bool exhausted = false;       // W_p a absorbe le nuage entier
  long long candidates = 0;     // sous-ensembles examines
  long long witness_tests = 0;  // tests de position (flux temoin)
  long long emitted = 0;        // supports emis, proprietaire canonique compris
  long long owned = 0;          // supports dont p est le proprietaire canonique
  long long two_faces = 0;      // paires {p,u} portant au moins un support
  long long degenerate_shells = 0;  // cospheries rencontrees : hors domaine declare
  long long by_size[5] = {0, 0, 0, 0, 0};
};

struct AnchoredSupport {
  std::vector<mhgp::i32> support;  // trie
  std::vector<mhgp::i32> members;  // trie
  mhgp::Sphere sphere{};
  int rank = 0;
};

namespace detail {

inline mhgp::i128 squared_distance(const mhgp::P3& a, const mhgp::P3& b) {
  const mhgp::P3 d = mhgp::p3_sub(a, b);
  return mhgp::p3_norm2(d);
}

// Construit la sphere portee par `support` et dit si elle est bien centree.
inline bool build_sphere(const std::vector<mhgp::P3>& points,
                         const std::vector<mhgp::i32>& support, mhgp::Sphere* out) {
  const std::size_t m = support.size();
  const mhgp::P3& a = points[static_cast<std::size_t>(support[0])];
  if (m == 1) { *out = mhgp::sphere1(a); return true; }
  const mhgp::P3& b = points[static_cast<std::size_t>(support[1])];
  if (m == 2) { *out = mhgp::sphere2(a, b); return true; }
  const mhgp::P3& c = points[static_cast<std::size_t>(support[2])];
  if (m == 3) {
    if (!mhgp::sphere3(a, b, c, out)) return false;
    return mhgp::well_centered3(a, b, c);
  }
  const mhgp::P3& d = points[static_cast<std::size_t>(support[3])];
  if (!mhgp::sphere4(a, b, c, d, out)) return false;
  return mhgp::well_centered4(*out, a, b, c, d);
}

// Comparaison exacte de 4 * beta(s) contre une distance au carre entiere :
// 4 * |num|^2 / den^2 <= d2, soit 4 |num|^2 <= d2 * den^2, en BigInt<6>.
inline bool diameter_squared_at_most(const mhgp::Sphere& sphere, mhgp::i128 squared_distance_) {
  const mhgp::BigInt<4> numerator = mhgp::sphere_num2(sphere);
  mhgp::BigInt<6> left = mhgp::big_mul_i128<6, 4>(numerator, 4);
  mhgp::BigInt<6> right = mhgp::big_mul_i128<6, 2>(
      mhgp::big_from_i128<2>(squared_distance_), sphere.den);
  right = mhgp::big_mul_i128<6, 6>(right, sphere.den);
  return mhgp::big_cmp(left, right) <= 0;
}

}  // namespace detail

// Enumere tous les supports minimaux bien centres contenant `anchor`, de rang
// ferme au plus `s_max`. Renvoie faux si le certificat de localite n'a pas pu
// etre etabli (le voisinage a absorbe le nuage sans le satisfaire) — dans ce cas
// le resultat reste complet, mais l'ancre est declaree `exhausted`.
inline bool anchored_supports(const std::vector<mhgp::P3>& points, mhgp::i32 anchor, int s_max,
                              int seed_neighbours, std::vector<AnchoredSupport>* out,
                              AnchorStatistics* statistics) {
  const int n = static_cast<int>(points.size());
  out->clear();
  *statistics = AnchorStatistics{};
  statistics->anchor = anchor;

  // Voisins tries par distance croissante a l'ancre : c'est l'ordre dans lequel
  // le certificat se ferme.
  std::vector<std::pair<mhgp::i128, mhgp::i32>> ordered;
  ordered.reserve(static_cast<std::size_t>(n - 1));
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == anchor) continue;
    ordered.emplace_back(detail::squared_distance(points[static_cast<std::size_t>(z)],
                                                  points[static_cast<std::size_t>(anchor)]), z);
  }
  std::sort(ordered.begin(), ordered.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

  int budget = std::max(seed_neighbours, s_max + 1);
  while (true) {
    ++statistics->growth_rounds;
    const int available = std::min<int>(budget, static_cast<int>(ordered.size()));
    const bool exhausted = available == static_cast<int>(ordered.size());

    // Le flux TEMOIN est le voisinage entier : aucun filtre ne le reduit.
    std::vector<mhgp::i32> window;
    window.reserve(static_cast<std::size_t>(available) + 1);
    window.push_back(anchor);
    for (int i = 0; i < available; ++i) window.push_back(ordered[static_cast<std::size_t>(i)].second);

    std::vector<AnchoredSupport> found;
    long long candidates = 0, witness_tests = 0, degenerate = 0;
    mhgp::i128 largest_diameter_squared = 0;
    bool largest_valid = false;
    mhgp::Sphere largest{};

    // Le masque PORTEUR : les sommets candidats sont pris dans la fenetre, mais
    // le comptage du rang, lui, balaie toute la fenetre.
    const int w = static_cast<int>(window.size());
    std::vector<mhgp::i32> support;
    for (int size = 1; size <= 4 && size <= w; ++size) {
      std::vector<int> index(static_cast<std::size_t>(size - 1));
      for (int i = 0; i + 1 < size; ++i) index[static_cast<std::size_t>(i)] = i + 1;
      while (true) {
        support.clear();
        support.push_back(anchor);
        for (int i = 0; i + 1 < size; ++i)
          support.push_back(window[static_cast<std::size_t>(index[static_cast<std::size_t>(i)])]);
        std::sort(support.begin(), support.end());
        ++candidates;

        mhgp::Sphere sphere{};
        if (detail::build_sphere(points, support, &sphere)) {
          AnchoredSupport emitted;
          int on_shell = 0;
          bool extra_on_shell = false;
          bool too_many = false;
          for (mhgp::i32 z : window) {
            ++witness_tests;
            const int side = mhgp::sphere_side(sphere, points[static_cast<std::size_t>(z)]);
            if (side > 0) continue;
            if (side == 0) {
              ++on_shell;
              if (!std::binary_search(support.begin(), support.end(), z)) extra_on_shell = true;
            }
            emitted.members.push_back(z);
            if (static_cast<int>(emitted.members.size()) > s_max) { too_many = true; break; }
          }
          if (extra_on_shell) ++degenerate;
          if (!too_many && !extra_on_shell && on_shell == size) {
            std::sort(emitted.members.begin(), emitted.members.end());
            emitted.support = support;
            emitted.sphere = sphere;
            emitted.rank = static_cast<int>(emitted.members.size());
            if (!largest_valid || mhgp::sphere_cmp_beta(sphere, largest) > 0) {
              largest = sphere;
              largest_valid = true;
            }
            found.push_back(std::move(emitted));
          }
        }

        if (size == 1) break;
        int i = size - 2;
        while (i >= 0 && index[static_cast<std::size_t>(i)] == w - (size - 1) + i) --i;
        if (i < 0) break;
        ++index[static_cast<std::size_t>(i)];
        for (int j = i + 1; j + 1 < size; ++j)
          index[static_cast<std::size_t>(j)] = index[static_cast<std::size_t>(j - 1)] + 1;
      }
    }
    (void)largest_diameter_squared;

    // ---- certificat de localite -----------------------------------------
    // Aucun support n'a pu etre manque si le diametre de la plus grande sphere
    // emise n'atteint pas le premier voisin exclu.
    bool certified = true;
    if (!exhausted && largest_valid) {
      const mhgp::i128 first_excluded = ordered[static_cast<std::size_t>(available)].first;
      certified = detail::diameter_squared_at_most(largest, first_excluded);
    }

    if (certified || exhausted) {
      statistics->neighbourhood = available;
      statistics->certified = certified;
      statistics->exhausted = exhausted;
      statistics->candidates = candidates;
      statistics->witness_tests = witness_tests;
      statistics->degenerate_shells = degenerate;
      statistics->emitted = static_cast<long long>(found.size());
      std::vector<std::pair<mhgp::i32, mhgp::i32>> faces;
      for (const AnchoredSupport& item : found) {
        ++statistics->by_size[item.support.size()];
        // Proprietaire canonique : le plus petit identifiant du support.
        if (item.support.front() == anchor) ++statistics->owned;
        for (mhgp::i32 u : item.support)
          if (u != anchor) faces.emplace_back(std::min(anchor, u), std::max(anchor, u));
      }
      std::sort(faces.begin(), faces.end());
      faces.erase(std::unique(faces.begin(), faces.end()), faces.end());
      statistics->two_faces = static_cast<long long>(faces.size());
      *out = std::move(found);
      return certified;
    }
    if (budget >= static_cast<int>(ordered.size())) {
      statistics->neighbourhood = available;
      statistics->exhausted = true;
      *out = std::move(found);
      return false;
    }
    budget *= 2;
  }
}

// Assemble le catalogue complet en n'emettant chaque support qu'a son
// proprietaire canonique — le plus petit identifiant de son support. Le resultat
// a exactement la forme du catalogue de production, donc le meme juge le compare.
struct AnchoredCampaign {
  long long candidates = 0;
  long long witness_tests = 0;
  long long emitted = 0;
  long long two_faces = 0;
  int neighbourhood_max = 0;
  double neighbourhood_mean = 0.0;
  long long degenerate_shells = 0;
  int uncertified_anchors = 0;
  int exhausted_anchors = 0;
  std::vector<AnchorStatistics> per_anchor;
};

inline mhgp::Catalogue anchored_catalogue(const std::vector<mhgp::P3>& points, int s_max,
                                          int seed_neighbours, AnchoredCampaign* campaign) {
  mhgp::Catalogue catalogue;
  *campaign = AnchoredCampaign{};
  const int n = static_cast<int>(points.size());
  long long total_neighbourhood = 0;

  std::vector<AnchoredSupport> supports;
  AnchorStatistics statistics;
  for (mhgp::i32 anchor = 0; anchor < n; ++anchor) {
    const bool certified =
        anchored_supports(points, anchor, s_max, seed_neighbours, &supports, &statistics);
    if (!certified) ++campaign->uncertified_anchors;
    if (statistics.exhausted) ++campaign->exhausted_anchors;
    campaign->candidates += statistics.candidates;
    campaign->witness_tests += statistics.witness_tests;
    campaign->two_faces += statistics.two_faces;
    campaign->degenerate_shells += statistics.degenerate_shells;
    campaign->neighbourhood_max = std::max(campaign->neighbourhood_max, statistics.neighbourhood);
    total_neighbourhood += statistics.neighbourhood;
    campaign->per_anchor.push_back(statistics);

    for (const AnchoredSupport& item : supports) {
      if (item.support.front() != anchor) continue;  // proprietaire canonique
      mhgp::CriticalSphere critical;
      for (std::size_t i = 0; i < item.support.size(); ++i)
        critical.support[i] = item.support[i];
      for (std::size_t i = item.support.size(); i < mhgp::kMaxSupport; ++i)
        critical.support[i] = -1;
      critical.n_support = static_cast<mhgp::i32>(item.support.size());
      critical.rank = item.rank;
      critical.sph = item.sphere;
      critical.beta = mhgp::sphere_beta(item.sphere);
      critical.members_begin = static_cast<mhgp::i32>(catalogue.members.size());
      catalogue.members.insert(catalogue.members.end(), item.members.begin(), item.members.end());
      catalogue.spheres.push_back(critical);
      ++campaign->emitted;
    }
  }
  campaign->neighbourhood_mean = n > 0 ? static_cast<double>(total_neighbourhood) / n : 0.0;
  catalogue.neighbourhood_size.assign(static_cast<std::size_t>(n), 0);
  catalogue.growth_rounds.assign(static_cast<std::size_t>(n), 0);
  catalogue.certified.assign(static_cast<std::size_t>(n), 1u);
  for (int i = 0; i < n; ++i) {
    catalogue.neighbourhood_size[static_cast<std::size_t>(i)] =
        campaign->per_anchor[static_cast<std::size_t>(i)].neighbourhood;
    catalogue.growth_rounds[static_cast<std::size_t>(i)] =
        campaign->per_anchor[static_cast<std::size_t>(i)].growth_rounds;
    catalogue.certified[static_cast<std::size_t>(i)] =
        campaign->per_anchor[static_cast<std::size_t>(i)].certified ? 1u : 0u;
  }
  std::sort(catalogue.spheres.begin(), catalogue.spheres.end(),
            [](const mhgp::CriticalSphere& a, const mhgp::CriticalSphere& b) {
              for (int i = 0; i < mhgp::kMaxSupport; ++i)
                if (a.support[i] != b.support[i]) return a.support[i] < b.support[i];
              return false;
            });
  return catalogue;
}

}  // namespace mhgp3v
