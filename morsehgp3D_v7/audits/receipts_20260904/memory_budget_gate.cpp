#include <atomic>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>
#include "instrumented/src/cloud/families.hpp"
#include "instrumented/src/pipeline/run.hpp"

using namespace mhgp7;
namespace {
const char* label = "";
bool hold_b1 = false;
std::atomic<bool> b1_active{false}, release_b1{false};
size_t prior_capacity = 0;
size_t observed_overlap = 0;
int errors = 0;
void check(bool value, const char* why) {
  if (!value) { ++errors; std::printf("FAIL %s\n", why); }
}
void wait_for(const std::atomic<bool>& flag) {
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!flag.load()) {
    if (std::chrono::steady_clock::now() >= deadline) throw std::runtime_error("audit_timeout");
    std::this_thread::yield();
  }
}
struct Result { RunResult run; size_t callbacks = 0; };
Result execute(const char* name, const std::vector<InputPoint>& points, RunOptions options) {
  label = name;
  Result result;
  options.digest = true;
  options.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++result.callbacks; };
  options.on_fold_phase = [&](u64 k, FoldPhase phase) {
    if (hold_b1 && k == 1 && phase == FoldPhase::kReduceBegin) {
      b1_active.store(true);
      wait_for(release_b1);
    }
  };
  result.run = run_pipeline(points, options);
  std::printf("run=%s status=%d stage=%s callbacks=%zu events=%llu peak_b=%llu digest=%s message=%s\n",
      name, (int)result.run.status, run_stage_name(result.run.stage_reached), result.callbacks,
      (unsigned long long)result.run.total_events, (unsigned long long)result.run.peak_fold_inflight,
      result.run.digest_all.c_str(), result.run.message.c_str());
  for (size_t k = 1; k < result.run.silent_stats.size(); ++k) {
    const auto& s = result.run.silent_stats[k];
    std::printf("silent=%s K=%zu added=%llu steps=%llu query_nodes=%llu supports=%llu\n", name,k,
        (unsigned long long)s.added_cofaces,(unsigned long long)s.chain_steps,
        (unsigned long long)s.query_nodes,(unsigned long long)s.meb_supports);
  }
  return result;
}
bool empty_payload(const RunResult& r) {
  return r.cards.empty() && r.digest_all.empty() && r.digest_forest.empty() &&
      r.digest_balls.empty() && r.digest_postprefilter.empty() && r.total_events == 0 &&
      r.forest_storage.empty() && r.sum_parents_by_k.empty();
}
}  // namespace

namespace mhgp7 {
void audit_census_arrays(size_t size_c, size_t cap_c, size_t size_s, size_t cap_s, size_t cap_b) {
  std::printf("census=%s C=%zu capC=%zu S=%zu capS=%zu capB=%zu logical_bytes=%zu capacity_bytes=%zu\n",
      label,size_c,cap_c,size_s,cap_s,cap_b,
      size_c*sizeof(BallCandidate)+size_s*(sizeof(Survivor)+sizeof(BallData)),
      cap_c*sizeof(BallCandidate)+cap_s*sizeof(Survivor)+cap_b*sizeof(BallData));
}
void audit_expansion_arrays(size_t k, size_t events, size_t shard_capacity, size_t output_capacity) {
  if (hold_b1 && k == 1) prior_capacity = output_capacity;
  if (hold_b1 && k == 2) {
    wait_for(b1_active);
    ++observed_overlap;
    std::printf("overlap=%s K=%zu previousB_capacity=%zu shard_size=%zu shard_capacity=%zu "
                "output_capacity=%zu capacity_bytes=%zu\n",label,k,prior_capacity,events,
                shard_capacity,output_capacity,
                (prior_capacity+shard_capacity+output_capacity)*sizeof(ForestEvent));
    release_b1.store(true);
  }
  std::printf("expansion=%s K=%zu events=%zu shard_capacity=%zu output_capacity=%zu capacity_bytes=%zu\n",
      label,k,events,shard_capacity,output_capacity,(shard_capacity+output_capacity)*sizeof(ForestEvent));
}
}  // namespace mhgp7

int main() {
  std::vector<InputPoint> two{{10,{0,0,0}},{20,{2,0,0}}};
  RunOptions opt; opt.smax=2; opt.fold_inflight=1;
  const auto reference=execute("two_reference",two,opt);
  check(reference.run.status==PipelineStatus::kCompleteRegular && reference.callbacks==1 &&
        reference.run.total_events==1,"nonempty two reference");
  opt.memory_budget_bytes=432;
  const auto census=execute("two_census_432",two,opt);
  check(census.run.status==PipelineStatus::kResourceExhausted && census.callbacks==0 &&
        census.run.stage_reached==kRunStagePrefiltre && empty_payload(census.run),"early census refusal");
  opt.memory_budget_bytes=1000;
  const auto f1=execute("two_f1_1000",two,opt);
  check(f1.run.status==PipelineStatus::kCompleteRegular &&
        f1.run.digest_all==reference.run.digest_all,"same object bounded f1");
  opt.fold_inflight=16;
  const auto f16=execute("two_f16_1000",two,opt);
  check(f16.run.status==PipelineStatus::kResourceExhausted && f16.callbacks==0 &&
        f16.run.stage_reached==kRunStageFold && empty_payload(f16.run),"one-order fold overprovision");
  opt.memory_budget_bytes=0;
  const auto wide=execute("two_f16_reference",two,opt);
  check(wide.run.status==PipelineStatus::kCompleteRegular && wide.run.digest_all==reference.run.digest_all &&
        wide.run.peak_fold_inflight==1,"same one-order object and actual concurrency");
  opt.fold_inflight=1;opt.memory_budget_bytes=607;
  const auto two_below=execute("two_clamped_607",two,opt);
  check(two_below.run.status==PipelineStatus::kResourceExhausted && two_below.callbacks==0 &&
        empty_payload(two_below.run),"clamped run still refuses lower census budget");
  opt.memory_budget_bytes=608;
  const auto two_equal=execute("two_clamped_608",two,opt);
  check(two_equal.run.status==PipelineStatus::kCompleteRegular &&
        two_equal.run.digest_all==reference.run.digest_all,"clamped two-point exact threshold");

  const auto uniform=make_family_input(CloudFamily::kUniform,10,65536,3);
  opt=RunOptions{};opt.smax=3;opt.fold_inflight=16;opt.complete_silent_incidence=true;
  const auto normalized=execute("uniform10_reference",uniform,opt);
  check(normalized.run.status==PipelineStatus::kCompleteRegular && normalized.callbacks==2 &&
        normalized.run.silent_stats[2].added_cofaces==0,"nonempty no-addition silent reference");
  opt.memory_budget_bytes=69984;
  const auto late=execute("uniform10_f16_69984",uniform,opt);
  check(late.run.status==PipelineStatus::kResourceExhausted && late.callbacks==1 &&
        late.run.silent_stats[2].added_cofaces==0 && late.run.silent_stats[2].query_nodes>0 &&
        empty_payload(late.run),"late predictable silent refusal remains transactional");
  opt.fold_inflight=1;
  const auto narrow=execute("uniform10_f1_69984",uniform,opt);
  check(narrow.run.status==PipelineStatus::kCompleteRegular &&
        narrow.run.digest_all==normalized.run.digest_all,"same normalized object at feasible concurrency");
  opt.fold_inflight=2;opt.memory_budget_bytes=27359;
  const auto below=execute("uniform10_clamped_27359",uniform,opt);
  check(below.run.status==PipelineStatus::kResourceExhausted && below.callbacks==0 &&
        empty_payload(below.run),"clamped run still refuses lower budget");
  opt.memory_budget_bytes=27360;
  const auto equal=execute("uniform10_clamped_27360",uniform,opt);
  check(equal.run.status==PipelineStatus::kCompleteRegular &&
        equal.run.digest_all==normalized.run.digest_all,"clamped normalized exact threshold");
  opt.memory_budget_bytes=0;opt.fold_inflight=16;
  hold_b1=true;
  const auto overlap=execute("uniform10_overlap",uniform,opt);
  hold_b1=false;
  check(overlap.run.status==PipelineStatus::kCompleteRegular &&
        overlap.run.digest_all==normalized.run.digest_all && observed_overlap==1,"forced array overlap preserves object");

  std::vector<InputPoint> cube;
  for (int x:{0,4})for(int y:{0,4})for(int z:{0,4})cube.push_back({(PointId)cube.size(),{x,y,z}});
  opt=RunOptions{};opt.fold_inflight=1;
  const auto capacities=execute("cube_retained_capacity",cube,opt);
  check(capacities.run.status==PipelineStatus::kCompleteRegular &&
        capacities.run.diag_candidates_capacity>capacities.run.expand.unique_balls,"post-RLE retained capacity witness");
  std::printf("budget_gate errors=%d overlap_witnesses=%zu\n",errors,observed_overlap);
  return errors==0?0:1;
}
