// Regression permanente : un triangle aigu tres mince ne delocalise pas son
// circumcentre relativement a son arete maximale. Le cas entier ci-dessous
// approche les angles 89 degres, 89 degres et 2 degres.
#include <cstdio>
#include <vector>

#include "../src/lanes/q3.hpp"

using namespace mhgp5;

int main() {
  const P3 p{100, 100, 100};
  const P3 q{102, 100, 100};
  const P3 r{101, 157, 100};
  const i64 d2 = p3_norm2(p3_sub(r, p));
  const i64 base2 = p3_norm2(p3_sub(q, p));

  const i64 acute_p = p3_dot(p3_sub(q, p), p3_sub(r, p));
  const i64 acute_q = p3_dot(p3_sub(p, q), p3_sub(r, q));
  const i64 acute_r = p3_dot(p3_sub(p, r), p3_sub(q, r));
  if (d2 != 3250 || d2 <= 800 * base2 || acute_p <= 0 || acute_q <= 0 || acute_r <= 0 ||
      !is_acute_seed(p, r, q, d2, 0, 2, 1)) {
    std::fprintf(stderr, "ECHEC: la fixture n'est pas le triangle aigu mince canonique attendu\n");
    return 3;
  }

  const Q3Form form = q3_form(p, r, q);
  const Rational128 level = q3_level_raw(p, r, q);
  if (3 * level.num > (i128)d2 * level.den) {
    std::fprintf(stderr, "ECHEC: le rayon q3 depasse D2/3\n");
    return 3;
  }

  const BallForm ball = q3_ball_form(form);
  const i128 center_den = 2 * ball.a;
  i128 offset_num2 = 0;
  const i64 anchor_sum[3] = {p.x + r.x, p.y + r.y, p.z + r.z};
  for (int axis = 0; axis < 3; ++axis) {
    const i128 center_num = -ball.b[axis];
    const i128 twice_offset_num = 2 * center_num - center_den * anchor_sum[axis];
    offset_num2 += twice_offset_num * twice_offset_num;
  }
  if (3 * offset_num2 > (i128)d2 * center_den * center_den) {
    std::fprintf(stderr, "ECHEC: le circumcentre aigu sort du disque D2/12\n");
    return 3;
  }

  // Contre-famille locale de la decomposition ternaire symetrique : douze
  // points sur un cercle de rayon 5 et douze points sur son axe. Tout triplet
  // (deux points du cercle, un point de l'axe) est strictement aigu. Pour s=8,
  // la borne 2R/s=5/4 est plus petite que toute distance entre deux points de
  // l'axe ou entre l'axe et le cercle : le facteur axial est donc singleton.
  const std::vector<P3> circle = {
      {205, 200, 100}, {195, 200, 100}, {200, 205, 100}, {200, 195, 100},
      {203, 204, 100}, {203, 196, 100}, {197, 204, 100}, {197, 196, 100},
      {204, 203, 100}, {204, 197, 100}, {196, 203, 100}, {196, 197, 100},
  };
  std::vector<P3> axis;
  for (i64 j = 0; j < 12; ++j) axis.push_back(P3{200, 200, 110 + 2 * j});

  u64 acute_triples = 0;
  for (size_t i = 0; i < circle.size(); ++i) {
    if (p3_norm2(p3_sub(circle[i], P3{200, 200, 100})) != 25) return 3;
    for (size_t j = i + 1; j < circle.size(); ++j) {
      for (const P3& z : axis) {
        const i64 at_i = p3_dot(p3_sub(circle[j], circle[i]), p3_sub(z, circle[i]));
        const i64 at_j = p3_dot(p3_sub(circle[i], circle[j]), p3_sub(z, circle[j]));
        const i64 at_z = p3_dot(p3_sub(circle[i], z), p3_sub(circle[j], z));
        if (at_i <= 0 || at_j <= 0 || at_z <= 0) {
          std::fprintf(stderr, "ECHEC: triplet cercle-axe non aigu\n");
          return 3;
        }
        ++acute_triples;
      }
    }
  }
  for (size_t i = 0; i < axis.size(); ++i) {
    if (!p3_in_profile(axis[i]) || 64 * p3_norm2(p3_sub(axis[i], circle[0])) <= 100) return 3;
    for (size_t j = i + 1; j < axis.size(); ++j)
      if (64 * p3_norm2(p3_sub(axis[i], axis[j])) <= 100) return 3;
  }
  if (acute_triples != 792) {
    std::fprintf(stderr, "ECHEC: cardinalite de la contre-famille ternaire\n");
    return 3;
  }

  std::printf("q3_skinny_center OK D2=%lld base2=%lld acute=%lld/%lld/%lld ternary=%llu\n",
              (long long)d2, (long long)base2, (long long)acute_p, (long long)acute_q,
              (long long)acute_r, (unsigned long long)acute_triples);
  return 0;
}
