// Audit-only white-box scheduler. Include the production implementation ONCE
// to reuse its anonymous Front::expand without copying any geometry. Do not
// also compile src/gen/wspd/front.cpp into this executable.
#include "../../src/gen/wspd/front.cpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <tuple>

namespace audit {
using namespace mhgp9::gen;

void need(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
double cpu_seconds() {
  timespec t{};
  need(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t) == 0, "clock_thread");
  return static_cast<double>(t.tv_sec) + static_cast<double>(t.tv_nsec) * 1e-9;
}
using Clock = std::chrono::steady_clock;
double wall_seconds(Clock::time_point t) { return std::chrono::duration<double>(Clock::now()-t).count(); }

struct WaveWork {
  u64 waves{}, tasks{}, children{}, emitted{}, chunks{}, max_width{}, max_depth{};
  u64 task_capacity_bytes{}, scratch_bytes{}, rectangle_capacity_bytes{};
};
struct Answer { WspdFrontResult result; std::vector<WspdRectangle> rectangles; WaveWork work; };
struct Expansion {
  std::array<Task, 3> children{};
  WspdRectangle terminal{};
  std::uint8_t count{}, terminals{};
};

// Two owned frontier vectors, constant-sized (<= chunk_size) expansion
// packets, per-packet prefix counts, and the output. The chunk size controls
// temporary storage only; EVERY chunk of EVERY wave is consumed.
Answer waves(Q2CensusIndexPtr index, unsigned k, unsigned s, WspdFrontMode mode,
             std::uint8_t mask, bool reverse, const std::string& mutant) {
  constexpr std::size_t chunk_size = 1024;
  validate_front(k, s, mode, mask, {});
  need(static_cast<bool>(index), "missing_owner");
  const WspdRectangleConsumer unused = [](const WspdRectangle&) {};
  Front front(*index, k, s, mode, unused, mask, {});
  Answer answer;
  std::vector<Task> current{front.root_task()}, next;
  std::vector<Expansion> expansions(chunk_size);
  std::vector<std::size_t> child_first(chunk_size+1), terminal_first(chunk_size+1);
  answer.work.scratch_bytes = expansions.capacity()*sizeof(Expansion)+
      (child_first.capacity()+terminal_first.capacity())*sizeof(std::size_t);
  bool mutated = false;
  while (!current.empty()) {
    ++answer.work.waves;
    answer.work.max_width = std::max(answer.work.max_width, static_cast<u64>(current.size()));
    if (reverse) std::reverse(current.begin(), current.end());
    next.clear();
    for (std::size_t begin = 0; begin < current.size(); begin += chunk_size) {
      const std::size_t count = std::min(chunk_size, current.size()-begin);
      child_first[0] = terminal_first[0] = 0;
      ++answer.work.chunks;
      for (std::size_t i = 0; i < count; ++i) {
        const Task& task = current[begin+i];
        need(!task.terminal && task.depth+1 == answer.work.waves, "cause=waves.depth_or_terminal");
        need(task.witnesses.count == 0, "cause=waves.unexpected_q2_credits");
        auto& e = expansions[i]; e.count = e.terminals = 0;
        ++answer.work.tasks;
        answer.work.max_depth = std::max(answer.work.max_depth, task.depth);
        front.expand(task, [&](const Task& child) {
          need(e.count < e.children.size(), "cause=waves.fanout");
          need(!child.terminal && child.depth == task.depth+1 && (child.mask & ~task.mask) == 0,
               "cause=waves.child_contract");
          e.children[e.count++] = child;
        }, [&](const Task& terminal) {
          need(e.terminals == 0 && terminal.terminal && terminal.depth == task.depth,
               "cause=waves.emission_contract");
          e.terminal = {terminal.a, terminal.b, terminal.mask}; ++e.terminals;
        });
        need(e.count == 0 || e.terminals == 0, "cause=waves.emission_and_children");
        if (!mutated && e.count != 0 && !mutant.empty()) {
          if (mutant == "drop") --e.count;
          else if (mutant == "depth") ++e.children[0].depth;
          else if (mutant == "pending") e.children[0].dfs_pending += 1000;
          else if (mutant == "mask") e.children[0].mask = 0;
          else throw std::runtime_error("unknown_mutant");
          mutated = true;
        }
        // expand pushes in reverse canonical DFS order. Reverse within each
        // packet to make the normal wave traversal a canonical breadth-first.
        std::reverse(e.children.begin(), e.children.begin()+e.count);
        child_first[i+1] = child_first[i] + e.count;
        terminal_first[i+1] = terminal_first[i] + e.terminals;
      }
      const std::size_t child_base = next.size(), terminal_base = answer.rectangles.size();
      need(child_first[count] <= next.max_size()-child_base, "wave_children_size_overflow");
      need(terminal_first[count] <= answer.rectangles.max_size()-terminal_base, "wave_terminals_size_overflow");
      next.resize(child_base+child_first[count]);
      answer.rectangles.resize(terminal_base+terminal_first[count]);
      for (std::size_t i = 0; i < count; ++i) {
        const auto& e = expansions[i];
        std::copy_n(e.children.begin(), e.count, next.begin()+static_cast<std::ptrdiff_t>(child_base+child_first[i]));
        if (e.terminals != 0) answer.rectangles[terminal_base+terminal_first[i]] = e.terminal;
      }
      answer.work.children += child_first[count];
      answer.work.emitted += terminal_first[count];
      answer.work.task_capacity_bytes = std::max(answer.work.task_capacity_bytes,
          static_cast<u64>((current.capacity()+next.capacity())*sizeof(Task)));
    }
    current.swap(next);
  }
  answer.work.rectangle_capacity_bytes = answer.rectangles.capacity()*sizeof(WspdRectangle);
  answer.result = front.result();
  need(answer.work.tasks == answer.work.children+1, "cause=waves.task_partition");
  need(answer.work.tasks == answer.result.work.product_visits, "cause=waves.visit_count");
  need(answer.work.emitted == answer.result.work.emitted_rectangles, "cause=waves.emission_count");
  need(mutant.empty() || mutated, "cause=waves.mutant_not_exercised");
  return answer;
}

bool less(const WspdRectangle& a, const WspdRectangle& b) {
  return std::tie(a.a_node,a.b_node,a.lane_mask)<std::tie(b.a_node,b.b_node,b.lane_mask);
}
bool equal(const WspdRectangle& a, const WspdRectangle& b) {
  return !less(a,b) && !less(b,a);
}

struct Tiles {
  u64 pairs{}, full_tiles{}, contained_tiles{}, crossing_tiles{}, max_rectangle_mass{};
  std::array<u64,6> rectangles_by_mass{};  // 1, 2..7, 8..31, 32..255, 256..4095, >=4096
};
Tiles tiles(const Q2CensusIndex& index, std::span<const WspdRectangle> rectangles) {
  Tiles t;
  const auto nodes=index.spatial_nodes();
  for (const auto& r:rectangles) {
    const u64 mass=product(nodes[r.a_node].range.size(),nodes[r.b_node].range.size());
    need(mass<=std::numeric_limits<u64>::max()-t.pairs,"tile_mass_overflow");
    const u64 first=t.pairs/32+(t.pairs%32!=0), last=(t.pairs+mass)/32;
    if (last>first) t.contained_tiles+=last-first;
    t.pairs+=mass; t.max_rectangle_mass=std::max(t.max_rectangle_mass,mass);
    const unsigned bin=mass==1?0:mass<8?1:mass<32?2:mass<256?3:mass<4096?4:5;
    ++t.rectangles_by_mass[bin];
  }
  t.full_tiles=t.pairs/32;
  need(t.contained_tiles<=t.full_tiles,"tile_partition");
  t.crossing_tiles=t.full_tiles-t.contained_tiles;
  return t;
}

void judge(const std::string& name, std::span<const Point3> points, unsigned k, unsigned s,
           WspdFrontMode mode, std::uint8_t mask, bool reverse, const std::string& mutant) {
  auto index = make_q2_cloud_index(prepare_cloud(points));
  std::vector<WspdRectangle> reference;
  const auto start = Clock::now(); const auto cpu = cpu_seconds();
  const auto expected = run_wspd_front(*index,k,s,mode,[&](const WspdRectangle& r){ reference.push_back(r); },mask);
  const double reference_cpu = cpu_seconds()-cpu, reference_wall = wall_seconds(start);
  const auto wave_start = Clock::now(); const auto wave_cpu_start = cpu_seconds();
  auto actual = waves(index,k,s,mode,mask,reverse,mutant);
  const double wave_cpu = cpu_seconds()-wave_cpu_start, wave_wall = wall_seconds(wave_start);
  need(expected.work == actual.result.work, "cause=waves.work_diff");
  need(expected.total_unordered_pairs == actual.result.total_unordered_pairs &&
       expected.active_lane_mask == actual.result.active_lane_mask, "cause=waves.metadata_diff");
  const auto reference_tiles=tiles(*index,reference), wave_tiles=tiles(*index,actual.rectangles);
  std::sort(reference.begin(),reference.end(),less);
  std::sort(actual.rectangles.begin(),actual.rectangles.end(),less);
  need(std::adjacent_find(reference.begin(),reference.end(),equal) == reference.end(), "cause=waves.reference_duplicate");
  need(reference.size() == actual.rectangles.size() &&
       std::equal(reference.begin(),reference.end(),actual.rectangles.begin(),equal), "cause=waves.rectangles_diff");
  const auto& w=actual.work;
  std::cout << "{\"schema\":\"mhgp9_front_waves_audit_v1\",\"case\":\""<<name<<"\",\"n\":"<<points.size()
    <<",\"K\":"<<k<<",\"s\":"<<s<<",\"mode\":\""<<(mode==WspdFrontMode::Pure?"pure":"midpoint")
    <<"\",\"mask\":"<<static_cast<unsigned>(mask)<<",\"reverse\":"<<(reverse?"true":"false")
    <<",\"equal_all_work\":true,\"equal_rectangles\":true,\"rectangles\":"<<reference.size()
    <<",\"tasks\":"<<w.tasks<<",\"children\":"<<w.children<<",\"waves\":"<<w.waves
    <<",\"max_width\":"<<w.max_width<<",\"max_depth\":"<<w.max_depth
    <<",\"task_capacity_bytes\":"<<w.task_capacity_bytes<<",\"scratch_bytes\":"<<w.scratch_bytes
    <<",\"rectangle_capacity_bytes\":"<<w.rectangle_capacity_bytes<<",\"chunks\":"<<w.chunks
    <<",\"reference_cpu_s\":"<<reference_cpu<<",\"reference_wall_s\":"<<reference_wall
    <<",\"waves_cpu_s\":"<<wave_cpu<<",\"waves_wall_s\":"<<wave_wall
    <<",\"h_bound_tests\":"<<expected.work.h_bound_tests<<",\"xi_bound_tests\":"<<expected.work.xi_bound_tests
    <<",\"witness_descent_steps\":"<<expected.work.witness_descent_steps
    <<",\"max_stack_size\":"<<expected.work.max_stack_size
    <<",\"raw_front_tile32\":{\"scope\":\"before_S2_rectangle_filter\",\"pairs\":"<<reference_tiles.pairs
    <<",\"full_tiles\":"<<reference_tiles.full_tiles<<",\"dfs_contained_tiles\":"<<reference_tiles.contained_tiles
    <<",\"dfs_crossing_tiles\":"<<reference_tiles.crossing_tiles<<",\"waves_contained_tiles\":"<<wave_tiles.contained_tiles
    <<",\"waves_crossing_tiles\":"<<wave_tiles.crossing_tiles<<",\"max_rectangle_mass\":"<<reference_tiles.max_rectangle_mass
    <<",\"rectangles_by_mass\":[";
  for(std::size_t i=0;i<reference_tiles.rectangles_by_mass.size();++i) {
    if(i!=0) std::cout<<',';
    std::cout<<reference_tiles.rectangles_by_mass[i];
  }
  std::cout<<"]}}\n";
}

std::vector<Point3> synthetic(const std::string& kind, std::size_t n) {
  std::mt19937 rng(94721);
  std::vector<Point3> p; p.reserve(n);
  for (std::size_t i=0;i<n;++i) {
    const auto x=static_cast<Coordinate>(i*7);  // unique and <=224000 for n<=32000
    if (kind=="uniform") p.push_back({x,static_cast<Coordinate>(rng()%262144),static_cast<Coordinate>(rng()%262144)});
    else if (kind=="terrain") p.push_back({x,static_cast<Coordinate>(rng()%262144),static_cast<Coordinate>(100000+rng()%1000)});
    else if (kind=="rows") p.push_back({static_cast<Coordinate>(i/2*7),static_cast<Coordinate>((i%2)*7000),0});
    else throw std::runtime_error("unknown_synthetic");
  }
  return p;
}

std::vector<Point3> read_frame(const std::string& path) {
  std::ifstream in(path,std::ios::binary); need(static_cast<bool>(in),"input_open");
  std::vector<Point3> points; std::array<unsigned char,12> bytes{};
  while (in.read(reinterpret_cast<char*>(bytes.data()),12)) {
    std::array<Coordinate,3> xyz{};
    for (std::size_t d=0;d<3;++d) {
      std::uint32_t x=0; for (unsigned j=0;j<4;++j) x|=static_cast<std::uint32_t>(bytes[4*d+j])<<(8*j);
      need(x<=262143,"input_coordinate"); xyz[d]=static_cast<Coordinate>(x);
    }
    points.push_back({xyz[0],xyz[1],xyz[2]});
  }
  need(in.eof() && in.gcount()==0,"input_partial_record"); return points;
}
}  // namespace audit

int main(int argc,char** argv) {
  try {
    using namespace audit;
    if (argc==2 && std::string(argv[1])=="--fixtures") {
      std::vector<std::pair<std::string,std::vector<Point3>>> fixtures{
        {"singleton",{{0,0,0}}},{"pair",{{0,0,0},{262143,262143,262143}}},
        {"contact",{{0,0,0},{2,0,0},{1,1,0},{1,0,0},{1,0,1},{1,0,2}}},
        {"extremes",{{0,0,0},{262143,0,0},{0,262143,0},{0,0,262143},{262143,262143,262143},{131071,131072,131071}}},
        {"uniform53",synthetic("uniform",53)},{"terrain53",synthetic("terrain",53)},{"rows54",synthetic("rows",54)}};
      for (const auto& [name,p]:fixtures) for (unsigned k:{2U,5U,10U}) for (unsigned s:{8U,10U,12U})
        for (auto mode:{WspdFrontMode::Pure,WspdFrontMode::MidpointSamples})
          for (std::uint8_t mask:{std::uint8_t{2},std::uint8_t{6}}) for (bool reverse:{false,true})
            judge(name,p,k,s,mode,mask,reverse,"");
      return 0;
    }
    if (argc==3 && std::string(argv[1])=="--mutant") {
      const auto p=synthetic("uniform",31);
      judge("mutant",p,5,8,WspdFrontMode::MidpointSamples,6,false,argv[2]); return 0;
    }
    if (argc==6 && std::string(argv[1])=="--synthetic") {
      const auto p=synthetic(argv[2],std::stoull(argv[3]));
      judge(argv[2],p,static_cast<unsigned>(std::stoul(argv[4])),static_cast<unsigned>(std::stoul(argv[5])),
          WspdFrontMode::MidpointSamples,6,false,""); return 0;
    }
    if (argc==5 && std::string(argv[1])=="--frame") {
      const auto p=read_frame(argv[2]);
      judge("frame_full_u18",p,static_cast<unsigned>(std::stoul(argv[3])),static_cast<unsigned>(std::stoul(argv[4])),
          WspdFrontMode::MidpointSamples,6,false,""); return 0;
    }
    std::cerr<<"usage: --fixtures | --mutant drop|depth|pending|mask | --synthetic kind n K s | --frame file K s\n"; return 2;
  } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
