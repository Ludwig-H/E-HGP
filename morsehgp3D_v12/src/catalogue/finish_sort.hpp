// Fin d'etage, source unique hote et appareil : sommes prefixes par tuiles et tri par base stable, en noyaux de warps
// (memes conventions que les noyaux du parcours, traversal_kernels.hpp : un warp par indice, memoire partagee propre au
// warp, aucun atomique global, sorties independantes de l'ordre d'execution des warps). Joues par l'executeur Pool de
// la voie CPU (exec_host.hpp) et par l'executeur CUDA de la voie appareil (device_cuda.cu).
//
// Somme prefixe exclusive de n valeurs u64 : tuiles de kTileItems valeurs (32 consecutives par voie), sommes des
// tuiles, prefixe des tuiles par un seul warp, puis ecriture (en place permise).
// Tri par base LSD stable de n paires (cle, valeur u32) : passes d'un octet, histogramme de chaque tuile de kTileItems
// paires (memoire partagee), prefixe de chaque colonne d'octet sur les tuiles, puis ecriture stable par tuile (paquets
// de 32 dans l'ordre, places prises par simt::claim). Les octets communs a toutes les cles (ET = OU) sont sautes.
#pragma once

#include "catalogue/simt.hpp"

namespace mhgp12::catalogue_detail::fin {

using simt::kWarp;
using simt::Lanes;

inline constexpr u32 kTileItems = 1024;
inline constexpr u32 kBins = 256;

struct Key2 {
  u64 lo, hi;
};
MHGP12_HD u32 key_byte(u64 k, u32 d) { return static_cast<u32>((k >> (8 * d)) & 255u); }
MHGP12_HD u32 key_byte(const Key2& k, u32 d) { return d < 8 ? key_byte(k.lo, d) : key_byte(k.hi, d - 8); }
MHGP12_HD u64 key_and(u64 a, u64 b) { return a & b; }
MHGP12_HD u64 key_or(u64 a, u64 b) { return a | b; }
MHGP12_HD Key2 key_and(const Key2& a, const Key2& b) { return Key2{a.lo & b.lo, a.hi & b.hi}; }
MHGP12_HD Key2 key_or(const Key2& a, const Key2& b) { return Key2{a.lo | b.lo, a.hi | b.hi}; }

MHGP12_HD u64 tiles_of(u64 n) { return (n + kTileItems - 1) / kTileItems; }
MHGP12_HD u32 width_of(u64 n) {  // plus petit b tel que n < 2^b, au moins 1
  u32 b = 1;
  while (b < 64 && (n >> b) != 0) ++b;
  return b;
}

// ------------------------------------------------------------------------------------------- sommes prefixes
struct ScanView {
  const u64* in;
  u64* out;
  u64 n, n_tiles;
  u64* tile_sum;  // sommes des tuiles, puis leurs bases
  u64* total;     // une case
};

MHGP12_HD u64 lane_part(const ScanView& v, u64 tile, u32 l) {
  u64 s = 0;
  const u64 first = tile * kTileItems + u64{l} * 32;
  for (u64 i = first; i < first + 32 && i < v.n; ++i) s += v.in[i];
  return s;
}

struct ScanTileKernel {
  ScanView v;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    Lanes<u64> part;
    MHGP12_LANES(kWarp, l) { part[l] = lane_part(v, tile, l); }
    const u64 sum = simt::sum64(part);
    if (simt::leader()) v.tile_sum[tile] = sum;
  }
};

struct ScanTopKernel {  // un seul warp
  ScanView v;
  struct Shared {};
  MHGP12_HD void operator()(u64, Shared&) const {
    u64 running = 0;
    for (u64 base = 0; base < v.n_tiles; base += kWarp) {
      Lanes<u64> x;
      MHGP12_LANES(kWarp, l) { x[l] = base + l < v.n_tiles ? v.tile_sum[base + l] : 0; }
      u64 total = 0;
      const Lanes<u64> off = simt::exclusive_scan64(x, total);
      MHGP12_LANES(kWarp, l) {
        if (base + l < v.n_tiles) v.tile_sum[base + l] = running + off[l];
      }
      running += total;
    }
    if (simt::leader()) *v.total = running;
  }
};

struct ScanApplyKernel {
  ScanView v;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    Lanes<u64> part;
    MHGP12_LANES(kWarp, l) { part[l] = lane_part(v, tile, l); }
    u64 total = 0;
    const Lanes<u64> off = simt::exclusive_scan64(part, total);
    const u64 base = v.tile_sum[tile];
    MHGP12_LANES(kWarp, l) {
      u64 at = base + off[l];
      const u64 first = tile * kTileItems + u64{l} * 32;
      for (u64 i = first; i < first + 32 && i < v.n; ++i) {
        const u64 x = v.in[i];
        v.out[i] = at;
        at += x;
      }
    }
  }
};

// ---------------------------------------------------------------------------- bornes des cles (ET et OU)
template <class Key>
struct BoundsView {
  const Key* keys;
  u64 n, n_tiles;
  Key* tile_and;
  Key* tile_or;
  Key* result;  // deux cases : ET puis OU
};

template <class Key>
MHGP12_HD void reduce_bounds(Lanes<Key>& a, Lanes<Key>& o) {
  for (u32 d = 16; d > 0; d >>= 1) {
    const Lanes<Key> pa = simt::shfl_xor(a, d), po = simt::shfl_xor(o, d);
    MHGP12_LANES(kWarp, l) {
      a[l] = key_and(a[l], pa[l]);
      o[l] = key_or(o[l], po[l]);
    }
  }
}

template <class Key>
struct BoundsTileKernel {
  BoundsView<Key> v;
  struct Shared {};
  MHGP12_HD void operator()(u64 tile, Shared&) const {
    Lanes<Key> a, o;
    const u64 first = tile * kTileItems;
    MHGP12_LANES(kWarp, l) {
      a[l] = v.keys[first < v.n ? first : 0];  // la tuile n'est jamais vide
      o[l] = a[l];
      for (u64 i = first + l; i < first + kTileItems && i < v.n; i += kWarp) {
        a[l] = key_and(a[l], v.keys[i]);
        o[l] = key_or(o[l], v.keys[i]);
      }
    }
    reduce_bounds(a, o);
    if (simt::leader()) {
      v.tile_and[tile] = a[0];
      v.tile_or[tile] = o[0];
    }
  }
};

template <class Key>
struct BoundsTopKernel {  // un seul warp
  BoundsView<Key> v;
  struct Shared {};
  MHGP12_HD void operator()(u64, Shared&) const {
    Lanes<Key> a, o;
    MHGP12_LANES(kWarp, l) {
      a[l] = v.tile_and[0];
      o[l] = v.tile_or[0];
      for (u64 t = l; t < v.n_tiles; t += kWarp) {
        a[l] = key_and(a[l], v.tile_and[t]);
        o[l] = key_or(o[l], v.tile_or[t]);
      }
    }
    reduce_bounds(a, o);
    if (simt::leader()) {
      v.result[0] = a[0];
      v.result[1] = o[0];
    }
  }
};

// ------------------------------------------------------------------------------------------- tri par base
template <class Key>
struct RadixView {
  const Key* keys_in;
  Key* keys_out;
  const u32* vals_in;
  u32* vals_out;
  u64 n, n_tiles;
  u32 byte;
  u32* tile_hist;  // kBins colonnes de n_tiles comptes (colonne b : comptes de l'octet b), puis leurs prefixes
  u32* bin_total;  // kBins
};

template <class Key>
struct RadixHistKernel {
  RadixView<Key> v;
  struct Shared {
    u32 hist[kBins];
  };
  MHGP12_HD void operator()(u64 tile, Shared& sh) const {
    MHGP12_LANES(kWarp, l) {
      for (u32 b = l; b < kBins; b += kWarp) sh.hist[b] = 0;
    }
    simt::sync();
    const u64 first = tile * kTileItems;
    for (u32 c = 0; c < kTileItems / kWarp; ++c) {
      MHGP12_LANES(kWarp, l) {
        const u64 i = first + u64{c} * kWarp + l;
        if (i < v.n) simt::atomic_add(&sh.hist[key_byte(v.keys_in[i], v.byte)], 1u);
      }
    }
    simt::sync();
    MHGP12_LANES(kWarp, l) {
      for (u32 b = l; b < kBins; b += kWarp) v.tile_hist[u64{b} * v.n_tiles + tile] = sh.hist[b];
    }
  }
};

template <class Key>
struct RadixColumnKernel {  // un warp par octet b
  RadixView<Key> v;
  struct Shared {};
  MHGP12_HD void operator()(u64 b, Shared&) const {
    u32* column = v.tile_hist + b * v.n_tiles;
    u32 running = 0;
    for (u64 base = 0; base < v.n_tiles; base += kWarp) {
      Lanes<u32> x;
      MHGP12_LANES(kWarp, l) { x[l] = base + l < v.n_tiles ? column[base + l] : 0u; }
      u32 total = 0;
      const Lanes<u32> off = simt::exclusive_scan<kWarp>(x, total);
      MHGP12_LANES(kWarp, l) {
        if (base + l < v.n_tiles) column[base + l] = running + off[l];
      }
      running += total;
    }
    if (simt::leader()) v.bin_total[b] = running;
  }
};

template <class Key>
struct RadixScatterKernel {
  RadixView<Key> v;
  struct Shared {
    u32 next[kBins + 1];  // case kBins : voies inactives
  };
  // Base de chaque octet : prefixe exclusif des totaux des octets (recalcule par chaque warp).
  MHGP12_HD void bases(u64 tile, Shared& sh) const {
    u32 running = 0;
    for (u32 base = 0; base < kBins; base += kWarp) {
      Lanes<u32> x;
      MHGP12_LANES(kWarp, l) { x[l] = v.bin_total[base + l]; }
      u32 total = 0;
      const Lanes<u32> off = simt::exclusive_scan<kWarp>(x, total);
      MHGP12_LANES(kWarp, l) { sh.next[base + l] = running + off[l] + v.tile_hist[u64{base + l} * v.n_tiles + tile]; }
      running += total;
    }
    if (simt::leader()) sh.next[kBins] = 0;
    simt::sync();
  }
  MHGP12_HD void operator()(u64 tile, Shared& sh) const {
    bases(tile, sh);
    const u64 first = tile * kTileItems;
    for (u32 c = 0; c < kTileItems / kWarp; ++c) {
      Lanes<u32> bin;
      MHGP12_LANES(kWarp, l) {
        const u64 i = first + u64{c} * kWarp + l;
        bin[l] = i < v.n ? key_byte(v.keys_in[i], v.byte) : kBins;
      }
      const Lanes<u32> at = simt::claim(sh.next, bin);
      MHGP12_LANES(kWarp, l) {
        const u64 i = first + u64{c} * kWarp + l;
        if (i < v.n) {
          v.keys_out[at[l]] = v.keys_in[i];
          v.vals_out[at[l]] = v.vals_in[i];
        }
      }
    }
  }
};

// Tableaux d'un tri : cles et valeurs en double tampon, histogrammes, bornes.
template <class B, class Key>
struct RadixArrays {
  typename B::template Array<Key> keys[2];
  typename B::template Array<u32> vals[2];
  typename B::template Array<u32> tile_hist, bin_total;
  typename B::template Array<Key> tile_and, tile_or, result;
};

template <class B, class Key>
Outcome radix_reserve(B& b, RadixArrays<B, Key>& a, u64 n) noexcept {
  const u64 tiles = tiles_of(n) == 0 ? 1 : tiles_of(n);
  for (int i = 0; i < 2; ++i) {
    MHGP12_TRY(b.ensure(a.keys[i], n));
    MHGP12_TRY(b.ensure(a.vals[i], n));
  }
  MHGP12_TRY(b.ensure(a.tile_hist, u64{kBins} * tiles));
  MHGP12_TRY(b.ensure(a.bin_total, kBins));
  MHGP12_TRY(b.ensure(a.tile_and, tiles));
  MHGP12_TRY(b.ensure(a.tile_or, tiles));
  return b.ensure(a.result, 2);
}

// Tri stable de (keys[0], vals[0]) sur `bytes` octets de cle ; rend l'indice du tampon qui porte le resultat.
template <class B, class Key>
Result<int> radix_sort(B& b, RadixArrays<B, Key>& a, u64 n, u32 bytes) noexcept {
  if (n <= 1) return 0;
  const u64 tiles = tiles_of(n);
  const BoundsView<Key> bv{a.keys[0].data(), n, tiles, a.tile_and.data(), a.tile_or.data(), a.result.data()};
  MHGP12_TRY(b.launch(BoundsTileKernel<Key>{bv}, tiles));
  MHGP12_TRY(b.launch(BoundsTopKernel<Key>{bv}, 1));
  const auto lower = b.read(a.result, 0), upper = b.read(a.result, 1);
  if (!lower.ok()) return lower.outcome();
  if (!upper.ok()) return upper.outcome();
  int cur = 0;
  for (u32 d = 0; d < bytes; ++d) {
    if (key_byte(lower.value(), d) == key_byte(upper.value(), d)) continue;  // octet commun : passe identite
    const RadixView<Key> v{a.keys[cur].data(),     a.keys[cur ^ 1].data(), a.vals[cur].data(), a.vals[cur ^ 1].data(),
                           n,  tiles, d, a.tile_hist.data(), a.bin_total.data()};
    MHGP12_TRY(b.launch(RadixHistKernel<Key>{v}, tiles));
    MHGP12_TRY(b.launch(RadixColumnKernel<Key>{v}, kBins));
    MHGP12_TRY(b.launch(RadixScatterKernel<Key>{v}, tiles));
    cur ^= 1;
  }
  return cur;
}

// Somme prefixe exclusive de in[0..n) dans out (en place permise) ; total rendu.
template <class B>
struct ScanArrays {
  typename B::template Array<u64> tile_sum, total;
};

template <class B>
Result<u64> exclusive_scan(B& b, ScanArrays<B>& a, const u64* in, u64* out, u64 n) noexcept {
  const u64 tiles = tiles_of(n) == 0 ? 1 : tiles_of(n);
  MHGP12_TRY(b.ensure(a.tile_sum, tiles));
  MHGP12_TRY(b.ensure(a.total, 1));
  const ScanView v{in, out, n, tiles_of(n), a.tile_sum.data(), a.total.data()};
  MHGP12_TRY(b.launch(ScanTileKernel{v}, tiles_of(n)));
  MHGP12_TRY(b.launch(ScanTopKernel{v}, 1));
  MHGP12_TRY(b.launch(ScanApplyKernel{v}, tiles_of(n)));
  return b.read(a.total, 0);
}

}  // namespace mhgp12::catalogue_detail::fin
