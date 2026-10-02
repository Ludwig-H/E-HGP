// Sonde descente : K k budget n m, n lignes XYZ/PointId puis m SiteIdx Morton. Aucun chemin produit alloue.
#include <algorithm>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "tower/descent.hpp"

using namespace mhgp11;
using namespace mhgp11::tower_detail;
namespace {
struct Request {
  int kmax = 0;
  u32 order = 0;
  std::vector<SiteIdx> part;
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
  u32 n = 0, m = 0;
  if (!number(first, r.kmax) || !read(r.order) || !read(r.budget) || !read(n) || !read(m) || n > 14 || m > 13)
    return false;
  r.x.resize(n); r.y.resize(n); r.z.resize(n); r.ids.resize(n);
  for (u32 i = 0; i < n; ++i) {
    u32 id = 0;
    if (!read(r.x[i]) || !read(r.y[i]) || !read(r.z[i]) || !read(id)) return false;
    r.ids[i] = PointId{id};
  }
  r.part.resize(m);
  for (u32 i = 0; i < m; ++i) { u32 id = 0; if (!read(id)) return false; r.part[i] = SiteIdx{id}; }
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
void ledger(std::ostream& out, const DescentLedger& l) {
  out << "{\"steps\":" << l.steps << ",\"interior_steps\":" << l.interior_steps
      << ",\"trace_steps\":" << l.trace_steps << ",\"candidate_traces\":" << l.candidate_traces
      << ",\"trace_meb_calls\":" << l.trace_meb_calls << ",\"census_calls\":" << l.census_calls
      << ",\"catalogue_hits\":" << l.catalogue_hits << ",\"part_meb\":";
  meb(out, l.part_meb); out << ",\"trace_meb\":"; meb(out, l.trace_meb);
  out << ",\"census\":{\"nodes\":" << l.census.nodes << ",\"bounds\":" << l.census.bounds
      << ",\"point_tests\":" << l.census.point_tests << ",\"inside_blocks\":" << l.census.inside_blocks
      << ",\"outside_blocks\":" << l.census.outside_blocks << ",\"passes\":" << l.census.passes << "}}";
}
void seed(std::ostream& out, const BirthSeed& value) {
  out << "{\"site\":";
  if (value.site()) out << idx(*value.site()); else out << "null";
  out << ",\"ball\":";
  if (value.ball()) out << idx(*value.ball()); else out << "null";
  out << ",\"order\":" << unsigned{value.order()} << '}';
}
Outcome path(std::ostream& out, const FullDomain& domain, const Request& r,
             const DescentResult& result, MemoryBudget& work) {
  std::span<const SiteIdx> part = r.part;
  CellTrace storage;
  DescentLedger sum;
  out << ",\"steps\":[";
  for (u64 i = 0; i < result.ledger().steps; ++i) {
    auto step = descent_step(domain, part, r.order, work);
    if (!step.ok()) return step.outcome();
    const auto& value = step.value();
    out << (i == 0 ? "" : ",") << "{\"part\":"; ids(out, part);
    out << ",\"level\":"; level(out, value.level());
    out << ",\"next\":"; ids(out, value.next().part());
    out << ",\"seed\":"; if (value.seed()) seed(out, *value.seed()); else out << "null";
    out << ",\"ledger\":"; ledger(out, value.ledger()); out << '}';
    MHGP11_TRY(add_descent(sum, value.ledger()));
    if (value.seed()) {
      if (i + 1 != result.ledger().steps || sum != result.ledger() || *value.seed() != result.seed())
        return fail(Reason::tower_invariant);
      out << ']'; return {};
    }
    storage = value.next(); part = storage.part();
  }
  return fail(Reason::tower_invariant);
}
Result<std::string> payload(const Request& r, MemoryBudget& owner, MemoryBudget& work) {
  auto cloud = prepare_cloud(r.x, r.y, r.z, r.ids, CoordWidth{}, owner);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{2}, owner);
  if (!index.ok()) return index.outcome();
  CatalogueParams params; params.kmax = r.kmax;
  auto made = prepare_full_domain(std::move(index.value()), params, owner);
  if (!made.ok()) return made.outcome();
  const auto& domain = made.value(); const auto& cat = domain.catalogue(); const auto& points = domain.index().cloud();
  auto result = descend(domain, r.part, r.order, work);
  if (!result.ok()) return result.outcome();
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
  out << ']';
  MHGP11_TRY(path(out, domain, r, result.value(), work));
  out << ",\"result\":{\"initial_level\":"; level(out, result.value().initial_level());
  out << ",\"terminal_level\":"; level(out, result.value().terminal_level());
  out << ",\"seed\":"; seed(out, result.value().seed());
  out << ",\"ledger\":"; ledger(out, result.value().ledger());
  out << '}'; return out.str();
}
void execute(const Request& r) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(r.budget);
  auto answer = guarded([&] { return payload(r, owner, work); });
  const auto issue = merge(answer.outcome(), merge(owner.released(), work.released()));
  std::cout << "{\"status\":\"" << status_name(issue.status()) << "\",\"reason\":\"" << reason_name(issue.reason)
            << "\",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << r.kmax << ",\"order\":" << r.order
            << ",\"query_memory\":{\"after\":" << work.used() << ",\"peak\":" << work.peak()
            << "},\"owner_after\":" << owner.used() << ',';
  if (issue.ok()) std::cout << answer.value();
  else std::cout << "\"sites\":[],\"site_ids\":[],\"balls\":null,\"steps\":[],\"result\":null";
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
