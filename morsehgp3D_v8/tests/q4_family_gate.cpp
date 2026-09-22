#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <future>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

#include "lanes/q4_family.hpp"

// Bounded independent judge: rational Gaussian elimination finds the face
// centre and, independently, the sphere through every emitted root. It uses
// neither the production W coefficients nor its reduced determinant. This
// deliberately quadratic small-cloud oracle is not a generation algorithm.
namespace {
using Big = boost::multiprecision::cpp_int;
using Rational = boost::rational<Big>;
using Vector = std::array<Big, 3>;
using Center = std::array<Rational, 3>;
using Points = std::vector<mhgp8::Point3>;
using Ids = std::array<std::size_t, 3>;
using mhgp8::Point3;
using mhgp8::u64;

static_assert(!std::is_default_constructible_v<mhgp8::Q4FamilySeed>);
static_assert(!std::is_copy_assignable_v<mhgp8::Q4FamilySeed>);
static_assert(!std::is_move_assignable_v<mhgp8::Q4FamilySeed>);

struct Gate {
  u64 checks{}, clouds{}, runs{}, primitive_sites{}, root_comparisons{};
  u64 oracle_sites{}, groups{}, mixed_groups{}, decreasing_steps{}, high_depth{};
  u64 max_shell{}, max_cross_bits{}, permutations{}, invalid_inputs{};
  u64 callback_failures{}, reentrant_calls{}, parallel_calls{}, mutants{};
  // 18-bit twins (a seed coordinate above 65535) with their own floors.
  u64 wide_clouds{}, wide_runs{}, wide_max_cross_bits{};
  void require(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
  }
  template<class Exception = std::invalid_argument, class Function>
  void rejects(Function&& function, const char* message) {
    bool caught = false;
    try { function(); } catch (const Exception&) { caught = true; }
    require(caught, message);
    ++invalid_inputs;
  }
};

Vector difference(Point3 a, Point3 b) {
  return {Big(a.x) - b.x, Big(a.y) - b.y, Big(a.z) - b.z};
}
Big dot(const Vector& a, const Vector& b) {
  Big value = 0;
  for (std::size_t axis = 0; axis != 3; ++axis) value += a[axis] * b[axis];
  return value;
}
Vector cross(const Vector& a, const Vector& b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
          a[0] * b[1] - a[1] * b[0]};
}
int sign(const Big& value) { return value > 0 ? 1 : value < 0 ? -1 : 0; }
int sign(const Rational& value) { return sign(value.numerator()); }
u64 bits(Big value) {
  if (value < 0) value = -value;
  return value == 0 ? 0 : static_cast<u64>(boost::multiprecision::msb(value)) + 1;
}
bool wide_point(Point3 point) { return point.x > 65535 || point.y > 65535 || point.z > 65535; }

std::optional<Center> solve(std::array<std::array<Rational, 4>, 3> matrix) {
  for (std::size_t column = 0; column != 3; ++column) {
    std::size_t pivot = column;
    while (pivot != 3 && matrix[pivot][column].numerator() == 0) ++pivot;
    if (pivot == 3) return std::nullopt;
    std::swap(matrix[pivot], matrix[column]);
    const auto diagonal = matrix[column][column];
    for (std::size_t entry = column; entry != 4; ++entry) matrix[column][entry] /= diagonal;
    for (std::size_t row = 0; row != 3; ++row) {
      if (row == column) continue;
      const auto factor = matrix[row][column];
      for (std::size_t entry = column; entry != 4; ++entry)
        matrix[row][entry] -= factor * matrix[column][entry];
    }
  }
  return Center{matrix[0][3], matrix[1][3], matrix[2][3]};
}

Rational physical_power(const Center& center, const Vector& site) {
  Rational value(dot(site, site));
  for (std::size_t axis = 0; axis != 3; ++axis)
    value -= Rational(2) * center[axis] * Rational(site[axis]);
  return value;
}

struct SeedOracle {
  Point3 origin;
  Vector d, u, normal;
  Big gram;
  Center center;
  [[nodiscard]] Rational power(Point3 site) const {
    return physical_power(center, difference(site, origin));
  }
  [[nodiscard]] Big side(Point3 site) const { return dot(normal, difference(site, origin)); }
  [[nodiscard]] Rational root(Point3 site) const {
    return Rational(gram) * power(site) / Rational(side(site));
  }
  [[nodiscard]] Center sphere(Point3 site) const {
    const auto v = difference(site, origin);
    const std::array<Vector, 3> rows{d, u, v};
    std::array<std::array<Rational, 4>, 3> matrix{};
    for (std::size_t row = 0; row != 3; ++row) {
      for (std::size_t axis = 0; axis != 3; ++axis)
        matrix[row][axis] = Rational(2 * rows[row][axis]);
      matrix[row][3] = Rational(dot(rows[row], rows[row]));
    }
    const auto value = solve(std::move(matrix));
    if (!value) throw std::runtime_error("oracle root was coplanar");
    return *value;
  }
};

std::optional<SeedOracle> oracle_seed(Point3 a, Point3 b, Point3 x) {
  const auto d = difference(b, a), u = difference(x, a), v = difference(x, b);
  if (dot(d, u) <= 0 || dot(d, v) >= 0 || dot(u, v) <= 0) return std::nullopt;
  const auto normal = cross(d, u);
  const auto gram = dot(normal, normal);
  if (gram == 0) return std::nullopt;
  std::array<std::array<Rational, 4>, 3> matrix{};
  for (std::size_t axis = 0; axis != 3; ++axis) {
    matrix[0][axis] = Rational(2 * d[axis]);
    matrix[1][axis] = Rational(2 * u[axis]);
    matrix[2][axis] = Rational(normal[axis]);
  }
  matrix[0][3] = Rational(dot(d, d));
  matrix[1][3] = Rational(dot(u, u));
  const auto center = solve(std::move(matrix));
  if (!center) throw std::runtime_error("acute oracle seed was singular");
  return SeedOracle{a, d, u, normal, gram, *center};
}

struct Group {
  std::size_t representative{}, depth{};
  std::vector<std::size_t> roots, constants;
  bool operator==(const Group&) const = default;
};
struct Expected {
  std::vector<Group> groups;
  std::vector<std::size_t> constant_shell;
  u64 entries{}, exits{}, constant_inside{}, constant_outside{};
};

Expected oracle(Gate& gate, const Points& points, const SeedOracle& seed) {
  gate.require(points.size() <= 96, "q4 family oracle exceeded bounded n<=96 domain");
  Expected result;
  std::map<Rational, std::vector<std::size_t>> roots;
  for (std::size_t id = 0; id != points.size(); ++id) {
    const auto b = seed.side(points[id]);
    if (b > 0) ++result.entries;
    if (b < 0) ++result.exits;
    if (b != 0) {
      roots[seed.root(points[id])].push_back(id);
    } else {
      const auto p = seed.power(points[id]);
      if (p.numerator() < 0) ++result.constant_inside;
      if (p.numerator() > 0) ++result.constant_outside;
      if (p.numerator() == 0) result.constant_shell.push_back(id);
    }
  }
  for (const auto& [parameter, members] : roots) {
    static_cast<void>(parameter);
    const auto center = seed.sphere(points[members.front()]);
    Group group{members.front(), 0, members, result.constant_shell};
    std::vector<std::size_t> shell;
    bool entry = false, exit = false;
    for (const auto id : members) {
      entry = entry || seed.side(points[id]) > 0;
      exit = exit || seed.side(points[id]) < 0;
    }
    if (entry && exit) ++gate.mixed_groups;
    for (std::size_t id = 0; id != points.size(); ++id) {
      const auto p = physical_power(center, difference(points[id], seed.origin));
      if (p.numerator() < 0) ++group.depth;
      if (p.numerator() == 0) shell.push_back(id);
      ++gate.oracle_sites;
    }
    auto expected_shell = members;
    expected_shell.insert(expected_shell.end(), result.constant_shell.begin(), result.constant_shell.end());
    std::sort(expected_shell.begin(), expected_shell.end());
    gate.require(shell == expected_shell, "rational root partition disagrees with independent sphere solve");
    gate.max_shell = std::max(gate.max_shell, static_cast<u64>(shell.size()));
    gate.high_depth = std::max(gate.high_depth, static_cast<u64>(group.depth));
    if (!result.groups.empty() && result.groups.back().depth > group.depth) ++gate.decreasing_steps;
    result.groups.push_back(std::move(group));
  }
  return result;
}

std::vector<Group> collect(const mhgp8::CloudPtr& cloud, Ids ids, mhgp8::Q4FamilyWork* work = nullptr) {
  std::vector<Group> result;
  const auto value = mhgp8::run_q4_family(cloud, ids, [&](const mhgp8::Q4FamilyGroup& group) {
    result.push_back({group.representative_id, group.depth,
                      {group.root_ids.begin(), group.root_ids.end()},
                      {group.constant_shell.begin(), group.constant_shell.end()}});
  });
  if (work != nullptr) *work = value;
  return result;
}

void check_case(Gate& gate, const Points& points, Ids ids) {
  const bool wide = wide_point(points[ids[0]]) || wide_point(points[ids[1]]) || wide_point(points[ids[2]]);
  const auto reference = oracle_seed(points[ids[0]], points[ids[1]], points[ids[2]]);
  const auto seed = mhgp8::Q4FamilySeed::make(points[ids[0]], points[ids[1]], points[ids[2]]);
  gate.require(reference.has_value() == seed.has_value(), "seed positivity disagrees with rational oracle");
  if (!reference) return;
  gate.require(seed->points() == std::array<Point3, 3>{points[ids[0]], points[ids[1]], points[ids[2]]},
               "seed changed its copied oriented coordinates");
  gate.require(Big(seed->gram()) == reference->gram, "wrong Gram determinant");
  std::vector<std::size_t> roots;
  for (std::size_t id = 0; id != points.size(); ++id) {
    const auto p = Rational(reference->gram) * reference->power(points[id]);
    gate.require(p.denominator() == 1, "oracle scaled power was nonintegral");
    gate.require(Big(seed->power(points[id])) == p.numerator(), "q3 power differs from solved face centre");
    gate.require(Big(seed->side(points[id])) == reference->side(points[id]), "oriented side differs");
    if (reference->side(points[id]) != 0) roots.push_back(id);
    ++gate.primitive_sites;
  }
  for (const auto first : roots) for (const auto second : roots) {
    const auto wanted = sign(reference->root(points[first]) - reference->root(points[second]));
    gate.require(seed->compare_roots(points[first], points[second]) == wanted,
                 "reduced determinant root order differs from exact rational quotient");
    const Big crossed = Rational(reference->gram * reference->side(points[second]) *
                                reference->power(points[first])).numerator();
    gate.max_cross_bits = std::max(gate.max_cross_bits, bits(crossed));
    if (wide) gate.wide_max_cross_bits = std::max(gate.wide_max_cross_bits, bits(crossed));
    ++gate.root_comparisons;
  }
  const auto expected = oracle(gate, points, *reference);
  const auto cloud = mhgp8::prepare_cloud(points);
  const auto before = Points(cloud->points().begin(), cloud->points().end());
  const auto bytes_before = cloud->retained_bytes();
  mhgp8::Q4FamilyWork work;
  const auto actual = collect(cloud, ids, &work);
  gate.require(actual == expected.groups, "q4 family groups, strict depths or full shell differ");
  gate.require(work.sites == points.size(), "not all global sites were classified");
  gate.require(work.entries == expected.entries && work.exits == expected.exits,
               "entry/exit ledger differs");
  gate.require(work.constant_inside == expected.constant_inside &&
               work.constant_outside == expected.constant_outside &&
               work.constant_on == expected.constant_shell.size(), "coplanar ledger differs");
  gate.require(work.event_count == expected.entries + expected.exits &&
               work.event_count + work.constant_inside + work.constant_outside + work.constant_on == work.sites,
               "site partition ledger is incomplete");
  gate.require(work.groups == actual.size() && work.callbacks == actual.size(), "callback/group ledger differs");
  gate.require(work.group_comparisons == (work.event_count == 0 ? 0 : work.event_count - 1),
               "group comparisons did not inspect every event boundary once");
  gate.require(work.event_count < 2 || work.sort_comparisons > 0,
               "nontrivial root sort did not report comparisons");
  gate.require(work.retained_capacity_bytes % sizeof(std::size_t) == 0 &&
               work.retained_capacity_bytes >= 2 * points.size() * sizeof(std::size_t),
               "two reserved ID capacities were not fully charged");
  std::size_t largest = 0;
  for (const auto& group : actual) largest = std::max(largest, group.roots.size());
  gate.require(work.max_group == largest, "maximum root group ledger differs");
  gate.require(Points(cloud->points().begin(), cloud->points().end()) == before &&
               cloud->retained_bytes() == bytes_before, "family mutated shared immutable cloud");
  gate.groups += actual.size();
  ++gate.runs;
  if (wide) ++gate.wide_runs;
}

void add_unique(Points& points, Point3 point) {
  if (std::find(points.begin(), points.end(), point) == points.end()) points.push_back(point);
}
Points shell_fixture() {
  Points result{{25, 20, 20}, {17, 24, 20}, {17, 16, 20}};
  for (int x = -5; x <= 5; ++x) for (int y = -5; y <= 5; ++y)
    for (int z = -5; z <= 5; ++z) if (x * x + y * y + z * z == 25)
      add_unique(result, {static_cast<mhgp8::Coordinate>(x + 20),
                          static_cast<mhgp8::Coordinate>(y + 20),
                          static_cast<mhgp8::Coordinate>(z + 20)});
  result.push_back({20, 20, 20});  // Permanent strict interior.
  result.push_back({50, 50, 20});  // Permanent exterior.
  return result;
}
std::vector<Points> fixtures() {
  std::vector<Points> result;
  result.push_back({{10, 10, 10}, {30, 10, 10}, {20, 30, 10}});
  result.push_back({{10, 10, 10}, {30, 10, 10}, {20, 30, 10},
                    {20, 18, 10}, {20, 50, 10}, {10, 25, 10}, {30, 25, 10}});
  result.push_back({{57, 50, 51}, {45, 55, 50}, {45, 45, 50},
                    {57, 50, 49}, {57, 50, 48}});
  result.push_back(shell_fixture());
  result.push_back({{0, 0, 0}, {65535, 0, 0}, {32767, 65535, 0},
                    {0, 0, 65535}, {65535, 65535, 65534}, {1, 65534, 32767},
                    {65535, 0, 1}, {32767, 32767, 0}});
  // 18-bit twin of the corner fixture (262143/262142/131071), with the old
  // 65535 corner kept as an interior site.
  result.push_back({{0, 0, 0}, {262143, 0, 0}, {131071, 262143, 0},
                    {0, 0, 262143}, {262143, 262143, 262142}, {1, 262142, 131071},
                    {262143, 0, 1}, {131071, 131071, 0}, {65535, 65535, 65535}});
  Points deep{{100, 100, 100}, {120, 100, 100}, {110, 120, 100}};
  for (unsigned z = 70; z <= 130; ++z)
    deep.push_back({110, 108, static_cast<mhgp8::Coordinate>(z)});
  result.push_back(std::move(deep));
  // Symmetric off-plane points produce equal roots with opposite directions.
  result.push_back({{15, 10, 10}, {7, 14, 10}, {7, 6, 10},
                    {10, 10, 15}, {10, 10, 5}, {10, 10, 10}, {10, 10, 30}});
  return result;
}

void invalid_cases(Gate& gate) {
  // The last two are consecutive Fibonacci pairs (Cassini: Gram determinant
  // 1), obtuse at x; the second is the 18-bit twin of the 16-bit one.
  const std::array<Points, 6> bad{{
      {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}},
      {{0, 0, 0}, {2, 0, 0}, {1, 0, 0}},
      {{0, 0, 0}, {1, 0, 0}, {3, 1, 0}},
      {{0, 0, 0}, {0, 0, 0}, {0, 1, 0}},
      {{0, 0, 0}, {46368, 28657, 0}, {28657, 17711, 0}},
      {{0, 0, 0}, {196418, 121393, 0}, {121393, 75025, 0}}
  }};
  for (const auto& points : bad) {
    gate.require(!mhgp8::Q4FamilySeed::make(points[0], points[1], points[2]),
                 "non-positive seed was admitted");
    gate.require(!oracle_seed(points[0], points[1], points[2]), "bad-seed fixture became acute");
    ++gate.invalid_inputs;
  }
  const auto points = fixtures().front();
  const auto cloud = mhgp8::prepare_cloud(points);
  const mhgp8::Q4FamilyConsumer callback = [](const mhgp8::Q4FamilyGroup&) {};
  gate.rejects([&] { static_cast<void>(mhgp8::run_q4_family({}, {0, 1, 2}, callback)); }, "null cloud accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q4_family(cloud, {0, 1, 2}, {})); }, "empty callback accepted");
  gate.rejects([&] { static_cast<void>(mhgp8::run_q4_family(cloud, {0, 0, 2}, callback)); }, "repeated seed ID accepted");
  gate.rejects<std::out_of_range>([&] {
    static_cast<void>(mhgp8::run_q4_family(cloud, {0, 1, points.size()}, callback));
  }, "out-of-range seed ID accepted");
  const auto seed = *mhgp8::Q4FamilySeed::make(points[0], points[1], points[2]);
  gate.rejects([&] { static_cast<void>(seed.compare_roots(points[0], {20, 18, 11})); }, "coplanar first root accepted");
  gate.rejects([&] { static_cast<void>(seed.compare_roots({20, 18, 11}, points[1])); }, "coplanar second root accepted");
  const auto right = mhgp8::prepare_cloud(bad.front());
  gate.rejects([&] { static_cast<void>(mhgp8::run_q4_family(right, {0, 1, 2}, callback)); }, "run accepted right seed");
}

void callback_cases(Gate& gate) {
  const auto points = fixtures().back();
  auto cloud = mhgp8::prepare_cloud(points);
  const auto expected = collect(cloud, {0, 1, 2});
  std::size_t calls = 0;
  struct CallbackFailure final : std::exception {};
  bool caught = false;
  try {
    static_cast<void>(mhgp8::run_q4_family(cloud, {0, 1, 2}, [&](const auto&) {
      ++calls;
      throw CallbackFailure{};
    }));
  } catch (const CallbackFailure&) { caught = true; }
  gate.require(caught && calls == 1, "callback exception was swallowed or processing continued");
  gate.require(collect(cloud, {0, 1, 2}) == expected, "failed run leaked mutable global state");
  ++gate.callback_failures;
  calls = 0;
  static_cast<void>(mhgp8::run_q4_family(cloud, {0, 1, 2}, [&](const auto& group) {
    if (calls++ == 0) {
      gate.require(collect(cloud, {0, 1, 2}) == expected, "independent reentrant family changed output");
      gate.require(group.root_ids.front() == expected.front().roots.front(), "nested call invalidated borrowed callback view");
      ++gate.reentrant_calls;
    }
  }));
  std::vector<std::future<std::vector<Group>>> futures;
  for (unsigned worker = 0; worker != 4; ++worker)
    futures.push_back(std::async(std::launch::async, [cloud] { return collect(cloud, {0, 1, 2}); }));
  for (auto& future : futures) {
    gate.require(future.get() == expected, "concurrent independent family calls disagree");
    ++gate.parallel_calls;
  }
  calls = 0;
  static_cast<void>(mhgp8::run_q4_family(cloud, {0, 1, 2}, [&](const auto&) {
    if (calls++ == 0) cloud.reset();
  }));
  gate.require(calls == expected.size() && !cloud, "run did not own cloud across callbacks");
}

// Causal judge checks, not product fault-injection: intentionally damage its
// observable output and ensure the independent expected result detects it.
void mutation_cases(Gate& gate) {
  const auto points = shell_fixture();
  const auto reference = *oracle_seed(points[0], points[1], points[2]);
  const auto expected = oracle(gate, points, reference).groups;
  const auto actual = collect(mhgp8::prepare_cloud(points), {0, 1, 2});
  gate.require(actual == expected && !actual.empty(), "mutant base run is not correct/nonempty");
  auto wrong = actual;
  ++wrong.front().depth;
  gate.require(wrong != expected, "judge missed strict-shell credited as interior");
  ++gate.mutants;
  wrong = actual;
  wrong.front().roots.pop_back();
  gate.require(wrong != expected, "judge missed omitted root incidence");
  ++gate.mutants;
  wrong = actual;
  wrong.front().constants.clear();
  gate.require(wrong != expected, "judge missed constant shell");
  ++gate.mutants;
  const auto decline = fixtures()[2];
  auto declined = collect(mhgp8::prepare_cloud(decline), {0, 1, 2});
  gate.require(declined.size() == 2 && declined[0].depth == 1 && declined[1].depth == 0,
               "decreasing-depth counterfixture not positively exercised");
  const auto correct = declined;
  std::reverse(declined.begin(), declined.end());
  gate.require(declined != correct, "judge missed root order reversal");
  ++gate.mutants;
  const auto mixed = fixtures().back();
  const auto seed = *oracle_seed(mixed[0], mixed[1], mixed[2]);
  bool missing_denominator_sign = false;
  for (const auto& a : mixed) for (const auto& b : mixed) {
    const auto ba = seed.side(a), bb = seed.side(b);
    if (ba == 0 || bb == 0) continue;
    const Rational crossed = seed.power(a) * Rational(bb) - seed.power(b) * Rational(ba);
    missing_denominator_sign = missing_denominator_sign ||
        sign(crossed) != sign(seed.root(a) - seed.root(b));
  }
  gate.require(missing_denominator_sign, "denominator-sign mutant survived opposite-side fixtures");
  ++gate.mutants;
}

void run(Gate& gate) {
  invalid_cases(gate);
  const auto fixed = fixtures();
  for (const auto& points : fixed) {
    ++gate.clouds;
    Ids ids{0, 1, 2};
    do { check_case(gate, points, ids); ++gate.permutations; }
    while (std::next_permutation(ids.begin(), ids.end()));
    auto rotated = points;
    for (auto& point : rotated) point = {point.z, point.x, point.y};
    check_case(gate, rotated, {0, 1, 2});
    auto reversed = points;
    std::reverse(reversed.begin(), reversed.end());
    check_case(gate, reversed, {points.size() - 1, points.size() - 2, points.size() - 3});
  }
  std::uint64_t state = 0x452b812cd39a1f07ULL;
  auto next = [&]() {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return state >> 32;
  };
  for (unsigned trial = 0; trial != 50; ++trial) {
    Points points;
    const unsigned side = trial % 3 == 0 ? 65536U : 17U;
    while (points.size() < 9 + trial % 12)
      add_unique(points, {static_cast<mhgp8::Coordinate>(next() % side),
                          static_cast<mhgp8::Coordinate>(next() % side),
                          static_cast<mhgp8::Coordinate>(next() % side)});
    unsigned admitted = 0;
    for (std::size_t a = 0; a != points.size() && admitted != 3; ++a)
      for (std::size_t b = a + 1; b != points.size() && admitted != 3; ++b)
        for (std::size_t x = b + 1; x != points.size() && admitted != 3; ++x)
          if (oracle_seed(points[a], points[b], points[x])) {
            check_case(gate, points, {a, b, x});
            ++admitted;
          }
    gate.require(admitted == 3, "random fixture did not exercise enough positive seeds");
    ++gate.clouds;
  }
  // Separate 18-bit random clouds (side 262144) with their own floors; the
  // historical generator above is pinned by its floors and left unchanged.
  std::uint64_t wide_state = 0x9e3779b97f4a7c15ULL;
  auto wide_next = [&]() {
    wide_state = wide_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return wide_state >> 32;
  };
  for (unsigned trial = 0; trial != 12; ++trial) {
    Points points;
    while (points.size() < 9 + trial % 12)
      add_unique(points, {static_cast<mhgp8::Coordinate>(wide_next() % 262144U),
                          static_cast<mhgp8::Coordinate>(wide_next() % 262144U),
                          static_cast<mhgp8::Coordinate>(wide_next() % 262144U)});
    unsigned admitted = 0;
    for (std::size_t a = 0; a != points.size() && admitted != 3; ++a)
      for (std::size_t b = a + 1; b != points.size() && admitted != 3; ++b)
        for (std::size_t x = b + 1; x != points.size() && admitted != 3; ++x)
          if (oracle_seed(points[a], points[b], points[x])) {
            check_case(gate, points, {a, b, x});
            ++admitted;
          }
    gate.require(admitted == 3, "18-bit random fixture did not exercise enough positive seeds");
    ++gate.wide_clouds;
  }
  callback_cases(gate);
  mutation_cases(gate);
  gate.require(gate.runs >= 200 && gate.groups >= 1000 && gate.oracle_sites >= 10000,
               "family oracle nonvacuity floor");
  gate.require(gate.root_comparisons >= 20000 && gate.mixed_groups >= 8 &&
               gate.decreasing_steps >= 20 && gate.high_depth > 10 &&
               gate.max_shell >= 30 && gate.max_cross_bits > 128, "adversarial nonvacuity floor");
  gate.require(gate.mutants == 5 && gate.callback_failures == 1 && gate.parallel_calls == 4 &&
               gate.reentrant_calls == 1, "lifecycle/judge nonvacuity floor");
  gate.require(gate.wide_clouds == 12 && gate.wide_runs >= 30 && gate.wide_max_cross_bits > 128,
               "18-bit nonvacuity floor");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") {
    std::cerr << "usage: mhgp8_q4_family_gate --selftest\n";
    return 2;
  }
  try {
    Gate gate;
    run(gate);
    std::cout << "{\"schema\":\"mhgp8_q4_family_gate_v1\",\"status\":\"passed\""
              << ",\"checks\":" << gate.checks << ",\"clouds\":" << gate.clouds
              << ",\"runs\":" << gate.runs << ",\"primitive_sites\":" << gate.primitive_sites
              << ",\"root_comparisons\":" << gate.root_comparisons << ",\"oracle_sites\":" << gate.oracle_sites
              << ",\"groups\":" << gate.groups << ",\"mixed_groups\":" << gate.mixed_groups
              << ",\"decreasing_steps\":" << gate.decreasing_steps << ",\"max_depth\":" << gate.high_depth
              << ",\"max_shell\":" << gate.max_shell << ",\"max_cross_bits\":" << gate.max_cross_bits
              << ",\"permutations\":" << gate.permutations << ",\"invalid_inputs\":" << gate.invalid_inputs
              << ",\"callback_failures\":" << gate.callback_failures << ",\"reentrant_calls\":" << gate.reentrant_calls
              << ",\"parallel_calls\":" << gate.parallel_calls << ",\"judge_mutants\":" << gate.mutants
              << ",\"wide_clouds\":" << gate.wide_clouds << ",\"wide_runs\":" << gate.wide_runs
              << ",\"wide_max_cross_bits\":" << gate.wide_max_cross_bits << "}\n";
  } catch (const std::exception& error) {
    std::cerr << "q4 family gate: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
