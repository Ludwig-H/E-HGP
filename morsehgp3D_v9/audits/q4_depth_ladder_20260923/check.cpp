// Audit-only complete-stream gate: q4 is accepted at nonzero depth while
// every q3 face of its four-point support fails at the same K.
// run.py copies the unmodified product rational gate into a temporary file,
// renaming only main, so this sidecar can reuse its independent oracle.
#include "wspd_q34_gate_renamed.cpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::array<Point3, 4> support{{{30,30,30},{30,10,10},{10,30,10},{10,10,30}}};
constexpr std::array<Point3, 7> core{{{20,20,20},{21,20,20},{20,21,20},{20,20,21},
                                       {19,20,20},{20,19,20},{20,20,19}}};

Points fixture(unsigned p, unsigned permutation) {
  Points points(support.begin(), support.end());
  for (const auto v : support) for (const int radius : {11, 12}) {
    points.push_back({static_cast<mhgp9::gen::Coordinate>(20-radius*((static_cast<int>(v.x)-20)/10)),
                      static_cast<mhgp9::gen::Coordinate>(20-radius*((static_cast<int>(v.y)-20)/10)),
                      static_cast<mhgp9::gen::Coordinate>(20-radius*((static_cast<int>(v.z)-20)/10))});
  }
  points.insert(points.end(), core.begin(), core.begin()+p);
  if (permutation == 1) std::reverse(points.begin(), points.end());
  return points;
}

void geometry(GlobalGate& gate, const Points& points, const Output& all, unsigned p) {
  const Coefficients target{Big(1), Big(-40), Big(-40), Big(-40), Big(900)};
  std::array<std::size_t, 4> ids{};
  std::size_t n = 0;
  for (std::size_t id=0; id<points.size(); ++id) {
    const auto equal=[&](Point3 v) { return v.x==points[id].x && v.y==points[id].y && v.z==points[id].z; };
    if (std::any_of(support.begin(), support.end(), equal)) ids[n++] = id;
  }
  gate.require(n == 4, "target support missing");
  const std::vector<std::size_t> wanted_support(ids.begin(), ids.end());
  for (std::size_t omitted=0; omitted<4; ++omitted) {
    Ids face{};
    std::size_t cursor=0;
    for (std::size_t j=0; j<4; ++j) if (j!=omitted) face[cursor++]=ids[j];
    const auto ball=oracle::make(select(points, face));
    gate.require(ball.ball.has_value(), "q3 face is not positive");
    gate.require(census(gate, points, *ball.ball, face).depth == p+2,
                 "q3 face is not exactly at rejection threshold");
  }
  unsigned present=0;
  for (const auto& item : eligible(all, p+3, 6)) if (item.key == target) {
    gate.require(item.arity == 4 && item.depth == p && item.support == wanted_support &&
                 item.shell == wanted_support, "target q4 oracle payload differs");
    ++present;
  }
  gate.require(present == 1, "target q4 oracle presentation absent or repeated");
  for (const auto& item : eligible(all, p+2, 6))
    gate.require(item.key != target, "target q4 admitted one K too early");
}

void compare_direct(GlobalGate& gate, const Points& points, const Output& wanted,
                    unsigned k, unsigned s, mhgp9::gen::WspdQ34Options opts,
                    unsigned workers) {
  const auto index=mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
  Output actual;
  if (workers==0) {
    static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,k,s,opts,
        [&](const auto& item) { actual.push_back(copy(item)); }));
  } else {
    std::vector<Output> slots(workers);
    const auto result=mhgp9::gen::run_wspd_q34_parallel(index,k,s,opts,workers,
        [&](std::size_t slot,const auto& item) { slots.at(slot).push_back(copy(item)); },1);
    gate.require(result.parallel.completed_jobs == result.parallel.jobs,
                 "parallel jobs incomplete");
    for (const auto& slot:slots) actual.insert(actual.end(),slot.begin(),slot.end());
  }
  normalize(actual);
  gate.require(actual == wanted, "complete q3/q4 stream differs from rational oracle");
}

void run(GlobalGate& gate, unsigned p, unsigned permutation) {
  const auto points=fixture(p, permutation);
  const auto all=global_oracle(gate, points);
  geometry(gate, points, all, p);
  const unsigned k=p+3;
  const auto wanted=eligible(all,k,6);
  auto local=options(mhgp9::gen::WspdFrontMode::MidpointSamples,
                     mhgp9::gen::WspdQ4Backend::Local28);
  local.local.max_depth=3;local.local.node_budget=85;
  local.local.z_test_budget=32;local.local.leaf_sites=0;
  auto optimized=local;
  optimized.local.saturate_deep=true;
  optimized.local.retain_q3_fragments=true;
  optimized.witness_mode=mhgp9::gen::WspdQ34WitnessMode::RectanglePair;
  optimized.q3_census_mode=mhgp9::gen::WspdQ3CensusMode::GlobalBoxes;
  optimized.witness_bounds_mode=mhgp9::gen::Q34WitnessBoundsMode::Affine;
  optimized.q4_seed_cells={mhgp9::gen::Q4SeedCellMode::LiveOnly,64};
  optimized.q3_atlas_consultation=true;
  optimized.q3_leaf_census=true;
  optimized.dead_lanes=true;optimized.dead_core=true;optimized.pair_witness_cache=true;
  for (unsigned s : {8U,10U,12U}) {
    const auto baseline=check_global(gate,points,all,k,s,local);
    gate.require(baseline==wanted,"baseline payload changed");
    compare_direct(gate,points,wanted,k,s,optimized,0);
    if (s==8 && p!=1) compare_direct(gate,points,wanted,k,s,optimized,1);
    compare_direct(gate,points,wanted,k,s,optimized,4);
  }
  auto window=options(mhgp9::gen::WspdFrontMode::MidpointSamples,
                      mhgp9::gen::WspdQ4Backend::Window30);
  static_cast<void>(check_global(gate,points,all,k,8,window));
  const auto before=eligible(all,k-1,6);
  compare_direct(gate,points,before,k-1,8,local,0);
  compare_direct(gate,points,before,k-1,8,optimized,0);
}
}

int main() {
  try {
    GlobalGate gate;
    for (const unsigned p : {1U,2U,7U}) for (unsigned permutation=0;permutation<2;++permutation)
      run(gate,p,permutation);
    std::printf("{\"status\":\"PASS\",\"clouds\":6,\"depths\":[1,2,7],\"k\":[4,5,10],\"complete_streams\":76,\"checks\":%llu,\"oracle_tetrahedra\":%llu,\"oracle_balls\":%llu}\n",
                static_cast<unsigned long long>(gate.checks),
                static_cast<unsigned long long>(gate.oracle_tetrahedra),
                static_cast<unsigned long long>(gate.oracle_balls));
    return 0;
  } catch(const std::exception& error) {
    std::fprintf(stderr,"FAIL: %s\n",error.what());
    return 1;
  }
}
