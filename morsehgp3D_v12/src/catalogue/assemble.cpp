// Fin d'etage de la voie CPU : lots de la LeafStage rassembles en tableaux contigus (copie parallele, un lot par
// tache), fin d'etage partagee avec la voie appareil (finish_driver.hpp : cles, tri par base, verification exacte des
// voisins, rangs et CSR par sommes prefixes, table S* -> boule) jouee par l'executeur Pool, puis publication
// (Assembly::adopt, commune aux deux voies) : niveaux exacts materialises pour les rangs distincts, en parallele. Les
// sorties ne dependent pas du nombre de fils ; un refus ne publie rien.
#include <cstring>

#include "catalogue/exec_host.hpp"
#include "catalogue/finish_driver.hpp"
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

// Niveaux exacts des rangs 1..L-1 depuis leurs mots : num::Level::make, memes numerateur et denominateur non reduits
// que num::Sphere::through (finish_level.hpp).
struct Materialize {
  const fin::LevelWords* words;
  num::Level* levels;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& m = *static_cast<Materialize*>(context);
    for (u64 r = begin; r < end; ++r) {
      num::Wide<fin::kNumWords> n{};
      num::Wide<fin::kDenWords> d{};
      for (int i = 0; i < fin::kNumWords; ++i) n.words[i] = m.words[r].n[i];
      for (int i = 0; i < fin::kDenWords; ++i) d.words[i] = m.words[r].d[i];
      const auto level = num::Level::make(n, d);
      if (!level.ok()) return fail(Reason::catalogue_invariant);
      m.levels[r + 1] = level.value();
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

Result<Catalogue> Assembly::adopt(fin::FinishOutput& out, Order kmax, const CatalogueLedger& ledger,
                                  MemoryBudget& budget, sched::Pool& pool) noexcept {
  Catalogue result;
  const u64 distinct = out.levels.size();
  if (distinct + 1 > kNone) return fail(Reason::index_overflow_u32);
  MHGP12_TRY(result.levels_.allocate(distinct + 1, budget));
  result.levels_[0] = num::Level{};
  Materialize materialize{out.levels.data(), result.levels_.data()};
  MHGP12_TRY(pool.parallel_for(distinct, 4096, &materialize, &Materialize::body));
  out.levels.reset();
  result.balls_.swap(out.balls);
  result.population_.off.swap(out.offsets);
  result.population_.val.swap(out.values);
  result.table_.off.swap(out.table_offsets);
  result.table_.val.swap(out.table_values);
  result.kmax_ = kmax;
  result.ledger_ = ledger;
  return result;
}

Result<Catalogue> Assembly::finish(const Cloud& cloud, const CatalogueParams& params, std::vector<Chunk>& chunks,
                                   u64 balls, const CatalogueLedger& ledger, MemoryBudget& budget, sched::Pool& pool,
                                   CatalogueDiagnostics& diagnostics) noexcept {
  if (balls >= kNone) return fail(Reason::index_overflow_u32);
  Stopwatch gather_watch;
  Buffer<u64> record_at, population_at;
  u64 incidences = 0;
  MHGP12_TRY(chunk_offsets(chunks, record_at, population_at, balls, incidences, budget));
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
  auto out = fin::finish_stage(executor, arrays, in, budget, stats);
  if (!out.ok()) return out.outcome();
  Stopwatch adopt_watch;
  auto result = adopt(out.value(), static_cast<Order>(params.kmax), ledger, budget, pool);
  if (!result.ok()) return result.outcome();
  diagnostics.levels_ns = stats.keys_ns;
  diagnostics.sort_ns = stats.sort_ns;
  // voie CPU : les copies de l'executeur Pool (sorties prises, lectures) restent dans l'assemblage (transfer_ns nul)
  diagnostics.assemble_ns =
      gather_ns + stats.check_ns + stats.emit_ns + stats.take_ns + executor.meter.ns + adopt_watch.nanoseconds();
  diagnostics.table_ns = stats.table_ns;
  diagnostics.chains_repaired = stats.chains_repaired;
  diagnostics.chain_elements = stats.chain_elements;
  return result;
}

}  // namespace mhgp12::catalogue_detail
