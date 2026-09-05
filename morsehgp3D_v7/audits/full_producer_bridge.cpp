// Audit adapter. Inputs and expected Gamma partitions are supplied by Python.
// This process invokes the actual product headers pinned in its dependency file.
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/forest/full_gabriel.hpp"

namespace {
using namespace mhgp7;
constexpr u64 kRoom = 10000000;

template <typename T> T read() {
  T value{};
  if (!(std::cin >> value)) throw std::runtime_error("audit_truncated_input");
  return value;
}
ExactLevel read_level() {
  ExactLevel value{};
  for (auto& limb : value.num) limb = read<u64>();
  value.den = static_cast<i128>(read<u64>());
  return value;
}
template <typename T> void array(const std::vector<T>& values) {
  std::cout << '[';
  for (size_t i = 0; i < values.size(); ++i) {
    if (i) std::cout << ',';
    std::cout << values[i];
  }
  std::cout << ']';
}
void stats(const FullGabrielStats& s) {
  std::cout << '{';
#define FIELD(name) std::cout << "\"" #name "\":" << s.name << ','
  FIELD(input_records); FIELD(face_visits); FIELD(aliases); FIELD(alias_hits);
  FIELD(portal_requests); FIELD(chain_steps); FIELD(terminal_direct);
  FIELD(max_chain_length); FIELD(normalized_anchors); FIELD(successor_steps);
  FIELD(no_op_connections); FIELD(meb_calls);
#undef FIELD
  std::cout << "\"geometry\":{";
  const auto& g = s.geometry;
#define FIELD(name) std::cout << "\"" #name "\":" << g.name << ','
  FIELD(core_records); FIELD(core_facets); FIELD(facets_with_two_intruders);
  FIELD(chain_steps); FIELD(added_cofaces); FIELD(terminal_direct);
  FIELD(terminal_cached); FIELD(max_chain_length); FIELD(query_nodes);
  FIELD(query_leaves); FIELD(query_range_skips); FIELD(meb_calls);
#undef FIELD
  std::cout << "\"meb_supports\":" << g.meb_supports << "}}";
}
bool empty(const FullCertificate& f) {
  return f.order() == 0 && f.nodes().empty() && f.minima().empty() && f.parents().empty();
}
bool same(const FullCertificate& a, const FullCertificate& b) {
  if (a.order() != b.order() || a.nodes().size() != b.nodes().size() ||
      a.minima() != b.minima() || a.parents() != b.parents()) return false;
  for (size_t i = 0; i < a.nodes().size(); ++i) {
    const auto& x = a.nodes()[i];
    const auto& y = b.nodes()[i];
    if (!same_exact_level(x.level, y.level) || x.first != y.first || x.parent_count != y.parent_count)
      return false;
  }
  return true;
}
FullGabrielLimits roomy() {
  FullGabrielLimits c;
  c.certificate = {kRoom, kRoom, kRoom};
  c.max_points = c.max_input_records = c.max_aliases = kRoom;
  c.max_face_visits = c.max_portal_requests = c.max_chain_steps = kRoom;
  c.max_meb_calls = c.max_query_nodes = c.max_meb_supports = c.max_successor_steps = kRoom;
  return c;
}
std::vector<ForestEvent> catalogue(size_t count) {
  std::vector<ForestEvent> result;
  for (size_t i = 0; i < count; ++i) {
    ForestEvent e{};
    const unsigned q = read<unsigned>(), d = read<unsigned>();
    if (q > 4 || d > 9) throw std::runtime_error("audit_event_bound");
    e.q = static_cast<u8>(q);
    e.d = static_cast<u8>(d);
    e.active_mask = read<u16>();
    e.level = read_level();
    for (size_t j = 0; j < q; ++j) e.support[j] = read<PointId>();
    for (size_t j = 0; j < d; ++j) e.interior[j] = read<PointId>();
    result.push_back(e);
  }
  return result;
}
void one_record() {
  const u64 id = read<u64>();
  const size_t n = read<size_t>();
  const unsigned k = read<unsigned>();
  const size_t nm = read<size_t>(), nd = read<size_t>(), nc = read<size_t>();
  const bool probe = read<unsigned>() != 0;
  if (n > 10 || k > 10 || nm + nd > 2048 || nc > 4096)
    throw std::runtime_error("audit_record_bound");
  std::vector<InputPoint> points;
  for (size_t i = 0; i < n; ++i) {
    const PointId point = read<PointId>();
    const i32 x = read<i32>(), y = read<i32>(), z = read<i32>();
    points.push_back({point, P3{x, y, z}});
  }
  const auto minima = catalogue(nm), direct = catalogue(nd);
  const auto index = build_cloud_index(points);
  const auto out = build_full_gabriel_order(index, k, minima, direct, roomy());
  const auto& f = out.forest;
  std::cout << "{\"id\":" << id << ",\"status\":" << static_cast<int>(out.status)
            << ",\"reason\":\"" << out.reason << "\",\"stats\":";
  stats(out.stats);
  std::cout << ",\"order\":" << f.order() << ",\"nodes\":[";
  for (size_t i = 0; i < f.nodes().size(); ++i) {
    if (i) std::cout << ',';
    const auto& node = f.nodes()[i];
    std::cout << "{\"num\":[" << node.level.num[0] << ',' << node.level.num[1]
              << ',' << node.level.num[2] << "],\"den\":" << static_cast<u64>(node.level.den)
              << ",\"first\":" << node.first << ",\"parent_count\":" << node.parent_count << '}';
  }
  std::cout << "],\"minima\":[";
  for (size_t i = 0; i < f.minima().size(); ++i) {
    if (i) std::cout << ',';
    const auto& facet = f.minima()[i];
    array(std::vector<PointId>(facet.p.begin(), facet.p.begin() + facet.k));
  }
  std::cout << "],\"parents\":";
  array(f.parents());
  std::cout << ",\"coverage\":[";
  for (size_t i = 0; i < f.nodes().size(); ++i) {
    if (i) std::cout << ',';
    const auto covered = full_certificate_coverage(f, i, kRoom, kRoom);
    if (covered.status != FullCertificateStatus::kOk) throw std::runtime_error("audit_coverage_failed");
    array(covered.values);
  }
  std::cout << "],\"cuts\":[";
  for (size_t i = 0; i < nc; ++i) {
    const auto cut = read_level();
    const bool closed = read<unsigned>() != 0;
    const auto roots = full_certificate_roots_at(f, cut, closed, kRoom);
    if (i) std::cout << ',';
    std::cout << "{\"status\":" << static_cast<int>(roots.status)
              << ",\"reason\":\"" << roots.reason << "\",\"roots\":";
    array(roots.values);
    std::cout << '}';
  }
  std::cout << "],\"budget_trials\":[";
  if (probe && out.status == FullGabrielStatus::kCompleteRelative) {
    FullGabrielLimits caps;
    const auto& s = out.stats;
    caps.max_points = n;
    caps.max_input_records = s.input_records;
    caps.max_aliases = s.aliases;
    caps.max_face_visits = s.face_visits;
    caps.max_portal_requests = s.portal_requests;
    caps.max_chain_steps = s.chain_steps;
    caps.max_meb_calls = s.meb_calls;
    caps.max_query_nodes = s.geometry.query_nodes;
    caps.max_meb_supports = s.geometry.meb_supports;
    caps.max_successor_steps = s.successor_steps;
    caps.certificate.max_nodes = f.nodes().size();
    caps.certificate.max_parent_refs = f.parents().size();
    for (size_t i = 0; i < f.nodes().size(); ++i)
      if (i == 0 || !same_exact_level(f.nodes()[i - 1].level, f.nodes()[i].level))
        ++caps.certificate.max_batches;
    const auto trial = [&](const char* dimension, const char* kind, u64 cap) {
      const auto tested = build_full_gabriel_order(index, k, minima, direct, caps);
      std::cout << "{\"dimension\":\"" << dimension << "\",\"kind\":\"" << kind
                << "\",\"cap\":" << cap << ",\"status\":" << static_cast<int>(tested.status)
                << ",\"reason\":\"" << tested.reason << "\",\"empty\":"
                << (empty(tested.forest) ? "true" : "false") << ",\"same\":"
                << (same(f, tested.forest) ? "true" : "false") << ",\"stats\":";
      stats(tested.stats);
      std::cout << '}';
    };
    trial("all", "exact", 0);
    const auto under = [&](const char* dimension, u64& cap) {
      if (cap == 0) return;
      --cap;
      std::cout << ',';
      trial(dimension, "minus_one", cap);
      ++cap;
    };
    under("points", caps.max_points);
    under("input_records", caps.max_input_records);
    under("aliases", caps.max_aliases);
    under("face_visits", caps.max_face_visits);
    under("portal_requests", caps.max_portal_requests);
    under("chain_steps", caps.max_chain_steps);
    under("meb_calls", caps.max_meb_calls);
    under("query_nodes", caps.max_query_nodes);
    under("meb_supports", caps.max_meb_supports);
    under("successor_steps", caps.max_successor_steps);
    under("batches", caps.certificate.max_batches);
    under("nodes", caps.certificate.max_nodes);
    under("parent_refs", caps.certificate.max_parent_refs);
  }
  std::cout << "]}";
}
}  // namespace

int main() {
  try {
    if (read<std::string>() != "FULLPROD1") throw std::runtime_error("audit_magic");
    const size_t count = read<size_t>();
    if (count > 200) throw std::runtime_error("audit_records_bound");
    std::cout << "{\"records\":[";
    for (size_t i = 0; i < count; ++i) {
      if (i) std::cout << ',';
      one_record();
    }
    std::string trailing;
    if (std::cin >> trailing) throw std::runtime_error("audit_trailing_input");
    std::cout << "]}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
