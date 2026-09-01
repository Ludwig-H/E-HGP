// MorseHGP3D v6 — PILOTE SERIE C (C5, session G4 sous MHGP6_ENABLE_CUDA) :
// le MEME nuage passe par la route CPU de production puis par la route
// device (index wire resident, lots a double tampon logique, kernels
// prefiltre+census, reconstruction hote) ; le pilote exige l'EGALITE de
// TOUS les digests (candidats, post-prefiltre, objet, dix forets), cartes et
// totaux, puis imprime les temps — mur des deux routes, et POUR LA ROUTE
// DEVICE les couts separes wire/H2D/kernels/D2H (exigence § 5.5 : couts de
// transfert mesures, repetitions par --repeat). L'architecture compilee et
// le device observe sont signes (le recu les certifie).
//
// Codes : 0 parite complete ; 1 divergence ; 2 refus (arguments, CUDA,
// statut non complet) ; 3 plancher. Ce binaire n'existe qu'en session G4 —
// il ne porte AUCUN claim hors reçu (public_status=not_claimed).
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/core/parse.hpp"
#include "../src/gpu/census_kernels.cuh"
#include "../src/gpu/wire.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp6;

#if !defined(__CUDACC__) && !defined(MHGP6_FAKE_DEVICE)
int main() {
  std::fprintf(stderr, "REFUS : pilote serie C compile sans nvcc\n");
  return 2;
}
#else

#define CUDA_OK_RET(call, msg)                                                                    \
  do {                                                                                            \
    cudaError_t e_ = (call);                                                                      \
    if (e_ != cudaSuccess) return std::string(msg) + " : " + cudaGetErrorString(e_);              \
  } while (0)

namespace {

double now_ms() {
  static const auto t0 = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

struct GpuRouteCosts {
  double wire_ms = 0, h2d_ms = 0, kernel_ms = 0, d2h_ms = 0, rebuild_ms = 0;
  u64 h2d_bytes = 0, d2h_bytes = 0, lots = 0;
};

// Route device : appelable comme RunOptions::prefilter_census_override.
std::string device_route(const CloudIndex& ix, const std::vector<BallCandidate>& cands, u64 smax,
                         size_t shell_cap, std::vector<Survivor>* surv, std::vector<BallData>* balls,
                         ExpandStats* st, size_t lot_balls, GpuRouteCosts* costs) {
  const double t_w0 = now_ms();
  const gpu::GpuCloudIndexWire w = gpu::build_index_wire(ix);
  if (!w.error.empty()) return w.error;
  gpu::GpuBallInWire bw;
  for (const BallCandidate& bc : cands) gpu::append_ball_in(&bw, bc.key, smax + 1 - (u64)bc.arity);
  if (!bw.error.empty()) return bw.error;
  costs->wire_ms += now_ms() - t_w0;
  const u64 nb_total = bw.balls;
  if (nb_total == 0) return "";

  // Index resident (une fois).
  const double t_h0 = now_ms();
  i32 *d_nl = nullptr, *d_nr = nullptr, *d_nf = nullptr, *d_nlast = nullptr, *d_ids = nullptr;
  u16 *d_nbox = nullptr, *d_up = nullptr;
  u32 *d_ws = nullptr, *d_cand = nullptr;
  u64 *d_balls = nullptr, *d_count = nullptr;
  u8 *d_status = nullptr, *d_cst = nullptr, *d_nint = nullptr, *d_nsh = nullptr;
  const auto up1 = [&](void** p, const std::vector<u8>& src) -> std::string {
    CUDA_OK_RET(cudaMalloc(p, src.size()), "cudaMalloc index");
    CUDA_OK_RET(cudaMemcpy(*p, src.data(), src.size(), cudaMemcpyHostToDevice), "cudaMemcpy index");
    costs->h2d_bytes += src.size();
    return "";
  };
  std::string e;
  if (!(e = up1((void**)&d_nl, w.node_left)).empty()) return e;
  if (!(e = up1((void**)&d_nr, w.node_right)).empty()) return e;
  if (!(e = up1((void**)&d_nf, w.node_first)).empty()) return e;
  if (!(e = up1((void**)&d_nlast, w.node_last)).empty()) return e;
  if (!(e = up1((void**)&d_nbox, w.node_box)).empty()) return e;
  if (!(e = up1((void**)&d_up, w.upos)).empty()) return e;
  if (!(e = up1((void**)&d_ws, w.wsum)).empty()) return e;
  // --lot INOFFENSIF (e9cfad9e) : lot_eff = min(lot demande, nb_total) — un
  // lot geant sur un petit nuage n'echoue jamais ; produits d'allocation
  // verifies avant cudaMalloc (aucun rebouclage size_t).
  const size_t lot = std::min<size_t>(lot_balls == 0 ? (size_t)nb_total : lot_balls, (size_t)nb_total);
  if (lot > (size_t)1 << 40) return "invalid_input : lot au-dela de toute allocation sensee";
  CUDA_OK_RET(cudaMalloc(&d_balls, lot * gpu::kBallWords * 8), "cudaMalloc lot");
  CUDA_OK_RET(cudaMalloc(&d_count, lot * 8), "cudaMalloc count");
  CUDA_OK_RET(cudaMalloc(&d_status, lot), "cudaMalloc status");
  CUDA_OK_RET(cudaMalloc(&d_ids, lot * gpu::kOutIdsPerBall * 4), "cudaMalloc ids");
  CUDA_OK_RET(cudaMalloc(&d_cst, lot), "cudaMalloc cstatus");
  CUDA_OK_RET(cudaMalloc(&d_nint, lot), "cudaMalloc nint");
  CUDA_OK_RET(cudaMalloc(&d_nsh, lot), "cudaMalloc nsh");
  CUDA_OK_RET(cudaMalloc(&d_cand, lot * 4), "cudaMalloc cand");
  costs->h2d_ms += now_ms() - t_h0;

  // Sentinelles D2H (f3704e99) : une ecriture omise ne se consomme jamais.
  std::vector<u64> count(nb_total, ~0ull);
  std::vector<u8> pst(nb_total, gpu::kSentinelStatus), cst(nb_total, gpu::kSentinelStatus),
      nint(nb_total, 0xff), nsh(nb_total, 0xff);
  std::vector<i32> ids((size_t)nb_total * gpu::kOutIdsPerBall, gpu::kSentinelId);
  std::vector<u32> candi(nb_total, 0xffffffffu);
  // Sentinelles DEVICE par lot (69817569 : sans preremplissage device, une
  // ecriture omise relirait une allocation CUDA indeterminee, pas la
  // sentinelle) — tampons hote de taille lot, reutilises a chaque lot.
  const std::vector<u64> sl_count((size_t)lot, ~0ull);
  const std::vector<u8> sl_status((size_t)lot, gpu::kSentinelStatus);
  const std::vector<u8> sl_cnt((size_t)lot, 0xff);
  const std::vector<i32> sl_ids((size_t)lot * gpu::kOutIdsPerBall, gpu::kSentinelId);
  const std::vector<u32> sl_cand((size_t)lot, 0xffffffffu);
  for (u64 base = 0; base < nb_total; base += lot) {
    const u32 nb = (u32)std::min<u64>(lot, nb_total - base);
    ++costs->lots;
    const double t_l0 = now_ms();
    // Trafic des sentinelles COMPTE (8c60cb8e : une protection de surete
    // n'apparait jamais gratuitement dans le reçu) : 8+1+84+1+1+1+4 = 100 o
    // par boule du lot.
    costs->h2d_bytes += (size_t)nb * (8 + 1 + (size_t)gpu::kOutIdsPerBall * 4 + 1 + 1 + 1 + 4);
    CUDA_OK_RET(cudaMemcpy(d_count, sl_count.data(), (size_t)nb * 8, cudaMemcpyHostToDevice), "sentinelle count");
    CUDA_OK_RET(cudaMemcpy(d_status, sl_status.data(), nb, cudaMemcpyHostToDevice), "sentinelle status");
    CUDA_OK_RET(cudaMemcpy(d_ids, sl_ids.data(), (size_t)nb * gpu::kOutIdsPerBall * 4, cudaMemcpyHostToDevice),
                "sentinelle ids");
    CUDA_OK_RET(cudaMemcpy(d_cst, sl_status.data(), nb, cudaMemcpyHostToDevice), "sentinelle cstatus");
    CUDA_OK_RET(cudaMemcpy(d_nint, sl_cnt.data(), nb, cudaMemcpyHostToDevice), "sentinelle nint");
    CUDA_OK_RET(cudaMemcpy(d_nsh, sl_cnt.data(), nb, cudaMemcpyHostToDevice), "sentinelle nsh");
    CUDA_OK_RET(cudaMemcpy(d_cand, sl_cand.data(), (size_t)nb * 4, cudaMemcpyHostToDevice), "sentinelle cand");
    CUDA_OK_RET(cudaMemcpy(d_balls, bw.bytes.data() + base * gpu::kBallWords * 8,
                           (size_t)nb * gpu::kBallWords * 8, cudaMemcpyHostToDevice),
                "cudaMemcpy lot");
    costs->h2d_bytes += (size_t)nb * gpu::kBallWords * 8;
    const double t_k0 = now_ms();
    costs->h2d_ms += t_k0 - t_l0;
    MHGP6_LAUNCH(gpu::k_prefilter, (nb + 255) / 256, 256, d_nl, d_nr, d_nf, d_nlast, d_nbox, d_up, d_ws,
                 w.root, d_balls, nb, d_count, d_status, 0u);
    CUDA_OK_RET(cudaGetLastError(), "k_prefilter");
    // MUTANT gpu-lot-base-reset (e9cfad9e) : base toujours 0 — le validateur
    // refuse cand_idx des le second lot (la fusion en ordre GLOBAL est jugee).
    const u32 kbase = MHGP6_MUTANT("gpu-lot-base-reset") ? 0u : (u32)base;
    MHGP6_LAUNCH(gpu::k_census, (nb + 255) / 256, 256, d_nl, d_nr, d_nbox, d_up, w.root, d_balls, nb,
                 kbase, (u32)shell_cap, d_ids, d_cst, d_nint, d_nsh, d_cand, 0u);
    CUDA_OK_RET(cudaGetLastError(), "k_census");
    CUDA_OK_RET(cudaDeviceSynchronize(), "cudaDeviceSynchronize");
    const double t_d0 = now_ms();
    costs->kernel_ms += t_d0 - t_k0;
    CUDA_OK_RET(cudaMemcpy(count.data() + base, d_count, (size_t)nb * 8, cudaMemcpyDeviceToHost), "d2h count");
    CUDA_OK_RET(cudaMemcpy(pst.data() + base, d_status, nb, cudaMemcpyDeviceToHost), "d2h status");
    CUDA_OK_RET(cudaMemcpy(ids.data() + base * gpu::kOutIdsPerBall, d_ids,
                           (size_t)nb * gpu::kOutIdsPerBall * 4, cudaMemcpyDeviceToHost),
                "d2h ids");
    CUDA_OK_RET(cudaMemcpy(cst.data() + base, d_cst, nb, cudaMemcpyDeviceToHost), "d2h cstatus");
    CUDA_OK_RET(cudaMemcpy(nint.data() + base, d_nint, nb, cudaMemcpyDeviceToHost), "d2h nint");
    CUDA_OK_RET(cudaMemcpy(nsh.data() + base, d_nsh, nb, cudaMemcpyDeviceToHost), "d2h nsh");
    CUDA_OK_RET(cudaMemcpy(candi.data() + base, d_cand, (size_t)nb * 4, cudaMemcpyDeviceToHost), "d2h cand");
    costs->d2h_bytes += (size_t)nb * (8 + 1 + 1 + 1 + 1 + 4 + gpu::kOutIdsPerBall * 4);
    costs->d2h_ms += now_ms() - t_d0;
  }
  for (void* p : {(void*)d_nl, (void*)d_nr, (void*)d_nf, (void*)d_nlast, (void*)d_nbox, (void*)d_up,
                  (void*)d_ws, (void*)d_balls, (void*)d_count, (void*)d_status, (void*)d_ids, (void*)d_cst,
                  (void*)d_nint, (void*)d_nsh, (void*)d_cand})
    CUDA_OK_RET(cudaFree(p), "cudaFree");

  // Reconstruction HOTE TRANSACTIONNELLE (8c60cb8e) : temporaires + stats
  // locales, publication a la fin seulement.
  const double t_r0 = now_ms();
  std::vector<Survivor> lsurv;
  std::vector<BallData> lballs;
  u64 dead = 0, l_int = 0, l_sh = 0;
  const u64 total_mass = ix.wsum.back();
  for (u64 i = 0; i < nb_total; ++i) {
    const i32* row = &ids[(size_t)i * gpu::kOutIdsPerBall];
    const u64 h = smax + 1 - (u64)cands[i].arity;
    if (const char* why = gpu::validate_ball_out(pst[i], cst[i], nint[i], nsh[i], candi[i], (u32)i, row,
                                                 w.n_upos, count[i], h, total_mass))
      return why;
    if (pst[i] == gpu::kBallStackOverflow || cst[i] == gpu::kBallStackOverflow)
      return "invariant : pile DFS au-dela du profil (49) sur device";
    if (pst[i] == gpu::kBallAtLeastH) {
      ++dead;
      continue;
    }
    if (cst[i] == gpu::kBallShellOverflow) return "coquille au-dela du plafond (jamais de troncature)";
    if (cst[i] == gpu::kBallInteriorOverflow || (u64)nint[i] != count[i])
      return "invariant : census device contredit la passe count-only";
    lsurv.push_back(Survivor{(u32)i, count[i]});
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
  costs->rebuild_ms += now_ms() - t_r0;
  return "";
}

}  // namespace

int main(int argc, char** argv) {
  // Parsing EXACT (doctrine v6 : jamais atoll — suffixe, vide, debordement
  // sont des refus code 2).
  std::string family = "uniform";
  std::string inject;
  i64 n = 50000, seed = 3, threads = 48, repeat = 1, lot = 1 << 21, min_lots = 0;
  bool ok = true;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    const auto val = [&](const char* p) -> const char* {
      return a.rfind(p, 0) == 0 ? a.c_str() + std::strlen(p) : nullptr;
    };
    if (const char* s = val("--family=")) family = s;
    else if (const char* s = val("--n=")) ok = parse_i64_exact(s, &n) && ok;
    else if (const char* s = val("--seed=")) ok = parse_i64_exact(s, &seed) && ok;
    else if (const char* s = val("--threads=")) ok = parse_i64_exact(s, &threads) && ok;
    else if (const char* s = val("--repeat=")) ok = parse_i64_exact(s, &repeat) && ok;
    else if (const char* s = val("--lot=")) ok = parse_i64_exact(s, &lot) && ok;
    else if (const char* s = val("--min-lots=")) ok = parse_i64_exact(s, &min_lots) && ok;
    else if (const char* s = val("--inject=")) inject = s;
    else {
      std::fprintf(stderr, "REFUS : argument inconnu %s\n", a.c_str());
      return 2;
    }
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;  // refuse en build produit
  if (!ok || n < 2 || n > (i64)kMaxTreePositions || repeat < 1 || repeat > 1000 || lot < 1 ||
      threads < 1 || threads > 1024) {
    std::fprintf(stderr, "REFUS : parametre hors domaine ou mal forme\n");
    return 2;
  }
  CloudFamily fam;
  if (!parse_cloud_family(family.c_str(), &fam)) {
    std::fprintf(stderr, "REFUS : famille inconnue %s\n", family.c_str());
    return 2;
  }

  int ndev = 0;
  if (cudaGetDeviceCount(&ndev) != cudaSuccess || ndev < 1) {
    std::fprintf(stderr, "REFUS : aucun device CUDA visible\n");
    return 2;
  }
  cudaDeviceProp prop{};
  if (cudaGetDeviceProperties(&prop, 0) != cudaSuccess) return 2;
  std::printf("pilote_serie_c device=%s sm=%d.%d arch_compilees=%s famille=%s n=%lld graine=%lld fils=%lld lot=%lld\n",
              prop.name, prop.major, prop.minor,
#ifdef MHGP6_CUDA_ARCHS
              MHGP6_CUDA_ARCHS,
#else
              "non_signees",
#endif
              family.c_str(), (long long)n, (long long)seed, (long long)threads, (long long)lot);

  const std::vector<InputPoint> in = make_family_input(fam, (int)n, cloud_family_default_coord(fam, (int)n), (long long)seed);
  for (i64 r = 0; r < repeat; ++r) {
    RunOptions cpu;
    cpu.s = 8;
    cpu.smax = 11;
    cpu.threads = (int)threads;
    cpu.digest = true;
    const auto t_a0 = std::chrono::steady_clock::now();
    const RunResult a = run_pipeline(in, cpu);
    const double mur_cpu = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t_a0).count();
    if (a.status != PipelineStatus::kCompleteRegular) {
      std::fprintf(stderr, "REFUS : route CPU non complete (%s)\n", a.message.c_str());
      return 2;
    }
    GpuRouteCosts costs;
    RunOptions dev = cpu;
    dev.prefilter_census_override = [&](const CloudIndex& ix, const std::vector<BallCandidate>& cands,
                                        u64 smax_eff, size_t shell_cap, std::vector<Survivor>* sv,
                                        std::vector<BallData>* bd, ExpandStats* st) {
      return device_route(ix, cands, smax_eff, shell_cap, sv, bd, st, (size_t)lot, &costs);
    };
    const auto t_b0 = std::chrono::steady_clock::now();
    const RunResult b = run_pipeline(in, dev);
    const double mur_gpu = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t_b0).count();
    if (MHGP6_MUTANT("gpu-lot-base-reset")) {
      // Scene-signature multi-lots : base toujours 0 => cand_idx inattendu
      // des le second lot, refus par le VALIDATEUR (fusion en ordre global
      // jugee) — la scene exige lots > 1, sinon plancher.
      if (costs.lots < 2) {
        std::printf("PLANCHER : la scene mutante exige lots > 1 (lots=%llu)\n",
                    (unsigned long long)costs.lots);
        return 3;
      }
      if (b.status != PipelineStatus::kCompleteRegular &&
          b.message.find("cand_idx inattendu") != std::string::npos) {
        std::printf("mutant gpu-lot-base-reset TUE : cand_idx refuse au second lot (lots=%llu)\n",
                    (unsigned long long)costs.lots);
        return 4;
      }
      std::printf("MUTANT NON TUE (statut=%d)\n", (int)b.status);
      return 1;
    }
    if (b.status != PipelineStatus::kCompleteRegular) {
      std::fprintf(stderr, "REFUS : route device non complete (%s)\n", b.message.c_str());
      return 2;
    }
    if (min_lots > 0 && costs.lots < (u64)min_lots) {
      std::printf("PLANCHER : lots=%llu < min_lots=%lld (la frontiere multi-lots n'est pas exercee)\n",
                  (unsigned long long)costs.lots, (long long)min_lots);
      return 3;
    }
    const bool same = a.digest_all == b.digest_all && a.digest_balls == b.digest_balls &&
                      a.digest_postprefilter == b.digest_postprefilter && a.digest_forest == b.digest_forest &&
                      a.cards == b.cards && a.total_events == b.total_events && a.emitted == b.emitted;
    std::printf("repetition=%lld parite=%s mur_cpu_ms=%.1f mur_route_device_ms=%.1f "
                "prefiltre_census_cpu_ms=%.1f route_device_etage_ms=%.1f "
                "wire_ms=%.1f h2d_ms=%.1f kernels_ms=%.1f d2h_ms=%.1f rebuild_ms=%.1f "
                "h2d_octets=%llu d2h_octets=%llu lots=%llu digest_all=%s\n",
                (long long)r, same ? "OUI" : "NON", mur_cpu, mur_gpu, a.t_prefilter_ms + a.t_census_ms,
                b.t_census_ms,
                costs.wire_ms, costs.h2d_ms, costs.kernel_ms, costs.d2h_ms, costs.rebuild_ms,
                (unsigned long long)costs.h2d_bytes, (unsigned long long)costs.d2h_bytes,
                (unsigned long long)costs.lots, a.digest_all.c_str());
    if (!same) {
      std::printf("DIVERGENCE : l'objet des deux routes differe\n");
      return 1;
    }
  }
  return 0;
}
#endif
