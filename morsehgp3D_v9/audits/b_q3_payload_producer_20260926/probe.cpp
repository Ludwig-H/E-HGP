#include "producer.hpp"
#include "../../src/gen/lanes/exact_ball.hpp"
#include "../../src/tower/forest/ball_data.hpp"
#include "../../src/tower/lanes/q3.hpp"
#include "../../src/tower/pipeline/census.hpp"
#include "../../tests/gen/front_fixtures.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace g = mhgp9::gpu;
namespace p = mhgp9::audit_q3_payload;
namespace gen = mhgp9::gen;
using g::u32;
using g::u64;
using Clock = std::chrono::steady_clock;
void require(bool okay, const char* cause) { if (!okay) throw std::runtime_error(cause); }
double ms(Clock::time_point t) { return std::chrono::duration<double, std::milli>(Clock::now()-t).count(); }

// Independent host median tree; not a shadow of the product index. The
// census and its cover see the exact input through this tree. ID != rank.
struct Flat {
  std::vector<gen::Point3> input;
  std::vector<g::FlatNode> nodes;
  std::vector<u32> order, inverse, escapes;
  std::vector<std::int32_t> points;
  explicit Flat(std::vector<gen::Point3> values) : input(std::move(values)), order(input.size()), inverse(input.size()) {
    require(!input.empty() && input.size() < g::absent32 / 2, "flat_domain");
    std::iota(order.begin(), order.end(), 0U);
    nodes.reserve(2*input.size());
    escapes.reserve(2*input.size());
    build(0, static_cast<u32>(input.size()), 0);
    for (u32 r=0; r<order.size(); ++r) {
      inverse[order[r]]=r;
      const auto q=input[order[r]];
      points.push_back(static_cast<std::int32_t>(q.x));
      points.push_back(static_cast<std::int32_t>(q.y));
      points.push_back(static_cast<std::int32_t>(q.z));
    }
  }
  u32 build(u32 lo,u32 hi,unsigned depth) {
    const u32 at=static_cast<u32>(nodes.size());
    g::FlatNode node{};
    node.first=lo; node.last=hi; node.left=node.right=g::absent32;
    node.box.low[0]=node.box.low[1]=node.box.low[2]=262143;
    node.box.high[0]=node.box.high[1]=node.box.high[2]=0;
    for(u32 i=lo;i<hi;++i) {
      const auto v=input[order[i]];
      const std::int32_t q[3]={static_cast<std::int32_t>(v.x),static_cast<std::int32_t>(v.y),static_cast<std::int32_t>(v.z)};
      for(unsigned a=0;a<3;++a) { node.box.low[a]=std::min(node.box.low[a],q[a]); node.box.high[a]=std::max(node.box.high[a],q[a]); }
    }
    nodes.push_back(node); escapes.push_back(0);
    if(hi-lo>1) {
      const unsigned axis=depth%3;
      const u32 mid=lo+(hi-lo)/2;
      const auto coord=[&](u32 id) { const auto q=input[id]; return axis==0?q.x:axis==1?q.y:q.z; };
      std::nth_element(order.begin()+lo,order.begin()+mid,order.begin()+hi,[&](u32 a,u32 b) {
        return coord(a)!=coord(b)?coord(a)<coord(b):a<b;
      });
      const u32 left=build(lo,mid,depth+1),right=build(mid,hi,depth+1);
      nodes[at].left=left; nodes[at].right=right;
    }
    escapes[at]=static_cast<u32>(nodes.size());
    return at;
  }
  g::LanesIndex view() const { return {{nodes.data(),escapes.data(),static_cast<u32>(nodes.size()),points.data()},order.data()}; }
};

struct Slab {
  std::vector<u32> ranges,ranks,seeds,scratch;
  std::vector<std::int32_t> points;
  std::vector<g::LaneRecord> records;
  std::vector<p::InteriorIds> ids;
  std::array<u32,8> payload_scratch{};
  explicit Slab(u32 n) : ranges(2*std::size_t{n}),ranks(n),seeds(n),scratch(n),points(3*std::size_t{n}),records(n),ids(n) {}
  g::LanesSlab view() { return {ranges.data(),points.data(),ranks.data(),seeds.data(),scratch.data(),records.data(),static_cast<u32>(ranks.size()),static_cast<u32>(records.size())}; }
  p::PayloadSlab payload() { return {payload_scratch.data(),ids.data()}; }
};

struct Coverage { u64 calls=0,records=0,ids=0,extra_shell=0,rejected=0,partial_discarded=0,permuted=0,oracle_points=0,consumer_compared=0,max_depth=0; } coverage;

bool records_equal(const g::LaneRecord& a,const g::LaneRecord& b) {
  return std::equal(a.key,a.key+5,b.key) && std::equal(a.support,a.support+4,b.support) &&
    a.edge==b.edge && a.depth==b.depth && a.shell==b.shell && a.arity==b.arity && a.shell_sum==b.shell_sum && a.shell_xor==b.shell_xor;
}
bool census_work_equal(const g::EdgeQ3Work& a,const g::EdgeQ3Work& b) {
  return a.census_point_tests==b.census_point_tests && a.census_inside_sites==b.census_inside_sites &&
    a.census_shell_sites==b.census_shell_sites && a.census_outside_sites==b.census_outside_sites &&
    a.depth_rejections==b.depth_rejections && a.emitted==b.emitted && a.shell_ids==b.shell_ids;
}

void judge_consumer(const Flat& flat,const g::LaneRecord& r,const p::InteriorIds& payload) {
  namespace t=mhgp9::tower;
  if(r.shell!=3) return;  // Complete shell IDs were not carried: global fallback is mandatory.
  std::vector<t::InputPoint> input;
  for(u32 id=0;id<flat.input.size();++id) {
    const auto q=flat.input[id]; input.push_back({id,{q.x,q.y,q.z}});
  }
  const auto index=t::build_cloud_index(input);
  require(index.valid && !index.has_duplicate_positions(),"tower_index_invalid");
  std::vector<t::i32> ranks(input.size());
  for(std::size_t u=0;u<index.upos.size();++u) ranks[index.point_id(static_cast<t::i32>(u))]=static_cast<t::i32>(u);
  const auto a=index.upos[static_cast<std::size_t>(ranks[r.support[0]])];
  const auto b=index.upos[static_cast<std::size_t>(ranks[r.support[1]])];
  const auto c=index.upos[static_cast<std::size_t>(ranks[r.support[2]])];
  t::BallData imported{};
  imported.key=t::q3_ball_key(t::q3_form(a,b,c));
  imported.level=t::promote_level(t::q3_exact_level(a,b,c));
  imported.arity=3; imported.n_shell=3; imported.n_interior=static_cast<t::u8>(r.depth);
  for(u32 i=0;i<r.depth;++i) {
    require(payload.ids[i]<input.size(),"consumer_id_domain");
    const auto rank=ranks[payload.ids[i]];
    require(imported.key.power(index.upos[static_cast<std::size_t>(rank)])<0,"consumer_noninterior");
    imported.interior_ids[i]=rank;
  }
  for(u32 i=0;i<3;++i) imported.shell_ids[i]=ranks[r.support[i]];
  std::sort(imported.interior_ids,imported.interior_ids+r.depth);
  std::sort(imported.shell_ids,imported.shell_ids+3);
  require(std::adjacent_find(imported.interior_ids,imported.interior_ids+r.depth)==imported.interior_ids+r.depth,"consumer_duplicate");
  // Only the independent judge pays this census. The producer above never
  // calls it, and the importing path above only visits O(K) supplied IDs.
  std::vector<t::i32> inside,shell;
  const auto status=t::ball_census(index,imported.key,9,12,&inside,&shell);
  require(status==t::CensusStatus::kOk,"consumer_census_status");
  std::sort(inside.begin(),inside.end());std::sort(shell.begin(),shell.end());
  t::BallData expected{};
  expected.key=imported.key;expected.level=imported.level;expected.arity=3;
  expected.n_interior=static_cast<t::u8>(inside.size());expected.n_shell=static_cast<t::u8>(shell.size());
  std::copy(inside.begin(),inside.end(),expected.interior_ids);std::copy(shell.begin(),shell.end(),expected.shell_ids);
  require(expected.key==imported.key && expected.level==imported.level && expected.arity==imported.arity &&
          expected.n_interior==imported.n_interior && expected.n_shell==imported.n_shell &&
          std::equal(inside.begin(),inside.end(),imported.interior_ids) &&
          std::equal(shell.begin(),shell.end(),imported.shell_ids),"consumer_BallData_differs");
  ++coverage.consumer_compared;
}

void judge_record(const Flat& flat,const g::LaneRecord& r,const p::InteriorIds& payload) {
  require(r.arity==3 && r.depth<=8,"record_domain");
  const auto ball=gen::ExactBall::make_q3({flat.input[r.support[0]],flat.input[r.support[1]],flat.input[r.support[2]]});
  require(ball.has_value(),"oracle_nonpositive");
  const auto& key=ball->coefficients();
  for(unsigned j=0;j<5;++j) require(r.key[j]==key[j],"oracle_key");
  std::vector<u32> expect,actual;
  u32 shell=0; u64 sum=0,xor_sum=0;
  // Independent global scan of every original point, not the candidate's
  // cover or scan-order IDs. ExactBall uses primitive global coefficients.
  for(u32 id=0;id<flat.input.size();++id) {
    const auto v=ball->power(flat.input[id]); ++coverage.oracle_points;
    if(v<0) expect.push_back(id);
    if(v==0) { ++shell; const auto h=g::mix64(id); sum+=h; xor_sum^=h; }
  }
  for(u32 j=0;j<8;++j) {
    if(j<r.depth) actual.push_back(payload.ids[j]);
    else require(payload.ids[j]==g::absent32,"unused_payload_not_empty");
  }
  std::sort(actual.begin(),actual.end());
  require(actual==expect,"interior_ids_differ");
  require(r.depth==expect.size() && r.shell==shell && r.shell_sum==sum && r.shell_xor==xor_sum,"oracle_counts");
  require(std::adjacent_find(actual.begin(),actual.end())==actual.end(),"duplicate_id");
  coverage.extra_shell+=r.shell>3; coverage.records++; coverage.ids+=r.depth;
  coverage.max_depth=std::max(coverage.max_depth,u64{r.depth});
  judge_consumer(flat,r,payload);
}

struct PairResult { g::EdgeQ3Work work{}; p::PayloadWork payload{}; double old_ms=0,new_ms=0; u32 records=0,sites=0,seeds=0; };

PairResult compare(const Flat& flat,u32 a,u32 b,unsigned k,bool global_judge=true,
                   const std::vector<u32>* explicit_order=nullptr,u32 single_seed=0,bool producer_first=false) {
  Slab original(static_cast<u32>(flat.input.size()));
  u32 sites=0,seeds=0;
  g::EdgeQ3Work prep{};
  if(explicit_order==nullptr) {
    const auto st=g::lanes_prologue(g::HostGroup{},flat.view(),flat.inverse[a],flat.inverse[b],original.view(),sites,seeds,prep);
    require(st==g::CertificateStatus::decided,"prologue_not_decided");
  } else {
    sites=static_cast<u32>(explicit_order->size()); seeds=1;
    for(u32 i=0;i<sites;++i) {
      const u32 rank=flat.inverse[(*explicit_order)[i]];
      original.ranks[i]=rank;
      std::copy_n(flat.points.data()+3*std::size_t{rank},3,original.points.data()+3*std::size_t{i});
      if((*explicit_order)[i]==single_seed) original.seeds[0]=i;
    }
  }
  Slab candidate=original;
  for(auto& slot:candidate.ids) for(auto& id:slot.ids) id=0xa5a5a5a5U;
  u32 old_count=0,new_count=0;
  g::EdgeQ3Work old_work{},new_work{};
  p::PayloadWork extra{};
  double old_ms=0,new_ms=0;
  g::CertificateStatus old_status{},new_status{};
  const auto run_old=[&] {
    const auto t=Clock::now();
    old_status=g::q3_census_range(g::HostGroup{},flat.view(),flat.inverse[a],flat.inverse[b],k,original.view(),sites,0,seeds,old_count,old_work);
    old_ms=ms(t);
  };
  const auto run_new=[&] {
    const auto t=Clock::now();
    new_status=p::census_range(g::HostGroup{},flat.view(),flat.inverse[a],flat.inverse[b],k,candidate.view(),candidate.payload(),sites,0,seeds,new_count,new_work,extra);
    new_ms=ms(t);
  };
  if(producer_first) { run_new();run_old(); } else { run_old();run_new(); }
  require(old_status==new_status && new_status==g::CertificateStatus::decided,"status_differs");
  require(old_count==new_count && census_work_equal(old_work,new_work),"census_differs");
  for(u32 i=0;i<old_count;++i) {
    require(records_equal(original.records[i],candidate.records[i]),"record_differs");
    if(global_judge) judge_record(flat,candidate.records[i],candidate.ids[i]);
  }
  for(std::size_t i=new_count;i<candidate.ids.size();++i)
    for(u32 id:candidate.ids[i].ids) require(id==0xa5a5a5a5U,"rejected_payload_published");
  require(extra.output_writes==8*u64{new_count},"output_work");
  ++coverage.calls; coverage.rejected+=new_work.depth_rejections; coverage.partial_discarded+=extra.rejected_scratch_writes;
  for(u32 i=0;i<flat.order.size();++i) coverage.permuted+=flat.order[i]!=i;
  return {new_work,extra,old_ms,new_ms,new_count,sites,seeds};
}

std::vector<gen::Point3> small_cloud(bool extra_shell) {
  std::vector<gen::Point3> x={{88,100,100},{112,100,100},{100,116,100},{100,104,100},{100,105,100},{101,104,100}};
  if(extra_shell) { x.push_back({100,91,100}); x.push_back({110,111,100}); x.push_back({90,96,100}); }
  x.push_back({1000,1000,1000});
  return x;
}

void stress_chunks() {
  // Deliberately supplied fixed census order, not an altered prologue.
  // All original sites are included; interiors cross lanes 31,32,33,63,64.
  for(const u32 n:{31U,32U,33U,63U,64U,65U,66U}) {
    std::vector<gen::Point3> x={{88,100,100},{112,100,100},{100,116,100}};
    while(x.size()<n) x.push_back({static_cast<gen::Coordinate>(1000+x.size()),1000,1000});
    std::vector<u32> positions;
    for(u32 pos:{3U,4U,31U,32U,33U,34U,63U,64U}) if(pos<n) {
      x[pos]={static_cast<gen::Coordinate>(100+positions.size()),104,100}; positions.push_back(pos);
    }
    Flat flat(x);
    std::vector<u32> order(n); std::iota(order.begin(),order.end(),0U);
    for(unsigned k:{2U,3U,5U,10U}) compare(flat,0,1,k,true,&order,2);
  }
}

void selftest() {
  for(bool shell:{false,true}) {
    auto x=small_cloud(shell);
    for(unsigned rotation=0;rotation<3;++rotation) {
      for(auto& point:x) {const auto a=point.x;point.x=point.y;point.y=point.z;point.z=a;}
      Flat flat(x);
      for(unsigned k:{2U,3U,5U,10U}) for(u32 a=0;a<3;++a) for(u32 b=a+1;b<3;++b) compare(flat,a,b,k);
    }
  }
  stress_chunks();
  for(const std::string family:{"uniform","terrain","clusters"}) {
    Flat flat(gen::bench::make_front_fixture(96,family,71).points);
    for(unsigned k:{2U,3U,5U,10U}) for(u32 r=0;r<32;++r) compare(flat,flat.order[r],flat.order[r+1],k);
  }
  require(coverage.records>0 && coverage.ids>0 && coverage.extra_shell>0 && coverage.rejected>0 && coverage.partial_discarded>0 && coverage.permuted>0 && coverage.max_depth==8 && coverage.consumer_compared>0,"coverage_vacuous");
  std::cout<<"{\"status\":\"pass\",\"calls\":"<<coverage.calls<<",\"records\":"<<coverage.records<<",\"interior_ids\":"<<coverage.ids
    <<",\"extra_shell_records\":"<<coverage.extra_shell<<",\"rejected_seeds\":"<<coverage.rejected<<",\"discarded_prefix_ids\":"<<coverage.partial_discarded
    <<",\"nonidentity_ranks\":"<<coverage.permuted<<",\"oracle_point_tests\":"<<coverage.oracle_points<<",\"consumer_BallData_compared\":"<<coverage.consumer_compared<<",\"max_depth\":"<<coverage.max_depth<<"}\n";
}

void benchmark(std::vector<gen::Point3> x,const std::string& label,unsigned k,unsigned repeats) {
  const auto n=x.size(); auto t=Clock::now(); Flat flat(std::move(x)); const double prep=ms(t);
  // Consecutive spatial ranks, stratified over the entire cloud: a bounded
  // edge diagnostic, never the WSPD's complete admitted edge stream.
  const u32 samples=128;
  for(unsigned rep=0;rep<repeats;++rep) {
    double old_ms=0,new_ms=0; u64 point_tests=0,seeds=0,rejected=0,records=0,cover=0,writes=0,discarded=0,syncs=0;
    for(u32 i=0;i<samples;++i) {
      const u32 rank=static_cast<u32>((u64{i}*(n-1))/samples);
      const auto result=compare(flat,flat.order[rank],flat.order[rank+1],k,rep==0 && i<4,nullptr,0,rep%4==1 || rep%4==2);
      old_ms+=result.old_ms;new_ms+=result.new_ms;point_tests+=result.work.census_point_tests;
      seeds+=result.seeds; rejected+=result.work.depth_rejections;records+=result.records;cover+=result.sites;
      writes+=result.payload.scratch_writes;discarded+=result.payload.rejected_scratch_writes;syncs+=result.payload.payload_syncs;
    }
    std::cout<<"{\"status\":\"equal\",\"scope\":\"128_stratified_adjacent_rank_edges_not_full\",\"label\":\""<<label<<"\",\"n\":"<<n<<",\"k\":"<<k<<",\"repeat\":"<<rep
      <<",\"producer_first\":"<<(rep%4==1 || rep%4==2 ? "true" : "false")<<",\"index_ms_excluded\":"<<prep<<",\"old_census_ms\":"<<old_ms<<",\"payload_census_ms\":"<<new_ms<<",\"cover_sites\":"<<cover<<",\"seeds\":"<<seeds
      <<",\"point_tests_both\":"<<point_tests<<",\"rejected\":"<<rejected<<",\"records\":"<<records<<",\"scratch_id_writes\":"<<writes
      <<",\"discarded_prefix_ids\":"<<discarded<<",\"extra_syncs\":"<<syncs<<",\"output_bytes\":"<<32*records<<"}\n";
  }
}

std::vector<gen::Point3> read_cloud(const std::string& path) {
  std::ifstream in(path,std::ios::binary);require(static_cast<bool>(in),"input_open");
  std::vector<gen::Point3> points; unsigned char bytes[12];
  while(in.read(reinterpret_cast<char*>(bytes),12)) {
    u32 v[3]{};
    for(unsigned a=0;a<3;++a) for(unsigned b=0;b<4;++b) v[a]|=u32{bytes[4*a+b]}<<(8*b);
    for(auto value:v) require(value<=262143,"input_domain");
    points.push_back({static_cast<gen::Coordinate>(v[0]),static_cast<gen::Coordinate>(v[1]),static_cast<gen::Coordinate>(v[2])});
  }
  require(in.eof() && in.gcount()==0 && points.size()>=2,"input_size");return points;
}
}  // namespace

int main(int argc,char** argv) {
  try {
    std::cout<<std::setprecision(9);
    if(argc==2 && std::string(argv[1])=="--selftest") selftest();
    else if(argc==4 && std::string(argv[1])=="--synthetic") {
      const unsigned n=static_cast<unsigned>(std::stoul(argv[2]));
      const std::string family=argv[3]; benchmark(gen::bench::make_front_fixture(n,family,731).points,family,5,4);
    } else if(argc==4 && std::string(argv[1])=="--file") {
      const unsigned k=static_cast<unsigned>(std::stoul(argv[3]));require(k>=2 && k<=10,"K_domain");
      benchmark(read_cloud(argv[2]),"lidar_nonground_grid_1mm",k,4);
    } else { std::cerr<<"usage: --selftest | --synthetic N FAMILY | --file CLOUD.u32le K\n";return 2; }
    return 0;
  } catch(const std::exception& e) { std::cerr<<"cause="<<e.what()<<'\n';return 1; }
}
