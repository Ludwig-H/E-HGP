// SHA-256 (FIPS 180-4) et ecriture hexadecimale des empreintes. Port de CanonicalSha256Builder de morsehgp3d
// (src/cpu/contract/canonical_id.cpp, lignes 18-45 et 49-221 au commit 1bf4be68f ; docs/PROVENANCE.md, section io).
//
// Repris tels quels : constantes de tour, fonction de compression a mots de travail tournes par leur nom (huit tours
// deroules, aucune recopie de registres), absorption en trois phases (completer un bloc partiel, compresser les blocs
// entiers directement depuis la memoire de l'appelant, garder la queue), remplissage final (0x80, zeros, longueur en
// bits sur 64 bits gros-boutistes).
// Ce qui change : aucune exception. La source levait sur une mise a jour apres finalisation et sur un debordement du
// compte de bits ; ici finish() calcule sur une copie de l'etat (il ne finalise rien et peut etre rappele), et la
// longueur du message est bornee par kMaxMessageBytes, que FileWriter garde avant d'absorber.
// Portes : mhgp11_io_unit_sha256 (vecteurs de FIPS 180-4 et decoupages), mhgp11_io_sha256 (differentiel contre
// hashlib, tailles 0 a 200 puis multiblocs) ; mutants sha_tronque, sha_longueur_en_octets.
#include <algorithm>
#include <bit>
#include <cstring>

#include "io/io.hpp"

namespace mhgp11::io {

namespace {

constexpr std::array<u32, 64> kRound{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

// Une tour : les huit mots de travail sont tournes par l'ordre des arguments, seuls d et h sont ecrits.
inline void round_step(u32 a, u32 b, u32 c, u32& d, u32 e, u32 f, u32 g, u32& h, u32 k, u32 w) noexcept {
  const u32 sum1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
  const u32 choice = (e & f) ^ (~e & g);
  const u32 t1 = h + sum1 + choice + k + w;
  const u32 sum0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
  const u32 majority = (a & b) ^ (a & c) ^ (b & c);
  d += t1;
  h = t1 + sum0 + majority;
}

}  // namespace

Sha256::Sha256() noexcept
    : state_{0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU, 0x510e527fU, 0x9b05688cU, 0x1f83d9abU,
             0x5be0cd19U} {}

void Sha256::compress(const u8* block) noexcept {
  std::array<u32, 64> w{};
  for (std::size_t i = 0; i < 16; ++i)
    w[i] = (u32{block[4 * i]} << 24) | (u32{block[4 * i + 1]} << 16) | (u32{block[4 * i + 2]} << 8) |
           u32{block[4 * i + 3]};
  for (std::size_t i = 16; i < 64; ++i) {
    const u32 s0 = std::rotr(w[i - 15], 7) ^ std::rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    const u32 s1 = std::rotr(w[i - 2], 17) ^ std::rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  u32 a = state_[0], b = state_[1], c = state_[2], d = state_[3];
  u32 e = state_[4], f = state_[5], g = state_[6], h = state_[7];
  for (std::size_t r = 0; r < 64; r += 8) {
    round_step(a, b, c, d, e, f, g, h, kRound[r], w[r]);
    round_step(h, a, b, c, d, e, f, g, kRound[r + 1], w[r + 1]);
    round_step(g, h, a, b, c, d, e, f, kRound[r + 2], w[r + 2]);
    round_step(f, g, h, a, b, c, d, e, kRound[r + 3], w[r + 3]);
    round_step(e, f, g, h, a, b, c, d, kRound[r + 4], w[r + 4]);
    round_step(d, e, f, g, h, a, b, c, kRound[r + 5], w[r + 5]);
    round_step(c, d, e, f, g, h, a, b, kRound[r + 6], w[r + 6]);
    round_step(b, c, d, e, f, g, h, a, kRound[r + 7], w[r + 7]);
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

void Sha256::update(std::span<const u8> bytes) noexcept {
  const u64 n = bytes.size();
  if (n == 0) return;  // une vue vide peut porter un pointeur nul : aucun memcpy
  total_ += n;
  u64 offset = 0;
  if (buffered_ != 0) {  // completer un bloc partiel
    const u64 taken = std::min<u64>(64 - buffered_, n);
    std::memcpy(buffer_.data() + buffered_, bytes.data(), taken);
    buffered_ += taken;
    offset = taken;
    if (buffered_ == 64) {
      compress(buffer_.data());
      buffered_ = 0;
    }
  }
  for (; n - offset >= 64; offset += 64) compress(bytes.data() + offset);  // blocs entiers, sans recopie
  const u64 rest = n - offset;
  if (rest != 0) {  // garder la queue ; buffered_ est nul ici des que rest > 0
    std::memcpy(buffer_.data(), bytes.data() + offset, rest);
    buffered_ = rest;
  }
}

void Sha256::update(std::string_view text) noexcept {
  update(std::span<const u8>(reinterpret_cast<const u8*>(text.data()), text.size()));
}

Digest Sha256::finish() const noexcept {
  Sha256 last = *this;  // copie : l'etat de cet objet n'est pas finalise
  const u64 bits = total_ * 8;
  last.buffer_[last.buffered_++] = 0x80;
  if (last.buffered_ > 56) {
    std::memset(last.buffer_.data() + last.buffered_, 0, 64 - last.buffered_);
    last.compress(last.buffer_.data());
    last.buffered_ = 0;
  }
  std::memset(last.buffer_.data() + last.buffered_, 0, 56 - last.buffered_);
  for (std::size_t i = 0; i < 8; ++i) last.buffer_[56 + i] = static_cast<u8>(bits >> (8 * (7 - i)));
  last.compress(last.buffer_.data());
  Digest out{};
  for (std::size_t i = 0; i < 8; ++i)
    for (std::size_t j = 0; j < 4; ++j) out[4 * i + j] = static_cast<u8>(last.state_[i] >> (8 * (3 - j)));
  return out;
}

std::array<char, 2 * kDigestBytes> to_hex(const Digest& digest) noexcept {
  constexpr char kDigits[] = "0123456789abcdef";
  std::array<char, 2 * kDigestBytes> out{};
  for (std::size_t i = 0; i < kDigestBytes; ++i) {
    out[2 * i] = kDigits[digest[i] >> 4];
    out[2 * i + 1] = kDigits[digest[i] & 15];
  }
  return out;
}

}  // namespace mhgp11::io
