#include "bridge.hpp"
#include "support_io.hpp"
#include "front_fixtures.hpp"

namespace {
std::vector<mhgp8::Point3> load_points(const char* path, std::size_t n) {
  if (std::string_view(path) == "clusters")
    return mhgp8::bench::make_front_fixture(n, "clusters", 3).points;
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  require(input.is_open(), "cannot open audit input");
  const auto bytes = input.tellg();
  require(bytes >= std::streamoff{12} && bytes % std::streamoff{6} == 0 &&
          static_cast<u64>(bytes / std::streamoff{6}) == n, "wrong declared input length");
  input.seekg(0);
  std::vector<Point3> points;
  points.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    std::array<unsigned char, 6> raw{};
    input.read(reinterpret_cast<char*>(raw.data()), 6);
    require(input.gcount() == 6 && !input.bad(), "truncated input");
    const auto word = [&](std::size_t j) {
      return static_cast<std::uint16_t>(static_cast<unsigned>(raw[j]) |
                                       (static_cast<unsigned>(raw[j+1]) << 8U));
    };
    points.push_back({word(0), word(2), word(4)});
  }
  require(input.peek() == std::char_traits<char>::eof() && !input.bad(), "input changed length");
  return points;
}

int probe(int argc, char** argv) {
  require(argc == 8, "usage: probe input.u16le|clusters n Kmax s cutoff baseline|pool-pair|pool-shared samples");
  const auto n = number(argv[2]), k = number(argv[3]), s = number(argv[4]), cutoff = number(argv[5]);
  const std::string_view mode(argv[6]);
  require(n >= 2 && k >= 1 && k <= 10 && s > 0 && std::string_view(argv[7]) == "samples" &&
          (mode == "baseline" || mode == "pool-pair" || mode == "pool-shared") &&
          ((mode == "baseline") == (cutoff == 0)), "invalid audit mode");
  const auto started = Clock::now();
  auto points = load_points(argv[1], n);
  u64 input_hash = 14695981039346656037ULL;
  for (const auto& p : points) for (std::size_t axis = 0; axis < 3; ++axis) {
    input_hash ^= p[axis] & 255U; input_hash *= 1099511628211ULL;
    input_hash ^= p[axis] >> 8U; input_hash *= 1099511628211ULL;
  }
  const auto loaded = Clock::now();
  auto cloud = prepare_cloud(points);
  const auto prepared = Clock::now();
  auto index = make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  OutputDigest digest;
  const auto output = audit_pool::run_bridge(*index, k, s, cutoff,
      mode == "pool-pair" ? audit_pool::ResidualMode::Pairwise : audit_pool::ResidualMode::Shared,
      [&](const Q2Support& support) { digest.consume(support, cloud->points(), k); });
  const auto processed = Clock::now();
  const auto& result = output.pipeline;
  const auto& p = output.pool;
  const auto& f = result.front.work;
  const auto& c = result.census;
  const auto total = product(n, n-1)/2;
  require(result.front.total_unordered_pairs == total && result.front.active_lane_mask == 1 &&
          f.rejected_pair_mass[0] <= total && f.residual_pair_mass[0] == total-f.rejected_pair_mass[0] &&
          c.candidate_pairs + p.filtered_pairs == f.residual_pair_mass[0] &&
          c.accepted_pairs <= c.candidate_pairs && c.rejected_pairs == c.candidate_pairs-c.accepted_pairs,
          "front/Pool/census mass mismatch");
  require(p.selected_pairs == p.filtered_pairs + p.residual_pairs &&
          result.input_rectangles == f.emitted_rectangles && f.xi_bound_tests == 0 &&
          c.work.frontier_restarts == 0 && c.work.count_root_starts ==
          result.anchor_queries-p.original_selected_anchors+p.local_roots,
          "Pool coverage/root mismatch");
  require(c.work.query_tasks == c.work.count_root_starts + 2*c.work.query_splits &&
          c.work.count_node_visits == c.work.count_point_tests+c.work.count_bound_tests &&
          p.local_b_sites == c.work.query_build_point_visits && p.local_b_nodes == c.work.query_build_nodes,
          "query/census work mismatch");
  require(digest.supports == c.accepted_pairs && digest.supports == c.work.payload_supports &&
          digest.interior_ids == c.work.payload_interior_sites && digest.shell_ids == c.work.payload_shell_sites,
          "physical output mismatch");
  if (mode != "pool-pair")
    require(c.work.cursor_advances == c.work.count_node_visits-c.work.query_splits +
            result.order_work.structural_splits+result.order_work.deferred_skips+
            result.order_work.anchor_skips+result.order_work.phase_switches, "cursor ledger mismatch");
  const auto cloud_bytes = cloud->retained_bytes(), index_bytes = index->retained_bytes();
  const auto cloud_work = cloud->work();
  std::vector<std::size_t>().swap(digest.interior); std::vector<std::size_t>().swap(digest.shell);
  index.reset(); cloud.reset(); std::vector<Point3>().swap(points);
  const auto finished = Clock::now();
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17) << "{\"status\":\"completed\",\"schema\":\"mhgp8_audit_pool_bridge_v1\","
            << "\"scope\":\"audit_adapter_q2_stream_not_full\",\"public_status\":\"not_claimed\","
            << "\"gcp_used\":false,\"threads\":1,\"n\":" << n << ",\"kmax\":" << k << ",\"s\":" << s
            << ",\"mode\":\"" << mode << "\",\"cutoff\":" << cutoff << ",\"input_fnv64\":\""
            << std::hex << input_hash << std::dec << "\",\"input_rectangles\":" << result.input_rectangles
            << ",\"anchor_queries\":" << result.anchor_queries << ",\"candidate_pairs\":" << c.candidate_pairs
            << ",\"accepted_pairs\":" << c.accepted_pairs << ",\"rejected_pairs\":" << c.rejected_pairs;
#define EMIT(object, name) fields.add(#name, object.name)
  std::cout << ",\"front_work\":{";
  {
    Fields fields;
    EMIT(f, product_visits); EMIT(f, diagonal_splits); EMIT(f, diagonal_leaves); EMIT(f, disjoint_splits);
    EMIT(f, separation_tests); EMIT(f, witness_searches); EMIT(f, witness_descent_steps); EMIT(f, witness_box_distance_tests);
    EMIT(f, proposed_sites); EMIT(f, proposals_in_factors); EMIT(f, h_bound_tests); EMIT(f, xi_bound_tests);
    EMIT(f, witness_lane_credits); EMIT(f, fully_rejected_products); EMIT(f, emitted_rectangles); EMIT(f, emitted_factor_sites);
    EMIT(f, max_factor_size); EMIT(f, leaf_pair_rectangles); EMIT(f, max_stack_size); EMIT(f, max_product_depth);
  }
  array("size_class_rectangles", f.size_class_rectangles); array("size_class_pair_mass", f.size_class_pair_mass);
  array("rejected_pair_mass", f.rejected_pair_mass); array("residual_pair_mass", f.residual_pair_mass);
  array("lane_rectangles", f.lane_rectangles);
  std::cout << "},\"census_work\":{";
  {
    Fields fields;
    EMIT(c.work, query_build_point_visits); EMIT(c.work, query_build_nodes); EMIT(c.work, query_build_max_depth);
    EMIT(c.work, input_descriptors); EMIT(c.work, query_cover_visits); EMIT(c.work, query_tasks); EMIT(c.work, query_splits);
    EMIT(c.work, witness_splits); EMIT(c.work, count_root_starts); EMIT(c.work, shared_splits_after_credit);
    EMIT(c.work, cursor_advances); EMIT(c.work, cursor_reuses); EMIT(c.work, count_node_visits);
    EMIT(c.work, count_bound_tests); EMIT(c.work, count_point_tests); EMIT(c.work, uniform_credited_pairs);
    EMIT(c.work, uniform_rejected_pairs); EMIT(c.work, uniform_accepted_pairs); EMIT(c.work, consumed_witness_sites);
    EMIT(c.work, frontier_restarts); EMIT(c.work, payload_node_visits); EMIT(c.work, payload_bound_tests);
    EMIT(c.work, payload_point_tests); EMIT(c.work, payload_interior_sites); EMIT(c.work, payload_shell_sites); EMIT(c.work, payload_supports);
  }
  std::cout << "},\"pool_work\":{";
  {
    Fields fields;
    EMIT(p, selected_rectangles); EMIT(p, selected_pairs); EMIT(p, residual_pairs); EMIT(p, filtered_pairs);
    EMIT(p, factor_sites); EMIT(p, selection_tests); EMIT(p, witness_attempts); EMIT(p, universal_queries);
    EMIT(p, pool_selected); EMIT(p, pool_insertions); EMIT(p, pool_shifted_entries);
    EMIT(p, prefix_class_visits); EMIT(p, q2_axis_terms);
    EMIT(p, factor_read_visits); EMIT(p, grouping_visits); EMIT(p, local_b_sites); EMIT(p, local_b_nodes);
    EMIT(p, cover_visits); EMIT(p, cover_nodes); EMIT(p, local_roots); EMIT(p, selected_anchors);
    EMIT(p, original_selected_anchors); EMIT(p, plan_peak_bytes); EMIT(p, query_peak_bytes);
  }
  std::cout << "},\"order_work\":{";
  { Fields fields; EMIT(result.order_work, structural_splits); EMIT(result.order_work, deferred_skips);
    EMIT(result.order_work, anchor_skips); EMIT(result.order_work, phase_switches); }
  std::cout << "},\"sibling_work\":{";
  { Fields fields; EMIT(result.sibling_work, proposals); EMIT(result.sibling_work, cardinality_skips);
    EMIT(result.sibling_work, bound_tests); EMIT(result.sibling_work, rejected_tasks);
    EMIT(result.sibling_work, rejected_pairs); EMIT(result.sibling_work, rejected_after_credit); }
#undef EMIT
  std::cout << "},\"digest\":{\"encoding\":\"canonical_q2_support_v2\",\"supports\":" << digest.supports
            << ",\"interior_ids\":" << digest.interior_ids << ",\"shell_ids\":" << digest.shell_ids
            << ",\"sum\":\"" << std::hex << digest.sum << "\",\"xor\":\"" << digest.xor_value << std::dec << "\"}"
            << ",\"cloud_coordinate_copies\":" << cloud_work.coordinate_copies
            << ",\"cloud_validation_points\":" << cloud_work.validation_points
            << ",\"cloud_retained_bytes\":" << cloud_bytes << ",\"index_retained_bytes\":" << index_bytes
            << ",\"load_ms\":" << ms(started, loaded) << ",\"cloud_ms\":" << ms(loaded, prepared)
            << ",\"index_ms\":" << ms(prepared, indexed) << ",\"pipeline_ms\":" << result.total_ms
            << ",\"payload_ms\":" << c.payload_ms << ",\"pool_preparation_ms\":" << p.preparation_ms
            << ",\"query_build_ms\":" << p.query_build_ms << ",\"selected_total_ms\":" << p.selected_total_ms
            << ",\"validation_destruction_ms\":" << ms(processed, finished)
            << ",\"total_ms\":" << ms(started, finished) << "}\n";
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  try { return probe(argc, argv); }
  catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
