// Graphe J2 ferme d'une petite feuille, construit pendant les tests de dominance deja payes.
// Implementation neuve du lemme des cliques ; aucun tableau de triplets ni condition d'angle.
#pragma once

#include <algorithm>
#include <bit>
#include <span>

#include "core/core.hpp"

namespace mhgp11::catalogue_detail {

class SmallPairGraph {
 public:
  static constexpr u32 kCapacity = 32;
  static Result<SmallPairGraph> make(std::span<u64> storage, u32 count, bool requested) noexcept {
    const bool enabled = requested && count <= kCapacity;
    if (enabled && storage.size() < kCapacity) return fail(Reason::catalogue_invariant);
    auto rows = enabled ? storage.first(kCapacity) : std::span<u64>{};
    std::fill(rows.begin(), rows.end(), u64{0});  // poison, feuille et passe precedents oublies
    return SmallPairGraph(rows, count, enabled);
  }
  bool enabled() const noexcept { return enabled_; }
  u64 initial() const noexcept {
    // count<=32 sur cette voie : jamais de decalage de 64 ; bits [count,64) toujours nuls.
    return enabled_ ? (u64{1} << count_) - 1 : 0;
  }
  // Le prepare certifie 0<=i<j<count<=32. Une arete signifie contact OU intersection de bissectrice.
  void connect(u32 i, u32 j) noexcept {
    rows_[i] |= u64{1} << j;
    rows_[j] |= u64{1} << i;
  }
  u64 neighbors(u32 i) const noexcept { return rows_[i]; }
  u32 next(u64& candidates, u32 fallback) const noexcept {
    if (!enabled_) return fallback;
    if (candidates == 0) return count_;
    const u32 i = static_cast<u32>(std::countr_zero(candidates));
    candidates &= candidates - 1;
    return i;
  }

 private:
  SmallPairGraph(std::span<u64> rows, u32 count, bool enabled) noexcept
      : rows_(rows), count_(count), enabled_(enabled) {}
  std::span<u64> rows_;
  u32 count_;
  bool enabled_;
};

}  // namespace mhgp11::catalogue_detail
