#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include <geogram/basic/algorithm.h>
#include <geogram/basic/command_line.h>
#include <geogram/basic/command_line_args.h>
#include <geogram/basic/common.h>
#include <geogram/basic/logger.h>
#include <geogram/basic/process.h>
#include <geogram/delaunay/delaunay.h>

#include "enclosing_ball.hpp"

namespace py = pybind11;

namespace hgp {

constexpr std::size_t kDimension = 3;
constexpr std::size_t kMaxOrder = 63;
constexpr double kSiteJitter = 1e-12;

using Index = std::int32_t;
using Edge = std::pair<Index, Index>;

class ErrorSlot {
 public:
  template <typename Body>
  void guard(Body&& body) {
    if (failed_.load(std::memory_order_relaxed)) {
      return;
    }
    try {
      body();
    } catch (...) {
      const std::lock_guard<std::mutex> lock(mutex_);
      if (!error_) {
        error_ = std::current_exception();
      }
      failed_.store(true, std::memory_order_relaxed);
    }
  }

  void rethrow() const {
    if (error_) {
      std::rethrow_exception(error_);
    }
  }

 private:
  std::atomic<bool> failed_{false};
  std::mutex mutex_;
  std::exception_ptr error_;
};

inline std::uint64_t mix(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

template <typename Body>
void parallel_for(std::size_t count, Body body) {
  if (count > std::numeric_limits<GEO::index_t>::max()) {
    throw std::length_error("too many elements for a Geogram parallel loop");
  }
  ErrorSlot errors;
  GEO::parallel_for_slice(
      0, static_cast<GEO::index_t>(count), [&](GEO::index_t from, GEO::index_t to) {
        errors.guard([&] {
          for (GEO::index_t i = from; i < to; ++i) {
            body(static_cast<std::size_t>(i));
          }
        });
      });
  errors.rethrow();
}

void initialize_geogram() {
  static std::once_flag flag;
  std::call_once(flag, [] {
    GEO::initialize(GEO::GEOGRAM_INSTALL_NONE);
    GEO::CmdLine::import_arg_group("standard");
    GEO::CmdLine::import_arg_group("algo");
    GEO::CmdLine::set_arg("sys:multithread", true);
    GEO::Logger::instance()->set_quiet(true);
  });
}

std::vector<Edge> delaunay_edges(const double* vertices, std::size_t count,
                                 GEO::coord_index_t dimension) {
  if (count < kDimension + 1) {
    throw std::logic_error("a 3D triangulation needs at least 4 vertices");
  }
  if (count > std::numeric_limits<GEO::index_t>::max() / 28) {
    throw std::length_error("too many vertices for a Geogram triangulation");
  }
  GEO::Delaunay_var delaunay = GEO::Delaunay::create(dimension, "PDEL");
  if (delaunay.is_null() || delaunay->cell_size() != kDimension + 1) {
    throw std::runtime_error("Geogram PDEL is not available for this dimension");
  }
  delaunay->set_keeps_infinite(false);
  delaunay->set_stores_neighbors(false);
  delaunay->set_stores_cicl(false);
  delaunay->set_reorder(true);
  delaunay->set_vertices(static_cast<GEO::index_t>(count), vertices);

  const GEO::Delaunay* mesh = delaunay.get();
  const GEO::index_t n = static_cast<GEO::index_t>(count);
  const std::size_t cells = mesh->nb_cells();
  std::vector<Edge> edges(6 * cells);
  parallel_for(cells, [&](std::size_t cell) {
    const auto c = static_cast<GEO::index_t>(cell);
    Edge* out = &edges[6 * cell];
    for (GEO::index_t i = 0; i < 4; ++i) {
      const GEO::index_t a = mesh->cell_vertex(c, i);
      for (GEO::index_t j = i + 1; j < 4; ++j) {
        const GEO::index_t b = mesh->cell_vertex(c, j);
        *out++ = (a < n && b < n)
                     ? Edge(static_cast<Index>(std::min(a, b)), static_cast<Index>(std::max(a, b)))
                     : Edge(-1, -1);
      }
    }
  });

  GEO::sort(edges.begin(), edges.end());
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
  if (!edges.empty() && edges.front().first < 0) {
    edges.erase(edges.begin());
  }
  return edges;
}

std::vector<Edge> power_diagram_edges(const std::vector<double>& centers,
                                      const std::vector<double>& weights) {
  const std::size_t count = weights.size();
  const double max_weight = *std::max_element(weights.begin(), weights.end());
  std::vector<double> lifted(count * (kDimension + 1));
  parallel_for(count, [&](std::size_t i) {
    for (std::size_t d = 0; d < kDimension; ++d) {
      lifted[i * (kDimension + 1) + d] = centers[i * kDimension + d];
    }
    lifted[i * (kDimension + 1) + kDimension] = std::sqrt(std::max(max_weight - weights[i], 0.0));
  });
  return delaunay_edges(lifted.data(), count, kDimension + 1);
}

std::vector<Index> next_order(const double* points, const std::vector<Index>& sets,
                              std::size_t size) {
  const std::size_t count = sets.size() / size;
  const double inverse = 1.0 / static_cast<double>(size);

  std::vector<double> centers(count * kDimension);
  std::vector<double> weights(count);
  parallel_for(count, [&](std::size_t i) {
    const Index* s = &sets[i * size];
    double* c = &centers[i * kDimension];
    for (std::size_t d = 0; d < kDimension; ++d) {
      double sum = 0.0;
      for (std::size_t v = 0; v < size; ++v) {
        sum += points[static_cast<std::size_t>(s[v]) * kDimension + d];
      }
      c[d] = sum * inverse;
    }
    double spread = 0.0;
    std::uint64_t hash = size;
    for (std::size_t v = 0; v < size; ++v) {
      const double* p = &points[static_cast<std::size_t>(s[v]) * kDimension];
      for (std::size_t d = 0; d < kDimension; ++d) {
        spread += (p[d] - c[d]) * (p[d] - c[d]);
      }
      hash = mix(hash ^ static_cast<std::uint64_t>(s[v]));
    }
    weights[i] = -spread * inverse;

    const double amplitude =
        kSiteJitter * std::max({1.0, std::abs(c[0]), std::abs(c[1]), std::abs(c[2])});
    for (std::size_t d = 0; d < kDimension; ++d) {
      hash = mix(hash + d);
      c[d] += amplitude * (static_cast<double>(hash >> 11) * 0x1.0p-52 - 1.0);
    }
  });

  const std::vector<Edge> adjacency = power_diagram_edges(centers, weights);
  std::vector<double>().swap(centers);
  std::vector<double>().swap(weights);

  const std::size_t next = size + 1;
  std::vector<Index> candidates(adjacency.size() * next);
  std::vector<char> valid(adjacency.size());
  parallel_for(adjacency.size(), [&](std::size_t e) {
    const Index* a = &sets[static_cast<std::size_t>(adjacency[e].first) * size];
    const Index* b = &sets[static_cast<std::size_t>(adjacency[e].second) * size];
    Index merged[kMaxOrder + 2];
    std::size_t ia = 0;
    std::size_t ib = 0;
    std::size_t im = 0;
    while (ia < size && ib < size && im <= next) {
      if (a[ia] < b[ib]) {
        merged[im++] = a[ia++];
      } else if (b[ib] < a[ia]) {
        merged[im++] = b[ib++];
      } else {
        merged[im++] = a[ia++];
        ++ib;
      }
    }
    while (ia < size && im <= next) {
      merged[im++] = a[ia++];
    }
    while (ib < size && im <= next) {
      merged[im++] = b[ib++];
    }
    valid[e] = (im == next);
    if (valid[e]) {
      std::copy(merged, merged + next, &candidates[e * next]);
    }
  });

  std::size_t kept = 0;
  for (std::size_t e = 0; e < adjacency.size(); ++e) {
    if (valid[e]) {
      if (kept != e) {
        std::copy_n(&candidates[e * next], next, &candidates[kept * next]);
      }
      ++kept;
    }
  }
  candidates.resize(kept * next);
  std::vector<char>().swap(valid);

  const std::size_t n_candidates = candidates.size() / next;
  std::vector<std::size_t> order(n_candidates);
  std::iota(order.begin(), order.end(), std::size_t{0});
  const auto less = [&](std::size_t i, std::size_t j) {
    return std::lexicographical_compare(&candidates[i * next], &candidates[i * next] + next,
                                        &candidates[j * next], &candidates[j * next] + next);
  };
  GEO::sort(order.begin(), order.end(), less);

  std::vector<Index> result;
  result.reserve(candidates.size());
  for (std::size_t k = 0; k < n_candidates; ++k) {
    const Index* current = &candidates[order[k] * next];
    if (k > 0 && std::equal(current, current + next, &candidates[order[k - 1] * next])) {
      continue;
    }
    result.insert(result.end(), current, current + next);
  }
  return result;
}

std::vector<double> squared_radii(const double* points, const std::vector<Index>& sets,
                                  std::size_t size) {
  const std::size_t n_sets = sets.size() / size;
  std::vector<double> radii(n_sets);
  parallel_for(n_sets, [&](std::size_t i) {
    radii[i] = enclosing_ball::squared_radius(points, &sets[i * size], size, kDimension);
  });
  return radii;
}

std::vector<Index> all_subsets(std::size_t count, std::size_t size) {
  std::vector<Index> sets;
  std::vector<Index> combination(size);
  std::iota(combination.begin(), combination.end(), Index{0});
  while (true) {
    sets.insert(sets.end(), combination.begin(), combination.end());
    std::size_t i = size;
    while (i > 0 && static_cast<std::size_t>(combination[i - 1]) == count - size + i - 1) {
      --i;
    }
    if (i == 0) {
      return sets;
    }
    ++combination[i - 1];
    for (std::size_t j = i; j < size; ++j) {
      combination[j] = combination[j - 1] + 1;
    }
  }
}

std::pair<std::vector<Index>, std::vector<double>> order_k_delaunay(const double* points,
                                                                    std::size_t count,
                                                                    std::size_t order) {
  static std::mutex mutex;
  const std::lock_guard<std::mutex> lock(mutex);
  initialize_geogram();

  const std::size_t size = order + 1;
  if (size > count) {
    return {};
  }
  if (count <= kDimension) {
    std::vector<Index> sets = all_subsets(count, size);
    std::vector<double> radii = squared_radii(points, sets, size);
    return {std::move(sets), std::move(radii)};
  }

  std::vector<Index> sets;
  {
    const std::vector<Edge> edges = delaunay_edges(points, count, kDimension);
    sets.reserve(2 * edges.size());
    for (const Edge& e : edges) {
      sets.push_back(e.first);
      sets.push_back(e.second);
    }
  }
  for (std::size_t current = 2; current < size && !sets.empty(); ++current) {
    sets = next_order(points, sets, current);
  }

  std::vector<double> radii = squared_radii(points, sets, size);
  return {std::move(sets), std::move(radii)};
}

}  // namespace hgp

py::tuple order_k_delaunay(
    py::array_t<double, py::array::c_style | py::array::forcecast> points, int K) {
  if (points.ndim() != 2 || static_cast<std::size_t>(points.shape(1)) != hgp::kDimension) {
    throw std::invalid_argument("points must have shape (n_samples, 3)");
  }
  if (K < 1 || static_cast<std::size_t>(K) > hgp::kMaxOrder) {
    throw std::invalid_argument("K must be between 1 and 63");
  }
  const auto count = static_cast<std::size_t>(points.shape(0));
  if (count > static_cast<std::size_t>(INT32_MAX)) {
    throw std::invalid_argument("too many points");
  }
  const double* data = points.data();
  if (!std::all_of(data, data + count * hgp::kDimension, [](double x) { return std::isfinite(x); })) {
    throw std::invalid_argument("points must be finite");
  }
  const auto order = static_cast<std::size_t>(K);

  std::pair<std::vector<hgp::Index>, std::vector<double>> result;
  {
    py::gil_scoped_release release;
    result = hgp::order_k_delaunay(data, count, order);
  }

  const std::size_t n_sets = result.second.size();
  py::array_t<std::int32_t> simplices({static_cast<py::ssize_t>(n_sets),
                                       static_cast<py::ssize_t>(order + 1)});
  py::array_t<double> radii(static_cast<py::ssize_t>(n_sets));
  std::copy(result.first.begin(), result.first.end(), simplices.mutable_data());
  std::copy(result.second.begin(), result.second.end(), radii.mutable_data());
  return py::make_tuple(simplices, radii);
}

PYBIND11_MODULE(_geometry, m) {
  m.doc() = "Order-k Delaunay simplices of 3D point clouds, computed with Geogram.";
  m.def("order_k_delaunay", &order_k_delaunay, py::arg("points"), py::arg("K"),
        "Return the (K+1)-point sets with a non-empty order-(K+1) Voronoi cell and the squared "
        "radii of their minimum enclosing balls. The points must be in general position, as "
        "produced by hgp_clusterer.hypergraph.normalize.");
}
