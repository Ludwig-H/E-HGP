// Audit of traversal orders, NOT a product port or a representative estimator.
// Same fixed-pair certificates in every order; each active lane starts at zero.
// Local vector stacks are intentional: this probe does not test compact state.
#include "snapshot/PAIR_BOUNDS.hpp"
#include "wspd/front.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace {
using namespace mhgp8;
void require(bool value, std::string_view message) {
  if (!value) throw std::runtime_error(std::string(message));
}
u64 number(std::string_view text) {
  u64 value{};
  const auto [end, error] = std::from_chars(text.data(), text.data()+text.size(), value);
  require(!text.empty() && error == std::errc{} && end == text.data()+text.size(), "unsigned integer required");
  return value;
}
void word(u64& hash, u64 value) {
  for (unsigned byte=0; byte<8; ++byte) {
    hash ^= value & 255U; hash *= 1099511628211ULL; value >>= 8;
  }
}
u64 mix(u64 value) { // SplitMix64 finalizer; arithmetic modulo 2^64.
  value ^= value >> 30; value *= 0xbf58476d1ce4e5b9ULL;
  value ^= value >> 27; value *= 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}
struct Input { std::vector<Point3> points; u64 hash{14695981039346656037ULL}; };
Input read_input(const char* path) {
  std::ifstream stream(path, std::ios::binary|std::ios::ate);
  require(stream.good(), "cannot open u16le input");
  const auto length=static_cast<std::streamoff>(stream.tellg());
  require(length>=12 && length%6==0, "u16le requires exactly 6*n bytes, n>=2");
  require(static_cast<std::uintmax_t>(length/6)<=std::numeric_limits<std::size_t>::max(), "input too large");
  const auto n=static_cast<std::size_t>(length/6);
  Input input; input.points.reserve(n); word(input.hash, n); stream.seekg(0);
  for (std::size_t id=0; id<n; ++id) {
    std::array<char,6> bytes{};
    require(static_cast<bool>(stream.read(bytes.data(),6)), "truncated u16le input");
    std::array<std::uint16_t,3> p{};
    for (std::size_t axis=0; axis<3; ++axis) {
      p[axis]=static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[2*axis]) |
             (static_cast<unsigned>(static_cast<unsigned char>(bytes[2*axis+1]))<<8));
      word(input.hash,p[axis]);
    }
    input.points.push_back({p[0],p[1],p[2]});
  }
  char extra{}; stream.read(&extra,1);
  require(stream.gcount()==0 && stream.eof() && !stream.bad(), "input changed or read failed");
  return input;
}
template<class Values> void numbers(const Values& values) {
  std::cout << '['; bool first=true;
  for (const auto value: values) { if (!first) std::cout << ','; first=false; std::cout << value; }
  std::cout << ']';
}
struct Sample { u64 hash{}; WspdRectangle rectangle{}; };
struct Less {
  u64* comparisons{};
  bool operator()(const Sample& a, const Sample& b) const {
    counter_add(*comparisons);
    return std::tie(a.hash,a.rectangle.a_node,a.rectangle.b_node) <
           std::tie(b.hash,b.rectangle.a_node,b.rectangle.b_node);
  }
};
i64 distance16(const std::array<i64,3>& center4, const Box3& box) {
  i64 result=0;
  for (std::size_t axis=0; axis<3; ++axis) {
    const i64 delta=std::max<i64>({0,4*static_cast<i64>(box.low[axis])-center4[axis],
                                    center4[axis]-4*static_cast<i64>(box.high[axis])});
    result += delta*delta;
  }
  return result;
}
struct Pivot { std::size_t rank{}, start{}; u64 steps{}, distance_tests{}; };
Pivot pivot(const Q2CensusIndex& index, WspdRectangle rectangle) {
  const auto nodes=index.spatial_nodes();
  const auto& a=nodes[rectangle.a_node].box; const auto& b=nodes[rectangle.b_node].box;
  std::array<i64,3> center4{};
  for (std::size_t axis=0; axis<3; ++axis)
    center4[axis]=static_cast<i64>(a.low[axis])+a.high[axis]+b.low[axis]+b.high[axis];
  Pivot result; std::size_t at=0;
  while (nodes[at].left!=Q2SpatialNode::absent) {
    const auto& node=nodes[at]; ++result.steps; result.distance_tests+=2;
    const bool left_first=distance16(center4,nodes[node.left].box)<=distance16(center4,nodes[node.right].box);
    at=left_first ? node.left : node.right;
    if (!left_first) result.start=at; // Highest node beginning at the final pivot rank.
  }
  result.rank=nodes[at].range.first; return result;
}
struct Work {
  u64 geom_nodes{},h_tests{},xi_tests{},structural_cuts{},distance_box_tests{};
  u64 positive_blocks{},negative_blocks{},leaf_nodes{},peak_stack{},order_rank_tests{},prepared_start_uses{};
};
static_assert(sizeof(Work)==11*sizeof(u64));
enum Order : unsigned { Global, Near, Circular, PivotPath };
struct Outcome { unsigned count{}; Work work{}; };
Outcome search(const Q2CensusIndex& index, const PreparedPairCitronBounds& bounds,
               Point3 a, Point3 b, unsigned q, unsigned threshold, std::size_t r,
               std::size_t prepared_start, Order order) {
  const auto nodes=index.spatial_nodes(); const auto n=index.spatial_order().size();
  Outcome out; auto& w=out.work; std::vector<std::size_t> stack; stack.reserve(49);
  const auto push=[&](std::size_t id) { stack.push_back(id); w.peak_stack=std::max(w.peak_stack,static_cast<u64>(stack.size())); };
  std::array<i64,3> center4{};
  for (std::size_t axis=0; axis<3; ++axis) center4[axis]=2*(static_cast<i64>(a[axis])+b[axis]);
  const unsigned phases=order==Circular ? 2 : 1;
  for (unsigned phase=0; phase<phases && out.count<threshold; ++phase) {
    const std::size_t lo=order==Circular && phase==0 ? r : 0;
    const std::size_t hi=order==Circular && phase==1 ? r : n;
    if (lo==hi) continue;
    std::size_t start=0;
    if (order==Circular && phase==0) {
      start=prepared_start; ++w.prepared_start_uses;
      require(nodes[start].range.first==r, "invalid prepared pivot start");
    }
    // Phase 0 starts at the highest node with first=r, then follows its
    // escape forest. Phase 1 clips the root BEFORE any geometric bound.
    std::size_t next=order==Circular && phase==0 ? nodes[start].escape : nodes.size();
    push(start);
    while ((!stack.empty() || next<nodes.size()) && out.count<threshold) {
      if (stack.empty()) { const auto at=next; next=nodes[at].escape; push(at); }
      const auto id=stack.back(); stack.pop_back(); const auto& node=nodes[id];
      const bool leaf=node.left==Q2SpatialNode::absent;
      if (node.range.first<lo || node.range.last>hi) {
        ++w.structural_cuts;
        if (node.range.last<=lo || node.range.first>=hi) continue;
        require(!leaf, "partial singleton rank range");
        push(node.right); push(node.left); continue;
      }
      ++w.geom_nodes; ++w.h_tests; w.leaf_nodes+=leaf ? 1U : 0U;
      const auto h=bounds.h_bounds(node.box);
      if (h.maximum4<=0) { ++w.negative_blocks; continue; }
      ++w.xi_tests; const auto xi=bounds.xi_bounds(node.box);
      const i128 alpha=6-q;
      if (alpha*static_cast<i128>(h.maximum4)*h.maximum4<=16*xi.low) {
        ++w.negative_blocks; continue;
      }
      if (h.minimum4>0 && alpha*static_cast<i128>(h.minimum4)*h.minimum4>16*xi.high) {
        ++w.positive_blocks;
        out.count+=static_cast<unsigned>(std::min<std::size_t>(threshold-out.count,node.range.size()));
        continue;
      }
      require(!leaf, "fixed-pair singleton bounds did not decide");
      bool left_first=true;
      if (order==Near) {
        w.distance_box_tests+=2;
        left_first=distance16(center4,nodes[node.left].box)<=distance16(center4,nodes[node.right].box);
      } else if (order==PivotPath) {
        ++w.order_rank_tests;
        if (node.range.first<=r && r<node.range.last) left_first=r<nodes[node.right].range.first;
      }
      push(left_first ? node.right : node.left); push(left_first ? node.left : node.right);
    }
    stack.clear();
  }
  return out;
}
std::size_t oracle(std::span<const Point3> points, Point3 a, Point3 b, unsigned q) {
  const auto distance=[](Point3 x,Point3 y) {
    i64 result=0;
    for (std::size_t axis=0; axis<3; ++axis) { const i64 d=static_cast<i64>(x[axis])-y[axis]; result+=d*d; }
    return result;
  };
  const i128 d=distance(a,b); std::size_t count=0;
  for (const auto z:points) {
    const i128 u=distance(a,z), v=distance(b,z), h2=d-u-v, projection2=d+u-v;
    count += h2>0 && static_cast<i128>(6-q)*h2*h2>4*d*u-projection2*projection2 ? 1U : 0U;
  }
  return count;
}
} // namespace

int main(int argc,char** argv) {
  try {
    require(argc==4,"usage: order_probe input.u16le K(2..10) samples_per_stratum");
    const auto k64=number(argv[2]), limit64=number(argv[3]);
    require(k64>=2 && k64<=10 && limit64>0 && limit64<=std::numeric_limits<std::size_t>::max(),"invalid K/sample count");
    const auto k=static_cast<unsigned>(k64); const auto limit=static_cast<std::size_t>(limit64);
    const auto input=read_input(argv[1]); const auto cloud=prepare_cloud(input.points);
    const auto index=make_q2_cloud_index(cloud); const auto nodes=index->spatial_nodes();
    const auto permutation=index->spatial_order(); u64 comparisons=0;
    using Heap=std::priority_queue<Sample,std::vector<Sample>,Less>;
    std::array<Heap,2> heaps{Heap(Less{&comparisons}),Heap(Less{&comparisons})};
    std::array<u64,2> populations{},masses{};
    const auto front=run_wspd_front(*index,k,8,WspdFrontMode::MidpointSamples,[&](const WspdRectangle& rect) {
      const auto as=nodes[rect.a_node].range.size(), bs=nodes[rect.b_node].range.size();
      require(as<=std::numeric_limits<u64>::max()/bs,"pair mass overflow");
      const unsigned stratum=as==1 && bs==1 ? 0 : 1;
      counter_add(populations[stratum]); counter_add(masses[stratum],static_cast<u64>(as)*bs);
      Sample sample{mix(static_cast<u64>(rect.a_node)^mix(static_cast<u64>(rect.b_node)+0x9e3779b97f4a7c15ULL)),rect};
      auto& heap=heaps[stratum];
      if (heap.size()<limit) heap.push(sample);
      else if (Less{&comparisons}(sample,heap.top())) { heap.pop(); heap.push(sample); }
    },6);
    std::array<std::vector<Sample>,2> samples;
    for (unsigned stratum=0; stratum<2; ++stratum) {
      while (!heaps[stratum].empty()) { samples[stratum].push_back(heaps[stratum].top()); heaps[stratum].pop(); }
      std::sort(samples[stratum].begin(),samples[stratum].end(),Less{&comparisons});
    }
    static_assert(sizeof(WspdFrontWork)==49*sizeof(u64));
    std::cout << "{\"kind\":\"sampled_fixed_pair_orders\",\"n\":" << input.points.size()
              << ",\"K\":" << k << ",\"input_fnv_words\":" << input.hash
              << ",\"samples_per_stratum\":" << limit << ",\"population_rectangles\":";
    numbers(populations); std::cout << ",\"population_pair_mass\":"; numbers(masses);
    std::cout << ",\"sampling_comparisons\":" << comparisons << ",\"front_words\":";
    numbers(std::bit_cast<std::array<u64,49>>(front.work));
    std::cout << ",\"front_total_pairs\":" << front.total_unordered_pairs
              << ",\"orders\":[\"global\",\"near\",\"circular\",\"pivotpath\"]"
              << ",\"work_fields\":[\"geom_nodes\",\"H\",\"Xi\",\"structural_cut\",\"distance_box_tests\",\"positive_blocks\",\"negative_blocks\",\"leaf\",\"peak_stack\",\"order_rank_tests\",\"prepared_start_uses\"]"
              << ",\"queries\":[";
    bool comma=false; u64 pair_preparations=0,pivot_steps=0,pivot_tests=0,oracle_tests=0,queries=0;
    for (unsigned stratum=0; stratum<2; ++stratum) for (const auto& sample:samples[stratum]) {
      const auto rect=sample.rectangle; const auto a=nodes[rect.a_node].range, b=nodes[rect.b_node].range;
      const auto p=pivot(*index,rect); pivot_steps+=p.steps; pivot_tests+=p.distance_tests;
      std::array<std::array<std::size_t,2>,4> pairs{{{a.first,b.first},{a.last-1,b.last-1},
          {a.first+mix(sample.hash)%a.size(),b.first+mix(sample.hash+1)%b.size()},
          {a.first+mix(sample.hash+2)%a.size(),b.first+mix(sample.hash+3)%b.size()}}};
      std::sort(pairs.begin(),pairs.end()); const auto end=std::unique(pairs.begin(),pairs.end());
      for (auto it=pairs.begin(); it!=end; ++it) {
        const auto ia=permutation[(*it)[0]], ib=permutation[(*it)[1]];
        require(ia!=ib,"overlapping sampled factors");
        const auto pa=input.points[ia], pb=input.points[ib]; PreparedPairCitronBounds bounds(pa,pb); ++pair_preparations;
        for (unsigned q=3; q<=4; ++q) {
          if ((rect.lane_mask & (1U<<(q-2)))==0) continue;
          const unsigned threshold=k+2-q; require(threshold>0,"inactive sampled lane");
          const auto exact=oracle(input.points,pa,pb,q); oracle_tests+=input.points.size(); ++queries;
          if (comma) std::cout << ',';
          comma=true;
          std::cout << "{\"stratum\":" << stratum << ",\"rect\":[" << rect.a_node << ',' << rect.b_node
                    << "],\"mask\":" << static_cast<unsigned>(rect.lane_mask) << ",\"ranks\":[" << (*it)[0] << ',' << (*it)[1]
                    << "],\"ids\":[" << ia << ',' << ib << "],\"q\":" << q << ",\"pivot_rank\":" << p.rank
                    << ",\"pivot_steps\":" << p.steps << ",\"pivot_distance_tests\":" << p.distance_tests
                    << ",\"oracle_count\":" << exact << ",\"alive\":" << (exact<threshold ? "true" : "false") << ",\"work\":[";
          for (unsigned order=0; order<4; ++order) {
            const auto out=search(*index,bounds,pa,pb,q,threshold,p.rank,p.start,static_cast<Order>(order));
            require(out.count==std::min<std::size_t>(exact,threshold),"order differs from full point oracle");
            if (order!=0) std::cout << ',';
            numbers(std::bit_cast<std::array<u64,11>>(out.work));
          }
          std::cout << "]}";
        }
      }
    }
    std::cout << "],\"sampled_rectangles\":[" << samples[0].size() << ',' << samples[1].size()
              << "],\"pair_preparations\":" << pair_preparations << ",\"pivot_preparation_steps\":" << pivot_steps
              << ",\"pivot_preparation_distance_tests\":" << pivot_tests << ",\"oracle_point_tests\":" << oracle_tests
              << ",\"query_count\":" << queries << ",\"status\":\"PASS\"}\n";
    return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
