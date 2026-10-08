// Etage M, premiere partie : numerotation canonique des naissances d'un ordre (contrat, paragraphe 1 ; CST-0107) par
// (rang, centre exact). Les naissances arrivent par cle croissante ; le catalogue range ses boules par niveau
// croissant, donc les rangs sont croissants au sens large et chaque cohorte de meme rang est contigue (controle). Seules
// les cohortes de plus d'une naissance sont triees : a l'ordre 1 par (x, y, z) des sites, ensuite par la comparaison
// exacte en deux temps des centres (num::compare_centers). Deux centres egaux au meme rang : refus tower_invariant
// (une meme sphere ne peut porter deux naissances d'un ordre). Port de number_births de MES-M4. Les cohortes sont
// independantes : la Session recouverte les trie par morceaux de naissances (number_births_range, levier N de T2-d-A6),
// chaque cohorte par le morceau ou elle commence, avec la meme comparaison.
#include <algorithm>

#include "tower/forest_internal.hpp"

namespace mhgp12::tower::detail {

Result<num::Sphere> ball_sphere(const Cloud& cloud, const BallSource& balls, u32 ball) noexcept {
  if (balls.support == nullptr || ball >= balls.balls) return fail(Reason::tower_invariant);
  const std::array<u32, 4> support = balls.support(balls.context, ball);
  std::array<num::Point, 4> points{};
  u32 q = 0;
  for (; q < 4 && support[q] != kNone; ++q) {
    const u32 s = support[q];
    if (s >= cloud.sites()) return fail(Reason::tower_invariant);
    auto p = num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]);
    if (!p.ok()) return p.outcome();
    points[q] = p.value();
  }
  if (q == 1) return num::Sphere::point(points[0]);
  auto made = q == 2   ? num::Sphere::through(points[0], points[1])
              : q == 3 ? num::Sphere::through(points[0], points[1], points[2])
              : q == 4 ? num::Sphere::through(points[0], points[1], points[2], points[3])
                       : Result<std::optional<num::Sphere>>(fail(Reason::tower_invariant));
  if (!made.ok()) return made.outcome();
  if (!made.value()) return fail(Reason::tower_invariant);
  return *made.value();
}

namespace {

// Cohorte de sites (ordre 1) : (x, y, z) lexicographique ; positions distinctes, donc jamais d'egalite.
Outcome sort_sites(const Cloud& cloud, const ForestInput& input, std::span<u32> cohort) noexcept {
  const auto x = cloud.x(), y = cloud.y(), z = cloud.z();
  for (u32 i : cohort)
    if (input.birth_key[i] >= cloud.sites()) return fail(Reason::tower_invariant);
  std::sort(cohort.begin(), cohort.end(), [&](u32 a, u32 b) {
    const u32 sa = input.birth_key[a], sb = input.birth_key[b];
    if (x[sa] != x[sb]) return x[sa] < x[sb];
    if (y[sa] != y[sb]) return y[sa] < y[sb];
    return z[sa] < z[sb];
  });
  for (std::size_t j = 1; j < cohort.size(); ++j) {
    const u32 a = input.birth_key[cohort[j - 1]], b = input.birth_key[cohort[j]];
    if (x[a] == x[b] && y[a] == y[b] && z[a] == z[b]) return fail(Reason::tower_invariant);
  }
  return {};
}

// Cohorte de boules (ordres 2 et plus) : centres exacts, comparaison en deux temps ; spheres calculees une fois dans
// spheres (au moins la taille de la cohorte), indices locaux tries dans local.
Outcome sort_balls(const Cloud& cloud, const BallSource& balls, const ForestInput& input, std::span<u32> cohort,
                   std::span<num::Sphere> spheres, std::span<u32> local) noexcept {
  const u32 n = static_cast<u32>(cohort.size());
  for (u32 j = 0; j < n; ++j) {
    auto sphere = ball_sphere(cloud, balls, input.birth_key[cohort[j]]);
    if (!sphere.ok()) return sphere.outcome();
    spheres[j] = sphere.value();
    local[j] = j;
  }
  // Comparateur sans effet de bord : un tri peut comparer un element a lui-meme (controle d'irreflexivite de
  // _GLIBCXX_DEBUG) ; les centres egaux, contigus apres le tri, sont cherches parmi les voisins, comme dans sort_sites
  // (recu comparateur_naissances de l'auditeur, 8 octobre).
  std::sort(local.begin(), local.begin() + n,
            [&](u32 a, u32 b) { return num::compare_centers(spheres[a], spheres[b]) < 0; });
  for (u32 j = 1; j < n; ++j)
    if (num::compare_centers(spheres[local[j - 1]], spheres[local[j]]) == 0) return fail(Reason::tower_invariant);
  // Les indices d'entree de la cohorte sont croissants : position locale j -> indice cohort[0] + j.
  const u32 first = cohort[0];
  for (u32 j = 0; j < n; ++j) cohort[j] = first + local[j];
  return {};
}

// Une cohorte de plus d'une naissance, triee : a l'ordre 1 par (x, y, z), ensuite par centre exact (tampons de la taille
// de la cohorte au moins).
Outcome sort_cohort(const Cloud& cloud, const BallSource& balls, const ForestInput& input, std::span<u32> cohort,
                    Buffer<num::Sphere>& spheres, Buffer<u32>& local) noexcept {
  if (input.k == 1) return sort_sites(cloud, input, cohort);
  else MHGP12_TRY(sort_balls(cloud, balls, input, cohort, spheres.span(), local.span()));
  return {};
}

// Fin de la cohorte qui commence en lo ; refus tower_invariant si le rang suivant decroit.
Result<u64> cohort_end(const ForestInput& input, u64 lo) noexcept {
  const u64 nb = input.birth_key.size();
  u64 hi = lo + 1;
  while (hi < nb && input.birth_rank[hi] == input.birth_rank[lo]) ++hi;
  if (hi < nb && input.birth_rank[hi] < input.birth_rank[lo]) return fail(Reason::tower_invariant);
  return hi;
}

}  // namespace

u64 widest_cohort(const ForestInput& input) noexcept {
  const u64 nb = input.birth_key.size();
  u64 widest = 1;
  for (u64 lo = 0; lo < nb;) {
    u64 hi = lo + 1;
    while (hi < nb && input.birth_rank[hi] == input.birth_rank[lo]) ++hi;
    widest = std::max(widest, hi - lo);
    lo = hi;
  }
  return widest;
}

Outcome number_births_range(const Cloud& cloud, const BallSource& balls, const ForestInput& input,
                            std::span<u32> order_out, u64 begin, u64 end, ForestWork& work,
                            MemoryBudget& budget) noexcept {
  const u64 nb = input.birth_key.size();
  if (order_out.size() != nb || end > nb || begin > end) return fail(Reason::tower_invariant);
  // premiere cohorte qui commence dans [begin, end) : la suite d'une cohorte commencee avant revient a son morceau
  u64 first = begin;
  while (first > 0 && first < end && input.birth_rank[first] == input.birth_rank[first - 1]) ++first;
  u64 widest = 1;
  for (u64 lo = first; lo < end;) {
    auto hi = cohort_end(input, lo);
    if (!hi.ok()) return hi.outcome();
    widest = std::max(widest, hi.value() - lo);
    lo = hi.value();
  }
  Buffer<num::Sphere> spheres;
  Buffer<u32> local;
  if (input.k >= 2 && widest > 1) {
    MHGP12_TRY(spheres.allocate(widest, budget));
    MHGP12_TRY(local.allocate(widest, budget));
  }
  for (u64 lo = first; lo < end;) {
    const u64 hi = cohort_end(input, lo).value();
    for (u64 v = lo; v < hi; ++v) order_out[v] = static_cast<u32>(v);
    if (hi - lo > 1) {
      ++work.cohorts;
      work.max_cohort = std::max<u64>(work.max_cohort, hi - lo);
      const std::span<u32> cohort = order_out.subspan(lo, hi - lo);
      MHGP12_TRY(sort_cohort(cloud, balls, input, cohort, spheres, local));
    }
    lo = hi;
  }
  return {};
}

Outcome number_births(const Cloud& cloud, const BallSource& balls, const ForestInput& input,
                      std::span<u32> order_out, std::span<u32> node_out, ForestWork& work,
                      MemoryBudget& budget) noexcept {
  const u32 nb = static_cast<u32>(input.birth_key.size());
  if (order_out.size() != nb || node_out.size() != nb) return fail(Reason::tower_invariant);
  u32 widest = 0;
  for (u32 lo = 0; lo < nb;) {
    u32 hi = lo + 1;
    while (hi < nb && input.birth_rank[hi] == input.birth_rank[lo]) ++hi;
    if (hi < nb && input.birth_rank[hi] < input.birth_rank[lo]) return fail(Reason::tower_invariant);
    widest = std::max(widest, hi - lo);
    lo = hi;
  }
  for (u32 i = 0; i < nb; ++i) order_out[i] = i;
  Buffer<num::Sphere> spheres;
  Buffer<u32> local;
  if (input.k >= 2 && widest > 1) {
    MHGP12_TRY(spheres.allocate(widest, budget));
    MHGP12_TRY(local.allocate(widest, budget));
  }
  for (u32 lo = 0; lo < nb;) {
    u32 hi = lo + 1;
    while (hi < nb && input.birth_rank[hi] == input.birth_rank[lo]) ++hi;
    if (hi - lo > 1) {
      ++work.cohorts;
      work.max_cohort = std::max<u64>(work.max_cohort, hi - lo);
      const std::span<u32> cohort = order_out.subspan(lo, hi - lo);
      MHGP12_TRY(sort_cohort(cloud, balls, input, cohort, spheres, local));
    }
    lo = hi;
  }
  for (u32 i = 0; i < nb; ++i) node_out[order_out[i]] = i;
  return {};
}

}  // namespace mhgp12::tower::detail
