// Controles du banc MEB : scan repris explicitement du banc index, meme num::side partage.
#pragma once
#include "index_io.hpp"
#include "tower/tower.hpp"

namespace meb_bench {
using namespace mhgp11;
num::Point point(const Cloud& cloud, u32 s) {
  return num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]).value();
}

bool ordered(std::span<const SiteIdx> sites, u32 size) {
  for (std::size_t i = 0; i < sites.size(); ++i)
    if (idx(sites[i]) >= size || (i > 0 && idx(sites[i - 1]) >= idx(sites[i]))) return false;
  return true;
}

Outcome scan(const Cloud& cloud, const num::Sphere& query, u32 threshold, const Census& result) {
  const auto inner = result.interior(), shell = result.shell();
  if (!ordered(inner, cloud.sites()) || !ordered(shell, cloud.sites())) return fail(Reason::arithmetic_invariant);
  const bool saturated = result.kind() == CensusKind::saturated;
  u64 pi = 0, pu = 0, witness = 0;
  for (u32 s = 0; s < cloud.sites(); ++s) {
    auto side = num::side(query, point(cloud, s));
    if (!side.ok()) return side.outcome();
    if (side.value() < 0) {
      if (!saturated && (pi >= inner.size() || idx(inner[pi]) != s)) return fail(Reason::arithmetic_invariant);
      ++pi;
      witness += std::binary_search(inner.begin(), inner.end(), make_id<SiteIdx>(s));
    } else if (side.value() == 0) {
      if (!saturated && (pu >= shell.size() || idx(shell[pu]) != s)) return fail(Reason::arithmetic_invariant);
      ++pu;
    }
  }
  const bool valid = saturated ? pi >= threshold && inner.size() == threshold && shell.empty() && witness == threshold
                               : pi < threshold && pi == inner.size() && pu == shell.size();
  return valid ? Outcome{} : fail(Reason::arithmetic_invariant);
}

Outcome support_check(const Cloud& cloud, std::span<const SiteIdx> part, const BoundedMeb& meb) {
  const auto support = meb.support();
  const auto& ball = meb.sphere();
  if (support.empty() || support.size() > 4 || !ordered(support, cloud.sites()))
    return fail(Reason::arithmetic_invariant);
  for (SiteIdx s : support) {
    if (std::find(part.begin(), part.end(), s) == part.end()) return fail(Reason::arithmetic_invariant);
    const auto side = num::side(ball, point(cloud, idx(s)));
    if (!side.ok()) return side.outcome();
    if (side.value() != 0) return fail(Reason::arithmetic_invariant);
  }
  for (SiteIdx s : part) {
    const auto side = num::side(ball, point(cloud, idx(s)));
    if (!side.ok()) return side.outcome();
    if (side.value() > 0) return fail(Reason::arithmetic_invariant);
  }
  const auto a = point(cloud, idx(support[0]));
  if (support.size() == 1)
    return num::compare(ball.level(), num::Level{}) == 0 ? Outcome{} : fail(Reason::arithmetic_invariant);
  const auto b = point(cloud, idx(support[1]));
  if (support.size() == 2)
    return num::is_midpoint(ball, a, b) ? Outcome{} : fail(Reason::arithmetic_invariant);
  const auto c = point(cloud, idx(support[2]));
  if (support.size() == 3) {
    const auto plane = num::orientation(a, b, c, ball);
    if (!plane.ok()) return plane.outcome();
    return num::strictly_acute(a, b, c) && plane.value() == 0 ? Outcome{} : fail(Reason::arithmetic_invariant);
  }
  const auto inside = num::strictly_inside(ball, a, b, c, point(cloud, idx(support[3])));
  if (!inside.ok()) return inside.outcome();
  return inside.value() ? Outcome{} : fail(Reason::arithmetic_invariant);
}

bool same_meb(const BoundedMeb& a, const BoundedMeb& b) {
  if (!std::equal(a.support().begin(), a.support().end(), b.support().begin(), b.support().end()) ||
      !(a.ledger() == b.ledger())) return false;
  const auto& x = a.sphere(); const auto& y = b.sphere();
  if (x.anchor() != y.anchor() || num::compare(x.level(), y.level()) != 0 ||
      num::compare(num::to_wide(x.denominator()), num::to_wide(y.denominator())) != 0) return false;
  for (std::size_t i = 0; i < 3; ++i)
    if (num::compare(num::to_wide(x.numerator()[i]), num::to_wide(y.numerator()[i])) != 0) return false;
  return true;
}

bool same_census(const Census& a, const Census& b) {
  return a.kind() == b.kind() && a.ledger() == b.ledger() &&
         std::equal(a.interior().begin(), a.interior().end(), b.interior().begin(), b.interior().end()) &&
         std::equal(a.shell().begin(), a.shell().end(), b.shell().begin(), b.shell().end());
}
}  // namespace meb_bench
