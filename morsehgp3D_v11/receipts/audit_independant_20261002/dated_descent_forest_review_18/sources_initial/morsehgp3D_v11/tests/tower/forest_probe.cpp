// Sonde FULL : K budget_foret n puis n lignes XYZ/PointId ; structure canonique et verticales exactes.
#include <algorithm>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "tower/forest.hpp"

using namespace mhgp11;
using namespace mhgp11::tower_detail;
namespace {
struct Request {
  int kmax = 0;
  u64 budget = 0;
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
};
template <class T>
bool number(std::string_view word, T& value) {
  const auto result = std::from_chars(word.data(), word.data() + word.size(), value);
  return !word.empty() && result.ec == std::errc{} && result.ptr == word.data() + word.size();
}
template <class T>
bool read(T& value) { std::string word; return bool(std::cin >> word) && number(word, value); }
bool request(std::string_view first, Request& r) {
  u32 n = 0;
  if (!number(first, r.kmax) || !read(r.budget) || !read(n) || n > 14) return false;
  r.x.resize(n); r.y.resize(n); r.z.resize(n); r.ids.resize(n);
  for (u32 i = 0; i < n; ++i) {
    u32 id = 0;
    if (!read(r.x[i]) || !read(r.y[i]) || !read(r.z[i]) || !read(id)) return false;
    r.ids[i] = PointId{id};
  }
  return true;
}
template <class T>
std::string hex(const T& value) {
  const auto wide = num::to_wide(value);
  std::ostringstream out;
  if (wide.sign() < 0) out << '-';
  out << std::hex << std::setfill('0');
  for (std::size_t i = wide.words.size(); i != 0; --i) out << std::setw(16) << wide.words[i - 1];
  return out.str();
}
template <StrongId Id>
void ids(std::ostream& out, std::span<const Id> values) {
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) out << (i == 0 ? "" : ",") << idx(values[i]);
  out << ']';
}
void level(std::ostream& out, const num::Level& value) {
  out << "[\"" << hex(value.numerator()) << "\",\"" << hex(value.denominator()) << "\"]";
}
void meb(std::ostream& out, const MebLedger& l) {
  out << "{\"presentations\":" << l.presentations << ",\"nondegenerate\":" << l.nondegenerate
      << ",\"positive\":" << l.positive << ",\"containing\":" << l.containing
      << ",\"comparisons\":" << l.comparisons << ",\"point_tests\":" << l.point_tests << '}';
}
void descent_work(std::ostream& out, const DescentLedger& l) {
  out << "{\"steps\":" << l.steps << ",\"interior_steps\":" << l.interior_steps
      << ",\"trace_steps\":" << l.trace_steps << ",\"candidate_traces\":" << l.candidate_traces
      << ",\"trace_meb_calls\":" << l.trace_meb_calls << ",\"census_calls\":" << l.census_calls
      << ",\"catalogue_hits\":" << l.catalogue_hits << ",\"part_meb\":";
  meb(out, l.part_meb); out << ",\"trace_meb\":"; meb(out, l.trace_meb);
  out << ",\"census\":{\"nodes\":" << l.census.nodes << ",\"bounds\":" << l.census.bounds
      << ",\"point_tests\":" << l.census.point_tests << ",\"inside_blocks\":" << l.census.inside_blocks
      << ",\"outside_blocks\":" << l.census.outside_blocks << ",\"passes\":" << l.census.passes << "}}";
}
void forest_work(std::ostream& out, const ForestLedger& l) {
  out << "{\"classified_cells\":" << l.classified_cells << ",\"replayed_cells\":" << l.replayed_cells
      << ",\"plateaus\":" << l.plateaus << ",\"trace_resolutions\":" << l.trace_resolutions
      << ",\"unions\":" << l.unions << ",\"touched_components\":" << l.touched_components
      << ",\"continuations\":" << l.continuations << ",\"center_comparisons\":" << l.center_comparisons
      << ",\"birth_presentations\":" << l.birth_presentations << ",\"ancestor_hops\":" << l.ancestor_hops
      << ",\"vertical_descents\":" << l.vertical_descents << ",\"vertical_checks\":" << l.vertical_checks
      << ",\"cells\":{\"combinations\":" << l.cells.combinations << ",\"passes\":" << l.cells.passes
      << ",\"trace_tests\":" << l.cells.trace_tests << ",\"meb_calls\":" << l.cells.meb_calls
      << ",\"meb\":"; meb(out, l.cells.meb); out << "},\"descent\":"; descent_work(out, l.descent); out << '}';
}
void order_json(std::ostream& out, const FullTower& tower, Order k) {
  const auto& forest = tower.order(k);
  out << "{\"order\":" << unsigned{k} << ",\"births\":" << forest.births()
      << ",\"node_capacity\":" << forest.node_capacity() << ",\"edge_capacity\":" << forest.edge_capacity()
      << ",\"root\":" << idx(forest.root()) << ",\"nodes\":[";
  for (u32 i = 0; i < forest.nodes().size(); ++i) {
    const auto& node = forest.nodes()[i];
    out << (i == 0 ? "" : ",") << "{\"level\":";
    level(out, tower.domain().catalogue().levels()[idx(node.rank)]);
    out << ",\"parent\":"; if (node.parent == NodeIdx{kNone}) out << "null"; else out << idx(node.parent);
    out << ",\"children\":"; ids(out, forest.children(NodeIdx{i})); out << ",\"seed\":";
    if (node.child_count != 0) out << "null";
    else if (k == 1) out << "{\"site\":" << node.birth_key << ",\"ball\":null}";
    else out << "{\"site\":null,\"ball\":" << node.birth_key << '}';
    out << '}';
  }
  out << "],\"lower\":";
  if (k == 1) out << "null"; else ids(out, forest.lower());
  out << ",\"ledger\":"; forest_work(out, forest.ledger()); out << '}';
}
Result<std::string> payload(const Request& r, MemoryBudget& owner, MemoryBudget& work) {
  auto cloud = prepare_cloud(r.x, r.y, r.z, r.ids, CoordWidth{}, owner);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{2}, owner);
  if (!index.ok()) return index.outcome();
  CatalogueParams params; params.kmax = r.kmax;
  auto made = prepare_full_domain(std::move(index.value()), params, owner);
  if (!made.ok()) return made.outcome();
  auto full = build_full(std::move(made.value()), work);
  if (!full.ok()) return full.outcome();
  const auto& domain = full.value().domain();
  const auto& cat = domain.catalogue(); const auto& points = domain.index().cloud();
  std::ostringstream out;
  out << "\"sites\":[";
  for (u32 i = 0; i < points.sites(); ++i)
    out << (i == 0 ? "" : ",") << '[' << points.x()[i] << ',' << points.y()[i] << ',' << points.z()[i] << ']';
  out << "],\"site_ids\":[";
  for (u32 i = 0; i < points.sites(); ++i) { if (i != 0) out << ','; ids(out, points.points(SiteIdx{i})); }
  out << "],\"balls\":[";
  for (u32 b = 0; b < cat.balls(); ++b) {
    const auto& ball = cat.balls_data()[b]; const auto& level = cat.levels()[idx(ball.rank)];
    out << (b == 0 ? "" : ",") << "{\"support\":";
    ids(out, std::span<const SiteIdx>{ball.support}.first(ball.qmin));
    out << ",\"qmin\":" << unsigned{ball.qmin} << ",\"level\":[\"" << hex(level.numerator())
        << "\",\"" << hex(level.denominator()) << "\"],\"inner\":";
    ids(out, cat.interior(BallIdx{b})); out << ",\"shell\":"; ids(out, cat.shell(BallIdx{b}));
    out << '}';
  }
  out << "],\"orders\":[";
  for (u32 k = 1; k <= full.value().kmax(); ++k) {
    if (k != 1) out << ',';
    order_json(out, full.value(), static_cast<Order>(k));
  }
  out << ']'; return out.str();
}
void execute(const Request& r) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(r.budget);
  auto answer = guarded([&] { return payload(r, owner, work); });
  const auto issue = merge(answer.outcome(), merge(owner.released(), work.released()));
  std::cout << "{\"status\":\"" << status_name(issue.status()) << "\",\"reason\":\"" << reason_name(issue.reason)
            << "\",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << r.kmax
            << ",\"forest_memory\":{\"after\":" << work.used() << ",\"peak\":" << work.peak()
            << "},\"owner_after\":" << owner.used() << ',';
  if (issue.ok()) std::cout << answer.value();
  else std::cout << "\"sites\":[],\"site_ids\":[],\"balls\":null,\"orders\":[]";
  std::cout << "}\n";
}
}  // namespace
int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--profile") {
    std::cout << "{\"coord_bits\":" << kCoordBits << "}\n"; return 0;
  }
  if (argc != 1) return 2;
  try {
    std::string first;
    while (std::cin >> first) { Request r; if (!request(first, r)) return 2; execute(r); }
  } catch (const std::bad_alloc&) { return 2; }
  return std::cin.eof() ? 0 : 2;
}
