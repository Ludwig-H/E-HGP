// M1/M2 : identifier apres inclusion stricte certifiee ; un support local absent n'est pas un certificat global.
#include <algorithm>

#include "tower/locate.hpp"

namespace mhgp11::tower_detail {

Result<LocatedPart> locate_part(const FullDomain& domain, std::span<const SiteIdx> part, u32 k,
                               MemoryBudget& budget) noexcept {
  if (k == 0 || k > static_cast<u32>(domain.catalogue().kmax())) return fail(Reason::kmax_out_of_range);
  if (part.size() != k) return fail(Reason::parameter_out_of_range);
  auto made = bounded_meb(domain.index().cloud(), part);
  if (!made.ok()) return made.outcome();
  auto& meb = made.value();
  SupportKey local{{SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}},
                   static_cast<u8>(meb.support().size())};
  std::copy(meb.support().begin(), meb.support().end(), local.sites.begin());
  if (const auto found = domain.find_support(local.sites))
    return LocatedPart(std::move(meb), domain, *found);
  auto population = census(domain.index(), meb.sphere(), k, budget);
  if (!population.ok()) return population.outcome();
  if (population.value().kind() == CensusKind::saturated)
    return LocatedPart(std::move(meb), std::move(population.value()), std::nullopt, std::nullopt);
  const auto support = global_support(domain, meb, population.value());
  if (!support.ok()) return support.outcome();
  const auto found = domain.find_support(support.value().sites);
  // Une boule positive admissible doit etre dans CatK. q1 est traite par les naissances de niveau zero.
  if (!found && support.value().arity > 1 &&
      population.value().interior().size() + support.value().arity <= u64(domain.catalogue().kmax()) + 1)
    return fail(Reason::tower_invariant);
  return LocatedPart(std::move(meb), std::move(population.value()), support.value(), found);
}

namespace {
struct LocatedQuery {
  const FullDomain& domain;
  const BoundedMeb& meb;
  void* context;
  LocatedCallback callback;
  static Outcome consume(void* raw, const BorrowedCensus& population) noexcept {
    const auto& q = *static_cast<LocatedQuery*>(raw);
    std::optional<SupportKey> key;
    std::optional<BallIdx> ball;
    if (population.kind() == CensusKind::complete) {
      auto found = global_support(q.domain, q.meb, population);
      if (!found.ok()) return found.outcome();
      key = found.value(); ball = q.domain.find_support(key->sites);
      if (!ball && key->arity > 1 &&
          population.interior().size() + key->arity <= u64{q.domain.catalogue().kmax()} + 1)
        return fail(Reason::tower_invariant);
    }
    const LocatedView view{q.meb, ball, key, population.kind(), population.interior(), population.shell(),
                           &population.ledger()};
    return q.callback(q.context, view);
  }
};
}  // namespace

Outcome visit_located_part(const FullDomain& domain, std::span<const SiteIdx> part, u32 k,
                           MemoryBudget& budget, CensusWorkspace* scratch, void* context,
                           LocatedCallback callback) noexcept {
  if (callback == nullptr || (scratch != nullptr && !scratch->belongs_to(domain.index())))
    return fail(Reason::parameter_out_of_range);
  if (scratch == nullptr) {
    auto located = locate_part(domain, part, k, budget);
    if (!located.ok()) return located.outcome();
    const auto& p = located.value();
    const LocatedView view{p.meb(), p.ball(), p.support(), p.kind(), p.interior(), p.shell(), p.census_work()};
    return callback(context, view);
  }
  if (k == 0 || k > static_cast<u32>(domain.catalogue().kmax())) return fail(Reason::kmax_out_of_range);
  if (part.size() != k) return fail(Reason::parameter_out_of_range);
  auto made = bounded_meb(domain.index().cloud(), part);
  if (!made.ok()) return made.outcome();
  const auto& meb = made.value();
  SupportKey local{{SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}},
                   static_cast<u8>(meb.support().size())};
  std::copy(meb.support().begin(), meb.support().end(), local.sites.begin());
  if (const auto found = domain.find_support(local.sites)) {
    const auto& data = domain.catalogue().balls_data()[idx(*found)];
    const LocatedView view{meb, found, SupportKey{data.support, data.qmin}, CensusKind::complete,
                           domain.catalogue().interior(*found), domain.catalogue().shell(*found), nullptr};
    return callback(context, view);
  }
  LocatedQuery query{domain, meb, context, callback};
  return scratch->query(domain.index(), meb.sphere(), k, &query, LocatedQuery::consume);
}

}  // namespace mhgp11::tower_detail
