// MorseHGP3D v4 — TÉMOIN DE COMPILATION DEVICE (réponse d'audit
// « fold compact, vrai préflight et fermeture device » § 4) : ce
// fichier n'est PAS compilé localement (aucun nvcc dans le conteneur) —
// la PREMIÈRE session G4 avec nvcc doit le compiler AVANT tout kernel
// et tout benchmark. Il prouve la fermeture transitive des annotations
// MHGP4_HD en appelant chaque primitive arithmétique exacte depuis un
// __global__. Aucune exécution requise pour le témoin de compilation ;
// le selftest d'EXÉCUTION device (mêmes témoins que
// mhgp3v_arith_selftest) viendra avec les kernels.
#include "../events/q4_event.hpp"
#include "../pipeline/ball_stream.hpp"

namespace mhgp4 {

__global__ void mhgp4_device_compile_witness(const i64* in, int* out) {
  const i128 a = (i128)in[0] * in[1];
  const u128 ua = detail_ev::uabs(a);
  const U192 m = mul_level_192(ua, (u128)(u64)in[2]);
  const u64 nw[3] = {m.w[0], m.w[1], m.w[2]};
  const U320 p = mul_192_128_to_320(nw, ua);
  Q4Level l1{{m.w[0], m.w[1], m.w[2]}, ua};
  Q4Level l2 = l1;
  int acc = cmp_u192(m, m) + cmp_u320(p, p);
  acc += compare_exact_level(l1, l2);
  acc += same_exact_level(l1, l2) ? 1 : 0;
  acc += cmp_mu_same_side(a, in[3], a, in[3]);
  acc += cmp_2p2_jb2(-a, a, in[3]);
  out[0] = acc;
}

}  // namespace mhgp4
