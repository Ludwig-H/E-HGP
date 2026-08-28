// MorseHGP3D v5 — ORACLE des tests d'ancre (V11 de l'auditeur, 27 aout 2026).
// Sur de petits nuages (n <= 60 : familles a coord par defaut et grille serree
// cocirculaire), pour TOUTES les paires (a,b) prises comme ancres (cover exact
// par cover_query, pas seulement les ancres WSPD) :
//   (1) ON/OFF : les corps partages scan_anchor_q3 / process_anchor_q4 avec
//       anchor_tests = true et = false emettent le MEME multiensemble de
//       candidats (cle, niveau) — les tests d'ancre ne changent pas l'objet ;
//   (2) J > 0 pour tout seed q4 aigu (J = D²(3G − 2 l_ax l_bx) >= G D²/3) ;
//   (3) identite de signe P/B : pour un seed (a,b,x) et une completion y non
//       coplanaire (B(y) != 0), sign q4_power(f4, z) = sign B(y) · sign(P(z) B(y)
//       − P(y) B(z)) pour tout site z, avec P(z) = L(z)/4 (kernel affine du
//       seed) et B(z) = n·(z − a), n = (b−a)×(x−a) — en __int128 exact ;
//   (4) non-vacuite : W_3, secteurs q3 et secteurs q4 tuent chacun >= 1 ancre
//       sur l'ensemble des nuages (sinon l'oracle serait vert par vacuite).
// Codes : 0, 1 desaccord, 3 vacuite.
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/q3.hpp"
#include "../src/lanes/q4.hpp"
#include "../src/lanes/sector_kill.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp5;

namespace {
u64 mism_onoff = 0, jneg = 0, sign_mism = 0, sign_checked = 0, killed_w3 = 0, killed_s3 = 0, killed_s4 = 0, anchors = 0;
u64 killed_c3 = 0, killed_c4 = 0, seeds_c3 = 0, seeds_c4 = 0;  // grille de cellules (theoreme 10.5), seuil 0 : grille sur toute ancre
// Mode FORCE (cell_min_sites = 0) : la grille est CONSTRUITE sur toute ancre survivante des pretests
// (V15 des auditeurs : le seuil 0 ne suffisait pas, ratio et near_m restaient actifs) — compte de vacuite.
u64 grids_attempted = 0, grids_built = 0, grid_missing = 0;
int sgn128(i128 v) { return v < 0 ? -1 : v > 0 ? 1 : 0; }

void oracle_cloud(const CloudIndex& ix) {
  const u64 smax = 11;
  const u64 h3 = lane_h(Lane::kQ3, smax), h4 = lane_h(Lane::kQ4, smax);
  const bool float_on = float_filter_runtime_enabled();
  generate_detail::AnchorScratch sc;
  sc.cell_min_sites = 0;  // mode FORCE : la grille de cellules est construite sur TOUTE ancre survivante (ratio et near_m ignores) : ON/OFF la juge aussi
  const i32 m = (i32)ix.upos.size();
  for (i32 ua = 0; ua < m; ++ua)
    for (i32 ub = ua + 1; ub < m; ++ub) {
      const P3& pa = ix.upos[(size_t)ua];
      const P3& pb = ix.upos[(size_t)ub];
      const i64 D2 = p3_norm2(p3_sub(pb, pa));
      if (D2 == 0) continue;
      ++anchors;
      for (int lane = 3; lane <= 4; ++lane) {
        cover_query(ix, pa, pb, D2, 3, &sc.cover);
        sc.affine_filled = false;
        std::vector<BallCandidate> on, off;
        GenerateStats son, soff;
        if (lane == 3) {
          generate_detail::scan_anchor_q3(ix, sc, ua, ub, pa, pb, D2, h3, float_on, false, &on, &son, generate_detail::AnchorPretests::kApply);
          sc.affine_filled = false;
          generate_detail::scan_anchor_q3(ix, sc, ua, ub, pa, pb, D2, h3, float_on, false, &off, &soff, generate_detail::AnchorPretests::kCounterfactual);
          killed_w3 += son.anchors_killed_w3;
          killed_s3 += son.anchors_killed_sectors[1];
          killed_c3 += son.anchors_killed_cells[1];
          seeds_c3 += son.seeds_killed_cells[1];
          grids_attempted += son.grids_attempted[1];
          grids_built += son.grids_built[1];
          if (son.anchors_killed_w3 + son.anchors_killed_sectors[1] == 0 && son.grids_built[1] != 1) ++grid_missing;
        } else {
          generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h4, float_on, false, false, false, &on, &son, generate_detail::AnchorPretests::kApply);
          sc.affine_filled = false;
          generate_detail::process_anchor_q4(ix, sc, ua, ub, pa, pb, D2, h4, float_on, false, false, false, &off, &soff, generate_detail::AnchorPretests::kCounterfactual);
          killed_s4 += son.anchors_killed_sectors[2];
          killed_c4 += son.anchors_killed_cells[2];
          seeds_c4 += son.seeds_killed_cells[2];
          grids_attempted += son.grids_attempted[2];
          grids_built += son.grids_built[2];
          if (son.anchors_killed_w4 + son.anchors_killed_sectors[2] == 0 && son.grids_built[2] != 1) ++grid_missing;
          jneg += soff.invariant_jneg + son.invariant_jneg;
        }
        // (1) meme multiensemble (tri canonique puis comparaison).
        rle_candidates(&on, 1);
        rle_candidates(&off, 1);
        bool same = on.size() == off.size();
        for (size_t i = 0; same && i < on.size(); ++i) same = on[i].key == off[i].key && on[i].level == off[i].level;
        if (!same) ++mism_onoff;
      }
      // (2)-(3) sur les seeds q4 aigus de la lentille (cover coef 3 deja dans sc.cover).
      for (const CoverPoint& cx : sc.cover) {
        if (cx.u == ua || cx.u == ub) continue;
        const P3& px = ix.upos[(size_t)cx.u];
        if (p3_norm2(p3_sub(px, pa)) > D2 || p3_norm2(p3_sub(px, pb)) > D2) continue;
        if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(cx.u))) continue;
        const i64 l_ax = p3_norm2(p3_sub(px, pa)), l_bx = p3_norm2(p3_sub(px, pb));
        const Q3Form f3 = q3_form(pa, pb, px);
        const i128 J = (i128)D2 * (3 * f3.g - 2 * (i128)l_ax * l_bx);
        if (J < 0) ++jneg;
        const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
        // P(z) = q3_power(f3, z) / 4 ? Le kernel affine : L = 4P ; on emploie q3_power (= L/4 exact : P).
        for (const CoverPoint& cy : sc.cover) {
          const P3& py = ix.upos[(size_t)cy.u];
          if (cy.u == ua || cy.u == ub || cy.u == cx.u) continue;
          const i128 By = (i128)p3_dot(nrm, p3_sub(py, pa));
          if (By == 0) continue;  // coplanaire : identite non definie
          const Q4Form f4 = q4_form(pa, pb, px, py);
          if (f4.det == 0) continue;
          const i128 Py = q3_power(f3, py);
          int checked = 0;
          for (const CoverPoint& cz : sc.cover) {
            const P3& pz = ix.upos[(size_t)cz.u];
            const i128 Pz = q3_power(f3, pz);
            const i128 Bz = (i128)p3_dot(nrm, p3_sub(pz, pa));
            const int lhs = sgn128(q4_power(f4, pz));
            const int rhs = sgn128(By) * sgn128(Pz * By - Py * Bz);
            ++sign_checked;
            if (lhs != rhs) ++sign_mism;
            if (++checked >= 12) break;  // borne le cout : 12 sites par completion
          }
        }
      }
    }
}
}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const bool mutant = MHGP5_MUTANT("sector-kill-nonstrict") || MHGP5_MUTANT("anchor-kill-h-minus-one") ||
                      MHGP5_MUTANT("cell-kill-nonstrict") || MHGP5_MUTANT("cell-kill-h-minus-one");
  const struct { CloudFamily f; int n; int coord; } clouds[] = {
      {CloudFamily::kUniform, 50, 0}, {CloudFamily::kEightClusters, 60, 0}, {CloudFamily::kTerrain, 50, 0},
      {CloudFamily::kUniform, 40, 14}, {CloudFamily::kEightClusters, 48, 14}};
  for (const auto& c : clouds) {
    const int coord = c.coord > 0 ? c.coord : cloud_family_default_coord(c.f, c.n);
    const CloudIndex ix = build_cloud_index(make_family_input(c.f, c.n, coord, 3));
    if (!ix.valid || ix.has_duplicate_positions()) continue;
    oracle_cloud(ix);
  }
  std::printf("anchor_tests_oracle ancres=%llu desaccords_on_off=%llu J<0=%llu signes=%llu/%llu W3=%llu secteurs_q3=%llu secteurs_q4=%llu "
              "cellules_ancres=%llu/%llu cellules_seeds=%llu/%llu grilles_tentees=%llu construites=%llu manquantes=%llu\n",
              (unsigned long long)anchors, (unsigned long long)mism_onoff, (unsigned long long)jneg, (unsigned long long)sign_mism,
              (unsigned long long)sign_checked, (unsigned long long)killed_w3, (unsigned long long)killed_s3, (unsigned long long)killed_s4,
              (unsigned long long)killed_c3, (unsigned long long)killed_c4, (unsigned long long)seeds_c3, (unsigned long long)seeds_c4,
              (unsigned long long)grids_attempted, (unsigned long long)grids_built, (unsigned long long)grid_missing);
  // Non-vacuite : chaque test (W3, secteurs q3/q4, grille de cellules en q3 ET en q4 : seeds tues) a agi au moins une fois,
  // et le mode force a construit UNE grille sur CHAQUE ancre survivante des pretests, dans chaque lane.
  if (killed_w3 == 0 || killed_s3 == 0 || killed_s4 == 0 || seeds_c3 == 0 || seeds_c4 == 0 || sign_checked < 1000 || anchors < 1000 ||
      grid_missing != 0 || grids_built != grids_attempted || grids_built < anchors) {
    std::printf("VACUITE\n");
    return 3;
  }
  if (mutant) {
    // Un mutant non strict / h−1 doit produire au moins un desaccord ON/OFF sur ces nuages (dont la grille cocirculaire).
    if (mism_onoff) return 4;
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  if (mism_onoff || jneg || sign_mism) return 1;
  std::printf("anchor_tests_oracle OK\n");
  return 0;
}
