// Collectives de warp supplementaires du microbanc MES-M5 (hors produit), au-dessus du warp en source unique de MES-M2
// (mhgp12/leaf/simt.hpp, inchange) : meme discipline « super-pas » (code uniforme, blocs MHGP12_LANES sans collective,
// collectives en code uniforme a masque plein). Sur l'hote, le warp simule rend des resultats deterministes ; sur
// l'appareil, ce sont les intrinseques CUDA de warp.
#pragma once

#include "mhgp12/leaf/simt.hpp"

namespace mhgp12::traversal::warp {

using simt::i128;
using simt::i32;
using simt::i64;
using simt::kFull;
using simt::kWarp;
using simt::Lanes;
using simt::u32;
using simt::u64;
using simt::u8;
__extension__ using u128 = unsigned __int128;  // extension GNU admise par nvcc (comme simt::i128)

// Valeur de la voie src[l] pour chaque voie l (source propre a chaque voie).
template <class T>
MHGP12_HD Lanes<T> shfl_lane(const Lanes<T>& v, const Lanes<u32>& src) {
  Lanes<T> out;
#if defined(__CUDA_ARCH__)
  static_assert(sizeof(T) % 4 == 0, "shfl_lane : taille multiple de 4");
  constexpr unsigned kWords = sizeof(T) / 4;
  u32 words[kWords];
  memcpy(words, &v.v[0], sizeof(T));
#pragma unroll
  for (unsigned i = 0; i < kWords; ++i) words[i] = __shfl_sync(kFull, words[i], static_cast<int>(src.v[0] & 31u));
  memcpy(&out.v[0], words, sizeof(T));
#else
  for (u32 l = 0; l < kWarp; ++l) out.v[l] = v.v[src.v[l] & 31u];
#endif
  return out;
}

// Valeur de la voie l ^ mask (mask uniforme).
template <class T>
MHGP12_HD Lanes<T> shfl_xor(const Lanes<T>& v, u32 mask) {
  Lanes<T> out;
#if defined(__CUDA_ARCH__)
  static_assert(sizeof(T) % 4 == 0, "shfl_xor : taille multiple de 4");
  constexpr unsigned kWords = sizeof(T) / 4;
  u32 words[kWords];
  memcpy(words, &v.v[0], sizeof(T));
#pragma unroll
  for (unsigned i = 0; i < kWords; ++i) words[i] = __shfl_xor_sync(kFull, words[i], static_cast<int>(mask));
  memcpy(&out.v[0], words, sizeof(T));
#else
  for (u32 l = 0; l < kWarp; ++l) out.v[l] = v.v[(l ^ mask) & 31u];
#endif
  return out;
}

MHGP12_HD u32 reduce_min(const Lanes<u32>& v) {
#if defined(__CUDA_ARCH__)
  return __reduce_min_sync(kFull, v.v[0]);
#else
  u32 m = v.v[0];
  for (u32 l = 1; l < kWarp; ++l) m = v.v[l] < m ? v.v[l] : m;
  return m;
#endif
}

MHGP12_HD u32 reduce_max(const Lanes<u32>& v) {
#if defined(__CUDA_ARCH__)
  return __reduce_max_sync(kFull, v.v[0]);
#else
  u32 m = v.v[0];
  for (u32 l = 1; l < kWarp; ++l) m = v.v[l] > m ? v.v[l] : m;
  return m;
#endif
}

// Somme 64 bits sur les 32 voies (papillon ; meme resultat sur toutes les voies).
MHGP12_HD u64 sum64(const Lanes<u64>& v) {
#if defined(__CUDA_ARCH__)
  u64 x = v.v[0];
#pragma unroll
  for (u32 d = 16; d > 0; d >>= 1) x += __shfl_xor_sync(kFull, x, static_cast<int>(d));
  return x;
#else
  u64 s = 0;
  for (u32 l = 0; l < kWarp; ++l) s += v.v[l];
  return s;
#endif
}

// Prefixe exclusif 64 bits sur les voies ; total rendu dans total.
MHGP12_HD Lanes<u64> exclusive_scan64(const Lanes<u64>& v, u64& total) {
  Lanes<u64> out;
#if defined(__CUDA_ARCH__)
  const u32 lane = threadIdx.x & 31u;
  u64 x = v.v[0];
#pragma unroll
  for (u32 d = 1; d < 32; d <<= 1) {
    const u64 y = __shfl_up_sync(kFull, x, d);
    if (lane >= d) x += y;
  }
  out.v[0] = x - v.v[0];
  total = __shfl_sync(kFull, x, 31);
#else
  u64 s = 0;
  for (u32 l = 0; l < kWarp; ++l) {
    out.v[l] = s;
    s += v.v[l];
  }
  total = s;
#endif
  return out;
}

}  // namespace mhgp12::traversal::warp
