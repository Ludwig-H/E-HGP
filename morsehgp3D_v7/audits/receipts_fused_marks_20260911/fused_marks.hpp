// Audit-only prototype derived from the pinned reconstruct function.
// The plateau body, exact comparisons and normalized input guards are retained.
#ifndef MHGP7_AUDIT_CALENDAR_HEADER
#error "Supply the immutable calendar header via MHGP7_AUDIT_CALENDAR_HEADER"
#endif
#include MHGP7_AUDIT_CALENDAR_HEADER
namespace mhgp7::fused_marks_audit {
using namespace filtered_calendar_private;
enum class Fault { none, before_plateau, birth_admission };
struct Work { Id union_attempts = 0, mark_find_queries = 0, event_dates = 0; };
template<class Date, class Less> History<Date> reconstruct(const Certificate<Date>& certificate, Less less, Work& work, Fault fault = Fault::none) {
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
  std::size_t next_vertex = 0, next_edge = 0, next_mark = 0;
  auto birth = [&](std::size_t i) {
    const auto& vertex = forest.births[i];
    const Id node = out.nodes.size();
    require(node != absent, "filtered.node_representation");
    out.nodes.push_back({vertex.date, static_cast<Id>(out.parents.size()), 0, vertex.id});
    out.successors.push_back(absent);
    segment[i] = node;
    out.births.push_back({vertex.id, node, vertex.date});
  };
  std::sort(forest.marks.begin(), forest.marks.end(), [&](const auto& a, const auto& b) {
    if (less(a.admission, b.admission)) return true;
    if (less(b.admission, a.admission)) return false;
    return a.id < b.id;
  });
  auto marks_at = [&](const Date& level) {
    while (next_mark < forest.marks.size() && !less(level, forest.marks[next_mark].admission)) {
      const auto& mark = forest.marks[next_mark++];
      const auto representative = index_of(forest.births, mark.representative);
      const Id target = segment[dsu.find(representative)];
      ++work.mark_find_queries;
      require(target != absent, "filtered.mark_not_admitted");
      const Date admission = fault == Fault::birth_admission
          ? forest.births[representative].date : mark.admission;
      out.marks.push_back({mark.id, mark.representative, target, admission});
    }
  };
  while (next_edge < forest.edges.size() || next_mark < forest.marks.size()) {
    const bool edge_level = next_edge < forest.edges.size() &&
        (next_mark == forest.marks.size() || !less(forest.marks[next_mark].admission, forest.edges[next_edge].date));
    // Choosing the EDGE representation at a tied date preserves raw Node.date.
    const Date level = edge_level ? forest.edges[next_edge].date : forest.marks[next_mark].admission;
    ++work.event_dates;
    while (next_vertex < dates.size() && !less(level, forest.births[dates[next_vertex]].date))
      birth(dates[next_vertex++]);
    if (fault == Fault::before_plateau) marks_at(level);
    if (edge_level) {
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
        ++work.union_attempts;
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
    marks_at(level);  // Closed admission sees the WHOLE completed plateau.
  }
  while (next_vertex < dates.size()) birth(dates[next_vertex++]);
  sort_ids(out.births);
  for (Id node = 0; node < out.nodes.size(); ++node)
    if (out.successors[node] == absent) out.roots.push_back(node);
  sort_ids(out.marks);
  return out;
}
}  // namespace mhgp7::fused_marks_audit
