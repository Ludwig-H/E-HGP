// Explicit audit port of cuda_waves/probe.cpp at 33c1d28d7; new resident
// ownership, two-stage rectangle/pair waves, and complete adapter lifetime.
#include "host.hpp"
#include "../b_q34_cuda_waves_20260927/snapshot.hpp"
#include "front_fixtures.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>
namespace rr=mhgp9::audit::resident;
namespace cw=mhgp9::audit::cuda_waves;
namespace wa=mhgp9::audit::waves;
using namespace mhgp9::gen;
using cw::u32;
using Clock=std::chrono::steady_clock;
double elapsed(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
void need(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
constexpr const char* schema="mhgp9_q34_filtered_resident_v1";
void same(const Q34FilterBatch& a,const Q34FilterBatch& b) {
  need(a.rectangle_masks==b.rectangle_masks,"resident_gate.rectangle_masks");
  need(a.survivors==b.survivors,"resident_gate.survivors_order_mask");
  need(a.expanded_pairs==b.expanded_pairs && a.pair_q3_rejected==b.pair_q3_rejected &&
       a.pair_q4_rejected==b.pair_q4_rejected && a.rectangle_visits==b.rectangle_visits,"resident_gate.logical_counts");
}
u64 digest(const Q34FilterBatch& out) {
  u64 h=14695981039346656037ULL;const auto word=[&](u64 x){bench::front_hash_word(h,x);};
  word(out.expanded_pairs);word(out.pair_q3_rejected);word(out.pair_q4_rejected);
  for(auto m:out.rectangle_masks) word(m);
  for(const auto& e:out.survivors) {word(e.a_rank);word(e.b_rank);word(e.mask);}return h;
}
struct Coverage {
  u64 cases{},portable_runs{},cuda_runs{},queries{},survivors{},planned{},fallbacks{},empty{},zero_output{},
    reordered{},pool_rejected{},pool_lane_reduced{},fallback_survivors{},closed{},source_gaps{},arena_gaps{},
    raw_holes{},rectangle_waves{},pair_waves{},rejections{};
};
template<class F> void rejects(F&& f,Coverage& c,const char* expected) {
  try {f();} catch(const std::exception& e) {need(e.what()==std::string(expected),"resident_gate.refusal_reason");++c.rejections;return;}
  throw std::runtime_error("resident_gate.refusal_missing");
}
void check_case(const Q2CensusIndexPtr& index,const std::vector<WspdRectangle>& rectangles,unsigned k,bool cuda,Coverage& c) {
  const auto reference=run_q34_filter_batch_cpu(*index,k,rectangles,1);
  const auto baseline=wa::Prepared::build(index,rectangles,k);++c.cases;
  // Coverage oracle only; never used in the resident decision or frame path.
  const auto snapshot=cw::Snapshot::build(baseline);const auto view=snapshot->view();
  for(u64 i=0;i<view.effective;++i) {cw::Edge e{};need(cw::decode(view,i,e),"resident_gate.oracle_decode");
    size_t segment=0;while(view.segments[segment].end<=i) ++segment;
    c.pool_lane_reduced+=e.mask!=view.rectangles[view.segments[segment].rectangle].mask;}
  const auto nodes=index->spatial_nodes();
  cw::Counters first_counters{};bool have_first=false;
  const auto one=[&](rr::Backend backend,size_t qr,size_t q,unsigned workers) {
    auto caller=rectangles;auto session=rr::Session::open(index,caller,k,backend,qr);
    for(auto& r:caller) r={0,0,0}; // No surviving mutable alias to copied requests.
    auto decision=session->rectangles();
    need(std::vector<std::uint8_t>(decision->masks().begin(),decision->masks().end())==reference.rectangle_masks,"resident_gate.rectangle_masks");
    need(decision->logical()==baseline->logical(),"resident_gate.logical_mass");
    u64 raw=0,compact=0,arena=0;bool hole=false;
    for(size_t r=0;r<rectangles.size();++r) {
      const auto& in=rectangles[r];const auto mass=wa::product(nodes[in.a_node].range.size(),nodes[in.b_node].range.size());
      if(reference.rectangle_masks[r]) {
        need(compact<decision->compact().size(),"resident_gate.compact_count");const auto& out=decision->compact()[compact];
        need(out.source_ordinal==r && out.raw_base==raw && out.a_node==in.a_node && out.b_node==in.b_node &&
             out.mask==reference.rectangle_masks[r],"resident_gate.compact_identity");
        c.source_gaps+=compact!=r;c.raw_holes+=hole;
        if(baseline->rectangles()[r].arena_rectangle!=wa::absent) {c.arena_gaps+=arena!=compact && compact!=r;++arena;}
        ++compact;
      } else {++c.closed;hole=true;}
      raw=wa::sum(raw,mass);
    }
    need(compact==decision->compact().size() && raw==session->work().raw_pairs,"resident_gate.raw_mass");
    auto prepared=rr::Prepared::build(decision,workers);
    need(prepared->effective()==baseline->effective() && prepared->planned()==baseline->work().planned &&
         prepared->fallbacks()==baseline->work().fallbacks,"resident_gate.preparation");
    auto result=session->consume(*prepared,q);same(result.output,reference);
    need(result.counters.queries==prepared->effective().all,"resident_gate.queries");
    const auto& stats=result.counters;
    if(have_first) need(stats.queries==first_counters.queries && stats.q3==first_counters.q3 && stats.q4==first_counters.q4 &&
      stats.rejected3==first_counters.rejected3 && stats.rejected4==first_counters.rejected4 && stats.visits==first_counters.visits,
      "resident_gate.physical_counts");
    else {first_counters=stats;have_first=true;}
    if(backend==rr::Backend::CUDA) ++c.cuda_runs;else ++c.portable_runs;
    c.planned+=prepared->planned();c.fallbacks+=prepared->fallbacks();
    const auto p=decision->logical(),e=prepared->effective();
    c.pool_rejected+=p.all-e.all;
    c.queries+=result.counters.queries;c.survivors+=result.output.survivors.size();c.reordered+=result.counters.out_of_order;
    c.empty+=e.all==0;c.zero_output+=e.all!=0 && result.output.survivors.empty();
    if(!prepared->planned()) c.fallback_survivors+=result.output.survivors.size();
    c.rectangle_waves+=session->work().rectangle_waves;c.pair_waves+=result.counters.waves;
    rejects([&]{static_cast<void>(session->consume(*prepared,0));},c,"resident.session_identity_or_closed");
    prepared.reset();decision.reset();session->close();session->close();
  };
  // Small complete portable matrix; device avoids thousands of launch/sync Q1.
  one(rr::Backend::Portable,1,1,1);one(rr::Backend::Portable,7,257,4);
  one(rr::Backend::Portable,size_t{0x80000011ULL},size_t{0x80000011ULL},1);
  if(cuda) {one(rr::Backend::CUDA,7,7,4);one(rr::Backend::CUDA,257,257,4);
    if(baseline->effective().all<=4) one(rr::Backend::CUDA,1,1,1);}
}
void ownership_gate(bool cuda,Coverage& c) {
  static_assert(!std::is_default_constructible_v<rr::FilteredRectangles>);
  static_assert(!std::is_copy_constructible_v<rr::FilteredRectangles>);
  static_assert(!std::is_move_constructible_v<rr::FilteredRectangles>);
  auto index=make_q2_cloud_index(prepare_cloud(bench::make_front_fixture(12,"uniform",7).points));
  const auto nodes=index->spatial_nodes();const std::vector<WspdRectangle> requests{{nodes[0].left,nodes[0].right,6}};
  const auto backend=cuda?rr::Backend::CUDA:rr::Backend::Portable;
  auto a=rr::Session::open(index,requests,5,backend,7);auto p=rr::Prepared::build(a->rectangles(),1);
  auto b=rr::Session::open(index,requests,5,backend,7);
  rejects([&]{static_cast<void>(b->consume(*p,7));},c,"resident.session_identity_or_closed");
  auto other_k=rr::Session::open(index,requests,3,backend,7);
  rejects([&]{static_cast<void>(other_k->consume(*p,7));},c,"resident.session_identity_or_closed");
  auto index2=make_q2_cloud_index(prepare_cloud(bench::make_front_fixture(12,"uniform",7).points));
  auto other_index=rr::Session::open(index2,requests,5,backend,7);
  rejects([&]{static_cast<void>(other_index->consume(*p,7));},c,"resident.session_identity_or_closed");
  const auto ref=run_q34_filter_batch_cpu(*index,5,requests,1);index.reset();same(a->consume(*p,7).output,ref);
  a->close();rejects([&]{static_cast<void>(a->consume(*p,7));},c,"resident.session_identity_or_closed");
  rejects([&]{static_cast<void>(rr::Prepared::build({},1));},c,"resident.preparation_configuration");
  rejects([&]{static_cast<void>(rr::Prepared::build(b->rectangles(),0));},c,"resident.preparation_configuration");
  for(unsigned k:{0U,11U}) rejects([&]{static_cast<void>(rr::Session::open(index2,requests,k,backend,7));},c,"resident.configuration");
  rejects([&]{static_cast<void>(rr::Session::open({},requests,5,backend,7));},c,"resident.configuration");
  rejects([&]{static_cast<void>(rr::Session::open(index2,requests,5,backend,0));},c,"resident.configuration");
  rejects([&]{static_cast<void>(rr::Session::open(index2,requests,5,static_cast<rr::Backend>(7),7));},c,"resident.configuration");
  for(unsigned k:{1U,5U}) for(const WspdRectangle bad:std::vector<WspdRectangle>{{0,0,6},{nodes.size(),1,6},{1,2,0},{1,2,1},{1,2,8}}) {
    const std::vector<WspdRectangle> input{bad};
    rejects([&]{static_cast<void>(rr::Session::open(index2,input,k,backend,7));},c,
      bad.a_node==0 && bad.b_node==0?"resident.disjoint_factors":"resident.rectangle");
  }
}
void gate(bool cuda) {
  Coverage c;
  for(const char* family:{"uniform","terrain","clusters","rows"}) {
    const auto index=make_q2_cloud_index(prepare_cloud(bench::make_front_fixture(12,family,3).points));
    for(unsigned k:{1U,2U,5U,10U}) for(unsigned s:{8U,10U,12U}) {
      std::vector<WspdRectangle> rectangles;
      static_cast<void>(run_wspd_front(*index,std::max(k,2U),s,WspdFrontMode::Pure,[&](const auto& r){
        rectangles.push_back({r.a_node,r.b_node,k==1?std::uint8_t{6}:r.lane_mask});},6));
      check_case(index,rectangles,k,cuda,c);
    }
  }
  bool found_zero=false;
  for(const char* family:{"uniform","terrain","clusters"}) {
    auto points=bench::make_front_fixture(64,family,3).points;
    for(bool reverse:{false,true}) {
      if(reverse) std::reverse(points.begin(),points.end());
      const auto index=make_q2_cloud_index(prepare_cloud(points));const auto nodes=index->spatial_nodes();
      for(std::uint8_t mask:{2,4,6}) check_case(index,{{nodes[0].left,nodes[0].right,mask}},5,cuda,c);
      check_case(index,{},5,cuda,c);
      std::vector<WspdRectangle> mixed;size_t leaf=1;while(nodes[leaf].range.size()!=1) ++leaf;
      for(size_t other=leaf+1;other<nodes.size();++other) if(nodes[other].range.size()==1) mixed.push_back({leaf,other,6});
      mixed.push_back({nodes[0].left,nodes[0].right,6});check_case(index,mixed,3,cuda,c);
      if(!mixed.empty()) check_case(index,{mixed.front()},5,cuda,c);
      if(!found_zero) for(size_t a=1;a<nodes.size() && !found_zero;++a) for(size_t b=a+1;b<nodes.size() && !found_zero;++b) {
        const auto ar=nodes[a].range,br=nodes[b].range;if(!(ar.last<=br.first || br.last<=ar.first)) continue;
        const std::vector<WspdRectangle> request{{a,b,6}};const auto ref=run_q34_filter_batch_cpu(*index,3,request,1);
        if(ref.expanded_pairs && ref.survivors.empty() && wa::Prepared::build(index,request,3)->effective().all) {
          check_case(index,request,3,cuda,c);found_zero=true;}
      }
    }
  }
  ownership_gate(cuda,c);
  const cw::Rectangle r{(u64{1}<<32)+17,cw::absent,0,65536,65536,65536,6};
  const cw::Segment seg{0,cw::absent,u64{1}<<32};cw::View v{};v.rectangles=&r;v.segments=&seg;
  v.rectangle_count=1;v.segment_count=1;v.points_count=131072;v.effective=u64{1}<<32;v.k=5;cw::Edge edge{};
  need(cw::decode(v,(u64{1}<<32)-1,edge) && edge.a==65535 && edge.b==131071 && edge.ordinal==(u64{1}<<33)+16,"resident_gate.u64_decode");
  need(!cw::decode(v,v.effective,edge),"resident_gate.decode_eof");u64 out=0;
  need(!cw::add(cw::absent,1,out) && !cw::multiply(cw::absent,2,out),"resident_gate.u64_overflow");
  need(c.planned && c.fallbacks && c.empty && c.zero_output && c.reordered && found_zero && c.pool_rejected &&
       c.pool_lane_reduced && c.fallback_survivors && c.source_gaps && c.arena_gaps && c.raw_holes,"resident_gate.nonvacuity");
  std::cout<<"{\"schema\":\""<<schema<<"\",\"status\":\"passed\",\"mode\":\"gate\",\"cuda_executed\":"<<(cuda?"true":"false")
    <<",\"cases\":"<<c.cases<<",\"portable_runs\":"<<c.portable_runs<<",\"cuda_runs\":"<<c.cuda_runs<<",\"queries\":"<<c.queries
    <<",\"survivors\":"<<c.survivors<<",\"planned\":"<<c.planned<<",\"fallbacks\":"<<c.fallbacks<<",\"empty\":"<<c.empty
    <<",\"zero_output\":"<<c.zero_output<<",\"reordered\":"<<c.reordered<<",\"pool_rejected\":"<<c.pool_rejected
    <<",\"pool_lane_reduced\":"<<c.pool_lane_reduced<<",\"fallback_survivors\":"<<c.fallback_survivors<<",\"closed\":"<<c.closed
    <<",\"source_gaps\":"<<c.source_gaps<<",\"arena_gaps\":"<<c.arena_gaps<<",\"raw_holes\":"<<c.raw_holes
    <<",\"rectangle_waves\":"<<c.rectangle_waves<<",\"pair_waves\":"<<c.pair_waves<<",\"rejections\":"<<c.rejections<<"}\n";
}
std::vector<Point3> read_frame(const std::string& path) {
  std::ifstream file(path,std::ios::binary);need(bool(file),"frame.open");
  const std::vector<unsigned char> raw((std::istreambuf_iterator<char>(file)),{});
  need(!raw.empty() && raw.size()%12==0,"frame.shape");std::vector<Point3> out(raw.size()/12);
  for(size_t i=0;i<out.size();++i) {std::array<u32,3> xyz{};
    for(size_t d=0;d<3;++d) {for(size_t j=0;j<4;++j) xyz[d]|=u32(raw[12*i+4*d+j])<<(8*j);need(xyz[d]<=262143,"frame.u18");}
    out[i]={static_cast<Coordinate>(xyz[0]),static_cast<Coordinate>(xyz[1]),static_cast<Coordinate>(xyz[2])};}
  return out;
}
void measure(const std::string& path,size_t qr,size_t q,unsigned workers,unsigned k,unsigned s,bool cuda) {
  const auto start=Clock::now();auto points=read_frame(path);const auto input_ms=elapsed(start);const auto n=points.size();
  u64 hash=14695981039346656037ULL;bench::front_hash_word(hash,1);bench::front_hash_word(hash,n);
  for(const auto p:points) for(const auto x:{p.x,p.y,p.z}) bench::front_hash_word(hash,x);
  auto tick=Clock::now();auto index=make_q2_cloud_index(prepare_cloud(points));const auto index_ms=elapsed(tick);
  tick=Clock::now();std::vector<WspdRectangle> rectangles;
  static_cast<void>(run_wspd_front(*index,k,s,WspdFrontMode::MidpointSamples,[&](const auto& r){rectangles.push_back(r);},6));
  const auto front_ms=elapsed(tick);const auto adapter_start=Clock::now();
  auto session=rr::Session::open(index,rectangles,k,cuda?rr::Backend::CUDA:rr::Backend::Portable,qr);
  const auto open=session->work();const auto device=session->device_name();
  const auto logical=session->rectangles()->logical();const auto decision_bytes=session->rectangles()->retained_bytes();
  tick=Clock::now();auto prepared=rr::Prepared::build(session->rectangles(),workers);const auto arena_ms=elapsed(tick);
  const auto effective=prepared->effective();const auto prepared_bytes=prepared->retained_bytes();
  const auto before_release_bytes=prepared->before_arena_release_bytes(),arena_peak=prepared->arena_owned_peak_bound();
  const auto planned=prepared->planned(),fallbacks=prepared->fallbacks();auto result=session->consume(*prepared,q);
  tick=Clock::now();prepared.reset();session->close();session.reset();const auto adapter_cleanup_ms=elapsed(tick);
  const auto adapter_ms=elapsed(adapter_start); // All adapter owners released; S still retained.
  tick=Clock::now();auto reference=run_q34_filter_batch_cpu(*index,k,rectangles,4);const auto reference_ms=elapsed(tick);
  tick=Clock::now();same(result.output,reference);const auto comparison_ms=elapsed(tick);
  const auto output_digest=digest(result.output),survivors=result.output.survivors.size();const auto counters=result.counters;const auto timing=result.timing;
  tick=Clock::now();result.output={};reference={};index.reset();std::vector<WspdRectangle>().swap(rectangles);std::vector<Point3>().swap(points);
  const auto destruction_ms=elapsed(tick),total_ms=elapsed(start);
  std::cout<<std::setprecision(12)<<"{\"schema\":\""<<schema<<"\",\"status\":\"passed\",\"mode\":\"frame\",\"cuda_executed\":"<<(cuda?"true":"false")
    <<",\"n\":"<<n<<",\"input_hash_u64\":"<<hash<<",\"k\":"<<k<<",\"s\":"<<s<<",\"Q_requested\":"<<q<<",\"Q_actual\":"<<timing.q_actual
    <<",\"Qr_requested\":"<<qr<<",\"workers_arena\":"<<workers<<",\"workers_front\":1,\"reference_workers\":4,\"device\":"<<std::quoted(device)
    <<",\"R\":"<<open.R<<",\"R_live\":"<<open.R-open.closed<<",\"raw_pairs\":"<<open.raw_pairs<<",\"raw_q3\":"<<open.raw_q3<<",\"raw_q4\":"<<open.raw_q4
    <<",\"P\":"<<logical.all<<",\"P3\":"<<logical.q3<<",\"P4\":"<<logical.q4<<",\"E\":"<<effective.all<<",\"E3\":"<<effective.q3<<",\"E4\":"<<effective.q4
    <<",\"S\":"<<survivors<<",\"digest_u64\":"<<output_digest<<",\"physical_queries\":"<<counters.queries<<",\"visits\":"<<counters.visits<<",\"waves\":"<<counters.waves
    <<",\"rectangle_visits\":"<<open.rectangle_visits<<",\"rectangle_waves\":"<<open.rectangle_waves<<",\"planned\":"<<planned<<",\"fallbacks\":"<<fallbacks
    <<",\"decision_retained_bytes\":"<<decision_bytes<<",\"prepared_retained_bytes\":"<<prepared_bytes<<",\"before_arena_release_bytes\":"<<before_release_bytes
    <<",\"arena_owned_peak_bound\":"<<arena_peak<<",\"temporary_input_bytes\":"<<open.temporary_input_bytes<<",\"resident_device_bytes\":"<<open.resident_bytes
    <<",\"rectangle_device_bytes\":"<<open.rectangle_device_peak_bytes<<",\"pair_device_bytes\":"<<timing.device_bytes
    <<",\"rectangle_upload_bytes\":"<<open.rectangle_upload_bytes<<",\"rectangle_download_bytes\":"<<open.rectangle_download_bytes
    <<",\"pair_upload_bytes\":"<<timing.upload_bytes<<",\"pair_download_bytes\":"<<timing.download_bytes
    <<",\"times_ms\":{\"input\":"<<input_ms<<",\"index\":"<<index_ms<<",\"front\":"<<front_ms<<",\"open\":"<<open.total_ms
    <<",\"input_copy_validate\":"<<open.input_ms<<",\"index_copy\":"<<open.index_copy_ms<<",\"cuda_init\":"<<open.init_ms<<",\"index_upload\":"<<open.index_upload_ms
    <<",\"rectangle_upload_allocate\":"<<open.rectangle_upload_ms<<",\"rectangle_kernel\":"<<open.rectangle_kernel_ms
    <<",\"rectangle_download_allocate\":"<<open.rectangle_download_ms<<",\"rectangle_release\":"<<open.rectangle_release_ms<<",\"compaction\":"<<open.compaction_ms
    <<",\"input_release\":"<<open.input_release_ms<<",\"arena\":"<<arena_ms<<",\"consume\":"<<timing.total_ms<<",\"pair_upload_allocate\":"<<timing.upload_ms
    <<",\"waves_including_count_download\":"<<timing.waves_ms<<",\"survivor_download_allocate\":"<<timing.download_ms
    <<",\"order_convert\":"<<timing.order_ms<<",\"pair_release\":"<<timing.release_ms<<",\"adapter_cleanup\":"<<adapter_cleanup_ms
    <<",\"adapter\":"<<adapter_ms<<",\"reference\":"<<reference_ms<<",\"comparison\":"<<comparison_ms
    <<",\"destruction\":"<<destruction_ms<<",\"total\":"<<total_ms<<"}}\n";
}
u64 number(const std::string& text,u64 maximum) {
  need(!text.empty(),"resident_probe.number");u64 value=0;
  for(const auto c:text) {need(c>='0' && c<='9',"resident_probe.number");const unsigned digit=static_cast<unsigned>(c-'0');
    need(value<=maximum/10 && (value<maximum/10 || digit<=maximum%10),"resident_probe.number_range");value=value*10+digit;}return value;
}
int main(int argc,char** argv) {
  try {
    bool gate_mode=false,cuda=false;std::string path;size_t qr=262144,q=262144;unsigned workers=4,k=5,s=8;
    for(int i=1;i<argc;++i) {
      const std::string arg=argv[i];
      if(arg=="--gate") gate_mode=true;else if(arg=="--cuda") cuda=true;
      else if(arg=="--frame" && i+1<argc) path=argv[++i];
      else if((arg=="--q" || arg=="--qr" || arg=="--workers" || arg=="--k" || arg=="--s") && i+1<argc) {
        const std::string val=argv[++i];
        if(arg=="--q") q=static_cast<size_t>(number(val,std::numeric_limits<size_t>::max()));
        else if(arg=="--qr") qr=static_cast<size_t>(number(val,std::numeric_limits<size_t>::max()));
        else if(arg=="--workers") workers=static_cast<unsigned>(number(val,std::numeric_limits<unsigned>::max()));
        else if(arg=="--k") k=static_cast<unsigned>(number(val,10));else s=static_cast<unsigned>(number(val,12));
      } else throw std::invalid_argument("resident_probe.usage");
    }
    need(q>0 && qr>0 && workers>0 && k>=2 && k<=10 && (s==8 || s==10 || s==12),"resident_probe.configuration");
    if(gate_mode && path.empty()) gate(cuda);
    else {need(!gate_mode && !path.empty(),"resident_probe.mode");measure(path,qr,q,workers,k,s,cuda);}
    return 0;
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
