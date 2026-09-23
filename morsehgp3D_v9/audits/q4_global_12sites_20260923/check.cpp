// Audit-only global completeness gate for a q4 key whose four q3 faces fail
// at K=3. The runner makes an unmodified temporary copy of the product gate
// with only its main symbol renamed; its rational oracle and full-payload
// comparison are reused here. No code in src/ or tests/ is modified.
#include "wspd_q34_gate_renamed.cpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {
Points fixture(unsigned permutation) {
  const std::array<Point3,4> support{{{30,30,30},{30,10,10},{10,30,10},{10,10,30}}};
  Points points(support.begin(),support.end());
  for(const auto p:support) for(const int radius:{11,12}) {
    const auto reflected=[&](int coordinate) {
      return static_cast<mhgp9::gen::Coordinate>(20-radius*((coordinate-20)/10));
    };
    points.push_back({reflected(p.x),reflected(p.y),reflected(p.z)});
  }
  if(permutation==1) std::reverse(points.begin(),points.end());
  return points;
}

void geometry(GlobalGate& gate,const Points& points,const Output& all) {
  const Coefficients target{Big(1),Big(-40),Big(-40),Big(-40),Big(900)};
  std::array<std::size_t,4> support{};
  std::size_t n=0;
  for(std::size_t id=0;id<points.size();++id) {
    const auto p=points[id];
    if((p.x==30 && p.y==30 && p.z==30) || (p.x==30 && p.y==10 && p.z==10) ||
       (p.x==10 && p.y==30 && p.z==10) || (p.x==10 && p.y==10 && p.z==30))
      support[n++]=id;
  }
  gate.require(n==4,"four support sites not found");
  std::sort(support.begin(),support.end());
  for(std::size_t omitted=0;omitted<4;++omitted) {
    Ids face{};std::size_t cursor=0;
    for(std::size_t j=0;j<4;++j) if(j!=omitted) face[cursor++]=support[j];
    const auto ball=oracle::make(select(points,face));
    gate.require(ball.ball.has_value(),"one q3 face ceased to be positive");
    gate.require(census(gate,points,*ball.ball,face).depth==2,
                 "one q3 face is no longer rejected at K3");
  }
  std::size_t matches=0;
  for(const auto& item:all) if(item.key==target)
    gate.require(item.arity==4,"target BallKey acquired a smaller support arity");
  for(const auto& item:eligible(all,3,6)) if(item.arity==4 && item.key==target) {
    gate.require(item.depth==0 && item.support==std::vector<std::size_t>(support.begin(),support.end()) &&
                 item.shell==std::vector<std::size_t>(support.begin(),support.end()),
                 "target q4 oracle payload differs");
    ++matches;
  }
  gate.require(matches==1,"target q4 key absent or repeated in rational oracle");
}

void parallel(GlobalGate& gate,const Points& points,const Output& wanted,
              unsigned s,mhgp9::gen::WspdQ34Options options,std::size_t workers) {
  const auto index=mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
  std::vector<Output> slots(workers);
  const auto result=mhgp9::gen::run_wspd_q34_parallel(index,3,s,options,workers,
      [&](std::size_t slot,const auto& item) {
        if(slot>=slots.size()) throw std::runtime_error("worker slot out of range");
        slots[slot].push_back(copy(item));
      },1);
  Output actual;
  for(auto& slot:slots) actual.insert(actual.end(),slot.begin(),slot.end());
  normalize(actual);
  gate.require(actual==wanted,"parallel full q34 stream differs from rational oracle");
  gate.require(result.pipeline.work.q4_emitted>0 && result.parallel.completed_jobs==result.parallel.jobs,
               "parallel q4 job completion ledger differs");
}

void run(GlobalGate& gate,const Points& points) {
  const auto all=global_oracle(gate,points);
  geometry(gate,points,all);
  const auto wanted=eligible(all,3,6);
  for(const unsigned s:{8U,10U,12U}) {
    for(const bool clip:{false,true}) for(const bool saturated:{false,true}) {
      auto options=::options(mhgp9::gen::WspdFrontMode::MidpointSamples,
                             mhgp9::gen::WspdQ4Backend::Local28);
      options.local.max_depth=3;
      options.local.node_budget=85;
      options.local.z_test_budget=32;
      options.local.leaf_sites=0;
      options.local.clip_events=clip;
      options.local.saturate_deep=saturated;
      const auto mono=check_global(gate,points,all,3,s,options);
      gate.require(mono==wanted,"local full q34 stream differs from rational oracle");
      parallel(gate,points,wanted,s,options,1);
      parallel(gate,points,wanted,s,options,4);
    }
    auto window=::options(mhgp9::gen::WspdFrontMode::MidpointSamples,
                          mhgp9::gen::WspdQ4Backend::Window30);
    const auto mono=check_global(gate,points,all,3,s,window);
    gate.require(mono==wanted,"window full q34 stream differs from rational oracle");
    parallel(gate,points,wanted,s,window,1);
    parallel(gate,points,wanted,s,window,4);

    // The same optional levers as the dense-key audit, on the complete
    // twelve-site stream. The product helper check_global assumes no pair
    // filtering, so this arm compares its payload directly to global_oracle.
    auto chain=::options(mhgp9::gen::WspdFrontMode::MidpointSamples,
                         mhgp9::gen::WspdQ4Backend::Local28);
    chain.local.max_depth=3;chain.local.node_budget=85;
    chain.local.z_test_budget=32;chain.local.leaf_sites=0;
    chain.local.saturate_deep=true;chain.local.retain_q3_fragments=true;
    chain.witness_mode=mhgp9::gen::WspdQ34WitnessMode::RectanglePair;
    chain.q3_census_mode=mhgp9::gen::WspdQ3CensusMode::GlobalBoxes;
    chain.witness_bounds_mode=mhgp9::gen::Q34WitnessBoundsMode::Affine;
    chain.q4_seed_cells={mhgp9::gen::Q4SeedCellMode::LiveOnly,64};
    chain.q3_atlas_consultation=true;chain.q3_leaf_census=true;
    chain.dead_lanes=true;chain.dead_core=true;chain.pair_witness_cache=true;
    const auto index=mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    Output actual;
    static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,3,s,chain,
        [&](const auto& item) {actual.push_back(copy(item));}));
    normalize(actual);
    gate.require(actual==wanted,"optimized full q34 stream differs from rational oracle");
    parallel(gate,points,wanted,s,chain,1);
    parallel(gate,points,wanted,s,chain,4);
  }
}
}

int main() {
  try {
    GlobalGate gate;
    run(gate,fixture(0));
    run(gate,fixture(1));
    std::printf("{\"status\":\"PASS\",\"sites\":12,\"permutations\":2,\"logical_configs\":36,\"complete_streams\":108,\"checks\":%llu,\"oracle_tetrahedra\":%llu,\"oracle_balls\":%llu}\n",
                static_cast<unsigned long long>(gate.checks),
                static_cast<unsigned long long>(gate.oracle_tetrahedra),
                static_cast<unsigned long long>(gate.oracle_balls));
    return 0;
  } catch(const std::exception& error) {
    std::fprintf(stderr,"FAIL: %s\n",error.what());
    return 1;
  }
}
