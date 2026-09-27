#define main old_factor_unused_main
#include "../b_q34_factor_plan_20260926/probe.cpp"
#undef main
#include "../b_q34_direct_bands_20260927/direct.hpp"
#include "arena.hpp"
#include <type_traits>

namespace ca=mhgp9::audit::collective;
namespace db=mhgp9::audit::direct_bands;
constexpr const char* arena_schema="mhgp9_q34_collective_v1";
static_assert(!std::is_copy_constructible_v<ca::Arena> && !std::is_move_constructible_v<ca::Arena>);
struct Checks {u64 batches{},rectangles{},factors{},ranks{},classes{},bands{},pairs{},refusals{},pool_ids{},worker_throws{};};
auto geometry(const fp::Work& w) {
  return std::array{w.factor_sites,w.selection_visits,w.selection_tests,w.selection_shifts,w.selected_sites,
    w.anchor_visits,w.witness_attempts,w.self_skips,w.q3_credits,w.q4_credits,w.predicates.point_tests,
    w.predicates.universal_queries,w.predicates.q2_axis_terms,w.predicates.corner_tests,
    w.predicates.block_bound_tests,w.predicates.negative_probes};
}
template<class T> bool equal_span(std::span<const T> a,std::span<const T> b) {
  return a.size()==b.size() && std::equal(a.begin(),a.end(),b.begin());
}
void compare_arenas(const ca::Arena& a,const ca::Arena& b,bool same_grain=true) {
  require(equal_span(a.rectangles(),b.rectangles()) && equal_span(a.factors(),b.factors()) &&
    equal_span(a.classes(),b.classes()) && equal_span(a.bands(),b.bands()) &&
    equal_span(a.ranks(),b.ranks()) && equal_span(a.credits(),b.credits()),"arena_parallel_output");
  require(geometry(a.geometry_work())==geometry(b.geometry_work()) && a.mass()==b.mass(),"arena_parallel_geometry");
  auto aw=a.work(),bw=b.work();if (!same_grain) {aw.anchor_jobs=0;bw.anchor_jobs=0;}
  require(aw==bw,"arena_parallel_work");
}
void judge(const Q2CensusIndex& index,std::span<const ca::Request> requests,unsigned k,
           const ca::Arena& arena,Checks& checks,bool expand) {
  ++checks.batches;fp::Work expected_work;ca::Mass expected_mass;
  require(arena.rectangles().size()==requests.size() && arena.factors().size()==2*requests.size(),"arena_counts");
  u64 F=0,C=0,D=0;
  for (std::size_t r=0;r!=requests.size();++r) {
    const auto& req=requests[r];const db::Plan ref(index,req.a_node,req.b_node,k,req.mask);
    ca::detail::add_work(expected_work,ref.work());++checks.rectangles;
    expected_mass.all+=ref.output().union_mass;expected_mass.q3+=ref.output().q3_mass;expected_mass.q4+=ref.output().q4_mass;
    const auto& rm=arena.rectangles()[r];
    require(rm.source_ordinal==req.source_ordinal && rm.mask==req.mask && rm.band_first==D &&
        rm.bands==ref.output().bands.size(),"arena_rectangle_header");
    for (unsigned side=0;side!=2;++side) {
      ++checks.factors;const auto& f=arena.factors()[2*r+side];const auto& rf=side==0?ref.a():ref.b();
      require(f.original_first==rf.original_ranks.first && f.first==F && f.class_first==C &&
              f.size==rf.grouped.size() && f.classes==rf.groups.size(),"arena_factor_header");
      for (std::size_t i=0;i!=rf.groups.size();++i) {
        ++checks.classes;const auto& g=arena.classes()[f.class_first+i];const auto& rg=rf.groups[i];
        require(g.first==rg.ranks.first && g.last==rg.ranks.last &&
                g.credit==ca::detail::pack(rg.credit.q3,rg.credit.q4),"arena_classes");
      }
      for (std::size_t i=0;i!=rf.grouped.size();++i) {
        ++checks.ranks;const auto rank=arena.ranks()[f.first+i];
        require(rank+f.original_first==rf.grouped[i],"arena_scatter_rank");
        const auto credit=rf.credits[rf.grouped[i]-rf.original_ranks.first];
        require(arena.credits()[f.first+i]==ca::detail::pack(credit.q3,credit.q4),"arena_scatter_credit");
      }
      if (expand) {
        fp::Work pool_work;
        const auto pool=ca::detail::select_pool(index,side==0?req.a_node:req.b_node,side==0?req.b_node:req.a_node,k,pool_work);
        require(pool.size==rf.proposals.size(),"arena_pool_size");
        for (std::size_t i=0;i!=pool.size;++i) {require(pool.ids[i]==rf.proposals[i],"arena_original_id_pool");++checks.pool_ids;}
      }
      F+=f.size;C+=f.classes;
    }
    for (std::size_t i=0;i!=ref.output().bands.size();++i) {
      ++checks.bands;const auto& band=arena.bands()[rm.band_first+i];const auto& expected=ref.output().bands[i];
      require(band.a_class==expected.a_class && band.b_first==expected.b_first && band.b_last==expected.b_last,"arena_bands");
      const auto& af=arena.factors()[2*r];const auto& ag=arena.classes()[af.class_first+band.a_class];
      if (expand) {
        for (auto ai=ag.first;ai!=ag.last;++ai) for (auto bi=band.b_first;bi!=band.b_last;++bi) {
          static_cast<void>(ai);++checks.pairs;
          require(arena.pair_mask(r,band.a_class,bi)==ref.pair_mask(band.a_class,bi),"arena_pair_mask");
        }
      } else {
        for (const auto& bg:ref.b().groups) if (band.b_first<bg.ranks.last && bg.ranks.first<band.b_last)
          require(arena.pair_mask(r,band.a_class,bg.ranks.first)==ref.pair_mask(band.a_class,bg.ranks.first),"arena_pair_mask");
      }
    }
    D+=rm.bands;
  }
  require(F==arena.ranks().size() && F==arena.credits().size() && C==arena.classes().size() && D==arena.bands().size(),"arena_prefix_totals");
  require(geometry(expected_work)==geometry(arena.geometry_work()) && expected_mass==arena.mass(),"arena_geometry_once");
  require(arena.work().grouping_reads==3*F && arena.work().scatter_writes==F &&
          arena.work().band_passes==2*requests.size() && arena.work().emitted_bands==D,"arena_paid_passes");
}
void arena_gate(ca::Mutant mutant) {
  Checks checks;
  for (const char* family:{"uniform","terrain","clusters","rows"}) {
    auto points=bench::make_front_fixture(24,family,3).points;
    for (unsigned permutation=0;permutation!=2;++permutation) {
      if (permutation!=0) std::reverse(points.begin(),points.end());
      const auto cloud=prepare_cloud(points);const auto index=make_q2_cloud_index(cloud);
      for (const unsigned k:{2U,3U,5U,10U}) for (const unsigned s:{8U,10U,12U}) {
        std::vector<ca::Request> requests;
        static_cast<void>(run_wspd_front(*index,k,s,WspdFrontMode::Pure,[&](const WspdRectangle& r) {
          requests.push_back({r.a_node,r.b_node,requests.size()*3,r.lane_mask});
        },6));
        const auto one=ca::Arena::build(index,requests,k,1,7,mutant);
        judge(*index,requests,k,*one,checks,true);
        const auto four=ca::Arena::build(index,requests,k,4,7);
        compare_arenas(*one,*four);
        const auto fine=ca::Arena::build(index,requests,k,4,1);
        compare_arenas(*one,*fine,false);
        if (!requests.empty()) {
          const auto prior=one->rectangles()[0];requests[0].source_ordinal=999999;
          require(one->rectangles()[0]==prior,"arena_input_alias");
        }
      }
      const auto empty=ca::Arena::build(index,{},5,4,128);
      require(empty->rectangles().empty() && empty->retained_buffers()==0 && empty->mass()==ca::Mass{},"arena_empty_batch");
      for (unsigned failure=0;failure!=9;++failure) {
        std::vector<ca::Request> bad{{0,0,0,6}};
        if (failure==5) bad[0].a_node=index->spatial_nodes().size();
        if (failure==6) bad[0].mask=0;
        if (failure==7) bad[0].mask=1;
        if (failure==8) bad.push_back(bad[0]);
        bool rejected=false;
        try { static_cast<void>(ca::Arena::build(index,bad,failure==0?1U:failure==1?11U:5U,
                    failure==2?0U:4U,failure==3?0U:7U)); }
        catch (const std::invalid_argument&) {rejected=true;}
        require(rejected,"arena_refusal_missing");++checks.refusals;
      }
      std::vector<ca::Request> real;
      static_cast<void>(run_wspd_front(*index,5,8,WspdFrontMode::Pure,[&](const WspdRectangle& r) {
        real.push_back({r.a_node,r.b_node,real.size(),r.lane_mask});
      },6));
      bool rejected=false;
      try {static_cast<void>(ca::Arena::build(index,real,5,4,1,ca::Mutant::WorkerThrow));}
      catch (const std::runtime_error& e) {rejected=std::string_view(e.what())=="arena.injected_worker";}
      require(rejected,"arena_worker_exception");++checks.worker_throws;
    }
  }
  std::cout<<"{\"schema\":\""<<arena_schema<<"\",\"status\":\"pass\",\"mode\":\"gate\",\"coverage\":{"
    <<"\"batches\":"<<checks.batches<<",\"rectangles\":"<<checks.rectangles<<",\"factors\":"<<checks.factors
    <<",\"ranks\":"<<checks.ranks<<",\"classes\":"<<checks.classes<<",\"bands\":"<<checks.bands
    <<",\"pairs\":"<<checks.pairs<<",\"refusals\":"<<checks.refusals<<",\"pool_ids\":"<<checks.pool_ids
    <<",\"worker_throws\":"<<checks.worker_throws<<"}}\n";
}

void arena_measure(std::vector<Point3> points,unsigned k,unsigned s,std::string_view mode,std::string_view family,
                   Clock::time_point start,double input_ms) {
  const auto hash=input_hash(points);const auto tick_index=Clock::now();
  const auto cloud=prepare_cloud(points);const auto index=make_q2_cloud_index(cloud);const auto index_ms=elapsed(tick_index);
  Q34WitnessSearchWork search;Q34WitnessBoundsWork bounds;
  std::vector<ca::Request> requests;u64 ordinal=0,P=0,fallback=0,fallback_mass=0,F=0;
  const auto tick_front=Clock::now();
  static_cast<void>(run_wspd_front(*index,k,s,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& r) {
    const auto source=ordinal++;const auto& a=index->spatial_nodes()[r.a_node];const auto& b=index->spatial_nodes()[r.b_node];
    const auto mask=filter_q34_witnesses(*index,a.box,b.box,static_cast<std::uint8_t>(k),r.lane_mask,search,Q34WitnessBoundsMode::Affine,bounds);
    if (mask==0) return;
    const auto mass=fp::product(a.range.size(),b.range.size());P+=mass;
    const auto threshold=(mask&4U)!=0?k-2U:k-1U;
    if (std::min(a.range.size(),b.range.size())<2 || a.range.size()+b.range.size()-2<threshold) {++fallback;fallback_mass+=mass;return;}
    requests.push_back({r.a_node,r.b_node,source,mask});F+=a.range.size()+b.range.size();
  },6));
  const auto front_ms=elapsed(tick_front);
  std::array<double,2> one_ms{},four_ms{};std::shared_ptr<const ca::Arena> last_one,last_four;
  double comparison_ms=0;
  for (unsigned repeat=0;repeat!=2;++repeat) {
    last_one.reset();last_four.reset();
    std::shared_ptr<const ca::Arena> one,four;
    const auto first=[&] {const auto t=Clock::now();one=ca::Arena::build(index,requests,k,1,128);one_ms[repeat]=elapsed(t);};
    const auto second=[&] {const auto t=Clock::now();four=ca::Arena::build(index,requests,k,4,128);four_ms[repeat]=elapsed(t);};
    if (repeat==0) {first();second();} else {second();first();}
    const auto t=Clock::now();compare_arenas(*one,*four);comparison_ms+=elapsed(t);
    last_one=std::move(one);last_four=std::move(four);
  }
  const auto tick_judge=Clock::now();Checks checks;judge(*index,requests,k,*last_one,checks,false);const auto judge_ms=elapsed(tick_judge);
  const auto& work=last_one->work();const auto& geo=last_one->geometry_work();
  require(F==geo.factor_sites,"arena_factor_scan_once");
  std::cout<<std::fixed<<std::setprecision(6)<<"{\"schema\":\""<<arena_schema<<"\",\"status\":\"pass\",\"mode\":"<<quoted(mode)
    <<",\"family\":"<<quoted(family)<<",\"n\":"<<points.size()<<",\"k\":"<<k<<",\"s\":"<<s
    <<",\"seed\":3,\"min_factor\":2,\"grain\":128,\"input_hash\":\""<<hex(hash)<<"\",\"scope\":\"collective_CPU_no_S2_no_FULL_no_GPU\""
    <<",\"metrics\":{\"P\":"<<P<<",\"E\":"<<last_one->mass().all+fallback_mass<<",\"F\":"<<F
    <<",\"planned\":"<<requests.size()<<",\"fallback\":"<<fallback<<",\"fallback_mass\":"<<fallback_mass
    <<",\"classes\":"<<last_one->classes().size()<<",\"bands\":"<<last_one->bands().size()
    <<",\"final_buffers\":"<<last_one->retained_buffers()<<",\"retained_bytes\":"<<last_one->retained_bytes()
    <<",\"scratch_peak_one\":"<<last_one->temporary_bytes_peak()<<",\"scratch_peak_four\":"<<last_four->temporary_bytes_peak()
    <<",\"owned_array_peak_one\":"<<last_one->owned_bytes_peak_bound()<<",\"owned_array_peak_four\":"<<last_four->owned_bytes_peak_bound()
    <<",\"borrowed_cloud_index_bytes\":"<<cloud->retained_bytes()+index->retained_bytes()
    <<",\"caller_request_bytes\":"<<requests.capacity()*sizeof(ca::Request)<<",\"anchor_jobs\":"<<work.anchor_jobs
    <<",\"max_factor\":"<<work.max_factor<<",\"grouping_reads\":"<<work.grouping_reads
    <<",\"scatter_writes\":"<<work.scatter_writes<<",\"histogram_slots\":"<<work.histogram_slots
    <<",\"class_visits\":"<<work.class_visits<<",\"band_passes\":"<<work.band_passes
    <<",\"band_rows\":"<<work.band_rows<<",\"band_binary_tests\":"<<work.band_binary_tests
    <<",\"corner_tests\":"<<geo.predicates.corner_tests<<",\"selected_sites\":"<<geo.selected_sites
    <<",\"anchor_visits\":"<<geo.anchor_visits<<",\"prefix_entries\":"<<work.prefix_entries
    <<"},\"times_ms\":{\"input\":"<<input_ms<<",\"index\":"<<index_ms<<",\"front_filter\":"<<front_ms
    <<",\"one_ab\":"<<one_ms[0]<<",\"four_ab\":"<<four_ms[0]<<",\"four_ba\":"<<four_ms[1]<<",\"one_ba\":"<<one_ms[1]
    <<",\"comparison\":"<<comparison_ms<<",\"direct_reference_check\":"<<judge_ms<<",\"total\":"<<elapsed(start)<<"}}\n";
}
int main(int argc,char** argv) {
  try {
    if (argc==2 && std::string_view(argv[1])=="--gate") {arena_gate(ca::Mutant::None);return 0;}
    if (argc==3 && std::string_view(argv[1])=="--mutant") {
      const std::string_view name=argv[2];
      if (name=="scatter") arena_gate(ca::Mutant::Scatter);
      else if (name=="mask6") arena_gate(ca::Mutant::Mask6);
      else throw std::invalid_argument("arena.unknown_mutant");
      throw std::runtime_error("arena.mutant_survived");
    }
    const auto start=Clock::now();
    if (argc==6 && std::string_view(argv[1])=="--synthetic") {
      auto fixture=bench::make_front_fixture(number<std::size_t>(argv[3]),argv[2],3);const auto ms=elapsed(start);
      arena_measure(std::move(fixture.points),number<unsigned>(argv[4]),number<unsigned>(argv[5]),"synthetic",argv[2],start,ms);return 0;
    }
    if (argc==5 && std::string_view(argv[1])=="--frame") {
      auto points=load_frame(argv[2]);const auto ms=elapsed(start);
      arena_measure(std::move(points),number<unsigned>(argv[3]),number<unsigned>(argv[4]),"frame","none",start,ms);return 0;
    }
    throw std::invalid_argument("arena.usage");
  } catch (const std::exception& error) {
    std::cout<<"{\"schema\":\""<<arena_schema<<"\",\"status\":\"failed\",\"cause\":"<<quoted(error.what())<<"}\n";return 1;
  }
}
