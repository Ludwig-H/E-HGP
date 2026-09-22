#include "lanes/q4_center_map.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mhgp9::gen {
namespace {
using Vec = std::array<i64, 3>;
// Q=2^42: the disk test 2*norm<=96*M^2*Q^2 must stay below 2^127, which
// 2^44 did for M=65535 but not for M=262143 (18 bits). Real cells and every
// classification are unchanged; only the integer depth ceiling becomes 42.
constexpr i64 scale = i64{1} << 42;
constexpr auto absent = std::numeric_limits<std::size_t>::max();
struct Form { i64 constant{}, x{}, y{}; };
struct Cell { i64 left{}, right{}, bottom{}, top{}; };
struct Projection { i128 x{}, y{}; Vec raw{}; };
struct Facet { Vec normal{}; i128 height{}; };
i64 dot(Vec a, Vec b) {
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
Vec cross(Vec a, Vec b) {
  return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
i64 distance(Point3 a, Point3 b) {
  Vec v{};
  for (std::size_t i=0;i<3;++i) v[i]=static_cast<i64>(a[i])-b[i];
  return dot(v,v);
}
std::pair<i128,i128> linear_bounds(i128 constant, i128 x, i128 y, Cell c) {
  return {constant + x*(x<0?c.right:c.left) + y*(y<0?c.top:c.bottom),
          constant + x*(x<0?c.left:c.right) + y*(y<0?c.bottom:c.top)};
}
std::pair<i128,i128> bounds(Form f, Cell c) {
  return linear_bounds(static_cast<i128>(scale)*f.constant, f.x, f.y, c);
}
std::size_t bytes(std::size_t count, std::size_t item) {
  if (count>std::numeric_limits<std::size_t>::max()/item)
    throw std::overflow_error("mhgp9 gen center map capacity overflow");
  return count*item;
}
}  // namespace

void validate_q4_center_map_options(Q4CenterMapOptions options) {
  if ((options.domain!=Q4CenterDomainMode::Disk && options.domain!=Q4CenterDomainMode::Positive) ||
      options.max_depth>42)
    throw std::invalid_argument("mhgp9 gen center map requires valid domain and depth <=42");
}

struct Q4CenterMap::Impl {
  enum class State { Fresh, Unknown, Deep, Outside };
  struct Node {
    Cell cell;
    std::vector<std::size_t> pending;
    std::size_t credit{}, children{absent};
    unsigned depth{};
    State state{State::Fresh};
  };
  Q34WitnessPoolPtr owner;
  std::size_t kmax;
  Q4CenterMapOptions options;
  Q4PositiveDomainPtr domain;
  Q4CenterMapWork work;
  Vec v{}, A{}, B{}, midpoint_twice{};
  i64 d{};
  std::size_t axis_i{},axis_j{};
  std::vector<Form> forms;
  std::array<Facet,9> facets{};
  std::size_t facet_count{}, pending_bytes{};
  std::vector<Node> nodes;
  bool failed{};
  std::size_t last_query_peak{};

  Impl(Q34WitnessPoolPtr pool, std::size_t k, Q4CenterMapOptions opts)
      : owner(std::move(pool)), kmax(k), options(opts) {
    if (options.node_budget==0) return;
    counter_add(work.preparations);
    const auto points=owner->cover()->index()->cloud().points();
    const auto edge=owner->cover()->edge_ids();
    const auto a=points[edge[0]],b=points[edge[1]];
    std::size_t main_axis=0;
    for (std::size_t i=0;i<3;++i) {
      v[i]=static_cast<i64>(b[i])-a[i];
      midpoint_twice[i]=static_cast<i64>(a[i])+b[i];
      if (std::abs(v[i])>std::abs(v[main_axis])) main_axis=i;
    }
    d=dot(v,v);
    axis_i=(main_axis+1)%3;
    axis_j=(main_axis+2)%3;
    const i64 h=std::abs(v[main_axis]),sign=v[main_axis]>0?1:-1;
    A[axis_i]=h; A[main_axis]=-sign*v[axis_i];
    B[axis_j]=h; B[main_axis]=-sign*v[axis_j];
    if (options.domain==Q4CenterDomainMode::Positive) {
      domain=Q4PositiveDomain::make(owner->cover());
      work.domain=domain->work();
      if (domain->completion_box()) prepare_hull(*domain->completion_box());
    }
    forms.reserve(owner->ids().size());
    for (const auto id:owner->ids()) {
      forms.push_back(form(points[id]));
      counter_add(work.prepared_forms);
    }
    Node root{{-2*scale,2*scale,-2*scale,2*scale},{},0,absent,0,State::Fresh};
    root.pending.resize(forms.size());
    std::iota(root.pending.begin(),root.pending.end(),std::size_t{0});
    counter_add(work.pending_ids_copied,static_cast<u64>(root.pending.size()));
    if (!root.pending.empty()) counter_add(work.pending_lists_created);
    pending_bytes=bytes(root.pending.capacity(),sizeof(std::size_t));
    nodes.push_back(std::move(root));
    counter_add(work.node_storage_growths);
    counter_add(work.cells_created);
    observe_memory();
  }

  Form form(Point3 point) const {
    Vec w{};
    for (std::size_t i=0;i<3;++i) w[i]=2*static_cast<i64>(point[i])-midpoint_twice[i];
    // |w|<=2M; |A_i|,|B_i|<=M. All coefficients fit signed i64.
    return {dot(w,w)-d,-2*dot(w,A),-2*dot(w,B)};
  }

  void prepare_hull(const Box3& box) {
    std::array<Projection,9> points{};
    for (std::size_t corner=0;corner<8;++corner) {
      Vec raw{};
      for (std::size_t i=0;i<3;++i)
        raw[i]=2*static_cast<i64>((corner&(std::size_t{1}<<i))?box.high[i]:box.low[i])-midpoint_twice[i];
      const i64 along=dot(raw,v);
      points[corner+1]={static_cast<i128>(d)*raw[axis_i]-static_cast<i128>(along)*v[axis_i],
                       static_cast<i128>(d)*raw[axis_j]-static_cast<i128>(along)*v[axis_j],raw};
    }
    counter_add(work.projection_points,9);
    std::sort(points.begin(),points.end(),[&](const auto& a,const auto& b) {
      counter_add(work.hull_sort_comparisons);
      return a.x<b.x || (a.x==b.x && a.y<b.y);
    });
    const auto end=std::unique(points.begin(),points.end(),[](const auto& a,const auto& b) {
      return a.x==b.x && a.y==b.y;
    });
    const auto count=static_cast<std::size_t>(end-points.begin());
    if (count<3) return;  // Degenerate hull: keep disk, never infer empty.
    const auto turn=[&](const Projection& a,const Projection& b,const Projection& c) {
      counter_add(work.hull_orientation_tests);
      // <=1152*M^6<2^119 for M=262143; promotion already precedes products.
      return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
    };
    std::array<Projection,18> hull{};
    std::size_t used=0;
    for (std::size_t i=0;i<count;++i) {
      while (used>=2 && turn(hull[used-2],hull[used-1],points[i])<=0) --used;
      hull[used++]=points[i];
    }
    const auto lower=used;
    for (std::size_t i=count-1;i>0;--i) {
      while (used>lower && turn(hull[used-2],hull[used-1],points[i-1])<=0) --used;
      hull[used++]=points[i-1];
    }
    --used;
    if (used<3) return;
    counter_add(work.hull_vertices,static_cast<u64>(used));
    for (std::size_t i=0;i<used;++i) {
      Vec delta{};
      for (std::size_t j=0;j<3;++j) delta[j]=hull[(i+1)%used].raw[j]-hull[i].raw[j];
      Vec normal=cross(v,delta);
      i128 height=0;
      for (std::size_t j=0;j<3;++j) height+=static_cast<i128>(normal[j])*hull[i].raw[j];
      i128 interior=-height*static_cast<i128>(used);
      for (std::size_t j=0;j<used;++j)
        for (std::size_t a=0;a<3;++a) interior+=static_cast<i128>(normal[a])*hull[j].raw[a];
      if (interior==0) throw std::logic_error("mhgp9 gen center hull invalid supporting facet");
      if (interior>0) { for (auto& x:normal) x=-x; height=-height; }
      facets[facet_count++]={normal,height};
      counter_add(work.facets);
    }
  }

  bool outside(Cell cell) {
    counter_add(work.disk_tests);
    i128 norm=0;
    for (std::size_t i=0;i<3;++i) {
      const auto [low,high]=linear_bounds(0,A[i],B[i],cell);
      const i128 nearest=low>0?low:(high<0?high:0);
      norm+=nearest*nearest;
    }
    // |alpha|,|beta|<=2q; 2*norm<=96*M^2*q^2<2^127 for q=2^42 and M=262143.
    if (2*norm>static_cast<i128>(d)*scale*scale) return true;
    if (domain && domain->completion_count()<2) return true;
    for (std::size_t i=0;i<facet_count;++i) {
      counter_add(work.facet_tests);
      const auto& f=facets[i];
      i128 x=0,y=0;
      for (std::size_t j=0;j<3;++j) {
        x+=static_cast<i128>(f.normal[j])*A[j];
        y+=static_cast<i128>(f.normal[j])*B[j];
      }
      if (linear_bounds(-f.height*scale,x,y,cell).first>0) return true;
    }
    return false;
  }

  std::size_t retained_bytes() const {
    u64 result=static_cast<u64>(bytes(nodes.capacity(),sizeof(Node)));
    counter_add(result,static_cast<u64>(bytes(forms.capacity(),sizeof(Form))));
    counter_add(result,static_cast<u64>(pending_bytes));
    if (result>std::numeric_limits<std::size_t>::max())
      throw std::overflow_error("mhgp9 gen center map memory size overflow");
    return static_cast<std::size_t>(result);
  }
  void observe_memory() {
    work.peak_pending_bytes=std::max(work.peak_pending_bytes,static_cast<u64>(pending_bytes));
    work.peak_retained_bytes=std::max(work.peak_retained_bytes,static_cast<u64>(retained_bytes()));
    last_query_peak=std::max(last_query_peak,retained_bytes());
  }
  void release_pending(Node& node) {
    pending_bytes-=bytes(node.pending.capacity(),sizeof(std::size_t));
    std::vector<std::size_t>().swap(node.pending);
  }
  void evaluate(std::size_t id) {
    auto& node=nodes[id];
    if (node.state!=State::Fresh) return;
    counter_add(work.cells_evaluated);
    if (outside(node.cell)) {
      node.state=State::Outside; counter_add(work.outside_cells); release_pending(node); return;
    }
    std::size_t retained=0;
    for (const auto witness:node.pending) {
      counter_add(work.witness_tests);
      const auto [low,high]=bounds(forms[witness],node.cell);
      if (high<0) { ++node.credit; counter_add(work.inside_credits); }
      else if (low>=0) counter_add(work.outside_witnesses);
      else node.pending[retained++]=witness;
      if (node.credit>=kmax-2) {
        node.state=State::Deep; counter_add(work.deep_cells); release_pending(node); return;
      }
    }
    node.pending.resize(retained);
    node.state=State::Unknown;
  }
  void split(std::size_t id) {
    const auto cell=nodes[id].cell;
    const auto mid_x=cell.left+(cell.right-cell.left)/2;
    const auto mid_y=cell.bottom+(cell.top-cell.bottom)/2;
    const auto first=nodes.size();
    // References into nodes never survive push_back. Parents stay intact until
    // all four children have been built; an allocation failure poisons cache.
    for (unsigned child=0;child<4;++child) {
      Node next{{child&1U?mid_x:cell.left,child&1U?cell.right:mid_x,
                 child&2U?mid_y:cell.bottom,child&2U?cell.top:mid_y},
                nodes[id].pending,nodes[id].credit,absent,nodes[id].depth+1,State::Fresh};
      const auto capacity=bytes(next.pending.capacity(),sizeof(std::size_t));
      counter_add(work.pending_ids_copied,static_cast<u64>(next.pending.size()));
      if (!next.pending.empty()) counter_add(work.pending_lists_created);
      const auto old_capacity=nodes.capacity();
      nodes.push_back(std::move(next));
      if (old_capacity!=nodes.capacity()) counter_add(work.node_storage_growths);
      if (capacity>std::numeric_limits<std::size_t>::max()-pending_bytes)
        throw std::overflow_error("mhgp9 gen center pending capacity overflow");
      pending_bytes+=capacity;
      counter_add(work.cells_created);
      work.max_depth=std::max(work.max_depth,static_cast<u64>(nodes.back().depth));
      observe_memory();
    }
    nodes[id].children=first;
    release_pending(nodes[id]);
    counter_add(work.splits);
  }
  bool query(std::size_t id,Form line) {
    counter_add(work.query_visits);
    if (nodes[id].state==State::Deep || nodes[id].state==State::Outside) return true;
    counter_add(work.line_tests);
    const auto [low,high]=bounds(line,nodes[id].cell);
    if (low>0 || high<0) { counter_add(work.line_skips); return true; }
    // A miss above is LOCAL. No global node state has been modified.
    evaluate(id);
    if (nodes[id].state==State::Deep || nodes[id].state==State::Outside) return true;
    if (nodes[id].children==absent) {
      if (nodes[id].pending.empty()) { counter_add(work.empty_stops); return false; }
      if (nodes[id].depth==options.max_depth) { counter_add(work.depth_stops); return false; }
      if (options.node_budget-nodes.size()<4) { counter_add(work.budget_stops); return false; }
      split(id);
    }
    const auto first=nodes[id].children;
    for (std::size_t i=0;i<4;++i) if (!query(first+i,line)) return false;
    bool global=true,any_deep=false;
    for (std::size_t i=0;i<4;++i) {
      const auto state=nodes[first+i].state;
      global=global && (state==State::Deep || state==State::Outside);
      any_deep=any_deep || state==State::Deep;
    }
    if (global) {
      nodes[id].state=any_deep?State::Deep:State::Outside;
      if (any_deep) counter_add(work.compressed_deep);
      else counter_add(work.compressed_outside);
    }
    return true;
  }
};

Q4CenterMap::Q4CenterMap(std::unique_ptr<Impl> impl):impl_(std::move(impl)) {}
Q4CenterMap::~Q4CenterMap()=default;
std::unique_ptr<Q4CenterMap> Q4CenterMap::make(
    Q34WitnessPoolPtr pool,std::size_t kmax,Q4CenterMapOptions options) {
  validate_q4_center_map_options(options);
  if (!pool || kmax<3) throw std::invalid_argument("mhgp9 gen center map requires pool and K>=3");
  return std::unique_ptr<Q4CenterMap>(new Q4CenterMap(std::make_unique<Impl>(std::move(pool),kmax,options)));
}
const Q4CenterMapWork& Q4CenterMap::work() const noexcept { return impl_->work; }
const Q34WitnessPoolPtr& Q4CenterMap::pool() const noexcept { return impl_->owner; }
std::size_t Q4CenterMap::retained_bytes() const { return impl_->retained_bytes(); }
std::size_t Q4CenterMap::last_query_peak_bytes() const noexcept { return impl_->last_query_peak; }
bool Q4CenterMap::reject_seed(std::size_t x_id) {
  auto& p=*impl_;
  if (p.failed) throw std::logic_error("mhgp9 gen failed center map cannot be resumed");
  const auto points=p.owner->cover()->index()->cloud().points();
  if (x_id>=points.size()) throw std::out_of_range("mhgp9 gen center map seed ID outside cloud");
  const auto edge=p.owner->cover()->edge_ids();
  const auto a=points[edge[0]],b=points[edge[1]],x=points[x_id];
  const auto d=distance(a,b),e=distance(a,x),f=distance(b,x);
  if (d+e<=f || d+f<=e || e+f<=d)
    throw std::invalid_argument("mhgp9 gen center map seed must be strictly acute");
  try {
    p.last_query_peak=0;
    p.observe_memory();
    counter_add(p.work.queries);
    const auto owner=std::minmax(edge[0],edge[1]);
    for (const auto& [length,u]:std::array<std::pair<i64,std::size_t>,2>{{{e,edge[0]},{f,edge[1]}}}) {
      counter_add(p.work.seed_owner_tests);
      const auto alternative=std::minmax(u,x_id);
      if (length>d || (length==d && alternative<owner)) {
        counter_add(p.work.seed_owner_rejections); counter_add(p.work.unknown_queries); return false;
      }
    }
    const bool rejected=!p.nodes.empty() && p.query(0,p.form(x));
    if (rejected) counter_add(p.work.rejected_queries);
    else counter_add(p.work.unknown_queries);
    p.observe_memory();
    return rejected;
  } catch (...) { p.failed=true; throw; }
}

}  // namespace mhgp9::gen
