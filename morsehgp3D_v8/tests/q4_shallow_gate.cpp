// Explicit reuse of test-only rational Gram/census/canonical helpers. The
// previous gate is not executed and no independent audit model is imported.
#define main mhgp8_pruning_helpers_for_shallow_gate
#include "q34_family_pruning_gate.cpp"
#undef main

#include "lanes/q4_shallow.hpp"

#include <cstdlib>
#include <new>

// Explicit adaptation of the previous gate's test-only allocation injector.
// It is armed only during factory calls, never during an oracle/assertion.
namespace shallow_allocation {
struct State { bool armed{};std::size_t remaining{}; };
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
[[gnu::noinline]] void release(void* pointer) noexcept {std::free(pointer);}
}
void* operator new(std::size_t s) {return shallow_allocation::allocate(s);}
void* operator new[](std::size_t s) {return shallow_allocation::allocate(s);}
void* operator new(std::size_t s,std::align_val_t a) {return shallow_allocation::allocate(s,static_cast<std::size_t>(a));}
void* operator new[](std::size_t s,std::align_val_t a) {return shallow_allocation::allocate(s,static_cast<std::size_t>(a));}
void operator delete(void* p) noexcept {shallow_allocation::release(p);}
void operator delete[](void* p) noexcept {shallow_allocation::release(p);}
void operator delete(void* p,std::size_t) noexcept {shallow_allocation::release(p);}
void operator delete[](void* p,std::size_t) noexcept {shallow_allocation::release(p);}
void operator delete(void* p,std::align_val_t) noexcept {shallow_allocation::release(p);}
void operator delete[](void* p,std::align_val_t) noexcept {shallow_allocation::release(p);}
void operator delete(void* p,std::size_t,std::align_val_t) noexcept {shallow_allocation::release(p);}
void operator delete[](void* p,std::size_t,std::align_val_t) noexcept {shallow_allocation::release(p);}

namespace {
using Dual = std::array<Rational,2>;
using RationalForm = std::array<Rational,3>;

struct ShallowGate : Gate {
  u64 sets{}, oracle_layers{}, boundary_groups{}, degenerate_groups{}, duplicate_ids{};
  u64 retained_ids{}, discarded_ids{}, zero_sites{}, positive_sites{}, negative_sites{};
  u64 query_centres{}, shallow_centres{}, removed_nonpositive{}, strict_witness_checks{};
  u64 removed_shell_contacts{}, t0_queries{}, collinear_cases{}, extreme_calls{};
  u64 boundary_nonvertices{}, coincident_duals{}, saved_seed_sites{}, allocation_failures{};
  u64 removed_seed_rejections{}, retained_seed_queries{}, constant_shells{}, mixed_sign_cases{};
};

static_assert(!std::is_copy_constructible_v<mhgp8::Q4ShallowSet>);
static_assert(!std::is_move_constructible_v<mhgp8::Q4ShallowSet>);

Rational shallow_fraction(Big n, Big d) {
  if (d == 0) throw std::runtime_error("shallow oracle singular fraction");
  if (d < 0) { n = -n; d = -d; }
  return Rational(std::move(n),std::move(d));
}

struct ShallowPlane {
  Point3 a,b;
  oracle::Vector first{},second{};
  ShallowPlane(Point3 aa,Point3 bb):a(aa),b(bb) {
    const auto v=oracle::difference(b,a);
    std::size_t dominant=0;
    for (std::size_t axis=1;axis!=3;++axis)
      if (oracle::absolute(v[axis])>oracle::absolute(v[dominant])) dominant=axis;
    const Big h=oracle::absolute(v[dominant]);
    const Big sign=v[dominant]<0?-1:1;
    const auto i=(dominant+1)%3,j=(dominant+2)%3;
    first[i]=h;first[dominant]=-sign*v[i];
    second[j]=h;second[dominant]=-sign*v[j];
  }
  oracle::Ball ball(const Dual& coordinate) const {
    oracle::Ball result{};
    for (std::size_t axis=0;axis!=3;++axis) {
      result.center[axis]=(Rational(Big(a[axis])+b[axis])+Rational(first[axis])*coordinate[0]+
                            Rational(second[axis])*coordinate[1])/Rational(2);
      const auto delta=result.center[axis]-Rational(Big(a[axis]));
      result.radius_squared+=delta*delta;
    }
    return result;
  }
  RationalForm form(Point3 point) const {
    const auto constant=Rational(4)*ball({Rational(0),Rational(0)}).power(point);
    return {constant,Rational(4)*ball({Rational(1),Rational(0)}).power(point)-constant,
                     Rational(4)*ball({Rational(0),Rational(1)}).power(point)-constant};
  }
};

Rational dual_turn(const Dual& a,const Dual& b,const Dual& c) {
  return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
}

struct PeelOracle {
  std::set<std::size_t> retained;
  u64 positive_layers{},negative_layers{},boundary_groups{},degenerate_groups{};
  u64 groups{},duplicates{},boundary_nonvertices{};
};

// Tiny-cloud oracle: examine every pair as a candidate supporting line and
// mark ALL collinear sites when every group lies on one side. This is not
// Andrew's scan, not the product's homogeneous determinant, and not a fast
// production algorithm. Coincident rational points keep their full ID list.
PeelOracle shallow_peel(const Points& points, const mhgp8::Q34EdgeCoverPtr& cover,
                       const std::vector<RationalForm>& forms,std::size_t k) {
  PeelOracle result;
  for (const int sign : {1,-1}) {
    std::map<Dual,std::vector<std::size_t>> grouped;
    for (std::size_t id=0;id!=points.size();++id) {
      if (!cover->contains_id(id)) continue;
      if (forms[id][0].numerator()==0) {result.retained.insert(id);continue;}
      if ((forms[id][0].numerator()>0?1:-1)!=sign) continue;
      grouped[Dual{shallow_fraction(forms[id][1].numerator(),forms[id][0].numerator()),
                   shallow_fraction(forms[id][2].numerator(),forms[id][0].numerator())}].push_back(id);
    }
    using Group=std::pair<Dual,std::vector<std::size_t>>;
    std::vector<Group> remaining(grouped.begin(),grouped.end());
    result.groups+=remaining.size();
    for (const auto& group:remaining) result.duplicates+=group.second.size()-1;
    for (std::size_t layer=0;layer!=k-2 && !remaining.empty();++layer) {
      if (sign>0) ++result.positive_layers;else ++result.negative_layers;
      bool proper=false;
      if (remaining.size()>=3)
        for (std::size_t z=2;z!=remaining.size();++z)
          proper=proper || dual_turn(remaining[0].first,remaining[1].first,remaining[z].first).numerator()!=0;
      if (!proper) {
        result.degenerate_groups+=remaining.size();
        for (const auto& group:remaining) result.retained.insert(group.second.begin(),group.second.end());
        remaining.clear();break;
      }
      std::vector<bool> boundary(remaining.size());
      std::set<std::size_t> nonvertices;
      for (std::size_t i=0;i!=remaining.size();++i)
        for (std::size_t j=i+1;j!=remaining.size();++j) {
          bool positive=false,negative=false;
          for (const auto& group:remaining) {
            const auto turn=dual_turn(remaining[i].first,remaining[j].first,group.first).numerator();
            positive=positive || turn>0;negative=negative || turn<0;
          }
          if (positive && negative) continue;
          std::vector<std::size_t> line;
          for (std::size_t z=0;z!=remaining.size();++z)
            if (dual_turn(remaining[i].first,remaining[j].first,remaining[z].first).numerator()==0) {
              boundary[z]=true;line.push_back(z);
            }
          if (line.size()>2)
            for (std::size_t q=1;q+1<line.size();++q) nonvertices.insert(line[q]);
        }
      result.boundary_nonvertices+=nonvertices.size();
      std::vector<Group> next;
      for (std::size_t i=0;i!=remaining.size();++i) {
        if (boundary[i]) {
          ++result.boundary_groups;
          result.retained.insert(remaining[i].second.begin(),remaining[i].second.end());
        } else next.push_back(std::move(remaining[i]));
      }
      remaining=std::move(next);
    }
  }
  return result;
}

void shallow_query(ShallowGate& gate,const Points& points,
    const mhgp8::Q4ShallowSetPtr& selected,const ShallowPlane& plane,const Dual& t) {
  const auto ball=plane.ball(t);
  const std::set<std::size_t> retained(selected->retained_ids().begin(),selected->retained_ids().end());
  std::size_t depth=0,whole=0;
  std::set<std::size_t> shell,whole_shell;
  for (std::size_t id=0;id!=points.size();++id) {
    if (!selected->geometry()->cover()->contains_id(id)) continue;
    const auto power=ball.power(points[id]).numerator();
    if (power<0) {++whole;if(retained.contains(id)) ++depth;}
    if (power==0) {whole_shell.insert(id);if(retained.contains(id)) shell.insert(id);}
  }
  const auto threshold=selected->kmax()-2;
  if (depth<threshold) {
    gate.require(depth==whole && shell==whole_shell,
                 "shallow retained depth or full shell differs from rational cover census");
    ++gate.shallow_centres;
  }
  for (std::size_t id=0;id!=points.size();++id) {
    if (!selected->geometry()->cover()->contains_id(id) || retained.contains(id)) continue;
    const auto power=ball.power(points[id]).numerator();
    if (power<=0) {
      gate.require(depth>=threshold,"removed nonpositive site lacks T retained strict witnesses");
      ++gate.removed_nonpositive;gate.strict_witness_checks+=depth;
      if(power==0) ++gate.removed_shell_contacts;
    }
  }
  ++gate.query_centres;
}

void shallow_primitive(ShallowGate& gate,const Points& points,Edge edge,std::size_t k,
                       bool all_intersections=true) {
  const auto cover=mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points)),edge);
  const auto geometry=mhgp8::Q4LocalGeometry::make(cover,mhgp8::Q4CenterDomainMode::Positive);
  const auto selected=mhgp8::Q4ShallowSet::make(geometry,k);
  edge=cover->edge_ids();
  const ShallowPlane plane(points[edge[0]],points[edge[1]]);
  std::vector<RationalForm> forms;
  for (const auto point:points) forms.push_back(plane.form(point));
  const auto expected=shallow_peel(points,cover,forms,k);
  const std::vector<std::size_t> ids(expected.retained.begin(),expected.retained.end());
  gate.require(selected->geometry().get()==geometry.get() && selected->kmax()==k &&
    std::vector<std::size_t>(selected->retained_ids().begin(),selected->retained_ids().end())==ids,
    "shallow layers differ from independent rational supporting-line oracle");
  const auto& work=selected->work();
  gate.require(work.input_sites==cover->site_count() && work.input_sites==work.zero_sites+work.positive_sites+work.negative_sites &&
    work.input_sites==work.retained_ids+work.discarded_ids && work.retained_ids==ids.size() &&
    work.record_insertions==work.positive_sites+work.negative_sites &&
    work.coordinate_groups+work.duplicate_ids==work.record_insertions && work.group_insertions==work.coordinate_groups,
    "shallow disjoint input/group/retained accounting");
  gate.require(work.positive_layers==expected.positive_layers && work.negative_layers==expected.negative_layers &&
    work.boundary_groups==expected.boundary_groups && work.degenerate_groups==expected.degenerate_groups &&
    work.coordinate_groups==expected.groups && work.duplicate_ids==expected.duplicates,
    "shallow layer/group work differs from rational complete-boundary peeling");
  gate.require(work.retained_bytes==selected->retained_bytes() && work.peak_live_bytes>=work.retained_bytes,
    "shallow preparation omits retained capacity");
  const std::array<Rational,7> samples{Rational(-2),Rational(-1),shallow_fraction(-1,2),Rational(0),shallow_fraction(1,2),Rational(1),Rational(2)};
  for (const auto& x:samples) for(const auto& y:samples) shallow_query(gate,points,selected,plane,{x,y});
  ++gate.t0_queries;
  if (all_intersections)
    for(std::size_t i=0;i!=points.size();++i) for(std::size_t j=i+1;j!=points.size();++j) {
      if(!cover->contains_id(i)||!cover->contains_id(j)) continue;
      const auto result=oracle::solve({{forms[i][1],forms[i][2],-forms[i][0]},
                                      {forms[j][1],forms[j][2],-forms[j][0]}});
      if(result) shallow_query(gate,points,selected,plane,{(*result)[0],(*result)[1]});
    }
  gate.retained_ids+=work.retained_ids;gate.discarded_ids+=work.discarded_ids;
  gate.zero_sites+=work.zero_sites;gate.positive_sites+=work.positive_sites;gate.negative_sites+=work.negative_sites;
  gate.oracle_layers+=expected.positive_layers+expected.negative_layers;
  gate.boundary_groups+=expected.boundary_groups;gate.degenerate_groups+=expected.degenerate_groups;
  gate.duplicate_ids+=expected.duplicates;gate.boundary_nonvertices+=expected.boundary_nonvertices;
  if(expected.duplicates!=0) ++gate.coincident_duals;
  ++gate.sets;
}

std::vector<std::size_t> shallow_seeds(const Points& points,Edge edge) {
  std::vector<std::size_t> ids;
  for(std::size_t x=0;x!=points.size();++x) {
    if(x==edge[0]||x==edge[1]) continue;
    const Ids seed{edge[0],edge[1],x};
    if(oracle::make(select(points,seed)).ball && owner(points,seed)==edge) ids.push_back(x);
  }
  return ids;
}

Output shallow_expected(ShallowGate& gate,const Points& points,Edge edge,std::size_t x,std::size_t k) {
  auto result=oracle_seed(gate,points,edge,x,k);
  result.erase(std::remove_if(result.begin(),result.end(),[](const Candidate& c) {return c.arity!=4;}),result.end());
  return result;
}

void shallow_ledger(ShallowGate& gate,const mhgp8::Q4ShallowSweepWork& work,std::size_t outputs) {
  gate.require(work.emitted==outputs && work.family.callbacks==work.family.groups,
               "shallow sweep emission/group ledger");
  gate.require(work.presentations==work.owner_rejections+work.positive_tests &&
    work.positive_tests==work.positive_rejections+work.canonical_tests &&
    work.canonical_tests==work.canonical_rejections+work.emitted,
    "shallow positive/owner/canonical presentation partition");
  gate.require(work.family.event_count==work.depth_skipped_ids+work.presentations+work.unexamined_after_emit &&
    work.family.groups==work.depth_rejected_groups+work.emitted+work.groups_without_support,
    "shallow discarded events or root groups are unaccounted");
}

Output shallow_pipeline(ShallowGate& gate,const Points& points,Edge edge,std::size_t k) {
  const auto cover=mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points)),edge);
  const auto selected=mhgp8::Q4ShallowSet::make(
    mhgp8::Q4LocalGeometry::make(cover,mhgp8::Q4CenterDomainMode::Positive),k);
  edge=cover->edge_ids();
  const auto ids=shallow_seeds(points,edge);
  const std::set<std::size_t> retained(selected->retained_ids().begin(),selected->retained_ids().end());
  Output all;
  std::size_t retained_seed_count=0;
  for(const auto x:ids) {
    const auto expected=shallow_expected(gate,points,edge,x,k);
    Output actual,baseline;
    const auto work=mhgp8::run_q4_shallow_seed_candidates(selected,x,[&](const auto& c) {actual.push_back(copy(c));});
    static_cast<void>(mhgp8::run_q34_cover_seed_candidates(cover,x,k,[&](const auto& c) {
      if(c.arity==4) baseline.push_back(copy(c));
    }));
    normalize(actual);normalize(baseline);
    gate.require(actual==expected && baseline==expected,
                 "shallow sweep differs from rational complete ball/depth/support/shell oracle");
    shallow_ledger(gate,work,actual.size());
    if(retained.contains(x)) {
      ++retained_seed_count;++gate.retained_seed_queries;
      gate.require(work.removed_seed_rejections==0,"retained acute seed rejected by subset membership");
    } else {
      gate.require(work.removed_seed_rejections==1 && work.family.sites==0 && actual.empty(),
                   "removed acute seed was swept or could produce a shallow root");
      ++gate.removed_seed_rejections;
    }
    gate.constant_shells+=work.family.constant_on;
    for(const auto& candidate:actual) {
      gate.require(candidate.arity==4,"q4 shallow path emitted q3");
      gate.max_shell=std::max(gate.max_shell,static_cast<u64>(candidate.shell.size()));
      ++gate.candidates;++gate.q4;
    }
    all.insert(all.end(),expected.begin(),expected.end());
    ++gate.seed_calls;++gate.reference_calls;
  }
  normalize(all);
  Output actual;
  const auto work=mhgp8::run_q4_shallow_edge_candidates(cover,k,[&](const auto& c) {actual.push_back(copy(c));});
  normalize(actual);
  gate.require(actual==all && work.seeds==retained_seed_count,
               "shallow edge generation lost a canonical seed or complete rational candidate");
  shallow_ledger(gate,work.sweep,actual.size());
  gate.require(work.seed_candidates+2==selected->retained_ids().size() &&
    work.selection.input_sites==cover->site_count() && work.selection.retained_ids==selected->retained_ids().size(),
    "shallow edge did not use exactly its retained original IDs");
  gate.saved_seed_sites+=ids.size()-retained_seed_count;
  ++gate.edge_calls;
  return actual;
}

Points shallow_rings(bool mixed) {
  Points points{{20,30,30},{40,30,30}};
  for(const int radius:{11,12,15}) {
    points.push_back({30,static_cast<std::uint16_t>(30+radius),30});
    points.push_back({30,static_cast<std::uint16_t>(30-radius),30});
    points.push_back({30,30,static_cast<std::uint16_t>(30+radius)});
    points.push_back({30,30,static_cast<std::uint16_t>(30-radius)});
  }
  if(mixed) {
    for(const int radius:{5,2}) {
      points.push_back({30,static_cast<std::uint16_t>(30+radius),30});
      points.push_back({30,static_cast<std::uint16_t>(30-radius),30});
      points.push_back({30,30,static_cast<std::uint16_t>(30+radius)});
      points.push_back({30,30,static_cast<std::uint16_t>(30-radius)});
    }
    points.push_back({30,30,30});
    points.push_back({30,40,30});points.push_back({30,30,40});
  }
  return points;
}

void shallow_fixtures(ShallowGate& gate) {
  const auto rings=shallow_rings(false),mixed=shallow_rings(true);
  // Positive and negative groups each contain a proper triangle with a
  // collinear nonvertex on its boundary; same-coordinate sites keep all IDs.
  // The dual point(1,0) is present with BOTH signs, which must never be merged.
  const Points boundary{{20,30,30},{40,30,30},
    {40,20,20},{40,20,40},{44,20,28},{16,20,28},{44,22,30},{50,30,30},
    {30,34,28},{30,34,32},{28,34,30},{32,34,30},{30,30,30}};
  const Points collinear{{20,30,30},{40,30,30},{30,41,30},{30,42,30},{30,45,30},
    {30,35,30},{30,32,30},{30,30,30},{30,40,30}};
  for(const auto k:{3U,4U,5U,10U}) {
    shallow_primitive(gate,rings,{0,1},k);
    shallow_primitive(gate,mixed,{0,1},k);++gate.mixed_sign_cases;
    shallow_primitive(gate,boundary,{0,1},k);++gate.mixed_sign_cases;
    shallow_primitive(gate,collinear,{0,1},k);++gate.collinear_cases;
    static_cast<void>(shallow_pipeline(gate,rings,{0,1},k));
    static_cast<void>(shallow_pipeline(gate,mixed,{0,1},k));
    static_cast<void>(shallow_pipeline(gate,boundary,{0,1},k));
  }
  const Points regular{{10,10,10},{12,12,10},{10,12,8},{12,10,8}};
  const Points late_valid{{0,0,0},{2,2,0},{2,2,2},{2,0,2},{0,2,2}};
  const Points extremes{{0,0,0},{65535,65534,65533},{0,65535,65535},
    {65535,0,65535},{32767,32767,32767},{65535,0,0}};
  const Points clipped{{100,100,100},{120,120,100},{100,120,80},
    {120,100,80},{105,110,96}};
  for(const auto* points:{&regular,&late_valid,&extremes,&clipped}) {
    shallow_primitive(gate,*points,{0,1},5);
    for(const auto k:{3U,5U,10U}) static_cast<void>(shallow_pipeline(gate,*points,{0,1},k));
  }
  gate.extreme_calls+=4;
  const Points oblique{{0,0,0},{60000,65000,1000},{62000,500,64000},{2000,63000,62000},
    {32000,32000,32000},{65535,65535,65535},{65000,30000,30000},{100,25000,62000}};
  shallow_primitive(gate,oblique,{0,1},3);
  static_cast<void>(shallow_pipeline(gate,oblique,{0,1},5));
  gate.extreme_calls+=2;
  const auto shell=shell30();
  shallow_primitive(gate,shell,{0,1},5,false);
  static_cast<void>(shallow_pipeline(gate,shell,{0,1},5));
  for(std::size_t a=0;a!=late_valid.size();++a)
    for(std::size_t b=a+1;b!=late_valid.size();++b) {
      static_cast<void>(shallow_pipeline(gate,late_valid,{a,b},5));++gate.exhaustive_edges;
    }
  auto permuted=boundary;
  std::reverse(permuted.begin(),permuted.end());
  const Edge reversed{boundary.size()-2,boundary.size()-1};
  shallow_primitive(gate,permuted,reversed,3);
  static_cast<void>(shallow_pipeline(gate,permuted,reversed,5));
  ++gate.permutations;
}

void shallow_lifecycle(ShallowGate& gate) {
  const Points points{{10,10,10},{12,12,10},{10,12,8},{12,10,8}};
  auto cloud=mhgp8::prepare_cloud(points);
  auto index=mhgp8::make_q2_cloud_index(cloud);
  auto cover=mhgp8::Q34EdgeCover::make(index,{0,1});
  auto geometry=mhgp8::Q4LocalGeometry::make(cover,mhgp8::Q4CenterDomainMode::Disk);
  auto selected=mhgp8::Q4ShallowSet::make(geometry,5);
  const auto wanted=shallow_expected(gate,points,{0,1},2,5);
  gate.require(!wanted.empty(),"shallow lifecycle fixture has no positive callback");
  const auto ignore=[](const mhgp8::Q34SeedCandidate&) {};
  gate.rejects([&] {static_cast<void>(mhgp8::Q4ShallowSet::make({},5));},"null shallow geometry accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::Q4ShallowSet::make(geometry,2));},"shallow K<3 accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_shallow_seed_candidates({},2,ignore));},"null shallow set accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_shallow_seed_candidates(selected,2,{}));},"empty shallow callback accepted");
  gate.rejects<std::out_of_range>([&] {static_cast<void>(mhgp8::run_q4_shallow_seed_candidates(selected,points.size(),ignore));},"invalid shallow seed ID accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_shallow_seed_candidates(selected,0,ignore));},"nonacute repeated shallow seed accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_shallow_edge_candidates({},5,ignore));},"null shallow edge cover accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_shallow_edge_candidates(cover,0,ignore));},"zero shallow edge K accepted");
  gate.rejects([&] {static_cast<void>(mhgp8::run_q4_shallow_edge_candidates(cover,1,{}));},"inactive edge hid invalid callback");
  for(const auto k:{1U,2U}) {
    unsigned callbacks=0;
    const auto work=mhgp8::run_q4_shallow_edge_candidates(cover,k,[&](const auto&) {++callbacks;});
    gate.require(callbacks==0 && work.selection.preparations==0 && work.seeds==0,"inactive shallow q4 prepared witnesses or emitted");
  }
  {
    const Points foreign{{0,0,0},{2,0,0},{1,3,0},{1,0,3}};
    const auto foreign_cover=mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(foreign)),{0,1});
    const auto foreign_set=mhgp8::Q4ShallowSet::make(mhgp8::Q4LocalGeometry::make(foreign_cover,mhgp8::Q4CenterDomainMode::Disk),5);
    unsigned callbacks=0;
    const auto work=mhgp8::run_q4_shallow_seed_candidates(foreign_set,2,[&](const auto&) {++callbacks;});
    gate.require(callbacks==0 && work.seed_owner_rejections==1 && work.family.sites==0,
                 "foreign-owner seed reached shallow census");
  }
  const auto immutable_work=selected->work();
  const auto immutable_ids=std::vector<std::size_t>(selected->retained_ids().begin(),selected->retained_ids().end());
  for(const auto position:{std::size_t{0},std::size_t{1},std::size_t{4},std::size_t{7}}) {
    bool caught=false;
    shallow_allocation::state={true,position};
    try {static_cast<void>(mhgp8::Q4ShallowSet::make(geometry,5));}
    catch(const std::bad_alloc&) {caught=true;}
    catch(...) {shallow_allocation::state.armed=false;throw;}
    shallow_allocation::state.armed=false;
    gate.require(caught,"injected shallow allocation failure was not reached");
    gate.require(selected->work()==immutable_work &&
      std::vector<std::size_t>(selected->retained_ids().begin(),selected->retained_ids().end())==immutable_ids,
      "failed shallow construction modified an existing immutable owner");
    ++gate.allocation_failures;
  }
  struct CallbackFailure {};
  bool caught=false;
  try {static_cast<void>(mhgp8::run_q4_shallow_seed_candidates(selected,2,[](const auto&) {throw CallbackFailure{};}));}
  catch(const CallbackFailure&) {caught=true;}
  gate.require(caught,"shallow callback exception hidden");++gate.callback_failures;
  std::array<std::future<Output>,4> futures;
  for(auto& future:futures) future=std::async(std::launch::async,[selected] {
    Output output;
    static_cast<void>(mhgp8::run_q4_shallow_seed_candidates(selected,2,[&](const auto& c) {output.push_back(copy(c));}));
    normalize(output);return output;
  });
  for(auto& future:futures) {gate.require(future.get()==wanted,"concurrent sweeps changed shared shallow set");++gate.parallel_calls;}
  Output output;
  static_cast<void>(mhgp8::run_q4_shallow_seed_candidates(selected,2,[&](const auto& c) {
    selected.reset();geometry.reset();cover.reset();index.reset();cloud.reset();output.push_back(copy(c));
  }));
  normalize(output);
  gate.require(output==wanted,"callback owner reset invalidated shallow candidate views");
}

}  // namespace

int main(int argc,char** argv) {
  try {
    if(argc!=2 || std::string_view(argv[1])!="--selftest") throw std::invalid_argument("expected --selftest");
    ShallowGate gate;
    shallow_fixtures(gate);shallow_lifecycle(gate);
    gate.require(gate.discarded_ids>0 && gate.retained_ids>0 && gate.shallow_centres>0 &&
      gate.removed_nonpositive>0 && gate.removed_shell_contacts>0 && gate.zero_sites>0 &&
      gate.positive_sites>0 && gate.negative_sites>0 && gate.boundary_nonvertices>0 &&
      gate.coincident_duals>0 && gate.degenerate_groups>0 && gate.removed_seed_rejections>0,
      "nonvacuity: shallow geometric certificate classes missing");
    gate.require(gate.q4>0 && gate.max_shell>=30 && gate.exhaustive_edges==10 &&
      gate.parallel_calls==4 && gate.callback_failures==1 && gate.allocation_failures==4,
      "nonvacuity: shallow outputs, ownership or lifetime classes missing");
    std::cout<<"{\"schema\":\"mhgp8_q4_shallow_gate_v1\",\"status\":\"passed\"";
#define MHGP8_SHALLOW_FIELD(name) std::cout<<",\"" #name "\":"<<gate.name
    MHGP8_SHALLOW_FIELD(checks);MHGP8_SHALLOW_FIELD(sets);MHGP8_SHALLOW_FIELD(oracle_layers);
    MHGP8_SHALLOW_FIELD(boundary_groups);MHGP8_SHALLOW_FIELD(degenerate_groups);MHGP8_SHALLOW_FIELD(duplicate_ids);
    MHGP8_SHALLOW_FIELD(retained_ids);MHGP8_SHALLOW_FIELD(discarded_ids);MHGP8_SHALLOW_FIELD(zero_sites);
    MHGP8_SHALLOW_FIELD(positive_sites);MHGP8_SHALLOW_FIELD(negative_sites);MHGP8_SHALLOW_FIELD(query_centres);
    MHGP8_SHALLOW_FIELD(shallow_centres);MHGP8_SHALLOW_FIELD(removed_nonpositive);MHGP8_SHALLOW_FIELD(strict_witness_checks);
    MHGP8_SHALLOW_FIELD(removed_shell_contacts);MHGP8_SHALLOW_FIELD(t0_queries);MHGP8_SHALLOW_FIELD(collinear_cases);
    MHGP8_SHALLOW_FIELD(extreme_calls);MHGP8_SHALLOW_FIELD(boundary_nonvertices);MHGP8_SHALLOW_FIELD(coincident_duals);
    MHGP8_SHALLOW_FIELD(saved_seed_sites);MHGP8_SHALLOW_FIELD(allocation_failures);MHGP8_SHALLOW_FIELD(removed_seed_rejections);
    MHGP8_SHALLOW_FIELD(retained_seed_queries);MHGP8_SHALLOW_FIELD(constant_shells);MHGP8_SHALLOW_FIELD(mixed_sign_cases);
    MHGP8_SHALLOW_FIELD(edge_calls);MHGP8_SHALLOW_FIELD(seed_calls);MHGP8_SHALLOW_FIELD(reference_calls);
    MHGP8_SHALLOW_FIELD(oracle_completions);MHGP8_SHALLOW_FIELD(oracle_sites);MHGP8_SHALLOW_FIELD(candidates);
    MHGP8_SHALLOW_FIELD(q4);MHGP8_SHALLOW_FIELD(max_shell);MHGP8_SHALLOW_FIELD(exhaustive_edges);
    MHGP8_SHALLOW_FIELD(permutations);MHGP8_SHALLOW_FIELD(invalid_inputs);MHGP8_SHALLOW_FIELD(callback_failures);
    MHGP8_SHALLOW_FIELD(parallel_calls);
#undef MHGP8_SHALLOW_FIELD
    std::cout<<"}\n";return 0;
  } catch(const std::exception& error) {
    std::cerr<<"q4 shallow gate: "<<error.what()<<'\n';return 1;
  }
}
