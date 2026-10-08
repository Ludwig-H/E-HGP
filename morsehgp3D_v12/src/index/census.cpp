// Deux parcours sans pile : comptage saturant, puis allocation exacte et remplissage transactionnel.
// Le meme parcours sert le census generique (LatticeSphere) et le census garde d'une boule certifiee (GuardedSphere),
// a plat (T2-d-B2, index/bounds.hpp).
#include "index/index.hpp"
#include "index/access.hpp"
#include "index/bounds.hpp"

#include <algorithm>

namespace mhgp12 {
namespace {

template <class Bounds>
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

  Outcome points(const Cloud& cloud, u32 begin, u32 end, const Bounds& lattice) noexcept {
    const u32 *x = cloud.x().data(), *y = cloud.y().data(), *z = cloud.z().data();
    for (u32 i = begin; i < end && p < threshold; ++i) {
      ++ledger.point_tests;
      int side = 0;
      MHGP12_TRY(lattice.side_at(x[i], y[i], z[i], side, ledger));
      if (side < 0) {
        if (fill) interior[p] = SiteIdx{i};
        ++p;
      } else if (side == 0 && keep_shell) {
        if (fill) shell[m] = SiteIdx{i};
        ++m;
      }
    }
    return {};
  }

  // Une preparation des bornes par parcours (lattice), sites de l'index entiers.
  Outcome walk(const GlobalIndex& index, const Bounds& lattice) noexcept {
    ++ledger.passes;
    const auto nodes = index_detail::Access::nodes(index);
    for (u64 cursor = 0; cursor < nodes.size() && p < threshold;) {
      const auto& node = nodes[cursor];
      ++ledger.nodes;
      ++ledger.bounds;
      num::PowerBoundSigns signs;
      MHGP12_TRY(lattice.bound_signs(node.box, signs, ledger));
      if (signs.lower > 0) {
        ++ledger.outside_blocks;
        cursor = node.escape;
      } else if (signs.upper < 0) {
        ++ledger.inside_blocks;
        accept_range(node.begin, node.end);
        cursor = node.escape;
      } else if (node.end - node.begin <= index.leaf_size()) {
        MHGP12_TRY(points(index.cloud(), node.begin, node.end, lattice));
        cursor = node.escape;
      } else {
        ++cursor;
      }
    }
    return {};
  }
};

// Les deux passes du census, sur les tableaux du resultat ; rend le genre du certificat.
template <class Bounds, class Ball>
Result<CensusKind> run_census(const GlobalIndex& index, const Ball& ball, u32 threshold, MemoryBudget& budget,
                              CensusLedger& ledger, Buffer<SiteIdx>& interior, Buffer<SiteIdx>& shell) noexcept {
  if (threshold == 0) return fail(Reason::parameter_out_of_range);
  if (index.cloud().sites() == 0) return fail(Reason::empty_input);
  Pass<Bounds> count{threshold, true, {}, {}, false, ledger};
  {
    const Bounds lattice(ball);
    MHGP12_TRY(count.walk(index, lattice));
    lattice.flush(ledger);
  }
  const bool saturated = count.p == threshold;
  const u64 shell_size = saturated ? 0 : count.m;
  // I et U disjoints, leurs tailles cumulees <= sites<2^32. Toutes les allocations sont payees ici.
  MHGP12_TRY(budget.admit((count.p + shell_size) * sizeof(SiteIdx)));
  MHGP12_TRY(interior.allocate(count.p, budget));
  MHGP12_TRY(shell.allocate(shell_size, budget));
  Pass<Bounds> fill{threshold, !saturated, interior.span(), shell.span(), true, ledger};
  {
    const Bounds lattice(ball);
    MHGP12_TRY(fill.walk(index, lattice));
    lattice.flush(ledger);
  }
  return saturated ? CensusKind::saturated : CensusKind::complete;
}

}  // namespace

Result<Census> census(const GlobalIndex& index, const num::Sphere& sphere, u32 threshold,
                      MemoryBudget& budget) noexcept {
  Census result;
  auto kind = run_census<index_detail::GenericBounds>(index, sphere, threshold, budget, result.ledger_,
                                                      result.interior_, result.shell_);
  if (!kind.ok()) return kind.outcome();
  result.kind_ = kind.value();
  return result;
}

Result<Census> census(const GlobalIndex& index, const num::CertifiedBall& ball, u32 threshold,
                      MemoryBudget& budget) noexcept {
  Census result;
  auto kind = run_census<index_detail::GuardedBounds>(index, ball, threshold, budget, result.ledger_,
                                                      result.interior_, result.shell_);
  if (!kind.ok()) return kind.outcome();
  result.kind_ = kind.value();
  return result;
}

}  // namespace mhgp12
