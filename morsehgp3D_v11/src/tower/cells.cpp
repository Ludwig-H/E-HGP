// T2 : beta(A)<lambda equivaut a la separabilite de A dans la coquille, donc de I union A.
// Cette MEB(A) classe la trace seulement ; elle n'est jamais la MEB de I union A pour une descente.
#include "tower/cells.hpp"

#include <algorithm>
#include <limits>

namespace mhgp11::tower_detail {

Result<u64> cell_binomial(u32 m, u32 t) noexcept {
  if (t > m || t > kMaxMebSites) return fail(Reason::parameter_out_of_range);
  t = std::min(t, m - t);
  u64 result = 1;
  for (u32 i = 1; i <= t; ++i) {
    // result<=2^64-1 et facteur<=2^32-1 : produit<2^96 avant la division exacte par i<=12.
    const u128 next = u128{result} * (m - t + i) / i;
    if (next > std::numeric_limits<u64>::max()) return fail(Reason::tower_capacity);
    result = static_cast<u64>(next);
  }
  return result;
}

Outcome cell_same_pass(u64 counted, u64 filled, const CellLedger& first, const CellLedger& second) noexcept {
  if (counted != filled || first != second) return fail(Reason::tower_invariant);
  return {};
}

namespace {
Outcome add_meb(MebLedger& sum, const MebLedger& one) noexcept {
  MHGP11_TRY(cell_add(sum.presentations, one.presentations));
  MHGP11_TRY(cell_add(sum.nondegenerate, one.nondegenerate));
  MHGP11_TRY(cell_add(sum.positive, one.positive));
  MHGP11_TRY(cell_add(sum.containing, one.containing));
  MHGP11_TRY(cell_add(sum.comparisons, one.comparisons));
  MHGP11_TRY(cell_add(sum.diameter_pairs, one.diameter_pairs));
  return cell_add(sum.point_tests, one.point_tests);
}

CellTrace combine(std::span<const SiteIdx> inner, std::span<const SiteIdx> shell) noexcept {
  CellTrace trace;
  trace.sites.fill(SiteIdx{kNone});
  trace.arity = static_cast<u8>(inner.size() + shell.size());
  std::merge(inner.begin(), inner.end(), shell.begin(), shell.end(), trace.sites.begin(),
             [](SiteIdx a, SiteIdx b) noexcept { return idx(a) < idx(b); });
  return trace;
}

bool next_tuple(std::array<u32, kMaxMebSites>& tuple, u32 m, u32 t) noexcept {
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

struct CellBuilder {
  const FullDomain& domain;
  BallIdx ball;
  Order order;
  MemoryBudget& budget;
  const CatalogueBall& data;
  std::span<const SiteIdx> inner, shell;
  u32 t;
  u64 combinations;

  Result<bool> strict(std::span<const SiteIdx> part, CellLedger& ledger) const noexcept {
    MHGP11_TRY(cell_add(ledger.trace_tests, 1));
    if (t < data.qmin) return true;  // Aucun sous-ensemble de cardinal<qmin ne contient le centre.
    MHGP11_TRY(cell_add(ledger.meb_calls, 1));
    auto meb = bounded_meb(domain.index().cloud(), part);
    if (!meb.ok()) return meb.outcome();
    MHGP11_TRY(add_meb(ledger.meb, meb.value().ledger()));
    const int side = num::compare(meb.value().sphere().level(), domain.catalogue().levels()[idx(data.rank)]);
    if (side > 0) return fail(Reason::tower_invariant);
    return side < 0;
  }

  Result<u64> pass(std::span<CellTrace> output, bool filling, CellLedger& ledger) const noexcept {
    ledger.combinations = combinations;
    ledger.passes = 1;
    std::array<u32, kMaxMebSites> tuple{};
    std::array<SiteIdx, kMaxMebSites> selected{};
    for (u32 i = 0; i < t; ++i) tuple[i] = i;
    u64 count = 0;
    do {
      for (u32 i = 0; i < t; ++i) selected[i] = shell[tuple[i]];
      const std::span<const SiteIdx> part{selected.data(), t};
      auto accepted = strict(part, ledger);
      if (!accepted.ok()) return accepted.outcome();
      if (accepted.value()) {
        if (filling) {
          if (count >= output.size()) return fail(Reason::tower_invariant);
          output[count] = combine(inner, part);
        }
        MHGP11_TRY(cell_add(count, 1));
      }
    } while (next_tuple(tuple, data.m, t));
    return count;
  }

  Result<LocalCell> run() const noexcept {
    Buffer<CellTrace> traces;
    CellLedger ledger;
    ledger.combinations = combinations;
    const bool regular = data.m == data.qmin;
    if (t == data.m) return LocalCell(std::move(traces), ball, order, regular, ledger);
    if (regular) {
      // La fenetre impose t=qmin-1. Les q faces strictes, en ordre lexicographique, sont analytiques.
      MHGP11_TRY(budget.admit(sizeof(CellTrace) * u64{data.m}));
      MHGP11_TRY(traces.allocate(data.m, budget));
      std::array<SiteIdx, 4> selected{};
      for (u32 r = 0; r < data.m; ++r) {
        const u32 omitted = data.m - 1 - r;
        u32 written = 0;
        for (u32 j = 0; j < data.m; ++j) if (j != omitted) selected[written++] = shell[j];
        traces[r] = combine(inner, {selected.data(), t});
      }
      return LocalCell(std::move(traces), ball, order, true, ledger);
    }
    CellLedger first, second;
    auto counted = pass({}, false, first);
    if (!counted.ok()) return counted.outcome();
    if (counted.value() > Buffer<CellTrace>::kMaxCount) return fail(Reason::tower_capacity);
    MHGP11_TRY(budget.admit(sizeof(CellTrace) * counted.value()));
    MHGP11_TRY(traces.allocate(counted.value(), budget));
    auto filled = pass(traces.span(), true, second);
    if (!filled.ok()) return filled.outcome();
    MHGP11_TRY(cell_same_pass(counted.value(), filled.value(), first, second));
    ledger = first;
    MHGP11_TRY(cell_add(ledger.passes, second.passes));
    MHGP11_TRY(cell_add(ledger.trace_tests, second.trace_tests));
    MHGP11_TRY(cell_add(ledger.meb_calls, second.meb_calls));
    MHGP11_TRY(add_meb(ledger.meb, second.meb));
    return LocalCell(std::move(traces), ball, order, false, ledger);
  }
};

Result<LocalCell> build_cell(const FullDomain& domain, BallIdx ball, Order order, MemoryBudget& budget) noexcept {
  const auto& cat = domain.catalogue();
  if (idx(ball) >= cat.balls() || order == 0 || order > cat.kmax()) return fail(Reason::parameter_out_of_range);
  const auto& data = cat.balls_data()[idx(ball)];
  const u64 lo = u64{data.p} + data.qmin - 1, hi = std::min(u64{data.p} + data.m, u64{cat.kmax()});
  if (order < lo || order > hi) return fail(Reason::parameter_out_of_range);
  const u32 t = static_cast<u32>(order) - data.p;  // 1<=t<=m et t<=K<=12, par la fenetre certifiee.
  auto count = cell_binomial(data.m, t);
  if (!count.ok()) return count.outcome();
  // Deux passes comptent toutes les traces, sans debordement meme avant la premiere iteration.
  if (count.value() > std::numeric_limits<u64>::max() / 2) return fail(Reason::tower_capacity);
  return CellBuilder{domain, ball, order, budget, data, cat.interior(ball), cat.shell(ball), t, count.value()}.run();
}

}  // namespace mhgp11::tower_detail
