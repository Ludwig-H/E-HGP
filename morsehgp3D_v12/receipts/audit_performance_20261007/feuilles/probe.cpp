// Audit hors produit : travail physique de la source J3, sans chronometre.
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

namespace audit {
struct Counts {
  std::uint64_t dominance = 0, census_sites = 0, census_lanes = 0;
  std::uint64_t side_calls = 0, side_after_stop = 0, rejected = 0;
} counts;
}

#include "catalogue/leaf_j3.hpp"
using namespace mhgp12;
using namespace mhgp12::catalogue_detail;

template <u32 N> struct Sink {
  u64 emitted = 0, checksum = 1469598103934665603ull;
  std::vector<u64> words;
  void word(u64 v) { checksum = (checksum ^ v) * 1099511628211ull; words.push_back(v); }
  void emit(const Emission<N>& e) {
    ++emitted;
    for (u32 k = 0; k < 4; ++k) word(e.support[k]);
    word(e.p); word(e.qmin); word(e.q); word(e.m);
    for (u32 k = 0; k < N; ++k) {
      word(simt::test(e.interior, k)); word(simt::test(e.shell, k));
    }
  }
};

template <u32 N, class A = Narrow>
void run(const char* name, const std::vector<u32>& x, const std::vector<u32>& y,
         const std::vector<u32>& z, int k) {
  const u32 m = static_cast<u32>(x.size());
  std::vector<u32> ids(m);
  for (u32 i = 0; i < m; ++i) ids[i] = i;
  LeafInput in;
  in.x = x.data(); in.y = y.data(); in.z = z.data(); in.sites = ids.data();
  in.m = m; in.kmax = k;
  for (u32 a = 0; a < 3; ++a) in.hi[a] = 128;
  auto shared = std::make_unique<LeafShared<N>>();
  LeafCounts out{};
  Sink<N> sink;
  audit::counts = {};
  const auto status = run_leaf<N, A>(in, *shared, out, sink);
  std::cout << "{\"case\":\"" << name << "\",\"status\":" << status
            << ",\"sites\":" << m << ",\"width\":" << N << ",\"k\":" << k
            << ",\"emission_checksum\":" << sink.checksum << ",\"emission_words\":[";
  for (std::size_t i = 0; i < sink.words.size(); ++i) std::cout << (i ? "," : "") << sink.words[i];
  std::cout << "],\"logical\":[";
  for (u32 i = 0; i < kLeafCounters; ++i) std::cout << (i ? "," : "") << out.c[i];
  const auto c = audit::counts;
  std::cout << "],\"physical\":{\"dominance\":" << c.dominance
            << ",\"census_sites\":" << c.census_sites << ",\"census_lanes\":" << c.census_lanes
            << ",\"side_calls\":" << c.side_calls << ",\"side_after_stop\":" << c.side_after_stop
            << ",\"rejected\":" << c.rejected << "}}\n";
  if (status != kLeafOk || sink.emitted != out.c[kEmitted]) std::exit(1);
}

int main() {
  std::vector<u32> x(24), y(24, 1), z(24, 1);
  for (u32 i = 0; i < 24; ++i) x[i] = i + 1;
  run<32>("line24_k5", x, y, z, 5);
  run<32>("line24_k10", x, y, z, 10);
  run<32, Exact>("line24_exact_k5", x, y, z, 5);
  x.clear(); y.clear(); z.clear();
  for (u32 i = 0; i < 8; ++i) {
    x.push_back(16 * (i & 1)); y.push_back(16 * ((i >> 1) & 1)); z.push_back(16 * (i >> 2));
  }
  x.push_back(8); y.push_back(8); z.push_back(8);
  run<32>("cube_center9_k5", x, y, z, 5);
  x.resize(24); y.resize(24); z.resize(24);
  u32 state = 20261007;
  auto next = [&]() { state = state * 1664525u + 1013904223u; return 1 + (state >> 16) % 99; };
  for (u32 i = 0; i < 24; ++i) { x[i] = next(); y[i] = next(); z[i] = next(); }
  run<32>("synthetic24_k5", x, y, z, 5);
  run<32>("synthetic24_k10", x, y, z, 10);
  x.resize(40); y.assign(40, 1); z.assign(40, 1);
  for (u32 i = 0; i < 40; ++i) x[i] = i + 1;
  run<256>("line40_k5", x, y, z, 5);
}
