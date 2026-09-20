#include "lanes/q4_family.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using mhgp8::i64;
using mhgp8::i128;
using mhgp8::Point3;
using mhgp8::u64;
using Clock = std::chrono::steady_clock;

void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

std::size_t number(std::string_view value) {
  std::size_t result{};
  const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
  if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
    throw std::invalid_argument("expected an unsigned decimal integer");
  return result;
}

double ms(Clock::time_point a, Clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}

void hash_word(u64& hash, u64 value) {
  for (unsigned byte = 0; byte < 8; ++byte) {
    hash ^= value & 255U;
    hash *= 1099511628211ULL;
    value >>= 8U;
  }
}

u64 random_word(u64& state) {
  auto value = (state += 0x9e3779b97f4a7c15ULL);
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

u64 point_key(Point3 point) {
  return static_cast<u64>(point.x) | (static_cast<u64>(point.y) << 16U) |
         (static_cast<u64>(point.z) << 32U);
}

struct Input {
  std::vector<Point3> points;
  u64 proposals{}, duplicates{}, random_calls{};
  u64 hash{14695981039346656037ULL};
};

Input make_input(std::size_t n, std::string_view family) {
  // Three fixed seed sites. Their exact circle is centered at
  // (32000,26400,32768), with squared radius 184960000. The triangle is acute.
  Input input{{{20000, 20000, 32768}, {44000, 20000, 32768}, {32000, 40000, 32768}}};
  input.points.reserve(n);
  std::set<u64> seen;
  for (auto point : input.points) seen.insert(point_key(point));
  u64 state = 3;
  while (input.points.size() < n) {
    const auto i = input.points.size() - 3;
    Point3 point;
    if (family == "rows") {
      // Two lines at distinct heights: unlike the old coplanar rows,
      // this regime positively exercises both event orientations.
      point = {static_cast<std::uint16_t>(i % 2 == 0 ? 16000 : 48000),
               static_cast<std::uint16_t>(1000 + i / 2),
               static_cast<std::uint16_t>(i % 2 == 0 ? 28768 : 36768)};
    } else {
      point.x = static_cast<std::uint16_t>(random_word(state) & 65535U);
      point.y = static_cast<std::uint16_t>(random_word(state) & 65535U);
      input.random_calls += 2;
      if (family == "coplanar") point.z = 32768;
      else {
        point.z = static_cast<std::uint16_t>(random_word(state) & 65535U);
        ++input.random_calls;
      }
    }
    ++input.proposals;
    if (!seen.insert(point_key(point)).second) {
      ++input.duplicates;
      continue;
    }
    input.points.push_back(point);
  }
  hash_word(input.hash, n);
  for (auto point : input.points)
    for (unsigned axis = 0; axis < 3; ++axis) hash_word(input.hash, point[axis]);
  return input;
}

// Independent scalar representation specialized to this fixed circle, NOT
// the production Gram/power/comparator. Event roots are center heights t:
// |z-c0|^2-R0^2 - 2*t*(z.z-32768) = 0. Denominators are made positive.
struct ScalarEvent { i64 numerator{}, denominator{}; int direction{}; };
ScalarEvent scalar(Point3 point) {
  const auto x = static_cast<i64>(point.x) - 32000;
  const auto y = static_cast<i64>(point.y) - 26400;
  const auto z = static_cast<i64>(point.z) - 32768;
  const auto numerator = x * x + y * y + z * z - 184960000;
  if (z < 0) return {-numerator, -2 * z, -1};
  if (z > 0) return {numerator, 2 * z, 1};
  return {numerator, 0, 0};
}

int compare(ScalarEvent a, ScalarEvent b) {
  const auto difference = static_cast<i128>(a.numerator) * b.denominator -
                          static_cast<i128>(b.numerator) * a.denominator;
  return difference < 0 ? -1 : difference > 0 ? 1 : 0;
}

int run(std::size_t n, std::string_view family) {
  const auto start = Clock::now();
  auto input = make_input(n, family);
  const auto generated = Clock::now();
  auto cloud = mhgp8::prepare_cloud(input.points);
  const auto prepared = Clock::now();
  std::vector<unsigned char> visited(n);
  std::vector<std::size_t> constant_shell;
  u64 entries = 0, exits = 0, inside = 0, outside = 0;
  for (std::size_t id = 0; id < n; ++id) {
    const auto e = scalar(input.points[id]);
    if (e.direction > 0) ++entries;
    else if (e.direction < 0) ++exits;
    else if (e.numerator < 0) ++inside;
    else if (e.numerator > 0) ++outside;
    else constant_shell.push_back(id);
  }
  const auto oracle_prepared = Clock::now();
  u64 callbacks = 0, root_ids = 0, group_max = 0, depth_sum = 0;
  u64 callback_constant_ids = 0;
  u64 output_hash = 14695981039346656037ULL;
  u64 current_depth = inside + exits;
  ScalarEvent previous{};
  const auto work = mhgp8::run_q4_family(cloud, {0, 1, 2}, [&](const mhgp8::Q4FamilyGroup& group) {
    require(!group.root_ids.empty(), "empty root group");
    require(group.representative_id < n, "representative outside cloud");
    require(group.representative_id == group.root_ids.front(), "representative is not the least original ID");
    const auto root = scalar(input.points[group.representative_id]);
    require(root.direction != 0, "constant representative");
    if (callbacks != 0) require(compare(previous, root) < 0, "roots not strictly sorted");
    previous = root;
    require(group.constant_shell.size() == constant_shell.size(), "constant shell cardinality mismatch");
    // The immutable constant-shell view belongs to this whole synchronous
    // family, not to each root. Reading it once avoids groups*shell work.
    if (callbacks == 0) {
      require(std::equal(group.constant_shell.begin(), group.constant_shell.end(), constant_shell.begin()),
              "constant shell mismatch");
      callback_constant_ids = group.constant_shell.size();
      for (auto id : group.constant_shell) hash_word(output_hash, id);
    }
    std::size_t last = 0;
    bool first = true, found_representative = false;
    u64 entering = 0, leaving = 0;
    for (auto id : group.root_ids) {
      require(id < n && visited[id] == 0, "lost identity or duplicate root site");
      require(first || last < id, "root IDs not sorted");
      first = false;
      last = id;
      found_representative = found_representative || id == group.representative_id;
      visited[id] = 1;
      const auto e = scalar(input.points[id]);
      require(e.direction != 0 && compare(e, root) == 0, "group merged unequal roots");
      if (e.direction > 0) ++entering;
      else ++leaving;
    }
    require(found_representative && current_depth >= leaving, "invalid representative/depth");
    current_depth -= leaving;
    require(group.depth == current_depth, "strict event depth mismatch");
    current_depth += entering;
    ++callbacks;
    root_ids += group.root_ids.size();
    group_max = std::max(group_max, static_cast<u64>(group.root_ids.size()));
    depth_sum += group.depth;
    hash_word(output_hash, group.depth);
    hash_word(output_hash, group.root_ids.size());
    for (auto id : group.root_ids) hash_word(output_hash, id);
  });
  const auto processed = Clock::now();
  for (std::size_t id = 0; id < n; ++id)
    require((visited[id] != 0) == (scalar(input.points[id]).direction != 0), "event coverage mismatch");
  require(work.sites == n && work.entries == entries && work.exits == exits &&
          work.constant_inside == inside && work.constant_on == constant_shell.size() &&
          work.constant_outside == outside && work.event_count == entries + exits &&
          work.groups == callbacks && work.callbacks == callbacks && work.max_group == group_max &&
          root_ids == entries + exits && current_depth == inside + entries,
          "work ledger mismatch");
  require(family == "coplanar" ? work.event_count == 0 : (entries > 0 && exits > 0 && callbacks > 0),
          "fixture event non-vacuity mismatch");
  const auto cloud_work = cloud->work();
  const auto cloud_bytes = cloud->retained_bytes();
  const auto input_bytes = input.points.capacity() * sizeof(Point3);
  const auto judge_bytes = visited.capacity() * sizeof(unsigned char) + constant_shell.capacity() * sizeof(std::size_t);
  const auto constant_shell_size = constant_shell.size();
  const auto validated = Clock::now();
  cloud.reset();
  input.points.clear();
  input.points.shrink_to_fit();
  std::vector<unsigned char>().swap(visited);
  std::vector<std::size_t>().swap(constant_shell);
  const auto done = Clock::now();
  std::cout << std::setprecision(17)
      << "{\"schema\":\"mhgp8_q4_family_probe_v1\",\"status\":\"completed\","
      << "\"scope\":\"one_q4_family_all_sites_not_q4_producer\",\"public_status\":\"not_claimed\","
      << "\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\","
      << "\"timing_scope\":\"generation_owner_family_callback_validation_release_excludes_json\","
      << "\"n\":" << n << ",\"family\":\"" << family << "\",\"seed\":3,\"seed_ids\":[0,1,2],"
      << "\"threads\":1,\"input_hash\":" << input.hash << ",\"generation\":{\"proposals\":" << input.proposals
      << ",\"duplicates\":" << input.duplicates << ",\"random_calls\":" << input.random_calls << "},"
      << "\"work\":{\"sites\":" << work.sites << ",\"entries\":" << work.entries << ",\"exits\":" << work.exits
      << ",\"constant_inside\":" << work.constant_inside << ",\"constant_on\":" << work.constant_on
      << ",\"constant_outside\":" << work.constant_outside << ",\"sort_comparisons\":" << work.sort_comparisons
      << ",\"group_comparisons\":" << work.group_comparisons << ",\"groups\":" << work.groups
      << ",\"max_group\":" << work.max_group << ",\"callbacks\":" << work.callbacks
      << ",\"event_count\":" << work.event_count << ",\"retained_capacity_bytes\":" << work.retained_capacity_bytes
      << "},\"digest\":{\"groups\":" << callbacks << ",\"root_ids\":" << root_ids
      << ",\"constant_shell\":" << constant_shell_size << ",\"depth_sum\":" << depth_sum
      << ",\"hash\":" << output_hash << ",\"callback_root_ids_visited\":" << root_ids
      << ",\"callback_constant_ids_visited\":" << callback_constant_ids
      << "},\"cloud_work\":{\"copies\":" << cloud_work.coordinate_copies
      << ",\"validation_points\":" << cloud_work.validation_points
      << ",\"uniqueness_comparisons\":" << cloud_work.uniqueness_comparisons
      << ",\"range_tree_nodes\":" << cloud_work.range_tree_nodes << "},"
      << "\"memory\":{\"input_capacity_bytes\":" << input_bytes << ",\"cloud_retained_bytes\":" << cloud_bytes
      << ",\"judge_capacity_bytes\":" << judge_bytes
      << ",\"event_id_bytes\":" << sizeof(std::size_t)
      << ",\"scope\":\"retained_capacities_not_RSS\"},\"timings\":{\"generation_ms\":" << ms(start, generated)
      << ",\"cloud_ms\":" << ms(generated, prepared) << ",\"oracle_prepare_ms\":" << ms(prepared, oracle_prepared)
      << ",\"family_with_validation_callback_ms\":" << ms(oracle_prepared, processed)
      << ",\"validation_ms\":" << ms(processed, validated) << ",\"release_ms\":" << ms(validated, done)
      << ",\"total_ms\":" << ms(start, done) << "}}\n";
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 3) throw std::invalid_argument("usage: mhgp8_q4_family_probe n uniform|rows|coplanar");
    const auto n = number(argv[1]);
    const std::string_view family(argv[2]);
    if (n < 8 || (family != "uniform" && family != "rows" && family != "coplanar"))
      throw std::invalid_argument("invalid fixture size or family");
    if (family == "rows" && n > 129075)
      throw std::invalid_argument("two-row recipe exceeds the finite u16 y domain");
    if (n > (family == "coplanar" ? (u64{1} << 32U) : (u64{1} << 48U)))
      throw std::invalid_argument("fixture cardinality exceeds the finite u16 domain");
    return run(n, family);
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
