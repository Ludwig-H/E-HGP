#include "../arena.hpp"
#include "../../b_q34_direct_bands_20260927/direct.hpp"
#include "front_fixtures.hpp"
#include <iostream>
#include <string_view>

namespace ca=mhgp9::audit::collective;
namespace db=mhgp9::audit::direct_bands;
using namespace mhgp9::gen;
void need(bool value,const char* why) {if (!value) throw std::runtime_error(why);}
template<class Error,class Fn> void rejects(Fn fn,std::string_view expected,u64& count) {
  bool refused=false;
  try {fn();} catch (const Error& e) {refused=std::string_view(e.what())==expected;}
  need(refused,"supplement.exact_refusal");++count;
}
int main() {
  try {
    u64 positives=0,refusals=0,queries=0,aliases=0,permuted=0,nonidentity_ids=0;
    for (const char* family:{"uniform","terrain","clusters"}) {
      auto points=bench::make_front_fixture(128,family,3).points;
      for (unsigned input_order=0;input_order!=2;++input_order) {
        if (input_order!=0) std::reverse(points.begin(),points.end());
        const auto index=make_q2_cloud_index(prepare_cloud(points));
        const auto nodes=index->spatial_nodes();
        const auto left=nodes.front().left,right=nodes.front().right;
        need(left!=Q2SpatialNode::absent && right!=Q2SpatialNode::absent,"supplement.nonempty_children");
        for (std::size_t i=0;i!=index->spatial_order().size();++i)
          if (index->spatial_order()[i]!=i) ++nonidentity_ids;
        ca::Request valid{left,right,11,6};
        std::vector<ca::Request> requests{valid};
        for (const std::uint8_t mask:{2,4,6}) {
          requests[0]=valid;requests[0].mask=mask;
          const auto arena=ca::Arena::build(index,requests,5,4,7);
          const db::Plan ref(*index,left,right,5,mask);
          need(arena->mass()==ca::Mass{ref.output().union_mass,ref.output().q3_mass,ref.output().q4_mass},"supplement.mass");
          for (unsigned side=0;side!=2;++side) {
            const auto& f=arena->factors()[side];const auto& rf=side==0?ref.a():ref.b();
            for (std::size_t i=0;i!=rf.grouped.size();++i) {
              const auto local=arena->ranks()[f.first+i];
              need(local+f.original_first==rf.grouped[i],"supplement.stable_permutation");
              if (local!=i) ++permuted;
              const auto c=rf.credits[rf.grouped[i]-rf.original_ranks.first];
              need(arena->credits()[f.first+i]==ca::detail::pack(c.q3,c.q4),"supplement.credit");
            }
          }
          const auto header=arena->rectangles()[0];
          requests[0]={0,0,999999,0};
          need(arena->rectangles()[0]==header,"supplement.request_alias");++aliases;++positives;
        }
        const auto build=[&](std::span<const ca::Request> r,unsigned k=5,unsigned workers=4,unsigned grain=7) {
          static_cast<void>(ca::Arena::build(index,r,k,workers,grain));
        };
        std::vector<ca::Request> two{valid,valid};
        rejects<std::invalid_argument>([&]{build(two);},"arena.request",refusals);
        two[1].source_ordinal=10;
        rejects<std::invalid_argument>([&]{build(two);},"arena.request",refusals);
        two[1].source_ordinal=19;
        build(two); // Positive strictly increasing source ordinals.
        std::vector<ca::Request> bad{valid};
        bad[0].mask=4;
        rejects<std::invalid_argument>([&]{build(bad,2);},"arena.request",refusals);
        bad[0].mask=6;
        rejects<std::invalid_argument>([&]{build(bad,2);},"arena.request",refusals);
        for (const std::uint8_t mask:{0,1}) {
          bad[0]=valid;bad[0].mask=mask;
          rejects<std::invalid_argument>([&]{build(bad);},"arena.request",refusals);
        }
        bad[0]=valid;bad[0].a_node=nodes.size();
        rejects<std::invalid_argument>([&]{build(bad);},"arena.request",refusals);
        bad[0]=valid;bad[0].b_node=left;
        rejects<std::invalid_argument>([&]{build(bad);},"arena.factors_disjoint",refusals);
        requests={valid};
        rejects<std::invalid_argument>([&]{static_cast<void>(ca::Arena::build({},requests,5,4,7));},"arena.configuration",refusals);
        rejects<std::invalid_argument>([&]{build(requests,1);},"arena.configuration",refusals);
        rejects<std::invalid_argument>([&]{build(requests,11);},"arena.configuration",refusals);
        rejects<std::invalid_argument>([&]{build(requests,5,0);},"arena.configuration",refusals);
        rejects<std::invalid_argument>([&]{build(requests,5,4,0);},"arena.configuration",refusals);
        const auto arena=ca::Arena::build(index,requests,5,1,7);
        rejects<std::out_of_range>([&]{static_cast<void>(arena->pair_mask(1,0,0));},"arena.rectangle_query",queries);
        rejects<std::out_of_range>([&]{static_cast<void>(arena->pair_mask(0,arena->factors()[0].classes,0));},"arena.local_query",queries);
        rejects<std::out_of_range>([&]{static_cast<void>(arena->pair_mask(0,0,arena->factors()[1].size));},"arena.local_query",queries);
        requests[0].mask=2;build(requests,2); // Positive K2/q3-only boundary.
      }
    }
    need(positives==18 && refusals==78 && queries==18 && aliases==18 && permuted>0 && nonidentity_ids>0,"supplement.coverage");
    std::cout<<"{\"schema\":\"mhgp9_collective_supplement_v1\",\"status\":\"pass\",\"positive_plans\":"<<positives
      <<",\"exact_refusals\":"<<refusals<<",\"query_refusals\":"<<queries<<",\"alias_checks\":"<<aliases
      <<",\"permuted_sites\":"<<permuted<<",\"nonidentity_original_ids\":"<<nonidentity_ids<<"}\n";
    return 0;
  } catch (const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
