// Nominal product primitive vs independent rational Gram supports.
#include <algorithm>
#include <cstdio>
#include <limits>
#include <random>
#include <string_view>
#include <vector>

#include "../src/forest/anchor_meb.hpp"
#include "../oracle/local_plateau_oracle.hpp"

#ifdef MHGP7_TESTING
#error Compile the anchor MEB against nominal product primitives
#endif

namespace {
using namespace mhgp7;
namespace oracle = local_plateau_oracle;
using oracle::Int;
using oracle::Rat;
u64 checks = 0, comparisons = 0, extra_shells = 0, permutations = 0;
std::array<u64, 5> accepted_supports{};
struct Failure { const char* why; };
void need(bool value, const char* why) { ++checks; if (!value) throw Failure{why}; }
Int integer(i128 value) {
  const u128 magnitude = uabs128(value);
  Int out = static_cast<u64>(magnitude >> 64);
  out <<= 64;
  out += static_cast<u64>(magnitude);
  return value < 0 ? -out : out;
}
oracle::Ball rational_ball(const BallKey& key) {
  oracle::Ball out;
  out.radius2 = -Rat(integer(key.c), integer(key.a));
  for (unsigned axis = 0; axis < 3; ++axis) {
    out.center[axis] = Rat(-integer(key.b[axis]), 2 * integer(key.a));
    out.radius2 += out.center[axis] * out.center[axis];
  }
  return out;
}
Rat rational_level(const ExactLevel& level) {
  Int numerator = level.num[2];
  numerator <<= 64; numerator += level.num[1];
  numerator <<= 64; numerator += level.num[0];
  return Rat(numerator, integer(level.den));
}
Rat distance2(const P3& point, const oracle::Ball& ball) {
  const auto delta = oracle::detail::difference(oracle::detail::point(point), ball.center);
  return oracle::detail::dot(delta, delta);
}
std::vector<u32> supports(const std::vector<P3>& points) {
  std::vector<std::vector<unsigned>> slots;
  for (u32 mask = 1; mask < (u32{1} << points.size()); ++mask) {
    if (std::popcount(mask) > 4) continue;
    if (points.size() > 1 && std::popcount(mask) == 1) continue;
    std::vector<unsigned> selected;
    for (unsigned i = 0; i < points.size(); ++i)
      if ((mask & (1u << i)) != 0) selected.push_back(i);
    slots.push_back(selected);
  }
  std::sort(slots.begin(), slots.end(), [](const auto& a, const auto& b) {
    return a.size() != b.size() ? a.size() < b.size() : a < b;
  });
  std::vector<u32> result;
  for (const auto& selected : slots) {
    u32 mask = 0;
    for (unsigned slot : selected) mask |= 1u << slot;
    result.push_back(mask);
  }
  return result;
}
// Independent exact-support search. It evaluates every possible positive
// Gram support, not the product's first-containing early exit.
oracle::Ball judge(const std::vector<P3>& points, AnchorMebWork& expected, u32& first_support) {
  const u32 full = (u32{1} << points.size()) - 1;
  std::optional<oracle::Ball> best;
  ++expected.calls;
  for (u32 support : supports(points)) {
    const auto ball = oracle::detail::support_ball(points, support);
    if (first_support == 0) ++expected.supports_by_size[std::popcount(support)];
    if (!ball) continue;
    bool enclosed = true;
    for (const auto& point : points) {
      if (first_support == 0 && points.size() > 1) ++expected.power_tests;
      if (distance2(point, *ball) > ball->radius2) { enclosed = false; break; }
    }
    if (enclosed) {
      if (first_support == 0) { first_support = support; ++expected.materializations; }
      if (!best || ball->radius2 < best->radius2) best = *ball;
      need(oracle::detail::enclosed(*ball, points, full), "oracle.containment_consistency");
    }
  }
  need(best.has_value(), "oracle.no_positive_support");
  return *best;
}
AnchorMebResult compare(const std::vector<P3>& points, AnchorMebWork& cumulative) {
  const auto before = points;
  AnchorMebWork expected = cumulative;
  u32 selected = 0;
  const auto truth = judge(points, expected, selected);
  const auto paid_before = cumulative;
  const auto got = anchor_meb(points, cumulative);
  need(points == before, "input.modified");
  need(got.status == AnchorMebStatus::kOk, "meb.expected_success");
  need(got.key.a > 0 && got.level.den > 0, "meb.invalid_geometry");
  const auto observed = rational_ball(got.key);
  need(observed.center == truth.center && observed.radius2 == truth.radius2, "meb.rational_geometry");
  need(rational_level(got.level) == truth.radius2, "meb.rational_level");
  need(cumulative.calls == expected.calls && cumulative.supports_by_size == expected.supports_by_size &&
       cumulative.power_tests == expected.power_tests && cumulative.materializations == expected.materializations,
       "meb.physical_work");
  u32 support = 0;
  for (unsigned i = 0; i < got.support_size; ++i) {
    need(got.support_slots[i] < points.size(), "support.slot_domain");
    if (i != 0) need(got.support_slots[i - 1] < got.support_slots[i], "support.slot_order");
    support |= 1u << got.support_slots[i];
  }
  need(support == selected, "support.first_positive_containing");
  for (unsigned i = got.support_size; i < 4; ++i)
    need(got.support_slots[i] == 0, "support.padding");
  unsigned shell = 0;
  for (const auto& point : points) {
    need(distance2(point, truth) <= truth.radius2, "meb.selected_containment");
    if (distance2(point, truth) == truth.radius2) ++shell;
  }
  need(got.selected_shell_count == shell, "meb.selected_shell_count");
  if (shell > got.support_size) ++extra_shells;
  ++accepted_supports[got.support_size];
  ++comparisons;
  std::printf("{%zu, {", points.size());
  for (const auto& point : points)
    std::printf("{%lld,%lld,%lld},", static_cast<long long>(point.x),
        static_cast<long long>(point.y), static_cast<long long>(point.z));
  std::printf("}, %u, %u, {%u,%u,%u,%u}, {", got.support_size, got.selected_shell_count,
      got.support_slots[0], got.support_slots[1], got.support_slots[2], got.support_slots[3]);
  for (unsigned q = 0; q < 5; ++q)
    std::printf("%lluULL,", static_cast<unsigned long long>(cumulative.supports_by_size[q] - paid_before.supports_by_size[q]));
  std::printf("}, %lluULL, {", static_cast<unsigned long long>(cumulative.power_tests - paid_before.power_tests));
  const auto words = [](i128 value) {
    const auto raw = static_cast<u128>(value);
    std::printf("%lluULL,%lluULL,", static_cast<unsigned long long>(raw),
        static_cast<unsigned long long>(raw >> 64));
  };
  words(got.key.a);
  for (i128 value : got.key.b) words(value);
  words(got.key.c);
  std::printf("}, {");
  for (u64 word : got.level.num) std::printf("%lluULL,", static_cast<unsigned long long>(word));
  words(got.level.den);
  std::printf("}},\n");
  return got;
}
void check_failure(const std::vector<P3>& sites, AnchorMebWork work, AnchorMebStatus status) {
  const auto result = anchor_meb(sites, work);
  need(result.status == status, "refusal.status");
  need(result.key == BallKey{} && result.level == ExactLevel{} && result.support_size == 0 &&
       result.support_slots == std::array<u8, 4>{} && result.selected_shell_count == 0,
       "refusal.transaction_empty");
}
void run() {
  AnchorMebWork work;
  const std::vector<std::vector<P3>> fixtures{
    {{0,0,0},{4,0,0},{2,2,0}},
    {{0,0,0},{4,0,0},{4,4,0},{0,4,0}},
    {{1,8,0},{5,10,0},{9,8,0},{5,0,0}},
    {{10,5,0},{0,5,0},{2,1,0},{2,9,0}},
    {{2,2,2},{2,0,0},{0,2,0},{0,0,2},{0,0,0}},
    {{0,0,0},{2,2,0},{2,0,2},{0,2,2}},
    {{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535}},
    {{10,5,0},{0,5,0},{5,10,0},{5,0,0},{8,9,0},{2,1,0},{9,8,0}}
  };
  for (const auto& fixture : fixtures)
    for (u32 mask = 1; mask < (1u << fixture.size()); ++mask) {
      std::vector<P3> sites;
      for (unsigned i = 0; i < fixture.size(); ++i)
        if ((mask & (1u << i)) != 0) sites.push_back(fixture[i]);
      const auto canonical = compare(sites, work);
      std::reverse(sites.begin(), sites.end());
      const auto reversed = compare(sites, work);
      need(canonical.key == reversed.key && same_exact_level(canonical.level, reversed.level) &&
           canonical.selected_shell_count == reversed.selected_shell_count, "meb.permutation_geometry");
      ++permutations;
    }
  std::mt19937_64 random(20260910);
  for (unsigned n = 1; n <= 10; ++n)
    for (unsigned repetition = 0; repetition < 6; ++repetition) {
      std::vector<P3> sites;
      while (sites.size() != n) {
        const u64 bound = repetition % 2 == 0 ? 65536 : 16;
        const P3 point{static_cast<i64>(random() % bound), static_cast<i64>(random() % bound),
                       static_cast<i64>(random() % bound)};
        if (std::find(sites.begin(), sites.end(), point) == sites.end()) sites.push_back(point);
      }
      const auto before = compare(sites, work);
      std::shuffle(sites.begin(), sites.end(), random);
      const auto after = compare(sites, work);
      need(before.key == after.key && same_exact_level(before.level, after.level), "random.permutation");
      ++permutations;
    }
  const auto diameter = compare({{10,5,0},{0,5,0},{2,1,0},{2,9,0}}, work);
  const auto triangle = compare({{10,5,0},{2,1,0},{2,9,0}}, work);
  need(diameter.key == triangle.key && diameter.support_size == 2 && triangle.support_size == 3,
       "alternative_support.same_ball_different_arity");
  std::vector<P3> exchange{{0,0,0},{4,0,0},{4,4,0},{0,4,0}};
  const auto initial = compare(exchange, work);
  exchange[initial.support_slots[0]] = {2,2,0};
  const auto equal = compare(exchange, work);
  need(equal.key == initial.key && same_exact_level(equal.level, initial.level) &&
       equal.selected_shell_count + 1 == initial.selected_shell_count, "exchange.equal_radius_shell_descent");
  exchange[equal.support_slots[0]] = {1,2,0};
  const auto lower = compare(exchange, work);
  need(compare_exact_level(lower.level, equal.level) < 0, "exchange.subsequent_strict_radius_descent");
  check_failure({}, {}, AnchorMebStatus::kInvalidInput);
  check_failure(std::vector<P3>(11), {}, AnchorMebStatus::kInvalidInput);
  check_failure({{0,0,0},{0,0,0}}, {}, AnchorMebStatus::kInvalidInput);
  check_failure({{-1,0,0}}, {}, AnchorMebStatus::kInvalidInput);
  check_failure({{0,65536,0}}, {}, AnchorMebStatus::kInvalidInput);
  check_failure({{0,0,std::numeric_limits<i64>::max()}}, {}, AnchorMebStatus::kInvalidInput);
  const u64 maximum = std::numeric_limits<u64>::max();
  AnchorMebWork overflow;
  overflow.calls = maximum;
  check_failure({{0,0,0}}, overflow, AnchorMebStatus::kCounterOverflow);
  overflow = {}; overflow.power_tests = maximum;
  check_failure({{0,0,0},{1,0,0}}, overflow, AnchorMebStatus::kCounterOverflow);
  overflow = {}; overflow.materializations = maximum;
  check_failure({{0,0,0},{1,0,0}}, overflow, AnchorMebStatus::kCounterOverflow);
  overflow = {}; overflow.materializations = maximum;
  check_failure({{0,0,0}}, overflow, AnchorMebStatus::kCounterOverflow);
  const std::vector<std::vector<P3>> arities{
    {}, {{0,0,0}}, {{0,0,0},{1,0,0}}, {{0,0,0},{2,2,0},{2,0,2}},
    {{0,0,0},{2,2,0},{2,0,2},{0,2,2}}
  };
  for (unsigned q = 1; q <= 4; ++q) {
    overflow = {}; overflow.supports_by_size[q] = maximum;
    check_failure(arities[q], overflow, AnchorMebStatus::kCounterOverflow);
  }
  overflow = {}; overflow.calls = maximum - 1;
  need(anchor_meb(arities[1], overflow).status == AnchorMebStatus::kOk && overflow.calls == maximum,
       "counter.last_representable_call");
  const auto last = overflow;
  need(anchor_meb(arities[1], overflow).status == AnchorMebStatus::kCounterOverflow &&
       overflow.calls == last.calls && overflow.supports_by_size == last.supports_by_size &&
       overflow.power_tests == last.power_tests && overflow.materializations == last.materializations,
       "counter.no_work_after_overflow");
  need(comparisons >= 600 && permutations >= 300 && extra_shells >= 30 && accepted_supports[1] > 0 &&
       accepted_supports[2] > 0 && accepted_supports[3] > 0 && accepted_supports[4] > 0,
       "nonvacuity");
  std::fprintf(stderr, "{\"status\":\"pass\",\"checks\":%llu,\"comparisons\":%llu,\"permutations\":%llu,"
              "\"extra_shells\":%llu,\"accepted_supports\":[%llu,%llu,%llu,%llu],"
              "\"scope\":\"exact_local_meb_only\",\"gcp_used\":false}\n",
              static_cast<unsigned long long>(checks), static_cast<unsigned long long>(comparisons),
              static_cast<unsigned long long>(permutations), static_cast<unsigned long long>(extra_shells),
              static_cast<unsigned long long>(accepted_supports[1]), static_cast<unsigned long long>(accepted_supports[2]),
              static_cast<unsigned long long>(accepted_supports[3]), static_cast<unsigned long long>(accepted_supports[4]));
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try { run(); return 0; }
  catch (const Failure& failure) { std::fprintf(stderr, "%s\n", failure.why); return 1; }
  catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
