// Independent audit prototype, not linked to the HGP engine.
// Exact, bounded-pool certificates. A failed proposal never rejects an edge.
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using I = __int128_t;
using L = std::int64_t;
using U = std::uint64_t;
using P = std::array<L,3>;
struct HV { L h{}; P v{}; };
struct Query { unsigned q{}, k{}; P a{}, b{}; std::vector<P> pool; };
struct Result {
  unsigned single{}, match{}, triangle{}, bound{};
  U tests{}; bool guard_skip{};
  std::vector<std::vector<unsigned>> groups;
};
P sub(P a,P b) { for(unsigned i=0;i<3;++i) a[i]-=b[i]; return a; }
L dot(P a,P b) { L r=0; for(unsigned i=0;i<3;++i) r+=a[i]*b[i]; return r; }
P cross(P a,P b) {
  return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
HV hv(P a,P b,P z) { return {dot(sub(z,a),sub(b,z)),cross(sub(b,a),sub(z,a))}; }
HV add(HV a,HV b) { a.h+=b.h; for(unsigned i=0;i<3;++i) a.v[i]+=b.v[i]; return a; }
bool cert(HV p,unsigned q,const std::string& mutant) {
  // M=262143. For at most two sites: |h|<=6M^2, |v_i|<=4M^2.
  // alpha*h^2<=108M^4<2^79; sum v_i^2<=48M^4<2^78.
  if(mutant!="no_h" && p.h<=0) return false;
  const I alpha=(q==3 || mutant=="q4_alpha3")?3:2;
  I norm=0; for(auto x:p.v) norm+=I(x)*x;
  const I left=alpha*I(p.h)*p.h;
  return mutant=="nonstrict"?left>=norm:left>norm;
}
void validate(const Query& q) {
  if((q.q!=3 && q.q!=4) || q.k<q.q-1 || q.k>10 || q.pool.size()>64 || q.a==q.b)
    throw std::invalid_argument("invalid q/K/pool/edge");
  const auto point=[](P p) { for(auto x:p) if(x<0 || x>262143) throw std::invalid_argument("coordinate range"); };
  point(q.a); point(q.b); for(auto p:q.pool) point(p);
  auto sorted=q.pool; std::sort(sorted.begin(),sorted.end());
  if(std::adjacent_find(sorted.begin(),sorted.end())!=sorted.end()) throw std::invalid_argument("duplicate pool site");
}
std::vector<Query> read() {
  std::vector<Query> out;
  for(;;) {
    Query q; unsigned n{};
    if(!(std::cin>>q.q)) { if(!std::cin.eof()) throw std::invalid_argument("invalid record"); break; }
    if(!(std::cin>>q.k)) throw std::invalid_argument("truncated record");
    for(auto& x:q.a) if(!(std::cin>>x)) throw std::invalid_argument("truncated edge");
    for(auto& x:q.b) if(!(std::cin>>x)) throw std::invalid_argument("truncated edge");
    if(!(std::cin>>n) || n>64) throw std::invalid_argument("pool size");
    q.pool.resize(n); for(auto& p:q.pool) for(auto& x:p) if(!(std::cin>>x)) throw std::invalid_argument("truncated pool");
    validate(q); out.push_back(std::move(q));
  }
  return out;
}
unsigned matching(U available,const std::array<U,64>& adj,std::vector<std::vector<unsigned>>& groups) {
  unsigned count=0;
  while(available) {
    const unsigned i=std::countr_zero(available); available&=~(U{1}<<i);
    const U candidates=available&adj[i];
    if(candidates) { const unsigned j=std::countr_zero(candidates); available&=~(U{1}<<j); groups.push_back({i,j}); ++count; }
  }
  return count;
}
Result assess(const Query& q,const std::string& mutant="",bool use_triangles=true,bool guards=true) {
  Result out; std::vector<HV> values; U available=0;
  for(unsigned i=0;i<q.pool.size();++i) {
    const auto p=hv(q.a,q.b,q.pool[i]); values.push_back(p);
    if(cert(p,q.q,mutant)) { ++out.single; out.groups.push_back({i}); }
    else available|=U{1}<<i;
  }
  out.match=out.triangle=out.bound=out.single;
  const unsigned threshold=q.k-q.q+2;
  if(out.single>=threshold) return out;
  // In this packing, a pair needs >=1 positive-H site and a triangle >=2.
  // With no individual certificate left, at most floor(2*m/3) credits can
  // be obtained by disjoint pairs/triangles. These are impossibility guards,
  // not heuristic rejection or a cap on the native engine's exploration.
  unsigned positive=0; for(const auto& p:values) positive+=p.h>0;
  const unsigned packing=out.single+2*std::popcount(available)/3;
  if(guards && (positive<threshold || packing<threshold)) { out.guard_skip=true; return out; }
  std::array<U,64> adj{};
  U scan=available;
  while(scan) {
    const unsigned i=std::countr_zero(scan); scan&=~(U{1}<<i);
    for(U rest=scan;rest;rest&=rest-1) {
      const unsigned j=std::countr_zero(rest); ++out.tests;
      if(cert(add(values[i],values[j]),q.q,mutant)) { adj[i]|=U{1}<<j; adj[j]|=U{1}<<i; }
    }
  }
  auto pairs=out.groups;
  out.match+=matching(available,adj,pairs);
  if(mutant=="pair_two") out.match=out.single+2*(out.match-out.single);
  if(mutant=="overlap") {
    out.match=out.single; pairs=out.groups;
    for(unsigned i=0;i<q.pool.size();++i) for(unsigned j=i+1;j<q.pool.size();++j)
      if(adj[i]&(U{1}<<j)) { ++out.match; pairs.push_back({i,j}); }
  }
  out.bound=out.match;
  auto triangles=out.groups;
  if(use_triangles && out.match<threshold) {
    U remaining=available;
    for(unsigned i=0;i<q.pool.size();++i) {
      if(!(remaining&(U{1}<<i))) continue;
      for(U js=adj[i]&remaining&~(U{1}<<i);js;js&=js-1) {
        const unsigned j=std::countr_zero(js);
        const U ks=adj[i]&adj[j]&remaining&~(U{1}<<i)&~(U{1}<<j);
        if(ks) {
          const unsigned k=std::countr_zero(ks);
          triangles.push_back({i,j,k}); out.triangle+=2;
          remaining&=~((U{1}<<i)|(U{1}<<j)|(U{1}<<k)); break;
        }
      }
    }
    out.triangle+=matching(remaining,adj,triangles);
  }
  // Either cover is valid; greedy triangles need not dominate greedy matching.
  if(out.triangle>out.bound) { out.bound=out.triangle; out.groups=std::move(triangles); }
  else out.groups=std::move(pairs);
  return out;
}
void print(const Result& r) {
  std::cout<<"{\"single\":"<<r.single<<",\"matching\":"<<r.match<<",\"triangles\":"<<r.triangle
    <<",\"bound\":"<<r.bound<<",\"pair_tests\":"<<r.tests<<",\"guard_skip\":"<<(r.guard_skip?"true":"false")<<",\"groups\":[";
  bool first=true;
  for(const auto& g:r.groups) { if(!first) std::cout<<','; first=false; std::cout<<'[';
    for(unsigned j=0;j<g.size();++j) { if(j) std::cout<<','; std::cout<<g[j]; } std::cout<<']'; }
  std::cout<<"]}\n";
}
int main(int argc,char** argv) {
  try {
    const auto queries=read();
    if((argc==3 || argc==4) && std::string(argv[1])=="--bench") {
      const auto rounds=std::stoul(argv[2]); if(rounds==0) throw std::invalid_argument("zero rounds");
      U digest=0;
      const unsigned rotation=argc==4?std::stoul(argv[3])%4:0;
      for(unsigned step=0;step<4;++step) {
        const unsigned mode=(step+rotation)%4;
        const auto start=std::chrono::steady_clock::now();
        for(unsigned long r=0;r<rounds;++r) for(const auto& q:queries) {
          if(mode==0) { unsigned count=0; for(auto p:q.pool) count+=cert(hv(q.a,q.b,p),q.q,""); digest+=count; }
          else { const auto v=assess(q,"",mode>=2,mode==3); digest+=v.bound; }
        }
        const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();
        std::cout<<"{\"mode\":"<<mode<<",\"queries\":"<<queries.size()<<",\"rounds\":"<<rounds
          <<",\"ns\":"<<elapsed<<",\"digest\":"<<digest<<"}\n";
      }
    } else {
      const std::string mutant=argc==3 && std::string(argv[1])=="--mutant"?argv[2]:"";
      const bool unguarded=argc==2 && std::string(argv[1])=="--unguarded";
      if(argc!=1 && mutant.empty() && !unguarded) throw std::invalid_argument("arguments");
      for(const auto& q:queries) print(assess(q,mutant,true,!unguarded && mutant.empty()));
    }
  } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 2; }
}
