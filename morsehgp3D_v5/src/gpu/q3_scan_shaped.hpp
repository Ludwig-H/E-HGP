// MorseHGP3D v5 — scan de profondeur q3 « EN FORME DE KERNEL » (docs/GPU.md § 3,
// livraison 2) : la logique exacte de la lane q3 de generate.hpp (filtre
// flottant certifie + repli exact) ecrite comme un kernel — tableaux plats,
// indices explicites, aucune allocation, arithmetique 128 bits PORTABLE
// (dint.hpp, pas de __int128) — et compilable sur host pour etre PROUVEE egale
// a la lane de production (tests/q3_scan_shaped_gate.cpp) avant toute session
// G4. La version device (warp-par-seed, reduction par ballot) sera la
// transcription directe de `q3_scan_seed_shaped`.
//
// Contrat par seed x d'une ancre (a,b) :
//   sites affines de l'ancre (partages par tous les seeds) : u = 2z−a−b (trois
//   composantes i64 < 2^17... jusqu'a 2^18), q = |u|² − D² (i64 < 2^36), copies
//   double EXACTES ; seed : G = DE − F² (DI128 < 2^68), N = W − G·d (DI128,
//   |N_i| < 2^87), copies double, borne E = 2^-48 (G·qmax + 2|N|_1·umax) ;
//   L^ = fma(G, q, −2·(N·u)) ; L^ < −E ⟹ interieur certifie ; L^ > E ⟹
//   exterieur certifie ; sinon repli exact L = G·q − 2·u·N en DI128 (|L| < 2^105,
//   tous les produits sous le contrat de dint.hpp) et interieur ⟺ L < 0.
//   Le seed est MORT des que h_3 interieurs sont comptes (sortie anticipee —
//   sur device, la reduction par ballot rend le meme compte, l'arret anticipe
//   est un detail de cout, pas d'objet).
#pragma once

#include <cmath>

#include "../core/device.hpp"
#include "../core/dint.hpp"
#include "../core/types.hpp"

namespace mhgp5 {

// Sites affines de l'ancre, en structure de tableaux (SoA : le layout device).
// Les copies double ne sont PAS stockees : |u| < 2^18 et |q| < 2^36 sont
// exactement representables, la conversion (double) est deterministe et
// identique sur host et device — elle se fait a la volee (moitie moins de
// memoire et de transferts par site).
struct AnchorSitesSoA {
  const i64* u0;
  const i64* u1;
  const i64* u2;
  const i64* q;
  u32 n;
};

// Un seed : forme affine et copies flottantes (calculees une fois par seed).
struct SeedAffineD {
  DI128 G, N0, N1, N2;
  double Gd, Nd0, Nd1, Nd2, bound;
};

MHGP5_HD inline double q3_l_hat_shaped(const SeedAffineD& s, double u0, double u1, double u2, double q) {
  const double t = std::fma(s.Nd2, u2, std::fma(s.Nd1, u1, s.Nd0 * u0));
  return std::fma(s.Gd, q, -(t + t));
}

// L = G·q − 2·(u0·N0 + u1·N1 + u2·N2) en arithmetique portable.
MHGP5_HD inline DI128 q3_l_exact_shaped(const SeedAffineD& s, i64 u0, i64 u1, i64 u2, i64 q) {
  DI128 acc = di_mul_di128_i64(s.G, q);
  DI128 dot = di_mul_di128_i64(s.N0, u0);
  dot = di_add(dot, di_mul_di128_i64(s.N1, u1));
  dot = di_add(dot, di_mul_di128_i64(s.N2, u2));
  return di_sub(acc, di_shl1(dot));
}

// Compte des interieurs stricts du seed sur les sites de l'ancre, ecrete a
// `h3` (sortie anticipee), avec les compteurs de certification. Rend true si
// le seed est MORT (>= h3 interieurs). `skip` : index du site du carrier x
// (jamais compte comme interieur — il est sur la sphere ; ses u/q sont
// pourtant dans le tableau) ; a et b n'y sont pas (H = 0 : L > 0 par
// construction ? non — a et b ont u = ±d et q = 0, donc L = −2·u·N = 0 pour
// les deux : la lane de production les inclut dans le scan et ils rendent
// exactement L = 0, jamais < 0 ; on garde le meme comportement).
MHGP5_HD inline bool q3_scan_seed_shaped(const SeedAffineD& s, const AnchorSitesSoA& sites, u32 h3, u32 skip,
                                         u32* cert_neg, u32* cert_pos, u32* fallback, bool nonstrict = false) {
  u32 depth = 0;
  for (u32 i = 0; i < sites.n; ++i) {
    if (i == skip) continue;
    const double lh = q3_l_hat_shaped(s, (double)sites.u0[i], (double)sites.u1[i], (double)sites.u2[i], (double)sites.q[i]);
    bool interior;
    if (lh < -s.bound) {
      ++*cert_neg;
      interior = true;
    } else if (lh > s.bound) {
      ++*cert_pos;
      interior = false;
    } else {
      ++*fallback;
      const DI128 L = q3_l_exact_shaped(s, sites.u0[i], sites.u1[i], sites.u2[i], sites.q[i]);
      const int sg = di_sign(L);
      interior = sg < 0 || (nonstrict && sg == 0);
    }
    if (interior && ++depth >= h3) return true;
  }
  return false;
}

}  // namespace mhgp5
