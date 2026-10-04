// M1/M2 : identifier apres inclusion stricte certifiee ; un support local absent n'est pas un certificat global.
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

}  // namespace mhgp11::tower_detail
