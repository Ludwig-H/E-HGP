// Etats internes du catalogue : enregistrements des boules emises, lots de feuilles, etage des feuilles et fin
// d'etage. Aucun etat global modifiable.
#pragma once

#include <vector>

#include "catalogue/catalogue.hpp"
#include "catalogue/leaf_j3.hpp"
#include "catalogue/traversal.hpp"

namespace mhgp12::catalogue_detail {

namespace fin {
struct FinishOutput;
}

// Boule emise par une feuille, en SiteIdx globaux : S* croissant (kNone au-dela de qmin), debut de sa population
// (I puis U) dans la population de son lot.
struct BallRecord {
  u32 support[4];
  u64 population;
  u8 p, m, qmin, pad;
  u32 chunk;
};
static_assert(sizeof(BallRecord) == 32, "catalogue : 32 octets par boule emise");

// Lot de feuilles traite d'un bloc (comptage, reservation exacte, ecriture) : ses boules et leurs populations.
struct Chunk {
  Buffer<BallRecord> records;
  Buffer<SiteIdx> population;
};

// Totaux de l'etage des feuilles : compteurs logiques (sommes sur les feuilles resolues) et diagnostics physiques.
struct LeafTotals {
  LeafCounts counts{};
  u64 narrow = 0, medium = 0, wide = 0, exact = 0, virtual_warp = 0, rewritten = 0, max_span = 0;
  u64 count_ns = 0, fill_ns = 0;
};

// Etage des feuilles, consommateur du parcours : chaque niveau est decoupe en lots d'au plus kBatchLeaves feuilles ;
// un lot est compte (feuille J3 avec une case de kCase emissions par feuille), reserve exactement, puis ecrit (copie
// des cases, rejeu des feuilles qui en debordent, meme source, meme arithmetique). Une feuille ne compte qu'une fois,
// au comptage, apres succes ; le rejeu doit rendre les memes comptes (invariant).
class LeafStage final : public LeafConsumer {
 public:
  LeafStage(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget, sched::Pool& pool) noexcept
      : cloud_(cloud), params_(params), budget_(budget), pool_(pool) {}
  [[nodiscard]] Outcome consume(std::span<const bfs::Leaf> leaves, std::span<const u32> sites,
                                u32 depth) noexcept override;
  std::vector<Chunk>& chunks() noexcept { return chunks_; }
  const LeafTotals& totals() const noexcept { return totals_; }
  u64 balls() const noexcept { return balls_; }

 private:
  Outcome batch(std::span<const bfs::Leaf> leaves, std::span<const u32> sites) noexcept;
  const Cloud& cloud_;
  const CatalogueParams& params_;
  MemoryBudget& budget_;
  sched::Pool& pool_;
  std::vector<Chunk> chunks_;
  LeafTotals totals_;
  u64 balls_ = 0;
};

// Grand livre logique du catalogue : parcours et sommes des quinze compteurs de feuille (voies CPU et appareil).
[[nodiscard]] CatalogueLedger make_ledger(const TraversalLedger& walked, const LeafCounts& counts) noexcept;

// Somme controlee des quinze compteurs d'une feuille : faux sur depassement de 2^64 - 1 (refus
// catalogue_counter_overflow). Bornes : chaque champ d'une feuille de m <= 256 sites est < m * sum_{q<=4} C(m,q) < 2^42.
[[nodiscard]] bool add_leaf_counts(LeafCounts& into, const LeafCounts& c) noexcept;

struct Assembly {
  // Fin d'etage de la voie CPU : lots rassembles, fin d'etage partagee (finish_driver.hpp) jouee par le Pool.
  static Result<Catalogue> finish(const Cloud& cloud, const CatalogueParams& params, std::vector<Chunk>& chunks,
                                  u64 balls, const CatalogueLedger& ledger, MemoryBudget& budget, sched::Pool& pool,
                                  CatalogueDiagnostics& diagnostics) noexcept;
  // Publication commune aux deux voies : sorties de la fin d'etage (videes ; niveaux deja materialises par le flux).
  static Result<Catalogue> adopt(fin::FinishOutput& out, Order kmax, const CatalogueLedger& ledger) noexcept;
};

}  // namespace mhgp12::catalogue_detail
