#include "lanes/q4_local.hpp"
#include "lanes/q4_seed_cells.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp8 {
namespace {
constexpr auto absent=std::numeric_limits<std::size_t>::max();
constexpr i128 scale=Q4LocalCell::scale;
u64 capacity_bytes(std::size_t count,std::size_t size) {
  if (count>std::numeric_limits<u64>::max()/size) throw std::overflow_error("mhgp8 local capacity overflow");
  return static_cast<u64>(count)*size;
}
i64 distance(Point3 a,Point3 b) {
  i64 out=0;
  for(std::size_t i=0;i<3;++i) { const i64 d=static_cast<i64>(a[i])-b[i]; out+=d*d; }
  return out;
}
bool acute(Point3 a,Point3 b,Point3 c) {
  const auto d=distance(a,b),e=distance(a,c),f=distance(b,c);
  return d+e>f && d+f>e && e+f>d;
}
bool owned(std::span<const Point3> points,std::span<const std::size_t> ids,u64& tests) {
  const auto owner=std::minmax(ids[0],ids[1]);
  const auto d=distance(points[ids[0]],points[ids[1]]);
  for(std::size_t i=0;i<ids.size();++i) for(std::size_t j=i+1;j<ids.size();++j) {
    if(i==0 && j==1) continue;
    counter_add(tests);
    const auto e=distance(points[ids[i]],points[ids[j]]);
    if(e>d || (e==d && std::minmax(ids[i],ids[j])<owner)) return false;
  }
  return true;
}
Q4LocalCell child_cell(Q4LocalCell c,unsigned quadrant) {
  const auto x=c.left+(c.right-c.left)/2,y=c.bottom+(c.top-c.bottom)/2;
  if(quadrant&1U) c.left=x; else {c.right=x;c.owns_right=false;}
  if(quadrant&2U) c.bottom=y; else {c.top=y;c.owns_top=false;}
  ++c.depth;
  return c;
}
void validate(Q4LocalOptions o) {
  if((o.domain!=Q4CenterDomainMode::Disk && o.domain!=Q4CenterDomainMode::Positive) ||
     o.max_depth>44 || o.node_budget==0)
    throw std::invalid_argument("mhgp8 local atlas requires domain, depth<=44 and at least its root");
}
void merge(Q4LocalPartitionWork& a,const Q4LocalPartitionWork& b) {
#define A(f) counter_add(a.f,b.f)
  A(root_factories);A(child_factories);A(refine_factories);A(input_nodes);A(input_sites);A(inherited_inside_sites);
  A(node_visits);A(block_bound_tests);A(point_tests);A(z_splits);A(inside_nodes);A(outside_nodes);
  A(inside_sites);A(outside_sites);A(active_nodes);A(active_sites);A(budget_unexamined_nodes);
  A(budget_ambiguous_nodes);A(frontier_ids_copied);
#undef A
  a.peak_retained_bytes=std::max(a.peak_retained_bytes,b.peak_retained_bytes);
}
struct Center { i128 x{},y{},den{}; };
Center normalized(Center p) {
  if(p.den==0) throw std::logic_error("mhgp8 local center has zero denominator");
  if(p.den<0) {p.x=-p.x;p.y=-p.y;p.den=-p.den;}
  return p;
}
bool contains(Q4LocalCell c,Center p,bool ownership) {
  const i128 x=scale*p.x,y=scale*p.y;
  if(x<static_cast<i128>(c.left)*p.den || x>static_cast<i128>(c.right)*p.den ||
     y<static_cast<i128>(c.bottom)*p.den || y>static_cast<i128>(c.top)*p.den) return false;
  return !ownership || ((c.owns_right || x!=static_cast<i128>(c.right)*p.den) &&
                        (c.owns_top || y!=static_cast<i128>(c.top)*p.den));
}
Center intersection(Q4LocalForm a,Q4LocalForm b) {
  // Reduced intersections are degree four. Never cross-multiply two roots.
  return normalized({static_cast<i128>(a.y)*b.constant-static_cast<i128>(b.y)*a.constant,
      static_cast<i128>(b.x)*a.constant-static_cast<i128>(a.x)*b.constant,
      static_cast<i128>(a.x)*b.y-static_cast<i128>(b.x)*a.y});
}
Center reference(Q4LocalForm f,Q4LocalCell c,Q4LocalSweepWork& work) {
  counter_add(work.reference_points);
  if(f.y!=0) for(const auto x:std::array<i64,2>{c.left,c.right}) {
    const auto p=normalized({static_cast<i128>(x)*f.y,-scale*f.constant-static_cast<i128>(f.x)*x,scale*f.y});
    counter_add(work.reference_side_tests);
    if(contains(c,p,false)) return p;
  }
  if(f.x!=0) for(const auto y:std::array<i64,2>{c.bottom,c.top}) {
    const auto p=normalized({-scale*f.constant-static_cast<i128>(f.y)*y,static_cast<i128>(y)*f.x,scale*f.x});
    counter_add(work.reference_side_tests);
    if(contains(c,p,false)) return p;
  }
  throw std::logic_error("mhgp8 intersecting seed line has no closed-cell reference");
}
i128 evaluate(Q4LocalForm f,Center p) {
  return static_cast<i128>(f.constant)*p.den+static_cast<i128>(f.x)*p.x+static_cast<i128>(f.y)*p.y;
}
}  // namespace

struct Q4LocalAtlas::Impl {
  enum class State { Branch,Outside,Deep,Leaf };
  struct Node {
    Q4LocalCell cell;
    Q4LocalFragmentPtr fragment;
    std::size_t children{absent};
    State state{State::Leaf};
  };
  Q4LocalGeometryPtr geometry;
  std::size_t k;
  Q4LocalOptions options;
  Q4LocalAtlasWork work;
  std::vector<Node> nodes;
  u64 fragment_bytes{};
  Impl(Q34EdgeCoverPtr cover,std::size_t kmax,Q4LocalOptions o)
      :geometry(Q4LocalGeometry::make(std::move(cover),o.domain)),k(kmax),options(o) {
    nodes.emplace_back();work.cells_created=1;
    build(0,{},0);
    work.retained_bytes=memory();
  }
  u64 memory() const {
    u64 out=capacity_bytes(nodes.capacity(),sizeof(Node));
    counter_add(out,fragment_bytes);counter_add(out,static_cast<u64>(geometry->retained_bytes()));
    return out;
  }
  void observe() {
    work.peak_fragment_bytes=std::max(work.peak_fragment_bytes,fragment_bytes);
    work.peak_build_bytes=std::max(work.peak_build_bytes,memory());
  }
  void release(std::size_t id) {
    fragment_bytes-=static_cast<u64>(nodes[id].fragment->retained_bytes());
    nodes[id].fragment.reset();
  }
  void build(std::size_t id,Q4LocalFragmentPtr parent,unsigned quadrant) {
    const auto c=nodes[id].cell;
    work.max_depth=std::max(work.max_depth,static_cast<u64>(c.depth));
    observe();
    if(geometry->outside(c,work.domain)) {nodes[id].state=State::Outside;counter_add(work.outside_cells);return;}
    auto fragment=parent?Q4LocalFragment::child(std::move(parent),quadrant,options.z_test_budget):
                         Q4LocalFragment::root(geometry,options.z_test_budget);
    if(fragment->cell()!=c) throw std::logic_error("mhgp8 local atlas partition cell mismatch");
    merge(work.partition,fragment->work());
    counter_add(fragment_bytes,static_cast<u64>(fragment->retained_bytes()));
    nodes[id].fragment=fragment;
    observe();
    if(fragment->inside_count()>=k-2) {
      nodes[id].state=State::Deep;counter_add(work.deep_cells);release(id);return;
    }
    bool stop=false;
    if(fragment->active_sites()<=options.leaf_sites) {counter_add(work.small_stops);stop=true;}
    else if(c.depth==options.max_depth) {counter_add(work.depth_stops);stop=true;}
    else if(options.node_budget-nodes.size()<4) {counter_add(work.node_stops);stop=true;}
    if(stop) {
      if(fragment->work().budget_unexamined_nodes!=0 || fragment->work().budget_ambiguous_nodes!=0) {
        // Pay the final partition ONCE per shared leaf, before any seed can
        // scan it. Unclassified large Z blocks must not become repeated full
        // cover scans just because the center-refinement budget was exhausted.
        auto finished=Q4LocalFragment::refine(fragment,std::numeric_limits<u64>::max());
        merge(work.partition,finished->work());counter_add(work.terminal_refinements);
        counter_add(fragment_bytes,static_cast<u64>(finished->retained_bytes()));
        observe();  // Both the previous and replacement frontier are live.
        release(id);fragment.reset();
        nodes[id].fragment=finished;fragment=std::move(finished);
        if(fragment->inside_count()>=k-2) {
          nodes[id].state=State::Deep;counter_add(work.deep_cells);
          counter_add(work.terminal_deep_cells);release(id);return;
        }
      }
      counter_add(work.leaf_cells);
      counter_add(work.active_sites_sum,static_cast<u64>(fragment->active_sites()));
      counter_add(work.active_blocks_sum,static_cast<u64>(fragment->active_nodes().size()));
      return;
    }
    const auto first=nodes.size();
    nodes.resize(first+4);
    nodes[id].children=first;nodes[id].state=State::Branch;
    counter_add(work.cells_created,4);counter_add(work.splits);
    observe();
    for(unsigned q=0;q<4;++q) nodes[first+q].cell=child_cell(c,q);
    for(unsigned q=0;q<4;++q) build(first+q,fragment,q);
    release(id);
  }
};

Q4LocalAtlas::Q4LocalAtlas(std::unique_ptr<Impl> p):impl_(std::move(p)) {}
Q4LocalAtlas::~Q4LocalAtlas()=default;
Q4LocalAtlasPtr Q4LocalAtlas::make(Q34EdgeCoverPtr cover,std::size_t k,Q4LocalOptions o) {
  validate(o);
  if(!cover || k<3) throw std::invalid_argument("mhgp8 local atlas requires cover and K>=3");
  return Q4LocalAtlasPtr(new Q4LocalAtlas(std::make_unique<Impl>(std::move(cover),k,o)));
}
const Q4LocalGeometryPtr& Q4LocalAtlas::geometry() const noexcept {return impl_->geometry;}
const Q4LocalAtlasWork& Q4LocalAtlas::work() const noexcept {return impl_->work;}
std::size_t Q4LocalAtlas::kmax() const noexcept {return impl_->k;}
const Q4LocalOptions& Q4LocalAtlas::options() const noexcept {return impl_->options;}
std::size_t Q4LocalAtlas::retained_bytes() const {
  const auto size=impl_->memory();
  if(size>std::numeric_limits<std::size_t>::max()) throw std::overflow_error("mhgp8 local atlas bytes exceed size_t");
  return static_cast<std::size_t>(size);
}

struct Q4LocalEngine {
  Q4LocalAtlasPtr atlas;
  Q4LocalSweepWork work;
  std::vector<std::size_t> events,shell;
  explicit Q4LocalEngine(Q4LocalAtlasPtr a):atlas(std::move(a)) {}
  void observe() {
    u64 size=capacity_bytes(events.capacity(),sizeof(std::size_t));
    counter_add(size,capacity_bytes(shell.capacity(),sizeof(std::size_t)));
    work.peak_buffer_bytes=std::max(work.peak_buffer_bytes,size);
  }
  void sweep(const Q4LocalFragment& fragment,std::size_t x,const Q4FamilySeed& family,
             Q4LocalForm line,const Q34SeedConsumer& consumer) {
    const auto& geometry=*atlas->geometry();
    const auto cover=geometry.cover();
    const auto points=cover->index()->cloud().points();
    const auto nodes=cover->index()->spatial_nodes();
    const auto order=cover->index()->spatial_order();
    const auto edge=cover->edge_ids();
    const auto c=fragment.cell();
    const bool clip=atlas->options().clip_events;
    const auto origin=clip?reference(line,c,work):Center{};
    events.clear();shell.clear();
    std::size_t inside=fragment.inside_count(),base=inside,entry_count=0;
    counter_add(work.leaf_queries);
    for(const auto node_id:fragment.active_nodes()) {
      counter_add(work.active_blocks);
      const auto range=nodes[node_id].range;
      for(auto rank=range.first;rank<range.last;++rank) {
        const auto id=order[rank];counter_add(work.active_sites);
        const auto side=family.side(points[id]);
        if(side==0) {
          const auto power=family.power(points[id]);
          if(power<0) {++inside;++base;counter_add(work.constant_inside);}
          else if(power==0) {shell.push_back(id);counter_add(work.constant_shell_ids);}
          else counter_add(work.constant_outside);
          continue;
        }
        if(clip) {
          const auto f=geometry.form(id);
          counter_add(work.root_locations);
          if(!contains(c,intersection(line,f),false)) {
            counter_add(work.clipped_events);
            const auto value=evaluate(f,origin);
            if(value==0) throw std::logic_error("mhgp8 clipped event vanishes in closed cell");
            if(value<0) {++inside;++base;counter_add(work.clipped_inside);}
            continue;
          }
        }
        events.push_back(id);counter_add(work.kept_events);
        if(side>0) {++entry_count;counter_add(work.entries);}
        else {++inside;counter_add(work.exits);}
      }
    }
    observe();
    std::sort(shell.begin(),shell.end(),[&](auto a,auto b) {counter_add(work.shell_sort_comparisons);return a<b;});
    std::sort(events.begin(),events.end(),[&](auto a,auto b) {
      counter_add(work.sort_comparisons);
      const auto comparison=family.compare_roots(points[a],points[b]);
      return comparison<0 || (comparison==0 && a<b);
    });
    for(std::size_t first=0;first<events.size();) {
      auto last=first+1;
      while(last<events.size()) {
        counter_add(work.group_comparisons);
        if(family.compare_roots(points[events[first]],points[events[last]])!=0) break;
        ++last;
      }
      std::size_t entries=0,exits=0;
      for(auto pos=first;pos<last;++pos) {
        if(family.side(points[events[pos]])>0) ++entries; else ++exits;
      }
      if(exits>inside) throw std::logic_error("mhgp8 local sweep exit underflow");
      inside-=exits;counter_add(work.groups);
      work.max_group=std::max(work.max_group,static_cast<u64>(last-first));
      counter_add(work.root_locations);
      if(!contains(c,intersection(line,geometry.form(events[first])),true)) {
        counter_add(work.boundary_skips);counter_add(work.boundary_skipped_ids,static_cast<u64>(last-first));
      } else if(inside>=atlas->kmax()-2) {
        counter_add(work.depth_rejections);counter_add(work.depth_skipped_ids,static_cast<u64>(last-first));
      }
      else {
        bool emitted=false;
        for(auto pos=first;pos<last;++pos) {
          const auto y=events[pos];counter_add(work.presentations);
          std::array<std::size_t,4> ids{edge[0],edge[1],x,y};
          if(!owned(points,ids,work.owner_tests)) {counter_add(work.owner_rejections);continue;}
          counter_add(work.positive_tests);
          const auto ball=ExactBall::make_q4({points[edge[0]],points[edge[1]],points[x],points[y]});
          if(!ball) {counter_add(work.positive_rejections);continue;}
          counter_add(work.canonical_tests);
          if(y<x && acute(points[edge[0]],points[edge[1]],points[y])) {counter_add(work.canonical_rejections);continue;}
          std::sort(ids.begin(),ids.end());
          consumer(Q34SeedCandidate{4,ids,*ball,inside,
              std::span<const std::size_t>(events).subspan(first,last-first),shell});
          counter_add(work.emitted);counter_add(work.shell_ids,static_cast<u64>(last-first+shell.size()));
          counter_add(work.unexamined_after_emit,static_cast<u64>(last-pos-1));
          emitted=true;break;
        }
        if(!emitted) counter_add(work.groups_without_support);
      }
      if(entries>cover->site_count()-inside) throw std::logic_error("mhgp8 local sweep entry overflow");
      inside+=entries;first=last;
    }
    if(inside!=base+entry_count) throw std::logic_error("mhgp8 local sweep final population mismatch");
  }
  void visit(std::size_t id,std::size_t x,const Q4FamilySeed& family,Q4LocalForm line,
             const Q34SeedConsumer& consumer) {
    const auto& p=*atlas->impl_;
    const auto& node=p.nodes[id];counter_add(work.query_visits);
    if(node.state==Q4LocalAtlas::Impl::State::Deep || node.state==Q4LocalAtlas::Impl::State::Outside) return;
    counter_add(work.line_tests);
    const auto b=p.geometry->bounds(line,node.cell);
    if(b.minimum>0 || b.maximum<0) {counter_add(work.line_skips);return;}
    if(node.state==Q4LocalAtlas::Impl::State::Leaf) sweep(*node.fragment,x,family,line,consumer);
    else for(std::size_t q=0;q<4;++q) visit(node.children+q,x,family,line,consumer);
  }
  void seed(std::size_t x,const Q34SeedConsumer& consumer) {
    const auto cover=atlas->geometry()->cover();
    const auto points=cover->index()->cloud().points();
    if(x>=points.size()) throw std::out_of_range("mhgp8 local seed ID outside owner");
    const auto edge=cover->edge_ids();
    const auto family=Q4FamilySeed::make(points[edge[0]],points[edge[1]],points[x]);
    if(!family) throw std::invalid_argument("mhgp8 local seed must be strictly acute");
    counter_add(work.seed_queries);
    const std::array<std::size_t,3> ids{edge[0],edge[1],x};
    if(!owned(points,ids,work.seed_owner_tests)) {counter_add(work.seed_owner_rejections);return;}
    visit(0,x,*family,atlas->geometry()->form(x),consumer);
  }
};

Q4LocalSweepWork run_q4_local_seed_candidates(Q4LocalAtlasPtr atlas,std::size_t x,
                                             const Q34SeedConsumer& consumer) {
  if(!atlas || !consumer) throw std::invalid_argument("mhgp8 local seed requires atlas and callback");
  Q4LocalEngine engine(std::move(atlas));engine.seed(x,consumer);return engine.work;
}
Q4LocalEdgeWork run_q4_local_edge_candidates(Q34EdgeCoverPtr cover,std::size_t k,Q4LocalOptions o,
                                           const Q34SeedConsumer& consumer) {
  validate(o);
  if(!cover || !consumer || k==0) throw std::invalid_argument("mhgp8 local edge requires cover, callback and K>0");
  Q4LocalEdgeWork work{};
  if(k<3) return work;
  const auto atlas=Q4LocalAtlas::make(cover,k,o);
  work.atlas=atlas->work();work.geometry=atlas->geometry()->work();
  Q4LocalEngine engine(atlas);
  const auto points=cover->index()->cloud().points();
  const auto nodes=cover->index()->spatial_nodes();
  const auto order=cover->index()->spatial_order();
  const auto edge=cover->edge_ids();
  const auto a=points[edge[0]],b=points[edge[1]];
  const auto diameter=distance(a,b);
  // Explicitly requalified port of tranche24's block seed generator, no census
  // callback or q3 filtering is inherited by this Q4-only entry.
  std::size_t cursor=0;
  while(cursor<nodes.size()) {
    const auto& node=nodes[cursor];counter_add(work.node_visits);
    if(node.range.size()==1) {
      const auto id=order[node.range.first];counter_add(work.point_tests);
      if(acute(a,b,points[id])) {
        counter_add(work.acute_seeds);
        const std::array<std::size_t,3> ids{edge[0],edge[1],id};
        if(owned(points,ids,work.owner_tests)) {counter_add(work.seeds);engine.seed(id,consumer);}
        else counter_add(work.owner_rejections);
      }
      cursor=node.escape;continue;
    }
    counter_add(work.bound_tests);
    i64 min_a=0,min_b=0,max_sum=0;
    for(std::size_t i=0;i<3;++i) {
      const i64 al=static_cast<i64>(node.box.low[i])-a[i],ah=static_cast<i64>(node.box.high[i])-a[i];
      const i64 bl=static_cast<i64>(node.box.low[i])-b[i],bh=static_cast<i64>(node.box.high[i])-b[i];
      const auto na=al>0?al:(ah<0?ah:0),nb=bl>0?bl:(bh<0?bh:0);
      min_a+=na*na;min_b+=nb*nb;max_sum+=std::max(al*al+bl*bl,ah*ah+bh*bh);
    }
    if(min_a>diameter || min_b>diameter || max_sum<=diameter) {
      counter_add(work.rejected_nodes);counter_add(work.rejected_sites,static_cast<u64>(node.range.size()));cursor=node.escape;
    } else {counter_add(work.split_nodes);cursor=node.left;}
  }
  work.sweep=engine.work;
  work.peak_live_buffer_bytes=static_cast<u64>(atlas->retained_bytes());
  counter_add(work.peak_live_buffer_bytes,work.sweep.peak_buffer_bytes);
  work.peak_live_buffer_bytes=std::max(work.peak_live_buffer_bytes,work.atlas.peak_build_bytes);
  return work;
}

// Explicit port of the independent seed/cell traversal idea, not its copied
// private-access instrumentation or qualification. The existing atlas and
// sweep are reused directly; the old entry above has no live-summary/cache.
struct Q4SeedCellEngine {
  using State=Q4LocalAtlas::Impl::State;
  struct CacheEntry {
    enum class Status : std::uint8_t { Unknown,Invalid,Valid };
    Status status{Status::Unknown};
    Q4LocalForm line{};
    std::optional<Q4FamilySeed> family;
  };
  struct Product {std::size_t x{},cell{};bool spatial_test_paid{};};
  static constexpr std::size_t stack_capacity=1+48+3*44;

  Q4LocalEngine engine;
  Q4LocalEdgeWork& result;
  Q4SeedCellWork& extra;
  Q4SeedCellOptions options;
  std::span<const Q2SpatialNode> nodes;
  std::span<const std::size_t> order;
  std::span<const Point3> points;
  std::array<std::size_t,2> edge;
  Point3 a,b;
  i64 diameter;
  std::vector<u64> live;
  std::vector<CacheEntry> cache;
  u64 stack_bytes{};

  Q4SeedCellEngine(Q4LocalAtlasPtr atlas,Q4LocalEdgeWork& work,
      Q4SeedCellWork& additional,Q4SeedCellOptions config)
      :engine(std::move(atlas)),result(work),extra(additional),options(config),
       nodes(engine.atlas->geometry()->cover()->index()->spatial_nodes()),
       order(engine.atlas->geometry()->cover()->index()->spatial_order()),
       points(engine.atlas->geometry()->cover()->index()->cloud().points()),
       edge(engine.atlas->geometry()->cover()->edge_ids()),a(points[edge[0]]),
       b(points[edge[1]]),diameter(distance(a,b)) {}

  void observe(u64 replacement_cache_bytes=0) {
    u64 cache_bytes=capacity_bytes(cache.capacity(),sizeof(CacheEntry));
    counter_add(cache_bytes,replacement_cache_bytes);
    const u64 live_bytes=capacity_bytes(live.capacity(),sizeof(u64));
    extra.peak_cache_bytes=std::max(extra.peak_cache_bytes,cache_bytes);
    extra.peak_live_bytes=std::max(extra.peak_live_bytes,live_bytes);
    u64 auxiliary=cache_bytes;
    counter_add(auxiliary,live_bytes);counter_add(auxiliary,stack_bytes);
    extra.peak_auxiliary_bytes=std::max(extra.peak_auxiliary_bytes,auxiliary);
    u64 total=static_cast<u64>(engine.atlas->retained_bytes());
    counter_add(total,auxiliary);
    counter_add(total,capacity_bytes(engine.events.capacity(),sizeof(std::size_t)));
    counter_add(total,capacity_bytes(engine.shell.capacity(),sizeof(std::size_t)));
    // Atlas construction precedes these private buffers. Do not sum maxima
    // from disjoint phases; actual capacities coexisting now are summed.
    total=std::max(total,result.atlas.peak_build_bytes);
    result.peak_live_buffer_bytes=std::max(result.peak_live_buffer_bytes,total);
    extra.peak_total_buffer_bytes=std::max(extra.peak_total_buffer_bytes,total);
  }

  void prepare_live() {
    counter_add(extra.live_preparations);
    const auto& cells=engine.atlas->impl_->nodes;
    live.resize(cells.size());
    observe();
    // The atlas reserves four siblings before any descendants: NOT preorder.
    // Children nevertheless have larger IDs, so reverse IDs form a valid
    // bottom-up evaluation of the exact number of descendant live leaves.
    for(auto id=cells.size();id!=0;) {
      --id;counter_add(extra.live_node_visits);
      const auto& cell=cells[id];
      if(cell.state==State::Leaf) {
        live[id]=1;counter_add(extra.live_leaves);
      } else if(cell.state==State::Branch) {
        const auto first=cell.children;
        if(first<=id || first>cells.size() || cells.size()-first<4)
          throw std::logic_error("mhgp8 seed/cell atlas has invalid child topology");
        for(std::size_t quadrant=0;quadrant<4;++quadrant) {
          counter_add(extra.live_child_reads);
          counter_add(live[id],live[first+quadrant]);
        }
      }
    }
    if(live.empty() || live[0]!=result.atlas.leaf_cells)
      throw std::logic_error("mhgp8 seed/cell live summary differs from atlas leaves");
  }

  bool spatial_pass(std::size_t id) {
    const auto& node=nodes[id];
    counter_add(result.node_visits);counter_add(result.bound_tests);
    i64 min_a=0,min_b=0,max_sum=0;
    for(std::size_t axis=0;axis<3;++axis) {
      const i64 al=static_cast<i64>(node.box.low[axis])-a[axis];
      const i64 ah=static_cast<i64>(node.box.high[axis])-a[axis];
      const i64 bl=static_cast<i64>(node.box.low[axis])-b[axis];
      const i64 bh=static_cast<i64>(node.box.high[axis])-b[axis];
      const auto na=al>0?al:(ah<0?ah:0),nb=bl>0?bl:(bh<0?bh:0);
      min_a+=na*na;min_b+=nb*nb;max_sum+=std::max(al*al+bl*bl,ah*ah+bh*bh);
    }
    if(min_a>diameter || min_b>diameter || max_sum<=diameter) {
      counter_add(result.rejected_nodes);
      counter_add(result.rejected_sites,static_cast<u64>(node.range.size()));
      return false;
    }
    return true;
  }

  bool seed_pass(std::size_t id) {
    counter_add(result.node_visits);counter_add(result.point_tests);
    if(!acute(a,b,points[id])) return false;
    counter_add(result.acute_seeds);
    const std::array<std::size_t,3> support{edge[0],edge[1],id};
    if(!owned(points,support,result.owner_tests)) {
      counter_add(result.owner_rejections);return false;
    }
    counter_add(result.seeds);return true;
  }

  void visit_live(std::size_t cell_id,std::size_t x,const Q4FamilySeed& family,
                  Q4LocalForm line,const Q34SeedConsumer& consumer) {
    counter_add(engine.work.query_visits);
    if(live[cell_id]==0) {counter_add(extra.live_skipped_nodes);return;}
    const auto& cell=engine.atlas->impl_->nodes[cell_id];
    counter_add(engine.work.line_tests);
    const auto bound=engine.atlas->geometry()->bounds(line,cell.cell);
    if(bound.minimum>0 || bound.maximum<0) {counter_add(engine.work.line_skips);return;}
    if(cell.state==State::Leaf) {
      engine.sweep(*cell.fragment,x,family,line,consumer);observe();
    } else {
      if(cell.state!=State::Branch) throw std::logic_error("mhgp8 live atlas node has no continuation");
      for(std::size_t quadrant=0;quadrant<4;++quadrant)
        visit_live(cell.children+quadrant,x,family,line,consumer);
    }
  }

  void live_only(const Q34SeedConsumer& consumer) {
    std::size_t cursor=0;
    while(cursor<nodes.size()) {
      const auto& node=nodes[cursor];
      if(node.range.size()==1) {
        const auto x=order[node.range.first];
        if(seed_pass(x)) {
          const auto family=Q4FamilySeed::make(a,b,points[x]);
          if(!family) throw std::logic_error("mhgp8 live acute seed has no family");
          counter_add(extra.family_preparations);counter_add(engine.work.seed_queries);
          const std::array<std::size_t,3> ids{edge[0],edge[1],x};
          if(!owned(points,ids,engine.work.seed_owner_tests)) counter_add(engine.work.seed_owner_rejections);
          else {
            const auto line=engine.atlas->geometry()->form(x);
            counter_add(extra.form_preparations);
            visit_live(0,x,*family,line,consumer);
          }
        }
        cursor=node.escape;
      } else if(!spatial_pass(cursor)) cursor=node.escape;
      else {counter_add(result.split_nodes);cursor=node.left;}
    }
  }

  void prepare_cache(std::size_t count) {
    cache.clear();
    if(count>cache.capacity()) {
      // Explicit growth lets the peak account for BOTH allocations. Entries
      // are initialized only once here; no reserve(n), global family array or
      // copied antichain. Actual selected block size determines allocation.
      std::vector<CacheEntry> replacement(count);
      counter_add(extra.cache_entries_initialized,static_cast<u64>(count));
      observe(capacity_bytes(replacement.capacity(),sizeof(CacheEntry)));
      cache.swap(replacement);
    } else {
      cache.resize(count);
      counter_add(extra.cache_entries_initialized,static_cast<u64>(count));
    }
    observe();
  }

  void joined(const Q34SeedConsumer& consumer) {
    std::array<Product,stack_capacity> stack{};
    stack_bytes=static_cast<u64>(sizeof(stack));
    extra.product_stack_bytes=std::max(extra.product_stack_bytes,stack_bytes);
    observe();
    std::size_t size=0;
    const auto push=[&](Product product) {
      if(size==stack.size()) throw std::logic_error("mhgp8 seed/cell product exceeds proven DFS height");
      stack[size++]=product;
      extra.peak_product_stack=std::max(extra.peak_product_stack,static_cast<u64>(size));
    };
    const auto& cells=engine.atlas->impl_->nodes;
    const auto& geometry=*engine.atlas->geometry();
    std::size_t cursor=0;
    while(cursor<nodes.size()) {
      const auto& block=nodes[cursor];
      counter_add(extra.antichain_node_visits);
      if(block.range.size()>1 && !spatial_pass(cursor)) {cursor=block.escape;continue;}
      if(block.range.size()>options.block_sites) {
        counter_add(extra.antichain_splits);counter_add(result.split_nodes);cursor=block.left;continue;
      }
      counter_add(extra.blocks);counter_add(extra.block_sites,static_cast<u64>(block.range.size()));
      extra.max_block_sites=std::max(extra.max_block_sites,static_cast<u64>(block.range.size()));
      prepare_cache(block.range.size());
      push({cursor,0,block.range.size()>1});
      while(size!=0) {
        auto product=stack[--size];counter_add(extra.product_visits);
        if(live[product.cell]==0) {counter_add(extra.live_skipped_nodes);continue;}
        const auto& xnode=nodes[product.x];
        const auto& cell=cells[product.cell];
        const bool singleton=xnode.range.size()==1;
        CacheEntry* seed=nullptr;
        std::size_t x=0;
        Q4LocalBounds bound{};
        if(singleton) {
          if(xnode.range.first<block.range.first || xnode.range.last>block.range.last)
            throw std::logic_error("mhgp8 seed/cell cache received a foreign spatial rank");
          seed=&cache[xnode.range.first-block.range.first];x=order[xnode.range.first];
          if(seed->status==CacheEntry::Status::Unknown) {
            counter_add(extra.cache_misses);
            seed->status=CacheEntry::Status::Invalid;
            if(seed_pass(x)) {
              seed->line=geometry.form(x);counter_add(extra.form_preparations);
              seed->status=CacheEntry::Status::Valid;
            }
          } else {
            counter_add(extra.cache_hits);
            if(seed->status==CacheEntry::Status::Invalid) counter_add(extra.invalid_cache_hits);
          }
          if(seed->status==CacheEntry::Status::Invalid) {
            counter_add(extra.product_seed_rejections);continue;
          }
          counter_add(extra.singleton_bound_tests);bound=geometry.bounds(seed->line,cell.cell);
        } else {
          if(product.spatial_test_paid) counter_add(extra.spatial_tests_reused);
          else if(!spatial_pass(product.x)) {counter_add(extra.product_seed_rejections);continue;}
          product.spatial_test_paid=true;
          counter_add(extra.block_bound_tests);bound=geometry.node_bounds(product.x,cell.cell);
        }
        // Cell geometry is CLOSED. Even an isolated contact/corner or an
        // entire coincident seed line must reach the existing owner-aware sweep.
        if(bound.minimum>=0) {counter_add(extra.positive_products);continue;}
        if(bound.maximum<0) {counter_add(extra.negative_products);continue;}
        counter_add(extra.uncertain_products);
        if(bound.minimum==0 || bound.maximum==0) counter_add(extra.zero_bound_products);
        if(singleton && cell.state==State::Leaf) {
          counter_add(extra.terminal_pairs);
          if(!seed->family) {
            const auto family=Q4FamilySeed::make(a,b,points[x]);
            if(!family) throw std::logic_error("mhgp8 cached acute seed has no family");
            seed->family.emplace(*family);counter_add(extra.family_preparations);
            counter_add(engine.work.seed_queries);
          } else counter_add(extra.family_cache_hits);
          engine.sweep(*cell.fragment,x,*seed->family,seed->line,consumer);observe();
        } else {
          if(cell.state!=State::Leaf && cell.state!=State::Branch)
            throw std::logic_error("mhgp8 live seed/cell product has no continuation");
          const bool split_x=!singleton &&
              (cell.state==State::Leaf || xnode.range.size()>=live[product.cell]);
          if(split_x) {
            counter_add(extra.x_splits);counter_add(result.split_nodes);
            push({xnode.right,product.cell,false});push({xnode.left,product.cell,false});
          } else {
            counter_add(extra.cell_splits);
            for(std::size_t quadrant=4;quadrant!=0;--quadrant)
              push({product.x,cell.children+quadrant-1,product.spatial_test_paid});
          }
        }
      }
      cursor=block.escape;
    }
    observe();
    stack_bytes=0; // Fixed descriptor backing ends when joined returns.
  }

  void run(const Q34SeedConsumer& consumer) {
    // O(1) atlas certificate, shared by BOTH non-Individual controls. It
    // precedes any live-summary allocation and is not a product-join gain.
    if(result.atlas.leaf_cells==0) {
      counter_add(extra.whole_atlas_skips);observe();return;
    }
    prepare_live();
    if(options.mode==Q4SeedCellMode::LiveOnly) live_only(consumer);
    else joined(consumer);
    result.sweep=engine.work;observe();
  }
};

Q4LocalEdgeWork run_q4_local_edge_candidates(Q34EdgeCoverPtr cover,std::size_t k,
    Q4LocalOptions local_options,const Q34SeedConsumer& consumer,
    Q4SeedCellOptions seed_options,Q4SeedCellWork& extra) {
  if((seed_options.mode!=Q4SeedCellMode::Individual && seed_options.mode!=Q4SeedCellMode::LiveOnly &&
      seed_options.mode!=Q4SeedCellMode::Joined) || seed_options.block_sites==0)
    throw std::invalid_argument("mhgp8 seed/cell traversal requires a valid mode and positive block grain");
  validate(local_options);
  if(!cover || !consumer || k==0)
    throw std::invalid_argument("mhgp8 seed/cell traversal requires cover, callback and K>0");
  if(seed_options.mode==Q4SeedCellMode::Individual)
    return run_q4_local_edge_candidates(std::move(cover),k,local_options,consumer);
  Q4LocalEdgeWork result{};
  if(k<3) return result;
  counter_add(extra.queries);
  auto atlas=Q4LocalAtlas::make(std::move(cover),k,local_options);
  result.atlas=atlas->work();result.geometry=atlas->geometry()->work();
  Q4SeedCellEngine engine(std::move(atlas),result,extra,seed_options);
  engine.run(consumer);
  return result;
}

}  // namespace mhgp8
