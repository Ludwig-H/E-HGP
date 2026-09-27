#include "runner.hpp"
#include "front_fixtures.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
namespace cw=mhgp9::audit::cuda_waves;
namespace wa=mhgp9::audit::waves;
using namespace mhgp9::gen;
using Clock=std::chrono::steady_clock;
double elapsed(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
void need(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
constexpr const char* schema="mhgp9_q34_cuda_waves_v1";
void same(const Q34FilterBatch& a,const Q34FilterBatch& b) {
  need(a.rectangle_masks==b.rectangle_masks,"cuda_gate.rectangle_masks");
  need(a.survivors==b.survivors,"cuda_gate.survivors_order_mask");
  need(a.expanded_pairs==b.expanded_pairs && a.pair_q3_rejected==b.pair_q3_rejected &&
       a.pair_q4_rejected==b.pair_q4_rejected && a.rectangle_visits==b.rectangle_visits,"cuda_gate.logical_counts");
}
u64 digest(const Q34FilterBatch& out) {
  u64 h=14695981039346656037ULL;const auto word=[&](u64 x){bench::front_hash_word(h,x);};
  word(out.expanded_pairs);word(out.pair_q3_rejected);word(out.pair_q4_rejected);
  for(auto m:out.rectangle_masks) word(m);
  for(const auto& e:out.survivors) {word(e.a_rank);word(e.b_rank);word(e.mask);}return h;
}
struct Coverage {u64 cases{},runs{},queries{},survivors{},planned{},fallbacks{},empty{},zero_output{},reordered{},mixed_masks{},pool_rejected{},pool_lane_reduced{},fallback_survivors{};};
void check_case(const Q2CensusIndexPtr& index,std::vector<WspdRectangle> rectangles,unsigned k,bool cuda,Coverage& c) {
  const auto reference=run_q34_filter_batch_cpu(*index,k,rectangles,1);
  const auto prepared=wa::Prepared::build(index,rectangles,k);
  const auto snapshot=cw::Snapshot::build(prepared);++c.cases;
  c.planned+=prepared->work().planned;c.fallbacks+=prepared->work().fallbacks;
  c.pool_rejected+=prepared->logical().all-prepared->effective().all;
  for(auto& r:rectangles) r={0,0,0}; // Snapshot/Prepared never adopt caller storage.
  const auto view=snapshot->view();
  for(u64 i=0;i<view.effective;++i) {
    cw::Edge e{};need(cw::decode(view,i,e),"cuda_gate.decode");
    if(e.mask!=6) ++c.mixed_masks;
    size_t seg=0;while(view.segments[seg].end<=i) ++seg;
    const auto& r=view.rectangles[view.segments[seg].rectangle];
    c.pool_lane_reduced+=e.mask!=r.mask;
    if(view.segments[seg].band==cw::absent) {u64 visits=0;c.fallback_survivors+=cw::point_filter(view,e,visits)!=0;}
  }
  for(size_t q:{size_t{1},size_t{7},size_t{257},size_t{0x80000011ULL}}) {
    cw::Counters counters;const auto portable=cw::portable(*snapshot,q,counters);same(portable,reference);
    // Device tiny-wave gate avoids thousands of launch/sync calls at Q1;
    // Q1 is still exercised on a small explicit rectangle below.
    if(cuda && (q==7 || q==257 || (view.effective<=4 && q==1))) {
      const auto result=cw::run_cuda(*snapshot,q);
      if(!result.error.empty()) throw std::runtime_error(result.error);
      need(result.available,"cuda_gate.device_required");same(result.output,reference);
      need(result.counters.queries==counters.queries && result.counters.q3==counters.q3 &&
           result.counters.q4==counters.q4 && result.counters.visits==counters.visits,"cuda_gate.device_physical_counts");
    }
    ++c.runs;c.queries+=counters.queries;c.survivors+=portable.survivors.size();c.reordered+=counters.out_of_order;
    c.empty+=view.effective==0;c.zero_output+=view.effective!=0 && portable.survivors.empty();
  }
}
void gate(bool cuda) {
  Coverage c;
  for(const char* family:{"uniform","terrain","clusters","rows"}) {
    const auto index=make_q2_cloud_index(prepare_cloud(bench::make_front_fixture(12,family,3).points));
    for(unsigned k:{1U,2U,5U,10U}) for(unsigned s:{8U,10U,12U}) {
      std::vector<WspdRectangle> rectangles;
      static_cast<void>(run_wspd_front(*index,std::max(k,2U),s,WspdFrontMode::Pure,[&](const auto& r){
        rectangles.push_back({r.a_node,r.b_node,k==1?std::uint8_t{6}:r.lane_mask});
      },6));
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
      for(size_t other=leaf+1;other<nodes.size();++other)
        if(nodes[other].range.size()==1) mixed.push_back({leaf,other,6});
      mixed.push_back({nodes[0].left,nodes[0].right,6});check_case(index,mixed,3,cuda,c);
      if(!mixed.empty()) check_case(index,{mixed.front()},5,cuda,c);
      if(!found_zero) for(size_t a=1;a<nodes.size() && !found_zero;++a)
        for(size_t b=a+1;b<nodes.size() && !found_zero;++b) {
          const auto ar=nodes[a].range,br=nodes[b].range;if(!(ar.last<=br.first || br.last<=ar.first)) continue;
          const std::vector<WspdRectangle> requests{{a,b,6}};
          const auto ref=run_q34_filter_batch_cpu(*index,3,requests,1);
          if(ref.expanded_pairs && ref.survivors.empty()) {
            const auto p=wa::Prepared::build(index,requests,3);
            if(p->effective().all) {check_case(index,requests,3,cuda,c);found_zero=true;}
          }
        }
    }
  }
  // >32-bit products/ordinals exercise only the decoding formula, not an
  // allocated billion-candidate fixture and never a claimed scale test.
  const cw::Rectangle r{(u64{1}<<32)+17,cw::absent,0,65536,65536,65536,6};
  const cw::Segment seg{0,cw::absent,u64{1}<<32};cw::View v{};
  v.rectangles=&r;v.segments=&seg;v.rectangle_count=1;v.segment_count=1;
  v.points_count=131072;v.effective=u64{1}<<32;v.k=5;cw::Edge edge{};
  need(cw::decode(v,(u64{1}<<32)-1,edge) && edge.a==65535 && edge.b==131071 && edge.ordinal==(u64{1}<<33)+16,"cuda_gate.u64_decode");
  need(!cw::decode(v,v.effective,edge),"cuda_gate.decode_eof");
  u64 out=0;need(!cw::add(cw::absent,1,out) && !cw::multiply(cw::absent,2,out),"cuda_gate.u64_overflow");
  need(c.planned && c.fallbacks && c.empty && c.zero_output && c.reordered && c.mixed_masks && found_zero &&
       c.pool_rejected && c.pool_lane_reduced && c.fallback_survivors,"cuda_gate.nonvacuity");
  std::cout<<"{\"schema\":\""<<schema<<"\",\"status\":\"passed\",\"mode\":\"gate\",\"cuda_executed\":"<<(cuda?"true":"false")
    <<",\"cases\":"<<c.cases<<",\"runs\":"<<c.runs<<",\"queries\":"<<c.queries<<",\"survivors\":"<<c.survivors
    <<",\"planned\":"<<c.planned<<",\"fallbacks\":"<<c.fallbacks<<",\"empty\":"<<c.empty<<",\"zero_output\":"<<c.zero_output
    <<",\"reordered\":"<<c.reordered<<",\"mixed_masks\":"<<c.mixed_masks<<",\"pool_rejected\":"<<c.pool_rejected
    <<",\"pool_lane_reduced\":"<<c.pool_lane_reduced<<",\"fallback_survivors\":"<<c.fallback_survivors<<"}\n";
}
std::vector<Point3> read_frame(const std::string& path) {
  std::ifstream file(path,std::ios::binary);need(bool(file),"frame.open");
  const std::vector<unsigned char> raw((std::istreambuf_iterator<char>(file)),{});
  need(!raw.empty() && raw.size()%12==0,"frame.shape");std::vector<Point3> out(raw.size()/12);
  for(size_t i=0;i<out.size();++i) {
    std::array<u32,3> xyz{};
    for(size_t d=0;d<3;++d) {for(size_t j=0;j<4;++j) xyz[d]|=u32(raw[12*i+4*d+j])<<(8*j);need(xyz[d]<=262143,"frame.u18");}
    out[i]={static_cast<Coordinate>(xyz[0]),static_cast<Coordinate>(xyz[1]),static_cast<Coordinate>(xyz[2])};
  }
  return out;
}
void measure(const std::string& path,size_t q,unsigned k,unsigned s) {
  const auto start=Clock::now();auto points=read_frame(path);const auto input_ms=elapsed(start);
  const auto n=points.size();
  u64 hash=14695981039346656037ULL;bench::front_hash_word(hash,1);bench::front_hash_word(hash,points.size());
  for(const auto p:points) for(const auto x:{p.x,p.y,p.z}) bench::front_hash_word(hash,x);
  auto tick=Clock::now();auto index=make_q2_cloud_index(prepare_cloud(points));const auto index_ms=elapsed(tick);
  tick=Clock::now();std::vector<WspdRectangle> rectangles;
  static_cast<void>(run_wspd_front(*index,k,s,WspdFrontMode::MidpointSamples,[&](const auto& r){rectangles.push_back(r);},6));
  const auto front_ms=elapsed(tick);tick=Clock::now();auto prepared=wa::Prepared::build(index,rectangles,k,4);const auto preparation_ms=elapsed(tick);
  tick=Clock::now();auto snapshot=cw::Snapshot::build(prepared);const auto snapshot_ms=elapsed(tick);
  auto result=cw::run_cuda(*snapshot,q);if(!result.error.empty()) throw std::runtime_error(result.error);need(result.available,"cuda_measure.device_required");
  tick=Clock::now();auto reference=run_q34_filter_batch_cpu(*index,k,rectangles,4);const auto reference_ms=elapsed(tick);
  tick=Clock::now();same(result.output,reference);const auto comparison_ms=elapsed(tick);
  const auto output_digest=digest(result.output),survivors=result.output.survivors.size();
  const auto logical=prepared->logical(),effective=prepared->effective();const auto retained=prepared->retained_bytes(),snapshot_bytes=snapshot->array_bytes();
  const auto counters=result.counters;const auto timing=result.timing;const auto device=result.device;tick=Clock::now();
  result.output={};reference={};snapshot.reset();prepared.reset();index.reset();std::vector<WspdRectangle>().swap(rectangles);std::vector<Point3>().swap(points);
  const auto destruction_ms=elapsed(tick),total_ms=elapsed(start);
  std::cout<<std::setprecision(12)<<"{\"schema\":\""<<schema<<"\",\"status\":\"passed\",\"mode\":\"frame\",\"cuda_executed\":true"
    <<",\"n\":"<<n<<",\"input_hash_u64\":"<<hash<<",\"k\":"<<k<<",\"s\":"<<s<<",\"Q_requested\":"<<q<<",\"Q_actual\":"<<timing.q_actual
    <<",\"preparation_workers\":4,\"reference_workers\":4,\"device\":"<<std::quoted(device)
    <<",\"P\":"<<logical.all<<",\"P3\":"<<logical.q3<<",\"P4\":"<<logical.q4<<",\"E\":"<<effective.all<<",\"E3\":"<<effective.q3<<",\"E4\":"<<effective.q4
    <<",\"S\":"<<survivors<<",\"digest_u64\":"<<output_digest<<",\"physical_queries\":"<<counters.queries<<",\"visits\":"<<counters.visits<<",\"waves\":"<<counters.waves
    <<",\"prepared_retained_bytes\":"<<retained<<",\"snapshot_array_bytes\":"<<snapshot_bytes<<",\"device_bytes\":"<<timing.device_bytes
    <<",\"upload_bytes\":"<<timing.upload_bytes<<",\"download_bytes\":"<<timing.download_bytes
    <<",\"times_ms\":{\"input\":"<<input_ms<<",\"index\":"<<index_ms<<",\"front\":"<<front_ms<<",\"preparation\":"<<preparation_ms
    <<",\"snapshot\":"<<snapshot_ms<<",\"cuda_runner\":"<<timing.total_ms<<",\"upload_allocate\":"<<timing.upload_ms
    <<",\"waves_including_count_download\":"<<timing.waves_ms<<",\"survivor_download_allocate\":"<<timing.download_ms
    <<",\"order_convert\":"<<timing.order_ms<<",\"runner_release\":"<<timing.release_ms<<",\"reference\":"<<reference_ms
    <<",\"comparison\":"<<comparison_ms<<",\"destruction\":"<<destruction_ms<<",\"total\":"<<total_ms<<"}}\n";
}
u64 number(const std::string& text,u64 maximum) {
  need(!text.empty(),"cuda_probe.number");u64 value=0;
  for(const auto c:text) {need(c>='0' && c<='9',"cuda_probe.number");const unsigned digit=static_cast<unsigned>(c-'0');
    need(value<=maximum/10 && (value<maximum/10 || digit<=maximum%10),"cuda_probe.number_range");value=value*10+digit;}
  return value;
}
int main(int argc,char** argv) {
  try {
    bool gate_mode=false,cuda=false;std::string path;size_t q=262144;unsigned k=5,s=8;
    for(int i=1;i<argc;++i) {
      const std::string arg=argv[i];
      if(arg=="--gate") gate_mode=true;else if(arg=="--cuda") cuda=true;
      else if(arg=="--frame" && i+1<argc) path=argv[++i];
      else if(arg.starts_with("--q=")) q=static_cast<size_t>(number(arg.substr(4),std::numeric_limits<size_t>::max()));
      else if(arg.starts_with("--k=")) k=static_cast<unsigned>(number(arg.substr(4),10));
      else if(arg.starts_with("--s=")) s=static_cast<unsigned>(number(arg.substr(4),12));
      else throw std::invalid_argument("cuda_probe.usage");
    }
    need(q>0 && k>=2 && k<=10 && (s==8 || s==10 || s==12),"cuda_probe.configuration");
    if(gate_mode && path.empty()) gate(cuda);
    else {need(!gate_mode && cuda && !path.empty(),"cuda_probe.mode");measure(path,q,k,s);}
    return 0;
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
