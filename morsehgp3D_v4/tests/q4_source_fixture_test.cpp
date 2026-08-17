// MorseHGP3D v4 — FIXTURE BLOQUANTE : la source q4 est INDEPENDANTE de q3.
//
// Audit cible avant q4 (bc5b05d, § 2) : pour K_max = 10, h_3 = 9 et
// h_4 = 8 ; W_4 ⊂ W_3 n'induit AUCUNE implication de survie entre lanes.
// Fixture entiere gravee (13 points, IDs = rangs 0..12) :
//   a=(100,300,300), b=(300,300,300), x=(200,160,400), y=(200,160,200),
//   z1..z9 = m + (0, dy, dz) autour du milieu m=(200,300,300), tous dans
//   W_3(a,b) ∖ W_4(a,b) (3H² > Xi et 2H² <= Xi).
// Alors :
//   n3 = 9  => l'ancre (a,b) est MORTE pour q3 (n3 >= h_3) ;
//   n4 = 0  => l'ancre est VIVANTE pour q4 ;
//   le tetraedre {a,b,x,y} est un support q4 positif : circumcentre
//   c = (200,230,300) = (a+b+x+y)/4, R² = 14900, les neuf z_i strictement
//   exterieurs (distances carrees gravees), owner = ab car
//   |ab|² = |xy|² = 40000 > 39600 et EdgeKey(0,1) < EdgeKey(2,3).
// Une source q4 branchee apres le filtre q3 (evenements q3 OU ancres q3
// vivantes) perd silencieusement ce tetraedre. Le mutant
// --inject=q4-seeds-from-q3-live simule exactement ce branchement : il ne
// considere que les ancres q3 vivantes et doit perdre le support (code 4).
//
// Codes : 0 fixture conforme ; 3 fixture violee ; 4 mutant tue.
#include <cstdio>
#include <cstring>

#include "../src/events/q3_instruction.hpp"
#include "../src/events/spindle.hpp"

namespace {

using namespace mhgp4;

struct Fx {
  P3 a{100, 300, 300}, b{300, 300, 300}, x{200, 160, 400}, y{200, 160, 200};
  P3 z[9] = {{200, 355, 300}, {200, 354, 310}, {200, 353, 315},
             {200, 352, 320}, {200, 351, 323}, {200, 350, 325},
             {200, 356, 305}, {200, 355, 312}, {200, 354, 317}};
  // Distances carrees des z_i au circumcentre c=(200,230,300), gravees par
  // l'audit ; toutes strictement superieures a R² = 14900.
  i64 zc2[9] = {15625, 15476, 15354, 15284, 15170, 15025, 15901, 15769, 15665};
};

i64 d2(const P3& p, const P3& q) { return p3_norm2(p3_sub(p, q)); }

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  bool mutant = false;
  for (int i = 1; i < argc; ++i)
    if (std::strcmp(argv[i], "--inject=q4-seeds-from-q3-live") == 0)
      mutant = true;

  const Fx fx;
  const u64 smax = 11;  // K_max = 10
  const u64 h3 = lane_h(Lane::kQ3, smax);
  const u64 h4 = lane_h(Lane::kQ4, smax);
  if (h3 != 9 || h4 != 8) {
    std::fprintf(stderr, "FIXTURE : seuils h3=%llu h4=%llu (attendus 9/8)\n",
                 (unsigned long long)h3, (unsigned long long)h4);
    return 3;
  }

  // 1. Les neuf z_i sont dans W_3(a,b) et hors de W_4(a,b) — predicats de
  // fuseau EXACTS de la production.
  u64 n3 = 0, n4 = 0;
  for (const P3& z : fx.z) {
    if (in_spindle(Lane::kQ3, fx.a, fx.b, z)) ++n3;
    if (in_spindle(Lane::kQ4, fx.a, fx.b, z)) ++n4;
  }
  // x et y : hors des deux fuseaux ? (ils sont temoins d'aucun des deux —
  // H = d·w - |w|² <= 0 pour x et y, verifie ici plutot que suppose)
  for (const P3* p : {&fx.x, &fx.y}) {
    if (in_spindle(Lane::kQ3, fx.a, fx.b, *p)) ++n3;
    if (in_spindle(Lane::kQ4, fx.a, fx.b, *p)) ++n4;
  }
  if (n3 != 9 || n4 != 0) {
    std::fprintf(stderr, "FIXTURE : n3=%llu n4=%llu (attendus 9/0)\n",
                 (unsigned long long)n3, (unsigned long long)n4);
    return 3;
  }
  const bool q3_anchor_alive = n3 < h3;
  const bool q4_anchor_alive = n4 < h4;
  if (q3_anchor_alive || !q4_anchor_alive) {
    std::fprintf(stderr, "FIXTURE : survies q3=%d q4=%d (attendues 0/1)\n",
                 (int)q3_anchor_alive, (int)q4_anchor_alive);
    return 3;
  }

  // 2. Le support q4 {a,b,x,y} existe : longueurs gravees, owner ab par
  // EdgeKey(0,1) < EdgeKey(2,3) a longueur egale, circumcentre entier
  // c = (a+b+x+y)/4, quatre distances R² = 14900, neuf z_i strictement
  // exterieurs aux distances gravees.
  if (d2(fx.a, fx.b) != 40000 || d2(fx.x, fx.y) != 40000 ||
      d2(fx.a, fx.x) != 39600 || d2(fx.a, fx.y) != 39600 ||
      d2(fx.b, fx.x) != 39600 || d2(fx.b, fx.y) != 39600) {
    std::fprintf(stderr, "FIXTURE : longueurs du tetraedre\n");
    return 3;
  }
  if (!edge_key_less(edge_key(0, 1), edge_key(2, 3))) {
    std::fprintf(stderr, "FIXTURE : owner EdgeKey(0,1)\n");
    return 3;
  }
  const P3 c{(fx.a.x + fx.b.x + fx.x.x + fx.y.x) / 4,
             (fx.a.y + fx.b.y + fx.x.y + fx.y.y) / 4,
             (fx.a.z + fx.b.z + fx.x.z + fx.y.z) / 4};
  if (c.x != 200 || c.y != 230 || c.z != 300) {
    std::fprintf(stderr, "FIXTURE : circumcentre\n");
    return 3;
  }
  const i64 R2 = 14900;
  if (d2(fx.a, c) != R2 || d2(fx.b, c) != R2 || d2(fx.x, c) != R2 ||
      d2(fx.y, c) != R2) {
    std::fprintf(stderr, "FIXTURE : R² du circumcentre\n");
    return 3;
  }
  u64 q4_depth = 0;
  for (int i = 0; i < 9; ++i) {
    if (d2(fx.z[i], c) != fx.zc2[i]) {
      std::fprintf(stderr, "FIXTURE : distance gravee z%d\n", i + 1);
      return 3;
    }
    if (d2(fx.z[i], c) <= R2) ++q4_depth;
  }
  if (q4_depth != 0) {
    std::fprintf(stderr, "FIXTURE : profondeur q4 %llu (attendue 0)\n",
                 (unsigned long long)q4_depth);
    return 3;
  }

  // 3. Les faces abx et aby sont strictement aigues et chaque z_i, membre
  // de W_3(a,b), est interieur a leurs circum-boules : ces faces ne sont pas
  // des evenements q3 peu profonds — le tetraedre est invisible depuis q3.
  for (const P3* apex : {&fx.x, &fx.y}) {
    const P3 vv{2 * apex->x - fx.a.x - fx.b.x, 2 * apex->y - fx.a.y - fx.b.y,
                2 * apex->z - fx.a.z - fx.b.z};
    if (p3_norm2(vv) <= d2(fx.a, fx.b)) {
      std::fprintf(stderr, "FIXTURE : face non strictement aigue\n");
      return 3;
    }
    const Q3Form f = q3_form(fx.a, fx.b, *apex);
    for (const P3& z : fx.z)
      if (q3_power(f, z) >= 0) {
        std::fprintf(stderr, "FIXTURE : z_i non interieur a la face\n");
        return 3;
      }
  }

  // 4. La SOURCE : combien de supports q4 selon le branchement ?
  //   - source correcte (lane q4 du front partage) : l'ancre q4 est vivante,
  //     le support {a,b,x,y} est trouve -> 1 ;
  //   - MUTANT q4-seeds-from-q3-live : seules les ancres q3 VIVANTES
  //     sement ; (a,b) est q3-morte -> 0 support, tetraedre perdu.
  const u64 q4_supports_found = mutant ? (q3_anchor_alive ? 1 : 0)
                                       : (q4_anchor_alive ? 1 : 0);
  std::printf(
      "q4_source : n3=%llu n4=%llu q3_alive=%d q4_alive=%d depth_q4=%llu "
      "supports=%llu owner=(0,1)\n",
      (unsigned long long)n3, (unsigned long long)n4, (int)q3_anchor_alive,
      (int)q4_anchor_alive, (unsigned long long)q4_depth,
      (unsigned long long)q4_supports_found);
  if (mutant) {
    if (q4_supports_found == 0) {
      std::printf("MUTANT TUE : la source q3-vivante perd le tetraedre\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (q4_supports_found != 1) {
    std::fprintf(stderr, "FIXTURE : support q4 non trouve\n");
    return 3;
  }
  return 0;
}
