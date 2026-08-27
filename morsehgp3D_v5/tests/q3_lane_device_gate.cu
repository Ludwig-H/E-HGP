// MorseHGP3D v5 — porte de la LANE q3 DEVICE (src/gpu/q3_lane_device.cuh) contre
// la lane q3 de production (generate_candidates, arite 3) : egalite post-RLE
// des candidats (cle, niveau) et des compteurs de la lane q3 (ancres, tues
// par histogramme, seeds, tues par profondeur, candidats, q3_cert x3) ; a un
// fil, egalite de l'ordre brut aussi. Planchers : --min-candidates,
// --min-killed. Codes : 0, 1 desaccord, 2 refus (pas de device), 3 plancher.
// Imprime aussi le temps kernel cumule et le nombre de lancements (mesure,
// pas un claim).
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/q3_lane_device.cuh"

using namespace mhgp5;

int main(int argc, char** argv) {
  CloudFamily family = CloudFamily::kUniform;
  int n = 1200, coord = 0, threads = 1;
  size_t spl = kSeedsPerLaunch;
  u64 min_flushes = 1;
  BatchStats bs;
  u64 min_candidates = 1000, min_killed = 10;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--family=", 0) == 0) { if (!parse_cloud_family(arg.c_str() + 9, &family)) return 2; }
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--coord=", 0) == 0) coord = std::atoi(arg.c_str() + 8);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else if (arg.rfind("--min-candidates=", 0) == 0) min_candidates = (u64)std::atoll(arg.c_str() + 17);
    else if (arg.rfind("--min-killed=", 0) == 0) min_killed = (u64)std::atoll(arg.c_str() + 13);
    else if (arg.rfind("--seeds-per-launch=", 0) == 0) {
      const long long v = std::atoll(arg.c_str() + 19);
      if (v < 1) return 2;
      spl = (size_t)v;
    } else if (arg.rfind("--min-flushes=", 0) == 0) min_flushes = (u64)std::atoll(arg.c_str() + 14);
    else return 2;
  }
  int ndev = 0;
  if (cudaGetDeviceCount(&ndev) != cudaSuccess || ndev < 1) {
    std::fprintf(stderr, "REFUS : aucun device CUDA visible\n");
    return 2;
  }
  if (coord <= 0) coord = cloud_family_default_coord(family, n);
  const CloudIndex ix = build_cloud_index(make_family_input(family, n, coord, 3));
  if (!ix.valid || ix.has_duplicate_positions()) return 2;
  GenerateOptions opt;
  opt.threads = threads;
  std::vector<BallCandidate> prod_all, prod, dev;
  GenerateStats sp, sd;
  generate_candidates(ix, opt, &prod_all, &sp);
  for (const BallCandidate& c : prod_all)
    if (c.arity == 3) prod.push_back(c);
  double kernel_ms = 0;
  u64 launches = 0;
  try {
    gpu::generate_q3_device(ix, opt, &dev, &sd, &kernel_ms, &launches, spl, &bs);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "REFUS : %s\n", e.what());
    return 2;
  }
  u64 bad = 0;
  auto cmp = [&](const char* what, u64 a, u64 b) {
    if (a != b) {
      std::printf("desaccord %s : production=%llu device=%llu\n", what, (unsigned long long)a, (unsigned long long)b);
      ++bad;
    }
  };
  cmp("rect_alive", sp.rect_alive[1], sd.rect_alive[1]);
  cmp("anchors", sp.anchors[1], sd.anchors[1]);
  cmp("anchors_killed_hist", sp.anchors_killed_hist[1], sd.anchors_killed_hist[1]);
  cmp("seeds", sp.seeds[0], sd.seeds[0]);
  cmp("depth_killed", sp.depth_killed[1], sd.depth_killed[1]);
  cmp("candidates", sp.candidates[1], sd.candidates[1]);
  cmp("q3_cert_neg", sp.q3_cert[0], sd.q3_cert[0]);
  cmp("q3_cert_pos", sp.q3_cert[1], sd.q3_cert[1]);
  cmp("q3_fallback", sp.q3_cert[2], sd.q3_cert[2]);
  auto count_mism = [](const std::vector<BallCandidate>& a, const std::vector<BallCandidate>& b) {
    u64 m = a.size() != b.size() ? 1 : 0;
    for (size_t i = 0; i < a.size() && i < b.size(); ++i)
      if (!(a[i].key == b[i].key) || !(a[i].level == b[i].level) || a[i].arity != b[i].arity) ++m;
    return m;
  };
  u64 vec_mism = threads == 1 ? count_mism(prod, dev) : 0;
  rle_candidates(&prod, 1);
  rle_candidates(&dev, 1);
  vec_mism += count_mism(prod, dev);
  std::printf("q3_lane_device famille=%s n=%d fils=%d seuil=%zu vidages=%llu max_lot_seeds=%llu max_ancre_seeds=%llu candidats_q3=%zu seeds=%llu tues=%llu replis=%llu lancements=%llu "
              "kernel_ms=%.1f desaccords_vecteur=%llu desaccords_compteurs=%llu\n",
              cloud_family_name(family), n, threads, spl, (unsigned long long)bs.flushes, (unsigned long long)bs.max_lot_seeds,
              (unsigned long long)bs.max_anchor_seeds, prod.size(), (unsigned long long)sp.seeds[0],
              (unsigned long long)sp.depth_killed[1], (unsigned long long)sp.q3_cert[2], (unsigned long long)launches,
              kernel_ms, (unsigned long long)vec_mism, (unsigned long long)bad);
  if (sp.candidates[1] < min_candidates || sp.depth_killed[1] < min_killed) {
    std::printf("PLANCHER\n");
    return 3;
  }
  if (bs.max_lot_seeds > (u64)spl + bs.max_anchor_seeds || bs.flushes < min_flushes || launches < 1) {
    std::printf("LOTISSEMENT : borne ou vidages ou lancements hors contrat\n");
    return 1;
  }
  if (vec_mism || bad) return 1;
  std::printf("q3_lane_device OK\n");
  return 0;
}
