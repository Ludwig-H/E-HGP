// Table S* -> boule (certificats LEM-T1 de la tranche T2) : CSR par premier site de S*, chaque ligne rangee par
// (S*[1], S*[2], S*[3]), case absente au-dela de tout SiteIdx. Construite par la fin d'etage (finish_driver.hpp : tri
// par base des supports, debuts de ligne par dichotomie), deterministe. Un support minimal NON canonique d'une boule n'y
// figure pas : le carre et ses deux diagonales (WIT-T1-CARRE) n'y mettent qu'une diagonale.
// T2-d-B3 : la fin d'etage garde aussi la cle packee de chaque case (celle de son tri par base, hors export) ; la
// recherche est une dichotomie DIRECTE sur les cles de la ligne, sans relire les boules (la dichotomie indirecte par
// balls_ coutait un defaut de cache dependant par pas).
#include <algorithm>

#include "catalogue/finish_kernels.hpp"
#include "catalogue/internal.hpp"

namespace mhgp12 {

std::optional<BallIdx> Catalogue::find_support(std::span<const SiteIdx> support) const noexcept {
  if (support.size() < 2 || support.size() > 4 || table_.off.empty()) return std::nullopt;
  for (u64 k = 1; k < support.size(); ++k)
    if (idx(support[k - 1]) >= idx(support[k])) return std::nullopt;  // S* : SiteIdx strictement croissants
  const u64 sites = table_.off.size() - 1;
  if (idx(support.back()) >= sites) return std::nullopt;  // hors du nuage : aucune boule, cle hors de ses bits
  u32 parts[4];
  for (u32 k = 0; k < 4; ++k) parts[k] = k < support.size() ? idx(support[k]) : static_cast<u32>(sites);
  const catalogue_detail::fin::Key2 packed = catalogue_detail::fin::pack4(parts, table_bits_);
  const TableKey probe{packed.lo, packed.hi};
  const u64 first = idx(support[0]);
  const TableKey* begin = table_keys_.data() + table_.off[first];
  const TableKey* end = table_keys_.data() + table_.off[first + 1];
  const TableKey* found = std::lower_bound(begin, end, probe, [](const TableKey& a, const TableKey& b) {
    return a.hi != b.hi ? a.hi < b.hi : a.lo < b.lo;
  });
  if (found == end || found->hi != probe.hi || found->lo != probe.lo) return std::nullopt;
  return table_.val[static_cast<u64>(found - table_keys_.data())];
}

}  // namespace mhgp12
