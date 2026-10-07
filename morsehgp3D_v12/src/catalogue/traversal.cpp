// Parcours en largeur de la voie CPU de reference : le pilote des niveaux de traversal_driver.hpp joue par l'executeur
// Pool (exec_host.hpp), commun a la voie appareil ; les feuilles de chaque niveau sont remises au consommateur avant
// le niveau suivant (arene reutilisee).
#include "catalogue/exec_host.hpp"
#include "catalogue/traversal_driver.hpp"

namespace mhgp12::catalogue_detail {
namespace {

// Crochet de la voie CPU : feuilles du niveau remises au consommateur, arene reutilisee d'un niveau a l'autre.
struct ConsumerHook {
  LeafConsumer& consumer;
  u64 leaf_base() const noexcept { return 0; }
  u64 leaf_site_base() const noexcept { return 0; }
  Outcome after_level(const bfs::Level& lv, const bfs::LevelTotals& t, u32 depth) noexcept {
    return consumer.consume({lv.leaves, static_cast<std::size_t>(t.f[3])},
                            {lv.leaf_sites, static_cast<std::size_t>(t.f[4])}, depth);
  }
  Outcome refuse(Outcome refusal) const noexcept { return refusal; }
  Outcome finish() const noexcept { return {}; }
};

}  // namespace

Outcome traverse(const Cloud& cloud, const bfs::Params& params, MemoryBudget& budget, sched::Pool& pool,
                 LeafConsumer& consumer, TraversalLedger& ledger, TraversalDiagnostics& diagnostics) noexcept {
  if (cloud.sites() == 0) return fail(Reason::empty_input);
  PoolExecutor executor{pool, budget};
  Front<PoolExecutor> front;
  ConsumerHook hook{consumer};
  const TraversalInput in{cloud.x().data(), cloud.y().data(), cloud.z().data(), cloud.sites(),
                          traversal_root(cloud.x(), cloud.y(), cloud.z())};
  TraversalLedger walked;
  TraversalDiagnostics diag;
  MHGP12_TRY(traverse_with(executor, front, in, params, hook, walked, diag));
  diag.front_peak_bytes = budget.peak();
  ledger = walked;
  diagnostics = diag;
  return {};
}

}  // namespace mhgp12::catalogue_detail
