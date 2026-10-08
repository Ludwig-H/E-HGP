// Boule certifiee et garde entiere (docs/CONTRAT_NUMERIQUE.md, paragraphe 2 : NUM-CERTIFIEE, NUM-GARDE ; CST-0108,
// CST-0109, CST-0201).
//
// NUM-CERTIFIEE. La garde repose sur c dans conv(S), faux pour une proposition flottante ou une sphere qui passe
// seulement par trois ou quatre sites (centre circonscrit d'un triangle obtus hors du triangle ; d'un triangle presque
// aligne, arbitrairement loin). Le TYPE CertifiedBall porte la preuve : sa seule fabrique de num verifie c dans
// conv(S) en exact par les signes barycentriques (q1, q2 toujours ; q3 strictement aigu ; q4 poids de presentation
// strictement positifs). Une candidate refusee reste dans la voie generique (LatticeSphere), sans garde.
//
// NUM-GARDE. Soit s l'etendue du support S, m son coin minimal, M = 2^s. Le centre est dans [m, m+M-1] par axe.
// Avec c dans conv(S) et S sur la sphere, la boule est MEB(S). La boule au milieu de la boite du support contient S,
// donc R^2 <= (wx^2+wy^2+wz^2)/4 <= 3(M-1)^2/4 < M^2. Tout point x de la boule fermee verifie ainsi
// m_j - M < x_j < m_j + 2M : c'est le PAVE (ouvert), y compris s=0.
//   - site hors du pave : exterieur, sans arithmetique ; site dans le pave : |x_j - o_j| < 2M pour l'ancre o de S.
//     On conserve ici les budgets anterieurs plus larges : 6s+11 (CST-0111), domaine t = s+2 et voies inchangees ;
//   - boite DISJOINTE du pave : exterieure ; sinon minorant au point entier de la boite le plus proche du centre,
//     calcule en local (plancher de N_j/D, + o_j, saturation a la boite), qui est dans le pave ;
//   - boite NON CONTENUE dans le pave : majorant positif sans arithmetique (elle n'est pas dans la boule) ; une
//     boite partielle n'est jamais rejetee, elle est raffinee ; sinon coin lointain au budget mixte.
// La preuve se fait par requete, l'ancre conservee : aucun repere commun a toutes les requetes n'est construit.
//
// Evaluation A PLAT (T2-d-B2, levier B2-C, sur le modele de microbancs/mes_g_appareil/noyau_g.hpp) : cote d'un site,
// signes d'une boite et puissance des voies native et certifiee sont en ligne, pour que le parcours du census
// (index/census*.cpp) n'appelle aucune fonction par noeud ni par site ; la preparation et les voies controlee et large
// (essai controle puis repli large) restent hors ligne (guard.cpp), memes compteurs et memes refus.
#pragma once

#include <algorithm>
#include <span>

#include "num/geometry.hpp"

namespace mhgp12::num {

class CertifiedBall {
 public:
  // Un a quatre points de support. Rien si le support est degenere ou si son centre n'est pas certifie dans
  // l'enveloppe convexe ; refus parameter_out_of_range hors de 1..4 points.
  [[nodiscard]] static Result<std::optional<CertifiedBall>> certify(std::span<const Point> support) noexcept;
  const Sphere& sphere() const noexcept { return sphere_; }
  // Repere du support (NUM-REPERE) : coin minimal et etendue.
  const std::array<u32, 3>& corner() const noexcept { return corner_; }
  int span() const noexcept { return span_; }

 private:
  CertifiedBall(const Sphere& sphere, std::array<u32, 3> corner, int span) noexcept
      : sphere_(sphere), corner_(corner), span_(static_cast<u8>(span)) {}
  Sphere sphere_;
  std::array<u32, 3> corner_;
  u8 span_;
};

// Compteurs de la garde : voies des evaluations, et decisions prises sans arithmetique.
struct GuardLedger {
  LaneCount lanes;
  u64 disjoint_boxes = 0, partial_boxes = 0, outside_sites = 0;
  friend bool operator==(const GuardLedger&, const GuardLedger&) = default;
};

// Garde preparee une fois par parcours ; la boule certifiee doit survivre a l'objet. Le parcours du census emploie les
// formes A PLAT (issue rendue, valeur ecrite dans un argument) ; les formes Result en sont des enveloppes.
class GuardedSphere {
 public:
  explicit GuardedSphere(const CertifiedBall& ball) noexcept;
  GuardedSphere(const GuardedSphere&) = delete;
  GuardedSphere& operator=(const GuardedSphere&) = delete;
  // Signes (-1,0,1) du minorant entier et du majorant d'une boite de l'index, lower <= upper ; lower > 0 rend upper +1.
  [[nodiscard]] Result<PowerBoundSigns> bound_signs(const Box& box, GuardLedger* ledger = nullptr) const noexcept {
    PowerBoundSigns signs;
    const Outcome outcome = bound_signs(box, signs, ledger);
    if (!outcome.ok()) return outcome;
    return signs;
  }
  // A plat (parcours du census) : memes signes, ecrits dans signs.
  [[nodiscard]] Outcome bound_signs(const Box& box, PowerBoundSigns& signs, GuardLedger* ledger) const noexcept;
  // Cote d'un site : identique a side(sphere, point).
  [[nodiscard]] Result<int> side(Point point, GuardLedger* ledger = nullptr) const noexcept {
    int sign = 0;
    const Outcome outcome = side_site(point.x(), point.y(), point.z(), sign, ledger);
    if (!outcome.ok()) return outcome;
    return sign;
  }
  // A plat (parcours du census) : cote du site (x, y, z), ecrit dans side, sans construire de Point. Une coordonnee
  // au-dela de kCoordMax rend coordinate_out_of_domain, le refus de Point::make (un OU et une comparaison ; aucun site
  // d'un Cloud ne l'atteint, prepare_cloud l'a controle). L'ecart a l'ancre est alors sous 2^32 en valeur absolue : le
  // controle de side_offset n'y refuse rien et n'est pas refait.
  [[nodiscard]] Outcome side_site(u32 x, u32 y, u32 z, int& side, GuardLedger* ledger) const noexcept;
  // Cote d'une requete donnee par son ecart a l'ancre (repere local), |offset_j| <= 2^34 sinon refus.
  [[nodiscard]] Result<int> side_offset(const std::array<i64, 3>& offset, GuardLedger* ledger = nullptr) const noexcept;
  // Voie de la boule, uniforme sur tout le parcours : palier etroit natif, certificat au domaine s+2, essai controle
  // ou voie large (au palier moyen : 6s+11 <= 155 bits, trois mots ; au palier large : 209 bits, quatre mots).
  Lane lane() const noexcept { return lane_; }
  // Point du pave ouvert, en coordonnees absolues.
  bool in_guard(const std::array<i64, 3>& point) const noexcept {
    for (int j = 0; j < 3; ++j)
      if (point[j] <= guard_lo_[j] || point[j] >= guard_hi_[j]) return false;
    return true;
  }

 private:
  // Signe de la puissance a l'ecart v (|v_j| < 2M), ecrit dans sign : voies native et certifiee en ligne, les autres
  // par slow_sign.
  [[nodiscard]] Outcome power_sign(const std::array<i64, 3>& v, int& sign, GuardLedger* ledger) const noexcept;
  // Preparation impossible (invariant), essai controle, repli large : hors ligne (guard.cpp).
  [[nodiscard]] Outcome slow_sign(const std::array<i64, 3>& v, int& sign, GuardLedger* ledger) const noexcept;
  template <int Words>
  Result<int> wide_sign(const std::array<i64, 3>& v) const noexcept;
  const CertifiedBall& ball_;
  Lane lane_ = Lane::wide;
  bool native_coefficients_ = false;
  bool inline_lane_ = false;  // voie native ou certifiee et preparation reussie : puissance en ligne
  bool short_norm_ = false;   // s + 2 <= 30 : |v|^2 tient en i64
  std::array<i64, 3> anchor_{}, guard_lo_{}, guard_hi_{};
  std::array<i64, 3> nearest_{};    // entier le plus proche de c_j - o_j (ex aequo : le plus petit), |.| < M
  std::array<i64, 3> threshold_{};  // ceil(2 (c_j - o_j)) : le coin lointain est hi_j ssi lo_j + hi_j - 2 o_j >= seuil
  std::array<i128, 3> numerator_{};
  i128 denominator_ = 0;
  bool broken_ = false;  // preparation impossible (invariant) : toute question rend arithmetic_invariant
};

// Precondition : |v_j| < 2M, ecart a l'ancre d'un point du pave resserre ; les budgets restent ceux de |v_j| < 3M.
[[gnu::always_inline]] inline Outcome GuardedSphere::power_sign(const std::array<i64, 3>& v, int& sign,
                                                                GuardLedger* ledger) const noexcept {
  if (!inline_lane_) return slow_sign(v, sign, ledger);
  // Palier etroit : D|v|^2 < 648 M^6, 2|N.v| < 432 M^6, somme < 2^(6s+11) <= 2^107 (CST-0111). Certificat au domaine
  // s+2 : |v_j| < 2^(s+2), chaque produit et somme partielle < 2^127 (power_certificate.hpp). Etendue s+2 <= 30 :
  // |v|^2 < 3 * 2^58 tient en i64 ; l'etendue d'un support ne depasse pas B, d'ou s+2 <= 30 des que B <= 28.
  if (ledger != nullptr) ledger->lanes.add(lane_);
  const bool short_norm = kCoordBits + 2 <= 30 || short_norm_;
  const i128 norm = short_norm ? i128{v[0] * v[0] + v[1] * v[1] + v[2] * v[2]}
                               : i128{v[0]} * v[0] + i128{v[1]} * v[1] + i128{v[2]} * v[2];
  i128 total = denominator_ * norm;
  for (int j = 0; j < 3; ++j) total += numerator_[j] * (-2 * i128{v[j]});
  sign = (total > 0) - (total < 0);
  return {};
}

[[gnu::always_inline]] inline Outcome GuardedSphere::side_site(u32 x, u32 y, u32 z, int& side,
                                                               GuardLedger* ledger) const noexcept {
  if constexpr (kCoordBits < 32) {
    if ((x | y | z) > kCoordMax) return fail(Reason::coordinate_out_of_domain);
  }
  const std::array<i64, 3> point{x, y, z};
  if (!in_guard(point)) {
    if (ledger != nullptr) ++ledger->outside_sites;
    side = 1;  // hors du pave : exterieur a la boule fermee, sans arithmetique
    return {};
  }
  return power_sign({point[0] - anchor_[0], point[1] - anchor_[1], point[2] - anchor_[2]}, side, ledger);
}

inline Result<int> GuardedSphere::side_offset(const std::array<i64, 3>& offset, GuardLedger* ledger) const noexcept {
  constexpr i64 limit = i64{1} << 34;
  std::array<i64, 3> point{};
  for (int j = 0; j < 3; ++j) {
    if (offset[j] < -limit || offset[j] > limit) return fail(Reason::parameter_out_of_range);
    point[j] = anchor_[j] + offset[j];
  }
  if (!in_guard(point)) {
    if (ledger != nullptr) ++ledger->outside_sites;
    return 1;  // hors du pave : exterieur a la boule fermee, sans arithmetique
  }
  int sign = 0;
  const Outcome outcome = power_sign(offset, sign, ledger);
  if (!outcome.ok()) return outcome;
  return sign;
}

[[gnu::always_inline]] inline Outcome GuardedSphere::bound_signs(const Box& box, PowerBoundSigns& signs,
                                                                 GuardLedger* ledger) const noexcept {
  const auto lo = box.lo().coordinates(), hi = box.hi().coordinates();
  bool contained = true;
  for (int j = 0; j < 3; ++j) {
    if (i64{hi[j]} <= guard_lo_[j] || i64{lo[j]} >= guard_hi_[j]) {  // disjointe du pave ouvert
      if (ledger != nullptr) ++ledger->disjoint_boxes;
      signs = PowerBoundSigns{.lower = 1, .upper = 1};
      return {};
    }
    contained = contained && i64{lo[j]} > guard_lo_[j] && i64{hi[j]} < guard_hi_[j];
  }
  // Point entier le plus proche du centre ramene dans la boite : dans le pave (la boite le rencontre sur chaque axe et
  // l'entier le plus proche du centre est dans [m, m+M-1]).
  std::array<i64, 3> near{}, far{};
  for (int j = 0; j < 3; ++j) {
    near[j] = std::clamp<i64>(anchor_[j] + nearest_[j], lo[j], hi[j]) - anchor_[j];
    far[j] = (i64{lo[j]} + hi[j] - 2 * anchor_[j] >= threshold_[j] ? i64{hi[j]} : i64{lo[j]}) - anchor_[j];
  }
  int lower = 0, upper = 0;
  MHGP12_TRY(power_sign(near, lower, ledger));
  if (lower > 0) {
    signs = PowerBoundSigns{.lower = 1, .upper = 1};
    return {};
  }
  if (!contained) {  // une boite non contenue dans le pave n'est pas dans la boule : majorant positif, a raffiner
    if (ledger != nullptr) ++ledger->partial_boxes;
    signs = PowerBoundSigns{.lower = lower, .upper = 1};
    return {};
  }
  MHGP12_TRY(power_sign(far, upper, ledger));
  if (lower > upper) return fail(Reason::arithmetic_invariant);
  signs = PowerBoundSigns{.lower = lower, .upper = upper};
  return {};
}

}  // namespace mhgp12::num
