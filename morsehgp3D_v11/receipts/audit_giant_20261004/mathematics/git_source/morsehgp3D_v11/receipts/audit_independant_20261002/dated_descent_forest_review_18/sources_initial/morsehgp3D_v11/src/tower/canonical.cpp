// Port explicite des boucles de catalogue/support.cpp v11 ffc2ff95f (source R2 support.hpp 7cc62930).
// Prive a tower : seul le census complet du domaine est consomme. Aucun include interne entre modules.
#include "tower/locate.hpp"

namespace mhgp11::tower_detail {
namespace {

Result<num::Point> point(const FullDomain& domain, SiteIdx id) noexcept {
  const auto& cloud = domain.index().cloud();
  if (idx(id) >= cloud.sites()) return fail(Reason::tower_invariant);
  return num::Point::make(cloud.x()[idx(id)], cloud.y()[idx(id)], cloud.z()[idx(id)]);
}

// La coquille complete est triee par SiteIdx. Le premier support strict a arite minimale est donc S*.
Result<std::optional<SupportKey>> positive_support(const FullDomain& domain, const num::Sphere& sphere,
                                                  std::span<const SiteIdx> shell, u8 q) noexcept {
  const SiteIdx none{kNone};
  for (u32 i = 0; i < shell.size(); ++i) {
    const auto a = point(domain, shell[i]);
    if (!a.ok()) return a.outcome();
    for (u32 j = i + 1; j < shell.size(); ++j) {
      const auto b = point(domain, shell[j]);
      if (!b.ok()) return b.outcome();
      if (q == 2) {
        if (num::is_midpoint(sphere, a.value(), b.value()))
          return std::optional<SupportKey>{SupportKey{{shell[i], shell[j], none, none}, q}};
        continue;
      }
      for (u32 k = j + 1; k < shell.size(); ++k) {
        const auto c = point(domain, shell[k]);
        if (!c.ok()) return c.outcome();
        if (q == 3) {
          if (!num::strictly_acute(a.value(), b.value(), c.value())) continue;
          const auto plane = num::orientation(a.value(), b.value(), c.value(), sphere);
          if (!plane.ok()) return plane.outcome();
          if (plane.value() == 0)
            return std::optional<SupportKey>{SupportKey{{shell[i], shell[j], shell[k], none}, q}};
          continue;
        }
        for (u32 l = k + 1; l < shell.size(); ++l) {
          const auto d = point(domain, shell[l]);
          if (!d.ok()) return d.outcome();
          const auto inside = num::strictly_inside(sphere, a.value(), b.value(), c.value(), d.value());
          if (!inside.ok()) return inside.outcome();
          if (inside.value())
            return std::optional<SupportKey>{SupportKey{{shell[i], shell[j], shell[k], shell[l]}, q}};
        }
      }
    }
  }
  return std::optional<SupportKey>{};
}
}  // namespace

Result<SupportKey> global_support(const FullDomain& domain, const BoundedMeb& meb,
                                  const Census& population) noexcept {
  if (population.kind() != CensusKind::complete || population.shell().empty())
    return fail(Reason::tower_invariant);
  const auto shell = population.shell();
  if (meb.support().size() == 1) {
    if (shell.size() != 1 || shell[0] != meb.support()[0]) return fail(Reason::tower_invariant);
    return SupportKey{{shell[0], SiteIdx{kNone}, SiteIdx{kNone}, SiteIdx{kNone}}, 1};
  }
  for (u8 q = 2; q <= 4; ++q) {
    const auto result = positive_support(domain, meb.sphere(), shell, q);
    if (!result.ok()) return result.outcome();
    if (result.value()) return *result.value();
  }
  return fail(Reason::tower_invariant);
}

}  // namespace mhgp11::tower_detail
