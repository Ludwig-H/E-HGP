// MorseHGP3D v4 — ACUTESEED : la face aigue detectee EN AMONT des census.
//
// Audit avant q4 (bc5b05d, § 3.1) : le calcul geometrique commun aux lanes
// q3 et q4 est la detection d'une face aigue possedee par l'ancre, PAS son
// statut d'evenement q3. Cette structure est produite avant tout census ;
// la lane q3 lui ajoute un census de circum-boule, la lane q4 une
// completion tetraedrique. Les deux consommateurs partagent le calcul sans
// jamais partager leurs filtres de profondeur (les survies q3 et q4 sont
// incomparables — fixture q4_source_independent_from_q3).
#pragma once

#include "q3_instruction.hpp"

namespace mhgp4 {

struct AcuteSeed {
  EdgeKey owner_edge;  // sur les vrais PointId
  PointId a, b, carrier;
  i32 ua, ub, ux;  // index uniques (acces positions/arbre)
  Q3Form face_form;
};

// Le porteur x est-il un seed aigu canonique de l'ancre (a,b) ?
//   - lentille : |x-a|² <= D² et |x-b|² <= D² (arete maximale faible) ;
//   - hors boule diametrale ⟺ acuite stricte : V² > D², V = 2x-a-b ;
//   - owner : (a,b) reste l'arete maximale du triangle, ties par EdgeKey.
inline bool is_acute_seed(const P3& pa, const P3& pb, const P3& px, i64 D2,
                          PointId ida, PointId idb, PointId idx) {
  const i64 l_ax = p3_norm2(p3_sub(px, pa));
  const i64 l_bx = p3_norm2(p3_sub(px, pb));
  if (l_ax > D2 || l_bx > D2) return false;
  const P3 v{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y,
             2 * px.z - pa.z - pb.z};
  if (p3_norm2(v) <= D2) return false;
  return anchor_owns_q3(D2, l_ax, l_bx, ida, idb, idx);
}

}  // namespace mhgp4
