#include "native_a.hpp"
#include "fixtures.hpp"
#include <iostream>
#include <string_view>
namespace am=mhgp9::audit::a_manifest;
using namespace mhgp9::tower;
namespace {
void same_forest(const FullCoverageCertificate& a,const FullCoverageCertificate& b) {
  am::need(a.order()==b.order() && a.parents()==b.parents() && a.successors()==b.successors() && a.nodes().size()==b.nodes().size(),"passthrough.topology");
  const auto ap=a.populations(),bp=b.populations();am::need(bool(ap) && bool(bp) && ap->domain()==bp->domain() && ap->rows().size()==bp->rows().size(),"passthrough.bank");
  for(size_t j=0;j<ap->rows().size();++j) am::need(ap->rows()[j].interior==bp->rows()[j].interior && ap->rows()[j].shell==bp->rows()[j].shell,"passthrough.population");
  for(size_t j=0;j<a.nodes().size();++j) {const auto& x=a.nodes()[j];const auto& y=b.nodes()[j];
    am::need(x.level==y.level && x.first==y.first && x.parent_count==y.parent_count,"passthrough.node");}
  am::need(a.contributions().size()==b.contributions().size(),"passthrough.contribution_count");
  for(size_t j=0;j<a.contributions().size();++j) {const auto& x=a.contributions()[j];const auto& y=b.contributions()[j];
    am::need(x.level==y.level && x.segment==y.segment && am::same_ref(x.ref,y.ref),"passthrough.contribution");}
}
void same_tower(const FullBallTowerResult& a,const FullBallTowerResult& b) {
  am::need(a.status==FullBallStatus::kCompleteRelative && b.status==a.status && std::string(a.reason)==b.reason && a.orders.size()==b.orders.size(),"passthrough.status");
  for(size_t k=0;k<a.orders.size();++k) {same_forest(a.orders[k].forest,b.orders[k].forest);am::need(a.orders[k].lower_nodes==b.orders[k].lower_nodes,"passthrough.verticals");}
}
FullBallTowerResult observed(const CloudIndex& index,const std::vector<BallData>& balls,unsigned k,int workers,
                           const FullBallTowerOptions& options) {
  // Same success route as native build_tower; any exception aborts this audit
  // call and invalidates all slots, not a new product refusal precedence.
  FullBallTowerResult out;
  out.orders=full_ball_detail::AObservedBuilder(index,balls,k,out.stats,workers,{},true,&out.times,options).run();
  out.status=FullBallStatus::kCompleteRelative;out.reason=kFullBallAuthority;return out;
}
struct Counts {u64 fixtures{},captures{},orders{},blocks{},targets{},actions{},contributions{},inert{},continuations{},
  extra{},global_run_gaps{},max_parents{},k1_nonidentity{},raw_roots{},reverse_slot_passes{},refusals{},capture_reuse_refused{},
  late_failure_refused{},duplicate_roots{},shared_plateau_roots{},parent_incidence{};
  std::array<u64,5> mutants{};};
template<class F> void refuses(F fn,const char* reason,Counts& c) {
  try {fn();} catch(const std::runtime_error& e) {am::need(std::string(e.what())==reason,"gate.wrong_refusal");++c.refusals;return;}
  throw std::runtime_error("gate.missing_refusal");
}
am::Manifest erase_inert(const am::Manifest& in,size_t b) {
  auto out=in;const auto first=out.representative_begin[b],last=out.representative_begin[b+1];
  am::need(first<last,"mutant.inert_has_target");const auto replacement=out.target[first];
  out.target.erase(out.target.begin()+static_cast<size_t>(first),out.target.begin()+static_cast<size_t>(last));
  out.representative_begin.erase(out.representative_begin.begin()+b+1);
  for(size_t i=b+1;i<out.representative_begin.size();++i) out.representative_begin[i]-=last-first;
  out.blocks.erase(out.blocks.begin()+b);
  for(auto& target:out.target) if(out.k!=1) {if(target==b) target=replacement;if(target>b) --target;}
  u32 first_ball=am::absent32;
  for(size_t i=0;i<out.blocks.size();++i) {auto& row=out.blocks[i];row.program_ordinal=i;
    if(i==0 || row.run!=out.blocks[i-1].run) first_ball=row.ball;
    row.lot_first_ball=first_ball;}
  am::validate_manifest(out);return out;
}
void adversaries(const am::Manifest& m,const am::Slot& slot,const am::Output& golden,Counts& c) {
  auto bad=m;bad.representative_begin.back()=am::absent;
  refuses([&]{am::validate_manifest(bad);},"manifest.csr_shape",c);
  bad=m;bad.k=0;refuses([&]{am::validate_manifest(bad);},"manifest.domain",c);
  if(!m.target.empty()) {bad=m;bad.target[0]=am::absent;
    refuses([&]{am::validate_manifest(bad);},m.k==1?"manifest.k1_target":"manifest.target_domain",c);}
  if(!m.blocks.empty()) {bad=m;bad.blocks[0].run=0;refuses([&]{am::validate_manifest(bad);},"manifest.block_identity",c);}
  if(m.k==1 && !c.mutants[0]) {
    for(size_t b=1;b+1<m.representative_begin.size();++b) if(m.representative_begin[b]<m.representative_begin[b+1]) {
      bad=m;++bad.representative_begin[b];am::validate_manifest(bad);
      refuses([&]{am::validate_binding(bad,slot);},"binding.representative_count",c);++c.mutants[0];break;
    }
  }
  if(!c.mutants[1]) for(size_t b=0;b<m.blocks.size();++b) {
    const auto& row=m.blocks[b];if(row.shell_mask || row.include_interior) continue;
    const auto first=golden.root_begin[b],last=golden.root_begin[b+1];if(first==last) continue;
    if(!std::all_of(golden.roots.begin()+static_cast<size_t>(first),golden.roots.begin()+static_cast<size_t>(last),
                   [&](u64 root){return root==golden.roots[first];})) continue;
    bad=erase_inert(m,b);refuses([&]{am::validate_binding(bad,slot);},"binding.block_count",c);++c.mutants[1];break;
  }
  if(m.k==1 && !c.mutants[2]) for(size_t j=0;j<m.target.size();++j) {
    const auto point=m.domain[m.target[j]];const auto found=std::find(slot.input.spatial_ids.begin(),slot.input.spatial_ids.end(),point);
    am::need(found!=slot.input.spatial_ids.end(),"mutant.geometric_id_map");const u64 rank=found-slot.input.spatial_ids.begin();
    if(rank==m.target[j]) continue;
    bad=m;bad.target[j]=rank;am::validate_manifest(bad);
    refuses([&]{am::validate_binding(bad,slot);},"binding.target",c);++c.mutants[2];break;
  }
  if(!c.mutants[3]) {
    bool changes=false;
    for(size_t begin=0;begin<m.blocks.size();) {size_t end=begin+1;while(end<m.blocks.size() && m.blocks[end].run==m.blocks[begin].run) ++end;
      for(size_t b=begin;b<end;++b) if(m.blocks[b].shell_mask || m.blocks[b].include_interior) {
        changes|=m.blocks[b].level!=m.blocks[begin].level;break;}
      begin=end;
    }
    if(changes) {const auto trial=am::replay_a(m,am::ReplayMutant::FirstContributorLevel);
      refuses([&]{am::compare_output(trial,golden);},"replay.level_representation",c);++c.mutants[3];}
  }
  if(!c.mutants[4]) {
    bool continuation=false;for(size_t a=0;a<golden.draft.actions();++a)
      continuation|=golden.draft.parent_begin[a+1]-golden.draft.parent_begin[a]==1;
    if(continuation) {const auto trial=am::replay_a(m,am::ReplayMutant::DropContinuation);
      am::need(trial.anchors==golden.anchors && trial.next==golden.next && trial.birth_ball==golden.birth_ball && trial.runs==golden.runs &&
               trial.roots==golden.roots && trial.draft.contribution.size()<golden.draft.contribution.size(),"mutant.dated_growth_only");
      refuses([&]{am::equal_draft(trial.draft,golden.draft);},"replay.level_representation",c);
      ++c.mutants[4];}
  }
}
void check_capture(am::Capture& capture,const FullBallStaticTrace& trace,unsigned kmax,Counts& c,bool reverse) {
  am::need(capture.complete,"gate.capture_complete");
  for(unsigned j=1;j<=kmax;++j) {
    const unsigned k=reverse?kmax+1-j:j;const auto& slot=capture.slots[k];auto manifest=am::make_manifest(capture,k);
    if(k>=2) am::need(slot.input.targets==trace.targets[k] && trace.firsts[k].size()==slot.input.targets.size(),"gate.phase0_trace");
    am::validate_binding(manifest,slot);const auto result=am::replay_a(manifest);am::compare_output(result,slot.output);
    if(!reverse) adversaries(manifest,slot,result,c);
    ++c.orders;c.blocks+=manifest.blocks.size();c.targets+=manifest.target.size();c.actions+=result.draft.actions();
    c.contributions+=result.draft.contribution.size();c.inert+=result.stats.inert_blocks;c.extra+=result.stats.extra_blocks;c.raw_roots+=result.roots.size();
    c.parent_incidence+=result.draft.parent.size();
    for(size_t begin=0;begin<manifest.blocks.size();) {
      size_t end=begin+1;while(end<manifest.blocks.size() && manifest.blocks[end].run==manifest.blocks[begin].run) ++end;
      std::set<u64> plateau;
      for(size_t b=begin;b<end;++b) {
        std::set<u64> block;
        for(u64 j=result.root_begin[b];j<result.root_begin[b+1];++j) {
          const auto root=result.roots[j];c.duplicate_roots+=!block.insert(root).second;
        }
        for(auto root:block) c.shared_plateau_roots+=!plateau.insert(root).second;
      }
      begin=end;
    }
    for(size_t b=1;b<manifest.blocks.size();++b) c.global_run_gaps+=manifest.blocks[b].run>manifest.blocks[b-1].run+1;
    for(size_t a=0;a<result.draft.actions();++a) {const auto parents=result.draft.parent_begin[a+1]-result.draft.parent_begin[a];
      c.max_parents=std::max(c.max_parents,parents);c.continuations+=parents==1;}
    if(k==1) for(size_t d=0;d<manifest.domain.size();++d) c.k1_nonidentity+=manifest.domain[d]!=d;
  }
  c.reverse_slot_passes+=reverse;
}
void gate(bool full) {
  Counts c;
  for(const auto& fixture:am::fixtures::all()) {
    if(!full && std::string_view(fixture.name)=="ico12") continue;
    for(bool variant:{false,true}) {
      const auto input=am::fixtures::input(fixture,variant);const auto index=build_cloud_index(input);
      auto balls=am::fixtures::catalogue(fixture,index);if(variant) am::fixtures::distinct_representations(balls);++c.fixtures;
      // True native mono-thread reference: not observed (it never enters lean A).
      const auto mono=build_full_ball_tower(index,balls,fixture.k,1);
      am::need(mono.status==FullBallStatus::kCompleteRelative,mono.reason);
      for(bool hashed:{false,true}) {
        FullBallStaticTrace trace;FullBallTowerOptions options{.overlap_static=true,.hash_grouping=hashed,.trace=&trace};
        am::Capture capture;FullBallTowerResult hooked;
        {am::CaptureScope scope(capture);hooked=observed(index,balls,fixture.k,4,options);}
        for(unsigned k=1;k<=hooked.orders.size();++k) am::need(capture.slots[k].starts==1 && capture.slots[k].finishes==1,"gate.hook_completion");
        capture.complete=true;const auto captured_trace=trace;const auto native=build_full_ball_tower(index,balls,fixture.k,4,{},true,options);
        am::need(trace.targets==captured_trace.targets && trace.firsts==captured_trace.firsts,"passthrough.phase0_native");
        const auto pass=observed(index,balls,fixture.k,4,options);
        am::need(trace.targets==captured_trace.targets && trace.firsts==captured_trace.firsts,"passthrough.phase0_observed");
        same_tower(native,hooked);same_tower(pass,hooked);same_tower(mono,hooked);
        check_capture(capture,captured_trace,static_cast<unsigned>(hooked.orders.size()),c,false);
        check_capture(capture,captured_trace,static_cast<unsigned>(hooked.orders.size()),c,true);++c.captures;
        if(!c.capture_reuse_refused) {
          refuses([&]{am::CaptureScope scope(capture);static_cast<void>(observed(index,balls,fixture.k,4,options));},
                  "capture.failed_or_repeated_input",c);
          refuses([&]{static_cast<void>(am::make_manifest(capture,1));},"manifest.capture_not_admitted",c);
          ++c.capture_reuse_refused;
        }
        if(!c.late_failure_refused) {
          // Unpipelined test path finishes every A before the native B fault.
          FullBallTowerOptions late_options{.overlap_static=false};am::Capture late;
          struct Reset {~Reset(){full_ball_detail::failpoint_populations.store(0);}} reset;
          full_ball_detail::failpoint_populations.store(2);
          const auto failed=build_full_ball_tower(index,balls,fixture.k,4,{},true,late_options);
          am::need(failed.status==FullBallStatus::kInvariantViolated &&
                   std::string(failed.reason)=="failpoint_populations_k2","gate.native_late_failure");
          bool caught=false;
          try {am::CaptureScope scope(late);static_cast<void>(observed(index,balls,fixture.k,4,late_options));}
          catch(const full_ball_detail::Failure& f) {
            am::need(f.status==failed.status && std::string(f.reason)==failed.reason,"gate.late_failure_reason");caught=true;
          }
          am::need(caught && !late.complete,"gate.global_admission");
          for(unsigned k=1;k<=fixture.k;++k) am::need(late.slots[k].starts==1 && late.slots[k].finishes==1,"gate.late_complete_slots");
          refuses([&]{static_cast<void>(am::make_manifest(late,1));},"manifest.capture_not_admitted",c);
          ++c.late_failure_refused;
        }
      }
    }
  }
  am::need(c.inert && c.continuations && c.extra && c.global_run_gaps && c.k1_nonidentity && c.raw_roots &&
           c.duplicate_roots && c.shared_plateau_roots && c.late_failure_refused,"gate.coverage");
  for(size_t j=0;j<c.mutants.size();++j) if(!c.mutants[j]) throw std::runtime_error("gate.mutant_not_exercised."+std::to_string(j));
  if(full) am::need(c.max_parents>=32,"gate.multifusion32");
  std::cout<<"{\"schema\":\"mhgp9_full_a_manifest_v1\",\"status\":\"passed\",\"GCP_used\":false,\"full_gate\":"<<(full?"true":"false")
    <<",\"fixtures\":"<<c.fixtures<<",\"captures\":"<<c.captures<<",\"order_replays\":"<<c.orders<<",\"blocks\":"<<c.blocks
    <<",\"targets\":"<<c.targets<<",\"actions\":"<<c.actions<<",\"contributions\":"<<c.contributions<<",\"inert\":"<<c.inert
    <<",\"continuations\":"<<c.continuations<<",\"extra_blocks\":"<<c.extra<<",\"global_run_gaps\":"<<c.global_run_gaps
    <<",\"max_parents\":"<<c.max_parents<<",\"k1_nonidentity\":"<<c.k1_nonidentity<<",\"raw_roots\":"<<c.raw_roots
    <<",\"reverse_slot_passes\":"<<c.reverse_slot_passes<<",\"refusals\":"<<c.refusals
    <<",\"capture_reuse_refused\":"<<c.capture_reuse_refused<<",\"late_failure_refused\":"<<c.late_failure_refused
    <<",\"duplicate_roots\":"<<c.duplicate_roots<<",\"shared_plateau_roots\":"<<c.shared_plateau_roots
    <<",\"parent_incidence\":"<<c.parent_incidence<<",\"mutants\":[";
  for(size_t j=0;j<c.mutants.size();++j) {if(j) std::cout<<',';std::cout<<c.mutants[j];}std::cout<<"]}\n";
}
}
int main(int argc,char** argv) {
  try {am::need(argc==2 && (std::string_view(argv[1])=="--preflight" || std::string_view(argv[1])=="--gate"),"gate.usage");
    gate(std::string_view(argv[1])=="--gate");return 0;}
  catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
