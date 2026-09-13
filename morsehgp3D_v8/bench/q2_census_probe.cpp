#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <locale>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "p0_fixtures.hpp"
#include "probe_emit.hpp"
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

struct Options {
  std::size_t n;
  std::string_view family;
  unsigned kmax;
  unsigned separation;
  std::string_view prefilter;
  std::string_view order;
};

Options options(int argc, char** argv) {
  if (argc != 7)
    throw std::invalid_argument("usage: mhgp8_q2_census_probe n grid|sheet|sheet_full|skew|tube|rails "
                                "kmax s independent|additive|intersection_pool pairwise-first|shared-first");
  Options result{integer<std::size_t>(argv[1]), argv[2], integer<unsigned>(argv[3]),
                  integer<unsigned>(argv[4]), argv[5], argv[6]};
  if (result.n < 2 || result.kmax < 1 || result.kmax > 10 || result.separation == 0 ||
      (result.prefilter != "independent" && result.prefilter != "additive" &&
       result.prefilter != "intersection_pool") ||
      (result.order != "pairwise-first" && result.order != "shared-first"))
    throw std::invalid_argument("invalid census size, Kmax, separation, prefilter or order");
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
    hash_word(hash, std::min(support.a_id, support.b_id));
    hash_word(hash, std::max(support.a_id, support.b_id));
    for (const auto coordinate : support.key.center_twice) hash_word(hash, coordinate);
    hash_word(hash, support.key.diameter_squared);
    bool saw_a = false;
    bool saw_b = false;
    for (const bool shell : {false, true}) {
      const auto ids = shell ? support.shell : support.interior;
      u64 id_sum = 0;
      u64 id_xor = 0;
      for (const auto id : ids) {
        require(id < n, "payload ID is outside its immutable owner");
        require(shell || (id != support.a_id && id != support.b_id),
                "support endpoint was counted as a strict interior");
        if (shell) {
          saw_a = saw_a || id == support.a_id;
          saw_b = saw_b || id == support.b_id;
        }
        u64 item = 14695981039346656037ULL;
        hash_word(item, 1);
        hash_word(item, id);
        id_sum += item;  // Deliberate modulo-2^64 checksum arithmetic.
        id_xor ^= item;
      }
      hash_word(hash, ids.size());
      hash_word(hash, id_sum);
      hash_word(hash, id_xor);
    }
    require(saw_a && saw_b, "materialized shell lost a support endpoint");
    mhgp8::counter_add(supports);
    mhgp8::counter_add(interior_ids, support.interior.size());
    mhgp8::counter_add(shell_ids, support.shell.size());
    sum += hash;
    xor_value ^= hash;
  }
};

struct Arm {
  mhgp8::Q2CensusResult result;
  OutputDigest digest;
  double consumption_ms{};
};

Arm consume(const mhgp8::Q2CensusIndex& index, const mhgp8::AxisQ2Plan& plan,
             mhgp8::Q2CensusMode mode) {
  Arm arm;
  const auto started = Clock::now();
  arm.result = mhgp8::run_q2_census(index, plan, mode, [&](const mhgp8::Q2Support& support) {
    arm.digest.consume(support, index.rectangle().points().size(), index.rectangle().kmax());
  });
  arm.consumption_ms = milliseconds(started, Clock::now());
  return arm;
}

void print_fields(std::initializer_list<std::pair<const char*, u64>> fields) {
  std::cout << '{';
  bool comma = false;
  for (const auto& [name, value] : fields) {
    if (comma) std::cout << ',';
    std::cout << '"' << name << "\":" << value;
    comma = true;
  }
  std::cout << '}';
}

#define MHGP8_Q2_FIELD(object, name) {#name, object.name}
void print_axis_work(const mhgp8::AxisQ2Work& w) {
  print_fields({MHGP8_Q2_FIELD(w, sort_passes), MHGP8_Q2_FIELD(w, sorted_sites),
    MHGP8_Q2_FIELD(w, sort_comparisons), MHGP8_Q2_FIELD(w, columns),
    MHGP8_Q2_FIELD(w, constrained_anchors), MHGP8_Q2_FIELD(w, slab_bound_updates),
    MHGP8_Q2_FIELD(w, tree_point_visits), MHGP8_Q2_FIELD(w, tree_nodes),
    MHGP8_Q2_FIELD(w, query_nodes), MHGP8_Q2_FIELD(w, contained_nodes),
    MHGP8_Q2_FIELD(w, disjoint_nodes), MHGP8_Q2_FIELD(w, whole_factor_accepts),
    MHGP8_Q2_FIELD(w, whole_factor_rejects), MHGP8_Q2_FIELD(w, emitted_blocks),
    MHGP8_Q2_FIELD(w, max_tree_depth), MHGP8_Q2_FIELD(w, axis_bound_queries),
    MHGP8_Q2_FIELD(w, axis_count_queries), MHGP8_Q2_FIELD(w, axis_rank_comparisons),
    MHGP8_Q2_FIELD(w, axis_pruned_nodes), MHGP8_Q2_FIELD(w, axis_slab_rejects),
    MHGP8_Q2_FIELD(w, restriction_credit_copies), MHGP8_Q2_FIELD(w, restriction_credit_visits),
    MHGP8_Q2_FIELD(w, restriction_bound_queries), MHGP8_Q2_FIELD(w, restriction_pruned_nodes),
    MHGP8_Q2_FIELD(w, coalesced_blocks)});
}

void print_census_work(const mhgp8::Q2CensusWork& w) {
  print_fields({MHGP8_Q2_FIELD(w, query_build_point_visits), MHGP8_Q2_FIELD(w, query_build_nodes),
    MHGP8_Q2_FIELD(w, query_build_max_depth), MHGP8_Q2_FIELD(w, input_descriptors),
    MHGP8_Q2_FIELD(w, query_cover_visits), MHGP8_Q2_FIELD(w, query_tasks),
    MHGP8_Q2_FIELD(w, query_splits), MHGP8_Q2_FIELD(w, witness_splits),
    MHGP8_Q2_FIELD(w, count_root_starts), MHGP8_Q2_FIELD(w, shared_splits_after_credit),
    MHGP8_Q2_FIELD(w, cursor_reuses), MHGP8_Q2_FIELD(w, count_node_visits),
    MHGP8_Q2_FIELD(w, count_bound_tests), MHGP8_Q2_FIELD(w, count_point_tests),
    MHGP8_Q2_FIELD(w, uniform_credited_pairs), MHGP8_Q2_FIELD(w, uniform_rejected_pairs),
    MHGP8_Q2_FIELD(w, uniform_accepted_pairs), MHGP8_Q2_FIELD(w, consumed_witness_sites),
    MHGP8_Q2_FIELD(w, cursor_advances), MHGP8_Q2_FIELD(w, frontier_restarts),
    MHGP8_Q2_FIELD(w, payload_node_visits), MHGP8_Q2_FIELD(w, payload_bound_tests),
    MHGP8_Q2_FIELD(w, payload_point_tests), MHGP8_Q2_FIELD(w, payload_interior_sites),
    MHGP8_Q2_FIELD(w, payload_shell_sites), MHGP8_Q2_FIELD(w, payload_supports)});
}
#undef MHGP8_Q2_FIELD

void print_arm(const Arm& arm, const char* mode, double common_ms) {
  const auto& r = arm.result;
  std::cout << "{\"mode\":\"" << mode << "\",\"consumption_ms\":" << arm.consumption_ms
            << ",\"query_index_ms\":" << r.query_index_ms << ",\"count_ms\":" << r.count_ms
            << ",\"payload_ms\":" << r.payload_ms << ",\"census_total_ms\":" << r.total_ms
            << ",\"total_ms\":" << common_ms + arm.consumption_ms
            << ",\"candidate_pairs\":" << r.candidate_pairs
            << ",\"accepted_pairs\":" << r.accepted_pairs
            << ",\"rejected_pairs\":" << r.rejected_pairs << ",\"digest\":";
  const auto& d = arm.digest;
  std::cout << "{\"supports\":" << d.supports << ",\"interior_ids\":" << d.interior_ids
            << ",\"shell_ids\":" << d.shell_ids << ",\"sum\":\"" << std::hex << d.sum
            << "\",\"xor\":\"" << d.xor_value << std::dec << "\"},\"work\":";
  print_census_work(r.work);
  std::cout << '}';
}

int run(const Options& o) {
  const auto started = Clock::now();
  std::optional<mhgp8::RectangleInput> input(o.family == "sheet_full"
      ? mhgp8::bench::make_sheet_full_fixture(o.n) : mhgp8::bench::make_fixture(o.n, o.family));
  const auto input_hash = mhgp8::bench::fixture_hash(*input);
  const auto generated = Clock::now();
  auto owner = mhgp8::prepare_rectangle(*input, o.kmax, o.separation);
  const auto prepared = Clock::now();
  auto index = mhgp8::make_q2_census_index(owner);
  const auto indexed = Clock::now();
  std::optional<mhgp8::CreditPlan> local;
  auto local_finished = indexed;
  if (o.prefilter == "intersection_pool") {
    local.emplace(mhgp8::make_credit_plan(owner, mhgp8::Lane::Q2, mhgp8::Strategy::Pool));
    local_finished = Clock::now();
  }
  std::optional<mhgp8::AxisQ2Plan> plan(mhgp8::make_axis_q2_plan(owner,
      o.prefilter == "independent" ? mhgp8::AxisQ2Mode::Independent : mhgp8::AxisQ2Mode::Additive,
      local ? &*local : nullptr));
  const auto filtered = Clock::now();
  Arm pairwise;
  Arm shared;
  if (o.order == "pairwise-first") {
    pairwise = consume(*index, *plan, mhgp8::Q2CensusMode::Pairwise);
    shared = consume(*index, *plan, mhgp8::Q2CensusMode::SharedBlocks);
  } else {
    shared = consume(*index, *plan, mhgp8::Q2CensusMode::SharedBlocks);
    pairwise = consume(*index, *plan, mhgp8::Q2CensusMode::Pairwise);
  }
  const auto inspection_started = Clock::now();
  require(&index->rectangle() == owner.get() && &plan->rectangle() == owner.get(),
          "index and prefilter must share one immutable owner");
  for (const auto* arm : {&pairwise, &shared}) {
    const auto& r = arm->result;
    require(r.candidate_pairs == plan->candidate_pairs() && r.accepted_pairs <= r.candidate_pairs &&
            r.rejected_pairs == r.candidate_pairs - r.accepted_pairs &&
            r.accepted_pairs == arm->digest.supports && r.work.payload_supports == arm->digest.supports &&
            r.work.payload_interior_sites == arm->digest.interior_ids &&
            r.work.payload_shell_sites == arm->digest.shell_ids,
            "census counts disagree with materialized payloads");
  }
  require(pairwise.digest == shared.digest, "pairwise/shared materialized output digests differ");
  const auto n_a = owner->a_range().size();
  const auto n_b = owner->b_range().size();
  const auto preparation_work = owner->preparation_work();
  const auto index_work = index->work();
  const auto axis_work = plan->work();
  const auto candidates = plan->candidate_pairs();
  const auto descriptors = plan->blocks().size();
  const auto local_work = local ? std::optional<mhgp8::Work>(local->work()) : std::nullopt;
  const auto inspected = Clock::now();
  plan.reset();
  local.reset();
  index.reset();
  owner.reset();
  input.reset();
  const auto finished = Clock::now();
  const double destruction_ms = milliseconds(inspected, finished);
  const double setup_ms = milliseconds(started, filtered);
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17)
            << "{\"schema\":\"mhgp8_q2_census_probe_v1\",\"status\":\"completed\""
            << ",\"scope\":\"single_rectangle_q2_materialized_census\""
            << ",\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\""
            << ",\"public_status\":\"not_claimed\",\"threads\":1,\"seed\":null"
            << ",\"fixture_version\":" << (o.family == "sheet_full" ? 2 : 1)
            << ",\"n\":" << o.n << ",\"n_a\":" << n_a << ",\"n_b\":" << n_b
            << ",\"family\":\"" << o.family << "\",\"kmax\":" << o.kmax
            << ",\"separation_s\":" << o.separation << ",\"prefilter\":\"" << o.prefilter
            << "\",\"order\":\"" << o.order << "\",\"input_fnv1a64_le_u16_xyz\":\""
            << std::hex << input_hash << std::dec
            << "\",\"s_role\":\"rectangle_precondition_not_wspd_generation\""
            << ",\"owner_preparations\":1,\"index_preparations\":1,\"prefilter_preparations\":1"
            << ",\"same_owner\":true,\"payload_materialized\":true,\"output_digests_match\":true"
            << ",\"output_sink\":\"streaming_digest\",\"canonical_balls_deduplicated\":false"
            << ",\"full_pipeline_measured\":false,\"digest_kind\":\"sum_xor_fnv1a64_q2_payload_v1\""
            << ",\"count_timing_kind\":\"residual_including_instrumentation\""
            << ",\"totals_kind\":\"common_setup_and_destruction_plus_one_arm_inspection_separate\""
            << ",\"generation_ms\":" << milliseconds(started, generated)
            << ",\"prepare_ms\":" << milliseconds(generated, prepared)
            << ",\"index_ms\":" << milliseconds(prepared, indexed)
            << ",\"local_plan_ms\":" << milliseconds(indexed, local_finished)
            << ",\"axis_selection_ms\":" << milliseconds(local_finished, filtered)
            << ",\"prefilter_ms\":" << milliseconds(indexed, filtered)
            << ",\"setup_ms\":" << setup_ms << ",\"destruction_ms\":" << destruction_ms
            << ",\"inspection_ms\":" << milliseconds(inspection_started, inspected)
            << ",\"paired_execution_ms\":" << milliseconds(started, finished)
            << ",\"candidate_pairs\":" << candidates << ",\"candidate_descriptors\":" << descriptors
            << ",\"preparation_work\":";
  mhgp8::bench::print_work(preparation_work);
  std::cout << ",\"index_work\":";
  print_fields({{"point_visits", index_work.point_visits}, {"nodes", index_work.nodes},
                 {"max_depth", index_work.max_depth}, {"escape_links", index_work.escape_links}});
  std::cout << ",\"prefilter_work\":";
  print_axis_work(axis_work);
  std::cout << ",\"local_work\":";
  if (local_work) mhgp8::bench::print_work(*local_work); else std::cout << "null";
  std::cout << ",\"arms\":[";
  print_arm(pairwise, "pairwise", setup_ms + destruction_ms);
  std::cout << ',';
  print_arm(shared, "shared", setup_ms + destruction_ms);
  std::cout << "]}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_q2_census_probe: " << error.what() << '\n';
    return 2;
  }
}
