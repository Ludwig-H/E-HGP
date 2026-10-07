// Feuille J3, partie commune : memoire d'un warp, preparation (dominances, graphe de paires, lignes vivantes),
// compteurs logiques. Port explicite de microbancs/mes_m2_feuille/include/mhgp12/leaf/leaf_common.hpp (MES-M2), sur le
// warp de N voies de simt.hpp (une voie par site : N = 32 sur l'appareil et l'hote, N = 256 sur l'hote seulement).
//
// Semantique de reference : la feuille de la v11 (src/catalogue/leaf.cpp, ac081a06f), definie par ENSEMBLES, donc
// independante de l'ordre de visite (preuve et lemme des faces : README de MES-M2, paragraphe 3) :
//   cnt(P) = |union des Dom(p)| ; live[t][x] = voisins y de x avec |Dom(x) u Dom(y)| <= K-1-t (t = 0, 1, 2) ;
//   P = (i0 < ... < i_{q-1}) est DEVELOPPE s'il passe G3 (cnt(P) <= K+1-q), la droite J2 si q = 3, et si q < 4 et
//   K >= q ; ses enfants logiques sont CE(P) = au-dessus(i_{q-1}) inter les voisins de P.
// Compteurs : ceux de leaf.cpp, champ par champ. Une feuille d'au plus 32 sites suit la voie << graphe de paires >>
// de la v11 (prefixes = m + somme des |CE(P)|, aucun test de couple, cache J2 : evaluations = triplets juges,
// succes = le reste). Une feuille de 33 a 256 sites suit, dans la v11, le DFS historique sans graphe de paires ni
// cache : ses enfants visites sont TOUS les sites au-dessus du dernier, chacun teste contre les sites du prefixe dans
// l'ordre jusqu'au premier couple dominant (region_pair_tests, region_pair_rejects), et chaque demande de droite est
// une evaluation et un repli. Ses compteurs ont la forme close ci-dessous (dfs_children), sur les memes prefixes
// developpes : l'ensemble des presentations jugees est le meme dans les deux voies (un enfant hors du graphe de paires
// echoue au test de couple, un enfant hors des lignes vivantes echoue a G3), donc aussi judged, census_tests, emitted,
// incidences, q4_candidates, q4_levels et les tests de droites.
#pragma once

#include "catalogue/leaf_predicates.hpp"

namespace mhgp12::catalogue_detail {

inline constexpr u32 kLeafCounters = 15;
enum LeafCounter : u32 {
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
struct LeafCounts {
  u64 c[kLeafCounters];
};

// Rang local d'un site dans sa feuille (0..255 sur le warp virtuel) ; kNoLocal marque une case vide de S*.
using LocalRank = u16;
inline constexpr LocalRank kNoLocal = 0xFFFF;
inline constexpr u32 kGraphSites = 32;  // voie << graphe de paires >> de la v11 : m <= 32
inline constexpr u32 kQueue = 512;      // file d'items d'une phase, par tranches

// Issue d'une feuille : succes, coquille au-dela du plafond declare, invariant (faute arithmetique ou de census).
enum LeafStatus : u32 { kLeafOk = 0, kLeafShellCapacity = 1, kLeafInvariant = 2 };

// Une feuille : sites (SiteIdx croissants) et boite demi-ouverte en coordonnees ABSOLUES ; origin = coin minimal du
// repere E (fermeture de la boite et sites de la liste), soustrait au chargement.
struct LeafInput {
  const u32* x = nullptr;
  const u32* y = nullptr;
  const u32* z = nullptr;
  const u32* sites = nullptr;
  u32 m = 0;
  i64 lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0};
  i64 origin[3] = {0, 0, 0};
  int kmax = 0;
};

template <u32 N>
using MaskOf = typename simt::Width<N>::Mask;

// Emission resolue, en rangs locaux ; populations par masques (I puis U, bits croissants).
template <u32 N>
struct Emission {
  LocalRank support[4];
  u8 p, qmin, q, pad;
  u32 m;
  MaskOf<N> interior, shell;
};

template <u32 N>
inline constexpr u32 kPairRows = N * (N - 1) / 2;

// Memoire partagee d'un warp. P : coordonnees locales ; dom[i] : sites qui dominent i sur la fermeture de la boite ;
// domby[i] : sites que i domine ; nbr : graphe de paires ; H[(i,j)] bit k : triplet (i,j,k) vivant (juge J2 positif).
template <u32 N>
struct LeafShared {
  u32 P[N][3];
  MaskOf<N> dom[N], domby[N], nbr[N];
  MaskOf<N> live[3][N];
  MaskOf<N> H[kPairRows<N>];
  u32 queue[kQueue];
};

template <u32 N>
MHGP12_HD u32 hrow(u32 a, u32 b) {  // ligne (a, b), a < b < N, triangle superieur dense
  return a * (N - 1u) - a * (a + 1u) / 2u + (b - 1u);
}

// Accumulateur par voie : u32 sur le warp de l'appareil (bornes de MES-M2), u64 sur le warp large de l'hote.
template <u32 N>
using AccOf = std::conditional_t<N == simt::kWarp, u32, u64>;
template <u32 N>
struct LaneAccumulators {
  simt::Lanes<AccOf<N>, N> prefixes, pair_tests, pair_rejects, line_tests3, line_tests4, line_rejects, q4_candidates;
};

template <u32 N>
MHGP12_HD u64 lane_sum(const simt::Lanes<AccOf<N>, N>& v) {
  if constexpr (N == simt::kWarp) {
    return simt::sum<N>(v);
  } else {
    u64 s = 0;
    for (u32 l = 0; l < N; ++l) s += v[l];
    return s;
  }
}

// Etat uniforme d'une feuille en cours. lo/hi : boite en coordonnees locales.
template <u32 N, class A>
struct LeafCtx {
  LeafShared<N>& S;
  u32 m;
  int K;
  i64 lo[3], hi[3];
  u32 status;
  u64 judged, census_tests, emitted, incidences, q4_levels;
};

// Chargement en repere local, dominances, graphe de paires et lignes vivantes. Une ligne par voie : chaque couple est
// evalue par ses deux voies, toujours dans l'orientation canonique (i < j) de la v11, donc avec la meme decision.
// Dominance |p_j|^2 - |p_i|^2 - 2 c.(p_j - p_i) aux coins : < 9 M^2 en coordonnees locales (Small).
template <u32 N, class A>
MHGP12_HD void prepare(const LeafInput& in, LeafCtx<N, A>& X) {
  using S = typename A::Small;
  using W = simt::Width<N>;
  LeafShared<N>& L = X.S;
  const u32 m = in.m;
  const int K = in.kmax;
  MHGP12_LANES(N, l) {
    if (l < m) {
      const u32 s = in.sites[l];
      L.P[l][0] = static_cast<u32>(i64(in.x[s]) - in.origin[0]);
      L.P[l][1] = static_cast<u32>(i64(in.y[s]) - in.origin[1]);
      L.P[l][2] = static_cast<u32>(i64(in.z[s]) - in.origin[2]);
    }
  }
  simt::sync();
  MHGP12_LANES(N, x) {
    auto dom = W::empty(), domby = W::empty(), nbr = W::empty();
    if (x < m) {
      for (u32 y = 0; y < m; ++y) {
        if (y == x) continue;
        const u32 i = x < y ? x : y, j = x < y ? y : x;
        S base = 0, cmin = 0, cmax = 0;
        for (int a = 0; a < 3; ++a) {
          const S pi = S(L.P[i][a]), pj = S(L.P[j][a]);
          const S delta = pj - pi;
          base += pj * pj - pi * pi;
          cmin += S(delta > 0 ? X.lo[a] : X.hi[a]) * delta;
          cmax += S(delta > 0 ? X.hi[a] : X.lo[a]) * delta;
        }
        if (base - 2 * cmin < 0) {  // j domine i
          if (x == i) dom = dom | W::bit(j);
          else domby = domby | W::bit(i);
        } else if (base - 2 * cmax > 0) {  // i domine j
          if (x == j) dom = dom | W::bit(i);
          else domby = domby | W::bit(j);
        } else {
          nbr = nbr | W::bit(y);
        }
      }
    }
    L.dom[x] = dom;
    L.domby[x] = domby;
    L.nbr[x] = nbr;
  }
  simt::sync();
  MHGP12_LANES(N, x) {
    auto l0 = W::empty(), l1 = W::empty(), l2 = W::empty();
    if (x < m) {
      for (auto rest = L.nbr[x]; simt::any(rest); rest = simt::clear_lowest(rest)) {
        const u32 y = simt::ctz(rest);
        const int w = static_cast<int>(simt::popc(L.dom[x] | L.dom[y]));
        if (w <= K - 1) l0 = l0 | W::bit(y);
        if (w <= K - 2) l1 = l1 | W::bit(y);
        if (w <= K - 3) l2 = l2 | W::bit(y);
      }
    }
    L.live[0][x] = l0;
    L.live[1][x] = l1;
    L.live[2][x] = l2;
  }
  simt::sync();
}

// Enfants d'un prefixe developpe dont le dernier site est `last` et dont les sites sont adjacents a ceux de `common`
// (intersection des lignes du graphe de paires du prefixe, `last` compris). Voie graphe : |CE(P)|. Voie DFS (m > 32) :
// tous les sites au-dessus de `last` ; tests de couples dans l'ordre du prefixe, arret au premier couple dominant
// (nbr_j : intersection des lignes des j premiers sites du prefixe).
template <u32 N>
MHGP12_HD void children(LaneAccumulators<N>& acc, u32 lane, u32 m, u32 last, u32 depth,
                        const MaskOf<N> (&nbr_prefix)[4]) {
  using W = simt::Width<N>;
  const auto above = W::above(last);
  if (m <= kGraphSites) {
    acc.prefixes[lane] += simt::popc(above & nbr_prefix[depth - 1]);
    return;
  }
  const u32 all = m - 1u - last;  // sites au-dessus de last dans la feuille
  acc.prefixes[lane] += all;
  u64 tests = all;
  for (u32 j = 1; j < depth; ++j) tests += simt::popc(above & nbr_prefix[j - 1]);
  acc.pair_tests[lane] += static_cast<AccOf<N>>(tests);
  acc.pair_rejects[lane] += all - simt::popc(above & nbr_prefix[depth - 1]);
}

// Compteurs finaux : sommes des accumulateurs par voie et compteurs uniformes.
template <u32 N, class A>
MHGP12_HD void finish(const LeafCtx<N, A>& X, const LaneAccumulators<N>& a, LeafCounts& out) {
  const u64 m = X.m;
  const u64 tests3 = lane_sum<N>(a.line_tests3), tests4 = lane_sum<N>(a.line_tests4);
  const bool graph = X.m <= kGraphSites;
  out.c[kDominanceTests] = m * (m - 1) / 2;
  out.c[kPrefixes] = m + lane_sum<N>(a.prefixes);
  out.c[kJudged] = X.judged;
  out.c[kCensusTests] = X.census_tests;
  out.c[kEmitted] = X.emitted;
  out.c[kIncidences] = X.incidences;
  out.c[kQ4Candidates] = lane_sum<N>(a.q4_candidates);
  out.c[kQ4Levels] = X.q4_levels;
  out.c[kRegionPairTests] = lane_sum<N>(a.pair_tests);
  out.c[kRegionPairRejects] = lane_sum<N>(a.pair_rejects);
  out.c[kRegionLineTests] = tests3 + tests4;
  out.c[kRegionLineRejects] = lane_sum<N>(a.line_rejects);
  out.c[kRegionLineEvaluations] = graph ? tests3 : tests3 + tests4;
  out.c[kRegionLineCacheHits] = graph ? tests4 : 0;
  out.c[kRegionLineFallbacks] = graph ? 0 : tests3 + tests4;
}

}  // namespace mhgp12::catalogue_detail
