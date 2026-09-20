// Explicit audit adaptation of morsehgp3D_v8/bench/wspd_q2_inheritance_probe.cpp
// SHA256 92a5a8681cb22a6479f6efa70d4f25f09bc16150bd44ac9a8f8916b76c784414
// Public engine unchanged; file input replaces the synthetic generator.
#include "parallel_probe_common.hpp"

#include <limits>
#include <fstream>

// Explicit port of the proposals probe for the inherited witness identifiers
// of the front (WspdFrontProposals::inherit_witnesses). front_work keeps the
// historical key set; the extension and inheritance counters are reported
// apart, with the ledger theorems restated for every combination of options.
namespace {

struct Options {
  std::size_t n;
  std::string_view family;
  unsigned kmax;
  unsigned separation;
  u64 seed;
  std::size_t threads;
  std::size_t jobs_per_worker;
  std::size_t pool_min_factor;
  mhgp8::WspdFrontProposals proposals;
};

Options options(int argc, char** argv) {
  if (argc != 12)
    throw std::invalid_argument("usage: mhgp8_wspd_q2_inheritance_probe n family K s seed "
                                "threads jobs_per_worker pool_min_factor window_factor small_factor_limit|all "
                                "inherit_witnesses(0|1)");
  const std::string_view limit(argv[10]);
  const auto flag = [](std::string_view text) {
    if (text != "0" && text != "1") throw std::invalid_argument("inheritance probe flag is exactly 0 or 1");
    return text == "1";
  };
  const mhgp8::WspdFrontProposals proposals{integer<unsigned>(argv[9]),
      limit == "all" ? std::numeric_limits<std::size_t>::max() : integer<std::size_t>(argv[10]),
      flag(argv[11])};
  if ((proposals.window_factor != 1 && proposals.window_factor != 2 && proposals.window_factor != 4) ||
      proposals.small_factor_limit == 0 ||
      (limit != "all" && proposals.small_factor_limit == std::numeric_limits<std::size_t>::max()))
    throw std::invalid_argument("inheritance probe requires window_factor 1, 2 or 4 and a positive limit or all");
  const Options o{integer<std::size_t>(argv[1]), argv[2], integer<unsigned>(argv[3]),
                  integer<unsigned>(argv[4]), integer<u64>(argv[5]),
                  integer<std::size_t>(argv[6]), integer<std::size_t>(argv[7]),
                  integer<std::size_t>(argv[8]), proposals};
  if (o.n < 2) throw std::invalid_argument("input needs two sites");
  if (o.kmax == 0 || o.kmax > 10 || o.separation == 0 || o.jobs_per_worker == 0)
    throw std::invalid_argument("inheritance probe requires K1..10, positive s and jobs_per_worker");
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
        o.pool_min_factor, {}, o.proposals);
  const auto mono = mhgp8::run_wspd_q2_census(*index, o.kmax, o.separation,
      mhgp8::WspdFrontMode::MidpointSamples, mhgp8::Q2CensusMode::SharedBlocks,
      consumers.front(), mhgp8::Q2SiblingMode::Saturating,
      mhgp8::Q2WitnessOrder::ComplementFirst, mhgp8::Q2AnchorMode::Individual,
      o.pool_min_factor, o.proposals);
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
      mono.pool_work.plan_peak_bytes, mono.total_ms, mono.census.payload_ms});
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
  // Extension ledger of the q2 lane. Bounded by the widened window, zero for
  // factor 1, and its removal leaves a historical-window work within the
  // historical bounds. An extended product ran its whole historical window
  // and then proposed at least one extra rank; a product rejected by the
  // extension received at least one extra credit; a searched product that is
  // not extended was rejected by its historical window or is not eligible.
  const auto& f = r.front.work;
  const auto historical = std::min<u64>(o.kmax, size_counter(o.n));
  const auto wider = std::min<u64>(u64{o.kmax} * o.proposals.window_factor, size_counter(o.n));
  require(f.extended_products <= f.witness_searches && f.extended_products <= f.extended_proposals &&
          f.extended_proposals <= product(wider - historical, f.extended_products) &&
          f.extended_proposals <= f.proposed_sites &&
          f.extended_proposals_in_factors <= f.extended_proposals &&
          f.extended_proposals_in_factors <= f.proposals_in_factors &&
          f.extended_inherited_duplicates <= f.inherited_duplicates &&
          f.extended_inherited_duplicates <= f.extended_proposals - f.extended_proposals_in_factors &&
          f.extended_credits <= f.witness_lane_credits &&
          f.extended_credits <= f.extended_proposals - f.extended_proposals_in_factors -
                                    f.extended_inherited_duplicates &&
          f.extended_rejections <= f.extended_credits && f.extended_rejections <= f.extended_products &&
          f.extended_rejections <= f.fully_rejected_products,
          "front proposal extension ledger mismatch");
  const auto unextended = f.witness_searches - f.extended_products;
  const auto historical_proposals = f.proposed_sites - f.extended_proposals;
  require(historical_proposals <= product(historical, f.witness_searches) &&
          historical_proposals >= product(historical, f.extended_products) + unextended &&
          unextended >= f.fully_rejected_products - f.extended_rejections,
          "historical-window part of the proposal ledger mismatch");
  const bool unlimited = o.proposals.small_factor_limit == std::numeric_limits<std::size_t>::max();
  if (o.proposals.window_factor != 1 && unlimited)
    require(unextended == f.fully_rejected_products - f.extended_rejections,
            "an unlimited widened window left a surviving product without extension");
  if (o.proposals.window_factor == 1)
    require(f.extended_products == 0 && f.extended_proposals == 0 && f.extended_proposals_in_factors == 0 &&
            f.extended_credits == 0 && f.extended_rejections == 0 && f.extended_inherited_duplicates == 0,
            "historical window reported extension work");
  // Every proposed rank is in a factor, already received, or tested: once.
  require(f.proposed_sites == f.proposals_in_factors + f.inherited_duplicates + f.h_bound_tests &&
          f.witness_lane_credits <= f.h_bound_tests, "proposed-rank partition mismatch");
  // One root descent per search: at least one step (a search needs n >= Kmax+2
  // sites, so the root is not a leaf), at most the index depth, two box
  // distances per step.
  require(f.witness_box_distance_tests == 2 * f.witness_descent_steps &&
          f.witness_searches <= f.witness_descent_steps &&
          f.witness_descent_steps <= product(48, f.witness_searches),
          "descent ledger mismatch");
  if (o.proposals.inherit_witnesses) {
    // Credit ledger: every searched product ends rejected with Kmax credits,
    // emitted with its final credits, or split, its final credits being
    // received once by each of its two (searched) children.
    require(f.inherited_duplicates <= f.inherited_credits &&
            f.inherited_credits <= product(o.kmax - 1, f.witness_searches) && f.inherited_credits % 2 == 0 &&
            f.inherited_rejections <= f.fully_rejected_products &&
            f.emitted_witness_credits <= product(o.kmax - 1, f.emitted_rectangles) &&
            f.witness_lane_credits + f.inherited_credits / 2 ==
                product(o.kmax, f.fully_rejected_products) + f.emitted_witness_credits,
            "inheritance ledger mismatch");
  } else {
    require(f.inherited_credits == 0 && f.inherited_duplicates == 0 && f.extended_inherited_duplicates == 0 &&
            f.inherited_rejections == 0 && f.emitted_witness_credits == 0,
            "default front reported inheritance work");
  }
}

mhgp8::bench::FrontFixture load_fixture(const Options& o) {
  std::ifstream file(std::string(o.family), std::ios::binary | std::ios::ate);
  require(file.is_open(), "cannot open u16le cloud");
  require(file.tellg() == static_cast<std::streamoff>(product(o.n, 6)), "input size mismatch");
  file.seekg(0);
  mhgp8::bench::FrontFixture result;
  result.points.reserve(o.n);
  for (std::size_t i = 0; i < o.n; ++i) {
    std::array<unsigned char, 6> raw{};
    file.read(reinterpret_cast<char*>(raw.data()), 6);
    require(file.gcount() == 6 && !file.bad(), "truncated input");
    std::array<std::uint16_t, 3> coordinates{};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      coordinates[axis] = static_cast<std::uint16_t>(static_cast<unsigned>(raw[2*axis]) |
                      (static_cast<unsigned>(raw[2*axis+1]) << 8U));
      mhgp8::bench::front_hash_word(result.input_hash, coordinates[axis]);
    }
    result.points.push_back({coordinates[0], coordinates[1], coordinates[2]});
  }
  require(file.peek() == std::char_traits<char>::eof() && !file.bad(), "input length changed");
  return result;
}

int run(const Options& o) {
  const auto started = Clock::now();
  std::optional<mhgp8::bench::FrontFixture> input(load_fixture(o));
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
      << "{\"schema\":\"mhgp8_audit_front_options_lidar_v1\",\"status\":\"completed\""
      << ",\"phase\":\"exploration_v8_hors_registre\",\"backend\":\"cpu_reference\""
      << ",\"profile\":\"quantized_u16_input_only\",\"mode\":\"audit_independant_math_and_architecture\""
      << ",\"public_status\":\"not_claimed\",\"scope\":\"q2_all_cloud_supports_not_full\""
      << ",\"separation_convention\":\"box_gap_diameter_v1\",\"gcp_used\":false"
      << ",\"execution\":\"" << (o.threads == 0 ? "mono_reference" : "parallel_front") << '"'
      << ",\"front_mode\":\"samples\",\"census_mode\":\"shared\",\"sibling_mode\":\"sibling\""
      << ",\"witness_order\":\"complement\",\"anchor_mode\":\"anchors\""
      << ",\"clock_contract\":\"wall_and_worker_sums_no_subtraction_v1\""
      << ",\"n\":" << o.n << ",\"family\":\"" << o.family << "\",\"kmax\":" << o.kmax
      << ",\"s\":" << o.separation << ",\"seed\":" << o.seed << ",\"threads\":" << o.threads
      << ",\"jobs_per_worker\":" << o.jobs_per_worker << ",\"pool_min_factor\":" << o.pool_min_factor
      << ",\"window_factor\":" << o.proposals.window_factor << ",\"small_factor_limit\":";
  if (o.proposals.small_factor_limit == std::numeric_limits<std::size_t>::max()) std::cout << "null";
  else std::cout << o.proposals.small_factor_limit;
  std::cout << ",\"inherit_witnesses\":" << (o.proposals.inherit_witnesses ? "true" : "false");
  std::cout
      << ",\"recipe\":\"" << "existing_u16le_sites_no_transform" << "\",\"seed_affects_input\":"
      << "false" << ",\"input_hash\":\"" << std::hex << input_hash << std::dec << '"'
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
  std::cout << "},\"extension_work\":{\"extended_products\":" << result.front.work.extended_products
            << ",\"extended_proposals\":" << result.front.work.extended_proposals
            << ",\"extended_proposals_in_factors\":" << result.front.work.extended_proposals_in_factors
            << ",\"extended_credits\":" << result.front.work.extended_credits
            << ",\"extended_rejections\":" << result.front.work.extended_rejections;
  std::cout << "},\"inheritance_work\":{\"inherited_credits\":" << result.front.work.inherited_credits
            << ",\"inherited_duplicates\":" << result.front.work.inherited_duplicates
            << ",\"extended_inherited_duplicates\":" << result.front.work.extended_inherited_duplicates
            << ",\"inherited_rejections\":" << result.front.work.inherited_rejections
            << ",\"emitted_witness_credits\":" << result.front.work.emitted_witness_credits;
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
      << "},\"workers\":[";
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
    std::cerr << "mhgp8 inheritance probe: " << error.what() << '\n';
    return 2;
  }
}
