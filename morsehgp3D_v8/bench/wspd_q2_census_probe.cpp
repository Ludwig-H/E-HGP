#include "front_fixtures.hpp"
#include "pipeline/wspd_q2_census.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <locale>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using mhgp8::u64;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template <class Integer>
Integer integer(std::string_view text) {
  Integer value{};
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
    throw std::invalid_argument("invalid unsigned integer");
  return value;
}

u64 size_counter(std::size_t value) {
  if (std::cmp_greater(value, std::numeric_limits<u64>::max()))
    throw std::overflow_error("WSPD census probe size exceeds u64");
  return static_cast<u64>(value);
}

u64 product(u64 left, u64 right) {
  if (right != 0 && left > std::numeric_limits<u64>::max() / right)
    throw std::overflow_error("WSPD census probe product exceeds u64");
  return left * right;
}

struct Options {
  std::size_t n;
  std::string_view family;
  unsigned kmax;
  unsigned separation;
  u64 seed;
  std::string_view front_mode;
  std::string_view census_mode;
  std::string_view sibling_mode;  // Empty preserves the historical v1 CLI/schema.
  std::string_view witness_order;  // Empty preserves the v1/v2 schemas.
};

Options options(int argc, char** argv) {
  if (argc != 8 && argc != 9 && argc != 10)
    throw std::invalid_argument("usage: mhgp8_wspd_q2_census_probe n uniform|terrain|clusters|rows "
                                "Kmax s seed pure|samples pairwise|shared [none|sibling [global|complement]]");
  const Options result{integer<std::size_t>(argv[1]), argv[2], integer<unsigned>(argv[3]),
                       integer<unsigned>(argv[4]), integer<u64>(argv[5]), argv[6], argv[7],
                       argc >= 9 ? std::string_view(argv[8]) : std::string_view{},
                       argc == 10 ? std::string_view(argv[9]) : std::string_view{}};
  mhgp8::bench::validate_front_fixture_size(result.n, result.family);
  if (result.kmax == 0 || result.kmax > 10 || result.separation == 0 ||
      (result.front_mode != "pure" && result.front_mode != "samples") ||
      (result.census_mode != "pairwise" && result.census_mode != "shared") ||
      (argc >= 9 && result.sibling_mode != "none" && result.sibling_mode != "sibling") ||
      (result.sibling_mode == "sibling" && result.census_mode != "shared") ||
      (argc == 10 && result.witness_order != "global" && result.witness_order != "complement") ||
      (result.witness_order == "complement" && result.census_mode != "shared"))
    throw std::invalid_argument("WSPD census requires Kmax 1..10, positive s, valid front/census modes");
  return result;
}

double milliseconds(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

struct CallbackWork {
  u64 copied_ids{};
  u64 sort_calls{};
  u64 sort_comparisons{};
  u64 validation_ids{};
  u64 adjacent_tests{};
  u64 cross_set_comparisons{};
  u64 support_key_axis_checks{};
  u64 hash_words{};
};

// Explicitly adapted from q2_census_probe.cpp's streaming physical support
// digest. Encoding v2 sorts materialized IDs rather than hashing their order
// or using an inner commutative checksum. All sorting/validation is paid here.
struct OutputDigest {
  u64 supports{};
  u64 interior_ids{};
  u64 shell_ids{};
  u64 sum{};
  u64 xor_value{};
  CallbackWork work;
  std::vector<std::size_t> interior;
  std::vector<std::size_t> shell;

  void consume(const mhgp8::Q2Support& support, std::span<const mhgp8::Point3> points,
               unsigned kmax) {
    require(support.a_id < points.size() && support.b_id < points.size() &&
            support.a_id != support.b_id && support.interior.size() < kmax &&
            support.shell.size() >= 2, "invalid materialized q2 support");
    const auto a = points[support.a_id];
    const auto b = points[support.b_id];
    u64 diameter_squared = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      mhgp8::counter_add(work.support_key_axis_checks);
      require(support.key.center_twice[axis] == static_cast<unsigned>(a[axis]) + b[axis],
              "support key midpoint disagrees with its endpoints");
      const mhgp8::i64 delta = static_cast<mhgp8::i64>(a[axis]) - b[axis];
      mhgp8::counter_add(diameter_squared, static_cast<u64>(delta * delta));
    }
    require(diameter_squared == support.key.diameter_squared,
            "support key diameter disagrees with its endpoints");
    interior.assign(support.interior.begin(), support.interior.end());
    shell.assign(support.shell.begin(), support.shell.end());
    mhgp8::counter_add(work.copied_ids, size_counter(interior.size()));
    mhgp8::counter_add(work.copied_ids, size_counter(shell.size()));
    bool saw_a = false;
    bool saw_b = false;
    for (const bool is_shell : {false, true}) {
      auto& ids = is_shell ? shell : interior;
      mhgp8::counter_add(work.sort_calls);
      std::sort(ids.begin(), ids.end(), [&](std::size_t left, std::size_t right) {
        mhgp8::counter_add(work.sort_comparisons);
        return left < right;
      });
      for (std::size_t i = 0; i < ids.size(); ++i) {
        mhgp8::counter_add(work.validation_ids);
        require(ids[i] < points.size(), "materialized ID escaped its immutable cloud");
        require(is_shell || (ids[i] != support.a_id && ids[i] != support.b_id),
                "support endpoint was emitted as a strict interior");
        if (i != 0) {
          mhgp8::counter_add(work.adjacent_tests);
          require(ids[i - 1] != ids[i], "duplicate materialized ID in a q2 support");
        }
        if (is_shell) {
          saw_a = saw_a || ids[i] == support.a_id;
          saw_b = saw_b || ids[i] == support.b_id;
        }
      }
    }
    require(saw_a && saw_b, "materialized shell lost a support endpoint");
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < interior.size() && j < shell.size()) {
      mhgp8::counter_add(work.cross_set_comparisons);
      require(interior[i] != shell[j], "q2 interior and shell overlap");
      if (interior[i] < shell[j]) ++i;
      else ++j;
    }
    u64 hash = 14695981039346656037ULL;
    const auto word = [&](u64 value) {
      mhgp8::counter_add(work.hash_words);
      mhgp8::bench::front_hash_word(hash, value);
    };
    word(2);  // Canonical physical support encoding version.
    word(size_counter(std::min(support.a_id, support.b_id)));
    word(size_counter(std::max(support.a_id, support.b_id)));
    for (const auto coordinate : support.key.center_twice) word(coordinate);
    word(support.key.diameter_squared);
    word(size_counter(interior.size()));
    for (const auto id : interior) word(size_counter(id));
    word(size_counter(shell.size()));
    for (const auto id : shell) word(size_counter(id));
    mhgp8::counter_add(supports);
    mhgp8::counter_add(interior_ids, size_counter(interior.size()));
    mhgp8::counter_add(shell_ids, size_counter(shell.size()));
    sum += hash;  // Only checksums, never work/cardinality counters, wrap.
    xor_value ^= hash;
  }

  u64 buffers_capacity_bytes() const {
    auto result = product(size_counter(interior.capacity()), sizeof(std::size_t));
    mhgp8::counter_add(result, product(size_counter(shell.capacity()), sizeof(std::size_t)));
    return result;
  }
};

template <class T>
struct Field { const char* name; u64 T::* member; };

#define MHGP8_FIELD(type, name) Field<type>{#name, &type::name}
const std::array generation_fields{
  MHGP8_FIELD(mhgp8::bench::FrontGenerationWork, rng_calls),
  MHGP8_FIELD(mhgp8::bench::FrontGenerationWork, proposed_points),
  MHGP8_FIELD(mhgp8::bench::FrontGenerationWork, duplicate_rejections),
  MHGP8_FIELD(mhgp8::bench::FrontGenerationWork, accepted_points),
  MHGP8_FIELD(mhgp8::bench::FrontGenerationWork, ordered_set_comparisons)};
const std::array cloud_fields{
  MHGP8_FIELD(mhgp8::CloudWork, coordinate_copies),
  MHGP8_FIELD(mhgp8::CloudWork, validation_points),
  MHGP8_FIELD(mhgp8::CloudWork, uniqueness_comparisons),
  MHGP8_FIELD(mhgp8::CloudWork, uniqueness_adjacent_tests),
  MHGP8_FIELD(mhgp8::CloudWork, range_tree_leaf_visits),
  MHGP8_FIELD(mhgp8::CloudWork, range_tree_nodes),
  MHGP8_FIELD(mhgp8::CloudWork, range_tree_merges)};
const std::array index_fields{
  MHGP8_FIELD(mhgp8::Q2IndexWork, point_visits), MHGP8_FIELD(mhgp8::Q2IndexWork, nodes),
  MHGP8_FIELD(mhgp8::Q2IndexWork, max_depth), MHGP8_FIELD(mhgp8::Q2IndexWork, escape_links)};
const std::array front_fields{
  MHGP8_FIELD(mhgp8::WspdFrontWork, product_visits),
  MHGP8_FIELD(mhgp8::WspdFrontWork, diagonal_splits),
  MHGP8_FIELD(mhgp8::WspdFrontWork, diagonal_leaves),
  MHGP8_FIELD(mhgp8::WspdFrontWork, disjoint_splits),
  MHGP8_FIELD(mhgp8::WspdFrontWork, separation_tests),
  MHGP8_FIELD(mhgp8::WspdFrontWork, witness_searches),
  MHGP8_FIELD(mhgp8::WspdFrontWork, witness_descent_steps),
  MHGP8_FIELD(mhgp8::WspdFrontWork, witness_box_distance_tests),
  MHGP8_FIELD(mhgp8::WspdFrontWork, proposed_sites),
  MHGP8_FIELD(mhgp8::WspdFrontWork, proposals_in_factors),
  MHGP8_FIELD(mhgp8::WspdFrontWork, h_bound_tests),
  MHGP8_FIELD(mhgp8::WspdFrontWork, xi_bound_tests),
  MHGP8_FIELD(mhgp8::WspdFrontWork, witness_lane_credits),
  MHGP8_FIELD(mhgp8::WspdFrontWork, fully_rejected_products),
  MHGP8_FIELD(mhgp8::WspdFrontWork, emitted_rectangles),
  MHGP8_FIELD(mhgp8::WspdFrontWork, emitted_factor_sites),
  MHGP8_FIELD(mhgp8::WspdFrontWork, max_factor_size),
  MHGP8_FIELD(mhgp8::WspdFrontWork, leaf_pair_rectangles),
  MHGP8_FIELD(mhgp8::WspdFrontWork, max_stack_size),
  MHGP8_FIELD(mhgp8::WspdFrontWork, max_product_depth)};
const std::array census_fields{
  MHGP8_FIELD(mhgp8::Q2CensusWork, query_build_point_visits),
  MHGP8_FIELD(mhgp8::Q2CensusWork, query_build_nodes),
  MHGP8_FIELD(mhgp8::Q2CensusWork, query_build_max_depth),
  MHGP8_FIELD(mhgp8::Q2CensusWork, input_descriptors),
  MHGP8_FIELD(mhgp8::Q2CensusWork, query_cover_visits),
  MHGP8_FIELD(mhgp8::Q2CensusWork, query_tasks),
  MHGP8_FIELD(mhgp8::Q2CensusWork, query_splits),
  MHGP8_FIELD(mhgp8::Q2CensusWork, witness_splits),
  MHGP8_FIELD(mhgp8::Q2CensusWork, count_root_starts),
  MHGP8_FIELD(mhgp8::Q2CensusWork, shared_splits_after_credit),
  MHGP8_FIELD(mhgp8::Q2CensusWork, cursor_advances),
  MHGP8_FIELD(mhgp8::Q2CensusWork, cursor_reuses),
  MHGP8_FIELD(mhgp8::Q2CensusWork, count_node_visits),
  MHGP8_FIELD(mhgp8::Q2CensusWork, count_bound_tests),
  MHGP8_FIELD(mhgp8::Q2CensusWork, count_point_tests),
  MHGP8_FIELD(mhgp8::Q2CensusWork, uniform_credited_pairs),
  MHGP8_FIELD(mhgp8::Q2CensusWork, uniform_rejected_pairs),
  MHGP8_FIELD(mhgp8::Q2CensusWork, uniform_accepted_pairs),
  MHGP8_FIELD(mhgp8::Q2CensusWork, consumed_witness_sites),
  MHGP8_FIELD(mhgp8::Q2CensusWork, frontier_restarts),
  MHGP8_FIELD(mhgp8::Q2CensusWork, payload_node_visits),
  MHGP8_FIELD(mhgp8::Q2CensusWork, payload_bound_tests),
  MHGP8_FIELD(mhgp8::Q2CensusWork, payload_point_tests),
  MHGP8_FIELD(mhgp8::Q2CensusWork, payload_interior_sites),
  MHGP8_FIELD(mhgp8::Q2CensusWork, payload_shell_sites),
  MHGP8_FIELD(mhgp8::Q2CensusWork, payload_supports)};
const std::array callback_fields{
  MHGP8_FIELD(CallbackWork, copied_ids), MHGP8_FIELD(CallbackWork, sort_calls),
  MHGP8_FIELD(CallbackWork, sort_comparisons), MHGP8_FIELD(CallbackWork, validation_ids),
  MHGP8_FIELD(CallbackWork, adjacent_tests), MHGP8_FIELD(CallbackWork, cross_set_comparisons),
  MHGP8_FIELD(CallbackWork, support_key_axis_checks), MHGP8_FIELD(CallbackWork, hash_words)};
using SiblingWork = decltype(mhgp8::WspdQ2CensusResult{}.sibling_work);
const std::array sibling_fields{
  MHGP8_FIELD(SiblingWork, proposals), MHGP8_FIELD(SiblingWork, cardinality_skips),
  MHGP8_FIELD(SiblingWork, bound_tests), MHGP8_FIELD(SiblingWork, rejected_tasks),
  MHGP8_FIELD(SiblingWork, rejected_pairs), MHGP8_FIELD(SiblingWork, rejected_after_credit)};
using OrderWork = decltype(mhgp8::WspdQ2CensusResult{}.order_work);
const std::array order_fields{
  MHGP8_FIELD(OrderWork, structural_splits), MHGP8_FIELD(OrderWork, deferred_skips),
  MHGP8_FIELD(OrderWork, anchor_skips), MHGP8_FIELD(OrderWork, phase_switches)};
#undef MHGP8_FIELD

template <class T, std::size_t N>
void print_fields(const T& value, const std::array<Field<T>, N>& fields) {
  for (std::size_t i = 0; i < N; ++i) {
    if (i != 0) std::cout << ',';
    std::cout << '"' << fields[i].name << "\":" << value.*(fields[i].member);
  }
}

template <std::size_t N>
void print_array(const std::array<u64, N>& values) {
  std::cout << '[';
  for (std::size_t i = 0; i < N; ++i) {
    if (i != 0) std::cout << ',';
    std::cout << values[i];
  }
  std::cout << ']';
}

void validate(const mhgp8::WspdQ2CensusResult& result, const OutputDigest& digest, const Options& o) {
  const auto n = size_counter(o.n);
  const auto total = n % 2 == 0 ? product(n / 2, n - 1) : product(n, (n - 1) / 2);
  const auto& f = result.front;
  const auto& c = result.census;
  require(f.total_unordered_pairs == total && f.active_lane_mask == 1,
          "q2-only front total or mask mismatch");
  require(f.work.rejected_pair_mass[0] <= total &&
          f.work.residual_pair_mass[0] == total - f.work.rejected_pair_mass[0] &&
          f.work.lane_rectangles[0] == f.work.emitted_rectangles && f.work.xi_bound_tests == 0,
          "q2 front pair mass ledger mismatch");
  for (unsigned lane = 1; lane < 3; ++lane) {
    require(f.work.rejected_pair_mass[lane] == 0 && f.work.residual_pair_mass[lane] == 0 &&
            f.work.lane_rectangles[lane] == 0, "q2 pipeline unexpectedly used a higher lane");
  }
  require(c.candidate_pairs == f.work.residual_pair_mass[0] &&
          c.accepted_pairs <= c.candidate_pairs &&
          c.rejected_pairs == c.candidate_pairs - c.accepted_pairs &&
          result.input_rectangles == f.work.emitted_rectangles &&
          result.anchor_queries >= result.input_rectangles && result.anchor_queries <= c.candidate_pairs,
          "WSPD census rectangle, anchor or candidate accounting mismatch");
  require(c.work.query_build_point_visits == 0 && c.work.query_build_nodes == 0 &&
          c.work.query_build_max_depth == 0 && c.work.query_cover_visits == 0 &&
          c.work.input_descriptors == result.input_rectangles && c.work.frontier_restarts == 0 &&
          c.work.count_root_starts == (o.census_mode == "shared" ? result.anchor_queries : c.candidate_pairs),
          "WSPD census rebuilt local index or lost its root/continuation contract");
  require(digest.supports == c.accepted_pairs && digest.supports == c.work.payload_supports &&
          digest.interior_ids == c.work.payload_interior_sites &&
          digest.shell_ids == c.work.payload_shell_sites,
          "materialized q2 payload accounting mismatch");
  auto payload_ids = digest.interior_ids;
  mhgp8::counter_add(payload_ids, digest.shell_ids);
  auto hashed_words = product(9, digest.supports);
  mhgp8::counter_add(hashed_words, payload_ids);
  require(digest.work.copied_ids == payload_ids && digest.work.validation_ids == payload_ids &&
          digest.work.sort_calls == product(2, digest.supports) &&
          digest.work.support_key_axis_checks == product(3, digest.supports) &&
          digest.work.hash_words == hashed_words, "canonical callback work accounting mismatch");
  require(c.total_ms == result.total_ms && c.query_index_ms == 0 &&
          c.count_ms >= 0 && c.payload_ms >= 0 &&
          c.total_ms + 1e-6 >= c.count_ms + c.payload_ms,
          "pipeline clocks disagree with their enclosing time contract");
  const auto& sibling = result.sibling_work;
  if (o.sibling_mode == "sibling") {
    require(sibling.proposals == product(2, c.work.query_splits) &&
            sibling.bound_tests <= sibling.proposals &&
            sibling.cardinality_skips == sibling.proposals - sibling.bound_tests &&
            sibling.rejected_tasks <= sibling.bound_tests &&
            sibling.rejected_tasks <= sibling.rejected_pairs &&
            (sibling.rejected_tasks == 0) == (sibling.rejected_pairs == 0) &&
            sibling.rejected_after_credit <= sibling.rejected_tasks &&
            sibling.rejected_pairs <= c.rejected_pairs,
            "sibling certificate work accounting mismatch");
  } else {
    for (const auto& field : sibling_fields)
      require(sibling.*(field.member) == 0, "disabled sibling filter performed work");
  }
  const auto& order = result.order_work;
  if (o.witness_order == "complement") {
    require(order.structural_splits <= product(96, c.work.query_tasks) &&
            order.deferred_skips <= c.work.query_tasks && order.anchor_skips <= c.work.query_tasks &&
            order.phase_switches <= c.work.query_tasks, "complement witness-order work exceeds structural bounds");
  } else {
    for (const auto& field : order_fields)
      require(order.*(field.member) == 0, "global witness order performed complement work");
  }
}

int run(const Options& o) {
  const auto started = Clock::now();
  std::optional<mhgp8::bench::FrontFixture> input(mhgp8::bench::make_front_fixture(o.n, o.family, o.seed));
  const auto generated = Clock::now();
  auto cloud = mhgp8::prepare_cloud(input->points);
  const auto prepared = Clock::now();
  auto index = mhgp8::make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  OutputDigest digest;
  const auto result = mhgp8::run_wspd_q2_census(*index, o.kmax, o.separation,
      o.front_mode == "pure" ? mhgp8::WspdFrontMode::Pure : mhgp8::WspdFrontMode::MidpointSamples,
      o.census_mode == "pairwise" ? mhgp8::Q2CensusMode::Pairwise : mhgp8::Q2CensusMode::SharedBlocks,
      [&](const mhgp8::Q2Support& support) { digest.consume(support, cloud->points(), o.kmax); },
      o.sibling_mode == "sibling" ? mhgp8::Q2SiblingMode::Saturating : mhgp8::Q2SiblingMode::Disabled,
      o.witness_order == "complement" ? mhgp8::Q2WitnessOrder::ComplementFirst : mhgp8::Q2WitnessOrder::GlobalDfs);
  const auto processed = Clock::now();
  validate(result, digest, o);
  require(&index->cloud() == cloud.get(), "WSPD census index lost immutable cloud identity");
  const auto input_hash = input->input_hash;
  const auto generation_work = input->work;
  const auto cloud_work = cloud->work();
  const auto index_work = index->work();
  const auto input_bytes = product(size_counter(input->points.capacity()), sizeof(mhgp8::Point3));
  const auto cloud_bytes = cloud->retained_bytes();
  const auto index_bytes = index->retained_bytes();
  const auto callback_bytes = digest.buffers_capacity_bytes();
  const auto validated = Clock::now();
  {
    // Move into bounded temporary owners to pay destruction now while keeping
    // the streaming scalar digest; no support catalogue was ever retained.
    auto interior = std::move(digest.interior);
    auto shell = std::move(digest.shell);
  }
  index.reset();
  cloud.reset();
  input.reset();
  const auto finished = Clock::now();

  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17)
            << "{\"schema\":\"mhgp8_wspd_q2_census_probe_"
            << (!o.witness_order.empty() ? "v3" : o.sibling_mode.empty() ? "v1" : "v2")
            << "\",\"status\":\"completed\""
            << ",\"phase\":\"exploration_v8_hors_registre\",\"backend\":\"cpu_reference\""
            << ",\"profile\":\"quantized_u16_input_only\",\"mode\":\"implementation_v8_p0\""
            << ",\"public_status\":\"not_claimed\",\"scope\":\"q2_all_cloud_supports_not_full\""
            << ",\"separation_convention\":\"box_gap_diameter_v1\",\"threads\":1,\"gcp_used\":false"
            << ",\"n\":" << o.n << ",\"family\":\"" << o.family << "\",\"kmax\":" << o.kmax
            << ",\"s\":" << o.separation << ",\"seed\":" << o.seed
            << ",\"front_mode\":\"" << o.front_mode << "\",\"census_mode\":\"" << o.census_mode
            << "\",\"recipe\":\"" << mhgp8::bench::front_recipe(o.family)
            << "\",\"seed_affects_input\":" << (o.family == "rows" ? "false" : "true")
            << ",\"input_hash\":\"" << std::hex << input_hash << std::dec << '"'
            << ",\"total_unordered_pairs\":" << result.front.total_unordered_pairs
            << ",\"active_lane_mask\":" << static_cast<unsigned>(result.front.active_lane_mask)
            << ",\"input_rectangles\":" << result.input_rectangles
            << ",\"anchor_queries\":" << result.anchor_queries
            << ",\"candidate_pairs\":" << result.census.candidate_pairs
            << ",\"accepted_pairs\":" << result.census.accepted_pairs
            << ",\"rejected_pairs\":" << result.census.rejected_pairs
            << ",\"generation_work\":{";
  print_fields(generation_work, generation_fields);
  std::cout << "},\"cloud_work\":{";
  print_fields(cloud_work, cloud_fields);
  std::cout << "},\"index_work\":{";
  print_fields(index_work, index_fields);
  std::cout << "},\"front_work\":{";
  print_fields(result.front.work, front_fields);
  std::cout << ",\"size_class_rectangles\":";
  print_array(result.front.work.size_class_rectangles);
  std::cout << ",\"size_class_pair_mass\":";
  print_array(result.front.work.size_class_pair_mass);
  std::cout << ",\"rejected_pair_mass\":";
  print_array(result.front.work.rejected_pair_mass);
  std::cout << ",\"residual_pair_mass\":";
  print_array(result.front.work.residual_pair_mass);
  std::cout << ",\"lane_rectangles\":";
  print_array(result.front.work.lane_rectangles);
  std::cout << "},\"census_work\":{";
  print_fields(result.census.work, census_fields);
  if (!o.sibling_mode.empty()) {
    std::cout << "},\"sibling_mode\":\"" << o.sibling_mode << "\",\"sibling_work\":{";
    print_fields(result.sibling_work, sibling_fields);
  }
  if (!o.witness_order.empty()) {
    std::cout << "},\"witness_order\":\"" << o.witness_order << "\",\"order_work\":{";
    print_fields(result.order_work, order_fields);
  }
  std::cout << "},\"callback_work\":{";
  print_fields(digest.work, callback_fields);
  std::cout << "},\"digest\":{\"encoding\":\"canonical_q2_support_v2\",\"supports\":" << digest.supports
            << ",\"interior_ids\":" << digest.interior_ids << ",\"shell_ids\":" << digest.shell_ids
            << ",\"sum\":\"" << std::hex << digest.sum << "\",\"xor\":\"" << digest.xor_value
            << std::dec << "\"},\"memory\":{\"input_capacity_bytes\":" << input_bytes
            << ",\"cloud_retained_bytes\":" << cloud_bytes << ",\"index_retained_bytes\":" << index_bytes
            << ",\"callback_buffers_capacity_bytes\":" << callback_bytes
            << "},\"timings\":{\"generation_ms\":" << milliseconds(started, generated)
            << ",\"cloud_ms\":" << milliseconds(generated, prepared)
            << ",\"index_ms\":" << milliseconds(prepared, indexed)
            << ",\"pipeline_and_callback_ms\":" << milliseconds(indexed, processed)
            << ",\"validation_ms\":" << milliseconds(processed, validated)
            << ",\"destruction_ms\":" << milliseconds(validated, finished)
            << ",\"total_ms\":" << milliseconds(started, finished)
            << ",\"pipeline_total_ms\":" << result.total_ms
            << ",\"front_and_count_ms\":" << result.census.count_ms
            << ",\"payload_ms\":" << result.census.payload_ms
            << ",\"query_index_ms\":" << result.census.query_index_ms << "}}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(options(argc, argv));
  } catch (const std::invalid_argument& error) {
    std::cerr << "mhgp8_wspd_q2_census_probe: " << error.what() << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_wspd_q2_census_probe failed: " << error.what() << '\n';
    return 1;
  }
}
