// Explicit reuse of the tranche24 fixture, exact candidate record/digest and
// JSON field tables. No edit or hidden inheritance of its qualification:
// this binary and its complete source inventory receive their own capture.
#define main mhgp8_frozen_cover_probe_main
#include "q34_cover_probe.cpp"
#undef main
#include "lanes/q34_pruning.hpp"

namespace {
#define F(type, member) Field<type>{#member, &type::member}
constexpr std::array pool_fields{F(Q34WitnessPoolWork, range_visits), F(Q34WitnessPoolWork, selected_sites)};
constexpr std::array pruning_fields{
  F(Q34FamilyPruningWork, seed_queries), F(Q34FamilyPruningWork, certificate_builds),
  F(Q34FamilyPruningWork, sqrt_iterations), F(Q34FamilyPruningWork, proposed_sites),
  F(Q34FamilyPruningWork, paired_predicate_tests), F(Q34FamilyPruningWork, q3_credits),
  F(Q34FamilyPruningWork, q4_credits), F(Q34FamilyPruningWork, q3_rejected),
  F(Q34FamilyPruningWork, q4_rejected), F(Q34FamilyPruningWork, both_rejected),
  F(Q34FamilyPruningWork, q3_only_survivors), F(Q34FamilyPruningWork, q4_only_survivors),
  F(Q34FamilyPruningWork, both_survivors)};
#undef F
static_assert(sizeof(Q34WitnessPoolWork) == pool_fields.size() * sizeof(u64));
static_assert(sizeof(Q34FamilyPruningWork) == pruning_fields.size() * sizeof(u64));
template<class T, std::size_t N>
bool equal_fields(const T& left, const T& right, const std::array<Field<T>, N>& fields) {
  return std::all_of(fields.begin(), fields.end(), [&](const auto& field) {
    return left.*(field.member) == right.*(field.member);
  });
}
bool equal_edge(const Q34EdgeWork& a, const Q34EdgeWork& b) {
  return equal_fields(a, b, edge_fields) && equal_fields(a.covered, b.covered, covered_fields) &&
         equal_fields(a.covered.seed, b.covered.seed, seed_fields) &&
         equal_fields(a.covered.seed.family, b.covered.seed.family, family_fields);
}
void dump_edge(const Q34EdgeWork& value) {
  std::cout << '{'; dump_fields(value, edge_fields);
  std::cout << ",\"covered\":{"; dump_fields(value.covered, covered_fields);
  std::cout << ",\"seed\":"; dump_seed(value.covered.seed); std::cout << "}}";
}

int pruning_run(std::size_t n, std::string_view regime, std::size_t kmax, std::size_t budget) {
  const auto start = Clock::now();
  auto input = make_input(n, regime);
  const auto generated = Clock::now();
  const auto seeds = regime == "adversarial" ? n - 2 : 2;
  const auto wanted_cover = regime == "far" ? 6 : n;
  u64 fixture_tests = 0, ball_tests = 0;
  for (std::size_t id = 0; id < n; ++id) {
    const auto point = input.points[id];
    const i64 x = static_cast<i64>(point.x) - 1000, y = static_cast<i64>(point.y) - 1000, z = static_cast<i64>(point.z) - 1000;
    const bool inside = x*x + y*y + z*z <= 40000;
    const bool owned = x > -100 && x < 100 && x*x + y*y + z*z > 10000 &&
        (x+100)*(x+100) + y*y + z*z <= 40000 && (x-100)*(x-100) + y*y + z*z <= 40000;
    require(inside == (regime != "far" || id < 6) && owned == (id >= 2 && id < 2 + seeds), "fixture certificate failed");
    ++fixture_tests;
  }
  if (regime != "adversarial") {
    for (const auto& ball : expected())
      for (std::size_t id = 0; id < n; ++id) {
        const auto p = input.points[id];
        const auto& c = ball.coefficients;
        const i128 norm = static_cast<i128>(p.x)*p.x + static_cast<i128>(p.y)*p.y + static_cast<i128>(p.z)*p.z;
        const i128 power = c[0]*norm + c[1]*p.x + c[2]*p.y + c[3]*p.z + c[4];
        require((power < 0) == (id == 4 || id == 5) &&
                (power == 0) == std::binary_search(ball.shell.begin(), ball.shell.end(), id), "closed-form ball census failed");
        ++ball_tests;
      }
  }
  const auto fixture_checked = Clock::now();
  auto cloud = prepare_cloud(input.points);
  const auto prepared = Clock::now();
  auto index = make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  auto cover = Q34EdgeCover::make(index, {0, 1});
  const auto covered = Clock::now();
  Output baseline_output;
  const auto baseline = run_q34_edge_candidates(cover, kmax,
      [&](const auto& candidate) { baseline_output.collect(candidate, n, kmax); });
  const auto baseline_ran = Clock::now();
  baseline_output.finish();
  if (regime != "adversarial") require(baseline_output.records == expected(), "baseline differs from closed-form fixture");
  const auto baseline_checked = Clock::now();
  auto pool = Q34WitnessPool::make(cover, budget);
  const auto pooled = Clock::now();
  Output output;
  const auto pruned = run_q34_pruned_edge_candidates(pool, kmax,
      [&](const auto& candidate) { output.collect(candidate, n, kmax); });
  const auto ran = Clock::now();
  output.finish();
  require(output.records == baseline_output.records && output.hash == baseline_output.hash, "pruned candidate mismatch");
  require(equal_fields(baseline, pruned.edge, edge_fields), "pruning changed edge seed enumeration");
  require(baseline.seeds == seeds && cover->site_count() == wanted_cover &&
          baseline.covered.site_reads == seeds * wanted_cover, "baseline seed/cover work mismatch");
  if (budget == 0) require(equal_edge(baseline, pruned.edge), "disabled pruning changed reference work");
  require(output.q3 == pruned.edge.covered.seed.q3_emitted && output.q4 == pruned.edge.covered.seed.q4_emitted &&
          baseline_output.q3 == baseline.covered.seed.q3_emitted && baseline_output.q4 == baseline.covered.seed.q4_emitted,
          "candidate output ledger differs");
  const auto selected = pool->ids().size();
  require(selected == std::min(budget, wanted_cover), "pool selection count differs");
  u64 pool_hash = 14695981039346656037ULL;
  std::size_t block = 0, offset = 0;
  for (std::size_t i = 0; i < selected; ++i) {
    const auto position = i * wanted_cover / selected; // Bounded benchmark fixture; not production arithmetic.
    while (position >= offset + cover->ranges()[block].size()) offset += cover->ranges()[block++].size();
    const auto expected_id = index->spatial_order()[cover->ranges()[block].first + position - offset];
    require(pool->ids()[i] == expected_id, "pool is not evenly spaced over covered ranks");
    word(pool_hash, expected_id);
  }
  const auto cloud_work = cloud->work();
  const auto index_work = index->work();
  const auto cover_work = cover->work();
  const auto pool_work = pool->work();
  const auto input_bytes = input.points.capacity() * sizeof(Point3), cloud_bytes = cloud->retained_bytes();
  const auto index_bytes = index->retained_bytes(), cover_bytes = cover->retained_bytes(), pool_bytes = pool->retained_bytes();
  const auto baseline_output_bytes = baseline_output.retained_bytes(), output_bytes = output.retained_bytes();
  const auto checked = Clock::now();
  pool.reset(); cover.reset(); index.reset(); cloud.reset();
  output.release(); baseline_output.release(); std::vector<Point3>().swap(input.points);
  const auto done = Clock::now();
  const double shared_prepare = ms(start, generated) + ms(fixture_checked, covered);
  std::cout << std::setprecision(17)
      << "{\"schema\":\"mhgp8_q34_pruning_probe_v1\",\"status\":\"completed\","
      << "\"scope\":\"one_edge_family_pruning_not_global_q34_producer\",\"public_status\":\"not_claimed\","
      << "\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\",\"threads\":1,\"seed\":3,"
      << "\"recipe\":\"owned_edge_far_cap_dense_v1\",\"edge_ids\":[0,1],"
      << "\"timing_scope\":\"paired_wall_including_validation_release_pipeline_sums_share_measured_preparation\","
      << "\"n\":" << n << ",\"regime\":\"" << regime << "\",\"kmax\":" << kmax << ",\"budget\":" << budget
      << ",\"input_hash\":" << input.hash << ",\"expected_seeds\":" << seeds << ",\"cover_sites\":" << wanted_cover
      << ",\"generation\":{\"proposals\":" << input.proposals << ",\"duplicates\":" << input.duplicates << ",\"random_calls\":" << input.random_calls
      << "},\"validation\":{\"fixture_point_tests\":" << fixture_tests << ",\"independent_ball_point_tests\":" << ball_tests
      << ",\"pool_ids_checked\":" << selected << ",\"method\":\""
      << (regime == "adversarial" ? "qualified_covered_path_differential" : "three_closed_form_balls_plus_differential") << "\"}"
      << ",\"baseline_work\":"; dump_edge(baseline);
  std::cout << ",\"edge_work\":"; dump_edge(pruned.edge);
  std::cout << ",\"pruning_work\":{"; dump_fields(pruned.pruning, pruning_fields); std::cout << '}';
  std::cout << ",\"pool_work\":{"; dump_fields(pool_work, pool_fields); std::cout << '}';
  std::cout << ",\"pool_hash\":" << pool_hash;
  std::cout << ",\"cover_work\":{"; dump_fields(cover_work, cover_fields); std::cout << '}';
  std::cout << ",\"index_work\":{"; dump_fields(index_work, index_fields); std::cout << '}';
  std::cout << ",\"cloud_work\":{"; dump_fields(cloud_work, cloud_fields); std::cout << '}';
  std::cout << ",\"baseline_digest\":"; baseline_output.dump(); std::cout << ",\"digest\":"; output.dump();
  std::cout << ",\"memory\":{\"scope\":\"retained_capacities_not_RSS_shared_preparation_and_both_outputs\","
      << "\"id_bytes\":" << sizeof(std::size_t) << ",\"input_capacity_bytes\":" << input_bytes
      << ",\"cloud_retained_bytes\":" << cloud_bytes << ",\"index_retained_bytes\":" << index_bytes
      << ",\"cover_retained_bytes\":" << cover_bytes << ",\"pool_retained_bytes\":" << pool_bytes
      << ",\"baseline_output_retained_bytes\":" << baseline_output_bytes << ",\"output_retained_bytes\":" << output_bytes << '}'
      << ",\"timings\":{\"generation_ms\":" << ms(start, generated) << ",\"fixture_validation_ms\":" << ms(generated, fixture_checked)
      << ",\"cloud_ms\":" << ms(fixture_checked, prepared) << ",\"index_ms\":" << ms(prepared, indexed)
      << ",\"cover_ms\":" << ms(indexed, covered) << ",\"baseline_run_callback_ms\":" << ms(covered, baseline_ran)
      << ",\"baseline_validation_ms\":" << ms(baseline_ran, baseline_checked) << ",\"pool_prepare_ms\":" << ms(baseline_checked, pooled)
      << ",\"run_callback_ms\":" << ms(pooled, ran) << ",\"validation_ms\":" << ms(ran, checked)
      << ",\"release_ms\":" << ms(checked, done)
      << ",\"baseline_prepare_run_sum_ms\":" << shared_prepare + ms(covered, baseline_ran)
      << ",\"pruned_prepare_run_sum_ms\":" << shared_prepare + ms(baseline_checked, pooled) + ms(pooled, ran)
      << ",\"paired_total_ms\":" << ms(start, done) << "}}\n";
  return 0;
}
} // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 5) throw std::invalid_argument("usage: mhgp8_q34_pruning_probe n far|cap|adversarial K5_or_10 budget0_32_64");
    const auto n = number(argv[1]), k = number(argv[3]), budget = number(argv[4]);
    const std::string_view regime(argv[2]);
    if ((regime != "far" && regime != "cap" && regime != "adversarial") || n < 8 || (k != 5 && k != 10) ||
        (budget != 0 && budget != 32 && budget != 64) || (regime == "cap" && n > 35307) ||
        (regime == "adversarial" && n > 514))
      throw std::invalid_argument("outside distinct-coordinate fixture domain; not an algorithm quota");
    return pruning_run(n, regime, k, budget);
  } catch (const std::exception& error) {
    std::cerr << "q34 pruning probe: " << error.what() << '\n';
    return 1;
  }
}
