// Supports et certificats exacts de l'etage G (CONTRAT_TOUR.md, paragraphes 4.1 et 6 ; CONTRAT_NUMERIQUE.md,
// paragraphes 2 et 4). Support canonique de la v12 par POSITIONS (CST-0113 ; meme convention que S* du catalogue,
// src/catalogue/leaf_census.hpp, canonical) ; supports d'une coquille (temoins de non-separabilite) ; certificat exact
// d'un support propose pour une partie (port de certify et finish_certificate de
// microbancs/mes_m3_m4_tour/mes_m3/meb_cert.hpp, MES-M3 adopte) ; repli exact (port de bounded_meb de la v11 gelee,
// src/tower/meb.cpp, ac081a06f : diametre, puis triangles, puis tetraedres). Aucune decision en flottant.
#include <algorithm>

#include "tower/internal.hpp"

namespace mhgp12::tower_detail {
namespace {

bool position_less(const num::Point& a, const num::Point& b) noexcept { return a.coordinates() < b.coordinates(); }

// Rangement par positions de sites tous distincts (insertion : au plus kMaxShell sites).
void sort_by_position(const Domain& d, std::span<u32> sites) noexcept {
  for (std::size_t i = 1; i < sites.size(); ++i)
    for (std::size_t j = i; j > 0 && position_less(d.points[sites[j]], d.points[sites[j - 1]]); --j)
      std::swap(sites[j], sites[j - 1]);
}

void store(std::array<u32, 4>& out, std::initializer_list<u32> sites) noexcept {
  out = {kNone, kNone, kNone, kNone};
  std::size_t i = 0;
  for (const u32 s : sites) out[i++] = s;
  std::sort(out.begin(), out.begin() + static_cast<std::ptrdiff_t>(sites.size()));
}

Result<bool> is_triangle_support(const Domain& d, const num::Sphere& sphere, u32 a, u32 b, u32 c) noexcept {
  const auto& P = d.points;
  if (num::classify_triangle(P[a], P[b], P[c]) != num::TriangleKind::strict) return false;
  auto plane = num::orientation(P[a], P[b], P[c], sphere);
  if (!plane.ok()) return plane.outcome();
  return plane.value() == 0;
}

// Plus petite boule de quatre sites deja certifiee non degeneree ; contient-elle toute la partie ?
template <class Ball>
Result<bool> contains_part(const Domain& d, const Ball& ball, const Part& f) noexcept {
  for (u32 i = 0; i < f.k; ++i) {
    auto side = num::side(ball, d.points[f.id[i]]);
    if (!side.ok()) return side.outcome();
    if (side.value() > 0) return false;
  }
  return true;
}

}  // namespace

Result<u8> canonical_support(const Domain& d, const num::Sphere& sphere, std::span<const u32> sites,
                             std::array<u32, 4>& support) noexcept {
  if (sites.size() > kMaxShell) return fail(Reason::shell_capacity);
  std::array<u32, kMaxShell> order{};
  std::copy(sites.begin(), sites.end(), order.begin());
  const std::span<u32> z(order.data(), sites.size());
  sort_by_position(d, z);
  const auto& P = d.points;
  const std::size_t n = z.size();
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = i + 1; j < n; ++j)
      if (num::is_midpoint(sphere, P[z[i]], P[z[j]])) {
        store(support, {z[i], z[j]});
        return u8{2};
      }
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = i + 1; j < n; ++j)
      for (std::size_t l = j + 1; l < n; ++l) {
        auto found = is_triangle_support(d, sphere, z[i], z[j], z[l]);
        if (!found.ok()) return found.outcome();
        if (found.value()) {
          store(support, {z[i], z[j], z[l]});
          return u8{3};
        }
      }
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = i + 1; j < n; ++j)
      for (std::size_t l = j + 1; l < n; ++l)
        for (std::size_t h = l + 1; h < n; ++h) {
          auto inside = num::strictly_inside(sphere, P[z[i]], P[z[j]], P[z[l]], P[z[h]]);
          if (!inside.ok()) return inside.outcome();
          if (inside.value()) {
            store(support, {z[i], z[j], z[l], z[h]});
            return u8{4};
          }
        }
  return u8{0};
}

Result<u32> shell_witnesses(const Domain& d, const num::Sphere& sphere, std::span<const SiteIdx> shell,
                            std::span<u64> out) noexcept {
  const u32 m = static_cast<u32>(shell.size());
  if (m > kMaxShell) return fail(Reason::shell_capacity);
  const auto& P = d.points;
  u32 count = 0;
  auto push = [&](u64 mask) noexcept {
    if (count >= out.size()) return false;
    out[count++] = mask;
    return true;
  };
  auto at = [&](u32 j) noexcept { return P[idx(shell[j])]; };
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      if (num::is_midpoint(sphere, at(i), at(j)) && !push((u64{1} << i) | (u64{1} << j)))
        return fail(Reason::cell_capacity);
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 l = j + 1; l < m; ++l) {
        auto found = is_triangle_support(d, sphere, idx(shell[i]), idx(shell[j]), idx(shell[l]));
        if (!found.ok()) return found.outcome();
        if (found.value() && !push((u64{1} << i) | (u64{1} << j) | (u64{1} << l))) return fail(Reason::cell_capacity);
      }
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 l = j + 1; l < m; ++l)
        for (u32 h = l + 1; h < m; ++h) {
          auto inside = num::strictly_inside(sphere, at(i), at(j), at(l), at(h));
          if (!inside.ok()) return inside.outcome();
          if (inside.value() && !push((u64{1} << i) | (u64{1} << j) | (u64{1} << l) | (u64{1} << h)))
            return fail(Reason::cell_capacity);
        }
  if (count == 0) return fail(Reason::tower_invariant);  // le centre est dans l'enveloppe de sa coquille
  return count;
}

Result<num::CertifiedBall> catalogue_sphere(const Domain& d, u32 ball) noexcept {
  const auto& data = d.catalogue.balls_data()[ball];
  if (data.qmin < 2 || data.qmin > 4) return fail(Reason::tower_invariant);
  std::array<num::Point, 4> points{};
  for (u32 i = 0; i < data.qmin; ++i) points[i] = d.points[idx(data.support[i])];
  auto made = num::CertifiedBall::certify(std::span<const num::Point>(points.data(), data.qmin));
  if (!made.ok()) return made.outcome();
  if (!made.value()) return fail(Reason::tower_invariant);  // S* du catalogue : support strict
  return *made.value();
}

Result<std::optional<Certified>> certify_part(const Domain& d, const Part& f, std::span<const u32> support) noexcept {
  std::array<num::Point, 4> points{};
  for (std::size_t i = 0; i < support.size(); ++i) points[i] = d.points[support[i]];
  auto made = num::CertifiedBall::certify(std::span<const num::Point>(points.data(), support.size()));
  if (!made.ok()) return made.outcome();
  if (!made.value()) return std::optional<Certified>{};  // degenere, ou centre hors de l'enveloppe ouverte
  const num::Sphere& sphere = made.value()->sphere();
  std::array<u32, kMaxPart> on{};
  u32 count = 0;
  std::size_t next_support = 0;
  for (u32 i = 0; i < f.k; ++i) {
    // S trie inclus dans F (les deux appelants : proposition apres sorted_subset, repli exact_support) et sphere
    // certifiee passant par S : ses sites sont sur la sphere, sans predicat (proposition de l'auditeur, recu
    // audit_g_pistes_20261007 : au plus q appels evites, memes sites << sur >>, meme ordre, meme canonisation).
    if (next_support < support.size() && f.id[i] == support[next_support]) {
      on[count++] = f.id[i];
      ++next_support;
      continue;
    }
    auto side = num::side(sphere, d.points[f.id[i]]);
    if (!side.ok()) return side.outcome();
    if (side.value() > 0) return std::optional<Certified>{};  // un site de F sort de la boule proposee
    if (side.value() == 0) on[count++] = f.id[i];
  }
  Certified out{*made.value(), {kNone, kNone, kNone, kNone}, static_cast<u8>(support.size())};
  if (count == support.size()) {  // F sur la sphere = S : S est l'unique support parmi eux
    std::copy(support.begin(), support.end(), out.support.begin());
    return std::optional<Certified>{out};
  }
  auto arity = canonical_support(d, sphere, std::span<const u32>(on.data(), count), out.support);
  if (!arity.ok()) return arity.outcome();
  if (arity.value() == 0) return fail(Reason::tower_invariant);  // S lui-meme est un support parmi eux
  out.arity = arity.value();
  return std::optional<Certified>{out};
}

Result<u8> exact_support(const Domain& d, const Part& f, std::array<u32, 4>& support) noexcept {
  const auto& P = d.points;
  // Diametre : la premiere paire (ordre lexicographique de F) de distance maximale ; si sa boule contient F, c'est la
  // plus petite boule ; sinon aucune paire ne convient (meb.cpp de la v11).
  u32 a = 0, b = 1;
  num::DotInt longest = 0;
  for (u32 i = 0; i + 1 < f.k; ++i)
    for (u32 j = i + 1; j < f.k; ++j) {
      const auto distance = num::squared_distance(P[f.id[i]], P[f.id[j]]);
      if (distance > longest) {
        longest = distance;
        a = i;
        b = j;
      }
    }
  auto pair = num::Sphere::through(P[f.id[a]], P[f.id[b]]);
  if (!pair.ok()) return pair.outcome();
  if (!pair.value()) return fail(Reason::tower_invariant);  // sites distincts
  auto inside = contains_part(d, *pair.value(), f);
  if (!inside.ok()) return inside.outcome();
  if (inside.value()) {
    store(support, {f.id[a], f.id[b]});
    return u8{2};
  }
  for (u32 i = 0; i < f.k; ++i)
    for (u32 j = i + 1; j < f.k; ++j)
      for (u32 l = j + 1; l < f.k; ++l) {
        if (num::classify_triangle(P[f.id[i]], P[f.id[j]], P[f.id[l]]) != num::TriangleKind::strict) continue;
        auto made = num::Q3Candidate::through(P[f.id[i]], P[f.id[j]], P[f.id[l]]);
        if (!made.ok()) return made.outcome();
        if (!made.value()) return fail(Reason::tower_invariant);  // triangle strict : non degenere
        auto holds = contains_part(d, *made.value(), f);
        if (!holds.ok()) return holds.outcome();
        if (holds.value()) {
          store(support, {f.id[i], f.id[j], f.id[l]});
          return u8{3};
        }
      }
  for (u32 i = 0; i < f.k; ++i)
    for (u32 j = i + 1; j < f.k; ++j)
      for (u32 l = j + 1; l < f.k; ++l)
        for (u32 h = l + 1; h < f.k; ++h) {
          auto made = num::Q4Candidate::through(P[f.id[i]], P[f.id[j]], P[f.id[l]], P[f.id[h]]);
          if (!made.ok()) return made.outcome();
          if (!made.value() || !made.value()->q4_presentation_strictly_inside()) continue;
          auto holds = contains_part(d, *made.value(), f);
          if (!holds.ok()) return holds.outcome();
          if (holds.value()) {
            store(support, {f.id[i], f.id[j], f.id[l], f.id[h]});
            return u8{4};
          }
        }
  return fail(Reason::tower_invariant);  // une partie de sites distincts a toujours une plus petite boule
}

}  // namespace mhgp12::tower_detail
