#include <sys/resource.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "tower/forest/full_ball_tower.hpp"

using namespace mhgp9::tower;

template<class T> void read_field(const char*& p, T& out) {
  std::memcpy(&out, p, sizeof(out)); p += sizeof(out);
}
template<class T> void write_field(std::ofstream& out, const T& x) {
  out.write(reinterpret_cast<const char*>(&x), sizeof(x));
}
void write_level(std::ofstream& out, const ExactLevel& x) {
  for (auto word : x.num) write_field(out, word);
  write_field(out, x.den);
}

int main(int argc, char** argv) {
  if (argc != 4) throw std::invalid_argument("usage: build_tower scene.u32le catalogue.bin canonical.bin");
  std::ifstream raw(argv[1], std::ios::binary | std::ios::ate);
  if (!raw) throw std::runtime_error("input open");
  const auto bytes = raw.tellg();
  if (bytes < 0 || bytes % 12 != 0) throw std::runtime_error("input size");
  raw.seekg(0);
  std::vector<InputPoint> points(static_cast<std::size_t>(bytes / 12));
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::uint32_t coords[3];
    raw.read(reinterpret_cast<char*>(coords), sizeof(coords));
    if (!raw) throw std::runtime_error("input read");
    points[i] = {static_cast<PointId>(i), {coords[0], coords[1], coords[2]}};
  }
  auto ix = build_cloud_index(points);
  if (!ix.valid || ix.upos.size() != points.size()) throw std::runtime_error("index invalid");
  std::ifstream cat(argv[2], std::ios::binary);
  if (!cat) throw std::runtime_error("catalogue open");
  std::uint64_t n = 0;
  cat.read(reinterpret_cast<char*>(&n), sizeof(n));
  if (!cat || n > 6000000) throw std::runtime_error("catalogue count");
  std::vector<BallData> balls(static_cast<std::size_t>(n));
  for (auto& b : balls) {
    char row[207]; cat.read(row, sizeof(row));
    if (!cat) throw std::runtime_error("catalogue read");
    const char* p = row;
    read_field(p, b.key.a);
    for (auto& x : b.key.b) read_field(p, x);
    read_field(p, b.key.c);
    for (auto& x : b.level.num) read_field(p, x);
    read_field(p, b.level.den);
    read_field(p, b.arity); read_field(p, b.n_interior); read_field(p, b.n_shell);
    for (auto& x : b.interior_ids) read_field(p, x);
    for (auto& x : b.shell_ids) read_field(p, x);
    if (p != row + sizeof(row)) throw std::runtime_error("catalogue row mismatch");
  }
  if (cat.peek() != EOF) throw std::runtime_error("catalogue trailing bytes");
  auto begin = std::chrono::steady_clock::now();
  auto result = build_full_ball_tower(ix, balls, 10, 48, {}, true);
  auto end = std::chrono::steady_clock::now();
  if (result.status != FullBallStatus::kCompleteRelative) {
    std::cerr << "status=" << static_cast<int>(result.status) << " reason=" << result.reason << '\n';
    return 3;
  }
  std::ofstream out(argv[3], std::ios::binary | std::ios::trunc);
  if (!out) throw std::runtime_error("canonical open");
  const auto write_count = [&](std::size_t value) { write_field(out, static_cast<std::uint64_t>(value)); };
  write_count(result.orders.size());
  const auto bank = result.orders.front().forest.populations();
  write_count(bank->domain().size());
  for (auto id : bank->domain()) write_field(out, id);
  write_count(bank->rows().size());
  for (const auto& row : bank->rows()) {
    write_count(row.interior.size());
    for (auto id : row.interior) write_field(out, id);
    write_count(row.shell.size());
    for (auto id : row.shell) write_field(out, id);
  }
  for (const auto& order : result.orders) {
    const auto& f = order.forest;
    write_field(out, f.order());
    write_count(f.nodes().size());
    for (const auto& node : f.nodes()) {
      write_level(out, node.level);
      write_field(out, node.first);
      write_field(out, node.parent_count);
    }
    write_count(f.parents().size());
    for (auto id : f.parents()) write_field(out, id);
    write_count(f.successors().size());
    for (auto id : f.successors()) write_field(out, id);
    write_count(f.contributions().size());
    for (const auto& c : f.contributions()) {
      write_level(out, c.level);
      write_field(out, c.segment);
      write_field(out, c.ref.population);
      write_field(out, c.ref.shell_mask);
      write_field(out, static_cast<std::uint8_t>(c.ref.include_interior));
    }
    write_count(order.lower_nodes.size());
    for (auto id : order.lower_nodes) write_field(out, id);
  }
  if (!out) throw std::runtime_error("canonical write");
  rusage ru{}; getrusage(RUSAGE_SELF, &ru);
  std::cout << "status=complete_relative orders=" << result.orders.size()
            << " representatives=" << result.stats.representatives
            << " births=" << result.stats.births
            << " merges=" << result.stats.merges
            << " tower_ms=" << std::chrono::duration<double,std::milli>(end-begin).count()
            << " peak_rss_kb=" << ru.ru_maxrss << '\n';
}
