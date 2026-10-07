#include <algorithm>
#include <array>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "mhgp12/leaf/dump_format.hpp"
#include "mhgp12/leaf/leaf_j3.hpp"
#include "mhgp12/leaf/leaf_coherent.hpp"
#include "catalogue/leaf_device.hpp"

namespace l = mhgp12::leaf;
namespace d = mhgp12::dump;
namespace r = mhgp11::leaf_device;
using Item = std::array<unsigned, 9>;
void require(bool ok, const char* what) { if (!ok) throw std::runtime_error(what); }
struct Sink {
  std::vector<Item> items;
  void emit(const l::Emission& e) {
    items.push_back({e.support[0],e.support[1],e.support[2],e.support[3],e.p,e.m,e.qmin,e.interior,e.shell});
  }
  void emit(const r::Ball& e, const unsigned*, const unsigned*, const unsigned* in, const unsigned* on) {
    Item item{e.support[0],e.support[1],e.support[2],e.support[3],e.p,e.m,e.qmin,0,0};
    for (unsigned j=e.qmin;j<4;++j) item[j]=255;
    for (unsigned j=0;j<e.p;++j) item[7] |= 1u<<in[j];
    for (unsigned j=0;j<e.m;++j) item[8] |= 1u<<on[j];
    items.push_back(item);
  }
};
std::array<unsigned long long,15> counters(const r::Counts& c) {
  return {c.dominance_tests,c.prefixes,c.judged,c.census_tests,c.emitted,c.incidences,c.q4_candidates,c.q4_levels,
    c.region_pair_tests,c.region_pair_rejects,c.region_line_tests,c.region_line_rejects,c.region_line_evaluations,
    c.region_line_cache_hits,c.region_line_fallbacks};
}
void shape(const char* name, const std::vector<std::array<unsigned,3>>& points, long long lo, long long hi) {
  std::vector<unsigned> x,y,z,sites;
  for (const auto& p:points) {x.push_back(p[0]);y.push_back(p[1]);z.push_back(p[2]);sites.push_back(sites.size());}
  l::Input in;in.x=x.data();in.y=y.data();in.z=z.data();in.sites=sites.data();in.m=sites.size();in.kmax=5;in.cache=true;
  r::Input ref;ref.x=x.data();ref.y=y.data();ref.z=z.data();ref.sites=sites.data();ref.m=sites.size();ref.kmax=5;ref.cache=true;
  for(int j=0;j<3;++j) {in.lo[j]=ref.lo[j]=lo;in.hi[j]=ref.hi[j]=hi;}
  r::Counts rc;Sink rs;const auto status=r::run_leaf(ref,rc,rs);std::sort(rs.items.begin(),rs.items.end());
  for (int form=0;form<2;++form) {
    l::j3::SharedJ3 shared{};l::Counts c{};Sink sink;l::Diag diag;
    const auto got=form==0?l::j3::run_leaf(in,shared,c,sink,&diag):l::coherent::run_leaf(in,shared,c,sink,&diag);
    require(got==status,"status mismatch");std::sort(sink.items.begin(),sink.items.end());
    if(status==0) {require(sink.items==rs.items,"emissions mismatch");auto cr=counters(rc);for(unsigned j=0;j<15;++j)require(c.c[j]==cr[j],"count mismatch");}
    std::cout<<"{\"shape\":\""<<name<<"\",\"form\":"<<form<<",\"status\":"<<got<<",\"records\":"<<sink.items.size()
      <<",\"chunks\":"<<diag.chunks<<",\"triples\":"<<diag.items[3]<<",\"quadruples\":"<<diag.items[4]<<"}\n";
    if(std::string(name)=="sphere32" && form==0) require(diag.items[3]>512 && diag.items[4]>512,"queue not exercised");
  }
}
d::LeafDump base() {
  d::LeafDump out;auto& h=out.header;h.coord_bits=21;h.kmax=1;h.leaf_size=1;h.max_leaf=256;h.flags=3;
  h.n_sites=h.n_leaves=h.n_leaf_sites=1;h.walk_leaves=1;
  out.x={1};out.y={1};out.z={1};out.sites={0};out.jobs.resize(1);out.jobs[0].m=1;
  for(int j=0;j<3;++j)out.jobs[0].hi[j]=2;
  out.counts.resize(15);out.counts[1]=1;out.status={1};out.record_begin={0,0};out.population_begin={0,0};return out;
}
int main(int argc,char** argv) {
  require(argc==2,"temporary directory needed");const std::string dir=argv[1];
  std::vector<std::array<unsigned,3>> sphere;
  for(int x=-7;x<=7 && sphere.size()<32;++x)for(int y=-7;y<=7 && sphere.size()<32;++y)for(int z=-7;z<=7 && sphere.size()<32;++z)
    if(x*x+y*y+z*z==50 && (x>0 || (x==0 && (y>0 || (y==0 && z>0))))) {
      sphere.push_back({unsigned(x+8),unsigned(y+8),unsigned(z+8)});sphere.push_back({unsigned(8-x),unsigned(8-y),unsigned(8-z)});
    }
  std::sort(sphere.begin(),sphere.end());require(sphere.size()==32,"sphere cardinal");
  shape("sphere32",sphere,0,17);
  constexpr unsigned m=1u<<20;
  shape("span_exact",{{m,m,m},{2*m-1,2*m-1,2*m-1}},m,2*m);
  shape("span_plus_one",{{m,m,m},{2*m-1,2*m-1,2*m-1}},m-1,2*m);
  for(int mutation=0;mutation<7;++mutation) {
    auto out=base();const char* name="baseline";
    if(mutation==1){name="site_out_of_cloud";out.sites[0]=1;}
    if(mutation==2){name="wrapped_job_begin";out.jobs[0].begin=~d::u64{0};}
    if(mutation==3){name="zero_k";out.header.kmax=0;}
    if(mutation==4){name="profile_mismatch";out.header.coord_bits=24;}
    if(mutation==5){name="coordinate_out_of_profile";out.x[0]=UINT_MAX;out.jobs[0].lo[0]=UINT_MAX;out.jobs[0].hi[0]=d::i64(UINT_MAX)+1;}
    if(mutation==6){name="short_record_population";out.header.n_records=1;out.header.n_population=1;out.records={{{0,0,255,255},0,2,2,0}};
      out.record_begin={0,1};out.population_begin={0,1};out.population={0};}
    std::string error;const auto path=dir+"/"+name+".bin";require(d::write(path,out,error),"writer failed");
    d::LeafDump got;const bool accepted=d::read(path,got,error);require(accepted,"reader changed; re-audit expected");
    std::cout<<"{\"dump_case\":\""<<name<<"\",\"reader_accepted\":"<<(accepted?"true":"false")<<"}\n";
  }
}
