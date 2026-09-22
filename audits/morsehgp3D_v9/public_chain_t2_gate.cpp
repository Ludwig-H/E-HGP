// Independent audit of the public v9 chain, deliberately outside the engine.
// Compile against the detached d2700314 build, for example:
//   c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -I/workspaces/E-HGP/build/v9-open-worktree -I/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src/gen -I/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include audits/morsehgp3D_v9/public_chain_t2_gate.cpp build/v9-open-worktree/build/v9/libmhgp9_chain.a build/v9-open-worktree/build/v9/libmhgp9_gen.a -o /tmp/public_chain_t2_gate
// The included bounded T2 judge supplies an independent rational MEB/Gamma
// oracle and exact forest reader; no product source is copied into this audit.
#include <morsehgp3D_v9/tests/tower/census_tower_oracle.hpp>
#define local_plateau_oracle census_tower_oracle
#define main mhgp9_inherited_full_ball_gate_main
#include <morsehgp3D_v9/tests/tower/full_ball_tower_gate.cpp>
#undef main
#undef local_plateau_oracle

#include <morsehgp3D_v9/src/chain/tower_chain.hpp>

#include <array>
#include <numeric>
#include <optional>

namespace {

using mhgp9::ChainOptions;
using mhgp9::ChainResult;
using mhgp9::ChainStatus;
u64 public_runs = 0, oracle_catalogue_rows = 0, q4_emissions = 0;

std::vector<P3> q4_without_q3_faces() {
  const P3 origin{20, 20, 20};
  std::vector<P3> points{{30, 30, 30}, {30, 10, 10},
                         {10, 30, 10}, {10, 10, 30}};
  for (size_t i = 0; i < 4; ++i) {
    const P3 v = points[i];
    const P3 sign{(v.x - origin.x) / 10, (v.y - origin.y) / 10,
                  (v.z - origin.z) / 10};
    for (i64 radius : {11, 12})
      points.push_back({origin.x - radius * sign.x,
                        origin.y - radius * sign.y,
                        origin.z - radius * sign.z});
  }
  return points;
}

std::vector<P3> permuted(unsigned variant) {
  const auto base = q4_without_q3_faces();
  std::array<size_t, 12> order{};
  std::iota(order.begin(), order.end(), 0);
  if (variant == 1) std::reverse(order.begin(), order.begin() + 4);
  if (variant == 2) std::rotate(order.begin(), order.begin() + 4, order.end());
  if (variant == 3) std::reverse(order.begin(), order.end());
  std::vector<P3> out;
  for (size_t i : order) out.push_back(base[i]);
  return out;
}

std::vector<BallData> rational_catalogue(const std::vector<P3>& points,
                                         const CloudIndex& ix, unsigned kmax) {
  struct Row { oracle::Ball ball; unsigned qmin; };
  std::map<BallKey, Row> candidates;
  const u32 domain = (u32{1} << points.size()) - 1;
  for (u32 mask = 1; mask <= domain; ++mask) {
    const unsigned q = std::popcount(mask);
    if (q < 2 || q > 4) continue;
    const auto ball = oracle::detail::support_ball(points, mask);
    if (!ball) continue;
    const auto [at, inserted] = candidates.emplace(key(*ball), Row{*ball, q});
    if (!inserted) at->second.qmin = std::min(at->second.qmin, q);
  }
  std::vector<BallData> out;
  for (const auto& [ball_key, row] : candidates) {
    std::vector<i32> interior, shell;
    for (size_t u = 0; u < ix.upos.size(); ++u) {
      const Rat distance = distance2(ix.upos[u], row.ball);
      if (distance < row.ball.radius2) interior.push_back(static_cast<i32>(u));
      else if (distance == row.ball.radius2) shell.push_back(static_cast<i32>(u));
    }
    if (interior.size() + row.qmin > std::min<size_t>(kmax + 1, points.size())) continue;
    need(interior.size() <= kBallInteriorMax && shell.size() <= kBallShellMax,
         "public_T2.oracle_representation");
    BallData ball{};
    ball.key = ball_key;
    ball.level = level(row.ball.radius2);
    ball.arity = static_cast<u8>(row.qmin);
    ball.n_interior = static_cast<u8>(interior.size());
    ball.n_shell = static_cast<u8>(shell.size());
    std::copy(interior.begin(), interior.end(), ball.interior_ids);
    std::copy(shell.begin(), shell.end(), ball.shell_ids);
    out.push_back(ball);
  }
  return out;
}

void compare_inventory(std::vector<BallData> actual,
                       std::vector<BallData> expected) {
  const auto less = [](const BallData& a, const BallData& b) { return a.key < b.key; };
  std::sort(actual.begin(), actual.end(), less);
  std::sort(expected.begin(), expected.end(), less);
  need(!expected.empty() && actual.size() == expected.size(),
       "public_T2.inventory_cardinality");
  for (size_t j = 0; j < expected.size(); ++j) {
    const auto& a = actual[j];
    const auto& e = expected[j];
    need(a.key == e.key && rational(a.level) == rational(e.level) &&
         a.arity == e.arity, "public_T2.ball_key_level_qmin");
    const auto ids = [](std::span<const i32> values) {
      std::vector<i32> out(values.begin(), values.end());
      std::sort(out.begin(), out.end());
      return out;
    };
    need(ids(a.interior()) == ids(e.interior()) &&
         ids(a.shell()) == ids(e.shell()), "public_T2.global_population");
  }
}

void check_sentinel(const std::vector<BallData>& expected,
                    const CloudIndex& ix) {
  const BallKey sentinel{1, {-40, -40, -40}, 900};
  const auto supports = q4_without_q3_faces();
  size_t hits = 0;
  for (const auto& ball : expected) if (ball.key == sentinel) {
    ++hits;
    need(ball.arity == 4 && ball.n_interior == 0 && ball.n_shell == 4 &&
         rational(ball.level) == Rat(300), "public_T2.sentinel_geometry");
    std::vector<P3> shell;
    for (i32 u : ball.shell()) shell.push_back(ix.upos[static_cast<size_t>(u)]);
    for (size_t j = 0; j < 4; ++j)
      need(std::find(shell.begin(), shell.end(), supports[j]) != shell.end(),
           "public_T2.sentinel_shell");
  }
  need(hits == 1, "public_T2.sentinel_unique");
}

void check_public_tower(const ChainResult& actual, const std::vector<P3>& points,
                        const std::vector<InputPoint>& input_points) {
  constexpr unsigned kmax = 3;
  need(actual.orders.size() == kmax && actual.tower.orders.size() == kmax &&
       actual.tower_digest != 0, "public_T2.full_object_present");
  const oracle::Model model(points);
  const u32 domain = (u32{1} << points.size()) - 1;
  std::set<Rat> levels{Rat(0)};
  for (u32 mask = 1; mask <= domain; ++mask) levels.insert(model.meb(mask).radius2);
  levels.insert(*levels.rbegin() + Rat(1));
  std::array<Snapshot, kmax> previous{}, current{};
  for (const Rat& cut : levels) for (bool closed : {false, true}) {
    for (unsigned k = 1; k <= kmax; ++k) {
      current[k - 1] = check_cut(actual.tower.orders[k - 1].forest,
          model.components(k, cut, closed, domain), previous[k - 1],
          input_points, cut, closed);
      if (closed) previous[k - 1] = current[k - 1];
      for (const auto& [facet, root] : current[k - 1]) {
        const auto vertical = full_ball_vertical_root_at(
            actual.tower, k, root, level(cut), closed);
        if (k == 1) need(vertical == kFullCoverageAbsent,
                         "public_T2.vertical_K1_absent");
        else for (u32 bits = facet; bits; bits &= bits - 1) {
          const u32 subfacet = facet & ~(u32{1} << std::countr_zero(bits));
          const auto found = current[k - 2].find(subfacet);
          need(found != current[k - 2].end() && vertical == found->second,
               "public_T2.vertical_subfacet");
        }
      }
    }
  }
}

void run_case(unsigned variant, bool omit_q4, bool all_configs) {
  constexpr unsigned kmax = 3;
  context = "public_variant_" + std::to_string(variant);
  const auto points = permuted(variant);
  std::vector<InputPoint> input_points;
  std::vector<mhgp9::gen::Point3> chain_points;
  for (size_t i = 0; i < points.size(); ++i) {
    const P3 p = points[i];
    input_points.push_back({static_cast<PointId>(i), p});
    chain_points.push_back({static_cast<mhgp9::gen::Coordinate>(p.x),
                            static_cast<mhgp9::gen::Coordinate>(p.y),
                            static_cast<mhgp9::gen::Coordinate>(p.z)});
  }
  const CloudIndex ix = build_cloud_index(input_points);
  need(ix.valid && ix.unique_count() == 12, "public_T2.index_valid");
  auto expected = rational_catalogue(points, ix, kmax);
  check_sentinel(expected, ix);
  oracle_catalogue_rows += expected.size();
  std::optional<ChainResult> baseline;
  for (unsigned s : {8u, 10u, 12u}) for (size_t workers : {size_t{1}, size_t{4}}) {
    if (!all_configs && (s != 8 || workers != 1)) continue;
    ChainOptions options;
    options.kmax = kmax;
    options.separation_s = s;
    options.workers = workers;
    options.run_tower = true;
    options.keep_catalogue = true;
    ChainResult actual = mhgp9::run_tower_chain(chain_points, options);
    need(actual.status == ChainStatus::kComplete,
         "public_T2.chain_complete");
    ++public_runs;
    q4_emissions += actual.q4_emitted;
    need(actual.q4_emitted > 0, "public_T2.q4_emission_nonvacuous");
    if (omit_q4) {
      const BallKey sentinel{1, {-40, -40, -40}, 900};
      const size_t target_count = std::count_if(
          actual.catalogue_balls.begin(), actual.catalogue_balls.end(),
          [&](const BallData& ball) { return ball.key == sentinel; });
      need(target_count == 1, "public_T2.mutant_target_canonical_key");
      const size_t old_size = actual.catalogue_balls.size();
      std::erase_if(actual.catalogue_balls,
                    [&](const BallData& ball) { return ball.key == sentinel; });
      need(actual.catalogue_balls.size() + 1 == old_size,
           "public_T2.mutant_setup");
    }
    compare_inventory(actual.catalogue_balls, expected);
    if (!baseline) {
      check_public_tower(actual, points, input_points);
      baseline = std::move(actual);
    } else {
      need(actual.tower_digest == baseline->tower_digest,
           "public_T2.config_digest");
      same_payload(actual.tower, baseline->tower);
    }
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const bool one = argc == 2 && std::string_view(argv[1]) == "--one";
    const bool mutant = argc == 2 && std::string_view(argv[1]) == "--mutant-omit-q4";
    if (argc != 1 && !one && !mutant) return 2;
    const unsigned variants = one || mutant ? 1 : 4;
    for (unsigned variant = 0; variant < variants; ++variant)
      run_case(variant, mutant, !one && !mutant);
    std::printf("{\"status\":\"passed\",\"scope\":\"public_chain_T2_q4_without_q3\","
                "\"variants\":%u,\"configs_per_variant\":%u,\"public_runs\":%llu,"
                "\"oracle_catalogue_rows\":%llu,\"q4_emissions\":%llu,\"checks\":%llu}\n",
                variants, one || mutant ? 1u : 6u,
                static_cast<unsigned long long>(public_runs),
                static_cast<unsigned long long>(oracle_catalogue_rows),
                static_cast<unsigned long long>(q4_emissions),
                static_cast<unsigned long long>(checks));
    return 0;
  } catch (const Failure& error) {
    std::fprintf(stderr, "FAIL [%s] %s\n", context.c_str(), error.why);
    std::printf("cause=%s\n", error.why);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "EXCEPTION [%s] %s\n", context.c_str(), error.what());
    std::printf("cause=%s\n", error.what());
  }
  return 1;
}
