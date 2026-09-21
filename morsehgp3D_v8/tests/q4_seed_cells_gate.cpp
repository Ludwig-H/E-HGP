// Explicit reuse of constructor test-only Gram/census/canonical-support
// helpers. The former gate is not executed; no independent audit model is
// imported. New node/cell bounds use Cartesian rational sphere distances.
#define main mhgp8_cover_helpers_for_seed_cells_gate
#include "q34_cover_gate.cpp"
#undef main

#include "lanes/q4_seed_cells.hpp"

#include <bit>
#include <cstdlib>
#include <new>

namespace seed_cells_allocation {
struct State { bool armed{}; std::size_t remaining{}; bool counting{}; std::size_t calls{}; };
thread_local State state;
[[gnu::noinline]] void* allocate(std::size_t size,std::size_t align=alignof(std::max_align_t)) {
  if (state.counting) ++state.calls;
  if (state.armed) {
    if (state.remaining==0) {state.armed=false;throw std::bad_alloc();}
    --state.remaining;
  }
  void* result=nullptr;
  if (align<=alignof(std::max_align_t)) result=std::malloc(size==0?1:size);
  else if (posix_memalign(&result,align,size==0?1:size)!=0) result=nullptr;
  if (!result) throw std::bad_alloc();
  return result;
}
[[gnu::noinline]] void release(void* value) noexcept {std::free(value);}
}
void* operator new(std::size_t size) {return seed_cells_allocation::allocate(size);}
void* operator new[](std::size_t size) {return seed_cells_allocation::allocate(size);}
void* operator new(std::size_t size,std::align_val_t align) {return seed_cells_allocation::allocate(size,static_cast<std::size_t>(align));}
void* operator new[](std::size_t size,std::align_val_t align) {return seed_cells_allocation::allocate(size,static_cast<std::size_t>(align));}
void operator delete(void* value) noexcept {seed_cells_allocation::release(value);}
void operator delete[](void* value) noexcept {seed_cells_allocation::release(value);}
void operator delete(void* value,std::size_t) noexcept {seed_cells_allocation::release(value);}
void operator delete[](void* value,std::size_t) noexcept {seed_cells_allocation::release(value);}
void operator delete(void* value,std::align_val_t) noexcept {seed_cells_allocation::release(value);}
void operator delete[](void* value,std::align_val_t) noexcept {seed_cells_allocation::release(value);}
void operator delete(void* value,std::size_t,std::align_val_t) noexcept {seed_cells_allocation::release(value);}
void operator delete[](void* value,std::size_t,std::align_val_t) noexcept {seed_cells_allocation::release(value);}

namespace {
using oracle::Rational;
using Cell=mhgp8::Q4LocalCell;

struct SeedCellsGate : Gate {
  u64 bounds_queries{},bounds_corners{},bounds_site_checks{},bounds_rounding{},bounds_wide{};
  u64 bounds_contacts{},interior_minimum_cases{},axis_permutations{},extreme_cases{};
  u64 individual_calls{},live_calls{},product_calls{},work_checks{},cache_hits{};
  u64 all_dead_cases{},dead_branch_cases{},live_skips{},positive_rejections{},negative_rejections{};
  u64 seed_splits{},cell_splits{},terminal_pairs{},family_builds{},max_stack{};
  u64 cached_fixture{},obtuse_completions{},inactive_calls{},allocation_failures{},late_allocation_failures{},owner_resets{};
};

// The chart is part of the public mathematical contract. Extrema below use
// the Cartesian centre/radius, not the product's per-axis quadratic formula.
struct SeedPlane {
  Point3 a{},b{};
  oracle::Vector p{},q{};
  SeedPlane(Point3 first,Point3 second):a(first),b(second) {
    const auto d=oracle::difference(b,a);
    std::size_t axis=0;
    for (std::size_t i=1;i<3;++i) if (oracle::absolute(d[i])>oracle::absolute(d[axis])) axis=i;
    const Big h=oracle::absolute(d[axis]),sign=d[axis]<0?-1:1;
    if (h==0) throw std::runtime_error("seed-cell oracle has a duplicate edge");
    p[(axis+1)%3]=h;p[axis]=-sign*d[(axis+1)%3];
    q[(axis+2)%3]=h;q[axis]=-sign*d[(axis+2)%3];
  }
  oracle::Ball ball(mhgp8::i64 alpha,mhgp8::i64 beta) const {
    const Rational scale{Big(Cell::scale)};
    oracle::Ball result{};
    for (std::size_t axis=0;axis<3;++axis) {
      result.center[axis]=(Rational(Big(a[axis])+b[axis])+
        Rational(p[axis])*Rational(Big(alpha))/scale+Rational(q[axis])*Rational(Big(beta))/scale)/Rational(2);
      const auto delta=result.center[axis]-Rational(Big(a[axis]));
      result.radius_squared+=delta*delta;
    }
    return result;
  }
};

std::pair<Rational,Rational> cartesian_bounds(SeedCellsGate& gate,const SeedPlane& plane,
                                            const mhgp8::Box3& box,Cell cell) {
  std::optional<Rational> minimum,maximum;
  for (unsigned corner=0;corner<4;++corner) {
    const auto ball=plane.ball((corner&1U)?cell.right:cell.left,(corner&2U)?cell.top:cell.bottom);
    Rational near=-ball.radius_squared,far=near;
    for (std::size_t axis=0;axis<3;++axis) {
      const Rational lo{Big(box.low[axis])},hi{Big(box.high[axis])};
      const auto nearest=std::max(lo,std::min(hi,ball.center[axis]));
      const auto dn=nearest-ball.center[axis],dl=lo-ball.center[axis],dh=hi-ball.center[axis];
      near+=dn*dn;far+=std::max(dl*dl,dh*dh);
    }
    const Rational multiplier{4*Big(Cell::scale)};
    near*=multiplier;far*=multiplier;
    if (!minimum || near<*minimum) minimum=near;
    if (!maximum || far>*maximum) maximum=far;
    ++gate.bounds_corners;
  }
  return {*minimum,*maximum};
}

Output seed_cells_oracle(SeedCellsGate& gate,const Points& points,Edge edge,std::size_t k) {
  Output result;
  for (std::size_t id=0;id<points.size();++id) {
    if (id==edge[0] || id==edge[1]) continue;
    auto seed=oracle_seed(gate,points,edge,id,k);
    const bool q3=std::any_of(seed.begin(),seed.end(),[](const Candidate& value) {return value.arity==3;});
    const bool q4=std::any_of(seed.begin(),seed.end(),[](const Candidate& value) {return value.arity==4;});
    if (!q3 && q4) ++gate.q4_without_q3;
    for (auto& candidate:seed) if (candidate.arity==4) result.push_back(std::move(candidate));
  }
  normalize(result);
  return result;
}

void account_seed_cells(SeedCellsGate& gate,const Output& actual) {
  for (const auto& candidate:actual) {
    gate.require(candidate.arity==4,"seed-cell q4 path emitted another arity");
    ++gate.candidates;++gate.q4;
    gate.max_shell=std::max(gate.max_shell,static_cast<u64>(candidate.shell.size()));
  }
}

Points contact_eight() {
  // Explicit coordinates from audit A, independently re-solved by our Gram
  // oracle. Neither its proof status nor its model code is reused.
  return {{30,30,30},{36,36,30},{30,36,24},{36,30,24},
          {30,36,30},{36,30,30},{30,30,24},{32,32,32}};
}

void check_seed_bounds(SeedCellsGate& gate,const Points& points,Edge edge) {
  const auto index=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto geometry=mhgp8::Q4LocalGeometry::make(mhgp8::Q34EdgeCover::make(index,edge),mhgp8::Q4CenterDomainMode::Disk);
  const SeedPlane plane(points[edge[0]],points[edge[1]]);
  constexpr auto q=Cell::scale;
  const std::array<Cell,6> cells{{{}, {0,q,-2*q,-q,2,false,false},
    {-q,0,-q,0,2,false,false},{0,0,-q,-q,Cell::max_depth,false,false},
    {1,q-1,-q+1,3,Cell::max_depth,true,false},{-2*q,-2*q+1,2*q-1,2*q,Cell::max_depth,false,true}}};
  for (std::size_t id=0;id<index->spatial_nodes().size();++id) for (const auto cell:cells) {
    const auto& node=index->spatial_nodes()[id];
    const auto exact=cartesian_bounds(gate,plane,node.box,cell);
    const auto bound=geometry->node_bounds(id,cell);
    const Rational low{Big(bound.minimum)},high{Big(bound.maximum)};
    // Three independently rounded coordinates can weaken the lower bound by
    // less than three integer units; the maximum has no division or rounding.
    gate.require(low<=exact.first && exact.first-low<Rational(3) && high==exact.second,
      "node-cell bounds differ from independent Cartesian rational extrema");
    ++gate.bounds_queries;
    if (low!=exact.first) ++gate.bounds_rounding;
    // Scale 2^20 keeps every partition bound inside the proven i64 domain
    // (<2^59, q4_local_partition.cpp); wide bounds count the large ones.
    gate.require(oracle::absolute(Big(bound.minimum))<(Big(1)<<59) && oracle::absolute(Big(bound.maximum))<(Big(1)<<59),
      "block bound left the proven i64 domain of the 2^20 scale");
    if (oracle::absolute(Big(bound.minimum))>=(Big(1)<<50) || oracle::absolute(Big(bound.maximum))>=(Big(1)<<50)) ++gate.bounds_wide;
    if (bound.minimum==0 || bound.maximum==0) ++gate.bounds_contacts;
    for (std::size_t rank=node.range.first;rank<node.range.last;++rank) {
      const auto point=points[index->spatial_order()[rank]];
      for (unsigned corner=0;corner<4;++corner) {
        const auto ball=plane.ball((corner&1U)?cell.right:cell.left,(corner&2U)?cell.top:cell.bottom);
        const auto power=ball.power(point)*Rational(4*Big(q));
        gate.require(low<=power && power<=high,"node-cell bounds exclude an actual site's Cartesian power");
        ++gate.bounds_site_checks;
      }
    }
  }
  gate.rejects<std::out_of_range>([&] {static_cast<void>(geometry->node_bounds(index->spatial_nodes().size(),Cell{}));},
    "node-cell bounds accepted an invalid spatial ID");
  auto invalid=Cell{};invalid.depth=45;
  gate.rejects([&] {static_cast<void>(geometry->node_bounds(0,invalid));},"node-cell bounds accepted depth45");
  invalid=Cell{};invalid.left=invalid.right+1;
  gate.rejects([&] {static_cast<void>(geometry->node_bounds(0,invalid));},"node-cell bounds accepted reversed cell");
}

void bounds_fixtures(SeedCellsGate& gate) {
  auto points=contact_eight();
  for (unsigned corner=0;corner<8;++corner)
    add_unique(points,{static_cast<std::uint16_t>((corner&1U)?39:27),
      static_cast<std::uint16_t>((corner&2U)?39:27),static_cast<std::uint16_t>((corner&4U)?33:21)});
  const auto index=mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points));
  const auto geometry=mhgp8::Q4LocalGeometry::make(mhgp8::Q34EdgeCover::make(index,{0,1}),mhgp8::Q4CenterDomainMode::Disk);
  const Cell single{0,0,-Cell::scale,-Cell::scale,Cell::max_depth,false,false};
  const auto bound=geometry->node_bounds(0,single);
  gate.require(Big(bound.minimum)==-108*Big(Cell::scale) && Big(bound.maximum)==324*Big(Cell::scale),
    "interior box minimum lost a genuine contact although every box corner is positive");
  const auto ball=SeedPlane(points[0],points[1]).ball(0,-Cell::scale);
  gate.require(ball.power(points[2]).numerator()==0,"interior-minimum fixture lost its shell seed");
  ++gate.interior_minimum_cases;
  check_seed_bounds(gate,points,{0,1});
  for (auto& p:points) p={p[2],p[0],p[1]};
  check_seed_bounds(gate,points,{1,0});++gate.axis_permutations;
  check_seed_bounds(gate,{{0,0,0},{65535,65534,65533},{65535,0,0},{0,65535,0},
    {0,0,65535},{65535,65535,65535},{32767,32768,32769}}, {0,1});++gate.extreme_cases;
}

static_assert(std::is_trivially_copyable_v<mhgp8::Q4SeedCellWork> &&
  sizeof(mhgp8::Q4SeedCellWork)==37*sizeof(u64));
static_assert(std::is_trivially_copyable_v<mhgp8::Q4LocalEdgeWork> &&
  sizeof(mhgp8::Q4LocalEdgeWork)==117*sizeof(u64));

void seed_cells_ledger(SeedCellsGate& gate,const mhgp8::Q4LocalEdgeWork& edge,
                       const mhgp8::Q4SeedCellWork& extra,mhgp8::Q4SeedCellOptions options,
                       std::size_t point_count) {
  using Mode=mhgp8::Q4SeedCellMode;
  gate.require(extra.queries==1 && extra.live_preparations+extra.whole_atlas_skips==1 &&
    extra.live_node_visits==(extra.live_preparations?edge.atlas.cells_created:0),
    "live summary did not visit each necessary atlas cell once");
  gate.require((extra.peak_live_bytes>0)==(extra.live_preparations!=0) &&
    extra.peak_total_buffer_bytes<=edge.peak_live_buffer_bytes,
    "seed-cell auxiliary memory is missing from total capacity peak");
  if (options.mode==Mode::Joined) {
    gate.require(extra.product_visits==extra.live_skipped_nodes+extra.product_seed_rejections+
      extra.positive_products+extra.negative_products+extra.uncertain_products &&
      extra.uncertain_products==extra.x_splits+extra.cell_splits+extra.terminal_pairs,
      "joined product partition does not account for every visited descriptor");
    gate.require(extra.terminal_pairs==extra.family_preparations+extra.family_cache_hits &&
      extra.terminal_pairs==edge.sweep.leaf_queries && extra.cache_misses==edge.point_tests,
      "joined cache/terminal counters disagree with actual seed tests or leaf sweeps");
    gate.require(extra.family_preparations==edge.sweep.seed_queries &&
      extra.family_preparations<=edge.seeds && edge.seeds<=point_count &&
      extra.cache_hits>=extra.invalid_cache_hits && extra.max_block_sites<=options.block_sites &&
      extra.peak_product_stack<=181,"joined cache repeated a family or exceeded its owned block/DFS bounds");
    gate.require(edge.sweep.query_visits==0 && edge.sweep.line_tests==0 && edge.sweep.line_skips==0 &&
      edge.sweep.seed_owner_tests==0,"joined terminal restarted the individual atlas traversal");
    gate.cache_hits+=extra.family_cache_hits;gate.family_builds+=extra.family_preparations;
    gate.positive_rejections+=extra.positive_products;gate.negative_rejections+=extra.negative_products;
    gate.seed_splits+=extra.x_splits;gate.cell_splits+=extra.cell_splits;gate.terminal_pairs+=extra.terminal_pairs;
    gate.max_stack=std::max(gate.max_stack,extra.peak_product_stack);
  }
  gate.all_dead_cases+=extra.whole_atlas_skips;
  if (extra.whole_atlas_skips!=0 && edge.atlas.splits!=0) ++gate.dead_branch_cases;
  gate.live_skips+=extra.live_skipped_nodes;
  ++gate.work_checks;
}

void seed_cells_fixture(SeedCellsGate& gate,const Points& points,Edge edge,std::size_t k,
                         mhgp8::Q4LocalOptions local,std::size_t block_sites) {
  const auto cover=mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points)),edge);
  edge=cover->edge_ids();
  const auto expected=seed_cells_oracle(gate,points,edge,k);
  Output old_output;
  const auto old=mhgp8::run_q4_local_edge_candidates(cover,k,local,[&](const auto& value) {old_output.push_back(copy(value));});
  normalize(old_output);
  gate.require(old_output==expected,"individual reference differs from independent rational support/depth/shell oracle");
  ++gate.reference_calls;
  for (const auto mode:{mhgp8::Q4SeedCellMode::Individual,mhgp8::Q4SeedCellMode::LiveOnly,mhgp8::Q4SeedCellMode::Joined}) {
    const mhgp8::Q4SeedCellOptions option{mode,block_sites};
    mhgp8::Q4SeedCellWork extra{};
    if (mode==mhgp8::Q4SeedCellMode::Individual) extra.queries=19;
    Output actual;
    const auto result=mhgp8::run_q4_local_edge_candidates(cover,k,local,[&](const auto& value) {actual.push_back(copy(value));},option,extra);
    normalize(actual);
    // Full payload oracle ALWAYS precedes counters, for causal compiled mutants.
    gate.require(actual==expected,"seed-cell stream differs from independent rational support/depth/shell oracle");
    gate.require(result.sweep.emitted==actual.size(),"seed-cell callback count differs");
    account_seed_cells(gate,actual);++gate.edge_calls;
    if (mode==mhgp8::Q4SeedCellMode::Individual) {
      mhgp8::Q4SeedCellWork untouched{};untouched.queries=19;
      gate.require(extra==untouched && std::bit_cast<std::array<u64,117>>(old)==
        std::bit_cast<std::array<u64,117>>(result),"Individual changed historical work or wrote the new ledger");
      ++gate.individual_calls;
    } else {
      if (mode==mhgp8::Q4SeedCellMode::LiveOnly) ++gate.live_calls;else ++gate.product_calls;
      if (k>=3) seed_cells_ledger(gate,result,extra,option,points.size());
      else {gate.require(extra==mhgp8::Q4SeedCellWork{},"inactive arity prepared live/cache work");++gate.inactive_calls;}
    }
  }
}

void traversal_fixtures(SeedCellsGate& gate) {
  const Points coincident{{15,20,20},{24,23,20},{20,15,20},{23,16,20},{20,20,14},{20,20,26}};
  const Points obtuse{{10,20,20},{30,20,20},{20,29,27},{20,11,22}};
  const auto tetra=oracle::make(select(obtuse,std::array<std::size_t,4>{0,1,2,3}));
  gate.require(tetra.ball.has_value() && !oracle::make(select(obtuse,Ids{0,1,3})).ball,
    "obtuse-completion fixture no longer has a positive tetrahedron and nonacute other face");
  ++gate.obtuse_completions;
  std::vector<Points> fixtures{
    {{10,10,10},{16,16,10},{16,10,16},{10,16,16}},contact_eight(),coincident,obtuse,
    {{900,1000,1000},{1100,1000,1000},{1000,1120,1040},{1000,1120,960},
      {1000,1020,1105},{1001,1020,1105}},shell_fixture(),
    {{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535},{32767,32767,32767}}};
  for (std::size_t f=0;f<fixtures.size();++f) {
    for (const auto domain:{mhgp8::Q4CenterDomainMode::Disk,mhgp8::Q4CenterDomainMode::Positive})
      for (const std::size_t block:{2U,64U}) {
        mhgp8::Q4LocalOptions local;
        local.domain=domain;local.max_depth=f==2?2U:4U;local.node_budget=341;
        local.leaf_sites=0;local.z_test_budget=32;
        seed_cells_fixture(gate,fixtures[f],{0,1},f==4?3U:5U,local,block);
      }
  }
  auto permuted=contact_eight();std::reverse(permuted.begin(),permuted.end());
  mhgp8::Q4LocalOptions local;local.max_depth=3;local.node_budget=85;local.leaf_sites=0;
  for (const auto k:{1U,2U,3U,10U}) seed_cells_fixture(gate,permuted,{6,7},k,local,1);
  ++gate.permutations;
  // Keep the owning edge IDs0/1, but make (36,30,24) the canonical seed.
  // Its line is 144*(1+xi+eta)=0. At the isolated centre (0,-1), the
  // owning depth2 cell [0,1]x[-1,0] lies entirely on its POSITIVE side.
  // Replacing min>0 by min>=0 therefore loses the only q4 presentation;
  // a full ID reversal instead changes the longest-edge ownership ties.
  auto positive_contact=contact_eight();std::swap(positive_contact[2],positive_contact[3]);
  auto contact_local=local;contact_local.domain=mhgp8::Q4CenterDomainMode::Disk;
  contact_local.max_depth=2;contact_local.node_budget=21;contact_local.z_test_budget=32;
  seed_cells_fixture(gate,positive_contact,{0,1},3,contact_local,64);++gate.permutations;
  // Uniform midpoint witness: every centre is deep at K3, but q3 remains an
  // independent lane in the global gate. Nothing is removed from witness Z.
  seed_cells_fixture(gate,{{10,10,10},{16,16,10},{16,10,16},{10,16,16},{13,13,10}},
                     {0,1},3,local,2);
  // Root is not uniformly deep; each first-level child is. The live summary
  // must discover a genuinely dead branch, not reinterpret it as a leaf.
  seed_cells_fixture(gate,{{10,20,20},{30,20,20},{20,25,20},{20,15,20},
    {20,29,27},{20,11,27},{20,29,13},{20,11,13}}, {0,1},3,local,2);
  auto shell=shell_fixture();shell.push_back({20,20,20});
  local.max_depth=0;local.node_budget=1;local.z_test_budget=0;
  seed_cells_fixture(gate,shell,{0,1},5,local,2);
  local.max_depth=Cell::max_depth;local.node_budget=85;local.z_test_budget=0;
  seed_cells_fixture(gate,fixtures.back(),{0,1},5,local,2);
  // Exhaustive edges of a tiny cloud, not enumeration in the product.
  for (std::size_t a=0;a<4;++a) for (std::size_t b=a+1;b<4;++b) {
    local.max_depth=2;local.node_budget=21;local.z_test_budget=32;
    seed_cells_fixture(gate,fixtures[0],{a,b},5,local,2);++gate.exhaustive_edges;
  }
}

void exact_cache_fixture(SeedCellsGate& gate) {
  const Points points{{15,20,20},{24,23,20},{20,15,20},{23,16,20},{20,20,14},{20,20,26}};
  const Edge edge{0,1};
  const auto cover=mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points)),edge);
  mhgp8::Q4LocalOptions local;local.domain=mhgp8::Q4CenterDomainMode::Disk;
  local.max_depth=2;local.node_budget=21;local.leaf_sites=0;local.z_test_budget=32;
  const auto atlas=mhgp8::Q4LocalAtlas::make(cover,5,local);
  u64 distinct=0,incidences=0;
  for (std::size_t x=2;x<points.size();++x) {
    const Ids ids{0,1,x};
    if (!oracle::make(select(points,ids)).ball || owner(points,ids)!=edge) continue;
    const auto reference=mhgp8::run_q4_local_seed_candidates(atlas,x,[](const auto&) {});
    distinct+=static_cast<u64>(reference.leaf_queries!=0);incidences+=reference.leaf_queries;
  }
  gate.require(distinct>=2 && incidences>distinct,"coincident seed fixture did not revisit families across cells");
  const auto expected=seed_cells_oracle(gate,points,edge,5);
  gate.require(!expected.empty(),"coincident seed fixture has no independently positive q4 payload");
  for (const std::size_t grain:{1U,2U,64U}) {
    mhgp8::Q4SeedCellWork extra{};Output actual;
    static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,5,local,[&](const auto& value) {
      actual.push_back(copy(value));},{mhgp8::Q4SeedCellMode::Joined,grain},extra));
    normalize(actual);
    gate.require(actual==expected,"seed-cell stream differs from independent rational support/depth/shell oracle");
    gate.require(extra.family_preparations==distinct && extra.terminal_pairs==incidences &&
      extra.family_cache_hits==incidences-distinct,
      "coincident families are not prepared exactly once per distinct seed reaching a leaf");
  }
  ++gate.cached_fixture;
}

void seed_cells_lifecycle(SeedCellsGate& gate) {
  using Mode=mhgp8::Q4SeedCellMode;
  const auto points=contact_eight();
  auto cloud=mhgp8::prepare_cloud(points);auto index=mhgp8::make_q2_cloud_index(cloud);
  auto cover=mhgp8::Q34EdgeCover::make(index,{0,1});
  mhgp8::Q4LocalOptions local;local.max_depth=3;local.node_budget=85;local.leaf_sites=0;
  const auto expected=seed_cells_oracle(gate,points,{0,1},5);
  const mhgp8::Q34SeedConsumer discard=[](const auto&) {};
  for (const auto mode:{Mode::Individual,Mode::LiveOnly,Mode::Joined}) {
    mhgp8::Q4SeedCellWork extra{};extra.queries=29;
    for (const auto k:{1U,5U}) {
      const auto before=extra;
      gate.rejects([&] {static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,k,local,discard,{mode,0},extra));},
        "invalid zero cache grain was accepted, possibly while inactive");
      gate.require(extra==before,"invalid cache grain changed extra work");
      gate.rejects([&] {static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,k,local,discard,{static_cast<Mode>(99),2},extra));},
        "invalid traversal enum was accepted, possibly while inactive");
      gate.require(extra==before,"invalid traversal mode changed extra work");
    }
  }
  for (const auto mode:{Mode::LiveOnly,Mode::Joined}) {
    mhgp8::Q4SeedCellWork work{};
    static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,5,local,discard,{mode,2},work));
    const auto first=std::bit_cast<std::array<u64,37>>(work);
    static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,5,local,discard,{mode,2},work));
    const auto twice=std::bit_cast<std::array<u64,37>>(work);
    for (std::size_t field=0;field<first.size();++field)
      gate.require(twice[field]==(field<30?2*first[field]:first[field]),"new work did not preserve SUM/MAX accumulation");
  }
  for (std::size_t before=0;before<4;++before) {
    mhgp8::Q4SeedCellWork work{};seed_cells_allocation::state={true,before};bool caught=false;
    try {static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,5,local,discard,{Mode::Joined,2},work));}
    catch (const std::bad_alloc&) {caught=true;}
    seed_cells_allocation::state.armed=false;
    gate.require(caught,"allocation failure did not propagate from seed-cell path");++gate.allocation_failures;
  }
  // Count only the same immutable atlas factory, then inject at the next
  // allocations: live summary, then first one-site cache. This targets new
  // owned buffers rather than claiming the first four atlas failures did so.
  seed_cells_allocation::state={false,0,true,0};
  const auto atlas=mhgp8::Q4LocalAtlas::make(cover,5,local);
  const auto atlas_allocations=seed_cells_allocation::state.calls;
  seed_cells_allocation::state.counting=false;
  gate.require(atlas->work().leaf_cells>0 && atlas_allocations>0,"late-allocation fixture has no live atlas");
  for (std::size_t offset=0;offset<2;++offset) {
    mhgp8::Q4SeedCellWork extra{};bool caught=false;
    seed_cells_allocation::state={true,atlas_allocations+offset,false,0};
    try {static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,5,local,discard,{Mode::Joined,1},extra));}
    catch (const std::bad_alloc&) {caught=true;}
    seed_cells_allocation::state.armed=false;
    gate.require(caught && extra.live_preparations==1,"late allocation did not reach live-summary construction");
    if (offset==0) gate.require(extra.peak_live_bytes==0 && extra.blocks==0,"summary-allocation fault occurred after its target");
    else gate.require(extra.live_node_visits==atlas->work().cells_created && extra.blocks>0 && extra.cache_entries_initialized==0,
      "late allocation did not target the first owned seed cache");
    ++gate.late_allocation_failures;
  }
  bool caught=false;
  mhgp8::Q4SeedCellWork work{};
  try {static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,5,local,[](const auto&) {
    throw std::logic_error("seed cells callback marker");},{Mode::Joined,2},work));}
  catch (const std::logic_error& e) {caught=std::string_view(e.what())=="seed cells callback marker";}
  gate.require(caught,"seed-cell callback failure was swallowed");++gate.callback_failures;
  std::array<std::future<Output>,4> futures;
  for (std::size_t i=0;i<futures.size();++i) futures[i]=std::async(std::launch::async,[cover,local,i] {
    Output output;mhgp8::Q4SeedCellWork extra{};
    static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,5,local,[&](const auto& v) {output.push_back(copy(v));},
      {i%2?Mode::Joined:Mode::LiveOnly,2},extra));normalize(output);return output;
  });
  for (auto& future:futures) {gate.require(future.get()==expected,"shared immutable cover changed concurrent full payload");++gate.parallel_calls;}
  Output actual;
  static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,5,local,[&](const auto& v) {
    actual.push_back(copy(v));if (cover) {cover.reset();index.reset();cloud.reset();++gate.owner_resets;}
  },{Mode::Joined,2},work));
  normalize(actual);gate.require(actual==expected && !cover && !index && !cloud,"callback owner reset lost seed-cell payload");
}

}  // namespace

int main(int argc,char** argv) {
  try {
    if (argc!=2 || std::string_view(argv[1])!="--selftest") throw std::invalid_argument("expected --selftest");
    SeedCellsGate gate;bounds_fixtures(gate);traversal_fixtures(gate);exact_cache_fixture(gate);seed_cells_lifecycle(gate);
    gate.require(gate.candidates>0 && gate.max_shell>=30 && gate.cache_hits>0 && gate.all_dead_cases>0 &&
      gate.dead_branch_cases>0 && gate.positive_rejections>0 && gate.negative_rejections>0 &&
      gate.seed_splits>0 && gate.cell_splits>0 && gate.terminal_pairs>0 && gate.bounds_rounding>0 &&
      gate.bounds_wide>0 && gate.bounds_contacts>0 && gate.q4_without_q3>0,"seed-cell gate nonvacuity failed");
    std::cout<<"{\"schema\":\"mhgp8_q4_seed_cells_gate_v1\",\"status\":\"PASS\"";
#define EMIT(field) std::cout<<",\"" #field "\":"<<gate.field
    EMIT(checks);EMIT(bounds_queries);EMIT(bounds_corners);EMIT(bounds_site_checks);EMIT(bounds_rounding);EMIT(bounds_wide);
    EMIT(bounds_contacts);EMIT(interior_minimum_cases);EMIT(axis_permutations);EMIT(extreme_cases);
    EMIT(edge_calls);EMIT(reference_calls);EMIT(oracle_completions);EMIT(oracle_sites);EMIT(candidates);EMIT(q4);EMIT(max_shell);
    EMIT(individual_calls);EMIT(live_calls);EMIT(product_calls);EMIT(work_checks);EMIT(cache_hits);
    EMIT(all_dead_cases);EMIT(dead_branch_cases);EMIT(live_skips);EMIT(positive_rejections);EMIT(negative_rejections);
    EMIT(seed_splits);EMIT(cell_splits);EMIT(terminal_pairs);EMIT(family_builds);EMIT(max_stack);
    EMIT(cached_fixture);EMIT(obtuse_completions);EMIT(q4_without_q3);EMIT(inactive_calls);EMIT(exhaustive_edges);EMIT(permutations);
    EMIT(invalid_inputs);EMIT(allocation_failures);EMIT(late_allocation_failures);EMIT(callback_failures);EMIT(parallel_calls);EMIT(owner_resets);
#undef EMIT
    std::cout<<"}\n";return 0;
  } catch (const std::exception& error) {
    seed_cells_allocation::state.armed=false;
    std::cerr<<"q4 seed cells gate: "<<error.what()<<'\n';return 1;
  }
}
