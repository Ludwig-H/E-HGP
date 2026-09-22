// Audit-only caller of the unchanged native v8 library. No production patch.
#include "pipeline/q2_census.hpp"
#include "lanes/q34_witness_search.hpp"
#include "spindle/predicates.hpp"
#include "wspd/front.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <compare>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <tuple>
#include <vector>
using namespace mhgp8;
using Mask=std::uint8_t;
struct Pair {std::size_t a,b; Mask mask; auto operator<=>(const Pair&) const=default;};
struct Ops {u64 rectangles{}, pairs{}, plans{}, factor_sites{}, local_tests{}, singleton_reuses{};};
struct Run {std::vector<Pair> out; Ops ops;};
static double cpu(){timespec t{}; if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t)) throw std::runtime_error("clock"); return 1000.0*t.tv_sec+t.tv_nsec/1e6;}
static u64 mass(const Q2CensusIndex& idx,const WspdRectangle& r){return idx.spatial_nodes()[r.a_node].range.size()*u64(idx.spatial_nodes()[r.b_node].range.size());}
static Mask native(const Q2CensusIndex& idx,Box3 a,Box3 b,unsigned k,Mask mask,Q34WitnessSearchWork& w){Q34WitnessBoundsWork bw{};return filter_q34_witnesses(idx,a,b,static_cast<Mask>(k),mask,w,Q34WitnessBoundsMode::Affine,bw);}
static void pair_call(const Q2CensusIndex& idx,std::size_t a,std::size_t b,unsigned k,Mask mask,Run& r){
  Q34WitnessSearchWork w{}; ++r.ops.pairs;
  mask=native(idx,singleton_box(idx.cloud().points()[a]),singleton_box(idx.cloud().points()[b]),k,mask,w);
  if(mask) r.out.push_back({std::min(a,b),std::max(a,b),mask});
}
using Credit=std::array<unsigned,2>;
using Groups=std::map<Credit,std::vector<std::size_t>>;
static Groups groups(const Q2CensusIndex& idx,std::size_t own,std::size_t other,unsigned k,Mask mask,Credit h,Ops& ops){
  auto nodes=idx.spatial_nodes();auto order=idx.spatial_order();auto p=idx.cloud().points();auto range=nodes[own].range;
  std::array<i64,3> d{};for(unsigned j=0;j<3;++j)d[j]=i64(nodes[other].box.low[j])+nodes[other].box.high[j]-nodes[own].box.low[j]-nodes[own].box.high[j];
  std::vector<std::pair<i64,std::size_t>> pool;pool.reserve(k+2);
  for(auto r=range.first;r<range.last;++r){auto id=order[r];i64 score=0;for(unsigned j=0;j<3;++j)score+=d[j]*p[id][j];pool.emplace_back(-score,id);std::sort(pool.begin(),pool.end());if(pool.size()>k+1)pool.pop_back();}
  Groups out;ops.factor_sites+=range.size();
  for(auto r=range.first;r<range.last;++r){auto id=order[r];Credit c{};
    for(auto [score,z]:pool){(void)score;if(z==id)continue;
      for(unsigned j=0;j<2;++j){unsigned t=k-1-j;Mask bit=static_cast<Mask>(2U<<j);if(!(mask&bit)||h[j]+c[j]>=t)continue;
        PredicateWork w{};++ops.local_tests;if(universal_witness(j==0?Lane::Q3:Lane::Q4,p[id],nodes[other].box,p[z],w))++c[j];}}
    out[c].push_back(id);
  }return out;
}
static Run run(const Q2CensusIndex& idx,const std::vector<WspdRectangle>& rect,unsigned k,unsigned mode){
  Run out;auto nodes=idx.spatial_nodes();auto order=idx.spatial_order();
  for(const auto& r:rect){Q34WitnessSearchWork w{};++out.ops.rectangles;
    Mask mask=native(idx,nodes[r.a_node].box,nodes[r.b_node].box,k,r.lane_mask,w);if(!mask)continue;
    auto ar=nodes[r.a_node].range,br=nodes[r.b_node].range;
    if(mode!=0 && ar.size()==1 && br.size()==1){auto a=order[ar.first],b=order[br.first];out.out.push_back({std::min(a,b),std::max(a,b),mask});++out.ops.singleton_reuses;continue;}
    auto n=mass(idx,r);bool plan=mode>=2 && (mode==3 || (n>=256 && n>=4*(ar.size()+br.size())));
    if(!plan){for(auto a=ar.first;a<ar.last;++a)for(auto b=br.first;b<br.last;++b)pair_call(idx,order[a],order[b],k,mask,out);continue;}
    ++out.ops.plans;Credit h{static_cast<unsigned>(w.q3_credits),static_cast<unsigned>(w.q4_credits)};
    auto ag=groups(idx,r.a_node,r.b_node,k,mask,h,out.ops),bg=groups(idx,r.b_node,r.a_node,k,mask,h,out.ops);
    for(const auto& [ac,ai]:ag)for(const auto& [bc,bi]:bg){Mask m=mask;for(unsigned j=0;j<2;++j)if(h[j]+ac[j]+bc[j]>=k-1-j)m&=static_cast<Mask>(~(2U<<j));if(!m)continue;for(auto a:ai)for(auto b:bi)pair_call(idx,a,b,k,m,out);}
  }return out;
}
static std::vector<Pair> oracle(const std::vector<Point3>& p,unsigned k){
  std::vector<Pair> out;
  for(std::size_t a=0;a<p.size();++a)for(std::size_t b=a+1;b<p.size();++b){Credit count{};
    for(auto z:p){i64 h=0,uu=0,vv=0;for(unsigned i=0;i<3;++i){i64 u=i64(z[i])-p[a][i],v=i64(p[b][i])-z[i];h+=u*v;uu+=u*u;vv+=v*v;}
      if(h<=0)continue;for(unsigned j=0;j<2;++j)if(i128(4-j)*h*h>i128(uu)*vv)++count[j];}
    Mask m=0;for(unsigned j=0;j<2;++j)if(count[j]<k-1-j)m|=static_cast<Mask>(2U<<j);if(m)out.push_back({a,b,m});
  }return out;
}
static void selftest(){
  std::mt19937_64 rng(2026092218);u64 comparisons=0;
  for(unsigned t=0;t<20;++t){std::vector<Point3> p;
    if(t==0){for(auto x:{0,262143})for(auto y:{0,262143})for(auto z:{0,262143})p.push_back({x,y,z});}
    else {while(p.size()<22){Point3 q{Coordinate(rng()%(t%2?262144:64)),Coordinate(rng()%(t%2?262144:64)),Coordinate(t%4==0?0:rng()%(t%2?262144:64))};if(std::find(p.begin(),p.end(),q)==p.end())p.push_back(q);}}
    auto idx=make_q2_cloud_index(prepare_cloud(p));
    for(unsigned k:{5U,10U})for(unsigned s:{2U,8U}){
      std::vector<WspdRectangle> rect;auto fr=run_wspd_front(*idx,k,s,WspdFrontMode::MidpointSamples,[&](auto r){rect.push_back(r);},6);(void)fr;
      auto expected=oracle(p,k);for(unsigned mode=0;mode<4;++mode){auto r=run(*idx,rect,k,mode);std::sort(r.out.begin(),r.out.end());if(r.out!=expected)throw std::runtime_error("selftest pair/lane mismatch");comparisons+=p.size()*(p.size()-1)/2;}
    }
  }std::cout<<"{\"selftest\":\"pass\",\"pair_lane_comparisons\":"<<comparisons<<"}\n";
}
static std::vector<Point3> read_cloud(const char* name){
  std::ifstream f(name,std::ios::binary);if(!f)throw std::runtime_error("input absent");std::vector<Point3> p;
  std::array<unsigned char,12> b{};while(f.read(reinterpret_cast<char*>(b.data()),12)){std::array<Coordinate,3> c{};for(unsigned i=0;i<3;++i){std::uint32_t u=0;for(unsigned j=0;j<4;++j)u|=std::uint32_t(b[4*i+j])<<(8*j);if(u>262143)throw std::runtime_error("range");c[i]=static_cast<Coordinate>(u);}p.push_back({c[0],c[1],c[2]});}if(f.gcount()!=0||f.bad())throw std::runtime_error("truncated input");return p;
}
int main(int argc,char**argv){try{
  if(argc==2&&std::string(argv[1])=="--selftest"){selftest();return 0;}
  if(argc!=5)throw std::runtime_error("usage: probe file.u32le K samples-per-mass-class repetitions");
  auto p=read_cloud(argv[1]);unsigned k=std::stoul(argv[2]),n=std::stoul(argv[3]),repeats=std::stoul(argv[4]);if((k!=5&&k!=10)||n==0||repeats==0)throw std::runtime_error("arguments");
  double t=cpu();auto idx=make_q2_cloud_index(prepare_cloud(p));double prep=cpu()-t;
  std::array<std::vector<WspdRectangle>,4> samples;std::array<u64,5> counts{},masses{};std::mt19937_64 rng(20260922+k);t=cpu();
  auto front=run_wspd_front(*idx,k,8,WspdFrontMode::MidpointSamples,[&](const WspdRectangle&r){auto m=mass(*idx,r);unsigned c=m==1?0:m<64?1:m<1024?2:m<=65536?3:4;++counts[c];masses[c]+=m;if(c==4)return;auto&v=samples[c];if(v.size()<n)v.push_back(r);else{auto j=std::uniform_int_distribution<u64>(0,counts[c]-1)(rng);if(j<n)v[j]=r;}},6);
  double ft=cpu()-t;std::cout<<std::setprecision(12)<<"{\"scope\":\"native_front_on_full_cloud_sampled_rectangles_filter_only\",\"n\":"<<p.size()<<",\"K\":"<<k<<",\"preparation_cpu_ms\":"<<prep<<",\"front_cpu_ms\":"<<ft<<",\"front_products\":"<<front.work.product_visits<<",\"classes\":[";
  for(unsigned c=0;c<5;++c){if(c)std::cout<<',';std::cout<<"{\"class\":"<<c<<",\"rectangles\":"<<counts[c]<<",\"pair_mass\":"<<masses[c]<<'}';}std::cout<<"],\"measures\":[";bool first=true;
  for(unsigned c=0;c<4;++c){auto&rect=samples[c];if(rect.empty())continue;std::vector<Pair>expected;u64 sampled_mass=0;for(auto r:rect)sampled_mass+=mass(*idx,r);
    for(unsigned rep=0;rep<repeats;++rep)for(unsigned step=0;step<4;++step){unsigned mode=(step+rep)%4;double start=cpu();auto r=run(*idx,rect,k,mode);double elapsed=cpu()-start;std::sort(r.out.begin(),r.out.end());if(rep==0&&step==0)expected=r.out;else if(r.out!=expected)throw std::runtime_error("LiDAR sampled pair/lane mismatch");
      if(!first)std::cout<<',';first=false;std::cout<<"{\"class\":"<<c<<",\"rep\":"<<rep<<",\"mode\":"<<mode<<",\"sampled_rectangles\":"<<rect.size()<<",\"sampled_pair_mass\":"<<sampled_mass<<",\"cpu_ms\":"<<elapsed<<",\"surviving_pairs\":"<<r.out.size()<<",\"pair_searches\":"<<r.ops.pairs<<",\"plans\":"<<r.ops.plans<<",\"factor_sites\":"<<r.ops.factor_sites<<",\"local_tests\":"<<r.ops.local_tests<<",\"singleton_reuses\":"<<r.ops.singleton_reuses<<'}';}
  }std::cout<<"],\"pair_lane_comparisons\":\"pass\"}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 2;}}
