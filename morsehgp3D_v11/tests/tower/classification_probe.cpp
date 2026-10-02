// Sonde classification : K n puis n lignes XYZ/PointId ; inventaire canonique de chaque cellule admissible.
#include <algorithm>
#include <charconv>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "tower/cells.hpp"

using namespace mhgp11;
using namespace mhgp11::tower_detail;
namespace {
template<class T>
bool read(T& value) {
  std::string word;
  if (!(std::cin >> word)) return false;
  const auto result = std::from_chars(word.data(), word.data() + word.size(), value);
  return result.ec == std::errc{} && result.ptr == word.data() + word.size();
}
void ledger(std::ostream& out, const ClassificationLedger& l) {
  out << "{\"combinations\":" << l.combinations << ",\"examined\":" << l.examined
      << ",\"meb_calls\":" << l.meb_calls << ",\"meb\":{\"presentations\":" << l.meb.presentations
      << ",\"nondegenerate\":" << l.meb.nondegenerate << ",\"positive\":" << l.meb.positive
      << ",\"containing\":" << l.meb.containing << ",\"comparisons\":" << l.meb.comparisons
      << ",\"point_tests\":" << l.meb.point_tests << "}}";
}
Result<std::string> payload(int kmax, std::span<const u32> x, std::span<const u32> y, std::span<const u32> z,
                            std::span<const PointId> ids, MemoryBudget& owner) {
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth{}, owner);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), IndexParams{2}, owner);
  if (!index.ok()) return index.outcome();
  CatalogueParams params; params.kmax = kmax;
  auto made = prepare_full_domain(std::move(index.value()), params, owner);
  if (!made.ok()) return made.outcome();
  const auto& domain = made.value(); const auto& cat = domain.catalogue();
  std::ostringstream out; out << '[';
  bool comma = false;
  for (u32 b = 0; b < cat.balls(); ++b) {
    const auto& data = cat.balls_data()[b];
    const u32 lo = data.p + data.qmin - 1, hi = std::min<u32>(data.p + data.m, cat.kmax());
    for (u32 k = lo; k <= hi; ++k) {
      auto classified = classify_cell(domain, BallIdx{b}, static_cast<Order>(k));
      if (!classified.ok()) return classified.outcome();
      out << (comma ? "," : "") << "{\"ball\":" << b << ",\"order\":" << k
          << ",\"qmin\":" << unsigned{data.qmin} << ",\"kind\":\""
          << (classified.value().kind() == CellKind::birth ? "birth" : "strict_traces") << "\",\"ledger\":";
      ledger(out, classified.value().ledger()); out << '}'; comma = true;
    }
  }
  out << ']'; return out.str();
}
void execute(int kmax, std::span<const u32> x, std::span<const u32> y, std::span<const u32> z,
             std::span<const PointId> ids) {
  MemoryBudget owner(MemoryBudget::kUnlimited);
  auto answer = guarded([&] { return payload(kmax, x, y, z, ids, owner); });
  const auto issue = merge(answer.outcome(), owner.released());
  std::cout << "{\"status\":\"" << status_name(issue.status()) << "\",\"reason\":\"" << reason_name(issue.reason)
            << "\",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << kmax << ",\"owner_after\":" << owner.used()
            << ",\"classifications\":" << (issue.ok() ? answer.value() : "null") << "}\n";
}
}  // namespace
int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--profile") {
    std::cout << "{\"coord_bits\":" << kCoordBits << "}\n"; return 0;
  }
  if (argc != 1) return 2;
  try {
    while (std::cin >> std::ws && std::cin.peek() != std::char_traits<char>::eof()) {
      int kmax = 0; u32 n = 0;
      if (!read(kmax) || !read(n) || n > 14) return 2;
      std::vector<u32> x(n), y(n), z(n); std::vector<PointId> ids(n);
      for (u32 i = 0; i < n; ++i) {
        u32 id = 0;
        if (!read(x[i]) || !read(y[i]) || !read(z[i]) || !read(id)) return 2;
        ids[i] = PointId{id};
      }
      execute(kmax, x, y, z, ids);
    }
  } catch (const std::bad_alloc&) { return 2; }
  return std::cin.eof() ? 0 : 2;
}
