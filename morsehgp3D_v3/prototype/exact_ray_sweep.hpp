// MorseHGP3D v3 — LE NOYAU PRODUIT COMMUN DE delta : profondeur fermee de
// demi-boule par TRI DE RAYONS + DEUX POINTEURS, O(m log m).
//
// Contrat durable (PROPOSITION §6.3) : « le noyau produit commun de delta
// recoit un MULTIENSEMBLE DE RAYONS NON NULS et la banque scalaire `always`.
// Les adaptateurs de projection peuvent employer r = d cross V ou une base
// entiere equivalente, mais aucun consommateur produit ne conserve une
// seconde copie du tri et du sweep. » Le juge, lui, reste une minimisation
// fermee quadratique n <= 32, avec une AUTRE collecte et une AUTRE base
// (oracle du falsificateur d'ancres) — un defaut du tri ou des frontieres ne
// peut pas etre rejoue par le meme code.
//
// LE LEMME (NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4, complement exact) : pour
// une paire distincte (a,b), P = points strictement interieurs a la boule
// diametrale, delta(a,b) = min sur les directions nu du plan mediateur de
// |{z de P : (z-m).nu >= 0}|. Toute sphere passant par a,b contient
// strictement au moins delta points. Identite du noyau :
//
//     min ferme = always + m - max ouvert,
//
// ou le max ouvert est atteint sur un arc semi-ouvert [theta_i, theta_i+pi)
// ancre sur un rayon porte : rayons CONFONDUS inclus, ANTIPODE exclu. Les
// rayons sont tries par angle exact (moitie du cercle puis produit croise),
// les groupes confondus fusionnes, et la fenetre contigue de chaque ancre
// avance de facon monotone sur le tableau circulaire double.
//
// BORNES u16 de l'adaptateur diametral (l'ancienne annonce « dot ~2^93 »
// etait fausse — audit etat courant, profondeur point 4) : |V| <= 2^17 par
// axe ; e1 = (-dy,dx,0) (ou (1,0,0) si d axial en z) a deux composantes
// < 2^16 donc |w1| < 2^35 ; e2 = d x e1 a des composantes < 2^34 donc
// |w2| < 3*2^51 < 2^53 ; le comparateur cross du noyau < 2*2^35*2^53 = 2^89
// et le dot < 2^70 + 2^106 < 2^107 — tous tiennent dans __int128 avec marge.
// Permutation-invariant : le tri canonise l'ordre.
//
// Les INJECTIONS sont les mutants des portes (le juge ne les herite jamais) :
// `always_dropped` perd la banque des projections nulles (adaptateur),
// `open_boundary` compte l'antipode dans l'arc, `confounded_dropped` ote les
// rayons confondus de l'ancre — les fixtures depth-rays, confounded-arc et
// diametral-contact les tuent a code 4.
#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {

struct RaySweepInjections {
  bool always_dropped = false;
  bool open_boundary = false;
  bool confounded_dropped = false;
};

struct RaySweepStats {
  long long groups = 0;
  long long steps = 0;
  // L'audit etat courant refuse les compteurs partiels : « depth_sweep_steps
  // ignore les comparaisons du tri, et depth_scratch_capacity ne compte pas
  // les allocations locales planar, dir et count. » Le noyau compte donc ses
  // comparaisons de tri (le vrai cout O(m log m)) et le high-water en OCTETS
  // de tous ses tampons (rayons du multiensemble inclus, cote adaptateur).
  long long sort_compares = 0;
  long long scratch_bytes = 0;
};

// LE NOYAU : delta ferme depuis un multiensemble de rayons 2D NON NULS et la
// banque `always`. Le vecteur `rays` est trie en place (le tri canonise
// l'ordre — le resultat est invariant par permutation de l'entree).
inline long long closed_depth_from_rays(std::vector<std::pair<long long, long long>>* rays,
                                        long long always,
                                        const RaySweepInjections& injections,
                                        RaySweepStats* stats = nullptr) {
  using i64 = long long;
  using i128 = __int128;
  std::vector<std::pair<i64, i64>>& planar = *rays;
  const i64 m = (i64)planar.size();
  if (m == 0) return always;
  // Tri circulaire exact : moitie 0 = angles [0,pi) (w2 > 0, ou w2 = 0 avec
  // w1 > 0), moitie 1 = [pi,2*pi) ; a moitie egale, u avant v ssi
  // cross(u,v) > 0.
  const auto half_of = [](const std::pair<i64, i64>& u) {
    return (u.second > 0 || (u.second == 0 && u.first > 0)) ? 0 : 1;
  };
  i64 sort_compares = 0;
  std::sort(planar.begin(), planar.end(),
            [&](const std::pair<i64, i64>& u, const std::pair<i64, i64>& v) {
              ++sort_compares;
              const int hu = half_of(u), hv = half_of(v);
              if (hu != hv) return hu < hv;
              const i128 cross = (i128)u.first * v.second - (i128)u.second * v.first;
              if (cross != 0) return cross > 0;
              // Rayons confondus : ordre arbitraire stable par la norme —
              // le groupe les fusionne de toute facon.
              if (u.first != v.first) return u.first < v.first;
              return u.second < v.second;
            });
  // Groupes de rayons confondus (meme moitie et cross nul => meme direction).
  std::vector<std::pair<i64, i64>> dir;    // un representant par direction
  std::vector<i64> count;                  // sa multiplicite
  for (const auto& u : planar) {
    if (!dir.empty() && half_of(dir.back()) == half_of(u)) {
      const i128 cross = (i128)dir.back().first * u.second -
                         (i128)dir.back().second * u.first;
      if (cross == 0) { ++count.back(); continue; }
    }
    dir.push_back(u);
    count.push_back(1);
  }
  const std::size_t groups = dir.size();
  if (stats != nullptr) {
    stats->groups += (i64)groups;
    stats->sort_compares += sort_compares;
    stats->scratch_bytes = std::max(
        stats->scratch_bytes,
        (i64)(planar.capacity() * sizeof(std::pair<i64, i64>) +
              dir.capacity() * sizeof(std::pair<i64, i64>) + count.capacity() * sizeof(i64)));
  }
  // Deux pointeurs sur le tableau circulaire double : la fenetre de l'ancre
  // i contient son propre groupe puis tous les groupes a cross > 0 (arc
  // ouvert (theta_i, theta_i+pi)) ; l'antipode (cross = 0, dot < 0) est la
  // premiere direction refusee et reste dehors.
  i64 max_open = 0;
  std::size_t j = 0;
  i64 window = 0;
  for (std::size_t i = 0; i < groups; ++i) {
    if (j < i + 1) {
      j = i;
      window = count[i];
      ++j;
    }
    while (j < i + groups) {
      const std::pair<i64, i64>& u = dir[i];
      const std::pair<i64, i64>& v = dir[j % groups];
      const i128 cross = (i128)u.first * v.second - (i128)u.second * v.first;
      if (cross <= 0) break;
      window += count[j % groups];
      ++j;
      if (stats != nullptr) ++stats->steps;
    }
    i64 in_arc = window;
    // MUTANT confounded-dropped : les rayons confondus de l'ancre sortent de
    // l'arc — max ouvert sous-estime, delta surestime, faux prune possible.
    if (injections.confounded_dropped) in_arc -= count[i] - 1;
    // MUTANT open-boundary : l'antipode compterait dans l'arc.
    if (injections.open_boundary && j < i + groups) {
      const std::pair<i64, i64>& u = dir[i];
      const std::pair<i64, i64>& v = dir[j % groups];
      const i128 cross = (i128)u.first * v.second - (i128)u.second * v.first;
      const i128 dot = (i128)u.first * v.first + (i128)u.second * v.second;
      if (cross == 0 && dot < 0) in_arc += count[j % groups];
    }
    max_open = std::max(max_open, in_arc);
    window -= count[i];
  }
  return always + m - max_open;
}

// L'ADAPTATEUR DIAMETRAL : projette les temoins stricts fournis (PointId dans
// `pts`) sur le plan mediateur de (a,b) dans la base entiere e1/e2 ci-dessus,
// route les projections nulles vers `always`, puis appelle le noyau.
inline long long exact_closed_depth(const std::vector<mhgp::P3>& pts, const mhgp::P3& a,
                                    const mhgp::P3& b,
                                    const std::vector<int>& witness_ids,
                                    long long* always_out,
                                    const RaySweepInjections& injections,
                                    RaySweepStats* stats = nullptr) {
  using i64 = long long;
  const i64 dx = (i64)b.x - a.x, dy = (i64)b.y - a.y, dz = (i64)b.z - a.z;
  i64 e1x, e1y, e1z;
  if (dx != 0 || dy != 0) { e1x = -dy; e1y = dx; e1z = 0; }
  else { e1x = 1; e1y = 0; e1z = 0; }
  const i64 e2x = dy * e1z - dz * e1y;
  const i64 e2y = dz * e1x - dx * e1z;
  const i64 e2z = dx * e1y - dy * e1x;
  i64 always = 0;
  std::vector<std::pair<i64, i64>> rays;
  rays.reserve(witness_ids.size());
  for (int id : witness_ids) {
    const mhgp::P3& z = pts[(std::size_t)id];
    const i64 vx = 2 * (i64)z.x - a.x - b.x;
    const i64 vy = 2 * (i64)z.y - a.y - b.y;
    const i64 vz = 2 * (i64)z.z - a.z - b.z;
    const i64 cx = dy * vz - dz * vy;
    const i64 cy = dz * vx - dx * vz;
    const i64 cz = dx * vy - dy * vx;
    if (cx == 0 && cy == 0 && cz == 0) {
      if (!injections.always_dropped) ++always;   // MUTANT : banque perdue
      continue;
    }
    rays.push_back({vx * e1x + vy * e1y + vz * e1z, vx * e2x + vy * e2y + vz * e2z});
  }
  if (always_out != nullptr) *always_out = always;
  return closed_depth_from_rays(&rays, always, injections, stats);
}

}  // namespace mhgp3v
