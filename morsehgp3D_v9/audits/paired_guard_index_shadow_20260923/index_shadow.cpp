// Audit-only exact top-B quadrant search on the immutable product spatial index.
#include "pipeline/q2_census.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace mhgp9::gen;
using Clock = std::chrono::steady_clock;
using V3 = std::array<i64, 3>;

static void check(bool okay, const char* message) {
  if (!okay) throw std::runtime_error(message);
}
static std::uint64_t ns(Clock::time_point first, Clock::time_point last) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(last - first).count());
}
static V3 vec(Point3 p) { return {p.x, p.y, p.z}; }
static i64 dot(V3 a, V3 b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
static i64 square(V3 a) { return dot(a, a); }
static V3 minus(V3 a, V3 b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }

struct Edge {
  std::uint32_t seed{}, a{}, b{}, F{}, mask{};
  std::array<int,3> q3{}, q4{}, closed{};
};
static std::vector<Edge> load_edges(const std::string& path) {
  std::ifstream in(path);
  check(bool(in), "cannot open sample table");
  std::string heading;
  std::getline(in, heading);
  check(heading == "seed a b F mask q3_4 q4_4 closed_4 q3_8 q4_8 closed_8 q3_16 q4_16 closed_16", "sample schema");
  std::vector<Edge> edges;
  Edge e;
  while (in >> e.seed >> e.a >> e.b >> e.F >> e.mask
            >> e.q3[0] >> e.q4[0] >> e.closed[0]
            >> e.q3[1] >> e.q4[1] >> e.closed[1]
            >> e.q3[2] >> e.q4[2] >> e.closed[2]) edges.push_back(e);
  check(in.eof() && edges.size()==120, "sample rows");
  return edges;
}

static std::vector<std::uint32_t> read_u32(const std::string& path) {
  static_assert(std::endian::native == std::endian::little);
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  check(bool(in), "cannot open binary input");
  const auto bytes = in.tellg();
  check(bytes >= 0 && bytes % 4 == 0, "u32 input length");
  std::vector<std::uint32_t> out(static_cast<std::size_t>(bytes)/4);
  in.seekg(0);
  in.read(reinterpret_cast<char*>(out.data()), bytes);
  check(bool(in), "u32 input truncated");
  return out;
}

struct Candidate { std::size_t site{}; i64 q{}; std::uint32_t raw{}; };
static bool less_candidate(const Candidate& a, const Candidate& b) {
  return std::tie(a.q,a.raw) < std::tie(b.q,b.raw);
}
static void add_candidate(std::vector<Candidate>& bucket, Candidate item, std::size_t budget) {
  auto at = std::lower_bound(bucket.begin(), bucket.end(), item, less_candidate);
  if (bucket.size() == budget && at == bucket.end()) return;
  bucket.insert(at, item);
  if (bucket.size() > budget) bucket.pop_back();
}

struct Geometry {
  V3 d{}, s{}, A{}, B{};
  i64 D{};
  Geometry(Point3 pa, Point3 pb) {
    const auto a=vec(pa), b=vec(pb);
    d=minus(b,a);
    s={a[0]+b[0],a[1]+b[1],a[2]+b[2]};
    D=square(d);
    check(D>0, "zero edge");
    int main=0;
    for(int i=1;i<3;++i) if(std::abs(d[i])>std::abs(d[main])) main=i;
    const int ia=(main+1)%3, ib=(main+2)%3;
    const i64 h=std::abs(d[main]), sign=d[main]>0 ? 1 : -1;
    A[ia]=h; A[main]=-sign*d[ia];
    B[ib]=h; B[main]=-sign*d[ib];
    check(dot(A,d)==0 && dot(B,d)==0, "basis perpendicularity");
  }
  V3 w(Point3 p) const { return {2*i64(p.x)-s[0],2*i64(p.y)-s[1],2*i64(p.z)-s[2]}; }
  int quadrant(V3 z) const { return (int(dot(z,A)>=0)<<1)|int(dot(z,B)>=0); }
};

struct Bounds { i64 qmin{}, umin{}, umax{}, vmin{}, vmax{}; };
static Bounds bounds(const Geometry& g, Box3 box) {
  Bounds out;
  for(int i=0;i<3;++i) {
    const i64 lo=2*i64(box.low[i])-g.s[i], hi=2*i64(box.high[i])-g.s[i];
    const i64 near=lo>0 ? lo : (hi<0 ? hi : 0);
    out.qmin+=near*near;
    const i64 u0=lo*g.A[i], u1=hi*g.A[i];
    const i64 v0=lo*g.B[i], v1=hi*g.B[i];
    out.umin+=std::min(u0,u1); out.umax+=std::max(u0,u1);
    out.vmin+=std::min(v0,v1); out.vmax+=std::max(v0,v1);
  }
  return out;
}
static bool can_hit(const Bounds& b, i64 D, int quadrant) {
  if(b.qmin>D) return false;
  if((quadrant&2) ? b.umax<0 : b.umin>=0) return false;
  if((quadrant&1) ? b.vmax<0 : b.vmin>=0) return false;
  return true;
}

struct QueryWork {
  std::uint64_t pops{}, boxes{}, box_rejects{}, leaves{}, sites{}, pushes{},
      frontier_cut{}, queue_peak{};
  QueryWork& operator+=(const QueryWork& x) {
    pops+=x.pops; boxes+=x.boxes; box_rejects+=x.box_rejects;
    leaves+=x.leaves; sites+=x.sites; pushes+=x.pushes;
    frontier_cut+=x.frontier_cut; queue_peak=std::max(queue_peak,x.queue_peak);
    return *this;
  }
};
struct QueueItem { i64 qmin{}; std::size_t node{}; Bounds bound{}; };
struct QueueGreater {
  bool operator()(QueueItem a,QueueItem b) const {
    return std::tie(a.qmin,a.node)>std::tie(b.qmin,b.node);
  }
};
static std::vector<Candidate> query(const Q2CensusIndex& index,
    const std::vector<std::uint32_t>& raw_ids, const Geometry& g,
    std::size_t aid, std::size_t bid, int quadrant, std::size_t budget,
    QueryWork& work, std::size_t max_pops=0, bool* budget_hit=nullptr) {
  const auto nodes=index.spatial_nodes();
  const auto order=index.spatial_order();
  const auto points=index.cloud().points();
  std::priority_queue<QueueItem,std::vector<QueueItem>,QueueGreater> queue;
  auto enqueue=[&](std::size_t id, const std::vector<Candidate>& best) {
    ++work.boxes;
    const Bounds bound=bounds(g,nodes[id].box);
    if(!can_hit(bound,g.D,quadrant) ||
       (best.size()==budget && !best.empty() && bound.qmin>best.back().q)) {
      ++work.box_rejects; return;
    }
    queue.push({bound.qmin,id,bound}); ++work.pushes;
    work.queue_peak=std::max(work.queue_peak,std::uint64_t(queue.size()));
  };
  std::vector<Candidate> best;
  enqueue(0,best);
  while(!queue.empty()) {
    if(best.size()==budget && !best.empty() && queue.top().qmin>best.back().q) {
      work.frontier_cut+=queue.size(); break;
    }
    if(max_pops && work.pops>=max_pops) {
      if(budget_hit) *budget_hit=true;
      break;
    }
    const auto top=queue.top(); queue.pop(); ++work.pops;
    const auto& node=nodes[top.node];
    if(node.left==Q2SpatialNode::absent) {
      ++work.leaves;
      check(node.right==Q2SpatialNode::absent && node.range.size()==1, "non-singleton leaf");
      const std::size_t site=order[node.range.first];
      ++work.sites;
      if(site==aid || site==bid) continue;
      const V3 w=g.w(points[site]);
      const i64 q=square(w);
      if(q<=g.D && g.quadrant(w)==quadrant) add_candidate(best,{site,q,raw_ids[site]},budget);
    } else {
      check(node.right!=Q2SpatialNode::absent, "unpaired children");
      enqueue(node.left,best); enqueue(node.right,best);
    }
  }
  return best;
}

// One exact B=16 traversal carries all four heaps. Its sorted prefixes are
// precisely the top B=4/8 palettes, so the three requests need not be repeated.
static std::array<std::vector<Candidate>,4> query_all(
    const Q2CensusIndex& index, const std::vector<std::uint32_t>& raw_ids,
    const Geometry& g, std::size_t aid, std::size_t bid, QueryWork& work) {
  const auto nodes=index.spatial_nodes();
  const auto order=index.spatial_order();
  const auto points=index.cloud().points();
  std::array<std::vector<Candidate>,4> best;
  std::priority_queue<QueueItem,std::vector<QueueItem>,QueueGreater> queue;
  auto useful=[&](const Bounds& b) {
    for(int q=0;q<4;++q)
      if(can_hit(b,g.D,q) && (best[q].size()<16 || b.qmin<=best[q].back().q))
        return true;
    return false;
  };
  auto enqueue=[&](std::size_t id) {
    ++work.boxes;
    const Bounds b=bounds(g,nodes[id].box);
    if(!useful(b)) { ++work.box_rejects; return; }
    queue.push({b.qmin,id,b}); ++work.pushes;
    work.queue_peak=std::max(work.queue_peak,std::uint64_t(queue.size()));
  };
  enqueue(0);
  while(!queue.empty()) {
    bool full=true;
    i64 largest_worst=0;
    for(const auto& bucket:best) {
      if(bucket.size()<16) { full=false; break; }
      largest_worst=std::max(largest_worst,bucket.back().q);
    }
    if(full && queue.top().qmin>largest_worst) {
      work.frontier_cut+=queue.size(); break;
    }
    const auto top=queue.top(); queue.pop(); ++work.pops;
    if(!useful(top.bound)) { ++work.frontier_cut; continue; }
    const auto& node=nodes[top.node];
    if(node.left==Q2SpatialNode::absent) {
      ++work.leaves;
      check(node.right==Q2SpatialNode::absent && node.range.size()==1,"shared non-singleton leaf");
      const std::size_t site=order[node.range.first]; ++work.sites;
      if(site==aid || site==bid) continue;
      const V3 w=g.w(points[site]);
      const i64 q=square(w);
      if(q<=g.D) add_candidate(best[g.quadrant(w)],{site,q,raw_ids[site]},16);
    } else {
      enqueue(node.left); enqueue(node.right);
    }
  }
  return best;
}

static std::array<std::vector<Candidate>,4> oracle(
    std::span<const Point3> points, const std::vector<std::uint32_t>& raw,
    const Geometry& g, std::size_t aid, std::size_t bid, std::uint32_t& F) {
  std::array<std::vector<Candidate>,4> bins;
  F=2;
  for(std::size_t i=0;i<points.size();++i) {
    if(i==aid || i==bid) continue;
    const V3 w=g.w(points[i]);
    const i64 q=square(w);
    if(q>g.D) continue;
    ++F;
    add_candidate(bins[g.quadrant(w)],{i,q,raw[i]},16);
  }
  return bins;
}

struct PairEdge {
  i128 margin{}; std::uint32_t lo{},hi{}; std::size_t i{},j{};
};
static int greedy(const Geometry& g, std::span<const Point3> points,
    const std::vector<std::uint32_t>& raw, const std::vector<Candidate>& chosen,
    int lane) {
  std::vector<PairEdge> edges;
  for(std::size_t i=0;i<chosen.size();++i) for(std::size_t j=i+1;j<chosen.size();++j) {
    const V3 wg=g.w(points[chosen[i].site]), wh=g.w(points[chosen[j].site]);
    const i64 H=2*g.D-square(wg)-square(wh);
    if(H<=0) continue;
    const V3 t={wg[0]+wh[0],wg[1]+wh[1],wg[2]+wh[2]};
    i128 X=0;
    for(int k=0;k<3;++k) {
      const i64 c=g.d[(k+1)%3]*t[(k+2)%3]-g.d[(k+2)%3]*t[(k+1)%3];
      X+=i128(c)*c;
    }
    const i128 margin=lane==3 ? 3*i128(H)*H-4*X : i128(H)*H-2*X;
    if(margin<=0) continue;
    edges.push_back({margin,std::min(raw[chosen[i].site],raw[chosen[j].site]),
                     std::max(raw[chosen[i].site],raw[chosen[j].site]),i,j});
  }
  std::sort(edges.begin(),edges.end(),[](const PairEdge& x,const PairEdge& y) {
    if(x.margin!=y.margin) return x.margin>y.margin;
    return std::tie(x.lo,x.hi)<std::tie(y.lo,y.hi);
  });
  std::vector<bool> used(chosen.size());
  int count=0;
  for(const auto& e:edges) if(!used[e.i] && !used[e.j]) {
    used[e.i]=used[e.j]=true; ++count;
  }
  return count;
}

static void selftest() {
  std::vector<Point3> points={{10,10,10},{30,10,10}};
  for(int dx=-2;dx<=2;++dx) for(int dy=-1;dy<=1;++dy)
    for(int dz=-1;dz<=1;++dz) points.push_back({20+dx,10+dy,10+dz});
  std::vector<std::uint32_t> raw(points.size());
  for(std::size_t i=0;i<raw.size();++i) raw[i]=1000-static_cast<std::uint32_t>(i);
  const auto cloud=prepare_cloud(points);
  const auto index=make_q2_cloud_index(cloud);
  const Geometry g(points[0],points[1]);
  std::uint32_t F=0;
  const auto expected=oracle(cloud->points(),raw,g,0,1,F);
  check(F==points.size(),"fixture core");
  QueryWork shared_work;
  const auto shared=query_all(*index,raw,g,0,1,shared_work);
  for(int q=0;q<4;++q) {
    check(expected[q].size()==shared[q].size(),"fixture shared length");
    for(std::size_t i=0;i<shared[q].size();++i)
      check(expected[q][i].site==shared[q][i].site,"fixture shared tie or boundary");
    for(std::size_t B : {std::size_t(4),std::size_t(8),std::size_t(16)}) {
      QueryWork work;
      const auto selected=query(*index,raw,g,0,1,q,B,work);
      check(selected.size()==std::min(B,expected[q].size()),"fixture budget length");
      for(std::size_t i=0;i<selected.size();++i)
        check(selected[i].site==expected[q][i].site,"fixture budget tie or boundary");
    }
  }
  check(g.quadrant(g.w(Point3{20,10,10}))==3,"zero signs must be nonnegative");
  std::cout << "SELFTEST 47 sites, 4 sign quadrants, 3 budgets, raw-ID ties: PASS\n";
}

int main(int argc,char** argv) {
  try {
    if(argc==2 && std::string(argv[1])=="--selftest") { selftest(); return 0; }
    check(argc==4 || (argc==5 &&
          (std::string(argv[4])=="--budget64" || std::string(argv[4])=="--budget128" ||
           std::string(argv[4])=="--budget256")),
          "usage: index_shadow points.u32le raw_ids.u32le SAMPLES.tsv [--budget64|128|256]");
    const std::size_t visit_cap=argc==4 ? 0 : std::stoul(std::string(argv[4]).substr(8));
    const auto xyz=read_u32(argv[1]), raw=read_u32(argv[2]);
    check(xyz.size()==3*raw.size() && raw.size()==123389,"cloud length");
    std::vector<Point3> points; points.reserve(raw.size());
    std::unordered_map<std::uint32_t,std::size_t> where; where.reserve(raw.size()*2);
    for(std::size_t i=0;i<raw.size();++i) {
      points.push_back({static_cast<Coordinate>(xyz[3*i]),static_cast<Coordinate>(xyz[3*i+1]),static_cast<Coordinate>(xyz[3*i+2])});
      check(where.emplace(raw[i],i).second,"duplicate raw ID");
    }
    const auto t0=Clock::now();
    const auto cloud=prepare_cloud(points);
    const auto t1=Clock::now();
    const auto index=make_q2_cloud_index(cloud);
    const auto t2=Clock::now();
    std::cout << "META " << raw.size() << ' ' << index->spatial_nodes().size() << ' '
              << ns(t0,t1) << ' ' << ns(t1,t2) << ' '
              << cloud->retained_bytes() << ' ' << index->retained_bytes() << ' '
              << index->work().point_visits << ' ' << index->work().max_depth << '\n';
    const auto edges=load_edges(argv[3]);
    constexpr std::array<std::size_t,3> budgets={4,8,16};
    for(const auto& e:edges) {
      const auto ia=where.at(e.a), ib=where.at(e.b);
      const Geometry g(points[ia],points[ib]);
      std::uint32_t F=0;
      const auto to0=Clock::now();
      const auto expected=oracle(cloud->points(),raw,g,ia,ib,F);
      const auto to1=Clock::now();
      check(F==e.F,"oracle F differs from S2 trace");
      std::cout << "ORACLE " << e.seed << ' ' << e.a << ' ' << e.b << ' ' << e.F << ' ' << ns(to0,to1) << '\n';
      if(visit_cap) {
        for(std::size_t k=0;k<budgets.size();++k) {
          const auto B=budgets[k]; QueryWork work;
          std::vector<Candidate> chosen;
          int hit=0;
          const auto tb0=Clock::now();
          for(int q=0;q<4;++q) {
            QueryWork one; bool one_hit=false;
            auto bin=query(*index,raw,g,ia,ib,q,B,one,visit_cap,&one_hit);
            hit+=int(one_hit); work+=one;
            chosen.insert(chosen.end(),bin.begin(),bin.end());
          }
          const auto tb1=Clock::now();
          const int q3=(e.mask&2) ? greedy(g,cloud->points(),raw,chosen,3) : 0;
          const int q4=(e.mask&4) ? greedy(g,cloud->points(),raw,chosen,4) : 0;
          const int closed=(!(e.mask&2) || q3>=4) && (!(e.mask&4) || q4>=3);
          const auto tb2=Clock::now();
          std::cout << "BUDGET " << e.seed << ' ' << e.a << ' ' << e.b << ' ' << e.F << ' '
                    << e.mask << ' ' << B << ' ' << chosen.size() << ' ' << hit << ' '
                    << q3 << ' ' << q4 << ' ' << closed << ' '
                    << ns(tb0,tb1) << ' ' << ns(tb1,tb2) << ' '
                    << work.pops << ' ' << work.boxes << ' ' << work.leaves << ' '
                    << work.sites << '\n';
        }
        continue;
      }
      QueryWork shared_work;
      const auto ts0=Clock::now();
      const auto shared=query_all(*index,raw,g,ia,ib,shared_work);
      const auto ts1=Clock::now();
      for(int q=0;q<4;++q) {
        check(shared[q].size()==expected[q].size(),"shared palette length mismatch");
        for(std::size_t i=0;i<shared[q].size();++i)
          check(shared[q][i].raw==expected[q][i].raw && shared[q][i].q==expected[q][i].q,
                "shared palette differs from full scan");
      }
      std::cout << "SHARED " << e.seed << ' ' << e.a << ' ' << e.b << ' ' << e.F << ' '
                << ns(ts0,ts1) << ' ' << shared_work.pops << ' ' << shared_work.boxes << ' '
                << shared_work.box_rejects << ' ' << shared_work.leaves << ' '
                << shared_work.sites << ' ' << shared_work.pushes << ' '
                << shared_work.frontier_cut << ' ' << shared_work.queue_peak << '\n';
      for(std::size_t k=0;k<budgets.size();++k) {
        const auto B=budgets[k]; QueryWork work;
        std::vector<Candidate> chosen;
        const auto tq0=Clock::now();
        for(int q=0;q<4;++q) {
          QueryWork one;
          auto bin=query(*index,raw,g,ia,ib,q,B,one);
          work+=one;
          const std::size_t size=std::min(B,expected[q].size());
          check(bin.size()==size,"indexed palette length mismatch");
          for(std::size_t i=0;i<size;++i)
            check(bin[i].raw==expected[q][i].raw && bin[i].q==expected[q][i].q,
                  "indexed palette differs from full scan");
          chosen.insert(chosen.end(),bin.begin(),bin.end());
        }
        const auto tq1=Clock::now();
        const int q3=(e.mask&2) ? greedy(g,cloud->points(),raw,chosen,3) : 0;
        const int q4=(e.mask&4) ? greedy(g,cloud->points(),raw,chosen,4) : 0;
        const int closed=(!(e.mask&2) || q3>=4) && (!(e.mask&4) || q4>=3);
        const auto tm1=Clock::now();
        check(q3==e.q3[k] && q4==e.q4[k] && closed==e.closed[k],"matching differs from published oracle");
        std::cout << "ROW " << e.seed << ' ' << e.a << ' ' << e.b << ' ' << e.F << ' '
                  << e.mask << ' ' << B << ' ' << chosen.size() << ' '
                  << q3 << ' ' << q4 << ' ' << closed << ' '
                  << ns(tq0,tq1) << ' ' << ns(tq1,tm1) << ' '
                  << work.pops << ' ' << work.boxes << ' ' << work.box_rejects << ' '
                  << work.leaves << ' ' << work.sites << ' ' << work.pushes << ' '
                  << work.frontier_cut << ' ' << work.queue_peak << '\n';
      }
    }
  } catch(const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << '\n'; return 1;
  }
}
