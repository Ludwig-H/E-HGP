// Premier raccord de tower : MEB exacte d'une petite partie et census sur le meme index global.
// Construction neuve v11, pas encore une descente, un catalogue de cellules ou une foret FULL.
#pragma once

#include <array>
#include <span>

#include "index/index.hpp"

namespace mhgp11 {

inline constexpr u32 kMaxMebSites = 12;

// Travail effectivement execute ; une presentation est une sous-partie d'arite 1..4.
// point_tests s'arrete au premier point exterieur. comparisons exclut le premier contenant.
struct MebLedger {
  u64 presentations = 0, nondegenerate = 0, positive = 0, containing = 0, comparisons = 0, point_tests = 0;
  friend bool operator==(const MebLedger&, const MebLedger&) = default;
};

class BoundedMeb {
 public:
  const num::Sphere& sphere() const noexcept { return sphere_; }
  std::span<const SiteIdx> support() const noexcept { return {support_.data(), arity_}; }
  const MebLedger& ledger() const noexcept { return ledger_; }

 private:
  friend Result<BoundedMeb> bounded_meb(const Cloud&, std::span<const SiteIdx>) noexcept;
  BoundedMeb(num::Sphere sphere, std::array<SiteIdx, 4> support, u8 arity, MebLedger ledger) noexcept
      : sphere_(std::move(sphere)), support_(support), arity_(arity), ledger_(ledger) {}
  num::Sphere sphere_;
  std::array<SiteIdx, 4> support_;
  u8 arity_;
  MebLedger ledger_;
};

// M1 : toutes les presentations 1..4, positivite stricte et inclusion de TOUTE la partie, minimum exact.
// Ordre des refus : partie vide, taille >12, Cloud vide, SiteIdx hors domaine, SiteIdx repete.
// Taille >12, indice hors domaine ou repete : parameter_out_of_range. Vide : empty_input.
// Partie empruntee stable pendant l'appel ; copies fixes (12 points/IDs), aucune allocation ni cache.
// support() est LOCAL a la partie : cardinal minimal puis ordre lexicographique des SiteIdx croissants.
// Sphere et IDs sont possedes ; interpreter ces IDs exige le meme Cloud. Pas de cle de boule globale.
// Les poids sont ignores : primitive geometrique de sites, pas qualification de FULL pondere.
[[nodiscard]] Result<BoundedMeb> bounded_meb(const Cloud& cloud, std::span<const SiteIdx> part) noexcept;

class MebCensus {
 public:
  MebCensus(const MebCensus&) = delete;
  MebCensus& operator=(const MebCensus&) = delete;
  MebCensus& operator=(MebCensus&&) = delete;
  MebCensus(MebCensus&&) noexcept = default;
  const BoundedMeb& meb() const noexcept { return meb_; }
  const Census& population() const noexcept { return population_; }

 private:
  friend Result<MebCensus> meb_census(const GlobalIndex&, std::span<const SiteIdx>, u32, MemoryBudget&) noexcept;
  MebCensus(BoundedMeb meb, Census&& population) noexcept
      : meb_(std::move(meb)), population_(std::move(population)) {}
  BoundedMeb meb_;
  Census population_;
};

// Seuil positif controle avant la partie. MEB puis census de index.cloud() au meme appel synchrone.
// Refus transactionnel, reservations du census seulement ; aucun MEB partiel publie sur refus memoire.
// Resultat sature : threshold temoins stricts, pas une population complete ni un support canonique global.
// Complete : tout I/U, coquille sans plafond 12. Les deux resultats restent dans le meme domaine SiteIdx.
[[nodiscard]] Result<MebCensus> meb_census(const GlobalIndex& index, std::span<const SiteIdx> part,
                                         u32 threshold, MemoryBudget& budget) noexcept;

}  // namespace mhgp11
