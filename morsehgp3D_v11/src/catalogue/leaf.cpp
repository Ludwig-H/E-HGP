// Feuilles bornees : dominateurs distincts, DFS de supports, census local exact G2 et emission de S* seulement.
// Les triplets obtus restent des prefixes q4. Memo J2 optionnel borne, jamais memo de boules par candidat.
#include <algorithm>
#include <bit>

#include "catalogue/internal.hpp"
#include "catalogue/center_line_cache.hpp"
#include "catalogue/leaf_device.hpp"
#include "catalogue/small_pair_graph.hpp"

namespace mhgp11::catalogue_detail {
namespace {

// Compteurs du ledger tenus par feuille, sans controle ni Outcome dans la boucle (contrat R1 du 4 octobre). Pour
// m<=kMaxLeaf sites : prefixes <= sum_{q<=4} C(m,q) = P, demandes de couples et de droites <= 3P, census et
// incidences <= mP ; chaque champ reste < kLeafCountBound < 2^63, donc aucune addition locale ne deborde.
// flush les ajoute au ledger de la tache par checked_add apres le succes de la feuille ; sinon rien n'est publie.
constexpr u64 choose(u64 n, u64 k) noexcept {
  u64 r = 1;
  for (u64 i = 1; i <= k; ++i) r = r * (n - k + i) / i;
  return r;
}
inline constexpr u64 kLeafPrefixBound = choose(kMaxLeaf, 1) + choose(kMaxLeaf, 2) + choose(kMaxLeaf, 3) +
                                        choose(kMaxLeaf, 4);
inline constexpr u64 kLeafCountBound = u64(kMaxLeaf) * 3 * kLeafPrefixBound;
static_assert(kMaxLeaf <= 1024 && kLeafPrefixBound < (u64{1} << 37) && kLeafCountBound < (u64{1} << 49),
              "catalogue : compteurs locaux d'une feuille bornes");

struct LeafCounts {
  u64 dominance_tests = 0, prefixes = 0, judged = 0, census_tests = 0, emitted = 0, incidences = 0;
  u64 q4_candidates = 0, q4_levels = 0;
  u64 region_pair_tests = 0, region_pair_rejects = 0, region_line_tests = 0, region_line_rejects = 0;
  u64 region_line_evaluations = 0, region_line_cache_hits = 0, region_line_fallbacks = 0;
};

Outcome flush(const LeafCounts& c, CatalogueLedger& ledger) noexcept {
  MHGP11_TRY(checked_add(ledger.dominance_tests, c.dominance_tests));
  MHGP11_TRY(checked_add(ledger.prefixes, c.prefixes));
  MHGP11_TRY(checked_add(ledger.judged, c.judged));
  MHGP11_TRY(checked_add(ledger.census_tests, c.census_tests));
  MHGP11_TRY(checked_add(ledger.emitted, c.emitted));
  MHGP11_TRY(checked_add(ledger.incidences, c.incidences));
  MHGP11_TRY(checked_add(ledger.q4_candidates, c.q4_candidates));
  MHGP11_TRY(checked_add(ledger.q4_levels, c.q4_levels));
  MHGP11_TRY(checked_add(ledger.region_pair_tests, c.region_pair_tests));
  MHGP11_TRY(checked_add(ledger.region_pair_rejects, c.region_pair_rejects));
  MHGP11_TRY(checked_add(ledger.region_line_tests, c.region_line_tests));
  MHGP11_TRY(checked_add(ledger.region_line_rejects, c.region_line_rejects));
  MHGP11_TRY(checked_add(ledger.region_line_evaluations, c.region_line_evaluations));
  MHGP11_TRY(checked_add(ledger.region_line_cache_hits, c.region_line_cache_hits));
  return checked_add(ledger.region_line_fallbacks, c.region_line_fallbacks);
}

struct Leaf {
  Run& run;
  std::span<const SiteIdx> sites;
  const Box& box;
  CenterLineCache& lines;
  const SmallPairGraph& pairs;
  u32 words;
  std::array<u32, 4> prefix{};
  std::array<std::array<u64, kMaxWords>, 5> masks{};  // borne constante : 5*16 mots, profondeur <=4
  // Voie graphe (<=32 sites, un mot) : live[q-2][x] = voisins y de x avec |Dom(x) u Dom(y)| <= K+1-q.
  // Un prefixe de cardinal q contenant x et y hors de cette ligne echoue G3 ; borne constante 3*32 mots.
  std::array<std::array<u64, SmallPairGraph::kCapacity>, 3> live{};
  LeafCounts counts{};
};

Outcome prepare(Run& run, std::span<const SiteIdx> sites, const Box& box, u32 words,
                SmallPairGraph& pairs, LeafCounts& counts) noexcept {
  auto& work = run.workspace;
  const u32 m = static_cast<u32>(sites.size());
  if (m > work.points.size() || u64(m) * words > work.dominance.size() || u64(m) * words > work.dominated.size())
    return fail(Reason::catalogue_invariant);
  std::fill_n(work.dominance.data(), u64(m) * words, u64{0});
  std::fill_n(work.dominated.data(), u64(m) * words, u64{0});
  for (u32 i = 0; i < m; ++i) {
    const auto p = point(run.cloud, sites[i]);
    if (!p.ok()) return p.outcome();
    work.points[i] = p.value();
  }
  // Forme affine de difference des distances ; chaque somme partielle < 12*2^(2B), en i64.
  static_assert(2 * kCoordBits + 5 <= 63, "catalogue : dominance fermee T0 en i64");
  counts.dominance_tests += u64(m) * (m - 1) / 2;  // un test par couple i<j, comme la boucle ci-dessous
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j) {
      i64 base = 0, cmin = 0, cmax = 0;
      const auto x = work.points[i].coordinates(), y = work.points[j].coordinates();
      for (int axis = 0; axis < 3; ++axis) {
        const i64 delta = i64(y[axis]) - x[axis];
        base += i64(y[axis]) * y[axis] - i64(x[axis]) * x[axis];
        cmin += (delta > 0 ? box.lo[axis] : box.hi[axis]) * delta;
        cmax += (delta > 0 ? box.hi[axis] : box.lo[axis]) * delta;
      }
      if (base - 2 * cmin < 0) {  // j domine i
        work.dominance[u64(i) * words + j / 64] |= u64{1} << (j % 64);
        work.dominated[u64(j) * words + i / 64] |= u64{1} << (i % 64);
      } else if (base - 2 * cmax > 0) {  // i domine j
        work.dominance[u64(j) * words + i / 64] |= u64{1} << (i % 64);
        work.dominated[u64(i) * words + j / 64] |= u64{1} << (j % 64);
      } else if (pairs.enabled()) {
        pairs.connect(i, j);
      }
    }
  return {};
}

// J2, paires : une dominance stricte dans un sens equivaut a une bissectrice disjointe de la fermeture.
// La voie graphe certifie ces couples par intersection ; le repli conserve les lectures historiques.
Result<bool> region_pairs_possible(Leaf& leaf, u32 q) noexcept {
  const auto& work = leaf.run.workspace;
  const u32 last = leaf.prefix[q - 1];
  for (u32 j = 0; !leaf.pairs.enabled() && j + 1 < q; ++j) {
    const u32 first = leaf.prefix[j];
    ++leaf.counts.region_pair_tests;
    const bool forward = (work.dominance[u64(first) * leaf.words + last / 64] >> (last % 64)) & 1;
    const bool backward = (work.dominance[u64(last) * leaf.words + first / 64] >> (first % 64)) & 1;
    if (forward || backward) {
      ++leaf.counts.region_pair_rejects;
      return false;
    }
  }
  return true;
}

// J2, droites : chaque face doit avoir sa droite de centres dans la fermeture. Un triplet aligne ne peut
// appartenir a aucun support affine independant. Aucune condition d'angle n'intervient ici.
// Appele apres G3 (un simple masque) : les droites exactes ne sont evaluees que pour les prefixes G3-admis.
Result<bool> region_lines_possible(Leaf& leaf, u32 q) noexcept {
  const u32 last = leaf.prefix[q - 1];
  for (u32 j = 0; j + 2 < q; ++j)
    for (u32 k = j + 1; k + 1 < q; ++k) {
      ++leaf.counts.region_line_tests;
      const auto query = leaf.lines.lookup(leaf.prefix[j], leaf.prefix[k], last);
      if (!query.ok()) return query.outcome();
      const auto reply = query.value();
      leaf.counts.region_line_evaluations += reply.hit ? 0u : 1u;
      leaf.counts.region_line_cache_hits += reply.hit ? 1u : 0u;
      leaf.counts.region_line_fallbacks += reply.fallback ? 1u : 0u;
      const auto relation = reply.relation;
      if (relation != num::CenterLineRelation::intersects) {
        ++leaf.counts.region_line_rejects;
        return false;
      }
    }
  return true;
}

// Population d'un mot : instruction materielle si la cible l'a, sinon forme SWAR (le profil x86-64 de base
// appelait __popcountdi2). Meme valeur dans les deux cas.
inline u32 popcount_word(u64 x) noexcept {
#if defined(__POPCNT__)
  return static_cast<u32>(std::popcount(x));
#else
  x -= (x >> 1) & 0x5555555555555555ull;
  x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
  x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0Full;
  return static_cast<u32>((x * 0x0101010101010101ull) >> 56);
#endif
}

Result<std::optional<num::Sphere>> q2_of(Leaf& leaf) noexcept {
  const auto& p = leaf.run.workspace.points;
  return num::Sphere::through(p[leaf.prefix[0]], p[leaf.prefix[1]]);
}

// q3 strict seulement ; le Level attend l'emission (proprietaire, census, S* et admission passes).
// Enveloppe fermee de points DOUBLES (2c entiers) contre la boite demi-ouverte [lo,hi) : faux seulement si, sur un
// axe, tous les points sont sous 2lo ou tous au moins 2hi ; tout point de l'enveloppe manque alors la boite.
// T0 : coordonnees et bornes <= 2^B, sommes doubles < 2^(B+2) en i64.
template <std::size_t N>
bool doubled_envelope_meets(const std::array<std::array<i64, 3>, N>& doubled, const Box& box) noexcept {
  for (int axis = 0; axis < 3; ++axis) {
    i64 low = doubled[0][axis], high = doubled[0][axis];
    for (std::size_t i = 1; i < N; ++i) {
      low = std::min(low, doubled[i][axis]);
      high = std::max(high, doubled[i][axis]);
    }
    if (high < 2 * box.lo[axis] || low >= 2 * box.hi[axis]) return false;
  }
  return true;
}

std::array<i64, 3> doubled(num::Point a, num::Point b) noexcept {
  return {i64{a.x()} + b.x(), i64{a.y()} + b.y(), i64{a.z()} + b.z()};
}

// q3 strict seulement ; le Level attend l'emission (proprietaire, census, S* et admission passes).
// Lemme M3 (feuille J3 de la v10) : le centre circonscrit d'un triangle strictement aigu est l'orthocentre de son
// triangle median, strictement interieur a celui-ci, donc dans l'enveloppe des trois milieux. Si elle manque la
// boite, center_in_box rejetterait aussi : meme decision, sans construire N/D.
Result<std::optional<num::Q3Candidate>> q3_of(Leaf& leaf) noexcept {
  const auto& p = leaf.run.workspace.points;
  const auto a = p[leaf.prefix[0]], b = p[leaf.prefix[1]], c = p[leaf.prefix[2]];
  if (!num::strictly_acute(a, b, c)) return std::optional<num::Q3Candidate>{};
  if (!doubled_envelope_meets(std::array{doubled(a, b), doubled(b, c), doubled(a, c)}, leaf.box))
    return std::optional<num::Q3Candidate>{};
  return num::Q3Candidate::through(a, b, c);
}

// Lemme E4 (feuille J3) : un centre strictement interieur au tetraedre (positivite exigee ensuite) est dans
// l'enveloppe de ses sommets ; si elle manque la boite, positivite ou center_in_box rejetterait. La non-degenerescence
// (det = orientation, meme produit mixte que la fabrique) est comptee avant ce rejet : q4_candidates inchange.
Result<std::optional<num::Q4Candidate>> q4_of(Leaf& leaf) noexcept {
  const auto& p = leaf.run.workspace.points;
  const auto a = p[leaf.prefix[0]], b = p[leaf.prefix[1]];
  const auto c = p[leaf.prefix[2]], d = p[leaf.prefix[3]];
  if (num::orientation(a, b, c, d) == 0) return std::optional<num::Q4Candidate>{};
  ++leaf.counts.q4_candidates;
  if (!doubled_envelope_meets(std::array{doubled(a, a), doubled(b, b), doubled(c, c), doubled(d, d)}, leaf.box))
    return std::optional<num::Q4Candidate>{};
  auto result = num::Q4Candidate::through(a, b, c, d);
  if (!result.ok()) return result.outcome();
  if (!result.value()) return fail(Reason::catalogue_invariant);  // det != 0 deja certifie
  // Meme tuple que through ci-dessus : le flag ne remplace pas le predicat generique de canonical_support.
  if (!result.value()->q4_presentation_strictly_inside()) return std::optional<num::Q4Candidate>{};
  return result;
}

Result<num::Level> emission_level(const num::Sphere& sphere, LeafCounts&) noexcept {
  return sphere.level();
}

Result<num::Level> emission_level(const num::Q3Candidate& sphere, LeafCounts&) noexcept {
  const auto full = sphere.materialize();
  if (!full.ok()) return full.outcome();
  return full.value().level();
}

Result<num::Level> emission_level(const num::Q4Candidate& sphere, LeafCounts& counts) noexcept {
  const auto full = sphere.materialize();
  if (!full.ok()) return full.outcome();
  ++counts.q4_levels;
  return full.value().level();
}

template <class Ball>
Outcome census_and_emit(Leaf& leaf, u32 q, const Ball& sphere) noexcept {
  auto& run = leaf.run;
  auto& work = run.workspace;
  ++leaf.counts.judged;
  const u32 threshold = static_cast<u32>(run.params.kmax + 1) - q;  // appele seulement si q<=K+1
  // Lemme R (feuille J3 de la v10) : le centre est dans la boite et chaque generateur s est sur la sphere ; un site
  // qui domine s sur la fermeture est donc strictement interieur, un site que s domine strictement exterieur. Seuls
  // les autres sites sont testes ; l'ordre, les listes, l'arret et le compteur logique restent ceux du census complet.
  std::array<u64, kMaxWords> inside{}, outside{};
  for (u32 word = 0; word < leaf.words; ++word)
    for (u32 j = 0; j < q; ++j) {
      inside[word] |= work.dominance[u64(leaf.prefix[j]) * leaf.words + word];
      outside[word] |= work.dominated[u64(leaf.prefix[j]) * leaf.words + word];
    }
  for (u32 word = 0; word < leaf.words; ++word)
    if ((inside[word] & outside[word]) != 0) return fail(Reason::catalogue_invariant);  // centre hors de la boite
  u32 p = 0, m = 0, support_cursor = 0;
  for (u32 i = 0; i < leaf.sites.size(); ++i) {
    // Compteur LOGIQUE de classifications, pas un compte d'appels au predicat de puissance.
    ++leaf.counts.census_tests;
    int relation = 0;
    if (support_cursor < q && i == leaf.prefix[support_cursor]) {
      // La fabrique exacte a certifie le contact des q sites de CETTE presentation.
      // Le prefixe est croissant en positions locales ; aucun autre site de coquille n'est saute.
      ++support_cursor;
    } else if ((inside[i / 64] >> (i % 64)) & 1) {
      relation = -1;
    } else if ((outside[i / 64] >> (i % 64)) & 1) {
      relation = 1;
    } else {
      const auto side = num::side(sphere, work.points[i]);
      if (!side.ok()) return side.outcome();
      relation = side.value();
    }
    if (relation < 0) {
      if (p == threshold) return {};  // le prochain interieur donne p>theta_q ; aucun census accepte tronque
      work.interior[p++] = leaf.sites[i];
    } else if (relation == 0) {
      work.shell[m++] = leaf.sites[i];
    }
  }
  if (m < q || support_cursor != q) return fail(Reason::catalogue_invariant);
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
  const auto level = emission_level(sphere, leaf.counts);
  if (!level.ok()) return level.outcome();
  MHGP11_TRY(run.collector.accept(ball, level.value(), work.interior.span().first(p),
                                   work.shell.span().first(m), run.params));
  ++leaf.counts.emitted;
  leaf.counts.incidences += u64(p) + m;
  return {};
}

// logical : candidats que visiterait la voie graphe sans lignes vivantes (sur-ensemble de candidates). Les
// prefixes absents de candidates echoueraient G3 seul, sans autre effet : prefixes reste ce compte logique.
Outcome extend(Leaf& leaf, u32 depth, u32 begin, u64 candidates, u64 logical) noexcept {
  const u32 q = depth + 1;
  const int threshold = leaf.run.params.kmax + 1 - static_cast<int>(q);
  if (threshold < 0) return {};
  for (u32 i = leaf.pairs.next(candidates, begin); i < leaf.sites.size();
       i = leaf.pairs.next(candidates, i + 1)) {
    ++leaf.counts.prefixes;
    leaf.prefix[depth] = i;
    if (q >= 2) {
      const auto possible = region_pairs_possible(leaf, q);
      if (!possible.ok()) return possible.outcome();
      if (!possible.value()) continue;
    }
    u32 count = 0;
    for (u32 word = 0; word < leaf.words; ++word) {
      const u64 mask = leaf.masks[depth][word] | leaf.run.workspace.dominance[u64(i) * leaf.words + word];
      leaf.masks[q][word] = mask;
      count += popcount_word(mask);
    }
    if (count > static_cast<u32>(threshold)) continue;  // G3, union de temoins distincts
    // Filtres conjonctifs : rejeter par G3 avant les droites ne change ni les juges ni les emissions.
    if (q >= 3) {
      const auto possible = region_lines_possible(leaf, q);
      if (!possible.ok()) return possible.outcome();
      if (!possible.value()) continue;
    }
    if (q == 4) {
      const auto sphere = q4_of(leaf);
      if (!sphere.ok()) return sphere.outcome();
      if (sphere.value() && center_in_box(*sphere.value(), leaf.box))
        MHGP11_TRY(census_and_emit(leaf, q, *sphere.value()));
    } else if (q == 3) {
      const auto sphere = q3_of(leaf);
      if (!sphere.ok()) return sphere.outcome();
      if (sphere.value() && center_in_box(*sphere.value(), leaf.box))
        MHGP11_TRY(census_and_emit(leaf, q, *sphere.value()));
    } else if (q == 2) {
      const auto sphere = q2_of(leaf);
      if (!sphere.ok()) return sphere.outcome();
      if (sphere.value() && center_in_box(*sphere.value(), leaf.box))
        MHGP11_TRY(census_and_emit(leaf, q, *sphere.value()));
    }
    // Aucun test "q3 aigu" ne gouverne cette recursion : un prefixe obtus peut porter un q4 positif.
    // candidates ne contient deja que des indices >i. L'intersection conserve exactement les cliques
    // du prefixe, dans le meme ordre lexicographique ; les contacts ne sont jamais exclus.
    if (q < 4) {
      u64 next = 0, next_logical = 0;
      // Seuil du cardinal suivant < 0 : l'appel rend avant tout prefixe, aucun candidat a former.
      if (leaf.pairs.enabled() && leaf.run.params.kmax + 1 - static_cast<int>(q + 1) >= 0) {
        next_logical = logical & (~u64{0} << (i + 1)) & leaf.pairs.neighbors(i);  // i<32
        // Union du prefixe deja au-dela du seuil K-q du cardinal suivant : l'union etant monotone, chaque
        // prolongement echouerait G3 seul ; ces prefixes restent comptes, aucun appel.
        if (count > static_cast<u32>(leaf.run.params.kmax - static_cast<int>(q))) {
          leaf.counts.prefixes += popcount_word(next_logical);
          continue;
        }
        // Lignes vivantes, incluses dans les voisins : chaque candidat satisfait chaque paire du prefixe.
        next = candidates;  // vivants restants, tous > i
        for (u32 j = 0; j < q; ++j) next &= leaf.live[q - 1][leaf.prefix[j]];
        leaf.counts.prefixes += popcount_word(next_logical) - popcount_word(next);
      }
      MHGP11_TRY(extend(leaf, q, i + 1, next, next_logical));
    }
  }
  return {};
}

// Lignes vivantes depuis les masques de dominance complets (un mot par site sur la voie graphe).
void live_rows(Leaf& leaf) noexcept {
  if (!leaf.pairs.enabled()) return;
  const auto& dominance = leaf.run.workspace.dominance;
  const int kmax = leaf.run.params.kmax;
  for (u32 x = 0; x < leaf.sites.size(); ++x)
    for (u64 rest = leaf.pairs.neighbors(x); rest != 0; rest &= rest - 1) {
      const u32 y = static_cast<u32>(std::countr_zero(rest));
      const int weight = static_cast<int>(popcount_word(dominance[x] | dominance[y]));
      for (u32 q = 2; q <= 4; ++q)
        if (weight <= kmax + 1 - static_cast<int>(q)) leaf.live[q - 2][x] |= u64{1} << y;
    }
}

// Voie feuille source unique sur l'hote : emissions acceptees comme leaf.cpp, Level tire du support par
// Sphere::through de meme arite (identique a emission_level, puisque S* est la presentation generatrice).
struct AcceptSink {
  Run& run;
  Outcome outcome{};
  void emit(const leaf_device::Ball& ball, const u32* interior, const u32* shell) noexcept {
    if (!outcome.ok()) return;
    outcome = accept(ball, interior, shell);
  }
  Outcome accept(const leaf_device::Ball& ball, const u32* interior, const u32* shell) noexcept {
    auto& work = run.workspace;
    if (ball.p > work.interior.size() || ball.m > work.shell.size()) return fail(Reason::catalogue_invariant);
    for (u32 i = 0; i < ball.p; ++i) work.interior[i] = make_id<SiteIdx>(interior[i]);
    for (u32 i = 0; i < ball.m; ++i) work.shell[i] = make_id<SiteIdx>(shell[i]);
    std::array<SiteIdx, 4> support{};
    std::array<num::Point, 4> points{};
    for (u32 i = 0; i < 4; ++i) support[i] = make_id<SiteIdx>(ball.support[i]);
    for (u32 i = 0; i < ball.qmin; ++i) {
      auto p = point(run.cloud, support[i]);
      if (!p.ok()) return p.outcome();
      points[i] = p.value();
    }
    auto made = ball.qmin == 2 ? num::Sphere::through(points[0], points[1])
                : ball.qmin == 3 ? num::Sphere::through(points[0], points[1], points[2])
                                 : num::Sphere::through(points[0], points[1], points[2], points[3]);
    if (!made.ok()) return made.outcome();
    if (!made.value()) return fail(Reason::catalogue_invariant);
    const CatalogueBall record{support, make_id<LevelRank>(0), ball.p, ball.m, static_cast<u8>(ball.qmin)};
    return run.collector.accept(record, made.value()->level(), work.interior.span().first(ball.p),
                                work.shell.span().first(ball.m), run.params);
  }
};

// Joue la feuille source unique si elle s'applique ; rend vrai si la feuille est traitee (emissions et compteurs
// publies), faux si leaf.cpp doit la refaire (non resolue).
Result<bool> device_leaf(Run& run, std::span<const SiteIdx> sites, const Box& box) noexcept {
  std::array<u32, leaf_device::kMaxSites> ids{};
  for (u32 i = 0; i < sites.size(); ++i) ids[i] = idx(sites[i]);
  leaf_device::Input in;
  in.x = run.cloud.x().data(); in.y = run.cloud.y().data(); in.z = run.cloud.z().data();
  in.sites = ids.data(); in.m = static_cast<u32>(sites.size());
  for (int j = 0; j < 3; ++j) { in.lo[j] = box.lo[j]; in.hi[j] = box.hi[j]; }
  in.kmax = run.params.kmax; in.cache = run.params.cache_center_lines;
  leaf_device::Counts probe_counts;
  leaf_device::CountSink probe;
  if (leaf_device::run_leaf(in, probe_counts, probe) != leaf_device::kOk) return false;
  leaf_device::Counts c;
  AcceptSink sink{run};
  if (leaf_device::run_leaf(in, c, sink) != leaf_device::kOk) return fail(Reason::catalogue_invariant);
  MHGP11_TRY(sink.outcome);
  LeafCounts counts;
  counts.dominance_tests = c.dominance_tests; counts.prefixes = c.prefixes; counts.judged = c.judged;
  counts.census_tests = c.census_tests; counts.emitted = c.emitted; counts.incidences = c.incidences;
  counts.q4_candidates = c.q4_candidates; counts.q4_levels = c.q4_levels;
  counts.region_pair_tests = c.region_pair_tests; counts.region_pair_rejects = c.region_pair_rejects;
  counts.region_line_tests = c.region_line_tests; counts.region_line_rejects = c.region_line_rejects;
  counts.region_line_evaluations = c.region_line_evaluations;
  counts.region_line_cache_hits = c.region_line_cache_hits;
  counts.region_line_fallbacks = c.region_line_fallbacks;
  MHGP11_TRY(flush(counts, run.ledger));
  return true;
}

}  // namespace

Outcome enumerate_leaf(Run& run, std::span<const SiteIdx> sites, const Box& box) noexcept {
  if (run.deferred != nullptr && run.params.pair_graph && !sites.empty() && sites.size() <= leaf_device::kMaxSites)
    return run.deferred->push(sites, box);  // voie lot : compteurs et emissions viendront du lot
  if (run.params.device_leaf && run.params.pair_graph && !sites.empty() && sites.size() <= leaf_device::kMaxSites) {
    const auto done = device_leaf(run, sites, box);
    if (!done.ok()) return done.outcome();
    if (done.value()) return {};
  }
  const auto region = num::CenterRegion::make(box.lo, box.hi);
  if (!region.ok()) return fail(Reason::catalogue_invariant);
  const u32 words = static_cast<u32>((sites.size() + 63) / 64);
  auto pairs = SmallPairGraph::make(run.workspace.pair_rows.span(), static_cast<u32>(sites.size()),
                                     run.params.pair_graph);
  if (!pairs.ok()) return pairs.outcome();
  if (sites.size() > kMaxLeaf) return fail(Reason::catalogue_invariant);  // borne des compteurs locaux
  LeafCounts counts;
  MHGP11_TRY(prepare(run, sites, box, words, pairs.value(), counts));
  auto lines = CenterLineCache::make(run.workspace.center_lines.span(), run.workspace.points.span().first(sites.size()),
                                     region.value(), run.params.cache_center_lines);
  if (!lines.ok()) return lines.outcome();
  Leaf leaf{run, sites, box, lines.value(), pairs.value(), words, {}, {}, {}, counts};
  live_rows(leaf);
  // G2 : centre dans Q et p<=theta_q<=K-1 impliquent I et U complets dans la liste K-certifiee.
  // Si le vrai p>=K, la liste contient au moins K interieurs ; le rejet precede donc toute acceptation.
  MHGP11_TRY(extend(leaf, 0, 0, pairs.value().initial(), pairs.value().initial()));
  return flush(leaf.counts, run.ledger);
}

}  // namespace mhgp11::catalogue_detail
