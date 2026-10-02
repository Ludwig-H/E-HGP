// Port explicite du principe FlatIndex/hash_sites de tower.cpp R2, commit 865f5e64ddd08bedf6ab8f94e8bb94812e380e79.
// Source SHA256 a23ed15546195ed75044b7d8e9ea7920c7079d3dfc1f3b23ee5851dcfe07671e.
// Difference : table sequentielle de BallIdx sans tag, charge <=1/2, sondages bornes et budget explicite.
#include "tower/full_domain.hpp"

#include <algorithm>

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
  FullDomain result(std::move(index), std::move(catalogue), std::move(slots));
  if (timings != nullptr) *timings = draft;
  return result;
}

}  // namespace mhgp11
