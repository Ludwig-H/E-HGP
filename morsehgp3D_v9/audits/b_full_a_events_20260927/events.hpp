#pragma once
// Explicit C++ port of the finite model b_full_phase_a_work_20260926.
// This first constructor is sequential. No geometric resolver is copied.
#include "../b_full_a_manifest_20260927/manifest.hpp"
#include <bit>

namespace mhgp9::audit::a_events {
namespace am = a_manifest;
using namespace mhgp9::tower;
inline void require(bool ok, const char* reason) { am::need(ok, reason); }
inline u64 plus(u64 a, u64 b) { return am::sum(a, b); }
inline u64 product(u64 a, u64 b) {
  require(!a || b <= am::absent / a, "event.size_overflow");
  return a * b;
}
template<class T> u64 bytes(const std::vector<T>& v) { return product(v.capacity(), sizeof(T)); }
template<class T> void release(std::vector<T>& v) { std::vector<T>().swap(v); }

struct Work {
  u64 vertices{}, edges{}, forest_edges{}, components{}, groups{}, parent_event_incidences{}, contributions{};
  u64 parent_draft_incidences{}, parent_forest_incidences{};
  u64 dsu_steps{}, ancestor_entries{}, ancestor_queries{}, ancestor_steps{}, predecessor_steps{};
  u64 continuation_rounds{}, continuation_tests{}, silent_groups{};
  // Simultaneously retained vector capacities only; not allocator/RSS peak.
  u64 temporary_capacity_observed_max{}, combined_capacity_observed_max{}, output_capacity{};
};
struct Times {
  double validate_ms{}, forest_ms{}, ancestors_ms{}, groups_ms{}, parents_ms{}, redirects_ms{}, output_ms{}, total_ms{};
};
struct Result { am::Output output; Work work; Times times; };
struct TreeEdge { u64 a{}, b{}; u32 weight{}; };
struct Adjacent { u64 vertex{}; u32 weight{}; };
struct Group { u32 run{}; u64 label{}, begin{}, end{}, minimum{}; };
struct Parent { u64 label{}, event{}; };
enum class Mutant { None, ClosedParents, OmitContributionFreeHistory, ReversePlateau };

// A minimum spanning forest preserves every open/closed weight cut.
// Source blocks already appear in nondecreasing run order. Kruskal can
// consume their occurrences directly: ties use their original ordinal.
// No new sort/materialization of all E edges is required in this witness.
inline Result build(const am::Manifest& m, bool root_high = true, Mutant mutant = Mutant::None) {
  const auto started = am::Clock::now();
  Result result; auto& w = result.work; auto& timing = result.times; auto& out = result.output;
  am::validate_manifest(m);
  const u64 initial = m.k == 1 ? m.domain.size() : 0;
  const u64 vertices = plus(initial, m.blocks.size());
  require(vertices && vertices <= std::numeric_limits<size_t>::max(), "event.vertex_domain");
  w.vertices = vertices; w.edges = m.target.size();
  timing.validate_ms = am::milliseconds(started);

  const auto phase_forest = am::Clock::now();
  std::vector<u64> dsu(vertices), degrees(vertices), offsets, cursor, stack;
  std::vector<u8> ranks(vertices), visited;
  std::vector<TreeEdge> forest;
  std::vector<Adjacent> adjacency;
  std::vector<u64> ancestors, closed_label, members, histories, history_offsets, node_ids;
  std::vector<u64> occurrence_event, parent_begin, parents, redirects, jumped;
  std::vector<u32> maximum;
  std::vector<Group> groups;
  std::vector<Parent> local_parents;
  std::vector<u64> local_nodes;
  std::vector<FullCoverageRef> local_refs;
  const auto output_bytes = [&] {
    u64 total = 0;
    const auto take = [&](const auto& v) { total = plus(total, bytes(v)); };
    take(out.anchors); take(out.next); take(out.runs); take(out.birth_ball); take(out.root_begin); take(out.roots);
    take(out.draft.level); take(out.draft.batch_begin); take(out.draft.parent_begin); take(out.draft.parent);
    take(out.draft.contribution_begin); take(out.draft.contribution);
    return total;
  };
  const auto observe = [&] {
    u64 total = 0;
    const auto take = [&](const auto& v) { total = plus(total, bytes(v)); };
    take(dsu); take(degrees); take(offsets); take(cursor); take(stack); take(ranks); take(visited);
    take(forest); take(adjacency); take(ancestors); take(maximum); take(closed_label); take(members);
    take(histories); take(history_offsets); take(node_ids); take(occurrence_event); take(parent_begin);
    take(parents); take(redirects); take(jumped); take(groups); take(local_parents); take(local_nodes); take(local_refs);
    w.temporary_capacity_observed_max = std::max(w.temporary_capacity_observed_max, total);
    w.combined_capacity_observed_max = std::max(w.combined_capacity_observed_max, plus(total, output_bytes()));
  };
  const auto run_of = [&](u64 vertex) -> u32 { return vertex < initial ? 0 : m.blocks[vertex - initial].run; };
  const auto target_vertex = [&](u64 occurrence) { return m.k == 1 ? m.target[occurrence] : plus(initial, m.target[occurrence]); };
  std::iota(dsu.begin(), dsu.end(), u64{0});
  const auto find = [&](u64 v) {
    while (dsu[v] != v) { ++w.dsu_steps; dsu[v] = dsu[dsu[v]]; v = dsu[v]; }
    return v;
  };
  for (u64 b = 0; b < m.blocks.size(); ++b) {
    const u64 source = plus(initial, b);
    for (u64 j = m.representative_begin[b]; j < m.representative_begin[b + 1]; ++j) {
      const u64 target = target_vertex(j); u64 a = find(source), c = find(target);
      if (a == c) continue;
      if (ranks[a] < ranks[c]) std::swap(a, c);
      dsu[c] = a;
      if (ranks[a] == ranks[c]) { require(ranks[a] < 255, "event.dsu_rank"); ++ranks[a]; }
      forest.push_back({source, target, m.blocks[b].run}); ++degrees[source]; ++degrees[target];
    }
  }
  w.forest_edges = forest.size(); w.components = vertices - forest.size();
  offsets.resize(plus(vertices, 1));
  for (u64 v = 0; v < vertices; ++v) offsets[v + 1] = plus(offsets[v], degrees[v]);
  adjacency.resize(offsets.back()); cursor = offsets;
  for (const auto& edge : forest) {
    adjacency[cursor[edge.a]++] = {edge.b, edge.weight};
    adjacency[cursor[edge.b]++] = {edge.a, edge.weight};
  }
  observe(); release(dsu); release(ranks); release(degrees); release(cursor); release(forest);
  timing.forest_ms = am::milliseconds(phase_forest);

  const auto phase_ancestors = am::Clock::now();
  const unsigned powers = std::max(1U, static_cast<unsigned>(std::bit_width(vertices)));
  const u64 entries = product(vertices, powers);
  require(entries <= ancestors.max_size() && entries <= maximum.max_size(), "event.ancestor_size");
  ancestors.resize(entries); maximum.resize(entries); visited.resize(vertices);
  w.ancestor_entries = product(entries, 2);
  for (u64 step = 0; step < vertices; ++step) {
    const u64 root = root_high ? vertices - 1 - step : step;
    if (visited[root]) continue;
    visited[root] = 1; ancestors[root] = root; stack.push_back(root);
    while (!stack.empty()) {
      const u64 source = stack.back(); stack.pop_back();
      for (u64 j = offsets[source]; j < offsets[source + 1]; ++j) {
        const auto edge = adjacency[j];
        if (visited[edge.vertex]) continue;
        visited[edge.vertex] = 1; ancestors[edge.vertex] = source; maximum[edge.vertex] = edge.weight;
        stack.push_back(edge.vertex);
      }
    }
  }
  for (unsigned p = 1; p < powers; ++p) for (u64 v = 0; v < vertices; ++v) {
    const u64 old = product(p - 1, vertices), next = product(p, vertices), parent = ancestors[old + v];
    ancestors[next + v] = ancestors[old + parent];
    maximum[next + v] = std::max(maximum[old + v], maximum[old + parent]);
  }
  observe(); release(adjacency); release(offsets); release(stack); release(visited);
  const auto label = [&](u64 vertex, u32 cut, bool closed) {
    ++w.ancestor_queries;
    for (unsigned p = powers; p > 0; --p) {
      ++w.ancestor_steps; const u64 index = product(p - 1, vertices) + vertex;
      const u32 weight = maximum[index];
      if (weight < cut || (closed && weight == cut)) vertex = ancestors[index];
    }
    return vertex;
  };
  timing.ancestors_ms = am::milliseconds(phase_ancestors);

  const auto phase_groups = am::Clock::now();
  closed_label.resize(vertices); members.resize(vertices);
  for (u64 v = 0; v < vertices; ++v) { closed_label[v] = label(v, run_of(v), true); members[v] = v; }
  std::sort(members.begin(), members.end(), [&](u64 a, u64 b) {
    return std::tuple(run_of(a), closed_label[a], a) < std::tuple(run_of(b), closed_label[b], b);
  });
  for (u64 begin = 0; begin < vertices;) {
    u64 end = begin + 1; const u64 vertex = members[begin];
    while (end < vertices && run_of(members[end]) == run_of(vertex) && closed_label[members[end]] == closed_label[vertex]) ++end;
    groups.push_back({run_of(vertex), closed_label[vertex], begin, end, vertex}); begin = end;
  }
  std::sort(groups.begin(), groups.end(), [&](const Group& a, const Group& b) {
    if (a.run != b.run) return a.run < b.run;
    return mutant == Mutant::ReversePlateau ? a.minimum > b.minimum : a.minimum < b.minimum;
  });
  w.groups = groups.size(); history_offsets.assign(plus(vertices, 1), 0);
  const auto keep_history = [&](const Group& group) {
    if (mutant != Mutant::OmitContributionFreeHistory) return true;
    for (u64 i = group.begin; i < group.end; ++i) {
      const auto v = members[i]; if (v < initial) return true;
      const auto& block = m.blocks[v - initial]; if (block.shell_mask || block.include_interior) return true;
    }
    return false;
  };
  for (const auto& group : groups) if (keep_history(group)) ++history_offsets[group.label + 1];
  for (u64 v = 0; v < vertices; ++v) history_offsets[v + 1] = plus(history_offsets[v + 1], history_offsets[v]);
  histories.resize(history_offsets.back()); cursor = history_offsets;
  for (u64 g = 0; g < groups.size(); ++g) if (keep_history(groups[g])) histories[cursor[groups[g].label]++] = g;
  observe(); release(cursor); release(closed_label);
  timing.groups_ms = am::milliseconds(phase_groups);

  const auto phase_parents = am::Clock::now();
  occurrence_event.resize(m.target.size()); parent_begin.push_back(0);
  redirects.resize(groups.size()); node_ids.assign(groups.size(), am::absent);
  const auto predecessor = [&](u64 component, u32 cut) {
    u64 low = history_offsets[component], high = history_offsets[component + 1], begin = low;
    while (low < high) {
      ++w.predecessor_steps; const u64 mid = low + (high - low) / 2;
      if (groups[histories[mid]].run < cut) low = mid + 1; else high = mid;
    }
    require(low > begin, "event.predecessor_missing"); return histories[low - 1];
  };
  u64 nodes = 0;
  for (u64 g = 0; g < groups.size(); ++g) {
    const auto& group = groups[g]; local_parents.clear();
    for (u64 i = group.begin; i < group.end; ++i) {
      const u64 vertex = members[i]; if (vertex < initial) continue;
      const u64 block = vertex - initial;
      for (u64 j = m.representative_begin[block]; j < m.representative_begin[block + 1]; ++j) {
        const u64 component = label(target_vertex(j), group.run, mutant == Mutant::ClosedParents), prior = predecessor(component, group.run);
        require(prior < g, "event.predecessor_chronology");
        occurrence_event[j] = prior; local_parents.push_back({component, prior});
      }
    }
    std::sort(local_parents.begin(), local_parents.end(), [](const Parent& a, const Parent& b) {
      return std::pair(a.label, a.event) < std::pair(b.label, b.event);
    });
    for (size_t i = 0; i < local_parents.size(); ++i) {
      if (i && local_parents[i].label == local_parents[i - 1].label) {
        require(local_parents[i].event == local_parents[i - 1].event, "event.label_history_inconsistent"); continue;
      }
      parents.push_back(local_parents[i].event);
    }
    parent_begin.push_back(parents.size());
    const u64 count = parent_begin[g + 1] - parent_begin[g];
    if (count == 1) redirects[g] = parents[parent_begin[g]];
    else { redirects[g] = g; node_ids[g] = nodes++; }
    observe();
  }
  w.parent_event_incidences = parents.size();
  observe(); release(ancestors); release(maximum); release(histories); release(history_offsets); release(local_parents);
  timing.parents_ms = am::milliseconds(phase_parents);

  const auto phase_redirects = am::Clock::now();
  jumped.resize(redirects.size());
  for (;;) {
    bool changed = false; ++w.continuation_rounds;
    for (u64 g = 0; g < groups.size(); ++g) {
      ++w.continuation_tests; jumped[g] = redirects[redirects[g]]; changed |= jumped[g] != redirects[g];
    }
    redirects.swap(jumped);
    if (!changed) break;
    require(w.continuation_rounds <= static_cast<u64>(std::bit_width(w.groups)) + 1, "event.redirect_cycle");
  }
  observe(); release(jumped); timing.redirects_ms = am::milliseconds(phase_redirects);

  const auto phase_output = am::Clock::now();
  require(nodes < am::absent32, "event.node_domain");
  out.anchors.assign(m.ball_count, am::absent32); out.next.assign(nodes, am::absent);
  out.runs.resize(nodes); out.birth_ball.assign(nodes, am::absent32);
  out.root_begin = m.representative_begin; out.roots.resize(m.target.size());
  for (u64 j = 0; j < m.target.size(); ++j) {
    out.roots[j] = node_ids[redirects[occurrence_event[j]]];
    require(out.roots[j] != am::absent, "event.root_identity");
  }
  // Native lot statistics concern only active ball blocks, not K1's
  // initial point births. Preserve globally skipped run numbers.
  for (size_t begin = 0; begin < m.blocks.size();) {
    size_t end = begin + 1;
    while (end < m.blocks.size() && m.blocks[end].run == m.blocks[begin].run) ++end;
    if (end - begin == 1) ++out.stats.singleton_lots;
    else { ++out.stats.grouped_lots; out.stats.lot_dsu_slots += end - begin; }
    begin = end;
  }
  out.stats.anchor_blocks = m.blocks.size(); out.stats.representatives = m.target.size();
  for (const auto& block : m.blocks) { if (block.regular) ++out.stats.regular_blocks; else ++out.stats.extra_blocks; }
  u32 emitted_run = am::absent32; size_t plateau_begin = 0;
  for (u64 g = 0; g < groups.size(); ++g) {
    const auto& group = groups[g]; const u64 owner = node_ids[redirects[g]];
    require(owner != am::absent, "event.owner_identity"); local_nodes.clear(); local_refs.clear();
    for (u64 j = parent_begin[g]; j < parent_begin[g + 1]; ++j) local_nodes.push_back(node_ids[redirects[parents[j]]]);
    std::sort(local_nodes.begin(), local_nodes.end());
    require(std::adjacent_find(local_nodes.begin(), local_nodes.end()) == local_nodes.end(), "event.parent_identity_collision");
    for (u64 i = group.begin; i < group.end; ++i) {
      const u64 vertex = members[i];
      if (vertex < initial) { local_refs.push_back({vertex, 1, false}); continue; }
      const auto& block = m.blocks[vertex - initial]; out.anchors[block.ball] = static_cast<u32>(owner);
      if (block.shell_mask || block.include_interior) local_refs.push_back({am::ball_tag | block.ball, block.shell_mask, block.include_interior});
    }
    out.stats.contributions += local_refs.size();
    if (local_nodes.size() != 1) {
      require(node_ids[g] == owner, "event.birth_redirect"); out.runs[owner] = group.run;
      if (local_nodes.empty()) {
        require(group.end - group.begin == 1 && local_refs.size() == 1, "event.distinct_birth");
        const u64 vertex = members[group.begin];
        out.birth_ball[owner] = vertex < initial ? am::absent32 : m.blocks[vertex - initial].ball; ++out.stats.births;
      } else ++out.stats.merges;
      for (u64 parent : local_nodes) {
        require(parent < owner && out.next[parent] == am::absent, "event.parent_written_twice"); out.next[parent] = owner;
      }
    }
    if (local_nodes.size() == 1 && local_refs.empty()) { ++w.silent_groups; out.stats.inert_blocks += group.end - group.begin; continue; }
    if (emitted_run != group.run) {
      ExactLevel level{{0, 0, 0}, 1};
      if (group.run) {
        while (plateau_begin < m.blocks.size() && m.blocks[plateau_begin].run < group.run) ++plateau_begin;
        require(plateau_begin < m.blocks.size() && m.blocks[plateau_begin].run == group.run, "event.plateau_level");
        level = m.blocks[plateau_begin].level;
      }
      out.draft.open_batch(level); emitted_run = group.run;
    }
    out.draft.add_action(local_nodes, local_refs); observe();
  }
  w.contributions = out.stats.contributions;
  require(std::count(out.next.begin(), out.next.end(), am::absent) == 1, "event.final_component");
  w.parent_draft_incidences = out.draft.parent.size();
  w.parent_forest_incidences = static_cast<u64>(std::count_if(out.next.begin(), out.next.end(), [](u64 x) { return x != am::absent; }));
  w.output_capacity = output_bytes(); observe();
  // Explicitly destroy all owned temporaries inside total_ms. Result and
  // immutable input remain live; this is neither FULL nor an RSS peak.
  release(members); release(node_ids); release(occurrence_event); release(parent_begin); release(parents);
  release(redirects); release(groups); release(local_nodes); release(local_refs);
  timing.output_ms = am::milliseconds(phase_output);
  timing.total_ms = am::milliseconds(started); return result;
}
} // namespace mhgp9::audit::a_events
