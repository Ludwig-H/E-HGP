#include "host.hpp"
#include "../b_q34_filtered_resident_20260927/host.hpp"
#include "front_fixtures.hpp"
#include <iostream>
#include <string_view>
namespace ss=mhgp9::audit::resident_survivors;
namespace rr=mhgp9::audit::resident;
namespace cw=mhgp9::audit::cuda_waves;
using namespace mhgp9::gen;
using cw::u64;
void check(bool b,const char* why) {if(!b) throw std::runtime_error(why);}
struct Coverage {u64 collector_cases{},refused{},cases{},calls{},queries{},survivors{},reordered{},empty{},
  zero_output{},sparse_waves{},high_keys{},growths{},max_array_bytes{},planned{},fallbacks{};} coverage;
template<class F> void refuse(F&& call,const char* reason) {
  try {call();} catch(const std::exception& e) {check(std::string_view(e.what())==reason,"survivors_gate.refusal_reason");++coverage.refused;return;}
  throw std::runtime_error("survivors_gate.refusal_missing");
}
void collect_case(const std::vector<cw::Edge>& edges,u64 q,const std::vector<size_t>& chunks,bool debug) {
  ss::PortableCollector collector(q);size_t position=0;
  for(auto n:chunks) {
    check(n<=edges.size()-position,"survivors_gate.fixture_chunks");
    collector.append(n?edges.data()+position:nullptr,n);position+=n;
    check(collector.capacity()<=ss::plus(ss::times(collector.size(),2),q),"survivors_gate.sparse_capacity");
    coverage.sparse_waves+=n==0;
  }
  check(position==edges.size(),"survivors_gate.fixture_complete");
  auto expected=edges;u64 inversions=0;
  for(size_t i=1;i<edges.size();++i) inversions+=edges[i-1].ordinal>=edges[i].ordinal;
  std::sort(expected.begin(),expected.end(),[](const auto& a,const auto& b){return a.ordinal<b.ordinal;});
  ss::OrderedRun result;collector.finish(result,debug);
  check(result.survivors.size()==expected.size() && result.counters.out_of_order==inversions,"survivors_gate.collect_count");
  for(size_t i=0;i<expected.size();++i) {
    const auto& a=result.survivors[i];const auto& b=expected[i];
    check(a.a==b.a && a.b==b.b && a.mask==b.mask && a.reserved[0]==0 && a.reserved[1]==0 && a.reserved[2]==0,
      "survivors_gate.key_payload");
    if(debug) check(result.debug_keys[i]==b.ordinal,"survivors_gate.full64_order");
    coverage.high_keys+=b.ordinal>0xffffffffULL;
  }
  if(!debug) check(result.debug_keys.empty() && result.memory.debug_key_bytes==0,"survivors_gate.optional_keys");
  check(result.memory.append_copy_bytes==edges.size()*sizeof(cw::Edge) && result.memory.size==edges.size() &&
    result.memory.array_peak_bytes>=result.memory.edge_peak_bytes,"survivors_gate.collect_memory");
  refuse([&]{collector.append(nullptr,0);},"survivors.collector_closed");
  coverage.growths+=result.memory.growths;coverage.max_array_bytes=std::max(coverage.max_array_bytes,result.memory.array_peak_bytes);
  ++coverage.collector_cases;
}
void collector_gate() {
  const std::vector<cw::Edge> high{{(u64{1}<<63)+1,1,31,2},{3,2,32,4},{(u64{1}<<32)+2,3,33,6},
    {cw::absent-1,4,34,2},{0,5,35,6},{(u64{1}<<32),6,36,4}};
  for(u64 q:{1,7,64}) for(bool debug:{false,true}) {
    collect_case(high,q,{0,1,0,2,0,3,0},debug);collect_case({},q,{0,0,0},debug);
    std::vector<cw::Edge> sparse;std::vector<size_t> chunks;
    for(size_t wave=0;wave<1024;++wave) {const bool live=wave%3==0;chunks.push_back(live?1:0);
      if(live) sparse.push_back({(u64{1}<<40)+1024-wave,static_cast<cw::u32>(wave),static_cast<cw::u32>(wave+1),6});}
    collect_case(sparse,q,chunks,debug);
  }
  ss::OrderedRun untouched;untouched.survivors.push_back({42,43,6,{0,0,0}});untouched.counters.out_of_order=9;
  ss::PortableCollector dup(1);const cw::Edge duplicates[]{{17,1,2,2},{17,8,9,4}};dup.append(duplicates,2);
  refuse([&]{dup.finish(untouched,true);},"survivors.duplicate_or_unsorted");
  check(untouched.survivors.size()==1 && untouched.survivors[0].a==42 && untouched.counters.out_of_order==9,
    "survivors_gate.no_partial_duplicate_output");
  refuse([&]{dup.finish(untouched,true);},"survivors.collector_closed");
  refuse([]{ss::PortableCollector bad(0);},"survivors.zero_quantum");
  refuse([]{ss::PortableCollector bad(1);bad.append(nullptr,1);},"survivors.null_input");
  refuse([]{ss::PortableCollector bad(1);const cw::Edge e{0,1,2,1};bad.append(&e,1);},"survivors.invalid_payload");
  refuse([]{static_cast<void>(ss::plus(cw::absent,1));},"survivors.counter_overflow");
  refuse([]{static_cast<void>(ss::times(cw::absent,2));},"survivors.byte_overflow");
  refuse([]{static_cast<void>(ss::count_size<cw::Edge>(cw::absent));},"survivors.allocation_size");
  check(ss::next_capacity(u64{1}<<31,(u64{1}<<31)+1,7)==(u64{1}<<32) &&
    ss::count_size<ss::Payload>(u64{1}<<32)==(u64{1}<<32),"survivors_gate.no_global_int_ceiling");
  ss::PortableCollector fault(1);const cw::Edge e{0,1,2,6};fault.append(&e,1);bool rejected=false;
  try {fault.append(&e,1,true);} catch(const std::bad_alloc&) {rejected=true;++coverage.refused;}
  check(rejected,"survivors_gate.injected_allocation_failure");
  refuse([&]{fault.finish(untouched,false);},"survivors.collector_closed");
}
void same(const Q34FilterBatch& a,const Q34FilterBatch& b) {
  check(a.rectangle_masks==b.rectangle_masks && a.survivors==b.survivors,"survivors_gate.native_output");
  check(a.expanded_pairs==b.expanded_pairs && a.pair_q3_rejected==b.pair_q3_rejected &&
    a.pair_q4_rejected==b.pair_q4_rejected && a.rectangle_visits==b.rectangle_visits,
    "survivors_gate.native_counts");
}
void same_counters(const cw::Counters& a,const cw::Counters& b) {
  check(a.queries==b.queries && a.q3==b.q3 && a.q4==b.q4 && a.rejected3==b.rejected3 && a.rejected4==b.rejected4 &&
    a.visits==b.visits && a.waves==b.waves && a.out_of_order==b.out_of_order,"survivors_gate.identical_work");
}
void native_case(Q2CensusIndexPtr index,const std::vector<WspdRectangle>& input,unsigned k,bool cuda) {
  const auto reference=run_q34_filter_batch_cpu(*index,k,input,1);
  auto old=rr::Session::open(index,input,k,cuda?rr::Backend::CUDA:rr::Backend::Portable,7);
  auto newer=ss::Session::open(index,input,k,cuda?ss::Backend::CUDA:ss::Backend::Portable,7);
  auto old_plan=rr::Prepared::build(old->rectangles(),1);auto new_plan=ss::Prepared::build(newer->rectangles(),1);
  check(old_plan->effective()==new_plan->effective(),"survivors_gate.identical_E");
  coverage.planned+=new_plan->planned();coverage.fallbacks+=new_plan->fallbacks();
  // Same process, same inputs, ABBA. No warm-subtracted performance claim.
  for(size_t q:{size_t{1},size_t{7},size_t{257},size_t{0x80000011ULL}}) {
    if(cuda && q==1 && new_plan->effective().all>16) continue;
    auto a=old->consume(*old_plan,q);auto b=newer->consume(*new_plan,q,true);
    auto bb=newer->consume(*new_plan,q,false);auto aa=old->consume(*old_plan,q);
    same(a.output,reference);same(b.output,reference);same(bb.output,a.output);same(aa.output,a.output);
    same_counters(a.counters,b.counters);same_counters(a.counters,bb.counters);same_counters(a.counters,aa.counters);
    check(b.debug_keys.size()==b.output.survivors.size() && bb.debug_keys.empty(),"survivors_gate.debug_mode");
    for(size_t i=1;i<b.debug_keys.size();++i) check(b.debug_keys[i-1]<b.debug_keys[i],"survivors_gate.debug_order");
    coverage.calls+=4;coverage.queries+=b.counters.queries;coverage.survivors+=b.output.survivors.size();
    coverage.reordered+=b.counters.out_of_order;coverage.empty+=b.counters.queries==0;
    coverage.zero_output+=b.counters.queries!=0 && b.output.survivors.empty();
    coverage.max_array_bytes=std::max(coverage.max_array_bytes,b.memory.array_peak_bytes);
  }
  old_plan.reset();new_plan.reset();old->close();newer->close();++coverage.cases;
}
void ownership_gate(bool cuda) {
  static_assert(!std::is_copy_constructible_v<ss::Prepared> && !std::is_move_constructible_v<ss::Prepared>);
  static_assert(!std::is_copy_constructible_v<ss::FilteredRectangles> && !std::is_default_constructible_v<ss::FilteredRectangles>);
  auto index=make_q2_cloud_index(prepare_cloud(bench::make_front_fixture(12,"uniform",7).points));
  const auto nodes=index->spatial_nodes();std::vector<WspdRectangle> request{{nodes[0].left,nodes[0].right,6}};
  const auto expected=run_q34_filter_batch_cpu(*index,5,request,1);const auto backend=cuda?ss::Backend::CUDA:ss::Backend::Portable;
  auto first=ss::Session::open(index,request,5,backend,7);auto second=ss::Session::open(index,request,5,backend,7);
  auto other_k=ss::Session::open(index,request,3,backend,7);auto prepared=ss::Prepared::build(first->rectangles(),1);
  request[0]={0,0,0};index.reset();same(first->consume(*prepared,7).output,expected);
  refuse([&]{static_cast<void>(second->consume(*prepared,7));},"resident.session_identity_or_closed");
  refuse([&]{static_cast<void>(other_k->consume(*prepared,7));},"resident.session_identity_or_closed");
  refuse([&]{static_cast<void>(first->consume(*prepared,0));},"resident.session_identity_or_closed");
  first->close();first->close();refuse([&]{static_cast<void>(first->consume(*prepared,7));},"resident.session_identity_or_closed");
  refuse([]{static_cast<void>(ss::Prepared::build({},1));},"resident.preparation_configuration");
}
void gate(bool cuda) {
  collector_gate();ownership_gate(cuda);
  for(const char* family:{"uniform","terrain","clusters","rows"}) {
    const auto index=make_q2_cloud_index(prepare_cloud(bench::make_front_fixture(12,family,3).points));
    for(unsigned k:{1U,2U,5U,10U}) for(unsigned s:{8U,10U,12U}) {
      std::vector<WspdRectangle> rectangles;
      static_cast<void>(run_wspd_front(*index,std::max(k,2U),s,WspdFrontMode::Pure,[&](const auto& r){
        rectangles.push_back({r.a_node,r.b_node,k==1?std::uint8_t{6}:r.lane_mask});},6));
      native_case(index,rectangles,k,cuda);
    }
  }
  bool found_zero=false;
  for(const char* family:{"uniform","terrain","clusters"}) for(bool reverse:{false,true}) {
    auto points=bench::make_front_fixture(64,family,3).points;if(reverse) std::reverse(points.begin(),points.end());
    const auto index=make_q2_cloud_index(prepare_cloud(points));const auto nodes=index->spatial_nodes();
    native_case(index,{},5,cuda);
    for(std::uint8_t mask:{2,4,6}) native_case(index,{{nodes[0].left,nodes[0].right,mask}},5,cuda);
    std::vector<WspdRectangle> mixed;size_t leaf=1;while(nodes[leaf].range.size()!=1) ++leaf;
    for(size_t other=leaf+1;other<nodes.size();++other) if(nodes[other].range.size()==1) mixed.push_back({leaf,other,6});
    mixed.push_back({nodes[0].left,nodes[0].right,6});native_case(index,mixed,3,cuda);
    if(!mixed.empty()) native_case(index,{mixed.front()},5,cuda);
    if(!found_zero) for(size_t a=1;a<nodes.size() && !found_zero;++a) for(size_t b=a+1;b<nodes.size() && !found_zero;++b) {
      const auto ar=nodes[a].range,br=nodes[b].range;if(!(ar.last<=br.first || br.last<=ar.first)) continue;
      const std::vector<WspdRectangle> request{{a,b,6}};const auto reference=run_q34_filter_batch_cpu(*index,3,request,1);
      if(reference.expanded_pairs && reference.survivors.empty()) {native_case(index,request,3,cuda);found_zero=true;}
    }
  }
  check(coverage.planned,"survivors_gate.nonvacuity_planned");check(coverage.fallbacks,"survivors_gate.nonvacuity_fallbacks");
  check(coverage.empty && coverage.survivors,"survivors_gate.nonvacuity_output");check(coverage.reordered,"survivors_gate.nonvacuity_reordered");
  check(found_zero && coverage.zero_output,"survivors_gate.nonvacuity_zero_output");
  check(coverage.high_keys && coverage.sparse_waves && coverage.growths,"survivors_gate.nonvacuity_collector");
  std::cout<<"{\"schema\":\"mhgp9_resident_survivors_v1\",\"status\":\"passed\",\"cuda_executed\":"<<(cuda?"true":"false")
    <<",\"collector_cases\":"<<coverage.collector_cases<<",\"refused\":"<<coverage.refused<<",\"cases\":"<<coverage.cases
    <<",\"calls\":"<<coverage.calls<<",\"queries\":"<<coverage.queries<<",\"S\":"<<coverage.survivors
    <<",\"reordered\":"<<coverage.reordered<<",\"empty\":"<<coverage.empty<<",\"zero_output\":"<<coverage.zero_output
    <<",\"sparse_waves\":"<<coverage.sparse_waves<<",\"high_keys\":"<<coverage.high_keys<<",\"growths\":"<<coverage.growths
    <<",\"max_array_bytes\":"<<coverage.max_array_bytes<<",\"planned\":"<<coverage.planned<<",\"fallbacks\":"<<coverage.fallbacks<<"}\n";
}
int main(int argc,char** argv) {
  try {const bool cuda=argc==2 && std::string_view(argv[1])=="--cuda";
    check(argc==1 || cuda,"survivors_probe.usage");gate(cuda);return 0;
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
