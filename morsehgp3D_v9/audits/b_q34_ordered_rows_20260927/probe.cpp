// Explicit reuse of the frozen Pool input/front helpers, not its entry point.
#define main pool_unused_main
#include "../b_q34_factor_plan_20260926/probe.cpp"
#undef main
#include "rows.hpp"
namespace rr = mhgp9::audit::ordered_rows;
constexpr const char* row_schema = "mhgp9_q34_ordered_rows_v1";

struct Checks { u64 cases{}, pairs{}, fronts{}, implicit{}, order_changes{}, refusals{}; };
void compare(const fp::Plan& old, const rr::Plan& row, unsigned k, unsigned mask,
             Checks& checks, bool expand) {
  require(row.mass()==old.union_mass && row.q3_mass()==old.q3_mass && row.q4_mass()==old.q4_mass,
          "ordered_mass");
  require(row.work()==fp::product(old.a.groups.size(),old.b.credits.size()),"ordered_work");
  require(row.ranks().size()<=row.mass() && row.ranks().size()<=row.work(),"ordered_storage_bound");
  ++checks.cases;
  checks.implicit+=fp::product(old.a.credits.size(),old.b.credits.size())-row.mass();
  const auto offsets=row.offsets();
  require(offsets.size()==old.a.groups.size()+1 && offsets.front()==0 && offsets.back()==row.ranks().size(),
          "ordered_shape");
  for (std::size_t s=0;s!=old.a.groups.size();++s) {
    const auto& c=old.a.groups[s].credit;
    auto cursor=offsets[s];
    for (std::size_t b=0;b!=old.b.credits.size();++b) {
      // Reference formula independent of mask_for() and class-row output.
      const auto bc=old.b.credits[b];
      unsigned m=0;
      if ((mask&2U)!=0 && unsigned(c.q3)+bc.q3<k-1) m|=2U;
      if ((mask&4U)!=0 && unsigned(c.q4)+bc.q4<k-2) m|=4U;
      if (m==0) continue;
      require(cursor<offsets[s+1] && row.ranks()[cursor]==b,"ordered_original_B");
      require(row.masks()[cursor]==m,"ordered_mask");
      ++cursor;
    }
    require(cursor==offsets[s+1],"ordered_complete_B");
  }
  if (!expand) return;
  u64 emitted=0;
  bool changed=false;
  for (std::size_t a=0;a!=old.a.credits.size();++a) {
    const auto slot=row.a_slots()[a];
    require(slot<old.a.groups.size() && old.a.groups[slot].credit==old.a.credits[a],"ordered_A_slot");
    auto j=offsets[slot];
    for (std::size_t b=0;b!=old.b.credits.size();++b) {
      ++checks.pairs;
      const auto ac=old.a.credits[a],bc=old.b.credits[b];
      unsigned expected=0;
      if ((mask&2U)!=0 && unsigned(ac.q3)+bc.q3<k-1) expected|=2U;
      if ((mask&4U)!=0 && unsigned(ac.q4)+bc.q4<k-2) expected|=4U;
      if (expected==0) continue;
      require(j<offsets[slot+1] && row.ranks()[j]==b && row.masks()[j]==expected,"ordered_row_major");
      ++emitted; ++j;
    }
    require(j==offsets[slot+1],"ordered_row_size");
    changed=changed || old.a.grouped[a]!=old.a.original_ranks.first+a;
  }
  require(emitted==row.mass(),"ordered_emission_mass");
  if (changed) ++checks.order_changes;
}

void gate(rr::Mutant mutant) {
  Checks checks;
  for (const char* family : {"uniform","terrain","clusters","rows"}) {
    auto points=bench::make_front_fixture(24,family,3).points;
    for (unsigned reverse=0;reverse!=2;++reverse) {
      if (reverse) std::reverse(points.begin(),points.end());
      const auto ix=make_q2_cloud_index(prepare_cloud(points));
      for (unsigned k:{2U,3U,5U,10U}) for (unsigned s:{8U,10U,12U}) {
        ++checks.fronts;
        static_cast<void>(run_wspd_front(*ix,k,s,WspdFrontMode::Pure,[&](const WspdRectangle& r) {
          const fp::Plan old(*ix,r.a_node,r.b_node,k,r.lane_mask);
          const rr::Plan now(old.a.credits,old.b.credits,k,r.lane_mask,mutant);
          compare(old,now,k,r.lane_mask,checks,true);
        },6));
      }
    }
  }
  const std::vector<fp::Credit> empty, zero{{0,0}}, saturated{{4,3}}, bad{{10,0}};
  const rr::Plan empty_a(empty,zero,5,6),empty_b(zero,empty,5,6),dead(saturated,saturated,5,6),
      k1(zero,zero,1,6);
  require(empty_a.mass()==0 && empty_b.mass()==0 && dead.mass()==0 && k1.mass()==0,"ordered_empty_cases");
  for (unsigned f=0;f!=4;++f) {
    bool refused=false;
    try { const rr::Plan p(f==3?bad:zero,zero,f==0?0:f==1?11:5,f==2?1:6); }
    catch (const std::invalid_argument&) { refused=true; }
    require(refused,"ordered_refusal"); ++checks.refusals;
  }
  require(checks.fronts==96 && checks.implicit>0 && checks.order_changes>0,"ordered_nonvacuity");
  std::cout<<"{\"schema\":\""<<row_schema<<"\",\"status\":\"pass\",\"mode\":\"gate\",\"cases\":"<<checks.cases
    <<",\"pairs\":"<<checks.pairs<<",\"fronts\":"<<checks.fronts<<",\"implicit\":"<<checks.implicit
    <<",\"order_changes\":"<<checks.order_changes<<",\"refusals\":"<<checks.refusals<<"}\n";
}

void measure(std::vector<Point3> points,unsigned k,unsigned s,std::string_view mode,
             std::string_view family,Clock::time_point start) {
  require(k>=2 && k<=10 && (s==8 || s==10 || s==12),"ordered_measure_domain");
  const auto hash=input_hash(points),n=points.size();
  const auto ix=make_q2_cloud_index(prepare_cloud(points));
  Q34WitnessSearchWork sw; Q34WitnessBoundsWork bw; Checks checks;
  u64 P=0,E=0,E3=0,E4=0,F=0,FA=0,FB=0,W=0,T=0,classes=0,nonempty=0,planned=0,fallback=0;
  u64 bytes=0,logical=0,peak=0,old_bytes=0;
  double pool_ms=0,rows_ms=0,verify_ms=0;
  const auto front=run_wspd_front(*ix,k,s,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& r) {
    const auto& a=ix->spatial_nodes()[r.a_node]; const auto& b=ix->spatial_nodes()[r.b_node];
    const auto mask=filter_q34_witnesses(*ix,a.box,b.box,static_cast<std::uint8_t>(k),r.lane_mask,
                                      sw,Q34WitnessBoundsMode::Affine,bw);
    if (!mask) return;
    const auto p=fp::product(a.range.size(),b.range.size()); P+=p;
    const unsigned threshold=(mask&4U)!=0?k-2:k-1;
    if (std::min(a.range.size(),b.range.size())<2 || a.range.size()+b.range.size()-2<threshold) {
      ++fallback; E+=p; if (mask&2U) E3+=p; if (mask&4U) E4+=p; return;
    }
    auto tick=Clock::now(); const fp::Plan old(*ix,r.a_node,r.b_node,k,mask); pool_ms+=elapsed(tick);
    tick=Clock::now(); const rr::Plan row(old.a.credits,old.b.credits,k,mask); rows_ms+=elapsed(tick);
    tick=Clock::now(); compare(old,row,k,mask,checks,false); verify_ms+=elapsed(tick);
    ++planned; F+=old.work.factor_sites; FA+=a.range.size(); FB+=b.range.size();
    E+=row.mass(); E3+=row.q3_mass(); E4+=row.q4_mass(); W+=row.work(); T+=row.ranks().size();
    classes+=row.classes(); nonempty+=row.nonempty(); old_bytes+=old.retained_bytes();
    bytes+=row.retained_bytes(); logical+=row.logical_bytes();
    peak=std::max(peak,static_cast<u64>(row.retained_bytes()));
  },6);
  std::cout<<std::fixed<<std::setprecision(6)<<"{\"schema\":\""<<row_schema<<"\",\"status\":\"pass\",\"scope\":\"ordered_representation_old_pool_paid_no_S2_FULL_GPU\",\"mode\":"<<quoted(mode)
    <<",\"family\":"<<quoted(family)<<",\"n\":"<<n<<",\"k\":"<<k<<",\"s\":"<<s<<",\"input_hash\":\""<<hex(hash)
    <<"\",\"metrics\":{\"P\":"<<P<<",\"E\":"<<E<<",\"E3\":"<<E3<<",\"E4\":"<<E4<<",\"F\":"<<F
    <<",\"FA\":"<<FA<<",\"FB\":"<<FB<<",\"W\":"<<W<<",\"T\":"<<T<<",\"classes\":"<<classes<<",\"nonempty\":"<<nonempty
    <<",\"planned\":"<<planned<<",\"fallback\":"<<fallback<<",\"capacity_bytes\":"<<bytes<<",\"logical_bytes\":"<<logical
    <<",\"peak_plan_bytes\":"<<peak<<",\"old_plan_bytes\":"<<old_bytes<<",\"front_F\":"<<front.work.emitted_factor_sites
    <<",\"histogram_slots\":"<<100*planned<<"},\"times_ms\":{\"pool\":"<<pool_ms<<",\"ordered_rows\":"<<rows_ms
    <<",\"verification\":"<<verify_ms<<",\"total\":"<<elapsed(start)<<"}}\n";
}

int main(int argc,char** argv) {
  try {
    if (argc==2 && std::string_view(argv[1])=="--gate") { gate(rr::Mutant::None); return 0; }
    if (argc==3 && std::string_view(argv[1])=="--mutant") {
      const auto name=std::string_view(argv[2]);
      require(name=="reverse-b" || name=="mask6","ordered_mutant_name");
      gate(name=="reverse-b"?rr::Mutant::ReverseB:rr::Mutant::WidenMask);
      throw std::runtime_error("ordered_mutant_survived");
    }
    const auto start=Clock::now();
    if (argc==6 && std::string_view(argv[1])=="--synthetic") {
      auto f=bench::make_front_fixture(number<std::size_t>(argv[3]),argv[2],3);
      measure(std::move(f.points),number<unsigned>(argv[4]),number<unsigned>(argv[5]),"synthetic",argv[2],start); return 0;
    }
    if (argc==5 && std::string_view(argv[1])=="--frame") {
      measure(load_frame(argv[2]),number<unsigned>(argv[3]),number<unsigned>(argv[4]),"frame","none",start); return 0;
    }
    throw std::invalid_argument("ordered.usage");
  } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
