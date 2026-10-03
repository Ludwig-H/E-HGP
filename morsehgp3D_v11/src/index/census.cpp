// Deux parcours sans pile : comptage saturant, puis allocation exacte et remplissage transactionnel.
#include "index/index.hpp"
#include "index/access.hpp"

#include <algorithm>

namespace mhgp11 {
namespace {

struct Pass {
  u32 threshold;
  bool keep_shell;
  std::span<SiteIdx> interior, shell;
  bool fill;
  CensusLedger& ledger;
  u64 p = 0, m = 0;

  void accept_range(u32 begin, u32 end) noexcept {
    const u64 added = std::min<u64>(end - begin, threshold - p);
    if (fill)
      for (u64 i = 0; i < added; ++i) interior[p + i] = SiteIdx{static_cast<u32>(begin + i)};
    p += added;
  }

  Outcome points(const Cloud& cloud, u32 begin, u32 end, const num::Sphere& sphere) noexcept {
    for (u32 i = begin; i < end && p < threshold; ++i) {
      ++ledger.point_tests;
      auto point = num::Point::make(cloud.x()[i], cloud.y()[i], cloud.z()[i]);
      if (!point.ok()) return point.outcome();
      auto side = num::side(sphere, point.value());
      if (!side.ok()) return side.outcome();
      if (side.value() < 0) {
        if (fill) interior[p] = SiteIdx{i};
        ++p;
      } else if (side.value() == 0 && keep_shell) {
        if (fill) shell[m] = SiteIdx{i};
        ++m;
      }
    }
    return {};
  }

  Outcome walk(const GlobalIndex& index, const num::Sphere& sphere) noexcept {
    ++ledger.passes;
    const auto nodes = index_detail::Access::nodes(index);
    for (u64 cursor = 0; cursor < nodes.size() && p < threshold;) {
      const auto& node = nodes[cursor];
      ++ledger.nodes;
      ++ledger.bounds;
      auto signs = num::power_bound_signs(sphere, node.box);  // signes seuls : aucune conversion Wide native
      if (!signs.ok()) return signs.outcome();
      if (signs.value().lower > 0) {
        ++ledger.outside_blocks;
        cursor = node.escape;
      } else if (signs.value().upper < 0) {
        ++ledger.inside_blocks;
        accept_range(node.begin, node.end);
        cursor = node.escape;
      } else if (node.end - node.begin <= index.leaf_size()) {
        MHGP11_TRY(points(index.cloud(), node.begin, node.end, sphere));
        cursor = node.escape;
      } else {
        ++cursor;
      }
    }
    return {};
  }
};

}  // namespace

Result<Census> census(const GlobalIndex& index, const num::Sphere& sphere, u32 threshold,
                      MemoryBudget& budget) noexcept {
  if (threshold == 0) return fail(Reason::parameter_out_of_range);
  if (index.cloud().sites() == 0) return fail(Reason::empty_input);
  Census result;
  Pass count{threshold, true, {}, {}, false, result.ledger_};
  MHGP11_TRY(count.walk(index, sphere));
  const bool saturated = count.p == threshold;
  const u64 shell_size = saturated ? 0 : count.m;
  // I et U disjoints, leurs tailles cumulees <= sites<2^32. Toutes les allocations sont payees ici.
  MHGP11_TRY(budget.admit((count.p + shell_size) * sizeof(SiteIdx)));
  MHGP11_TRY(result.interior_.allocate(count.p, budget));
  MHGP11_TRY(result.shell_.allocate(shell_size, budget));
  Pass fill{threshold, !saturated, result.interior_.span(), result.shell_.span(), true, result.ledger_};
  MHGP11_TRY(fill.walk(index, sphere));
  result.kind_ = saturated ? CensusKind::saturated : CensusKind::complete;
  return result;
}

}  // namespace mhgp11
