// Supports d'une sphere donnee par son centre (anchor + N/D) et sa coquille (sites tries) :
// q_min, support canonique S* (plus petit lexicographique de cardinal q_min), et appartenance du centre a
// l'enveloppe convexe fermee d'un sous-ensemble de la coquille (theoreme de Gordan : un sous-ensemble A de
// la coquille est separable, c'est-a-dire qu'une direction rapproche strictement tous ses points, si et
// seulement si le centre n'est pas dans conv(A)).
#pragma once

#include <array>
#include <span>

#include "arith/geometry.hpp"

namespace mhgp10 {

// Accesseur de coordonnees : P(site) -> geom::P3.
template <class Coords>
bool center_in_closed_hull(const Coords& P, std::span<const u32> A, const geom::P3& anchor, const geom::Center& c) {
  const u32 m = static_cast<u32>(A.size());
  // paire antipodale (le centre sur un segment dont les extremites sont sur la sphere est son milieu)
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      if (geom::is_midpoint(P(A[i]), P(A[j]), anchor, c)) return true;
  // triangle non obtus dont le plan contient le centre
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 k = j + 1; k < m; ++k) {
        const geom::P3 &a = P(A[i]), &b = P(A[j]), &d = P(A[k]);
        const geom::P3 cr = geom::cross(geom::sub(b, a), geom::sub(d, a));
        if (cr.x == 0 && cr.y == 0 && cr.z == 0) continue;
        if (geom::dot(geom::sub(b, a), geom::sub(d, a)) < 0 || geom::dot(geom::sub(a, b), geom::sub(d, b)) < 0 ||
            geom::dot(geom::sub(a, d), geom::sub(b, d)) < 0)
          continue;
        if (geom::center_in_plane(a, b, d, anchor, c)) return true;
      }
  // tetraedre non degenere contenant le centre (bord compris)
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 k = j + 1; k < m; ++k)
        for (u32 l = k + 1; l < m; ++l) {
          const geom::P3* t[4] = {&P(A[i]), &P(A[j]), &P(A[k]), &P(A[l])};
          if (geom::orient(*t[0], *t[1], *t[2], *t[3]) == 0) continue;
          bool inside = true;
          for (int f = 0; f < 4 && inside; ++f) {
            const int so = geom::orient(*t[(f + 1) % 4], *t[(f + 2) % 4], *t[(f + 3) % 4], *t[f]);
            const int sc = geom::orient_center_wide(*t[(f + 1) % 4], *t[(f + 2) % 4], *t[(f + 3) % 4], anchor, c);
            if (sc != 0 && sc != so) inside = false;
          }
          if (inside) return true;
        }
  return false;
}

// q_min et S* d'une coquille (sites tries par indice). Rend false si aucun support (le centre n'est pas
// dans l'interieur relatif d'un sous-ensemble : sphere non critique).
template <class Coords>
bool canonical_support(const Coords& P, std::span<const u32> sh, const geom::P3& anchor, const geom::Center& ctr,
                       std::array<u32, 4>& sup, u8& q) {
  const u32 m = static_cast<u32>(sh.size());
  sup = {kNone, kNone, kNone, kNone};
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      if (geom::is_midpoint(P(sh[i]), P(sh[j]), anchor, ctr)) {
        sup = {sh[i], sh[j], kNone, kNone};
        q = 2;
        return true;
      }
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 k = j + 1; k < m; ++k) {
        const geom::P3 &a = P(sh[i]), &b = P(sh[j]), &c = P(sh[k]);
        const geom::P3 cr = geom::cross(geom::sub(b, a), geom::sub(c, a));
        if (cr.x == 0 && cr.y == 0 && cr.z == 0) continue;
        if (!geom::acute(a, b, c)) continue;
        if (!geom::center_in_plane(a, b, c, anchor, ctr)) continue;
        sup = {sh[i], sh[j], sh[k], kNone};
        q = 3;
        return true;
      }
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 k = j + 1; k < m; ++k)
        for (u32 l = k + 1; l < m; ++l) {
          const geom::P3* t[4] = {&P(sh[i]), &P(sh[j]), &P(sh[k]), &P(sh[l])};
          if (geom::orient(*t[0], *t[1], *t[2], *t[3]) == 0) continue;
          bool inside = true;
          for (int f = 0; f < 4 && inside; ++f) {
            const int so = geom::orient(*t[(f + 1) % 4], *t[(f + 2) % 4], *t[(f + 3) % 4], *t[f]);
            const int sc = geom::orient_center_wide(*t[(f + 1) % 4], *t[(f + 2) % 4], *t[(f + 3) % 4], anchor, ctr);
            if (sc == 0 || sc != so) inside = false;
          }
          if (!inside) continue;
          sup = {sh[i], sh[j], sh[k], sh[l]};
          q = 4;
          return true;
        }
  return false;
}

}  // namespace mhgp10
