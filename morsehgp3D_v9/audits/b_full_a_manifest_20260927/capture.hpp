#pragma once
// Audit-only observations. No view survives its hook and no native decisions
// are replaced. One writer per K; active is fixed before threads and read
// after every builder join. A failed call invalidates the complete capture.
#include "tower/forest/full_ball_tower.hpp"
#include <memory>
#include <stdexcept>
namespace mhgp9::audit::a_manifest {
using namespace mhgp9::tower;
inline constexpr u64 absent=std::numeric_limits<u64>::max();
inline constexpr u32 absent32=std::numeric_limits<u32>::max();
inline constexpr u64 ball_tag=u64{1}<<63;
inline void need(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
using Clock=std::chrono::steady_clock;
inline double milliseconds(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
struct Stats {
  u64 anchor_blocks{},regular_blocks{},extra_blocks{},representatives{},births{},merges{},contributions{},
      inert_blocks{},singleton_lots{},grouped_lots{},lot_dsu_slots{};
  bool operator==(const Stats&) const=default;
};
template<class S> Stats semantic(const S& s) {return {s.anchor_blocks,s.regular_blocks,s.extra_blocks,s.representatives,
  s.births,s.merges,s.contributions,s.inert_blocks,s.singleton_lots,s.grouped_lots,s.lot_dsu_slots};}
struct Input {
  unsigned k{};std::vector<PointId> domain,spatial_ids;
  std::vector<u32> program,level_run,count,targets;
  std::vector<ExactLevel> level;std::vector<u8> regular,shell_size,interior;
  std::vector<u16> contribution;
};
struct Output {
  FullCoverageFlatDraft draft;std::vector<u32> anchors,runs,birth_ball;
  std::vector<u64> next,root_begin{0},roots;Stats stats;
};
struct Slot {Input input;Output output;unsigned starts{},finishes{};u64 blocks_seen{};double copy_ms{};};
struct Capture {std::array<Slot,11> slots;bool complete=false;};
inline Capture* active=nullptr;
template<class O,class Program,class Balls,class Runs,class Domain,class Index>
void capture_a_input(const O& o,const Program& program,const Balls& balls,const Runs& runs,const Domain& domain,const Index& ix) {
  if(!active) return;
  const auto t=Clock::now();need(o.k>0 && o.k<active->slots.size(),"capture.order");auto& slot=active->slots[o.k];
  need(slot.starts==0 && o.lean_failed==std::numeric_limits<size_t>::max(),"capture.failed_or_repeated_input");
  auto& in=slot.input;in.k=o.k;in.program.assign(program.begin(),program.end());
  in.level_run.assign(runs.begin(),runs.end());in.count=o.lean_count;in.contribution=o.lean_contribution;in.interior=o.lean_interior;
  if(o.k==1) in.targets=o.lean_k1;else in.targets.assign(o.static_targets.begin(),o.static_targets.end());
  in.domain.assign(domain.begin(),domain.end());
  for(size_t j=0;j<ix.upos.size();++j) in.spatial_ids.push_back(ix.point_id(static_cast<i32>(j)));
  for(const auto& b:balls) {in.level.push_back(b.level);in.regular.push_back(b.n_shell==b.arity);in.shell_size.push_back(b.n_shell);}
  ++slot.starts;slot.copy_ms+=milliseconds(t);
}
template<class Roots> void capture_a_roots(unsigned k,size_t position,u32 ball,const Roots& roots) {
  if(!active) return;
  const auto t=Clock::now();need(k>0 && k<active->slots.size(),"capture.roots_order");auto& s=active->slots[k];
  need(s.starts==1 && s.finishes==0 && position==s.blocks_seen && position<s.input.program.size() && s.input.program[position]==ball,
       "capture.roots_position");
  s.output.roots.insert(s.output.roots.end(),roots.begin(),roots.end());s.output.root_begin.push_back(s.output.roots.size());
  ++s.blocks_seen;s.copy_ms+=milliseconds(t);
}
template<class O> void capture_a_output(const O& o) {
  if(!active) return;
  const auto t=Clock::now();need(o.k>0 && o.k<active->slots.size(),"capture.output_order");auto& s=active->slots[o.k];
  need(s.starts==1 && s.finishes==0 && s.blocks_seen==s.input.program.size(),"capture.output_shape");
  s.output.draft=o.draft.flat;s.output.anchors=o.anchors;s.output.next=o.current.next;s.output.runs=o.runs;
  s.output.birth_ball=o.birth_ball;s.output.stats=semantic(o.st);++s.finishes;s.copy_ms+=milliseconds(t);
}
class CaptureScope final {
 public:
  explicit CaptureScope(Capture& c) {c.complete=false;need(active==nullptr,"capture.nested");active=&c;}
  ~CaptureScope() {active=nullptr;}
  CaptureScope(const CaptureScope&)=delete;CaptureScope& operator=(const CaptureScope&)=delete;
};
}
