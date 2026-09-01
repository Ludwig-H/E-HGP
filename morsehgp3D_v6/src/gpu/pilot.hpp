// MorseHGP3D v6 — ROUTE PREFILTRE+CENSUS SERIE C (C5, docs/GPU.md § Wire) :
// l'implementation de `RunOptions::prefilter_census_override` — wire, un
// aller-retour kernel (prefiltre puis census), reconstruction HOTE des
// survivants et des BallData dans l'ORDRE DES CANDIDATS (le meme que la
// route CPU), invariant n_int == depth croise comme en production. Sous le
// stub (MHGP6_FAKE_DEVICE), les kernels tournent en boucles hote : la porte
// pilote prouve l'EGALITE DES DIGESTS bout en bout localement — jamais un
// recu device. Sous nvcc, le pilote (cli/mhgp6_cuda.cu) fournit la
// version a tampons device, lots et chronometrage.
//
// Refus TRANSACTIONNELS (jamais un prefixe) : pile DFS au-dela du profil,
// coquille au-dela du plafond, census contredisant la passe count-only —
// message prefixe "invariant :" quand c'est une faute d'implementation.
#pragma once

#include <string>
#include <vector>

#include "../pipeline/expand.hpp"
#include "census_kernels.cuh"
#include "wire.hpp"

#if defined(__CUDACC__) || defined(MHGP6_FAKE_DEVICE)

namespace mhgp6 {
namespace gpu {

inline std::string stub_prefilter_census_route(const CloudIndex& ix, const std::vector<BallCandidate>& cands,
                                               u64 smax, size_t shell_cap, std::vector<Survivor>* surv,
                                               std::vector<BallData>* balls, ExpandStats* st, u32 mut = 0) {
  const GpuCloudIndexWire w = build_index_wire(ix);
  if (!w.error.empty()) return w.error;
  GpuBallInWire bw;
  for (const BallCandidate& bc : cands) append_ball_in(&bw, bc.key, smax + 1 - (u64)bc.arity);
  if (!bw.error.empty()) return bw.error;
  const u32 nb = (u32)bw.balls;
  if (nb == 0) {
    // bc5812dc : le cas VIDE publie des sorties vides — jamais les anciennes
    // valeurs du caller conservees en silence.
    surv->clear();
    balls->clear();
    st->dead_depth = 0;
    st->survivors = 0;
    return "";
  }

  // VUES TYPEES (§ 5.11) : decodage explicite — jamais un reinterpret_cast
  // des octets (alignement/aliasing indefinis sur l'hote).
  const GpuIndexHostView v = decode_index_wire(w);
  const std::vector<u64> words = decode_ball_words(bw);
  const i32* nl = v.node_left.data();
  const i32* nr = v.node_right.data();
  const i32* nf = v.node_first.data();
  const i32* nlast = v.node_last.data();
  const u16* nbox = v.node_box.data();
  const u16* up = v.upos.data();
  const u32* ws = v.wsum.data();
  const u64* bwords = words.data();

  // Tampons PREREMPLIS de sentinelles (f3704e99) : une ecriture omise est
  // detectee par le validateur, jamais consommee.
  std::vector<u64> count(nb, ~0ull);
  std::vector<u8> pstatus(nb, kSentinelStatus);
  MHGP6_LAUNCH(k_prefilter, (nb + 255) / 256, 256, nl, nr, nf, nlast, nbox, up, ws, w.root, bwords, nb,
               count.data(), pstatus.data(), mut);
  std::vector<i32> ids((size_t)nb * kOutIdsPerBall, kSentinelId);
  std::vector<u8> cstatus(nb, kSentinelStatus), nint(nb, 0xff), nsh(nb, 0xff);
  std::vector<u32> candi(nb, 0xffffffffu);
  MHGP6_LAUNCH(k_census, (nb + 255) / 256, 256, nl, nr, nbox, up, w.root, bwords, nb, 0u, (u32)shell_cap,
               ids.data(), cstatus.data(), nint.data(), nsh.data(), candi.data(), mut);

  // RECONSTRUCTION TRANSACTIONNELLE (8c60cb8e) : temporaires + statistiques
  // LOCALES, echange/publication a la toute fin — un refus tardif ne laisse
  // jamais un prefixe dans surv/balls ni des compteurs deja incrementes.
  std::vector<Survivor> lsurv;
  std::vector<BallData> lballs;
  u64 dead = 0, l_int = 0, l_sh = 0;
  const u64 total_mass = ix.wsum.back();
  for (u32 i = 0; i < nb; ++i) {
    const i32* row = &ids[(size_t)i * kOutIdsPerBall];
    const u64 h = smax + 1 - (u64)cands[i].arity;
    // VALIDATEUR D2H CENTRALISE avant TOUTE interpretation (f3704e99 +
    // count/h/masse de 8c60cb8e).
    if (const char* why = validate_ball_out(pstatus[i], cstatus[i], nint[i], nsh[i], candi[i], i, row,
                                            w.n_upos, count[i], h, total_mass))
      return why;
    if (pstatus[i] == kBallStackOverflow || cstatus[i] == kBallStackOverflow)
      return "invariant : pile DFS au-dela du profil (49) sur la route device";
    if (pstatus[i] == kBallAtLeastH) {
      ++dead;
      continue;
    }
    // Survivant : le compte exact du prefiltre EST la profondeur.
    const u64 depth = count[i];
    if (cstatus[i] == kBallShellOverflow) return "coquille au-dela du plafond (jamais de troncature)";
    if (cstatus[i] == kBallInteriorOverflow || (u64)nint[i] != depth)
      return "invariant : census contredit la passe count-only (route device)";
    lsurv.push_back(Survivor{i, depth});
    const BallCandidate& bc = cands[i];
    BallData bd;
    bd.key = bc.key;
    bd.level = bc.level;
    bd.arity = bc.arity;
    bd.n_interior = nint[i];
    bd.n_shell = nsh[i];
    for (u8 j = 0; j < bd.n_interior; ++j) bd.interior_ids[j] = row[j];
    for (u8 j = 0; j < bd.n_shell; ++j) bd.shell_ids[j] = row[9 + j];
    lballs.push_back(bd);
    l_int += bd.n_interior;
    l_sh += bd.n_shell;
  }
  surv->swap(lsurv);
  balls->swap(lballs);
  st->census_interior += l_int;
  st->census_shell += l_sh;
  st->dead_depth = dead;
  st->survivors = surv->size();
  return "";
}

}  // namespace gpu
}  // namespace mhgp6

#endif  // __CUDACC__ || MHGP6_FAKE_DEVICE
