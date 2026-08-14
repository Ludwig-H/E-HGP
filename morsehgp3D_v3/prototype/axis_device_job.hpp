// MorseHGP3D v3 — LE LOT `Q4SeedAxisTopR4`, HOTE ET DEVICE.
//
// Ce fichier ne contient AUCUNE geometrie. Il transporte un lot plat et appelle
// `q4axis::select_axis_topr4`, ecrite une seule fois et compilee pour les deux
// cibles. C'est la condition pour qu'une comparaison bit a bit ait un sens :
// sans cela on comparerait deux implementations et non deux executions.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference_et_cuda,
//         profile=quantized_u16_input_only, mode=diagnostic_exact_borne,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CE LOT MESURE, ET POURQUOI CELUI-LA
//
// A `smax=6`, `n=6000`, le noyau d'axe divise les candidats par cinquante-neuf
// — `48 791 131` paires de lentille contre `830 044` racines — mais ne deplace
// pas le mur de temps, parce que la selection balaie encore TOUS les sites de
// chaque `Q4Seed3`. Ce balayage est donc le terme dominant, et c'est exactement
// lui que ce lot met sur le device.
//
// Il ne mesure NI la chaine complete, NI un `warm_e2e`, NI un SLO. Il mesure le
// debit d'un noyau, et la parite de ses verdicts.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"
#include "prototype/q4seed_axis_topr4.hpp"

namespace mhgp3v {
namespace axisdev {

using mhgp::i64;

// Capacite de sortie par cote. La borne du theoreme vaut `r4-p <= 8` GROUPES ;
// une concurrence peut depasser en IDs, et le verdict `DEBORDEMENT` du noyau le
// signale alors — jamais une troncature muette.
inline constexpr int kCapSortie = 24;

struct AxisJob {
  const i64* coords = nullptr;      // `3 * npoints`, entrelacees
  int npoints = 0;
  const int* seed = nullptr;        // `3 * nseeds` : les trois `PointId`
  const int* site_offset = nullptr; // `nseeds + 1`, CSR
  const int* site_id = nullptr;     // sites de chaque seed
  int nseeds = 0;
  int r4 = 8;
};

// Le verdict transporte, champ par champ, pour une comparaison bit a bit.
struct SeedOut {
  int verdict = 0;
  int p = 0, k = 0, n_entrants = 0, n_sortants = 0, prof_min = 0;
  int deborde_sortie = 0;
  int entrants[kCapSortie] = {0};
  int sortants[kCapSortie] = {0};
};

MHGP_HD inline void evaluate_seed(const AxisJob& job, int index, SeedOut* out) {
  const int* s3 = job.seed + 3 * index;
  const i64* a = job.coords + 3 * s3[0];
  const i64* b = job.coords + 3 * s3[1];
  const i64* x = job.coords + 3 * s3[2];
  const q4axis::SeedAxis f = q4axis::seed_axis(a, b, x);
  const int deb = job.site_offset[index];
  const int fin = job.site_offset[index + 1];
  auto pw = [&](int id) { return q4axis::site_power(f, job.coords + 3 * id); };
  const q4axis::Selection sel =
      q4axis::select_axis_topr4(f, job.site_id + deb, fin - deb, pw, job.r4);
  out->verdict = (int)sel.verdict;
  out->p = sel.p;
  out->k = sel.k;
  out->prof_min = sel.profondeur_min_minoree;
  out->n_entrants = sel.n_entrants;
  out->n_sortants = sel.n_sortants;
  out->deborde_sortie = 0;
  for (int t = 0; t < kCapSortie; ++t) {
    out->entrants[t] = (t < sel.n_entrants) ? sel.entrants[t] : -1;
    out->sortants[t] = (t < sel.n_sortants) ? sel.sortants[t] : -1;
  }
  if (sel.n_entrants > kCapSortie || sel.n_sortants > kCapSortie)
    out->deborde_sortie = 1;
}

// Defini par `axis_device_kernel.cu` uniquement. Un binaire compile sans CUDA
// n'a simplement pas ce symbole, et son chemin hote reste qualifiable seul.
bool launch_axis_on_gpu(const AxisJob& job, SeedOut* out, double* milliseconds,
                        char* error, int error_capacity);

}  // namespace axisdev
}  // namespace mhgp3v
