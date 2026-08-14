// MorseHGP3D v3 — ORACLE DE FORCE BRUTE q4, TOTALEMENT INDEPENDANT DU SUJET.
//
// Cadre : phase=exploration_v3_hors_registre, backend=oracle_borne,
//         profile=quantized_u16_input_only, mode=verite_terrain_petite_taille,
//         public_status=not_claimed.
//
// Le contre-audit `AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`
// section 11 reproche a juste titre au « controle » interne du probe de
// partager les lambdas geometriques, les terminaux et les fates du sujet. Ce
// fichier ne partage RIEN : autre unite de traduction, autre owner (l'indice
// EST le PointId, il n'y a ni tri Morton ni rang), aucun terminal, aucun fate,
// aucune fenetre. Il n'a besoin que du generateur de nuage.
//
// CE QU'IL ETABLIT, ET QUI RETIRE UN DE MES PROPRES TITRES. `M4_apex` somme
// `|Q_e|` sur les aretes owner. Or chaque 4-sous-ensemble possede EXACTEMENT
// une arete owner. La somme sur TOUTES les aretes est donc le nombre de
// 4-sous-ensembles non coplanaires portant une face aigue incidente a leur
// arete maximale — c'est-a-dire `Theta(n^4)` pour n'importe quel nuage. Les
// mesures ci-dessous donnent un rapport `M4/C(n,4)` autour de `0,64`, stable
// entre familles et entre tailles.
//
// « `M4` est cubique sur les amas » etait donc un faux titre : ce que mesurait
// la pente etait la fraction d'aretes owner que le prune universel laisse
// OUVERTES, ponderee par la taille de leur lentille. Le vrai contenu est que le
// prune echoue sur les LONGUES aretes, et qu'une longue arete porte `Theta(n^2)`
// couples d'apex.
//
// Force brute independante : enumere TOUS les 4-sous-ensembles d'un petit nuage
// et compte, sans aucun echantillonnage,
//   - les quadruplets non coplanaires dont l'arete maximale (avec tie-break
//     EdgeKey) porte au moins une face incidente strictement aigue  -> M4_total
//   - ceux dont le circumcentre est strictement interieur            -> W4
//   - ceux dont la sphere circonscrite contient au plus `smax-4` points
//     interieurs                                                     -> H4
// Elle sert a valider le predicat in-sphere et a mesurer l'ecart exact entre
// candidats et sortie. Aucun code du probe n'est reutilise.
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "cloud_families.hpp"

using i128 = __int128;

int main(int argc, char** argv) {
  int n = 60, rangmax = 7;
  std::string fam = "uniform";
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a.rfind("--points=", 0) == 0) n = atoi(a.c_str() + 9);
    else if (a.rfind("--family=", 0) == 0) fam = a.substr(9);
    else if (a.rfind("--rangmax=", 0) == 0) rangmax = atoi(a.c_str() + 10);
  }
  mhgp3v::CloudFamily f = mhgp3v::CloudFamily::kUniform;
  if (fam == "eight_clusters") f = mhgp3v::CloudFamily::kEightClusters;
  else if (fam == "two_lines") f = mhgp3v::CloudFamily::kTwoLines;
  const int coord = mhgp3v::cloud_family_default_coord(f, n);
  auto pts = mhgp3v::make_family_cloud(f, n, coord, 12345);
  const int m = (int)pts.size();
  std::vector<std::array<long long, 3>> p(m);
  for (int i = 0; i < m; ++i) p[i] = {(long long)pts[i].x, (long long)pts[i].y, (long long)pts[i].z};

  auto d2 = [&](int i, int j) {
    const long long dx = p[i][0] - p[j][0], dy = p[i][1] - p[j][1], dz = p[i][2] - p[j][2];
    return dx * dx + dy * dy + dz * dz;
  };
  // EdgeKey sur l'indice (le nuage n'est pas trie ici, l'indice EST le PointId).
  auto ek_less = [&](int a1, int a2, int b1, int b2) {
    int x1 = a1 < a2 ? a1 : a2, x2 = a1 < a2 ? a2 : a1;
    int y1 = b1 < b2 ? b1 : b2, y2 = b1 < b2 ? b2 : b1;
    return (x1 != y1) ? (x1 < y1) : (x2 < y2);
  };

  long long tot4 = 0, m4 = 0, w4 = 0, h4 = 0, maxint = 0;
  double somme_int = 0.0;
  std::vector<long long> hist(64, 0);
  int q[4];
  for (q[0] = 0; q[0] < m; ++q[0])
   for (q[1] = q[0] + 1; q[1] < m; ++q[1])
    for (q[2] = q[1] + 1; q[2] < m; ++q[2])
     for (q[3] = q[2] + 1; q[3] < m; ++q[3]) {
       ++tot4;
       // owner : arete de longueur carree maximale, tie-break EdgeKey.
       int oa = q[0], ob = q[1];
       long long best = d2(q[0], q[1]);
       for (int i = 0; i < 4; ++i)
         for (int j = i + 1; j < 4; ++j) {
           const long long l = d2(q[i], q[j]);
           if (l > best || (l == best && ek_less(q[i], q[j], oa, ob))) {
             best = l; oa = q[i]; ob = q[j];
           }
         }
       int c1 = -1, c2 = -1;
       for (int i = 0; i < 4; ++i) if (q[i] != oa && q[i] != ob) { if (c1 < 0) c1 = q[i]; else c2 = q[i]; }
       // au moins une face incidente strictement aigue
       auto aigu = [&](int x) {
         const long long dd = best, ee = d2(x, oa), xx = d2(x, ob);
         return ee <= dd && xx <= dd && ee + xx > dd;
       };
       if (!aigu(c1) && !aigu(c2)) continue;
       const long long wx = p[ob][0] - p[oa][0], wy = p[ob][1] - p[oa][1], wz = p[ob][2] - p[oa][2];
       const long long ux = p[c1][0] - p[oa][0], uy = p[c1][1] - p[oa][1], uz = p[c1][2] - p[oa][2];
       const long long vx = p[c2][0] - p[oa][0], vy = p[c2][1] - p[oa][1], vz = p[c2][2] - p[oa][2];
       const i128 orient = (i128)wx * ((i128)uy * vz - (i128)uz * vy) -
                           (i128)wy * ((i128)ux * vz - (i128)uz * vx) +
                           (i128)wz * ((i128)ux * vy - (i128)uy * vx);
       if (orient == 0) continue;
       ++m4;
       // bien centre : Gram-Cramer, sans division
       const long long g11 = wx * wx + wy * wy + wz * wz;
       const long long g22 = ux * ux + uy * uy + uz * uz;
       const long long g33 = vx * vx + vy * vy + vz * vz;
       const long long g12 = wx * ux + wy * uy + wz * uz;
       const long long g13 = wx * vx + wy * vy + wz * vz;
       const long long g23 = ux * vx + uy * vy + uz * vz;
       auto det3 = [](i128 a11, i128 a12, i128 a13, i128 a21, i128 a22, i128 a23,
                      i128 a31, i128 a32, i128 a33) {
         return a11 * (a22 * a33 - a23 * a32) - a12 * (a21 * a33 - a23 * a31) +
                a13 * (a21 * a32 - a22 * a31);
       };
       const i128 dg = det3(g11, g12, g13, g12, g22, g23, g13, g23, g33);
       bool pos = false;
       if (dg > 0) {
         const i128 e1 = det3(g11, g12, g13, g22, g22, g23, g33, g23, g33);
         const i128 e2 = det3(g11, g11, g13, g12, g22, g23, g13, g33, g33);
         const i128 e3 = det3(g11, g12, g11, g12, g22, g22, g13, g23, g33);
         if (e1 > 0 && e2 > 0 && e3 > 0 && e1 + e2 + e3 < 2 * dg) pos = true;
       }
       if (!pos) continue;
       ++w4;
       // rang : nombre d'interieurs stricts de la sphere circonscrite
       const i128 nw = (i128)g11, nu = (i128)g22, nv = (i128)g33;
       const i128 k0 = (i128)uy * ((i128)vz * nw - nv * wz) - (i128)uz * ((i128)vy * nw - nv * wy) +
                       nu * ((i128)vy * wz - (i128)vz * wy);
       const i128 k1 = (i128)ux * ((i128)vz * nw - nv * wz) - (i128)uz * ((i128)vx * nw - nv * wx) +
                       nu * ((i128)vx * wz - (i128)vz * wx);
       const i128 k2 = (i128)ux * ((i128)vy * nw - nv * wy) - (i128)uy * ((i128)vx * nw - nv * wx) +
                       nu * ((i128)vx * wy - (i128)vy * wx);
       const i128 k3 = (i128)ux * ((i128)vy * wz - (i128)vz * wy) -
                       (i128)uy * ((i128)vx * wz - (i128)vz * wx) +
                       (i128)uz * ((i128)vx * wy - (i128)vy * wx);
       long long interieurs = 0;
       for (int z = 0; z < m; ++z) {
         if (z == oa || z == ob || z == c1 || z == c2) continue;
         const i128 zx = p[z][0] - p[oa][0], zy = p[z][1] - p[oa][1], zz = p[z][2] - p[oa][2];
         const i128 nz = zx * zx + zy * zy + zz * zz;
         const i128 det4 = -zx * k0 + zy * k1 - zz * k2 + nz * k3;
         if ((orient > 0) ? (det4 < 0) : (det4 > 0)) ++interieurs;
       }
       if (interieurs < (long long)hist.size()) ++hist[(size_t)interieurs];
       somme_int += (double)interieurs;
       if (interieurs > maxint) maxint = interieurs;
       if (interieurs <= rangmax) ++h4;
     }
  std::printf("famille=%s n=%d coord=%d | 4-sous-ensembles=%lld M4_total=%lld W4=%lld"
              " H4(rang<=%d)=%lld\n", fam.c_str(), m, coord, tot4, m4, w4, rangmax, h4);
  std::printf("  M4/C(n,4)=%.4f  W4/M4=%.4f  H4/W4=%.6f  H4/n=%.3f\n",
              (double)m4 / (double)tot4, (double)w4 / (double)m4,
              (double)h4 / (double)w4, (double)h4 / (double)m);
  // LA MARGE DU REJET. Un candidat rejete ne l'est pas de justesse : la
  // profondeur moyenne de sa sphere depasse le seuil de deux ordres de
  // grandeur. C'est exactement ce qui laisse de la place a un certificat de
  // BLOC conservateur — il n'a qu'a certifier « au moins huit », alors que la
  // valeur typique est cent fois plus grande.
  std::printf("  interieurs_moyen=%.2f max=%lld seuil=%d marge=%.1fx\n",
              (w4 == 0) ? 0.0 : somme_int / (double)w4, maxint, rangmax,
              (w4 == 0 || rangmax == 0) ? 0.0 : somme_int / (double)w4 / (double)rangmax);
  std::printf("  histogramme des interieurs (0..15) :");
  for (int i = 0; i < 16; ++i) std::printf(" %lld", hist[(size_t)i]);
  std::printf("\n");
  return 0;
}
