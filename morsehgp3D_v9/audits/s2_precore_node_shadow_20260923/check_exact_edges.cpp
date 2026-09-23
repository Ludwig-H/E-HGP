#include "gen/lanes/q34_cover.hpp"
#include "gen/pipeline/prepared_cloud.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace mhgp9::gen;

namespace {
std::vector<unsigned char> bytes(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("cannot open input");
  return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
std::uint32_t u32(const unsigned char* p) {
  return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
         (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
}

int main(int argc, char** argv) {
  if (argc < 4) return 2;
  const auto point_data = bytes(argv[1]), raw_data = bytes(argv[2]);
  if (point_data.empty() || point_data.size() % 12 || raw_data.size() != point_data.size() / 3)
    throw std::runtime_error("bad point/raw-ID input");
  const auto n = point_data.size() / 12;
  std::vector<Point3> points(n);
  std::unordered_map<std::uint32_t, std::size_t> local;
  local.reserve(n * 2);
  for (std::size_t i = 0; i != n; ++i) {
    points[i] = {Coordinate(u32(point_data.data() + 12 * i)),
                 Coordinate(u32(point_data.data() + 12 * i + 4)),
                 Coordinate(u32(point_data.data() + 12 * i + 8))};
    if (!local.emplace(u32(raw_data.data() + 4 * i), i).second)
      throw std::runtime_error("duplicate raw ID");
  }
  std::set<std::pair<std::uint32_t, std::uint32_t>> anomaly;
  for (int file = 3; file < argc; ++file) {
    std::ifstream input(argv[file]);
    if (!input) throw std::runtime_error("cannot open shadow result");
    std::string tag;
    while (input >> tag) {
      if (tag == "anomaly") {
        std::uint32_t a, b, before, after, sites;
        if (!(input >> a >> b >> before >> after >> sites) || a == b ||
            (before != 2 && before != 4 && before != 6) || after == 0 ||
            (after & ~before) != 0 || sites < 2)
          throw std::runtime_error("invalid anomaly record");
        anomaly.emplace(std::min(a, b), std::max(a, b));
      }
      std::string rest;
      std::getline(input, rest);
    }
  }
  if (anomaly.empty()) throw std::runtime_error("no anomaly edges");
  const auto index = make_q2_cloud_index(prepare_cloud(points));
  std::uint64_t q3_all = 0, q4_all = 0, cover_sites = 0, node_visits = 0;
  for (const auto& [a, b] : anomaly) {
    if (!local.contains(a) || !local.contains(b)) throw std::runtime_error("anomaly ID not in input");
    const auto cover = Q34EdgeCover::make(index, {local.at(a), local.at(b)});
    std::uint64_t q3 = 0, q4 = 0;
    const auto work = run_q34_edge_candidates(cover, 5, [&](const Q34SeedCandidate& c) {
      if (c.arity == 3) ++q3;
      else if (c.arity == 4) ++q4;
      else throw std::runtime_error("unexpected candidate arity");
    });
    cover_sites += cover->site_count();
    node_visits += work.node_visits;
    q3_all += q3;
    q4_all += q4;
    std::cout << "edge " << a << ' ' << b << ' ' << cover->site_count() << ' '
              << work.node_visits << ' ' << q3 << ' ' << q4 << '\n';
  }
  std::cout << "total " << anomaly.size() << ' ' << cover_sites << ' '
            << node_visits << ' ' << q3_all << ' ' << q4_all << '\n';
  if (q3_all != 0 || q4_all != 0) return 1;
}
