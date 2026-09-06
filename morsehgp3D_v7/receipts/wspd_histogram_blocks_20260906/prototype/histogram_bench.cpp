// Private component measurement, not a FULL/SLO qualification. One fixed input.
// Same surviving front for all arms; literal outputs compared outside timers.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <exception>
#include <string_view>
#include <vector>
#include "src/cloud/families.hpp"
#include "src/core/sha256.hpp"
#include "src/pipeline/generate.hpp"
#include "histogram_blocks.hpp"

#ifdef MHGP7_TESTING
#error Nominal product only
#endif

namespace {
using namespace mhgp7;
namespace hb = histogram_blocks_private;
using Clock = std::chrono::steady_clock;
struct Failure { const char* why; };
void need(bool ok, const char* why) { if (!ok) throw Failure{why}; }
unsigned long long num(u64 value) { return static_cast<unsigned long long>(value); }
double ms(Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
constexpr std::array<u64, 8> upper{1, 2, 4, 8, 16, 32, 64, 8000};
unsigned bucket(u64 n) {
  return static_cast<unsigned>(std::lower_bound(upper.begin(), upper.end(), n) - upper.begin());
}
u64 size_of(const CloudIndex& ix, NodeRef r) {
  const auto range = ix.range_of(r);
  return static_cast<u64>(range.last - range.first + 1);
}
Lane lane_of(unsigned q) { return q == 0 ? Lane::kQ2 : q == 1 ? Lane::kQ3 : Lane::kQ4; }
#define WORK_FIELDS(X) X(logical_pairs) X(node_visits) X(hmax_tests) X(hmax_rejected_nodes) \
  X(hmax_rejected_pairs) X(hmin_tests) X(ximax_tests) X(credited_blocks) X(credited_positions) \
  X(scalar_tests) X(scalar_true) X(diagonal_leaves) X(scalar_factors) X(block_factors) X(singleton_factors)
void add(hb::Work& a, const hb::Work& b) {
#define ADD(name) a.name += b.name;
  WORK_FIELDS(ADD)
#undef ADD
}
void print_work(const hb::Work& work) {
  bool first = true;
  std::printf("{");
#define PRINT(name) std::printf("%s\"" #name "\":%llu", first ? "" : ",", num(work.name)); first = false;
  WORK_FIELDS(PRINT)
#undef PRINT
  std::printf("}");
}
#undef WORK_FIELDS

int measure(i64 separation) {
  constexpr u64 n = 8000;
  const u64 h[3]{10, 9, 8};
  std::printf("{\"event\":\"config\",\"schema\":\"mhgp7-private-histogram-measure-v1\","
              "\"public_status\":\"not_claimed\",\"n\":8000,\"family\":\"uniform\","
              "\"coord\":65536,\"seed\":3,\"s\":%lld,\"smax\":11,\"threads\":1,"
              "\"h\":[10,9,8],\"mask\":7,\"arm_order\":[\"scalar\",\"block0\",\"hybrid8\"],"
              "\"timed_scope\":\"histogram_calls_plus_flatten_and_work_sum\","
              "\"cohort_key\":\"max_factor_size_per_rectangle\"}\n", static_cast<long long>(separation));
  std::fflush(stdout);
  const auto start_index = Clock::now();
  const auto input = make_family_input(CloudFamily::kUniform, static_cast<int>(n), 65536, 3);
  const auto ix = build_cloud_index(input);
  need(ix.valid && !ix.has_duplicate_positions() && ix.upos.size() == n, "input.index");
  const double index_ms = ms(start_index);
  Sha256 input_hash;
  input_hash.tag("mhgp7-private-histogram-input-v1");
  for (const auto& p : input) {
    input_hash.u64le(p.id);
    input_hash.i64le(p.position.x); input_hash.i64le(p.position.y); input_hash.i64le(p.position.z);
  }
  std::vector<MultiAliveRect> alive;
  GenerateStats stats;
  const auto start_front = Clock::now();
  alive_rectangles_fused(ix, separation, h, 7, 1, &alive, &stats);
  const double front_ms = ms(start_front);
  need(stats.cap_refus == kCapRefusNone, "front.refusal");
  Sha256 front_hash;
  front_hash.tag("mhgp7-private-surviving-front-v1");
  front_hash.u64le(alive.size());
  const u64 mass = n * (n - 1) / 2;
  std::array<u64, 3> emitted{}, count{}, pairs{};
  std::array<std::vector<std::size_t>, 8> cohorts;
  std::array<u64, 8> words{};
  std::array<std::array<u64, 8001>, 3> distribution{};
  for (std::size_t i = 0; i < alive.size(); ++i) {
    const auto& r = alive[i];
    front_hash.i64le(r.r.a); front_hash.i64le(r.r.b); front_hash.u64le(r.mask);
    for (unsigned q = 0; q < 3; ++q) front_hash.u64le(r.core[q]);
    const u64 a = size_of(ix, r.r.a), b = size_of(ix, r.r.b);
    const unsigned k = bucket(std::max(a, b));
    need(k < 8 && r.mask != 0 && r.mask <= 7, "front.domain");
    cohorts[k].push_back(i);
    for (unsigned q = 0; q < 3; ++q) if (r.mask & (1u << q)) {
      need(r.core[q] < h[q], "front.live");
      emitted[q] += a * b; ++count[q];
      pairs[q] += a * (a - 1) + b * (b - 1);
      ++distribution[q][a]; ++distribution[q][b];
      words[k] += a + b;
    }
  }
  for (unsigned q = 0; q < 3; ++q) {
    need(emitted[q] == stats.ledger_emitted_mass[q] && count[q] == stats.rect_alive[q], "front.binding");
    need(stats.ledger_emitted_mass[q] + stats.ledger_killed_mass[q] == mass, "front.closure");
  }
  const std::string input_digest = input_hash.hex(), front_digest = front_hash.hex();
  std::printf("{\"event\":\"front\",\"index_ms\":%.6f,\"front_ms\":%.6f,"
              "\"input_digest\":\"%s\",\"front_digest\":\"%s\",\"rectangles\":%llu,"
              "\"visited\":%llu,\"witness_nodes\":%llu,\"corner_evals\":%llu,\"lanes\":[",
              index_ms, front_ms, input_digest.c_str(), front_digest.c_str(), num(alive.size()),
              num(stats.rect_visited_fused), num(stats.wspd_witness_nodes), num(stats.wspd_corner_evals));
  for (unsigned q = 0; q < 3; ++q) {
    std::printf("%s{\"q\":%u,\"rectangles\":%llu,\"emitted_mass\":%llu,\"killed_mass\":%llu,"
                "\"P_factor_logical\":%llu,\"factor_size_counts\":[", q ? "," : "", q + 2,
                num(count[q]), num(emitted[q]), num(static_cast<u64>(stats.ledger_killed_mass[q])), num(pairs[q]));
    bool comma = false;
    for (unsigned size = 1; size <= n; ++size) if (distribution[q][size]) {
      std::printf("%s[%u,%llu]", comma ? "," : "", size, num(distribution[q][size])); comma = true;
    }
    std::printf("]}");
  }
  std::printf("]}\n"); std::fflush(stdout);
  Sha256 reference;
  reference.tag("mhgp7-private-histogram-values-v1");
  reference.u64le(n); reference.u64le(static_cast<u64>(separation));
  std::array<double, 3> totals{};
  std::array<hb::Work, 3> total_work{};
  u64 compared_words = 0, nonempty = 0;
  for (unsigned k = 0; k < 8; ++k) {
    if (cohorts[k].empty()) continue;
    ++nonempty;
    std::vector<u64> expected, observed, ha, hb_values;
    expected.reserve(static_cast<std::size_t>(words[k])); observed.reserve(static_cast<std::size_t>(words[k]));
    // Buffers are pre-reserved equally; repeated assign/flatten is timed.
    ha.reserve(static_cast<std::size_t>(upper[k])); hb_values.reserve(static_cast<std::size_t>(upper[k]));
    reference.u64le(k); reference.u64le(cohorts[k].size()); reference.u64le(words[k]);
    for (unsigned arm = 0; arm < 3; ++arm) {
      auto& flat = arm == 0 ? expected : observed;
      flat.clear();
      hb::Work work;
      const auto start = Clock::now();
      for (std::size_t i : cohorts[k]) {
        const auto& r = alive[i];
        for (unsigned q = 0; q < 3; ++q) if (r.mask & (1u << q)) {
          if (arm == 0) {
            generate_detail::corner_histograms(ix, lane_of(q), r.r, &ha, &hb_values);
            const u64 a = ha.size(), b = hb_values.size();
            work.logical_pairs += a * (a - 1) + b * (b - 1);
            work.scalar_tests += a * (a - 1) + b * (b - 1);
            work.singleton_factors += (a == 1) + (b == 1);
            work.scalar_factors += (a > 1) + (b > 1);
          } else add(work, hb::corner_histograms(ix, lane_of(q), r.r, ha, hb_values, arm == 1 ? 0 : 8));
          flat.insert(flat.end(), ha.begin(), ha.end());
          flat.insert(flat.end(), hb_values.begin(), hb_values.end());
        }
      }
      const double elapsed = ms(start);
      need(flat.size() == words[k], "histogram.word_count");
      if (arm == 0) {
        // True count is inferred from exact outputs, outside the scalar timer.
        for (u64 value : flat) { work.scalar_true += value; reference.u64le(value); }
      } else {
        need(flat == expected, "histogram.literal_difference");
        compared_words += flat.size();
      }
      need(work.logical_pairs == work.scalar_tests + work.credited_positions + work.hmax_rejected_pairs,
           "histogram.work_partition");
      totals[arm] += elapsed; add(total_work[arm], work);
      std::printf("{\"event\":\"cohort\",\"max_factor_size_lower\":%llu,\"max_factor_size_upper\":%llu,"
                  "\"rectangles\":%llu,\"words\":%llu,\"arm\":\"%s\",\"elapsed_ms\":%.6f,\"work\":",
                  num(k == 0 ? 1 : upper[k - 1] + 1), num(upper[k]), num(cohorts[k].size()), num(words[k]),
                  arm == 0 ? "scalar" : arm == 1 ? "block0" : "hybrid8", elapsed);
      print_work(work); std::printf("}\n"); std::fflush(stdout);
    }
  }
  const u64 total_pairs = pairs[0] + pairs[1] + pairs[2];
  need(nonempty > 0 && compared_words > 0, "measure.nonvacuity");
  for (const auto& work : total_work) need(work.logical_pairs == total_pairs, "histogram.total_P_factor");
  std::printf("{\"event\":\"terminal\",\"status\":\"completed\",\"public_status\":\"not_claimed\","
              "\"scope\":\"front_and_histograms_only\",\"all_histograms_literal_equal\":true,"
              "\"compared_words\":%llu,\"reference_digest\":\"%s\","
              "\"P_factor_logical\":%llu,\"histogram_ms\":[%.6f,%.6f,%.6f],\"work\":[",
              num(compared_words), reference.hex().c_str(), num(total_pairs),
              totals[0], totals[1], totals[2]);
  for (unsigned arm = 0; arm < 3; ++arm) { if (arm) std::printf(","); print_work(total_work[arm]); }
  std::printf("]}\n");
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string_view option(argv[1]);
  const i64 s = option == "--s=8" ? 8 : option == "--s=10" ? 10 : option == "--s=12" ? 12 : 0;
  if (s == 0) return 2;
  try { return measure(s); }
  catch (const Failure& error) { std::fprintf(stderr, "histogram measure rejected: %s\n", error.why); }
  catch (const std::exception& error) { std::fprintf(stderr, "histogram measure exception: %s\n", error.what()); }
  return 1;
}
