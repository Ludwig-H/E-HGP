#include "gen/lanes/q34_witness_search.hpp"
#include "gen/pipeline/prepared_cloud.hpp"
#include "gen/spindle/predicates.hpp"
#include "gen/wspd/front.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace mhgp9::gen;
using Clock=std::chrono::steady_clock;
struct Phase {Clock::time_point wall=Clock::now(); std::clock_t cpu=std::clock();};
void show(const char* name,Phase p){std::cout<<"time "<<name<<" wall_s "<<std::chrono::duration<double>(Clock::now()-p.wall).count()<<" cpu_s "<<double(std::clock()-p.cpu)/CLOCKS_PER_SEC<<'\n';}
struct Rect {std::size_t a,b; std::uint8_t mask;};
struct Pick {std::uint32_t rank;std::uint64_t d2;};
struct Pal {std::array<std::uint32_t,20> near{};std::array<std::array<std::uint32_t,20>,8> oct{};std::array<std::uint8_t,8> oct_n{};};
struct Count {std::uint64_t rows{},mass{},full{},q3{},q4{},proposals{},skip_b{},duplicate{},queries{},corners{};};
struct ReplayState {
 std::size_t owner=std::numeric_limits<std::size_t>::max();
 std::vector<Q34WitnessNode> cache,trace;
 Q34WitnessSearchWork search{};Q34WitnessBoundsWork bounds{};Q34WitnessCacheWork cached{};
 std::uint64_t pairs{},searches{},cache_full{},covers{};
};
static std::uint8_t replay_pair(const Q2CensusIndex& index,std::size_t aid,std::size_t bid,std::uint8_t mask,unsigned k,ReplayState& st){
 const auto points=index.cloud().points();
 const auto cached=st.owner==aid?q34_cached_witness_rejections(index,points[aid],points[bid],std::uint8_t(k),mask,st.cache,st.cached):std::uint8_t{0};
 const auto open=std::uint8_t(mask&~cached);std::uint8_t result=0;
 if(open){result=filter_q34_witnesses(index,points[aid],points[bid],std::uint8_t(k),open,st.search,st.bounds,st.trace);st.owner=aid;st.cache.swap(st.trace);++st.searches;}else ++st.cache_full;
 ++st.pairs;if(result)++st.covers;return result;
}
static std::uint64_t dist2(Point3 a,Point3 b){std::uint64_t d2=0;for(unsigned j=0;j<3;++j){auto d=std::int64_t(a[j])-b[j];d2+=std::uint64_t(d*d);}return d2;}
static unsigned octant(Point3 a,Point3 b){unsigned o=0;for(unsigned j=0;j<3;++j)if(b[j]>=a[j])o|=1u<<j;return o;}
static unsigned target_oct(Point3 a,Box3 b){unsigned o=0;for(unsigned j=0;j<3;++j)if(std::int64_t(b.low[j])+b.high[j]>=2*std::int64_t(a[j]))o|=1u<<j;return o;}
static bool less_pick(Pick a,Pick b){return a.d2<b.d2||(a.d2==b.d2&&a.rank<b.rank);}
int main(int argc,char** argv){
 if(argc!=4&&argc!=5)throw std::invalid_argument("usage: shadow file.u32le K s [rows]");
 unsigned k=std::stoul(argv[2]),s=std::stoul(argv[3]);if((k!=5&&k!=10)||(s!=8&&s!=10&&s!=12))throw std::invalid_argument("bad K/s");
 std::ifstream input(argv[1],std::ios::binary);if(!input)throw std::runtime_error("input");
 const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)),{});if(bytes.size()%12||bytes.empty())throw std::runtime_error("u32le");
 std::vector<Point3> points(bytes.size()/12);for(std::size_t i=0;i<points.size();++i){std::uint32_t xyz[3]{};for(unsigned j=0;j<3;++j)for(unsigned h=0;h<4;++h)xyz[j]|=std::uint32_t(bytes[12*i+4*j+h])<<(8*h);points[i]={Coordinate(xyz[0]),Coordinate(xyz[1]),Coordinate(xyz[2])};}
 Phase p;auto index=make_q2_cloud_index(prepare_cloud(points));show("index",p);
 const auto nodes=index->spatial_nodes();const auto order=index->spatial_order();if(order.size()!=points.size()||points.size()>UINT32_MAX)throw std::runtime_error("rank size");
 std::vector<Pal> palettes(points.size());
 p=Phase{};std::uint64_t distances=0;for(std::size_t r=0;r<order.size();++r){
   const auto first=r>4*k?r-4*k:0,last=std::min(order.size(),r+4*k+1);
   std::vector<Pick> all;std::array<std::vector<Pick>,8> bins;
   all.reserve(last-first);for(std::size_t q=first;q<last;++q){if(q==r)continue;Pick x{std::uint32_t(q),dist2(points[order[r]],points[order[q]])};++distances;all.push_back(x);bins[octant(points[order[r]],points[order[q]])].push_back(x);}
   auto& pal=palettes[r];std::size_t take=std::min<std::size_t>(2*k,all.size());std::partial_sort(all.begin(),all.begin()+take,all.end(),less_pick);for(std::size_t j=0;j<take;++j)pal.near[j]=all[j].rank;
   for(unsigned d=0;d<8;++d){auto& xs=bins[d];take=std::min<std::size_t>(2*k,xs.size());std::partial_sort(xs.begin(),xs.begin()+take,xs.end(),less_pick);pal.oct_n[d]=std::uint8_t(take);for(std::size_t j=0;j<take;++j)pal.oct[d][j]=xs[j].rank;}
 }show("prepare_both",p);
 std::vector<Rect> rects;p=Phase{};auto front=run_wspd_front(*index,k,s,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& r){rects.push_back({r.a_node,r.b_node,r.lane_mask});},6);show("front",p);(void)front;
 p=Phase{};Q34WitnessSearchWork rw{};Q34WitnessBoundsWork rb{};std::size_t out=0;std::uint64_t residual_mass=0;for(auto r:rects){r.mask=filter_q34_witnesses(*index,nodes[r.a].box,nodes[r.b].box,std::uint8_t(k),r.mask,rw,Q34WitnessBoundsMode::Affine,rb);if(!r.mask)continue;rects[out++]=r;residual_mass+=std::uint64_t(nodes[r.a].range.size())*nodes[r.b].range.size();}rects.resize(out);show("rectangle_filter",p);
 std::cout<<"meta sites "<<points.size()<<" k "<<k<<" s "<<s<<" candidate_distances "<<distances<<" rectangles "<<rects.size()<<" residual_mass "<<residual_mass<<"\n";
 auto run=[&](unsigned mode,const char* name,std::vector<std::uint8_t>* masks){Count c{};PredicateWork w{};Phase start;for(const auto& r:rects){const auto ar=nodes[r.a].range,br=nodes[r.b].range;const auto B=nodes[r.b].box;if(br.size()<8){if(masks)for(std::size_t ai=ar.first;ai<ar.last;++ai)masks->push_back(0);continue;}for(std::size_t ai=ar.first;ai<ar.last;++ai){const auto a=points[order[ai]];const auto& pal=palettes[ai];const unsigned bin=target_oct(a,B);unsigned n3=0,n4=0;std::uint8_t closed=0;auto check=[&](std::uint32_t zr){++c.proposals;if(zr>=br.first&&zr<br.last){++c.skip_b;return;}const auto z=points[order[zr]];bool q4ok=false;if((r.mask&4u)&&!(closed&4u)){q4ok=universal_witness(Lane::Q4,a,B,z,w);if(q4ok&&++n4>=k-2)closed|=4u;}if((r.mask&2u)&&!(closed&2u)){if(q4ok||universal_witness(Lane::Q3,a,B,z,w)){if(++n3>=k-1)closed|=2u;}}};
 if(mode==0||mode==2){for(unsigned j=0;j<2*k&&closed!=r.mask;++j)check(pal.near[j]);}
 if(mode==1||mode==2){for(unsigned j=0;j<pal.oct_n[bin]&&closed!=r.mask;++j){const auto zr=pal.oct[bin][j];if(mode==2&&std::find(pal.near.begin(),pal.near.begin()+2*k,zr)!=pal.near.begin()+2*k){++c.duplicate;continue;}check(zr);}}
 if(masks){masks->push_back(closed);}
 ++c.rows;c.mass+=br.size();if(closed==r.mask)c.full+=br.size();if(closed&2u)c.q3+=br.size();if(closed&4u)c.q4+=br.size();}}
 show(name,start);c.queries=w.universal_queries;c.corners=w.corner_tests;std::cout<<"count "<<name<<" rows "<<c.rows<<" mass "<<c.mass<<" full "<<c.full<<" q3 "<<c.q3<<" q4 "<<c.q4<<" proposals "<<c.proposals<<" skip_b "<<c.skip_b<<" duplicate "<<c.duplicate<<" queries "<<c.queries<<" corners "<<c.corners<<'\n';return c;};
 std::vector<std::uint8_t> direction_masks,union_masks;
 run(0,"near_only",nullptr);run(1,"direction_only",&direction_masks);run(2,"near_then_direction",&union_masks);
 if(argc==5){if(std::string(argv[4])!="rows")throw std::invalid_argument("mode");return 0;}
 std::vector<std::uint8_t> baseline_masks;baseline_masks.reserve(residual_mass);std::vector<std::uint8_t> baseline_row_or;baseline_row_or.reserve(direction_masks.size());
 ReplayState baseline{};p=Phase{};for(const auto& r:rects){const auto ar=nodes[r.a].range,br=nodes[r.b].range;for(std::size_t ai=ar.first;ai<ar.last;++ai){std::uint8_t row_or=0;for(std::size_t bi=br.first;bi<br.last;++bi){auto result=replay_pair(*index,order[ai],order[bi],r.mask,k,baseline);baseline_masks.push_back(result);row_or|=result;}baseline_row_or.push_back(row_or);}}show("baseline_replay",p);
 if(baseline_masks.size()!=residual_mass||baseline_row_or.size()!=direction_masks.size()||direction_masks.size()!=union_masks.size())throw std::logic_error("replay cardinal mismatch");
 auto replay=[&](const char* name,const std::vector<std::uint8_t>& masks){ReplayState st{};std::size_t row=0,pair=0;std::uint64_t skipped_rows=0,skipped_pairs=0;Phase start;for(const auto& r:rects){const auto ar=nodes[r.a].range,br=nodes[r.b].range;for(std::size_t ai=ar.first;ai<ar.last;++ai){const auto open=std::uint8_t(r.mask&~masks[row]);if(!open){if(baseline_row_or[row]!=0)throw std::logic_error("skip lost survivor");pair+=br.size();skipped_pairs+=br.size();++skipped_rows;}else for(std::size_t bi=br.first;bi<br.last;++bi){const auto result=replay_pair(*index,order[ai],order[bi],open,k,st);if(result!=baseline_masks[pair])throw std::logic_error("changed pair mask");++pair;}++row;}}show(name,start);if(row!=masks.size()||pair!=baseline_masks.size())throw std::logic_error("replay incomplete");std::cout<<"replay "<<name<<" skipped_rows "<<skipped_rows<<" skipped_pairs "<<skipped_pairs<<" pairs "<<st.pairs<<" searches "<<st.searches<<" cache_full "<<st.cache_full<<" cache_tests "<<st.cached.node_tests<<" pair_visits "<<st.search.node_visits<<" covers "<<st.covers<<"\n";};
 std::cout<<"replay baseline skipped_rows 0 skipped_pairs 0 pairs "<<baseline.pairs<<" searches "<<baseline.searches<<" cache_full "<<baseline.cache_full<<" cache_tests "<<baseline.cached.node_tests<<" pair_visits "<<baseline.search.node_visits<<" covers "<<baseline.covers<<"\n";
 replay("direction",direction_masks);replay("union",union_masks);
}
