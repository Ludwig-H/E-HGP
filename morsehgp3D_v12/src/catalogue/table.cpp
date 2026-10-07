// Table S* -> boule (certificats LEM-T1 de la tranche T2) : CSR par premier site de S*, chaque ligne rangee par
// (S*[1], S*[2], S*[3]) ; recherche par dichotomie dans la ligne. Construite apres l'ordre canonique, dans le budget ;
// deterministe (comptage, prefixe, places dans l'ordre des BallIdx, tri des lignes). Un support minimal NON canonique
// d'une boule n'y figure pas : le carre et ses deux diagonales (WIT-T1-CARRE) n'y mettent qu'une diagonale.
#include <algorithm>

#include "catalogue/internal.hpp"

namespace mhgp12 {
namespace catalogue_detail {
namespace {

bool tail_less(const CatalogueBall& a, const CatalogueBall& b) noexcept {
  for (u32 k = 1; k < 4; ++k)
    if (a.support[k] != b.support[k]) return idx(a.support[k]) < idx(b.support[k]);
  return false;
}

}  // namespace

Outcome build_table(Csr<BallIdx>& table, std::span<const CatalogueBall> balls, u32 sites,
                    MemoryBudget& budget) noexcept {
  Csr<BallIdx> made;
  MHGP12_TRY(made.off.allocate(u64{sites} + 1, budget));
  MHGP12_TRY(made.val.allocate(balls.size(), budget));
  for (u64 s = 0; s <= sites; ++s) made.off[s] = 0;
  for (const auto& ball : balls) {
    const u32 first = idx(ball.support[0]);
    if (first >= sites) return fail(Reason::catalogue_invariant);
    ++made.off[first + 1];
  }
  for (u64 s = 0; s < sites; ++s) made.off[s + 1] += made.off[s];
  Buffer<u64> cursor;
  MHGP12_TRY(cursor.allocate(sites, budget));
  for (u64 s = 0; s < sites; ++s) cursor[s] = made.off[s];
  for (u64 b = 0; b < balls.size(); ++b) made.val[cursor[idx(balls[b].support[0])]++] = make_id<BallIdx>(static_cast<u32>(b));
  for (u64 s = 0; s < sites; ++s)
    std::sort(made.val.data() + made.off[s], made.val.data() + made.off[s + 1],
              [&](BallIdx a, BallIdx b) { return tail_less(balls[idx(a)], balls[idx(b)]); });
  table.off.swap(made.off);
  table.val.swap(made.val);
  return {};
}

}  // namespace catalogue_detail

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
    return catalogue_detail::tail_less(balls_[idx(a)], key);
  });
  if (found == end || balls_[idx(*found)].support != probe.support) return std::nullopt;
  return *found;
}

}  // namespace mhgp12
