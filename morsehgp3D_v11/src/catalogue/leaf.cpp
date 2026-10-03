// Feuilles bornees : dominateurs distincts, DFS de supports, census local exact G2 et emission de S* seulement.
// Les triplets obtus restent des prefixes q4. Memo J2 optionnel borne, jamais memo de boules par candidat.
#include <bit>

#include "catalogue/internal.hpp"
#include "catalogue/center_line_cache.hpp"
#include "catalogue/small_pair_graph.hpp"

namespace mhgp11::catalogue_detail {
namespace {

struct Leaf {
  Run& run;
  std::span<const SiteIdx> sites;
  const Box& box;
  CenterLineCache& lines;
  const SmallPairGraph& pairs;
  u32 words;
  std::array<u32, 4> prefix{};
  std::array<std::array<u64, kMaxWords>, 5> masks{};  // borne constante : 5*16 mots, profondeur <=4
};

Outcome prepare(Run& run, std::span<const SiteIdx> sites, const Box& box, u32 words,
                SmallPairGraph& pairs) noexcept {
  auto& work = run.workspace;
  const u32 m = static_cast<u32>(sites.size());
  if (m > work.points.size() || u64(m) * words > work.dominance.size()) return fail(Reason::catalogue_invariant);
  std::fill_n(work.dominance.data(), u64(m) * words, u64{0});
  for (u32 i = 0; i < m; ++i) {
    const auto p = point(run.cloud, sites[i]);
    if (!p.ok()) return p.outcome();
    work.points[i] = p.value();
  }
  // Forme affine de difference des distances ; chaque somme partielle < 12*2^(2B), en i64.
  static_assert(2 * kCoordBits + 5 <= 63, "catalogue : dominance fermee T0 en i64");
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j) {
      MHGP11_TRY(checked_add(run.ledger.dominance_tests, 1));
      i64 base = 0, cmin = 0, cmax = 0;
      const auto x = work.points[i].coordinates(), y = work.points[j].coordinates();
      for (int axis = 0; axis < 3; ++axis) {
        const i64 delta = i64(y[axis]) - x[axis];
        base += i64(y[axis]) * y[axis] - i64(x[axis]) * x[axis];
        cmin += (delta > 0 ? box.lo[axis] : box.hi[axis]) * delta;
        cmax += (delta > 0 ? box.hi[axis] : box.lo[axis]) * delta;
      }
      if (base - 2 * cmin < 0) work.dominance[u64(i) * words + j / 64] |= u64{1} << (j % 64);
      else if (base - 2 * cmax > 0) work.dominance[u64(j) * words + i / 64] |= u64{1} << (i % 64);
      else if (pairs.enabled()) pairs.connect(i, j);
    }
  return {};
}

Result<bool> center_region_possible(Leaf& leaf, u32 q) noexcept {
  auto& ledger = leaf.run.ledger;
  const auto& work = leaf.run.workspace;
  const u32 last = leaf.prefix[q - 1];
  // J2 : une dominance stricte dans un sens equivaut a une bissectrice disjointe de la fermeture.
  // La voie graphe certifie ces couples par intersection ; le repli conserve les lectures historiques.
  for (u32 j = 0; !leaf.pairs.enabled() && j + 1 < q; ++j) {
    const u32 first = leaf.prefix[j];
    MHGP11_TRY(checked_add(ledger.region_pair_tests, 1));
    const bool forward = (work.dominance[u64(first) * leaf.words + last / 64] >> (last % 64)) & 1;
    const bool backward = (work.dominance[u64(last) * leaf.words + first / 64] >> (first % 64)) & 1;
    if (forward || backward) {
      MHGP11_TRY(checked_add(ledger.region_pair_rejects, 1));
      return false;
    }
  }
  // J2 : chaque face doit avoir sa droite de centres dans la fermeture. Un triplet aligne ne peut
  // appartenir a aucun support affine independant. Aucune condition d'angle n'intervient ici.
  for (u32 j = 0; j + 2 < q; ++j)
    for (u32 k = j + 1; k + 1 < q; ++k) {
      MHGP11_TRY(checked_add(ledger.region_line_tests, 1));
      const auto query = leaf.lines.lookup(leaf.prefix[j], leaf.prefix[k], last);
      if (!query.ok()) return query.outcome();
      const auto reply = query.value();
      MHGP11_TRY(checked_add(ledger.region_line_evaluations, reply.hit ? 0u : 1u));
      MHGP11_TRY(checked_add(ledger.region_line_cache_hits, reply.hit ? 1u : 0u));
      MHGP11_TRY(checked_add(ledger.region_line_fallbacks, reply.fallback ? 1u : 0u));
      const auto relation = reply.relation;
      if (relation != num::CenterLineRelation::intersects) {
        MHGP11_TRY(checked_add(ledger.region_line_rejects, 1));
        return false;
      }
    }
  return true;
}

Result<std::optional<num::Sphere>> sphere_of(Leaf& leaf, u32 q) noexcept {
  const auto& p = leaf.run.workspace.points;
  const auto a = p[leaf.prefix[0]], b = p[leaf.prefix[1]];
  if (q == 2) return num::Sphere::through(a, b);
  const auto c = p[leaf.prefix[2]];
  if (!num::strictly_acute(a, b, c)) return std::optional<num::Sphere>{};
  return num::Sphere::through(a, b, c);
}

Result<std::optional<num::Q4Candidate>> q4_of(Leaf& leaf) noexcept {
  const auto& p = leaf.run.workspace.points;
  const auto a = p[leaf.prefix[0]], b = p[leaf.prefix[1]];
  const auto c = p[leaf.prefix[2]], d = p[leaf.prefix[3]];
  auto result = num::Q4Candidate::through(a, b, c, d);
  if (!result.ok()) return result.outcome();
  if (result.value()) {
    MHGP11_TRY(checked_add(leaf.run.ledger.q4_candidates, 1));
    // Meme tuple que through ci-dessus : le flag ne remplace pas le predicat generique de canonical_support.
    if (!result.value()->q4_presentation_strictly_inside()) return std::optional<num::Q4Candidate>{};
  }
  return result;
}

Result<num::Level> emission_level(const num::Sphere& sphere, CatalogueLedger&) noexcept {
  return sphere.level();
}

Result<num::Level> emission_level(const num::Q4Candidate& sphere, CatalogueLedger& ledger) noexcept {
  const auto full = sphere.materialize();
  if (!full.ok()) return full.outcome();
  MHGP11_TRY(checked_add(ledger.q4_levels, 1));
  return full.value().level();
}

template <class Ball>
Outcome census_and_emit(Leaf& leaf, u32 q, const Ball& sphere) noexcept {
  auto& run = leaf.run;
  auto& work = run.workspace;
  MHGP11_TRY(checked_add(run.ledger.judged, 1));
  const u32 threshold = static_cast<u32>(run.params.kmax + 1) - q;  // appele seulement si q<=K+1
  u32 p = 0, m = 0;
  for (u32 i = 0; i < leaf.sites.size(); ++i) {
    MHGP11_TRY(checked_add(run.ledger.census_tests, 1));
    const auto side = num::side(sphere, work.points[i]);
    if (!side.ok()) return side.outcome();
    if (side.value() < 0) {
      if (p == threshold) return {};  // le prochain interieur donne p>theta_q ; aucun census accepte tronque
      work.interior[p++] = leaf.sites[i];
    } else if (side.value() == 0) {
      work.shell[m++] = leaf.sites[i];
    }
  }
  if (m < q) return fail(Reason::catalogue_invariant);
  const SiteIdx none = make_id<SiteIdx>(kNone);
  std::array<SiteIdx, 4> generated{none, none, none, none};
  for (u32 i = 0; i < q; ++i) generated[i] = leaf.sites[leaf.prefix[i]];
  u8 qmin = static_cast<u8>(q);
  auto support = generated;
  if (m != q) {
    const auto canonical = canonical_support(run.cloud, work.shell.span().first(m), sphere, qmin);
    if (!canonical.ok()) return canonical.outcome();
    support = canonical.value();
  }
  if (support != generated) return {};  // S* sera visite dans cette meme boite ; aucun memo necessaire.
  if (p + qmin > static_cast<u32>(run.params.kmax) + 1) return {};
  const CatalogueBall ball{support, make_id<LevelRank>(0), p, m, qmin};
  const auto level = emission_level(sphere, run.ledger);
  if (!level.ok()) return level.outcome();
  MHGP11_TRY(run.collector.accept(ball, level.value(), work.interior.span().first(p),
                                   work.shell.span().first(m), run.params));
  MHGP11_TRY(checked_add(run.ledger.emitted, 1));
  return checked_add(run.ledger.incidences, u64(p) + m);
}

Outcome extend(Leaf& leaf, u32 depth, u32 begin, u64 candidates) noexcept {
  const u32 q = depth + 1;
  const int threshold = leaf.run.params.kmax + 1 - static_cast<int>(q);
  if (threshold < 0) return {};
  for (u32 i = leaf.pairs.next(candidates, begin); i < leaf.sites.size();
       i = leaf.pairs.next(candidates, i + 1)) {
    MHGP11_TRY(checked_add(leaf.run.ledger.prefixes, 1));
    leaf.prefix[depth] = i;
    if (q >= 2) {
      const auto possible = center_region_possible(leaf, q);
      if (!possible.ok()) return possible.outcome();
      if (!possible.value()) continue;
    }
    u32 count = 0;
    for (u32 word = 0; word < leaf.words; ++word) {
      const u64 mask = leaf.masks[depth][word] | leaf.run.workspace.dominance[u64(i) * leaf.words + word];
      leaf.masks[q][word] = mask;
      count += static_cast<u32>(std::popcount(mask));
    }
    if (count > static_cast<u32>(threshold)) continue;  // G3, union de temoins distincts
    if (q == 4) {
      const auto sphere = q4_of(leaf);
      if (!sphere.ok()) return sphere.outcome();
      if (sphere.value() && center_in_box(*sphere.value(), leaf.box))
        MHGP11_TRY(census_and_emit(leaf, q, *sphere.value()));
    } else if (q >= 2) {
      const auto sphere = sphere_of(leaf, q);
      if (!sphere.ok()) return sphere.outcome();
      if (sphere.value() && center_in_box(*sphere.value(), leaf.box))
        MHGP11_TRY(census_and_emit(leaf, q, *sphere.value()));
    }
    // Aucun test "q3 aigu" ne gouverne cette recursion : un prefixe obtus peut porter un q4 positif.
    // candidates ne contient deja que des indices >i. L'intersection conserve exactement les cliques
    // du prefixe, dans le meme ordre lexicographique ; les contacts ne sont jamais exclus.
    if (q < 4) MHGP11_TRY(extend(leaf, q, i + 1,
                                leaf.pairs.enabled() ? candidates & leaf.pairs.neighbors(i) : 0));
  }
  return {};
}

}  // namespace

Outcome enumerate_leaf(Run& run, std::span<const SiteIdx> sites, const Box& box) noexcept {
  const auto region = num::CenterRegion::make(box.lo, box.hi);
  if (!region.ok()) return fail(Reason::catalogue_invariant);
  const u32 words = static_cast<u32>((sites.size() + 63) / 64);
  auto pairs = SmallPairGraph::make(run.workspace.pair_rows.span(), static_cast<u32>(sites.size()),
                                     run.params.pair_graph);
  if (!pairs.ok()) return pairs.outcome();
  MHGP11_TRY(prepare(run, sites, box, words, pairs.value()));
  auto lines = CenterLineCache::make(run.workspace.center_lines.span(), run.workspace.points.span().first(sites.size()),
                                     region.value(), run.params.cache_center_lines);
  if (!lines.ok()) return lines.outcome();
  Leaf leaf{run, sites, box, lines.value(), pairs.value(), words, {}, {}};
  // G2 : centre dans Q et p<=theta_q<=K-1 impliquent I et U complets dans la liste K-certifiee.
  // Si le vrai p>=K, la liste contient au moins K interieurs ; le rejet precede donc toute acceptation.
  return extend(leaf, 0, 0, pairs.value().initial());
}

}  // namespace mhgp11::catalogue_detail
