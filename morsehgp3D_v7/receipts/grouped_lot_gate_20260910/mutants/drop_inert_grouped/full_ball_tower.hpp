// FULL horizontal forests and vertical birth maps from supplied COMPLETE EXACT
// ball censuses. No claim of WSPD completeness, archive authority, or speed.
#pragma once

#include <numeric>
#include <unordered_map>

#include "anchor_meb.hpp"
#include "full_coverage_certificate.hpp"
#include "local_plateau.hpp"
#include "../pipeline/expand.hpp"

namespace mhgp7 {

inline constexpr const char* kFullBallAuthority =
    "full_tower_relative_to_supplied_complete_exact_ball_censuses";
inline constexpr const char* kFullBallValidationAccounting =
    "declared_regular_support_plus_extra_anchor_meb_v1";
inline constexpr const char* kFullBallLotAccounting =
    "direct_unitary_lots_and_grouped_dsu_slots_v1";
inline constexpr const char* kFullBallLowerAccounting =
    "monotone_lower_edge_activation_ranked_union_find_v1";
enum class FullBallStatus { kCompleteRelative, kInvalidInput, kResourceExhausted, kInvariantViolated };
struct FullBallStats {
  u64 records = 0, extra_records = 0, anchor_blocks = 0, regular_blocks = 0, extra_blocks = 0;
  u64 representatives = 0, anchor_hits = 0, key_lookups = 0;
  u64 intruder_queries = 0, intruder_nodes = 0, intruder_power_tests = 0, interior_ranges = 0;
  u64 same_radius_steps = 0, descending_steps = 0, max_chain_steps = 0;
  u64 births = 0, merges = 0, contributions = 0, inert_blocks = 0;
  u64 declared_support_checks = 0, singleton_lots = 0, grouped_lots = 0, lot_dsu_slots = 0;
  u64 lower_edges_indexed = 0, lower_nodes_activated = 0, lower_edges_activated = 0;
  u64 lower_queries = 0, lower_find_steps = 0, lower_path_writes = 0;
  AnchorMebWork validation_work, resolve_work;
};
struct FullBallOrder {
  FullCoverageCertificate forest;
  // Image at the node's closed creation level. Queries normalize in the lower
  // history at their OWN cut. All entries are absent at K1.
  std::vector<FullNodeId> lower_nodes;
};
struct FullBallTowerResult {
  FullBallStatus status = FullBallStatus::kInvalidInput;
  const char* reason = "full_ball_uninitialized";
  FullBallStats stats;
  std::vector<FullBallOrder> orders;  // index K-1, no partial orders on failure
};

namespace full_ball_detail {
using BallId = u32;
constexpr u64 absent = kFullCoverageAbsent;
struct Failure { FullBallStatus status; const char* reason; };
inline void require(bool ok, const char* reason, FullBallStatus status = FullBallStatus::kInvariantViolated) {
  if (!ok) throw Failure{status, reason};
}
inline void add(u64& count, u64 amount = 1) {
  require(amount <= std::numeric_limits<u64>::max() - count, "full_ball_counter_overflow",
          FullBallStatus::kResourceExhausted);
  count += amount;
}
struct History {
  std::vector<ExactLevel> levels;
  std::vector<u64> next;
  u64 root_at(u64 token, const ExactLevel& cut, bool closed) const {
    require(token < levels.size() && full_coverage_detail::admitted(levels[token], cut, closed),
            "full_ball_anchor_not_born");
    while (next[token] != absent && full_coverage_detail::admitted(levels[next[token]], cut, closed))
      token = next[token];
    return token;
  }
};
// Working view for chronologically increasing LOWER closed cuts. Component
// labels are separate from DSU representatives, so union by rank is permitted.
// The immutable source history still answers arbitrary historical export cuts.
class MonotoneHistory {
 public:
  MonotoneHistory(const History& history, FullBallStats& stats) : source(history), st(stats),
      heads(history.levels.size(), absent), links(history.levels.size(), absent),
      parent(history.levels.size()), owner(history.levels.size()), rank(history.levels.size(), 0) {
    const size_t n = source.levels.size();
    require(source.next.size() == n, "full_ball_lower_history_shape");
    std::iota(parent.begin(), parent.end(), u64{0});
    std::iota(owner.begin(), owner.end(), u64{0});
    for (size_t node = 0; node < n; ++node) {
      require(source.levels[node].den > 0 && (!node ||
          compare_exact_level(source.levels[node - 1], source.levels[node]) <= 0),
          "full_ball_lower_history_chronology");
      const u64 next = source.next[node];
      if (next == absent) continue;
      require(next > node && next < n && compare_exact_level(source.levels[node], source.levels[next]) < 0,
          "full_ball_lower_history_edge");
      add(st.lower_edges_indexed);
      links[node] = heads[next]; heads[next] = node;
    }
  }

  u64 root_at(u64 token, const ExactLevel& cut, bool closed) {
    add(st.lower_queries);
    require(cut.den > 0, "full_ball_lower_cut_domain");
    if (has_cut) {
      const int cmp = compare_exact_level(cut, previous_cut);
      require(cmp > 0 || (cmp == 0 && (closed || !previous_closed)), "full_ball_lower_cut_not_monotone");
    }
    has_cut = true; previous_cut = cut; previous_closed = closed;
    while (cursor < source.levels.size() &&
        full_coverage_detail::admitted(source.levels[cursor], cut, closed)) {
      add(st.lower_nodes_activated);
      u64 representative = cursor;
      for (u64 child = heads[cursor]; child != absent; child = links[child]) {
        add(st.lower_edges_activated);
        const u64 a = find(representative), b = find(child);
        require(a != b, "full_ball_lower_duplicate_component");
        representative = unite(a, b);
      }
      owner[representative] = cursor;
      ++cursor;
    }
    require(token < cursor, "full_ball_lower_anchor_not_born");
    return owner[find(token)];
  }

 private:
  const History& source;
  FullBallStats& st;
  std::vector<u64> heads, links, parent, owner;
  std::vector<u8> rank;
  size_t cursor = 0;
  ExactLevel previous_cut{};
  bool has_cut = false, previous_closed = false;

  u64 find(u64 token) {
    u64 root = token;
    while (parent[root] != root) { add(st.lower_find_steps); root = parent[root]; }
    while (parent[token] != token) {
      add(st.lower_find_steps);
      const u64 next = parent[token];
      if (next != root) { add(st.lower_path_writes); parent[token] = root; }
      token = next;
    }
    return root;
  }
  u64 unite(u64 a, u64 b) {
    if (rank[a] < rank[b] || (rank[a] == rank[b] && a > b)) std::swap(a, b);
    add(st.lower_path_writes); parent[b] = a;
    if (rank[a] == rank[b]) ++rank[a];  // rank <= log2(node count) <= 64
    return a;
  }
};
struct Draft {
  std::vector<FullCoverageBatch> batches;
  std::vector<u64> lower_nodes;
};
struct Block {
  BallId ball;
  std::vector<u64> roots;
  u16 contribution = 0;
  bool interior = false;
};

class Builder {
 public:
  Builder(const CloudIndex& index, std::span<const BallData> census, unsigned max_k, FullBallStats& stats)
      : ix(index), balls(census), requested(max_k), st(stats) {}

  std::vector<FullBallOrder> run() {
    validate_catalogue();
    std::vector<Draft> drafts;
    History lower_history;
    std::vector<u64> lower_anchors;
    for (unsigned k = 1; k <= kmax; ++k) {
      current_k = k;
      anchors.assign(balls.size(), absent);
      current = {}; compressed.clear();
      MonotoneHistory lower_cursor(lower_history, st);
      Draft draft;
      if (k == 1) {
        FullCoverageBatch initial;
        for (PointId id : domain) {
          const u64 population = populations.size();
          populations.push_back({{}, {id}});
          initial.actions.push_back({{}, {{population, 1, false}}});
          add(st.contributions);
          new_node(initial.level, {}, draft, lower_cursor, absent);
        }
        draft.batches.push_back(std::move(initial));
      }
      const auto& program = programs[k];
      for (size_t begin = 0; begin < program.size();) {
        size_t end = begin + 1;
        const auto& level = balls[program[begin]].level;
        while (end < program.size() && same_exact_level(level, balls[program[end]].level)) ++end;
        // No anchor of this lot is installed until EVERY representative resolves.
        std::vector<Block> blocks;
        blocks.reserve(end - begin);
        const u64 prior_count = current.levels.size();
        for (size_t j = begin; j < end; ++j) blocks.push_back(prepare_block(program[j], level, prior_count));
        close_lot(blocks, level, draft, lower_cursor, lower_anchors);
        begin = end;
      }
      u64 live = 0;
      for (u64 next : current.next) if (next == absent) ++live;
      require(live == 1, "full_ball_final_component_count");
      drafts.push_back(std::move(draft));
      lower_history = std::move(current);
      lower_anchors = std::move(anchors);
    }
    auto bank = build_full_coverage_populations(domain, populations);
    require(bank.status == FullCertificateStatus::kOk, "full_ball_population_bank",
        bank.status == FullCertificateStatus::kResourceExhausted ? FullBallStatus::kResourceExhausted
                                                               : FullBallStatus::kInvariantViolated);
    // No further population is created: release construction rows before
    // encoding all forests. The immutable bank is shared across the tower.
    std::vector<FullCoveragePopulation>().swap(populations);
    std::vector<FullBallOrder> result;
    result.reserve(kmax);
    for (unsigned k = 1; k <= kmax; ++k) {
      auto forest = build_full_coverage_certificate(k, bank.value, drafts[k - 1].batches);
      require(forest.status == FullCertificateStatus::kOk, "full_ball_structural_certificate",
          forest.status == FullCertificateStatus::kResourceExhausted ? FullBallStatus::kResourceExhausted
                                                                   : FullBallStatus::kInvariantViolated);
      require(forest.value.nodes().size() == drafts[k - 1].lower_nodes.size(), "full_ball_node_encoding");
      result.push_back({std::move(forest.value), std::move(drafts[k - 1].lower_nodes)});
      std::vector<FullCoverageBatch>().swap(drafts[k - 1].batches);
    }
    return result;
  }

 private:
  const CloudIndex& ix;
  std::span<const BallData> balls;
  unsigned requested, kmax = 0, current_k = 0;
  FullBallStats& st;
  std::vector<PointId> domain;
  std::vector<std::pair<PointId,i32>> identity;
  std::vector<BallId> by_key;
  std::vector<std::vector<BallId>> programs;
  std::unordered_map<BallId, local_plateau::ShellTable> extra;
  std::vector<FullCoveragePopulation> populations;
  std::vector<u64> population_ids, anchors, compressed;
  History current;
  std::vector<NodeRef> stack;

  AnchorMebResult meb(std::span<const i32> sites, AnchorMebWork& work) {
    std::array<P3, kFacetMaxK> positions{};
    require(!sites.empty() && sites.size() <= positions.size(), "full_ball_meb_cardinality");
    for (size_t j = 0; j < sites.size(); ++j) positions[j] = ix.upos[sites[j]];
    auto result = anchor_meb(std::span<const P3>(positions.data(), sites.size()), work);
    require(result.status == AnchorMebStatus::kOk, "full_ball_meb_failure",
        result.status == AnchorMebStatus::kCounterOverflow ? FullBallStatus::kResourceExhausted
                                                         : FullBallStatus::kInvariantViolated);
    return result;
  }

  void validate_declared_support(const BallData& ball, const std::array<i32, 4>& support) {
    // The caller has already validated indices, primitive-domain bounds and
    // every interior/shell power. A positive declared support reproducing the
    // ball certifies it directly, without enumerating any of its sub-supports.
    std::array<P3, 4> positions{};
    for (size_t j = 0; j < ball.arity; ++j) positions[j] = ix.upos[support[j]];
    add(st.declared_support_checks);
    anchor_meb_detail::Candidate candidate;
    require(anchor_meb_detail::form(std::span<const P3>(positions.data(), ball.arity),
        {0, 1, 2, 3}, ball.arity, candidate), "full_ball_census_geometry", FullBallStatus::kInvalidInput);
    BallKey key;
    ExactLevel level;
    if (ball.arity == 2) {
      key = q2_ball_key(candidate.a, candidate.b);
      level = promote_level(q2_exact_level(p3_norm2(p3_sub(candidate.a, candidate.b))));
    } else if (ball.arity == 3) {
      key = q3_ball_key(candidate.three);
      level = promote_level(q3_exact_level(candidate.a, candidate.b, positions[2]));
    } else {
      key = ball_key_reduce(q4_ball_form(candidate.four));
      level = q4_level_raw(candidate.four);
    }
    require(key == ball.key && same_exact_level(level, ball.level),
        "full_ball_census_geometry", FullBallStatus::kInvalidInput);
  }

  void validate_catalogue() {
    constexpr auto invalid = FullBallStatus::kInvalidInput;
    require(requested > 0 && requested <= kFacetMaxK && ix.valid && !ix.upos.empty() &&
        !ix.has_duplicate_positions() && ix.input_count <= static_cast<u64>(std::numeric_limits<i32>::max()) &&
        balls.size() <= std::numeric_limits<BallId>::max(), "full_ball_input_domain", invalid);
    // As for existing spatial consumers, ix must be an authentic immutable
    // build_cloud_index result. No arbitrary hand-edited tree is certified.
    kmax = static_cast<unsigned>(std::min<u64>(requested, ix.input_count));
    for (size_t j = 0; j < ix.upos.size(); ++j) {
      require(p3_in_profile(ix.upos[j]), "full_ball_u16_domain", invalid);
      identity.emplace_back(ix.point_id(static_cast<i32>(j)), static_cast<i32>(j));
    }
    std::sort(identity.begin(), identity.end());
    for (const auto& row : identity) domain.push_back(row.first);
    require(std::adjacent_find(domain.begin(), domain.end()) == domain.end(), "full_ball_duplicate_id", invalid);
    by_key.resize(balls.size()); std::iota(by_key.begin(), by_key.end(), BallId{0});
    std::sort(by_key.begin(), by_key.end(), [&](BallId a, BallId b) { return balls[a].key < balls[b].key; });
    for (size_t j = 1; j < by_key.size(); ++j)
      require(!(balls[by_key[j]].key == balls[by_key[j - 1]].key), "full_ball_duplicate_key", invalid);
    std::vector<u8> qmins(balls.size());
    std::vector<size_t> counts(kmax + 1, 0);
    for (size_t j = 0; j < balls.size(); ++j) {
      const auto& ball = balls[j];
      require(ball.arity >= 2 && ball.arity <= 4 && ball.n_shell >= ball.arity &&
          ball.n_shell <= kBallShellMax && ball.n_interior <= kBallInteriorMax &&
          ball.level.den > 0, "full_ball_census_shape", invalid);
      std::array<i32, kBallInteriorMax + kBallShellMax> selected{};
      size_t n = 0;
      for (const auto sites : {ball.interior(), ball.shell()}) for (i32 site : sites) {
        require(site >= 0 && static_cast<size_t>(site) < ix.upos.size(), "full_ball_geometry_index", invalid);
        selected[n++] = site;
      }
      std::sort(selected.begin(), selected.begin() + n);
      require(std::adjacent_find(selected.begin(), selected.begin() + n) == selected.begin() + n,
              "full_ball_repeated_census_site", invalid);
      // Bound BEFORE any power/axis arithmetic, including malformed callers.
      require(ball.key.a > 0 && ball.key.a < (i128{1} << 68) &&
          uabs128(ball.key.c) < (u128{1} << 105), "full_ball_key_domain", invalid);
      for (i128 b : ball.key.b) require(uabs128(b) < (u128{1} << 87), "full_ball_key_domain", invalid);
      for (i32 site : ball.interior()) require(ball.key.power(ix.upos[site]) < 0, "full_ball_census_power", invalid);
      for (i32 site : ball.shell()) require(ball.key.power(ix.upos[site]) == 0, "full_ball_census_power", invalid);
      std::array<i32,4> support{};
      unsigned q = ball.arity;
      if (ball.n_shell == ball.arity) {
        std::copy(ball.shell().begin(), ball.shell().end(), support.begin());
      } else {
        local_plateau::LocalCensus local{ball.key, {}, {}};
        for (i32 site : ball.interior()) local.interior.push_back({ix.point_id(site), ix.upos[site]});
        for (i32 site : ball.shell()) local.shell.push_back({ix.point_id(site), ix.upos[site]});
        auto table = local_plateau::ShellTable::prepare(std::move(local));
        q = table.q_min();
        require(q == ball.arity, "full_ball_minimum_arity", invalid);
        const auto mask = table.minimal_supports().front();
        unsigned at = 0;
        for (size_t bit = 0; bit < table.census().shell.size(); ++bit)
          if (mask & (u16{1} << bit)) support[at++] = geometry_id(table.census().shell[bit].id);
        require(at == q, "full_ball_minimum_support");
        extra.emplace(static_cast<BallId>(j), std::move(table)); add(st.extra_records);
      }
      if (ball.n_shell == ball.arity) validate_declared_support(ball, support);
      else {
        const auto witness = meb(std::span<const i32>(support.data(), q), st.validation_work);
        require(witness.support_size == q && witness.key == ball.key &&
            same_exact_level(witness.level, ball.level), "full_ball_census_geometry", invalid);
      }
      require(ball.n_interior + q <= std::min<u64>(kmax + 1, ix.input_count),
              "full_ball_outside_rank_window", invalid);
      qmins[j] = static_cast<u8>(q);
      const unsigned lo = ball.n_interior + q - 1;
      const unsigned hi = std::min<unsigned>(kmax, ball.n_interior + ball.n_shell);
      for (unsigned k = lo; k <= hi; ++k) ++counts[k];
      add(st.records);
    }
    auto by_level = by_key;
    std::stable_sort(by_level.begin(), by_level.end(), [&](BallId a, BallId b) {
      return compare_exact_level(balls[a].level, balls[b].level) < 0;
    });
    programs.resize(kmax + 1);
    for (unsigned k = 1; k <= kmax; ++k) programs[k].reserve(counts[k]);
    for (BallId b : by_level) {
      const unsigned lo = balls[b].n_interior + qmins[b] - 1;
      const unsigned hi = std::min<unsigned>(kmax, balls[b].n_interior + balls[b].n_shell);
      for (unsigned k = lo; k <= hi; ++k) programs[k].push_back(b);
    }
    population_ids.assign(balls.size(), absent);
  }

  i32 geometry_id(PointId id) const {
    const auto found = std::lower_bound(identity.begin(), identity.end(), std::pair<PointId,i32>{id, -1});
    require(found != identity.end() && found->first == id, "full_ball_unknown_identity");
    return found->second;
  }
  u64 root(u64 token, u64 prior_count) {
    require(token < prior_count, "full_ball_anchor_not_prior");
    u64 r = token;
    while (compressed[r] != r) r = compressed[r];
    while (compressed[token] != token) {
      const auto next = compressed[token]; compressed[token] = r; token = next;
    }
    require(r < prior_count && current.next[r] == absent, "full_ball_root_not_prior");
    return r;
  }
  i32 intruder(const BallKey& key, std::span<const i32> selected) {
    add(st.intruder_queries);
    const auto member = [&](i32 u) { return std::binary_search(selected.begin(), selected.end(), u); };
    const census_detail::AxisBounds bounds(key);
    stack.clear(); stack.push_back(ix.root());
    while (!stack.empty()) {
      const auto node = stack.back(); stack.pop_back(); add(st.intruder_nodes);
      i128 lo, hi; bounds.bounds(ix.box_of(node), &lo, &hi);
      if (lo >= 0) continue;
      if (hi < 0) {
        add(st.interior_ranges);
        const auto range = ix.range_of(node);
        for (i32 u = range.first; u <= range.last; ++u) if (!member(u)) return u;
      } else if (is_leaf(node)) {
        const i32 u = leaf_index(node);
        if (!member(u)) {
          add(st.intruder_power_tests);
          if (key.power(ix.upos[u]) < 0) return u;
        }
      } else {
        stack.push_back(ix.nodes[node].right); stack.push_back(ix.nodes[node].left);
      }
    }
    return -1;
  }
  u64 resolve(std::vector<i32> sites, const ExactLevel& before, u64 prior_count) {
    add(st.representatives);
    std::sort(sites.begin(), sites.end());
    require(sites.size() == current_k && std::adjacent_find(sites.begin(), sites.end()) == sites.end(),
            "full_ball_representative_cardinality");
    if (current_k == 1) {
      const PointId id = ix.point_id(sites.front());
      return root(std::lower_bound(domain.begin(), domain.end(), id) - domain.begin(), prior_count);
    }
    auto local = meb(sites, st.resolve_work);
    u64 length = 0;
    for (;;) {
      require(compare_exact_level(local.level, before) < 0, "full_ball_representative_not_strict");
      add(st.key_lookups);
      const auto found = std::lower_bound(by_key.begin(), by_key.end(), local.key,
          [&](BallId id, const BallKey& key) { return balls[id].key < key; });
      if (found != by_key.end() && balls[*found].key == local.key) {
        require(same_exact_level(balls[*found].level, local.level), "full_ball_anchor_level");
        if (anchors[*found] != absent) {
          add(st.anchor_hits); st.max_chain_steps = std::max(st.max_chain_steps, length);
          return root(anchors[*found], prior_count);  // hit BEFORE spatial query
        }
        const auto& ball = balls[*found];
        const unsigned lo = ball.n_interior + ball.arity - 1;
        require(current_k < lo || current_k > ball.n_interior + ball.n_shell,
                "full_ball_missing_closed_anchor");
      }
      const i32 z = intruder(local.key, sites);
      require(z >= 0, "full_ball_missing_weak_terminal");
      require(local.support_size > 0 && local.support_slots[0] < sites.size(), "full_ball_missing_support");
      sites[local.support_slots[0]] = z;
      std::sort(sites.begin(), sites.end());
      auto next = meb(sites, st.resolve_work);
      const int cmp = compare_exact_level(next.level, local.level);
      require(cmp <= 0, "full_ball_radius_increased");
      if (cmp == 0) {
        require(next.key == local.key && next.selected_shell_count + 1 == local.selected_shell_count,
                "full_ball_equal_radius_shell_not_decreased");
        add(st.same_radius_steps);
      } else add(st.descending_steps);
      add(length); local = next;
    }
  }

  Block prepare_block(BallId id, const ExactLevel& level, u64 prior_count) {
    Block block{id, {}, 0, false};
    const auto& b = balls[id]; add(st.anchor_blocks);
    if (b.n_shell == b.arity) {
      add(st.regular_blocks);
      if (current_k == b.n_interior + b.n_shell) {
        block.contribution = static_cast<u16>((1u << b.n_shell) - 1);
        block.interior = b.n_interior != 0;
      } else {
        require(current_k + 1 == b.n_interior + b.n_shell, "full_ball_regular_rank");
        for (size_t omit = 0; omit < b.n_shell; ++omit) {
          std::vector<i32> sites(b.interior().begin(), b.interior().end());
          for (size_t j = 0; j < b.n_shell; ++j) if (j != omit) sites.push_back(b.shell_ids[j]);
          block.roots.push_back(resolve(std::move(sites), level, prior_count));
        }
      }
    } else {
      add(st.extra_blocks);
      const auto& table = extra.at(id);
      const auto rank = table.rank(current_k);
      require(rank.present, "full_ball_absent_scheduled_block");
      block.contribution = rank.contribution_shell; block.interior = rank.contribution_interior;
      for (const auto& component : rank.strict_components) {
        std::vector<i32> sites;
        for (size_t j = 0; j < component.interior_prefix; ++j)
          sites.push_back(geometry_id(table.census().interior[j].id));
        for (size_t j = 0; j < table.census().shell.size(); ++j)
          if (component.representative_shell & (u16{1} << j))
            sites.push_back(geometry_id(table.census().shell[j].id));
        block.roots.push_back(resolve(std::move(sites), level, prior_count));
      }
    }
    std::sort(block.roots.begin(), block.roots.end());
    block.roots.erase(std::unique(block.roots.begin(), block.roots.end()), block.roots.end());
    return block;
  }
  u64 population(BallId id) {
    if (population_ids[id] != absent) return population_ids[id];
    FullCoveragePopulation row;
    for (i32 site : balls[id].interior()) row.interior.push_back(ix.point_id(site));
    for (i32 site : balls[id].shell()) row.shell.push_back(ix.point_id(site));
    std::sort(row.interior.begin(), row.interior.end()); std::sort(row.shell.begin(), row.shell.end());
    population_ids[id] = populations.size(); populations.push_back(std::move(row));
    return population_ids[id];
  }
  u64 new_node(const ExactLevel& level, const std::vector<u64>& parents, Draft& draft,
      MonotoneHistory& lower, u64 birth_image) {
    u64 image = birth_image;
    if (current_k > 1 && !parents.empty()) {
      image = lower.root_at(draft.lower_nodes[parents.front()], level, true);
      for (u64 p : parents)
        require(lower.root_at(draft.lower_nodes[p], level, true) == image, "full_ball_vertical_naturality");
    }
    const u64 id = current.levels.size();
    require(id != absent, "full_ball_node_representation", FullBallStatus::kResourceExhausted);
    current.levels.push_back(level); current.next.push_back(absent); compressed.push_back(id);
    for (u64 parent : parents) { current.next[parent] = id; compressed[parent] = id; }
    draft.lower_nodes.push_back(image);
    if (parents.empty()) add(st.births); else add(st.merges);
    return id;
  }
  void close_lot(const std::vector<Block>& blocks, const ExactLevel& level, Draft& draft,
      MonotoneHistory& lower, const std::vector<u64>& lower_anchors) {
    if (blocks.size() == 1) {
      add(st.singleton_lots);
      const auto& block = blocks.front();
      FullCoverageAction action;
      action.parents = block.roots;  // Already sorted and unique in prepare_block.
      if (block.contribution || block.interior) {
        action.contributions.push_back({population(block.ball), block.contribution, block.interior});
        add(st.contributions);
      }
      u64 target;
      if (action.parents.size() == 1) target = action.parents.front();
      else {
        u64 birth_image = absent;
        if (action.parents.empty()) {
          require(action.contributions.size() == 1, "full_ball_distinct_births");
          if (current_k > 1) {
            require(block.ball < lower_anchors.size() && lower_anchors[block.ball] != absent,
                "full_ball_vertical_birth_anchor");
            birth_image = lower.root_at(lower_anchors[block.ball], level, true);
          }
        }
        target = new_node(level, action.parents, draft, lower, birth_image);
      }
      if (action.parents.size() != 1 || !action.contributions.empty()) {
        FullCoverageBatch batch{level, {}};
        batch.actions.push_back(std::move(action));
        draft.batches.push_back(std::move(batch));
      } else add(st.inert_blocks);
      require(anchors[block.ball] == absent && target != absent, "full_ball_anchor_duplicate");
      anchors[block.ball] = target;  // All representatives and the whole lot are closed.
      return;
    }
    add(st.grouped_lots);
    // Prospectively account the actual DSU initialization; unitary lots never
    // reach this path and create no DSU, owner groups or target array.
    add(st.lot_dsu_slots, blocks.size());
    std::vector<size_t> dsu(blocks.size()); std::iota(dsu.begin(), dsu.end(), size_t{0});
    const auto find = [&](size_t a) { while (dsu[a] != a) { dsu[a] = dsu[dsu[a]]; a = dsu[a]; } return a; };
    std::vector<std::pair<u64,size_t>> owners;
    for (size_t b = 0; b < blocks.size(); ++b) for (u64 p : blocks[b].roots) owners.emplace_back(p,b);
    std::sort(owners.begin(), owners.end());
    for (size_t j = 1; j < owners.size(); ++j) if (owners[j - 1].first == owners[j].first) {
      const auto a = find(owners[j - 1].second), b = find(owners[j].second);
      dsu[std::max(a,b)] = std::min(a,b);
    }
    std::vector<std::vector<size_t>> groups(blocks.size());
    for (size_t b = 0; b < blocks.size(); ++b) groups[find(b)].push_back(b);
    FullCoverageBatch batch{level, {}};
    std::vector<u64> targets(blocks.size(), absent);
    for (const auto& group : groups) {
      if (group.empty()) continue;
      FullCoverageAction action;
      for (size_t b : group) {
        const auto& block = blocks[b];
        action.parents.insert(action.parents.end(), block.roots.begin(), block.roots.end());
        if (block.contribution || block.interior) {
          action.contributions.push_back({population(block.ball), block.contribution, block.interior});
          add(st.contributions);
        }
      }
      std::sort(action.parents.begin(), action.parents.end());
      action.parents.erase(std::unique(action.parents.begin(), action.parents.end()), action.parents.end());
      u64 target;
      if (action.parents.size() == 1) target = action.parents.front();
      else {
        u64 birth_image = absent;
        if (action.parents.empty()) {
          require(group.size() == 1 && action.contributions.size() == 1, "full_ball_distinct_births");
          if (current_k > 1) {
            const auto ball = blocks[group.front()].ball;
            require(ball < lower_anchors.size() && lower_anchors[ball] != absent, "full_ball_vertical_birth_anchor");
            birth_image = lower.root_at(lower_anchors[ball], level, true);
          }
        }
        target = new_node(level, action.parents, draft, lower, birth_image);
      }
      for (size_t b : group) targets[b] = target;
      if (action.parents.size() != 1 || !action.contributions.empty()) batch.actions.push_back(std::move(action));
      else add(st.inert_blocks, group.size());
    }
    if (!batch.actions.empty()) draft.batches.push_back(std::move(batch));
    // Publish anchors only AFTER the complete atomic group has been staged.
    // Failure anywhere invalidates the whole result, not just this lot.
    for (size_t b = 0; b < blocks.size(); ++b) {
      if (blocks[b].roots.size() == 1 && !blocks[b].contribution && !blocks[b].interior) continue;
      require(anchors[blocks[b].ball] == absent && targets[b] != absent, "full_ball_anchor_duplicate");
      anchors[blocks[b].ball] = targets[b];
    }
  }
};
}  // namespace full_ball_detail

inline FullBallTowerResult build_full_ball_tower(const CloudIndex& ix, std::span<const BallData> balls,
    unsigned kmax) {
  FullBallTowerResult result;
  try {
    result.orders = full_ball_detail::Builder(ix, balls, kmax, result.stats).run();
    result.status = FullBallStatus::kCompleteRelative; result.reason = kFullBallAuthority;
  } catch (const full_ball_detail::Failure& error) {
    result.status = error.status; result.reason = error.reason;
  } catch (const std::bad_alloc&) {
    result.status = FullBallStatus::kResourceExhausted; result.reason = "full_ball_allocation_failed";
  } catch (const std::length_error&) {
    result.status = FullBallStatus::kResourceExhausted; result.reason = "full_ball_size_overflow";
  } catch (const std::invalid_argument&) {
    result.status = FullBallStatus::kInvalidInput; result.reason = "full_ball_local_census_invalid";
  }
  if (result.status != FullBallStatus::kCompleteRelative) result.orders.clear();
  return result;
}

inline FullNodeId full_ball_vertical_root_at(const FullBallTowerResult& tower, unsigned k,
    FullNodeId root, const ExactLevel& cut, bool closed) {
  if (tower.status != FullBallStatus::kCompleteRelative || k < 2 || k > tower.orders.size())
    return kFullCoverageAbsent;
  const auto& upper = tower.orders[k - 1];
  if (root >= upper.lower_nodes.size() || full_coverage_root_at(upper.forest, root, cut, closed) != root)
    return kFullCoverageAbsent;
  return full_coverage_root_at(tower.orders[k - 2].forest, upper.lower_nodes[root], cut, closed);
}

}  // namespace mhgp7
