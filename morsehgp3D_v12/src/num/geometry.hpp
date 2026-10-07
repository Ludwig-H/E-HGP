// Geometrie exacte locale : entrees certifiees dans [0,2^B), centres a+N/D, niveaux rationnels.
// Port explicite des formules R2 geometry.hpp/cpp 865f5e6 ; les budgets et refus sont propres a la v11.
#pragma once

#include <array>
#include <optional>

#include "num/level.hpp"

namespace mhgp12::num {

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

// Boite continue FERMEE dans le domaine de Point. Une largeur nulle est valide ; lo>hi est refuse.
// Les deux extremites sont possedees, sans alias mutable ni constructeur de coefficients nus.
class Box {
 public:
  static Result<Box> make(Point lo, Point hi) noexcept;
  Point lo() const noexcept { return lo_; }
  Point hi() const noexcept { return hi_; }

 private:
  Box(Point lo, Point hi) noexcept : lo_(lo), hi_(hi) {}
  Point lo_, hi_;
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
  // Arite de la presentation fabriquee, pas qmin ni une propriete canonique de la boule.
  u8 presentation_arity() const noexcept { return presentation_arity_; }
  // Certificat suffisant pour TOUS Point/Box du profil : seulement power/side/bounds q3, pas orientation/Level.
  bool q3_power_i128_certified() const noexcept { return q3_power_i128_; }
  // Certificat distinct : orientation avec trois Point quelconques du profil, sans hypothese de support local.
  bool orientation_i128_certified() const noexcept { return orientation_i128_; }
  // Uniquement le tetraedre fourni a through4 ; false aux autres arites, pas qmin ni un test sur d'autres sites.
  bool q4_presentation_strictly_inside() const noexcept { return q4_presentation_inside_; }

 private:
  friend class Q3Candidate;
  friend class Q4Candidate;
  Sphere(Point anchor, std::array<CenterInt, 3> numerator, CenterDen denominator, Level level, u8 arity,
         bool q3_power_i128 = false, bool orientation_i128 = false, bool q4_presentation_inside = false) noexcept
      : anchor_(anchor), presentation_arity_(arity), q3_power_i128_(q3_power_i128),
        orientation_i128_(orientation_i128), q4_presentation_inside_(q4_presentation_inside),
        numerator_(numerator), denominator_(denominator), level_(level) {}
  Point anchor_;
  u8 presentation_arity_;  // factories seulement ; place dans l'alignement avant les coefficients i128
  bool q3_power_i128_;  // meme padding avant numerator_ ; copie avec les coefficients, aucun cache mutable
  bool orientation_i128_;  // ne derive jamais du certificat de puissance
  bool q4_presentation_inside_;  // dernier octet du padding ; propriete des quatre sites d'origine seulement
  std::array<CenterInt, 3> numerator_;
  CenterDen denominator_;
  Level level_;
};

// Presentation q3 fermee, sans Level (audit heritage 1235da4ac) : ancre, N/D et les deux certificats de
// Sphere::through3, plus les deux autres sommets. Materialiser calcule la formule brute de degre six
// |u|^2|v|^2|c-b|^2/(4|u x v|^2), sans PGCD ni |N|^2/D^2 : Sphere::through3 la delegue ici, une seule source.
// Arite 3 pour les predicats : jamais de retag q4, la voie native q3 reste soumise a son certificat de puissance.
class Q3Candidate {
 public:
  static Result<std::optional<Q3Candidate>> through(Point a, Point b, Point c) noexcept;
  Point anchor() const noexcept { return anchor_; }
  const std::array<CenterInt, 3>& numerator() const noexcept { return numerator_; }
  CenterDen denominator() const noexcept { return denominator_; }
  u8 presentation_arity() const noexcept { return 3; }
  bool q3_power_i128_certified() const noexcept { return q3_power_i128_; }
  bool orientation_i128_certified() const noexcept { return orientation_i128_; }
  Result<Sphere> materialize() const noexcept;

 private:
  Q3Candidate(Point anchor, Point second, Point third, std::array<CenterInt, 3> numerator, CenterDen denominator,
              bool q3_power_i128, bool orientation_i128) noexcept
      : anchor_(anchor), second_(second), third_(third), q3_power_i128_(q3_power_i128),
        orientation_i128_(orientation_i128), numerator_(numerator), denominator_(denominator) {}
  Point anchor_, second_, third_;  // sommets de CETTE presentation, lus seulement par materialize
  bool q3_power_i128_;  // meme certificat que Sphere::through3, jamais derive de l'orientation
  bool orientation_i128_;
  std::array<CenterInt, 3> numerator_;
  CenterDen denominator_;
};

// Presentation q4 fermee, sans Level : l'ancre reste un site de coquille, N/D est le centre relatif exact.
// La fabrique conserve la positivite stricte de SA presentation sans rejeter les poids nuls/negatifs.
// strictly_inside sur quatre sites quelconques reste un predicat distinct. Materialiser garde le niveau non reduit
// de Sphere::through4 (v11 d40585570), sans cache mutable, allocation ni emprunt aux points de construction.
class Q4Candidate {
 public:
  static Result<std::optional<Q4Candidate>> through(Point a, Point b, Point c, Point d) noexcept;
  Point anchor() const noexcept { return anchor_; }
  const std::array<CenterInt, 3>& numerator() const noexcept { return numerator_; }
  CenterDen denominator() const noexcept { return denominator_; }
  u8 presentation_arity() const noexcept { return 4; }
  bool orientation_i128_certified() const noexcept { return orientation_i128_; }
  bool q4_presentation_strictly_inside() const noexcept { return q4_presentation_inside_; }
  Result<Sphere> materialize() const noexcept;

 private:
  Q4Candidate(Point anchor, std::array<CenterInt, 3> numerator, CenterDen denominator, bool orientation_i128,
              bool q4_presentation_inside) noexcept
      : anchor_(anchor), orientation_i128_(orientation_i128), q4_presentation_inside_(q4_presentation_inside),
        numerator_(numerator), denominator_(denominator) {}
  Point anchor_;
  bool orientation_i128_;  // padding avant numerator_, transmission avec D/N a materialize
  bool q4_presentation_inside_;  // booleen ferme, pas de cache mutable ni de proprietaire du support
  std::array<CenterInt, 3> numerator_;
  CenterDen denominator_;
};

// Ordre lexicographique (x,y,z) exact des centres a+N/D, meme hors du domaine de Point.
// Ignore le rayon et l'arite ; centres egaux sous des ancres/denominateurs differents =>0. Aucun tas/flottant.
int compare_centers(const Sphere& a, const Sphere& b) noexcept;

// Distance carree de deux Point certifies : resultat dans [0,3*(2^B-1)^2], natif i64 aux trois profils.
DotInt squared_distance(Point a, Point b) noexcept;

// Signes geometriques, sans epsilon : power<0 interieur, =0 coquille, >0 exterieur.
Result<SideInt> power(const Sphere& sphere, Point point) noexcept;
Result<SideInt> power(const Q3Candidate& sphere, Point point) noexcept;
Result<SideInt> power(const Q4Candidate& sphere, Point point) noexcept;
Result<int> side(const Sphere& sphere, Point point) noexcept;
Result<int> side(const Q3Candidate& sphere, Point point) noexcept;
Result<int> side(const Q4Candidate& sphere, Point point) noexcept;
// Encadrement entier de D*|z-a|^2-2N.(z-a) sur la boite continue fermee, pas les extrema exacts en general.
// Les extrema separes du terme quadratique et du terme lineaire evitent N^2 et tout nouveau degre dix.
struct PowerBounds { SideInt lower{}, upper{}; };
Result<PowerBounds> power_bounds(const Sphere& sphere, const Box& box) noexcept;
// Signes (-1,0,1) des deux bornes de power_bounds, memes refus. Voie native : sommes i128 lues sans Wide.
struct PowerBoundSigns { int lower = 0, upper = 0; };
Result<PowerBoundSigns> power_bound_signs(const Sphere& sphere, const Box& box) noexcept;
DeterminantInt orientation(Point a, Point b, Point c, Point d) noexcept;
Result<int> orientation(Point a, Point b, Point c, const Sphere& center) noexcept;
Result<int> orientation(Point a, Point b, Point c, const Q3Candidate& center) noexcept;
Result<int> orientation(Point a, Point b, Point c, const Q4Candidate& center) noexcept;
bool strictly_acute(Point a, Point b, Point c) noexcept;
// Classe la presentation sans construire centre/niveau. Non_strict inclut les triangles droits et obtus,
// jamais les alignements/doublons ; une presentation stricte implique deja la non-degenerescence.
enum class TriangleKind : u8 { degenerate, non_strict, strict };
TriangleKind classify_triangle(Point a, Point b, Point c) noexcept;
Result<bool> strictly_inside(const Sphere& center, Point a, Point b, Point c, Point d) noexcept;
Result<bool> strictly_inside(const Q3Candidate& center, Point a, Point b, Point c, Point d) noexcept;
Result<bool> strictly_inside(const Q4Candidate& center, Point a, Point b, Point c, Point d) noexcept;
bool is_midpoint(const Sphere& center, Point a, Point b) noexcept;
bool is_midpoint(const Q3Candidate& center, Point a, Point b) noexcept;
bool is_midpoint(const Q4Candidate& center, Point a, Point b) noexcept;

}  // namespace mhgp12::num
