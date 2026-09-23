#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>
struct P{uint32_t x,y,z;}; struct E{uint32_t a,b,F,mask;};
template<class T> std::vector<T> read(const char* path){std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f) throw std::runtime_error(path);auto n=f.tellg();if(n%sizeof(T))throw std::runtime_error("size");std::vector<T> v(n/sizeof(T)); f.seekg(0);f.read((char*)v.data(),n);return v;}
int main(int argc,char**argv){if(argc!=5)return 2;auto ps=read<P>(argv[1]);auto ids=read<uint32_t>(argv[2]);if(ps.size()!=123389||ps.size()!=ids.size())throw std::runtime_error("input count");std::vector<P> byid(size_t(*std::max_element(ids.begin(),ids.end()))+1);std::vector<uint8_t> present(byid.size());for(size_t i=0;i<ps.size();++i){if(present[ids[i]]||ps[i].x>=(1U<<18)||ps[i].y>=(1U<<18)||ps[i].z>=(1U<<18))throw std::runtime_error("input identity/coordinate");present[ids[i]]=1;byid[ids[i]]=ps[i];}std::ofstream out(argv[4],std::ios::binary);if(!out)return 3;uint64_t n=0,sumF=0;for(int part=0;part<8;++part){char path[4096];std::snprintf(path,sizeof(path),"%s/part_%d.bin",argv[3],part);auto es=read<E>(path);for(auto e:es){if(e.a>=present.size()||e.b>=present.size()||!present[e.a]||!present[e.b]||e.a>=e.b||e.F<2||(e.mask!=2&&e.mask!=4&&e.mask!=6))throw std::runtime_error("trace edge identity");auto a=byid[e.a],b=byid[e.b];int64_t dx=int64_t(a.x)-b.x,dy=int64_t(a.y)-b.y,dz=int64_t(a.z)-b.z;uint64_t D=dx*dx+dy*dy+dz*dz;unsigned m=(std::max)({uint64_t(std::abs(dx)),uint64_t(std::abs(dy)),uint64_t(std::abs(dz))});unsigned axis=uint64_t(std::abs(dx))==m?0:uint64_t(std::abs(dy))==m?1:2;if(D>=(1ULL<<23)&&((a.x+b.x)/2)>>12==19&&((a.y+b.y)/2)>>12==18&&((a.z+b.z)/2)>>12==6&&axis==0){if(a.x>b.x)std::swap(e.a,e.b);out.write((char*)&e,sizeof(e));++n;sumF+=e.F;}}}std::cerr<<"group "<<n<<" F "<<sumF<<"\n";}
