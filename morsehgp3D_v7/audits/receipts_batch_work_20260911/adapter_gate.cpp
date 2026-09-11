// Test only the real adapter's accounting boundary. No geometry or GPU execution.
#include <cstdio>
#include <cstring>
#include <limits>
#include "batch_route.cuh"
#include "source/morsehgp3D_v7/src/forest/full_ball_tower.hpp"

namespace mhgp7::gpu_terminal_batch_private {
// Injection substitutes only Context's return value; the adapter body is unchanged.
class InjectedContext {
 public:
  u64 first_supports = 2;
  bool known = true;
  bool bound_to(const CloudIndex&, std::span<const BallData>) const { return true; }
  terminal::SeedView seeds(u32 k) const { return {nullptr, 0, 1, k}; }
  u64 snapshot() const { return 1; }
  size_t capacity() const { return 2; }
  Batch execute(std::span<const terminal::Request> requests) const {
    Batch out;
    out.raw_work_known = known;
    out.complete = known;
    out.failure = known ? nullptr : "injected_unknown_work";
    if (!known) return out;
    for (size_t j = 0; j < requests.size(); ++j) {
      WireResult row;
      row.result.status = terminal::Status::kOk;
      row.result.target = 0;
      row.result.ordinal = requests[j].ordinal;
      row.result.work.calls = 1;
      row.result.work.materializations = 1;
      row.result.work.supports[2] = j ? 1 : first_supports;
      out.diagnostic.push_back(row);
      out.accepted.push_back(row.result);
    }
    return out;
  }
};
}  // namespace mhgp7::gpu_terminal_batch_private

#define Context InjectedContext
#include "batch_adapter.hpp"
#undef Context

namespace {
using namespace mhgp7;
namespace route = gpu_terminal_batch_private;

struct Observation {
  FullBallBatchResult output;
  bool overflow = false;
};

Observation call(u64 first_supports, bool known = true) {
  CloudIndex index;
  std::array<BallData, 1> balls{};
  std::array<FullBallBatchRequest, 2> requests{};
  requests[0].ordinal = 17;
  requests[1].ordinal = 42;
  route::InjectedContext context;
  context.first_supports = first_supports;
  context.known = known;
  Observation observed;
  try {
    route::resolve_batch(&context, {index, balls, {}}, {2, {}, requests}, observed.output);
  } catch (const full_ball_detail::Failure& error) {
    if (error.status != FullBallStatus::kResourceExhausted ||
        std::strcmp(error.reason, "full_ball_counter_overflow")) throw;
    observed.overflow = true;
  }
  return observed;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::strcmp(argv[1], "--selftest")) return 2;
  const auto maximum = std::numeric_limits<mhgp7::u64>::max();
  const auto nominal = call(2);
  const auto exact = call(maximum - 1);
  const auto unknown = call(0, false);
  const auto overflow = call(maximum);
  const bool positives = !nominal.overflow && nominal.output.work_known &&
      nominal.output.status == mhgp7::FullBallStatus::kCompleteRelative &&
      nominal.output.work.resolve_work.calls == 2 &&
      nominal.output.work.resolve_work.supports_by_size[2] == 3 &&
      nominal.output.targets.size() == 2 && nominal.output.targets[0].ordinal == 17 &&
      nominal.output.targets[1].ordinal == 42 && !exact.overflow && exact.output.work_known &&
      exact.output.work.resolve_work.supports_by_size[2] == maximum &&
      exact.output.targets.size() == 2 && !unknown.output.work_known && unknown.output.targets.empty();
  const bool rejected = overflow.overflow && overflow.output.targets.empty() &&
      !overflow.output.work_known;
  std::printf("{\"positive_boundary_unknown_cases_pass\":%s,\"overflow_thrown\":%s,"
      "\"overflow_work_known\":%s,\"overflow_partial_calls\":%llu,"
      "\"overflow_partial_q2\":%llu,\"overflow_public_targets\":%zu,"
      "\"contract_pass\":%s,\"geometry_executed\":false,\"device_executed\":false}\n",
      positives ? "true" : "false", overflow.overflow ? "true" : "false",
      overflow.output.work_known ? "true" : "false",
      static_cast<unsigned long long>(overflow.output.work.resolve_work.calls),
      static_cast<unsigned long long>(overflow.output.work.resolve_work.supports_by_size[2]),
      overflow.output.targets.size(), positives && rejected ? "true" : "false");
  return positives && rejected ? 0 : 1;
}
