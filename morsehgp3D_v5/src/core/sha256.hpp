// MorseHGP3D v5 — SHA-256 autonome en streaming (FIPS 180-4) pour la
// SIGNATURE CANONIQUE de l'objet produit : deux objets differents peuvent
// partager toutes leurs cardinalites, les campagnes hors juge comparent des
// digests. La porte est l'EGALITE entre runs apparies, jamais une propriete
// cryptographique.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "types.hpp"

#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#include <immintrin.h>
#define MHGP5_SHA256_NI 1
#endif

namespace mhgp5 {

#if defined(MHGP5_SHA256_NI)
namespace sha256_detail {
// Transformation d'UN bloc par les instructions SHA (Intel/AMD SHA-NI),
// mise en page de Jeffrey Walton (domaine public) : meme fonction de
// compression FIPS 180-4, verifiee bit a bit contre le chemin portable par
// `mhgp5_sha256_gate` (vecteurs FIPS + tampons aleatoires).
__attribute__((target("sha,sse4.1,ssse3"))) inline void block_ni(uint32_t state[8], const uint8_t* data) {
  __m128i STATE0, STATE1, MSG, TMP, MSG0, MSG1, MSG2, MSG3, ABEF_SAVE, CDGH_SAVE;
  const __m128i MASK = _mm_set_epi64x(0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL);
  TMP = _mm_loadu_si128((const __m128i*)&state[0]);
  STATE1 = _mm_loadu_si128((const __m128i*)&state[4]);
  TMP = _mm_shuffle_epi32(TMP, 0xB1);
  STATE1 = _mm_shuffle_epi32(STATE1, 0x1B);
  STATE0 = _mm_alignr_epi8(TMP, STATE1, 8);
  STATE1 = _mm_blend_epi16(STATE1, TMP, 0xF0);
  ABEF_SAVE = STATE0;
  CDGH_SAVE = STATE1;
  // Rounds 0-3
  MSG = _mm_loadu_si128((const __m128i*)(data + 0));
  MSG0 = _mm_shuffle_epi8(MSG, MASK);
  MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0xE9B5DBA5B5C0FBCFULL, 0x71374491428A2F98ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  // Rounds 4-7
  MSG1 = _mm_loadu_si128((const __m128i*)(data + 16));
  MSG1 = _mm_shuffle_epi8(MSG1, MASK);
  MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0xAB1C5ED5923F82A4ULL, 0x59F111F13956C25BULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);
  // Rounds 8-11
  MSG2 = _mm_loadu_si128((const __m128i*)(data + 32));
  MSG2 = _mm_shuffle_epi8(MSG2, MASK);
  MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0x550C7DC3243185BEULL, 0x12835B01D807AA98ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);
  // Rounds 12-15
  MSG3 = _mm_loadu_si128((const __m128i*)(data + 48));
  MSG3 = _mm_shuffle_epi8(MSG3, MASK);
  MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0xC19BF1749BDC06A7ULL, 0x80DEB1FE72BE5D74ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
  MSG0 = _mm_add_epi32(MSG0, TMP);
  MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);
  // Rounds 16-19
  MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0x240CA1CC0FC19DC6ULL, 0xEFBE4786E49B69C1ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
  MSG1 = _mm_add_epi32(MSG1, TMP);
  MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);
  // Rounds 20-23
  MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0x76F988DA5CB0A9DCULL, 0x4A7484AA2DE92C6FULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
  MSG2 = _mm_add_epi32(MSG2, TMP);
  MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);
  // Rounds 24-27
  MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0xBF597FC7B00327C8ULL, 0xA831C66D983E5152ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
  MSG3 = _mm_add_epi32(MSG3, TMP);
  MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);
  // Rounds 28-31
  MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0x1429296706CA6351ULL, 0xD5A79147C6E00BF3ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
  MSG0 = _mm_add_epi32(MSG0, TMP);
  MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);
  // Rounds 32-35
  MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0x53380D134D2C6DFCULL, 0x2E1B213827B70A85ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
  MSG1 = _mm_add_epi32(MSG1, TMP);
  MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);
  // Rounds 36-39
  MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0x92722C8581C2C92EULL, 0x766A0ABB650A7354ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
  MSG2 = _mm_add_epi32(MSG2, TMP);
  MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG0 = _mm_sha256msg1_epu32(MSG0, MSG1);
  // Rounds 40-43
  MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0xC76C51A3C24B8B70ULL, 0xA81A664BA2BFE8A1ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
  MSG3 = _mm_add_epi32(MSG3, TMP);
  MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG1 = _mm_sha256msg1_epu32(MSG1, MSG2);
  // Rounds 44-47
  MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0x106AA070F40E3585ULL, 0xD6990624D192E819ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG3, MSG2, 4);
  MSG0 = _mm_add_epi32(MSG0, TMP);
  MSG0 = _mm_sha256msg2_epu32(MSG0, MSG3);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG2 = _mm_sha256msg1_epu32(MSG2, MSG3);
  // Rounds 48-51
  MSG = _mm_add_epi32(MSG0, _mm_set_epi64x(0x34B0BCB52748774CULL, 0x1E376C0819A4C116ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG0, MSG3, 4);
  MSG1 = _mm_add_epi32(MSG1, TMP);
  MSG1 = _mm_sha256msg2_epu32(MSG1, MSG0);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  MSG3 = _mm_sha256msg1_epu32(MSG3, MSG0);
  // Rounds 52-55
  MSG = _mm_add_epi32(MSG1, _mm_set_epi64x(0x682E6FF35B9CCA4FULL, 0x4ED8AA4A391C0CB3ULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG1, MSG0, 4);
  MSG2 = _mm_add_epi32(MSG2, TMP);
  MSG2 = _mm_sha256msg2_epu32(MSG2, MSG1);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  // Rounds 56-59
  MSG = _mm_add_epi32(MSG2, _mm_set_epi64x(0x8CC7020884C87814ULL, 0x78A5636F748F82EEULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  TMP = _mm_alignr_epi8(MSG2, MSG1, 4);
  MSG3 = _mm_add_epi32(MSG3, TMP);
  MSG3 = _mm_sha256msg2_epu32(MSG3, MSG2);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  // Rounds 60-63
  MSG = _mm_add_epi32(MSG3, _mm_set_epi64x(0xC67178F2BEF9A3F7ULL, 0xA4506CEB90BEFFFAULL));
  STATE1 = _mm_sha256rnds2_epu32(STATE1, STATE0, MSG);
  MSG = _mm_shuffle_epi32(MSG, 0x0E);
  STATE0 = _mm_sha256rnds2_epu32(STATE0, STATE1, MSG);
  STATE0 = _mm_add_epi32(STATE0, ABEF_SAVE);
  STATE1 = _mm_add_epi32(STATE1, CDGH_SAVE);
  TMP = _mm_shuffle_epi32(STATE0, 0x1B);
  STATE1 = _mm_shuffle_epi32(STATE1, 0xB1);
  STATE0 = _mm_blend_epi16(TMP, STATE1, 0xF0);
  STATE1 = _mm_alignr_epi8(STATE1, TMP, 8);
  _mm_storeu_si128((__m128i*)&state[0], STATE0);
  _mm_storeu_si128((__m128i*)&state[4], STATE1);
}
inline bool sha_ni_available() {
  static const bool ok = __builtin_cpu_supports("sha") && __builtin_cpu_supports("sse4.1") && __builtin_cpu_supports("ssse3");
  return ok;
}
}  // namespace sha256_detail
#endif

class Sha256 {
 public:
  // `force_portable` : impose le chemin portable (porte d'egalite des deux chemins).
  explicit Sha256(bool force_portable = false) : portable_(force_portable) { reset(); }
  static bool accelerated() {
#if defined(MHGP5_SHA256_NI)
    return sha256_detail::sha_ni_available();
#else
    return false;
#endif
  }
  void reset() {
    static constexpr uint32_t kInit[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                          0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    std::memcpy(h_, kInit, sizeof(h_));
    len_ = 0;
    buf_len_ = 0;
  }
  void update(const void* data, size_t n) {
    const uint8_t* p = (const uint8_t*)data;
    len_ += n;
    if (buf_len_ == 0) {  // blocs entiers directement depuis la source
      while (n >= 64) {
        block(p);
        p += 64;
        n -= 64;
      }
    }
    while (n > 0) {
      const size_t take = 64 - buf_len_ < n ? 64 - buf_len_ : n;
      std::memcpy(buf_ + buf_len_, p, take);
      buf_len_ += take;
      p += take;
      n -= take;
      if (buf_len_ == 64) {
        block(buf_);
        buf_len_ = 0;
      }
    }
  }
  // Commodites de signature : entiers en little-endian a largeur fixe, textes
  // avec leur longueur (aucune ambiguite de concatenation).
  void u64le(uint64_t v) {
    uint8_t b[8];
    for (int i = 0; i < 8; ++i) b[i] = (uint8_t)(v >> (8 * i));
    update(b, 8);
  }
  void i64le(int64_t v) { u64le((uint64_t)v); }
  void u128le(u128 v) {
    u64le((uint64_t)v);
    u64le((uint64_t)(v >> 64));
  }
  void i128le(i128 v) { u128le((u128)v); }
  void tag(const char* text) {
    const uint64_t n = std::strlen(text);
    u64le(n);
    update(text, (size_t)n);
  }
  // Finalise dans out[32] ; l'objet doit etre reset() avant reutilisation.
  void final(uint8_t out[32]) {
    const uint64_t bits = len_ * 8;
    const uint8_t one = 0x80;
    update(&one, 1);
    const uint8_t zero = 0;
    while (buf_len_ != 56) update(&zero, 1);
    uint8_t be[8];
    for (int i = 0; i < 8; ++i) be[i] = (uint8_t)(bits >> (56 - 8 * i));
    update(be, 8);
    for (int i = 0; i < 8; ++i)
      for (int j = 0; j < 4; ++j) out[4 * i + j] = (uint8_t)(h_[i] >> (24 - 8 * j));
  }
  std::string hex() {
    uint8_t d[32];
    final(d);
    char s[65];
    for (int i = 0; i < 32; ++i) std::snprintf(s + 2 * i, 3, "%02x", d[i]);
    return std::string(s, 64);
  }

 private:
  static uint32_t rotr(uint32_t x, int r) { return (x >> r) | (x << (32 - r)); }
  void block(const uint8_t* p) {
#if defined(MHGP5_SHA256_NI)
    if (!portable_ && sha256_detail::sha_ni_available()) {
      sha256_detail::block_ni(h_, p);
      return;
    }
#endif
    block_portable(p);
  }
  void block_portable(const uint8_t* p) {
    static constexpr uint32_t kK[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u,
        0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu,
        0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu,
        0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
        0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
        0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
        0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u,
        0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u,
        0xc67178f2u};
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
      w[i] = ((uint32_t)p[4 * i] << 24) | ((uint32_t)p[4 * i + 1] << 16) |
             ((uint32_t)p[4 * i + 2] << 8) | (uint32_t)p[4 * i + 3];
    for (int i = 16; i < 64; ++i) {
      const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
      const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = h_[0], b = h_[1], c = h_[2], d = h_[3], e = h_[4], f = h_[5], g = h_[6], hh = h_[7];
    for (int i = 0; i < 64; ++i) {
      const uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      const uint32_t ch = (e & f) ^ (~e & g);
      const uint32_t t1 = hh + s1 + ch + kK[i] + w[i];
      const uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      const uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
      const uint32_t t2 = s0 + mj;
      hh = g; g = f; f = e; e = d + t1;
      d = c; c = b; b = a; a = t1 + t2;
    }
    h_[0] += a; h_[1] += b; h_[2] += c; h_[3] += d;
    h_[4] += e; h_[5] += f; h_[6] += g; h_[7] += hh;
  }
  uint32_t h_[8];
  uint64_t len_ = 0;
  uint8_t buf_[64];
  size_t buf_len_ = 0;
  bool portable_ = false;
};

}  // namespace mhgp5
