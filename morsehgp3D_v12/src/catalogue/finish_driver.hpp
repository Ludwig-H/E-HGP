// Pilote de la fin d'etage, ecrit une fois pour l'executeur Pool (voie CPU) et l'executeur CUDA (voie appareil) :
//   1. rangs lexicographiques des sites (tri par base de (x, y, z)) ;
//   2. par boule : niveau exact (mots), cle F3, cle des positions de S* ;
//   3. tri par base stable par positions, puis par cle F3 : ordre (cle F3, positions) ;
//   4. chaque paire de voisins non certainement ordonnee par F4 est comparee en exact (ChainKernel) ; une chaine de
//      voisins incertains mal ordonnee est retriee en exact sur l'hote (repli, compte), puis tout est reverifie ;
//   5. rangs denses (somme prefixe des debuts de niveau), CSR des populations I puis U (somme prefixe des longueurs),
//      boules canoniques, niveau du premier element de chaque rang ;
//   6. table S* -> boule : tri par base des (S*[0], S*[1], S*[2], S*[3]) et dichotomie des debuts de ligne.
// Sorties rendues dans des Buffer hote de taille exacte (take). Refus : catalogue_invariant (enregistrement ou support
// invalide, niveau nul, doublon, comptes incoherents), index_overflow_u32, memory_budget ; rien n'est publie.
#pragma once

#include <algorithm>

#include "catalogue/finish_kernels.hpp"
#include "catalogue/transfer_meter.hpp"

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

// Sorties hote : boules canoniques, CSR des populations, niveaux des rangs 1..L-1 (cases 0..L-2), table S* -> boule.
struct FinishOutput {
  Buffer<CatalogueBall> balls;
  Buffer<u64> offsets;
  Buffer<SiteIdx> values;
  Buffer<LevelWords> levels;
  Buffer<u64> table_offsets;
  Buffer<BallIdx> table_values;
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

// Chaine de voisins incertains [s, e) autour de la paire (i - 1, i) : bornee par des paires certaines (F4).
inline void chain_around(const u64* keys, u64 n, u64 i, u64& s, u64& e) noexcept {
  s = i - 1;
  while (s > 0 && key_order(keys[s - 1], keys[s]) == 0) --s;
  e = i + 1;
  while (e < n && key_order(keys[e - 1], keys[e]) == 0) ++e;
}

// Chaines mal ordonnees du repli exact : bornes [s, e) et elements (boule et cle F3 d'origine), dans l'ordre.
struct Chains {
  Buffer<u64> bounds;   // 2 par chaine
  Buffer<u32> balls;    // elements, chaine apres chaine
  Buffer<u64> keys;
  u64 count = 0, elements = 0;
};

// Chaines autour des paires mal ordonnees (verdicts, ordre et cles rapatries), en deux passes : compte, puis remplit.
inline Outcome collect_chains(const u32* verdict, const u32* order, const u64* keys, u64 n, Chains& out,
                              MemoryBudget& budget) noexcept {
  for (int pass = 0; pass < 2; ++pass) {
    u64 chains = 0, count = 0, covered = 0;
    for (u64 i = 1; i < n; ++i) {
      if (verdict[i] != kPairMisordered || i < covered) continue;
      u64 s = 0, e = 0;
      chain_around(keys, n, i, s, e);
      if (pass == 1) {
        out.bounds[2 * chains] = s;
        out.bounds[2 * chains + 1] = e;
        for (u64 j = s; j < e; ++j) {
          out.balls[count + j - s] = order[j];
          out.keys[count + j - s] = keys[j];
        }
      }
      covered = e;
      ++chains;
      count += e - s;
    }
    if (pass == 0) {
      MHGP12_TRY(out.bounds.allocate(2 * chains, budget));
      MHGP12_TRY(out.balls.allocate(count, budget));
      MHGP12_TRY(out.keys.allocate(count, budget));
    }
    out.count = chains;
    out.elements = count;
  }
  return {};
}

// Repli exact (rare) : les chaines de voisins incertains mal ordonnees sont retriees sur l'hote par (niveau exact,
// positions de S*), memes comparaisons que ChainKernel ; leurs niveaux et cles de positions sont rassembles par
// l'executeur (PickKernel), puis ordre et cles reecrits chaine par chaine.
template <class B>
Outcome finish_repair(B& b, FinishArrays<B>& a, int ck, u64 n, MemoryBudget& budget, FinishStats& stats) noexcept {
  Buffer<u32> verdict, order;
  Buffer<u64> keys;
  MHGP12_TRY(verdict.allocate(n, budget));
  MHGP12_TRY(order.allocate(n, budget));
  MHGP12_TRY(keys.allocate(n, budget));
  MHGP12_TRY(b.download(verdict.data(), a.verdict, n, 0));
  MHGP12_TRY(b.download(order.data(), a.keys.vals[ck], n, 0));
  MHGP12_TRY(b.download(keys.data(), a.keys.keys[ck], n, 0));
  Chains chains;
  MHGP12_TRY(collect_chains(verdict.data(), order.data(), keys.data(), n, chains, budget));
  const u64 count = chains.elements;
  MHGP12_TRY(b.ensure(a.pick, count));
  MHGP12_TRY(b.ensure(a.pick_levels, count));
  MHGP12_TRY(b.ensure(a.pick_pkey, count));
  MHGP12_TRY(b.upload(a.pick, chains.balls.data(), count, 0));
  MHGP12_TRY(b.launch(PickKernel{a.pick.data(), count, a.levels.data(), a.pkey.data(), a.pick_levels.data(),
                                 a.pick_pkey.data()},
                      tiles_of(count)));
  Buffer<LevelWords> levels;
  Buffer<Key2> pkeys;
  Buffer<u32> rank;
  MHGP12_TRY(levels.allocate(count, budget));
  MHGP12_TRY(pkeys.allocate(count, budget));
  MHGP12_TRY(rank.allocate(count, budget));
  MHGP12_TRY(b.download(levels.data(), a.pick_levels, count, 0));
  MHGP12_TRY(b.download(pkeys.data(), a.pick_pkey, count, 0));
  for (u64 c = 0, at = 0; c < chains.count; ++c) {
    const u64 s = chains.bounds[2 * c], e = chains.bounds[2 * c + 1];
    for (u64 j = 0; j < e - s; ++j) rank[at + j] = static_cast<u32>(at + j);
    std::sort(rank.data() + at, rank.data() + at + (e - s), [&](u32 x, u32 y) {
      const int level = compare_levels(levels[x], levels[y]);
      return level != 0 ? level < 0 : key2_cmp(pkeys[x], pkeys[y]) < 0;
    });
    for (u64 j = s; j < e; ++j) {
      order[j] = chains.balls[rank[at + j - s]];
      keys[j] = chains.keys[rank[at + j - s]];
    }
    MHGP12_TRY(b.upload(a.keys.vals[ck], order.data() + s, e - s, s));
    MHGP12_TRY(b.upload(a.keys.keys[ck], keys.data() + s, e - s, s));
    at += e - s;
  }
  stats.chains_repaired += chains.count;
  stats.chain_elements += count;
  return {};
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
    MHGP12_TRY(finish_repair(b, a, ck, n, budget, stats));
    MHGP12_TRY(b.put(a.fault, 0u, 0));
    MHGP12_TRY(b.launch(check, tiles_of(n)));
    MHGP12_TRY(read_fault(b, a, 0));
  }
  stats.check_ns += watch.nanoseconds();
  return {};
}

// Etape 5 : rangs, CSR des populations, boules canoniques, niveaux des rangs ; rend le nombre de niveaux non nuls.
template <class B>
Result<u64> finish_emit(B& b, FinishArrays<B>& a, const FinishInput& in, int ck, FinishStats& stats) noexcept {
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

// Fin d'etage complete ; les tableaux de l'executeur sont gardes d'un appel a l'autre (sauf ceux rendus par take).
template <class B>
Result<FinishOutput> finish_stage(B& b, FinishArrays<B>& a, const FinishInput& in, MemoryBudget& budget,
                                  FinishStats& stats) noexcept {
  if (in.balls >= kNone) return fail(Reason::index_overflow_u32);
  MHGP12_TRY(finish_reserve(b, a, in));
  const auto ck = finish_order(b, a, in, stats);
  if (!ck.ok()) return ck.outcome();
  MHGP12_TRY(finish_check(b, a, ck.value(), in.balls, budget, stats));
  const auto distinct = finish_emit(b, a, in, ck.value(), stats);
  if (!distinct.ok()) return distinct.outcome();
  const auto ct = finish_table(b, a, in, stats);
  if (!ct.ok()) return ct.outcome();
  const NetWatch<B> watch(b);
  FinishOutput out;
  MHGP12_TRY(b.take(a.balls, out.balls, in.balls));
  MHGP12_TRY(b.take(a.offset, out.offsets, in.balls + 1));
  MHGP12_TRY(b.take(a.values, out.values, in.incidences));
  MHGP12_TRY(b.take(a.level_of_rank, out.levels, distinct.value()));
  MHGP12_TRY(b.take(a.table_offset, out.table_offsets, u64{in.sites} + 1));
  MHGP12_TRY(b.take_cast(a.table.vals[ct.value()], out.table_values, in.balls));
  stats.take_ns += watch.nanoseconds();
  return Result<FinishOutput>(std::move(out));  // deplacement explicite (le frontal de nvcc ne le deduit pas)
}

}  // namespace mhgp12::catalogue_detail::fin
