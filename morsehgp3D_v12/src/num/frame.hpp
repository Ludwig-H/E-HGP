// Repere local certifie (NUM-REPERE, docs/CONTRAT_NUMERIQUE.md, paragraphe 2). Pour un ensemble fini E de points
// entiers (sites, coins de boites fermees), le coin minimal est m = min E par axe et l'etendue est le plus petit s tel
// que max_i max_{x dans E} (x_i - m_i) < 2^s ; etendue nulle : s = 0. Alors |x_i - y_i| < 2^s pour tous x, y de E,
// l'hypothese des budgets (num/budgets.hpp) avec M = 2^s, quelle que soit l'origine o de E des formules.
//
// Coordonnees admises : [0, 2^32]. Un site est dans [0, 2^32) ; la fermeture [lo, hi] d'une boite de centres peut avoir
// hi = max(site) + 1 = 2^32 (CST-0204) : les bornes vivent en u64, aucune n'est calculee en u32, et s va jusqu'a 33
// (22 et 25 aux profils 21 et 24). Calcul entier exact : un minimum, un maximum, un comptage de zeros de tete.
#pragma once

#include <array>
#include <bit>
#include <span>

#include "num/budgets.hpp"

namespace mhgp12::num {

// Plus grande coordonnee d'un repere : borne fermee d'une boite a l'extremite du domaine u32.
inline constexpr u64 kFrameCoordinateMax = u64{1} << 32;

// Etendue en bits d'une largeur : le plus petit s tel que width < 2^s ; 0 pour une largeur nulle.
constexpr int span_bits(u64 width) noexcept { return static_cast<int>(std::bit_width(width)); }
static_assert(span_bits(0) == 0 && span_bits(1) == 1 && span_bits(3) == 2 && span_bits(4) == 3 &&
              span_bits(kFrameCoordinateMax) == kMaxSpan, "num : etendue d'une largeur");

// Repere en construction puis lu : coin minimal, coin maximal, etendue. Vide tant qu'aucun point n'est ajoute.
class Frame {
 public:
  constexpr Frame() noexcept = default;
  // Ajoute un point ou un coin de boite fermee. Refus parameter_out_of_range si une coordonnee depasse 2^32 : le
  // repere reste alors inchange.
  [[nodiscard]] Outcome add(const std::array<u64, 3>& point) noexcept {
    for (const u64 value : point)
      if (value > kFrameCoordinateMax) return fail(Reason::parameter_out_of_range);
    include(point);
    return {};
  }
  // Ajoute un site du domaine u32 : toujours admis.
  constexpr void add_site(u32 x, u32 y, u32 z) noexcept { include({u64{x}, u64{y}, u64{z}}); }
  // Ajoute la fermeture [lo, hi] d'une boite (lo <= hi par axe, sinon refus parameter_out_of_range).
  [[nodiscard]] Outcome add_box(const std::array<u64, 3>& lo, const std::array<u64, 3>& hi) noexcept {
    for (int j = 0; j < 3; ++j)
      if (lo[j] > hi[j]) return fail(Reason::parameter_out_of_range);
    for (int j = 0; j < 3; ++j)
      if (hi[j] > kFrameCoordinateMax) return fail(Reason::parameter_out_of_range);
    include(lo);
    include(hi);
    return {};
  }
  // Reunion de deux reperes : le repere de l'union de leurs ensembles.
  constexpr void unite(const Frame& other) noexcept {
    if (other.empty_) return;
    include(other.lo_);
    include(other.hi_);
  }
  constexpr bool empty() const noexcept { return empty_; }
  constexpr const std::array<u64, 3>& origin() const noexcept { return lo_; }
  constexpr const std::array<u64, 3>& upper() const noexcept { return hi_; }
  // Etendue s (0 pour un repere vide ou d'un seul point), au plus 33.
  constexpr int span() const noexcept {
    u64 width = 0;
    for (int j = 0; j < 3; ++j) width = hi_[j] - lo_[j] > width ? hi_[j] - lo_[j] : width;
    return span_bits(width);
  }
  constexpr Tier tier() const noexcept { return tier_of(span()); }
  // Vrai si le point appartient a la boite [origine, origine + 2^s - 1] du repere (il n'en change pas l'etendue).
  constexpr bool covers(const std::array<u64, 3>& point) const noexcept {
    if (empty_) return false;
    const int s = span();
    for (int j = 0; j < 3; ++j)
      if (point[j] < lo_[j] || point[j] - lo_[j] >= (u64{1} << s)) return false;
    return true;
  }

 private:
  constexpr void include(const std::array<u64, 3>& point) noexcept {
    for (int j = 0; j < 3; ++j) {
      if (empty_ || point[j] < lo_[j]) lo_[j] = point[j];
      if (empty_ || point[j] > hi_[j]) hi_[j] = point[j];
    }
    empty_ = false;
  }
  std::array<u64, 3> lo_{}, hi_{};
  bool empty_ = true;
};

}  // namespace mhgp12::num
