#include "pipeline/axis_q2.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using mhgp8::Point3;
using i64 = std::int64_t;
using u64 = std::uint64_t;
using Vector = std::array<i64, 3>;
constexpr std::array<Vector, 3> matrix{{{1, 2, 2}, {2, 1, -2}, {-2, 2, -1}}};
constexpr Vector offset{5000, 25000, 30000};

void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

Point3 checked_point(Vector coordinates) {
  for (auto value : coordinates) require(value >= 0 && value <= 65535, "u16 fixture");
  return {static_cast<std::uint16_t>(coordinates[0]),
          static_cast<std::uint16_t>(coordinates[1]),
          static_cast<std::uint16_t>(coordinates[2])};
}

i64 distance2(Point3 a, Point3 b) {
  i64 result = 0;
  for (unsigned axis = 0; axis < 3; ++axis) {
    const auto delta = i64(a[axis]) - b[axis];
    result += delta * delta;
  }
  return result;
}

// Independent strict diametral-ball membership by midpoint and radius.
bool interior(Point3 a, Point3 b, Point3 z) {
  i64 gap = distance2(a, b);
  for (unsigned axis = 0; axis < 3; ++axis) {
    const auto delta = 2 * i64(z[axis]) - a[axis] - b[axis];
    gap -= delta * delta;
  }
  return gap > 0;
}

mhgp8::RectangleInput fixture(unsigned ny, unsigned nz, bool rotated) {
  mhgp8::RectangleInput result;
  const unsigned m = ny * nz;
  for (unsigned side = 0; side < 2; ++side)
    for (unsigned y = 0; y < ny; ++y)
      for (unsigned z = 0; z < nz; ++z) {
        Vector v{8000 * side, y, z}, p = offset;
        for (unsigned axis = 0; axis < 3; ++axis)
          if (rotated) {
            for (unsigned j = 0; j < 3; ++j) p[axis] += matrix[axis][j] * v[j];
          } else p[axis] += 3 * v[axis];
        result.points.push_back(checked_point(p));
      }
  result.a = {0, m};
  result.b = {m, 2 * m};
  return result;
}

mhgp8::RectangleInput recover_frame(mhgp8::RectangleInput result) {
  for (auto& point : result.points) {
    Vector transformed{};
    for (unsigned axis = 0; axis < 3; ++axis) {
      i64 numerator = 0;
      for (unsigned k = 0; k < 3; ++k)
        numerator += matrix[k][axis] * (i64(point[k]) - offset[k]);
      // Exact division is a gate, never a rounding rule for arbitrary input.
      require(numerator % 3 == 0, "nonintegral recovery refused");
      transformed[axis] = offset[axis] + numerator / 3;
    }
    point = checked_point(transformed);
  }
  return result;
}

u64 band_count(unsigned n, unsigned h) {
  u64 count = 0;
  for (unsigned i = 0; i < n; ++i) {
    const unsigned first = i > h ? i - h : 0;
    const unsigned last = std::min(n, i + h + 1);
    count += last - first;
  }
  return count;
}

void emit(const char* orientation, const mhgp8::AxisQ2Plan& plan) {
  const auto& work = plan.work();
  std::cout << "\"" << orientation << "\":{\"candidates\":" << plan.candidate_pairs()
            << ",\"descriptors\":" << plan.blocks().size()
            << ",\"columns\":" << work.columns
            << ",\"sorted_sites\":" << work.sorted_sites
            << ",\"sort_comparisons\":" << work.sort_comparisons
            << ",\"constrained_anchors\":" << work.constrained_anchors
            << ",\"tree_point_visits\":" << work.tree_point_visits
            << ",\"query_nodes\":" << work.query_nodes
            << ",\"whole_factor_accepts\":" << work.whole_factor_accepts << '}';
}

void run(unsigned ny, unsigned nz, unsigned h, unsigned separation) {
  auto input = fixture(ny, nz, false);
  const auto rotated_input = fixture(ny, nz, true);
  const auto recovered_input = recover_frame(rotated_input);
  require(input.points == recovered_input.points, "frame recovery changed sites or IDs");
  const auto owner = mhgp8::prepare_rectangle(input, h, separation);
  const auto rotated_owner = mhgp8::prepare_rectangle(rotated_input, h, separation);
  const auto recovered_owner = mhgp8::prepare_rectangle(recovered_input, h, separation);
  const auto plan = mhgp8::make_axis_q2_plan(owner);
  const auto rotated = mhgp8::make_axis_q2_plan(rotated_owner);
  const auto recovered = mhgp8::make_axis_q2_plan(recovered_owner);
  const u64 m = ny * nz;
  const auto expected = band_count(ny, h) * band_count(nz, h);
  require(plan.candidate_pairs() == expected && recovered.candidate_pairs() == expected,
          "aligned band cardinal");
  require(rotated.candidate_pairs() == m * m && rotated.work().columns == 3 * m &&
          rotated.work().constrained_anchors == 0 && rotated.work().query_nodes == 0 &&
          rotated.work().tree_nodes == 0 && rotated.work().whole_factor_accepts == m,
          "rotation singleton columns");
  require(rotated.blocks().size() == m, "whole factor descriptor cost");
  u64 oracle_tests = 0, survivors = 0;
  const bool exhaustive = m <= 64;
  if (exhaustive) {
    std::vector<bool> coverage(m * m, false);
    plan.for_each_candidate([&](std::size_t a, std::size_t b) {
      require(a < m && b >= m && b < 2 * m, "foreign original ID");
      const auto key = a * m + b - m;
      require(!coverage[key], "duplicate pair expansion");
      coverage[key] = true;
    });
    for (unsigned i = 0; i < 2 * m; ++i)
      for (unsigned j = 0; j < 2 * m; ++j)
        require(distance2(input.points[i], input.points[j]) ==
                distance2(rotated_input.points[i], rotated_input.points[j]),
                "isometry changed squared distance");
    for (unsigned a = 0; a < m; ++a)
      for (unsigned b = static_cast<unsigned>(m); b < 2 * m; ++b) {
        require(plan.keeps(a, b) == coverage[a * m + b - m] &&
                recovered.keeps(a, b) == plan.keeps(a, b) && rotated.keeps(a, b),
                "candidate membership mismatch");
        unsigned depth = 0;
        for (unsigned z = 0; z < 2 * m; ++z) {
          const bool inside = interior(input.points[a], input.points[b], input.points[z]);
          const bool rotated_inside = interior(rotated_input.points[a], rotated_input.points[b],
                                               rotated_input.points[z]);
          oracle_tests += 2;
          require(inside == rotated_inside, "rotation changed q2 depth");
          depth += inside;
        }
        if (depth < h) {
          ++survivors;
          require(plan.keeps(a, b), "lost exact q2 survivor");
        }
      }
  }
  std::cout << "{\"n\":" << 2 * m << ",\"ny\":" << ny << ",\"nz\":" << nz
            << ",\"h\":" << h << ",\"s\":" << separation
            << ",\"exhaustive\":" << (exhaustive ? "true" : "false")
            << ",\"total_pairs\":" << m * m << ",\"oracle_point_tests\":" << oracle_tests
            << ",\"q2_survivors\":";
  if (exhaustive) std::cout << survivors;
  else std::cout << "null";
  std::cout << ',';
  emit("aligned", plan);
  std::cout << ',';
  emit("rotated", rotated);
  std::cout << ',';
  emit("exact_frame_recovered", recovered);
  std::cout << "}\n";
}
}  // namespace

int main() {
  try {
    for (unsigned i = 0; i < 3; ++i)
      for (unsigned j = 0; j < 3; ++j) {
        i64 value = 0;
        for (unsigned k = 0; k < 3; ++k) value += matrix[k][i] * matrix[k][j];
        require(value == (i == j ? 9 : 0), "matrix is not scaled orthogonal");
      }
    auto invalid = fixture(4, 4, true);
    ++invalid.points.front().x;
    bool rejected = false;
    try { static_cast<void>(recover_frame(invalid)); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected, "nonintegral transformed fixture accepted");
    for (unsigned h : {1U, 5U, 10U}) {
      run(4, 4, h, 12);
      run(5, 7, h, 12);
      run(8, 8, h, 12);
    }
    for (const auto shape : {std::array{50U, 80U}, std::array{100U, 80U},
                             std::array{125U, 128U}})
      for (unsigned h : {5U, 10U})
        for (unsigned s : {8U, 10U, 12U}) run(shape[0], shape[1], h, s);
  } catch (const std::exception& error) {
    std::cerr << "axis rotation probe failed: " << error.what() << '\n';
    return 1;
  }
}
