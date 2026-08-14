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
// Les familles SCANLINE (reponse auditeur Q5, premiere famille proche LiDAR) :
//
//   - `scanline_single_pass` : une nappe balayee en lignes — espacement
//     ANISOTROPE (pas 2 le long de la ligne, 8 entre lignes), BANDES de
//     densite (une ligne sur trois au pas double), TROUS de segments
//     (dropouts markoviens), BORDS FRANCS (plateaux rectangulaires en plus
//     des calottes lisses), jitter capteur {0,1,2}.
//   - `scanline_overlap_multiecho` : la meme scene re-echantillonnee par DEUX
//     passages faiblement decales, plus des MULTI-ECHOS verticaux quantifies
//     (jusqu'a trois retours au meme (x,y)) — le regime multi-captation.
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

enum class CloudFamily {
  kUniform,
  kTerrain,
  kScanlineSinglePass,
  kScanlineOverlapMultiecho,
  kEightClusters,
  // ---- CONTRE-FAMILLE, PAS UN REGIME DU CONTRAT.
  //
  // `two_lines` vient de la section 3 de
  // audits/AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md. Elle
  // n'est pas une famille de mesure : c'est une REFUTATION gravee. Avec
  // A_i=(i,0,0) et B_j=(0,j,H), chaque paire croisee possede une sphere vide de
  // centre (i, j, (H^2+i^2-j^2)/(2H)), puisque les puissances exterieures des
  // autres sites valent exactement (k-i)^2 et (l-j)^2, strictement positives.
  //
  // Donc AUCUN certificat universel sound ne ferme ces m^2 = n^2/4 paires : ni
  // le masque central, ni SOC64, ni CORNER512, ni un LP, ni des cages. Et
  // pourtant la vraie source q3/q4 y est VIDE : pour i<k, l'angle en A_i
  // verifie (A_k-A_i).(B_j-A_i) = -(k-i) i < 0, donc aucun triangle n'est aigu.
  //
  // Elle est ici pour qu'une porte puisse mesurer les deux nombres cote a cote
  // sur le meme nuage : masse universelle quadratique, ZERO porteur aigu.
  kTwoLines
};

inline const char* cloud_family_name(CloudFamily family) {
  switch (family) {
    case CloudFamily::kUniform: return "uniform";
    case CloudFamily::kTerrain: return "terrain";
    case CloudFamily::kScanlineSinglePass: return "scanline_single_pass";
    case CloudFamily::kScanlineOverlapMultiecho: return "scanline_overlap_multiecho";
    case CloudFamily::kEightClusters: return "eight_clusters";
    case CloudFamily::kTwoLines: return "two_lines";
  }
  return "?";
}

// L'emprise par defaut de chaque famille : volumique 1e-3 pour uniform (le
// protocole historique), areale 1 point pour 25 cases pour terrain (pas moyen
// de grille ~5, l'espacement relatif d'un balayage aerien dense), areale
// 1 pour 40 pour les scanlines (l'anisotropie 2x8 et les trous laissent une
// marge de capacite d'environ 1,5x sur la grille de balayage).
inline int cloud_family_default_coord(CloudFamily family, int n) {
  double c = 0.0;
  switch (family) {
    case CloudFamily::kUniform: c = std::cbrt((double)n * 1000.0); break;
    // Huit amas equilibres : meme densite INTERNE que `uniform`, mais separes.
    // L'emprise est donc celle d'un nuage huit fois plus dense, dilatee par le
    // pas d'amas ; le regime 2 du plan de test le nomme melange equilibre.
    case CloudFamily::kEightClusters: c = 4.0 * std::cbrt((double)n * 1000.0 / 8.0); break;
    case CloudFamily::kTerrain: c = std::sqrt((double)n * 25.0); break;
    case CloudFamily::kScanlineSinglePass:
    case CloudFamily::kScanlineOverlapMultiecho: c = std::sqrt((double)n * 40.0); break;
    // La contre-famille EXIGE toute la grille : la hauteur `H` separe les deux
    // droites et la construction de la sphere vide en depend. Une emprise
    // reduite changerait l'objet refute.
    case CloudFamily::kTwoLines: c = 65536.0; break;
  }
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

// LA FAMILLE ADVERSARIALE. Huit amas equilibres aux sommets d'un cube, chacun
// de densite interne egale a celle de `uniform`, separes par un vide large.
// Elle attaque precisement les certificats de temoins : une paire inter-amas a
// un milieu situe dans le vide, donc aucune boule de milieu ne la tue, et seule
// la geometrie des amas eux-memes peut la fermer. Construction entiere,
// deduplication identique aux autres familles.
inline std::vector<mhgp::P3> eight_clusters_cloud(int n, int coord, long long seed) {
  std::mt19937 rng((unsigned)seed);
  // Demi-cote d'un amas : le quart de l'emprise, ce qui laisse un vide egal a
  // deux fois ce demi-cote entre deux amas voisins.
  const int half = std::max(1, coord / 8);
  const int lo = coord / 4;
  const int hi = coord - coord / 4;
  const int centres[8][3] = {{lo, lo, lo}, {hi, lo, lo}, {lo, hi, lo}, {hi, hi, lo},
                             {lo, lo, hi}, {hi, lo, hi}, {lo, hi, hi}, {hi, hi, hi}};
  std::uniform_int_distribution<int> jitter(-half, half);
  std::vector<mhgp::P3> pts;
  std::set<long long> keys;
  int placed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  const int quota = (n + 7) / 8;
  for (int guard = 0; (int)pts.size() < n && guard < 400 * n; ++guard) {
    // Tourniquet strict : les amas restent equilibres a un point pres, meme si
    // la deduplication rejette. Un amas sature cede son tour au suivant.
    int c = -1;
    for (int t = 0; t < 8; ++t) {
      const int cand = (guard + t) % 8;
      if (placed[cand] < quota) { c = cand; break; }
    }
    if (c < 0) break;
    mhgp::P3 q{};
    q.x = (mhgp::i32)std::min(coord - 1, std::max(0, centres[c][0] + jitter(rng)));
    q.y = (mhgp::i32)std::min(coord - 1, std::max(0, centres[c][1] + jitter(rng)));
    q.z = (mhgp::i32)std::min(coord - 1, std::max(0, centres[c][2] + jitter(rng)));
    const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
    if (!keys.insert(key).second) continue;
    ++placed[c];
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

// Le champ de hauteur des familles scanline : les calottes lisses du terrain
// PLUS des plateaux rectangulaires a BORD FRANC (batiments) — la discontinuite
// de hauteur est exigee par le regime declare. Construction entiere.
struct ScanlineField {
  struct Bump { long long cx = 0, cy = 0, r = 1, amp = 0; };
  struct Slab { long long x0 = 0, x1 = 0, y0 = 0, y1 = 0, h = 0; };
  std::vector<Bump> bumps;
  std::vector<Slab> slabs;
  static ScanlineField make(std::mt19937* rng, int coord) {
    ScanlineField field;
    field.bumps.resize(5);
    std::uniform_int_distribution<int> place(0, coord - 1);
    std::uniform_int_distribution<int> radius(std::max(2, coord / 6), std::max(3, coord / 3));
    std::uniform_int_distribution<int> height(std::max(1, coord / 16), std::max(2, coord / 8));
    for (Bump& b : field.bumps) {
      b.cx = place(*rng);
      b.cy = place(*rng);
      b.r = radius(*rng);
      b.amp = height(*rng);
    }
    field.slabs.resize(4);
    std::uniform_int_distribution<int> side(std::max(2, coord / 12), std::max(3, coord / 5));
    for (Slab& s : field.slabs) {
      s.x0 = place(*rng);
      s.y0 = place(*rng);
      s.x1 = std::min<long long>(coord - 1, s.x0 + side(*rng));
      s.y1 = std::min<long long>(coord - 1, s.y0 + side(*rng));
      s.h = height(*rng);
    }
    return field;
  }
  long long height(long long x, long long y) const {
    long long h = 0;
    for (const Bump& b : bumps) {
      const long long dx = x - b.cx, dy = y - b.cy;
      const long long d2 = dx * dx + dy * dy;
      if (d2 < b.r * b.r) h += b.amp * (b.r * b.r - d2) / (b.r * b.r);
    }
    for (const Slab& s : slabs)
      if (x >= s.x0 && x <= s.x1 && y >= s.y0 && y <= s.y1) h += s.h;   // bord FRANC
    return h;
  }
};

// Une passe de balayage : lignes a y fixe espacees de `pitch`, pas anisotrope
// `step_along` le long de la ligne, bandes de densite (une ligne sur trois au
// pas double), trous markoviens (entree 1/40, sortie 1/8 — environ un sixieme
// du lineaire perdu), jitter capteur {0,1,2}, offsets de passe (dx, dy) et
// multi-echos optionnels (jusqu'a trois retours verticaux au meme (x,y)).
// CONTRAT DE CARDINALITE (audit etat courant) : CHAQUE push est borne par n
// — l'ancienne garde `size < n`, testee seulement avant un pixel, laissait un
// ou deux echos de recouvrement depasser n (12 501 points pour 12 500 a
// coord 707) et desynchronisait ledger, fate et oracle du driver.
// `mutant_overshoot` retablit l'ancien comportement pour la porte qui le tue.
inline void scanline_pass(std::mt19937* rng, const ScanlineField& field, int n, int coord,
                          long long dx, long long dy, int step_along, int pitch,
                          bool multi_echo, std::vector<mhgp::P3>* pts,
                          std::set<long long>* keys, bool mutant_overshoot = false) {
  std::uniform_int_distribution<int> jitter(0, 2);
  std::uniform_int_distribution<int> hole_enter(0, 39);
  std::uniform_int_distribution<int> hole_exit(0, 7);
  std::uniform_int_distribution<int> echo_roll(0, 7);
  std::uniform_int_distribution<int> echo_lift(2, std::max(3, coord / 10));
  bool hole = false;
  for (long long y = dy % (long long)pitch; y < coord && (int)pts->size() < n; y += pitch) {
    const bool sparse_band = (y / pitch) % 3 == 2;
    const int band_step = sparse_band ? step_along * 2 : step_along;
    for (long long x = dx % (long long)band_step; x < coord && (int)pts->size() < n;
         x += band_step) {
      if (!hole && hole_enter(*rng) == 0) hole = true;
      else if (hole && hole_exit(*rng) == 0) hole = false;
      if (hole) continue;
      const long long ground = field.height(x, y) + jitter(*rng);
      const auto push = [&](long long z) {
        if (!mutant_overshoot && (int)pts->size() >= n) return;   // MUTANT : garde retiree
        if (z > 65535) z = 65535;
        if (z < 0) z = 0;
        mhgp::P3 q{};
        q.x = (mhgp::i32)x;
        q.y = (mhgp::i32)y;
        q.z = (mhgp::i32)z;
        const long long key = ((long long)q.x << 34) | ((long long)q.y << 17) | (long long)q.z;
        if (keys->insert(key).second) pts->push_back(q);
      };
      push(ground);
      if (multi_echo && echo_roll(*rng) == 0) {
        const long long lift = echo_lift(*rng);
        push(ground + lift);
        if (echo_roll(*rng) < 4) push(ground + lift / 2);
      }
    }
  }
}

inline std::vector<mhgp::P3> scanline_cloud(int n, int coord, long long seed,
                                            bool overlap_multiecho,
                                            bool mutant_overshoot = false) {
  std::mt19937 rng((unsigned)seed);
  const ScanlineField field = ScanlineField::make(&rng, coord);
  const int step_along = 2;
  const int pitch = 8;   // ANISOTROPIE 4:1 entre l'inter-ligne et le pas
  std::vector<mhgp::P3> pts;
  std::set<long long> keys;
  scanline_pass(&rng, field, overlap_multiecho ? (n * 3) / 5 : n, coord, 0, 1, step_along,
                pitch, overlap_multiecho, &pts, &keys, mutant_overshoot);
  if (overlap_multiecho)
    // DEUX PASSAGES faiblement decales : la meme scene, offsets non alignes.
    scanline_pass(&rng, field, n, coord, step_along / 2 + 1, pitch / 3 + 1, step_along,
                  pitch, true, &pts, &keys, mutant_overshoot);
  // Passes de complement si la capacite a manque (trous, deduplication) :
  // memes lois, offsets decales — jamais de troncature silencieuse ailleurs.
  for (int extra = 0; (int)pts.size() < n && extra < 8; ++extra)
    scanline_pass(&rng, field, n, coord, 2 * extra + 1, 3 * extra + 2, step_along, pitch,
                  overlap_multiecho, &pts, &keys, mutant_overshoot);
  return pts;
}

// ---------------------------------------------------------------------------
// LA CONTRE-FAMILLE A DEUX DROITES.
//
//   A_i = (i, 0, 0)        pour i = 1..m
//   B_j = (0, j, H)        pour j = 1..m,   H = coord - 1
//
// Elle est DETERMINISTE : aucune graine ne l'affecte, et c'est voulu. Une
// refutation qui dependrait d'un tirage ne serait pas une refutation.
//
// Le nombre de points rendu est `2m` avec `m = n/2` ; un `n` impair rend
// `n-1` points. L'appelant qui exige une cardinalite exacte doit passer un `n`
// pair, et le probe le refuse sinon par son propre controle de cardinalite.
inline std::vector<mhgp::P3> two_lines_cloud(int n, int coord) {
  std::vector<mhgp::P3> pts;
  const int m = n / 2;
  if (m <= 0) return pts;
  const int h = std::max(1, coord - 1);
  // Les indices commencent a 1 : `A_0 = (0,0,0)` et `B_0 = (0,0,H)` seraient
  // sur l'axe commun et casseraient l'argument d'angle obtus.
  const int imax = std::min(m, h);
  pts.reserve((size_t)(2 * imax));
  for (int i = 1; i <= imax; ++i) pts.push_back(mhgp::P3{(std::uint16_t)i, 0, 0});
  for (int j = 1; j <= imax; ++j)
    pts.push_back(mhgp::P3{0, (std::uint16_t)j, (std::uint16_t)h});
  return pts;
}

inline std::vector<mhgp::P3> make_family_cloud(CloudFamily family, int n, int coord,
                                               long long seed,
                                               bool mutant_overshoot = false) {
  switch (family) {
    case CloudFamily::kTwoLines: return two_lines_cloud(n, coord);
    case CloudFamily::kUniform: return uniform_cloud(n, coord, seed);
    case CloudFamily::kEightClusters: return eight_clusters_cloud(n, coord, seed);
    case CloudFamily::kTerrain: return terrain_cloud(n, coord, seed);
    case CloudFamily::kScanlineSinglePass:
      return scanline_cloud(n, coord, seed, false, mutant_overshoot);
    case CloudFamily::kScanlineOverlapMultiecho:
      return scanline_cloud(n, coord, seed, true, mutant_overshoot);
  }
  return {};
}

}  // namespace mhgp3v
