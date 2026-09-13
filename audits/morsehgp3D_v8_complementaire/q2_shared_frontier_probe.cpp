// Audit prototype: fixed A anchor, groups of B queries, persistent Z frontier.
#include "pipeline/axis_q2.hpp"
#include "p0_fixtures.hpp"
#include "sheet_full_fixture.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using Point = mhgp8::Point3;
using Box = mhgp8::Box3;
using u64 = std::uint64_t;
using i64 = std::int64_t;
constexpr auto nil = std::numeric_limits<std::size_t>::max();
void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
Box merge(Box a, Box b) {
  return {{std::min(a.low.x,b.low.x),std::min(a.low.y,b.low.y),std::min(a.low.z,b.low.z)},
          {std::max(a.high.x,b.high.x),std::max(a.high.y,b.high.y),std::max(a.high.z,b.high.z)}};
}
unsigned extent(Box box) {
  return std::max({unsigned(box.high.x-box.low.x), unsigned(box.high.y-box.low.y),
                   unsigned(box.high.z-box.low.z)});
}
struct Node {
  Box box;
  std::size_t first{}, last{}, left{nil}, right{nil};
  std::size_t size() const { return last-first; }
};
class Tree {
 public:
  Tree(std::span<const Point> points, std::vector<std::size_t> order, bool spatial)
      : ids(std::move(order)), points_(points), spatial_(spatial) {
    if(ids.empty()) { require(!spatial,"empty cloud"); return; }
    nodes.reserve(2*ids.size()-1);
    build(0, ids.size());
  }
  std::vector<std::size_t> ids;
  std::vector<Node> nodes;
  u64 point_visits{};
 private:
  std::span<const Point> points_;
  bool spatial_;
  std::size_t build(std::size_t first, std::size_t last) {
    const auto id = nodes.size();
    const auto p = points_[ids[first]];
    nodes.push_back({{p,p},first,last});
    if (last-first==1) { ++point_visits; return id; }
    auto split = first+(last-first)/2;
    if (spatial_) {
      Box box{p,p};
      for (auto i=first; i<last; ++i) {
        box=merge(box,{points_[ids[i]],points_[ids[i]]});
        ++point_visits;
      }
      unsigned axis=0;
      for (unsigned k=1; k<3; ++k)
        if (box.high[k]-box.low[k]>box.high[axis]-box.low[axis]) axis=k;
      const auto middle=(unsigned(box.low[axis])+box.high[axis])/2;
      const auto cut=std::partition(ids.begin()+static_cast<std::ptrdiff_t>(first),
                                    ids.begin()+static_cast<std::ptrdiff_t>(last), [&](auto i) {
        ++point_visits;
        return points_[i][axis]<=middle;
      });
      split=static_cast<std::size_t>(cut-ids.begin());
      require(first<split && split<last, "nonseparating spatial split");
    }
    const auto left=build(first,split), right=build(split,last);
    nodes[id].left=left;
    nodes[id].right=right;
    nodes[id].box=merge(nodes[left].box,nodes[right].box);
    return id;
  }
};

// A single point a, continuous boxes B and Z; independent of product predicates.
std::pair<i64,i64> bounds(Point a, Box b, Box z) {
  i64 low=0, high4=0;
  for (unsigned k=0;k<3;++k) {
    const i64 aa=a[k], zl=z.low[k], zh=z.high[k];
    i64 minimum=std::numeric_limits<i64>::max();
    i64 maximum=std::numeric_limits<i64>::min();
    for (const i64 bb : {i64(b.low[k]),i64(b.high[k])}) {
      for (const i64 zz : {zl,zh}) minimum=std::min(minimum,(zz-aa)*(bb-zz));
      const i64 center2=aa+bb, delta=bb-aa;
      const i64 nearest=std::clamp(center2,2*zl,2*zh)-center2;
      maximum=std::max(maximum,delta*delta-nearest*nearest);
    }
    low+=minimum;
    high4+=maximum;
  }
  return {low,high4};
}
std::pair<i64,i64> point_bounds(Point a, Point b, Box z) {
  i64 radius4=0, minimum=0, maximum=0;
  for (unsigned k=0;k<3;++k) {
    const i64 delta=i64(b[k])-a[k], center2=i64(a[k])+b[k];
    const i64 low=2*i64(z.low[k])-center2, high=2*i64(z.high[k])-center2;
    const i64 near=low>0 ? low : high<0 ? high : 0;
    radius4+=delta*delta;
    minimum+=near*near;
    maximum+=std::max(low*low,high*high);
  }
  // The first coordinate is scaled by four too; only its strict sign is used.
  return {radius4-maximum,radius4-minimum};
}
struct Work {
  u64 classifications{}, box_classifications{}, outside{}, inside{}, mixed{};
  u64 query_splits{}, witness_splits{}, frontier_allocations{}, frontier_peak{};
  u64 cover_visits{}, initial_groups{}, output_groups{}, multi_output_groups{};
  u64 shared_inside_updates{}, shared_outside_updates{}, max_query_depth{};
};
struct Summary {
  std::array<u64,11> histogram{};
  u64 digest{}, digest2{};
  std::vector<unsigned> exact;
};
class Consumer {
 public:
  Consumer(const mhgp8::AxisQ2Plan& plan, unsigned cap, bool check)
      : plan_(plan), points_(plan.rectangle().points()), cap_(cap),
        z_(points_,all_ids(points_.size()),true),
        b_(points_,std::vector<std::size_t>(plan.b_order().begin(),plan.b_order().end()),false),
        check_(check) {
    prefix_.push_back(0);
    for (auto id : b_.ids) prefix_.push_back(prefix_.back()+id+1);
  }
  Summary reference() {
    reset();
    plan_.for_each_candidate([&](auto a,auto b) {
      unsigned count=0;
      point_visit(a,b,0,count);
      emit_point(a,b,count);
    });
    return output;
  }
  Summary shared() {
    reset();
    for (const auto& block:plan_.blocks()) cover(block.a_id,block.b,0);
    require(frontier_.empty(), "frontier storage escaped query");
    return output;
  }
  Work work;
  Summary output;
  u64 z_build_visits() const { return z_.point_visits; }
  u64 z_nodes() const { return z_.nodes.size(); }
  u64 b_build_visits() const { return b_.point_visits; }
  u64 b_nodes() const { return b_.nodes.size(); }
 private:
  const mhgp8::AxisQ2Plan& plan_;
  std::span<const Point> points_;
  unsigned cap_;
  Tree z_,b_;
  bool check_;
  std::vector<u64> prefix_;
  struct Pending { std::size_t z, next; };
  std::vector<Pending> frontier_;
  static std::vector<std::size_t> all_ids(std::size_t n) {
    std::vector<std::size_t> ids(n);
    std::iota(ids.begin(),ids.end(),0);
    return ids;
  }
  void reset() {
    work={}; output={}; frontier_.clear();
    if(check_) output.exact.assign(plan_.total_pairs(),cap_+1);
  }
  void record(std::size_t a,std::size_t b,unsigned p) {
    if(!check_) return;
    const auto ar=plan_.rectangle().a_range(), br=plan_.rectangle().b_range();
    auto& value=output.exact[(a-ar.first)*br.size()+b-br.first];
    require(value==cap_+1,"duplicated emitted pair");
    value=p;
  }
  void emit_point(std::size_t a,std::size_t b,unsigned p) {
    ++output.histogram[p];
    output.digest+=(a+1)*(b+1)*(p+1);
    output.digest2+=(a+1)*(a+1)*(b+1)*(p+1)*(p+1);
    record(a,b,p);
  }
  void emit_group(std::size_t a,const Node& b,unsigned p) {
    ++work.output_groups;
    work.multi_output_groups+=b.size()>1;
    output.histogram[p]+=b.size();
    const u64 sum_b=prefix_[b.last]-prefix_[b.first];
    output.digest+=(a+1)*sum_b*(p+1);
    output.digest2+=(a+1)*(a+1)*sum_b*(p+1)*(p+1);
    if(check_) for(auto i=b.first;i<b.last;++i) record(a,b_.ids[i],p);
  }
  void point_visit(std::size_t a,std::size_t b,std::size_t zid,unsigned& count) {
    if(count>=cap_) return;
    ++work.classifications;
    const auto& z=z_.nodes[zid];
    const auto [lo,hi]=point_bounds(points_[a],points_[b],z.box);
    if(hi<=0) { ++work.outside; return; }
    if(lo>0) { ++work.inside; count+=std::min<std::size_t>(cap_-count,z.size()); return; }
    ++work.mixed;
    require(z.size()>1,"uncertain point pair/site");
    point_visit(a,b,z.left,count);
    point_visit(a,b,z.right,count);
  }
  std::size_t push(std::size_t z,std::size_t next) {
    frontier_.push_back({z,next});
    ++work.frontier_allocations;
    work.frontier_peak=std::max<u64>(work.frontier_peak,frontier_.size());
    return frontier_.size()-1;
  }
  void group(std::size_t a,std::size_t bid,std::size_t head,unsigned count,u64 depth) {
    const auto checkpoint=frontier_.size();
    work.max_query_depth=std::max(work.max_query_depth,depth);
    const auto& b=b_.nodes[bid];
    while(head!=nil && count<cap_) {
      const auto current=frontier_[head];
      const auto& z=z_.nodes[current.z];
      ++work.classifications;
      work.box_classifications+=b.size()>1;
      const auto [lo,hi]=b.size()==1
          ? point_bounds(points_[a],points_[b_.ids[b.first]],z.box)
          : bounds(points_[a],b.box,z.box);
      if(hi<=0) {
        ++work.outside;
        work.shared_outside_updates+=b.size()>1;
        head=current.next;
      } else if(lo>0) {
        ++work.inside;
        work.shared_inside_updates+=b.size()>1;
        count+=std::min<std::size_t>(cap_-count,z.size());
        head=current.next;
      } else {
        ++work.mixed;
        if(b.size()>1 && (z.size()==1 || extent(b.box)>=extent(z.box))) {
          ++work.query_splits;
          // Children inherit the EXACT count and the still-unconsumed frontier.
          group(a,b.left,head,count,depth+1);
          group(a,b.right,head,count,depth+1);
          frontier_.resize(checkpoint);
          return;
        }
        require(z.size()>1,"uncertain singleton task");
        ++work.witness_splits;
        const auto right=push(z.right,current.next);
        head=push(z.left,right);
      }
    }
    emit_group(a,b,count);
    frontier_.resize(checkpoint);
  }
  void cover(std::size_t a,mhgp8::Range range,std::size_t bid) {
    ++work.cover_visits;
    const auto& b=b_.nodes[bid];
    if(range.last<=b.first || range.first>=b.last) return;
    if(range.first<=b.first && b.last<=range.last) {
      ++work.initial_groups;
      require(frontier_.empty(),"initial frontier not empty");
      const auto head=push(0,nil);
      group(a,bid,head,0,0);
      frontier_.clear();
      return;
    }
    require(b.size()>1,"partial leaf cover");
    cover(a,range,b.left);
    cover(a,range,b.right);
  }
};
unsigned oracle(const mhgp8::RectangleInput& input,std::size_t a,std::size_t b) {
  unsigned p=0;
  for(auto z:input.points) {
    i64 h=0;
    for(unsigned k=0;k<3;++k) h+=(i64(z[k])-input.points[a][k])*(i64(input.points[b][k])-z[k]);
    p+=h>0;
  }
  return p;
}
void run(std::string_view family,const mhgp8::RectangleInput& input,unsigned cap,
         bool check,bool restrict) {
  const auto owner=mhgp8::prepare_rectangle(input,cap,12);
  std::optional<mhgp8::CreditPlan> local;
  if(restrict) local.emplace(mhgp8::make_credit_plan(owner,mhgp8::Lane::Q2,mhgp8::Strategy::Pool));
  const auto plan=mhgp8::make_axis_q2_plan(owner,mhgp8::AxisQ2Mode::Additive,local?&*local:nullptr);
  Consumer consumer(plan,cap,check);
  const auto reference=consumer.reference();
  const auto rw=consumer.work;
  const auto shared=consumer.shared();
  const auto sw=consumer.work;
  require(reference.histogram==shared.histogram && reference.digest==shared.digest &&
          reference.digest2==shared.digest2 && reference.exact==shared.exact,
          "shared result differs from individual census");
  require(std::accumulate(shared.histogram.begin(),shared.histogram.end(),u64{0})==plan.candidate_pairs(),
          "residual mass differs");
  u64 oracle_tests=0;
  if(check) {
    for(auto a=input.a.first;a<input.a.last;++a)
      for(auto b=input.b.first;b<input.b.last;++b) {
        const auto truth=oracle(input,a,b);
        oracle_tests+=input.points.size();
        const auto observed=shared.exact[(a-input.a.first)*input.b.size()+b-input.b.first];
        if(observed==cap+1) require(truth>=cap,"prefilter lost survivor");
        else require(observed==std::min(cap,truth),"shared depth differs from independent oracle");
      }
  }
  std::cout<<"{\"family\":\""<<family<<"\",\"n\":"<<input.points.size()<<",\"cap\":"<<cap
    <<",\"pool_restriction\":"<<(restrict?"true":"false")<<",\"candidates\":"<<plan.candidate_pairs()
    <<",\"oracle_tests\":"<<oracle_tests<<",\"reference_visits\":"<<rw.classifications
    <<",\"shared_classifications\":"<<sw.classifications<<",\"shared_box_classifications\":"<<sw.box_classifications
    <<",\"query_splits\":"<<sw.query_splits<<",\"witness_splits\":"<<sw.witness_splits
    <<",\"frontier_allocations\":"<<sw.frontier_allocations<<",\"frontier_peak\":"<<sw.frontier_peak
    <<",\"cover_visits\":"<<sw.cover_visits<<",\"initial_groups\":"<<sw.initial_groups
    <<",\"output_groups\":"<<sw.output_groups<<",\"multi_output_groups\":"<<sw.multi_output_groups
    <<",\"shared_inside_updates\":"<<sw.shared_inside_updates<<",\"shared_outside_updates\":"<<sw.shared_outside_updates
    <<",\"max_query_depth\":"<<sw.max_query_depth<<",\"z_build_visits\":"<<consumer.z_build_visits()
    <<",\"z_nodes\":"<<consumer.z_nodes()<<",\"b_build_visits\":"<<consumer.b_build_visits()
    <<",\"b_nodes\":"<<consumer.b_nodes()<<",\"digest\":"<<shared.digest
    <<",\"digest2\":"<<shared.digest2<<",\"histogram\":[";
  for(unsigned p=0;p<=cap;++p) { if(p)std::cout<<',';std::cout<<shared.histogram[p]; }
  std::cout<<"]}\n"<<std::flush;
}
} // namespace
int main(int argc,char** argv) {
  try {
    if(argc==2 && std::string_view(argv[1])=="--selftest") {
      for(auto family:{"grid","sheet","skew"})
        for(auto n:{32U,70U,128U})
          for(auto cap:{1U,5U,10U})
            for(bool restrict:{false,true}) run(family,mhgp8::bench::make_fixture(n,family),cap,true,restrict);
      const mhgp8::RectangleInput core{{{0,0,0},{50,0,0},{100,0,0}}, {0,1},{2,3},{1}};
      run("core",core,2,true,false);
      run("saturated_core",core,1,true,true);
      run("half_center",{{{0,1,1},{3,2,2},{1,0,1},{2,3,2},{1,1,1}}, {0,1},{1,2},{}},2,true,false);
      return 0;
    }
    if(argc==4 && std::string_view(argv[1])=="--large") {
      const auto n=static_cast<std::size_t>(std::stoul(argv[2]));
      const std::string_view family=argv[3];
      const auto input=family=="sheet_full" ? mhgp8::bench::make_sheet_full_fixture(n) : mhgp8::bench::make_fixture(n,family);
      run(family,input,10,false,family!="sheet_full");
      return 0;
    }
    std::cerr<<"usage: q2_shared_frontier_probe --selftest | --large n family\n";
    return 2;
  } catch(const std::exception& e) { std::cerr<<"q2 shared frontier audit: "<<e.what()<<'\n';return 1; }
}
