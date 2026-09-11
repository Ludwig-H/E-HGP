#include "fused_marks.hpp"
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <string_view>

namespace gate {
namespace fc = mhgp7::filtered_calendar_private;
namespace fused = mhgp7::fused_marks_audit;
using Id = fc::Id;
struct Date {
  int rank = 0, raw_tag = 0;
  bool operator==(const Date&) const = default;
};
struct Less { bool operator()(const Date& a, const Date& b) const { return a.rank < b.rank; } };
using Graph = fc::Graph<Date>;
using Certificate = fc::Certificate<Date>;
using History = fc::History<Date>;
Date date(int rank, int tag = 0) { return {2 * rank, tag}; }
void need(bool condition, const char* reason) {
  if (!condition) throw std::runtime_error(reason);
}
struct Counts {
  Id cases = 0, nodes = 0, marks = 0, mark_queries = 0, union_attempts = 0;
  Id avoided_replay_union_attempts = 0, removed_logical_workspace_bytes = 0;
  Id mark_find_queries = 0, event_dates = 0, raw_tie_fixtures = 0;
} counts;

void same(const History& a, const History& b) {
  need(a.parents == b.parents && a.successors == b.successors && a.roots == b.roots,
       "history.topology");
  need(a.nodes.size() == b.nodes.size() && a.births.size() == b.births.size()
       && a.marks.size() == b.marks.size(), "history.shape");
  for (size_t i = 0; i < a.nodes.size(); ++i) {
    const auto& x = a.nodes[i]; const auto& y = b.nodes[i];
    need(x.date == y.date && x.first == y.first && x.parent_count == y.parent_count
         && x.least_birth == y.least_birth, "history.node_fields");
  }
  for (size_t i = 0; i < a.births.size(); ++i) {
    const auto& x = a.births[i]; const auto& y = b.births[i];
    need(x.id == y.id && x.segment == y.segment && x.admission == y.admission,
         "history.birth_fields");
  }
  for (size_t i = 0; i < a.marks.size(); ++i) {
    const auto& x = a.marks[i]; const auto& y = b.marks[i];
    need(x.id == y.id && x.representative == y.representative && x.segment == y.segment
         && x.admission == y.admission, "history.mark_fields");
  }
}

std::vector<Id> bfs(const Graph& graph, Id representative, Date cut, bool closed) {
  const auto active = [&](Date level) { return fc::admitted(level, cut, closed, Less{}); };
  std::map<Id, std::vector<Id>> adjacency;
  for (const auto& birth : graph.births) if (active(birth.date)) adjacency.emplace(birth.id, std::vector<Id>{});
  for (const auto& edge : graph.edges) if (active(edge.date)) {
    need(adjacency.contains(edge.a) && adjacency.contains(edge.b), "bfs.edge_domain");
    adjacency[edge.a].push_back(edge.b); adjacency[edge.b].push_back(edge.a);
  }
  need(adjacency.contains(representative), "bfs.inactive_representative");
  std::set<Id> seen{representative};
  std::vector<Id> pending{representative};
  while (!pending.empty()) {
    const Id current = pending.back(); pending.pop_back();
    for (Id neighbor : adjacency.at(current)) if (seen.insert(neighbor).second) pending.push_back(neighbor);
  }
  return {seen.begin(), seen.end()};
}

std::vector<std::vector<Id>> memberships(const History& history) {
  std::vector<std::vector<Id>> out;
  for (const auto& node : history.nodes) {
    std::vector<Id> members;
    if (node.parent_count == 0) members.push_back(node.least_birth);
    else for (Id j = 0; j < node.parent_count; ++j) {
      const auto& child = out.at(history.parents[node.first + j]);
      members.insert(members.end(), child.begin(), child.end());
    }
    std::sort(members.begin(), members.end());
    need(std::adjacent_find(members.begin(), members.end()) == members.end(), "members.duplicate_birth");
    out.push_back(std::move(members));
  }
  return out;
}

void oracle(const Graph& graph, const History& history) {
  std::set<int> levels;
  for (const auto& birth : graph.births) levels.insert(birth.date.rank);
  for (const auto& edge : graph.edges) levels.insert(edge.date.rank);
  for (const auto& mark : graph.marks) levels.insert(mark.admission.rank);
  if (levels.empty()) levels.insert(0);
  std::vector<int> values(levels.begin(), levels.end());
  levels.insert(values.front() - 1); levels.insert(values.back() + 1);
  for (size_t i = 1; i < values.size(); ++i) levels.insert((values[i - 1] + values[i]) / 2);
  const auto members = memberships(history);
  for (int rank : levels) for (bool closed : {false, true}) for (const auto& mark : graph.marks) {
    const Date cut{rank, -1};
    std::vector<Id> expected;
    if (fc::admitted(mark.admission, cut, closed, Less{})) expected = bfs(graph, mark.representative, cut, closed);
    const Id segment = fc::mark_component(history, mark.id, cut, closed, Less{});
    const std::vector<Id> actual = segment == fc::absent ? std::vector<Id>{} : members.at(segment);
    need(actual == expected, "oracle.mark_cut_component");
    ++counts.mark_queries;
  }
}

struct Fixture { const char* name; Graph graph; };
std::vector<Fixture> fixtures() {
  std::vector<Fixture> out{
    {"empty", {}},
    {"late_birth", {{{17, date(8, 71)}}, {}, {{900, 17, date(9, 72)}}}},
    {"disconnected", {{{5, date(0)}, {2, date(1)}, {9, date(10)}}, {},
                       {{3, 5, date(4)}, {1, 2, date(1)}, {7, 9, date(12)}}}},
    {"ternary_plateau", {{{1, date(0)}, {2, date(0)}, {3, date(0)}},
                        {{1, 1, 2, date(3)}, {2, 2, 3, date(3)}, {3, 1, 3, date(3)}},
                        {{50, 1, date(3)}, {51, 2, date(3)}, {52, 3, date(7)}}}},
    {"late_continuation", {{{1, date(0)}, {2, date(0)}, {3, date(9)}},
                          {{1, 1, 2, date(2)}},
                          {{70, 1, date(6)}, {71, 3, date(11)}, {72, 2, date(1)}}}},
    {"raw_equivalent_mark_edge_dates", {{{1, date(0, 11)}, {2, date(0, 12)}, {3, date(1, 13)}},
                                       {{7, 1, 2, date(3, 101)}, {8, 2, 3, date(3, 102)}},
                                       {{90, 1, date(3, 999)}, {91, 3, date(6, 888)}}}},
    {"unmarked_tail", {{{1, date(0)}, {2, date(0)}, {3, date(0)}, {4, date(12)}},
                       {{1, 1, 2, date(2)}, {2, 2, 3, date(5)}}, {{1, 1, date(1)}}}},
  };
  for (int recipe = 0; recipe < 8; ++recipe) {
    Graph graph;
    const int n = 3 + recipe % 4;
    for (int i = 0; i < n; ++i) graph.births.push_back({static_cast<Id>(10 + i), date(i % 2, 100 + i)});
    for (int i = 1; i < n; ++i)
      graph.edges.push_back({static_cast<Id>(i), static_cast<Id>(9 + i), static_cast<Id>(10 + i),
                             date(2 + (recipe + i) % 3, 200 + i)});
    graph.edges.push_back({100, 10, static_cast<Id>(9 + n), date(5, 250)});
    for (int i = 0; i < n; ++i) {
      const Id vertex = static_cast<Id>(10 + i);
      graph.marks.push_back({static_cast<Id>(2 * i), vertex, date(i % 2, 300 + i)});
      graph.marks.push_back({static_cast<Id>(2 * i + 1), vertex, date(2 + (recipe + i) % 6, 400 + i)});
    }
    graph.births.push_back({999, date(12, 500)});
    graph.marks.push_back({999, 999, date(13, 501)});
    out.push_back({"fixed_recipe", std::move(graph)});
  }
  return out;
}

void qualify(Graph graph, bool reverse) {
  if (reverse) {
    std::reverse(graph.births.begin(), graph.births.end());
    std::reverse(graph.edges.begin(), graph.edges.end());
    std::reverse(graph.marks.begin(), graph.marks.end());
  }
  const auto certificate = fc::spanning_certificate(graph, Less{});
  fc::verify_certificate(graph, certificate, Less{});
  const auto reference = fc::reconstruct(certificate, Less{});
  fused::Work work;
  const auto trial = fused::reconstruct(certificate, Less{}, work);
  same(reference, trial);
  oracle(graph, trial);
  need(work.union_attempts == certificate.edges.size() && work.mark_find_queries == certificate.marks.size(),
       "work.qualified_count");
  ++counts.cases; counts.nodes += trial.nodes.size(); counts.marks += trial.marks.size();
  counts.union_attempts += work.union_attempts; counts.mark_find_queries += work.mark_find_queries;
  counts.event_dates += work.event_dates;
  counts.removed_logical_workspace_bytes += (2 * sizeof(size_t) + sizeof(Id)) * certificate.births.size();
  if (!certificate.marks.empty()) {
    Date last = certificate.marks.front().admission;
    for (const auto& mark : certificate.marks) if (Less{}(last, mark.admission)) last = mark.admission;
    for (const auto& node : reference.nodes) if (!Less{}(last, node.date))
      counts.avoided_replay_union_attempts += node.parent_count;
  }
}

void targeted_rejections() {
  const auto f = fixtures();
  const auto certificate = fc::spanning_certificate(f[3].graph, Less{});
  const auto original = fc::reconstruct(certificate, Less{});
  for (auto fault : {fused::Fault::before_plateau, fused::Fault::birth_admission}) {
    fused::Work work;
    const auto wrong = fused::reconstruct(certificate, Less{}, work, fault);
    bool rejected = false;
    try { same(original, wrong); }
    catch (const std::runtime_error& error) {
      need(std::string_view(error.what()) == "history.mark_fields", "mutant.unexpected_cause");
      rejected = true;
    }
    need(rejected, "mutant.survived");
    if (fault == fused::Fault::before_plateau)
      need(wrong.marks.front().segment != original.marks.front().segment, "mutant.before_plateau_nonvacuity");
    else need(fc::admitted(wrong.marks.front().admission, date(1), true, Less{}) &&
              !fc::admitted(original.marks.front().admission, date(1), true, Less{}),
              "mutant.birth_admission_nonvacuity");
  }
  const auto reject = [&](Certificate invalid, const char* expected) {
    for (bool use_fused : {false, true}) {
      bool rejected = false;
      try {
        if (use_fused) { fused::Work work; (void)fused::reconstruct(invalid, Less{}, work); }
        else (void)fc::reconstruct(invalid, Less{});
      } catch (const std::invalid_argument& error) {
        need(std::string_view(error.what()) == expected, "guard.unexpected_cause"); rejected = true;
      }
      need(rejected, "guard.survived");
    }
  };
  auto invalid = certificate;
  invalid.marks.front().representative = 9999;
  reject(invalid, "filtered.unknown_identity");
  invalid = certificate; invalid.marks.front().admission = date(-1);
  reject(invalid, "filtered.mark_before_representative");
  invalid = certificate; invalid.edges.front().date = date(0);
  reject(invalid, "filtered.edge_not_strictly_after_birth");
  invalid = certificate; invalid.edges.push_back({888, 1, 3, date(3)});
  reject(invalid, "filtered.certificate_cycle");
  invalid = certificate; invalid.marks.push_back(invalid.marks.front());
  reject(invalid, "filtered.duplicate_or_reserved_identity");
}
}  // namespace gate

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    for (const auto& fixture : gate::fixtures()) for (bool reverse : {false, true}) {
      gate::qualify(fixture.graph, reverse);
      if (std::string_view(fixture.name) == "raw_equivalent_mark_edge_dates") ++gate::counts.raw_tie_fixtures;
    }
    gate::targeted_rejections();
    const auto& c = gate::counts;
    gate::need(c.cases == 30 && c.mark_queries > 1000 && c.raw_tie_fixtures == 2 &&
               c.avoided_replay_union_attempts > 0, "gate.nonvacuity");
    std::printf("{\"status\":\"passed_bounded_fused_marks_model\",\"runs\":%llu,\"nodes\":%llu,"
        "\"marks\":%llu,\"BFS_mark_cut_comparisons\":%llu,\"first_sweep_union_attempts\":%llu,"
        "\"removed_replay_union_attempts\":%llu,\"mark_find_queries\":%llu,\"event_dates\":%llu,"
        "\"removed_logical_workspace_bytes_sum\":%llu,\"raw_equivalent_date_runs\":%llu,"
        "\"physical_mutants_rejected\":2,\"shared_input_guards_rejected\":5,"
        "\"scope\":\"synthetic exact-preorder dates; no 3D census or timing claim\","
        "\"workspace_scope\":\"removed Dsu parent/size and active_segment, logical sizes not RSS; other temporaries remain\"}\n",
        static_cast<unsigned long long>(c.cases), static_cast<unsigned long long>(c.nodes),
        static_cast<unsigned long long>(c.marks), static_cast<unsigned long long>(c.mark_queries),
        static_cast<unsigned long long>(c.union_attempts), static_cast<unsigned long long>(c.avoided_replay_union_attempts),
        static_cast<unsigned long long>(c.mark_find_queries), static_cast<unsigned long long>(c.event_dates),
        static_cast<unsigned long long>(c.removed_logical_workspace_bytes), static_cast<unsigned long long>(c.raw_tie_fixtures));
    return 0;
  } catch (const std::exception& error) { std::fprintf(stderr, "FAIL %s\n", error.what()); }
  return 1;
}
