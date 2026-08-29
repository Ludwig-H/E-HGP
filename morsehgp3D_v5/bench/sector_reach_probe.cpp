// V90 — combien de secteurs sur 8 un handle atteint-il ?
// `anchor_sector_kill` recouvre le disque des centres par 8 secteurs et exige
// h_3 temoins dans CHACUN. Si Box(C) n'en atteint qu'un sous-ensemble, exiger le
// seuil sur ceux-la seulement est strictement plus faible, donc tue plus.
// Cette mesure donne le PLANCHER du benefice : elle compte les secteurs
// reellement atteints par les centres EXACTS, a ANCRE FIXE. Un calcul par
// boites en atteindrait davantage (conservateur), donc le vrai gain sera
// moindre — mais si meme ce plancher vaut 7 ou 8, l'idee est morte.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "/workspaces/E-HGP/morsehgp3D_v5/src/cloud/families.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/lanes/edge_cover.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/lanes/q3.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/lanes/sector_kill.hpp"
#include "/workspaces/E-HGP/morsehgp3D_v5/src/pipeline/generate.hpp"
using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 8000, coord = 0; size_t cible = 3000;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { if (!parse_cloud_family(a.c_str() + 9, &family)) return 2; }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--blocs=", 0) == 0) cible = (size_t)std::atoll(a.c_str() + 8);
    else return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  const u64 smax = 11;
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax), lane_h(Lane::kQ3, smax), lane_h(Lane::kQ4, smax)};
  std::vector<AliveRect> alive; u64 vis = 0, wk = 0;
  generate_detail::alive_rectangles(ix, 8, h_of, 1, 1, &alive, &vis, &wk);
  generate_detail::AnchorScratch sc;
  u64 blocs = 0;
  for (const AliveRect& ar : alive) {
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    blocs += (u64)sc.handles.size();
  }
  if (blocs == 0) return 3;
  const u64 pas = std::max<u64>(1, blocs / std::max<u64>(1, (u64)cible));

  u64 vus = 0, groupes = 0, hist[9] = {};
  double somme_secteurs = 0.0, somme_angle = 0.0, angle_max = 0.0;
  for (const AliveRect& ar : alive) {
    const NodeRange ra = ix.range_of(ar.r.a), rb = ix.range_of(ar.r.b);
    rect_cover_handles(ix, ix.box_of(ar.r.a), ix.box_of(ar.r.b), 3, &sc.handles, &sc.cover_nodes);
    for (const NodeRef h : sc.handles) {
      const u64 id = vus++;
      if (id % pas != 0) continue;
      const NodeRange rc = ix.range_of(h);
      for (i32 ua = ra.first; ua <= ra.last; ++ua)
        for (i32 ub = rb.first; ub <= rb.last; ++ub) {
          const P3& pa = ix.upos[(size_t)ua];
          const P3& pb = ix.upos[(size_t)ub];
          const i64 D2 = p3_norm2(p3_sub(pb, pa));
          if (D2 == 0) continue;
          i64 bu[3], bv[3];
          if (!bisector_basis(pa, pb, D2, 12, bu, bv)) continue;
          // Coordonnees du centre dans la base (bu, bv) du plan bissecteur, et
          // secteur d'appartenance parmi 8 (octogone ±u, ±v, ±(u+v), ±(u−v)).
          bool touche[8] = {};
          double ax[64], ay[64]; size_t k = 0;
          for (i32 ux = rc.first; ux <= rc.last && k < 64; ++ux) {
            if (ux == ua || ux == ub) continue;
            const P3& px = ix.upos[(size_t)ux];
            if (!is_acute_seed(pa, pb, px, D2, ix.point_id(ua), ix.point_id(ub), ix.point_id(ux))) continue;
            const Q3Form f = q3_form(pa, pb, px);
            if (f.g <= 0) continue;
            const double g2 = 2.0 * (double)f.g;
            const double cx = (double)f.a.x + (double)f.w[0] / g2 - 0.5 * ((double)pa.x + (double)pb.x);
            const double cy = (double)f.a.y + (double)f.w[1] / g2 - 0.5 * ((double)pa.y + (double)pb.y);
            const double cz = (double)f.a.z + (double)f.w[2] / g2 - 0.5 * ((double)pa.z + (double)pb.z);
            // projection sur la base entiere du plan bissecteur
            const double pu = cx * (double)bu[0] + cy * (double)bu[1] + cz * (double)bu[2];
            const double pv = cx * (double)bv[0] + cy * (double)bv[1] + cz * (double)bv[2];
            if (pu == 0 && pv == 0) continue;
            ax[k] = pu; ay[k] = pv;
            const double ang = std::atan2(pv, pu);
            int s = (int)std::floor((ang + 3.14159265358979) / (2 * 3.14159265358979) * 8.0);
            touche[std::max(0, std::min(7, s))] = true;
            ++k;
          }
          if (k < 1) continue;
          int ns = 0; for (int s = 0; s < 8; ++s) ns += touche[s] ? 1 : 0;
          ++groupes; ++hist[ns]; somme_secteurs += ns;
          double cosmin = 1.0;
          for (size_t t = 0; t < k; ++t)
            for (size_t u2 = t + 1; u2 < k; ++u2) {
              const double d1 = std::sqrt(ax[t]*ax[t]+ay[t]*ay[t]), d2 = std::sqrt(ax[u2]*ax[u2]+ay[u2]*ay[u2]);
              if (d1 > 0 && d2 > 0) cosmin = std::min(cosmin, (ax[t]*ax[u2]+ay[t]*ay[u2])/(d1*d2));
            }
          if (k >= 2) {
            const double a2 = std::acos(std::max(-1.0, std::min(1.0, cosmin))) * 180.0 / 3.14159265358979;
            somme_angle += a2; angle_max = std::max(angle_max, a2);
          }
        }
    }
  }
  std::printf("secteurs famille=%s n=%d : groupes (ancre,handle) avec >=1 centre = %llu\n",
              cloud_family_name(family), n, (unsigned long long)groupes);
  std::printf("  secteurs atteints sur 8 : moyenne %.2f ; histogramme 1..8 :", groupes ? somme_secteurs/(double)groupes : 0.0);
  for (int s = 1; s <= 8; ++s) std::printf(" %llu", (unsigned long long)hist[s]);
  std::printf("\n  ouverture angulaire des centres : %.1f deg en moyenne, %.1f au pire (secteur = 45 deg)\n",
              groupes ? somme_angle/(double)groupes : 0.0, angle_max);
  return 0;
}
