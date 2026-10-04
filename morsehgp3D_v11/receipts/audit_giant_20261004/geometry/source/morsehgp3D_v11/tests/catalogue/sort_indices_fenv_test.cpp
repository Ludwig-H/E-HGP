// Portes F3/F4 : rangs rationnels analytiques, modes mixtes, environnement restaure et permutation complete.
#include <bit>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "catalogue/internal.hpp"
#include "catalogue/sort_indices.hpp"
#include "catalogue/sort_level_key.hpp"
#include "fenv.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp11;
namespace cat = mhgp11::catalogue_detail;

namespace {
struct Ranked {
  num::Level level;
  u32 rank;
};

template <int Words>
num::Wide<Words> power(int exponent) {
  num::Wide<Words> value;
  value.words[exponent / 64] = u64{1} << (exponent % 64);
  return value;
}

template <int Words>
num::Wide<Words> maximum(int bits) {
  num::Wide<Words> value;
  for (int i = 0; i < bits; ++i) value.words[i / 64] |= u64{1} << (i % 64);
  return value;
}

void append(std::vector<Ranked>& out, const num::Wide<4>& n, const num::Wide<3>& d, u32 rank) {
  auto level = num::Level::make(n, d);
  if (!level.ok()) throw std::runtime_error("level fixture hors domaine");
  out.push_back({level.value(), rank});
}

std::vector<Ranked> levels() {
  std::vector<Ranked> out;
  const auto one_n = num::Wide<4>::from_u64(1), three_n = num::Wide<4>::from_u64(3);
  const auto one_d = num::Wide<3>::from_u64(1), three_d = num::Wide<3>::from_u64(3);
  u32 rank = 0;
  append(out, {}, one_d, rank++);
  // 1/(2^d-1), par d decroissant : niveaux strictement croissants et tetes de mots exercees.
  append(out, one_n, maximum<3>(num::Budget::level_denominator), rank++);
  for (int bits : {129, 128, 127, 65, 64, 63})
    if (bits < num::Budget::level_denominator) append(out, one_n, maximum<3>(bits), rank++);
  // c +/- 2^-100, puis c exactement : tous <1, ordre connu sans appeler num::compare.
  num::Wide<4> margin, below, above;
  if (!num::subtract(power<4>(100), power<4>(60), margin) ||
      !num::subtract(margin, one_n, below) || !num::add(margin, one_n, above))
    throw std::runtime_error("margin fixture");
  append(out, below, power<3>(100), rank++);
  append(out, margin, power<3>(100), rank++);
  append(out, above, power<3>(100), rank++);
  append(out, one_n, one_d, rank);
  append(out, three_n, three_d, rank++);  // Egalite exacte, representation non reduite 3x.
  const auto n = num::Wide<4>::from_u64((u64{1} << 53) + 1);
  const auto d = power<3>(53);
  num::Wide<4> twice_n, triple_n;
  num::Wide<3> twice_d, triple_d;
  if (!num::add(n, n, twice_n) || !num::add(n, twice_n, triple_n) ||
      !num::add(d, d, twice_d) || !num::add(d, twice_d, triple_d))
    throw std::runtime_error("triple fixture");
  append(out, n, d, rank);
  append(out, triple_n, triple_d, rank++);  // Cast n->double non exact : arrondis diriges distincts.
  append(out, num::Wide<4>::from_u64(2), one_d, rank);
  append(out, num::Wide<4>::from_u64(6), three_d, rank++);
  for (int bits : {63, 64, 65, 127, 128, 129, 192, 193}) {
    if (bits > num::Budget::level_numerator) continue;
    auto large = power<4>(bits - 1);
    if (!num::add(large, one_n, large)) throw std::runtime_error("large fixture");
    append(out, large, one_d, rank++);
  }
  append(out, maximum<4>(num::Budget::level_numerator), one_d, rank);
  return out;
}

// La barriere de fonction de TEST garde une vraie preparation puis une vraie comparaison distinctes.
// Le mode est lu dans chaque appel : la porte ne demande aucun crochet ni etat dans le produit.
[[gnu::noinline]] double prepare_key(const num::Level& level, int mode) {
  if (std::fegetround() != mode) throw std::runtime_error("preparation mode");
  return cat::level_key(level);
}

[[gnu::noinline]] int compare_keys(double a, double b, int mode) {
  if (std::fegetround() != mode) throw std::runtime_error("comparison mode");
  return cat::level_key_order(a, b);
}

void permutation(const std::vector<Ranked>& values, sched::Pool* pool) {
  constexpr u32 count = 4097;  // Trois runs, deux etages de fusion, co-rangs et egalites inter-runs.
  std::vector<cat::Emission> records(count);
  std::vector<u32> ranks(count), expected(count);
  for (u32 i = 0; i < count; ++i) {
    const auto& value = values[(u64(i) * 2053) % values.size()];
    records[i].level = value.level;
    ranks[i] = value.rank;
    const u32 support = (i * 13) % 17;
    records[i].ball.support = {SiteIdx{2 * support}, SiteIdx{2 * support + 1}, SiteIdx{kNone}, SiteIdx{kNone}};
    records[i].ball.qmin = 2;
    records[i].population_begin = u64(i) * 7;
    expected[i] = i;
  }
  std::sort(expected.begin(), expected.end(), [&](u32 a, u32 b) {
    if (ranks[a] != ranks[b]) return ranks[a] < ranks[b];
    const auto& x = records[a].ball.support;
    const auto& y = records[b].ball.support;
    return x != y ? x < y : a < b;
  });
  const auto original = records;
  const int mode = std::fegetround();
  MemoryBudget budget(7 + 16 * u64(count));
  Buffer<u8> sentinel;
  REQUIRE(sentinel.allocate(7, budget).ok());
  sentinel[0] = 123;
  u64 comparisons = 123;
  auto sorted = cat::sort_indices(records, budget, pool, &comparisons);
  REQUIRE(sorted.ok());
  CHECK(std::equal(sorted.value().span().begin(), sorted.value().span().end(), expected.begin(), expected.end()));
  CHECK(comparisons > 0 && comparisons <= 4 * u64(count) * std::bit_width(u64(count) - 1));
  CHECK_EQ(budget.used(), 7 + 4 * u64(count));
  CHECK_EQ(budget.peak(), 7 + 16 * u64(count));
  CHECK_EQ(sentinel[0], 123u);
  CHECK_EQ(std::fegetround(), mode);
  for (u32 i = 0; i < count; ++i) {
    CHECK(records[i].ball.support == original[i].ball.support);
    CHECK_EQ(records[i].population_begin, original[i].population_begin);
    CHECK(num::to_wide(records[i].level.numerator()).words == num::to_wide(original[i].level.numerator()).words);
    CHECK(num::to_wide(records[i].level.denominator()).words == num::to_wide(original[i].level.denominator()).words);
  }
  sorted.value().reset();
  sentinel.reset();
  CHECK(budget.released().ok());
}
}  // namespace

MHGP11_TEST(key_modes, 10000) {
  const auto values = levels();
  test::FenvGuard environment;
  u64 fast = 0, fallback = 0, equal = 0;
  for (unsigned flush : test::kFlushModes) {
    std::array<std::vector<double>, 4> keys;
    for (u32 mode = 0; mode < test::kRoundModes.size(); ++mode) {
      REQUIRE(environment.set(test::kRoundModes[mode], flush));
      for (const auto& value : values) {
        const double key = prepare_key(value.level, test::kRoundModes[mode]);
        CHECK(value.rank == 0 ? key == 0 : std::isnormal(key) && key > 0);
        keys[mode].push_back(key);
      }
    }
    bool distinct_directed_keys = false;
    for (u32 i = 0; i < values.size(); ++i) distinct_directed_keys |= keys[1][i] < keys[2][i];
    CHECK(distinct_directed_keys);  // La porte joue effectivement les arrondis, pas quatre aliases.
    for (int mode : test::kRoundModes) {
      REQUIRE(environment.set(mode, flush));
      for (u32 a_mode = 0; a_mode < keys.size(); ++a_mode)
        for (u32 b_mode = 0; b_mode < keys.size(); ++b_mode)
          for (u32 a = 0; a < values.size(); ++a)
            for (u32 b = 0; b < values.size(); ++b) {
              const int expected = (values[a].rank > values[b].rank) - (values[a].rank < values[b].rank);
              const int order = compare_keys(keys[a_mode][a], keys[b_mode][b], mode);
              CHECK(order == 0 || order == expected);
              CHECK_EQ(order, -compare_keys(keys[b_mode][b], keys[a_mode][a], mode));
              fast += order != 0;
              fallback += order == 0;
              equal += expected == 0;
            }
    }
  }
  CHECK(fast > 0); CHECK(fallback > 0); CHECK(equal > 0);
  std::printf("sort_fenv_keys bits=%d fast=%llu fallback=%llu equal=%llu flush_modes=%zu\n", kCoordBits,
              (unsigned long long)fast, (unsigned long long)fallback, (unsigned long long)equal,
              test::kFlushModes.size());
}

MHGP11_TEST(permutations, 10000) {
  const auto values = levels();
  test::FenvGuard environment;
  for (unsigned flush : test::kFlushModes)
    for (int mode : test::kRoundModes) {
      REQUIRE(environment.set(mode, flush));
      // Les fils neufs heritent de l'environnement du pilote ; aucune reconfiguration du Pool produit.
      auto one = sched::make_pool({1}), four = sched::make_pool({4});
      REQUIRE(one.ok() && four.ok());
      permutation(values, nullptr);
      permutation(values, one.value().get());
      permutation(values, four.value().get());
    }
}

MHGP11_TEST_MAIN()
