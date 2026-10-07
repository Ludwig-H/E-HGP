// Forme B, « coherente » (MES-M2, hors produit) : un warp par feuille, tout le warp sur un meme prefixe.
//
// Parcours en profondeur des prefixes, uniforme sur le warp (le meme ensemble de prefixes visites que leaf.cpp, sans
// pile locale ni divergence de parcours). A chaque prefixe developpe P, tous ses enfants visites P u {x} sont evalues
// ensemble, la voie x jugeant l'enfant x (G3, droites J2, fabrique du candidat, comptes logiques) ; chaque presentation
// jugee est ensuite recensee par tout le warp (une voie par site, vote). Le warp descend ensuite dans chaque enfant
// developpe, dans l'ordre croissant.
// Reutilisation locale : au niveau des quadruplets sous (i0, i1, i2), la face (i0, i1, x) est lue dans le vote des
// droites fait au niveau des triplets sous (i0, i1) ; les faces (i0, i2, x) et (i1, i2, x) sont recalculees (le travail
// physique differe de leaf.cpp, les compteurs logiques non : voir leaf_common.hpp).
#pragma once

#include "mhgp12/leaf/leaf_common.hpp"

namespace mhgp12::leaf::coherent {

template <class Sink>
MHGP12_HD void census_round(Ctx& X, Sink& sink, u32 q, u32 judged, const u8 (&prefix)[4], const Lanes<Center>& center) {
  for (u32 rest = judged; rest != 0; rest &= rest - 1) {
    const u32 x = simt::ctz(rest);
    u8 gen[4] = {prefix[0], prefix[1], prefix[2], prefix[3]};
    gen[q - 1] = static_cast<u8>(x);
    for (u32 t = q; t < 4; ++t) gen[t] = kNoLocal;
    Center c;
    if (q == 2) c = q2_center(X.S.P[gen[0]], X.S.P[gen[1]]);
    else c = simt::shfl(center, x);
    census(X, sink, q, gen, c);
    if (X.unresolved) return;
  }
}

template <class Sink>
MHGP12_HD u32 run_leaf(const Input& in, Shared& S, Counts& out, Sink& sink, Diag* diag = nullptr) {
  zero(out);
  if (in.m == 0 || in.m > kMaxSites || in.kmax < 1) return kStatusUnresolved;
  if (!prepare(in, S)) return kStatusUnresolved;
  Ctx X{in, S, in.m, in.kmax, false, 0, 0, 0, 0, 0, diag};
  if (diag != nullptr) ++diag->leaves;
  const int K = in.kmax;
  const u32 m = in.m;
  const i64* lo = in.lo;
  const i64* hi = in.hi;
  LaneAccumulators acc;
  clear(acc);
  Lanes<bool> flag;
  Lanes<u32> next;
  Lanes<Center> center;
  // Cardinal 1 : voie i.
  MHGP12_LANES(i) {
    next[i] = 0;
    center[i] = Center{};
    if (i < m) {
      const int c1 = static_cast<int>(simt::popc(S.dom[i]));
      if (c1 <= K) {
        acc.prefixes[i] += simt::popc(simt::above(i) & S.nbr[i]);
        if (c1 <= K - 1) next[i] = simt::above(i) & S.live[0][i];
      }
    }
  }
  Lanes<u32> next1 = next;
  MHGP12_LANES(i) flag[i] = next1[i] != 0;
  for (u32 r1 = simt::ballot(flag); r1 != 0; r1 &= r1 - 1) {
    const u32 i0 = simt::ctz(r1);
    const u32 n1 = simt::shfl(next1, i0);
    // Cardinal 2 : enfants (i0, x).
    Lanes<bool> judged;
    Lanes<u32> next2;
    MHGP12_LANES(x) {
      judged[x] = false;
      next2[x] = 0;
      if ((n1 >> x) & 1u) {
        const int c2 = static_cast<int>(simt::popc(S.dom[i0] | S.dom[x]));
        if (c2 <= K - 1) {
          judged[x] = q2_in_box(S.P[i0], S.P[x], lo, hi);
          if (K >= 2) {
            acc.prefixes[x] += simt::popc(simt::above(x) & S.nbr[i0] & S.nbr[x]);
            if (c2 <= K - 2) next2[x] = simt::above(x) & S.live[1][i0] & S.live[1][x];
          }
        }
      }
    }
    if (diag != nullptr) {
      ++diag->steps;
      diag->active += simt::popc(n1);
    }
    {
      const u8 prefix[4] = {static_cast<u8>(i0), 0, 0, 0};
      census_round(X, sink, 2, simt::ballot(judged), prefix, center);
      if (X.unresolved) return kStatusUnresolved;
    }
    MHGP12_LANES(x) flag[x] = next2[x] != 0;
    for (u32 r2 = simt::ballot(flag); r2 != 0; r2 &= r2 - 1) {
      const u32 i1 = simt::ctz(r2);
      const u32 n2 = simt::shfl(next2, i1);
      const u32 m01 = S.dom[i0] | S.dom[i1];
      // Cardinal 3 : enfants (i0, i1, x).
      Lanes<bool> hit;
      Lanes<u32> next3;
      MHGP12_LANES(x) {
        judged[x] = false;
        hit[x] = false;
        next3[x] = 0;
        if ((n2 >> x) & 1u) {
          const int c3 = static_cast<int>(simt::popc(m01 | S.dom[x]));
          if (c3 <= K - 2) {
            acc.line_tests3[x] += 1;
            if (center_line_meets(S.P[i0], S.P[i1], S.P[x], lo, hi) != kIntersects) {
              acc.line_rejects[x] += 1;
            } else {
              hit[x] = true;
              judged[x] = q3_candidate(S.P[i0], S.P[i1], S.P[x], lo, hi, center[x]);
              if (K >= 3) {
                acc.prefixes[x] += simt::popc(simt::above(x) & S.nbr[i0] & S.nbr[i1] & S.nbr[x]);
                if (c3 <= K - 3) next3[x] = simt::above(x) & S.live[2][i0] & S.live[2][i1] & S.live[2][x];
              }
            }
          }
        }
      }
      if (diag != nullptr) {
        ++diag->steps;
        diag->active += simt::popc(n2);
      }
      const u32 h01 = simt::ballot(hit);  // droites (i0, i1, x) qui rencontrent la boite
      {
        const u8 prefix[4] = {static_cast<u8>(i0), static_cast<u8>(i1), 0, 0};
        census_round(X, sink, 3, simt::ballot(judged), prefix, center);
        if (X.unresolved) return kStatusUnresolved;
      }
      MHGP12_LANES(x) flag[x] = next3[x] != 0;
      for (u32 r3 = simt::ballot(flag); r3 != 0; r3 &= r3 - 1) {
        const u32 i2 = simt::ctz(r3);
        const u32 n3 = simt::shfl(next3, i2);
        const u32 m012 = m01 | S.dom[i2];
        // Cardinal 4 : enfants (i0, i1, i2, x).
        MHGP12_LANES(x) {
          judged[x] = false;
          if ((n3 >> x) & 1u) {
            const int c4 = static_cast<int>(simt::popc(m012 | S.dom[x]));
            if (c4 <= K - 3) {
              const u32 f0 = (h01 >> x) & 1u;
              const u32 f1 = f0 != 0 && center_line_meets(S.P[i0], S.P[i2], S.P[x], lo, hi) == kIntersects ? 1u : 0u;
              const u32 f2 = f1 != 0 && center_line_meets(S.P[i1], S.P[i2], S.P[x], lo, hi) == kIntersects ? 1u : 0u;
              acc.line_tests4[x] += 1 + f0 + f1;  // faces dans l'ordre, arret au premier echec
              if (f2 == 0) {
                acc.line_rejects[x] += 1;
              } else {
                bool counted = false;
                judged[x] = q4_candidate(S.P[i0], S.P[i1], S.P[i2], S.P[x], lo, hi, center[x], counted);
                acc.q4_candidates[x] += counted ? 1u : 0u;
              }
            }
          }
        }
        if (diag != nullptr) {
          ++diag->steps;
          diag->active += simt::popc(n3);
        }
        const u8 prefix[4] = {static_cast<u8>(i0), static_cast<u8>(i1), static_cast<u8>(i2), 0};
        census_round(X, sink, 4, simt::ballot(judged), prefix, center);
        if (X.unresolved) return kStatusUnresolved;
      }
    }
  }
  finish(X, acc, out);
  return kStatusOk;
}

}  // namespace mhgp12::leaf::coherent
