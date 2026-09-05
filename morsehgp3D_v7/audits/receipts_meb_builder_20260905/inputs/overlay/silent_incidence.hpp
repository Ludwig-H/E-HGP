// MorseHGP3D v7 — completion des facettes du coeur Gabriel, a la demande.
//
// Precondition scientifique EXTERNE : direct_events est le catalogue COMPLET
// des cofaces Gabriel regulieres d'un seul ordre K. Cette brique ne certifie
// pas sa source. Elle ne publie ni Gamma exhaustif ni les identites v2.
//
// Pour une facette F, seuls >= 2 intrus stricts etrangers a F demandent une
// completion : avec zero ou un intrus, ses premieres incidences sont deja
// directes (INCIDENCES_SILENCIEUSES_GAMMA.md, corollaire 4.1). On ajoute
// Q=F+z, puis remplace un sommet ESSENTIEL de Q par un intrus strict w.
// La miniball diminue strictement ; les deux cofaces partagent Q\{u}.
// Une chaine ne reussit qu'en atteignant le catalogue direct fourni, ou une
// chaine deja certifiee. Tous les ajouts sont de vraies cofaces de Gamma.
// La confluence reguliere du meme corollaire permet une seule chaine par F.
//
// Aucun catalogue C(n,K), cellule ou mosaïque : les seules enumerations de
// supports sont LOCALES, sur <= 11 sites (<= 550 supports de tailles 2..4).
// La longueur des chaines n'a pas de borne pratique demontree : des plafonds
// prospectifs bornent records, supports, visites et pas. Tout refus efface
// les ajouts. Le domaine local est volontairement strict : extra-shells ou
// supports non essentiels rencontres sont refuses, jamais perturbes.
#pragma once

// PRIVATE OVERLAY of F f75a136a; no product integration or qualification.
// P>0: meb_supports is a reference ordinal, not a physical total.
// Physical proposal/fallback candidates have separate appended statistics.

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <unordered_set>
#include <utility>
#include <vector>

#include "meb_proposal.hpp"
#include "src/forest/fold.hpp"
#include "src/lanes/q2.hpp"
#include "src/lanes/q3.hpp"
#include "src/lanes/q4.hpp"
#include "src/pipeline/census.hpp"

namespace mhgp7 {

enum class SilentIncidenceStatus {
  kComplete, kInvalidInput, kUnsupportedDegeneracy,
  kResourceExhausted, kInvariantViolated
};

struct SilentIncidenceLimits {
  u64 max_core_records = 8000000;
  u64 max_chain_steps = 2000000;
  u64 max_added_cofaces = 2000000;
  u64 max_query_nodes = 1000000000;
  u64 max_meb_supports = 1000000000;
  // Opt-in proposal budget; zero keeps the literal F reference route.
  u64 max_meb_proposal_supports = 0;
};

struct SilentIncidenceStats {
  u64 core_records = 0, core_facets = 0;
  u64 facets_with_two_intruders = 0, chain_steps = 0, added_cofaces = 0;
  u64 terminal_direct = 0, terminal_cached = 0, max_chain_length = 0;
  u64 query_nodes = 0, query_leaves = 0, query_range_skips = 0;
  u64 meb_calls = 0, meb_supports = 0;
  // Appended diagnostics, never part of the legacy terminal projection.
  u64 meb_proposal_supports = 0, meb_fallback_supports = 0;
  u64 meb_proposal_pivots = 0, meb_proposal_certified = 0, meb_proposal_fallback = 0;
};

struct SilentIncidenceResult {
  SilentIncidenceStatus status = SilentIncidenceStatus::kComplete;
  const char* reason = "complete_relative_to_supplied_regular_direct_catalogue";
  std::vector<ForestEvent> events;
  SilentIncidenceStats stats;
};

namespace silent_detail {

struct CofaceKey {
  u8 n = 0;
  std::array<PointId, 11> p{};
  bool operator==(const CofaceKey& b) const { return n == b.n && p == b.p; }
  bool operator<(const CofaceKey& b) const { return n != b.n ? n < b.n : p < b.p; }
};

struct CofaceHash {
  size_t operator()(const CofaceKey& a) const {
    u64 h = 1469598103934665603ull ^ a.n;
    for (size_t i = 0; i < a.n; ++i) {
      h ^= a.p[i];
      h *= 1099511628211ull;
    }
    return (size_t)h;
  }
};

inline void sort_key(CofaceKey* k) {
  // Taille fixe <= 11 : insertion sans les speculative heap paths de
  // std::sort (GCC -O3 -Warray-bounds), avec indices explicitement bornes.
  for (size_t i = 1; i < k->p.size(); ++i) {
    if (i >= k->n) break;
    for (size_t j = i; j > 0 && k->p[j] < k->p[j - 1]; --j)
      std::swap(k->p[j], k->p[j - 1]);
  }
}

inline CofaceKey event_key(const ForestEvent& e) {
  CofaceKey k;
  if (e.q > 11 || e.d > 9 || (size_t)e.q + e.d > 11) return k;
  k.n = (u8)(e.q + e.d);
  for (size_t i = 0; i < e.q; ++i) k.p[i] = e.support[i];
  for (size_t i = 0; i < e.d; ++i) k.p[(size_t)e.q + i] = e.interior[i];
  sort_key(&k);
  return k;
}

struct LocalBall {
  BallKey key{};
  ExactLevel level{};
  u8 q = 0;
  std::array<i32, 4> support{};
};

#if defined(MHGP7_TESTING)
// Private per-thread observations; absent from production and public Stats.
// These count logical materializations, not retained machine instructions.
struct MebMaterializationForTests {
  u64 q2_rejected = 0, q2_materialized = 0, q2_rejected_materialized = 0;
  u64 q3_rejected = 0, q4_rejected = 0;
  u64 q3_materialized = 0, q4_materialized = 0;
  u64 q3_rejected_materialized = 0, q4_rejected_materialized = 0;
};
inline MebMaterializationForTests& meb_materialization_for_tests() {
  static thread_local MebMaterializationForTests counters;
  return counters;
}
#endif

class Builder {
 public:
  Builder(const CloudIndex& index, const std::vector<ForestEvent>& direct,
          const SilentIncidenceLimits& limits, SilentIncidenceResult* result)
      : ix(index), source(direct), caps(limits), out(*result),
        meb_work{out.stats.meb_proposal_supports, out.stats.meb_proposal_pivots,
                 out.stats.meb_proposal_certified, out.stats.meb_proposal_fallback} {}

  bool fail(SilentIncidenceStatus status, const char* reason) {
    out.status = status;
    out.reason = reason;
    return false;
  }

  bool charge(u64& counter, u64 limit, const char* reason) {
    if (counter >= limit) return fail(SilentIncidenceStatus::kResourceExhausted, reason);
    ++counter;
    return true;
  }

  bool resolve(const CofaceKey& key, std::array<i32, 11>* sites) {
    for (size_t i = 0; i < key.n; ++i) {
      const auto it = std::lower_bound(ids.begin(), ids.end(), key.p[i],
          [](const auto& item, PointId p) { return item.first < p; });
      if (it == ids.end() || it->first != key.p[i])
        return fail(SilentIncidenceStatus::kInvalidInput, "silent_unknown_point_id");
      (*sites)[i] = it->second;
    }
    return true;
  }

  // Work belongs to this Builder/order. Mirror its diagnostics on every
  // exit, including an exception; never roll back paid proposal candidates.
  bool miniball(const std::array<i32, 11>& sites, size_t n, LocalBall* ball) {
    struct WorkMirror {
      const meb_proposal_detail::Work& work;
      SilentIncidenceStats& stats;
      ~WorkMirror() noexcept {
        stats.meb_proposal_supports = work.meb_proposal_supports;
        stats.meb_proposal_pivots = work.pivots;
        stats.meb_proposal_certified = work.certified;
        stats.meb_proposal_fallback = work.fallback;
      }
    } mirror{meb_work, out.stats};
    if (caps.max_meb_proposal_supports == 0) {
      // Call the literal F body even when its legacy cap is already reached.
      // This observation matches 0645: no fallback decision after legacy
      // exhaustion; A still measures only candidates actually attempted.
      if (out.stats.meb_supports < caps.max_meb_supports) ++meb_work.fallback;
      return miniball_reference_counted(sites, n, ball);
    }
    if (out.stats.meb_supports >= caps.max_meb_supports) {
      ++out.stats.meb_calls;
      return fail(SilentIncidenceStatus::kResourceExhausted, "silent_meb_support_budget");
    }
    meb_proposal_detail::Candidate candidate;
    meb_proposal_detail::NoObserver observer;
    const meb_proposal_detail::Limits proposal_limits{caps.max_meb_proposal_supports};
    if (meb_work.meb_proposal_supports >= caps.max_meb_proposal_supports ||
        !meb_proposal_detail::propose<false>(
            ix, sites, n, &candidate, proposal_limits, &meb_work, &observer)) {
      ++meb_work.fallback;
      return miniball_reference_counted(sites, n, ball);
    }
    ++meb_work.certified;
    ++out.stats.meb_calls;
    const u64 count = meb_proposal_detail::ordinal(n, candidate);
    const u64 prior = out.stats.meb_supports;
    if (prior >= caps.max_meb_supports || count > caps.max_meb_supports - prior) {
      if (prior < caps.max_meb_supports) out.stats.meb_supports = caps.max_meb_supports;
      return fail(SilentIncidenceStatus::kResourceExhausted, "silent_meb_support_budget");
    }
    out.stats.meb_supports += count;
    *ball = meb_proposal_detail::materialize<LocalBall>(ix, sites, candidate);
    return true;
  }

 private:
  bool miniball_reference_counted(const std::array<i32, 11>& sites, size_t n, LocalBall* ball) {
    struct ReferenceWorkMirror {
      SilentIncidenceStats& stats;
      const u64 prior;
      ~ReferenceWorkMirror() noexcept {
        // The unchanged F body never decreases the legacy counter.
        // For admitted initial A<=legacy, A+delta<=legacy+delta<=UINT64_MAX.
        stats.meb_fallback_supports += stats.meb_supports - prior;
      }
    } mirror{out.stats, out.stats.meb_supports};
    return miniball_reference(sites, n, ball);
  }

  // Un support positif dont la boule contient F satisfait les conditions
  // d'optimalite de la MEB. Le premier suffit : l'unicite de la MEB rend
  // inutile de trier/comparer toutes les boules candidates.
  bool miniball_reference(const std::array<i32, 11>& sites, size_t n, LocalBall* ball) {
    ++out.stats.meb_calls;
    bool found = false;
    const auto accept = [&](const BallKey& key, const ExactLevel& level,
                            const std::array<i32, 4>& support, u8 q) {
      for (size_t i = 0; i < n; ++i)
        if (key.power(ix.upos[(size_t)sites[i]]) > 0) return false;
      ball->key = key;
      ball->level = level;
      ball->support = support;
      ball->q = q;
      return true;
    };
    for (size_t a = 0; a < n && !found; ++a)
      for (size_t b = a + 1; b < n && !found; ++b) {
        if (!charge(out.stats.meb_supports, caps.max_meb_supports, "silent_meb_support_budget")) return false;
        const P3& pa = ix.upos[(size_t)sites[a]];
        const P3& pb = ix.upos[(size_t)sites[b]];
        bool contains = true;
        for (size_t i = 0; i < n; ++i) {
          const P3& z = ix.upos[(size_t)sites[i]];
          // Exact q2 power. Each difference is in [-65535,65535]; all
          // products and partial sums fit i64 (absolute sum < 2^34).
          const i64 power = p3_dot(p3_sub(z, pa), p3_sub(z, pb));
          if (MHGP7_MUTANT("silent-meb-q2-reject-shell") ? power >= 0 : power > 0) {
            contains = false;
            break;
          }
        }
        const auto materialize = [&]() {
#if defined(MHGP7_TESTING)
          auto& observed = meb_materialization_for_tests();
          ++observed.q2_materialized;
          if (!contains) ++observed.q2_rejected_materialized;
#endif
          return std::pair<BallKey, ExactLevel>{q2_ball_key(pa, pb),
              promote_level(q2_exact_level(p3_norm2(p3_sub(pa, pb))))};
        };
        if (!contains) {
#if defined(MHGP7_TESTING)
          ++meb_materialization_for_tests().q2_rejected;
          if (MHGP7_MUTANT("silent-meb-eager-materialization")) (void)materialize();
#endif
          continue;
        }
        const auto built = materialize();
        found = accept(built.first, built.second, {sites[a], sites[b], 0, 0}, 2);
      }
    for (size_t a = 0; a < n && !found; ++a)
      for (size_t b = a + 1; b < n && !found; ++b)
        for (size_t c = b + 1; c < n && !found; ++c) {
          if (!charge(out.stats.meb_supports, caps.max_meb_supports, "silent_meb_support_budget")) return false;
          const P3& pa = ix.upos[(size_t)sites[a]];
          const P3& pb = ix.upos[(size_t)sites[b]];
          const P3& pc = ix.upos[(size_t)sites[c]];
          if (p3_dot(p3_sub(pb, pa), p3_sub(pc, pa)) <= 0 ||
              p3_dot(p3_sub(pa, pb), p3_sub(pc, pb)) <= 0 ||
              p3_dot(p3_sub(pa, pc), p3_sub(pb, pc)) <= 0) continue;
          const Q3Form f = q3_form(pa, pb, pc);
          if (f.g <= 0) continue;
          bool contains = true;
          for (size_t i = 0; i < n; ++i) {
            const i128 power = q3_power(f, ix.upos[(size_t)sites[i]]);
            if (MHGP7_MUTANT("silent-meb-q3-reject-shell") ? power >= 0 : power > 0) {
              contains = false;
              break;
            }
          }
          const auto materialize = [&]() {
#if defined(MHGP7_TESTING)
            auto& observed = meb_materialization_for_tests();
            ++observed.q3_materialized;
            if (!contains) ++observed.q3_rejected_materialized;
#endif
            return std::pair<BallKey, ExactLevel>{q3_ball_key(f), promote_level(q3_exact_level(pa, pb, pc))};
          };
          if (!contains) {
#if defined(MHGP7_TESTING)
            ++meb_materialization_for_tests().q3_rejected;
            if (MHGP7_MUTANT("silent-meb-eager-materialization")) (void)materialize();
#endif
            continue;
          }
          const auto built = materialize();
          found = accept(built.first, built.second, {sites[a], sites[b], sites[c], 0}, 3);
        }
    for (size_t a = 0; a < n && !found; ++a)
      for (size_t b = a + 1; b < n && !found; ++b)
        for (size_t c = b + 1; c < n && !found; ++c)
          for (size_t d = c + 1; d < n && !found; ++d) {
            if (!charge(out.stats.meb_supports, caps.max_meb_supports, "silent_meb_support_budget")) return false;
            const P3& pa = ix.upos[(size_t)sites[a]];
            const P3& pb = ix.upos[(size_t)sites[b]];
            const P3& pc = ix.upos[(size_t)sites[c]];
            const P3& pd = ix.upos[(size_t)sites[d]];
            const Q4Form f = q4_form(pa, pb, pc, pd);
            if (f.det == 0 || !q4_center_strictly_inside(f, pa, pb, pc, pd)) continue;
            bool contains = true;
            for (size_t i = 0; i < n; ++i) {
              const i128 power = q4_power(f, ix.upos[(size_t)sites[i]]);
              if (MHGP7_MUTANT("silent-meb-q4-reject-shell") ? power >= 0 : power > 0) {
                contains = false;
                break;
              }
            }
            const auto materialize = [&]() {
#if defined(MHGP7_TESTING)
              auto& observed = meb_materialization_for_tests();
              ++observed.q4_materialized;
              if (!contains) ++observed.q4_rejected_materialized;
#endif
              return std::pair<BallKey, ExactLevel>{ball_key_reduce(q4_ball_form(f)), q4_level_raw(f)};
            };
            if (!contains) {
#if defined(MHGP7_TESTING)
              ++meb_materialization_for_tests().q4_rejected;
              if (MHGP7_MUTANT("silent-meb-eager-materialization")) (void)materialize();
#endif
              continue;
            }
            const auto built = materialize();
            found = accept(built.first, built.second, {sites[a], sites[b], sites[c], sites[d]}, 4);
          }
    if (!found) return fail(SilentIncidenceStatus::kInvariantViolated, "silent_no_local_miniball");
    size_t shell = 0;
    for (size_t i = 0; i < n; ++i)
      if (ball->key.power(ix.upos[(size_t)sites[i]]) == 0) ++shell;
    if (shell != ball->q)
      return fail(SilentIncidenceStatus::kUnsupportedDegeneracy, "silent_local_nonessential_shell");
    return true;
  }

 public:
  // Deux intrus suffisent, MAIS la requete de bord est achevee pour refuser
  // toute extra-shell. Les sous-arbres strictement interieurs se sautent
  // apres collecte : aucune liste de tous leurs points n'est materialisee.
  bool intruders(const LocalBall& ball, const std::array<i32, 11>& sites,
                 size_t n, std::array<i32, 2>* found, size_t* count) {
    *count = 0;
    const auto member = [&](i32 u) {
      return std::find(sites.begin(), sites.begin() + n, u) != sites.begin() + n;
    };
    const auto note = [&](i32 u) {
      if (*count < 2 && !member(u)) (*found)[(*count)++] = u;
    };
    stack.clear();
    stack.push_back(ix.root());
    const census_detail::AxisBounds bounds{ball.key};
    while (!stack.empty()) {
      if (!charge(out.stats.query_nodes, caps.max_query_nodes, "silent_query_node_budget")) return false;
      const NodeRef node = stack.back();
      stack.pop_back();
      i128 mn = 0, mx = 0;
      bounds.bounds(ix.box_of(node), &mn, &mx);
      if (mn > 0) continue;
      if (mx < 0) {
        ++out.stats.query_range_skips;
        const NodeRange range = ix.range_of(node);
        for (i32 u = range.first; u <= range.last && *count < 2; ++u) note(u);
        continue;
      }
      if (is_leaf(node)) {
        ++out.stats.query_leaves;
        const i32 u = leaf_index(node);
        const i128 power = ball.key.power(ix.upos[(size_t)u]);
        if (power < 0) note(u);
        else if (power == 0 && !member(u))
          return fail(SilentIncidenceStatus::kUnsupportedDegeneracy, "silent_external_shell");
      } else {
        // Ordre Morton croissant, invariant sous permutation physique.
        stack.push_back(ix.nodes[(size_t)node].right);
        stack.push_back(ix.nodes[(size_t)node].left);
      }
    }
    return true;
  }

  ForestEvent event(const CofaceKey& key, const std::array<i32, 11>& sites,
                    const LocalBall& ball) const {
    ForestEvent e;
    e.q = ball.q;
    e.d = (u8)(key.n - ball.q);
    e.active_mask = (u16)((1u << e.q) - 1u);
    e.level = ball.level;
    size_t ns = 0, ni = 0;
    for (size_t i = 0; i < key.n; ++i) {
      if (ball.key.power(ix.upos[(size_t)sites[i]]) == 0) e.support[ns++] = key.p[i];
      else e.interior[ni++] = key.p[i];
    }
    return e;
  }

  bool run() {
    if (!ix.valid || ix.has_duplicate_positions() || ix.keys.size() > (size_t)INT32_MAX)
      return fail(SilentIncidenceStatus::kInvalidInput, "silent_invalid_index");
    if (source.empty()) return true;
    const int K = (int)source[0].q + source[0].d - 1;
    if (K < 1 || K > 10) return fail(SilentIncidenceStatus::kInvalidInput, "silent_invalid_order");
    std::vector<std::pair<CofaceKey, size_t>> catalog;
    std::vector<FacetKey> core;
    ids.reserve(ix.keys.size());
    for (i32 u = 0; u < ix.unique_count(); ++u) ids.emplace_back(ix.point_id(u), u);
    std::sort(ids.begin(), ids.end());
    if ((u64)source.size() > caps.max_core_records)
      return fail(SilentIncidenceStatus::kResourceExhausted, "silent_direct_catalogue_budget");
    catalog.reserve(source.size());
    for (size_t i = 0; i < source.size(); ++i) {
      const ForestEvent& e = source[i];
      if (!fold_event_ok(e, K) || e.q > 4 || e.active_mask != (u16)((1u << e.q) - 1u) || e.level.den <= 0)
        return fail(SilentIncidenceStatus::kUnsupportedDegeneracy, "silent_nonregular_direct_catalogue");
      const CofaceKey key = event_key(e);
      std::array<i32, 11> sites{};
      if (!resolve(key, &sites)) return false;
      catalog.emplace_back(key, i);
      if (K == 1) continue;
      for (size_t drop = 0; drop < e.q; ++drop) {
        if (!charge(out.stats.core_records, caps.max_core_records, "silent_core_record_budget")) return false;
        FacetKey f;
        f.k = (u8)K;
        size_t j = 0;
        for (size_t t = 0; t < key.n; ++t)
          if (key.p[t] != e.support[drop]) f.p[j++] = key.p[t];
        core.push_back(f);
      }
    }
    std::sort(catalog.begin(), catalog.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (size_t i = 1; i < catalog.size(); ++i)
      if (catalog[i - 1].first == catalog[i].first)
        return fail(SilentIncidenceStatus::kInvalidInput, "silent_duplicate_direct_coface");
    if (K == 1) return true;
    std::sort(core.begin(), core.end());
    core.erase(std::unique(core.begin(), core.end()), core.end());
    out.stats.core_facets = core.size();
    const auto direct_at = [&](const CofaceKey& key) -> const ForestEvent* {
      const auto it = std::lower_bound(catalog.begin(), catalog.end(), key,
          [](const auto& item, const CofaceKey& k) { return item.first < k; });
      return it != catalog.end() && it->first == key ? &source[it->second] : nullptr;
    };
    std::unordered_set<CofaceKey, CofaceHash> completed;
    std::vector<CofaceKey> path;
    for (const FacetKey& f : core) {
      CofaceKey key;
      key.n = f.k;
      std::copy(f.p.begin(), f.p.begin() + f.k, key.p.begin());
      std::array<i32, 11> sites{};
      if (!resolve(key, &sites)) return false;
      LocalBall ball;
      if (!miniball(sites, key.n, &ball)) return false;
      std::array<i32, 2> foreign{};
      size_t count = 0;
      if (!intruders(ball, sites, key.n, &foreign, &count)) return false;
      if (count < 2) continue;
      ++out.stats.facets_with_two_intruders;
      key.p[key.n++] = ix.point_id(foreign[0]);
      sort_key(&key);
      path.clear();
      ExactLevel previous{};
      bool has_previous = false;
      while (true) {
        if (!resolve(key, &sites) || !miniball(sites, key.n, &ball)) return false;
        if (has_previous && compare_exact_level(ball.level, previous) >= 0)
          return fail(SilentIncidenceStatus::kInvariantViolated, "silent_descent_not_strict");
        if (completed.find(key) != completed.end()) { ++out.stats.terminal_cached; break; }
        if (const ForestEvent* terminal = direct_at(key)) {
          if (!same_exact_level(terminal->level, ball.level))
            return fail(SilentIncidenceStatus::kInvariantViolated, "silent_terminal_level_mismatch");
          ++out.stats.terminal_direct;
          break;
        }
        if (!intruders(ball, sites, key.n, &foreign, &count)) return false;
        if (count == 0)
          return fail(SilentIncidenceStatus::kInvariantViolated, "silent_terminal_missing_from_catalogue");
        if (!charge(out.stats.chain_steps, caps.max_chain_steps, "silent_chain_step_budget")) return false;
        if (!charge(out.stats.added_cofaces, caps.max_added_cofaces, "silent_added_coface_budget")) return false;
        out.events.push_back(event(key, sites, ball));
        path.push_back(key);
        previous = ball.level;
        has_previous = true;
        const PointId removed = ix.point_id(ball.support[0]);
        const auto at = std::find(key.p.begin(), key.p.begin() + key.n, removed);
        if (at == key.p.begin() + key.n)
          return fail(SilentIncidenceStatus::kInvariantViolated, "silent_support_not_in_coface");
        *at = ix.point_id(foreign[0]);
        sort_key(&key);
      }
      out.stats.max_chain_length = std::max(out.stats.max_chain_length, (u64)path.size());
      for (const CofaceKey& step : path) completed.insert(step);
    }
    std::sort(out.events.begin(), out.events.end(), [](const ForestEvent& a, const ForestEvent& b) {
      const int cmp = compare_exact_level(a.level, b.level);
      return cmp != 0 ? cmp < 0 : event_key(a) < event_key(b);
    });
    if (MHGP7_MUTANT("silent-drop-coface") && !out.events.empty()) out.events.clear();
    return true;
  }

 private:
  const CloudIndex& ix;
  const std::vector<ForestEvent>& source;
  const SilentIncidenceLimits& caps;
  SilentIncidenceResult& out;
  meb_proposal_detail::Work meb_work;
  std::vector<std::pair<PointId, i32>> ids;
  std::vector<NodeRef> stack;
};

}  // namespace silent_detail

inline SilentIncidenceResult build_silent_cofaces(
    const CloudIndex& ix, const std::vector<ForestEvent>& direct_events,
    const SilentIncidenceLimits& limits = {}) {
  SilentIncidenceResult result;
  try {
    silent_detail::Builder builder(ix, direct_events, limits, &result);
    if (!builder.run()) std::vector<ForestEvent>().swap(result.events);
  } catch (const std::bad_alloc&) {
    std::vector<ForestEvent>().swap(result.events);
    result.status = SilentIncidenceStatus::kResourceExhausted;
    result.reason = "silent_allocation_failure";
  }
  return result;
}

}  // namespace mhgp7
