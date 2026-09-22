#include "pipeline/float32_q3_global.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cfenv>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace {
using Words = mhgp8::Float32Words;
using Options = mhgp8::Float32Q3GlobalOptions;
using Work = mhgp8::Float32Q3GlobalWork;
using Emission = mhgp8::Float32Q3GlobalEmission;
using Clock = std::chrono::steady_clock;
constexpr const char* schema = "mhgp8_float32_q3_global_probe_v1";
void require(bool ok, const char* reason) { if (!ok) throw std::invalid_argument(reason); }
template<class T> T integer(std::string_view s, int base = 10) {
  T value{};
  const auto result = std::from_chars(s.data(), s.data()+s.size(), value, base);
  require(!s.empty() && result.ec == std::errc{} && result.ptr == s.data()+s.size(), "invalid integer");
  return value;
}
std::string token() { std::string s; require(static_cast<bool>(std::cin >> s), "missing token"); return s; }
std::vector<Words> input() {
  const auto n = integer<std::size_t>(token());
  std::vector<Words> data;
  // Do not allocate from an untrusted advertised n before reading the data.
  for (std::size_t i = 0; i != n; ++i) {
    Words point{};
    for (auto& w : point) w = integer<std::uint32_t>(token(), 16);
    data.push_back(point);
  }
  std::string extra;
  require(!(std::cin >> extra), "trailing token");
  return data;
}
Options options(char** argv) {
  Options o;
  o.kmax = integer<std::size_t>(argv[0]);
  o.separation = integer<std::uint32_t>(argv[1]);
  const std::string_view mode(argv[2]), filter(argv[3]);
  require(mode == "individual" || mode == "shared", "invalid mode");
  require(filter == "off" || filter == "on", "invalid filter");
  o.mode = mode == "individual" ? mhgp8::Float32Q3OwnedMode::Individual : mhgp8::Float32Q3OwnedMode::SharedPrefix;
  o.filter = filter == "on";
  o.relay_sites = integer<std::size_t>(argv[4]);
  mhgp8::validate_float32_q3_global_options(o);
  return o;
}
void dump(const Options& o) {
  std::cout << "{\"kmax\":" << o.kmax << ",\"separation\":" << o.separation
      << ",\"mode\":\"" << (o.mode == mhgp8::Float32Q3OwnedMode::Individual ? "individual" : "shared")
      << "\",\"filter\":\"" << (o.filter ? "on" : "off") << "\",\"relay_sites\":" << o.relay_sites << '}';
}
template<class Range> void array(const Range& r) {
  std::cout << '['; bool first = true;
  for (auto x : r) { if (!first) std::cout << ','; first = false; std::cout << x; }
  std::cout << ']';
}

#define EDGE_FIELDS(F) F(preparations) F(owner_queries) F(owner_filter_accepts) F(owner_exact_queries) F(owner_equalities) F(owner_rejections) F(owner_box_queries) F(owner_box_rejections) F(citron_box_queries) F(citron_inside) F(citron_outside) F(citron_unknown) F(citron_point_queries) F(citron_filter_accepts) F(citron_exact_queries) F(separation_queries) F(separation_filter_accepts) F(separation_exact_queries) F(interval_additions) F(interval_products) F(exact_additions) F(exact_products) F(exact_decodes)
#define BLOCK_FIELDS(F) F(edge_preparations) F(edge_squared_positive) F(edge_squared_unresolved) F(block_preparations) F(projected_envelopes) F(gram_positive) F(gram_unresolved) F(xi_tightened) F(xi_intersection_fallbacks) F(center_axes_tightened) F(center_intersection_fallbacks) F(bound_queries) F(inside_certificates) F(outside_certificates) F(unknown_bounds) F(axis_parabolas) F(vertex_clamps) F(power_evaluations) F(interval_additions) F(interval_products) F(interval_divisions) F(scalar_divisions)
#define BALL_FIELDS(F) F(q3_preparations) F(q4_preparations) F(preparation_filter_attempts) F(preparation_filter_accepts) F(preparation_exact_fallbacks) F(preparation_exact_evaluations) F(accepted_supports) F(rejected_supports) F(power_queries) F(power_filter_attempts) F(power_filter_accepts) F(power_exact_fallbacks) F(power_exact_evaluations) F(interval_additions) F(interval_products) F(exact_additions) F(exact_products) F(exact_point_decodes)
#define FRONT_FIELDS(F) F(queries) F(total_unordered_pairs) F(product_visits) F(diagonal_splits) F(diagonal_leaves) F(disjoint_splits) F(witness_searches) F(witness_descent_steps) F(witness_box_tests) F(proposed_sites) F(proposals_in_factors) F(witness_credits) F(rejected_products) F(rejected_pairs) F(emitted_rectangles) F(residual_pairs) F(emitted_factor_sites) F(leaf_pair_rectangles) F(max_stack_size) F(max_product_depth) F(peak_stack_bytes)
#define OWNED_FIELDS(F) F(calls) F(input_seed_slots) F(seed_node_visits) F(seed_rejected_nodes) F(seed_rejected_slots) F(seed_splits) F(shared_frames) F(shared_witness_visits) F(shared_endpoint_skips) F(shared_inside_nodes) F(shared_inside_sites) F(shared_outside_nodes) F(shared_witness_splits) F(shared_saturated_blocks) F(shared_rejected_seed_slots) F(shared_children_with_credit) F(relay_blocks) F(relayed_seed_slots) F(endpoint_seeds) F(owner_candidates) F(owner_rejections) F(owned_seeds) F(invalid_supports) F(valid_supports) F(relays_with_credit) F(relays_at_eof) F(count_node_visits) F(count_point_tests) F(count_inside_nodes) F(count_inside_sites) F(count_outside_nodes) F(count_splits) F(saturated_supports) F(accepted_supports) F(shell_node_visits) F(shell_point_tests) F(shell_excluded_nodes) F(shell_splits) F(shell_ids) F(callbacks) F(stack_reserves) F(shell_growths) F(peak_pending_frames) F(stack_capacity_bytes) F(peak_shell_capacity_bytes) F(peak_workspace_bytes)
#define KEY_FIELDS(F) F(q2_requests) F(q3_requests) F(q4_requests) F(from_support_requests) F(rejected_supports) F(keys_created) F(canonical_gcd_calls) F(canonical_divisions) F(packed_words)
#define INDEX_FIELDS(F) F(input_word_triples_copied) F(finite_points_validated) F(point_objects_constructed) F(presort_comparisons) F(duplicate_adjacent_tests) F(box_endpoint_reads) F(partition_rank_writes) F(partition_id_reads) F(partition_id_writes) F(inverse_rank_writes) F(nodes) F(leaves)
#define GLOBAL_FIELDS(F) F(calls) F(rectangles) F(edges) F(edge_filter_calls) F(witness_node_visits) F(witness_inside_nodes) F(witness_inside_sites) F(witness_outside_nodes) F(witness_splits) F(rejected_edges) F(emitted) F(shell_ids) F(peak_key_bytes)
#define FIELD(n) if (!first) std::cout << ','; first = false; std::cout << '"' << #n << "\":" << w.n;
void dump(const mhgp8::Float32EdgeWork& w) { bool first=true; std::cout<<'{'; EDGE_FIELDS(FIELD) std::cout<<'}'; }
void dump(const mhgp8::Float32Q3OwnedBlockWork& w) { bool first=true; std::cout<<'{'; BLOCK_FIELDS(FIELD) std::cout<<'}'; }
void dump(const mhgp8::Float32BallWork& w) { bool first=true; std::cout<<'{'; BALL_FIELDS(FIELD) std::cout<<'}'; }
void dump(const mhgp8::Float32IndexWork& w) { bool first=true; std::cout<<'{'; INDEX_FIELDS(FIELD) std::cout<<'}'; }
void dump(const mhgp8::Float32FrontWork& w) {
  bool first=false; std::cout<<"{\"geometry\":"; dump(w.geometry); FRONT_FIELDS(FIELD) std::cout<<'}';
}
void dump(const mhgp8::Float32Q3OwnedWork& w) {
  bool first=false; std::cout<<"{\"selection\":"; dump(w.selection);
  std::cout<<",\"shared_bounds\":"; dump(w.shared_bounds);
  std::cout<<",\"individual_bounds\":"; dump(w.individual_bounds);
  std::cout<<",\"supports\":"; dump(w.supports); std::cout<<",\"power\":"; dump(w.power);
  OWNED_FIELDS(FIELD) std::cout<<'}';
}
void dump(const mhgp8::Float32KeyWork& w) {
  bool first=false; std::cout<<"{\"support\":"; dump(w.support); KEY_FIELDS(FIELD) std::cout<<'}';
}
void dump(const Work& w) {
  bool first=false; std::cout<<"{\"front\":"; dump(w.front);
  std::cout<<",\"edge_filter\":"; dump(w.edge_filter); std::cout<<",\"owned\":"; dump(w.owned);
  std::cout<<",\"keys\":"; dump(w.keys); GLOBAL_FIELDS(FIELD) std::cout<<'}';
}
#undef FIELD

struct Record {
  std::array<std::size_t,3> support;
  std::array<std::size_t,2> owner;
  std::size_t depth;
  std::vector<std::size_t> shell;
  std::vector<std::uint32_t> key;
  bool operator==(const Record&) const = default;
};
Record record(const Emission& e) {
  Record r{e.support,e.owner,e.depth,{e.shell.begin(),e.shell.end()},
      {e.key.serialized_words().begin(),e.key.serialized_words().end()}};
  std::sort(r.shell.begin(),r.shell.end()); return r;
}
void dump(const Record& r) {
  std::cout<<"{\"support\":"; array(r.support); std::cout<<",\"owner\":"; array(r.owner);
  std::cout<<",\"depth\":"<<r.depth<<",\"shell\":"; array(r.shell);
  std::cout<<",\"key\":"; array(r.key); std::cout<<'}';
}
std::uint64_t mix(std::uint64_t x) {
  x+=0x9e3779b97f4a7c15ULL; x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
  x=(x^(x>>27))*0x94d049bb133111ebULL; return x^(x>>31);
}
struct Payload {
  std::uint64_t emitted{},shell_ids{},depth_sum{},key_words{},digest_sum{},digest_xor{};
  void add(const Emission& e) {
    std::uint64_t h=0, s=0, x=0;
    for (auto id:e.support) h=mix(h^mix(id));
    for (auto w:e.key.serialized_words()) h=mix(h^w);
    for (auto id:e.shell) { s+=mix(id); x^=mix(id); }
    h=mix(h^mix(e.depth)^s^std::rotl(x,13));
    mhgp8::float32_predicate_detail::count(emitted);
    mhgp8::float32_predicate_detail::count(shell_ids,e.shell.size());
    mhgp8::float32_predicate_detail::count(depth_sum,e.depth);
    mhgp8::float32_predicate_detail::count(key_words,e.key.serialized_words().size());
    digest_sum+=h; digest_xor^=h;
  }
  bool operator==(const Payload&) const = default;
};
void dump(const Payload& p) {
  std::cout<<"{\"emitted\":"<<p.emitted<<",\"shell_ids\":"<<p.shell_ids<<",\"depth_sum\":"<<p.depth_sum
      <<",\"key_words\":"<<p.key_words<<",\"digest_sum\":"<<p.digest_sum<<",\"digest_xor\":"<<p.digest_xor<<'}';
}

class Environment {
 public:
  Environment(std::string_view round, unsigned ftz):saved_(std::fegetround()) {
    require(ftz<=1,"invalid FTZ/DAZ");
    const int mode=round=="nearest"?FE_TONEAREST:round=="down"?FE_DOWNWARD:
        round=="up"?FE_UPWARD:round=="zero"?FE_TOWARDZERO:-1;
    require(mode!=-1,"invalid rounding mode");
#if defined(__SSE__)
    saved_csr_=_mm_getcsr();
    auto csr=saved_csr_&~(0x8000U|0x0040U);
    if(ftz) csr|=0x8000U|0x0040U;
    _mm_setcsr(csr);
#else
    require(ftz==0,"FTZ/DAZ unavailable");
#endif
    require(std::fesetround(mode)==0,"rounding mode unavailable");
  }
  ~Environment() {
    std::fesetround(saved_);
#if defined(__SSE__)
    _mm_setcsr(saved_csr_);
#endif
  }
  Environment(const Environment&)=delete;
  Environment& operator=(const Environment&)=delete;
 private:
  int saved_;
#if defined(__SSE__)
  unsigned saved_csr_{};
#endif
};

void case_run(const std::vector<Words>& points,const Options& o) {
  Work w; std::vector<Record> rows;
  if(!points.empty()) {
    auto index=mhgp8::prepare_float32_index(points);
    mhgp8::run_float32_q3_global(index,o,[&](const Emission& e){rows.push_back(record(e));},w);
  }
  std::cout<<"{\"schema\":\""<<schema<<"\",\"kind\":\"case\",\"options\":";dump(o);
  std::cout<<",\"n\":"<<points.size()<<",\"records\":[";
  bool first=true;for(const auto& r:rows){if(!first)std::cout<<',';first=false;dump(r);}
  std::cout<<"],\"work\":";dump(w);std::cout<<"}\n";
}
Words point(float x,float y,float z){return {std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),std::bit_cast<std::uint32_t>(z)};}
std::vector<Words> synthetic(std::string_view regime,std::size_t n){
  require(regime=="tetra_chain"||regime=="grid","unknown regime");
  std::vector<Words> points;points.reserve(n);
  std::size_t side=1;
  if(regime=="grid")while(side<n/side/side || side*side*side<n)++side;
  for(std::size_t i=0;i!=n;++i){
    if(regime=="tetra_chain") {
      const auto group=i/4, lane=i%4;
      const float x=static_cast<float>(group)*16.0F;
      points.push_back(lane==0?point(x,0,0):lane==1?point(x+2,0,0):lane==2?point(x+1,2,0):point(x+1,1,2));
    } else points.push_back(point(static_cast<float>(i%side)*4.0F,static_cast<float>((i/side)%side)*4.0F,
                                 static_cast<float>(i/side/side)*4.0F));
  }return points;
}
std::vector<Words> file_input(const char* path){
  std::ifstream stream(path,std::ios::binary|std::ios::ate);require(static_cast<bool>(stream),"cannot open f32 input");
  const auto bytes=stream.tellg();require(bytes>=0 && bytes%12==0,"invalid f32 input length");
  stream.seekg(0);std::vector<Words> points;
  for(std::streamoff i=0;i<bytes;i+=12){
    unsigned char data[12]{};stream.read(reinterpret_cast<char*>(data),12);require(static_cast<bool>(stream),"incomplete f32 input");
    Words p{};for(std::size_t j=0;j<3;++j)p[j]=std::uint32_t(data[4*j])|(std::uint32_t(data[4*j+1])<<8)|
       (std::uint32_t(data[4*j+2])<<16)|(std::uint32_t(data[4*j+3])<<24);
    points.push_back(p);
  }require(stream.peek()==std::char_traits<char>::eof(),"f32 input grew");return points;
}
void measure(const std::vector<Words>& points,const Options& o,std::string_view regime,double input_ms){
  Work w;Payload payload;mhgp8::Float32IndexWork iw{};
  const auto start=Clock::now();
  const auto index=points.empty()?mhgp8::Float32IndexPtr{}:mhgp8::prepare_float32_index(points);
  const auto prepared=Clock::now();
  if(index){iw=index->work();mhgp8::run_float32_q3_global(index,o,[&](const Emission& e){payload.add(e);},w);}
  const auto finished=Clock::now();
  const auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};
  std::cout<<std::setprecision(17)<<"{\"schema\":\""<<schema<<"\",\"kind\":\"measure\",\"regime\":\""<<regime
      <<"\",\"n\":"<<points.size()<<",\"options\":";dump(o);std::cout<<",\"work\":";dump(w);
  std::cout<<",\"index_work\":";dump(iw);std::cout<<",\"payload\":";dump(payload);
  std::cout<<",\"index_bytes\":"<<(index?index->retained_bytes():0)<<",\"index_peak_bytes\":"
      <<(index?index->construction_peak_vector_bytes():0)<<",\"timing_ms\":{\"input\":"<<input_ms
      <<",\"index\":"<<ms(start,prepared)<<",\"pipeline\":"<<ms(prepared,finished)
      <<",\"index_pipeline\":"<<ms(start,finished)<<"}}\n";
}

void selftest(){
  std::size_t checks=0;
  const auto check=[&](bool ok){require(ok,"native global selftest failed");++checks;};
  const auto points=synthetic("tetra_chain",12);const auto index=mhgp8::prepare_float32_index(points);
  Options o;Work w;Payload baseline;
  mhgp8::run_float32_q3_global(index,o,[&](const Emission& e){baseline.add(e);},w);
  check(baseline.emitted>0);check(w.emitted==baseline.emitted);check(w.keys.keys_created==w.emitted);
  check(w.owned.stack_reserves<=1);check(w.edges==w.front.residual_pairs);
  for(const auto mode:{mhgp8::Float32Q3OwnedMode::Individual,mhgp8::Float32Q3OwnedMode::SharedPrefix})
    for(const bool filter:{false,true})for(const std::uint32_t s:{8U,10U,12U}) {
      auto other=o;other.mode=mode;other.filter=filter;other.separation=s;Work work;Payload p;
      mhgp8::run_float32_q3_global(index,other,[&](const Emission& e){p.add(e);},work);
      check(p==baseline);check(work.front.residual_pairs+work.front.rejected_pairs==points.size()*(points.size()-1)/2);
    }
  std::array<Payload,4> results{};std::array<std::exception_ptr,4> errors{};std::vector<std::thread> readers;
  try{for(std::size_t i=0;i<4;++i)readers.emplace_back([&,i]{try{Work work;mhgp8::run_float32_q3_global(index,o,
        [&](const Emission&e){results[i].add(e);},work);}catch(...){errors[i]=std::current_exception();}});}
  catch(...){for(auto&t:readers)t.join();throw;}
  for(auto&t:readers)t.join();for(std::size_t i=0;i<4;++i)check(!errors[i]&&results[i]==baseline);
  Work untouched;auto invalid=o;invalid.kmax=0;bool refused=false;
  try{mhgp8::run_float32_q3_global(index,invalid,[](const Emission&){},untouched);}catch(const std::invalid_argument&){refused=true;}
  check(refused&&untouched==Work{});
  Work partial;bool propagated=false;
  try{mhgp8::run_float32_q3_global(index,o,[](const Emission&){throw std::runtime_error("consumer");},partial);}
  catch(const std::runtime_error& e){propagated=std::string_view(e.what())=="consumer";}
  check(propagated&&partial.emitted==1);
  auto handle=index;Work keepalive;Payload kept;
  mhgp8::run_float32_q3_global(handle,o,[&](const Emission&e){handle.reset();kept.add(e);},keepalive);
  check(!handle&&kept==baseline);
  auto inactive=o;inactive.kmax=1;Work inactive_work;
  mhgp8::run_float32_q3_global(index,inactive,[](const Emission&){throw std::runtime_error("inactive");},inactive_work);
  check(inactive_work.calls==1&&inactive_work.emitted==0&&inactive_work.edges==0);
  mhgp8::Float32Q3OwnedWorkspace workspace(index);
  const mhgp8::Float32Q3OwnedOptions owned_options{10,mhgp8::Float32Q3OwnedMode::SharedPrefix,1};
  std::uint64_t first_count=0,second_count=0;
  mhgp8::Float32Q3OwnedWork first_work,second_work;
  bool reentrant_refused=false;
  mhgp8::run_float32_q3_owned_edge(workspace,0,2,owned_options,
      [&](const mhgp8::Float32Q3OwnedEmission&){
        ++first_count;
        if(!reentrant_refused){
          mhgp8::Float32Q3OwnedWork denied;
          try{mhgp8::run_float32_q3_owned_edge(workspace,0,2,owned_options,
              [](const mhgp8::Float32Q3OwnedEmission&){},denied);}
          catch(const std::invalid_argument&){reentrant_refused=denied==mhgp8::Float32Q3OwnedWork{};}
        }
      },first_work);
  check(first_count>0);check(reentrant_refused);check(first_work.stack_reserves==1);
  const auto retained=workspace.retained_bytes();
  mhgp8::run_float32_q3_owned_edge(workspace,0,2,owned_options,
      [&](const mhgp8::Float32Q3OwnedEmission&){++second_count;},second_work);
  check(second_count==first_count);check(second_work.stack_reserves==0&&second_work.shell_growths==0);
  check(workspace.retained_bytes()==retained);
  mhgp8::Float32Q3OwnedWork failed_owned;bool owned_exception=false;
  try{mhgp8::run_float32_q3_owned_edge(workspace,0,2,owned_options,
      [](const mhgp8::Float32Q3OwnedEmission&){throw std::runtime_error("owned consumer");},failed_owned);}
  catch(const std::runtime_error&e){owned_exception=std::string_view(e.what())=="owned consumer";}
  check(owned_exception&&failed_owned.callbacks==1);
  second_count=0;
  mhgp8::run_float32_q3_owned_edge(workspace,0,2,owned_options,
      [&](const mhgp8::Float32Q3OwnedEmission&){++second_count;},second_work);
  check(second_count==first_count);
  std::cout<<"{\"schema\":\""<<schema<<"\",\"kind\":\"selftest\",\"checks\":"<<checks
      <<",\"concurrent_calls\":4,\"status\":\"passed\"}\n";
}
} // namespace

int main(int argc,char**argv){
  try{
    if(argc==2&&std::string_view(argv[1])=="--selftest"){selftest();return 0;}
    if(argc==7&&std::string_view(argv[1])=="--case"){const auto o=options(argv+2);case_run(input(),o);return 0;}
    if(argc==9&&std::string_view(argv[1])=="--case-env"){
      const auto o=options(argv+4);const auto data=input();Environment env(argv[2],integer<unsigned>(argv[3]));case_run(data,o);return 0;
    }
    if(argc==9&&std::string_view(argv[1])=="--scale"){
      const auto o=options(argv+4);const auto start=Clock::now();const auto data=synthetic(argv[2],integer<std::size_t>(argv[3]));
      const auto end=Clock::now();measure(data,o,argv[2],std::chrono::duration<double,std::milli>(end-start).count());return 0;
    }
    if(argc==8&&std::string_view(argv[1])=="--file"){
      const auto o=options(argv+3);const auto start=Clock::now();const auto data=file_input(argv[2]);
      const auto end=Clock::now();measure(data,o,"prepared_f32",std::chrono::duration<double,std::milli>(end-start).count());return 0;
    }
    throw std::invalid_argument("usage: --case K S individual|shared off|on RELAY; --case-env ROUND FTZ ...; --scale REGIME N ...; --file F32 ...; --selftest");
  }catch(const std::exception&e){std::cerr<<"mhgp8 q3 global: "<<e.what()<<'\n';return 2;}
}
