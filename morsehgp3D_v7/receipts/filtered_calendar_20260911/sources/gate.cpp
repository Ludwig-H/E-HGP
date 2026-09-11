#include "filtered_calendar.hpp"

#include <cstdio>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <string>

namespace f = mhgp7::filtered_calendar_private;
using Id = f::Id;
struct Rational {
  std::uint64_t num = 0, den = 1;
  Rational() = default;
  Rational(std::uint64_t n, std::uint64_t d = 1) : num(n), den(d) {
    if (!d) throw std::invalid_argument("date.zero_denominator");
  }
};
struct Less {
  bool operator()(const Rational& a, const Rational& b) const {
    __extension__ using Wide = unsigned __int128;
    return static_cast<Wide>(a.num) * b.den < static_cast<Wide>(b.num) * a.den;
  }
};
// Oracle date comparison is Euclidean continued fractions, not the product
// cross product used above. No floating point and no graph DSU in the oracle.
bool oracle_less(Rational a, Rational b) {
  bool reverse = false;
  for (;;) {
    const auto qa = a.num / a.den, qb = b.num / b.den;
    if (qa != qb) return reverse ? qa > qb : qa < qb;
    const auto ra = a.num % a.den, rb = b.num % b.den;
    if (ra == 0 || rb == 0) return ra == rb ? false : (ra == 0 ? !reverse : reverse);
    a = {a.den, ra}; b = {b.den, rb}; reverse = !reverse;
  }
}
bool oracle_equal(const Rational& a, const Rational& b) { return !oracle_less(a, b) && !oracle_less(b, a); }
bool active(const Rational& date, const Rational& cut, bool closed) {
  return closed ? !oracle_less(cut, date) : oracle_less(date, cut);
}
using Graph = f::Graph<Rational>;
using History = f::History<Rational>;
using Certificate = f::Certificate<Rational>;
using Label = std::vector<Id>;
using Partition = std::vector<Label>;
std::uint64_t checks = 0, cases = 0, cuts = 0, events = 0, mark_reads = 0, rejects = 0;
std::uint64_t ternary = 0, native_births = 0, equal_level_merges = 0, input_edges = 0, forest_edges = 0;
std::uint64_t canonical_pairs = 0, calendar_mutants = 0;
void need(bool ok, const char* reason) {
  ++checks;
  if (!ok) throw std::runtime_error(reason);
}
template<class Function> void reject(Function&& function, const char* expected) {
  try { function(); }
  catch (const std::invalid_argument& error) {
    need(std::string(error.what()) == expected, "gate.rejection_cause"); ++rejects; return;
  }
  throw std::runtime_error("gate.missing_rejection");
}
Partition bfs(const Graph& graph, const Rational& cut, bool closed) {
  std::map<Id, Label> adjacent;
  for (const auto& vertex : graph.births) if (active(vertex.date, cut, closed)) adjacent[vertex.id];
  for (const auto& edge : graph.edges) if (active(edge.date, cut, closed)) {
    need(adjacent.contains(edge.a) && adjacent.contains(edge.b), "oracle.edge_endpoint_admitted");
    adjacent[edge.a].push_back(edge.b); adjacent[edge.b].push_back(edge.a);
  }
  std::set<Id> seen;
  Partition result;
  for (const auto& [start, unused] : adjacent) {
    (void)unused;
    if (seen.contains(start)) continue;
    Label stack{start}, component; seen.insert(start);
    while (!stack.empty()) {
      const Id vertex = stack.back(); stack.pop_back(); component.push_back(vertex);
      for (Id next : adjacent.at(vertex)) if (seen.insert(next).second) stack.push_back(next);
    }
    std::sort(component.begin(), component.end()); result.push_back(std::move(component));
  }
  std::sort(result.begin(), result.end()); return result;
}
Label label_for(const Partition& partition, Id vertex) {
  for (const auto& component : partition)
    if (std::binary_search(component.begin(), component.end(), vertex)) return component;
  return {};
}
Partition history_partition(const History& history, const Rational& cut, bool closed) {
  std::map<Id, Label> groups;
  for (const auto& vertex : history.births) {
    const Id segment = f::vertex_component(history, vertex.id, cut, closed, Less{});
    if (segment != f::absent) groups[segment].push_back(vertex.id);
  }
  Partition result;
  for (auto& [segment, component] : groups) { (void)segment; result.push_back(std::move(component)); }
  std::sort(result.begin(), result.end()); return result;
}
std::vector<Rational> event_dates(const Graph& graph) {
  std::vector<Rational> result;
  for (const auto& vertex : graph.births) result.push_back(vertex.date);
  for (const auto& edge : graph.edges) result.push_back(edge.date);
  for (const auto& mark : graph.marks) result.push_back(mark.admission);
  std::sort(result.begin(), result.end(), oracle_less);
  result.erase(std::unique(result.begin(), result.end(), oracle_equal), result.end());
  return result;
}
struct Event { Label label; Partition parents; };
bool same_events(std::vector<Event> a, std::vector<Event> b) {
  const auto order = [](const Event& x, const Event& y) { return x.label < y.label; };
  std::sort(a.begin(), a.end(), order); std::sort(b.begin(), b.end(), order);
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (a[i].label != b[i].label || a[i].parents != b[i].parents) return false;
  return true;
}
std::vector<Label> descendant_labels(const History& history) {
  std::vector<Label> labels;
  for (std::size_t n = 0; n < history.nodes.size(); ++n) {
    const auto& node = history.nodes[n];
    Label label;
    if (node.parent_count == 0) label.push_back(node.least_birth);
    else for (Id j = 0; j < node.parent_count; ++j) {
      const auto parent = history.parents[node.first + j];
      need(parent < n && oracle_less(history.nodes[parent].date, node.date), "oracle.no_zero_lifetime_parent");
      label.insert(label.end(), labels[parent].begin(), labels[parent].end());
    }
    std::sort(label.begin(), label.end());
    need(std::adjacent_find(label.begin(), label.end()) == label.end(), "oracle.distinct_birth_identities");
    labels.push_back(std::move(label));
  }
  return labels;
}
void check_events(const Graph& graph, const History& history, const std::vector<Rational>& dates) {
  const auto labels = descendant_labels(history);
  for (const auto& date : dates) {
    const auto before = bfs(graph, date, false), after = bfs(graph, date, true);
    std::vector<Event> expected, actual;
    for (const auto& component : after) {
      if (std::find(before.begin(), before.end(), component) != before.end()) continue;
      Partition parents;
      for (const auto& previous : before)
        if (std::includes(component.begin(), component.end(), previous.begin(), previous.end())) parents.push_back(previous);
      expected.push_back({component, parents});
    }
    for (std::size_t n = 0; n < history.nodes.size(); ++n) if (oracle_equal(history.nodes[n].date, date)) {
      const auto& node = history.nodes[n];
      Partition parents;
      for (Id j = 0; j < node.parent_count; ++j) parents.push_back(labels[history.parents[node.first + j]]);
      std::sort(parents.begin(), parents.end()); actual.push_back({labels[n], parents});
      ++events; native_births += parents.empty(); ternary += parents.size() >= 3;
    }
    need(same_events(expected, actual), "oracle.multifusion_parents_and_births");
    if (actual.size() >= 2) ++equal_level_merges;
  }
}
void qualify(Graph graph) {
  const auto certificate = f::spanning_certificate(graph, Less{});
  f::verify_certificate(graph, certificate, Less{});
  const auto history = f::reconstruct(certificate, Less{});
  const Graph sparse{certificate.births, certificate.edges, certificate.marks};
  const auto dates = event_dates(graph);
  for (const auto& date : dates) for (bool closed : {false, true}) {
    const auto expected = bfs(graph, date, closed);
    need(bfs(sparse, date, closed) == expected, "oracle.sparse_all_cuts");
    need(history_partition(history, date, closed) == expected, "oracle.history_all_cuts");
    for (const auto& mark : graph.marks) {
      const Id segment = f::mark_component(history, mark.id, date, closed, Less{});
      if (!active(mark.admission, date, closed)) need(segment == f::absent, "oracle.mark_admission_not_representative_birth");
      else {
        need(segment != f::absent, "oracle.active_mark");
        Label found;
        for (const auto& birth : history.births)
          if (f::vertex_component(history, birth.id, date, closed, Less{}) == segment) found.push_back(birth.id);
        need(found == label_for(expected, mark.representative), "oracle.mark_component_identity");
      }
      ++mark_reads;
    }
    ++cuts;
  }
  check_events(graph, history, dates);
  need(certificate.edges.size() + history.roots.size() == graph.births.size(), "oracle.sparse_edge_identity");
  need(history.nodes.size() <= 2 * graph.births.size(), "oracle.linear_node_bound");
  input_edges += graph.edges.size(); forest_edges += certificate.edges.size(); ++cases;
}
Graph triangle() {
  return {{{1, {0}}, {2, {0}}, {3, {0}}},
          {{11, 1, 2, {2, 2}}, {12, 2, 3, {1}}, {13, 1, 3, {4, 4}}},
          {{101, 1, {2}}, {102, 2, {1}}}};
}
void canonical_pair(const Graph& a, const Graph& b) {
  const auto x = f::reconstruct(f::spanning_certificate(a, Less{}), Less{});
  const auto y = f::reconstruct(f::spanning_certificate(b, Less{}), Less{});
  need(x.nodes.size() == y.nodes.size() && x.parents == y.parents && x.successors == y.successors &&
       x.roots == y.roots && x.births.size() == y.births.size() && x.marks.size() == y.marks.size(),
       "gate.canonical_physical_arenas");
  const auto physical_date = [](Rational p, Rational q) { return p.num == q.num && p.den == q.den; };
  for (std::size_t i = 0; i < x.nodes.size(); ++i)
    need(x.nodes[i].first == y.nodes[i].first && x.nodes[i].parent_count == y.nodes[i].parent_count &&
         x.nodes[i].least_birth == y.nodes[i].least_birth && physical_date(x.nodes[i].date, y.nodes[i].date),
         "gate.canonical_physical_nodes");
  for (std::size_t i = 0; i < x.births.size(); ++i)
    need(x.births[i].id == y.births[i].id && x.births[i].segment == y.births[i].segment &&
         physical_date(x.births[i].admission, y.births[i].admission), "gate.canonical_birth_anchors");
  for (std::size_t i = 0; i < x.marks.size(); ++i)
    need(x.marks[i].id == y.marks[i].id && x.marks[i].segment == y.marks[i].segment &&
         x.marks[i].representative == y.marks[i].representative &&
         physical_date(x.marks[i].admission, y.marks[i].admission), "gate.canonical_mark_anchors");
  ++canonical_pairs;
}
int main(int argc, char** argv) {
  if (argc != 2 || std::string(argv[1]) != "--selftest") return 2;
  try {
    qualify({});
    qualify({{{1, {0}}}, {}, {{7, 1, {5}}}});
    qualify({{{1, {0}}, {2, {0}}, {3, {5}}}, {}, {{7, 1, {4}}}});
    qualify(triangle());
    auto parallel = triangle();
    parallel.edges.push_back({14, 1, 2, {3}}); parallel.edges.push_back({15, 2, 2, {2}});
    qualify(parallel);
    qualify({{{1, {0}}, {2, {0}}, {3, {1}}, {4, {1}}, {5, {3}}},
             {{1, 1, 2, {2}}, {2, 3, 4, {2}}}, {{11, 1, {2}}, {12, 3, {2}}, {13, 5, {3}}}});
    qualify({{{1, {0}}, {2, {1}}, {3, {2}}, {4, {3}}},
             {{1, 1, 2, {2}}, {2, 1, 3, {3}}, {3, 1, 4, {4}}},
             {{101, 2, {2}}, {102, 3, {4}}, {103, 1, {9}}}});
    const auto high = std::numeric_limits<std::uint64_t>::max();
    qualify({{{high - 1, {high - 1, high}}, {high - 2, {high - 2, high - 1}}},
             {{high - 3, high - 1, high - 2, {high, high - 1}}},
             {{high - 4, high - 1, {high, high - 1}}}});
    std::mt19937_64 rng(20260911);
    for (unsigned sample = 0; sample < 128; ++sample) {
      Graph graph;
      const std::size_t n = 3 + rng() % 16;
      for (std::size_t i = 0; i < n; ++i) graph.births.push_back({(Id{1} << 40) + sample * 100 + i, {rng() % 7, 2}});
      for (std::size_t j = 0; j < 3 * n; ++j) {
        const auto a = rng() % n, b = rng() % n;
        const auto numerator = std::max(graph.births[a].date.num, graph.births[b].date.num) + 1 + rng() % 4;
        graph.edges.push_back({(Id{1} << 42) + j, graph.births[a].id, graph.births[b].id, {numerator, 2}});
      }
      for (std::size_t j = 0; j < 2 * n; ++j) {
        const auto vertex = rng() % n;
        graph.marks.push_back({(Id{1} << 44) + j, graph.births[vertex].id,
                               {graph.births[vertex].date.num + rng() % 10, 2}});
      }
      qualify(graph);
      const auto original = graph;
      std::reverse(graph.births.begin(), graph.births.end());
      std::reverse(graph.edges.begin(), graph.edges.end());
      std::reverse(graph.marks.begin(), graph.marks.end());
      qualify(graph);
      canonical_pair(original, graph);
    }
    auto base = triangle();
    auto certificate = f::spanning_certificate(base, Less{});
    const Certificate alternative{base.births, {base.edges[0], base.edges[1]}, base.marks};
    f::verify_certificate(base, alternative, Less{});
    check_events(base, f::reconstruct(alternative, Less{}), event_dates(base));
    auto delayed = f::reconstruct(certificate, Less{});
    delayed.nodes.back().date = {2};
    need(history_partition(delayed, {1}, true) != bfs(base, {1}, true), "gate.delayed_merge_mutant_refuted");
    ++calendar_mutants;
    auto binary = f::reconstruct(certificate, Less{});
    binary.nodes[3].parent_count = 2;
    binary.nodes.push_back({{1}, 3, 2, 1});
    binary.parents = {0, 1, 2, 3, 2};
    bool rejected_binary = false;
    try { (void)descendant_labels(binary); }
    catch (const std::runtime_error& error) { rejected_binary = std::string(error.what()) == "oracle.no_zero_lifetime_parent"; }
    need(rejected_binary, "gate.zero_lifetime_binary_mutant_refuted");
    ++calendar_mutants;
    const f::Graph<Id> ranked{{{1, 0}, {2, 0}, {3, 0}}, {{1, 1, 2, 1}, {2, 2, 3, 1}}, {{9, 1, 2}}};
    const auto rank_certificate = f::spanning_certificate(ranked, std::less<Id>{});
    f::verify_certificate(ranked, rank_certificate, std::less<Id>{});
    const auto rank_history = f::reconstruct(rank_certificate, std::less<Id>{});
    need(rank_history.nodes.size() == 4 && rank_history.nodes.back().parent_count == 3 &&
         f::mark_component(rank_history, 9, Id{1}, true, std::less<Id>{}) == f::absent &&
         f::mark_component(rank_history, 9, Id{2}, true, std::less<Id>{}) == 3, "gate.generic_certified_rank_carrier");
    auto bad = certificate; bad.edges.front().date = {3};
    reject([&] { f::verify_certificate(base, bad, Less{}); }, "filtered.edge_identity");
    bad = certificate; bad.edges.front().a = 3;
    reject([&] { f::verify_certificate(base, bad, Less{}); }, "filtered.edge_identity");
    bad = certificate; bad.edges.pop_back();
    reject([&] { f::verify_certificate(base, bad, Less{}); }, "filtered.cut_connectivity");
    bad = certificate; bad.edges = base.edges;
    reject([&] { f::verify_certificate(base, bad, Less{}); }, "filtered.certificate_cycle");
    bad = certificate; bad.births.front().date = {1, 2};
    reject([&] { f::verify_certificate(base, bad, Less{}); }, "filtered.birth_identity");
    bad = certificate; bad.marks.front().representative = 3;
    reject([&] { f::verify_certificate(base, bad, Less{}); }, "filtered.mark_identity");
    bad = certificate; bad.marks.front().admission = {1};
    reject([&] { f::verify_certificate(base, bad, Less{}); }, "filtered.mark_identity");
    auto late = base; late.edges.push_back({14, 1, 2, {5}});
    bad = {late.births, {late.edges[1], late.edges.back()}, late.marks};
    reject([&] { f::verify_certificate(late, bad, Less{}); }, "filtered.cut_connectivity");
    auto invalid = base; invalid.edges.front().date = {0};
    reject([&] { (void)f::spanning_certificate(invalid, Less{}); }, "filtered.edge_not_strictly_after_birth");
    invalid = base; invalid.births.front().date = {2};
    reject([&] { (void)f::spanning_certificate(invalid, Less{}); }, "filtered.edge_not_strictly_after_birth");
    invalid = base; invalid.edges.front().b = 99;
    reject([&] { (void)f::spanning_certificate(invalid, Less{}); }, "filtered.unknown_identity");
    invalid = base; invalid.births.push_back(invalid.births.front());
    reject([&] { (void)f::spanning_certificate(invalid, Less{}); }, "filtered.duplicate_or_reserved_identity");
    invalid = base; invalid.edges.push_back(invalid.edges.front());
    reject([&] { (void)f::spanning_certificate(invalid, Less{}); }, "filtered.duplicate_or_reserved_identity");
    invalid = base; invalid.marks.push_back(invalid.marks.front());
    reject([&] { (void)f::spanning_certificate(invalid, Less{}); }, "filtered.duplicate_or_reserved_identity");
    invalid = base; invalid.marks.front().representative = 99;
    reject([&] { (void)f::spanning_certificate(invalid, Less{}); }, "filtered.unknown_identity");
    invalid = base; invalid.births.front().date = {1, 2}; invalid.marks.front().admission = {0};
    reject([&] { (void)f::spanning_certificate(invalid, Less{}); }, "filtered.mark_before_representative");
    reject([] { (void)Rational(1, 0); }, "date.zero_denominator");
    need(cases == 264 && cuts > 4000 && mark_reads > 50000 && ternary > 100 && native_births > 1000
         && equal_level_merges > 100 && input_edges > forest_edges && rejects == 17 && canonical_pairs == 128 &&
         calendar_mutants == 2, "gate.nonvacuity");
    std::printf("{\"status\":\"passed\",\"scope\":\"abstract_native_birth_filtered_graph\","
        "\"cases\":%llu,\"checks\":%llu,\"cuts\":%llu,\"events\":%llu,\"mark_reads\":%llu,"
        "\"native_births\":%llu,\"multifusions_ge3\":%llu,\"multiple_events_same_date\":%llu,"
        "\"input_edges\":%llu,\"forest_edges\":%llu,\"rejections\":%llu,"
        "\"canonical_pairs\":%llu,\"calendar_mutants\":%llu,\"alternative_msf\":1,\"rank_instantiations\":1,"
        "\"geometry_qualified\":false,\"performance_claim\":false,\"GCP_used\":false}\n",
        static_cast<unsigned long long>(cases), static_cast<unsigned long long>(checks),
        static_cast<unsigned long long>(cuts), static_cast<unsigned long long>(events),
        static_cast<unsigned long long>(mark_reads), static_cast<unsigned long long>(native_births),
        static_cast<unsigned long long>(ternary), static_cast<unsigned long long>(equal_level_merges),
        static_cast<unsigned long long>(input_edges), static_cast<unsigned long long>(forest_edges),
        static_cast<unsigned long long>(rejects), static_cast<unsigned long long>(canonical_pairs),
        static_cast<unsigned long long>(calendar_mutants));
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL %s\n", error.what()); return 1;
  }
}
