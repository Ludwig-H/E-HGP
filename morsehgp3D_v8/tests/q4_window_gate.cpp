// Explicit reuse of test-only rational Gram/census/canonical helpers. The
// previous gate is not executed; no independent audit model is imported.
#define main mhgp8_pruning_helpers_for_window_gate
#include "q34_family_pruning_gate.cpp"
#undef main

#include "lanes/q4_window.hpp"

#include <cstdlib>
#include <new>

// Test-only allocation injector, armed solely around the new sweep call.
namespace window_allocation {
struct State { bool armed{}; std::size_t remaining{}; };
thread_local State state;
[[gnu::noinline]] void* allocate(std::size_t size,std::size_t alignment=alignof(std::max_align_t)) {
  if(state.armed) {
    if(state.remaining==0) {state.armed=false;throw std::bad_alloc();}
    --state.remaining;
  }
  void* result=nullptr;
  if(alignment<=alignof(std::max_align_t)) result=std::malloc(size==0?1:size);
  else if(posix_memalign(&result,alignment,size==0?1:size)!=0) result=nullptr;
  if(!result) throw std::bad_alloc();
  return result;
}
[[gnu::noinline]] void release(void* p) noexcept {std::free(p);}
}
void* operator new(std::size_t s) {return window_allocation::allocate(s);}
void* operator new[](std::size_t s) {return window_allocation::allocate(s);}
void* operator new(std::size_t s,std::align_val_t a) {return window_allocation::allocate(s,static_cast<std::size_t>(a));}
void* operator new[](std::size_t s,std::align_val_t a) {return window_allocation::allocate(s,static_cast<std::size_t>(a));}
void operator delete(void* p) noexcept {window_allocation::release(p);}
void operator delete[](void* p) noexcept {window_allocation::release(p);}
void operator delete(void* p,std::size_t) noexcept {window_allocation::release(p);}
void operator delete[](void* p,std::size_t) noexcept {window_allocation::release(p);}
void operator delete(void* p,std::align_val_t) noexcept {window_allocation::release(p);}
void operator delete[](void* p,std::align_val_t) noexcept {window_allocation::release(p);}
void operator delete(void* p,std::size_t,std::align_val_t) noexcept {window_allocation::release(p);}
void operator delete[](void* p,std::size_t,std::align_val_t) noexcept {window_allocation::release(p);}

namespace {
struct WindowGate : Gate {
  u64 families{},root_groups{},root_ids{},constant_rejections{},empty_windows{};
  u64 point_windows{},point_windows_with_constants{},point_window_candidates{};
  u64 lower_infinite{},upper_infinite{},both_infinite{},finite_intervals{};
  u64 mixed_groups{},constant_inside{},constant_shells{},endpoint_ids{},inner_ids{};
  u64 fixed_inside{},outside_ids{},depth_rejected_groups{},removed_seed_rejections{};
  u64 interior_bound_checks{},strict_shell_checks{},extreme_calls{},allocation_failures{};
  u64 fixed_rejections{},second_pass_sites{},heap_comparisons{},heap_replacements{};
  u64 huge_k_calls{};
  u64 isolated_point_fixtures{};
  // 18-bit twin fixtures (coordinate_limit = 262143), counted apart.
  u64 wide_calls{};
};

Rational window_fraction(Big n,Big d) {
  if(d==0) throw std::runtime_error("window oracle singular fraction");
  if(d<0) {n=-n;d=-d;}
  return Rational(std::move(n),std::move(d));
}
struct WindowEvent { std::size_t id{}; Rational root; bool entry{}; };
struct WindowRoot {
  std::vector<std::size_t> ids;
  std::size_t entries{},exits{},depth{};
};
struct WindowOracle {
  std::vector<WindowEvent> events;
  std::vector<std::size_t> shell;
  std::map<Rational,WindowRoot> all_groups,groups;
  std::optional<Rational> lower,upper;
  std::size_t constant_inside{},constant_outside{},entries{},exits{},h{};
  std::size_t lower_ids{},upper_ids{},inner_ids{},outside_ids{},fixed_inside{};
  std::size_t rejected_groups{},rejected_ids{};
  bool constant_rejected{},empty{},point{};
};

// Every root comes from an independently solved rational circumcircle and
// a multiprecision normal. This neither calls the reduced root comparator
// nor selects roots with the product's heap/window algorithm.
WindowOracle window_oracle(WindowGate& gate,const Points& points,Edge edge,std::size_t x,
                           const mhgp8::Q4ShallowSetPtr& selected) {
  WindowOracle result;
  const auto face=oracle::make(select(points,Ids{edge[0],edge[1],x}));
  gate.require(face.ball.has_value(),"window oracle requires a rational acute face");
  const auto d=oracle::difference(points[edge[1]],points[edge[0]]);
  const auto u=oracle::difference(points[x],points[edge[0]]);
  const oracle::Vector normal{d[1]*u[2]-d[2]*u[1],d[2]*u[0]-d[0]*u[2],d[0]*u[1]-d[1]*u[0]};
  const Big gram=oracle::dot(normal,normal);
  std::vector<Rational> entries,exits;
  for(const auto id:selected->retained_ids()) {
    const Big side=oracle::dot(normal,oracle::difference(points[id],points[edge[0]]));
    const Rational power=Rational(gram)*face.ball->power(points[id]);
    gate.require(power.denominator()==1,"rational scaled family power is nonintegral");
    if(side==0) {
      if(power<0) ++result.constant_inside;
      else if(power==0) result.shell.push_back(id);
      else ++result.constant_outside;
      continue;
    }
    const auto root=window_fraction(power.numerator(),side);
    result.events.push_back({id,root,side>0});
    auto& group=result.all_groups[root];group.ids.push_back(id);
    if(side>0) {entries.push_back(root);++group.entries;++result.entries;}
    else {exits.push_back(root);++group.exits;++result.exits;}
  }
  for(auto& [root,group]:result.all_groups) {
    group.depth=result.constant_inside;
    for(const auto& event:result.events)
      group.depth+=static_cast<std::size_t>(event.entry?event.root<root:event.root>root);
  }
  const auto threshold=selected->kmax()-2;
  if(result.constant_inside>=threshold) {result.constant_rejected=true;return result;}
  result.h=threshold-result.constant_inside;
  std::sort(entries.begin(),entries.end());std::sort(exits.begin(),exits.end(),std::greater<Rational>{});
  if(entries.size()>=result.h) result.upper=entries[result.h-1];
  if(exits.size()>=result.h) result.lower=exits[result.h-1];
  if(result.lower && result.upper && *result.lower>*result.upper) {result.empty=true;return result;}
  result.point=result.lower && result.upper && *result.lower==*result.upper;
  result.fixed_inside=result.constant_inside;
  for(const auto& event:result.events) {
    const bool before=result.lower && event.root<*result.lower;
    const bool after=result.upper && event.root>*result.upper;
    if(before || after) {
      ++result.outside_ids;
      if((event.entry && before)||(!event.entry && after)) ++result.fixed_inside;
    } else if(result.lower && event.root==*result.lower) ++result.lower_ids;
    else if(result.upper && event.root==*result.upper) ++result.upper_ids;
    else ++result.inner_ids;
  }
  for(const auto& [root,group]:result.all_groups) {
    const bool included=(!result.lower || root>=*result.lower)&&(!result.upper || root<=*result.upper);
    gate.require(group.depth>=threshold || included,"a rational shallow event escaped the closed window");
    if(!included) continue;
    result.groups.emplace(root,group);
    if(group.depth>=threshold) {++result.rejected_groups;result.rejected_ids+=group.ids.size();}
    // Independently test every omitted event at this centre. Only the fixed
    // interior population can contribute; none may silently disappear from
    // the full shell, even at either quantile endpoint.
    std::size_t reduced_depth=result.fixed_inside;
    std::set<std::size_t> reduced_shell(result.shell.begin(),result.shell.end());
    for(const auto& event:result.events) {
      const bool kept=(!result.lower || event.root>=*result.lower)&&(!result.upper || event.root<=*result.upper);
      if(!kept) continue;
      reduced_depth+=static_cast<std::size_t>(event.entry?event.root<root:event.root>root);
      if(event.root==root) reduced_shell.insert(event.id);
    }
    gate.require(reduced_depth==group.depth && reduced_shell.size()==group.ids.size()+result.shell.size(),
                 "window clipping changed strict rational depth or lost a shell contact");
    ++gate.strict_shell_checks;
  }
  const auto one_side=result.h-1;
  gate.require(result.inner_ids<=one_side || result.inner_ids-one_side<=one_side,
               "strict interior events exceed the proven 2H-2 bound");
  ++gate.interior_bound_checks;
  return result;
}

Output window_expected(WindowGate& gate,const Points& points,Edge edge,std::size_t x,std::size_t k) {
  auto result=oracle_seed(gate,points,edge,x,k);
  result.erase(std::remove_if(result.begin(),result.end(),[](const auto& candidate) {return candidate.arity!=4;}),result.end());
  return result;
}

std::vector<std::size_t> window_seeds(const Points& points,Edge edge) {
  std::vector<std::size_t> ids;
  for(std::size_t x=0;x!=points.size();++x) {
    if(x==edge[0] || x==edge[1]) continue;
    const Ids seed{edge[0],edge[1],x};
    if(oracle::make(select(points,seed)).ball && owner(points,seed)==edge) ids.push_back(x);
  }
  return ids;
}

void window_ledger(WindowGate& gate,const mhgp8::Q4WindowSweepWork& work,std::size_t outputs) {
  const auto& s=work.sweep;const auto& w=work.window;
  gate.require(s.emitted==outputs && s.family.callbacks==s.family.groups,
               "window emission/group ledger");
  gate.require(s.presentations==s.owner_rejections+s.positive_tests &&
    s.positive_tests==s.positive_rejections+s.canonical_tests &&
    s.canonical_tests==s.canonical_rejections+s.emitted,
    "window positive/owner/canonical presentation partition");
  gate.require(s.family.event_count==w.outside_ids+w.rejected_event_ids+s.depth_skipped_ids+
    s.presentations+s.unexamined_after_emit &&
    s.family.groups==s.depth_rejected_groups+s.emitted+s.groups_without_support,
    "window discarded events or root groups are unaccounted");
  gate.require(s.family.sites==s.family.entries+s.family.exits+s.family.constant_inside+
    s.family.constant_on+s.family.constant_outside,
    "window first-pass site classification is not exhaustive");
  gate.require(s.family.retained_capacity_bytes==s.peak_buffer_bytes &&
    w.peak_buffer_bytes==s.peak_buffer_bytes && w.peak_heap_bytes<=w.peak_buffer_bytes,
    "window simultaneous heap/shell/event capacities are omitted");
}

void window_account(WindowGate& gate,const mhgp8::Q4WindowSweepWork& work,
                    const WindowOracle& ref,std::size_t r,std::size_t threshold) {
  const auto& s=work.sweep;const auto& w=work.window;
  gate.require(w.seed_queries==1 && s.family.sites==r &&
    s.family.entries==ref.entries && s.family.exits==ref.exits &&
    s.family.event_count==ref.events.size() && s.family.constant_inside==ref.constant_inside &&
    s.family.constant_on==ref.shell.size() && s.family.constant_outside==ref.constant_outside,
    "window census differs from all rational retained roots and constants");
  gate.require(w.entry_heap_insertions==std::min(threshold,ref.entries) &&
    w.exit_heap_insertions==std::min(threshold,ref.exits),
    "window heaps did not select bounded numbers of original IDs");
  gate.require(w.constant_rejected_seeds==static_cast<u64>(ref.constant_rejected) &&
    w.disjoint_rejected_seeds==static_cast<u64>(ref.empty) &&
    w.lower_bounds==static_cast<u64>(ref.lower.has_value()) &&
    w.upper_bounds==static_cast<u64>(ref.upper.has_value()) &&
    w.point_windows==static_cast<u64>(ref.point),
    "window bounds/rejections differ from rational weighted order statistics");
  ++gate.families;gate.constant_inside+=ref.constant_inside;gate.constant_shells+=ref.shell.size();
  gate.heap_comparisons+=w.heap_comparisons;gate.heap_replacements+=w.entry_heap_replacements+w.exit_heap_replacements;
  if(ref.constant_rejected || ref.empty) {
    gate.require(w.second_pass_sites==0 && w.rejected_event_ids==ref.events.size() &&
      w.lower_ids==0 && w.upper_ids==0 && w.inner_ids==0 && w.outside_ids==0 &&
      w.fixed_inside_sites==0 && s.family.groups==0,
      "rejected window scanned again or lost the complete rejected-event ledger");
    if(ref.constant_rejected) ++gate.constant_rejections;else ++gate.empty_windows;
    return;
  }
  gate.require(w.second_pass_sites==r && w.lower_ids==ref.lower_ids &&
    w.upper_ids==ref.upper_ids && w.inner_ids==ref.inner_ids && w.outside_ids==ref.outside_ids &&
    w.fixed_inside_sites==ref.fixed_inside-ref.constant_inside && w.max_inner_ids==ref.inner_ids &&
    w.max_endpoint_ids==std::max(ref.lower_ids,ref.upper_ids),
    "closed window masses differ from all rational roots including endpoint ties");
  const bool fixed_rejected=ref.fixed_inside>=threshold;
  gate.require(w.fixed_depth_rejected_seeds==static_cast<u64>(fixed_rejected),
    "window fixed-depth rejection differs from rational permanent population");
  gate.require(s.family.groups==(fixed_rejected?0:ref.groups.size()) &&
    w.rejected_event_ids==(fixed_rejected?ref.lower_ids+ref.upper_ids+ref.inner_ids:0) &&
    s.depth_rejected_groups==(fixed_rejected?0:ref.rejected_groups) &&
    s.depth_skipped_ids==(fixed_rejected?0:ref.rejected_ids),
    "window root groups or strict depths differ from full rational sweep");
  if(ref.point) {
    ++gate.point_windows;
    if(ref.constant_inside) ++gate.point_windows_with_constants;
    if(s.emitted) ++gate.point_window_candidates;
  } else if(ref.lower && ref.upper) ++gate.finite_intervals;
  if(!ref.lower) ++gate.lower_infinite;
  if(!ref.upper) ++gate.upper_infinite;
  if(!ref.lower && !ref.upper) ++gate.both_infinite;
  for(const auto& [root,group]:ref.groups) {
    static_cast<void>(root);++gate.root_groups;gate.root_ids+=group.ids.size();
    if(group.entries && group.exits) ++gate.mixed_groups;
  }
  gate.endpoint_ids+=ref.lower_ids+ref.upper_ids;gate.inner_ids+=ref.inner_ids;
  gate.fixed_inside+=ref.fixed_inside-ref.constant_inside;gate.outside_ids+=ref.outside_ids;
  gate.depth_rejected_groups+=ref.rejected_groups;gate.fixed_rejections+=static_cast<u64>(fixed_rejected);
  gate.second_pass_sites+=w.second_pass_sites;
}

Output window_seed_check(WindowGate& gate,const Points& points,Edge edge,std::size_t x,
                         const mhgp8::Q4ShallowSetPtr& selected) {
  const auto expected=window_expected(gate,points,edge,x,selected->kmax());
  Output actual,baseline;
  const auto work=mhgp8::run_q4_window_seed_candidates(selected,x,[&](const auto& c) {actual.push_back(copy(c));});
  static_cast<void>(mhgp8::run_q4_shallow_seed_candidates(selected,x,[&](const auto& c) {baseline.push_back(copy(c));}));
  normalize(actual);normalize(baseline);
  gate.require(actual==expected && baseline==expected,
    "window sweep differs from complete rational ball/depth/support/shell oracle");
  window_ledger(gate,work,actual.size());
  const bool retained=std::binary_search(selected->retained_ids().begin(),selected->retained_ids().end(),x);
  if(retained) {
    const auto ref=window_oracle(gate,points,edge,x,selected);
    window_account(gate,work,ref,selected->retained_ids().size(),selected->kmax()-2);
  } else {
    gate.require(work.sweep.removed_seed_rejections==1 && work.sweep.family.sites==0 &&
      work.window.seed_queries==0 && actual.empty(),"removed seed entered the window census");
    ++gate.removed_seed_rejections;
  }
  for(const auto& candidate:actual) {
    gate.require(candidate.arity==4,"window q4 path emitted q3");
    gate.max_shell=std::max(gate.max_shell,static_cast<u64>(candidate.shell.size()));
    ++gate.candidates;++gate.q4;
  }
  ++gate.seed_calls;++gate.reference_calls;
  return actual;
}

Output window_pipeline(WindowGate& gate,const Points& points,Edge edge,std::size_t k,
                       bool all_seeds=true) {
  const auto cover=mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points)),edge);
  edge=cover->edge_ids();
  const auto selected=mhgp8::Q4ShallowSet::make(mhgp8::Q4LocalGeometry::make(cover,mhgp8::Q4CenterDomainMode::Disk),k);
  Output expected;
  for(const auto x:window_seeds(points,edge)) {
    auto one=all_seeds?window_seed_check(gate,points,edge,x,selected):window_expected(gate,points,edge,x,k);
    expected.insert(expected.end(),one.begin(),one.end());
  }
  Output actual,baseline;
  const auto work=mhgp8::run_q4_window_edge_candidates(cover,k,[&](const auto& c) {actual.push_back(copy(c));});
  static_cast<void>(mhgp8::run_q4_shallow_edge_candidates(cover,k,[&](const auto& c) {baseline.push_back(copy(c));}));
  normalize(expected);normalize(actual);normalize(baseline);
  gate.require(actual==expected && baseline==expected,
    "window edge lost a complete rational candidate or canonical acute seed");
  window_ledger(gate,work.sweep,actual.size());
  gate.require(work.seed_candidates+2==selected->retained_ids().size() &&
    work.selection==selected->work(),"window edge changed the common shallow preparation");
  ++gate.edge_calls;++gate.reference_calls;
  return actual;
}

Points window_rings() {
  Points result{{20,30,30},{40,30,30}};
  for(const int radius:{11,12,15}) {
    result.push_back({30,static_cast<mhgp8::Coordinate>(30+radius),30});
    result.push_back({30,static_cast<mhgp8::Coordinate>(30-radius),30});
    result.push_back({30,30,static_cast<mhgp8::Coordinate>(30+radius)});
    result.push_back({30,30,static_cast<mhgp8::Coordinate>(30-radius)});
  }
  return result;
}

Points window_point_fixture(std::size_t k) {
  Points points{{900,1000,1000},{1100,1000,1000},{1000,1040,1120},{1000,1100,1000},{1000,900,1000}};
  for(std::size_t j=0;j<k-3;++j) points.push_back({static_cast<mhgp8::Coordinate>(1000+j),1000,1000});
  return points;
}
// 18-bit twin: the same configuration translated by +261000 on every axis so
// that its top site reaches 262120 < coordinate_limit. Translation preserves
// the point window, the depth K-3 and the five-site shell; the ball key is
// taken from the rational oracle at run time, never written by hand.
Points window_point_fixture18(std::size_t k) {
  Points points{{261900,262000,262000},{262100,262000,262000},{262000,262040,262120},
    {262000,262100,262000},{262000,261900,262000}};
  for(std::size_t j=0;j<k-3;++j) points.push_back({static_cast<mhgp8::Coordinate>(262000+j),262000,262000});
  return points;
}

void window_fixtures(WindowGate& gate) {
  // First causal case, before any ledger-only fixture: the full sphere has
  // 30 shell sites and one noncoplanar interior site. For seed 0,1,2 at K4,
  // both quantile endpoints coincide, c=0 and fixed_inside=1. Opening the
  // window loses the sphere, dropping fixed_inside changes its depth, and
  // dropping the constant shell loses original support IDs. All are judged
  // by the independently reconstructed output before work counters.
  auto weighted_shell=shell30();weighted_shell.push_back({20,20,20});
  const auto weighted_output=window_pipeline(gate,weighted_shell,{0,1},4);
  gate.require(std::any_of(weighted_output.begin(),weighted_output.end(),[](const auto& candidate) {
    return candidate.depth==1 && candidate.shell.size()==30;
  }),"causal point-window fixture lacks its depth-one sphere and complete 30-site shell");
  // Explicit port of the eight coordinates in independent audit A's
  // audits/q4_kernel_composition_20260920/DEGENERACIES.md. Its model/code and
  // qualifications are NOT imported: our Gram oracle rebuilds every ball,
  // depth, support and shell, then compares both product paths afresh.
  // At K3 the shallow centre is an isolated point in the full centre plane.
  const Points isolated{{30,30,30},{36,36,30},{30,36,24},{36,30,24},
    {30,36,30},{36,30,30},{30,30,24},{32,32,32}};
  const auto point_windows_before=gate.point_windows;
  const auto isolated_output=window_pipeline(gate,isolated,{0,1},3);
  const Coefficients isolated_key{Big(1),Big(-66),Big(-66),Big(-54),Big(2880)};
  gate.require(gate.point_windows>point_windows_before &&
    std::any_of(isolated_output.begin(),isolated_output.end(),[&](const auto& candidate) {
      return candidate.key==isolated_key && candidate.depth==0 &&
        candidate.support==std::vector<std::size_t>{0,1,2,3} &&
        candidate.shell==std::vector<std::size_t>{0,1,2,3,4,5,6,7};
    }),"isolated shallow centre lost its exact positive support or eight-site shell");
  ++gate.isolated_point_fixtures;
  const Points regular{{10,10,10},{12,12,10},{10,12,8},{12,10,8}};
  const Points late_valid{{0,0,0},{2,2,0},{2,2,2},{2,0,2},{0,2,2}};
  const Points fixed{{10,20,20},{30,20,20},{20,35,20},{20,20,21},{20,20,19},{20,20,35},{20,20,5}};
  const Points internal{{10,20,20},{30,20,20},{20,35,20},{20,20,35},{20,20,5},
    {20,20,36},{20,20,4},{20,20,37},{20,20,3}};
  auto constants=fixed;constants.push_back({20,20,20});constants.push_back({20,21,20});
  for(const auto k:{3U,4U,5U,10U}) {
    for(const auto* points:std::array<const Points*,5>{&regular,&late_valid,&fixed,&internal,&constants})
      static_cast<void>(window_pipeline(gate,*points,{0,1},k));
    static_cast<void>(window_pipeline(gate,window_rings(),{0,1},k));
  }
  static_cast<void>(window_pipeline(gate,regular,{0,1},std::numeric_limits<std::size_t>::max()));
  ++gate.huge_k_calls;
  for(const auto k:{5U,10U}) {
    const auto points=window_point_fixture(k);
    const auto output=window_pipeline(gate,points,{0,1},k);
    const Coefficients key{Big(1),Big(-2000),Big(-2000),Big(-2050),Big(3040000)};
    gate.require(std::any_of(output.begin(),output.end(),[&](const auto& candidate) {
      return candidate.key==key && candidate.depth==k-3 && candidate.shell==std::vector<std::size_t>{0,1,2,3,4};
    }),"point window with T-1 constants lost its exact positive sphere and five-site shell");
  }
  for(const auto k:{5U,10U}) {
    const auto points=window_point_fixture18(k);
    const auto output=window_pipeline(gate,points,{0,1},k);
    // Circumsphere of the four fixed sites (full rank, positivity not claimed):
    // the emitted candidate must carry exactly this rational key.
    const auto sphere=oracle::make(select(points,std::array<std::size_t,4>{0,1,2,3}),false);
    gate.require(sphere.ball.has_value(),"18-bit point window fixture lost its rational circumsphere");
    gate.require(std::any_of(output.begin(),output.end(),[&](const auto& candidate) {
      return candidate.key==sphere.ball->coefficients && candidate.depth==k-3 &&
        candidate.shell==std::vector<std::size_t>{0,1,2,3,4};
    }),"18-bit point window with T-1 constants lost its exact positive sphere and five-site shell");
    ++gate.wide_calls;
  }
  for(const auto k:{3U,5U,10U}) {
    auto points=shell30();
    if(k==5) {points.push_back({20,17,19});points.push_back({20,17,21});}
    if(k==10) for(mhgp8::Coordinate z=17;z<=23;++z) points.push_back({20,17,z});
    static_cast<void>(window_pipeline(gate,points,{0,1},k));
  }
  const Points extremes{{0,0,0},{65535,65534,65533},{0,65535,65535},
    {65535,0,65535},{32767,32767,32767},{65535,0,0}};
  const Points oblique{{0,0,0},{60000,65000,1000},{62000,500,64000},{2000,63000,62000},
    {32000,32000,32000},{65535,65535,65535},{65000,30000,30000},{100,25000,62000}};
  for(const auto* points:{&extremes,&oblique}) for(const auto k:{3U,5U,10U}) {
    static_cast<void>(window_pipeline(gate,*points,{0,1},k));++gate.extreme_calls;
  }
  // 18-bit twins (coordinate_limit = 262143; 262142/262141 and 131071 replace
  // 65534/65533 and 32767): the corners above are interior sites since the
  // widening. Same rational window oracle, same ledgers.
  const Points extremes18{{0,0,0},{262143,262142,262141},{0,262143,262143},
    {262143,0,262143},{131071,131071,131071},{262143,0,0}};
  const Points oblique18{{0,0,0},{240000,260000,4000},{248000,2000,256000},{8000,252000,248000},
    {128000,128000,128000},{262143,262143,262143},{260000,120000,120000},{400,100000,248000}};
  for(const auto* points:{&extremes18,&oblique18}) for(const auto k:{3U,5U,10U}) {
    static_cast<void>(window_pipeline(gate,*points,{0,1},k));++gate.extreme_calls;++gate.wide_calls;
  }
  for(std::size_t a=0;a!=late_valid.size();++a) for(std::size_t b=a+1;b!=late_valid.size();++b) {
    static_cast<void>(window_pipeline(gate,late_valid,{a,b},5));++gate.exhaustive_edges;
  }
  for(const auto* source:{&internal,&late_valid}) {
    auto points=*source;std::reverse(points.begin(),points.end());
    static_cast<void>(window_pipeline(gate,points,{points.size()-2,points.size()-1},5));++gate.permutations;
  }
}

void window_lifecycle(WindowGate& gate) {
  auto points=shell30();points.push_back({20,17,19});points.push_back({20,17,21});
  auto cloud=mhgp8::prepare_cloud(points);
  auto index=mhgp8::make_q2_cloud_index(cloud);
  auto cover=mhgp8::Q34EdgeCover::make(index,{0,1});
  auto geometry=mhgp8::Q4LocalGeometry::make(cover,mhgp8::Q4CenterDomainMode::Disk);
  auto selected=mhgp8::Q4ShallowSet::make(geometry,5);
  const auto wanted=window_expected(gate,points,{0,1},2,5);
  gate.require(!wanted.empty(),"window lifecycle fixture has no positive callback");
  const auto ignore=[](const mhgp8::Q34SeedCandidate&) {};
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_window_seed_candidates({},2,ignore));},"null window set accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_window_seed_candidates(selected,2,{}));},"empty window callback accepted");
  gate.rejects<std::out_of_range>([&] {static_cast<void>(mhgp8::run_q4_window_seed_candidates(selected,points.size(),ignore));},"invalid window seed ID accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_window_seed_candidates(selected,0,ignore));},"nonacute repeated window seed accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_window_edge_candidates({},5,ignore));},"null window edge cover accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_window_edge_candidates(cover,0,ignore));},"zero window edge K accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_window_edge_candidates(cover,1,{}));},"inactive window hid invalid callback");
  for(const auto k:{1U,2U}) {
    unsigned callbacks=0;
    const auto work=mhgp8::run_q4_window_edge_candidates(cover,k,[&](const auto&) {++callbacks;});
    gate.require(callbacks==0 && work.selection.preparations==0 && work.seeds==0 &&
      work.sweep.window.seed_queries==0,"inactive q4 window prepared or emitted");
  }
  {
    const Points foreign{{0,0,0},{2,0,0},{1,3,0},{1,0,3}};
    const auto foreign_cover=mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(foreign)),{0,1});
    const auto foreign_set=mhgp8::Q4ShallowSet::make(mhgp8::Q4LocalGeometry::make(foreign_cover,mhgp8::Q4CenterDomainMode::Disk),5);
    unsigned callbacks=0;
    const auto work=mhgp8::run_q4_window_seed_candidates(foreign_set,2,[&](const auto&) {++callbacks;});
    gate.require(callbacks==0 && work.sweep.seed_owner_rejections==1 && work.sweep.family.sites==0 &&
      work.window.seed_queries==0,"foreign owner seed reached window selection");
  }
  const auto immutable_work=selected->work();
  const auto immutable_ids=std::vector<std::size_t>(selected->retained_ids().begin(),selected->retained_ids().end());
  for(const auto position:{std::size_t{0},std::size_t{1},std::size_t{2},std::size_t{3}}) {
    bool caught=false;window_allocation::state={true,position};
    try {static_cast<void>(mhgp8::run_q4_window_seed_candidates(selected,2,ignore));}
    catch(const std::bad_alloc&) {caught=true;}
    catch(...) {window_allocation::state.armed=false;throw;}
    window_allocation::state.armed=false;
    gate.require(caught,"injected window allocation failure was not reached");
    gate.require(selected->work()==immutable_work &&
      std::vector<std::size_t>(selected->retained_ids().begin(),selected->retained_ids().end())==immutable_ids,
      "failed window allocation modified the immutable shallow owner");
    ++gate.allocation_failures;
  }
  struct CallbackFailure {};
  bool caught=false;
  try {static_cast<void>(mhgp8::run_q4_window_seed_candidates(selected,2,[](const auto&) {throw CallbackFailure{};}));}
  catch(const CallbackFailure&) {caught=true;}
  gate.require(caught,"window callback exception hidden");++gate.callback_failures;
  std::array<std::future<Output>,4> futures;
  for(auto& future:futures) future=std::async(std::launch::async,[selected] {
    Output output;
    static_cast<void>(mhgp8::run_q4_window_seed_candidates(selected,2,[&](const auto& c) {output.push_back(copy(c));}));
    normalize(output);return output;
  });
  for(auto& future:futures) {gate.require(future.get()==wanted,"concurrent windows changed shared set or borrowed outputs");++gate.parallel_calls;}
  Output output;
  static_cast<void>(mhgp8::run_q4_window_seed_candidates(selected,2,[&](const auto& c) {
    selected.reset();geometry.reset();cover.reset();index.reset();cloud.reset();output.push_back(copy(c));
  }));
  normalize(output);gate.require(output==wanted,"callback owner reset invalidated window candidate views");
}

}  // namespace

int main(int argc,char** argv) {
  try {
    if(argc!=2 || std::string_view(argv[1])!="--selftest") throw std::invalid_argument("expected --selftest");
    WindowGate gate;window_fixtures(gate);window_lifecycle(gate);
    gate.require(gate.constant_rejections>0 && gate.empty_windows>0 && gate.fixed_rejections>0 &&
      gate.point_windows>0 && gate.point_windows_with_constants>0 && gate.point_window_candidates>0 &&
      gate.lower_infinite>0 && gate.upper_infinite>0 && gate.both_infinite>0 && gate.finite_intervals>0 &&
      gate.mixed_groups>0 && gate.inner_ids>0 && gate.fixed_inside>0 && gate.outside_ids>0 &&
      gate.removed_seed_rejections>0,"nonvacuity: window geometric/event classes missing");
    gate.require(gate.q4>0 && gate.max_shell>=30 && gate.exhaustive_edges==10 && gate.isolated_point_fixtures==1 &&
      gate.allocation_failures==4 && gate.callback_failures==1 && gate.parallel_calls==4,
      "nonvacuity: window output, shell, ownership or lifetime classes missing");
    gate.require(gate.wide_calls==8,"nonvacuity: window 18-bit twin fixtures missing");
    std::cout<<"{\"schema\":\"mhgp8_q4_window_gate_v1\",\"status\":\"passed\"";
#define MHGP8_WINDOW_FIELD(name) std::cout<<",\"" #name "\":"<<gate.name
    MHGP8_WINDOW_FIELD(checks);MHGP8_WINDOW_FIELD(families);MHGP8_WINDOW_FIELD(root_groups);MHGP8_WINDOW_FIELD(root_ids);
    MHGP8_WINDOW_FIELD(constant_rejections);MHGP8_WINDOW_FIELD(empty_windows);MHGP8_WINDOW_FIELD(fixed_rejections);
    MHGP8_WINDOW_FIELD(point_windows);MHGP8_WINDOW_FIELD(point_windows_with_constants);MHGP8_WINDOW_FIELD(point_window_candidates);
    MHGP8_WINDOW_FIELD(lower_infinite);MHGP8_WINDOW_FIELD(upper_infinite);MHGP8_WINDOW_FIELD(both_infinite);MHGP8_WINDOW_FIELD(finite_intervals);
    MHGP8_WINDOW_FIELD(mixed_groups);MHGP8_WINDOW_FIELD(constant_inside);MHGP8_WINDOW_FIELD(constant_shells);
    MHGP8_WINDOW_FIELD(endpoint_ids);MHGP8_WINDOW_FIELD(inner_ids);MHGP8_WINDOW_FIELD(fixed_inside);MHGP8_WINDOW_FIELD(outside_ids);
    MHGP8_WINDOW_FIELD(depth_rejected_groups);MHGP8_WINDOW_FIELD(removed_seed_rejections);MHGP8_WINDOW_FIELD(interior_bound_checks);
    MHGP8_WINDOW_FIELD(strict_shell_checks);MHGP8_WINDOW_FIELD(extreme_calls);MHGP8_WINDOW_FIELD(allocation_failures);
    MHGP8_WINDOW_FIELD(second_pass_sites);MHGP8_WINDOW_FIELD(heap_comparisons);MHGP8_WINDOW_FIELD(heap_replacements);MHGP8_WINDOW_FIELD(huge_k_calls);
    MHGP8_WINDOW_FIELD(isolated_point_fixtures);MHGP8_WINDOW_FIELD(wide_calls);
    MHGP8_WINDOW_FIELD(edge_calls);MHGP8_WINDOW_FIELD(seed_calls);MHGP8_WINDOW_FIELD(reference_calls);
    MHGP8_WINDOW_FIELD(oracle_completions);MHGP8_WINDOW_FIELD(oracle_sites);MHGP8_WINDOW_FIELD(candidates);MHGP8_WINDOW_FIELD(q4);
    MHGP8_WINDOW_FIELD(max_shell);MHGP8_WINDOW_FIELD(exhaustive_edges);MHGP8_WINDOW_FIELD(permutations);
    MHGP8_WINDOW_FIELD(invalid_inputs);MHGP8_WINDOW_FIELD(callback_failures);MHGP8_WINDOW_FIELD(parallel_calls);
#undef MHGP8_WINDOW_FIELD
    std::cout<<"}\n";return 0;
  } catch(const std::exception& error) {
    std::cerr<<"q4 window gate: "<<error.what()<<'\n';return 1;
  }
}
