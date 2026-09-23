// Audit-only deterministic shadow: real WSPD rectangles, K5/s8, u18.
#include "gen/lanes/q34_witness_search.hpp"
#include "gen/pipeline/prepared_cloud.hpp"
#include "gen/spindle/predicates.hpp"
#include "gen/wspd/front.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace mhgp9::gen;
using Clock = std::chrono::steady_clock;
using V = std::array<i64,3>;

static void ck(bool v, const char* why) { if (!v) throw std::runtime_error(why); }
static std::uint64_t ns(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count();
}
static std::vector<unsigned char> bytes(const std::filesystem::path& p) {
  std::ifstream f(p,std::ios::binary); ck(bool(f),"input open");
  return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
static std::uint32_t u32(const unsigned char* p) {
  return std::uint32_t(p[0]) | (std::uint32_t(p[1])<<8) |
      (std::uint32_t(p[2])<<16) | (std::uint32_t(p[3])<<24);
}
static std::uint64_t key(std::uint32_t a, std::uint32_t b) {
  if(a>b) std::swap(a,b);
  ck(a!=b,"self pair");
  return (std::uint64_t(a)<<32)|b;
}
static std::uint64_t mix(std::uint64_t x) {
  x+=0x9e3779b97f4a7c15ULL; x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;
  x=(x^(x>>27))*0x94d049bb133111ebULL; return x^(x>>31);
}
struct Rect {
  std::size_t an{},bn{}; std::uint64_t ordinal{},product{},hash{};
  std::uint8_t mask{}; std::uint64_t segment{},forms{},q3edges{},q4edges{};
};
static int stratum(std::uint64_t p) {
  if(p<16) return -1;
  if(p<64) return 0;
  if(p<256) return 1;
  if(p<1024) return 2;
  if(p<4096) return 3;
  if(p<16384) return 4;
  return 5;
}
static int quota(int s) { return s<3 ? 8 : 2000; }
static V vec(Point3 p) { return {p.x,p.y,p.z}; }
static V sub(V a,V b) {return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
static i64 dot(V a,V b) {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
struct Geometry {
  V s{},d{},u{},v{}; i64 D{};
  Geometry(Point3 a,Point3 b) {
    s={i64(a.x)+b.x,i64(a.y)+b.y,i64(a.z)+b.z};
    d=sub(vec(b),vec(a)); D=dot(d,d); ck(D>0,"zero edge");
    int main=0; for(int i=1;i<3;++i) if(std::abs(d[i])>std::abs(d[main])) main=i;
    const int ia=(main+1)%3,ib=(main+2)%3;
    const i64 h=std::abs(d[main]),sgn=d[main]>0?1:-1;
    u[ia]=h;u[main]=-sgn*d[ia];v[ib]=h;v[main]=-sgn*d[ib];
    ck(dot(u,d)==0 && dot(v,d)==0,"basis");
  }
  V w(Point3 z) const {return {2*i64(z.x)-s[0],2*i64(z.y)-s[1],2*i64(z.z)-s[2]};}
  int quadrant(V w) const {return (int(dot(w,u)>=0)<<1)|int(dot(w,v)>=0);}
};
struct Bound {i64 qmin{},umin{},umax{},vmin{},vmax{};};
static Bound bound(const Geometry& g,Box3 box) {
  Bound o;
  for(int i=0;i<3;++i) {
    const i64 lo=2*i64(box.low[i])-g.s[i],hi=2*i64(box.high[i])-g.s[i];
    const i64 near=lo>0?lo:(hi<0?hi:0);o.qmin+=near*near;
    const i64 ua=lo*g.u[i],ub=hi*g.u[i],va=lo*g.v[i],vb=hi*g.v[i];
    o.umin+=std::min(ua,ub);o.umax+=std::max(ua,ub);
    o.vmin+=std::min(va,vb);o.vmax+=std::max(va,vb);
  }
  return o;
}
static bool can_hit(Bound b,i64 D,int q) {
  return b.qmin<=D && ((q&2)?b.umax>=0:b.umin<0) &&
      ((q&1)?b.vmax>=0:b.vmin<0);
}
struct Candidate {std::size_t site{};i64 q{};};
struct QueueItem {i64 q{};std::size_t node{};};
struct Greater {bool operator()(QueueItem a,QueueItem b) const {
  return std::tie(a.q,a.node)>std::tie(b.q,b.node);
}};
struct SearchWork {std::uint64_t pops{},boxes{},sites{},cuts{};};
static std::vector<Candidate> query(const Q2CensusIndex& idx,const Geometry& g,
    Range ar,Range br,int quadrant,std::size_t max_pops,SearchWork& work) {
  const auto nodes=idx.spatial_nodes();
  const auto order=idx.spatial_order();
  const auto points=idx.cloud().points();
  std::priority_queue<QueueItem,std::vector<QueueItem>,Greater> todo;
  auto push=[&](std::size_t node) {
    ++work.boxes;auto b=bound(g,nodes[node].box);
    if(can_hit(b,g.D,quadrant))todo.push({b.qmin,node});
  };
  push(0);
  std::vector<Candidate> selected;
  while(!todo.empty()) {
    if(work.pops==max_pops) {++work.cuts;break;}
    auto x=todo.top();todo.pop();++work.pops;
    const auto& n=nodes[x.node];
    if(n.left!=Q2SpatialNode::absent) {push(n.left);push(n.right);continue;}
    for(std::size_t rank=n.range.first;rank<n.range.last;++rank) {
      if((ar.first<=rank && rank<ar.last)||(br.first<=rank && rank<br.last))continue;
      const auto site=order[rank];
      const auto w=g.w(points[site]);
      ++work.sites;
      const i64 q=dot(w,w); if(q>g.D||g.quadrant(w)!=quadrant)continue;
      selected.push_back({site,q});
    }
  }
  std::sort(selected.begin(),selected.end(),[](auto a,auto b){return std::tie(a.q,a.site)<std::tie(b.q,b.site);});
  if(selected.size()>8) selected.resize(8);
  return selected;
}
struct PairBits {std::size_t i{},j{};bool q3{},q4{};};
static std::pair<i64,i128> point_formula(Point3 a,Point3 b,Point3 g,Point3 h) {
  const V d=sub(vec(b),vec(a)); const i64 D=dot(d,d);
  const V wg{2*i64(g.x)-a.x-b.x,2*i64(g.y)-a.y-b.y,2*i64(g.z)-a.z-b.z};
  const V wh{2*i64(h.x)-a.x-b.x,2*i64(h.y)-a.y-b.y,2*i64(h.z)-a.z-b.z};
  const i64 H=2*D-dot(wg,wg)-dot(wh,wh);
  const V w{wg[0]+wh[0],wg[1]+wh[1],wg[2]+wh[2]};
  const V c{d[1]*w[2]-d[2]*w[1],d[2]*w[0]-d[0]*w[2],d[0]*w[1]-d[1]*w[0]};
  const i128 X=i128(c[0])*c[0]+i128(c[1])*c[1]+i128(c[2])*c[2];
  return {H,X};
}
static PairBits classify(Box3 a,Box3 b,Point3 g,Point3 h,
    std::size_t i,std::size_t j,std::uint64_t& corners) {
  PairBits out{i,j,true,true};
  for(unsigned ai=0;ai<8 && (out.q3||out.q4);++ai)
    for(unsigned bi=0;bi<8 && (out.q3||out.q4);++bi) {
      ++corners;const auto [H,X]=point_formula(box_corner(a,ai),box_corner(b,bi),g,h);
      const i128 H2=i128(H)*H;
      if(H<=0) {out.q3=false;out.q4=false;break;}
      if(3*H2<=4*X)out.q3=false;
      if(H2<=2*X)out.q4=false;
    }
  return out;
}
static unsigned greedy(std::vector<PairBits> pairs,std::size_t n,int lane) {
  std::vector<bool> used(n);unsigned credit=0;
  for(auto p:pairs) {
    if(!(lane==3?p.q3:p.q4)||used[p.i]||used[p.j])continue;
    used[p.i]=used[p.j]=true;++credit;
  }
  return credit;
}
// Edmonds blossom: exact maximum cardinality matching on at most 32 sites.
static unsigned maximum_matching(const std::vector<PairBits>& pairs,std::size_t n,int lane) {
  std::vector<std::vector<int>> graph(n);
  for(const auto& e:pairs) if(lane==3?e.q3:e.q4) {
    graph[e.i].push_back(int(e.j));graph[e.j].push_back(int(e.i));
  }
  std::vector<int> match(n,-1),parent(n,-1),base(n),queue;
  std::vector<bool> used(n),flower(n),seen(n);
  auto lca=[&](int a,int b) {
    std::fill(seen.begin(),seen.end(),false);
    while(true) {
      a=base[a];seen[a]=true;
      if(match[a]==-1)break;
      a=parent[match[a]];
    }
    while(true) {
      b=base[b];if(seen[b])return b;
      b=parent[match[b]];
    }
  };
  auto mark_path=[&](int v,int root,int child) {
    while(base[v]!=root) {
      flower[base[v]]=true;flower[base[match[v]]]=true;
      parent[v]=child;child=match[v];v=parent[match[v]];
    }
  };
  auto find_path=[&](int root) {
    std::fill(used.begin(),used.end(),false);
    std::fill(parent.begin(),parent.end(),-1);
    for(std::size_t i=0;i<n;++i)base[i]=int(i);
    queue.clear();queue.push_back(root);used[root]=true;
    for(std::size_t head=0;head<queue.size();++head) {
      const int v=queue[head];
      for(const int to:graph[v]) {
        if(base[v]==base[to] || match[v]==to)continue;
        if(to==root || (match[to]!=-1 && parent[match[to]]!=-1)) {
          const int b=lca(v,to);
          std::fill(flower.begin(),flower.end(),false);
          mark_path(v,b,to);mark_path(to,b,v);
          for(std::size_t i=0;i<n;++i)if(flower[base[i]]) {
            base[i]=b;
            if(!used[i]) {used[i]=true;queue.push_back(int(i));}
          }
        } else if(parent[to]==-1) {
          parent[to]=v;
          if(match[to]==-1)return to;
          const int next=match[to];used[next]=true;queue.push_back(next);
        }
      }
    }
    return -1;
  };
  for(std::size_t i=0;i<n;++i)if(match[i]==-1) {
    int v=find_path(int(i));
    while(v!=-1) {
      const int pv=parent[v],next=match[pv];
      match[v]=pv;match[pv]=v;v=next;
    }
  }
  unsigned count=0;for(int x:match)if(x!=-1)++count;
  return count/2;
}
struct Result {
  std::uint64_t select_ns{},pair_ns{},singleton_ns{};
  SearchWork search;std::uint64_t proposed{},first_positive3{},first_positive4{},
      pair_tests{},corners{},rect_positive3{},rect_positive4{},
      singleton_corner_tests{},singleton_positive3{},singleton_positive4{};
  unsigned pair3{},pair4{},max3{},max4{},single3{},single4{};
};
static Result measure(const Q2CensusIndex& idx,const Rect& r) {
  Result out;
  const auto nodes=idx.spatial_nodes();
  const auto order=idx.spatial_order();
  const auto points=idx.cloud().points();
  const auto& na=nodes[r.an],&nb=nodes[r.bn];
  const Geometry g(points[order[na.range.first]],points[order[nb.range.first]]);
  const auto start=Clock::now();
  std::vector<Candidate> palette;
  for(int q=0;q<4;++q) {
    SearchWork w;auto c=query(idx,g,na.range,nb.range,q,128,w);
    out.search.pops+=w.pops;out.search.boxes+=w.boxes;
    out.search.sites+=w.sites;out.search.cuts+=w.cuts;
    palette.insert(palette.end(),c.begin(),c.end());
  }
  out.proposed=palette.size();out.select_ns=ns(start,Clock::now());
  auto pstart=Clock::now();
  std::vector<PairBits> positives;positives.reserve(palette.size()*palette.size()/2);
  for(std::size_t i=0;i<palette.size();++i)
    for(std::size_t j=i+1;j<palette.size();++j) {
      ++out.pair_tests;const auto pa=points[palette[i].site],pb=points[palette[j].site];
      const auto [H,X]=point_formula(points[order[na.range.first]],points[order[nb.range.first]],pa,pb);
      const i128 H2=i128(H)*H;
      const bool first3=H>0 && 3*H2>4*X,first4=H>0 && H2>2*X;
      out.first_positive3+=first3;out.first_positive4+=first4;
      if(!first3 && !first4)continue;
      auto cert=classify(na.box,nb.box,pa,pb,i,j,out.corners);
      out.rect_positive3+=cert.q3;out.rect_positive4+=cert.q4;
      if(cert.q3||cert.q4) positives.push_back(cert);
    }
  out.pair3=greedy(positives,palette.size(),3);
  out.pair4=greedy(positives,palette.size(),4);
  out.max3=maximum_matching(positives,palette.size(),3);
  out.max4=maximum_matching(positives,palette.size(),4);
  ck(out.max3>=out.pair3 && out.max4>=out.pair4,"matching dominance");
  out.pair_ns=ns(pstart,Clock::now());
  auto sstart=Clock::now();
  for(auto c:palette) {
    const auto p=points[c.site];
    for(auto lane:{Lane::Q3,Lane::Q4}) {
      PredicateWork w{};
      const bool yes=box_witness(lane,na.box,nb.box,p,w);
      out.singleton_corner_tests+=w.corner_tests;
      if(yes) {
        if(lane==Lane::Q3)++out.singleton_positive3;
        else ++out.singleton_positive4;
      }
    }
  }
  out.single3=out.singleton_positive3;out.single4=out.singleton_positive4;
  out.singleton_ns=ns(sstart,Clock::now());
  return out;
}
int main(int argc,char** argv) try {
  ck(argc==4,"usage: probe points.u32le raw_ids.u32le trace_directory");
  const auto pbytes=bytes(argv[1]),rbytes=bytes(argv[2]);
  ck(pbytes.size()==123389*12ULL && rbytes.size()==123389*4ULL,"pinned input lengths");
  std::vector<Point3> points(123389);std::vector<std::uint32_t> raw(123389);
  for(std::size_t i=0;i<points.size();++i) {
    points[i]={Coordinate(u32(pbytes.data()+12*i)),Coordinate(u32(pbytes.data()+12*i+4)),Coordinate(u32(pbytes.data()+12*i+8))};
    raw[i]=u32(rbytes.data()+4*i);
  }
  const auto t0=Clock::now();auto idx=make_q2_cloud_index(prepare_cloud(points));
  const auto t1=Clock::now();
  const auto nodes=idx->spatial_nodes();
  const auto order=idx->spatial_order();
  std::array<std::vector<Rect>,6> sample;
  std::array<std::uint64_t,6> populations{},product_mass{};
  std::uint64_t front=0,open=0,front_mass=0,open_mass=0,visits=0;
  auto front_result=run_wspd_front(*idx,5,8,WspdFrontMode::MidpointSamples,
    [&](const WspdRectangle& r) {
      const auto& a=nodes[r.a_node];const auto& b=nodes[r.b_node];
      const auto product=std::uint64_t(a.range.size())*b.range.size();
      ++front;front_mass+=product;
      Q34WitnessSearchWork work{};Q34WitnessBoundsWork bounds{};
      const auto mask=filter_q34_witnesses(*idx,a.box,b.box,5,r.lane_mask,
          work,Q34WitnessBoundsMode::Affine,bounds);
      visits+=work.node_visits;if(!mask)return;
      ++open;open_mass+=product;
      const int s=stratum(product);if(s<0)return;
      ++populations[s];product_mass[s]+=product;
      Rect e{r.a_node,r.b_node,front,product,
        mix(front^0x9232026ULL),mask};
      auto& v=sample[s];
      if(v.size()<std::size_t(quota(s)))v.push_back(e);
      else {
        auto worst=std::max_element(v.begin(),v.end(),[](const Rect& x,const Rect& y){
          return std::tie(x.hash,x.ordinal)<std::tie(y.hash,y.ordinal);
        });
        if(std::tie(e.hash,e.ordinal)<std::tie(worst->hash,worst->ordinal))*worst=e;
      }
    },6);
  (void)front_result;
  const auto t2=Clock::now();
  ck(front==6175011 && front_mass==238364135 && open==2548453 &&
     open_mass==22034426 && visits==503488729,"front/rectangle ledger mismatch");
  std::vector<Rect> selected;
  for(auto& v:sample)selected.insert(selected.end(),v.begin(),v.end());
  std::sort(selected.begin(),selected.end(),[](const Rect& a,const Rect& b){return a.ordinal<b.ordinal;});
  std::unordered_map<std::uint64_t,std::size_t> by_edge;
  std::uint64_t sampled_mass=0;
  for(const auto& e:selected)sampled_mass+=e.product;
  by_edge.reserve(static_cast<std::size_t>(sampled_mass));
  for(std::size_t i=0;i<selected.size();++i) {
    const auto a=nodes[selected[i].an].range,b=nodes[selected[i].bn].range;
    for(std::size_t ai=a.first;ai<a.last;++ai)
      for(std::size_t bi=b.first;bi<b.last;++bi)
        ck(by_edge.emplace(key(raw[order[ai]],raw[order[bi]]),i).second,
           "sampled rectangles overlap on edge");
  }
  const auto t3=Clock::now();
  std::uint64_t traced=0,trace_forms=0;
  for(int part=0;part<8;++part) {
    const auto data=bytes(std::filesystem::path(argv[3])/("part_"+std::to_string(part)+".bin"));
    ck(data.size()%16==0,"trace alignment");
    for(std::size_t pos=0;pos<data.size();pos+=16) {
      const auto a=u32(data.data()+pos),b=u32(data.data()+pos+4),
          F=u32(data.data()+pos+8),mask=u32(data.data()+pos+12);
      ck(a<b && F>=2 && (mask==2||mask==4||mask==6),"trace record");
      ++traced;trace_forms+=F;
      auto it=by_edge.find(key(a,b));if(it==by_edge.end())continue;
      auto& e=selected[it->second];
      ck((mask&~e.mask)==0,"trace mask wider than rectangle");
      ++e.segment;e.forms+=F;e.q3edges+=bool(mask&2);e.q4edges+=bool(mask&4);
      by_edge.erase(it);
    }
  }
  const auto t4=Clock::now();
  ck(traced==3986433 && trace_forms==559661741,"trace ledger mismatch");
  std::cout<<"META "<<points.size()<<' '<<nodes.size()<<' '<<front<<' '<<front_mass<<' '
      <<open<<' '<<open_mass<<' '<<visits<<' '<<traced<<' '<<trace_forms<<' '
      <<selected.size()<<' '<<sampled_mass<<' '<<ns(t0,t1)<<' '<<ns(t1,t2)<<' '
      <<ns(t2,t3)<<' '<<ns(t3,t4)<<'\n';
  for(int s=0;s<6;++s)std::cout<<"STRATUM "<<s<<' '<<populations[s]<<' '<<product_mass[s]
      <<' '<<sample[s].size()<<'\n';
  for(const auto& r:selected) {
    const auto x=measure(*idx,r);const auto& a=nodes[r.an].range,&b=nodes[r.bn].range;
    const bool close3=!(r.mask&2)||x.pair3>=4,close4=!(r.mask&4)||x.pair4>=3;
    const bool singleclose3=!(r.mask&2)||x.single3>=4;
    const bool singleclose4=!(r.mask&4)||x.single4>=3;
    std::cout<<"ROW "<<stratum(r.product)<<' '<<r.ordinal<<' '<<r.product<<' '
      <<a.size()<<' '<<b.size()<<' '<<unsigned(r.mask)<<' '<<r.segment<<' '
      <<r.forms<<' '<<r.q3edges<<' '<<r.q4edges<<' '
      <<x.proposed<<' '<<x.search.pops<<' '<<x.search.boxes<<' '
      <<x.search.sites<<' '<<x.search.cuts<<' '<<x.pair_tests<<' '
      <<x.first_positive3<<' '<<x.first_positive4<<' '<<x.corners<<' '
      <<x.rect_positive3<<' '<<x.rect_positive4<<' '
      <<x.pair3<<' '<<x.pair4<<' '<<close3<<' '<<close4<<' '
      <<x.singleton_corner_tests<<' '<<x.single3<<' '<<x.single4<<' '
      <<singleclose3<<' '<<singleclose4<<' '
      <<x.select_ns<<' '<<x.pair_ns<<' '<<x.singleton_ns<<' '
      <<x.max3<<' '<<x.max4<<'\n';
  }
} catch(const std::exception& e) {std::cerr<<"probe: "<<e.what()<<'\n';return 1;}
