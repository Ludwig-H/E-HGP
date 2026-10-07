// Port explicite du principe FlatIndex/hash_sites de tower.cpp R2, commit 865f5e64ddd08bedf6ab8f94e8bb94812e380e79.
// Source SHA256 a23ed15546195ed75044b7d8e9ea7920c7079d3dfc1f3b23ee5851dcfe07671e.
// Difference : table sequentielle de BallIdx sans tag, charge <=1/2, sondages bornes et budget explicite.
#include "tower/full_domain.hpp"

#include <algorithm>
#include <atomic>

#include "sched/sched.hpp"

namespace mhgp11 {
namespace {

constexpr u64 lookup_capacity(u32 balls) noexcept {
  if (balls == 0) return 0;
  const u64 target = u64{balls} * 2;
  u64 capacity = 1;
  while (capacity < target) capacity *= 2;
  return capacity;
}

// B<kNone dans Catalogue. Meme B=kNone tient : 2B<2^33, C<=2^33, 4C<=2^35 ; aucun produit/shift ne deborde.
static_assert(sizeof(BallIdx) == 4 && lookup_capacity(kNone) == (u64{1} << 33));
static_assert((u64{1} << 33) <= Buffer<BallIdx>::kMaxCount);

u64 hash_support(const std::array<SiteIdx, 4>& key) noexcept {
  constexpr std::array<u64, 4> factors{0x9E3779B97F4A7C15ull, 0xC2B2AE3D27D4EB4Full,
                                      0x165667B19E3779F9ull, 0xD6E8FEB86659FD93ull};
  u64 hash = 4;
  // Arithmetique modulaire NON signee intentionnelle : le hash adresse, seule l'egalite des IDs decide.
  for (unsigned i = 0; i < 4; ++i) hash += (u64{idx(key[i])} + 1) * factors[i];
  hash ^= hash >> 33;
  hash *= 0xff51afd7ed558ccdull;
  hash ^= hash >> 33;
  hash *= 0xc4ceb9fe1a85ec53ull;
  return hash ^ (hash >> 33);
}

Outcome insert(const Catalogue& catalogue, Buffer<BallIdx>& slots, BallIdx ball) noexcept {
  const auto& key = catalogue.balls_data()[idx(ball)].support;
  const u64 mask = slots.size() - 1;
  u64 position = hash_support(key) & mask;
  for (u64 probe = 0; probe < slots.size(); ++probe) {
    const auto previous = slots[position];
    if (idx(previous) == kNone) {
      slots[position] = ball;
      return {};
    }
    // Deux entrees du catalogue ne peuvent avoir le meme S* global ; ne pas les dedupliquer en silence.
    if (catalogue.balls_data()[idx(previous)].support == key) return fail(Reason::catalogue_invariant);
    position = (position + 1) & mask;
  }
  return fail(Reason::catalogue_invariant);  // Impossible avec B<=C/2 et un catalogue sans doublon.
}

// Meme table que insert, remplie par plusieurs workers : la case vide est prise par CAS. Les cles sont uniques ;
// la disposition des sondages peut dependre de l'ordonnancement, jamais la reponse d'une recherche.
Outcome insert_shared(const Catalogue& catalogue, std::span<BallIdx> slots, BallIdx ball) noexcept {
  const auto& key = catalogue.balls_data()[idx(ball)].support;
  const u64 mask = slots.size() - 1;
  u64 position = hash_support(key) & mask;
  for (u64 probe = 0; probe < slots.size(); ++probe) {
    std::atomic_ref<BallIdx> slot(slots[position]);
    BallIdx current = slot.load(std::memory_order_relaxed);
    if (idx(current) == kNone && slot.compare_exchange_strong(current, ball, std::memory_order_relaxed)) return {};
    // current est l'occupant (rechargement sur echec) ; son support est immuable pendant la construction.
    if (idx(current) != kNone && catalogue.balls_data()[idx(current)].support == key)
      return fail(Reason::catalogue_invariant);
    position = (position + 1) & mask;
  }
  return fail(Reason::catalogue_invariant);
}

struct SharedFill {
  const Catalogue& catalogue;
  std::span<BallIdx> slots;
  static Outcome clear(void* context, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<SharedFill*>(context);
    for (u64 i = begin; i < end; ++i) self.slots[i] = make_id<BallIdx>(kNone);
    return {};
  }
  static Outcome insert(void* context, u64 begin, u64 end, u32) noexcept {
    auto& self = *static_cast<SharedFill*>(context);
    for (u64 b = begin; b < end; ++b) MHGP11_TRY(insert_shared(self.catalogue, self.slots, make_id<BallIdx>(b)));
    return {};
  }
};

}  // namespace

std::optional<BallIdx> FullDomain::find_support(const std::array<SiteIdx, 4>& key) const noexcept {
  if (slots_.empty()) return std::nullopt;
  const u64 mask = slots_.size() - 1;
  u64 position = hash_support(key) & mask;
  for (u64 probe = 0; probe < slots_.size(); ++probe) {
    const auto ball = slots_[position];
    if (idx(ball) == kNone) return std::nullopt;
    if (catalogue_.balls_data()[idx(ball)].support == key) return ball;
    position = (position + 1) & mask;
  }
  return std::nullopt;  // La table possedee garde au moins C/2 cases vides.
}

Result<FullDomain> prepare_full_domain(GlobalIndex&& index, const CatalogueParams& params,
                                      MemoryBudget& budget) noexcept {
  auto made = build_catalogue(index.cloud(), params, budget);
  if (!made.ok()) return made.outcome();
  auto& catalogue = made.value();
  const u64 capacity = lookup_capacity(catalogue.balls());
  MHGP11_TRY(budget.admit(capacity * sizeof(BallIdx)));
  Buffer<BallIdx> slots;
  MHGP11_TRY(slots.allocate(capacity, budget));
  if (capacity != 0) {
    std::fill(slots.begin(), slots.end(), make_id<BallIdx>(kNone));
    for (u32 b = 0; b < catalogue.balls(); ++b)
      MHGP11_TRY(insert(catalogue, slots, make_id<BallIdx>(b)));
  }
  return FullDomain(std::move(index), std::move(catalogue), std::move(slots));
}

Result<FullDomain> prepare_full_domain(GlobalIndex&& index, const CatalogueParams& params,
                                      MemoryBudget& budget, sched::Pool& pool, CatalogueTimings* timings) noexcept {
  CatalogueTimings draft;
  auto made = build_catalogue(index.cloud(), params, budget, pool, timings == nullptr ? nullptr : &draft);
  if (!made.ok()) return made.outcome();
  const Stopwatch lookup;  // diagnostic : table support -> boule (draft.lookup_ns)
  auto& catalogue = made.value();
  const u64 capacity = lookup_capacity(catalogue.balls());
  MHGP11_TRY(budget.admit(capacity * sizeof(BallIdx)));
  Buffer<BallIdx> slots;
  MHGP11_TRY(slots.allocate(capacity, budget));
  if (capacity != 0) {
    // Remplissage par le Pool : la table sequentielle coutait ~30 ms de pilote seul sur G4 (1,3 M boules).
    SharedFill fill{catalogue, slots.span()};
    MHGP11_TRY(pool.parallel_for(capacity, 65536, &fill, SharedFill::clear));
    MHGP11_TRY(pool.parallel_for(catalogue.balls(), 16384, &fill, SharedFill::insert));
  }
  FullDomain result(std::move(index), std::move(catalogue), std::move(slots));
  draft.lookup_ns = lookup.nanoseconds();
  if (timings != nullptr) *timings = draft;
  return result;
}

}  // namespace mhgp11
