#include "waves.hpp"
#include "front_fixtures.hpp"
#include <iostream>
#include <string_view>
#include <type_traits>

namespace wa=mhgp9::audit::waves;
using namespace mhgp9::gen;
constexpr const char* schema="mhgp9_q34_arena_waves_v1";
void need(bool value,const char* why) {if (!value) throw std::runtime_error(why);}
struct Checks {
  u64 batches{},consumptions{},pairs{},survivors{},pool_rejected{},pool_lane_rejected{},fallback_survivors{};
  u64 band_splits{},rectangle_crossings{},reordered{},empty{},no_survivors{},closed{},holes{},retries{},refusals{};
};
void same(const Q34FilterBatch& a,const Q34FilterBatch& b) {
  need(a.rectangle_masks==b.rectangle_masks,"gate.rectangle_masks");
  need(a.survivors==b.survivors,"gate.survivors_order_mask");
  need(a.expanded_pairs==b.expanded_pairs,"gate.logical_mass");
  need(a.pair_q3_rejected==b.pair_q3_rejected && a.pair_q4_rejected==b.pair_q4_rejected,"gate.lane_rejections");
  need(a.rectangle_visits==b.rectangle_visits,"gate.rectangle_once");
}
void check(const Q2CensusIndexPtr& index,std::vector<WspdRectangle> rectangles,unsigned k,
           wa::Mutant mutant,Checks& checks) {
  const auto reference=run_q34_filter_batch_cpu(*index,k,rectangles,1);
  const auto prepared=wa::Prepared::build(index,rectangles,k);
  wa::Mass logical;
  for (std::size_t i=0;i!=rectangles.size();++i) if (reference.rectangle_masks[i]!=0) {
    const auto& r=rectangles[i];const auto nodes=index->spatial_nodes();
    wa::add(logical,wa::product(nodes[r.a_node].range.size(),nodes[r.b_node].range.size()),reference.rectangle_masks[i]);
  }
  need(prepared->logical()==logical,"gate.logical_lane_masses");
  ++checks.batches;checks.closed+=prepared->work().closed;
  checks.pool_rejected+=prepared->logical().all-prepared->effective().all;
  checks.pool_lane_rejected+=prepared->logical().q3-prepared->effective().q3+
      prepared->logical().q4-prepared->effective().q4;
  bool closed_before=false;
  for (const auto& r:prepared->rectangles()) {
    if (r.mask==0) closed_before=true;else if (closed_before) ++checks.holes;
    if (r.mask!=0 && r.arena_rectangle==wa::absent)
      for (const auto& e:reference.survivors)
        if (e.a_rank>=r.a_first && e.a_rank-r.a_first<r.a_size &&
            e.b_rank>=r.b_first && e.b_rank-r.b_first<r.b_size) ++checks.fallback_survivors;
  }
  // Mutable caller data no longer influences the owner.
  for (auto& r:rectangles) r={0,0,0};
  Q34WitnessSearchWork first_work;Q34WitnessBoundsWork first_bounds;bool first=true;
  const std::array<std::size_t,6> capacities{1,2,7,17,64,static_cast<std::size_t>(prepared->effective().all+1)};
  for (const auto capacity:capacities) {
    wa::Cursor cursor(prepared,capacity,mutant);
    if (prepared->effective().all!=0 && capacity==7 && mutant==wa::Mutant::None) {
      bool failed=false;
      try {static_cast<void>(cursor.step(true));} catch (const std::bad_alloc&) {failed=true;}
      need(failed && cursor.pending() && cursor.consumed()==0,"gate.pending_failure");
      const auto work=cursor.pair_work();const auto bounds=cursor.pair_bounds();
      failed=false;
      try {static_cast<void>(cursor.step(true));} catch (const std::bad_alloc&) {failed=true;}
      need(failed && cursor.pair_work()==work && cursor.pair_bounds()==bounds,"gate.retry_no_geometry");
      need(cursor.step() && cursor.pair_work()==work && cursor.pair_bounds()==bounds,"gate.pending_commit");
      ++checks.retries;
    }
    while (cursor.step()) {}
    need(cursor.consumed()==prepared->effective().all,"gate.exhaustion");
    const auto output=cursor.finish();same(output,reference);
    need(cursor.work().decoded==prepared->effective().all && cursor.work().slot_peak<=capacity,"gate.wave_memory_bound");
    need(cursor.work().compacted==output.survivors.size(),"gate.compaction_mass");
    need(cursor.pair_work().queries==prepared->effective().all,"gate.actual_queries");
    need(cursor.work().array_bytes_peak>=cursor.work().slot_peak*sizeof(wa::Slot)+
         cursor.work().keyed_capacity_peak*sizeof(wa::KeyedEdge),"gate.capacity_peak");
    if (first) {first_work=cursor.pair_work();first_bounds=cursor.pair_bounds();first=false;}
    else need(cursor.pair_work()==first_work && cursor.pair_bounds()==first_bounds,"gate.capacity_geometry_equal");
    ++checks.consumptions;checks.pairs+=prepared->effective().all;checks.survivors+=output.survivors.size();
    checks.band_splits+=cursor.work().band_wave_splits;checks.rectangle_crossings+=cursor.work().rectangle_crossings;
    checks.reordered+=cursor.work().out_of_order;
    if (prepared->effective().all==0) ++checks.empty;
    if (prepared->effective().all!=0 && output.survivors.empty()) ++checks.no_survivors;
  }
}
void gate(wa::Mutant mutant) {
  static_assert(!std::is_copy_constructible_v<wa::Prepared> && !std::is_move_constructible_v<wa::Prepared>);
  Checks checks;
  for (const char* family:{"uniform","terrain","clusters","rows"}) {
    auto points=bench::make_front_fixture(24,family,3).points;
    for (unsigned input_order=0;input_order!=2;++input_order) {
      if (input_order!=0) std::reverse(points.begin(),points.end());
      const auto index=make_q2_cloud_index(prepare_cloud(points));
      for (const unsigned k:{1U,2U,3U,5U,10U}) for (const unsigned s:{8U,10U,12U}) {
        std::vector<WspdRectangle> rectangles;
        // A q34 front is unavailable at K1. Its consumer still accepts
        // valid raw rectangles and returns dead lanes, like the native batch.
        static_cast<void>(run_wspd_front(*index,std::max(k,2U),s,WspdFrontMode::Pure,[&](const WspdRectangle& r) {
          rectangles.push_back({r.a_node,r.b_node,k==1?std::uint8_t{6}:r.lane_mask});
        },6));
        check(index,std::move(rectangles),k,mutant,checks);
      }
    }
  }
  for (const char* family:{"uniform","terrain","clusters"}) {
    auto points=bench::make_front_fixture(64,family,3).points;
    for (unsigned input_order=0;input_order!=2;++input_order) {
      if (input_order!=0) std::reverse(points.begin(),points.end());
      const auto index=make_q2_cloud_index(prepare_cloud(points));const auto nodes=index->spatial_nodes();
      const auto left=nodes[0].left,right=nodes[0].right;
      for (const unsigned k:{1U,2U,3U,5U,10U}) check(index,{{left,right,6}},k,mutant,checks);
      for (const std::uint8_t mask:{2,4}) check(index,{{right,left,mask}},5,mutant,checks);
      // Explicit zero-sized lot and mixed ordered rectangles, including
      // closed singleton products, test holes in raw Cartesian ordinals.
      check(index,{},5,mutant,checks);
      std::vector<std::size_t> leaves;
      for (std::size_t i=0;i!=nodes.size();++i) if (nodes[i].range.size()==1) leaves.push_back(i);
      std::vector<WspdRectangle> mixed;
      for (std::size_t i=1;i<leaves.size();++i) mixed.push_back({leaves[0],leaves[i],6});
      mixed.push_back({left,right,6});check(index,mixed,3,mutant,checks);
      auto bad=mixed;bad[0].a_node=nodes.size();
      bool refused=false;try {static_cast<void>(wa::Prepared::build(index,bad,5));}
      catch (const std::invalid_argument& e) {refused=std::string_view(e.what())=="waves.rectangle";}
      need(refused,"gate.invalid_node");++checks.refusals;
      const auto prepared=wa::Prepared::build(index,mixed,3);
      refused=false;try {wa::Cursor cursor(prepared,0);}
      catch (const std::invalid_argument& e) {refused=std::string_view(e.what())=="waves.cursor_configuration";}
      need(refused,"gate.invalid_capacity");++checks.refusals;
      wa::Cursor cursor(prepared,1);
      refused=false;try {static_cast<void>(cursor.finish());}
      catch (const std::logic_error& e) {refused=std::string_view(e.what())=="waves.not_exhausted";}
      need(refused,"gate.premature_finish");++checks.refusals;
    }
  }
  // Find one small native rectangle whose conservative box filter stays
  // open while exact pair filtering closes every candidate. Exhaustive,
  // bounded fixture discovery, not a search quota in the consumer.
  bool empty_survivors_found=false;
  for (const char* family:{"uniform","terrain","clusters"}) {
    const auto index=make_q2_cloud_index(prepare_cloud(bench::make_front_fixture(64,family,3).points));
    const auto nodes=index->spatial_nodes();
    for (std::size_t a=1;a<nodes.size() && !empty_survivors_found;++a)
      for (std::size_t b=a+1;b<nodes.size() && !empty_survivors_found;++b) {
        const auto ar=nodes[a].range,br=nodes[b].range;
        if (!(ar.last<=br.first || br.last<=ar.first)) continue;
        const std::vector<WspdRectangle> rectangles{{a,b,6}};
        const auto out=run_q34_filter_batch_cpu(*index,3,rectangles,1);
        if (out.expanded_pairs!=0 && out.survivors.empty()) {
          const auto prepared=wa::Prepared::build(index,rectangles,3);
          if (prepared->effective().all==0) continue;
          check(index,rectangles,3,mutant,checks);empty_survivors_found=true;
        }
      }
    if (empty_survivors_found) break;
  }
  need(empty_survivors_found && checks.no_survivors>0,"gate.nonempty_candidates_empty_survivors");
  need(wa::product(65536,65536)==u64{1}<<32 && wa::sum(u64{1}<<32,17)==(u64{1}<<32)+17,"gate.u64_prefix");
  const wa::Rectangle large{(u64{1}<<32)+17,wa::absent,0,0,65536,65536,6};
  const auto large_ranks=wa::fallback_ranks(large,(u64{1}<<32)-1);
  need(large_ranks==std::array<u64,2>{65535,65535} &&
       wa::original_ordinal(large,large_ranks[0],large_ranks[1])==(u64{1}<<33)+16,"gate.u64_decode_formula");
  bool refused=false;try {static_cast<void>(wa::product(std::numeric_limits<u64>::max(),2));}
  catch (const std::overflow_error&) {refused=true;}need(refused,"gate.product_overflow");++checks.refusals;
  refused=false;try {static_cast<void>(wa::sum(std::numeric_limits<u64>::max(),1));}
  catch (const std::overflow_error&) {refused=true;}need(refused,"gate.prefix_overflow");++checks.refusals;
  need(checks.pool_rejected>0 && checks.pool_lane_rejected>0 && checks.fallback_survivors>0 && checks.band_splits>0 &&
       checks.rectangle_crossings>0 && checks.reordered>0 && checks.empty>0 && checks.closed>0 && checks.holes>0 && checks.retries>0,
       "gate.nonvacuous_coverage");
  std::cout<<"{\"schema\":\""<<schema<<"\",\"status\":\"pass\",\"coverage\":{"
    <<"\"batches\":"<<checks.batches<<",\"consumptions\":"<<checks.consumptions<<",\"pairs\":"<<checks.pairs
    <<",\"survivors\":"<<checks.survivors<<",\"pool_rejected\":"<<checks.pool_rejected
    <<",\"pool_lane_rejected\":"<<checks.pool_lane_rejected<<",\"fallback_survivors\":"<<checks.fallback_survivors
    <<",\"band_splits\":"<<checks.band_splits<<",\"rectangle_crossings\":"<<checks.rectangle_crossings
    <<",\"reordered\":"<<checks.reordered<<",\"empty\":"<<checks.empty<<",\"no_survivors\":"<<checks.no_survivors
    <<",\"closed\":"<<checks.closed<<",\"holes\":"<<checks.holes<<",\"retries\":"<<checks.retries
    <<",\"refusals\":"<<checks.refusals<<"}}\n";
}
int main(int argc,char** argv) {
  try {
    if (argc==2 && std::string_view(argv[1])=="--gate") {gate(wa::Mutant::None);return 0;}
    if (argc==3 && std::string_view(argv[1])=="--mutant") {
      const std::string_view name=argv[2];
      if (name=="mask6") gate(wa::Mutant::Mask6);
      else if (name=="fallback") gate(wa::Mutant::SkipFallback);
      else if (name=="ordinal") gate(wa::Mutant::GroupOrdinal);
      else throw std::invalid_argument("gate.unknown_mutant");
      throw std::runtime_error("gate.mutant_survived");
    }
    throw std::invalid_argument("gate.usage");
  } catch (const std::exception& e) {
    std::cout<<"{\"schema\":\""<<schema<<"\",\"status\":\"failed\",\"cause\":\""<<e.what()<<"\"}\n";return 1;
  }
}
