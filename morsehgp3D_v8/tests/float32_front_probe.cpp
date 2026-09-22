#include "wspd/float32_front.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace {
// Independent native oracle restricted to INTEGRAL coordinates [-64,64].
// Difference<=128, H<=49152, Xi<=12*128^4, and 3H^2<2^33: i64 is ample.
// Arbitrary binary32 geometry is judged by the separate Python Fraction gate.
using Big = std::int64_t;
using V = std::array<Big, 3>;
using Words = mhgp8::Float32Words;
using Point = mhgp8::Float32Point3;
using Box = mhgp8::Float32Box3;
using Mode = mhgp8::Float32PredicateMode;
using Witness = mhgp8::Float32FrontWitnessMode;
std::uint64_t checks{}, scalar_queries{}, box_queries{}, front_queries{}, rejected_pairs{};

void check(bool value, const char* message) {
  ++checks;
  if (!value) throw std::runtime_error(message);
}
Words words(float x, float y, float z) {
  return {std::bit_cast<std::uint32_t>(x), std::bit_cast<std::uint32_t>(y), std::bit_cast<std::uint32_t>(z)};
}
Point point(float x, float y, float z) { return Point::from_bits(words(x, y, z)); }
V integer(const Words& w) {
  V result{};
  for (std::size_t i = 0; i != 3; ++i) {
    const auto e = (w[i] >> 23) & 255U;
    const auto mantissa=(w[i]&0x7fffffU)|(e==0?0U:0x800000U);
    if(mantissa==0)continue;
    const int shift=static_cast<int>(e)-150;
    if(e==0 || shift>=0 || shift < -24)throw std::logic_error("native oracle domain is integral [-64,64]");
    const auto divisor=std::uint32_t{1}<<static_cast<unsigned>(-shift);
    if(mantissa%divisor!=0)throw std::logic_error("native oracle does not accept fractions");
    result[i]=mantissa/divisor;
    if(result[i]>64)throw std::logic_error("native oracle exceeds coordinate bound64");
    if ((w[i] >> 31) != 0) result[i] = -result[i];
  }
  return result;
}
V sub(const V& a, const V& b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
Big dot(const V& a, const V& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Big dist(const V& a, const V& b) { const auto d = sub(a,b); return dot(d,d); }
bool citron(const Words& a, const Words& b, const Words& z) {
  const auto ai = integer(a), bi = integer(b), zi = integer(z);
  const auto za = sub(zi, ai), bz = sub(bi, zi);
  const Big h = dot(za, bz);
  // Lagrange identity, independent of the native explicit cross-product.
  const Big xi = dot(za, za)*dot(bz, bz)-h*h;
  return h > 0 && 3*h*h > xi;
}
std::array<std::size_t, 2> pair(std::size_t a, std::size_t b) { return {std::min(a,b),std::max(a,b)}; }
bool owned(const std::vector<Words>& p, std::size_t a, std::size_t b, std::size_t x) {
  if (x == a || x == b) return false;
  const auto ai = integer(p[a]), bi = integer(p[b]), xi = integer(p[x]);
  const Big ab = dist(ai,bi), ax = dist(ai,xi), bx = dist(bi,xi);
  return (ab > ax || (ab == ax && pair(a,b)<pair(a,x))) &&
         (ab > bx || (ab == bx && pair(a,b)<pair(b,x)));
}
bool acute(const Words& a, const Words& b, const Words& x) {
  const auto ai=integer(a),bi=integer(b),xi=integer(x);
  const Big ab=dist(ai,bi),ax=dist(ai,xi),bx=dist(bi,xi);
  return ab+ax>bx && ab+bx>ax && ax+bx>ab;
}
bool separated(const Box& a, const Box& b, std::uint32_t s) {
  const auto al=integer(a.low()),ah=integer(a.high()),bl=integer(b.low()),bh=integer(b.high());
  Big gap=0;
  for(std::size_t i=0;i!=3;++i) {
    const Big d=std::max(Big(0),std::max(Big(al[i]-bh[i]),Big(bl[i]-ah[i])));
    gap+=d*d;
  }
  const Big da=dist(al,ah),db=dist(bl,bh),largest=std::max(da,db);
  const std::uint64_t scale=static_cast<std::uint64_t>(s)*s;
  return largest==0 || scale<=static_cast<std::uint64_t>(gap/largest);
}
void geometry_cases() {
  const std::vector<std::array<Words,3>> inputs{
      {words(0,0,0),words(10,0,0),words(6,8,0)},
      {words(0,0,0),words(10,0,0),words(5,0,0)},
      {words(-2,0,0),words(2,0,0),words(0,1,0)},
      {words(-2,0,0),words(2,0,0),words(1,1,1)},
      {words(-1,-1,0),words(1,0,1),words(0,0,0)}, // H=1, Xi=3: exact strict contact.
      {words(0,0,0),words(2,0,0),words(2,0,0)},
  };
  for(const auto& input:inputs) {
    const std::vector<Words> p(input.begin(),input.end());
    for(const auto mode:{Mode::Filtered,Mode::ExactOnly}) {
      mhgp8::Float32EdgeWork work;
      const auto a=Point::from_bits(input[0]),b=Point::from_bits(input[1]),z=Point::from_bits(input[2]);
      const auto edge=mhgp8::Float32EdgeGeometry::make(a,0,b,1,mode,work);
      check(edge.citron(z,work)==citron(input[0],input[1],input[2]),"point citron differs from integer oracle");
      check(edge.owns(z,2,work)==owned(p,0,1,2),"ownership differs from integer oracle");
      const auto box=Box::from_corners(z.bits(),z.bits());
      check(edge.citron(box,work)==(citron(input[0],input[1],input[2])?-1:1),"singleton citron undecided or wrong");
      scalar_queries+=3;
    }
    const auto a=Box::from_corners(input[0],input[0]),b=Box::from_corners(input[1],input[1]);
    const auto z=Box::from_corners(input[2],input[2]);
    mhgp8::Float32EdgeWork work;
    check(mhgp8::float32_q3_citron_boxes(a,b,z,work)==(citron(input[0],input[1],input[2])?-1:1),
          "general singleton citron differs from integer oracle");
    ++scalar_queries;
  }
  const std::vector<Box> boxes{
      Box::from_corners(words(-2,-2,-2),words(2,2,2)),
      Box::from_corners(words(20,0,0),words(22,1,1)),
      Box::from_corners(words(0,0,0),words(0,0,0)),
      Box::from_corners(Words{0x80000000U,0,0},Words{0,0,0}),
  };
  for(const auto& a:boxes) for(const auto& b:boxes)
    for(const auto s:{1U,8U,10U,12U,std::numeric_limits<std::uint32_t>::max()}) {
      mhgp8::Float32EdgeWork w;
      check(mhgp8::float32_boxes_separated(a,b,s,w)==separated(a,b,s),"separation differs from integer oracle");
      ++box_queries;
    }
  mhgp8::Float32EdgeWork equality_work;
  check(mhgp8::float32_boxes_separated(
      Box::from_corners(words(0,0,0),words(1,0,0)),
      Box::from_corners(words(9,0,0),words(10,0,0)),8,equality_work),"separation equality lost");
  ++box_queries;
  // Literal symbolic cases are not sent through the bounded integer oracle:
  // symmetric endpoints have midpoint strict in the citron and short halves;
  // scaled tiny edge (1,0)-(7,0), witness(4,1), has H=8 and Xi=36.
  for(const auto& input:std::vector<std::array<Words,3>>{
      {Words{0xff7fffffU,0,0},Words{0x7f7fffffU,0,0},Words{0,0,0}},
      {Words{1,0,0},Words{7,0,0},Words{4,1,0}}}) {
    mhgp8::Float32EdgeWork work;
    const auto a=Point::from_bits(input[0]),b=Point::from_bits(input[1]),z=Point::from_bits(input[2]);
    const auto edge=mhgp8::Float32EdgeGeometry::make(a,0,b,1,Mode::Filtered,work);
    check(edge.citron(z,work),"symbolic extreme midpoint citron lost");
    check(edge.owns(z,2,work),"symbolic extreme owner lost");
    check(!edge.citron(a,work),"symbolic extreme endpoint wrongly strict");
    check(mhgp8::float32_boxes_separated(Box::from_corners(a.bits(),a.bits()),
          Box::from_corners(b.bits(),b.bits()),std::numeric_limits<std::uint32_t>::max(),work),
          "symbolic extreme singleton separation lost");
    scalar_queries+=4;
  }
}
void grid_boxes() {
  const auto a=point(-3,0,0),b=point(3,0,0);
  mhgp8::Float32EdgeWork work;
  const auto edge=mhgp8::Float32EdgeGeometry::make(a,0,b,1,Mode::Filtered,work);
  for(int ix=-4;ix<=4;ix+=2) for(int iy=-4;iy<=4;iy+=2) {
    const auto low=words(static_cast<float>(ix),static_cast<float>(iy),-1);
    const auto high=words(static_cast<float>(ix+1),static_cast<float>(iy+1),1);
    const auto box=Box::from_corners(low,high);
    const int certificate=edge.citron(box,work);
    const bool keep=edge.may_own(box,work);
    const auto general=mhgp8::float32_q3_citron_boxes(
        Box::from_corners(words(-4,-1,0),a.bits()),Box::from_corners(b.bits(),words(4,1,0)),box,work);
    for(int x=ix;x<=ix+1;++x) for(int y=iy;y<=iy+1;++y) for(int z=-1;z<=1;++z) {
      const auto q=words(static_cast<float>(x),static_cast<float>(y),static_cast<float>(z));
      const bool witness=citron(a.bits(),b.bits(),q);
      check(certificate==0 || (certificate<0)==witness,"box citron false certificate");
      const std::vector<Words> points{a.bits(),b.bits(),q};
      check(keep || !owned(points,0,1,2) || !acute(a.bits(),b.bits(),q),"box discarded an owned acute seed");
      for(int aa=-4;aa<=-3;++aa) for(int bb=3;bb<=4;++bb)
        for(int ay=-1;ay<=0;++ay) for(int by=0;by<=1;++by) {
          const bool inside=citron(words(static_cast<float>(aa),static_cast<float>(ay),0),
                                   words(static_cast<float>(bb),static_cast<float>(by),0),q);
          check(general==0 || (general<0)==inside,"general box citron false certificate");
        }
    }
    ++box_queries;
  }
}
void front(const std::vector<Words>& input,std::size_t k,std::uint32_t s,Witness witness) {
  auto index=mhgp8::prepare_float32_index(input);
  std::vector<unsigned> seen(input.size()*input.size());
  mhgp8::Float32FrontWork work;
  mhgp8::run_float32_front(index,k,s,{witness},[&](const auto& r){
    const auto& a=index->nodes()[r.node_a];const auto& b=index->nodes()[r.node_b];
    check(separated(a.box,b.box,s),"front emitted nonseparated product");
    check(r.pairs==(a.last-a.first)*(b.last-b.first),"rectangle pair mass wrong");
    for(auto i=a.first;i!=a.last;++i)for(auto j=b.first;j!=b.last;++j){
      const auto ids=pair(index->permutation()[i],index->permutation()[j]);
      check(ids[0]!=ids[1],"front emitted diagonal pair");
      ++seen[ids[0]*input.size()+ids[1]];
    }
  },work);
  std::size_t lost=0,residual=0;
  for(std::size_t a=0;a<input.size();++a)for(std::size_t b=a+1;b<input.size();++b) {
    check(seen[a*input.size()+b]<=1,"front duplicated unordered pair");
    if(k==1){check(seen[a*input.size()+b]==0,"K1 emitted pairs");continue;}
    if(seen[a*input.size()+b]){++residual;continue;}
    std::size_t count=0;
    for(const auto& p:input)count+=citron(input[a],input[b],p)?1U:0U;
    check(witness==Witness::MidpointSamples && count>=k-1,"front lost an uncertified pair");
    ++lost;
  }
  check(work.rejected_pairs==lost && work.residual_pairs==residual,"front pair ledger differs");
  check(k==1 || lost+residual==input.size()*(input.size()-1)/2,"front does not partition all pairs");
  check(work.product_visits==work.diagonal_splits+work.diagonal_leaves+work.disjoint_splits+
        work.rejected_products+work.emitted_rectangles,"front visit partition");
  rejected_pairs+=lost;
  ++front_queries;
}
void selftest() {
  geometry_cases();grid_boxes();
  std::vector<Words> points;
  for(int i=0;i<17;++i)points.push_back(words(static_cast<float>(i-8),static_cast<float>((i*i)%5),static_cast<float>(i%3)));
  for(const auto k:{std::size_t{1},std::size_t{2},std::size_t{5},std::numeric_limits<std::size_t>::max()})
    for(const auto s:{8U,10U,12U})for(const auto w:{Witness::Disabled,Witness::MidpointSamples})front(points,k,s,w);
  std::vector<Words> large;
  for(int i=0;i!=300;++i)large.push_back(words(static_cast<float>(i%17-8),
      static_cast<float>((i/17)%17-8),static_cast<float>(i/289)));
  front(large,std::numeric_limits<std::size_t>::max(),8,Witness::Disabled);
  front({words(0,0,0)},2,12,Witness::MidpointSamples);
  front({words(0,0,0),words(1,0,0)},2,12,Witness::MidpointSamples);
  const int original=std::fegetround();
#if defined(__SSE__)
  const auto mxcsr=_mm_getcsr();
  const unsigned flush_modes=4;
#else
  const unsigned flush_modes=1;
#endif
  for(const int rounding:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}) {
    check(std::fesetround(rounding)==0,"cannot set rounding");
    for(unsigned flush=0;flush<flush_modes;++flush) {
#if defined(__SSE__)
      const auto control=_mm_getcsr();
      _mm_setcsr((control&~0x8040U)|((flush&1U)?0x8000U:0U)|((flush&2U)?0x40U:0U));
#endif
      geometry_cases();
    }
  }
  check(std::fesetround(original)==0,"cannot restore rounding");
#if defined(__SSE__)
  _mm_setcsr(mxcsr);
#endif
  mhgp8::Float32FrontWork w;
  auto owner=mhgp8::prepare_float32_index(points);
  std::size_t callbacks=0;
  mhgp8::Float32FrontConsumer cb=[&](const auto&){++callbacks;owner.reset();cb={};};
  mhgp8::run_float32_front(owner,2,8,{},cb,w);
  check(callbacks>0 && !owner && !cb,"front ownership/callback snapshot failed");
  owner=mhgp8::prepare_float32_index(points);
  bool thrown=false;
  try{mhgp8::run_float32_front(owner,2,8,{},[](const auto&){throw std::runtime_error("callback");},w);}
  catch(const std::runtime_error&){thrown=true;}
  check(thrown,"callback exception lost");
  for(unsigned which=0;which!=4;++which){
    mhgp8::Float32FrontWork untouched;
    bool invalid=false;
    try{mhgp8::run_float32_front(which==0?nullptr:owner,which==1?0:1,which==2?0:8,
        {which==3?static_cast<Witness>(99):Witness::Disabled},[](const auto&){},untouched);}
    catch(const std::invalid_argument&){invalid=true;}
    check(invalid && untouched==mhgp8::Float32FrontWork{},"invalid inactive front modified work");
  }
  mhgp8::Float32FrontWork overflow;overflow.queries=std::numeric_limits<std::uint64_t>::max();
  thrown=false;
  try{mhgp8::run_float32_front(owner,1,8,{},[](const auto&){},overflow);}
  catch(const std::overflow_error&){thrown=true;}
  check(thrown,"counter overflow not refused");
  std::array<std::future<std::uint64_t>,4> jobs;
  for(auto& job:jobs)job=std::async(std::launch::async,[owner]{
    mhgp8::Float32FrontWork local;
    mhgp8::run_float32_front(owner,5,10,{},[](const auto&){},local);
    return local.residual_pairs;
  });
  for(auto& job:jobs)check(job.get()==points.size()*(points.size()-1)/2,"concurrent front differs");
  check(rejected_pairs>0,"nonvacuity: no rejected pair");
  std::cout<<"{\"schema\":\"mhgp8_float32_front_selftest_v1\",\"status\":\"passed\",\"checks\":"<<checks
           <<",\"scalar_queries\":"<<scalar_queries<<",\"box_queries\":"<<box_queries
           <<",\"front_queries\":"<<front_queries<<",\"rejected_pairs\":"<<rejected_pairs
           <<",\"flush_modes\":"<<flush_modes<<",\"parallel_calls\":4,\"callback_failures\":1}\n";
}
}
int main(int argc,char** argv) {
  try{
    if(argc!=2 || std::string_view(argv[1])!="--selftest")throw std::invalid_argument("expected --selftest");
    selftest();return 0;
  }catch(const std::exception& e){std::cerr<<"float32 front probe: "<<e.what()<<'\n';return 1;}
}
