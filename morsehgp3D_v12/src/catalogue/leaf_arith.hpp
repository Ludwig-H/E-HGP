// Arithmetiques de la feuille J3 en repere local (docs/CONTRAT_NUMERIQUE.md, paragraphes 2 et 3 ;
// docs/CONTRAT_CATALOGUE.md, paragraphe 3). Le repere d'une feuille est E = fermeture de sa boite de centres et tous
// les sites de sa liste (NUM-COUVERTURE) : coin minimal o, etendue s (num::Frame). Toutes les coordonnees de la feuille
// sont lues en local (x - o) : chaque predicat ne lit que des differences, donc rend la meme decision qu'en coordonnees
// absolues, et ses bornes sont celles des budgets en s (num::SpanBudgets<s>).
//
// Deux politiques, une seule source J3 (leaf_predicates.hpp, leaf_common.hpp, leaf_census.hpp, leaf_j3.hpp) :
//   Narrow : palier etroit s <= 16 ; Tiny = i32, Small = i64, Large = i128, toutes natives. C'est l'arithmetique de la
//            feuille de MES-M2 (voie du levier C de la v11), en repere local ; a s <= 16 chaque expression tient dans
//            son type sans certificat (bornes ci-dessous), donc aucune feuille etroite n'est << non resolue >>.
//   Exact  : toute etendue s <= 33 ; Tiny = i64, Small = i128, Large = WideExact (320 bits, drapeau de depassement
//            collant). C'est le REPLI EXACT de la feuille : meme structure J3, operations plus larges, jamais un second
//            algorithme. La plus large expression vaut 7s+9 = 240 bits a s = 33 (orientation avec centre) ; un produit
//            d'intermediaires n'y depasse pas 238 bits : 320 bits suffisent, le drapeau collant n'est qu'une defense
//            (lu en fin de predicat, il rend la feuille invariant_violated).
// Bornes de la politique etroite (M = 2^s, s <= 16, coordonnees locales dans [0, 2^s], differences < M) :
//   dominance |y|^2-|x|^2 - 2 c.(y-x) < 9 M^2 (i64) ; droite J2 : |f|,|g| < M (i32), p0,p1 < 3 M^2, gauche et droite
//   < 6 M^3 (i64) ; q3 : produits scalaires < 3 M^2, t < 6 M^3 (i64), N < 24 M^5, D < 24 M^4 (i128) ; centre dans la
//   boite N + D(a - borne) < 48 M^5 ; cote 6s+8 = 104 bits ; orientation avec centre 7s+9 = 121 bits ; q4 : det
//   < 6 M^3 (i64), N < 18 M^4, h = 2 det^2 < 72 M^6, poids < 2^(6s+9) (i128) ; milieu D (a+b) < 48 M^5.
#pragma once

#include "catalogue/simt.hpp"
#include "num/num.hpp"

namespace mhgp12::catalogue_detail {

// Entier signe exact de 320 bits, a drapeau de depassement collant (hote seulement, feuilles a s > 16).
struct WideExact {
  num::Wide<5> value{};
  bool overflow = false;
  WideExact() noexcept = default;
  // Conversions voulues : les formules s'ecrivent une fois pour les types natifs et pour celui-ci.
  WideExact(i32 v) noexcept { set(i128{v}); }
  WideExact(i64 v) noexcept { set(i128{v}); }
  WideExact(i128 v) noexcept { set(v); }
  void set(i128 v) noexcept {
    const bool negative = v < 0;
    const u128 magnitude = negative ? u128{0} - static_cast<u128>(v) : static_cast<u128>(v);
    value = num::Wide<5>{};
    value.words[0] = static_cast<u64>(magnitude);
    value.words[1] = static_cast<u64>(magnitude >> 64);
    value.neg = negative && magnitude != 0;
    overflow = false;
  }
  friend WideExact operator+(const WideExact& a, const WideExact& b) noexcept {
    WideExact out;
    out.overflow = a.overflow || b.overflow || !num::add(a.value, b.value, out.value);
    return out;
  }
  friend WideExact operator-(const WideExact& a, const WideExact& b) noexcept {
    WideExact out;
    out.overflow = a.overflow || b.overflow || !num::subtract(a.value, b.value, out.value);
    return out;
  }
  friend WideExact operator*(const WideExact& a, const WideExact& b) noexcept {
    WideExact out;
    out.overflow = a.overflow || b.overflow || !num::multiply_into(a.value, b.value, out.value);
    return out;
  }
  WideExact operator-() const noexcept {
    WideExact out = *this;
    out.value = value.negated();
    return out;
  }
};

// Signe et sante d'une valeur : natifs pour les types natifs, exacts pour WideExact.
MHGP12_HD int sign_of(i32 v) { return (v > 0) - (v < 0); }
MHGP12_HD int sign_of(i64 v) { return (v > 0) - (v < 0); }
MHGP12_HD int sign_of(i128 v) { return (v > 0) - (v < 0); }
inline int sign_of(const WideExact& v) noexcept { return v.value.sign(); }
MHGP12_HD bool healthy(i128) { return true; }
inline bool healthy(const WideExact& v) noexcept { return !v.overflow; }

// Politiques de la feuille (voir l'en-tete).
struct Narrow {
  using Tiny = i32;
  using Small = i64;
  using Large = i128;
};
struct Exact {
  using Tiny = i64;
  using Small = i128;
  using Large = WideExact;
};

// Plus grande etendue de la politique etroite : le palier etroit du contrat numerique.
inline constexpr int kLeafNarrowSpan = num::kNarrowSpan;
static_assert(num::SpanBudgets<kLeafNarrowSpan>::center_orientation <= 127 &&
              num::SpanBudgets<kLeafNarrowSpan>::side <= 127 &&
              num::SpanBudgets<kLeafNarrowSpan>::numerator3 <= 127 && 3 * kLeafNarrowSpan + 3 <= 63,
              "feuille : la politique etroite tient dans les types natifs jusqu'a s = 16");
static_assert(num::SpanBudgets<num::kMaxSpan>::center_orientation <= 320 - 64,
              "feuille : 320 bits couvrent l'orientation avec centre a s = 33 avec marge");

}  // namespace mhgp12::catalogue_detail
