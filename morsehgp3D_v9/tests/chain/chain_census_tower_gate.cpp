// Permanent bounded census -> FULL qualification; product receives only the real census.
// Derived from T2 13f06875: no product code or exhaustive oracle is moved into src/.
#include "../tower/census_tower_oracle.hpp"
#define main mhgp9_inherited_bounded_census_gate_main
#define local_plateau_oracle census_tower_oracle
#include "../tower/full_ball_tower_gate.cpp"
#undef local_plateau_oracle
#undef main
#include "../../src/chain/tower_chain.hpp"

namespace {
u64 oracle_mebs = 0, oracle_components = 0, catalogue_rows = 0, tower_pairs = 0;
u64 census_runs = 0, high_order_facets = 0, high_order_verticals = 0, shell12 = 0;
u64 q3_rows = 0, q4_rows = 0, euler_runs = 0;
oracle::Mutation oracle_mutation = oracle::Mutation::none;
bool omit_census_ball = false;

void compare_historical_oracle() {
  for (const auto& fixture : fixtures()) {
    context = std::string("historical/") + fixture.name;
    std::fprintf(stderr, "start=%s\n", context.c_str()); std::fflush(stderr);
    const local_plateau_oracle::Model historical(fixture.points);
    const oracle::Model trial(fixture.points, oracle_mutation);
    const u32 domain = (u32{1} << fixture.points.size()) - 1;
    std::set<Rat> levels{Rat(0)};
    for (u32 mask = 1; mask <= domain; ++mask) {
      const auto a = historical.meb(mask), b = trial.meb(mask);
      need(a.center == b.center && a.radius2 == b.radius2, "T2.historical.exact_MEB");
      levels.insert(a.radius2); ++oracle_mebs;
    }
    levels.insert(*levels.rbegin() + Rat(1));
    // Exhaustive cut/K queries on the full cloud and every single-site deletion.
    std::vector<u32> allowed{domain};
    for (size_t bit = 0; bit < fixture.points.size(); ++bit)
      allowed.push_back(domain & ~(u32{1} << bit));
    for (u32 subset : allowed) for (unsigned k = 1; k <= fixture.points.size(); ++k)
      for (const auto& cut : levels) for (bool closed : {false, true}) {
        need(historical.components(k, cut, closed, subset) ==
            trial.components(k, cut, closed, subset), "T2.historical.exact_Gamma");
        ++oracle_components;
      }
  }
  need(oracle_mebs > 1000 && oracle_components > 10000, "T2.historical.nonvacuity");
}

std::vector<InputPoint> large_input(const Fixture& fixture, unsigned variant) {
  std::vector<InputPoint> result;
  for (size_t j = 0; j < fixture.points.size(); ++j)
    result.push_back({variant ? static_cast<PointId>(4294967295u - 100003u*j) :
        static_cast<PointId>(j), fixture.points[j]});
  if (variant) std::reverse(result.begin(), result.end());
  return result;
}

std::vector<BallData> independent_catalogue(const std::vector<P3>& points,
    const CloudIndex& ix, unsigned kmax) {
  struct Row { oracle::Ball ball; unsigned qmin; };
  std::map<BallKey, Row> candidates;
  const u32 domain = (u32{1} << points.size()) - 1;
  for (u32 mask = 1; mask <= domain; ++mask) {
    const unsigned q = std::popcount(mask);
    if (q < 2 || q > 4) continue;
    const auto ball = oracle::detail::support_ball(points, mask);
    if (!ball) continue;
    const auto [found, inserted] = candidates.emplace(key(*ball), Row{*ball, q});
    if (!inserted) found->second.qmin = std::min(found->second.qmin, q);
  }
  std::vector<BallData> result;
  for (const auto& [ball_key, row] : candidates) {
    std::vector<i32> interior, shell;
    for (size_t j = 0; j < ix.upos.size(); ++j) {
      const auto d = distance2(ix.upos[j], row.ball);
      if (d < row.ball.radius2) interior.push_back(static_cast<i32>(j));
      else if (d == row.ball.radius2) shell.push_back(static_cast<i32>(j));
    }
    if (interior.size() + row.qmin > std::min<size_t>(kmax+1, points.size())) continue;
    need(interior.size() <= kBallInteriorMax && shell.size() <= kBallShellMax,
         "T2.oracle.admitted_census_representation");
    BallData out{}; out.key = ball_key; out.level = level(row.ball.radius2);
    out.arity = static_cast<u8>(row.qmin);
    out.n_interior = static_cast<u8>(interior.size()); out.n_shell = static_cast<u8>(shell.size());
    std::copy(interior.begin(), interior.end(), out.interior_ids);
    std::copy(shell.begin(), shell.end(), out.shell_ids);
    result.push_back(out);
  }
  return result;
}

unsigned chain_workers = 1;
std::vector<BallData> actual_census(const CloudIndex& ix, unsigned kmax, int s) {
  // Chaine v9 reelle : generateur exact v8 porte -> catalogue recoupe. Les
  // indices geometriques ne dependent que des positions distinctes : ils sont
  // communs a l'index de cette porte et a celui de la chaine.
  std::vector<mhgp9::gen::Point3> points;
  for (i32 u = 0; u < ix.unique_count(); ++u) {
    const auto& p = ix.upos[static_cast<size_t>(u)];
    points.push_back({static_cast<mhgp9::gen::Coordinate>(p.x), static_cast<mhgp9::gen::Coordinate>(p.y),
                      static_cast<mhgp9::gen::Coordinate>(p.z)});
  }
  mhgp9::ChainOptions options;
  options.kmax = kmax; options.separation_s = static_cast<unsigned>(s);
  options.workers = chain_workers; options.run_tower = false; options.keep_catalogue = true;
  auto chain = mhgp9::run_tower_chain(points, options);
  if (chain.status != mhgp9::ChainStatus::kComplete)
    std::fprintf(stderr, "chain refusal=%s\n", chain.reason.c_str());
  need(chain.status == mhgp9::ChainStatus::kComplete, "T2.chain.accepted");
  // Euler, condition necessaire publiee par la chaine, sur un catalogue que
  // cette porte juge exhaustivement : sommes egales a 1 pour K <= min(Kmax-2, n).
  need(chain.catalogue.euler_status == mhgp9::EulerStatus::kHolds &&
       chain.catalogue.euler_checkable_max_k == std::min<unsigned>(kmax - 2, static_cast<unsigned>(points.size())),
       "T2.chain.euler_invariant");
  ++euler_runs;
  auto balls = std::move(chain.catalogue_balls);
  std::sort(balls.begin(), balls.end(), [](const auto& a, const auto& b) { return a.key < b.key; });
  if (omit_census_ball && !balls.empty()) balls.pop_back();
  ++census_runs;
  return balls;
}

void compare_catalogue(const std::vector<BallData>& a, const std::vector<BallData>& b) {
  const auto canonical = [](std::span<const i32> values) {
    std::vector<i32> result(values.begin(), values.end());
    std::sort(result.begin(), result.end());
    return result;
  };
  need(!a.empty() && a.size() == b.size(), "T2.census.exhaustive_ball_inventory");
  for (size_t j = 0; j < a.size(); ++j) {
    need(a[j].key == b[j].key && rational(a[j].level) == rational(b[j].level) &&
         a[j].arity == b[j].arity, "T2.census.exact_ball_and_qmin");
    need(canonical(a[j].interior()) == canonical(b[j].interior()) &&
         canonical(a[j].shell()) == canonical(b[j].shell()),
         "T2.census.exact_geometry_population");
    shell12 += a[j].n_shell == 12;
    q3_rows += a[j].arity == 3; q4_rows += a[j].arity == 4;
    ++catalogue_rows;
  }
}

void check_large_cuts(const FullBallTowerResult& tower, const std::vector<InputPoint>& in,
    const oracle::Model& model) {
  // Once per order, before any geometric cuts. Physical paired towers must
  // then retain these exact metadata, not merely agree on a wrong reference.
  for (unsigned k = 1; k <= 10; ++k) {
    const auto& order = tower.orders[k - 1];
    need(order.forest.order() == k, "T2.metadata.order_identity");
    need(order.lower_nodes.size() == order.forest.nodes().size(), "T2.metadata.vertical_node_indexed");
  }
  const u32 domain = (u32{1} << in.size()) - 1;
  std::set<Rat> levels{Rat(0)};
  for (u32 mask = 1; mask <= domain; ++mask) levels.insert(model.meb(mask).radius2);
  levels.insert(*levels.rbegin() + Rat(1));
  std::vector<Snapshot> previous(10), current(10);
  for (const auto& cut : levels) for (bool closed : {false, true}) {
    for (unsigned k = 1; k <= 10; ++k) {
      const auto expected = model.components(k, cut, closed, domain);
      current[k-1] = check_cut(tower.orders[k-1].forest, expected, previous[k-1], in, cut, closed);
      if (closed) previous[k-1] = current[k-1];
      for (const auto& [facet, root] : current[k-1]) {
        if (k >= 9) ++high_order_facets;
        const auto actual = full_ball_vertical_root_at(tower, k, root, level(cut), closed);
        if (k == 1) need(actual == kFullCoverageAbsent, "T2.vertical.K1_absent");
        else for (u32 bits = facet; bits; bits &= bits-1) {
          const auto found = current[k-2].find(facet & ~(u32{1} << std::countr_zero(bits)));
          need(found != current[k-2].end() && actual == found->second,
               "T2.vertical.all_subfacets_same_cut");
          ++vertical; if (k >= 9) ++high_order_verticals;
        }
      }
    }
  }
}

void large_case(const Fixture& fixture, unsigned variant) {
  context = std::string(fixture.name) + "/variant" + std::to_string(variant);
  std::fprintf(stderr, "start=%s\n", context.c_str()); std::fflush(stderr);
  const auto in = large_input(fixture, variant);
  const auto ix = build_cloud_index(in);
  std::vector<P3> points; for (const auto& p : in) points.push_back(p.position);
  const oracle::Model model(points);
  const auto expected = independent_catalogue(points, ix, 10);
  FullBallTowerResult reference;
  for (int s : {8, 10, 12}) {
    chain_workers = s == 10 ? 4 : 1;
    const auto balls = actual_census(ix, 10, s);
    compare_catalogue(balls, expected);
    for (int threads : {0, 1, 4}) {
      auto tower = build_full_ball_tower(ix, balls, 10, threads);
      if (tower.status != FullBallStatus::kCompleteRelative)
        std::fprintf(stderr, "refusal=%s\n", tower.reason);
      need(tower.status == FullBallStatus::kCompleteRelative && tower.orders.size() == 10,
           "T2.FULL.ten_orders");
      if (s == 8 && threads == 0) {
        check_large_cuts(tower, in, model);
        reference = std::move(tower);
      } else { same_payload(reference, tower); ++tower_pairs; }
      orders += 10;
    }
  }
  // Tour PUBLIEE par la chaine (run_tower=true), jugee elle-meme contre le
  // modele Gamma : points passes dans l'ordre de `in`, donc PointId = rang.
  {
    std::vector<mhgp9::gen::Point3> chain_points;
    std::vector<InputPoint> in_rank;
    for (size_t j = 0; j < in.size(); ++j) {
      const auto& p = in[j].position;
      chain_points.push_back({static_cast<mhgp9::gen::Coordinate>(p.x), static_cast<mhgp9::gen::Coordinate>(p.y),
                              static_cast<mhgp9::gen::Coordinate>(p.z)});
      in_rank.push_back({static_cast<PointId>(j), p});
    }
    FullBallTowerResult published_reference;
    for (std::size_t workers : {1u, 4u}) {
      mhgp9::ChainOptions options;
      options.kmax = 10; options.separation_s = 8; options.workers = workers; options.run_tower = true;
      auto chain = mhgp9::run_tower_chain(chain_points, options);
      if (chain.status != mhgp9::ChainStatus::kComplete)
        std::fprintf(stderr, "chain refusal=%s\n", chain.reason.c_str());
      need(chain.status == mhgp9::ChainStatus::kComplete && chain.tower.orders.size() == 10 &&
           chain.tower_digest == mhgp9::tower_digest(chain.tower), "T2.chain.published_tower");
      if (workers == 1) {
        check_large_cuts(chain.tower, in_rank, model);
        published_reference = std::move(chain.tower);
      } else { same_payload(published_reference, chain.tower); ++tower_pairs; }
      orders += 10;
    }
  }
  ++clouds;
  std::fprintf(stderr, "complete=%s gram=%llu assignments=%llu\n", context.c_str(),
      static_cast<unsigned long long>(model.gram_calls), static_cast<unsigned long long>(model.assignments));
  std::fflush(stderr);
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 2) return 2;
    std::string_view mode(argv[1]);
    if (mode == "--rejects") {
      context = "rejects";
      const auto reject = [](auto&& operation) {
        bool rejected = false;
        try { operation(); } catch (const std::invalid_argument&) { rejected = true; }
        need(rejected, "T2.invalid_request_rejected"); ++rejections;
      };
      reject([] { const oracle::Model m({}); });
      reject([] { const oracle::Model m({{1,1,1},{1,1,1}}); });
      reject([] { const oracle::Model m({{-1,0,0}}); });
      reject([] { const oracle::Model m({{0,262144,0}}); });
      reject([] { std::vector<P3> p; for (i64 i=0; i<15; ++i) p.push_back({i,0,0}); const oracle::Model m(p); });
      const oracle::Model m({{0,0,0},{2,0,0}});
      reject([&] { (void)m.meb(0); });
      reject([&] { (void)m.meb(4); });
      reject([&] { (void)m.components(0, Rat(0), true, 3); });
      reject([&] { (void)m.components(1, Rat(0), true, 4); });
      need(rejections == 9, "T2.reject_floor");
      std::puts("{\"status\":\"passed\",\"rejections\":9}");
      return 0;
    }
    if (mode == "--mutant-assignment") { oracle_mutation = oracle::Mutation::skip_full_assignment; mode = "--historical"; }
    if (mode == "--mutant-open") { oracle_mutation = oracle::Mutation::close_open_cut; mode = "--historical"; }
    if (mode == "--mutant-adjacency") { oracle_mutation = oracle::Mutation::omit_adjacency; mode = "--historical"; }
    if (mode == "--mutant-census") { omit_census_ball = true; mode = "--line12"; }
    if (mode == "--historical") {
      compare_historical_oracle();
      std::printf("{\"status\":\"passed\",\"oracle_mebs\":%llu,\"oracle_components\":%llu}\n",
          static_cast<unsigned long long>(oracle_mebs), static_cast<unsigned long long>(oracle_components));
      return 0;
    }
    Fixture fixture{"line12", {}, 10};
    if (mode == "--line12") {
      for (i64 j = 0; j < 12; ++j) fixture.points.push_back({2*j+(j%2), 7, 9});
    } else if (mode == "--shell14") {
      fixture = {"shell12_center_external14", {{15,10,10},{5,10,10},{10,15,10},{10,5,10},
        {10,10,15},{10,10,5},{13,14,10},{13,6,10},{7,14,10},{7,6,10},{10,13,14},{10,7,6},
        {10,10,10},{20,25,30}}, 10};
    } else if (mode == "--spatial12") {
      fixture = {"spatial12", {{7,42,83},{91,12,64},{33,88,9},{54,20,71},{18,61,39},{76,53,95},
        {42,7,24},{62,94,47},{3,29,58},{85,73,15},{29,36,97},{58,65,3}}, 10};
    } else return 2;
    for (unsigned variant = 0; variant < 2; ++variant) large_case(fixture, variant);
    need(clouds == 2 && orders == 220 && census_runs == 6 && euler_runs == 6 && tower_pairs == 18 &&
         high_order_facets > 0 && high_order_verticals > 0, "T2.nonvacuity");
    if (mode == "--shell14") need(shell12 == 6, "T2.nonvacuity.shell12");
    if (mode == "--spatial12") need(q3_rows > 0 && q4_rows > 0, "T2.nonvacuity.three_dimensional_supports");
    std::printf("{\"status\":\"passed\",\"scope\":\"bounded_real_census_FULL_K1_K10\","
        "\"checks\":%llu,\"clouds\":%llu,\"orders\":%llu,\"census_runs\":%llu,"
        "\"catalogue_rows\":%llu,\"physical_tower_pairs\":%llu,\"cuts\":%llu,\"vertical_checks\":%llu,"
        "\"high_order_facets\":%llu,\"high_order_verticals\":%llu,\"shell12_rows\":%llu,"
        "\"q3_rows\":%llu,\"q4_rows\":%llu}\n",
        static_cast<unsigned long long>(checks), static_cast<unsigned long long>(clouds),
        static_cast<unsigned long long>(orders), static_cast<unsigned long long>(census_runs),
        static_cast<unsigned long long>(catalogue_rows), static_cast<unsigned long long>(tower_pairs),
        static_cast<unsigned long long>(cuts), static_cast<unsigned long long>(vertical),
        static_cast<unsigned long long>(high_order_facets), static_cast<unsigned long long>(high_order_verticals),
        static_cast<unsigned long long>(shell12), static_cast<unsigned long long>(q3_rows),
        static_cast<unsigned long long>(q4_rows));
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

