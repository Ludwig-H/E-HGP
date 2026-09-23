#include <sys/resource.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "tower/forest/ball_data.hpp"
using mhgp9::tower::BallData;
static long minflt(){ rusage r{}; getrusage(RUSAGE_SELF,&r); return r.ru_minflt; }
int main(int argc,char**argv){
  const std::size_t B = std::strtoull(argv[1],nullptr,10);
  using C=std::chrono::steady_clock;
  for(int rep=0;rep<2;++rep){
    long f0=minflt(); auto t0=C::now();
    double init_ms, free_ms; long f1;
    {
      std::vector<BallData> balls(B);
      init_ms=std::chrono::duration<double,std::milli>(C::now()-t0).count(); f1=minflt();
      volatile unsigned s=balls[B/2].arity; (void)s;
      t0=C::now();
    }
    free_ms=std::chrono::duration<double,std::milli>(C::now()-t0).count();
    std::printf("sizeof=%zu B=%zu bytes=%zu init_ms=%.1f minflt=%ld free_ms=%.1f\n",sizeof(BallData),B,B*sizeof(BallData),init_ms,f1-f0,free_ms);
  }
}
