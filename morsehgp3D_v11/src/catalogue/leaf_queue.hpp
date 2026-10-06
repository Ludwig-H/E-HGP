// File de feuilles differees d'une tache de la passe unique, et bloc d'emissions du lot (voie lot/GPU).
#pragma once

#include "catalogue/leaf_batch.hpp"
#include "catalogue/single_pass_storage.hpp"

namespace mhgp11::sched { class Pool; }

namespace mhgp11::catalogue_detail {

// Pages budgetees, sans agrandissement : une feuille = une entree LeafJob et ses m SiteIdx (m <= 32).
class TaskLeafQueue final : public LeafQueue {
 public:
  void bind(MemoryBudget& budget) noexcept { budget_ = &budget; }
  Outcome push(std::span<const SiteIdx> sites, const Box& box) noexcept override {
    if (budget_ == nullptr || sites.empty() || sites.size() > leaf_device::kMaxSites)
      return fail(Reason::catalogue_invariant);
    LeafJob job;
    job.begin = sites_.size();
    job.m = static_cast<u32>(sites.size());
    for (int j = 0; j < 3; ++j) { job.lo[j] = box.lo[j]; job.hi[j] = box.hi[j]; }
    std::array<u32, leaf_device::kMaxSites> ids{};
    for (u32 i = 0; i < job.m; ++i) ids[i] = idx(sites[i]);
    auto entry = jobs_.prepare(1, *budget_);
    if (!entry.ok()) return entry.outcome();
    auto values = sites_.prepare(job.m, *budget_);
    if (!values.ok()) return values.outcome();
    jobs_.commit(std::move(entry.value()), {&job, 1});
    sites_.commit(std::move(values.value()), std::span<const u32>(ids.data(), job.m));
    return {};
  }
  u64 jobs() const noexcept { return jobs_.size(); }
  u64 sites() const noexcept { return sites_.size(); }
  // Copie dans le lot contigu ; les debuts sont decales de la place des sites de cette tache dans le lot.
  Outcome copy_to(std::span<LeafJob> jobs, std::span<u32> sites, u64 site_offset) const noexcept {
    MHGP11_TRY(jobs_.copy_to(jobs));
    MHGP11_TRY(sites_.copy_to(sites));
    for (auto& job : jobs) MHGP11_TRY(checked_add(job.begin, site_offset));
    return {};
  }

 private:
  FixedPages<LeafJob, 256> jobs_;
  FixedPages<u32, 4096> sites_;
  MemoryBudget* budget_ = nullptr;
};

// Emissions du lot avec leurs Level (ordre du lot), feuilles non resolues refaites par leaf.cpp, et compteurs de
// toutes les feuilles du lot (resolues sur l'executeur, refaites sur l'hote).
struct BatchBlock {
  Buffer<Emission> records;
  Buffer<SiteIdx> population;
  SinglePassOutput fallback;
  CatalogueLedger ledger;
};

// Vue d'un lot rassemble ; rassemblement des files a places fixes (sur le Pool, ou sequentiel sans Pool).
LeafBatchView batch_view(const Cloud& cloud, const CatalogueParams& params, const Buffer<LeafJob>& jobs,
                         const Buffer<u32>& sites) noexcept;
[[nodiscard]] Outcome gather_leaves(MemoryBudget& budget, sched::Pool* pool,
                                    std::span<const TaskLeafQueue* const> queues, Buffer<LeafJob>& jobs,
                                    Buffer<u32>& sites) noexcept;
// Level, population et repli exact d'un lot deja execute, sur le Pool ; chronos du lot.
[[nodiscard]] Outcome finalize_leaf_batch(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget,
                                          sched::Pool& pool, const LeafBatchView& view, const LeafBatchResult& result,
                                          u64 gather_ns, u64 executor_ns, BatchBlock& out,
                                          CatalogueTimings* timings) noexcept;

// Recouvrement (overlap_leaves, levier L4 du plan GPU du 6 octobre 2026) : un fil dedie execute les feuilles par
// sous-lots PENDANT la passe des taches. Les taches sont rangees dans leur ordre de reclamation (heaviest_first,
// deterministe) et coupees en au plus kOverlapChunks sous-lots fixes ; un sous-lot part des que ses taches sont
// finies. L'executeur (hote ou CUDA) tourne sur un Pool prive d'un fil : le Pool de la Session est occupe par la
// passe. Apres la passe, les sous-lots sont concatenes DANS L'ORDRE DES SOUS-LOTS (jamais dans l'ordre d'achevement),
// puis finalises comme un lot unique (Level, repli exact) sur le Pool. Le catalogue final est trie dans l'ordre
// canonique : memes octets que le lot unique.
inline constexpr u32 kOverlapChunks = 16;
class OverlapLane {
 public:
  OverlapLane() = default;
  OverlapLane(const OverlapLane&) = delete;
  OverlapLane& operator=(const OverlapLane&) = delete;
  ~OverlapLane();
  // queues : files des taches dans l'ordre de reclamation (position -> file). Lance le fil.
  [[nodiscard]] Outcome start(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget,
                              std::span<const TaskLeafQueue* const> queues) noexcept;
  // La tache de position `position` (ordre de reclamation) a fini sa generation, avec succes.
  void task_done(u64 position) noexcept;
  // Arret sans attendre les taches (refus de la passe) ; finish rend alors le refus.
  void abort() noexcept;
  // Joint le fil, concatene les sous-lots et finalise sur le Pool.
  [[nodiscard]] Outcome finish(sched::Pool& pool, BatchBlock& out, CatalogueTimings* timings) noexcept;

 private:
  struct State;
  State* state_ = nullptr;
};

// Traite toutes les feuilles des files, dans l'ordre des taches puis des feuilles. Les sorties rejoignent la
// compaction de la passe unique ; le catalogue final reste trie dans l'ordre canonique.
[[nodiscard]] Outcome process_leaf_batch(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget,
                                         sched::Pool& pool, std::span<const TaskLeafQueue* const> queues,
                                         BatchBlock& out, CatalogueTimings* timings) noexcept;

}  // namespace mhgp11::catalogue_detail
