// SHA-256, ecrit depuis la norme FIPS 180-4 (Secure Hash Standard, aout 2015), sans dependance : il sert a publier
// l'empreinte d'une sortie canonique, y compris sans l'ecrire sur disque. Ecrit a neuf pour la v11 (la v10 confiait
// les empreintes a des outils externes).
//
// Le message est absorbe par morceaux (update) ; digest rend l'empreinte de tout ce qui a ete absorbe et ne modifie
// pas l'objet. Portes : vecteurs officiels du NIST, longueurs aux bords du bourrage, decoupages
// (mhgp11_io_unit_sha256).
//
// Domaine : la longueur du message en bits s'ecrit sur 64 bits (FIPS 180-4, 5.1.1), donc moins de 2^61 octets. Un
// morceau qui ferait sortir le message de ce domaine est refuse (output_unwritable : la sortie dont on prend
// l'empreinte ne peut pas etre produite en entier) et l'etat est inchange ; aucune longueur n'est jamais tronquee.
#pragma once

#include <array>
#include <limits>
#include <string_view>

#include "core/core.hpp"

namespace mhgp11::io {

using Sha256Digest = std::array<u8, 32>;

// Plus grand nombre d'octets d'un message : 8 fois ce nombre tient dans 64 bits.
inline constexpr u64 kSha256MaxBytes = (u64{1} << 61) - 1;
static_assert(kSha256MaxBytes <= std::numeric_limits<u64>::max() / 8, "sha256 : la longueur en bits tient sur 64 bits");

// Vrai si un message de `total` octets peut en recevoir `more` de plus sans sortir du domaine. Aucune somme n'est
// calculee avant d'etre bornee : total <= max, puis more <= max - total.
constexpr bool sha256_accepts(u64 total, u64 more) noexcept {
  return total <= kSha256MaxBytes && more <= kSha256MaxBytes - total;
}
static_assert(sha256_accepts(0, kSha256MaxBytes) && sha256_accepts(kSha256MaxBytes - 1, 1) &&
                  sha256_accepts(kSha256MaxBytes, 0),
              "sha256 : le domaine va jusqu'a 2^61 - 1 octets compris");
static_assert(!sha256_accepts(0, kSha256MaxBytes + 1) && !sha256_accepts(kSha256MaxBytes, 1) &&
                  !sha256_accepts(1, std::numeric_limits<u64>::max()),
              "sha256 : 2^61 octets sont hors du domaine");

class Sha256 {
 public:
  Sha256() noexcept;

  // Absorbe un morceau du message. Refus output_unwritable hors du domaine, etat inchange.
  [[nodiscard]] Outcome update(std::string_view bytes) noexcept;
  // Empreinte du message absorbe jusqu'ici. Ne modifie pas l'objet : on peut continuer d'absorber.
  Sha256Digest digest() const noexcept;
  // Octets absorbes.
  u64 bytes() const noexcept { return bytes_; }

 private:
  std::array<u32, 8> state_;   // valeur de hachage courante, H(i) de la norme
  std::array<u8, 64> block_;   // bloc en cours : ses bytes_ % 64 premiers octets sont remplis
  u64 bytes_ = 0;              // longueur du message absorbe, <= kSha256MaxBytes
};

// Empreinte en 64 chiffres hexadecimaux minuscules (sans terminateur).
std::array<char, 64> sha256_hex(const Sha256Digest& digest) noexcept;

}  // namespace mhgp11::io
