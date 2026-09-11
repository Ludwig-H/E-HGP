#define main mhgp7_original_full_gate_main
#include "source/morsehgp3D_v7/tests/full_ball_tower_gate.cpp"
#undef main
#include "batch_adapter.hpp"
#include "cuda_trial/reference.hpp"

namespace batch_gate {
namespace route = mhgp7::gpu_terminal_batch_private;
namespace terminal = mhgp7::gpu_terminal_private;
using namespace mhgp7;
u64 pairs = 0, batches = 0, hits = 0, requests_compared = 0, rejects = 0;
u64 terminals_compared = 0, aggregate_rejects = 0;
u64 omission_reuse = 0, omission_growth = 0, initialization_rollbacks = 0;
u64 reuse_h2d_bytes = 0, reuse_initialization_bytes = 0;

struct CheckedResolver {
  route::Context& device;
  terminal_cuda_gate::CpuTerminal reference;
  static void resolve(void* opaque, const FullBallGeometryView& geometry,
      const FullBallBatchView& batch, FullBallBatchResult& output) {
    auto& owner = *static_cast<CheckedResolver*>(opaque);
    route::resolve_batch(&owner.device, geometry, batch, output);
    need(output.status == FullBallStatus::kCompleteRelative, "batch.direct_success");
    u64 calls = 0;
    for (size_t j = 0; j < batch.requests.size(); ++j) {
      const auto& source = batch.requests[j];
      terminal::Request request;
      request.snapshot = owner.device.snapshot(); request.k = batch.k; request.ordinal = source.ordinal;
      request.before = terminal::encode_level(geometry.balls[source.consumer].level);
      std::copy(source.key.begin(), source.key.end(), request.selected);
      std::vector<terminal::TraceRow> trace;
      const auto expected = owner.reference.resolve(request, trace);
      need(expected.status == terminal::Status::kOk && output.targets.at(j).ball == expected.target &&
          output.targets[j].ordinal == expected.ordinal, "batch.direct_terminal_identity");
      full_ball_detail::add(calls, expected.work.calls);
      ++terminals_compared;
    }
    need(calls == output.work.resolve_work.calls + output.work.static_post_seed_hits[batch.k],
        "batch.direct_saved_last_MEB");
  }
};

void work_equal(const FullBallStats& a, const FullBallStats& b) {
  need(a.resolve_work.calls == b.resolve_work.calls &&
      a.resolve_work.materializations == b.resolve_work.materializations &&
      a.resolve_work.power_tests == b.resolve_work.power_tests &&
      a.resolve_work.supports_by_size == b.resolve_work.supports_by_size, "batch.paid_MEB_work");
  for (auto member : {&FullBallStats::key_lookups, &FullBallStats::anchor_hits,
      &FullBallStats::intruder_queries, &FullBallStats::intruder_nodes, &FullBallStats::intruder_power_tests,
      &FullBallStats::interior_ranges, &FullBallStats::same_radius_steps, &FullBallStats::descending_steps,
      &FullBallStats::max_chain_steps}) need(a.*member == b.*member, "batch.paid_descent_work");
  need(a.static_requests == b.static_requests && a.static_unique == b.static_unique &&
      a.static_seeded == b.static_seeded && a.static_post_seed_queries == b.static_post_seed_queries &&
      a.static_post_seed_hits == b.static_post_seed_hits &&
      a.static_post_seed_terminals == b.static_post_seed_terminals, "batch.RUSQHT");
}
void fixture(const Fixture& f, unsigned variant) {
  context = std::string(f.name) + "/batch/" + std::to_string(variant);
  const auto in = input(f, variant);
  std::vector<P3> points;
  for (const auto& p : in) points.push_back(p.position);
  const oracle::Model model(points);
  const auto ix = build_cloud_index(in);
  auto balls = catalogue(points, ix, model, f.kmax);
  if (variant) std::reverse(balls.begin(), balls.end());
  const auto expected = build_full_ball_tower(ix, balls, f.kmax, 1);
  need(expected.status == FullBallStatus::kCompleteRelative, "batch.CPU_reference_success");
  route::Context device(ix, balls);
  CheckedResolver checked{device, terminal_cuda_gate::CpuTerminal(ix, balls)};
  const auto source_uploads = device.backend().work.h2d_bytes;
  const auto source_allocations = device.backend().work.allocations;
  for (int threads : {1, 4}) {
    const auto actual = build_full_ball_tower(ix, balls, f.kmax, threads,
        FullBallBatchResolver{&checked, CheckedResolver::resolve});
    if (actual.status != FullBallStatus::kCompleteRelative)
      std::fprintf(stderr, "batch_tower_failure=%s\n", actual.reason);
    same_payload(expected, actual); work_equal(expected.stats, actual.stats);
    need(actual.stats.static_batch_work_known, "batch.known_work_on_success");
    for (auto h : actual.stats.static_post_seed_hits) hits += h;
    for (size_t k = 2; k < actual.stats.static_unique.size(); ++k)
      requests_compared += actual.stats.static_unique[k] - actual.stats.static_seeded[k];
    batches += actual.stats.static_batch_calls; ++pairs;
  }
  need(device.backend().work.h2d_bytes >= source_uploads &&
      device.backend().work.allocations >= source_allocations, "batch.resident_accounting");
  need(device.close() && device.backend().allocation_bytes == device.backend().certified_free_bytes,
      "batch.owner_cleanup");
}

void transaction_tests() {
  const auto f = fixtures()[1];
  const auto in = input(f, 0);
  const auto ix = build_cloud_index(in);
  const oracle::Model model(f.points);
  const auto balls = catalogue(f.points, ix, model, f.kmax);
  // Synthetic accounting fault, not a claimed geometrically realizable work
  // count: even the first field overflowing must not publish a partial sum.
  {
    std::array<route::WireResult, 2> rows{};
    rows[0].result.work.calls = ~u64{0}; rows[1].result.work.calls = 1;
    FullBallBatchResult output;
    bool rejected = false;
    try { route::publish_work(rows, 2, output); }
    catch (const full_ball_detail::Failure& error) {
      rejected = error.status == FullBallStatus::kResourceExhausted &&
          std::string_view(error.reason) == "full_ball_counter_overflow";
    }
    need(rejected && !output.work_known && output.work.resolve_work.calls == 0 && output.targets.empty(),
        "batch.synthetic_aggregate_overflow_is_unknown");
    ++aggregate_rejects;
  }
  for (auto fault : {route::transport::Fault::kAllocation, route::transport::Fault::kUpload}) {
    for (unsigned after : {0u, 3u}) {
      bool rejected = false;
      try { route::Context invalid(ix, balls, fault, after); }
      catch (const route::transport::Error& error) {
        rejected = std::string_view(error.reason) == "injected_transport_failure";
      }
      need(rejected, "batch.constructor_partial_residency_rollback"); ++rejects;
    }
  }
  const auto make_requests = [&](const route::Context& device) {
    std::vector<terminal::Request> result;
    for (i32 a = 0; a < static_cast<i32>(ix.upos.size()); ++a)
      for (i32 b = a + 1; b < static_cast<i32>(ix.upos.size()); ++b) {
        terminal::Request r;
        r.snapshot = device.snapshot(); r.k = 2; r.selected[0] = a; r.selected[1] = b;
        r.ordinal = (u64{1} << 40) + result.size();
        r.before = terminal::encode_level(ExactLevel{{12884508676ull, 0, 0}, 1});
        result.push_back(r);
      }
    result.back().ordinal = ~u64{0};
    return result;
  };
  {
    route::Context device(ix, balls);
    auto requests = make_requests(device);
    const auto launch_before = device.backend().work.launches;
    const auto allocation_before = device.backend().work.allocations;
    const auto upload_before = device.backend().work.h2d_bytes;
    const auto download_before = device.backend().work.d2h_bytes;
    need(device.execute({}).complete && device.backend().work.launches == launch_before &&
        device.backend().work.allocations == allocation_before &&
        device.backend().work.h2d_bytes == upload_before && device.initialization_calls() == 0,
        "batch.empty_no_work");
    need(device.execute(std::span<const terminal::Request>(requests).first(1)).complete, "batch.singleton");
    need(device.execute(requests).complete, "batch.growth");
    const auto allocated = device.backend().work.allocations;
    const auto capacity = device.capacity();
    need(device.execute(std::span<const terminal::Request>(requests).first(3)).complete &&
        device.execute(requests).complete && device.backend().work.allocations == allocated &&
        device.capacity() == capacity, "batch.capacity_reuse");
    const u64 slots = 1 + requests.size() + 3 + requests.size();
    reuse_h2d_bytes = device.backend().work.h2d_bytes - upload_before;
    reuse_initialization_bytes = device.initialization_bytes();
    need(reuse_h2d_bytes == slots * sizeof(terminal::Request) &&
        device.backend().work.d2h_bytes - download_before == slots * sizeof(route::WireResult),
        "traffic.requests_only_H2D_results_only_D2H");
    need(device.backend().work.allocations - allocation_before == 4 &&
        device.initialization_calls() == 2 &&
        reuse_initialization_bytes == (1 + requests.size()) * sizeof(route::WireResult),
        "traffic.initialize_only_new_capacity");
    need(device.close(), "batch.reuse_close");
  }
  // Reuse must refuse an omitted row whose old result is otherwise valid for
  // exactly the same request. Growth must refuse a never-written new row.
  for (bool growth : {false, true}) for (u64 slot : {0ull, 5ull, 9ull}) {
    route::Context device(ix, balls);
    const auto requests = make_requests(device);
    if (growth) {
      need(device.execute(std::span<const terminal::Request>(requests).first(1)).complete,
          "traffic.growth_prime");
    } else {
      need(device.execute(requests).complete &&
          device.execute(std::span<const terminal::Request>(requests).first(3)).complete,
          "traffic.reuse_prime");
    }
    const auto allocations = device.backend().work.allocations;
    const auto uploads = device.backend().work.h2d_bytes;
    const auto init_calls = device.initialization_calls(), init_bytes = device.initialization_bytes();
    const auto rejected = device.execute(requests, route::Mutation::kOmit, slot);
    need(!rejected.complete && rejected.accepted.empty() && !rejected.raw_work_known &&
        std::string_view(rejected.failure) == "batch_result_rejected",
        growth ? "traffic.omission_after_growth" : "traffic.omission_after_reuse");
    need(device.backend().work.allocations - allocations == (growth ? 2u : 0u) &&
        device.initialization_calls() - init_calls == (growth ? 1u : 0u) &&
        device.initialization_bytes() - init_bytes == (growth ? requests.size() * sizeof(route::WireResult) : 0u) &&
        device.backend().work.h2d_bytes - uploads == requests.size() * sizeof(terminal::Request),
        "traffic.omission_exact_transfers");
    if (growth) {
      need(rejected.diagnostic[slot].written == 0, "traffic.grown_omission_unwritten");
      ++omission_growth;
    } else {
      need(rejected.diagnostic[slot].written == 1 &&
          rejected.diagnostic[slot].batch < rejected.diagnostic[slot == 0 ? 1 : 0].batch,
          "traffic.reused_omission_stale_epoch");
      ++omission_reuse;
    }
    need(!device.execute(requests).complete, "traffic.omission_poisons_owner");
    need(device.close() && device.backend().allocation_bytes == device.backend().certified_free_bytes,
        "traffic.omission_cleanup");
    ++rejects;
  }
  {
    route::Context device(ix, balls);
    const auto requests = make_requests(device);
    need(device.execute(std::span<const terminal::Request>(requests).first(1)).complete,
        "traffic.rollback_prime");
    const auto allocations = device.backend().work.allocations, uploads = device.backend().work.h2d_bytes;
    const auto launches = device.backend().work.launches, init_calls = device.initialization_calls();
    device.inject(route::transport::Fault::kUpload);  // new-buffer initialization write, not request upload
    const auto rejected = device.execute(requests);
    need(!rejected.complete && rejected.accepted.empty() && !rejected.raw_work_known &&
        std::string_view(rejected.failure) == "injected_transport_failure" &&
        device.capacity() == 1 && device.backend().work.allocations == allocations + 2 &&
        device.initialization_calls() == init_calls && device.backend().work.launches == launches &&
        device.backend().work.h2d_bytes == uploads, "traffic.initialization_failure_keeps_old_buffers");
    device.inject(route::transport::Fault::kNone);
    need(device.close() && device.backend().allocation_bytes == device.backend().certified_free_bytes,
        "traffic.initialization_failure_cleanup");
    ++initialization_rollbacks; ++rejects;
  }
  for (auto mutation : {route::Mutation::kOmit, route::Mutation::kOrdinal, route::Mutation::kSnapshot,
      route::Mutation::kBatch, route::Mutation::kStatus, route::Mutation::kTarget, route::Mutation::kWork}) {
    for (u64 slot : {0ull, 5ull, 9ull}) {
      route::Context device(ix, balls);
      const auto requests = make_requests(device);
      const auto rejected = device.execute(requests, mutation, slot);
      need(!rejected.complete && rejected.accepted.empty() && !rejected.raw_work_known &&
          std::string_view(rejected.failure) == "batch_result_rejected", "batch.whole_rejection");
      need(!device.execute(requests).complete, "batch.poisoned_after_failure");
      need(device.close(), "batch.mutation_close"); ++rejects;
    }
  }
  for (auto fault : {route::transport::Fault::kAllocation, route::transport::Fault::kUpload,
      route::transport::Fault::kLaunch, route::transport::Fault::kSynchronize,
      route::transport::Fault::kDownload, route::transport::Fault::kHostPublish}) {
    route::Context device(ix, balls);
    const auto requests = make_requests(device);
    device.inject(fault);
    const auto rejected = device.execute(requests);
    need(!rejected.complete && rejected.accepted.empty() &&
        rejected.raw_work_known == (fault == route::transport::Fault::kHostPublish), "batch.failure_accounting");
    device.inject(route::transport::Fault::kNone);
    need(device.close() && device.backend().allocation_bytes == device.backend().certified_free_bytes,
        "batch.failure_cleanup"); ++rejects;
  }
  {
    route::Context device(ix, balls);
    const auto requests = make_requests(device);
    device.inject(route::transport::Fault::kAllocation, 1);
    const auto rejected = device.execute(requests);
    need(!rejected.complete && rejected.accepted.empty() && !rejected.raw_work_known,
        "batch.second_allocation_failure");
    device.inject(route::transport::Fault::kNone);
    need(device.close() && device.backend().allocation_bytes == device.backend().certified_free_bytes,
        "batch.second_allocation_rollback"); ++rejects;
  }
  {
    route::Context device(ix, balls);
    const auto requests = make_requests(device);
    need(device.execute(requests).complete, "batch.release_fault_precondition");
    device.inject(route::transport::Fault::kRelease);
    need(!device.close() && !device.close() && !device.backend().cleanup_certified &&
        device.backend().free_failures > 0, "batch.uncertified_release_is_sticky"); ++rejects;
  }
  {
    route::Context device(ix, balls);
    auto requests = make_requests(device);
    requests[5].snapshot ^= 1;
    const auto launches = device.backend().work.launches, allocations = device.backend().work.allocations;
    const auto rejected = device.execute(requests);
    need(!rejected.complete && rejected.accepted.empty() && device.backend().work.launches == launches &&
        device.backend().work.allocations == allocations, "batch.prevalidate_every_input");
    need(device.close(), "batch.input_reject_close"); ++rejects;
  }
}
}  // namespace batch_gate

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    auto cases = fixtures();
    cases.push_back({"post_seed_ABEZW", {{0,3,0},{8,3,0},{4,2,0},{6,0,0},{4,23,0}}, 5});
    cases.push_back({"post_seed_square_partial", {{0,0,0},{10,0,0},{0,10,0},{10,10,0},
        {4,4,0},{5,4,0},{4,5,0},{5,5,0}}, 4});
    for (const auto& f : cases) for (unsigned variant : {0u, 1u}) batch_gate::fixture(f, variant);
    batch_gate::transaction_tests();
    need(batch_gate::pairs >= 64 && batch_gate::hits >= 4 && batch_gate::requests_compared > 100 &&
        batch_gate::batches > 0 && batch_gate::rejects == 41 && batch_gate::aggregate_rejects == 1 &&
        batch_gate::terminals_compared == batch_gate::requests_compared &&
        batch_gate::omission_reuse == 3 && batch_gate::omission_growth == 3 &&
        batch_gate::initialization_rollbacks == 1, "batch.nonvacuity");
    std::printf("{\"status\":\"passed\",\"scope\":\"private_seeded_terminal_batch_host\","
        "\"checks\":%llu,\"physical_pairs\":%llu,\"batches\":%llu,\"requests\":%llu,"
        "\"seed_hits\":%llu,\"transaction_rejections\":%llu,\"direct_terminals\":%llu,"
        "\"synthetic_aggregate_rejections\":%llu,\"omissions_after_reuse\":%llu,"
        "\"omissions_after_growth\":%llu,\"initialization_rollbacks\":%llu,"
        "\"reuse_H2D_bytes\":%llu,\"reuse_initialization_bytes\":%llu,"
        "\"request_size\":%zu,\"wire_result_size\":%zu,\"compact_target_size\":%zu,"
        "\"former_full_result_size\":%zu,\"device_executed\":false}\n",
        static_cast<unsigned long long>(checks), static_cast<unsigned long long>(batch_gate::pairs),
        static_cast<unsigned long long>(batch_gate::batches), static_cast<unsigned long long>(batch_gate::requests_compared),
        static_cast<unsigned long long>(batch_gate::hits), static_cast<unsigned long long>(batch_gate::rejects),
        static_cast<unsigned long long>(batch_gate::terminals_compared),
        static_cast<unsigned long long>(batch_gate::aggregate_rejects),
        static_cast<unsigned long long>(batch_gate::omission_reuse),
        static_cast<unsigned long long>(batch_gate::omission_growth),
        static_cast<unsigned long long>(batch_gate::initialization_rollbacks),
        static_cast<unsigned long long>(batch_gate::reuse_h2d_bytes),
        static_cast<unsigned long long>(batch_gate::reuse_initialization_bytes),
        sizeof(batch_gate::terminal::Request), sizeof(batch_gate::route::WireResult),
        sizeof(batch_gate::route::AcceptedTarget), sizeof(batch_gate::terminal::Result));
    return 0;
  } catch (const Failure& error) { std::fprintf(stderr, "%s: %s\n", context.c_str(), error.why); }
    catch (const batch_gate::route::transport::Error& error) { std::fprintf(stderr, "%s\n", error.reason); }
    catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); }
  return 1;
}
