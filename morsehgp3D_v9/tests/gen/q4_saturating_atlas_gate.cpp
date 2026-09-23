#include "lanes/q4_local.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

namespace {
using namespace mhgp9::gen;
using Points=std::vector<Point3>;
constexpr u64 unlimited=std::numeric_limits<u64>::max();
u64 checks{}, partitions{}, certificates{}, interrupted{}, exact_results{}, point_stops{}, block_stops{};
u64 edge_calls{}, outputs{}, max_shell{}, atlas_certificates{}, q3_locations{}, callbacks_thrown{};
void require(bool ok,const char* message) {++checks;if(!ok)throw std::runtime_error(message);}

// Independent closed-cell oracle via Euclidean distances in physical 3D,
// expressed over the common denominator 2Q. No product form/bounds or
// rounded q3 center is used. Coordinates18bit, Q20: squares fit i128.
i128 power(Point3 a,Point3 b,Point3 z,i64 alpha,i64 beta) {
  std::array<i128,3> v{},first{},second{},center{},dz{},da{};
  unsigned main=0;
  for(unsigned i=0;i!=3;++i){v[i]=static_cast<i128>(b[i])-a[i];if(v[i]*v[i]>v[main]*v[main])main=i;}
  const auto magnitude=v[main]<0?-v[main]:v[main];
  const auto sign=v[main]<0?-1:1;
  first[(main+1)%3]=magnitude;first[main]=-sign*v[(main+1)%3];
  second[(main+2)%3]=magnitude;second[main]=-sign*v[(main+2)%3];
  const i128 q=Q4LocalCell::scale;
  i128 result=0;
  for(unsigned i=0;i!=3;++i) {
    center[i]=q*(static_cast<i128>(a[i])+b[i])+alpha*first[i]+beta*second[i];
    dz[i]=2*q*z[i]-center[i];da[i]=2*q*a[i]-center[i];
    result+=dz[i]*dz[i]-da[i]*da[i];
  }
  return result;
}

void inspect(const Q4LocalPartitionResult& result,const Q4LocalFragmentPtr& baseline,
             std::size_t threshold) {
  ++partitions;
  auto copied=result;
  const auto moved=std::move(copied);
  require(result.geometry()==baseline->geometry() && copied.geometry()==result.geometry() &&
      moved.geometry()==result.geometry() && copied.saturated()==result.saturated() &&
      moved.saturated()==result.saturated() && copied.exact_fragment()==result.exact_fragment() &&
      moved.exact_fragment()==result.exact_fragment(),"copy or move changed result tag or owner");
  require(result.cell()==baseline->cell(),"cell changed");
  if(!result.saturated()) {
    ++exact_results;
    const auto& actual=result.exact_fragment();
    require(actual && actual->inside_count()==baseline->inside_count() &&
      actual->active_sites()==baseline->active_sites() && actual->work()==baseline->work() &&
      std::equal(actual->active_nodes().begin(),actual->active_nodes().end(),
                 baseline->active_nodes().begin(),baseline->active_nodes().end()),
      "unsaturated partition differs from complete reference");
    return;
  }
  ++certificates;
  interrupted+=result.unvisited_sites()!=0?1U:0U;
  require(!result.exact_fragment() && result.certified_depth()>=threshold,
          "partial frontier escaped or threshold changed");
  const auto& w=result.work();
  if(w.inherited_inside_sites==0 && w.inside_sites==w.inside_nodes && w.inside_nodes!=0)++point_stops;
  if(w.inherited_inside_sites==0 && w.inside_nodes==1 && w.inside_sites>1)++block_stops;
  require(result.certified_depth()<=baseline->inside_count(),"prefix overcounts complete partition");
  require(w.node_visits<=baseline->work().node_visits,"prefix repeats complete work");
  const auto geometry=baseline->geometry();
  const auto points=geometry->cover()->index()->cloud().points();
  const auto ids=geometry->cover()->edge_ids();
  const auto cell=result.cell();
  std::size_t common=0;
  for(const auto z:points) {
    bool all=true;
    for(const auto x:{cell.left,cell.right})for(const auto y:{cell.bottom,cell.top})
      all=all && power(points[ids[0]],points[ids[1]],z,x,y)<0;
    common+=all?1U:0U;
  }
  require(common>=result.certified_depth(),"terminal certificate fails independent closed-cell oracle");
}

void partition_cases(const Points& points) {
  const auto cover=Q34EdgeCover::make(make_q2_cloud_index(prepare_cloud(points)),{0,1});
  const auto geometry=Q4LocalGeometry::make(cover,Q4CenterDomainMode::Disk);
  for(const auto budget:{u64{0},u64{1},unlimited}) {
    const auto root=Q4LocalFragment::root(geometry,budget);
    for(const auto threshold:{std::size_t{1},std::size_t{2},std::size_t{4},std::size_t{9}})
      inspect(Q4LocalFragment::root_until(geometry,budget,threshold),root,threshold);
    for(unsigned quadrant=0;quadrant!=4;++quadrant) {
      const auto child=Q4LocalFragment::child(root,quadrant,budget);
      for(const auto threshold:{std::size_t{1},std::size_t{2},std::size_t{9}})
        inspect(Q4LocalFragment::child_until(root,quadrant,budget,threshold),child,threshold);
      const auto refined=Q4LocalFragment::refine(child,unlimited);
      for(const auto threshold:{std::size_t{1},std::size_t{2},std::size_t{9}})
        inspect(Q4LocalFragment::refine_until(child,unlimited,threshold),refined,threshold);
    }
  }
}
struct Candidate {
  std::array<std::size_t,4> ids;
  std::array<i128,5> key;
  std::size_t depth;
  std::vector<std::size_t> shell;
  bool operator==(const Candidate&)const=default;
  bool operator<(const Candidate& b)const{return std::tie(ids,key,depth,shell)<std::tie(b.ids,b.key,b.depth,b.shell);}
};
using Output=std::vector<Candidate>;
Output collect(Q4LocalAtlasPtr atlas) {
  Output output;
  static_cast<void>(run_q4_local_edge_candidates(atlas,[&](const Q34SeedCandidate& candidate){
    if(candidate.arity!=4)throw std::runtime_error("q4-only atlas emitted another arity");
    Candidate value{candidate.support_ids,candidate.ball.coefficients(),candidate.depth,
      std::vector<std::size_t>(candidate.shell_first.begin(),candidate.shell_first.end())};
    value.shell.insert(value.shell.end(),candidate.shell_second.begin(),candidate.shell_second.end());
    std::sort(value.shell.begin(),value.shell.end());output.push_back(std::move(value));
  }));
  std::sort(output.begin(),output.end());return output;
}
void edge_case(const Points& points,std::size_t k,u64 budget) {
  const auto cover=Q34EdgeCover::make(make_q2_cloud_index(prepare_cloud(points)),{0,1});
  Q4LocalOptions opts{Q4CenterDomainMode::Disk,3,85,budget,0,true,false};
  const auto baseline=Q4LocalAtlas::make(cover,k,opts);
  opts.saturate_deep=true;
  const auto limited=Q4LocalAtlas::make(cover,k,opts);
  const auto expected=collect(baseline),actual=collect(limited);
  require(expected==actual,"saturating atlas changed complete supports, keys, depths or shells");
  ++edge_calls;outputs+=actual.size();
  for(const auto& candidate:actual) {
    max_shell=std::max(max_shell,static_cast<u64>(candidate.shell.size()));
    // Re-evaluate the declared exact polynomial directly for every site,
    // independent of fragment counts/event sweep and strict-shell assembly.
    std::size_t depth=0;std::vector<std::size_t> shell;
    for(std::size_t id=0;id<points.size();++id) {
      i128 norm=0,p=candidate.key[4];
      for(unsigned axis=0;axis!=3;++axis){const i128 z=points[id][axis];norm+=z*z;p+=candidate.key[axis+1]*z;}
      p+=candidate.key[0]*norm;
      if(p<0)++depth;else if(p==0)shell.push_back(id);
    }
    require(depth==candidate.depth && shell==candidate.shell,"candidate census differs from global polynomial scan");
  }
  const auto& s=limited->saturation_work();
  require(s.queries==s.exact_fragments+s.certificates,"saturation call partition");
  if(s.queries==1 && s.certificates==1)
    require(limited->work().partition==s.prefixes,
            "root certificate physical work omitted from atlas total");
  atlas_certificates+=s.certificates;
  const auto tested=[](const Q4LocalPartitionWork& w){return w.block_bound_tests+w.point_tests;};
  require(tested(s.prefixes)<=tested(limited->work().partition),"prefix is not a subset of total work");
  require(tested(limited->work().partition)<=tested(baseline->work().partition),
          "saturation repeats geometric work");
  const auto& empty=baseline->saturation_work();
  require(empty.queries==0 && empty.prefixes.node_visits==0,"disabled option pays hidden saturation work");
  for(std::size_t x=2;x<points.size();++x) {
    // Only strictly acute q3 seeds are eligible for the independent q3 check.
    const auto ball=ExactBall::make_q3({points[0],points[1],points[x]});
    if(!ball)continue;
    const auto center=baseline->geometry()->q3_center(x);
    const auto a=baseline->certified_inside_count(center),b=limited->certified_inside_count(center);
    require(a.has_value()==b.has_value() && (!a || ((*a>=k-1)==(*b>=k-1))),
            "saturation changed q3 rejection threshold");
    if(b) {
      std::size_t depth=0;
      for(const auto z:points)depth+=ball->power(z)<0?1U:0U;
      require(depth>=*b,"q3 certificate overcounts true strict depth");
    }
    ++q3_locations;
  }
}
Points shell30() {
  // Explicit fixture port, not a port of an audit oracle.
  Points points{{23,24,20},{20,17,24},{20,17,16},{17,24,20}};
  for(int x=-5;x<=5;++x)for(int y=-5;y<=5;++y)for(int z=-5;z<=5;++z)
    if(x*x+y*y+z*z==25) {
      const Point3 p{20+x,20+y,20+z};
      if(std::find(points.begin(),points.end(),p)==points.end())points.push_back(p);
    }
  return points;
}
void selftest() {
  const Points dense{{10,20,20},{30,20,20},{19,20,20},{20,20,20},{21,20,20},{22,20,20},
                     {20,34,20},{20,20,34},{20,20,6},{20,6,20}};
  const Points contact{{30,30,30},{36,36,30},{30,36,24},{36,30,24},{30,36,30},{36,30,30},{30,30,24},{32,32,32}};
  Points extreme=dense;
  for(auto& p:extreme){p.x+=262100;p.y+=262100;p.z+=262100;}
  for(const auto* p:std::array<const Points*,3>{&dense,&contact,&extreme})partition_cases(*p);
  const auto shell=shell30();
  for(const auto* p:std::array<const Points*,4>{&dense,&contact,&extreme,&shell})
    for(const auto k:{3U,5U,10U})for(const auto budget:{u64{0},u64{1},unlimited})
      edge_case(*p,k,budget);
  const auto cover=Q34EdgeCover::make(make_q2_cloud_index(prepare_cloud(shell)),{0,1});
  Q4LocalOptions opts;opts.saturate_deep=true;
  auto atlas=Q4LocalAtlas::make(cover,5,opts);
  const auto expected=collect(atlas);
  std::array<std::future<Output>,4> jobs;
  for(auto& job:jobs)job=std::async(std::launch::async,[atlas]{return collect(atlas);});
  for(auto& job:jobs)require(job.get()==expected,"shared immutable atlas differs across workers");
  bool threw=false;
  try{static_cast<void>(run_q4_local_edge_candidates(atlas,[](const auto&){throw std::runtime_error("callback");}));}
  catch(const std::runtime_error&){threw=true;}
  require(threw,"callback did not propagate");++callbacks_thrown;
  require(collect(atlas)==expected,"callback failure damaged atlas");
  for(unsigned which=0;which!=3;++which) {
    threw=false;
    try{
      if(which==0)static_cast<void>(Q4LocalFragment::root_until({},0,1));
      else if(which==1)static_cast<void>(Q4LocalFragment::root_until(atlas->geometry(),0,0));
      else static_cast<void>(Q4LocalFragment::child_until(Q4LocalFragment::root(atlas->geometry(),0),4,0,1));
    }catch(const std::invalid_argument&){threw=true;}
    require(threw,"invalid saturating factory accepted");
  }
  require(certificates>0 && interrupted>0 && point_stops>0 && block_stops>0 &&
          atlas_certificates>0 && outputs>0 && max_shell>=30 && q3_locations>0,"nonvacuity missing");
  std::cout<<"{\"schema\":\"mhgp9_gen_q4_saturating_atlas_gate_v1\",\"status\":\"passed\",\"checks\":"<<checks
    <<",\"partitions\":"<<partitions<<",\"certificates\":"<<certificates<<",\"interrupted\":"<<interrupted
    <<",\"exact_results\":"<<exact_results<<",\"point_stops\":"<<point_stops<<",\"block_stops\":"<<block_stops
    <<",\"edge_calls\":"<<edge_calls<<",\"outputs\":"<<outputs<<",\"max_shell\":"<<max_shell
    <<",\"atlas_certificates\":"<<atlas_certificates<<",\"q3_locations\":"<<q3_locations
    <<",\"parallel_calls\":4,\"callback_failures\":"<<callbacks_thrown<<"}\n";
}
void scale_case(std::size_t n) {
  if(n<32 || n>172144)throw std::invalid_argument("diagnostic fixture requires32..172144 unique sites in coordinate domain");
  Points points{{0,131000,131000},{262000,131000,131000}};
  for(int i=0;i!=16;++i)points.push_back({131000+i,131000,131000});
  // Interior cylinder is deep on the WHOLE root cell; this explicitly
  // bounds the diagnostic to construction of one terminal-root atlas.
  for(std::size_t i=18;i<n;++i)points.push_back({90000+static_cast<int>(i),
      130900+static_cast<int>(i%17),130900+static_cast<int>((i/17)%19)});
  const auto start=std::chrono::steady_clock::now();
  const auto cover=Q34EdgeCover::make(make_q2_cloud_index(prepare_cloud(points)),{0,1});
  const auto prepared=std::chrono::steady_clock::now();
  Q4LocalOptions opts;opts.domain=Q4CenterDomainMode::Disk;
  const auto a=Q4LocalAtlas::make(cover,10,opts);
  const auto middle=std::chrono::steady_clock::now();
  opts.saturate_deep=true;
  const auto b=Q4LocalAtlas::make(cover,10,opts);
  const auto end=std::chrono::steady_clock::now();
  require(a->work().cells_created==1 && b->work().cells_created==1 &&
      a->root_certified_inside_count()>=9 && b->root_certified_inside_count()>=9,
      "scale fixture is not certified deep at its root");
  require(collect(a).empty() && collect(b).empty(),"deep fixture emitted a q4 support");
  const auto& x=a->work().partition;const auto& y=b->work().partition;const auto& z=b->saturation_work();
  const auto ms=[](auto first,auto last){return std::chrono::duration<double,std::milli>(last-first).count();};
  std::cout<<"{\"schema\":\"mhgp9_gen_q4_saturating_atlas_scale_v1\",\"scope\":\"one_supplied_edge_empty_q4_stream_not_pipeline\","
    <<"\"n\":"<<n<<",\"kmax\":10,\"outputs\":0,\"prepare_ms\":"<<ms(start,prepared)
    <<",\"baseline_ms\":"<<ms(prepared,middle)<<",\"saturated_ms\":"<<ms(middle,end)
    <<",\"baseline_tests\":"<<x.block_bound_tests+x.point_tests
    <<",\"saturated_total_tests\":"<<y.block_bound_tests+y.point_tests
    <<",\"baseline_node_visits\":"<<x.node_visits<<",\"saturated_total_node_visits\":"<<y.node_visits
    <<",\"baseline_frontier_copies\":"<<x.frontier_ids_copied
    <<",\"saturated_total_frontier_copies\":"<<y.frontier_ids_copied
    <<",\"unvisited_site_mass\":"<<z.unvisited_site_mass<<",\"certificates\":"<<z.certificates
    <<",\"baseline_peak_build_bytes\":"<<a->work().peak_build_bytes
    <<",\"saturated_peak_build_bytes\":"<<b->work().peak_build_bytes<<"}\n";
}
}
int main(int argc,char** argv) {
  try {
    if(argc==2 && std::string_view(argv[1])=="--selftest")selftest();
    else if(argc==3 && std::string_view(argv[1])=="--scale") {
      const std::string_view value=argv[2];std::size_t n=0;
      const auto parsed=std::from_chars(value.data(),value.data()+value.size(),n);
      if(parsed.ec!=std::errc{} || parsed.ptr!=value.data()+value.size())throw std::invalid_argument("invalid n");
      scale_case(n);
    } else throw std::invalid_argument("expected --selftest or --scale N");
    return 0;
  }catch(const std::exception& e){std::cerr<<"q4 saturating atlas gate: "<<e.what()<<'\n';return 1;}
}
