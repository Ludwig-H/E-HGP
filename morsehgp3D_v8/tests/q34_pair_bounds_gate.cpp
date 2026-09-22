#include "lanes/q34_pair_bounds.hpp"

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <future>
#include <iostream>
#include <limits>
#include <new>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pair_bounds_allocation {
thread_local bool forbidden=false;
[[gnu::noinline]] void* allocate(std::size_t size,std::size_t alignment=alignof(std::max_align_t)) {
  if (forbidden) throw std::bad_alloc();
  void* result=nullptr;
  if (alignment<=alignof(std::max_align_t)) result=std::malloc(size==0?1:size);
  else if (posix_memalign(&result,alignment,size==0?1:size)!=0) result=nullptr;
  if (!result) throw std::bad_alloc();
  return result;
}
[[gnu::noinline]] void release(void* value) noexcept {std::free(value);}
}
void* operator new(std::size_t s) {return pair_bounds_allocation::allocate(s);}
void* operator new[](std::size_t s) {return pair_bounds_allocation::allocate(s);}
void* operator new(std::size_t s,std::align_val_t a) {return pair_bounds_allocation::allocate(s,static_cast<std::size_t>(a));}
void* operator new[](std::size_t s,std::align_val_t a) {return pair_bounds_allocation::allocate(s,static_cast<std::size_t>(a));}
void operator delete(void* p) noexcept {pair_bounds_allocation::release(p);}
void operator delete[](void* p) noexcept {pair_bounds_allocation::release(p);}
void operator delete(void* p,std::size_t) noexcept {pair_bounds_allocation::release(p);}
void operator delete[](void* p,std::size_t) noexcept {pair_bounds_allocation::release(p);}
void operator delete(void* p,std::align_val_t) noexcept {pair_bounds_allocation::release(p);}
void operator delete[](void* p,std::align_val_t) noexcept {pair_bounds_allocation::release(p);}
void operator delete(void* p,std::size_t,std::align_val_t) noexcept {pair_bounds_allocation::release(p);}
void operator delete[](void* p,std::size_t,std::align_val_t) noexcept {pair_bounds_allocation::release(p);}

namespace {
using mhgp8::Point3;
using mhgp8::Box3;
using mhgp8::u64;
using Big=boost::multiprecision::cpp_int;
using Vector=std::array<Big,3>;
using Bounds=mhgp8::PreparedPairCitronBounds;

struct Gate {
  u64 checks{},queries{},corner_evaluations{},sample_evaluations{},singleton_boxes{},nondegenerate_boxes{};
  u64 half_integer_summits{},cross_zero_components{},positive_xi_lower{},wide_xi{},wide_h_squares{};
  u64 reversed_pairs{},coincident_pairs{},extreme_cases{},axis_permutations{},invalid_inputs{};
  u64 allocation_free_calls{},parallel_calls{};
  // 18-bit twins: floors provably unreachable by 16-bit inputs (see check()).
  u64 extreme_cases_u18{},wide_xi_u18{},wide_h_squares_u18{};
  void require(bool value,const char* message) {++checks;if (!value) throw std::runtime_error(message);}
};

Vector twice(Point3 p) {return {2*Big(p.x),2*Big(p.y),2*Big(p.z)};}
Big h4(Point3 a,Point3 b,const Vector& z2) {
  Big value=0;
  for (std::size_t axis=0;axis<3;++axis)
    value+=(z2[axis]-2*Big(a[axis]))*(2*Big(b[axis])-z2[axis]);
  return value;
}
Vector cross(Point3 a,Point3 b,Point3 z) {
  const Vector u{Big(z.x)-a.x,Big(z.y)-a.y,Big(z.z)-a.z};
  const Vector v{Big(b.x)-a.x,Big(b.y)-a.y,Big(b.z)-a.z};
  return {u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
}
Big gram_xi(Point3 a,Point3 b,Point3 z) {
  Big uu=0,vv=0,uv=0;
  for (std::size_t axis=0;axis<3;++axis) {
    const Big u=Big(z[axis])-a[axis],v=Big(b[axis])-a[axis];
    uu+=u*u;vv+=v*v;uv+=u*v;
  }
  return Big(uu*vv-uv*uv);
}
std::array<Point3,8> corners(const Box3& box) {
  std::array<Point3,8> points{};
  for (unsigned i=0;i<8;++i) points[i]={(i&1U)?box.high.x:box.low.x,
      (i&2U)?box.high.y:box.low.y,(i&4U)?box.high.z:box.low.z};
  return points;
}

struct Expected {Big hlow,hhigh,xlow,xhigh;};
Expected oracle(Gate& gate,Point3 a,Point3 b,const Box3& box) {
  const auto vertices=corners(box);
  Big low=h4(a,b,twice(vertices[0])),high=low;
  auto xmin=cross(a,b,vertices[0]),xmax=xmin;
  for (const auto z:vertices) {
    const Big h=h4(a,b,twice(z));low=std::min(low,h);high=std::max(high,h);
    const auto v=cross(a,b,z);
    Big norm=0;
    for (std::size_t axis=0;axis<3;++axis) {
      xmin[axis]=std::min(xmin[axis],v[axis]);xmax[axis]=std::max(xmax[axis],v[axis]);
      norm+=v[axis]*v[axis];
    }
    gate.require(norm==gram_xi(a,b,z),"independent cross and Gram identities disagree");
    ++gate.corner_evaluations;
  }
  // Enumerate all Cartesian products of endpoint/stationary candidates.
  // Coordinates are doubled, so half-integer stationary points stay exact;
  // evaluate the defining dot product, not the prepared product constants.
  std::array<std::array<Big,3>,3> candidates{};
  for (std::size_t axis=0;axis<3;++axis) {
    const Big lo=2*Big(box.low[axis]),hi=2*Big(box.high[axis]);
    const Big middle=Big(a[axis])+b[axis];
    candidates[axis]={lo,hi,std::max(lo,std::min(hi,middle))};
    if (middle>=lo && middle<=hi && (middle%2)!=0) ++gate.half_integer_summits;
  }
  for (const auto& x:candidates[0]) for (const auto& y:candidates[1]) for (const auto& z:candidates[2])
    high=std::max(high,h4(a,b,{x,y,z}));
  Big xlow=0,xhigh=0;
  for (std::size_t axis=0;axis<3;++axis) {
    const Big l=xmin[axis]*xmin[axis],r=xmax[axis]*xmax[axis];
    xhigh+=std::max(l,r);
    if (xmin[axis]<=0 && xmax[axis]>=0) {
      if (xmin[axis]<0 && xmax[axis]>0) ++gate.cross_zero_components;
    } else xlow+=std::min(l,r);
  }
  return {low,high,xlow,xhigh};
}

void check(Gate& gate,Point3 a,Point3 b,const Box3& box) {
  const auto expected=oracle(gate,a,b,box);
  const Bounds prepared(a,b);
  const auto h=prepared.h_bounds(box);
  const auto x=prepared.xi_bounds(box);
  gate.require(Big(h.minimum4)==expected.hlow && Big(h.maximum4)==expected.hhigh,
      "prepared H extrema differ from independent continuous dot-product oracle");
  gate.require(Big(x.low)==expected.xlow && Big(x.high)==expected.xhigh,
      "prepared Xi bounds differ from independent corner-cross oracle");
  ++gate.queries;
  if (box.low==box.high) ++gate.singleton_boxes;else ++gate.nondegenerate_boxes;
  if (expected.xlow>0) ++gate.positive_xi_lower;
  if (expected.xhigh>(Big(1)<<64)) ++gate.wide_xi;
  if (expected.hlow*expected.hlow>(Big(1)<<64) || expected.hhigh*expected.hhigh>(Big(1)<<64)) ++gate.wide_h_squares;
  // On 16-bit inputs Xi<=|u|^2|v|^2<=(3*65535^2)^2<2^67.2 and |H4|<=12*65535^2<2^35.6
  // (H4^2<2^71.2): the two counters below are reachable only by an 18-bit fixture.
  if (expected.xhigh>(Big(1)<<68)) ++gate.wide_xi_u18;
  if (expected.hlow*expected.hlow>(Big(1)<<72) || expected.hhigh*expected.hhigh>(Big(1)<<72)) ++gate.wide_h_squares_u18;
  if (a==b) ++gate.coincident_pairs;
  const Bounds reverse(b,a);
  const auto rh=reverse.h_bounds(box);
  const auto rx=reverse.xi_bounds(box);
  gate.require(rh.minimum4==h.minimum4 && rh.maximum4==h.maximum4 && rx.low==x.low && rx.high==x.high,
      "endpoint reversal changed pair bounds");++gate.reversed_pairs;
  // A three-by-three-by-three integer sample also judges enclosure, not just
  // equality of interval endpoints. This is bounded even for a full 18-bit box.
  std::array<std::array<mhgp8::Coordinate,3>,3> grid{};
  for (std::size_t axis=0;axis<3;++axis)
    grid[axis]={box.low[axis],static_cast<mhgp8::Coordinate>((box.low[axis]+box.high[axis])/2),box.high[axis]};
  for (auto xx:grid[0]) for (auto yy:grid[1]) for (auto zz:grid[2]) {
    const Point3 z{xx,yy,zz};const Big hv=h4(a,b,twice(z)),xv=gram_xi(a,b,z);
    gate.require(Big(h.minimum4)<=hv && hv<=Big(h.maximum4) && Big(x.low)<=xv && xv<=Big(x.high),
      "pair bound excluded an independently evaluated site");++gate.sample_evaluations;
  }
}

void run(Gate& gate) {
  static_assert(!std::is_default_constructible_v<Bounds>);
  const std::array<Point3,6> sites{{{0,0,0},{1,1,1},{3,1,2},{2,4,1},{5,2,4},{4,5,3}}};
  const std::array<Box3,6> boxes{{{{0,0,0},{0,0,0}},{{1,1,1},{1,1,1}},{{0,0,0},{1,1,1}},
      {{1,0,2},{4,4,3}},{{0,0,0},{5,5,5}},{{4,1,0},{5,3,4}}}};
  for (const auto a:sites) for (const auto b:sites) for (const auto& box:boxes) check(gate,a,b,box);
  const std::array<Point3,5> extreme{{{0,0,0},{65535,65535,65535},{65535,0,65535},{0,65535,0},{65535,65534,1}}};
  for (const auto a:extreme) for (const auto b:extreme) {
    check(gate,a,b,{{0,0,0},{65535,65535,65535}});
    check(gate,a,b,{{65534,0,32767},{65535,1,32768}});gate.extreme_cases+=2;
  }
  // 18-bit twins: corners at 262143, the far corner cell and the halving
  // midpoints 131071/131072; the historical 65535 corners become interior sites.
  const std::array<Point3,5> extreme18{{{0,0,0},{262143,262143,262143},{262143,0,262143},{0,262143,0},{262143,262142,1}}};
  for (const auto a:extreme18) for (const auto b:extreme18) {
    check(gate,a,b,{{0,0,0},{262143,262143,262143}});
    check(gate,a,b,{{262142,0,131071},{262143,1,131072}});gate.extreme_cases_u18+=2;
  }
  check(gate,{65535,65535,65535},{262143,262143,262143},{{65534,65534,65534},{262143,262143,262143}});
  check(gate,{0,0,0},{262143,262143,262143},{{65535,65535,65535},{131072,131072,131072}});gate.extreme_cases_u18+=2;
  const Point3 a{100,101,102},b{200,170,140};
  const Box3 box{{80,95,100},{220,190,160}};
  check(gate,a,b,box);
  const auto rotate=[](Point3 p) {return Point3{p.y,p.z,p.x};};
  check(gate,rotate(a),rotate(b),{rotate(box.low),rotate(box.high)});++gate.axis_permutations;
  Bounds prepared(a,b);
  for (const Box3 invalid:std::array<Box3,3>{{{{2,0,0},{1,0,0}},{{0,2,0},{0,1,0}},{{0,0,2},{0,0,1}}}}) {
    bool h=false,x=false;
    try {static_cast<void>(prepared.h_bounds(invalid));} catch (const std::invalid_argument&) {h=true;}
    try {static_cast<void>(prepared.xi_bounds(invalid));} catch (const std::invalid_argument&) {x=true;}
    gate.require(h && x,"invalid box accepted by a prepared query");gate.invalid_inputs+=2;
  }
  const auto h=prepared.h_bounds(box);
  const auto x=prepared.xi_bounds(box);
  pair_bounds_allocation::forbidden=true;
  const Bounds allocation_free(a,b);
  const auto nh=allocation_free.h_bounds(box);
  const auto nx=allocation_free.xi_bounds(box);
  pair_bounds_allocation::forbidden=false;
  gate.require(nh.minimum4==h.minimum4 && nh.maximum4==h.maximum4 && nx.low==x.low && nx.high==x.high,
      "allocation-free prepared queries differ");++gate.allocation_free_calls;
  std::array<std::future<std::array<Big,4>>,4> tasks;
  for (auto& task:tasks) task=std::async(std::launch::async,[prepared,box] {
    const auto hh=prepared.h_bounds(box);
    const auto xx=prepared.xi_bounds(box);
    return std::array<Big,4>{Big(hh.minimum4),Big(hh.maximum4),Big(xx.low),Big(xx.high)};
  });
  const std::array<Big,4> expected{Big(h.minimum4),Big(h.maximum4),Big(x.low),Big(x.high)};
  for (auto& task:tasks) {gate.require(task.get()==expected,"concurrent immutable pair bounds differ");++gate.parallel_calls;}
  gate.require(gate.half_integer_summits && gate.cross_zero_components && gate.positive_xi_lower &&
      gate.wide_xi && gate.wide_h_squares && gate.coincident_pairs && gate.singleton_boxes && gate.nondegenerate_boxes,
      "required pair-bound geometry was not exercised");
  gate.require(gate.extreme_cases_u18==52 && gate.wide_xi_u18 && gate.wide_h_squares_u18,
      "18-bit pair-bound geometry was not exercised");
}
}

int main(int argc,char** argv) {
  try {
    if (argc!=2 || std::string_view(argv[1])!="--selftest") throw std::invalid_argument("expected --selftest");
    Gate gate;run(gate);
    std::cout<<"{\"schema\":\"mhgp8_q34_pair_bounds_gate_v1\",\"status\":\"PASS\"";
#define FIELD(name) std::cout<<",\"" #name "\":"<<gate.name
    FIELD(checks);FIELD(queries);FIELD(corner_evaluations);FIELD(sample_evaluations);FIELD(singleton_boxes);
    FIELD(nondegenerate_boxes);FIELD(half_integer_summits);FIELD(cross_zero_components);FIELD(positive_xi_lower);
    FIELD(wide_xi);FIELD(wide_h_squares);FIELD(reversed_pairs);FIELD(coincident_pairs);FIELD(extreme_cases);
    FIELD(axis_permutations);FIELD(invalid_inputs);FIELD(allocation_free_calls);FIELD(parallel_calls);
    FIELD(extreme_cases_u18);FIELD(wide_xi_u18);FIELD(wide_h_squares_u18);
#undef FIELD
    std::cout<<"}\n";return 0;
  } catch (const std::exception& error) {
    pair_bounds_allocation::forbidden=false;
    std::cerr<<"q34 pair bounds gate: "<<error.what()<<'\n';return 1;
  }
}
