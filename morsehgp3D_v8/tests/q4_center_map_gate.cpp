// Explicit helper reuse only; the former gate is not executed or inherited
// as qualification. Rational Gram/census, canonical ownership and payload
// copying come from the test-only helper path, never from audit A's model.
#define main mhgp8_pruning_gate_helpers_for_center_map
#include "q34_family_pruning_gate.cpp"
#undef main

#include "lanes/q4_center_map.hpp"

#include <cstdlib>
#include <new>

// Explicit adaptation of the test-only allocator in q2_census_detach_gate.
// Armed only around ONE private map query, never a callback/assertion. This
// exercises the documented poisoned-cache state without a product test hook.
namespace center_map_allocation {
struct State { bool armed{}, hit{}; std::size_t remaining{}; };
thread_local State state;
[[gnu::noinline]] void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
  if (state.armed) {
    if (state.remaining == 0) { state.armed = false; state.hit = true; throw std::bad_alloc(); }
    --state.remaining;
  }
  void* pointer = nullptr;
  if (alignment <= alignof(std::max_align_t)) pointer = std::malloc(size == 0 ? 1 : size);
  else if (posix_memalign(&pointer,alignment,size == 0 ? 1 : size) != 0) pointer = nullptr;
  if (pointer == nullptr) throw std::bad_alloc();
  return pointer;
}
[[gnu::noinline]] void release(void* pointer) noexcept { std::free(pointer); }
}
void* operator new(std::size_t size) { return center_map_allocation::allocate(size); }
void* operator new[](std::size_t size) { return center_map_allocation::allocate(size); }
void* operator new(std::size_t size, std::align_val_t alignment) { return center_map_allocation::allocate(size,static_cast<std::size_t>(alignment)); }
void* operator new[](std::size_t size, std::align_val_t alignment) { return center_map_allocation::allocate(size,static_cast<std::size_t>(alignment)); }
void operator delete(void* pointer) noexcept { center_map_allocation::release(pointer); }
void operator delete[](void* pointer) noexcept { center_map_allocation::release(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { center_map_allocation::release(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { center_map_allocation::release(pointer); }
void operator delete(void* pointer, std::align_val_t) noexcept { center_map_allocation::release(pointer); }
void operator delete[](void* pointer, std::align_val_t) noexcept { center_map_allocation::release(pointer); }
void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept { center_map_allocation::release(pointer); }
void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept { center_map_allocation::release(pointer); }

namespace {

static_assert(!std::is_copy_constructible_v<mhgp8::Q4PositiveDomain>);
static_assert(!std::is_move_constructible_v<mhgp8::Q4PositiveDomain>);
static_assert(!std::is_copy_constructible_v<mhgp8::Q4CenterMap>);
static_assert(!std::is_move_constructible_v<mhgp8::Q4CenterMap>);

struct MapGate : Gate {
  u64 domains{}, domain_sites{}, empty_domains{}, obtuse_completions{}, lens_boundary{};
  u64 outside_lens_inside{}, admitted_domain_blocks{}, rejected_domain_blocks{};
  u64 maps{}, queries{}, rejected_queries{}, unknown_queries{}, requeries{}, order_trials{};
  u64 positive_presentations{}, pool_depth_checks{}, shallow_pool_roots{}, disk_contacts{};
  u64 line_skips{}, witness_tests{}, inside_credits{}, outside_witnesses{};
  u64 outside_cells{}, deep_cells{}, splits{}, compressed_deep{}, compressed_outside{};
  u64 depth_stops{}, budget_stops{}, empty_stops{}, max_depth{}, rotations{}, extreme_calls{};
  u64 map_q3_only{}, map_both_rejected{}, bypass_calls{}, deferred_calls{};
  u64 allocation_failures{};
};

std::vector<u64> map_filter_words(const mhgp8::Q34PoolWork& w) {
  return {w.seed_owner_tests,w.seed_owner_rejections,w.seed_queries,w.certificate_builds,
    w.sqrt_iterations,w.variance_bounds,w.variance_sqrt_iterations,w.proposed_sites,
    w.paired_predicate_tests,w.q3_credits,w.q4_universal_credits,w.q3_rejected,
    w.q4_universal_rejected,w.collective_queries,w.endpoint_tests,w.constant_tests,
    w.event_count,w.sort_comparisons,w.group_comparisons,w.event_side_tests,w.groups,
    w.max_group,w.collective_minimum_sum,w.collective_q4_rejected,w.q4_rejected,
    w.both_rejected,w.q3_only_survivors,w.q4_only_survivors,w.both_survivors,w.peak_event_bytes};
}

void check_domain(MapGate& gate, const Points& points, const mhgp8::Q34EdgeCoverPtr& cover) {
  const auto domain = mhgp8::Q4PositiveDomain::make(cover);
  const auto edge = cover->edge_ids();
  const auto d = distance_squared(points[edge[0]],points[edge[1]]);
  std::vector<std::size_t> ids;
  for (std::size_t z = 0; z != points.size(); ++z) {
    if (z == edge[0] || z == edge[1]) continue;
    const auto da = distance_squared(points[z],points[edge[0]]);
    const auto db = distance_squared(points[z],points[edge[1]]);
    if (da > d || db > d) continue;
    ids.push_back(z);
    if (da == d || db == d) ++gate.lens_boundary;
    if (da+db < d && oracle::make(select(points,Ids{edge[0],edge[1],z}),false).ball)
      ++gate.obtuse_completions;
  }
  gate.require(domain->cover().get() == cover.get() && domain->completion_count() == ids.size(),
               "positive domain changed owner or closed-lens population");
  gate.require(domain->completion_box().has_value() == !ids.empty(),
               "positive-domain emptiness differs from exhaustive lens");
  if (ids.empty()) ++gate.empty_domains;
  else {
    std::array<std::uint16_t,3> low{65535,65535,65535},high{};
    for (const auto id : ids)
      for (std::size_t axis = 0; axis != 3; ++axis) {
        low[axis] = std::min(low[axis],points[id][axis]);
        high[axis] = std::max(high[axis],points[id][axis]);
      }
    for (std::size_t axis = 0; axis != 3; ++axis)
      gate.require(domain->completion_box()->low[axis] == low[axis] &&
                   domain->completion_box()->high[axis] == high[axis],
                   "positive domain AABB excludes an obtuse/tied completion or retains endpoint extrema");
  }
  const auto& w = domain->work();
  gate.require(w.node_visits == w.bound_tests+w.endpoint_leaf_tests &&
    w.node_visits == w.admitted_nodes+w.rejected_nodes+w.split_nodes+w.excluded_endpoints &&
    w.endpoint_leaf_tests == w.point_tests+w.excluded_endpoints &&
    w.admitted_sites+w.rejected_sites+w.excluded_endpoints == points.size() &&
    w.excluded_endpoints == 2 && w.admitted_sites == ids.size() &&
    w.box_merges == (ids.empty() ? 0 : w.admitted_nodes-1) &&
    w.endpoint_box_tests%2 == 0 && w.endpoint_box_tests <= 2*w.bound_tests,
    "positive-domain disjoint block accounting");
  gate.admitted_domain_blocks += w.admitted_nodes;
  gate.rejected_domain_blocks += w.rejected_nodes;
  gate.domain_sites += ids.size();
  ++gate.domains;
}

struct PositiveRoot { oracle::Ball ball; std::size_t pool_depth{}, global_depth{}; };
std::vector<PositiveRoot> positive_roots(MapGate& gate, const Points& points, Edge edge,
    std::size_t x, const mhgp8::Q34WitnessPoolPtr& pool) {
  std::vector<PositiveRoot> roots;
  const auto diameter = distance_squared(points[edge[0]],points[edge[1]]);
  for (std::size_t y = 0; y != points.size(); ++y) {
    if (y == edge[0] || y == edge[1] || y == x) continue;
    const std::array<std::size_t,4> ids{edge[0],edge[1],x,y};
    const auto ball = oracle::make(select(points,ids));
    if (!ball.ball || owner(points,ids) != edge) continue;
    PositiveRoot root{*ball.ball,0,0};
    Rational t_norm(0);
    for (std::size_t axis = 0; axis != 3; ++axis) {
      const Rational t = Rational(2)*root.ball.center[axis] -
        Rational(Big(points[edge[0]][axis])+points[edge[1]][axis]);
      t_norm += t*t;
    }
    gate.require(Rational(2)*t_norm <= Rational(diameter), "rational positive center escaped edge disk");
    if (Rational(2)*t_norm == Rational(diameter)) ++gate.disk_contacts;
    for (const auto id : pool->ids()) if (root.ball.power(points[id]).numerator() < 0) ++root.pool_depth;
    for (std::size_t id = 0; id != points.size(); ++id) {
      if (root.ball.power(points[id]).numerator() >= 0) continue;
      ++root.global_depth;
      if (distance_squared(points[id],points[edge[0]]) > diameter ||
          distance_squared(points[id],points[edge[1]]) > diameter) ++gate.outside_lens_inside;
    }
    roots.push_back(std::move(root));
    ++gate.positive_presentations;
  }
  return roots;
}

std::vector<std::size_t> seeds(const Points& points, Edge edge) {
  std::vector<std::size_t> result;
  for (std::size_t x = 0; x != points.size(); ++x) {
    if (x == edge[0] || x == edge[1]) continue;
    const Ids ids{edge[0],edge[1],x};
    if (oracle::make(select(points,ids)).ball && owner(points,ids) == edge) result.push_back(x);
  }
  return result;
}

void accumulate_map(MapGate& gate, const mhgp8::Q4CenterMapWork& w) {
  gate.line_skips += w.line_skips; gate.witness_tests += w.witness_tests;
  gate.inside_credits += w.inside_credits; gate.outside_witnesses += w.outside_witnesses;
  gate.outside_cells += w.outside_cells; gate.deep_cells += w.deep_cells;
  gate.splits += w.splits; gate.compressed_deep += w.compressed_deep;
  gate.compressed_outside += w.compressed_outside; gate.depth_stops += w.depth_stops;
  gate.budget_stops += w.budget_stops; gate.empty_stops += w.empty_stops;
  gate.max_depth = std::max(gate.max_depth,w.max_depth);
}

void direct_map(MapGate& gate, const Points& points, Edge edge, std::size_t k,
    std::size_t budget, mhgp8::Q4CenterMapOptions options, bool reverse_order = false) {
  const auto cloud = mhgp8::prepare_cloud(points);
  const auto cover = mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(cloud),edge);
  const auto pool = mhgp8::Q34WitnessPool::make(cover,budget);
  edge = cover->edge_ids();
  auto ids = seeds(points,edge);
  if (reverse_order) { std::reverse(ids.begin(),ids.end()); ++gate.order_trials; }
  std::map<std::size_t,std::vector<PositiveRoot>> wanted;
  for (const auto x : ids) wanted.emplace(x,positive_roots(gate,points,edge,x,pool));
  auto map = mhgp8::Q4CenterMap::make(pool,k,options);
  gate.require(map->pool().get() == pool.get(), "map copied or changed its immutable pool");
  gate.require(map->work().preparations == (options.node_budget == 0 ? 0 : 1) &&
               map->work().prepared_forms == (options.node_budget == 0 ? 0 : pool->ids().size()),
               "map did not prepare exactly its pool once");
  std::map<std::size_t,bool> previous;
  for (unsigned pass = 0; pass != 2; ++pass) {
    for (const auto x : ids) {
      const bool rejected = map->reject_seed(x);
      ++gate.queries;
      if (rejected) ++gate.rejected_queries; else ++gate.unknown_queries;
      if (pass != 0) {
        gate.require(!previous[x] || rejected, "global map certificate became invalid after later queries");
        ++gate.requeries;
      }
      for (const auto& root : wanted.at(x)) {
        gate.require(!rejected || root.pool_depth >= k-2,
                     "map rejected a positive owner root with insufficient distinct pool depth");
        ++gate.pool_depth_checks;
        if (root.pool_depth < k-2) ++gate.shallow_pool_roots;
      }
      previous[x] = rejected;
      gate.require(map->work().max_depth <= options.max_depth &&
        map->work().cells_created <= options.node_budget,
        "map refinement exceeded its certificate resources");
    }
  }
  const auto& w = map->work();
  gate.require(w.queries == 2*ids.size() && w.rejected_queries+w.unknown_queries == w.queries &&
               w.seed_owner_rejections == 0 && w.line_tests <= w.query_visits &&
               w.line_skips <= w.line_tests && w.cells_evaluated <= w.cells_created,
               "map query/evaluation ledger");
  gate.require(w.peak_retained_bytes >= map->retained_bytes(), "map retained capacity peak understated");
  if (options.node_budget == 0)
    gate.require(w.cells_created == 0 && w.witness_tests == 0 && w.rejected_queries == 0,
                 "disabled standalone map did certificate work");
  accumulate_map(gate,w);
  ++gate.maps;
}

void mapped_edge(MapGate& gate, const Points& points, Edge edge, std::size_t k,
    std::size_t budget, mhgp8::Q34PoolOptions filter, mhgp8::Q4CenterMapOptions options) {
  gate.require(points.size() <= 40, "map oracle exceeded bounded small-cloud domain");
  const auto cloud = mhgp8::prepare_cloud(points);
  const auto cover = mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(cloud),edge);
  const auto pool = mhgp8::Q34WitnessPool::make(cover,budget);
  edge = cover->edge_ids();
  Output all_expected;
  for (const auto x : seeds(points,edge)) {
    const auto expected = oracle_seed(gate,points,edge,x,k);
    Output actual,old_output;
    const auto old = mhgp8::run_q34_collective_seed_candidates(pool,x,k,filter,
      [&](const auto& item) { old_output.push_back(copy(item)); });
    const auto work = mhgp8::run_q34_mapped_seed_candidates(pool,x,k,filter,options,
      [&](const auto& item) { actual.push_back(copy(item)); });
    normalize(actual); normalize(old_output);
    gate.require(actual == expected && old_output == expected,
                 "mapped seed differs from rational complete ball/depth/support/shell oracle");
    gate.require(map_filter_words(work.collective.filter) == map_filter_words(old.filter),
                 "map changed the preceding independent pool filter");
    const bool queried = k >= 3 && options.node_budget != 0 && old.filter.q4_rejected == 0;
    gate.require(work.map.queries == static_cast<u64>(queried) &&
      work.map.preparations == static_cast<u64>(queried), "map was not deferred until a live q4 lane");
    gate.require(work.q3_only_after_map+work.both_rejected_by_map == work.map.rejected_queries,
                 "map q3/q4 lane partition");
    if (work.q3_only_after_map != 0)
      gate.require(old.filter.q3_rejected == 0 && work.collective.covered.seed.family.sites == 0,
                   "map q4 rejection incorrectly coupled the q3 lane");
    if (work.both_rejected_by_map != 0)
      gate.require(old.filter.q3_rejected == 1 && work.collective.covered.site_reads == 0,
                   "fully rejected family still entered exact census");
    if (!queried) {
      gate.require(covered_signature(work.collective.covered) == covered_signature(old.covered),
                   "bypassed/deferred map changed baseline work");
      ++gate.deferred_calls;
    }
    if (options.node_budget == 0) ++gate.bypass_calls;
    gate.require(work.peak_live_buffer_bytes >= work.collective.peak_live_buffer_bytes &&
      work.peak_live_buffer_bytes >= work.map.peak_retained_bytes,
      "mapped seed simultaneous-buffer peak understated");
    gate.map_q3_only += work.q3_only_after_map;
    gate.map_both_rejected += work.both_rejected_by_map;
    all_expected.insert(all_expected.end(),expected.begin(),expected.end());
    ++gate.seed_calls; ++gate.reference_calls;
  }
  normalize(all_expected);
  Output actual,baseline;
  const auto work = mhgp8::run_q34_mapped_edge_candidates(pool,k,filter,options,
    [&](const auto& item) { actual.push_back(copy(item)); });
  const auto old = mhgp8::run_q34_collective_edge_candidates(pool,k,filter,
    [&](const auto& item) { baseline.push_back(copy(item)); });
  normalize(actual); normalize(baseline);
  gate.require(actual == all_expected && baseline == all_expected,
               "mapped edge lost a canonical seed candidate or exact payload");
  gate.require(generator_signature(work.collective.edge) == generator_signature(old.edge) &&
    map_filter_words(work.collective.filter) == map_filter_words(old.filter),
    "shared lazy map changed seed generation or pool-filter work");
  gate.require(work.map.preparations <= 1 &&
    work.q3_only_after_map+work.both_rejected_by_map == work.map.rejected_queries,
    "edge did not share a single map or lost lane classification");
  if (options.node_budget == 0)
    gate.require(covered_signature(work.collective.edge.covered) == covered_signature(old.edge.covered) &&
      work.map.preparations == 0 && work.map.queries == 0,
      "zero-node-budget edge changed reference counters");
  for (const auto& item : actual) {
    if (item.arity == 3) ++gate.q3; else ++gate.q4;
    gate.max_shell = std::max(gate.max_shell,static_cast<u64>(item.shell.size()));
  }
  gate.candidates += actual.size();
  gate.map_q3_only += work.q3_only_after_map; gate.map_both_rejected += work.both_rejected_by_map;
  gate.require(Points(cloud->points().begin(),cloud->points().end()) == points, "map modified cloud coordinates");
  ++gate.edge_calls;
}

Points rotated(Points points) {
  constexpr std::array<std::array<int,3>,3> matrix{{{-20,4,22},{20,-10,20},{10,28,4}}};
  for (auto& point : points) {
    const std::array<int,3> p{static_cast<int>(point.x)-20,static_cast<int>(point.y)-20,static_cast<int>(point.z)-20};
    std::array<std::uint16_t,3> output{};
    for (std::size_t row = 0; row != 3; ++row) {
      int value = 1000;
      for (std::size_t col = 0; col != 3; ++col) value += matrix[row][col]*p[col];
      if (value < 0 || value > 65535) throw std::runtime_error("rotation fixture outside u16");
      output[row] = static_cast<std::uint16_t>(value);
    }
    point = {output[0],output[1],output[2]};
  }
  return points;
}

void center_map_fixtures(MapGate& gate) {
  const Points twin{{20,20,20},{26,26,20},{26,20,26},{20,26,26},{26,20,14},{20,26,14}};
  const Points both{{10,10,10},{12,12,10},{12,10,12},{12,10,10},{10,10,11}};
  const Points q3_survives{{10,10,10},{16,16,10},{16,10,16},{14,8,9},{11,13,14}};
  const Points obtuse{{10,20,20},{30,20,20},{20,29,27},{20,11,22}};
  const Points outside_lens{{0,0,0},{20,20,0},{20,0,20},{0,20,20},{19,19,19}};
  const Points coplanar{{10,10,10},{20,10,10},{15,17,10},{12,16,10},{18,16,10}};
  const Points extreme{{0,0,0},{65535,65534,65533},{0,65535,65535},{65535,0,65535}};
  const Points tangent{{10,10,10},{13,14,10},{15,10,10},{10,15,10},{0,0,0}};
  const Points singleton{{20,20,20},{26,26,20},{26,20,26}};
  const Points compression{{10,20,20},{30,20,20},{20,25,20},{20,15,20},
    {20,29,27},{20,11,27},{20,29,13},{20,11,13}};
  const auto spun = rotated(twin);
  auto flipped = spun; std::swap(flipped[0],flipped[1]);
  for (const auto& points : {twin,both,q3_survives,obtuse,outside_lens,coplanar,extreme,tangent,singleton,compression,spun,flipped,
                            Points{{0,0,0},{1,0,0}},Points{{0,0,0},{2,0,0},{1,0,0}}}) {
    const auto cover = mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points)),{0,1});
    check_domain(gate,points,cover);
  }
  constexpr mhgp8::Q34PoolOptions universal{};
  constexpr mhgp8::Q34PoolOptions collective{mhgp8::Q34ChordBound::Variance,mhgp8::Q34PoolReduction::Collective};
  for (const auto mode : {mhgp8::Q4CenterDomainMode::Disk,mhgp8::Q4CenterDomainMode::Positive}) {
    const mhgp8::Q4CenterMapOptions full{mode,7,1024};
    for (const auto& points : {twin,both,q3_survives,obtuse,outside_lens,coplanar,spun,flipped}) {
      direct_map(gate,points,{0,1},3,32,full);
      direct_map(gate,points,{0,1},3,32,full,true);
      mapped_edge(gate,points,{0,1},3,32,universal,full);
    }
    for (const auto nodes : {0U,1U,5U}) {
      direct_map(gate,twin,{0,1},3,32,{mode,7,nodes});
      mapped_edge(gate,q3_survives,{0,1},3,32,universal,{mode,7,nodes});
    }
    direct_map(gate,twin,{0,1},3,0,{mode,7,1024});
    direct_map(gate,both,{0,1},3,32,{mode,0,1024});
    direct_map(gate,compression,{0,1},3,32,full);
    direct_map(gate,extreme,{0,1},3,32,{mode,44,85}); ++gate.extreme_calls;
    mapped_edge(gate,extreme,{0,1},3,32,universal,{mode,44,85}); ++gate.extreme_calls;
    mapped_edge(gate,singleton,{0,1},3,32,universal,full);
    mapped_edge(gate,shell30(),{0,1},5,32,collective,full);
    for (const auto k : {1U,2U,5U,10U}) mapped_edge(gate,q3_survives,{0,1},k,32,collective,full);
  }
  gate.rotations += 2;
  for (std::size_t a = 0; a != twin.size(); ++a)
    for (std::size_t b = a+1; b != twin.size(); ++b) {
      mapped_edge(gate,twin,{a,b},3,3,universal,{mhgp8::Q4CenterDomainMode::Positive,5,128});
      ++gate.exhaustive_edges;
    }
}

void allocation_poison(MapGate& gate) {
  const Points points{{10,20,20},{30,20,20},{20,25,20},{20,15,20},
    {20,29,27},{20,11,27},{20,29,13},{20,11,13}};
  const auto pool = mhgp8::Q34WitnessPool::make(mhgp8::Q34EdgeCover::make(
    mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points)),{0,1}),32);
  for (const auto position : {0U,1U,3U,5U}) {
    auto map = mhgp8::Q4CenterMap::make(pool,3,{mhgp8::Q4CenterDomainMode::Disk,7,128});
    bool caught = false;
    center_map_allocation::state = {true,false,position};
    try { static_cast<void>(map->reject_seed(4)); }
    catch (const std::bad_alloc&) { caught = true; }
    catch (...) { center_map_allocation::state.armed = false; throw; }
    center_map_allocation::state.armed = false;
    gate.require(caught && center_map_allocation::state.hit,
                 "map allocation-injection fixture did not reach its designated split allocation");
    gate.rejects<std::logic_error>([&] { static_cast<void>(map->reject_seed(4)); },
                                  "allocation-failed map resumed after a partial split");
    ++gate.allocation_failures;
  }
}

void center_map_lifecycle(MapGate& gate) {
  const Points points{{10,10,10},{12,12,10},{12,10,12},{10,12,12},{11,11,11}};
  auto cloud = mhgp8::prepare_cloud(points);
  auto index = mhgp8::make_q2_cloud_index(cloud);
  auto cover = mhgp8::Q34EdgeCover::make(index,{0,1});
  auto pool = mhgp8::Q34WitnessPool::make(cover,32);
  const mhgp8::Q4CenterMapOptions options{mhgp8::Q4CenterDomainMode::Positive,5,128};
  const mhgp8::Q34PoolOptions filter{};
  const mhgp8::Q34SeedConsumer sink = [](const auto&) {};
  gate.rejects([&] { static_cast<void>(mhgp8::Q4PositiveDomain::make({})); }, "null positive domain accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::Q4CenterMap::make({},3,options)); }, "null map pool accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::Q4CenterMap::make(pool,2,options)); }, "map accepted inactive q4 threshold");
  auto invalid = options; invalid.max_depth = 45;
  gate.rejects([&] { static_cast<void>(mhgp8::Q4CenterMap::make(pool,3,invalid)); }, "unproved arithmetic depth accepted");
  invalid = options; invalid.domain = static_cast<mhgp8::Q4CenterDomainMode>(99);
  gate.rejects([&] { mhgp8::validate_q4_center_map_options(invalid); }, "invalid center-domain mode accepted");
  auto map = mhgp8::Q4CenterMap::make(pool,3,options);
  gate.rejects<std::out_of_range>([&] { static_cast<void>(map->reject_seed(points.size())); }, "map outside seed ID accepted");
  gate.rejects([&] { static_cast<void>(map->reject_seed(0)); }, "map repeated endpoint accepted");
  gate.rejects([&] { static_cast<void>(map->reject_seed(4)); }, "map nonacute seed accepted");
  static_cast<void>(map->reject_seed(2));  // Invalid requests must not poison.
  {
    const auto foreign_pool = mhgp8::Q34WitnessPool::make(mhgp8::Q34EdgeCover::make(index,{1,2}),32);
    const auto foreign = mhgp8::Q4CenterMap::make(foreign_pool,3,options);
    gate.require(!foreign->reject_seed(0) && foreign->work().seed_owner_rejections == 1 &&
      foreign->work().witness_tests == 0, "foreign owner reached the mutable map certificate");
    const auto work = mhgp8::run_q34_mapped_seed_candidates(foreign_pool,0,3,filter,options,
      [&](const auto&) { throw std::runtime_error("foreign mapped seed emitted"); });
    gate.require(work.map.preparations == 0 && work.collective.covered.seed.seed_owner_rejections == 1,
                 "foreign wrapper prepared a map");
  }
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_mapped_edge_candidates(pool,3,filter,options,{})); }, "mapped null callback accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q34_mapped_seed_candidates(pool,2,0,filter,options,sink)); }, "mapped K0 accepted");
  struct Failure {};
  bool failed = false;
  try { static_cast<void>(mhgp8::run_q34_mapped_seed_candidates(pool,2,3,filter,options,
    [&](const auto&) { throw Failure{}; })); } catch (const Failure&) { failed = true; }
  gate.require(failed, "mapped callback exception swallowed"); ++gate.callback_failures;
  Output expected;
  {
    const auto execute = [pool,filter,options] {
      Output output;
      static_cast<void>(mhgp8::run_q34_mapped_edge_candidates(pool,3,filter,options,
        [&](const auto& item) { output.push_back(copy(item)); }));
      normalize(output); return output;
    };
    expected = execute();
    std::array<std::future<Output>,4> calls;
    for (auto& call : calls) call = std::async(std::launch::async,execute);
    for (auto& call : calls) { gate.require(call.get() == expected, "independent mapped calls shared mutable state"); ++gate.parallel_calls; }
  }
  map.reset();
  Output actual;
  static_cast<void>(mhgp8::run_q34_mapped_edge_candidates(pool,3,filter,options,[&](const auto& item) {
    pool.reset(); cover.reset(); index.reset(); cloud.reset(); actual.push_back(copy(item));
  }));
  normalize(actual);
  gate.require(actual == expected, "mapped callback reset invalidated owned parents");
}

int center_map_selftest() {
  MapGate gate;
  center_map_fixtures(gate);
  center_map_lifecycle(gate);
  allocation_poison(gate);
  gate.require(gate.empty_domains > 0 && gate.obtuse_completions > 0 && gate.lens_boundary > 0 &&
    gate.outside_lens_inside > 0 && gate.positive_presentations > 0 && gate.shallow_pool_roots > 0 &&
    gate.disk_contacts > 0 && gate.rejected_queries > 0 && gate.unknown_queries > 0,
    "domain/center-map geometric non-vacuity floors");
  gate.require(gate.line_skips > 0 && gate.splits > 0 && gate.witness_tests > 0 &&
    gate.depth_stops > 0 && gate.budget_stops > 0 && gate.empty_stops > 0 &&
    gate.map_q3_only > 0 && gate.map_both_rejected > 0 && gate.bypass_calls > 0 &&
    gate.q3 > 0 && gate.q4 > 0 && gate.max_shell >= 30,
    "lazy map and exact fallback non-vacuity floors");
  gate.require(gate.compressed_deep > 0 && gate.allocation_failures == 4,
               "compression and sticky allocation-failure non-vacuity floors");
  std::cout << "{\"schema\":\"mhgp8_q4_center_map_gate_v1\",\"status\":\"passed\"";
#define MHGP8_MAP_FIELD(name) std::cout << ",\"" #name "\":" << gate.name
  MHGP8_MAP_FIELD(checks); MHGP8_MAP_FIELD(domains); MHGP8_MAP_FIELD(domain_sites);
  MHGP8_MAP_FIELD(empty_domains); MHGP8_MAP_FIELD(obtuse_completions); MHGP8_MAP_FIELD(lens_boundary);
  MHGP8_MAP_FIELD(outside_lens_inside); MHGP8_MAP_FIELD(admitted_domain_blocks); MHGP8_MAP_FIELD(rejected_domain_blocks);
  MHGP8_MAP_FIELD(maps); MHGP8_MAP_FIELD(queries); MHGP8_MAP_FIELD(rejected_queries); MHGP8_MAP_FIELD(unknown_queries);
  MHGP8_MAP_FIELD(requeries); MHGP8_MAP_FIELD(order_trials); MHGP8_MAP_FIELD(positive_presentations);
  MHGP8_MAP_FIELD(pool_depth_checks); MHGP8_MAP_FIELD(shallow_pool_roots); MHGP8_MAP_FIELD(disk_contacts);
  MHGP8_MAP_FIELD(line_skips); MHGP8_MAP_FIELD(witness_tests); MHGP8_MAP_FIELD(inside_credits); MHGP8_MAP_FIELD(outside_witnesses);
  MHGP8_MAP_FIELD(outside_cells); MHGP8_MAP_FIELD(deep_cells); MHGP8_MAP_FIELD(splits);
  MHGP8_MAP_FIELD(compressed_deep); MHGP8_MAP_FIELD(compressed_outside); MHGP8_MAP_FIELD(depth_stops);
  MHGP8_MAP_FIELD(budget_stops); MHGP8_MAP_FIELD(empty_stops); MHGP8_MAP_FIELD(max_depth);
  MHGP8_MAP_FIELD(rotations); MHGP8_MAP_FIELD(extreme_calls); MHGP8_MAP_FIELD(map_q3_only); MHGP8_MAP_FIELD(map_both_rejected);
  MHGP8_MAP_FIELD(bypass_calls); MHGP8_MAP_FIELD(deferred_calls); MHGP8_MAP_FIELD(edge_calls); MHGP8_MAP_FIELD(seed_calls);
  MHGP8_MAP_FIELD(reference_calls); MHGP8_MAP_FIELD(oracle_completions); MHGP8_MAP_FIELD(oracle_sites);
  MHGP8_MAP_FIELD(candidates); MHGP8_MAP_FIELD(q3); MHGP8_MAP_FIELD(q4); MHGP8_MAP_FIELD(max_shell);
  MHGP8_MAP_FIELD(exhaustive_edges); MHGP8_MAP_FIELD(invalid_inputs); MHGP8_MAP_FIELD(callback_failures); MHGP8_MAP_FIELD(parallel_calls);
  MHGP8_MAP_FIELD(allocation_failures);
#undef MHGP8_MAP_FIELD
  std::cout << "}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try { return center_map_selftest(); }
  catch (const std::exception& error) {
    std::cerr << "q4 center map gate: " << error.what() << '\n';
    return 1;
  }
}
