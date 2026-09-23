#define main probe_main
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "probe.cpp"
#pragma GCC diagnostic pop
#undef main

#include <random>

static unsigned brute(const std::vector<PairBits>& edges,int n,int lane) {
  std::vector<unsigned> adj(n);
  for(auto e:edges)if(lane==3?e.q3:e.q4) {
    adj[e.i]|=1U<<e.j;adj[e.j]|=1U<<e.i;
  }
  const unsigned full=(1U<<n)-1;
  std::vector<int> memo(full+1,-1);
  auto solve=[&](auto&& self,unsigned mask)->int {
    if(!mask)return 0;
    int& m=memo[mask];if(m!=-1)return m;
    const unsigned i=std::countr_zero(mask),rest=mask&~(1U<<i);
    m=self(self,rest);
    for(unsigned j=i+1;j<unsigned(n);++j)
      if((rest&(1U<<j))&&(adj[i]&(1U<<j)))
        m=std::max(m,1+self(self,rest&~(1U<<j)));
    return m;
  };
  return solve(solve,full);
}

int main() {
  std::mt19937_64 rng(0x9232026ULL);
  std::uint64_t graphs=0;
  for(int n=2;n<=10;++n)for(int t=0;t<3000;++t) {
    std::vector<PairBits> edges;
    for(int i=0;i<n;++i)for(int j=i+1;j<n;++j) {
      const auto bits=rng()&3U;
      if(bits)edges.push_back({std::size_t(i),std::size_t(j),bool(bits&1U),bool(bits&2U)});
    }
    for(int lane:{3,4})
      ck(maximum_matching(edges,n,lane)==brute(edges,n,lane),"matching brute mismatch");
    ++graphs;
  }
  const Box3 A{{5,10,10},{6,10,10}},B{{15,10,10},{15,10,10}};
  const std::array<i64,4> expected{40,72,88,88};
  for(int x=8;x<=11;++x) {
    const Point3 g{x,13,10},h{x,7,10};
    std::uint64_t corners=0;
    const auto cert=classify(A,B,g,h,0,1,corners);
    ck(cert.q3&&cert.q4&&corners==64,"published rectangle pair fixture");
    i64 minimum=std::numeric_limits<i64>::max();
    for(unsigned ai=0;ai<8;++ai)for(unsigned bi=0;bi<8;++bi)
      minimum=std::min(minimum,point_formula(box_corner(A,ai),box_corner(B,bi),g,h).first);
    ck(minimum==expected[x-8],"published H minimum");
    PredicateWork work{};
    ck(!box_witness(Lane::Q3,A,B,g,work),"singleton should fail");
    ck(!box_witness(Lane::Q3,A,B,h,work),"singleton should fail");
  }
  std::cout<<"PASS blossom vs brute "<<graphs<<" random graphs, exact rectangle fixture\n";
}
