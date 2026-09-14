// Explicit protocol port of wspd_q2_parallel_probe.cpp; shared canonical callback
// remains in parallel_probe_common.hpp. No former timing/result is inherited.
#include "dynamic_probe_common.hpp"

namespace {

struct Options {
  std::size_t n;
  std::string_view family;
  unsigned kmax;
  unsigned separation;
  u64 seed;
  std::size_t threads;
  std::size_t jobs_per_worker;
  std::string_view schedule;
  std::size_t queue_capacity;
  std::size_t donation_interval;
  std::size_t pool_min_factor;
};

Options options(int argc, char** argv) {
  if (argc != 11 && argc != 12)
    throw std::invalid_argument("usage: mhgp8_wspd_q2_dynamic_probe n family K s seed "
                                "threads jobs_per_worker coarse|donate queue_capacity "
                                "donation_interval [pool_min_factor=64]");
  const Options o{integer<std::size_t>(argv[1]), argv[2], integer<unsigned>(argv[3]),
                  integer<unsigned>(argv[4]), integer<u64>(argv[5]),
                  integer<std::size_t>(argv[6]), integer<std::size_t>(argv[7]),
                  argv[8], integer<std::size_t>(argv[9]), integer<std::size_t>(argv[10]),
                  argc == 12 ? integer<std::size_t>(argv[11]) : 64};
  validate_input_size(o.n, o.family, o.seed);
  if (o.kmax == 0 || o.kmax > 10 || o.separation == 0 || o.jobs_per_worker == 0 ||
      o.queue_capacity == 0 || o.donation_interval == 0 ||
      (o.schedule != "coarse" && o.schedule != "donate"))
    throw std::invalid_argument("dynamic probe requires K1..10, positive s/jobs/queue/interval and valid schedule");
  static_cast<void>(product(size_counter(o.threads), size_counter(o.jobs_per_worker)));
  return o;
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

mhgp8::WspdQ2ParallelResult run_pipeline(
    const Options& o, const mhgp8::Q2CensusIndexPtr& index,
    std::span<const mhgp8::Q2CensusConsumer> consumers) {
  if (o.threads != 0)
    return mhgp8::run_wspd_q2_census_parallel(index, o.kmax, o.separation,
        mhgp8::WspdFrontMode::MidpointSamples, mhgp8::Q2CensusMode::SharedBlocks,
        consumers, o.jobs_per_worker, mhgp8::Q2SiblingMode::Saturating,
        mhgp8::Q2WitnessOrder::ComplementFirst, mhgp8::Q2AnchorMode::Individual,
        o.pool_min_factor, {o.schedule == "coarse" ? mhgp8::WspdQ2ScheduleMode::Coarse :
            mhgp8::WspdQ2ScheduleMode::Donate, o.queue_capacity, o.donation_interval});
  const auto mono = mhgp8::run_wspd_q2_census(*index, o.kmax, o.separation,
      mhgp8::WspdFrontMode::MidpointSamples, mhgp8::Q2CensusMode::SharedBlocks,
      consumers.front(), mhgp8::Q2SiblingMode::Saturating,
      mhgp8::Q2WitnessOrder::ComplementFirst, mhgp8::Q2AnchorMode::Individual,
      o.pool_min_factor);
  mhgp8::WspdQ2ParallelResult r;
  r.front = mono.front;
  r.census_work = mono.census.work;
  r.sibling_work = mono.sibling_work;
  r.order_work = mono.order_work;
  r.joint_work = mono.joint_work;
  r.pool_work = mono.pool_work;
  r.input_rectangles = mono.input_rectangles;
  r.anchor_queries = mono.anchor_queries;
  r.candidate_pairs = mono.census.candidate_pairs;
  r.accepted_pairs = mono.census.accepted_pairs;
  r.rejected_pairs = mono.census.rejected_pairs;
  // One logical execution slot, no parallel scheduler or launched thread.
  r.started_workers = 1;
  r.pool_peak_bytes_sum = mono.pool_work.plan_peak_bytes;
  r.worker_ms_sum = mono.total_ms;
  r.payload_ms_sum = mono.census.payload_ms;
  r.total_ms = mono.total_ms;
  r.workers.push_back({0, mono.front.work.product_visits, mono.input_rectangles,
      mono.census.work.count_node_visits, mono.census.accepted_pairs,
      mono.pool_work.plan_peak_bytes, mono.total_ms, mono.census.payload_ms, {}});
  return r;
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
          c.query_build_nodes == 0 && c.frontier_restarts == 0, "payload/descriptor mismatch");
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
    validate_dispatch(w.dispatch_work, o.queue_capacity);
    if (o.threads > 0 && o.schedule == "donate")
      require(w.jobs == w.dispatch_work.seeds_completed, "worker seed count mismatch");
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
  validate_dispatch(r.dispatch_work, o.queue_capacity);
  for (std::size_t field_id = 0; field_id < dispatch_fields.size(); ++field_id) {
    const auto member = dispatch_fields[field_id].member;
    u64 merged = 0;
    for (const auto& worker : r.workers) {
      if (field_id >= 12) merged = std::max(merged, worker.dispatch_work.*member);
      else mhgp8::counter_add(merged, worker.dispatch_work.*member);
    }
    require(merged == r.dispatch_work.*member, "dispatcher reduction mismatch");
    if (o.threads == 0 || o.schedule == "coarse") require(merged == 0, "inactive dispatcher paid work");
  }
  if (o.threads > 0 && o.schedule == "donate")
    require(r.dispatch_work.seeds_completed == r.jobs &&
            r.dispatch_work.donations == r.dispatch_work.stolen_completed &&
            r.queue_storage_bytes >= o.queue_capacity, "dispatcher lost seeds, donations or queue storage");
  else require(r.queue_storage_bytes == 0, "inactive dispatcher allocated queue");
}

int run(const Options& o) {
  const auto started = Clock::now();
  std::optional<mhgp8::bench::FrontFixture> input(make_input(o.n, o.family, o.seed));
  const auto generated = Clock::now();
  auto cloud = mhgp8::prepare_cloud(input->points);
  const auto prepared = Clock::now();
  auto index = mhgp8::make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  std::vector<OutputDigest> digests(std::max<std::size_t>(1, o.threads));
  std::vector<mhgp8::Q2CensusConsumer> consumers;
  consumers.reserve(digests.size());
  for (std::size_t i = 0; i < digests.size(); ++i)
    consumers.emplace_back([&, i](const mhgp8::Q2Support& support) {
      digests[i].consume(support, cloud->points(), o.kmax);
    });
  const auto result = run_pipeline(o, index, consumers);
  OutputDigest digest;
  for (const auto& part : digests) merge_digest(digest, part);
  const auto processed = Clock::now();
  validate(result, digest, o, digests);
  require(&index->cloud() == cloud.get(), "parallel index lost its cloud identity");
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
      << "{\"schema\":\"mhgp8_wspd_q2_dynamic_probe_v1\",\"status\":\"completed\""
      << ",\"phase\":\"exploration_v8_hors_registre\",\"backend\":\"cpu_reference\""
      << ",\"profile\":\"quantized_u16_input_only\",\"mode\":\"implementation_v8_p0\""
      << ",\"public_status\":\"not_claimed\",\"scope\":\"q2_all_cloud_supports_not_full\""
      << ",\"separation_convention\":\"box_gap_diameter_v1\",\"gcp_used\":false"
      << ",\"execution\":\"" << (o.threads == 0 ? "mono_reference" : "parallel_front") << '"'
      << ",\"front_mode\":\"samples\",\"census_mode\":\"shared\",\"sibling_mode\":\"sibling\""
      << ",\"witness_order\":\"complement\",\"anchor_mode\":\"anchors\""
      << ",\"clock_contract\":\"wall_and_worker_sums_no_subtraction_v1\""
      << ",\"n\":" << o.n << ",\"family\":" << std::quoted(std::string(o.family)) << ",\"kmax\":" << o.kmax
      << ",\"s\":" << o.separation << ",\"seed\":" << o.seed << ",\"threads\":" << o.threads
      << ",\"jobs_per_worker\":" << o.jobs_per_worker << ",\"pool_min_factor\":" << o.pool_min_factor
      << ",\"schedule\":\"" << o.schedule << "\",\"queue_capacity\":" << o.queue_capacity
      << ",\"donation_interval\":" << o.donation_interval
      << ",\"input_kind\":\"" << (file_family(o.family) ? "file_u16le" : "synthetic")
      << "\",\"input_path\":" << std::quoted(std::string(input_path(o.family)))
      << ",\"input_bytes\":" << (file_family(o.family) ? product(size_counter(o.n), 6) : 0)
      << ",\"input_work\":{\"bytes_read\":" << (file_family(o.family) ? product(size_counter(o.n), 6) : 0)
      << ",\"decoded_points\":" << (file_family(o.family) ? size_counter(o.n) : 0)
      << ",\"hash_words\":" << (file_family(o.family) ? 2 + product(size_counter(o.n), 3) : 0) << '}'
      << ",\"recipe\":\"" << (file_family(o.family) ? std::string_view("xyz_u16le_file_v1") : mhgp8::bench::front_recipe(o.family))
      << "\",\"seed_affects_input\":" << (o.family == "rows" || file_family(o.family) ? "false" : "true")
      << ",\"input_hash\":\"" << std::hex << input_hash << std::dec << '"'
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
      << ",\"queue_storage_bytes\":" << result.queue_storage_bytes << "},\"dispatch_work\":{";
  print_fields(result.dispatch_work, dispatch_fields);
  std::cout << "},\"workers\":[";
  for (std::size_t i = 0; i < result.workers.size(); ++i) {
    if (i != 0) std::cout << ',';
    const auto& w = result.workers[i];
    std::cout << "{\"slot\":" << i << ",\"jobs\":" << w.jobs << ",\"front_products\":" << w.front_products
        << ",\"input_rectangles\":" << w.input_rectangles << ",\"count_node_visits\":" << w.count_node_visits
        << ",\"supports\":" << w.supports << ",\"pool_peak_bytes\":" << w.pool_peak_bytes
        << ",\"elapsed_ms\":" << w.elapsed_ms << ",\"payload_ms\":" << w.payload_ms
        << ",\"callback_buffers_capacity_bytes\":" << worker_callback_bytes[i] << ",\"callback_work\":{";
    print_fields(digests[i].work, callback_fields);
    std::cout << "},\"dispatch_work\":{"; print_fields(w.dispatch_work, dispatch_fields);
    std::cout << "},\"digest\":"; print_digest(digests[i]);
    std::cout << '}';
  }
  std::cout << "],\"memory\":{\"input_capacity_bytes\":" << input_bytes << ",\"cloud_retained_bytes\":" << cloud_bytes
      << ",\"index_retained_bytes\":" << index_bytes << ",\"callback_buffers_capacity_bytes\":" << callback_bytes
      << ",\"callback_state_bytes\":" << callback_state_bytes << ",\"callback_slots\":" << digests.size()
      << "},\"timings\":{\"generation_ms\":" << (file_family(o.family) ? 0 : milliseconds(started, generated))
      << ",\"load_ms\":" << (file_family(o.family) ? milliseconds(started, generated) : 0)
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
    std::cerr << "mhgp8 parallel probe: " << error.what() << '\n';
    return 2;
  }
}
