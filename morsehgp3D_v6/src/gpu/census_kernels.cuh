// MorseHGP3D v6 — KERNELS PREFILTRE (C3) et CENSUS (C4) de la serie C
// (docs/GPU.md § Wire serie C v1). Un fil = une boule ; DFS sur l'index
// resident (tableaux du wire), ORDRE DE PILE DU SCALAIRE (gauche puis
// droite empiles, droite puis gauche visites — census.hpp) : les listes
// interior/shell sortent BIT-IDENTIQUES au CPU. Arithmetique 100 % entiere
// (__int128 compose de paires u64 — le socle prouve par
// mhgp6_device_witness) ; AUCUNE division device : les six candidats u32
// arrivent du wire (hisses hote, docs/GPU.md § Decision division).
//
// Pile bornee kDfsStackCap = 64 (legitime <= 49, fixture peigne Morton) :
// un depassement est un STATUT par boule (kBallStackOverflow), jamais une
// troncature — l'hote le reduit au plus petit index global et refuse le RUN
// entier (§ 5.5 recus).
//
// Compile par nvcc (MHGP6_ENABLE_CUDA) OU par le stub hote
// (MHGP6_FAKE_DEVICE, tests/cuda_stub.hpp) : le stub prouve la LOGIQUE
// bit-identique contre le scalaire AVANT toute session — jamais un recu
// device. Mutants device par drapeaux (l'hote traduit MHGP6_MUTANT en bits,
// le device ne consulte jamais le registre) :
//   bit 0 gpu-range-add-le      (prefiltre : ajout de sous-arbre a mx <= 0
//                                — les coquilles comptees interieures) ;
//   bit 1 gpu-stack-shallow     (pile coupee a 8 : le statut overflow doit
//                                APPARAITRE sur un arbre legitime) ;
//   bit 2 gpu-swap-push-order   (pushes inverses : l'ordre des listes
//                                diverge du scalaire) ;
//   bit 3 gpu-census-nonstrict  (census : puissance nulle comptee
//                                interieure).
#pragma once

#include "../core/types.hpp"

#if defined(__CUDACC__) || defined(MHGP6_FAKE_DEVICE)

#if defined(__CUDACC__) && !defined(MHGP6_FAKE_DEVICE)
#define MHGP6_DEV __device__
#ifndef MHGP6_LAUNCH
#define MHGP6_LAUNCH(kernel, blocks, threads, ...) kernel<<<(blocks), (threads)>>>(__VA_ARGS__)
#endif
#else
#define MHGP6_DEV inline
#endif

namespace mhgp6 {
namespace gpu {

inline constexpr u8 kBallOk = 0, kBallAtLeastH = 1, kBallInteriorOverflow = 2, kBallShellOverflow = 3,
                    kBallStackOverflow = 4;
inline constexpr int kDfsStackCap = 64;
inline constexpr u32 kMutRangeAddLe = 1u, kMutStackShallow = 2u, kMutSwapPush = 4u, kMutCensusNonstrict = 8u,
                     kMutSkipBallWrite = 16u, kMutNshellOverdomain = 32u, kMutSkipCountWrite = 64u;
// SENTINELLES D2H (contre-lecture f3704e99) : les tampons de sortie sont
// PREREMPLIS par l'hote — une ecriture device omise laisse la sentinelle et
// le VALIDATEUR refuse AVANT toute reconstruction (jamais une lecture hors
// borne sur des comptes non produits).
inline constexpr u8 kSentinelStatus = 0xee;
inline constexpr i32 kSentinelId = -9;
inline constexpr size_t kBallWords = 14;  // 5 x i128 (paires u64) + 6 candidats u32 + h u64
inline constexpr int kOutIdsPerBall = 21;  // interieur [0, 9), coquille [9, 21)

struct DeviceBall {
  i128 a, b[3], c;
  u32 c0[3], c1[3];  // candidats du sommet, clamp_domaine hote (§ 5.11 — aucune division ni +1 device)
  u64 h;
};

MHGP6_DEV i128 dev_i128(u64 lo, u64 hi) { return (i128)(((u128)hi << 64) | (u128)lo); }

MHGP6_DEV DeviceBall load_ball(const u64* w) {
  DeviceBall d;
  d.a = dev_i128(w[0], w[1]);
  for (int i = 0; i < 3; ++i) d.b[i] = dev_i128(w[2 + 2 * i], w[3 + 2 * i]);
  d.c = dev_i128(w[8], w[9]);
  for (int i = 0; i < 3; ++i) {
    d.c0[i] = (u32)w[10 + i];
    d.c1[i] = (u32)(w[10 + i] >> 32);
  }
  d.h = w[13];
  return d;
}

MHGP6_DEV i128 dev_axis_val(i128 a, i128 b, i64 t) { return a * ((i128)t * t) + b * t; }

MHGP6_DEV i64 dev_clamp(i64 v, i64 lo, i64 hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Minimum d'axe par la liste de candidats du wire : {clamp_boite(c0),
// clamp_boite(c1), lo, hi} — exact car la parabole est convexe (a > 0) et
// c0/c1 sont clamp_domaine(t1)/clamp_domaine(t1+1) hote : pour toute boite
// incluse dans [0, 65535], rabattre domaine puis boite == rabattre boite
// (§ 5.11). Aucune division device.
MHGP6_DEV i128 dev_axis_min(i128 a, i128 b, u32 c0, u32 c1, i64 lo, i64 hi) {
  i128 best = dev_axis_val(a, b, dev_clamp((i64)c0, lo, hi));
  const i128 v2 = dev_axis_val(a, b, dev_clamp((i64)c1, lo, hi));
  if (v2 < best) best = v2;
  const i128 vl = dev_axis_val(a, b, lo);
  if (vl < best) best = vl;
  const i128 vh = dev_axis_val(a, b, hi);
  if (vh < best) best = vh;
  return best;
}

MHGP6_DEV void dev_bounds(const DeviceBall& d, const u16* box6, i128* mn, i128* mx) {
  *mn = d.c;
  *mx = d.c;
  for (int i = 0; i < 3; ++i) {
    const i64 lo = (i64)box6[i], hi = (i64)box6[3 + i];
    *mn += dev_axis_min(d.a, d.b[i], d.c0[i], d.c1[i], lo, hi);
    const i128 vl = dev_axis_val(d.a, d.b[i], lo), vh = dev_axis_val(d.a, d.b[i], hi);
    *mx += vl > vh ? vl : vh;
  }
}

MHGP6_DEV i128 dev_power(const DeviceBall& d, const u16* upos3) {
  const i64 x = (i64)upos3[0], y = (i64)upos3[1], z = (i64)upos3[2];
  return d.a * ((i128)x * x + (i128)y * y + (i128)z * z) + d.b[0] * x + d.b[1] * y + d.b[2] * z + d.c;
}

// PREFILTRE (C3) : rend le statut (ok | at_least_h | stack_overflow) et, au
// statut ok, le compte EXACT des interieurs stricts (poids par prefixe).
__global__ void k_prefilter(const i32* nl, const i32* nr, const i32* nfirst, const i32* nlast,
                            const u16* nbox, const u16* upos, const u32* wsum, i32 root,
                            const u64* balls, u32 n_balls, u64* out_count, u8* out_status, u32 mut) {
  const u32 gid = blockIdx.x * blockDim.x + threadIdx.x;
  if (gid >= n_balls) return;
  if ((mut & kMutSkipBallWrite) && (gid % 4096u) == 7u) return;  // MUTANT : sortie jamais produite
  const DeviceBall d = load_ball(balls + (size_t)gid * kBallWords);
  const int cap = (mut & kMutStackShallow) ? 8 : kDfsStackCap;
  i32 stack[kDfsStackCap];
  int sp = 0;
  stack[sp++] = root;
  u64 c = 0;
  u8 status = kBallOk;
  while (sp > 0) {
    const i32 z = stack[--sp];
    i128 mn, mx;
    if (z >= 0) {
      dev_bounds(d, nbox + (size_t)z * 6, &mn, &mx);
    } else {
      const i32 u = -1 - z;  // leaf_index
      const i128 pw = dev_power(d, upos + (size_t)u * 3);
      mn = pw;
      mx = pw;
    }
    if (mn >= 0) continue;
    const bool full = (mut & kMutRangeAddLe) ? (mx <= 0) : (mx < 0);
    if (full) {
      i32 first, last;
      if (z >= 0) {
        first = nfirst[(size_t)z];
        last = nlast[(size_t)z];
      } else {
        first = last = -1 - z;
      }
      c += (u64)wsum[(size_t)last + 1] - (u64)wsum[(size_t)first];
      if (c >= d.h) {
        status = kBallAtLeastH;
        break;
      }
      continue;
    }
    // Ici z est forcement interne (une feuille a mn == mx).
    if (sp + 2 > cap) {
      status = kBallStackOverflow;
      break;
    }
    if (mut & kMutSwapPush) {
      stack[sp++] = nr[(size_t)z];
      stack[sp++] = nl[(size_t)z];
    } else {
      stack[sp++] = nl[(size_t)z];  // ordre du scalaire : gauche puis droite
      stack[sp++] = nr[(size_t)z];
    }
  }
  if (!((mut & kMutSkipCountWrite) && (gid % 4096u) == 5u))  // MUTANT : count seul jamais ecrit
    out_count[gid] = c;
  out_status[gid] = status;
}

// CENSUS (C4) : listes interieur/coquille BIT-IDENTIQUES au scalaire
// (ball_census : prune mn > 0, chaque feuille survivante testee une a une).
// interior_cap = h - 1 (= smax - arite), shell_cap constant. Les 21 slots
// sont zeros d'abord (tampons deterministes pour tout digest brut).
__global__ void k_census(const i32* nl, const i32* nr, const u16* nbox, const u16* upos, i32 root,
                         const u64* balls, u32 n_balls, u32 base_idx, u32 shell_cap, i32* out_ids,
                         u8* out_status, u8* out_nint, u8* out_nshell, u32* out_cand, u32 mut) {
  const u32 gid = blockIdx.x * blockDim.x + threadIdx.x;
  if (gid >= n_balls) return;
  if ((mut & kMutSkipBallWrite) && (gid % 4096u) == 7u) return;  // MUTANT : sortie jamais produite
  const DeviceBall d = load_ball(balls + (size_t)gid * kBallWords);
  const u64 interior_cap = d.h - 1;
  i32* ids = out_ids + (size_t)gid * kOutIdsPerBall;
  for (int i = 0; i < kOutIdsPerBall; ++i) ids[i] = 0;
  const int cap = (mut & kMutStackShallow) ? 8 : kDfsStackCap;
  i32 stack[kDfsStackCap];
  int sp = 0;
  stack[sp++] = root;
  u32 n_int = 0, n_sh = 0;
  u8 status = kBallOk;
  while (sp > 0) {
    const i32 z = stack[--sp];
    if (z < 0) {
      const i32 u = -1 - z;
      const i128 pw = dev_power(d, upos + (size_t)u * 3);
      if (pw < 0 || ((mut & kMutCensusNonstrict) && pw == 0)) {
        if ((u64)n_int + 1 > interior_cap || n_int >= 9) {
          status = kBallInteriorOverflow;
          break;
        }
        ids[n_int++] = u;
      } else if (pw == 0) {
        if (n_sh + 1 > shell_cap || n_sh >= 12) {
          status = kBallShellOverflow;
          break;
        }
        ids[9 + n_sh++] = u;
      }
      continue;
    }
    i128 mn, mx;
    dev_bounds(d, nbox + (size_t)z * 6, &mn, &mx);
    if (mn > 0) continue;  // strict : mn == 0 descend (coquilles a voir)
    if (sp + 2 > cap) {
      status = kBallStackOverflow;
      break;
    }
    if (mut & kMutSwapPush) {
      stack[sp++] = nr[(size_t)z];
      stack[sp++] = nl[(size_t)z];
    } else {
      stack[sp++] = nl[(size_t)z];
      stack[sp++] = nr[(size_t)z];
    }
  }
  out_status[gid] = status;
  out_nint[gid] = (u8)n_int;
  out_nshell[gid] = ((mut & kMutNshellOverdomain) && (gid % 4096u) == 3u) ? (u8)13 : (u8)n_sh;  // MUTANT
  out_cand[gid] = base_idx + gid;
}

// VALIDATEUR D2H CENTRALISE (hote, f3704e99) : statut CONNU, comptes dans le
// profil, cand_idx attendu, tous les ids dans [0, n_upos) — verifie AVANT
// toute reconstruction (une ecriture omise ou corrompue ne devient jamais
// une borne de lecture). Rend nullptr si conforme, sinon le motif du refus.
inline const char* validate_ball_out(u8 pstat, u8 cstat, u8 n_int, u8 n_sh, u32 cand, u32 expected_cand,
                                     const i32* ids21, u32 n_upos, u64 count, u64 h, u64 total_mass) {
  // ORDRE DES DENTS (bc5812dc) : sentinelles de STATUT d'abord —
  // gpu-skip-ball-write (qui omet statuts ET count) doit mourir sur SA dent
  // de statut, jamais sur celle du count.
  if (pstat == kSentinelStatus || cstat == kSentinelStatus)
    return "invariant : ecriture device omise (sentinelle survivante)";
  // COUNT sous contrat (8c60cb8e) : sentinelle refusee, borne par la masse
  // totale, coherent avec le statut (ok => count < h ; at_least_h =>
  // count >= h ; stack_overflow : partiel, borne par la masse seulement).
  if (count == ~0ull) return "invariant : count jamais ecrit (sentinelle survivante)";
  if (count > total_mass) return "invariant : count au-dela de la masse totale";
  if (pstat == kBallOk && count >= h) return "invariant : statut ok avec count >= h";
  if (pstat == kBallAtLeastH && count < h) return "invariant : statut at_least_h avec count < h";
  // DOMAINES PAR ETAGE (69817569) : le prefiltre n'autorise que
  // ok|at_least_h|stack_overflow, le census que ok|interior_overflow|
  // shell_overflow|stack_overflow — les combinaisons croisees sont refusees.
  if (!(pstat == kBallOk || pstat == kBallAtLeastH || pstat == kBallStackOverflow))
    return "invariant : statut de prefiltre hors domaine";
  if (!(cstat == kBallOk || cstat == kBallInteriorOverflow || cstat == kBallShellOverflow ||
        cstat == kBallStackOverflow))
    return "invariant : statut de census hors domaine";
  if (n_int > 9 || n_sh > 12) return "invariant : comptes hors profil au retour device";
  if (cand != expected_cand) return "invariant : cand_idx inattendu au retour device";
  for (u8 j = 0; j < n_int; ++j)
    if (ids21[j] < 0 || (u32)ids21[j] >= n_upos) return "invariant : id upos hors domaine au retour device";
  for (u8 j = 0; j < n_sh; ++j)
    if (ids21[9 + j] < 0 || (u32)ids21[9 + j] >= n_upos)
      return "invariant : id upos hors domaine au retour device";
  return nullptr;
}

}  // namespace gpu
}  // namespace mhgp6

#endif  // __CUDACC__ || MHGP6_FAKE_DEVICE
