// Donnees synthetiques d'assemblage : les niveaux attendus viennent d'indices entiers, pas de num::compare.
#pragma once
#include <vector>
#include "catalogue/assembly_parallel.hpp"

namespace assembly_test {
using namespace mhgp11;
namespace cat = mhgp11::catalogue_detail;

inline bool same_level(const num::Level& a, const num::Level& b) {
  const auto an = num::to_wide(a.numerator()), bn = num::to_wide(b.numerator());
  const auto ad = num::to_wide(a.denominator()), bd = num::to_wide(b.denominator());
  return an.neg == bn.neg && an.words == bn.words && ad.neg == bd.neg && ad.words == bd.words;
}
inline bool same_ball(const CatalogueBall& a, const CatalogueBall& b) {
  return a.support == b.support && a.rank == b.rank && a.p == b.p && a.m == b.m && a.qmin == b.qmin;
}
inline std::array<u64, 14> timing_values(const CatalogueTimings& value) {
  return {value.prefix_ns, value.count_ns, value.replay_ns, value.fill_ns, value.sort_ns,
          value.level_scan_ns, value.allocation_ns, value.assembly_ns, value.count_task_sum_ns,
          value.count_task_max_ns, value.fill_task_sum_ns, value.fill_task_max_ns,
          value.sort_comparisons, value.tasks};
}
inline bool same(const Catalogue& a, const Catalogue& b) {
  if (a.kmax() != b.kmax() || a.balls() != b.balls() || a.levels().size() != b.levels().size() ||
      a.ledger() != b.ledger() || a.population().size() != b.population().size()) return false;
  for (u32 i = 0; i < a.balls(); ++i)
    if (!same_ball(a.balls_data()[i], b.balls_data()[i])) return false;
  for (u64 i = 0; i < a.levels().size(); ++i)
    if (!same_level(a.levels()[i], b.levels()[i])) return false;
  return std::equal(a.population_offsets().begin(), a.population_offsets().end(), b.population_offsets().begin()) &&
         std::equal(a.population().begin(), a.population().end(), b.population().begin());
}

struct Fixture {
  std::vector<cat::Emission> sorted;
  std::vector<SiteIdx> population;
  u32 group = 1;
};

inline Result<Fixture> fixture(u32 count, u32 group, bool wide = false) {
  Fixture value; value.group = group;
  for (u32 i = 0; i < count; ++i) {
    const u64 rank = u64(i / group) + 1;
    auto n = num::Wide<4>::from_u64(rank);
    auto d = num::Wide<3>::from_u64(3);
    if (wide) {
      constexpr int nb = num::Budget::level_numerator - 3, db = num::Budget::level_denominator - 3;
      n.words[nb / 64] |= u64{1} << (nb % 64);
      d = {}; d.words[db / 64] = u64{1} << (db % 64);
    }
    if ((i & 1u) != 0) {
      num::Wide<4> nn; num::Wide<3> dd;
      if (!num::add(n, n, nn) || !num::add(d, d, dd)) return fail(Reason::arithmetic_invariant);
      n = nn; d = dd;
    }
    auto level = num::Level::make(n, d);
    if (!level.ok()) return level.outcome();
    CatalogueBall ball;
    ball.qmin = static_cast<u8>(2 + i % 3); ball.p = i % 3; ball.m = ball.qmin;
    ball.support.fill(SiteIdx{kNone});
    for (u32 j = 0; j < ball.qmin; ++j) ball.support[j] = SiteIdx{4 * i + j};
    ball.rank = LevelRank{kNone};
    value.sorted.push_back({ball, level.value(), value.population.size()});
    for (u32 j = 0; j < ball.p + ball.m; ++j) value.population.push_back(SiteIdx{7 * i + j});
  }
  return value;
}

struct Input {
  Buffer<cat::Emission> records;
  Buffer<SiteIdx> population;
  Outcome load(const Fixture& value, MemoryBudget& budget, bool reverse = true) {
    MHGP11_TRY(records.allocate(value.sorted.size(), budget));
    MHGP11_TRY(population.allocate(value.population.size(), budget));
    std::copy(value.population.begin(), value.population.end(), population.begin());
    for (u64 i = 0; i < value.sorted.size(); ++i)
      records[i] = value.sorted[reverse ? value.sorted.size() - 1 - i : i];
    return {};
  }
};

inline u64 output_bytes(const Fixture& f) {
  const u64 levels = f.sorted.empty() ? 1 : (f.sorted.size() - 1) / f.group + 2;
  return f.sorted.size() * sizeof(CatalogueBall) + levels * sizeof(num::Level) +
         (f.sorted.size() + 1) * sizeof(u64) + f.population.size() * sizeof(SiteIdx);
}
inline Result<Cloud> cloud(MemoryBudget& budget) {
  const std::array<u32, 5> x{0, 2, 4, 6, 8}, y{}, z{};
  const std::array<PointId, 5> ids{PointId{0}, PointId{1}, PointId{2}, PointId{3}, PointId{4}};
  return prepare_cloud(x, y, z, ids, CoordWidth{}, budget);
}
}  // namespace assembly_test
