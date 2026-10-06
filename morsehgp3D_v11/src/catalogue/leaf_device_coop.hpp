// Feuille cooperative, partie commune a l'hote et a l'appareil : profondeur 0 en paires, sous-arbre d'une paire,
// emulation hote (run_leaf_coop) et aiguillage run_leaf_any. Le noyau CUDA est dans leaf_batch_coop_cuda.cuh.
#pragma once

#include "catalogue/leaf_device.hpp"

namespace mhgp11::leaf_device {

// Feuille cooperative (6 octobre 2026 ; note aux auditeurs, section O). La profondeur 0 (sites i) est jouee sur un fil
// et rend les paires (i, j) de la profondeur 1 dans l'ordre du parcours ; chaque paire porte les candidats restants
// apres j et l'ensemble logique de sa profondeur, et son sous-arbre (extend_one<1> puis extend<2>, extend<3>) est
// independant des autres, hors le cache J2 (bit pose par seen_test_set, succes = tests - rangs distincts). Les
// emissions d'un sous-arbre de paire sont contigues dans l'ordre du parcours : leur ordre global est l'ordre des
// paires, et des comptes par paire suivis d'un prefixe placent chaque emission a sa place exacte.
inline constexpr u32 kMaxPairs = kMaxSites * (kMaxSites - 1) / 2;
struct PairTask {
  u32 i = 0, j = 0;
  u64 remaining = 0, logical = 0;  // candidats restants apres j ; ensemble logique de la profondeur 1
};

// Profondeur 0 : memes coupes et memes compteurs qu'extend<0> ; appelle out(k, i, j, remaining, logical, next) pour
// chaque paire k dans l'ordre du parcours (next : candidats de la profondeur 1 sous i, remaining = next au-dela de j) ;
// rend le nombre de paires.
template <class Sink, class Out>
MHGP11_LEAF_HD u32 depth0_pairs(Leaf<Sink>& leaf, Out&& out) {
  const u64 initial = leaf.in.m >= 64 ? ~u64(0) : (u64(1) << leaf.in.m) - 1;
  if (leaf.in.kmax + 1 - 1 < 0) return 0;
  u32 count = 0;
  u64 candidates = initial;
  while (candidates != 0) {
    const u32 i = ctz(candidates);
    candidates &= candidates - 1;
    u64 next = 0, next_logical = 0;
    if (!leaf.template extend_one<0, false>(i, candidates, initial, &next, &next_logical)) continue;
    if (leaf.in.kmax + 1 - 2 < 0) continue;
    for (u64 rest = next; rest != 0; rest &= rest - 1) out(count++, i, ctz(rest), rest & (rest - 1), next_logical, next);
  }
  return count;
}

// Sous-arbre d'une paire, sur un fil : prefixe (i), puis le corps d'extend<1> pour j et sa recursion.
template <class Sink>
MHGP11_LEAF_HD void run_pair(Leaf<Sink>& leaf, const PairTask& pair) {
  leaf.prefix[0] = pair.i;
  leaf.masks[0] = 0;
  leaf.masks[1] = leaf.t.dom[pair.i];
  leaf.template extend_one<1>(pair.j, pair.remaining, pair.logical);
}

// Emulation hote de la feuille cooperative, pour les portes et l'executeur hote : comptage des paires dans l'ordre
// INVERSE (les compteurs ne dependent pas de l'ordre), puis emissions des paires dans l'ordre du parcours (compteurs de
// cette seconde passe jetes, cache J2 remis a zero entre les passes). Memes statut, compteurs et emissions que run_leaf.
struct NullSink {
  MHGP11_LEAF_HD void emit(const Ball&, const u32*, const u32*) {}
};
// order : 0, paires comptees dans l'ordre inverse ; sinon k = (order * step + order) mod n, une permutation quand order
// est premier avec n, l'ordre inverse sinon (portes : ordres normal, inverse et permutes ; audit d117de397).
MHGP11_LEAF_HD u32 gcd(u32 a, u32 b) {
  while (b != 0) {
    const u32 r = a % b;
    a = b; b = r;
  }
  return a;
}
template <class Sink>
MHGP11_LEAF_HD u32 run_leaf_coop(const Input& in, Counts& counts, Sink& sink, u32 order = 0) {
  if (in.m == 0 || in.m > kMaxSites || in.kmax < 1) return kUnresolved;
  Tables tables;
  fill_tables(tables, in);
  counts.dominance_tests += u64(in.m) * (in.m - 1) / 2;
  PairTask pairs[kMaxPairs];
  NullSink none;
  Leaf<NullSink> root(in, counts, none, tables);
  const u32 n = depth0_pairs(root, [&](u32 k, u32 i, u32 j, u64 remaining, u64 logical, u64) {
    pairs[k] = PairTask{i, j, remaining, logical};
  });
  if (root.unresolved) return kUnresolved;
  CountSink tally_sink;
  if (order != 0 && gcd(order, n) != 1) order = 0;
  for (u32 step = 0; step < n; ++step) {
    const u32 k = order == 0 ? n - 1 - step : static_cast<u32>((u64(order) * step + order) % n);
    Leaf<CountSink> lane(in, counts, tally_sink, tables);
    run_pair(lane, pairs[k]);
    if (lane.unresolved) return kUnresolved;
  }
  for (u32 w = 0; w < kSeenWords; ++w) tables.seen[w] = 0;
  Counts discarded;
  for (u32 k = 0; k < n; ++k) {
    Leaf<Sink> lane(in, discarded, sink, tables);
    run_pair(lane, pairs[k]);
    if (lane.unresolved) return kUnresolved;
  }
  return kOk;
}

// Feuille sequentielle ou cooperative selon Input::coop.
template <class Sink>
MHGP11_LEAF_HD u32 run_leaf_any(const Input& in, Counts& counts, Sink& sink) {
  return in.coop ? run_leaf_coop(in, counts, sink) : run_leaf(in, counts, sink);
}

}  // namespace mhgp11::leaf_device
