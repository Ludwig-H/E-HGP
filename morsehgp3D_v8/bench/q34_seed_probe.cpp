#include "lanes/q34_seed.hpp"

#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using mhgp8::i64;
using mhgp8::i128;
using mhgp8::Point3;
using mhgp8::u64;
using Clock = std::chrono::steady_clock;
constexpr std::array<i128, 5> q3_key{1, -2000, -2045, -2015, 3050000};
constexpr std::array<i128, 5> q4_key{1, -2000, -2050, -2000, 3040000};

void require(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}
std::size_t number(std::string_view text) {
  std::size_t value{};
  const auto r = std::from_chars(text.data(), text.data() + text.size(), value);
  if (r.ec != std::errc{} || r.ptr != text.data() + text.size())
    throw std::invalid_argument("expected unsigned decimal integer");
  return value;
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
struct Input {
  std::vector<Point3> points;
  u64 proposals{}, duplicates{}, random_calls{};
  u64 hash{14695981039346656037ULL};
};
Input make_input(std::size_t n, std::string_view family) {
  // Positive local seed/tetrahedron, then two strict interiors common to
  // their balls. IDs 0/1 own both presentations; seed ID2 is canonical.
  Input input{{{900, 1000, 1000}, {1100, 1000, 1000}, {1000, 1120, 1040},
               {1000, 1120, 960}, {1000, 1025, 1000}, {1001, 1025, 1000}}};
  input.points.reserve(n);
  std::set<u64> seen;
  u64 state = 3;
  while (input.points.size() < n) {
    const auto i = input.points.size() - 6;
    Point3 point;
    if (family == "uniform") {
      for (unsigned axis = 0; axis < 3; ++axis) {
        const auto coordinate = static_cast<std::uint16_t>(40000 + (random_word(state) & 16383U));
        if (axis == 0) point.x = coordinate;
        else if (axis == 1) point.y = coordinate;
        else point.z = coordinate;
        ++input.random_calls;
      }
    } else if (family == "rows") {
      point = {static_cast<std::uint16_t>(i % 2 == 0 ? 45000 : 50000),
               static_cast<std::uint16_t>(40000 + i / 2),
               static_cast<std::uint16_t>(i % 2 == 0 ? 50000 : 55000)};
    } else {
      const auto z = 16000 + i / 256;
      point = {static_cast<std::uint16_t>(40000 + i % 256),
               static_cast<std::uint16_t>(3 * z - 2000), static_cast<std::uint16_t>(z)};
    }
    ++input.proposals;
    const auto key = static_cast<u64>(point.x) | (static_cast<u64>(point.y) << 16U) |
                     (static_cast<u64>(point.z) << 32U);
    if (!seen.insert(key).second) { ++input.duplicates; continue; }
    input.points.push_back(point);
  }
  hash_word(input.hash, n);
  for (auto point : input.points)
    for (unsigned axis = 0; axis < 3; ++axis) hash_word(input.hash, point[axis]);
  return input;
}

// Closed-form circle/sphere equations, independent of ExactBall factories.
// Both have exactly IDs4/5 inside, the stated local shell and every remote
// point outside. Remote x>=40000 also rules out every other positive q4
// completion: a family centre has x=1000 whereas such a site's barycentric
// weight would exceed 1/2. Coplanar remotes instead give flat tetrahedra.
i64 scalar_power(Point3 point, bool tetrahedron) {
  const i64 x = point.x, y = point.y, z = point.z;
  return x * x + y * y + z * z - 2000 * x - (tetrahedron ? 2050 : 2045) * y
       - (tetrahedron ? 2000 : 2015) * z + (tetrahedron ? 3040000 : 3050000);
}
struct WorkField { const char* name; u64 mhgp8::Q34SeedWork::* member; };
constexpr WorkField fields[]{
  {"seed_owner_tests", &mhgp8::Q34SeedWork::seed_owner_tests},
  {"seed_owner_rejections", &mhgp8::Q34SeedWork::seed_owner_rejections},
  {"q3_point_tests", &mhgp8::Q34SeedWork::q3_point_tests},
  {"q3_shell_ids", &mhgp8::Q34SeedWork::q3_shell_ids},
  {"q3_depth_rejections", &mhgp8::Q34SeedWork::q3_depth_rejections},
  {"q3_emitted", &mhgp8::Q34SeedWork::q3_emitted},
  {"q3_shell_capacity_bytes", &mhgp8::Q34SeedWork::q3_shell_capacity_bytes},
  {"q4_depth_rejected_groups", &mhgp8::Q34SeedWork::q4_depth_rejected_groups},
  {"q4_depth_skipped_ids", &mhgp8::Q34SeedWork::q4_depth_skipped_ids},
  {"q4_presentations", &mhgp8::Q34SeedWork::q4_presentations},
  {"q4_owner_tests", &mhgp8::Q34SeedWork::q4_owner_tests},
  {"q4_owner_rejections", &mhgp8::Q34SeedWork::q4_owner_rejections},
  {"q4_positive_tests", &mhgp8::Q34SeedWork::q4_positive_tests},
  {"q4_positive_rejections", &mhgp8::Q34SeedWork::q4_positive_rejections},
  {"q4_seed_tests", &mhgp8::Q34SeedWork::q4_seed_tests},
  {"q4_seed_rejections", &mhgp8::Q34SeedWork::q4_seed_rejections},
  {"q4_groups_without_support", &mhgp8::Q34SeedWork::q4_groups_without_support},
  {"q4_unexamined_after_emit", &mhgp8::Q34SeedWork::q4_unexamined_after_emit},
  {"q4_emitted", &mhgp8::Q34SeedWork::q4_emitted}};
struct FamilyField { const char* name; u64 mhgp8::Q4FamilyWork::* member; };
constexpr FamilyField family_fields[]{
  {"sites", &mhgp8::Q4FamilyWork::sites}, {"entries", &mhgp8::Q4FamilyWork::entries},
  {"exits", &mhgp8::Q4FamilyWork::exits}, {"constant_inside", &mhgp8::Q4FamilyWork::constant_inside},
  {"constant_on", &mhgp8::Q4FamilyWork::constant_on}, {"constant_outside", &mhgp8::Q4FamilyWork::constant_outside},
  {"sort_comparisons", &mhgp8::Q4FamilyWork::sort_comparisons},
  {"group_comparisons", &mhgp8::Q4FamilyWork::group_comparisons},
  {"groups", &mhgp8::Q4FamilyWork::groups}, {"max_group", &mhgp8::Q4FamilyWork::max_group},
  {"callbacks", &mhgp8::Q4FamilyWork::callbacks}, {"event_count", &mhgp8::Q4FamilyWork::event_count},
  {"retained_capacity_bytes", &mhgp8::Q4FamilyWork::retained_capacity_bytes}};
static_assert(sizeof(mhgp8::Q34SeedWork) == std::size(fields) * sizeof(u64) + sizeof(mhgp8::Q4FamilyWork));
static_assert(sizeof(mhgp8::Q4FamilyWork) == std::size(family_fields) * sizeof(u64));

int run(std::size_t n, std::string_view family, std::size_t kmax) {
  const auto start = Clock::now();
  auto input = make_input(n, family);
  const auto generated = Clock::now();
  auto cloud = mhgp8::prepare_cloud(input.points);
  const auto prepared = Clock::now();
  for (std::size_t id = 0; id < n; ++id) {
    for (const bool q4 : {false, true}) {
      const auto p = scalar_power(input.points[id], q4);
      const bool shell = id < (q4 ? 4U : 3U), interior = id == 4 || id == 5;
      require((p == 0) == shell && (p < 0) == interior, "closed-form fixture classification failed");
    }
    if (id >= 6) require(input.points[id].x >= 40000, "remote completion proof violated");
  }
  const auto judged = Clock::now();
  u64 callbacks = 0, support_reads = 0, shell_reads = 0;
  u64 hash = 14695981039346656037ULL;
  std::array<std::array<i64, 5>, 2> actual_keys{};
  const auto work = mhgp8::run_q34_seed_candidates(cloud, {0, 1, 2}, kmax,
      [&](const mhgp8::Q34SeedCandidate& candidate) {
    require(callbacks < 2 && candidate.arity == 3 + callbacks && candidate.depth == 2,
            "missing, extra or wrong-depth candidate");
    const auto& expected = candidate.arity == 3 ? q3_key : q4_key;
    require(candidate.ball.coefficients() == expected, "canonical ball differs from closed form");
    hash_word(hash, candidate.arity);
    hash_word(hash, candidate.depth);
    for (std::size_t i = 0; i < expected.size(); ++i) {
      const auto value = static_cast<i64>(candidate.ball.coefficients()[i]);
      actual_keys[callbacks][i] = value;
      hash_word(hash, static_cast<u64>(value));
    }
    for (unsigned i = 0; i < candidate.arity; ++i) {
      require(candidate.support_ids[i] == i, "noncanonical local support");
      hash_word(hash, candidate.support_ids[i]);
      ++support_reads;
    }
    if (candidate.arity == 3) {
      require(candidate.shell_first.size() == 3 && candidate.shell_second.empty(), "q3 shell cardinality");
    } else {
      require(candidate.shell_first.size() == 1 && candidate.shell_first[0] == 3 &&
              candidate.shell_second.size() == 3, "q4 shell cardinality/identity");
    }
    for (auto shell : {candidate.shell_first, candidate.shell_second}) {
      hash_word(hash, shell.size());
      for (std::size_t i = 0; i < shell.size(); ++i) {
        const auto expected_id = candidate.arity == 4 && shell.size() == 1 ? 3 : i;
        require(shell[i] == expected_id, "shell identity mismatch");
        hash_word(hash, shell[i]);
        ++shell_reads;
      }
    }
    ++callbacks;
  });
  const auto processed = Clock::now();
  require(callbacks == 2 && support_reads == 7 && shell_reads == 7 &&
          work.q3_emitted == 1 && work.q4_emitted == 1 && work.q3_point_tests == n &&
          work.q3_depth_rejections == 0 && work.family.sites == n,
          "incomplete seed processing");
  const auto input_bytes = input.points.capacity() * sizeof(Point3);
  const auto cloud_bytes = cloud->retained_bytes();
  const auto cloud_work = cloud->work();
  const auto validated = Clock::now();
  cloud.reset();
  std::vector<Point3>().swap(input.points);
  const auto done = Clock::now();
  std::cout << std::setprecision(17)
      << "{\"schema\":\"mhgp8_q34_seed_probe_v1\",\"status\":\"completed\","
      << "\"scope\":\"one_seed_positive_presentations_not_q34_producer\",\"public_status\":\"not_claimed\","
      << "\"backend\":\"cpu_reference\",\"profile\":\"quantized_u16_input_only\","
      << "\"timing_scope\":\"generation_owner_seed_callback_validation_release_excludes_json\","
      << "\"n\":" << n << ",\"family\":\"" << family << "\",\"recipe\":\"local_positive_remote_v1\","
      << "\"kmax\":" << kmax << ",\"seed\":3,\"seed_ids\":[0,1,2],\"threads\":1,\"input_hash\":" << input.hash
      << ",\"generation\":{\"proposals\":" << input.proposals << ",\"duplicates\":" << input.duplicates
      << ",\"random_calls\":" << input.random_calls << "},\"work\":{";
  bool first = true;
  for (const auto& field : fields) {
    if (!first) std::cout << ',';
    first = false;
    std::cout << '"' << field.name << "\":" << work.*(field.member);
  }
  std::cout << ",\"family\":{";
  first = true;
  for (const auto& field : family_fields) {
    if (!first) std::cout << ',';
    first = false;
    std::cout << '"' << field.name << "\":" << work.family.*(field.member);
  }
  std::cout << "}},\"digest\":{\"callbacks\":" << callbacks << ",\"support_ids_visited\":" << support_reads
      << ",\"shell_ids_visited\":" << shell_reads << ",\"depths\":[2,2],\"ball_coefficients\":[";
  for (std::size_t b = 0; b < actual_keys.size(); ++b) {
    if (b != 0) std::cout << ',';
    std::cout << '[';
    for (std::size_t i = 0; i < actual_keys[b].size(); ++i) {
      if (i != 0) std::cout << ',';
      std::cout << actual_keys[b][i];
    }
    std::cout << ']';
  }
  std::cout << "],\"hash\":" << hash << "},\"oracle\":{\"scalar_point_tests\":" << 2 * n
      << ",\"remote_certificate_tests\":" << n - 6 << "},\"cloud_work\":{\"copies\":" << cloud_work.coordinate_copies
      << ",\"validation_points\":" << cloud_work.validation_points
      << ",\"uniqueness_comparisons\":" << cloud_work.uniqueness_comparisons
      << ",\"range_tree_nodes\":" << cloud_work.range_tree_nodes << "},\"memory\":{\"input_capacity_bytes\":" << input_bytes
      << ",\"cloud_retained_bytes\":" << cloud_bytes << ",\"event_id_bytes\":" << sizeof(std::size_t)
      << ",\"scope\":\"retained_capacities_not_RSS\"},\"timings\":{\"generation_ms\":" << ms(start, generated)
      << ",\"cloud_ms\":" << ms(generated, prepared) << ",\"oracle_prepare_ms\":" << ms(prepared, judged)
      << ",\"seed_with_validation_callback_ms\":" << ms(judged, processed) << ",\"validation_ms\":" << ms(processed, validated)
      << ",\"release_ms\":" << ms(validated, done) << ",\"total_ms\":" << ms(start, done) << "}}\n";
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 4) throw std::invalid_argument("usage: mhgp8_q34_seed_probe n uniform|rows|coplanar kmax");
    const auto n = number(argv[1]), kmax = number(argv[3]);
    const std::string_view family(argv[2]);
    if (n < 8 || (kmax != 5 && kmax != 10) ||
        (family != "uniform" && family != "rows" && family != "coplanar"))
      throw std::invalid_argument("invalid fixture size, family or Kmax");
    const u64 domain = family == "uniform" ? (u64{1} << 42U) + 6 :
                       family == "rows" ? 51078 : 1667078;
    if (n > domain) throw std::invalid_argument("fixture exceeds its finite u16 recipe domain");
    return run(n, family, kmax);
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
