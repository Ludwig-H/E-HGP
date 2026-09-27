// Narrow benchmark port of b_q34_filtered_resident/probe.cpp at af369c44:
// exact input bytes/hash/front reused explicitly; no front/census source copy.
#include "../b_q34_filtered_resident_20260927/host.hpp"
#include "../b_q34_resident_survivors_20260927/host.hpp"
#include "front_fixtures.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string_view>
namespace rr=mhgp9::audit::resident;
namespace ss=mhgp9::audit::resident_survivors;
namespace cw=mhgp9::audit::cuda_waves;
namespace wa=mhgp9::audit::waves;
namespace fp=mhgp9::audit::factor_plan;
namespace ca=mhgp9::audit::collective;
using namespace mhgp9::gen;
using Clock=std::chrono::steady_clock;
using Counts=std::map<std::string,u64>;
using Times=std::map<std::string,double>;
constexpr const char* schema="mhgp9_survivors_compare_v1";
constexpr size_t frame_q=262144,frame_qr=262144;
double ms(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
void need(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
void same_native(const Q34FilterBatch& got,const Q34FilterBatch& expected) {
  need(got.rectangle_masks==expected.rectangle_masks && got.survivors==expected.survivors,"compare.native_output");
  need(got.expanded_pairs==expected.expanded_pairs && got.pair_q3_rejected==expected.pair_q3_rejected &&
       got.pair_q4_rejected==expected.pair_q4_rejected && got.rectangle_visits==expected.rectangle_visits,"compare.native_counts");
}
u64 digest(const Q34FilterBatch& out) {
  u64 h=14695981039346656037ULL;const auto word=[&](u64 x){bench::front_hash_word(h,x);};
  word(out.expanded_pairs);word(out.pair_q3_rejected);word(out.pair_q4_rejected);
  for(auto m:out.rectangle_masks) word(m);
  for(const auto& e:out.survivors) {word(e.a_rank);word(e.b_rank);word(e.mask);}return h;
}
Counts geometry(const fp::Work& w) {
  Counts out;
#define FIELD(f) out[#f]=w.f
  FIELD(factor_sites);FIELD(selection_visits);FIELD(selection_tests);FIELD(selection_shifts);FIELD(selected_sites);
  FIELD(anchor_visits);FIELD(witness_attempts);FIELD(self_skips);FIELD(q3_credits);FIELD(q4_credits);FIELD(grouping_visits);
  FIELD(grouping_sort_tests);FIELD(occupied_classes);FIELD(class_slots);FIELD(cells_tested);FIELD(descriptors);
#undef FIELD
#define FIELD(f) out["predicates_" #f]=w.predicates.f
  FIELD(point_tests);FIELD(universal_queries);FIELD(q2_axis_terms);FIELD(corner_tests);FIELD(block_bound_tests);FIELD(negative_probes);
#undef FIELD
  return out;
}
Counts arena_work(const ca::ExtraWork& w) {
  Counts out;
#define FIELD(f) out[#f]=w.f
  FIELD(requests);FIELD(factors);FIELD(anchor_jobs);FIELD(max_factor);FIELD(grouping_reads);FIELD(scatter_writes);
  FIELD(histogram_slots);FIELD(class_visits);FIELD(band_passes);FIELD(band_classes_b);FIELD(band_classes_a);
  FIELD(band_rows);FIELD(band_binary_tests);FIELD(prefix_entries);FIELD(emitted_bands);
#undef FIELD
  return out;
}
Counts physical(const cw::Counters& w) {
  return {{"queries",w.queries},{"q3",w.q3},{"q4",w.q4},{"rejected3",w.rejected3},{"rejected4",w.rejected4},
    {"visits",w.visits},{"waves",w.waves},{"out_of_order",w.out_of_order}};
}
struct Record {
  std::string implementation,device;unsigned position{};bool first_cuda_call{},first_implementation_call{},output_memory_available{};
  Counts masses,counters,geometry,arena,memory;Times times;
};
template<bool Candidate> struct Types;
template<> struct Types<false> {using Session=rr::Session;using Prepared=rr::Prepared;using Backend=rr::Backend;};
template<> struct Types<true> {using Session=ss::Session;using Prepared=ss::Prepared;using Backend=ss::Backend;};
template<bool Candidate> Record operate(const Q2CensusIndexPtr& index,const std::vector<WspdRectangle>& requests,
    const Q34FilterBatch& reference,unsigned k,unsigned workers,bool cuda,size_t qr,size_t q,unsigned position,bool process_first) {
  using T=Types<Candidate>;const auto operation_start=Clock::now(),adapter_start=operation_start;
  auto session=T::Session::open(index,requests,k,cuda?T::Backend::CUDA:T::Backend::Portable,qr);
  const auto open=session->work();const auto device=session->device_name();const auto p=session->rectangles()->logical();
  const auto decision_bytes=session->rectangles()->retained_bytes();
  auto tick=Clock::now();auto prepared=T::Prepared::build(session->rectangles(),workers);const auto arena_ms=ms(tick);
  const auto e=prepared->effective();const auto gw=prepared->geometry_work();const auto aw=prepared->arena_work();
  const auto prepared_bytes=prepared->retained_bytes(),before_release=prepared->before_arena_release_bytes();
  const auto arena_peak=prepared->arena_owned_peak_bound(),planned=prepared->planned(),fallbacks=prepared->fallbacks();
  tick=Clock::now();auto result=session->consume(*prepared,q);const auto consume_outer_ms=ms(tick);
  tick=Clock::now();prepared.reset();session->close();session.reset();const auto cleanup_ms=ms(tick),adapter_ms=ms(adapter_start);
  // All operator owners released. Only the returned native output remains.
  tick=Clock::now();same_native(result.output,reference);const auto comparison_ms=ms(tick);
  need(result.counters.queries==e.all && result.counters.q3==e.q3 && result.counters.q4==e.q4 &&
    result.output.pair_visits==result.counters.visits,"compare.physical_counts");
  u64 s3=0,s4=0;for(const auto& edge:result.output.survivors) {s3+=(edge.mask&2U)!=0;s4+=(edge.mask&4U)!=0;}
  need(s3==p.q3-result.output.pair_q3_rejected && s4==p.q4-result.output.pair_q4_rejected &&
    s3==e.q3-result.counters.rejected3 && s4==e.q4-result.counters.rejected4,"compare.lane_mass_identity");
  Record out;out.implementation=Candidate?"survivors_6af40d886":"resident_af369c44";out.device=device;out.position=position;
  out.first_cuda_call=cuda && process_first && position==0;
  out.first_implementation_call=cuda && process_first && position<2;out.output_memory_available=Candidate;
  out.geometry=geometry(gw);out.arena=arena_work(aw);out.counters=physical(result.counters);
  out.masses={{"R",open.R},{"R_live",open.R-open.closed},{"raw_pairs",open.raw_pairs},{"raw_q3",open.raw_q3},{"raw_q4",open.raw_q4},
    {"P",p.all},{"P3",p.q3},{"P4",p.q4},{"E",e.all},{"E3",e.q3},{"E4",e.q4},{"S",result.output.survivors.size()},
    {"S3",s3},{"S4",s4},{"digest_u64",digest(result.output)},{"pair_q3_rejected",result.output.pair_q3_rejected},
    {"pair_q4_rejected",result.output.pair_q4_rejected},{"rectangle_visits",open.rectangle_visits},
    {"rectangle_waves",open.rectangle_waves},{"planned",planned},{"fallbacks",fallbacks},{"Q_actual",result.timing.q_actual}};
  const auto& t=result.timing;
  out.memory={{"decision_retained_bytes",decision_bytes},{"prepared_retained_bytes",prepared_bytes},{"before_arena_release_bytes",before_release},
    {"arena_owned_peak_bound",arena_peak},{"temporary_input_bytes",open.temporary_input_bytes},{"resident_device_bytes",open.resident_bytes},
    {"rectangle_device_bytes",open.rectangle_device_peak_bytes},{"pair_device_bytes",t.device_bytes},
    {"rectangle_upload_bytes",open.rectangle_upload_bytes},{"rectangle_download_bytes",open.rectangle_download_bytes},
    {"pair_upload_bytes",t.upload_bytes},{"pair_download_bytes",t.download_bytes},
    {"native_retained_bytes",wa::sum(wa::product(result.output.survivors.capacity(),sizeof(Q34SurvivingEdge)),result.output.rectangle_masks.capacity())}};
  out.times={{"open",open.total_ms},{"input_copy_validate",open.input_ms},{"index_copy",open.index_copy_ms},{"cuda_init",open.init_ms},
    {"index_upload",open.index_upload_ms},{"rectangle_upload_allocate",open.rectangle_upload_ms},{"rectangle_kernel",open.rectangle_kernel_ms},
    {"rectangle_download_allocate",open.rectangle_download_ms},{"rectangle_release",open.rectangle_release_ms},{"compaction",open.compaction_ms},
    {"input_release",open.input_release_ms},{"arena",arena_ms},{"consume",t.total_ms},{"consume_outer",consume_outer_ms},
    {"pair_upload_allocate",t.upload_ms},{"waves_including_count_download",t.waves_ms},{"survivor_download_allocate",t.download_ms},
    {"order_convert",t.order_ms},{"pair_release",t.release_ms},{"adapter_cleanup",cleanup_ms},{"adapter",adapter_ms},{"comparison",comparison_ms}};
  if constexpr(Candidate) {
    need(result.debug_keys.empty() && result.memory.debug_key_bytes==0,"compare.no_debug_transport");
    const auto& m=result.memory;const auto& ot=result.output_times;
#define FIELD(f) out.memory["output_" #f]=m.f
    FIELD(size);FIELD(capacity);FIELD(growths);FIELD(growth_copy_bytes);FIELD(append_copy_bytes);FIELD(edge_peak_bytes);
    FIELD(array_peak_bytes);FIELD(sort_scratch_bytes);FIELD(host_payload_bytes);FIELD(debug_key_bytes);FIELD(host_conversion_peak_bytes);
#undef FIELD
    out.times.insert({{"output_append",ot.append_ms},{"output_split",ot.split_ms},{"output_sort",ot.sort_ms},
      {"output_validate",ot.validate_ms},{"output_download",ot.download_ms},{"output_release",ot.release_ms}});
  }
  tick=Clock::now();result.output={};const auto native_release=ms(tick);
  out.times["native_output_release"]=native_release;
  out.times["adapter_plus_native_release_noncontiguous"]=adapter_ms+native_release;
  out.times["operation_observed"]=ms(operation_start);
  out.times["diagnostic_overhead"]=out.times["operation_observed"]-adapter_ms-comparison_ms-native_release;
  need(out.times["diagnostic_overhead"]>=0,"compare.clock_partition");
  return out;
}
std::vector<Record> paired(const Q2CensusIndexPtr& index,const std::vector<WspdRectangle>& requests,const Q34FilterBatch& reference,
    unsigned k,unsigned workers,bool cuda,size_t qr,size_t q,bool candidate_first,bool process_first=true) {
  std::vector<Record> out;out.reserve(4);
  for(unsigned position=0;position<4;++position) {
    const bool candidate=(position==0 || position==3)?candidate_first:!candidate_first;
    out.push_back(candidate?operate<true>(index,requests,reference,k,workers,cuda,qr,q,position,process_first):
      operate<false>(index,requests,reference,k,workers,cuda,qr,q,position,process_first));
    // Outputs were compared to one common native reference and destroyed before
    // the next operator; pairwise equality is exact by that common reference.
    if(out.size()>1) need(out.back().masses==out.front().masses && out.back().counters==out.front().counters &&
      out.back().geometry==out.front().geometry && out.back().arena==out.front().arena && out.back().device==out.front().device,
      "compare.paired_exact_work");
  }
  return out;
}
std::vector<Point3> read_frame(const std::string& path) {
  std::ifstream file(path,std::ios::binary);need(bool(file),"frame.open");
  const std::vector<unsigned char> raw((std::istreambuf_iterator<char>(file)),{});
  need(!raw.empty() && raw.size()%12==0,"frame.shape");std::vector<Point3> out(raw.size()/12);
  for(size_t i=0;i<out.size();++i) {std::array<std::uint32_t,3> xyz{};
    for(size_t d=0;d<3;++d) {for(size_t j=0;j<4;++j) xyz[d]|=std::uint32_t(raw[12*i+4*d+j])<<(8*j);need(xyz[d]<=262143,"frame.u18");}
    out[i]={static_cast<Coordinate>(xyz[0]),static_cast<Coordinate>(xyz[1]),static_cast<Coordinate>(xyz[2])};}
  return out;
}
template<class V> void emit_map(const std::map<std::string,V>& map) {
  std::cout<<'{';bool first=true;for(const auto& item:map) {if(!first) std::cout<<',';first=false;
    std::cout<<std::quoted(item.first)<<':'<<item.second;}std::cout<<'}';
}
void emit_record(const Record& r) {
  std::cout<<"{\"implementation\":"<<std::quoted(r.implementation)<<",\"position\":"<<r.position
    <<",\"first_cuda_call\":"<<(r.first_cuda_call?"true":"false")
    <<",\"first_implementation_call\":"<<(r.first_implementation_call?"true":"false")<<",\"device\":"<<std::quoted(r.device)
    <<",\"output_memory_available\":"<<(r.output_memory_available?"true":"false")<<",\"masses\":";
  emit_map(r.masses);std::cout<<",\"counters\":";emit_map(r.counters);std::cout<<",\"geometry\":";emit_map(r.geometry);
  std::cout<<",\"arena\":";emit_map(r.arena);std::cout<<",\"memory\":";emit_map(r.memory);std::cout<<",\"times_ms\":";emit_map(r.times);std::cout<<'}';
}
void frame(const std::string& path,unsigned workers,bool candidate_first) {
  const auto start=Clock::now();auto points=read_frame(path);const auto input_ms=ms(start);const auto n=points.size();
  u64 hash=14695981039346656037ULL;bench::front_hash_word(hash,1);bench::front_hash_word(hash,n);
  for(const auto p:points) for(const auto x:{p.x,p.y,p.z}) bench::front_hash_word(hash,x);
  auto tick=Clock::now();auto index=make_q2_cloud_index(prepare_cloud(points));const auto index_ms=ms(tick);
  tick=Clock::now();std::vector<WspdRectangle> requests;
  static_cast<void>(run_wspd_front(*index,5,8,WspdFrontMode::MidpointSamples,[&](const auto& r){requests.push_back(r);},6));
  const auto front_ms=ms(tick);
  tick=Clock::now();auto reference=run_q34_filter_batch_cpu(*index,5,requests,4);const auto reference_ms=ms(tick);
  auto results=paired(index,requests,reference,5,workers,true,frame_qr,frame_q,candidate_first);
  tick=Clock::now();reference={};index.reset();std::vector<WspdRectangle>().swap(requests);std::vector<Point3>().swap(points);
  const auto common_release_ms=ms(tick),total_ms=ms(start);
  std::cout<<std::setprecision(12)<<"{\"schema\":\""<<schema<<"\",\"status\":\"passed\",\"mode\":\"frame\",\"cuda_executed\":true"
    <<",\"n\":"<<n<<",\"input_hash_u64\":"<<hash<<",\"k\":5,\"s\":8,\"Q\":"<<frame_q<<",\"Qr\":"<<frame_qr
    <<",\"workers_arena\":"<<workers<<",\"workers_front\":1,\"reference_workers\":4,\"first\":\""<<(candidate_first?"candidate":"baseline")
    <<"\",\"sequence\":\""<<(candidate_first?"BAAB":"ABBA")<<"\",\"common_times_ms\":";
  emit_map(Times{{"input",input_ms},{"index",index_ms},{"front",front_ms},{"reference",reference_ms},{"common_release",common_release_ms},{"total",total_ms}});
  std::cout<<",\"runs\":[";for(size_t i=0;i<results.size();++i) {if(i) std::cout<<',';emit_record(results[i]);}std::cout<<"]}\n";
}
void gate(bool cuda) {
  u64 cases=0,operators=0,queries=0,survivors=0,planned=0,fallbacks=0,reordered=0,empty=0,first_cuda_calls=0,first_implementation_calls=0;
  const auto test=[&](const Q2CensusIndexPtr& index,const std::vector<WspdRectangle>& requests,unsigned k,unsigned workers,bool first) {
    const auto reference=run_q34_filter_batch_cpu(*index,k,requests,1);
    const auto result=paired(index,requests,reference,k,workers,cuda,7,7,first,cases==0);++cases;operators+=result.size();
    for(const auto& r:result) {queries+=r.counters.at("queries");survivors+=r.masses.at("S");planned+=r.masses.at("planned");
      fallbacks+=r.masses.at("fallbacks");reordered+=r.counters.at("out_of_order");empty+=r.masses.at("E")==0;
      first_cuda_calls+=r.first_cuda_call;first_implementation_calls+=r.first_implementation_call;}
  };
  for(const char* family:{"uniform","terrain","clusters","rows"}) {
    const auto index=make_q2_cloud_index(prepare_cloud(bench::make_front_fixture(16,family,3).points));
    for(unsigned k:{2U,5U,10U}) for(bool first:{false,true}) for(unsigned workers:{1U,4U}) {
      std::vector<WspdRectangle> requests;
      static_cast<void>(run_wspd_front(*index,k,8,WspdFrontMode::MidpointSamples,[&](const auto& r){requests.push_back(r);},6));
      test(index,requests,k,workers,first);
    }
  }
  for(bool first:{false,true}) {
    auto points=bench::make_front_fixture(64,"uniform",3).points;std::reverse(points.begin(),points.end());
    const auto index=make_q2_cloud_index(prepare_cloud(points));const auto nodes=index->spatial_nodes();
    test(index,{},5,4,first);test(index,{{nodes[0].left,nodes[0].right,6}},5,4,first);
  }
  need(queries && survivors && planned && fallbacks && reordered && empty,"compare.gate_nonvacuity");
  need(first_cuda_calls==(cuda?1U:0U) && first_implementation_calls==(cuda?2U:0U),"compare.gate_first_calls");
  std::cout<<"{\"schema\":\""<<schema<<"\",\"status\":\"passed\",\"mode\":\"gate\",\"cuda_executed\":"<<(cuda?"true":"false")
    <<",\"cases\":"<<cases<<",\"operators\":"<<operators<<",\"queries\":"<<queries<<",\"S\":"<<survivors
    <<",\"planned\":"<<planned<<",\"fallbacks\":"<<fallbacks<<",\"reordered\":"<<reordered<<",\"empty\":"<<empty
    <<",\"first_cuda_calls\":"<<first_cuda_calls<<",\"first_implementation_calls\":"<<first_implementation_calls<<"}\n";
}
int main(int argc,char** argv) {
  try {
    if((argc==2 || argc==3) && std::string_view(argv[1])=="--gate") {
      need(argc==2 || std::string_view(argv[2])=="--cuda","compare.usage");gate(argc==3);return 0;
    }
    bool cuda=false,have_workers=false,have_first=false;std::string path;unsigned workers=0;bool first=false;
    for(int i=1;i<argc;++i) {
      const std::string_view arg=argv[i];
      if(arg=="--cuda" && !cuda) cuda=true;
      else if(arg=="--frame" && path.empty() && i+1<argc) path=argv[++i];
      else if(arg=="--workers" && !have_workers && i+1<argc) {
        const std::string_view value=argv[++i];need(value=="4" || value=="48","compare.workers");workers=value=="4"?4:48;have_workers=true;
      } else if(arg=="--first" && !have_first && i+1<argc) {
        const std::string_view value=argv[++i];need(value=="baseline" || value=="candidate","compare.first");first=value=="candidate";have_first=true;
      } else throw std::invalid_argument("compare.usage");
    }
    need(cuda && !path.empty() && have_workers && have_first,"compare.frame_requires_cuda_workers_first");frame(path,workers,first);return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 2;}
}
