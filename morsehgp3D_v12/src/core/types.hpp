// Types fondamentaux de la v12 : entiers, identifiants forts, constantes du profil numerique, garde du flottant.
// Port de src/core/types.hpp de la v11 (commit ac081a06f), lui-meme port de src/core/types.hpp et de la garde de
// src/core/fp_strict.hpp de la v10 (raccord R2, commit 865f5e6).
//
// Identifiants forts : des enum class sur u32. Un rang et un identifiant n'ont jamais le meme type (la v9 a du graver
// un mutant parce qu'ils en partageaient un). Les cinq domaines sont ceux que fixe ARCHITECTURE.md de la v11,
// paragraphe 7.3 ; un decalage de tableau est un u64. La v10 declarait les memes types et n'en employait qu'un, son
// moteur manipulant des u32 nus : chaque module les emploie dans ses signatures.
//
// Profil numerique (ARCHITECTURE.md de la v11, paragraphe 3) : coordonnees entieres 0 <= x < 2^B par axe, ou
// B = MHGP12_COORD_BITS est une constante de compilation posee par la construction (CMakeLists.txt). Aucune valeur par
// defaut ici : deux sources d'une meme constante finissent par diverger. Valeurs admises (decision D6 de la v12) : 21,
// 24 et 32 (ce dernier depuis l'arithmetique en repere local, docs/CONTRAT_NUMERIQUE.md) ; le profil 18 bits de la
// v11 est abandonne et refuse ici comme a la configuration.
//
// Garde du flottant (ARCHITECTURE.md de la v11, paragraphe 4, F5). La v10 exigeait IEEE-754 strict de chaque unite
// et refusait une a une les macros de ses affaiblissements (fp_strict.hpp), sans fermer tous les canaux. La v11 et la
// v12 n'ont pas ce contrat : leurs usages du flottant restent justes sous tout mode d'arrondi, avec ou sans
// contraction, avec ou sans reassociation (F1 a F4). Il ne reste ici qu'une defense en profondeur, sans role dans
// les preuves : le refus de __FAST_MATH__ (-ffast-math, -Ofast), que toute unite du produit porte en incluant cet
// en-tete, et le controle que double est bien le binaire64 de F2. Portes : mhgp12_core_fast_math_refusal,
// mhgp12_core_ofast_refusal.
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#if defined(__FAST_MATH__)
#error "mhgp12_fast_math_interdit : -ffast-math et -Ofast sont refuses (ARCHITECTURE.md de la v11, paragraphe 4, F5)"
#endif

#ifndef MHGP12_COORD_BITS
#error "mhgp12_coord_bits_absent : MHGP12_COORD_BITS (21, 24 ou 32) doit etre defini par la construction"
#endif

namespace mhgp12 {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;

// Hypotheses de plate-forme dont dependent les conversions du produit (tailles en octets, std::size_t sur 64 bits).
static_assert(sizeof(u64) == 8 && sizeof(i128) == 16 && sizeof(std::size_t) == 8,
              "mhgp12 : plate-forme a std::size_t de 64 bits et entiers de 128 bits");
// F2 : les noyaux entiers portes en flottant supposent le binaire64 (base 2, mantisse de 53 bits).
static_assert(std::numeric_limits<double>::radix == 2 && std::numeric_limits<double>::digits == 53,
              "mhgp12 : double doit etre le binaire64");

enum class PointId : u32 {};    // identite externe d'un point d'entree (arbitraire, unique)
enum class SiteIdx : u32 {};    // rang dense d'une position distincte (ordre de Morton)
enum class BallIdx : u32 {};    // rang canonique d'une boule du catalogue
enum class LevelRank : u32 {};  // rang dense d'un niveau exact distinct (0 = niveau nul)
enum class NodeIdx : u32 {};    // noeud d'une foret d'ordre K

using Order = u8;  // ordre K dans [1, kmax] ; 0 = sans objet

// Absence de rang dense (SiteIdx, BallIdx, LevelRank, NodeIdx) : aucun rang valide n'atteint cette valeur, un tableau
// indexe en u32 ayant moins de 2^32 - 1 cases. Ne concerne pas PointId : une identite externe est un u32 quelconque,
// 0xFFFFFFFF compris.
inline constexpr u32 kNone = 0xFFFFFFFFu;

// Bits par coordonnee et plus grande coordonnee admise, 2^B - 1. Le decalage se fait en u64 : B peut valoir 32.
inline constexpr int kCoordBits = MHGP12_COORD_BITS;
static_assert(kCoordBits == 21 || kCoordBits == 24 || kCoordBits == 32,
              "mhgp12_coord_bits_invalide : MHGP12_COORD_BITS doit valoir 21, 24 ou 32 (18 abandonne, decision D6)");
inline constexpr u32 kCoordMax = static_cast<u32>((u64{1} << kCoordBits) - 1);

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

}  // namespace mhgp12
