// Normalized horizontal fold: separate set-hypergraph oracle and token reader.
// Synthetic event streams qualify the fold contract, not Euclidean realizability.
// 0 pass; 1 disagreement; 2 bad CLI; 3 surviving mutation; 4 named mutation killed.
#include <algorithm>
#include <cstdio>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/forest/fold.hpp"

using namespace mhgp7;

namespace {

using Key = std::vector<u32>;
using Carrier = std::set<Key>;
using State = std::set<Carrier>;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

struct Rational {
  u64 n = 0, d = 1;
  Rational(u64 numerator = 0, u64 denominator = 1) : n(numerator), d(denominator) {
    require(d != 0, "oracle.level_domain");
    const u64 g = std::gcd(n, d);
    n /= g;
    d /= g;
  }
  bool operator==(const Rational&) const = default;
  bool operator<(const Rational& other) const {
    return static_cast<u128>(n) * other.d < static_cast<u128>(other.n) * d;
  }
};

struct Event {
  Key support, interior;
  u16 active;
  u64 numerator, denominator;
  Rational level() const { return {numerator, denominator}; }
};

struct Fixture {
  const char* name;
  int k;
  std::vector<Event> events;
};

Carrier facets_of(const Event& event) {
  Key vertices = event.support;
  vertices.insert(vertices.end(), event.interior.begin(), event.interior.end());
  std::sort(vertices.begin(), vertices.end());
  require(std::adjacent_find(vertices.begin(), vertices.end()) == vertices.end(), "oracle.distinct_vertices");
  Carrier facets;
  for (u32 omitted : vertices) {
    Key facet;
    std::copy_if(vertices.begin(), vertices.end(), std::back_inserter(facet),
                 [&](u32 id) { return id != omitted; });
    facets.insert(facet);
  }
  return facets;
}

bool intersects(const Carrier& first, const Carrier& second) {
  for (const auto& key : first) if (second.count(key)) return true;
  return false;
}

// Union of whole sets: no product DSU, facet interning or parent routine.
void join_hyperedge(State* state, const Carrier& edge) {
  Carrier merged = edge;
  State next;
  for (const auto& block : *state) {
    if (intersects(block, edge)) merged.insert(block.begin(), block.end());
    else next.insert(block);
  }
  next.insert(merged);
  *state = std::move(next);
}

struct Delta {
  Key output;
  std::vector<Key> parents, born;
  bool operator==(const Delta&) const = default;
  bool operator<(const Delta& other) const { return output < other.output; }
};

struct Expected {
  Carrier catalog;
  std::vector<Rational> levels;
  std::vector<State> before, after;
  std::vector<std::vector<Delta>> deltas;
  State final;
  u64 births = 0, growths = 0, merges = 0, continuations = 0;
  u64 no_new_point_growths = 0, latent_contacts = 0;
};

std::set<u32> point_union(const Carrier& carrier) {
  std::set<u32> result;
  for (const auto& facet : carrier) result.insert(facet.begin(), facet.end());
  return result;
}

Expected independent_hypergraph(const Fixture& fixture) {
  Expected result;
  std::map<Rational, std::vector<Carrier>> batches;
  for (const auto& event : fixture.events) {
    const auto edge = facets_of(event);
    require(!edge.empty() && edge.begin()->size() == static_cast<size_t>(fixture.k), "oracle.fixed_order");
    result.catalog.insert(edge.begin(), edge.end());
    batches[event.level()].push_back(edge);
  }
  Carrier seen;
  State state;
  if (fixture.k == 1) {
    seen = result.catalog;  // normative input vertices of this event universe.
    for (const auto& facet : seen) state.insert(Carrier{facet});
  }
  for (const auto& [level, hyperedges] : batches) {
    result.levels.push_back(level);
    result.before.push_back(state);
    const State previous = state;
    Carrier touched, born;
    for (const auto& edge : hyperedges) touched.insert(edge.begin(), edge.end());
    for (const auto& facet : touched)
      if (!seen.count(facet)) {
        born.insert(facet);
        state.insert(Carrier{facet});
      }
    // Active roles are independent geometric metadata. Count first contacts
    // marked active, without using those marks to obtain oracle connectivity.
    for (const auto& event : fixture.events) {
      if (!(event.level() == level)) continue;
      Key vertices = event.support;
      vertices.insert(vertices.end(), event.interior.begin(), event.interior.end());
      for (size_t s = 0; s < event.support.size(); ++s) {
        if (!(event.active & (1u << s))) continue;
        Key facet;
        for (u32 id : vertices) if (id != event.support[s]) facet.push_back(id);
        std::sort(facet.begin(), facet.end());
        result.latent_contacts += fixture.k >= 2 && !seen.count(facet);
      }
    }
    for (const auto& edge : hyperedges) join_hyperedge(&state, edge);
    std::vector<Delta> deltas;
    for (const auto& block : state) {
      if (!intersects(block, touched)) continue;
      Delta delta;
      delta.output = *block.begin();
      Carrier old_carrier;
      for (const auto& old : previous) {
        if (!intersects(old, block)) continue;
        delta.parents.push_back(*old.begin());
        old_carrier.insert(old.begin(), old.end());
      }
      for (const auto& facet : born) if (block.count(facet)) delta.born.push_back(facet);
      std::sort(delta.parents.begin(), delta.parents.end());
      if (delta.parents.size() == 1 && delta.born.empty()) {
        ++result.continuations;
        continue;
      }
      result.births += delta.parents.empty();
      result.growths += delta.parents.size() == 1;
      result.merges += delta.parents.size() >= 2;
      result.no_new_point_growths += delta.parents.size() == 1 && !delta.born.empty() &&
                                    point_union(old_carrier) == point_union(block);
      deltas.push_back(std::move(delta));
    }
    std::sort(deltas.begin(), deltas.end());
    result.deltas.push_back(std::move(deltas));
    result.after.push_back(state);
    seen.insert(touched.begin(), touched.end());
  }
  result.final = state;
  return result;
}

ForestEvent product_event(const Event& event) {
  ForestEvent result;
  result.q = static_cast<u8>(event.support.size());
  result.d = static_cast<u8>(event.interior.size());
  result.active_mask = event.active;
  std::copy(event.support.begin(), event.support.end(), result.support);
  std::copy(event.interior.begin(), event.interior.end(), result.interior);
  result.level = {{event.numerator, 0, 0}, static_cast<i128>(event.denominator)};
  return result;
}

Key read_key(const FacetKey& key, int k) {
  require(key.k == k, "key.order");
  Key result(key.p.begin(), key.p.begin() + key.k);
  require(std::is_sorted(result.begin(), result.end()) &&
          std::adjacent_find(result.begin(), result.end()) == result.end(), "key.canonical");
  for (size_t i = key.k; i < key.p.size(); ++i) require(key.p[i] == 0, "key.padding");
  return result;
}

Rational read_level(const ExactLevel& level) {
  require(level.num[1] == 0 && level.num[2] == 0 && level.den > 0 &&
          static_cast<u128>(level.den) <= std::numeric_limits<u64>::max(), "probe.level_domain");
  return {level.num[0], static_cast<u64>(level.den)};
}

struct Totals {
  u64 streams = 0, cuts = 0, deltas = 0, births = 0, growths = 0, merges = 0;
  u64 continuations = 0, no_new_point_growths = 0, latent_contacts = 0;
  u64 k1_roots = 0, rename_checks = 0, invalid_inputs = 0;
} totals;

std::vector<State> check_payload(const ForestResult& actual, const Expected& oracle, int k, ForestLayout layout) {
  require(actual.refusal.empty() && actual.storage_message.empty(), "unexpected.refusal");
  require(actual.normalized_reduced && actual.attach_violations == 0 && actual.birth_violations == 0 &&
          actual.partition_violations == 0 && actual.storage_violations == 0, "invariants");
  require(actual.storage_kind == (layout == ForestLayout::kCsr ? ForestStorageKind::kCsrFacetKeysV1 :
                                                                  ForestStorageKind::kVectorComponentDeltaV1), "layout.kind");
  require(actual.batches == oracle.levels.size() && actual.batch_levels.size() == oracle.levels.size(), "batch_count");
  Carrier catalog;
  for (const auto& facet : actual.facet_keys) require(catalog.insert(read_key(facet, k)).second, "catalog.unique");
  require(catalog == oracle.catalog && actual.facets == catalog.size(), "catalog.membership");
  std::vector<std::vector<Delta>> observed(oracle.levels.size());
  u64 expected_delta_count = 0;
  for (const auto& group : oracle.deltas) expected_delta_count += group.size();
  require(actual.delta_count() == expected_delta_count, "delta_count");
  for (size_t batch = 0; batch < oracle.levels.size(); ++batch)
    require(read_level(actual.batch_levels[batch]) == oracle.levels[batch], "batch_level");
  for (size_t index = 0; index < actual.delta_count(); ++index) {
    const auto view = actual.delta(index);
    require(view.batch < oracle.levels.size(), "delta.batch");
    require(read_level(view.level) == oracle.levels[view.batch], "delta.level");
    Delta delta;
    delta.output = read_key(view.output, k);
    for (const auto& facet : view.parents) delta.parents.push_back(read_key(facet, k));
    for (const auto& facet : view.born) delta.born.push_back(read_key(facet, k));
    require(std::is_sorted(delta.parents.begin(), delta.parents.end()) &&
            std::adjacent_find(delta.parents.begin(), delta.parents.end()) == delta.parents.end(), "parents.sorted_unique");
    require(std::is_sorted(delta.born.begin(), delta.born.end()) &&
            std::adjacent_find(delta.born.begin(), delta.born.end()) == delta.born.end(), "born.sorted_unique");
    observed[view.batch].push_back(std::move(delta));
  }
  // A fresh reader consumes only tokens. Oracle events never supply its unions.
  std::map<Key, Carrier> live;
  Carrier materialized;
  if (k == 1) {
    for (const auto& facet : catalog) live.emplace(facet, Carrier{facet});
    materialized = catalog;
    totals.k1_roots += catalog.size();
  }
  const auto snapshot = [&]() {
    State state;
    for (const auto& [key, carrier] : live) {
      require(key == *carrier.begin(), "reader.canonical_root");
      state.insert(carrier);
    }
    return state;
  };
  std::vector<State> cuts;
  u64 parents_count = 0, born_count = 0;
  for (size_t batch = 0; batch < oracle.levels.size(); ++batch) {
    require(snapshot() == oracle.before[batch], "cut.strict");
    cuts.push_back(snapshot());
    auto& group = observed[batch];
    std::sort(group.begin(), group.end());
    require(group.size() == oracle.deltas[batch].size(), "delta.batch_count");
    const auto before = live;
    std::set<Key> consumed, born_in_batch;
    std::map<Key, Carrier> published;
    for (size_t index = 0; index < group.size(); ++index) {
      const auto& delta = group[index];
      const auto& wanted = oracle.deltas[batch][index];
      require(delta.output == wanted.output, "delta.output");
      require(delta.parents == wanted.parents, "delta.parents");
      require(delta.born == wanted.born, "delta.born");
      require(delta.parents.size() != 1 || !delta.born.empty(), "continuation.emitted");
      Carrier carrier;
      for (const auto& parent : delta.parents) {
        const auto previous = before.find(parent);
        require(previous != before.end() && consumed.insert(parent).second, "reader.parent_not_live_prebatch");
        carrier.insert(previous->second.begin(), previous->second.end());
      }
      for (const auto& facet : delta.born) {
        require(!materialized.count(facet) && born_in_batch.insert(facet).second, "reader.repeated_materialization");
        carrier.insert(facet);
      }
      require(!carrier.empty() && *carrier.begin() == delta.output, "reader.output_canonical");
      require(published.emplace(delta.output, carrier).second, "reader.output_unique");
      parents_count += delta.parents.size();
      born_count += delta.born.size();
    }
    for (const auto& parent : consumed) live.erase(parent);
    for (const auto& [output, carrier] : published) require(live.emplace(output, carrier).second, "reader.output_collision");
    materialized.insert(born_in_batch.begin(), born_in_batch.end());
    require(snapshot() == oracle.after[batch], "cut.closed");
    cuts.push_back(snapshot());
  }
  require(snapshot() == oracle.final, "reader.final_partition");
  require(actual.keys_parents == parents_count && actual.keys_born == born_count, "payload.key_counts");
  require(actual.nodes == oracle.merges && actual.fusions == catalog.size() - oracle.final.size(), "forest.counters");
  require(actual.final_canon_fid.size() == actual.facet_keys.size(), "final.canon_size");
  for (size_t fid = 0; fid < actual.facet_keys.size(); ++fid) {
    const auto key = read_key(actual.facet_keys[fid], k);
    const auto canon = actual.final_canon_fid[fid];
    require(canon < actual.facet_keys.size(), "final.canon_domain");
    bool found = false;
    for (const auto& block : oracle.final)
      if (block.count(key)) {
        require(read_key(actual.facet_keys[canon], k) == *block.begin(), "final.canonical_membership");
        found = true;
      }
    require(found, "final.member_missing");
  }
  ++totals.streams;
  totals.cuts += cuts.size();
  totals.deltas += actual.delta_count();
  totals.births += oracle.births;
  totals.growths += oracle.growths;
  totals.merges += oracle.merges;
  totals.continuations += oracle.continuations;
  totals.no_new_point_growths += oracle.no_new_point_growths;
  totals.latent_contacts += oracle.latent_contacts;
  return cuts;
}

Event all_active(Key vertices, u64 n, u64 d = 1) {
  const u16 mask = static_cast<u16>((1u << vertices.size()) - 1);
  return {std::move(vertices), {}, mask, n, d};
}

std::vector<Fixture> fixtures() {
  return {{"empty", 2, {}},
    {"k1_normative", 1, {all_active({0, 1}, 1, 2), all_active({2, 3}, 2, 4),
      all_active({1, 2}, 1), all_active({0, 3}, 3, 2)}},
    {"k2_materialization_without_new_point", 2, {
      {{2, 3}, {0}, 3, 1, 1}, all_active({0, 4, 5}, 2, 2), all_active({1, 2, 3}, 2),
      {{0, 1}, {3}, 3, 3, 1}, all_active({0, 1, 2}, 4), all_active({0, 1, 4}, 5)}},
    {"k2_equal_level_bridge", 2, {all_active({0, 1, 2}, 1, 2), all_active({0, 3, 4}, 2, 4),
      all_active({0, 1, 3}, 3, 2), all_active({0, 2, 4}, 6, 4),
      all_active({1, 2, 3}, 2), all_active({2, 3, 4}, 5, 2)}},
    {"k3_complete_boundary", 3, {all_active({0, 1, 2, 3}, 1), all_active({0, 1, 2, 4}, 2),
      all_active({0, 1, 3, 4}, 3), all_active({0, 2, 3, 4}, 4), all_active({1, 2, 3, 4}, 5)}}};
}

const std::vector<u32> renamed{0xFFFFFFFFu, 7, 131071, 0, 37, 1u << 30};

State unrename(State state) {
  State result;
  for (const auto& carrier : state) {
    Carrier mapped;
    for (auto facet : carrier) {
      for (auto& id : facet) {
        const auto found = std::find(renamed.begin(), renamed.end(), id);
        require(found != renamed.end(), "rename.inverse_domain");
        id = static_cast<u32>(found - renamed.begin());
      }
      std::sort(facet.begin(), facet.end());
      mapped.insert(std::move(facet));
    }
    result.insert(std::move(mapped));
  }
  return result;
}

Fixture variant(Fixture fixture, int version) {
  if (version >= 1) std::reverse(fixture.events.begin(), fixture.events.end());
  if (version >= 2)
    for (auto& event : fixture.events) {
      u16 mask = 0;
      for (size_t s = 0; s < event.support.size(); ++s)
        if (event.active & (1u << s)) mask |= static_cast<u16>(1u << (event.support.size() - 1 - s));
      event.active = mask;
      std::reverse(event.support.begin(), event.support.end());
      std::reverse(event.interior.begin(), event.interior.end());
      event.numerator *= u64{1} << 50;
      event.denominator *= u64{1} << 50;
    }
  if (version >= 3)
    for (auto& event : fixture.events) {
      for (auto& id : event.support) id = renamed.at(id);
      for (auto& id : event.interior) id = renamed.at(id);
    }
  return fixture;
}

std::vector<ForestEvent> encode(const Fixture& fixture) {
  std::vector<ForestEvent> events;
  for (const auto& event : fixture.events) events.push_back(product_event(event));
  return events;
}

int main_cases(const std::string& mutant = "") {
  if (!mutant.empty()) require(mutants_enable(mutant), "unknown.mutant");
  for (const auto& fixture : fixtures()) {
    std::vector<State> baseline;
    for (int version = 0; version < 4; ++version) {
      const auto input = variant(fixture, version);
      const auto oracle = independent_hypergraph(input);
      const auto events = encode(input);
      std::vector<State> classic;
      for (ForestLayout layout : {ForestLayout::kClassic, ForestLayout::kCsr}) {
        const auto forest = build_forest(events, 1, layout, true);
        auto cuts = check_payload(forest, oracle, fixture.k, layout);
        if (layout == ForestLayout::kClassic) classic = cuts;
        else require(classic == cuts, "layouts.cuts");
        if (version == 3)
          for (auto& cut : cuts) { cut = unrename(std::move(cut)); ++totals.rename_checks; }
        if (version == 0 && layout == ForestLayout::kClassic) baseline = cuts;
        else require(cuts == baseline, "permutation_or_reindex.cuts");
      }
    }
  }
  for (int failure = 0; failure < 4; ++failure) {
    std::vector<ForestEvent> invalid{product_event(all_active({0, 1, 2}, 1))};
    if (failure == 0) invalid[0].q = 1;
    if (failure == 1) invalid[0].level.den = 0;
    if (failure == 2) invalid[0].active_mask = 8;
    if (failure == 3) invalid[0].support[1] = invalid[0].support[0];
    for (auto layout : {ForestLayout::kClassic, ForestLayout::kCsr}) {
      const auto result = build_forest(invalid, 1, layout, true);
      require(!result.refusal.empty() && result.delta_count() == 0, "invalid_input.refusal");
      ++totals.invalid_inputs;
    }
  }
  require(totals.streams == 40 && totals.cuts >= 250 && totals.deltas >= 100 &&
          totals.births >= 20 && totals.growths >= 20 && totals.merges >= 20 &&
          totals.continuations >= 20 && totals.no_new_point_growths >= 15 &&
          totals.latent_contacts >= 100 && totals.k1_roots == 32 &&
          totals.rename_checks >= 60 && totals.invalid_inputs == 8, "nonvacuity");
  std::printf("streams=%llu strict_closed_cuts=%llu deltas=%llu births=%llu growths=%llu merges=%llu "
              "omitted_continuations=%llu no_new_point_growths=%llu latent_contacts=%llu k1_initial_roots=%llu "
              "reindexed_cuts=%llu invalid_inputs=%llu\n",
              (unsigned long long)totals.streams, (unsigned long long)totals.cuts, (unsigned long long)totals.deltas,
              (unsigned long long)totals.births, (unsigned long long)totals.growths, (unsigned long long)totals.merges,
              (unsigned long long)totals.continuations, (unsigned long long)totals.no_new_point_growths,
              (unsigned long long)totals.latent_contacts, (unsigned long long)totals.k1_roots,
              (unsigned long long)totals.rename_checks, (unsigned long long)totals.invalid_inputs);
  return mutant.empty() ? 0 : 3;
}

}  // namespace

int main(int argc, char** argv) {
  const std::map<std::string, std::set<std::string>> expected_failures{
      {"binary-ties", {"batch_count"}}, {"repr-ties", {"batch_count"}},
      {"reduced-latent-parent", {"delta.parents"}},
      {"reduced-drop-materialization", {"delta.born", "delta_count"}},
      {"drop-nonmerge", {"delta_count"}}, {"csr-keep-continuation", {"delta_count"}},
      {"csr-stale-output", {"delta.output"}}};
  std::string mutant;
  if (argc == 3 && std::string(argv[1]) == "--mutant") mutant = argv[2];
  else if (argc != 1) return 2;
  if (!mutant.empty() && !expected_failures.count(mutant)) return 2;
  try { return main_cases(mutant); }
  catch (const std::exception& error) {
    const bool targeted = !mutant.empty() && expected_failures.at(mutant).count(error.what());
    std::printf("%s mutant=%s reason=%s completed_streams=%llu\n", targeted ? "killed" : "failure",
                mutant.c_str(), error.what(), (unsigned long long)totals.streams);
    return targeted ? 4 : 1;
  }
}
