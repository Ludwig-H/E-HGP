// MorseHGP3D v6 — PORTE STUB des kernels C3/C4 (prefiltre + census device)
// contre le SCALAIRE de production, boule a boule, sur les candidats REELS
// du pipeline (generation -> tri -> RLE). Preuve C++ HOTE de la logique
// bit-identique — jamais un recu device (les portes gpu reelles n'existent
// que sous MHGP6_ENABLE_CUDA, session G4).
//
// Exige, par famille (uniform 400 et eight_clusters 400, graine 3) :
//   (1) PREFILTRE : pour CHAQUE candidat, statut device ==
//       ball_depth_at_least (at_least_h <=> tue) et compte EXACT egal quand
//       la boule survit ;
//   (2) CENSUS : pour chaque SURVIVANT, statut, n_int, n_shell et les DEUX
//       listes d'ids BIT-IDENTIQUES en ORDRE (ordre de pile du scalaire) ;
//   (3) planchers de non-vacuite : >= 20 000 boules jugees, >= 100 tuees au
//       prefiltre, >= 100 survivants a coquille >= 4, masse de range-add
//       CPU > 0 (le chemin d'ajout de sous-arbre est exerce) ;
//   (4) aucun statut stack_overflow au nominal (pile legitime <= 49).
// Codes : 0 ; 1 desaccord ; 2 refus ; 3 plancher ; 4 mutant tue —
//   gpu-range-add-le      : >= 1 divergence de prefiltre (eight_clusters
//                           porte la contre-fixture mx == 0, comme le mutant
//                           CPU range-add-max-le-zero) ;
//   gpu-stack-shallow     : >= 1 stack_overflow sur un arbre legitime ;
//   gpu-swap-push-order   : >= 1 divergence d'ORDRE a multiset EGAL (la
//                           cause est bien l'ordre, pas le contenu) ;
//   gpu-census-nonstrict  : >= 1 coquille comptee interieure.
#define MHGP6_FAKE_DEVICE 1
#include "cuda_stub.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/census_kernels.cuh"
#include "../src/gpu/wire.hpp"
#include "../src/pipeline/candidates.hpp"
#include "../src/pipeline/census.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp6;

namespace {
int failures = 0;
void expect(bool ok, const char* what) {
  if (!ok) {
    std::printf("ECHEC : %s\n", what);
    ++failures;
  } else {
    std::printf("ok : %s\n", what);
  }
}
}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--inject=", 0) == 0) inject = a.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  u32 mut = 0;
  if (MHGP6_MUTANT("gpu-range-add-le")) mut |= gpu::kMutRangeAddLe;
  if (MHGP6_MUTANT("gpu-stack-shallow")) mut |= gpu::kMutStackShallow;
  if (MHGP6_MUTANT("gpu-swap-push-order")) mut |= gpu::kMutSwapPush;
  if (MHGP6_MUTANT("gpu-census-nonstrict")) mut |= gpu::kMutCensusNonstrict;
  if (MHGP6_MUTANT("gpu-skip-ball-write")) mut |= gpu::kMutSkipBallWrite;
  if (MHGP6_MUTANT("gpu-nshell-overdomain")) mut |= gpu::kMutNshellOverdomain;
  if (MHGP6_MUTANT("gpu-skip-count-write")) mut |= gpu::kMutSkipCountWrite;
  const bool mutant = mut != 0;
  // Motifs SELECTIFS (bc5812dc) : chaque dent meurt sur SON motif.
  u64 refus_statut_omis = 0, refus_count_omis = 0, refus_comptes_profil = 0, refus_autre = 0;

  const u64 smax = 11;
  u64 total_balls = 0, total_dead = 0, total_shell4 = 0;
  u64 prefilter_div = 0, overflow_seen = 0, order_div_same_multiset = 0, census_div = 0;
  u64 cpu_range_add_mass = 0;

  for (const CloudFamily fam : {CloudFamily::kUniform, CloudFamily::kEightClusters}) {
    const std::vector<InputPoint> in = make_family_input(fam, 400, cloud_family_default_coord(fam, 400), 3);
    const CloudIndex ix = build_cloud_index(in);
    GenerateOptions go;
    go.s = 8;
    go.smax = smax;
    go.threads = 4;
    std::vector<BallCandidate> cands;
    GenerateStats gs;
    generate_candidates(ix, go, &cands, &gs);
    if (gs.cap_refus != kCapRefusNone) return 2;
    sort_candidates(&cands, 4);
    deduplicate_candidates(&cands);

    // Wire : index + boules.
    const gpu::GpuCloudIndexWire w = gpu::build_index_wire(ix);
    if (!w.error.empty()) {
      std::printf("REFUS : %s\n", w.error.c_str());
      return 2;
    }
    gpu::GpuBallInWire bw;
    for (const BallCandidate& bc : cands) gpu::append_ball_in(&bw, bc.key, smax + 1 - (u64)bc.arity);
    if (!bw.error.empty()) return 2;

    // VUES TYPEES (§ 5.11) : decodage explicite — jamais un reinterpret_cast
    // des octets sur l'hote (alignement/aliasing).
    const gpu::GpuIndexHostView v = gpu::decode_index_wire(w);
    const std::vector<u64> words = gpu::decode_ball_words(bw);
    const i32* nl = v.node_left.data();
    const i32* nr = v.node_right.data();
    const i32* nf = v.node_first.data();
    const i32* nlast = v.node_last.data();
    const u16* nbox = v.node_box.data();
    const u16* up = v.upos.data();
    const u32* ws = v.wsum.data();
    const u64* balls = words.data();
    const u32 nb = (u32)bw.balls;

    std::vector<u64> d_count(nb, ~0ull);
    std::vector<u8> d_status(nb, gpu::kSentinelStatus);
    MHGP6_LAUNCH(gpu::k_prefilter, (nb + 255) / 256, 256, nl, nr, nf, nlast, nbox, up, ws, w.root, balls,
                 nb, d_count.data(), d_status.data(), mut);

    std::vector<u8> c_status(nb, gpu::kSentinelStatus), c_nint(nb, 0xff), c_nsh(nb, 0xff);
    std::vector<i32> c_ids((size_t)nb * gpu::kOutIdsPerBall, gpu::kSentinelId);
    std::vector<u32> c_cand(nb, 0xffffffffu);
    MHGP6_LAUNCH(gpu::k_census, (nb + 255) / 256, 256, nl, nr, nbox, up, w.root, balls, nb, 0u, 12u,
                 c_ids.data(), c_status.data(), c_nint.data(), c_nsh.data(), c_cand.data(), mut);

    // Confrontation boule a boule au scalaire de production.
    for (u32 i = 0; i < nb; ++i) {
      const BallCandidate& bc = cands[i];
      const u64 h = smax + 1 - (u64)bc.arity;
      ++total_balls;
      // VALIDATEUR D2H d'abord (f3704e99 + count/h/masse 8c60cb8e).
      if (const char* why = gpu::validate_ball_out(d_status[i], c_status[i], c_nint[i], c_nsh[i],
                                                   c_cand[i], i, &c_ids[(size_t)i * gpu::kOutIdsPerBall],
                                                   w.n_upos, d_count[i], h, ix.wsum.back())) {
        const std::string m = why;
        if (m.find("ecriture device omise") != std::string::npos) ++refus_statut_omis;
        else if (m.find("count jamais ecrit") != std::string::npos) ++refus_count_omis;
        else if (m.find("comptes hors profil") != std::string::npos) ++refus_comptes_profil;
        else ++refus_autre;
        continue;
      }
      u64 cpu_count = 0;
      DepthStats ds;
      const bool cpu_dead = ball_depth_at_least(ix, bc.key, h, &cpu_count, &ds);
      cpu_range_add_mass += ds.range_add_mass;
      if (d_status[i] == gpu::kBallStackOverflow) {
        ++overflow_seen;
        continue;
      }
      const bool dev_dead = d_status[i] == gpu::kBallAtLeastH;
      if (cpu_dead != dev_dead || (!cpu_dead && cpu_count != d_count[i])) {
        ++prefilter_div;
        continue;
      }
      if (cpu_dead) {
        ++total_dead;
        continue;
      }
      // Census sur les survivants seulement (comme le pipeline).
      std::vector<i32> ci, cs;
      const CensusStatus st = ball_census(ix, bc.key, (size_t)(smax - bc.arity), 12, &ci, &cs);
      if (c_status[i] == gpu::kBallStackOverflow) {
        ++overflow_seen;
        continue;
      }
      const u8 want = st == CensusStatus::kOk ? gpu::kBallOk
                      : st == CensusStatus::kInteriorOverflow ? gpu::kBallInteriorOverflow
                                                              : gpu::kBallShellOverflow;
      if (c_status[i] != want) {
        ++census_div;
        continue;
      }
      if (st != CensusStatus::kOk) continue;
      const i32* ids = &c_ids[(size_t)i * gpu::kOutIdsPerBall];
      bool same = c_nint[i] == ci.size() && c_nsh[i] == cs.size() && c_cand[i] == i;
      for (size_t j = 0; same && j < ci.size(); ++j) same = ids[j] == ci[j];
      for (size_t j = 0; same && j < cs.size(); ++j) same = ids[9 + j] == cs[j];
      if (!same) {
        // La cause est-elle l'ORDRE seul (multiset egal) ? — la dent de
        // gpu-swap-push-order, distinguee d'une divergence de contenu.
        std::vector<i32> a(ids, ids + c_nint[i]), b = ci;
        std::vector<i32> a2(ids + 9, ids + 9 + c_nsh[i]), b2 = cs;
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        std::sort(a2.begin(), a2.end());
        std::sort(b2.begin(), b2.end());
        if (c_nint[i] == ci.size() && c_nsh[i] == cs.size() && a == b && a2 == b2)
          ++order_div_same_multiset;
        else
          ++census_div;
      }
      if (cs.size() >= 4) ++total_shell4;
    }
  }

  std::printf("bilan boules=%llu tuees=%llu coquille4=%llu masse_range_add=%llu "
              "div_prefiltre=%llu div_census=%llu div_ordre=%llu overflow=%llu\n",
              (unsigned long long)total_balls, (unsigned long long)total_dead,
              (unsigned long long)total_shell4, (unsigned long long)cpu_range_add_mass,
              (unsigned long long)prefilter_div, (unsigned long long)census_div,
              (unsigned long long)order_div_same_multiset, (unsigned long long)overflow_seen);

  const u64 validator_refus = refus_statut_omis + refus_count_omis + refus_comptes_profil + refus_autre;
  std::printf("validateur_refus=%llu (statut_omis=%llu count_omis=%llu comptes_profil=%llu autre=%llu)\n",
              (unsigned long long)validator_refus, (unsigned long long)refus_statut_omis,
              (unsigned long long)refus_count_omis, (unsigned long long)refus_comptes_profil,
              (unsigned long long)refus_autre);
  if (mutant) {
    // SELECTIVITE (bc5812dc) : chaque dent exige EXACTEMENT son motif —
    // skip-ball-write meurt sur la dent de STATUT (les statuts sont testes
    // avant le count), skip-count-write sur « count jamais ecrit » SEUL.
    if ((mut & gpu::kMutSkipBallWrite) && refus_statut_omis > 0 && refus_count_omis == 0) {
      std::printf("mutant gpu-skip-ball-write TUE : %llu statuts omis (dent de statut, jamais celle du count)\n",
                  (unsigned long long)refus_statut_omis);
      return 4;
    }
    if ((mut & gpu::kMutNshellOverdomain) && refus_comptes_profil > 0 && refus_statut_omis == 0) {
      std::printf("mutant gpu-nshell-overdomain TUE : %llu comptes hors profil refuses\n",
                  (unsigned long long)refus_comptes_profil);
      return 4;
    }
    if ((mut & gpu::kMutSkipCountWrite) && refus_count_omis > 0 && refus_statut_omis == 0) {
      std::printf("mutant gpu-skip-count-write TUE : %llu counts jamais ecrits (motif exact)\n",
                  (unsigned long long)refus_count_omis);
      return 4;
    }
    if ((mut & gpu::kMutRangeAddLe) && prefilter_div > 0) {
      std::printf("mutant gpu-range-add-le TUE : %llu divergences de prefiltre\n",
                  (unsigned long long)prefilter_div);
      return 4;
    }
    if ((mut & gpu::kMutStackShallow) && overflow_seen > 0) {
      std::printf("mutant gpu-stack-shallow TUE : %llu stack_overflow sur arbre legitime\n",
                  (unsigned long long)overflow_seen);
      return 4;
    }
    if ((mut & gpu::kMutSwapPush) && order_div_same_multiset > 0 && census_div == 0) {
      std::printf("mutant gpu-swap-push-order TUE : %llu divergences d'ORDRE a multiset egal\n",
                  (unsigned long long)order_div_same_multiset);
      return 4;
    }
    if ((mut & gpu::kMutCensusNonstrict) && census_div > 0) {
      std::printf("mutant gpu-census-nonstrict TUE : %llu divergences de census\n",
                  (unsigned long long)census_div);
      return 4;
    }
    std::printf("MUTANT NON TUE\n");
    return 1;
  }

  expect(validator_refus == 0, "validateur D2H : aucune sentinelle ni valeur hors profil au nominal");
  expect(prefilter_div == 0, "prefiltre bit-identique au scalaire (statut et compte exact)");
  expect(census_div == 0 && order_div_same_multiset == 0, "census bit-identique (statut, comptes, listes EN ORDRE)");
  expect(overflow_seen == 0, "aucun stack_overflow au nominal (pile legitime <= 49)");
  expect(total_balls >= 20000, "plancher : >= 20 000 boules jugees");
  expect(total_dead >= 100, "plancher : >= 100 boules tuees au prefiltre");
  expect(total_shell4 >= 100, "plancher : >= 100 survivants a coquille >= 4");
  expect(cpu_range_add_mass > 0, "plancher : chemin d'ajout de sous-arbre exerce");
  return failures ? 1 : 0;
}
