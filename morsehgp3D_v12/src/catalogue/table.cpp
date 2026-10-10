// Table S* -> boule (certificats LEM-T1 de la tranche T2) : CSR par premier site de S*, chaque ligne rangee par
// (S*[1], S*[2], S*[3]), case absente au-dela de tout SiteIdx ; recherche par dichotomie dans la ligne. Construite par
// la fin d'etage (finish_driver.hpp : tri par base des supports, debuts de ligne par dichotomie), deterministe. Un
// support minimal NON canonique d'une boule n'y figure pas : le carre et ses deux diagonales (WIT-T1-CARRE) n'y mettent
// qu'une diagonale.
#include <algorithm>

#include "catalogue/internal.hpp"

namespace mhgp12 {
namespace {

bool tail_less(const CatalogueBall& a, const CatalogueBall& b) noexcept {
  for (u32 k = 1; k < 4; ++k)
    if (a.support[k] != b.support[k]) return idx(a.support[k]) < idx(b.support[k]);
  return false;
}

}  // namespace

std::optional<BallIdx> Catalogue::find_support(std::span<const SiteIdx> support) const noexcept {
  if (support.size() < 2 || support.size() > 4 || table_.off.empty()) return std::nullopt;
  for (u64 k = 1; k < support.size(); ++k)
    if (idx(support[k - 1]) >= idx(support[k])) return std::nullopt;  // S* : SiteIdx strictement croissants
  const u32 first = idx(support[0]);
  if (u64{first} + 1 >= table_.off.size()) return std::nullopt;
  CatalogueBall probe;
  for (u32 k = 0; k < 4; ++k) probe.support[k] = k < support.size() ? support[k] : make_id<SiteIdx>(kNone);
  const BallIdx* begin = table_.val.data() + table_.off[first];
  const BallIdx* end = table_.val.data() + table_.off[first + 1];
  const BallIdx* found = std::lower_bound(begin, end, probe, [&](BallIdx a, const CatalogueBall& key) {
    return tail_less(balls_[idx(a)], key);
  });
  if (found == end || balls_[idx(*found)].support != probe.support) return std::nullopt;
  return *found;
}

}  // namespace mhgp12
