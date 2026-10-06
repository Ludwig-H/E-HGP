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

// Emission d'une feuille resolue, sans Level, en rangs locaux (indices dans la liste des sites de la feuille ; une
// feuille du lot a au plus 32 sites) : 8 octets. Sa population (p + m rangs, I puis U, un octet chacun) suit celle
// de l'emission precedente de la meme feuille ; les debuts par feuille viennent des prefixes. L'hote convertit en
// SiteIdx et calcule le Level (single_pass_batch.cpp). Rapatriement trois fois moindre qu'en SiteIdx.
inline constexpr u8 kNoLocal = 0xFF;
struct LeafRecord {
  u8 support[4];       // S* en rangs locaux, kNoLocal au-dela de qmin
  u8 p, m, qmin, pad;  // p, m <= 32
};

// Rang local d'un site de la feuille (recherche lineaire sur au plus 32 sites) ; kNoLocal pour kNoSite.
MHGP11_LEAF_HD u8 local_rank(const u32* sites, u32 m, u32 site) {
  for (u32 i = 0; i < m; ++i)
    if (sites[i] == site) return static_cast<u8>(i);
  return kNoLocal;
}

MHGP11_LEAF_HD void encode(const leaf_device::Ball& ball, const u32* sites, u32 m, LeafRecord& r) {
  for (int i = 0; i < 4; ++i) r.support[i] = local_rank(sites, m, ball.support[i]);
  r.p = static_cast<u8>(ball.p); r.m = static_cast<u8>(ball.m); r.qmin = static_cast<u8>(ball.qmin); r.pad = 0;
}

// Puits d'ecriture de la seconde passe : place exacte donnee par les prefixes de la premiere.
struct FillSink {
  LeafRecord* records = nullptr;
  u8* population = nullptr;
  u64 record_at = 0, population_at = 0;
  const u32* sites = nullptr;  // sites de la feuille
  u32 m = 0;
  // interior_local, shell_local : rangs locaux deja connus de la feuille (aucune recherche ; local_rank faisait 7 % du
  // comptage GPU, profil coop2). Les rangs sont < 32 : u8 exact.
  MHGP11_LEAF_HD void emit(const leaf_device::Ball& ball, const u32*, const u32*, const u32* interior_local,
                           const u32* shell_local) {
    encode(ball, sites, m, records[record_at++]);
    for (u32 i = 0; i < ball.p; ++i) population[population_at++] = static_cast<u8>(interior_local[i]);
    for (u32 i = 0; i < ball.m; ++i) population[population_at++] = static_cast<u8>(shell_local[i]);
  }
};

// Case de chaque feuille au comptage : ses emissions y sont rangees tant qu'elles tiennent (K = 5 : p999 de 72 boules
// et 318 incidences par feuille ; K = 10, feuilles de 24 : p99 de 115 boules et 913 incidences ; la moitie des
// feuilles n'emet rien), puis copiees a leur place par copy_scratch. 2 Kio par feuille. Memes cases sur les deux
// executeurs : l'hote valide la logique du GPU.
//
// Reservoir chaine (6 octobre 2026) : une feuille qui deborde de sa case continue dans des blocs du reservoir, de la
// taille d'une case, pris par un curseur atomique et chaines bloc a bloc (next_*) ; enregistrements et population ont
// chacun leur chaine. Seules les feuilles qui debordent aussi du reservoir (curseur au-dela de spare) sont rejouees
// par la seconde passe. Sans reservoir (spare = 0, voie du 5 octobre), toute feuille qui deborde est rejouee. L'ordre
// des blocs pris depend des fils, jamais les sorties : la copie suit la chaine de chaque feuille et ecrit aux places
// fixees par les prefixes. Session G4 j2memo : la seconde passe (14 feuilles rejouees a K5, 4 196 a K10) valait 13 et
// 77 ms, latence de feuilles lourdes sur un fil.
inline constexpr u32 kScratchRecords = 128, kScratchPopulation = 1024;
inline constexpr u32 kNoChunk = 0xFFFFFFFFu;

// Blocs des cases : le bloc j (j < count) est la case de la feuille j ; les blocs count .. count + spare - 1 forment
// le reservoir. cursor[0] et cursor[1] comptent les blocs pris (enregistrements, population), au-dela de spare un
// bloc est refuse. count + spare < 2^32 (garde des executeurs).
struct ScratchArena {
  LeafRecord* records = nullptr;  // (count + spare) * kScratchRecords
  u8* population = nullptr;       // (count + spare) * kScratchPopulation
  u32* next_record = nullptr;     // count + spare : bloc suivant de la chaine d'enregistrements
  u32* next_population = nullptr;
  u32* cursor = nullptr;          // 2 compteurs, nuls au depart
  u32 count = 0, spare = 0;
};

// Bloc suivant du reservoir pour le genre kind (0 : enregistrements, 1 : population) ; kNoChunk s'il est epuise.
// Atomique : les fils de l'appareil et les ouvriers de l'hote prennent des blocs distincts.
MHGP11_LEAF_HD u32 claim_chunk(const ScratchArena& arena, u32 kind) {
  if (arena.spare == 0) return kNoChunk;
#if defined(__CUDA_ARCH__)
  const u32 taken = atomicAdd(&arena.cursor[kind], 1u);
#else
  const u32 taken = __atomic_fetch_add(&arena.cursor[kind], 1u, __ATOMIC_RELAXED);
#endif
  return taken < arena.spare ? arena.count + taken : kNoChunk;
}

#if defined(__CUDA_ARCH__)
#define MHGP11_LEAF_COLD __noinline__
#else
#define MHGP11_LEAF_COLD __attribute__((noinline))
#endif

// Puits du comptage : compte tout et range les emissions dans la case, puis dans les blocs du reservoir tant qu'il en
// reste, dans l'ordre d'emission (celui de FillSink). fits faux : la feuille sera rejouee. Chemin chaud : la boule
// tient dans les blocs courants, ecriture directe comme la case du 5 octobre. Chemin froid (changement de bloc, rare)
// hors ligne : integre, il ajoutait des points de reconvergence au parcours et ralentissait le comptage GPU de 30 a
// 43 ms a K5 (session G4 reservoir2).
struct ScratchSink {
  const ScratchArena* arena = nullptr;
  const u32* sites = nullptr;
  u32 m = 0;
  LeafRecord* record_at = nullptr;  // prochaine place d'enregistrement dans le bloc courant
  u8* population_at = nullptr;      // prochaine place de population dans le bloc courant
  u32 record_room = kScratchRecords, population_room = kScratchPopulation;  // places restantes des blocs courants
  u32 record_chunk = 0, population_chunk = 0;                               // blocs courants
  u64 balls = 0, incidences = 0;
  bool fits = true;

  MHGP11_LEAF_HD ScratchSink(const ScratchArena& a, u32 leaf, const u32* leaf_sites, u32 leaf_m)
      : arena(&a), sites(leaf_sites), m(leaf_m), record_at(a.records + u64(leaf) * kScratchRecords),
        population_at(a.population + u64(leaf) * kScratchPopulation), record_chunk(leaf), population_chunk(leaf) {}

  // interior_local, shell_local : rangs locaux deja connus de la feuille (aucune recherche). Les rangs sont < 32 : u8.
  MHGP11_LEAF_HD void emit(const leaf_device::Ball& ball, const u32*, const u32*, const u32* interior_local,
                           const u32* shell_local) {
    const u32 need = ball.p + ball.m;  // <= 2 * kMaxSites
    ++balls;
    incidences += need;
    if (fits && record_room != 0 && need <= population_room) {  // chemin chaud
      encode(ball, sites, m, *record_at++);
      --record_room;
      for (u32 i = 0; i < ball.p; ++i) population_at[i] = static_cast<u8>(interior_local[i]);
      for (u32 i = 0; i < ball.m; ++i) population_at[ball.p + i] = static_cast<u8>(shell_local[i]);
      population_at += need;
      population_room -= need;
    } else if (fits) {
      fits = cold(ball, interior_local, shell_local);
    }
  }

  // Chemin froid : blocs chaines du reservoir pour l'enregistrement et pour la population ; faux s'il est epuise.
  MHGP11_LEAF_HD MHGP11_LEAF_COLD bool cold(const leaf_device::Ball& ball, const u32* interior_local,
                                            const u32* shell_local) {
    if (record_room == 0) {
      const u32 taken = claim_chunk(*arena, 0);
      if (taken == kNoChunk) return false;
      arena->next_record[record_chunk] = taken;
      record_chunk = taken;
      record_at = arena->records + u64(taken) * kScratchRecords;
      record_room = kScratchRecords;
    }
    encode(ball, sites, m, *record_at++);
    --record_room;
    for (u32 i = 0; i < u32(ball.p) + ball.m; ++i) {
      if (population_room == 0) {
        const u32 taken = claim_chunk(*arena, 1);
        if (taken == kNoChunk) return false;
        arena->next_population[population_chunk] = taken;
        population_chunk = taken;
        population_at = arena->population + u64(taken) * kScratchPopulation;
        population_room = kScratchPopulation;
      }
      *population_at++ = static_cast<u8>(i < ball.p ? interior_local[i] : shell_local[i - ball.p]);
      --population_room;
    }
    return true;
  }
};

// Copie la chaine de la feuille j (n enregistrements, k rangs) a ses places.
MHGP11_LEAF_HD void copy_scratch(const ScratchArena& arena, u32 j, u64 n, u64 k, u64 record_begin,
                                 u64 population_begin, LeafRecord* records, u8* population) {
  u32 chunk = j;
  for (u64 i = 0; i < n; ++i) {
    if (i != 0 && i % kScratchRecords == 0) chunk = arena.next_record[chunk];
    records[record_begin + i] = arena.records[u64(chunk) * kScratchRecords + i % kScratchRecords];
  }
  chunk = j;
  for (u64 i = 0; i < k; ++i) {
    if (i != 0 && i % kScratchPopulation == 0) chunk = arena.next_population[chunk];
    population[population_begin + i] = arena.population[u64(chunk) * kScratchPopulation + i % kScratchPopulation];
  }
}

// Blocs du reservoir pour un lot de count feuilles : un huitieme des feuilles, au moins 64 (aucun sans reservoir).
// Majorant des blocs pris, jamais une decision : au-dela, les feuilles sont rejouees.
inline u64 spare_chunks(u64 count, bool reservoir) noexcept { return reservoir ? count / 8 + 64 : 0; }

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
  bool reservoir = true;  // cases chainees au comptage (faux : voie du 5 octobre, toute feuille qui deborde est rejouee)
};

// Chronos et volumes d'un lot (diagnostic, hors ledger). Les champs device_* ne sont remplis que par CUDA.
struct LeafBatchTimings {
  u64 jobs = 0, unresolved = 0, records = 0, population = 0;
  u64 count_ns = 0, scan_ns = 0, fill_ns = 0, total_ns = 0;
  u64 device_init_ns = 0, upload_ns = 0, download_ns = 0, device_bytes = 0;
  u64 prefetch_ns = 0;  // duree de l'ouverture anticipee du contexte (fil d'arriere-plan), 0 sans elle
  u64 fill_jobs = 0;    // feuilles rejouees par la seconde passe : celles qui emettent et debordent de leur case
  u64 copied_jobs = 0;  // feuilles qui emettent et tiennent dans leurs blocs (case et reservoir) : copiees sans rejeu
  u64 spare_record_chunks = 0, spare_population_chunks = 0;  // blocs du reservoir pris (au plus spare) ; 0 sans lui
  u64 device_pool_used_high = 0, device_pool_reserved_high = 0;  // pics physiques du pool CUDA pendant le lot
  // Executeur partage : feuilles confiees au Pool de l'hote, durees des deux cotes (zeros hors partage).
  u64 split_host_jobs = 0, split_host_ns = 0, split_device_ns = 0;
};

// Resultat d'un executeur : statut par feuille, compteurs des seules feuilles resolues, emissions.
struct LeafBatchResult {
  Buffer<u8> status;  // leaf_device::kOk ou kUnresolved, par feuille du lot
  Buffer<u64> record_begin, population_begin;  // debuts par feuille (prefixes exclusifs)
  Buffer<LeafRecord> records;
  Buffer<u8> population;  // rangs locaux
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
// Executeur d'un sous-lot (run_leaf_batch_host ou run_leaf_batch_cuda).
using LeafBatchRunner = Outcome (*)(const LeafBatchView&, sched::Pool&, MemoryBudget&, LeafBatchResult&) noexcept;
// Partage deterministe : to_host[j] vaut 1 pour les feuilles les plus lourdes (travail estime m^3, par paliers de m
// decroissant, premieres feuilles du lot dans le palier de bascule) jusqu'a host_permille du travail estime total.
[[nodiscard]] Outcome select_host_leaves(std::span<const LeafJob> jobs, u32 host_permille,
                                         std::span<u8> to_host) noexcept;
// Executeur partage (leaf_batch_split.cpp) : feuilles choisies par select_host_leaves sur le Pool de l'hote, les autres
// sur `device` dans un fil a part (avec un petit Pool a lui), en meme temps ; fusion dans l'ordre du lot. Meme resultat
// que tout executeur unique : chaque feuille est traitee par le meme code et ses emissions n'en dependent pas.
[[nodiscard]] Outcome run_leaf_batch_split(const LeafBatchView& view, sched::Pool& pool, MemoryBudget& budget,
                                           u32 host_permille, LeafBatchRunner device, LeafBatchResult& result) noexcept;
// Ouverture anticipee du contexte CUDA (fil d'arriere-plan, une fois par processus) ; sans CUDA, rien.
void prefetch_cuda_context() noexcept;

}  // namespace mhgp11::catalogue_detail
