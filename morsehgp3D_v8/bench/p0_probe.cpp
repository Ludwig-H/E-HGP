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
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "pipeline/local_credits.hpp"

namespace {

using Clock = std::chrono::steady_clock;

struct Options {
  std::size_t n{};
  mhgp8::Strategy strategy{};
  mhgp8::Lane lane{};
  std::string_view strategy_name;
  std::string_view family;
  unsigned kmax{};
  unsigned separation_s{};
};

template <class Integer>
Integer parse_integer(std::string_view text, std::string_view name) {
  Integer value{};
  const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  if (text.empty() || result.ec != std::errc{} ||
      result.ptr != text.data() + text.size()) {
    throw std::invalid_argument("invalid unsigned integer for " + std::string(name));
  }
  return value;
}

Options parse_options(int argc, char** argv) {
  if (argc != 7) {
    throw std::invalid_argument(
        "usage: mhgp8_p0_probe n pool|dual|tubes 2|3|4 grid|sheet|skew|tube|rails kmax s");
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
  const auto lane = parse_integer<unsigned>(argv[3], "lane");
  if (lane < 2 || lane > 4) {
    throw std::invalid_argument("lane must be 2, 3 or 4");
  }
  options.lane = static_cast<mhgp8::Lane>(lane);
  options.family = argv[4];
  if (options.family != "grid" && options.family != "sheet" &&
      options.family != "skew" && options.family != "tube" && options.family != "rails") {
    throw std::invalid_argument("family must be grid, sheet, skew, tube or rails");
  }
  options.kmax = parse_integer<unsigned>(argv[5], "kmax");
  options.separation_s = parse_integer<unsigned>(argv[6], "s");
  if (options.n < 2 || options.kmax < 1 || options.kmax > 10 ||
      options.separation_s == 0) {
    throw std::invalid_argument("require n >= 2, 1 <= kmax <= 10 and s > 0");
  }
  return options;
}

std::size_t grid_side(std::size_t count, unsigned dimension) {
  std::size_t side = 1;
  while ((dimension == 2 ? side * side : side * side * side) < count) {
    ++side;
  }
  return side;
}

void append_factor(mhgp8::RectangleInput& input, std::size_t count,
                   std::uint16_t base_x, std::string_view family) {
  const auto side = family == "tube" ? std::size_t{1} :
      grid_side(count, family == "sheet" ? 2U : 3U);
  for (std::size_t index = 0; index < count; ++index) {
    std::size_t x = base_x;
    std::size_t y = 1000;
    std::size_t z = 1000;
    if (family == "tube") {
      x += index;
    } else if (family == "sheet") {
      y += index % side;
      z += index / side;
    } else {
      x += index % side;
      y += (index / side) % side;
      z += index / (side * side);
    }
    // Capacity checks precede this loop. Never wrap or truncate coordinates.
    if (std::max({x, y, z}) > std::numeric_limits<std::uint16_t>::max()) {
      throw std::logic_error("fixture coordinate escaped its declared u16 domain");
    }
    input.points.push_back({static_cast<std::uint16_t>(x),
                            static_cast<std::uint16_t>(y),
                            static_cast<std::uint16_t>(z)});
  }
}

mhgp8::RectangleInput make_fixture(const Options& options) {
  if (options.family == "rails") {
    if (options.n != 2718) {
      throw std::invalid_argument("rails version 1 is the fixed n=2718 auditor counter-fixture");
    }
    mhgp8::RectangleInput result;
    for (unsigned side = 0; side < 2; ++side) {
      for (unsigned rail = 0; rail < 9; ++rail) {
        for (unsigned x = 0; x <= 150; ++x) {
          result.points.push_back({static_cast<std::uint16_t>(64800 * side + x),
                                   static_cast<std::uint16_t>(600 * rail), 0});
        }
      }
    }
    result.a = {0, 1359};
    result.b = {1359, 2718};
    return result;
  }
  const auto b_count = options.family == "skew"
      ? std::max(std::size_t{1}, options.n / 16) : options.n - options.n / 2;
  const auto a_count = options.n - b_count;
  // These are domains of the named fixture recipes, not execution quotas.
  // In particular a 32k one-dimensional tube is NOT relabelled as separated.
  const std::size_t capacity = options.family == "sheet" ? 256 * 256 :
      options.family == "tube" ? 65536 - 60000 : 32 * 32 * 32;
  if (std::max(a_count, b_count) > capacity) {
    throw std::invalid_argument(
        "requested n exceeds the declared coordinate capacity of this fixture family");
  }
  mhgp8::RectangleInput input;
  input.points.reserve(options.n);
  append_factor(input, a_count, 1000, options.family);
  append_factor(input, b_count, 60000, options.family);
  input.a = {0, a_count};
  input.b = {a_count, options.n};
  // The probe measures the fallback with no externally proposed core sites.
  // prepare_rectangle still validates the full input and the actual separation.
  return input;
}

std::uint64_t fixture_hash(const mhgp8::RectangleInput& input) {
  // FNV-1a over x,y,z as little-endian u16, in original point order.
  // Deliberate unsigned wrap defines the reproducible 64-bit hash.
  std::uint64_t hash = 14695981039346656037ULL;
  for (const auto& point : input.points) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto coordinate = point[axis];
      hash ^= coordinate & 255U;
      hash *= 1099511628211ULL;
      hash ^= coordinate >> 8U;
      hash *= 1099511628211ULL;
    }
  }
  return hash;
}

double elapsed_ms(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

void print_predicate_work(const mhgp8::PredicateWork& work) {
  std::cout << "{\"point_tests\":" << work.point_tests
            << ",\"universal_queries\":" << work.universal_queries
            << ",\"q2_axis_terms\":" << work.q2_axis_terms
            << ",\"corner_tests\":" << work.corner_tests
            << ",\"block_bound_tests\":" << work.block_bound_tests
            << ",\"negative_probes\":" << work.negative_probes << '}';
}

void print_work(const mhgp8::Work& work) {
  std::cout << "{\"validation_points\":" << work.validation_points
            << ",\"uniqueness_comparisons\":" << work.uniqueness_comparisons
            << ",\"pool_selection_tests\":" << work.pool_selection_tests
            << ",\"pool_selected\":" << work.pool_selected
            << ",\"tree_nodes\":" << work.tree_nodes
            << ",\"tree_point_visits\":" << work.tree_point_visits
            << ",\"dual_tasks\":" << work.dual_tasks
            << ",\"credited_blocks\":" << work.credited_blocks
            << ",\"noncredit_blocks\":" << work.noncredit_blocks
            << ",\"leaf_pairs\":" << work.leaf_pairs
            << ",\"saturated_tasks\":" << work.saturated_tasks
            << ",\"max_tree_depth\":" << work.max_tree_depth
            << ",\"max_task_depth\":" << work.max_task_depth
            << ",\"tube_records\":" << work.tube_records
            << ",\"tube_cells\":" << work.tube_cells
            << ",\"tube_sort_comparisons\":" << work.tube_sort_comparisons
            << ",\"tube_sweep_tests\":" << work.tube_sweep_tests
            << ",\"tube_credited_sites\":" << work.tube_credited_sites
            << ",\"tube_separation_fallbacks\":" << work.tube_separation_fallbacks
            << ",\"predicates\":";
  print_predicate_work(work.predicates);
  std::cout << '}';
}

int run(const Options& options) {
  const auto started = Clock::now();
  auto input = make_fixture(options);
  const auto input_hash = fixture_hash(input);
  const auto generated = Clock::now();
  const auto rectangle = mhgp8::prepare_rectangle(
      std::move(input), options.kmax, options.separation_s);
  const auto prepared = Clock::now();
  const auto plan = mhgp8::make_credit_plan(rectangle, options.lane, options.strategy);
  const auto planned = Clock::now();
  if (plan.candidate_pairs() > plan.total_pairs() || plan.total_pairs() == 0) {
    throw std::logic_error("invalid pair counts from the credit plan");
  }
  const auto rejected_pairs = plan.total_pairs() - plan.candidate_pairs();
  const auto rejected_fraction = static_cast<double>(rejected_pairs) /
      static_cast<double>(plan.total_pairs());
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17)
            << "{\"schema\":\"mhgp8_p0_probe_v1\",\"status\":\"completed\""
            << ",\"scope\":\"single_separated_rectangle_credits\""
            << ",\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\""
            << ",\"public_status\":\"not_claimed\",\"threads\":1"
            << ",\"fixture_version\":1,\"seed\":null,\"family\":\"" << options.family
            << "\",\"input_fnv1a64_le_u16_xyz\":\"" << std::hex << input_hash << std::dec
            << "\",\"n\":" << options.n
            << ",\"n_a\":" << rectangle->a_range().size()
            << ",\"n_b\":" << rectangle->b_range().size()
            << ",\"strategy\":\"" << options.strategy_name
            << "\",\"lane\":" << mhgp8::arity(options.lane)
            << ",\"kmax\":" << options.kmax
            << ",\"separation_s\":" << options.separation_s
            << ",\"s_role\":\"rectangle_precondition_not_wspd_generation\""
            << ",\"threshold\":" << static_cast<unsigned>(plan.threshold())
            << ",\"core_credit\":" << static_cast<unsigned>(plan.core_credit())
            << ",\"generation_ms\":" << elapsed_ms(started, generated)
            << ",\"prepare_ms\":" << elapsed_ms(generated, prepared)
            << ",\"plan_ms\":" << elapsed_ms(prepared, planned)
            << ",\"total_component_ms\":" << elapsed_ms(started, planned)
            << ",\"total_pairs\":" << plan.total_pairs()
            << ",\"candidate_pairs\":" << plan.candidate_pairs()
            << ",\"rejected_pairs\":" << rejected_pairs
            << ",\"rejected_fraction\":" << rejected_fraction
            << ",\"candidate_descriptors\":" << plan.blocks().size()
            << ",\"candidates_expanded\":false,\"downstream_measured\":false"
            << ",\"preparation_work\":";
  print_work(rectangle->preparation_work());
  std::cout << ",\"plan_work\":";
  print_work(plan.work());
  std::cout << "}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(parse_options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_p0_probe: " << error.what() << '\n';
    return 2;
  }
}
