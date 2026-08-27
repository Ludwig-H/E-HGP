// MorseHGP3D v5 — COMPLETIONS q4 « EN FORME DE KERNEL » (docs/GPU.md,
// livraison 4c) : pour un seed x VIVANT de l'ancre (a,b) et une completion y
// de la lentille, la sequence exacte de la lane q4 de production
// (generate.hpp) — rejets de lentille, de propriete (owner), d'exact-once, du
// prefiltre i64, de la puissance de face, de la forme de Cramer (det = 0), du
// bien-centrage, puis filtre de profondeur a la generation — ecrite avec les
// formes device (device_forms.hpp : Q3FormD, Q4FormD, DI128), sans allocation,
// et prouvee egale a la production (tests/q4_completion_shaped_gate.cpp).
// La cle et le niveau du candidat emis restent calcules par l'hote a partir de
// la forme (ball_key_reduce, q4_level_raw) : le device ne rend que l'etage
// atteint et la forme.
#pragma once

#include "../core/device.hpp"
#include "../core/dint.hpp"
#include "../core/types.hpp"
#include "../lanes/device_forms.hpp"
#include "../lanes/keys.hpp"
#include "../lanes/q4.hpp"

namespace mhgp5 {

enum class Q4Stage : u8 { kRejLens = 0, kRejOwner, kRejOnce, kRejI64, kRejFacePower, kRejDet, kRejCenter, kDeep, kEmit };

// Positions absolues des sites de l'ancre (SoA) pour le filtre de profondeur.
struct AnchorPositionsSoA {
  const i64* x;
  const i64* y;
  const i64* z;
  u32 n;
};

// Etages avant la profondeur. `no_canonical` : mutant CPU q4-no-canonical
// (toutes les faces emettent) ; `once_flip` : mutant de porte.
MHGP5_HD inline Q4Stage q4_completion_stage_shaped(const P3& a, const P3& b, const P3& x, const P3& y, PointId ida,
                                                   PointId idb, PointId idx, PointId idy, i64 D2, i64 l_ax, i64 l_bx,
                                                   const Q3FormD& face, bool no_canonical, bool once_flip, Q4FormD* f4) {
  const i64 l_ay = device_detail::norm2(device_detail::sub(y, a));
  const i64 l_by = device_detail::norm2(device_detail::sub(y, b));
  const i64 l_xy = device_detail::norm2(device_detail::sub(y, x));
  if (l_ay > D2 || l_by > D2 || l_xy > D2) return Q4Stage::kRejLens;
  if (!tetra_owned_by(D2, l_ax, l_ay, l_bx, l_by, l_xy, ida, idb, idx, idy)) return Q4Stage::kRejOwner;
  const P3 vy{2 * y.x - a.x - b.x, 2 * y.y - a.y - b.y, 2 * y.z - a.z - b.z};
  const bool y_smaller = once_flip ? (idy > idx) : (idy < idx);
  if (!no_canonical && device_detail::norm2(vy) > D2 && y_smaller) return Q4Stage::kRejOnce;
  if (!q4_i64_prefilter(D2, l_ax, l_bx, l_ay, l_by, l_xy)) return Q4Stage::kRejI64;
  if (di_sign(q3_power_d(face, y)) <= 0) return Q4Stage::kRejFacePower;
  *f4 = q4_form_d(a, b, x, y);
  if (di_is_zero(f4->det)) return Q4Stage::kRejDet;
  if (!q4_center_strictly_inside_d(*f4, a, b, x, y)) return Q4Stage::kRejCenter;
  return Q4Stage::kEmit;  // avant profondeur
}

// Filtre de profondeur : >= h4 sites de puissance < 0 (<= 0 sous nonstrict)
// tuent le candidat. Balayage plat, sortie anticipee.
MHGP5_HD inline bool q4_depth_shaped(const Q4FormD& f4, const AnchorPositionsSoA& pos, u32 h4, bool nonstrict) {
  u32 depth = 0;
  for (u32 i = 0; i < pos.n; ++i) {
    const P3 pz{pos.x[i], pos.y[i], pos.z[i]};
    const int sg = di_sign(q4_power_d(f4, pz));
    if ((sg < 0 || (nonstrict && sg == 0)) && ++depth >= h4) return true;
  }
  return false;
}

}  // namespace mhgp5
