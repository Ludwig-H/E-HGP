// Assemblage optionnel apres tri exact : positions et blocs independants du nombre de workers.
#pragma once

#include "catalogue/internal.hpp"
#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {

inline constexpr u64 kAssemblyGrain = 4096;
inline constexpr u64 kAssemblyMaxBlocks = 1024;
// Exigent count<kNone, comme le catalogue. Fonctions pures pour les limites sans allouer un faux grand span.
inline constexpr u64 assembly_grain(u64 count) noexcept {
  return std::max(kAssemblyGrain, (count + kAssemblyMaxBlocks - 1) / kAssemblyMaxBlocks);
}
inline constexpr u64 assembly_blocks(u64 count) noexcept {
  return (count + assembly_grain(count) - 1) / assembly_grain(count);
}

struct AssemblyBlock {
  u64 transitions = 0, incidences = 0, rank_begin = 0, population_begin = 0;
};
static_assert(sizeof(AssemblyBlock) == 32);

// Prive au module : permutation vide (ordre physique) ou bijection certifiee par sort_indices.
// Les emissions/populations restent vivantes et immobiles. scan ecrit seulement ball.rank : le halo ne lit
// QUE level/support. Aucune utilisation simultanee du plan, aucun changement des emissions entre scan/fill.
// Les rangs locaux sont du scratch prive, jamais publie sur refus. Un bloc continu commence au rang local 0.
class AssemblyPlan {
 public:
  AssemblyPlan(const AssemblyPlan&) = delete;
  AssemblyPlan& operator=(const AssemblyPlan&) = delete;
  AssemblyPlan(AssemblyPlan&&) noexcept = default;
  AssemblyPlan& operator=(AssemblyPlan&&) = delete;

  static Result<AssemblyPlan> make(std::span<Emission> records, std::span<const u32> permutation,
                                   u64 population_size, MemoryBudget& budget) noexcept;
  Outcome scan(sched::Pool* pool) noexcept;
  Outcome fill(std::span<const SiteIdx> population, std::span<CatalogueBall> balls,
               std::span<num::Level> levels, std::span<u64> offsets, std::span<SiteIdx> values,
               sched::Pool* pool) const noexcept;
  u64 levels() const noexcept { return levels_; }
  u64 blocks() const noexcept { return blocks_.size(); }
  u64 grain() const noexcept { return grain_; }

 private:
  AssemblyPlan(std::span<Emission> records, std::span<const u32> permutation, u64 population_size) noexcept
      : records_(records), permutation_(permutation), population_size_(population_size) {}
  Emission& record(u64 i) const noexcept { return records_[permutation_.empty() ? i : permutation_[i]]; }
  Outcome scan_block(u64 ordinal) noexcept;
  static Outcome scan_body(void* context, u64 begin, u64 end, u32 worker) noexcept;
  struct Filling;
  std::span<Emission> records_;
  std::span<const u32> permutation_;
  u64 population_size_ = 0, grain_ = kAssemblyGrain, levels_ = 1;
  bool scanned_ = false;
  Buffer<AssemblyBlock> blocks_;
};

}  // namespace mhgp11::catalogue_detail
