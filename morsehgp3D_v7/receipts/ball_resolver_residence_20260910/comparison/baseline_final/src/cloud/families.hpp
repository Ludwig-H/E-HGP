// MorseHGP3D v6 — familles de nuages des campagnes d'echelle.
//
// PORT BIT A BIT des generateurs v5 (`morsehgp3D_v5/src/cloud/families.hpp`,
// lui-meme port v3/v4) pour les familles DILATEES : le meme quadruplet
// (famille, n, coord, graine) produit le meme nuage, point par point et dans
// le meme ordre, y compris les parametres contrefactuels `canopy_lift_cap` /
// `bump_amp_cap` (0 = inerte, chemin de production inchange). C'est voulu :
// les mesures v6 restent confrontables aux recus v3/v4/v5 sans re-deriver une
// verite, et la porte de conformite par digests canoniques en depend. Toute
// divergence de generation serait un changement d'objet — la fixture
// `mhgp7_families_fixture` grave les digests des nuages.
//
// Familles DILATEES (docs/REGIMES.md § 1) : `uniform` (regime volumique dur),
// `terrain` (LiDAR aerien 2,5D, coplanarites massives), `eight_clusters`
// (adversariale pour les certificats de temoins : milieux inter-amas dans le
// vide), `scanline_*` (LiDAR anisotrope). Leurs hauteurs croissent en sqrt(n)
// a espacement sol constant : role v6 = conformite differentielle et stress
// non extrapolable, JAMAIS une pente de lane.
//
// Familles STATIONNAIRES (docs/REGIMES.md § 2, NEUVES, aucun digest v4/v5) :
// `terrain_stationnaire`, `scanline_stationnaire` — hauteurs et tailles de
// motifs fixes en unites absolues (loi dilatee evaluee a n0 = 8000), nombre
// de motifs proportionnel a l'aire, seule la fenetre croit. C'est le regime
// qui modelise un capteur reel : toute conclusion de pente s'enonce ici.
//
// CONTRE-FAMILLES gravees (`two_lines`, `collinear_seven`) : refutations
// deterministes, jamais des regimes de mesure.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>
#include <set>
#include <vector>

#include "../core/intmath.hpp"
#include "../core/mutants.hpp"
#include "../core/types.hpp"

namespace mhgp7 {

enum class CloudFamily {
  kUniform,
  kTerrain,
  kScanlineSinglePass,
  kScanlineOverlapMultiecho,
  kEightClusters,
  kTwoLines,        // deux droites gauches : zero porteur aigu, masse universelle quadratique
  kCollinearSeven,  // sept temoins colineaires de profondeur exactement sept
  kTerrainStationnaire,   // terrain a motifs figes (constantes n0 = 8000), regime de pente
  kScanlineStationnaire,  // scanline a motifs figes (constantes n0 = 8000), regime de pente
};

inline constexpr CloudFamily kAllFamilies[] = {
    CloudFamily::kUniform,       CloudFamily::kTerrain,  CloudFamily::kScanlineSinglePass,
    CloudFamily::kScanlineOverlapMultiecho, CloudFamily::kEightClusters,
    CloudFamily::kTwoLines,      CloudFamily::kCollinearSeven,
    CloudFamily::kTerrainStationnaire, CloudFamily::kScanlineStationnaire};

// Familles dilatees de mesure (conformite differentielle v4/v5, stress).
inline constexpr CloudFamily kMeasureFamilies[] = {
    CloudFamily::kUniform, CloudFamily::kTerrain, CloudFamily::kEightClusters,
    CloudFamily::kScanlineSinglePass, CloudFamily::kScanlineOverlapMultiecho};

// Familles stationnaires : regimes de cout de premiere classe — toute
// conclusion de pente s'enonce ici (docs/REGIMES.md § 1 et § 4).
inline constexpr CloudFamily kStationaryFamilies[] = {
    CloudFamily::kTerrainStationnaire, CloudFamily::kScanlineStationnaire};

inline const char* cloud_family_name(CloudFamily family) {
  switch (family) {
    case CloudFamily::kUniform: return "uniform";
    case CloudFamily::kTerrain: return "terrain";
    case CloudFamily::kScanlineSinglePass: return "scanline_single_pass";
    case CloudFamily::kScanlineOverlapMultiecho: return "scanline_overlap_multiecho";
    case CloudFamily::kEightClusters: return "eight_clusters";
    case CloudFamily::kTwoLines: return "two_lines";
    case CloudFamily::kCollinearSeven: return "collinear_seven";
    case CloudFamily::kTerrainStationnaire: return "terrain_stationnaire";
    case CloudFamily::kScanlineStationnaire: return "scanline_stationnaire";
  }
  return "?";
}

inline bool parse_cloud_family(const char* name, CloudFamily* out) {
  for (const CloudFamily f : kAllFamilies)
    if (std::strcmp(name, cloud_family_name(f)) == 0) {
      *out = f;
      return true;
    }
  return false;
}

// Emprise par defaut : volumique 1e-3 pour `uniform`, areale 1/25 pour
// `terrain`, 1/40 pour les scanlines ; `eight_clusters` : densite INTERNE de
// `uniform`, dilatee par le pas d'amas. Identique v3/v4/v5 pour les familles
// portees (troncature). Les familles STATIONNAIRES gardent la meme densite
// areale mais ARRONDISSENT AU PLUS PROCHE (docs/REGIMES.md § 2 :
// coord = clamp(round(sqrt(25 n | 40 n)), 4, 65536)) — a n = 8000 cela donne
// 447 pour terrain_stationnaire et 566 pour scanline_stationnaire.

// Racine carree entiere arrondie au plus proche (milieu vers le haut), puis
// clamp au domaine [4, 65536] des emprises. Exacte et independante de libm.
inline long long detail_round_isqrt_clamped(long long m) {
  long long r = (long long)isqrt64_pure((i64)m);  // bit a bit, AUCUN libm (core/intmath.hpp)
  if (m > r * (r + 1)) ++r;                       // arrondi au plus proche, milieu vers le haut
  return std::max(4ll, std::min(65536ll, r));
}

inline int cloud_family_default_coord(CloudFamily family, int n) {
  double c = 0.0;
  switch (family) {
    case CloudFamily::kUniform: c = std::cbrt((double)n * 1000.0); break;
    case CloudFamily::kEightClusters: c = 4.0 * std::cbrt((double)n * 1000.0 / 8.0); break;
    case CloudFamily::kTerrain: c = std::sqrt((double)n * 25.0); break;
    case CloudFamily::kScanlineSinglePass:
    case CloudFamily::kScanlineOverlapMultiecho: c = std::sqrt((double)n * 40.0); break;
    case CloudFamily::kTwoLines: c = 65536.0; break;  // la hauteur separe les deux droites
    case CloudFamily::kCollinearSeven: c = 16.0; break;
    // Stationnaires : racine ENTIERE a regle d'arrondi explicite (au plus
    // proche, milieu vers le haut : r+1 ssi m > r(r+1)) — la frontiere du
    // domaine ne depend jamais de libm (portabilite des digests, audit v6
    // du 31 aout). A n = 8000 : 447 (terrain), 566 (scanline).
    case CloudFamily::kTerrainStationnaire: return (int)detail_round_isqrt_clamped((long long)n * 25);
    case CloudFamily::kScanlineStationnaire: return (int)detail_round_isqrt_clamped((long long)n * 40);
  }
  return (int)std::max(4.0, std::min(65536.0, c));
}

namespace detail {
// Cle de deduplication historique v3 : 17 bits par axe.
inline long long dedup_key(i64 x, i64 y, i64 z) {
  return ((long long)x << 34) | ((long long)y << 17) | (long long)z;
}
}  // namespace detail

inline std::vector<P3> uniform_cloud(int n, int coord, long long seed) {
  std::mt19937 rng((unsigned)seed);
  std::uniform_int_distribution<int> pick(0, coord - 1);
  std::vector<P3> pts;
  std::set<long long> keys;
  for (long long guard = 0; (int)pts.size() < n && guard < 200LL * n; ++guard) {
    P3 q{};
    q.x = (i64)pick(rng);
    q.y = (i64)pick(rng);
    q.z = (i64)pick(rng);
    if (!keys.insert(detail::dedup_key(q.x, q.y, q.z)).second) continue;
    pts.push_back(q);
  }
  return pts;
}

// Huit amas equilibres aux sommets d'un cube, densite interne de `uniform`.
inline std::vector<P3> eight_clusters_cloud(int n, int coord, long long seed) {
  std::mt19937 rng((unsigned)seed);
  const int half = std::max(1, coord / 8);
  const int lo = coord / 4;
  const int hi = coord - coord / 4;
  const int centres[8][3] = {{lo, lo, lo}, {hi, lo, lo}, {lo, hi, lo}, {hi, hi, lo},
                             {lo, lo, hi}, {hi, lo, hi}, {lo, hi, hi}, {hi, hi, hi}};
  std::uniform_int_distribution<int> jitter(-half, half);
  std::vector<P3> pts;
  std::set<long long> keys;
  int placed[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  const int quota = (int)(((long long)n + 7) / 8);
  for (long long guard = 0; (int)pts.size() < n && guard < 400LL * n; ++guard) {
    int c = -1;  // tourniquet strict : amas equilibres a un point pres
    for (int t = 0; t < 8; ++t) {
      const int cand = (guard + t) % 8;
      if (placed[cand] < quota) { c = cand; break; }
    }
    if (c < 0) break;
    P3 q{};
    q.x = (i64)std::min(coord - 1, std::max(0, centres[c][0] + jitter(rng)));
    q.y = (i64)std::min(coord - 1, std::max(0, centres[c][1] + jitter(rng)));
    q.z = (i64)std::min(coord - 1, std::max(0, centres[c][2] + jitter(rng)));
    if (!keys.insert(detail::dedup_key(q.x, q.y, q.z)).second) continue;
    ++placed[c];
    pts.push_back(q);
  }
  return pts;
}

// Relief : six calottes quadratiques entieres, sol a jitter {0,1,2}, 2 % de
// points hauts. Construction entiere, aucune libm.
// `canopy_lift_cap` (0 = inerte, chemin de production INCHANGE bit a bit) :
// borne l'amplitude du saut de canopee, qui vaut sinon coord/8 et grandit donc
// AVEC coord = sqrt(25 n) — la canopee s'eleve quand le nuage grandit, a
// espacement de points constant. Ce parametre sert UNIQUEMENT aux
// contrefactuels de mesure : il fabrique une famille differente, jamais un
// regime de production, et aucun recu de conformite ne l'emploie.
// `bump_amp_cap` (0 = inerte) : SECONDE anisotropie, jumelle de la canopee.
// L'amplitude des six calottes est tiree dans [coord/16, coord/8] : elle croit
// donc en sqrt(n) alors que l'espacement au sol reste constant (5 unites), et
// le nuage devient de plus en plus VOLUMIQUE quand il grandit — le nombre
// d'altitudes distinctes passe de 137 a 284 entre n = 8000 et n = 32000
// (graine 3). Le plafond fige cette hauteur ; comme `canopy_lift_cap`, il
// fabrique une famille differente et ne sert qu'aux contrefactuels de mesure.
inline std::vector<P3> terrain_cloud(int n, int coord, long long seed, int canopy_lift_cap = 0, int bump_amp_cap = 0) {
  std::mt19937 rng((unsigned)seed);
  struct Bump { long long cx = 0, cy = 0, r = 1, amp = 0; };
  std::vector<Bump> bumps(6);
  std::uniform_int_distribution<int> place(0, coord - 1);
  std::uniform_int_distribution<int> radius(std::max(2, coord / 6), std::max(3, coord / 3));
  const int amp_lo = bump_amp_cap > 0 ? std::max(1, bump_amp_cap / 2) : std::max(1, coord / 16);
  const int amp_hi = bump_amp_cap > 0 ? std::max(2, bump_amp_cap) : std::max(2, coord / 8);
  std::uniform_int_distribution<int> height(amp_lo, amp_hi);
  for (Bump& b : bumps) {
    b.cx = place(rng);
    b.cy = place(rng);
    b.r = radius(rng);
    b.amp = height(rng);
  }
  std::uniform_int_distribution<int> ground(0, coord - 1);
  std::uniform_int_distribution<int> jitter(0, 2);
  std::uniform_int_distribution<int> canopy_roll(0, 49);
  const int lift_max = canopy_lift_cap > 0 ? std::max(1, canopy_lift_cap) : std::max(2, coord / 8);
  std::uniform_int_distribution<int> canopy_lift(1, lift_max);
  std::vector<P3> pts;
  std::set<long long> keys;
  for (long long guard = 0; (int)pts.size() < n && guard < 200LL * n; ++guard) {
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
    P3 q{};
    q.x = (i64)x;
    q.y = (i64)y;
    q.z = (i64)z;
    if (!keys.insert(detail::dedup_key(q.x, q.y, q.z)).second) continue;
    pts.push_back(q);
  }
  return pts;
}

// Champ de hauteur scanline : calottes lisses PLUS plateaux a bord franc.
struct ScanlineField {
  struct Bump { long long cx = 0, cy = 0, r = 1, amp = 0; };
  struct Slab { long long x0 = 0, x1 = 0, y0 = 0, y1 = 0, h = 0; };
  std::vector<Bump> bumps;
  std::vector<Slab> slabs;
  // `bump_amp_cap` (0 = inerte) : MEME anisotropie que `terrain` — les cinq
  // calottes ET les quatre plateaux a bord franc tirent leur hauteur dans
  // [coord/16, coord/8], donc en sqrt(n), alors que l'espacement reste
  // constant.
  static ScanlineField make(std::mt19937* rng, int coord, int bump_amp_cap = 0) {
    ScanlineField field;
    field.bumps.resize(5);
    std::uniform_int_distribution<int> place(0, coord - 1);
    std::uniform_int_distribution<int> radius(std::max(2, coord / 6), std::max(3, coord / 3));
    const int amp_lo = bump_amp_cap > 0 ? std::max(1, bump_amp_cap / 2) : std::max(1, coord / 16);
    const int amp_hi = bump_amp_cap > 0 ? std::max(2, bump_amp_cap) : std::max(2, coord / 8);
    std::uniform_int_distribution<int> height(amp_lo, amp_hi);
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
      if (x >= s.x0 && x <= s.x1 && y >= s.y0 && y <= s.y1) h += s.h;
    return h;
  }
};

// Une passe de balayage : lignes espacees de `pitch`, pas anisotrope, bandes
// de densite, trous markoviens, jitter capteur, multi-echos optionnels.
// CONTRAT DE CARDINALITE : chaque push est borne par n ; la garde relachee est
// le mutant `family-scanline-overshoot` que la porte de cardinalite tue.
inline void scanline_pass(std::mt19937* rng, const ScanlineField& field, int n, int coord,
                          long long dx, long long dy, int step_along, int pitch,
                          bool multi_echo, std::vector<P3>* pts, std::set<long long>* keys) {
  const bool overshoot = MHGP7_MUTANT("family-scanline-overshoot");
  std::uniform_int_distribution<int> jitter(0, 2);
  std::uniform_int_distribution<int> hole_enter(0, 39);
  std::uniform_int_distribution<int> hole_exit(0, 7);
  std::uniform_int_distribution<int> echo_roll(0, 7);
  std::uniform_int_distribution<int> echo_lift(2, std::max(3, coord / 10));
  bool hole = false;
  for (long long y = dy % (long long)pitch; y < coord && (int)pts->size() < n; y += pitch) {
    const bool sparse_band = (y / pitch) % 3 == 2;
    const int band_step = sparse_band ? step_along * 2 : step_along;
    for (long long x = dx % (long long)band_step; x < coord && (int)pts->size() < n; x += band_step) {
      if (!hole && hole_enter(*rng) == 0) hole = true;
      else if (hole && hole_exit(*rng) == 0) hole = false;
      if (hole) continue;
      const long long ground = field.height(x, y) + jitter(*rng);
      const auto push = [&](long long z) {
        if (!overshoot && (int)pts->size() >= n) return;
        if (z > 65535) z = 65535;
        if (z < 0) z = 0;
        P3 q{};
        q.x = (i64)x;
        q.y = (i64)y;
        q.z = (i64)z;
        if (keys->insert(detail::dedup_key(q.x, q.y, q.z)).second) pts->push_back(q);
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

inline std::vector<P3> scanline_cloud(int n, int coord, long long seed, bool overlap_multiecho, int bump_amp_cap = 0) {
  std::mt19937 rng((unsigned)seed);
  const ScanlineField field = ScanlineField::make(&rng, coord, bump_amp_cap);
  const int step_along = 2;
  const int pitch = 8;  // anisotropie 4:1 entre l'inter-ligne et le pas
  std::vector<P3> pts;
  std::set<long long> keys;
  scanline_pass(&rng, field, overlap_multiecho ? (int)(((long long)n * 3) / 5) : n, coord, 0, 1, step_along, pitch,
                overlap_multiecho, &pts, &keys);
  if (overlap_multiecho)
    scanline_pass(&rng, field, n, coord, step_along / 2 + 1, pitch / 3 + 1, step_along, pitch, true,
                  &pts, &keys);
  for (int extra = 0; (int)pts.size() < n && extra < 8; ++extra)
    scanline_pass(&rng, field, n, coord, 2 * extra + 1, 3 * extra + 2, step_along, pitch,
                  overlap_multiecho, &pts, &keys);
  return pts;
}

// Deux droites gauches : A_i = (i,0,0), B_j = (0,j,H), indices depuis 1.
inline std::vector<P3> two_lines_cloud(int n, int coord) {
  std::vector<P3> pts;
  const int m = n / 2;
  if (m <= 0) return pts;
  const int h = std::max(1, coord - 1);
  const int imax = std::min(m, h);
  pts.reserve((size_t)(2 * imax));
  for (int i = 1; i <= imax; ++i) pts.push_back(P3{(i64)i, 0, 0});
  for (int j = 1; j <= imax; ++j) pts.push_back(P3{0, (i64)j, (i64)h});
  return pts;
}

// Neuf points exactement, coordonnees gravees de l'audit v3 du 14 aout 2026.
inline std::vector<P3> collinear_seven_cloud() {
  std::vector<P3> pts;
  pts.reserve(9);
  pts.push_back(P3{0, 0, 0});
  pts.push_back(P3{10, 0, 0});
  for (int i = 1; i <= 7; ++i) pts.push_back(P3{(i64)i, 0, 0});
  return pts;
}

// ---------------------------------------------------------------------------
// Familles STATIONNAIRES (docs/REGIMES.md § 2). Principe : les constantes de
// motif sont la loi dilatee evaluee a n0 = 8000 — a n = n0 les bornes de
// distribution coincident avec la famille dilatee — et le NOMBRE de motifs
// croit proportionnellement a l'aire (arrondi au plus proche, arithmetique
// entiere). Hauteurs et supports horizontaux fixes en unites absolues : c'est
// le regime d'un capteur reel, et le seul ou une pente de lane s'enonce.
// Familles NEUVES : aucun digest v4/v5, la v6 grave les siens
// (`mhgp7_families_fixture`).
// ---------------------------------------------------------------------------

// Terrain stationnaire. c0 = round(sqrt(25*8000)) = round(447.21) = 447.
// Constantes gravees (loi dilatee evaluee a c0 = 447) :
//   rayon    [max(2, 447/6), max(3, 447/3)]  = [74, 149]
//   amplitude [max(1, 447/16), max(2, 447/8)] = [27, 55]
//   lift canopee [1, max(2, 447/8)]           = [1, 55]
// nb_bosses = max(1, round(6 n / 8000)) = max(1, (6 n + 4000) / 8000).
// Meme formule quadratique entiere (division plancher) que `terrain`, meme
// jitter {0,1,2}, meme canopee 1/50, meme clamp z <= 65535, meme cle de
// deduplication, meme garde 200 n tirages.
inline std::vector<P3> terrain_stationnaire_cloud(int n, int coord, long long seed) {
  std::mt19937 rng((unsigned)seed);
  struct Bump { long long cx = 0, cy = 0, r = 1, amp = 0; };
  const int nb = (int)std::max<long long>(1, (6LL * (long long)n + 4000) / 8000);
  std::vector<Bump> bumps((size_t)nb);
  std::uniform_int_distribution<int> place(0, coord - 1);
  std::uniform_int_distribution<int> radius(74, 149);  // = [max(2, c0/6), max(3, c0/3)], c0 = 447
  std::uniform_int_distribution<int> height(27, 55);   // = [max(1, c0/16), max(2, c0/8)]
  for (Bump& b : bumps) {
    b.cx = place(rng);
    b.cy = place(rng);
    b.r = radius(rng);
    b.amp = height(rng);
  }
  std::uniform_int_distribution<int> ground(0, coord - 1);
  std::uniform_int_distribution<int> jitter(0, 2);
  std::uniform_int_distribution<int> canopy_roll(0, 49);
  std::uniform_int_distribution<int> canopy_lift(1, 55);  // = [1, max(2, c0/8)]
  std::vector<P3> pts;
  std::set<long long> keys;
  for (long long guard = 0; (int)pts.size() < n && guard < 200LL * n; ++guard) {
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
    P3 q{};
    q.x = (i64)x;
    q.y = (i64)y;
    q.z = (i64)z;
    if (!keys.insert(detail::dedup_key(q.x, q.y, q.z)).second) continue;
    pts.push_back(q);
  }
  return pts;
}

// Champ scanline stationnaire : memes structures Bump/Slab que ScanlineField,
// mais nc calottes et ns plateaux (proportionnels a l'aire) aux bornes gravees
// de c0 = 566. Ordre de tirage : toutes les calottes (cx, cy, r, amp), puis
// tous les plateaux (x0, y0, cote_x, cote_y, h) — comme ScanlineField::make.
inline ScanlineField make_scanline_stationnaire_field(std::mt19937* rng, int coord, int n) {
  ScanlineField field;
  const int nc = (int)std::max<long long>(1, (5LL * (long long)n + 4000) / 8000);
  const int ns = (int)std::max<long long>(1, (4LL * (long long)n + 4000) / 8000);
  field.bumps.resize((size_t)nc);
  std::uniform_int_distribution<int> place(0, coord - 1);
  std::uniform_int_distribution<int> radius(94, 188);  // = [max(2, c0/6), max(3, c0/3)], c0 = 566
  std::uniform_int_distribution<int> height(35, 70);   // = [max(1, c0/16), max(2, c0/8)]
  for (ScanlineField::Bump& b : field.bumps) {
    b.cx = place(*rng);
    b.cy = place(*rng);
    b.r = radius(*rng);
    b.amp = height(*rng);
  }
  field.slabs.resize((size_t)ns);
  std::uniform_int_distribution<int> side(47, 113);  // = [max(2, c0/12), max(3, c0/5)]
  for (ScanlineField::Slab& s : field.slabs) {
    s.x0 = place(*rng);
    s.y0 = place(*rng);
    s.x1 = std::min<long long>(coord - 1, s.x0 + side(*rng));
    s.y1 = std::min<long long>(coord - 1, s.y0 + side(*rng));
    s.h = height(*rng);
  }
  return field;
}

// Passe de balayage stationnaire : transcription a l'identique de
// `scanline_pass` (pas anisotrope, bandes de densite, trous markoviens
// entree 1/40 / sortie 1/8, jitter {0,1,2}, meme contrat de cardinalite et
// meme mutant), a UNE difference pres, qui est la raison d'etre de la
// famille : le lift des multi-echos est grave a [2, 56] = [2, max(3, c0/10)]
// au lieu de suivre coord/10 — l'epaisseur des echos ne croit pas avec la
// fenetre. Multi-echos toujours actifs (probabilite 1/8, second echo lift/2
// avec probabilite 4/8).
inline void scanline_stationnaire_pass(std::mt19937* rng, const ScanlineField& field, int n, int coord,
                                       long long dx, long long dy, int step_along, int pitch,
                                       std::vector<P3>* pts, std::set<long long>* keys) {
  const bool overshoot = MHGP7_MUTANT("family-scanline-overshoot");
  std::uniform_int_distribution<int> jitter(0, 2);
  std::uniform_int_distribution<int> hole_enter(0, 39);
  std::uniform_int_distribution<int> hole_exit(0, 7);
  std::uniform_int_distribution<int> echo_roll(0, 7);
  std::uniform_int_distribution<int> echo_lift(2, 56);  // = [2, max(3, c0/10)], c0 = 566
  bool hole = false;
  for (long long y = dy % (long long)pitch; y < coord && (int)pts->size() < n; y += pitch) {
    const bool sparse_band = (y / pitch) % 3 == 2;
    const int band_step = sparse_band ? step_along * 2 : step_along;
    for (long long x = dx % (long long)band_step; x < coord && (int)pts->size() < n; x += band_step) {
      if (!hole && hole_enter(*rng) == 0) hole = true;
      else if (hole && hole_exit(*rng) == 0) hole = false;
      if (hole) continue;
      const long long ground = field.height(x, y) + jitter(*rng);
      const auto push = [&](long long z) {
        if (!overshoot && (int)pts->size() >= n) return;
        if (z > 65535) z = 65535;
        if (z < 0) z = 0;
        P3 q{};
        q.x = (i64)x;
        q.y = (i64)y;
        q.z = (i64)z;
        if (keys->insert(detail::dedup_key(q.x, q.y, q.z)).second) pts->push_back(q);
      };
      push(ground);
      if (echo_roll(*rng) == 0) {
        const long long lift = echo_lift(*rng);
        push(ground + lift);
        if (echo_roll(*rng) < 4) push(ground + lift / 2);
      }
    }
  }
}

// Scanline stationnaire. c0 = round(sqrt(40*8000)) = round(565.685) = 566
// (docs/REGIMES.md § 2 imprime 565 : c'est une TRONCATURE, pas l'arrondi au
// plus proche que la meme ligne prescrit — 566 fait foi ici, et les bornes
// [94, 188] / [35, 70] / [47, 113] / [2, 56] du meme paragraphe sont bien
// celles de c0 = 566). Capteur inchange (anisotropie 4:1 gravee) : pas 2 le
// long des lignes, pitch 8 entre lignes, une ligne sur trois clairsemee,
// passe principale (dx = 0, dy = 1) puis passes de complement bornees comme
// en v5 (offsets 2*extra+1 / 3*extra+2, au plus 8).
inline std::vector<P3> scanline_stationnaire_cloud(int n, int coord, long long seed) {
  std::mt19937 rng((unsigned)seed);
  const ScanlineField field = make_scanline_stationnaire_field(&rng, coord, n);
  const int step_along = 2;
  const int pitch = 8;  // anisotropie 4:1 entre l'inter-ligne et le pas
  std::vector<P3> pts;
  std::set<long long> keys;
  scanline_stationnaire_pass(&rng, field, n, coord, 0, 1, step_along, pitch, &pts, &keys);
  for (int extra = 0; (int)pts.size() < n && extra < 8; ++extra)
    scanline_stationnaire_pass(&rng, field, n, coord, 2 * extra + 1, 3 * extra + 2, step_along, pitch,
                               &pts, &keys);
  return pts;
}

// REFUS des contrefactuels sur les familles stationnaires : `canopy_lift_cap`
// et `bump_amp_cap` figent des hauteurs qui, ici, sont DEJA stationnaires par
// construction — un cap non nul serait silencieusement sans objet, et
// l'ignorer fabriquerait un recu mensonger. Un cap non nul sur une famille
// stationnaire rend donc un nuage VIDE (jamais un nuage plausible) ; la CLI
// refuse en amont avec un diagnostic (code 2).
inline std::vector<P3> make_family_cloud(CloudFamily family, int n, int coord, long long seed,
                                         int canopy_lift_cap = 0, int bump_amp_cap = 0) {
  switch (family) {
    case CloudFamily::kTwoLines: return two_lines_cloud(n, coord);
    case CloudFamily::kCollinearSeven: return collinear_seven_cloud();
    case CloudFamily::kUniform: return uniform_cloud(n, coord, seed);
    case CloudFamily::kEightClusters: return eight_clusters_cloud(n, coord, seed);
    case CloudFamily::kTerrain: return terrain_cloud(n, coord, seed, canopy_lift_cap, bump_amp_cap);
    case CloudFamily::kScanlineSinglePass: return scanline_cloud(n, coord, seed, false, bump_amp_cap);
    case CloudFamily::kScanlineOverlapMultiecho: return scanline_cloud(n, coord, seed, true, bump_amp_cap);
    case CloudFamily::kTerrainStationnaire:
      if (canopy_lift_cap != 0 || bump_amp_cap != 0) return {};
      return terrain_stationnaire_cloud(n, coord, seed);
    case CloudFamily::kScanlineStationnaire:
      if (canopy_lift_cap != 0 || bump_amp_cap != 0) return {};
      return scanline_stationnaire_cloud(n, coord, seed);
  }
  return {};
}

// Nuage d'une famille avec l'emprise par defaut (coord = 0) et des identites
// id = index d'entree (commodite de campagne ; la bibliotheque recoit des
// InputPoint explicites).
inline std::vector<InputPoint> make_family_input(CloudFamily family, int n, int coord, long long seed,
                                                int canopy_lift_cap = 0, int bump_amp_cap = 0) {
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const std::vector<P3> pts = make_family_cloud(family, n, coord, seed, canopy_lift_cap, bump_amp_cap);
  std::vector<InputPoint> in(pts.size());
  for (size_t i = 0; i < pts.size(); ++i) in[i] = InputPoint{(PointId)i, pts[i]};
  return in;
}

}  // namespace mhgp7

