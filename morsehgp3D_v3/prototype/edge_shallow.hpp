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
  long long emitted_arity_two = 0;
  long long emitted_arity_three = 0;
  long long emitted_arity_four = 0;
  long long depth_tests = 0;
  // Le rang lu sur la profondeur contredit le nombre de membres reellement
  // presents. Ce compteur DOIT rester nul : s'il ne l'est pas, le dictionnaire
  // rang = 4 + c_e + delta_e est refute, et l'omettre en silence reviendrait a
  // masquer la refutation.
  long long dictionary_refuted = 0;
  long long degenerate_shells = 0;  // cospheries sur un candidat bien centre
  // Sans ces deux compteurs, un vert ne prouve que le cas trivial rang = arite :
  // profondeur nulle et aucune constante interieure. Ce sont eux qui disent que
  // le dictionnaire GENERAL a ete exerce.
  long long emitted_positive_depth = 0;
  long long emitted_positive_constant = 0;
  long long rank_histogram[16] = {};
  // Clipping de JUNG : ce que l'ellipse retire reellement.
  long long lines_total = 0;          // points examines, toutes ancres
  long long lines_outside_lens = 0;   // hors lentille : inelig. comme PORTEUR
  long long lines_constant_outside = 0;  // ne coupent pas l'ellipse, exterieures
  long long lines_constant_inside_clip = 0;  // ne coupent pas, interieures

  // Un compteur oublie ici est un compteur qui reste a zero dans le recu : le
  // reviewer lit alors « branche non exercee » la ou elle l'etait. C'est ce qui
  // vient d'arriver aux quatre compteurs de clipping. Le static_assert casse
  // donc le build des qu'un champ est ajoute sans etre somme ci-dessous.
  void absorb(const EdgeShallowStatistics& other) {
    static_assert(sizeof(EdgeShallowStatistics) == 18 * sizeof(long long) + 16 * sizeof(long long),
                  "champ ajoute a EdgeShallowStatistics : le sommer dans absorb()");
    edges_examined += other.edges_examined;
    edges_retained += other.edges_retained;
    lines_active += other.lines_active;
    lines_constant_inside += other.lines_constant_inside;
    vertices_examined += other.vertices_examined;
    vertices_shallow += other.vertices_shallow;
    emitted_arity_two += other.emitted_arity_two;
    emitted_arity_three += other.emitted_arity_three;
    emitted_arity_four += other.emitted_arity_four;
    depth_tests += other.depth_tests;
    dictionary_refuted += other.dictionary_refuted;
    degenerate_shells += other.degenerate_shells;
    emitted_positive_depth += other.emitted_positive_depth;
    emitted_positive_constant += other.emitted_positive_constant;
    lines_total += other.lines_total;
    lines_outside_lens += other.lines_outside_lens;
    lines_constant_outside += other.lines_constant_outside;
    lines_constant_inside_clip += other.lines_constant_inside_clip;
    for (int i = 0; i < 16; ++i) rank_histogram[i] += other.rank_histogram[i];
  }
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

// Comparaison exacte de deux positions N/D le long d'une droite, sans division.
// N < 2^106,8 et D < 2^70,2, donc le produit croise < 2^177 : au-dela de l'i128,
// mais tres en deca des 256 bits de mul128.
inline int compare_positions(mhgp::i128 na, mhgp::i128 da, mhgp::i128 nb, mhgp::i128 db) {
  int order = mhgp::big_cmp(mhgp::mul128(na, db), mhgp::mul128(nb, da));
  const bool opposite = (da > 0) != (db > 0);
  return opposite ? -order : order;
}

// Ordre des croisements le long d'une droite, SANS multiplication croisee.
//
// La droite i porte le parametre tau selon (-b_i, a_i). Le croisement avec j est
// en tau_ij = N_ij / (G_i D_ij) avec G_i = a_i^2 + b_i^2 > 0, D_ij = a_i b_j -
// a_j b_i et N_ij = c_j G_i - c_i (a_i a_j + b_i b_j). La difference tau_ij -
// tau_ik a donc pour numerateur N_ij D_ik - N_ik D_ij, et l'identite de LAGRANGE
//
//   |u_i|^2 (u_j ^ u_k) = (u_i . u_j)(u_i ^ u_k) - (u_i . u_k)(u_i ^ u_j)
//
// y factorise exactement G_i > 0. Il reste le determinant homogene des trois
// droites ecrites lambda = (a, b, -c) :
//
//   sgn(tau_ij - tau_ik) = sgn(det(lambda_i, lambda_j, lambda_k)) sgn(D_ij) sgn(D_ik).
//
// Le produit croise naif demandait environ 210 bits ; ce determinant reste sous
// 2^107,4 sur la grille u16 (|a|,|b| < 2^34,6 et |c| < 2^35,6 donnent |D| <
// 2^70,2, puis trois termes |c D| < 2^105,8). Le tri tient donc en i128 pur.
inline mhgp::i128 line_determinant(const Line& li, const Line& lj, const Line& lk) {
  const mhgp::i128 djk = lj.a * lk.b - lk.a * lj.b;
  const mhgp::i128 dik = li.a * lk.b - lk.a * li.b;
  const mhgp::i128 dij = li.a * lj.b - lj.a * li.b;
  return -li.c * djk + lj.c * dik - lk.c * dij;
}

inline int chirotope_order(const Line& li, const Line& lj, const Line& lk) {
  const mhgp::i128 det = line_determinant(li, lj, lk);
  if (det == 0) return 0;                       // concurrence : positions egales
  const mhgp::i128 dij = li.a * lj.b - lj.a * li.b;
  const mhgp::i128 dik = li.a * lk.b - lk.a * li.b;
  int sign = det > 0 ? 1 : -1;
  if (dij < 0) sign = -sign;
  if (dik < 0) sign = -sign;
  return sign;
}

// La droite n.t = c/4 coupe-t-elle l'ellipse de JUNG t^T G t <= R^2 ?
// Le minimum de t^T G t sous la contrainte vaut (c/4)^2 det(G) / Q, donc la
// condition est c^2 det(G) <= 16 R^2 Q. Avec R^2 = (gamma^2 - 1/4) D^2 :
//   arite 4, gamma^2 = 3/8 : R^2 = D^2/8   ->  c^2 detG <= 2 D^2 Q ;
//   arite 3, gamma^2 = 1/3 : R^2 = D^2/12  ->  3 c^2 detG <= 4 D^2 Q.
// Largeurs : c^2 < 2^71,2 et detG < 2^67,2, donc 256 bits par mul128.
inline bool crosses_jung_ellipse(mhgp::i128 c, mhgp::i128 determinant_gram, mhgp::i128 quadratic,
                                 mhgp::i128 squared_diameter, int arity) {
  const mhgp::i128 left = arity == 4 ? c * c : 3 * c * c;
  const mhgp::i128 right = arity == 4 ? 2 * squared_diameter : 4 * squared_diameter;
  return mhgp::big_cmp(mhgp::mul128(left, determinant_gram),
                       mhgp::mul128(right, quadratic)) <= 0;
}

struct Crossing {
  mhgp::i128 numerator = 0;    // position = numerator / determinant
  mhgp::i128 determinant = 0;
  std::size_t line = 0;
};

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

  // Classification, en deux etages distincts (audit 2 §2.2, §3.1) :
  //
  //   LENTILLE — un sommet du support est a distance au plus D de p ET de q,
  //   sinon (p,q) ne serait pas l'arete de diametre maximal. C'est un masque de
  //   PORTEUR : il dit qui peut engendrer un support, jamais qui compte dans la
  //   profondeur.
  //
  //   ELLIPSE DE JUNG — une droite qui ne coupe pas l'ellipse est CONSTANTE sur
  //   elle : interieure (elle grossit c_e) ou exterieure (elle disparait). Seules
  //   les droites actives demandent un travail par sommet. C'est la reduction
  //   que le prototype ne faisait pas : il ne declarait constante qu'une forme
  //   algebriquement constante, c'est-a-dire un point colineaire a l'ancre.
  //
  // Les ellipses des arites trois et quatre DIFFERENT, donc les classifications
  // aussi : une preuve d'arite quatre ne s'y propage pas.
  const mhgp::i128 gram11 = mhgp::p3_norm2(b1);
  const mhgp::i128 gram12 = mhgp::p3_dot(b1, b2);
  const mhgp::i128 gram22 = mhgp::p3_norm2(b2);
  const mhgp::i128 gram_determinant = gram11 * gram22 - gram12 * gram12;

  std::vector<detail::Line> every;          // flux TEMOIN complet
  std::vector<char> in_lens;                // masque PORTEUR
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
    every.push_back(line);
    const bool lens = detail::squared_distance(x, p) <= squared_diameter
                   && detail::squared_distance(x, q) <= squared_diameter;
    in_lens.push_back(lens ? 1 : 0);
    ++statistics->lines_total;
    if (!lens) ++statistics->lines_outside_lens;
  }

  // Classification par arite : constante interieure, constante exterieure, ou
  // active. Une forme algebriquement constante (a = b = 0) est le cas limite.
  auto classify = [&](int arity, int* constant_inside_out,
                      std::vector<std::size_t>* active_out) {
    *constant_inside_out = 0;
    active_out->clear();
    for (std::size_t i = 0; i < every.size(); ++i) {
      const detail::Line& line = every[i];
      const mhgp::i128 quadratic =
          gram22 * line.a * line.a - 2 * gram12 * line.a * line.b + gram11 * line.b * line.b;
      const bool degenerate = line.a == 0 && line.b == 0;
      if (degenerate || !detail::crosses_jung_ellipse(line.c, gram_determinant, quadratic,
                                                      squared_diameter, arity)) {
        if (line.c < 0) { ++(*constant_inside_out); ++statistics->lines_constant_inside_clip; }
        else { ++statistics->lines_constant_outside; }
        continue;
      }
      active_out->push_back(i);
    }
  };

  // ---- ARITE DEUX -------------------------------------------------------
  // Le support {p,q} a pour sphere la boule diametrale, de centre M donc s = 0.
  // La profondeur y est immediate sur le flux TEMOIN entier : 0 > c.
  {
    int depth = 0;
    for (const detail::Line& line : every) {
      ++statistics->depth_tests;
      if (line.c < 0) ++depth;
    }
    const int rank = 2 + depth;
    if (rank <= s_max) {
      const std::vector<mhgp::i32> support{std::min(first, second), std::max(first, second)};
      mhgp::Sphere sphere{};
      bool centred = false;
      if (detail::build_sphere(points, support, &sphere, &centred) && centred) {
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
        if (extra_on_shell) ++statistics->degenerate_shells;
        if (!extra_on_shell && on_shell == 2) {
          if (static_cast<int>(emitted.members.size()) != rank) {
            ++statistics->dictionary_refuted;
          } else {
            std::sort(emitted.members.begin(), emitted.members.end());
            emitted.support = support;
            emitted.sphere = sphere;
            emitted.rank = rank;
            out->push_back(std::move(emitted));
            ++statistics->emitted_arity_two;
            if (depth > 0) ++statistics->emitted_positive_depth;
            if (rank < 16) ++statistics->rank_histogram[rank];
          }
        }
      }
    }
  }

  bool retained = false;

  // ---- ARITE TROIS ------------------------------------------------------
  int constant_inside_three = 0;
  std::vector<std::size_t> active_three;
  classify(3, &constant_inside_three, &active_three);
  const int depth_budget_three = s_max - 3 - constant_inside_three;
  if (depth_budget_three >= 0) {
    for (std::size_t ii = 0; ii < active_three.size(); ++ii) {
      if (!in_lens[active_three[ii]]) continue;          // masque PORTEUR
      const detail::Line& lz = every[active_three[ii]];
      const mhgp::i128 v1 = gram22 * lz.a - gram12 * lz.b;
      const mhgp::i128 v2 = gram11 * lz.b - gram12 * lz.a;
      const mhgp::i128 denominator = lz.a * v1 + lz.b * v2;
      if (denominator <= 0) continue;

      int depth = 0;
      bool exceeded = false;
      for (std::size_t kk = 0; kk < active_three.size() && !exceeded; ++kk) {
        if (kk == ii) continue;
        ++statistics->depth_tests;
        const detail::Line& lk = every[active_three[kk]];
        const mhgp::i128 combined = lk.a * v1 + lk.b * v2;
        if (detail::compare_products(lz.c, combined, lk.c, denominator) > 0
            && ++depth > depth_budget_three)
          exceeded = true;
      }
      if (exceeded) continue;

      const int rank = 3 + constant_inside_three + depth;
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
      if (extra_on_shell) { ++statistics->degenerate_shells; continue; }
      if (on_shell != 3) continue;
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
      if (depth > 0) ++statistics->emitted_positive_depth;
      if (constant_inside_three > 0) ++statistics->emitted_positive_constant;
      if (rank < 16) ++statistics->rank_histogram[rank];
    }
  }

  // ---- ARITE QUATRE -----------------------------------------------------
  int constant_inside = 0;
  std::vector<std::size_t> active_index;
  classify(4, &constant_inside, &active_index);
  statistics->lines_active += static_cast<long long>(active_index.size());
  statistics->lines_constant_inside += constant_inside;
  const int depth_budget = s_max - 4 - constant_inside;
  if (depth_budget < 0) { if (retained) ++statistics->edges_retained; return; }

  std::vector<detail::Line> active;
  std::vector<char> active_lens;
  for (std::size_t index : active_index) {
    active.push_back(every[index]);
    active_lens.push_back(in_lens[index]);
  }

  // Balayage par droite : la profondeur le long d'une droite ne varie que de +-1
  // a chaque croisement, donc un tri puis un balayage donnent tous ses sommets.
  std::vector<detail::Crossing> crossings;
  for (std::size_t i = 0; i < active.size(); ++i) {
    if (!active_lens[i]) continue;                        // masque PORTEUR
    const detail::Line& li = active[i];
    crossings.clear();
    int running_depth = 0;
    for (std::size_t j = 0; j < active.size(); ++j) {
      if (j == i) continue;
      const detail::Line& lj = active[j];
      const mhgp::i128 determinant = li.a * lj.b - lj.a * li.b;
      if (determinant == 0) {
        const mhgp::i128 value = li.a != 0 ? (lj.a * li.c - li.a * lj.c) * (li.a > 0 ? 1 : -1)
                                           : (lj.b * li.c - li.b * lj.c) * (li.b > 0 ? 1 : -1);
        if (value > 0) ++running_depth;
        continue;
      }
      if (determinant < 0) ++running_depth;
      const mhgp::i128 t1 = li.c * lj.b - lj.c * li.b;
      const mhgp::i128 t2 = li.a * lj.c - lj.a * li.c;
      detail::Crossing crossing;
      crossing.numerator = -li.b * t1 + li.a * t2;
      crossing.determinant = determinant;
      crossing.line = j;
      crossings.push_back(crossing);
    }
    std::sort(crossings.begin(), crossings.end(),
              [&active, &li](const detail::Crossing& x, const detail::Crossing& y) {
                return detail::chirotope_order(li, active[x.line], active[y.line]) < 0;
              });
    statistics->depth_tests += static_cast<long long>(active.size());

    for (std::size_t t = 0; t < crossings.size(); ++t) {
      const detail::Crossing& crossing = crossings[t];
      const std::size_t j = crossing.line;
      const mhgp::i128 determinant = crossing.determinant;
      const int depth = running_depth - (determinant < 0 ? 1 : 0);
      running_depth = depth + (determinant > 0 ? 1 : 0);
      if (j < i || !active_lens[j]) continue;
      ++statistics->vertices_examined;
      if (depth > depth_budget) continue;
      ++statistics->vertices_shallow;

      const int rank = 4 + constant_inside + depth;
      std::vector<mhgp::i32> support{first, second, active[i].point, active[j].point};
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
      if (extra_on_shell) { ++statistics->degenerate_shells; continue; }
      if (on_shell != 4) continue;
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
      if (depth > 0) ++statistics->emitted_positive_depth;
      if (constant_inside > 0) ++statistics->emitted_positive_constant;
      if (rank < 16) ++statistics->rank_histogram[rank];
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

  (void)anchored_campaign;
  mhgp::Catalogue catalogue;
  std::vector<mhgp::CriticalSphere> kept;
  std::vector<mhgp::i32> members;

  // Arite 1 : la sphere de rayon nul en p. Son rang est le nombre de points
  // confondus avec p, donc 1 dans le domaine declare ; un doublon est une
  // degenerescence et sera vu comme coquille supplementaire.
  for (mhgp::i32 p = 0; p < n; ++p) {
    const mhgp::Sphere sphere = mhgp::sphere1(points[static_cast<std::size_t>(p)]);
    int rank = 0;
    for (mhgp::i32 z = 0; z < n; ++z)
      if (mhgp::sphere_side(sphere, points[static_cast<std::size_t>(z)]) <= 0) ++rank;
    if (rank != 1) { ++statistics->degenerate_shells; continue; }
    mhgp::CriticalSphere critical;
    critical.support[0] = p;
    for (int i = 1; i < mhgp::kMaxSupport; ++i) critical.support[i] = -1;
    critical.n_support = 1;
    critical.rank = 1;
    critical.sph = sphere;
    critical.beta = mhgp::sphere_beta(sphere);
    critical.members_begin = static_cast<mhgp::i32>(members.size());
    members.push_back(p);
    kept.push_back(critical);
  }

  // Arites 2, 3 et 4 : par profondeur, sur toutes les aretes (source d'ancres exhaustive,
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
