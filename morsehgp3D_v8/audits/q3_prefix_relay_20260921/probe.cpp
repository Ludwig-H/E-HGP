// Independent relay-contract experiment. The included source is byte-identical
// to product32, d1b4dbc6. It supplies PreparedPower and the reference census.
// Link normally against the pinned archive, never with --whole-archive.
// Prefixes below are computed by exhaustive scans FOR THIS TEST ONLY; this is
// not an implementation or performance measurement of shared seed filtering.
#include "snapshot/q3_ball_census.cpp"

#include <bit>
#include <charconv>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace audit_relay {
using namespace mhgp8;
void require(bool condition,std::string_view message) {
  if (!condition) throw std::runtime_error(std::string(message));
}
u64 number(std::string_view text) {
  u64 value{};
  const auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),value);
  require(!text.empty() && error==std::errc{} && end==text.data()+text.size(),"unsigned decimal required");
  return value;
}
void word(u64& hash,u64 value) {
  for (unsigned byte=0;byte<8;++byte) { hash^=value&255U; hash*=1099511628211ULL; value>>=8; }
}
u64 mix(u64 value) {
  value^=value>>30; value*=0xbf58476d1ce4e5b9ULL;
  value^=value>>27; value*=0x94d049bb133111ebULL;
  return value^(value>>31);
}
std::string decimal(i128 value) {
  if (value==0) return "0";
  const bool negative=value<0;
  if (!negative) value=-value;
  std::string result;
  while (value!=0) { result.push_back(static_cast<char>('0'-static_cast<int>(value%10))); value/=10; }
  if (negative) result.push_back('-');
  std::reverse(result.begin(),result.end()); return result;
}
template<class Values> void numbers(const Values& values) {
  std::cout<<'['; bool comma=false;
  for (const auto value:values) { if (comma) std::cout<<','; comma=true; std::cout<<value; }
  std::cout<<']';
}
struct Input { std::vector<Point3> points; u64 hash{14695981039346656037ULL}; };
Input read_input(const char* path) {
  std::ifstream stream(path,std::ios::binary|std::ios::ate);
  require(stream.good(),"cannot open u16le input");
  const auto length=static_cast<std::streamoff>(stream.tellg());
  require(length>=12 && length%6==0,"u16le requires exactly 6*n bytes, n>=2");
  require(static_cast<std::uintmax_t>(length/6)<=std::numeric_limits<std::size_t>::max(),"input too large");
  const auto n=static_cast<std::size_t>(length/6);
  Input input; input.points.reserve(n); word(input.hash,n); stream.seekg(0);
  for (std::size_t id=0;id<n;++id) {
    std::array<char,6> bytes{};
    require(static_cast<bool>(stream.read(bytes.data(),6)),"truncated u16le input");
    std::array<std::uint16_t,3> p{};
    for (std::size_t axis=0;axis<3;++axis) {
      p[axis]=static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[2*axis]) |
        (static_cast<unsigned>(static_cast<unsigned char>(bytes[2*axis+1]))<<8));
      word(input.hash,p[axis]);
    }
    input.points.push_back({p[0],p[1],p[2]});
  }
  char extra{}; stream.read(&extra,1);
  require(stream.gcount()==0 && stream.eof() && !stream.bad(),"input changed or read failed");
  return input;
}
i64 distance(Point3 a,Point3 b) {
  i64 result=0;
  for (std::size_t axis=0;axis<3;++axis) { const i64 d=static_cast<i64>(a[axis])-b[axis]; result+=d*d; }
  return result;
}
struct Selection { u64 point_tests{},acute_seeds{},owner_edge_tests{},owned_seeds{}; };
std::vector<std::size_t> select_seeds(std::span<const Point3> points,std::size_t a,std::size_t b,
                                    std::size_t limit,Selection& work) {
  std::vector<std::size_t> selected;
  std::priority_queue<std::pair<u64,std::size_t>> heap;
  const auto edge=std::pair{a,b}; const i64 d=distance(points[a],points[b]);
  for (std::size_t x=0;x<points.size();++x) {
    if (x==a || x==b) continue;
    counter_add(work.point_tests);
    const i64 ax=distance(points[a],points[x]),bx=distance(points[b],points[x]);
    if (d+ax<=bx || d+bx<=ax || ax+bx<=d) continue;
    counter_add(work.acute_seeds); counter_add(work.owner_edge_tests,2);
    const std::pair<std::size_t,std::size_t> ae=std::minmax(a,x),be=std::minmax(b,x);
    if (ax>d || (ax==d && ae<edge) || bx>d || (bx==d && be<edge)) continue;
    counter_add(work.owned_seeds);
    if (limit==0) selected.push_back(x);
    else {
      const auto item=std::pair{mix(static_cast<u64>(x)+1),x};
      if (heap.size()<limit) heap.push(item);
      else if (item<heap.top()) { heap.pop(); heap.push(item); }
    }
  }
  while (!heap.empty()) { selected.push_back(heap.top().second); heap.pop(); }
  std::sort(selected.begin(),selected.end()); return selected;
}
struct Cut { std::string_view name; std::size_t cursor{},rank{}; };
std::array<Cut,5> cuts(const Q2CensusIndex& index) {
  const auto nodes=index.spatial_nodes(); const auto n=index.spatial_order().size();
  std::array<Cut,5> result{{{"root",0,0},{"EOF",nodes.size(),n},
                          {"q25",0,n/4},{"q50",0,n/2},{"q75",0,3*(n/4)+3*(n%4)/4}}};
  for (std::size_t i=2;i<result.size();++i) {
    std::size_t at=0;
    while (nodes[at].left!=Q2SpatialNode::absent) {
      const auto right=nodes[at].right;
      at=result[i].rank>=nodes[right].range.first ? right : nodes[at].left;
    }
    require(nodes[at].range.first==result[i].rank,"cut leaf rank mismatch");
    result[i].cursor=at;
  }
  return result;
}
struct Extra { u64 forest_roots{},count_global_root_visits{},shell_global_roots{},prefix_saturated{}; };
Q3BallCensusResult relay(const Q2CensusIndex& index,const ExactBall& ball,std::size_t threshold,
                         std::size_t incoming,std::size_t cursor,std::vector<std::size_t>& shell,
                         Q3BallCensusWork& work,Extra& extra) {
  const auto nodes=index.spatial_nodes();
  require(threshold>0 && !nodes.empty() && cursor<=nodes.size(),"invalid relay state");
  const auto prefix_rank=cursor==nodes.size() ? index.spatial_order().size() : nodes[cursor].range.first;
  shell.clear(); counter_add(work.queries);
  if (incoming>=threshold) {
    counter_add(extra.prefix_saturated); counter_add(work.count_saturations); counter_add(work.rejected_queries);
    return {false,threshold}; // No preparation, extension or shell collection.
  }
  const mhgp8::PreparedPower prepared(ball);
  counter_add(work.preparations); counter_add(work.vertex_axes,3);
  std::array<mhgp8::Frame,mhgp8::stack_capacity> stack{};
  work.stack_storage_bytes=std::max(work.stack_storage_bytes,static_cast<u64>(sizeof(stack)));
  std::size_t size=0,depth=incoming;
  const auto push=[&](mhgp8::Frame frame,u64& peak) {
    require(size<stack.size(),"relay exceeded proven u16 DFS depth");
    stack[size++]=frame; peak=std::max(peak,static_cast<u64>(size));
  };
  const auto prepare=[&](std::size_t id,bool payload) {
    const auto& node=nodes[id];
    if (payload) {
      counter_add(work.shell_bounds_prepared);
      counter_add(node.left==Q2SpatialNode::absent ? work.shell_point_tests : work.shell_box_bound_tests);
    } else {
      require(node.range.first>=prefix_rank,"relay revisited a consumed population prefix");
      counter_add(work.count_bounds_prepared);
      counter_add(node.left==Q2SpatialNode::absent ? work.count_point_tests : work.count_box_bound_tests);
    }
    return mhgp8::Frame{id,prepared.bounds(node)};
  };
  // Only the successor forest is global DFS. Each of its roots uses the
  // unchanged reference ordering by child power minimum; no global restart.
  for (std::size_t root=cursor;root<nodes.size() && depth<threshold;root=nodes[root].escape) {
    require(nodes[root].escape>root && nodes[root].escape<=nodes.size(),"invalid escape successor");
    require(size==0,"previous suffix subtree not exhausted");
    counter_add(extra.forest_roots); push(prepare(root,false),work.peak_count_stack);
    while (size!=0 && depth<threshold) {
      const auto frame=stack[--size]; const auto& node=nodes[frame.node];
      counter_add(work.count_node_visits);
      if (frame.node==0) counter_add(extra.count_global_root_visits);
      if (frame.bounds.minimum>=0) {
        counter_add(work.count_nonnegative_nodes);
        counter_add(work.count_nonnegative_sites,static_cast<u64>(node.range.size())); continue;
      }
      if (frame.bounds.maximum<0) {
        counter_add(work.count_inside_nodes);
        const auto credit=std::min(threshold-depth,node.range.size()); depth+=credit;
        counter_add(work.count_inside_sites,static_cast<u64>(credit)); continue;
      }
      require(node.left!=Q2SpatialNode::absent,"unclassified relay singleton");
      counter_add(work.count_split_nodes);
      const auto left=prepare(node.left,false),right=prepare(node.right,false);
      const bool left_first=left.bounds.minimum<=right.bounds.minimum;
      push(left_first ? right : left,work.peak_count_stack);
      push(left_first ? left : right,work.peak_count_stack);
    }
  }
  counter_add(work.count_prepared_unvisited,static_cast<u64>(size));
  if (depth==threshold) {
    counter_add(work.count_saturations); counter_add(work.rejected_queries); return {false,threshold};
  }
  require(size==0,"accepted suffix left pending nodes");
  // Depth exclusions contain contacts: collect the full shell independently.
  const auto order=index.spatial_order();
  counter_add(extra.shell_global_roots); push(prepare(0,true),work.peak_shell_stack);
  while (size!=0) {
    const auto frame=stack[--size]; const auto& node=nodes[frame.node];
    counter_add(work.shell_node_visits);
    if (frame.bounds.minimum>0 || frame.bounds.maximum<0) {
      counter_add(work.shell_excluded_nodes); continue;
    }
    if (node.left==Q2SpatialNode::absent) {
      shell.push_back(order[node.range.first]); counter_add(work.shell_ids); continue;
    }
    counter_add(work.shell_split_nodes);
    push(prepare(node.right,true),work.peak_shell_stack); push(prepare(node.left,true),work.peak_shell_stack);
  }
  counter_add(work.accepted_queries); return {true,depth};
}
u64 sort_shell(std::vector<std::size_t>& shell) {
  u64 comparisons=0;
  std::sort(shell.begin(),shell.end(),[&](std::size_t a,std::size_t b) { counter_add(comparisons); return a<b; });
  require(std::adjacent_find(shell.begin(),shell.end())==shell.end(),"duplicate shell ID"); return comparisons;
}
void outcome(Q3BallCensusResult result,const std::vector<std::size_t>& shell,const Q3BallCensusWork& work,u64 comparisons) {
  static_assert(std::is_trivially_copyable_v<Q3BallCensusWork> && sizeof(Q3BallCensusWork)==26*sizeof(u64));
  std::cout<<"\"accepted\":"<<(result.accepted ? "true" : "false")<<",\"depth\":"<<result.depth<<",\"shell\":";
  numbers(shell); std::cout<<",\"work\":"; numbers(std::bit_cast<std::array<u64,26>>(work));
  std::cout<<",\"sort_comparisons\":"<<comparisons;
}
} // namespace audit_relay

int main(int argc,char** argv) {
  using namespace audit_relay;
  try {
    require(argc==5 || argc==6,"usage: probe input.u16le K(2..10) a b [sample_limit; 0=all]");
    const auto k=number(argv[2]),av=number(argv[3]),bv=number(argv[4]);
    const auto lv=argc==6 ? number(argv[5]) : 0;
    require(k>=2 && k<=10 && lv<=std::numeric_limits<std::size_t>::max(),"invalid K/limit");
    const auto input=read_input(argv[1]);
    require(av<input.points.size() && bv<input.points.size() && av!=bv,"invalid edge IDs");
    const auto a=static_cast<std::size_t>(std::min(av,bv)),b=static_cast<std::size_t>(std::max(av,bv));
    const auto limit=static_cast<std::size_t>(lv),threshold=static_cast<std::size_t>(k-1);
    const auto cloud=prepare_cloud(input.points); const auto index=make_q2_cloud_index(cloud);
    const auto cut_list=cuts(*index); Selection selection;
    const auto seeds=select_seeds(input.points,a,b,limit,selection);
    std::cout<<"{\"kind\":\"q3_prefix_relay_contract\",\"n\":"<<input.points.size()<<",\"K\":"<<k
             <<",\"edge\":["<<a<<','<<b<<"],\"input_fnv_words\":"<<input.hash<<",\"sample_limit\":"<<limit
             <<",\"spatial_order\":";
    numbers(index->spatial_order()); std::cout<<",\"index_nodes\":"<<index->spatial_nodes().size()<<",\"cuts\":[";
    for (std::size_t i=0;i<cut_list.size();++i) {
      if (i!=0) std::cout<<',';
      const auto cut=cut_list[i]; std::cout<<"{\"name\":\""<<cut.name<<"\",\"cursor\":"<<cut.cursor<<",\"rank\":"<<cut.rank<<'}';
    }
    std::cout<<"],\"selection\":{\"point_tests\":"<<selection.point_tests<<",\"acute_seeds\":"<<selection.acute_seeds
             <<",\"owner_edge_tests\":"<<selection.owner_edge_tests<<",\"owned_seeds\":"<<selection.owned_seeds
             <<",\"selected\":"<<seeds.size()<<"},\"records\":[";
    bool comma=false;
    for (const auto x:seeds) {
      const auto ball=ExactBall::make_q3({input.points[a],input.points[b],input.points[x]});
      require(ball.has_value(),"selected seed is not a positive q3 support");
      std::array<std::size_t,3> support{a,b,x}; std::sort(support.begin(),support.end());
      Q3BallCensusWork reference_work{}; std::vector<std::size_t> reference_shell;
      const auto reference=census_q3_ball(*index,*ball,threshold,reference_shell,reference_work);
      const auto reference_sort=sort_shell(reference_shell);
      if (comma) std::cout<<',';
      comma=true; std::cout<<"{\"x\":"<<x<<",\"support\":"; numbers(support); std::cout<<",\"coefficients\":[";
      for (std::size_t i=0;i<5;++i) { if (i!=0) std::cout<<','; std::cout<<'"'<<decimal(ball->coefficients()[i])<<'"'; }
      std::cout<<"],\"reference\":{"; outcome(reference,reference_shell,reference_work,reference_sort); std::cout<<"},\"relays\":[";
      for (std::size_t i=0;i<cut_list.size();++i) {
        const auto cut=cut_list[i]; std::size_t prefix_exact=0;
        // Oracle preparation only: exhaustive over the consumed population prefix.
        for (std::size_t rank=0;rank<cut.rank;++rank)
          if (ball->power(input.points[index->spatial_order()[rank]])<0) ++prefix_exact;
        const auto incoming=std::min(prefix_exact,threshold);
        Q3BallCensusWork work{}; Extra extra{}; std::vector<std::size_t> shell;
        const auto result=relay(*index,*ball,threshold,incoming,cut.cursor,shell,work,extra);
        const auto comparisons=sort_shell(shell);
        require(result==reference && shell==reference_shell,"relay changed depth/complete shell");
        require(incoming+work.count_inside_sites==result.depth,"relay credit ledger mismatch");
        require(cut.cursor==0 || extra.count_global_root_visits==0,"count restarted at global root");
        if (cut.name=="root") require(work==reference_work,"root relay differs from reference work");
        if (i!=0) std::cout<<',';
        std::cout<<"{\"cut\":\""<<cut.name<<"\",\"cursor\":"<<cut.cursor<<",\"rank\":"<<cut.rank
                 <<",\"incoming_count\":"<<incoming<<",\"prefix_exact_count\":"<<prefix_exact
                 <<",\"prefix_point_tests\":"<<cut.rank<<',';
        outcome(result,shell,work,comparisons);
        std::cout<<",\"forest_roots\":"<<extra.forest_roots<<",\"count_global_root_visits\":"<<extra.count_global_root_visits
                 <<",\"shell_global_roots\":"<<extra.shell_global_roots<<",\"prefix_saturated\":"<<extra.prefix_saturated<<'}';
      }
      std::cout<<"]}";
    }
    std::cout<<"],\"status\":\"PASS\"}\n"; return 0;
  } catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
