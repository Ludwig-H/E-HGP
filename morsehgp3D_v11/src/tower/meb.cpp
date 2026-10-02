// MEB de cardinal borne : premier support strict contenant, ordre arite/lex, aucune enumeration globale.
#include "tower/tower.hpp"

#include <algorithm>
#include <optional>

namespace mhgp11 {
namespace {

struct Part {
  std::array<SiteIdx, kMaxMebSites> ids{};
  std::array<num::Point, kMaxMebSites> points{};
  u32 size = 0;
};

Result<Part> prepare(const Cloud& cloud, std::span<const SiteIdx> input) noexcept {
  if (input.empty()) return fail(Reason::empty_input);
  if (input.size() > kMaxMebSites) return fail(Reason::parameter_out_of_range);
  if (cloud.sites() == 0) return fail(Reason::empty_input);
  Part part;
  part.size = static_cast<u32>(input.size());
  for (u32 i = 0; i < part.size; ++i) {
    if (idx(input[i]) >= cloud.sites()) return fail(Reason::parameter_out_of_range);
    part.ids[i] = input[i];
  }
  std::sort(part.ids.begin(), part.ids.begin() + part.size,
            [](SiteIdx a, SiteIdx b) noexcept { return idx(a) < idx(b); });
  for (u32 i = 0; i < part.size; ++i) {
    if (i != 0 && part.ids[i] == part.ids[i - 1]) return fail(Reason::parameter_out_of_range);
    const auto s = idx(part.ids[i]);
    auto point = num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]);
    if (!point.ok()) return point.outcome();
    part.points[i] = point.value();
  }
  return part;
}

Result<std::optional<num::Sphere>> sphere_of(const Part& part, const std::array<u32, 4>& tuple,
                                            u8 arity) noexcept {
  const auto a = part.points[tuple[0]];
  if (arity == 1) return std::optional{num::Sphere::point(a)};
  const auto b = part.points[tuple[1]];
  if (arity == 2) return num::Sphere::through(a, b);
  const auto c = part.points[tuple[2]];
  if (arity == 3) return num::Sphere::through(a, b, c);
  return num::Sphere::through(a, b, c, part.points[tuple[3]]);
}

Result<bool> strict_support(const Part& part, const std::array<u32, 4>& tuple,
                            u8 arity, const num::Sphere& sphere) noexcept {
  if (arity <= 2) return true;  // Point ou milieu de deux sites distincts certifies par Cloud.
  const auto a = part.points[tuple[0]], b = part.points[tuple[1]], c = part.points[tuple[2]];
  if (arity == 3) return num::strictly_acute(a, b, c);
  return num::strictly_inside(sphere, a, b, c, part.points[tuple[3]]);
}

struct Search {
  const Part& part;
  std::optional<num::Sphere> best;
  std::array<SiteIdx, 4> support{};
  u8 arity = 0;
  MebLedger ledger;

  Outcome consider(const std::array<u32, 4>& tuple, u8 q) noexcept {
    ++ledger.presentations;
    auto made = sphere_of(part, tuple, q);
    if (!made.ok()) return made.outcome();
    if (!made.value()) return {};
    ++ledger.nondegenerate;
    const auto& sphere = *made.value();
    auto positive = strict_support(part, tuple, q, sphere);
    if (!positive.ok()) return positive.outcome();
    if (!positive.value()) return {};
    ++ledger.positive;
    for (u32 i = 0; i < part.size; ++i) {
      ++ledger.point_tests;
      auto side = num::side(sphere, part.points[i]);
      if (!side.ok()) return side.outcome();
      if (side.value() > 0) return {};
    }
    ++ledger.containing;
    // M1 : le centre est dans conv(support) et toute la partie est contenue. Cette boule est sa MEB.
    best = sphere;
    arity = q;
    support.fill(make_id<SiteIdx>(kNone));
    for (u8 i = 0; i < q; ++i) support[i] = part.ids[tuple[i]];
    return {};
  }

  Outcome extend(std::array<u32, 4>& tuple, u8 depth, u8 q, u32 start) noexcept {
    if (depth == q) return consider(tuple, q);
    for (u32 i = start; i + (q - depth) <= part.size; ++i) {
      tuple[depth] = i;
      MHGP11_TRY(extend(tuple, static_cast<u8>(depth + 1), q, i + 1));
      if (best) return {};  // Propager l'arret dans chaque niveau de la combinaison courante.
    }
    return {};
  }
};

}  // namespace

Result<BoundedMeb> bounded_meb(const Cloud& cloud, std::span<const SiteIdx> part) noexcept {
  auto prepared = prepare(cloud, part);
  if (!prepared.ok()) return prepared.outcome();
  Search search{prepared.value(), {}, {}, 0, {}};
  std::array<u32, 4> tuple{};
  // M1 assure un support minimal strict de taille <=4. Aucune positivite de prefixe n'elague q4.
  // <=12+66+220+495=793 presentations, <=9516 tests, aucune comparaison de niveaux.
  // Le premier support contenant est minimal en arite puis lex ; aucun candidat suivant n'est necessaire.
  // Sphere/side gardent les budgets qualifies num, sans nouvelle expression numerique.
  static_assert(kMaxMebSites == 12 && kCoordBits <= 24);
  for (u8 q = 1; q <= 4 && q <= prepared.value().size && !search.best; ++q)
    MHGP11_TRY(search.extend(tuple, 0, q, 0));
  if (!search.best) return fail(Reason::arithmetic_invariant);
  return BoundedMeb(*search.best, search.support, search.arity, search.ledger);
}

Result<MebCensus> meb_census(const GlobalIndex& index, std::span<const SiteIdx> part,
                            u32 threshold, MemoryBudget& budget) noexcept {
  if (threshold == 0) return fail(Reason::parameter_out_of_range);
  auto meb = bounded_meb(index.cloud(), part);
  if (!meb.ok()) return meb.outcome();
  auto population = census(index, meb.value().sphere(), threshold, budget);
  if (!population.ok()) return population.outcome();
  return MebCensus(std::move(meb).take(), std::move(population).take());
}

}  // namespace mhgp11
