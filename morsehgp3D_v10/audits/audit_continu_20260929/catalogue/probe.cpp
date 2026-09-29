#include <fstream>
#include <iostream>
#include <vector>
#include "catalogue/catalogue.hpp"
using namespace mhgp10;
int main(int argc, char** argv) {
  if (argc != 5) return 2;
  std::ifstream in(argv[1]);
  std::vector<u32> x,y,z,ids;
  u32 a,b,c;
  while (in >> a >> b >> c) { x.push_back(a); y.push_back(b); z.push_back(c); ids.push_back(u32(ids.size())); }
  auto prep=prepare_cloud(x,y,z,ids);
  if (!prep.ok()) return 2;
  sched::Pool pool(unsigned(std::stoul(argv[4])));
  CatalogueParams params;
  params.kmax=std::stoi(argv[2]); params.leaf_size=u32(std::stoul(argv[3]));
  auto result=build_catalogue(prep.value(),params,pool);
  if (!result.ok()) return 2;
  const auto& cat=result.value();
  const auto& cloud=prep.value();
  auto coord=[&](u32 s) { std::cout << cloud.x[s] << ',' << cloud.y[s] << ',' << cloud.z[s] << ' '; };
  for(u32 i=0;i<cat.balls();++i) {
    const auto& lev=cat.level[cat.rank[i]];
    std::cout<<cat.rank[i]<<' '<<unsigned(cat.qmin[i])<<' '<<cat.p[i]<<' '<<cat.u[i]<<' '<<unsigned(cat.flags[i])<<' '
             <<arith::to_string(lev.num)<<' '<<arith::to_string(lev.den)<<'|';
    for(auto s:cat.support[i]) if(s!=kNone) coord(s);
    std::cout<<'|'; for(auto s:cat.interior(i)) coord(s);
    std::cout<<'|'; for(auto s:cat.shell(i)) coord(s);
    std::cout<<'\n';
  }
}
