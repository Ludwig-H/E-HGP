#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "pipeline/local_credits.hpp"

namespace {

using Exact = boost::multiprecision::cpp_int;

void require(bool value, const char* message) {
  if (!value) {
    throw std::runtime_error(message);
  }
}

void append(mhgp8::RectangleInput& input, unsigned x, unsigned y, unsigned z) {
  require(std::max({x, y, z}) <= 65535, "fixture exceeds u16");
  input.points.push_back({static_cast<std::uint16_t>(x),
                          static_cast<std::uint16_t>(y),
                          static_cast<std::uint16_t>(z)});
}

mhgp8::RectangleInput fixture(const std::string& family, unsigned size) {
  mhgp8::RectangleInput result;
  for (unsigned side = 0; side < 2; ++side) {
    const auto first = result.points.size();
    if (family == "boundary" || family == "reflected_boundary") {
      for (unsigned i = 0; i < size; ++i) {
        append(result, 60000 * side + i, i, i);
      }
      for (unsigned i = 0; i < 8; ++i) {
        append(result, side == 0 ? 3 * size + i : 60000 - 3 * size - i, 0, 0);
      }
    } else if (family == "line") {
      for (unsigned i = 0; i < size; ++i) {
        append(result, 60000 * side + i, 0, 0);
      }
    } else if (family == "grid") {
      unsigned width = 1;
      while (width * width * width < size) {
        ++width;
      }
      for (unsigned i = 0; i < size; ++i) {
        append(result, 60000 * side + i % width,
               (i / width) % width, i / (width * width));
      }
    } else {
      throw std::invalid_argument("unsupported fixture family");
    }
    if (side == 0) {
      result.a = {first, result.points.size()};
    } else {
      result.b = {first, result.points.size()};
    }
  }
  if (family == "reflected_boundary") {
    for (auto& point : result.points) {
      point.x = static_cast<std::uint16_t>(65535 - point.x);
    }
  }
  return result;
}

// Independent arithmetic and corner enumeration: no production predicate or
// bound is called by the oracle, and no production counter records this work.
bool witness(const mhgp8::Point3& a, const mhgp8::Point3& b,
             const mhgp8::Point3& z, unsigned q) {
  std::array<std::int64_t, 3> edge{};
  std::array<std::int64_t, 3> delta{};
  std::int64_t h = 0;
  for (std::size_t i = 0; i < 3; ++i) {
    edge[i] = static_cast<std::int64_t>(b[i]) - a[i];
    delta[i] = static_cast<std::int64_t>(z[i]) - a[i];
    h += delta[i] * (edge[i] - delta[i]);
  }
  if (h <= 0 || q == 2) {
    return h > 0;
  }
  Exact xi = 0;
  for (std::size_t i = 0; i < 3; ++i) {
    const auto j = (i + 1) % 3;
    const auto k = (i + 2) % 3;
    const Exact cross = Exact(edge[j]) * delta[k] - Exact(edge[k]) * delta[j];
    xi += cross * cross;
  }
  return Exact(q == 3 ? 3 : 2) * h * h > xi;
}

std::vector<std::uint8_t> oracle(const mhgp8::RectangleInput& input,
                                mhgp8::Range own, mhgp8::Range opposite,
                                unsigned q, std::uint64_t& checked) {
  std::array<std::uint16_t, 3> low{65535, 65535, 65535};
  std::array<std::uint16_t, 3> high{};
  for (auto id = opposite.first; id < opposite.last; ++id) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      low[axis] = std::min(low[axis], input.points[id][axis]);
      high[axis] = std::max(high[axis], input.points[id][axis]);
    }
  }
  std::vector<std::uint8_t> result(own.size(), 0);
  const unsigned need = 12 - q;
  for (auto a = own.first; a < own.last; ++a) {
    unsigned count = 0;
    for (auto z = own.first; z < own.last; ++z) {
      ++checked;
      bool universal = a != z;
      for (unsigned corner = 0; corner < 8 && universal; ++corner) {
        const mhgp8::Point3 b{(corner & 1U) != 0 ? high[0] : low[0],
                             (corner & 2U) != 0 ? high[1] : low[1],
                             (corner & 4U) != 0 ? high[2] : low[2]};
        universal = witness(input.points[a], b, input.points[z], q);
      }
      count += universal ? 1U : 0U;
    }
    result[a - own.first] = static_cast<std::uint8_t>(std::min(need, count));
  }
  return result;
}

void print_values(std::span<const std::uint8_t> values) {
  std::cout << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      std::cout << ',';
    }
    std::cout << static_cast<unsigned>(values[i]);
  }
  std::cout << ']';
}

void run(const std::string& family, unsigned size, unsigned q) {
  require(size >= 8 && size <= 1024 && q >= 2 && q <= 4, "invalid test domain");
  auto input = fixture(family, size);
  std::uint64_t checked = 0;
  const auto expected_a = oracle(input, input.a, input.b, q, checked);
  const auto expected_b = oracle(input, input.b, input.a, q, checked);
  std::uint64_t expected_candidates = 0;
  for (const auto a : expected_a) {
    for (const auto b : expected_b) {
      expected_candidates += a + b < 12 - q ? 1U : 0U;
    }
  }
  if ((family == "boundary" || family == "reflected_boundary") && q == 4) {
    require(expected_candidates == 36, "boundary residual is not constant");
    for (unsigned i = 0; i < size; ++i) {
      require(expected_a[i] == 8 && expected_b[i] == 8, "stem not saturated");
    }
    for (unsigned i = 0; i < 8; ++i) {
      require(expected_a[size + i] == 7 - i && expected_b[size + i] == 7 - i,
              "tip ranks changed");
    }
  }
  const auto owner = mhgp8::prepare_rectangle(std::move(input), 10, 12);
  for (const auto strategy : {mhgp8::Strategy::DualBlocks, mhgp8::Strategy::Pool,
                              mhgp8::Strategy::Tubes}) {
    const auto plan = mhgp8::make_credit_plan(owner, static_cast<mhgp8::Lane>(q), strategy);
    const auto& work = plan.work();
    const std::string name = strategy == mhgp8::Strategy::DualBlocks ? "dual" :
        strategy == mhgp8::Strategy::Pool ? "pool" : "tubes";
    const auto same = std::equal(plan.a_credits().begin(), plan.a_credits().end(),
                                 expected_a.begin(), expected_a.end()) &&
        std::equal(plan.b_credits().begin(), plan.b_credits().end(),
                   expected_b.begin(), expected_b.end());
    require(strategy != mhgp8::Strategy::DualBlocks || same,
            "DualBlocks differs from exhaustive independent oracle");
    for (std::size_t i = 0; i < expected_a.size(); ++i) {
      require(plan.a_credits()[i] <= expected_a[i], "left minorant exceeds oracle");
    }
    for (std::size_t i = 0; i < expected_b.size(); ++i) {
      require(plan.b_credits()[i] <= expected_b[i], "right minorant exceeds oracle");
    }
    require(plan.candidate_pairs() >= expected_candidates, "missing candidates");
    std::cout << "{\"family\":\"" << family << "\",\"size\":" << size
              << ",\"lane\":" << q << ",\"strategy\":\"" << name
              << "\",\"candidate_pairs\":" << plan.candidate_pairs()
              << ",\"oracle_candidates\":" << expected_candidates
              << ",\"oracle_site_pairs\":" << checked
              << ",\"equal_oracle_credits\":" << (same ? "true" : "false")
              << ",\"tree_nodes\":" << work.tree_nodes
              << ",\"tree_point_visits\":" << work.tree_point_visits
              << ",\"dual_tasks\":" << work.dual_tasks
              << ",\"block_bound_tests\":" << work.predicates.block_bound_tests
              << ",\"negative_probes\":" << work.predicates.negative_probes
              << ",\"leaf_pairs\":" << work.leaf_pairs
              << ",\"universal_queries\":" << work.predicates.universal_queries
              << ",\"point_tests\":" << work.predicates.point_tests
              << ",\"credited_blocks\":" << work.credited_blocks
              << ",\"noncredit_blocks\":" << work.noncredit_blocks
              << ",\"saturated_tasks\":" << work.saturated_tasks
              << ",\"tube_sweep_tests\":" << work.tube_sweep_tests
              << ",\"a_credits\":";
    print_values(plan.a_credits());
    std::cout << ",\"b_credits\":";
    print_values(plan.b_credits());
    std::cout << "}\n";
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    require(argc == 4, "usage: dual_order_probe family size lane");
    run(argv[1], static_cast<unsigned>(std::stoul(argv[2])),
         static_cast<unsigned>(std::stoul(argv[3])));
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "dual order probe: " << error.what() << '\n';
    return 1;
  }
}
