// Explicit benchmark port from wspd_q2_cooperative_probe.cpp at beee3341.
// The frozen common callback still pays every worker's copies, sorting,
// validation and canonical digest. No old qualification or timing is inherited.
#include "parallel_probe_common.hpp"
#include "pipeline/wspd_q2_ranges.hpp"

namespace {

struct Options {
  std::size_t n;
  std::string_view family;
  unsigned kmax;
  unsigned separation;
  u64 seed;
  std::size_t threads;
  std::size_t jobs_per_worker;
  std::size_t anchor_grain;
  std::size_t queue_capacity;
  std::size_t pool_min_factor;
};

Options options(int argc, char** argv) {
  if (argc != 11)
    throw std::invalid_argument("usage: mhgp8_wspd_q2_ranges_probe n family K s seed "
        "workers jobs_per_worker anchor_grain queue_capacity pool_min_factor");
  const Options o{integer<std::size_t>(argv[1]), argv[2], integer<unsigned>(argv[3]),
                  integer<unsigned>(argv[4]), integer<u64>(argv[5]),
                  integer<std::size_t>(argv[6]), integer<std::size_t>(argv[7]),
                  integer<std::size_t>(argv[8]), integer<std::size_t>(argv[9]),
                  integer<std::size_t>(argv[10])};
  mhgp8::bench::validate_front_fixture_size(o.n, o.family);
  if (o.kmax == 0 || o.kmax > 10 || o.separation == 0 || o.threads == 0 ||
      o.jobs_per_worker == 0 || o.anchor_grain == 0 || o.queue_capacity == 0)
    throw std::invalid_argument("ranges probe requires K1..10 and positive "
        "s, workers, jobs_per_worker, anchor_grain, queue_capacity");
  static_cast<void>(product(size_counter(o.threads), size_counter(o.jobs_per_worker)));
  return o;
}

#define MHGP8_RANGE_FIELD(name) Field<mhgp8::Q2RangeWork>{#name, &mhgp8::Q2RangeWork::name}
constexpr std::array range_fields{
  MHGP8_RANGE_FIELD(initial_ranges),
  MHGP8_RANGE_FIELD(completed_ranges),
  MHGP8_RANGE_FIELD(received_ranges),
  MHGP8_RANGE_FIELD(initial_anchors),
  MHGP8_RANGE_FIELD(completed_anchors),
  MHGP8_RANGE_FIELD(initial_pairs),
  MHGP8_RANGE_FIELD(completed_pairs),
  MHGP8_RANGE_FIELD(initial_shared_ranges),
  MHGP8_RANGE_FIELD(initial_pool_ranges),
  MHGP8_RANGE_FIELD(initial_passthrough_ranges),
  MHGP8_RANGE_FIELD(donations),
  MHGP8_RANGE_FIELD(shared_donations),
  MHGP8_RANGE_FIELD(pool_donations),
  MHGP8_RANGE_FIELD(passthrough_donations),
  MHGP8_RANGE_FIELD(donated_anchors),
  MHGP8_RANGE_FIELD(donated_pairs),
  MHGP8_RANGE_FIELD(donations_after_seeds_exhausted),
  MHGP8_RANGE_FIELD(offer_checks),
  MHGP8_RANGE_FIELD(offer_busy),
  MHGP8_RANGE_FIELD(offer_full),
  MHGP8_RANGE_FIELD(offer_no_waiter),
  MHGP8_RANGE_FIELD(waits),
  MHGP8_RANGE_FIELD(wakes),
  MHGP8_RANGE_FIELD(max_queue_size),
  MHGP8_RANGE_FIELD(max_active_tasks)};
#undef MHGP8_RANGE_FIELD

void print_ranges(const mhgp8::Q2RangeWork& work) {
  std::cout << '{';
  print_fields(work, range_fields);
  std::cout << '}';
}

void validate_ranges(const mhgp8::WspdQ2RangeResult& result, const Options& o) {
  const auto& p = result.pipeline;
  const auto& w = result.work;
  require(result.workers.size() == p.workers.size(), "range worker slots mismatch");
  require(p.requested_workers == size_counter(o.threads) &&
          p.started_workers == (p.jobs == 0 ? 0 : p.requested_workers) &&
          p.target_jobs == product(size_counter(o.threads), size_counter(o.jobs_per_worker)),
          "range seed/worker mismatch");
  require(p.dispatch_work == mhgp8::WspdFrontDispatchWork{}, "ranges reported front donations");
  for (const auto& field : range_fields) {
    u64 combined = 0;
    for (const auto& worker : result.workers) {
      const auto value = worker.work.*(field.member);
      if (field.member == &mhgp8::Q2RangeWork::max_queue_size ||
          field.member == &mhgp8::Q2RangeWork::max_active_tasks)
        combined = std::max(combined, value);
      else mhgp8::counter_add(combined, value);
    }
    require(combined == w.*(field.member), "range worker reduction mismatch");
  }
  require(w.initial_anchors == w.completed_anchors && w.initial_pairs == w.completed_pairs &&
          w.completed_pairs == p.candidate_pairs &&
          w.completed_ranges >= w.initial_ranges &&
          w.completed_ranges - w.initial_ranges == w.donations &&
          w.received_ranges == w.donations && w.waits == w.wakes,
          "range mass or lineage mismatch");
  require(w.max_queue_size <= size_counter(o.queue_capacity) &&
          w.max_active_tasks <= p.started_workers && result.range_task_bytes > 0 &&
          p.queue_storage_bytes >= product(size_counter(o.queue_capacity), result.range_task_bytes),
          "range queue storage mismatch");
  if (o.threads == 1)
    require(w.offer_checks == 0 && w.donations == 0 && w.waits == 0,
            "single worker polled ranges or waited");
}

void merge_digest(OutputDigest& total, const OutputDigest& part) {
  mhgp8::counter_add(total.supports, part.supports);
  mhgp8::counter_add(total.interior_ids, part.interior_ids);
  mhgp8::counter_add(total.shell_ids, part.shell_ids);
  total.sum += part.sum;
  total.xor_value ^= part.xor_value;
  for (const auto& field : callback_fields)
    mhgp8::counter_add(total.work.*(field.member), part.work.*(field.member));
}

void print_digest(const OutputDigest& d) {
  std::cout << "{\"encoding\":\"canonical_q2_support_v2\",\"supports\":" << d.supports
            << ",\"interior_ids\":" << d.interior_ids << ",\"shell_ids\":" << d.shell_ids
            << ",\"sum\":\"" << std::hex << d.sum << "\",\"xor\":\"" << d.xor_value
            << std::dec << "\"}";
}

mhgp8::WspdQ2RangeResult run_pipeline(
    const Options& o, const mhgp8::Q2CensusIndexPtr& index,
    std::span<const mhgp8::Q2CensusConsumer> consumers) {
  const mhgp8::WspdQ2RangeOptions scheduling{
      o.jobs_per_worker, o.queue_capacity, o.anchor_grain};
  return mhgp8::run_wspd_q2_census_ranges(index, o.kmax, o.separation,
      mhgp8::WspdFrontMode::MidpointSamples, consumers, scheduling,
      mhgp8::Q2SiblingMode::Saturating, mhgp8::Q2WitnessOrder::ComplementFirst,
      o.pool_min_factor);
}

void validate(const mhgp8::WspdQ2ParallelResult& r, const OutputDigest& d,
              const Options& o, std::span<const OutputDigest> digests) {
  const auto& c = r.census_work;
  const auto& p = r.pool_work;
  const auto total = product(size_counter(o.n), size_counter(o.n - 1)) / 2;
  require(r.front.total_unordered_pairs == total && r.front.active_lane_mask == 1 &&
          r.front.work.rejected_pair_mass[0] <= total &&
          r.front.work.residual_pair_mass[0] == total - r.front.work.rejected_pair_mass[0],
          "front pair partition mismatch");
  require(p.filtered_pairs <= r.front.work.residual_pair_mass[0] &&
          r.candidate_pairs == r.front.work.residual_pair_mass[0] - p.filtered_pairs &&
          r.accepted_pairs <= r.candidate_pairs && r.rejected_pairs == r.candidate_pairs - r.accepted_pairs &&
          p.passthrough_pairs <= p.residual_pairs && p.pair_roots == p.residual_pairs - p.passthrough_pairs,
          "Pool/census pair partition mismatch");
  require(d.supports == r.accepted_pairs && c.payload_supports == d.supports &&
          c.payload_interior_sites == d.interior_ids && c.payload_shell_sites == d.shell_ids &&
          c.input_descriptors == r.input_rectangles && c.query_cover_visits == 0 &&
          c.query_build_nodes == 0 && c.query_build_point_visits == 0 &&
          c.query_build_max_depth == 0 && c.frontier_restarts == 0, "payload/descriptor mismatch");
  require(r.front.work.xi_bound_tests == 0 &&
          r.front.work.rejected_pair_mass[1] == 0 && r.front.work.rejected_pair_mass[2] == 0 &&
          r.front.work.residual_pair_mass[1] == 0 && r.front.work.residual_pair_mass[2] == 0 &&
          r.front.work.lane_rectangles[1] == 0 && r.front.work.lane_rectangles[2] == 0,
          "q2 ranges front leaked another lane");
  require(r.workers.size() == r.started_workers && r.workers.size() <= digests.size(),
          "worker slots mismatch");
  u64 jobs = 0, products = r.prefix_product_visits, rectangles = 0, visits = 0, supports = 0, pool_bytes = 0;
  double elapsed = 0, payload = 0;
  for (std::size_t i = 0; i < r.workers.size(); ++i) {
    const auto& w = r.workers[i];
    mhgp8::counter_add(jobs, w.jobs);
    mhgp8::counter_add(products, w.front_products);
    mhgp8::counter_add(rectangles, w.input_rectangles);
    mhgp8::counter_add(visits, w.count_node_visits);
    mhgp8::counter_add(supports, w.supports);
    mhgp8::counter_add(pool_bytes, w.pool_peak_bytes);
    require(w.supports == digests[i].supports && std::isfinite(w.elapsed_ms) &&
            std::isfinite(w.payload_ms) && w.payload_ms >= 0 && w.elapsed_ms + 1e-6 >= w.payload_ms,
            "worker payload or clock mismatch");
    elapsed += w.elapsed_ms;
    payload += w.payload_ms;
  }
  require(jobs == r.jobs && jobs == r.completed_jobs && products == r.front.work.product_visits &&
          rectangles == r.input_rectangles && visits == c.count_node_visits && supports == d.supports &&
          pool_bytes == r.pool_peak_bytes_sum, "parallel reduction lost work or jobs");
  require(std::abs(elapsed - r.worker_ms_sum) <= 1e-6 * std::max(1.0, elapsed) &&
          std::abs(payload - r.payload_ms_sum) <= 1e-6 * std::max(1.0, payload) &&
          std::isfinite(r.total_ms) && r.total_ms >= r.partition_ms && r.partition_ms >= 0,
          "parallel sum/wall clocks mismatch");
  for (std::size_t i = r.workers.size(); i < digests.size(); ++i)
    require(digests[i].supports == 0, "inactive worker emitted a support");
}

int run(const Options& o) {
  const auto started = Clock::now();
  std::optional<mhgp8::bench::FrontFixture> input(mhgp8::bench::make_front_fixture(o.n, o.family, o.seed));
  const auto generated = Clock::now();
  auto cloud = mhgp8::prepare_cloud(input->points);
  const auto prepared = Clock::now();
  auto index = mhgp8::make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  std::vector<OutputDigest> digests(o.threads);
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  consumers.reserve(digests.size());
  for (std::size_t i = 0; i < digests.size(); ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& support) {
      digests[i].consume(support, cloud->points(), o.kmax);
    });
  const auto ranges = run_pipeline(o, index, consumers);
  const auto& result = ranges.pipeline;
  OutputDigest digest;
  for (const auto& part : digests) merge_digest(digest, part);
  const auto processed = Clock::now();
  validate(result, digest, o, digests);
  validate_ranges(ranges, o);
  require(&index->cloud() == cloud.get(), "ranges index lost its cloud identity");
  const auto input_hash = input->input_hash;
  const auto generation_work = input->work;
  const auto cloud_work = cloud->work();
  const auto index_work = index->work();
  const auto input_bytes = product(size_counter(input->points.capacity()), sizeof(mhgp8::Point3));
  const auto cloud_bytes = cloud->retained_bytes();
  const auto index_bytes = index->retained_bytes();
  u64 callback_bytes = 0;
  std::vector<u64> worker_callback_bytes;
  for (const auto& part : digests) {
    worker_callback_bytes.push_back(part.buffers_capacity_bytes());
    mhgp8::counter_add(callback_bytes, worker_callback_bytes.back());
  }
  const auto callback_state_bytes = product(size_counter(digests.capacity()), sizeof(OutputDigest));
  const auto validated = Clock::now();
  for (auto& part : digests) {
    auto interior = std::move(part.interior);
    auto shell = std::move(part.shell);
  }
  consumers.clear();
  index.reset();
  cloud.reset();
  input.reset();
  const auto finished = Clock::now();
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17)
      << "{\"schema\":\"mhgp8_wspd_q2_ranges_probe_v1\",\"status\":\"completed\""
      << ",\"phase\":\"exploration_v8_hors_registre\",\"backend\":\"cpu_reference\""
      << ",\"profile\":\"quantized_u16_input_only\",\"mode\":\"implementation_v8_p0\""
      << ",\"public_status\":\"not_claimed\",\"scope\":\"q2_all_cloud_supports_not_full\""
      << ",\"separation_convention\":\"box_gap_diameter_v1\",\"gcp_used\":false"
      << ",\"execution\":\"anchor_ranges_front_census\""
      << ",\"front_mode\":\"samples\",\"census_mode\":\"shared\",\"sibling_mode\":\"sibling\""
      << ",\"witness_order\":\"complement\",\"anchor_mode\":\"anchors\""
      << ",\"clock_contract\":\"wall_and_worker_sums_no_subtraction_v1\""
      << ",\"worker_clock_scope\":\"presence_including_waits\""
      << ",\"pool_clock_scope\":\"preparation_plus_active_range_intervals\""
      << ",\"n\":" << o.n << ",\"family\":\"" << o.family << "\",\"kmax\":" << o.kmax
      << ",\"s\":" << o.separation << ",\"seed\":" << o.seed << ",\"threads\":" << o.threads
      << ",\"jobs_per_worker\":" << o.jobs_per_worker << ",\"pool_min_factor\":" << o.pool_min_factor
      << ",\"anchor_grain\":" << o.anchor_grain << ",\"queue_capacity\":" << o.queue_capacity
      << ",\"recipe\":\"" << mhgp8::bench::front_recipe(o.family) << "\",\"seed_affects_input\":"
      << (o.family == "rows" ? "false" : "true") << ",\"input_hash\":\"" << std::hex << input_hash << std::dec << '"'
      << ",\"total_unordered_pairs\":" << result.front.total_unordered_pairs
      << ",\"active_lane_mask\":" << static_cast<unsigned>(result.front.active_lane_mask)
      << ",\"input_rectangles\":" << result.input_rectangles << ",\"anchor_queries\":" << result.anchor_queries
      << ",\"candidate_pairs\":" << result.candidate_pairs << ",\"accepted_pairs\":" << result.accepted_pairs
      << ",\"rejected_pairs\":" << result.rejected_pairs << ",\"generation_work\":{";
  print_fields(generation_work, generation_fields);
  std::cout << "},\"cloud_work\":{"; print_fields(cloud_work, cloud_fields);
  std::cout << "},\"index_work\":{"; print_fields(index_work, index_fields);
  std::cout << "},\"front_work\":{"; print_fields(result.front.work, front_fields);
  std::cout << ",\"size_class_rectangles\":"; print_array(result.front.work.size_class_rectangles);
  std::cout << ",\"size_class_pair_mass\":"; print_array(result.front.work.size_class_pair_mass);
  std::cout << ",\"rejected_pair_mass\":"; print_array(result.front.work.rejected_pair_mass);
  std::cout << ",\"residual_pair_mass\":"; print_array(result.front.work.residual_pair_mass);
  std::cout << ",\"lane_rectangles\":"; print_array(result.front.work.lane_rectangles);
  std::cout << "},\"census_work\":{"; print_fields(result.census_work, census_fields);
  std::cout << "},\"sibling_work\":{"; print_fields(result.sibling_work, sibling_fields);
  std::cout << "},\"order_work\":{"; print_fields(result.order_work, order_fields);
  std::cout << "},\"joint_work\":{"; print_fields(result.joint_work, joint_fields);
  std::cout << "},\"pool_work\":{"; print_fields(result.pool_work, pool_fields);
  std::cout << ",\"preparation_ms_sum\":" << result.pool_work.preparation_ms
            << ",\"selected_total_ms_sum\":" << result.pool_work.selected_total_ms;
  std::cout << "},\"callback_work\":{"; print_fields(digest.work, callback_fields);
  std::cout << "},\"digest\":"; print_digest(digest);
  std::cout << ",\"parallel_work\":{\"requested_workers\":" << result.requested_workers
      << ",\"started_workers\":" << result.started_workers << ",\"target_jobs\":" << result.target_jobs
      << ",\"jobs\":" << result.jobs << ",\"completed_jobs\":" << result.completed_jobs
      << ",\"terminal_jobs\":" << result.terminal_jobs << ",\"prefix_product_visits\":" << result.prefix_product_visits
      << ",\"job_storage_bytes\":" << result.job_storage_bytes << ",\"pool_peak_bytes_sum\":" << result.pool_peak_bytes_sum
      << ",\"queue_storage_bytes\":" << result.queue_storage_bytes << "},\"range_work\":";
  print_ranges(ranges.work);
  std::cout << ",\"range_storage\":{\"task_bytes\":" << ranges.range_task_bytes
            << ",\"max_live_pool_parents\":" << ranges.max_live_pool_parents
            << ",\"max_live_pool_bytes\":" << ranges.max_live_pool_bytes << '}';
  std::cout << ",\"workers\":[";
  for (std::size_t i = 0; i < result.workers.size(); ++i) {
    if (i != 0) std::cout << ',';
    const auto& w = result.workers[i];
    std::cout << "{\"slot\":" << i << ",\"jobs\":" << w.jobs << ",\"front_products\":" << w.front_products
        << ",\"input_rectangles\":" << w.input_rectangles << ",\"count_node_visits\":" << w.count_node_visits
        << ",\"supports\":" << w.supports << ",\"pool_peak_bytes\":" << w.pool_peak_bytes
        << ",\"elapsed_ms\":" << w.elapsed_ms << ",\"payload_ms\":" << w.payload_ms
        << ",\"callback_buffers_capacity_bytes\":" << worker_callback_bytes[i] << ",\"callback_work\":{";
    print_fields(digests[i].work, callback_fields);
    std::cout << "},\"digest\":"; print_digest(digests[i]);
    std::cout << ",\"range_work\":";
    print_ranges(ranges.workers[i].work);
    std::cout << '}';
  }
  std::cout << "],\"memory\":{\"input_capacity_bytes\":" << input_bytes << ",\"cloud_retained_bytes\":" << cloud_bytes
      << ",\"index_retained_bytes\":" << index_bytes << ",\"callback_buffers_capacity_bytes\":" << callback_bytes
      << ",\"callback_state_bytes\":" << callback_state_bytes << ",\"callback_slots\":" << digests.size()
      << "},\"timings\":{\"generation_ms\":" << milliseconds(started, generated)
      << ",\"cloud_ms\":" << milliseconds(generated, prepared) << ",\"index_ms\":" << milliseconds(prepared, indexed)
      << ",\"pipeline_and_callback_ms\":" << milliseconds(indexed, processed)
      << ",\"validation_ms\":" << milliseconds(processed, validated)
      << ",\"destruction_ms\":" << milliseconds(validated, finished) << ",\"total_ms\":" << milliseconds(started, finished)
      << ",\"pipeline_wall_ms\":" << result.total_ms << ",\"partition_ms\":" << result.partition_ms
      << ",\"worker_ms_sum\":" << result.worker_ms_sum << ",\"payload_ms_sum\":" << result.payload_ms_sum << "}}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "mhgp8 ranges probe: " << error.what() << '\n';
    return 2;
  }
}
