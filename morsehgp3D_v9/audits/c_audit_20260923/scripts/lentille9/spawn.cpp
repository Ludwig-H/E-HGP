#include <thread>
#include <chrono>
#include <cstdio>
#include <vector>
int main(){
  using C=std::chrono::steady_clock;
  volatile int sink=0;
  // warmup
  for(int i=0;i<50;++i){ std::thread t([&]{sink=sink+1;}); t.join(); }
  const int N=2000;
  auto t0=C::now();
  for(int i=0;i<N;++i){ std::thread t([&]{sink=sink+1;}); t.join(); }
  auto t1=C::now();
  double us=std::chrono::duration<double,std::micro>(t1-t0).count()/N;
  // 2 threads spawned back-to-back then joined (team of 2)
  auto t2=C::now();
  for(int i=0;i<N;++i){ std::vector<std::thread> v; v.reserve(2); for(int k=0;k<2;++k) v.emplace_back([&]{sink=sink+1;}); for(auto&x:v) x.join(); }
  auto t3=C::now();
  double us2=std::chrono::duration<double,std::micro>(t3-t2).count()/N;
  std::printf("spawn+join 1 thread: %.2f us ; team of 2: %.2f us per team\n",us,us2);
}
