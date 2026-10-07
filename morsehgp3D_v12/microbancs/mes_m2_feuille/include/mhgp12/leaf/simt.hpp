// Warp en source unique (microbanc MES-M2, hors produit) : le meme texte s'execute sur l'appareil (un warp CUDA reel,
// 32 voies) et sur l'hote (un warp simule de facon deterministe, voies 0..31 jouees l'une apres l'autre).
//
// Discipline d'ecriture (style « super-pas ») :
//   - le code UNIFORME (hors MHGP12_LANES) est execute par les 32 voies sur l'appareil, une fois sur l'hote ; il ne
//     manipule que des valeurs identiques sur toutes les voies (parametres, resultats de collectives, memoire
//     partagee relue apres sync) ;
//   - un bloc MHGP12_LANES(l) { ... } est le corps d'UNE voie : sur l'appareil, la voie courante ; sur l'hote, une
//     boucle l = 0..31. Il ne contient aucune collective, aucun break, aucun return ;
//   - une valeur propre a chaque voie vit dans Lanes<T> (un registre sur l'appareil, 32 cases sur l'hote) ;
//   - les collectives (ballot, shfl, sum, scan) sont appelees en code uniforme par les 32 voies (masque plein) ;
//   - une ecriture en memoire partagee lue par une autre voie est suivie de sync() (__syncwarp sur l'appareil).
// Sur l'hote, la simulation est sequentielle et deterministe ; sur l'appareil, les collectives synchronisent les
// voies et __syncwarp publie les ecritures partagees (regles CUDA des votes, shuffles et synchronisations de warp).
#pragma once

#include <cstdint>
#include <cstring>

#if defined(__CUDACC__)
#define MHGP12_HD __host__ __device__ inline
#define MHGP12_HD_COLD __host__ __device__ __noinline__
#else
#define MHGP12_HD inline
#define MHGP12_HD_COLD inline __attribute__((noinline))
#endif

namespace mhgp12::simt {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
__extension__ using i128 = __int128;  // extension GNU, admise par nvcc (meme type que la v11)

inline constexpr u32 kWarp = 32;
inline constexpr u32 kFull = 0xFFFFFFFFu;

#if defined(__CUDA_ARCH__)
inline constexpr u32 kSlots = 1;
#define MHGP12_LANES(lane) \
  for (::mhgp12::simt::u32 lane = (threadIdx.x & 31u), lane##_once_ = 1u; lane##_once_ != 0u; lane##_once_ = 0u)
#else
inline constexpr u32 kSlots = kWarp;
#define MHGP12_LANES(lane) for (::mhgp12::simt::u32 lane = 0; lane < ::mhgp12::simt::kWarp; ++lane)
#endif

MHGP12_HD u32 slot(u32 lane) {
#if defined(__CUDA_ARCH__)
  (void)lane;
  return 0;
#else
  return lane;
#endif
}

// Valeur propre a chaque voie.
template <class T>
struct Lanes {
  T v[kSlots];
  MHGP12_HD T& operator[](u32 lane) { return v[slot(lane)]; }
  MHGP12_HD const T& operator[](u32 lane) const { return v[slot(lane)]; }
};

// Vrai pour la voie 0 en code uniforme (ecritures partagees uniques) ; toujours vrai sur l'hote.
MHGP12_HD bool leader() {
#if defined(__CUDA_ARCH__)
  return (threadIdx.x & 31u) == 0u;
#else
  return true;
#endif
}

MHGP12_HD void sync() {
#if defined(__CUDA_ARCH__)
  __syncwarp(kFull);
#endif
}

MHGP12_HD u32 popc(u32 x) {
#if defined(__CUDA_ARCH__)
  return static_cast<u32>(__popc(x));
#else
  return static_cast<u32>(__builtin_popcount(x));
#endif
}
MHGP12_HD u32 ctz(u32 x) {  // x != 0
#if defined(__CUDA_ARCH__)
  return static_cast<u32>(__ffs(static_cast<int>(x)) - 1);
#else
  return static_cast<u32>(__builtin_ctz(x));
#endif
}
// Bits strictement au-dessus de b (b < 32), sans decalage de 32.
MHGP12_HD u32 above(u32 b) { return (kFull << b) << 1; }
// Bits strictement au-dessous de b (b <= 31).
MHGP12_HD u32 below(u32 b) { return (1u << b) - 1u; }
// Position du n-ieme bit a un (n >= 1) ; 32 s'il n'existe pas.
MHGP12_HD u32 select_nth(u32 mask, u32 n) {
  for (u32 i = 1; i < n && mask != 0; ++i) mask &= mask - 1;
  return mask != 0 ? ctz(mask) : 32u;
}

MHGP12_HD u32 ballot(const Lanes<bool>& p) {
#if defined(__CUDA_ARCH__)
  return __ballot_sync(kFull, p.v[0]);
#else
  u32 m = 0;
  for (u32 l = 0; l < kWarp; ++l) m |= static_cast<u32>(p.v[l]) << l;
  return m;
#endif
}

// Valeur de la voie src (types trivialement copiables de taille multiple de 4 octets).
template <class T>
MHGP12_HD T shfl(const Lanes<T>& v, u32 src) {
#if defined(__CUDA_ARCH__)
  static_assert(sizeof(T) % 4 == 0, "shfl : taille multiple de 4");
  constexpr unsigned kWords = sizeof(T) / 4;
  u32 words[kWords];
  memcpy(words, &v.v[0], sizeof(T));
#pragma unroll
  for (unsigned i = 0; i < kWords; ++i) words[i] = __shfl_sync(kFull, words[i], static_cast<int>(src));
  T out;
  memcpy(&out, words, sizeof(T));
  return out;
#else
  return v.v[src];
#endif
}

// Somme sur les 32 voies (u32, sans debordement par les bornes des compteurs de feuille).
MHGP12_HD u32 sum(const Lanes<u32>& v) {
#if defined(__CUDA_ARCH__)
  return __reduce_add_sync(kFull, v.v[0]);
#else
  u32 s = 0;
  for (u32 l = 0; l < kWarp; ++l) s += v.v[l];
  return s;
#endif
}

// Prefixe exclusif sur les voies ; total rendu dans total (identique sur toutes les voies).
MHGP12_HD Lanes<u32> exclusive_scan(const Lanes<u32>& v, u32& total) {
  Lanes<u32> out;
#if defined(__CUDA_ARCH__)
  const u32 lane = threadIdx.x & 31u;
  u32 x = v.v[0];
#pragma unroll
  for (u32 d = 1; d < 32; d <<= 1) {
    const u32 y = __shfl_up_sync(kFull, x, d);
    if (lane >= d) x += y;
  }
  out.v[0] = x - v.v[0];
  total = __shfl_sync(kFull, x, 31);
#else
  u32 s = 0;
  for (u32 l = 0; l < kWarp; ++l) {
    out.v[l] = s;
    s += v.v[l];
  }
  total = s;
#endif
  return out;
}

// OU atomique en memoire partagee (plusieurs voies peuvent viser le meme mot dans un meme super-pas).
MHGP12_HD void atomic_or(u32* word, u32 bits) {
#if defined(__CUDA_ARCH__)
  atomicOr(word, bits);
#else
  *word |= bits;
#endif
}

}  // namespace mhgp12::simt
