#include "lanes/q4_local_partition.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mhgp8 {
namespace {
using Vec = std::array<i64,3>;
constexpr i64 scale = Q4LocalCell::scale;
struct Projection { i128 x{}, y{}; Vec raw{}; };

void validate_cell(Q4LocalCell c) {
  if (c.depth>44 || c.left < -2*scale || c.right > 2*scale || c.left>c.right ||
      c.bottom < -2*scale || c.top > 2*scale || c.bottom>c.top)
    throw std::invalid_argument("mhgp8 local cell outside exact arithmetic domain");
}
i64 dot(Vec a, Vec b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Vec cross(Vec a, Vec b) {
  return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
Q4LocalBounds linear_bounds(i128 c,i128 x,i128 y,Q4LocalCell cell) {
  return {c+x*(x<0?cell.right:cell.left)+y*(y<0?cell.top:cell.bottom),
          c+x*(x<0?cell.left:cell.right)+y*(y<0?cell.bottom:cell.top)};
}
std::size_t id_bytes(std::size_t capacity) {
  if (capacity>std::numeric_limits<std::size_t>::max()/sizeof(std::size_t))
    throw std::overflow_error("mhgp8 local frontier capacity overflow");
  return capacity*sizeof(std::size_t);
}
}  // namespace

Q4LocalGeometryPtr Q4LocalGeometry::make(Q34EdgeCoverPtr cover,Q4CenterDomainMode mode) {
  if (!cover || (mode!=Q4CenterDomainMode::Disk && mode!=Q4CenterDomainMode::Positive))
    throw std::invalid_argument("mhgp8 local geometry requires cover and valid domain");
  return Q4LocalGeometryPtr(new Q4LocalGeometry(std::move(cover),mode));
}

Q4LocalGeometry::Q4LocalGeometry(Q34EdgeCoverPtr cover,Q4CenterDomainMode mode)
    :cover_(std::move(cover)) {
  counter_add(work_.preparations);
  const auto points=cover_->index()->cloud().points();
  const auto ids=cover_->edge_ids();
  const auto a=points[ids[0]],b=points[ids[1]];
  std::size_t main_axis=0;
  for (std::size_t i=0;i<3;++i) {
    v_[i]=static_cast<i64>(b[i])-a[i];
    midpoint_twice_[i]=static_cast<i64>(a[i])+b[i];
    if (std::abs(v_[i])>std::abs(v_[main_axis])) main_axis=i;
  }
  diameter_squared_=dot(v_,v_);
  axis_i_=(main_axis+1)%3;
  axis_j_=(main_axis+2)%3;
  const i64 h=std::abs(v_[main_axis]),sign=v_[main_axis]>0?1:-1;
  a_basis_[axis_i_]=h;a_basis_[main_axis]=-sign*v_[axis_i_];
  b_basis_[axis_j_]=h;b_basis_[main_axis]=-sign*v_[axis_j_];
  if (mode==Q4CenterDomainMode::Positive) {
    domain_=Q4PositiveDomain::make(cover_);
    work_.domain=domain_->work();
    if (domain_->completion_box()) prepare_hull(*domain_->completion_box());
  }
  decompose_cover();
  work_.peak_retained_bytes=static_cast<u64>(retained_bytes());
}

void Q4LocalGeometry::decompose_cover() {
  const auto nodes=cover_->index()->spatial_nodes();
  const auto ranges=cover_->ranges();
  std::size_t cursor=0,range=0;
  while (cursor<nodes.size()) {
    const auto& node=nodes[cursor];
    counter_add(work_.cover_node_visits);
    while (range<ranges.size() && ranges[range].last<=node.range.first) {
      ++range;counter_add(work_.cover_range_advances);
    }
    if (range==ranges.size() || ranges[range].first>=node.range.last) {
      counter_add(work_.cover_disjoint_nodes);
      counter_add(work_.cover_excluded_sites,static_cast<u64>(node.range.size()));
      cursor=node.escape;
    } else if (ranges[range].first<=node.range.first && node.range.last<=ranges[range].last) {
      cover_nodes_.push_back(cursor);
      counter_add(work_.cover_blocks);
      counter_add(work_.cover_node_ids_copied);
      counter_add(work_.cover_sites,static_cast<u64>(node.range.size()));
      cursor=node.escape;
    } else {
      if (node.range.size()==1) throw std::logic_error("mhgp8 integral cover partially intersects a singleton");
      counter_add(work_.cover_splits);
      cursor=node.left;
    }
  }
  // Cover ranges can join several index subtrees. Only a node FULLY within
  // one range is consumed; partial nodes refine before any population use.
  // The preorder frontier is disjoint, spatially ordered and exactly covers
  // the certified ranges. No original-ID/rank search or scalar point scan.
}

Q4LocalForm Q4LocalGeometry::form(std::size_t id) const {
  const auto points=cover_->index()->cloud().points();
  if (id>=points.size()) throw std::out_of_range("mhgp8 local form ID outside cloud");
  Vec w{};
  for (std::size_t i=0;i<3;++i) w[i]=2*static_cast<i64>(points[id][i])-midpoint_twice_[i];
  // u16, M=65535: |w_i|<=2M, basis coordinates <=M and only two nonzero
  // coordinates per basis vector. |c|<=15M^2, |x|,|y|<=8M^2 fit i64.
  return {dot(w,w)-diameter_squared_,-2*dot(w,a_basis_),-2*dot(w,b_basis_)};
}

Q4LocalBounds Q4LocalGeometry::bounds(Q4LocalForm f,Q4LocalCell cell) const {
  validate_cell(cell);
  return linear_bounds(static_cast<i128>(scale)*f.constant,f.x,f.y,cell);
}

void Q4LocalGeometry::prepare_hull(const Box3& box) {
  // Explicit port of the independently qualified tranche27 integer hull;
  // no rejection-cache state or per-site forms are reused here.
  std::array<Projection,9> points{};
  for (std::size_t corner=0;corner<8;++corner) {
    Vec raw{};
    for (std::size_t i=0;i<3;++i)
      raw[i]=2*static_cast<i64>((corner&(std::size_t{1}<<i))?box.high[i]:box.low[i])-midpoint_twice_[i];
    const i64 along=dot(raw,v_);
    points[corner+1]={static_cast<i128>(diameter_squared_)*raw[axis_i_]-static_cast<i128>(along)*v_[axis_i_],
                     static_cast<i128>(diameter_squared_)*raw[axis_j_]-static_cast<i128>(along)*v_[axis_j_],raw};
  }
  counter_add(work_.projection_points,9);
  std::sort(points.begin(),points.end(),[&](const auto& a,const auto& b) {
    counter_add(work_.hull_sort_comparisons);
    return a.x<b.x || (a.x==b.x && a.y<b.y);
  });
  const auto end=std::unique(points.begin(),points.end(),[](const auto& a,const auto& b) {
    return a.x==b.x && a.y==b.y;
  });
  const auto count=static_cast<std::size_t>(end-points.begin());
  if (count<3) return;
  const auto turn=[&](const Projection& a,const Projection& b,const Projection& c) {
    counter_add(work_.hull_orientation_tests);
    // Projected coordinates <=12M^3, differences <=24M^3; orientation
    // <=1152M^6<2^107. Every multiplication already has i128 operands.
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
  if (used<3) return;  // Degenerate hull retains the disk, not an empty domain.
  counter_add(work_.hull_vertices,static_cast<u64>(used));
  for (std::size_t i=0;i<used;++i) {
    Vec delta{};
    for (std::size_t j=0;j<3;++j) delta[j]=hull[(i+1)%used].raw[j]-hull[i].raw[j];
    Vec normal=cross(v_,delta);
    i128 height=0;
    for (std::size_t j=0;j<3;++j) height+=static_cast<i128>(normal[j])*hull[i].raw[j];
    i128 interior=-height*static_cast<i128>(used);
    for (std::size_t j=0;j<used;++j)
      for (std::size_t axis=0;axis<3;++axis) interior+=static_cast<i128>(normal[axis])*hull[j].raw[axis];
    if (interior==0) throw std::logic_error("mhgp8 local hull invalid supporting facet");
    if (interior>0) {for (auto& x:normal) x=-x;height=-height;}
    facets_[facet_count_++]={normal,height};
    counter_add(work_.facets);
  }
}

bool Q4LocalGeometry::outside(Q4LocalCell cell,Q4LocalGeometryQueryWork& work) const {
  validate_cell(cell);
  counter_add(work.disk_tests);
  i128 norm=0;
  for (std::size_t i=0;i<3;++i) {
    const auto bound=linear_bounds(0,a_basis_[i],b_basis_[i],cell);
    const i128 nearest=bound.minimum>0?bound.minimum:(bound.maximum<0?bound.maximum:0);
    norm+=nearest*nearest;
  }
  // |Q*t_i|<=4MQ; 2*norm<=96M^2Q^2<2^127 for Q=2^44.
  if (2*norm>static_cast<i128>(diameter_squared_)*scale*scale) return true;
  if (domain_ && domain_->completion_count()<2) return true;
  for (std::size_t i=0;i<facet_count_;++i) {
    counter_add(work.facet_tests);
    const auto& facet=facets_[i];
    i128 x=0,y=0;
    for (std::size_t j=0;j<3;++j) {
      x+=static_cast<i128>(facet.normal[j])*a_basis_[j];
      y+=static_cast<i128>(facet.normal[j])*b_basis_[j];
    }
    // |n_i|<=4M^2, |H|<=24M^3. Facet test <=64M^3Q<2^99.
    if (linear_bounds(-facet.height*scale,x,y,cell).minimum>0) return true;
  }
  return false;
}

Q4LocalBounds Q4LocalGeometry::node_bounds(std::size_t id,Q4LocalCell cell) const {
  const auto& box=cover_->index()->spatial_nodes()[id].box;
  Q4LocalBounds result{};
  for (unsigned corner=0;corner<4;++corner) {
    const i64 alpha=(corner&1U)?cell.right:cell.left;
    const i64 beta=(corner&2U)?cell.top:cell.bottom;
    i128 minimum=-static_cast<i128>(scale)*diameter_squared_,maximum=minimum;
    for (std::size_t axis=0;axis<3;++axis) {
      const i64 low=2*static_cast<i64>(box.low[axis])-midpoint_twice_[axis];
      const i64 high=2*static_cast<i64>(box.high[axis])-midpoint_twice_[axis];
      const i128 target=static_cast<i128>(a_basis_[axis])*alpha+static_cast<i128>(b_basis_[axis])*beta;
      const auto value=[&](i64 w) {return static_cast<i128>(scale)*w*w-2*static_cast<i128>(w)*target;};
      const i128 at_low=value(low),at_high=value(high);
      maximum+=std::max(at_low,at_high);
      if (target<static_cast<i128>(scale)*low) minimum+=at_low;
      else if (target>static_cast<i128>(scale)*high) minimum+=at_high;
      else {
        // Minimum of Q*w^2-2*T*w is -T^2/Q at w=T/Q. Round DOWN,
        // never toward zero, to keep a certified lower integer bound.
        const i128 square=target*target;
        minimum-=square/scale+(square%scale!=0?1:0);
      }
    }
    if (corner==0) result={minimum,maximum};
    else {result.minimum=std::min(result.minimum,minimum);result.maximum=std::max(result.maximum,maximum);}
  }
  // L is affine in the center: extrema on Z-box x cell reduce to the four
  // center corners. For fixed center the w quadratic is separable/convex;
  // endpoints give max, clamping T/Q gives min. Rounding min only weakens it.
  // |T|<=4MQ -> T^2<=16M^2Q^2<2^124; each evaluated value<=20M^2Q
  // in absolute value, sums<=63M^2Q<2^82. All products promoted beforehand.
  return result;
}

std::size_t Q4LocalGeometry::retained_bytes() const {return id_bytes(cover_nodes_.capacity());}

Q4LocalFragmentPtr Q4LocalFragment::root(Q4LocalGeometryPtr geometry,u64 budget) {
  if (!geometry) throw std::invalid_argument("mhgp8 local root requires immutable geometry");
  const auto input=geometry->cover_nodes();
  return Q4LocalFragmentPtr(new Q4LocalFragment(std::move(geometry),Q4LocalCell{},0,input,budget,Origin::Root));
}

Q4LocalFragmentPtr Q4LocalFragment::child(Q4LocalFragmentPtr parent,unsigned quadrant,u64 budget) {
  if (!parent || quadrant>=4) throw std::invalid_argument("mhgp8 local child requires parent and quadrant0..3");
  const auto p=parent->cell();
  if (p.depth>=44) throw std::invalid_argument("mhgp8 local child exceeds exact cell depth");
  const i64 mx=p.left+(p.right-p.left)/2,my=p.bottom+(p.top-p.bottom)/2;
  const bool right=(quadrant&1U)!=0,top=(quadrant&2U)!=0;
  const Q4LocalCell cell{right?mx:p.left,right?p.right:mx,top?my:p.bottom,top?p.top:my,
                        p.depth+1,right&&p.owns_right,top&&p.owns_top};
  return Q4LocalFragmentPtr(new Q4LocalFragment(parent->geometry(),cell,parent->inside_count(),parent->active_nodes(),budget,Origin::Child));
}

Q4LocalFragmentPtr Q4LocalFragment::refine(Q4LocalFragmentPtr parent,u64 budget) {
  if (!parent) throw std::invalid_argument("mhgp8 local refinement requires immutable parent");
  return Q4LocalFragmentPtr(new Q4LocalFragment(parent->geometry(),parent->cell(),parent->inside_count(),
                                               parent->active_nodes(),budget,Origin::Refine));
}

Q4LocalFragment::Q4LocalFragment(Q4LocalGeometryPtr geometry,Q4LocalCell cell,std::size_t inherited,
    std::span<const std::size_t> input,u64 budget,Origin origin)
    :geometry_(std::move(geometry)),cell_(cell),inside_count_(inherited) {
  switch (origin) {
    case Origin::Root: counter_add(work_.root_factories);break;
    case Origin::Child: counter_add(work_.child_factories);break;
    case Origin::Refine: counter_add(work_.refine_factories);break;
  }
  work_.inherited_inside_sites=static_cast<u64>(inherited);
  const auto& index=*geometry_->cover()->index();
  const auto nodes=index.spatial_nodes();
  u64 tested=0;
  for (const auto input_id:input) {
    counter_add(work_.input_nodes);
    counter_add(work_.input_sites,static_cast<u64>(nodes[input_id].range.size()));
    std::size_t cursor=input_id;
    const auto end=nodes[input_id].escape;
    while (cursor<end) {
      const auto& node=nodes[cursor];
      counter_add(work_.node_visits);
      if (tested==budget) {
        counter_add(work_.budget_unexamined_nodes);
        retain(cursor);cursor=node.escape;continue;
      }
      ++tested;
      Q4LocalBounds bound;
      if (node.range.size()==1) {
        counter_add(work_.point_tests);
        bound=geometry_->bounds(geometry_->form(index.spatial_order()[node.range.first]),cell_);
      } else {
        counter_add(work_.block_bound_tests);
        bound=geometry_->node_bounds(cursor,cell_);
      }
      if (bound.maximum<0) {
        counter_add(work_.inside_nodes);
        counter_add(work_.inside_sites,static_cast<u64>(node.range.size()));
        inside_count_+=node.range.size();cursor=node.escape;
      } else if (bound.minimum>0) {
        counter_add(work_.outside_nodes);
        counter_add(work_.outside_sites,static_cast<u64>(node.range.size()));
        cursor=node.escape;
      } else if (node.range.size()==1) {
        retain(cursor);cursor=node.escape;
      } else if (tested==budget) {
        counter_add(work_.budget_ambiguous_nodes);
        retain(cursor);cursor=node.escape;
      } else {
        counter_add(work_.z_splits);cursor=node.left;
      }
    }
  }
  work_.peak_retained_bytes=static_cast<u64>(retained_bytes());
  // Each input subtree is consumed once, in preorder; retaining/classifying
  // a node skips its complete subtree. Refinement only replaces a node with
  // disjoint children. Counts inherited from removed ancestors never reenter
  // the frontier. Population additions are bounded by the owner's size.
}

void Q4LocalFragment::retain(std::size_t id) {
  active_nodes_.push_back(id);
  const auto population=geometry_->cover()->index()->spatial_nodes()[id].range.size();
  active_sites_+=population;
  counter_add(work_.active_nodes);
  counter_add(work_.active_sites,static_cast<u64>(population));
  counter_add(work_.frontier_ids_copied);
}
std::size_t Q4LocalFragment::retained_bytes() const {return id_bytes(active_nodes_.capacity());}

}  // namespace mhgp8
