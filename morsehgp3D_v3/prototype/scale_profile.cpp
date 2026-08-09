// MorseHGP3D v3 — LA MESURE QUI DÉCIDE : le terrain croît-il, et la sortie ?
//
// Le contrat est 50 000 points, exact, en moins de 100 ms pour TOUTE la chaîne. La
// question n'est donc pas « à quelle vitesse va-t-on » mais « combien y a-t-il à
// faire », et cette question n'a qu'une réponse mesurable : le nombre de sommets
// d'arrangement par point, et le nombre de sphères critiques par point, quand $n$
// croît à DENSITÉ FIXE.
//
// ---------------------------------------------------------------------------
// POURQUOI MES MESURES PRÉCÉDENTES NE POUVAIENT PAS RÉPONDRE
// ---------------------------------------------------------------------------
//
// Elles tiraient les points dans un cube d'emprise $\propto\sqrt{n}$ par
// coordonnée, donc de volume $\propto n^{3/2}$ : la densité DÉCROISSAIT en
// $n^{-1/2}$. Un tel profil ne dit rien d'une croissance à $n$ fixe de densité, et
// les extrapoler à 50 000 points était une faute de protocole. À densité fixe,
// l'emprise doit croître comme $n^{1/3}$ en chaque coordonnée.
//
// Le profil LiDAR est l'autre régime légitime — une nappe, donc une emprise
// $\propto\sqrt{n}$ en $x$ et $y$ et une épaisseur bornée en $z$. Les deux sont
// offerts ici parce que le contrat vise des nuages réels, et que la réponse peut
// différer entre les deux.
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/order_k_flats.hpp"

using mhgp::P3;
using mhgp::i32;

int main(int argc, char** argv) {
  int n = 200, smax = 11, repeats = 1, lidar = 0;
  long long seed = 20260809;
  double density = 1.0e-3;   // points par unité de volume, tenue FIXE
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
    else if (!strcmp(argv[i], "--lidar")) { lidar = 1; continue; }
    else if (!strcmp(argv[i], "--seed")) {
      if (!has) { printf("ECHEC : --seed invalide\n"); return 2; }
      ++i; seed = value; continue;
    } else { printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    if (!has) { printf("ECHEC : valeur invalide pour %s\n", argv[i]); return 2; }
    ++i;
    *target = (int)value;
  }
  if (n < 8 || n > 200000 || smax < 3 || smax > mhgp::kMaxRank || repeats < 1 || repeats > 100) {
    printf("ECHEC : campagne absurde\n");
    return 2;
  }

  // DENSITÉ FIXE. Volume $\propto n$, donc emprise $\propto n^{1/3}$ en chaque
  // coordonnée ; en profil LiDAR, aire $\propto n$ et épaisseur bornée.
  const double volume = (double)n / density;
  int span_xy = 0, span_z = 0;
  if (lidar) {
    span_z = 40;
    span_xy = (int)std::llround(std::sqrt(volume / (double)span_z));
  } else {
    span_xy = (int)std::llround(std::cbrt(volume));
    span_z = span_xy;
  }
  if (span_xy < 4 || span_xy > 65535 || span_z < 4 || span_z > 65535) {
    printf("ECHEC : emprise %d x %d hors grille declaree\n", span_xy, span_z);
    return 2;
  }

  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick_xy(0, span_xy - 1);
  std::uniform_int_distribution<int> pick_z(0, span_z - 1);

  double sum_vertices = 0, sum_catalogue = 0, sum_arity[5] = {0, 0, 0, 0, 0};
  double sum_seconds_nav = 0, sum_seconds_cat = 0;
  long long decided = 0;

  for (int r = 0; r < repeats; ++r) {
    std::vector<P3> pts;
    pts.reserve((std::size_t)n);
    {
      std::vector<long long> keys;
      keys.reserve((std::size_t)n);
      for (int guard = 0; (int)pts.size() < n && guard < 60 * n; ++guard) {
        P3 q{};
        q.x = (i32)pick_xy(rng); q.y = (i32)pick_xy(rng); q.z = (i32)pick_z(rng);
        const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
        if (std::find(keys.begin(), keys.end(), key) != keys.end()) continue;
        keys.push_back(key);
        pts.push_back(q);
      }
    }
    if ((int)pts.size() < n) { printf("ECHEC : nuage non genere\n"); return 3; }

    mhgp3v::CertifiedIndex tree;
    tree.build(pts, 16);

    mhgp3v::FlatStatistics nav_st{};
    mhgp3v::CloudStatus nav_status = mhgp3v::CloudStatus::kOk;
    const auto t0 = std::chrono::steady_clock::now();
    const auto vertices = mhgp3v::navigate_shallow(pts, smax - 2, &nav_st, &nav_status, false,
                                                   &tree);
    const auto t1 = std::chrono::steady_clock::now();
    if (nav_status != mhgp3v::CloudStatus::kOk) {
      printf("  statut %s : nuage ignore\n", mhgp3v::cloud_status_name(nav_status));
      continue;
    }

    mhgp3v::FlatStatistics cat_st{};
    mhgp3v::CloudStatus cat_status = mhgp3v::CloudStatus::kOk;
    const auto t2 = std::chrono::steady_clock::now();
    const mhgp::Catalogue cat = mhgp3v::flat_catalogue(pts, smax, &cat_st, &cat_status, false,
                                                       true);
    const auto t3 = std::chrono::steady_clock::now();
    if (cat_status != mhgp3v::CloudStatus::kOk) {
      printf("  catalogue %s : nuage ignore\n", mhgp3v::cloud_status_name(cat_status));
      continue;
    }

    ++decided;
    sum_vertices += (double)vertices.size();
    sum_catalogue += (double)cat.spheres.size();
    for (const auto& sphere : cat.spheres)
      if (sphere.n_support >= 1 && sphere.n_support <= 4) sum_arity[sphere.n_support] += 1.0;
    sum_seconds_nav += std::chrono::duration<double>(t1 - t0).count();
    sum_seconds_cat += std::chrono::duration<double>(t3 - t2).count();
  }

  if (decided == 0) { printf("ECHEC : aucun nuage decide\n"); return 3; }
  const double d = (double)decided;
  const double per = (double)n;
  printf("profil=%s n=%d smax=%d emprise=%dx%dx%d densite=%.3g graine=%lld nuages=%lld\n",
         lidar ? "lidar" : "cube", n, smax, span_xy, span_xy, span_z,
         (double)n / ((double)span_xy * span_xy * span_z), seed, decided);
  printf("  sommets/point=%.1f  catalogue/point=%.2f  arites 1/2/3/4 par point ="
         " %.2f/%.2f/%.2f/%.2f\n", sum_vertices / d / per, sum_catalogue / d / per,
         sum_arity[1] / d / per, sum_arity[2] / d / per, sum_arity[3] / d / per,
         sum_arity[4] / d / per);
  printf("  secondes navigation=%.3f  catalogue=%.3f  (un coeur, sans accelerateur)\n",
         sum_seconds_nav / d, sum_seconds_cat / d);
  return 0;
}
