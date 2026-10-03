// Trace T2 puis descente T3 : choix deterministe, sans quotient local ni identite de terminal imposee par R2.
#include "tower/descent.hpp"
#include "tower/locate.hpp"

#include <algorithm>

namespace mhgp11::tower_detail {
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
Outcome add_all(DescentLedger& sum, const DescentLedger& one) noexcept {
  MHGP11_TRY(cell_add(sum.memo.queries, one.memo.queries));
  MHGP11_TRY(cell_add(sum.memo.lookups, one.memo.lookups));
  MHGP11_TRY(cell_add(sum.memo.hits, one.memo.hits));
  MHGP11_TRY(cell_add(sum.memo.misses, one.memo.misses));
  MHGP11_TRY(cell_add(sum.memo.collisions, one.memo.collisions));
  MHGP11_TRY(cell_add(sum.memo.insertions, one.memo.insertions));
  MHGP11_TRY(cell_add(sum.memo.evictions, one.memo.evictions));
  MHGP11_TRY(cell_add(sum.memo.suffix_hits, one.memo.suffix_hits));
  MHGP11_TRY(cell_add(sum.steps, one.steps));
  MHGP11_TRY(cell_add(sum.interior_steps, one.interior_steps));
  MHGP11_TRY(cell_add(sum.trace_steps, one.trace_steps));
  MHGP11_TRY(cell_add(sum.candidate_traces, one.candidate_traces));
  MHGP11_TRY(cell_add(sum.trace_meb_calls, one.trace_meb_calls));
  MHGP11_TRY(cell_add(sum.census_calls, one.census_calls));
  MHGP11_TRY(cell_add(sum.catalogue_hits, one.catalogue_hits));
  MHGP11_TRY(cell_add(sum.singleton_hits, one.singleton_hits));
  MHGP11_TRY(cell_add(sum.population_hits, one.population_hits));
  MHGP11_TRY(add_meb(sum.part_meb, one.part_meb));
  MHGP11_TRY(add_meb(sum.trace_meb, one.trace_meb));
  MHGP11_TRY(cell_add(sum.census.nodes, one.census.nodes));
  MHGP11_TRY(cell_add(sum.census.bounds, one.census.bounds));
  MHGP11_TRY(cell_add(sum.census.point_tests, one.census.point_tests));
  MHGP11_TRY(cell_add(sum.census.inside_blocks, one.census.inside_blocks));
  MHGP11_TRY(cell_add(sum.census.outside_blocks, one.census.outside_blocks));
  return cell_add(sum.census.passes, one.census.passes);
}
CellTrace combine(std::span<const SiteIdx> inner, std::span<const SiteIdx> selected) noexcept {
  CellTrace trace;
  trace.sites.fill(SiteIdx{kNone});
  trace.arity = static_cast<u8>(inner.size() + selected.size());
  std::merge(inner.begin(), inner.end(), selected.begin(), selected.end(), trace.sites.begin(),
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

Outcome add_descent(DescentLedger& sum, const DescentLedger& one) noexcept {
  auto staged = sum;
  MHGP11_TRY(add_all(staged, one));
  sum = staged;
  return {};
}

struct DescentBuilder {
  const FullDomain& domain;
  const LocatedView& located;
  u32 k;
  DescentLedger ledger;

  static DescentStep singleton(const BoundedMeb& meb) noexcept {
    // Appel apres validation de la partie de taille 1. FullDomain certifie les sites distincts :
    // beta=0, aucun interieur strict et seule coquille {site}. Conserver la representation du MEB.
    DescentLedger work;
    work.steps = 1;
    work.singleton_hits = 1;
    work.part_meb = meb.ledger();
    return DescentStep(meb.sphere().level(), combine({}, {}),
                       BirthSeed(meb.support()[0], std::nullopt, 1), work);
  }

  Result<DescentStep> terminal() const noexcept {
    const auto key = located.support();
    if (!key) return fail(Reason::tower_invariant);
    auto empty = combine({}, {});
    if (key->arity == 1) {
      if (k != 1 || !located.interior().empty() || located.shell().size() != 1)
        return fail(Reason::tower_invariant);
      return DescentStep(located.meb().sphere().level(), empty,
                         BirthSeed(located.shell()[0], std::nullopt, 1), ledger);
    }
    if (!located.ball()) return fail(Reason::tower_invariant);
    return DescentStep(located.meb().sphere().level(), empty,
                       BirthSeed(std::nullopt, located.ball(), static_cast<Order>(k)), ledger);
  }

  Result<DescentStep> strict_trace(u32 t) noexcept {
    const auto shell = located.shell();
    const u32 m = static_cast<u32>(shell.size());  // Cloud compte au plus kNone-1 sites.
    std::array<u32, kMaxMebSites> tuple{};
    std::array<SiteIdx, kMaxMebSites> selected{};
    for (u32 i = 0; i < t; ++i) tuple[i] = i;
    do {
      MHGP11_TRY(cell_add(ledger.candidate_traces, 1));
      for (u32 i = 0; i < t; ++i) selected[i] = shell[tuple[i]];
      const std::span<const SiteIdx> part{selected.data(), t};
      bool strict = t < located.support()->arity;
      if (!strict) {
        MHGP11_TRY(cell_add(ledger.trace_meb_calls, 1));
        auto meb = bounded_meb(domain.index().cloud(), part);
        if (!meb.ok()) return meb.outcome();
        MHGP11_TRY(add_meb(ledger.trace_meb, meb.value().ledger()));
        const int side = num::compare(meb.value().sphere().level(), located.meb().sphere().level());
        if (side > 0) return fail(Reason::tower_invariant);
        strict = side < 0;
      }
      if (strict) {
        ledger.trace_steps = 1;
        return DescentStep(located.meb().sphere().level(), combine(located.interior(), part),
                           std::nullopt, ledger);
      }
    } while (next_tuple(tuple, m, t));
    return terminal();
  }

  Result<DescentStep> run() noexcept {
    ledger.steps = 1;
    ledger.part_meb = located.meb().ledger();
    if (const auto* work = located.census_work()) { ledger.census_calls = 1; ledger.census = *work; }
    else ledger.catalogue_hits = 1;
    // Un hit complete peut avoir p>=k : le certificat geometrique precede la representation du census.
    if (located.interior().size() >= k) {
      ledger.interior_steps = 1;
      return DescentStep(located.meb().sphere().level(), combine(located.interior().first(k), {}),
                         std::nullopt, ledger);
    }
    if (located.kind() != CensusKind::complete || !located.support()) return fail(Reason::tower_invariant);
    const u32 t = k - static_cast<u32>(located.interior().size());  // 1<=t<=k<=12.
    if (t > located.shell().size()) return fail(Reason::tower_invariant);
    if (t == located.shell().size()) return terminal();
    return strict_trace(t);  // Fonctionne aussi hors fenetre de cellule et hors CatK.
  }
};

namespace {
struct StepQuery {
  const FullDomain& domain;
  u32 k;
  std::optional<DescentStep> result;
  static Outcome consume(void* raw, const LocatedView& view) noexcept {
    auto& q = *static_cast<StepQuery*>(raw);
    auto made = DescentBuilder{q.domain, view, q.k, {}}.run();
    if (!made.ok()) return made.outcome();
    q.result.emplace(made.value());  // Valeurs uniquement, copie avant fermeture du callback.
    return {};
  }
};
}  // namespace
Result<DescentStep> descent_step(const FullDomain& domain, std::span<const SiteIdx> part, u32 k,
                                MemoryBudget& budget, CensusWorkspace* scratch) noexcept {
  if (k == 1) {
    // Meme priorite que visit_located_part : identite, ordre, cardinal, puis validations du MEB.
    if (scratch != nullptr && !scratch->belongs_to(domain.index())) return fail(Reason::parameter_out_of_range);
    if (k > domain.catalogue().kmax()) return fail(Reason::kmax_out_of_range);
    if (part.size() != k) return fail(Reason::parameter_out_of_range);
    auto meb = bounded_meb(domain.index().cloud(), part);
    if (!meb.ok()) return meb.outcome();
    return DescentBuilder::singleton(meb.value());
  }
  StepQuery query{domain, k, std::nullopt};
  MHGP11_TRY(visit_located_part(domain, part, k, budget, scratch, &query, StepQuery::consume));
  if (!query.result) return fail(Reason::tower_invariant);
  return *query.result;
}

Result<DescentResult> descend(const FullDomain& domain, std::span<const SiteIdx> part, u32 k,
                            MemoryBudget& budget, CensusWorkspace* scratch) noexcept {
  auto first = descent_step(domain, part, k, budget, scratch);
  if (!first.ok()) return first.outcome();
  const num::Level initial = first.value().level();
  auto current = first.value();
  DescentLedger ledger;
  for (;;) {
    MHGP11_TRY(add_descent(ledger, current.ledger()));
    if (current.seed()) return DescentResult(initial, current.level(), *current.seed(), ledger);
    auto next = descent_step(domain, current.next().part(), k, budget, scratch);
    if (!next.ok()) return next.outcome();
    if (num::compare(next.value().level(), current.level()) >= 0) return fail(Reason::tower_invariant);
    current = next.value();
  }
}

}  // namespace mhgp11::tower_detail
