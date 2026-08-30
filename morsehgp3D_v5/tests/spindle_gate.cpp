// MorseHGP3D v5 — porte des fuseaux et du comptage de temoins.
//
// Juge FAIL-OPEN : pour chaque rectangle terminal A×B de la WSPD et chaque
// lane q, le compte fusionne du cœur (temoins universels certifies) est un
// MINORANT de |P ∩ W_q(a,b)| pour TOUTE ancre (a,b) ∈ A×B — verifie contre
// le juge ponctuel `true_spindle_count` sur un echantillon d'ancres par
// rectangle. Une seule fausse mort (compte > vrai) est un desaccord.
// Planchers : --min-dead-pct (rectangles morts par le cœur, par lane q2),
// --min-rect. Fixtures gravees : appartenance ponctuelle a W_2/W_3/W_4
// (emboitement, coquille exclue), boule-cœur incluse dans le fuseau.
// Mutants : `core-ball-ceil-distance` (distance majoree : credits fantomes),
// `witness-no-lane-mask` (doubles credits) — tues par le juge (code 4).
// Codes : 0 conforme, 1 desaccord du juge, 2 refus, 3 plancher/invariant, 4 mutant.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/mutants.hpp"
#include "../src/spindle/witness_count.hpp"
#include "../src/wspd/wavefront.hpp"

using namespace mhgp5;

namespace {

int g_fail = 0;
void check(bool ok, const char* what) {
  if (!ok) {
    std::fprintf(stderr, "INVARIANT VIOLE : %s\n", what);
    ++g_fail;
  }
}

// Fixtures ponctuelles : a=(0,0,0), b=(100,0,0).
void fixtures() {
  const P3 a{0, 0, 0}, b{100, 0, 0};
  // Milieu : dans les trois fuseaux.
  check(in_spindle(Lane::kQ2, a, b, P3{50, 0, 0}) && in_spindle(Lane::kQ3, a, b, P3{50, 0, 0}) &&
            in_spindle(Lane::kQ4, a, b, P3{50, 0, 0}),
        "milieu hors fuseau");
  // Coquille de la boule diametrale : H = 0, exclue partout.
  check(!in_spindle(Lane::kQ2, a, b, P3{50, 50, 0}) && !in_spindle(Lane::kQ2, a, b, a), "coquille comptee");
  // Emboitement W_4 ⊂ W_3 ⊂ W_2 : z=(50,20,0) — H = 50·50 − (50²+20²) = 2100 ; Xi = |d×w|² = (100·20)² = 4e6 ;
  // 3H² = 13,23e6 > Xi (q3 oui) ; 2H² = 8,82e6 > Xi (q4 oui).
  check(in_spindle(Lane::kQ3, a, b, P3{50, 20, 0}) && in_spindle(Lane::kQ4, a, b, P3{50, 20, 0}), "emboitement");
  // z=(50,30,0) : H = 5000−3400 = 1600 ; Xi = 9e6 ; 3H² = 7,68e6 < Xi -> hors q3 et q4, dans q2.
  check(in_spindle(Lane::kQ2, a, b, P3{50, 30, 0}) && !in_spindle(Lane::kQ3, a, b, P3{50, 30, 0}) &&
            !in_spindle(Lane::kQ4, a, b, P3{50, 30, 0}),
        "frontiere q3");
  // z=(50,26,0) : H = 5000−3376 = 1624 ; Xi = 6,76e6 ; 3H² = 7,912e6 > Xi (q3) ; 2H² = 5,275e6 < Xi (pas q4).
  check(in_spindle(Lane::kQ3, a, b, P3{50, 26, 0}) && !in_spindle(Lane::kQ4, a, b, P3{50, 26, 0}), "frontiere q4");
  // Boule-cœur d'un couple de boites ponctuelles : incluse dans le fuseau
  // (echantillon de points de la boule -> in_spindle).
  const AxisBox A{{0, 0, 0}, {0, 0, 0}}, B{{100, 0, 0}, {100, 0, 0}};
  for (const Lane q : kLanes) {
    const CoreBall cb = core_ball(q, A, B);
    check(cb.radius4 > 0, "boule-cœur vide sur ancre ponctuelle");
    for (i64 x = 0; x <= 100; ++x)
      for (i64 y = -60; y <= 60; ++y)
        for (i64 z = -60; z <= 60; z += 3) {
          const P3 p{x, y, z};
          if (point_in_ball(p, cb)) check(in_spindle(q, a, b, p), "boule-cœur deborde du fuseau");
        }
  }
}

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool ok = true;
  int n = 1200;
  int coord = 0;
  long long seed = 3;
  i64 s = 8;
  u64 smax = 11;
  u64 min_rect = 1;
  int min_dead_pct = 0;
  int sample = 4;
  std::string inject;
};

}  // namespace

int main(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--family=")) a.ok = parse_cloud_family(v, &a.family) && a.ok;
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--s=")) a.s = std::atoll(v);
    else if (const char* v = val("--smax=")) a.smax = (u64)std::atoll(v);
    else if (const char* v = val("--min-rect=")) a.min_rect = (u64)std::atoll(v);
    else if (const char* v = val("--min-dead-pct=")) a.min_dead_pct = std::atoi(v);
    else if (const char* v = val("--sample=")) a.sample = std::atoi(v);
    else if (const char* v = val("--inject=")) a.inject = v;
    else a.ok = false;
  }
  if (!a.ok || a.n < 3 || a.s < 1 || a.smax < 4 || a.smax > 11) return 2;
  if (!a.inject.empty() && !mutants_enable(a.inject)) return 2;
  fixtures();
  if (g_fail) return 3;

  const int coord = a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n);
  const std::vector<P3> pts = make_family_cloud(a.family, a.n, coord, a.seed);
  if (pts.size() < 3) return 2;
  const CloudIndex ix = build_cloud_index(pts);
  if (!ix.valid) return 2;
  const u64 h[3] = {lane_h(Lane::kQ2, a.smax), lane_h(Lane::kQ3, a.smax), lane_h(Lane::kQ4, a.smax)};
  u64 rects = 0, dead[3] = {0, 0, 0}, judged = 0, false_deaths = 0, over_credit = 0;
  std::vector<WspdRect> terminal;
  wspd_wavefront(ix, a.s, 1, [&](const WspdRect& r) { terminal.push_back(r); });
  rects = terminal.size();
  u64 rr = 12345;
  for (const WspdRect& r : terminal) {
    const FusedCounts fc = count_universal_witnesses(ix, r.a, r.b, h, 0b111, true);
    for (int li = 0; li < 3; ++li)
      if (fc.c[li] >= h[li]) ++dead[li];
    // Echantillon d'ancres du rectangle.
    const NodeRange ra = ix.range_of(r.a), rb = ix.range_of(r.b);
    for (int k = 0; k < a.sample; ++k) {
      rr = rr * 6364136223846793005ull + 1442695040888963407ull;
      const i32 ua = ra.first + (i32)((rr >> 33) % (u64)(ra.last - ra.first + 1));
      rr = rr * 6364136223846793005ull + 1442695040888963407ull;
      const i32 ub = rb.first + (i32)((rr >> 33) % (u64)(rb.last - rb.first + 1));
      ++judged;
      for (int li = 0; li < 3; ++li) {
        const u64 truth = true_spindle_count(kLanes[li], ix, ua, ub, h[li]);
        if (fc.c[li] > truth) {
          ++over_credit;
          if (fc.c[li] >= h[li] && truth < h[li]) ++false_deaths;
        }
      }
      if (ra.first == ra.last && rb.first == rb.last) break;
    }
  }
  std::printf("famille=%s n=%d s=%lld smax=%llu rectangles=%llu morts_q2=%llu morts_q3=%llu morts_q4=%llu juges=%llu "
              "surcredits=%llu fausses_morts=%llu\n",
              cloud_family_name(a.family), a.n, (long long)a.s, (unsigned long long)a.smax, (unsigned long long)rects,
              (unsigned long long)dead[0], (unsigned long long)dead[1], (unsigned long long)dead[2],
              (unsigned long long)judged, (unsigned long long)over_credit, (unsigned long long)false_deaths);
  if (over_credit != 0) {
    std::fprintf(stderr, "%s : %llu sur-credits (le minorant n'en est pas un)\n",
                 a.inject.empty() ? "DESACCORD DU JUGE" : "MUTANT TUE", (unsigned long long)over_credit);
    return a.inject.empty() ? 1 : 4;
  }
  if (!a.inject.empty()) {
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", a.inject.c_str());
    return 3;
  }
  if (rects < a.min_rect) return 3;
  if (a.min_dead_pct > 0 && dead[0] * 100 < (u64)a.min_dead_pct * rects) {
    std::fprintf(stderr, "PLANCHER : %llu rectangles morts q2 < %d %% de %llu\n", (unsigned long long)dead[0],
                 a.min_dead_pct, (unsigned long long)rects);
    return 3;
  }
  std::printf("spindle_gate OK\n");
  return 0;
}
