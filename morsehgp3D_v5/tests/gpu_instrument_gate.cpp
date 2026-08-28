// MorseHGP3D v5 — porte de l'INSTRUMENT device recevable (audit
// AUDIT_RENDEMENT_GPU_MULTICPU_20260828, § reception de 63deda74), partie
// verifiable SANS nvcc : src/gpu/device_stats.hpp (statistiques d'executeur,
// pic de flux) et le BatchStats de q3_lane_batched.hpp.
//   1. Log2Hist : classes log2 gravees (0 -> 0 ; 1 -> 1 ; 2, 3 -> 2 ; 4 -> 3 ;
//      2^16 -> 17 ; UINT64_MAX -> 64), quantiles par classe sur un echantillon
//      grave (rang ceil(q n)), histogramme vide -> classe 0, fusion add_from =
//      histogramme direct ;
//   2. BatchStats des lanes par lots (executeurs hote) : un echantillon par
//      vidage (n == flushes) pour seeds, sites et, en q4, paires ; classe du
//      max exact = plus haute classe non vide ; p50 <= p95 <= classe(max) ;
//      plancher --min-flushes contre le vert-par-vacuite ; a plusieurs fils,
//      n == flushes encore (fusion par ouvrier) ;
//   3. ConcurrencyGauge : T fils entrent dans une portee et se retiennent sur
//      une barriere -> pic == T (plancher T >= 2), retombe a 0 actif apres,
//      RAII sous exception, reset_peak ;
//   4. DeviceExecutorStats : add, kernel_ms (kernels seuls), rest_ms borne.
// Mutants tues (code 4) : `log2hist-class-shift` (classe decalee : fixtures
// 1), `gauge-no-peak` (pic jamais releve : plancher 3). Codes : 0, 2, 3, 4.
#include <barrier>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/gpu/device_stats.hpp"
#include "../src/gpu/q4_lane_batched.hpp"

using namespace mhgp5;

namespace {
u64 g_bad = 0;
void check(bool ok, const char* what) {
  if (!ok) {
    std::printf("desaccord : %s\n", what);
    ++g_bad;
  }
}
int top_class(const Log2Hist& h) {
  for (int k = 64; k >= 0; --k)
    if (h.count[k]) return k;
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  int n = 400, threads = 4;
  unsigned gauge_threads = 4;
  u64 min_flushes = 20;
  std::string inject;
  BatchLimits lim;
  lim.seeds = 2000;  // petits lots : beaucoup de vidages a n = 400
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--threads=", 0) == 0) threads = std::atoi(arg.c_str() + 10);
    else if (arg.rfind("--gauge-threads=", 0) == 0) gauge_threads = (unsigned)std::atoi(arg.c_str() + 16);
    else if (arg.rfind("--min-flushes=", 0) == 0) min_flushes = (u64)std::atoll(arg.c_str() + 14);
    else if (arg.rfind("--seeds-per-launch=", 0) == 0) {
      const long long v = std::atoll(arg.c_str() + 19);
      if (v < 1) return 2;
      lim.seeds = (size_t)v;
    } else if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  if (n < 2 || threads < 1 || gauge_threads < 2) return 2;  // plancher : le pic exige au moins deux fils
  const bool mutant = mutants_any();

  // 1. Log2Hist : fixtures gravees.
  {
    const u64 v[] = {0, 1, 2, 3, 4, 5, 7, 8, (u64)1 << 16, ((u64)1 << 16) - 1, (u64)1 << 63, UINT64_MAX};
    const int k[] = {0, 1, 2, 2, 3, 3, 3, 4, 17, 16, 64, 64};
    for (size_t i = 0; i < sizeof(v) / sizeof(v[0]); ++i) check(Log2Hist::class_of(v[i]) == k[i], "classe log2 gravee");
    Log2Hist h;
    check(h.quantile_class(0.5) == 0 && h.quantile_class(0.95) == 0, "histogramme vide -> classe 0");
    // Echantillon grave : {0, 1, 2, 3, 4, 8, 16, 32, 64, 1000} (n = 10) ->
    // classes {0, 1, 2, 2, 3, 4, 5, 6, 7, 10} ; p50 = rang 5 -> classe 3 ;
    // p95 = rang 10 -> classe 10 ; p10 = rang 1 -> classe 0.
    const u64 sample[] = {1000, 0, 64, 1, 32, 2, 16, 3, 8, 4};
    for (const u64 x : sample) h.add(x);
    check(h.n == 10, "n de l'echantillon");
    check(h.quantile_class(0.50) == 3, "p50 grave (rang 5 -> classe 3)");
    check(h.quantile_class(0.95) == 10, "p95 grave (rang 10 -> classe 10)");
    check(h.quantile_class(0.10) == 0, "p10 grave (rang 1 -> classe 0)");
    check(h.quantile_class(1.0) == 10, "p100 = classe du max");
    check(top_class(h) == Log2Hist::class_of(1000), "classe du max = plus haute classe non vide");
    // Fusion : deux moities fusionnees == histogramme direct.
    Log2Hist a, b, direct;
    for (size_t i = 0; i < 10; ++i) { (i % 2 ? a : b).add(sample[i]); direct.add(sample[i]); }
    a.add_from(b);
    bool same = a.n == direct.n;
    for (int c = 0; c < 65; ++c) same = same && a.count[c] == direct.count[c];
    check(same, "add_from == histogramme direct");
  }

  // 2. BatchStats des lanes par lots (executeurs hote).
  {
    const int coord = cloud_family_default_coord(CloudFamily::kUniform, n);
    const CloudIndex ix = build_cloud_index(make_family_input(CloudFamily::kUniform, n, coord, 3));
    if (!ix.valid || ix.has_duplicate_positions()) return 2;
    for (const int t : {1, threads}) {
      GenerateOptions opt;
      opt.threads = t;
      std::vector<BallCandidate> out3, out4;
      GenerateStats s3, s4;
      BatchStats b3, b4;
      generate_q3_batched(ix, opt, &out3, &s3, lim, &b3);
      generate_q4_batched(ix, opt, &out4, &s4, lim, &b4);
      std::printf("lots fils=%d q3: vidages=%llu seeds p50<2^%d p95<2^%d max=%llu sites p50<2^%d p95<2^%d max=%llu ; q4: vidages=%llu seeds p50<2^%d p95<2^%d max=%llu paires p50<2^%d p95<2^%d max=%llu\n",
                  t, (unsigned long long)b3.flushes, b3.lot_seeds.quantile_class(0.5), b3.lot_seeds.quantile_class(0.95),
                  (unsigned long long)b3.max_lot_seeds, b3.lot_sites.quantile_class(0.5), b3.lot_sites.quantile_class(0.95),
                  (unsigned long long)b3.max_lot_sites, (unsigned long long)b4.flushes, b4.lot_seeds.quantile_class(0.5),
                  b4.lot_seeds.quantile_class(0.95), (unsigned long long)b4.max_lot_seeds, b4.lot_pairs.quantile_class(0.5),
                  b4.lot_pairs.quantile_class(0.95), (unsigned long long)b4.max_lot_pairs);
      if (b3.flushes < min_flushes || b4.flushes < min_flushes) {
        std::printf("PLANCHER : vidages q3=%llu q4=%llu < %llu\n", (unsigned long long)b3.flushes, (unsigned long long)b4.flushes,
                    (unsigned long long)min_flushes);
        return 3;
      }
      for (const BatchStats* bs : {&b3, &b4}) {
        check(bs->lot_seeds.n == bs->flushes && bs->lot_sites.n == bs->flushes, "un echantillon seeds/sites par vidage");
        check(top_class(bs->lot_seeds) == Log2Hist::class_of(bs->max_lot_seeds), "classe du max des seeds");
        check(top_class(bs->lot_sites) == Log2Hist::class_of(bs->max_lot_sites), "classe du max des sites");
        check(bs->lot_seeds.quantile_class(0.5) <= bs->lot_seeds.quantile_class(0.95), "p50 <= p95 (seeds)");
        check(bs->lot_seeds.quantile_class(0.95) <= top_class(bs->lot_seeds), "p95 <= classe du max (seeds)");
        check(bs->max_lot_seeds <= (u64)lim.seeds, "borne dure des lots");
      }
      check(b4.lot_pairs.n == b4.flushes, "un echantillon de paires par vidage (q4)");
      check(top_class(b4.lot_pairs) == Log2Hist::class_of(b4.max_lot_pairs), "classe du max des paires");
      check(b3.lot_pairs.n == 0, "q3 : aucune paire");
    }
  }

  // 3. ConcurrencyGauge : pic sous T fils retenus par une barriere.
  {
    gpu::ConcurrencyGauge g;
    g.reset_peak();
    std::barrier<> bar((std::ptrdiff_t)gauge_threads);
    std::vector<std::thread> team;
    for (unsigned t = 0; t < gauge_threads; ++t)
      team.emplace_back([&] {
        const gpu::ConcurrencyGauge::Scope s(g);
        bar.arrive_and_wait();  // tous dedans en meme temps
      });
    for (std::thread& t : team) t.join();
    const u32 peak = g.read_peak();
    std::printf("gauge fils=%u pic=%u actifs_apres=%u\n", gauge_threads, peak, g.active.load());
    if (peak < 2) {
      std::printf("PLANCHER : pic de scan() simultanes %u < 2 (vert-par-vacuite)\n", peak);
      return mutant ? 4 : 3;
    }
    check(peak == gauge_threads, "pic == nombre de fils retenus");
    check(g.active.load() == 0, "aucun actif apres la jointure");
    // RAII sous exception.
    try {
      const gpu::ConcurrencyGauge::Scope s(g);
      throw std::runtime_error("scan simule");
    } catch (const std::runtime_error&) {
    }
    check(g.active.load() == 0, "portee relachee sur exception");
    g.reset_peak();
    check(g.read_peak() == 0, "reset_peak sans actif -> 0");
  }

  // 4. DeviceExecutorStats.
  {
    gpu::DeviceExecutorStats a, b;
    a.k1_ms = 1; a.k2_ms = 2; a.k3_ms = 4; a.executor_ms_sum = 100; a.reserve_ms = 10; a.issue_ms = 5; a.wait_ms = 50;
    a.host1_ms = 1; a.host2_ms = 2; a.host3_ms = 3; a.h2d_bytes = 7; a.lots = 1; a.launches = 3; a.peak_concurrent = 2;
    b.k1_ms = 1; b.executor_ms_sum = 1; b.wait_ms = 2; b.peak_concurrent = 5; b.d2h_bytes = 9; b.lots = 1; b.launches = 1;
    check(a.kernel_ms() == 7, "kernel_ms = k1 + k2 + k3");
    check(a.rest_ms() == 29, "rest_ms = executor - (reserve + enfilement + attente + hote1..3)");
    check(b.rest_ms() == 0, "rest_ms borne a zero");  // 1 - 2 < 0 -> 0 (horloges croisees), jamais negatif
    a.add(b);
    check(a.kernel_ms() == 8 && a.executor_ms_sum == 101 && a.h2d_bytes == 7 && a.d2h_bytes == 9 && a.lots == 2 && a.launches == 4,
          "add : sommes");
    check(a.peak_concurrent == 5, "add : pic = max");
  }

  std::printf("gpu_instrument desaccords=%llu mutant=%d\n", (unsigned long long)g_bad, mutant ? 1 : 0);
  if (g_bad) return mutant ? 4 : 1;
  if (mutant) {
    std::printf("MUTANT NON TUE\n");
    return 3;
  }
  std::printf("gpu_instrument OK\n");
  return 0;
}
