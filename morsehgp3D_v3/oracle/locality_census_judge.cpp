// MorseHGP3D v3 — LE JUGE INDEPENDANT DU CENSUS DES TROIS ARITES.
//
// Pourquoi ce binaire existe
// --------------------------
// Le juge interne de `prototype/certified_locality_probe.cpp` partage ses
// constructeurs de sphere et ses predicats avec le sujet : il compare des
// cardinalites obtenues par la MEME arithmetique. Un defaut commun ne s'y
// compenserait pas, il s'y annulerait. L'audit du 12 aout l'a explicitement
// refuse comme reception.
//
// Ce binaire recalcule le meme census avec l'arithmetique de l'oracle :
// rationnels multiprecision, elimination de GAUSS, centre explicite et
// comparaison `||z-c||^2` contre `R^2`. Aucun determinant de Cramer, aucun
// InSphere developpe, aucun `__int128`, aucune borne de Jung. Seul le
// generateur de nuages est partage — c'est l'autorite de generation unique
// voulue par le dossier, pas une primitive de decision.
//
// Ce qu'il calcule
// ----------------
// Pour un nuage et un `K` donnes, le nombre de supports propres positifs de
// taille 2, 3 et 4 dont la miniboule contient au plus `K-1`, `K-2` et `K-3`
// points strictement interieurs, plus le nombre de records portant une
// EXTRA-SHELL : un point hors du support exactement sur la sphere. Le compteur
// d'extra-shell ne prouve pas la non-unicite du support minimal ; il prouve
// seulement `E \ U != vide` pour le support choisi.
//
// Il est exhaustif en `O(n^4)` supports et borne a 400 points. Ce n'est pas une
// route, c'est une verite de reference.
//
// Codes : 0 OK ; 2 campagne refusee avant calcul ; 3 desaccord avec les valeurs
// attendues gravees.
#include <charconv>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "oracle/exact_geometry.hpp"
#include "prototype/cloud_families.hpp"

namespace {

using i64 = long long;

struct Counts {
  i64 q2 = 0, q3 = 0, q4 = 0;
  i64 shell2 = 0, shell3 = 0, shell4 = 0;
};

// Un support propre positif est bien centre : toutes ses coordonnees
// barycentriques sont strictement positives. `side_of` rend -1 dedans, 0 sur le
// bord, +1 dehors — la comparaison est faite sur les rationnels de l'oracle.
Counts exhaustive_census(const std::vector<mhgp::P3>& pts, int kmin) {
  Counts out;
  const int n = (int)pts.size();
  for (int size = 2; size <= 4; ++size) {
    const int limit = kmin - (size - 1);  // p <= K-1, K-2, K-3
    std::vector<int> index((std::size_t)size);
    for (int i = 0; i < size; ++i) index[(std::size_t)i] = i;
    for (;;) {
      const mhgp3v::geometry::RationalSphere sphere = mhgp3v::geometry::sphere_through(pts, index);
      if (sphere.ok && mhgp3v::geometry::well_centred(sphere)) {
        int strict = 0, shell = 0;
        for (int z = 0; z < n; ++z) {
          bool in_support = false;
          for (int t : index)
            if (t == z) { in_support = true; break; }
          if (in_support) continue;
          const int side = mhgp3v::geometry::side_of(sphere, pts[(std::size_t)z]);
          if (side < 0) {
            if (++strict > limit) break;
          } else if (side == 0) {
            ++shell;
          }
        }
        if (strict <= limit) {
          if (size == 2) { ++out.q2; if (shell > 0) ++out.shell2; }
          else if (size == 3) { ++out.q3; if (shell > 0) ++out.shell3; }
          else { ++out.q4; if (shell > 0) ++out.shell4; }
        }
      }
      int i = size - 1;
      while (i >= 0 && index[(std::size_t)i] == n - size + i) --i;
      if (i < 0) break;
      ++index[(std::size_t)i];
      for (int j = i + 1; j < size; ++j)
        index[(std::size_t)j] = index[(std::size_t)(j - 1)] + 1;
    }
  }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 60, coord = 0, kmin = 4;
  i64 seed = 1;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kUniform;
  i64 expect_q2 = -1, expect_q3 = -1, expect_q4 = -1;
  auto integer = [](const char* text, i64* value) {
    return std::from_chars(text, text + std::strlen(text), *value).ec == std::errc();
  };
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    auto suffix = [&](const char* key) -> const char* {
      const std::size_t len = std::strlen(key);
      return arg.compare(0, len, key) == 0 ? argv[i] + len : nullptr;
    };
    i64 v = 0;
    if (const char* s = suffix("--points=")) { if (!integer(s, &v)) return 2; n = (int)v; }
    else if (const char* s2 = suffix("--coord=")) { if (!integer(s2, &v)) return 2; coord = (int)v; }
    else if (const char* s3 = suffix("--seed=")) { if (!integer(s3, &v)) return 2; seed = v; }
    else if (const char* s4 = suffix("--kmin=")) { if (!integer(s4, &v)) return 2; kmin = (int)v; }
    else if (const char* s5 = suffix("--expect-q2=")) { if (!integer(s5, &v)) return 2; expect_q2 = v; }
    else if (const char* s6 = suffix("--expect-q3=")) { if (!integer(s6, &v)) return 2; expect_q3 = v; }
    else if (const char* s7 = suffix("--expect-q4=")) { if (!integer(s7, &v)) return 2; expect_q4 = v; }
    else if (const char* sf = suffix("--family=")) {
      const std::string f = sf;
      if (f == "uniform") family = mhgp3v::CloudFamily::kUniform;
      else if (f == "terrain") family = mhgp3v::CloudFamily::kTerrain;
      else if (f == "scanline_single_pass") family = mhgp3v::CloudFamily::kScanlineSinglePass;
      else if (f == "scanline_overlap_multiecho")
        family = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
      else return 2;
    } else {
      std::printf("REFUS : argument inconnu %s\n", arg.c_str());
      return 2;
    }
  }
  if (n < 5 || n > 400 || kmin < 2 || kmin > 12) {
    std::printf("REFUS : le juge exhaustif est borne a 400 points et K dans [2,12]\n");
    return 2;
  }
  if (coord <= 0) coord = mhgp3v::cloud_family_default_coord(family, n);
  const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(family, n, coord, seed);
  if ((int)pts.size() != n) {
    std::printf("REFUS : le generateur a publie %d points au lieu de %d\n", (int)pts.size(), n);
    return 2;
  }
  const Counts c = exhaustive_census(pts, kmin);
  std::printf("juge independant (rationnels, Gauss, centre explicite) :"
              " famille=%s n=%d coord=%d graine=%lld K=%d\n",
              mhgp3v::cloud_family_name(family), n, coord, seed, kmin);
  std::printf("  supports propres positifs retenus : q2=%lld q3=%lld q4=%lld\n", c.q2, c.q3, c.q4);
  std::printf("  records portant une extra-shell : q2=%lld q3=%lld q4=%lld\n", c.shell2, c.shell3,
              c.shell4);
  int bad = 0;
  if (expect_q2 >= 0 && c.q2 != expect_q2) {
    std::printf("ECHEC : q2 attendu %lld, obtenu %lld\n", expect_q2, c.q2);
    bad = 1;
  }
  if (expect_q3 >= 0 && c.q3 != expect_q3) {
    std::printf("ECHEC : q3 attendu %lld, obtenu %lld\n", expect_q3, c.q3);
    bad = 1;
  }
  if (expect_q4 >= 0 && c.q4 != expect_q4) {
    std::printf("ECHEC : q4 attendu %lld, obtenu %lld\n", expect_q4, c.q4);
    bad = 1;
  }
  if (bad) return 3;
  std::printf("OK : census exhaustif independant\n");
  return 0;
}
