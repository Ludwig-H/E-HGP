// Sommes de radicaux a radicandes entiers et comparaisons exactes de racines (tranche S8, specification paragraphes
// 7.6 et 7.8). Port EXPLICITE, decision par decision, de bench/points_radius.py (sqrt_diff_cmp, sqrt_cmp2,
// sqrt_bounds, square_ratio, radical_classes, sign_of_radicals ; epingle dans docs/PROVENANCE.md) avec la signature
// de classe de bench/points_flat.py (class_signature, group_classes) comme accelerateur, jamais comme decision.
//
// Forme : une somme de c_j sqrt(N_j), c_j rationnels, N_j entiers >= 0. Un terme c sqrt(n / d) de Python s'ecrit
// (c / d) sqrt(n d) : memes classes, memes encadrements, memes decisions. Deux radicandes sont de la meme classe si et
// seulement si N_1 N_2 est un carre parfait (racines de classes distinctes lineairement independantes sur Q) :
// l'egalite est certifiee si et seulement si tous les coefficients de classe s'annulent. Une somme non nulle est
// separee par encadrements entiers isqrt(N 4^b) / 2^b, b = 96, 192, ..., 6144 (b <= 8192) ; au-dela, ou si un
// encadrement depasse la capacite de Big, refus radical_sign_budget, jamais une egalite supposee.
//
// Budget declare (ARCHITECTURE.md de la v11, paragraphe 3) : au plus kMaxTerms termes par somme, precision au plus
// kSignBudgetBits, entiers au plus kBigCapacityBits bits. Les termes et les classes vivent dans des Buffer comptes au
// budget de l'appelant (un RadicalSum par fil) ; les temporaires (quelques Big) vivent sur la pile.
#pragma once

#include <array>

#include "core/core.hpp"
#include "num/rational.hpp"

namespace mhgp12::num {

inline constexpr u32 kSignatureCount = 40;

class RadicalSum {
 public:
  static constexpr u32 kMaxTerms = 16;
  static constexpr u32 kFirstBits = 96;
  static constexpr u32 kSignBudgetBits = 8192;

  struct Term {
    Rational coef;
    Big radicand;
  };
  struct Class {
    Rational coef;
    Big rep;
    std::array<u8, kSignatureCount> signature;
  };
  // Trace de la derniere decision : classes non nulles, precision de l'encadrement qui a tranche (0 sans encadrement).
  struct Trace {
    u32 classes = 0;
    u32 bits = 0;
  };

  RadicalSum() = default;
  // Octets reserves par make pour `capacity` termes : formule d'admission.
  static constexpr u64 bytes(u64 capacity = kMaxTerms) noexcept { return capacity * (sizeof(Term) + sizeof(Class)); }
  // kMaxTerms termes : le budget declare des comparaisons de dates et de la table des racines.
  [[nodiscard]] static Result<RadicalSum> make(MemoryBudget& budget) noexcept;
  // Capacite explicite (tete plate, tranche S10 : une decision EOM somme les plateaux d'un sous-arbre condense) : au
  // plus `capacity` termes, refus radical_sign_budget au-dela ; capacite nulle : parameter_out_of_range.
  [[nodiscard]] static Result<RadicalSum> make(MemoryBudget& budget, u32 capacity) noexcept;
  u32 capacity() const noexcept { return static_cast<u32>(terms_.size()); }

  void clear() noexcept { count_ = 0; }
  u32 size() const noexcept { return count_; }
  // coef sqrt(radicand), radicande rationnel >= 0 (sinon arithmetic_invariant) : (coef / d) sqrt(n d).
  [[nodiscard]] Outcome add(const Rational& coef, const Rational& radicand) noexcept;
  // coef sqrt(radicand), radicande entier >= 0. Au-dela de kMaxTerms termes : radical_sign_budget.
  [[nodiscard]] Outcome add_integer(const Rational& coef, const Big& radicand) noexcept;
  // Signe exact de la somme (port de sign_of_radicals) : 0 seulement si l'egalite est certifiee.
  [[nodiscard]] Outcome sign(int& out) noexcept;
  const Trace& trace() const noexcept { return trace_; }

 private:
  Outcome group(u32& classes) noexcept;
  Outcome refine(u32 classes, int& out) noexcept;
  Buffer<Term> terms_;
  Buffer<Class> classes_;
  u32 count_ = 0;
  Trace trace_{};
};

// Signature de classe de carres d'un entier N > 0 : parite de la valuation et caractere quadratique de la partie
// inversible pour les 40 premiers premiers de bench/points_flat.py (modulo 8 pour 2). Meme classe => meme signature ;
// la reciproque n'est jamais supposee.
std::array<u8, kSignatureCount> class_signature(const Big& n) noexcept;

// Signe exact de sqrt(x) - sqrt(y) - u, x, y >= 0 (port de sqrt_diff_cmp, bench/points_radius.py:33-45).
[[nodiscard]] Outcome sqrt_diff_cmp(const Rational& x, const Rational& y, const Rational& u, int& out) noexcept;
// Signe exact de (sqrt a + sqrt b) - (sqrt c + sqrt d), a, b, c, d >= 0 (port de sqrt_cmp2, :48-50).
[[nodiscard]] Outcome sqrt_cmp2(const Rational& a, const Rational& b, const Rational& c, const Rational& d,
                                int& out) noexcept;
// Ordre exact de deux dates sqrt(t) + sqrt(m) - sqrt(q) (port de RValue.cmp, bench/points_radius.py:148-150) : trois
// racines contre trois par sign_of_radicals. sum recoit les six termes.
[[nodiscard]] Outcome compare_dates(const std::array<Rational, 3>& first, const std::array<Rational, 3>& second,
                                    RadicalSum& sum, int& out) noexcept;

}  // namespace mhgp12::num
