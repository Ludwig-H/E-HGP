#pragma once
// Explicit isolated port of b_full_a_events_20260927/events.hpp, SHA256
// 8c1280e9082d20e69cc1c47240160c336cb54f30d88594d34722db944d6de94f.
// Grouping/output contract retained; virtual minimum forest, one up table,
// and creator-only histories implement NEXT_MIN_LABEL.md (27 September).
// Sequential prototype only. No geometric producer/resolver is copied.
#include "../b_full_a_manifest_20260927/manifest.hpp"
#include <bit>

namespace mhgp9::audit::a_min_label {
namespace am = a_manifest;
using namespace mhgp9::tower;
inline void require(bool ok,const char* why) {am::need(ok,why);}
inline u64 plus(u64 a,u64 b) {return am::sum(a,b);}
inline u64 product(u64 a,u64 b) {require(!a || b<=am::absent/a,"minimum.size_overflow");return a*b;}
template<class T> u64 bytes(const std::vector<T>& v) {return product(v.capacity(),sizeof(T));}
template<class T> void release(std::vector<T>& v) {std::vector<T>().swap(v);}
enum class Mutant {None, SkipFinalStep, StartingLoss, DropContributionFreeCreators};
struct Work {
  u64 vertices{},edges{},forest_edges{},components{},groups{},contributions{};
  u64 parent_event_incidences{},parent_draft_incidences{},parent_forest_incidences{};
  u64 dsu_steps{},ancestor_entries{},loss_entries{},ancestor_queries{},ancestor_steps{};
  u64 history_entries{},omitted_history_groups{},predecessor_queries{},predecessor_steps{},silent_groups{};
  u64 temporary_capacity_observed_max{},combined_capacity_observed_max{},output_capacity{};
};
struct Times {double validate_ms{},forest_ms{},ancestors_ms{},groups_ms{},parents_ms{},histories_ms{},output_ms{},total_ms{};};
struct Result {am::Output output;Work work;Times times;};

// No factory from arbitrary parent/loss arrays. The only constructor consumes
// a validated chronological manifest. The returned tables are private/owned.
class CutIndex final {
 public:
  static CutIndex prepare(const am::Manifest& m,Work& work,Times& timing,bool reverse_equal=false) {
    const auto validated=am::Clock::now();work=Work{};timing=Times{};am::validate_manifest(m);
    const u64 initial=m.k==1?m.domain.size():0,vertices=plus(initial,m.blocks.size());
    require(vertices && vertices<=std::numeric_limits<size_t>::max(),"minimum.vertex_domain");
    work.vertices=vertices;work.edges=m.target.size();timing.validate_ms=am::milliseconds(validated);
    const auto forest_start=am::Clock::now();CutIndex out;
    out.vertices_=vertices;out.powers_=static_cast<unsigned>(std::bit_width(vertices));
    out.up_.resize(vertices);out.loss_.assign(vertices,am::absent32);
    std::vector<u64> dsu(vertices),minimum(vertices);std::vector<u8> ranks(vertices);
    std::iota(dsu.begin(),dsu.end(),u64{0});std::iota(minimum.begin(),minimum.end(),u64{0});
    std::iota(out.up_.begin(),out.up_.end(),u64{0});
    const auto find=[&](u64 v) {while(dsu[v]!=v) {++work.dsu_steps;dsu[v]=dsu[dsu[v]];v=dsu[v];}return v;};
    const auto occurrence=[&](u64 b,u64 j) {
      const u64 source=plus(initial,b),target=m.k==1?m.target[j]:plus(initial,m.target[j]);
      u64 a=find(source),c=find(target);if(a==c) return;
      const u64 low=std::min(minimum[a],minimum[c]),high=std::max(minimum[a],minimum[c]);
      require(out.up_[high]==high && out.loss_[high]==am::absent32,"minimum.parent_written_twice");
      out.up_[high]=low;out.loss_[high]=m.blocks[b].run;++work.forest_edges;
      if(ranks[a]<ranks[c]) std::swap(a,c);
      dsu[c]=a;minimum[a]=low;
      if(ranks[a]==ranks[c]) {require(ranks[a]<255,"minimum.dsu_rank");++ranks[a];}
    };
    // Optional gate schedule reverses both blocks and their occurrences inside
    // one equal-rank plateau, never the order of distinct ranks.
    for(u64 begin=0;begin<m.blocks.size();) {
      u64 end=begin+1;while(end<m.blocks.size() && m.blocks[end].run==m.blocks[begin].run) ++end;
      for(u64 step=0;step<end-begin;++step) {
        const u64 b=reverse_equal?end-1-step:begin+step;
        const u64 first=m.representative_begin[b],last=m.representative_begin[b+1];
        for(u64 j=0;j<last-first;++j) occurrence(b,reverse_equal?last-1-j:first+j);
      }
      begin=end;
    }
    for(u64 v=0;v<vertices;++v) {
      const u64 parent=out.up_[v];
      require(parent<=v,"minimum.parent_order");
      if(parent==v) require(out.loss_[v]==am::absent32,"minimum.root_loss");
      else require(out.loss_[v]<am::absent32 && out.loss_[v]<=out.loss_[parent],"minimum.loss_monotonic");
    }
    work.components=vertices-work.forest_edges;
    const u64 forest_capacity=plus(out.capacity_bytes(),plus(plus(bytes(dsu),bytes(minimum)),bytes(ranks)));
    work.temporary_capacity_observed_max=forest_capacity;work.combined_capacity_observed_max=forest_capacity;
    release(dsu);release(minimum);release(ranks);timing.forest_ms=am::milliseconds(forest_start);

    const auto ancestors_start=am::Clock::now();const u64 entries=product(vertices,out.powers_);
    require(entries<=out.up_.max_size(),"minimum.ancestor_size");out.up_.resize(entries);
    for(unsigned p=1;p<out.powers_;++p) for(u64 v=0;v<vertices;++v) {
      const u64 old=product(p-1,vertices),next=product(p,vertices);
      out.up_[next+v]=out.up_[old+out.up_[old+v]];
    }
    work.ancestor_entries=entries;work.loss_entries=vertices;
    work.temporary_capacity_observed_max=std::max(work.temporary_capacity_observed_max,out.capacity_bytes());
    work.combined_capacity_observed_max=std::max(work.combined_capacity_observed_max,out.capacity_bytes());
    timing.ancestors_ms=am::milliseconds(ancestors_start);return out;
  }
  u64 label(u64 v,u32 cut,bool closed,Work& work,Mutant mutant=Mutant::None) const {
    require(v<vertices_ && cut<am::absent32,"minimum.query_domain");++work.ancestor_queries;
    const auto allowed=[&](u32 loss) {return loss<cut || (closed && loss==cut);};
    for(unsigned p=powers_;p>0;--p) {
      ++work.ancestor_steps;const u64 ancestor=up_[product(p-1,vertices_)+v];
      if(allowed(loss_[mutant==Mutant::StartingLoss?v:ancestor])) v=ancestor;
    }
    ++work.ancestor_steps;
    if(mutant!=Mutant::SkipFinalStep && allowed(loss_[v])) v=up_[v];
    return v;
  }
  u64 capacity_bytes() const {return plus(bytes(up_),bytes(loss_));}
  u64 vertices() const {return vertices_;}
  u64 parent(u64 v) const {require(v<vertices_,"minimum.query_domain");return up_[v];}
  u32 loss(u64 v) const {require(v<vertices_,"minimum.query_domain");return loss_[v];}
  void clear() {release(up_);release(loss_);vertices_=0;powers_=0;}
 private:
  CutIndex()=default;
  u64 vertices_{};unsigned powers_{};std::vector<u64> up_;std::vector<u32> loss_;
};
struct Group {u32 run{};u64 label{},begin{},end{},minimum{};};

inline Result build(const am::Manifest& m,bool reverse_equal=false,Mutant mutant=Mutant::None) {
  const auto started=am::Clock::now();Result result;auto& w=result.work;auto& timing=result.times;auto& out=result.output;
  auto index=CutIndex::prepare(m,w,timing,reverse_equal);
  const u64 initial=m.k==1?m.domain.size():0,vertices=w.vertices;
  std::vector<u64> closed_label,members,occurrence_label,parent_begin,parents,node_ids;
  std::vector<u64> history_begin,histories,cursor,local_parents,local_nodes;
  std::vector<FullCoverageRef> local_refs;std::vector<Group> groups;
  const auto output_bytes=[&] {
    u64 total=0;const auto take=[&](const auto& v){total=plus(total,bytes(v));};
    take(out.anchors);take(out.next);take(out.runs);take(out.birth_ball);take(out.root_begin);take(out.roots);
    take(out.draft.level);take(out.draft.batch_begin);take(out.draft.parent_begin);take(out.draft.parent);
    take(out.draft.contribution_begin);take(out.draft.contribution);return total;
  };
  // Result already existed during prepare(): its four {0} CSR vectors
  // coexist with every factory phase, including the temporary DSU peak.
  w.combined_capacity_observed_max=plus(w.combined_capacity_observed_max,output_bytes());
  const auto observe=[&] {
    u64 total=index.capacity_bytes();const auto take=[&](const auto& v){total=plus(total,bytes(v));};
    take(closed_label);take(members);take(occurrence_label);take(parent_begin);take(parents);take(node_ids);
    take(history_begin);take(histories);take(cursor);take(local_parents);take(local_nodes);take(local_refs);take(groups);
    w.temporary_capacity_observed_max=std::max(w.temporary_capacity_observed_max,total);
    w.combined_capacity_observed_max=std::max(w.combined_capacity_observed_max,plus(total,output_bytes()));
  };
  const auto run_of=[&](u64 v)->u32 {return v<initial?0:m.blocks[v-initial].run;};
  const auto target_vertex=[&](u64 j){return m.k==1?m.target[j]:plus(initial,m.target[j]);};
  const auto groups_start=am::Clock::now();closed_label.resize(vertices);members.resize(vertices);
  for(u64 v=0;v<vertices;++v) {closed_label[v]=index.label(v,run_of(v),true,w,mutant);members[v]=v;}
  std::sort(members.begin(),members.end(),[&](u64 a,u64 b){
    return std::tuple(run_of(a),closed_label[a],a)<std::tuple(run_of(b),closed_label[b],b);
  });
  for(u64 begin=0;begin<vertices;) {
    u64 end=begin+1;const u64 v=members[begin];
    while(end<vertices && run_of(members[end])==run_of(v) && closed_label[members[end]]==closed_label[v]) ++end;
    groups.push_back({run_of(v),closed_label[v],begin,end,v});begin=end;
  }
  std::sort(groups.begin(),groups.end(),[](const Group& a,const Group& b){return std::pair(a.run,a.minimum)<std::pair(b.run,b.minimum);});
  w.groups=groups.size();observe();release(closed_label);timing.groups_ms=am::milliseconds(groups_start);

  const auto parents_start=am::Clock::now();occurrence_label.resize(m.target.size());parent_begin.push_back(0);
  node_ids.assign(groups.size(),am::absent);u64 nodes=0;
  for(u64 g=0;g<groups.size();++g) {
    const auto& group=groups[g];local_parents.clear();
    for(u64 i=group.begin;i<group.end;++i) {
      const u64 v=members[i];if(v<initial) continue;const u64 b=v-initial;
      for(u64 j=m.representative_begin[b];j<m.representative_begin[b+1];++j) {
        const u64 label=index.label(target_vertex(j),group.run,false,w,mutant);
        occurrence_label[j]=label;local_parents.push_back(label);
      }
    }
    std::sort(local_parents.begin(),local_parents.end());
    const auto unique=std::unique(local_parents.begin(),local_parents.end());
    parents.insert(parents.end(),local_parents.begin(),unique);parent_begin.push_back(parents.size());
    if(parent_begin[g+1]-parent_begin[g]!=1) node_ids[g]=nodes++;
    observe();
  }
  w.parent_event_incidences=parents.size();w.omitted_history_groups=groups.size()-nodes;
  observe();index.clear();release(local_parents);timing.parents_ms=am::milliseconds(parents_start);

  const auto histories_start=am::Clock::now();history_begin.assign(plus(vertices,1),0);
  const auto keep_history=[&](u64 g) {
    if(node_ids[g]==am::absent) return false;
    if(mutant!=Mutant::DropContributionFreeCreators) return true;
    for(u64 i=groups[g].begin;i<groups[g].end;++i) {
      const u64 v=members[i];if(v<initial) return true;
      if(m.blocks[v-initial].shell_mask || m.blocks[v-initial].include_interior) return true;
    }
    return false;
  };
  for(u64 g=0;g<groups.size();++g) if(keep_history(g)) ++history_begin[groups[g].label+1];
  for(u64 v=0;v<vertices;++v) history_begin[v+1]=plus(history_begin[v+1],history_begin[v]);
  histories.resize(history_begin.back());cursor=history_begin;
  for(u64 g=0;g<groups.size();++g) if(keep_history(g)) histories[cursor[groups[g].label]++]=g;
  w.history_entries=histories.size();observe();release(cursor);
  const auto predecessor=[&](u64 component,u32 cut,bool closed) {
    ++w.predecessor_queries;u64 low=history_begin[component],high=history_begin[component+1],first=low;
    while(low<high) {
      ++w.predecessor_steps;const u64 mid=low+(high-low)/2;const u32 run=groups[histories[mid]].run;
      if(run<cut || (closed && run==cut)) low=mid+1;else high=mid;
    }
    require(low>first,"minimum.predecessor_missing");const u64 event=histories[low-1];
    require(node_ids[event]!=am::absent,"minimum.history_creator");return node_ids[event];
  };
  require(nodes<am::absent32,"minimum.node_domain");
  out.root_begin=m.representative_begin;out.roots.resize(m.target.size());
  for(u64 b=0;b<m.blocks.size();++b) for(u64 j=m.representative_begin[b];j<m.representative_begin[b+1];++j)
    out.roots[j]=predecessor(occurrence_label[j],m.blocks[b].run,false);
  for(u64 g=0;g<groups.size();++g) {
    const auto& group=groups[g];const u64 owner=predecessor(group.label,group.run,true);
    if(node_ids[g]!=am::absent) require(owner==node_ids[g],"minimum.creator_history_matches");
    node_ids[g]=owner;
    for(u64 j=parent_begin[g];j<parent_begin[g+1];++j) parents[j]=predecessor(parents[j],group.run,false);
  }
  observe();release(history_begin);release(histories);release(occurrence_label);timing.histories_ms=am::milliseconds(histories_start);

  const auto output_start=am::Clock::now();out.anchors.assign(m.ball_count,am::absent32);out.next.assign(nodes,am::absent);
  out.runs.resize(nodes);out.birth_ball.assign(nodes,am::absent32);
  // Native lot statistics exclude K1's initial point events.
  for(size_t begin=0;begin<m.blocks.size();) {
    size_t end=begin+1;while(end<m.blocks.size() && m.blocks[end].run==m.blocks[begin].run) ++end;
    if(end-begin==1) ++out.stats.singleton_lots;else {++out.stats.grouped_lots;out.stats.lot_dsu_slots+=end-begin;}
    begin=end;
  }
  out.stats.anchor_blocks=m.blocks.size();out.stats.representatives=m.target.size();
  for(const auto& block:m.blocks) {if(block.regular) ++out.stats.regular_blocks;else ++out.stats.extra_blocks;}
  u32 emitted_run=am::absent32;size_t plateau_begin=0;
  for(u64 g=0;g<groups.size();++g) {
    const auto& group=groups[g];const u64 owner=node_ids[g];local_nodes.clear();local_refs.clear();
    for(u64 j=parent_begin[g];j<parent_begin[g+1];++j) local_nodes.push_back(parents[j]);
    std::sort(local_nodes.begin(),local_nodes.end());
    require(std::adjacent_find(local_nodes.begin(),local_nodes.end())==local_nodes.end(),"minimum.parent_identity_collision");
    for(u64 i=group.begin;i<group.end;++i) {
      const u64 v=members[i];if(v<initial) {local_refs.push_back({v,1,false});continue;}
      const auto& block=m.blocks[v-initial];out.anchors[block.ball]=static_cast<u32>(owner);
      if(block.shell_mask || block.include_interior) local_refs.push_back({am::ball_tag|block.ball,block.shell_mask,block.include_interior});
    }
    out.stats.contributions+=local_refs.size();
    if(local_nodes.size()!=1) {
      out.runs[owner]=group.run;
      if(local_nodes.empty()) {
        require(group.end-group.begin==1 && local_refs.size()==1,"minimum.distinct_birth");
        const u64 v=members[group.begin];out.birth_ball[owner]=v<initial?am::absent32:m.blocks[v-initial].ball;++out.stats.births;
      } else ++out.stats.merges;
      for(u64 parent:local_nodes) {require(parent<owner && out.next[parent]==am::absent,"minimum.parent_written_twice");out.next[parent]=owner;}
    }
    if(local_nodes.size()==1 && local_refs.empty()) {++w.silent_groups;out.stats.inert_blocks+=group.end-group.begin;continue;}
    if(emitted_run!=group.run) {
      ExactLevel level{{0,0,0},1};
      if(group.run) {
        while(plateau_begin<m.blocks.size() && m.blocks[plateau_begin].run<group.run) ++plateau_begin;
        require(plateau_begin<m.blocks.size() && m.blocks[plateau_begin].run==group.run,"minimum.plateau_level");level=m.blocks[plateau_begin].level;
      }
      out.draft.open_batch(level);emitted_run=group.run;
    }
    out.draft.add_action(local_nodes,local_refs);observe();
  }
  w.contributions=out.stats.contributions;require(std::count(out.next.begin(),out.next.end(),am::absent)==1,"minimum.final_component");
  w.parent_draft_incidences=out.draft.parent.size();
  w.parent_forest_incidences=static_cast<u64>(std::count_if(out.next.begin(),out.next.end(),[](u64 v){return v!=am::absent;}));
  w.output_capacity=output_bytes();observe();
  release(members);release(parent_begin);release(parents);release(node_ids);release(groups);release(local_nodes);release(local_refs);
  timing.output_ms=am::milliseconds(output_start);timing.total_ms=am::milliseconds(started);return result;
}
} // namespace mhgp9::audit::a_min_label
