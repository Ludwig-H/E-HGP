// Canonicalisation apres census : coquille COMPLETE, cardinal minimal puis ordre lexicographique des SiteIdx.
// Port de support.hpp R2, sans repli silencieux sur la presentation generatrice en cas d'invariant viole.
#include "catalogue/internal.hpp"

namespace mhgp11::catalogue_detail {
namespace {

using Support = std::array<SiteIdx, 4>;
using OptionalSupport = std::optional<Support>;

Support empty_support() noexcept {
  const SiteIdx none = make_id<SiteIdx>(kNone);
  return {none, none, none, none};
}

template <class Ball>
Result<OptionalSupport> pair_support(const Cloud& cloud, std::span<const SiteIdx> shell,
                                     const Ball& sphere) noexcept {
  for (u32 i = 0; i < shell.size(); ++i) {
    const auto a = point(cloud, shell[i]);
    if (!a.ok()) return a.outcome();
    for (u32 j = i + 1; j < shell.size(); ++j) {
      const auto b = point(cloud, shell[j]);
      if (!b.ok()) return b.outcome();
      if (num::is_midpoint(sphere, a.value(), b.value())) {
        auto support = empty_support();
        support[0] = shell[i];
        support[1] = shell[j];
        return OptionalSupport{support};
      }
    }
  }
  return OptionalSupport{};
}

template <class Ball>
Result<OptionalSupport> triangle_support(const Cloud& cloud, std::span<const SiteIdx> shell,
                                         const Ball& sphere) noexcept {
  for (u32 i = 0; i < shell.size(); ++i) {
    const auto a = point(cloud, shell[i]);
    if (!a.ok()) return a.outcome();
    for (u32 j = i + 1; j < shell.size(); ++j) {
      const auto b = point(cloud, shell[j]);
      if (!b.ok()) return b.outcome();
      for (u32 k = j + 1; k < shell.size(); ++k) {
        const auto c = point(cloud, shell[k]);
        if (!c.ok()) return c.outcome();
        if (!num::strictly_acute(a.value(), b.value(), c.value())) continue;
        const auto plane = num::orientation(a.value(), b.value(), c.value(), sphere);
        if (!plane.ok()) return plane.outcome();
        if (plane.value() == 0) {
          auto support = empty_support();
          support[0] = shell[i];
          support[1] = shell[j];
          support[2] = shell[k];
          return OptionalSupport{support};
        }
      }
    }
  }
  return OptionalSupport{};
}

template <class Ball>
Result<OptionalSupport> tetra_support(const Cloud& cloud, std::span<const SiteIdx> shell,
                                      const Ball& sphere) noexcept {
  for (u32 i = 0; i < shell.size(); ++i) {
    const auto a = point(cloud, shell[i]);
    if (!a.ok()) return a.outcome();
    for (u32 j = i + 1; j < shell.size(); ++j) {
      const auto b = point(cloud, shell[j]);
      if (!b.ok()) return b.outcome();
      for (u32 k = j + 1; k < shell.size(); ++k) {
        const auto c = point(cloud, shell[k]);
        if (!c.ok()) return c.outcome();
        for (u32 l = k + 1; l < shell.size(); ++l) {
          const auto d = point(cloud, shell[l]);
          if (!d.ok()) return d.outcome();
          const auto inside = num::strictly_inside(sphere, a.value(), b.value(), c.value(), d.value());
          if (!inside.ok()) return inside.outcome();
          if (inside.value()) return OptionalSupport{Support{shell[i], shell[j], shell[k], shell[l]}};
        }
      }
    }
  }
  return OptionalSupport{};
}

template <class Ball>
Result<std::array<SiteIdx, 4>> canonical_support_impl(const Cloud& cloud, std::span<const SiteIdx> shell,
                                               const Ball& sphere, u8& qmin) noexcept {
  const auto pair = pair_support(cloud, shell, sphere);
  if (!pair.ok()) return pair.outcome();
  if (pair.value()) {
    qmin = 2;
    return *pair.value();
  }
  const auto triangle = triangle_support(cloud, shell, sphere);
  if (!triangle.ok()) return triangle.outcome();
  if (triangle.value()) {
    qmin = 3;
    return *triangle.value();
  }
  const auto tetra = tetra_support(cloud, shell, sphere);
  if (!tetra.ok()) return tetra.outcome();
  if (tetra.value()) {
    qmin = 4;
    return *tetra.value();
  }
  return fail(Reason::catalogue_invariant);
}

}  // namespace

Result<std::array<SiteIdx, 4>> canonical_support(const Cloud& cloud, std::span<const SiteIdx> shell,
                                               const num::Sphere& sphere, u8& qmin) noexcept {
  return canonical_support_impl(cloud, shell, sphere, qmin);
}

Result<std::array<SiteIdx, 4>> canonical_support(const Cloud& cloud, std::span<const SiteIdx> shell,
                                               const num::Q4Candidate& sphere, u8& qmin) noexcept {
  return canonical_support_impl(cloud, shell, sphere, qmin);
}

}  // namespace mhgp11::catalogue_detail
