// Audit-only: independent row tiles with one traced representative per tile.
// No product edits. Samples are diagnostics, never full-frame contracts.
#include "lanes/q34_witness_search.hpp"
#include "wspd/front.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {
using namespace mhgp9::gen;
using Clock = std::chrono::steady_clock;
void need(bool ok, const char* reason) { if (!ok) throw std::runtime_error(reason); }
double ms(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now()-t).count();
}
std::vector<Point3> read(const std::string& path) {
  need(path.ends_with(".u32le"), "u32le input required");
  std::ifstream f(path, std::ios::binary);
  need(static_cast<bool>(f), "input open");
  std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(f)), {});
  need(!bytes.empty() && bytes.size()%12 == 0, "whole u32 records");
  std::vector<Point3> p(bytes.size()/12);
  for (std::size_t i=0;i<p.size();++i)
    for (std::size_t a=0;a<3;++a) {
      std::uint32_t v=0;
      for (std::size_t b=0;b<4;++b) v |= std::uint32_t{bytes[12*i+4*a+b]}<<(8*b);
      need(v<262144, "u18 domain");
      const auto coordinate=static_cast<Coordinate>(v);
      if (a==0) p[i].x=coordinate;
      else if (a==1) p[i].y=coordinate;
      else p[i].z=coordinate;
    }
  return p;
}
struct Tile {
  std::size_t r, a, first, last;
  auto operator<=>(const Tile&) const = default;
};
}

int main(int argc, char** argv) {
  try {
    need(argc==5, "usage: probe frame.u32le K s samples (diagnostic tiles)");
    const auto number=[](const char* s) {
      std::size_t used=0; const auto value=std::stoull(s,&used);
      need(used==std::string(s).size(), "integer argument"); return value;
    };
    const auto k64=number(argv[2]), sep64=number(argv[3]), samples=number(argv[4]);
    need(k64>=3 && k64<=10 && (sep64==8 || sep64==10 || sep64==12), "K/s domain");
    need(samples>0 && samples<=1000000, "diagnostic sample count 1..1000000");
    const auto k=static_cast<std::uint8_t>(k64);
    const auto cloud=prepare_cloud(read(argv[1]));
    const auto index=make_q2_cloud_index(cloud);
    const auto nodes=index->spatial_nodes();
    const auto order=index->spatial_order();
    const auto points=cloud->points();
    const auto start=Clock::now();
    std::vector<WspdRectangle> rectangles;
    const auto front=run_wspd_front(*index,k,static_cast<unsigned>(sep64),WspdFrontMode::MidpointSamples,
        [&](const auto& r) { rectangles.push_back(r); },6);
    const double front_ms=ms(start);
    std::vector<std::uint64_t> offsets(1,0);
    for (const auto& r: rectangles) {
      const auto a=nodes[r.a_node].range, b=nodes[r.b_node].range;
      const auto mass=static_cast<std::uint64_t>(a.size())*b.size();
      need(offsets.back()<=std::numeric_limits<std::uint64_t>::max()-mass, "mass overflow");
      offsets.push_back(offsets.back()+mass);
    }
    const auto mass=offsets.back();
    need(mass>0, "nonvacuous raw rectangle population");
    std::set<Tile> selected;
    for (std::uint64_t i=0;i<samples;++i) {
      // Avoid overflow in i*mass; a stratum's first point, deterministic.
      const auto p=static_cast<std::uint64_t>((static_cast<i128>(i)*mass)/samples);
      const auto r=static_cast<std::size_t>(std::upper_bound(offsets.begin(),offsets.end(),p)-offsets.begin()-1);
      const auto a=nodes[rectangles[r].a_node].range, b=nodes[rectangles[r].b_node].range;
      const auto local=p-offsets[r];
      const auto row=a.first+local/b.size();
      const auto begin=b.first+(local%b.size()/32)*32;
      selected.insert({r,row,begin,std::min(b.last,begin+32)});
    }
    std::vector<std::uint8_t> rect_mask(rectangles.size(),0xff);
    Q34WitnessSearchWork rect_work, reference_work, tiled_work;
    Q34WitnessBoundsWork rect_bounds, reference_bounds, tiled_bounds;
    Q34WitnessCacheWork cache_work;
    std::vector<Q34WitnessNode> trace, ignored;
    std::uint64_t pairs=0, representatives=0, full_from_cache=0, partial_from_cache=0;
    std::uint64_t max_trace=0, sum_trace=0, rect_rejected_tiles=0, mismatches=0;
    const auto sample_start=Clock::now();
    for (const auto& tile:selected) {
      const auto& r=rectangles[tile.r];
      auto& mask=rect_mask[tile.r];
      if (mask==0xff)
        mask=filter_q34_witnesses(*index,nodes[r.a_node].box,nodes[r.b_node].box,k,r.lane_mask,
                                  rect_work,Q34WitnessBoundsMode::Affine,rect_bounds);
      if (mask==0) { ++rect_rejected_tiles; continue; }
      const auto a=points[order[tile.a]];
      // Independent representative: future lanes only read this antichain.
      const auto first=points[order[tile.first]];
      const auto rep=filter_q34_witnesses(*index,a,first,k,mask,tiled_work,tiled_bounds,trace);
      ++representatives; sum_trace+=trace.size();
      max_trace=std::max<std::uint64_t>(max_trace,trace.size());
      need(trace.size()<=static_cast<std::size_t>(2*k-3), "trace exceeded proved credit bound");
      for (std::size_t b=tile.first;b<tile.last;++b) {
        const auto pb=points[order[b]];
        const auto expected=filter_q34_witnesses(*index,a,pb,k,mask,reference_work,reference_bounds,ignored);
        std::uint8_t observed=rep;
        if (b!=tile.first) {
          const auto dead=q34_cached_witness_rejections(*index,a,pb,k,mask,trace,cache_work);
          const auto open=static_cast<std::uint8_t>(mask&~dead);
          full_from_cache+=open==0;
          partial_from_cache+=dead!=0 && open!=0;
          // No partial credits seed this search. No feedback across lanes.
          observed=open==0 ? std::uint8_t{0} : filter_q34_witnesses(
              *index,a,pb,k,open,tiled_work,tiled_bounds,ignored);
        }
        ++pairs; mismatches+=observed!=expected;
      }
    }
    need(pairs>0 && representatives>0 && cache_work.queries>0, "nonvacuous tile cache");
    std::cout << "{\"schema\":\"mhgp9_audit_tile_cache_v1\",\"status\":\""
              << (mismatches==0?"equal":"mismatch") << "\",\"scope\":\"stratified_raw_pair_mass_tiles_not_full\","
              << "\"n\":" << points.size() << ",\"K\":" << unsigned{k} << ",\"s\":" << sep64
              << ",\"requested_samples\":" << samples << ",\"rectangles\":" << rectangles.size()
              << ",\"front_products\":" << front.work.product_visits << ",\"raw_pair_mass\":" << mass
              << ",\"selected_tiles\":" << selected.size() << ",\"rect_rejected_tiles\":" << rect_rejected_tiles
              << ",\"pairs\":" << pairs << ",\"representatives\":" << representatives
              << ",\"sum_trace_nodes\":" << sum_trace << ",\"max_trace_nodes\":" << max_trace
              << ",\"rect_nodes\":" << rect_work.node_visits << ",\"baseline_nodes\":" << reference_work.node_visits
              << ",\"tiled_search_nodes\":" << tiled_work.node_visits
              << ",\"cache_node_tests\":" << cache_work.node_tests << ",\"cache_queries\":" << cache_work.queries
              << ",\"fully_cached\":" << full_from_cache << ",\"partly_cached\":" << partial_from_cache
              << ",\"mismatches\":" << mismatches << ",\"front_ms\":" << front_ms
              << ",\"sample_with_judge_ms\":" << ms(sample_start) << "}\n";
    return mismatches==0?0:1;
  } catch(const std::exception& e) {
    std::cerr << "tile_cache: " << e.what() << '\n'; return 2;
  }
}
