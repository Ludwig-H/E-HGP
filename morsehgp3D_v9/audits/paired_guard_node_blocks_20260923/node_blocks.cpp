// Audit-only bounded block-pair search on the shared native spatial index.
#include "pipeline/q2_census.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdlib>
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
using Vec = std::array<i64, 3>;
using Clock = std::chrono::steady_clock;

namespace {
void check(bool okay, const char* message) {
  if (!okay) throw std::runtime_error(message);
}
std::uint64_t nanoseconds(Clock::time_point a, Clock::time_point b) {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count());
}
std::vector<std::uint32_t> read_u32(const std::string& path) {
  static_assert(std::endian::native == std::endian::little);
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  check(bool(in), "input open");
  const auto bytes = in.tellg();
  check(bytes >= 0 && bytes % 4 == 0, "input byte count");
  std::vector<std::uint32_t> out(static_cast<std::size_t>(bytes)/4);
  in.seekg(0);
  in.read(reinterpret_cast<char*>(out.data()), bytes);
  check(bool(in), "input read");
  return out;
}
struct Edge {
  std::uint32_t seed{}, a{}, b{}, F{}, mask{};
  std::array<int, 3> oracle{};
};
std::vector<Edge> read_edges(const std::string& path) {
  std::ifstream in(path);
  check(bool(in), "sample open");
  std::string heading;
  std::getline(in, heading);
  check(heading == "seed a b F mask q3_4 q4_4 closed_4 q3_8 q4_8 closed_8 q3_16 q4_16 closed_16",
        "sample heading");
  std::vector<Edge> out;
  Edge e;
  int q3{}, q4{};
  while (in >> e.seed >> e.a >> e.b >> e.F >> e.mask
            >> q3 >> q4 >> e.oracle[0]
            >> q3 >> q4 >> e.oracle[1]
            >> q3 >> q4 >> e.oracle[2]) out.push_back(e);
  check(in.eof() && out.size() == 120, "sample count");
  return out;
}
Vec as_vec(Point3 p) { return {p.x, p.y, p.z}; }
i64 dot(Vec a, Vec b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
i64 norm2(Vec a) { return dot(a,a); }
struct Geometry {
  Vec d{}, s{}, u{}, v{};
  i64 D{};
  Geometry(Point3 a, Point3 b) {
    const auto x=as_vec(a), y=as_vec(b);
    for (int i=0;i<3;++i) { d[i]=y[i]-x[i]; s[i]=y[i]+x[i]; }
    D=norm2(d);
    check(D>0, "zero edge");
    int main=0;
    for (int i=1;i<3;++i) if (std::abs(d[i])>std::abs(d[main])) main=i;
    const int ia=(main+1)%3, ib=(main+2)%3;
    const i64 h=std::abs(d[main]), sign=d[main]>0 ? 1 : -1;
    u[ia]=h; u[main]=-sign*d[ia];
    v[ib]=h; v[main]=-sign*d[ib];
    check(dot(u,d)==0 && dot(v,d)==0, "perpendicular basis");
  }
  Vec w(Point3 p) const { return {2*i64(p.x)-s[0],2*i64(p.y)-s[1],2*i64(p.z)-s[2]}; }
  int quadrant(Vec w0) const { return (int(dot(w0,u)>=0)<<1)|int(dot(w0,v)>=0); }
};
struct Bounds {
  i64 qmin{},qmax{},umin{},umax{},vmin{},vmax{};
};
Bounds bounds(const Geometry& g, Box3 box) {
  Bounds out;
  for (int i=0;i<3;++i) {
    const i64 lo=2*i64(box.low[i])-g.s[i], hi=2*i64(box.high[i])-g.s[i];
    const i64 near=lo>0 ? lo : (hi<0 ? hi : 0);
    out.qmin+=near*near;
    out.qmax+=std::max(lo*lo,hi*hi);
    const i64 ua=lo*g.u[i], ub=hi*g.u[i], va=lo*g.v[i], vb=hi*g.v[i];
    out.umin+=std::min(ua,ub); out.umax+=std::max(ua,ub);
    out.vmin+=std::min(va,vb); out.vmax+=std::max(va,vb);
  }
  return out;
}
bool can_hit(Bounds b, i64 D, int q) {
  if (b.qmin>D) return false;
  if ((q&2) ? b.umax<0 : b.umin>=0) return false;
  if ((q&1) ? b.vmax<0 : b.vmin>=0) return false;
  return true;
}
bool contained(Bounds b, int q) {
  if ((q&2) ? b.umin<0 : b.umax>=0) return false;
  if ((q&1) ? b.vmin<0 : b.vmax>=0) return false;
  return true;
}
struct Candidate { std::size_t node{}; std::size_t pop{}; };
struct SearchWork {
  std::uint64_t pops{}, boxes{}, box_rejects{}, endpoint_splits{},
      candidates{}, pair_tests{}, pair_positive3{}, pair_positive4{}, exhausted{};
};
struct Queued { i64 qmin{}; std::size_t id{}; };
struct QueueGreater {
  bool operator()(Queued a, Queued b) const {
    return std::tie(a.qmin,a.id)>std::tie(b.qmin,b.id);
  }
};
std::vector<Candidate> query(const Q2CensusIndex& index, const Geometry& g,
    std::size_t rank_a, std::size_t rank_b, int quadrant,
    std::size_t cap, std::size_t budget, SearchWork& work) {
  const auto nodes=index.spatial_nodes();
  std::priority_queue<Queued,std::vector<Queued>,QueueGreater> todo;
  auto push=[&](std::size_t id) {
    ++work.boxes;
    const auto b=bounds(g,nodes[id].box);
    if (!can_hit(b,g.D,quadrant)) { ++work.box_rejects; return; }
    todo.push({b.qmin,id});
  };
  push(0);
  std::vector<Candidate> found;
  std::size_t visits=0;
  while (!todo.empty() && found.size()<16 && visits<budget) {
    const auto top=todo.top(); todo.pop(); ++work.pops;
    ++visits;
    const auto& node=nodes[top.id];
    const auto b=bounds(g,node.box);
    const bool has_end=(node.range.first<=rank_a && rank_a<node.range.last) ||
                       (node.range.first<=rank_b && rank_b<node.range.last);
    if (!has_end && node.range.size()<=cap && b.qmax<=g.D && contained(b,quadrant)) {
      found.push_back({top.id,node.range.size()}); ++work.candidates;
    } else if (node.left!=Q2SpatialNode::absent) {
      if (has_end) ++work.endpoint_splits;
      push(node.left); push(node.right);
    }
  }
  if (!todo.empty() && found.size()<16) ++work.exhausted;
  return found;
}
struct Certificate { bool q3{},q4{}; i64 Hmin{}; i128 Xmax{},margin3{},margin4{}; };
i64 interval_product_min(i64 a, i64 b, i64 c) { return std::min(a*c,b*c); }
i64 interval_product_max(i64 a, i64 b, i64 c) { return std::max(a*c,b*c); }
Certificate certificate(const Geometry& g, Box3 G, Box3 H) {
  std::array<i64,3> gl{},gh{},hl{},hh{},tl{},th{};
  i64 Qg=0,Qh=0;
  for (int i=0;i<3;++i) {
    gl[i]=2*i64(G.low[i])-g.s[i]; gh[i]=2*i64(G.high[i])-g.s[i];
    hl[i]=2*i64(H.low[i])-g.s[i]; hh[i]=2*i64(H.high[i])-g.s[i];
    Qg+=std::max(gl[i]*gl[i],gh[i]*gh[i]);
    Qh+=std::max(hl[i]*hl[i],hh[i]*hh[i]);
    tl[i]=gl[i]+hl[i]; th[i]=gh[i]+hh[i];
  }
  Certificate out;
  out.Hmin=2*g.D-Qg-Qh;
  if (out.Hmin<=0) return out;
  for (int i=0;i<3;++i) {
    const int j=(i+1)%3,k=(i+2)%3;
    const i64 cl=interval_product_min(tl[k],th[k],g.d[j])-
                 interval_product_max(tl[j],th[j],g.d[k]);
    const i64 ch=interval_product_max(tl[k],th[k],g.d[j])-
                 interval_product_min(tl[j],th[j],g.d[k]);
    const i128 l=cl,r=ch;
    out.Xmax+=std::max(l*l,r*r);
  }
  out.margin3=3*i128(out.Hmin)*out.Hmin-4*out.Xmax;
  out.margin4=i128(out.Hmin)*out.Hmin-2*out.Xmax;
  out.q3=out.margin3>0;
  out.q4=out.margin4>0;
  return out;
}
struct Pair { std::size_t i{},j{},cap{}; i128 margin{}; Certificate cert; };
struct Choice { std::size_t credit{}; std::vector<Pair> pairs; };
Choice choose(std::vector<Pair> pairs, std::size_t count, std::size_t threshold) {
  std::sort(pairs.begin(),pairs.end(),[](const Pair& a,const Pair& b) {
    if (a.cap!=b.cap) return a.cap>b.cap;
    if (a.margin!=b.margin) return a.margin>b.margin;
    return std::tie(a.i,a.j)<std::tie(b.i,b.j);
  });
  std::vector<bool> used(count);
  Choice out;
  for (const auto& pair:pairs) {
    if (used[pair.i] || used[pair.j]) continue;
    used[pair.i]=used[pair.j]=true;
    out.credit+=pair.cap;
    out.pairs.push_back(pair);
    if (out.credit>=threshold) break;
  }
  return out;
}
void emit_block(const Q2CensusIndex& index, const std::vector<std::uint32_t>& raw,
                Candidate c) {
  const auto nodes=index.spatial_nodes();
  const auto order=index.spatial_order();
  const auto points=index.cloud().points();
  const auto& n=nodes[c.node];
  std::cout << c.node << ':' << c.pop << ':';
  for (int i=0;i<3;++i) std::cout << n.box.low[i] << ',';
  for (int i=0;i<3;++i) std::cout << n.box.high[i] << (i==2 ? ':' : ',');
  for (std::size_t r=n.range.first;r<n.range.last;++r) {
    if (r!=n.range.first) std::cout << ',';
    const auto site=order[r];
    const auto p=points[site];
    std::cout << raw[site] << '/' << p.x << '/' << p.y << '/' << p.z;
  }
}
} // namespace

int main(int argc, char** argv) {
  try {
    check(argc==4, "usage: node_blocks points.u32le raw_ids.u32le SAMPLES.tsv");
    const auto xyz=read_u32(argv[1]), raw=read_u32(argv[2]);
    check(xyz.size()==3*raw.size() && raw.size()==123389, "cloud length");
    std::vector<Point3> points; points.reserve(raw.size());
    std::unordered_map<std::uint32_t,std::size_t> where; where.reserve(raw.size()*2);
    for (std::size_t i=0;i<raw.size();++i) {
      points.push_back({static_cast<Coordinate>(xyz[3*i]),
                        static_cast<Coordinate>(xyz[3*i+1]),
                        static_cast<Coordinate>(xyz[3*i+2])});
      check(where.emplace(raw[i],i).second, "duplicate raw ID");
    }
    const auto sample=read_edges(argv[3]);
    const auto prep0=Clock::now();
    const auto cloud=prepare_cloud(points);
    const auto index=make_q2_cloud_index(cloud);
    const auto prep1=Clock::now();
    std::vector<std::size_t> rank(raw.size());
    const auto order=index->spatial_order();
    const auto nodes=index->spatial_nodes();
    for (std::size_t r=0;r<order.size();++r) rank[order[r]]=r;
    std::cout << "META " << raw.size() << ' ' << nodes.size() << ' '
              << nanoseconds(prep0,prep1) << ' ' << index->retained_bytes() << '\n';
    for (std::size_t cap : {std::size_t(1),std::size_t(2),std::size_t(4),std::size_t(8)}) {
      for (std::size_t budget : {std::size_t(64),std::size_t(256),std::size_t(1024),std::size_t(4096)}) {
        for (const auto& e:sample) {
          const auto aid=where.at(e.a), bid=where.at(e.b);
          const Geometry g(points[aid],points[bid]);
          // Independent core size check, outside the timed shadow search.
          std::size_t F=0;
          for (Point3 p:points) if (norm2(g.w(p))<=g.D) ++F;
          check(F==e.F, "core size mismatch");
          SearchWork work;
          const auto start=Clock::now();
          std::vector<Candidate> candidates;
          for (int q=0;q<4;++q) {
            // The node-visit budget applies separately to each quadrant.
            const auto before=work.pops;
            auto part=query(*index,g,rank[aid],rank[bid],q,cap,budget,work);
            check(work.pops-before<=budget, "query budget");
            candidates.insert(candidates.end(),part.begin(),part.end());
          }
          std::vector<Pair> pairs3,pairs4;
          for (std::size_t i=0;i<candidates.size();++i) {
            for (std::size_t j=i+1;j<candidates.size();++j) {
              ++work.pair_tests;
              const auto cert=certificate(g,nodes[candidates[i].node].box,
                                           nodes[candidates[j].node].box);
              const auto pair=Pair{i,j,std::min(candidates[i].pop,candidates[j].pop),{},cert};
              if (cert.q3) { ++work.pair_positive3; auto x=pair; x.margin=cert.margin3; pairs3.push_back(x); }
              if (cert.q4) { ++work.pair_positive4; auto x=pair; x.margin=cert.margin4; pairs4.push_back(x); }
            }
          }
          const auto ch3=(e.mask&2) ? choose(std::move(pairs3),candidates.size(),4) : Choice{};
          const auto ch4=(e.mask&4) ? choose(std::move(pairs4),candidates.size(),3) : Choice{};
          const bool closed3=!(e.mask&2) || ch3.credit>=4;
          const bool closed4=!(e.mask&4) || ch4.credit>=3;
          const bool closed=closed3 && closed4;
          const auto end=Clock::now();
          // Audit assertion outside the measured search/matching interval.
          for (std::size_t i=0;i<candidates.size();++i) {
            const auto& x=nodes[candidates[i].node];
            for (std::size_t j=i+1;j<candidates.size();++j) {
              const auto& y=nodes[candidates[j].node];
              check(x.range.last<=y.range.first || y.range.last<=x.range.first,
                    "candidate ranges overlap");
            }
          }
          std::cout << "ROW " << cap << ' ' << budget << ' ' << e.seed << ' ' << e.a << ' ' << e.b
                    << ' ' << e.F << ' ' << e.mask << ' ' << e.oracle[2]
                    << ' ' << ch3.credit << ' ' << ch4.credit << ' ' << closed
                    << ' ' << work.pops << ' ' << work.boxes << ' ' << work.box_rejects
                    << ' ' << work.endpoint_splits << ' ' << work.candidates
                    << ' ' << work.pair_tests << ' ' << work.pair_positive3
                    << ' ' << work.pair_positive4 << ' ' << work.exhausted
                    << ' ' << nanoseconds(start,end) << '\n';
          if (closed && (budget==4096 || (budget==64 && (cap==4 || cap==8)))) {
            for (int lane : {3,4}) {
              if (!(e.mask & (lane==3 ? 2 : 4))) continue;
              const auto& choice=lane==3 ? ch3 : ch4;
              for (const auto& pair:choice.pairs) {
                std::cout << "PROOF " << cap << ' ' << budget << ' ' << e.seed << ' ' << e.a << ' ' << e.b
                          << ' ' << lane << ' ' << pair.cap << ' ' << pair.cert.Hmin << ' ';
                // Xmax is <2^80; print it in decimal without narrowing.
                auto x=pair.cert.Xmax;
                std::string digits;
                do { digits.push_back(char('0'+int(x%10))); x/=10; } while (x);
                std::reverse(digits.begin(),digits.end());
                std::cout << digits << ' ';
                emit_block(*index,raw,candidates[pair.i]); std::cout << ' ';
                emit_block(*index,raw,candidates[pair.j]); std::cout << '\n';
              }
            }
          }
        }
      }
    }
  } catch (const std::exception& ex) {
    std::cerr << "node_blocks: " << ex.what() << '\n';
    return 1;
  }
}
