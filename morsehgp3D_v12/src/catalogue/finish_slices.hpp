// Fin d'etage par tranches de cles (tranche T1-d, ARCHITECTURE.md paragraphe 4.6), pilote ecrit une fois pour
// l'executeur Pool (voie CPU, portes de l'hote) et l'executeur CUDA (voie appareil). Entree : l'arene de l'hote (lots
// de la voie CPU, ou lots rapatries par la voie appareil en flux) ; sortie : les memes tableaux que finish_stage, a
// l'octet pres. Sequence (parties de l'hote dans slices.hpp) :
//   1. rangs lexicographiques des sites sur l'executeur, une fois ;
//   2. passe des cles : lot par lot, cle F3 de chaque boule (FKeyKernel, memes controles que BallKeyKernel), rapatriee ;
//   3. plan des tranches (coupes certaines, memoire de travail sous slice_bytes), repartition stable des boules ;
//   4. par tranche, dans l'ordre des cles : rassemblement et envoi (populations rebasees), cles, tris, verification et
//      repli exacts, rangs et CSR locaux (finish_driver.hpp), rebasage (rang, incidence) sur l'executeur, sorties en
//      flux directement dans les tableaux finaux, mots des niveaux gardes par tranche ;
//   5. niveaux finaux (leur nombre est alors connu), table S* -> boule sur l'hote.
// L'arene de l'hote est rendue apres la derniere tranche. Memoire de l'executeur : rangs des sites, plus la plus grande
// tranche (kSliceBytesPerBall par boule et kSliceBytesPerIncidence par incidence), quelle que soit la taille du nuage.
// Refus : ceux de finish_stage (catalogue_invariant, index_overflow_u32, memory_budget), plus memory_budget pour un
// plateau de cles incoupable plus lourd que slice_bytes. Rien n'est publie sur un refus.
#pragma once

#include <utility>
#include <vector>

#include "catalogue/finish_driver.hpp"
#include "catalogue/slices.hpp"

namespace mhgp12::catalogue_detail::fin {

// Cle F3 de chaque boule d'un lot de l'arene ; memes controles d'enregistrement que BallKeyKernel (populations du lot).
struct FKeyKernel {
  const u32 *x, *y, *z;
  u64 sites;
  const BallRecord* records;
  u64 balls, incidences;
  u64* fkey;
  u32* fault;
  struct Shared {};
  MHGP12_HD void one(u64 i) const {
    const BallRecord& r = records[i];
    const u32 q = r.qmin;
    bool bad = q < 2 || q > 4 || r.population > incidences || u64{r.p} + r.m > incidences - r.population;
    u32 pts[4][3] = {};
    const u32* p[4] = {pts[0], pts[1], pts[2], pts[3]};
    for (u32 k = 0; k < q && !bad; ++k) {
      const u32 s = r.support[k];
      bad = s >= sites;
      if (bad) break;
      pts[k][0] = x[s];
      pts[k][1] = y[s];
      pts[k][2] = z[s];
    }
    LevelWords level{};
    bool faulty = bad;
    if (!bad) ball_level(p, q, level, faulty);
    fkey[i] = faulty ? 0u : level_key_bits(level);
    if (faulty) simt::global_or(fault, kFaultBall);
  }
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < balls) one(i);
      }
    }
  }
};

// Rebasage d'une tranche : rang de base ajoute au rang de chaque boule, incidence de base a chaque decalage CSR.
struct RebaseKernel {
  CatalogueBall* balls;
  u64* offset;
  u64 n, ranks, incidences;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n) {
          balls[i].rank = static_cast<LevelRank>(static_cast<u32>(static_cast<u32>(balls[i].rank) + ranks));
          offset[i] += incidences;
        }
      }
    }
  }
};

// Entree de la voie par tranches : nuage sur l'executeur, arene de l'hote (rendue a la fin).
struct SliceInput {
  const u32 *x, *y, *z;
  u32 sites;
  ArenaView arena;
  std::vector<Chunk>* chunks;
};

// Arene d'une tranche sur l'executeur.
template <class B>
struct SliceArrays {
  typename B::template Array<BallRecord> records;
  typename B::template Array<SiteIdx> population;
};

struct SliceBases {
  u64 balls = 0, incidences = 0, ranks = 0;
};

// Passe des cles : cle F3 de chaque boule, par morceaux d'au plus `piece` boules d'un lot (memoire de l'executeur
// dans celle d'une tranche) ; keys[i] pour la boule globale i.
template <class B>
Outcome slice_keys(B& b, FinishArrays<B>& a, SliceArrays<B>& s, const SliceInput& in, u64 piece, Buffer<u64>& keys,
                   MemoryBudget& budget) noexcept {
  MHGP12_TRY(keys.allocate(in.arena.balls, budget));
  for (u64 c = 0; c < in.arena.chunks.size(); ++c) {
    const Chunk& chunk = in.arena.chunks[c];
    for (u64 first = 0; first < chunk.records.size(); first += piece) {
      const u64 m = chunk.records.size() - first < piece ? chunk.records.size() - first : piece;
      MHGP12_TRY(b.ensure(s.records, m));
      MHGP12_TRY(b.ensure(a.fkey, m));
      MHGP12_TRY(b.upload(s.records, chunk.records.data() + first, m, 0));
      MHGP12_TRY(b.launch(FKeyKernel{in.x, in.y, in.z, in.sites, s.records.data(), m, chunk.population.size(),
                                     a.fkey.data(), a.fault.data()},
                          tiles_of(m)));
      MHGP12_TRY(read_fault(b, a, 0));
      MHGP12_TRY(b.download(keys.data() + in.arena.ball_at[c] + first, a.fkey, m, 0));
    }
  }
  return {};
}

inline Outcome keep_words(std::vector<Buffer<LevelWords>>& words, Buffer<LevelWords>&& w) noexcept {
  try {
    words.push_back(std::move(w));
  } catch (const std::bad_alloc&) {
    return fail(Reason::memory_budget);
  }
  return {};
}

// Sorties d'une tranche en flux, a leurs bases dans les tableaux finaux ; mots des niveaux dans `w`.
template <class B>
Outcome slice_take(B& b, FinishArrays<B>& a, u64 m, u64 inc, u64 distinct, const SliceBases& base, FinishOutput& out,
                   Buffer<LevelWords>& w, MemoryBudget& budget, sched::Pool& pool) noexcept {
  MHGP12_TRY(w.allocate(distinct, budget));
  CopyChunk copies[4] = {{reinterpret_cast<u8*>(out.balls.data() + base.balls), sizeof(CatalogueBall), &pool},
                         {reinterpret_cast<u8*>(out.offsets.data() + base.balls), sizeof(u64), &pool},
                         {reinterpret_cast<u8*>(out.values.data() + base.incidences), sizeof(SiteIdx), &pool},
                         {reinterpret_cast<u8*>(w.data()), sizeof(LevelWords), &pool}};
  const StreamSegment segments[4] = {
      {a.balls.data(), sizeof(CatalogueBall), m, &CopyChunk::consume, &copies[0], false},
      {a.offset.data(), sizeof(u64), m, &CopyChunk::consume, &copies[1], false},
      {a.values.data(), sizeof(SiteIdx), inc, &CopyChunk::consume, &copies[2], false},
      {a.level_of_rank.data(), sizeof(LevelWords), distinct, &CopyChunk::consume, &copies[3], false}};
  return b.stream_out(std::span<const StreamSegment>(segments, 4));
}

// Une tranche : rassemblement, envoi, fin d'etage locale, rebasage, sorties.
template <class B>
Outcome slice_finish(B& b, FinishArrays<B>& a, SliceArrays<B>& s, const SliceInput& in, std::span<const u32> ids,
                     SliceBases& base, FinishOutput& out, std::vector<Buffer<LevelWords>>& words, MemoryBudget& budget,
                     sched::Pool& pool, FinishStats& stats) noexcept {
  const u64 m = ids.size();
  u64 inc = 0;
  {
    Buffer<BallRecord> records;
    Buffer<SiteIdx> population;
    MHGP12_TRY(gather_slice(in.arena, ids, records, population, budget, pool));
    inc = population.size();
    MHGP12_TRY(b.ensure(s.records, m));
    MHGP12_TRY(b.ensure(s.population, inc));
    MHGP12_TRY(b.upload(s.records, records.data(), m, 0));
    if (inc != 0) MHGP12_TRY(b.upload(s.population, population.data(), inc, 0));
  }
  MHGP12_TRY(finish_reserve_balls(b, a, m, inc));
  const FinishInput fi{in.x, in.y, in.z, in.sites, s.records.data(), s.population.data(), m, inc};
  const NetWatch<B> watch(b);
  const auto ck = finish_ball_order(b, a, fi, stats, watch);
  if (!ck.ok()) return ck.outcome();
  MHGP12_TRY(finish_check(b, a, ck.value(), m, budget, stats));
  FinishOutput unused;
  const auto distinct = finish_emit(b, a, fi, ck.value(), unused, false, budget, pool, stats);
  if (!distinct.ok()) return distinct.outcome();
  if (distinct.value() >= kNone - base.ranks) return fail(Reason::index_overflow_u32);
  const NetWatch<B> take_watch(b);
  MHGP12_TRY(b.launch(RebaseKernel{a.balls.data(), a.offset.data(), m, base.ranks, base.incidences}, tiles_of(m)));
  Buffer<LevelWords> w;
  MHGP12_TRY(slice_take(b, a, m, inc, distinct.value(), base, out, w, budget, pool));
  MHGP12_TRY(keep_words(words, std::move(w)));
  stats.take_ns += take_watch.nanoseconds();
  base.balls += m;
  base.incidences += inc;
  base.ranks += distinct.value();
  return {};
}

// Plan et repartition (passe des cles comprise) ; les cles sont rendues avant la premiere tranche.
template <class B>
Outcome slice_plan(B& b, FinishArrays<B>& a, SliceArrays<B>& s, const SliceInput& in, u64 slice_bytes, SlicePlan& plan,
                   Buffer<u32>& ids, Buffer<u64>& start, MemoryBudget& budget, sched::Pool& pool,
                   FinishStats& stats) noexcept {
  const NetWatch<B> watch(b);
  Buffer<u64> keys;
  const u64 piece = slice_bytes / kSliceBytesPerBall;
  MHGP12_TRY(slice_keys(b, a, s, in, piece > 1024 ? piece : 1024, keys, budget));
  MHGP12_TRY(plan_slices(in.arena, keys.span(), slice_bytes, plan, budget, pool));
  MHGP12_TRY(assign_slices(keys.span(), plan, ids, start, budget, pool));
  stats.keys_ns += watch.nanoseconds();
  return {};
}

// Tableaux de l'executeur dimensionnes une fois pour la plus grande tranche (aucune croissance dans la boucle).
template <class B>
Outcome slice_reserve(B& b, FinishArrays<B>& a, SliceArrays<B>& s, const SlicePlan& plan) noexcept {
  u64 balls = 0, incidences = 0;
  for (u64 j = 0; j < plan.count; ++j) {
    balls = plan.balls[j] > balls ? plan.balls[j] : balls;
    incidences = plan.incidences[j] > incidences ? plan.incidences[j] : incidences;
  }
  if (balls == 0) return {};
  MHGP12_TRY(b.ensure(s.records, balls));
  MHGP12_TRY(b.ensure(s.population, incidences));
  MHGP12_TRY(b.ensure(a.level_of_rank, balls));
  return finish_reserve_balls(b, a, balls, incidences);
}

// Mots des niveaux de chaque tranche, dans l'ordre des rangs, et nombre de rangs.
struct SliceWords {
  std::vector<Buffer<LevelWords>> words;
  u64 ranks = 0;
};

// Tranches de la fin d'etage dans `out` (boules, decalages, populations), mots des niveaux dans `w` ; arene de l'hote
// rendue a la fin. slice_bytes nul : la moitie de la place libre de l'executeur une fois les tableaux des sites
// reserves (voie appareil).
template <class B>
Outcome slices_run(B& b, FinishArrays<B>& a, SliceArrays<B>& s, const SliceInput& in, FinishOutput& out,
                   u64 slice_bytes, MemoryBudget& budget, sched::Pool& pool, FinishStats& stats,
                   SliceWords& w) noexcept {
  const u64 n = in.arena.balls, total = in.arena.incidences;
  if (n >= kNone) return fail(Reason::index_overflow_u32);
  MHGP12_TRY(finish_reserve_sites(b, a, in.sites));
  {
    const NetWatch<B> watch(b);
    MHGP12_TRY(finish_lexrank(b, a, in.x, in.y, in.z, in.sites));
    stats.keys_ns += watch.nanoseconds();
  }
  if (slice_bytes == 0) slice_bytes = b.room() / 2;  // la moitie de la place libre : croissance et marges
  SlicePlan plan;
  Buffer<u32> ids;
  Buffer<u64> start;
  MHGP12_TRY(slice_plan(b, a, s, in, slice_bytes, plan, ids, start, budget, pool, stats));
  MHGP12_TRY(slice_reserve(b, a, s, plan));
  MHGP12_TRY(out.balls.allocate(n, budget));
  MHGP12_TRY(out.offsets.allocate(n + 1, budget));
  MHGP12_TRY(out.values.allocate(total, budget));
  SliceBases base;
  for (u64 j = 0; j < plan.count; ++j)
    MHGP12_TRY(slice_finish(b, a, s, in, ids.span().subspan(start[j], start[j + 1] - start[j]), base, out, w.words,
                            budget, pool, stats));
  if (base.balls != n || base.incidences != total) return fail(Reason::catalogue_invariant);
  out.offsets[n] = total;
  w.ranks = base.ranks;
  stats.slices += plan.count;
  if (in.chunks != nullptr) in.chunks->clear();  // arene de l'hote rendue avant les niveaux et la table
  return {};
}

// Niveaux finaux (leur nombre est alors connu) et table S* -> boule, sur l'hote ; la voie CPU y arrive une fois les
// tableaux de l'executeur rendus.
inline Outcome slices_close(const SliceInput& in, FinishOutput& out, SliceWords& w, MemoryBudget& budget,
                            sched::Pool& pool, FinishStats& stats) noexcept {
  MHGP12_TRY(materialize_levels(w.words, w.ranks, out.levels, budget, pool));
  const Stopwatch table_watch;
  MHGP12_TRY(host_table(out.balls.span(), in.sites, out.table_offsets, out.table_values, budget, pool));
  stats.table_ns += table_watch.nanoseconds();
  return {};
}

// Fin d'etage complete par tranches dans `out` (voie appareil, portes).
template <class B>
Outcome finish_sliced(B& b, FinishArrays<B>& a, SliceArrays<B>& s, const SliceInput& in, FinishOutput& out,
                      u64 slice_bytes, MemoryBudget& budget, sched::Pool& pool, FinishStats& stats) noexcept {
  SliceWords w;
  MHGP12_TRY(slices_run(b, a, s, in, out, slice_bytes, budget, pool, stats, w));
  return slices_close(in, out, w, budget, pool, stats);
}

}  // namespace mhgp12::catalogue_detail::fin
