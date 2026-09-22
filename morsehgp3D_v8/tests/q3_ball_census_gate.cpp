#include "lanes/q3_ball_census.hpp"

// Explicit reuse of the constructor's independent rational Gram solver,
// never the product's coefficient, box-bound or census implementation.
#include "exact_ball_oracle.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <future>
#include <iostream>
#include <limits>
#include <new>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace ball_census_allocation {
thread_local bool fail=false;
[[gnu::noinline]] void* allocate(std::size_t bytes,std::size_t alignment=alignof(std::max_align_t)) {
  if (fail) { fail=false;throw std::bad_alloc(); }
  void* pointer=nullptr;
  if (alignment<=alignof(std::max_align_t)) pointer=std::malloc(bytes==0?1:bytes);
  else if (posix_memalign(&pointer,alignment,bytes==0?1:bytes)!=0) pointer=nullptr;
  if (!pointer) throw std::bad_alloc();
  return pointer;
}
[[gnu::noinline]] void release(void* pointer) noexcept { std::free(pointer); }
}
void* operator new(std::size_t n) { return ball_census_allocation::allocate(n); }
void* operator new[](std::size_t n) { return ball_census_allocation::allocate(n); }
void* operator new(std::size_t n,std::align_val_t a) { return ball_census_allocation::allocate(n,static_cast<std::size_t>(a)); }
void* operator new[](std::size_t n,std::align_val_t a) { return ball_census_allocation::allocate(n,static_cast<std::size_t>(a)); }
void operator delete(void* p) noexcept { ball_census_allocation::release(p); }
void operator delete[](void* p) noexcept { ball_census_allocation::release(p); }
void operator delete(void* p,std::size_t) noexcept { ball_census_allocation::release(p); }
void operator delete[](void* p,std::size_t) noexcept { ball_census_allocation::release(p); }
void operator delete(void* p,std::align_val_t) noexcept { ball_census_allocation::release(p); }
void operator delete[](void* p,std::align_val_t) noexcept { ball_census_allocation::release(p); }
void operator delete(void* p,std::size_t,std::align_val_t) noexcept { ball_census_allocation::release(p); }
void operator delete[](void* p,std::size_t,std::align_val_t) noexcept { ball_census_allocation::release(p); }

namespace {
using mhgp8::Point3;
using mhgp8::u64;
using Points=std::vector<Point3>;
using Ids=std::vector<std::size_t>;
namespace oracle=mhgp8_test::ball_oracle;
using Work=mhgp8::Q3BallCensusWork;
static_assert(std::is_trivially_copyable_v<Work> && std::is_standard_layout_v<Work> && sizeof(Work)==26*sizeof(u64));

struct Gate {
  u64 checks{},calls{},oracle_sites{},accepted{},rejected{},q2_balls{},q3_balls{},q4_balls{};
  u64 shell_ids{},max_shell{},empty_shells{},integer_grid_contacts{},ceil_required{},floor_required{};
  u64 whole_inside{},whole_nonnegative{},shell_exclusions{},saturating_overshoot{},prepared_unvisited{};
  u64 near_first_cases{},singleton_cases{},extreme_cases{},wide_linear_squares{},permutations{},huge_threshold{};
  u64 invalid_inputs{},allocation_failures{},allocation_free_rejections{},parallel_calls{},repeated_calls{};
  u64 source_alias_checks{},overflow_exceptions{};
  // 18-bit twin of the wide support (262143/262142/262141): its own floor, so
  // the historical 65535 fixture keeps wide_linear_squares==3 unchanged.
  u64 wide_linear_squares_u18{},extreme_cases_u18{};
  void require(bool condition,const char* message) {
    ++checks;if (!condition) throw std::runtime_error(message);
  }
};

struct Ball {
  mhgp8::ExactBall product;
  oracle::Ball rational;
};
Ball make_ball(Gate& gate,const Points& support) {
  const auto product=[&]() {
    if (support.size()==2) return mhgp8::ExactBall::make_q2(support[0],support[1]);
    if (support.size()==3) return mhgp8::ExactBall::make_q3({support[0],support[1],support[2]});
    if (support.size()==4) return mhgp8::ExactBall::make_q4({support[0],support[1],support[2],support[3]});
    throw std::runtime_error("fixture support size");
  }();
  const auto independent=oracle::make(support);
  gate.require(product.has_value() && independent.ball.has_value(),"fixture is not a positive rational ball");
  for (std::size_t i=0;i<5;++i)
    gate.require(oracle::Big(product->coefficients()[i])==independent.ball->coefficients[i],
        "product key differs from independent rational Gram solve");
  if (support.size()==2) ++gate.q2_balls;
  if (support.size()==3) ++gate.q3_balls;
  if (support.size()==4) ++gate.q4_balls;
  return {*product,*independent.ball};
}

struct Census { std::size_t depth{};Ids shell; };
Census census(Gate& gate,const Points& points,const oracle::Ball& ball) {
  Census result;
  for (std::size_t i=0;i<points.size();++i) {
    const auto power=ball.power(points[i]);++gate.oracle_sites;
    if (power.numerator()<0) ++result.depth;
    if (power.numerator()==0) result.shell.push_back(i);
  }
  return result;
}

void ledger(Gate& gate,const Work& work,mhgp8::Q3BallCensusResult result,
            std::size_t n,std::size_t threshold,const Ids& shell) {
  gate.require(work.queries==1 && work.preparations==1 && work.vertex_axes==3,"power preparation accounting");
  gate.require(work.accepted_queries==static_cast<u64>(result.accepted) &&
      work.rejected_queries==static_cast<u64>(!result.accepted),"result accounting");
  gate.require(work.count_bounds_prepared==work.count_node_visits+work.count_prepared_unvisited &&
      work.count_bounds_prepared==work.count_box_bound_tests+work.count_point_tests,
      "prepared first-pass bounds omitted or repeated");
  gate.require(work.count_node_visits==work.count_inside_nodes+work.count_nonnegative_nodes+work.count_split_nodes,
      "count node classification partition");
  gate.require(work.count_bounds_prepared==1+2*work.count_split_nodes && work.count_inside_sites==result.depth &&
      work.count_saturations==static_cast<u64>(!result.accepted),"count expansion or saturated credits");
  gate.require(work.peak_count_stack>=1 && work.peak_count_stack<=mhgp8::index_stack_frames && work.peak_shell_stack<=mhgp8::index_stack_frames &&
      work.stack_storage_bytes>=mhgp8::index_stack_frames*sizeof(std::size_t),"fixed proven stack backing");
  if (result.accepted) {
    gate.require(result.depth<threshold && work.count_prepared_unvisited==0 &&
        work.count_inside_sites+work.count_nonnegative_sites==n,"accepted global depth population partition");
    gate.require(work.shell_node_visits==work.shell_bounds_prepared &&
        work.shell_bounds_prepared==work.shell_box_bound_tests+work.shell_point_tests &&
        work.shell_bounds_prepared==1+2*work.shell_split_nodes &&
        work.shell_node_visits==work.shell_excluded_nodes+work.shell_split_nodes+work.shell_ids &&
        work.shell_ids==shell.size() && work.peak_shell_stack>=1,"shell second-pass partition");
  } else {
    gate.require(result.depth==threshold && shell.empty() && work.shell_node_visits==0 &&
        work.shell_bounds_prepared==0 && work.shell_ids==0 && work.peak_shell_stack==0,
        "rejected census collected a shell or reported unsaturated depth");
  }
  if (work.count_inside_nodes>work.count_point_tests) ++gate.whole_inside;
  if (work.count_nonnegative_nodes>work.count_point_tests) ++gate.whole_nonnegative;
  gate.prepared_unvisited+=work.count_prepared_unvisited;
  gate.shell_exclusions+=work.shell_excluded_nodes;
}

Work check(Gate& gate,const mhgp8::Q2CensusIndex& index,const Points& points,const Ball& ball,std::size_t threshold) {
  const auto exact=census(gate,points,ball.rational);
  Ids shell{points.size()+17};Work work;
  const auto result=mhgp8::census_q3_ball(index,ball.product,threshold,shell,work);
  const bool accepted=exact.depth<threshold;
  std::sort(shell.begin(),shell.end());
  gate.require(std::adjacent_find(shell.begin(),shell.end())==shell.end() &&
      result.accepted==accepted && result.depth==std::min(exact.depth,threshold) &&
      shell==(accepted?exact.shell:Ids{}),
      "global census differs from independent rational depth and shell oracle");
  // The geometric comparison above deliberately precedes bookkeeping.
  ++gate.calls;if (accepted) ++gate.accepted;else ++gate.rejected;
  gate.shell_ids+=static_cast<u64>(shell.size());gate.max_shell=std::max(gate.max_shell,static_cast<u64>(shell.size()));
  if (accepted && shell.empty()) ++gate.empty_shells;
  ledger(gate,work,result,points.size(),threshold,shell);
  return work;
}

Work query(Gate& gate,const Points& points,const Ball& ball,std::size_t threshold) {
  const auto index=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  return check(gate,*index,points,ball,threshold);
}

Points shell30() {
  Points result;
  for (int x=-5;x<=5;++x) for (int y=-5;y<=5;++y) for (int z=-5;z<=5;++z)
    if (x*x+y*y+z*z==25)
      result.push_back({static_cast<mhgp8::Coordinate>(30+x),static_cast<mhgp8::Coordinate>(30+y),
                        static_cast<mhgp8::Coordinate>(30+z)});
  return result;
}

void rounding_and_contacts(Gate& gate) {
  const auto fractional=make_ball(gate,{{0,0,0},{2,2,0},{2,0,2}});
  gate.require(fractional.rational.coefficients==oracle::Coefficients{3,-8,-4,-4,0},"fractional fixture key");
  const auto ceil=query(gate,{{0,0,0},{0,1,0}},fractional,2);
  gate.require(ceil.count_inside_sites==1 && ceil.shell_ids==1,"ceil fixture changed");++gate.ceil_required;
  const auto floor=query(gate,{{1,0,2},{2,0,2}},fractional,2);
  gate.require(floor.count_inside_sites==1 && floor.shell_ids==1,"floor fixture changed");++gate.floor_required;
  // Auditor B's correction is explicit: this is a quarter-centre, not the
  // incorrectly proposed half-centre fixture. Rational elimination decides it.
  const auto quarter=make_ball(gate,{{0,0,0},{2,0,0},{1,1,1}});
  gate.require(quarter.rational.coefficients==oracle::Coefficients{2,-4,-1,-1,0},"corrected quarter-centre key");
  static_cast<void>(query(gate,{{0,0,0},{0,1,0}},quarter,1));
  const auto half=make_ball(gate,{{0,0,0},{1,1,0},{1,0,1},{0,1,1}});
  gate.require(half.rational.coefficients==oracle::Coefficients{1,-1,-1,-1,0},"half-centre tetrahedral key");
  Points cube;
  for (unsigned x=0;x<2;++x) for (unsigned y=0;y<2;++y) for (unsigned z=0;z<2;++z)
    cube.push_back({static_cast<mhgp8::Coordinate>(x),static_cast<mhgp8::Coordinate>(y),static_cast<mhgp8::Coordinate>(z)});
  const auto boundary=query(gate,cube,half,1);
  gate.require(boundary.count_node_visits==1 && boundary.count_nonnegative_sites==8 &&
      boundary.count_inside_sites==0 && boundary.shell_ids==8,"integer-box tangent root was not handled exactly");
  ++gate.integer_grid_contacts;
  Points grid;
  for (unsigned x=0;x<4;++x) for (unsigned y=0;y<4;++y) for (unsigned z=0;z<4;++z)
    grid.push_back({static_cast<mhgp8::Coordinate>(x),static_cast<mhgp8::Coordinate>(y),static_cast<mhgp8::Coordinate>(z)});
  for (std::size_t threshold:{1U,2U,3U,5U,10U,65U}) {
    static_cast<void>(query(gate,grid,fractional,threshold));
    static_cast<void>(query(gate,grid,quarter,threshold));
    static_cast<void>(query(gate,grid,half,threshold));
  }
}

void regimes(Gate& gate) {
  const auto ball=make_ball(gate,{{0,50,50},{100,50,50}});
  Points interior;
  for (unsigned x=40;x<60;++x) interior.push_back({static_cast<mhgp8::Coordinate>(x),50,50});
  const auto overshoot=query(gate,interior,ball,3);
  gate.require(overshoot.count_node_visits==1 && overshoot.count_inside_nodes==1 &&
      overshoot.count_inside_sites==3 && overshoot.count_point_tests==0,"whole-node saturation overshoot fixture");
  ++gate.saturating_overshoot;
  static_cast<void>(query(gate,interior,ball,21));
  const auto small=make_ball(gate,{{90,0,0},{110,0,0}});
  const auto near=query(gate,{{0,0,0},{100,0,0}},small,1);
  gate.require(near.count_node_visits==2 && near.count_bounds_prepared==3 &&
      near.count_prepared_unvisited==1 && near.count_point_tests==2,"near-centre order did not retain charged pending bounds");
  ++gate.near_first_cases;
  for (const auto point:Points{{0,50,50},{50,50,50},{65535,65535,65535},{262143,262143,262143}}) {
    static_cast<void>(query(gate,{point},ball,1));static_cast<void>(query(gate,{point},ball,2));++gate.singleton_cases;
  }
  const auto circle=make_ball(gate,{{35,30,30},{27,34,30},{27,26,30}});
  auto points=shell30();gate.require(points.size()==30,"sphere shell size");
  points.push_back({30,30,30});points.push_back({31,30,30});points.push_back({30,31,30});points.push_back({100,100,100});
  for (std::size_t threshold:{1U,2U,3U,4U,5U,10U}) static_cast<void>(query(gate,points,circle,threshold));
  std::reverse(points.begin(),points.end());static_cast<void>(query(gate,points,circle,4));++gate.permutations;
  static_cast<void>(query(gate,points,circle,std::numeric_limits<std::size_t>::max()));++gate.huge_threshold;
  const Points support{{65535,65534,65533},{0,0,65532},{2,65531,0}};
  const auto wide=make_ball(gate,support);
  for (std::size_t i=1;i<4;++i) if (oracle::bits(wide.rational.coefficients[i]*wide.rational.coefficients[i])>128)
    ++gate.wide_linear_squares;
  Points extreme=support;
  extreme.insert(extreme.end(),{{0,0,0},{65535,65535,65535},{32768,32768,32768},{65535,0,0},{0,65535,0}});
  for (std::size_t threshold:{1U,2U,3U,5U,10U}) {
    static_cast<void>(query(gate,extreme,wide,threshold));++gate.extreme_cases;
  }
  // 18-bit twin: the same acute support carried to the 262143 corner (the
  // 65535 corner and the old midpoint 32768 are now interior sites of it).
  const Points support18{{262143,262142,262141},{0,0,262140},{2,262139,0}};
  const auto wide18=make_ball(gate,support18);
  for (std::size_t i=1;i<4;++i) if (oracle::bits(wide18.rational.coefficients[i]*wide18.rational.coefficients[i])>128)
    ++gate.wide_linear_squares_u18;
  Points extreme18=support18;
  extreme18.insert(extreme18.end(),{{0,0,0},{262143,262143,262143},{131072,131072,131072},{262143,0,0},{0,262143,0},
      {65535,65535,65535},{32768,32768,32768}});
  for (std::size_t threshold:{1U,2U,3U,5U,10U}) {
    static_cast<void>(query(gate,extreme18,wide18,threshold));++gate.extreme_cases_u18;
  }
}

void lifecycle(Gate& gate) {
  auto points=shell30();const auto original=points;
  const auto ball=make_ball(gate,{{35,30,30},{27,34,30},{27,26,30}});
  auto cloud=mhgp8::prepare_cloud(points);auto index=mhgp8::make_q2_cloud_index(cloud);
  Ids shell{777};Work work;bool threw=false;
  try { static_cast<void>(mhgp8::census_q3_ball(*index,ball.product,0,shell,work)); }
  catch (const std::invalid_argument&) { threw=true; }
  gate.require(threw && shell==Ids{777} && work==Work{},"invalid threshold modified shell/work");++gate.invalid_inputs;
  shell.clear();shell.shrink_to_fit();work={};threw=false;
  ball_census_allocation::fail=true;
  try { static_cast<void>(mhgp8::census_q3_ball(*index,ball.product,1,shell,work)); }
  catch (const std::bad_alloc&) { threw=true; }
  ball_census_allocation::fail=false;
  gate.require(threw,"shell allocation failure was not propagated");++gate.allocation_failures;
  Work baseline;Ids ids;
  const auto first=mhgp8::census_q3_ball(*index,ball.product,1,ids,baseline);
  const auto once=baseline;
  const auto second=mhgp8::census_q3_ball(*index,ball.product,1,ids,baseline);
  gate.require(first==second && first.accepted && first.depth==0 && ids.size()==30,"retry/repeat after allocation failure");
  const auto a=std::bit_cast<std::array<u64,26>>(once),b=std::bit_cast<std::array<u64,26>>(baseline);
  for (std::size_t i=0;i<26;++i) gate.require(b[i]==(i<23?2*a[i]:a[i]),"census SUM/MAX accumulation");
  ++gate.repeated_calls;
  Work overflow;overflow.queries=std::numeric_limits<u64>::max();threw=false;
  try { static_cast<void>(mhgp8::census_q3_ball(*index,ball.product,1,ids,overflow)); }
  catch (const std::overflow_error&) { threw=true; }
  gate.require(threw,"work overflow was not checked");++gate.overflow_exceptions;
  const Points just_inside{{30,30,30},{31,30,30}};
  const auto ii=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(just_inside));
  Ids no_shell;Work no_work;
  ball_census_allocation::fail=true;
  const auto rejected=mhgp8::census_q3_ball(*ii,ball.product,1,no_shell,no_work);
  const bool no_allocation=ball_census_allocation::fail;ball_census_allocation::fail=false;
  gate.require(no_allocation && !rejected.accepted && rejected.depth==1 && no_shell.empty(),"rejection allocated a payload");
  ++gate.allocation_free_rejections;
  points[0]={65535,65535,65535};cloud.reset();
  gate.require(std::equal(index->cloud().points().begin(),index->cloud().points().end(),original.begin()),"input alias altered census owner");
  ++gate.source_alias_checks;
  using Parallel=std::tuple<mhgp8::Q3BallCensusResult,Ids,Work>;
  std::vector<std::future<Parallel>> tasks;
  for (unsigned i=0;i<4;++i) tasks.push_back(std::async(std::launch::async,[owned=index,&ball]() {
    Ids local;Work local_work;const auto result=mhgp8::census_q3_ball(*owned,ball.product,1,local,local_work);
    std::sort(local.begin(),local.end());return Parallel{result,std::move(local),local_work};
  }));
  index.reset();
  const auto expected=census(gate,original,ball.rational);
  for (auto& task:tasks) {
    const auto [result,actual,actual_work]=task.get();
    gate.require(result==first && actual==expected.shell && actual_work==once,"independent concurrent census differs");++gate.parallel_calls;
  }
}

void run(Gate& gate) {
  rounding_and_contacts(gate);regimes(gate);lifecycle(gate);
  gate.require(gate.accepted && gate.rejected && gate.whole_inside && gate.whole_nonnegative && gate.prepared_unvisited &&
      gate.shell_exclusions && gate.max_shell>=30 && gate.wide_linear_squares==3 && gate.empty_shells,
      "required census branch was not exercised");
  // ==3 observed from the rational Gram solve of support18 at execution (22 September 2026).
  gate.require(gate.wide_linear_squares_u18==3 && gate.extreme_cases_u18==5,
      "18-bit wide support branch was not exercised");
}
}  // namespace

int main(int argc,char** argv) {
  try {
    if (argc!=2 || std::string_view(argv[1])!="--selftest") throw std::invalid_argument("expected --selftest");
    Gate gate;run(gate);
    std::cout<<"{\"schema\":\"mhgp8_q3_ball_census_gate_v1\",\"status\":\"PASS\"";
#define FIELD(name) std::cout<<",\"" #name "\":"<<gate.name
    FIELD(checks);FIELD(calls);FIELD(oracle_sites);FIELD(accepted);FIELD(rejected);FIELD(q2_balls);FIELD(q3_balls);FIELD(q4_balls);
    FIELD(shell_ids);FIELD(max_shell);FIELD(empty_shells);FIELD(integer_grid_contacts);FIELD(ceil_required);FIELD(floor_required);
    FIELD(whole_inside);FIELD(whole_nonnegative);FIELD(shell_exclusions);FIELD(saturating_overshoot);FIELD(prepared_unvisited);
    FIELD(near_first_cases);FIELD(singleton_cases);FIELD(extreme_cases);FIELD(wide_linear_squares);FIELD(permutations);FIELD(huge_threshold);
    FIELD(invalid_inputs);FIELD(allocation_failures);FIELD(allocation_free_rejections);FIELD(parallel_calls);FIELD(repeated_calls);
    FIELD(source_alias_checks);FIELD(overflow_exceptions);FIELD(wide_linear_squares_u18);FIELD(extreme_cases_u18);
#undef FIELD
    std::cout<<"}\n";return 0;
  } catch (const std::exception& error) {
    ball_census_allocation::fail=false;
    std::cerr<<"q3 ball census gate: "<<error.what()<<'\n';return 1;
  }
}
