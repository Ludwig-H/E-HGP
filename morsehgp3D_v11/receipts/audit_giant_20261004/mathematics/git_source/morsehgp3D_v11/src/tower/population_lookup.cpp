// Table population -> boule : construction budgetee par blocs (CAS si Pool), lecture seule ensuite.
// Un succes coute une case et une ligne contigue ; seule l'egalite exacte des sites decide.
#include "tower/population_lookup.hpp"

#include <atomic>

#include "sched/sched.hpp"

namespace mhgp11::tower_detail {
namespace {

constexpr u64 kBlocks = 256;
// En-tete d'une ligne : boule, rang de son niveau, naissance liee (kNone avant bind) ; puis les sites.
constexpr u32 kHeader = 3;

// Adressage seulement : arithmetique modulaire NON signee voulue ; toute reponse exige l'egalite des sites.
template <class Site>
u64 hash_of(u32 count, Site site) noexcept {
  u64 hash = 0x9E3779B97F4A7C15ull * (u64{count} + 1);
  for (u32 i = 0; i < count; ++i) hash = (hash ^ (u64{site(i)} + 0x632BE59BD9B4E019ull)) * 0xff51afd7ed558ccdull;
  hash ^= hash >> 33;
  hash *= 0xc4ceb9fe1a85ec53ull;
  return hash ^ (hash >> 33);
}
u64 hash_sites(const u32* sites, u32 count) noexcept {
  return hash_of(count, [sites](u32 i) noexcept { return sites[i]; });
}

bool eligible(const CatalogueBall& data, u64 kmax) noexcept { return u64{data.p} + data.m <= kmax; }

}  // namespace

struct PopulationLookup::Builder {
  const Catalogue& cat;
  u64 kmax, balls, width;
  std::span<u64> slots;
  std::span<u32> rows;
  std::array<u64, kBlocks + 1> first{};  // premiere entree de chaque bloc de boules

  u64 begin(u64 block) const noexcept { return std::min(balls, block * width); }

  Outcome insert(u64 entry) const noexcept {
    const u32 stride = static_cast<u32>(kmax) + kHeader;
    const u32* row = rows.data() + entry * stride;
    u32 count = 0;
    while (count < kmax && row[kHeader + count] != kNone) ++count;
    const u64 hash = hash_sites(row + kHeader, count);
    const u64 value = ((hash >> 32) << 32) | (entry + 1), mask = slots.size() - 1;
    u64 position = hash & mask;
    for (u64 probe = 0; probe < slots.size(); ++probe) {
      std::atomic_ref<u64> slot(slots[position]);
      u64 current = slot.load(std::memory_order_relaxed);
      if (current == 0 && slot.compare_exchange_strong(current, value, std::memory_order_relaxed)) return {};
      position = (position + 1) & mask;  // occupee (ou prise a l'instant) : sondage suivant
    }
    return fail(Reason::tower_invariant);  // Impossible a charge <= 1/2.
  }

  Outcome fill(u64 block) const noexcept {
    const u32 stride = static_cast<u32>(kmax) + kHeader;
    u64 entry = first[block];
    for (u64 b = begin(block); b < begin(block + 1); ++b) {
      const auto& data = cat.balls_data()[b];
      if (!eligible(data, kmax)) continue;
      if (entry >= first[block + 1]) return fail(Reason::tower_invariant);
      u32* row = rows.data() + entry * stride;
      row[0] = static_cast<u32>(b);
      row[1] = idx(data.rank);
      row[2] = kNone;  // naissance liee plus tard par bind
      // I et U sont croissants et disjoints (contrat du Catalogue) : fusion croissante, puis kNone.
      const auto inner = cat.interior(BallIdx{static_cast<u32>(b)}), shell = cat.shell(BallIdx{static_cast<u32>(b)});
      u64 i = 0, j = 0, n = 0;
      while (i < inner.size() || j < shell.size())
        row[kHeader + n++] = idx((j == shell.size() || (i < inner.size() && idx(inner[i]) < idx(shell[j]))) ? inner[i++] : shell[j++]);
      for (; n < kmax; ++n) row[kHeader + n] = kNone;
      MHGP11_TRY(insert(entry++));
    }
    return entry == first[block + 1] ? Outcome{} : fail(Reason::tower_invariant);
  }

  static Outcome count_body(void* context, u64 lo, u64 hi, u32) noexcept {
    auto& self = *static_cast<Builder*>(context);
    for (u64 block = lo; block < hi; ++block) {
      u64 count = 0;
      for (u64 b = self.begin(block); b < self.begin(block + 1); ++b) count += eligible(self.cat.balls_data()[b], self.kmax);
      self.first[block + 1] = count;  // somme prefixe faite ensuite par le pilote
    }
    return {};
  }
  static Outcome zero_body(void* context, u64 lo, u64 hi, u32) noexcept {
    auto& self = *static_cast<Builder*>(context);
    const u64 part = (self.slots.size() + kBlocks - 1) / kBlocks;
    for (u64 block = lo; block < hi; ++block)
      for (u64 s = std::min(self.slots.size(), block * part); s < std::min(self.slots.size(), (block + 1) * part); ++s)
        self.slots[s] = 0;
    return {};
  }
  static Outcome fill_body(void* context, u64 lo, u64 hi, u32) noexcept {
    auto& self = *static_cast<Builder*>(context);
    for (u64 block = lo; block < hi; ++block) MHGP11_TRY(self.fill(block));
    return {};
  }
};

namespace {
Outcome run(sched::Pool* pool, u64 n, void* context, sched::Pool::Body body) noexcept {
  return pool != nullptr ? pool->parallel_for(n, 1, context, body) : body(context, 0, n, 0);
}
}  // namespace

Result<PopulationLookup> PopulationLookup::make(const FullDomain& domain, MemoryBudget& budget,
                                                sched::Pool* pool) noexcept {
  const auto& cat = domain.catalogue();
  PopulationLookup result(domain);
  result.width_ = static_cast<u32>(cat.kmax()) + kHeader;
  Builder builder{cat, cat.kmax(), cat.balls(), std::max<u64>(1, (u64{cat.balls()} + kBlocks - 1) / kBlocks), {}, {}};
  MHGP11_TRY(run(pool, kBlocks, &builder, Builder::count_body));
  for (u64 block = 0; block < kBlocks; ++block) builder.first[block + 1] += builder.first[block];
  result.entries_ = builder.first[kBlocks];
  if (result.entries_ == 0) return result;
  u64 capacity = 1;
  while (capacity < 2 * result.entries_) capacity *= 2;  // E<2^32 : C<=2^33 cases.
  // 8C<=2^36 et 4E(K+3)<=2^39 octets : sommes sans debordement.
  MHGP11_TRY(budget.admit(capacity * sizeof(u64) + result.entries_ * result.width_ * sizeof(u32)));
  MHGP11_TRY(result.slots_.allocate(capacity, budget));
  MHGP11_TRY(result.rows_.allocate(result.entries_ * result.width_, budget));
  builder.slots = result.slots_.span();
  builder.rows = result.rows_.span();
  MHGP11_TRY(run(pool, kBlocks, &builder, Builder::zero_body));
  MHGP11_TRY(run(pool, kBlocks, &builder, Builder::fill_body));
  return result;
}

struct PopulationLookup::BindContext {
  PopulationLookup* self;
  std::span<OrderForest* const> forests;
};

// Bloc d'entrees : chaque ligne d'ordre present recoit la naissance de sa boule, controlee contre la foret.
Outcome PopulationLookup::bind_body(void* raw, u64 begin, u64 end, u32) noexcept {
  auto& context = *static_cast<BindContext*>(raw);
  auto& self = *context.self;
  const u64 width = std::max<u64>(1, (self.entries_ + kBlocks - 1) / kBlocks);
  for (u64 block = begin; block < end; ++block) {
    const u64 last = std::min(self.entries_, (block + 1) * width);
    for (u64 entry = std::min(self.entries_, block * width); entry < last; ++entry) {
      u32* row = self.rows_.data() + entry * self.width_;
      u32 count = 0;
      while (count + kHeader < self.width_ && row[kHeader + count] != kNone) ++count;
      if (count < 2 || count > context.forests.size() || context.forests[count - 1] == nullptr) continue;
      const OrderForest& forest = *context.forests[count - 1];
      if (u32{forest.order()} != count) return fail(Reason::tower_invariant);
      const auto node = forest.birth_node(BirthSeed(std::nullopt, BallIdx{row[0]}, static_cast<Order>(count)));
      if (!node || idx(*node) >= forest.births()) return fail(Reason::tower_invariant);
      const auto& birth = forest.birth_nodes()[idx(*node)];
      // Lemme du terminal : la naissance de la boule a son rang ; sinon la table ou les naissances sont fausses.
      if (birth.birth_key != row[0] || idx(birth.rank) != row[1]) return fail(Reason::tower_invariant);
      row[2] = idx(*node);
    }
  }
  return {};
}

Outcome PopulationLookup::bind(std::span<OrderForest* const> forests, sched::Pool* pool) noexcept {
  if (domain_ == nullptr || forests.size() > kMaxMebSites) return fail(Reason::parameter_out_of_range);
  // bound_ promet que TOUTE entree est liee : chaque ordre 2..K de la table doit etre fourni.
  if (forests.size() + kHeader != width_) return fail(Reason::parameter_out_of_range);
  for (u64 k = 2; k <= forests.size(); ++k)
    if (forests[k - 1] == nullptr || u64{forests[k - 1]->order()} != k) return fail(Reason::parameter_out_of_range);
  BindContext context{this, forests};
  if (entries_ != 0) MHGP11_TRY(run(pool, kBlocks, &context, &PopulationLookup::bind_body));
  bound_ = true;
  return {};
}

std::optional<PopulationLookup::Bound> PopulationLookup::bound(std::span<const SiteIdx> sorted) const noexcept {
  const u32 count = static_cast<u32>(sorted.size());
  if (slots_.empty() || count < 2 || count + kHeader > width_) return std::nullopt;
  const u64 hash = hash_of(count, [sorted](u32 i) noexcept { return idx(sorted[i]); });
  const u64 mask = slots_.size() - 1, tag = hash >> 32;
  for (u64 position = hash & mask;; position = (position + 1) & mask) {
    const u64 current = slots_[position];  // Lecture apres le join de la construction et de bind.
    if (current == 0) return std::nullopt;
    if ((current >> 32) != tag) continue;
    const u32* row = rows_.data() + ((current & 0xFFFFFFFFull) - 1) * width_;
    bool same = count + kHeader == width_ || row[kHeader + count] == kNone;  // meme cardinal
    for (u32 i = 0; same && i < count; ++i) same = row[kHeader + i] == idx(sorted[i]);
    if (!same) continue;
    if (row[2] == kNone) return std::nullopt;  // ordre non lie : voie complete
    return Bound{NodeIdx{row[2]}, LevelRank{row[1]}};
  }
}

std::optional<BallIdx> PopulationLookup::find(const std::array<SiteIdx, kMaxMebSites>& sorted,
                                              u32 count) const noexcept {
  if (slots_.empty() || count + kHeader > width_) return std::nullopt;
  std::array<u32, kMaxMebSites> raw{};
  for (u32 i = 0; i < count; ++i) raw[i] = idx(sorted[i]);
  const u64 hash = hash_sites(raw.data(), count), mask = slots_.size() - 1, tag = hash >> 32;
  for (u64 position = hash & mask;; position = (position + 1) & mask) {
    const u64 current = slots_[position];  // Lecture apres le join de la construction.
    if (current == 0) return std::nullopt;
    if ((current >> 32) != tag) continue;
    const u32* row = rows_.data() + ((current & 0xFFFFFFFFull) - 1) * width_;
    bool same = count + kHeader == width_ || row[kHeader + count] == kNone;  // meme cardinal
    for (u32 i = 0; same && i < count; ++i) same = row[kHeader + i] == raw[i];
    if (same) return BallIdx{row[0]};
  }
}

Result<std::optional<PopulationLookup::Hit>> PopulationLookup::hit(std::span<const SiteIdx> part,
                                                                  u32 k) const noexcept {
  if (domain_ == nullptr) return fail(Reason::parameter_out_of_range);
  const auto& cat = domain_->catalogue();
  const u32 sites = domain_->index().cloud().sites();
  if (k == 0 || k > cat.kmax() || k > kMaxMebSites || part.size() != k) return std::optional<Hit>{};
  std::array<SiteIdx, kMaxMebSites> sorted{};
  for (u32 i = 0; i < k; ++i) {
    if (idx(part[i]) >= sites) return std::optional<Hit>{};  // La reference refusera elle-meme.
    u32 at = i;
    for (; at > 0 && idx(sorted[at - 1]) > idx(part[i]); --at) sorted[at] = sorted[at - 1];
    sorted[at] = part[i];
  }
  for (u32 i = 1; i < k; ++i)
    if (sorted[i] == sorted[i - 1]) return std::optional<Hit>{};
  // Niveau nul : le site est sa propre naissance d'ordre un.
  if (k == 1) return std::optional{Hit{BirthSeed(sorted[0], std::nullopt, 1), &cat.levels()[0]}};
  const auto ball = find(sorted, k);
  if (!ball) return std::optional<Hit>{};
  return std::optional{Hit{BirthSeed(std::nullopt, *ball, static_cast<Order>(k)),
                           &cat.levels()[idx(cat.balls_data()[idx(*ball)].rank)]}};
}

Result<std::optional<DescentResult>> PopulationLookup::descend(std::span<const SiteIdx> part, u32 k) const noexcept {
  auto found = hit(part, k);
  if (!found.ok()) return found.outcome();
  if (!found.value()) return std::optional<DescentResult>{};
  DescentLedger work;
  work.steps = 1;
  work.population_hits = 1;
  (k == 1 ? work.singleton_hits : work.catalogue_hits) = 1;
  const auto& h = *found.value();
  return std::optional{DescentResult(*h.level, *h.level, h.seed, work)};
}

void PopulationLookup::prefetch(std::span<const SiteIdx> part, u32 k) const noexcept {
  if (slots_.empty() || k < 2 || k > kMaxMebSites || part.size() != k || k + kHeader > width_) return;
  std::array<u32, kMaxMebSites> raw{};  // meme tri et meme empreinte que find
  for (u32 i = 0; i < k; ++i) {
    const u32 value = idx(part[i]);
    u32 at = i;
    for (; at > 0 && raw[at - 1] > value; --at) raw[at] = raw[at - 1];
    raw[at] = value;
  }
  __builtin_prefetch(slots_.data() + (hash_sites(raw.data(), k) & (slots_.size() - 1)));
}

Result<DescentResult> PopulationLookup::descend_each_step(std::span<const SiteIdx> part, u32 k, MemoryBudget& budget,
                                                        CensusWorkspace* scratch, bool first_missed) const noexcept {
  if (domain_ == nullptr) return fail(Reason::parameter_out_of_range);
  if (scratch != nullptr && !scratch->belongs_to(domain_->index())) return fail(Reason::parameter_out_of_range);
  if (!first_missed) {
    auto found = descend(part, k);
    if (!found.ok()) return found.outcome();
    if (found.value()) return *found.value();
  }
  auto first = descent_step(*domain_, part, k, budget, scratch);
  if (!first.ok()) return first.outcome();
  const num::Level initial = first.value().level();
  auto current = first.value();
  DescentLedger ledger;
  for (;;) {
    MHGP11_TRY(add_descent(ledger, current.ledger()));
    if (current.seed()) return DescentResult(initial, current.level(), *current.seed(), ledger);
    auto found = hit(current.next().part(), k);
    if (!found.ok()) return found.outcome();
    if (found.value()) {
      const auto& h = *found.value();
      if (num::compare(*h.level, current.level()) >= 0) return fail(Reason::tower_invariant);
      DescentLedger step;  // Meme ledger qu'un succes de descend : un pas, population et catalogue/singleton.
      step.steps = 1;
      step.population_hits = 1;
      (k == 1 ? step.singleton_hits : step.catalogue_hits) = 1;
      MHGP11_TRY(add_descent(ledger, step));
      return DescentResult(initial, *h.level, h.seed, ledger);
    }
    auto next = descent_step(*domain_, current.next().part(), k, budget, scratch);
    if (!next.ok()) return next.outcome();
    if (num::compare(next.value().level(), current.level()) >= 0) return fail(Reason::tower_invariant);
    current = next.value();
  }
}

}  // namespace mhgp11::tower_detail
