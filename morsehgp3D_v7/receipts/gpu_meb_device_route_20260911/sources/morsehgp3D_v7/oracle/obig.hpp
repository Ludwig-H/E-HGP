// MorseHGP3D v6 — ARITHMETIQUE DE L'ORACLE, ecrite a neuf (namespace
// mhgp7_oracle, jamais dans un chemin produit).
//
// REPRESENTATION VOLONTAIREMENT AUTRE que la production ET que la v4 :
//   - la production (src/core/wide.hpp) decide en __int128 et en limbes u64
//     non signes (U192/U320) avec des accumulateurs u128 ;
//   - l'oracle v4 employait des limbes u64 en signe-magnitude ;
//   - l'oracle v6 emploie des LIMBES DE 32 BITS (std::uint32_t) en
//     signe-magnitude, largeur fixe N limbes (OBig<12> = 384 bits), avec des
//     operations scolaires : addition/soustraction a retenue de 1 bit, produit
//     long O(N^2) dont chaque produit partiel 32x32 tient dans un u64 —
//     (2^32-1)^2 + (2^32-1) + (2^32-1) = 2^64 - 1 exactement : le u64 ne
//     deborde jamais, c'est la preuve de correction de la colonne.
// Un defaut commun aux deux chemins (production et oracle) ne peut donc pas
// se compenser : ni la largeur des limbes, ni la sequence des retenues, ni le
// type des accumulateurs ne coincident. `mhgp7_obig_selftest` juge ce juge
// contre __int128 (autorite materielle), contre une reconstruction par
// quatre produits 64x64 materiels, et contre boost::cpp_int si present.
//
// ECHEC FERME, JAMAIS UN SIGNAL : toute saturation de largeur leve un drapeau
// COLLANT global (`overflow_flag()`) et rend un resultat EMPOISONNE (tronque,
// sans signification). Contrat d'usage : l'appelant efface le drapeau
// (`clear_overflow()`) avant une campagne et le lit (`overflow_seen()`) avant
// de convertir un verdict en statut ; drapeau leve => refus (code 3,
// numeric_failure), jamais un verdict ni un abort. Le message de
// `note_overflow` est debrayable (`overflow_log()`) : le selftest provoque des
// debordements ATTENDUS par centaines, seul le message se tait, le drapeau
// reste leve.
//
// MUTANT `obig-carry-lost` (registre unique src/core/mutants.hpp, inclus ici
// pour le SEUL registre) : jette la retenue interne du produit long aux
// positions i+j >= 4 (limbes de 32 bits), c'est-a-dire les retenues sortant du
// cinquieme limbe (bits 128..159) et au-dela. Les magnitudes sont additives
// (jamais de compensation) : une retenue sortant de la position 4 implique un
// resultat vrai >= 2^160 ; tout produit < 2^160 reste EXACT sous le mutant.
// Le tuer prouve donc que les limbes hauts (>= 128 bits) sont reellement
// traverses par la porte. Jamais actif hors des portes qui le tuent.
//
// Toute grandeur ci-dessous est un entier ; il n'existe ni division ni
// flottant dans cette arithmetique : l'ordre des rationnels se decide par
// produits croises chez l'appelant.
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

#include "../src/core/mutants.hpp"

namespace mhgp7_oracle {

// `__extension__` : __int128 n'est pas ISO, le build est -Wpedantic -Werror.
__extension__ typedef __int128 oi128;
__extension__ typedef unsigned __int128 ou128;

inline bool& overflow_flag() {
  static bool f = false;
  return f;
}
inline bool overflow_seen() { return overflow_flag(); }
inline void clear_overflow() { overflow_flag() = false; }
inline bool& overflow_log() {
  static bool f = true;
  return f;
}
// Leve le drapeau collant ; n'imprime qu'a la premiere levee et si le journal
// est actif. Jamais un abort, jamais un signal.
inline void note_overflow(const char* what) {
  if (!overflow_flag() && overflow_log())
    std::fprintf(stderr, "obig: debordement de largeur (%s) — echec ferme\n", what);
  overflow_flag() = true;
}

template <int N>
struct OBig {
  static_assert(N >= 4, "OBig<N> : from_i128 remplit quatre limbes de 32 bits ; N >= 4");
  static constexpr int kLimbs = N;
  static constexpr int kBits = 32 * N;

  std::uint32_t w[N] = {};  // w[0] = poids faible
  bool neg = false;         // signe-magnitude ; zero a neg = false canonique

  // ---- constructeurs ------------------------------------------------------

  static OBig from_i64(std::int64_t v) { return from_i128((oi128)v); }

  static OBig from_i128(oi128 v) {
    OBig r;
    ou128 m;
    if (v < 0) {
      r.neg = true;
      m = (ou128)(-(v + 1)) + 1;  // |INT128_MIN| sans debordement signe
    } else {
      m = (ou128)v;
    }
    for (int i = 0; i < 4; ++i) r.w[i] = (std::uint32_t)(m >> (32 * i));
    r.canon();
    return r;
  }

  // Magnitude donnee par des MOTS de 64 bits (poids faible d'abord), signe
  // separe : conversion de representation des U192/U320 de la production
  // (jamais une decision). Mot non representable => drapeau, resultat tronque.
  static OBig from_u64_words(const std::uint64_t* words, int count, bool negative = false) {
    OBig r;
    for (int k = 0; k < count; ++k) {
      const int lo = 2 * k, hi = 2 * k + 1;
      const std::uint32_t wlo = (std::uint32_t)words[k];
      const std::uint32_t whi = (std::uint32_t)(words[k] >> 32);
      if (lo < N) r.w[lo] = wlo;
      else if (wlo) note_overflow("from_u64_words");
      if (hi < N) r.w[hi] = whi;
      else if (whi) note_overflow("from_u64_words");
    }
    r.neg = negative;
    r.canon();
    return r;
  }

  // 2^k exactement (k >= 0) ; k >= 32N => drapeau, zero empoisonne.
  static OBig pow2(int k) {
    OBig r;
    if (k < 0 || k >= kBits) {
      note_overflow("pow2");
      return r;
    }
    r.w[k / 32] = (std::uint32_t)1u << (k % 32);
    return r;
  }

  // ---- forme canonique et predicats --------------------------------------

  void canon() {
    if (is_zero()) neg = false;
  }

  bool is_zero() const {
    for (int i = 0; i < N; ++i)
      if (w[i]) return false;
    return true;
  }

  int sign() const {
    if (is_zero()) return 0;
    return neg ? -1 : 1;
  }

  // Index du limbe non nul le plus haut ; -1 pour zero.
  int top_limb() const {
    for (int i = N - 1; i >= 0; --i)
      if (w[i]) return i;
    return -1;
  }

  // Nombre de bits de la magnitude (0 pour zero).
  int bit_length() const {
    const int t = top_limb();
    if (t < 0) return 0;
    int b = 0;
    std::uint32_t x = w[t];
    while (x) {
      ++b;
      x >>= 1;
    }
    return 32 * t + b;
  }

  OBig abs() const {
    OBig r = *this;
    r.neg = false;
    return r;
  }

  // Conversion vers i128 si la valeur tient (sinon false, *out inchange).
  bool to_i128(oi128* out) const {
    if (top_limb() >= 4) return false;
    ou128 m = 0;
    for (int i = 3; i >= 0; --i) m = (m << 32) | w[i];
    const ou128 half = (ou128)1 << 127;
    if (neg) {
      if (m > half) return false;
      if (m == half) {
        *out = -(oi128)(half - 1) - 1;  // INT128_MIN
        return true;
      }
      *out = -(oi128)m;
      return true;
    }
    if (m >= half) return false;
    *out = (oi128)m;
    return true;
  }

  // Hexadecimal signe ("-0x..." / "0x..."), sans zeros de tete ("0x0" pour zero).
  std::string hex() const {
    static const char* digits = "0123456789abcdef";
    std::string s;
    const int t = top_limb();
    if (t < 0) return "0x0";
    bool started = false;
    for (int i = t; i >= 0; --i)
      for (int nib = 7; nib >= 0; --nib) {
        const int d = (int)((w[i] >> (4 * nib)) & 0xF);
        if (!started && d == 0) continue;
        started = true;
        s.push_back(digits[d]);
      }
    return std::string(neg ? "-0x" : "0x") + s;
  }

  // ---- magnitudes ---------------------------------------------------------

  static int cmp_mag(const OBig& a, const OBig& b) {
    for (int i = N - 1; i >= 0; --i)
      if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
    return 0;
  }

  static OBig add_mag(const OBig& a, const OBig& b) {
    OBig r;
    std::uint64_t carry = 0;
    for (int i = 0; i < N; ++i) {
      const std::uint64_t s = (std::uint64_t)a.w[i] + b.w[i] + carry;
      r.w[i] = (std::uint32_t)s;
      carry = s >> 32;
    }
    if (carry) note_overflow("add");
    return r;
  }

  // Precondition |a| >= |b|, VERIFIEE : un emprunt final est un echec ferme.
  static OBig sub_mag(const OBig& a, const OBig& b) {
    OBig r;
    std::uint64_t borrow = 0;
    for (int i = 0; i < N; ++i) {
      const std::uint64_t d = (std::uint64_t)a.w[i] - b.w[i] - borrow;
      r.w[i] = (std::uint32_t)d;
      borrow = (d >> 32) ? 1 : 0;  // d < 0 en u64 : bits hauts a un
    }
    if (borrow) note_overflow("sub precondition");
    return r;
  }

  // ---- operateurs signes --------------------------------------------------

  friend OBig operator+(const OBig& a, const OBig& b) {
    OBig r;
    if (a.neg == b.neg) {
      r = add_mag(a, b);
      r.neg = a.neg;
    } else if (cmp_mag(a, b) >= 0) {
      r = sub_mag(a, b);
      r.neg = a.neg;
    } else {
      r = sub_mag(b, a);
      r.neg = b.neg;
    }
    r.canon();
    return r;
  }

  OBig operator-() const {
    OBig r = *this;
    r.neg = !r.neg;
    r.canon();
    return r;
  }

  friend OBig operator-(const OBig& a, const OBig& b) { return a + (-b); }

  // Produit long scolaire : colonne i+j, accumulateur u64 exact (voir en-tete).
  friend OBig operator*(const OBig& a, const OBig& b) {
    const bool carry_lost = MHGP7_MUTANT("obig-carry-lost");
    OBig r;
    for (int i = 0; i < N; ++i) {
      if (!a.w[i]) continue;
      std::uint64_t carry = 0;
      for (int j = 0; j < N; ++j) {
        const int pos = i + j;
        if (pos >= N) {
          if (b.w[j]) note_overflow("mul");
          continue;
        }
        const std::uint64_t cur = (std::uint64_t)a.w[i] * b.w[j] + r.w[pos] + carry;
        r.w[pos] = (std::uint32_t)cur;
        carry = cur >> 32;
        if (carry_lost && pos >= 4) carry = 0;  // MUTANT obig-carry-lost
      }
      if (carry) note_overflow("mul fin");
    }
    r.neg = a.neg != b.neg;
    r.canon();
    return r;
  }

  OBig& operator+=(const OBig& o) { return *this = *this + o; }
  OBig& operator-=(const OBig& o) { return *this = *this - o; }
  OBig& operator*=(const OBig& o) { return *this = *this * o; }

  // -1 / 0 / +1 selon le signe de a - b, sans arithmetique.
  friend int cmp(const OBig& a, const OBig& b) {
    if (a.neg != b.neg) return a.neg ? -1 : 1;
    const int m = cmp_mag(a, b);
    return a.neg ? -m : m;
  }

  friend bool operator==(const OBig& a, const OBig& b) { return cmp(a, b) == 0; }
  friend bool operator!=(const OBig& a, const OBig& b) { return cmp(a, b) != 0; }
  friend bool operator<(const OBig& a, const OBig& b) { return cmp(a, b) < 0; }
  friend bool operator>(const OBig& a, const OBig& b) { return cmp(a, b) > 0; }
  friend bool operator<=(const OBig& a, const OBig& b) { return cmp(a, b) <= 0; }
  friend bool operator>=(const OBig& a, const OBig& b) { return cmp(a, b) >= 0; }
};

// Largeur de reference de l'oracle : 384 bits (produits croises q4 < 2^260,
// carres de |N'| < 2^292 pour les re-derivations, marge > 2^90).
using OBig384 = OBig<12>;

}  // namespace mhgp7_oracle

