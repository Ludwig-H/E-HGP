// MorseHGP3D v6 — GRILLE DE CELLULES SANS APEX sur le disque des centres
// (docs/MATHEMATIQUES.md § 10, theoreme 10.5).
//
// Les centres des boules-candidates d'une ancre (a,b) vivent dans le plan
// bissecteur de ab, a distance <= rho de m (rho² = D²/rho2_den : 12 en q3,
// 8 en q4). Base entiere (u, v) du plan (sector_kill.hpp : losange (±u, ±v)
// contenant le disque) ; la cellule (i, j), -G <= i, j < G, est le
// parallelogramme de sommets (i'·u + j'·v)/G, i' ∈ {i, i+1}, j' ∈ {j, j+1}.
// Un site z (w = z − m) est strictement interieur a la boule de centre m + p
// ssi 8 w·p > 4|w|² − D² : condition AFFINE en p, donc vraie sur le
// parallelogramme ssi vraie aux quatre sommets — avec p = (i'u + j'v)/G et
// w' = 2w : 4 w'·(i'u + j'v) > G (|w'|² − D²), entier et exact. Une cellule
// est MORTE si >= h sites en sont temoins ; toute boule admissible dont le
// centre est dans une cellule morte (FERMEE : ses sommets et ses aretes
// comprises) a >= h interieurs stricts, donc est tuee par le filtre de
// profondeur : le seed (q3 : centre v3 = N/(2G3) ; q4 : corde v3 ± μ̂·n/(2G3),
// theoreme 10.4) est mort sans balayer un site. Les cellules necessaires sont
// celles qui rencontrent le losange ⊇ disque ; une cellule hors losange ne
// contient aucun centre admissible. Localisation des centres en coordonnees
// (α, β) de la base par la matrice de Gram, en binaire64 avec borne d'erreur
// explicite (derivation complete : MATHEMATIQUES.md, theoreme 10.5, « erreur
// du localisateur ») : les cellules de [αG − ε, αG + ε] (et de la boite des
// deux extremites pour la corde) doivent TOUTES etre mortes — sur-ensemble
// conservatif qui contient TOUTES les cellules fermees contenant le centre
// exact (un centre exactement sur une arete consulte ses deux cellules),
// jamais un faux positif.
// Pourquoi les secteurs (10.3) echouent la ou la grille tue : un secteur
// contient l'apex ET le bord du disque ; ses temoins doivent etre communs a
// la boule diametrale et aux boules les plus excentrees — sur une ancre
// au-dessus d'une vallee (vidage de bench/rect_probe.cpp, scanline 16 000 :
// 1643 sites dans la boule diametrale, aucun a moins de 0,30 D de m, 1066
// seeds tous morts avec 183 a 1680 temoins de cœur) ces temoins communs sont
// rares, mais chaque petite cellule en a des centaines.
// Mutants : `cell-kill-nonstrict` (>= au lieu de >), `cell-kill-h-minus-one`
// (cellule morte a h−1) — faux positifs, tues par fixture et conformites ;
// `cell-locate-eps-zero` (localisation sans marge d'erreur) — tue par
// l'oracle du localisateur rationnel (tests/cell_grid_oracle.cpp) et la
// fixture F11 (centre exactement sur un coin / une arete de cellule).
// Politique (sans effet sur l'objet) : grille seulement quand le cover est
// dense (kCellGridMinSites) ; `min_sites = 0` est le mode FORCE des tests
// (grille sur toute ancre, ratio et near_m ignores).
// Environnement : la localisation binaire64 exige arrondi au plus proche,
// evaluation en double (FLT_EVAL_METHOD = 0) et aucune reassociation
// (pas de -ffast-math) — la meme garde que le filtre flottant
// (float_filter_runtime_enabled) est passee a `build` par l'appelant ; hors
// de ces conditions la grille n'est pas construite (fail-open : aucun kill).
#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <vector>

#include "../core/mutants.hpp"
#include "../core/types.hpp"
#include "edge_cover.hpp"
#include "sector_kill.hpp"

namespace mhgp6 {

inline constexpr int kCellGridG = 8;
inline constexpr size_t kCellGridMinSites = 256;
// Ratio seeds/cover par lane : la grille coute ~58 ns par site du cover, un
// seed q3 ~0,8 µs (filtre de profondeur seul, mort en ~15 sites a 16 000), un
// seed q4 plusieurs µs (cœur, corde, completions) et jusqu'a ~10 µs a 200 k
// scanline : q3 ne la justifie que sur les ancres tres riches en seeds.
inline constexpr size_t kCellGridSeedsRatioQ3 = 2;   // seeds >= cover / 2
inline constexpr size_t kCellGridSeedsRatioQ4 = 8;   // seeds >= cover / 8
// Capacite des compteurs u32 par cellule (garde fail-open, jamais atteinte :
// un cover est borne par le nuage, < 2^31 sites sous le profil).
inline constexpr size_t kCellGridMaxSites = (size_t)1 << 31;
// Evaluation binaire64 stricte requise par la borne d'erreur du localisateur.
inline constexpr bool kCellLocateEvalOk = (FLT_EVAL_METHOD == 0);

// POLITIQUE (sans effet sur l'objet, identique sur tous les chemins — une
// seule definition, appelee par generate_detail::anchor_grid_stage) : la
// grille vaut son cout quand le cover est dense ET que les seeds aigus sont
// assez nombreux relativement au cover.
// `near_m` : sites a |2w|² < 0,36 D² (|w| < 0,30 D) — quand ils sont >= h, les
// seeds trouvent leurs temoins de cœur des les premieres classes radiales et
// la grille ne peut pas gagner ; quand ils manquent (ancre au-dessus d'une
// vallee), chaque seed balaie loin et la grille tue sans balayer.
// `min_sites = 0` : mode FORCE (tests : oracle ON/OFF, oracle de la grille) —
// la grille est construite sur TOUTE ancre, ratio et near_m ignores.
inline bool cell_grid_wanted(size_t cover_size, size_t acute_seeds, size_t near_m, u64 h, size_t min_sites, size_t ratio) {
  if (min_sites == 0) return true;
  return cover_size >= min_sites && acute_seeds * ratio >= cover_size && (u64)near_m < h;
}
inline bool cell_grid_near_m(i64 dist2q, i64 D2) { return (i128)100 * dist2q < (i128)36 * D2; }

// La grille est PARAMETRIQUE en G (etage E6 : G = 16 sur les ancres
// lourdes) : le certificat 10.5, la borne du localisateur (relative en G,
// terme absolu 2^-40 inchange) et la construction sont symboliques en G.
template <int GParam>
struct CellGridT {
  static constexpr int G = GParam;
  static constexpr int NV = 2 * G + 1;  // sommets par axe : i' = -G..G -> indice i' + G
  static constexpr int NC = 2 * G;      // cellules par axe : i = -G..G-1 -> indice i + G
  // Motif d'un echec de construction (fail-open : la grille ne tue rien).
  enum class Fail : u8 { kNone, kEnvironment, kCapacity, kBasis, kDegenerate };
  i64 u[3] = {0, 0, 0}, v[3] = {0, 0, 0};
  i128 uu_i = 0, vv_i = 0, uv_i = 0, det_i = 0;  // Gram EXACT (i128) — autorite ; les doubles en sont l'image
  double uu = 0, vv = 0, uv = 0, det = 0;        // convertis UNE fois (une conversion = un arrondi, compte dans la borne)
  u32 cnt[NC][NC] = {};                          // [j+G][i+G] : temoins de la cellule
  bool built = false, all_dead = false, nonstrict = false;
  Fail fail = Fail::kNone;
  u64 h = 0;
  u32 needed_cells = 0, dead_cells = 0;

  // Cellule (i, j) necessaire ssi sa boite [i, i+1]/G × [j, j+1]/G rencontre
  // le losange |α| + |β| <= 1 (⊇ disque) : min(|α|) + min(|β|) <= 1.
  static bool cell_needed(int i, int j) {
    const int mi = i >= 0 ? i : -(i + 1), mj = j >= 0 ? j : -(j + 1);
    return mi + mj <= G;
  }
  bool cell_dead(int i, int j) const {
    if (i < -G || i >= G || j < -G || j >= G) return true;  // hors grille : aucun centre admissible
    if (!cell_needed(i, j)) return true;                    // hors losange : idem
    return (u64)cnt[j + G][i + G] >= h;
  }

  // Comptage d'un site. ok(i', j') <=> a[i'] > b[j'] avec a[i'] = 4 i' du
  // MONOTONE en i' (croissante si du > 0, decroissante si du < 0, constante
  // si du = 0) et b[j'] = rhs − 4 j' dv monotone en j' : sur chaque ligne j',
  // les sommets temoins forment un intervalle [lo_j, G] (du > 0) ou
  // [−G, hi_j] (du < 0), dont la borne se deplace de facon monotone avec j'
  // (deux pointeurs, ~50 comparaisons par site au lieu de 289). Une cellule
  // (cj, ci) a ses quatre sommets temoins ssi ci >= max(lo_cj, lo_cj+1)
  // (resp. ci + 1 <= min(hi_cj, hi_cj+1)) : incrementation d'un INTERVALLE de
  // cellules par ligne, tableau de differences (prefixes/suffixes sommes a
  // la fin de build). Autorite independante : tests/cell_grid_oracle.cpp
  // (evaluation directe i128 des 289 sommets, cellule par cellule).
  template <typename Int, bool kNonstrict>
  static void count_site_t(Int du, Int dv, Int rhs, u32 (*dlo)[NC + 1], u32 (*dhi)[NC + 1]) {
    const auto okv = [&](int ii, Int bj) { const Int av = (Int)4 * (Int)(ii - G) * du; return kNonstrict ? (av >= bj) : (av > bj); };
    int bound[NV];  // du > 0 : lo_j (premier i' temoin, NV si aucun) ; du < 0 : hi_j (dernier, -1 si aucun) ; du = 0 : NV ou 0/-1
    if (du > 0) {
      int lo = 0;
      for (int jj = 0; jj < NV; ++jj) {
        const Int bj = rhs - (Int)4 * (Int)(jj - G) * dv;
        // a croissante en i' : lo = min i' avec ok ; b monotone en j' : lo se deplace dans un seul sens, mais on
        // corrige dans les deux directions (sur et amorti) pour rester exact sans hypothese de signe sur dv.
        while (lo > 0 && okv(lo - 1, bj)) --lo;
        while (lo < NV && !okv(lo, bj)) ++lo;
        bound[jj] = lo;
      }
      for (int cj = 0; cj < NC; ++cj) {
        const int L = std::max(bound[cj], bound[cj + 1]);
        ++dlo[cj][std::min(L, NC)];
      }
    } else if (du < 0) {
      int hi = NV - 1;
      for (int jj = 0; jj < NV; ++jj) {
        const Int bj = rhs - (Int)4 * (Int)(jj - G) * dv;
        // a decroissante en i' : hi = max i' avec ok.
        while (hi < NV - 1 && okv(hi + 1, bj)) ++hi;
        while (hi >= 0 && !okv(hi, bj)) --hi;
        bound[jj] = hi;
      }
      for (int cj = 0; cj < NC; ++cj) {
        const int H = std::min(bound[cj], bound[cj + 1]);  // cellules ci avec ci + 1 <= H, i.e. ci < H
        ++dhi[cj][std::max(H, 0)];
      }
    } else {
      for (int jj = 0; jj < NV; ++jj) {
        const Int bj = rhs - (Int)4 * (Int)(jj - G) * dv;
        bound[jj] = (kNonstrict ? ((Int)0 >= bj) : ((Int)0 > bj)) ? 0 : NV;  // ligne entiere ou vide
      }
      for (int cj = 0; cj < NC; ++cj) {
        const int L = std::max(bound[cj], bound[cj + 1]);
        ++dlo[cj][std::min(L, NC)];
      }
    }
  }
  template <typename Int>
  static void count_site(Int du, Int dv, Int rhs, bool nonstrict_, u32 (*dlo)[NC + 1], u32 (*dhi)[NC + 1]) {
    if (nonstrict_) count_site_t<Int, true>(du, dv, rhs, dlo, dhi);
    else count_site_t<Int, false>(du, dv, rhs, dlo, dhi);
  }
  // cnt[cj][ci] = #{sites : L <= ci} + #{sites : H > ci} (prefixes de dlo, suffixes de dhi).
  static void accumulate(const u32 (*dlo)[NC + 1], const u32 (*dhi)[NC + 1], u32 (*out)[NC]) {
    for (int cj = 0; cj < NC; ++cj) {
      u32 acc = 0;
      for (int ci = 0; ci < NC; ++ci) { acc += dlo[cj][ci]; out[cj][ci] = acc; }
      acc = 0;
      for (int ci = NC - 1; ci >= 0; --ci) { acc += dhi[cj][ci + 1]; out[cj][ci] += acc; }
    }
  }
  // Borne du chemin rapide i64 : |4·G·(|du| + |dv|)| et |G·rhs| < 2^62. Sous le
  // profil u16 (|w'| < 2^17, |u| < 2^24 : A, B <= 129) le chemin i128 est
  // INATTEIGNABLE (|mag| < 2^49) ; il reste une garde, exercee par l'oracle.
  static constexpr i128 kFastLimit = (i128)1 << 62;

  // Construit la grille sur le cover (sites ≠ a, b). `locate_env_ok` : garde
  // d'environnement du localisateur binaire64 (float_filter_runtime_enabled()),
  // passee par l'appelant. Rend false (fail-open, `fail` renseigne) si
  // l'environnement, la capacite, la base ou le determinant font defaut.
  // `all_dead` : toutes les cellules necessaires ont >= h temoins — l'ANCRE
  // est morte.
  bool build(const std::vector<CoverPoint>& cover, const std::vector<P3>& upos, i32 ua, i32 ub, const P3& pa, const P3& pb,
             i64 D2, i128 rho2_den, u64 h_in, bool locate_env_ok) {
    built = false;
    all_dead = false;
    fail = Fail::kNone;
    if (!locate_env_ok || !kCellLocateEvalOk) { fail = Fail::kEnvironment; return false; }
    if (cover.size() >= kCellGridMaxSites) { fail = Fail::kCapacity; return false; }
    if (!bisector_basis(pa, pb, D2, rho2_den, u, v)) { fail = Fail::kBasis; return false; }
    nonstrict = MHGP6_MUTANT("cell-kill-nonstrict");
    h = (MHGP6_MUTANT("cell-kill-h-minus-one") && h_in > 1) ? h_in - 1 : h_in;
    uu_i = vv_i = uv_i = 0;
    for (int k = 0; k < 3; ++k) {
      uu_i += (i128)u[k] * u[k];
      vv_i += (i128)v[k] * v[k];
      uv_i += (i128)u[k] * v[k];
    }
    det_i = uu_i * vv_i - uv_i * uv_i;
    uu = (double)uu_i;
    vv = (double)vv_i;
    uv = (double)uv_i;
    det = (double)det_i;
    if (!(det_i > 0) || !(det > 0)) { fail = Fail::kDegenerate; return false; }
    std::memset(cnt, 0, sizeof(cnt));
    u32 dlo[NC][NC + 1], dhi[NC][NC + 1];
    std::memset(dlo, 0, sizeof(dlo));
    std::memset(dhi, 0, sizeof(dhi));
    const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
    for (const CoverPoint& cz : cover) {
      if (cz.u == ua || cz.u == ub) continue;
      const P3& z = upos[(size_t)cz.u];
      const i64 w0 = 2 * z.x - sx, w1 = 2 * z.y - sy, w2 = 2 * z.z - sz;
      const i128 n2w = (i128)w0 * w0 + (i128)w1 * w1 + (i128)w2 * w2;
      const i128 rhs = (i128)G * (n2w - (i128)D2);
      const i128 du = (i128)w0 * u[0] + (i128)w1 * u[1] + (i128)w2 * u[2];
      const i128 dv = (i128)w0 * v[0] + (i128)w1 * v[1] + (i128)w2 * v[2];
      const i128 mag = (i128)4 * G * ((du < 0 ? -du : du) + (dv < 0 ? -dv : dv));
      if (mag < kFastLimit && (rhs < 0 ? -rhs : rhs) < kFastLimit) count_site<i64>((i64)du, (i64)dv, (i64)rhs, nonstrict, dlo, dhi);
      else count_site<i128>(du, dv, rhs, nonstrict, dlo, dhi);
    }
    accumulate(dlo, dhi, cnt);
    needed_cells = dead_cells = 0;
    for (int j = -G; j < G; ++j)
      for (int i = -G; i < G; ++i) {
        if (!cell_needed(i, j)) continue;
        ++needed_cells;
        if ((u64)cnt[j + G][i + G] >= h) ++dead_cells;
      }
    all_dead = dead_cells == needed_cells;
    built = true;
    return true;
  }

  // Coordonnees de cellule (α·G, β·G) d'un point p du plan donne par ses
  // produits scalaires entiers pu = den·(p·u), pv = den·(p·v), den > 0, avec
  // une borne d'erreur absolue eps commune (binaire64). Graphe fige : sept
  // conversions (une par entier), quatre produits, deux soustractions, un
  // produit et une division pour l'echelle, deux produits finaux ; erreur
  // vraie < 9,001 u · G·(|pu·vv| + |pv·uv|)/(den·det) (u = 2^-53), borne
  // calculee >= 2^-46 (1 − 12u) · G·(|t1|+|t2|+|s1|+|s2|)/(den·det) + 2^-40 (1 − u) :
  // marge > 14 ; le terme 2^-40 rend la borne STRICTEMENT positive, donc un
  // centre exactement sur une arete consulte ses deux cellules fermees
  // (derivation : MATHEMATIQUES.md, theoreme 10.5).
  void locate(i128 pu, i128 pv, i128 den, double* aG, double* bG, double* eps) const {
    const double pud = (double)pu, pvd = (double)pv, dend = (double)den;
    const double t1 = pud * vv, t2 = pvd * uv, s1 = pvd * uu, s2 = pud * uv;
    const double scale = (double)G / (dend * det);
    *aG = (t1 - t2) * scale;
    *bG = (s1 - s2) * scale;
    const double mag = std::fabs(t1) + std::fabs(t2) + std::fabs(s1) + std::fabs(s2);
    *eps = MHGP6_MUTANT("cell-locate-eps-zero") ? 0.0 : (mag * scale * 0x1p-46 + 0x1p-40);
  }
  // Domaine de la localisation : NaN ou |·| > 4G -> fail-open (aucune cellule consultee, aucun kill).
  static bool range_in_domain(double lo, double hi) {
    return (lo >= -4.0 * G) && (hi <= 4.0 * G) && (lo <= hi);
  }
  // Cellules consultees pour un intervalle [lo, hi] de coordonnee : floor(lo)..floor(hi).
  static void cell_range(double lo, double hi, int* c0, int* c1) {
    *c0 = (int)std::floor(lo);
    *c1 = (int)std::floor(hi);
  }
  // Boite de cellules consultee pour le point (pu, pv)/den : r = {i0, i1, j0, j1}.
  // Rend false hors domaine (fail-open). Contrat (oracle du localisateur
  // rationnel) : [i0, i1] contient TOUTES les cellules fermees contenant αG
  // exact (idem j).
  bool locate_box(i128 pu, i128 pv, i128 den, int r[4]) const {
    double aG, bG, eps;
    locate(pu, pv, den, &aG, &bG, &eps);
    const double lo_a = aG - eps, hi_a = aG + eps, lo_b = bG - eps, hi_b = bG + eps;
    if (!range_in_domain(lo_a, hi_a) || !range_in_domain(lo_b, hi_b)) return false;
    cell_range(lo_a, hi_a, &r[0], &r[1]);
    cell_range(lo_b, hi_b, &r[2], &r[3]);
    return true;
  }
  // Boite consultee pour le segment d'extremites (pu0, pv0)/den et (pu1, pv1)/den.
  bool segment_box(i128 pu0, i128 pv0, i128 pu1, i128 pv1, i128 den, int r[4]) const {
    double a0, b0, e0, a1, b1, e1;
    locate(pu0, pv0, den, &a0, &b0, &e0);
    locate(pu1, pv1, den, &a1, &b1, &e1);
    const double e = std::max(e0, e1);
    const double lo_a = std::min(a0, a1) - e, hi_a = std::max(a0, a1) + e;
    const double lo_b = std::min(b0, b1) - e, hi_b = std::max(b0, b1) + e;
    if (!range_in_domain(lo_a, hi_a) || !range_in_domain(lo_b, hi_b)) return false;
    cell_range(lo_a, hi_a, &r[0], &r[1]);
    cell_range(lo_b, hi_b, &r[2], &r[3]);
    return true;
  }
  bool range_dead(const int r[4]) const {
    for (int j = r[2]; j <= r[3]; ++j)
      for (int i = r[0]; i <= r[1]; ++i)
        if (!cell_dead(i, j)) return false;
    return true;
  }
  // Seed q3 : centre v3 = N/(2·G3) ; pu = N·u, pv = N·v (i128 exacts), den = 2·G3.
  bool point_dead(i128 pu, i128 pv, i128 den) const {
    int r[4];
    return locate_box(pu, pv, den, r) && range_dead(r);
  }
  // Seed q4 : corde d'extremites (N ± μ̂·n)/(2·G3) — boite des deux extremites.
  bool segment_dead(i128 pu0, i128 pv0, i128 pu1, i128 pv1, i128 den) const {
    int r[4];
    return segment_box(pu0, pv0, pu1, pv1, den, r) && range_dead(r);
  }
  // SONDE E6 (lecture seule, opt-in --sonde-e6) : MINIMUM du compte de
  // temoins sur les cellules NECESSAIRES de la boite du segment — la
  // contrainte qui a empeche le kill par cellules. -1 si la boite sort du
  // domaine ou ne rencontre aucune cellule necessaire. N'affecte jamais
  // l'objet : aucune decision n'en depend.
  long long segment_min_count(i128 pu0, i128 pv0, i128 pu1, i128 pv1, i128 den) const {
    int r[4];
    if (!segment_box(pu0, pv0, pu1, pv1, den, r)) return -1;
    long long mn = -1;
    for (int j = r[2]; j <= r[3]; ++j)
      for (int i = r[0]; i <= r[1]; ++i) {
        if (i < -G || i >= G || j < -G || j >= G || !cell_needed(i, j)) continue;
        const long long c = (long long)cnt[j + G][i + G];
        if (mn < 0 || c < mn) mn = c;
      }
    return mn;
  }
};

// L'alias historique : la grille de production a G = 8 (theoreme 10.5).
using CellGrid = CellGridT<kCellGridG>;
// Grille RAFFINEE de l'etage E6 (opt-in --e6-grille, ancres q4 lourdes).
inline constexpr int kCellGridGFine = 16;
using CellGridFine = CellGridT<kCellGridGFine>;

}  // namespace mhgp6
