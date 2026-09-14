#pragma once

// Explicit port of the canonical support callback and field tables from
// bench/wspd_q2_census_probe.cpp at ba11e3ab. No previous timings or
// qualification are inherited; each worker pays its copies, sort and checks.

#include "front_fixtures.hpp"
#include "pipeline/wspd_q2_parallel.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
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
// Worker slots never share the same 64-byte boundary for their hot counters.
// This padding is included in callback_state_bytes, not hidden from storage.
struct alignas(64) OutputDigest {
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
using JointWork = decltype(mhgp8::WspdQ2CensusResult{}.joint_work);
const std::array joint_fields{
  MHGP8_FIELD(JointWork, root_products), MHGP8_FIELD(JointWork, tasks),
  MHGP8_FIELD(JointWork, splits_a), MHGP8_FIELD(JointWork, splits_b),
  MHGP8_FIELD(JointWork, witness_splits), MHGP8_FIELD(JointWork, bound_tests),
  MHGP8_FIELD(JointWork, cursor_advances), MHGP8_FIELD(JointWork, structural_splits),
  MHGP8_FIELD(JointWork, deferred_skips), MHGP8_FIELD(JointWork, phase_switches),
  MHGP8_FIELD(JointWork, consumed_witness_sites), MHGP8_FIELD(JointWork, credit_events),
  MHGP8_FIELD(JointWork, credited_pair_mass), MHGP8_FIELD(JointWork, splits_after_credit),
  MHGP8_FIELD(JointWork, singleton_handoffs), MHGP8_FIELD(JointWork, handoffs_after_credit),
  MHGP8_FIELD(JointWork, handoff_pair_mass), MHGP8_FIELD(JointWork, rejected_pairs),
  MHGP8_FIELD(JointWork, accepted_pairs), MHGP8_FIELD(JointWork, max_depth)};
using PoolWork = decltype(mhgp8::WspdQ2CensusResult{}.pool_work);
const std::array pool_fields{
  MHGP8_FIELD(PoolWork, selected_rectangles), MHGP8_FIELD(PoolWork, selected_pairs),
  MHGP8_FIELD(PoolWork, residual_pairs), MHGP8_FIELD(PoolWork, filtered_pairs),
  MHGP8_FIELD(PoolWork, factor_sites), MHGP8_FIELD(PoolWork, selection_tests),
  MHGP8_FIELD(PoolWork, witness_attempts), MHGP8_FIELD(PoolWork, universal_queries),
  MHGP8_FIELD(PoolWork, q2_axis_terms), MHGP8_FIELD(PoolWork, pool_selected),
  MHGP8_FIELD(PoolWork, pool_insertions), MHGP8_FIELD(PoolWork, pool_shifted_entries),
  MHGP8_FIELD(PoolWork, prefix_class_visits), MHGP8_FIELD(PoolWork, factor_read_visits),
  MHGP8_FIELD(PoolWork, grouping_visits), MHGP8_FIELD(PoolWork, bands),
  MHGP8_FIELD(PoolWork, selected_anchors), MHGP8_FIELD(PoolWork, pair_roots),
  MHGP8_FIELD(PoolWork, plan_peak_bytes), MHGP8_FIELD(PoolWork, original_selected_anchors),
  MHGP8_FIELD(PoolWork, passthrough_rectangles), MHGP8_FIELD(PoolWork, passthrough_pairs),
  MHGP8_FIELD(PoolWork, passthrough_anchors)};
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


}  // namespace
