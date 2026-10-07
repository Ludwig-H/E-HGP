// Table de populations de l'ordre k (LEM-POP ; port du principe de src/tower/population_lookup.cpp de la v11 gelee,
// ac081a06f) : si une k-partie F est EXACTEMENT la population I u U d'une boule b de Cat_K (p + m = k), alors b
// contient F et S* est dans U, donc la plus petite boule de F est b, et la cellule (b, k) est une naissance (t = m) :
// pas terminal. Ce qui change : une table par ordre, la valeur est l'indice de naissance de l'ordre, et les lignes ne
// sont plus copiees (la v11 gardait K + 3 mots par boule) : une case porte une etiquette de 32 bits et la naissance,
// l'egalite est verifiee sur les SiteIdx contre la CSR du catalogue (I puis U, fusionnes). Construction sequentielle
// dans l'ordre des naissances : disposition des cases deterministe. Capacite : plus petite puissance de deux >= 2E.
#include "tower/internal.hpp"

namespace mhgp12::tower_detail {
namespace {

// Adressage seulement (arithmetique modulaire non signee voulue) : toute reponse exige l'egalite des sites.
// Meme fonction que hash_sites de la v11.
template <class Site>
u64 hash_of(u32 count, Site site) noexcept {
  u64 hash = 0x9E3779B97F4A7C15ull * (u64{count} + 1);
  for (u32 i = 0; i < count; ++i) hash = (hash ^ (u64{site(i)} + 0x632BE59BD9B4E019ull)) * 0xff51afd7ed558ccdull;
  hash ^= hash >> 33;
  hash *= 0xc4ceb9fe1a85ec53ull;
  return hash ^ (hash >> 33);
}

bool exact_population(const CatalogueBall& data, u32 k) noexcept { return u64{data.p} + data.m == k; }

// Population triee de la boule b (fusion de I et U, croissants et disjoints) dans out ; rend son cardinal.
u32 merged_population(const Catalogue& catalogue, u32 b, std::array<u32, kMaxPart>& out) noexcept {
  const auto inner = catalogue.interior(make_id<BallIdx>(b)), shell = catalogue.shell(make_id<BallIdx>(b));
  std::size_t i = 0, j = 0;
  u32 n = 0;
  while ((i < inner.size() || j < shell.size()) && n < kMaxPart)
    out[n++] = idx((j == shell.size() || (i < inner.size() && idx(inner[i]) < idx(shell[j]))) ? inner[i++]
                                                                                                : shell[j++]);
  return n;
}

}  // namespace

u64 population_entries(const Catalogue& catalogue, std::span<const u32> birth_keys, Order k) noexcept {
  u64 count = 0;
  for (const u32 b : birth_keys) count += exact_population(catalogue.balls_data()[b], k) ? 1 : 0;
  return count;
}

u64 PopulationTable::capacity_for(u64 entries) noexcept {
  if (entries == 0) return 0;
  u64 capacity = 1;
  while (capacity < 2 * entries) capacity *= 2;
  return capacity;
}

Outcome PopulationTable::build(const Catalogue& catalogue, std::span<const u32> birth_keys, Order k,
                               MemoryBudget& budget) noexcept {
  if (k < 2 || k > kMaxPart || birth_keys.size() > kMaxOrderBirths) return fail(Reason::tower_invariant, k);
  const u64 capacity = capacity_for(population_entries(catalogue, birth_keys, k));
  MHGP12_TRY(slots_.allocate(capacity, budget));
  for (u64 s = 0; s < capacity; ++s) slots_[s] = 0;
  if (capacity == 0) return {};
  const u64 mask = capacity - 1;
  std::array<u32, kMaxPart> sites{};
  for (u64 birth = 0; birth < birth_keys.size(); ++birth) {
    const auto& data = catalogue.balls_data()[birth_keys[birth]];
    if (!exact_population(data, k)) continue;
    if (merged_population(catalogue, birth_keys[birth], sites) != k) return fail(Reason::tower_invariant, k);
    const u64 hash = hash_of(k, [&](u32 i) noexcept { return sites[i]; });
    u64 at = hash & mask;
    while (slots_[at] != 0) at = (at + 1) & mask;  // charge <= 1/2 : une case vide existe
    slots_[at] = ((hash >> 32) << 32) | (birth + 1);
  }
  return {};
}

std::optional<u32> PopulationTable::find(const Catalogue& catalogue, std::span<const u32> birth_keys,
                                         const Part& f) const noexcept {
  if (slots_.empty()) return std::nullopt;
  const u64 hash = hash_of(f.k, [&](u32 i) noexcept { return f.id[i]; });
  const u64 mask = slots_.size() - 1, tag = hash >> 32;
  for (u64 at = hash & mask;; at = (at + 1) & mask) {
    const u64 slot = slots_[at];
    if (slot == 0) return std::nullopt;
    if ((slot >> 32) != tag) continue;
    const u32 birth = static_cast<u32>(slot & 0xFFFFFFFFull) - 1;
    const u32 b = birth_keys[birth];
    const auto inner = catalogue.interior(make_id<BallIdx>(b)), shell = catalogue.shell(make_id<BallIdx>(b));
    if (inner.size() + shell.size() != f.k) continue;
    // Egalite exacte : F (croissante) est la fusion de I et U (croissants, disjoints).
    std::size_t i = 0, j = 0;
    bool same = true;
    for (u32 n = 0; same && n < f.k; ++n) {
      const bool take_inner = j == shell.size() || (i < inner.size() && idx(inner[i]) < idx(shell[j]));
      same = (take_inner ? idx(inner[i++]) : idx(shell[j++])) == f.id[n];
    }
    if (same) return birth;
  }
}

}  // namespace mhgp12::tower_detail
