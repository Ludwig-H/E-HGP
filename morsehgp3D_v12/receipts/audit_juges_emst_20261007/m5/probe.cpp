// Small exact boundary records and a driver backend which traps before any large reservation.
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include "mhgp12/traversal/driver.hpp"
namespace t = mhgp12::traversal;
namespace b = t::bfs;
void need(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }

struct ReservationReached {};
struct Backend {
  template <class T> struct Array { std::vector<T> v; T* data() { return v.data(); } };
  bool read = false;
  unsigned after_reservations = 0, after_launches = 0;
  b::LevelTotals fabricated{};
  template <class T> b::u64 ensure(Array<T>& a, b::u64 n) {
    if (read) { ++after_reservations; throw ReservationReached{}; }
    need(n <= 128, "large allocation before injected totals");
    a.v.resize(n); return 1;
  }
  template <class T> b::u64 ensure_keep(Array<T>& a, b::u64 n, b::u64) { return ensure(a,n); }
  template <class T> b::u64 upload(Array<T>& a, const T* src, b::u64 n) {
    const auto value = ensure(a,n);
    if (n) std::memcpy(a.data(),src,n*sizeof(T));
    return value;
  }
  template <class K> void launch(const K&, b::u64) { if (read) ++after_launches; }
  b::LevelTotals read_totals(Array<b::LevelTotals>&) { read = true; return fabricated; }
  void mark(int, b::u32) {}
};

int main() {
  b::Parent parent{}; parent.sides=1;
  b::ChildOut child{}; child.kind=b::kKindSplit;
  for (int a=0; a<3; ++a) child.hi[a]=2;
  b::ChildScan offset{};
  b::Parent emitted{}; b::u32 next=0;
  b::Level lv{}; lv.parents=&parent; lv.child_out=&child; lv.child_scan=&offset;
  lv.next_parents=&emitted; lv.next_task_begin=&next;
  b::EmitKernel<b::kNone>::Shared shared;
  std::vector<b::u32> counts={0,1,255,256,257,511,512,513};
  for (b::u64 c=0xfffffe00ull; c<=0xffffffffull; ++c) counts.push_back(static_cast<b::u32>(c));
  for (b::u32 count:counts) {
    child.count=count;
    b::u64 f[b::kFields], tests=0, leaf=0;
    b::child_fields(child,f,tests,leaf);
    b::EmitKernel<b::kNone>{lv}(0,shared);
    const b::u64 expected=(b::u64(count)+255)/256;
    need(emitted.tasks==expected && f[2]==2*expected && next==0,"scan/emission boundary");
  }
  const b::u64 hi=std::numeric_limits<b::u64>::max();
  need(b::tasks_of(hi)==(b::u64{1}<<56),"helper u64 maximum");
  std::cout << "{\"kind\":\"capacity\",\"emit_records\":" << counts.size()
            << ",\"helper_u64_max\":true,\"all_exact\":true}\n";
  struct Case { const char* name; b::u64 parents,tasks; bool refuse; };
  const Case cases[]={{"small",1,2,false},
                      {"parent_limit",0x7fffffffull,2,false},
                      {"parent_over",0x80000000ull,2,true},
                      {"task_limit",1,0xffffffffull,false},
                      {"task_over",1,0x100000000ull,true},
                      {"both_over",0x80000000ull,0x100000000ull,true}};
  for (const auto& c:cases) {
    Backend backend;
    backend.fabricated.f[0]=c.parents; backend.fabricated.f[2]=c.tasks;
    backend.fabricated.f[1]=3; backend.fabricated.f[3]=1; backend.fabricated.f[4]=1;
    backend.fabricated.tests=123;
    t::Driver<Backend,b::kNone> driver(backend,b::Params{});
    driver.n_sites=1;
    const b::u32 zero=0;
    t::NoHook hook;
    bool refused=false, next_reservation=false;
    try {
      const auto result=driver.run(&zero,&zero,&zero,hook);
      refused=result.status==t::kStatusCapacity;
      need(result.ledger==t::Ledger{} && result.n_leaves==0 && result.n_leaf_sites==0,
           "capacity refusal published a prefix");
    } catch (const ReservationReached&) { next_reservation=true; }
    need(refused==c.refuse && next_reservation!=c.refuse,"guard boundary");
    need(backend.after_launches==0,"kernel after fabricated totals");
    need(backend.after_reservations==(c.refuse?0u:1u),"guard after reservation");
    std::cout << "{\"kind\":\"guard\",\"case\":\"" << c.name
              << "\",\"capacity_refused\":" << (refused?"true":"false")
              << ",\"reservations_after_totals\":" << backend.after_reservations
              << ",\"kernels_after_totals\":0}\n";
  }
}
