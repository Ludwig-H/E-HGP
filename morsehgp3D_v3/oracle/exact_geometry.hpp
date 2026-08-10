// MorseHGP3D v3 — geometrie rationnelle EXACTE de l'ORACLE.
//
// Extrait de `oracle_main.cpp` pour etre partage entre le juge M1 et le juge
// Gamma_k, SANS rien changer aux definitions : ces fonctions sont celles que
// les campagnes M1 ont deja exercees. La regle du dossier tient : l'oracle
// n'emprunte a la production aucune primitive — vecteurs, Gauss, spheres,
// miniboule et enumerations sont ecrits ici, sur l'arithmetique de l'oracle.
#pragma once

#include <functional>
#include <vector>

#include "bigint.hpp"
#include "rational.hpp"
#include "mhgp/mhgp.hpp"

namespace mhgp3v {
namespace geometry {

// ---------------------------------------------------------------------------
// Vecteurs rationnels
// ---------------------------------------------------------------------------
struct Vec3 {
  Rational x, y, z;
};

inline Vec3 subtract(const Vec3& a, const Vec3& b) {
  return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}
inline Vec3 add(const Vec3& a, const Vec3& b) {
  return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}
inline Vec3 scale(const Vec3& a, const Rational& s) {
  return Vec3{a.x * s, a.y * s, a.z * s};
}
inline Rational dot(const Vec3& a, const Vec3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 of_point(const mhgp::P3& p) {
  return Vec3{Rational(BigInt(static_cast<long long>(p.x))),
              Rational(BigInt(static_cast<long long>(p.y))),
              Rational(BigInt(static_cast<long long>(p.z)))};
}

// ---------------------------------------------------------------------------
// Elimination de GAUSS sur les rationnels. Renvoie faux si la matrice est
// singuliere, ce qui signale une dependance affine du sous-ensemble.
// ---------------------------------------------------------------------------
inline bool gauss_solve(std::vector<std::vector<Rational>> matrix, std::vector<Rational> rhs,
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

inline RationalSphere sphere_through(const std::vector<mhgp::P3>& points,
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
inline int side_of(const RationalSphere& sphere, const mhgp::P3& z) {
  const Vec3 d = subtract(of_point(z), sphere.centre);
  return compare(dot(d, d), sphere.squared_radius);
}

inline bool well_centred(const RationalSphere& sphere) {
  for (const Rational& lambda : sphere.barycentric)
    if (lambda.sign() <= 0) return false;
  return true;
}

// ---------------------------------------------------------------------------
// Enumeration des sous-ensembles
// ---------------------------------------------------------------------------
inline void each_subset(int n, int size,
                        const std::function<void(const std::vector<int>&)>& visit) {
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
inline bool exact_miniball(const std::vector<mhgp::P3>& points, const std::vector<int>& family,
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

}  // namespace geometry
}  // namespace mhgp3v
