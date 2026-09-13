// Independent audit judge. Including the tested translation unit exposes its
// box bounds to this judge; reference classifications below use direct H only.
#include "pipeline/q2_census.cpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

namespace {
using Pair = std::pair<std::size_t, std::size_t>;
using Point = mhgp8::Point3;
using Input = mhgp8::RectangleInput;
using Ids = std::vector<std::size_t>;
using Geometry = std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint64_t>;
struct Payload { Geometry key; Ids interior; Ids shell; bool operator==(const Payload&) const = default; };
using Output = std::map<Pair, Payload>;
struct Totals {
  std::uint64_t cases{}, runs{}, oracle_pairs{}, oracle_point_tests{}, emitted{}, rejected{};
  std::uint64_t shell_ids{}, interior_ids{}, empty_plans{}, positive_cores{}, partial_descriptors{};
  std::uint64_t wide_diameters{}, equal_key_pairs{}, shared_credit{}, shared_accept{}, shared_reject{};
  std::uint64_t splits_after_credit{}, bound_cases{}, bound_samples{}, cursor_reuses{};
} totals;
void require(bool good, const char* reason) { if (!good) throw std::runtime_error(reason); }
std::int64_t direct_h(const Point& a, const Point& b, const Point& z) {
  std::int64_t value=0;
  for (std::size_t d=0;d<3;++d) value+=(static_cast<std::int64_t>(z[d])-a[d])*(static_cast<std::int64_t>(b[d])-z[d]);
  return value;
}
Geometry geometry(const Point& a, const Point& b) {
  std::uint64_t diameter=0;
  for (std::size_t d=0;d<3;++d) {const auto delta=static_cast<std::int64_t>(a[d])-b[d];diameter+=static_cast<std::uint64_t>(delta*delta);}
  return {static_cast<std::uint32_t>(a.x)+b.x,static_cast<std::uint32_t>(a.y)+b.y,static_cast<std::uint32_t>(a.z)+b.z,diameter};
}
Payload direct(const Input& input,std::size_t a,std::size_t b) {
  Payload result{geometry(input.points[a],input.points[b]),{}, {}};
  for (std::size_t z=0;z<input.points.size();++z) {
    const auto value=direct_h(input.points[a],input.points[b],input.points[z]);
    if(value>0) result.interior.push_back(z);
    if(value==0) result.shell.push_back(z);
    ++totals.oracle_point_tests;
  }
  ++totals.oracle_pairs;
  return result;
}
void normalize(Ids& ids,std::size_t n) {
  std::sort(ids.begin(),ids.end());
  require(std::adjacent_find(ids.begin(),ids.end())==ids.end(),"duplicate census ID");
  require(ids.empty() || ids.back()<n,"census ID exceeds cloud");
}
Output check(const Input& input,unsigned kmax,unsigned separation=12,
             mhgp8::AxisQ2Mode axis_mode=mhgp8::AxisQ2Mode::Independent,bool restrict=false) {
  using namespace mhgp8;
  const auto owner=prepare_rectangle(input,kmax,separation);
  const auto local=make_credit_plan(owner,Lane::Q2,Strategy::Pool);
  const auto plan=make_axis_q2_plan(owner,axis_mode,restrict?&local:nullptr);
  const auto index=make_q2_census_index(owner);
  if(owner->core_credit(Lane::Q2)>0)++totals.positive_cores;
  if(plan.candidate_pairs()==0)++totals.empty_plans;
  for(const auto& block:plan.blocks()) if(block.b.size()!=input.b.size()) ++totals.partial_descriptors;
  Output expected;
  std::uint64_t candidates=0;
  for(std::size_t a=input.a.first;a<input.a.last;++a)for(std::size_t b=input.b.first;b<input.b.last;++b) {
    auto ref=direct(input,a,b);
    const bool keep=plan.keeps(a,b);
    candidates+=keep;
    if(ref.interior.size()<kmax) {
      require(keep,"prefilter lost a useful support");
      if(std::get<3>(ref.key)>UINT64_C(4294967295))++totals.wide_diameters;
      expected.emplace(Pair{a,b},std::move(ref));
    }
  }
  require(candidates==plan.candidate_pairs(),"prefilter pair count");
  std::set<Geometry> unique_keys;
  for(const auto& [pair,payload]:expected) {static_cast<void>(pair);if(!unique_keys.insert(payload.key).second)++totals.equal_key_pairs;}
  Output prior;
  for(const auto mode:{Q2CensusMode::Pairwise,Q2CensusMode::SharedBlocks}) {
    Output actual;
    std::uint64_t interior_ids=0,shell_ids=0;
    const auto result=run_q2_census(*index,plan,mode,[&](const Q2Support& support) {
      const Pair pair{support.a_id,support.b_id};
      Payload got{{support.key.center_twice[0],support.key.center_twice[1],support.key.center_twice[2],support.key.diameter_squared},
                  {support.interior.begin(),support.interior.end()},{support.shell.begin(),support.shell.end()}};
      normalize(got.interior,input.points.size());normalize(got.shell,input.points.size());
      const auto found=expected.find(pair);
      require(found!=expected.end(),"census emitted an unexpected support");
      require(got==found->second,"exact interior/shell/key differ from direct H");
      require(std::binary_search(got.shell.begin(),got.shell.end(),pair.first) && std::binary_search(got.shell.begin(),got.shell.end(),pair.second),"support endpoints missing from shell");
      interior_ids+=got.interior.size();shell_ids+=got.shell.size();
      require(actual.emplace(pair,std::move(got)).second,"support emitted twice");
    });
    require(actual==expected,"census omitted a support");
    require(result.candidate_pairs==candidates && result.accepted_pairs==expected.size() && result.rejected_pairs==candidates-expected.size(),"wrong census result partition");
    require(result.work.payload_supports==actual.size() && result.work.payload_interior_sites==interior_ids && result.work.payload_shell_sites==shell_ids,"payload counters differ from copied IDs");
    require(result.work.frontier_restarts==0,"nonzero count restarted at root");
    if(candidates==0)require(result.work.count_node_visits==0 && result.work.query_tasks==0 && result.work.payload_node_visits==0,"empty residual paid census work");
    if(mode==Q2CensusMode::SharedBlocks) {
      require(actual==prior,"two modes disagree");
      require(result.work.cursor_reuses==2*result.work.query_splits,"query split cursor inheritance");
      totals.shared_credit+=result.work.uniform_credited_pairs;
      totals.shared_accept+=result.work.uniform_accepted_pairs;
      totals.shared_reject+=result.work.uniform_rejected_pairs;
      totals.splits_after_credit+=result.work.shared_splits_after_credit;
      totals.cursor_reuses+=result.work.cursor_reuses;
    } else prior=actual;
    totals.emitted+=actual.size();totals.rejected+=result.rejected_pairs;
    totals.interior_ids+=interior_ids;totals.shell_ids+=shell_ids;++totals.runs;
  }
  ++totals.cases;
  return expected;
}
Input changed(Input input,unsigned code) {
  const std::array<std::array<unsigned,3>,6> permutations{{{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}}};
  for(auto& p:input.points) {
    const auto old=p;std::array<std::uint16_t,3> values{};
    for(unsigned d=0;d<3;++d){const auto v=old[permutations[code%6][d]];values[d]=static_cast<std::uint16_t>((code/6)&(1U<<d)?65535-v:v);}
    p={values[0],values[1],values[2]};
  }
  if(code%2) {
    const auto n=input.points.size();std::reverse(input.points.begin(),input.points.end());
    input.a={n-input.a.last,n-input.a.first};input.b={n-input.b.last,n-input.b.first};
    for(auto& id:input.core_candidates)id=n-1-id;
  }
  return input;
}
std::pair<std::int64_t,std::int64_t> scalar_grid(std::int64_t a,std::int64_t bl,std::int64_t bh,std::int64_t zl,std::int64_t zh) {
  std::int64_t minimum=INT64_MAX,maximum=INT64_MIN;
  for(auto b=bl;b<=bh;++b)for(auto z4=4*zl;z4<=4*zh;++z4) {
    const auto h16=(z4-4*a)*(4*b-z4);
    minimum=std::min(minimum,h16);maximum=std::max(maximum,h16);++totals.bound_samples;
  }
  return {minimum,maximum};
}
void bounds() {
  for(unsigned a=0;a<=4;++a)for(unsigned bl=0;bl<=4;++bl)for(unsigned bh=bl;bh<=4;++bh)
    for(unsigned zl=0;zl<=4;++zl)for(unsigned zh=zl;zh<=4;++zh) {
      const auto reference=scalar_grid(a,bl,bh,zl,zh);
      for(unsigned axis=0;axis<3;++axis) {
        const auto p=[axis](unsigned value){std::array<std::uint16_t,3> v{};v[axis]=static_cast<std::uint16_t>(value);return Point{v[0],v[1],v[2]};};
        const mhgp8::Box3 b{p(bl),p(bh)},z{p(zl),p(zh)};
        const auto got=mhgp8::shared_bounds(p(a),b,z);
        require(4*got.minimum4==reference.first && 4*got.maximum4==reference.second,"shared extrema differ from exhaustive rational grid");
        if(bl==bh) {
          const auto pair=mhgp8::pair_bounds(mhgp8::ball_key(p(a),p(bl)),z);
          require(pair.minimum4==got.minimum4 && pair.maximum4==got.maximum4,"singleton bounds differ from rational grid");
        }
        ++totals.bound_cases;
      }
    }
}
Input grid(unsigned ny,unsigned nz,bool outsider) {
  Input input;input.a.first=0;
  for(unsigned y=0;y<ny;++y)for(unsigned z=0;z<nz;++z)input.points.push_back({1000,static_cast<std::uint16_t>(1000+3*y),static_cast<std::uint16_t>(1000+3*z)});
  input.a.last=input.points.size();input.b.first=input.points.size();
  for(unsigned y=0;y<ny;++y)for(unsigned z=0;z<nz;++z)input.points.push_back({60000,static_cast<std::uint16_t>(1000+3*y),static_cast<std::uint16_t>(1000+3*z)});
  input.b.last=input.points.size();
  if(outsider){input.points.push_back({31000,1001,1001});input.points.push_back({0,65535,65535});}
  return input;
}
void fixtures() {
  const std::vector<Input> small{
    {{{0,1,1},{3,2,2},{1,0,1},{2,3,2},{1,1,1},{1,1,4}},{0,1},{1,2},{}},
    {{{0,1,1},{3,2,2},{1,0,1},{2,3,2},{1,1,1},{1,1,4}},{0,1},{1,2},{4,5}},
    {{{0,2,2},{4,2,2},{2,4,2},{2,5,2}},{0,1},{1,2},{}},
    {{{0,2,2},{4,2,2},{2,0,0},{2,4,4},{2,2,2}},{0,1},{1,2},{}},
    {{{1000,0,0},{60000,0,0},{60000,2,0},{30000,1,0},{60000,1,0}},{0,1},{1,3},{}},
    {{{1000,1000,1000},{60000,999,1000},{60000,1001,1000},{60000,998,1000},{60000,1002,1000}},{0,1},{1,3},{}},
    {{{1000,1,0},{60000,1,0},{60000,3,0},{1000,2,0}},{0,1},{1,3},{}},
  };
  for(const auto& input:small)for(unsigned code=0;code<12;++code)for(unsigned k:{1U,2U,5U,10U})
    static_cast<void>(check(changed(input,code),k,8+2*(code%3)));
  for(unsigned offset:{10U,1000U,65000U}) {
    auto translated=small.front();
    for(auto& p:translated.points)p={static_cast<std::uint16_t>(p.x+offset),static_cast<std::uint16_t>(p.y+offset),static_cast<std::uint16_t>(p.z+offset)};
    for(unsigned k:{1U,2U})static_cast<void>(check(translated,k));
  }
  Input two{{{0,1,1},{1,0,1},{3,2,2},{2,3,2},{1,1,1},{2,2,2},{1,1,0},{1,1,4}},{0,2},{2,4},{}};
  for(unsigned code=0;code<6;++code)static_cast<void>(check(changed(two,code),3,1));
  Input extreme;
  for(unsigned mask=0;mask<8;++mask)extreme.points.push_back({static_cast<std::uint16_t>((mask&1)?65535:0),static_cast<std::uint16_t>((mask&2)?65535:0),static_cast<std::uint16_t>((mask&4)?65535:0)});
  extreme.points.push_back({32767,32767,32767});extreme.a={0,1};extreme.b={7,8};
  for(unsigned code=0;code<6;++code)for(unsigned k:{1U,2U,10U})static_cast<void>(check(changed(extreme,code),k));
  for(unsigned y=2;y<=5;++y)for(unsigned k:{1U,2U,5U,10U})for(unsigned code=0;code<3;++code)
    static_cast<void>(check(changed(grid(y,3,code!=0),code),k,12,code==0?mhgp8::AxisQ2Mode::Independent:mhgp8::AxisQ2Mode::Additive,code==2));
  Input sphere;
  for(int x=-9;x<=9;++x)for(int y=-9;y<=9;++y)for(int z=-9;z<=9;++z)if(x*x+y*y+z*z==81) {
    if(x==-9)sphere.a={sphere.points.size(),sphere.points.size()+1};
    if(x==9)sphere.b={sphere.points.size(),sphere.points.size()+1};
    sphere.points.push_back({static_cast<std::uint16_t>(20+x),static_cast<std::uint16_t>(20+y),static_cast<std::uint16_t>(20+z)});
  }
  const auto wide=check(sphere,1);require(wide.begin()->second.shell.size()>30,"large shell nonvacuity");
  Input shared{{{1000,1000,1000}},{0,1},{1,33},{}};
  for(unsigned i=0;i<32;++i)shared.points.push_back({60000,static_cast<std::uint16_t>(1000+i),1000});
  shared.points.push_back({30000,1000,1000});
  for(unsigned k:{1U,5U,10U})static_cast<void>(check(shared,k));
  std::uint64_t random=UINT64_C(0x54a9e8d0b7731c2f);
  const auto next=[&](){random^=random<<13;random^=random>>7;random^=random<<17;return random;};
  for(unsigned trial=0;trial<48;++trial) {
    Input input;std::set<std::tuple<unsigned,unsigned,unsigned>> used;
    const auto add=[&](unsigned x,unsigned y,unsigned z){if(!used.emplace(x,y,z).second)return false;input.points.push_back({static_cast<std::uint16_t>(x),static_cast<std::uint16_t>(y),static_cast<std::uint16_t>(z)});return true;};
    static_cast<void>(add(0,65535,0));input.a.first=input.points.size();
    while(input.points.size()<input.a.first+3+trial%5) {
      const auto x=1000+next()%3,y=1000+next()%19,z=1000+next()%19;
      static_cast<void>(add(x,y,z));
    }
    input.a.last=input.points.size();static_cast<void>(add(65535,0,65535));input.b.first=input.points.size();
    while(input.points.size()<input.b.first+4+trial%7) {
      const auto x=60000+next()%3,y=1000+next()%19,z=1000+next()%19;
      static_cast<void>(add(x,y,z));
    }
    input.b.last=input.points.size();
    if(trial%3){static_cast<void>(add(30000,1000+trial,1000));if(trial%5==0)input.core_candidates.push_back(input.points.size()-1);}
    static_cast<void>(check(changed(input,trial%12),std::array<unsigned,4>{1,2,5,10}[trial%4]));
  }
}
void run() {
  bounds();fixtures();
  require(totals.cases==466 && totals.runs==932 && totals.oracle_pairs>7000 && totals.oracle_point_tests>150000,"campaign nonvacuity");
  require(totals.emitted>1000 && totals.rejected>1000 && totals.shell_ids>1000 && totals.interior_ids>1000,"payload nonvacuity");
  require(totals.empty_plans>0 && totals.positive_cores>0 && totals.partial_descriptors>0,"prefilter branch nonvacuity");
  require(totals.wide_diameters>0 && totals.equal_key_pairs>0 && totals.splits_after_credit>0 && totals.shared_credit>0 && totals.shared_accept>0 && totals.shared_reject>0,"shared branch nonvacuity");
  std::cout<<"{\"status\":\"passed\",\"scope\":\"bounded_cpp_q2_census_direct_H_judge\"";
#define MHGP8_AUDIT_FIELD(name) std::cout<<",\"" #name "\":"<<totals.name
  MHGP8_AUDIT_FIELD(cases);MHGP8_AUDIT_FIELD(runs);MHGP8_AUDIT_FIELD(oracle_pairs);MHGP8_AUDIT_FIELD(oracle_point_tests);MHGP8_AUDIT_FIELD(emitted);MHGP8_AUDIT_FIELD(rejected);MHGP8_AUDIT_FIELD(shell_ids);MHGP8_AUDIT_FIELD(interior_ids);MHGP8_AUDIT_FIELD(empty_plans);MHGP8_AUDIT_FIELD(positive_cores);MHGP8_AUDIT_FIELD(partial_descriptors);MHGP8_AUDIT_FIELD(wide_diameters);MHGP8_AUDIT_FIELD(equal_key_pairs);MHGP8_AUDIT_FIELD(shared_credit);MHGP8_AUDIT_FIELD(shared_accept);MHGP8_AUDIT_FIELD(shared_reject);MHGP8_AUDIT_FIELD(splits_after_credit);MHGP8_AUDIT_FIELD(bound_cases);MHGP8_AUDIT_FIELD(bound_samples);MHGP8_AUDIT_FIELD(cursor_reuses);
#undef MHGP8_AUDIT_FIELD
  std::cout<<"}\n";
}
} // namespace
int main(int argc,char** argv) {
  if(argc!=2 || std::string_view(argv[1])!="--selftest")return 2;
  try {run();return 0;}catch(const std::exception& error){std::cerr<<"q2 independent audit: "<<error.what()<<'\n';return 1;}
}
