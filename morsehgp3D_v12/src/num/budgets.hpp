// Budgets d'entiers en fonction de l'etendue locale s, paliers et voies (docs/CONTRAT_NUMERIQUE.md, paragraphe 3).
// Convention de la v11 (src/num/budgets.hpp, commit ac081a06f) : une expression "de b bits" est de valeur absolue
// < 2^b quand toute difference de coordonnees qu'elle lit est de valeur absolue < M = 2^s. La v11 posait s = B, le
// domaine global (Budgets<B>) ; la v12 lit s dans le repere de l'objet (NUM-REPERE, num/frame.hpp) : feuille, boule,
// partie de descente, requete. Les formules restent celles de la v11, seul leur argument change.
//
// Paliers : constantes de compilation communes a l'hote et a l'appareil. Etroit s <= 16, moyen s <= 24, large
// s <= 33 (boites fermees a 2^32, CST-0204). Chaque expression a une voie par palier, lue au plafond du palier : native
// (i64 ou i128) si son budget y tient ; sinon controlee (__builtin_*_overflow sur des operandes deja au type large, repli
// au debordement) ou certifiee (certificat de domaine, CST-0201) ; sinon large, a nombre de mots fixe par le palier.
// Les coupes : 16 est le plus grand s ou l'orientation avec centre (7s+9) et le cote garde (6s+11) tiennent en i128 ;
// 24 le plus grand ou le numerateur du centre q3 (5s+5) et le test du milieu en repere de feuille (5s+6) y tiennent ;
// 33 couvre toute fermeture de boite du profil 32. Elles sont celles que le contrat propose (paragraphe 3), sans
// changement : MES-S place toutes les feuilles mesurees sous s = 17 et tous les supports sous s = 15.
//
// Le STOCKAGE (coefficients d'une Sphere, niveaux, valeurs rendues par les predicats generiques sur des Point du
// profil) suit le plus grand repere que le profil produit pour ces objets : un support est fait de points de [0,2^B),
// son etendue est au plus B. DomainBudget = SpanBudgets<kCoordBits> ne sert qu'a ces types ; aucun choix de voie et
// aucune largeur de calcul ne le lisent.
#pragma once

#include "num/integer.hpp"

namespace mhgp12::num {

// Plus grande etendue d'un repere : fermeture d'une boite de centres a l'extremite du domaine u32 (hi = 2^32).
inline constexpr int kMaxSpan = 33;

template <int S>
struct SpanBudgets {
  static_assert(S >= 0 && S <= kMaxSpan, "num : etendue locale dans [0, 33]");
  static constexpr int span = S;
  static constexpr int difference = S;
  static constexpr int dot = 2 * S + 2;               // somme de trois produits : < 3 M^2
  static constexpr int cross = 2 * S + 1;             // difference de deux produits : < 2 M^2
  static constexpr int determinant = 3 * S + 3;       // six produits : < 6 M^3
  static constexpr int dominance = 2 * S + 3;         // G1 sur une boite : < 6 M^2
  static constexpr int reservoir = 2 * S + 4;         // sum (2x-lo-hi)^2 : < 12 M^2 (CST-0208)
  static constexpr int numerator3 = 5 * S + 5;        // (uu v-vv u) x (u x v) : < 24 M^5
  static constexpr int denominator3 = 4 * S + 5;      // 2 |u x v|^2 : < 24 M^4
  static constexpr int numerator4 = 4 * S + 5;        // trois produits norm2*cross : < 18 M^4
  static constexpr int denominator4 = 3 * S + 4;      // 2 det : < 12 M^3
  static constexpr int center_numerator = numerator3;
  static constexpr int center_denominator = denominator3;
  static constexpr int side = 6 * S + 8;              // D |z-a|^2 - 2 N.(z-a) : < 216 M^6
  static constexpr int side4 = 5 * S + 7;             // meme expression q4, normales a ancrage commun : < 72 M^5
  static constexpr int center_orientation = 7 * S + 9;  // cross.(N+D(a-p)) : < 288 M^7
  static constexpr int level_numerator = 8 * S + 12;  // q4 : 3*(18 M^4)^2 < 1024 M^8
  static constexpr int level_denominator = 6 * S + 8;  // q4 : (12 M^3)^2 < 256 M^6
  static constexpr int level_comparison = level_numerator + level_denominator;  // 14 S + 20
  // Budgets mixtes (CST-0111) : centre au budget du support, sites confrontes dans la garde (|z_j-o_j| < 3M ;
  // majorant conserve depuis le pave resserre de NUM-GARDE, qui donne |z_j-o_j| < 2M).
  static constexpr int guarded_side = 6 * S + 11;     // < 648 M^6 + 432 M^6 < 2^11 M^6
  static constexpr int guarded_orientation = 7 * S + 14;  // < 14 400 M^7 < 2^14 M^7
  static constexpr int guarded_vertex_orientation = 3 * S + 10;
  static constexpr int acute = 2 * S + 7;
  // Test du milieu en forme locale 2 N_j = D ((a_j-o_j)+(b_j-o_j)) (CST-0114).
  static constexpr int midpoint_guarded = 5 * S + 8;
  static constexpr int midpoint_frame = 5 * S + 6;
  // Comparaison de centres en deux temps (paragraphe 4) : produits croises des parties fractionnaires.
  static constexpr int center_fraction = 8 * S + 10;
  static_assert(6 * S + 5 <= level_numerator && 4 * S + 6 <= level_denominator,
                "num : formes reduites q2/q3 couvertes par le budget de niveau");
};

// Paliers d'etendue.
enum class Tier : u8 { narrow = 0, medium = 1, wide = 2 };
inline constexpr int kNarrowSpan = 16;
inline constexpr int kMediumSpan = 24;
inline constexpr int kWideSpan = kMaxSpan;

constexpr int tier_span(Tier tier) noexcept {
  return tier == Tier::narrow ? kNarrowSpan : tier == Tier::medium ? kMediumSpan : kWideSpan;
}
// Palier d'une etendue s de [0, 33] ; au-dela, le palier large (aucune etendue d'un repere valide n'y arrive).
constexpr Tier tier_of(int span) noexcept {
  return span <= kNarrowSpan ? Tier::narrow : span <= kMediumSpan ? Tier::medium : Tier::wide;
}
template <Tier T>
using TierBudgets = SpanBudgets<tier_span(T)>;
// Nombre de mots de 64 bits d'un entier large de `bits` bits de magnitude.
constexpr int words_for(int bits) noexcept { return (bits + 63) / 64; }

// Coupes des paliers, verifiees contre la table du contrat (paragraphe 3).
static_assert(TierBudgets<Tier::narrow>::center_orientation <= 127 && TierBudgets<Tier::narrow>::guarded_side <= 127 &&
              TierBudgets<Tier::narrow>::guarded_orientation <= 127 && TierBudgets<Tier::narrow>::side <= 127,
              "num : palier etroit, orientation et cotes natifs");
static_assert(SpanBudgets<kNarrowSpan + 1>::center_orientation > 127, "num : 16 est la coupe de l'orientation");
static_assert(TierBudgets<Tier::medium>::numerator3 <= 127 && TierBudgets<Tier::medium>::denominator3 <= 127 &&
              TierBudgets<Tier::medium>::numerator4 <= 127 && TierBudgets<Tier::medium>::side4 <= 127 &&
              TierBudgets<Tier::medium>::midpoint_frame <= 127 && TierBudgets<Tier::medium>::dot <= 63,
              "num : palier moyen, centres et milieu natifs");
static_assert(SpanBudgets<kMediumSpan + 1>::numerator3 > 127, "num : 24 est la coupe du centre q3");
static_assert(TierBudgets<Tier::wide>::level_comparison <= 512, "num : comparaison de niveaux en 512 bits a s = 33");

// Voie effectivement empruntee par une evaluation : native garantie par le palier, native garantie par un certificat
// de domaine, controlee (essai natif sans debordement), large (essai controle en echec ou voie large d'office).
enum class Lane : u8 { native = 0, certified = 1, checked = 2, wide = 3 };

// Compteurs logiques de voies : independants de l'ordre de visite, jamais dans une empreinte de sortie.
struct LaneCount {
  u64 native = 0, certified = 0, checked = 0, wide = 0;
  void add(Lane lane) noexcept {
    switch (lane) {
      case Lane::native: ++native; break;
      case Lane::certified: ++certified; break;
      case Lane::checked: ++checked; break;
      case Lane::wide: ++wide; break;
    }
  }
  u64 total() const noexcept { return native + certified + checked + wide; }
  friend bool operator==(const LaneCount&, const LaneCount&) = default;
};
inline void count_lane(LaneCount* lanes, Lane lane) noexcept {
  if (lanes != nullptr) lanes->add(lane);
}

// Stockage du profil (voir l'en-tete) : coefficients et valeurs publiques sur des Point de [0, 2^B).
using DomainBudget = SpanBudgets<kCoordBits>;
using DotInt = Int<DomainBudget::dot>;
using CrossInt = Int<DomainBudget::cross>;
using DeterminantInt = Int<DomainBudget::determinant>;
using CenterInt = Int<DomainBudget::center_numerator>;
using CenterDen = Int<DomainBudget::center_denominator>;
using SideInt = Int<DomainBudget::side>;
static_assert(SpanBudgets<21>::center_numerator == 110 && SpanBudgets<24>::center_numerator == 125 &&
              SpanBudgets<32>::center_numerator == 165, "num : bornes des trois profils");

}  // namespace mhgp12::num
