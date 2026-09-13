#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "p0_fixtures.hpp"
#include "probe_emit.hpp"
#include "pipeline/axis_q2.hpp"

namespace {

using Clock = std::chrono::steady_clock;
using mhgp8::bench::print_work;

struct Options {
  std::size_t n{};
  mhgp8::Strategy strategy{};
  std::string_view strategy_name;
  std::string_view family;
  unsigned kmax{};
  unsigned separation_s{};
  std::string_view order;
};

template <class Integer>
Integer parse_integer(std::string_view text, std::string_view name) {
  Integer value{};
  const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  if (text.empty() || result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
    throw std::invalid_argument("invalid unsigned integer for " + std::string(name));
  }
  return value;
}

Options parse_options(int argc, char** argv) {
  if (argc != 7) {
    throw std::invalid_argument(
        "usage: mhgp8_axis_probe n pool|dual|tubes grid|sheet|skew|tube|rails|sheet_full "
        "kmax s baseline-first|axis-first");
  }
  Options options;
  options.n = parse_integer<std::size_t>(argv[1], "n");
  options.strategy_name = argv[2];
  if (options.strategy_name == "pool") {
    options.strategy = mhgp8::Strategy::Pool;
  } else if (options.strategy_name == "dual") {
    options.strategy = mhgp8::Strategy::DualBlocks;
  } else if (options.strategy_name == "tubes") {
    options.strategy = mhgp8::Strategy::Tubes;
  } else {
    throw std::invalid_argument("strategy must be pool, dual or tubes");
  }
  options.family = argv[3];
  options.kmax = parse_integer<unsigned>(argv[4], "kmax");
  options.separation_s = parse_integer<unsigned>(argv[5], "s");
  options.order = argv[6];
  if (options.n < 2 || options.kmax < 1 || options.kmax > 10 ||
      options.separation_s == 0 ||
      (options.order != "baseline-first" && options.order != "axis-first")) {
    throw std::invalid_argument("require n>=2, 1<=kmax<=10, s>0 and baseline-first|axis-first");
  }
  return options;
}

mhgp8::RectangleInput make_fixture(const Options& options) {
  if (options.family != "sheet_full") {
    return mhgp8::bench::make_fixture(options.n, options.family);
  }
  if (options.n % 2 != 0) {
    throw std::invalid_argument("sheet_full version 2 requires an even n");
  }
  const std::size_t count = options.n / 2;
  constexpr std::size_t coordinate_capacity = 65536 - 1000;
  if (count > coordinate_capacity * coordinate_capacity) {
    throw std::invalid_argument("sheet_full cannot fit the required u16 coordinates");
  }
  // Exact integer square root and largest divisor at or below it. No float,
  // dropped last row, artificial sample cap or altered point count.
  std::size_t low = 1;
  std::size_t high = std::min(count, coordinate_capacity);
  while (low < high) {
    const std::size_t middle = low + (high - low + 1) / 2;
    if (middle <= count / middle) {
      low = middle;
    } else {
      high = middle - 1;
    }
  }
  std::size_t width = low;
  while (count % width != 0) {
    --width;
  }
  const std::size_t height = count / width;
  if (height > coordinate_capacity) {
    throw std::invalid_argument("sheet_full full rectangle exceeds its u16 coordinate domain");
  }
  mhgp8::RectangleInput input;
  input.points.reserve(options.n);
  for (const std::uint16_t x : {std::uint16_t{1000}, std::uint16_t{60000}}) {
    for (std::size_t index = 0; index < count; ++index) {
      input.points.push_back({x, static_cast<std::uint16_t>(1000 + index % width),
                              static_cast<std::uint16_t>(1000 + index / width)});
    }
  }
  input.a = {0, count};
  input.b = {count, options.n};
  return input;
}

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void hash_word(std::uint64_t& hash, std::uint64_t value) {
  for (unsigned byte = 0; byte < 8; ++byte) {
    hash ^= value & 255U;
    hash *= 1099511628211ULL;
    value >>= 8U;
  }
}

std::uint64_t inspect_plan(const mhgp8::AxisQ2Plan& plan,
                           const mhgp8::PreparedRectangle* owner) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  require(&plan.rectangle() == owner, "axis plan lost its certified input owner");
  const auto a = owner->a_range();
  const auto b = owner->b_range();
  require(plan.b_order().size() == b.size(), "axis B permutation has wrong cardinality");
  std::vector<bool> seen(b.size(), false);
  std::uint64_t hash = 14695981039346656037ULL;
  // V1 words, all little-endian u64: version, need, total, candidates,
  // A/B ranges, permutation length/IDs, block count and (a_id,first,last).
  // Exact descriptor inspection precedes publication; this is not an oracle.
  hash_word(hash, 1);
  hash_word(hash, plan.need());
  hash_word(hash, plan.total_pairs());
  hash_word(hash, plan.candidate_pairs());
  for (const auto range : {a, b}) {
    hash_word(hash, range.first);
    hash_word(hash, range.last);
  }
  hash_word(hash, plan.b_order().size());
  for (const auto id : plan.b_order()) {
    require(id >= b.first && id < b.last, "axis B permutation contains a foreign ID");
    require(!seen[id - b.first], "axis B permutation repeats an ID");
    seen[id - b.first] = true;
    hash_word(hash, id);
  }
  hash_word(hash, plan.blocks().size());
  std::uint64_t count = 0;
  std::size_t previous_a = 0;
  std::size_t previous_end = 0;
  bool first = true;
  for (const auto& block : plan.blocks()) {
    require(block.a_id >= a.first && block.a_id < a.last &&
            block.b.first < block.b.last && block.b.last <= plan.b_order().size(),
            "axis descriptor contains a foreign ID or invalid range");
    require(first || block.a_id > previous_a ||
            (block.a_id == previous_a && block.b.first >= previous_end),
            "axis descriptors overlap or lose canonical traversal order");
    previous_a = block.a_id;
    previous_end = block.b.last;
    first = false;
    mhgp8::counter_add(count, block.b.size());
    hash_word(hash, block.a_id);
    hash_word(hash, block.b.first);
    hash_word(hash, block.b.last);
  }
  require(count == plan.candidate_pairs() && count <= plan.total_pairs(),
          "axis descriptor cardinality disagrees with reported residual");
  return hash;
}

void print_axis_work(const mhgp8::AxisQ2Work& work) {
  std::cout << "{\"sort_passes\":" << work.sort_passes
            << ",\"sorted_sites\":" << work.sorted_sites
            << ",\"sort_comparisons\":" << work.sort_comparisons
            << ",\"columns\":" << work.columns
            << ",\"constrained_anchors\":" << work.constrained_anchors
            << ",\"slab_bound_updates\":" << work.slab_bound_updates
            << ",\"tree_point_visits\":" << work.tree_point_visits
            << ",\"tree_nodes\":" << work.tree_nodes
            << ",\"query_nodes\":" << work.query_nodes
            << ",\"contained_nodes\":" << work.contained_nodes
            << ",\"disjoint_nodes\":" << work.disjoint_nodes
            << ",\"whole_factor_accepts\":" << work.whole_factor_accepts
            << ",\"whole_factor_rejects\":" << work.whole_factor_rejects
            << ",\"emitted_blocks\":" << work.emitted_blocks
            << ",\"max_tree_depth\":" << work.max_tree_depth << '}';
}

double elapsed_ms(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

int run(const Options& options) {
  const auto started = Clock::now();
  const auto input = make_fixture(options);
  const auto input_hash = mhgp8::bench::fixture_hash(input);
  const auto generated = Clock::now();
  const auto owner = mhgp8::prepare_rectangle(input, options.kmax, options.separation_s);
  const auto prepared = Clock::now();
  std::optional<mhgp8::CreditPlan> baseline;
  std::optional<mhgp8::AxisQ2Plan> axis;
  double baseline_ms = 0;
  double axis_ms = 0;
  const auto run_baseline = [&] {
    const auto begin = Clock::now();
    baseline.emplace(mhgp8::make_credit_plan(owner, mhgp8::Lane::Q2, options.strategy));
    baseline_ms = elapsed_ms(begin, Clock::now());
  };
  const auto run_axis = [&] {
    const auto begin = Clock::now();
    axis.emplace(mhgp8::make_axis_q2_plan(owner));
    axis_ms = elapsed_ms(begin, Clock::now());
  };
  if (options.order == "baseline-first") {
    run_baseline();
    run_axis();
  } else {
    run_axis();
    run_baseline();
  }
  const auto inspection_started = Clock::now();
  require(&baseline->rectangle() == owner.get(), "baseline did not share the input owner");
  require(baseline->total_pairs() == axis->total_pairs(), "arms disagree on rectangle size");
  const auto axis_hash = inspect_plan(*axis, owner.get());
  const auto finished = Clock::now();
  const double generation_ms = elapsed_ms(started, generated);
  const double prepare_ms = elapsed_ms(generated, prepared);
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17)
            << "{\"schema\":\"mhgp8_axis_probe_v1\",\"status\":\"completed\""
            << ",\"scope\":\"single_rectangle_axis_q2_residual\""
            << ",\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\""
            << ",\"public_status\":\"not_claimed\",\"threads\":1"
            << ",\"fixture_version\":" << (options.family == "sheet_full" ? 2 : 1)
            << ",\"seed\":null,\"family\":\"" << options.family
            << "\",\"input_fnv1a64_le_u16_xyz\":\"" << std::hex << input_hash << std::dec
            << "\",\"n\":" << options.n
            << ",\"n_a\":" << owner->a_range().size()
            << ",\"n_b\":" << owner->b_range().size()
            << ",\"strategy\":\"" << options.strategy_name
            << "\",\"order\":\"" << options.order
            << "\",\"kmax\":" << options.kmax
            << ",\"separation_s\":" << options.separation_s
            << ",\"s_role\":\"rectangle_precondition_not_wspd_generation\""
            << ",\"owner_preparations\":1,\"same_owner\":true"
            << ",\"baseline_kind\":\"single_q2_credit_plan\""
            << ",\"candidates_expanded\":false,\"downstream_measured\":false"
            << ",\"generation_ms\":" << generation_ms
            << ",\"prepare_ms\":" << prepare_ms
            << ",\"baseline_ms\":" << baseline_ms
            << ",\"axis_ms\":" << axis_ms
            << ",\"baseline_total_ms\":" << generation_ms + prepare_ms + baseline_ms
            << ",\"axis_total_ms\":" << generation_ms + prepare_ms + axis_ms
            << ",\"totals_kind\":\"common_setup_plus_one_arm_not_pair_wall_time\""
            << ",\"inspection_ms\":" << elapsed_ms(inspection_started, finished)
            << ",\"paired_execution_ms\":" << elapsed_ms(started, finished)
            << ",\"total_pairs\":" << axis->total_pairs()
            << ",\"baseline_candidates\":" << baseline->candidate_pairs()
            << ",\"axis_candidates\":" << axis->candidate_pairs()
            << ",\"axis_descriptors\":" << axis->blocks().size()
            << ",\"axis_checksum\":\"" << std::hex << axis_hash << std::dec
            << "\",\"checksum_kind\":\"fnv1a64_le_u64_axis_plan_v1\""
            << ",\"preparation_work\":";
  print_work(owner->preparation_work());
  std::cout << ",\"baseline_work\":";
  print_work(baseline->work());
  std::cout << ",\"axis_work\":";
  print_axis_work(axis->work());
  std::cout << "}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(parse_options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_axis_probe: " << error.what() << '\n';
    return 2;
  }
}
