// Local quotient vs independent exhaustive rational MEB, on small fixtures.
#include <algorithm>
#include <cstdio>
#include <exception>
#include <set>
#include <string_view>
#include "../src/forest/local_plateau.hpp"
#include "../oracle/local_plateau_oracle.hpp"
#ifdef MHGP7_TESTING
#error Compile the local quotient against nominal product primitives
#endif
namespace {
using namespace mhgp7;
namespace lp = local_plateau;
namespace oracle = local_plateau_oracle;
using oracle::Rat;
struct Failure { const char* why; };
void need(bool ok, const char* why) { if (!ok) throw Failure{why}; }
oracle::Int integer(i128 value) {
  const u128 magnitude = uabs128(value);
  oracle::Int out = static_cast<u64>(magnitude >> 64);
  out <<= 64; out += static_cast<u64>(magnitude);
  return value < 0 ? -out : out;
}
oracle::Ball rational_ball(const BallKey& key) {
  oracle::Ball out;
  out.radius2 = -Rat(integer(key.c), integer(key.a));
  for (unsigned axis = 0; axis < 3; ++axis) {
    out.center[axis] = Rat(-integer(key.b[axis]), 2 * integer(key.a));
    out.radius2 += out.center[axis] * out.center[axis];
  }
  return out;
}
Rat distance2(const P3& p, const oracle::Ball& ball) {
  const std::array<i64, 3> point{p.x, p.y, p.z};
  Rat result(0);
  for (unsigned axis = 0; axis < 3; ++axis) {
    const Rat delta = Rat(point[axis]) - ball.center[axis]; result += delta * delta;
  }
  return result;
}
struct Case { const char* name; std::vector<P3> points; BallKey key; };
std::vector<Case> fixtures() {
  return {
    {"triangle", {{0,0,0},{4,0,0},{2,2,0}}, {1,{-4,0,0},0}},
    {"external", {{0,0,0},{4,0,0},{2,2,0},{2,3,0}}, {1,{-4,0,0},0}},
    {"square", {{0,0,0},{2,0,0},{2,2,0},{0,2,0}}, {1,{-2,-2,0},0}},
    {"growth", {{1,8,0},{5,10,0},{9,8,0},{5,0,0}}, {1,{-10,-10,0},25}},
    {"shell7", {{10,5,0},{0,5,0},{5,10,0},{5,0,0},{8,9,0},{2,1,0},{9,8,0}}, {1,{-10,-10,0},25}},
    {"interiors", {{0,2,0},{2,4,0},{4,2,0},{2,0,0},{2,2,0},{2,1,0}}, {1,{-4,-4,0},4}},
    {"support3", {{0,0,0},{2,2,0},{2,0,2}}, {3,{-8,-4,-4},0}},
    {"support4", {{0,0,0},{2,2,0},{2,0,2},{0,2,2}}, {1,{-2,-2,-2},0}},
    {"u16_tetra", {{0,0,0},{65534,65534,0},{65534,0,65534},{0,65534,65534}}, {1,{-65534,-65534,-65534},0}}
  };
}
lp::LocalCensus census(const Case& f) {
  lp::LocalCensus out{f.key, {}, {}};
  const auto ball = rational_ball(f.key);
  for (std::size_t i = 0; i < f.points.size(); ++i) {
    const Rat d = distance2(f.points[i], ball);
    const InputPoint site{static_cast<PointId>(i), f.points[i]};
    if (d < ball.radius2) out.interior.push_back(site);
    else if (d == ball.radius2) out.shell.push_back(site);
  }
  return out;
}
u32 mask_of(const std::vector<InputPoint>& points) {
  u32 result = 0;
  for (const auto& p : points) { need(p.id < 32, "oracle.fixture_id"); result |= u32{1} << p.id; }
  return result;
}
u32 lift(const lp::ShellTable& table, lp::Mask shell, std::size_t interior_prefix) {
  u32 result = 0;
  for (std::size_t i = 0; i < interior_prefix; ++i) result |= u32{1} << table.census().interior[i].id;
  for (unsigned bit = 0; bit < table.census().shell.size(); ++bit)
    if (shell & (1u << bit)) result |= u32{1} << table.census().shell[bit].id;
  return result;
}
u32 coverage(const std::vector<u32>& component) { u32 out = 0; for (u32 f : component) out |= f; return out; }
u64 ranks = 0, tables = 0, components = 0, representatives = 0, inert = 0, births = 0;
u64 complete_support_sets = 0, diameter_hubs = 0, dsu_ranks = 0, u2_exclusions = 0;
std::array<u64, 5> positive_by_size{};
void physical_work(const lp::ShellTable& table, const lp::LocalRank& rank) {
  const auto p = table.census().interior.size(), u = table.census().shell.size();
  const bool shortcut = rank.present && rank.k > p && rank.k - p == 1 && u >= 3 && table.q_min() == 2;
  need(shortcut || !rank.analytic_diameter_hub, "diameter_hub.forbidden_domain");
  need(rank.analytic_diameter_hub == shortcut, "diameter_hub.dispatch");
  if (shortcut) {
    need(rank.reduced_vertices == u && rank.strict_cofaces == 0 && rank.union_attempts == 0
         && rank.dsu_mask_slots == 0, "diameter_hub.no_DSU_work");
    need(rank.strict_components.size() == 1 && !rank.analytic_interior_hub
         && rank.contribution_shell == 0 && !rank.contribution_interior, "diameter_hub.single_full_cover");
    const auto& component = rank.strict_components.front();
    need(component.interior_prefix == p && component.representative_shell == 1
         && component.shell_cover == rank.closed_shell_cover && component.reduced_members.size() == u,
         "diameter_hub.canonical_representative");
    for (unsigned bit = 0; bit < u; ++bit)
      need(component.reduced_members[bit] == (1u << bit), "diameter_hub.singleton_members");
    ++diameter_hubs;
  } else if (rank.present && rank.k > p) {
    const unsigned t = static_cast<unsigned>(rank.k - p);
    u64 vertices = 0, cofaces = 0;
    for (unsigned mask = 0; mask < table.contains_center().size(); ++mask)
      if (!table.contains_center()[mask]) {
        vertices += std::popcount(mask) == static_cast<int>(t);
        cofaces += std::popcount(mask) == static_cast<int>(t + 1);
      }
    need(rank.reduced_vertices == vertices && rank.strict_cofaces == cofaces
         && rank.union_attempts == cofaces * t && rank.dsu_mask_slots == 2 * table.contains_center().size(),
         "general_DSU.physical_work");
    ++dsu_ranks;
    if (u == 2 && t == 1) {
      need(rank.strict_components.size() == 2, "diameter_hub.u2_two_components");
      ++u2_exclusions;
    }
  } else {
    need(rank.reduced_vertices == 0 && rank.strict_cofaces == 0 && rank.union_attempts == 0
         && rank.dsu_mask_slots == 0, "analytic_or_empty.no_DSU_work");
  }
}
void compare(const Case& f, const oracle::Model& model, const lp::ShellTable& table) {
  const auto ball = rational_ball(f.key);
  const u32 imask = mask_of(table.census().interior), umask = mask_of(table.census().shell), all = imask | umask;
  const std::size_t p = table.census().interior.size();
  need(model.meb(all).center == ball.center && model.meb(all).radius2 == ball.radius2, "oracle.local_miniball");
  unsigned h = 0, qmin = 99;
  const unsigned mask_count = 1u << table.census().shell.size();
  need(table.contains_center().size() == mask_count, "quotient.table_domain");
  std::vector<u8> gram_contains(mask_count, 0);
  for (unsigned mask = 0; mask < mask_count; ++mask) {
    const bool expected = oracle::contains_center(ball.center, f.points, lift(table, static_cast<lp::Mask>(mask), 0));
    gram_contains[mask] = expected;
    need(expected == static_cast<bool>(table.contains_center()[mask]), "quotient.contains_center_table");
    if (expected) qmin = std::min(qmin, static_cast<unsigned>(std::popcount(mask)));
    else h = std::max(h, static_cast<unsigned>(std::popcount(mask)));
  }
  need(h == table.max_strict_cardinality() && qmin == table.q_min(), "quotient.qmin_h");
  std::set<lp::Mask> gram_minima;
  for (unsigned mask = 1; mask < mask_count; ++mask) if (gram_contains[mask]) {
    bool minimal = true;
    for (unsigned bit = 0; bit < table.census().shell.size(); ++bit)
      if (mask & (1u << bit)) minimal = minimal && !gram_contains[mask ^ (1u << bit)];
    if (minimal) gram_minima.insert(static_cast<lp::Mask>(mask));
  }
  const std::set<lp::Mask> actual_supports(table.minimal_supports().begin(), table.minimal_supports().end());
  need(actual_supports.size() == table.minimal_supports().size() && actual_supports == gram_minima,
       "support.complete_Gram_minima_set");
  ++complete_support_sets;
  for (auto support : table.minimal_supports()) {
    ++positive_by_size[static_cast<std::size_t>(std::popcount(support))];
    for (unsigned bit = 0; bit < table.census().shell.size(); ++bit)
      if (support & (1u << bit)) need(!table.contains_center()[support ^ (1u << bit)], "support.positive_minimal");
  }
  ++tables;
  for (unsigned k = 1; k <= f.points.size() + 1; ++k) {
    const auto rank = table.rank(k);
    physical_work(table, rank);
    const auto strict = model.components(k, ball.radius2, false, all);
    const auto closed = model.components(k, ball.radius2, true, all);
    need(rank.present == !closed.empty() && closed.size() <= 1, "quotient.closed_component");
    need(rank.strict_components.size() == strict.size(), "quotient.strict_component_count");
    if (!rank.present) { need(!rank.no_strict_local_component, "quotient.empty_not_birth"); ++ranks; continue; }
    need(lift(table, rank.closed_shell_cover, p) == all && coverage(closed[0]) == all, "quotient.closed_coverage");
    need(rank.no_strict_local_component == strict.empty(), "quotient.local_birth_threshold");
    u32 strict_coverage = 0;
    for (const auto& component : strict) strict_coverage |= coverage(component);
    need(lift(table, rank.contribution_shell, rank.contribution_interior ? p : 0) == (all & ~strict_coverage),
         "quotient.local_contribution_not_global_delta");
    if (rank.no_strict_local_component) ++births;
    if (rank.inert_sufficient) {
      need(strict.size() == 1 && coverage(strict[0]) == all, "quotient.inert_coverage"); ++inert;
    }
    std::vector<bool> matched(strict.size(), false);
    for (const auto& component : rank.strict_components) {
      const u32 representative = lift(table, component.representative_shell, component.interior_prefix);
      need(std::popcount(representative) == static_cast<int>(k), "quotient.representative_cardinality");
      std::size_t found = strict.size();
      for (std::size_t j = 0; j < strict.size(); ++j)
        if (std::find(strict[j].begin(), strict[j].end(), representative) != strict[j].end()) found = j;
      need(found < strict.size() && !matched[found], "quotient.representative_bijection");
      matched[found] = true;
      need(lift(table, component.shell_cover, p) == coverage(strict[found]), "quotient.component_coverage");
      if (k > p) {
        std::set<lp::Mask> projection;
        for (u32 facet : strict[found]) {
          lp::Mask shell = 0;
          for (unsigned bit = 0; bit < table.census().shell.size(); ++bit)
            if (facet & (u32{1} << table.census().shell[bit].id)) shell |= static_cast<lp::Mask>(1u << bit);
          for (unsigned a = shell; a != 0; a = (a - 1) & shell)
            if (std::popcount(a) == static_cast<int>(k - p)) projection.insert(static_cast<lp::Mask>(a));
        }
        need(std::vector<lp::Mask>(projection.begin(), projection.end()) == component.reduced_members,
             "quotient.exact_projected_component");
      } else need(rank.analytic_interior_hub && component.reduced_members.empty() && rank.reduced_vertices == 0,
                  "quotient.no_interior_combinations");
      ++components; ++representatives;
    }
    ++ranks;
  }
}
void named(const std::vector<Case>& data) {
  const auto square = lp::ShellTable::prepare(census(data[2]));
  need(square.minimal_supports() == std::vector<lp::Mask>{5,10}, "square.two_diameters_not_one");
  need(square.rank(2).strict_components.size() == 4 && square.rank(3).strict_components.empty()
       && square.rank(3).closed_shell_cover == 15, "square.four_parents_then_birth_cover4");
  const auto growth = lp::ShellTable::prepare(census(data[3]));
  const auto g = growth.rank(3);
  need(g.strict_components.size() == 1 && lift(growth, g.strict_components[0].shell_cover, 0) == 7
       && lift(growth, g.closed_shell_cover, 0) == 15 && g.contribution_shell == 8
       && !g.contribution_interior, "growth.single_local_component_adds_Z");
  const auto tri = lp::ShellTable::prepare(census(data[0])), ext = lp::ShellTable::prepare(census(data[1]));
  const auto local_a = tri.rank(2), local_b = ext.rank(2);
  need(local_a.strict_components.size() == 2 && local_b.strict_components.size() == 2
       && tri.contains_center() == ext.contains_center(), "external.same_local_quotient");
  std::array<unsigned, 2> parent_counts{};
  for (unsigned which = 0; which < 2; ++which) {
    const auto& f = data[which]; const auto& table = which ? ext : tri;
    oracle::Model model(f.points);
    const auto global = model.components(2, Rat(4), false, (u32{1} << f.points.size()) - 1);
    std::set<std::size_t> images;
    for (const auto& component : table.rank(2).strict_components) {
      const auto representative = lift(table, component.representative_shell, 0);
      for (std::size_t j = 0; j < global.size(); ++j)
        if (std::find(global[j].begin(), global[j].end(), representative) != global[j].end()) images.insert(j);
    }
    parent_counts[which] = static_cast<unsigned>(images.size());
  }
  need(parent_counts == std::array<unsigned, 2>{2, 1}, "external.local_is_not_global_parents");
  const auto window = lp::ShellTable::prepare(census(data[4]));
  need(window.rank(5).present && window.rank(5).no_strict_local_component
       && window.rank(5).closed_shell_cover == 127, "shell7.no_window_omission");
}
void noncombinatorial_interior() {
  lp::LocalCensus c{{1,{-65534,0,0},0}, {}, {{5000,{0,0,0}},{5001,{65534,0,0}}}};
  for (u32 i = 0; i < 5000; ++i) c.interior.push_back({i,{static_cast<i64>(i + 1),0,0}});
  const auto table = lp::ShellTable::prepare(std::move(c));
  need(table.contains_center().size() == 4 && table.q_min() == 2 && table.max_strict_cardinality() == 1,
       "large_p.constant_shell_table");
  for (std::size_t k : {1u, 2500u, 5000u}) {
    const auto r = table.rank(k);
    physical_work(table, r);
    need(r.analytic_interior_hub && r.strict_components.size() == 1 && r.reduced_vertices == 0
         && r.strict_components[0].interior_prefix == k, "large_p.shared_analytic_representative");
  }
  physical_work(table, table.rank(5001));
  need(table.rank(5001).reduced_vertices == 2 && table.rank(5002).no_strict_local_component
       && !table.rank(5003).present, "large_p.upper_ranks");
}
void maximal_shell() {
  const std::vector<P3> points{{10,5,0},{9,8,0},{8,9,0},{5,10,0},{2,9,0},{1,8,0},
                             {0,5,0},{1,2,0},{2,1,0},{5,0,0},{8,1,0},{9,2,0}};
  lp::LocalCensus c{{1,{-10,-10,0},25}, {}, {}};
  for (std::size_t i = 0; i < points.size(); ++i) c.shell.push_back({static_cast<PointId>(i), points[i]});
  const auto table = lp::ShellTable::prepare(std::move(c));
  need(table.contains_center().size() == 4096 && table.q_min() == 2 && table.max_strict_cardinality() == 6
       && table.upward_steps() == 12 * 2048, "shell12.table_boundary");
  need(table.rank(1).strict_components.size() == 1 && table.rank(6).strict_components.size() == 12
       && table.rank(7).no_strict_local_component && table.rank(12).present && !table.rank(13).present,
       "shell12.all_ranks_domain");
  physical_work(table, table.rank(1));
  physical_work(table, table.rank(6));
}
void rejects(const Case& f) {
  auto c = census(f);
  const auto rejected = [](lp::LocalCensus bad, const char* reason) {
    try { (void)lp::ShellTable::prepare(std::move(bad)); }
    catch (const std::invalid_argument& e) { return std::string_view(e.what()) == reason; }
    return false;
  };
  auto bad = c; bad.ball.a = 0; need(rejected(bad, "local.BallKey_A_bound"), "reject.nonpositive_A");
  bad = c; bad.shell[1].id = bad.shell[0].id; need(rejected(bad, "local.distinct_ids"), "reject.duplicate_id");
  bad = c; bad.shell[1].position = bad.shell[0].position; need(rejected(bad, "local.distinct_positions"), "reject.duplicate_position");
  bad = c; bad.shell[0].position.x = 65536; need(rejected(bad, "local.u16_positions"), "reject.u16");
  bad = c; bad.interior.push_back(bad.shell.back()); bad.shell.pop_back(); need(rejected(bad, "local.census_classification"), "reject.classification");
  bad = c; while (bad.shell.size() < 13) bad.shell.push_back(bad.shell[0]); need(rejected(bad, "local.shell_size_2_to_12"), "reject.shell_cap");
  bad = c; bad.ball.b[0] = static_cast<i128>(1) << 87; need(rejected(bad, "local.BallKey_B_bound"), "reject.B_bound");
  bad = c; bad.ball.c = static_cast<i128>(1) << 105; need(rejected(bad, "local.BallKey_C_bound"), "reject.C_bound");
  // The qualified wider coefficient domain reaches the exact power check;
  // these malformed censuses are NOT claimed to be geometric MEB fixtures.
  bad = c; bad.ball.b[0] = (static_cast<i128>(1) << 87) - 1;
  need(rejected(bad, "local.census_classification"), "reject.wide_B_reaches_power");
  bad = c; bad.ball.c = (static_cast<i128>(1) << 105) - 1;
  need(rejected(bad, "local.census_classification"), "reject.wide_C_reaches_power");
  bad = c; bad.ball.a *= 2; for (auto& b : bad.ball.b) b *= 2; bad.ball.c *= 2;
  need(rejected(bad, "local.BallKey_primitive"), "reject.nonprimitive_key");
  bad = c; bad.shell.resize(2); need(rejected(bad, "local.shell_defines_miniball"), "reject.hemisphere_not_MEB");
  const auto table = lp::ShellTable::prepare(c);
  bool zero = false; try { (void)table.rank(0); } catch (const std::invalid_argument&) { zero = true; }
  need(zero, "reject.K_zero");
}
// Real 50k extraction; literal rational expectations, no runtime oracle.
// Source: build/v7_extra_shell_20260906/run_r3/n50000_k10.stderr
// SHA256 3cd74b330c62978d8c3eedd175e12bf5fe02893facb2e008150c32b5054aea72
// Judge: build/v7_extra_shell_read_20260906_r3/checks/normal.stdout
// SHA256 c4a066e620b7850b6b3f1937f5b6d92b027f763012a554f9d1fbbf5512cc3c81
// IDs are original external PointIds, never bit positions or geometry indices.
// Included inside the gate namespace: lp, mhgp7 types and need are visible.
void real_fixtures() {
  struct ExpectedComponent {
    std::vector<PointId> representative, coverage;
    std::vector<lp::Mask> members;
    bool analytic;
  };
  struct ExpectedRank {
    bool present, inert;
    std::vector<ExpectedComponent> components;
  };
  struct Fixture {
    u64 ball_index;
    lp::LocalCensus sites;
    std::vector<PointId> interior_ids, shell_ids, closed_coverage;
    std::vector<u8> contains;
    unsigned qmin, h;
    std::vector<lp::Mask> supports;
    std::vector<ExpectedRank> expected;
  };
  const std::vector<Fixture> fixtures{
    {174406,
     {{1,{-117436,-63637,-105474},7237823525},
      {{27569,{59112,31775,52793}},{12055,{58267,32440,51837}},{46679,{59595,30228,52912}}},
      {{46707,{60349,31678,51784}},{36860,{57087,31959,53690}},{42779,{56857,31903,52394}}}},
     {12055,27569,46679},{36860,42779,46707},
     {12055,27569,36860,42779,46679,46707},
     {0,0,0,0,0,1,0,1},2,2,{5}, {
       {true,true,{{{12055},{12055,27569,36860,42779,46679,46707},{},true}}},  // K=1
       {true,true,{{{12055,27569},{12055,27569,36860,42779,46679,46707},{},true}}},  // K=2
       {true,true,{{{12055,27569,46679},{12055,27569,36860,42779,46679,46707},{},true}}},  // K=3
       {true,false,{{{12055,27569,36860,46679},{12055,27569,36860,42779,46679,46707},{1,2,4},false}}},  // K=4
       {true,false,{{{12055,27569,36860,42779,46679},{12055,27569,36860,42779,46679},{3},false},{{12055,27569,42779,46679,46707},{12055,27569,42779,46679,46707},{6},false}}},  // K=5
       {true,false,{}},  // K=6
       {false,false,{}},  // K=7
       {false,false,{}},  // K=8
       {false,false,{}},  // K=9
       {false,false,{}}  // K=10
     }},
    {254569,
     {{1,{-111935,-55901,-93422},6094057006},
      {},
      {{34292,{56652,28927,46884}},{32276,{56657,27037,47088}},{4912,{55283,26974,46538}}}},
     {},{4912,32276,34292},
     {4912,32276,34292},
     {0,0,0,0,0,1,0,1},2,2,{5}, {
       {true,false,{{{4912},{4912,32276,34292},{1,2,4},false}}},  // K=1
       {true,false,{{{4912,32276},{4912,32276},{3},false},{{32276,34292},{32276,34292},{6},false}}},  // K=2
       {true,false,{}},  // K=3
       {false,false,{}},  // K=4
       {false,false,{}},  // K=5
       {false,false,{}},  // K=6
       {false,false,{}},  // K=7
       {false,false,{}},  // K=8
       {false,false,{}},  // K=9
       {false,false,{}}  // K=10
     }},
    {996863,
     {{1,{-60899,-109188,-104077},6612344594},
      {{34559,{30002,56227,52178}},{23184,{31361,53742,52379}},{42468,{28688,54484,51573}},{23681,{30692,53074,52590}}},
      {{43571,{31658,55904,51637}},{25389,{31694,53596,52929}},{40661,{29205,55592,51148}}}},
     {23184,23681,34559,42468},{25389,40661,43571},
     {23184,23681,25389,34559,40661,42468,43571},
     {0,0,0,1,0,0,0,1},2,2,{3}, {
       {true,true,{{{23184},{23184,23681,25389,34559,40661,42468,43571},{},true}}},  // K=1
       {true,true,{{{23184,23681},{23184,23681,25389,34559,40661,42468,43571},{},true}}},  // K=2
       {true,true,{{{23184,23681,34559},{23184,23681,25389,34559,40661,42468,43571},{},true}}},  // K=3
       {true,true,{{{23184,23681,34559,42468},{23184,23681,25389,34559,40661,42468,43571},{},true}}},  // K=4
       {true,false,{{{23184,23681,25389,34559,42468},{23184,23681,25389,34559,40661,42468,43571},{1,2,4},false}}},  // K=5
       {true,false,{{{23184,23681,25389,34559,42468,43571},{23184,23681,25389,34559,42468,43571},{5},false},{{23184,23681,34559,40661,42468,43571},{23184,23681,34559,40661,42468,43571},{6},false}}},  // K=6
       {true,false,{}},  // K=7
       {false,false,{}},  // K=8
       {false,false,{}},  // K=9
       {false,false,{}}  // K=10
     }},
    {1251653,
     {{1,{-43377,-128636,-55885},5381668141},
      {{16188,{22931,64793,28880}},{49421,{23855,63782,28680}},{9003,{21594,64060,29969}},{49148,{23000,65217,26759}},{8622,{23415,62995,28142}},{13287,{20792,62287,28443}},{49620,{21771,62035,27236}},{14710,{22530,64159,26323}},{49945,{21328,64582,25645}}},
      {{44066,{23583,64771,29529}},{48544,{22951,63283,29852}},{27084,{19794,63865,26356}}}},
     {8622,9003,13287,14710,16188,49148,49421,49620,49945},{27084,44066,48544},
     {8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},
     {0,0,0,1,0,0,0,1},2,2,{3}, {
       {true,true,{{{8622},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{},true}}},  // K=1
       {true,true,{{{8622,9003},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{},true}}},  // K=2
       {true,true,{{{8622,9003,13287},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{},true}}},  // K=3
       {true,true,{{{8622,9003,13287,14710},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{},true}}},  // K=4
       {true,true,{{{8622,9003,13287,14710,16188},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{},true}}},  // K=5
       {true,true,{{{8622,9003,13287,14710,16188,49148},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{},true}}},  // K=6
       {true,true,{{{8622,9003,13287,14710,16188,49148,49421},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{},true}}},  // K=7
       {true,true,{{{8622,9003,13287,14710,16188,49148,49421,49620},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{},true}}},  // K=8
       {true,true,{{{8622,9003,13287,14710,16188,49148,49421,49620,49945},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{},true}}},  // K=9
       {true,false,{{{8622,9003,13287,14710,16188,27084,49148,49421,49620,49945},{8622,9003,13287,14710,16188,27084,44066,48544,49148,49421,49620,49945},{1,2,4},false}}}  // K=10
     }}
  };
  const auto ids = [](const std::vector<InputPoint>& sites) {
    std::vector<PointId> out;
    for (const auto& site : sites) out.push_back(site.id);
    return out;
  };
  const auto lift_ids = [](const lp::ShellTable& table, lp::Mask mask, std::size_t prefix) {
    need(prefix <= table.census().interior.size(), "real.representative_prefix_domain");
    need(mask < (1u << table.census().shell.size()), "real.shell_mask_domain");
    std::vector<PointId> out;
    for (std::size_t i = 0; i < prefix; ++i) out.push_back(table.census().interior[i].id);
    for (std::size_t bit = 0; bit < table.census().shell.size(); ++bit)
      if (mask & (1u << bit)) out.push_back(table.census().shell[bit].id);
    std::sort(out.begin(), out.end());
    return out;
  };
  unsigned checked_tables = 0, checked_ranks = 0, potential_anchors = 0;
  for (const auto& fixture : fixtures) {
    const auto table = lp::ShellTable::prepare(fixture.sites);
    need(table.census().ball == fixture.sites.ball, "real.original_BallKey");
    need(ids(table.census().interior) == fixture.interior_ids &&
         ids(table.census().shell) == fixture.shell_ids, "real.original_external_ids");
    need(table.contains_center() == fixture.contains, "real.rational_contains_table");
    need(table.q_min() == fixture.qmin && table.max_strict_cardinality() == fixture.h &&
         table.minimal_supports() == fixture.supports, "real.rational_positive_supports");
    need(fixture.expected.size() == 10, "real.ten_literal_ranks");
    const auto p = table.census().interior.size();
    for (std::size_t k = 1; k <= 10; ++k) {
      const auto rank = table.rank(k);
      physical_work(table, rank);
      const auto& expected = fixture.expected[k - 1];
      need(rank.k == k && rank.present == expected.present && rank.inert_sufficient == expected.inert,
           "real.rational_rank_classification");
      need(rank.strict_components.size() == expected.components.size() &&
           rank.no_strict_local_component == (expected.present && expected.components.empty()),
           "real.rational_strict_components");
      const bool analytic = !expected.components.empty() && expected.components[0].analytic;
      need(rank.analytic_interior_hub == analytic, "real.rational_analytic_hub");
      if (expected.present)
        need(lift_ids(table, rank.closed_shell_cover, p) == fixture.closed_coverage,
             "real.rational_closed_coverage");
      else need(rank.closed_shell_cover == 0, "real.empty_block_not_birth");
      u64 expected_vertices = 0;
      for (std::size_t j = 0; j < expected.components.size(); ++j) {
        const auto& actual = rank.strict_components[j];
        const auto& wanted = expected.components[j];
        need(lift_ids(table, actual.representative_shell, actual.interior_prefix) == wanted.representative,
             "real.rational_representative");
        need(actual.interior_prefix == (wanted.analytic ? wanted.representative.size() : p),
             "real.rational_interior_prefix");
        need(lift_ids(table, actual.shell_cover, p) == wanted.coverage &&
             actual.reduced_members == wanted.members, "real.rational_coverage_and_members");
        expected_vertices += wanted.members.size();
      }
      need(rank.reduced_vertices == expected_vertices, "real.no_hidden_interior_enumeration");
      std::vector<PointId> contribution;
      if (expected.present) for (PointId id : fixture.closed_coverage) {
        bool covered = false;
        for (const auto& wanted : expected.components)
          covered = covered || std::find(wanted.coverage.begin(), wanted.coverage.end(), id) != wanted.coverage.end();
        if (!covered) contribution.push_back(id);
      }
      need(lift_ids(table, rank.contribution_shell, rank.contribution_interior ? p : 0) == contribution,
           "real.rational_local_contribution");
      if (fixture.ball_index == 1251653 && k == 10) {
        // p=9: connected and covering S, but 45 facets arrive at this level.
        // Keep a possible anchor: this does NOT qualify global parent lookup.
        need(p == 9 && rank.present && !rank.inert_sufficient && !rank.analytic_interior_hub &&
             rank.strict_components.size() == 1 &&
             lift_ids(table, rank.strict_components[0].shell_cover, p) == fixture.closed_coverage &&
             rank.strict_components[0].reduced_members == std::vector<lp::Mask>{1,2,4},
             "real.p9_K10_potential_anchor_not_omittable_by_inertness");
        ++potential_anchors;
      }
      ++checked_ranks;
    }
    ++checked_tables;
  }
  need(checked_tables == 4 && checked_ranks == 40 && potential_anchors == 1, "real.nonvacuity_4x10");
}

int run() {
  const auto data = fixtures();
  for (const auto& f : data) {
    oracle::Model model(f.points);
    for (unsigned perm = 0; perm < 2; ++perm) {
      auto c = census(f);
      if (perm) { std::reverse(c.interior.begin(), c.interior.end()); std::reverse(c.shell.begin(), c.shell.end()); }
      compare(f, model, lp::ShellTable::prepare(std::move(c)));
    }
  }
  named(data); noncombinatorial_interior(); maximal_shell(); rejects(data[2]); real_fixtures();
  need(tables == 18 && ranks > 80 && components > 0 && representatives == components && inert > 0 && births > 0
       && positive_by_size[2] > 0 && positive_by_size[3] > 0 && positive_by_size[4] > 0
       && complete_support_sets == 18 && diameter_hubs > 4 && dsu_ranks > 0 && u2_exclusions == 1, "nonvacuity");
  std::printf("{\"schema\":\"mhgp7-local-plateau-v1\",\"status\":\"passed\",\"public_status\":\"not_claimed\","
              "\"tables\":%llu,\"ranks\":%llu,\"components\":%llu,\"representatives\":%llu,\"inert\":%llu,"
              "\"local_births\":%llu,\"supports_q2_q3_q4\":[%llu,%llu,%llu],\"large_p\":5000,"
              "\"external_global_parent_counts\":[2,1],\"real_tables\":4,\"real_ranks\":40,"
              "\"complete_support_sets\":%llu,\"diameter_hubs\":%llu,\"dsu_ranks\":%llu,\"u2_exclusions\":%llu}\n", static_cast<unsigned long long>(tables),
              static_cast<unsigned long long>(ranks), static_cast<unsigned long long>(components),
              static_cast<unsigned long long>(representatives), static_cast<unsigned long long>(inert),
              static_cast<unsigned long long>(births), static_cast<unsigned long long>(positive_by_size[2]),
              static_cast<unsigned long long>(positive_by_size[3]), static_cast<unsigned long long>(positive_by_size[4]),
              static_cast<unsigned long long>(complete_support_sets), static_cast<unsigned long long>(diameter_hubs),
              static_cast<unsigned long long>(dsu_ranks), static_cast<unsigned long long>(u2_exclusions));
  return 0;
}
}  // namespace
int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try { return run(); }
  catch (const Failure& e) { std::fprintf(stderr, "local plateau rejected: %s\n", e.why); }
  catch (const std::exception& e) { std::fprintf(stderr, "local plateau exception: %s\n", e.what()); }
  return 1;
}
