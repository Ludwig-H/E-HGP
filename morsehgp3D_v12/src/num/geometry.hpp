// Geometrie exacte locale : entrees certifiees dans [0,2^B), centres a+N/D, niveaux rationnels.
// Port explicite des formules R2 geometry.hpp/cpp 865f5e6 ; les budgets et refus sont propres a la v11. La v12 ajoute
// le repere local (docs/CONTRAT_NUMERIQUE.md, paragraphes 2 et 3) : chaque sphere porte l'etendue s de son support de
// presentation et deux certificats lies a leur domaine (exposant t, CST-0201) ; chaque predicat choisit sa voie par le
// palier de son repere (num/budgets.hpp) et peut compter la voie empruntee (LaneCount).
#pragma once

#include <array>
#include <optional>

#include "num/frame.hpp"
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

// Certificats lies a leur domaine (CST-0201), calcules une fois par la fabrique : exposant t, le plus grand tel que les
// coefficients garantissent la voie native i128 pour des differences < 2^t (power_certificate.hpp,
// orientation_certificate.hpp) ; -1 si aucun. Le certificat global de la v11 est le cas t >= B.
struct CenterDomains {
  i8 power = -1;
  i8 orientation = -1;
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
  u8 presentation_arity() const noexcept { return static_cast<u8>(shape_ & 7u); }
  // Etendue s des points de presentation (NUM-REPERE) et son palier.
  u8 support_span() const noexcept { return support_span_; }
  Tier tier() const noexcept { return tier_of(support_span_); }
  // Domaines des certificats (exposants t, -1 : aucun).
  int power_domain() const noexcept { return domains_.power; }
  int orientation_domain() const noexcept { return domains_.orientation; }
  // Certificat suffisant pour TOUS Point/Box du profil : seulement power/side/bounds q3, pas orientation/Level.
  bool q3_power_i128_certified() const noexcept { return presentation_arity() == 3 && domains_.power >= kCoordBits; }
  // Certificat distinct : orientation avec trois Point quelconques du profil, sans hypothese de support local.
  bool orientation_i128_certified() const noexcept { return domains_.orientation >= kCoordBits; }
  // Uniquement le tetraedre fourni a through4 ; false aux autres arites, pas qmin ni un test sur d'autres sites.
  bool q4_presentation_strictly_inside() const noexcept { return (shape_ & 8u) != 0; }

 private:
  friend class Q3Candidate;
  friend class Q4Candidate;
  Sphere(Point anchor, std::array<CenterInt, 3> numerator, CenterDen denominator, Level level, u8 arity, u8 span,
         CenterDomains domains, bool q4_presentation_inside = false) noexcept
      : anchor_(anchor), shape_(static_cast<u8>(arity | (q4_presentation_inside ? 8u : 0u))), support_span_(span),
        domains_(domains), numerator_(numerator), denominator_(denominator), level_(level) {}
  Point anchor_;
  u8 shape_;          // arite (bits 0 a 2) et positivite de la presentation q4 (bit 3) ; factories seulement
  u8 support_span_;   // etendue du support de presentation, 0 a 32
  CenterDomains domains_;  // deux octets ; avec les precedents, remplit l'alignement avant les coefficients
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
  // Voie comptee : native (paliers etroit et moyen, i128) ou large (palier large, entiers exacts).
  static Result<std::optional<Q3Candidate>> through(Point a, Point b, Point c, LaneCount* lanes = nullptr) noexcept;
  Point anchor() const noexcept { return anchor_; }
  const std::array<CenterInt, 3>& numerator() const noexcept { return numerator_; }
  CenterDen denominator() const noexcept { return denominator_; }
  u8 presentation_arity() const noexcept { return 3; }
  u8 support_span() const noexcept { return support_span_; }
  Tier tier() const noexcept { return tier_of(support_span_); }
  int power_domain() const noexcept { return domains_.power; }
  int orientation_domain() const noexcept { return domains_.orientation; }
  bool q3_power_i128_certified() const noexcept { return domains_.power >= kCoordBits; }
  bool orientation_i128_certified() const noexcept { return domains_.orientation >= kCoordBits; }
  Result<Sphere> materialize() const noexcept;

 private:
  // Palier large (25 <= s <= 32, profil 32) : memes formules en entiers exacts.
  static Result<std::optional<Q3Candidate>> through_exact(Point a, Point b, Point c, u8 span) noexcept;
  Q3Candidate(Point anchor, Point second, Point third, std::array<CenterInt, 3> numerator, CenterDen denominator,
              u8 span, CenterDomains domains) noexcept
      : anchor_(anchor), second_(second), third_(third), support_span_(span), domains_(domains),
        numerator_(numerator), denominator_(denominator) {}
  Point anchor_, second_, third_;  // sommets de CETTE presentation, lus seulement par materialize
  u8 support_span_;
  CenterDomains domains_;  // meme certificat de puissance que Sphere::through3, jamais derive de l'orientation
  std::array<CenterInt, 3> numerator_;
  CenterDen denominator_;
};

// Presentation q4 fermee, sans Level : l'ancre reste un site de coquille, N/D est le centre relatif exact.
// La fabrique conserve la positivite stricte de SA presentation sans rejeter les poids nuls/negatifs.
// strictly_inside sur quatre sites quelconques reste un predicat distinct. Materialiser garde le niveau non reduit
// de Sphere::through4 (v11 d40585570), sans cache mutable, allocation ni emprunt aux points de construction.
class Q4Candidate {
 public:
  static Result<std::optional<Q4Candidate>> through(Point a, Point b, Point c, Point d,
                                                    LaneCount* lanes = nullptr) noexcept;
  Point anchor() const noexcept { return anchor_; }
  const std::array<CenterInt, 3>& numerator() const noexcept { return numerator_; }
  CenterDen denominator() const noexcept { return denominator_; }
  u8 presentation_arity() const noexcept { return 4; }
  u8 support_span() const noexcept { return support_span_; }
  Tier tier() const noexcept { return tier_of(support_span_); }
  int power_domain() const noexcept { return domains_.power; }
  int orientation_domain() const noexcept { return domains_.orientation; }
  bool orientation_i128_certified() const noexcept { return domains_.orientation >= kCoordBits; }
  bool q4_presentation_strictly_inside() const noexcept { return q4_presentation_inside_; }
  Result<Sphere> materialize() const noexcept;

 private:
  static Result<std::optional<Q4Candidate>> through_exact(Point a, Point b, Point c, Point d, u8 span) noexcept;
  Q4Candidate(Point anchor, std::array<CenterInt, 3> numerator, CenterDen denominator, u8 span, CenterDomains domains,
              bool q4_presentation_inside) noexcept
      : anchor_(anchor), support_span_(span), domains_(domains), q4_presentation_inside_(q4_presentation_inside),
        numerator_(numerator), denominator_(denominator) {}
  Point anchor_;
  u8 support_span_;
  CenterDomains domains_;  // transmis avec D/N a materialize
  bool q4_presentation_inside_;  // booleen ferme, pas de cache mutable ni de proprietaire du support
  std::array<CenterInt, 3> numerator_;
  CenterDen denominator_;
};

// Ordre lexicographique (x,y,z) exact des centres a+N/D, meme hors du domaine de Point. En deux temps (paragraphe 4
// du contrat) : parties entieres floor(c_j) = a_j + floor(N_j/D) sur 64 bits, puis parties fractionnaires
// (N_j mod D)/D par produits croises au palier du plus large des deux supports ; axe par axe, meme ordre que la v11.
// Ignore le rayon et l'arite ; centres egaux sous des ancres/denominateurs differents =>0. Aucun tas/flottant.
int compare_centers(const Sphere& a, const Sphere& b, LaneCount* lanes = nullptr) noexcept;

// Distance carree de deux Point certifies (NUM-REQUETE, CST-0110) : resultat dans [0,3*(2^B-1)^2]. Somme des carres
// en u64 controlee, recalculee en u128 au debordement (possible seulement au profil 32) ; voie native garantie quand
// l'etendue de la paire est au plus 31.
DotInt squared_distance(Point a, Point b, LaneCount* lanes = nullptr) noexcept;

// Signes geometriques, sans epsilon : power<0 interieur, =0 coquille, >0 exterieur. Voie par le palier du repere
// support+requete (u = max(s, etendue de la requete autour de l'ancre)), puis certificat de domaine, puis essai
// controle, puis voie large a largeur du palier.
Result<SideInt> power(const Sphere& sphere, Point point, LaneCount* lanes = nullptr) noexcept;
Result<SideInt> power(const Q3Candidate& sphere, Point point, LaneCount* lanes = nullptr) noexcept;
Result<SideInt> power(const Q4Candidate& sphere, Point point, LaneCount* lanes = nullptr) noexcept;
Result<int> side(const Sphere& sphere, Point point, LaneCount* lanes = nullptr) noexcept;
Result<int> side(const Q3Candidate& sphere, Point point, LaneCount* lanes = nullptr) noexcept;
Result<int> side(const Q4Candidate& sphere, Point point, LaneCount* lanes = nullptr) noexcept;
// Encadrement entier de D*|z-a|^2-2N.(z-a) sur la boite continue fermee, pas les extrema exacts en general.
// Les extrema separes du terme quadratique et du terme lineaire evitent N^2 et tout nouveau degre dix.
struct PowerBounds { SideInt lower{}, upper{}; };
Result<PowerBounds> power_bounds(const Sphere& sphere, const Box& box, LaneCount* lanes = nullptr) noexcept;
// Signes (-1,0,1) des deux bornes de power_bounds, memes refus. Voie native : sommes i128 lues sans Wide.
struct PowerBoundSigns { int lower = 0, upper = 0; };
Result<PowerBoundSigns> power_bound_signs(const Sphere& sphere, const Box& box, LaneCount* lanes = nullptr) noexcept;
DeterminantInt orientation(Point a, Point b, Point c, Point d) noexcept;
Result<int> orientation(Point a, Point b, Point c, const Sphere& center, LaneCount* lanes = nullptr) noexcept;
Result<int> orientation(Point a, Point b, Point c, const Q3Candidate& center, LaneCount* lanes = nullptr) noexcept;
Result<int> orientation(Point a, Point b, Point c, const Q4Candidate& center, LaneCount* lanes = nullptr) noexcept;
bool strictly_acute(Point a, Point b, Point c) noexcept;
// Classe la presentation sans construire centre/niveau. Non_strict inclut les triangles droits et obtus,
// jamais les alignements/doublons ; une presentation stricte implique deja la non-degenerescence.
enum class TriangleKind : u8 { degenerate, non_strict, strict };
TriangleKind classify_triangle(Point a, Point b, Point c) noexcept;
Result<bool> strictly_inside(const Sphere& center, Point a, Point b, Point c, Point d) noexcept;
Result<bool> strictly_inside(const Q3Candidate& center, Point a, Point b, Point c, Point d) noexcept;
Result<bool> strictly_inside(const Q4Candidate& center, Point a, Point b, Point c, Point d) noexcept;
// Test du milieu en forme locale (CST-0114) : 2 N_j = D ((a_j-o_j)+(b_j-o_j)), au palier du repere support+{a,b}.
bool is_midpoint(const Sphere& center, Point a, Point b, LaneCount* lanes = nullptr) noexcept;
bool is_midpoint(const Q3Candidate& center, Point a, Point b, LaneCount* lanes = nullptr) noexcept;
bool is_midpoint(const Q4Candidate& center, Point a, Point b, LaneCount* lanes = nullptr) noexcept;

}  // namespace mhgp12::num
