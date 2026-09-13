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
#include "p0_fixtures.hpp"
#include "probe_emit.hpp"

namespace {

using Clock = std::chrono::steady_clock;
using mhgp8::bench::print_work;

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

double elapsed_ms(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

int run(const Options& options) {
  const auto started = Clock::now();
  auto input = mhgp8::bench::make_fixture(options.n, options.family);
  const auto input_hash = mhgp8::bench::fixture_hash(input);
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
