// T2 : classer sans traces possedees ; premier temoin strict seulement, aucun quotient des incidences FULL.
#include "tower/cells.hpp"

#include <algorithm>
#include <limits>

namespace mhgp11::tower_detail {
namespace {
Outcome add_work(MebLedger& sum, const MebLedger& one) noexcept {
  MHGP11_TRY(cell_add(sum.presentations, one.presentations));
  MHGP11_TRY(cell_add(sum.nondegenerate, one.nondegenerate));
  MHGP11_TRY(cell_add(sum.positive, one.positive));
  MHGP11_TRY(cell_add(sum.containing, one.containing));
  MHGP11_TRY(cell_add(sum.comparisons, one.comparisons));
  MHGP11_TRY(cell_add(sum.diameter_pairs, one.diameter_pairs));
  return cell_add(sum.point_tests, one.point_tests);
}
bool advance(std::array<u32, kMaxMebSites>& tuple, u32 m, u32 t) noexcept {
  for (u32 j = t; j != 0; --j) {
    const u32 i = j - 1;
    if (tuple[i] == m - t + i) continue;
    ++tuple[i];
    for (u32 k = i + 1; k < t; ++k) tuple[k] = tuple[k - 1] + 1;
    return true;
  }
  return false;
}
}  // namespace

struct CellClassifier {
  const FullDomain& domain;
  BallIdx ball;
  const CatalogueBall& data;
  u32 t;
  u64 combinations;

  Result<CellClassification> run() const noexcept {
    ClassificationLedger ledger;
    ledger.combinations = combinations;
    if (t == data.m) return CellClassification(CellKind::birth, ledger);
    if (t < data.qmin) return CellClassification(CellKind::strict_traces, ledger);
    const auto shell = domain.catalogue().shell(ball);
    const auto& level = domain.catalogue().levels()[idx(data.rank)];
    std::array<u32, kMaxMebSites> tuple{};
    std::array<SiteIdx, kMaxMebSites> selected{};
    for (u32 i = 0; i < t; ++i) tuple[i] = i;
    do {
      for (u32 i = 0; i < t; ++i) selected[i] = shell[tuple[i]];
      MHGP11_TRY(cell_add(ledger.examined, 1));
      MHGP11_TRY(cell_add(ledger.meb_calls, 1));
      auto meb = bounded_meb(domain.index().cloud(), {selected.data(), t});
      if (!meb.ok()) return meb.outcome();
      MHGP11_TRY(add_work(ledger.meb, meb.value().ledger()));
      const int side = num::compare(meb.value().sphere().level(), level);
      if (side > 0) return fail(Reason::tower_invariant);
      if (side < 0) return CellClassification(CellKind::strict_traces, ledger);
    } while (advance(tuple, data.m, t));
    return CellClassification(CellKind::birth, ledger);
  }
};

Result<CellClassification> classify_cell(const FullDomain& domain, BallIdx ball, Order order) noexcept {
  const auto& cat = domain.catalogue();
  if (idx(ball) >= cat.balls() || order == 0 || order > cat.kmax()) return fail(Reason::parameter_out_of_range);
  const auto& data = cat.balls_data()[idx(ball)];
  const u64 lo = u64{data.p} + data.qmin - 1, hi = std::min(u64{data.p} + data.m, u64{cat.kmax()});
  if (order < lo || order > hi) return fail(Reason::parameter_out_of_range);
  const u32 t = static_cast<u32>(order) - data.p;
  auto count = cell_binomial(data.m, t);
  if (!count.ok()) return count.outcome();
  // Domaine d'acceptation inchange : le rejeu exhaustif conserve ses deux passes ulterieures.
  if (count.value() > std::numeric_limits<u64>::max() / 2) return fail(Reason::tower_capacity);
  return CellClassifier{domain, ball, data, t, count.value()}.run();
}

}  // namespace mhgp11::tower_detail
