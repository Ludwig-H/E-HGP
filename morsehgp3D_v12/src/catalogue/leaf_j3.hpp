// Feuille J3 << par phases >> sur un warp de N voies, source unique de la feuille du catalogue (adoptee par MES-M2 sur
// G4 le 7 octobre 2026, variante j3_r168). Port explicite de microbancs/mes_m2_feuille/include/mhgp12/leaf/leaf_j3.hpp :
//
//   D  dominances, graphe de paires et lignes vivantes : une ligne par voie (prepare, leaf_common.hpp) ;
//   P  paires visitees (i, j) : file d'items, une paire par voie et par super-pas ; candidat q2 (milieu dans la boite) ;
//   T  triplets visites (i, j, k) : G3, droite J2 calculee UNE fois par triplet, table H des triplets vivants
//      (H[(i,j)] bit k : triplet visite, G3, droite qui rencontre la boite) ; candidat q3 ;
//   Q  quadruplets visites (i, j, k, l) : engendres depuis les seuls triplets vivants, G3, faces J2 lues dans trois
//      lignes de H (ET des trois bits, compteurs dans l'ordre sequentiel des faces) ; candidat q4 ;
//   R  chaque presentation jugee d'un super-pas est recensee par tout le warp (leaf_census.hpp).
// Distribution du travail : chaque voie i engendre les items de la ligne i (iterateur persistant), un prefixe exclusif
// sur les voies place les items dans une file partagee de kQueue cases, traitee par paquets de N (une voie par item).
//
// Ce qui change par rapport au microbanc : warp de N voies (N = 256 : feuilles de 33 a 256 sites, hote seulement) ;
// politique arithmetique A (Narrow ou Exact, leaf_arith.hpp), coordonnees locales ; une faute de la politique exacte
// rend la feuille invariant_violated (jamais << non resolue >> : la politique est choisie avant le lancement).
#pragma once

#include "catalogue/leaf_census.hpp"

namespace mhgp12::catalogue_detail {

template <u32 N>
MHGP12_HD u32 pack(u32 i, u32 j, u32 k, u32 l) {
  constexpr u32 b = simt::Width<N>::kIndexBits;
  return i | (j << b) | (k << (2 * b)) | (l << (3 * b));
}
template <u32 N>
MHGP12_HD u32 unpack(u32 item, u32 position) {
  constexpr u32 b = simt::Width<N>::kIndexBits;
  return (item >> (b * position)) & ((1u << b) - 1u);
}

// Iterateur persistant de la ligne i : paires (Q = 2), triplets (Q = 3) ou quadruplets (Q = 4), ordre lexicographique.
template <u32 N>
struct Iter {
  MaskOf<N> jm;
  u32 j;
  MaskOf<N> km;
  u32 k;
  MaskOf<N> lm;
};

// Enfants visites d'une paire visitee (i, j) qui se developpe (K >= 2, cnt <= K-2) ; vide sinon.
template <u32 N>
MHGP12_HD MaskOf<N> next2(const LeafShared<N>& S, int K, u32 i, u32 j) {
  using W = simt::Width<N>;
  if (K < 2) return W::empty();
  const int c2 = static_cast<int>(simt::popc(S.dom[i] | S.dom[j]));
  if (c2 > K - 2) return W::empty();
  return W::above(j) & S.live[1][i] & S.live[1][j];
}
// Enfants visites d'un triplet vivant (i, j, k) qui se developpe (K >= 3, cnt <= K-3) ; vide sinon.
template <u32 N>
MHGP12_HD MaskOf<N> next3(const LeafShared<N>& S, int K, u32 i, u32 j, u32 k) {
  using W = simt::Width<N>;
  if (K < 3) return W::empty();
  const int c3 = static_cast<int>(simt::popc(S.dom[i] | S.dom[j] | S.dom[k]));
  if (c3 > K - 3) return W::empty();
  return W::above(k) & S.live[2][i] & S.live[2][j] & S.live[2][k];
}

// Nombre d'items de la ligne i.
template <int Q, u32 N>
MHGP12_HD u32 count_items(const LeafShared<N>& S, int K, u32 i, MaskOf<N> next1) {
  u32 n = 0;
  if constexpr (Q == 2) {
    n = simt::popc(next1);
  } else if constexpr (Q == 3) {
    for (auto jm = next1; simt::any(jm); jm = simt::clear_lowest(jm)) n += simt::popc(next2<N>(S, K, i, simt::ctz(jm)));
  } else {
    for (auto jm = next1; simt::any(jm); jm = simt::clear_lowest(jm)) {
      const u32 j = simt::ctz(jm);
      for (auto km = next2<N>(S, K, i, j) & S.H[hrow<N>(i, j)]; simt::any(km); km = simt::clear_lowest(km))
        n += simt::popc(next3<N>(S, K, i, j, simt::ctz(km)));
    }
  }
  return n;
}

// Item suivant de la ligne i (precondition : il en reste un).
template <int Q, u32 N>
MHGP12_HD u32 next_item(const LeafShared<N>& S, int K, u32 i, Iter<N>& it) {
  if constexpr (Q == 2) {
    const u32 j = simt::ctz(it.jm);
    it.jm = simt::clear_lowest(it.jm);
    return pack<N>(i, j, 0, 0);
  } else if constexpr (Q == 3) {
    while (!simt::any(it.km)) {
      it.j = simt::ctz(it.jm);
      it.jm = simt::clear_lowest(it.jm);
      it.km = next2<N>(S, K, i, it.j);
    }
    const u32 k = simt::ctz(it.km);
    it.km = simt::clear_lowest(it.km);
    return pack<N>(i, it.j, k, 0);
  } else {
    while (!simt::any(it.lm)) {
      while (!simt::any(it.km)) {
        it.j = simt::ctz(it.jm);
        it.jm = simt::clear_lowest(it.jm);
        it.km = next2<N>(S, K, i, it.j) & S.H[hrow<N>(i, it.j)];
      }
      it.k = simt::ctz(it.km);
      it.km = simt::clear_lowest(it.km);
      it.lm = next3<N>(S, K, i, it.j, it.k);
    }
    const u32 l = simt::ctz(it.lm);
    it.lm = simt::clear_lowest(it.lm);
    return pack<N>(i, it.j, it.k, l);
  }
}

// Jugement d'un item par sa voie : G3, droites J2, fabrique du candidat ; compteurs dans l'ordre sequentiel.
template <int Q, u32 N, class A>
MHGP12_HD Verdict judge_item(LeafCtx<N, A>& X, LaneAccumulators<N>& acc, u32 l, u32 item, Center<A>& center) {
  using W = simt::Width<N>;
  LeafShared<N>& S = X.S;
  const int K = X.K;
  const u32 i = unpack<N>(item, 0), j = unpack<N>(item, 1), k = unpack<N>(item, 2), q = unpack<N>(item, 3);
  // Visite deja comptee dans les enfants du parent (prefixes = m + somme des enfants des prefixes developpes).
  if constexpr (Q == 2) {
    if (static_cast<int>(simt::popc(S.dom[i] | S.dom[j])) > K - 1) return kNo;  // G3 (toujours vrai : live[0])
    if (K >= 2) {
      const MaskOf<N> prefix[4] = {S.nbr[i], S.nbr[i] & S.nbr[j], W::empty(), W::empty()};
      children<N>(acc, l, X.m, j, 2, prefix);
    }
    return q2_in_box(S.P[i], S.P[j], X.lo, X.hi) ? kYes : kNo;
  } else if constexpr (Q == 3) {
    if (static_cast<int>(simt::popc(S.dom[i] | S.dom[j] | S.dom[k])) > K - 2) return kNo;  // G3
    acc.line_tests3[l] += 1;
    if (center_line_meets<A>(S.P[i], S.P[j], S.P[k], X.lo, X.hi) != kIntersects) {
      acc.line_rejects[l] += 1;
      return kNo;
    }
    simt::atomic_or(&S.H[hrow<N>(i, j)], W::bit(k));
    if (K >= 3) {
      const MaskOf<N> prefix[4] = {S.nbr[i], S.nbr[i] & S.nbr[j], S.nbr[i] & S.nbr[j] & S.nbr[k], W::empty()};
      children<N>(acc, l, X.m, k, 3, prefix);
    }
    return q3_candidate<A>(S.P[i], S.P[j], S.P[k], X.lo, X.hi, center);
  } else {
    if (static_cast<int>(simt::popc(S.dom[i] | S.dom[j] | S.dom[k] | S.dom[q])) > K - 3) return kNo;  // G3
    const u32 f0 = simt::test(S.H[hrow<N>(i, j)], q) ? 1u : 0u, f1 = simt::test(S.H[hrow<N>(i, k)], q) ? 1u : 0u;
    const u32 f2 = simt::test(S.H[hrow<N>(j, k)], q) ? 1u : 0u;
    acc.line_tests4[l] += 1 + f0 + (f0 & f1);  // faces dans l'ordre, arret au premier echec
    if ((f0 & f1 & f2) == 0) {
      acc.line_rejects[l] += 1;
      return kNo;
    }
    bool counted = false;
    const Verdict v = q4_candidate<A>(S.P[i], S.P[j], S.P[k], S.P[q], X.lo, X.hi, center, counted);
    acc.q4_candidates[l] += counted ? 1u : 0u;
    return v;
  }
}

// Un paquet de N items de la file (une voie par item), puis le census de chaque presentation jugee.
template <int Q, u32 N, class A, class Sink>
MHGP12_HD void round(LeafCtx<N, A>& X, LaneAccumulators<N>& acc, Sink& sink, u32 first, u32 n) {
  LeafShared<N>& S = X.S;
  simt::Lanes<bool, N> judged, fault;
  simt::Lanes<u32, N> tuple;
  simt::Lanes<Center<A>, N> center;
  MHGP12_LANES(N, l) {
    judged[l] = fault[l] = false;
    tuple[l] = 0;
    center[l] = Center<A>{};
    if (first + l < n) {
      tuple[l] = S.queue[first + l];
      const Verdict v = judge_item<Q, N, A>(X, acc, l, tuple[l], center[l]);
      judged[l] = v == kYes;
      fault[l] = v == kFault;
    }
  }
  if (simt::any(simt::ballot<N>(fault))) {
    X.status = kLeafInvariant;
    return;
  }
  for (auto rest = simt::ballot<N>(judged); simt::any(rest); rest = simt::clear_lowest(rest)) {
    const u32 src = simt::ctz(rest);
    const u32 item = simt::shfl(tuple, src);
    const LocalRank gen[4] = {static_cast<LocalRank>(unpack<N>(item, 0)), static_cast<LocalRank>(unpack<N>(item, 1)),
                              Q > 2 ? static_cast<LocalRank>(unpack<N>(item, 2)) : kNoLocal,
                              Q > 3 ? static_cast<LocalRank>(unpack<N>(item, 3)) : kNoLocal};
    Center<A> c;
    if constexpr (Q == 2) c = q2_center<A>(S.P[gen[0]], S.P[gen[1]]);
    else c = simt::shfl(center, src);
    census<N, A>(X, sink, Q, gen, c);
    if (X.status != kLeafOk) return;
  }
}

// Une phase : comptes par ligne, prefixe exclusif, tranches de la file, paquets de N.
template <int Q, u32 N, class A, class Sink>
MHGP12_HD void phase(LeafCtx<N, A>& X, LaneAccumulators<N>& acc, Sink& sink, const simt::Lanes<MaskOf<N>, N>& next1) {
  LeafShared<N>& S = X.S;
  const int K = X.K;
  simt::Lanes<u32, N> count;
  simt::Lanes<Iter<N>, N> it;
  MHGP12_LANES(N, i) {
    count[i] = X.m > i ? count_items<Q, N>(S, K, i, next1[i]) : 0u;
    it[i] = Iter<N>{next1[i], 0, simt::Width<N>::empty(), 0, simt::Width<N>::empty()};
  }
  u32 total = 0;
  const simt::Lanes<u32, N> offset = simt::exclusive_scan<N>(count, total);
  for (u32 base = 0; base < total; base += kQueue) {
    MHGP12_LANES(N, i) {
      const u32 begin = offset[i] > base ? offset[i] : base;
      const u32 stop = offset[i] + count[i] < base + kQueue ? offset[i] + count[i] : base + kQueue;
      for (u32 g = begin; g < stop; ++g) S.queue[g - base] = next_item<Q, N>(S, K, i, it[i]);
    }
    simt::sync();
    const u32 n = total - base < kQueue ? total - base : kQueue;
    for (u32 first = 0; first < n; first += N) {
      round<Q, N, A>(X, acc, sink, first, n);
      if (X.status != kLeafOk) return;
    }
    simt::sync();
  }
}

// Une feuille : kLeafOk (emissions et compteurs publies) ou un refus (tout est a jeter). Precondition : 1 <= m <= N,
// 1 <= K, repere de la feuille d'etendue couverte par la politique A.
template <u32 N, class A, class Sink>
MHGP12_HD u32 run_leaf(const LeafInput& in, LeafShared<N>& S, LeafCounts& out, Sink& sink) {
#if defined(__CUDA_ARCH__)
  static_assert(N == simt::kWarp, "feuille : seul le warp de 32 voies existe sur l'appareil (m > 32 : repli de l'hote)");
#endif
  for (u32 f = 0; f < kLeafCounters; ++f) out.c[f] = 0;
  if (in.m == 0 || in.m > N || in.kmax < 1) return kLeafInvariant;
  LeafCtx<N, A> X{S, in.m, in.kmax, {}, {}, kLeafOk, 0, 0, 0, 0, 0};
  for (int a = 0; a < 3; ++a) {
    X.lo[a] = in.lo[a] - in.origin[a];
    X.hi[a] = in.hi[a] - in.origin[a];
  }
  prepare<N, A>(in, X);
  const int K = in.kmax;
  LaneAccumulators<N> acc;
  simt::Lanes<MaskOf<N>, N> next1;
  MHGP12_LANES(N, i) {
    acc.prefixes[i] = acc.pair_tests[i] = acc.pair_rejects[i] = 0;
    acc.line_tests3[i] = acc.line_tests4[i] = acc.line_rejects[i] = acc.q4_candidates[i] = 0;
    for (u32 w = i; w < kPairRows<N>; w += N) S.H[w] = simt::Width<N>::empty();
    next1[i] = simt::Width<N>::empty();
    if (i < in.m) {
      const int c1 = static_cast<int>(simt::popc(S.dom[i]));
      if (c1 <= K) {  // G3 au cardinal 1 ; K >= 1 : developpe
        const MaskOf<N> prefix[4] = {S.nbr[i], S.nbr[i], S.nbr[i], S.nbr[i]};
        children<N>(acc, i, in.m, i, 1, prefix);
        if (c1 <= K - 1) next1[i] = simt::Width<N>::above(i) & S.live[0][i];
      }
    }
  }
  simt::sync();
  phase<2, N, A>(X, acc, sink, next1);
  if (X.status == kLeafOk) phase<3, N, A>(X, acc, sink, next1);
  simt::sync();  // table H complete avant les quadruplets
  if (X.status == kLeafOk) phase<4, N, A>(X, acc, sink, next1);
  if (X.status != kLeafOk) return X.status;
  finish<N, A>(X, acc, out);
  return kLeafOk;
}

}  // namespace mhgp12::catalogue_detail
