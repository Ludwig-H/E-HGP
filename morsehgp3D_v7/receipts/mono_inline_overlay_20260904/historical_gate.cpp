// Overlay-only qualification: real pthread symbol interposition, no cloud.
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <dlfcn.h>
#include <memory>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(MHGP7_MONO_REFERENCE)
#include "reference_run.hpp"
#else
#include "overlay_run.hpp"
#endif

using namespace mhgp7;
namespace {
std::atomic<unsigned> created{0}, active{0};
std::atomic<bool> refuse_creation{false};
struct Start { void* (*function)(void*); void* argument; };
void* trampoline(void* argument) {
  std::unique_ptr<Start> start(static_cast<Start*>(argument));
  struct Active { ~Active() { --active; } } guard;
  return start->function(start->argument);
}
int failures = 0;
void check(bool ok, const char* text) {
  if (!ok) { ++failures; std::fprintf(stderr, "FAIL %s\n", text); }
}
bool empty_payload(const RunResult& result) {
  return result.digest_raw_candidates.empty() && result.digest_balls.empty() &&
         result.digest_postprefilter.empty() && result.digest_all.empty() &&
         result.digest_forest.empty() && result.cards.empty() && result.total_events == 0 &&
         result.total_facets == 0 && result.total_fusions == 0 && result.total_deltas == 0 &&
         result.total_nodes == 0 && result.forest_storage.empty() &&
         result.sum_parents_by_k.empty() && result.sum_parents_total == 0;
}
std::vector<InputPoint> fixture() {
  const P3 points[] = {
      {31052,37054,53791}, {63099,62295,5489}, {45851,18621,10092},
      {32290,41054,26270}, {35795,23044,15792}, {22475,26532,25195},
      {55919,55323,7531}, {60817,37898,64418}, {48853,14056,27781},
      {26341,59313,45083}, {7417,12277,35399}};
  std::vector<InputPoint> points_id;
  for (size_t i = 0; i < 11; ++i) points_id.push_back({static_cast<PointId>(i), points[i]});
  return points_id;
}
struct Observation {
  RunResult result;
  std::string exception;
  std::vector<u64> callbacks;
  std::vector<std::pair<u64, FoldPhase>> phases;
  unsigned pthreads = 0, callbacks_off_main = 0;
};
Observation exercise(const std::vector<InputPoint>& input, int kmax, bool complete,
                     bool join, int threads, const std::string& fault = "") {
  Observation observation;
  const auto caller = std::this_thread::get_id();
  RunOptions options;
  options.threads = threads;
  options.fold_inflight = 1;
  options.fold_join_before_next_k = join;
  options.smax = static_cast<u64>(kmax + 1);
  options.complete_silent_incidence = complete;
  options.digest = true;
  options.diagnostic_raw_candidates_digest = true;
  options.forest_layout = ForestLayout::kCsr;
  std::mutex observations_mutex;
  options.on_forest = [&](u64 K, const std::vector<ForestEvent>&, const ForestResult&) {
    std::lock_guard<std::mutex> lock(observations_mutex);
    observation.callbacks.push_back(K);
    if (std::this_thread::get_id() != caller) ++observation.callbacks_off_main;
    if (fault == "forest2" && K == 2) throw std::runtime_error("fixture forest2");
    if (fault == "late_alloc3" && K == 3) throw std::bad_alloc();
  };
  options.on_fold_phase = [&](u64 K, FoldPhase phase) {
    std::lock_guard<std::mutex> lock(observations_mutex);
    observation.phases.emplace_back(K, phase);
    if (fault == "begin2" && K == 2 && phase == FoldPhase::kReduceBegin)
      throw std::runtime_error("fixture begin2");
    if (fault == "published2" && K == 2 && phase == FoldPhase::kPublished)
      throw std::runtime_error("fixture published2");
  };
  created.store(0);
  try { observation.result = run_pipeline(input, options); }
  catch (const std::runtime_error& error) { observation.exception = error.what(); }
  observation.pthreads = created.load();
  check(active.load() == 0, "all created pthreads drained before handoff");
  return observation;
}
void print_object(const char* label, int kmax, bool complete, const Observation& observation) {
  std::printf("object=%s kmax=%d complete=%d exception=%s status=%d callbacks=",
              label, kmax, complete, observation.exception.c_str(), static_cast<int>(observation.result.status));
  for (const u64 K : observation.callbacks) std::printf("%llu,", static_cast<unsigned long long>(K));
  std::printf(" digest=%s\n", observation.result.digest_all.c_str());
  for (size_t K = 1; K < observation.result.cards.size(); ++K) {
    const auto& card = observation.result.cards[K];
    std::printf("K=%zu digest=%s events=%llu facets=%llu deltas=%llu attachments=%llu fusions=%llu nodes=%llu\n",
                K, observation.result.digest_forest[K].c_str(),
                static_cast<unsigned long long>(card.events), static_cast<unsigned long long>(card.facets),
                static_cast<unsigned long long>(card.deltas), static_cast<unsigned long long>(card.attachments),
                static_cast<unsigned long long>(card.fusions), static_cast<unsigned long long>(card.nodes));
  }
  std::printf("phases=");
  for (const auto& [K, phase] : observation.phases)
    std::printf("%llu:%u,", static_cast<unsigned long long>(K), static_cast<unsigned>(phase));
  std::printf("\n");
}
}  // namespace

// --wrap=pthread_create would miss references originating inside a shared
// libstdc++; symbol interposition catches std::thread's real dynamic call.
extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attributes,
                              void* (*function)(void*), void* argument) noexcept {
  using Create = int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
  static const auto real_create = reinterpret_cast<Create>(dlsym(RTLD_NEXT, "pthread_create"));
  ++created;
  if (refuse_creation.load()) return EAGAIN;
  if (!real_create) return ENOSYS;
  Start* start = new (std::nothrow) Start{function, argument};
  if (!start) return ENOMEM;
  ++active;
  const int result = real_create(thread, attributes, trampoline, start);
  if (result != 0) { --active; delete start; }
  return result;
}

int main(int argc, char** argv) {
  if (argc > 2) return 2;
  const std::string argument = argc == 2 ? argv[1] : "";
  const bool late_a = argument == "--inject=fold-inject-a-failure-k2";
  const bool late_b = argument == "--inject=fold-inject-b-exception-k3";
  const bool early_alloc = argument == "--inject=caps-throw-bad-alloc-fold";
  if (!argument.empty() && argument != "--require-zero" && !late_a && !late_b && !early_alloc) return 2;
  if (late_a || late_b || early_alloc)
    check(mutants_enable(argument.substr(9)), "existing failure mutant enabled before any pipeline");
  bool require_zero = true;
#if defined(MHGP7_MONO_REFERENCE)
  require_zero = argument == "--require-zero";
#endif
  // Non-vacuous interception witness before observing any pipeline.
  std::thread([] {}).join();
  check(created.load() == 1 && active.load() == 0, "pthread hook observes real std::thread");
  const auto input = fixture();
  if (late_a || late_b || early_alloc) {
    for (const bool complete : {false, true}) {
      const Observation failed = exercise(input, 10, complete, true, 1);
      const std::vector<u64> wanted = late_a ? std::vector<u64>{1} : late_b ? std::vector<u64>{1,2} : std::vector<u64>{};
      check(failed.callbacks == wanted, "injected failure preserves exact provisional callback prefix");
      if (late_b) check(!failed.exception.empty(), "late B exception preserved");
      else check(failed.exception.empty() && empty_payload(failed.result) &&
                 failed.result.status == (late_a ? PipelineStatus::kInvariantViolated : PipelineStatus::kResourceExhausted),
                 "injected status failure invalidates payload");
      check(failed.pthreads == (require_zero ? 0u : late_a ? 1u : late_b ? 3u : 1u), "injected failure thread count");
      print_object(argument.c_str(), 10, complete, failed);
    }
    std::fprintf(stderr, "mono_inline_injected cases=2 failures=%d require_zero=%d\n", failures, require_zero);
    return failures == 0 ? 0 : 1;
  }
  unsigned successful = 0, rejected = 0, total_baseline_threads = 0;
  for (const bool complete : {false, true}) {
    for (const int kmax : {5, 10}) {
      const Observation mono = exercise(input, kmax, complete, true, 1);
      check(mono.exception.empty() && mono.result.status == PipelineStatus::kCompleteRegular, "full tower completes");
      check(mono.result.kmax_eff == static_cast<u64>(kmax) && mono.callbacks.size() == static_cast<size_t>(kmax), "whole K tower callbacks");
      check(!mono.result.digest_all.empty() && mono.result.cards.back().events > 0, "top order non-vacuous");
      check(mono.pthreads == (require_zero ? 0u : static_cast<unsigned>(kmax)), "mono pthread creation count");
      check(mono.callbacks_off_main == (require_zero ? 0u : static_cast<unsigned>(kmax)), "mono callback caller thread");
      const Observation overlap = exercise(input, kmax, complete, false, 1);
      check(overlap.exception.empty() && overlap.result.status == PipelineStatus::kCompleteRegular &&
            overlap.result.digest_all == mono.result.digest_all && overlap.result.cards == mono.result.cards &&
            overlap.result.digest_forest == mono.result.digest_forest && overlap.callbacks == mono.callbacks,
            "full tower agrees with unchanged threaded route");
      check(overlap.pthreads == static_cast<unsigned>(kmax), "threads1 without join stays threaded");
      total_baseline_threads += overlap.pthreads;
      print_object("nominal", kmax, complete, mono);
      ++successful;
    }
  }
  // Guard's other side: join=true alone is insufficient when threads>1.
  const Observation parallel = exercise(input, 5, false, true, 2);
  check(parallel.result.status == PipelineStatus::kCompleteRegular && parallel.pthreads >= 5,
        "threads2 with join remains threaded");
  for (const bool complete : {false, true}) {
    for (const std::string fault : {"forest2", "begin2", "published2", "late_alloc3"}) {
      const Observation observation = exercise(input, 10, complete, true, 1, fault);
      const size_t expected = fault == "begin2" ? 1 : fault == "late_alloc3" ? 3 : 2;
      check(observation.callbacks.size() == expected, "failure callback prefix retained only provisionally");
      if (fault == "late_alloc3") {
        check(observation.exception.empty() && observation.result.status == PipelineStatus::kResourceExhausted &&
              empty_payload(observation.result), "late bad_alloc refuses and invalidates every payload field");
      } else check(!observation.exception.empty(), "callback exception preserved");
      check(!require_zero || observation.pthreads == 0, "failure also creates zero threads");
      print_object(fault.c_str(), 10, complete, observation);
      ++rejected;
    }
  }
  std::fprintf(stderr, "mono_inline_gate successful=%u rejected=%u reference_threads=%u failures=%d require_zero=%d\n",
               successful, rejected, total_baseline_threads, failures, require_zero);
  return failures == 0 && successful == 4 && rejected == 8 && total_baseline_threads == 30 ? 0 : 1;
}
