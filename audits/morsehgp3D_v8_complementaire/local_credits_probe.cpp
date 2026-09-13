#include "pipeline/local_credits.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void require(bool value, const char* cause) {
  if (!value) throw std::runtime_error(cause);
}

void run(unsigned h, unsigned length, unsigned separation, mhgp8::Lane lane) {
  const unsigned rails = h + 1;
  const unsigned shift = 48 * rails * length;
  require(shift + length <= 65535, "u16 domain");
  mhgp8::RectangleInput input;
  for (unsigned side = 0; side < 2; ++side)
    for (unsigned rail = 0; rail < rails; ++rail)
      for (unsigned x = 0; x <= length; ++x)
        input.points.push_back({static_cast<std::uint16_t>(side * shift + x),
                                static_cast<std::uint16_t>(4 * length * rail), 0});
  const std::size_t m = input.points.size() / 2;
  input.a = {0, m};
  input.b = {m, 2 * m};
  auto rectangle = mhgp8::prepare_rectangle(std::move(input), 10, separation);
  auto pool = mhgp8::make_credit_plan(rectangle, lane, mhgp8::Strategy::Pool);
  auto dual = mhgp8::make_credit_plan(rectangle, lane, mhgp8::Strategy::DualBlocks);
  require(dual.threshold() == h && dual.core_credit() == 0, "threshold/core");
  require(pool.work().pool_selected == 2 * (h + 1), "actual pool budget");
  const std::uint64_t expected = rails * rails * h * (h + 1) / 2;
  require(dual.candidate_pairs() == expected, "dual residual");
  require(pool.candidate_pairs() == m * m, "pool residual changed; re-audit finding");
  for (std::size_t i = 0; i < m; ++i) {
    const auto x = static_cast<unsigned>(i % (length + 1));
    require(dual.a_credits()[i] == std::min(h, length - x), "dual left credit");
    require(dual.b_credits()[i] == std::min(h, x), "dual right credit");
    require(pool.a_credits()[i] == (x < length ? 1 : 0), "pool left credit");
    require(pool.b_credits()[i] == (x > 0 ? 1 : 0), "pool right credit");
  }
  std::vector<std::pair<std::size_t, std::size_t>> pairs;
  dual.for_each_candidate([&pairs](std::size_t a, std::size_t b) {
    pairs.emplace_back(a, b);
  });
  require(pairs.size() == expected, "physical expansion");
  std::sort(pairs.begin(), pairs.end());
  require(std::adjacent_find(pairs.begin(), pairs.end()) == pairs.end(), "duplicates");
  for (const auto& [a, b] : pairs) {
    const auto ax = static_cast<unsigned>(a % (length + 1));
    const auto bx = static_cast<unsigned>((b - m) % (length + 1));
    require(length - ax + bx < h, "false physical candidate");
    require(dual.keeps(a, b), "keeps/expansion mismatch");
  }
  require(dual.work().credited_blocks > 0 && dual.work().noncredit_blocks > 0,
          "dual branch nonvacuity");
  std::cout << "{\"status\":\"passed\",\"h\":" << h
            << ",\"s\":" << separation << ",\"length\":" << length
            << ",\"n\":" << 2 * m << ",\"pool_selected\":"
            << pool.work().pool_selected << ",\"pool_candidates\":"
            << pool.candidate_pairs() << ",\"dual_candidates\":"
            << dual.candidate_pairs() << ",\"dual_tasks\":"
            << dual.work().dual_tasks << ",\"dual_leaf_pairs\":"
            << dual.work().leaf_pairs << ",\"credited_blocks\":"
            << dual.work().credited_blocks << ",\"noncredit_blocks\":"
            << dual.work().noncredit_blocks << "}\n";
}

}  // namespace

int main() {
  try {
    for (unsigned s : {8U, 10U, 12U}) {
      run(8, 8, s, mhgp8::Lane::Q4);
      run(8, 150, s, mhgp8::Lane::Q4);
      run(9, 9, s, mhgp8::Lane::Q3);
    }
  } catch (const std::exception& error) {
    std::cerr << "local credit probe failed: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
