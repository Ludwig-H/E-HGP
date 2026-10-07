// Feuille data-parallele, partie commune aux deux formes (MES-M2, hors produit) : un warp par feuille.
//
// Semantique de reference : la voie graphe de paires de morsehgp3D_v11/src/catalogue/leaf.cpp (et son port fidele
// leaf_device.hpp), m <= 32 sites. Les deux formes rendent, pour chaque feuille resolue, le meme ENSEMBLE d'emissions
// (S* en rangs locaux, p, m, qmin, populations I puis U croissantes) et les memes quinze compteurs logiques ; seul
// l'ordre des emissions dans la feuille change (le catalogue est ensuite trie par (niveau, S*)).
//
// Ensemble des prefixes visites (definition par ensembles, independante de l'ordre de visite) :
//   cnt(P) = |union des Dom(p)| ; live[t][x] = voisins y de x avec |Dom(x) u Dom(y)| <= K-1-t (t = 0, 1, 2) ;
//   V1 = tous les sites ; P = (i0 < ... < i_{q-1}) de V_q est DEVELOPPE si cnt(P) <= K+1-q (G3), si q = 3 implique la
//   droite J2 de (i0,i1,i2), et si q < 4 et K >= q ; ses enfants logiques sont CE(P) = au-dessus(i_{q-1}) inter les
//   nbr(p) ; ses enfants visites (si de plus cnt(P) <= K-q) sont au-dessus(i_{q-1}) inter les live[q-1][p].
// Compteurs logiques (contrat R1 de la v11, et memes valeurs que la boucle DFS de leaf.cpp) :
//   prefixes = m + somme sur P developpe de |CE(P)| ; judged = presentations soumises au census ;
//   region_line_tests : 1 par triplet visite qui passe G3, puis pour chaque quadruplet visite qui passe G3 les faces
//   (i0,i1,i3), (i0,i2,i3), (i1,i2,i3) jusqu'au premier echec ; toute face d'un tel quadruplet est un triplet visite
//   qui passe G3 (preuve dans README.md), donc avec le cache J2 : evaluations = nombre de ces triplets (T3),
//   cache_hits = tests - evaluations ; sans cache : evaluations = tests, cache_hits = 0.
// Census par masques (lemme R) et vote du warp : chaque voie classe un site ; premier evenement de l'ordre sequentiel
// (contrat Q1 de l'auditeur v11) : rejet au (theta+1)-ieme interieur, non resolue au premier site non certifie.
#pragma once

#include "mhgp12/leaf/predicates.hpp"

namespace mhgp12::leaf {

using simt::Lanes;

inline constexpr u32 kCounters = 15;
enum Counter : u32 {
  kDominanceTests = 0,
  kPrefixes,
  kJudged,
  kCensusTests,
  kEmitted,
  kIncidences,
  kQ4Candidates,
  kQ4Levels,
  kRegionPairTests,
  kRegionPairRejects,
  kRegionLineTests,
  kRegionLineRejects,
  kRegionLineEvaluations,
  kRegionLineCacheHits,
  kRegionLineFallbacks
};
inline constexpr u32 kStatusOk = 0, kStatusUnresolved = 1;
inline constexpr u8 kNoLocal = 0xFF;

// Une feuille : memes champs que leaf_device::Input de la v11.
struct Input {
  const u32* x = nullptr;  // coordonnees du nuage, par SiteIdx global
  const u32* y = nullptr;
  const u32* z = nullptr;
  const u32* sites = nullptr;  // SiteIdx globaux croissants de la feuille
  u32 m = 0;                   // 1 <= m <= 32
  i64 lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0};  // boite T0 demi-ouverte
  int kmax = 0;
  bool cache = false;  // option cache_center_lines : seulement les compteurs evaluations/hits
};

struct Counts {
  u32 c[kCounters];
};

// Emission resolue, en rangs locaux ; populations par masques (I puis U, bits croissants).
struct Emission {
  u8 support[4];
  u8 p, m, qmin, q;
  u32 interior, shell;
};

// Memoire partagee d'un warp (commune aux deux formes).
struct Shared {
  u32 P[kMaxSites][3];
  u32 dom[kMaxSites];    // dom[i] : sites qui dominent i sur la fermeture de la boite
  u32 domby[kMaxSites];  // domby[i] : sites que i domine
  u32 nbr[kMaxSites];    // graphe de paires : ni l'un ni l'autre ne domine
  u32 live[3][kMaxSites];
};

// Diagnostics physiques (hote seulement, jamais dans une empreinte) : super-pas et voies utiles.
struct Diag {
  u64 leaves = 0, rounds[5] = {0, 0, 0, 0, 0}, items[5] = {0, 0, 0, 0, 0}, censuses = 0, chunks = 0;
  u64 steps = 0, active = 0;  // forme coherente : pas d'enfants et voies actives
};

// Etat uniforme d'une feuille en cours.
struct Ctx {
  const Input& in;
  Shared& S;
  u32 m;
  int K;
  bool unresolved;
  u32 judged, census_tests, emitted, incidences, q4_levels;
  Diag* diag;
};

MHGP12_HD u32 hrow(u32 a, u32 b) {  // ligne (a, b), a < b < 32, triangle superieur dense
  return a * 31u - a * (a + 1u) / 2u + (b - 1u);
}

// Chargement, etendue, dominances, graphe de paires et lignes vivantes. Faux si la feuille est trop etendue
// (non resolue, comme la v11 : aucun prefixe).
MHGP12_HD bool prepare(const Input& in, Shared& S) {
  const u32 m = in.m;
  const int K = in.kmax;
  MHGP12_LANES(l) {
    if (l < m) {
      const u32 s = in.sites[l];
      S.P[l][0] = in.x[s];
      S.P[l][1] = in.y[s];
      S.P[l][2] = in.z[s];
    }
  }
  simt::sync();
  if constexpr (kBits > 20) {
    i64 low[3] = {in.lo[0], in.lo[1], in.lo[2]}, high[3] = {in.hi[0], in.hi[1], in.hi[2]};
    for (u32 i = 0; i < m; ++i)
      for (int a = 0; a < 3; ++a) {
        const i64 v = S.P[i][a];
        low[a] = v < low[a] ? v : low[a];
        high[a] = v > high[a] ? v : high[a];
      }
    if (high[0] - low[0] > kNarrowSpan || high[1] - low[1] > kNarrowSpan || high[2] - low[2] > kNarrowSpan)
      return false;
  }
  // Une ligne par voie : chaque couple est evalue par ses deux voies, toujours dans l'orientation canonique (i < j)
  // de la v11, donc avec la meme decision entiere. Sommes < 12*2^(2B) en i64 (2B+5 <= 63).
  MHGP12_LANES(x) {
    u32 dom = 0, domby = 0, nbr = 0;
    if (x < m) {
      for (u32 y = 0; y < m; ++y) {
        if (y == x) continue;
        const u32 i = x < y ? x : y, j = x < y ? y : x;
        i64 base = 0, cmin = 0, cmax = 0;
        for (int a = 0; a < 3; ++a) {
          const i64 pi = S.P[i][a], pj = S.P[j][a];
          const i64 delta = pj - pi;
          base += pj * pj - pi * pi;
          cmin += (delta > 0 ? in.lo[a] : in.hi[a]) * delta;
          cmax += (delta > 0 ? in.hi[a] : in.lo[a]) * delta;
        }
        if (base - 2 * cmin < 0) {  // j domine i
          if (x == i) dom |= 1u << j;
          else domby |= 1u << i;
        } else if (base - 2 * cmax > 0) {  // i domine j
          if (x == j) dom |= 1u << i;
          else domby |= 1u << j;
        } else {
          nbr |= 1u << y;
        }
      }
    }
    S.dom[x] = dom;
    S.domby[x] = domby;
    S.nbr[x] = nbr;
  }
  simt::sync();
  MHGP12_LANES(x) {
    u32 l0 = 0, l1 = 0, l2 = 0;
    if (x < m) {
      const u32 dx = S.dom[x];
      for (u32 rest = S.nbr[x]; rest != 0; rest &= rest - 1) {
        const u32 y = simt::ctz(rest);
        const int w = static_cast<int>(simt::popc(dx | S.dom[y]));
        l0 |= (w <= K - 1 ? 1u : 0u) << y;
        l1 |= (w <= K - 2 ? 1u : 0u) << y;
        l2 |= (w <= K - 3 ? 1u : 0u) << y;
      }
    }
    S.live[0][x] = l0;
    S.live[1][x] = l1;
    S.live[2][x] = l2;
  }
  simt::sync();
  return true;
}

// Support canonique (support.cpp de la v11, port leaf_device::canonical) sur la coquille complete : paires de milieu,
// puis triangles strictement aigus coplanaires au centre, puis tetraedres contenant strictement le centre ; premier
// evenement de l'ordre lexicographique (succes, ou refus non certifie qui rend la feuille non resolue). Uniforme et
// sequentiel : coquilles etendues rares (0,02 a 0,04 % des boules). Chemin froid hors ligne : il ne charge pas les
// registres du chemin chaud (mesure ptxas : 224 -> 178 registres pour j3 libre, debordements quasi nuls a 168).
MHGP12_HD_COLD bool canonical(Ctx& X, const Center& s, u32 shell, u8 (&support)[4], u32& qmin) {
  const Shared& S = X.S;
  for (u32 ri = shell; ri != 0; ri &= ri - 1) {
    const u32 a = simt::ctz(ri);
    for (u32 rj = ri & (ri - 1); rj != 0; rj &= rj - 1) {
      const u32 b = simt::ctz(rj);
      if (midpoint(s, S.P[a], S.P[b])) {
        support[0] = static_cast<u8>(a);
        support[1] = static_cast<u8>(b);
        support[2] = support[3] = kNoLocal;
        qmin = 2;
        return true;
      }
    }
  }
  for (u32 ri = shell; ri != 0; ri &= ri - 1) {
    const u32 a = simt::ctz(ri);
    for (u32 rj = ri & (ri - 1); rj != 0; rj &= rj - 1) {
      const u32 b = simt::ctz(rj);
      for (u32 rk = rj & (rj - 1); rk != 0; rk &= rk - 1) {
        const u32 c = simt::ctz(rk);
        if (!strictly_acute(S.P[a], S.P[b], S.P[c])) continue;
        int plane = 0;
        if (!center_orientation(S.P[a], S.P[b], S.P[c], s, plane)) {
          X.unresolved = true;
          return false;
        }
        if (plane == 0) {
          support[0] = static_cast<u8>(a);
          support[1] = static_cast<u8>(b);
          support[2] = static_cast<u8>(c);
          support[3] = kNoLocal;
          qmin = 3;
          return true;
        }
      }
    }
  }
  for (u32 ri = shell; ri != 0; ri &= ri - 1) {
    const u32 a = simt::ctz(ri);
    for (u32 rj = ri & (ri - 1); rj != 0; rj &= rj - 1) {
      const u32 b = simt::ctz(rj);
      for (u32 rk = rj & (rj - 1); rk != 0; rk &= rk - 1) {
        const u32 c = simt::ctz(rk);
        for (u32 rl = rk & (rk - 1); rl != 0; rl &= rl - 1) {
          const u32 d = simt::ctz(rl);
          const u32* p[4] = {S.P[a], S.P[b], S.P[c], S.P[d]};
          bool inside = false;
          if (!center_inside(s, p, inside)) {
            X.unresolved = true;
            return false;
          }
          if (inside) {
            support[0] = static_cast<u8>(a);
            support[1] = static_cast<u8>(b);
            support[2] = static_cast<u8>(c);
            support[3] = static_cast<u8>(d);
            qmin = 4;
            return true;
          }
        }
      }
    }
  }
  X.unresolved = true;  // leaf.cpp : catalogue_invariant
  return false;
}

// Census d'une presentation (generateurs gen[0..q) en rangs locaux croissants, centre c), puis support canonique,
// admission et emission. Appel uniforme ; chaque voie classe le site de son rang.
template <class Sink>
MHGP12_HD void census(Ctx& X, Sink& sink, u32 q, const u8 (&gen)[4], const Center& c) {
  const Shared& S = X.S;
  const u32 m = X.m;
  ++X.judged;
  if (X.diag != nullptr) ++X.diag->censuses;
  u32 gm = 0, inside = 0, outside = 0;
  for (u32 t = 0; t < q; ++t) {
    gm |= 1u << gen[t];
    inside |= S.dom[gen[t]];
    outside |= S.domby[gen[t]];
  }
  if ((inside & outside) != 0) {  // leaf.cpp : catalogue_invariant (centre hors de la boite)
    X.unresolved = true;
    return;
  }
  Lanes<bool> is_in, is_on, unknown;
  MHGP12_LANES(s) {
    is_in[s] = is_on[s] = unknown[s] = false;
    if (s < m) {
      if ((gm >> s) & 1u) {
        is_on[s] = true;  // contact certifie par la fabrique
      } else if ((inside >> s) & 1u) {
        is_in[s] = true;  // lemme R
      } else if (!((outside >> s) & 1u)) {
        int r = 0;
        bool certified = true;
        if (q == 2) r = side_q2_narrow(S.P[gen[0]], S.P[gen[1]], S.P[s]);
        else certified = side(c, S.P[s], r);
        if (!certified) unknown[s] = true;
        else if (r < 0) is_in[s] = true;
        else if (r == 0) is_on[s] = true;
      }
    }
  }
  const u32 I = simt::ballot(is_in), C = simt::ballot(is_on), U = simt::ballot(unknown);
  const u32 theta = static_cast<u32>(X.K + 1) - q;
  const u32 t = simt::popc(I) > theta ? simt::select_nth(I, theta + 1) : m;
  const u32 u = U != 0 ? simt::ctz(U) : m;
  if (t < m || u < m) {
    const u32 e = t < u ? t : u;
    X.census_tests += e + 1;
    if (u < t) X.unresolved = true;  // premier site non certifie atteint avant l'arret
    return;                          // sinon rejet au (theta+1)-ieme interieur
  }
  X.census_tests += m;
  const u32 p = simt::popc(I), count = simt::popc(C);
  u8 support[4] = {gen[0], gen[1], q > 2 ? gen[2] : kNoLocal, q > 3 ? gen[3] : kNoLocal};
  u32 qmin = q;
  if (count != q) {
    if (!canonical(X, c, C, support, qmin)) return;
    for (u32 k = 0; k < 4; ++k)
      if (support[k] != (k < q ? gen[k] : kNoLocal)) return;  // S* sera visite dans cette meme feuille
  }
  if (p + qmin > static_cast<u32>(X.K) + 1) return;
  if (q == 4) ++X.q4_levels;
  ++X.emitted;
  X.incidences += p + count;
  Emission e;
  for (u32 k = 0; k < 4; ++k) e.support[k] = support[k];
  e.p = static_cast<u8>(p);
  e.m = static_cast<u8>(count);
  e.qmin = static_cast<u8>(qmin);
  e.q = static_cast<u8>(q);
  e.interior = I;
  e.shell = C;
  sink.emit(e);
}

// Compteurs finaux : sommes des accumulateurs par voie et compteurs uniformes.
struct LaneAccumulators {
  Lanes<u32> prefixes, line_tests3, line_tests4, line_rejects, q4_candidates;
};

MHGP12_HD void clear(LaneAccumulators& a) {
  MHGP12_LANES(l) {
    a.prefixes[l] = 0;
    a.line_tests3[l] = 0;
    a.line_tests4[l] = 0;
    a.line_rejects[l] = 0;
    a.q4_candidates[l] = 0;
  }
}

MHGP12_HD void finish(const Ctx& X, const LaneAccumulators& a, Counts& out) {
  const u32 m = X.m;
  const u32 tests3 = simt::sum(a.line_tests3), tests4 = simt::sum(a.line_tests4);
  out.c[kDominanceTests] = m * (m - 1) / 2;
  out.c[kPrefixes] = m + simt::sum(a.prefixes);
  out.c[kJudged] = X.judged;
  out.c[kCensusTests] = X.census_tests;
  out.c[kEmitted] = X.emitted;
  out.c[kIncidences] = X.incidences;
  out.c[kQ4Candidates] = simt::sum(a.q4_candidates);
  out.c[kQ4Levels] = X.q4_levels;
  out.c[kRegionPairTests] = 0;  // graphe de paires : aucun couple teste dans le DFS
  out.c[kRegionPairRejects] = 0;
  out.c[kRegionLineTests] = tests3 + tests4;
  out.c[kRegionLineRejects] = simt::sum(a.line_rejects);
  out.c[kRegionLineEvaluations] = X.in.cache ? tests3 : tests3 + tests4;
  out.c[kRegionLineCacheHits] = X.in.cache ? tests4 : 0;
  out.c[kRegionLineFallbacks] = 0;  // m <= 32 : jamais de repli du cache
}

MHGP12_HD void zero(Counts& out) {
  for (u32 f = 0; f < kCounters; ++f) out.c[f] = 0;
}

}  // namespace mhgp12::leaf
