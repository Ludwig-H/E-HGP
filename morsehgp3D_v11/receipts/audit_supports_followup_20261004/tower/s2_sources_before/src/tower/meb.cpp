// MEB bornee : diametre exact canonique, puis premier q3/q4 strict contenant ; aucune enumeration globale.
#include "tower/meb.hpp"

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
  if (arity == 2) return num::Sphere::through(a, part.points[tuple[1]]);
  return fail(Reason::arithmetic_invariant);  // q3/q4 gardent un candidat sans Level (consider_q3/q4).
}

struct Search {
  const Part& part;
  std::optional<num::Sphere> best;
  std::array<SiteIdx, 4> support{};
  u8 arity = 0;
  MebLedger ledger;

  Outcome select_diameter(std::array<u32, 4>& tuple) noexcept {
    // n>1, sites distincts deja certifies. La primitive num porte la borne native <2^50 aux trois profils.
    // Aucun calcul geometrique ni formule de distance n'est recopie dans tower.
    num::DotInt longest = 0;
    for (u32 i = 0; i + 1 < part.size; ++i) {
      for (u32 j = i + 1; j < part.size; ++j) {
        ++ledger.diameter_pairs;  // <=C(12,2)=66, hors presentations.
        const auto distance = num::squared_distance(part.points[i], part.points[j]);
        if (distance > longest) {
          longest = distance; tuple[0] = i; tuple[1] = j;
        }
      }
    }
    return {};  // Egalite : conserver la premiere paire lex, jamais la derniere.
  }

  template<class Ball>
  Result<bool> contains(const Ball& sphere) noexcept {
    for (u32 i = 0; i < part.size; ++i) {
      ++ledger.point_tests;
      auto side = num::side(sphere, part.points[i]);
      if (!side.ok()) return side.outcome();
      if (side.value() > 0) return false;
    }
    return true;
  }

  Outcome accept(const num::Sphere& sphere, const std::array<u32, 4>& tuple, u8 q) noexcept {
    ++ledger.containing;
    // M1 : le centre est dans conv(support) et toute la partie est contenue. Cette boule est sa MEB.
    best = sphere;
    arity = q;
    support.fill(make_id<SiteIdx>(kNone));
    for (u8 i = 0; i < q; ++i) support[i] = part.ids[tuple[i]];
    return {};
  }

  Outcome consider_q4(const std::array<u32, 4>& tuple) noexcept {
    auto made = num::Q4Candidate::through(part.points[tuple[0]], part.points[tuple[1]],
                                        part.points[tuple[2]], part.points[tuple[3]]);
    if (!made.ok()) return made.outcome();
    if (!made.value()) return {};
    ++ledger.nondegenerate;
    if (!made.value()->q4_presentation_strictly_inside()) return {};
    ++ledger.positive;
    auto inside = contains(*made.value());
    if (!inside.ok()) return inside.outcome();
    if (!inside.value()) return {};
    auto sphere = made.value()->materialize();
    if (!sphere.ok()) return sphere.outcome();
    return accept(sphere.value(), tuple, 4);
  }

  // Report du Level q3 (audit heritage 1235da4ac) : meme centre N/D, memes tests d'inclusion et meme ordre ;
  // seul le support accepte materialise la formule brute de degre six.
  Outcome consider_q3(const std::array<u32, 4>& tuple) noexcept {
    const auto a = part.points[tuple[0]], b = part.points[tuple[1]], c = part.points[tuple[2]];
    const auto kind = num::classify_triangle(a, b, c);
    if (kind == num::TriangleKind::degenerate) return {};
    ++ledger.nondegenerate;
    if (kind == num::TriangleKind::non_strict) return {};
    auto made = num::Q3Candidate::through(a, b, c);
    if (!made.ok()) return made.outcome();
    if (!made.value()) return fail(Reason::arithmetic_invariant);  // q3 strict certifie non degenere.
    ++ledger.positive;
    auto inside = contains(*made.value());
    if (!inside.ok()) return inside.outcome();
    if (!inside.value()) return {};
    auto sphere = made.value()->materialize();
    if (!sphere.ok()) return sphere.outcome();
    return accept(sphere.value(), tuple, 3);
  }

  Outcome consider(const std::array<u32, 4>& tuple, u8 q) noexcept {
    // Presentations et nondegenerate comptent les candidats LOGIQUES, pas les Sphere/Level materialises.
    // q3 rejete avant le centre, puis avant le Level ; q4 avant le Level. Ordre, sept compteurs et arret
    // d'inclusion restent identiques.
    ++ledger.presentations;
    if (q == 4) return consider_q4(tuple);  // Aucune condition sur l'acuite d'une face q3.
    if (q == 3) return consider_q3(tuple);
    auto made = sphere_of(part, tuple, q);
    if (!made.ok()) return made.outcome();
    if (!made.value()) return fail(Reason::arithmetic_invariant);  // q1/q2 distincts.
    ++ledger.nondegenerate;
    ++ledger.positive;
    auto inside = contains(*made.value());
    if (!inside.ok()) return inside.outcome();
    if (!inside.value()) return {};
    return accept(*made.value(), tuple, q);
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
  // Diametre : si sa boule contient F elle est la MEB q2 canonique ; sinon aucun q2 ne convient.
  // M1 conserve alors q3/q4 exhaustifs et independants des positivites de leurs prefixes.
  // <=1+C(12,3)+C(12,4)=716 candidats, <=8592 inclusions, PLUS <=66 distances auxiliaires.
  static_assert(kMaxMebSites == 12 && kCoordBits <= 24);
  if (prepared.value().size == 1) MHGP11_TRY(search.consider(tuple, 1));
  else {
    MHGP11_TRY(search.select_diameter(tuple));
    MHGP11_TRY(search.consider(tuple, 2));
  }
  for (u8 q = 3; q <= 4 && q <= prepared.value().size && !search.best; ++q)
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
