// Warp en source unique du catalogue : le meme texte s'execute sur l'appareil (un warp CUDA reel de 32 voies) et sur
// l'hote (un warp simule de facon deterministe, voies jouees l'une apres l'autre). Port explicite de
// microbancs/mes_m2_feuille/include/mhgp12/leaf/simt.hpp (MES-M2) et des collectives ajoutees par
// microbancs/mes_m5_parcours/include/mhgp12/traversal/warp.hpp (MES-M5) ; empreintes dans src/catalogue/source_pins.json.
//
// Ce qui change par rapport aux microbancs : la largeur du warp est un parametre N. N = 32 est le warp de l'appareil
// et celui des feuilles d'au plus 32 sites ; N = 256 est un warp VIRTUEL de l'hote seulement, qui joue la meme source
// J3 sur les feuilles de 33 a 256 sites (capacite max_leaf de la v11), une voie par site comme a N = 32 : meme
// structure avec des masques plus larges, jamais un second algorithme (docs/CONTRAT_CATALOGUE.md, paragraphe 2).
// Le masque d'un warp de N voies est un u32 (N = 32) ou un Bits<4> (N = 256).
//
// Discipline d'ecriture (style << super-pas >> des microbancs) :
//   - le code UNIFORME (hors MHGP12_LANES) est execute par toutes les voies sur l'appareil, une fois sur l'hote ; il ne
//     lit que des valeurs identiques sur les voies (parametres, collectives, memoire partagee relue apres sync) ;
//   - MHGP12_LANES(N, l) { ... } est le corps d'UNE voie : sur l'appareil la voie courante, sur l'hote une boucle
//     l = 0..N-1 ; ni collective, ni break, ni return dedans ;
//   - Lanes<T, N> porte une valeur par voie ; les collectives sont appelees en code uniforme a masque plein ;
//   - une ecriture partagee lue par une autre voie est suivie de sync() (__syncwarp sur l'appareil).
#pragma once

#include <atomic>
#include <cstring>

#include "core/core.hpp"

#if defined(__CUDACC__)
#define MHGP12_HD __host__ __device__ inline
#define MHGP12_HD_COLD __host__ __device__ __noinline__
#else
#define MHGP12_HD inline
#define MHGP12_HD_COLD inline __attribute__((noinline))
#endif

// Mode du warp : MHGP12_SIMT_WARP vaut 1 quand le code d'appareil joue un vrai warp de 32 voies ; 0 sur l'hote, et sur
// l'appareil quand l'unite de traduction definit MHGP12_SIMT_SERIAL avant cet en-tete : un fil joue alors seul le warp
// entier, voies en serie (mutant << un fil par feuille >> de CONTRAT_CATALOGUE.md, paragraphe 6.6 ; le produit ne le
// definit jamais). Les intrinseques materielles (popc, ctz, atomiques globaux) restent celles de l'appareil.
#if defined(__CUDA_ARCH__) && !defined(MHGP12_SIMT_SERIAL)
#define MHGP12_SIMT_WARP 1
#else
#define MHGP12_SIMT_WARP 0
#endif

#if MHGP12_SIMT_WARP
#define MHGP12_LANES(width, lane) \
  for (::mhgp12::u32 lane = (threadIdx.x & 31u), lane##_once_ = 1u; lane##_once_ != 0u; lane##_once_ = 0u)
#else
#define MHGP12_LANES(width, lane) for (::mhgp12::u32 lane = 0; lane < (width); ++lane)
#endif

namespace mhgp12::simt {

inline constexpr u32 kWarp = 32;
inline constexpr u32 kFull = 0xFFFFFFFFu;

// Nombre de cases d'une valeur par voie : une sur l'appareil (warp de 32 voies seulement), N sur l'hote.
#if MHGP12_SIMT_WARP
template <u32 N>
inline constexpr u32 kSlots = 1;
#else
template <u32 N>
inline constexpr u32 kSlots = N;
#endif

MHGP12_HD u32 slot(u32 lane) {
#if MHGP12_SIMT_WARP
  (void)lane;
  return 0;
#else
  return lane;
#endif
}

template <class T, u32 N = kWarp>
struct Lanes {
  T v[kSlots<N>];
  MHGP12_HD T& operator[](u32 lane) { return v[slot(lane)]; }
  MHGP12_HD const T& operator[](u32 lane) const { return v[slot(lane)]; }
};

// ---------------------------------------------------------------------------------------------------- masques
// Masque large de W mots (hote seulement) : bit i au mot i/64, position i%64.
template <u32 W>
struct Bits {
  u64 w[W];
  friend constexpr Bits operator&(const Bits& a, const Bits& b) noexcept {
    Bits r{};
    for (u32 i = 0; i < W; ++i) r.w[i] = a.w[i] & b.w[i];
    return r;
  }
  friend constexpr Bits operator|(const Bits& a, const Bits& b) noexcept {
    Bits r{};
    for (u32 i = 0; i < W; ++i) r.w[i] = a.w[i] | b.w[i];
    return r;
  }
  friend constexpr bool operator==(const Bits& a, const Bits& b) noexcept {
    for (u32 i = 0; i < W; ++i)
      if (a.w[i] != b.w[i]) return false;
    return true;
  }
  constexpr Bits& operator|=(const Bits& b) noexcept { return *this = *this | b; }
  constexpr Bits& operator&=(const Bits& b) noexcept { return *this = *this & b; }
};

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
MHGP12_HD bool any(u32 x) { return x != 0; }
MHGP12_HD bool test(u32 x, u32 b) { return ((x >> b) & 1u) != 0; }
MHGP12_HD u32 clear_lowest(u32 x) { return x & (x - 1u); }

template <u32 W>
inline u32 popc(const Bits<W>& x) {
  u32 n = 0;
  for (u32 i = 0; i < W; ++i) n += static_cast<u32>(__builtin_popcountll(x.w[i]));
  return n;
}
template <u32 W>
inline bool any(const Bits<W>& x) {
  for (u32 i = 0; i < W; ++i)
    if (x.w[i] != 0) return true;
  return false;
}
template <u32 W>
inline u32 ctz(const Bits<W>& x) {  // x non vide
  for (u32 i = 0; i < W; ++i)
    if (x.w[i] != 0) return 64 * i + static_cast<u32>(__builtin_ctzll(x.w[i]));
  return 64 * W;
}
template <u32 W>
inline bool test(const Bits<W>& x, u32 b) {
  return ((x.w[b / 64] >> (b % 64)) & 1u) != 0;
}
template <u32 W>
inline Bits<W> clear_lowest(Bits<W> x) {
  for (u32 i = 0; i < W; ++i)
    if (x.w[i] != 0) {
      x.w[i] &= x.w[i] - 1;
      break;
    }
  return x;
}

// Operations dependant de la largeur : un mot (N = 32) ou quatre (N = 256).
template <u32 N>
struct Width;

template <>
struct Width<32> {
  using Mask = u32;
  static constexpr u32 kIndexBits = 5;  // indices de site dans un item de file
  MHGP12_HD static Mask bit(u32 b) { return 1u << b; }
  MHGP12_HD static Mask above(u32 b) { return (kFull << b) << 1; }  // bits strictement au-dessus de b (b < 32)
  MHGP12_HD static Mask empty() { return 0u; }
};

template <>
struct Width<256> {
  static constexpr u32 kWords = 4;
  using Mask = Bits<kWords>;
  static constexpr u32 kIndexBits = 8;
  static Mask bit(u32 b) {
    Mask m{};
    m.w[b / 64] = u64{1} << (b % 64);
    return m;
  }
  static Mask above(u32 b) {
    Mask m{};
    for (u32 i = 0; i < kWords; ++i) {
      const u32 lo = 64 * i;
      if (b + 1 <= lo) m.w[i] = ~u64{0};
      else if (b + 1 < lo + 64) m.w[i] = ~u64{0} << (b + 1 - lo);
    }
    return m;
  }
  static Mask empty() { return Mask{}; }
};

// Position du n-ieme bit a un (n >= 1) ; N s'il n'existe pas.
template <u32 N, class M>
MHGP12_HD u32 select_nth(M mask, u32 n) {
  for (u32 i = 1; i < n && any(mask); ++i) mask = clear_lowest(mask);
  return any(mask) ? ctz(mask) : N;
}

// ------------------------------------------------------------------------------------------------- collectives
// Vrai pour la voie 0 en code uniforme (ecritures partagees uniques) ; toujours vrai sur l'hote.
MHGP12_HD bool leader() {
#if MHGP12_SIMT_WARP
  return (threadIdx.x & 31u) == 0u;
#else
  return true;
#endif
}

MHGP12_HD void sync() {
#if MHGP12_SIMT_WARP
  __syncwarp(kFull);
#endif
}

template <u32 N>
MHGP12_HD typename Width<N>::Mask ballot(const Lanes<bool, N>& p) {
#if MHGP12_SIMT_WARP
  return __ballot_sync(kFull, p.v[0]);
#else
  auto m = Width<N>::empty();
  for (u32 l = 0; l < N; ++l)
    if (p.v[l]) m = m | Width<N>::bit(l);
  return m;
#endif
}

// Valeur de la voie src (types trivialement copiables de taille multiple de 4 octets).
template <class T, u32 N>
MHGP12_HD T shfl(const Lanes<T, N>& v, u32 src) {
#if MHGP12_SIMT_WARP
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

// Somme sur les voies, en u64.
template <u32 N>
MHGP12_HD u64 sum(const Lanes<u32, N>& v) {
#if MHGP12_SIMT_WARP
  return __reduce_add_sync(kFull, v.v[0]);
#else
  u64 s = 0;
  for (u32 l = 0; l < N; ++l) s += v.v[l];
  return s;
#endif
}

// Prefixe exclusif sur les voies ; total rendu dans total (identique sur toutes les voies).
template <u32 N>
MHGP12_HD Lanes<u32, N> exclusive_scan(const Lanes<u32, N>& v, u32& total) {
  Lanes<u32, N> out;
#if MHGP12_SIMT_WARP
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
  for (u32 l = 0; l < N; ++l) {
    out.v[l] = s;
    s += v.v[l];
  }
  total = s;
#endif
  return out;
}

// OU atomique en memoire partagee (plusieurs voies peuvent viser le meme mot dans un meme super-pas).
MHGP12_HD void atomic_or(u32* word, u32 bits) {
#if MHGP12_SIMT_WARP
  atomicOr(word, bits);
#else
  *word |= bits;
#endif
}
template <u32 W>
inline void atomic_or(Bits<W>* word, const Bits<W>& bits) {
  *word |= bits;
}

// Addition atomique en memoire partagee d'UN warp (histogrammes du tri par base) : plusieurs voies peuvent viser le
// meme mot dans un meme super-pas ; l'hote joue le warp sur un seul fil.
MHGP12_HD void atomic_add(u32* word, u32 value) {
#if MHGP12_SIMT_WARP
  atomicAdd(word, value);
#else
  *word += value;
#endif
}

// OU atomique en memoire GLOBALE (drapeaux de faute partages par tous les warps d'un lancement) : sur l'hote, les warps
// d'un noyau sont repartis sur les fils du Pool, d'ou l'atomique de la bibliotheque standard. Le resultat ne depend
// pas de l'ordre (OU).
MHGP12_HD void global_or(u32* word, u32 bits) {
#if defined(__CUDA_ARCH__)
  atomicOr(word, bits);
#else
  std::atomic_ref<u32>(*word).fetch_or(bits, std::memory_order_relaxed);
#endif
}

// Places stables dans des files par valeur (tri par base) : la voie l de valeur b = v[l] recoit next[b] plus le nombre
// de voies k < l de meme valeur, puis next[b] augmente du nombre de voies de valeur b. next est en memoire partagee du
// warp. Sur l'appareil : groupes de voies egales (__match_any_sync), rang dans le groupe, mise a jour par la premiere
// voie du groupe ; sur l'hote : les voies dans l'ordre, une place chacune. Meme resultat.
MHGP12_HD Lanes<u32> claim(u32* next, const Lanes<u32>& v) {
  Lanes<u32> out;
#if MHGP12_SIMT_WARP
  const u32 lane = threadIdx.x & 31u;
  const u32 peers = __match_any_sync(kFull, v.v[0]);
  const u32 below = peers & ((1u << lane) - 1u);
  out.v[0] = next[v.v[0]] + static_cast<u32>(__popc(below));
  __syncwarp(kFull);
  if (below == 0) next[v.v[0]] += static_cast<u32>(__popc(peers));
  __syncwarp(kFull);
#else
  for (u32 l = 0; l < kWarp; ++l) out.v[l] = next[v.v[l]]++;
#endif
  return out;
}

// --------------------------------------------- collectives du parcours (warp de 32 voies, port de warp.hpp de MES-M5)
// Valeur de la voie l ^ mask (mask uniforme).
template <class T>
MHGP12_HD Lanes<T> shfl_xor(const Lanes<T>& v, u32 mask) {
  Lanes<T> out;
#if MHGP12_SIMT_WARP
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
#if MHGP12_SIMT_WARP
  return __reduce_min_sync(kFull, v.v[0]);
#else
  u32 m = v.v[0];
  for (u32 l = 1; l < kWarp; ++l) m = v.v[l] < m ? v.v[l] : m;
  return m;
#endif
}

MHGP12_HD u32 reduce_max(const Lanes<u32>& v) {
#if MHGP12_SIMT_WARP
  return __reduce_max_sync(kFull, v.v[0]);
#else
  u32 m = v.v[0];
  for (u32 l = 1; l < kWarp; ++l) m = v.v[l] > m ? v.v[l] : m;
  return m;
#endif
}

// Somme 64 bits sur les 32 voies (meme resultat sur toutes les voies).
MHGP12_HD u64 sum64(const Lanes<u64>& v) {
#if MHGP12_SIMT_WARP
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
#if MHGP12_SIMT_WARP
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

}  // namespace mhgp12::simt
