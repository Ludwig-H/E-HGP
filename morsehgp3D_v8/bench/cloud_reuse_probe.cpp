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
#include <stdexcept>
#include <string_view>
#include <utility>

#include "p0_fixtures.hpp"
#include "sheet_full_fixture.hpp"
#include "pipeline/q2_census.hpp"

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
    throw std::overflow_error("cloud reuse size exceeds u64 counters");
  return static_cast<u64>(value);
}

u64 product(u64 left, u64 right) {
  if (right != 0 && left > std::numeric_limits<u64>::max() / right)
    throw std::overflow_error("cloud reuse counter product overflow");
  return left * right;
}

struct Options {
  std::size_t n;
  std::string_view family;
  unsigned kmax;
  unsigned separation;
  std::size_t rectangles;
  std::string_view order;
};

Options options(int argc, char** argv) {
  if (argc != 7)
    throw std::invalid_argument("usage: mhgp8_cloud_reuse_probe n grid|sheet_full|skew "
                                "kmax s rectangles fresh-first|shared-first");
  Options result{integer<std::size_t>(argv[1]), argv[2], integer<unsigned>(argv[3]),
                  integer<unsigned>(argv[4]), integer<std::size_t>(argv[5]), argv[6]};
  if (result.n < 2 || result.kmax == 0 || result.kmax > 10 || result.separation == 0 ||
      result.rectangles == 0 ||
      (result.family != "grid" && result.family != "sheet_full" && result.family != "skew") ||
      (result.order != "fresh-first" && result.order != "shared-first"))
    throw std::invalid_argument("invalid cloud reuse size, family, Kmax, s, rectangle count or order");
  return result;
}

double milliseconds(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

void hash_word(u64& hash, u64 word) {
  for (unsigned byte = 0; byte < 8; ++byte) {
    hash ^= word & 255U;
    hash *= 1099511628211ULL;
    word >>= 8U;
  }
}

// Explicitly adapted from q2_census_probe.cpp OutputDigest. This is a
// bounded streaming observation of physical supports and materialized IDs,
// not another geometry engine or a proof of producer completeness.
struct OutputDigest {
  u64 supports{};
  u64 interior_ids{};
  u64 shell_ids{};
  u64 sum{};
  u64 xor_value{};
  bool operator==(const OutputDigest&) const = default;

  void consume(const mhgp8::Q2Support& support, std::size_t n, unsigned kmax) {
    require(support.a_id < n && support.b_id < n && support.a_id != support.b_id &&
            support.interior.size() < kmax && support.shell.size() >= 2,
            "invalid support identity or materialized census cardinality");
    u64 hash = 14695981039346656037ULL;
    hash_word(hash, 1);
    hash_word(hash, size_counter(std::min(support.a_id, support.b_id)));
    hash_word(hash, size_counter(std::max(support.a_id, support.b_id)));
    for (const auto coordinate : support.key.center_twice) hash_word(hash, coordinate);
    hash_word(hash, support.key.diameter_squared);
    bool saw_a = false;
    bool saw_b = false;
    for (const bool shell : {false, true}) {
      const auto ids = shell ? support.shell : support.interior;
      u64 id_sum = 0;
      u64 id_xor = 0;
      for (const auto id : ids) {
        require(id < n, "payload ID is outside its immutable cloud");
        require(shell || (id != support.a_id && id != support.b_id),
                "support endpoint was counted as a strict interior");
        if (shell) {
          saw_a = saw_a || id == support.a_id;
          saw_b = saw_b || id == support.b_id;
        }
        u64 item = 14695981039346656037ULL;
        hash_word(item, 1);
        hash_word(item, size_counter(id));
        id_sum += item;  // Deliberate modulo-2^64 checksum arithmetic.
        id_xor ^= item;
      }
      hash_word(hash, size_counter(ids.size()));
      hash_word(hash, id_sum);
      hash_word(hash, id_xor);
    }
    require(saw_a && saw_b, "materialized shell lost a support endpoint");
    mhgp8::counter_add(supports);
    mhgp8::counter_add(interior_ids, size_counter(support.interior.size()));
    mhgp8::counter_add(shell_ids, size_counter(support.shell.size()));
    sum += hash;  // Only checksums, never work/cardinality counters, wrap.
    xor_value ^= hash;
  }
};

template <class T>
struct Field {
  const char* name;
  u64 T::* member;
  bool maximum;
};

#define MHGP8_SUM(type, name) Field<type>{#name, &type::name, false}
#define MHGP8_MAX(type, name) Field<type>{#name, &type::name, true}
const std::array cloud_fields{
  MHGP8_SUM(mhgp8::CloudWork, coordinate_copies),
  MHGP8_SUM(mhgp8::CloudWork, validation_points),
  MHGP8_SUM(mhgp8::CloudWork, uniqueness_comparisons),
  MHGP8_SUM(mhgp8::CloudWork, uniqueness_adjacent_tests),
  MHGP8_SUM(mhgp8::CloudWork, range_tree_leaf_visits),
  MHGP8_SUM(mhgp8::CloudWork, range_tree_nodes),
  MHGP8_SUM(mhgp8::CloudWork, range_tree_merges)};
const std::array index_fields{
  MHGP8_SUM(mhgp8::Q2IndexWork, point_visits), MHGP8_SUM(mhgp8::Q2IndexWork, nodes),
  MHGP8_MAX(mhgp8::Q2IndexWork, max_depth), MHGP8_SUM(mhgp8::Q2IndexWork, escape_links)};
const std::array predicate_fields{
  MHGP8_SUM(mhgp8::PredicateWork, point_tests), MHGP8_SUM(mhgp8::PredicateWork, universal_queries),
  MHGP8_SUM(mhgp8::PredicateWork, q2_axis_terms), MHGP8_SUM(mhgp8::PredicateWork, corner_tests),
  MHGP8_SUM(mhgp8::PredicateWork, block_bound_tests), MHGP8_SUM(mhgp8::PredicateWork, negative_probes)};
const std::array work_fields{
  MHGP8_SUM(mhgp8::Work, validation_points), MHGP8_SUM(mhgp8::Work, uniqueness_comparisons),
  MHGP8_SUM(mhgp8::Work, pool_selection_tests), MHGP8_SUM(mhgp8::Work, pool_selected),
  MHGP8_SUM(mhgp8::Work, tree_nodes), MHGP8_SUM(mhgp8::Work, tree_point_visits),
  MHGP8_SUM(mhgp8::Work, dual_tasks), MHGP8_SUM(mhgp8::Work, credited_blocks),
  MHGP8_SUM(mhgp8::Work, noncredit_blocks), MHGP8_SUM(mhgp8::Work, leaf_pairs),
  MHGP8_SUM(mhgp8::Work, saturated_tasks), MHGP8_MAX(mhgp8::Work, max_tree_depth),
  MHGP8_MAX(mhgp8::Work, max_task_depth), MHGP8_SUM(mhgp8::Work, tube_records),
  MHGP8_SUM(mhgp8::Work, tube_cells), MHGP8_SUM(mhgp8::Work, tube_sort_comparisons),
  MHGP8_SUM(mhgp8::Work, tube_sweep_tests), MHGP8_SUM(mhgp8::Work, tube_credited_sites),
  MHGP8_SUM(mhgp8::Work, tube_separation_fallbacks)};
const std::array axis_fields{
  MHGP8_SUM(mhgp8::AxisQ2Work, sort_passes), MHGP8_SUM(mhgp8::AxisQ2Work, sorted_sites),
  MHGP8_SUM(mhgp8::AxisQ2Work, sort_comparisons), MHGP8_SUM(mhgp8::AxisQ2Work, columns),
  MHGP8_SUM(mhgp8::AxisQ2Work, constrained_anchors), MHGP8_SUM(mhgp8::AxisQ2Work, slab_bound_updates),
  MHGP8_SUM(mhgp8::AxisQ2Work, tree_point_visits), MHGP8_SUM(mhgp8::AxisQ2Work, tree_nodes),
  MHGP8_SUM(mhgp8::AxisQ2Work, query_nodes), MHGP8_SUM(mhgp8::AxisQ2Work, contained_nodes),
  MHGP8_SUM(mhgp8::AxisQ2Work, disjoint_nodes), MHGP8_SUM(mhgp8::AxisQ2Work, whole_factor_accepts),
  MHGP8_SUM(mhgp8::AxisQ2Work, whole_factor_rejects), MHGP8_SUM(mhgp8::AxisQ2Work, emitted_blocks),
  MHGP8_MAX(mhgp8::AxisQ2Work, max_tree_depth), MHGP8_SUM(mhgp8::AxisQ2Work, axis_bound_queries),
  MHGP8_SUM(mhgp8::AxisQ2Work, axis_count_queries), MHGP8_SUM(mhgp8::AxisQ2Work, axis_rank_comparisons),
  MHGP8_SUM(mhgp8::AxisQ2Work, axis_pruned_nodes), MHGP8_SUM(mhgp8::AxisQ2Work, axis_slab_rejects),
  MHGP8_SUM(mhgp8::AxisQ2Work, restriction_credit_copies),
  MHGP8_SUM(mhgp8::AxisQ2Work, restriction_credit_visits),
  MHGP8_SUM(mhgp8::AxisQ2Work, restriction_bound_queries),
  MHGP8_SUM(mhgp8::AxisQ2Work, restriction_pruned_nodes), MHGP8_SUM(mhgp8::AxisQ2Work, coalesced_blocks)};
const std::array census_fields{
  MHGP8_SUM(mhgp8::Q2CensusWork, query_build_point_visits),
  MHGP8_SUM(mhgp8::Q2CensusWork, query_build_nodes),
  MHGP8_MAX(mhgp8::Q2CensusWork, query_build_max_depth),
  MHGP8_SUM(mhgp8::Q2CensusWork, input_descriptors), MHGP8_SUM(mhgp8::Q2CensusWork, query_cover_visits),
  MHGP8_SUM(mhgp8::Q2CensusWork, query_tasks), MHGP8_SUM(mhgp8::Q2CensusWork, query_splits),
  MHGP8_SUM(mhgp8::Q2CensusWork, witness_splits), MHGP8_SUM(mhgp8::Q2CensusWork, count_root_starts),
  MHGP8_SUM(mhgp8::Q2CensusWork, shared_splits_after_credit),
  MHGP8_SUM(mhgp8::Q2CensusWork, cursor_advances), MHGP8_SUM(mhgp8::Q2CensusWork, cursor_reuses),
  MHGP8_SUM(mhgp8::Q2CensusWork, count_node_visits), MHGP8_SUM(mhgp8::Q2CensusWork, count_bound_tests),
  MHGP8_SUM(mhgp8::Q2CensusWork, count_point_tests),
  MHGP8_SUM(mhgp8::Q2CensusWork, uniform_credited_pairs),
  MHGP8_SUM(mhgp8::Q2CensusWork, uniform_rejected_pairs),
  MHGP8_SUM(mhgp8::Q2CensusWork, uniform_accepted_pairs),
  MHGP8_SUM(mhgp8::Q2CensusWork, consumed_witness_sites),
  MHGP8_SUM(mhgp8::Q2CensusWork, frontier_restarts), MHGP8_SUM(mhgp8::Q2CensusWork, payload_node_visits),
  MHGP8_SUM(mhgp8::Q2CensusWork, payload_bound_tests), MHGP8_SUM(mhgp8::Q2CensusWork, payload_point_tests),
  MHGP8_SUM(mhgp8::Q2CensusWork, payload_interior_sites),
  MHGP8_SUM(mhgp8::Q2CensusWork, payload_shell_sites), MHGP8_SUM(mhgp8::Q2CensusWork, payload_supports)};
#undef MHGP8_SUM
#undef MHGP8_MAX

template <class T, std::size_t N>
void accumulate(T& target, const T& source, const std::array<Field<T>, N>& fields) {
  for (const auto& field : fields) {
    if (field.maximum) target.*field.member = std::max(target.*field.member, source.*field.member);
    else mhgp8::counter_add(target.*field.member, source.*field.member);
  }
}

template <class T, std::size_t N>
void require_scaled(const T& fresh, const T& shared, u64 factor,
                    const std::array<Field<T>, N>& fields) {
  for (const auto& field : fields)
    require(fresh.*field.member == product(shared.*field.member, field.maximum ? 1 : factor),
            "fresh/shared discrete work differs from its declared scaling");
}

template <class T, std::size_t N>
void print_counters(const T& value, const std::array<Field<T>, N>& fields) {
  std::cout << '{';
  bool comma = false;
  for (const auto& field : fields) {
    if (comma) std::cout << ',';
    std::cout << '"' << field.name << "\":" << value.*field.member;
    comma = true;
  }
  std::cout << '}';
}

void add_work(mhgp8::Work& target, const mhgp8::Work& source) {
  accumulate(target, source, work_fields);
  accumulate(target.predicates, source.predicates, predicate_fields);
}

void same_work(const mhgp8::Work& fresh, const mhgp8::Work& shared) {
  require_scaled(fresh, shared, 1, work_fields);
  require_scaled(fresh.predicates, shared.predicates, 1, predicate_fields);
}

void print_work(const mhgp8::Work& work) {
  std::cout << "{\"counters\":";
  print_counters(work, work_fields);
  std::cout << ",\"predicates\":";
  print_counters(work.predicates, predicate_fields);
  std::cout << '}';
}

struct Arm {
  mhgp8::CloudWork cloud_work;
  mhgp8::Q2IndexWork index_work;
  mhgp8::Work preparation_work;
  mhgp8::Work local_work;
  mhgp8::AxisQ2Work axis_work;
  mhgp8::Q2CensusWork census_work;
  OutputDigest digest;
  u64 cloud_preparations{};
  u64 index_preparations{};
  u64 rectangle_preparations{};
  u64 factor_box_visits{};
  u64 factor_box_steps{};
  u64 total_pairs{};
  u64 candidate_pairs{};
  u64 candidate_descriptors{};
  u64 accepted_pairs{};
  u64 rejected_pairs{};
  u64 max_cloud_retained_bytes{};
  u64 max_index_retained_bytes{};
  double cloud_ms{};
  double index_ms{};
  double rectangle_ms{};
  double local_plan_ms{};
  double axis_selection_ms{};
  double consumption_ms{};
  double census_query_index_ms{};
  double census_count_ms{};
  double census_payload_ms{};
  double census_total_ms{};
  double destruction_ms{};
  double processing_ms{};
};

Arm measure(const mhgp8::RectangleInput& input, const Options& o, bool shared) {
  Arm arm;
  const auto started = Clock::now();
  mhgp8::CloudPtr cloud;
  mhgp8::Q2CensusIndexPtr index;
  const auto prepare_global = [&] {
    const auto copying = Clock::now();
    cloud = mhgp8::prepare_cloud(input.points);
    const auto copied = Clock::now();
    index = mhgp8::make_q2_cloud_index(cloud);
    const auto indexed = Clock::now();
    arm.cloud_ms += milliseconds(copying, copied);
    arm.index_ms += milliseconds(copied, indexed);
    mhgp8::counter_add(arm.cloud_preparations);
    mhgp8::counter_add(arm.index_preparations);
    accumulate(arm.cloud_work, cloud->work(), cloud_fields);
    accumulate(arm.index_work, index->work(), index_fields);
    arm.max_cloud_retained_bytes = std::max(arm.max_cloud_retained_bytes,
                                            size_counter(cloud->retained_bytes()));
    arm.max_index_retained_bytes = std::max(arm.max_index_retained_bytes,
                                            size_counter(index->retained_bytes()));
    require(&index->cloud() == cloud.get(), "global index lost its cloud identity");
  };
  if (shared) prepare_global();
  const auto quotient = input.a.size() / o.rectangles;
  const auto remainder = input.a.size() % o.rectangles;
  auto first = input.a.first;
  for (std::size_t rectangle_id = 0; rectangle_id < o.rectangles; ++rectangle_id) {
    if (!shared) prepare_global();
    // Advancing an endpoint avoids potentially overflowing rectangle_id*nA.
    const auto count = quotient + static_cast<std::size_t>(rectangle_id < remainder);
    require(count > 0 && count <= input.a.last - first, "invalid contiguous partition step");
    const auto last = first + count;
    const mhgp8::RectangleSpec spec{{first, last}, input.b, input.core_candidates};
    auto destruction_started = Clock::now();
    {
      const auto preparing = Clock::now();
      const auto rectangle = mhgp8::prepare_rectangle(cloud, spec, o.kmax, o.separation);
      const auto prepared = Clock::now();
      const auto local = mhgp8::make_credit_plan(rectangle, mhgp8::Lane::Q2, mhgp8::Strategy::Pool);
      const auto local_finished = Clock::now();
      const auto plan = mhgp8::make_axis_q2_plan(rectangle, mhgp8::AxisQ2Mode::Additive, &local);
      const auto filtered = Clock::now();
      const auto result = mhgp8::run_q2_census(*index, plan, mhgp8::Q2CensusMode::Pairwise,
          [&](const mhgp8::Q2Support& support) {
            require(support.a_id >= first && support.a_id < last &&
                    support.b_id >= input.b.first && support.b_id < input.b.last,
                    "emitted support escaped its partition rectangle");
            arm.digest.consume(support, input.points.size(), o.kmax);
          });
      const auto consumed = Clock::now();
      require(&rectangle->cloud() == cloud.get() && &plan.rectangle() == rectangle.get() &&
              &local.rectangle() == rectangle.get(), "local context identity mismatch");
      require(result.candidate_pairs == plan.candidate_pairs() &&
              result.accepted_pairs <= result.candidate_pairs &&
              result.rejected_pairs == result.candidate_pairs - result.accepted_pairs,
              "local candidate accounting mismatch");
      arm.rectangle_ms += milliseconds(preparing, prepared);
      arm.local_plan_ms += milliseconds(prepared, local_finished);
      arm.axis_selection_ms += milliseconds(local_finished, filtered);
      arm.consumption_ms += milliseconds(filtered, consumed);
      arm.census_query_index_ms += result.query_index_ms;
      arm.census_count_ms += result.count_ms;
      arm.census_payload_ms += result.payload_ms;
      arm.census_total_ms += result.total_ms;
      mhgp8::counter_add(arm.rectangle_preparations);
      mhgp8::counter_add(arm.factor_box_visits, rectangle->factor_box_visits());
      mhgp8::counter_add(arm.factor_box_steps, rectangle->factor_box_steps());
      mhgp8::counter_add(arm.total_pairs, plan.total_pairs());
      mhgp8::counter_add(arm.candidate_pairs, result.candidate_pairs);
      mhgp8::counter_add(arm.candidate_descriptors, size_counter(plan.blocks().size()));
      mhgp8::counter_add(arm.accepted_pairs, result.accepted_pairs);
      mhgp8::counter_add(arm.rejected_pairs, result.rejected_pairs);
      add_work(arm.preparation_work, rectangle->preparation_work());
      add_work(arm.local_work, local.work());
      accumulate(arm.axis_work, plan.work(), axis_fields);
      accumulate(arm.census_work, result.work, census_fields);
      destruction_started = Clock::now();
    }  // Release the only active rectangle, its Pool and axial plan.
    if (!shared) {
      index.reset();
      cloud.reset();
    }
    arm.destruction_ms += milliseconds(destruction_started, Clock::now());
    first = last;
  }
  require(first == input.a.last, "rectangle partition did not cover all of A");
  const auto destruction_started = Clock::now();
  index.reset();
  cloud.reset();
  arm.destruction_ms += milliseconds(destruction_started, Clock::now());
  arm.processing_ms = milliseconds(started, Clock::now());
  return arm;
}

void validate(const Arm& fresh, const Arm& shared, const mhgp8::RectangleInput& input,
              const Options& o) {
  const auto rectangles = size_counter(o.rectangles);
  const auto expected_pairs = product(size_counter(input.a.size()), size_counter(input.b.size()));
  require(fresh.cloud_preparations == rectangles && fresh.index_preparations == rectangles &&
          shared.cloud_preparations == 1 && shared.index_preparations == 1,
          "wrong global preparation multiplicity");
  for (const auto* arm : {&fresh, &shared}) {
    require(arm->rectangle_preparations == rectangles && arm->total_pairs == expected_pairs &&
            arm->candidate_pairs <= expected_pairs && arm->accepted_pairs <= arm->candidate_pairs &&
            arm->rejected_pairs == arm->candidate_pairs - arm->accepted_pairs,
            "partition, residual or rectangle count mismatch");
    require(arm->digest.supports == arm->accepted_pairs &&
            arm->digest.supports == arm->census_work.payload_supports &&
            arm->digest.interior_ids == arm->census_work.payload_interior_sites &&
            arm->digest.shell_ids == arm->census_work.payload_shell_sites,
            "collected payload accounting mismatch");
    require(arm->preparation_work.validation_points == 0 &&
            arm->preparation_work.uniqueness_comparisons == 0,
            "shared rectangle adapter repeated global validation");
  }
  require(fresh.digest == shared.digest && fresh.total_pairs == shared.total_pairs &&
          fresh.candidate_pairs == shared.candidate_pairs &&
          fresh.candidate_descriptors == shared.candidate_descriptors &&
          fresh.accepted_pairs == shared.accepted_pairs && fresh.rejected_pairs == shared.rejected_pairs &&
          fresh.factor_box_visits == shared.factor_box_visits &&
          fresh.factor_box_steps == shared.factor_box_steps &&
          fresh.max_cloud_retained_bytes == shared.max_cloud_retained_bytes &&
          fresh.max_index_retained_bytes == shared.max_index_retained_bytes,
          "fresh/shared output, local work or cloud capacity mismatch");
  require_scaled(fresh.cloud_work, shared.cloud_work, rectangles, cloud_fields);
  require_scaled(fresh.index_work, shared.index_work, rectangles, index_fields);
  same_work(fresh.preparation_work, shared.preparation_work);
  same_work(fresh.local_work, shared.local_work);
  require_scaled(fresh.axis_work, shared.axis_work, 1, axis_fields);
  require_scaled(fresh.census_work, shared.census_work, 1, census_fields);
}

void print_arm(const Arm& arm, const char* mode, double common_ms) {
  std::cout << "{\"mode\":\"" << mode << "\",\"processing_ms\":" << arm.processing_ms
            << ",\"total_ms\":" << arm.processing_ms + common_ms
            << ",\"cloud_ms\":" << arm.cloud_ms << ",\"index_ms\":" << arm.index_ms
            << ",\"rectangle_ms\":" << arm.rectangle_ms << ",\"local_plan_ms\":" << arm.local_plan_ms
            << ",\"axis_selection_ms\":" << arm.axis_selection_ms
            << ",\"consumption_ms\":" << arm.consumption_ms
            << ",\"census_query_index_ms\":" << arm.census_query_index_ms
            << ",\"census_count_ms\":" << arm.census_count_ms
            << ",\"census_payload_ms\":" << arm.census_payload_ms
            << ",\"census_total_ms\":" << arm.census_total_ms
            << ",\"destruction_ms\":" << arm.destruction_ms
            << ",\"cloud_preparations\":" << arm.cloud_preparations
            << ",\"index_preparations\":" << arm.index_preparations
            << ",\"rectangle_preparations\":" << arm.rectangle_preparations
            << ",\"factor_box_visits\":" << arm.factor_box_visits
            << ",\"factor_box_steps\":" << arm.factor_box_steps
            << ",\"total_pairs\":" << arm.total_pairs
            << ",\"candidate_pairs\":" << arm.candidate_pairs
            << ",\"candidate_descriptors\":" << arm.candidate_descriptors
            << ",\"accepted_pairs\":" << arm.accepted_pairs
            << ",\"rejected_pairs\":" << arm.rejected_pairs
            << ",\"max_cloud_retained_bytes\":" << arm.max_cloud_retained_bytes
            << ",\"max_index_retained_bytes\":" << arm.max_index_retained_bytes
            << ",\"cloud_work\":";
  print_counters(arm.cloud_work, cloud_fields);
  std::cout << ",\"index_work\":";
  print_counters(arm.index_work, index_fields);
  std::cout << ",\"preparation_work\":";
  print_work(arm.preparation_work);
  std::cout << ",\"local_work\":";
  print_work(arm.local_work);
  std::cout << ",\"axis_work\":";
  print_counters(arm.axis_work, axis_fields);
  std::cout << ",\"census_work\":";
  print_counters(arm.census_work, census_fields);
  const auto& digest = arm.digest;
  std::cout << ",\"digest\":{\"supports\":" << digest.supports
            << ",\"interior_ids\":" << digest.interior_ids << ",\"shell_ids\":" << digest.shell_ids
            << ",\"sum\":\"" << std::hex << digest.sum << "\",\"xor\":\""
            << digest.xor_value << std::dec << "\"}}";
}

int run(const Options& o) {
  const auto started = Clock::now();
  std::optional<mhgp8::RectangleInput> input(o.family == "sheet_full"
      ? mhgp8::bench::make_sheet_full_fixture(o.n) : mhgp8::bench::make_fixture(o.n, o.family));
  require(o.rectangles <= input->a.size(), "rectangle count exceeds nonempty A partitions");
  const auto input_hash = mhgp8::bench::fixture_hash(*input);
  const auto n_a = input->a.size();
  const auto n_b = input->b.size();
  const auto generated = Clock::now();
  Arm fresh;
  Arm shared;
  if (o.order == "fresh-first") {
    fresh = measure(*input, o, false);
    shared = measure(*input, o, true);
  } else {
    shared = measure(*input, o, true);
    fresh = measure(*input, o, false);
  }
  const auto comparison_started = Clock::now();
  validate(fresh, shared, *input, o);
  const auto compared = Clock::now();
  input.reset();
  const auto finished = Clock::now();
  const auto generation_ms = milliseconds(started, generated);
  const auto input_destruction_ms = milliseconds(compared, finished);
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17)
            << "{\"schema\":\"mhgp8_cloud_reuse_probe_v1\",\"status\":\"completed\""
            << ",\"scope\":\"single_rectangle_partition_cloud_reuse_not_wspd\""
            << ",\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\""
            << ",\"public_status\":\"not_claimed\",\"threads\":1,\"gcp_used\":false,\"seed\":null"
            << ",\"n\":" << o.n << ",\"n_a\":" << n_a << ",\"n_b\":" << n_b
            << ",\"family\":\"" << o.family << "\",\"kmax\":" << o.kmax
            << ",\"separation_s\":" << o.separation << ",\"rectangles\":" << o.rectangles
            << ",\"order\":\"" << o.order << "\",\"fixture_version\":"
            << (o.family == "sheet_full" ? 2 : 1)
            << ",\"input_fnv1a64_le_u16_xyz\":\"" << std::hex << input_hash << std::dec
            << "\",\"partition_recipe\":\"balanced_contiguous_a_original_order_v1\""
            << ",\"s_role\":\"rectangle_precondition_not_wspd_generation\""
            << ",\"prefilter\":\"intersection_pool\",\"census_mode\":\"pairwise\""
            << ",\"payload_materialized\":true,\"output_digests_match\":true"
            << ",\"local_work_match\":true,\"global_work_scaling_match\":true"
            << ",\"output_sink\":\"streaming_digest\",\"canonical_balls_deduplicated\":false"
            << ",\"full_pipeline_measured\":false,\"digest_kind\":\"sum_xor_fnv1a64_q2_payload_v1\""
            << ",\"work_aggregation\":\"sum_except_depth_maxima\""
            << ",\"cloud_memory_kind\":\"maximum_live_cloud_vector_capacities_only\""
            << ",\"index_memory_kind\":\"maximum_live_index_vector_capacities_cloud_excluded\""
            << ",\"contexts_retained\":1"
            << ",\"totals_kind\":\"common_generation_and_input_destruction_plus_enclosing_arm\""
            << ",\"generation_ms\":" << generation_ms
            << ",\"input_destruction_ms\":" << input_destruction_ms
            << ",\"comparison_ms\":" << milliseconds(comparison_started, compared)
            << ",\"paired_execution_ms\":" << milliseconds(started, finished)
            << ",\"arms\":[";
  print_arm(fresh, "fresh", generation_ms + input_destruction_ms);
  std::cout << ',';
  print_arm(shared, "shared", generation_ms + input_destruction_ms);
  std::cout << "]}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_cloud_reuse_probe: " << error.what() << '\n';
    return 2;
  }
}
