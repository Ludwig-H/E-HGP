// Sonde bornee : mode feuille seuil budget_index budget_requete n m, n XYZ/PointId puis m SiteIdx Morton.
// Mode0 MEB seule, mode1 MEB+census. Budgets produit separes ; vecteurs et JSON appartiennent au harnais.
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "tower/tower.hpp"

using namespace mhgp11;
namespace {
struct Request {
  u32 mode = 0, threshold = 0;
  IndexParams params;
  u64 index_budget = 0, query_budget = 0;
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  std::vector<SiteIdx> part;
};

template <class T>
bool number(std::string_view word, T& value) {
  if (word.empty()) return false;
  const auto parsed = std::from_chars(word.data(), word.data() + word.size(), value);
  return parsed.ec == std::errc{} && parsed.ptr == word.data() + word.size();
}
template <class T>
bool read(T& value) {
  std::string word;
  return bool(std::cin >> word) && number(word, value);
}

bool request(std::string_view first, Request& req) {
  u64 n = 0, m = 0;
  if (!number(first, req.mode) || !read(req.params.leaf_size) || !read(req.threshold) ||
      !read(req.index_budget) || !read(req.query_budget) || !read(n) || !read(m)) return false;
  if (req.mode > 1 || n > 10000 || m > 1024) return false;
  req.x.resize(n); req.y.resize(n); req.z.resize(n); req.ids.resize(n); req.part.resize(m);
  for (u64 i = 0; i < n; ++i) {
    u32 id = 0;
    if (!read(req.x[i]) || !read(req.y[i]) || !read(req.z[i]) || !read(id)) return false;
    req.ids[i] = PointId{id};
  }
  for (u64 i = 0; i < m; ++i) {
    u32 id = 0;
    if (!read(id)) return false;
    req.part[i] = SiteIdx{id};
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
void indices(std::ostream& out, std::span<const Id> values) {
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) out << (i == 0 ? "" : ",") << idx(values[i]);
  out << ']';
}
void cloud_json(std::ostream& out, const Cloud& cloud) {
  out << "\"sites\":[";
  for (u32 i = 0; i < cloud.sites(); ++i) {
    if (i != 0) out << ',';
    out << '[' << cloud.x()[i] << ',' << cloud.y()[i] << ',' << cloud.z()[i] << ']';
  }
  out << "],\"site_ids\":[";
  for (u32 i = 0; i < cloud.sites(); ++i) {
    if (i != 0) out << ',';
    indices(out, cloud.points(SiteIdx{i}));
  }
  out << ']';
}
void meb_json(std::ostream& out, const BoundedMeb& meb) {
  const auto& s = meb.sphere(); const auto a = s.anchor(); const auto& l = meb.ledger();
  out << ",\"meb\":{\"support\":";
  indices(out, meb.support());
  out << ",\"anchor\":[" << a.x() << ',' << a.y() << ',' << a.z() << "],\"N\":[";
  for (unsigned i = 0; i < 3; ++i) out << (i == 0 ? "" : ",") << '"' << hex(s.numerator()[i]) << '"';
  out << "],\"D\":\"" << hex(s.denominator()) << "\",\"level\":[\"" << hex(s.level().numerator())
      << "\",\"" << hex(s.level().denominator()) << "\"],\"arity\":" << unsigned{s.presentation_arity()}
      << ",\"ledger\":{\"presentations\":" << l.presentations << ",\"nondegenerate\":" << l.nondegenerate
      << ",\"positive\":" << l.positive << ",\"containing\":" << l.containing
      << ",\"comparisons\":" << l.comparisons << ",\"point_tests\":" << l.point_tests << ",\"diameter_pairs\":" << l.diameter_pairs << "}}";
}
void census_json(std::ostream& out, const Census& census) {
  const auto& l = census.ledger();
  out << ",\"census\":{\"kind\":\"" << (census.kind() == CensusKind::saturated ? "saturated" : "complete")
      << "\",\"inner\":";
  indices(out, census.interior());
  out << ",\"shell\":";
  indices(out, census.shell());
  out << ",\"ledger\":{\"nodes\":" << l.nodes << ",\"bounds\":" << l.bounds << ",\"point_tests\":" << l.point_tests
      << ",\"inside_blocks\":" << l.inside_blocks << ",\"outside_blocks\":" << l.outside_blocks
      << ",\"passes\":" << l.passes << "}}";
}

Result<std::string> payload(const Request& req, MemoryBudget& cloud_budget,
                            MemoryBudget& index_budget, MemoryBudget& query_budget) {
  auto cloud = prepare_cloud(req.x, req.y, req.z, req.ids, CoordWidth{}, cloud_budget);
  if (!cloud.ok()) return cloud.outcome();
  std::ostringstream out;
  if (req.mode == 0) {
    auto meb = bounded_meb(cloud.value(), req.part);
    if (!meb.ok()) return meb.outcome();
    cloud_json(out, cloud.value()); meb_json(out, meb.value()); out << ",\"census\":null";
  } else {
    auto index = build_index(std::move(cloud.value()), req.params, index_budget);
    if (!index.ok()) return index.outcome();
    auto result = meb_census(index.value(), req.part, req.threshold, query_budget);
    if (!result.ok()) return result.outcome();
    cloud_json(out, index.value().cloud()); meb_json(out, result.value().meb()); census_json(out, result.value().population());
  }
  return out.str();
}
void memory(std::string_view name, const MemoryBudget& budget) {
  std::cout << ",\"" << name << "_memory\":{\"before\":0,\"after\":" << budget.used()
            << ",\"peak\":" << budget.peak() << '}';
}
void execute(const Request& req) {
  MemoryBudget cloud_budget(MemoryBudget::kUnlimited), index_budget(req.index_budget), query_budget(req.query_budget);
  auto answer = guarded([&]() { return payload(req, cloud_budget, index_budget, query_budget); });
  const auto issue = merge(answer.outcome(), merge(cloud_budget.released(),
                           merge(index_budget.released(), query_budget.released())));
  std::cout << "{\"status\":\"" << status_name(issue.status()) << "\",\"reason\":\"" << reason_name(issue.reason)
            << "\",\"coord_bits\":" << kCoordBits << ",\"mode\":" << req.mode << ",\"threshold\":" << req.threshold;
  memory("index", index_budget); memory("query", query_budget);
  std::cout << ',';
  if (issue.ok()) std::cout << answer.value();
  else std::cout << "\"sites\":[],\"site_ids\":[],\"meb\":null,\"census\":null";
  std::cout << "}\n";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--profile") {
    std::cout << "{\"coord_bits\":" << kCoordBits << "}\n";
    return 0;
  }
  if (argc != 1) return 2;
  try {
    std::string first;
    while (std::cin >> first) {
      Request req;
      if (!request(first, req)) return 2;
      execute(req);
    }
  } catch (const std::bad_alloc&) { return 2; }
  return std::cin.eof() ? 0 : 2;
}
