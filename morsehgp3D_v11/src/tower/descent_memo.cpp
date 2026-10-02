// Memo AVANT MEB : aucune hypothese sur le taux de reutilisation, aucun etat DSU ou NodeIdx memorise.
#include "tower/descent_memo.hpp"
#include <algorithm>

namespace mhgp11::tower_detail {
namespace {
Result<CellTrace> key_of(const FullDomain& domain, std::span<const SiteIdx> part, u32 k) noexcept {
  if (k == 0 || k > domain.catalogue().kmax()) return fail(Reason::kmax_out_of_range);
  if (part.size() != k || k > kMaxMebSites) return fail(Reason::parameter_out_of_range);
  CellTrace key; key.sites.fill(SiteIdx{kNone}); key.arity = static_cast<u8>(k);
  for (u32 i = 0; i < k; ++i) {
    if (idx(part[i]) >= domain.index().cloud().sites()) return fail(Reason::parameter_out_of_range);
    key.sites[i] = part[i];
  }
  std::sort(key.sites.begin(), key.sites.begin() + k);
  for (u32 i = 1; i < k; ++i)
    if (key.sites[i] == key.sites[i - 1]) return fail(Reason::parameter_out_of_range);
  return key;
}
}  // namespace

Result<DescentMemo> DescentMemo::make(const FullDomain& domain, u64 capacity, MemoryBudget& budget) noexcept {
  if (domain.catalogue().kmax() == 0 || domain.index().cloud().sites() == 0 ||
      (capacity != 0 && (capacity & (capacity - 1)) != 0)) return fail(Reason::parameter_out_of_range);
  if (capacity > Buffer<Slot>::kMaxCount) return fail(Reason::tower_capacity);
  MHGP11_TRY(budget.admit(capacity * sizeof(Slot)));
  DescentMemo memo(domain);
  MHGP11_TRY(memo.slots_.allocate(capacity, budget));
  for (u64 i = 0; i < capacity; ++i) memo.slots_[i] = Slot{};  // Buffer non initialise/poison.
  return memo;
}

u64 DescentMemo::bucket(const CellTrace& key) const noexcept {
  u64 hash = key.arity;
  // Debordement u64 volontaire du hash modulo2^64 ; toute collision exige la comparaison entiere.
  for (u32 i = 0; i < key.arity; ++i) hash = (hash ^ idx(key.sites[i])) * 1099511628211ull;
  return hash & (slots_.size() - 1);
}

const DescentMemo::Slot* DescentMemo::lookup(const CellTrace& key, MemoLedger& work) const noexcept {
  const auto& slot = slots_[bucket(key)];
  if (slot.cardinal == key.arity && slot.ids == key.sites) return &slot;
  // Ancien collisions<lookups<=max apres increment controle ; cet increment ne deborde pas.
  if (slot.cardinal != 0) ++work.collisions;
  return nullptr;
}

Result<DescentResult> DescentMemo::publish(const CellTrace& key, const num::Level& initial,
                                         const num::Level& terminal, const BirthSeed& seed,
                                         DescentLedger work) noexcept {
  MHGP11_TRY(cell_add(work.memo.insertions, 1));
  auto& slot = slots_[bucket(key)];
  if (slot.cardinal != 0) MHGP11_TRY(cell_add(work.memo.evictions, 1));
  // Derniere mutation : tous calculs, dates et additions de compteurs ont reussi.
  slot = Slot{key.sites, initial, terminal, seed.site() ? idx(*seed.site()) : kNone,
              seed.ball() ? idx(*seed.ball()) : kNone, key.arity};
  return DescentResult(initial, terminal, seed, work);
}

Result<DescentResult> DescentMemo::resolve(const FullDomain& domain, std::span<const SiteIdx> part,
                                         u32 k, MemoryBudget& budget) noexcept {
  if (!belongs_to(domain)) return fail(Reason::parameter_out_of_range);
  if (slots_.empty()) return descend(domain, part, k, budget);
  auto prepared = key_of(domain, part, k);
  if (!prepared.ok()) return prepared.outcome();
  const auto origin = prepared.value(); auto current = origin;
  std::optional<num::Level> initial, previous;
  DescentLedger work; work.memo.queries = 1;
  for (;;) {
    MHGP11_TRY(cell_add(work.memo.lookups, 1));
    if (const auto* saved = lookup(current, work.memo)) {
      MHGP11_TRY(cell_add(work.memo.hits, 1));
      if (previous && num::compare(saved->initial, *previous) >= 0) return fail(Reason::tower_invariant);
      const BirthSeed seed(saved->site == kNone ? std::nullopt : std::optional{SiteIdx{saved->site}},
                           saved->ball == kNone ? std::nullopt : std::optional{BallIdx{saved->ball}}, saved->cardinal);
      if (!initial) return DescentResult(saved->initial, saved->terminal, seed, work);
      MHGP11_TRY(cell_add(work.memo.suffix_hits, 1));
      return publish(origin, *initial, saved->terminal, seed, work);
    }
    MHGP11_TRY(cell_add(work.memo.misses, 1));
    auto step = descent_step(domain, current.part(), k, budget);
    if (!step.ok()) return step.outcome();
    MHGP11_TRY(add_descent(work, step.value().ledger()));
    if (previous && num::compare(step.value().level(), *previous) >= 0) return fail(Reason::tower_invariant);
    if (!initial) initial = step.value().level();
    if (step.value().seed()) return publish(origin, *initial, step.value().level(), *step.value().seed(), work);
    previous = step.value().level(); current = step.value().next();
  }
}

Result<DescentResult> resolve_descent(const FullDomain& domain, std::span<const SiteIdx> part, u32 k,
                                     MemoryBudget& budget, DescentMemo* memo) noexcept {
  return memo == nullptr ? descend(domain, part, k, budget) : memo->resolve(domain, part, k, budget);
}
}  // namespace mhgp11::tower_detail
