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
#pragma once

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

// Garde preparee une fois par parcours ; la boule certifiee doit survivre a l'objet.
class GuardedSphere {
 public:
  explicit GuardedSphere(const CertifiedBall& ball) noexcept;
  GuardedSphere(const GuardedSphere&) = delete;
  GuardedSphere& operator=(const GuardedSphere&) = delete;
  // Signes (-1,0,1) du minorant entier et du majorant d'une boite de l'index, lower <= upper ; lower > 0 rend upper +1.
  [[nodiscard]] Result<PowerBoundSigns> bound_signs(const Box& box, GuardLedger* ledger = nullptr) const noexcept;
  // Cote d'un site : identique a side(sphere, point).
  [[nodiscard]] Result<int> side(Point point, GuardLedger* ledger = nullptr) const noexcept;
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
  Result<int> power_sign(const std::array<i64, 3>& v, GuardLedger* ledger) const noexcept;
  template <int Words>
  Result<int> wide_sign(const std::array<i64, 3>& v) const noexcept;
  const CertifiedBall& ball_;
  Lane lane_ = Lane::wide;
  bool native_coefficients_ = false;
  std::array<i64, 3> anchor_{}, guard_lo_{}, guard_hi_{};
  std::array<i64, 3> nearest_{};    // entier le plus proche de c_j - o_j (ex aequo : le plus petit), |.| < M
  std::array<i64, 3> threshold_{};  // ceil(2 (c_j - o_j)) : le coin lointain est hi_j ssi lo_j + hi_j - 2 o_j >= seuil
  std::array<i128, 3> numerator_{};
  i128 denominator_ = 0;
  bool broken_ = false;  // preparation impossible (invariant) : toute question rend arithmetic_invariant
};

}  // namespace mhgp12::num
