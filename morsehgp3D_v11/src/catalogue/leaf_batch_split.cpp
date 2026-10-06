// Executeur partage du lot de feuilles : les plus lourdes sur le Pool de l'hote, les autres sur l'appareil, en meme
// temps, puis fusion dans l'ordre du lot. Diagnostic claudedom1 du 6 octobre 2026 (trames LiDAR reelles, K = 5,
// feuilles de 24) : le lot entier prend 71 a 77 ms sur le GPU et 102 a 128 ms sur le Pool, qui attend pendant que le
// GPU calcule. Partager le travail entre les deux raccourcit le lot sans changer une seule sortie.
#include "catalogue/leaf_batch.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <thread>

#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {
namespace {

// Travail estime d'une feuille : m^3 (les triplets candidats dominent le comptage, rapport D du workflow GPU). Les
// feuilles du lot ont 1 <= m <= 32 : 32 poids distincts, selection lineaire par paliers de m.
inline constexpr u32 kSplitMaxSites = 32;
inline constexpr u32 kSplitAuxWorkers = 4;  // Pool auxiliaire du fil de l'appareil (pages du retour CUDA)

u64 weight(u32 m) noexcept { return u64{m} * m * m; }

void add_counts(leaf_device::Counts& total, const leaf_device::Counts& c) noexcept {
  total.dominance_tests += c.dominance_tests; total.prefixes += c.prefixes; total.judged += c.judged;
  total.census_tests += c.census_tests; total.emitted += c.emitted; total.incidences += c.incidences;
  total.q4_candidates += c.q4_candidates; total.q4_levels += c.q4_levels;
  total.region_pair_tests += c.region_pair_tests; total.region_pair_rejects += c.region_pair_rejects;
  total.region_line_tests += c.region_line_tests; total.region_line_rejects += c.region_line_rejects;
  total.region_line_evaluations += c.region_line_evaluations;
  total.region_line_cache_hits += c.region_line_cache_hits;
  total.region_line_fallbacks += c.region_line_fallbacks;
}

// Fin (exclusive) des emissions de la feuille locale j d'un resultat : debut suivant, ou total.
u64 record_end(const LeafBatchResult& r, u64 j) noexcept {
  return j + 1 < r.record_begin.size() ? r.record_begin[j + 1] : r.records.size();
}
u64 population_end(const LeafBatchResult& r, u64 j) noexcept {
  return j + 1 < r.population_begin.size() ? r.population_begin[j + 1] : r.population.size();
}

// Copie, par feuille du lot, de ses emissions depuis le resultat de son cote vers leur place globale.
struct MergeCopy {
  std::span<const u8> to_host;
  std::span<const u32> local;
  const LeafBatchResult& host;
  const LeafBatchResult& device;
  LeafBatchResult& out;
  static Outcome body(void* context, u64 begin, u64 end, u32) noexcept {
    const auto& self = *static_cast<const MergeCopy*>(context);
    for (u64 j = begin; j < end; ++j) {
      const LeafBatchResult& side = self.to_host[j] ? self.host : self.device;
      const u64 l = self.local[j];
      const u64 records = record_end(side, l) - side.record_begin[l];
      const u64 population = population_end(side, l) - side.population_begin[l];
      if (records != 0)
        std::memcpy(self.out.records.data() + self.out.record_begin[j], side.records.data() + side.record_begin[l],
                    records * sizeof(LeafRecord));
      if (population != 0)
        std::memcpy(self.out.population.data() + self.out.population_begin[j],
                    side.population.data() + side.population_begin[l], population);
    }
    return {};
  }
};

// Fusion des deux cotes dans l'ordre du lot : statut, debuts globaux, emissions copiees a leurs places, compteurs
// sommes ; chronos de l'appareil pour ses etapes et volumes des deux cotes.
Outcome merge_sides(std::span<const u8> to_host, std::span<const u32> local, const LeafBatchResult& host_result,
                    const LeafBatchResult& device_result, sched::Pool& pool, MemoryBudget& budget,
                    LeafBatchResult& result) noexcept {
  const u64 n = to_host.size();
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<u8>(bytes, n));
  MHGP11_TRY(add_bytes<u64>(bytes, 2 * n));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(result.status.allocate(n, budget));
  MHGP11_TRY(result.record_begin.allocate(n, budget));
  MHGP11_TRY(result.population_begin.allocate(n, budget));
  u64 records = 0, population = 0;
  for (u64 j = 0; j < n; ++j) {
    const LeafBatchResult& side = to_host[j] ? host_result : device_result;
    const u64 l = local[j];
    result.status[j] = side.status[l];
    result.record_begin[j] = records;
    result.population_begin[j] = population;
    MHGP11_TRY(checked_add(records, record_end(side, l) - side.record_begin[l]));
    MHGP11_TRY(checked_add(population, population_end(side, l) - side.population_begin[l]));
  }
  if (records != host_result.records.size() + device_result.records.size() ||
      population != host_result.population.size() + device_result.population.size())
    return fail(Reason::catalogue_invariant);
  bytes = 0;
  MHGP11_TRY(add_bytes<LeafRecord>(bytes, records));
  MHGP11_TRY(add_bytes<u8>(bytes, population));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(result.records.allocate(records, budget));
  MHGP11_TRY(result.population.allocate(population, budget));
  MergeCopy copy{to_host, local, host_result, device_result, result};
  if (n != 0) MHGP11_TRY(pool.parallel_for(n, 256, &copy, MergeCopy::body));
  // Compteurs : somme des deux cotes (feuilles resolues seulement, comme chaque executeur).
  result.counts = leaf_device::Counts{};
  add_counts(result.counts, host_result.counts);
  add_counts(result.counts, device_result.counts);
  // Chronos : ceux de l'appareil pour ses etapes, plus le partage ; volumes des deux cotes.
  auto& t = result.timings;
  t = device_result.timings;
  t.jobs = n; t.records = records; t.population = population;
  t.unresolved = host_result.timings.unresolved + device_result.timings.unresolved;
  t.fill_jobs = host_result.timings.fill_jobs + device_result.timings.fill_jobs;
  t.copied_jobs = host_result.timings.copied_jobs + device_result.timings.copied_jobs;
  t.spare_record_chunks = host_result.timings.spare_record_chunks + device_result.timings.spare_record_chunks;
  t.spare_population_chunks =
      host_result.timings.spare_population_chunks + device_result.timings.spare_population_chunks;
  return {};
}

}  // namespace

Outcome select_host_leaves(std::span<const LeafJob> jobs, u32 host_permille, std::span<u8> to_host) noexcept {
  if (host_permille > 1000 || to_host.size() != jobs.size()) return fail(Reason::parameter_out_of_range);
  std::array<u64, kSplitMaxSites + 1> count{}, taken{};
  u64 total = 0;
  for (const LeafJob& job : jobs) {
    if (job.m < 1 || job.m > kSplitMaxSites) return fail(Reason::catalogue_invariant);
    ++count[job.m];
    total += weight(job.m);  // <= 2^15 par feuille, au plus kMaxBatchJobs feuilles : aucun debordement
  }
  // Cible : host_permille du travail estime, prise par paliers de m decroissant ; dans le palier de bascule, les
  // premieres feuilles dans l'ordre du lot. Deterministe : ne depend ni des fils ni des temps.
  u64 target = total / 1000 * host_permille + total % 1000 * host_permille / 1000, acquired = 0;
  for (u32 m = kSplitMaxSites; m >= 1 && acquired < target; --m) {
    const u64 w = weight(m), need = (target - acquired + w - 1) / w;
    taken[m] = std::min(count[m], need);
    acquired += taken[m] * w;
  }
  for (u64 j = 0; j < jobs.size(); ++j) {
    const u32 m = jobs[j].m;
    to_host[j] = taken[m] != 0;
    if (taken[m] != 0) --taken[m];
  }
  return {};
}

Outcome run_leaf_batch_split(const LeafBatchView& view, sched::Pool& pool, MemoryBudget& budget, u32 host_permille,
                             LeafBatchRunner device, LeafBatchResult& result) noexcept {
  if (device == nullptr) return fail(Reason::parameter_out_of_range);
  const Stopwatch total;
  const u64 n = view.count;
  // Partage, puis sous-lots dans l'ordre du lot ; ils pointent la meme liste de sites (job.begin inchange).
  Buffer<u8> to_host;
  Buffer<u32> local;
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<u8>(bytes, n));
  MHGP11_TRY(add_bytes<u32>(bytes, n));
  MHGP11_TRY(add_bytes<LeafJob>(bytes, n));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(to_host.allocate(n, budget));
  MHGP11_TRY(local.allocate(n, budget));
  MHGP11_TRY(select_host_leaves(std::span<const LeafJob>(view.jobs, n), host_permille, to_host.span()));
  u64 host_count = 0;
  for (u64 j = 0; j < n; ++j) host_count += to_host[j];
  Buffer<LeafJob> host_jobs, device_jobs;
  MHGP11_TRY(host_jobs.allocate(host_count, budget));
  MHGP11_TRY(device_jobs.allocate(n - host_count, budget));
  for (u64 j = 0, h = 0, d = 0; j < n; ++j) {
    if (to_host[j]) { local[j] = static_cast<u32>(h); host_jobs[h++] = view.jobs[j]; }
    else { local[j] = static_cast<u32>(d); device_jobs[d++] = view.jobs[j]; }
  }
  LeafBatchView host_view = view, device_view = view;
  host_view.jobs = host_jobs.data(); host_view.count = host_count;
  device_view.jobs = device_jobs.data(); device_view.count = n - host_count;
  LeafBatchResult host_result, device_result;
  Outcome host_outcome, device_outcome;
  u64 host_ns = 0, device_ns = 0;
  // L'appareil dans un fil a part, avec un petit Pool a lui (CUDA y touche les pages de ses tampons de retour) ; le
  // Pool principal porte les feuilles de l'hote. Sans fil disponible, les deux parties s'enchainent sur ce fil.
  auto aux = sched::make_pool({kSplitAuxWorkers});
  if (!aux.ok()) return aux.outcome();
  auto run_device = [&]() noexcept {
    const Stopwatch clock;
    device_outcome = device_view.count == 0 ? Outcome{} : device(device_view, *aux.value(), budget, device_result);
    device_ns = clock.nanoseconds();
  };
  std::thread device_thread;
  try { device_thread = std::thread(run_device); } catch (...) { run_device(); }
  {
    const Stopwatch clock;
    host_outcome = host_count == 0 ? Outcome{} : run_leaf_batch_host(host_view, pool, budget, host_result);
    host_ns = clock.nanoseconds();
  }
  if (device_thread.joinable()) device_thread.join();
  MHGP11_TRY(device_outcome);
  MHGP11_TRY(host_outcome);
  // Un cote vide n'a produit aucun tableau : ses tailles restent nulles, comme ses emissions.
  if ((host_count != 0 && host_result.status.size() != host_count) ||
      (n - host_count != 0 && device_result.status.size() != n - host_count))
    return fail(Reason::catalogue_invariant);
  MHGP11_TRY(merge_sides(to_host.span(), local.span(), host_result, device_result, pool, budget, result));
  auto& t = result.timings;
  t.split_host_jobs = host_count; t.split_host_ns = host_ns; t.split_device_ns = device_ns;
  t.total_ns = total.nanoseconds();
  return {};
}

}  // namespace mhgp11::catalogue_detail
