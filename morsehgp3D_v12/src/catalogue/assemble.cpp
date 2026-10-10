// Fin d'etage de la voie CPU : lots de la LeafStage rassembles en tableaux contigus (copie parallele, un lot par
// tache), fin d'etage partagee avec la voie appareil (finish_driver.hpp : cles, tri par base, verification exacte des
// voisins, rangs et CSR par sommes prefixes, table S* -> boule, sorties en flux dont les niveaux exacts materialises
// pour les rangs distincts, en parallele) jouee par l'executeur Pool, puis publication (Assembly::adopt, commune aux
// deux voies). Les sorties ne dependent pas du nombre de fils ; un refus ne publie rien.
#include <cstring>

#include "catalogue/exec_host.hpp"
#include "catalogue/finish_driver.hpp"
#include "catalogue/finish_slices.hpp"
#include "sched/sched.hpp"

namespace mhgp12::catalogue_detail {
namespace {

// Lot c -> enregistrements a record_at[c], populations a population_at[c] (decalages rendus absolus).
struct Gather {
  std::vector<Chunk>* chunks;
  const u64* record_at;
  const u64* population_at;
  BallRecord* records;
  SiteIdx* population;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& g = *static_cast<Gather*>(context);
    for (u64 c = begin; c < end; ++c) {
      const Chunk& chunk = (*g.chunks)[c];
      for (u64 i = 0; i < chunk.records.size(); ++i) {
        BallRecord r = chunk.records[i];
        r.population += g.population_at[c];
        r.chunk = 0;
        g.records[g.record_at[c] + i] = r;
      }
      if (!chunk.population.empty())
        std::memcpy(g.population + g.population_at[c], chunk.population.data(),
                    chunk.population.size() * sizeof(SiteIdx));
    }
    return {};
  }
};

// Decalages des lots ; sommes controlees.
Outcome chunk_offsets(const std::vector<Chunk>& chunks, Buffer<u64>& record_at, Buffer<u64>& population_at,
                      u64 balls, u64& incidences, MemoryBudget& budget) noexcept {
  MHGP12_TRY(record_at.allocate(chunks.size(), budget));
  MHGP12_TRY(population_at.allocate(chunks.size(), budget));
  u64 at = 0;
  incidences = 0;
  for (u64 c = 0; c < chunks.size(); ++c) {
    record_at[c] = at;
    population_at[c] = incidences;
    if (chunks[c].records.size() > balls - at) return fail(Reason::catalogue_invariant);
    at += chunks[c].records.size();
    if (__builtin_add_overflow(incidences, chunks[c].population.size(), &incidences))
      return fail(Reason::catalogue_invariant);
  }
  return at == balls ? Outcome{} : fail(Reason::catalogue_invariant);
}

}  // namespace

Result<Catalogue> Assembly::adopt(fin::FinishOutput& out, Order kmax, const CatalogueLedger& ledger) noexcept {
  Catalogue result;
  if (out.levels.empty() || out.levels.size() > kNone) return fail(Reason::catalogue_invariant);
  result.levels_.swap(out.levels);
  result.balls_.swap(out.balls);
  result.population_.off.swap(out.offsets);
  result.population_.val.swap(out.values);
  result.table_.off.swap(out.table_offsets);
  result.table_.val.swap(out.table_values);
  result.table_keys_.swap(out.table_keys);
  // Une cle par case de la table, toutes voies (complete, par tranches, appareil) ; sinon rien n'est publie.
  if (result.table_keys_.size() != result.table_.val.size() || result.table_.off.empty())
    return fail(Reason::catalogue_invariant);
  result.table_bits_ = fin::width_of(result.table_.off.size() - 1);
  result.kmax_ = kmax;
  result.ledger_ = ledger;
  return result;
}

// Voie CPU par tranches (tranche T1-d) : l'arene des lots est lue sans etre rassemblee ; la memoire de travail des
// tranches est la moitie de ce qui reste du budget une fois comptees les sorties tenues pendant les tranches ; les
// tableaux de l'executeur sont rendus avant les niveaux et la table.
Outcome Assembly::sliced(const Cloud& cloud, std::vector<Chunk>& chunks, u64 incidences, MemoryBudget& budget,
                         sched::Pool& pool, fin::FinishOutput& out, fin::FinishStats& stats, u64& moved) noexcept {
  Buffer<u64> ball_at, incidence_at;
  fin::ArenaView view;
  MHGP12_TRY(fin::arena_bases(chunks, ball_at, incidence_at, view, budget));
  const u64 room = budget.limit() - budget.used(), host = fin::sliced_host_bytes(view.balls, incidences);
  const u64 slice_bytes = room > host ? (room - host) / 2 : fin::kSliceBytesPerBall;
  const fin::SliceInput in{cloud.x().data(), cloud.y().data(), cloud.z().data(), cloud.sites(), view, &chunks};
  fin::SliceWords words;
  {
    PoolExecutor executor{pool, budget};
    fin::FinishArrays<PoolExecutor> arrays;
    fin::SliceArrays<PoolExecutor> slices;
    MHGP12_TRY(fin::slices_run(executor, arrays, slices, in, out, slice_bytes, budget, pool, stats, words));
    moved = executor.meter.off_stage();
  }
  return fin::slices_close(in, out, words, budget, pool, stats);
}

Result<Catalogue> Assembly::finish(const Cloud& cloud, const CatalogueParams& params, std::vector<Chunk>& chunks,
                                   u64 balls, const CatalogueLedger& ledger, MemoryBudget& budget, sched::Pool& pool,
                                   CatalogueDiagnostics& diagnostics) noexcept {
  if (balls >= kNone) return fail(Reason::index_overflow_u32);
  Stopwatch gather_watch;
  Buffer<u64> record_at, population_at;
  u64 incidences = 0;
  MHGP12_TRY(chunk_offsets(chunks, record_at, population_at, balls, incidences, budget));
  // Choix de la voie (tranche T1-d) : la voie complete (rassemblement, fin d'etage sur toute l'arene, sorties) quand
  // son pic estime tient dans la place libre, comme avant ; sinon la voie par tranches ; si celle-ci refuse faute de
  // memoire avant d'avoir rendu l'arene et que le minimum du pic de la voie complete tient, la voie complete est
  // jouee : une limite qui porte le pic de la voie complete la sert toujours.
  const u64 room = budget.limit() - budget.used();
  if (fin::full_finish_upper(balls, incidences, cloud.sites()) > room) {
    const u64 lots = chunks.size();
    Outcome refusal;
    {
      fin::FinishStats stats;
      fin::FinishOutput out;
      u64 moved = 0;
      refusal = sliced(cloud, chunks, incidences, budget, pool, out, stats, moved);
      if (refusal.ok()) {
        auto result = adopt(out, static_cast<Order>(params.kmax), ledger);
        if (!result.ok()) return result.outcome();
        const u64 elapsed = gather_watch.nanoseconds(), parts = stats.keys_ns + stats.sort_ns + stats.table_ns;
        publish_stats(stats, elapsed > parts ? elapsed - parts : 0, diagnostics);
        return result;
      }
    }
    if (refusal.reason != Reason::memory_budget || chunks.size() != lots ||
        fin::full_finish_floor(balls, incidences, cloud.sites()) > room)
      return refusal;
  }
  PoolExecutor executor{pool, budget};
  FrontArray<BallRecord> records;
  FrontArray<SiteIdx> population;
  MHGP12_TRY(executor.ensure(records, balls));
  MHGP12_TRY(executor.ensure(population, incidences));
  Gather gather{&chunks, record_at.data(), population_at.data(), records.data(), population.data()};
  MHGP12_TRY(pool.parallel_for(chunks.size(), 1, &gather, &Gather::body));
  for (Chunk& chunk : chunks) {
    chunk.records.reset();
    chunk.population.reset();
  }
  const u64 gather_ns = gather_watch.nanoseconds();
  fin::FinishArrays<PoolExecutor> arrays;
  const fin::FinishInput in{cloud.x().data(), cloud.y().data(), cloud.z().data(), cloud.sites(),
                            records.data(),   population.data(), balls,           incidences};
  fin::FinishStats stats;
  fin::FinishOutput out;
  MHGP12_TRY(fin::finish_stage(executor, arrays, in, out, false, budget, pool, stats));
  Stopwatch adopt_watch;
  auto result = adopt(out, static_cast<Order>(params.kmax), ledger);
  if (!result.ok()) return result.outcome();
  // voie CPU : les copies de l'executeur Pool (sorties prises, lectures) et la materialisation des niveaux restent
  // dans l'assemblage (transfer_ns et publish_ns nuls)
  publish_stats(stats, gather_ns + stats.check_ns + stats.emit_ns + stats.take_ns + executor.meter.off_stage() +
                           adopt_watch.nanoseconds(),
                diagnostics);
  return result;
}

void Assembly::publish_stats(const fin::FinishStats& stats, u64 assemble_ns, CatalogueDiagnostics& d) noexcept {
  d.levels_ns = stats.keys_ns;
  d.sort_ns = stats.sort_ns;
  d.assemble_ns = assemble_ns;
  d.table_ns = stats.table_ns;
  d.chains_repaired = stats.chains_repaired;
  d.chain_elements = stats.chain_elements;
  d.finish_slices = stats.slices;
}

}  // namespace mhgp12::catalogue_detail
