// Read-only adapter of the native FULL result. No facet catalogue and no
// Chapter-9 voting masses are manufactured from dated point-set coverage.
#include "../../src/chain/tower_chain.hpp"
#include <charconv>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string_view>

namespace fixed_k {
using namespace mhgp9::tower;
using mhgp9::gen::Point3;
using Clock = std::chrono::steady_clock;
void need(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
double elapsed(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}
u64 integer(std::string_view text) {
  u64 n = 0;
  const auto r = std::from_chars(text.data(), text.data() + text.size(), n);
  need(r.ec == std::errc{} && r.ptr == text.data() + text.size(), "invalid_integer");
  return n;
}
std::vector<Point3> read_points(const char* path) {
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  need(bool(in), "input_open");
  const auto size = in.tellg();
  need(size > 0 && size % 12 == 0, "input_u32le_xyz_size");
  need(static_cast<u64>(size / 12) <= std::numeric_limits<PointId>::max(), "input_id_range");
  in.seekg(0);
  std::vector<Point3> points(static_cast<size_t>(size / 12));
  for (auto& p : points) {
    std::array<mhgp9::gen::Coordinate, 3> xyz{};
    for (auto& x : xyz) {
      unsigned char b[4]{};
      in.read(reinterpret_cast<char*>(b), 4);
      need(bool(in), "input_short_read");
      const u32 v = u32{b[0]} | (u32{b[1]} << 8) | (u32{b[2]} << 16) | (u32{b[3]} << 24);
      need(v <= static_cast<u32>(mhgp9::gen::coordinate_limit), "input_outside_u18");
      x = static_cast<mhgp9::gen::Coordinate>(v);
    }
    p = {xyz[0], xyz[1], xyz[2]};
  }
  return points;
}
std::vector<PointId> expand(const FullCoverageCertificate& f, const FullDatedContribution& c) {
  const auto& row = f.populations()->rows().at(c.ref.population);
  std::vector<PointId> ids;
  if (c.ref.include_interior) ids = row.interior;
  for (size_t j = 0; j < row.shell.size(); ++j)
    if (c.ref.shell_mask & (u16{1} << j)) ids.push_back(row.shell[j]);
  std::sort(ids.begin(), ids.end());
  need(std::adjacent_find(ids.begin(), ids.end()) == ids.end(), "contribution_duplicate_point");
  need(!ids.empty(), "empty_contribution");
  return ids;
}
struct Checks { u64 coverage_queries = 0, overlapping_live_root_pairs = 0, continuations = 0; };
Checks validate(const FullCoverageCertificate& f, size_t n, bool replay) {
  Checks checks;
  need(f.order() > 0 && f.populations() && f.populations()->domain().size() == n, "forest_domain");
  const auto& nodes = f.nodes(); const auto& next = f.successors();
  need(!nodes.empty() && next.size() == nodes.size(), "forest_shape");
  for (size_t i = 0; i < n; ++i) need(f.populations()->domain()[i] == i, "point_id_identity");
  std::vector<u64> incoming(nodes.size());
  for (size_t i = 0; i < nodes.size(); ++i) {
    const auto& node = nodes[i];
    need(node.level.den > 0 && node.parent_count != 1 && node.first <= f.parents().size() &&
         node.parent_count <= f.parents().size() - node.first, "node_shape");
    for (u64 j = 0; j < node.parent_count; ++j) {
      const auto child = f.parents()[node.first + j];
      need(child < i && next[child] == i && ++incoming[child] == 1, "forest_child_link");
      need(compare_exact_level(nodes[child].level, node.level) < 0, "strict_birth_merge_level");
    }
    need(next[i] == kFullCoverageAbsent || (next[i] > i && next[i] < nodes.size()), "forest_successor");
  }
  for (size_t i = 0; i < nodes.size(); ++i)
    need(incoming[i] == (next[i] == kFullCoverageAbsent ? 0U : 1U), "forest_complete_links");
  std::set<PointId> all;
  for (size_t i = 0; i < f.contributions().size(); ++i) {
    const auto& c = f.contributions()[i];
    need(c.segment < nodes.size() && c.ref.population < f.populations()->rows().size(), "contribution_reference");
    need(compare_exact_level(nodes[c.segment].level, c.level) <= 0, "contribution_before_birth");
    need(next[c.segment] == kFullCoverageAbsent || compare_exact_level(c.level, nodes[next[c.segment]].level) < 0,
         "contribution_after_merge");
    if (i) need(compare_exact_level(f.contributions()[i - 1].level, c.level) <= 0, "contribution_chronology");
    const auto& row = f.populations()->rows()[c.ref.population];
    need(!(c.ref.shell_mask & ~full_coverage_detail::all_shell(row)), "shell_mask_range");
    const auto ids = expand(f, c); all.insert(ids.begin(), ids.end());
    checks.continuations += compare_exact_level(nodes[c.segment].level, c.level) < 0;
  }
  need(all.size() == n && *all.begin() == 0 && *all.rbegin() == n - 1, "final_point_coverage");
  if (!replay) return checks;
  std::vector<ExactLevel> cuts;
  for (const auto& node : nodes) cuts.push_back(node.level);
  for (const auto& c : f.contributions()) cuts.push_back(c.level);
  std::sort(cuts.begin(), cuts.end(), [](const auto& a, const auto& b) { return compare_exact_level(a,b) < 0; });
  cuts.erase(std::unique(cuts.begin(), cuts.end(), same_exact_level), cuts.end());
  // Independent replay walks downward child CSR, rather than the reader's
  // upward successor normalization. Repeated contributions are SET UNION.
  for (const auto& cut : cuts) for (bool closed : {false, true}) {
    std::vector<std::vector<PointId>> live_sets;
    for (size_t root = 0; root < nodes.size(); ++root) {
      if (!full_coverage_detail::admitted(nodes[root].level, cut, closed) ||
          (next[root] != kFullCoverageAbsent && full_coverage_detail::admitted(nodes[next[root]].level, cut, closed))) continue;
      std::vector<bool> descendant(nodes.size()); std::vector<u64> stack{root};
      while (!stack.empty()) {
        const auto id = stack.back(); stack.pop_back(); descendant[id] = true;
        for (u64 j = 0; j < nodes[id].parent_count; ++j) stack.push_back(f.parents()[nodes[id].first + j]);
      }
      std::set<PointId> expected;
      for (const auto& c : f.contributions()) if (descendant[c.segment] && full_coverage_detail::admitted(c.level, cut, closed)) {
        const auto ids = expand(f,c); expected.insert(ids.begin(),ids.end());
      }
      const auto actual = full_coverage_at(f, root, cut, closed);
      need(actual.status == FullCertificateStatus::kOk && std::vector<PointId>(expected.begin(),expected.end()) == actual.values,
           "coverage_replay_mismatch");
      live_sets.push_back(actual.values); ++checks.coverage_queries;
    }
    for (size_t i = 0; i < live_sets.size(); ++i) for (size_t j = i + 1; j < live_sets.size(); ++j) {
      std::vector<PointId> common;
      std::set_intersection(live_sets[i].begin(),live_sets[i].end(),live_sets[j].begin(),live_sets[j].end(),std::back_inserter(common));
      checks.overlapping_live_root_pairs += !common.empty();
      if (f.order() == 1) need(common.empty(), "k1_live_cover_must_partition");
    }
  }
  return checks;
}
mhgp9::ChainResult run(std::span<const Point3> points, unsigned k, size_t workers) {
  mhgp9::ChainOptions options; options.kmax=k; options.workers=workers;
  options.catalogue_digest=true; options.keep_catalogue=false;
  auto result = mhgp9::run_tower_chain(points, options);
  // Move-only forest inside the result: return value is moved by the caller.
  need(result.status == mhgp9::ChainStatus::kComplete, result.reason.c_str());
  need(result.tower.status == FullBallStatus::kCompleteRelative && result.tower.orders.size() == k && result.kmax_effective == k,
       "complete_requested_tower");
  return result;
}
std::string decimal(std::array<u64,3> words) {
  std::string result;
  do {
    u128 remainder=0;
    for(size_t i=words.size();i-- >0;) {
      const u128 current=(remainder<<64)|words[i];
      words[i]=static_cast<u64>(current/10); remainder=current%10;
    }
    result.push_back(static_cast<char>('0'+static_cast<unsigned>(remainder)));
  } while(words[0] || words[1] || words[2]);
  std::reverse(result.begin(),result.end()); return result;
}
void level(const ExactLevel& l) {
  const u128 d = static_cast<u128>(l.den);
  std::cout << "{\"num\":\"" << decimal({l.num[0],l.num[1],l.num[2]}) << "\",\"den\":\""
    << decimal({static_cast<u64>(d),static_cast<u64>(d>>64),0}) << "\"}";
}
template<class Range> void array(const Range& values) {
  std::cout << '['; bool first=true;
  for (const auto value : values) { if (!first) std::cout << ','; first=false; std::cout << value; }
  std::cout << ']';
}
void output(std::span<const Point3> points, unsigned k, size_t workers, const mhgp9::ChainResult& result,
            const Checks& checks, bool replay, double wall, double validation_ms) {
  const auto& f=result.tower.orders[k-1].forest;
  std::map<u64,u64> population_ids;
  for (const auto& c:f.contributions()) population_ids.emplace(c.ref.population,0);
  u64 dense=0; for (auto& [native,local]:population_ids) { static_cast<void>(native); local=dense++; }
  std::cout << std::setprecision(17) << "{\"schema\":\"mhgp9_fixed_k_export_v1\",\"status\":\"completed\",\"point_count\":" << points.size()
    << ",\"k\":" << k << ",\"computed_orders\":" << k << ",\"workers\":" << workers
    << ",\"profile\":\"quantized_u18_input_only\",\"scope\":\"fixed_order_dated_point_cover_not_chapter9_vote_mass\""
    << ",\"GCP_used\":false,\"GPU_used\":false,\"vertical_maps_exported\":false,\"facet_catalogue_exported\":false"
    << ",\"tower_digest\":\"" << result.tower_digest << "\",\"catalogue_digest\":\"" << result.catalogue_digest
    << "\",\"presentation_digest\":\"" << result.presentation_digest << "\",\"points\":[";
  bool first=true; for (const auto p:points) { if(!first) std::cout<<','; first=false; std::cout<<'['<<p.x<<','<<p.y<<','<<p.z<<']'; }
  std::cout << "],\"nodes\":[";
  for (size_t i=0;i<f.nodes().size();++i) {
    if(i) std::cout<<',';
    const auto& node=f.nodes()[i];
    std::cout << "{\"id\":"<<i<<",\"level\":"; level(node.level); std::cout<<",\"children\":[";
    for(u64 j=0;j<node.parent_count;++j) { if(j) std::cout<<','; std::cout<<f.parents()[node.first+j]; }
    std::cout<<"],\"successor\":"; if(f.successors()[i]==kFullCoverageAbsent) std::cout<<"null"; else std::cout<<f.successors()[i];
    std::cout<<'}';
  }
  std::vector<u64> roots; for(size_t i=0;i<f.nodes().size();++i) if(f.successors()[i]==kFullCoverageAbsent) roots.push_back(i);
  std::cout<<"],\"roots\":"; array(roots); std::cout<<",\"populations\":[";
  first=true; for(const auto& [native,local]:population_ids) {
    static_cast<void>(local); if(!first) std::cout<<','; first=false;
    const auto& row=f.populations()->rows()[native]; std::cout<<"{\"interior\":"; array(row.interior);
    std::cout<<",\"shell\":"; array(row.shell); std::cout<<'}';
  }
  std::cout<<"],\"contributions\":["; first=true;
  for(const auto& c:f.contributions()) {
    if(!first) std::cout<<',';
    first=false; std::cout<<"{\"level\":"; level(c.level);
    std::cout<<",\"segment\":"<<c.segment<<",\"population\":"<<population_ids.at(c.ref.population)
      <<",\"shell_mask\":"<<c.ref.shell_mask<<",\"include_interior\":"<<(c.ref.include_interior?"true":"false")<<'}';
  }
  std::cout<<"],\"validation\":{\"structural\":true,\"final_point_coverage\":true,\"all_cut_coverage_replayed\":"<<(replay?"true":"false")
    <<",\"coverage_queries\":"<<checks.coverage_queries<<",\"overlapping_live_root_pairs\":"<<checks.overlapping_live_root_pairs
    <<",\"continuations\":"<<checks.continuations<<"},\"times_ms\":{\"native_chain_wall\":"<<wall
    <<",\"native_chain_reported\":"<<result.times.total_ms<<",\"native_tower\":"<<result.times.tower_ms
    <<",\"validation\":"<<validation_ms<<"}}\n";
}
void gate() {
  need(decimal({0,0,0})=="0" && decimal({~u64{0},~u64{0},~u64{0}})==
       "6277101735386680763835789423207666416102355444464034512895", "decimal_u192");
  const std::vector<std::vector<Point3>> fixtures{
    {{0,0,0},{2,0,0},{2,2,0},{0,2,0}},
    {{1,8,0},{5,10,0},{9,8,0},{5,0,0}},
    {{0,0,0},{4,0,0},{0,4,0},{0,0,4},{7,3,2},{9,8,5},{2,9,11}}};
  u64 cases=0,queries=0,overlap=0,continuations=0;
  for(const auto& points:fixtures) for(unsigned k=1;k<=std::min<size_t>(5,points.size());++k) {
    const auto result=run(points,k,1); const auto checks=validate(result.tower.orders[k-1].forest,points.size(),true);
    ++cases; queries+=checks.coverage_queries; overlap+=checks.overlapping_live_root_pairs; continuations+=checks.continuations;
  }
  need(overlap>0,"gate_shared_points_not_exercised");
  std::cout<<"{\"schema\":\"mhgp9_fixed_k_export_gate_v1\",\"status\":\"passed\",\"cases\":"<<cases
    <<",\"coverage_queries\":"<<queries<<",\"overlapping_live_root_pairs\":"<<overlap<<",\"continuations\":"<<continuations<<"}\n";
}
} // namespace fixed_k
int main(int argc,char** argv) {
  try {
    using namespace fixed_k;
    if(argc==2 && std::string_view(argv[1])=="--gate") { gate(); return 0; }
    const char* input=nullptr; unsigned k=0; size_t workers=1; bool replay=false;
    std::set<std::string_view> seen;
    for(int i=1;i<argc;++i) {
      const std::string_view arg=argv[i]; need(seen.insert(arg).second,"duplicate_argument");
      if(arg=="--verify-coverage") { replay=true; continue; }
      need(i+1<argc,"missing_argument_value"); const char* value=argv[++i];
      if(arg=="--input") input=value;
      else if(arg=="--k") { const auto x=integer(value); need(x>=1 && x<=10,"k_range_1_10"); k=static_cast<unsigned>(x); }
      else if(arg=="--workers") { const auto x=integer(value); need(x>=1 && x<=256,"workers_range"); workers=static_cast<size_t>(x); }
      else throw std::runtime_error("unknown_argument");
    }
    need(input && k,"usage_input_k"); const auto points=read_points(input); need(k<=points.size(),"k_exceeds_point_count");
    const auto start=Clock::now(); const auto result=run(points,k,workers); const auto wall=elapsed(start);
    const auto validation_start=Clock::now(); const auto checks=validate(result.tower.orders[k-1].forest,points.size(),replay);
    const auto validation_ms=elapsed(validation_start); output(points,k,workers,result,checks,replay,wall,validation_ms); return 0;
  } catch(const std::exception& e) { std::cerr<<"native_export: "<<e.what()<<'\n'; return 2; }
}
