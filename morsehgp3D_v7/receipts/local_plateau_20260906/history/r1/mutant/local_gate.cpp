// Private quotient vs exhaustive rational MEB oracle, on small fixtures only.
#include <algorithm>
#include <cstdio>
#include <exception>
#include <set>
#include <string_view>
#include "local_plateau.hpp"
#include "oracle.hpp"
#ifdef MHGP7_TESTING
#error Compile the private quotient against nominal product primitives
#endif
namespace {
using namespace mhgp7;
namespace lp = local_plateau_private;
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
std::array<u64, 5> positive_by_size{};
void compare(const Case& f, const oracle::Model& model, const lp::ShellTable& table) {
  const auto ball = rational_ball(f.key);
  const u32 imask = mask_of(table.census().interior), umask = mask_of(table.census().shell), all = imask | umask;
  const std::size_t p = table.census().interior.size();
  need(model.meb(all).center == ball.center && model.meb(all).radius2 == ball.radius2, "oracle.local_miniball");
  unsigned h = 0, qmin = 99;
  for (unsigned mask = 0; mask < table.contains_center().size(); ++mask) {
    const bool expected = oracle::contains_center(ball.center, f.points, lift(table, static_cast<lp::Mask>(mask), 0));
    need(expected == static_cast<bool>(table.contains_center()[mask]), "quotient.contains_center_table");
    if (expected) qmin = std::min(qmin, static_cast<unsigned>(std::popcount(mask)));
    else h = std::max(h, static_cast<unsigned>(std::popcount(mask)));
  }
  need(h == table.max_strict_cardinality() && qmin == table.q_min(), "quotient.qmin_h");
  for (auto support : table.minimal_supports()) {
    ++positive_by_size[static_cast<std::size_t>(std::popcount(support))];
    for (unsigned bit = 0; bit < table.census().shell.size(); ++bit)
      if (support & (1u << bit)) need(!table.contains_center()[support ^ (1u << bit)], "support.positive_minimal");
  }
  ++tables;
  for (unsigned k = 1; k <= f.points.size() + 1; ++k) {
    const auto rank = table.rank(k);
    const auto strict = model.components(k, ball.radius2, false, all);
    const auto closed = model.components(k, ball.radius2, true, all);
    need(rank.present == !closed.empty() && closed.size() <= 1, "quotient.closed_component");
    need(rank.strict_components.size() == strict.size(), "quotient.strict_component_count");
    if (!rank.present) { need(!rank.no_strict_local_component, "quotient.empty_not_birth"); ++ranks; continue; }
    need(lift(table, rank.closed_shell_cover, p) == all && coverage(closed[0]) == all, "quotient.closed_coverage");
    need(rank.no_strict_local_component == strict.empty(), "quotient.local_birth_threshold");
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
  need(square.rank(2).strict_components.size() == 4 && square.rank(3).strict_components.empty()
       && square.rank(3).closed_shell_cover == 15, "square.four_parents_then_birth_cover4");
  const auto growth = lp::ShellTable::prepare(census(data[3]));
  const auto g = growth.rank(3);
  need(g.strict_components.size() == 1 && lift(growth, g.strict_components[0].shell_cover, 0) == 7
       && lift(growth, g.closed_shell_cover, 0) == 15, "growth.single_local_component_adds_Z");
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
    need(r.analytic_interior_hub && r.strict_components.size() == 1 && r.reduced_vertices == 0
         && r.strict_components[0].interior_prefix == k, "large_p.shared_analytic_representative");
  }
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
}
void rejects(const Case& f) {
  auto c = census(f);
  const auto rejected = [](lp::LocalCensus bad) {
    try { (void)lp::ShellTable::prepare(std::move(bad)); } catch (const std::invalid_argument&) { return true; }
    return false;
  };
  auto bad = c; bad.ball.a = 0; need(rejected(bad), "reject.nonpositive_A");
  bad = c; bad.shell[1].id = bad.shell[0].id; need(rejected(bad), "reject.duplicate_id");
  bad = c; bad.shell[1].position = bad.shell[0].position; need(rejected(bad), "reject.duplicate_position");
  bad = c; bad.shell[0].position.x = 65536; need(rejected(bad), "reject.u16");
  bad = c; bad.interior.push_back(bad.shell.back()); bad.shell.pop_back(); need(rejected(bad), "reject.classification");
  bad = c; while (bad.shell.size() < 13) bad.shell.push_back(bad.shell[0]); need(rejected(bad), "reject.shell_cap");
  const auto table = lp::ShellTable::prepare(c);
  bool zero = false; try { (void)table.rank(0); } catch (const std::invalid_argument&) { zero = true; }
  need(zero, "reject.K_zero");
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
  named(data); noncombinatorial_interior(); maximal_shell(); rejects(data[2]);
  need(tables == 18 && ranks > 80 && components > 0 && representatives == components && inert > 0 && births > 0
       && positive_by_size[2] > 0 && positive_by_size[3] > 0 && positive_by_size[4] > 0, "nonvacuity");
  std::printf("{\"schema\":\"mhgp7-private-local-plateau-v1\",\"status\":\"passed\",\"public_status\":\"not_claimed\","
              "\"tables\":%llu,\"ranks\":%llu,\"components\":%llu,\"representatives\":%llu,\"inert\":%llu,"
              "\"local_births\":%llu,\"supports_q2_q3_q4\":[%llu,%llu,%llu],\"large_p\":5000,"
              "\"external_global_parent_counts\":[2,1]}\n", static_cast<unsigned long long>(tables),
              static_cast<unsigned long long>(ranks), static_cast<unsigned long long>(components),
              static_cast<unsigned long long>(representatives), static_cast<unsigned long long>(inert),
              static_cast<unsigned long long>(births), static_cast<unsigned long long>(positive_by_size[2]),
              static_cast<unsigned long long>(positive_by_size[3]), static_cast<unsigned long long>(positive_by_size[4]));
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
