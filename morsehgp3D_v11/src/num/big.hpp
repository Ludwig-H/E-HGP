// Entier signe-magnitude a longueur utile (tranche S8, specification paragraphe 7.8). Les mots vivent dans un tableau
// fixe de kBigWords mots, mais chaque operation ne parcourt que les mots utiles : la capacite de 16384 + 1024 bits
// n'est qu'un plafond de refus. Tout resultat exact qui la depasserait rend radical_sign_budget (resource_exhausted),
// jamais une valeur tronquee. Aucune exception, aucune allocation : un Big vit sur la pile de l'appelant ou dans un
// tampon compte au budget (RadicalSum).
//
// Semantique des operations : celle de l'int de Python, que la porte mhgp11_num_big compare sur plus de 20 000
// operations (python3 -S) : division et reste par defaut (floor), reste du signe du diviseur ; pgcd positif ou nul ;
// reste modulo un petit entier dans [0, p) ; decalage a droite de la magnitude (troncature vers zero, signe garde).
// La racine entiere recoit une proposition binary64 et la certifie en entier (s^2 <= a < (s+1)^2) : le flottant
// propose, l'entier decide (regle F1, docs/ARCHITECTURE.md paragraphe 4).
#pragma once

#include <optional>
#include <span>

#include "num/wide.hpp"

namespace mhgp11::num {

inline constexpr u32 kBigCapacityBits = 16384 + 1024;
inline constexpr u32 kBigWords = kBigCapacityBits / 64;
static_assert(kBigWords == 272, "num : capacite de Big, 272 mots");

class Big {
 public:
  // Zero. Les mots au-dela de size() ne sont jamais lus.
  Big() noexcept {}

  static Big from_u64(u64 value) noexcept;
  static Big from_i64(i64 value) noexcept;
  static Big from_u128(u128 value) noexcept;
  static Big from_i128(i128 value) noexcept;
  template <int Words>
  static Big from_wide(const Wide<Words>& value) noexcept {
    static_assert(Words <= static_cast<int>(kBigWords), "num : Wide plus large que Big");
    Big out;
    for (int i = 0; i < Words; ++i) out.w_[i] = value.words[i];
    out.len_ = Words;
    out.neg_ = value.neg;
    out.trim();
    return out;
  }

  // Mots utiles : le mot de rang size() - 1 est non nul ; zero a size() == 0.
  u32 size() const noexcept { return len_; }
  u64 word(u32 i) const noexcept { return w_[i]; }
  bool negative() const noexcept { return neg_; }
  bool is_zero() const noexcept { return len_ == 0; }
  int sign() const noexcept { return len_ == 0 ? 0 : (neg_ ? -1 : 1); }
  u32 bit_length() const noexcept;
  bool is_one() const noexcept { return len_ == 1 && w_[0] == 1 && !neg_; }

  // Copie des seuls mots utiles (la copie implicite copie tout le tableau).
  void assign(const Big& other) noexcept;
  void negate() noexcept { neg_ = !neg_ && len_ != 0; }
  void set_negative(bool negative) noexcept { neg_ = negative && len_ != 0; }
  // Magnitude donnee par mots petit-boutistes, zeros de tete admis ; refus radical_sign_budget au-dela de la capacite.
  [[nodiscard]] Outcome assign_words(std::span<const u64> words, bool negative) noexcept;
  // Magnitude en u128 si elle tient (signe ignore).
  std::optional<u128> magnitude_u128() const noexcept;

 private:
  friend struct BigAccess;
  void trim() noexcept {
    while (len_ != 0 && w_[len_ - 1] == 0) --len_;
    if (len_ == 0) neg_ = false;
  }
  u32 len_ = 0;
  bool neg_ = false;
  u64 w_[kBigWords];
};

int compare(const Big& a, const Big& b) noexcept;
int compare_magnitude(const Big& a, const Big& b) noexcept;

// out peut designer a ou b. Refus radical_sign_budget si le resultat exact depasse la capacite ; out est alors
// indetermine.
[[nodiscard]] Outcome add(const Big& a, const Big& b, Big& out) noexcept;
[[nodiscard]] Outcome subtract(const Big& a, const Big& b, Big& out) noexcept;
// Multiplication d'ecole ; out peut designer a ou b.
[[nodiscard]] Outcome multiply(const Big& a, const Big& b, Big& out) noexcept;
[[nodiscard]] Outcome shift_left(const Big& a, u32 bits, Big& out) noexcept;
// |a| >> bits, signe garde (zero sans signe) ; out peut designer a.
void shift_right(const Big& a, u32 bits, Big& out) noexcept;
// Division par defaut de Python (Knuth D sur les magnitudes) : q = floor(a / b), r = a - q b. Diviseur nul :
// arithmetic_invariant. q et r distincts entre eux ; chacun peut designer a ou b.
[[nodiscard]] Outcome divide(const Big& a, const Big& b, Big& quotient, Big& remainder) noexcept;
// PGCD binaire des magnitudes, positif ou nul ; out peut designer a ou b.
[[nodiscard]] Outcome gcd(const Big& a, const Big& b, Big& out) noexcept;
// Racine entiere floor(sqrt(a)), a >= 0 (sinon arithmetic_invariant) : proposition binary64, iterations de Newton
// entieres, puis certificat s^2 <= a < (s+1)^2 ; un certificat faux est arithmetic_invariant.
[[nodiscard]] Outcome isqrt(const Big& a, Big& out) noexcept;
// Carre parfait : square vaut vrai si et seulement si a = root^2 (root = isqrt(a)) ; a < 0 n'est pas un carre.
[[nodiscard]] Outcome perfect_square(const Big& a, bool& square, Big& root) noexcept;
// a mod p dans [0, p) (convention de Python), p >= 1.
u32 residue(const Big& a, u32 p) noexcept;
// Division exacte par un petit entier p >= 1 (le reste est rendu) ; out peut designer a.
u32 divide_small(const Big& a, u32 p, Big& out) noexcept;
// Nombre de zeros de poids faible de la magnitude (0 pour zero).
u32 trailing_zeros(const Big& a) noexcept;

}  // namespace mhgp11::num
