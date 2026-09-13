// Audit fixture: five diameter incidences, one geometry, a 398-ID shell.
#include "pipeline/q2_census.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {
using Point = mhgp8::Point3;
using i64 = std::int64_t;
using u64 = std::uint64_t;
using Pair = std::pair<std::size_t,std::size_t>;
using Key = std::tuple<std::uint32_t,std::uint32_t,std::uint32_t,u64>;
void require(bool ok,const char* why) { if(!ok)throw std::runtime_error(why); }
using SignedPoint = std::array<int,3>;
mhgp8::RectangleInput fixture(bool reverse) {
  std::set<SignedPoint> sphere;
  // Integer squares table, no floating square root or geometric tolerance.
  std::array<int,65*65+1> roots{};
  roots.fill(-1);
  for(int z=0;z<=65;++z) roots[z*z]=z;
  for(int x=-65;x<=65;++x)
    for(int y=-65;y<=65;++y) {
      const int zz=65*65-x*x-y*y;
      if(zz>=0 && roots[zz]>=0) {
        sphere.insert({17*x,17*y,17*roots[zz]});
        sphere.insert({17*x,17*y,-17*roots[zz]});
      }
    }
  const std::array<SignedPoint,5> cap{{{1105,0,0},{1104,47,0},{1104,-47,0},
                                      {1104,0,47},{1104,0,-47}}};
  std::vector<SignedPoint> ordered;
  for(const auto p:cap) ordered.push_back(p);
  for(const auto p:cap) ordered.push_back({-p[0],-p[1],-p[2]});
  for(const auto p:ordered) sphere.erase(p);
  ordered.insert(ordered.end(),sphere.begin(),sphere.end());
  mhgp8::RectangleInput input;
  input.a={0,5};input.b={5,10};
  if(reverse) {
    std::reverse(ordered.begin(),ordered.end());
    const auto n=ordered.size();
    input.a={n-5,n};input.b={n-10,n-5};
  }
  for(const auto p:ordered) {
    require(i64(p[0])*p[0]+i64(p[1])*p[1]+i64(p[2])*p[2]==1105*1105,"fixture sphere");
    input.points.push_back({static_cast<std::uint16_t>(p[0]+2000),
                            static_cast<std::uint16_t>(p[1]+2000),
                            static_cast<std::uint16_t>(p[2]+2000)});
  }
  require(input.points.size()==398,"sphere fixture nonvacuity");
  return input;
}
struct Payload {
  mhgp8::Q2BallKey key;
  std::vector<std::size_t> interior,shell;
  bool operator==(const Payload&) const = default;
};
Payload exact(const mhgp8::RectangleInput& input,std::size_t a,std::size_t b) {
  Payload result;
  for(unsigned k=0;k<3;++k) {
    result.key.center_twice[k]=unsigned(input.points[a][k])+input.points[b][k];
    const i64 delta=i64(input.points[a][k])-input.points[b][k];
    result.key.diameter_squared+=static_cast<u64>(delta*delta);
  }
  for(std::size_t id=0;id<input.points.size();++id) {
    i64 h=0;
    for(unsigned k=0;k<3;++k)
      h+=(i64(input.points[id][k])-input.points[a][k])*(i64(input.points[b][k])-input.points[id][k]);
    if(h>0)result.interior.push_back(id);
    else if(h==0)result.shell.push_back(id);
  }
  return result;
}
Key key_of(const mhgp8::Q2BallKey& key) {
  return {key.center_twice[0],key.center_twice[1],key.center_twice[2],key.diameter_squared};
}
void run() {
  u64 cases=0,calls=0,oracle_tests=0,supports=0,shell_ids=0,canonical_shell_ids=0;
  u64 payload_visits=0,pairwise_visits=0,shared_visits=0;
  for(bool reverse:{false,true})
    for(unsigned cap:{1U,5U,10U})
      for(unsigned separation:{8U,10U,12U}) {
        const auto input=fixture(reverse);
        const auto owner=mhgp8::prepare_rectangle(input,cap,separation);
        const auto plan=mhgp8::make_axis_q2_plan(owner,mhgp8::AxisQ2Mode::Additive);
        const auto index=mhgp8::make_q2_census_index(owner);
        std::map<Pair,Payload> expected;
        for(auto a=input.a.first;a<input.a.last;++a)
          for(auto b=input.b.first;b<input.b.last;++b) {
            const auto result=exact(input,a,b);
            oracle_tests+=input.points.size();
            if(result.interior.size()<cap)expected.emplace(Pair{a,b},result);
          }
        require(plan.candidate_pairs()==25 && expected.size()==5,"expected fixture population");
        for(auto mode:{mhgp8::Q2CensusMode::Pairwise,mhgp8::Q2CensusMode::SharedBlocks}) {
          std::map<Pair,Payload> observed;
          const auto result=mhgp8::run_q2_census(*index,plan,mode,[&](const auto& support) {
            Payload value{support.key,{support.interior.begin(),support.interior.end()},
                                      {support.shell.begin(),support.shell.end()}};
            std::sort(value.interior.begin(),value.interior.end());
            std::sort(value.shell.begin(),value.shell.end());
            require(observed.emplace(Pair{support.a_id,support.b_id},value).second,"duplicate incidence");
          });
          require(observed==expected,"payload differs from independent census");
          require(result.accepted_pairs==5 && result.rejected_pairs==20 &&
                  result.work.payload_interior_sites==0 && result.work.payload_shell_sites==1990,
                  "materialized payload work");
          std::map<Key,Payload> catalog;
          std::vector<std::pair<Pair,Key>> incidences;
          for(const auto& [pair,payload]:observed) {
            const auto key=key_of(payload.key);
            const auto [entry,inserted]=catalog.emplace(key,payload);
            require(inserted || entry->second==payload,"equal geometry has different census");
            incidences.emplace_back(pair,key);
          }
          require(catalog.size()==1 && incidences.size()==5,"canonicalization lost incidences");
          std::map<Pair,Payload> reconstructed;
          for(const auto& [pair,key]:incidences) reconstructed.emplace(pair,catalog.at(key));
          require(reconstructed==observed,"canonical catalog round-trip");
          require(catalog.begin()->second.shell.size()==398,"canonical shell must remain complete");
          supports+=observed.size();
          shell_ids+=result.work.payload_shell_sites;
          canonical_shell_ids+=catalog.begin()->second.shell.size();
          payload_visits+=result.work.payload_node_visits;
          if(mode==mhgp8::Q2CensusMode::Pairwise)pairwise_visits+=result.work.count_node_visits;
          else shared_visits+=result.work.count_node_visits;
          ++calls;
        }
        ++cases;
      }
  require(cases==18 && calls==36 && supports==180,"nonvacuous repeated-shell audit");
  std::cout<<"{\"status\":\"passed\",\"cases\":"<<cases<<",\"calls\":"<<calls
    <<",\"input_sites\":398,\"candidates_per_call\":25,\"supports_per_call\":5,\"balls_per_call\":1"
    <<",\"shell_per_ball\":398,\"oracle_point_tests\":"<<oracle_tests
    <<",\"emitted_supports\":"<<supports<<",\"emitted_shell_ids\":"<<shell_ids
    <<",\"canonical_shell_ids\":"<<canonical_shell_ids<<",\"payload_node_visits\":"<<payload_visits
    <<",\"pairwise_count_node_visits\":"<<pairwise_visits
    <<",\"shared_count_node_visits\":"<<shared_visits<<"}\n";
}
} // namespace
int main() {
  try { run(); }
  catch(const std::exception& e) {std::cerr<<"q2 repeated shell audit: "<<e.what()<<'\n';return 1;}
}
