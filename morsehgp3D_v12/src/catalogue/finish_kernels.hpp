// Fin d'etage, source unique hote et appareil : noyaux de l'ordre canonique, des rangs, du CSR des populations et de la
// table S* -> boule. Un warp par tuile de kTileItems elements, 32 paquets de 32 (voies consecutives sur elements
// consecutifs) ; chaque element est ecrit par sa seule voie ; les fautes sont des bits d'un mot global (OU, ordre
// indifferent). Ordre canonique de la voie CPU de la v12 (sort.cpp de 671072339) : niveau exact, puis S* compare par la
// liste triee des POSITIONS de ses sites ; ici la liste des positions devient la liste triee des rangs
// lexicographiques des sites (cle pkey, 0 en queue d'un support plus court : un support plus court et prefixe de
// l'autre passe avant, comme compare_support_positions), et le niveau exact est trie par sa cle F3 puis verifie.
#pragma once

#include "catalogue/finish_level.hpp"
#include "catalogue/finish_sort.hpp"
#include "catalogue/internal.hpp"

namespace mhgp12::catalogue_detail::fin {

inline constexpr u32 kChunks = kTileItems / kWarp;

// Bits du mot de fautes de la fin d'etage.
enum : u32 {
  kFaultBall = 1,         // enregistrement invalide ou support degenere (invariant)
  kFaultZero = 2,         // premier niveau nul (invariant)
  kFaultDuplicate = 4,    // meme niveau et meme S* : doublon d'emission (invariant)
  kFaultMisordered = 8,   // voisins mal ordonnes par la cle F3 : chaine a retrier en exact (repli)
};
// Verdicts d'une paire de voisins (i - 1, i) de l'ordre trie.
enum : u32 { kPairOk = 0, kPairMisordered = 1, kPairDuplicate = 2 };

MHGP12_HD u64 item(u64 tile, u32 c, u32 l) { return tile * kTileItems + u64{c} * kWarp + l; }

// Cle de 4 composantes de `bits` bits (bits <= 32), la premiere en poids fort.
MHGP12_HD Key2 pack4(const u32 (&c)[4], u32 bits) {
  u128 v = 0;
  for (int k = 0; k < 4; ++k) v = (v << bits) | c[k];
  return Key2{static_cast<u64>(v), static_cast<u64>(v >> 64)};
}
MHGP12_HD u32 first_of(const Key2& key, u32 bits) {
  const u128 v = (static_cast<u128>(key.hi) << 64) | key.lo;
  return static_cast<u32>(v >> (3 * bits));
}
MHGP12_HD int key2_cmp(const Key2& a, const Key2& b) {
  if (a.hi != b.hi) return a.hi < b.hi ? -1 : 1;
  if (a.lo != b.lo) return a.lo < b.lo ? -1 : 1;
  return 0;
}

// Cle lexicographique des sites (x en mot haut, puis y et z) et valeurs initiales.
struct SiteKeyKernel {
  const u32 *x, *y, *z;
  u64 n;
  Key2* keys;
  u32* vals;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n) {
          keys[i] = Key2{(u64{y[i]} << 32) | z[i], x[i]};
          vals[i] = static_cast<u32>(i);
        }
      }
    }
  }
};

// Rang lexicographique 1..n de chaque site.
struct LexRankKernel {
  const u32* order;
  u64 n;
  u32* rank;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n) rank[order[i]] = static_cast<u32>(i + 1);
      }
    }
  }
};

// Niveau exact, cle F3, cle des positions de S* et valeur initiale de chaque boule.
struct BallKeyKernel {
  const u32 *x, *y, *z;
  u64 sites;
  const BallRecord* records;
  u64 balls, incidences;
  const u32* lexrank;
  u32 rank_bits;
  LevelWords* levels;
  u64* fkey;
  Key2* pkey;       // par boule (verification des voisins)
  Key2* sort_keys;  // copie, entree du tri par positions
  u32* vals;
  u32* fault;
  struct Shared {};
  MHGP12_HD void one(u64 i) const {
    const BallRecord& r = records[i];
    const u32 q = r.qmin;
    bool bad = q < 2 || q > 4 || r.population > incidences || u64{r.p} + r.m > incidences - r.population;
    u32 pts[4][3] = {}, ranks[4] = {0, 0, 0, 0};
    const u32* p[4] = {pts[0], pts[1], pts[2], pts[3]};
    for (u32 k = 0; k < 4 && !bad; ++k) {
      if (k >= q) break;
      const u32 s = r.support[k];
      bad = s >= sites;
      if (bad) break;
      pts[k][0] = x[s];
      pts[k][1] = y[s];
      pts[k][2] = z[s];
      ranks[k] = lexrank[s];
    }
    LevelWords level{};
    bool faulty = bad;
    if (!bad) ball_level(p, q, level, faulty);
    const u32 count = bad ? 0u : q;
    for (u32 k = 1; k < count; ++k)  // rangs croissants ; cases absentes nulles, en queue
      for (u32 j = k; j > 0 && ranks[j] < ranks[j - 1]; --j) {
        const u32 t = ranks[j];
        ranks[j] = ranks[j - 1];
        ranks[j - 1] = t;
      }
    levels[i] = level;
    fkey[i] = faulty ? 0u : level_key_bits(level);
    pkey[i] = pack4(ranks, rank_bits);
    sort_keys[i] = pkey[i];
    vals[i] = static_cast<u32>(i);
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

// Cle F3 et boule de chaque position de l'ordre partiel (entree du tri suivant).
struct GatherKeyKernel {
  const u32* order;
  const u64* fkey;
  u64 n;
  u64* keys;
  u32* vals;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n) {
          keys[i] = fkey[order[i]];
          vals[i] = order[i];
        }
      }
    }
  }
};

// Niveaux et cles de positions des boules d'une liste (elements des chaines a retrier, repli exact).
struct PickKernel {
  const u32* balls;
  u64 n;
  const LevelWords* levels;
  const Key2* pkey;
  LevelWords* out_levels;
  Key2* out_pkey;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 j = item(tile, c, l);
        if (j < n) {
          out_levels[j] = levels[balls[j]];
          out_pkey[j] = pkey[balls[j]];
        }
      }
    }
  }
};

// Paires de voisins de l'ordre trie : debut de niveau (flag), verdict. Ordre certain par F4, sinon comparaison
// exacte des niveaux ; a niveau egal, S* strictement croissant (sinon doublon) ; niveau decroissant : chaine a retrier.
struct ChainKernel {
  const u32* order;
  const u64* keys;  // cles F3 dans l'ordre trie
  const LevelWords* levels;
  const Key2* pkey;
  u64 n;
  u64* flag;
  u32* verdict;
  u32* fault;
  struct Shared {};
  MHGP12_HD void one(u64 i) const {
    if (i == 0) {
      flag[0] = 1;
      verdict[0] = kPairOk;
      if (bit_length(levels[order[0]].n) == 0) simt::global_or(fault, kFaultZero);
      return;
    }
    const u32 a = order[i - 1], b = order[i];
    const u64 ka = keys[i - 1], kb = keys[i];
    const int order_key = key_order(ka, kb);
    const bool certain = order_key < 0;
    const int c = certain ? -1 : compare_levels(levels[a], levels[b]);
    u32 v = order_key > 0 || c > 0 ? kPairMisordered : kPairOk;
    if (c == 0) {
      const int s = key2_cmp(pkey[a], pkey[b]);
      v = s < 0 ? kPairOk : s == 0 ? kPairDuplicate : kPairMisordered;
    }
    flag[i] = c == 0 ? 0u : 1u;
    verdict[i] = v;
    if (v != kPairOk) simt::global_or(fault, v == kPairDuplicate ? kFaultDuplicate : kFaultMisordered);
  }
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n) one(i);
      }
    }
  }
};

// Longueur de la population (I puis U) de chaque position.
struct LengthKernel {
  const u32* order;
  const BallRecord* records;
  u64 n;
  u64* length;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n) length[i] = u64{records[order[i]].p} + records[order[i]].m;
      }
    }
  }
};

// Boules dans l'ordre canonique, populations copiees a leurs decalages, niveau de chaque debut de rang.
struct EmitKernel {
  const u32* order;
  const BallRecord* records;
  const SiteIdx* population;
  const u64* flag;
  const u64* before;  // nombre de debuts de rang avant chaque position (prefixe exclusif de flag)
  const u64* offset;  // decalage CSR de chaque position
  const LevelWords* levels;
  u64 n;
  CatalogueBall* balls;
  SiteIdx* values;
  LevelWords* level_of_rank;  // rangs 1..L-1 aux cases 0..L-2
  struct Shared {};
  MHGP12_HD void one(u64 i) const {
    const u32 b = order[i];
    const BallRecord& r = records[b];
    const u64 rank = before[i] + flag[i];
    CatalogueBall out;
    for (u32 k = 0; k < 4; ++k) out.support[k] = static_cast<SiteIdx>(r.support[k]);
    out.rank = static_cast<LevelRank>(static_cast<u32>(rank));
    out.p = r.p;
    out.m = r.m;
    out.qmin = r.qmin;
    balls[i] = out;
    const u64 length = u64{r.p} + r.m;
    for (u64 j = 0; j < length; ++j) values[offset[i] + j] = population[r.population + j];
    if (flag[i] != 0) level_of_rank[rank - 1] = levels[b];
  }
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n) one(i);
      }
    }
  }
};

// Cle de la table S* -> boule : (S*[0], S*[1], S*[2], S*[3]) en SiteIdx, case absente = nombre de sites (au-dela de
// tout SiteIdx, comme kNone dans tail_less de la voie CPU).
struct TableKeyKernel {
  const CatalogueBall* balls;
  u64 n;
  u32 sites, bits;
  Key2* keys;
  u32* vals;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = item(tile, c, l);
        if (i < n) {
          u32 parts[4];
          for (u32 k = 0; k < 4; ++k) {
            const u32 s = static_cast<u32>(balls[i].support[k]);
            parts[k] = k < balls[i].qmin ? s : sites;
          }
          keys[i] = pack4(parts, bits);
          vals[i] = static_cast<u32>(i);
        }
      }
    }
  }
};

// Decalages de la table : premiere position dont S*[0] >= s, pour s = 0..sites (dichotomie dans les cles triees).
struct TableOffsetKernel {
  const Key2* keys;
  u64 n;
  u32 sites, bits;
  u64* offset;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    for (u32 c = 0; c < kChunks; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 s = item(tile, c, l);
        if (s <= sites) {
          u64 lo = 0, hi = n;
          while (lo < hi) {
            const u64 mid = lo + (hi - lo) / 2;
            if (first_of(keys[mid], bits) < s) lo = mid + 1;
            else hi = mid;
          }
          offset[s] = lo;
        }
      }
    }
  }
};

}  // namespace mhgp12::catalogue_detail::fin
