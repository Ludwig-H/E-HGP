// Auditeur : front scalaire vs front par lots sur des nuages GENERES (familles
// reelles, n en milliers), egalite ordonnee des rectangles, masques, coeurs,
// grand-livre et travail physique. Aucun temps revendique.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../src/cloud/families.hpp"
#include "../src/pipeline/witness_front.hpp"
using namespace mhgp7;
int main(int argc, char** argv) {
  if (argc < 5) return 2;
  const int n = std::atoi(argv[1]); const i64 s = std::atoll(argv[2]); const size_t lot = std::strtoull(argv[3], nullptr, 10);
  const int threads = std::atoi(argv[4]); const std::string fam_s = argc > 5 ? argv[5] : "uniform";
  CloudFamily fam = CloudFamily::kUniform;
  if (fam_s == "scanline_overlap_multiecho") fam = CloudFamily::kScanlineOverlapMultiecho;
  else if (fam_s == "eight_clusters") fam = CloudFamily::kEightClusters;
  else if (fam_s == "terrain") fam = CloudFamily::kTerrain;
  const auto ix = build_cloud_index(make_family_input(fam, n, 65536, 3));
  if (!ix.valid) return 3;
  const u64 h[3] = {10, 9, 8};  // smax=11 : h_q = smax - q + 1
  std::vector<MultiAliveRect> ref, out; GenerateStats a, b; WitnessFrontWork work; u64 gen = 0;
  alive_rectangles_fused(ix, s, h, 7, threads, &ref, &a);
  CpuWitnessBatch cpu(ix, threads);
  alive_rectangles_batched(ix, s, h, 7, &out, &b, lot, gen, &work, cpu);
  bool same = ref.size() == out.size() && a.cap_refus == b.cap_refus;
  size_t first_diff = ref.size();
  for (size_t i = 0; same && i < ref.size(); ++i) {
    const auto& x = ref[i]; const auto& y = out[i];
    if (x.r.a != y.r.a || x.r.b != y.r.b || x.mask != y.mask || x.core[0] != y.core[0] || x.core[1] != y.core[1] || x.core[2] != y.core[2]) { same = false; first_diff = i; }
  }
  bool ledger = true;
  for (unsigned q = 0; q < 3; ++q) ledger = ledger && a.rect_alive[q] == b.rect_alive[q] && a.ledger_emitted_mass[q] == b.ledger_emitted_mass[q] && a.ledger_killed_mass[q] == b.ledger_killed_mass[q];
  const bool work_same = a.rect_visited_fused == b.rect_visited_fused && a.wspd_witness_nodes == b.wspd_witness_nodes && a.wspd_corner_evals == b.wspd_corner_evals && a.wave_peak_tasks == b.wave_peak_tasks && a.alive_peak_rects == b.alive_peak_rects;
  std::printf("{\"family\":\"%s\",\"n\":%d,\"s\":%lld,\"lot\":%zu,\"threads\":%d,\"rows\":%zu,\"rows_batched\":%zu,\"ordered_equal\":%d,\"first_diff\":%zu,\"ledger_equal\":%d,\"work_equal\":%d,\"batches\":%llu,\"requests\":%llu,\"corner_requests\":%llu,\"peak_batch\":%llu,\"witness_nodes\":%llu,\"cap_refus\":%llu}\n",
    fam_s.c_str(), n, (long long)s, lot, threads, ref.size(), out.size(), same, first_diff, ledger, work_same,
    (unsigned long long)work.batches, (unsigned long long)work.requests, (unsigned long long)work.corner_requests, (unsigned long long)work.peak_batch,
    (unsigned long long)a.wspd_witness_nodes, (unsigned long long)a.cap_refus);
  return (same && ledger && work_same) ? 0 : 1;
}
