#include "narrow_pair.hpp"
#include "../../src/gpu/flat_index.hpp"
#include "../../src/gen/wspd/front.hpp"
#include "../../tests/gen/front_fixtures.hpp"
#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace mhgp9;
using namespace audit_b::s2_narrow;
struct Query { gpu::FlatBox a, b; gpu::u8 mask; };
using Clock = std::chrono::steady_clock;
double seconds(Clock::time_point t) { return std::chrono::duration<double>(Clock::now()-t).count(); }
std::size_t number(std::string_view s) {
  std::size_t n{}; const auto [p,e] = std::from_chars(s.data(), s.data()+s.size(), n);
  if (e != std::errc{} || p != s.data()+s.size()) throw std::invalid_argument("invalid number");
  return n;
}
std::vector<gen::Point3> read(const char* path) {
  std::ifstream in(path,std::ios::binary); if (!in) throw std::runtime_error("input missing");
  std::vector<gen::Point3> points; std::array<unsigned char,12> bytes{};
  while (in.read(reinterpret_cast<char*>(bytes.data()),12)) {
    std::uint32_t v[3]{};
    for (unsigned a=0;a<3;++a) for (unsigned j=0;j<4;++j) v[a] |= static_cast<std::uint32_t>(bytes[4*a+j]) << (8*j);
    if (v[0]>262143 || v[1]>262143 || v[2]>262143) throw std::runtime_error("input outside u18");
    points.push_back({static_cast<gen::Coordinate>(v[0]),static_cast<gen::Coordinate>(v[1]),static_cast<gen::Coordinate>(v[2])});
  }
  if (in.gcount()!=0 || !in.eof() || points.size()<2) throw std::runtime_error("invalid input length");
  return points;
}
}

int main(int argc,char** argv) try {
  // sample synthetic terrain 8000 1024 5 ; sample file /path/full.u32le 1024 5
  std::vector<mhgp9::gen::Point3> input; std::string kind; std::size_t stride{},k{};
  if (argc==6 && std::string_view(argv[1])=="synthetic") {
    kind=argv[2]; input=mhgp9::gen::bench::make_front_fixture(number(argv[3]),kind,3).points;
    stride=number(argv[4]); k=number(argv[5]);
  } else if (argc==5 && std::string_view(argv[1])=="file") {
    kind="u32le"; input=read(argv[2]); stride=number(argv[3]); k=number(argv[4]);
  } else throw std::runtime_error("usage: sample synthetic FAMILY N STRIDE K | file PATH STRIDE K");
  if (stride==0 || k<3 || k>10) throw std::runtime_error("invalid sampling stride or K");
  const auto start=Clock::now();
  const auto cloud=mhgp9::gen::prepare_cloud(input);
  const auto index=mhgp9::gen::make_q2_cloud_index(cloud);
  const auto flat=mhgp9::gpu::flatten_nodes(*index);
  const auto nodes=index->spatial_nodes(); const auto ranks=index->spatial_order(); const auto points=index->cloud().points();
  std::vector<Query> queries;
  std::uint64_t rectangles=0,surviving=0,mass=0,rectangle_visits=0;
  const auto front=mhgp9::gen::run_wspd_front(*index,static_cast<unsigned>(k),8,mhgp9::gen::WspdFrontMode::MidpointSamples,
      [&](const mhgp9::gen::WspdRectangle& r) {
        ++rectangles; const auto& a=nodes[r.a_node]; const auto& b=nodes[r.b_node];
        const auto mask=mhgp9::gpu::filter_boxes(flat.data(),mhgp9::gpu::flat_box(a.box),mhgp9::gpu::flat_box(b.box),
            static_cast<unsigned>(k),r.lane_mask,rectangle_visits);
        if(mask==mhgp9::gpu::stack_failure) throw std::runtime_error("rectangle stack failure");
        if(mask==0) return;
        ++surviving;
        const std::uint64_t na=a.range.last-a.range.first,nb=b.range.last-b.range.first;
        if(na && nb>std::numeric_limits<std::uint64_t>::max()/na) throw std::runtime_error("mass overflow");
        const auto count=na*nb;
        if(mass>std::numeric_limits<std::uint64_t>::max()-count) throw std::runtime_error("sum overflow");
        for(std::uint64_t p=(stride-mass%stride)%stride;p<count;p+=stride) {
          queries.push_back({mhgp9::gpu::flat_point(points[ranks[a.range.first+p/nb]]),
                             mhgp9::gpu::flat_point(points[ranks[b.range.first+p%nb]]),mask});
          if (p>std::numeric_limits<std::uint64_t>::max()-stride) break;
        }
        mass+=count;
      },6);
  static_cast<void>(front);
  const double preparation=seconds(start);
  std::vector<mhgp9::gpu::u8> expected(queries.size());
  std::vector<std::uint64_t> visits(queries.size());
  Work all{}; std::uint64_t output_hash=14695981039346656037ULL,open=0,rejected=0;
  double baseline[3]{},candidate[3]{};
  for(unsigned rep=0;rep<3;++rep) for(unsigned step=0;step<2;++step) {
    const bool narrow=(step+(rep%2))%2!=0;
    const auto begin=Clock::now();
    for(std::size_t q=0;q<queries.size();++q) {
      const auto& x=queries[q];
      if(narrow) {
        Work w{}; const auto result=filter_pairs(flat.data(),x.a,x.b,static_cast<unsigned>(k),x.mask,w);
        if(result==mhgp9::gpu::stack_failure || result!=expected[q] || w.visits!=visits[q])
          throw std::runtime_error("mask or visits differ");
        if(rep==0) { all.visits+=w.visits; all.eligible+=w.eligible; all.fallback+=w.fallback; all.h_rejected+=w.h_rejected; }
      } else {
        std::uint64_t v=0; const auto result=mhgp9::gpu::filter<true>(flat.data(),x.a,x.b,static_cast<unsigned>(k),x.mask,v);
        if(result==mhgp9::gpu::stack_failure) throw std::runtime_error("baseline stack failure");
        if(rep==0) {
          expected[q]=result; visits[q]=v; output_hash^=result; output_hash*=1099511628211ULL;
          ++(result?open:rejected);
        } else if(expected[q]!=result || visits[q]!=v) throw std::runtime_error("baseline changed");
      }
    }
    (narrow?candidate[rep]:baseline[rep])=seconds(begin);
  }
  if(queries.empty() || all.eligible==0 || all.h_rejected==0)
    throw std::runtime_error("sample nonvacuity");
  std::printf("{\"status\":\"pass\",\"kind\":\"%s\",\"n\":%zu,\"k\":%zu,\"s\":8,\"stride\":%zu,\"rectangles\":%llu,\"surviving_rectangles\":%llu,\"pair_mass\":%llu,\"queries\":%zu,\"open\":%llu,\"rejected\":%llu,\"visits\":%llu,\"eligible\":%llu,\"fallback\":%llu,\"h_rejected\":%llu,\"output_hash\":%llu,\"preparation_s\":%.9f,\"baseline_s\":[%.9f,%.9f,%.9f],\"candidate_s\":[%.9f,%.9f,%.9f]}\n",
    kind.c_str(),input.size(),k,stride,static_cast<unsigned long long>(rectangles),static_cast<unsigned long long>(surviving),
    static_cast<unsigned long long>(mass),queries.size(),static_cast<unsigned long long>(open),static_cast<unsigned long long>(rejected),
    static_cast<unsigned long long>(all.visits),static_cast<unsigned long long>(all.eligible),static_cast<unsigned long long>(all.fallback),
    static_cast<unsigned long long>(all.h_rejected),static_cast<unsigned long long>(output_hash),preparation,
    baseline[0],baseline[1],baseline[2],candidate[0],candidate[1],candidate[2]);
  return 0;
} catch(const std::exception& e) { std::fprintf(stderr,"failure: %s\n",e.what()); return 1; }
