#pragma once

// Structural reference only: graph of native births, not a geometric producer.
// Date/Less must supply a valid, exact total preorder (equal representations
// may differ). There is no floating-point conversion or date arithmetic here.
#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mhgp7::filtered_calendar_private {
using Id = std::uint64_t;
inline constexpr Id absent = std::numeric_limits<Id>::max();
inline void require(bool ok, const char* reason) {
  if (!ok) throw std::invalid_argument(reason);
}
template<class Date> struct Birth { Id id; Date date; };
template<class Date> struct Edge { Id id, a, b; Date date; };
template<class Date> struct Mark { Id id, representative; Date admission; };
template<class Date> struct Graph {
  std::vector<Birth<Date>> births;
  std::vector<Edge<Date>> edges;
  std::vector<Mark<Date>> marks;
};
template<class Date> struct Certificate {
  std::vector<Birth<Date>> births;
  std::vector<Edge<Date>> edges;
  std::vector<Mark<Date>> marks;
};
template<class Date> struct Node {
  Date date;
  Id first = 0, parent_count = 0, least_birth = absent;
};
template<class Date> struct VertexAnchor { Id id, segment; Date admission; };
template<class Date> struct MarkAnchor { Id id, representative, segment; Date admission; };
template<class Date> struct History {
  std::vector<Node<Date>> nodes;
  std::vector<Id> parents, successors, roots;
  std::vector<VertexAnchor<Date>> births;
  std::vector<MarkAnchor<Date>> marks;
};
template<class Date, class Less> bool equal_date(const Date& a, const Date& b, Less less) {
  return !less(a, b) && !less(b, a);
}
template<class Date, class Less> bool admitted(const Date& date, const Date& cut, bool closed, Less less) {
  return closed ? !less(cut, date) : less(date, cut);
}
template<class Row> std::size_t index_of(const std::vector<Row>& rows, Id id) {
  const auto found = std::lower_bound(rows.begin(), rows.end(), id,
      [](const auto& row, Id value) { return row.id < value; });
  require(found != rows.end() && found->id == id, "filtered.unknown_identity");
  return static_cast<std::size_t>(found - rows.begin());
}
template<class Rows> void sort_ids(Rows& rows) {
  std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
  for (std::size_t i = 0; i < rows.size(); ++i)
    require(rows[i].id != absent && (i == 0 || rows[i - 1].id != rows[i].id), "filtered.duplicate_or_reserved_identity");
}
struct Dsu {
  std::vector<std::size_t> parent, size;
  explicit Dsu(std::size_t n) : parent(n), size(n, 1) { std::iota(parent.begin(), parent.end(), 0); }
  std::size_t find(std::size_t a) {
    while (a != parent[a]) { parent[a] = parent[parent[a]]; a = parent[a]; }
    return a;
  }
  bool unite(std::size_t a, std::size_t b) {
    a = find(a); b = find(b);
    if (a == b) return false;
    if (size[a] < size[b]) std::swap(a, b);
    parent[b] = a; size[a] += size[b]; return true;
  }
};

template<class Date, class Less> Graph<Date> normalized(Graph<Date> graph, Less less) {
  sort_ids(graph.births); sort_ids(graph.edges); sort_ids(graph.marks);
  for (const auto& edge : graph.edges) {
    const auto a = index_of(graph.births, edge.a), b = index_of(graph.births, edge.b);
    // A reduced edge comes from a later consumer. Equal dates are NOT admitted:
    // hubs born and attached at one level must first become separate marks.
    require(less(graph.births[a].date, edge.date) && less(graph.births[b].date, edge.date),
            "filtered.edge_not_strictly_after_birth");
  }
  for (const auto& mark : graph.marks) {
    const auto vertex = index_of(graph.births, mark.representative);
    require(!less(mark.admission, graph.births[vertex].date), "filtered.mark_before_representative");
  }
  return graph;
}
template<class Date, class Less> void sort_edges(std::vector<Edge<Date>>& edges, Less less) {
  std::sort(edges.begin(), edges.end(), [&](const auto& a, const auto& b) {
    if (less(a.date, b.date)) return true;
    if (less(b.date, a.date)) return false;
    const auto aa = std::minmax(a.a, a.b), bb = std::minmax(b.a, b.b);
    if (aa != bb) return aa < bb;
    return a.id < b.id;
  });
}

template<class Date, class Less> Certificate<Date> spanning_certificate(const Graph<Date>& input, Less less) {
  auto graph = normalized(input, less);
  sort_edges(graph.edges, less);
  Certificate<Date> out{std::move(graph.births), {}, std::move(graph.marks)};
  Dsu dsu(out.births.size());
  for (const auto& edge : graph.edges)
    if (dsu.unite(index_of(out.births, edge.a), index_of(out.births, edge.b))) out.edges.push_back(edge);
  return out;
}

// Independent of the Kruskal tie choice: any input-edge forest passes if every
// original edge is connected in the forest by its own date. This verifies all
// open AND closed cuts, not just final connectivity or total edge count.
template<class Date, class Less> void verify_certificate(const Graph<Date>& input,
    const Certificate<Date>& certificate, Less less) {
  auto graph = normalized(input, less);
  auto forest = normalized(Graph<Date>{certificate.births, certificate.edges, certificate.marks}, less);
  require(graph.births.size() == forest.births.size() && graph.marks.size() == forest.marks.size(),
          "filtered.certificate_domain");
  for (std::size_t i = 0; i < graph.births.size(); ++i)
    require(graph.births[i].id == forest.births[i].id && equal_date(graph.births[i].date, forest.births[i].date, less),
            "filtered.birth_identity");
  for (std::size_t i = 0; i < graph.marks.size(); ++i)
    require(graph.marks[i].id == forest.marks[i].id && graph.marks[i].representative == forest.marks[i].representative &&
            equal_date(graph.marks[i].admission, forest.marks[i].admission, less), "filtered.mark_identity");
  Dsu acyclic(graph.births.size());
  for (const auto& edge : forest.edges) {
    const auto& original = graph.edges[index_of(graph.edges, edge.id)];
    require(edge.a == original.a && edge.b == original.b && equal_date(edge.date, original.date, less),
            "filtered.edge_identity");
    require(acyclic.unite(index_of(graph.births, edge.a), index_of(graph.births, edge.b)), "filtered.certificate_cycle");
  }
  sort_edges(graph.edges, less); sort_edges(forest.edges, less);
  Dsu threshold(graph.births.size());
  std::size_t next = 0;
  for (const auto& edge : graph.edges) {
    while (next < forest.edges.size() && !less(edge.date, forest.edges[next].date)) {
      const auto& added = forest.edges[next++];
      threshold.unite(index_of(graph.births, added.a), index_of(graph.births, added.b));
    }
    require(threshold.find(index_of(graph.births, edge.a)) == threshold.find(index_of(graph.births, edge.b)),
            "filtered.cut_connectivity");
  }
}

template<class Date, class Less> Id normalize_segment(const History<Date>& history, Id segment,
    const Date& cut, bool closed, Less less) {
  require(segment < history.nodes.size(), "filtered.segment_identity");
  require(admitted(history.nodes[segment].date, cut, closed, less), "filtered.segment_not_admitted");
  while (history.successors[segment] != absent) {
    const Id next = history.successors[segment];
    require(next > segment && next < history.nodes.size(), "filtered.successor_identity");
    if (!admitted(history.nodes[next].date, cut, closed, less)) break;
    segment = next;
  }
  return segment;
}
template<class Date, class Less> Id vertex_component(const History<Date>& history, Id vertex,
    const Date& cut, bool closed, Less less) {
  const auto& anchor = history.births[index_of(history.births, vertex)];
  if (!admitted(anchor.admission, cut, closed, less)) return absent;
  return normalize_segment(history, anchor.segment, cut, closed, less);
}
template<class Date, class Less> Id mark_component(const History<Date>& history, Id mark,
    const Date& cut, bool closed, Less less) {
  const auto& anchor = history.marks[index_of(history.marks, mark)];
  // Its old representative is NOT permission to admit this mark early.
  if (!admitted(anchor.admission, cut, closed, less)) return absent;
  return normalize_segment(history, anchor.segment, cut, closed, less);
}

// Sequential structural reference. Each plateau snapshots old components
// before ANY union, then emits one multifusion per resulting connected group.
// Temporary work touches only plateau edge endpoints; no all-vertices scan per
// date. No binary nodes of zero lifetime and no continuation nodes are emitted.
template<class Date, class Less> History<Date> reconstruct(const Certificate<Date>& certificate, Less less) {
  auto forest = normalized(Graph<Date>{certificate.births, certificate.edges, certificate.marks}, less);
  sort_edges(forest.edges, less);
  Dsu dsu(forest.births.size());
  std::vector<Id> segment(forest.births.size(), absent);
  std::vector<std::size_t> dates(forest.births.size());
  std::iota(dates.begin(), dates.end(), 0);
  std::sort(dates.begin(), dates.end(), [&](auto a, auto b) {
    if (less(forest.births[a].date, forest.births[b].date)) return true;
    if (less(forest.births[b].date, forest.births[a].date)) return false;
    return forest.births[a].id < forest.births[b].id;
  });
  History<Date> out;
  out.births.reserve(forest.births.size());
  std::size_t next_vertex = 0, next_edge = 0;
  auto birth = [&](std::size_t i) {
    const auto& vertex = forest.births[i];
    const Id node = out.nodes.size();
    require(node != absent, "filtered.node_representation");
    out.nodes.push_back({vertex.date, static_cast<Id>(out.parents.size()), 0, vertex.id});
    out.successors.push_back(absent);
    segment[i] = node;
    out.births.push_back({vertex.id, node, vertex.date});
  };
  while (next_edge < forest.edges.size()) {
    const Date level = forest.edges[next_edge].date;
    while (next_vertex < dates.size() && !less(level, forest.births[dates[next_vertex]].date))
      birth(dates[next_vertex++]);
    std::size_t end = next_edge;
    while (end < forest.edges.size() && equal_date(forest.edges[end].date, level, less)) ++end;
    std::vector<std::size_t> touched;
    for (std::size_t e = next_edge; e < end; ++e) {
      const auto& edge = forest.edges[e];
      touched.push_back(dsu.find(index_of(forest.births, edge.a)));
      touched.push_back(dsu.find(index_of(forest.births, edge.b)));
    }
    std::sort(touched.begin(), touched.end());
    touched.erase(std::unique(touched.begin(), touched.end()), touched.end());
    for (std::size_t e = next_edge; e < end; ++e) {
      const auto& edge = forest.edges[e];
      require(dsu.unite(index_of(forest.births, edge.a), index_of(forest.births, edge.b)), "filtered.certificate_cycle");
    }
    struct Item { std::size_t root; Id parent; };
    std::vector<Item> items;
    for (const auto old : touched) {
      require(segment[old] != absent, "filtered.future_parent");
      items.push_back({dsu.find(old), segment[old]});
    }
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
      return a.root < b.root || (a.root == b.root && a.parent < b.parent);
    });
    struct Group { std::size_t first, end, root; Id least; };
    std::vector<Group> groups;
    for (std::size_t first = 0; first < items.size();) {
      std::size_t last = first + 1;
      Id least = out.nodes[items[first].parent].least_birth;
      while (last < items.size() && items[last].root == items[first].root) {
        least = std::min(least, out.nodes[items[last].parent].least_birth); ++last;
      }
      require(last - first >= 2, "filtered.multifusion_parent_count");
      groups.push_back({first, last, items[first].root, least}); first = last;
    }
    std::sort(groups.begin(), groups.end(), [](const auto& a, const auto& b) { return a.least < b.least; });
    for (const auto& group : groups) {
      const Id node = out.nodes.size();
      require(node != absent, "filtered.node_representation");
      out.nodes.push_back({level, static_cast<Id>(out.parents.size()), static_cast<Id>(group.end - group.first), group.least});
      out.successors.push_back(absent);
      for (std::size_t j = group.first; j < group.end; ++j) {
        const Id parent = items[j].parent;
        require(parent < node && less(out.nodes[parent].date, level) && out.successors[parent] == absent,
                "filtered.parents_are_strict_pre_plateau_roots");
        out.parents.push_back(parent); out.successors[parent] = node;
      }
      segment[group.root] = node;
    }
    next_edge = end;
  }
  while (next_vertex < dates.size()) birth(dates[next_vertex++]);
  sort_ids(out.births);
  for (Id node = 0; node < out.nodes.size(); ++node)
    if (out.successors[node] == absent) out.roots.push_back(node);
  // Resolve every mark at its closed admission in one offline sweep, not M
  // ancestor walks (which could be quadratic on a chain). Read queries above
  // remain simple O(height) reference queries, outside this construction cost.
  std::sort(forest.marks.begin(), forest.marks.end(), [&](const auto& a, const auto& b) {
    if (less(a.admission, b.admission)) return true;
    if (less(b.admission, a.admission)) return false;
    return a.id < b.id;
  });
  Dsu marked(out.births.size());
  std::vector<Id> active_segment(out.births.size(), absent);
  std::size_t next_node = 0;
  for (const auto& mark : forest.marks) {
    while (next_node < out.nodes.size() && !less(mark.admission, out.nodes[next_node].date)) {
      const auto& node = out.nodes[next_node];
      const auto first = index_of(out.births, node.least_birth);
      for (Id j = 0; j < node.parent_count; ++j) {
        const auto parent = out.parents[node.first + j];
        marked.unite(first, index_of(out.births, out.nodes[parent].least_birth));
      }
      active_segment[marked.find(first)] = next_node++;
    }
    const Id at_admission = active_segment[marked.find(index_of(out.births, mark.representative))];
    require(at_admission != absent, "filtered.mark_not_admitted");
    out.marks.push_back({mark.id, mark.representative, at_admission, mark.admission});
  }
  sort_ids(out.marks);
  return out;
}
}  // namespace mhgp7::filtered_calendar_private
