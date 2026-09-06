// MorseHGP3D v6 — PORTE DEVICE REELLE des kernels C3/C4 (session G4,
// MHGP7_ENABLE_CUDA seulement) : MEMES exigences que la porte stub
// (tests/census_device_stub_gate.cpp — bit-identite boule a boule contre le
// scalaire de production, candidats reels, ordre de pile compris, 4 mutants
// par drapeaux), mais les kernels tournent sur le DEVICE (cudaMalloc/Memcpy,
// erreurs CUDA = refus 2). Le recu doit certifier l'architecture compilee et
// le device observe (imprimes ici ; § 5.8 : CMAKE_CUDA_ARCHITECTURES est
// surchargeable — la ligne device=... fait foi au reçu).
// Codes : 0 ; 1 desaccord ; 2 refus ; 3 plancher ; 4 mutant tue.
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

using namespace mhgp7;

#if !defined(__CUDACC__)
int main() {
  std::fprintf(stderr, "REFUS : porte device compilee sans nvcc\n");
  return 2;
}
#else

#define CUDA_OK(call)                                                                                 \
  do {                                                                                                \
    cudaError_t e_ = (call);                                                                          \
    if (e_ != cudaSuccess) {                                                                          \
      std::fprintf(stderr, "REFUS cuda : %s (%s:%d)\n", cudaGetErrorString(e_), __FILE__, __LINE__); \
      return 2;                                                                                       \
    }                                                                                                 \
  } while (0)

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

template <class T>
struct DevBuf {
  T* p = nullptr;
  ~DevBuf() {
    if (p) cudaFree(p);
  }
};

template <class T>
int upload(DevBuf<T>* d, const void* src, size_t bytes) {
  if (cudaMalloc(&d->p, bytes) != cudaSuccess) return 1;
  if (cudaMemcpy(d->p, src, bytes, cudaMemcpyHostToDevice) != cudaSuccess) return 1;
  return 0;
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
  if (MHGP7_MUTANT("gpu-range-add-le")) mut |= gpu::kMutRangeAddLe;
  if (MHGP7_MUTANT("gpu-stack-shallow")) mut |= gpu::kMutStackShallow;
  if (MHGP7_MUTANT("gpu-swap-push-order")) mut |= gpu::kMutSwapPush;
  if (MHGP7_MUTANT("gpu-census-nonstrict")) mut |= gpu::kMutCensusNonstrict;
  if (MHGP7_MUTANT("gpu-skip-ball-write")) mut |= gpu::kMutSkipBallWrite;
  if (MHGP7_MUTANT("gpu-nshell-overdomain")) mut |= gpu::kMutNshellOverdomain;
  if (MHGP7_MUTANT("gpu-skip-count-write")) mut |= gpu::kMutSkipCountWrite;
  const bool mutant = mut != 0;
  u64 refus_statut_omis = 0, refus_count_omis = 0, refus_comptes_profil = 0, refus_autre = 0;

  int ndev = 0;
  if (cudaGetDeviceCount(&ndev) != cudaSuccess || ndev < 1) {
    std::fprintf(stderr, "REFUS : aucun device CUDA visible\n");
    return 2;
  }
  cudaDeviceProp prop{};
  CUDA_OK(cudaGetDeviceProperties(&prop, 0));
  std::printf("device=%s sm=%d.%d arch_compilees=%s\n", prop.name, prop.major, prop.minor,
#ifdef MHGP7_CUDA_ARCHS
              MHGP7_CUDA_ARCHS
#else
              "non_signees"
#endif
  );

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

    const gpu::GpuCloudIndexWire w = gpu::build_index_wire(ix);
    if (!w.error.empty()) return 2;
    gpu::GpuBallInWire bw;
    for (const BallCandidate& bc : cands) gpu::append_ball_in(&bw, bc.key, smax + 1 - (u64)bc.arity);
    if (!bw.error.empty()) return 2;
    const u32 nb = (u32)bw.balls;

    DevBuf<i32> d_nl, d_nr, d_nf, d_nlast, d_ids;
    DevBuf<u16> d_nbox, d_up;
    DevBuf<u32> d_ws, d_cand;
    DevBuf<u64> d_balls, d_count;
    DevBuf<u8> d_status, d_cst, d_nint, d_nsh;
    if (upload(&d_nl, w.node_left.data(), w.node_left.size()) || upload(&d_nr, w.node_right.data(), w.node_right.size()) ||
        upload(&d_nf, w.node_first.data(), w.node_first.size()) || upload(&d_nlast, w.node_last.data(), w.node_last.size()) ||
        upload(&d_nbox, w.node_box.data(), w.node_box.size()) || upload(&d_up, w.upos.data(), w.upos.size()) ||
        upload(&d_ws, w.wsum.data(), w.wsum.size()) || upload(&d_balls, bw.bytes.data(), bw.bytes.size())) {
      std::fprintf(stderr, "REFUS cuda : televersement\n");
      return 2;
    }
    // RELECTURE COMPLETE des sept tableaux apres upload (§ 5.11 : identite
    // des octets RESIDENTS — cout borne 38*m-24 octets, HORS de tout mur de
    // benchmark ; la route produit se contente du digest hote + erreurs CUDA).
    {
      const auto readback_eq = [&](const void* dev, const std::vector<u8>& src) -> bool {
        std::vector<u8> back(src.size());
        if (cudaMemcpy(back.data(), dev, src.size(), cudaMemcpyDeviceToHost) != cudaSuccess) return false;
        return back == src;
      };
      const bool same = readback_eq(d_nl.p, w.node_left) && readback_eq(d_nr.p, w.node_right) &&
                        readback_eq(d_nf.p, w.node_first) && readback_eq(d_nlast.p, w.node_last) &&
                        readback_eq(d_nbox.p, w.node_box) && readback_eq(d_up.p, w.upos) &&
                        readback_eq(d_ws.p, w.wsum);
      expect(same, "relecture : les sept tableaux residents == payload hote (identite des octets)");
      if (!same) return 1;
    }
    // SENTINELLES TELEVERSEES (f3704e99 : cudaMalloc n'initialise pas — sans
    // prerempli, une ecriture omise resterait invisible) puis VALIDATEUR.
    std::vector<u64> s_count(nb, ~0ull);
    std::vector<u8> s_status(nb, gpu::kSentinelStatus), s_cnt(nb, 0xff);
    std::vector<i32> s_ids((size_t)nb * gpu::kOutIdsPerBall, gpu::kSentinelId);
    std::vector<u32> s_cand(nb, 0xffffffffu);
    CUDA_OK(cudaMalloc(&d_count.p, (size_t)nb * 8));
    CUDA_OK(cudaMalloc(&d_status.p, nb));
    CUDA_OK(cudaMalloc(&d_ids.p, (size_t)nb * gpu::kOutIdsPerBall * 4));
    CUDA_OK(cudaMalloc(&d_cst.p, nb));
    CUDA_OK(cudaMalloc(&d_nint.p, nb));
    CUDA_OK(cudaMalloc(&d_nsh.p, nb));
    CUDA_OK(cudaMalloc(&d_cand.p, (size_t)nb * 4));
    CUDA_OK(cudaMemcpy(d_count.p, s_count.data(), (size_t)nb * 8, cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_status.p, s_status.data(), nb, cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_ids.p, s_ids.data(), (size_t)nb * gpu::kOutIdsPerBall * 4, cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_cst.p, s_status.data(), nb, cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_nint.p, s_cnt.data(), nb, cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_nsh.p, s_cnt.data(), nb, cudaMemcpyHostToDevice));
    CUDA_OK(cudaMemcpy(d_cand.p, s_cand.data(), (size_t)nb * 4, cudaMemcpyHostToDevice));

    gpu::k_prefilter<<<(nb + 255) / 256, 256>>>(d_nl.p, d_nr.p, d_nf.p, d_nlast.p, d_nbox.p, d_up.p,
                                                d_ws.p, w.root, d_balls.p, nb, d_count.p, d_status.p, mut);
    CUDA_OK(cudaGetLastError());
    gpu::k_census<<<(nb + 255) / 256, 256>>>(d_nl.p, d_nr.p, d_nbox.p, d_up.p, w.root, d_balls.p, nb, 0u,
                                             12u, d_ids.p, d_cst.p, d_nint.p, d_nsh.p, d_cand.p, mut);
    CUDA_OK(cudaGetLastError());
    CUDA_OK(cudaDeviceSynchronize());
    std::vector<u64> h_count(nb);
    std::vector<u8> h_status(nb), h_cst(nb), h_nint(nb), h_nsh(nb);
    std::vector<i32> h_ids((size_t)nb * gpu::kOutIdsPerBall);
    std::vector<u32> h_cand(nb);
    CUDA_OK(cudaMemcpy(h_count.data(), d_count.p, (size_t)nb * 8, cudaMemcpyDeviceToHost));
    CUDA_OK(cudaMemcpy(h_status.data(), d_status.p, nb, cudaMemcpyDeviceToHost));
    CUDA_OK(cudaMemcpy(h_ids.data(), d_ids.p, (size_t)nb * gpu::kOutIdsPerBall * 4, cudaMemcpyDeviceToHost));
    CUDA_OK(cudaMemcpy(h_cst.data(), d_cst.p, nb, cudaMemcpyDeviceToHost));
    CUDA_OK(cudaMemcpy(h_nint.data(), d_nint.p, nb, cudaMemcpyDeviceToHost));
    CUDA_OK(cudaMemcpy(h_nsh.data(), d_nsh.p, nb, cudaMemcpyDeviceToHost));
    // dd9d8092 : nb * sizeof(u32) — la copie a `nb` octets laissait h_cand a
    // zero et produisait un faux rouge sur G4 ; la sentinelle mord desormais.
    CUDA_OK(cudaMemcpy(h_cand.data(), d_cand.p, (size_t)nb * 4, cudaMemcpyDeviceToHost));

    for (u32 i = 0; i < nb; ++i) {
      const BallCandidate& bc = cands[i];
      const u64 h = smax + 1 - (u64)bc.arity;
      ++total_balls;
      // VALIDATEUR D2H d'abord (f3704e99 + count/h/masse 8c60cb8e).
      if (const char* why = gpu::validate_ball_out(h_status[i], h_cst[i], h_nint[i], h_nsh[i], h_cand[i],
                                                   i, &h_ids[(size_t)i * gpu::kOutIdsPerBall], w.n_upos,
                                                   h_count[i], h, ix.wsum.back())) {
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
      if (h_status[i] == gpu::kBallStackOverflow) {
        ++overflow_seen;
        continue;
      }
      const bool dev_dead = h_status[i] == gpu::kBallAtLeastH;
      if (cpu_dead != dev_dead || (!cpu_dead && cpu_count != h_count[i])) {
        ++prefilter_div;
        continue;
      }
      if (cpu_dead) {
        ++total_dead;
        continue;
      }
      std::vector<i32> ci, cs;
      const CensusStatus st = ball_census(ix, bc.key, (size_t)(smax - bc.arity), 12, &ci, &cs);
      if (h_cst[i] == gpu::kBallStackOverflow) {
        ++overflow_seen;
        continue;
      }
      const u8 want = st == CensusStatus::kOk ? gpu::kBallOk
                      : st == CensusStatus::kInteriorOverflow ? gpu::kBallInteriorOverflow
                                                              : gpu::kBallShellOverflow;
      if (h_cst[i] != want) {
        ++census_div;
        continue;
      }
      if (st != CensusStatus::kOk) continue;
      const i32* ids = &h_ids[(size_t)i * gpu::kOutIdsPerBall];
      bool same = h_nint[i] == ci.size() && h_nsh[i] == cs.size() && h_cand[i] == i;
      for (size_t j = 0; same && j < ci.size(); ++j) same = ids[j] == ci[j];
      for (size_t j = 0; same && j < cs.size(); ++j) same = ids[9 + j] == cs[j];
      if (!same) {
        std::vector<i32> a(ids, ids + h_nint[i]), b = ci;
        std::vector<i32> a2(ids + 9, ids + 9 + h_nsh[i]), b2 = cs;
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        std::sort(a2.begin(), a2.end());
        std::sort(b2.begin(), b2.end());
        if (h_nint[i] == ci.size() && h_nsh[i] == cs.size() && a == b && a2 == b2)
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
    if ((mut & gpu::kMutSkipBallWrite) && refus_statut_omis > 0 && refus_count_omis == 0) return 4;
    if ((mut & gpu::kMutNshellOverdomain) && refus_comptes_profil > 0 && refus_statut_omis == 0) return 4;
    if ((mut & gpu::kMutSkipCountWrite) && refus_count_omis > 0 && refus_statut_omis == 0) return 4;
    if ((mut & gpu::kMutRangeAddLe) && prefilter_div > 0) return 4;
    if ((mut & gpu::kMutStackShallow) && overflow_seen > 0) return 4;
    if ((mut & gpu::kMutSwapPush) && order_div_same_multiset > 0 && census_div == 0) return 4;
    if ((mut & gpu::kMutCensusNonstrict) && census_div > 0) return 4;
    std::printf("MUTANT NON TUE\n");
    return 1;
  }
  expect(validator_refus == 0, "validateur D2H : aucune sentinelle au nominal");
  expect(prefilter_div == 0, "prefiltre device bit-identique au scalaire");
  expect(census_div == 0 && order_div_same_multiset == 0, "census device bit-identique (ordre compris)");
  expect(overflow_seen == 0, "aucun stack_overflow au nominal");
  expect(total_balls >= 20000, "plancher : >= 20 000 boules jugees");
  expect(total_dead >= 100 && total_shell4 >= 100 && cpu_range_add_mass > 0, "planchers de masse");
  return failures ? 1 : 0;
}
#endif

