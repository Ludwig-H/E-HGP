// Outils de test : proprietaires d'entree et egalite exacte des champs, sans comparer les paddings natifs.
#pragma once
#include <vector>
#include "catalogue/single_pass_storage.hpp"
#include "sched/sched.hpp"

namespace single_test {
using namespace mhgp11;
using namespace mhgp11::catalogue_detail;
using Xyz = std::array<u32,3>;
inline Result<Cloud> cloud_of(const std::vector<Xyz>& input, MemoryBudget& budget) {
  std::vector<u32> x,y,z;
  std::vector<PointId> ids;
  for (const auto& p : input) {
    x.push_back(p[0]); y.push_back(p[1]); z.push_back(p[2]);
    ids.push_back(PointId{static_cast<u32>(ids.size())});
  }
  return prepare_cloud(x,y,z,ids,CoordWidth{},budget);
}
inline bool same(const Catalogue& a, const Catalogue& b) {
  if (a.balls() != b.balls() || a.levels().size() != b.levels().size() || a.ledger() != b.ledger() ||
      a.population().size() != b.population().size() || a.population_offsets().size() != b.population_offsets().size())
    return false;
  for (u32 i=0; i<a.balls(); ++i) {
    const auto& x=a.balls_data()[i]; const auto& y=b.balls_data()[i];
    if (x.support!=y.support || x.rank!=y.rank || x.p!=y.p || x.m!=y.m || x.qmin!=y.qmin) return false;
  }
  for (u64 i=0; i<a.levels().size(); ++i) {
    const auto& x=a.levels()[i]; const auto& y=b.levels()[i];
    if (num::compare(num::to_wide(x.numerator()),num::to_wide(y.numerator()))!=0 ||
        num::compare(num::to_wide(x.denominator()),num::to_wide(y.denominator()))!=0) return false;
  }
  return std::equal(a.population().begin(),a.population().end(),b.population().begin()) &&
         std::equal(a.population_offsets().begin(),a.population_offsets().end(),b.population_offsets().begin());
}
inline std::array<u64,20> times(const CatalogueTimings& t) {
  return {t.prefix_ns,t.count_ns,t.replay_ns,t.fill_ns,t.sort_ns,t.level_scan_ns,t.allocation_ns,t.assembly_ns,
          t.count_task_sum_ns,t.count_task_max_ns,t.fill_task_sum_ns,t.fill_task_max_ns,t.sort_comparisons,t.tasks,
          t.single_pass_ns,t.compact_ns,t.single_task_sum_ns,t.single_task_max_ns,t.compact_task_sum_ns,t.compact_task_max_ns};
}
inline CatalogueTimings sentinel() {
  CatalogueTimings t;
  t.prefix_ns=7; t.count_ns=11; t.fill_ns=13; t.single_pass_ns=17; t.compact_ns=19; t.tasks=23;
  return t;
}
inline CatalogueBall storage_ball() {
  return {{SiteIdx{1},SiteIdx{2},SiteIdx{kNone},SiteIdx{kNone}},LevelRank{0},1,7,2};
}
inline const std::array<SiteIdx,8> ids{SiteIdx{0},SiteIdx{1},SiteIdx{2},SiteIdx{3},
                                      SiteIdx{4},SiteIdx{5},SiteIdx{6},SiteIdx{7}};
inline Outcome append(SinglePassOutput& output, MemoryBudget& budget) {
  return output.append(storage_ball(),num::Level{},std::span(ids).first(1),std::span(ids).subspan(1),budget);
}
}  // namespace single_test
