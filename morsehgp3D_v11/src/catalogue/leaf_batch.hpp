// Lot de feuilles differees de la passe unique (voie GPU) : les taches mettent en file leurs feuilles admissibles
// (graphe de paires, 1 <= m <= 32), un executeur les traite toutes par leaf_device.hpp en deux passes (compter,
// prefixer, ecrire), puis l'hote calcule les Level depuis les supports et refait par leaf.cpp les feuilles non
// resolues. Deux executeurs : l'hote (Pool, validation de la voie) et CUDA (catalogue_gpu, si construit).
// Disposition deterministe : enregistrements et populations dans l'ordre des feuilles du lot, quel que soit
// l'executeur ; le catalogue final est de toute facon trie dans l'ordre canonique (niveau, S*).
#pragma once

#include "catalogue/internal.hpp"
#include "catalogue/leaf_device.hpp"

namespace mhgp11::sched { class Pool; }

namespace mhgp11::catalogue_detail {

// Feuille differee : sites[begin, begin+m) dans la liste du lot, boite T0 demi-ouverte de la feuille.
struct LeafJob {
  u64 begin = 0;
  u32 m = 0, pad = 0;
  i64 lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0};
};

// Emission d'une feuille resolue, sans Level : population [population_begin, +p+m) du lot, I puis U.
struct LeafRecord {
  u32 support[4];
  u32 p, m, qmin, pad;
  u64 population_begin;
};

// Puits d'ecriture de la seconde passe : place exacte donnee par les prefixes de la premiere.
struct FillSink {
  LeafRecord* records = nullptr;
  u32* population = nullptr;
  u64 record_at = 0, population_at = 0;
  MHGP11_LEAF_HD void emit(const leaf_device::Ball& ball, const u32* interior, const u32* shell) {
    LeafRecord& r = records[record_at++];
    for (int i = 0; i < 4; ++i) r.support[i] = ball.support[i];
    r.p = ball.p; r.m = ball.m; r.qmin = ball.qmin; r.pad = 0;
    r.population_begin = population_at;
    for (u32 i = 0; i < ball.p; ++i) population[population_at++] = interior[i];
    for (u32 i = 0; i < ball.m; ++i) population[population_at++] = shell[i];
  }
};

// Case de chaque feuille au comptage : ses emissions y sont rangees tant qu'elles tiennent (99 % des feuilles LiDAR a
// K = 5 : p99 39 boules et 177 incidences ; la moitie n'emet rien), puis copiees a leur place par copy_scratch ;
// seules les feuilles qui emettent et debordent rejouent leur feuille a l'ecriture. Memes cases sur les deux
// executeurs, si bien que l'hote valide la logique du GPU.
inline constexpr u32 kScratchRecords = 32, kScratchPopulation = 256;

// Puits du comptage : compte tout et range les emissions dans la case tant qu'elles y tiennent, dans l'ordre
// d'emission (celui de FillSink) ; population_begin relatif a la case, recale a la copie.
struct ScratchSink {
  LeafRecord* records = nullptr;  // case de la feuille : kScratchRecords enregistrements
  u32* population = nullptr;      // case de la feuille : kScratchPopulation incidences
  u64 balls = 0, incidences = 0;
  bool fits = true;
  MHGP11_LEAF_HD void emit(const leaf_device::Ball& ball, const u32* interior, const u32* shell) {
    const u64 need = u64(ball.p) + ball.m;
    if (fits && balls < kScratchRecords && incidences + need <= kScratchPopulation) {
      LeafRecord& r = records[balls];
      for (int i = 0; i < 4; ++i) r.support[i] = ball.support[i];
      r.p = ball.p; r.m = ball.m; r.qmin = ball.qmin; r.pad = 0;
      r.population_begin = incidences;
      for (u32 i = 0; i < ball.p; ++i) population[incidences + i] = interior[i];
      for (u32 i = 0; i < ball.m; ++i) population[incidences + ball.p + i] = shell[i];
    } else {
      fits = false;
    }
    ++balls;
    incidences += need;
  }
};

// Copie la case de la feuille j (n enregistrements, k incidences) a ses places, population_begin recale.
MHGP11_LEAF_HD void copy_scratch(u64 j, u64 n, u64 k, u64 record_begin, u64 population_begin,
                                 const LeafRecord* scratch_records, const u32* scratch_population,
                                 LeafRecord* records, u32* population) {
  for (u64 i = 0; i < n; ++i) {
    LeafRecord r = scratch_records[j * kScratchRecords + i];
    r.population_begin += population_begin;
    records[record_begin + i] = r;
  }
  for (u64 i = 0; i < k; ++i) population[population_begin + i] = scratch_population[j * kScratchPopulation + i];
}

// Vue du lot pour un executeur : coordonnees du nuage, feuilles, liste des sites, parametres.
struct LeafBatchView {
  const u32* x = nullptr;
  const u32* y = nullptr;
  const u32* z = nullptr;
  u32 cloud_sites = 0;
  const LeafJob* jobs = nullptr;
  u64 count = 0;
  const u32* sites = nullptr;
  u64 site_count = 0;
  int kmax = 0;
  bool cache = false;
};

// Chronos et volumes d'un lot (diagnostic, hors ledger). Les champs device_* ne sont remplis que par CUDA.
struct LeafBatchTimings {
  u64 jobs = 0, unresolved = 0, records = 0, population = 0;
  u64 count_ns = 0, scan_ns = 0, fill_ns = 0, total_ns = 0;
  u64 device_init_ns = 0, upload_ns = 0, download_ns = 0, device_bytes = 0;
  u64 prefetch_ns = 0;  // duree de l'ouverture anticipee du contexte (fil d'arriere-plan), 0 sans elle
  u64 fill_jobs = 0;    // feuilles rejouees par la seconde passe : celles qui emettent et debordent de leur case
};

// Resultat d'un executeur : statut par feuille, compteurs des seules feuilles resolues, emissions.
struct LeafBatchResult {
  Buffer<u8> status;  // leaf_device::kOk ou kUnresolved, par feuille du lot
  Buffer<LeafRecord> records;
  Buffer<u32> population;
  leaf_device::Counts counts;
  LeafBatchTimings timings;
};

// Executeur hote : meme code que l'appareil, feuilles reparties sur le Pool. Toujours construit.
[[nodiscard]] Outcome run_leaf_batch_host(const LeafBatchView& view, sched::Pool& pool, MemoryBudget& budget,
                                          LeafBatchResult& result) noexcept;
// Executeur CUDA ; sans MHGP11_HAVE_CUDA il refuse (parameter_out_of_range) avant tout calcul.
// Le Pool sert a faire fauter en parallele les pages des tampons hote avant le retour (la copie vers des pages neuves
// plafonnait a 4 Go/s sur G4) ; il est libre pendant l'appel.
[[nodiscard]] Outcome run_leaf_batch_cuda(const LeafBatchView& view, sched::Pool& pool, MemoryBudget& budget,
                                          LeafBatchResult& result) noexcept;
bool cuda_leaf_batch_available() noexcept;
// Ouverture anticipee du contexte CUDA (fil d'arriere-plan, une fois par processus) ; sans CUDA, rien.
void prefetch_cuda_context() noexcept;

}  // namespace mhgp11::catalogue_detail
