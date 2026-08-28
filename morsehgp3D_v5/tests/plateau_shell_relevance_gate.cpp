// Regression permanente : le rang ferme de la boule complete ne borne pas la
// pertinence d'un sous-support sur un plateau. Douze points u16 cospheriques
// contiennent ici un tetraedre strictement bien centre de cardinalite quatre,
// encore pertinent dans la fenetre smax=11. Pour son ancre canonique, les dix
// autres sites donnent dix droites distinctes concurrentes dans le cover q4
// historique de coefficient 3.
#include <algorithm>
#include <cstdio>
#include <vector>

#include "../src/forest/plateau.hpp"
#include "../src/lanes/q4.hpp"

using namespace mhgp5;

int main() {
  const P3 center{200, 200, 200};
  const std::vector<P3> points = {
      {313, 313, 313}, {313, 87, 87},   {87, 313, 87},   {87, 87, 313},
      {327, 313, 297}, {327, 297, 313}, {321, 321, 295}, {321, 295, 321},
      {313, 327, 297}, {297, 327, 313}, {297, 313, 327}, {295, 321, 321},
  };
  const std::vector<i32> shell = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  const std::vector<i32> interior;
  const std::vector<i32> target = {0, 1, 2, 3};
  constexpr i64 radius2 = 38307;

  for (const P3& p : points) {
    if (!p3_in_profile(p) || p3_norm2(p3_sub(p, center)) != radius2) {
      std::fprintf(stderr, "ECHEC: point hors de la coquille commune\n");
      return 3;
    }
  }

  const P3 e1 = p3_sub(points[1], points[0]);
  const P3 e2 = p3_sub(points[2], points[0]);
  const P3 e3 = p3_sub(points[3], points[0]);
  const i64 affine_det = p3_dot(e1, p3_cross(e2, e3));
  const P3 target_sum = p3_add(p3_add(points[0], points[1]), p3_add(points[2], points[3]));
  if (affine_det == 0 || target_sum != P3{4 * center.x, 4 * center.y, 4 * center.z}) {
    std::fprintf(stderr, "ECHEC: le tetraedre temoin n'est pas strictement bien centre\n");
    return 3;
  }

  const BallRat ball{{center.x, center.y, center.z}, 1};
  for (size_t i = 0; i < shell.size(); ++i) {
    for (size_t j = i + 1; j < shell.size(); ++j) {
      const std::vector<i32> pair = {shell[i], shell[j]};
      if (center_in_conv(ball, points, pair)) {
        std::fprintf(stderr, "ECHEC: le centre est deja porte par une paire\n");
        return 3;
      }
      for (size_t k = j + 1; k < shell.size(); ++k) {
        const std::vector<i32> triangle = {shell[i], shell[j], shell[k]};
        if (center_in_conv(ball, points, triangle)) {
          std::fprintf(stderr, "ECHEC: le centre est deja porte par un triangle\n");
          return 3;
        }
      }
    }
  }

  const P3& a = points[0];
  const P3& b = points[1];
  const i64 diameter2 = p3_norm2(p3_sub(b, a));
  const i64 l02 = p3_norm2(p3_sub(points[2], a));
  const i64 l03 = p3_norm2(p3_sub(points[3], a));
  const i64 l12 = p3_norm2(p3_sub(points[2], b));
  const i64 l13 = p3_norm2(p3_sub(points[3], b));
  const i64 l23 = p3_norm2(p3_sub(points[3], points[2]));
  if (l02 != diameter2 || l03 != diameter2 || l12 != diameter2 || l13 != diameter2 || l23 != diameter2 ||
      !tetra_owned_by(diameter2, l02, l03, l12, l13, l23, 0, 1, 2, 3)) {
    std::fprintf(stderr, "ECHEC: (0,1) n'est pas l'ancre canonique du tetraedre regulier\n");
    return 3;
  }
  const P3 twice_center_offset{2 * center.x - a.x - b.x,
                               2 * center.y - a.y - b.y,
                               2 * center.z - a.z - b.z};
  struct LineNormal {
    i64 a;
    i64 b;
  };
  std::vector<LineNormal> normals;
  for (size_t i = 0; i < points.size(); ++i) {
    const P3 twice_offset{2 * points[i].x - a.x - b.x,
                          2 * points[i].y - a.y - b.y,
                          2 * points[i].z - a.z - b.z};
    if (p3_norm2(twice_offset) > 3 * diameter2) {
      std::fprintf(stderr, "ECHEC: site hors du cover q4 coefficient 3\n");
      return 3;
    }
    if (i < 2) continue;

    // Le plan mediateur de (0,1) verifie v_y+v_z=0. Dans les coordonnees
    // (v_x,v_y), la normale de h_x vaut (2(x_x-M_x), 2(x_y-x_z)).
    const LineNormal normal{2 * points[i].x - a.x - b.x, 2 * (points[i].y - points[i].z)};
    const i64 line_at_center_times4 =
        2 * (normal.a * twice_center_offset.x + normal.b * twice_center_offset.y) -
        (p3_norm2(twice_offset) - diameter2);
    if ((normal.a == 0 && normal.b == 0) || line_at_center_times4 != 0) {
      std::fprintf(stderr, "ECHEC: contrainte non lineaire ou non concurrente\n");
      return 3;
    }
    for (const LineNormal& previous : normals) {
      if (normal.a * previous.b == previous.a * normal.b) {
        std::fprintf(stderr, "ECHEC: deux droites concurrentes sont identiques\n");
        return 3;
      }
    }
    normals.push_back(normal);
  }

  std::vector<PlateauEvent> events;
  expand_plateau(ball, points, interior, shell, 11, &events);

  const auto found = std::find_if(events.begin(), events.end(), [&](const PlateauEvent& ev) {
    return ev.tpart == target && ev.ipart.empty() && ev.active_mask == 0x0fu;
  });
  if (diameter2 != 102152 || shell.size() != 12 || normals.size() != 10 || found == events.end()) {
    std::fprintf(stderr,
                 "ECHEC: la coquille complete de 12 points a masque le sous-support pertinent de cardinalite quatre\n");
    return 3;
  }

  std::printf("plateau_shell_relevance OK shell=%zu droites=%zu support=%zu evenements=%zu det=%lld\n",
              shell.size(), normals.size(), target.size(), events.size(), (long long)affine_det);
  return 0;
}
