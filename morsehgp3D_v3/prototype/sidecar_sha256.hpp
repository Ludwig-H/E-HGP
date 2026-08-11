// MorseHGP3D v3 — SHA-256 CONTRACTUEL des decisions de confiance du sidecar
// (portes 1-2 de AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811).
//
// Toute decision de confiance (liaison recu<->table, digest des membres,
// digest final du certificat) passe par cette serialisation canonique :
// octets LITTLE-ENDIAN EXPLICITES (jamais l'endianness native), sections
// TAGGEES et sequences a longueur prefixee (framing), version de schema en
// tete, et SHA-256 (FIPS 180-4) a la place du FNV-1a 64 bits collisionnable.
// Le digest LIE une preuve ; il ne cree jamais une completude.
//
// L'implementation est autonome. Elle est jugee par les vecteurs FIPS dans
// `sidecar_sha256_selftest` : la factory elle-meme refuse (controle EFFECTIF
// en tete de `build`) et la porte CTest le rejoue — un SHA-256 faux serait
// sinon un lieur muet.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace mhgp3v {

struct Sha256Digest {
  std::array<std::uint8_t, 32> bytes{};
  bool operator==(const Sha256Digest& other) const { return bytes == other.bytes; }
  bool operator!=(const Sha256Digest& other) const { return !(bytes == other.bytes); }
};

inline std::string sha256_hex(const Sha256Digest& digest) {
  static const char* kHex = "0123456789abcdef";
  std::string out;
  out.reserve(64);
  for (std::uint8_t b : digest.bytes) {
    out.push_back(kHex[b >> 4]);
    out.push_back(kHex[b & 15]);
  }
  return out;
}

class Sha256Stream {
 public:
  Sha256Stream() { reset(); }

  void reset() {
    state_ = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
              0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    buffer_fill_ = 0;
    total_bytes_ = 0;
  }

  void update(const void* data, std::size_t bytes) {
    const std::uint8_t* p = (const std::uint8_t*)data;
    total_bytes_ += (unsigned long long)bytes;
    while (bytes > 0) {
      const std::size_t take =
          bytes < (std::size_t)64 - buffer_fill_ ? bytes : (std::size_t)64 - buffer_fill_;
      std::memcpy(buffer_.data() + buffer_fill_, p, take);
      buffer_fill_ += take;
      p += take;
      bytes -= take;
      if (buffer_fill_ == 64) {
        compress(buffer_.data());
        buffer_fill_ = 0;
      }
    }
  }

  // Serialisation canonique : chaque entier est emis en octets LITTLE-ENDIAN
  // explicites — le digest est identique sur toute architecture.
  void put_u8(std::uint8_t v) { update(&v, 1); }
  void put_u32(std::uint32_t v) {
    std::uint8_t b[4];
    for (int i = 0; i < 4; ++i) b[i] = (std::uint8_t)(v >> (8 * i));
    update(b, sizeof(b));
  }
  void put_u64(unsigned long long v) {
    std::uint8_t b[8];
    for (int i = 0; i < 8; ++i) b[i] = (std::uint8_t)(v >> (8 * i));
    update(b, sizeof(b));
  }
  // Complement a deux : la conversion signee -> non signee est definie et
  // canonique, aucune lecture d'image memoire.
  void put_i64(long long v) { put_u64((unsigned long long)v); }
  void put_i128(__int128 v) {
    const unsigned __int128 u = (unsigned __int128)v;
    put_u64((unsigned long long)u);
    put_u64((unsigned long long)(u >> 64));
  }
  // Framing : une section commence par son tag ; toute sequence est prefixee
  // de sa longueur — deux flux concatenes distincts ne se confondent pas.
  void put_tag(std::uint32_t tag) { put_u32(tag); }
  void put_length(unsigned long long count) { put_u64(count); }
  void put_digest(const Sha256Digest& digest) { update(digest.bytes.data(), 32); }

  Sha256Digest finalize() {
    const unsigned long long bit_length = total_bytes_ * 8ULL;
    const std::uint8_t one = 0x80;
    update(&one, 1);
    const std::uint8_t zero = 0x00;
    while (buffer_fill_ != 56) update(&zero, 1);
    // Longueur en BITS, big-endian (FIPS 180-4) — hors de la voie little-endian
    // des champs, c'est le padding du standard.
    std::uint8_t length_bytes[8];
    for (int i = 0; i < 8; ++i)
      length_bytes[i] = (std::uint8_t)(bit_length >> (8 * (7 - i)));
    update(length_bytes, sizeof(length_bytes));
    Sha256Digest digest;
    for (int i = 0; i < 8; ++i) {
      digest.bytes[(std::size_t)(4 * i)] = (std::uint8_t)(state_[(std::size_t)i] >> 24);
      digest.bytes[(std::size_t)(4 * i + 1)] = (std::uint8_t)(state_[(std::size_t)i] >> 16);
      digest.bytes[(std::size_t)(4 * i + 2)] = (std::uint8_t)(state_[(std::size_t)i] >> 8);
      digest.bytes[(std::size_t)(4 * i + 3)] = (std::uint8_t)state_[(std::size_t)i];
    }
    reset();
    return digest;
  }

 private:
  static std::uint32_t rotr(std::uint32_t x, int s) {
    return (x >> s) | (x << (32 - s));
  }

  void compress(const std::uint8_t* block) {
    static const std::uint32_t kRound[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u,
        0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
        0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u,
        0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
        0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
        0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
        0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au,
        0x5b9cca4fu, 0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
        0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
    std::uint32_t schedule[64];
    for (int t = 0; t < 16; ++t)
      schedule[t] = ((std::uint32_t)block[4 * t] << 24) |
                    ((std::uint32_t)block[4 * t + 1] << 16) |
                    ((std::uint32_t)block[4 * t + 2] << 8) | (std::uint32_t)block[4 * t + 3];
    for (int t = 16; t < 64; ++t) {
      const std::uint32_t s0 = rotr(schedule[t - 15], 7) ^ rotr(schedule[t - 15], 18) ^
                               (schedule[t - 15] >> 3);
      const std::uint32_t s1 = rotr(schedule[t - 2], 17) ^ rotr(schedule[t - 2], 19) ^
                               (schedule[t - 2] >> 10);
      schedule[t] = schedule[t - 16] + s0 + schedule[t - 7] + s1;
    }
    std::uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    std::uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
    for (int t = 0; t < 64; ++t) {
      const std::uint32_t big_s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      const std::uint32_t choose = (e & f) ^ (~e & g);
      const std::uint32_t t1 = h + big_s1 + choose + kRound[t] + schedule[t];
      const std::uint32_t big_s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
      const std::uint32_t t2 = big_s0 + majority;
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
  }

  std::array<std::uint32_t, 8> state_{};
  std::array<std::uint8_t, 64> buffer_{};
  std::size_t buffer_fill_ = 0;
  unsigned long long total_bytes_ = 0;
};

// Les vecteurs FIPS 180-4 : vide, « abc », le message de 448 bits, et le
// million de 'a'. Un SHA-256 faux rendrait tous les digests du sidecar muets ;
// la porte refuse donc si ce selftest echoue.
inline bool sidecar_sha256_selftest() {
  const struct {
    const char* message;
    std::size_t repeat;
    const char* expected_hex;
  } kVectors[] = {
      {"", 1, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
      {"abc", 1, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 1,
       "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
      {"a", 1000000,
       "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"},
  };
  Sha256Stream stream;
  for (const auto& vector : kVectors) {
    stream.reset();
    const std::size_t bytes = std::strlen(vector.message);
    for (std::size_t r = 0; r < vector.repeat; ++r) stream.update(vector.message, bytes);
    if (sha256_hex(stream.finalize()) != vector.expected_hex) return false;
  }
  return true;
}

}  // namespace mhgp3v
