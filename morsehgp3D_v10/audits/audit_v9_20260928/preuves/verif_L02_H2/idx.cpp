#include "gen/pipeline/q2_census.hpp"
#include "gen/pipeline/prepared_cloud.hpp"
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>
#include <cstdlib>
using namespace mhgp9::gen;
int main(int argc,char**argv){
  std::size_t n = argc>1? std::strtoull(argv[1],0,10):40000;
  std::mt19937_64 rng(3);
  std::uniform_int_distribution<int> u(0,(1<<18)-1);
  std::vector<Point3> pts; pts.reserve(n);
  for(std::size_t i=0;i<n;++i) pts.push_back(Point3{u(rng),u(rng),u(rng)});
  auto cloud = prepare_cloud(pts);
  double best=1e9; std::size_t bytes=0, nodes=0;
  for(int r=0;r<5;++r){
    auto t=std::chrono::steady_clock::now();
    auto ix = make_q2_cloud_index(cloud);
    double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();
    if(ms<best) best=ms; bytes=ix->retained_bytes(); nodes=ix->spatial_nodes().size();
  }
  std::printf("n=%zu best_ms=%.3f nodes=%zu retained_bytes=%zu min_bytes=%zu\n",n,best,nodes,bytes,(std::size_t)(nodes*64+n*8+n*12));
}
