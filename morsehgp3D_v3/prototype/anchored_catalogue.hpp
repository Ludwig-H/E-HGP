// MorseHGP3D v3 — M2.1, SUJET DIFFERENTIEL ancre par point.
//
// AVERTISSEMENT — ce composant N'EST PAS un prototype de peeling, et il ne porte
// aucun certificat de localite. Il enumere tous les supports de taille au plus
// quatre dans une fenetre de voisins : c'est la cascade locale en
// somme_p C(|W_p|,3) que la proposition condamne. Il sert de SUJET borne pour le
// juge, et de mesure du travail reellement paye. Rien d'autre.
//
// UN CERTIFICAT FAUX A ETE RETIRE ICI (audit du 8 aout, §5.8). Il affirmait :
// « si 2*r_max des supports DEJA TROUVES n'atteint pas le premier voisin exclu,
// aucun support n'a pu etre manque ». L'implication est invalide : un support
// inconnu employant un point exclu verifie precisement 2r >= d_{M+1}, donc le
// maximum observe ne le borne pas. Contre-exemple reproduit par le juge, 22
// points, graine 4242 : 69 spheres au lieu de 70, support {6,10} manquant, et
// les 22 ancres se declaraient certifiees.
//
// Il n'existe pas de regle d'arret valide fondee sur les seules distances : la
// completude d'un generateur ancre exige soit l'EXHAUSTIVITE, soit un majorant
// de R(p) — le supremum des rayons de boules passant par p, de centre dans
// conv(X), de contenu au plus s_max, qui est fini (<= diam(X)) mais qu'il reste
// a calculer. C'est exactement l'obligation ouverte du plan.
//
// Deux regimes, nommes et jamais confondus :
//   - `exhaustive`      : la fenetre est le nuage entier ; le resultat est
//                         complet, et c'est la seule completude disponible ;
//   - `assumed_window`  : fenetre bornee ; le resultat est une HYPOTHESE
//                         declaree, jamais une certification.
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

enum class Regime { exhaustive, assumed_window };

struct AnchorStatistics {
  int anchor = -1;
  int neighbourhood = 0;        // |W_p| employe
  bool exhaustive = false;      // la fenetre EST le nuage : seule completude
  // Fenetre qui aurait SUFFI, calculee a posteriori depuis les supports
  // reellement emis : nombre de voisins a distance au plus 2 r_max. Ce n'est
  // une mesure honnete que sous le regime exhaustif, ou r_max est le vrai
  // maximum. Sous fenetre supposee, elle est un minorant.
  int sufficient_neighbours = 0;
  long long candidates = 0;     // sous-ensembles examines, CUMULE
  long long witness_tests = 0;  // tests de position (flux temoin), CUMULE
  long long emitted = 0;
  long long owned = 0;
  // Paires ancre-membre rencontrees. Ce ne sont PAS les 2-faces de A2p : le
  // prototype ne construit aucun arrangement.
  long long anchor_member_pairs = 0;
  long long degenerate_shells = 0;
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

// Vrai si la distance au carre donnee est AU PLUS le diametre au carre de la
// sphere : d2 <= 4 beta, soit d2 * den^2 <= 4 |num|^2. C'est le test « ce voisin
// est-il assez proche pour pouvoir appartenir a cette boule ».
inline bool within_diameter(const mhgp::Sphere& sphere, mhgp::i128 squared_distance_) {
  const mhgp::BigInt<4> numerator = mhgp::sphere_num2(sphere);
  const mhgp::BigInt<6> right = mhgp::big_mul_i128<6, 4>(numerator, 4);
  mhgp::BigInt<6> left = mhgp::big_mul_i128<6, 2>(
      mhgp::big_from_i128<2>(squared_distance_), sphere.den);
  left = mhgp::big_mul_i128<6, 6>(left, sphere.den);
  return mhgp::big_cmp(left, right) <= 0;
}

// Conserve pour memoire : 4 * beta(s) <= d2.
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
                              int seed_neighbours, Regime regime,
                              std::vector<AnchoredSupport>* out, AnchorStatistics* statistics) {
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
  // Les distances egales sont departagees par PointId : sans cela l'ordre, donc
  // la fenetre, dependrait de l'implementation du tri.
  std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
    if (a.first != b.first) return a.first < b.first;
    return a.second < b.second;
  });

  {
    const int budget = regime == Regime::exhaustive
                           ? static_cast<int>(ordered.size())
                           : std::max(seed_neighbours, s_max + 1);
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

    // ---- PAS DE CERTIFICAT ------------------------------------------------
    // On publie seulement la fenetre qui aurait SUFFI : le nombre de voisins a
    // distance au plus 2 r_max des supports emis. Sous le regime exhaustif,
    // r_max est le vrai maximum et ce nombre est la mesure recherchee.
    int sufficient = available;
    if (largest_valid) {
      sufficient = 0;
      for (int i = 0; i < static_cast<int>(ordered.size()); ++i) {
        if (!detail::within_diameter(largest, ordered[static_cast<std::size_t>(i)].first)) break;
        ++sufficient;
      }
    }

    statistics->neighbourhood = available;
    statistics->exhaustive = exhausted;
    statistics->sufficient_neighbours = sufficient;
    statistics->candidates += candidates;
    statistics->witness_tests += witness_tests;
    statistics->degenerate_shells += degenerate;
    statistics->emitted = static_cast<long long>(found.size());
    std::vector<std::pair<mhgp::i32, mhgp::i32>> pairs;
    for (const AnchoredSupport& item : found) {
      ++statistics->by_size[item.support.size()];
      if (item.support.front() == anchor) ++statistics->owned;
      for (mhgp::i32 u : item.support)
        if (u != anchor) pairs.emplace_back(std::min(anchor, u), std::max(anchor, u));
    }
    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
    statistics->anchor_member_pairs = static_cast<long long>(pairs.size());
    *out = std::move(found);
    // La completude n'est vraie QUE par exhaustivite.
    return exhausted;
  }
}

// Assemble le catalogue complet en n'emettant chaque support qu'a son
// proprietaire canonique — le plus petit identifiant de son support. Le resultat
// a exactement la forme du catalogue de production, donc le meme juge le compare.
struct AnchoredCampaign {
  long long candidates = 0;
  long long witness_tests = 0;
  long long emitted = 0;
  long long anchor_member_pairs = 0;
  int neighbourhood_max = 0;
  double neighbourhood_mean = 0.0;
  int sufficient_max = 0;
  double sufficient_mean = 0.0;
  long long degenerate_shells = 0;
  int incomplete_anchors = 0;   // ancres non exhaustives : resultat NON complet
  int exhaustive_anchors = 0;
  std::vector<AnchorStatistics> per_anchor;
};

inline mhgp::Catalogue anchored_catalogue(const std::vector<mhgp::P3>& points, int s_max,
                                          int seed_neighbours, Regime regime,
                                          AnchoredCampaign* campaign) {
  mhgp::Catalogue catalogue;
  *campaign = AnchoredCampaign{};
  const int n = static_cast<int>(points.size());
  long long total_neighbourhood = 0, total_sufficient = 0;

  std::vector<AnchoredSupport> supports;
  AnchorStatistics statistics;
  for (mhgp::i32 anchor = 0; anchor < n; ++anchor) {
    const bool complete =
        anchored_supports(points, anchor, s_max, seed_neighbours, regime, &supports, &statistics);
    if (!complete) ++campaign->incomplete_anchors;
    if (statistics.exhaustive) ++campaign->exhaustive_anchors;
    campaign->candidates += statistics.candidates;
    campaign->witness_tests += statistics.witness_tests;
    campaign->anchor_member_pairs += statistics.anchor_member_pairs;
    campaign->degenerate_shells += statistics.degenerate_shells;
    campaign->neighbourhood_max = std::max(campaign->neighbourhood_max, statistics.neighbourhood);
    campaign->sufficient_max = std::max(campaign->sufficient_max, statistics.sufficient_neighbours);
    total_neighbourhood += statistics.neighbourhood;
    total_sufficient += statistics.sufficient_neighbours;
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
  campaign->sufficient_mean = n > 0 ? static_cast<double>(total_sufficient) / n : 0.0;
  catalogue.neighbourhood_size.assign(static_cast<std::size_t>(n), 0);
  catalogue.growth_rounds.assign(static_cast<std::size_t>(n), 0);
  catalogue.certified.assign(static_cast<std::size_t>(n), 1u);
  for (int i = 0; i < n; ++i) {
    catalogue.neighbourhood_size[static_cast<std::size_t>(i)] =
        campaign->per_anchor[static_cast<std::size_t>(i)].neighbourhood;
    catalogue.growth_rounds[static_cast<std::size_t>(i)] = 0;
    catalogue.certified[static_cast<std::size_t>(i)] =
        campaign->per_anchor[static_cast<std::size_t>(i)].exhaustive ? 1u : 0u;
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
