// Sonde cells : K budget_cellule n, puis n lignes XYZ/PointId. Toutes les cellules de CatK sont rendues.
#include <algorithm>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "tower/cells.hpp"

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
void ledger(std::ostream& out, const CellLedger& l) {
  out << "\"ledger\":{\"combinations\":" << l.combinations << ",\"passes\":" << l.passes
      << ",\"trace_tests\":" << l.trace_tests << ",\"meb_calls\":" << l.meb_calls
      << ",\"meb\":{\"presentations\":" << l.meb.presentations << ",\"nondegenerate\":" << l.meb.nondegenerate
      << ",\"positive\":" << l.meb.positive << ",\"containing\":" << l.meb.containing
      << ",\"comparisons\":" << l.meb.comparisons << ",\"point_tests\":" << l.meb.point_tests << "}}";
}
void cell_json(std::ostream& out, const LocalCell& cell) {
  out << "{\"order\":" << unsigned{cell.order()} << ",\"regular\":" << (cell.regular() ? "true" : "false")
      << ",\"kind\":\"" << (cell.kind() == CellKind::birth ? "birth" : "strict_traces") << "\",\"traces\":[";
  for (std::size_t i = 0; i < cell.traces().size(); ++i) {
    const auto& trace = cell.traces()[i];
    out << (i == 0 ? "" : ",") << "{\"arity\":" << unsigned{trace.arity} << ",\"sites\":";
    ids(out, std::span<const SiteIdx>{trace.sites}); out << '}';
  }
  out << "],"; ledger(out, cell.ledger()); out << '}';
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
    out << ",\"cells\":[";
    const u32 lo = ball.p + ball.qmin - 1, hi = std::min<u32>(ball.p + ball.m, cat.kmax());
    for (u32 k = lo; k <= hi; ++k) {
      auto cell = build_cell(domain, BallIdx{b}, static_cast<Order>(k), work);
      if (!cell.ok()) return cell.outcome();
      if (k != lo) out << ',';
      cell_json(out, cell.value());
    }
    out << "]}";
  }
  out << ']'; return out.str();
}
void execute(const Request& r) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(r.budget);
  auto answer = guarded([&] { return payload(r, owner, work); });
  const auto issue = merge(answer.outcome(), merge(owner.released(), work.released()));
  std::cout << "{\"status\":\"" << status_name(issue.status()) << "\",\"reason\":\"" << reason_name(issue.reason)
            << "\",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << r.kmax << ",\"trace_bytes\":" << sizeof(CellTrace)
            << ",\"cell_memory\":{\"after\":" << work.used() << ",\"peak\":" << work.peak()
            << "},\"owner_after\":" << owner.used() << ',';
  if (issue.ok()) std::cout << answer.value();
  else std::cout << "\"sites\":[],\"site_ids\":[],\"balls\":null";
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
