// Pilote de la fin d'etage, ecrit une fois pour l'executeur Pool (voie CPU) et l'executeur CUDA (voie appareil) :
//   1. rangs lexicographiques des sites (tri par base de (x, y, z)) ;
//   2. par boule : niveau exact (mots), cle F3, cle des positions de S* ;
//   3. tri par base stable par positions, puis par cle F3 : ordre (cle F3, positions) ;
//   4. chaque paire de voisins non certainement ordonnee par F4 est comparee en exact (ChainKernel) ; une chaine de
//      voisins incertains mal ordonnee est retriee en exact sur l'hote (repli compte, finish_repair.hpp), puis tout est
//      reverifie ;
//   5. rangs denses (somme prefixe des debuts de niveau), CSR des populations I puis U (somme prefixe des longueurs),
//      boules canoniques, niveau du premier element de chaque rang ;
//   6. table S* -> boule : tri par base des (S*[0], S*[1], S*[2], S*[3]) et dichotomie des debuts de ligne ;
//   7. sorties en flux (finish_outputs.hpp) vers des Buffer hote de taille exacte : reserves a l'avance par la voie
//      appareil (sorties anticipees, niveaux compris des que leur nombre est connu), sinon ici ; un tableau de la
//      voie CPU de taille exacte est adopte sans copie ; niveaux materialises en num::Level au fil du flux.
// Refus : catalogue_invariant (enregistrement ou support invalide, niveau nul, doublon, comptes incoherents, sortie
// anticipee d'une autre taille), index_overflow_u32, memory_budget ; rien n'est publie.
#pragma once

#include "catalogue/finish_kernels.hpp"
#include "catalogue/finish_outputs.hpp"
#include "catalogue/finish_repair.hpp"

namespace mhgp12::catalogue_detail::fin {

template <class B>
struct FinishArrays {
  RadixArrays<B, Key2> sites, positions, table;
  RadixArrays<B, u64> keys;
  typename B::template Array<u32> lexrank, verdict, fault, pick;
  typename B::template Array<LevelWords> levels, level_of_rank, pick_levels;
  typename B::template Array<u64> fkey, flag, before, offset, table_offset;
  typename B::template Array<Key2> pkey, pick_pkey;
  typename B::template Array<CatalogueBall> balls;
  typename B::template Array<SiteIdx> values;
  ScanArrays<B> scan;
};

// Entree : memoire de l'executeur ; enregistrements en SiteIdx globaux, population de chaque boule a son decalage.
struct FinishInput {
  const u32 *x, *y, *z;
  u32 sites;
  const BallRecord* records;
  const SiteIdx* population;
  u64 balls, incidences;
};

// Diagnostics physiques de la fin d'etage (jamais dans une empreinte) ; durees des etapes nettes des transferts de
// l'executeur (comptes a part par son TransferMeter, CST-0235).
struct FinishStats {
  u64 chains_repaired = 0, chain_elements = 0;
  u64 keys_ns = 0, sort_ns = 0, check_ns = 0, emit_ns = 0, table_ns = 0, take_ns = 0;
};

template <class B>
Outcome finish_reserve(B& b, FinishArrays<B>& a, const FinishInput& in) noexcept {
  const u64 n = in.balls;
  MHGP12_TRY(radix_reserve(b, a.sites, in.sites));
  MHGP12_TRY(radix_reserve(b, a.positions, n));
  MHGP12_TRY(radix_reserve(b, a.keys, n));
  MHGP12_TRY(radix_reserve(b, a.table, n));
  MHGP12_TRY(b.ensure(a.lexrank, in.sites));
  MHGP12_TRY(b.ensure(a.verdict, n));
  MHGP12_TRY(b.ensure(a.fault, 1));
  MHGP12_TRY(b.ensure(a.levels, n));
  MHGP12_TRY(b.ensure(a.fkey, n));
  MHGP12_TRY(b.ensure(a.flag, n));
  MHGP12_TRY(b.ensure(a.before, n));
  MHGP12_TRY(b.ensure(a.offset, n + 1));
  MHGP12_TRY(b.ensure(a.table_offset, u64{in.sites} + 1));
  MHGP12_TRY(b.ensure(a.pkey, n));
  MHGP12_TRY(b.ensure(a.balls, n));
  MHGP12_TRY(b.ensure(a.values, in.incidences));
  return b.put(a.fault, 0u, 0);
}

template <class B>
Outcome read_fault(B& b, FinishArrays<B>& a, u32 allowed) noexcept {
  const auto fault = b.read(a.fault, 0);
  if (!fault.ok()) return fault.outcome();
  return (fault.value() & ~allowed) == 0 ? Outcome{} : fail(Reason::catalogue_invariant);
}

// Etapes 1 a 3 : cles, puis ordre trie (cle F3, positions) ; rend l'indice du tampon de a.keys qui le porte.
template <class B>
Result<int> finish_order(B& b, FinishArrays<B>& a, const FinishInput& in, FinishStats& stats) noexcept {
  const u64 n = in.balls;
  const NetWatch<B> watch(b);
  MHGP12_TRY(b.launch(SiteKeyKernel{in.x, in.y, in.z, in.sites, a.sites.keys[0].data(), a.sites.vals[0].data()},
                      tiles_of(in.sites)));
  const auto cs = radix_sort(b, a.sites, in.sites, 16);
  if (!cs.ok()) return cs.outcome();
  MHGP12_TRY(b.launch(LexRankKernel{a.sites.vals[cs.value()].data(), in.sites, a.lexrank.data()}, tiles_of(in.sites)));
  const BallKeyKernel keys{in.x,           in.y,         in.z,         in.sites,
                           in.records,     n,            in.incidences, a.lexrank.data(),
                           width_of(in.sites), a.levels.data(), a.fkey.data(), a.pkey.data(),
                           a.positions.keys[0].data(), a.positions.vals[0].data(), a.fault.data()};
  MHGP12_TRY(b.launch(keys, tiles_of(n)));
  MHGP12_TRY(read_fault(b, a, 0));
  stats.keys_ns += watch.nanoseconds();
  const NetWatch<B> sort_watch(b);
  const auto cp = radix_sort(b, a.positions, n, 16);
  if (!cp.ok()) return cp.outcome();
  MHGP12_TRY(b.launch(GatherKeyKernel{a.positions.vals[cp.value()].data(), a.fkey.data(), n, a.keys.keys[0].data(),
                                      a.keys.vals[0].data()},
                      tiles_of(n)));
  const auto ck = radix_sort(b, a.keys, n, 8);
  if (!ck.ok()) return ck.outcome();
  MHGP12_TRY(b.sync());  // diagnostic : chaque etape porte son propre travail (CST-0235)
  stats.sort_ns += sort_watch.nanoseconds();
  return ck.value();
}

// Etape 4 : verification exacte des voisins, repli exact si une chaine est mal ordonnee, puis reverification.
template <class B>
Outcome finish_check(B& b, FinishArrays<B>& a, int ck, u64 n, MemoryBudget& budget, FinishStats& stats) noexcept {
  const NetWatch<B> watch(b);
  const ChainKernel check{a.keys.vals[ck].data(), a.keys.keys[ck].data(), a.levels.data(), a.pkey.data(), n,
                          a.flag.data(), a.verdict.data(), a.fault.data()};
  MHGP12_TRY(b.launch(check, tiles_of(n)));
  const auto fault = b.read(a.fault, 0);
  if (!fault.ok()) return fault.outcome();
  if ((fault.value() & ~u32{kFaultMisordered}) != 0) return fail(Reason::catalogue_invariant);
  if (fault.value() != 0) {
    MHGP12_TRY(finish_repair(b, a, ck, n, budget, stats.chains_repaired, stats.chain_elements));
    MHGP12_TRY(b.put(a.fault, 0u, 0));
    MHGP12_TRY(b.launch(check, tiles_of(n)));
    MHGP12_TRY(read_fault(b, a, 0));
  }
  stats.check_ns += watch.nanoseconds();
  return {};
}

// Etape 5 : rangs, CSR des populations, boules canoniques, niveaux des rangs ; rend le nombre de niveaux non nuls. Voie
// appareil (anticipate) : la sortie des niveaux est reservee des que leur nombre est connu et ses pages touchees
// pendant le noyau Emit.
template <class B>
Result<u64> finish_emit(B& b, FinishArrays<B>& a, const FinishInput& in, int ck, FinishOutput& out, bool anticipate,
                        MemoryBudget& budget, sched::Pool& pool, FinishStats& stats) noexcept {
  const u64 n = in.balls;
  const NetWatch<B> watch(b);
  const auto distinct = exclusive_scan(b, a.scan, a.flag.data(), a.before.data(), n);
  if (!distinct.ok()) return distinct.outcome();
  if (distinct.value() >= kNone) return fail(Reason::index_overflow_u32);
  MHGP12_TRY(b.ensure(a.level_of_rank, distinct.value()));  // apres le compte : taille exacte pour un tableau neuf
  const u32* order = a.keys.vals[ck].data();
  MHGP12_TRY(b.launch(LengthKernel{order, in.records, n, a.offset.data()}, tiles_of(n)));
  const auto total = exclusive_scan(b, a.scan, a.offset.data(), a.offset.data(), n);
  if (!total.ok()) return total.outcome();
  if (total.value() != in.incidences) return fail(Reason::catalogue_invariant);
  MHGP12_TRY(b.put(a.offset, total.value(), n));
  const EmitKernel emit{order,          in.records,      in.population, a.flag.data(),  a.before.data(),
                        a.offset.data(), a.levels.data(), n,             a.balls.data(), a.values.data(),
                        a.level_of_rank.data()};
  MHGP12_TRY(b.launch(emit, tiles_of(n)));
  if (anticipate && kAnticipateOutputs)  // pendant Emit
    MHGP12_TRY(prepare_one(out.levels, distinct.value() + 1, budget, pool, b.meter));
  MHGP12_TRY(b.sync());
  stats.emit_ns += watch.nanoseconds();
  return distinct.value();
}

// Etape 6 : table S* -> boule ; rend l'indice du tampon de a.table qui porte les BallIdx tries.
template <class B>
Result<int> finish_table(B& b, FinishArrays<B>& a, const FinishInput& in, FinishStats& stats) noexcept {
  const u64 n = in.balls;
  const NetWatch<B> watch(b);
  const u32 bits = width_of(in.sites);
  MHGP12_TRY(b.launch(TableKeyKernel{a.balls.data(), n, in.sites, bits, a.table.keys[0].data(), a.table.vals[0].data()},
                      tiles_of(n)));
  const auto ct = radix_sort(b, a.table, n, 16);
  if (!ct.ok()) return ct.outcome();
  MHGP12_TRY(b.launch(TableOffsetKernel{a.table.keys[ct.value()].data(), n, in.sites, bits, a.table_offset.data()},
                      tiles_of(u64{in.sites} + 1)));
  MHGP12_TRY(b.sync());
  stats.table_ns += watch.nanoseconds();
  return ct.value();
}

// Sortie d'un tableau de l'executeur vers le Buffer hote `out` de n elements : tableau adopte sans copie (executeur
// Pool, sortie non reservee, taille exacte), sinon segment de flux vers `out`, reserve ici s'il est vide ; une sortie
// anticipee d'une autre taille est un invariant viole.
template <class B, class A, class T>
Outcome take_segment(B& b, A& a, Buffer<T>& out, u64 n, MemoryBudget& budget, sched::Pool& pool, CopyChunk& copy,
                     StreamSegment* segments, u64& k) noexcept {
  static_assert(sizeof(*a.data()) == sizeof(T), "catalogue : sortie de meme taille d'element que son tableau");
  if (out.empty() && b.adopt(a, out, n)) return {};
  if (out.empty()) MHGP12_TRY(out.allocate(n, budget));
  if (out.size() != n) return fail(Reason::catalogue_invariant);
  copy = CopyChunk{reinterpret_cast<u8*>(out.data()), sizeof(T), &pool};
  segments[k++] = StreamSegment{a.data(), sizeof(T), n, &CopyChunk::consume, &copy, false};
  return {};
}

// Etape 7 : toutes les sorties en un flux (boules, decalages, populations, table, puis niveaux materialises).
template <class B>
Outcome finish_take(B& b, FinishArrays<B>& a, const FinishInput& in, u64 distinct, int ct, FinishOutput& out,
                    MemoryBudget& budget, sched::Pool& pool) noexcept {
  if (distinct + 1 > kNone) return fail(Reason::index_overflow_u32);
  if (out.levels.empty()) MHGP12_TRY(out.levels.allocate(distinct + 1, budget));
  if (out.levels.size() != distinct + 1) return fail(Reason::catalogue_invariant);
  out.levels[0] = num::Level{};
  CopyChunk copies[5] = {};
  StreamSegment segments[6] = {};
  u64 k = 0;
  MHGP12_TRY(take_segment(b, a.balls, out.balls, in.balls, budget, pool, copies[0], segments, k));
  MHGP12_TRY(take_segment(b, a.offset, out.offsets, in.balls + 1, budget, pool, copies[1], segments, k));
  MHGP12_TRY(take_segment(b, a.values, out.values, in.incidences, budget, pool, copies[2], segments, k));
  const u64 rows = u64{in.sites} + 1;
  MHGP12_TRY(take_segment(b, a.table_offset, out.table_offsets, rows, budget, pool, copies[3], segments, k));
  MHGP12_TRY(take_segment(b, a.table.vals[ct], out.table_values, in.balls, budget, pool, copies[4], segments, k));
  LevelChunk levels{out.levels.data() + 1, &pool};
  segments[k++] = StreamSegment{a.level_of_rank.data(), sizeof(LevelWords), distinct, &LevelChunk::consume, &levels,
                                true};
  return b.stream_out(std::span<const StreamSegment>(segments, k));
}

// Fin d'etage complete dans `out` (sorties anticipees par la voie appareil : anticipate, niveaux compris) ; les
// tableaux de l'executeur sont gardes d'un appel a l'autre (sauf ceux que l'executeur Pool adopte).
template <class B>
Outcome finish_stage(B& b, FinishArrays<B>& a, const FinishInput& in, FinishOutput& out, bool anticipate,
                     MemoryBudget& budget, sched::Pool& pool, FinishStats& stats) noexcept {
  if (in.balls >= kNone) return fail(Reason::index_overflow_u32);
  MHGP12_TRY(finish_reserve(b, a, in));
  const auto ck = finish_order(b, a, in, stats);
  if (!ck.ok()) return ck.outcome();
  MHGP12_TRY(finish_check(b, a, ck.value(), in.balls, budget, stats));
  const auto distinct = finish_emit(b, a, in, ck.value(), out, anticipate, budget, pool, stats);
  if (!distinct.ok()) return distinct.outcome();
  const auto ct = finish_table(b, a, in, stats);
  if (!ct.ok()) return ct.outcome();
  const NetWatch<B> watch(b);
  MHGP12_TRY(finish_take(b, a, in, distinct.value(), ct.value(), out, budget, pool));
  stats.take_ns += watch.nanoseconds();
  return {};
}

}  // namespace mhgp12::catalogue_detail::fin
