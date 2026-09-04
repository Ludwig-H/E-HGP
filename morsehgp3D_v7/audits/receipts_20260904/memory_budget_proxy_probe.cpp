#include <cstdio>
#include <string>
#include <vector>
#include "source/src/cloud/families.hpp"
#include "source/src/pipeline/run.hpp"

using namespace mhgp7;

RunResult run(const char* name, const std::vector<InputPoint>& in, RunOptions opt) {
  size_t callbacks = 0;
  opt.digest = true;
  opt.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++callbacks; };
  auto r = run_pipeline(in, opt);
  std::printf("scene=%s n=%zu inflight=%d silent=%d budget=%llu status=%d stage=%s "
              "raw=%zu capacity=%llu unique=%llu survivors=%llu callbacks=%zu "
              "total_events=%llu digest=%s message=%s\n", name, in.size(), opt.fold_inflight,
              opt.complete_silent_incidence, (unsigned long long)opt.memory_budget_bytes,
              (int)r.status, run_stage_name(r.stage_reached), r.emitted,
              (unsigned long long)r.diag_candidates_capacity,
              (unsigned long long)r.expand.unique_balls, (unsigned long long)r.expand.survivors,
              callbacks, (unsigned long long)r.total_events, r.digest_all.c_str(), r.message.c_str());
  for (size_t k=1;k<r.cards.size();++k)
    std::printf("K=%zu events=%llu added=%llu\n",k,(unsigned long long)r.cards[k].events,
                (unsigned long long)r.silent_stats[k].added_cofaces);
  for (size_t k=1;k<r.silent_stats.size();++k)
    if (r.status != PipelineStatus::kCompleteRegular)
      std::printf("refused_K=%zu added=%llu steps=%llu\n",k,
                  (unsigned long long)r.silent_stats[k].added_cofaces,
                  (unsigned long long)r.silent_stats[k].chain_steps);
  return r;
}

int main() {
  std::vector<InputPoint> two{{10,{0,0,0}},{20,{2,0,0}}};
  RunOptions opt; opt.smax=2; opt.fold_inflight=1;
  const auto full=run("two_unbudgeted",two,opt);
  opt.memory_budget_bytes=432;
  run("two_census_refusal",two,opt);
  opt.memory_budget_bytes=1000;
  run("two_inflight1",two,opt);
  opt.fold_inflight=16;
  run("two_inflight16",two,opt);
  opt.memory_budget_bytes=0;
  run("two_inflight16_unbudgeted",two,opt);
  if (full.status!=PipelineStatus::kCompleteRegular) return 1;
  for (int n : {5,10,20,50}) {
    const auto cloud=make_family_input(CloudFamily::kUniform,n,65536,3);
    RunOptions common;common.smax=3;common.fold_inflight=16;
    const auto base=run("uniform_base",cloud,common);
    if (base.status!=PipelineStatus::kCompleteRegular) continue;
    u64 events_max=0;
    for (const auto& card:base.cards) events_max=std::max(events_max,card.events);
    common.memory_budget_bytes=std::max({(u64)base.emitted*288,
        base.expand.unique_balls*608,events_max*144*18});
    run("uniform_budget",cloud,common);
    common.complete_silent_incidence=true;
    run("uniform_budget_silent",cloud,common);
  }
}
