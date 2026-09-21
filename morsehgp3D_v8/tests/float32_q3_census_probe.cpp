#include "lanes/float32_q3_census.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cfenv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace {
using Words = mhgp8::Float32Words;
using Options = mhgp8::Float32Q3CensusOptions;
using Mode = mhgp8::Float32Q3CensusMode;
using Work = mhgp8::Float32Q3CensusWork;
using Emission = mhgp8::Float32Q3Emission;
using Clock = std::chrono::steady_clock;

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template<class Integer> Integer integer(std::string_view text, int base = 10) {
  Integer value{};
  const auto parsed = std::from_chars(text.data(), text.data()+text.size(), value, base);
  require(!text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data()+text.size(), "invalid integer token");
  return value;
}

std::string token() {
  std::string value;
  require(static_cast<bool>(std::cin >> value), "missing input token");
  return value;
}

template<class Range> void array(const Range& values) {
  std::cout << '[';
  bool first = true;
  for (const auto value : values) {
    if (!first) std::cout << ',';
    std::cout << value; first = false;
  }
  std::cout << ']';
}

#define BLOCK_FIELDS(F) F(preparations) F(gram_positive) F(gram_unresolved) F(center_axes_tightened) F(center_intersection_fallbacks) F(bound_queries) F(inside_certificates) F(outside_certificates) F(unknown_bounds) F(axis_parabolas) F(vertex_clamps) F(power_evaluations) F(interval_additions) F(interval_products) F(interval_divisions) F(scalar_divisions)
#define BALL_FIELDS(F) F(q3_preparations) F(q4_preparations) F(preparation_filter_attempts) F(preparation_filter_accepts) F(preparation_exact_fallbacks) F(preparation_exact_evaluations) F(accepted_supports) F(rejected_supports) F(power_queries) F(power_filter_attempts) F(power_filter_accepts) F(power_exact_fallbacks) F(power_exact_evaluations) F(interval_additions) F(interval_products) F(exact_additions) F(exact_products) F(exact_point_decodes)
#define CENSUS_FIELDS(F) F(calls) F(input_seed_slots) F(shared_frames) F(shared_splits) F(shared_witness_visits) F(shared_endpoint_skips) F(shared_inside_nodes) F(shared_inside_sites) F(shared_outside_nodes) F(shared_witness_splits) F(shared_saturated_blocks) F(shared_rejected_seed_slots) F(shared_children_with_credit) F(relay_blocks) F(relayed_seed_slots) F(endpoint_seeds) F(support_candidates) F(invalid_supports) F(valid_supports) F(relays_with_credit) F(relays_at_eof) F(count_node_visits) F(count_point_tests) F(count_inside_nodes) F(count_inside_sites) F(count_outside_nodes) F(count_splits) F(saturated_supports) F(accepted_supports) F(shell_node_visits) F(shell_point_tests) F(shell_excluded_nodes) F(shell_splits) F(shell_ids) F(callbacks) F(peak_pending_frames) F(stack_capacity_bytes) F(peak_shell_capacity_bytes)
#define INDEX_FIELDS(F) F(input_word_triples_copied) F(finite_points_validated) F(point_objects_constructed) F(presort_comparisons) F(duplicate_adjacent_tests) F(box_endpoint_reads) F(partition_rank_writes) F(partition_id_reads) F(partition_id_writes) F(inverse_rank_writes) F(nodes) F(leaves)
#define FIELD(name) if (!first) std::cout << ','; std::cout << '"' << #name << "\":" << work.name; first = false;
void dump(const mhgp8::Float32Q3BlockWork& work) { bool first = true; std::cout << '{'; BLOCK_FIELDS(FIELD) std::cout << '}'; }
void dump(const mhgp8::Float32BallWork& work) { bool first = true; std::cout << '{'; BALL_FIELDS(FIELD) std::cout << '}'; }
void dump(const mhgp8::Float32IndexWork& work) { bool first = true; std::cout << '{'; INDEX_FIELDS(FIELD) std::cout << '}'; }
void dump(const Work& work) {
  bool first = false;
  std::cout << "{\"shared_bounds\":"; dump(work.shared_bounds);
  std::cout << ",\"individual_bounds\":"; dump(work.individual_bounds);
  std::cout << ",\"supports\":"; dump(work.supports);
  std::cout << ",\"power\":"; dump(work.power);
  CENSUS_FIELDS(FIELD)
  std::cout << '}';
}
#undef FIELD

std::uint64_t mix(std::uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}

struct Payload {
  std::uint64_t callbacks{}, shell_ids{}, depth_sum{}, digest_sum{}, digest_xor{};
  void add(const Emission& emission) {
    std::uint64_t sum = 0, parity = 0;
    for (const auto id : emission.shell) { const auto h = mix(id); sum += h; parity ^= h; }
    const auto h = mix(emission.seed) ^ std::rotl(mix(emission.depth), 7) ^
                   std::rotl(sum, 19) ^ std::rotl(parity, 31) ^ mix(emission.shell.size());
    ++callbacks; shell_ids += emission.shell.size(); depth_sum += emission.depth;
    digest_sum += h; digest_xor ^= h;
  }
};

void dump(const Payload& p) {
  std::cout << "{\"callbacks\":" << p.callbacks << ",\"shell_ids\":" << p.shell_ids << ",\"depth_sum\":" << p.depth_sum
            << ",\"digest_sum\":" << p.digest_sum << ",\"digest_xor\":" << p.digest_xor << '}';
}

struct Request {
  std::size_t a{}, b{}, node{}, kmax{}, mode{}, grain{};
  Options options() const { return {kmax, static_cast<Mode>(mode), grain}; }
};

void dump(const Request& r) {
  std::cout << "{\"a\":" << r.a << ",\"b\":" << r.b << ",\"node\":" << r.node << ",\"kmax\":" << r.kmax
            << ",\"mode\":" << r.mode << ",\"grain\":" << r.grain << '}';
}

void selftest() {
  const std::vector<Words> input{{0xbf800000U, 0, 0}, {0x3f800000U, 0, 0}, {0, 0x40000000U, 0}};
  const auto index = mhgp8::prepare_float32_index(input);
  std::uint64_t tests = 0, callbacks = 0, invalids = 0;
  const auto check = [&](bool condition, const char* message) { ++tests; require(condition, message); };
  const auto checked = [&](Mode mode, std::size_t grain) {
    Work work{};
    std::size_t emitted = 0;
    mhgp8::run_float32_q3_edge_census(index, 0, 1, 0, Options{2, mode, grain}, [&](const Emission& e) {
      auto shell = std::vector<std::size_t>(e.shell.begin(), e.shell.end());
      std::sort(shell.begin(), shell.end());
      check(e.seed == 2 && e.depth == 0 && shell == std::vector<std::size_t>({0, 1, 2}), "selftest depth/global shell");
      check(e.ball.arity() == 3, "callback lost positive q3 support");
      ++emitted; ++callbacks;
    }, work);
    check(emitted == 1 && work.callbacks == 1 && work.shell_ids == 3, "selftest missing callback/work");
  };
  for (const auto mode : {Mode::Individual, Mode::SharedPrefix})
    for (const std::size_t grain : {std::size_t{1}, std::size_t{4}, std::size_t{16}}) checked(mode, grain);
  const auto rejects = [&](mhgp8::Float32IndexPtr owner, std::size_t a, std::size_t b, std::size_t node,
                           Options options, bool empty_callback) {
    Work work{};
    std::size_t emitted = 0;
    std::function<void(const Emission&)> callback;
    if (!empty_callback) callback = [&](const Emission&) { ++emitted; };
    bool refused = false;
    try { mhgp8::run_float32_q3_edge_census(std::move(owner), a, b, node, options, callback, work); }
    catch (const std::invalid_argument&) { refused = true; }
    check(refused && emitted == 0 && work == Work{}, "invalid argument changed work/output or was accepted");
    ++invalids;
  };
  rejects({}, 0, 1, 0, {}, false);
  rejects(index, 3, 1, 0, {}, false); rejects(index, 0, 3, 0, {}, false);
  rejects(index, 0, 0, 0, {}, false); rejects(index, 0, 1, index->nodes().size(), {}, false);
  rejects(index, 0, 1, 0, Options{0, Mode::Individual, 1}, false);
  rejects(index, 0, 1, 0, Options{1, Mode::Individual, 1}, false);
  rejects(index, 0, 1, 0, Options{2, static_cast<Mode>(91), 1}, false);
  rejects(index, 0, 1, 0, Options{2, Mode::Individual, 0}, false);
  rejects(index, 0, 1, 0, {}, true);
  struct Interrupted final : std::exception {};
  Work partial{};
  std::size_t observed = 0;
  bool propagated = false;
  try {
    mhgp8::run_float32_q3_edge_census(index, 0, 1, 0, Options{2, Mode::SharedPrefix, 1}, [&](const Emission& e) {
      observed += e.shell.size(); throw Interrupted{};
    }, partial);
  } catch (const Interrupted&) { propagated = true; }
  check(propagated && observed == 3 && partial.callbacks == 1 && partial.shell_ids == 3, "callback exception lost partial output/work");
  checked(Mode::SharedPrefix, 1);
  auto lifetime = mhgp8::prepare_float32_index(input);
  const std::weak_ptr<const mhgp8::Float32CloudIndex> weak = lifetime;
  Work lifetime_work{};
  mhgp8::run_float32_q3_edge_census(lifetime, 0, 1, 0, Options{2, Mode::Individual, 1}, [&](const Emission&) {
    lifetime.reset(); check(!weak.expired(), "census did not own index during callback");
  }, lifetime_work);
  check(weak.expired(), "census retained an owner after return");
  constexpr std::size_t workers = 4, repeats = 4;
  std::array<std::exception_ptr, workers> errors{};
  {
    std::vector<std::jthread> threads;
    for (std::size_t worker = 0; worker != workers; ++worker) {
      threads.emplace_back([&, worker] {
        try {
          for (std::size_t repeat = 0; repeat != repeats; ++repeat) {
            for (const auto mode : {Mode::Individual, Mode::SharedPrefix}) {
              Work work{};
              std::size_t count = 0;
              mhgp8::run_float32_q3_edge_census(index, 0, 1, 0, Options{2, mode, 1}, [&](const Emission& e) {
                auto shell = std::vector<std::size_t>(e.shell.begin(), e.shell.end());
                std::sort(shell.begin(), shell.end());
                require(e.seed == 2 && e.depth == 0 && shell == std::vector<std::size_t>({0, 1, 2}), "concurrent census payload");
                ++count;
              }, work);
              require(count == 1 && work.callbacks == 1, "concurrent census missing output");
            }
          }
        } catch (...) { errors[worker] = std::current_exception(); }
      });
    }
  }
  for (const auto& error : errors) {
    if (error) std::rethrow_exception(error);
    check(!error, "concurrent census reader failed");
  }
  // Test the NEW block division and continuous parabola bounds under every
  // rounding/flush environment; qualification of old ball predicates is not
  // substituted for these tests. Preserve the caller's environment on errors.
  struct Environment {
    int rounding{std::fegetround()};
#if defined(__SSE__)
    unsigned mxcsr{_mm_getcsr()};
#endif
    ~Environment() {
      static_cast<void>(std::fesetround(rounding));
#if defined(__SSE__)
      _mm_setcsr(mxcsr);
#endif
    }
  } environment;
#if defined(__SSE__)
  constexpr unsigned flush_modes = 4;
#else
  constexpr unsigned flush_modes = 1;
#endif
  std::uint64_t environment_calls = 0, bound_checks = 0;
  const std::array<std::vector<Words>, 3> clouds{{input,
      {{0x80000001U, 0, 0}, {1, 0, 0}, {0, 2, 0}},
      {{0xff7fffffU, 0, 0}, {0x7f7fffffU, 0, 0}, {0, 0x7f7fffffU, 1}}}};
  for (const int rounding : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    check(std::fesetround(rounding) == 0, "rounding mode unavailable");
    for (unsigned flush = 0; flush != flush_modes; ++flush) {
#if defined(__SSE__)
      auto control = _mm_getcsr() & ~((1U << 15) | (1U << 6));
      if ((flush & 1U) != 0) control |= 1U << 15;
      if ((flush & 2U) != 0) control |= 1U << 6;
      _mm_setcsr(control);
#endif
      for (const auto& cloud : clouds) {
        const auto owner = mhgp8::prepare_float32_index(cloud);
        for (const auto mode : {Mode::Individual, Mode::SharedPrefix}) {
          Work work{};
          std::size_t emitted = 0;
          mhgp8::run_float32_q3_edge_census(owner, 0, 1, 0, Options{2, mode, 1}, [&](const Emission& e) {
            auto shell = std::vector<std::size_t>(e.shell.begin(), e.shell.end());
            std::sort(shell.begin(), shell.end());
            check(e.seed == 2 && e.depth == 0 && shell == std::vector<std::size_t>({0, 1, 2}),
                  "rounding/FTZ changed census depth or complete shell");
            ++emitted;
          }, work);
          check(emitted == 1 && work.callbacks == 1, "rounding/FTZ lost acute seed");
          ++environment_calls;
        }
      }
      for (const std::size_t scaled : {std::size_t{0}, std::size_t{1}}) {
        const auto& cloud = clouds[scaled];
        const auto a = mhgp8::Float32Point3::from_bits(cloud[0]);
        const auto b = mhgp8::Float32Point3::from_bits(cloud[1]);
        mhgp8::Float32Q3BlockWork work{};
        const auto prepared = mhgp8::Float32Q3Block::make(a, b, mhgp8::Float32Box3::from_corners(cloud[2], cloud[2]), work);
        const double scale = scaled == 0 ? 1.0 : 0x1p-149;
        const std::array<double, 3> centre{0, .75*scale, 0};
        for (std::size_t axis = 0; axis != 3; ++axis) {
          const auto interval = prepared.center_bounds()[axis];
          check(interval.low <= centre[axis] && centre[axis] <= interval.high, "rounded block excluded rational centre");
          ++bound_checks;
        }
        const auto one = scaled == 0 ? 0x3f800000U : 1U;
        const auto two = scaled == 0 ? 0x40000000U : 2U;
        const std::array<Words, 3> witnesses{{{0, one, 0}, {0, one | 0x80000000U, 0}, cloud[0]}};
        const std::array<double, 3> powers{-1.5*scale*scale, 1.5*scale*scale, 0};
        for (std::size_t i = 0; i != witnesses.size(); ++i) {
          const auto bounds = prepared.bounds(mhgp8::Float32Box3::from_corners(witnesses[i], witnesses[i]), work);
          const bool certified = bounds.low <= powers[i] && powers[i] <= bounds.high &&
                                 (i == 0 ? bounds.high < 0 : i == 1 ? bounds.low > 0 : true);
          if (!certified)
            std::cerr << "bound diagnostic rounding=" << rounding << " flush=" << flush << " scaled=" << scaled
                      << " witness=" << i << " low=" << bounds.low << " high=" << bounds.high << " power=" << powers[i] << '\n';
          check(certified,
                "rounded singleton bound/contact certificate wrong");
          ++bound_checks;
        }
        const Words low{two | 0x80000000U, two | 0x80000000U, two | 0x80000000U}, high{two, two, two};
        const auto continuous = prepared.bounds(mhgp8::Float32Box3::from_corners(low, high), work);
        // All eight corners are outside, but the real centre is inside Z.
        // Integer floor/ceil or testing corners only would lose this minimum.
        check(continuous.low <= -1.5625*scale*scale && continuous.high >= 14*scale*scale,
              "continuous box bound lost interior parabola vertex");
        ++bound_checks;
      }
    }
  }
  std::cout << "{\"schema\":\"mhgp8_float32_q3_census_selftest_v1\",\"status\":\"passed\",\"tests\":" << tests
            << ",\"callbacks_checked\":" << callbacks << ",\"invalid_arguments\":" << invalids
            << ",\"callback_exceptions\":1,\"lifetime_checks\":2,\"concurrent_workers\":" << workers
            << ",\"concurrent_calls\":" << workers * repeats * 2 << ",\"rounding_modes\":4,\"flush_modes\":" << flush_modes
            << ",\"environment_calls\":" << environment_calls << ",\"bound_checks\":" << bound_checks << "}\n";
}

void bench(std::size_t n, std::string_view regime, std::size_t mode, std::size_t kmax, std::size_t grain) {
  require(n >= 3 && (regime == "column" || regime == "slab") && mode <= 1 && kmax >= 2 && grain > 0,
          "invalid census benchmark options");
  const auto word = [](double value) { return std::bit_cast<std::uint32_t>(static_cast<float>(value)); };
  std::vector<Words> points;
  points.reserve(n);
  points.push_back({word(-1), 0, 0}); points.push_back({word(1), 0, 0});
  for (std::size_t i = 0; i != n-2; ++i) {
    const double x = regime == "column" ? 0 : (static_cast<int>(((i % 127)*37) % 127)-63)/128.0;
    const double z = regime == "column" ? 0 : (static_cast<int>(((i % 31)*53) % 31)-15)/256.0;
    points.push_back({word(x), word(2.0+static_cast<double>(i)/4.0), word(z)});
  }
  const auto begin = Clock::now();
  const auto index = mhgp8::prepare_float32_index(points);
  const auto built = Clock::now();
  Work work{};
  Payload payload;
  mhgp8::run_float32_q3_edge_census(index, 0, 1, 0, Options{kmax, static_cast<Mode>(mode), grain},
                                 [&](const Emission& e) { payload.add(e); }, work);
  const auto finished = Clock::now();
  std::cout.precision(17);
  std::cout << "{\"schema\":\"mhgp8_float32_q3_census_bench_v1\",\"status\":\"passed\",\"n\":" << n
            << ",\"regime\":\"" << regime << "\",\"mode\":" << mode << ",\"kmax\":" << kmax << ",\"relay_sites\":" << grain
            << ",\"construction_ms\":" << std::chrono::duration<double, std::milli>(built-begin).count()
            << ",\"census_ms\":" << std::chrono::duration<double, std::milli>(finished-built).count()
            << ",\"index_work\":"; dump(index->work());
  std::cout << ",\"index_bytes\":" << index->retained_bytes() << ",\"work\":"; dump(work);
  std::cout << ",\"payload\":"; dump(payload); std::cout << "}\n";
}

void probe() {
  const auto n = integer<std::size_t>(token()), q = integer<std::size_t>(token());
  std::vector<Words> points;
  points.reserve(n);
  for (std::size_t i = 0; i != n; ++i) {
    Words words{};
    for (auto& w : words) { const auto text = token(); require(text.size() == 8, "requires eight hexadecimal digits"); w = integer<std::uint32_t>(text, 16); }
    points.push_back(words);
  }
  std::vector<Request> requests;
  requests.reserve(q);
  for (std::size_t i = 0; i != q; ++i)
    requests.push_back({integer<std::size_t>(token()), integer<std::size_t>(token()), integer<std::size_t>(token()),
                       integer<std::size_t>(token()), integer<std::size_t>(token()), integer<std::size_t>(token())});
  std::string extra;
  require(!(std::cin >> extra) && std::cin.eof() && !std::cin.bad(), "extra input or stream failure");
  const auto index = mhgp8::prepare_float32_index(points);
  // Validate all requests before writing a partial JSON document. Runtime
  // census errors still retain partial stdout and the failing native exit.
  for (const auto& r : requests)
    require(r.a < n && r.b < n && r.a != r.b && r.node < index->nodes().size() && r.kmax >= 2 && r.mode <= 1 && r.grain > 0,
            "invalid census request");
  std::cout << "{\"schema\":\"mhgp8_float32_q3_census_probe_v1\",\"status\":\"passed\",\"points\":[";
  for (std::size_t i = 0; i != n; ++i) { if (i != 0) std::cout << ','; array(index->points()[i].bits()); }
  std::cout << "],\"permutation\":"; array(index->permutation());
  std::cout << ",\"nodes\":[";
  for (std::size_t i = 0; i != index->nodes().size(); ++i) {
    if (i != 0) std::cout << ',';
    const auto& node = index->nodes()[i];
    std::cout << "{\"first\":" << node.first << ",\"last\":" << node.last << ",\"left\":";
    if (node.leaf()) std::cout << "null"; else std::cout << node.left;
    std::cout << ",\"right\":";
    if (node.leaf()) std::cout << "null"; else std::cout << node.right;
    std::cout << ",\"escape\":" << node.escape << ",\"low\":"; array(node.box.low());
    std::cout << ",\"high\":"; array(node.box.high()); std::cout << '}';
  }
  std::cout << "],\"queries\":[";
  for (std::size_t i = 0; i != requests.size(); ++i) {
    if (i != 0) std::cout << ',';
    const auto& request = requests[i];
    std::cout << "{\"request\":"; dump(request); std::cout << ",\"emissions\":[";
    bool first = true;
    Work work{};
    mhgp8::run_float32_q3_edge_census(index, request.a, request.b, request.node, request.options(), [&](const Emission& e) {
      if (!first) std::cout << ',';
      first = false;
      std::cout << "{\"seed\":" << e.seed << ",\"depth\":" << e.depth << ",\"shell\":"; array(e.shell); std::cout << '}';
    }, work);
    std::cout << "],\"work\":"; dump(work); std::cout << '}';
  }
  std::cout << "]}\n";
}
}  // namespace

int main(int argc, char** argv) {
  const bool testing = argc == 2 && std::string_view(argv[1]) == "--selftest";
  try {
    if (testing) selftest();
    else if (argc == 7 && std::string_view(argv[1]) == "--bench")
      bench(integer<std::size_t>(argv[2]), argv[3], integer<std::size_t>(argv[4]), integer<std::size_t>(argv[5]), integer<std::size_t>(argv[6]));
    else { require(argc == 1, "usage: probe [--selftest | --bench N column|slab MODE K GRAIN]"); probe(); }
    require(static_cast<bool>(std::cout), "output stream failure");
    return 0;
  } catch (const std::exception& error) {
    if (testing) std::cout << "{\"status\":\"failed\"}\n";
    std::cerr << "float32 q3 census probe: " << error.what() << '\n';
    return 1;
  }
}
