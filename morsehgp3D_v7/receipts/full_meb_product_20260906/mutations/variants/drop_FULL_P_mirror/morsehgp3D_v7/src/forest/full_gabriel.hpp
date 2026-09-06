// FULL horizontal construction relative to supplied COMPLETE, EXACT and
// REGULAR Gabriel catalogues. This module does not certify their producer.
// It retains minima and true merges, not the Gamma core or silent cofaces.
// F geometry helpers are reused without calling their core-building run().
#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "full_certificate.hpp"
#include "meb_proposal.hpp"

namespace mhgp7 {

inline constexpr const char* kFullGabrielAuthority =
    "full_horizontal_relative_to_supplied_complete_exact_regular_gabriel_catalogues";
inline constexpr const char* kFullGabrielEagerAliases = "eager_all_incident_facets_v1";
inline constexpr const char* kFullGabrielLazyAliases = "lazy_first_c_strict_resolutions_v1";
// This is a work/admission version, not a change to the FULL forest authority.
inline constexpr const char* kFullGabrielSuccessorAccounting =
    "full_successor_reads_writes_no_last_pair_v2";
// Independent of successor/alias calendars. The legacy support count is an
// ordinal prefix when a proposal certifies; physical work is reported apart.
inline constexpr const char* kFullGabrielMebAccounting =
    meb_proposal_detail::kWorkAccounting;

enum class FullGabrielStatus {
  kCompleteRelative, kInvalidInput, kUnsupportedDegeneracy,
  kResourceExhausted, kInvariantViolated
};

struct FullGabrielLimits {
  FullCertificateLimits certificate{};
  u64 max_points = 0, max_input_records = 0, max_aliases = 0;
  u64 max_face_visits = 0, max_portal_requests = 0, max_chain_steps = 0;
  u64 max_meb_calls = 0, max_query_nodes = 0, max_meb_supports = 0;
  u64 max_successor_steps = 0;
  // Opt-in, shared by every MEB of this order. Exhaustion falls back to F.
  u64 max_meb_proposal_supports = 0;
};

// A separate optional-cache contract, never a reinterpretation of max_aliases.
// Retain the first C resolved non-minimum strict facets. When full (including
// C=0), resolve without inserting; there is no eviction or budget reset.
struct FullGabrielCacheLimits {
  u64 max_entries = 0;
};

struct FullGabrielStats {
  u64 input_records = 0, face_visits = 0, aliases = 0, alias_hits = 0;
  u64 portal_requests = 0, chain_steps = 0, terminal_direct = 0;
  u64 max_chain_length = 0, normalized_anchors = 0, successor_steps = 0;
  u64 no_op_connections = 0, meb_calls = 0;
  u64 minimum_lookups = 0, minimum_hits = 0;
  u64 cache_lookups = 0, cache_hits = 0, cache_inserts = 0, cache_skips = 0;
  u64 singleton_intruder_resolutions = 0, direct_lookups = 0;
  SilentIncidenceStats geometry{};
  meb_proposal_detail::Work meb_proposal{};
};

struct FullGabrielResult {
  FullGabrielStatus status = FullGabrielStatus::kInvalidInput;
  const char* reason = "full_gabriel_uninitialized";
  const char* alias_policy = kFullGabrielEagerAliases;
  const char* successor_accounting = kFullGabrielSuccessorAccounting;
  const char* meb_accounting = kFullGabrielMebAccounting;
  FullCertificate forest;
  FullGabrielStats stats;
};

namespace full_gabriel_detail {

using CofaceKey = silent_detail::CofaceKey;
using LocalBall = silent_detail::LocalBall;
inline constexpr FullNodeId kAbsent = std::numeric_limits<FullNodeId>::max();

#ifdef MHGP7_TESTING
// Private branch observations and differential control. Absent from product
// builds; never a caller option or an alternate scientific authority.
struct SingletonLotTestState {
  bool force_general = false;
  u64 eligible[5]{}, unique_roots[5]{};
  u64 repeated_roots = 0, simultaneous_births = 0, multi_direct_lots = 0;
  u64 specialized_lots = 0, general_singleton_lots = 0;
};
inline thread_local SingletonLotTestState* singleton_lot_test_state = nullptr;
inline thread_local bool force_legacy_successor = false;
#endif

enum class SuccessorStatus { kOk, kUnknownAnchor, kBudget };

// Internal successor forest: IDs and acyclicity are owned by Builder. The
// terminal read is charged; on each successful nontrivial walk the last node
// already points to root, so its compression read/write pair is unnecessary.
// Preserve every other write, even an idempotent one earlier in the path.
inline SuccessorStatus normalize_successor(std::vector<FullNodeId>& successor,
    FullNodeId token, FullNodeId& root, u64& steps, u64& normalized_anchors,
    u64 cap) {
  if (token >= successor.size()) return SuccessorStatus::kUnknownAnchor;
  const auto charge = [&]() {
    if (steps >= cap) return false;
    ++steps;
    return true;
  };
  root = token;
  FullNodeId last = token;
  while (true) {
    if (!charge()) return SuccessorStatus::kBudget;
    const FullNodeId next = successor[static_cast<size_t>(root)];
    if (next == root) break;
    last = root;
    root = next;
  }
  if (root != token) ++normalized_anchors;  // Bounded by charged reads.
  FullNodeId stop = last;
#ifdef MHGP7_TESTING
  // Differential witness only; no old-calendar switch in product builds.
  if (force_legacy_successor) stop = root;
#endif
  // Charge prospectively, including a read whose following write may refuse.
  // At depth zero token == last; at depth one there is no compression write.
  while (token != stop) {
    if (!charge()) return SuccessorStatus::kBudget;
    const FullNodeId next = successor[static_cast<size_t>(token)];
    if (!charge()) return SuccessorStatus::kBudget;
    successor[static_cast<size_t>(token)] = root;
    token = next;
  }
  return SuccessorStatus::kOk;
}

struct FacetHash {
  size_t operator()(const FacetKey& key) const {
    u64 h = 1469598103934665603ull ^ key.k;
    for (size_t i = 0; i < key.k; ++i) {
      h ^= key.p[i];
      h *= 1099511628211ull;
    }
    return static_cast<size_t>(h);
  }
};

struct Record {
  CofaceKey key;
  const ForestEvent* event = nullptr;
  FullNodeId token = kAbsent;
};

inline FacetKey facet_without(const CofaceKey& key, PointId removed) {
  FacetKey f;
  f.k = static_cast<u8>(key.n - 1);
  size_t n = 0;
  for (size_t i = 0; i < key.n; ++i)
    if (key.p[i] != removed) f.p[n++] = key.p[i];
  return f;
}

// This owner is local to one call. It is neither copied nor moved: geometry
// has references to its immutable input, persistent budgets and private stats.
class Builder {
 public:
  Builder(const CloudIndex& index, unsigned order,
          const std::vector<ForestEvent>& minima,
          const std::vector<ForestEvent>& connections,
          const FullGabrielLimits& limits, FullGabrielResult& result,
          const FullGabrielCacheLimits* cache_limits = nullptr)
      : ix(index), k(order), minimum_source(minima), direct_source(connections),
        caps(limits), cache_caps(cache_limits), out(result),
        geometry_caps{0, 0, 0, limits.max_query_nodes, limits.max_meb_supports},
        geometry(index, empty_source, geometry_caps, &geometry_result) {
#ifdef MHGP7_TESTING
    if (force_legacy_successor)
      out.successor_accounting = "full_successor_reads_writes_v1";
#endif
  }
  Builder(const Builder&) = delete;
  Builder& operator=(const Builder&) = delete;
  ~Builder() {
    // Publish on every exit, including unwinding before a public wrapper
    // catches an allocation failure. These mirrors never reset live budgets.
    out.stats.geometry = geometry_result.stats;
    // Private mutation: omit the external FULL Work mirror.
  }

  bool run() {
    if (cache_caps && caps.max_aliases != 0)
      return fail(FullGabrielStatus::kInvalidInput, "full_gabriel_lazy_alias_budget_conflict");
    // CloudIndex must come from the checked index constructor and remain
    // immutable. Its public internals are not an untrusted wire format.
    if (!ix.valid || ix.keys.empty() || ix.has_duplicate_positions() ||
        ix.keys.size() > static_cast<size_t>(INT32_MAX) ||
        k < 1 || k > 10 || k > ix.keys.size())
      return fail(FullGabrielStatus::kInvalidInput, "full_gabriel_invalid_index_or_order");
    if (ix.keys.size() > caps.max_points)
      return resource("full_gabriel_point_budget");
    if (k == 1 && !minimum_source.empty())
      return fail(FullGabrielStatus::kInvalidInput, "full_gabriel_k1_minimum_catalogue");
    if (k > 1 && minimum_source.empty())
      return fail(FullGabrielStatus::kInvalidInput, "full_gabriel_missing_minima");
    if (!full_certificate_detail::add(out.stats.input_records, minimum_source.size(), caps.max_input_records) ||
        !full_certificate_detail::add(out.stats.input_records, direct_source.size(), caps.max_input_records))
      return resource("full_gabriel_input_budget");

    ids.reserve(ix.keys.size());
    points.reserve(ix.keys.size());
    for (i32 u = 0; u < ix.unique_count(); ++u) ids.emplace_back(ix.point_id(u), u);
    std::sort(ids.begin(), ids.end());
    for (const auto& entry : ids) points.push_back(entry.first);
    if (!read_catalogue(minimum_source, k, minima) ||
        !read_catalogue(direct_source, k + 1, direct)) return false;
    chronological(minima, minimum_order);
    chronological(direct, direct_order);

    if (k == 1) {
      FullBatch batch;
      if (!node_room(points.size())) return false;
      for (PointId p : points) {
        FacetKey f;
        f.k = 1;
        f.p[0] = p;
        batch.births.push_back(f);
      }
      if (!install_births(batch.births) || !keep_batch(std::move(batch))) return false;
    }

    size_t m = 0, d = 0;
    while (m < minimum_order.size() || d < direct_order.size()) {
      ExactLevel level;
      if (d == direct_order.size() ||
          (m < minimum_order.size() && compare_exact_level(
               minima[minimum_order[m]].event->level, direct[direct_order[d]].event->level) <= 0))
        level = minima[minimum_order[m]].event->level;
      else level = direct[direct_order[d]].event->level;
      size_t me = m, de = d;
      while (me < minimum_order.size() &&
             same_exact_level(minima[minimum_order[me]].event->level, level)) ++me;
      while (de < direct_order.size() &&
             same_exact_level(direct[direct_order[de]].event->level, level)) ++de;
      if (!lot(level, m, me, d, de)) return false;
      m = me;
      d = de;
    }

    auto built = build_full_certificate(k, points, batches, caps.certificate);
    if (built.status != FullCertificateStatus::kOk) {
      if (std::strcmp(built.reason, "full_allocation_failed") == 0)
        return resource("full_gabriel_allocation_failed");
      if (std::strcmp(built.reason, "full_size_overflow") == 0)
        return resource("full_gabriel_size_overflow");
      return fail(built.status == FullCertificateStatus::kResourceExhausted
                      ? FullGabrielStatus::kResourceExhausted : FullGabrielStatus::kInvariantViolated,
                  built.reason);
    }
    out.forest = std::move(built.value);
    out.status = FullGabrielStatus::kCompleteRelative;
    out.reason = kFullGabrielAuthority;
    return true;
  }

 private:
  const CloudIndex& ix;
  unsigned k;
  const std::vector<ForestEvent>& minimum_source;
  const std::vector<ForestEvent>& direct_source;
  const FullGabrielLimits& caps;
  const FullGabrielCacheLimits* cache_caps;
  FullGabrielResult& out;
  std::vector<ForestEvent> empty_source;
  SilentIncidenceLimits geometry_caps;
  SilentIncidenceResult geometry_result;
  silent_detail::Builder geometry;
  meb_proposal_detail::Work meb_work;
  std::vector<std::pair<PointId, i32>> ids;
  std::vector<PointId> points;
  std::vector<Record> minima, direct;
  std::vector<size_t> minimum_order, direct_order;
  std::unordered_map<FacetKey, FullNodeId, FacetHash> aliases;
  std::vector<FullNodeId> successor;
  std::vector<FullBatch> batches;
  u64 parent_refs = 0;

  bool fail(FullGabrielStatus status, const char* reason) {
    out.status = status;
    out.reason = reason;
    return false;
  }
  bool resource(const char* reason) { return fail(FullGabrielStatus::kResourceExhausted, reason); }
  bool invariant(const char* reason) { return fail(FullGabrielStatus::kInvariantViolated, reason); }
  bool charge(u64& count, u64 cap, const char* reason) {
    if (count >= cap) return resource(reason);
    ++count;
    return true;
  }
  bool node_room(size_t extra) {
    u64 count = successor.size();
    return full_certificate_detail::add(count, extra, caps.certificate.max_nodes) ||
           resource("full_gabriel_node_budget");
  }
  bool resolve(const CofaceKey& key, std::array<i32, 11>& sites) {
    for (size_t i = 0; i < key.n; ++i) {
      const auto it = std::lower_bound(ids.begin(), ids.end(), key.p[i],
          [](const auto& item, PointId p) { return item.first < p; });
      if (it == ids.end() || it->first != key.p[i])
        return fail(FullGabrielStatus::kInvalidInput, "full_gabriel_unknown_point");
      sites[i] = it->second;
    }
    return true;
  }
  bool read_catalogue(const std::vector<ForestEvent>& source, unsigned cardinal,
                      std::vector<Record>& records) {
    records.reserve(source.size());
    for (const auto& e : source) {
      if (!fold_event_ok(e, static_cast<int>(cardinal) - 1) || e.q > 4 ||
          e.active_mask != static_cast<u16>((1u << e.q) - 1u) || e.level.den <= 0 ||
          full_certificate_detail::zero(e.level))
        return fail(FullGabrielStatus::kInvalidInput, "full_gabriel_invalid_record");
      CofaceKey key = silent_detail::event_key(e);
      std::array<i32, 11> sites{};
      if (!resolve(key, sites)) return false;
      records.push_back({key, &e, kAbsent});
    }
    std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) { return a.key < b.key; });
    for (size_t i = 1; i < records.size(); ++i)
      if (records[i - 1].key == records[i].key)
        return fail(FullGabrielStatus::kInvalidInput, "full_gabriel_duplicate_record");
    return true;
  }
  static void chronological(const std::vector<Record>& records, std::vector<size_t>& order) {
    order.resize(records.size());
    std::iota(order.begin(), order.end(), size_t{0});
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      const int cmp = compare_exact_level(records[a].event->level, records[b].event->level);
      return cmp != 0 ? cmp < 0 : records[a].key < records[b].key;
    });
  }
  bool normalize(FullNodeId token, FullNodeId& root) {
    switch (normalize_successor(successor, token, root, out.stats.successor_steps,
                               out.stats.normalized_anchors, caps.max_successor_steps)) {
      case SuccessorStatus::kOk: return true;
      case SuccessorStatus::kUnknownAnchor: return invariant("full_gabriel_unknown_anchor");
      case SuccessorStatus::kBudget: return resource("full_gabriel_successor_budget");
    }
    return invariant("full_gabriel_successor_status");
  }
  bool put_alias(const FacetKey& f, FullNodeId token) {
    const auto it = aliases.find(f);
    if (it != aliases.end()) {
      FullNodeId current = kAbsent;
      if (!normalize(it->second, current)) return false;
      return current == token || invariant("full_gabriel_inconsistent_alias");
    }
    if (cache_caps) {
      if (aliases.size() >= cache_caps->max_entries) {
        ++out.stats.cache_skips;  // Bounded by charged portal requests.
        return true;
      }
      // Admission before allocation, retained even if emplace throws.
      ++out.stats.cache_inserts;
    } else if (!charge(out.stats.aliases, caps.max_aliases, "full_gabriel_alias_budget")) return false;
    aliases.emplace(f, token);
    return true;
  }
  bool install_births(const std::vector<FacetKey>& births) {
    if (!node_room(births.size())) return false;
    for (const FacetKey& f : births) {
      if (!cache_caps && aliases.find(f) != aliases.end())
        return invariant("full_gabriel_late_minimum");
      const FullNodeId id = successor.size();
      successor.push_back(id);
      // Lazy minima are mandatory catalogue tokens, not optional cache entries.
      // At K1 their IDs are exactly the offsets in the sorted points array.
      if (!cache_caps && !put_alias(f, id)) return false;
    }
    return true;
  }
  bool keep_batch(FullBatch&& batch) {
    if (batch.births.empty() && batch.merges.empty()) return true;
    if (batches.size() >= caps.certificate.max_batches) return resource("full_gabriel_batch_budget");
    batches.push_back(std::move(batch));
    return true;
  }
  bool geometry_failed() {
    switch (geometry_result.status) {
      case SilentIncidenceStatus::kUnsupportedDegeneracy:
        return fail(FullGabrielStatus::kUnsupportedDegeneracy, geometry_result.reason);
      case SilentIncidenceStatus::kResourceExhausted:
        return resource(geometry_result.reason);
      case SilentIncidenceStatus::kInvalidInput:
        return fail(FullGabrielStatus::kInvalidInput, geometry_result.reason);
      default:
        return invariant(geometry_result.reason);
    }
  }
  bool miniball(const std::array<i32, 11>& sites, size_t n, LocalBall& ball) {
    if (!charge(out.stats.meb_calls, caps.max_meb_calls, "full_gabriel_meb_call_budget")) return false;
    meb_proposal_detail::NoObserver observer;
    return meb_proposal_detail::miniball(ix, geometry, geometry_caps,
        &geometry_result, sites, n, &ball,
        meb_proposal_detail::Limits{caps.max_meb_proposal_supports},
        &meb_work, &observer) || geometry_failed();
  }
  bool intruders(const LocalBall& ball, const std::array<i32, 11>& sites,
                 size_t n, std::array<i32, 2>& foreign, size_t& count) {
    return geometry.intruders(ball, sites, n, &foreign, &count) || geometry_failed();
  }

  bool minimum_anchor(const FacetKey& f, const ExactLevel& cut,
                      FullNodeId prior_count, FullNodeId& root, bool& found) {
    ++out.stats.minimum_lookups;  // At most the charged strict face visits.
    found = false;
    FullNodeId token = kAbsent;
    if (k == 1) {
      const auto it = std::lower_bound(points.begin(), points.end(), f.p[0]);
      if (it == points.end() || *it != f.p[0])
        return invariant("full_gabriel_missing_point_minimum");
      token = static_cast<FullNodeId>(it - points.begin());
    } else {
      CofaceKey key;
      key.n = f.k;
      std::copy_n(f.p.begin(), f.k, key.p.begin());
      const auto it = std::lower_bound(minima.begin(), minima.end(), key,
          [](const Record& item, const CofaceKey& sought) { return item.key < sought; });
      if (it == minima.end() || !(it->key == key)) return true;
      if (compare_exact_level(it->event->level, cut) >= 0)
        return invariant("full_gabriel_minimum_not_prior");
      token = it->token;
    }
    if (token == kAbsent || token >= prior_count)
      return invariant("full_gabriel_minimum_not_prior");
    if (!normalize(token, root)) return false;
    if (root >= prior_count) return invariant("full_gabriel_nonprior_anchor");
    ++out.stats.minimum_hits;
    found = true;
    return true;
  }

  bool direct_anchor(const CofaceKey& key, const ExactLevel& level,
                     const ExactLevel& cut, FullNodeId prior_count,
                     FullNodeId& root, bool& found) {
    ++out.stats.direct_lookups;  // At most one per charged MEB invocation.
    const auto it = std::lower_bound(direct.begin(), direct.end(), key,
        [](const Record& item, const CofaceKey& sought) { return item.key < sought; });
    found = it != direct.end() && it->key == key;
    if (!found) return true;
    if (!same_exact_level(it->event->level, level))
      return invariant("full_gabriel_terminal_level_mismatch");
    if (compare_exact_level(it->event->level, cut) >= 0 || it->token == kAbsent ||
        it->token >= prior_count)
      return invariant("full_gabriel_terminal_not_prior");
    if (!normalize(it->token, root)) return false;
    return root < prior_count || invariant("full_gabriel_nonprior_anchor");
  }

  // Only STRICT facets enter. Eager requires a prior alias for J<=1.
  // Lazy first checks mandatory minima, then its optional cache; a J=1 miss
  // resolves through the closed anchor of F plus its unique global intruder.
  bool locate(const FacetKey& f, const ExactLevel& cut, FullNodeId prior_count, FullNodeId& root) {
    if (cache_caps) {
      bool found = false;
      if (!minimum_anchor(f, cut, prior_count, root, found)) return false;
      if (found) return true;
      ++out.stats.cache_lookups;
    }
    const auto known = aliases.find(f);
    if (known != aliases.end()) {
      if (cache_caps) ++out.stats.cache_hits;
      else ++out.stats.alias_hits;  // At most the charged strict face visits.
      if (!normalize(known->second, root)) return false;
      return root < prior_count || invariant("full_gabriel_nonprior_anchor");
    }
    if (k == 1) return invariant("full_gabriel_missing_point_minimum");
    if (!charge(out.stats.portal_requests, caps.max_portal_requests,
                "full_gabriel_portal_budget")) return false;
    CofaceKey key;
    key.n = f.k;
    std::copy_n(f.p.begin(), f.k, key.p.begin());
    std::array<i32, 11> sites{};
    LocalBall ball;
    std::array<i32, 2> foreign{};
    size_t count = 0;
    if (!resolve(key, sites) || !miniball(sites, key.n, ball)) return false;
    if (compare_exact_level(ball.level, cut) >= 0)
      return invariant("full_gabriel_facet_not_strict");
    if (!intruders(ball, sites, key.n, foreign, count)) return false;
    if (count < 2 && !cache_caps) return invariant("full_gabriel_missing_prior_alias");
    // A missing minimum is an authority failure, never a late birth. Under
    // the exact catalogue premise it would have been found before the MEB.
    if (count == 0) return invariant("full_gabriel_minimum_missing");

    if (count == 1) {
      key.p[key.n++] = ix.point_id(foreign[0]);
      silent_detail::sort_key(&key);
      bool found = false;
      if (!direct_anchor(key, ball.level, cut, prior_count, root, found)) return false;
      if (!found) return invariant("full_gabriel_terminal_missing");
      ++out.stats.singleton_intruder_resolutions;
      ++out.stats.terminal_direct;
      // The finished census of F proves the same ball/support for F+z.
      // No second MEB, census, chain step, or same-lot parent is introduced.
      return put_alias(f, root);
    }

    // Q0 = F+z has exactly the certified ball/support of F. The other
    // certified intruder w remains foreign: no MEB or boundary rescan of Q0.
    key.p[key.n++] = ix.point_id(foreign[0]);
    silent_detail::sort_key(&key);
    i32 next_intruder = foreign[1];
    u64 length = 0;
    while (true) {
      if (!charge(out.stats.chain_steps, caps.max_chain_steps,
                  "full_gabriel_chain_budget")) return false;
      ++length;
      out.stats.max_chain_length = std::max(out.stats.max_chain_length, length);
      const ExactLevel previous = ball.level;
      const PointId removed = ix.point_id(ball.support[0]);
      const auto at = std::find(key.p.begin(), key.p.begin() + key.n, removed);
      if (at == key.p.begin() + key.n) return invariant("full_gabriel_support_not_in_coface");
      *at = ix.point_id(next_intruder);
      silent_detail::sort_key(&key);
      if (!resolve(key, sites) || !miniball(sites, key.n, ball)) return false;
      if (compare_exact_level(ball.level, previous) >= 0)
        return invariant("full_gabriel_descent_not_strict");
      bool found = false;
      if (!direct_anchor(key, ball.level, cut, prior_count, root, found)) return false;
      if (found) {
        ++out.stats.terminal_direct;  // One terminal per charged request.
        return put_alias(f, root);
      }
      if (!intruders(ball, sites, key.n, foreign, count)) return false;
      if (count == 0) return invariant("full_gabriel_terminal_missing");
      next_intruder = foreign[0];
    }
  }

  bool lot(const ExactLevel& level, size_t mb, size_t me, size_t db, size_t de) {
    const FullNodeId prior_count = successor.size();
    bool single = de - db == 1;
#ifdef MHGP7_TESTING
    if (singleton_lot_test_state && singleton_lot_test_state->force_general) single = false;
#endif
    std::array<FullNodeId, 4> single_roots{};
    size_t single_count = 0;
    std::vector<std::vector<FullNodeId>> groups;
    if (single) {
      // read_catalogue already checked 2<=q<=4. All requests stay in support
      // order, even when they resolve to the same root: first-C and charges
      // depend on this sequence. Keep the first token before any sorting.
      Record& r = direct[direct_order[db]];
      single_count = r.event->q;
      for (size_t drop = 0; drop < r.event->q; ++drop) {
        if (!charge(out.stats.face_visits, caps.max_face_visits, "full_gabriel_face_budget")) return false;
        const FacetKey f = facet_without(r.key, r.event->support[drop]);
        if (!locate(f, level, prior_count, single_roots[drop])) return false;
        if (drop == 0) r.token = single_roots[drop];
      }
    } else {
      // General local DSU joins DIRECT connections through strict old roots.
      // Distinct regular Gabriel cofaces at one level cannot share an equal
      // facet: uniqueness of the MEB would leave a foreign point in one ball.
      std::unordered_map<FullNodeId, size_t> local_ids;
      std::vector<FullNodeId> old_roots;
      std::vector<size_t> parent;
      const auto find = [&](size_t u) {
        size_t r = u;
        while (parent[r] != r) r = parent[r];
        while (parent[u] != u) { const size_t next = parent[u]; parent[u] = r; u = next; }
        return r;
      };
      const auto intern = [&](FullNodeId root) {
        const auto [it, fresh] = local_ids.emplace(root, old_roots.size());
        if (fresh) { old_roots.push_back(root); parent.push_back(parent.size()); }
        return it->second;
      };
      for (size_t t = db; t < de; ++t) {
        Record& r = direct[direct_order[t]];
        size_t first = 0;
        for (size_t drop = 0; drop < r.event->q; ++drop) {
          if (!charge(out.stats.face_visits, caps.max_face_visits, "full_gabriel_face_budget")) return false;
          const FacetKey f = facet_without(r.key, r.event->support[drop]);
          FullNodeId root = kAbsent;
          if (!locate(f, level, prior_count, root)) return false;
          const size_t local = intern(root);
          if (drop == 0) { first = local; r.token = root; }
          else {
            const size_t a = find(first), b = find(local);
            if (a != b) parent[std::max(a, b)] = std::min(a, b);
          }
        }
      }
      groups.resize(old_roots.size());
      for (size_t i = 0; i < old_roots.size(); ++i) groups[find(i)].push_back(old_roots[i]);
    }
    FullBatch batch;
    batch.level = level;
    for (size_t i = mb; i < me; ++i) {
      const CofaceKey& key = minima[minimum_order[i]].key;
      FacetKey f;
      f.k = static_cast<u8>(k);
      std::copy_n(key.p.begin(), k, f.p.begin());
      batch.births.push_back(f);
    }
    if (single) {
      // Insertion sort on at most four roots avoids any auxiliary allocation.
      for (size_t i = 1; i < single_count; ++i) {
        const FullNodeId value = single_roots[i];
        size_t j = i;
        while (j > 0 && single_roots[j - 1] > value) {
          single_roots[j] = single_roots[j - 1];
          --j;
        }
        single_roots[j] = value;
      }
      const auto end = std::unique(single_roots.begin(), single_roots.begin() + single_count);
      single_count = static_cast<size_t>(end - single_roots.begin());
    }
#ifdef MHGP7_TESTING
    if (singleton_lot_test_state) {
      auto& test = *singleton_lot_test_state;
      if (de - db == 1) {
        if (single) ++test.specialized_lots;
        else ++test.general_singleton_lots;
        const size_t q = direct[direct_order[db]].event->q;
        size_t unique_count = single_count;
        if (!single) for (const auto& group : groups) unique_count += group.size();
        ++test.eligible[q];
        ++test.unique_roots[unique_count];
        if (unique_count < q) ++test.repeated_roots;
        if (mb < me) ++test.simultaneous_births;
      } else if (de - db > 1) ++test.multi_direct_lots;
    }
#endif
    if (single) {
      // The sole local equivalence class is the set of returned old roots.
      // A no-op still reaches the common closed-anchor suffix below.
      if (single_count >= 2) {
        if (!full_certificate_detail::add(parent_refs, single_count, caps.certificate.max_parent_refs))
          return resource("full_gabriel_parent_budget");
        batch.merges.emplace_back(single_roots.begin(), single_roots.begin() + single_count);
      }
    } else {
      for (auto& group : groups) {
        if (group.size() < 2) continue;
        std::sort(group.begin(), group.end());
        if (!full_certificate_detail::add(parent_refs, group.size(), caps.certificate.max_parent_refs))
          return resource("full_gabriel_parent_budget");
        batch.merges.push_back(std::move(group));
      }
    }
    std::sort(batch.merges.begin(), batch.merges.end());
    if (!node_room(batch.births.size()) || !install_births(batch.births) ||
        !node_room(batch.merges.size())) return false;
    if (cache_caps)
      for (size_t i = mb; i < me; ++i)
        minima[minimum_order[i]].token = prior_count + static_cast<FullNodeId>(i - mb);
    for (const auto& group : batch.merges) {
      const FullNodeId id = successor.size();
      successor.push_back(id);
      for (FullNodeId p : group) successor[static_cast<size_t>(p)] = id;
    }
    // Close the ENTIRE lot before assigning new aliases or direct anchors.
    // Known aliases keep their historical tokens; the successor map suffices.
    for (size_t t = db; t < de; ++t) {
      Record& r = direct[direct_order[t]];
      FullNodeId root = kAbsent;
      if (!normalize(r.token, root)) return false;
      r.token = root;
      if (root < prior_count) ++out.stats.no_op_connections;
      // Lazy keeps this mandatory closed anchor, including no-op connections,
      // but does not enumerate or install the direct coface's K+1 aliases.
      if (cache_caps) continue;
      for (size_t drop = 0; drop < r.key.n; ++drop) {
        if (!charge(out.stats.face_visits, caps.max_face_visits, "full_gabriel_face_budget")) return false;
        if (!put_alias(facet_without(r.key, r.key.p[drop]), root)) return false;
      }
    }
    return keep_batch(std::move(batch));
  }
};

}  // namespace full_gabriel_detail

// minima: cardinal K (empty at K1, where point minima are implicit).
// connections: cardinal K+1 (empty at terminal K=n). Supplied catalogues
// must be complete/exact/regular under their own scientific source authority.
inline FullGabrielResult build_full_gabriel_order(
    const CloudIndex& ix, unsigned k, const std::vector<ForestEvent>& minima,
    const std::vector<ForestEvent>& connections, const FullGabrielLimits& limits) {
  FullGabrielResult out;
  try {
    full_gabriel_detail::Builder builder(ix, k, minima, connections, limits, out);
    if (!builder.run()) out.forest = FullCertificate{};
  } catch (const std::bad_alloc&) {
    out.forest = FullCertificate{};
    out.status = FullGabrielStatus::kResourceExhausted;
    out.reason = "full_gabriel_allocation_failed";
  } catch (const std::length_error&) {
    out.forest = FullCertificate{};
    out.status = FullGabrielStatus::kResourceExhausted;
    out.reason = "full_gabriel_size_overflow";
  }
  return out;
}

// Distinct optional-cache API. max_aliases MUST be zero: it is reserved for
// the eager API above. Cache capacity zero is supported, not an error. All
// other work limits and failure/authority semantics apply to the whole call.
inline FullGabrielResult build_full_gabriel_order_lazy(
    const CloudIndex& ix, unsigned k, const std::vector<ForestEvent>& minima,
    const std::vector<ForestEvent>& connections, const FullGabrielLimits& limits,
    const FullGabrielCacheLimits& cache_limits) {
  FullGabrielResult out;
  out.alias_policy = kFullGabrielLazyAliases;
  try {
    full_gabriel_detail::Builder builder(ix, k, minima, connections, limits, out, &cache_limits);
    if (!builder.run()) out.forest = FullCertificate{};
  } catch (const std::bad_alloc&) {
    out.forest = FullCertificate{};
    out.status = FullGabrielStatus::kResourceExhausted;
    out.reason = "full_gabriel_allocation_failed";
  } catch (const std::length_error&) {
    out.forest = FullCertificate{};
    out.status = FullGabrielStatus::kResourceExhausted;
    out.reason = "full_gabriel_size_overflow";
  }
  return out;
}

}  // namespace mhgp7
