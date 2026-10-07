// Types entiers du juge JUG-EMST et entiers naturels de precision arbitraire (mots de 64 bits, petit-boutiste) :
// juste ce qu'il faut pour lire et comparer exactement les rationnels d'un vidage MHGP11FUL1 (produit, comparaison,
// ecriture decimale). Aucun flottant, aucune dependance au produit ni a la v11.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mhgp12_emst {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;
__extension__ typedef unsigned __int128 u128;

constexpr u32 kAucun = 0xFFFFFFFFu;  // NONE des vidages (parent de la racine) et sentinelle d'indice

// Entier naturel : mots petit-boutistes, sans mot de poids fort nul ; zero = aucun mot.
struct Naturel {
  std::vector<u64> mots;

  void normaliser() {
    while (!mots.empty() && mots.back() == 0) mots.pop_back();
  }
  bool nul() const { return mots.empty(); }
};

inline Naturel naturel(u128 valeur) {
  Naturel n;
  while (valeur != 0) {
    n.mots.push_back(static_cast<u64>(valeur));
    valeur >>= 64;
  }
  return n;
}

// Produit exact (schoolbook) : chaque terme a_i * b_j + r + retenue tient dans 128 bits.
inline Naturel produit(const Naturel& a, const Naturel& b) {
  Naturel r;
  if (a.nul() || b.nul()) return r;
  r.mots.assign(a.mots.size() + b.mots.size(), 0);
  for (std::size_t i = 0; i < a.mots.size(); ++i) {
    u128 retenue = 0;
    for (std::size_t j = 0; j < b.mots.size(); ++j) {
      const u128 t = static_cast<u128>(a.mots[i]) * b.mots[j] + r.mots[i + j] + retenue;
      r.mots[i + j] = static_cast<u64>(t);
      retenue = t >> 64;
    }
    for (std::size_t k = i + b.mots.size(); retenue != 0; ++k) {
      const u128 t = static_cast<u128>(r.mots[k]) + retenue;
      r.mots[k] = static_cast<u64>(t);
      retenue = t >> 64;
    }
  }
  r.normaliser();
  return r;
}

// -1, 0 ou 1 selon a < b, a = b, a > b.
inline int comparer(const Naturel& a, const Naturel& b) {
  if (a.mots.size() != b.mots.size()) return a.mots.size() < b.mots.size() ? -1 : 1;
  for (std::size_t i = a.mots.size(); i-- > 0;) {
    if (a.mots[i] != b.mots[i]) return a.mots[i] < b.mots[i] ? -1 : 1;
  }
  return 0;
}

// Ecriture decimale par divisions successives par 10^19 (pour les messages d'ecart seulement).
inline std::string decimal(Naturel a) {
  if (a.nul()) return "0";
  const u64 base = 10000000000000000000ull;
  std::vector<u64> morceaux;
  while (!a.nul()) {
    u128 reste = 0;
    for (std::size_t i = a.mots.size(); i-- > 0;) {
      const u128 courant = (reste << 64) | a.mots[i];
      a.mots[i] = static_cast<u64>(courant / base);
      reste = courant % base;
    }
    a.normaliser();
    morceaux.push_back(static_cast<u64>(reste));
  }
  std::string texte = std::to_string(morceaux.back());
  for (std::size_t i = morceaux.size() - 1; i-- > 0;) {
    const std::string partie = std::to_string(morceaux[i]);
    texte += std::string(19 - partie.size(), '0') + partie;
  }
  return texte;
}

inline std::string decimal128(u128 valeur) { return decimal(naturel(valeur)); }

// Niveau d'une fusion d'ordre un : rayon carre d2 / 4 de la boule diametrale, sous forme reduite (num, den).
inline void niveau_reduit(u128 d2, u128& num, u128& den) {
  const u128 g = (d2 % 4 == 0) ? 4 : (d2 % 2 == 0) ? 2 : 1;
  num = d2 / g;
  den = 4 / g;
}

}  // namespace mhgp12_emst
