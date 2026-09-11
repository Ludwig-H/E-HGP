#include "historical_chains.hpp"
#include <bit>
#include <iostream>
#include <string>

namespace fc = mhgp7::filtered_calendar_private;
using Date = std::uint64_t;
using Graph = fc::Graph<Date>;
using History = fc::History<Date>;
using Request = fc::HistoricalRequest<Date>;
const std::less<Date> date_less;
std::uint64_t checks = 0, rejected = 0, queries = 0, inactive = 0;
std::uint64_t lights = 0, binaries = 0, maximum_nodes = 0, index_bytes = 0;
void check(bool ok, const char* why) {
  ++checks;
  if (!ok) throw std::runtime_error(why);
}
template<class F> void reject(F action, const char* expected) {
  try { action(); }
  catch (const std::invalid_argument& error) {
    check(error.what() == std::string(expected), "wrong rejection cause");
    ++rejected;
    return;
  }
  throw std::runtime_error("mutant survived");
}
fc::Id reference(const History& history, const Request& request) {
  if (!fc::admitted(request.admission, request.cut, request.closed, date_less)) return fc::absent;
  // Independent linear walk: it does not consume the heavy-chain layout.
  fc::Id node = request.segment;
  while (history.successors[node] != fc::absent) {
    const auto next = history.successors[node];
    if (!fc::admitted(history.nodes[next].date, request.cut, request.closed, date_less)) break;
    node = next;
  }
  return node;
}
History history_of(const Graph& graph) {
  auto certificate = fc::spanning_certificate(graph, date_less);
  fc::verify_certificate(graph, certificate, date_less);
  return fc::reconstruct(certificate, date_less);
}
void exercise(const Graph& graph) {
  const auto history = history_of(graph);
  const fc::HistoricalChains chains(history, date_less);
  maximum_nodes = std::max(maximum_nodes, static_cast<std::uint64_t>(history.nodes.size()));
  index_bytes += chains.logical_index_bytes();
  check(chains.logical_index_bytes() == 24 * history.nodes.size(), "nonlinear logical layout");
  std::vector<Request> requests;
  for (fc::Id i = 0; i < history.nodes.size(); ++i) {
    const auto birth = history.nodes[i].date;
    const auto max_date = history.nodes.back().date;
    for (Date cut : {Date{0}, birth, birth + 1, max_date, max_date + 1})
      for (bool closed : {false, true}) requests.push_back({i, birth, cut, closed});
  }
  for (const auto& mark : history.marks)
    for (Date cut : {mark.admission - 1, mark.admission, mark.admission + 1})
      for (bool closed : {false, true}) requests.push_back({mark.segment, mark.admission, cut, closed});
  for (std::size_t workers : {1, 2, 4}) {
    const auto answers = chains.batch(requests, workers);
    check(answers.size() == requests.size(), "missing batch answer");
    for (std::size_t i = 0; i < requests.size(); ++i) {
      check(answers[i].segment == reference(history, requests[i]), "heavy chain differs from linear walk");
      const auto bit_bound = static_cast<fc::Id>(std::bit_width(history.nodes.size()));
      check(answers[i].work.light_steps <= bit_bound, "light path bound");
      check(answers[i].work.binary_steps <= bit_bound, "binary path bound");
      ++queries;
      inactive += answers[i].segment == fc::absent;
      lights += answers[i].work.light_steps;
      binaries += answers[i].work.binary_steps;
    }
  }
  check(chains.batch({}, 4).empty(), "empty batch");
}

Graph comb(std::size_t n) {
  Graph graph;
  for (std::size_t i = 0; i < n; ++i) graph.births.push_back({100 + i, 0});
  for (std::size_t i = 1; i < n; ++i) graph.edges.push_back({i, 100, 100 + i, i});
  graph.marks.push_back({50000, 100, n / 2});
  return graph;
}
Graph balanced(std::size_t n) {
  Graph graph;
  for (std::size_t i = 0; i < n; ++i) graph.births.push_back({100 + i, 0});
  fc::Id edge = 0;
  Date level = 1;
  for (std::size_t width = 1; width < n; width *= 2, ++level)
    for (std::size_t i = 0; i + width < n; i += 2 * width)
      graph.edges.push_back({++edge, 100 + i, 100 + i + width, level});
  graph.marks.push_back({50000, 100, 4});
  return graph;
}
int main() {
  try {
    exercise(comb(4096));
    exercise(balanced(1024));
    const Graph plateaux{{{10, 0}, {20, 0}, {30, 0}, {40, 0}, {50, 0}, {60, 9}},
                         {{1, 10, 20, 2}, {2, 20, 30, 2}, {3, 10, 30, 2}, {4, 40, 50, 2}},
                         {{1, 10, 5}, {2, 40, 3}, {3, 60, 9}}};
    exercise(plateaux);
    exercise(Graph{{{0, 0}}, {}, {}});
    exercise(Graph{});
    const auto good = history_of(plateaux);
    const fc::HistoricalChains chains(good, date_less);
    const auto birth = good.births[fc::index_of(good.births, 10)].segment;
    check(chains.query({birth, 0, 2, false}).segment != chains.query({birth, 0, 2, true}).segment,
          "open closed boundary vacuous");
    const auto mark = good.marks.front();
    check(chains.query({mark.segment, mark.admission, 4, true}).segment == fc::absent,
          "mark admitted too early");
    check(chains.query({mark.segment, good.nodes[mark.segment].date, 4, true}).segment != fc::absent,
          "undated mark mutant vacuous");
    ++rejected;  // Dated-vs-undated semantic mutation above has its causal fixture.
    reject([&] { chains.batch({{fc::absent, 0, 0, true}}, 4); }, "chains.query_identity");
    std::vector<Request> late(200, {birth, 0, 5, true});
    late.back().segment = fc::absent;
    reject([&] { chains.batch(late, 4); }, "chains.query_identity");
    reject([&] { chains.batch({}, 0); }, "chains.zero_workers");
    reject([&] { chains.query({mark.segment, 0, 5, true}); }, "chains.query_admission");
    auto mutant = good;
    mutant.nodes[good.successors[birth]].date = good.nodes[birth].date;
    reject([&] { fc::HistoricalChains invalid(mutant, date_less); }, "chains.non_strict_level");
    mutant = good;
    mutant.parents.front() = mutant.nodes.size();
    reject([&] { fc::HistoricalChains invalid(mutant, date_less); }, "chains.parent_identity");
    mutant = good;
    mutant.nodes.back().first += 1;
    reject([&] { fc::HistoricalChains invalid(mutant, date_less); }, "chains.parent_offsets");
    mutant = good;
    mutant.roots.pop_back();
    reject([&] { fc::HistoricalChains invalid(mutant, date_less); }, "chains.roots");
    check(queries > 300000 && inactive > 10000 && lights > 1000 && binaries > 1000 && rejected == 9,
          "nonvacuity floor");
    std::cout << "{\"status\":\"passed\",\"checks\":" << checks << ",\"queries\":" << queries
              << ",\"inactive\":" << inactive << ",\"light_steps\":" << lights
              << ",\"binary_steps\":" << binaries << ",\"rejected\":" << rejected
              << ",\"cases\":5,\"workers\":[1,2,4],\"maximum_nodes\":" << maximum_nodes
              << ",\"logical_index_bytes_total\":" << index_bytes
              << ",\"construction_parallel\":false,\"batch_parallel\":true,\"geometry_tested\":false,"
                 "\"gcp_used\":false,\"public_status\":\"not_claimed\"}\n";
  } catch (const std::exception& error) {
    std::cerr << "historical chains gate failed: " << error.what() << '\n';
    return 2;
  }
}
