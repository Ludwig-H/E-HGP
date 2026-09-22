// Explicit reuse of test-only rational Gram geometry and canonical-support
// helpers. The former gate is not executed, and no audit model is imported.
#define main mhgp8_pruning_gate_helpers_for_q4_local
#include "q34_family_pruning_gate.cpp"
#undef main

#include "lanes/q4_local.hpp"

#include <cstdlib>
#include <new>

// Test-only allocation failure, armed around one factory and disarmed before
// any assertion. No product hook or mutable shared geometry is introduced.
namespace local_allocation {
thread_local bool armed = false;
[[gnu::noinline]] void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
  if (armed) { armed = false; throw std::bad_alloc(); }
  void* pointer = nullptr;
  if (alignment <= alignof(std::max_align_t)) pointer = std::malloc(size == 0 ? 1 : size);
  else if (posix_memalign(&pointer,alignment,size == 0 ? 1 : size) != 0) pointer = nullptr;
  if (!pointer) throw std::bad_alloc();
  return pointer;
}
[[gnu::noinline]] void release(void* pointer) noexcept { std::free(pointer); }
}
void* operator new(std::size_t size) { return local_allocation::allocate(size); }
void* operator new[](std::size_t size) { return local_allocation::allocate(size); }
void* operator new(std::size_t size, std::align_val_t align) { return local_allocation::allocate(size,static_cast<std::size_t>(align)); }
void* operator new[](std::size_t size, std::align_val_t align) { return local_allocation::allocate(size,static_cast<std::size_t>(align)); }
void operator delete(void* p) noexcept { local_allocation::release(p); }
void operator delete[](void* p) noexcept { local_allocation::release(p); }
void operator delete(void* p, std::size_t) noexcept { local_allocation::release(p); }
void operator delete[](void* p, std::size_t) noexcept { local_allocation::release(p); }
void operator delete(void* p, std::align_val_t) noexcept { local_allocation::release(p); }
void operator delete[](void* p, std::align_val_t) noexcept { local_allocation::release(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { local_allocation::release(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { local_allocation::release(p); }

namespace {

struct LocalGate : Gate {
  u64 partitions{}, fragment_checks{}, active_blocks{}, active_sites{};
  u64 inactive_inside{}, inactive_outside{}, tangent_active{}, parent_checks{};
  u64 closed_roots{}, owned_roots{}, unowned_boundary_roots{}, corner_roots{};
  u64 root_cells{}, child_cells{}, exhausted_partitions{}, extreme_calls{};
  u64 clipped_events{}, clipped_inside{}, retained_events{}, constant_shells{}, depth_drops{};
  u64 clip_pairs{}, atlas_reuses{}, full_sweeps{}, empty_outputs{}, rotations{};
  u64 partition_inside{}, partition_outside{}, partition_splits{}, allocation_failures{};
  u64 refinements{}, refine_inside_added{};
};

static_assert(!std::is_copy_constructible_v<mhgp8::Q4LocalGeometry>);
static_assert(!std::is_move_constructible_v<mhgp8::Q4LocalGeometry>);
static_assert(!std::is_copy_constructible_v<mhgp8::Q4LocalFragment>);
static_assert(!std::is_move_constructible_v<mhgp8::Q4LocalFragment>);
static_assert(!std::is_copy_constructible_v<mhgp8::Q4LocalAtlas>);
static_assert(!std::is_move_constructible_v<mhgp8::Q4LocalAtlas>);

Rational local_fraction(Big numerator, Big denominator) {
  if (denominator == 0) throw std::runtime_error("local oracle singular fraction");
  if (denominator < 0) { numerator = -numerator; denominator = -denominator; }
  return Rational(std::move(numerator), std::move(denominator));
}

// Coordinates in the same stated integral chart, but powers are computed
// from a rational 3D centre and squared radius, not from product forms/bounds.
struct LocalPlane {
  Point3 a{}, b{};
  oracle::Vector v{}, first{}, second{};
  Big diameter{}, h{};
  std::size_t i{}, j{};

  LocalPlane(Point3 first_point, Point3 second_point) : a(first_point), b(second_point) {
    v = oracle::difference(b,a);
    diameter = oracle::dot(v,v);
    std::size_t dominant = 0;
    for (std::size_t axis = 1; axis != 3; ++axis)
      if (oracle::absolute(v[axis]) > oracle::absolute(v[dominant])) dominant = axis;
    h = oracle::absolute(v[dominant]);
    if (h == 0) throw std::runtime_error("local oracle duplicate edge");
    i = (dominant+1)%3; j = (dominant+2)%3;
    const Big sign = v[dominant] < 0 ? -1 : 1;
    first[i] = h; first[dominant] = -sign*v[i];
    second[j] = h; second[dominant] = -sign*v[j];
  }

  oracle::Ball at(const Rational& xi, const Rational& eta) const {
    oracle::Ball ball{};
    for (std::size_t axis = 0; axis != 3; ++axis) {
      ball.center[axis] = (Rational(Big(a[axis])+b[axis]) +
        Rational(first[axis])*xi + Rational(second[axis])*eta)/Rational(2);
      const Rational delta = ball.center[axis]-Rational(Big(a[axis]));
      ball.radius_squared += delta*delta;
    }
    return ball;
  }

  std::array<Rational,2> coordinates(const oracle::Ball& ball) const {
    return {(Rational(2)*ball.center[i]-Rational(Big(a[i])+b[i]))/Rational(h),
            (Rational(2)*ball.center[j]-Rational(Big(a[j])+b[j]))/Rational(h)};
  }
};

Output local_oracle(LocalGate& gate, const Points& points, Edge edge,
                    std::size_t x, std::size_t kmax) {
  auto output = oracle_seed(gate,points,edge,x,kmax);
  output.erase(std::remove_if(output.begin(),output.end(),[](const Candidate& candidate) {
    return candidate.arity != 4;
  }),output.end());
  return output;
}

std::vector<std::size_t> local_seeds(const Points& points, Edge edge) {
  std::vector<std::size_t> ids;
  for (std::size_t x = 0; x != points.size(); ++x) {
    if (x == edge[0] || x == edge[1]) continue;
    const Ids seed{edge[0],edge[1],x};
    if (oracle::make(select(points,seed)).ball && owner(points,seed) == edge) ids.push_back(x);
  }
  return ids;
}

void local_account(LocalGate& gate, const Output& output) {
  for (const auto& candidate : output) {
    gate.require(candidate.arity == 4, "q4-only path emitted a different arity");
    ++gate.candidates; ++gate.q4;
    gate.max_shell = std::max(gate.max_shell,static_cast<u64>(candidate.shell.size()));
  }
  if (output.empty()) ++gate.empty_outputs;
}

Output local_baseline(LocalGate& gate, const mhgp8::Q34EdgeCoverPtr& cover,
                      std::size_t x, std::size_t k) {
  Output output;
  static_cast<void>(mhgp8::run_q34_cover_seed_candidates(cover,x,k,[&](const auto& candidate) {
    if (candidate.arity == 4) output.push_back(copy(candidate));
  }));
  normalize(output);
  ++gate.reference_calls;
  return output;
}

void local_fixture(LocalGate& gate, const Points& points, Edge edge,
                   std::size_t k, mhgp8::Q4LocalOptions options) {
  const auto cloud = mhgp8::prepare_cloud(points);
  const auto cover = mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(cloud),edge);
  edge = cover->edge_ids();
  const auto seeds_for_edge = local_seeds(points,edge);
  std::map<std::size_t,Output> expected;
  Output all_expected;
  for (const auto x : seeds_for_edge) {
    auto wanted = local_oracle(gate,points,edge,x,k);
    gate.require(wanted == local_baseline(gate,cover,x,k),
                 "rational local oracle differs from original covered path");
    all_expected.insert(all_expected.end(),wanted.begin(),wanted.end());
    expected.emplace(x,std::move(wanted));
  }
  normalize(all_expected);
  std::array<Output,2> pair_output;
  for (unsigned clip = 0; clip != 2; ++clip) {
    options.clip_events = clip != 0;
    const auto atlas = mhgp8::Q4LocalAtlas::make(cover,k,options);
    gate.require(atlas->kmax() == k && atlas->options().clip_events == options.clip_events,
                 "atlas changed threshold or clipping option");
    gate.require(atlas->work().cells_created <= options.node_budget &&
                 atlas->work().max_depth <= options.max_depth,
                 "atlas exceeded requested refinement resources");
    Output all;
    for (const auto x : seeds_for_edge) {
      Output actual;
      const auto work = mhgp8::run_q4_local_seed_candidates(atlas,x,[&](const auto& candidate) {
        actual.push_back(copy(candidate));
      });
      normalize(actual);
      gate.require(actual == expected.at(x),
                   "local sweep lost a rational ball, depth, canonical support or complete shell");
      gate.require(work.emitted == actual.size(), "local emission ledger differs from callbacks");
      if (!options.clip_events)
        gate.require(work.clipped_events == 0 && work.clipped_inside == 0,
                     "unclipped reference converted an exterior root to a constant");
      gate.clipped_events += work.clipped_events;
      gate.clipped_inside += work.clipped_inside;
      gate.retained_events += work.kept_events;
      gate.constant_shells += work.constant_shell_ids;
      gate.depth_drops += work.exits;
      gate.unowned_boundary_roots += work.boundary_skips;
      local_account(gate,actual);
      all.insert(all.end(),actual.begin(),actual.end());
      ++gate.seed_calls; ++gate.atlas_reuses;
    }
    normalize(all);
    gate.require(all == all_expected, "shared immutable atlas changed seed results");
    Output edge_actual;
    const auto work = mhgp8::run_q4_local_edge_candidates(cover,k,options,[&](const auto& candidate) {
      edge_actual.push_back(copy(candidate));
    });
    normalize(edge_actual);
    gate.require(edge_actual == all_expected && work.seeds == seeds_for_edge.size(),
                 "local edge producer differs from all rational canonical acute seeds");
    gate.require(work.sweep.emitted == edge_actual.size(), "edge callback ledger differs");
    gate.require(work.peak_live_buffer_bytes >= work.atlas.retained_bytes,
                 "edge live memory omits retained atlas capacity");
    pair_output[clip] = std::move(edge_actual);
    ++gate.edge_calls; ++gate.full_sweeps;
  }
  gate.require(pair_output[0] == pair_output[1],
               "closed clipping changed q4 outputs relative to complete event sweep");
  ++gate.clip_pairs;
}

bool local_contains(const mhgp8::Q4LocalCell& cell,
                    const std::array<Rational,2>& point, bool closed) {
  const Rational q{Big(mhgp8::Q4LocalCell::scale)};
  const Rational left = Rational(Big(cell.left))/q, right = Rational(Big(cell.right))/q;
  const Rational bottom = Rational(Big(cell.bottom))/q, top = Rational(Big(cell.top))/q;
  return point[0] >= left && point[1] >= bottom &&
    (point[0] < right || ((closed || cell.owns_right) && point[0] == right)) &&
    (point[1] < top || ((closed || cell.owns_top) && point[1] == top));
}

std::set<std::size_t> local_expand(LocalGate& gate,
    const mhgp8::Q4LocalGeometryPtr& geometry, std::span<const std::size_t> blocks) {
  const auto& index = *geometry->cover()->index();
  const auto nodes = index.spatial_nodes();
  const auto order = index.spatial_order();
  std::set<std::size_t> ids;
  std::size_t previous_end = 0;
  bool first = true;
  for (const auto id : blocks) {
    gate.require(id < nodes.size(), "frontier node ID escaped global index");
    const auto range = nodes[id].range;
    gate.require(first || previous_end <= range.first, "frontier blocks overlap or lost spatial order");
    first = false; previous_end = range.last;
    for (auto rank = range.first; rank < range.last; ++rank) {
      const auto point_id = order[rank];
      gate.require(geometry->cover()->contains_id(point_id) && ids.insert(point_id).second,
                   "frontier duplicated an ID or escaped the exact cover");
    }
  }
  return ids;
}

void local_fragment(LocalGate& gate, const Points& points,
    const mhgp8::Q4LocalFragmentPtr& fragment,
    const mhgp8::Q4LocalFragmentPtr& parent = {}, u64 budget = 2048) {
  const auto geometry = fragment->geometry();
  const auto edge = geometry->cover()->edge_ids();
  const LocalPlane plane(points[edge[0]],points[edge[1]]);
  const auto active = local_expand(gate,geometry,fragment->active_nodes());
  gate.require(active.size() == fragment->active_sites(), "fragment active mass differs from disjoint nodes");
  gate.require(active.contains(edge[0]) && active.contains(edge[1]),
               "globally null edge endpoints disappeared from active shell");
  const auto cell = fragment->cell();
  const Rational q{Big(mhgp8::Q4LocalCell::scale)};
  std::array<oracle::Ball,4> corners;
  for (unsigned corner = 0; corner != 4; ++corner)
    corners[corner] = plane.at(local_fraction(Big(corner&1U ? cell.right : cell.left),Big(mhgp8::Q4LocalCell::scale)),
                               local_fraction(Big(corner&2U ? cell.top : cell.bottom),Big(mhgp8::Q4LocalCell::scale)));
  std::size_t inside = 0, outside = 0;
  for (std::size_t id = 0; id != points.size(); ++id) {
    if (!geometry->cover()->contains_id(id)) continue;
    Rational low = corners[0].power(points[id]), high = low;
    for (const auto& corner : corners) {
      const auto power = corner.power(points[id]);
      low = std::min(low,power); high = std::max(high,power);
    }
    const auto product_bounds = geometry->bounds(geometry->form(id),cell);
    gate.require(Rational(Big(product_bounds.minimum)) == Rational(4)*q*low &&
                 Rational(Big(product_bounds.maximum)) == Rational(4)*q*high,
                 "point form bounds differ from independent rational corner powers");
    if (active.contains(id)) {
      if (low.numerator() == 0 && high.numerator() > 0) ++gate.tangent_active;
      continue;
    }
    gate.require(high.numerator() < 0 || low.numerator() > 0,
                 "partition removed a tangent, shell or sign-changing site");
    if (high.numerator() < 0) ++inside; else ++outside;
  }
  gate.require(inside == fragment->inside_count(),
               "exact inherited count differs from uniformly interior inactive IDs");
  gate.require(inside+outside+active.size() == geometry->cover()->site_count(),
               "exact fragment failed to partition complete cover population");
  for (const auto& corner : corners) {
    std::size_t whole = 0, reconstructed = fragment->inside_count();
    std::set<std::size_t> whole_shell, active_shell;
    for (std::size_t id = 0; id != points.size(); ++id) {
      if (!geometry->cover()->contains_id(id)) continue;
      const auto power = corner.power(points[id]).numerator();
      if (power < 0) { ++whole; if (active.contains(id)) ++reconstructed; }
      if (power == 0) { whole_shell.insert(id); if (active.contains(id)) active_shell.insert(id); }
    }
    gate.require(whole == reconstructed && whole_shell == active_shell,
                 "fragment exact count or shell identity failed at closed corner");
  }
  const auto& work = fragment->work();
  gate.require(work.root_factories+work.child_factories+work.refine_factories == 1,
               "fragment factory origin ledger is not exclusive");
  gate.require(work.block_bound_tests+work.point_tests <= budget,
               "fragment exceeded geometric-test budget");
  gate.require(work.active_sites == active.size() && work.active_nodes == fragment->active_nodes().size(),
               "fragment frontier counters differ from retained nodes");
  gate.require(work.input_sites == work.inside_sites+work.outside_sites+work.active_sites &&
               fragment->inside_count() == work.inherited_inside_sites+work.inside_sites,
               "fragment new and inherited populations were mixed");
  gate.require(work.node_visits == work.inside_nodes+work.outside_nodes+work.active_nodes+work.z_splits &&
               work.node_visits == work.block_bound_tests+work.point_tests+work.budget_unexamined_nodes &&
               work.frontier_ids_copied == work.active_nodes && work.budget_ambiguous_nodes <= work.active_nodes,
               "fragment disjoint DFS/test/frontier ledger");
  if (parent) {
    const auto inherited = local_expand(gate,geometry,parent->active_nodes());
    gate.require(parent->geometry().get() == geometry.get() &&
                 fragment->inside_count() >= parent->inside_count() &&
                 std::includes(inherited.begin(),inherited.end(),active.begin(),active.end()),
                 "child changed owner, reintroduced inactive IDs or lost inherited count");
    gate.require(work.inherited_inside_sites == parent->inside_count(), "child inherited a lower bound instead of exact count");
    if (budget == 0)
      gate.require(inherited == active && fragment->inside_count() == parent->inside_count(),
                   "budget-zero child dropped or reclassified unresolved IDs");
    ++gate.parent_checks;
  } else if (budget == 0) {
    gate.require(active.size() == geometry->cover()->site_count() && fragment->inside_count() == 0,
                 "budget-zero root did not retain the entire covered population");
  }
  if (budget == 0) ++gate.exhausted_partitions;
  gate.active_blocks += fragment->active_nodes().size(); gate.active_sites += active.size();
  gate.inactive_inside += inside; gate.inactive_outside += outside;
  gate.partition_inside += work.inside_sites; gate.partition_outside += work.outside_sites;
  gate.partition_splits += work.z_splits;
  ++gate.fragment_checks;
}

void local_partition_fixture(LocalGate& gate, const Points& points, Edge edge,
    mhgp8::Q4CenterDomainMode mode, u64 budget) {
  const auto cover = mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points)),edge);
  const auto geometry = mhgp8::Q4LocalGeometry::make(cover,mode);
  const auto all = local_expand(gate,geometry,geometry->cover_nodes());
  gate.require(all.size() == cover->site_count() && geometry->cover().get() == cover.get(),
               "geometry changed immutable cover or incomplete node decomposition");
  const auto& geometry_work = geometry->work();
  gate.require(geometry_work.preparations == 1 && geometry_work.cover_sites == cover->site_count() &&
               geometry_work.cover_sites+geometry_work.cover_excluded_sites == points.size() &&
               geometry_work.cover_node_visits == geometry_work.cover_disjoint_nodes+geometry_work.cover_splits+geometry_work.cover_blocks &&
               geometry_work.cover_blocks == geometry_work.cover_node_ids_copied &&
               geometry_work.cover_blocks == geometry->cover_nodes().size(),
               "geometry cover decomposition ledger");
  auto root = mhgp8::Q4LocalFragment::root(geometry,budget);
  local_fragment(gate,points,root,{},budget);
  ++gate.root_cells;
  std::array<mhgp8::Q4LocalFragmentPtr,4> children;
  for (unsigned quadrant = 0; quadrant != 4; ++quadrant) {
    children[quadrant] = mhgp8::Q4LocalFragment::child(root,quadrant,budget);
    local_fragment(gate,points,children[quadrant],root,budget);
    ++gate.child_cells;
  }
  // Half-open ownership must partition the root, including corners and both
  // internal separators; closed classification alone deliberately overlaps.
  const std::array<Rational,5> samples{Rational(-2),Rational(-1),Rational(0),Rational(1),Rational(2)};
  for (const auto& xi : samples) for (const auto& eta : samples) {
    const std::array<Rational,2> point{xi,eta};
    unsigned owners = 0;
    for (const auto& child : children) if (local_contains(child->cell(),point,false)) ++owners;
    gate.require(owners == 1, "dyadic right/top ownership duplicated or omitted a boundary centre");
  }
  // Refine all quadrants one more time; each child classifies only its
  // inherited frontier, while the oracle always consults the original cover.
  for (const auto& parent : children)
    for (unsigned quadrant = 0; quadrant != 4; ++quadrant) {
      const auto child = mhgp8::Q4LocalFragment::child(parent,quadrant,budget);
      local_fragment(gate,points,child,parent,budget);
      ++gate.child_cells;
    }
  const auto refine_parent = children[3];
  const auto parent_work = refine_parent->work();
  const auto parent_nodes = std::vector<std::size_t>(refine_parent->active_nodes().begin(),refine_parent->active_nodes().end());
  for (const auto refine_budget : {u64{0},u64{2048}}) {
    const auto refined = mhgp8::Q4LocalFragment::refine(refine_parent,refine_budget);
    local_fragment(gate,points,refined,refine_parent,refine_budget);
    gate.require(refined->cell() == refine_parent->cell() && refined->work().refine_factories == 1,
                 "same-cell refinement changed geometry or ownership boundaries");
    gate.require(refine_parent->work() == parent_work &&
      std::vector<std::size_t>(refine_parent->active_nodes().begin(),refine_parent->active_nodes().end()) == parent_nodes,
      "same-cell refinement modified its immutable parent");
    if (refine_budget != 0) {
      gate.require(refined->work().budget_unexamined_nodes == 0 && refined->work().budget_ambiguous_nodes == 0,
                   "small complete refinement stopped before classifying all unresolved nodes");
      for (const auto id : refined->active_nodes())
        gate.require(cover->index()->spatial_nodes()[id].range.size() == 1,
                     "complete classification kept an unresolved multi-site node");
    }
    gate.refine_inside_added += refined->inside_count()-refine_parent->inside_count();
    ++gate.refinements;
  }
  ++gate.partitions;
}

void local_corner_fixture(LocalGate& gate) {
  const Points points{{10,10,10},{12,12,10},{10,12,8},{12,10,8}};
  const auto cover = mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(points)),{0,1});
  const auto geometry = mhgp8::Q4LocalGeometry::make(cover,mhgp8::Q4CenterDomainMode::Positive);
  auto fragment = mhgp8::Q4LocalFragment::root(geometry,2048);
  // Exactly [0,1/4] x [-1,-3/4]. The positive regular tetrahedron root
  // (0,-1) is its owned lower-left corner; y has minimum0 and maximum>0.
  for (const auto quadrant : {1U,2U,0U,0U}) {
    const auto parent = fragment;
    fragment = mhgp8::Q4LocalFragment::child(parent,quadrant,2048);
    local_fragment(gate,points,fragment,parent,2048);
  }
  const auto ball = oracle::make(points);
  gate.require(ball.ball.has_value(), "corner fixture lost strict rational positivity");
  const LocalPlane plane(points[0],points[1]);
  const auto coordinate = plane.coordinates(*ball.ball);
  gate.require(coordinate[0] == Rational(0) && coordinate[1] == Rational(-1) &&
               local_contains(fragment->cell(),coordinate,true) &&
               local_contains(fragment->cell(),coordinate,false),
               "positive corner root is not owned by the intended dyadic child");
  const auto active = local_expand(gate,geometry,fragment->active_nodes());
  gate.require(active.contains(3), "min=0 completion was removed from an owned corner shell");
  mhgp8::Q4LocalGeometryQueryWork query{};
  gate.require(!geometry->outside(fragment->cell(),query),
               "positive-domain certificate discarded a true positive corner centre");
  ++gate.corner_roots; ++gate.closed_roots; ++gate.owned_roots;
  local_fixture(gate,points,{0,1},3,
      {mhgp8::Q4CenterDomainMode::Positive,4,341,2048,0,true});
}

Points local_rotated(const Points& source) {
  const std::array<std::array<int,3>,3> transform{{{-20,4,22},{20,-10,20},{10,28,4}}};
  Points points;
  for (const auto point : source) {
    std::array<mhgp8::Coordinate,3> target{};
    for (std::size_t row = 0; row != 3; ++row) {
      int coordinate = 1000;
      for (std::size_t column = 0; column != 3; ++column)
        coordinate += transform[row][column]*(static_cast<int>(point[column])-20);
      if (coordinate < 0 || coordinate > 65535) throw std::runtime_error("rotation fixture outside u16");
      target[row] = static_cast<std::uint16_t>(coordinate);
    }
    points.push_back({target[0],target[1],target[2]});
  }
  return points;
}

void local_fixtures(LocalGate& gate) {
  using Mode = mhgp8::Q4CenterDomainMode;
  const Points twin{{20,20,20},{26,26,20},{26,20,26},{20,26,26},{26,20,14},{20,26,14}};
  const Points interior{{20,20,20},{60,60,20},{60,20,60},{20,60,60},
                        {40,40,40},{41,40,40},{40,41,40},{40,40,41}};
  const Points obtuse{{10,20,20},{30,20,20},{20,29,27},{20,11,22},{20,20,20}};
  const Points extreme{{0,0,0},{65535,65534,65533},{0,65535,65535},
                       {65535,0,65535},{32767,32767,32767},{65535,0,0}};
  const Points late_valid{{0,0,0},{2,2,0},{2,2,2},{2,0,2},{0,2,2}};
  const Points q3_dead{{0,0,0},{2,2,0},{2,0,2},{0,2,2},{1,1,1}};
  const Points rows{{10,10,10},{18,10,10},{10,14,10},{12,14,10},
                    {14,14,10},{16,14,10},{18,14,10},{12,10,10},{14,10,10},{16,10,10}};
  // z4 is inside the regular positive ball0123, but not uniformly inside
  // the root square. Its restriction to seed2 has root xi=-239/20, outside
  // [-2,2]; dropping that event WITHOUT its constant loses depth1.
  const Points clipped_inside{{100,100,100},{120,120,100},{100,120,80},
                              {120,100,80},{105,110,96}};
  local_fixture(gate,clipped_inside,{0,1},5,{Mode::Positive,0,1,512,0,true});
  for (const auto mode : {Mode::Disk,Mode::Positive}) {
    for (const auto budget : {u64{0},u64{3},u64{2048}})
      local_partition_fixture(gate,interior,{0,1},mode,budget);
    local_partition_fixture(gate,extreme,{0,1},mode,2048);
    ++gate.extreme_calls;
    const std::array<mhgp8::Q4LocalOptions,4> variants{{
      {mode,0,1,0,0,true}, {mode,2,21,32,0,true},
      {mode,5,85,512,0,true}, {mode,7,4096,512,32,true}}};
    for (const auto& options : variants) local_fixture(gate,twin,{0,1},5,options);
    for (const auto* points : {&interior,&obtuse,&late_valid,&q3_dead,&rows})
      local_fixture(gate,*points,{0,1},5,{mode,3,85,512,0,true});
    local_fixture(gate,extreme,{0,1},5,{mode,mhgp8::Q4LocalCell::max_depth,85,512,0,true});
    ++gate.extreme_calls;
  }
  for (const auto k : {3U,5U,10U})
    local_fixture(gate,interior,{0,1},k,{Mode::Positive,3,85,512,0,true});
  local_fixture(gate,shell30(),{0,1},5,{Mode::Positive,3,85,512,0,true});
  local_corner_fixture(gate);
  auto rotated = local_rotated(twin);
  local_fixture(gate,rotated,{0,1},5,{Mode::Positive,3,85,512,0,true});
  ++gate.rotations;
  std::swap(rotated[0],rotated[1]);
  local_partition_fixture(gate,rotated,{0,1},Mode::Positive,2048);
  local_fixture(gate,rotated,{0,1},5,{Mode::Positive,3,85,512,0,true});
  ++gate.rotations;
  for (std::size_t a = 0; a != late_valid.size(); ++a)
    for (std::size_t b = a+1; b != late_valid.size(); ++b) {
      local_fixture(gate,late_valid,{a,b},5,{Mode::Positive,2,21,32,0,true});
      ++gate.exhaustive_edges;
    }
}

template<class Factory> void local_fail_factory(LocalGate& gate, Factory&& factory) {
  bool caught = false;
  local_allocation::armed = true;
  try { factory(); }
  catch (const std::bad_alloc&) { caught = true; }
  catch (...) { local_allocation::armed = false; throw; }
  local_allocation::armed = false;
  gate.require(caught, "allocation failure did not interrupt immutable factory");
  ++gate.allocation_failures;
}

void local_lifecycle(LocalGate& gate) {
  using Mode = mhgp8::Q4CenterDomainMode;
  const Points points{{10,10,10},{12,12,10},{10,12,8},{12,10,8}};
  auto cloud = mhgp8::prepare_cloud(points);
  auto index = mhgp8::make_q2_cloud_index(cloud);
  auto cover = mhgp8::Q34EdgeCover::make(index,{0,1});
  auto geometry = mhgp8::Q4LocalGeometry::make(cover,Mode::Positive);
  auto fragment = mhgp8::Q4LocalFragment::root(geometry,32);
  const auto original_work = fragment->work();
  const auto original_nodes = std::vector<std::size_t>(fragment->active_nodes().begin(),fragment->active_nodes().end());
  const mhgp8::Q4LocalOptions options{Mode::Positive,3,85,512,0,true};
  auto atlas = mhgp8::Q4LocalAtlas::make(cover,5,options);
  const auto wanted = local_oracle(gate,points,{0,1},2,5);
  gate.require(!wanted.empty(), "lifecycle fixture has no q4 callback");
  local_fail_factory(gate,[&] { static_cast<void>(mhgp8::Q4LocalGeometry::make(cover,Mode::Disk)); });
  local_fail_factory(gate,[&] { static_cast<void>(mhgp8::Q4LocalFragment::root(geometry,32)); });
  local_fail_factory(gate,[&] { static_cast<void>(mhgp8::Q4LocalFragment::child(fragment,0,32)); });
  local_fail_factory(gate,[&] { static_cast<void>(mhgp8::Q4LocalFragment::refine(fragment,32)); });
  local_fail_factory(gate,[&] { static_cast<void>(mhgp8::Q4LocalAtlas::make(cover,5,options)); });
  gate.require(fragment->work() == original_work &&
    std::vector<std::size_t>(fragment->active_nodes().begin(),fragment->active_nodes().end()) == original_nodes,
    "failed immutable construction modified its parent");
  local_fragment(gate,points,fragment,{},32);
  const auto ignore = [](const mhgp8::Q34SeedCandidate&) {};
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalGeometry::make({},Mode::Disk)); }, "null local geometry cover accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalGeometry::make(cover,static_cast<Mode>(99))); }, "invalid local domain accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalFragment::root({},0)); }, "null fragment geometry accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalFragment::child({},0,0)); }, "null fragment parent accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalFragment::refine({},0)); }, "null refine parent accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalFragment::child(fragment,4,0)); }, "invalid child quadrant accepted");
  gate.rejects<std::out_of_range>([&] { static_cast<void>(geometry->form(points.size())); }, "invalid form point ID accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalAtlas::make({},5,options)); }, "null atlas cover accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalAtlas::make(cover,2,options)); }, "atlas K<3 accepted");
  auto invalid_options = options; invalid_options.max_depth = 45;
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalAtlas::make(cover,5,invalid_options)); }, "atlas depth45 accepted");
  invalid_options = options; invalid_options.node_budget = 0;
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalAtlas::make(cover,5,invalid_options)); }, "atlas without root budget accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q4_local_seed_candidates({},2,ignore)); }, "null atlas sweep accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q4_local_seed_candidates(atlas,2,{})); }, "empty local callback accepted");
  gate.rejects<std::out_of_range>([&] { static_cast<void>(mhgp8::run_q4_local_seed_candidates(atlas,points.size(),ignore)); }, "invalid local seed ID accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q4_local_seed_candidates(atlas,0,ignore)); }, "repeated local seed accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q4_local_edge_candidates(cover,0,options,ignore)); }, "zero local edge K accepted");
  {
    const Points foreign_points{{0,0,0},{2,0,0},{1,3,0},{1,0,3}};
    const auto foreign_cover = mhgp8::Q34EdgeCover::make(mhgp8::make_q2_cloud_index(mhgp8::prepare_cloud(foreign_points)),{0,1});
    const auto foreign_atlas = mhgp8::Q4LocalAtlas::make(foreign_cover,5,options);
    unsigned callbacks = 0;
    const auto work = mhgp8::run_q4_local_seed_candidates(foreign_atlas,2,[&](const auto&) { ++callbacks; });
    gate.require(callbacks == 0 && work.seed_owner_rejections == 1 && work.active_sites == 0,
                 "foreign owner reached local witness sweeps");
  }
  for (const auto k : {1U,2U}) {
    unsigned callbacks = 0;
    const auto work = mhgp8::run_q4_local_edge_candidates(cover,k,options,[&](const auto&) { ++callbacks; });
    gate.require(callbacks == 0 && work.seeds == 0 && work.sweep.emitted == 0,
                 "inactive q4 K produced work or candidates");
  }
  auto deepest = fragment;
  for (unsigned depth = 0; depth != mhgp8::Q4LocalCell::max_depth; ++depth)
    deepest = mhgp8::Q4LocalFragment::child(deepest,3,0);
  gate.rejects([&] { static_cast<void>(mhgp8::Q4LocalFragment::child(deepest,0,0)); }, "max-depth fragment subdivided beyond exact dyadic domain");
  struct CallbackFailure {};
  bool caught = false;
  try { static_cast<void>(mhgp8::run_q4_local_seed_candidates(atlas,2,[](const auto&) { throw CallbackFailure{}; })); }
  catch (const CallbackFailure&) { caught = true; }
  gate.require(caught, "local callback exception was hidden");
  ++gate.callback_failures;
  std::array<std::future<Output>,4> futures;
  for (auto& future : futures)
    future = std::async(std::launch::async,[atlas] {
      Output output;
      static_cast<void>(mhgp8::run_q4_local_seed_candidates(atlas,2,[&](const auto& candidate) { output.push_back(copy(candidate)); }));
      normalize(output); return output;
    });
  for (auto& future : futures) {
    gate.require(future.get() == wanted, "shared immutable atlas changed under independent concurrent sweeps");
    ++gate.parallel_calls;
  }
  fragment.reset(); deepest.reset(); geometry.reset();
  Output reset_output;
  static_cast<void>(mhgp8::run_q4_local_seed_candidates(atlas,2,[&](const auto& candidate) {
    atlas.reset(); cover.reset(); index.reset(); cloud.reset();
    reset_output.push_back(copy(candidate));
  }));
  normalize(reset_output);
  gate.require(reset_output == wanted, "callback owner reset invalidated local candidate views");
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 2 || std::string_view(argv[1]) != "--selftest")
      throw std::invalid_argument("expected --selftest");
    LocalGate gate;
    local_fixtures(gate);
    local_lifecycle(gate);
    gate.require(gate.partitions > 0 && gate.parent_checks > 0 && gate.tangent_active > 0 &&
                 gate.inactive_inside > 0 && gate.inactive_outside > 0 && gate.partition_splits > 0,
                 "nonvacuity: exact partition cases not exercised");
    gate.require(gate.clipped_events > 0 && gate.clipped_inside > 0 && gate.retained_events > 0 && gate.constant_shells > 0 &&
                 gate.unowned_boundary_roots > 0 && gate.corner_roots > 0 && gate.q4 > 0 && gate.max_shell >= 30,
                 "nonvacuity: local clipping, shell or boundary cases not exercised");
    gate.require(gate.parallel_calls == 4 && gate.callback_failures == 1 && gate.allocation_failures == 5 &&
                 gate.refinements > 0 && gate.refine_inside_added > 0,
                 "nonvacuity: local ownership or exception cases not exercised");
    std::cout << "{\"schema\":\"mhgp8_q4_local_gate_v1\",\"status\":\"passed\"";
#define MHGP8_LOCAL_FIELD(name) std::cout << ",\"" #name "\":" << gate.name
    MHGP8_LOCAL_FIELD(checks); MHGP8_LOCAL_FIELD(partitions); MHGP8_LOCAL_FIELD(fragment_checks);
    MHGP8_LOCAL_FIELD(active_blocks); MHGP8_LOCAL_FIELD(active_sites);
    MHGP8_LOCAL_FIELD(inactive_inside); MHGP8_LOCAL_FIELD(inactive_outside); MHGP8_LOCAL_FIELD(tangent_active);
    MHGP8_LOCAL_FIELD(parent_checks); MHGP8_LOCAL_FIELD(closed_roots); MHGP8_LOCAL_FIELD(owned_roots);
    MHGP8_LOCAL_FIELD(unowned_boundary_roots); MHGP8_LOCAL_FIELD(corner_roots);
    MHGP8_LOCAL_FIELD(root_cells); MHGP8_LOCAL_FIELD(child_cells); MHGP8_LOCAL_FIELD(exhausted_partitions);
    MHGP8_LOCAL_FIELD(extreme_calls); MHGP8_LOCAL_FIELD(clipped_events); MHGP8_LOCAL_FIELD(retained_events);
    MHGP8_LOCAL_FIELD(clipped_inside);
    MHGP8_LOCAL_FIELD(constant_shells); MHGP8_LOCAL_FIELD(depth_drops); MHGP8_LOCAL_FIELD(clip_pairs);
    MHGP8_LOCAL_FIELD(atlas_reuses); MHGP8_LOCAL_FIELD(full_sweeps); MHGP8_LOCAL_FIELD(empty_outputs);
    MHGP8_LOCAL_FIELD(rotations); MHGP8_LOCAL_FIELD(partition_inside); MHGP8_LOCAL_FIELD(partition_outside);
    MHGP8_LOCAL_FIELD(partition_splits); MHGP8_LOCAL_FIELD(allocation_failures);
    MHGP8_LOCAL_FIELD(refinements); MHGP8_LOCAL_FIELD(refine_inside_added);
    MHGP8_LOCAL_FIELD(edge_calls); MHGP8_LOCAL_FIELD(seed_calls); MHGP8_LOCAL_FIELD(reference_calls);
    MHGP8_LOCAL_FIELD(oracle_completions); MHGP8_LOCAL_FIELD(oracle_sites); MHGP8_LOCAL_FIELD(candidates);
    MHGP8_LOCAL_FIELD(q4); MHGP8_LOCAL_FIELD(max_shell); MHGP8_LOCAL_FIELD(exhaustive_edges);
    MHGP8_LOCAL_FIELD(invalid_inputs); MHGP8_LOCAL_FIELD(callback_failures); MHGP8_LOCAL_FIELD(parallel_calls);
#undef MHGP8_LOCAL_FIELD
    std::cout << "}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "q4 local gate: " << error.what() << '\n';
    return 1;
  }
}
