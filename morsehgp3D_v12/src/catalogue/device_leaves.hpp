// Voie appareil du catalogue (tranche T1-b), source unique : noyaux d'un lot de feuilles. Un warp par feuille.
//   Classify : repere de la feuille (fermeture de sa boite et sites de sa liste, comme num::Frame : coin minimal,
//              etendue) ; cause de non-resolution (m > 32 : warp virtuel de l'hote ; etendue > 16 : politique exacte)
//              et cle d'ordre par taille decroissante ;
//   Count    : feuille J3 run_leaf<32, Narrow> (contrat R7 : seule decide la voie garantie par le palier etroit), cases de
//              kCase emissions par feuille, comptes et quinze compteurs logiques ;
//   Copy     : copie des cases aux places exactes du lot (noyau leger) ;
//   Replay   : rejeu, par la meme source, des seules feuilles qui debordent leur case ; memes comptes exiges ;
//   Compact  : feuilles non resolues (enregistrement et sites) rangees dans l'ordre, pour le rejeu exact de l'hote ;
//   Reduce   : sommes des compteurs, paliers, feuilles reecrites (plus de kCase emissions, rejouees par Replay),
//              etendue maximale et premiere faute du lot (cle profondeur, priorite).
// Joues par l'executeur CUDA (produit) et par l'executeur Pool (portes de l'hote, meme code).
#pragma once

#include "catalogue/finish_sort.hpp"
#include "catalogue/internal.hpp"

namespace mhgp12::catalogue_detail::dev {

using fin::kTileItems;
using simt::kWarp;
using simt::Lanes;

inline constexpr u64 kBatchLeaves = u64{1} << 17;  // feuilles par lot : cases <= 192 Mio
inline constexpr u32 kCase = 64;                   // emissions gardees par feuille au comptage (voie CPU : 64)
inline constexpr u32 kLeafUnresolved = 3;          // statut d'une feuille laissee a l'hote
enum : u32 { kCauseWide = 1, kCauseSpan = 2 };      // m > 32 ; etendue > 16
inline constexpr u32 kFaultFill = 16;               // rejeu d'ecriture incoherent avec le comptage (invariant)
inline constexpr u64 kNoFailure = ~u64{0};

struct LeafClass {
  i64 origin[3];
  u32 span, cause;
};

// Totaux d'une tuile de feuilles, puis du lot : quinze compteurs, paliers (etroit, moyen, large, exact, warp
// virtuel : les deux derniers sont les causes de non-resolution), feuilles resolues reecrites par Replay (plus de kCase
// emissions), etendue maximale, premiere faute (profondeur * 4 + priorite : coquille 0, invariant 1).
struct BatchStats {
  u64 counts[kLeafCounters];
  u64 tiers[5];
  u64 rewritten, max_span, failure, overflow;
};

struct LeafView {
  const u32 *x, *y, *z;
  const bfs::Leaf* leaves;  // feuilles du lot
  const u32* sites;         // arene des sites (debuts absolus)
  u64 n;
  int kmax;
};

MHGP12_HD LeafInput leaf_input(const LeafView& v, u64 i, const LeafClass& c) {
  const bfs::Leaf& f = v.leaves[i];
  LeafInput in;
  in.x = v.x;
  in.y = v.y;
  in.z = v.z;
  in.sites = v.sites + f.begin;
  in.m = f.m;
  for (int a = 0; a < 3; ++a) {
    in.lo[a] = f.lo[a];
    in.hi[a] = f.hi[a];
    in.origin[a] = c.origin[a];
  }
  in.kmax = v.kmax;
  return in;
}

struct ClassifyKernel {
  LeafView v;
  LeafClass* cls;
  u64* size_key;
  u64* unresolved;        // 0 ou 1
  u64* unresolved_sites;  // m ou 0
  struct Shared {};
  MHGP12_HD void operator()(u64 i, Shared&) const {
    const bfs::Leaf& f = v.leaves[i];
    Lanes<u32> mn[3], mx[3];
    MHGP12_LANES(kWarp, l) {
      for (int a = 0; a < 3; ++a) {
        mn[a][l] = 0xFFFFFFFFu;
        mx[a][l] = 0;
      }
      for (u32 j = l; j < f.m; j += kWarp) {
        const u32 s = v.sites[f.begin + j];
        const u32 c[3] = {v.x[s], v.y[s], v.z[s]};
        for (int a = 0; a < 3; ++a) {
          mn[a][l] = c[a] < mn[a][l] ? c[a] : mn[a][l];
          mx[a][l] = c[a] > mx[a][l] ? c[a] : mx[a][l];
        }
      }
    }
    LeafClass out{};
    u64 width = 0;
    for (int a = 0; a < 3; ++a) {
      const i64 lo = static_cast<i64>(simt::reduce_min(mn[a])), hi = static_cast<i64>(simt::reduce_max(mx[a]));
      out.origin[a] = f.lo[a] < lo ? f.lo[a] : lo;  // fermeture [lo, hi] de la boite et sites de la liste
      const i64 top = f.hi[a] > hi ? f.hi[a] : hi;
      width = static_cast<u64>(top - out.origin[a]) > width ? static_cast<u64>(top - out.origin[a]) : width;
    }
    out.span = fin::width_of(width) - (width == 0 ? 1u : 0u);  // plus petit s tel que width < 2^s (0 si nulle)
    out.cause = (f.m > kGraphSites ? kCauseWide : 0u) | (out.span > static_cast<u32>(kLeafNarrowSpan) ? kCauseSpan : 0u);
    if (simt::leader()) {
      cls[i] = out;
      size_key[i] = out.cause != 0 ? 255u : 32u - f.m;  // grandes feuilles d'abord (MES-M2), non resolues en queue
      unresolved[i] = out.cause != 0 ? 1u : 0u;
      unresolved_sites[i] = out.cause != 0 ? f.m : 0u;
    }
  }
};

// Puits du comptage : compte, garde les kCase premieres emissions (ecrites par la voie 0).
struct CaseSink {
  Emission<32>* slots;
  u64 balls = 0, incidences = 0;
  MHGP12_HD void emit(const Emission<32>& e) {
    if (balls < kCase && simt::leader()) slots[balls] = e;
    ++balls;
    incidences += u64{e.p} + e.m;
  }
};

struct LeafOut {
  u64* balls;
  u64* incidences;
  u32* status;
  u64* counts;  // kLeafCounters par feuille
};

struct CountKernel {
  LeafView v;
  const LeafClass* cls;
  const u32* order;  // rang de passage -> feuille du lot
  Emission<32>* cases;
  LeafOut out;
  using Shared = LeafShared<32>;
  MHGP12_HD void operator()(u64 slot, Shared& shared) const {
    const u32 i = order[slot];
    const LeafClass c = cls[i];
    if (c.cause != 0) {
      if (simt::leader()) {
        out.balls[i] = out.incidences[i] = 0;
        out.status[i] = kLeafUnresolved;
        for (u32 f = 0; f < kLeafCounters; ++f) out.counts[u64{i} * kLeafCounters + f] = 0;
      }
      return;
    }
    CaseSink sink{cases + u64{i} * kCase};
    LeafCounts counts;
    const u32 status = run_leaf<32, Narrow>(leaf_input(v, i, c), shared, counts, sink);
    if (simt::leader()) {
      out.balls[i] = status == kLeafOk ? sink.balls : 0;
      out.incidences[i] = status == kLeafOk ? sink.incidences : 0;
      out.status[i] = status;
      for (u32 f = 0; f < kLeafCounters; ++f) out.counts[u64{i} * kLeafCounters + f] = status == kLeafOk ? counts.c[f] : 0;
    }
  }
};

// Ecriture d'une emission (rangs locaux) en boule globale : population I puis U par le warp, enregistrement par la
// voie 0 ; meme conversion que convert() de leaves.cpp.
MHGP12_HD void write_emission(const Emission<32>& e, const u32* leaf_sites, BallRecord* record, SiteIdx* population,
                              u64 at) {
  MHGP12_LANES(kWarp, l) {
    const u32 below = (1u << l) - 1u;
    if (simt::test(e.interior, l)) population[at + simt::popc(e.interior & below)] = static_cast<SiteIdx>(leaf_sites[l]);
    if (simt::test(e.shell, l))
      population[at + e.p + simt::popc(e.shell & below)] = static_cast<SiteIdx>(leaf_sites[l]);
  }
  if (simt::leader()) {
    BallRecord r;
    for (u32 k = 0; k < 4; ++k) r.support[k] = e.support[k] == kNoLocal ? kNone : leaf_sites[e.support[k]];
    r.population = at;
    r.p = e.p;
    r.m = static_cast<u8>(e.m);
    r.qmin = e.qmin;
    r.pad = 0;
    r.chunk = 0;
    *record = r;
  }
}

// Puits du rejeu d'ecriture : places exactes du comptage ; debordement = faute.
struct WriteSink {
  const u32* leaf_sites;
  BallRecord* records;
  SiteIdx* population;
  u64 at, balls_cap, incidences_cap;
  u64 balls = 0, incidences = 0;
  bool overflow = false;
  MHGP12_HD void emit(const Emission<32>& e) {
    if (balls >= balls_cap || incidences + e.p + e.m > incidences_cap) {
      overflow = true;
      return;
    }
    write_emission(e, leaf_sites, records + balls, population, at + incidences);
    ++balls;
    incidences += u64{e.p} + e.m;
  }
};

// Destination des boules d'un lot : enregistrements a partir de la premiere boule du lot, populations a partir de
// population_base, decalages exclusifs de chaque feuille dans le lot.
struct FillView {
  const LeafClass* cls;
  const Emission<32>* cases;
  const u64* balls;
  const u64* incidences;
  const u64* ball_at;
  const u64* population_at;
  BallRecord* records;
  SiteIdx* population;
  u64 population_base;
  u32* fault;
};

// Copie des cases (feuilles d'au plus kCase emissions) : noyau leger, sans la feuille J3.
struct CopyKernel {
  LeafView v;
  FillView f;
  struct Shared {};
  MHGP12_HD void operator()(u64 i, Shared&) const {
    if (f.cls[i].cause != 0 || f.balls[i] == 0 || f.balls[i] > kCase) return;
    const u32* leaf_sites = v.sites + v.leaves[i].begin;
    u64 offset = f.population_base + f.population_at[i];
    for (u64 j = 0; j < f.balls[i]; ++j) {
      const Emission<32>& e = f.cases[u64{i} * kCase + j];
      write_emission(e, leaf_sites, f.records + f.ball_at[i] + j, f.population, offset);
      offset += u64{e.p} + e.m;
    }
  }
};

// Rejeu des feuilles qui debordent leur case, par la meme source, aux places exactes du comptage ; le rejeu doit
// rendre les memes comptes (faute sinon).
struct ReplayKernel {
  LeafView v;
  FillView f;
  using Shared = LeafShared<32>;
  MHGP12_HD void operator()(u64 i, Shared& shared) const {
    const LeafClass c = f.cls[i];
    if (c.cause != 0 || f.balls[i] <= kCase) return;
    WriteSink sink{v.sites + v.leaves[i].begin, f.records + f.ball_at[i], f.population,
                   f.population_base + f.population_at[i], f.balls[i], f.incidences[i]};
    LeafCounts counts;
    const u32 status = run_leaf<32, Narrow>(leaf_input(v, i, c), shared, counts, sink);
    if (status != kLeafOk || sink.overflow || sink.balls != f.balls[i] || sink.incidences != f.incidences[i])
      simt::global_or(f.fault, kFaultFill);
  }
};

// Feuilles non resolues rangees dans l'ordre du lot : enregistrement (debut relatif a la liste compacte) et sites.
struct CompactKernel {
  LeafView v;
  const LeafClass* cls;
  const u64* unresolved_at;
  const u64* unresolved_site_at;
  bfs::Leaf* out_leaves;
  u32* out_sites;
  struct Shared {};
  MHGP12_HD void operator()(u64 i, Shared&) const {
    if (cls[i].cause == 0) return;
    const bfs::Leaf& f = v.leaves[i];
    const u64 to = unresolved_site_at[i];
    MHGP12_LANES(kWarp, l) {
      for (u32 j = l; j < f.m; j += kWarp) out_sites[to + j] = v.sites[f.begin + j];
    }
    if (simt::leader()) {
      bfs::Leaf g = f;
      g.begin = to;
      out_leaves[unresolved_at[i]] = g;
    }
  }
};

// Minimum 64 bits sur les voies (meme resultat sur toutes les voies).
MHGP12_HD u64 reduce_min64(Lanes<u64> x) {
  for (u32 d = 16; d > 0; d >>= 1) {
    const Lanes<u64> y = simt::shfl_xor(x, d);
    MHGP12_LANES(kWarp, l) { x[l] = y[l] < x[l] ? y[l] : x[l]; }
  }
  return x[0];
}

struct ReduceView {
  const bfs::Leaf* leaves;
  const LeafClass* cls;
  const u32* status;
  const u64* counts;
  const u64* balls;  // boules de chaque feuille resolue (0 sinon)
  u64 n, n_tiles;
  BatchStats* tiles;
  BatchStats* total;
};

// Totaux d'une tuile de kTileItems feuilles (une feuille au plus 2^42 par compteur : aucune retenue dans une tuile).
struct ReduceTileKernel {
  ReduceView v;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    BatchStats out{};
    Lanes<u64> acc, low;
    for (u32 f = 0; f < kLeafCounters + 6; ++f) {
      MHGP12_LANES(kWarp, l) {
        acc[l] = 0;
        for (u64 i = tile * kTileItems + l; i < (tile + 1) * kTileItems && i < v.n; i += kWarp) {
          if (f < kLeafCounters) {
            acc[l] += v.counts[i * kLeafCounters + f];
          } else {
            const u32 span = v.cls[i].span, tier = f - kLeafCounters;
            const bool hit = tier == 0 ? span <= static_cast<u32>(num::kNarrowSpan)
                             : tier == 1 ? span > static_cast<u32>(num::kNarrowSpan) && span <= static_cast<u32>(num::kMediumSpan)
                             : tier == 2 ? span > static_cast<u32>(num::kMediumSpan)
                             : tier == 3 ? span > static_cast<u32>(kLeafNarrowSpan)
                             : tier == 4 ? v.leaves[i].m > kGraphSites
                                         : v.cls[i].cause == 0 && v.balls[i] > kCase;  // reecrite par Replay
            acc[l] += hit ? 1u : 0u;
          }
        }
      }
      const u64 sum = simt::sum64(acc);
      if (f < kLeafCounters) out.counts[f] = sum;
      else if (f < kLeafCounters + 5) out.tiers[f - kLeafCounters] = sum;
      else out.rewritten = sum;
    }
    Lanes<u32> span;
    MHGP12_LANES(kWarp, l) {
      span[l] = 0;
      low[l] = kNoFailure;
      for (u64 i = tile * kTileItems + l; i < (tile + 1) * kTileItems && i < v.n; i += kWarp) {
        span[l] = v.cls[i].span > span[l] ? v.cls[i].span : span[l];
        const u32 s = v.status[i];
        const u64 key = u64{v.leaves[i].depth} * 4 + (s == kLeafShellCapacity ? 0u : 1u);
        if (s == kLeafShellCapacity || s == kLeafInvariant) low[l] = key < low[l] ? key : low[l];
      }
    }
    out.max_span = simt::reduce_max(span);
    out.failure = reduce_min64(low);
    out.overflow = 0;
    if (simt::leader()) v.tiles[tile] = out;
  }
};

// Totaux du lot (un seul warp) : sommes controlees, maximum, minimum.
struct ReduceTopKernel {
  ReduceView v;
  struct Shared {};
  MHGP12_HD void operator()(u64, Shared&) const {
    if (!simt::leader()) return;
    BatchStats out{};
    out.failure = kNoFailure;
    for (u64 t = 0; t < v.n_tiles; ++t) {
      const BatchStats& s = v.tiles[t];
      for (u32 f = 0; f < kLeafCounters; ++f) {
        out.overflow |= out.counts[f] + s.counts[f] < out.counts[f] ? 1u : 0u;
        out.counts[f] += s.counts[f];
      }
      for (u32 f = 0; f < 5; ++f) out.tiers[f] += s.tiers[f];
      out.rewritten += s.rewritten;
      out.max_span = s.max_span > out.max_span ? s.max_span : out.max_span;
      out.failure = s.failure < out.failure ? s.failure : out.failure;
    }
    *v.total = out;
  }
};

// Lanceurs des noyaux de feuille (device_leaf.cu, executeur CUDA) : rendent le code d'erreur du pilote (cudaError_t) ;
// flux en pointeur opaque. Declares seulement dans une construction CUDA.
#if defined(MHGP12_HAVE_CUDA)
int launch_count_kernel(const CountKernel& kernel, u64 leaves, void* stream) noexcept;
int launch_replay_kernel(const ReplayKernel& kernel, u64 leaves, void* stream) noexcept;
#endif

}  // namespace mhgp12::catalogue_detail::dev
