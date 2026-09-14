#include "front_fixtures.hpp"
#include "wspd/front.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

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
    throw std::overflow_error("front probe size exceeds u64");
  return static_cast<u64>(value);
}

u64 product(u64 left, u64 right) {
  if (right != 0 && left > std::numeric_limits<u64>::max() / right)
    throw std::overflow_error("front probe product exceeds u64");
  return left * right;
}

struct Options {
  std::size_t n;
  std::string_view family;
  unsigned kmax;
  unsigned separation;
  u64 seed;
  std::string_view mode;
};

Options options(int argc, char** argv) {
  if (argc != 7)
    throw std::invalid_argument("usage: mhgp8_wspd_front_probe n uniform|terrain|clusters|rows "
                                "Kmax s seed pure|samples");
  const Options result{integer<std::size_t>(argv[1]), argv[2], integer<unsigned>(argv[3]),
                       integer<unsigned>(argv[4]), integer<u64>(argv[5]), argv[6]};
  mhgp8::bench::validate_front_fixture_size(result.n, result.family);
  if (result.kmax == 0 || result.kmax > 10 || result.separation == 0 ||
      (result.mode != "pure" && result.mode != "samples"))
    throw std::invalid_argument("front probe requires Kmax 1..10, positive s, pure|samples");
  return result;
}

double milliseconds(Clock::time_point begin, Clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

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

struct Digest {
  u64 rectangles{};
  u64 factor_sites{};
  u64 pair_mass{};
  std::array<u64, 3> lane_rectangles{};
  std::array<u64, 3> lane_pair_mass{};
  u64 sum{};
  u64 xor_value{};

  // A constant-work streaming observer, not a uniqueness or geometry oracle.
  // Only node handles/ranges/masks are examined; no represented pair expands.
  void consume(const mhgp8::WspdRectangle& rectangle,
               std::span<const mhgp8::Q2SpatialNode> nodes,
               std::size_t n, unsigned active_mask) {
    require(rectangle.a_node < nodes.size() && rectangle.b_node < nodes.size() &&
            rectangle.a_node != rectangle.b_node && rectangle.lane_mask != 0 &&
            (rectangle.lane_mask & ~active_mask) == 0,
            "invalid front node handles or active lane mask");
    const auto a = nodes[rectangle.a_node].range;
    const auto b = nodes[rectangle.b_node].range;
    require(a.first < a.last && b.first < b.last && a.last <= n && b.last <= n &&
            (a.last <= b.first || b.last <= a.first),
            "front rectangle factors are empty, out of range or overlapping");
    const auto na = size_counter(a.size());
    const auto nb = size_counter(b.size());
    const auto mass = product(na, nb);
    mhgp8::counter_add(rectangles);
    mhgp8::counter_add(factor_sites, na);
    mhgp8::counter_add(factor_sites, nb);
    mhgp8::counter_add(pair_mass, mass);
    for (unsigned lane = 0; lane < 3; ++lane) {
      if ((rectangle.lane_mask & (1U << lane)) != 0) {
        mhgp8::counter_add(lane_rectangles[lane]);
        mhgp8::counter_add(lane_pair_mass[lane], mass);
      }
    }
    u64 hash = 14695981039346656037ULL;
    mhgp8::bench::front_hash_word(hash, 1);  // Descriptor encoding, not point-pair digest.
    mhgp8::bench::front_hash_word(hash, size_counter(std::min(rectangle.a_node, rectangle.b_node)));
    mhgp8::bench::front_hash_word(hash, size_counter(std::max(rectangle.a_node, rectangle.b_node)));
    mhgp8::bench::front_hash_word(hash, rectangle.lane_mask);
    sum += hash;  // Checksums alone use deliberate modulo-2^64 arithmetic.
    xor_value ^= hash;
  }
};

void validate(const mhgp8::WspdFrontResult& result, const Digest& digest, const Options& o) {
  const auto n = size_counter(o.n);
  const auto pairs = n % 2 == 0 ? product(n / 2, n - 1) : product(n, (n - 1) / 2);
  const auto active = (1U << std::min(o.kmax, 3U)) - 1U;
  const auto& w = result.work;
  require(result.total_unordered_pairs == pairs && result.active_lane_mask == active,
          "front total pair mass or active mask mismatch");
  require(digest.rectangles == w.emitted_rectangles && digest.factor_sites == w.emitted_factor_sites &&
          digest.lane_rectangles == w.lane_rectangles && digest.lane_pair_mass == w.residual_pair_mass,
          "front callback accounting mismatch");
  u64 binned_rectangles = 0;
  u64 binned_mass = 0;
  for (std::size_t bin = 0; bin < w.size_class_rectangles.size(); ++bin) {
    mhgp8::counter_add(binned_rectangles, w.size_class_rectangles[bin]);
    mhgp8::counter_add(binned_mass, w.size_class_pair_mass[bin]);
  }
  require(binned_rectangles == digest.rectangles && binned_mass == digest.pair_mass &&
          w.leaf_pair_rectangles == w.size_class_rectangles[0], "front size-bin accounting mismatch");
  for (unsigned lane = 0; lane < 3; ++lane) {
    const auto expected = (active & (1U << lane)) != 0 ? pairs : 0;
    require(w.rejected_pair_mass[lane] <= expected &&
            w.residual_pair_mass[lane] == expected - w.rejected_pair_mass[lane],
            "front lane mass conservation failed");
    require(o.mode != "pure" || w.rejected_pair_mass[lane] == 0,
            "pure front unexpectedly rejected pair mass");
  }
  require(w.proposed_sites <= product(o.kmax, w.witness_searches) &&
          w.witness_descent_steps <= product(48, w.witness_searches) &&
          w.witness_box_distance_tests == product(2, w.witness_descent_steps),
          "front witness proposal work exceeds the bounded one-path recipe");
  require(o.mode != "pure" || (w.witness_searches == 0 && w.proposed_sites == 0 &&
          w.h_bound_tests == 0 && w.xi_bound_tests == 0 && w.fully_rejected_products == 0),
          "pure front unexpectedly did witness work");
}

int run(const Options& o) {
  const auto started = Clock::now();
  std::optional<mhgp8::bench::FrontFixture> input(mhgp8::bench::make_front_fixture(o.n, o.family, o.seed));
  const auto generated = Clock::now();
  auto cloud = mhgp8::prepare_cloud(input->points);
  const auto prepared = Clock::now();
  auto index = mhgp8::make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  Digest digest;
  const auto active = (1U << std::min(o.kmax, 3U)) - 1U;
  const auto result = mhgp8::run_wspd_front(*index, o.kmax, o.separation,
      o.mode == "pure" ? mhgp8::WspdFrontMode::Pure : mhgp8::WspdFrontMode::MidpointSamples,
      [&](const mhgp8::WspdRectangle& rectangle) {
        digest.consume(rectangle, index->spatial_nodes(), o.n, active);
      });
  const auto processed = Clock::now();
  validate(result, digest, o);
  require(&index->cloud() == cloud.get(), "front index lost immutable cloud identity");
  const auto input_hash = input->input_hash;
  const auto generation_work = input->work;
  const auto cloud_work = cloud->work();
  const auto index_work = index->work();
  const auto input_bytes = product(size_counter(input->points.capacity()), sizeof(mhgp8::Point3));
  const auto cloud_bytes = cloud->retained_bytes();
  const auto index_bytes = index->retained_bytes();
  const auto validated = Clock::now();
  index.reset();
  cloud.reset();
  input.reset();
  const auto finished = Clock::now();

  std::cout.imbue(std::locale::classic());
  std::cout << std::setprecision(17)
            << "{\"schema\":\"mhgp8_wspd_front_probe_v1\",\"status\":\"completed\""
            << ",\"phase\":\"exploration_v8_hors_registre\",\"backend\":\"cpu_reference\""
            << ",\"profile\":\"quantized_u16_input_only\",\"mode\":\"implementation_v8_p0\""
            << ",\"public_status\":\"not_claimed\",\"scope\":\"real_wspd_front_without_census_or_full\""
            << ",\"separation_convention\":\"box_gap_diameter_v1\",\"threads\":1,\"gcp_used\":false"
            << ",\"n\":" << o.n << ",\"family\":\"" << o.family << "\",\"kmax\":" << o.kmax
            << ",\"s\":" << o.separation << ",\"seed\":" << o.seed
            << ",\"front_mode\":\"" << o.mode << "\",\"recipe\":\"" << mhgp8::bench::front_recipe(o.family)
            << "\",\"seed_affects_input\":" << (o.family == "rows" ? "false" : "true")
            << ",\"input_hash\":\"" << std::hex << input_hash << std::dec << '"'
            << ",\"total_unordered_pairs\":" << result.total_unordered_pairs
            << ",\"active_lane_mask\":" << static_cast<unsigned>(result.active_lane_mask)
            << ",\"generation_work\":{";
  print_fields(generation_work, generation_fields);
  std::cout << "},\"cloud_work\":{";
  print_fields(cloud_work, cloud_fields);
  std::cout << "},\"index_work\":{";
  print_fields(index_work, index_fields);
  std::cout << "},\"front_work\":{";
  print_fields(result.work, front_fields);
  std::cout << ",\"size_class_rectangles\":";
  print_array(result.work.size_class_rectangles);
  std::cout << ",\"size_class_pair_mass\":";
  print_array(result.work.size_class_pair_mass);
  std::cout << ",\"rejected_pair_mass\":";
  print_array(result.work.rejected_pair_mass);
  std::cout << ",\"residual_pair_mass\":";
  print_array(result.work.residual_pair_mass);
  std::cout << ",\"lane_rectangles\":";
  print_array(result.work.lane_rectangles);
  std::cout << "},\"digest\":{\"rectangles\":" << digest.rectangles
            << ",\"factor_sites\":" << digest.factor_sites << ",\"pair_mass\":" << digest.pair_mass
            << ",\"lane_rectangles\":";
  print_array(digest.lane_rectangles);
  std::cout << ",\"lane_pair_mass\":";
  print_array(digest.lane_pair_mass);
  std::cout << ",\"sum\":\"" << std::hex << digest.sum << "\",\"xor\":\"" << digest.xor_value
            << std::dec << "\"},\"memory\":{\"input_capacity_bytes\":" << input_bytes
            << ",\"cloud_retained_bytes\":" << cloud_bytes << ",\"index_retained_bytes\":" << index_bytes
            << "},\"timings\":{\"generation_ms\":" << milliseconds(started, generated)
            << ",\"cloud_ms\":" << milliseconds(generated, prepared)
            << ",\"index_ms\":" << milliseconds(prepared, indexed)
            << ",\"front_and_callback_ms\":" << milliseconds(indexed, processed)
            << ",\"validation_ms\":" << milliseconds(processed, validated)
            << ",\"destruction_ms\":" << milliseconds(validated, finished)
            << ",\"total_ms\":" << milliseconds(started, finished) << "}}\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(options(argc, argv));
  } catch (const std::invalid_argument& error) {
    std::cerr << "mhgp8_wspd_front_probe: " << error.what() << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "mhgp8_wspd_front_probe failed: " << error.what() << '\n';
    return 1;
  }
}
