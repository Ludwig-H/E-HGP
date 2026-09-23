// Scratch harness (never committed): q34 generator alone, atlas options varied.
#include <sys/resource.h>
#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>
#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/wspd_q34.hpp"
using namespace mhgp9::gen;
static double cpu_s() { rusage u{}; getrusage(RUSAGE_SELF,&u); return u.ru_utime.tv_sec+u.ru_stime.tv_sec+1e-6*(u.ru_utime.tv_usec+u.ru_stime.tv_usec); }
int main(int argc,char**argv){
  if(argc<4){std::fprintf(stderr,"usage: file n K W [leaf=32] [depth=7] [z=512] [sat=1] [leafc=1]\n");return 2;}
  std::ifstream in(argv[1],std::ios::binary);
  std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
  std::size_t n=std::strtoull(argv[2],nullptr,10); unsigned K=std::atoi(argv[3]); std::size_t W=std::strtoull(argv[4],nullptr,10);
  std::size_t total=bytes.size()/12; if(n==0||n>total) n=total;
  std::vector<Point3> pts(n);
  for(std::size_t i=0;i<n;++i){ std::uint32_t c[3]; std::memcpy(c,&bytes[12*i],12); pts[i]={(Coordinate)c[0],(Coordinate)c[1],(Coordinate)c[2]}; }
  WspdQ34Options o;
  o.front_mode=WspdFrontMode::MidpointSamples; o.requested_lane_mask=6; o.q4_backend=WspdQ4Backend::Local28;
  o.local=Q4LocalOptions{};
  if(argc>5) o.local.leaf_sites=std::strtoull(argv[5],nullptr,10);
  if(argc>6) o.local.max_depth=std::atoi(argv[6]);
  if(argc>7) o.local.z_test_budget=std::strtoull(argv[7],nullptr,10);
  o.local.saturate_deep= argc>8 ? std::atoi(argv[8])!=0 : true;
  const bool leafc = argc>9 ? std::atoi(argv[9])!=0 : true;
  o.local.retain_q3_fragments=leafc;
  o.witness_mode=WspdQ34WitnessMode::RectanglePair; o.q3_census_mode=WspdQ3CensusMode::GlobalBoxes;
  o.witness_bounds_mode=Q34WitnessBoundsMode::Affine;
  o.q4_seed_cells=Q4SeedCellOptions{Q4SeedCellMode::LiveOnly,64};
  o.q3_atlas_consultation=true; o.q3_leaf_census=leafc;
  auto index=make_q2_cloud_index(prepare_cloud(pts));
  std::vector<std::vector<std::tuple<unsigned,std::size_t,std::size_t,std::size_t,std::size_t,std::size_t,std::size_t>>> slots(W);
  const double c0=cpu_s(); const auto t0=std::chrono::steady_clock::now();
  const auto r=run_wspd_q34_parallel(index,K,8,o,W,[&](std::size_t s,const Q34SeedCandidate& c){
    slots[s].emplace_back(c.arity,c.support_ids[0],c.support_ids[1],c.support_ids[2],c.arity>3?c.support_ids[3]:0,c.depth,c.shell_first.size()+c.shell_second.size());},16);
  const double wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count(); const double cpu=cpu_s()-c0;
  std::vector<std::tuple<unsigned,std::size_t,std::size_t,std::size_t,std::size_t,std::size_t,std::size_t>> all;
  for(auto&s:slots) all.insert(all.end(),s.begin(),s.end());
  std::sort(all.begin(),all.end());
  std::uint64_t h=1469598103934665603ull; auto mix=[&](std::uint64_t v){for(int i=0;i<8;++i){h^=(v>>(8*i))&255;h*=1099511628211ull;}};
  for(auto&t:all){mix(std::get<0>(t));mix(std::get<1>(t));mix(std::get<2>(t));mix(std::get<3>(t));mix(std::get<4>(t));mix(std::get<5>(t));mix(std::get<6>(t));}
  const auto& w=r.pipeline.work; const auto& a=w.local.atlas;
  std::printf("n=%zu K=%u W=%zu leaf=%zu depth=%u z=%llu sat=%d leafc=%d wall=%.3f cpu=%.2f out=%zu digest=%016llx\n",n,K,W,o.local.leaf_sites,o.local.max_depth,(unsigned long long)o.local.z_test_budget,(int)o.local.saturate_deep,(int)leafc,wall,cpu,all.size(),(unsigned long long)h);
  std::printf("  covers=%" PRIu64 " cover_sites=%" PRIu64 " cells=%" PRIu64 " deep=%" PRIu64 " outside=%" PRIu64 " leaf=%" PRIu64 " splits=%" PRIu64 "\n",w.cover_builds,w.cover_sites,a.cells_created,a.deep_cells,a.outside_cells,a.leaf_cells,a.splits);
  std::printf("  atlas node_visits=%" PRIu64 " block_bounds=%" PRIu64 " point_tests=%" PRIu64 " ids_copied=%" PRIu64 " active_sites_sum=%" PRIu64 "\n",a.partition.node_visits,a.partition.block_bound_tests,a.partition.point_tests,a.partition.frontier_ids_copied,a.active_sites_sum);
  std::printf("  q3 seeds=%" PRIu64 " census_bounds=%" PRIu64 " leaf_tests=%" PRIu64 " q4 seeds=%" PRIu64 " sweep_events=%" PRIu64 " q3_emitted=%" PRIu64 " q4_emitted=%" PRIu64 "\n",w.q3.seeds,w.q3_blocks.count_bounds_prepared,w.q3_atlas.leaf_point_tests,w.local.seeds,w.local.sweep.kept_events,w.q3.emitted,w.q4_emitted);
  return 0;
}
