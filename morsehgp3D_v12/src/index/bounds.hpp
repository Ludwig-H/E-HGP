// Bornes d'un parcours de census, interne au module index : voie generique (num::LatticeSphere, toute sphere, aucune
// garde) ou voie gardee (num::GuardedSphere, boule certifiee seulement, NUM-GARDE). Une preparation par parcours ; les
// compteurs de voies vont au registre du census a chaque evaluation (voie generique), ou s'accumulent dans la garde et
// sont reportes une fois a la fin du parcours par flush (voie gardee, T2-d : plus de registre temporaire par appel).
#pragma once

#include "index/index.hpp"

namespace mhgp12::index_detail {

class GenericBounds {
 public:
  explicit GenericBounds(const num::Sphere& sphere) noexcept : sphere_(sphere), lattice_(sphere) {}
  GenericBounds(const GenericBounds&) = delete;
  GenericBounds& operator=(const GenericBounds&) = delete;
  const num::Sphere& sphere() const noexcept { return sphere_; }
  Result<num::PowerBoundSigns> bound_signs(const num::Box& box, CensusLedger& ledger) const noexcept {
    return lattice_.bound_signs(box, &ledger.lanes);  // minorant sur sites entiers, majorant continu
  }
  Result<int> side(num::Point point, CensusLedger& ledger) const noexcept { return lattice_.side(point, &ledger.lanes); }
  void flush(CensusLedger&) const noexcept {}

 private:
  const num::Sphere& sphere_;
  num::LatticeSphere lattice_;
};

class GuardedBounds {
 public:
  explicit GuardedBounds(const num::CertifiedBall& ball) noexcept : ball_(ball), guard_(ball) {}
  GuardedBounds(const GuardedBounds&) = delete;
  GuardedBounds& operator=(const GuardedBounds&) = delete;
  const num::Sphere& sphere() const noexcept { return ball_.sphere(); }
  Result<num::PowerBoundSigns> bound_signs(const num::Box& box, CensusLedger&) const noexcept {
    return guard_.bound_signs(box, &guard_ledger_);
  }
  Result<int> side(num::Point point, CensusLedger&) const noexcept { return guard_.side(point, &guard_ledger_); }
  // Report des compteurs de la garde dans le registre du census, une fois a la fin du parcours.
  void flush(CensusLedger& ledger) const noexcept {
    absorb(guard_ledger_, ledger);
    guard_ledger_ = {};
  }

 private:
  static void absorb(const num::GuardLedger& guard, CensusLedger& ledger) noexcept {
    ledger.lanes.native += guard.lanes.native;
    ledger.lanes.certified += guard.lanes.certified;
    ledger.lanes.checked += guard.lanes.checked;
    ledger.lanes.wide += guard.lanes.wide;
    ledger.guard_disjoint += guard.disjoint_boxes;
    ledger.guard_partial += guard.partial_boxes;
    ledger.guard_outside += guard.outside_sites;
  }
  const num::CertifiedBall& ball_;
  num::GuardedSphere guard_;
  mutable num::GuardLedger guard_ledger_;  // compteurs du parcours en cours, reportes par flush
};

}  // namespace mhgp12::index_detail
