// Test G1 de dominance : boucle de reference et masque AVX2 aux memes decisions (g1.hpp).
#include "catalogue/g1.hpp"

#include <algorithm>
#include <bit>

#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#include <immintrin.h>
#define MHGP11_G1_AVX2 1
#endif

namespace mhgp11::catalogue_detail {
namespace {

std::array<i64, 3> site_coordinates(const Cloud& cloud, SiteIdx site) noexcept {
  const u32 i = idx(site);
  return {cloud.x()[i], cloud.y()[i], cloud.z()[i]};
}

// Arret de la boucle de reference deduit du masque de tous les temoins : moins de kmax dominateurs, tous examines ;
// sinon arret juste apres le kmax-ieme (rang de son bit, plus un).
G1Result from_mask(u64 mask, u32 selected, u32 kmax) noexcept {
  const u32 count = static_cast<u32>(std::popcount(mask));
  if (count < kmax) return {count, selected};
  for (u32 t = 1; t < kmax; ++t) mask &= mask - 1;  // efface les kmax-1 premiers dominateurs
  return {kmax, static_cast<u32>(std::countr_zero(mask)) + 1};
}

}  // namespace

void g1_witnesses(std::span<const G1Terms> terms, G1Witnesses& out) noexcept {
  const u32 selected = static_cast<u32>(std::min<u64>(terms.size(), kG1Slots));
  for (u32 i = 0; i < selected; ++i) {
    out.s0[i] = terms[i].scaled[0];
    out.s1[i] = terms[i].scaled[1];
    out.s2[i] = terms[i].scaled[2];
    out.square[i] = terms[i].square;
  }
  for (u32 i = selected; i < kG1Slots && i % 4 != 0; ++i) out.s0[i] = out.s1[i] = out.s2[i] = out.square[i] = 0;
  out.selected = selected;
}

G1Result g1_scalar(const G1Terms& x, const G1Witnesses& w, u32 kmax) noexcept {
  u32 found = 0, i = 0;
  for (; i < w.selected && found < kmax; ++i) {
    const i64 right = std::max<i64>(0, x.scaled[0] - w.s0[i]) + std::max<i64>(0, x.scaled[1] - w.s1[i]) +
                      std::max<i64>(0, x.scaled[2] - w.s2[i]);
    found += x.square - w.square[i] > right ? 1u : 0u;  // G1 : egalite conservee sur la fermeture de la boite.
  }
  return {found, i};
}

#if defined(MHGP11_G1_AVX2)

__attribute__((target("avx2"))) G1Result g1_avx2(const G1Terms& x, const G1Witnesses& w, u32 kmax) noexcept {
  const __m256i xs0 = _mm256_set1_epi64x(x.scaled[0]), xs1 = _mm256_set1_epi64x(x.scaled[1]);
  const __m256i xs2 = _mm256_set1_epi64x(x.scaled[2]), xsq = _mm256_set1_epi64x(x.square);
  const __m256i zero = _mm256_setzero_si256();
  u64 mask = 0;
  for (u32 i = 0; i < w.selected; i += 4) {
    __m256i d0 = _mm256_sub_epi64(xs0, _mm256_load_si256(reinterpret_cast<const __m256i*>(w.s0.data() + i)));
    __m256i d1 = _mm256_sub_epi64(xs1, _mm256_load_si256(reinterpret_cast<const __m256i*>(w.s1.data() + i)));
    __m256i d2 = _mm256_sub_epi64(xs2, _mm256_load_si256(reinterpret_cast<const __m256i*>(w.s2.data() + i)));
    d0 = _mm256_and_si256(d0, _mm256_cmpgt_epi64(d0, zero));  // max(0, d)
    d1 = _mm256_and_si256(d1, _mm256_cmpgt_epi64(d1, zero));
    d2 = _mm256_and_si256(d2, _mm256_cmpgt_epi64(d2, zero));
    const __m256i right = _mm256_add_epi64(_mm256_add_epi64(d0, d1), d2);
    const __m256i left =
        _mm256_sub_epi64(xsq, _mm256_load_si256(reinterpret_cast<const __m256i*>(w.square.data() + i)));
    const __m256i strict = _mm256_cmpgt_epi64(left, right);  // G1 : inegalite stricte, comme la reference
    mask |= static_cast<u64>(static_cast<u32>(_mm256_movemask_pd(_mm256_castsi256_pd(strict)))) << i;
  }
  mask &= (u64{1} << w.selected) - 1;  // selected <= 36 : les cases du dernier bloc au-dela ne comptent pas
  return from_mask(mask, w.selected, kmax);
}

bool g1_avx2_available() noexcept { return __builtin_cpu_supports("avx2"); }

namespace {
__attribute__((target("avx2"))) void filter_avx2(const Cloud& cloud, std::span<const SiteIdx> parent, const Box& box,
                                                 const G1Witnesses& w, u32 kmax, SiteIdx* out, u32& count,
                                                 u64& tests) noexcept {
  for (SiteIdx s : parent) {
    const G1Result r = g1_avx2(g1_terms(site_coordinates(cloud, s), box), w, kmax);
    tests += r.tests;
    if (r.found < kmax) out[count++] = s;
  }
}
}  // namespace

#else

G1Result g1_avx2(const G1Terms& x, const G1Witnesses& w, u32 kmax) noexcept { return g1_scalar(x, w, kmax); }
bool g1_avx2_available() noexcept { return false; }

#endif

void g1_filter(const Cloud& cloud, std::span<const SiteIdx> parent, const Box& box, const G1Witnesses& w, u32 kmax,
               SiteIdx* out, u32& count, u64& tests) noexcept {
#if defined(MHGP11_G1_AVX2)
  static const bool avx2 = g1_avx2_available();
  if (avx2) {
    filter_avx2(cloud, parent, box, w, kmax, out, count, tests);
    return;
  }
#endif
  for (SiteIdx s : parent) {
    const G1Result r = g1_scalar(g1_terms(site_coordinates(cloud, s), box), w, kmax);
    tests += r.tests;
    if (r.found < kmax) out[count++] = s;
  }
}

}  // namespace mhgp11::catalogue_detail
