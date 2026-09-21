#include "spatial/float32_index.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cfenv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace {
using Words = mhgp8::Float32Words;
using Box = mhgp8::Float32Box3;
using Index = mhgp8::Float32CloudIndex;
using QueryWork = mhgp8::Float32BoxQueryWork;

static_assert(!std::is_copy_constructible_v<Index> && !std::is_move_constructible_v<Index>);
static_assert(!std::is_copy_assignable_v<Index> && !std::is_move_assignable_v<Index>);
static_assert(std::is_same_v<decltype(std::declval<const Index&>().points()),
                           std::span<const mhgp8::Float32Point3>>);
static_assert(std::is_same_v<decltype(std::declval<const Index&>().nodes()),
                           std::span<const mhgp8::Float32IndexNode>>);

void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

template <class Integer>
Integer number(const std::string& token, int base) {
  Integer result{};
  const auto parsed = std::from_chars(token.data(), token.data() + token.size(), result, base);
  require(!token.empty() && parsed.ec == std::errc{} && parsed.ptr == token.data() + token.size(),
          "invalid integer token");
  return result;
}

std::string token() {
  std::string value;
  require(static_cast<bool>(std::cin >> value), "missing input token");
  return value;
}

Words read_words() {
  Words result{};
  for (auto& word : result) {
    const auto text = token();
    require(text.size() == 8, "each coordinate requires eight hexadecimal digits");
    word = number<std::uint32_t>(text, 16);
  }
  return result;
}

template <class Range>
void dump_array(const Range& values) {
  std::cout << '[';
  bool first = true;
  for (const auto value : values) {
    if (!first) std::cout << ',';
    std::cout << value;
    first = false;
  }
  std::cout << ']';
}

void dump(const mhgp8::Float32IndexWork& work) {
  std::cout << '{';
#define FIELD(name) std::cout << '"' << #name << "\":" << work.name
  FIELD(input_word_triples_copied); std::cout << ',';
  FIELD(finite_points_validated); std::cout << ',';
  FIELD(point_objects_constructed); std::cout << ',';
  FIELD(presort_comparisons); std::cout << ',';
  FIELD(duplicate_adjacent_tests); std::cout << ',';
  FIELD(box_endpoint_reads); std::cout << ',';
  FIELD(partition_rank_writes); std::cout << ',';
  FIELD(partition_id_reads); std::cout << ',';
  FIELD(partition_id_writes); std::cout << ',';
  FIELD(inverse_rank_writes); std::cout << ',';
  FIELD(nodes); std::cout << ',';
  FIELD(leaves);
#undef FIELD
  std::cout << '}';
}

void dump(const QueryWork& work) {
  std::cout << '{';
#define FIELD(name) std::cout << '"' << #name << "\":" << work.name
  FIELD(node_visits); std::cout << ',';
  FIELD(axis_tests); std::cout << ',';
  FIELD(rejected_nodes); std::cout << ',';
  FIELD(accepted_nodes); std::cout << ',';
  FIELD(refined_nodes); std::cout << ',';
  FIELD(emitted_sites); std::cout << ',';
  FIELD(callbacks);
#undef FIELD
  std::cout << '}';
}

auto query_signature(const QueryWork& work) {
  return std::array<std::uint64_t, 7>{work.node_visits, work.axis_tests, work.rejected_nodes,
      work.accepted_nodes, work.refined_nodes, work.emitted_sites, work.callbacks};
}

std::vector<std::size_t> query(const Index& index, const Box& box) {
  QueryWork work{};
  std::vector<std::size_t> result;
  index.visit_box(box, [&](std::span<const std::size_t> ids) {
    result.insert(result.end(), ids.begin(), ids.end());
  }, work);
  require(work.emitted_sites == result.size() && work.callbacks == work.accepted_nodes,
          "query selftest work mismatch");
  std::sort(result.begin(), result.end());
  return result;
}

void selftest() {
  std::uint64_t tests = 0, invalid_inputs = 0;
  const auto check = [&](bool condition, const char* message) {
    ++tests;
    require(condition, message);
  };
  const auto rejects = [&](const auto& action) {
    bool refused = false;
    try { action(); } catch (const std::invalid_argument&) { refused = true; }
    check(refused, "invalid index input or callback accepted");
    ++invalid_inputs;
  };
  constexpr std::uint32_t one = 0x3f800000U, maximum = 0x7f7fffffU;
  const std::vector<Words> original{
      {maximum, 0, 0}, {0xff7fffffU, 0, 0}, {1, 0, 0}, {2, 0, 0},
      {0x80000001U, 0, 0}, {0x80000000U, one, 0x80000000U},
      {0, 0xbf800000U, 0}, {0, 0, 1}, {0, 0, 0x80000001U}};
  const auto all = Box::from_corners({0xff7fffffU, 0xff7fffffU, 0xff7fffffU},
                                    {maximum, maximum, maximum});
  mhgp8::Float32IndexPtr owned;
  {
    auto input = original;
    auto* alias = input.data();
    owned = mhgp8::prepare_float32_index(input);
    alias[0] = {one, one, one};
    input[1] = {0, 0, 0};
    for (std::size_t i = 0; i != original.size(); ++i)
      check(owned->points()[i].bits() == original[i], "caller alias changed private point bits or IDs");
  }
  for (std::size_t i = 0; i != original.size(); ++i)
    check(owned->points()[i].bits() == original[i], "caller destruction invalidated point bits");
  check(owned->points()[5].bits()[0] == 0x80000000U &&
        owned->points()[5].bits()[2] == 0x80000000U, "unique signed-zero words were rewritten");
  std::vector<std::size_t> all_ids(original.size());
  for (std::size_t i = 0; i != all_ids.size(); ++i) all_ids[i] = i;
  check(query(*owned, all) == all_ids, "whole box lost original IDs");
  check(query(*owned, Box::from_corners({1, 0, 0}, {1, 0, 0})) == std::vector<std::size_t>{2},
        "subnormal singleton query lost its site");
  check(query(*owned, Box::from_corners({0, one, 0}, {0, one, 0})) == std::vector<std::size_t>{5},
        "positive-zero query missed negative-zero coordinates");
  rejects([&] { static_cast<void>(mhgp8::prepare_float32_index(std::span<const Words>{})); });
  for (unsigned mask = 0; mask != 8; ++mask) {
    Words zero{};
    for (std::size_t axis = 0; axis != 3; ++axis)
      if ((mask & (1U << axis)) != 0) zero[axis] = 0x80000000U;
    const std::array<Words, 2> duplicates{Words{}, zero};
    rejects([&] { static_cast<void>(mhgp8::prepare_float32_index(duplicates)); });
  }
  for (const auto invalid : std::array<std::uint32_t, 6>{
           0x7f800000U, 0xff800000U, 0x7fc00000U, 0xffc00000U, 0x7f800001U, 0xff800001U}) {
    for (std::size_t axis = 0; axis != 3; ++axis) {
      Words words{}; words[axis] = invalid;
      const std::array<Words, 1> bad{words};
      rejects([&] { static_cast<void>(mhgp8::prepare_float32_index(bad)); });
      rejects([&] { static_cast<void>(Box::from_corners(words, Words{})); });
      rejects([&] { static_cast<void>(Box::from_corners(Words{}, words)); });
    }
  }
  for (std::size_t axis = 0; axis != 3; ++axis) {
    Words low{}; low[axis] = one;
    rejects([&] { static_cast<void>(Box::from_corners(low, Words{})); });
  }
  QueryWork untouched{};
  const auto empty_work = query_signature(untouched);
  rejects([&] { owned->visit_box(all, {}, untouched); });
  check(query_signature(untouched) == empty_work, "empty callback mutated query work");
  struct CallbackFailure final : std::exception {};
  QueryWork interrupted{};
  std::vector<std::size_t> partial;
  bool propagated = false;
  try {
    owned->visit_box(all, [&](std::span<const std::size_t> ids) {
      partial.insert(partial.end(), ids.begin(), ids.end());
      throw CallbackFailure{};
    }, interrupted);
  } catch (const CallbackFailure&) { propagated = true; }
  check(propagated && partial.size() == original.size() && interrupted.callbacks == 1 &&
        interrupted.emitted_sites == original.size(), "callback failure lost its partial work/output");
  check(query(*owned, all) == all_ids, "callback failure damaged reusable index");

  // Geometry remains correct in each floating environment; the split-axis
  // heuristic is allowed to choose a different valid median when lengths tie.
  const auto initial_rounding = std::fegetround();
  check(initial_rounding != -1, "cannot read floating rounding mode");
#if defined(__SSE__)
  const auto initial_mxcsr = _mm_getcsr();
  constexpr unsigned flush_modes = 4;
#else
  constexpr unsigned flush_modes = 1;
#endif
  try {
    for (const auto rounding : std::array<int, 4>{FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO}) {
      check(std::fesetround(rounding) == 0, "cannot install floating rounding mode");
      for (unsigned flush = 0; flush != flush_modes; ++flush) {
#if defined(__SSE__)
        auto csr = _mm_getcsr() & ~static_cast<unsigned>((1U << 15) | (1U << 6));
        if ((flush & 1U) != 0) csr |= 1U << 15;
        if ((flush & 2U) != 0) csr |= 1U << 6;
        _mm_setcsr(csr);
#endif
        const auto index = mhgp8::prepare_float32_index(original);
        check(index->max_depth() <= std::bit_width(original.size() - 1), "median depth depends on exponent range");
        check(query(*index, all) == all_ids, "floating environment lost whole-box sites");
        check(query(*index, Box::from_corners({1, 0, 0}, {2, 0, 0})) == std::vector<std::size_t>({2, 3}),
              "DAZ/FTZ or rounding lost a subnormal boundary");
      }
    }
  } catch (...) {
    static_cast<void>(std::fesetround(initial_rounding));
#if defined(__SSE__)
    _mm_setcsr(initial_mxcsr);
#endif
    throw;
  }
  check(std::fesetround(initial_rounding) == 0, "cannot restore floating rounding mode");
#if defined(__SSE__)
  _mm_setcsr(initial_mxcsr);
#endif

  constexpr std::size_t workers = 4, repetitions = 8;
  std::array<std::exception_ptr, workers> errors{};
  {
    std::vector<std::jthread> threads;
    threads.reserve(workers);
    for (std::size_t worker = 0; worker != workers; ++worker) {
      threads.emplace_back([&, worker] {
        try {
          for (std::size_t repeat = 0; repeat != repetitions; ++repeat) {
            require(query(*owned, all) == all_ids, "concurrent all-box query changed IDs");
            require(query(*owned, Box::from_corners({1, 0, 0}, {1, 0, 0})) == std::vector<std::size_t>{2},
                    "concurrent point-box query changed IDs");
          }
        } catch (...) { errors[worker] = std::current_exception(); }
      });
    }
  }
  for (const auto& error : errors) {
    if (error) std::rethrow_exception(error);
    check(!error, "concurrent worker failed");
  }
  const std::weak_ptr<const Index> weak = owned;
  owned.reset();
  check(weak.expired(), "index ownership cycle survives last consumer");
  std::cout << "{\"schema\":\"mhgp8_float32_index_selftest_v1\",\"status\":\"passed\",\"tests\":" << tests
            << ",\"invalid_inputs\":" << invalid_inputs << ",\"rounding_modes\":4,\"flush_modes\":" << flush_modes
            << ",\"concurrent_workers\":" << workers << ",\"concurrent_queries\":" << workers * repetitions * 2 << "}\n";
}

void probe() {
  const auto n = number<std::size_t>(token(), 10);
  const auto q = number<std::size_t>(token(), 10);
  std::vector<Words> input;
  input.reserve(n);
  for (std::size_t i = 0; i != n; ++i) input.push_back(read_words());
  std::vector<Box> boxes;
  boxes.reserve(q);
  for (std::size_t i = 0; i != q; ++i) {
    const auto low = read_words(), high = read_words();
    boxes.push_back(Box::from_corners(low, high));
  }
  std::string extra;
  require(!(std::cin >> extra) && std::cin.eof() && !std::cin.bad(), "extra token or input read failure");
  const auto index = mhgp8::prepare_float32_index(input);
  std::cout << "{\"schema\":\"mhgp8_float32_index_probe_v1\",\"status\":\"passed\",\"points\":[";
  for (std::size_t i = 0; i != n; ++i) {
    if (i != 0) std::cout << ',';
    dump_array(index->points()[i].bits());
  }
  std::cout << "],\"permutation\":"; dump_array(index->permutation());
  std::cout << ",\"inverse\":"; dump_array(index->inverse());
  std::cout << ",\"nodes\":[";
  for (std::size_t i = 0; i != index->nodes().size(); ++i) {
    if (i != 0) std::cout << ',';
    const auto& node = index->nodes()[i];
    std::cout << "{\"first\":" << node.first << ",\"last\":" << node.last << ",\"left\":";
    if (node.left == mhgp8::Float32IndexNode::none) std::cout << "null"; else std::cout << node.left;
    std::cout << ",\"right\":";
    if (node.right == mhgp8::Float32IndexNode::none) std::cout << "null"; else std::cout << node.right;
    std::cout << ",\"escape\":" << node.escape << ",\"low\":"; dump_array(node.box.low());
    std::cout << ",\"high\":"; dump_array(node.box.high());
    std::cout << '}';
  }
  std::cout << "],\"work\":"; dump(index->work());
  std::cout << ",\"max_depth\":" << index->max_depth()
            << ",\"retained_bytes\":" << index->retained_bytes()
            << ",\"construction_peak_vector_bytes\":" << index->construction_peak_vector_bytes()
            << ",\"queries\":[";
  for (std::size_t i = 0; i != boxes.size(); ++i) {
    if (i != 0) std::cout << ',';
    const auto& box = boxes[i];
    std::vector<std::size_t> ids;
    QueryWork work{};
    index->visit_box(box, [&](std::span<const std::size_t> emitted) {
      ids.insert(ids.end(), emitted.begin(), emitted.end());
    }, work);
    std::cout << "{\"low\":"; dump_array(box.low());
    std::cout << ",\"high\":"; dump_array(box.high());
    std::cout << ",\"ids\":"; dump_array(ids);
    std::cout << ",\"work\":"; dump(work);
    std::cout << '}';
  }
  std::cout << "]}\n";
}
}  // namespace

int main(int argc, char** argv) {
  const bool testing = argc == 2 && std::string_view(argv[1]) == "--selftest";
  try {
    require(argc == 1 || testing, "usage: probe [--selftest]; stdin: N Q, N hex triples, Q hex corner pairs");
    if (testing) selftest(); else probe();
    require(static_cast<bool>(std::cout), "output stream failed");
    return 0;
  } catch (const std::exception& error) {
    if (testing) std::cout << "{\"status\":\"failed\"}\n";
    std::cerr << "float32 index probe: " << error.what() << '\n';
    return 1;
  }
}
