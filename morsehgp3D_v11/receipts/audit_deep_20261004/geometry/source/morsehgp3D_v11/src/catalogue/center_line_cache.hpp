// Memo J2 borne, distinct des angles, de G3 et de l'admission d'une boule.
// Le Buffer emprunte appartient au Workspace ; chaque nouvelle feuille reinitialise ses etats.
#pragma once

#include <algorithm>
#include <span>

#include "num/num.hpp"

namespace mhgp11::catalogue_detail {

class CenterLineCache {
 public:
  static constexpr u32 kCapacity = 32;
  static constexpr u32 entries(u32 count) noexcept {
    const u32 m = std::min(count, kCapacity);
    return m < 3 ? 0 : m * (m - 1) * (m - 2) / 6;
  }
  // Rang combinatoire dense : 0<=i<j<k<32, sans dependance envers la taille de la feuille.
  static constexpr u32 rank(u32 i, u32 j, u32 k) noexcept {
    return k * (k - 1) * (k - 2) / 6 + j * (j - 1) / 2 + i;
  }
  struct Reply {
    num::CenterLineRelation relation;
    bool hit, fallback;
  };

  static Result<CenterLineCache> make(std::span<u8> storage, std::span<const num::Point> points,
                                      num::CenterRegion region, bool requested) noexcept {
    const bool enabled = requested && points.size() <= kCapacity;
    const u32 size = enabled ? entries(static_cast<u32>(points.size())) : 0;
    if (size > storage.size()) return fail(Reason::catalogue_invariant);
    auto states = storage.first(size);
    std::fill(states.begin(), states.end(), u8{0});  // jamais reutiliser une autre feuille ou passe
    return CenterLineCache(states, points, region, enabled, requested && !enabled);
  }

  Result<Reply> lookup(u32 i, u32 j, u32 k) noexcept {
    if (!(i < j && j < k && k < points_.size())) return fail(Reason::catalogue_invariant);
    const u32 at = enabled_ ? rank(i, j, k) : 0;
    if (enabled_ && states_[at] != 0) {
      if (states_[at] > 3) return fail(Reason::catalogue_invariant);
      return Reply{static_cast<num::CenterLineRelation>(states_[at] - 1), true, false};
    }
    const auto relation = num::center_line_meets(points_[i], points_[j], points_[k], region_);
    if (enabled_) states_[at] = static_cast<u8>(static_cast<u8>(relation) + 1);
    return Reply{relation, false, fallback_};
  }

 private:
  CenterLineCache(std::span<u8> states, std::span<const num::Point> points, num::CenterRegion region,
                  bool enabled, bool fallback) noexcept
      : states_(states), points_(points), region_(region), enabled_(enabled), fallback_(fallback) {}
  std::span<u8> states_;
  std::span<const num::Point> points_;
  num::CenterRegion region_;
  bool enabled_, fallback_;
};

static_assert(CenterLineCache::entries(32) == 4960);
static_assert(CenterLineCache::rank(29, 30, 31) == 4959);

}  // namespace mhgp11::catalogue_detail
