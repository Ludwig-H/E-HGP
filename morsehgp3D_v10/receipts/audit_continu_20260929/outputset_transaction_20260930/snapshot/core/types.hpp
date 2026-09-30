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
inline constexpr int kCoordinateBits = 18;
inline constexpr i64 kCoordinateLimit = (i64{1} << kCoordinateBits) - 1;  // 262 143
inline constexpr int kMaxOrder = 10;         // kmax public
inline constexpr int kMaxCatalogueOrder = 12;  // catalogue interne (juge d'Euler a kmax + 2)

template <class Id>
constexpr u32 idx(Id x) {
  return static_cast<u32>(x);
}
template <class Id>
constexpr Id make_id(u32 v) {
  return static_cast<Id>(v);
}

}  // namespace mhgp10
