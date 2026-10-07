// Fin d'etage sur l'hote : boules des lots mises a plat, niveaux exacts (num::Sphere::through du support, meme arite
// que la presentation emettrice, donc le niveau d'emission de la v11), cles F3, ordre canonique, rangs de niveau, CSR
// des populations (I puis U), table S* -> boule. Toutes les reservations sont controlees dans le budget ; un refus ne
// publie rien (le Catalogue n'est rendu qu'entier).
#include "catalogue/sort.hpp"
#include "sched/sched.hpp"

namespace mhgp12::catalogue_detail {
namespace {

constexpr u64 kGrain = 4096;

Result<num::Point> point_of(const Cloud& cloud, u32 site) noexcept {
  auto made = num::Point::make(cloud.x()[site], cloud.y()[site], cloud.z()[site]);
  if (!made.ok()) return fail(Reason::catalogue_invariant);
  return made.value();
}

// Niveau exact d'une boule emise : la sphere de son S* (2 a 4 sites), fabriquee par num.
Result<num::Level> level_of(const Cloud& cloud, const BallRecord& r) noexcept {
  std::array<num::Point, 4> p{};
  for (u32 k = 0; k < r.qmin; ++k) {
    auto made = point_of(cloud, r.support[k]);
    if (!made.ok()) return made.outcome();
    p[k] = made.value();
  }
  auto sphere = r.qmin == 2 ? num::Sphere::through(p[0], p[1])
                : r.qmin == 3 ? num::Sphere::through(p[0], p[1], p[2])
                              : num::Sphere::through(p[0], p[1], p[2], p[3]);
  if (!sphere.ok()) return sphere.outcome();
  if (!sphere.value()) return fail(Reason::catalogue_invariant);
  return sphere.value()->level();
}

struct Gather {
  const Cloud& cloud;
  std::span<const BallRecord> records;
  num::Level* levels;
  double* keys;
  static Outcome levels_body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& g = *static_cast<Gather*>(context);
    for (u64 i = begin; i < end; ++i) {
      auto level = level_of(g.cloud, g.records[i]);
      if (!level.ok()) return level.outcome();
      g.levels[i] = level.value();
      g.keys[i] = level_key(level.value());
    }
    return {};
  }
};

// Rangs denses des niveaux dans l'ordre canonique ; nombre de niveaux distincts, niveau nul compris. Deux boules de
// meme niveau et de meme S* sont un doublon d'emission : invariant.
Result<u64> rank_levels(const Cloud& cloud, std::span<const BallRecord> records, std::span<const num::Level> levels,
                        std::span<const double> keys, std::span<const u32> order, Buffer<u32>& ranks) noexcept {
  u64 count = 1;
  const num::Level zero;
  for (u64 i = 0; i < order.size(); ++i) {
    const u32 b = order[i];
    bool distinct = true;  // niveau strictement superieur au precedent
    if (i == 0) {
      if (num::compare(levels[b], zero) <= 0) return fail(Reason::catalogue_invariant);
    } else {
      const u32 a = order[i - 1];
      int before = level_key_order(keys[a], keys[b]);
      if (before == 0) before = num::compare(levels[a], levels[b]);
      if (before > 0) return fail(Reason::catalogue_invariant);  // ordre non trie
      distinct = before < 0;
      if (!distinct && compare_support_positions(cloud, records[a].support, records[b].support) >= 0)
        return fail(Reason::catalogue_invariant);  // meme niveau : S* strictement croissants, jamais un doublon
    }
    count += distinct ? 1 : 0;
    ranks[b] = static_cast<u32>(count - 1);
  }
  if (count > kNone) return fail(Reason::index_overflow_u32);
  return count;
}

struct Fill {
  std::span<const BallRecord> records;
  std::span<const u32> order, ranks;
  const std::vector<Chunk>* chunks;
  CatalogueBall* balls;
  const u64* offsets;
  SiteIdx* values;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    auto& f = *static_cast<Fill*>(context);
    for (u64 i = begin; i < end; ++i) {
      const BallRecord& r = f.records[f.order[i]];
      CatalogueBall& ball = f.balls[i];
      for (u32 k = 0; k < 4; ++k) ball.support[k] = make_id<SiteIdx>(r.support[k]);
      ball.rank = make_id<LevelRank>(f.ranks[f.order[i]]);
      ball.p = r.p;
      ball.m = r.m;
      ball.qmin = r.qmin;
      const SiteIdx* source = (*f.chunks)[r.chunk].population.data() + r.population;
      const u64 length = u64{r.p} + r.m;
      if (f.offsets[i + 1] - f.offsets[i] != length) return fail(Reason::catalogue_invariant);
      for (u64 j = 0; j < length; ++j) f.values[f.offsets[i] + j] = source[j];
    }
    return {};
  }
};

}  // namespace

Result<Catalogue> Assembly::finish(const Cloud& cloud, const CatalogueParams& params, std::vector<Chunk>& chunks,
                                   u64 balls, const CatalogueLedger& ledger, MemoryBudget& budget, sched::Pool& pool,
                                   CatalogueDiagnostics& diagnostics) noexcept {
  if (balls >= kNone) return fail(Reason::index_overflow_u32);
  Buffer<BallRecord> records;
  MHGP12_TRY(records.allocate(balls, budget));
  u64 at = 0, incidences = 0;
  for (Chunk& chunk : chunks) {
    if (chunk.records.size() > balls - at) return fail(Reason::catalogue_invariant);
    for (u64 i = 0; i < chunk.records.size(); ++i) records[at + i] = chunk.records[i];
    at += chunk.records.size();
    incidences += chunk.population.size();
    chunk.records.reset();  // les populations restent jusqu'a la copie finale
  }
  if (at != balls) return fail(Reason::catalogue_invariant);
  Buffer<num::Level> levels;
  Buffer<double> keys;
  MHGP12_TRY(levels.allocate(balls, budget));
  MHGP12_TRY(keys.allocate(balls, budget));
  Stopwatch level_watch;
  Gather gather{cloud, records.span(), levels.data(), keys.data()};
  MHGP12_TRY(pool.parallel_for(balls, kGrain, &gather, &Gather::levels_body));
  diagnostics.levels_ns = level_watch.nanoseconds();
  Stopwatch sort_watch;
  auto order = sort_balls(cloud, records.span(), levels.span(), keys.span(), budget, pool);
  if (!order.ok()) return order.outcome();
  diagnostics.sort_ns = sort_watch.nanoseconds();
  Stopwatch assemble_watch;
  Buffer<u32> ranks;
  MHGP12_TRY(ranks.allocate(balls, budget));
  const auto distinct = rank_levels(cloud, records.span(), levels.span(), keys.span(), order.value().span(), ranks);
  if (!distinct.ok()) return distinct.outcome();
  Catalogue result;
  MHGP12_TRY(result.balls_.allocate(balls, budget));
  MHGP12_TRY(result.levels_.allocate(distinct.value(), budget));
  MHGP12_TRY(result.population_.off.allocate(balls + 1, budget));
  MHGP12_TRY(result.population_.val.allocate(incidences, budget));
  result.levels_[0] = num::Level{};
  result.population_.off[0] = 0;
  for (u64 i = 0; i < balls; ++i) {
    const u32 b = order.value()[i];
    // Niveau d'un rang : celui de sa premiere boule dans l'ordre canonique (ecriture non reduite de la v11).
    if (i == 0 || ranks[order.value()[i - 1]] != ranks[b]) result.levels_[ranks[b]] = levels[b];
    result.population_.off[i + 1] = result.population_.off[i] + records[b].p + records[b].m;
  }
  if (result.population_.off[balls] != incidences) return fail(Reason::catalogue_invariant);
  Fill fill{records.span(), order.value().span(), ranks.span(), &chunks, result.balls_.data(),
            result.population_.off.data(), result.population_.val.data()};
  MHGP12_TRY(pool.parallel_for(balls, kGrain, &fill, &Fill::body));
  diagnostics.assemble_ns = assemble_watch.nanoseconds();
  Stopwatch table_watch;
  MHGP12_TRY(build_table(result.table_, result.balls_.span(), cloud.sites(), budget));
  diagnostics.table_ns = table_watch.nanoseconds();
  result.kmax_ = static_cast<Order>(params.kmax);
  result.ledger_ = ledger;
  return result;
}

}  // namespace mhgp12::catalogue_detail
