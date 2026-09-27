// Frozen recipes/loader only; the old entry point is never called.
#define main factor_unused_main
#include "../b_q34_factor_plan_20260926/probe.cpp"
#undef main
#include "direct.hpp"
#include <optional>
#include <type_traits>

namespace db=mhgp9::audit::direct_bands;
constexpr const char* direct_schema="mhgp9_q34_direct_bands_v1";
static_assert(!std::is_copy_constructible_v<db::Plan> && !std::is_move_constructible_v<db::Plan>);
static_assert(std::is_same_v<decltype(std::declval<db::Plan&>().a()),const fp::Factor&>);
struct Checks { u64 cases{},cells{},pairs{},fronts{},rectangles{},refusals{},geometry_fields{}; };

unsigned expected_mask(fp::Credit a,fp::Credit b,unsigned k,unsigned mask) {
  unsigned out=0;
  if (k>=2 && (mask&2U)!=0 && unsigned(a.q3)+b.q3<k-1U) out|=2U;
  if (k>=3 && (mask&4U)!=0 && unsigned(a.q4)+b.q4<k-2U) out|=4U;
  return out;
}
auto geometric_work(const fp::Work& w) {
  return std::array{w.factor_sites,w.selection_visits,w.selection_tests,w.selection_shifts,
    w.selected_sites,w.anchor_visits,w.witness_attempts,w.self_skips,w.q3_credits,w.q4_credits,
    w.grouping_visits,w.grouping_sort_tests,w.occupied_classes,w.class_slots,
    w.predicates.point_tests,w.predicates.universal_queries,w.predicates.q2_axis_terms,
    w.predicates.corner_tests,w.predicates.block_bound_tests,w.predicates.negative_probes};
}
void check_factor(const fp::Factor& old,const fp::Factor& now) {
  require(old.original_ranks.first==now.original_ranks.first && old.original_ranks.last==now.original_ranks.last,
          "direct_factor_ranges");
  require(old.proposals==now.proposals,"direct_factor_proposals");
  require(old.credits==now.credits && old.grouped==now.grouped && old.groups.size()==now.groups.size(),
          "direct_factor_storage");
  for (std::size_t i=0;i!=old.groups.size();++i)
    require(old.groups[i].credit==now.groups[i].credit && old.groups[i].ranks.first==now.groups[i].ranks.first &&
            old.groups[i].ranks.last==now.groups[i].ranks.last,"direct_factor_groups");
}
template<class Mask>
void check_bands(const fp::Factor& a,const fp::Factor& b,const db::BandData& output,
                 unsigned k,unsigned mask,Mask decode,Checks& checks,bool expand) {
  ++checks.cases;
  u64 mass=0,q3=0,q4=0;
  for (std::size_t ai=0;ai!=a.groups.size();++ai) {
    const auto& ag=a.groups[ai];
    std::size_t count=0;
    for (const auto& band:output.bands) {
      require(band.a_class<a.groups.size() && band.b_first<band.b_last && band.b_last<=b.grouped.size(),
              "direct_band_bounds");
      if (band.a_class==ai) ++count;
    }
    require(count<=k+1,"direct_band_count");
    for (const auto& bg:b.groups) {
      ++checks.cells;
      unsigned covered=0;
      for (const auto& band:output.bands) if (band.a_class==ai &&
          bg.ranks.first<band.b_last && band.b_first<bg.ranks.last) {
        require(band.b_first<=bg.ranks.first && bg.ranks.last<=band.b_last,"direct_split_class"); ++covered;
      }
      require(covered<=1,"direct_overlap_duplicate");
      const auto expected=expected_mask(ag.credit,bg.credit,k,mask);
      require((covered==1)==(expected!=0),"direct_band_coverage");
      if (covered!=0) {
        require(decode(ai,bg.ranks.first)==expected && decode(ai,bg.ranks.last-1)==expected,"direct_pair_mask");
        const auto size=fp::product(ag.ranks.size(),bg.ranks.size());
        mass+=size;
        if ((expected&2U)!=0) q3+=size;
        if ((expected&4U)!=0) q4+=size;
      }
    }
  }
  require(mass==output.union_mass && q3==output.q3_mass && q4==output.q4_mass,"direct_union_masses");
  if (!expand) return;
  std::vector<unsigned char> emitted(a.grouped.size()*b.grouped.size(),0);
  for (const auto& band:output.bands) {
    const auto ar=a.groups[band.a_class].ranks;
    for (auto ai=ar.first;ai!=ar.last;++ai) for (auto bi=std::size_t{band.b_first};bi!=band.b_last;++bi) {
      auto& entry=emitted[ai*b.grouped.size()+bi];
      require(entry==0,"direct_pair_duplicate");
      entry=static_cast<unsigned char>(decode(band.a_class,bi));
    }
  }
  for (std::size_t ai=0;ai!=a.grouped.size();++ai) for (std::size_t bi=0;bi!=b.grouped.size();++bi) {
    ++checks.pairs;
    require(emitted[ai*b.grouped.size()+bi]==expected_mask(a.credits[a.grouped[ai]-a.original_ranks.first],
        b.credits[b.grouped[bi]-b.original_ranks.first],k,mask),"direct_expanded_pair");
  }
}
void compare(const fp::Plan& old,const db::Plan& now,unsigned k,unsigned mask,Checks& checks,bool expand) {
  check_factor(old.a,now.a()); check_factor(old.b,now.b());
  require(geometric_work(old.work)==geometric_work(now.work()),"direct_geometric_work");
  checks.geometry_fields+=geometric_work(old.work).size();
  require(now.work().cells_tested==0 && now.work().descriptors==0,"direct_old_cells_built");
  require(old.union_mass==now.output().union_mass && old.q3_mass==now.output().q3_mass &&
          old.q4_mass==now.output().q4_mass,"direct_native_mass");
  check_bands(now.a(),now.b(),now.output(),k,mask,[&](auto ai,auto bi) {return now.pair_mask(ai,bi);},checks,expand);
}
fp::Factor manufactured(unsigned k,unsigned pattern,std::size_t origin) {
  fp::Factor f;
  for (unsigned x=0;x<=(k>=2?k-1:0);++x) for (unsigned y=0;y<=(k>=3?k-2:0);++y) {
    const unsigned count=pattern==0?0:pattern==1?1:pattern==2?2:(7*x+11*y+pattern)%4;
    for (unsigned i=0;i!=count;++i) f.credits.push_back({static_cast<std::uint8_t>(x),static_cast<std::uint8_t>(y)});
  }
  if ((pattern&1U)!=0) std::reverse(f.credits.begin(),f.credits.end());
  f.original_ranks={origin,origin+f.credits.size()};
  for (std::size_t i=0;i!=f.credits.size();++i) f.grouped.push_back(origin+i);
  std::stable_sort(f.grouped.begin(),f.grouped.end(),[&](auto x,auto y) {
    const auto a=f.credits[x-origin],b=f.credits[y-origin];return 10*a.q3+a.q4<10*b.q3+b.q4;
  });
  for (std::size_t i=0;i!=f.grouped.size();) {
    const auto first=i; const auto c=f.credits[f.grouped[i]-origin];
    while (i!=f.grouped.size() && f.credits[f.grouped[i]-origin]==c) ++i;
    f.groups.push_back({c,{first,i}});
  }
  return f;
}
void direct_gate(db::Mutant mutant) {
  Checks checks;
  for (unsigned k=1;k<=10;++k) for (const unsigned mask:{0U,2U,4U,6U})
    for (unsigned ap=0;ap!=6;++ap) for (unsigned bpatt=0;bpatt!=6;++bpatt) {
      const auto a=manufactured(k,ap,127),b=manufactured(k,bpatt,911);
      const auto active=mask & (k<2?0U:k<3?2U:6U);
      const auto output=db::detail::group(a,b,k,active,mutant);
      check_bands(a,b,output,k,mask,[&](auto ai,auto bi) {
        return expected_mask(a.groups[ai].credit,b.credits[b.grouped[bi]-b.original_ranks.first],k,mask);
      },checks,true);
    }
  for (const char* family:{"uniform","terrain","clusters","rows"}) {
    auto points=bench::make_front_fixture(24,family,3).points;
    for (unsigned permutation=0;permutation!=2;++permutation) {
      if (permutation!=0) std::reverse(points.begin(),points.end());
      const auto cloud=prepare_cloud(points);const auto index=make_q2_cloud_index(cloud);
      for (const unsigned k:{2U,3U,5U,10U}) for (const unsigned s:{8U,10U,12U}) {
        ++checks.fronts;
        static_cast<void>(run_wspd_front(*index,k,s,WspdFrontMode::Pure,[&](const WspdRectangle& r) {
          const fp::Plan old(*index,r.a_node,r.b_node,k,r.lane_mask);
          const db::Plan now(*index,r.a_node,r.b_node,k,r.lane_mask,mutant);
          compare(old,now,k,r.lane_mask,checks,true);++checks.rectangles;
        },6));
      }
      for (unsigned failure=0;failure!=7;++failure) {
        const unsigned k=failure==0?1U:failure==1?11U:failure==2?2U:5U;
        const unsigned mask=failure==3?0U:failure==4?1U:6U;
        const auto an=failure==5?index->spatial_nodes().size():std::size_t{0};
        bool rejected=false;
        try { const db::Plan bad(*index,an,0,k,mask); }
        catch (const std::invalid_argument&) { rejected=true; }
        require(rejected,"direct_refusal_missing");++checks.refusals;
      }
    }
  }
  std::cout<<"{\"schema\":\""<<direct_schema<<"\",\"status\":\"pass\",\"mode\":\"gate\",\"cases\":"<<checks.cases
    <<",\"pairs\":"<<checks.pairs<<",\"cells\":"<<checks.cells<<",\"fronts\":"<<checks.fronts
    <<",\"rectangles\":"<<checks.rectangles<<",\"refusals\":"<<checks.refusals
    <<",\"geometry_fields\":"<<checks.geometry_fields<<"}\n";
}

void direct_measure(std::vector<Point3> points,unsigned k,unsigned s,std::string_view mode,
                    std::string_view family,Clock::time_point start,double input_ms) {
  if (k<2 || k>10 || (s!=8 && s!=10 && s!=12)) throw std::invalid_argument("direct.measure_K_s");
  const auto hash=input_hash(points),n=points.size();
  const auto tick_index=Clock::now();const auto cloud=prepare_cloud(points);const auto index=make_q2_cloud_index(cloud);
  const auto index_ms=elapsed(tick_index);
  Q34WitnessSearchWork search;Q34WitnessBoundsWork bounds;Checks checks;
  u64 P=0,E=0,E3=0,E4=0,F=0,planned=0,fallback=0,cells=0,band_count=0,old_bytes=0,new_bytes=0;
  u64 cell_bytes=0,band_bytes=0,old_buffers=0,new_buffers=0,old_cells_tested=0,group_b=0,row_tests=0,binary_tests=0;
  u64 corners=0,proposals=0,old_peak=0,new_peak=0;
  std::array<double,2> old_ms{},new_ms{};
  double filter_ms=0,verify_ms=0;
  const auto front_start=Clock::now();
  const auto front=run_wspd_front(*index,k,s,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& r) {
    const auto& a=index->spatial_nodes()[r.a_node];const auto& b=index->spatial_nodes()[r.b_node];
    auto tick=Clock::now();
    const auto mask=filter_q34_witnesses(*index,a.box,b.box,static_cast<std::uint8_t>(k),r.lane_mask,
                                      search,Q34WitnessBoundsMode::Affine,bounds);
    filter_ms+=elapsed(tick);if (mask==0) return;
    const auto mass=fp::product(a.range.size(),b.range.size());P+=mass;
    const unsigned threshold=(mask&4U)!=0?k-2U:k-1U;
    if (std::min(a.range.size(),b.range.size())<2 || a.range.size()+b.range.size()-2<threshold) {
      ++fallback;E+=mass;if ((mask&2U)!=0) E3+=mass;if ((mask&4U)!=0) E4+=mass;return;
    }
    for (unsigned pair=0;pair!=2;++pair) {
      std::optional<fp::Plan> old;std::optional<db::Plan> now;
      const auto build_old=[&] { const auto t=Clock::now();old.emplace(*index,r.a_node,r.b_node,k,mask);old_ms[pair]+=elapsed(t); };
      const auto build_new=[&] { const auto t=Clock::now();now.emplace(*index,r.a_node,r.b_node,k,mask);new_ms[pair]+=elapsed(t); };
      if (pair==0) { build_old();build_new(); } else { build_new();build_old(); }
      tick=Clock::now();compare(*old,*now,k,mask,checks,false);verify_ms+=elapsed(tick);
      if (pair!=0) continue;
      ++planned;F+=old->work.factor_sites;E+=now->output().union_mass;
      E3+=now->output().q3_mass;E4+=now->output().q4_mass;
      cells+=old->blocks.size();band_count+=now->output().bands.size();
      old_cells_tested+=old->work.cells_tested;
      group_b+=now->output().work.classes_b;row_tests+=now->output().work.row_tests;
      binary_tests+=now->output().work.binary_tests;
      corners+=old->work.predicates.corner_tests;proposals+=old->work.selected_sites;
      old_bytes+=old->retained_bytes();new_bytes+=now->retained_bytes();
      old_peak=std::max(old_peak,static_cast<u64>(old->retained_bytes()));
      new_peak=std::max(new_peak,static_cast<u64>(now->retained_bytes()));
      cell_bytes+=old->blocks.capacity()*sizeof(fp::Block);
      band_bytes+=now->output().bands.capacity()*sizeof(db::Band);
      old_buffers+=8U+static_cast<u64>(old->blocks.capacity()!=0);new_buffers+=now->retained_buffers();
    }
  },6);
  const auto front_ms=elapsed(front_start),total_ms=elapsed(start);
  std::cout<<std::fixed<<std::setprecision(6)<<"{\"schema\":\""<<direct_schema
    <<"\",\"status\":\"pass\",\"scope\":\"direct_preparation_paired_no_S2_no_FULL_no_GPU\",\"mode\":"<<quoted(mode)
    <<",\"family\":"<<quoted(family)<<",\"n\":"<<n<<",\"k\":"<<k<<",\"s\":"<<s
    <<",\"seed\":3,\"min_factor\":2,\"input_hash\":\""<<hex(hash)<<"\",\"paired_order\":\"ABBA_per_rectangle\""
    <<",\"metrics\":{\"P\":"<<P<<",\"E\":"<<E<<",\"E3\":"<<E3<<",\"E4\":"<<E4
    <<",\"F\":"<<F<<",\"front_F\":"<<front.work.emitted_factor_sites<<",\"planned\":"<<planned
    <<",\"fallback\":"<<fallback<<",\"cells\":"<<cells<<",\"bands\":"<<band_count
    <<",\"old_cells_tested\":"<<old_cells_tested<<",\"new_cells_tested\":0,\"groups_b\":"<<group_b
    <<",\"row_tests\":"<<row_tests<<",\"binary_tests\":"<<binary_tests<<",\"corner_tests\":"<<corners
    <<",\"selected_sites\":"<<proposals<<",\"old_bytes\":"<<old_bytes<<",\"new_bytes\":"<<new_bytes
    <<",\"old_peak\":"<<old_peak<<",\"new_peak\":"<<new_peak<<",\"cell_bytes\":"<<cell_bytes
    <<",\"band_bytes\":"<<band_bytes<<",\"old_retained_buffers\":"<<old_buffers
    <<",\"new_retained_buffers\":"<<new_buffers<<",\"verified_cells\":"<<checks.cells
    <<",\"geometry_fields_verified\":"<<checks.geometry_fields<<"},\"times_ms\":{\"input\":"<<input_ms
    <<",\"index\":"<<index_ms<<",\"front_all_pairs\":"<<front_ms<<",\"filter\":"<<filter_ms
    <<",\"old_ab\":"<<old_ms[0]<<",\"new_ab\":"<<new_ms[0]<<",\"new_ba\":"<<new_ms[1]
    <<",\"old_ba\":"<<old_ms[1]<<",\"verification\":"<<verify_ms<<",\"total\":"<<total_ms<<"}}\n";
}
int main(int argc,char** argv) {
  try {
    if (argc==2 && std::string_view(argv[1])=="--gate") { direct_gate(db::Mutant::None);return 0; }
    if (argc==3 && std::string_view(argv[1])=="--mutant") {
      const std::string_view name=argv[2];
      if (name=="overlap") direct_gate(db::Mutant::Overlap);
      else if (name=="mask6") direct_gate(db::Mutant::Mask6);
      else if (name=="ids") direct_gate(db::Mutant::Ids);
      else throw std::invalid_argument("direct.unknown_mutant");
      throw std::runtime_error("direct.mutant_survived");
    }
    const auto start=Clock::now();
    if (argc==6 && std::string_view(argv[1])=="--synthetic") {
      auto fixture=bench::make_front_fixture(number<std::size_t>(argv[3]),argv[2],3);const auto input_ms=elapsed(start);
      direct_measure(std::move(fixture.points),number<unsigned>(argv[4]),number<unsigned>(argv[5]),"synthetic",argv[2],start,input_ms);return 0;
    }
    if (argc==5 && std::string_view(argv[1])=="--frame") {
      auto points=load_frame(argv[2]);const auto input_ms=elapsed(start);
      direct_measure(std::move(points),number<unsigned>(argv[3]),number<unsigned>(argv[4]),"frame","none",start,input_ms);return 0;
    }
    throw std::invalid_argument("direct.usage");
  } catch (const std::exception& error) {
    std::cout<<"{\"schema\":\""<<direct_schema<<"\",\"status\":\"failed\",\"cause\":"<<quoted(error.what())<<"}\n";return 1;
  }
}
