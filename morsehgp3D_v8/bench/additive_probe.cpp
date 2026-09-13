#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <locale>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "p0_fixtures.hpp"
#include "probe_emit.hpp"
#include "sheet_full_fixture.hpp"
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
  std::string_view variant;
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
  if (argc != 8) {
    throw std::invalid_argument(
        "usage: mhgp8_additive_probe n pool|dual|tubes grid|sheet|skew|tube|rails|sheet_full "
        "kmax s independent-first|variant-first additive|intersection");
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
  options.variant = argv[7];
  if (options.n < 2 || options.kmax < 1 || options.kmax > 10 ||
      options.separation_s == 0 ||
      (options.order != "independent-first" && options.order != "variant-first") ||
      (options.variant != "additive" && options.variant != "intersection")) {
    throw std::invalid_argument(
        "require n>=2, 1<=kmax<=10, s>0, a declared order and additive|intersection");
  }
  return options;
}

mhgp8::RectangleInput make_fixture(const Options& options) {
  if (options.family != "sheet_full") {
    return mhgp8::bench::make_fixture(options.n, options.family);
  }
  return mhgp8::bench::make_sheet_full_fixture(options.n);
}

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

class PlanHash {
 public:
  void word(std::uint64_t value) {
    for (unsigned byte = 0; byte < 8; ++byte) {
      hash_ ^= value & 255U;
      hash_ *= 1099511628211ULL;
      value >>= 8U;
    }
  }
  template <class T>
  void values(std::span<const T> entries) {
    word(entries.size());
    for (const auto entry : entries) {
      word(entry);
    }
  }
  [[nodiscard]] std::uint64_t value() const noexcept { return hash_; }

 private:
  std::uint64_t hash_{14695981039346656037ULL};
};

std::uint64_t inspect_axis(const mhgp8::AxisQ2Plan& plan,
                           const mhgp8::PreparedRectangle* owner) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  require(&plan.rectangle() == owner, "axis plan lost its certified input owner");
  const auto a = owner->a_range();
  const auto b = owner->b_range();
  require(plan.b_order().size() == b.size(), "axis B permutation has wrong cardinality");
  std::vector<bool> seen(b.size(), false);
  // Exact axis_plan_v1 spelling: mode is deliberately not part of an object
  // checksum. Different representations can still describe the same pairs.
  PlanHash hash;
  hash.word(1);
  hash.word(plan.need());
  hash.word(plan.total_pairs());
  hash.word(plan.candidate_pairs());
  for (const auto range : {a, b}) {
    hash.word(range.first);
    hash.word(range.last);
  }
  hash.word(plan.b_order().size());
  for (const auto id : plan.b_order()) {
    require(id >= b.first && id < b.last, "axis B permutation contains a foreign ID");
    require(!seen[id - b.first], "axis B permutation repeats an ID");
    seen[id - b.first] = true;
    hash.word(id);
  }
  hash.word(plan.blocks().size());
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
    hash.word(block.a_id);
    hash.word(block.b.first);
    hash.word(block.b.last);
  }
  require(count == plan.candidate_pairs() && count <= plan.total_pairs(),
          "axis descriptor cardinality disagrees with reported residual");
  return hash.value();
}

std::uint64_t local_hash(const mhgp8::CreditPlan& plan,
                         const mhgp8::PreparedRectangle* owner) {
  require(&plan.rectangle() == owner && plan.lane() == mhgp8::Lane::Q2,
          "local restriction lost its owner or q2 lane");
  // The same full credit_plan_v1 checksum used by batch_probe.
  PlanHash hash;
  hash.word(1);
  hash.word(mhgp8::arity(plan.lane()));
  hash.word(static_cast<std::uint64_t>(plan.strategy()));
  hash.word(plan.threshold());
  hash.word(plan.core_credit());
  hash.word(plan.total_pairs());
  hash.word(plan.candidate_pairs());
  for (const auto range : {owner->a_range(), owner->b_range()}) {
    hash.word(range.first);
    hash.word(range.last);
  }
  hash.values(plan.a_credits());
  hash.values(plan.b_credits());
  hash.values(plan.a_order());
  hash.values(plan.b_order());
  hash.word(plan.blocks().size());
  std::uint64_t count = 0;
  for (const auto& block : plan.blocks()) {
    require(block.a.first < block.a.last && block.a.last <= plan.a_order().size() &&
            block.b.first < block.b.last && block.b.last <= plan.b_order().size(),
            "local descriptor has an invalid grouped range");
    mhgp8::counter_add(count, static_cast<std::uint64_t>(block.a.size()) * block.b.size());
    hash.word(block.a.first);
    hash.word(block.a.last);
    hash.word(block.b.first);
    hash.word(block.b.last);
  }
  require(count == plan.candidate_pairs(), "local descriptor cardinality mismatch");
  return hash.value();
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
            << ",\"max_tree_depth\":" << work.max_tree_depth
            << ",\"axis_bound_queries\":" << work.axis_bound_queries
            << ",\"axis_count_queries\":" << work.axis_count_queries
            << ",\"axis_rank_comparisons\":" << work.axis_rank_comparisons
            << ",\"axis_pruned_nodes\":" << work.axis_pruned_nodes
            << ",\"axis_slab_rejects\":" << work.axis_slab_rejects
            << ",\"restriction_credit_copies\":" << work.restriction_credit_copies
            << ",\"restriction_credit_visits\":" << work.restriction_credit_visits
            << ",\"restriction_bound_queries\":" << work.restriction_bound_queries
            << ",\"restriction_pruned_nodes\":" << work.restriction_pruned_nodes
            << ",\"coalesced_blocks\":" << work.coalesced_blocks << '}';
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
  std::optional<mhgp8::AxisQ2Plan> independent;
  std::optional<mhgp8::AxisQ2Plan> variant;
  std::optional<mhgp8::CreditPlan> local;
  double independent_ms = 0;
  double local_plan_ms = 0;
  double selection_ms = 0;
  double variant_ms = 0;
  const auto run_independent = [&] {
    const auto begin = Clock::now();
    independent.emplace(mhgp8::make_axis_q2_plan(owner, mhgp8::AxisQ2Mode::Independent));
    independent_ms = elapsed_ms(begin, Clock::now());
  };
  const auto run_variant = [&] {
    const auto begin = Clock::now();
    auto local_finished = begin;
    if (options.variant == "intersection") {
      local.emplace(mhgp8::make_credit_plan(owner, mhgp8::Lane::Q2, options.strategy));
      local_finished = Clock::now();
    }
    variant.emplace(mhgp8::make_axis_q2_plan(
        owner, mhgp8::AxisQ2Mode::Additive, local ? &*local : nullptr));
    const auto finished = Clock::now();
    local_plan_ms = elapsed_ms(begin, local_finished);
    selection_ms = elapsed_ms(local_finished, finished);
    variant_ms = elapsed_ms(begin, finished);
  };
  if (options.order == "independent-first") {
    run_independent();
    run_variant();
  } else {
    run_variant();
    run_independent();
  }
  const auto inspection_started = Clock::now();
  require(independent->mode() == mhgp8::AxisQ2Mode::Independent &&
          !independent->has_restriction() && variant->mode() == mhgp8::AxisQ2Mode::Additive &&
          variant->has_restriction() == local.has_value(), "wrong plan mode or restriction");
  require(independent->need() == variant->need() &&
          independent->need() == options.kmax &&
          independent->total_pairs() == variant->total_pairs() &&
          variant->candidate_pairs() <= independent->candidate_pairs(),
          "additive mode changed the threshold or increased the residual cardinality");
  const auto independent_hash = inspect_axis(*independent, owner.get());
  const auto variant_hash = inspect_axis(*variant, owner.get());
  std::optional<std::uint64_t> restriction_hash;
  if (local) {
    require(local->total_pairs() == variant->total_pairs() &&
            variant->candidate_pairs() <= local->candidate_pairs(),
            "intersection exceeds its local restriction cardinality");
    restriction_hash = local_hash(*local, owner.get());
  }
  const auto finished = Clock::now();
  const double generation_ms = elapsed_ms(started, generated);
  const double prepare_ms = elapsed_ms(generated, prepared);
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17)
            << "{\"schema\":\"mhgp8_additive_probe_v1\",\"status\":\"completed\""
            << ",\"scope\":\"single_rectangle_additive_axis_q2_residual\""
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
            << "\",\"variant\":\"" << options.variant
            << "\",\"kmax\":" << options.kmax
            << ",\"separation_s\":" << options.separation_s
            << ",\"s_role\":\"rectangle_precondition_not_wspd_generation\""
            << ",\"owner_preparations\":1,\"same_owner\":true"
            << ",\"independent_kind\":\"axis_q2_independent\""
            << ",\"candidates_expanded\":false,\"downstream_measured\":false"
            << ",\"generation_ms\":" << generation_ms
            << ",\"prepare_ms\":" << prepare_ms
            << ",\"independent_ms\":" << independent_ms
            << ",\"local_plan_ms\":" << local_plan_ms
            << ",\"selection_ms\":" << selection_ms
            << ",\"variant_ms\":" << variant_ms
            << ",\"independent_total_ms\":" << generation_ms + prepare_ms + independent_ms
            << ",\"variant_total_ms\":" << generation_ms + prepare_ms + variant_ms
            << ",\"totals_kind\":\"common_setup_plus_one_arm_not_pair_wall_time\""
            << ",\"inspection_ms\":" << elapsed_ms(inspection_started, finished)
            << ",\"paired_execution_ms\":" << elapsed_ms(started, finished)
            << ",\"need\":" << static_cast<unsigned>(independent->need())
            << ",\"total_pairs\":" << independent->total_pairs()
            << ",\"independent_candidates\":" << independent->candidate_pairs()
            << ",\"independent_descriptors\":" << independent->blocks().size()
            << ",\"variant_candidates\":" << variant->candidate_pairs()
            << ",\"variant_descriptors\":" << variant->blocks().size()
            << ",\"independent_checksum\":\"" << std::hex << independent_hash
            << "\",\"variant_checksum\":\"" << variant_hash << std::dec
            << "\",\"checksum_kind\":\"fnv1a64_le_u64_axis_plan_v1\""
            << ",\"local_checksum_kind\":\"fnv1a64_le_u64_plan_v1\""
            << ",\"local_plan_present\":" << (local ? "true" : "false")
            << ",\"local_candidates\":";
  if (local) std::cout << local->candidate_pairs(); else std::cout << "null";
  std::cout << ",\"local_descriptors\":";
  if (local) std::cout << local->blocks().size(); else std::cout << "null";
  std::cout << ",\"local_checksum\":";
  if (restriction_hash) std::cout << '"' << std::hex << *restriction_hash << std::dec << '"';
  else std::cout << "null";
  std::cout << ",\"local_work\":";
  if (local) print_work(local->work()); else std::cout << "null";
  std::cout << ",\"preparation_work\":";
  print_work(owner->preparation_work());
  std::cout << ",\"independent_work\":";
  print_axis_work(independent->work());
  std::cout << ",\"variant_work\":";
  print_axis_work(variant->work());
  std::cout << "}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(parse_options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_additive_probe: " << error.what() << '\n';
    return 2;
  }
}
