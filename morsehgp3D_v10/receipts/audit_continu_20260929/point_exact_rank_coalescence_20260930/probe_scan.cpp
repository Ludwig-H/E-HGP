#include <cfenv>
#include <cstdio>
#include <vector>
#include "tower/tower.hpp"
using namespace mhgp10;
int main() {
  if (std::fesetround(FE_TONEAREST) || std::fegetround() != FE_TONEAREST) return 2;
  geom::P3 a{0,0,0}, b{}, z{};
  bool chosen=false;
  for (int t=1;t<=100 && !chosen;++t) for(int m=511;m<=600 && !chosen;++m) {
    const int L=m*m-t*m; if(L<=0 || L>=262144) continue;
    geom::P3 bb{L,t,0}, zz{1,m,0};
    if(geom::acute(a,bb,zz) && geom::compare(geom::level2(a,bb),geom::level3(a,bb,zz))<0 && geom::level2(a,bb).approx()==geom::level3(a,bb,zz).approx()) { b=bb;z=zz;chosen=true; }
  }
  if(!chosen) return 9;
  std::printf("chosen b=%lld,%lld z=%lld,%lld\n",(long long)b.x,(long long)b.y,(long long)z.x,(long long)z.y);
  const geom::Level ab = geom::level2(a,b), abc = geom::level3(a,b,z);
  const bool positive = geom::acute(a,b,z);
  const int cmp = geom::compare(ab,abc);
  const double da = ab.approx(), db = abc.approx();
  std::printf("primitive acute=%d exact_compare=%d AB=%a ABC=%a same_double=%d\n", positive,cmp,da,db,da==db);
  if (!positive || cmp != -1 || da != db) return 1;
  std::vector<u32> x{0,u32(b.x),u32(z.x)}, y{0,u32(b.y),u32(z.y)}, zz{0,0,0}, ids{0,1,2};
  auto prep = prepare_cloud(x,y,zz,ids,kCoordinateBits);
  if (!prep.ok()) return 3;
  const Cloud& cloud = prep.value();
  sched::Pool pool(1);
  SiteTree tree(cloud);
  CatalogueParams cp; cp.kmax=3;
  auto cat=build_catalogue(cloud,cp,pool);
  if (!cat.ok()) return 4;
  unsigned found=0;
  for (PointEntry entry : {PointEntry::core,PointEntry::cover}) {
    TowerParams tp; tp.kmax=3; tp.entry=entry;
    auto tw=build_tower(cloud,tree,cat.value(),tp,pool);
    if (!tw.ok()) return 5;
    const OrderForest& f=tw.value().orders[1];
    const auto d=point_dendrogram(cat.value(),f,cloud);
    if (!validate(d).ok()) return 6;
    unsigned pair_node=0,tri_node=0,np=0,nt=0;
    for (unsigned v=0;v<f.rank.size();++v) {
      if (f.rank[v]==0) continue;
      const auto& level=cat.value().level[f.rank[v]-1];
      const bool p=geom::compare(level,ab)==0,t=geom::compare(level,abc)==0;
      if (p) {pair_node=v; ++np;}
      if (t) {tri_node=v; ++nt;}
      std::printf("node entry=%s v=%u exact_rank=%u rendered_rank=%u parent=%u hex=%a pair=%d triangle=%d\n",entry==PointEntry::core?"core":"cover",v,f.rank[v],d.node_rank[v],f.parent[v],level.approx(),p,t);
    }
    if (np!=1 || nt!=1 || f.rank[pair_node]>=f.rank[tri_node] || d.node_rank[pair_node]!=d.node_rank[tri_node]) return 7;
    ++found;
  }
  std::printf("coalescence_ok entries=%u native_full_exact_comparison_preserved=1 point_ranks_coalesced=1\n",found);
  return found==2?0:8;
}
