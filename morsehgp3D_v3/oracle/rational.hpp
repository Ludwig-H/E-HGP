// MorseHGP3D v3 — rationnel exact de l'ORACLE.
//
// Invariant : le denominateur est strictement positif. Le rationnel n'est PAS
// mis sous forme irreductible ; seule la part en puissances de deux est retiree,
// ce qui ne demande que des decalages. C'est suffisant pour contenir la
// croissance de l'elimination de GAUSS sur trois inconnues, et cela evite
// d'ecrire une division generale et un PGCD dans le composant dont depend tout
// le jugement.
//
// Consequences voulues :
//   - la division de deux rationnels est une multiplication croisee ;
//   - la comparaison de deux rationnels est un produit croise ;
//   - aucune operation ne peut deborder, la precision etant arbitraire.
#pragma once

#include <cstdio>
#include <cstdlib>
#include <utility>

#include "bigint.hpp"

namespace mhgp3v {

class Rational {
 public:
  Rational() : numerator_(0), denominator_(1) {}
  explicit Rational(BigInt n) : numerator_(std::move(n)), denominator_(1) {}
  Rational(BigInt n, BigInt d) : numerator_(std::move(n)), denominator_(std::move(d)) {
    // FAIL-CLOSED. Un juge ne normalise pas une entree impossible : 0/0 devenait
    // silencieusement 0/1 et 1/0 gardait un denominateur de signe nul, si bien
    // qu'une sphere sujet malformee pouvait se lire comme un niveau valide.
    if (denominator_.is_zero()) {
      std::fprintf(stderr, "ECHEC ORACLE : rationnel de denominateur nul\n");
      std::abort();
    }
    normalise_sign_and_powers_of_two();
  }

  const BigInt& numerator() const { return numerator_; }
  const BigInt& denominator() const { return denominator_; }
  bool is_zero() const { return numerator_.is_zero(); }
  int sign() const { return numerator_.sign(); }

  friend Rational operator+(const Rational& a, const Rational& b) {
    return Rational(a.numerator_ * b.denominator_ + b.numerator_ * a.denominator_,
                    a.denominator_ * b.denominator_);
  }
  friend Rational operator-(const Rational& a, const Rational& b) {
    return Rational(a.numerator_ * b.denominator_ - b.numerator_ * a.denominator_,
                    a.denominator_ * b.denominator_);
  }
  friend Rational operator*(const Rational& a, const Rational& b) {
    return Rational(a.numerator_ * b.numerator_, a.denominator_ * b.denominator_);
  }
  // Division = multiplication croisee, avec garde : le diviseur nul est une
  // faute, pas une convention.
  friend Rational operator/(const Rational& a, const Rational& b) {
    if (b.numerator_.is_zero()) {
      std::fprintf(stderr, "ECHEC ORACLE : division rationnelle par zero\n");
      std::abort();
    }
    return Rational(a.numerator_ * b.denominator_, a.denominator_ * b.numerator_);
  }

  // Ordre total : les deux denominateurs sont strictement positifs, donc le
  // signe du produit croise donne l'ordre sans division.
  friend int compare(const Rational& a, const Rational& b) {
    return (a.numerator_ * b.denominator_ - b.numerator_ * a.denominator_).sign();
  }
  friend bool operator<(const Rational& a, const Rational& b) { return compare(a, b) < 0; }
  friend bool operator>(const Rational& a, const Rational& b) { return compare(a, b) > 0; }
  friend bool operator==(const Rational& a, const Rational& b) { return compare(a, b) == 0; }

  // Largeur observee, pour le recu : elle documente le domaine reellement
  // exerce plutot que de le supposer.
  std::size_t widest_bit_length() const {
    const std::size_t a = numerator_.bit_length();
    const std::size_t b = denominator_.bit_length();
    return a > b ? a : b;
  }

  double to_double() const;  // publication seulement, jamais une decision

 private:
  void normalise_sign_and_powers_of_two() {
    if (denominator_.sign() < 0) {
      numerator_ = -numerator_;
      denominator_ = -denominator_;
    }
    if (numerator_.is_zero()) { denominator_ = BigInt(1); return; }  // 0/d == 0/1
    const std::size_t shift = std::min(numerator_.trailing_zero_bits(),
                                       denominator_.trailing_zero_bits());
    if (shift != 0) {
      numerator_ = numerator_.shifted_right(shift);
      denominator_ = denominator_.shifted_right(shift);
    }
  }

  BigInt numerator_;
  BigInt denominator_;  // > 0
};

}  // namespace mhgp3v
