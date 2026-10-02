// Lecture stricte d'un entier decimal : UN seul lecteur pour toute valeur entiere donnee en texte (options de la
// ligne de commande). Port de parse_integer de la v10 (src/core/cli_options.hpp du raccord R2, commit 865f5e6).
//
// Frontiere (audits de la v10 du 29 septembre 2026) : std::stoi et std::stoul consommaient un prefixe
// (--k=2not_an_integer lu 2, --k=0x2 lu 0), acceptaient un signe (--threads=-1 lu 2^64 - 1) et la conversion vers
// u32 reduisait modulo 2^32 (--leaf=4294967304 lu 8), toujours avec le code 0. Ici :
//   - le jeton est consomme EN ENTIER : aucun prefixe, aucun espace, aucun suffixe ;
//   - chiffres decimaux ASCII seulement (ni base implicite 0x ou 0, ni '+') ; un '-' en tete seulement si la borne
//     basse est negative ; zeros de tete admis (base 10 toujours) ;
//   - la borne [lo, hi] est verifiee AVANT la conversion vers le type cible : la magnitude est lue chiffre par
//     chiffre en u64 avec detection du depassement, puis comparee en i128 ; aucune valeur n'est jamais reduite
//     modulo 2^32 ou 2^64.
// Refus : parameter_out_of_range (invalid_input). N'alloue pas, ne leve pas.
//
// Non porte : parse_integer_list (listes --k-list), sans emploi dans cette tranche.
#pragma once

#include <concepts>
#include <limits>
#include <string_view>
#include <type_traits>

#include "core/core.hpp"

namespace mhgp11::io {

// Entier decimal strict dans [lo, hi]. T : entier de 64 bits au plus, ni bool ni caractere.
template <class T>
[[nodiscard]] Result<T> parse_integer(std::string_view token, T lo, T hi) noexcept {
  static_assert(std::is_integral_v<T> && !std::same_as<T, bool> && !std::same_as<T, char> && sizeof(T) <= 8,
                "parse_integer : entier de 64 bits au plus");
  const bool negative = !token.empty() && token.front() == '-';
  if (negative) {
    // signe refuse si aucune valeur negative n'est admise ("-0" compris)
    if (std::is_unsigned_v<T> || !(static_cast<i128>(lo) < 0)) return fail(Reason::parameter_out_of_range);
    token.remove_prefix(1);
  }
  if (token.empty()) return fail(Reason::parameter_out_of_range);
  const u64 max = std::numeric_limits<u64>::max();
  u64 magnitude = 0;
  for (const char c : token) {
    if (c < '0' || c > '9') return fail(Reason::parameter_out_of_range);
    const u64 digit = static_cast<u64>(c - '0');
    // 10 * magnitude + digit <= max si et seulement si magnitude <= (max - digit) / 10 (division entiere) : le test
    // precede le produit, qui ne deborde donc jamais.
    if (magnitude > (max - digit) / 10) return fail(Reason::parameter_out_of_range);
    magnitude = 10 * magnitude + digit;
  }
  // |valeur| < 2^64 : elle tient dans un i128 avec son signe.
  const i128 value = negative ? -static_cast<i128>(magnitude) : static_cast<i128>(magnitude);
  if (value < static_cast<i128>(lo) || value > static_cast<i128>(hi)) return fail(Reason::parameter_out_of_range);
  return static_cast<T>(value);  // exacte : lo <= value <= hi, bornes de T
}

}  // namespace mhgp11::io
