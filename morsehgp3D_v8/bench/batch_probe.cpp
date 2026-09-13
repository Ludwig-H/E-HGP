#include <algorithm>
#include <array>
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

#include "p0_fixtures.hpp"
#include "probe_emit.hpp"
#include "pipeline/local_credits.hpp"

namespace {

using Clock = std::chrono::steady_clock;
using mhgp8::bench::print_work;
using Plans = std::array<mhgp8::CreditPlan, 3>;
constexpr std::array lanes{mhgp8::Lane::Q2, mhgp8::Lane::Q3, mhgp8::Lane::Q4};

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
        "usage: mhgp8_batch_probe n pool|dual|tubes grid|sheet|skew|tube|rails "
        "kmax s baseline-first|batch-first");
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
      (options.order != "baseline-first" && options.order != "batch-first")) {
    throw std::invalid_argument(
        "require n>=2, 1<=kmax<=10, s>0 and baseline-first|batch-first");
  }
  return options;
}

void require(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <class T>
bool same_values(std::span<const T> left, std::span<const T> right) {
  return std::equal(left.begin(), left.end(), right.begin(), right.end());
}

bool same_blocks(std::span<const mhgp8::CandidateBlock> left,
                 std::span<const mhgp8::CandidateBlock> right) {
  return std::equal(left.begin(), left.end(), right.begin(), right.end(),
                    [](const auto& a, const auto& b) {
    return a.a.first == b.a.first && a.a.last == b.a.last &&
           a.b.first == b.b.first && a.b.last == b.b.last;
  });
}

void compare_plans(const mhgp8::CreditPlan& old_plan,
                   const mhgp8::CreditPlan& new_plan,
                   const mhgp8::PreparedRectangle* owner) {
  require(&old_plan.rectangle() == owner && &new_plan.rectangle() == owner,
          "batch and baseline must share the one certified input owner");
  require(old_plan.lane() == new_plan.lane() &&
          old_plan.strategy() == new_plan.strategy() &&
          old_plan.threshold() == new_plan.threshold() &&
          old_plan.core_credit() == new_plan.core_credit() &&
          old_plan.total_pairs() == new_plan.total_pairs() &&
          old_plan.candidate_pairs() == new_plan.candidate_pairs(),
          "batch/baseline scalar mismatch");
  require(same_values(old_plan.a_credits(), new_plan.a_credits()) &&
          same_values(old_plan.b_credits(), new_plan.b_credits()) &&
          same_values(old_plan.a_order(), new_plan.a_order()) &&
          same_values(old_plan.b_order(), new_plan.b_order()) &&
          same_blocks(old_plan.blocks(), new_plan.blocks()),
          "batch/baseline credit, identity, order or block mismatch");
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

std::uint64_t plan_hash(const mhgp8::CreditPlan& plan) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  // V1: each field is one little-endian u64; vector lengths precede entries.
  // This checksum is a receipt aid AFTER exact comparison, never its judge.
  PlanHash hash;
  hash.word(1);
  hash.word(mhgp8::arity(plan.lane()));
  hash.word(static_cast<std::uint64_t>(plan.strategy()));
  hash.word(plan.threshold());
  hash.word(plan.core_credit());
  hash.word(plan.total_pairs());
  hash.word(plan.candidate_pairs());
  for (const auto range : {plan.rectangle().a_range(), plan.rectangle().b_range()}) {
    hash.word(range.first);
    hash.word(range.last);
  }
  hash.values(plan.a_credits());
  hash.values(plan.b_credits());
  hash.values(plan.a_order());
  hash.values(plan.b_order());
  hash.word(plan.blocks().size());
  for (const auto& block : plan.blocks()) {
    hash.word(block.a.first);
    hash.word(block.a.last);
    hash.word(block.b.first);
    hash.word(block.b.last);
  }
  return hash.value();
}

double elapsed_ms(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

int run(const Options& options) {
  const auto started = Clock::now();
  const auto input = mhgp8::bench::make_fixture(options.n, options.family);
  const auto input_hash = mhgp8::bench::fixture_hash(input);
  const auto generated = Clock::now();
  const auto owner = mhgp8::prepare_rectangle(input, options.kmax, options.separation_s);
  const auto prepared = Clock::now();
  std::optional<Plans> baseline;
  std::optional<mhgp8::CreditBatch> batch;
  double baseline_ms = 0;
  double batch_ms = 0;
  const auto run_baseline = [&] {
    const auto begin = Clock::now();
    baseline.emplace(Plans{mhgp8::make_credit_plan(owner, lanes[0], options.strategy),
                           mhgp8::make_credit_plan(owner, lanes[1], options.strategy),
                           mhgp8::make_credit_plan(owner, lanes[2], options.strategy)});
    baseline_ms = elapsed_ms(begin, Clock::now());
  };
  const auto run_batch = [&] {
    const auto begin = Clock::now();
    batch.emplace(mhgp8::make_credit_batch(owner, options.strategy));
    batch_ms = elapsed_ms(begin, Clock::now());
  };
  if (options.order == "baseline-first") {
    run_baseline();
    run_batch();
  } else {
    run_batch();
    run_baseline();
  }
  const auto comparison_started = Clock::now();
  std::array<std::uint64_t, 3> old_hashes{};
  std::array<std::uint64_t, 3> new_hashes{};
  for (std::size_t lane = 0; lane < lanes.size(); ++lane) {
    const auto& old_plan = (*baseline)[lane];
    const auto& new_plan = batch->plan(lanes[lane]);
    compare_plans(old_plan, new_plan, owner.get());
    old_hashes[lane] = plan_hash(old_plan);
    new_hashes[lane] = plan_hash(new_plan);
    require(old_hashes[lane] == new_hashes[lane], "checksum implementation mismatch");
  }
  const auto finished = Clock::now();
  const double generation_ms = elapsed_ms(started, generated);
  const double prepare_ms = elapsed_ms(generated, prepared);
  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17)
            << "{\"schema\":\"mhgp8_batch_probe_v1\",\"status\":\"completed\""
            << ",\"scope\":\"single_rectangle_three_lane_credit_batch\""
            << ",\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\""
            << ",\"public_status\":\"not_claimed\",\"threads\":1"
            << ",\"fixture_version\":1,\"seed\":null,\"family\":\"" << options.family
            << "\",\"input_fnv1a64_le_u16_xyz\":\"" << std::hex << input_hash << std::dec
            << "\",\"n\":" << options.n
            << ",\"n_a\":" << owner->a_range().size()
            << ",\"n_b\":" << owner->b_range().size()
            << ",\"strategy\":\"" << options.strategy_name
            << "\",\"order\":\"" << options.order
            << "\",\"kmax\":" << options.kmax
            << ",\"separation_s\":" << options.separation_s
            << ",\"s_role\":\"rectangle_precondition_not_wspd_generation\""
            << ",\"owner_preparations\":1,\"same_owner\":true,\"plans_identical\":true"
            << ",\"candidates_expanded\":false,\"downstream_measured\":false"
            << ",\"generation_ms\":" << generation_ms
            << ",\"prepare_ms\":" << prepare_ms
            << ",\"baseline_shared_owner_ms\":" << baseline_ms
            << ",\"batch_ms\":" << batch_ms
            << ",\"baseline_total_ms\":" << generation_ms + prepare_ms + baseline_ms
            << ",\"batch_total_ms\":" << generation_ms + prepare_ms + batch_ms
            << ",\"totals_kind\":\"common_setup_plus_one_arm_not_pair_wall_time\""
            << ",\"comparison_ms\":" << elapsed_ms(comparison_started, finished)
            << ",\"paired_execution_ms\":" << elapsed_ms(started, finished)
            << ",\"checksum_kind\":\"fnv1a64_le_u64_plan_v1\",\"preparation_work\":";
  print_work(owner->preparation_work());
  std::cout << ",\"shared_work\":";
  print_work(batch->shared_work());
  std::cout << ",\"lanes\":[";
  for (std::size_t lane = 0; lane < lanes.size(); ++lane) {
    const auto& plan = batch->plan(lanes[lane]);
    if (lane != 0) {
      std::cout << ',';
    }
    std::cout << "{\"lane\":" << mhgp8::arity(lanes[lane])
              << ",\"threshold\":" << static_cast<unsigned>(plan.threshold())
              << ",\"core_credit\":" << static_cast<unsigned>(plan.core_credit())
              << ",\"total_pairs\":" << plan.total_pairs()
              << ",\"candidate_pairs\":" << plan.candidate_pairs()
              << ",\"candidate_descriptors\":" << plan.blocks().size()
              << ",\"baseline_checksum\":\"" << std::hex << old_hashes[lane]
              << "\",\"batch_checksum\":\"" << new_hashes[lane] << std::dec
              << "\",\"baseline_work\":";
    print_work((*baseline)[lane].work());
    std::cout << ",\"batch_work\":";
    print_work(plan.work());
    std::cout << '}';
  }
  std::cout << "]}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(parse_options(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_batch_probe: " << error.what() << '\n';
    return 2;
  }
}
