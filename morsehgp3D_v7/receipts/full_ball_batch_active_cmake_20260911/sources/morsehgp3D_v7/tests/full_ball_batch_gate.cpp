// Permanent bounded callback regression, without rewriting product headers.
// The scalar callback/reference lives in tests/; Gram/Gamma judges the forest.
#include "../src/forest/full_ball_tower.hpp"
#include "full_ball_batch_owner.hpp"

mhgp7::FullBallTowerResult observed_batch_tower(const mhgp7::CloudIndex& ix,
    std::span<const mhgp7::BallData> balls, unsigned kmax, int workers = 0);

// Interpose calls from the test only. The real product header was included
// above, before these macros, and is never generated or modified on disk.
#define build_full_ball_tower observed_batch_tower
#define main mhgp7_embedded_batch_full_gate_main
#include "full_ball_tower_gate.cpp"
#undef main
#undef build_full_ball_tower

namespace {
u64 callback_batches = 0, callback_requests = 0, direct_terminals = 0;
u64 reference_meb_calls = 0, callback_payload_checks = 0, callback_work_checks = 0;

void same_geometry_work(const FullBallStats& a, const FullBallStats& b) {
  for (const auto field : {&FullBallStats::anchor_hits, &FullBallStats::key_lookups,
      &FullBallStats::intruder_queries, &FullBallStats::intruder_nodes,
      &FullBallStats::intruder_power_tests, &FullBallStats::interior_ranges,
      &FullBallStats::same_radius_steps, &FullBallStats::descending_steps,
      &FullBallStats::max_chain_steps})
    need(a.*field == b.*field, "batch.nominal.geometry_counter");
  need(a.resolve_work.calls == b.resolve_work.calls &&
      a.resolve_work.supports_by_size == b.resolve_work.supports_by_size &&
      a.resolve_work.power_tests == b.resolve_work.power_tests &&
      a.resolve_work.materializations == b.resolve_work.materializations,
      "batch.nominal.exact_MEB_work");
  need(a.static_requests == b.static_requests && a.static_unique == b.static_unique &&
      a.static_seeded == b.static_seeded, "batch.nominal.request_partition");
  need(a.static_post_seed_queries == b.static_post_seed_queries &&
      a.static_post_seed_hits == b.static_post_seed_hits &&
      a.static_post_seed_terminals == b.static_post_seed_terminals,
      "batch.nominal.exact_post_exchange_QHT");
  need(b.static_batch_work_known, "batch.nominal.known_work");
}

int callback_rejects() {
  try {
    context = "permanent_batch_rejects";
    const Fixture fixture{"batch_rejections", {{0,3,0},{8,3,0},{4,2,0},{6,0,0},{4,23,0}}, 5};
    const auto in = input(fixture, 0);
    const auto ix = build_cloud_index(in);
    const oracle::Model model(fixture.points);
    const auto balls = catalogue(fixture.points, ix, model, fixture.kmax);
    const auto baseline = mhgp7::build_full_ball_tower(ix, balls, fixture.kmax, 1);
    need(baseline.status == FullBallStatus::kCompleteRelative, "batch_reject_reference");
    struct Fault { const char* name; const char* reason; FullBallStatus status; bool known; };
    const std::vector<Fault> faults{
      {"short_targets", "full_ball_batch_target_count", FullBallStatus::kInvariantViolated, true},
      {"ordinal", "full_ball_batch_target_ordinal", FullBallStatus::kInvariantViolated, true},
      {"out_of_domain", "full_ball_batch_target_domain", FullBallStatus::kInvariantViolated, true},
      {"not_strict", "full_ball_batch_target_not_strict", FullBallStatus::kInvariantViolated, true},
      {"omit_q", "full_ball_batch_work_identity", FullBallStatus::kInvariantViolated, true},
      {"other_k", "full_ball_batch_work_order", FullBallStatus::kInvariantViolated, true},
      {"partial_failure", "full_ball_batch_partial_publication", FullBallStatus::kInvariantViolated, true},
      {"failure_unknown", "batch_cpu_unknown_device_work", FullBallStatus::kResourceExhausted, false},
      {"throw_unknown", "full_ball_batch_exception", FullBallStatus::kInvariantViolated, false},
      {"success_unknown", "full_ball_batch_success_work_unknown", FullBallStatus::kInvariantViolated, false},
      {"worker_after_paid", "batch_cpu_worker_allocation", FullBallStatus::kResourceExhausted, true},
      {"work_merge_overflow", "full_ball_counter_overflow", FullBallStatus::kResourceExhausted, false},
      {"capacity_overflow", "full_ball_counter_overflow", FullBallStatus::kResourceExhausted, true},
      {"core_work_merge_overflow", "full_ball_counter_overflow", FullBallStatus::kResourceExhausted, false},
      {"wrong_admission", "full_ball_batch_target_rank", FullBallStatus::kInvariantViolated, true},
      {"wrong_admissible", "batch_cpu_actual_terminal_mismatch", FullBallStatus::kInvariantViolated, true}
    };
    u64 cases = 0, paid_after_fault = 0, prefix_cases = 0;
    for (int workers : {1, 4}) for (unsigned k : {2u, 3u}) for (const auto& fault : faults) {
      if (std::string_view(fault.name) == "core_work_merge_overflow" && k == 2) continue;
      batch_test::Owner owner(ix, balls, workers);
      owner.fault = fault.name; owner.fault_k = k;
      const auto result = mhgp7::build_full_ball_tower(ix, balls, fixture.kmax, workers, owner.resolver());
      if (result.status != fault.status || std::string_view(result.reason) != fault.reason)
        throw std::runtime_error(std::string("fault:") + fault.name + ":K" + std::to_string(k) + ":" + result.reason);
      need(result.orders.empty() && owner.active.load() == 0, "batch_failure_closed_joined");
      need(result.stats.static_batch_work_known == fault.known, "batch_failure_work_known");
      need(owner.reference_calls > 0, "batch_reject_direct_reference_nonvacuous");
      if (fault.known) { need(result.stats.resolve_work.calls > 0, "batch_paid_failure_nonzero"); ++paid_after_fault; }
      if (std::string_view(fault.name) == "capacity_overflow")
        need(result.stats.resolve_work.calls == owner.paid_calls, "batch_capacity_failure_preserves_all_paid_calls");
      if (k == 3) { need(result.stats.static_batch_calls >= 2, "batch_failure_after_prefix"); ++prefix_cases; }
      ++cases;
    }
    auto different_index = build_cloud_index(in);
    batch_test::Owner wrong_owner(different_index, balls, 1);
    const auto rejected_owner = mhgp7::build_full_ball_tower(ix, balls, 5, 1, wrong_owner.resolver());
    need(rejected_owner.status == FullBallStatus::kInvalidInput && rejected_owner.orders.empty() &&
        std::string_view(rejected_owner.reason) == "batch_cpu_owner_mismatch" &&
        rejected_owner.stats.resolve_work.calls == 0, "batch_same_shape_foreign_owner");
    const auto invalid_mode = mhgp7::build_full_ball_tower(ix, balls, 5, 0, wrong_owner.resolver());
    need(invalid_mode.status == FullBallStatus::kInvalidInput && invalid_mode.orders.empty() &&
        std::string_view(invalid_mode.reason) == "full_ball_batch_requires_static", "batch_explicit_opt_in");
    const Fixture pair{"pair_batch_empty", {{0,0,0},{2,0,0}}, 2};
    const auto pair_ix = build_cloud_index(input(pair, 0));
    const oracle::Model pair_model(pair.points);
    const auto pair_balls = catalogue(pair.points, pair_ix, pair_model, pair.kmax);
    batch_test::Owner unused(pair_ix, pair_balls, 4);
    const auto no_requests = mhgp7::build_full_ball_tower(pair_ix, pair_balls, 2, 4, unused.resolver());
    need(no_requests.status == FullBallStatus::kCompleteRelative && no_requests.orders.size() == 2 &&
        unused.batches == 0 && unused.reference_calls == 0, "batch_zero_representatives_and_Kn");
    need(cases == 62 && prefix_cases == 32 && paid_after_fault == 44, "batch_reject_nonvacuity");
    std::printf("{\"status\":\"passed_batch_rejections\",\"cases\":65,\"prefix_cases\":32,\"known_paid\":44,"
        "\"product_instrumentation\":false}\n");
    return 0;
  } catch (const Failure& error) { std::fprintf(stderr, "%s\n", error.why); }
  catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); }
  return 1;
}
}  // namespace

mhgp7::FullBallTowerResult observed_batch_tower(const mhgp7::CloudIndex& ix,
    std::span<const mhgp7::BallData> balls, unsigned kmax, int workers) {
  auto baseline = mhgp7::build_full_ball_tower(ix, balls, kmax, workers);
  if (workers <= 0 || baseline.status != mhgp7::FullBallStatus::kCompleteRelative) return baseline;
  mhgp7::batch_test::Owner owner(ix, balls, workers);
  auto result = mhgp7::build_full_ball_tower(ix, balls, kmax, workers, owner.resolver());
  need(result.status == mhgp7::FullBallStatus::kCompleteRelative, "batch.nominal.complete");
  const auto before_payload = checks;
  same_payload(baseline, result);
  callback_payload_checks += checks - before_payload;
  const auto before_work = checks;
  same_geometry_work(baseline.stats, result.stats);
  callback_work_checks += checks - before_work;
  need(owner.requests == owner.checked_terminals && owner.batches == result.stats.static_batch_calls &&
      owner.active.load() == 0, "batch.nominal.direct_terminals_and_join");
  callback_batches += owner.batches; callback_requests += owner.requests;
  direct_terminals += owner.checked_terminals; reference_meb_calls += owner.reference_calls;
  return result;
}

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string_view mode(argv[1]);
  if (mode == "--rejects") return callback_rejects();
  if (mode != "--selftest-1" && mode != "--selftest-4") return 2;
  char one[] = "--static-1", four[] = "--static-4";
  char* args[]{argv[0], mode == "--selftest-1" ? one : four};
  const int code = mhgp7_embedded_batch_full_gate_main(2, args);
  if (code) return code;
  try {
    need(callback_batches >= 20 && callback_requests >= 80 && direct_terminals == callback_requests &&
        reference_meb_calls > 0 && callback_payload_checks > 100 && callback_work_checks > 100,
        "batch.nominal.nonvacuity");
    std::printf("{\"status\":\"passed_batch_callback\",\"workers\":%d,\"batch_calls\":%llu,"
        "\"batch_requests\":%llu,\"direct_terminals\":%llu,\"diagnostic_reference_MEB_calls\":%llu,"
        "\"payload_checks\":%llu,\"work_checks\":%llu,\"product_instrumentation\":false}\n",
        static_threads, static_cast<unsigned long long>(callback_batches),
        static_cast<unsigned long long>(callback_requests), static_cast<unsigned long long>(direct_terminals),
        static_cast<unsigned long long>(reference_meb_calls), static_cast<unsigned long long>(callback_payload_checks),
        static_cast<unsigned long long>(callback_work_checks));
    return 0;
  } catch (const Failure& error) { std::fprintf(stderr, "%s\n", error.why); }
  catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); }
  return 1;
}

