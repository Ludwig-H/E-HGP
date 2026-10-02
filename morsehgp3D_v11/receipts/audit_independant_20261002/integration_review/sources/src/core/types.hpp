// Types fondamentaux de la v11 : entiers, identifiants forts, constantes du profil numerique, garde du flottant.
// Port de src/core/types.hpp et de la garde de src/core/fp_strict.hpp de la v10 (raccord R2, commit 865f5e6).
//
// Identifiants forts : des enum class sur u32. Un rang et un identifiant n'ont jamais le meme type (la v9 a du graver
// un mutant parce qu'ils en partageaient un) : identite d'un point, rang d'un site, rang d'une boule, rang d'un
// niveau et noeud d'une foret sont des domaines distincts.
//
// Profil numerique (docs/ARCHITECTURE.md, paragraphe 3) : coordonnees entieres 0 <= x < 2^B par axe, ou
// B = MHGP11_COORD_BITS est une constante de compilation posee par la construction (CMakeLists.txt). Aucune valeur par
// defaut ici : deux sources d'une meme constante finissent par diverger.
//
// Garde du flottant (docs/ARCHITECTURE.md, paragraphe 4, F5). La v10 exigeait IEEE-754 strict de chaque unite et
// refusait une a une les macros de ses affaiblissements (fp_strict.hpp), sans fermer tous les canaux. La v11 n'a pas ce
// contrat : ses usages du flottant restent justes sous tout mode d'arrondi, avec ou sans contraction, avec ou sans
// reassociation (F1 a F4). Il ne reste ici qu'une defense en profondeur, sans role dans les preuves : le refus de
// __FAST_MATH__ (-ffast-math, -Ofast), que toute unite du produit porte en incluant cet en-tete, et le controle que
// double est bien le binaire64 de F2. Portes : mhgp11_core_fast_math_refusal, mhgp11_core_ofast_refusal.
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#if defined(__FAST_MATH__)
#error "mhgp11_fast_math_interdit : -ffast-math et -Ofast sont refuses (docs/ARCHITECTURE.md, paragraphe 4, F5)"
#endif

#ifndef MHGP11_COORD_BITS
#error "mhgp11_coord_bits_absent : MHGP11_COORD_BITS (18, 21 ou 24) doit etre defini par la construction"
#endif

namespace mhgp11 {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;

// Hypotheses de plate-forme dont dependent les conversions du produit (tailles en octets, std::size_t sur 64 bits).
static_assert(sizeof(u64) == 8 && sizeof(i128) == 16 && sizeof(std::size_t) == 8,
              "mhgp11 : plate-forme a std::size_t de 64 bits et entiers de 128 bits");
// F2 : les noyaux entiers portes en flottant supposent le binaire64 (base 2, mantisse de 53 bits).
static_assert(std::numeric_limits<double>::radix == 2 && std::numeric_limits<double>::digits == 53,
              "mhgp11 : double doit etre le binaire64");

enum class PointId : u32 {};    // identite externe d'un point d'entree (arbitraire, unique)
enum class SiteIdx : u32 {};    // rang dense d'une position distincte (ordre de Morton)
enum class BallIdx : u32 {};    // rang d'une boule dans l'ordre moteur du catalogue
enum class LevelRank : u32 {};  // rang dense d'un niveau exact distinct (0 = niveau nul)
enum class NodeIdx : u32 {};    // noeud d'une foret d'ordre K

using Order = u8;  // ordre K dans [1, kmax] ; 0 = sans objet

// Absence d'indice. Aucun indice valide n'atteint cette valeur : un tableau indexe en u32 a moins de 2^32 - 1 cases.
inline constexpr u32 kNone = 0xFFFFFFFFu;

// Bits par coordonnee et plus grande coordonnee admise, 2^B - 1. B <= 24 < 32 : le decalage tient dans u32.
inline constexpr int kCoordBits = MHGP11_COORD_BITS;
static_assert(kCoordBits == 18 || kCoordBits == 21 || kCoordBits == 24,
              "mhgp11_coord_bits_invalide : MHGP11_COORD_BITS doit valoir 18, 21 ou 24");
inline constexpr u32 kCoordMax = (u32{1} << kCoordBits) - 1;

// Identifiant fort : enum class sur u32.
template <class Id>
concept StrongId = std::is_enum_v<Id> && std::same_as<std::underlying_type_t<Id>, u32>;

template <StrongId Id>
constexpr u32 idx(Id x) noexcept {
  return static_cast<u32>(x);
}
template <StrongId Id>
constexpr Id make_id(u32 v) noexcept {
  return static_cast<Id>(v);
}

}  // namespace mhgp11
