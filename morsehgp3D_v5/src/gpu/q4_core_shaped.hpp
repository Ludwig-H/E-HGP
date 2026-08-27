// MorseHGP3D v5 — CŒUR DE SEED q4 « EN FORME DE KERNEL » (docs/GPU.md,
// livraison 4b) : la logique exacte du cœur de seed de Jung de la lane q4 de
// generate.hpp (filtre flottant certifie, intervalle de Jung, replis exacts)
// ecrite comme un kernel — tableaux plats, indices explicites, aucune
// allocation — compilable sur host pour etre PROUVEE egale a la lane de
// production (tests/q4_core_shaped_gate.cpp) avant tout portage device.
//
// Contrat par seed x d'une ancre (a,b), sites affines de l'ancre partages
// (AnchorSitesSoA, comme q3) : forme affine du seed (SeedAffineD : L^ et L
// exact), normale n = (b−a)×(x−a), J = D²(3G − 2 l_ax l_bx) >= 0 (un J < 0
// tue le seed AVANT le scan, en amont), bornes flottantes Jlo/Jhi. Pour
// chaque site z hors {a, b, x} :
//   L^ > E  ⟹ P > 0 certifie : jamais temoin (cert_pos) ;
//   L^ < −E ⟹ P < 0 certifie (cert_neg) : signe de l'intervalle de Jung sur
//            2P² <=> J·B² avec B = n·(z−a) = (n·u)/2 (exact : n ⟂ b−a) ;
//            +1 temoin (jung_kill), −1 non-temoin (jung_skip), 0 repli exact
//            (jung_fallback : P = L/4 en DI128, cmp_2p2_jb2) ;
//   sinon   ⟹ repli exact (float_fallback) : P >= 0 (strict : P > 0 sous le
//            mutant CPU seed-core-nonstrict) n'est jamais temoin ; sinon
//            cmp_2p2_jb2 decide.
// Le seed est MORT des que h_4 temoins sont comptes (sortie anticipee).
#pragma once

#include <cmath>

#include "../core/device.hpp"
#include "../core/dint.hpp"
#include "../core/types.hpp"
#include "../core/wide.hpp"
#include "../pipeline/float_filter.hpp"
#include "q3_scan_shaped.hpp"

namespace mhgp5 {

struct SeedQ4D {
  SeedAffineD aff;      // forme affine (G, N, bornes) — la meme que q3
  i64 n0, n1, n2;       // normale (b−a)×(x−a), |n| < 2^36
  DI128 J;              // J = D²(3G − 2 l_ax l_bx) >= 0
  double Jlo, Jhi;      // J·(1 ∓ kJungGuard)
  u32 skip_x;           // index du carrier x dans les sites
};

struct Q4CoreCounters {
  u32 cert_pos = 0, cert_neg = 0, jung_kill = 0, jung_skip = 0, jung_fallback = 0, float_fallback = 0;
};

// Pont DI128 -> __int128 utilisable des deux cotes (wide.hpp emploie deja
// u128 en MHGP5_HD : nvcc supporte __int128 en code device).
MHGP5_HD inline i128 di_to_i128_hd(DI128 a) { return (i128)(((u128)a.hi << 64) | a.lo); }

// Copie HD de float_filter.hpp::jung_interval_sign (sans le mutant hote).
MHGP5_HD inline int jung_interval_sign_shaped(double lh, double e, double jlo, double jhi, i64 b) {
  const double bd = (double)b;
  const double b2 = bd * bd;
  const double pu = (lh + e) * 0.25;
  const double pl = (lh - e) * 0.25;
  const double lhs_min = 2.0 * (pu * pu) * (1.0 - kJungGuard);
  const double rhs_max = jhi * (b2 * (1.0 + kJungGuard));
  if (lhs_min > rhs_max) return 1;
  const double lhs_max = 2.0 * (pl * pl) * (1.0 + kJungGuard);
  const double rhs_min = jlo * (b2 * (1.0 - kJungGuard));
  if (lhs_max < rhs_min) return -1;
  return 0;
}

// Rend true si le seed est MORT (>= h4 temoins). skip_a / skip_b : indices
// de a et b dans les sites (UINT32_MAX si absents).
MHGP5_HD inline bool q4_seed_core_shaped(const SeedQ4D& s, const AnchorSitesSoA& sites, u32 skip_a, u32 skip_b, u32 h4,
                                         bool nonstrict, Q4CoreCounters* c) {
  u32 fcount = 0;
  const i128 J = di_to_i128_hd(s.J);
  for (u32 i = 0; i < sites.n; ++i) {
    if (i == skip_a || i == skip_b || i == s.skip_x) continue;
    const double lh = q3_l_hat_shaped(s.aff, sites.u0d[i], sites.u1d[i], sites.u2d[i], sites.qd[i]);
    if (lh > s.aff.bound) {
      ++c->cert_pos;
      continue;
    }
    const i64 nu = s.n0 * sites.u0[i] + s.n1 * sites.u1[i] + s.n2 * sites.u2[i];  // = 2B, pair
    const i64 Bz = nu / 2;
    if (lh < -s.aff.bound) {
      ++c->cert_neg;
      const int js = jung_interval_sign_shaped(lh, s.aff.bound, s.Jlo, s.Jhi, Bz);
      if (js != 0) {
        if (js > 0) {
          ++c->jung_kill;
          if (++fcount >= h4) return true;
        } else {
          ++c->jung_skip;
        }
        continue;
      }
      ++c->jung_fallback;
      const DI128 P = di_div_by_4_exact(q3_l_exact_shaped(s.aff, sites.u0[i], sites.u1[i], sites.u2[i], sites.q[i]));
      const int cm = cmp_2p2_jb2(di_to_i128_hd(P), J, Bz);
      if ((nonstrict ? (cm >= 0) : (cm > 0)) && ++fcount >= h4) return true;
      continue;
    }
    ++c->float_fallback;
    const DI128 P = di_div_by_4_exact(q3_l_exact_shaped(s.aff, sites.u0[i], sites.u1[i], sites.u2[i], sites.q[i]));
    const int sg = di_sign(P);
    if (nonstrict ? (sg > 0) : (sg >= 0)) continue;
    const int cm = cmp_2p2_jb2(di_to_i128_hd(P), J, Bz);
    if ((nonstrict ? (cm >= 0) : (cm > 0)) && ++fcount >= h4) return true;
  }
  return false;
}

}  // namespace mhgp5
