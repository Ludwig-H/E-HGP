// Explicit reuse of test-only rational Gram/census/ownership helpers. The
// previous gate is not run; no product or independent-audit model is copied.
#define main mhgp9_gen_pruning_helpers_for_global_q34_gate
#include "q34_family_pruning_gate.cpp"
#undef main

#include "pipeline/wspd_q34.hpp"

#include <cstdlib>
#include <bit>
#include <atomic>
#include <new>
#include <thread>
#include <tuple>

namespace global_q34_allocation {
struct State { bool armed{}; std::size_t remaining{}; };
thread_local State state;
[[gnu::noinline]] void* allocate(std::size_t size, std::size_t alignment=alignof(std::max_align_t)) {
  if (state.armed) {
    if (state.remaining==0) { state.armed=false; throw std::bad_alloc(); }
    --state.remaining;
  }
  void* result=nullptr;
  if (alignment<=alignof(std::max_align_t)) result=std::malloc(size==0?1:size);
  else if (posix_memalign(&result,alignment,size==0?1:size)!=0) result=nullptr;
  if (!result) throw std::bad_alloc();
  return result;
}
[[gnu::noinline]] void release(void* p) noexcept { std::free(p); }
}
void* operator new(std::size_t s) { return global_q34_allocation::allocate(s); }
void* operator new[](std::size_t s) { return global_q34_allocation::allocate(s); }
void* operator new(std::size_t s,std::align_val_t a) { return global_q34_allocation::allocate(s,static_cast<std::size_t>(a)); }
void* operator new[](std::size_t s,std::align_val_t a) { return global_q34_allocation::allocate(s,static_cast<std::size_t>(a)); }
void operator delete(void* p) noexcept { global_q34_allocation::release(p); }
void operator delete[](void* p) noexcept { global_q34_allocation::release(p); }
void operator delete(void* p,std::size_t) noexcept { global_q34_allocation::release(p); }
void operator delete[](void* p,std::size_t) noexcept { global_q34_allocation::release(p); }
void operator delete(void* p,std::align_val_t) noexcept { global_q34_allocation::release(p); }
void operator delete[](void* p,std::align_val_t) noexcept { global_q34_allocation::release(p); }
void operator delete(void* p,std::size_t,std::align_val_t) noexcept { global_q34_allocation::release(p); }
void operator delete[](void* p,std::size_t,std::align_val_t) noexcept { global_q34_allocation::release(p); }

namespace {
struct GlobalGate : Gate {
  u64 global_calls{},oracle_clouds{},oracle_triangles{},oracle_tetrahedra{};
  u64 oracle_balls{},positive_triangles{},positive_tetrahedra{},canonical_groups{};
  u64 front_replays{},expanded_pairs{},fat_rectangles{},q3_only_edges{},q4_only_edges{},both_edges{};
  u64 split_q3_only_edges{},split_q4_only_edges{};
  u64 q3_front_rejections{},q4_front_rejections{},xi_tests{},q3_depth_rejections{},q3_unread_sites{};
  u64 pure_calls{},sample_calls{},local_calls{},window_calls{},inactive_calls{},singleton_calls{};
  u64 s8_calls{},s10_calls{},s12_calls{},strict_w3_contacts{},strict_w4_contacts{};
  u64 independent_q2_cases{},independent_q3_cases{},isolated_cases{},extreme_calls{};
  u64 allocation_failures{},nested_calls{},owner_reset_calls{},input_alias_checks{};
  u64 parallel_pipeline_calls{},parallel_w1_calls{},parallel_w2_calls{},parallel_w4_calls{};
  u64 parallel_geometry_checks{},worker_ledger_checks{},callback_copy_checks{},multiworker_calls{};
  u64 parallel_callback_failures{},parallel_join_checks{},parallel_owner_resets{},parallel_empty_calls{};
  u64 indexed_calls{},indexed_pair_rejections{},indexed_rectangle_rejections{},boxed_calls{};
  u64 bounds_mode_calls{},bounds_exclusion_calls{},bounds_affine_calls{},bounds_work_checks{},bounds_parallel_calls{};
  u64 bounds_invalid_inputs{},bounds_inactive_calls{},bounds_callback_failures{},bounds_allocation_failures{};
  u64 bounds_shared_calls{},bounds_owner_resets{};
  u64 seed_cell_calls{},seed_cell_live_calls{},seed_cell_joined_calls{},seed_cell_work_checks{},seed_cell_parallel_calls{};
  u64 seed_cell_inactive_calls{},seed_cell_invalid_inputs{},seed_cell_callback_failures{},seed_cell_allocation_failures{};
  u64 seed_cell_shared_calls{},seed_cell_owner_resets{},seed_cell_parallel_failures{};
  u64 atlas_rejections{},atlas_lane_skips{},atlas_locations{},atlas_outside{};
  u64 task_sharing_calls{},task_ranges{},task_splits{},task_refusals{};
  // Internal floors for the 18-bit twins only (the emitted inventory is
  // frozen by bench/run_q34_indexed_checks.py): never emitted.
  u64 extreme18_calls{},extreme18_indexed_calls{},extreme18_atlas_locations{},extreme18_atlas_rejections{};
  u64 extreme18_bounds_calls{},deep_atlas_calls{};
  // v9 q3 leaf census (retained exact fragments of the q4 atlas).
  u64 leaf_calls{},leaf_censuses{},leaf_rejections{},leaf_point_tests{},leaf_fixture_censuses{};
};

// Every component is a standard-layout aggregate containing only u64 fields
// and the explicitly checked aggregates below. Exact size sums prove there
// are no padding bytes; bit_cast then compares ALL counter values, including
// future additions (which must first update these inventory assertions).
// This is not a memcmp of an aggregate with unspecified padding.
#define WORDS(type,count) static_assert(std::is_trivially_copyable_v<mhgp9::gen::type> && \
  std::is_standard_layout_v<mhgp9::gen::type> && sizeof(mhgp9::gen::type)==(count)*sizeof(u64))
static_assert(sizeof(u64)==8 && std::numeric_limits<u64>::digits==64);
WORDS(Q34EdgeCoverWork,10);WORDS(WspdQ3Work,23);
WORDS(Q4PositiveDomainWork,12);WORDS(Q4LocalGeometryQueryWork,2);
WORDS(Q4LocalGeometryWork,27);WORDS(Q4LocalPartitionWork,20);
WORDS(Q4LocalAtlasWork,38);WORDS(Q4LocalSweepWork,41);WORDS(Q4LocalEdgeWork,117);
WORDS(Q4ShallowSetWork,25);WORDS(Q4FamilyWork,13);WORDS(Q4ShallowSweepWork,32);
WORDS(Q4WindowSelectionWork,25);WORDS(Q4WindowSweepWork,57);WORDS(Q4WindowEdgeWork,116);
WORDS(Q34WitnessSearchWork,25);WORDS(Q34WitnessBoundsWork,12);WORDS(WspdQ34WitnessWork,82);
WORDS(Q3BallCensusWork,26);WORDS(Q4SeedCellWork,37);WORDS(WspdQ3AtlasWork,9);WORDS(WspdQ34Work,433);
#undef WORDS
std::array<u64,433> logical_work(mhgp9::gen::WspdQ34Work work) {
  // These two capacity peaks depend on the private buffer's previous jobs;
  // they are paid separately, not erased from the published result.
  work.q3.peak_shell_bytes=0;
  work.peak_edge_buffer_bytes=0;
  return std::bit_cast<std::array<u64,433>>(work);
}

// Enumerate every small-cloud support once. An independent Gaussian rational
// solve decides positivity, centre and radius; the global census is cached by
// primitive rational key. q4 grouping matches the published stream contract,
// not a globally deduplicated catalogue and not all support incidences.
Output global_oracle(GlobalGate& gate,const Points& points) {
  ++gate.oracle_clouds;
  std::map<Ids,bool> acute;
  std::map<Coefficients,Candidate> balls;
  auto payload=[&](const oracle::Ball& ball,std::span<const std::size_t> ids) {
    auto position=balls.find(ball.coefficients);
    if (position==balls.end()) {
      position=balls.emplace(ball.coefficients,census(gate,points,ball,ids)).first;
      ++gate.oracle_balls;
    }
    auto result=position->second;
    result.arity=static_cast<unsigned>(ids.size());
    result.support.assign(ids.begin(),ids.end());
    return result;
  };
  Output result;
  const auto n=points.size();
  for (std::size_t a=0;a<n;++a) for (std::size_t b=a+1;b<n;++b)
    for (std::size_t c=b+1;c<n;++c) {
      const Ids ids{a,b,c};
      const auto solved=oracle::make(select(points,ids));
      ++gate.oracle_triangles;
      acute.emplace(ids,solved.ball.has_value());
      if (!solved.ball) continue;
      ++gate.positive_triangles;
      result.push_back(payload(*solved.ball,ids));
    }
  using RootKey=std::tuple<Edge,std::size_t,Coefficients>;
  std::map<RootKey,std::pair<std::size_t,Candidate>> roots;
  for (std::size_t a=0;a<n;++a) for (std::size_t b=a+1;b<n;++b)
    for (std::size_t c=b+1;c<n;++c) for (std::size_t d=c+1;d<n;++d) {
      const std::array<std::size_t,4> ids{a,b,c,d};
      const auto solved=oracle::make(select(points,ids));
      ++gate.oracle_tetrahedra;
      if (!solved.ball) continue;
      ++gate.positive_tetrahedra;
      const auto edge=owner(points,ids);
      auto seed=n;
      for (const auto x:ids) {
        if (x==edge[0] || x==edge[1]) continue;
        Ids face{edge[0],edge[1],x};
        std::sort(face.begin(),face.end());
        if (acute.at(face)) seed=std::min(seed,x);
      }
      gate.require(seed<n,"positive tetrahedron has no acute face on its owning edge");
      auto completion=n;
      for (const auto id:ids) if (id!=edge[0] && id!=edge[1] && id!=seed) completion=id;
      RootKey key{edge,seed,solved.ball->coefficients};
      auto candidate=payload(*solved.ball,ids);
      const auto position=roots.find(key);
      if (position==roots.end()) roots.emplace(std::move(key),std::make_pair(completion,std::move(candidate)));
      else if (completion<position->second.first) position->second={completion,std::move(candidate)};
    }
  gate.canonical_groups+=static_cast<u64>(roots.size());
  for (auto& [key,value]:roots) { static_cast<void>(key);result.push_back(std::move(value.second)); }
  normalize(result);
  return result;
}

Output eligible(const Output& all,unsigned k,std::uint8_t mask) {
  Output result;
  for (const auto& value:all) {
    const auto bit=static_cast<std::uint8_t>(value.arity==3?2:4);
    if ((mask&bit)!=0 && k+2>value.arity && value.depth<k+2-value.arity) result.push_back(value);
  }
  return result;
}

mhgp9::gen::WspdQ34Options options(mhgp9::gen::WspdFrontMode mode,mhgp9::gen::WspdQ4Backend backend,std::uint8_t mask=6) {
  mhgp9::gen::WspdQ34Options result;
  result.front_mode=mode;result.q4_backend=backend;result.requested_lane_mask=mask;
  result.local.max_depth=0;result.local.node_budget=1;result.local.z_test_budget=0;
  result.local.leaf_sites=0;
  return result;
}

Output check_global(GlobalGate& gate,const Points& points,const Output& all,
                    unsigned k,unsigned s,mhgp9::gen::WspdQ34Options opts) {
  const auto index=mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
  Output actual;
  const auto result=mhgp9::gen::run_wspd_q34_candidates(index,k,s,opts,[&](const auto& value) {
    actual.push_back(copy(value));
  });
  normalize(actual);
  const auto expected=eligible(all,k,opts.requested_lane_mask);
  gate.require(actual==expected,"global q34 stream differs from independent rational support/depth/shell oracle");
  ++gate.global_calls;
  if (opts.front_mode==mhgp9::gen::WspdFrontMode::Pure) ++gate.pure_calls;else ++gate.sample_calls;
  if (opts.q4_backend==mhgp9::gen::WspdQ4Backend::Local28) ++gate.local_calls;else ++gate.window_calls;
  if (s==8) ++gate.s8_calls;
  if (s==10) ++gate.s10_calls;
  if (s==12) ++gate.s12_calls;
  if (points.size()==1) ++gate.singleton_calls;
  u64 q3=0,q4=0,shell=0;
  for (const auto& value:actual) {
    if (value.arity==3) ++q3;else ++q4;
    shell+=static_cast<u64>(value.shell.size());
    gate.max_shell=std::max(gate.max_shell,static_cast<u64>(value.shell.size()));
  }
  gate.candidates+=q3+q4;gate.q3+=q3;gate.q4+=q4;
  const auto& work=result.work;
  gate.require(work.q3_emitted==q3 && work.q4_emitted==q4 && work.payload_shell_ids==shell,
               "global output/payload ledger mismatch");
  gate.require(result.front.total_unordered_pairs==points.size()*(points.size()-1)/2,
               "global pair metadata mismatch");
  const std::uint8_t active=static_cast<std::uint8_t>(opts.requested_lane_mask&(k>=3?6:k==2?2:0));
  gate.require(result.front.active_lane_mask==active,"global active lane mask mismatch");
  if (active==0) {
    ++gate.inactive_calls;
    gate.require(result.front.work==mhgp9::gen::WspdFrontWork{} && work.expanded_pairs==0 &&
      work.input_rectangles==0 && work.cover_builds==0 && work.peak_edge_buffer_bytes==0 &&
      work.q3==mhgp9::gen::WspdQ3Work{} && work.local.atlas.cells_created==0 &&
      work.window.selection.input_sites==0,"inactive global call performed hidden work");
    return actual;
  }
  std::map<Edge,std::uint8_t> residual;
  const auto front=mhgp9::gen::run_wspd_front(*index,k,s,opts.front_mode,[&](const auto& rectangle) {
    const auto a=index->spatial_nodes()[rectangle.a_node].range;
    const auto b=index->spatial_nodes()[rectangle.b_node].range;
    if (a.size()>1 || b.size()>1) ++gate.fat_rectangles;
    for (auto ai=a.first;ai<a.last;++ai) for (auto bi=b.first;bi<b.last;++bi) {
      auto x=index->spatial_order()[ai],y=index->spatial_order()[bi];
      if (y<x) std::swap(x,y);
      gate.require(residual.emplace(Edge{x,y},rectangle.lane_mask).second,
                   "front residual rectangles duplicate an unordered edge");
    }
  },opts.requested_lane_mask);
  ++gate.front_replays;
  gate.require(result.front.work==front.work,"global wrapper changed the independently replayed front");
  u64 edges3=0,edges4=0,both=0,covered_sites=0,max_cover=0;
  for (const auto& [edge,mask]:residual) {
    edges3+=static_cast<u64>((mask&2)!=0);edges4+=static_cast<u64>((mask&4)!=0);
    both+=static_cast<u64>(mask==6);
    if (mask==2) ++gate.q3_only_edges;
    if (mask==4) ++gate.q4_only_edges;
    if (mask==6) ++gate.both_edges;
    if (opts.requested_lane_mask==6 && k>=3) {
      if (mask==2) ++gate.split_q3_only_edges;
      if (mask==4) ++gate.split_q4_only_edges;
    }
    const Big length=distance_squared(points[edge[0]],points[edge[1]]);
    u64 size=0;
    for (const auto z:points) {
      Big square=0;
      for (std::size_t axis=0;axis<3;++axis) {
        const Big delta=2*Big(z[axis])-Big(points[edge[0]][axis])-Big(points[edge[1]][axis]);
        square+=delta*delta;
      }
      size+=static_cast<u64>(square<=4*length);
    }
    covered_sites+=size;max_cover=std::max(max_cover,size);
  }
  gate.require(work.input_rectangles==front.work.emitted_rectangles && work.expanded_pairs==residual.size() &&
    work.q3_edges==edges3 && work.q4_edges==edges4 && work.both_edges==both &&
    work.cover_builds==residual.size() && work.cover_sites==covered_sites && work.max_cover_sites==max_cover,
    "global edge expansion/shared cover ledger mismatch");
  gate.require(front.work.residual_pair_mass[0]==0 && front.work.rejected_pair_mass[0]==0 &&
    front.work.residual_pair_mass[1]==edges3 && front.work.residual_pair_mass[2]==edges4,
    "global lane ledger includes hidden q2 or loses q3/q4 pairs");
  for (const auto& value:expected) {
    const auto edge=owner(points,value.support);
    const auto position=residual.find(edge);
    gate.require(position!=residual.end() && (position->second&(value.arity==3?2:4))!=0,
                 "front rejected the owning edge of a valid positive support");
  }
  gate.require(work.q3.edge_queries==edges3 && work.q3.emitted==q3 &&
    work.q3.census_point_tests==work.q3.census_inside_sites+work.q3.census_outside_sites+work.q3.census_shell_sites,
    "q3-only census ledger mismatch");
  gate.require(work.q3.seed_node_visits==work.q3.seed_bound_tests+work.q3.seed_point_tests &&
    work.q3.seed_bound_tests==work.q3.seed_rejected_nodes+work.q3.seed_split_nodes &&
    work.q3.seed_rejected_sites+work.q3.seed_point_tests==points.size()*edges3 &&
    work.q3.acute_seeds==work.q3.owner_rejections+work.q3.seeds &&
    work.q3.seeds==work.q3.ball_builds && work.q3.seeds==work.q3.depth_rejections+work.q3.emitted,
    "q3-only seed tree or rejection partition mismatch");
  if (edges3==0) gate.require(work.q3==mhgp9::gen::WspdQ3Work{},"q4-only mask performed hidden q3 work");
  if (opts.q4_backend==mhgp9::gen::WspdQ4Backend::Local28) {
    gate.require(work.window.selection.input_sites==0 && work.window.sweep.window.seed_queries==0 &&
      work.window.peak_live_buffer_bytes==0 && work.local.sweep.emitted==q4,
      "local global call performed hidden window work or miscounted outputs");
  } else {
    gate.require(work.local.atlas.cells_created==0 && work.local.sweep.seed_queries==0 &&
      work.local.peak_live_buffer_bytes==0 && work.window.sweep.sweep.emitted==q4,
      "window global call performed hidden local work or miscounted outputs");
  }
  if (edges4==0) gate.require(work.local.atlas.cells_created==0 && work.window.selection.input_sites==0,
                            "q3-only mask performed hidden q4 preparation");
  gate.expanded_pairs+=work.expanded_pairs;gate.q3_front_rejections+=front.work.rejected_pair_mass[1];
  gate.q4_front_rejections+=front.work.rejected_pair_mass[2];gate.xi_tests+=front.work.xi_bound_tests;
  gate.q3_depth_rejections+=work.q3.depth_rejections;gate.q3_unread_sites+=work.q3.early_unread_sites;
  return actual;
}

void run_fixture(GlobalGate& gate,const Points& points,unsigned k,unsigned s=8) {
  const auto all=global_oracle(gate,points);
  for (const auto mode:{mhgp9::gen::WspdFrontMode::Pure,mhgp9::gen::WspdFrontMode::MidpointSamples})
    for (const auto backend:{mhgp9::gen::WspdQ4Backend::Local28,mhgp9::gen::WspdQ4Backend::Window30})
      static_cast<void>(check_global(gate,points,all,k,s,options(mode,backend)));
}

void global_fixtures(GlobalGate& gate) {
  const Points regular{{10,10,10},{16,16,10},{16,10,16},{10,16,16},{13,13,13},{14,12,13}};
  const auto all=global_oracle(gate,regular);
  for (const auto k:{1U,2U,3U,5U,10U}) for (const auto s:{8U,10U,12U})
    for (const auto mask:{std::uint8_t{2},std::uint8_t{4},std::uint8_t{6}})
      for (const auto backend:{mhgp9::gen::WspdQ4Backend::Local28,mhgp9::gen::WspdQ4Backend::Window30})
        static_cast<void>(check_global(gate,regular,all,k,s,options(mhgp9::gen::WspdFrontMode::MidpointSamples,backend,mask)));
  mhgp9::gen::WspdQ34Options defaults;
  static_cast<void>(check_global(gate,regular,all,5,8,defaults));
  auto local_variant=defaults;
  local_variant.local.domain=mhgp9::gen::Q4CenterDomainMode::Disk;
  local_variant.local.max_depth=2;local_variant.local.node_budget=21;
  local_variant.local.clip_events=false;
  static_cast<void>(check_global(gate,regular,all,5,8,local_variant));
  run_fixture(gate,Points{{1,2,3}},3);
  run_fixture(gate,Points{{0,0,0},{10,10,10}},5);
  run_fixture(gate,Points{{0,0,0},{2,0,0},{1,0,0},{4,0,0},{3,0,0}},5);
  run_fixture(gate,Points{{0,0,0},{4,0,0},{0,4,0},{4,4,0},{2,2,0},{1,3,0}},3);
  // Two compact distant factors force genuine non-singleton rectangles.
  run_fixture(gate,Points{{10,10,10},{11,12,10},{12,10,12},{10,12,12},
                         {60000,60000,60000},{60001,60002,60000},{60002,60000,60002},{60000,60002,60002}},5);
  const Points w3{{10,10,10},{16,16,10},{16,10,16},{12,14,8}};
  const Points w4{{10,10,10},{16,16,10},{16,10,16},{10,16,16},{12,12,8},{14,14,8}};
  auto contact=[&](const Points& points,std::size_t id,unsigned alpha) {
    const auto za=oracle::difference(points[id],points[0]);
    const auto bz=oracle::difference(points[1],points[id]);
    const Big h=oracle::dot(za,bz);
    const oracle::Vector cross{za[1]*bz[2]-za[2]*bz[1],za[2]*bz[0]-za[0]*bz[2],za[0]*bz[1]-za[1]*bz[0]};
    gate.require(h>0 && Big(alpha)*h*h==oracle::dot(cross,cross),"strict WSPD contact fixture is not exact");
    if (alpha==3) ++gate.strict_w3_contacts;else ++gate.strict_w4_contacts;
  };
  contact(w3,3,3);contact(w4,4,2);contact(w4,5,2);
  const auto circle=oracle::make(select(w3,Ids{0,1,2}));
  const auto sphere=oracle::make(select(w4,std::array<std::size_t,4>{0,1,2,3}));
  gate.require(circle.ball && circle.ball->power(w3[3]).numerator()==0 && sphere.ball &&
    sphere.ball->power(w4[4]).numerator()==0 && sphere.ball->power(w4[5]).numerator()==0,
    "strict front contacts do not lie on the positive support shell");
  run_fixture(gate,w3,2);run_fixture(gate,w4,3);
  run_fixture(gate,Points{{10,10,10},{16,16,10},{16,10,16},{13,13,10}},3);
  for (const auto k:{5U,10U}) {
    Points q3_points{{900,1000,1000},{1100,1000,1000},{1000,1120,1000}};
    Points q4_points{{900,1000,1000},{1100,1000,1000},{1000,1120,1040},{1000,1120,960}};
    for (unsigned j=0;j<k;++j) {
      const Point3 z{static_cast<mhgp9::gen::Coordinate>(1000+j),910,1000};
      q3_points.push_back(z);q4_points.push_back(z);
    }
    const auto diameter=oracle::make(select(q3_points,Edge{0,1}));
    const auto face=oracle::make(select(q3_points,Ids{0,1,2}));
    const auto tetra=oracle::make(select(q4_points,std::array<std::size_t,4>{0,1,2,3}));
    gate.require(diameter.ball && face.ball && tetra.ball,"q2-independence fixtures lost strict positivity");
    gate.require(census(gate,q3_points,*diameter.ball,Edge{0,1}).depth==k &&
      census(gate,q3_points,*face.ball,Ids{0,1,2}).depth==0 &&
      census(gate,q4_points,*tetra.ball,std::array<std::size_t,4>{0,1,2,3}).depth==0,
      "q2 rejection does not coexist with accepted q3/q4 fixtures");
    run_fixture(gate,q3_points,k);run_fixture(gate,q4_points,k);gate.independent_q2_cases+=2;
    q4_points.resize(4);
    for (unsigned j=0;j<k-1;++j) q4_points.push_back({static_cast<mhgp9::gen::Coordinate>(1000+j),1020,1105});
    const auto rejected_face=oracle::make(select(q4_points,Ids{0,1,2}));
    gate.require(rejected_face.ball && census(gate,q4_points,*rejected_face.ball,Ids{0,1,2}).depth==k-1 &&
      census(gate,q4_points,*tetra.ball,std::array<std::size_t,4>{0,1,2,3}).depth==0,
      "q3 rejection does not coexist with accepted q4 fixture");
    run_fixture(gate,q4_points,k);++gate.independent_q3_cases;
  }
  const Points isolated{{30,30,30},{36,36,30},{30,36,24},{36,30,24},
                        {30,36,30},{36,30,30},{30,30,24},{32,32,32}};
  run_fixture(gate,isolated,3);++gate.isolated_cases;
  const Points extreme{{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535},
                       {32767,32767,32767},{65535,0,0}};
  run_fixture(gate,extreme,5);gate.extreme_calls+=4;
  // 18-bit twin (u16 corners are interior points since the widening): same K,
  // same four front/backend combinations, then the Local28 atlas at its exact
  // depth ceiling (max_depth=20 accepted, refined to leaves) on the same oracle.
  const Points extreme18{{0,0,0},{262143,262143,0},{262143,0,262143},{0,262143,262143},
                         {131071,131071,131071},{262143,0,0}};
  const auto extreme18_all=global_oracle(gate,extreme18);
  for (const auto mode:{mhgp9::gen::WspdFrontMode::Pure,mhgp9::gen::WspdFrontMode::MidpointSamples})
    for (const auto backend:{mhgp9::gen::WspdQ4Backend::Local28,mhgp9::gen::WspdQ4Backend::Window30})
      static_cast<void>(check_global(gate,extreme18,extreme18_all,5,8,options(mode,backend)));
  gate.extreme_calls+=4;gate.extreme18_calls+=4;
  auto deep_local=defaults;
  deep_local.q4_backend=mhgp9::gen::WspdQ4Backend::Local28;
  deep_local.local.max_depth=mhgp9::gen::Q4LocalCell::max_depth;deep_local.local.node_budget=85;
  deep_local.local.leaf_sites=0;
  static_cast<void>(check_global(gate,extreme18,extreme18_all,5,8,deep_local));
  ++gate.extreme18_calls;++gate.deep_atlas_calls;
  auto shuffled=w4;
  std::reverse(shuffled.begin(),shuffled.end());run_fixture(gate,shuffled,3,10);++gate.permutations;
  for (auto& p:shuffled) p={p[2],p[0],p[1]};
  run_fixture(gate,shuffled,3,12);++gate.permutations;
  const auto large=shell30();
  const auto large_oracle=global_oracle(gate,large);
  for (const auto backend:{mhgp9::gen::WspdQ4Backend::Local28,mhgp9::gen::WspdQ4Backend::Window30})
    static_cast<void>(check_global(gate,large,large_oracle,3,8,options(mhgp9::gen::WspdFrontMode::MidpointSamples,backend)));
}

void global_lifecycle(GlobalGate& gate) {
  const Points points{{10,10,10},{16,16,10},{16,10,16},{10,16,16},{12,12,8},{14,14,8}};
  const auto expected=eligible(global_oracle(gate,points),5,6);
  auto source=points;
  auto cloud=mhgp9::gen::prepare_cloud(source);
  auto index=mhgp9::gen::make_q2_cloud_index(cloud);
  const auto opts=options(mhgp9::gen::WspdFrontMode::MidpointSamples,mhgp9::gen::WspdQ4Backend::Window30);
  const mhgp9::gen::Q34SeedConsumer discard=[](const auto&) {};
  auto call=[&](mhgp9::gen::Q2CensusIndexPtr owner_index,unsigned k,unsigned s,mhgp9::gen::WspdQ34Options config,
                const mhgp9::gen::Q34SeedConsumer& consumer) {
    return mhgp9::gen::run_wspd_q34_candidates(std::move(owner_index),k,s,config,consumer);
  };
  gate.rejects([&] { static_cast<void>(call({},5,8,opts,discard)); },"null index accepted");
  gate.rejects([&] { static_cast<void>(call(index,5,8,opts,{})); },"empty callback accepted");
  for (const auto k:{0U,11U}) gate.rejects([&] { static_cast<void>(call(index,k,8,opts,discard)); },"invalid K accepted");
  gate.rejects([&] { static_cast<void>(call(index,1,0,opts,discard)); },"inactive invalid separation accepted");
  for (const auto mask:{std::uint8_t{0},std::uint8_t{1},std::uint8_t{3},std::uint8_t{7}}) {
    auto invalid=opts;invalid.requested_lane_mask=mask;
    gate.rejects([&] { static_cast<void>(call(index,1,8,invalid,discard)); },"inactive invalid mask accepted");
  }
  for (unsigned which=0;which<6;++which) {
    auto invalid=opts;
    if (which==0) invalid.front_mode=static_cast<mhgp9::gen::WspdFrontMode>(42);
    if (which==1) invalid.q4_backend=static_cast<mhgp9::gen::WspdQ4Backend>(42);
    if (which==2) invalid.local.domain=static_cast<mhgp9::gen::Q4CenterDomainMode>(42);
    if (which==3) invalid.local.max_depth=45;
    if (which==4) invalid.local.node_budget=0;
    if (which==5) invalid.local.max_depth=mhgp9::gen::Q4LocalCell::max_depth+1;  // First refused depth (21).
    gate.rejects([&] { static_cast<void>(call(index,1,8,invalid,discard)); },"inactive invalid option accepted");
  }
  {
    // The exact depth ceiling itself (20) is accepted on an active Local28
    // call and leaves the full payload equal to the oracle.
    auto accepted=opts;accepted.q4_backend=mhgp9::gen::WspdQ4Backend::Local28;
    accepted.local.max_depth=mhgp9::gen::Q4LocalCell::max_depth;accepted.local.node_budget=85;
    accepted.local.z_test_budget=512;
    Output deep;
    static_cast<void>(call(index,5,8,accepted,[&](const auto& value) { deep.push_back(copy(value)); }));
    normalize(deep);
    gate.require(deep==expected,"Local28 atlas at its exact depth ceiling changed the global stream");
    ++gate.deep_atlas_calls;
  }
  bool threw=false;
  try { static_cast<void>(call(index,5,8,opts,[&](const auto&) { ++gate.callback_failures;throw std::logic_error("callback marker"); })); }
  catch (const std::logic_error& error) { threw=std::string_view(error.what())=="callback marker"; }
  gate.require(threw && gate.callback_failures==1,"callback failure was swallowed or callback repeated");
  for (std::size_t before=0;before<4;++before) {
    bool failed=false;
    global_q34_allocation::state={true,before};
    try { static_cast<void>(call(index,5,8,opts,discard)); }
    catch (const std::bad_alloc&) { failed=true; }
    global_q34_allocation::state.armed=false;
    gate.require(failed,"allocation injection did not reach global pipeline");++gate.allocation_failures;
  }
  std::array<std::future<Output>,4> jobs;
  for (std::size_t i=0;i<jobs.size();++i) jobs[i]=std::async(std::launch::async,[index,opts,i] {
    auto config=opts;if (i%2==0) config.q4_backend=mhgp9::gen::WspdQ4Backend::Local28;
    Output result;
    static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,5,8,config,[&](const auto& value) { result.push_back(copy(value)); }));
    normalize(result);return result;
  });
  for (auto& job:jobs) { gate.require(job.get()==expected,"concurrent call changed global output");++gate.parallel_calls; }
  Output outer,inner;
  bool entered=false;
  static_cast<void>(call(index,5,8,opts,[&](const auto& value) {
    outer.push_back(copy(value));
    if (!entered) {
      entered=true;
      static_cast<void>(call(index,5,8,opts,[&](const auto& nested) { inner.push_back(copy(nested)); }));
      ++gate.nested_calls;
    }
  }));
  normalize(inner);normalize(outer);
  gate.require(inner==expected && outer==expected,"nested pipeline call shared mutable state");
  source.assign(source.size(),Point3{0,0,0});
  Output output;
  static_cast<void>(call(index,5,8,opts,[&](const auto& value) {
    output.push_back(copy(value));
    if (index) { index.reset();cloud.reset();++gate.owner_reset_calls; }
  }));
  normalize(output);
  gate.require(output==expected && !index && !cloud,"input alias mutation or callback owner reset changed output");
  ++gate.input_alias_checks;
}

void parallel_case(GlobalGate& gate,const Points& points,const Output& all,unsigned k,
                   mhgp9::gen::WspdQ34Options opts,std::size_t worker_count,std::size_t grain) {
  // Tiny fixtures never reach the default task grain: four-worker cases use a
  // two-pair grain and a one-task queue so that range splitting AND the
  // full-queue inline fallback are both exercised against the oracle.
  if (worker_count==4) {opts.parallel_task_pairs=2;opts.parallel_queue_capacity=1;}
  const auto index=mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
  const auto expected=eligible(all,k,opts.requested_lane_mask);
  Output mono_output;
  const auto mono=mhgp9::gen::run_wspd_q34_candidates(index,k,8,opts,[&](const auto& value) {
    mono_output.push_back(copy(value));
  });
  normalize(mono_output);
  gate.require(mono_output==expected,"parallel baseline differs from rational global oracle");
  std::vector<Output> slots(worker_count);
  std::vector<u64> slot_calls(worker_count),slot_q3(worker_count),slot_q4(worker_count);
  const mhgp9::gen::WspdQ34ParallelConsumer callback=[&,private_calls=u64{0}](std::size_t slot,const auto& value) mutable {
    if (slot>=worker_count) throw std::runtime_error("parallel callback received out-of-range slot");
    ++private_calls;
    if (private_calls!=slot_calls[slot]+1) throw std::runtime_error("parallel slots share mutable callback target");
    slot_calls[slot]=private_calls;
    if (value.arity==3) ++slot_q3[slot];else ++slot_q4[slot];
    slots[slot].push_back(copy(value));
  };
  const auto result=mhgp9::gen::run_wspd_q34_parallel(index,k,8,opts,worker_count,callback,grain);
  Output output;
  for (const auto& slot:slots) output.insert(output.end(),slot.begin(),slot.end());
  normalize(output);
  gate.require(output==expected,"parallel global stream differs from independent rational support/depth/shell oracle");
  gate.require(result.pipeline.front.total_unordered_pairs==mono.front.total_unordered_pairs &&
    result.pipeline.front.active_lane_mask==mono.front.active_lane_mask &&
    result.pipeline.front.work==mono.front.work && logical_work(result.pipeline.work)==logical_work(mono.work),
    "parallel global logical counters differ from every mono counter");
  ++gate.parallel_geometry_checks;
  ++gate.parallel_pipeline_calls;
  if (worker_count==1) ++gate.parallel_w1_calls;
  if (worker_count==2) ++gate.parallel_w2_calls;
  if (worker_count==4) ++gate.parallel_w4_calls;
  const auto& p=result.parallel;
  gate.require(p.requested_workers==worker_count && p.started_workers==result.workers.size() &&
    p.started_workers==std::min<u64>(worker_count,p.jobs) && p.completed_jobs==p.jobs,
    "parallel worker/job completion ledger mismatch");
  if (mono.front.active_lane_mask==0) {
    ++gate.parallel_empty_calls;
    gate.require(result.workers.empty() && p.jobs==0 && p.job_storage_bytes==0 &&
      p.worker_state_bytes==0 && p.callback_storage_bytes==0 && p.edge_buffer_bytes_sum==0,
      "inactive parallel call prepared hidden engines or jobs");
    return;
  }
  u64 jobs=0,products=0,rectangles=0,pairs=0,q3=0,q4=0,capacity_sum=0,capacity_max=0,used=0;
  for (std::size_t slot=0;slot<result.workers.size();++slot) {
    const auto& worker=result.workers[slot];
    jobs+=worker.jobs;products+=worker.front_products;rectangles+=worker.input_rectangles;
    pairs+=worker.expanded_pairs;q3+=worker.q3_emitted;q4+=worker.q4_emitted;
    capacity_sum+=worker.peak_edge_buffer_bytes;
    capacity_max=std::max(capacity_max,worker.peak_edge_buffer_bytes);
    used+=static_cast<u64>(worker.jobs!=0);
    gate.require(worker.q3_emitted==slot_q3[slot] && worker.q4_emitted==slot_q4[slot] &&
      worker.q3_emitted+worker.q4_emitted==slot_calls[slot],"private worker payload ledger mismatch");
    ++gate.worker_ledger_checks;gate.callback_copy_checks+=slot_calls[slot];
  }
  if (used>1) ++gate.multiworker_calls;
  gate.require(jobs==p.jobs && products+p.prefix_product_visits==mono.front.work.product_visits &&
    rectangles==mono.work.input_rectangles && pairs==mono.work.expanded_pairs &&
    q3==mono.work.q3_emitted && q4==mono.work.q4_emitted &&
    capacity_sum==p.edge_buffer_bytes_sum && capacity_max==result.pipeline.work.peak_edge_buffer_bytes,
    "parallel sums of private worker work do not reconstruct the pipeline");
  gate.require(p.target_jobs==worker_count*grain && p.terminal_jobs<=p.jobs &&
    (p.started_workers==0 || (p.worker_state_bytes>0 && p.callback_storage_bytes>0)),
    "parallel orchestration storage/target ledger mismatch");
  // Rectangle-range task sharing: every published range is consumed exactly
  // once by a started slot; a single started worker never shares.
  const auto& t=result.tasks;
  u64 consumed=0;
  for (const auto count:result.worker_tasks) consumed+=count;
  gate.require(result.worker_tasks.size()==result.workers.size() && t.published==t.consumed && consumed==t.consumed &&
    t.task_pairs<=mono.work.expanded_pairs && t.split_rectangles<=mono.work.input_rectangles &&
    (p.started_workers>1 || (t.published==0 && t.refused==0 && t.split_rectangles==0)),
    "parallel rectangle-range task ledger mismatch");
  if (t.published>0) ++gate.task_sharing_calls;
  gate.task_ranges+=t.published;gate.task_splits+=t.split_rectangles;gate.task_refusals+=t.refused;
}

void parallel_fixtures(GlobalGate& gate) {
  const std::array<Points,3> fixtures{{
    {{10,10,10},{16,16,10},{16,10,16},{10,16,16},{12,12,8},{14,14,8}},
    {{30,30,30},{36,36,30},{30,36,24},{36,30,24},{30,36,30},{36,30,30},{30,30,24},{32,32,32}},
    {{900,1000,1000},{1100,1000,1000},{1000,1120,1040},{1000,1120,960},
     {1000,1020,1105},{1001,1020,1105},{1002,1020,1105},{1003,1020,1105}}
  }};
  for (std::size_t fixture=0;fixture<fixtures.size();++fixture) {
    const auto& points=fixtures[fixture];
    const auto all=global_oracle(gate,points);
    const unsigned k=fixture==2?5:3;
    for (const auto backend:{mhgp9::gen::WspdQ4Backend::Local28,mhgp9::gen::WspdQ4Backend::Window30})
      for (const std::size_t workers:{1U,2U,4U})
        for (const std::size_t grain:{1U,3U})
          parallel_case(gate,points,all,k,options(mhgp9::gen::WspdFrontMode::MidpointSamples,backend),workers,grain);
    for (const auto mask:{std::uint8_t{2},std::uint8_t{4}})
      parallel_case(gate,points,all,k,options(mhgp9::gen::WspdFrontMode::Pure,mhgp9::gen::WspdQ4Backend::Window30,mask),4,1);
  }
  const auto points=shell30();
  const auto all=global_oracle(gate,points);
  parallel_case(gate,points,all,3,options(mhgp9::gen::WspdFrontMode::Pure,mhgp9::gen::WspdQ4Backend::Window30),4,3);
  const Points singleton{{1,2,3}};
  const Output empty;
  parallel_case(gate,singleton,empty,5,options(mhgp9::gen::WspdFrontMode::Pure,mhgp9::gen::WspdQ4Backend::Local28),4,1);
  parallel_case(gate,fixtures[0],empty,1,options(mhgp9::gen::WspdFrontMode::Pure,mhgp9::gen::WspdQ4Backend::Window30),4,1);
  parallel_case(gate,fixtures[0],empty,2,options(mhgp9::gen::WspdFrontMode::Pure,mhgp9::gen::WspdQ4Backend::Window30,4),2,1);
}

void parallel_lifecycle(GlobalGate& gate) {
  const Points points{{10,10,10},{16,16,10},{16,10,16},{10,16,16},{12,12,8},{14,14,8}};
  auto cloud=mhgp9::gen::prepare_cloud(points);
  auto index=mhgp9::gen::make_q2_cloud_index(cloud);
  const auto opts=options(mhgp9::gen::WspdFrontMode::MidpointSamples,mhgp9::gen::WspdQ4Backend::Window30);
  const mhgp9::gen::WspdQ34ParallelConsumer discard=[](std::size_t,const auto&) {};
  auto invoke=[&](std::size_t workers,std::size_t grain,unsigned k,const mhgp9::gen::WspdQ34ParallelConsumer& cb) {
    return mhgp9::gen::run_wspd_q34_parallel(index,k,8,opts,workers,cb,grain);
  };
  gate.rejects([&] { static_cast<void>(invoke(0,1,1,discard)); },"inactive zero workers accepted");
  gate.rejects([&] { static_cast<void>(invoke(1,0,1,discard)); },"inactive zero jobs per worker accepted");
  gate.rejects([&] { static_cast<void>(invoke(2,1,1,{})); },"inactive empty parallel callback accepted");
  gate.rejects<std::overflow_error>([&] { static_cast<void>(invoke(std::numeric_limits<std::size_t>::max(),2,1,discard)); },
                                  "inactive target-job multiplication overflow accepted");
  std::atomic<u64> active{0},calls{0};
  std::atomic<bool> first{true};
  bool caught=false;
  try {
    static_cast<void>(invoke(4,3,5,[&](std::size_t,const auto&) {
      struct Active { std::atomic<u64>& value;explicit Active(std::atomic<u64>& v):value(v) {++value;} ~Active() {--value;} } guard(active);
      ++calls;
      if (first.exchange(false)) throw std::logic_error("parallel callback marker");
      std::this_thread::yield();
    }));
  } catch (const std::logic_error& error) { caught=std::string_view(error.what())=="parallel callback marker"; }
  gate.require(caught && calls.load()>0 && active.load()==0,"parallel callback failure escaped before all callbacks joined");
  ++gate.parallel_callback_failures;++gate.parallel_join_checks;
  const auto expected=eligible(global_oracle(gate,points),5,6);
  const std::weak_ptr<const mhgp9::gen::Q2CensusIndex> weak=index;
  std::atomic<bool> reset{false};
  std::array<Output,4> output;
  const auto result=invoke(4,1,5,[&](std::size_t slot,const auto& value) {
    output.at(slot).push_back(copy(value));
    if (!reset.exchange(true)) {index.reset();cloud.reset();}
  });
  Output combined;
  for (const auto& slot:output) combined.insert(combined.end(),slot.begin(),slot.end());
  normalize(combined);
  gate.require(combined==expected && reset.load() && !index && !cloud && weak.expired() &&
    result.parallel.completed_jobs==result.parallel.jobs,
    "parallel owner reset lost output, retained work or returned before ownership joined");
  ++gate.parallel_owner_resets;++gate.parallel_join_checks;
}

void indexed_fixtures(GlobalGate& gate) {
  std::vector<Points> fixtures{
    {{10,10,10},{16,16,10},{16,10,16},{10,16,16},{12,12,8},{14,14,8}},
    {{100,100,100},{160,100,100},{120,142,100},{128,110,149},{128,88,88}},
    {{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535},{32767,32767,32767},{65535,0,0}},
    {{10,10,10},{11,12,10},{12,10,12},{10,12,12},
     {60000,60000,60000},{60001,60002,60000},{60002,60000,60002},{60000,60002,60002}},
    {{900,1000,1000},{1100,1000,1000},{1000,1120,1040},{1000,1120,960},
     {1000,1020,1105},{1001,1020,1105},{1002,1020,1105},{1003,1020,1105}}
  };
  Points grid;
  for (unsigned x=0;x<3;++x) for (unsigned y=0;y<3;++y) for (unsigned z=0;z<2;++z)
    grid.push_back({static_cast<mhgp9::gen::Coordinate>(20+4*x),static_cast<mhgp9::gen::Coordinate>(20+4*y),
                    static_cast<mhgp9::gen::Coordinate>(20+4*z)});
  fixtures.push_back(grid);
  fixtures.push_back(shell30());
  // 18-bit twins of the extreme fixture (index 2): index 7 runs reversed with
  // midpoint samples, index 8 (corner variant) pure in original order exactly
  // as the u16 fixture did; both pass the q3 atlas consultation identities.
  const std::size_t first_extreme18=fixtures.size();
  fixtures.push_back({{0,0,0},{262143,262143,0},{262143,0,262143},{0,262143,262143},{131071,131071,131071},{262143,0,0}});
  fixtures.push_back({{0,0,0},{262143,262142,262141},{0,262143,262143},{262143,0,262143},{131071,131071,131071},{262143,0,0}});
  // Contre-audit B du 23 septembre 2026 (census q3 sur feuille) : the q3 ball
  // of abx has centre (20,45/2,20) and squared radius 169/4; a,b,x,y are
  // contacts, m is strictly inside, ab is the longest edge, depth 1. m is
  // uniformly inside over the whole centre atlas of ab, so at K3 the root is
  // Deep with exact count K-2=1: only a RETAINED fragment serves this seed.
  const std::size_t leaf_fixture=fixtures.size();
  fixtures.push_back({{14,20,20},{26,20,20},{20,29,20},{20,16,20},{20,20,20}});
  for (std::size_t f=0;f<fixtures.size();++f) {
    auto points=fixtures[f];
    if (f%2!=0) std::reverse(points.begin(),points.end());
    const auto all=global_oracle(gate,points);
    const auto index=mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    for (const auto mode:{mhgp9::gen::WspdQ34WitnessMode::Pair,mhgp9::gen::WspdQ34WitnessMode::RectanglePair})
      for (const auto census_mode:{mhgp9::gen::WspdQ3CensusMode::ScalarCover,mhgp9::gen::WspdQ3CensusMode::GlobalBoxes})
      for (const auto backend:{mhgp9::gen::WspdQ4Backend::Local28,mhgp9::gen::WspdQ4Backend::Window30})
        for (const auto k:{3U,5U,10U}) for (const bool leaf:{false,true}) {
          if (leaf && (census_mode!=mhgp9::gen::WspdQ3CensusMode::GlobalBoxes ||
                       backend!=mhgp9::gen::WspdQ4Backend::Local28)) continue;
          auto config=options(f%2?mhgp9::gen::WspdFrontMode::MidpointSamples:mhgp9::gen::WspdFrontMode::Pure,backend);
          config.witness_mode=mode;
          config.q3_census_mode=census_mode;
          // Exercise the real Local28 defaults too, not only the tiny atlas
          // used by the old combinatorial gate for fast exhaustive checks.
          // Boxed census on Local28 also consults the shared q4 atlas for the
          // q3 seeds: the oracle comparison below must stay bit-identical. The
          // consultation runs with the real atlas options so that certified
          // cells actually exist (the tiny root-only atlas certifies nothing).
          config.q3_atlas_consultation=census_mode==mhgp9::gen::WspdQ3CensusMode::GlobalBoxes &&
            backend==mhgp9::gen::WspdQ4Backend::Local28;
          if (f==0 || config.q3_atlas_consultation) config.local=mhgp9::gen::Q4LocalOptions{};
          // Small fixtures never exceed the default leaf population (32 active
          // sites), so the atlas would keep its single root cell: force real
          // refinement so that certified sub-cells exist to be consulted.
          if (config.q3_atlas_consultation) config.local.leaf_sites=2;
          // Leaf census: the same oracle must hold with and without the
          // retained exact fragments, saturating atlas on (chain default).
          config.q3_leaf_census=leaf;
          config.local.retain_q3_fragments=leaf;
          if (leaf) config.local.saturate_deep=true;
          Output actual;
          const auto result=mhgp9::gen::run_wspd_q34_candidates(index,k,8+2*(k%3),config,
              [&](const auto& value) {actual.push_back(copy(value));});
          normalize(actual);
          gate.require(actual==eligible(all,k,6),"indexed filter lost support/depth/key/complete shell");
          const auto& w=result.work;const auto& v=w.witness;
          gate.require(w.q3_edges+v.rectangle_q3_pairs+v.pair_q3_pairs==result.front.work.residual_pair_mass[1] &&
            w.q4_edges+v.rectangle_q4_pairs+v.pair_q4_pairs==result.front.work.residual_pair_mass[2] &&
            w.expanded_pairs+v.rectangle_pair_mass==v.input_pair_mass &&
            w.cover_builds+v.rejected_pairs==w.expanded_pairs &&
            v.pairs.queries==w.expanded_pairs,"indexed mass partition differs");
          gate.require((mode==mhgp9::gen::WspdQ34WitnessMode::Pair && v.rectangles==mhgp9::gen::Q34WitnessSearchWork{}) ||
            (mode==mhgp9::gen::WspdQ34WitnessMode::RectanglePair && v.rectangles.queries==w.input_rectangles),
            "indexed search entry ledger differs");
          ++gate.indexed_calls;
          if (f>=first_extreme18 && f<leaf_fixture) ++gate.extreme18_indexed_calls;
          if (census_mode==mhgp9::gen::WspdQ3CensusMode::GlobalBoxes) {
            ++gate.boxed_calls;
            const auto& atlas=w.q3_atlas;
            gate.require(w.q3.census_point_tests==0 && w.q3.census_range_visits==0 &&
              w.q3_blocks.queries+atlas.rejections+atlas.leaf_censuses==w.q3.seeds &&
              w.q3_blocks.accepted_queries+atlas.leaf_censuses-atlas.leaf_rejections==w.q3.emitted &&
              w.q3_blocks.rejected_queries+atlas.rejections+atlas.leaf_rejections==w.q3.depth_rejections &&
              w.q3_blocks.shell_ids<=w.q3.shell_ids &&
              (atlas.leaf_censuses!=0 || w.q3_blocks.shell_ids==w.q3.shell_ids) &&
              atlas.leaf_rejections<=atlas.leaf_censuses &&
              w.q3.ball_builds+atlas.rejections==w.q3.seeds,
              "q3 boxed census ledger differs or hides a scalar scan");
            gate.require(leaf || (atlas.leaf_censuses==0 && atlas.leaf_point_tests==0 && atlas.leaf_rejections==0),
              "q3 leaf census ran although disabled");
            if (leaf) {
              ++gate.leaf_calls;gate.leaf_censuses+=atlas.leaf_censuses;
              gate.leaf_rejections+=atlas.leaf_rejections;gate.leaf_point_tests+=atlas.leaf_point_tests;
              if (f==leaf_fixture) gate.leaf_fixture_censuses+=atlas.leaf_censuses;
            }
            if (config.q3_atlas_consultation) {
              gate.require(atlas.edges_with_atlas==w.both_edges && atlas.locations+atlas.root_lane_skips<=w.both_edges+w.q3.seeds &&
                atlas.rejections+atlas.outside_domain<=atlas.locations,"q3 atlas consultation ledger differs");
              gate.atlas_rejections+=atlas.rejections;gate.atlas_lane_skips+=atlas.root_lane_skips;
              gate.atlas_locations+=atlas.locations;gate.atlas_outside+=atlas.outside_domain;
              if (f>=first_extreme18 && f<leaf_fixture) {
                gate.extreme18_atlas_locations+=atlas.locations;gate.extreme18_atlas_rejections+=atlas.rejections;
              }
            } else gate.require(atlas==mhgp9::gen::WspdQ3AtlasWork{},"atlas consultation ran although disabled");
          } else gate.require(w.q3_blocks==mhgp9::gen::Q3BallCensusWork{} && w.q3_atlas==mhgp9::gen::WspdQ3AtlasWork{},"scalar performed hidden boxed census");
          gate.indexed_pair_rejections+=v.rejected_pairs;
          gate.indexed_rectangle_rejections+=v.rejected_rectangles;
          if (k==3) parallel_case(gate,points,all,k,config,4,3);
        }
  }
  const auto index=mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(fixtures[0]));
  mhgp9::gen::WspdQ34Options config;
  config.witness_mode=static_cast<mhgp9::gen::WspdQ34WitnessMode>(91);
  gate.rejects([&] {static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,1,8,config,[](const auto&) {}));},
    "invalid witness mode accepted even on inactive lanes");
  config.witness_mode=mhgp9::gen::WspdQ34WitnessMode::Disabled;
  config.q3_census_mode=static_cast<mhgp9::gen::WspdQ3CensusMode>(91);
  gate.rejects([&] {static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,1,8,config,[](const auto&) {}));},
    "invalid q3 census mode accepted even on inactive lanes");
  config.q3_census_mode=mhgp9::gen::WspdQ3CensusMode::GlobalBoxes;
  parallel_case(gate,fixtures[0],global_oracle(gate,fixtures[0]),5,config,4,1);
  config.q3_census_mode=mhgp9::gen::WspdQ3CensusMode::ScalarCover;
  for (const auto mode:{mhgp9::gen::WspdQ34WitnessMode::Pair,mhgp9::gen::WspdQ34WitnessMode::RectanglePair})
    for (const auto k:{1U,2U,5U}) for (const std::uint8_t mask:{2,4}) {
      config.witness_mode=mode;config.requested_lane_mask=mask;
      parallel_case(gate,fixtures[0],global_oracle(gate,fixtures[0]),k,config,1,1);
    }
  gate.require(gate.indexed_calls>0 && gate.indexed_pair_rejections>0 && gate.indexed_rectangle_rejections>0,
    "indexed global modes have no exercised rejection");
}

std::array<u64,433> without_filter_geometry(mhgp9::gen::WspdQ34Work work) {
  // Keep ALL rejection masses and all work downstream of filtering. Only the
  // four explicitly changed search/bounds ledgers are normalized away.
  work.witness.rectangles={};work.witness.pairs={};
  work.witness.rectangles_bounds={};work.witness.pairs_bounds={};
  return logical_work(work);
}

void bounds_mode_global_fixtures(GlobalGate& gate) {
  using Mode=mhgp9::gen::Q34WitnessBoundsMode;
  std::vector<Points> fixtures{
    {{10,10,10},{16,16,10},{16,10,16},{10,16,16},{12,12,8},{14,14,8}},
    {{100,100,100},{160,100,100},{120,142,100},{128,110,149},{128,88,88}},
    {{30,30,30},{36,36,30},{30,36,24},{36,30,24},{30,36,30},{36,30,30},{30,30,24},{32,32,32}},
    {{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535},{32767,32767,32767},{65535,0,0}},
    shell30(),
    // 18-bit twin of index 3: index 5 is odd too (reversed, pure front, K=3).
    {{0,0,0},{262143,262143,0},{262143,0,262143},{0,262143,262143},{131071,131071,131071},{262143,0,0}}};
  for (std::size_t f=0;f<fixtures.size();++f) {
    auto points=fixtures[f];if (f%2) std::reverse(points.begin(),points.end());
    const auto all=global_oracle(gate,points);
    const unsigned k=f==0?5U:3U;
    if (f==5) ++gate.extreme18_bounds_calls;
    const auto expected=eligible(all,k,6);
    const auto index=mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    for (const auto witness:{mhgp9::gen::WspdQ34WitnessMode::Pair,mhgp9::gen::WspdQ34WitnessMode::RectanglePair})
      for (const auto backend:{mhgp9::gen::WspdQ4Backend::Local28,mhgp9::gen::WspdQ4Backend::Window30}) {
        auto config=options(f%2?mhgp9::gen::WspdFrontMode::Pure:mhgp9::gen::WspdFrontMode::MidpointSamples,backend);
        config.witness_mode=witness;config.q3_census_mode=mhgp9::gen::WspdQ3CensusMode::GlobalBoxes;
        Output previous;
        const auto legacy=mhgp9::gen::run_wspd_q34_candidates(index,k,8,config,[&](const auto& v) {previous.push_back(copy(v));});
        normalize(previous);
        gate.require(previous==expected,"bounds-mode Legacy baseline differs from independent full payload oracle");
        gate.require(legacy.work.witness.rectangles_bounds==mhgp9::gen::Q34WitnessBoundsWork{} &&
          legacy.work.witness.pairs_bounds==mhgp9::gen::Q34WitnessBoundsWork{},"Legacy wrote new bounds counters");
        for (const auto mode:{Mode::Exclusion,Mode::Affine}) {
          config.witness_bounds_mode=mode;
          Output actual;
          const auto result=mhgp9::gen::run_wspd_q34_candidates(index,k,8,config,[&](const auto& v) {actual.push_back(copy(v));});
          normalize(actual);
          gate.require(actual==expected,"bounds mode lost support/depth/key/complete shell");
          gate.require(result.front.work==legacy.front.work &&
            without_filter_geometry(result.work)==without_filter_geometry(legacy.work),
            "bounds mode changed non-filter geometric work or lane masses");
          ++gate.bounds_mode_calls;++gate.bounds_work_checks;
          if (mode==Mode::Exclusion) ++gate.bounds_exclusion_calls;else ++gate.bounds_affine_calls;
          const auto& w=result.work.witness;
          gate.require(w.pairs_bounds.queries==w.pairs.queries && w.rectangles_bounds.queries==w.rectangles.queries,
            "pipeline did not retain all private bounds work");
          if (f==0) {
            for (const std::size_t workers:{1U,2U,4U}) {
              parallel_case(gate,points,all,k,config,workers,1);++gate.bounds_parallel_calls;
            }
          }
        }
      }
  }
  const auto points=fixtures[0];auto cloud=mhgp9::gen::prepare_cloud(points);
  auto index=mhgp9::gen::make_q2_cloud_index(cloud);
  auto config=options(mhgp9::gen::WspdFrontMode::MidpointSamples,mhgp9::gen::WspdQ4Backend::Window30);
  config.witness_mode=mhgp9::gen::WspdQ34WitnessMode::RectanglePair;
  config.q3_census_mode=mhgp9::gen::WspdQ3CensusMode::GlobalBoxes;
  const auto expected=eligible(global_oracle(gate,points),5,6);
  for (const auto mode:{Mode::Exclusion,Mode::Affine}) {
    config.witness_bounds_mode=mode;
    for (const auto k:{1U,2U}) {
      auto inactive=config;inactive.requested_lane_mask=4;
      const auto result=mhgp9::gen::run_wspd_q34_parallel(index,k,8,inactive,4,
          [](std::size_t,const auto&) {throw std::runtime_error("inactive bounds callback");},1);
      gate.require(logical_work(result.pipeline.work)==logical_work(mhgp9::gen::WspdQ34Work{}) &&
        result.workers.empty() && result.parallel.jobs==0,"inactive bounds pipeline did work");++gate.bounds_inactive_calls;
    }
    auto disabled=config;disabled.witness_mode=mhgp9::gen::WspdQ34WitnessMode::Disabled;
    Output out;
    const auto result=mhgp9::gen::run_wspd_q34_candidates(index,5,8,disabled,[&](const auto& v) {out.push_back(copy(v));});
    normalize(out);
    gate.require(out==expected && result.work.witness.rectangles_bounds==mhgp9::gen::Q34WitnessBoundsWork{} &&
      result.work.witness.pairs_bounds==mhgp9::gen::Q34WitnessBoundsWork{},"disabled witness mode performed hidden bounds work");
    ++gate.bounds_inactive_calls;
    bool caught=false;
    try {static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,5,8,config,[](const auto&) {
      throw std::logic_error("bounds callback marker");}));}
    catch (const std::logic_error& error) {caught=std::string_view(error.what())=="bounds callback marker";}
    gate.require(caught,"new bounds pipeline swallowed callback failure");++gate.bounds_callback_failures;
  }
  for (const auto witness:{mhgp9::gen::WspdQ34WitnessMode::Disabled,mhgp9::gen::WspdQ34WitnessMode::RectanglePair}) {
    auto invalid=config;invalid.witness_mode=witness;invalid.witness_bounds_mode=static_cast<Mode>(99);
    for (bool parallel:{false,true}) {
      bool caught=false;
      try {
        if (parallel) static_cast<void>(mhgp9::gen::run_wspd_q34_parallel(index,1,8,invalid,1,[](std::size_t,const auto&) {},1));
        else static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,1,8,invalid,[](const auto&) {}));
      } catch (const std::invalid_argument&) {caught=true;}
      gate.require(caught,"inactive or disabled pipeline ignored invalid bounds enum");++gate.bounds_invalid_inputs;
    }
  }
  config.witness_bounds_mode=Mode::Affine;
  for (std::size_t before=0;before<4;++before) {
    global_q34_allocation::state={true,before};bool caught=false;
    try {static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,5,8,config,[](const auto&) {}));}
    catch (const std::bad_alloc&) {caught=true;}
    global_q34_allocation::state.armed=false;
    gate.require(caught,"allocation failure did not propagate through new bounds pipeline");++gate.bounds_allocation_failures;
  }
  std::array<std::future<Output>,4> tasks;
  for (std::size_t i=0;i<tasks.size();++i) tasks[i]=std::async(std::launch::async,[index,config,i] {
    auto opts=config;if (i%2) opts.witness_bounds_mode=Mode::Exclusion;
    Output out;static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,5,8,opts,[&](const auto& v) {out.push_back(copy(v));}));
    normalize(out);return out;
  });
  for (auto& task:tasks) {gate.require(task.get()==expected,"shared index bounds pipeline changed full payload");++gate.bounds_shared_calls;}
  Output out;
  static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,5,8,config,[&](const auto& v) {
    out.push_back(copy(v));if (index) {index.reset();cloud.reset();++gate.bounds_owner_resets;}
  }));
  normalize(out);gate.require(out==expected && !index && !cloud,"bounds callback owner reset lost payload");
}

std::array<u64,433> without_local_q4(mhgp9::gen::WspdQ34Work work) {
  // Only the selected q4 traversal is replaced. All q3, filtering, covers,
  // lane masses and output counters remain subject to exact comparison.
  work.local={};work.q4_seed_cells={};
  return logical_work(work);
}

void seed_cell_global_fixtures(GlobalGate& gate) {
  using Mode=mhgp9::gen::Q4SeedCellMode;
  const std::vector<Points> fixtures{
    {{10,10,10},{16,16,10},{16,10,16},{10,16,16},{12,12,8},{14,14,8}},
    {{30,30,30},{36,36,30},{30,36,24},{36,30,24},{30,36,30},{36,30,30},{30,30,24},{32,32,32}},
    {{10,20,20},{30,20,20},{20,29,27},{20,11,22}},
    {{900,1000,1000},{1100,1000,1000},{1000,1120,1040},{1000,1120,960},{1000,1020,1105},{1001,1020,1105}},
    shell30()};
  for (std::size_t f=0;f<fixtures.size();++f) {
    auto points=fixtures[f];if (f%2) std::reverse(points.begin(),points.end());
    const auto all=global_oracle(gate,points);const unsigned k=f==3?3U:5U;
    const auto expected=eligible(all,k,6);
    const auto index=mhgp9::gen::make_q2_cloud_index(mhgp9::gen::prepare_cloud(points));
    auto config=options(mhgp9::gen::WspdFrontMode::MidpointSamples,mhgp9::gen::WspdQ4Backend::Local28);
    config.local.max_depth=3;config.local.node_budget=85;config.local.leaf_sites=0;config.local.z_test_budget=32;
    config.local.domain=f%2?mhgp9::gen::Q4CenterDomainMode::Disk:mhgp9::gen::Q4CenterDomainMode::Positive;
    config.witness_mode=mhgp9::gen::WspdQ34WitnessMode::RectanglePair;
    config.q3_census_mode=mhgp9::gen::WspdQ3CensusMode::GlobalBoxes;
    config.witness_bounds_mode=mhgp9::gen::Q34WitnessBoundsMode::Affine;
    Output baseline;
    const auto old=mhgp9::gen::run_wspd_q34_candidates(index,k,8,config,[&](const auto& value) {baseline.push_back(copy(value));});
    normalize(baseline);
    gate.require(baseline==expected && old.work.q4_seed_cells==mhgp9::gen::Q4SeedCellWork{},
      "Individual global baseline differs from rational oracle or used hidden live work");
    for (const auto mode:{Mode::LiveOnly,Mode::Joined}) {
      config.q4_seed_cells={mode,f%2?1U:2U};Output actual;
      const auto result=mhgp9::gen::run_wspd_q34_candidates(index,k,8,config,[&](const auto& value) {actual.push_back(copy(value));});
      normalize(actual);
      gate.require(actual==expected,"seed-cell global stream differs from independent rational support/depth/shell oracle");
      gate.require(result.front.work==old.front.work && without_local_q4(result.work)==without_local_q4(old.work),
        "seed-cell q4 traversal changed another lane, filter, cover or output work");
      const auto& w=result.work.q4_seed_cells;
      gate.require(w.queries==result.work.q4_edges && w.live_preparations+w.whole_atlas_skips==w.queries &&
        w.live_node_visits<=result.work.local.atlas.cells_created,"pipeline lost private live-summary work");
      if (mode==Mode::Joined) gate.require(w.terminal_pairs==w.family_preparations+w.family_cache_hits &&
        w.terminal_pairs==result.work.local.sweep.leaf_queries && w.family_preparations==result.work.local.sweep.seed_queries,
        "pipeline lost joined cache/family/terminal accounting");
      ++gate.seed_cell_calls;++gate.seed_cell_work_checks;
      if (mode==Mode::LiveOnly) ++gate.seed_cell_live_calls;else ++gate.seed_cell_joined_calls;
      if (f<2) for (const std::size_t workers:{1U,2U,4U}) {
        parallel_case(gate,points,all,k,config,workers,1);++gate.seed_cell_parallel_calls;
      }
    }
  }
  const auto points=fixtures[0];const auto all=global_oracle(gate,points),expected=eligible(all,5,6);
  auto cloud=mhgp9::gen::prepare_cloud(points);auto index=mhgp9::gen::make_q2_cloud_index(cloud);
  auto config=options(mhgp9::gen::WspdFrontMode::MidpointSamples,mhgp9::gen::WspdQ4Backend::Local28);
  config.local.max_depth=3;config.local.node_budget=85;config.local.leaf_sites=0;config.local.z_test_budget=32;
  config.witness_mode=mhgp9::gen::WspdQ34WitnessMode::RectanglePair;
  config.q3_census_mode=mhgp9::gen::WspdQ3CensusMode::GlobalBoxes;config.witness_bounds_mode=mhgp9::gen::Q34WitnessBoundsMode::Affine;
  for (const auto mode:{Mode::LiveOnly,Mode::Joined}) {
    config.q4_seed_cells={mode,2};
    for (const auto k:{1U,2U}) {
      auto inactive=config;inactive.requested_lane_mask=4;
      const auto result=mhgp9::gen::run_wspd_q34_parallel(index,k,8,inactive,4,
        [](std::size_t,const auto&) {throw std::runtime_error("inactive seed-cell callback");},1);
      gate.require(logical_work(result.pipeline.work)==logical_work(mhgp9::gen::WspdQ34Work{}) && result.workers.empty(),
        "inactive seed-cell pipeline prepared hidden live/cache state");++gate.seed_cell_inactive_calls;
    }
    auto q3only=config;q3only.requested_lane_mask=2;Output q3;
    const auto result=mhgp9::gen::run_wspd_q34_candidates(index,5,8,q3only,[&](const auto& value) {q3.push_back(copy(value));});
    normalize(q3);gate.require(q3==eligible(all,5,2) && result.work.q4_seed_cells==mhgp9::gen::Q4SeedCellWork{},
      "q3-only stream depends on q4 live atlas");++gate.seed_cell_inactive_calls;
    bool caught=false;
    try {static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,5,8,config,[](const auto&) {
      throw std::logic_error("global seed-cell callback marker");}));}
    catch (const std::logic_error& e) {caught=std::string_view(e.what())=="global seed-cell callback marker";}
    gate.require(caught,"seed-cell global callback failure was swallowed");++gate.seed_cell_callback_failures;
  }
  for (unsigned invalid=0;invalid<3;++invalid) for (const auto k:{1U,5U}) for (bool parallel:{false,true}) {
    auto bad=config;
    if (invalid==0) bad.q4_seed_cells.mode=static_cast<Mode>(99);
    else if (invalid==1) bad.q4_seed_cells.block_sites=0;
    else bad.q4_backend=mhgp9::gen::WspdQ4Backend::Window30;
    bool caught=false;
    try {
      if (parallel) static_cast<void>(mhgp9::gen::run_wspd_q34_parallel(index,k,8,bad,2,[](std::size_t,const auto&) {},1));
      else static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,k,8,bad,[](const auto&) {}));
    } catch (const std::invalid_argument&) {caught=true;}
    gate.require(caught,"invalid or incompatible seed-cell options escaped active/inactive validation");++gate.seed_cell_invalid_inputs;
  }
  config.q4_seed_cells={Mode::Joined,2};
  for (std::size_t before=0;before<4;++before) {
    global_q34_allocation::state={true,before};bool caught=false;
    try {static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,5,8,config,[](const auto&) {}));}
    catch (const std::bad_alloc&) {caught=true;}
    global_q34_allocation::state.armed=false;
    gate.require(caught,"seed-cell pipeline allocation failure did not propagate");++gate.seed_cell_allocation_failures;
  }
  std::array<std::future<Output>,4> calls;
  for (std::size_t i=0;i<calls.size();++i) calls[i]=std::async(std::launch::async,[index,config,i] {
    auto opts=config;if (i%2) opts.q4_seed_cells.mode=Mode::LiveOnly;
    Output out;static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,5,8,opts,[&](const auto& v) {out.push_back(copy(v));}));
    normalize(out);return out;
  });
  for (auto& call:calls) {gate.require(call.get()==expected,"concurrent shared-index seed-cell output changed");++gate.seed_cell_shared_calls;}
  bool caught=false;std::atomic<u64> completed_callbacks{0};
  try {static_cast<void>(mhgp9::gen::run_wspd_q34_parallel(index,5,8,config,4,[&](std::size_t,const auto&) {
    completed_callbacks.fetch_add(1,std::memory_order_relaxed);throw std::logic_error("parallel seed-cell marker");},1));}
  catch (const std::logic_error& e) {caught=std::string_view(e.what())=="parallel seed-cell marker";}
  const auto after_join=completed_callbacks.load(std::memory_order_relaxed);
  gate.require(caught && after_join>0,"parallel seed-cell failure did not propagate after callback");
  ++gate.seed_cell_parallel_failures;
  Output out;
  static_cast<void>(mhgp9::gen::run_wspd_q34_candidates(index,5,8,config,[&](const auto& v) {
    out.push_back(copy(v));if (index) {index.reset();cloud.reset();++gate.seed_cell_owner_resets;}
  }));
  normalize(out);gate.require(out==expected && !index && !cloud && completed_callbacks.load(std::memory_order_relaxed)==after_join,
    "seed-cell callback owner reset lost output or failed parallel call left live workers");
}
}

int main(int argc,char** argv) {
  try {
    if (argc!=2 || std::string_view(argv[1])!="--selftest") throw std::invalid_argument("expected --selftest");
    GlobalGate gate;
    global_fixtures(gate);global_lifecycle(gate);parallel_fixtures(gate);parallel_lifecycle(gate);indexed_fixtures(gate);
    bounds_mode_global_fixtures(gate);
    seed_cell_global_fixtures(gate);
    gate.require(gate.task_sharing_calls>0 && gate.task_ranges>0 && gate.task_splits>0 && gate.task_refusals>0,
      "rectangle-range task sharing was never exercised (calls, ranges, split rectangles, full-queue refusals)");
    gate.require(gate.atlas_rejections>0 && gate.atlas_locations>gate.atlas_rejections && gate.atlas_outside>0,
      "q3 atlas consultation was never exercised (rejections, non-rejected locations, outside-domain centers)");
    gate.require(gate.extreme18_calls==5 && gate.deep_atlas_calls==2 && gate.extreme18_indexed_calls==60 &&
      gate.extreme18_bounds_calls==1 && gate.extreme18_atlas_locations>0,
      "18-bit extreme twins were not exercised (global, deep atlas, indexed consultation, bounds modes)");
    gate.require(gate.leaf_calls>0 && gate.leaf_censuses>0 && gate.leaf_rejections>0 &&
      gate.leaf_point_tests>gate.leaf_censuses && gate.leaf_fixture_censuses>0,
      "q3 leaf census was never exercised (calls, censuses, rejections, frontier tests, retained-fragment fixture)");
    gate.require(gate.q3>0 && gate.q4>0 && gate.max_shell>=30 && gate.fat_rectangles>0 &&
      gate.q3_front_rejections>0 && gate.q4_front_rejections>0 && gate.xi_tests>0 &&
      gate.q3_depth_rejections>0 && gate.q3_unread_sites>0 && gate.both_edges>0 &&
      gate.q3_only_edges>0 && gate.q4_only_edges>0 && gate.split_q3_only_edges>0 &&
      gate.split_q4_only_edges>0 && gate.multiworker_calls>0 && gate.callback_copy_checks>0,
      "global q34 gate nonvacuity failed");
    std::cout<<"{\"schema\":\"mhgp9_gen_wspd_q34_gate_v1\",\"status\":\"PASS\"";
#define EMIT(field) std::cout<<",\"" #field "\":"<<gate.field
    EMIT(checks);EMIT(global_calls);EMIT(oracle_clouds);EMIT(oracle_triangles);EMIT(oracle_tetrahedra);
    EMIT(oracle_balls);EMIT(positive_triangles);EMIT(positive_tetrahedra);EMIT(canonical_groups);EMIT(oracle_sites);
    EMIT(front_replays);EMIT(expanded_pairs);EMIT(fat_rectangles);EMIT(q3_only_edges);EMIT(q4_only_edges);EMIT(both_edges);
    EMIT(split_q3_only_edges);EMIT(split_q4_only_edges);
    EMIT(q3_front_rejections);EMIT(q4_front_rejections);EMIT(xi_tests);EMIT(q3_depth_rejections);EMIT(q3_unread_sites);
    EMIT(pure_calls);EMIT(sample_calls);EMIT(local_calls);EMIT(window_calls);EMIT(inactive_calls);EMIT(singleton_calls);
    EMIT(s8_calls);EMIT(s10_calls);EMIT(s12_calls);EMIT(strict_w3_contacts);EMIT(strict_w4_contacts);
    EMIT(independent_q2_cases);EMIT(independent_q3_cases);EMIT(isolated_cases);EMIT(extreme_calls);
    EMIT(candidates);EMIT(q3);EMIT(q4);EMIT(max_shell);EMIT(permutations);EMIT(invalid_inputs);
    EMIT(allocation_failures);EMIT(callback_failures);EMIT(parallel_calls);EMIT(nested_calls);EMIT(owner_reset_calls);EMIT(input_alias_checks);
    EMIT(parallel_pipeline_calls);EMIT(parallel_w1_calls);EMIT(parallel_w2_calls);EMIT(parallel_w4_calls);
    EMIT(parallel_geometry_checks);EMIT(worker_ledger_checks);EMIT(callback_copy_checks);EMIT(multiworker_calls);
    EMIT(parallel_callback_failures);EMIT(parallel_join_checks);EMIT(parallel_owner_resets);EMIT(parallel_empty_calls);
    EMIT(indexed_calls);EMIT(indexed_pair_rejections);EMIT(indexed_rectangle_rejections);EMIT(boxed_calls);
    EMIT(atlas_rejections);EMIT(atlas_lane_skips);EMIT(atlas_locations);EMIT(atlas_outside);
    EMIT(task_sharing_calls);EMIT(task_ranges);EMIT(task_splits);EMIT(task_refusals);
    EMIT(bounds_mode_calls);EMIT(bounds_exclusion_calls);EMIT(bounds_affine_calls);EMIT(bounds_work_checks);EMIT(bounds_parallel_calls);
    EMIT(bounds_invalid_inputs);EMIT(bounds_inactive_calls);EMIT(bounds_callback_failures);EMIT(bounds_allocation_failures);
    EMIT(bounds_shared_calls);EMIT(bounds_owner_resets);
    EMIT(seed_cell_calls);EMIT(seed_cell_live_calls);EMIT(seed_cell_joined_calls);EMIT(seed_cell_work_checks);EMIT(seed_cell_parallel_calls);
    EMIT(seed_cell_inactive_calls);EMIT(seed_cell_invalid_inputs);EMIT(seed_cell_callback_failures);EMIT(seed_cell_allocation_failures);
    EMIT(seed_cell_shared_calls);EMIT(seed_cell_owner_resets);EMIT(seed_cell_parallel_failures);
    EMIT(leaf_calls);EMIT(leaf_censuses);EMIT(leaf_rejections);EMIT(leaf_point_tests);EMIT(leaf_fixture_censuses);
#undef EMIT
    std::cout<<"}\n";
    return 0;
  } catch (const std::exception& error) {
    global_q34_allocation::state.armed=false;
    std::cerr<<"wspd q34 gate: "<<error.what()<<'\n';return 1;
  }
}
