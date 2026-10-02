// SHA-256 (sha256.hpp), d'apres FIPS 180-4 : fonctions du paragraphe 4.1.2, constantes du 4.2.2, bourrage du 5.1.1,
// valeur initiale du 5.3.3, calcul du 6.2.2. Les constantes sont controlees a la compilation contre leur definition :
// K[i] est fait des 32 premiers bits de la partie fractionnaire de la racine cubique du i-eme nombre premier, la
// valeur initiale de ceux de la racine carree des huit premiers.
#include "io/sha256.hpp"

#include <bit>
#include <cstring>

namespace mhgp11::io {

namespace {

inline constexpr std::array<u32, 64> kRound = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

inline constexpr std::array<u32, 8> kInitial = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                                0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};

constexpr bool is_prime(u32 n) noexcept {
  for (u32 d = 2; d * d <= n; ++d)
    if (n % d == 0) return false;
  return n >= 2;
}

// Vrai si `fraction` est fait des 32 premiers bits de la partie fractionnaire de la racine `degree`-ieme de p :
// avec a la partie entiere de la racine et c = a 2^32 + fraction, cela s'ecrit c^degree <= p 2^(32 degree) <
// (c + 1)^degree. Bornes : p <= 311 < 2^9 (64-ieme nombre premier), c + 1 <= 7 * 2^32 < 2^35, degree <= 3 : toutes
// les puissances sont inferieures a 2^105 et tiennent dans un u128.
constexpr bool is_root_fraction(u32 p, int degree, u32 fraction) noexcept {
  auto power = [degree](u128 v) {
    u128 out = 1;
    for (int i = 0; i < degree; ++i) out *= v;
    return out;
  };
  u128 whole = 1;
  while (power(whole + 1) <= p) ++whole;
  const u128 c = (whole << 32) | fraction;
  const u128 target = static_cast<u128>(p) << (32 * degree);
  return power(c) <= target && target < power(c + 1);
}

// Controle une table contre sa definition, sur les premiers nombres premiers dans l'ordre.
template <std::size_t N>
constexpr bool matches_definition(const std::array<u32, N>& table, int degree) noexcept {
  u32 p = 1;
  for (std::size_t i = 0; i < N; ++i) {
    do {
      ++p;
    } while (!is_prime(p));
    if (!is_root_fraction(p, degree, table[i])) return false;
  }
  return true;
}
static_assert(matches_definition(kRound, 3), "sha256 : constantes K de FIPS 180-4, paragraphe 4.2.2");
static_assert(matches_definition(kInitial, 2), "sha256 : valeur initiale de FIPS 180-4, paragraphe 5.3.3");

// Calcul d'un bloc de 64 octets (FIPS 180-4, 6.2.2). Toute l'arithmetique est modulo 2^32, par construction de u32.
void compress(std::array<u32, 8>& state, const u8* block) noexcept {
  std::array<u32, 64> w;
  for (int t = 0; t < 16; ++t)
    w[t] = (u32{block[4 * t]} << 24) | (u32{block[4 * t + 1]} << 16) | (u32{block[4 * t + 2]} << 8) |
           u32{block[4 * t + 3]};
  for (int t = 16; t < 64; ++t) {
    const u32 s0 = std::rotr(w[t - 15], 7) ^ std::rotr(w[t - 15], 18) ^ (w[t - 15] >> 3);
    const u32 s1 = std::rotr(w[t - 2], 17) ^ std::rotr(w[t - 2], 19) ^ (w[t - 2] >> 10);
    w[t] = s1 + w[t - 7] + s0 + w[t - 16];
  }
  u32 a = state[0], b = state[1], c = state[2], d = state[3], e = state[4], f = state[5], g = state[6], h = state[7];
  for (int t = 0; t < 64; ++t) {
    const u32 big1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
    const u32 choice = (e & f) ^ (~e & g);
    const u32 t1 = h + big1 + choice + kRound[t] + w[t];
    const u32 big0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
    const u32 majority = (a & b) ^ (a & c) ^ (b & c);
    const u32 t2 = big0 + majority;
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  state[5] += f;
  state[6] += g;
  state[7] += h;
}

}  // namespace

Sha256::Sha256() noexcept : state_(kInitial), block_{} {}

Outcome Sha256::update(std::string_view bytes) noexcept {
  if (!sha256_accepts(bytes_, bytes.size())) return fail(Reason::output_unwritable);
  std::size_t filled = static_cast<std::size_t>(bytes_ % 64);
  bytes_ += bytes.size();  // sans debordement : sha256_accepts
  const char* in = bytes.data();
  std::size_t left = bytes.size();
  if (filled != 0) {
    // complete le bloc en cours : 64 - filled octets au plus
    const std::size_t take = left < 64 - filled ? left : 64 - filled;
    std::memcpy(block_.data() + filled, in, take);
    in += take;
    left -= take;
    filled += take;
    if (filled < 64) return {};
    compress(state_, block_.data());
  }
  for (; left >= 64; in += 64, left -= 64) {
    std::memcpy(block_.data(), in, 64);
    compress(state_, block_.data());
  }
  if (left != 0) std::memcpy(block_.data(), in, left);
  return {};
}

Sha256Digest Sha256::digest() const noexcept {
  // Bourrage (FIPS 180-4, 5.1.1) sur une copie : un bit a un (octet 0x80), des zeros jusqu'a 56 octets modulo 64,
  // puis la longueur en bits sur 64 bits, poids fort d'abord. bytes_ <= kSha256MaxBytes : 8 * bytes_ ne deborde pas.
  std::array<u32, 8> state = state_;
  std::array<u8, 64> block = block_;
  std::size_t filled = static_cast<std::size_t>(bytes_ % 64);
  block[filled++] = 0x80;
  if (filled > 56) {
    std::memset(block.data() + filled, 0, 64 - filled);
    compress(state, block.data());
    filled = 0;
  }
  std::memset(block.data() + filled, 0, 56 - filled);
  const u64 bits = 8 * bytes_;
  for (int i = 0; i < 8; ++i) block[56 + i] = static_cast<u8>(bits >> (56 - 8 * i));
  compress(state, block.data());
  Sha256Digest out;
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 4; ++j) out[4 * i + j] = static_cast<u8>(state[i] >> (24 - 8 * j));
  return out;
}

std::array<char, 64> sha256_hex(const Sha256Digest& digest) noexcept {
  static constexpr char kHex[] = "0123456789abcdef";
  std::array<char, 64> out;
  for (std::size_t i = 0; i < digest.size(); ++i) {
    out[2 * i] = kHex[digest[i] >> 4];
    out[2 * i + 1] = kHex[digest[i] & 0x0F];
  }
  return out;
}

}  // namespace mhgp11::io
