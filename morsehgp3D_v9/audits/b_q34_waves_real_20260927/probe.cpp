// Fresh harness, explicitly re-executing the frozen wave gate in this binary.
#define main frozen_wave_gate_main
#include "../b_q34_arena_waves_20260927/probe.cpp"
#undef main
#include <charconv>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <sys/resource.h>

using RealClock=std::chrono::steady_clock;
constexpr const char* real_schema="mhgp9_q34_waves_real_v1";
double real_elapsed(RealClock::time_point start) {
  return std::chrono::duration<double,std::milli>(RealClock::now()-start).count();
}
template<class T> T real_number(std::string_view text) {
  T value{};const auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),value);
  if (text.empty() || error!=std::errc{} || end!=text.data()+text.size()) throw std::invalid_argument("real.integer");
  return value;
}
std::string real_hex(u64 value) {std::ostringstream out;out<<std::hex<<std::setw(16)<<std::setfill('0')<<value;return out.str();}
u64 real_hash(std::span<const Point3> points) {
  u64 hash=14695981039346656037ULL;bench::front_hash_word(hash,1);bench::front_hash_word(hash,points.size());
  for (const auto& p:points) for (std::size_t d=0;d!=3;++d) bench::front_hash_word(hash,p[d]);
  return hash;
}
u64 real_rss() {
  std::ifstream input("/proc/self/status");std::string line;
  while (std::getline(input,line)) if (line.starts_with("VmRSS:")) {
    std::istringstream fields(line.substr(6));u64 kib=0;std::string unit;fields>>kib>>unit;
    need(bool(fields) && unit=="kB","real.rss_format");return wa::product(kib,1024);
  }
  throw std::runtime_error("real.rss_unavailable");
}
u64 real_hwm() {
  rusage usage{};if (getrusage(RUSAGE_SELF,&usage)!=0 || usage.ru_maxrss<0) throw std::runtime_error("real.getrusage");
  return wa::product(static_cast<u64>(usage.ru_maxrss),1024);
}
// Explicit byte-reader port from the pinned factor-plan probe, not adoption
// of its plans, qualification, or timings. u32LE triples are checked as u18.
std::vector<Point3> real_load(const std::string& path) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);
  if (!input) throw std::runtime_error("real.frame_open");
  const auto end=input.tellg();if (end<=0 || end%12!=0) throw std::invalid_argument("real.frame_size");
  const auto bytes=static_cast<std::uintmax_t>(end);
  if (bytes/12>std::numeric_limits<std::size_t>::max()) throw std::overflow_error("real.frame_size");
  std::vector<Point3> points(static_cast<std::size_t>(bytes/12));input.seekg(0);
  for (auto& p:points) {
    std::array<Coordinate,3> xyz{};
    for (auto& c:xyz) {
      std::array<unsigned char,4> b{};input.read(reinterpret_cast<char*>(b.data()),b.size());
      if (!input) throw std::runtime_error("real.short_read");
      const auto v=std::uint32_t(b[0])|(std::uint32_t(b[1])<<8U)|(std::uint32_t(b[2])<<16U)|(std::uint32_t(b[3])<<24U);
      if (v>coordinate_limit) throw std::invalid_argument("real.u18_domain");
      c=static_cast<Coordinate>(v);
    }
    p={xyz[0],xyz[1],xyz[2]};
  }
  need(input.peek()==std::char_traits<char>::eof(),"real.file_changed");return points;
}
template<class T> void real_map(std::ostream& out,const std::map<std::string,T>& values) {
  out<<'{';bool first=true;for (const auto& [name,value]:values) {if (!first) out<<',';first=false;out<<'"'<<name<<"\":"<<value;}out<<'}';
}
void real_measure(std::vector<Point3> points,unsigned k,unsigned s,std::size_t capacity,
                  std::string_view mode,std::string_view family,RealClock::time_point start,double input_ms) {
  const auto n=points.size();const auto hash=real_hash(points);
  std::map<std::string,u64> metrics,rss;std::map<std::string,double> times;
  times["input"]=input_ms;rss["after_input"]=real_rss();rss["hwm_after_input"]=real_hwm();
  auto t=RealClock::now();auto cloud=prepare_cloud(points);auto index=make_q2_cloud_index(cloud);times["index"]=real_elapsed(t);
  metrics["input_capacity_bytes"]=wa::product(points.capacity(),sizeof(Point3));
  metrics["shared_cloud_index_bytes"]=wa::sum(cloud->retained_bytes(),index->retained_bytes());
  std::vector<WspdRectangle> rectangles;u64 rectangle_hash=14695981039346656037ULL;
  t=RealClock::now();const auto front=run_wspd_front(*index,k,s,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& r) {
    rectangles.push_back(r);
  },6);times["front"]=real_elapsed(t);
  for (const auto& r:rectangles) {bench::front_hash_word(rectangle_hash,r.a_node);bench::front_hash_word(rectangle_hash,r.b_node);bench::front_hash_word(rectangle_hash,r.lane_mask);}
  metrics["rectangles"]=rectangles.size();metrics["rectangle_capacity_bytes"]=wa::product(rectangles.capacity(),sizeof(WspdRectangle));
  metrics["front_product_visits"]=front.work.product_visits;
  rss["before_candidate"]=real_rss();rss["hwm_before_candidate"]=real_hwm();
  const auto candidate_start=RealClock::now();
  times["shared_elapsed_to_candidate"]=std::chrono::duration<double,std::milli>(candidate_start-start).count();
  t=RealClock::now();auto prepared=wa::Prepared::build(index,rectangles,k,1);times["candidate_prepare"]=real_elapsed(t);
  rss["after_prepare"]=real_rss();
  metrics["prepared_retained_bytes"]=prepared->retained_bytes();
  const auto p=prepared->logical(),e=prepared->effective();
  metrics["Praw"]=prepared->work().raw_pairs;metrics["P"]=p.all;metrics["P3"]=p.q3;metrics["P4"]=p.q4;
  metrics["E"]=e.all;metrics["E3"]=e.q3;metrics["E4"]=e.q4;
  metrics["planned"]=prepared->work().planned;metrics["fallbacks"]=prepared->work().fallbacks;metrics["closed_rectangles"]=prepared->work().closed;
  metrics["segments"]=prepared->segments().size();
  if (const auto* arena=prepared->arena()) {
    metrics["F"]=arena->geometry_work().factor_sites;metrics["classes"]=arena->classes().size();metrics["bands"]=arena->bands().size();
    metrics["pool_corner_tests"]=arena->geometry_work().predicates.corner_tests;
    metrics["arena_retained_bytes"]=arena->retained_bytes();
  } else for (const char* name:{"F","classes","bands","pool_corner_tests","arena_retained_bytes"}) metrics[name]=0;
  t=RealClock::now();auto cursor=std::make_unique<wa::Cursor>(prepared,capacity);
  while (cursor->step()) {}times["candidate_consume"]=real_elapsed(t);rss["after_consume"]=real_rss();
  t=RealClock::now();auto candidate=cursor->finish();times["candidate_sort_convert"]=real_elapsed(t);rss["after_sort_convert"]=real_rss();
  const auto work=cursor->work();const auto pair=cursor->pair_work();
  metrics["queries"]=pair.queries;metrics["q3_queries"]=pair.q3_queries;metrics["q4_queries"]=pair.q4_queries;
  metrics["candidate_pair_visits"]=pair.node_visits;metrics["rectangle_visits"]=prepared->rectangle_work().node_visits;
  metrics["point_rejected3"]=pair.q3_rejected;metrics["point_rejected4"]=pair.q4_rejected;
  metrics["S"]=candidate.survivors.size();u64 s3=0,s4=0;
  for (const auto& edge:candidate.survivors) {counter_add(s3,(edge.mask&2U)!=0?1:0);counter_add(s4,(edge.mask&4U)!=0?1:0);}
  metrics["S3"]=s3;metrics["S4"]=s4;metrics["waves"]=work.waves;metrics["chunks"]=work.chunks;
  metrics["sort_comparisons"]=work.sort_comparisons;metrics["out_of_order"]=work.out_of_order;
  metrics["slot_capacity_peak"]=work.slot_peak;metrics["keyed_capacity_peak"]=work.keyed_capacity_peak;
  metrics["cursor_array_peak_bytes"]=work.array_bytes_peak;metrics["native_output_capacity_bytes"]=work.native_capacity;
  t=RealClock::now();cursor.reset();prepared.reset();times["candidate_internal_cleanup"]=real_elapsed(t);
  times["candidate_total"]=real_elapsed(candidate_start);
  times["candidate_input_to_S2"]=real_elapsed(start);
  times["candidate_observation_overhead"]=times["candidate_total"]-times["candidate_prepare"]-
      times["candidate_consume"]-times["candidate_sort_convert"]-times["candidate_internal_cleanup"];
  rss["after_candidate_cleanup"]=real_rss();rss["hwm_after_candidate"]=real_hwm();
  t=RealClock::now();auto native=run_q34_filter_batch_cpu(*index,k,rectangles,1);times["native_total"]=real_elapsed(t);
  rss["after_native"]=real_rss();rss["hwm_after_native"]=real_hwm();
  metrics["native_pair_visits"]=native.pair_visits;metrics["native_rejected3"]=native.pair_q3_rejected;metrics["native_rejected4"]=native.pair_q4_rejected;
  t=RealClock::now();same(candidate,native);
  need(p.q3>=native.pair_q3_rejected && p.q4>=native.pair_q4_rejected && e.q3>=pair.q3_rejected && e.q4>=pair.q4_rejected,
       "real.rejection_domains");
  need(s3==p.q3-native.pair_q3_rejected && s3==e.q3-pair.q3_rejected &&
       s4==p.q4-native.pair_q4_rejected && s4==e.q4-pair.q4_rejected,"real.lane_mass_identity");
  need(std::max(s3,s4)<=candidate.survivors.size() && candidate.survivors.size()<=wa::sum(s3,s4),"real.union_mass");
  need(pair.queries==e.all && pair.q3_queries==e.q3 && pair.q4_queries==e.q4,"real.actual_queries");
  times["comparison"]=real_elapsed(t);
  t=RealClock::now();candidate=Q34FilterBatch{};native=Q34FilterBatch{};
  std::vector<WspdRectangle>().swap(rectangles);index.reset();cloud.reset();std::vector<Point3>().swap(points);
  times["final_output_input_cleanup"]=real_elapsed(t);rss["after_final_cleanup"]=real_rss();rss["hwm_final"]=real_hwm();
  times["shared_input_index_front"]=times["input"]+times["index"]+times["front"];
  times["experiment_total"]=real_elapsed(start);
  std::cout<<std::fixed<<std::setprecision(6)<<"{\"schema\":\""<<real_schema<<"\",\"status\":\"pass\",\"scope\":\"CPU_S2_no_FULL_no_GPU\""
    <<",\"mode\":\""<<mode<<"\",\"family\":\""<<family<<"\",\"n\":"<<n<<",\"k\":"<<k<<",\"s\":"<<s<<",\"Q\":"<<capacity
    <<",\"seed\":3,\"prepare_workers\":1,\"cursor_workers\":1,\"native_workers\":1,\"order\":\"candidate_then_native\""
    <<",\"input_hash\":\""<<real_hex(hash)<<"\",\"rectangle_hash\":\""<<real_hex(rectangle_hash)<<"\",\"metrics\":";
  real_map(std::cout,metrics);std::cout<<",\"times_ms\":";real_map(std::cout,times);std::cout<<",\"rss_bytes\":";real_map(std::cout,rss);std::cout<<"}\n";
}
int main(int argc,char** argv) {
  if (argc>=2 && (std::string_view(argv[1])=="--gate" || std::string_view(argv[1])=="--mutant")) return frozen_wave_gate_main(argc,argv);
  try {
    const auto start=RealClock::now();
    if (argc==7 && std::string_view(argv[1])=="--synthetic") {
      auto fixture=bench::make_front_fixture(real_number<std::size_t>(argv[3]),argv[2],3);const auto input_ms=real_elapsed(start);
      real_measure(std::move(fixture.points),real_number<unsigned>(argv[4]),real_number<unsigned>(argv[5]),real_number<std::size_t>(argv[6]),
                   "synthetic",argv[2],start,input_ms);return 0;
    }
    if (argc==6 && std::string_view(argv[1])=="--frame") {
      auto points=real_load(argv[2]);const auto input_ms=real_elapsed(start);
      real_measure(std::move(points),real_number<unsigned>(argv[3]),real_number<unsigned>(argv[4]),real_number<std::size_t>(argv[5]),
                   "frame","none",start,input_ms);return 0;
    }
    throw std::invalid_argument("real.usage");
  } catch (const std::exception& e) {
    std::cout<<"{\"schema\":\""<<real_schema<<"\",\"status\":\"failed\",\"cause\":\""<<e.what()<<"\"}\n";return 1;
  }
}
