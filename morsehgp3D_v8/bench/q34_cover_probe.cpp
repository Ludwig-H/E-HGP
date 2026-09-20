#include "lanes/q34_cover.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

namespace {
using namespace mhgp8;
using Clock = std::chrono::steady_clock;
__extension__ typedef unsigned __int128 u128;
void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
std::size_t number(std::string_view text) {
  std::size_t result{};
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
  if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
    throw std::invalid_argument("expected unsigned decimal integer");
  return result;
}
double ms(Clock::time_point first, Clock::time_point last) {
  return std::chrono::duration<double, std::milli>(last - first).count();
}
void word(u64& hash, u64 value) {
  for (unsigned i = 0; i != 8; ++i) {
    hash = (hash ^ (value & 255U)) * 1099511628211ULL;
    value >>= 8U;
  }
}
u64 random_word(u64& state) {
  auto v = (state += 0x9e3779b97f4a7c15ULL);
  v = (v ^ (v >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  v = (v ^ (v >> 27U)) * 0x94d049bb133111ebULL;
  return v ^ (v >> 31U);
}
struct Input {
  std::vector<Point3> points;
  u64 proposals{}, duplicates{}, random_calls{};
  u64 hash{14695981039346656037ULL};
};
Input make_input(std::size_t n, std::string_view regime) {
  Input input{{{900, 1000, 1000}, {1100, 1000, 1000}}};
  input.points.reserve(n);
  if (regime != "adversarial") {
    for (auto p : {Point3{1000, 1120, 1040}, Point3{1000, 1120, 960},
                   Point3{1000, 1025, 1000}, Point3{1001, 1025, 1000}})
      input.points.push_back(p);
  }
  std::set<u64> seen;
  u64 state = 3;
  while (input.points.size() < n) {
    const auto i = input.points.size() - (regime == "adversarial" ? 2U : 6U);
    Point3 p;
    if (regime == "far") {
      p = {static_cast<std::uint16_t>(40000 + (random_word(state) & 16383U)),
           static_cast<std::uint16_t>(40000 + (random_word(state) & 16383U)),
           static_cast<std::uint16_t>(40000 + (random_word(state) & 16383U))};
      input.random_calls += 3;
    } else if (regime == "cap") {
      p = {static_cast<std::uint16_t>(1150 + i / (41 * 41)),
           static_cast<std::uint16_t>(980 + (i / 41) % 41),
           static_cast<std::uint16_t>(980 + i % 41)};
    } else {
      const auto y = 1120 + (i / 2) / 16;
      const auto offset = 40 + (i / 2) % 16;
      p = {1000, static_cast<std::uint16_t>(y),
           static_cast<std::uint16_t>(i % 2 == 0 ? 1000 + offset : 1000 - offset)};
    }
    ++input.proposals;
    const auto key = static_cast<u64>(p.x) | (static_cast<u64>(p.y) << 16U) |
                     (static_cast<u64>(p.z) << 32U);
    if (!seen.insert(key).second) { ++input.duplicates; continue; }
    input.points.push_back(p);
  }
  word(input.hash, n);
  for (auto p : input.points)
    for (unsigned axis = 0; axis != 3; ++axis) word(input.hash, p[axis]);
  return input;
}

template<class T> struct Field { const char* name; u64 T::* member; };
#define F(type, member) Field<type>{#member, &type::member}
constexpr std::array seed_fields{
  F(Q34SeedWork, seed_owner_tests), F(Q34SeedWork, seed_owner_rejections),
  F(Q34SeedWork, q3_point_tests), F(Q34SeedWork, q3_shell_ids),
  F(Q34SeedWork, q3_depth_rejections), F(Q34SeedWork, q3_emitted),
  F(Q34SeedWork, q3_shell_capacity_bytes), F(Q34SeedWork, q4_depth_rejected_groups),
  F(Q34SeedWork, q4_depth_skipped_ids), F(Q34SeedWork, q4_presentations),
  F(Q34SeedWork, q4_owner_tests), F(Q34SeedWork, q4_owner_rejections),
  F(Q34SeedWork, q4_positive_tests), F(Q34SeedWork, q4_positive_rejections),
  F(Q34SeedWork, q4_seed_tests), F(Q34SeedWork, q4_seed_rejections),
  F(Q34SeedWork, q4_groups_without_support), F(Q34SeedWork, q4_unexamined_after_emit),
  F(Q34SeedWork, q4_emitted)};
constexpr std::array family_fields{
  F(Q4FamilyWork, sites), F(Q4FamilyWork, entries), F(Q4FamilyWork, exits),
  F(Q4FamilyWork, constant_inside), F(Q4FamilyWork, constant_on), F(Q4FamilyWork, constant_outside),
  F(Q4FamilyWork, sort_comparisons), F(Q4FamilyWork, group_comparisons),
  F(Q4FamilyWork, groups), F(Q4FamilyWork, max_group), F(Q4FamilyWork, callbacks),
  F(Q4FamilyWork, event_count), F(Q4FamilyWork, retained_capacity_bytes)};
constexpr std::array cover_fields{
  F(Q34EdgeCoverWork, node_visits), F(Q34EdgeCoverWork, bound_tests), F(Q34EdgeCoverWork, point_tests),
  F(Q34EdgeCoverWork, admitted_nodes), F(Q34EdgeCoverWork, rejected_nodes), F(Q34EdgeCoverWork, split_nodes),
  F(Q34EdgeCoverWork, admitted_sites), F(Q34EdgeCoverWork, rejected_sites),
  F(Q34EdgeCoverWork, retained_ranges), F(Q34EdgeCoverWork, merged_ranges)};
constexpr std::array edge_fields{
  F(Q34EdgeWork, node_visits), F(Q34EdgeWork, bound_tests), F(Q34EdgeWork, point_tests),
  F(Q34EdgeWork, rejected_nodes), F(Q34EdgeWork, split_nodes), F(Q34EdgeWork, rejected_sites),
  F(Q34EdgeWork, acute_seeds), F(Q34EdgeWork, owner_tests), F(Q34EdgeWork, owner_rejections), F(Q34EdgeWork, seeds)};
constexpr std::array covered_fields{
  F(Q34CoverSeedWork, site_reads), F(Q34CoverSeedWork, q3_shell_sort_comparisons),
  F(Q34CoverSeedWork, q4_shell_sort_comparisons), F(Q34CoverSeedWork, peak_buffer_bytes)};
constexpr std::array cloud_fields{
  F(CloudWork, coordinate_copies), F(CloudWork, validation_points), F(CloudWork, uniqueness_comparisons),
  F(CloudWork, uniqueness_adjacent_tests), F(CloudWork, range_tree_leaf_visits),
  F(CloudWork, range_tree_nodes), F(CloudWork, range_tree_merges)};
constexpr std::array index_fields{
  F(Q2IndexWork, point_visits), F(Q2IndexWork, nodes), F(Q2IndexWork, max_depth), F(Q2IndexWork, escape_links)};
#undef F
static_assert(sizeof(Q34SeedWork) == seed_fields.size() * sizeof(u64) + sizeof(Q4FamilyWork));
static_assert(sizeof(Q4FamilyWork) == family_fields.size() * sizeof(u64));
static_assert(sizeof(Q34EdgeCoverWork) == cover_fields.size() * sizeof(u64));
static_assert(sizeof(Q34EdgeWork) == edge_fields.size() * sizeof(u64) + sizeof(Q34CoverSeedWork));
static_assert(sizeof(Q34CoverSeedWork) == covered_fields.size() * sizeof(u64) + sizeof(Q34SeedWork));
static_assert(sizeof(CloudWork) == cloud_fields.size() * sizeof(u64));
static_assert(sizeof(Q2IndexWork) == index_fields.size() * sizeof(u64));
template<class T, std::size_t N>
void dump_fields(const T& value, const std::array<Field<T>, N>& fields) {
  bool first = true;
  for (const auto field : fields) {
    if (!first) std::cout << ',';
    first = false;
    std::cout << '"' << field.name << "\":" << value.*(field.member);
  }
}
void dump_seed(const Q34SeedWork& work) {
  std::cout << '{'; dump_fields(work, seed_fields);
  std::cout << ",\"family\":{"; dump_fields(work.family, family_fields); std::cout << "}}";
}
void aggregate(Q34SeedWork& dst, const Q34SeedWork& src) {
  for (const auto f : seed_fields) {
    if (f.member == &Q34SeedWork::q3_shell_capacity_bytes)
      dst.*(f.member) = std::max(dst.*(f.member), src.*(f.member));
    else counter_add(dst.*(f.member), src.*(f.member));
  }
  for (const auto f : family_fields) {
    if (f.member == &Q4FamilyWork::max_group || f.member == &Q4FamilyWork::retained_capacity_bytes)
      dst.family.*(f.member) = std::max(dst.family.*(f.member), src.family.*(f.member));
    else counter_add(dst.family.*(f.member), src.family.*(f.member));
  }
}

struct Record {
  unsigned arity{};
  std::array<std::size_t, 4> support{};
  std::array<i128, 5> coefficients{};
  std::size_t depth{};
  std::vector<std::size_t> shell;
  bool operator==(const Record&) const = default;
  bool operator<(const Record& other) const {
    return std::tie(arity, support, coefficients, depth, shell) <
           std::tie(other.arity, other.support, other.coefficients, other.depth, other.shell);
  }
};
struct Output {
  std::vector<Record> records;
  u64 callbacks{}, q3{}, q4{}, support_ids_visited{}, shell_ids_visited{};
  u64 hash{14695981039346656037ULL};
  void collect(const Q34SeedCandidate& candidate, std::size_t n, std::size_t kmax) {
    require(candidate.arity == 3 || candidate.arity == 4, "invalid arity");
    require(candidate.depth < kmax + 2 - candidate.arity, "invalid emitted depth");
    Record record{candidate.arity, candidate.support_ids, candidate.ball.coefficients(), candidate.depth, {}};
    require(std::is_sorted(record.support.begin(), record.support.begin() + record.arity), "unordered support");
    for (unsigned i = 0; i < record.arity; ++i) {
      require(record.support[i] < n && (i == 0 || record.support[i - 1] < record.support[i]), "invalid support IDs");
      ++support_ids_visited;
    }
    if (record.arity == 3) record.support[3] = std::numeric_limits<std::size_t>::max();
    record.shell.reserve(candidate.shell_first.size() + candidate.shell_second.size());
    for (auto part : {candidate.shell_first, candidate.shell_second}) {
      require(std::is_sorted(part.begin(), part.end()), "unordered original-ID shell part");
      for (auto id : part) {
        require(id < n, "invalid shell ID");
        record.shell.push_back(id);
        ++shell_ids_visited;
      }
    }
    std::sort(record.shell.begin(), record.shell.end());
    require(std::adjacent_find(record.shell.begin(), record.shell.end()) == record.shell.end(), "duplicated shell ID");
    for (unsigned i = 0; i < record.arity; ++i)
      require(std::binary_search(record.shell.begin(), record.shell.end(), record.support[i]), "missing support from shell");
    ++callbacks;
    if (record.arity == 3) ++q3; else ++q4;
    records.push_back(std::move(record));
  }
  void finish() {
    std::sort(records.begin(), records.end());
    require(std::adjacent_find(records.begin(), records.end()) == records.end(), "duplicated candidate");
    for (const auto& r : records) {
      word(hash, r.arity); word(hash, r.depth);
      for (auto coefficient : r.coefficients) {
        const auto bits = static_cast<u128>(coefficient);
        word(hash, static_cast<u64>(bits)); word(hash, static_cast<u64>(bits >> 64U));
      }
      for (unsigned i = 0; i < r.arity; ++i) word(hash, r.support[i]);
      word(hash, r.shell.size());
      for (auto id : r.shell) word(hash, id);
    }
  }
  std::size_t retained_bytes() const {
    auto bytes = records.capacity() * sizeof(Record);
    for (const auto& r : records) bytes += r.shell.capacity() * sizeof(std::size_t);
    return bytes;
  }
  void release() { std::vector<Record>().swap(records); }
  void dump() const {
    std::cout << "{\"callbacks\":" << callbacks << ",\"q3\":" << q3 << ",\"q4\":" << q4
              << ",\"support_ids_visited\":" << support_ids_visited << ",\"shell_ids_visited\":" << shell_ids_visited
              << ",\"hash\":" << hash << '}';
  }
};
std::vector<Record> expected() {
  const auto absent = std::numeric_limits<std::size_t>::max();
  return {{3, {0, 1, 2, absent}, {1, -2000, -2045, -2015, 3050000}, 2, {0, 1, 2}},
          {3, {0, 1, 3, absent}, {1, -2000, -2045, -1985, 3020000}, 2, {0, 1, 3}},
          {4, {0, 1, 2, 3}, {1, -2000, -2050, -2000, 3040000}, 2, {0, 1, 2, 3}}};
}

int run(std::size_t n, std::string_view regime, std::size_t kmax) {
  const auto start = Clock::now();
  auto input = make_input(n, regime);
  const auto generated = Clock::now();
  const auto seeds = regime == "adversarial" ? n - 2 : 2;
  const auto wanted_cover = regime == "far" ? 6 : n;
  u64 fixture_point_tests = 0, oracle_point_tests = 0;
  // Independent scalar certificates, once per point, never once per root.
  for (std::size_t id = 0; id < n; ++id) {
    const auto p = input.points[id];
    const i64 x = static_cast<i64>(p.x) - 1000, y = static_cast<i64>(p.y) - 1000, z = static_cast<i64>(p.z) - 1000;
    const bool in_cover = x * x + y * y + z * z <= 40000;
    const bool owned_seed = x > -100 && x < 100 && x * x + y * y + z * z > 10000 &&
        (x + 100) * (x + 100) + y * y + z * z <= 40000 &&
        (x - 100) * (x - 100) + y * y + z * z <= 40000;
    require(in_cover == (regime != "far" || id < 6), "fixture cover certificate failed");
    require(owned_seed == (id >= 2 && id < 2 + seeds), "fixture seed certificate failed");
    ++fixture_point_tests;
  }
  if (regime != "adversarial") {
    const auto wanted = expected();
    for (const auto& ball : wanted)
      for (std::size_t id = 0; id < n; ++id) {
        const auto p = input.points[id];
        const i128 norm = static_cast<i128>(p.x) * p.x + static_cast<i128>(p.y) * p.y + static_cast<i128>(p.z) * p.z;
        const auto& c = ball.coefficients;
        const i128 power = c[0] * norm + c[1] * p.x + c[2] * p.y + c[3] * p.z + c[4];
        require((power < 0) == (id == 4 || id == 5) &&
                (power == 0) == std::binary_search(ball.shell.begin(), ball.shell.end(), id), "independent ball census failed");
        ++oracle_point_tests;
      }
  }
  const auto certified = Clock::now();
  auto old_cloud = prepare_cloud(input.points);
  const auto old_prepared = Clock::now();
  Output old_output;
  Q34SeedWork old_work;
  for (std::size_t x = 2; x != 2 + seeds; ++x)
    aggregate(old_work, run_q34_seed_candidates(old_cloud, {0, 1, x}, kmax,
        [&](const auto& candidate) { old_output.collect(candidate, n, kmax); }));
  const auto old_processed = Clock::now();
  old_output.finish();
  if (regime != "adversarial") require(old_output.records == expected(), "old output differs from independent fixture");
  const auto old_validated = Clock::now();
  const auto old_cloud_work = old_cloud->work();
  const auto old_cloud_bytes = old_cloud->retained_bytes();
  old_cloud.reset();
  const auto old_released = Clock::now();
  auto cloud = prepare_cloud(input.points);
  const auto prepared = Clock::now();
  auto index = make_q2_cloud_index(cloud);
  const auto indexed = Clock::now();
  auto cover = Q34EdgeCover::make(index, {0, 1});
  const auto covered = Clock::now();
  Output output;
  const auto work = run_q34_edge_candidates(cover, kmax,
      [&](const auto& candidate) { output.collect(candidate, n, kmax); });
  const auto processed = Clock::now();
  output.finish();
  require(output.records == old_output.records && output.hash == old_output.hash, "old/new candidate mismatch");
  require(work.seeds == seeds && cover->site_count() == wanted_cover &&
          work.covered.site_reads == seeds * wanted_cover && old_work.family.sites == seeds * n &&
          work.covered.seed.family.sites == seeds * wanted_cover, "seed/cover work mismatch");
  require(output.q3 == work.covered.seed.q3_emitted && output.q4 == work.covered.seed.q4_emitted &&
          old_output.q3 == old_work.q3_emitted && old_output.q4 == old_work.q4_emitted, "output ledger mismatch");
  const auto cloud_work = cloud->work();
  const auto index_work = index->work();
  const auto cover_work = cover->work();
  const auto input_bytes = input.points.capacity() * sizeof(Point3);
  const auto cloud_bytes = cloud->retained_bytes(), index_bytes = index->retained_bytes(), cover_bytes = cover->retained_bytes();
  const auto old_output_bytes = old_output.retained_bytes(), output_bytes = output.retained_bytes();
  const auto validated = Clock::now();
  cover.reset(); index.reset(); cloud.reset();
  const auto released = Clock::now();
  output.release(); old_output.release(); std::vector<Point3>().swap(input.points);
  const auto done = Clock::now();
  std::cout << std::setprecision(17)
      << "{\"schema\":\"mhgp8_q34_cover_probe_v1\",\"status\":\"completed\","
      << "\"scope\":\"one_edge_not_global_q34_producer\",\"public_status\":\"not_claimed\","
      << "\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\",\"threads\":1,\"seed\":3,"
      << "\"recipe\":\"owned_edge_far_cap_dense_v1\",\"edge_ids\":[0,1],"
      << "\"timing_scope\":\"paired_generation_fixture_owners_index_cover_callbacks_validation_release_excludes_json\","
      << "\"n\":" << n << ",\"regime\":\"" << regime << "\",\"kmax\":" << kmax
      << ",\"input_hash\":" << input.hash << ",\"provided_old_seeds\":" << seeds << ",\"cover_sites\":" << wanted_cover
      << ",\"generation\":{\"proposals\":" << input.proposals << ",\"duplicates\":" << input.duplicates << ",\"random_calls\":" << input.random_calls
      << "},\"validation\":{\"fixture_point_tests\":" << fixture_point_tests << ",\"independent_ball_point_tests\":" << oracle_point_tests
      << ",\"method\":\"" << (regime == "adversarial" ? "qualified_old_path_differential" : "three_closed_form_balls_plus_differential") << "\"}"
      << ",\"old_work\":"; dump_seed(old_work);
  std::cout << ",\"edge_work\":{"; dump_fields(work, edge_fields);
  std::cout << ",\"covered\":{"; dump_fields(work.covered, covered_fields);
  std::cout << ",\"seed\":"; dump_seed(work.covered.seed); std::cout << "}}";
  std::cout << ",\"cover_work\":{"; dump_fields(cover_work, cover_fields); std::cout << '}';
  std::cout << ",\"index_work\":{"; dump_fields(index_work, index_fields); std::cout << '}';
  std::cout << ",\"old_cloud_work\":{"; dump_fields(old_cloud_work, cloud_fields); std::cout << '}';
  std::cout << ",\"cloud_work\":{"; dump_fields(cloud_work, cloud_fields); std::cout << '}';
  std::cout << ",\"old_digest\":"; old_output.dump(); std::cout << ",\"digest\":"; output.dump();
  std::cout << ",\"memory\":{\"scope\":\"retained_capacities_not_RSS_shared_old_output_persists_for_comparison\","
      << "\"id_bytes\":" << sizeof(std::size_t) << ",\"input_capacity_bytes\":" << input_bytes
      << ",\"old_cloud_retained_bytes\":" << old_cloud_bytes << ",\"cloud_retained_bytes\":" << cloud_bytes
      << ",\"index_retained_bytes\":" << index_bytes << ",\"cover_retained_bytes\":" << cover_bytes
      << ",\"old_output_retained_bytes\":" << old_output_bytes << ",\"output_retained_bytes\":" << output_bytes << '}'
      << ",\"timings\":{\"generation_ms\":" << ms(start, generated) << ",\"fixture_validation_ms\":" << ms(generated, certified)
      << ",\"old_cloud_ms\":" << ms(certified, old_prepared) << ",\"old_run_callback_ms\":" << ms(old_prepared, old_processed)
      << ",\"old_validation_ms\":" << ms(old_processed, old_validated) << ",\"old_owner_release_ms\":" << ms(old_validated, old_released)
      << ",\"cloud_ms\":" << ms(old_released, prepared) << ",\"index_ms\":" << ms(prepared, indexed)
      << ",\"cover_ms\":" << ms(indexed, covered) << ",\"run_callback_ms\":" << ms(covered, processed)
      << ",\"validation_ms\":" << ms(processed, validated) << ",\"owner_release_ms\":" << ms(validated, released)
      << ",\"shared_release_ms\":" << ms(released, done)
      << ",\"old_arm_ms\":" << ms(certified, old_released) << ",\"new_arm_ms\":" << ms(old_released, released)
      << ",\"total_ms\":" << ms(start, done) << "}}\n";
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 4) throw std::invalid_argument("usage: mhgp8_q34_cover_probe n far|cap|adversarial K5_or_10");
    const auto n = number(argv[1]), k = number(argv[3]);
    const std::string_view regime(argv[2]);
    if ((regime != "far" && regime != "cap" && regime != "adversarial") || n < 8 || (k != 5 && k != 10) ||
        (regime == "cap" && n > 35307) || (regime == "adversarial" && n > 514))
      throw std::invalid_argument("outside distinct-coordinate fixture domain; not an algorithm quota");
    return run(n, regime, k);
  } catch (const std::exception& error) {
    std::cerr << "q34 cover probe: " << error.what() << '\n';
    return 1;
  }
}
