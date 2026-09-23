#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"
#include "wspd/front.hpp"
#include "lanes/q34_witness_search.hpp"
#include "spindle/predicates.hpp"
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
using namespace mhgp9::gen;

static std::uint8_t cached_box(const Q2CensusIndex& ix, Point3 a, Box3 b,
    unsigned k, std::uint8_t mask, const std::vector<Q34WitnessNode>& trace,
    unsigned long long& tests) {
  const auto nodes=ix.spatial_nodes();
  const auto aa=singleton_box(a);
  unsigned count3=0,count4=0;
  for(const auto& e:trace) {
    const auto active=static_cast<std::uint8_t>(e.lanes&mask);
    if(!active) continue;
    ++tests;
    const auto h=spindle_detail::h_minimum(aa,b,nodes[e.node].box);
    if(h<=0) continue;
    const auto xi=spindle_detail::xi_bounds(aa,b,nodes[e.node].box).high;
    const auto h2=spindle_detail::square(h);
    if((active&2) && static_cast<i128>(3)*h2>xi) count3+=nodes[e.node].range.size();
    if((active&4) && static_cast<i128>(2)*h2>xi) count4+=nodes[e.node].range.size();
  }
  std::uint8_t reject=0;
  if((mask&2) && count3>=k-1) reject|=2;
  if((mask&4) && count4>=k-2) reject|=4;
  return reject;
}

int main(int argc,char**argv) {
  if(argc!=3) return 2;
  std::ifstream f(argv[1],std::ios::binary);
  if(!f) return 3;
  std::vector<Point3> points;
  std::array<std::uint32_t,3> xyz;
  while(f.read(reinterpret_cast<char*>(xyz.data()),sizeof(xyz)))
    points.push_back(Point3{static_cast<Coordinate>(xyz[0]),static_cast<Coordinate>(xyz[1]),static_cast<Coordinate>(xyz[2])});
  const auto k=static_cast<unsigned>(std::stoul(argv[2]));
  const auto ix=make_q2_cloud_index(prepare_cloud(points));
  const auto nodes=ix->spatial_nodes();
  std::array<unsigned long long,8> all_n{},all_mass{},live_n{},live_mass{};
  unsigned long long front_mask_mass[2]{},rect_mask_mass[2]{};
  unsigned long long child_rejected_mass[2]{},child_filter_count{},child_input_mass{},child_union_reject_mass{},child_reject_rect{};
  unsigned long long total_rect{},zero_rect{},live_rect{},large_rect{};
  unsigned long long live_one_factor{},live_both_factor{},large_mass{},large_a_sum{},large_b_sum{},large_b2_rect{},large_b2_mass{};
  unsigned long long cached_first_pairs{},cached_first_rejected{},cached_root_queries{},cached_root_tests{},cached_root_saves{};
  unsigned long long cached_child_queries{},cached_child_tests{},cached_child_saves{};
  unsigned long long sample_rows{},sample_pairs{},sample_saved_pairs{},sample_saved_cache_tests{},sample_saved_search_nodes{};
  unsigned long long sample_total_cache_tests{},sample_total_search_nodes{},sample_box_tests{};
  Q34WitnessSearchWork sample_pair_work{};Q34WitnessBoundsWork sample_pair_bounds{};Q34WitnessCacheWork sample_cache_work{};
  Q34WitnessSearchWork first_pair_work{};Q34WitnessBoundsWork first_pair_bounds{};
  std::vector<Q34WitnessNode> trace;
  Q34WitnessSearchWork work{}, child_work{};
  Q34WitnessBoundsWork bounds{}, child_bounds{};
  const auto tic=std::chrono::steady_clock::now();
  const auto fr=run_wspd_front(*ix,k,8,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& r){
    const auto& a=nodes[r.a_node]; const auto& b=nodes[r.b_node];
    const auto mass=static_cast<unsigned long long>(a.range.size())*b.range.size();
    auto cat=[](unsigned long long x){unsigned i=0;while(x>1 && i<7){x=(x+1)/2;++i;}return i;};
    const auto bin=cat(mass);++all_n[bin];all_mass[bin]+=mass;++total_rect;
    front_mask_mass[0]+= (r.lane_mask&2)?mass:0;front_mask_mass[1]+= (r.lane_mask&4)?mass:0;
    const auto open=filter_q34_witnesses(*ix,a.box,b.box,static_cast<std::uint8_t>(k),r.lane_mask,work,Q34WitnessBoundsMode::Affine,bounds);
    if(!open){++zero_rect;return;}
    ++live_rect; ++live_n[bin]; live_mass[bin]+=mass;
    rect_mask_mass[0]+= (open&2)?mass:0;rect_mask_mass[1]+= (open&4)?mass:0;
    if(a.range.size()>1 && b.range.size()>1) ++live_both_factor;else ++live_one_factor;
    if(b.range.size()>1){++large_b2_rect;large_b2_mass+=mass;}
    if(mass<16) return;
    ++large_rect;large_mass+=mass;large_a_sum+=a.range.size();large_b_sum+=b.range.size();
    if(b.range.size()>1) {
      const auto order=ix->spatial_order();const auto pp=ix->cloud().points();
      for(auto ai=a.range.first;ai<a.range.last;++ai) {
        const auto id=order[ai];const auto bid=order[b.range.first];
        ++cached_first_pairs;
        const auto kept=filter_q34_witnesses(*ix,pp[id],pp[bid],static_cast<std::uint8_t>(k),open,first_pair_work,first_pair_bounds,trace);
        if(kept)continue;
        ++cached_first_rejected;
        ++cached_root_queries;
        const auto root_reject=cached_box(*ix,pp[id],b.box,k,open,trace,cached_root_tests);
        if(root_reject==open){cached_root_saves+=b.range.size()-1;continue;}
        if(b.left==Q2SpatialNode::absent)continue;
        for(auto c:{b.left,b.right}) {
          ++cached_child_queries;
          const auto cr=cached_box(*ix,pp[id],nodes[c].box,k,open,trace,cached_child_tests);
          if(cr==open)cached_child_saves+=nodes[c].range.size()- (nodes[c].range.first==b.range.first?1:0);
        }
      }
    }
    if(b.range.size()>1 && (large_rect%32)==0) {
      const auto order=ix->spatial_order();const auto pp=ix->cloud().points();
      for(auto ai=a.range.first;ai<a.range.last;++ai) {
        ++sample_rows;
        const auto id=order[ai];
        std::vector<Q34WitnessNode> cache, scratch;
        std::uint8_t blocked_root=0,blocked_left=0,blocked_right=0;
        for(auto bi=b.range.first;bi<b.range.last;++bi) {
          const auto bid=order[bi];
          const auto before_cache=sample_cache_work.node_tests;
          const auto before_search=sample_pair_work.node_visits;
          const auto cached=cache.empty()?std::uint8_t{0}:q34_cached_witness_rejections(*ix,pp[id],pp[bid],static_cast<std::uint8_t>(k),open,cache,sample_cache_work);
          const auto still=static_cast<std::uint8_t>(open&~cached);
          if(still) {
            static_cast<void>(filter_q34_witnesses(*ix,pp[id],pp[bid],static_cast<std::uint8_t>(k),still,sample_pair_work,sample_pair_bounds,scratch));
            cache.swap(scratch);
          }
          ++sample_pairs;
          if(bi==b.range.first) {
            if(!cache.empty()) {
              blocked_root=cached_box(*ix,pp[id],b.box,k,open,cache,sample_box_tests);
              if(blocked_root!=open && b.left!=Q2SpatialNode::absent) {
                blocked_left=cached_box(*ix,pp[id],nodes[b.left].box,k,open,cache,sample_box_tests);
                blocked_right=cached_box(*ix,pp[id],nodes[b.right].box,k,open,cache,sample_box_tests);
              }
            }
          } else if(blocked_root==open ||
              (b.left!=Q2SpatialNode::absent &&
               (bi<nodes[b.left].range.last ? blocked_left==open : blocked_right==open))) {
            ++sample_saved_pairs;
            sample_saved_cache_tests+=sample_cache_work.node_tests-before_cache;
            sample_saved_search_nodes+=sample_pair_work.node_visits-before_search;
          }
        }
      }
    }
    const bool split_a=a.range.size()>=b.range.size() && a.left!=Q2SpatialNode::absent;
    const auto& spl=split_a?a:b;
    if(spl.left==Q2SpatialNode::absent) return;
    for(auto child:{spl.left,spl.right}) {
      const auto& ac=split_a?nodes[child]:a;
      const auto& bc=split_a?b:nodes[child];
      const auto cmass=static_cast<unsigned long long>(ac.range.size())*bc.range.size();
      child_input_mass+=cmass;++child_filter_count;
      const auto co=filter_q34_witnesses(*ix,ac.box,bc.box,static_cast<std::uint8_t>(k),open,child_work,Q34WitnessBoundsMode::Affine,child_bounds);
      child_rejected_mass[0]+= ((open&2)&&!(co&2))?cmass:0;
      child_rejected_mass[1]+= ((open&4)&&!(co&4))?cmass:0;
      if(co==0){child_union_reject_mass+=cmass;++child_reject_rect;}
    }
  },6);
  auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-tic).count();
  std::cout<<"n "<<points.size()<<" k "<<k<<" elapsed "<<elapsed<<" front_rect "<<fr.work.emitted_rectangles<<" total "<<total_rect<<" zero "<<zero_rect<<" live "<<live_rect<<"\n";
  std::cout<<"front_mass "<<front_mask_mass[0]<<" "<<front_mask_mass[1]<<" rect_mass "<<rect_mask_mass[0]<<" "<<rect_mask_mass[1]<<"\n";
  std::cout<<"large_rect "<<large_rect<<" large_mass "<<large_mass<<" child_filters "<<child_filter_count<<" child_input_mass "<<child_input_mass<<" child_reject_mass "<<child_rejected_mass[0]<<" "<<child_rejected_mass[1]<<" child_union_reject_mass "<<child_union_reject_mass<<" child_reject_rect "<<child_reject_rect<<"\n";
  std::cout<<"factor_shape "<<live_one_factor<<" "<<live_both_factor<<"\n";
  std::cout<<"large_factor_sum "<<large_a_sum<<" "<<large_b_sum<<" b2_rect_mass "<<large_b2_rect<<" "<<large_b2_mass<<"\n";
  std::cout<<"cached_first "<<cached_first_pairs<<" "<<cached_first_rejected<<" pair_nodes "<<first_pair_work.node_visits<<" pair_points "<<first_pair_work.point_tests<<"\n";
  std::cout<<"cached_root "<<cached_root_queries<<" "<<cached_root_tests<<" "<<cached_root_saves<<" child "<<cached_child_queries<<" "<<cached_child_tests<<" "<<cached_child_saves<<"\n";
  std::cout<<"sample "<<sample_rows<<" "<<sample_pairs<<" saved "<<sample_saved_pairs<<" cache_nodes "<<sample_saved_cache_tests<<" search_nodes "<<sample_saved_search_nodes<<" total_cache "<<sample_cache_work.node_tests<<" total_search "<<sample_pair_work.node_visits<<" box_tests "<<sample_box_tests<<"\n";
  for(unsigned i=0;i<8;++i)std::cout<<"bin "<<i<<" "<<all_n[i]<<" "<<all_mass[i]<<" "<<live_n[i]<<" "<<live_mass[i]<<"\n";
  std::cout<<"searches "<<work.queries<<" nodes "<<work.node_visits<<" points "<<work.point_tests<<"\n";
  std::cout<<"child_searches "<<child_work.queries<<" nodes "<<child_work.node_visits<<" points "<<child_work.point_tests<<"\n";
}
