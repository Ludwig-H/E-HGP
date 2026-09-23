// Auditeur C / piste D3 : attribution du travail q3/q4 par longueur d'arete ancre.
// Hors produit. Compteurs globaux atomiques, lus par le harnais apres la chaine.
#pragma once
#include <atomic>
#include <cstdint>
namespace d3probe {
constexpr int kBuckets = 10;
// bornes superieures (mm) de |ab| : 50,100,200,400,800,1600,3200,6400,12800,inf
constexpr std::int64_t kEdgesMm[kBuckets - 1] = {50, 100, 200, 400, 800, 1600, 3200, 6400, 12800};
inline int bucket_of_d2(std::int64_t d2) {
  int j = 0;
  while (j < kBuckets - 1 && d2 > kEdgesMm[j] * kEdgesMm[j]) ++j;
  return j;
}
struct Stats {
  std::atomic<std::uint64_t> edges[kBuckets]{}, witness_rejected[kBuckets]{}, cache_rejected[kBuckets]{},
      core_builds[kBuckets]{}, core_closed[kBuckets]{}, cover_builds[kBuckets]{}, core_sites[kBuckets]{},
      cover_sites[kBuckets]{}, atlas_pt[kBuckets]{}, q3_census_pt[kBuckets]{}, q3_leaf_pt[kBuckets]{},
      q4_sweep_active[kBuckets]{}, witness_node_visits[kBuckets]{}, dead_pt[kBuckets]{}, q3_emitted[kBuckets]{},
      q4_emitted[kBuckets]{}, cpu_ns[kBuckets]{}, q4_seeds[kBuckets]{}, q3_seeds[kBuckets]{};
  // niveau rectangle (avant expansion), rangees par borne inferieure de distance des boites
  std::atomic<std::uint64_t> rect_count[kBuckets]{}, rect_mass[kBuckets]{}, rect_rejected_mass[kBuckets]{},
      rect_cpu_ns[kBuckets]{}, rect_node_visits[kBuckets]{};
  // emissions : [bucket |ab| ancre][bucket rayon r (memes bornes, sur r)]
  std::atomic<std::uint64_t> emit_hist[kBuckets][kBuckets]{};
  // min de 1e6*|ab|/r sur les emissions (borne theorique >= 1,633e6)
  std::atomic<std::uint64_t> min_ratio_e6{~0ull};
};
extern Stats g;
}  // namespace d3probe
