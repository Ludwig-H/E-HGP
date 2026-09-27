// Corpus/capture orchestration explicitly ported from events/probe.cpp
// SHA256 133756d8b9a64016774ff5c8378d3958f3c8bef5067f35bb2dff3ac5805c53f2.
#include "../b_full_a_manifest_20260927/native_a.hpp"
#include "../b_full_a_manifest_20260927/fixtures.hpp"
#include "../b_full_a_events_20260927/events.hpp"
#include "min_label.hpp"
#include <iostream>
#include <string_view>

namespace am=mhgp9::audit::a_manifest;
namespace ae=mhgp9::audit::a_events;
namespace ml=mhgp9::audit::a_min_label;
using namespace mhgp9::tower;
namespace {
struct Counts {
  u64 fixtures{},captures{},candidate_replays{},abstract_replays{},boundary_replays{},field_comparisons{};
  u64 vertices{},edges{},groups{},nodes{},history_entries{},omitted_history_groups{},silent_groups{},continuations{},max_parents{};
  u64 parent_event{},parent_draft{},parent_forest{},ancestor_entries{},loss_entries{},queries{},steps{},capacity{};
  u64 cut_graphs{},cut_queries{},monotonic_vertices{},query_refusals{},mutations{};
};
template<class F> void refusal(F fn,const char* reason,Counts& c) {
  try {fn();} catch(const std::runtime_error& e) {am::need(std::string(e.what())==reason,"gate.refusal_reason");++c.query_refusals;return;}
  throw std::runtime_error("gate.refusal_missing");
}

// Independent graph BFS for each cut, not the virtual minimum forest and
// not an ancestor algorithm. Tiny audit inputs only, never product code.
void all_cuts(const am::Manifest& m,Counts& c) {
  const u64 initial=m.k==1?m.domain.size():0,n=initial+m.blocks.size();
  struct Edge {u64 vertex;u32 run;};std::vector<std::vector<Edge>> adjacency(n);
  std::vector<u32> cuts{0,am::absent32-1};
  for(u64 b=0;b<m.blocks.size();++b) {
    const u64 source=initial+b;const u32 run=m.blocks[b].run;
    cuts.push_back(run);cuts.push_back(run-1);
    if(run<am::absent32-1) cuts.push_back(run+1);
    for(u64 j=m.representative_begin[b];j<m.representative_begin[b+1];++j) {
      const u64 target=m.k==1?m.target[j]:initial+m.target[j];
      adjacency[source].push_back({target,m.blocks[b].run});adjacency[target].push_back({source,m.blocks[b].run});
    }
  }
  ml::Work wa,wb;ml::Times ta,tb;auto a=ml::CutIndex::prepare(m,wa,ta),b=ml::CutIndex::prepare(m,wb,tb,true);
  for(u64 v=0;v<n;++v) for(const auto* index:{&a,&b}) {
    am::need(index->parent(v)<=v && (index->parent(v)==v?index->loss(v)==am::absent32:
             index->loss(v)<am::absent32 && index->loss(v)<=index->loss(index->parent(v))),"gate.loss_monotonic");
    ++c.monotonic_vertices;
  }
  // All possible edge-admission states, including boundary values, without
  // an iteration proportional to gaps in the global run catalogue.
  std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
  for(u32 cut:cuts) for(bool closed:{false,true}) {
    std::vector<bool> seen(n);std::vector<u64> label(n),component;
    for(u64 start=0;start<n;++start) if(!seen[start]) {
      component.clear();component.push_back(start);seen[start]=true;u64 minimum=start;
      for(size_t j=0;j<component.size();++j) {
        const auto v=component[j];minimum=std::min(minimum,v);
        for(const auto edge:adjacency[v]) if(!seen[edge.vertex] && (edge.run<cut || (closed && edge.run==cut))) {
          seen[edge.vertex]=true;component.push_back(edge.vertex);
        }
      }
      for(u64 v:component) label[v]=minimum;
    }
    for(u64 v=0;v<n;++v) {
      am::need(a.label(v,cut,closed,wa)==label[v] && b.label(v,cut,closed,wb)==label[v],"gate.cut_minimum");
      c.cut_queries+=2;
    }
  }
  refusal([&]{static_cast<void>(a.label(n,0,true,wa));},"minimum.query_domain",c);
  refusal([&]{static_cast<void>(a.label(0,am::absent32,false,wa));},"minimum.query_domain",c);
  auto malformed=m;malformed.representative_begin.back()=am::absent;
  refusal([&]{static_cast<void>(ml::CutIndex::prepare(malformed,wa,ta));},"manifest.csr_shape",c);++c.cut_graphs;
}

void check(const am::Manifest& m,const am::Output* native,Counts& c,bool count_geometry) {
  const auto chronological=am::replay_a(m);
  const auto old=ae::build(m,true);am::compare_output(old.output,chronological);++c.field_comparisons;
  if(native) {am::compare_output(chronological,*native);++c.field_comparisons;}
  all_cuts(m,c);
  for(bool reverse:{false,true}) {
    const auto result=ml::build(m,reverse);const auto& w=result.work;
    am::compare_output(result.output,chronological);am::compare_output(result.output,old.output);c.field_comparisons+=2;
    if(native) {am::compare_output(result.output,*native);++c.field_comparisons;}
    am::need(w.vertices==(m.k==1?m.domain.size():0)+m.blocks.size() && w.edges==m.target.size() &&
             w.forest_edges+w.components==w.vertices && w.groups<=w.vertices &&
             w.history_entries==result.output.next.size() && w.omitted_history_groups+w.history_entries==w.groups &&
             w.ancestor_entries==w.vertices*std::bit_width(w.vertices) && w.loss_entries==w.vertices &&
             w.ancestor_queries==w.vertices+w.edges &&
             w.ancestor_steps==w.ancestor_queries*(std::bit_width(w.vertices)+1) &&
             w.parent_forest_incidences<=w.parent_draft_incidences && w.parent_draft_incidences<=w.parent_event_incidences &&
             w.parent_event_incidences<=w.edges && w.output_capacity<=w.combined_capacity_observed_max &&
             w.temporary_capacity_observed_max<=w.combined_capacity_observed_max,"gate.work_identity");
    if(!count_geometry) continue;
    ++c.candidate_replays;c.vertices+=w.vertices;c.edges+=w.edges;c.groups+=w.groups;c.nodes+=result.output.next.size();
    c.history_entries+=w.history_entries;c.omitted_history_groups+=w.omitted_history_groups;c.silent_groups+=w.silent_groups;
    c.parent_event+=w.parent_event_incidences;c.parent_draft+=w.parent_draft_incidences;c.parent_forest+=w.parent_forest_incidences;
    c.ancestor_entries+=w.ancestor_entries;c.loss_entries+=w.loss_entries;c.queries+=w.ancestor_queries;c.steps+=w.ancestor_steps;
    c.capacity=std::max(c.capacity,w.combined_capacity_observed_max);
    for(size_t a=0;a<result.output.draft.actions();++a) {
      const u64 parents=result.output.draft.parent_begin[a+1]-result.output.draft.parent_begin[a];
      c.max_parents=std::max(c.max_parents,parents);c.continuations+=parents==1;
    }
  }
}

struct Row {u32 run;std::vector<u64> targets;bool contribution;};
am::Manifest program(unsigned k,size_t sites,const std::vector<Row>& rows) {
  am::Manifest m;m.k=k;m.ball_count=rows.size();
  for(size_t i=0;i<sites;++i) m.domain.push_back(static_cast<PointId>(17+3*i));
  u32 first=0;
  for(size_t b=0;b<rows.size();++b) {
    const auto& r=rows[b];if(b==0 || r.run!=rows[b-1].run) first=static_cast<u32>(b);
    const u64 scale=b+2;
    m.blocks.push_back({static_cast<u32>(b),r.run,first,b,{{u64(r.run)*scale,0,0},static_cast<i128>(scale)},
                        static_cast<u16>(r.contribution),2,false,true});
    m.target.insert(m.target.end(),r.targets.begin(),r.targets.end());m.representative_begin.push_back(m.target.size());
  }
  am::validate_manifest(m);return m;
}
am::Manifest random_program(u64 count,u64 seed) {
  // Explicit fixture-generator port from events/probe.cpp, not candidate work.
  const auto random=[&](){seed^=seed<<13;seed^=seed>>7;seed^=seed<<17;return seed;};
  std::vector<Row> rows;u32 run=2;
  for(u64 b=0;b<count;++b) {
    if(b && (b+1==count || random()%3==0)) run+=2;
    std::vector<u64> earlier;for(u64 a=0;a<b;++a) if(rows[a].run<run) earlier.push_back(a);
    const u64 queries=b+1==count?earlier.size():(earlier.empty()?0:random()%5);
    Row row{run,{},queries==0 || random()%3==0};
    for(u64 j=0;j<queries;++j) {const u64 target=b+1==count?earlier[j]:earlier[random()%earlier.size()];
      row.targets.push_back(target);if(random()%7==0) row.targets.push_back(target);}
    rows.push_back(std::move(row));
  }
  return program(2,2,rows);
}

void proposals(Counts& c) {
  const auto chain=program(1,3,{{3,{2,1},false},{10,{1,0},false}});
  const auto equal=program(1,3,{{7,{2,1},false},{7,{1,0},false}});
  const auto side=program(1,3,{{3,{1,2},false},{7,{1,0},false}});
  // R1 rooted at vertex 4: path 2->3->1->4 does not contain component min0,
  // which is the side child of 4. The virtual forest must still return 0.
  for(const auto* m:{&chain,&equal,&side}) {check(*m,nullptr,c,false);c.abstract_replays+=2;}
  ml::Work w;ml::Times t;auto ix=ml::CutIndex::prepare(chain,w,t);
  am::need(ix.parent(2)==1 && ix.loss(2)==3 && ix.parent(1)==0 && ix.loss(1)==10 &&
           ix.label(2,5,true,w)==1,"gate.chain_losses");
  am::need(ix.label(2,5,true,w,ml::Mutant::SkipFinalStep)==2,"gate.no_final_step_mutation");++c.mutations;
  am::need(ix.label(2,5,true,w,ml::Mutant::StartingLoss)==0,"gate.starting_loss_mutation");++c.mutations;
  auto ex=ml::CutIndex::prepare(equal,w,t);am::need(ex.label(2,7,true,w)==0 && ex.label(2,7,false,w)==2,"gate.equal_plateau");
  auto sx=ml::CutIndex::prepare(side,w,t);am::need(sx.label(2,7,true,w)==0,"gate.side_minimum");
  std::vector<Row> rows{{1,{},true}};
  for(u32 b=1;b<=64;++b) rows.push_back({b+1,{b-1},b==32});
  const auto inert=program(2,2,rows);check(inert,nullptr,c,false);c.abstract_replays+=2;
  const auto ir=ml::build(inert);am::need(ir.work.history_entries==1 && ir.work.omitted_history_groups==64 &&
                                       ir.work.silent_groups==63 && ir.output.draft.actions()==2,"gate.long_inertia_continuation");
  const auto merge=program(2,2,{{1,{},true},{1,{},true},{2,{0,1},false}});
  check(merge,nullptr,c,false);c.abstract_replays+=2;
  const auto mr=ml::build(merge);am::need(mr.work.history_entries==3 && mr.output.draft.contribution.size()==2,"gate.empty_contribution_merge");
  refusal([&]{static_cast<void>(ml::build(merge,false,ml::Mutant::DropContributionFreeCreators));},"minimum.creator_history_matches",c);++c.mutations;
  am::Manifest singleton;singleton.k=1;singleton.domain={std::numeric_limits<PointId>::max()};
  check(singleton,nullptr,c,false);c.boundary_replays+=2;
  const auto last=program(1,3,{{3,{2,1},false},{am::absent32-1,{1,0},false}});
  check(last,nullptr,c,false);c.boundary_replays+=2;
  auto lx=ml::CutIndex::prepare(last,w,t);
  am::need(lx.label(2,am::absent32-1,true,w)==0 && lx.label(2,am::absent32-1,false,w)==1,"gate.last_finite_run");
  for(u64 count:{u64{8},u64{32},u64{96},u64{257}}) for(u64 seed=1;seed<=8;++seed) {
    const auto m=random_program(count,seed*927+count);check(m,nullptr,c,false);c.abstract_replays+=2;
  }
}
}

int main(int argc,char** argv) {
  try {
    am::need(argc==2 && (std::string_view(argv[1])=="--gate" || std::string_view(argv[1])=="--preflight"),"minimum.usage");
    const bool full=std::string_view(argv[1])=="--gate";Counts c;
    for(const auto& fixture:am::fixtures::all()) {
      if(!full && std::string_view(fixture.name)=="ico12") continue;
      for(bool variant:{false,true}) {
        const auto input=am::fixtures::input(fixture,variant);const auto index=build_cloud_index(input);
        auto balls=am::fixtures::catalogue(fixture,index);if(variant) am::fixtures::distinct_representations(balls);++c.fixtures;
        for(bool hashed:{false,true}) {
          am::Capture capture;FullBallStats stats;FullBallTimes times;
          FullBallTowerOptions options{.overlap_static=true,.hash_grouping=hashed};size_t orders=0;
          {am::CaptureScope scope(capture);
            orders=full_ball_detail::AObservedBuilder(index,balls,fixture.k,stats,4,{},true,&times,options).run().size();}
          for(unsigned k=1;k<=orders;++k) am::need(capture.slots[k].starts==1 && capture.slots[k].finishes==1,"gate.capture_complete");
          capture.complete=true;++c.captures;
          for(unsigned k=1;k<=orders;++k) {
            const auto m=am::make_manifest(capture,k);am::validate_binding(m,capture.slots[k]);check(m,&capture.slots[k].output,c,true);
          }
        }
      }
    }
    proposals(c);
    am::need(c.silent_groups && c.continuations && c.omitted_history_groups && c.mutations==3 && (!full || c.max_parents>=32),"gate.coverage");
    std::cout<<"{\"schema\":\"mhgp9_full_a_min_label_v1\",\"status\":\"passed\",\"full_gate\":"<<(full?"true":"false")
      <<",\"fixtures\":"<<c.fixtures<<",\"captures\":"<<c.captures<<",\"candidate_replays\":"<<c.candidate_replays
      <<",\"abstract_replays\":"<<c.abstract_replays<<",\"boundary_replays\":"<<c.boundary_replays<<",\"field_comparisons\":"<<c.field_comparisons
      <<",\"vertices\":"<<c.vertices<<",\"edges\":"<<c.edges<<",\"groups\":"<<c.groups<<",\"nodes\":"<<c.nodes
      <<",\"history_entries\":"<<c.history_entries<<",\"omitted_history_groups\":"<<c.omitted_history_groups
      <<",\"silent_groups\":"<<c.silent_groups<<",\"continuations\":"<<c.continuations<<",\"max_parents\":"<<c.max_parents
      <<",\"parent_event_incidences\":"<<c.parent_event<<",\"parent_draft_incidences\":"<<c.parent_draft<<",\"parent_forest_incidences\":"<<c.parent_forest
      <<",\"ancestor_entries\":"<<c.ancestor_entries<<",\"loss_entries\":"<<c.loss_entries
      <<",\"ancestor_queries\":"<<c.queries<<",\"ancestor_steps\":"<<c.steps<<",\"max_combined_capacity_observed_bytes\":"<<c.capacity
      <<",\"cut_graphs\":"<<c.cut_graphs<<",\"cut_queries\":"<<c.cut_queries<<",\"monotonic_vertices\":"<<c.monotonic_vertices
      <<",\"query_refusals\":"<<c.query_refusals<<",\"mutations\":"<<c.mutations
      <<",\"parallel\":false,\"FULL_executed_by_candidate\":false,\"GCP_used\":false}\n";
    return 0;
  } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 2;}
}
