#pragma once
#include "capture.hpp"
#include <map>
#include <set>
namespace mhgp9::audit::a_manifest {
inline u64 sum(u64 a,u64 b) {need(b<=absent-a,"manifest.overflow");return a+b;}
struct Block {
  u32 ball{},run{},lot_first_ball{};u64 program_ordinal{};ExactLevel level{};
  u16 shell_mask{};u8 shell_size{};bool include_interior{},regular{};
};
struct Manifest {
  unsigned k{};u64 ball_count{};std::vector<PointId> domain;
  std::vector<Block> blocks;std::vector<u64> representative_begin{0},target;
};
inline void csr(const std::vector<u64>& offsets,u64 rows,u64 count) {
  need(rows<absent && offsets.size()==rows+1 && !offsets.empty() && offsets.front()==0 && offsets.back()==count,"manifest.csr_shape");
  for(size_t i=1;i<offsets.size();++i) need(offsets[i-1]<=offsets[i] && offsets[i]<=count,"manifest.csr_order");
}
inline void validate_manifest(const Manifest& m) {
  need(m.k>=1 && m.k<=10 && m.ball_count<absent32,"manifest.domain");
  csr(m.representative_begin,m.blocks.size(),m.target.size());
  need(!m.domain.empty() && std::is_sorted(m.domain.begin(),m.domain.end()) &&
       std::adjacent_find(m.domain.begin(),m.domain.end())==m.domain.end(),"manifest.point_domain");
  std::set<u32> balls;u32 first=absent32;
  // Validate every row before following ANY target indirection.
  for(size_t b=0;b<m.blocks.size();++b) {
    const auto& row=m.blocks[b];need(row.ball<m.ball_count && balls.insert(row.ball).second,"manifest.unique_ball");
    need(row.program_ordinal==b && row.run>0 && row.run<absent32 && row.level.den>0,"manifest.block_identity");
    need(row.shell_size>=2 && row.shell_size<=12 && (row.shell_mask>>row.shell_size)==0,"manifest.contribution_mask");
    if(b) {need(m.blocks[b-1].run<=row.run,"manifest.run_order");
      const int compare=compare_exact_level(m.blocks[b-1].level,row.level);
      need((m.blocks[b-1].run==row.run && compare==0) || (m.blocks[b-1].run<row.run && compare<0),"manifest.run_level");}
    if(b==0 || m.blocks[b-1].run!=row.run) first=row.ball;
    need(row.lot_first_ball==first,"manifest.lot_first_ball");
  }
  for(size_t b=0;b<m.blocks.size();++b) for(u64 j=m.representative_begin[b];j<m.representative_begin[b+1];++j) {
    const auto target=m.target[j];
    if(m.k==1) need(target<m.domain.size(),"manifest.k1_target");
    else {need(target<m.blocks.size(),"manifest.target_domain");need(m.blocks[target].run<m.blocks[b].run,"manifest.target_not_strict");}
  }
}
inline Manifest make_manifest(const Capture& capture,unsigned k) {
  need(capture.complete && k>0 && k<capture.slots.size(),"manifest.capture_not_admitted");
  const auto& slot=capture.slots[k];
  need(slot.starts==1 && slot.finishes==1,"manifest.incomplete_capture");const auto& in=slot.input;
  need(in.k==k,"manifest.slot_order");
  const auto n=in.program.size();need(in.k>=1 && in.k<=10 && in.count.size()==n && in.contribution.size()==n && in.interior.size()==n &&
    in.level.size()==in.level_run.size() && in.level.size()==in.regular.size() && in.level.size()==in.shell_size.size(),"manifest.capture_shape");
  Manifest out;out.k=in.k;out.ball_count=in.level.size();out.domain=in.domain;
  std::map<u32,u64> positions;
  for(size_t p=0;p<n;++p) {need(in.program[p]<in.level.size() && positions.emplace(in.program[p],p).second,"manifest.capture_program");}
  u32 first=absent32;
  for(size_t p=0;p<n;++p) {const auto id=in.program[p];const auto run=in.level_run[id];
    need(run<absent32-1 && in.interior[p]<=1,"manifest.capture_run");
    if(p==0 || in.level_run[in.program[p-1]]!=run) first=id;
    out.blocks.push_back({id,run+1,first,p,in.level[id],in.contribution[p],in.shell_size[id],in.interior[p]!=0,in.regular[id]!=0});
    out.representative_begin.push_back(sum(out.representative_begin.back(),in.count[p]));}
  need(out.representative_begin.back()==in.targets.size(),"manifest.capture_target_count");
  for(u32 target:in.targets) {
    if(in.k==1) out.target.push_back(target);
    else {const auto found=positions.find(target);need(found!=positions.end(),"manifest.target_missing_block");out.target.push_back(found->second);}
  }
  validate_manifest(out);return out;
}
inline void validate_binding(const Manifest& m,const Slot& source) {
  validate_manifest(m);const auto& in=source.input;
  need(m.k==in.k && m.domain==in.domain && m.ball_count==in.level.size(),"binding.domain");
  need(m.blocks.size()==in.program.size(),"binding.block_count");
  for(size_t b=0;b<m.blocks.size();++b) {
    const auto& r=m.blocks[b];need(r.ball==in.program[b],"binding.program");
    need(r.run==u64(in.level_run[r.ball])+1 && r.level==in.level[r.ball],"binding.level");
    need(r.shell_mask==in.contribution[b] && r.include_interior==(in.interior[b]!=0) &&
         r.regular==(in.regular[r.ball]!=0) && r.shell_size==in.shell_size[r.ball],"binding.contribution");
    need(m.representative_begin[b+1]-m.representative_begin[b]==in.count[b],"binding.representative_count");
  }
  need(m.target.size()==in.targets.size(),"binding.target_count");
  for(size_t j=0;j<m.target.size();++j)
    need((m.k==1?m.target[j]:m.blocks[m.target[j]].ball)==in.targets[j],"binding.target");
}
inline bool same_ref(const FullCoverageRef& a,const FullCoverageRef& b) {
  return a.population==b.population && a.shell_mask==b.shell_mask && a.include_interior==b.include_interior;
}
inline void equal_draft(const FullCoverageFlatDraft& a,const FullCoverageFlatDraft& b) {
  need(a.level==b.level,"replay.level_representation");
  need(a.batch_begin==b.batch_begin && a.parent_begin==b.parent_begin && a.parent==b.parent,"replay.actions_parents");
  need(a.contribution_begin==b.contribution_begin && a.contribution.size()==b.contribution.size(),"replay.contribution_shape");
  for(size_t j=0;j<a.contribution.size();++j) need(same_ref(a.contribution[j],b.contribution[j]),"replay.contribution");
}
inline void compare_output(const Output& a,const Output& b) {
  need(a.root_begin==b.root_begin && a.roots==b.roots,"replay.occurrence_roots");
  need(a.anchors==b.anchors,"replay.anchors");need(a.next==b.next,"replay.successors");
  need(a.birth_ball==b.birth_ball && a.runs==b.runs,"replay.births_runs");
  equal_draft(a.draft,b.draft);need(a.stats==b.stats,"replay.semantic_counters");
}
// Independent chronological judge: histories followed without path compression;
// plateau components grouped through a map of parent owners, before mutation.
// This is a bounded audit oracle, not the proposed massive event constructor.
enum class ReplayMutant {None, FirstContributorLevel, DropContinuation};
inline Output replay_a(const Manifest& m,ReplayMutant mutant=ReplayMutant::None) {
  validate_manifest(m);Output out;out.anchors.assign(m.ball_count,absent32);
  const auto new_node=[&](std::span<const u64> parents,u32 birth,u32 run) {
    const auto id=out.next.size();need(id<absent32,"replay.node_domain");out.next.push_back(absent);
    out.birth_ball.push_back(birth);out.runs.push_back(run);
    for(auto parent:parents) {need(parent<id && out.next[parent]==absent,"replay.parent_live");out.next[parent]=id;}
    if(parents.empty()) ++out.stats.births;else ++out.stats.merges;return id;
  };
  if(m.k==1) {
    out.draft.open_batch({{0,0,0},1});
    for(size_t j=0;j<m.domain.size();++j) {const FullCoverageRef ref{j,1,false};out.draft.add_action({},std::span(&ref,1));
      ++out.stats.contributions;new_node({},absent32,0);}
  }
  std::vector<u64> block_anchor(m.blocks.size(),absent);
  for(size_t begin=0;begin<m.blocks.size();) {
    size_t end=begin+1;while(end<m.blocks.size() && m.blocks[end].run==m.blocks[begin].run) ++end;
    const auto count=end-begin,prior=out.next.size();std::vector<std::vector<u64>> roots(count);
    size_t level_position=begin;
    if(mutant==ReplayMutant::FirstContributorLevel)
      for(size_t b=begin;b<end;++b) if(m.blocks[b].shell_mask || m.blocks[b].include_interior) {level_position=b;break;}
    std::vector<size_t> dsu(count);std::iota(dsu.begin(),dsu.end(),0);std::map<u64,size_t> owner;
    const auto find=[&](size_t a) {while(dsu[a]!=a) a=dsu[a];return a;};
    for(size_t b=begin;b<end;++b) {
      const auto& row=m.blocks[b];++out.stats.anchor_blocks;
      if(row.regular) ++out.stats.regular_blocks;else ++out.stats.extra_blocks;
      for(u64 j=m.representative_begin[b];j<m.representative_begin[b+1];++j) {
        u64 root=m.k==1?m.target[j]:block_anchor[m.target[j]];need(root<prior,"replay.target_anchor");
        while(out.next[root]!=absent) {root=out.next[root];need(root<prior,"replay.prior_root");}
        out.roots.push_back(root);roots[b-begin].push_back(root);++out.stats.representatives;
        const auto [it,fresh]=owner.emplace(root,b-begin);if(!fresh) {
          const auto a=find(it->second),c=find(b-begin);if(a!=c) dsu[c]=a;}
      }
      out.root_begin.push_back(out.roots.size());
    }
    std::map<size_t,std::vector<size_t>> components;
    for(size_t b=0;b<count;++b) components[find(b)].push_back(b+begin);
    std::vector<std::vector<size_t>> groups;for(auto& [key,group]:components) {static_cast<void>(key);groups.push_back(std::move(group));}
    std::sort(groups.begin(),groups.end(),[](const auto& a,const auto& b){return a.front()<b.front();});
    if(count==1) ++out.stats.singleton_lots;else {++out.stats.grouped_lots;out.stats.lot_dsu_slots+=count;}
    bool opened=false;
    for(const auto& group:groups) {
      std::set<u64> unique;std::vector<FullCoverageRef> refs;
      for(auto b:group) {unique.insert(roots[b-begin].begin(),roots[b-begin].end());const auto& row=m.blocks[b];
        if(row.shell_mask || row.include_interior) {refs.push_back({ball_tag|row.ball,row.shell_mask,row.include_interior});++out.stats.contributions;}}
      const std::vector<u64> parents(unique.begin(),unique.end());u64 target;
      if(parents.size()==1) target=parents.front();
      else {if(parents.empty()) need(group.size()==1 && refs.size()==1,"replay.distinct_birth");
        target=new_node(parents,parents.empty()?m.blocks[group.front()].ball:absent32,m.blocks[begin].run);}
      for(auto b:group) {block_anchor[b]=target;out.anchors[m.blocks[b].ball]=static_cast<u32>(target);}
      if(mutant==ReplayMutant::DropContinuation && parents.size()==1) refs.clear();
      if(parents.size()!=1 || !refs.empty()) {
        if(!opened) {out.draft.open_batch(m.blocks[level_position].level);opened=true;}
        out.draft.add_action(parents,refs);
      } else out.stats.inert_blocks+=group.size();
    }
    begin=end;
  }
  need(std::count(out.next.begin(),out.next.end(),absent)==1,"replay.final_component");return out;
}
}
