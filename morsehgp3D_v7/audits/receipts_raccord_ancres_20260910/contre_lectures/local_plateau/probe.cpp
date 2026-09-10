// Audit probe: nominal header vs shortcut-disabled copy vs external judge.
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "src/forest/local_plateau.hpp"
#include "src/forest/local_plateau_general.hpp"
using namespace mhgp7;

template <class Rank>
void dump_rank(const char* tag, std::size_t index, std::size_t k, const Rank& r) {
  std::printf("rank %zu %s %zu %d %d %d %d %d %u %u %d %llu %llu %llu %llu %zu", index, tag, k,
              int(r.present), int(r.analytic_diameter_hub), int(r.analytic_interior_hub),
              int(r.inert_sufficient), int(r.no_strict_local_component), unsigned(r.closed_shell_cover),
              unsigned(r.contribution_shell), int(r.contribution_interior),
              (unsigned long long)r.reduced_vertices, (unsigned long long)r.strict_cofaces,
              (unsigned long long)r.union_attempts, (unsigned long long)r.dsu_mask_slots,
              r.strict_components.size());
  for (const auto& c : r.strict_components) {
    std::printf(" %zu:%u:%u:", c.interior_prefix, unsigned(c.representative_shell), unsigned(c.shell_cover));
    bool first = true;
    for (auto m : c.reduced_members) { std::printf("%s%u", first ? "" : ".", unsigned(m)); first = false; }
    if (c.reduced_members.empty()) std::printf("-");
  }
  std::printf("\n");
}

template <class Table>
void dump_table(const char* tag, std::size_t index, const Table& t) {
  std::printf("table %zu %s %u %u supports", index, tag, t.q_min(), t.max_strict_cardinality());
  bool first = true;
  for (auto m : t.minimal_supports()) { std::printf("%s%u", first ? " " : ",", unsigned(m)); first = false; }
  std::printf(" contains ");
  for (auto v : t.contains_center()) std::printf("%d", int(v));
  std::printf("\n");
}

int main() {
  std::size_t n = 0;
  if (!(std::cin >> n)) return 2;
  for (std::size_t i = 0; i < n; ++i) {
    long long a, b0, b1, b2, c; std::size_t p, u;
    std::cin >> a >> b0 >> b1 >> b2 >> c >> p >> u;
    local_plateau::LocalCensus census{BallKey{(i128)a, {(i128)b0, (i128)b1, (i128)b2}, (i128)c}, {}, {}};
    local_plateau_general::LocalCensus census_g{BallKey{(i128)a, {(i128)b0, (i128)b1, (i128)b2}, (i128)c}, {}, {}};
    for (std::size_t j = 0; j < p + u; ++j) {
      unsigned long long id; long long x, y, z; std::cin >> id >> x >> y >> z;
      InputPoint pt{(PointId)id, P3{x, y, z}};
      (j < p ? census.interior : census.shell).push_back(pt);
      (j < p ? census_g.interior : census_g.shell).push_back(pt);
    }
    std::string rej_n, rej_g;
    try {
      const auto table = local_plateau::ShellTable::prepare(census);
      dump_table("nominal", i, table);
      for (std::size_t k = 1; k <= p + u + 1; ++k) dump_rank("nominal", i, k, table.rank(k));
    } catch (const std::invalid_argument& e) { rej_n = e.what(); }
    try {
      const auto table = local_plateau_general::ShellTable::prepare(census_g);
      dump_table("general", i, table);
      for (std::size_t k = 1; k <= p + u + 1; ++k) dump_rank("general", i, k, table.rank(k));
    } catch (const std::invalid_argument& e) { rej_g = e.what(); }
    if (!rej_n.empty() || !rej_g.empty()) std::printf("rejected %zu nominal=%s general=%s\n", i, rej_n.c_str(), rej_g.c_str());
  }
  return 0;
}
