// Resolution privee d'une MEB dans le domaine FULL : le census complet vient toujours du meme index.
#pragma once

#include "tower/tower.hpp"

namespace mhgp11::tower_detail {

struct SupportKey {
  std::array<SiteIdx, 4> sites;
  u8 arity;
};

// Population complete obtenue seulement par locate_part. Aucun appel public sur une coquille arbitraire.
Result<SupportKey> global_support(const FullDomain&, const BoundedMeb&, const Census&) noexcept;
Result<SupportKey> global_support(const FullDomain&, const BoundedMeb&, const BorrowedCensus&) noexcept;

class LocatedPart {
 public:
  LocatedPart(const LocatedPart&) = delete;
  LocatedPart& operator=(const LocatedPart&) = delete;
  LocatedPart& operator=(LocatedPart&&) = delete;
  LocatedPart(LocatedPart&& other) noexcept
      : meb_(std::move(other.meb_)), ball_(std::exchange(other.ball_, std::nullopt)),
        support_(std::exchange(other.support_, std::nullopt)), owned_(std::move(other.owned_)),
        interior_(std::exchange(other.interior_, {})), shell_(std::exchange(other.shell_, {})) {
    other.owned_.reset();
  }
  const BoundedMeb& meb() const noexcept { return meb_; }
  std::optional<BallIdx> ball() const noexcept { return ball_; }
  // qmin global connu seulement si complete. Pour saturated, support() rend nullopt.
  std::optional<SupportKey> support() const noexcept { return support_; }
  CensusKind kind() const noexcept { return owned_ ? owned_->kind() : CensusKind::complete; }
  std::span<const SiteIdx> interior() const noexcept { return interior_; }
  std::span<const SiteIdx> shell() const noexcept { return shell_; }
  const CensusLedger* census_work() const noexcept { return owned_ ? &owned_->ledger() : nullptr; }

 private:
  friend Result<LocatedPart> locate_part(const FullDomain&, std::span<const SiteIdx>, u32, MemoryBudget&) noexcept;
  LocatedPart(BoundedMeb&& meb, const FullDomain& domain, BallIdx ball) noexcept
      : meb_(std::move(meb)), ball_(ball),
        support_(SupportKey{domain.catalogue().balls_data()[idx(ball)].support,
                            domain.catalogue().balls_data()[idx(ball)].qmin}),
        interior_(domain.catalogue().interior(ball)), shell_(domain.catalogue().shell(ball)) {}
  LocatedPart(BoundedMeb&& meb, Census&& population, std::optional<SupportKey> support,
              std::optional<BallIdx> ball) noexcept
      : meb_(std::move(meb)), ball_(ball), support_(support), owned_(std::move(population)),
        interior_(owned_->interior()), shell_(owned_->shell()) {}
  BoundedMeb meb_;
  std::optional<BallIdx> ball_;
  std::optional<SupportKey> support_;
  std::optional<Census> owned_;
  std::span<const SiteIdx> interior_, shell_;
};

// k dans 1..K, partie de k sites distincts. Le domaine survit au resultat (populations empruntees si hit).
// Un hit du support local strict determine la meme boule par M1/M2. Un miss ne conclut jamais a l'absence :
// census seuil k, puis S* global seulement si complete. Un resultat saturated porte exactement k temoins.
// Aucun tableau dependant de l'entree hors du Census budgete ; aucun memo ni foret n'est construit ici.
Result<LocatedPart> locate_part(const FullDomain&, std::span<const SiteIdx>, u32 k, MemoryBudget&) noexcept;

// Vue interne synchrone : ni cette vue ni ses spans ne survivent au callback. Le consommateur doit
// copier seulement ses valeurs (Level/trace/seed/ledger), jamais la population ou une reference a la MEB.
struct LocatedView {
  const BoundedMeb& source;
  std::optional<BallIdx> found;
  std::optional<SupportKey> key;
  CensusKind population_kind;
  std::span<const SiteIdx> inner, outer;
  const CensusLedger* work;
  const BoundedMeb& meb() const noexcept { return source; }
  std::optional<BallIdx> ball() const noexcept { return found; }
  std::optional<SupportKey> support() const noexcept { return key; }
  CensusKind kind() const noexcept { return population_kind; }
  std::span<const SiteIdx> interior() const noexcept { return inner; }
  std::span<const SiteIdx> shell() const noexcept { return outer; }
  const CensusLedger* census_work() const noexcept { return work; }
};
using LocatedCallback = Outcome (*)(void*, const LocatedView&) noexcept;
// nullptr garde locate_part possede. Workspace et domaine restent immobiles durant l'appel.
Outcome visit_located_part(const FullDomain&, std::span<const SiteIdx>, u32, MemoryBudget&,
                           CensusWorkspace*, void*, LocatedCallback) noexcept;

}  // namespace mhgp11::tower_detail
