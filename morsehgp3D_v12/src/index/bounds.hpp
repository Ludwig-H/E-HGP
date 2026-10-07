// Bornes d'un parcours de census, interne au module index : voie generique (num::LatticeSphere, toute sphere, aucune
// garde) ou voie gardee (num::GuardedSphere, boule certifiee seulement, NUM-GARDE). Une preparation par parcours ; les
// compteurs de voies et de garde sont reportes dans le registre du census a chaque evaluation.
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
  Result<num::PowerBoundSigns> bound_signs(const num::Box& box, CensusLedger& ledger) const noexcept {
    num::GuardLedger guard;
    auto signs = guard_.bound_signs(box, &guard);
    absorb(guard, ledger);
    return signs;
  }
  Result<int> side(num::Point point, CensusLedger& ledger) const noexcept {
    num::GuardLedger guard;
    auto value = guard_.side(point, &guard);
    absorb(guard, ledger);
    return value;
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
};

}  // namespace mhgp12::index_detail
