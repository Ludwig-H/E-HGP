// Comptes exacts d'une boule de Cat_K et de ses supports (lemme G de la sortie parametree), lus dans une table
// constexpr de binomes. Interne au module supports : un autre module passe par supports/supports.hpp, qui l'inclut.
//
// Notations : p interieurs stricts, m sites de coquille, qmin, ordre K, t = K - p (t >= 1 dans Cat_K), et
// N_j = nombre de parties de U_b de cardinal j qui contiennent un support de Q_b (fermeture vers le haut, Closure).
//   kparties_reliees  C(p+m, K)                 K-parties de P_b, toutes reliees entre elles au niveau lambda_b (T3)
//   compressed_parts  C(m, t)                   K-parties qui contiennent tout I_b (traces I u A)
//   strict_traces     C(m, t) - N_t             traces strictes, deja presentes avant lambda_b (T2) ; 0 : naissance
//   cofaces           somme_j C(p, K+1-j) N_j   (K+1)-parties G de P_b avec B(G) = b : liaisons distinctes (Prop. 5)
//   gabriel_cofaces   N_{t+1}                   celles qui contiennent aussi I_b (Def. 28)
// et par support Q d'arite a :
//   support_cofaces          C(p+m-a, K+1-a)    (K+1)-parties de P_b qui contiennent Q : incidences (Q, G)
//   support_gabriel_cofaces  C(m-a, t+1-a)      celles qui contiennent aussi I_b
// avec C(x, y) = 0 si y < 0 ou y > x (aucune partie). Un compte est une fonction de (p, m, K) et de N : rien n'est
// stocke. Bornes : K <= 12, p <= K - 1 <= 11 (Cat_K : p + qmin <= K + 1, qmin >= 2), m <= kMaxShell = 24 ; tout binome
// lu a un haut <= 35 et un bas <= K + 1 <= 13, donc < 2^32 (static_assert sur la table). La somme des cofaces est
// majoree par C(p+m, K+1) (Vandermonde, N_j <= C(m, j)), donc tient aussi en u32.
#pragma once

#include <array>
#include <span>

#include "core/core.hpp"

namespace mhgp11::supports {

inline constexpr u32 kMaxShell = 24;                            // coquille etendue ; au-dela : support_shell_capacity
inline constexpr u32 kMaxOrder = 12;                            // CatalogueParams::kmax <= 12
inline constexpr u32 kMaxInterior = kMaxOrder - 1;              // p <= K - 1 dans Cat_K
inline constexpr u32 kBinomialTop = kMaxInterior + kMaxShell;   // 35 : plus grand haut de binome lu
inline constexpr u32 kMaxBottom = kMaxOrder + 1;                // 13 : plus grand bas de binome lu

namespace supports_detail {

// C(a, b) pour 0 <= b <= a <= kBinomialTop, en u64 ; C(35, 17) depasse 2^32, mais aucun bas lu ne depasse 13.
inline constexpr auto kBinomial = [] {
  std::array<std::array<u64, kBinomialTop + 1>, kBinomialTop + 1> c{};
  for (u32 a = 0; a <= kBinomialTop; ++a) {
    c[a][0] = 1;
    for (u32 b = 1; b <= a; ++b) c[a][b] = c[a - 1][b - 1] + c[a - 1][b];
  }
  return c;
}();

constexpr bool binomials_fit_u32() noexcept {
  for (u32 a = 0; a <= kBinomialTop; ++a)
    for (u32 b = 0; b <= kMaxBottom; ++b)
      if (kBinomial[a][b] > 0xFFFFFFFFull) return false;
  return true;
}
static_assert(binomials_fit_u32(), "supports : tout binome lu (haut <= 35, bas <= 13) tient en u32");
static_assert(kBinomial[kMaxShell][kMaxShell / 2] <= 0xFFFFFFFFull, "supports : C(m, j) tient en u32 pour m <= 24");
static_assert(kBinomial[35][13] == 1476337800ull && kBinomial[35][12] == 834451800ull, "supports : table de Pascal");
static_assert(kBinomial[24][12] == 2704156ull && kBinomial[24][4] == 10626ull, "supports : table de Pascal");

// C(x, y) sur des ecarts signes : 0 si y < 0 ou y > x (aucune partie), et hors de la table (x > kBinomialTop). Dans
// les domaines de Shape, y <= kMaxBottom ou x <= kMaxShell : la valeur tient en u32 (static_assert ci-dessus).
constexpr u32 binomial(i32 x, i32 y) noexcept {
  if (y < 0 || y > x || x > static_cast<i32>(kBinomialTop)) return 0;
  return static_cast<u32>(kBinomial[static_cast<u32>(x)][static_cast<u32>(y)]);
}

struct Filler;

}  // namespace supports_detail

// Fermeture vers le haut de Q_b sur U_b : parts(j) = N_j pour 0 <= j <= m. Vide par le constructeur par defaut (m = 0,
// refusee par ball_counts), sinon construite seulement par ball_supports (regulier : N_j = [j = qmin]) ; alors
// N_j <= C(m, j), N_j = 0 pour j < qmin, N_qmin >= 1, N_m = 1.
class Closure {
 public:
  Closure() noexcept = default;
  u32 shell() const noexcept { return m_; }
  u32 parts(u32 j) const noexcept { return j <= m_ ? n_[j] : 0; }
  std::span<const u32> all() const noexcept { return std::span<const u32>(n_).first(m_ + 1); }

 private:
  friend struct supports_detail::Filler;
  std::array<u32, kMaxShell + 1> n_{};
  u32 m_ = 0;
};

// Forme d'une boule de Cat_K a l'ordre K : seul domaine des comptes. 1 <= K <= 12, 2 <= qmin <= 4,
// qmin <= m <= kMaxShell, p + qmin <= K + 1. Construite par make_shape (ou ball_shape), qui controle ce domaine.
class Shape {
 public:
  constexpr u32 interior() const noexcept { return p_; }
  constexpr u32 shell() const noexcept { return m_; }
  constexpr u32 qmin() const noexcept { return q_; }
  constexpr Order order() const noexcept { return k_; }
  constexpr u32 t() const noexcept { return u32{k_} - p_; }                 // >= 1
  constexpr bool in_window() const noexcept { return p_ + m_ >= u32{k_}; }  // W_K : p + qmin - 1 <= K <= p + m

 private:
  friend Result<Shape> make_shape(u32 p, u32 m, u32 q, Order k) noexcept;
  Shape() noexcept = default;
  u32 p_ = 0, m_ = 0, q_ = 0;
  Order k_ = 0;
};

// Refus : support_shell_capacity (m > kMaxShell, par check_shell) ; supports_invariant (tout autre ecart du domaine).
[[nodiscard]] Result<Shape> make_shape(u32 p, u32 m, u32 q, Order k) noexcept;

struct BallCounts {
  u32 kparties_reliees = 0;  // C(p+m, K)
  u32 compressed_parts = 0;  // C(m, t)
  u32 strict_traces = 0;     // C(m, t) - N_t ; doit egaler le journal du constructeur (contre-epreuve)
  u32 cofaces = 0;           // somme_j C(p, K+1-j) N_j
  u32 gabriel_cofaces = 0;   // N_{t+1}
  friend bool operator==(const BallCounts&, const BallCounts&) = default;
};

// Comptes d'une boule. Refus supports_invariant si la fermeture n'est pas celle d'une coquille de cette forme (taille,
// N_j <= C(m, j), N_j = 0 sous qmin, N_qmin >= 1, N_m = 1) : aucun compte ne deborde alors.
[[nodiscard]] Result<BallCounts> ball_counts(const Shape&, const Closure&) noexcept;

// Incidences (Q, G) d'un support d'arite a, et celles qui contiennent aussi I_b. Un support de Q_b a 2 <= a <= 4 (et
// a <= m) ; une autre arite n'est pas celle d'un support : 0.
constexpr u32 support_cofaces(const Shape& s, u32 arity) noexcept {
  if (arity < 2 || arity > 4) return 0;
  const i32 a = static_cast<i32>(arity);
  return supports_detail::binomial(static_cast<i32>(s.interior() + s.shell()) - a, i32{s.order()} + 1 - a);
}
constexpr u32 support_gabriel_cofaces(const Shape& s, u32 arity) noexcept {
  if (arity < 2 || arity > 4) return 0;
  const i32 a = static_cast<i32>(arity);
  return supports_detail::binomial(static_cast<i32>(s.shell()) - a, static_cast<i32>(s.t()) + 1 - a);
}

}  // namespace mhgp11::supports
