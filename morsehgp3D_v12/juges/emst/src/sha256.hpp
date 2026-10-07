// SHA-256 (FIPS 180-4), ecrit ici pour que le juge JUG-EMST n'ait aucune dependance ; verifie par l'auto-test sur
// les vecteurs de la norme et, dans les portes Python, contre hashlib.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace mhgp12_emst {

class Sha256 {
 public:
  Sha256() { reinitialiser(); }

  void reinitialiser() {
    etat_ = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
             0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
    rempli_ = 0;
    total_ = 0;
  }

  void ajouter(const void* donnees, std::size_t taille) {
    const auto* octets = static_cast<const std::uint8_t*>(donnees);
    total_ += taille;
    while (taille > 0) {
      if (rempli_ == 0 && taille >= 64) {
        compresser(octets);
        octets += 64;
        taille -= 64;
        continue;
      }
      const std::size_t pris = taille < 64 - rempli_ ? taille : 64 - rempli_;
      std::memcpy(bloc_.data() + rempli_, octets, pris);
      rempli_ += pris;
      octets += pris;
      taille -= pris;
      if (rempli_ == 64) {
        compresser(bloc_.data());
        rempli_ = 0;
      }
    }
  }

  // Mot de 64 bits petit-boutiste (convention des vidages et des empreintes de la v11).
  void mot(std::uint64_t valeur) {
    if (rempli_ > 56) {
      std::array<std::uint8_t, 8> octets{};
      for (int i = 0; i < 8; ++i) octets[i] = static_cast<std::uint8_t>(valeur >> (8 * i));
      ajouter(octets.data(), octets.size());
      return;
    }
    for (int i = 0; i < 8; ++i) bloc_[rempli_ + i] = static_cast<std::uint8_t>(valeur >> (8 * i));
    rempli_ += 8;
    total_ += 8;
    if (rempli_ == 64) {
      compresser(bloc_.data());
      rempli_ = 0;
    }
  }

  std::string hex() {
    const std::uint64_t bits = total_ * 8;
    const std::uint8_t un = 0x80;
    ajouter(&un, 1);
    const std::uint8_t zero = 0;
    while (rempli_ != 56) ajouter(&zero, 1);
    std::array<std::uint8_t, 8> longueur{};
    for (int i = 0; i < 8; ++i) longueur[i] = static_cast<std::uint8_t>(bits >> (56 - 8 * i));
    ajouter(longueur.data(), longueur.size());
    static const char* chiffres = "0123456789abcdef";
    std::string texte;
    for (std::uint32_t valeur : etat_) {
      for (int i = 28; i >= 0; i -= 4) texte += chiffres[(valeur >> i) & 15u];
    }
    reinitialiser();
    return texte;
  }

 private:
  static std::uint32_t rot(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

  void compresser(const std::uint8_t* b) {
    static const std::uint32_t k[64] = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
    std::uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
      w[i] = (std::uint32_t{b[4 * i]} << 24) | (std::uint32_t{b[4 * i + 1]} << 16) |
             (std::uint32_t{b[4 * i + 2]} << 8) | std::uint32_t{b[4 * i + 3]};
    }
    for (int i = 16; i < 64; ++i) {
      const std::uint32_t s0 = rot(w[i - 15], 7) ^ rot(w[i - 15], 18) ^ (w[i - 15] >> 3);
      const std::uint32_t s1 = rot(w[i - 2], 17) ^ rot(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    std::uint32_t a = etat_[0], bb = etat_[1], c = etat_[2], d = etat_[3];
    std::uint32_t e = etat_[4], f = etat_[5], g = etat_[6], h = etat_[7];
    for (int i = 0; i < 64; ++i) {
      const std::uint32_t t1 = h + (rot(e, 6) ^ rot(e, 11) ^ rot(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
      const std::uint32_t t2 = (rot(a, 2) ^ rot(a, 13) ^ rot(a, 22)) + ((a & bb) ^ (a & c) ^ (bb & c));
      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = bb;
      bb = a;
      a = t1 + t2;
    }
    etat_[0] += a;
    etat_[1] += bb;
    etat_[2] += c;
    etat_[3] += d;
    etat_[4] += e;
    etat_[5] += f;
    etat_[6] += g;
    etat_[7] += h;
  }

  std::array<std::uint32_t, 8> etat_{};
  std::array<std::uint8_t, 64> bloc_{};
  std::size_t rempli_ = 0;
  std::uint64_t total_ = 0;
};

}  // namespace mhgp12_emst
