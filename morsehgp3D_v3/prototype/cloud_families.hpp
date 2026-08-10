// MorseHGP3D v3 — LES FAMILLES DE NUAGES DES CAMPAGNES D'ECHELLE.
//
// Deux familles, une seule autorite de generation partagee par le pipeline et
// la qualification device (le meme (famille, n, coord, graine) produit le meme
// nuage bit a bit dans les deux binaires) :
//
//   - `uniform` : la famille historique des profils d'echelle — tirage entier
//     uniforme dans [0,coord)^3, densite volumique fixe 1e-3 par defaut. C'est
//     le REGIME LE PLUS DUR mesure jusqu'ici (I ~ n^1,6 sur 800..2400).
//   - `terrain` : le regime CIBLE type LiDAR aerien — densite AREALE fixe sur
//     [0,coord)^2, hauteur = somme entiere de calottes quadratiques (relief
//     lisse), sol plat a jitter fin {0,1,2} (bruit capteur, coplanarites
//     massives ASSUMEES : c'est le regime degenere que la voie multiplicitaire
//     traite), plus 2 % de points hauts (vegetation/sursols). Toute la
//     construction est ENTIERE : aucune libm, la reproduction inter-hotes est
//     exacte.
//
// La famille ne promet RIEN d'autre que sa construction : aucune densite
// LiDAR reelle n'est certifiee, c'est un regime de mesure declare.
#pragma once

#include <cmath>
#include <random>
#include <set>
#include <vector>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {

enum class CloudFamily { kUniform, kTerrain };

inline const char* cloud_family_name(CloudFamily family) {
  switch (family) {
    case CloudFamily::kUniform: return "uniform";
    case CloudFamily::kTerrain: return "terrain";
  }
  return "?";
}

// L'emprise par defaut de chaque famille : volumique 1e-3 pour uniform (le
// protocole historique), areale 1 point pour 25 cases pour terrain (pas moyen
// de grille ~5, l'espacement relatif d'un balayage aerien dense).
inline int cloud_family_default_coord(CloudFamily family, int n) {
  const double c = family == CloudFamily::kTerrain ? std::sqrt((double)n * 25.0)
                                                   : std::cbrt((double)n * 1000.0);
  return (int)std::max(4.0, std::min(65536.0, c));
}

// La famille historique, DEPLACEE VERBATIM du pipeline : meme graine, meme
// distribution, meme garde, meme clef de deduplication — les provenances des
// campagnes passees restent reproduites a l'identique.
inline std::vector<mhgp::P3> uniform_cloud(int n, int coord, long long seed) {
  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);
  std::vector<mhgp::P3> pts;
  std::set<long long> keys;
  for (int guard = 0; (int)pts.size() < n && guard < 200 * n; ++guard) {
    mhgp::P3 q{};
    q.x = (mhgp::i32)pick(rng);
    q.y = (mhgp::i32)pick(rng);
    q.z = (mhgp::i32)pick(rng);
    const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
    if (!keys.insert(key).second) continue;
    pts.push_back(q);
  }
  return pts;
}

inline std::vector<mhgp::P3> terrain_cloud(int n, int coord, long long seed) {
  std::mt19937 rng((unsigned)seed);
  // Le relief : six calottes quadratiques entieres h_b = amp*(r^2-d^2)/r^2,
  // centres et rayons tires une fois — le champ est lisse, sans table ni libm.
  struct Bump { long long cx = 0, cy = 0, r = 1, amp = 0; };
  std::vector<Bump> bumps(6);
  std::uniform_int_distribution<int> place(0, coord - 1);
  std::uniform_int_distribution<int> radius(std::max(2, coord / 6), std::max(3, coord / 3));
  std::uniform_int_distribution<int> height(std::max(1, coord / 16), std::max(2, coord / 8));
  for (Bump& b : bumps) {
    b.cx = place(rng);
    b.cy = place(rng);
    b.r = radius(rng);
    b.amp = height(rng);
  }
  std::uniform_int_distribution<int> ground(0, coord - 1);
  std::uniform_int_distribution<int> jitter(0, 2);
  std::uniform_int_distribution<int> canopy_roll(0, 49);       // 2 % de points hauts
  std::uniform_int_distribution<int> canopy_lift(1, std::max(2, coord / 8));
  std::vector<mhgp::P3> pts;
  std::set<long long> keys;
  for (int guard = 0; (int)pts.size() < n && guard < 200 * n; ++guard) {
    const long long x = ground(rng);
    const long long y = ground(rng);
    long long h = 0;
    for (const Bump& b : bumps) {
      const long long dx = x - b.cx, dy = y - b.cy;
      const long long d2 = dx * dx + dy * dy;
      if (d2 < b.r * b.r) h += b.amp * (b.r * b.r - d2) / (b.r * b.r);
    }
    long long z = h + jitter(rng);
    if (canopy_roll(rng) == 0) z += canopy_lift(rng);
    if (z > 65535) z = 65535;
    mhgp::P3 q{};
    q.x = (mhgp::i32)x;
    q.y = (mhgp::i32)y;
    q.z = (mhgp::i32)z;
    const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
    if (!keys.insert(key).second) continue;
    pts.push_back(q);
  }
  return pts;
}

inline std::vector<mhgp::P3> make_family_cloud(CloudFamily family, int n, int coord,
                                               long long seed) {
  return family == CloudFamily::kTerrain ? terrain_cloud(n, coord, seed)
                                         : uniform_cloud(n, coord, seed);
}

}  // namespace mhgp3v
