// Singleton-lot specialization: exact differential and an independent Gamma
// oracle on real generated, globally regular catalogues. The retained general
// path is a differential ONLY. Helper provenance is declared in the header.
// New tetrahedral and mixed-birth fixtures were admitted by a bounded private
// oracle-only search in build/v7_singleton_20260905_testdev/fixture_search.cpp.
// They are admitted again here; no search observation is inherited as a gate.
#include <cstring>
#include <exception>

#include "full_gabriel_singleton_fixtures.hpp"

#if !defined(MHGP7_TESTING)
#error "singleton qualification requires the private test-only observation hook"
#endif

using namespace mhgp7_singleton_test;
namespace {
using State = full_gabriel_detail::SingletonLotTestState;
u64 pairs = 0, positive_pairs = 0, refused_pairs = 0, order_cases = 0;
u64 permutations = 0, terminal_orders = 0, named_noop_future = 0, named_proper_repeated = 0;
u64 exact_caps = 0, short_caps = 0, partial_face_caps = 0, repeated_parent_caps = 0;
State observed;

bool same_state(const State& a, const State& b) {
  return std::equal(std::begin(a.eligible), std::end(a.eligible), std::begin(b.eligible)) &&
      std::equal(std::begin(a.unique_roots), std::end(a.unique_roots), std::begin(b.unique_roots)) &&
      a.repeated_roots == b.repeated_roots && a.simultaneous_births == b.simultaneous_births &&
      a.multi_direct_lots == b.multi_direct_lots;
}
void accumulate(const State& s) {
  for (size_t i = 0; i < 5; ++i) { observed.eligible[i] += s.eligible[i]; observed.unique_roots[i] += s.unique_roots[i]; }
  observed.repeated_roots += s.repeated_roots;
  observed.simultaneous_births += s.simultaneous_births;
  observed.multi_direct_lots += s.multi_direct_lots;
  observed.specialized_lots += s.specialized_lots;
  observed.general_singleton_lots += s.general_singleton_lots;
}
struct Hook {
  State* prior = full_gabriel_detail::singleton_lot_test_state;
  explicit Hook(State& s) { full_gabriel_detail::singleton_lot_test_state = &s; }
  ~Hook() { full_gabriel_detail::singleton_lot_test_state = prior; }
  Hook(const Hook&) = delete;
  Hook& operator=(const Hook&) = delete;
};
FullGabrielResult invoke(const CloudIndex& ix, unsigned k,
                         const std::vector<ForestEvent>& minima,
                         const std::vector<ForestEvent>& direct,
                         bool lazy, u64 cache, const FullGabrielLimits& caps, State& state) {
  Hook hook(state);
  return lazy ? build_full_gabriel_order_lazy(ix, k, minima, direct, caps, {cache}) :
                build_full_gabriel_order(ix, k, minima, direct, caps);
}
FullGabrielResult paired(const CloudIndex& ix, unsigned k,
                        const std::vector<ForestEvent>& minima,
                        const std::vector<ForestEvent>& direct,
                        bool lazy, u64 cache, const FullGabrielLimits& caps,
                        State* output = nullptr) {
  State fast, general;
  general.force_general = true;
  auto a = invoke(ix, k, minima, direct, lazy, cache, caps, fast);
  auto b = invoke(ix, k, minima, direct, lazy, cache, caps, general);
  ++pairs;
  check(a.status == b.status && a.reason && b.reason && std::strcmp(a.reason, b.reason) == 0,
        "specialized/general status and exact reason agree");
  check(a.alias_policy && b.alias_policy && std::strcmp(a.alias_policy, b.alias_policy) == 0 &&
        std::strcmp(a.alias_policy, lazy ? kFullGabrielLazyAliases : kFullGabrielEagerAliases) == 0,
        "both paths preserve the explicit API policy");
  check(same_forest(a.forest, b.forest), "literal nodes, levels, minima and parent CSR agree");
  check(work(a.stats) == work(b.stats), "ALL 33 existing FULL and geometry counters agree, also on refusals");
  check(same_state(fast, general), "same logical lot-observation prefix in fast and forced-general paths");
  u64 eligible = 0;
  for (u64 v : fast.eligible) eligible += v;
  // Zero is legitimate for K=n or a refusal before the last locate. The
  // aggregate floor below separately requires actual specialized execution.
  check(fast.specialized_lots == eligible && fast.general_singleton_lots == 0 &&
        general.specialized_lots == 0 && general.general_singleton_lots == eligible,
        "specialized and forced-general arms actually execute their requested branch");
  check(full_gabriel_detail::singleton_lot_test_state == nullptr, "private observer restored after both calls");
  if (a.status == FullGabrielStatus::kCompleteRelative) {
    ++positive_pairs;
    check(a.reason && std::strcmp(a.reason, kFullGabrielAuthority) == 0,
          "completion remains relative to supplied complete exact regular catalogues");
  } else {
    ++refused_pairs;
    check(empty(a.forest) && empty(b.forest), "every refusal clears all public arenas and the order");
  }
  if (output) *output = fast;
  return a;
}
FullGabrielResult paired(const Cloud& c, unsigned k, bool lazy, u64 cache,
                        const FullGabrielLimits& caps, State* output = nullptr) {
  return paired(c.ix, k, c.catalogue[k], c.catalogue[k+1], lazy, cache, caps, output);
}
void expect_refusal(const FullGabrielResult& r, const char* reason,
                    FullGabrielStatus status = FullGabrielStatus::kResourceExhausted) {
  check(r.status == status && r.reason && std::strcmp(r.reason, reason) == 0 && empty(r.forest), reason);
}

unsigned direct_peers(const Cloud& c, unsigned k, unsigned target) {
  unsigned count = 0;
  for (const auto& e : c.catalogue[k+1])
    if (oracle::compare(e.level, c.exact.ball(target)) == 0) ++count;
  return count;
}
std::vector<unsigned> strict_parents(const Cloud& c, unsigned k, unsigned t) {
  const auto groups = c.exact.full_components(k, c.exact.ball(t), false);
  std::vector<unsigned> result;
  for (unsigned j = 0; j < c.in.size(); ++j) if (c.exact.ball(t).support & (1u << j)) {
    const unsigned f = t ^ (1u << j);
    bool found = false;
    for (unsigned i = 0; i < groups.size(); ++i)
      if (std::find(groups[i].begin(), groups[i].end(), f) != groups[i].end()) {
        result.push_back(i); found = true; break;
      }
    check(found, "oracle locates each strict facet in an active pre-lot Gamma component");
  }
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  return result;
}
State oracle_state(const Cloud& c, unsigned k) {
  State out;
  std::vector<unsigned> multi_levels;
  for (const auto& e : c.catalogue[k+1]) {
    const unsigned t = label(e, c.in), peers = direct_peers(c, k, t);
    if (peers > 1) {
      if (std::none_of(multi_levels.begin(), multi_levels.end(), [&](unsigned m) {
            return oracle::compare(c.exact.ball(t), c.exact.ball(m)) == 0;
          })) { multi_levels.push_back(t); ++out.multi_direct_lots; }
      continue;
    }
    const auto roots = strict_parents(c, k, t);
    check(peers == 1 && e.q >= 2 && e.q <= 4 && !roots.empty() && roots.size() <= e.q,
          "oracle singleton has one to q distinct old roots");
    ++out.eligible[e.q]; ++out.unique_roots[roots.size()];
    if (roots.size() < e.q) ++out.repeated_roots;
    if (std::any_of(c.catalogue[k].begin(), c.catalogue[k].end(), [&](const ForestEvent& m) {
          return oracle::compare(m.level, c.exact.ball(t)) == 0;
        })) ++out.simultaneous_births;
  }
  return out;
}
void qualify(Cloud& c, bool permute) {
  if (!c.valid) return;
  for (unsigned k = 1; k <= c.in.size(); ++k) {
    ++order_cases;
    const State wanted = oracle_state(c, k);
    for (int policy = 0; policy < 4; ++policy) {
      const bool lazy = policy != 0;
      const u64 cache = policy < 2 ? 0 : policy == 2 ? 1 : 1000000;
      context = c.name + "/K=" + std::to_string(k) + "/policy=" + std::to_string(policy);
      State state;
      const auto result = paired(c, k, lazy, cache, roomy(lazy), &state);
      if (result.status != FullGabrielStatus::kCompleteRelative) { check(false, "nominal call succeeds"); continue; }
      check(same_state(state, wanted), "observed singleton/group calendar equals independent Gamma calendar");
      accumulate(state);
      compare_oracle(c, k, result.forest);
      check(result.stats.geometry.core_records == 0 && result.stats.geometry.core_facets == 0 &&
            result.stats.geometry.added_cofaces == 0, "neither path materializes the Gamma core");
      if (k == c.in.size()) {
        ++terminal_orders;
        check(result.forest.nodes().size() == 1 && result.forest.minima().size() == 1 &&
              result.forest.parents().empty() && std::all_of(std::begin(state.eligible), std::end(state.eligible),
                  [](u64 v) { return v == 0; }), "K=n retains one minimum and has no direct lot");
      }
      if (permute && k == 2) {
        auto points = c.in; std::reverse(points.begin(), points.end());
        auto minima = c.catalogue[k], direct = c.catalogue[k+1];
        std::reverse(minima.begin(), minima.end()); std::reverse(direct.begin(), direct.end());
        const auto changed = paired(build_cloud_index(points), k, minima, direct, lazy, cache, roomy(lazy));
        check(changed.status == FullGabrielStatus::kCompleteRelative && same_forest(result.forest, changed.forest) &&
              work(result.stats) == work(changed.stats), "physical/catalogue permutations preserve forest and all counters");
        ++permutations;
      }
    }
  }
}

void noop_future(const Cloud& c, bool rejects) {
  if (!c.valid) return;
  context = "two_step/named_noop_future";
  constexpr unsigned t = 21, f = 5, s = 37;
  check(c.exact.direct(t) && c.exact.direct(s) && !c.exact.direct(f) &&
        direct_peers(c, 2, t) == 1 && strict_parents(c, 2, t).size() == 1,
        "T={0,2,4} is a unique-direct no-op, not merely a point-coverage equality");
  unsigned foreign = 0;
  for (unsigned j = 0; j < c.in.size(); ++j)
    if (!(f & (1u << j)) && c.exact.ball(f).power(c.in[j].position).sign() < 0) foreign |= 1u << j;
  check(foreign == (t ^ f) && oracle::compare(c.exact.ball(f), c.exact.ball(t)) == 0 &&
        (s & f) == f && ((s ^ f) & c.exact.ball(s).support) != 0 &&
        oracle::compare(c.exact.ball(t), c.exact.ball(s)) < 0,
        "F={0,2} has unique intruder4; later S={0,2,5} must consume the closed no-op T anchor");
  const auto r = paired(c, 2, true, 0, roomy(true));
  check(r.status == FullGabrielStatus::kCompleteRelative && r.stats.cache_inserts == 0 &&
        r.stats.no_op_connections > 0 && r.stats.singleton_intruder_resolutions > 0,
        "C0 forces physical J1 resolution of the independently named future strict request");
  ++named_noop_future;
  if (!rejects) return;
  auto missing = c.catalogue[3];
  missing.erase(std::remove_if(missing.begin(), missing.end(), [&](const ForestEvent& e) {
    return label(e, c.in) == t;
  }), missing.end());
  check(missing.size()+1 == c.catalogue[3].size(), "remove exactly the named no-op catalogue record");
  expect_refusal(paired(c.ix, 2, c.catalogue[2], missing, true, 0, roomy(true)),
                 "full_gabriel_terminal_missing", FullGabrielStatus::kInvariantViolated);
}

void proper_repeated(const Cloud& c) {
  if (!c.valid) return;
  context = "E5/named_proper_repeated";
  for (unsigned k = 2; k < c.in.size(); ++k)
    for (const auto& e : c.catalogue[k+1]) {
      const unsigned t = label(e, c.in);
      if (direct_peers(c, k, t) != 1) continue;
      const auto roots = strict_parents(c, k, t);
      if (!(roots.size() > 1 && roots.size() < e.q)) continue;
      ++named_proper_repeated;
      check(k == 3 && t == 30 && e.q == 3 && roots.size() == 2,
            "named E5 coface {1,2,3,4} has q=3 and exactly two strict Gamma parents at K=3");
      std::printf("named_proper_repeated scene=E5 K=%u T=%u q=%u U=%zu\n", k, t, e.q, roots.size());
    }
  check(named_proper_repeated == 1,
        "E5 has a named independent Gamma witness with 1<U<q, not a U=1 no-op");
}

void budgets(const Cloud& c, unsigned k) {
  if (!c.valid) return;
  for (int policy = 0; policy < 4; ++policy) {
    const bool lazy = policy != 0;
    const u64 cache = policy < 2 ? 0 : policy == 2 ? 1 : 1000000;
    context = c.name + "/caps/K=" + std::to_string(k) + "/policy=" + std::to_string(policy);
    const auto baseline = paired(c, k, lazy, cache, roomy(lazy));
    if (baseline.status != FullGabrielStatus::kCompleteRelative) { check(false, "budget sentinel completes"); continue; }
    struct Limit { u64 FullGabrielLimits::*field; u64 used; const char* reason; };
    const Limit fields[] = {
      {&FullGabrielLimits::max_points, c.in.size(), "full_gabriel_point_budget"},
      {&FullGabrielLimits::max_input_records, baseline.stats.input_records, "full_gabriel_input_budget"},
      {&FullGabrielLimits::max_face_visits, baseline.stats.face_visits, "full_gabriel_face_budget"},
      {&FullGabrielLimits::max_aliases, baseline.stats.aliases, "full_gabriel_alias_budget"},
      {&FullGabrielLimits::max_portal_requests, baseline.stats.portal_requests, "full_gabriel_portal_budget"},
      {&FullGabrielLimits::max_chain_steps, baseline.stats.chain_steps, "full_gabriel_chain_budget"},
      {&FullGabrielLimits::max_meb_calls, baseline.stats.meb_calls, "full_gabriel_meb_call_budget"},
      {&FullGabrielLimits::max_query_nodes, baseline.stats.geometry.query_nodes, "silent_query_node_budget"},
      {&FullGabrielLimits::max_meb_supports, baseline.stats.geometry.meb_supports, "silent_meb_support_budget"},
      {&FullGabrielLimits::max_successor_steps, baseline.stats.successor_steps, "full_gabriel_successor_budget"}};
    for (const auto& field : fields) {
      auto caps = roomy(lazy); caps.*field.field = field.used;
      const auto exact = paired(c, k, lazy, cache, caps);
      check(exact.status == FullGabrielStatus::kCompleteRelative && same_forest(exact.forest, baseline.forest) &&
            work(exact.stats) == work(baseline.stats), "exact observed logical cap preserves complete forest and counters");
      ++exact_caps;
      if (field.used == 0) continue;
      for (u64 short_cap : {u64{0}, field.used-1}) {
        caps.*field.field = short_cap;
        expect_refusal(paired(c, k, lazy, cache, caps), field.reason); ++short_caps;
      }
    }
    u64 batches = 0;
    for (size_t i = 0; i < baseline.forest.nodes().size(); ++i)
      if (i == 0 || !same_exact_level(baseline.forest.nodes()[i-1].level, baseline.forest.nodes()[i].level)) ++batches;
    struct CertificateLimit { u64 FullCertificateLimits::*field; u64 used; const char* reason; };
    const CertificateLimit cert[] = {
      {&FullCertificateLimits::max_batches, batches, "full_gabriel_batch_budget"},
      {&FullCertificateLimits::max_nodes, baseline.forest.nodes().size(), "full_gabriel_node_budget"},
      {&FullCertificateLimits::max_parent_refs, baseline.forest.parents().size(), "full_gabriel_parent_budget"}};
    for (const auto& field : cert) {
      auto caps = roomy(lazy); caps.certificate.*field.field = field.used;
      const auto exact = paired(c, k, lazy, cache, caps);
      check(exact.status == FullGabrielStatus::kCompleteRelative && same_forest(exact.forest, baseline.forest) &&
            work(exact.stats) == work(baseline.stats), "exact certificate budget has identical work");
      ++exact_caps;
      if (field.used == 0) continue;
      for (u64 short_cap : {u64{0}, field.used-1}) {
        caps.certificate.*field.field = short_cap;
        expect_refusal(paired(c, k, lazy, cache, caps), field.reason); ++short_caps;
      }
    }
  }
}

void partial_faces(const Cloud& c, unsigned k, unsigned q) {
  if (!c.valid) return;
  context = c.name + "/partial_faces";
  check(c.catalogue[k+1].size() == 1 && c.catalogue[k+1][0].q == q,
        "named q2/q3/q4 has exactly one direct in the entire order");
  for (int policy = 0; policy < 4; ++policy) {
    const bool lazy = policy != 0;
    const u64 cache = policy < 2 ? 0 : policy == 2 ? 1 : 1000000;
    for (u64 face = 0; face < q; ++face) {
      auto caps = roomy(lazy); caps.max_face_visits = face;
      State state;
      const auto r = paired(c, k, lazy, cache, caps, &state);
      expect_refusal(r, "full_gabriel_face_budget");
      check(r.stats.face_visits == face && std::all_of(std::begin(state.eligible), std::end(state.eligible),
          [](u64 x) { return x == 0; }), "every incomplete q-prefix refuses before singleton preparation or global union");
      ++partial_face_caps;
    }
  }
}

void repeated_parent_budget(const Cloud& c) {
  if (!c.valid) return;
  context = "E5/repeated_parent_budget";
  bool found = false;
  for (unsigned k = 2; k < c.in.size() && !found; ++k)
    for (const auto& e : c.catalogue[k+1]) {
      const unsigned t = label(e, c.in);
      if (direct_peers(c, k, t) != 1) continue;
      const auto roots = strict_parents(c, k, t);
      if (!(roots.size() > 1 && roots.size() < e.q)) continue;
      found = true;
      for (int policy = 0; policy < 4; ++policy) {
        const bool lazy = policy != 0;
        const u64 cache = policy < 2 ? 0 : policy == 2 ? 1 : 1000000;
        const auto baseline = paired(c, k, lazy, cache, roomy(lazy));
        if (baseline.status != FullGabrielStatus::kCompleteRelative) { check(false, "repeated-root sentinel completes"); continue; }
        u64 prior_refs = 0, through_faces = 0;
        bool later_direct = false;
        for (const auto& node : baseline.forest.nodes())
          if (oracle::compare(node.level, c.exact.ball(t)) < 0) prior_refs += node.parent_count;
        for (const auto& d : c.catalogue[k+1]) {
          if (oracle::compare(d.level, c.exact.ball(t)) <= 0) through_faces += d.q + (lazy ? 0 : k+1);
          else later_direct = true;
        }
        auto caps = roomy(lazy);
        caps.max_face_visits = through_faces;
        caps.certificate.max_parent_refs = prior_refs + roots.size() - 1;
        expect_refusal(paired(c, k, lazy, cache, caps), "full_gabriel_parent_budget");
        ++caps.certificate.max_parent_refs;
        const auto enough = paired(c, k, lazy, cache, caps);
        if (later_direct) expect_refusal(enough, "full_gabriel_face_budget");
        else check(enough.status == FullGabrielStatus::kCompleteRelative, "exact U budget passes the final repeated-root merge");
        check(enough.stats.face_visits == through_faces, "charge U, not q: exact U reaches the next face boundary");
        ++repeated_parent_caps;
      }
      break;
    }
  check(found && repeated_parent_caps == 4, "non-vacuous 1<U<q exact-parent budget in all four policies");
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || (std::strcmp(argv[1], "--selftest") != 0 && std::strcmp(argv[1], "--rejects") != 0)) return 2;
  const bool rejects = std::strcmp(argv[1], "--rejects") == 0;
  mhgp7_oracle::clear_overflow();
  try {
    Cloud single("single", input({{7,11,13}}, true)); qualify(single, false);
    Cloud acute("acute", input({{0,0,0},{6,0,0},{2,3,0}})); qualify(acute, false);
    Cloud obtuse("obtuse", input({{0,0,0},{6,0,0},{1,1,0}})); qualify(obtuse, false);
    Cloud tetra("tetra", input({{20,20,20},{24,24,20},{24,20,24},{20,24,24}})); qualify(tetra, true);
    Cloud mixed("mixed", input({{25,20,20},{17,24,20},{17,16,20},{120,20,20},{130,20,20}})); qualify(mixed, true);
    const std::vector<P3> e5{{0,0,7},{0,9,6},{1,4,0},{0,0,1},{4,1,2}};
    Cloud original("E5", input(e5)); qualify(original, true);
    Cloud sparse("E5_sparse_s10", input(e5, true), 10); qualify(sparse, true);
    const std::vector<P3> chain{{622,745,858},{839,341,867},{111,242,715},{827,10,537},
                              {437,578,984},{396,213,30},{693,305,961},{814,71,415}};
    Cloud two_step("two_step_s12", input(chain), 12); qualify(two_step, true);
    Cloud shared("J1_shared_lot", input({{0,50,0},{40,50,0},{20,61,0},{20,0,0},{20,10,30}})); qualify(shared, true);
    noop_future(two_step, rejects);
    proper_repeated(original);
    if (rejects) {
      budgets(original, 2); budgets(two_step, 2); budgets(tetra, 3); budgets(mixed, 2);
      partial_faces(obtuse, 2, 2); partial_faces(acute, 2, 3); partial_faces(tetra, 3, 4);
      repeated_parent_budget(original);
    }
  } catch (const std::exception& e) {
    ++failures; std::fprintf(stderr, "FAIL exception [%s]: %s\n", context.c_str(), e.what());
  }
  check(!mhgp7_oracle::overflow_seen(), "sticky independent OBig overflow remains clear");
  const bool floor = admitted_clouds == 9 && order_cases == 39 && permutations == 24 && terminal_orders == 36 &&
      observed.eligible[2] > 0 && observed.eligible[3] > 0 && observed.eligible[4] > 0 &&
      observed.unique_roots[1] > 0 && observed.unique_roots[4] > 0 && observed.repeated_roots > 0 &&
      observed.simultaneous_births > 0 && observed.multi_direct_lots > 0 && named_noop_future == 1 &&
      named_proper_repeated == 1 && observed.specialized_lots > 0 && observed.general_singleton_lots == 0 &&
      records >= 100 && cuts >= 1000 && (!rejects || (exact_caps == 208 && short_caps >= 200 &&
      partial_face_caps == 36 && repeated_parent_caps == 4));
  std::printf("full_gabriel_singleton mode=%s clouds=%llu orders=%llu pairs=%llu positives=%llu refused=%llu "
              "cuts=%llu records=%llu permutations=%llu terminals=%llu q2=%llu q3=%llu q4=%llu "
              "U1=%llu U4=%llu repeated=%llu mixed_births=%llu multi_direct=%llu noop_future=%llu "
              "proper_repeated=%llu specialized=%llu general_singleton=%llu "
              "exact_caps=%llu short_caps=%llu partial_faces=%llu repeated_parent_caps=%llu "
              "checks=%llu failures=%llu floor=%d\n", argv[1],
      (unsigned long long)admitted_clouds, (unsigned long long)order_cases, (unsigned long long)pairs,
      (unsigned long long)positive_pairs, (unsigned long long)refused_pairs, (unsigned long long)cuts,
      (unsigned long long)records, (unsigned long long)permutations, (unsigned long long)terminal_orders,
      (unsigned long long)observed.eligible[2], (unsigned long long)observed.eligible[3], (unsigned long long)observed.eligible[4],
      (unsigned long long)observed.unique_roots[1], (unsigned long long)observed.unique_roots[4],
      (unsigned long long)observed.repeated_roots, (unsigned long long)observed.simultaneous_births,
      (unsigned long long)observed.multi_direct_lots, (unsigned long long)named_noop_future,
      (unsigned long long)named_proper_repeated, (unsigned long long)observed.specialized_lots,
      (unsigned long long)observed.general_singleton_lots,
      (unsigned long long)exact_caps, (unsigned long long)short_caps, (unsigned long long)partial_face_caps,
      (unsigned long long)repeated_parent_caps, (unsigned long long)checks, (unsigned long long)failures, (int)floor);
  return failures ? 1 : floor ? 0 : 3;
}
