#include <time.h>
#include <cstdio>
#include <cstdint>
static inline uint64_t tns(clockid_t c){timespec ts; clock_gettime(c,&ts); return uint64_t(ts.tv_sec)*1000000000ull+ts.tv_nsec;}
int main(){
  const int N=400000;
  for (int rep=0;rep<3;++rep){
  // cost of CLOCK_THREAD_CPUTIME_ID calls measured with itself and with MONOTONIC
  uint64_t m0=tns(CLOCK_MONOTONIC), c0=tns(CLOCK_THREAD_CPUTIME_ID);
  volatile uint64_t sink=0;
  for(int i=0;i<N;++i) sink+=tns(CLOCK_THREAD_CPUTIME_ID);
  uint64_t m1=tns(CLOCK_MONOTONIC), c1=tns(CLOCK_THREAD_CPUTIME_ID);
  // back-to-back delta: what an empty timed window reads
  uint64_t acc=0;
  for(int i=0;i<N;++i){uint64_t a=tns(CLOCK_THREAD_CPUTIME_ID); uint64_t b=tns(CLOCK_THREAD_CPUTIME_ID); acc+=b-a;}
  uint64_t accm=0;
  for(int i=0;i<N;++i){uint64_t a=tns(CLOCK_MONOTONIC); uint64_t b=tns(CLOCK_MONOTONIC); accm+=b-a;}
  std::printf("THREAD_CPUTIME call: %.1f ns wall/call, %.1f ns cpu/call ; empty window reads %.1f ns (thread cpu) ; MONOTONIC empty window %.1f ns\n",
    double(m1-m0)/N, double(c1-c0)/N, double(acc)/N, double(accm)/N);
  }
}
