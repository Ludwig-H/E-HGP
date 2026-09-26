#include "narrow_pair.hpp"
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdio>
#include <stdexcept>

using namespace mhgp9::audit_b::s2_narrow;
using Big = boost::multiprecision::cpp_int;

int main() try {
  std::uint64_t all=0, rejected=0, narrow=0, fallback=0;
  for (const auto scale : {1, 32768, 262143}) for (const auto translate : {false,true}) {
    const int base=translate ? 262143-scale : 0;
    for (unsigned a_code=0;a_code<8;++a_code) for (unsigned b_code=0;b_code<8;++b_code)
      for (unsigned box_code=0;box_code<27;++box_code) {
        std::int32_t a[3],b[3]; FlatBox box{}; auto code=box_code;
        for (unsigned j=0;j<3;++j) {
          a[j]=base+(((a_code>>j)&1U)!=0 ? scale : 0);
          b[j]=base+(((b_code>>j)&1U)!=0 ? scale : 0);
          const unsigned state=code%3; code/=3;
          box.low[j]=base+(state==2 ? scale : 0);
          box.high[j]=base+(state==0 ? 0 : scale);
        }
        const auto pair=prepare_pair(a,b); const auto h=pair_h(pair,box);
        Big d2=0, far2=0, near2=0;
        for (int j=0;j<3;++j) {
          const Big d=Big(b[j])-a[j]; d2+=d*d;
          const Big lo=2*Big(box.low[j])-a[j]-b[j], hi=2*Big(box.high[j])-a[j]-b[j];
          const Big l2=lo*lo, r2=hi*hi;
          far2 += l2>r2 ? l2:r2;
          if(lo>0) near2+=l2; else if(hi<0) near2+=r2;
        }
        if (d2-far2!=h.minimum4 || d2-near2!=h.maximum4) throw std::runtime_error("H oracle");
        ++all;
        if(h.maximum4<=0) {++rejected; continue;}
        const auto c=cross_extrema(pair,box); Big low2=0,high2=0;
        for(int axis=0;axis<3;++axis) {
          Big lo=0,hi=0;
          const int j=(axis+1)%3,k=(axis+2)%3;
          for(unsigned corner=0;corner<8;++corner) {
            const Big zj=((corner>>j)&1U)!=0 ? box.high[j]:box.low[j];
            const Big zk=((corner>>k)&1U)!=0 ? box.high[k]:box.low[k];
            const Big x=(Big(b[j])-a[j])*(zk-a[k])-(Big(b[k])-a[k])*(zj-a[j]);
            if(corner==0 || x<lo) lo=x;
            if(corner==0 || x>hi) hi=x;
          }
          if(lo!=c.low[axis] || hi!=c.high[axis]) throw std::runtime_error("cross vertex oracle");
          const Big l2=lo*lo,r2=hi*hi;
          high2+=l2>r2 ? l2:r2;
          if(!(lo<=0 && hi>=0)) low2+=l2<r2 ? l2:r2;
        }
        const auto decision=decide(pair,box,h); ++(decision.narrow?narrow:fallback);
        for(unsigned lane=0;lane<2;++lane) {
          const Big alpha=lane==0 ? 3:2; const u8 bit=static_cast<u8>(2U<<lane);
          const bool exclude=alpha*Big(h.maximum4)*h.maximum4<=16*low2;
          const bool admit=!exclude && h.minimum4>0 && alpha*Big(h.minimum4)*h.minimum4>16*high2;
          if(exclude!=((decision.excluded&bit)!=0) || admit!=((decision.admitted&bit)!=0))
            throw std::runtime_error("decision oracle");
        }
      }
  }
  if(all!=10368 || !rejected || !narrow || !fallback) throw std::runtime_error("nonvacuity");
  std::printf("{\"status\":\"pass\",\"geometries\":%llu,\"h_rejected\":%llu,\"narrow\":%llu,\"fallback\":%llu}\n",
      static_cast<unsigned long long>(all),static_cast<unsigned long long>(rejected),
      static_cast<unsigned long long>(narrow),static_cast<unsigned long long>(fallback));
  return 0;
} catch(const std::exception& e) { std::fprintf(stderr,"failure: %s\n",e.what()); return 1; }
