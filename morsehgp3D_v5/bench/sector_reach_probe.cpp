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

  u64 vus = 0, groupes = 0, hist[9] = {}, histb[9] = {}, non_sur = 0;
  double somme_secteurs = 0.0, somme_angle = 0.0, angle_max = 0.0, somme_boites = 0.0;
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
          // --- NIVEAU BOITES (ce qu'une implementation atteindrait vraiment).
          // La base (u,v) est EXACTE puisque le test est par ancre ; seul x
          // varie. Et u, v sont orthogonaux a d, donc p_x . u = (x - m) . u :
          // LINEAIRE en x, donc un intervalle exact par axe sur Box(C).
          // On double tout pour rester entier : 2(x-m) = 2x - (a+b).
          const AxisBox bC = ix.box_of(h);
          i64 Pu_lo = 0, Pu_hi = 0, Pv_lo = 0, Pv_hi = 0;
          for (int i = 0; i < 3; ++i) {
            const i64 s2 = (i == 0 ? pa.x + pb.x : i == 1 ? pa.y + pb.y : pa.z + pb.z);
            const i64 lo = 2 * bC.lo[i] - s2, hi = 2 * bC.hi[i] - s2;
            const i64 cu = bu[i], cv = bv[i];
            Pu_lo += std::min(cu * lo, cu * hi); Pu_hi += std::max(cu * lo, cu * hi);
            Pv_lo += std::min(cv * lo, cv * hi); Pv_hi += std::max(cv * lo, cv * hi);
          }
          bool touche_b[8] = {};
          if (Pu_lo <= 0 && Pu_hi >= 0 && Pv_lo <= 0 && Pv_hi >= 0) {
            for (int t2 = 0; t2 < 8; ++t2) touche_b[t2] = true;  // rectangle contenant l'origine
          } else {
            // Un convexe ne contenant pas l'origine tient dans un demi-plan par
            // l'origine : son image angulaire est donc un ARC de largeur < 180
            // degres, entierement determine par les QUATRE coins. Pas
            // d'echantillonnage — un balayage de frontiere pouvait sauter un
            // secteur entre deux points, et c'est ce qui produisait des
            // violations de surete.
            const double cs[4][2] = {{(double)Pu_lo, (double)Pv_lo}, {(double)Pu_hi, (double)Pv_lo},
                                     {(double)Pu_hi, (double)Pv_hi}, {(double)Pu_lo, (double)Pv_hi}};
            double an[4];
            for (int t2 = 0; t2 < 4; ++t2) an[t2] = std::atan2(cs[t2][1], cs[t2][0]);
            // Arc minimal contenant les quatre angles : on essaie chaque coin
            // comme origine de l'arc et on retient le plus court.
            double best_lo = an[0], best_w = 1e18;
            for (int t2 = 0; t2 < 4; ++t2) {
              double w = 0;
              for (int u2 = 0; u2 < 4; ++u2) {
                double dd = an[u2] - an[t2];
                while (dd < 0) dd += 2 * 3.14159265358979;
                w = std::max(w, dd);
              }
              if (w < best_w) { best_w = w; best_lo = an[t2]; }
            }
            const double pas2 = 2 * 3.14159265358979 / 8.0;
            for (int t2 = 0; t2 < 8; ++t2) {
              // secteur t2 = [-pi + t2*pas, -pi + (t2+1)*pas] ; intersecte-t-il l'arc ?
              const double s_lo = -3.14159265358979 + t2 * pas2, s_hi = s_lo + pas2;
              // Elargissement par epsilon : une direction exactement SUR une
              // frontiere de secteur doit marquer les DEUX secteurs adjacents.
              // C'est le sens conservateur — on en marque plus, jamais moins —
              // et c'est la seule instabilite reelle, celle de q_x proche de 0.
              const double eps = 1e-9;
              for (int shift = -1; shift <= 1; ++shift) {
                const double a_lo = best_lo + shift * 2 * 3.14159265358979 - eps, a_hi = a_lo + best_w + 2 * eps;
                if (a_lo <= s_hi && s_lo <= a_hi) { touche_b[t2] = true; break; }
              }
            }
          }
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
          int nb = 0; for (int s = 0; s < 8; ++s) nb += touche_b[s] ? 1 : 0;
          bool sur = true;  // le niveau boites doit CONTENIR le niveau exact
          for (int s = 0; s < 8; ++s) if (touche[s] && !touche_b[s]) sur = false;
          if (!sur) ++non_sur;
          ++groupes; ++hist[ns]; somme_secteurs += ns;
          ++histb[nb]; somme_boites += nb;
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
  std::printf("\n  NIVEAU BOITES (ce qu'une implementation atteindrait) : moyenne %.2f ; histogramme 1..8 :",
              groupes ? somme_boites/(double)groupes : 0.0);
  for (int s = 1; s <= 8; ++s) std::printf(" %llu", (unsigned long long)histb[s]);
  std::printf("\n  surete (boites contient exact) : %llu violations [doit valoir 0]\n", (unsigned long long)non_sur);
  std::printf("  ouverture angulaire des centres : %.1f deg en moyenne, %.1f au pire (secteur = 45 deg)\n",
              groupes ? somme_angle/(double)groupes : 0.0, angle_max);
  return 0;
}
