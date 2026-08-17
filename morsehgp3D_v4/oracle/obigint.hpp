// MorseHGP3D v4 — ARITHMETIQUE DE L'ORACLE, ecrite a neuf.
//
// REPRESENTATION VOLONTAIREMENT DIFFERENTE de la production : la production
// decide en __int128 sur des formes de Gram compactes ; l'oracle emploie un
// entier signe en signe-magnitude a limbes 64 bits, largeur fixe N limbes,
// construit par operations scolaires (addition avec retenue, produit long).
// Un defaut commun aux deux chemins ne peut donc pas se compenser.
//
// Toute saturation de largeur est un ABORT (jamais un wrap silencieux) : la
// largeur employee par chaque predicat de l'oracle est dimensionnee dans le
// test qui l'utilise, et l'abort vaut refus, pas verdict.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

// `__extension__` : __int128 n'est pas ISO, le build est -Wpedantic -Werror.
__extension__ typedef __int128 oi128;
__extension__ typedef unsigned __int128 ou128;

namespace mhgp4_oracle {

template <int N>
struct OBig {
  std::uint64_t w[N] = {};
  bool neg = false;  // signe-magnitude ; zero a neg=false canonique

  static OBig from_i128(oi128 v) {
    OBig r;
    ou128 m;
    if (v < 0) {
      r.neg = true;
      m = (ou128)(-(v + 1)) + 1;
    } else {
      m = (ou128)v;
    }
    r.w[0] = (std::uint64_t)m;
    if constexpr (N > 1) r.w[1] = (std::uint64_t)(m >> 64);
    r.canon();
    return r;
  }

  void canon() {
    bool zero = true;
    for (int i = 0; i < N; ++i)
      if (w[i]) { zero = false; break; }
    if (zero) neg = false;
  }

  bool is_zero() const {
    for (int i = 0; i < N; ++i)
      if (w[i]) return false;
    return true;
  }

  static int cmp_mag(const OBig& a, const OBig& b) {
    for (int i = N - 1; i >= 0; --i) {
      if (a.w[i] != b.w[i]) return a.w[i] < b.w[i] ? -1 : 1;
    }
    return 0;
  }

  static OBig add_mag(const OBig& a, const OBig& b) {
    OBig r;
    ou128 carry = 0;
    for (int i = 0; i < N; ++i) {
      const ou128 s = (ou128)a.w[i] + b.w[i] + carry;
      r.w[i] = (std::uint64_t)s;
      carry = s >> 64;
    }
    if (carry) {
      std::fprintf(stderr, "obigint: debordement de largeur (add)\n");
      std::abort();
    }
    return r;
  }

  // Precondition : |a| >= |b|.
  static OBig sub_mag(const OBig& a, const OBig& b) {
    OBig r;
    ou128 borrow = 0;
    for (int i = 0; i < N; ++i) {
      const ou128 d = (ou128)a.w[i] - b.w[i] - borrow;
      r.w[i] = (std::uint64_t)d;
      borrow = (d >> 64) ? 1 : 0;
    }
    return r;
  }

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

  friend OBig operator-(const OBig& a, const OBig& b) {
    OBig nb = b;
    nb.neg = !nb.neg;
    return a + nb;
  }

  friend OBig operator*(const OBig& a, const OBig& b) {
    OBig r;
    for (int i = 0; i < N; ++i) {
      if (!a.w[i]) continue;
      ou128 carry = 0;
      for (int j = 0; j < N; ++j) {
        if (i + j >= N) {
          if (b.w[j]) {
            std::fprintf(stderr, "obigint: debordement de largeur (mul)\n");
            std::abort();
          }
          continue;
        }
        const ou128 cur = (ou128)a.w[i] * b.w[j] + r.w[i + j] + carry;
        r.w[i + j] = (std::uint64_t)cur;
        carry = cur >> 64;
      }
      if (carry) {
        std::fprintf(stderr, "obigint: debordement de largeur (mul fin)\n");
        std::abort();
      }
    }
    r.neg = a.neg != b.neg;
    r.canon();
    return r;
  }

  // -1 / 0 / +1 selon le signe de a - b.
  friend int cmp(const OBig& a, const OBig& b) {
    if (a.neg != b.neg) return a.neg ? -1 : 1;
    const int m = cmp_mag(a, b);
    return a.neg ? -m : m;
  }

  int sign() const {
    if (is_zero()) return 0;
    return neg ? -1 : 1;
  }
};

}  // namespace mhgp4_oracle
