// Pure arithmetic gate for probe admission; no cloud, allocation or FULL run.
#include <array>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

#include "../bench/full_gabriel_probe_limits.hpp"
#include "../src/pipeline/expand.hpp"

#ifdef MHGP7_TESTING
#error "census payload gate uses nominal product headers"
#endif

namespace {
using namespace mhgp7;
namespace policy = mhgp7::full_probe_limits;
constexpr u64 c = sizeof(BallCandidate), s = sizeof(Survivor), b = sizeof(BallData);
constexpr u64 max = std::numeric_limits<u64>::max();
u64 checks = 0;

void need(bool ok, const char* reason) {
  ++checks;
  if (!ok) throw std::runtime_error(reason);
}

bool pre(u64 n, u64 budget) {
  return policy::prefilter_payload_fits<BallCandidate, Survivor>(n, budget);
}

bool census(u64 n, u64 survivors, u64 budget) {
  return policy::census_payload_fits<BallCandidate, Survivor, BallData>(n, survivors, budget);
}

void boundaries() {
  need(std::strcmp(policy::kCensusPayloadAccounting,
       "preflight_survivor_then_direct_census_v2") == 0, "accounting.version");
  need(pre(0, 0) && census(0, 0, 0), "empty.zero_budget");
  need(!pre(1, 0) && !census(1, 0, 0), "nonempty.zero_budget_not_unlimited");
  const std::array<std::pair<u64, u64>, 5> cases{{{1, 0}, {1, 1}, {17, 0}, {17, 3}, {17, 17}}};
  for (const auto& [n, survivors] : cases) {
    const u64 first = n * (c + 2 * s), second = n * c + survivors * (s + b);
    need(pre(n, first), "prefilter.exact_budget");
    need(!pre(n, first - 1), "prefilter.budget_minus_one");
    need(census(n, survivors, second), "census.exact_budget");
    need(!census(n, survivors, second - 1), "census.budget_minus_one");
  }
  need(!census(0, 1, max) && !census(17, 18, max), "survivors.cannot_exceed_candidates");
  need(c > 1 && s > 0 && b > s, "sizeof.nonvacuum_cost_domain");
  constexpr u64 n = 17;
  const u64 single = n * (c + s + b), old = n * (c + s + 2 * b);
  need(pre(n, single) && census(n, n, single), "one_BallData.admitted");
  need(old > single && !fits_budget(n, c + s + 2 * b, 1, single), "old_two_BallData.false_refusal");
  const u64 sparse = n * (c + 2 * s);
  need(pre(n, sparse) && census(n, 0, sparse) && !fits_budget(n, c + s + b, 1, sparse),
       "two_phase.less_conservative_than_single_384");
  const u64 u32max = std::numeric_limits<u32>::max();
  need(kMaxRawCandidates == u32max && candidates_capacity_ok(static_cast<size_t>(u32max)),
       "indices.u32_exact_unchanged");
  static_assert(std::numeric_limits<size_t>::max() > std::numeric_limits<u32>::max());
  need(!candidates_capacity_ok(static_cast<size_t>(u32max + 1)), "indices.u32_plus_one_refused");
}

void overflows() {
  need(!pre(max / c + 1, max), "prefilter.candidate_product_overflow");
  need(!pre(max / (2 * s) + 1, max), "prefilter.survivor_product_overflow");
  const u64 pre_sum = max / (c + 2 * s) + 1;
  need(pre_sum <= max / c && pre_sum <= max / (2 * s) && !pre(pre_sum, max),
       "prefilter.sum_overflow_without_product_overflow");
  need(!census(max / c + 1, 0, max), "census.candidate_product_overflow");
  need(!census(max / (s + b) + 1, max / (s + b) + 1, max), "census.survivor_product_overflow");
  const u64 census_sum = max / (c + s + b) + 1;
  need(census_sum <= max / c && census_sum <= max / (s + b) && !census(census_sum, census_sum, max),
       "census.sum_overflow_without_product_overflow");
  u64 result = 19;
  need(!policy::product_sum(max, 2, 0, 0, result) && result == 19, "product.first_overflow_preserves_output");
  need(!policy::product_sum(0, 0, max, 2, result) && result == 19, "product.second_overflow_preserves_output");
  need(!policy::product_sum(max, 1, 1, 1, result) && result == 19, "product.sum_overflow_preserves_output");
  need(policy::product_sum(max - 1, 1, 1, 1, result) && result == max, "product.exact_MAX");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::strcmp(argv[1], "--selftest") != 0 && std::strcmp(argv[1], "--rejects") != 0)) return 2;
  try {
    boundaries(); overflows();
    if (checks != 40) throw std::runtime_error("nonvacuum.expected_40");
    std::printf("{\"schema\":\"mhgp7-full-census-payload-gate-v1\",\"status\":\"passed\","
                "\"public_status\":\"not_claimed\",\"checks\":%llu,\"sizeof_candidate\":%llu,"
                "\"sizeof_survivor\":%llu,\"sizeof_ball\":%llu,\"full_hierarchy_built\":false}\n",
                static_cast<unsigned long long>(checks), static_cast<unsigned long long>(c),
                static_cast<unsigned long long>(s), static_cast<unsigned long long>(b));
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "census payload rejected: %s\n", error.what());
    return 1;
  }
}
