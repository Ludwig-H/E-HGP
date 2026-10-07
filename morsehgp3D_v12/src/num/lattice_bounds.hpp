// Bornes du census de SITES ENTIERS (revue independante 10 du 2 octobre 2026, levier V3 de l'audit des
// transpositions) : minorant exact de la puissance sur les points ENTIERS d'une boite, majorant exact sur la boite
// continue. Contrat distinct de power_bounds, qui borne la boite continue : le minorant entier ne minore pas un
// segment continu (q2 de (0,0,0) a (1,0,0) : F vaut 0 aux deux sites et -1/2 au milieu). Reserve aux sites de
// l'index ; aucun transfert a un centre, a un flottant ou a un autre domaine sans nouvelle preuve.
//
// Deux preparations :
//   - LatticeSphere, voie GENERIQUE : toute Sphere, certifiee ou non, contre des boites et des sites n'importe ou dans
//     le domaine du profil (aucune garde) ;
//   - GuardedSphere (num/guard.hpp), voie GARDEE : seulement une boule certifiee (NUM-CERTIFIEE), dans son pave.
// Ni l'une ni l'autre ne forme le centre absolu a_j D + N_j (5B+6 bits, CST-0109) : le point entier le plus proche
// se lit sur le plancher et le plafond de N_j/D en LOCAL, puis + a_j, puis saturation a la boite.
#pragma once

#include <array>

#include "num/geometry.hpp"

namespace mhgp12::num {

// Sphere preparee une fois par parcours. F(x)=D|x-a|^2-2N.(x-a)=D(|x-c|^2-r^2), c=a+N/D, D>0 : chaque axe est une
// parabole convexe et les axes sont independants. Minorant : F au point entier le plus proche de c, ramene dans la
// boite (minimum exact sur boite inter Z^3). Majorant : F au coin le plus eloigne de c (maximum exact sur la boite
// continue). Les deux points sont dans la boite : leurs puissances reprennent les budgets natifs de power/side pour
// tout point du domaine (voie native ou certifiee pour l'etendue B). Sinon (q3 non certifie aux profils 21 et 24,
// q3/q4 non certifies au profil 32) : power_bound_signs inchange, comme avant ce levier.
// La sphere doit survivre a l'objet ; aucune copie de ses coefficients hors voie native.
class LatticeSphere {
 public:
  explicit LatticeSphere(const Sphere& sphere) noexcept;
  LatticeSphere(const LatticeSphere&) = delete;
  LatticeSphere& operator=(const LatticeSphere&) = delete;
  // Signes (-1,0,1) du minorant entier et du majorant continu, lower<=upper ; memes refus que power_bound_signs.
  // lower>0 exclut la boite sans evaluer le majorant : upper vaut alors +1 (le maximum majore le minimum).
  [[nodiscard]] Result<PowerBoundSigns> bound_signs(const Box& box, LaneCount* lanes = nullptr) const noexcept;
  // Identique a side(sphere, point), sans reconstruire la vue a chaque site.
  [[nodiscard]] Result<int> side(Point point, LaneCount* lanes = nullptr) const noexcept;
  bool lattice() const noexcept { return lattice_; }

 private:
  const Sphere& sphere_;
  bool lattice_;
  Lane lane_ = Lane::wide;  // native ou certifiee quand lattice_
  // Entier le plus proche de c_j (ex aequo : le plus petit), sature a [0,M-1] : le ramener dans [lo_j,hi_j] donne
  // le meme point qu'avant saturation, car toute boite est dans [0,M-1].
  std::array<i64, 3> nearest_{};
  // ceil(2 c_j), sature a [0,2M-1] : le coin eloigne est hi_j ssi lo_j+hi_j >= ce seuil, soit 2(Da_j+N_j)<=D(lo_j+hi_j).
  std::array<i64, 3> far_threshold_{};
  // Voie native : ancre, N et D copies une fois, pour evaluer la puissance aux points de la boite et aux sites comme
  // native_power (meme expression, memes budgets), sans reconstruire de vue ni refabriquer de Point a chaque appel.
  std::array<i64, 3> anchor_{};
  std::array<i128, 3> numerator_{};
  i128 denominator_ = 0;
  i128 power_at(const std::array<i64, 3>& point) const noexcept;
};

}  // namespace mhgp12::num
