// q3 payload integration: owned ordinal transport -> canonical catalogue ->
// explicit FULL tower. Local host twin; no GPU timing is claimed here.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../src/chain/tower_chain.hpp"
#include "../src/gen/pipeline/wspd_q34.hpp"
#include "gen/front_fixtures.hpp"

namespace {
using namespace mhgp9;

void need(bool condition, const std::string& cause) {
  if (!condition) throw std::runtime_error(cause);
}

bool same_ball(const tower::BallData& a, const tower::BallData& b) {
  return a.key == b.key && a.level == b.level && a.arity == b.arity && a.n_interior == b.n_interior &&
      a.n_shell == b.n_shell && std::equal(a.interior().begin(), a.interior().end(), b.interior().begin()) &&
      std::equal(a.shell().begin(), a.shell().end(), b.shell().begin());
}

void compare(const ChainResult& a, const ChainResult& b, const std::string& where) {
  need(a.status == ChainStatus::kComplete && b.status == ChainStatus::kComplete,
       "status " + where + " " + a.reason + " " + b.reason);
  need(a.tower_digest == b.tower_digest && a.catalogue_digest == b.catalogue_digest &&
           a.presentation_digest == b.presentation_digest, "digest " + where);
  need(a.catalogue_balls.size() == b.catalogue_balls.size() &&
           std::equal(a.catalogue_balls.begin(), a.catalogue_balls.end(), b.catalogue_balls.begin(), same_ball),
       "BallData " + where);
  const auto& x = a.catalogue;
  const auto& y = b.catalogue;
  need(x.balls == y.balls && x.unique_keys == y.unique_keys && x.extra_shell_balls == y.extra_shell_balls &&
           x.max_shell == y.max_shell && x.max_interior == y.max_interior && x.balls_by_qmin == y.balls_by_qmin &&
           x.balls_by_shell == y.balls_by_shell && x.euler_by_k == y.euler_by_k && x.euler_status == y.euler_status &&
           x.regular_supports_by_arity == y.regular_supports_by_arity &&
           x.early_census_keys == y.early_census_keys && x.early_census_extra_shell_balls == y.early_census_extra_shell_balls,
       "catalogue_stats " + where);
  need(a.q3_emitted == b.q3_emitted && a.q4_emitted == b.q4_emitted &&
           a.q34_batch.lanes_records == b.q34_batch.lanes_records &&
           a.q34_batch.lanes_decided == b.q34_batch.lanes_decided &&
           a.q34_batch.lanes_deferred == b.q34_batch.lanes_deferred, "generation " + where);
  need(y.payload_keys + y.payload_fallback_keys == y.balls_by_qmin[3] &&
           y.payload_keys <= y.regular_supports_by_arity[3] &&
           y.payload_ids <= (b.kmax_effective >= 2 ? b.kmax_effective - 2 : 0) * y.payload_keys,
       "payload_ledger " + where);
}

ChainOptions options(unsigned k, std::size_t workers) {
  ChainOptions o;
  o.kmax = k;
  o.workers = workers;
  o.catalogue_digest = true;
  o.keep_catalogue = true;
  o.q34_batch_filter = o.q34_batch_certificates = o.q34_batch_q3 = true;
  return o;
}

std::vector<gen::Point3> extra_shell() {
  // Four cocircular sites, no antipodal pair: q_min=3, shell=4. The
  // interior site makes an omitted/concatenated interior payload visible.
  return {{135, 100, 100}, {121, 128, 100}, {72, 121, 100}, {79, 72, 100},
          {100, 100, 100}, {170, 161, 121}, {40, 51, 135}};
}

std::uint64_t shape_gates() {
  const std::vector<gen::Point3> points{{100, 100, 100}, {120, 100, 100}, {110, 117, 100}, {110, 105, 100}};
  const auto ix = gen::make_q2_cloud_index(gen::prepare_cloud(points));
  std::array<std::uint32_t, 4> ranks{};
  for (std::size_t r = 0; r < ix->spatial_order().size(); ++r)
    ranks[ix->spatial_order()[r]] = static_cast<std::uint32_t>(r);
  const std::array<gen::Q34SurvivingEdge, 1> edges{{{ranks[0], ranks[1], 2}}};
  const std::array<std::uint8_t, 1> asked{2};
  const auto make = [&] {
    gen::Q34LanesBatch b;
    b.decided = {2}; b.record_begin = {0}; b.record_count = {1};
    gen::Q34LaneRecord r{};
    r.key = gen::ExactBall::make_q3({points[0], points[1], points[2]})->coefficients();
    r.support = {0, 1, 2, UINT32_MAX}; r.edge = 0; r.depth = 1; r.shell = 3; r.arity = 3;
    for (unsigned id = 0; id < 3; ++id) {
      const auto h = gen::q34_shell_hash(id); r.shell_sum += h; r.shell_xor ^= h;
    }
    b.records.push_back(r);
    b.interior_payload = true; b.interior_stride = 3;
    b.interior_ids = std::make_shared<const RawVector<std::uint32_t>>(RawVector<std::uint32_t>{3, UINT32_MAX, UINT32_MAX});
    auto& w = b.work;
    w.edges = w.q3_edges = w.emitted = w.census_seeds = w.seeds = w.acute_sites = 1;
    w.shell_ids = w.census_shell_sites = 3;
    w.seed_tests = w.cover_sites = w.cover.admitted_sites = w.max_cover_sites = w.census_point_tests = 4;
    w.census_inside_sites = 1;
    return b;
  };
  const auto check = [&](const gen::Q34LanesBatch& b) { gen::check_lanes_batch(b, *ix, 5, edges, asked); };
  check(make());
  std::uint64_t refused = 0;
  const auto reject = [&](auto mutate, const std::string& fragment) {
    auto b = make(); mutate(b);
    bool caught = false;
    try { check(b); } catch (const std::logic_error& e) {
      caught = std::string(e.what()).find(fragment) != std::string::npos;
    }
    need(caught, "shape_refusal " + fragment); ++refused;
  };
  reject([](auto& b) { b.interior_stride = 2; }, "payload shape");
  reject([](auto& b) { b.interior_ids.reset(); }, "payload shape");
  reject([](auto& b) { b.interior_payload = false; }, "disabled interior payload");
  reject([](auto& b) { b.interior_ids = std::make_shared<const RawVector<std::uint32_t>>(RawVector<std::uint32_t>{3}); },
         "payload shape");
  reject([](auto& b) { b.interior_ids = std::make_shared<const RawVector<std::uint32_t>>(RawVector<std::uint32_t>{4, UINT32_MAX, UINT32_MAX}); },
         "ID outside cloud");
  reject([](auto& b) { b.interior_ids = std::make_shared<const RawVector<std::uint32_t>>(RawVector<std::uint32_t>{3, 0, UINT32_MAX}); },
         "unused interior slot");
  reject([](auto& b) { b.records[0].depth = 2;
    b.interior_ids = std::make_shared<const RawVector<std::uint32_t>>(RawVector<std::uint32_t>{3, 3, UINT32_MAX}); },
         "duplicate interior ID");
  // A support-site substituted for a listed interior has a valid shape,
  // but must fail the independent complete-ID judge, not merely a count.
  auto wrong = make();
  wrong.interior_ids = std::make_shared<const RawVector<std::uint32_t>>(RawVector<std::uint32_t>{0, UINT32_MAX, UINT32_MAX});
  check(wrong);
  auto judge = gen::judge_lanes_filter([&](const auto&, unsigned, auto, auto) { return wrong; }, {}, 1, nullptr);
  bool caught = false;
  try { (void)judge(ix, 5, edges, asked); } catch (const std::logic_error& e) {
    caught = std::string(e.what()).find("complete interior IDs differ") != std::string::npos;
  }
  need(caught, "global_ID_judge");
  return refused + 1;
}

void priority_gate(const std::vector<gen::Point3>& points) {
#if defined(MHGP9_TESTING)
  auto o = options(5, 1);
  o.q3_interior_payload = true;
  auto reference = run_tower_chain(points, o);
  need(reference.status == ChainStatus::kComplete && reference.catalogue_balls.size() > 2, "priority_reference");
  const auto key = [](const tower::BallKey& k) { return std::array<gen::i128, 5>{k.a, k.b[0], k.b[1], k.b[2], k.c}; };
  const auto& balls = reference.catalogue_balls;
  std::vector<std::array<gen::i128, 5>> keys{key(balls.back().key), key(balls.front().key)};
  chain_testing::set_census_failpoints(keys);
  for (const std::size_t w : {std::size_t{1}, std::size_t{4}})
    for (const bool early : {false, true}) {
      o.workers = w; o.q2_during_device = o.q2_early_census = early;
      const auto r = run_tower_chain(points, o);
      need(r.status == ChainStatus::kInvariantViolated && r.reason == "failpoint_census_1" && r.tower_digest == 0,
           "priority_smallest_key " + r.reason);
    }
  chain_testing::set_census_failpoints({});
  o.workers = 4; o.q2_during_device = o.q2_early_census = true;
  chain_testing::set_early_failpoints(chain_testing::kEarlyAfterIndex, false);
  const auto fallback = run_tower_chain(points, o);
  chain_testing::set_early_failpoints(chain_testing::kEarlyNoThrow, false);
  need(fallback.status == ChainStatus::kComplete && fallback.catalogue_digest == reference.catalogue_digest &&
           fallback.tower_digest == reference.tower_digest && fallback.catalogue.early_census_keys == 0,
       "early_index_fallback " + fallback.reason);
#else
  (void)points;
#endif
}
}  // namespace

int main(int argc, char** argv) {
  std::size_t n = 64;
  for (int a = 1; a < argc; ++a) {
    const std::string_view s(argv[a]);
    if (!s.starts_with("--n=")) return 2;
    const auto d = s.substr(4);
    const auto [end, ec] = std::from_chars(d.data(), d.data() + d.size(), n);
    if (d.empty() || ec != std::errc{} || end != d.data() + d.size() || n < 32 || n > 1024) return 2;
  }
  try {
    std::uint64_t cases = 0, imported = 0, ids = 0, fallbacks = 0, tails = 0, extra = 0, judged = 0;
    const auto shape_refusals = shape_gates();
    for (const std::string_view family : {"uniform", "terrain", "clusters", "extra"}) {
      auto points = family == "extra" ? extra_shell() : gen::bench::make_front_fixture(n, family, 19).points;
      std::reverse(points.begin(), points.end());  // input IDs deliberately not spatial ranks
      for (const unsigned k : {2U, 3U, 5U, 10U})
        for (const std::size_t workers : {std::size_t{1}, std::size_t{4}})
          for (unsigned mode = 0; mode < 6; ++mode) {
            auto off = options(k, workers);
            off.q34_batch_q4 = mode != 0;
            off.q34_lanes_fused = mode >= 2;
            off.q34_lanes_pinned = mode == 3;
            off.tower_sealed_catalogue = mode == 3;
            off.q2_during_device = off.q2_early_census = mode == 3;
            if (mode == 4) { off.q34_lanes_capacity = 12; off.q34_lanes_events = 2; }
            if (mode == 5) { off.q34_lanes_capacity = 2; off.q34_lanes_events = 2; }
            auto on = off; on.q3_interior_payload = true;
            const std::string where = std::string(family) + "/K" + std::to_string(k) + "/W" +
                std::to_string(workers) + "/mode" + std::to_string(mode);
            const auto a = run_tower_chain(points, off);
            const auto b = run_tower_chain(points, on);
            compare(a, b, where);
            need(a.catalogue.payload_keys == 0 && a.catalogue.payload_ids == 0 && a.catalogue.payload_fallback_keys == 0,
                 "disabled_counts " + where);
            need(b.catalogue.census_nodes <= a.catalogue.census_nodes &&
                     b.catalogue.census_leaf_tests <= a.catalogue.census_leaf_tests, "census_work " + where);
            need(b.reason == (mode == 3 ? "complete_relative_to_cross_checked_catalogue_sealed_in_process_payload"
                                       : "complete_relative_to_cross_checked_catalogue_payload"), "provenance " + where);
            if (mode == 0 && workers == 1) {
              on.q34_lanes_judge = true;
              const auto j = run_tower_chain(points, on);
              compare(a, j, where + "/judge");
              need(j.catalogue.census_nodes == a.catalogue.census_nodes &&
                       j.catalogue.census_leaf_tests == a.catalogue.census_leaf_tests, "judge_paid_census " + where);
              ++judged;
            }
            imported += b.catalogue.payload_keys; ids += b.catalogue.payload_ids;
            fallbacks += b.catalogue.payload_fallback_keys; tails += b.q34_batch.lanes_deferred;
            extra += b.catalogue.extra_shell_balls;
            ++cases;
          }
    }
    // One larger catalogue-only arm, not 192 enlarged configurations. More
    // than 4096 imported distinct q3 keys forces handles beyond the first
    // sink chunk; the parallel merge must also copy/sort several ranges.
    const auto many_points = gen::bench::make_front_fixture(384, "uniform", 23).points;
    auto many_options = options(10, 4);
    many_options.run_tower = false;
    many_options.q34_batch_q4 = many_options.q34_lanes_fused = many_options.q34_lanes_pinned = true;
    const auto many_off = run_tower_chain(many_points, many_options);
    many_options.q3_interior_payload = true;
    const auto many_on = run_tower_chain(many_points, many_options);
    compare(many_off, many_on, "multi_chunk");
    need(many_on.catalogue.payload_keys > 4096 && many_on.q34_batch.lanes_records > 4096 &&
             many_on.presentation_ranges > 1, "multi_chunk_floor");
    const auto points = gen::bench::make_front_fixture(n, "uniform", 19).points;
    priority_gate(points);
    for (const bool run : {false, true}) {
      auto o = options(1, 1); o.q3_interior_payload = true; o.run_tower = run; o.tower_sealed_catalogue = true;
      const auto r = run_tower_chain(std::span<const gen::Point3>(points).first(2), o);
      need(r.status == ChainStatus::kComplete && r.catalogue.payload_keys == 0 && r.catalogue.payload_ids == 0,
           "K1_empty_payload " + r.reason);
      need(r.reason == (run ? "complete_relative_to_cross_checked_catalogue_sealed_in_process_payload"
                           : "complete_relative_to_cross_checked_catalogue_payload"), "K1_provenance");
      const auto single = run_tower_chain(std::span<const gen::Point3>(points).first(1), o);
      need(single.status == ChainStatus::kInvalidInput && single.reason == "chain_requires_two_sites",
           "single_site_domain " + single.reason);
    }
    ChainOptions invalid; invalid.q3_interior_payload = true;
    const auto refused = run_tower_chain(points, invalid);
    need(refused.status == ChainStatus::kInvalidInput &&
             refused.reason == "chain_q3_interior_payload_requires_batch_q3", "option_dependency");
    need(imported != 0 && ids != 0 && fallbacks != 0 && tails != 0 && extra != 0 && judged == 16 && cases == 192,
         "coverage_floor");
    std::printf("chain_q3_payload_gate n=%zu cases=%llu imports=%llu ids=%llu fallbacks=%llu tails=%llu extra=%llu "
                "judged=%llu shape_refusals=%llu multi_chunk_records=%llu multi_chunk_imports=%llu "
                "merge_ranges=%zu presentation_bytes=112\n", n,
                static_cast<unsigned long long>(cases), static_cast<unsigned long long>(imported),
                static_cast<unsigned long long>(ids), static_cast<unsigned long long>(fallbacks),
                static_cast<unsigned long long>(tails), static_cast<unsigned long long>(extra),
                static_cast<unsigned long long>(judged), static_cast<unsigned long long>(shape_refusals),
                static_cast<unsigned long long>(many_on.q34_batch.lanes_records),
                static_cast<unsigned long long>(many_on.catalogue.payload_keys), many_on.presentation_ranges);
  } catch (const std::exception& e) {
    std::printf("cause=%s\n", e.what());
    return 1;
  }
  return 0;
}
