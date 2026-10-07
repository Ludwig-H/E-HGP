// Rationnel exact reduit sur num::Big (tranche S8) : numerateur et denominateur premiers entre eux, denominateur
// strictement positif, zero ecrit 0/1. Meme valeur et meme forme que fractions.Fraction de Python. Chaque operation
// rend un Outcome : radical_sign_budget si un produit intermediaire depasse la capacite de Big, arithmetic_invariant
// pour une division par zero.
#pragma once

#include "num/big.hpp"
#include "num/level.hpp"

namespace mhgp12::num {

class Rational {
 public:
  Rational() noexcept : den_(Big::from_u64(1)) {}
  static Rational from_i64(i64 value) noexcept {
    Rational out;
    out.num_ = Big::from_i64(value);
    return out;
  }
  static Rational from_big(const Big& value) noexcept {
    Rational out;
    out.num_.assign(value);
    return out;
  }
  // numerator / denominator reduit ; denominateur nul : arithmetic_invariant.
  [[nodiscard]] static Outcome make(const Big& numerator, const Big& denominator, Rational& out) noexcept;
  // Niveau N / D non reduit du catalogue, reduit ici.
  [[nodiscard]] static Outcome from_level(const Level& level, Rational& out) noexcept;

  const Big& numerator() const noexcept { return num_; }
  const Big& denominator() const noexcept { return den_; }
  int sign() const noexcept { return num_.sign(); }
  bool is_zero() const noexcept { return num_.is_zero(); }
  void negate() noexcept { num_.negate(); }
  void assign(const Rational& other) noexcept {
    num_.assign(other.num_);
    den_.assign(other.den_);
  }

 private:
  friend Outcome add(const Rational&, const Rational&, Rational&) noexcept;
  friend Outcome subtract(const Rational&, const Rational&, Rational&) noexcept;
  friend Outcome multiply(const Rational&, const Rational&, Rational&) noexcept;
  friend Outcome divide(const Rational&, const Rational&, Rational&) noexcept;
  Big num_;
  Big den_;
};

// out peut designer un operande.
[[nodiscard]] Outcome add(const Rational& a, const Rational& b, Rational& out) noexcept;
[[nodiscard]] Outcome subtract(const Rational& a, const Rational& b, Rational& out) noexcept;
[[nodiscard]] Outcome multiply(const Rational& a, const Rational& b, Rational& out) noexcept;
// b nul : arithmetic_invariant.
[[nodiscard]] Outcome divide(const Rational& a, const Rational& b, Rational& out) noexcept;
// Signe de a - b dans order.
[[nodiscard]] Outcome compare(const Rational& a, const Rational& b, int& order) noexcept;

}  // namespace mhgp12::num
