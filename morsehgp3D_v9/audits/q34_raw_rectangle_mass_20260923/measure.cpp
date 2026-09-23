#include "gen/lanes/q34_witness_search.hpp"
#include "gen/pipeline/prepared_cloud.hpp"
#include "gen/wspd/front.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
using namespace mhgp9::gen;
struct B { uint64_t rectangles=0,mass=0,q3=0,q4=0; };
int main(int argc,char**argv){
 if(argc!=3) return 2; unsigned K=std::stoul(argv[2]);
 std::ifstream in(argv[1],std::ios::binary); if(!in) throw std::runtime_error("open");
 const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),{});
 if(bytes.empty()||bytes.size()%12) throw std::runtime_error("length");
 std::vector<Point3> points(bytes.size()/12);
 for(size_t i=0;i<points.size();++i){uint32_t v[3]{};for(unsigned a=0;a<3;++a)for(unsigned b=0;b<4;++b)v[a]|=uint32_t(bytes[12*i+4*a+b])<<(8*b);points[i]={Coordinate(v[0]),Coordinate(v[1]),Coordinate(v[2])};}
 auto t0=std::chrono::steady_clock::now();
 auto index=make_q2_cloud_index(prepare_cloud(points)); auto nodes=index->spatial_nodes();
 std::array<B,32> front{},open{}; uint64_t allr=0,allm=0,openr=0,openm=0,closedm=0,closedr=0,visits=0,maxm=0;
 const auto result=run_wspd_front(*index,K,8,WspdFrontMode::MidpointSamples,[&](const WspdRectangle&r){
  uint64_t mass=uint64_t(nodes[r.a_node].range.size())*nodes[r.b_node].range.size();
  unsigned bucket=0;for(uint64_t x=mass;x>1;x>>=1)++bucket;
  ++allr;allm+=mass;front[bucket].rectangles++;front[bucket].mass+=mass;maxm=std::max(maxm,mass);
  Q34WitnessSearchWork w{};Q34WitnessBoundsWork bw{};
  auto mask=filter_q34_witnesses(*index,nodes[r.a_node].box,nodes[r.b_node].box,K,r.lane_mask,w,Q34WitnessBoundsMode::Affine,bw);visits+=w.node_visits;
  if(mask){++openr;openm+=mass;open[bucket].rectangles++;open[bucket].mass+=mass;if(mask&2){open[bucket].q3++;}if(mask&4){open[bucket].q4++;}}
  else{++closedr;closedm+=mass;}
 },6);
 auto dt=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
 std::cout<<"sites "<<points.size()<<" K "<<K<<" front_rect "<<allr<<" front_mass "<<allm<<" open_rect "<<openr<<" open_mass "<<openm<<" closed_rect "<<closedr<<" closed_mass "<<closedm<<" rect_visits "<<visits<<" max_rect_mass "<<maxm<<" time_s "<<dt<<" front_reported "<<result.work.emitted_rectangles<<"\n";
 for(unsigned i=0;i<32;++i)if(front[i].rectangles)std::cout<<"bucket 2^"<<i<<" front_rect "<<front[i].rectangles<<" front_mass "<<front[i].mass<<" open_rect "<<open[i].rectangles<<" open_mass "<<open[i].mass<<" open_q3 "<<open[i].q3<<" open_q4 "<<open[i].q4<<"\n";
}
