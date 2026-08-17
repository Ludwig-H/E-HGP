// MorseHGP3D v4 — ARITHMETIQUE DE L'ORACLE, ecrite a neuf.
//
// REPRESENTATION VOLONTAIREMENT DIFFERENTE de la production : la production
// decide en __int128 sur des formes de Gram compactes ; l'oracle emploie un
// entier signe en signe-magnitude a limbes 64 bits, largeur fixe N limbes,
// construit par operations scolaires (addition avec retenue, produit long).
// Un defaut commun aux deux chemins ne peut donc pas se compenser.
//
// ECHEC FERME, JAMAIS UN SIGNAL (audit oracle du 17 aout, P0.1) : toute
// saturation de largeur leve un drapeau collant `overflow_flag()` et rend un
// resultat EMPOISONNE (tronque, sans signification). Le contrat d'usage est :
// l'appelant verifie `overflow_seen()` avant de convertir un verdict en
// statut ; drapeau leve => refus type (numeric_failure), jamais un verdict
// ni un abort. `cmake/run_expect.cmake` grave qu'un signal n'est pas un code.
//
// `inject_mul_carry_lost()` : point d'injection DE TEST (mutant « carry
// perdu dans le produit long », audit § 7.4). Il jette la retenue interne du
// produit aux positions i+j >= 2, donc ne corrompt que les grandeurs qui
// traversent reellement le quatrieme limbe et au-dela : le tuer PROUVE que
// les limbes hauts sont exerces. Jamais actif hors des portes qui le tuent.
#pragma once

#include <cstdint>
#include <cstdio>

// `__extension__` : __int128 n'est pas ISO, le build est -Wpedantic -Werror.
__extension__ typedef __int128 oi128;
__extension__ typedef unsigned __int128 ou128;

namespace mhgp4_oracle {

inline bool& overflow_flag() {
  static bool f = false;
  return f;
}
inline bool overflow_seen() { return overflow_flag(); }
inline void clear_overflow() { overflow_flag() = false; }
// Journalisation debrayable : le selftest provoque des debordements ATTENDUS
// par milliers ; le drapeau reste toujours leve, seul le message se tait.
inline bool& overflow_log() {
  static bool f = true;
  return f;
}
inline void note_overflow(const char* what) {
  if (!overflow_flag() && overflow_log())
    std::fprintf(stderr, "obigint: debordement de largeur (%s) — echec ferme\n",
                 what);
  overflow_flag() = true;
}

inline bool& inject_mul_carry_lost() {
  static bool f = false;
  return f;
}

template <int N>
struct OBig {
  static_assert(N >= 2, "OBig<N> : from_i128 remplit deux limbes ; N >= 2");
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
    r.w[1] = (std::uint64_t)(m >> 64);
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
    if (carry) note_overflow("add");
    return r;
  }

  // Precondition : |a| >= |b| — desormais VERIFIEE : un borrow final signale
  // la violation comme echec ferme au lieu de laisser un wrap silencieux.
  static OBig sub_mag(const OBig& a, const OBig& b) {
    OBig r;
    ou128 borrow = 0;
    for (int i = 0; i < N; ++i) {
      const ou128 d = (ou128)a.w[i] - b.w[i] - borrow;
      r.w[i] = (std::uint64_t)d;
      borrow = (d >> 64) ? 1 : 0;
    }
    if (borrow) note_overflow("sub precondition");
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
          if (b.w[j]) note_overflow("mul");
          continue;
        }
        const ou128 cur = (ou128)a.w[i] * b.w[j] + r.w[i + j] + carry;
        r.w[i + j] = (std::uint64_t)cur;
        carry = cur >> 64;
        if (inject_mul_carry_lost() && i + j >= 2) carry = 0;  // MUTANT
      }
      if (carry) note_overflow("mul fin");
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
