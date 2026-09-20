#include "lanes/q4_local.hpp"

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

}  // namespace mhgp8
