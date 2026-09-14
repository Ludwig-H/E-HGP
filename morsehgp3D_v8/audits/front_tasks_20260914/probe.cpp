// Audit front-only probe. Loader explicitly reused from Pool probe SHA256
// 027e90cace942f31195056d0e8694e6acb6b092ffa64e38111581fab3b2cf021.
#include "front_tasks.hpp"
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

void print_front(const WspdFrontWork& f) {
  std::cout << '{';
  {
    Fields fields;
#define EMIT(name) fields.add(#name, f.name)
    EMIT(product_visits); EMIT(diagonal_splits); EMIT(diagonal_leaves); EMIT(disjoint_splits);
    EMIT(separation_tests); EMIT(witness_searches); EMIT(witness_descent_steps); EMIT(witness_box_distance_tests);
    EMIT(proposed_sites); EMIT(proposals_in_factors); EMIT(h_bound_tests); EMIT(xi_bound_tests);
    EMIT(witness_lane_credits); EMIT(fully_rejected_products); EMIT(emitted_rectangles); EMIT(emitted_factor_sites);
    EMIT(max_factor_size); EMIT(leaf_pair_rectangles); EMIT(max_stack_size); EMIT(max_product_depth);
#undef EMIT
  }
  array("size_class_rectangles", f.size_class_rectangles); array("size_class_pair_mass", f.size_class_pair_mass);
  array("rejected_pair_mass", f.rejected_pair_mass); array("residual_pair_mass", f.residual_pair_mass);
  array("lane_rectangles", f.lane_rectangles);
  std::cout << '}';
}

int probe(int argc, char** argv) {
  require(argc == 6, "usage: probe input.u16le n Kmax s width (0=public front)");
  const auto n = number(argv[2]), k = number(argv[3]), s = number(argv[4]), width = number(argv[5]);
  require(n >= 2 && k >= 1 && k <= 10 && s > 0 && width <= 4096, "invalid probe parameters");
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
  u64 rectangle_count = 0, mass = 0, digest_sum = 0, digest_xor = 0;
  const WspdRectangleConsumer consumer = [&](const WspdRectangle& r) {
    require(r.lane_mask == 1 && r.a_node < index->spatial_nodes().size() &&
            r.b_node < index->spatial_nodes().size(), "wrong emitted rectangle");
    const auto a = index->spatial_nodes()[r.a_node].range;
    const auto b = index->spatial_nodes()[r.b_node].range;
    require(a.last <= b.first || b.last <= a.first, "overlapping factors");
    u64 h = 14695981039346656037ULL;
    // Node IDs are stable in this exact immutable index. Orientation retained.
    hash_word(h, 1); hash_word(h, r.a_node); hash_word(h, r.b_node); hash_word(h, r.lane_mask);
    digest_sum += h; digest_xor ^= h;
    counter_add(rectangle_count); counter_add(mass, product(a.size(), b.size()));
  };
  audit_tasks::PartitionResult out;
  if (width == 0) {
    out.task_bytes = 0;
    out.front = run_wspd_front(*index, k, s, WspdFrontMode::MidpointSamples, consumer, 1);
  }
  else out = audit_tasks::run_partitioned(*index, k, s, WspdFrontMode::MidpointSamples, consumer, 1, width);
  const auto traversed = Clock::now();
  require(out.front.active_lane_mask == 1 && out.front.total_unordered_pairs == product(n, n-1)/2 &&
          out.front.work.emitted_rectangles == rectangle_count && out.front.work.residual_pair_mass[0] == mass &&
          mass + out.front.work.rejected_pair_mass[0] == out.front.total_unordered_pairs,
          "front/digest coverage mismatch");
  const auto cloud_work = cloud->work();
  const auto cloud_bytes = cloud->retained_bytes(), index_bytes = index->retained_bytes();
  index.reset(); cloud.reset(); std::vector<Point3>().swap(points);
  const auto finished = Clock::now();
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17) << "{\"status\":\"completed\",\"schema\":\"mhgp8_front_tasks_probe_v1\","
            << "\"scope\":\"serial_front_only_no_census_no_parallel_speedup\",\"public_status\":\"not_claimed\","
            << "\"gcp_used\":false,\"threads\":1,\"n\":" << n << ",\"kmax\":" << k << ",\"s\":" << s
            << ",\"width\":" << width << ",\"input_fnv64\":\"" << std::hex << input_hash << std::dec
            << "\",\"rectangles\":" << rectangle_count << ",\"residual_mass\":" << mass
            << ",\"digest\":{\"encoding\":\"oriented_index_rectangles_v1\",\"sum\":\""
            << std::hex << digest_sum << "\",\"xor\":\"" << digest_xor << std::dec << "\"},\"front_work\":";
  print_front(out.front.work);
  std::cout << ",\"prefix_work\":"; print_front(out.prefix);
  std::cout << ",\"maximum_ready\":" << out.maximum_ready << ",\"task_bytes\":" << out.task_bytes
            << ",\"prefix_ms\":" << out.prefix_ms << ",\"adapter_ms\":" << out.total_ms
            << ",\"jobs\":[";
  bool first = true;
  for (const auto& job : out.jobs) {
    if (!first) std::cout << ',';
    first = false;
    std::cout << "{\"a\":" << job.seed.a << ",\"b\":" << job.seed.b
              << ",\"mask\":" << static_cast<unsigned>(job.seed.mask) << ",\"depth\":" << job.seed.depth
              << ",\"seed_mass\":" << job.seed_mass << ",\"product_visits\":" << job.product_visits
              << ",\"witness_descent_steps\":" << job.witness_descent_steps
              << ",\"rectangles\":" << job.rectangles << ",\"pair_mass\":" << job.pair_mass
              << ",\"elapsed_ms\":" << job.elapsed_ms << '}';
  }
  std::cout << "],\"cloud_coordinate_copies\":" << cloud_work.coordinate_copies
            << ",\"cloud_validation_points\":" << cloud_work.validation_points
            << ",\"cloud_retained_bytes\":" << cloud_bytes << ",\"index_retained_bytes\":" << index_bytes
            << ",\"load_ms\":" << ms(started, loaded) << ",\"cloud_ms\":" << ms(loaded, prepared)
            << ",\"index_ms\":" << ms(prepared, indexed) << ",\"front_ms\":" << ms(indexed, traversed)
            << ",\"validation_destruction_ms\":" << ms(traversed, finished)
            << ",\"total_ms\":" << ms(started, finished) << "}\n";
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  try { return probe(argc, argv); }
  catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
