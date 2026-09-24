// Audit-only micro-shadow: exact multisite moment rectangle bounds.
#include "gen/pipeline/prepared_cloud.hpp"
#include "gen/lanes/q34_witness_search.hpp"
#include "gen/wspd/front.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace mhgp9::gen;
using i64 = std::int64_t;
using i128 = __int128_t;
using V = std::array<i64, 3>;

struct Moments { i64 n=0, q=0; V z{}; };
struct Interval { i64 lo=0, hi=0; };
static Interval mul(Interval a, Interval b) {
  const std::array<i64,4> p{a.lo*b.lo,a.lo*b.hi,a.hi*b.lo,a.hi*b.hi};
  return {*std::min_element(p.begin(),p.end()),*std::max_element(p.begin(),p.end())};
}
static i64 absmax(Interval x) { return std::max(std::abs(x.lo),std::abs(x.hi)); }
static i64 sq(i64 x) { return x*x; }
static V cross(V a,V b) { return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
static i128 norm2(V x) { return i128(x[0])*x[0]+i128(x[1])*x[1]+i128(x[2])*x[2]; }
static V corner(Box3 box,int j) {
  return {j&1?box.high[0]:box.low[0],j&2?box.high[1]:box.low[1],j&4?box.high[2]:box.low[2]};
}
static std::uint8_t classify(i64 h,i64 d,i128 x,unsigned k,std::uint8_t mask) {
  std::uint8_t result=0;
  if(mask&2) { const i64 a=3*h-4*i64(k-2)*d; if(a>0 && i128(a)*a>12*x) result|=2; }
  if(mask&4) { const i64 a=2*h-3*i64(k-3)*d; if(a>0 && i128(a)*a>8*x) result|=4; }
  return result;
}
static std::uint8_t point_test(V a,V b,Moments m,unsigned k,std::uint8_t mask) {
  V d{},s{},w{}; i64 ab=0,dsq=0,sz=0;
  for(int i=0;i<3;++i) {
    d[i]=b[i]-a[i]; s[i]=a[i]+b[i]; w[i]=2*m.z[i]-m.n*s[i];
    ab+=a[i]*b[i]; dsq+=sq(d[i]); sz+=s[i]*m.z[i];
  }
  const i64 h=4*(sz-m.q-m.n*ab);
  return classify(h,dsq,norm2(cross(d,w)),k,mask);
}
static std::uint8_t all_corners(Box3 a,Box3 b,Moments m,unsigned k,std::uint8_t mask) {
  std::uint8_t yes=mask;
  for(int ai=0;ai<8 && yes;++ai)
    for(int bi=0;bi<8 && yes;++bi) yes &= point_test(corner(a,ai),corner(b,bi),m,k,yes);
  return yes;
}
static std::uint8_t cheap(Box3 a,Box3 b,Moments m,unsigned k,std::uint8_t mask) {
  i64 hsum=-m.q,dhi=0;
  std::array<Interval,3> d{},w{};
  for(int i=0;i<3;++i) {
    const i64 al=a.low[i],ah=a.high[i],bl=b.low[i],bh=b.high[i];
    i64 minphi=std::numeric_limits<i64>::max();
    for(i64 x:{al,ah}) for(i64 y:{bl,bh})
      minphi=std::min(minphi,(x+y)*m.z[i]-m.n*x*y);
    hsum+=minphi;
    const i64 gmax=std::max(std::abs(bh-al),std::abs(ah-bl));
    dhi+=sq(gmax);
    d[i]={bl-ah,bh-al};
    w[i]={2*m.z[i]-m.n*(ah+bh),2*m.z[i]-m.n*(al+bl)};
  }
  const i64 hlo=4*hsum;
  i128 xhi=0;
  for(int i=0;i<3;++i) {
    const int j=(i+1)%3,t=(i+2)%3;
    const auto p=mul(d[j],w[t]),q=mul(d[t],w[j]);
    const Interval ci{p.lo-q.hi,p.hi-q.lo};
    const i64 u=absmax(ci);
    xhi+=i128(u)*u;
  }
  return classify(hlo,dhi,xhi,k,mask);
}
static i64 distance_box4(const std::array<i64,3>& q,Box3 b) {
  i64 d=0;
  for(int i=0;i<3;++i) {
    const i64 lo=4*i64(b.low[i]),hi=4*i64(b.high[i]);
    const i64 t=q[i]<lo?lo-q[i]:(q[i]>hi?q[i]-hi:0);
    d+=t*t;
  }
  return d;
}
static std::size_t choose_group(const Q2CensusIndex& idx,Box3 a,Box3 b) {
  const auto nodes=idx.spatial_nodes();
  std::array<i64,3> target{};
  for(int i=0;i<3;++i) target[i]=i64(a.low[i])+a.high[i]+b.low[i]+b.high[i];
  std::size_t node=0;
  while(nodes[node].range.size()>64) {
    const auto l=nodes[node].left,r=nodes[node].right;
    if(l==Q2SpatialNode::absent) throw std::runtime_error("oversize leaf");
    node=distance_box4(target,nodes[l].box)<=distance_box4(target,nodes[r].box)?l:r;
  }
  return node;
}
static Moments make_moments(const Q2CensusIndex& idx,std::size_t node) {
  Moments m; const auto points=idx.cloud().points(); const auto order=idx.spatial_order();
  const auto range=idx.spatial_nodes()[node].range;
  for(std::size_t r=range.first;r<range.last;++r) {
    const auto p=points[order[r]];
    ++m.n;
    for(int i=0;i<3;++i) {m.z[i]+=p[i];m.q+=sq(p[i]);}
  }
  return m;
}
static std::uint32_t load32(const unsigned char* p) {
  return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
int main(int argc,char** argv) try {
  if(argc!=4&&argc!=5) throw std::runtime_error("usage: shadow points.u32le K min_product [s]");
  const unsigned k=std::stoul(argv[2]);
  const std::uint64_t min_product=std::stoull(argv[3]);
  const unsigned separation_s=argc==5?std::stoul(argv[4]):8;
  if(k<4||k>10) throw std::runtime_error("K outside [4,10]");
  if(separation_s<2||separation_s>32) throw std::runtime_error("s outside [2,32]");
  std::ifstream file(argv[1],std::ios::binary);
  if(!file) throw std::runtime_error("cannot open input");
  const std::vector<unsigned char> raw{std::istreambuf_iterator<char>(file),{}};
  if(raw.size()%12) throw std::runtime_error("bad input length");
  std::vector<Point3> points(raw.size()/12);
  for(std::size_t j=0;j<points.size();++j)
    points[j]={Coordinate(load32(raw.data()+12*j)),Coordinate(load32(raw.data()+12*j+4)),
               Coordinate(load32(raw.data()+12*j+8))};
  const auto index=make_q2_cloud_index(prepare_cloud(points));
  const auto nodes=index->spatial_nodes();
  std::uint64_t front=0,open=0,selected=0,selected_mass=0,corners_both=0,cheap_both=0;
  std::uint64_t corners_3=0,corners_4=0,cheap_3=0,cheap_4=0,groups_small=0;
  std::uint64_t sampled_pairs=0,sampled_pair_q3=0,sampled_pair_q4=0,rect_any_sample=0;
  std::uint64_t corner_ms_ns=0,cheap_ms_ns=0,select_ms_ns=0,sample_ms_ns=0;
  const auto start=std::chrono::steady_clock::now();
  const auto result=run_wspd_front(*index,k,separation_s,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& r) {
    ++front;
    const auto& a=nodes[r.a_node],&b=nodes[r.b_node];
    Q34WitnessSearchWork work{};Q34WitnessBoundsWork bounds{};
    const std::uint8_t mask=filter_q34_witnesses(*index,a.box,b.box,k,r.lane_mask,work,
        Q34WitnessBoundsMode::Affine,bounds);
    if(!mask)return;
    ++open;
    const auto mass=std::uint64_t(a.range.size())*b.range.size();
    if(mass<min_product)return;
    ++selected;selected_mass+=mass;
    if(selected>10000) throw std::runtime_error("selected rectangle safety cap");
    auto t=std::chrono::steady_clock::now();
    const auto group=choose_group(*index,a.box,b.box);
    const auto moment=make_moments(*index,group);
    if(moment.n<unsigned(k-1))++groups_small;
    select_ms_ns+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-t).count();
    t=std::chrono::steady_clock::now();
    const auto order=index->spatial_order(); const auto all=index->cloud().points();
    bool any=false;
    const std::array<std::size_t,3> as{a.range.first,a.range.first+a.range.size()/2,a.range.last-1};
    const std::array<std::size_t,3> bs{b.range.first,b.range.first+b.range.size()/2,b.range.last-1};
    for(auto ai:as)for(auto bi:bs) {
      const auto pa=all[order[ai]],pb=all[order[bi]];
      const V av{pa.x,pa.y,pa.z},bv{pb.x,pb.y,pb.z};
      const auto yes=point_test(av,bv,moment,k,mask);
      ++sampled_pairs;sampled_pair_q3+=bool(yes&2);sampled_pair_q4+=bool(yes&4);
      any|=(yes==mask);
    }
    rect_any_sample+=any;
    sample_ms_ns+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-t).count();
    t=std::chrono::steady_clock::now();
    const auto fast=cheap(a.box,b.box,moment,k,mask);
    cheap_ms_ns+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-t).count();
    t=std::chrono::steady_clock::now();
    const auto exact=all_corners(a.box,b.box,moment,k,mask);
    corner_ms_ns+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-t).count();
    if((fast&~exact)!=0) throw std::runtime_error("unsound cheap bound");
    corners_3+=bool(exact&2); corners_4+=bool(exact&4); corners_both+=exact==mask;
    cheap_3+=bool(fast&2);cheap_4+=bool(fast&4);cheap_both+=fast==mask;
  },6);
  const auto wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  std::cout<<"sites "<<points.size()<<" K "<<k<<" s "<<separation_s<<" min_product "<<min_product<<" front "<<front
      <<" open "<<open<<" selected "<<selected<<" selected_pair_mass "<<selected_mass
      <<" group_below_threshold "<<groups_small<<" corner_q3 "<<corners_3<<" corner_q4 "<<corners_4
      <<" corner_all_open_lanes "<<corners_both<<" cheap_q3 "<<cheap_3<<" cheap_q4 "<<cheap_4
      <<" cheap_all_open_lanes "<<cheap_both<<" selection_ns "<<select_ms_ns
      <<" sampled_pairs "<<sampled_pairs<<" sampled_pair_q3 "<<sampled_pair_q3
      <<" sampled_pair_q4 "<<sampled_pair_q4<<" rect_any_sample "<<rect_any_sample
      <<" sample_ns "<<sample_ms_ns<<" cheap_ns "<<cheap_ms_ns<<" corner_ns "<<corner_ms_ns<<" total_wall_s "<<wall
      <<" front_rectangles "<<result.work.emitted_rectangles<<"\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
