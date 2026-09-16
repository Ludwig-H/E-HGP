#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <Eigen/Dense>

namespace hgp::enclosing_ball {

using Vec = Eigen::Matrix<double, Eigen::Dynamic, 1, 0, 32, 1>;
using Mat = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, 0, 32, 32>;

inline constexpr double kRelativeTolerance = 1e-10;
inline constexpr double kCoincidence = 1e-12;
inline constexpr std::size_t kMaxExhaustiveSize = 8;

inline bool covers(double squared_distance, double squared_radius) {
  return squared_distance <= squared_radius * (1.0 + kRelativeTolerance);
}

inline const double* point(const double* points, int index, std::size_t dim) {
  return &points[static_cast<std::size_t>(index) * dim];
}

inline double squared_distance(const double* p, const Vec& c, std::size_t dim) {
  double sum = 0.0;
  for (std::size_t k = 0; k < dim; ++k) {
    const double d = p[k] - c(k);
    sum += d * d;
  }
  return sum;
}

inline double squared_distance(const double* p, const double* q, std::size_t dim) {
  double sum = 0.0;
  for (std::size_t k = 0; k < dim; ++k) {
    const double d = p[k] - q[k];
    sum += d * d;
  }
  return sum;
}

inline void solve_basis(const double* points, const std::vector<int>& basis, std::size_t dim,
                        Vec& center, double& squared_radius) {
  const std::size_t m = basis.size();
  if (m == 0) {
    center.setZero(dim);
    squared_radius = 0.0;
    return;
  }
  const double* q0 = point(points, basis[0], dim);
  if (m == 1) {
    for (std::size_t k = 0; k < dim; ++k) {
      center(k) = q0[k];
    }
    squared_radius = 0.0;
    return;
  }

  const std::size_t n_vectors = m - 1;
  Mat V(dim, n_vectors);
  Vec b(n_vectors);
  for (std::size_t i = 0; i < n_vectors; ++i) {
    const double* qi = point(points, basis[i + 1], dim);
    double norm = 0.0;
    for (std::size_t k = 0; k < dim; ++k) {
      const double v = qi[k] - q0[k];
      V(k, i) = v;
      norm += v * v;
    }
    b(i) = norm;
  }

  const Mat G = V.transpose() * V;
  const Vec y = G.ldlt().solve(0.5 * b);
  const Vec offset = V * y;
  squared_radius = offset.squaredNorm();
  for (std::size_t k = 0; k < dim; ++k) {
    center(k) = q0[k] + offset(k);
  }
}

inline bool coincides_with_basis(const double* points, const double* p, const std::vector<int>& basis,
                                 std::size_t dim, double squared_radius) {
  return std::any_of(basis.begin(), basis.end(), [&](int q) {
    return squared_distance(p, point(points, q, dim), dim) <= kCoincidence * squared_radius;
  });
}

inline void welzl(const double* points, std::vector<int>& P, std::vector<int>& R, int n,
                  std::size_t dim, Vec& center, double& squared_radius) {
  if (n == 0 || R.size() >= dim + 1) {
    solve_basis(points, R, dim, center, squared_radius);
    return;
  }
  const int p = P[static_cast<std::size_t>(n - 1)];
  welzl(points, P, R, n - 1, dim, center, squared_radius);
  const double* x = point(points, p, dim);
  if (!covers(squared_distance(x, center, dim), squared_radius) &&
      !coincides_with_basis(points, x, R, dim, squared_radius)) {
    R.push_back(p);
    welzl(points, P, R, n - 1, dim, center, squared_radius);
    R.pop_back();
  }
}

inline void triangle_ball(const double* p0, const double* p1, const double* p2, std::size_t dim,
                          Vec& center, double& squared_radius) {
  double u2 = 0.0;
  double v2 = 0.0;
  double uv = 0.0;
  for (std::size_t k = 0; k < dim; ++k) {
    const double uk = p1[k] - p0[k];
    const double vk = p2[k] - p0[k];
    u2 += uk * uk;
    v2 += vk * vk;
    uv += uk * vk;
  }
  const double w2 = u2 + v2 - 2.0 * uv;

  const auto midpoint = [&](const double* a, const double* b, double length2) {
    for (std::size_t k = 0; k < dim; ++k) {
      center(k) = 0.5 * (a[k] + b[k]);
    }
    squared_radius = 0.25 * length2;
  };

  if (uv <= 0.0) {
    midpoint(p1, p2, w2);
    return;
  }
  if (u2 - uv <= 0.0) {
    midpoint(p0, p2, v2);
    return;
  }
  if (v2 - uv <= 0.0) {
    midpoint(p0, p1, u2);
    return;
  }

  const double det = u2 * v2 - uv * uv;
  if (det <= 1e-14 * u2 * v2) {
    const double longest = std::max({u2, v2, w2});
    if (u2 == longest) {
      midpoint(p0, p1, u2);
    } else if (v2 == longest) {
      midpoint(p0, p2, v2);
    } else {
      midpoint(p1, p2, w2);
    }
    return;
  }

  const double scale = 0.5 / det;
  const double alpha = v2 * (u2 - uv) * scale;
  const double beta = u2 * (v2 - uv) * scale;
  for (std::size_t k = 0; k < dim; ++k) {
    center(k) = p0[k] + alpha * (p1[k] - p0[k]) + beta * (p2[k] - p0[k]);
  }
  squared_radius = alpha * alpha * u2 + beta * beta * v2 + 2.0 * alpha * beta * uv;
}

inline double tetrahedron_squared_radius(const double* points, const int* indices,
                                         std::size_t dim) {
  const double* p[4] = {
      point(points, indices[0], dim),
      point(points, indices[1], dim),
      point(points, indices[2], dim),
      point(points, indices[3], dim),
  };

  Eigen::Matrix3d M;
  Eigen::Vector3d rhs;
  for (int i = 0; i < 3; ++i) {
    for (int j = i; j < 3; ++j) {
      double dot = 0.0;
      for (std::size_t k = 0; k < dim; ++k) {
        dot += (p[i + 1][k] - p[0][k]) * (p[j + 1][k] - p[0][k]);
      }
      M(i, j) = dot;
      M(j, i) = dot;
    }
    rhs(i) = 0.5 * M(i, i);
  }

  const Eigen::LDLT<Eigen::Matrix3d> solver = M.ldlt();
  if (solver.info() == Eigen::Success && solver.rcond() > 1e-10) {
    const Eigen::Vector3d coords = solver.solve(rhs);
    if (coords.minCoeff() >= -1e-10 && coords.sum() <= 1.0 + 1e-10) {
      return coords.dot(M * coords);
    }
  }

  static constexpr int faces[4][4] = {{1, 2, 3, 0}, {0, 2, 3, 1}, {0, 1, 3, 2}, {0, 1, 2, 3}};
  Vec center(dim);
  double squared_radius = 0.0;
  for (const auto& face : faces) {
    triangle_ball(p[face[0]], p[face[1]], p[face[2]], dim, center, squared_radius);
    if (covers(squared_distance(p[face[3]], center, dim), squared_radius)) {
      return squared_radius;
    }
  }

  std::vector<int> P(indices, indices + 4);
  std::vector<int> R;
  R.reserve(dim + 1);
  welzl(points, P, R, 4, dim, center, squared_radius);
  return squared_radius;
}

inline double largest_tetrahedron_squared_radius(const double* points, const int* indices,
                                                 std::size_t size, std::size_t dim) {
  double largest = 0.0;
  int subset[4];
  for (std::size_t a = 0; a < size; ++a) {
    subset[0] = indices[a];
    for (std::size_t b = a + 1; b < size; ++b) {
      subset[1] = indices[b];
      for (std::size_t c = b + 1; c < size; ++c) {
        subset[2] = indices[c];
        for (std::size_t d = c + 1; d < size; ++d) {
          subset[3] = indices[d];
          largest = std::max(largest, tetrahedron_squared_radius(points, subset, dim));
        }
      }
    }
  }
  return largest;
}

inline double squared_radius(const double* points, const int* indices, std::size_t size,
                             std::size_t dim) {
  if (size <= 1) {
    return 0.0;
  }
  if (size == 2) {
    return 0.25 * squared_distance(point(points, indices[0], dim), point(points, indices[1], dim), dim);
  }
  if (size == 3) {
    Vec center(dim);
    double r2 = 0.0;
    triangle_ball(point(points, indices[0], dim), point(points, indices[1], dim),
                  point(points, indices[2], dim), dim, center, r2);
    return r2;
  }
  if (size == 4) {
    return tetrahedron_squared_radius(points, indices, dim);
  }
  if (size <= kMaxExhaustiveSize && dim == 3) {
    return largest_tetrahedron_squared_radius(points, indices, size, dim);
  }

  std::vector<int> P(indices, indices + size);
  std::vector<int> R;
  R.reserve(dim + 1);
  if (size > 10) {
    for (std::size_t i = P.size() - 1; i > 0; --i) {
      std::swap(P[i], P[(i * 12345 + 6789) % (i + 1)]);
    }
  }
  Vec center(dim);
  double r2 = 0.0;
  welzl(points, P, R, static_cast<int>(P.size()), dim, center, r2);
  return r2;
}

}  // namespace hgp::enclosing_ball
