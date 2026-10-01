// Types fondamentaux de la v10 : entiers, identifiants forts, ordre.
//
// Les identifiants sont des enum class sur u32 : un rang et un identifiant n'ont jamais le meme type
// (la v9 a du graver un mutant parce qu'ils en partageaient un).
#pragma once

#include <cstddef>
#include <cstdint>

namespace mhgp10 {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;

enum class PointId : u32 {};    // identite externe d'un point d'entree (arbitraire, unique)
enum class SiteIdx : u32 {};    // rang dense d'une position distincte (ordre de Morton)
enum class BallIdx : u32 {};    // rang d'une boule dans l'ordre moteur du catalogue
enum class LevelRank : u32 {};  // rang dense d'un niveau exact distinct (0 = niveau nul)
enum class NodeIdx : u32 {};    // noeud d'une foret d'ordre K

using Order = u8;               // K dans [1, kmax]

inline constexpr u32 kNone = 0xFFFFFFFFu;
inline constexpr int kCoordinateBits = 18;   // profil par defaut (u18 : grille de 1 mm sur une trame KITTI)
inline constexpr i64 kCoordinateLimit = (i64{1} << kCoordinateBits) - 1;  // 262 143
// Palier servi par le moteur (catalogue, tour, filtres) : bits <= 21 (0,1 mm sur une trame KITTI, plan de precision du
// 30 septembre 2026, tranche T1). Au-dela : refus parameter_out_of_range. Les sorties du profil u18 sont inchangees.
inline constexpr int kEngineMaxBits = 21;
inline constexpr int kMaxOrder = 10;         // kmax public
inline constexpr int kMaxCatalogueOrder = 12;  // catalogue interne (juge d'Euler a kmax + 2)

// Modes du moteur pour les differentiels (defauts = production ; chaque mode est exact par construction) :
//  ArithMode : dispatch = voie courte i128 quand sa condition prouvee tient, voie large sinon ; wide = toujours la voie
//              large ; narrow = jamais la voie large : un predicat dont la condition de voie courte est fausse est
//              REFUSE (Reason::arith_guard), jamais calcule faux ;
//  FilterMode : on = filtres flottants certifies a repli exact ; exact = aucun filtre, toute decision en exact.
enum class ArithMode : u8 { dispatch, wide, narrow };
enum class FilterMode : u8 { on, exact };

template <class Id>
constexpr u32 idx(Id x) {
  return static_cast<u32>(x);
}
template <class Id>
constexpr Id make_id(u32 v) {
  return static_cast<Id>(v);
}

}  // namespace mhgp10
