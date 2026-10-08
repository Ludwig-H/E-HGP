// Sources du domaine immuable lues par les etages M et V et par l'export (forest.hpp) : supports S* du catalogue de T1,
// lus en place par leur BallIdx.
#include "tower/forest.hpp"

namespace mhgp12::tower {
namespace {

std::array<u32, 4> catalogue_support(const void* context, u32 ball) noexcept {
  const CatalogueBall& b = static_cast<const Catalogue*>(context)->balls_data()[ball];
  return {idx(b.support[0]), idx(b.support[1]), idx(b.support[2]), idx(b.support[3])};
}

}  // namespace

BallSource catalogue_balls(const Catalogue& catalogue) noexcept {
  return BallSource{&catalogue, catalogue.balls(), catalogue_support};
}

}  // namespace mhgp12::tower
