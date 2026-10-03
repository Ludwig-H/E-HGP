// Descente privee sans memo : chaque transition diminue beta, le terminal est date par beta(initiale).
#pragma once

#include "tower/cells.hpp"

namespace mhgp11::tower_detail {

struct MemoLedger {
  u64 queries = 0, lookups = 0, hits = 0, misses = 0, collisions = 0;
  u64 insertions = 0, evictions = 0, suffix_hits = 0;
  friend bool operator==(const MemoLedger&, const MemoLedger&) = default;
};
struct DescentLedger {
  u64 steps = 0, interior_steps = 0, trace_steps = 0, candidate_traces = 0, trace_meb_calls = 0;
  u64 census_calls = 0, catalogue_hits = 0;
  MebLedger part_meb, trace_meb;
  CensusLedger census;
  MemoLedger memo;  // Travail de CET appel uniquement ; aucun ledger ancien rejoue lors d'un hit.
  friend bool operator==(const DescentLedger&, const DescentLedger&) = default;
};

// Addition transactionnelle de tous les compteurs ; aucun compteur enveloppe silencieusement.
[[nodiscard]] Outcome add_descent(DescentLedger& sum, const DescentLedger& one) noexcept;

struct DescentBuilder;
class DescentMemo;
class BirthSeed {
 public:
  std::optional<SiteIdx> site() const noexcept { return site_; }
  std::optional<BallIdx> ball() const noexcept { return ball_; }
  Order order() const noexcept { return order_; }
  friend bool operator==(const BirthSeed&, const BirthSeed&) = default;

 private:
  friend struct DescentBuilder;
  friend class DescentMemo;
  BirthSeed(std::optional<SiteIdx> site, std::optional<BallIdx> ball, Order order) noexcept
      : site_(site), ball_(ball), order_(order) {}
  std::optional<SiteIdx> site_;
  std::optional<BallIdx> ball_;
  Order order_;
};

class DescentStep {
 public:
  const num::Level& level() const noexcept { return level_; }
  const CellTrace& next() const noexcept { return next_; }
  const std::optional<BirthSeed>& seed() const noexcept { return seed_; }
  const DescentLedger& ledger() const noexcept { return ledger_; }

 private:
  friend struct DescentBuilder;
  DescentStep(num::Level level, CellTrace next, std::optional<BirthSeed> seed, DescentLedger ledger) noexcept
      : level_(level), next_(next), seed_(seed), ledger_(ledger) {}
  num::Level level_;
  CellTrace next_;
  std::optional<BirthSeed> seed_;
  DescentLedger ledger_;
};

class DescentResult {
 public:
  const num::Level& initial_level() const noexcept { return initial_; }
  const num::Level& terminal_level() const noexcept { return terminal_; }
  const BirthSeed& seed() const noexcept { return seed_; }
  const DescentLedger& ledger() const noexcept { return ledger_; }

 private:
  friend Result<DescentResult> descend(const FullDomain&, std::span<const SiteIdx>, u32, MemoryBudget&,
                                      CensusWorkspace*) noexcept;
  friend class DescentMemo;
  DescentResult(num::Level initial, num::Level terminal, BirthSeed seed, DescentLedger ledger) noexcept
      : initial_(initial), terminal_(terminal), seed_(seed), ledger_(ledger) {}
  num::Level initial_, terminal_;
  BirthSeed seed_;
  DescentLedger ledger_;
};

// k dans 1..K puis partie de cardinal k (<=12), IDs distincts du meme domaine. Refus de locate_part.
// p>=k se traite AVANT le cas complete, y compris lors d'un hit catalogue complet. Sinon une premiere
// trace stricte I union A est cherchee, meme hors de la fenetre des cellules et hors de CatK.
// next() est trie, padde kNone et de cardinal k ; un terminal a next vide et exactement une seed.
// Aucun chemin possede. Sans workspace : census possede temporaire, deux passes. Avec : une passe dans
// le stockage deja reserve, identite verifiee meme sur hit. Aucune vue empruntee dans DescentStep/Result.
[[nodiscard]] Result<DescentStep> descent_step(const FullDomain&, std::span<const SiteIdx>, u32 k,
                                              MemoryBudget&, CensusWorkspace* = nullptr) noexcept;

// Le terminal represente la classe de la partie seulement aux coupes fermees a>=initial_level().
// Pour une coupe ouverte il faut a>initial_level(). Il reste a le relever dans la future foret au niveau a.
// Ne jamais remplacer cette date par terminal_level(). Les IDs exigent le meme domaine pour interpretation.
// Toute transition est verifiee strictement decroissante ; finitude sur les k-parties, sans plafond arbitraire.
[[nodiscard]] Result<DescentResult> descend(const FullDomain&, std::span<const SiteIdx>, u32 k,
                                          MemoryBudget&, CensusWorkspace* = nullptr) noexcept;

}  // namespace mhgp11::tower_detail
