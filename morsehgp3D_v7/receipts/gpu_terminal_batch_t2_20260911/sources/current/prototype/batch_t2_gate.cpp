// Test-only callback injection into the permanent real-census judge. It never
// constructs the product census from the independent oracle.
#include "batch_adapter.hpp"
#include "cuda_trial/reference.hpp"

namespace mhgp7 {
namespace batch_t2 {
namespace route = gpu_terminal_batch_private;
namespace terminal = gpu_terminal_private;
u64 towers = 0, calls = 0, direct[11]{}, queries[11]{}, hits[11]{};
bool high_k_done = false;
u64 high_direct[11]{}, high_queries[11]{}, high_hits[11]{}, high_q3[11]{}, high_q4[11]{};
void require(bool good, const char* reason) {
  if (!good) throw std::runtime_error(reason);
}
struct Owner {
  route::Context device;
  terminal_cuda_gate::CpuTerminal reference;
  Owner(const CloudIndex& ix, std::span<const BallData> balls) : device(ix, balls), reference(ix, balls) {}
  static void resolve(void* opaque, const FullBallGeometryView& geometry,
      const FullBallBatchView& batch, FullBallBatchResult& output) {
    auto& owner = *static_cast<Owner*>(opaque);
    route::resolve_batch(&owner.device, geometry, batch, output);
    require(output.status == FullBallStatus::kCompleteRelative, "T2.batch.complete");
    u64 paid_reference = 0;
    for (size_t j = 0; j < batch.requests.size(); ++j) {
      const auto& source = batch.requests[j];
      terminal::Request request;
      request.snapshot = owner.device.snapshot(); request.k = batch.k; request.ordinal = source.ordinal;
      request.before = terminal::encode_level(geometry.balls[source.consumer].level);
      std::copy(source.key.begin(), source.key.end(), request.selected);
      std::vector<terminal::TraceRow> trace;
      const auto expected = owner.reference.resolve(request, trace);
      require(expected.status == terminal::Status::kOk && output.targets.at(j).ball == expected.target &&
          output.targets[j].ordinal == expected.ordinal, "T2.batch.direct_terminal");
      full_ball_detail::add(paid_reference, expected.work.calls);
      ++direct[batch.k];
    }
    require(paid_reference == output.work.resolve_work.calls + output.work.static_post_seed_hits[batch.k],
        "T2.batch.saved_final_MEB");
    queries[batch.k] += output.work.static_post_seed_queries[batch.k];
    hits[batch.k] += output.work.static_post_seed_hits[batch.k];
    ++calls;
  }
};
// Separate diagnostic: all K9/K10 subsets of the first declared geometry
// variant/census, BEFORE any outcome is known. This is a bounded judge only;
// these upper-cut requests are not claimed to be chronological FULL demands.
void high_k_probe(Owner& owner, const CloudIndex& ix) {
  require(ix.upos.size() >= 10 && ix.upos.size() <= 14, "T2.highK.fixture_domain");
  std::vector<terminal::Request> requests;
  for (u32 mask = 1; mask < (u32{1} << ix.upos.size()); ++mask) {
    const unsigned k = std::popcount(mask);
    if (k != 9 && k != 10) continue;
    terminal::Request r;
    r.snapshot = owner.device.snapshot(); r.k = k; r.ordinal = (u64{1} << 40) + mask;
    r.before = terminal::encode_level(ExactLevel{{12884508676ull, 0, 0}, 1});
    unsigned at = 0;
    for (u32 bits = mask; bits; bits &= bits - 1) r.selected[at++] = static_cast<i32>(std::countr_zero(bits));
    requests.push_back(r);
  }
  require(!requests.empty(), "T2.highK.declared_nonempty");
  const auto batch = owner.device.execute(requests);
  require(batch.complete && batch.raw_work_known && batch.accepted.size() == requests.size(),
      "T2.highK.all_declared_requests_nominal");
  for (size_t j = 0; j < requests.size(); ++j) {
    const auto& r = requests[j];
    std::vector<terminal::TraceRow> trace;
    const auto expected = owner.reference.resolve(r, trace);
    const auto& actual = batch.diagnostic[j].result;
    const auto& w = actual.work;
    const u64 hit = w.post_seed_hits;
    require(expected.status == terminal::Status::kOk && actual.target == expected.target &&
        actual.ordinal == expected.ordinal && batch.accepted[j].target == expected.target &&
        batch.accepted[j].ordinal == expected.ordinal, "T2.highK.direct_terminal");
    require(hit <= 1 && !trace.empty() && expected.work.calls == w.calls + hit &&
        expected.work.materializations == w.materializations + hit &&
        expected.work.key_lookups == w.key_lookups + hit &&
        expected.work.anchor_hits == w.anchor_hits + hit &&
        expected.work.powers == w.powers + (hit ? trace.back().selection.powers : 0),
        "T2.highK.exact_saved_MEB_work");
    for (size_t q = 0; q < 5; ++q)
      require(expected.work.supports[q] == w.supports[q] + (hit ? trace.back().selection.supports[q] : 0),
          "T2.highK.exact_saved_support_work");
    for (auto member : {&terminal::Work::intruder_queries, &terminal::Work::intruder_nodes,
        &terminal::Work::intruder_power_tests, &terminal::Work::interior_ranges,
        &terminal::Work::same_radius_steps, &terminal::Work::descending_steps,
        &terminal::Work::max_chain_steps, &terminal::Work::axis_divisions})
      require(expected.work.*member == w.*member, "T2.highK.unchanged_descent_work");
    ++high_direct[r.k]; high_queries[r.k] += w.post_seed_queries; high_hits[r.k] += hit;
    // Reference q counts describe the paid observation, including the final
    // MEB deliberately absent from the seeded production result on a hit.
    for (const auto& row : trace) {
      high_q3[r.k] += row.selection.q == 3;
      high_q4[r.k] += row.selection.q == 4;
    }
  }
  high_k_done = true;
}
void same_work(const FullBallStats& a, const FullBallStats& b) {
  require(a.resolve_work.calls == b.resolve_work.calls &&
      a.resolve_work.materializations == b.resolve_work.materializations &&
      a.resolve_work.power_tests == b.resolve_work.power_tests &&
      a.resolve_work.supports_by_size == b.resolve_work.supports_by_size, "T2.batch.MEB_work");
  for (auto member : {&FullBallStats::key_lookups, &FullBallStats::anchor_hits,
      &FullBallStats::intruder_queries, &FullBallStats::intruder_nodes, &FullBallStats::intruder_power_tests,
      &FullBallStats::interior_ranges, &FullBallStats::same_radius_steps, &FullBallStats::descending_steps,
      &FullBallStats::max_chain_steps}) require(a.*member == b.*member, "T2.batch.descent_work");
  require(a.static_requests == b.static_requests && a.static_unique == b.static_unique &&
      a.static_seeded == b.static_seeded && a.static_post_seed_queries == b.static_post_seed_queries &&
      a.static_post_seed_hits == b.static_post_seed_hits && a.static_post_seed_terminals == b.static_post_seed_terminals,
      "T2.batch.RUSQHT");
}
}  // namespace batch_t2
FullBallTowerResult batch_t2_builder(const CloudIndex& ix, std::span<const BallData> balls,
    unsigned kmax, int threads = 0) {
  auto scalar = build_full_ball_tower(ix, balls, kmax, threads);
  if (!threads) return scalar;
  batch_t2::Owner owner(ix, balls);
  auto result = build_full_ball_tower(ix, balls, kmax, threads, {&owner, batch_t2::Owner::resolve});
  if (!batch_t2::high_k_done) batch_t2::high_k_probe(owner, ix);
  batch_t2::require(owner.device.close(), "T2.batch.cleanup");
  batch_t2::require(result.status == FullBallStatus::kCompleteRelative &&
      scalar.status == FullBallStatus::kCompleteRelative, "T2.batch.tower_success");
  batch_t2::same_work(scalar.stats, result.stats);
  ++batch_t2::towers;
  return result;
}
}  // namespace mhgp7

// The original Builder header was included before this call-site-only macro.
// Only test calls are replaced; the compiled Builder itself is byte-identical.
#define build_full_ball_tower batch_t2_builder
#include "source/morsehgp3D_v7/tests/census_tower_gate.cpp"
#undef build_full_ball_tower

int main(int argc, char** argv) {
  if (argc != 2 || (std::string_view(argv[1]) != "--line12" &&
      std::string_view(argv[1]) != "--shell14" && std::string_view(argv[1]) != "--spatial12")) return 2;
  const int status = mhgp7_private_census_main(argc, argv);
  if (status) return status;
  try {
    mhgp7::batch_t2::require(mhgp7::batch_t2::towers == 12, "T2.batch.tower_floor");
    std::printf("{\"status\":\"passed\",\"scope\":\"real_census_seeded_batch_T2\","
        "\"device_executed\":false,\"batch_towers\":%llu,\"batches\":%llu,\"per_k\":[",
        static_cast<unsigned long long>(mhgp7::batch_t2::towers),
        static_cast<unsigned long long>(mhgp7::batch_t2::calls));
    for (unsigned k = 2; k <= 10; ++k)
      std::printf("%s{\"K\":%u,\"direct_terminals\":%llu,\"Q\":%llu,\"H\":%llu}", k == 2 ? "" : ",", k,
          static_cast<unsigned long long>(mhgp7::batch_t2::direct[k]),
          static_cast<unsigned long long>(mhgp7::batch_t2::queries[k]),
          static_cast<unsigned long long>(mhgp7::batch_t2::hits[k]));
    std::printf("]}\n");
    mhgp7::batch_t2::require(mhgp7::batch_t2::high_k_done && mhgp7::batch_t2::high_direct[9] > 0 &&
        mhgp7::batch_t2::high_direct[10] > 0, "T2.highK.both_orders_nonempty");
    std::printf("{\"status\":\"passed\",\"scope\":\"separate_all_highK_subsets_upper_cut\","
        "\"device_executed\":false,\"failures_filtered\":0,\"per_k\":[");
    for (unsigned k : {9u, 10u})
      std::printf("%s{\"K\":%u,\"direct_terminals\":%llu,\"Q\":%llu,\"H\":%llu,"
          "\"reference_q3\":%llu,\"reference_q4\":%llu}", k == 9 ? "" : ",", k,
          static_cast<unsigned long long>(mhgp7::batch_t2::high_direct[k]),
          static_cast<unsigned long long>(mhgp7::batch_t2::high_queries[k]),
          static_cast<unsigned long long>(mhgp7::batch_t2::high_hits[k]),
          static_cast<unsigned long long>(mhgp7::batch_t2::high_q3[k]),
          static_cast<unsigned long long>(mhgp7::batch_t2::high_q4[k]));
    std::printf("]}\n");
    return 0;
  } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
