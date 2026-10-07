// Frontiere du catalogue : validation avant toute allocation, refus transactionnels, sequence parcours -> feuilles en
// flux -> fin d'etage. Une std::bad_alloc a la frontiere devient memory_budget (guarded) ; rien n'est publie sur un
// refus.
#include "catalogue/internal.hpp"
#include "sched/sched.hpp"

namespace mhgp12 {
namespace {

using catalogue_detail::LeafCounts;

CatalogueLedger ledger_of(const catalogue_detail::TraversalLedger& t, const LeafCounts& c) noexcept {
  using namespace catalogue_detail;
  CatalogueLedger l;
  l.nodes = t.nodes;
  l.leaves = t.leaves;
  l.filter_tests = t.filter_tests;
  l.max_leaf = t.max_leaf;
  l.max_depth = t.max_depth;
  l.dominance_tests = c.c[kDominanceTests];
  l.prefixes = c.c[kPrefixes];
  l.judged = c.c[kJudged];
  l.census_tests = c.c[kCensusTests];
  l.emitted = c.c[kEmitted];
  l.incidences = c.c[kIncidences];
  l.q4_candidates = c.c[kQ4Candidates];
  l.q4_levels = c.c[kQ4Levels];
  l.region_pair_tests = c.c[kRegionPairTests];
  l.region_pair_rejects = c.c[kRegionPairRejects];
  l.region_line_tests = c.c[kRegionLineTests];
  l.region_line_rejects = c.c[kRegionLineRejects];
  l.region_line_evaluations = c.c[kRegionLineEvaluations];
  l.region_line_cache_hits = c.c[kRegionLineCacheHits];
  l.region_line_fallbacks = c.c[kRegionLineFallbacks];
  return l;
}

Result<Catalogue> build(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget, sched::Pool& pool,
                        CatalogueDiagnostics* diagnostics) noexcept {
  using namespace catalogue_detail;
  CatalogueDiagnostics diag;
  LeafStage stage(cloud, params, budget, pool);
  const bfs::Params traversal{static_cast<u32>(params.kmax), params.leaf_size, params.max_leaf,
                              static_cast<u32>(kCoordBits)};
  TraversalLedger walked;
  TraversalDiagnostics front;
  Stopwatch watch;
  MHGP12_TRY(traverse(cloud, traversal, budget, pool, stage, walked, front));
  const u64 elapsed = watch.nanoseconds();
  const LeafTotals& leaves = stage.totals();
  const CatalogueLedger ledger = ledger_of(walked, leaves.counts);
  if (ledger.emitted != stage.balls()) return fail(Reason::catalogue_invariant);
  diag.levels = front.levels;
  diag.tasks = front.tasks;
  diag.candidates = front.candidates;
  diag.leaves_narrow = leaves.narrow;
  diag.leaves_medium = leaves.medium;
  diag.leaves_wide = leaves.wide;
  diag.leaves_exact = leaves.exact;
  diag.leaves_virtual_warp = leaves.virtual_warp;
  diag.leaves_rewritten = leaves.rewritten;
  diag.max_leaf_span = leaves.max_span;
  diag.count_ns = leaves.count_ns;
  diag.fill_ns = leaves.fill_ns;
  diag.traversal_ns = elapsed - leaves.count_ns - leaves.fill_ns;
  auto result = Assembly::finish(cloud, params, stage.chunks(), stage.balls(), ledger, budget, pool, diag);
  if (!result.ok()) return result.outcome();
  if (result.value().population().size() != ledger.incidences) return fail(Reason::catalogue_invariant);
  diag.peak_bytes = budget.peak();
  if (diagnostics != nullptr) *diagnostics = diag;
  return result;
}

}  // namespace

Outcome check_catalogue_params(const CatalogueParams& params) noexcept {
  if (params.kmax < 1 || params.kmax > static_cast<int>(catalogue_detail::bfs::kMaxOrder))
    return fail(Reason::kmax_out_of_range);
  if (params.max_leaf < 1 || params.max_leaf > kCatalogueMaxLeaf || params.leaf_size < static_cast<u32>(params.kmax) + 3 ||
      params.leaf_size > params.max_leaf)
    return fail(Reason::parameter_out_of_range);
  return {};
}

Result<Catalogue> build_catalogue(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget,
                                  sched::Pool& pool, CatalogueDiagnostics* diagnostics) noexcept {
  MHGP12_TRY(check_catalogue_params(params));
  if (cloud.sites() == 0) return fail(Reason::empty_input);
  for (const u32 weight : cloud.w())
    if (weight != 1) return fail(Reason::multiplicity_unsupported);  // decision D8 : refus par defaut
  return guarded([&]() { return build(cloud, params, budget, pool, diagnostics); });
}

}  // namespace mhgp12
