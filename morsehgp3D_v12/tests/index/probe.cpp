// Pilote de test en lots : feuille seuil budget_index budget_requete n q, puis n XYZ/PointId et q XYZ supports.
// Cloud, index et resultats ont des budgets separes ; entrees et JSON appartiennent au harnais.
// Option --guarded (v12, NUM-GARDE) : le support est d'abord certifie (num::CertifiedBall) ; certifie, le census
// garde remplace le census generique ; non certifie, la reponse est {"status":"uncertified",...} sans census.
#include <array>
#include <charconv>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "index/index.hpp"

using namespace mhgp12;

namespace {
struct Request {
  IndexParams params;
  u32 threshold = 0;
  u64 index_budget = 0, query_budget = 0;
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  std::array<num::Point, 4> support{};
  unsigned arity = 0;
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
  u64 n = 0;
  if (!number(first, req.params.leaf_size) || !read(req.threshold) || !read(req.index_budget) ||
      !read(req.query_budget) || !read(n) || !read(req.arity)) return false;
  if (n > 10000 || req.arity < 1 || req.arity > 4) return false;
  req.x.resize(n); req.y.resize(n); req.z.resize(n); req.ids.resize(n);
  for (u64 i = 0; i < n; ++i) {
    u32 id = 0;
    if (!read(req.x[i]) || !read(req.y[i]) || !read(req.z[i]) || !read(id)) return false;
    req.ids[i] = make_id<PointId>(id);
  }
  for (unsigned i = 0; i < req.arity; ++i) {
    i64 x = 0, y = 0, z = 0;
    if (!read(x) || !read(y) || !read(z)) return false;
    const auto point = num::Point::make(x, y, z);
    if (!point.ok()) return false;
    req.support[i] = point.value();
  }
  return true;
}

bool guarded_mode = false;

Result<std::optional<num::CertifiedBall>> certified(const Request& req) {
  return num::CertifiedBall::certify(std::span<const num::Point>(req.support.data(), req.arity));
}

Result<std::optional<num::Sphere>> sphere(const Request& req) {
  const auto& p = req.support;
  if (req.arity == 1) return std::optional<num::Sphere>{num::Sphere::point(p[0])};
  if (req.arity == 2) return num::Sphere::through(p[0], p[1]);
  if (req.arity == 3) return num::Sphere::through(p[0], p[1], p[2]);
  return num::Sphere::through(p[0], p[1], p[2], p[3]);
}

template <StrongId Id>
void indices(std::ostream& out, std::span<const Id> values) {
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ',';
    out << idx(values[i]);
  }
  out << ']';
}

Result<std::string> payload(const Request& req, MemoryBudget& cloud_budget,
                            MemoryBudget& index_budget, MemoryBudget& query_budget) {
  auto cloud = prepare_cloud(req.x, req.y, req.z, req.ids, CoordWidth{}, cloud_budget);
  if (!cloud.ok()) return cloud.outcome();
  auto index = build_index(std::move(cloud.value()), req.params, index_budget);
  if (!index.ok()) return index.outcome();
  const auto ball = sphere(req);
  if (!ball.ok()) return ball.outcome();
  if (!ball.value()) return fail(Reason::parameter_out_of_range);
  std::optional<num::CertifiedBall> certified_ball;
  if (guarded_mode) {
    auto made = certified(req);
    if (!made.ok()) return made.outcome();
    certified_ball = made.value();
  }
  auto result = certified_ball ? census(index.value(), *certified_ball, req.threshold, query_budget)
                               : census(index.value(), *ball.value(), req.threshold, query_budget);
  if (!result.ok()) return result.outcome();
  const auto& owner = index.value().cloud();
  std::ostringstream out;
  out << "\"sites\":[";
  for (u32 i = 0; i < owner.sites(); ++i) {
    if (i != 0) out << ',';
    out << '[' << owner.x()[i] << ',' << owner.y()[i] << ',' << owner.z()[i] << ']';
  }
  out << "],\"site_ids\":[";
  for (u32 i = 0; i < owner.sites(); ++i) {
    if (i != 0) out << ',';
    indices(out, owner.points(make_id<SiteIdx>(i)));
  }
  const auto& value = result.value();
  out << "],\"kind\":\"" << (value.kind() == CensusKind::saturated ? "saturated" : "complete")
      << "\",\"inner\":";
  indices(out, value.interior());
  out << ",\"shell\":";
  indices(out, value.shell());
  const auto& l = value.ledger();
  out << ",\"ledger\":{\"nodes\":" << l.nodes << ",\"bounds\":" << l.bounds
      << ",\"point_tests\":" << l.point_tests << ",\"inside_blocks\":" << l.inside_blocks
      << ",\"outside_blocks\":" << l.outside_blocks << ",\"passes\":" << l.passes
      << "},\"index_nodes\":" << index.value().nodes() << ",\"index_depth\":" << index.value().max_depth()
      << ",\"lanes\":{\"native\":" << l.lanes.native << ",\"certified\":" << l.lanes.certified
      << ",\"checked\":" << l.lanes.checked << ",\"wide\":" << l.lanes.wide << "},\"guard\":{\"disjoint\":"
      << l.guard_disjoint << ",\"partial\":" << l.guard_partial << ",\"outside\":" << l.guard_outside << '}';
  return out.str();
}

void memory(std::string_view name, const MemoryBudget& budget) {
  std::cout << ",\"" << name << "_memory\":{\"before\":0,\"after\":" << budget.used()
            << ",\"peak\":" << budget.peak() << '}';
}

void execute(const Request& req) {
  if (guarded_mode) {
    const auto ball = certified(req);
    if (ball.ok() && !ball.value()) {
      std::cout << "{\"status\":\"uncertified\",\"coord_bits\":" << kCoordBits << ",\"threshold\":" << req.threshold
                << "}\n";
      return;
    }
  }
  MemoryBudget cloud_budget(MemoryBudget::kUnlimited), index_budget(req.index_budget), query_budget(req.query_budget);
  auto answer = guarded([&]() { return payload(req, cloud_budget, index_budget, query_budget); });
  const auto issue = merge(answer.outcome(), merge(cloud_budget.released(),
                           merge(index_budget.released(), query_budget.released())));
  std::cout << "{\"status\":\"" << status_name(issue.status()) << "\",\"reason\":\"" << reason_name(issue.reason)
            << "\",\"coord_bits\":" << kCoordBits << ",\"threshold\":" << req.threshold;
  memory("index", index_budget); memory("query", query_budget);
  std::cout << ',';
  if (issue.ok()) std::cout << answer.value();
  else std::cout << "\"sites\":[],\"site_ids\":[],\"kind\":\"refused\",\"inner\":[],\"shell\":[],\"ledger\":{}";
  std::cout << "}\n";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--profile") {
    std::cout << "{\"coord_bits\":" << kCoordBits << "}\n";
    return 0;
  }
  if (argc == 2 && std::string_view(argv[1]) == "--guarded") guarded_mode = true;
  else if (argc != 1) return 2;
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
