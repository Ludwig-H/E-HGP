// Oracle Gamma borne, independant de la geometrie et du DSU de production.
// Les centres sont resolus dans une base affine par Gram/Cramer en limbes
// 32 bits OBig640 ; aucun predicat q2/q3/q4 ni BallKey::power ne sert de juge.
// Les produits croises de niveaux de cette representation tiennent en 640
// bits sous u16. Le drapeau d'overflow est une erreur, jamais un vert.
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../oracle/obig.hpp"
#include "../src/forest/silent_incidence.hpp"
#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp7;

namespace {
using Big = mhgp7_oracle::OBig<20>;
using Vec = std::array<Big, 3>;
using Matrix = std::array<std::array<Big, 3>, 3>;

int failures = 0;
u64 judged_cuts = 0, judged_pairs = 0, silent_total = 0, regular_cases = 0;
u64 longest_chain = 0, cache_hits = 0;
u64 normalized_transitions = 0;
u64 delta_cuts = 0, delta_records = 0;
void check(bool ok, const char* label) {
  if (!ok) { ++failures; std::fprintf(stderr, "ECHEC : %s\n", label); }
}
Big num(i64 x) { return Big::from_i64(x); }
Vec point(const P3& p) { return {num(p.x), num(p.y), num(p.z)}; }
Vec sub(const Vec& a, const Vec& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Big dot(const Vec& a, const Vec& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Big det(const Matrix& m, int n) {
  if (n == 1) return m[0][0];
  if (n == 2) return m[0][0] * m[1][1] - m[0][1] * m[1][0];
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
         m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

struct OracleBall {
  unsigned support = 0;
  Vec centre{};  // centre / den
  Big den{}, radius{}, level_den{};  // niveau radius / den^2
  Big power(const P3& p) const {
    Vec v = point(p);
    for (size_t d = 0; d < 3; ++d) v[d] = v[d] * den - centre[d];
    return dot(v, v) - radius;
  }
};

bool support_ball(const std::vector<P3>& pts, unsigned mask, OracleBall* ball) {
  const int n = std::popcount(mask) - 1;
  if (n < 1 || n > 3) return false;
  std::vector<Vec> vertices;
  for (size_t i = 0; i < pts.size(); ++i)
    if (mask & (1u << i)) vertices.push_back(point(pts[i]));
  std::array<Vec, 3> v{};
  Matrix gram{};
  for (int i = 0; i < n; ++i) v[(size_t)i] = sub(vertices[(size_t)i + 1], vertices[0]);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) gram[(size_t)i][(size_t)j] = dot(v[(size_t)i], v[(size_t)j]);
  const Big determinant = det(gram, n);
  if (determinant.sign() <= 0) return false;
  const Big den = num(2) * determinant;
  Big total{};
  std::array<Big, 3> alpha{};
  for (int c = 0; c < n; ++c) {
    Matrix replaced = gram;
    for (int r = 0; r < n; ++r) replaced[(size_t)r][(size_t)c] = gram[(size_t)r][(size_t)r];
    alpha[(size_t)c] = det(replaced, n);
    if (alpha[(size_t)c].sign() <= 0) return false;
    total += alpha[(size_t)c];
  }
  if (total >= den) return false;
  Vec relative{};
  for (size_t d = 0; d < 3; ++d)
    for (int i = 0; i < n; ++i) relative[d] += v[(size_t)i][d] * alpha[(size_t)i];
  ball->support = mask;
  ball->den = den;
  ball->level_den = den * den;
  ball->radius = dot(relative, relative);
  for (size_t d = 0; d < 3; ++d) ball->centre[d] = vertices[0][d] * den + relative[d];
  return true;
}

int compare(const OracleBall& a, const OracleBall& b) {
  return cmp(a.radius * b.level_den, b.radius * a.level_den);
}
int compare(const ExactLevel& a, const OracleBall& b) {
  return cmp(Big::from_u64_words(a.num, 3) * b.level_den,
             b.radius * Big::from_i128(a.den));
}
int compare(const ExactLevel& a, const ExactLevel& b) {
  return cmp(Big::from_u64_words(a.num, 3) * Big::from_i128(b.den),
             Big::from_u64_words(b.num, 3) * Big::from_i128(a.den));
}

struct Oracle {
  std::vector<P3> pts;
  std::vector<OracleBall> supports;
  std::vector<size_t> meb;
  bool regular = true;
  explicit Oracle(const std::vector<P3>& points) : pts(points) {
    const unsigned end = 1u << pts.size();
    for (unsigned mask = 1; mask < end; ++mask) {
      if (std::popcount(mask) < 2 || std::popcount(mask) > 4) continue;
      OracleBall b;
      if (support_ball(pts, mask, &b)) supports.push_back(b);
    }
    meb.assign(end, supports.size());
    for (unsigned mask = 1; mask < end; ++mask) {
      if (std::popcount(mask) < 2) continue;
      for (size_t i = 0; i < supports.size(); ++i) {
        const OracleBall& b = supports[i];
        if ((b.support & mask) != b.support) continue;
        bool contains = true;
        for (size_t p = 0; p < pts.size(); ++p)
          if ((mask & (1u << p)) && b.power(pts[p]).sign() > 0) contains = false;
        if (!contains) continue;
        meb[mask] = i;
        unsigned shell = 0;
        for (size_t p = 0; p < pts.size(); ++p)
          if (b.power(pts[p]).sign() == 0) shell |= 1u << p;
        if (shell != b.support) regular = false;
        break;
      }
      check(meb[mask] != supports.size(), "oracle MEB totale");
    }
  }
  const OracleBall& ball(unsigned mask) const { return supports[meb[mask]]; }
  bool direct(unsigned mask) const {
    const OracleBall& b = ball(mask);
    for (size_t p = 0; p < pts.size(); ++p)
      if (!(mask & (1u << p)) && b.power(pts[p]).sign() < 0) return false;
    return true;
  }
};

unsigned event_mask(const ForestEvent& e) {
  unsigned mask = 0;
  for (size_t i = 0; i < e.q; ++i) mask |= 1u << e.support[i];
  for (size_t i = 0; i < e.d; ++i) mask |= 1u << e.interior[i];
  return mask;
}

struct Partition {
  std::vector<unsigned> root;
  std::vector<bool> active;
  explicit Partition(size_t n) : root(n), active(n, false) {
    for (size_t i = 0; i < n; ++i) root[i] = (unsigned)i;
  }
  unsigned find(unsigned x) {
    while (root[x] != x) { root[x] = root[root[x]]; x = root[x]; }
    return x;
  }
  void coface(unsigned mask) {
    unsigned first = 0;
    for (unsigned bits = mask; bits != 0; bits &= bits - 1) {
      const unsigned facet = mask ^ (bits & (0u - bits));
      active[facet] = true;
      if (first == 0) first = facet;
      else root[find(facet)] = find(first);
    }
  }
  std::multiset<unsigned> coverage() {
    std::map<unsigned, unsigned> sets;
    for (unsigned f = 0; f < active.size(); ++f)
      if (active[f]) sets[find(f)] |= f;
    std::multiset<unsigned> result;
    for (const auto& entry : sets) result.insert(entry.second);
    return result;
  }
};

unsigned facet_mask(const FacetKey& f) {
  unsigned mask = 0;
  for (size_t i = 0; i < f.k; ++i) mask |= 1u << f.p[i];
  return mask;
}

// Les identites de facettes du sous-flot peuvent differer de celles de
// Gamma exhaustif. Le juge compare les vraies composantes pre-lot auxquelles
// elles appartiennent, le q_R et l'existence de chaque naissance/fusion.
bool compare_normalized(const Oracle& oracle, int K, const ForestResult& forest) {
  const unsigned end = 1u << oracle.pts.size();
  std::vector<unsigned> all;
  for (unsigned mask = 1; mask < end; ++mask)
    if (std::popcount(mask) == K + 1) all.push_back(mask);
  for (unsigned coface : all) {
    const OracleBall& cut = oracle.ball(coface);
    Partition pre(end), post(end);
    std::set<unsigned> touched;
    for (unsigned q : all) {
      const int c = compare(oracle.ball(q), cut);
      if (c < 0) pre.coface(q);
      if (c <= 0) post.coface(q);
      if (c == 0)
        for (unsigned bits = q; bits != 0; bits &= bits - 1)
          touched.insert(q ^ (bits & (0u - bits)));
    }
    std::map<unsigned, std::set<unsigned>> roots;
    std::map<unsigned, unsigned> old_points, new_points;
    for (unsigned f = 1; f < end; ++f) {
      if (!post.active[f]) continue;
      const unsigned r = post.find(f);
      new_points[r] |= f;
      if (pre.active[f]) {
        roots[r].insert(pre.find(f));
        old_points[r] |= f;
      }
    }
    std::set<unsigned> published;
    for (size_t i = 0; i < forest.delta_count(); ++i) {
      const auto delta = forest.delta(i);
      if (compare(delta.level, cut) != 0) continue;
      ++normalized_transitions;
      const unsigned output = facet_mask(delta.output);
      if (!post.active[output]) return false;
      const unsigned r = post.find(output);
      if (!published.insert(r).second) return false;
      std::set<unsigned> got_roots;
      for (const FacetKey& f : delta.parents) {
        const unsigned mask = facet_mask(f);
        if (!pre.active[mask]) return false;  // parent latent interdit
        got_roots.insert(pre.find(mask));
      }
      if (got_roots != roots[r] || got_roots.size() != delta.parents.size()) return false;
    }
    for (unsigned f : touched) {
      const unsigned r = post.find(f);
      const bool required = roots[r].size() != 1 || new_points[r] != old_points[r];
      if (required && published.find(r) == published.end()) return false;
    }
  }
  return true;
}

// Lecteur independant du payload LIVRE. Aucune coface candidate, aucun
// evenement et aucun DSU produit ne lui fournissent une union. Les seuls
// tokens sont parents et born : born signifie premiere materialisation
// dans le sous-flot, pas naissance geometrique. Chaque lot fige les parents
// anterieurs avant les unions. Le lecteur recoit ensuite Gamma comme juge
// externe de ses coupes, de sa couverture et des classes du coeur direct.
bool compare_delta_cuts(const Oracle& oracle, int K, const ForestResult& forest) {
  const unsigned end = 1u << oracle.pts.size();
  std::vector<unsigned> all, core, cuts;
  for (unsigned mask = 1; mask < end; ++mask) {
    if (std::popcount(mask) != K + 1) continue;
    all.push_back(mask);
    if (oracle.direct(mask))
      for (unsigned bits = mask; bits != 0; bits &= bits - 1)
        core.push_back(mask ^ (bits & (0u - bits)));
  }
  cuts = all;
  std::sort(cuts.begin(), cuts.end(), [&](unsigned a, unsigned b) {
    return compare(oracle.ball(a), oracle.ball(b)) < 0;
  });
  cuts.erase(std::unique(cuts.begin(), cuts.end(), [&](unsigned a, unsigned b) {
    return compare(oracle.ball(a), oracle.ball(b)) == 0;
  }), cuts.end());
  std::sort(core.begin(), core.end());
  core.erase(std::unique(core.begin(), core.end()), core.end());
  std::vector<size_t> order(forest.delta_count());
  for (size_t i = 0; i < order.size(); ++i) {
    order[i] = i;
    if (forest.delta(i).level.den <= 0) return false;
  }
  std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
    return compare(forest.delta(a).level, forest.delta(b).level) < 0;
  });
  const auto valid_facet = [&](const FacetKey& f) {
    if (f.k != K) return false;
    unsigned mask = 0;
    for (size_t i = 0; i < f.k; ++i) {
      if (f.p[i] >= oracle.pts.size() || (mask & (1u << f.p[i]))) return false;
      mask |= 1u << f.p[i];
    }
    return true;
  };
  Partition candidate(end);
  std::set<unsigned> public_roots;
  size_t cursor = 0;
  for (unsigned cut_mask : cuts) {
    const OracleBall& cut = oracle.ball(cut_mask);
    for (bool closed : {false, true}) {
      while (cursor < order.size()) {
        const int c = compare(forest.delta(order[cursor]).level, cut);
        if (c > 0 || (c == 0 && !closed)) break;
        const ExactLevel level = forest.delta(order[cursor]).level;
        size_t stop = cursor + 1;
        while (stop < order.size() && compare(forest.delta(order[stop]).level, level) == 0) ++stop;
        Partition before = candidate;
        const auto published_before = public_roots;
        std::set<unsigned> used_parents, used_born, consumed, outputs;
        for (; cursor < stop; ++cursor) {
          ++delta_records;
          const auto delta = forest.delta(order[cursor]);
          std::vector<unsigned> tokens;
          for (const FacetKey& f : delta.parents) {
            if (!valid_facet(f)) return false;
            const unsigned mask = facet_mask(f);
            if (!published_before.contains(mask) || !consumed.insert(mask).second) return false;
            if (!before.active[mask] || !used_parents.insert(before.find(mask)).second) return false;
            tokens.push_back(mask);
          }
          for (const FacetKey& f : delta.born) {
            if (!valid_facet(f)) return false;
            const unsigned mask = facet_mask(f);
            if (before.active[mask] || !used_born.insert(mask).second) return false;
            candidate.active[mask] = true;
            tokens.push_back(mask);
          }
          if (tokens.empty() || !valid_facet(delta.output)) return false;
          const unsigned first = tokens.front();
          for (unsigned mask : tokens) candidate.root[candidate.find(mask)] = candidate.find(first);
          const unsigned output = facet_mask(delta.output);
          if (!candidate.active[output] || candidate.find(output) != candidate.find(first)) return false;
          // L'identite du schema est la facette canonique du sous-flot,
          // pas une facette arbitraire de la meme composante. Un output
          // remplace pourrait sinon casser un parent futur sans changer H0.
          const auto lex_less = [](unsigned a, unsigned b) {
            while (a != 0 && b != 0) {
              const unsigned x = a & (0u - a), y = b & (0u - b);
              if (x != y) return x < y;
              a ^= x;
              b ^= y;
            }
            return a == 0 && b != 0;
          };
          for (unsigned f = 1; f < end; ++f)
            if (candidate.active[f] && candidate.find(f) == candidate.find(output) && lex_less(f, output)) return false;
          if (!outputs.insert(output).second) return false;
        }
        for (unsigned p : consumed) public_roots.erase(p);
        for (unsigned output : outputs)
          if (!public_roots.insert(output).second) return false;
      }
      ++delta_cuts;
      Partition reference(end);
      for (unsigned coface : all) {
        const int c = compare(oracle.ball(coface), cut);
        if (c < 0 || (c == 0 && closed)) reference.coface(coface);
      }
      if (candidate.coverage() != reference.coverage()) return false;
      for (unsigned a : core) {
        if (candidate.active[a] != reference.active[a]) return false;
        for (unsigned b : core)
          if (reference.active[a] && reference.active[b] &&
              ((candidate.find(a) == candidate.find(b)) != (reference.find(a) == reference.find(b)))) return false;
      }
    }
  }
  return cursor == order.size();  // un delta artificiellement retarde ne disparait pas du juge
}

std::vector<ForestEvent> product_direct(const CloudIndex& ix, int K) {
  GenerateOptions options;
  options.smax = std::min<u64>(11, ix.keys.size());
  options.threads = 1;
  GenerateStats gs;
  std::vector<BallCandidate> candidates;
  generate_candidates(ix, options, &candidates, &gs);
  sort_candidates(&candidates, 1);
  deduplicate_candidates(&candidates);
  std::vector<Survivor> survivors;
  ExpandStats stats;
  prefilter_balls(ix, candidates, options.smax, 1, &survivors, &stats);
  std::vector<BallData> balls;
  check(census_balls(ix, candidates, survivors, options.smax, 12, 1, &balls, &stats) ==
        PipelineStatus::kCompleteRegular, "census source complete");
  std::vector<ForestEvent> events;
  expand_events_k(ix, balls, (u64)K, options.smax - 1, 1, &events, &stats);
  return events;
}

bool compare_cuts(const Oracle& oracle, int K, const std::vector<ForestEvent>& direct,
                  const std::vector<ForestEvent>& extra) {
  const unsigned end = 1u << oracle.pts.size();
  std::vector<unsigned> all, core;
  for (unsigned mask = 1; mask < end; ++mask) {
    if (std::popcount(mask) != K + 1) continue;
    all.push_back(mask);
    if (oracle.direct(mask))
      for (unsigned bits = mask; bits != 0; bits &= bits - 1)
        core.push_back(mask ^ (bits & (0u - bits)));
  }
  std::sort(core.begin(), core.end());
  core.erase(std::unique(core.begin(), core.end()), core.end());
  std::vector<ForestEvent> events = direct;
  events.insert(events.end(), extra.begin(), extra.end());
  for (unsigned mask : all) {
    const OracleBall& cut = oracle.ball(mask);
    for (bool closed : {false, true}) {
      ++judged_cuts;
      Partition reference(end), candidate(end);
      for (unsigned coface : all) {
        const int cmp = compare(oracle.ball(coface), cut);
        if (cmp < 0 || (closed && cmp == 0)) reference.coface(coface);
      }
      for (const ForestEvent& e : events) {
        const int cmp = compare(e.level, cut);
        if (cmp < 0 || (closed && cmp == 0)) candidate.coface(event_mask(e));
      }
      if (reference.coverage() != candidate.coverage()) return false;
      for (unsigned a : core) {
        if (reference.active[a] != candidate.active[a]) return false;
        for (unsigned b : core) {
          ++judged_pairs;
          if (reference.active[a] && reference.active[b] &&
              ((reference.find(a) == reference.find(b)) != (candidate.find(a) == candidate.find(b)))) return false;
        }
      }
    }
  }
  return true;
}

bool case_run(const std::vector<P3>& pts, int K, bool mutant = false) {
  Oracle oracle(pts);
  if (!oracle.regular) return false;
  ++regular_cases;
  const CloudIndex ix = build_cloud_index(pts);
  const std::vector<ForestEvent> direct = product_direct(ix, K);
  std::set<unsigned> expected, actual;
  for (unsigned mask = 1; mask < (1u << pts.size()); ++mask)
    if (std::popcount(mask) == K + 1 && oracle.direct(mask)) expected.insert(mask);
  for (const ForestEvent& e : direct) {
    const unsigned mask = event_mask(e);
    actual.insert(mask);
    check(compare(e.level, oracle.ball(mask)) == 0, "source niveau OBig");
  }
  check(actual == expected, "catalogue Gabriel complet contre oracle OBig");
  const SilentIncidenceResult result = build_silent_cofaces(ix, direct);
  if (result.status != SilentIncidenceStatus::kComplete)
    std::fprintf(stderr, "silent refus : %s (n=%zu K=%d)\n", result.reason, pts.size(), K);
  check(result.status == SilentIncidenceStatus::kComplete, "completion reguliere");
  silent_total += result.events.size();
  longest_chain = std::max(longest_chain, result.stats.max_chain_length);
  cache_hits += result.stats.terminal_cached;
  for (const ForestEvent& e : result.events) {
    const unsigned mask = event_mask(e);
    unsigned support = 0;
    for (size_t i = 0; i < e.q; ++i) support |= 1u << e.support[i];
    check(!oracle.direct(mask), "ajout non-Gabriel reel");
    check(compare(e.level, oracle.ball(mask)) == 0, "ajout niveau exact OBig");
    check(oracle.ball(mask).support == support && e.active_mask == (u16)((1u << e.q) - 1u),
          "support essentiel et roles stricts OBig");
  }
  const bool same = compare_cuts(oracle, K, direct, result.events);
  if (mutant) return !same && result.status == SilentIncidenceStatus::kComplete && result.stats.added_cofaces > 0;
  check(same, "Gamma coupe ouverte/fermee, coeur et couverture");
  std::vector<ForestEvent> joined = direct;
  joined.insert(joined.end(), result.events.begin(), result.events.end());
  const ForestResult forest = build_forest(joined);
  check(forest.refusal.empty() && forest.attach_violations == 0 && forest.birth_violations == 0 &&
        forest.partition_violations == 0, "compatibilite fold et roles stricts");
  const ForestResult normalized = build_forest(joined, 1, ForestLayout::kClassic, true);
  check(normalized.normalized_reduced && normalized.refusal.empty() &&
        compare_normalized(oracle, K, normalized), "q_R et transitions normalises contre Gamma");
  check(compare_delta_cuts(oracle, K, normalized), "deltas classic seuls, coupes Gamma");
  const ForestResult normalized_csr = build_forest(joined, 2, ForestLayout::kCsr, true);
  check(normalized_csr.normalized_reduced && normalized_csr.refusal.empty() &&
        compare_normalized(oracle, K, normalized_csr), "q_R normalise CSR contre Gamma");
  check(compare_delta_cuts(oracle, K, normalized_csr), "deltas CSR seuls, coupes Gamma");
  auto permuted = direct;
  std::reverse(permuted.begin(), permuted.end());
  std::vector<InputPoint> physical;
  for (size_t i = pts.size(); i > 0; --i) physical.push_back({(PointId)(i - 1), pts[i - 1]});
  const auto repeat = build_silent_cofaces(build_cloud_index(physical), permuted);
  check(repeat.status == result.status && repeat.events.size() == result.events.size(),
        "permutation physique et ordre du catalogue");
  for (size_t i = 0; i < std::min(repeat.events.size(), result.events.size()); ++i)
    check(silent_detail::event_key(repeat.events[i]) == silent_detail::event_key(result.events[i]) &&
          same_exact_level(repeat.events[i].level, result.events[i].level), "rejeu canonique des ajouts");
  // Limites prospectives : la campagne s'arrete exactement au compteur,
  // sans rendre les cofaces eventuellement accumulees avant l'echec.
  if (result.stats.added_cofaces != 0) {
    SilentIncidenceLimits limits;
    limits.max_added_cofaces = result.stats.added_cofaces - 1;
    const auto refused = build_silent_cofaces(ix, direct, limits);
    check(refused.status == SilentIncidenceStatus::kResourceExhausted && refused.events.empty(),
          "plafond cofaces et refus atomique");
    limits = {};
    limits.max_chain_steps = 0;
    const auto steps = build_silent_cofaces(ix, direct, limits);
    check(steps.status == SilentIncidenceStatus::kResourceExhausted && steps.events.empty(),
          "plafond pas et refus atomique");
  }
  return true;
}

std::vector<P3> e5() { return {{0, 0, 7}, {0, 9, 6}, {1, 4, 0}, {0, 0, 1}, {4, 1, 2}}; }

void reject_cases() {
  const auto pts = e5();
  const auto ix = build_cloud_index(pts);
  auto direct = product_direct(ix, 2);
  SilentIncidenceLimits caps;
  caps.max_core_records = 0;
  auto r = build_silent_cofaces(ix, direct, caps);
  check(r.status == SilentIncidenceStatus::kResourceExhausted && r.events.empty(), "cap coeur zero");
  caps = {};
  caps.max_query_nodes = 0;
  r = build_silent_cofaces(ix, direct, caps);
  check(r.status == SilentIncidenceStatus::kResourceExhausted && r.events.empty(), "cap requete zero");
  caps = {};
  caps.max_meb_supports = 0;
  r = build_silent_cofaces(ix, direct, caps);
  check(r.status == SilentIncidenceStatus::kResourceExhausted && r.events.empty(), "cap MEB zero");
  auto corrupt = direct;
  corrupt[0].support[0] = 999999;
  r = build_silent_cofaces(ix, corrupt);
  check(r.status == SilentIncidenceStatus::kInvalidInput && r.events.empty(), "PointId inconnu refuse");
  corrupt = direct;
  corrupt.push_back(corrupt[0]);
  r = build_silent_cofaces(ix, corrupt);
  check(r.status == SilentIncidenceStatus::kInvalidInput && r.events.empty(), "coface directe dupliquee refusee");
  auto k1 = product_direct(ix, 1);
  r = build_silent_cofaces(ix, k1);
  check(r.status == SilentIncidenceStatus::kComplete && r.events.empty(), "K1 deja direct sans completion");
  k1.push_back(k1[0]);
  r = build_silent_cofaces(ix, k1);
  check(r.status == SilentIncidenceStatus::kInvalidInput && r.events.empty(), "doublon K1 refuse aussi");
  corrupt = direct;
  corrupt.erase(std::remove_if(corrupt.begin(), corrupt.end(), [](const ForestEvent& e) {
    return event_mask(e) == ((1u << 2) | (1u << 3) | (1u << 4));
  }), corrupt.end());
  r = build_silent_cofaces(ix, corrupt);
  check(r.status == SilentIncidenceStatus::kInvariantViolated && r.events.empty() &&
        std::strcmp(r.reason, "silent_terminal_missing_from_catalogue") == 0,
        "terminal Gabriel absent refuse causalement");
  const std::vector<P3> square{{0, 0, 0}, {8, 0, 0}, {8, 8, 0}, {0, 8, 0}};
  const auto square_ix = build_cloud_index(square);
  const auto square_events = product_direct(square_ix, 2);
  r = build_silent_cofaces(square_ix, square_events);
  check(r.status == SilentIncidenceStatus::kUnsupportedDegeneracy && r.events.empty(), "extra-shell refuse");
}

void pipeline_cases() {
  const auto pts = e5();
  const Oracle oracle(pts);
  std::vector<InputPoint> input;
  for (size_t i = 0; i < pts.size(); ++i) input.push_back({(PointId)i, pts[i]});
  for (int threads : {1, 3}) {
    RunOptions options;
    options.threads = threads;
    options.complete_silent_incidence = true;
    options.digest = true;
    options.forest_layout = threads == 1 ? ForestLayout::kClassic : ForestLayout::kCsr;
    std::vector<ForestEvent> captured;
    ForestResult captured_forest;
    options.on_forest = [&](u64 k, const auto& ev, const ForestResult& f) {
      if (k == 2) { captured = ev; captured_forest = f; }
    };
    const RunResult run = run_pipeline(input, options);
    check(run.status == PipelineStatus::kCompleteRegular && !run.digest_all.empty(), "API pipeline complet");
    check(compare_cuts(oracle, 2, {}, captured), "API pipeline coupes Gamma E5");
    check(captured_forest.normalized_reduced && compare_normalized(oracle, 2, captured_forest),
          "API pipeline q_R Gamma E5");
    check(compare_delta_cuts(oracle, 2, captured_forest), "API pipeline deltas seuls contre Gamma E5");
  }
  RunOptions options;
  options.complete_silent_incidence = true;
  options.digest = true;
  options.fold_join_before_next_k = true;
  std::vector<u64> callbacks;
  options.on_forest = [&](u64 k, const auto&, const auto&) { callbacks.push_back(k); };
  options.max_raw_candidates = 1;
  const auto early_cap = run_pipeline(input, options);
  check(early_cap.status == PipelineStatus::kResourceExhausted && callbacks.empty() &&
        early_cap.digest_all.empty() && early_cap.total_events == 0, "API cap brut avant callbacks");
  options.max_raw_candidates = kMaxRawCandidates;
  options.silent_limits.max_chain_steps = 0;
  const auto late_cap = run_pipeline(input, options);
  check(late_cap.status == PipelineStatus::kResourceExhausted && callbacks == std::vector<u64>{1} &&
        late_cap.message.find("silent_chain_step_budget") != std::string::npos &&
        late_cap.digest_all.empty() && late_cap.digest_forest.empty() && late_cap.total_events == 0,
        "API cap silencieux tardif, prefixe K1 provisoire et payload invalide");
  callbacks.clear();
  options.silent_limits = {};
  const std::vector<InputPoint> square{{0, {0, 0, 0}}, {1, {8, 0, 0}},
                                       {2, {8, 8, 0}}, {3, {0, 8, 0}}};
  const auto shell = run_pipeline(square, options);
  check(shell.status == PipelineStatus::kUnsupportedDegeneracy && callbacks.empty() &&
        shell.digest_all.empty(), "API extra-shell pertinent avant callbacks");

  const auto direct = product_direct(build_cloud_index(pts), 2);
  auto joined = direct;
  const auto added = build_silent_cofaces(build_cloud_index(pts), direct);
  joined.insert(joined.end(), added.events.begin(), added.events.end());
  const auto legacy = build_forest(joined);
  const auto normalized = build_forest(joined, 1, ForestLayout::kClassic, true);
  check(!compare_normalized(oracle, 2, legacy), "refutation permanente q_R du mode legacy");
  check(compare_normalized(oracle, 2, normalized), "mode normalise corrige le q_R legacy");
}

bool reduced_mutant_killed(const std::string& mutation) {
  const auto pts = e5();
  const Oracle oracle(pts);
  const auto ix = build_cloud_index(pts);
  auto events = product_direct(ix, 2);
  const auto added = build_silent_cofaces(ix, events);
  if (added.status != SilentIncidenceStatus::kComplete || added.events.empty()) return false;
  events.insert(events.end(), added.events.begin(), added.events.end());
  bool killed = true;
  for (ForestLayout layout : {ForestLayout::kClassic, ForestLayout::kCsr}) {
    if (mutation == "csr-stale-level" && layout == ForestLayout::kClassic) continue;
    const auto normalized = build_forest(events, 1, layout, true);
    killed = killed && normalized.refusal.empty() && !compare_delta_cuts(oracle, 2, normalized);
  }
  return killed;
}

void payload_mutants() {
  const auto pts = e5();
  const Oracle oracle(pts);
  const auto ix = build_cloud_index(pts);
  auto events = product_direct(ix, 2);
  const auto added = build_silent_cofaces(ix, events);
  events.insert(events.end(), added.events.begin(), added.events.end());
  const auto baseline = build_forest(events, 1, ForestLayout::kClassic, true);
  auto no_born = baseline;
  for (auto& d : no_born.deltas) d.born.clear();
  no_born.keys_born = 0;
  check(!compare_delta_cuts(oracle, 2, no_born), "mutant payload classic : effacement des seuls born");
  auto changed_output = baseline;
  bool replaced = false;
  for (const FacetKey& f : changed_output.deltas.front().born) {
    if (facet_mask(f) == facet_mask(changed_output.deltas.front().output)) continue;
    changed_output.deltas.front().output = f;
    replaced = true;
    break;
  }
  check(replaced && !compare_delta_cuts(oracle, 2, changed_output), "mutant payload classic : output non canonique");
  bool omitted = false, stale = false;
  for (size_t i = 0; i < baseline.deltas.size(); ++i) {
    if (baseline.deltas[i].parents.size() != 1 || baseline.deltas[i].born.empty()) continue;
    auto missing = baseline;
    missing.deltas.erase(missing.deltas.begin() + (ptrdiff_t)i);
    omitted = !compare_delta_cuts(oracle, 2, missing);
    if (i > 0) {
      auto shifted = baseline;
      shifted.deltas[i].level = baseline.deltas[i - 1].level;
      stale = !compare_delta_cuts(oracle, 2, shifted);
    }
    break;
  }
  check(omitted && stale, "mutants payload classic : attache omise ou avancee");
  const auto csr = build_forest(events, 2, ForestLayout::kCsr, true);
  no_born = csr;
  no_born.born_keys.clear();
  std::fill(no_born.born_off.begin(), no_born.born_off.end(), 0);
  no_born.keys_born = 0;
  check(!compare_delta_cuts(oracle, 2, no_born), "mutant payload CSR : effacement des seuls born");
  changed_output = csr;
  replaced = false;
  for (const FacetKey& f : csr.delta(0).born) {
    if (facet_mask(f) == facet_mask(csr.delta(0).output)) continue;
    changed_output.delta_meta.front().output = f;
    replaced = true;
    break;
  }
  check(replaced && !compare_delta_cuts(oracle, 2, changed_output), "mutant payload CSR : output non canonique");
  omitted = false;
  stale = false;
  for (size_t i = 0; i < csr.delta_count(); ++i) {
    const auto d = csr.delta(i);
    if (d.parents.size() != 1 || d.born.empty()) continue;
    auto missing = csr;
    // Reassembler les offsets conserve un payload CSR structurellement valide;
    // la faute vise seulement la perte semantique de cette attache.
    const u32 p0 = missing.parents_off[i], p1 = missing.parents_off[i + 1];
    const u32 b0 = missing.born_off[i], b1 = missing.born_off[i + 1];
    missing.parents_keys.erase(missing.parents_keys.begin() + p0, missing.parents_keys.begin() + p1);
    missing.born_keys.erase(missing.born_keys.begin() + b0, missing.born_keys.begin() + b1);
    missing.parents_off.erase(missing.parents_off.begin() + (ptrdiff_t)i + 1);
    missing.born_off.erase(missing.born_off.begin() + (ptrdiff_t)i + 1);
    for (size_t j = i + 1; j < missing.parents_off.size(); ++j) missing.parents_off[j] -= p1 - p0;
    for (size_t j = i + 1; j < missing.born_off.size(); ++j) missing.born_off[j] -= b1 - b0;
    missing.delta_meta.erase(missing.delta_meta.begin() + (ptrdiff_t)i);
    omitted = !compare_delta_cuts(oracle, 2, missing);
    if (i > 0) {
      auto shifted = csr;
      shifted.delta_meta[i].level = csr.delta_meta[i - 1].level;
      stale = !compare_delta_cuts(oracle, 2, shifted);
    }
    break;
  }
  check(omitted && stale, "mutants payload CSR : attache omise ou avancee");
}

void triangle_reduced_case() {
  const std::vector<P3> triangle{{0, 0, 0}, {6, 0, 0}, {2, 3, 0}};
  check(case_run(triangle, 2), "triangle aigu scalene regulier");
  const Oracle oracle(triangle);
  const auto events = product_direct(build_cloud_index(triangle), 2);
  const auto legacy = build_forest(events);
  check(legacy.delta_count() == 1 && legacy.delta(0).parents.size() == 3 && legacy.nodes == 1 &&
        !compare_normalized(oracle, 2, legacy), "triangle : legacy q_R=3 refute");
  for (ForestLayout layout : {ForestLayout::kClassic, ForestLayout::kCsr}) {
    const auto normalized = build_forest(events, 2, layout, true);
    check(normalized.delta_count() == 1 && normalized.delta(0).parents.empty() &&
          normalized.delta(0).born.size() == 3 && normalized.nodes == 0 &&
          compare_normalized(oracle, 2, normalized) && compare_delta_cuts(oracle, 2, normalized),
          "triangle : naissance reduite q_R=0, trois premieres materialisations");
  }
}
}  // namespace

int main(int argc, char** argv) {
  std::string mutation;
  if (argc == 2 && std::strncmp(argv[1], "--mutant=", 9) == 0) {
    mutation = argv[1] + 9;
    if (mutation != "silent-drop-coface" && mutation != "reduced-latent-parent" &&
        mutation != "reduced-drop-materialization" && mutation != "drop-nonmerge" &&
        mutation != "csr-stale-level") return 2;
    if (!mutants_enable(mutation)) return 2;
  } else if (argc != 1) return 2;
  mhgp7_oracle::clear_overflow();
  if (!mutation.empty()) {
    const bool killed = mutation == "silent-drop-coface" ? case_run(e5(), 2, true) : reduced_mutant_killed(mutation);
    return mhgp7_oracle::overflow_seen() || failures != 0 ? 1 : killed ? 4 : 3;
  }
  check(case_run(e5(), 2), "E5 reguliere");
  for (int K : {3, 4}) check(case_run(e5(), K), "E5 ordres superieurs");
  std::vector<P3> lifted = e5();
  for (P3& p : lifted) { p.x *= 16; p.y *= 16; p.z *= 16; }
  const P3 inner[] = {{31, 33, 47}, {34, 29, 52}, {29, 30, 44}};
  for (size_t i = 0; i < 3; ++i) {
    lifted.push_back(inner[i]);
    check(case_run(lifted, (int)i + 3), "E5 enrichie en points interieurs");
  }
  const std::vector<P3> chain_two{
      {31052, 37054, 53791}, {63099, 62295, 5489}, {45851, 18621, 10092},
      {32290, 41054, 26270}, {35795, 23044, 15792}, {22475, 26532, 25195},
      {55919, 55323, 7531}, {60817, 37898, 64418}, {48853, 14056, 27781}};
  check(case_run(chain_two, 4), "chaine deux remplacements essentiels");
  auto maximal_order = chain_two;
  maximal_order.push_back({26341, 59313, 45083});
  maximal_order.push_back({7417, 12277, 35399});
  check(case_run(maximal_order, 10), "K10, onze sites et facettes de cardinal maximal");
  const std::vector<P3> chain_cache{
      {36498, 1807, 6849}, {7348, 38797, 18402}, {56301, 17690, 57616},
      {31954, 5291, 1090}, {39114, 56187, 48653}, {62513, 51609, 20554},
      {59498, 28986, 5464}, {36722, 28396, 46897}, {28191, 56067, 52341}};
  check(case_run(chain_cache, 4), "reutilisation d'une chaine certifiee");
  u64 state = 0xdeadbeef12345678ull;
  for (size_t sample = 0; sample < 4; ++sample) {
    std::vector<P3> pts;
    for (size_t i = 0; i < 6 + sample % 3; ++i) {
      i64 c[3];
      for (int d = 0; d < 3; ++d) {
        state ^= state << 13; state ^= state >> 7; state ^= state << 17;
        c[d] = (i64)(state & 65535u);
      }
      pts.push_back({c[0], c[1], c[2]});
    }
    for (int K = 2; K <= (int)std::min<size_t>(5, pts.size() - 1); ++K)
      check(case_run(pts, K), "nuage u16 regulier");
  }
  reject_cases();
  pipeline_cases();
  payload_mutants();
  triangle_reduced_case();
  check(!mhgp7_oracle::overflow_seen(), "oracle sans debordement");
  check(regular_cases >= 26 && judged_cuts >= 1400 && judged_pairs >= 10000 && silent_total >= 8 &&
        longest_chain >= 2 && cache_hits >= 1 && delta_cuts >= 400 && delta_records >= 100 &&
        normalized_transitions >= 100,
        "planchers non-vacuite");
  std::printf("silent_incidence cases=%llu cuts=%llu core_pairs=%llu silent=%llu longest_chain=%llu cache_hits=%llu delta_cuts=%llu delta_records=%llu normalized_transitions=%llu failures=%d\n",
              (unsigned long long)regular_cases, (unsigned long long)judged_cuts,
              (unsigned long long)judged_pairs, (unsigned long long)silent_total,
              (unsigned long long)longest_chain, (unsigned long long)cache_hits,
              (unsigned long long)delta_cuts, (unsigned long long)delta_records,
              (unsigned long long)normalized_transitions, failures);
  return failures != 0 ? 1 : 0;
}
