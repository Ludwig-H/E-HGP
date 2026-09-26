// Opt-in q3 interior sidecar: producer, fused task layout, staging, gather,
// pinned record ownership and deferrals. The old path judges records/work;
// a GLOBAL exact census judges IDs, including extra-shell contacts.
#include "../../src/gpu/flat_index.hpp"
#include "../../src/gpu/lanes_host.hpp"
#include "../../src/gen/lanes/exact_ball.hpp"
#include "../gen/front_fixtures.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace g = mhgp9::gpu;
namespace gen = mhgp9::gen;
using g::u32;
using g::u64;
void need(bool yes,const char* cause) { if(!yes) throw std::runtime_error(cause); }

struct Flat {
  std::vector<gen::Point3> input;
  gen::Q2CensusIndexPtr owner;
  std::vector<g::FlatNode> nodes;
  std::vector<u32> escapes,ids,inverse,a,b;
  std::vector<g::u8> lanes;
  std::vector<std::int32_t> points;
  explicit Flat(std::vector<gen::Point3> x) : input(std::move(x)),owner(gen::make_q2_cloud_index(gen::prepare_cloud(input))),
      nodes(g::flatten_nodes(*owner)),escapes(g::flatten_escapes(*owner)),inverse(input.size()) {
    for(u32 rank=0;rank<owner->spatial_order().size();++rank) {
      const auto id=static_cast<u32>(owner->spatial_order()[rank]); ids.push_back(id);inverse[id]=rank;
      const auto p=owner->spatial_points()[rank];points.push_back(p.x);points.push_back(p.y);points.push_back(p.z);
    }
    for(u32 i=0;i<input.size();++i) for(u32 j=i+1;j<input.size();++j) {
      a.push_back(inverse[i]);b.push_back(inverse[j]);lanes.push_back(6);
    }
  }
  g::LanesInput view(unsigned k) const {
    g::LanesInput in;
    in.index.nodes=nodes.data();in.index.node_count=nodes.size();in.index.rank_points=points.data();in.index.rank_count=input.size();in.index.kmax=k;
    in.escapes=escapes.data();in.rank_ids=ids.data();in.edge_a=a.data();in.edge_b=b.data();in.edge_count=a.size();
    in.edge_lanes=k==2?nullptr:lanes.data();
    in.capacity=static_cast<u32>(input.size());in.record_capacity=256;in.event_capacity=256;
    in.arena_capacity=100000;in.staging_capacity=200000;in.cover_capacity=200000;
    return in;
  }
};

struct Counts { u64 calls=0,records3=0,records4=0,ids=0,extra_shell=0,deferred=0,permuted=0,device_calls=0,fused_fallbacks=0,direct_chunks=0,max_depth=0; } tally;

bool same3(const g::Q3Work& a,const g::Q3Work& b) {
  // Both ledgers are aggregates with integer fields only and initialized
  // to zero. Field comparison through their explicit additive identity is
  // checked by the existing lanes port gate; here compare every u64 word.
  static_assert(sizeof(g::Q3Work)%sizeof(u64)==0);
  return std::memcmp(&a,&b,sizeof(a))==0;
}
void compare(const g::LanesOutput& ref,const g::LanesOutput& got,unsigned k,const Flat& flat) {
  need(ref.error.empty() && got.error.empty() && ref.available && got.available,"output_error");
  need(got.interior_payload && got.interior_stride==k-2,"payload_echo");
  need(got.interior_ids.size()==got.record_total()*got.interior_stride,"payload_shape");
  need(ref.status==got.status && ref.record_begin==got.record_begin && ref.record_count==got.record_count,"record_slices");
  need(ref.record_total()==got.record_total() && same3(ref.work,got.work) &&
       std::memcmp(&ref.work4,&got.work4,sizeof(g::Q4Work))==0 && ref.deferred==got.deferred && ref.faults==got.faults,"ledger");
  for(std::size_t r=0;r<got.record_total();++r) {
    const auto& record=got.record_data()[r];
    need(std::memcmp(&record,ref.record_data()+r,sizeof(record))==0,"records_changed");
    if(record.arity==4) {
      for(u32 j=0;j<got.interior_stride;++j) need(got.interior_ids[r*got.interior_stride+j]==g::absent32,"q4_payload_nonempty");
      ++tally.records4;continue;
    }
    need(record.arity==3 && record.depth<=got.interior_stride,"q3_depth");
    const auto ball=gen::ExactBall::make_q3({flat.input[record.support[0]],flat.input[record.support[1]],flat.input[record.support[2]]});
    need(ball.has_value(),"nonpositive");
    std::vector<u32> expected,actual;
    for(u32 id=0;id<flat.input.size();++id) if(ball->power(flat.input[id])<0) expected.push_back(id);
    for(u32 j=0;j<got.interior_stride;++j) {
      const u32 id=got.interior_ids[r*got.interior_stride+j];
      if(j<record.depth) actual.push_back(id);else need(id==g::absent32,"padding_not_absent");
    }
    std::sort(actual.begin(),actual.end());need(actual==expected,"interior_ids_differ");
    ++tally.records3;tally.ids+=actual.size();tally.extra_shell+=record.shell>3;
  }
  ++tally.calls;tally.deferred+=got.deferred;tally.fused_fallbacks+=got.fused.fallbacks;
}

void fixture(Flat& flat,unsigned k,bool device) {
  for(u32 r=0;r<flat.ids.size();++r) tally.permuted+=flat.ids[r]!=r;
  auto in=flat.view(k);
  const auto ref=g::run_lanes_tasks_host(in,1,7);
  need(!ref.interior_payload && ref.interior_stride==0 && ref.interior_ids.empty(),"off_sidecar");
  in.interior_payload=true;
  for(bool fused:{false,true}) for(bool pinned:{false,true}) for(u64 budget:{u64{1},g::single_task_budget}) {
    in.fused_pass=fused;in.pinned_records=pinned;in.task_budget=budget;
    auto got=g::run_lanes_tasks_host(in,pinned?4:1,pinned?1:7);
    compare(ref,got,k,flat);
    // Keep the first sidecar and record lease alive across another call.
    const auto saved=got.interior_ids;
    const auto held=g::run_lanes_tasks_host(in,2,3);
    need(got.interior_ids==saved,"owned_sidecar_changed");
    compare(ref,held,k,flat);
    if(device) { auto gpu=g::run_lanes_batch(in);compare(ref,gpu,k,flat);++tally.device_calls; }
  }
  // Capacity-driven deferrals must stay exactly the old path's and may
  // never publish stale words from a failed fused attempt or rejected edge.
  for(unsigned mode=0;mode<4;++mode) {
    auto limited=in;limited.pinned_records=mode%2!=0;limited.task_budget=1;
    if(mode==0) limited.record_capacity=1;
    if(mode==1) limited.arena_capacity=1;
    if(mode==2) limited.staging_capacity=1;
    if(mode==3) limited.capacity=2;
    auto old=limited;old.interior_payload=false;
    const auto expected=g::run_lanes_tasks_host(old,2,7);
    const auto actual=g::run_lanes_tasks_host(limited,4,1);
    compare(expected,actual,k,flat);
    if(device) {auto gpu=g::run_lanes_batch(limited);compare(expected,gpu,k,flat);++tally.device_calls;}
  }
  // Empty calls retain the active K2/stride-zero distinction.
  auto empty=in;empty.edge_count=0;empty.edge_a=empty.edge_b=nullptr;empty.edge_lanes=nullptr;
  const auto e=g::run_lanes_batch_host(empty,2);
  need(e.error.empty() && e.interior_payload && e.interior_stride==k-2 && e.interior_ids.empty(),"empty_echo");
}

void chunk_boundaries() {
  for(u32 n:{31U,32U,33U,63U,64U,65U,66U}) {
    std::vector<gen::Point3> x={{88,100,100},{112,100,100},{100,116,100}};
    while(x.size()<n) x.push_back({static_cast<gen::Coordinate>(1000+x.size()),1000,1000});
    u32 inside=0;
    for(u32 position:{3U,4U,31U,32U,33U,34U,63U,64U}) if(position<n)
      x[position]={static_cast<gen::Coordinate>(100+inside++),104,100};
    Flat flat(x);
    // A deliberate fixed complete census order, separate from prologue
    // scan-order tests. Every global site is visited, including far outside.
    std::vector<std::int32_t> points;
    std::vector<u32> ranks;
    for(u32 id=0;id<n;++id) {points.push_back(x[id].x);points.push_back(x[id].y);points.push_back(x[id].z);ranks.push_back(flat.inverse[id]);}
    u32 seed=2;
    const g::LanesIndex index{{flat.nodes.data(),flat.escapes.data(),static_cast<u32>(flat.nodes.size()),flat.points.data()},flat.ids.data()};
    for(unsigned k:{2U,3U,5U,10U}) {
      g::LaneRecord before{},after{};
      std::array<u32,10> scratch; scratch.fill(0xcacacacaU);
      std::array<u32,10> ids;ids.fill(0xa5a5a5a5U);
      g::LanesSlab old{nullptr,points.data(),ranks.data(),&seed,nullptr,&before,n,1};
      auto current=old;current.records=&after;
      const g::Q3InteriorSlab payload{scratch.data(),ids.data(),k-2};
      g::EdgeQ3Work old_work{},new_work{};u32 old_count=0,new_count=0;
      const auto os=g::q3_census_range(g::HostGroup{},index,flat.inverse[0],flat.inverse[1],k,old,n,0,1,old_count,old_work);
      const auto ns=g::q3_census_range<true>(g::HostGroup{},index,flat.inverse[0],flat.inverse[1],k,current,n,0,1,new_count,new_work,payload);
      need(os==ns && old_count==new_count && old_work.census_point_tests==new_work.census_point_tests &&
           old_work.census_inside_sites==new_work.census_inside_sites && old_work.depth_rejections==new_work.depth_rejections,"chunk_work");
      need(scratch[8]==0xcacacacaU && scratch[9]==0xcacacacaU && ids[8]==0xa5a5a5a5U && ids[9]==0xa5a5a5a5U,"payload_overrun");
      if(new_count==0) for(auto id:ids) need(id==0xa5a5a5a5U,"rejected_prefix_published");
      else {
        need(std::memcmp(&before,&after,sizeof(before))==0,"chunk_record");
        std::vector<u32> expected,actual;
        const auto ball=gen::ExactBall::make_q3({x[0],x[1],x[2]});need(ball.has_value(),"chunk_ball");
        for(u32 id=0;id<n;++id) if(ball->power(x[id])<0) expected.push_back(id);
        for(u32 j=0;j<after.depth;++j) actual.push_back(ids[j]);
        std::sort(actual.begin(),actual.end());need(actual==expected,"interior_ids_differ");
        tally.max_depth=std::max(tally.max_depth,u64{after.depth});
      }
      ++tally.direct_chunks;
    }
  }
  need(tally.max_depth==8,"max_depth_not_exercised");
}
}  // namespace

int main(int argc,char** argv) {
  try {
    const bool device=argc==2 && std::string(argv[1])=="--device";
    if(argc!=1 && !device) return 2;
    chunk_boundaries();
    std::vector<gen::Point3> points={{88,100,100},{112,100,100},{100,116,100},
      {100,104,100},{101,104,100},{102,104,100},{103,104,100},{104,104,100},{105,104,100},{106,104,100},{107,104,100},
      {100,91,100},{110,111,100},{90,96,100},
      {200,200,202},{204,200,202},{202,203,202},{202,202,204},{202,202,200}};
    Flat exact(points);
    for(unsigned k:{2U,3U,5U,10U}) fixture(exact,k,device);
    for(const std::string family:{"uniform","terrain","clusters"}) {
      Flat generated(gen::bench::make_front_fixture(20,family,17).points);
      fixture(generated,5,device);
    }
    need(tally.records3 && tally.records4 && tally.ids && tally.extra_shell && tally.deferred && tally.permuted,"vacuous");
    std::cout<<"payload PASS calls="<<tally.calls<<" q3="<<tally.records3<<" q4="<<tally.records4<<" ids="<<tally.ids
      <<" extra_shell="<<tally.extra_shell<<" deferred="<<tally.deferred<<" permuted="<<tally.permuted<<" fused_fallbacks="<<tally.fused_fallbacks
      <<" device_calls="<<tally.device_calls<<" direct_chunks="<<tally.direct_chunks<<" max_depth="<<tally.max_depth<<'\n';return 0;
  } catch(const std::exception& e) {std::cout<<"cause="<<e.what()<<'\n';return 1;}
}
