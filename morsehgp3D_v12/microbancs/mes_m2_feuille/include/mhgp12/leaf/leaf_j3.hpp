// Forme A, « phases a la J3 » (MES-M2, hors produit) : un warp par feuille, phases successives.
//
//   D  dominances, graphe de paires et lignes vivantes : une ligne par voie (prepare, leaf_common.hpp) ;
//   P  paires visitees (i, j) : file d'items, une paire par voie et par super-pas ; candidat q2 (milieu dans la boite) ;
//   T  triplets visites (i, j, k) : G3, droite J2 calculee UNE fois par triplet (aucun cache a consulter), table H des
//      triplets vivants (H[(i,j)] bit k : triplet visite, G3, droite qui rencontre la boite) ; candidat q3 ;
//   Q  quadruplets visites (i, j, k, l) : generes depuis les seuls triplets vivants, G3, faces J2 lues dans trois lignes
//      de H (ET des trois bits, compteurs dans l'ordre sequentiel des faces) ; candidat q4 ;
//   R  chaque presentation jugee d'un super-pas est recensee par tout le warp (leaf_common.hpp, census).
// Distribution du travail : chaque voie i genere les items de la ligne i (iterateur persistant), un prefixe exclusif
// sur les voies place les items dans une file partagee de kQueue cases, traitee par paquets de 32 (une voie par item).
// Les files trop longues sont traitees par tranches successives, chaque item etant genere une seule fois.
#pragma once

#include "mhgp12/leaf/leaf_common.hpp"

namespace mhgp12::leaf::j3 {

inline constexpr u32 kQueue = 512;
inline constexpr u32 kPairRows = 496;  // couples (a, b), a < b < 32

struct SharedJ3 : Shared {
  u32 H[kPairRows];
  u32 queue[kQueue];
};

MHGP12_HD u32 pack(u32 i, u32 j, u32 k, u32 l) { return i | (j << 5) | (k << 10) | (l << 15); }

// Iterateur persistant de la ligne i : paires (Q = 2), triplets (Q = 3) ou quadruplets (Q = 4), ordre lexicographique.
struct Iter {
  u32 jm, j, km, k, lm;
};

template <int Q>
MHGP12_HD Iter start(u32 next1) {
  Iter it{next1, 0, 0, 0, 0};
  return it;
}

// Enfants visites d'une paire visitee (i, j) qui se developpe (K >= 2, cnt <= K-2) ; 0 sinon.
MHGP12_HD u32 next2(const SharedJ3& S, int K, u32 i, u32 j) {
  if (K < 2) return 0;
  const int c2 = static_cast<int>(simt::popc(S.dom[i] | S.dom[j]));
  if (c2 > K - 2) return 0;
  return simt::above(j) & S.live[1][i] & S.live[1][j];
}
// Enfants visites d'un triplet vivant (i, j, k) qui se developpe (K >= 3, cnt <= K-3) ; 0 sinon.
MHGP12_HD u32 next3(const SharedJ3& S, int K, u32 i, u32 j, u32 k) {
  if (K < 3) return 0;
  const int c3 = static_cast<int>(simt::popc(S.dom[i] | S.dom[j] | S.dom[k]));
  if (c3 > K - 3) return 0;
  return simt::above(k) & S.live[2][i] & S.live[2][j] & S.live[2][k];
}

// Nombre d'items de la ligne i.
template <int Q>
MHGP12_HD u32 count_items(const SharedJ3& S, int K, u32 i, u32 next1) {
  if constexpr (Q == 2) {
    return simt::popc(next1);
  } else if constexpr (Q == 3) {
    u32 n = 0;
    for (u32 jm = next1; jm != 0; jm &= jm - 1) n += simt::popc(next2(S, K, i, simt::ctz(jm)));
    return n;
  } else {
    u32 n = 0;
    for (u32 jm = next1; jm != 0; jm &= jm - 1) {
      const u32 j = simt::ctz(jm);
      for (u32 km = next2(S, K, i, j) & S.H[hrow(i, j)]; km != 0; km &= km - 1)
        n += simt::popc(next3(S, K, i, j, simt::ctz(km)));
    }
    return n;
  }
}

// Item suivant de la ligne i (precondition : il en reste un).
template <int Q>
MHGP12_HD u32 next_item(const SharedJ3& S, int K, u32 i, Iter& it) {
  if constexpr (Q == 2) {
    const u32 j = simt::ctz(it.jm);
    it.jm &= it.jm - 1;
    return pack(i, j, 0, 0);
  } else if constexpr (Q == 3) {
    while (it.km == 0) {
      it.j = simt::ctz(it.jm);
      it.jm &= it.jm - 1;
      it.km = next2(S, K, i, it.j);
    }
    const u32 k = simt::ctz(it.km);
    it.km &= it.km - 1;
    return pack(i, it.j, k, 0);
  } else {
    while (it.lm == 0) {
      while (it.km == 0) {
        it.j = simt::ctz(it.jm);
        it.jm &= it.jm - 1;
        it.km = next2(S, K, i, it.j) & S.H[hrow(i, it.j)];
      }
      it.k = simt::ctz(it.km);
      it.km &= it.km - 1;
      it.lm = next3(S, K, i, it.j, it.k);
    }
    const u32 l = simt::ctz(it.lm);
    it.lm &= it.lm - 1;
    return pack(i, it.j, it.k, l);
  }
}

// Un paquet de 32 items de la file (une voie par item), puis le census de chaque presentation jugee.
template <int Q, class Sink>
MHGP12_HD void round(Ctx& X, SharedJ3& S, LaneAccumulators& acc, Sink& sink, u32 first, u32 n) {
  const int K = X.K;
  const i64* lo = X.in.lo;
  const i64* hi = X.in.hi;
  Lanes<bool> judged;
  Lanes<u32> tuple;
  Lanes<Center> center;
  MHGP12_LANES(l) {
    judged[l] = false;
    tuple[l] = 0;
    center[l] = Center{};
    const u32 at = first + l;
    if (at < n) {
      const u32 item = S.queue[at];
      tuple[l] = item;
      const u32 i = item & 31u, j = (item >> 5) & 31u, k = (item >> 10) & 31u, q = (item >> 15) & 31u;
      // Visite : deja comptee dans |CE| du parent (prefixes = m + somme des |CE(P)| developpes).
      if constexpr (Q == 2) {
        const int c2 = static_cast<int>(simt::popc(S.dom[i] | S.dom[j]));
        if (c2 <= K - 1) {  // G3 (toujours vrai : live[0])
          judged[l] = q2_in_box(S.P[i], S.P[j], lo, hi);
          if (K >= 2) acc.prefixes[l] += simt::popc(simt::above(j) & S.nbr[i] & S.nbr[j]);
        }
      } else if constexpr (Q == 3) {
        const int c3 = static_cast<int>(simt::popc(S.dom[i] | S.dom[j] | S.dom[k]));
        if (c3 <= K - 2) {  // G3
          acc.line_tests3[l] += 1;
          if (center_line_meets(S.P[i], S.P[j], S.P[k], lo, hi) != kIntersects) {
            acc.line_rejects[l] += 1;
          } else {
            simt::atomic_or(&S.H[hrow(i, j)], 1u << k);
            judged[l] = q3_candidate(S.P[i], S.P[j], S.P[k], lo, hi, center[l]);
            if (K >= 3) acc.prefixes[l] += simt::popc(simt::above(k) & S.nbr[i] & S.nbr[j] & S.nbr[k]);
          }
        }
      } else {
        const int c4 = static_cast<int>(simt::popc(S.dom[i] | S.dom[j] | S.dom[k] | S.dom[q]));
        if (c4 <= K - 3) {  // G3
          const u32 f0 = (S.H[hrow(i, j)] >> q) & 1u, f1 = (S.H[hrow(i, k)] >> q) & 1u;
          const u32 f2 = (S.H[hrow(j, k)] >> q) & 1u;
          acc.line_tests4[l] += 1 + f0 + (f0 & f1);  // faces dans l'ordre, arret au premier echec
          if ((f0 & f1 & f2) == 0) {
            acc.line_rejects[l] += 1;
          } else {
            bool counted = false;
            judged[l] = q4_candidate(S.P[i], S.P[j], S.P[k], S.P[q], lo, hi, center[l], counted);
            acc.q4_candidates[l] += counted ? 1u : 0u;
          }
        }
      }
    }
  }
  if (X.diag != nullptr) {
    ++X.diag->rounds[Q];
    X.diag->items[Q] += (n - first) < 32u ? (n - first) : 32u;
  }
  for (u32 rest = simt::ballot(judged); rest != 0; rest &= rest - 1) {
    const u32 src = simt::ctz(rest);
    const u32 item = simt::shfl(tuple, src);
    const u8 gen[4] = {static_cast<u8>(item & 31u), static_cast<u8>((item >> 5) & 31u),
                       Q > 2 ? static_cast<u8>((item >> 10) & 31u) : kNoLocal,
                       Q > 3 ? static_cast<u8>((item >> 15) & 31u) : kNoLocal};
    Center c;
    if constexpr (Q == 2) c = q2_center(S.P[gen[0]], S.P[gen[1]]);
    else c = simt::shfl(center, src);
    census(X, sink, Q, gen, c);
    if (X.unresolved) return;
  }
}

// Une phase : comptes par ligne, prefixe exclusif, tranches de la file, paquets de 32.
template <int Q, class Sink>
MHGP12_HD void phase(Ctx& X, SharedJ3& S, LaneAccumulators& acc, Sink& sink, const Lanes<u32>& next1) {
  const int K = X.K;
  Lanes<u32> count;
  Lanes<Iter> it;
  MHGP12_LANES(i) {
    count[i] = X.m > i ? count_items<Q>(S, K, i, next1[i]) : 0u;
    it[i] = start<Q>(next1[i]);
  }
  u32 total = 0;
  const Lanes<u32> offset = simt::exclusive_scan(count, total);
  for (u32 base = 0; base < total; base += kQueue) {
    MHGP12_LANES(i) {
      const u32 begin = offset[i] > base ? offset[i] : base;
      const u32 stop = offset[i] + count[i] < base + kQueue ? offset[i] + count[i] : base + kQueue;
      for (u32 g = begin; g < stop; ++g) S.queue[g - base] = next_item<Q>(S, K, i, it[i]);
    }
    simt::sync();
    if (X.diag != nullptr) ++X.diag->chunks;
    const u32 n = total - base < kQueue ? total - base : kQueue;
    for (u32 first = 0; first < n; first += 32) {
      round<Q>(X, S, acc, sink, first, n);
      if (X.unresolved) return;
    }
    simt::sync();
  }
}

// Une feuille. Rend kStatusOk (emissions et compteurs publies) ou kStatusUnresolved (tout est a jeter ; l'hote rejoue
// la feuille par leaf.cpp avant admission).
template <class Sink>
MHGP12_HD u32 run_leaf(const Input& in, SharedJ3& S, Counts& out, Sink& sink, Diag* diag = nullptr) {
  zero(out);
  if (in.m == 0 || in.m > kMaxSites || in.kmax < 1) return kStatusUnresolved;
  if (!prepare(in, S)) return kStatusUnresolved;
  Ctx X{in, S, in.m, in.kmax, false, 0, 0, 0, 0, 0, diag};
  if (diag != nullptr) ++diag->leaves;
  const int K = in.kmax;
  const u32 m = in.m;
  LaneAccumulators acc;
  clear(acc);
  Lanes<u32> next1;
  MHGP12_LANES(i) {
    for (u32 w = i; w < kPairRows; w += 32) S.H[w] = 0;
    next1[i] = 0;
    if (i < m) {
      const int c1 = static_cast<int>(simt::popc(S.dom[i]));
      if (c1 <= K) {  // G3 au cardinal 1 ; K >= 1 : developpe
        acc.prefixes[i] += simt::popc(simt::above(i) & S.nbr[i]);
        if (c1 <= K - 1) next1[i] = simt::above(i) & S.live[0][i];
      }
    }
  }
  simt::sync();
  phase<2>(X, S, acc, sink, next1);
  if (!X.unresolved) phase<3>(X, S, acc, sink, next1);
  simt::sync();  // table H complete avant les quadruplets
  if (!X.unresolved) phase<4>(X, S, acc, sink, next1);
  if (X.unresolved) return kStatusUnresolved;
  finish(X, acc, out);
  return kStatusOk;
}

}  // namespace mhgp12::leaf::j3
