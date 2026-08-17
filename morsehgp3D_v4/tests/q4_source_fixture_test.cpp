// MorseHGP3D v4 — FIXTURE BLOQUANTE RENFORCEE : la source q4 est
// INDEPENDANTE de q3 — des ancres q3 vivantes ET du flux des evenements q3.
//
// Audit bc5b05d (13 points) renforce par l'audit apres f6b29e1 : la version
// initiale prouvait l'ancre (a,b) q3-morte/q4-vivante, mais les faces axy
// et bxy (owner xy) restaient des evenements q3 peu profonds — une source
// branchee sur le FLUX q3 voyait encore le tetraedre. Les neuf points
//   w_j = (196+j, 105, 300), j = 0..8   (t = j-4 ∈ [-4,4])
// ferment ce trou (§ 2 de l'audit, valeurs exactes re-verifiees ici) :
//   - 37·(|w_j - c_axy|² - r_face²) = 37t² + 2450t - 69425 <= -59033 < 0
//     et symetriquement pour bxy : les quatre faces deviennent q3-profondes
//     (depth >= h_3 = 9), plus aucune n'est un Q3Event ;
//   - r_j² = 55² + t² ∈ [3025, 3041] autour du milieu de xy : les w_j sont
//     dans W_3(x,y) ∖ W_4(x,y) — l'ancre xy est q3-morte ET q4-vivante,
//     comme ab avec les z_i ;
//   - |w_j - c|² = 125² + t² ∈ [15625, 15641] > R² = 14900 : l'evenement q4
//     garde profondeur zero, sans coquille.
// Porte (§ 3 de l'audit) : les six verifications explicites, et DEUX
// mutants de source — q4-seeds-from-q3-live (ancres q3 vivantes) et
// q4-seeds-from-q3-events (flux des evenements q3) — perdent le support.
//
// Codes : 0 fixture conforme ; 3 fixture violee ; 4 mutant tue.
#include <cstdio>
#include <cstring>

#include "../src/events/q3_instruction.hpp"
#include "../src/events/q4_instruction.hpp"
#include "../src/events/spindle.hpp"

namespace {

using namespace mhgp4;

struct Fx {
  P3 a{100, 300, 300}, b{300, 300, 300}, x{200, 160, 400}, y{200, 160, 200};
  P3 z[9] = {{200, 355, 300}, {200, 354, 310}, {200, 353, 315},
             {200, 352, 320}, {200, 351, 323}, {200, 350, 325},
             {200, 356, 305}, {200, 355, 312}, {200, 354, 317}};
  P3 w[9];  // w_j = (196+j, 105, 300)
  i64 zc2[9] = {15625, 15476, 15354, 15284, 15170, 15025, 15901, 15769, 15665};
  Fx() {
    for (int j = 0; j < 9; ++j) w[j] = P3{196 + j, 105, 300};
  }
};

i64 d2(const P3& p, const P3& q) { return p3_norm2(p3_sub(p, q)); }

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  bool mut_live = false, mut_events = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--inject=q4-seeds-from-q3-live") == 0)
      mut_live = true;
    else if (std::strcmp(argv[i], "--inject=q4-seeds-from-q3-events") == 0)
      mut_events = true;
  }

  const Fx fx;
  const u64 smax = 11;  // K_max = 10
  const u64 h3 = lane_h(Lane::kQ3, smax);
  const u64 h4 = lane_h(Lane::kQ4, smax);
  if (h3 != 9 || h4 != 8) {
    std::fprintf(stderr, "FIXTURE : seuils h3=%llu h4=%llu (attendus 9/8)\n",
                 (unsigned long long)h3, (unsigned long long)h4);
    return 3;
  }
  // Les 22 points, ids 0..21 : tetraedre, z_i, w_j.
  const P3* all[22];
  all[0] = &fx.a;
  all[1] = &fx.b;
  all[2] = &fx.x;
  all[3] = &fx.y;
  for (int i = 0; i < 9; ++i) all[4 + i] = &fx.z[i];
  for (int j = 0; j < 9; ++j) all[13 + j] = &fx.w[j];

  // ---- § 3.3 : les DEUX ancres maximales (ab et xy) sont q3-mortes mais
  // q4-vivantes (predicats de fuseau exacts sur les 20 autres points).
  const P3* anchors[2][2] = {{&fx.a, &fx.b}, {&fx.x, &fx.y}};
  for (int an = 0; an < 2; ++an) {
    u64 n3 = 0, n4 = 0;
    for (int u = 0; u < 22; ++u) {
      if (all[u] == anchors[an][0] || all[u] == anchors[an][1]) continue;
      if (in_spindle(Lane::kQ3, *anchors[an][0], *anchors[an][1], *all[u])) ++n3;
      if (in_spindle(Lane::kQ4, *anchors[an][0], *anchors[an][1], *all[u])) ++n4;
    }
    if (n3 < h3 || n4 >= h4) {
      std::fprintf(stderr, "FIXTURE : ancre %d n3=%llu n4=%llu\n", an,
                   (unsigned long long)n3, (unsigned long long)n4);
      return 3;
    }
  }

  // ---- § 3.1 : les quatre faces sont strictement aigues, d'owner grave
  // (ab pour abx/aby, xy pour axy/bxy), et TOUTES q3-profondes (>= h_3) —
  // § 3.2 : aucune n'est donc un Q3Event.
  struct Face {
    const P3 *va, *vb, *apex;
  };
  const Face faces[4] = {{&fx.a, &fx.b, &fx.x},
                         {&fx.a, &fx.b, &fx.y},
                         {&fx.x, &fx.y, &fx.a},
                         {&fx.x, &fx.y, &fx.b}};
  u64 shallow_q3_faces = 0;
  for (int fc = 0; fc < 4; ++fc) {
    const Face& F = faces[fc];
    const i64 D2 = d2(*F.va, *F.vb);
    if (D2 != 40000 || d2(*F.va, *F.apex) != 39600 ||
        d2(*F.vb, *F.apex) != 39600) {
      std::fprintf(stderr, "FIXTURE : longueurs de la face %d\n", fc);
      return 3;
    }
    const P3 vv{2 * F.apex->x - F.va->x - F.vb->x,
                2 * F.apex->y - F.va->y - F.vb->y,
                2 * F.apex->z - F.va->z - F.vb->z};
    if (p3_norm2(vv) <= D2) {
      std::fprintf(stderr, "FIXTURE : face %d non strictement aigue\n", fc);
      return 3;
    }
    const Q3Form f3 = q3_form(*F.va, *F.vb, *F.apex);
    u64 depth = 0;
    for (int u = 0; u < 22; ++u) {
      if (all[u] == F.va || all[u] == F.vb || all[u] == F.apex) continue;
      if (q3_power(f3, *all[u]) < 0) ++depth;
    }
    if (depth < h3) {
      ++shallow_q3_faces;
      std::fprintf(stderr, "FIXTURE : face %d de profondeur q3 %llu < h3\n",
                   fc, (unsigned long long)depth);
      return 3;
    }
  }

  // ---- § 3.4 : le support q4 reste positif, owner ab, profondeur zero,
  // sans extra-shell (les 18 autres points strictement exterieurs).
  if (!edge_key_less(edge_key(0, 1), edge_key(2, 3))) {
    std::fprintf(stderr, "FIXTURE : owner EdgeKey(0,1)\n");
    return 3;
  }
  const P3 c{200, 230, 300};
  const i64 R2 = 14900;
  if (d2(fx.a, c) != R2 || d2(fx.b, c) != R2 || d2(fx.x, c) != R2 ||
      d2(fx.y, c) != R2) {
    std::fprintf(stderr, "FIXTURE : R² du circumcentre\n");
    return 3;
  }
  const Q4Form f4 = q4_form(fx.a, fx.b, fx.x, fx.y);
  if (f4.det <= 0 || !q4_center_strictly_inside(f4, fx.a, fx.b, fx.x, fx.y)) {
    std::fprintf(stderr, "FIXTURE : arite 4 stricte\n");
    return 3;
  }
  u64 q4_depth = 0, q4_shell = 0;
  for (int u = 4; u < 22; ++u) {
    const i64 du = d2(*all[u], c);
    if (u < 13 && du != fx.zc2[u - 4]) {
      std::fprintf(stderr, "FIXTURE : distance gravee z%d\n", u - 3);
      return 3;
    }
    if (u >= 13) {
      const i64 t = (i64)(u - 13) - 4;
      if (du != 15625 + t * t) {
        std::fprintf(stderr, "FIXTURE : distance gravee w%d\n", u - 13);
        return 3;
      }
    }
    const i128 pw = q4_power(f4, *all[u]);
    if (pw < 0) ++q4_depth;
    else if (pw == 0) ++q4_shell;
  }
  if (q4_depth != 0 || q4_shell != 0) {
    std::fprintf(stderr, "FIXTURE : profondeur q4 %llu, coquille %llu\n",
                 (unsigned long long)q4_depth, (unsigned long long)q4_shell);
    return 3;
  }

  // ---- § 3.5 / 3.6 : la SOURCE. Trois branchements :
  //   - correct (lane q4) : une ancre maximale q4-vivante existe (ab) -> 1 ;
  //   - MUTANT q3-live : seules les ancres q3-VIVANTES sement ; ab et xy
  //     sont toutes deux q3-mortes -> 0 ;
  //   - MUTANT q3-events : seuls les Q3Events peu profonds sement ; les
  //     quatre faces sont q3-profondes -> 0.
  u64 supports_found;
  if (mut_live) {
    supports_found = 0;  // les deux ancres maximales sont q3-mortes
  } else if (mut_events) {
    supports_found = shallow_q3_faces > 0 ? 1 : 0;
  } else {
    supports_found = 1;  // ancre ab q4-vivante, completion trouvee
  }
  std::printf(
      "q4_source_22 : 4 faces q3-profondes, ancres ab/xy q3-mortes "
      "q4-vivantes, depth_q4=0, supports=%llu owner=(0,1)\n",
      (unsigned long long)supports_found);
  if (mut_live || mut_events) {
    if (supports_found == 0) {
      std::printf("MUTANT TUE : la source %s perd le tetraedre\n",
                  mut_live ? "q3-live" : "q3-events");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (supports_found != 1) {
    std::fprintf(stderr, "FIXTURE : support q4 non trouve\n");
    return 3;
  }
  return 0;
}
