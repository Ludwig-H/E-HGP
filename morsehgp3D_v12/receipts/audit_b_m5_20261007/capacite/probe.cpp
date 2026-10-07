// Petits enregistrements seulement : appel du vrai EmitKernel, sans grand nuage.
#include <iostream>
#include <cstdint>
#include "mhgp12/traversal/bfs.hpp"
namespace b=mhgp12::traversal::bfs;
int main() {
  b::Parent parent{}; parent.sides=1;
  b::ChildOut child{}; child.kind=b::kKindSplit;
  child.lo[0]=child.lo[1]=child.lo[2]=0;
  child.hi[0]=child.hi[1]=child.hi[2]=2;
  b::ChildScan offset{};
  b::Parent emitted{}; b::u32 next=0;
  b::Level lv{}; lv.parents=&parent;lv.child_out=&child;lv.child_scan=&offset;
  lv.next_parents=&emitted;lv.next_task_begin=&next;
  b::EmitKernel<b::kNone>::Shared shared;
  const b::u32 counts[]={1,256,257,0xfffffeffu,0xffffff00u,0xffffff01u,0xfffffffeu};
  bool seen_bad=false;
  for (b::u32 count:counts) {
    child.count=count;
    b::u64 f[b::kFields],tests=0,leaf=0;
    b::child_fields(child,f,tests,leaf);
    b::EmitKernel<b::kNone>{lv}(0,shared);
    const b::u64 exact=(b::u64(count)+255)/256;
    const bool correct=emitted.tasks==exact;
    if ((count<=0xffffff00u)!=correct || f[2]!=2*exact) return 1;
    seen_bad|=!correct;
    std::cout << "{\"count\":"<<count<<",\"exact_tasks\":"<<exact
              <<",\"emitted_tasks\":"<<emitted.tasks<<",\"scan_child_tasks\":"<<f[2]
              <<",\"correct\":"<<(correct?"true":"false")<<"}\n";
  }
  return seen_bad?0:2;
}
