// MorseHGP3D v3 — M2.2, arite quatre par PROFONDEUR d'arrangement 2D.
//
// Ce composant teste la brique centrale de l'architecture proposee :
//
//     rang_ferme(p,q,z,w) = 4 + c_e + delta_e(t)
//
// ou e = (p,q) est l'arete d'ancrage, c_e le nombre de points CONSTAMMENT
// interieurs, et delta_e(t) le nombre de demi-plans actifs strictement positifs
// au sommet t. Il calcule donc le rang des supports de taille quatre
// EXCLUSIVEMENT par cette formule, sans jamais compter les points de la boule.
// Si le juge exhaustif est vert, le dictionnaire est verifie.
//
// Les arites 1, 2 et 3 empruntent encore le chemin ancre exhaustif : elles ont
// leurs propres regions, leurs propres seuils et leurs propres temoins, et une
// preuve d'arite quatre ne s'y propage pas (audit 2 §3.1). C'est le prochain
// incrementiel, pas une omission silencieuse.
//
// ALGEBRE. Dans le plan mediateur de e, avec M = (p+q)/2, D^2 = |q-p|^2, une
// base ENTIERE b1, b2 orthogonale a d = q-p, et c = M + B t :
//
//     h_x(t) = r^2 - |x - c|^2 = 2 (Bt).(x - M) - (|x - M|^2 - D^2/4),
//
// dont le signe dit interieur / coquille / exterieur. En posant X = 2x - p - q
// (entier) et en multipliant par 4, la forme devient AFFINE A COEFFICIENTS
// ENTIERS :
//
//     4 h_x = 4 [ (b1.X) t1 + (b2.X) t2 ] - (|X|^2 - D^2).
//
// On reparametre t = s/4 pour absorber le facteur : la droite de x est
//
//     a_x s1 + b_x s2 = c_x,  a_x = b1.X,  b_x = b2.X,  c_x = |X|^2 - D^2,
//
// et « x strictement interieur » equivaut a a_x s1 + b_x s2 > c_x.
//
// LARGEURS, sur la grille declaree (coordonnees < 2^16) : |d| < 2^16,8,
// |b1| <= |d| < 2^16,8, |b2| <= |d||b1| < 2^33,6, |X| < 2^17,8, d'ou
// |a| < 2^34,6, |b| < 2^51,4, |c| < 2^35,6. Au sommet de deux droites, le
// determinant est < 2^87, les numerateurs < 2^88, et le test de profondeur
// compare des entiers < 2^123,6 : tout tient dans un i128, sans allocation.
#pragma once

#include <algorithm>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/anchored_catalogue.hpp"

namespace mhgp3v {

struct EdgeShallowStatistics {
  long long edges_examined = 0;
  long long edges_retained = 0;      // arete diametrale d'au moins un support
  long long lines_active = 0;        // somme des m_e
  long long lines_constant_inside = 0;   // somme des c_e
  long long vertices_examined = 0;   // paires de droites actives
  long long vertices_shallow = 0;    // profondeur au plus s_max - 4 - c_e
  long long emitted_arity_three = 0;
  long long emitted_arity_four = 0;
  long long depth_tests = 0;
  // Le rang lu sur la profondeur contredit le nombre de membres reellement
  // presents. Ce compteur DOIT rester nul : s'il ne l'est pas, le dictionnaire
  // rang = 4 + c_e + delta_e est refute, et l'omettre en silence reviendrait a
  // masquer la refutation.
  long long dictionary_refuted = 0;
};

namespace detail {

// Base entiere du plan mediateur, EQUILIBREE.
//
// Le choix naturel b2 = d x b1 est orthogonal — donc de matrice de GRAM
// diagonale — mais il porte un facteur |d| de trop : |b2| = |d| |b1| ~ |d|^2.
// A l'arite quatre cela passe encore ; a l'arite trois le denominateur
// Q = a^2|b2|^2 + b^2|b1|^2 monte a 2^136,4 et ne tient plus dans un i128.
//
// On prend donc b1 = d x e1 et b2 = d x e2, ou e1 et e2 sont les deux axes
// AUTRES que la composante dominante de d. Les deux sont orthogonaux a d et de
// taille au plus |d| ; ils sont independants car (d x e1) x (d x e2) =
// d (d . (e1 x e2)) = +- d_{e3} d, non nul par construction. On perd
// l'orthogonalite — la GRAM n'est plus diagonale — mais elle reste un 2x2
// entier, et Q redescend sous 2^104.
inline void bisector_basis(const mhgp::P3& d, mhgp::P3* b1, mhgp::P3* b2) {
  const mhgp::i64 ax = d.x < 0 ? -d.x : d.x;
  const mhgp::i64 ay = d.y < 0 ? -d.y : d.y;
  const mhgp::i64 az = d.z < 0 ? -d.z : d.z;
  if (ax >= ay && ax >= az) {          // composante dominante : x
    *b1 = mhgp::p3_cross(d, mhgp::P3{0, 1, 0});
    *b2 = mhgp::p3_cross(d, mhgp::P3{0, 0, 1});
  } else if (ay >= az) {               // y
    *b1 = mhgp::p3_cross(d, mhgp::P3{1, 0, 0});
    *b2 = mhgp::p3_cross(d, mhgp::P3{0, 0, 1});
  } else {                             // z
    *b1 = mhgp::p3_cross(d, mhgp::P3{1, 0, 0});
    *b2 = mhgp::p3_cross(d, mhgp::P3{0, 1, 0});
  }
}

struct Line {
  mhgp::i128 a = 0, b = 0, c = 0;
  mhgp::i32 point = -1;
};

// Comparaison exacte de deux produits de trois facteurs, en 256 bits sans
// allocation : le test de profondeur de l'arite trois monte a 2^140,4, au-dela
// de l'i128, mais reste tres en deca des 256 bits de BigInt<4>.
inline int compare_products(mhgp::i128 left_a, mhgp::i128 left_b,
                            mhgp::i128 right_a, mhgp::i128 right_b) {
  return mhgp::big_cmp(mhgp::mul128(left_a, left_b), mhgp::mul128(right_a, right_b));
}

}  // namespace detail

// Enumere, pour l'arete (p,q) prise comme arete diametrale, les supports de
// taille quatre dont le rang ferme vaut au plus s_max — le rang etant obtenu
// PAR LA PROFONDEUR, jamais par un comptage de boule.
inline void edge_shallow_supports(const std::vector<mhgp::P3>& points, mhgp::i32 first,
                                  mhgp::i32 second, int s_max,
                                  std::vector<AnchoredSupport>* out,
                                  EdgeShallowStatistics* statistics) {
  const mhgp::P3& p = points[static_cast<std::size_t>(first)];
  const mhgp::P3& q = points[static_cast<std::size_t>(second)];
  const mhgp::P3 d = mhgp::p3_sub(q, p);
  const mhgp::i128 squared_diameter = mhgp::p3_norm2(d);
  if (squared_diameter == 0) return;  // points confondus : hors domaine
  ++statistics->edges_examined;

  mhgp::P3 b1{}, b2{};
  detail::bisector_basis(d, &b1, &b2);

  // Classification exacte : constamment interieur, constamment exterieur, ou
  // droite active. Une forme constante correspond a X parallele a d.
  int constant_inside = 0;
  std::vector<detail::Line> active;
  const int n = static_cast<int>(points.size());
  for (mhgp::i32 z = 0; z < n; ++z) {
    if (z == first || z == second) continue;
    const mhgp::P3& x = points[static_cast<std::size_t>(z)];
    const mhgp::P3 X{2 * x.x - p.x - q.x, 2 * x.y - p.y - q.y, 2 * x.z - p.z - q.z};
    detail::Line line;
    line.a = mhgp::p3_dot(b1, X);
    line.b = mhgp::p3_dot(b2, X);
    line.c = mhgp::p3_norm2(X) - squared_diameter;
    line.point = z;
    if (line.a == 0 && line.b == 0) {
      if (line.c < 0) ++constant_inside;  // 0 > c
      continue;
    }
    active.push_back(line);
  }
  statistics->lines_constant_inside += constant_inside;
  statistics->lines_active += static_cast<long long>(active.size());

  // ---- ARITE TROIS ------------------------------------------------------
  // Le circumcentre du triangle (p,q,z) est le point de la droite h_z = 0 situe
  // dans le plan du triangle, c'est-a-dire celui dont le deplacement est
  // parallele a la projection de X_z. En coordonnees s, cette direction est
  // adj(G) n ; le parametre vaut alors c / Q avec Q = n^T adj(G) n > 0.
  const mhgp::i128 g11 = mhgp::p3_norm2(b1);
  const mhgp::i128 g12 = mhgp::p3_dot(b1, b2);
  const mhgp::i128 g22 = mhgp::p3_norm2(b2);
  const int depth_budget_three = s_max - 3 - constant_inside;
  if (depth_budget_three >= 0) {
    for (std::size_t i = 0; i < active.size(); ++i) {
      const detail::Line& lz = active[i];
      const mhgp::i128 v1 = g22 * lz.a - g12 * lz.b;
      const mhgp::i128 v2 = g11 * lz.b - g12 * lz.a;
      const mhgp::i128 denominator = lz.a * v1 + lz.b * v2;   // > 0 si n != 0
      if (denominator <= 0) continue;

      int depth = 0;
      bool exceeded = false;
      for (std::size_t k = 0; k < active.size() && !exceeded; ++k) {
        if (k == i) continue;
        ++statistics->depth_tests;
        // a' s1 + b' s2 > c'  <=>  c (a' v1 + b' v2) > c' Q, avec Q > 0.
        const mhgp::i128 combined = active[k].a * v1 + active[k].b * v2;
        if (detail::compare_products(lz.c, combined, active[k].c, denominator) > 0
            && ++depth > depth_budget_three)
          exceeded = true;
      }
      if (exceeded) continue;

      const int rank = 3 + constant_inside + depth;
      std::vector<mhgp::i32> support{first, second, lz.point};
      std::sort(support.begin(), support.end());
      mhgp::Sphere sphere{};
      bool centred = false;
      if (!detail::build_sphere(points, support, &sphere, &centred)) continue;
      if (!centred) continue;

      AnchoredSupport emitted;
      int on_shell = 0;
      bool extra_on_shell = false;
      for (mhgp::i32 z = 0; z < n; ++z) {
        const int side = mhgp::sphere_side(sphere, points[static_cast<std::size_t>(z)]);
        if (side > 0) continue;
        if (side == 0) {
          ++on_shell;
          if (!std::binary_search(support.begin(), support.end(), z)) extra_on_shell = true;
        }
        emitted.members.push_back(z);
      }
      if (extra_on_shell || on_shell != 3) continue;
      if (static_cast<int>(emitted.members.size()) != rank) {
        ++statistics->dictionary_refuted;
        continue;
      }
      std::sort(emitted.members.begin(), emitted.members.end());
      emitted.support = support;
      emitted.sphere = sphere;
      emitted.rank = rank;
      out->push_back(std::move(emitted));
      ++statistics->emitted_arity_three;
    }
  }

  // ---- ARITE QUATRE -----------------------------------------------------
  // La sortie anticipee ci-dessous ne vaut QUE pour l'arite quatre : a
  // s_max = 3 aucun support quatre n'existe, mais les triangles, eux, existent.
  // Le bloc d'arite trois doit donc la preceder — il ne le faisait pas, et les
  // triangles disparaissaient silencieusement.
  const int depth_budget = s_max - 4 - constant_inside;
  if (depth_budget < 0) return;  // l'ancre ne peut porter aucun support quatre

  bool retained = false;
  for (std::size_t i = 0; i < active.size(); ++i) {
    for (std::size_t j = i + 1; j < active.size(); ++j) {
      const detail::Line& lx = active[i];
      const detail::Line& ly = active[j];
      const mhgp::i128 determinant = lx.a * ly.b - ly.a * lx.b;
      if (determinant == 0) continue;  // droites paralleles : aucun sommet
      ++statistics->vertices_examined;
      // s = (T1, T2) / determinant
      const mhgp::i128 t1 = lx.c * ly.b - ly.c * lx.b;
      const mhgp::i128 t2 = lx.a * ly.c - ly.a * lx.c;

      // PROFONDEUR : nombre de droites actives strictement positives au sommet.
      // a_z s1 + b_z s2 > c_z, multiplie par le determinant en respectant son
      // signe. Tout est entier.
      int depth = 0;
      bool exceeded = false;
      for (std::size_t k = 0; k < active.size() && !exceeded; ++k) {
        if (k == i || k == j) continue;
        ++statistics->depth_tests;
        const mhgp::i128 value = active[k].a * t1 + active[k].b * t2;
        const mhgp::i128 threshold = active[k].c * determinant;
        const bool strictly_inside =
            determinant > 0 ? (value > threshold) : (value < threshold);
        if (strictly_inside && ++depth > depth_budget) exceeded = true;
      }
      if (exceeded) continue;
      ++statistics->vertices_shallow;

      // Le rang est LU sur la profondeur, jamais compte dans la boule.
      const int rank = 4 + constant_inside + depth;

      std::vector<mhgp::i32> support{first, second, lx.point, ly.point};
      std::sort(support.begin(), support.end());
      mhgp::Sphere sphere{};
      bool centred = false;
      if (!detail::build_sphere(points, support, &sphere, &centred)) continue;
      if (!centred) continue;

      // La coquille et l'appartenance des membres restent des faits a etablir
      // exactement : seul le RANG vient de la profondeur. Les membres sont donc
      // relus, mais leur NOMBRE doit coincider avec le rang deja calcule — c'est
      // exactement le dictionnaire que ce prototype met a l'epreuve.
      AnchoredSupport emitted;
      int on_shell = 0;
      bool extra_on_shell = false;
      for (mhgp::i32 z = 0; z < n; ++z) {
        const int side = mhgp::sphere_side(sphere, points[static_cast<std::size_t>(z)]);
        if (side > 0) continue;
        if (side == 0) {
          ++on_shell;
          if (!std::binary_search(support.begin(), support.end(), z)) extra_on_shell = true;
        }
        emitted.members.push_back(z);
      }
      if (extra_on_shell || on_shell != 4) continue;
      if (static_cast<int>(emitted.members.size()) != rank) {
        ++statistics->dictionary_refuted;
        continue;
      }
      std::sort(emitted.members.begin(), emitted.members.end());
      emitted.support = support;
      emitted.sphere = sphere;
      emitted.rank = rank;
      out->push_back(std::move(emitted));
      ++statistics->emitted_arity_four;
      retained = true;
    }
  }
  if (retained) ++statistics->edges_retained;
}

// Catalogue hybride : arite quatre par profondeur sur ancres d'aretes, arites
// un a trois par le chemin ancre exhaustif. Le proprietaire canonique reste le
// plus petit identifiant du support, comme partout ailleurs.
inline mhgp::Catalogue edge_shallow_catalogue(const std::vector<mhgp::P3>& points, int s_max,
                                              EdgeShallowStatistics* statistics,
                                              AnchoredCampaign* anchored_campaign) {
  *statistics = EdgeShallowStatistics{};
  const int n = static_cast<int>(points.size());

  // Arites 1 a 3 : chemin ancre exhaustif, filtre a l'emission.
  mhgp::Catalogue catalogue =
      anchored_catalogue(points, s_max, n, Regime::exhaustive, anchored_campaign);
  std::vector<mhgp::CriticalSphere> kept;
  std::vector<mhgp::i32> members;
  for (const mhgp::CriticalSphere& sphere : catalogue.spheres) {
    if (sphere.n_support >= 3) continue;
    mhgp::CriticalSphere copy = sphere;
    copy.members_begin = static_cast<mhgp::i32>(members.size());
    members.insert(members.end(),
                   catalogue.members.begin() + sphere.members_begin,
                   catalogue.members.begin() + sphere.members_begin + sphere.rank);
    kept.push_back(copy);
  }

  // Arites 3 et 4 : par profondeur, sur toutes les aretes (source d'ancres exhaustive,
  // volontairement : ce prototype isole le constructeur, pas A1-source).
  std::vector<AnchoredSupport> found;
  for (mhgp::i32 a = 0; a < n; ++a)
    for (mhgp::i32 b = a + 1; b < n; ++b)
      edge_shallow_supports(points, a, b, s_max, &found, statistics);

  // Chaque support de taille quatre est vu depuis chacune de ses aretes qui est
  // diametrale ; on ne garde qu'une occurrence.
  std::sort(found.begin(), found.end(),
            [](const AnchoredSupport& x, const AnchoredSupport& y) { return x.support < y.support; });
  found.erase(std::unique(found.begin(), found.end(),
                          [](const AnchoredSupport& x, const AnchoredSupport& y) {
                            return x.support == y.support;
                          }),
              found.end());

  for (const AnchoredSupport& item : found) {
    mhgp::CriticalSphere critical;
    for (std::size_t i = 0; i < item.support.size(); ++i)
      critical.support[i] = item.support[i];
    critical.n_support = static_cast<mhgp::i32>(item.support.size());
    critical.rank = item.rank;
    critical.sph = item.sphere;
    critical.beta = mhgp::sphere_beta(item.sphere);
    critical.members_begin = static_cast<mhgp::i32>(members.size());
    members.insert(members.end(), item.members.begin(), item.members.end());
    kept.push_back(critical);
  }

  catalogue.spheres = std::move(kept);
  catalogue.members = std::move(members);
  std::sort(catalogue.spheres.begin(), catalogue.spheres.end(),
            [](const mhgp::CriticalSphere& x, const mhgp::CriticalSphere& y) {
              for (int i = 0; i < mhgp::kMaxSupport; ++i)
                if (x.support[i] != y.support[i]) return x.support[i] < y.support[i];
              return false;
            });
  return catalogue;
}

}  // namespace mhgp3v
