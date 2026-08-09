// MorseHGP3D v3 — LA SÉLECTIVITÉ DU LEMME DU DEMI-BOULE, mesurée.
//
// La spécification du générateur direct repose sur une condition nécessaire exacte,
// démontrée sans hypothèse de régularité ni de zéro-extra-shell :
//
//   Si p et u appartiennent à la coquille d'une sphère critique de rang au plus
//   s_max, alors il existe un demi-espace fermé H dont le plan contient la droite
//   (p,u) tel que |X inter D_pu inter H| <= s_max, où D_pu est la boule diamétrale.
//
// La preuve tient en trois lignes : le centre c est équidistant de p et u, donc
// (c-m) est perpendiculaire à (u-p) ; d'où R² = rho² + d² ; et pour tout y du
// demi-boule du côté du centre, ||y-c||² = ||y-m||² - 2(y-m).(c-m) + d² <= R².
//
// ---------------------------------------------------------------------------
// CE QUE CE PROGRAMME MESURE, ET POURQUOI C'EST LUI QUI DÉCIDE
// ---------------------------------------------------------------------------
//
// Le budget du générateur direct dépend de deux quantités que la spécification
// déclare elle-même NON BORNÉES : la taille de l'univers admissible et celle des
// candidats d'arité trois. Si l'univers admissible est de l'ordre de n², la route
// meurt quel que soit le débit ; s'il est de l'ordre de la sortie, elle vit.
//
// On mesure donc trois nombres sur le même nuage :
//   * VRAI    : les paires qui apparaissent réellement dans une coquille critique,
//               lues sur le catalogue exact de référence ;
//   * ADMIS   : les paires que le lemme laisse passer ;
//   * TOTAL   : les C(n,2) paires.
// La sélectivité utile est ADMIS/VRAI — le facteur de travail gaspillé —, et
// ADMIS/n dit si l'univers est linéaire ou quadratique.
//
// Le minimum sur les demi-espaces se calcule exactement : les points de la boule
// diamétrale se projettent sur le plan perpendiculaire à (u-p), et un demi-espace
// dont le plan contient la droite devient un demi-plan de cette projection. Le
// minimum sur les demi-plans est donc un minimum de fenêtre circulaire, atteint sur
// l'une des directions délimitées par les points eux-mêmes. Aucun angle n'est
// calculé : les comparaisons sont des signes de déterminants entiers.
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_flats.hpp"

using mhgp::P3;
using mhgp::i32;
using mhgp::i128;

// Le minimum, sur les demi-plans fermés bordés par une droite passant par
// l'origine, du nombre de points d'un ensemble planaire. Les points sont donnés par
// leurs coordonnées entières dans une base du plan ; un point à l'origine est dans
// TOUS les demi-plans et se compte à part.
static int minimum_halfplane_count(const std::vector<std::pair<i128, i128>>& projected,
                                   int always_inside) {
  const int m = (int)projected.size();
  if (m == 0) return always_inside;
  // Pour chaque direction candidate d (celle d'un point, et son opposée), compter
  // les points y tels que d.y >= 0 au sens du produit croisé : le demi-plan bordé
  // par la droite portée par d, du côté positif.
  int best = m + always_inside;
  for (int i = 0; i < m; ++i) {
    for (int flip = 0; flip < 2; ++flip) {
      const i128 dx = flip ? -projected[(std::size_t)i].first : projected[(std::size_t)i].first;
      const i128 dy = flip ? -projected[(std::size_t)i].second : projected[(std::size_t)i].second;
      if (dx == 0 && dy == 0) continue;
      int count = 0;
      for (int j = 0; j < m; ++j) {
        // Demi-plan { y : cross(d, y) >= 0 }, fermé.
        const i128 cross = dx * projected[(std::size_t)j].second -
                           dy * projected[(std::size_t)j].first;
        if (cross >= 0) ++count;
      }
      best = std::min(best, count + always_inside);
    }
  }
  return best;
}

int main(int argc, char** argv) {
  int n = 200, smax = 11, repeats = 1;
  long long seed = 20260809;
  const double density = 1.0e-3;
  auto integer = [](const char* text, long long* value) {
    const char* first = text;
    const char* last = text + strlen(text);
    if (first == last) return false;
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(first, last, magnitude);
    if (r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 100000000ULL) return false;
    *value = (long long)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    long long value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    int* target = nullptr;
    if (!strcmp(argv[i], "--points")) target = &n;
    else if (!strcmp(argv[i], "--smax")) target = &smax;
    else if (!strcmp(argv[i], "--repeats")) target = &repeats;
    else if (!strcmp(argv[i], "--seed")) {
      if (!has) { printf("ECHEC : --seed invalide\n"); return 2; }
      ++i; seed = value; continue;
    } else { printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { printf("ECHEC : valeur invalide pour %s\n", argv[i]); return 2; }
    ++i;
    *target = (int)value;
  }
  if (n < 8 || n > 5000 || smax < 3 || smax > mhgp::kMaxRank || repeats < 1 || repeats > 20) {
    printf("ECHEC : campagne absurde\n");
    return 2;
  }

  const double volume = (double)n / density;
  const int span = (int)std::llround(std::cbrt(volume));
  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, span - 1);

  long long sum_true = 0, sum_admitted = 0, sum_total = 0, sum_missing = 0;
  long long sum_ball_points = 0;
  double sum_seconds = 0;
  int decided = 0;

  for (int r = 0; r < repeats; ++r) {
    std::vector<P3> pts;
    {
      std::vector<long long> keys;
      for (int guard = 0; (int)pts.size() < n && guard < 60 * n; ++guard) {
        P3 q{};
        q.x = (i32)pick(rng); q.y = (i32)pick(rng); q.z = (i32)pick(rng);
        const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
        if (std::find(keys.begin(), keys.end(), key) != keys.end()) continue;
        keys.push_back(key);
        pts.push_back(q);
      }
    }
    if ((int)pts.size() < n) { printf("ECHEC : nuage non genere\n"); return 3; }

    // LA VÉRITÉ : les paires qui apparaissent dans une coquille critique.
    mhgp3v::FlatStatistics st{};
    mhgp3v::CloudStatus status = mhgp3v::CloudStatus::kOk;
    const mhgp::Catalogue cat = mhgp3v::flat_catalogue(pts, smax, &st, &status, false, true);
    if (status != mhgp3v::CloudStatus::kOk) {
      printf("  statut %s : nuage ignore\n", mhgp3v::cloud_status_name(status));
      continue;
    }
    std::set<std::pair<i32, i32>> truth;
    for (const auto& sphere : cat.spheres) {
      std::vector<i32> shell;
      for (int i = 0; i < sphere.rank; ++i) {
        const i32 z = cat.members[(std::size_t)(sphere.members_begin + i)];
        if (mhgp::sphere_side(sphere.sph, pts[(std::size_t)z]) == 0) shell.push_back(z);
      }
      for (std::size_t a = 0; a < shell.size(); ++a)
        for (std::size_t b = a + 1; b < shell.size(); ++b)
          truth.insert({shell[a], shell[b]});
    }

    // LE FILTRE : le lemme du demi-boule, sur toutes les paires.
    const auto t0 = std::chrono::steady_clock::now();
    long long admitted = 0, missing = 0, ball_points = 0;
    std::vector<std::pair<i128, i128>> projected;
    for (i32 a = 0; a < n; ++a)
      for (i32 b = a + 1; b < n; ++b) {
        const P3& pa = pts[(std::size_t)a];
        const P3& pb = pts[(std::size_t)b];
        // Boule diametrale exacte : centre (pa+pb)/2, rayon |pb-pa|/2. Le test
        // « z dedans » s'ecrit sans division : |2z - pa - pb|^2 <= |pb - pa|^2.
        const i128 dx = (i128)pb.x - pa.x, dy = (i128)pb.y - pa.y, dz = (i128)pb.z - pa.z;
        const i128 diameter2 = dx * dx + dy * dy + dz * dz;
        projected.clear();
        int inside_line = 0;
        for (i32 z = 0; z < n; ++z) {
          if (z == a || z == b) continue;
          const P3& pz = pts[(std::size_t)z];
          const i128 ex = 2 * (i128)pz.x - pa.x - pb.x;
          const i128 ey = 2 * (i128)pz.y - pa.y - pb.y;
          const i128 ez = 2 * (i128)pz.z - pa.z - pb.z;
          if (ex * ex + ey * ey + ez * ez > diameter2) continue;
          // Projection sur le plan perpendiculaire a (pb - pa) : deux coordonnees
          // entieres obtenues par produits croises, sans division.
          const i128 cx = dy * ez - dz * ey;
          const i128 cy = dz * ex - dx * ez;
          const i128 cz = dx * ey - dy * ex;
          if (cx == 0 && cy == 0 && cz == 0) { ++inside_line; continue; }
          // Base entiere du plan : (u, v) avec u = d x e0 pour un axe e0 non
          // colineaire a d, et v = d x u. Les coordonnees du point projete sont
          // (c.u, c.v) ou c est le produit croise ci-dessus.
          i128 ux, uy, uz;
          if (dx != 0 || dy != 0) { ux = -dy; uy = dx; uz = 0; }
          else { ux = 1; uy = 0; uz = 0; }
          const i128 vx = dy * uz - dz * uy;
          const i128 vy = dz * ux - dx * uz;
          const i128 vz = dx * uy - dy * ux;
          projected.push_back({cx * ux + cy * uy + cz * uz, cx * vx + cy * vy + cz * vz});
        }
        ball_points += (long long)projected.size() + inside_line + 2;
        // Les deux extremites sont dans TOUS les demi-espaces bordes par leur droite.
        const int least = minimum_halfplane_count(projected, inside_line + 2);
        if (least <= smax) {
          ++admitted;
        } else if (truth.count({a, b}) != 0) {
          ++missing;                 // le lemme aurait refute une paire VRAIE
        }
      }
    const auto t1 = std::chrono::steady_clock::now();

    ++decided;
    sum_true += (long long)truth.size();
    sum_admitted += admitted;
    sum_total += (long long)n * (n - 1) / 2;
    sum_missing += missing;
    sum_ball_points += ball_points;
    sum_seconds += std::chrono::duration<double>(t1 - t0).count();
  }

  if (decided == 0) { printf("ECHEC : aucun nuage decide\n"); return 3; }
  const double d = (double)decided;
  printf("n=%d smax=%d emprise=%d^3 nuages=%d\n", n, smax, span, decided);
  printf("  paires totales=%.0f  ADMISES=%.0f (%.3f%%)  VRAIES=%.0f (%.3f%%)\n",
         sum_total / d, sum_admitted / d, 100.0 * (double)sum_admitted / (double)sum_total,
         sum_true / d, 100.0 * (double)sum_true / (double)sum_total);
  printf("  admises/vraies=%.2f   admises/point=%.1f   vraies/point=%.1f\n",
         sum_true ? (double)sum_admitted / (double)sum_true : 0.0,
         sum_admitted / d / n, sum_true / d / n);
  printf("  vraies REFUTEES par le lemme=%.0f  (doit etre ZERO : le lemme est necessaire)\n",
         sum_missing / d);
  printf("  points de boule diametrale visites=%.0f  secondes=%.2f\n", sum_ball_points / d,
         sum_seconds / d);
  if (sum_missing != 0) {
    printf("ECHEC : le lemme du demi-boule a refute %lld paires reelles — il est FAUX\n",
           sum_missing);
    return 1;
  }
  printf("OK : aucune paire reelle refutee\n");
  return 0;
}
