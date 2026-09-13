#include "pipeline/local_credits.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const char* cause) {
  if (!condition) throw std::runtime_error(cause);
}

// Independent diametral-ball membership, with exact midpoint coordinates.
bool interior(mhgp8::Point3 a, mhgp8::Point3 b, mhgp8::Point3 z) {
  std::int64_t four_h = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const std::int64_t diameter = std::int64_t(b[axis]) - a[axis];
    const std::int64_t offset = 2 * std::int64_t(z[axis]) - a[axis] - b[axis];
    four_h += diameter * diameter - offset * offset;
  }
  return four_h > 0;
}

void run(unsigned m, unsigned h, unsigned separation) {
  mhgp8::RectangleInput input;
  for (unsigned side = 0; side < 2; ++side)
    for (unsigned y = 0; y < m; ++y)
      input.points.push_back({static_cast<std::uint16_t>(1000 + 59000 * side),
                              static_cast<std::uint16_t>(y), 0});
  input.a = {0, m};
  input.b = {m, 2 * m};
  const auto rectangle = mhgp8::prepare_rectangle(std::move(input), h, separation);
  const auto points = rectangle->points();
  const unsigned width = (h + 1) / 2;
  const std::uint64_t expected_count = (2 * width + 1) * m - width * (width + 1);
  require(m > width, "formula domain");
  std::vector<std::pair<std::size_t, std::size_t>> reference;
  for (unsigned i = 0; i < m; ++i)
    for (unsigned j = 0; j < m; ++j)
      if (i > j ? i - j <= width : j - i <= width)
        reference.emplace_back(i, m + j);
  require(reference.size() == expected_count, "band formula");
  // An intentionally too narrow band loses real survivors at its two borders.
  const auto narrow = (2 * width - 1) * m - (width - 1) * width;
  require(narrow < expected_count, "band mutation nonvacuity");

  for (const auto strategy : {mhgp8::Strategy::Pool, mhgp8::Strategy::DualBlocks,
                              mhgp8::Strategy::Tubes}) {
    const auto plan = mhgp8::make_credit_plan(rectangle, mhgp8::Lane::Q2, strategy);
    require(plan.core_credit() == 0 && plan.threshold() == h, "core/threshold");
    require(std::all_of(plan.a_credits().begin(), plan.a_credits().end(),
                        [](auto credit) { return credit == 0; }), "left zero credit");
    require(std::all_of(plan.b_credits().begin(), plan.b_credits().end(),
                        [](auto credit) { return credit == 0; }), "right zero credit");
    require(plan.candidate_pairs() == std::uint64_t(m) * m, "local residual");
    std::uint64_t enumerated = 0, census_tests = 0, rejected = 0;
    std::vector<std::pair<std::size_t, std::size_t>> survived;
    plan.for_each_candidate([&](std::size_t a, std::size_t b) {
      ++enumerated;
      unsigned depth = 0;
      for (std::size_t z = 0; z < points.size(); ++z) {
        if (z == a || z == b) continue;
        ++census_tests;
        if (interior(points[a], points[b], points[z]) && ++depth == h) break;
      }
      const auto i = a, j = b - m;
      const auto gap = i > j ? i - j : j - i;
      const auto formula_depth = gap == 0 ? 0 : 2 * (gap - 1);
      require(depth == std::min<std::size_t>(h, formula_depth), "independent depth");
      if (depth < h) survived.emplace_back(a, b);
      else ++rejected;
    });
    require(enumerated == plan.candidate_pairs(), "physical expansion");
    std::sort(survived.begin(), survived.end());
    require(survived == reference, "consumer/band mismatch");
    require(rejected > 0 && !survived.empty(), "consumer nonvacuity");
    const char* name = strategy == mhgp8::Strategy::Pool ? "pool" :
        strategy == mhgp8::Strategy::DualBlocks ? "dual" : "tubes";
    std::cout << "{\"status\":\"passed\",\"m\":" << m << ",\"n\":" << 2 * m
              << ",\"h\":" << h << ",\"s\":" << separation
              << ",\"strategy\":\"" << name << "\",\"candidates\":" << enumerated
              << ",\"q2_survivors\":" << survived.size()
              << ",\"q2_rejected\":" << rejected << ",\"census_point_tests\":"
              << census_tests << ",\"dual_tasks\":" << plan.work().dual_tasks
              << ",\"tube_sweep_tests\":" << plan.work().tube_sweep_tests << "}\n";
  }
}

}  // namespace

int main() {
  try {
    for (unsigned h : {1U, 5U, 10U}) run(16, h, 12);
    for (unsigned m : {64U, 128U, 256U}) run(m, 10, 8);
    run(64, 10, 10);
    run(64, 10, 12);
  } catch (const std::exception& error) {
    std::cerr << "transverse residual probe failed: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
