// Geometrie exacte locale : entrees certifiees dans [0,2^B), centres a+N/D, niveaux rationnels.
// Port explicite des formules R2 geometry.hpp/cpp 865f5e6 ; les budgets et refus sont propres a la v11.
#pragma once

#include <array>
#include <optional>

#include "num/level.hpp"

namespace mhgp11::num {

class Point {
 public:
  Point() noexcept = default;
  static Result<Point> make(i64 x, i64 y, i64 z) noexcept;
  u32 x() const noexcept { return coordinates_[0]; }
  u32 y() const noexcept { return coordinates_[1]; }
  u32 z() const noexcept { return coordinates_[2]; }
  const std::array<u32, 3>& coordinates() const noexcept { return coordinates_; }
  friend bool operator==(const Point&, const Point&) = default;

 private:
  explicit Point(std::array<u32, 3> coordinates) noexcept : coordinates_(coordinates) {}
  std::array<u32, 3> coordinates_{};
};

// Sphere construite seulement par ses supports. q3/q4 ne supposent pas aigu/interieur : ces conditions sont des
// predicats distincts. Une dependance affine rend un succes sans sphere, jamais un faux centre ni un refus FULL.
// N et D restent relatifs a anchor ; D>0. Les coordonnees valides ne peuvent plus etre modifiees par un alias.
class Sphere {
 public:
  static Sphere point(Point a) noexcept;
  static Result<std::optional<Sphere>> through(Point a, Point b) noexcept;
  static Result<std::optional<Sphere>> through(Point a, Point b, Point c) noexcept;
  static Result<std::optional<Sphere>> through(Point a, Point b, Point c, Point d) noexcept;
  Point anchor() const noexcept { return anchor_; }
  const std::array<CenterInt, 3>& numerator() const noexcept { return numerator_; }
  CenterDen denominator() const noexcept { return denominator_; }
  const Level& level() const noexcept { return level_; }

 private:
  Sphere(Point anchor, std::array<CenterInt, 3> numerator, CenterDen denominator, Level level) noexcept
      : anchor_(anchor), numerator_(numerator), denominator_(denominator), level_(level) {}
  Point anchor_;
  std::array<CenterInt, 3> numerator_;
  CenterDen denominator_;
  Level level_;
};

// Signes geometriques, sans epsilon : power<0 interieur, =0 coquille, >0 exterieur.
Result<SideInt> power(const Sphere& sphere, Point point) noexcept;
Result<int> side(const Sphere& sphere, Point point) noexcept;
DeterminantInt orientation(Point a, Point b, Point c, Point d) noexcept;
Result<int> orientation(Point a, Point b, Point c, const Sphere& center) noexcept;
bool strictly_acute(Point a, Point b, Point c) noexcept;
Result<bool> strictly_inside(const Sphere& center, Point a, Point b, Point c, Point d) noexcept;
bool is_midpoint(const Sphere& center, Point a, Point b) noexcept;

}  // namespace mhgp11::num
