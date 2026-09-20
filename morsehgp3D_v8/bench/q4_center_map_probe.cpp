// Explicit port of the tranche26 paired benchmark layout and reuse of the tranche24
// fixture/candidate/JSON helpers. No inherited qualification:
// this binary and its complete source inventory receive their own capture.
#define main mhgp8_frozen_cover_probe_main
#include "q34_cover_probe.cpp"
#undef main
#include "lanes/q4_center_map.hpp"

namespace {
#define F(type, member) Field<type>{#member, &type::member}
constexpr std::array pool_fields{F(Q34WitnessPoolWork, range_visits), F(Q34WitnessPoolWork, selected_sites)};
#undef F
static_assert(sizeof(Q34WitnessPoolWork) == pool_fields.size() * sizeof(u64));
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

Q4CenterDomainMode parse_domain(std::string_view domain) {
  if (domain == "disk") return Q4CenterDomainMode::Disk;
  if (domain == "positive") return Q4CenterDomainMode::Positive;
  throw std::invalid_argument("unknown center map domain");
}
constexpr std::array filter_fields{
  Field<Q34PoolWork>{"seed_owner_tests", &Q34PoolWork::seed_owner_tests},
  Field<Q34PoolWork>{"seed_owner_rejections", &Q34PoolWork::seed_owner_rejections},
  Field<Q34PoolWork>{"seed_queries", &Q34PoolWork::seed_queries},
  Field<Q34PoolWork>{"certificate_builds", &Q34PoolWork::certificate_builds},
  Field<Q34PoolWork>{"sqrt_iterations", &Q34PoolWork::sqrt_iterations},
  Field<Q34PoolWork>{"variance_bounds", &Q34PoolWork::variance_bounds},
  Field<Q34PoolWork>{"variance_sqrt_iterations", &Q34PoolWork::variance_sqrt_iterations},
  Field<Q34PoolWork>{"proposed_sites", &Q34PoolWork::proposed_sites},
  Field<Q34PoolWork>{"paired_predicate_tests", &Q34PoolWork::paired_predicate_tests},
  Field<Q34PoolWork>{"q3_credits", &Q34PoolWork::q3_credits},
  Field<Q34PoolWork>{"q4_universal_credits", &Q34PoolWork::q4_universal_credits},
  Field<Q34PoolWork>{"q3_rejected", &Q34PoolWork::q3_rejected},
  Field<Q34PoolWork>{"q4_universal_rejected", &Q34PoolWork::q4_universal_rejected},
  Field<Q34PoolWork>{"collective_queries", &Q34PoolWork::collective_queries},
  Field<Q34PoolWork>{"endpoint_tests", &Q34PoolWork::endpoint_tests},
  Field<Q34PoolWork>{"constant_tests", &Q34PoolWork::constant_tests},
  Field<Q34PoolWork>{"event_count", &Q34PoolWork::event_count},
  Field<Q34PoolWork>{"sort_comparisons", &Q34PoolWork::sort_comparisons},
  Field<Q34PoolWork>{"group_comparisons", &Q34PoolWork::group_comparisons},
  Field<Q34PoolWork>{"event_side_tests", &Q34PoolWork::event_side_tests},
  Field<Q34PoolWork>{"groups", &Q34PoolWork::groups},
  Field<Q34PoolWork>{"max_group", &Q34PoolWork::max_group},
  Field<Q34PoolWork>{"collective_minimum_sum", &Q34PoolWork::collective_minimum_sum},
  Field<Q34PoolWork>{"collective_q4_rejected", &Q34PoolWork::collective_q4_rejected},
  Field<Q34PoolWork>{"q4_rejected", &Q34PoolWork::q4_rejected},
  Field<Q34PoolWork>{"both_rejected", &Q34PoolWork::both_rejected},
  Field<Q34PoolWork>{"q3_only_survivors", &Q34PoolWork::q3_only_survivors},
  Field<Q34PoolWork>{"q4_only_survivors", &Q34PoolWork::q4_only_survivors},
  Field<Q34PoolWork>{"both_survivors", &Q34PoolWork::both_survivors},
  Field<Q34PoolWork>{"peak_event_bytes", &Q34PoolWork::peak_event_bytes}};
static_assert(sizeof(Q34PoolWork) == filter_fields.size() * sizeof(u64));

#define F(type, member) Field<type>{#member, &type::member}
constexpr std::array map_fields{
  F(Q4CenterMapWork, preparations),
  F(Q4CenterMapWork, prepared_forms),
  F(Q4CenterMapWork, projection_points),
  F(Q4CenterMapWork, hull_sort_comparisons),
  F(Q4CenterMapWork, hull_orientation_tests),
  F(Q4CenterMapWork, hull_vertices),
  F(Q4CenterMapWork, facets),
  F(Q4CenterMapWork, queries),
  F(Q4CenterMapWork, seed_owner_tests),
  F(Q4CenterMapWork, seed_owner_rejections),
  F(Q4CenterMapWork, rejected_queries),
  F(Q4CenterMapWork, unknown_queries),
  F(Q4CenterMapWork, query_visits),
  F(Q4CenterMapWork, line_tests),
  F(Q4CenterMapWork, line_skips),
  F(Q4CenterMapWork, cells_created),
  F(Q4CenterMapWork, pending_ids_copied),
  F(Q4CenterMapWork, pending_lists_created),
  F(Q4CenterMapWork, node_storage_growths),
  F(Q4CenterMapWork, cells_evaluated),
  F(Q4CenterMapWork, disk_tests),
  F(Q4CenterMapWork, facet_tests),
  F(Q4CenterMapWork, outside_cells),
  F(Q4CenterMapWork, deep_cells),
  F(Q4CenterMapWork, witness_tests),
  F(Q4CenterMapWork, inside_credits),
  F(Q4CenterMapWork, outside_witnesses),
  F(Q4CenterMapWork, splits),
  F(Q4CenterMapWork, compressed_deep),
  F(Q4CenterMapWork, compressed_outside),
  F(Q4CenterMapWork, depth_stops),
  F(Q4CenterMapWork, budget_stops),
  F(Q4CenterMapWork, empty_stops),
  F(Q4CenterMapWork, max_depth),
  F(Q4CenterMapWork, peak_pending_bytes),
  F(Q4CenterMapWork, peak_retained_bytes)};
constexpr std::array domain_fields{
  F(Q4PositiveDomainWork, node_visits),
  F(Q4PositiveDomainWork, bound_tests),
  F(Q4PositiveDomainWork, endpoint_box_tests),
  F(Q4PositiveDomainWork, endpoint_leaf_tests),
  F(Q4PositiveDomainWork, point_tests),
  F(Q4PositiveDomainWork, admitted_nodes),
  F(Q4PositiveDomainWork, rejected_nodes),
  F(Q4PositiveDomainWork, split_nodes),
  F(Q4PositiveDomainWork, excluded_endpoints),
  F(Q4PositiveDomainWork, admitted_sites),
  F(Q4PositiveDomainWork, rejected_sites),
  F(Q4PositiveDomainWork, box_merges)};
#undef F
static_assert(sizeof(Q4CenterMapWork) == sizeof(Q4PositiveDomainWork) + map_fields.size() * sizeof(u64));
static_assert(sizeof(Q4PositiveDomainWork) == domain_fields.size() * sizeof(u64));

int mapped_run(std::size_t n, std::string_view regime, std::size_t kmax,
                   std::size_t budget, std::string_view domain, unsigned depth, std::size_t nodes) {
  const Q34PoolOptions filter{Q34ChordBound::Variance, Q34PoolReduction::Collective};
  const Q4CenterMapOptions options{parse_domain(domain), depth, nodes};
  validate_q4_center_map_options(options);
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
  auto pool = Q34WitnessPool::make(cover, budget);
  const auto pooled = Clock::now();
  Output baseline_output;
  const auto baseline = run_q34_collective_edge_candidates(pool, kmax, filter,
      [&](const auto& candidate) { baseline_output.collect(candidate, n, kmax); });
  const auto baseline_ran = Clock::now();
  baseline_output.finish();
  if (regime != "adversarial") require(baseline_output.records == expected(), "baseline differs from closed-form fixture");
  const auto baseline_checked = Clock::now();
  Output output;
  const auto mapped = run_q34_mapped_edge_candidates(pool, kmax, filter, options,
      [&](const auto& candidate) { output.collect(candidate, n, kmax); });
  const auto ran = Clock::now();
  output.finish();
  require(output.records == baseline_output.records && output.hash == baseline_output.hash, "pruned candidate mismatch");
  require(equal_fields(baseline.edge, mapped.collective.edge, edge_fields), "pruning changed edge seed enumeration");
  require(baseline.edge.seeds == seeds && cover->site_count() == wanted_cover, "baseline seed/cover work mismatch");
  require(equal_fields(baseline.filter, mapped.collective.filter, filter_fields), "map changed upstream filter work");
  if (nodes == 0) require(equal_edge(baseline.edge, mapped.collective.edge) &&
      baseline.peak_live_buffer_bytes == mapped.peak_live_buffer_bytes, "disabled map changed reference work");
  require(output.q3 == mapped.collective.edge.covered.seed.q3_emitted && output.q4 == mapped.collective.edge.covered.seed.q4_emitted &&
          baseline_output.q3 == baseline.edge.covered.seed.q3_emitted && baseline_output.q4 == baseline.edge.covered.seed.q4_emitted,
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
  const double shared_prepare = ms(start, generated) + ms(fixture_checked, pooled);
  std::cout << std::setprecision(17)
      << "{\"schema\":\"mhgp8_q4_center_map_probe_v1\",\"status\":\"completed\","
      << "\"scope\":\"one_edge_lazy_center_map_not_global_q34_producer\",\"public_status\":\"not_claimed\","
      << "\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\",\"threads\":1,\"seed\":3,"
      << "\"recipe\":\"owned_edge_far_cap_dense_v1\",\"edge_ids\":[0,1],"
      << "\"timing_scope\":\"paired_wall_including_validation_release_pipeline_sums_share_measured_preparation\","
      << "\"n\":" << n << ",\"regime\":\"" << regime << "\",\"kmax\":" << kmax << ",\"budget\":" << budget
      << ",\"domain\":\"" << domain << "\",\"map_depth\":" << depth << ",\"map_node_budget\":" << nodes
      << ",\"filter_mode\":\"variance_collective\",\"input_hash\":" << input.hash << ",\"expected_seeds\":" << seeds << ",\"cover_sites\":" << wanted_cover
      << ",\"generation\":{\"proposals\":" << input.proposals << ",\"duplicates\":" << input.duplicates << ",\"random_calls\":" << input.random_calls
      << "},\"validation\":{\"fixture_point_tests\":" << fixture_tests << ",\"independent_ball_point_tests\":" << ball_tests
      << ",\"pool_ids_checked\":" << selected << ",\"method\":\""
      << (regime == "adversarial" ? "qualified_collective_path_differential" : "three_closed_form_balls_plus_differential") << "\"}"
      << ",\"baseline_work\":"; dump_edge(baseline.edge);
  std::cout << ",\"baseline_filter_work\":{"; dump_fields(baseline.filter, filter_fields); std::cout << '}';
  std::cout << ",\"baseline_peak_live_buffer_bytes\":" << baseline.peak_live_buffer_bytes;
  std::cout << ",\"edge_work\":"; dump_edge(mapped.collective.edge);
  std::cout << ",\"filter_work\":{"; dump_fields(mapped.collective.filter, filter_fields); std::cout << '}';
  std::cout << ",\"collective_peak_live_buffer_bytes\":" << mapped.collective.peak_live_buffer_bytes;
  std::cout << ",\"map_work\":{"; dump_fields(mapped.map, map_fields);
  std::cout << ",\"domain\":{"; dump_fields(mapped.map.domain, domain_fields); std::cout << "}}";
  std::cout << ",\"q3_only_after_map\":" << mapped.q3_only_after_map << ",\"both_rejected_by_map\":" << mapped.both_rejected_by_map;
  std::cout << ",\"peak_live_buffer_bytes\":" << mapped.peak_live_buffer_bytes;
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
      << ",\"cover_ms\":" << ms(indexed, covered) << ",\"baseline_run_callback_ms\":" << ms(pooled, baseline_ran)
      << ",\"baseline_validation_ms\":" << ms(baseline_ran, baseline_checked) << ",\"pool_prepare_ms\":" << ms(covered, pooled)
      << ",\"run_callback_ms\":" << ms(baseline_checked, ran) << ",\"validation_ms\":" << ms(ran, checked)
      << ",\"release_ms\":" << ms(checked, done)
      << ",\"baseline_prepare_run_sum_ms\":" << shared_prepare + ms(pooled, baseline_ran)
      << ",\"mapped_prepare_run_sum_ms\":" << shared_prepare + ms(baseline_checked, ran)
      << ",\"paired_total_ms\":" << ms(start, done) << "}}\n";
  return 0;
}
} // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 8) throw std::invalid_argument("usage: mhgp8_q4_center_map_probe n far|cap|adversarial K5_or_10 pool0_32_64 disk|positive depth0_to44 node_budget");
    const auto n = number(argv[1]), k = number(argv[3]), budget = number(argv[4]);
    const std::string_view regime(argv[2]);
    if ((regime != "far" && regime != "cap" && regime != "adversarial") || n < 8 || (k != 5 && k != 10) ||
        (budget != 0 && budget != 32 && budget != 64) || (regime == "cap" && n > 35307) ||
        (regime == "adversarial" && n > 514))
      throw std::invalid_argument("outside distinct-coordinate fixture domain; not an algorithm quota");
    const auto depth = number(argv[6]);
    if (depth > 44) throw std::invalid_argument("outside exact center-map arithmetic depth");
    return mapped_run(n, regime, k, budget, argv[5], static_cast<unsigned>(depth), number(argv[7]));
  } catch (const std::exception& error) {
    std::cerr << "q4 center map probe: " << error.what() << '\n';
    return 1;
  }
}
