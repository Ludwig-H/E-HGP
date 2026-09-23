#include <algorithm>
#include <array>
#include <cfenv>
#include <cstdio>
#include <random>
#include <vector>
#include <xmmintrin.h>
#include "../src/tower/forest/anchor_meb.hpp"
using namespace mhgp9::tower;
static bool same(const AnchorMebResult& a, const AnchorMebResult& b) {
  return a.status == b.status && a.key == b.key && same_exact_level(a.level,b.level) &&
    a.support_size == b.support_size && a.support_slots == b.support_slots &&
    a.selected_shell_count == b.selected_shell_count;
}
int main(){
  const unsigned initial=_mm_getcsr();
  unsigned long long cases=0, fail=0, verify=0, fallback=0, canonical=0;
  for(int rnd:{FE_TONEAREST,FE_UPWARD,FE_DOWNWARD,FE_TOWARDZERO})
    for(int flush=0;flush<2;++flush){
      std::fesetround(rnd);
      unsigned csr=_mm_getcsr(); csr=(csr & ~(unsigned(1)<<15 | unsigned(1)<<6)) | (flush ? (unsigned(1)<<15 | unsigned(1)<<6) : 0); _mm_setcsr(csr);
      std::mt19937_64 rng(20260923);
      for(unsigned bound:{262144u,65536u,64u,8u,4u}) for(unsigned n=1;n<=10;++n)
        for(unsigned rep=0;rep<100;++rep){
          std::vector<P3> s;
          while(s.size()<n){P3 p{static_cast<i64>(rng()%bound),static_cast<i64>(rng()%bound),static_cast<i64>(rng()%bound)};
            if(std::find(s.begin(),s.end(),p)==s.end()) s.push_back(p);}
          AnchorMebWork a,b;auto x=anchor_meb(s,a),y=anchor_meb_proposed(s,b);
          ++cases;verify+=b.verified_proposals;fallback+=b.proposal_fallbacks;canonical+=b.boundary_canonicalizations;
          if(!same(x,y)){++fail;std::printf("FAIL mode=%d flush=%d bound=%u n=%u rep=%u\n",rnd,flush,bound,n,rep);}
        }
      const std::vector<P3> cube{{0,0,0},{2,0,0},{0,2,0},{0,0,2},{2,2,0},{2,0,2},{0,2,2},{2,2,2}};
      const std::vector<P3> oct{{2,1,1},{0,1,1},{1,2,1},{1,0,1},{1,1,2},{1,1,0}};
      for(const auto& base:{cube,oct})for(unsigned mask=1;mask<(1u<<base.size());++mask){std::vector<P3>s;
        for(unsigned j=0;j<base.size();++j)if(mask&(1u<<j))s.push_back(base[j]);
        AnchorMebWork a,b;auto x=anchor_meb(s,a),y=anchor_meb_proposed(s,b);++cases;verify+=b.verified_proposals;fallback+=b.proposal_fallbacks;canonical+=b.boundary_canonicalizations;
        if(!same(x,y)){++fail;std::printf("FAIL mode=%d flush=%d mask=%u\n",rnd,flush,mask);}
      }
    }
  _mm_setcsr(initial);std::fesetround(FE_TONEAREST);
  std::printf("cases=%llu fail=%llu verified=%llu fallback=%llu canonical=%llu\n",cases,fail,verify,fallback,canonical);
  return fail?1:0;
}
